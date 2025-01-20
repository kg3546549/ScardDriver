#include "SocketListener.hpp"

SocketListener::SocketListener() {
	

}

LONG SocketListener::InitListener() {
	

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup Failed" << std::endl;
		return -1;
	}

	this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed" << std::endl;
		WSACleanup();
		return -1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;  // 내부소켓용으로 Lookback 네트워크 인터페이스에서만 연결 수락
	serverAddr.sin_port = htons(12345);

	if (bind(this->serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind Failed! Error: " << WSAGetLastError() << std::endl;
		closesocket(this->serverSocket);
		WSACleanup();
		return -1;
	}

	std::cout << "Listening...!" << std::endl;
	if (listen(this->serverSocket, 1) == SOCKET_ERROR) {
		std::cerr << "Listen failed!" << std::endl;
		closesocket(this->serverSocket);
		WSACleanup();
		return -1;
	}

	std::cout << "Accept Client" << std::endl;
	clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Accept failed!" << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return -1;
	}

	

	std::cout << "Client connected!" << std::endl;

	return 1;
}

void SocketListener::Receiver() {
	char buffer[1024];
	std::cout << "[Receiver Started!]" << std::endl;

	while (true) {
		int recvDataLen = recv(this->clientSocket, buffer, sizeof(buffer), 0);

		if (recvDataLen <= 0) {
			std::cerr << "Connection closed or error during receive." << std::endl;

			std::lock_guard<std::mutex> lock(queueMutex);
			transmitQueue.push("CLOSE-END");

			break;
		}

		

		//BlaBla
		std::string receivedData(buffer, recvDataLen);
		std::cout << "received : " << receivedData << std::endl;

		std::lock_guard<std::mutex> lock(queueMutex);
		transmitQueue.push(receivedData);

		cv.notify_one();
	}

	closesocket(this->clientSocket);
}

void SocketListener::Transmitter() {
	std::cout << "[Transmitter Started!]" << std::endl;

	while (true) {
		std::unique_lock<std::mutex> lock(queueMutex);

		cv.wait(
			lock, 
			[this] { 
				return !this->transmitQueue.empty(); 
			}
		);

		while ( !( this->transmitQueue.empty() ) ) {
			std::string data = (this->transmitQueue.front());
			this->transmitQueue.pop();

			if (data == "CLOSE-END") {
				closesocket(this->serverSocket);
				return;
			}

			std::cout << "[Transmitter] - Transmitting data: " << data << std::endl;

			send(this->clientSocket, data.c_str(), data.size(), 0);
		}
	}

}

LONG SocketListener::StartListener() {
	// TCP 소켓 수락자 생성 (포트 12345)
	
	std::cout << "==Start Socket Listener==" << std::endl;

	std::thread recvTask( &SocketListener::Receiver, this);
	std::thread transmitTask( &SocketListener::Transmitter,this);
	
	recvTask.join();
	transmitTask.join();

	std::cout << "==Close All Thread!==" << std::endl;

	return 0;
}


