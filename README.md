# modular-ev-bms
UVA ECE Fall 2020 Capstone Project design for a modular BMS for use in electric vehicles. 

## Setup
The instructions on the [UVA Solar Car Team Website](https://solarcaratuva.github.io/stm32-mbed-info) were used to help setup this project. The [PlatformIO IDE](https://docs.platformio.org/en/latest/integration/ide/pioide.html) was installed using a VSCode extension. Alternatively, the [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/installation.html) can be installed.

The ST Link Driver may be needed to upload the program to the Nucleo-F413ZH. This driver can be found [here](https://os.mbed.com/teams/ST/wiki/ST-Link-Driver).

## Authors

- William Zhang
- Dipesh Manandhar
- Nripesh Manandhar
- Nikilesh Subramaniam
- Phillip Phan

## Project Structure
The `include` and `src` directories are split into `CellNode`, `MainNode`, and `Shared` subdirectories. The `CellNode` subdirectories will only include .cpp and .h files specific to the Cell Node, while the `MainNode` subdirectories will only include .cpp and .h files specific to the Main Node. The `Shared` subdirectories will include .cpp and .h files that are common between the two boards.

All testing that can be tested on both boards will be located in `src/Shared/test_main.cpp`.