#include <boost/asio.hpp>
#include <winscard.h>
#include <windows.h>

#include <iostream>

using boost::asio::ip::tcp;
class SocketListener {
	boost::asio::io_context io_context;

public:
	SocketListener() {

	}

	LONG StartListener();
};