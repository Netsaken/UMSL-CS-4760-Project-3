CC = gcc
CFLAGS = -g -Wshadow -std=gnu99

all: master slave

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

master: master.o
	$(CC) $(CFLAGS) -o $@ $^

slave: slave.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	/bin/rm -f *.o