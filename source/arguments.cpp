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
	std::vector<std::string> args;

	for (int i = 0; i < argc; i++)
	{
		args.push_back(argv[i]);
	}

	int max = argc - 1;

	for (int i = max; i >= 0; i--)
	{
		if (args[i].length())
		{
			if (args[i][0] == '-' || args[i][0] == '/')
			{
				flags.push_back(args[i].data() + 1);

				if (i < max)
				{
					arguments.push_back(std::pair<std::string, std::string>(args[i].data() + 1, args[i + 1]));
				}

				max = i - 1;
			}
		}
	}
}

bool Arguments::hasFlag(const std::string& flag)
{
	for (auto& tempFlag : flags)
	{
		if (tempFlag == flag)
		{
			return true;
		}
	}

	return false;
}

bool Arguments::hasArgument(const std::string& argument)
{
	for (auto& tempArgument : arguments)
	{
		if (tempArgument.first == argument)
		{
			return true;
		}
	}

	return false;
}

std::string Arguments::getArgument(const std::string& argument)
{
	for (auto& tempArgument : arguments)
	{
		if (tempArgument.first == argument)
		{
			return tempArgument.second;
		}
	}

	return "";
}

std::vector<std::pair<std::string, std::string>> Arguments::arguments;

std::vector<std::string> Arguments::flags;
