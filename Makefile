all: ufclient ufserver

ufclient: ufclient.c
	gcc ufclient.c -o ufclient

ufserver: ufserver.c
	gcc ufserver.c -o ufserver

clean: 
	rm ufclient
	rm ufserver