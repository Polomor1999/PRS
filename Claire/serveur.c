// Server program
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


#define MAXLINE 1024
#define SEGMENT_LENGTH 1030


void send_file_data(FILE* fp, int sockfd, struct sockaddr_in addr)
{
  int n,flag,lendata;
//numéro de sequence 
  long compteur = 0;
  char buffer[MAXLINE];
  bzero(buffer, MAXLINE);
  char bufftotal[SEGMENT_LENGTH];
  bzero(bufftotal, SEGMENT_LENGTH);
  flag = 1;
  // Sending the data

//  FILE* f2 = fopen("kjh.pdf", "wb");

  while (flag) //je lit tout d'un coup
  {
	lendata=fread(buffer, 1,MAXLINE, fp);//taille que j'ai reussi a lire dans mon file
	flag = !(lendata<1024); //flag=0 si on atteint la fin du file

	// convertir int to char en respectant le nb ce charcatere 
	// mettre au début du buffer
	// remplir le buffer avec les datas
	//printf("Data: %s\n",buffer);
    compteur++;
	printf("%d\n",lendata);

	//long to char 
	sprintf(bufftotal, "%06d\n", compteur);
	printf("numero : %s\n",bufftotal);
	
	//if (strcmp(buffer2,ACK + numero) ==0 ){
		//send apres avoir recu le ack 

	//add compteur to buffer
	memcpy(bufftotal+6,buffer,lendata);


	

	//printf("[SENDING] Data: %s\n",buffer);
    n = sendto(sockfd, bufftotal, lendata+6, 0, (struct sockaddr*)&addr, sizeof(addr));
    if (n == -1)
    {
      perror("[ERROR] sending data to the client.");
      exit(1);
    }

/* FOR DEBUG
	bzero(buffer, MAXLINE);
	memcpy(buffer,numero+6,lendata);
    //printf("[RECEVING] Data: ACK%d %s\n",buffer);
    //fprintf(fp, "%s", buffer);
    fwrite(buffer,lendata,1,f2);*/

    bzero(buffer, MAXLINE);
	bzero(bufftotal, SEGMENT_LENGTH);
  }
 // fclose(f2);

  // Sending the 'END'
  strcpy(buffer, "FIN");
  sendto(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&addr, sizeof(addr));

  fclose(fp);
  
}




int main(int argc,char* argv[])
{
    int port;

if (argc < 2)
{
    printf("Too few arguments given.\n");
    exit(1);
}
else if (argc > 2)
{
    printf("Too many arguments given.\n");
    exit(1);
}
else
{
    port = atoi(argv[1]);
}

	int listenfd, connfd, socketudp, nready, maxfdp1,newsocketudp,mb_octet;
	char buffer[MAXLINE];
	char buffer2[MAXLINE];
	pid_t childpid;
	fd_set rset;
	ssize_t n,b;
	socklen_t len;
	const int on = 1;
	struct sockaddr_in cliaddr, servaddr,servaddr1;
	char* message = "SYN-ACK1222"; //nouveau port pour socket d'écoute avec le client 
	char* message2 = "fleur.jpg";
	char* message3 = "let'sgo";
	void sig_chld(int);

	
	//listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	// binding server addr structure to listenfd
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	//listen(listenfd, 10);

	/* create UDP socket */
	socketudp = socket(AF_INET, SOCK_DGRAM, 0);
	// binding server addr structure to udp sockfd
	bind(socketudp, (struct sockaddr*)&servaddr, sizeof(servaddr));

	// clear the descriptor set
	FD_ZERO(&rset);

	//create new udp socket with new port
	bzero(&servaddr1, sizeof(servaddr1));
	servaddr1.sin_family = AF_INET;
	servaddr1.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr1.sin_port = htons(1222);
	newsocketudp = socket(AF_INET, SOCK_DGRAM, 0);

    // Bind the socket with the server address 
  	bind(newsocketudp, (struct sockaddr*)&servaddr1, sizeof(servaddr));



	for (;;) { //mettre la boucle for apres l'ouvertiure de connexion + rajouterfork pr gerer plusisuers client 

		// set socketudp in readset
	
		FD_SET(socketudp, &rset);

        //OUVERTURE DE CONNEXION
		// if udp socket is readable receive the message.
		len = sizeof(cliaddr);
		bzero(buffer, sizeof(buffer));
		printf("\nMessage from Client on socketbasic: ");
		n = recvfrom(socketudp, buffer, sizeof(buffer), 0,
					(struct sockaddr*)&cliaddr, &len);

		if (strcmp(buffer,"SYN") ==0 ){
			puts(buffer);
			//puts(message);
			mb_octet = sendto(socketudp, (const char*)message, strlen(message), 0,
			(struct sockaddr*)&cliaddr, sizeof(cliaddr));
			//printf("octet:%d\n", mb_octet);
		}
		if (strcmp(buffer,"ACK") ==0 ){
			puts(buffer);
			//FIN D'OUVERTURE DE CONNEXION
			//sendto(socketudp, (const char*)message3, strlen(message3), 0,(struct sockaddr*)&cliaddr, sizeof(cliaddr));
			//printf("FIN D'OUVERTURE DE CONNEXION\n");

			while(1){
				//ouverture nouvelle socket pr comm exclusivement avec le client 
				bzero(buffer2, sizeof(buffer2));
				printf("\nMessage from Client on newsocket: ");
				b = recvfrom(newsocketudp, buffer2, sizeof(buffer2), 0,(struct sockaddr*)&cliaddr, &len);
				//printf("octets recus :%d\n", b);
				//if (strcmp(buffer2,"GGO") ==0 ){
				//puts(buffer2);
				sendto(newsocketudp, (const char*)message2, strlen(message2), 0,(struct sockaddr*)&cliaddr, sizeof(cliaddr));
				char *filename = "fleur.jpg";
  				FILE *fp = fopen(filename, "rb");
				
				// Sending the file data to the server
				send_file_data(fp, newsocketudp, cliaddr);
				printf("[SUCCESS] Data transfer complete.\n");
				//close(newsocketudp);

				
			}
		
		}
	}

	
}




