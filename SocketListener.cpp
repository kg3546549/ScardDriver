#include "SocketListener.hpp"

using boost::asio::ip::tcp;


LONG SocketListener::StartListener() {
	// TCP ���� ������ ���� (��Ʈ 12345)

	try {
		//this->io_context = boost::asio::io_context();
		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

		std::cout << "Server is listening on port 12345..." << std::endl;

		while (true) {
			// Ŭ���̾�Ʈ ���� ����
			tcp::socket socket(io_context);
			acceptor.accept(socket);

			std::cout << "Client connected!" << std::endl;

			// Ŭ���̾�Ʈ �޽��� ����
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

			// Ŭ���̾�Ʈ�� ���� ����
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


