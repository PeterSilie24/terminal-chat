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

#include "server.hpp"

User::User(std::shared_ptr<TcpSocket> tcpSocket) : tcpSocket(tcpSocket)
{
	
}

bool User::isConnected() const
{
	if (this->tcpSocket)
	{
		return this->tcpSocket->isConnected();
	}

	return false;
}

bool User::hasName() const
{
	return this->name != "";
}

std::string User::getName()
{
	return this->name;
}

bool User::hasMessage() const
{
	return this->messages.size() > 0;
}

std::string User::getMessage()
{
	std::string str;

	if (this->messages.size() > 0)
	{
		str = this->messages.front();

		this->messages.pop();
	}

	return str;
}

void User::sendMessage(const std::string& message)
{
	if (this->tcpSocket)
	{
		this->tcpSocket->writeLine(message);
	}
}

void User::process()
{
	if (this->tcpSocket)
	{
		this->tcpSocket->process();

		while (this->tcpSocket->hasLine())
		{
			this->processMessage(this->tcpSocket->readLine());
		}

		if (this->hasName())
		{
			if (this->tcpSocket->hasTimedOut())
			{
				this->messages.push(this->name + " timed out");

				this->tcpSocket->close();
			}
			else if (!this->tcpSocket->isConnected())
			{
				this->messages.push(this->name + " left the chat room");
			}
		}
	}
}

void User::processMessage(const std::string& line)
{
	if (line.length() > 0)
	{
		if (!this->hasName())
		{
			this->name = line;

			this->messages.push(this->name + " joined the chat room");
		}
		else
		{
			this->messages.push(this->name + ": " + line);
		}
	}
}

Server::Server(unsigned short port)
{
	this->tcpSocket.bind(port);

	this->tcpSocket.listen();

	this->run = true;

	this->thread = std::thread([this]() { this->processNetwork(); });
}

Server::~Server()
{
	this->run = false;

	if (this->thread.joinable())
	{
		this->thread.join();
	}
}

void Server::acceptUser()
{
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

	while (this->tcpSocket.isAvailable())
	{
		this->users.push_back(std::shared_ptr<User>(new User(this->tcpSocket.accept())));
	}
}

void Server::writeMessage(const std::string& message)
{
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

	for (auto& user : this->users)
	{
		if (user)
		{
			user->sendMessage(message);
		}
	}
}

void Server::processUsers()
{
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

	std::vector<std::shared_ptr<User>>::iterator iter;

	for (iter = this->users.begin(); iter != this->users.end(); )
	{
		std::shared_ptr<User>& user = *iter;

		if (user)
		{
			user->process();

			while (user->hasMessage())
			{
				this->writeMessage(user->getMessage());
			}

			if (!user->isConnected())
			{
				iter = this->users.erase(iter);

				continue;
			}
		}

		iter++;
	}
}

void Server::processNetwork()
{
	while (this->run)
	{
		{
			std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

			this->acceptUser();

			this->processUsers();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
