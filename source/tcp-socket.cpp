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
}

TcpSocket::TcpSocket(Socket socket) : socket(socket), bound(false), connected(false), pinged(false)
{
	Network::startup();
}

TcpSocket::~TcpSocket()
{
	this->close();

	Network::cleanup();
}

void TcpSocket::bind(unsigned short port)
{
	this->setup(AF_INET6);

	if (this->socket != INVALID_SOCKET)
	{
		int flag = 1;

		if (setsockopt(this->socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&flag), sizeof(flag)) == SOCKET_ERROR)
		{
			this->close();

			throw std::runtime_error("Failed to set reuse address option for the TCP socket");
		}

		flag = 0;

		if (setsockopt(this->socket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&flag), sizeof(flag)) == SOCKET_ERROR)
		{
			this->close();

			throw std::runtime_error("Failed to disable IPv6 v6 only");
		}

		sockaddr_in6 addr;

		std::memset(&addr, 0, sizeof(addr));

		addr.sin6_family = AF_INET6;
		addr.sin6_addr = in6addr_any;
		addr.sin6_port = htons(port);

		if (::bind(this->socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
		{
			this->close();

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
	if (!this->bound)
	{
		throw std::runtime_error("The TCP socket is not bound");
	}

	if (this->socket != INVALID_SOCKET)
	{
		if (::listen(this->socket, maxConnections) == SOCKET_ERROR)
		{
			this->close();

			throw std::runtime_error("Failed to place the TCP socket in listening state");
		}
	}
}

void TcpSocket::connect(const std::string& address, unsigned short port)
{
	sockaddr* addr = nullptr;
	socklen_t addrLen = 0;

	int family = 0;

	std::string ipAddress = Network::resolveHostIPv6(address);

	if (ipAddress != "")
	{
		sockaddr_in6 addr6;

		std::memset(&addr6, 0, sizeof(addr6));

		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port);

		if (inet_pton(AF_INET6, ipAddress.c_str(), &(addr6.sin6_addr)))
		{
			addr = reinterpret_cast<sockaddr*>(&addr6);

			addrLen = sizeof(addr6);

			family = AF_INET6;
		}
		else
		{
			throw std::runtime_error("Not a valid IP address");
		}
	}
	else
	{
		ipAddress = Network::resolveHostIPv4(address);

		if (ipAddress != "")
		{
			sockaddr_in addr4;

			std::memset(&addr4, 0, sizeof(addr4));

			addr4.sin_family = AF_INET;
			addr4.sin_port = htons(port);

			if (inet_pton(AF_INET, ipAddress.c_str(), &(addr4.sin_addr)))
			{
				addr = reinterpret_cast<sockaddr*>(&addr4);

				addrLen = sizeof(addr4);

				family = AF_INET;
			}
			else
			{
				throw std::runtime_error("Not a valid IP address");
			}
		}
		else
		{
			throw std::runtime_error("Failed to resolve " + address);
		}
	}

	this->setup(family);

	if (this->socket != INVALID_SOCKET)
	{
		if (::connect(this->socket, addr, addrLen) == SOCKET_ERROR)
		{
			this->close();

			throw std::runtime_error("Failed to connect the TCP socket to [" + address + "]:" + std::to_string(port));
		}

		this->connected = true;
	}
}

void TcpSocket::connect(const std::string& address)
{
	std::pair<std::string, unsigned short> splittedAddress = Network::splitAddress(address);

	this->connect(splittedAddress.first, splittedAddress.second);
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
		Socket socket = ::accept(this->socket, nullptr, nullptr);

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
			this->close();
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
			this->close();
		}
	}
}

void TcpSocket::close()
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
				this->close();

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

void TcpSocket::setup(int family)
{
	this->close();

	this->socket = ::socket(family, SOCK_STREAM, IPPROTO_TCP);

	if (this->socket == INVALID_SOCKET)
	{
		throw std::runtime_error("Failed to initialize the TCP socket");
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
