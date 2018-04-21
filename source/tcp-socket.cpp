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

#include "tcp-socket.hpp"

TcpSocket::TcpSocket() : socket(INVALID_SOCKET), bound(false), connected(false), pinged(false), lastTime(std::chrono::high_resolution_clock::now())
{
	Network::startup();

	this->socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (this->socket == INVALID_SOCKET)
	{
		this->cleanup();

		throw std::runtime_error("Failed to initialize the TCP socket");
	}
}

TcpSocket::TcpSocket(Socket socket) : socket(socket), bound(false), connected(false), pinged(false)
{
	Network::startup();
}

TcpSocket::~TcpSocket()
{
	this->cleanup();
}

void TcpSocket::bind(unsigned short port)
{
	if (this->socket != INVALID_SOCKET)
	{
		sockaddr_in addr;

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(port);

		if (::bind(this->socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
		{
			this->close(true);

			throw std::runtime_error("Failed to bind the TCP socket on port " + std::to_string(port));
		}

		this->bound = true;
	}
}

bool TcpSocket::isBound() const
{
	return this->bound;
}

void TcpSocket::listen(int maxConnections)
{
	if (this->socket != INVALID_SOCKET)
	{
		if (::listen(this->socket, maxConnections) == SOCKET_ERROR)
		{
			this->close(true);

			throw std::runtime_error("Failed to place the TCP socket in listening state");
		}
	}
}

void TcpSocket::connect(const std::string& ipAddress, unsigned short port)
{
	if (this->socket != INVALID_SOCKET)
	{
		sockaddr_in addr;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (inet_pton(AF_INET, ipAddress.c_str(), &(addr.sin_addr)) != 1)
		{
			throw std::runtime_error("Not a valid IP address");
		}

		if (::connect(this->socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
		{
			this->close(true);

			throw std::runtime_error("Failed to connect the TCP socket to " + ipAddress + ":" + std::to_string(port));
		}

		this->connected = true;
	}
}

void TcpSocket::connect(const std::string& address)
{
	std::string ipAddress = address;
	unsigned short port = Network::DefaultPort;

	std::size_t position = address.find(":");

	if (position != std::string::npos)
	{
		ipAddress = std::string(address.begin(), address.begin() + position);

		port = static_cast<unsigned short>(atoi(std::string(address.begin() + position + 1, address.end()).c_str()));
	}

	this->connect(ipAddress, port);
}

bool TcpSocket::isConnected() const
{
	return this->connected;
}

bool TcpSocket::hasTimedOut() const
{
	if (this->isConnected() && this->pinged)
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();

		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - this->lastTime).count();

		if (time >= 10.0f)
		{
			return true;
		}
	}

	return false;
}

bool TcpSocket::isAvailable() const
{
	if (this->socket != INVALID_SOCKET)
	{
		fd_set readfds;

		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;

		FD_ZERO(&readfds);
		FD_SET(this->socket, &readfds);

		if (select(static_cast<int>(this->socket) + 1, &readfds, nullptr, nullptr, &tv) > 0)
		{
			if (FD_ISSET(this->socket, &readfds) > 0)
			{
				return true;
			}
		}
	}

	return false;
}

std::shared_ptr<TcpSocket> TcpSocket::accept()
{
	std::shared_ptr<TcpSocket> tcpSocket;

	if (this->socket != INVALID_SOCKET)
	{
		sockaddr_in addr;
		socklen_t addrSize = sizeof(addr);

		Socket socket = ::accept(this->socket, reinterpret_cast<sockaddr*>(&addr), &addrSize);

		if (socket != INVALID_SOCKET)
		{
			tcpSocket = std::shared_ptr<TcpSocket>(new TcpSocket(socket));

			if (tcpSocket)
			{
				tcpSocket->connected = true;
			}
		}
		else
		{
			this->close(true);
		}
	}

	return tcpSocket;
}

bool TcpSocket::hasLine()
{
	return this->lines.size() > 0;
}

std::string TcpSocket::readLine()
{
	std::string str;

	if (this->lines.size() > 0)
	{
		str = this->lines.front();

		this->lines.pop();
	}

	return str;
}

void TcpSocket::writeLine(const std::string& line)
{
	if (this->socket != INVALID_SOCKET)
	{
		std::string str = line + "\n";

		if (send(this->socket, str.c_str(), static_cast<int>(str.length()), 0) <= 0)
		{
			this->close(true);
		}
	}
}

void TcpSocket::close(bool force)
{
	if (this->socket != INVALID_SOCKET)
	{
		::close(this->socket);

		this->socket = INVALID_SOCKET;
	}

	this->pinged = false;

	this->bound = false;

	this->connected = false;
}

void TcpSocket::process()
{
	if (this->socket != INVALID_SOCKET)
	{
		while (this->isAvailable())
		{
			std::array<char, 1024> bytes;

			bytes.fill('\0');

			if (recv(this->socket, bytes.data(), static_cast<int>(bytes.size()) - 1, 0) <= 0)
			{
				this->close(true);

				break;
			}

			this->input += bytes.data();
		}

		std::size_t position = this->input.find("\n");

		if (position != std::string::npos)
		{
			std::string line = std::string(this->input.begin(), this->input.begin() + position);

			this->input = std::string(this->input.begin() + position + 1, this->input.end());

			if (!this->processCmd(line))
			{
				this->processLine(line);
			}
		}

		if (this->isConnected())
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();

			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - this->lastTime).count();

			if (time >= 0.1f && !this->pinged)
			{
				this->writeCmd('p');

				this->lastTime = std::chrono::high_resolution_clock::now();

				this->pinged = true;
			}
		}
	}
}

void TcpSocket::writeCmd(char cmd)
{
	char str[3] = { '\b', cmd, '\0' };

	this->writeLine(str);
}

bool TcpSocket::processCmd(const std::string& line)
{
	if (line.length() > 1)
	{
		if (line[0] == '\b')
		{
			char cmd = line[1];

			switch (cmd)
			{
			case 'p':
			{
				this->writeCmd('a');

				break;
			}
			case 'a':
			{
				this->lastTime = std::chrono::high_resolution_clock::now();
				
				this->pinged = false;

				break;
			}
			}

			return true;
		}
	}

	return false;
}

void TcpSocket::processLine(const std::string& line)
{
	if (line.length() > 0)
	{
		this->lines.push(line);
	}
}

void TcpSocket::cleanup()
{
	this->close();

	Network::cleanup();
}
