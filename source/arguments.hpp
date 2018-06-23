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

#include <vector>
#include <string>

class Arguments
{
public:
	static void setArgs(int argc, char* argv[]);

	static bool hasFlag(const std::string& flag);

	static bool hasArgument(const std::string& argument);

	static std::string getArgument(const std::string& argument);

private:
	static std::vector<std::pair<std::string, std::string>> arguments;

	static std::vector<std::string> flags;
};
