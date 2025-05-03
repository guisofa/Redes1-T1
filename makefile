CFLAGS = -Wall -g

all: server tst

pacote.o: pacote.c
	gcc $(CFLAGS) -c pacote.c
server: server.o pacote.o
	gcc $(CFLAGS) server.o pacote.o -o server
server.o: server.c
	gcc $(CFLAGS) -c server.c
tst: tst.o pacote.o
	gcc $(CFLAGS) tst.o pacote.o -o tst
tst.o : 
	gcc $(CFLAGS) -c tst.c
clean:
	rm -f *.o
purge:
	rm -f *.o
	rm -f main
	rm -f server
	rm -f tst
