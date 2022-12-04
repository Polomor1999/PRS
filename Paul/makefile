all: serveur client 
serveur: serveur.c 
	gcc -fno-stack-protector serveur.c -o serveur 
client: client.c
	gcc client.c -o client
clean:
	rm -f serveur
	rm -f client