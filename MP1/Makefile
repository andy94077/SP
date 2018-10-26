CC=gcc
CFLAGS=-O2 -Wall -std=c11
SRC=loser.c md5.c
OBJ=$(src:.o)

all: $(OBJ)
	$(CC) $(CFLAGS) -o loser $(OBJ)
%.o:%.c
md5: md5.c
	$(CC) $(CFLAGS) -o md5 md5.c
clean:
	@rm -f loser