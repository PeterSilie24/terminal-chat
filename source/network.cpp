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

#if defined(WINDOWS)

int close(SOCKET s)
{
	return closesocket(s);
}

#endif

std::vector<std::string> Network::resolveHostAddressesIPv4(const std::string& host)
{
	std::vector<std::string> addresses;

	addrinfo* info = nullptr;

	if (!getaddrinfo(unmapIPv4(host).c_str(), nullptr, nullptr, &info))
	{
		addrinfo* current = info;

		if (current)
		{
			do
			{
				sockaddr_in addr;

				if (current->ai_family = AF_INET && current->ai_addr && current->ai_addrlen == sizeof(addr))
				{
					std::memcpy(&addr, current->ai_addr, current->ai_addrlen);

					std::array<char, INET_ADDRSTRLEN> buffer;

					std::memset(buffer.data(), 0, buffer.size());

					if (inet_ntop(AF_INET, &(addr.sin_addr), buffer.data(), buffer.size()))
					{
						addresses.push_back(buffer.data());
					}
				}

				current = current->ai_next;
			}
			while (current);
		}

		freeaddrinfo(info);
	}

	return addresses;
}

std::vector<std::string> Network::resolveHostAddressesIPv6(const std::string& host)
{
	std::vector<std::string> addresses;

	addrinfo* info = nullptr;

	if (!getaddrinfo(unmapIPv4(host).c_str(), nullptr, nullptr, &info))
	{
		addrinfo* current = info;

		if (current)
		{
			do
			{
				sockaddr_in6 addr;

				if (current->ai_family = AF_INET6 && current->ai_addr && current->ai_addrlen == sizeof(addr))
				{
					std::memcpy(&addr, current->ai_addr, current->ai_addrlen);

					std::array<char, INET6_ADDRSTRLEN> buffer;

					std::memset(buffer.data(), 0, buffer.size());

					if (inet_ntop(AF_INET6, &(addr.sin6_addr), buffer.data(), buffer.size()))
					{
						addresses.push_back(buffer.data());
					}
				}

				current = current->ai_next;
			} while (current);
		}

		freeaddrinfo(info);
	}

	return addresses;
}

std::vector<std::string> Network::resolveHostAddresses(const std::string& host)
{
	std::vector<std::string> addresses;

	std::vector<std::string> addressesIPv4 = resolveHostAddressesIPv4(host);

	std::vector<std::string> addressesIPv6 = resolveHostAddressesIPv6(host);

	addresses.insert(addresses.end(), addressesIPv6.begin(), addressesIPv6.end());

	addresses.insert(addresses.end(), addressesIPv4.begin(), addressesIPv4.end());

	return addresses;
}

std::string Network::resolveHostIPv4(const std::string& host)
{
	std::vector<std::string> addresses = resolveHostAddressesIPv4(host);

	if (addresses.size())
	{
		return *(addresses.begin());
	}

	return "";
}

std::string Network::resolveHostIPv6(const std::string& host)
{
	std::vector<std::string> addresses = resolveHostAddressesIPv6(host);

	if (addresses.size())
	{
		return *(addresses.begin());
	}

	return "";
}

std::string Network::resolveHost(const std::string& host)
{
	std::vector<std::string> addresses = resolveHostAddresses(host);

	if (addresses.size())
	{
		return *(addresses.begin());
	}

	return "";
}

std::pair<std::string, unsigned short> Network::splitAddress(const std::string& address)
{
	std::string ipAddress = address;
	unsigned short port = DefaultPort;

	std::size_t pos = address.find('[');

	if (pos == 0)
	{
		pos = address.find(']');

		if (pos != std::string::npos)
		{
			ipAddress = std::string(address.begin() + 1, address.begin() + pos);

			std::string portStr = std::string(address.begin() + pos + 1, address.end());

			pos = portStr.rfind(':');

			if (pos == 0)
			{
				portStr = std::string(portStr.begin() + 1, portStr.end());

				std::stringstream stream(portStr);

				stream >> port;
			}
		}
	}
	else
	{
		pos = address.rfind(':');

		if (pos != std::string::npos && address.find(':') == address.rfind(':'))
		{
			ipAddress = std::string(address.begin(), address.begin() + pos);

			std::string portStr = std::string(address.begin() + pos + 1, address.end());

			std::stringstream stream(portStr);

			stream >> port;
		}
	}

	return std::pair<std::string, unsigned short>(ipAddress, port);
}

std::string Network::mapIPv4(const std::string& address)
{
	return "::ffff:" + address;
}

std::string Network::unmapIPv4(const std::string& address)
{
	std::size_t pos = address.find("::ffff:");

	if (pos == 0)
	{
		std::string ipv4Address(address.begin() + 7, address.end());

		pos = ipv4Address.find(':');

		std::size_t posDot = ipv4Address.find('.');

		if (pos == std::string::npos && posDot != std::string::npos)
		{
			return ipv4Address;
		}
	}

	return address;
}

void Network::startup()
{
	std::lock_guard<std::mutex> lockGuard(mutex);

	if (counter == 0)
	{
		#if defined(WINDOWS)

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
	std::lock_guard<std::mutex> lockGuard(mutex);

	counter--;

	if (counter == 0)
	{
		#if defined(WINDOWS)

		WSACleanup();

		#endif
	}
}

const unsigned short Network::DefaultPort = 1024;

const int Network::MaxConnections = SOMAXCONN;

int Network::counter = 0;

std::mutex Network::mutex;
