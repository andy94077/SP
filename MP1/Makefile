CC=g++
CXXFLAGS= -O2 -Wall -std=c++17 -lcrypto

all: loser
loser: loser.cpp FileMd5.h
	$(CC) $(CXXFLAGS) -o loser loser.cpp

clean:
	-@rm -f loser
