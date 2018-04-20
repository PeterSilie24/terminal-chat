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

#include "terminal.hpp"

Coord::Coord() : x(0), y(0)
{

}

Coord::Coord(int x, int y) : x(x), y(y)
{

}

Terminal::Terminal(const std::string& label) : label(label + ": "), run(false)
{
	signal(SIGINT, this->handlerSignal);

	signal(SIGTERM, this->handlerSignal);

	#ifdef WIN32

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

	GetConsoleMode(hStdin, &(this->dwMode));

	SetConsoleMode(hStdin, (this->dwMode) & (~ENABLE_ECHO_INPUT) & (~ENABLE_LINE_INPUT));

	#else

	termios term;

	tcgetattr(STDIN_FILENO, &term);

	this->oldTerm = term;

	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;

	tcsetattr(STDIN_FILENO, TCSANOW, &term);

	#endif
}

Terminal::~Terminal()
{
	this->disableInput();

	#ifdef WIN32

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hStdin, (this->dwMode));

	#else

	tcsetattr(STDIN_FILENO, TCSANOW, &(this->oldTerm));

	#endif
}

void Terminal::enableInput(bool enable)
{
	if (enable && !this->run)
	{
		std::lock_guard<std::mutex> lockGuard(this->mutex);

		this->run = true;

		std::cout << this->label << std::flush;

		this->thread = std::thread([this] { this->processInput(); });
	}
	else if (this->run)
	{
		{
			std::lock_guard<std::mutex> lockGuard(this->mutex);

			this->run = false;
		}

		if (this->thread.joinable())
		{
			this->thread.join();
		}

		this->erase(this->label.length() + this->input.length());

		this->input.clear();
	}
}

void Terminal::disableInput()
{
	this->enableInput(false);
}

bool Terminal::hasLine() const
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	return this->lines.size() > 0;
}

std::string Terminal::getLine()
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	std::string str;

	if (this->lines.size() > 0)
	{
		str = this->lines.front();

		this->lines.pop();
	}

	return str;
}

void Terminal::setLabel(const std::string& label)
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	if (this->run)
	{
		this->erase(this->label.length() + this->input.length());
	}

	this->label = label + ": ";

	if (this->run)
	{
		std::cout << this->label << this->input << std::flush;

		this->checkForNewline();
	}
}

bool Terminal::shouldExit() const
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	return this->exit;
}

void Terminal::printLine(const std::string& line)
{
	std::lock_guard<std::mutex> lockGuard(this->mutex);

	if (this->run)
	{
		this->erase(this->label.length() + this->input.length());
	}

	std::cout << line << std::endl;

	if (this->run)
	{
		std::cout << this->label << this->input << std::flush;

		this->checkForNewline();
	}
}

Coord Terminal::getCursorPosition() const
{
	#ifdef WIN32

	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO cbsi;

	GetConsoleScreenBufferInfo(hStdout, &cbsi);

	return Coord(static_cast<int>(cbsi.dwCursorPosition.X), static_cast<int>(cbsi.dwCursorPosition.Y));

	#else

	Coord cursorPosition;

	int n = static_cast<int>(this->label.length() + this->input.length());

	cursorPosition.x = n - (n / this->getMaximumSize().x) * this->getMaximumSize().x;
	cursorPosition.y = n / this->getMaximumSize().x;

	return cursorPosition;

	#endif
}

void Terminal::setCursorPosition(const Coord& cursorPosition)
{
	#ifdef WIN32
	
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD dwCursorPosition;
	dwCursorPosition.X = static_cast<SHORT>(cursorPosition.x);
	dwCursorPosition.Y = static_cast<SHORT>(cursorPosition.y);

	SetConsoleCursorPosition(hStdout, dwCursorPosition);

	#else

	Coord currentCursorPosition = this->getCursorPosition();

	if ((currentCursorPosition.x - cursorPosition.x) > 0)
	{
		std::cout << (char)0x1B << '[' << (currentCursorPosition.x - cursorPosition.x) << 'D' << std::flush;
	}
	else if ((cursorPosition.x - currentCursorPosition.x) > 0)
	{
		std::cout << (char)0x1B << '[' << (cursorPosition.x - currentCursorPosition.x) << 'C' << std::flush;
	}

	if ((currentCursorPosition.y - cursorPosition.y) > 0)
	{
		std::cout << (char)0x1B << '[' << (currentCursorPosition.y - cursorPosition.y) << 'A' << std::flush;
	}
	else if ((cursorPosition.y - currentCursorPosition.y) > 0)
	{
		std::cout << (char)0x1B << '[' << (cursorPosition.y - currentCursorPosition.y) << 'B' << std::flush;
	}

	#endif
}

