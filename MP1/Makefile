CC=g++
CFLAGS=-O2 -Wall -std=c11
CXXFLAGS= -O2 -Wall -std=c++17

all: loser
loser: loser.cpp
	$(CC) $(CXXFLAGS) -o loser loser.cpp
md5: md5.c
	$(CC) $(CFLAGS) -o md5 md5.c -lcrypto
clean:
	-@rm -f loser