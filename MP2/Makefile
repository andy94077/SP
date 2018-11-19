CC=g++
CXXFLAGS= -Wextra -Wall -std=c++17 -lcrypto

all: loser_peer
loser: loser_peer.cpp
	$(CC) $(CXXFLAGS) -O2 -o loser loser.cpp
debug: loser.cpp
	$(CC) $(CXXFLAGS) -g -o loser loser.cpp
clean:
	-@rm -f loser
