#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <stdio.h>
#include "Server.h"
#include "IniReader.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 21370

int main(void)
{
    INIReader reader("server.ini");

    std::string serverIP = reader.Get("network", "ip", "UNKNOWN");
    int port = reader.GetInteger("network", "port", -1);

    if (reader.ParseError() != 0) {
        std::cout << "Brak pliku server.ini!\n";
        return 1;
    }

	Server server(reader.GetInteger("network", "port", 54010));
	for (int i = 0; i < 1024; i++)
	{
		server.ListenForNewConnection();
	}
	system("pause");
	return 0;
}