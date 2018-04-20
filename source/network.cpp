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

#include "network.hpp"

#ifdef WIN32

int close(SOCKET s)
{
	return closesocket(s);
}

#endif

void Network::startup()
{
	if (counter == 0)
	{
		#ifdef WIN32

		WSADATA wsaData;

		WORD requiredWinsockVersion = MAKEWORD(2, 0);

		if (WSAStartup(requiredWinsockVersion, &wsaData))
		{
			if (!(wsaData.wVersion >= requiredWinsockVersion))
			{
				throw std::runtime_error("Failed to initialize Winsock (At least Winsock 2.0 required)");
			}
			else
			{
				throw std::runtime_error("Failed to initialize Winsock");
			}
		}

		#endif
	}

	counter++;
}

void Network::cleanup()
{
	counter--;

	if (counter == 0)
	{
		#ifdef WIN32

		WSACleanup();

		#endif
	}
}

const unsigned short Network::DefaultPort = 1024;

const int Network::MaxConnections = SOMAXCONN;

int Network::counter = 0;
