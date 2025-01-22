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
			{
				std::lock_guard<std::mutex> lock(procQueMutex);
				processQueue.push("CLOSE-END");
			}
			cv.notify_all();
			std::cerr << "Receiver Thread End" << std::endl;
			return;
		}

		//BlaBla
		std::string receivedData(buffer, recvDataLen);
		std::cout << "[Receiver] : " << receivedData << std::endl;
		{
			std::lock_guard<std::mutex> lock(procQueMutex);
			processQueue.push(receivedData);
		}
		cv.notify_all();
	}

	closesocket(this->clientSocket);
}

void SocketListener::Processor() {
	std::cout << "[Processor Started!]" << std::endl;

	while (true) {
		std::unique_lock<std::mutex> lock1(procQueMutex);

		
		cv.wait(
			lock1, 
			[this] { //이 람다식이 False를 리턴하면 Mutex lock1이 해제됨
				return !this->processQueue.empty(); 
			}
		);

		while ( !( this->processQueue.empty() ) ) {
			std::string data = (this->processQueue.front());
			this->processQueue.pop();

			if (data == "CLOSE-END") {
				{
					std::lock_guard<std::mutex> transLock(transQueMutex);
					transmitQueue.push("CLOSE-END");
				}
				std::cerr << "Processor Thread End" << std::endl;
				cv.notify_all();
				return;
			}


			/*
			* TODO : Processor Thread
			Processor Thread에 들어오는 string json 데이터를 파싱하여
			
			Request(요청) 객체를 만들고
			Request객체를 WinscardDriver에 던지는 인터페이스 함수를 하나 더 만들어서
			Winscard를 조작한 후에 해당 결과를 받아서
			
			Response(응답) 객체를 만든 후에
			Transmit Thread에 전송
			*/
			std::cout << "[Processor] - Process data: " << data << std::endl;

			json requestJson = json::parse(data);
			
			Protocol::ReaderRequest * requestData;

			try {
				requestData = new Protocol::ReaderRequest(requestJson);
			}
			catch (std::exception& e) {
				std::cerr << "[Processor] - Invalid Json Type" << std::endl;

				requestJson["result"] = Protocol::Result::Default_Fail;

				{
					std::lock_guard<std::mutex> transLock(transQueMutex);
					transmitQueue.push(requestJson.dump());
				}
				continue;
			}
			
			Protocol::ReaderRequest responseData(requestData);
			responseData.setSender(Protocol::Response);
			
			LONG result = ProcessWinscard(&responseData);
			
			/*requestJson["sender"] = Protocol::Sender::Response;
			requestJson["data"] = { "Process Sucess" };*/

			{
				std::lock_guard<std::mutex> transLock(transQueMutex);
				transmitQueue.push(responseData.toJson().dump());
			}

			cv.notify_all();
		}
	}

}

void SocketListener::Transmitter() {

	std::cout << "[Transmitter Started!]" << std::endl;

	while (true) {
		std::unique_lock<std::mutex> lock1(transQueMutex);

		cv.wait(
			lock1,
			[this] {
				return !this->transmitQueue.empty();
			}
		);

		while (!(this->transmitQueue.empty())) {
			std::string data = (this->transmitQueue.front());
			this->transmitQueue.pop();

			if (data == "CLOSE-END") {
				closesocket(this->serverSocket);
				std::cerr << "Transmit Thread End" << std::endl;
				return;
			}

			std::cout << "[Transmittor] - Transmitting data: " << data << std::endl;

			send(this->clientSocket, data.c_str(), data.size(), 0);
		}
	}

}

LONG SocketListener::StartListener() {
	// TCP 소켓 수락자 생성 (포트 12345)
	
	std::cout << "==Start Socket Listener==" << std::endl;

	std::thread recvTask( &SocketListener::Receiver, this);
	std::thread procTask(&SocketListener::Processor, this);
	std::thread transmitTask(&SocketListener::Transmitter, this);
	
	recvTask.join();
	procTask.join();
	transmitTask.join();

	std::cout << "==Close All Thread!==" << std::endl;

	return 0;
}


