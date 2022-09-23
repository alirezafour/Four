#pragma once
#include <cstdlib>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <functional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>

class ChatClient
{
public:
	ChatClient();
	~ChatClient();

	inline boost::asio::ip::tcp::socket& getSocket() { return m_Socket; }
	void Connect(const std::string& ip, const std::string& port);

	/**
	 * runs io_context in a new thread
	 */
	void Run();

	void Clear();

	/**
	 * add message to OutMessages and run writer if queue was empty
	 */
	void SendMessage(std::string message);

	void SetCallbackMessageFuntion(std::function<void(std::string)> function);
	void SetCallbackErrorsFuntion(std::function<void(std::string_view)> function);

protected:
	void OnMessage(std::string message);

private:
	boost::asio::awaitable<void> Do_Reading();
	void Do_Writing();

	/**
	 * close the socket and join the io_context thread
	 */
	void Stop();

private:
	boost::asio::io_context m_io_context;
	std::thread m_ThreadContext;
	boost::asio::ip::tcp::resolver m_Resolver;
	boost::asio::ip::tcp::socket m_Socket;
	std::function<void(std::string)> m_CallbackMessageFunction;
	std::function<void(std::string_view)> m_CallbackErrorsFunction;
	std::deque<std::string> m_OutMessages;
	bool m_IsConnected = false;
};