# Management of ws2812b address tape using ESP8266 via Web interface
![prev](https://i.ibb.co/fxM5H6V/led-min.jpg)
## Description of the project
##### Management:
* By WiFi or Ethernet port

##### Peculiarities:
- Permanent TCP connection
- Duration of operation with the worst WiFi signal is not more than 500ms
- Possibility of asynchronous control
- Convenient colorpicker ([author](https://github.com/NC22/HTML5-Color-Picker))
- There are **29** cool effects
- Smooth color change
- Smooth brightness control

## Materials and components
-ESP8266
-ws2812b
- ESP pin overload resistor (optional)
- 5-volt power supply (capacity depending on the number of pixels in the tape)
- jumper wires
- One of the latest versions of the Arduino IDE along with:
   - [Packages for ESP8266](https://github.com/esp8266/Arduino)
   - [ESP8266FS filesystem plugin](https://github.com/esp8266/arduino-esp8266fs-plugin) (used to upload HTML, JS, CSS files to ESP)
    - Websockets library (available from library manager)
    - FastLed library (available from library manager)
## Wiring diagram
![scheme](https://i.ibb.co/TMm0gJx/esp-ws2812b.png)
## Installation
- Connect ESP8266 to your computer
- Open `ESP8266-LED.ino` and update network settings for your network
- Upload sketch
- From the top menu of the IDE, select tools -> ESP8266 Sketch Upload to upload web files from the `data` directory.
- Open the serial port monitor (if connected successfully, your IP will be displayed).
- Navigate to IP address and enjoy )

## Ahtung !
![button](https://i.ibb.co/wzt967C/dan.png)
The marked functions with this color have a long cycle and with a quick change in brightness, the ESP8266 is heavily loaded and delays can be observed!
I recommend that you first set the brightness before using these functions.
