#undef UNICODE
#define WIN32_LEAN_AND_MEAN


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

	if (reader.ParseError() != 0) {
		std::cout << "Brak pliku\n";
	}

	Server server(reader.GetInteger("network", "port", 54010));
	for (int i = 0; i < 1024; i++)
	{
		server.ListenForNewConnection();
	}
	system("pause");
	return 0;
}