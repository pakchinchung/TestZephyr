#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdio.h>

#define MAX_INVENTORY 5

/* --- Devicetree LED Setup --- */
#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* --- Data Structures --- */

typedef struct Item {
    const char *name;
    const char *description;
} Item;

typedef struct Room {
    const char *name;
    const char *description;
    struct Room *north;
    struct Room *south;
    struct Room *east;
    struct Room *west;
    Item *room_item;
} Room;

/* --- Game State --- */

Item baton = {"baton", "A fiberglass conductor's baton."};
Item sheet_music = {"music", "The conductor's score for act one. Essential!"};

Room pit, stage, wings, green_room;
Room *current_room;

Item *inventory[MAX_INVENTORY];
int inventory_count = 0;

/* --- Initialization --- */

void init_world(void) {
    // Define Rooms
    pit.name = "Orchestra Pit";
    pit.description = "The stands are empty. Curtain is in 10 minutes, but you are missing the score. Exits: North.";
    pit.room_item = NULL;

    stage.name = "Main Stage";
    stage.description = "The stage is dark. The set pieces cast long shadows. Exits: South, East.";
    stage.room_item = NULL;

    wings.name = "Stage Right Wings";
    wings.description = "A tangle of ropes, pulleys, and nervous energy. Exits: West, North.";
    wings.room_item = &baton;

    green_room.name = "The Green Room";
    green_room.description = "It smells like hairspray and panic. Exits: South.";
    green_room.room_item = &sheet_music;

    // Link the map pointers
    pit.north = &stage;
    pit.south = NULL; pit.east = NULL; pit.west = NULL;

    stage.south = &pit;
    stage.east = &wings;
    stage.north = NULL; stage.west = NULL;

    wings.west = &stage;
    wings.north = &green_room;
    wings.south = NULL; wings.east = NULL;

    green_room.south = &wings;
    green_room.north = NULL; green_room.east = NULL; green_room.west = NULL;

    // Set starting location
    current_room = &pit;
    
    // Clear inventory
    for (int i = 0; i < MAX_INVENTORY; i++) {
        inventory[i] = NULL;
    }
    inventory_count = 0; 
}

/* --- Shell Commands --- */

static int cmd_look(const struct shell *sh, size_t argc, char **argv) {
    shell_print(sh, "\n[%s]", current_room->name);
    shell_print(sh, "%s", current_room->description);
    
    if (current_room->room_item != NULL) {
        shell_print(sh, "There is a [%s] here.", current_room->room_item->name);
    }
    shell_print(sh, "");
    return 0;
}

static int cmd_go(const struct shell *sh, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(sh, "Go where? (north, south, east, west)");
        return -EINVAL;
    }

    Room *next_room = NULL;
    const char *direction = argv[1];

    if (strcmp(direction, "north") == 0) {
        next_room = current_room->north;
    } else if (strcmp(direction, "south") == 0) {
        next_room = current_room->south;
    } else if (strcmp(direction, "east") == 0) {
        next_room = current_room->east;
    } else if (strcmp(direction, "west") == 0) {
        next_room = current_room->west;
    } else {
        shell_error(sh, "Unknown direction.");
        return -EINVAL;
    }

    if (next_room == NULL) {
        shell_print(sh, "You bump into a wall. You can't go that way.");
    } else {
        current_room = next_room;
        cmd_look(sh, 1, NULL); // Automatically look when entering a new room
    }
    
    return 0;
}

static int cmd_take(const struct shell *sh, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(sh, "Take what?");
        return -EINVAL;
    }

    const char *target_item = argv[1];

    if (current_room->room_item == NULL) {
        shell_print(sh, "There is nothing here to take.");
        return 0;
    }

    if (strcmp(current_room->room_item->name, target_item) == 0) {
        if (inventory_count >= MAX_INVENTORY) {
            shell_error(sh, "Your inventory is full!");
            return 0;
        }
        
        // Add to inventory array
        inventory[inventory_count] = current_room->room_item;
        inventory_count++;
        
        shell_print(sh, "You picked up the %s.", current_room->room_item->name);
        
        // Remove from room
        current_room->room_item = NULL;
        
        // Win condition check
        if (strcmp(target_item, "music") == 0) {
            shell_print(sh, "\n*** CONGRATULATIONS! ***");
            shell_print(sh, "You found the score! The overture can begin. You win!");
            
            // --- VICTORY LED FLICKER ---
            if (device_is_ready(led.port)) {
                for (int i = 0; i < 10; i++) {
                    gpio_pin_toggle_dt(&led);
                    k_msleep(100); // Block for 100ms
                }
                gpio_pin_set_dt(&led, 0); // Ensure it is turned off at the end
            }
        }
    } else {
        shell_print(sh, "You don't see a '%s' here.", target_item);
    }

    return 0;
}

static int cmd_inventory(const struct shell *sh, size_t argc, char **argv) {
    shell_print(sh, "\nInventory:");
    if (inventory_count == 0) {
        shell_print(sh, "  (empty)");
    } else {
        for (int i = 0; i < inventory_count; i++) {
            shell_print(sh, "  - %s: %s", inventory[i]->name, inventory[i]->description);
        }
    }
    shell_print(sh, "");
    return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char **argv) {
    init_world();
    shell_print(sh, "\n*** TIMELINE RESET ***");
    shell_print(sh, "The stage manager yelled 'PLACES!' and reset the timeline.");
    shell_print(sh, "Your inventory has been cleared.\n");
    cmd_look(sh, 1, NULL); // Show the starting room immediately
    return 0;
}

/* --- Zephyr Shell Registration --- */

SHELL_CMD_ARG_REGISTER(look, NULL, "Look around the current room", cmd_look, 1, 0);
SHELL_CMD_ARG_REGISTER(go, NULL, "Move direction (north, south, east, west)", cmd_go, 2, 0);
SHELL_CMD_ARG_REGISTER(take, NULL, "Take an item (e.g., 'take music')", cmd_take, 2, 0);
SHELL_CMD_ARG_REGISTER(inv, NULL, "Check your inventory", cmd_inventory, 1, 0);
SHELL_CMD_ARG_REGISTER(reset, NULL, "Reset the game back to the beginning", cmd_reset, 1, 0);

int main(void) {
    init_world();

    // Configure the LED as output
    if (gpio_is_ready_dt(&led)) {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    }
    
    // The Zephyr Shell starts automatically in the background on the UART.
    printk("\n\n======================================\n");
    printk("     THEATER PANIC: THE ADVENTURE     \n");
    printk("======================================\n");
    printk("Type 'look' to see where you are.\n");
    printk("Type 'go <direction>' to move.\n");
    printk("Type 'reset' to start over.\n");
    printk("======================================\n\n");

    return 0;
}