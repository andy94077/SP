CC=g++
CXXFLAGS= -Wextra -Wall -std=c++17 -lcrypto

all: loser
loser: loser.cpp FileMd5.h
	$(CC) $(CXXFLAGS) -O2 -o loser loser.cpp
debug: loser.cpp FileMd5.h
	$(CC) $(CXXFLAGS) -g -o loser loser.cpp
clean:
	-@rm -f loser
