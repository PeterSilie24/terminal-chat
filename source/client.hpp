/* terminal-chat - A simple chat application for the terminal
 * Copyright (C) 2018 Phil Badura
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <thread>
#include <mutex>
#include <vector>

#include "tcp-socket.hpp"

class Client
{
public:
	Client(const std::string& name, const std::string& address);

	~Client();

	bool isClosed() const;

	bool hasMessage() const;

	std::string getMessage();

	void sendMessage(const std::string& message);

private:
	void processMessage(const std::string& line);

	void processNetwork();

	std::shared_ptr<TcpSocket> tcpSocket;

	std::queue<std::string> messages;

	std::thread thread;
	mutable std::mutex mutex;

	bool run;
};
