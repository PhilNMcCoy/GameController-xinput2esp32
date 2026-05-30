/*
Receive Game Controller commands from a PC via UDP.
Assumes a network connection (e.g. WiFi) is already present.
The PC should be running the NetInput Capture program
to transmit Game Controller events.
*/

/*
BSD 3-Clause License

Copyright (c) 2026, Philip McCoy

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "AsyncUDP.h"

// Should match XINPUT_GAMEPAD from Winows Xinput.h
typedef struct _XINPUT_GAMEPAD
{
    unsigned short                      wButtons;
    unsigned char                       bLeftTrigger;
    unsigned char                       bRightTrigger;
    short                               sThumbLX;
    short                               sThumbLY;
    short                               sThumbRX;
    short                               sThumbRY;
} XINPUT_GAMEPAD;

// Gamepad class - holds current state of the gamepad
class xinput_gamepad
{
  public:
    // constructor
    xinput_gamepad() { state = (XINPUT_GAMEPAD)0; }

    // Hook up callback function to process UDP packets
    // with game controller update events from NetInput Capture
    // running on the PC
    void initUDPServer() {
      if (udp.listen(udp_port)) {
        // hook callback to handle received packets
        udp.onPacket(
          [this](AsyncUDPPacket packet) {
          // update controller state based on UDP command
          this->updateControllerState(packet.data(), packet.length());
          }
        );
      }
    }

    // callback function to update the gamecontroller state object
    // when a UDP packet is received
    void updateControllerState(const uint8_t *buffer, size_t size){
      // copy controller state from UDP packet
      // Note:  Some kind of zero-copy would be more efficient, but not sure how
      // the UDP code allocates/deallocates the buffer.
      if (sizeof(XINPUT_GAMEPAD) == size){
        state.wButtons      = buffer[1]  << 8 | buffer[0];
        state.bLeftTrigger  = buffer[2];
        state.bRightTrigger = buffer[3];
        state.sThumbLX      = buffer[5]  << 8 | buffer[4];
        state.sThumbLY      = buffer[7]  << 8 | buffer[6];
        state.sThumbRX      = buffer[9]  << 8 | buffer[8];
        state.sThumbRY      = buffer[11] << 8 | buffer[10];
      }
    };

    // methods to get current state of controller
    bool b1 () {return ((state.wButtons >> sel_b1 ) & 1) == 1;}
    bool b2 () {return ((state.wButtons >> sel_b2 ) & 1) == 1;}
    bool b3 () {return ((state.wButtons >> sel_b3 ) & 1) == 1;}
    bool b4 () {return ((state.wButtons >> sel_b4 ) & 1) == 1;}
    bool b5 () {return ((state.wButtons >> sel_b5 ) & 1) == 1;}
    bool b6 () {return ((state.wButtons >> sel_b6 ) & 1) == 1;}
    bool b7 () {return ((state.wButtons >> sel_b7 ) & 1) == 1;}
    bool b8 () {return ((state.wButtons >> sel_b8 ) & 1) == 1;}
    bool b9 () {return ((state.wButtons >> sel_b9 ) & 1) == 1;}
    bool b10() {return ((state.wButtons >> sel_b10) & 1) == 1;}
    bool b11() {return ((state.wButtons >> sel_b11) & 1) == 1;}
    bool b12() {return ((state.wButtons >> sel_b12) & 1) == 1;}
    bool b13() {return ((state.wButtons >> sel_b13) & 1) == 1;}
    bool b14() {return ((state.wButtons >> sel_b14) & 1) == 1;}
    bool b15() {return ((state.wButtons >> sel_b15) & 1) == 1;}
    bool b16() {return ((state.wButtons >> sel_b16) & 1) == 1;}
  
    // aliases for controller buttons
    // (button names could vary between different game controller models?)
    bool b_a      () {return  b1();}
    bool b_b      () {return  b2();}
    bool b_c      () {return  b3();}
    bool b_d      () {return  b4();}
    bool b_L      () {return  b5();}
    bool b_R      () {return  b6();}
    bool b_select () {return  b7();}
    bool b_start  () {return  b8();}
    bool b_LJ     () {return  b9();}
    bool b_RJ     () {return b10();}
    bool b_dp_up  () {return b11();}
    bool b_dp_dn  () {return b12();}
    bool b_dp_lf  () {return b13();}
    bool b_dp_rt  () {return b14();}

    int ltrig () {return (int)(state.bLeftTrigger);}
    int rtrig () {return (int)(state.bRightTrigger);}
    int ljoyx () {return (int)(state.sThumbLX);}
    int ljoyy () {return (int)(state.sThumbLY);}
    int rjoyx () {return (int)(state.sThumbRX);}
    int rjoyy () {return (int)(state.sThumbRY);}

    // in case user wants bigger chunks
    int dpad () {return (int)(state.wButtons & 0x0f);}
    int buttons () {return (int)(state.wButtons);}

  private:
    // game controller state
    XINPUT_GAMEPAD state;

    // bit positions for button bitmap
    const int sel_b1  = 12; // 0x1000 // a
    const int sel_b2  = 13; // 0x2000 // b
    const int sel_b3  = 14; // 0x4000 // c
    const int sel_b4  = 15; // 0x8000 // d
    const int sel_b5  =  8; // 0x0100 // L
    const int sel_b6  =  9; // 0x0200 // R
    const int sel_b7  =  5; // 0x0020 // Select
    const int sel_b8  =  4; // 0x0010 // Start
    const int sel_b9  =  6; // 0x0040 // LJ
    const int sel_b10 =  7; // 0x0080 // RJ
    const int sel_b11 =  0; // 0x0001 // D-pad up
    const int sel_b12 =  1; // 0x0002 // D-pad down
    const int sel_b13 =  2; // 0x0004 // D-pad left
    const int sel_b14 =  3; // 0x0008 // D-pad right
    const int sel_b15 = 10; // 0x0400 // Unused?
    const int sel_b16 = 11; // 0x0800 // Unused?
 
    // UDP connection from AsyncUDP.h
    const int udp_port = 4313;
    AsyncUDP udp;
};

// Main program will use this object to access the game controller
xinput_gamepad GameController;
