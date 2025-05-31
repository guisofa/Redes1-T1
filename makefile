CFLAGS = -Wall -g

all: server cliente

cliente: cliente.o pacote.o jogo.o
	gcc $(CFLAGS) cliente.o pacote.o jogo.o -o cliente
server: server.o pacote.o jogo.o
	gcc $(CFLAGS) server.o pacote.o jogo.o -o server

cliente.o : cliente.c
	gcc $(CFLAGS) -c cliente.c
server.o: server.c
	gcc $(CFLAGS) -c server.c
pacote.o: pacote.c
	gcc $(CFLAGS) -c pacote.c
jogo.o : jogo.c
	gcc $(CFLAGS) -c jogo.c

clean:
	rm -f *.o
purge:
	rm -f *.o
	rm -f main
	rm -f server
	rm -f cliente
	rm -f *copia