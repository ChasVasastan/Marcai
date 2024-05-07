# Marcai
A physical music box that calls a API connected to an AI music generator that gets fed different parameters from the box.

## Board setup
| Function  | PicoW Pin | Arduino Pin |
| --------- |---------- | ----------- |
| UART TX   | 1         | 0           |
| UART RX   | 0         | 1           |
| DHT Data  |           | 2           |
| I2C SDA   |           | SDA/A4      |
| I2C SCL   |           | SCL/A5      |
| BCLK      | 26        |             |
| LRC       | 27        |             |
| DIN       | 28        |             |

## Install toolchain

Follow instructions to install the [toolchain](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
for your system.

## Build

Clone the repository and run

```
export WIFI_SSID=YourWiFiSSID
export WIFI_PASS=YourSecretPassword
git submodule update --init
cmake -S . -B build
cmake --build build
```

After this, there is a .uf2 file in the build directory that you copy
to the Raspberry Pi Pico.

Optionally if [picotool](https://github.com/raspberrypi/picotool) is installed,
build target marcai_flash to flash the device.

```
cmake --build build --target marcai_flash
```
