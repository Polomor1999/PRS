// UDP client program
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXLINE 1024
#define SEGMENT_LENGTH 6

void write_file(int sockfd, struct sockaddr_in addr)
{

  char* filename = "server.pdf";
  int lendata;
  //numéro de sequence
  long compteur = 1; //commence a 1 je sais plus pq
  char buffer[MAXLINE];
  socklen_t addr_size;

  char buffersansnumero[MAXLINE];
  bzero(buffersansnumero, MAXLINE);
  char numero[SEGMENT_LENGTH];
  bzero(numero, SEGMENT_LENGTH);

  // Creating a file.
  FILE* fp = fp = fopen(filename, "wb");

  // Receiving the data and writing it into the file.
  while (1)
  {
    addr_size = sizeof(addr);
    lendata = recvfrom(sockfd, buffer, MAXLINE+6, 0, (struct sockaddr*)&addr, &addr_size);
    char end[3];
    memcpy(end,buffer,3);
    if (strncmp(end, "END",3) == 0)
    {
      break;
    }
    else{
     
    //sprintf(numero, "%.6s\n", buffer); //recup num de sequence
    memcpy(numero,buffer,6);
    printf("ACK %s\n",numero);
    memcpy(buffersansnumero,buffer+6,lendata-6);
    //printf("[RECEVING] Data: ACK%d %s\n",buffer);
    //fprintf(fp, "%s", buffer);
    fwrite(buffersansnumero,lendata-6,1,fp);//j'écris tout une fois
    bzero(buffer, MAXLINE);
    bzero(numero, SEGMENT_LENGTH);
    bzero(buffersansnumero, MAXLINE);
       
    }
  }

  fclose(fp);
}



int main(int argc,char* argv[])
{
    int port_serveur;
    char* ip_serveur;

    if (argc < 3)
    {
        printf("Too few arguments given.\n");
        exit(1);
    }
    else if (argc > 3)
    {
        printf("Too many arguments given.\n");
        exit(1);
    }
    else
    {
        ip_serveur = argv[1]; //conv str en int
        port_serveur = atoi(argv[2]);
    }
        
        
    
    int sockfd, mb_octet;
	char buffer[MAXLINE];
    //char newport[4];
	char* message = "GGO";
    char* messageSYN = "SYN";
    char* messageACK = "ACK";
	struct sockaddr_in servaddr,newservaddr;

	int n, len;
	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket creation failed");
		exit(0);
	}


	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_serveur);
	servaddr.sin_addr.s_addr = inet_addr(ip_serveur);

    //socket maj pour avoir le nouveau port
    memset(&newservaddr, 0, sizeof(newservaddr));
    newservaddr.sin_family = AF_INET;
	newservaddr.sin_addr.s_addr = inet_addr(ip_serveur);

    //send SYN message to server
    sendto(
        sockfd, (const char*)messageSYN, strlen(messageSYN),
        0, (const struct sockaddr*)&servaddr,
        sizeof(servaddr));
    
    while(1) {
        
        //OUVERTURE DE CONNEXION
        
        // receive server's response
        printf("Message from server: ");
        bzero(buffer,sizeof(buffer));
        
        n = recvfrom(sockfd, buffer, MAXLINE,
                    0, (struct sockaddr*)&servaddr,
                    &len);
        //puts(buffer);
        //printf("octet recus %d\n", n);

        if (strncmp(buffer,"SYN_ACK|",8) ==0 ){
            puts(buffer);
            char delim[]="|";
            char *newport = strtok(buffer, delim);
            newport = strtok(NULL, delim);
            //long newport = strtol( buffer, &endPtr, 8 ); 
            //newport = buffer+8; //je recupere le port pr la socket entre serv et client
            ///printf("%s\n", newport);
            //printf("%i\n", atoi(newport));
            mb_octet = sendto(sockfd, (const char*)messageACK, strlen(messageACK),
            0, (const struct sockaddr*)&servaddr,
            sizeof(servaddr));
            //printf("octet:%d\n", mb_octet);
            //printf("%s\n", newport);
            newservaddr.sin_port = htons(atoi(newport));
            
            while(1){
            n = recvfrom(sockfd, buffer, MAXLINE,0, (struct sockaddr*)&servaddr,&len);
            printf("message du new serveur : ");
            //printf("octet recus %d\n", n);
            if (strcmp(buffer,"let'sgo") == 0 ){
                puts(buffer);
                n= sendto(sockfd, (const char*)message, strlen(message),
                0, (const struct sockaddr*)&newservaddr,
                sizeof(newservaddr));
                //printf("octet envoyés %d\n", n);
            }
            //if (strcmp(buffer,"débutfichier") == 0 ){
                //puts(buffer);
                //printf("communication sur new socket finie avec succes!\n");
            //}

            // début reception fichier 

            printf("[STARTING] UDP File Server started. \n");
            
            write_file(sockfd, servaddr);

            printf("[SUCCESS] Data transfer complete.\n");
            
            }
            close(sockfd);

        }
        //FIN D'OUVERTURE DE CONNEXION
        
    }
    
    close(sockfd);
    return 0;

}


