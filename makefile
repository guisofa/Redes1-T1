CFLAGS = -Wall -g

all: main server tst

main: main.o pacote.o
	gcc $(CFLAGS) main.o pacote.o -o main
main.o: main.c
	gcc $(CFLAGS) -c main.c
pacote.o: pacote.c
	gcc $(CFLAGS) -c pacote.c
server: server.o
	gcc $(CFLAGS) server.o -o server
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