Coord Terminal::getMaximumSize() const
{
	#ifdef WIN32

	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO cbsi;

	GetConsoleScreenBufferInfo(hStdout, &cbsi);

	return Coord(static_cast<int>(cbsi.dwMaximumWindowSize.X), static_cast<int>(cbsi.dwMaximumWindowSize.Y));

	#else
	
	struct winsize size;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	return Coord(size.ws_col, size.ws_row);

	#endif
}

void Terminal::erase(std::size_t n)
{
	Coord cursorPosition = this->getCursorPosition();

	Coord maximumSize = this->getMaximumSize();

	int x = static_cast<int>(cursorPosition.x);
	int y = static_cast<int>(cursorPosition.y);

	int maxX = static_cast<int>(maximumSize.x);
	int maxY = static_cast<int>(maximumSize.y);

	int toRemove = n;

	if (toRemove > 0)
	{
		int xDiff = std::min(toRemove, x);

		x -= xDiff;

		toRemove -= xDiff;
	}

	if (toRemove && maxX > 0)
	{
		int yDiff = toRemove / maxX;

		y -= yDiff;

		toRemove -= yDiff * maxX;
	}

	if (toRemove)
	{
		x = maxX - toRemove;

		y -= 1;
	}

	cursorPosition = Coord(x, y);

	this->setCursorPosition(cursorPosition);

	#ifdef WIN32

	std::cout << std::string(n, ' ');

	this->setCursorPosition(cursorPosition);

	#else

	std::cout << (char)0x1B << "[s" << std::flush;

	std::cout << std::string(n, ' ');

	std::cout << (char)0x1B << "[u" << std::flush;

	#endif
}

char Terminal::getchar()
{
	char c = '\0';

	#ifdef WIN32

	if (_kbhit())
	{
		c = static_cast<char>(_getch());
	}

	#else

	struct timeval tv;
	fd_set rdfs;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&rdfs);
	FD_SET(STDIN_FILENO, &rdfs);

	select(STDIN_FILENO + 1, &rdfs, nullptr, nullptr, &tv);

	if (FD_ISSET(STDIN_FILENO, &rdfs) > 0)
	{
		read(STDIN_FILENO, &c, 1);
	}

	#endif

	if ((c >= 0x00 && c <= 0x1F) && c != '\b' && c != 0x7F && c != '\n' && c != '\r' && c != 0x1B)
	{
		c = '\0';
	}

	return c;
}

void Terminal::checkForNewline()
{
	#ifdef WIN32

	#else

	if (this->getCursorPosition().x == 0)
	{
		std::cout << "\n" << std::flush;
	}

	#endif
}

void Terminal::processInput()
{
	while (true)
	{
		{
			std::lock_guard<std::mutex> lockGuard(this->mutex);

			if (!this->run)
			{
				break;
			}

			char c = this->getchar();

			switch (c)
			{
			case '\0':
				break;
			case '\b':
			case 0x7F:
			{
				if (this->input.length() > 0)
				{
					this->erase(1);

					this->input.resize(input.length() - 1);
				}

				break;
			}
			case '\n':
			case '\r':
			{
				if (this->input.length() > 0)
				{
					this->erase(this->input.length());

					this->lines.push(input);

					this->input.clear();
				}

				break;
			}
			case 0x1B:
			{
				this->exit = true;

				break;
			}
			default:
			{
				this->input.resize(this->input.length() + 1, c);

				std::cout << c << std::flush;
				
				this->checkForNewline();

				break;
			}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void Terminal::handlerSignal(int signal)
{
	exit = true;
}

bool Terminal::exit = false;
