CC=gcc
CFLAGS=-O2 -Wall -std=c++17
SRC=loser.c
OBJ=$(src:.o)

all: $(OBJ)
	$(CC) $(CFLAGS) -o loser $(OBJ)
%.o:%.c
md5: md5.c
	$(CC) $(CFLAGS) -o md5 md5.c -lcrypto
clean:
	-@rm -f loser $(OBJ)