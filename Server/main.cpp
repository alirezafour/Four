#include "src/server.hpp"

#include <iostream>
#include <memory>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;
//----------------------------------------------------------------------

awaitable<void> listener(tcp::acceptor acceptor)
{
	chat_room room;

	for (;;)
	{
		std::make_shared<chat_session>(
			co_await acceptor.async_accept(use_awaitable),
			room
			)->Start();

		std::cout << "client connected...\n";
	}
}

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_context io_context(1);
		unsigned short port = 6000;

		// spawn thread to handle accept
		co_spawn(io_context,
			listener(tcp::acceptor(io_context, { tcp::v4(), port })),
			detached);

		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto) { io_context.stop(); });

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}