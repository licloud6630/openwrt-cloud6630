CC = gcc
CFLAGS = -Wall
OBJS = ls-mdio.o

all: ls-mdio

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

ls-mdio: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f ls-mdio *.o
