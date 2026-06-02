import subprocess 
import sys 
from west.commands import WestCommand 
from west import log 

class ContainerBuild(WestCommand): 
    def __init__(self): 
        super().__init__( 
            'cbuild', 
            'run west build inside the classroom container', 
            'Wraps the ‘podman run’ cmd to execute west build in an environment.', 
            # This flag allows us to pass unknown arguments (-b pic32cm...) to CMake 
            accepts_unknown_args=True  
        )

    def do_add_parser(self, parser_adder): 
        # Hooks the command into the standard 'west help' menu 
        parser = parser_adder.add_parser(self.name, help=self.help, description=self.description) 
        return parser 

    def do_run(self, args, unknown_args): 
        # 1. Reconstruct the user's intended build command using 'unknown_args' 
        inner_cmd = "west build " + " ".join(unknown_args) 
        log.inf(f"Executing in container: {inner_cmd}") 

        # 2. Construct the Podman command using self.topdir (the absolute path to your workspace) 
        # This makes the command completely immune to what subfolder you are currently in! 
        podman_cmd = [ 
            "podman", "run", "--rm", 
            "-u", "root", 
            "--entrypoint", "bash", 
            "-v", f"{self.topdir}:/workdir:z", 
            "-w", "/workdir", 
            "ghcr.io/zephyrproject-rtos/zephyr-build:v0.29.2", 
            "-c", inner_cmd 
        ] 

        try: 
            # 3. Execute the command and stream the output to the terminal 
            subprocess.run(podman_cmd, check=True) 
            log.inf("Container build completed successfully.") 
        except subprocess.CalledProcessError as e: 
            log.err(f"Container build failed with exit code {e.returncode}") 
            sys.exit(e.returncode) 