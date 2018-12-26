CC = gcc
CFLAG = -std=c11 -Wall -c -g -I./

all: xheap.o

xheap.o:  xmalloc.c
	$(CC) $(CFLAG) $^ -o $@

clean:
	rm -f xheap.o
