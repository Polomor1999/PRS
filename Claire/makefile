all: serveur1 serveur2 client 
serveur1: serveur1.c 
	gcc serveur1.c -o serveur1 -lpthread

serveur2: serveur2.c 
	gcc serveur2.c -o serveur2 -lpthread

client: client.c
	gcc client.c -o client
clean:
	rm -f serveur2
	rm -f serveur1
	rm -f client