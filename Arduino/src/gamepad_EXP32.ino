/*
UDP server to receive UDP packets and respond.

This is intended to be a starting point for robot code
to receive game controller commands from a PC over WiFi.
*/

#include <Arduino.h>
#include <WiFi.h>
#include "gamepadUDP.h"

#if __has_include("wifi_network.h")
// local file with wifi network name/password
#include "wifi_network.h"
#else
// dummy placeholders; adjust to match your network
#define NETWORK "my_wifi_network_name"
#define PASS "my_wifi_network_password"
#endif

#ifndef LED_BUILTIN
// LED is on pin 2 on the ESP32 Dev board 
#define LED_BUILTIN 2
#endif

// TODO use ESP32 in AP (access point) mode, instead of connecting
// to an external WIFI access point?  Driver station PC would then
// connect directly to the robot's WiFi access point.

// Set up a WiFi connection and print some diagnostic messages
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(NETWORK, PASS);
  Serial.print("Connecting to WiFi network ");
  Serial.print(NETWORK);
  Serial.print("\n");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("\n");
  Serial.print("IP Address:  ");
  Serial.println(WiFi.localIP());
}


void setup() {
  Serial.begin(115200);
  initWiFi();
  GameController.initUDPServer();
}

XINPUT_GAMEPAD prevstate;

void loop() {
  // drive based on gamepad command
  // UDP server will asynchronously update the gamepad object
  // with latest gamepad events

  if ( // check if there is any update from gamepad
    prevstate.wButtons      != GameController.buttons() ||
    prevstate.bLeftTrigger  != GameController.ltrig() ||
    prevstate.bRightTrigger != GameController.rtrig() ||
    prevstate.sThumbLX      != GameController.ljoyx() ||
    prevstate.sThumbLY      != GameController.ljoyy() ||
    prevstate.sThumbRX      != GameController.rjoyx() ||
    prevstate.sThumbRY      != GameController.rjoyy()
  ){

    analogWrite(LED_BUILTIN, ((GameController.ljoyx()+32768)/256));

    // gamecontroller state is updated by callback from UDP packet reception
    Serial.printf("buttons: %04x ",        GameController.buttons());
    Serial.printf("trigs: L %02x R %02x ", GameController.ltrig(), GameController.rtrig());
    Serial.printf("Ljoy: X % 04d Y % 04d ",  GameController.ljoyx(), GameController.ljoyy());
    Serial.printf("Rjoy: X % 04d Y % 04d",   GameController.rjoyx(), GameController.rjoyy());
    Serial.println();
  }

  prevstate.wButtons      = GameController.buttons();
  prevstate.bLeftTrigger  = GameController.ltrig();
  prevstate.bRightTrigger = GameController.rtrig();
  prevstate.sThumbLX      = GameController.ljoyx();
  prevstate.sThumbLY      = GameController.ljoyy();
  prevstate.sThumbRX      = GameController.rjoyx();
  prevstate.sThumbRY      = GameController.rjoyy();

  delay(10);
}
