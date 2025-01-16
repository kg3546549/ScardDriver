#include "SocketListener.hpp"

using boost::asio::ip::tcp;


LONG SocketListener::StartListener() {
	// TCP 소켓 수락자 생성 (포트 12345)

	try {
		//this->io_context = boost::asio::io_context();
		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

		std::cout << "Server is listening on port 12345..." << std::endl;

		while (true) {
			// 클라이언트 연결 수락
			tcp::socket socket(io_context);
			acceptor.accept(socket);

			std::cout << "Client connected!" << std::endl;

			// 클라이언트 메시지 수신
			char buffer[1024] = { 0 };
			boost::system::error_code error;
			size_t length = socket.read_some(boost::asio::buffer(buffer), error);

			if (error == boost::asio::error::eof) {
				std::cout << "Client disconnected." << std::endl;
			}
			else if (error) {
				throw boost::system::system_error(error);
			}

			std::cout << "Received: " << std::string(buffer, length) << std::endl;

			// 클라이언트에 응답 전송
			std::string message = "Hello from server!";
			boost::asio::write(socket, boost::asio::buffer(message), error);
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}


