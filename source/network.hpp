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

#include "platform.hpp"

#include <stdexcept>
#include <string>
#include <mutex>
#include <array>
#include <vector>
#include <cstring>
#include <sstream>

#if defined(WINDOWS)

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

typedef SOCKET Socket;

extern int close(SOCKET s);

#define SHUT_WR SD_SEND

#elif defined(POSIX)

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

typedef int Socket;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#endif

class Network
{
public:
	static std::vector<std::string> resolveHostAddressesIPv4(const std::string& host);

	static std::vector<std::string> resolveHostAddressesIPv6(const std::string& host);

	static std::vector<std::string> resolveHostAddresses(const std::string& host);

	static std::string resolveHostIPv4(const std::string& host);

	static std::string resolveHostIPv6(const std::string& host);

	static std::string resolveHost(const std::string& host);

	static std::pair<std::string, unsigned short> splitAddress(const std::string& address);

	static std::string mapIPv4(const std::string& address);

	static std::string unmapIPv4(const std::string& address);

	static void startup();

	static void cleanup();

	static const unsigned short DefaultPort;

	static const int MaxConnections;

private:
	static int counter;

	static std::mutex mutex;
};
