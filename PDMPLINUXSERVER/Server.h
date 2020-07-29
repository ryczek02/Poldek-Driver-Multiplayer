#pragma once
#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <stdio.h>

enum PACKET
{
	P_ChatMessage,
	P_DirectMessage
};

class Server
{
public:
	Server(int PORT);
	bool ListenForNewConnection();

	std::vector<std::string> usernames = {};

private:
	int Connections[100];
	std::thread connectionThreads[100];
	addrinfo* result;
	addrinfo hints;
	int ListenSocket;
	int iResult;
	int ConnectionCounter = 0;

	void Getusername(int id);

	bool GetInt(int id, int& value);
	bool SendInt(int id, int value);
	bool SendBool(int id, bool value);
	bool GetBool(int id, bool& value);
	bool SendPacketType(int id, const PACKET& packetType);
	bool GetPacketType(int id, PACKET& packetType);
	bool SendString(int id, const std::string& value);
	bool GetString(int id, std::string& value);
	bool ProcessPacket(int index, PACKET packetType);
	bool CloseConnection(int index);

	static void ClientHandler(int index);
};

static Server* serverPtr;