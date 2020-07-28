#include "Client.h"
#include "PACKETENUM.h"
#include <Windows.h>
#include <math.h>
#include <sstream>
#include <string.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

Client* myClient;

#define DEFAULT_PORT 54010

std::string coords;


void SecureUsername()
{
	bool usernameAccepted = false;
	do
	{
		std::cout << "NICK: ";
		std::string username;
		std::cin >> username;
		myClient->SendString(username);

		myClient->GetBool(usernameAccepted);
		if (!usernameAccepted)
			std::cout << "Nick zajety" << std::endl;


	} while (!usernameAccepted);
}


int main(int argc, char **argv)
{

	myClient = new Client(argc, argv, DEFAULT_PORT);

	if (!myClient->Connect())
	{
		system("pause");
		return 1;
	}
	SecureUsername();
	myClient->StartSubRoutine();
	
	float posXts, posYts, posZts;
	while (true)
	{
		std::string datad;
		std::cin >> datad;
		myClient->SendString(datad);
	}
	WSACleanup();
	system("pause");
	return 0;
}