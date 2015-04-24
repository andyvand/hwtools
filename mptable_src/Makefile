CC=gcc
CFLAGS=-O2 -Wall -Wextra -Wshadow -Wno-sign-compare $(EXTRAOPTS)
OBJS=mptable.o

mptable: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f mptable *.o
