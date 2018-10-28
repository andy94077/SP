CC=g++
CXXFLAGS= -Wall -std=c++14 -lcrypto
#You MUST use c++14 or c++11, c++17 will failed when you use -O2

all: loser
loser: loser.cpp FileMd5.h
	$(CC) $(CXXFLAGS) -O2 -o loser loser.cpp
debug: loser.cpp FileMd5.h
	$(CC) $(CXXFLAGS) -g -o loser loser.cpp
clean:
	-@rm -f loser
