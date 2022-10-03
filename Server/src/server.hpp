//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;

//----------------------------------------------------------------------

class chat_participant
{
public:
	virtual ~chat_participant() {}
	virtual void Deliver(const std::string& msg) = 0;
	uint32_t UserId = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

//----------------------------------------------------------------------

class chat_room
{
public:
	void Join(chat_participant_ptr participant)
	{
		participant->UserId = m_LastId++;
		m_Participants.insert(participant);
		for (auto msg : m_RecentMsgs)
			participant->Deliver(msg);
	}

	void Leave(chat_participant_ptr participant)
	{
		m_Participants.erase(participant);
	}

	void Deliver(const std::string& msg)
	{
		m_RecentMsgs.push_back(msg);
		while (m_RecentMsgs.size() > max_recent_msgs)
			m_RecentMsgs.pop_front();

		for (auto participant : m_Participants)
			participant->Deliver(msg);
	}

private:
	std::set<chat_participant_ptr> m_Participants;
	enum { max_recent_msgs = 100 };
	std::deque<std::string> m_RecentMsgs;
	uint32_t m_LastId = 1000;
};

//----------------------------------------------------------------------

class chat_session
	: public chat_participant,
	public std::enable_shared_from_this<chat_session>
{
public:
	chat_session(tcp::socket socket, chat_room& room)
		: m_Socket(std::move(socket)),
		m_Timer(m_Socket.get_executor()),
		m_Room(room)
	{
		m_Timer.expires_at(std::chrono::steady_clock::time_point::max());
	}

	void Start()
	{
		m_Room.Join(shared_from_this());

		co_spawn(m_Socket.get_executor(),
			[self = shared_from_this()] { return self->Reader(); },
			detached);

		co_spawn(m_Socket.get_executor(),
			[self = shared_from_this()] { return self->Writer(); },
			detached);
	}

	void Deliver(const std::string& msg) override
	{
		M_WriteMsgs.push_back(msg);
		m_Timer.cancel_one();
	}

private:
	awaitable<void> Reader()
	{
		try
		{
			for (std::string read_msg;;)
			{
				std::size_t n = co_await boost::asio::async_read_until(m_Socket,
					boost::asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);

				std::cout << "received message from " << UserId << '\n';

				std::string saveMessage = std::to_string(UserId) + ": " + read_msg.substr(0, n);
				m_Room.Deliver(saveMessage);
				read_msg.erase(0, n);
			}
		}
		catch (std::exception&)
		{
			Stop();
		}
	}

	awaitable<void> Writer()
	{
		try
		{
			while (m_Socket.is_open())
			{
				if (M_WriteMsgs.empty())
				{
					boost::system::error_code ec;
					co_await m_Timer.async_wait(redirect_error(use_awaitable, ec));
				}
				else
				{
					co_await boost::asio::async_write(m_Socket,
						boost::asio::buffer(M_WriteMsgs.front()), use_awaitable);
					M_WriteMsgs.pop_front();
				}
			}
		}
		catch (std::exception&)
		{
			Stop();
		}
	}

	void Stop()
	{
		m_Room.Leave(shared_from_this());
		m_Socket.close();
		m_Timer.cancel();
	}

	tcp::socket m_Socket;
	boost::asio::steady_timer m_Timer;
	chat_room& m_Room;
	std::deque<std::string> M_WriteMsgs;
};