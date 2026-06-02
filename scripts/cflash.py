import subprocess 
import sys 
from west.commands import WestCommand 
from west import log 

class ContainerFlash (WestCommand): 
    def __init__(self): 
        super().__init__( 
            'cflash', 
            'Flash the board bypassing the container paths', 
            'Wraps the native west flash command with the --no-rebuild flag automatically.', 
            accepts_unknown_args=True 
        ) 

    def do_add_parser(self, parser_adder): 
        parser = parser_adder.add_parser(self.name, help=self.help, description=self.description) 
        return parser 

    def do_run(self, args, unknown_args): 
        # Automatically inject --no-rebuild before appending the user's custom arguments 
        flash_cmd = ["west", "flash", "--no-rebuild"] + unknown_args 
        log.inf(f"Executing native flash: {' '.join(flash_cmd)}") 

        try: 
            subprocess.run(flash_cmd, check=True) 
            log.inf("Flash completed successfully.") 
        except subprocess.CalledProcessError as e: 
            log.err(f"Flash failed with exit code {e.returncode}") 
            sys.exit(e.returncode) 