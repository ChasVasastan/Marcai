# Marcai
A physical music box that is designed to collect data via sensors on a [arduino](https://github.com/ChasVasastan/SensorsArduino) which then sends the data to an raspberry pi pico w,which calls a API connected to an AI music generator that gets fed different parameters from the box. Ultimately the song is saved and streamed from a azure databse and played via the pico.

In its current form, the project does not actually make API calls which generate songs, but instead accesses a database which has pre-generated songs. The songs have different tags on them which are used when making a call with the sensor data.

## Features

- **Media Playback Management**: Play, stop, next and previous are functions used to navigate the media library or API calls with the appropriate sensory data.

- **Media**: Plays AI generated songs and pictures corresponding to the media.

- **Access point**: Set up the device as a wifi access point for intial configuration.

- **Wifi Configuration**: Configure wifi settings through a web interface and store credentials in the flash memory.

- **Button Controls**: Use of physical buttons to control media playback.

## TODO

- **Implement A Music Generation API**: Change the API call to access a real music generation service.
- **Enchance Web Interface**: Create a visually more appealing web interface.
- **Improve Error Handling**: Add error handling which manages edge cases i.e for the https connection.
- **Create extensions**: Create extensions which affect the API call to the AI Music generation service.
- **Expand Hardware Support**: Extend the support for more sensors and other tools which improve the product.
- **User Profiles**: Enable the user to save different generated songs as a preference for different sensory keywords.

## Table of Contents

- [Features](#features)
- [TODO](#todo)
- [Hardware Requirements](#hardware-requirements)
    - [Board Setup](#board-setup)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
  - [Connecting the Unit to the Internet](#connecting-the-unit-to-the-internet)
  - [Media Playback](#media-playback)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

## Hardware Requirements

- Raspberry Pi Pico w
- 4 buttons for physical control
- DAC for mono sound
- Speaker 8ohm 3W
- 1.3'' LCD Screen(ST7789) on a breakout
- Misc things such as cables, resistors etc.

See the [arduino](https://github.com/ChasVasastan/SensorsArduino) repository for the sensors.

### Board Setup
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

## Software Requirements

- Pico SDK
- C++ complier
- Build tools apropriate for development on the rp2040 board

Majority of the libraries are intergrated into the project


## Features

- **Media Playback Management**: Play, stop, skip, and navigate through your media library.
- **Wi-Fi Configuration**: Configure Wi-Fi settings through a web interface and store credentials in flash memory.
- **Access Point Mode**: Set up the device as a Wi-Fi access point for initial configuration.
- **Button Controls**: Use physical buttons to control media playback.


1. **Clone the repository**:
    ```sh
    git clone https://github.com/ChasVasastan/Marcai.git
    cd Marcai
    ```

2. **Initialize and update submodules** (if any):
    ```sh
    git submodule update --init --recursive
    ```
3. **Set up the Pico SDK**:
    Follow the [Pico SDK setup instructions](https://github.com/raspberrypi/pico-sdk) to install and configure the SDK.

4. **Build the project**:
    ```sh
    mkdir build
    cd build
    cmake ..
    make
    ```

5. **Flash the firmware**:
    After this, there is a .uf2 file in the build directory that you copy
    to the Raspberry Pi Pico w.

    Optionally if [picotool](https://github.com/raspberrypi/picotool) is installed,
    build target marcai_flash to flash the device.

## Usage

### Connecting the unit to the internet

When you run the project on your pico w it will start in AP-mode, via your wifi manager on your OS select the network (Marcai) and enter the ipv4 manually (how you do this could vary, depending on your manager). As we do not run a DHCP server, please just connect one unit.

Data to be entered on the users side.
```
IP:                 169, 254, 141, 158
Netmask(24 bit):    255, 255, 255, 0
Gateway:            169, 254, 141, 158
```

Once the data is entered, try to connect (SSID: Marcai, PASS: chas5678) and you should connected to the pico w. Now, open your browser of choice and enter http://169.254.141.158, be certain that your browser does not upgrade the link to a https connection, as it most likely wont connect then.

On the website you enter the name and password of the desired wifi connection to join and hit "Submit", and the unit should change from AP to STA mode and connect to your selected wifi.

### Media Playback

- **Control Buttons**:
    - Button 1: Play
    - Button 2: Stop
    - Button 3: Next Track
    - Button 4: Previous Track

## Contributing

TBD

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [Raspberry Pi](https://www.raspberrypi.org/) for the Pico and SDK
- [lwIP](https://savannah.nongnu.org/projects/lwip/) for the lightweight TCP/IP stack
- [mbedtls](https://github.com/Mbed-TLS/mbedtls) for the flexible TLS connection
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) for development support
- [ezxml](https://github.com/lxfontes/ezxml) for xml handling
- [libspng](https://libspng.org/) for help with decoding png
- [miniz](https://github.com/richgel999/miniz) for compression
- [picomp3lib](https://github.com/ikjordan/picomp3lib) for help of decoding mp3
