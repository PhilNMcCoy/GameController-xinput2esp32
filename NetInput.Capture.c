#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>

// Should match XINPUT_GAMEPAD from Winows Xinput.h
// since the ESP32 code uses the same.
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

// map SDL button numbering to XInput
int xinput_map(SDL_GameControllerButton b){
  switch (b) {
    case SDL_CONTROLLER_BUTTON_A:             return 12; break;
    case SDL_CONTROLLER_BUTTON_B:             return 13; break;
    case SDL_CONTROLLER_BUTTON_X:             return 14; break;
    case SDL_CONTROLLER_BUTTON_Y:             return 15; break;
    case SDL_CONTROLLER_BUTTON_BACK:          return  5; break;
    case SDL_CONTROLLER_BUTTON_GUIDE:         return 11; break; // FIXME how does windows xinput map this?
    case SDL_CONTROLLER_BUTTON_START:         return  4; break;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return  6; break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return  7; break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return  8; break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return  9; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:       return  0; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return  1; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return  2; break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return  3; break;
    case SDL_CONTROLLER_BUTTON_MISC1:         return 11; break; // ??
    case SDL_CONTROLLER_BUTTON_PADDLE1:       return 11; break; // ??
    case SDL_CONTROLLER_BUTTON_PADDLE2:       return 11; break; // ??
    case SDL_CONTROLLER_BUTTON_PADDLE3:       return 11; break; // ??
    case SDL_CONTROLLER_BUTTON_PADDLE4:       return 11; break; // ??
    case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return 11; break; // ??
    default:                                  return 11; break; // ??
  }
}

