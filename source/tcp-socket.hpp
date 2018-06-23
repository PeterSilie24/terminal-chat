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

#include <queue>
#include <memory>
#include <array>
#include <chrono>
#include <algorithm>
#include <cstring>

#include "network.hpp"

class TcpSocket
{
public:
	TcpSocket();

private:

	TcpSocket(Socket socket);

public:

	~TcpSocket();

	void bind(unsigned short port = Network::DefaultPort);

	bool isBound() const;

	void listen(int maxConnections = Network::MaxConnections);

	void connect(const std::string& address, unsigned short port);

	void connect(const std::string& address);

	bool isConnected() const;

	bool hasTimedOut() const;

	bool isAvailable() const;

	std::shared_ptr<TcpSocket> accept();

	bool hasLine();

	std::string readLine();

	void writeLine(const std::string& line);

	void close();

	void process();

private:
	void setup(int family);

	void writeCmd(char cmd);

	bool processCmd(const std::string& line);

	void processLine(const std::string& line);

	Socket socket;

	std::string input;
	std::queue<std::string> lines;

	bool bound;
	bool connected;
	bool pinged;

	std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
};
