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

class User
{
public:
	User(std::shared_ptr<TcpSocket> tcpSocket);

	bool isConnected() const;

	bool hasName() const;

	std::string getName();

	bool hasMessage() const;

	std::string getMessage();

	void sendMessage(const std::string& message);

	void process();

private:
	void processMessage(const std::string& line);

	std::shared_ptr<TcpSocket> tcpSocket;

	std::string name;

	std::queue<std::string> messages;
};

class Server
{
public:
	Server(unsigned short port = Network::DefaultPort);

	~Server();

private:
	void acceptUser();

	void writeMessage(const std::string& message);

	void processUsers();

	void processNetwork();

	std::shared_ptr<TcpSocket> tcpSocket;

	std::vector<std::shared_ptr<User>> users;

	std::thread thread;
	mutable std::mutex mutex;

	bool run;
};
