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

#include "client.hpp"

Client::Client(const std::string& name, const std::string& address) : run(true)
{
	this->tcpSocket = std::shared_ptr<TcpSocket>(new TcpSocket());

	if (this->tcpSocket)
	{
		this->tcpSocket->connect(address);
	}

	this->sendMessage(name);

	this->thread = std::thread([this]() { this->processNetwork(); });
}

Client::~Client()
{
	{
		std::lock_guard<std::mutex> lockGuard(this->mutex);

		this->run = false;
	}

	if (this->thread.joinable())
	{
		this->thread.join();
	}
}

bool Client::isClosed() const
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	return !this->run;
}

bool Client::hasMessage() const
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	return this->messages.size() > 0;
}

std::string Client::getMessage()
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	std::string str;

	if (this->messages.size() > 0)
	{
		str = this->messages.front();

		this->messages.pop();
	}

	return str;
}

void Client::sendMessage(const std::string& message)
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	if (this->tcpSocket)
	{
		this->tcpSocket->writeLine(message);
	}
}

void Client::processMessage(const std::string& line)
{
	if (line.length() > 0)
	{
		this->messages.push(line);
	}
}

void Client::processNetwork()
{
	while (true)
	{
		{
			std::lock_guard<std::mutex> lockGuard(this->mutex);

			if (!this->run)
			{
				break;
			}

			if (this->tcpSocket)
			{
				this->tcpSocket->process();

				while (this->tcpSocket->hasLine())
				{
					this->processMessage(this->tcpSocket->readLine());
				}

				if (this->tcpSocket->hasTimedOut())
				{
					this->messages.push("Connection has been lost");

					this->tcpSocket->close();

					this->run = false;
				}
				else if (!this->tcpSocket->isConnected())
				{
					this->messages.push("The server has been closed");

					this->run = false;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
