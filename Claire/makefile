all: serveur client 
serveur: serveur.c 
	gcc serveur.c -o serveur -lpthread
client: client.c
	gcc client.c -o client
clean:
	rm -f serveur
	rm -f client