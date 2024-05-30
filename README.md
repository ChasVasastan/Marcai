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

## Connecting the unit to the internet

When you run the project on your pico w it will start in AP-mode, via your wifi manager select the network (Marcai) and enter the ipv4 manually (how you do this could vary, depending on your manager). As we do not run a DHCP server, please just connect one unit.
...
Data to be entered on your side.
IP:                 169, 254, 141, 158
Netmask(24 bit):    255, 255, 255, 0
Gateway:            169, 254, 141, 158
...
Once the data is entered, try to connect (SSID: Marcai, PASS: chas5678) and you should connected to the pico w. Now, open your browser of choice and enter http://169.254.141.158, be certain that your browser does not upgrade the link to a https connection, as it most likely wont connect then.

On the website you enter the name and password of the desired wifi connection to join and hit "Submit", and the unit should change from AP to STA mode and connect to your selected wifi.