CFLAGS = -Wall -g

all: server tst

tst: tst.o pacote.o
	gcc $(CFLAGS) tst.o pacote.o -o tst
server: server.o pacote.o
	gcc $(CFLAGS) server.o pacote.o -o server

tst.o : tst.c
	gcc $(CFLAGS) -c tst.c
server.o: server.c
	gcc $(CFLAGS) -c server.c
pacote.o: pacote.c
	gcc $(CFLAGS) -c pacote.c

clean:
	rm -f *.o
purge:
	rm -f *.o
	rm -f main
	rm -f server
	rm -f tst