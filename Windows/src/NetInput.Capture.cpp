#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <xinput.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <objbase.h>

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <stdint.h>
#include <fstream>

using namespace std;



int main(int argc, char* argv[])
{
  
 XINPUT_STATE prev_state = {0};
 XINPUT_STATE state;

  string ipaddr;   // IP address of receiver
  int port    = 4313;   // UDP port
  int jsnum   = 0;      // which joystick to capture (default: 0)
  int dzth    = 3500;   // minimum axis threshold to avoid spurious events
                        // when the joystick is in the "dead zone" - not
              // quite at zero when not being pushed.
              // In my small sample set, an X-box controller needed
              // a much larger dead zone than a generic gamepad.
  
  // command line arguments
  // Convert array to a vector of strings for safer processing
  vector<string> args(argv, argv + argc);

  // Loop through the remaining parameters
  for (size_t i = 1; i < args.size(); ++i) {
     if (args[i] == "-i" && i + 1 < args.size()) {
      ipaddr = args[++i];
    } else if (args[i] == "-p" && i + 1 < args.size()) {
      port = stoi(args[++i]);
    } else if (args[i] == "-j" && i + 1 < args.size()) {
      jsnum = stoi(args[++i]);
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      dzth = stoi(args[++i]);
    } else {
      cout << "Usage: " << args[0].c_str()
          << " -i <ip address> [-p <port>][-j <joystick number>] [-t <dead zone threshold>]\n";
      return 0;
    }
  }

  if (ipaddr.empty())
  {
      cout << "Enter the IP address of the target computer.\n";
      cin >> ipaddr;
  }

  if (FAILED(CoInitialize(NULL))) {
      cout << "CoInitialize failed";
	  return 1;
  }

  WSADATA wsaData;
  int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (wsaResult != 0) 
  {
      cerr << "WSAStartup failed with error: " << wsaResult << "\n";
      return 1;
      }

  
  SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET)
  {
    cerr << "Socket creation failed with error: " << WSAGetLastError() << "\n";
	WSACleanup();
    return 1;
  }

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  // Convert string IP to network binary
  int ptonResult = inet_pton(AF_INET, ipaddr.c_str(), &addr.sin_addr);
  if (ptonResult <= 0) {
      if (ptonResult == 0) {
          cerr << "Error: Invalid IP string formatting.\n";
      } else {
          cerr << "inet_pton failed with error: " << WSAGetLastError() << "\n";
      }
      closesocket(sock);
      WSACleanup();
      return 1;
  }

  cout << "Press ESC or close the window to exit.\n";
  int unchanged_count = 0;

  while (true)
  {
      if (GetKeyState(VK_ESCAPE) & 0x8000) { break; }
      if (XInputGetState(jsnum, &state) != ERROR_SUCCESS) { continue; }

    // Packet number increments continuously, so only compare actual gamepad state
    // Send occasional update even if the state hasn't changed,
    // to be more tolerant of dropped packets
      if ((unchanged_count++ < 10) &&
          prev_state.Gamepad.wButtons == state.Gamepad.wButtons &&
          prev_state.Gamepad.bLeftTrigger == state.Gamepad.bLeftTrigger &&
          prev_state.Gamepad.bRightTrigger == state.Gamepad.bRightTrigger &&
          prev_state.Gamepad.sThumbLX == state.Gamepad.sThumbLX &&
          prev_state.Gamepad.sThumbLY == state.Gamepad.sThumbLY &&
          prev_state.Gamepad.sThumbRX == state.Gamepad.sThumbRX &&
          prev_state.Gamepad.sThumbRY == state.Gamepad.sThumbRY
          ) {
         continue;
	  }

    if (sendto(sock, (const char*)&(state.Gamepad), sizeof(XINPUT_GAMEPAD), 0, (struct sockaddr*)&addr, sizeof(addr)) != sizeof(XINPUT_GAMEPAD)) {
        cerr << "Failed to send update packet; error: " << WSAGetLastError() << "\n";
    }
	unchanged_count = 0;

    // save state for next comparison
    prev_state.Gamepad.wButtons = state.Gamepad.wButtons;
    prev_state.Gamepad.bLeftTrigger = state.Gamepad.bLeftTrigger;
    prev_state.Gamepad.bRightTrigger = state.Gamepad.bRightTrigger;
    prev_state.Gamepad.sThumbLX = state.Gamepad.sThumbLX;
    prev_state.Gamepad.sThumbLY = state.Gamepad.sThumbLY;
    prev_state.Gamepad.sThumbRX = state.Gamepad.sThumbRX;
    prev_state.Gamepad.sThumbRY = state.Gamepad.sThumbRY;
    
    // delay before next update
    this_thread::sleep_for(chrono::milliseconds(1));
  }

  closesocket(sock);
  WSACleanup();
  CoUninitialize();
  return 0;
}
