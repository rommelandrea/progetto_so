CC = gcc
CFLAGS = -Wall -pedantic
LIBS = -lpthread
PROG = server

attuatore: funzioni.c server.c 
	$(CC) $(CFLAGS) -c funzioni.c server.c
	$(CC) funzioni.o sever.o -o $(PROG)

clean:
	rm -f *.o *~
	
cleanbin:
	rm -f $(PROG)