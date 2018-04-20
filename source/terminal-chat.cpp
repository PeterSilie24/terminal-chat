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

#include "arguments.hpp"

#include "server.hpp"
#include "client.hpp"

#include "terminal.hpp"

std::string msgDefault =
"terminal-chat: Invalid input\n"
"Specify -? or -help for more information";

std::string msgHelp =
"terminal-chat:\n"
"Help: -? or -help\n"
"Host: -h -n [name]\n"
"Join: -j [ip[:port]] -n [name]";

int main(int argc, char* argv[])
{
	Arguments::setArgs(argc, argv);
	
	Terminal terminal;

	if (Arguments::hasArg("help") || Arguments::hasArg("?"))
	{
		terminal.printLine(msgHelp);
	}
	else if (!Arguments::hasArg("help") && !Arguments::hasArg("?") && !Arguments::hasArg("n") && (!Arguments::hasArg("h") || !Arguments::hasArg("j")))
	{
		terminal.printLine(msgDefault);
	}
	else
	{
		std::string name = Arguments::getArg("n");

		std::string address = "127.0.0.1";

		if (Arguments::hasArg("j"))
		{
			address = Arguments::getArg("j");
		}

		try
		{
			std::shared_ptr<Server> server;

			if (Arguments::hasArg("h"))
			{
				terminal.printLine("Hosting a chat room ...");

				server = std::shared_ptr<Server>(new Server());
			}
			else
			{
				terminal.printLine("Connecting to " + address +  " ...");
			}

			std::shared_ptr<Client> client(new Client(name, address));

			terminal.enableInput();

			while (!terminal.shouldExit())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

				if (client)
				{
					while (terminal.hasLine())
					{
						client->sendMessage(terminal.getLine());
					}

					while (client->hasMessage())
					{
						terminal.printLine(client->getMessage());
					}

					if (client->isClosed())
					{
						break;
					}
				}
			}
		}
		catch (std::runtime_error runtimeError)
		{
			terminal.printLine(runtimeError.what());

			return 1;
		}
	}
	
	return 0;
}
