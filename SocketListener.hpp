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

using json = nlohmann::json;

class SocketListener {

private :
	SOCKET serverSocket, clientSocket;
	sockaddr_in serverAddr, clientAddr;
	int clientAddrSize = sizeof(clientAddr);
	
	std::queue<std::string> transmitQueue;
	std::mutex queueMutex;
	std::condition_variable cv;
	WSADATA wsaData;

	void Receiver();
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