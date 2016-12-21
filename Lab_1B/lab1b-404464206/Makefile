.SILENT:

default: server client

server:
	gcc -g -lmcrypt -pthread server.c -o server

client:
	gcc -g -lmcrypt -pthread client.c -o client

clean:
	rm -f client server *~ *.txt *.tar.gz *.o

dist:
	tar -zcvf lab1b-404464206.tar.gz server.c client.c Makefile README my.key
