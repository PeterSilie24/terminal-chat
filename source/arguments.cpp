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

void Arguments::setArgs(int argc, char* argv[])
{
	args.clear();

	for (int i = 0; i < argc; i++)
	{
		args.push_back(argv[i]);
	}
}

bool Arguments::hasArg(const std::string& arg)
{
	for (auto& argument : args)
	{
		if (argument == ("/" + arg) || argument == ("-" + arg))
		{
			return true;
		}
	}

	return false;
}

std::string Arguments::getArg(const std::string& arg)
{
	if (args.size() > 1)
	{
		for (std::size_t i = 0; i < args.size() - 1; i++)
		{
			if (args[i] == ("/" + arg) || args[i] == ("-" + arg))
			{
				return args[i + 1];
			}
		}
	}

	return "";
}

std::vector<std::string> Arguments::args;
