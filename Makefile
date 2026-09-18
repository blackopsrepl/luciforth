CC ?= cc
CFLAGS ?= -Wall

all: luciforth

luciforth: luciforth.c
	$(CC) $(CFLAGS) -o $@ luciforth.c

clean:
	rm -f luciforth *.o
.PHONY: all clean
