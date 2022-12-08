all: serveur client oui
serveur: serveur.c 
	gcc -fno-stack-protector serveur.c -o serveur 
client: client.c
	gcc client.c -o client
oui: oui.c
	gcc oui.c -o oui
clean:
	rm -f serveur
	rm -f client
	rm -f oui