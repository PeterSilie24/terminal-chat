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

Terminal::Terminal(const std::string& label) : label(label + ": "), process(false)
{
	signal(SIGINT, this->handlerSignal);

	signal(SIGTERM, this->handlerSignal);

	#if defined(WINDOWS)

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

	GetConsoleMode(hStdin, &(this->dwMode));

	SetConsoleMode(hStdin, (this->dwMode) & (~ENABLE_ECHO_INPUT) & (~ENABLE_LINE_INPUT));

	#elif defined(POSIX)

	termios term;

	tcgetattr(STDIN_FILENO, &term);

	this->oldTerm = term;

	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;

	tcsetattr(STDIN_FILENO, TCSANOW, &term);

	#endif

	this->run = true;

	this->thread = std::thread([this] { this->processInput(); });
}

Terminal::~Terminal()
{
	this->disableInput();

	this->run = false;

	if (this->thread.joinable())
	{
		this->thread.join();
	}

	#if defined(WINDOWS)

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hStdin, (this->dwMode));

	#elif defined(POSIX)

	tcsetattr(STDIN_FILENO, TCSANOW, &(this->oldTerm));

	#endif
}

void Terminal::enableInput(bool enable)
{
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

	if (enable)
	{
		if (!this->process)
		{
			this->process = true;

			std::cout << this->label << std::flush;
		}
	}
	else
	{
		if (this->process)
		{
			this->process = false;

			this->erase(this->label.length() + this->input.length());

			this->input.clear();
		}
	}
}

void Terminal::disableInput()
{
	this->enableInput(false);
}

bool Terminal::hasLine() const
{
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

	return this->lines.size() > 0;
}

std::string Terminal::getLine()
{
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

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
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

	if (this->process)
	{
		this->erase(this->label.length() + this->input.length());
	}

	this->label = label + ": ";

	if (this->process)
	{
		std::cout << this->label << this->input << std::flush;

		this->checkForNewline();
	}
}

bool Terminal::shouldExit() const
{
	return this->exit;
}

void Terminal::printLine(const std::string& line)
{
	std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

	if (this->process)
	{
		this->erase(this->label.length() + this->input.length());
	}

	std::cout << line << std::endl;

	if (this->process)
	{
		std::cout << this->label << this->input << std::flush;

		this->checkForNewline();
	}
}

Coord Terminal::getCursorPosition() const
{
	#if defined(WINDOWS)

	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO cbsi;

	GetConsoleScreenBufferInfo(hStdout, &cbsi);

	return Coord(static_cast<int>(cbsi.dwCursorPosition.X), static_cast<int>(cbsi.dwCursorPosition.Y));

	#elif defined(POSIX)

	Coord cursorPosition;

	int n = static_cast<int>(this->label.length() + this->input.length());

	cursorPosition.x = n - (n / this->getMaximumSize().x) * this->getMaximumSize().x;
	cursorPosition.y = n / this->getMaximumSize().x;

	return cursorPosition;

	#endif
}

void Terminal::setCursorPosition(const Coord& cursorPosition)
{
	#if defined(WINDOWS)
	
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD dwCursorPosition;
	dwCursorPosition.X = static_cast<SHORT>(cursorPosition.x);
	dwCursorPosition.Y = static_cast<SHORT>(cursorPosition.y);

	SetConsoleCursorPosition(hStdout, dwCursorPosition);

	#elif defined(POSIX)

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
	#if defined(WINDOWS)

	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO cbsi;

	GetConsoleScreenBufferInfo(hStdout, &cbsi);

	return Coord(static_cast<int>(cbsi.dwMaximumWindowSize.X), static_cast<int>(cbsi.dwMaximumWindowSize.Y));

	#elif defined(POSIX)
	
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

	int toRemove = static_cast<int>(n);

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

	#if defined(WINDOWS)

	std::cout << std::string(n, ' ');

	this->setCursorPosition(cursorPosition);

	#elif defined(POSIX)

	std::cout << (char)0x1B << "[s" << std::flush;

	std::cout << std::string(n, ' ');

	std::cout << (char)0x1B << "[u" << std::flush;

	#endif
}

char Terminal::getchar()
{
	char c = '\0';

	#if defined(WINDOWS)

	if (_kbhit())
	{
		c = static_cast<char>(_getch());
	}

	#elif defined(POSIX)

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
	#if defined(WINDOWS)

	#elif defined(POSIX)

	if (this->getCursorPosition().x == 0)
	{
		std::cout << "\n" << std::flush;
	}

	#endif
}

void Terminal::processInput()
{
	while (this->run)
	{
		{
			std::lock_guard<std::recursive_mutex> lockGuard(this->mutex);

			char c = '\0';

			while ((c = this->getchar()) != '\0')
			{
				if (this->process)
				{
					switch (c)
					{
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
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void Terminal::handlerSignal(int signal)
{
	exit = true;
}

std::atomic_bool Terminal::exit;
