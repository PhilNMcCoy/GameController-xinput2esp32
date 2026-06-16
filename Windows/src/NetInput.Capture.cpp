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

SOCKET sock;
struct sockaddr_in addr;
XINPUT_STATE lastSentInputStates[XUSER_MAX_COUNT];

void PollControllers() 
{
	XINPUT_STATE state;
	for (uint32_t i = 0u; i < 1/* XUSER_MAX_COUNT */; i++)
	{
		memset(&state, 0, sizeof(XINPUT_STATE));
		if (XInputGetState(i, &state) != ERROR_SUCCESS)
			continue;

		// Packet number increments continuously, so only compare actual gamepad state
		//if (memcmp(lastSentInputStates + i, &state, sizeof(XINPUT_STATE)) == 0)
		//	continue;
		if (memcmp((&lastSentInputStates[i].Gamepad), &(state.Gamepad), sizeof(XINPUT_GAMEPAD))==0) continue;
		/*
		printf("pkt: %08x  ", state.dwPacketNumber);
		printf("btn: %04x ", state.Gamepad.wButtons);
		printf("trg: %02x%02x ", state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger);
		//printf("ljoy: % 06d % 06d rjoy: % 06d % 06d ",
		//	state.Gamepad.sThumbLX, state.Gamepad.sThumbLY, state.Gamepad.sThumbRX, state.Gamepad.sThumbRY);
		printf("ljoy: %04x %04x rjoy: %04x %04x ",
			(unsigned short)(state.Gamepad.sThumbLX),
			(unsigned short)(state.Gamepad.sThumbLY),
			(unsigned short)(state.Gamepad.sThumbRX),
			(unsigned short)(state.Gamepad.sThumbRY));
			*/

		//uint8_t packet[sizeof(XINPUT_STATE) + 1];
		uint8_t packet[sizeof(XINPUT_GAMEPAD)];
		//packet[0] = (uint8_t)i;
		//memcpy(packet + 1, &state, sizeof(XINPUT_STATE));
		memcpy(packet, &(state.Gamepad), sizeof(XINPUT_GAMEPAD));
		memcpy(lastSentInputStates + i, &state, sizeof(XINPUT_STATE));
/*
		printf("packet:  ");
		for (int ii = 0; ii <= sizeof(packet); ii++) {
			printf("%02x", packet[ii]);
		}
		printf("\n");
		*/
		if (sendto(sock, (const char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr)) != sizeof(packet))
			printf("Failed to send update message of controller %u.\n", i);
	}
}

int main(int argc, char* argv[])
{
	std::string ipaddr;   // IP address of receiver
	int port    = 4313;   // UDP port
	int jsnum   = 0;      // which joystick to capture (default: 0)
	int dzth    = 3500;   // minimum axis threshold to avoid spurious events
	                      // when the joystick is in the "dead zone" - not
						  // quite at zero when not being pushed.
						  // In my small sample set, an X-box controller needed
						  // a much larger dead zone than a generic gamepad.
	
	// command line arguments
	// Convert array to a vector of strings for safer processing
	std::vector<std::string> args(argv, argv + argc);

	// args[0] is always the program path or name
	cout << "Program Name: " << args[0] << "\n";

	// Loop through the remaining parameters
	for (size_t i = 1; i < args.size(); ++i) {
		 if (args[i] == "-i" && i + 1 < args.size()) {
			ipaddr = args[++i];
		} else if (args[i] == "-p" && i + 1 < args.size()) {
			port = std::stoi(args[++i]);
		} else if (args[i] == "-j" && i + 1 < args.size()) {
			jsnum = std::stoi(args[++i]);
		} else if (args[i] == "-t" && i + 1 < args.size()) {
			dzth = std::stoi(args[++i]);
		} else {
			cout << "Usage: " << args[0].c_str()
			    << " -i <ip address> [-p <port>][-j <joystick number>] [-t <dead zone threshold>]\n";
			return 0;
		}
	}

	cout << "IP: " << ipaddr << "\n";
	cout << "port: " << port << "\n";
	cout << "jsnum: " << jsnum << "\n";
	cout << "dzth: " << dzth << "\n";
	
	if (ipaddr.empty())
	{
		std::cout << "Enter the IP address of the target computer.\n";
		std::cin >> ipaddr;			
		if (!(inet_pton(AF_INET, ipaddr.c_str(), &(addr.sin_addr)) == 1)){
            std::cout << "Invalid IP address format.\n";
			return -1;
		}
	}

	CoInitialize(NULL);

	WSADATA wsaData;
	int wsaStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStartupResult != 0) 
	{
		printf("WSAStartup failed with error code 0x%08X.\n", wsaStartupResult);
		return -2;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		printf("Failed to create UDP transmission socket.\n");
		return -3;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	std::cout << "Press ESC or close the window to exit.\n";

	memset(lastSentInputStates, 0, sizeof(lastSentInputStates));
	while (true)
	{
		if (GetKeyState(VK_ESCAPE) & 0x8000) break;

		PollControllers();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	closesocket(sock);
	WSACleanup();
	CoUninitialize();

	return 0;
}
