#pragma once

#include <winsock2.h>
#include <windows.h>

//include STL
#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <nlohmann/json.hpp>

#include "ReaderRequest.hpp"
#include "WinscardInterface.hpp"

using json = nlohmann::json;

class SocketListener {

private :
	SOCKET serverSocket, clientSocket;
	sockaddr_in serverAddr, clientAddr;
	int clientAddrSize = sizeof(clientAddr);
	
	std::queue<std::string> processQueue;
	std::mutex procQueMutex;

	std::queue<std::string> transmitQueue;
	std::mutex transQueMutex;

	std::condition_variable cv;
	WSADATA wsaData;

	void Receiver();
	void Processor();
	void Transmitter();

public:
	enum Status {
		connected = 1,
		disconnected = 0
	};

	SocketListener();

	LONG InitListener();

	LONG StartListener();
};