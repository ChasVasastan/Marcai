## File structure

- **marcai.cpp**
  - entry point and main loop
- **audio.cpp**
  - Audio processing and mp3 decode
- **http_request.cpp**
  - Handling of HTTP requests
- **media_manager.cpp**
  - Manager for audio and display
- **screen.cpp**
  - LCD screen functionallity
- **image.cpp**
  - Image conversion of RGB pixel data for the screen
- **wifi.cpp**, **wifi_config.cpp**, **cgi.cpp**, **write_flash.cpp**
  - Initial setup of wifi that starts as an access point and lets the
    user enter an ssid and password to connect to
- **serial.cpp**
  - Serial communication with Arduino UNO
