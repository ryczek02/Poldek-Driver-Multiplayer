#include "Server.h"
#include "IniReader.h"

int MP_TICKRATE;
std::string rcon_password;
int current_clients = 0;


Server::Server(int PORT)
{
	WSADATA wsaData;

	ListenSocket = INVALID_SOCKET;

	printf("[ZAINICJOWANO WINSOCK]\n");

	// inicjalizacja winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("[BLAD UTWORZENIA WSASTARTUP]: %d\n", iResult);
		exit(0);
	}

	printf("[ZAWARTOSC PLIKU SERVER.INI]\n");

	INIReader reader("server.ini");

	std::string serverIP = reader.Get("network", "ip", "UNKNOWN");
	int port = reader.GetInteger("network", "port", -1);

	if (reader.ParseError() != 0) {
		std::cout << "Brak pliku server.ini!\n";
	}
	std::cout << "MAPA: " << reader.Get("game", "map", "UNKNOWN")
		<< std::endl << "DOMYSLNE AUTO: " << reader.Get("game", "default_car", "UNKNOWN")
		<< std::endl << "MAX GRACZY: " << reader.GetInteger("network", "clients", -1)
		<< std::endl << "HASLO RCON: " << reader.Get("network", "rcon", "UNKNOWN") << std::endl;

	MP_TICKRATE = reader.GetInteger("network", "tickrate", 55);

	
	rcon_password = reader.Get("network", "rcon", "UNKNOWN");

	if (rcon_password == "UNKNOWN") {
		std::cout << "Zmien domyslne haslo RCON w server.ini!" << std::endl;
		exit(0);
	}
	std::string ipAdr = reader.Get("network", "ip", "127.0.0.1");

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// resolve ip i portu
	iResult = getaddrinfo(ipAdr.c_str(), std::to_string(PORT).c_str(), &hints, &result);
	if (iResult != 0)
	{
		std::cout << "[POBIERANIE ADRESU: " << ipAdr << "]" << std::endl;
		WSACleanup();
		exit(0);
	}

	// stworzenie socketu do dolaczania na serwer
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("[BLAD SOCKETU]: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(0);
	}

	// ustawianie socketu nasluchiwania TCP
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "[BLAD BINDOWANIA]: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
	}

	freeaddrinfo(result);

	printf("[UTWORZONO SOCKET NASLUCHUJACY]\n");

	//nasluchuj przyszlych polaczen
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("[BLAD NASLUCHIWANIA]: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
	}
	std::cout << "[SERWER NASLUCHUJE NA IP: " << ipAdr << ":" << PORT << "]";

	serverPtr = this;
}

bool Server::ListenForNewConnection()
{
	INIReader reader("server.ini");
	SOCKET ClientSocket = INVALID_SOCKET;

	ClientSocket = accept(ListenSocket, NULL, NULL);


	if (ClientSocket == INVALID_SOCKET)
	{
		printf("[BLAD AKCEPTACJI KLIENTA]: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
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
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::GetInt(int id, int& value)
{
	int returnCheck = recv(Connections[id], (char*)&value, sizeof(int), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::SendBool(int id, bool value)
{
	int returnCheck = send(Connections[id], (char*)&value, sizeof(bool), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::GetBool(int id, bool& value)
{
	int returnCheck = recv(Connections[id], (char*)&value, sizeof(bool), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::SendPacketType(int id, const PACKET& packetType)
{
	int returnCheck = send(Connections[id], (char*)&packetType, sizeof(PACKET), NULL);
	if (returnCheck == SOCKET_ERROR)
		return false;

	return true;
}

bool Server::GetPacketType(int id, PACKET& packetType)
{
	int returnCheck = recv(Connections[id], (char*)&packetType, sizeof(PACKET), NULL);
	if (returnCheck == SOCKET_ERROR)
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
	if (returnCheck == SOCKET_ERROR)
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

	if (returnCheck == SOCKET_ERROR)
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
		
		std::cout << "[ODEBRANO PAKIET OD ID = " << index << " ]" << std::endl;
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
	if (closesocket(Connections[index]) == SOCKET_ERROR)
	{
		std::cout << "[BLAD ZAMYKANIA SOCKETU]: " << WSAGetLastError() << std::endl;
		return false;
	}

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
		std::string mar = "[INFO]: pustynia.mar car.mar car2.mar car3.mar\n[LICZBA GRACZY]: " + std::to_string(current_clients+1) + "/50";
		serverPtr->SendBool(index, usernameSaved);
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

		Sleep(1000 / MP_TICKRATE);
	}

	std::cout << "[UTRACONO POLACZENIE Z ID = " << index << "]" << std::endl;
	current_clients--;
	serverPtr->CloseConnection(index);
}
