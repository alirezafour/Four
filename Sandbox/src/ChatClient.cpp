#include "ChatClient.h"

#include <boost/asio/write.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>

using boost::asio::ip::tcp;
using boost::asio::co_spawn;
using boost::asio::awaitable;
using boost::asio::detached;
using boost::asio::use_awaitable;

ChatClient::ChatClient() 
	: m_io_context{}
	, m_Resolver{ m_io_context }
	, m_Socket{ m_io_context }
{
}

ChatClient::~ChatClient()
{
	Stop();
}

void ChatClient::Connect(const std::string& ip, const std::string& port)
{
	try
	{
		tcp::resolver::results_type endpoints = m_Resolver.resolve(ip, port);
		boost::asio::connect(m_Socket, endpoints);
	}
	catch (const std::exception& e)
	{
		std::cout << "error form connect exception: \n";
		if (m_CallbackErrorsFunction)
		{
			m_CallbackErrorsFunction(e.what());
		}
		Clear();
	}
}

awaitable<void> ChatClient::Do_Reading()
{
	try
	{
		for (std::string read_msg;;)
		{
				std::size_t n = co_await boost::asio::async_read_until(m_Socket,
					boost::asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);

				OnMessage(read_msg);
				read_msg.erase(0, n);
			}
			}
	catch (std::exception& e)
	{
		std::cout << "error form reading: \n";
		if(m_CallbackErrorsFunction)
		{
			m_CallbackErrorsFunction(e.what());
		}
		Stop();
	}
}


void ChatClient::Do_Writing()
{
	if (m_Socket.is_open() && m_OutMessages.size() > 0)
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(m_OutMessages.front()),
			[this](std::error_code errorCode, size_t length)
			{
				if (errorCode && m_CallbackErrorsFunction)
				{
					m_CallbackErrorsFunction(errorCode.message());
				}
			});
	}
}

void ChatClient::OnMessage(std::string message)
{
	std::cout << message << '\n';
	if (m_CallbackMessageFunction) 
	{
		m_CallbackMessageFunction(message);
	}
	else
		if(m_CallbackErrorsFunction)
			m_CallbackErrorsFunction("Callback function is not valid");

	// delete sent message from queue
	if(m_OutMessages.size() > 0)
		m_OutMessages.pop_front();

	// if we still have message in queue write
	if (!m_OutMessages.empty())
	{
		Do_Writing();
	}
}

void ChatClient::SetCallbackMessageFuntion(std::function<void(std::string)> function)
{
	m_CallbackMessageFunction = function;
}

void ChatClient::SetCallbackErrorsFuntion(std::function<void(std::string_view)> function)
{
	m_CallbackErrorsFunction = function;
}

void ChatClient::Run()
{
	co_spawn(m_io_context,
		[this] { return Do_Reading(); },
		detached);
	m_ThreadContext = std::thread([this]() { m_io_context.run(); });
	m_ThreadContext.detach();
}

void ChatClient::Clear()
{
	Stop();
	m_CallbackMessageFunction = nullptr;
}

void ChatClient::SendMessage(std::string message)
{
	bool queryIdle = m_OutMessages.empty();
	message += "\n";
	m_OutMessages.push_back(std::move(message));

	if (queryIdle)
	{
		Do_Writing();
	}
}

void ChatClient::Stop()
{
	m_Socket.close();

	if(m_ThreadContext.joinable())
		m_ThreadContext.join();
}
