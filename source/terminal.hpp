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

#include <queue>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <csignal>
#include <string>
#include <algorithm>

#if defined(WINDOWS)

#include <Windows.h>
#include <conio.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#elif defined(POSIX)

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#endif

struct Coord
{
public:
	Coord();
	Coord(int x, int y);

	int x;
	int y;
};

class Terminal
{
public:
	Terminal(const std::string& label = "Message");

	~Terminal();

	void enableInput(bool enable = true);

	void disableInput();

	bool hasLine() const;

	std::string getLine();

	void setLabel(const std::string& label);

	bool shouldExit() const;

	void printLine(const std::string& line);

private:
	Coord getCursorPosition() const;

	void setCursorPosition(const Coord& cursorPosition);

	Coord getMaximumSize() const;

	void erase(std::size_t n);

	char getchar();

	void checkForNewline();

	void processInput();

	static void handlerSignal(int signal);

	std::string label;

	std::string input;

	std::queue<std::string> lines;

	std::thread thread;
	mutable std::recursive_mutex mutex;

	std::atomic_bool run;
	bool process;

	static std::atomic_bool exit;

	#if defined(WINDOWS)
	
	DWORD dwMode;

	#elif defined(POSIX)

	termios oldTerm;

	#endif
};