int main(int argc, char* argv[]) {
    char empty = 0;
    char *ipaddr = &empty; // IP address of receiver
    int port     = 4313;   // UDP port
    int jsnum    = 0;      // which joystick to capture (default: 0)
    int dzth     = 3500;   // minimum axis threshold to avoid spurious events
                           // when the joystick is in the "dead zone" - not
                           // quite at zero when not being pushed.
                           // In my small sample set, an X-box controller needed
                           // a much larger dead zone than a generic gamepad.

    // command line arguments
    int opt;
    while((opt = getopt(argc, argv, "i:p:j:t:"))!=-1){
      switch (opt) {
        case 'i':
          ipaddr = optarg;
          break;
        case 'p':
          port = atoi(optarg);
          break;
        case 'j':
          jsnum = atoi(optarg);
          break;
        case 't':
          dzth = atoi(optarg);
          break;
      }
    }
    // ipaddr must be given on cmdline
    if (ipaddr[0] == 0) {
      printf("Usage:  %s -i <ip address> [-p <port>][-j <joystick number>] [-t <dead zone threshold>]\n",
          argv[0]);
      return 0;
    }

    // Initialize SDL Subsystems
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Optional: Load community controller database if available locally
    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    SDL_GameController* controller = NULL;

    // Attempt to open the specified gamepad
    int num_joysticks = SDL_NumJoysticks();
    if ((SDL_IsGameController(jsnum)) && (controller = SDL_GameControllerOpen(jsnum))){
      printf("\n--- Using Controller %d ---\n", jsnum);
      printf("Name:      %s\n", SDL_GameControllerName(controller));
      printf("Mapping:   %s\n", SDL_GameControllerMapping(controller));
      printf("Press Ctrl+C or close the window to exit.\n\n");
    } else {
      printf("Failed to use Controller %d\n", jsnum);
      return EXIT_FAILURE;
    }

    int sock;
    struct sockaddr_in server_addr;
    int pkt_count = 0;

    // Create a UDP socket (SOCK_DGRAM) and configure server address
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    server_addr.sin_family = AF_INET;
    //server_addr.sin_port = htons(4313); // Destination port
    server_addr.sin_port = htons(port); // Destination port
    //server_addr.sin_addr.s_addr = inet_addr(ESP32_IP); // Destination IP
    server_addr.sin_addr.s_addr = inet_addr(ipaddr); // Destination IP

    // gamepad button state to transmit
    XINPUT_GAMEPAD gamepad_state = (XINPUT_GAMEPAD){0};

    SDL_Event event;
    bool quit = false;

    // Ignore redundant joystick events and axis in "dead zone" events
    bool ignore_event;
    bool dz;
    bool ltz = 1;
    bool rtz = 1;
    bool ljx = 1;
    bool ljy = 1;
    bool rjx = 1;
    bool rjy = 1;

    // Main Event Loop
    while (!quit) {
      ignore_event = false;
      // SDL_WaitEvent puts the thread to sleep until an event occurs (0% CPU usage)
      // Send a redundant event on timeout, to compensate for any dropped packets
      //if (SDL_WaitEvent(&event)) {
      if (SDL_WaitEventTimeout(&event, 10)) {
        switch (event.type) {
          case SDL_QUIT:
            // apparently SDL catches Ctrl-C and generates SDL_QUIT event.
            quit = true;
            break;
          case SDL_CONTROLLERBUTTONDOWN:
//            printf("[BUTTON DOWN] ID: %-2d | Name: %s xinput: %d\n", 
//              event.cbutton.button, 
//              SDL_GameControllerGetStringForButton(event.cbutton.button),
//              xinput_map(event.cbutton.button)
//              );
            gamepad_state.wButtons |= 1 << xinput_map(event.cbutton.button);
            break;
          case SDL_CONTROLLERBUTTONUP:
//            printf("[BUTTON UP]   ID: %-2d | Name: %s xinput: %d\n", 
//              event.cbutton.button, 
//              SDL_GameControllerGetStringForButton(event.cbutton.button),
//              xinput_map(event.cbutton.button)
//              );
            gamepad_state.wButtons &= ~(1 << xinput_map(event.cbutton.button));
            break;
          case SDL_CONTROLLERAXISMOTION:
            // Looks like joystick Y-axes are reversed!
            //printf("[AXIS MOTION] ID: %-2d | Name: %-12s | Value: %d\n", 
            //event.caxis.axis, 
            //SDL_GameControllerGetStringForAxis(event.caxis.axis),
            //event.caxis.value);
            // Only transmit if movement crosses deadzone threshold
            dz = (abs(event.caxis.value) < dzth);
            // value is 16b signed int (Sint16); triggers are only positive
            switch (event.caxis.axis) {
              // convert trigger to smaller data type from xinput
              case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                gamepad_state.bLeftTrigger = (unsigned char)(dz ? 0 : (event.caxis.value >> 7) & 0x00ff);
                ignore_event = dz && ltz;
                ltz = dz;
                break;
              case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                gamepad_state.bRightTrigger = (unsigned char)(dz ? 0 : (event.caxis.value >> 7) & 0x00ff);
                ignore_event = dz && rtz;
                rtz = dz;
                break;
              // joystick values are already in a compatible type
              // For some reason, Y values are negated
              // TODO check X values against windows
              case SDL_CONTROLLER_AXIS_LEFTX:
                gamepad_state.sThumbLX = dz ? 0 : event.caxis.value;
                ignore_event = dz && ljx;
                ljx = dz;
                break;
              case SDL_CONTROLLER_AXIS_LEFTY:
                gamepad_state.sThumbLY = dz ? 0 : -event.caxis.value;
                ignore_event = dz && ljy;
                ljy = dz;
                break;
              case SDL_CONTROLLER_AXIS_RIGHTX:
                gamepad_state.sThumbRX = dz ? 0 : event.caxis.value;
                ignore_event = dz && rjx;
                rjx = dz;
                break;
              case SDL_CONTROLLER_AXIS_RIGHTY:
                gamepad_state.sThumbRY = dz ? 0 : -event.caxis.value;
                ignore_event = dz && rjy;
                rjy = dz;
                break;
            }
            break;
            default:
              // mostly redundant SDL_JOY* events
              // (button sends both SDL_JOYBUTTONDOWN and SDL_CONTROLLERBUTTONDOWN)
              ignore_event = true;
        }
      }
      // send
      if (!ignore_event){
//        printf("Sending UDP %d\n", pkt_count++);
        sendto(sock, &gamepad_state, sizeof(gamepad_state), 0, 
             (struct sockaddr*)&server_addr, sizeof(server_addr));
      }
    }

    // Cleanup
    if (controller) {
        SDL_GameControllerClose(controller);
    }
    SDL_Quit();
    return EXIT_SUCCESS;
}

