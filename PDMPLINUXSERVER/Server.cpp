#include "Server.h"
#include "IniReader.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include "signal.h"

int MP_TICKRATE;
std::string rcon_password;
int current_clients = 0;

//

Server::Server(int PORT)
{

	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	printf("[ZAINICJOWANO LINUX SOCKET]\n");
	// inicjalizacja winsock

	printf("[ZAWARTOSC PLIKU SERVER.INI]\n");

	INIReader reader("server.ini");

	std::string serverIP = reader.Get("network", "ip", "192.168.100.103");
	int port = reader.GetInteger("network", "port", 54010);

	if (reader.ParseError() != 0) {
		std::cout << "Brak pliku server.ini!\n";
	}

	std::cout << "MAPA: " << reader.Get("game", "map", "pustynia.mar")
		<< std::endl << "DOMYSLNE AUTO: " << reader.Get("game", "default_car", "pold.mar")
		<< std::endl << "MAX GRACZY: " << reader.GetInteger("network", "clients", 4)
		<< std::endl << "HASLO RCON: " << reader.Get("network", "rcon", "woytek") << std::endl;

	MP_TICKRATE = reader.GetInteger("network", "tickrate", 55);


	rcon_password = reader.Get("network", "rcon", "unkwn");

	if (rcon_password == "UNKNOWN") {
		std::cout << "Zmien domyslne haslo RCON w server.ini!" << std::endl;
		exit(0);
	}
	std::string ipAdr = reader.Get("network", "ip", "192.168.100.103");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// resolve ip i portu
	iResult = getaddrinfo(ipAdr.c_str(), std::to_string(PORT).c_str(), &hints, &result);
	if (iResult != 0)
	{
		std::cout << "[POBIERANIE ADRESU: " << ipAdr << "]" << std::endl;
		close(ListenSocket);
		exit(0);
	}

	// stworzenie socketu do dolaczania na serwer
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket < 0)
	{
		fprintf(stderr, "[BLAD SOCKETU]: %m\n");
		exit(0);
	}


	// ustawianie socketu nasluchiwania TCP
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);

	if (iResult < 0)
	{
		fprintf(stderr, "[BLAD BINDOWANIA]: %m\n");
		close(ListenSocket);
		exit(0);
	}



	freeaddrinfo(result);

	printf("[UTWORZONO SOCKET NASLUCHUJACY]\n");

	//nasluchuj przyszlych polaczen
	iResult = listen(ListenSocket, SOMAXCONN);

	if (iResult < 0)
	{
		fprintf(stderr, "[BLAD NASLUCHIWANIA]: %m\n");
		close(ListenSocket);
		exit(0);
	}

	std::cout << "[SERWER NASLUCHUJE NA IP: " << ipAdr << ":" << PORT << "]";

	serverPtr = this;
}

bool Server::ListenForNewConnection()
{
	INIReader reader("server.ini");
	int ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	ClientSocket = accept(ListenSocket, NULL, NULL);


	if (ClientSocket < 0)
	{
		printf("Blad akceptacji");
		return false;
	}

	else // uddane polaczenie
	{
		std::cout << "[UZYTKOWNIK PODLACZONY]" << std::endl;
		std::cout << "[OCZEKIWANIE NA WYBRANIE NICKU...]" << std::endl;
		Connections[ConnectionCounter] = ClientSocket;
		Getusername(ConnectionCounter);

		connectionThreads[ConnectionCounter] = std::thread(ClientHandler, ConnectionCounter);
		ConnectionCounter++;
		current_clients++;
		return true;
	}
}

