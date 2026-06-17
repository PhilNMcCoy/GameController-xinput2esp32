# GameController-xinput2esp32
Use gamepad connected to PC to control an ESP32 device over UDP (e.g. PC "driver station" controlling an ESP-32 robot via wifi)

Usage:
1. Add the following to your Arduino sketch:
```C++
#include "gamepadUDP.h"
```
2. Set up a network connection (e.g. using WiFi.h from the Arduino environment)
3. Use the public methods from the GameController object to query the status of the various gamepad buttons/joysticks/triggers.
This is illustrated in the gamepad_EXP32.ino example Arduino sketch.
4. On the PC (Windows or Linux), compile and execute the NetInput.Capture program to send joystick events to the Arduino/ESP32 board.
```text
Usage:  ./NetInput.Capture -i <ip address> [-p <port>][-j <joystick number>] [-t <dead zone threshold>]
```
