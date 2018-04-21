CXXFLAGS = -std=c++11
LDFLAGS = -lpthread

HPP_FILES = source/arguments.hpp source/client.hpp source/network.hpp source/server.hpp source/tcp-socket.hpp source/terminal.hpp
CPP_FILES = source/arguments.cpp source/client.cpp source/network.cpp source/server.cpp source/tcp-socket.cpp source/terminal.cpp source/terminal-chat.cpp

terminal-chat: $(HPP_FILES) $(CPP_FILES)
	mkdir -p bin
	g++ $(CXXFLAGS) -o bin/terminal-chat $(CPP_FILES) $(LDFLAGS)
