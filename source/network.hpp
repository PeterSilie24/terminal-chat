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

#include <stdexcept>
#include <string>

#ifdef WIN32

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

typedef SOCKET Socket;

extern int close(SOCKET s);

#define SHUT_WR SD_SEND

#else

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
	static void startup();

	static void cleanup();

	static const unsigned short DefaultPort;

	static const int MaxConnections;

private:
	static int counter;
};