bool Server::SendInt(int id, int value)
{
	int returnCheck = send(Connections[id], (char*)&value, sizeof(int), NULL);
	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::GetInt(int id, int& value)
{
	int returnCheck = recv(Connections[id], (char*)&value, sizeof(int), NULL);
	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::SendBool(int id, bool value)
{
	int returnCheck = send(Connections[id], (char*)&value, sizeof(bool), NULL);
	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::GetBool(int id, bool& value)
{
	int returnCheck = recv(Connections[id], (char*)&value, sizeof(bool), NULL);
	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::SendPacketType(int id, const PACKET& packetType)
{
	int returnCheck = send(Connections[id], (char*)&packetType, sizeof(PACKET), NULL);
	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::GetPacketType(int id, PACKET& packetType)
{
	int returnCheck = recv(Connections[id], (char*)&packetType, sizeof(PACKET), NULL);
	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::SendString(int id, const std::string& value)
{
	if (!SendPacketType(id, P_ChatMessage))
		return false;

	int bufferLength = value.size();
	if (!SendInt(id, bufferLength))
		return false;

	int returnCheck = send(Connections[id], value.c_str(), bufferLength, NULL);
	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::GetString(int id, std::string& value)
{
	int bufferLength = 0;
	if (!GetInt(id, bufferLength))
		return false;

	char* buffer = new char[bufferLength + 1];

	int returnCheck = recv(Connections[id], buffer, bufferLength, NULL);
	buffer[bufferLength] = '\0';
	value = buffer;
	delete[] buffer;

	if (returnCheck < 0)
		return false;

	return true;
}

bool Server::ProcessPacket(int index, PACKET packetType)
{
	switch (packetType)
	{
	case P_ChatMessage:
	{
		std::string message;
		if (!GetString(index, message))
			return false;
		for (int i = 0; i < ConnectionCounter; i++)
		{
			if (i == index)
				continue;

			std::string newMessage = usernames[index] + "[" + std::to_string(index) + "]" + ": " + message;
			if (!SendString(i, newMessage))
				std::cout << "[NIE MOZNA WYSLAC WIADOMOSCI: " << message << " DO ID =" << i << "]" << std::endl;
		}

		std::cout << "[ODEBRANO PAKIET OD ID = " << index << " ][" << message << "]" << std::endl;
		break;
	}

	case P_DirectMessage:
	{
		std::cout << "[WIADOMOSC BEZPOSREDNIA]: " << std::endl;
		std::string user;
		std::string message;

		std::string value;

		int usernameIndex = -1;
		bool userExists = false;

		//sprawdz uzytkownika
		if (!GetString(index, value))
			return false;

		int val = 0;
		//sprawdz pozadanego uzytkownika
		while (value[val] != ' ')
		{
			user += value[val];
			val++;
		}

		//sprawdz czy uzytkownik istnieje
		for (int i = 0; i < ConnectionCounter; i++)
		{
			if (usernames[i] == user)
			{
				userExists = true;
				usernameIndex = i;
				break;
			}
		}

		if (userExists)
		{
			//sprawdz wiadomosc
			for (int i = val; i < value.size(); i++)
			{
				message += value[i];
			}
		}

		SendPacketType(index, P_DirectMessage);
		SendBool(index, userExists);

		if (userExists)
		{
			std::string fullMessage = "[BEZPOSREDNI PAKIET OD]: " + usernames[index] + ": " + message;

			SendString(usernameIndex, fullMessage);
		}

		break;
	}

	default:
		std::cout << "[NIEROZPOZNANY PAKIET]: " << packetType << std::endl;
		break;
	}
	return true;
}

bool Server::CloseConnection(int index)
{
	shutdown(Connections[index], 2);
	close(Connections[index]);
	return true;
}

void Server::Getusername(int index)
{
	serverPtr->usernames.push_back("");
	PACKET packetType;

	//sprawdzanie nazwy uzytkownika i akceptacja / odrzut
	bool usernameSaved = true;
	do
	{
		usernameSaved = true;
		if (!serverPtr->GetPacketType(index, packetType))
		{
			std::cout << "[NIE MOZNA POBRAC NAZWY UZYTKOWNIKA]" << std::endl;
			break;
		}

		if (!packetType == P_ChatMessage)
		{
			std::cout << "[POBIERANIE NAZWY UZYTKOWNIKA NIE JEST WIADOMOSCIA]" << std::endl;
			break;
		}

		std::string userName;
		serverPtr->GetString(index, userName);
		/*
		for each (std::string var in serverPtr->usernames)
		{
			if (var == userName)
			{
				usernameSaved = false;
				break;
			}
		}
		*/
		if (usernameSaved)
		{
			serverPtr->usernames[index] = userName;
			std::cout << "[ID: " << index << " PRZYPISANE DLA " << userName << "]" << std::endl;
		}
		std::string mar = "[INFO]: pustynia.mar car.mar car2.mar car3.mar\n[LICZBA GRACZY]: " + std::to_string(current_clients + 1) + "/50";
		serverPtr->SendBool(index, usernameSaved);
		close(index);
		serverPtr->SendString(index, mar);
	} while (!usernameSaved);
}

//handler
void Server::ClientHandler(int index)
{
	PACKET packetType;
	while (true)
	{
		//odbieranie wiadomosci
		if (!serverPtr->GetPacketType(index, packetType))
			break;
		if (!serverPtr->ProcessPacket(index, packetType))
			break;

		usleep(1000/MP_TICKRATE);
	}
	std::cout << "[UTRACONO POLACZENIE Z ID = " << index << "]" << std::endl;
	current_clients--;
	serverPtr->CloseConnection(index);
}
