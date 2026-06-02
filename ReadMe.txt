1. switch env
.venv\Scripts\activate.ps1

2. init custom git
west init -m http://github.com/MicrochipTech/zephyr-advanced-workflows

3. to build
podman run --rm -u root --entrypoint bash -v ${PWD}:/workdir:z -w /workdir ghcr.io/zephyrproject-rtos/zephyr-build:v0.29.2 -c "west build -p always -b pic32cm_pl10_cnano app"