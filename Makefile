

funzioni.o: funzioni.c header.h
	gcc -std=c89 -pedantic -c funzioni.c
	
	
utente: utente.c funzioni.o header.h
	gcc -std=c89 -pedantic utente.c funzioni.o -o utente
	
	
nodo: nodo.c funzioni.o header.h
	gcc -std=c89 -pedantic nodo.c funzioni.o -o nodo
	
master: master.c funzioni.o header.h
	gcc -std=c89 -pedantic master.c funzioni.o -o master
	
all:	
	make utente nodo master
	
clean:		
	rm -f *.o *~
	rm utente
	rm nodo
	rm master
	
run:
	./master
