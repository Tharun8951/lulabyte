CFLAGS = -Wall -O2 -g

all: clean lulabyte

lulabyte: main.o eventLoop.o communicate.o 
	gcc ${CFLAGS} $^ -o $@

main.o: main.c
	gcc ${CFLAGS} -c $< -o $@

eventLoop.o: eventLoop.c 
	gcc ${CFLAGS} -c $< -o $@

communicate.o: communicate.c 
	gcc ${CFLAGS} -c $< -o $@

clean: 
	rm -rf *.o lulabyte