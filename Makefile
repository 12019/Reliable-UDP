# $Id: Makefile,v 1.1 2011-12-02 01:27:49-08 - - $
# Michael Baptist - mbaptist@ucsc.edu
# Written: April 7, 2014	

GCC       = gcc -g -O0 -Wall -Wextra -std=gnu99 -pthread
CSOURCE   = client.c client_utils.c server.c rudp.c 
CHEADER   = client.h client_utils.h server.h rudp.h

all: server client clean 

server: server.o utils.o rudp.o  
	${GCC} -o server server.o utils.o rudp.o

server.o: server.c
	${GCC} -c server.c

client: client.o utils.o rudp.o
	${GCC} -o client client.o utils.o rudp.o
	./movecli.sh

client.o: client.c
	${GCC} -c client.c

utils.o: utils.c
	${GCC} -c utils.c

rudp.o: rudp.c
	${GCC} -c rudp.c

clean:
	rm *.o

wipe: clean
	rm client
	rm server

testcli:
	./clitests.sh

#need Doxygen installed for this.
docs:
	./docgen.sh

scpcats:
	scp client mbaptist@unix.ic.ucsc.edu:~/private/CE156/lab1
	scp client mbaptist@unix2.ic.ucsc.edu:~/private/CE156/lab1

