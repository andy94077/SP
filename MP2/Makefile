CC=g++
CXXFLAGS= -Wextra -Wall -std=c++17
WITHMD5= -lcrypto

all: loser_peer
loser_peer: loser_peer.cpp
	$(CC) $(CXXFLAGS) -O2 -o $@ $<
debug: loser_peer.cpp
	$(CC) $(CXXFLAGS) $(WITHMD5) -g -o $@ $<
clean:
	-@rm -f loser_peer
