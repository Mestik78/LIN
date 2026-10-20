CC = gcc
CFLAGS = -Wall -g

all: cpuinfo

cpuinfo: cpuinfo.c
	$(CC) $(CFLAGS) -o cpuinfo cpuinfo.c

clean:
	rm -f cpuinfo

.PHONY: all clean
