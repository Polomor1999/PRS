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
#include <time.h>
#include <pthread.h>


#define BUFF_SIZE 1024
#define SEGMENT_LENGTH 1030

int nb_seg;
int last_ACK = 1; //dernier ack recu 
int last_SND = 0; //dernier segment envoyé
int timeout_flag = 0; //=1 si le timeout s'écoule et que ack non recu
double max_window = 1;
int ssthresh = 350;
int slowstart_flag = 0;
//int ffrts_flag = 0;
//int ffrts_ACK = 0;
//int ffrts_max = 2;

//double timeout_RTT = 10;
//double estimated_RTT = 0;
//double dev_RTT = 0;
//double RTT = 0;

struct timeval timeout_RTT_time;
pthread_mutex_t ackMutex = PTHREAD_MUTEX_INITIALIZER;//pb baguettes chinoises 

struct thread_args //threads pour continuer le code quand les timesout dorment 
{
	int socketDATA;
	long * pointerArray[];
};

void * timeout_THREAD(void* param){

}


void *thread_ack(int sockfd) {
	//recevoir buffer d_u client 
	// recuperer le numero
	//si numero > last ack on change last ack sinon on fait rien 
	char bufferACK[9];
	char numero_buff[6];
	int i;
	struct sockaddr_in cliaddr;
	int len = sizeof(cliaddr);
	char *ptr;
   	long numero_int;
	int tab[3]={0,-1,-2};

	while(1){
		recvfrom(sockfd,bufferACK,sizeof(bufferACK),0,(struct sockaddr*)&cliaddr, &len);
		puts(bufferACK);
		memcpy(numero_buff,bufferACK+3,6); //recuperer les numéros de séquence
   		numero_int = strtol(numero_buff, &ptr, 10); //conv str en int base 10
		if (numero_int > last_ACK){
			last_ACK = numero_int;
		}

		for (i = 0; i < 3; i++){
			//decaler indice
			tab[2] = tab[1];
			tab[1] = tab[0];
			tab[0] = numero_int;

			if (tab[i] ==  tab[i-1] & tab[i] == tab[i-2]){ //on recoit 3 fois le ack donc ca n'a pas été retransmit
				ACK_perdu_flag = tab[i]+1;
			}
		} 
			

	}

	//quand on recoit 3 fois le meme ack => on le renvoit 
	//slow start ici 

}


void transfert_data(int datasocket, struct sockaddr_in addr){

	char buff_DATA[BUFF_SIZE];
	memset((char*)&addr,0,sizeof(addr));
	int connection_flag = 1; //tant qu'on a pas recu le ackFIN 
	int Swindow = max_window;
	char bufferACK[9];
	
	while (connection_flag){
		int len = sizeof(addr);
		bzero(buff_DATA,sizeof(buff_DATA));
		int open_flag = 1; // passe à 0 si on a ouvert le fichier du client suivant (sinon means il y a encore des ack de celui d'avant )
		FILE *fileptr;
		while (open_flag){
			recvfrom(datasocket,buff_DATA,sizeof(buff_DATA),0,(struct sockaddr*)&addr, &len);
			fileptr = fopen(buff_DATA, "rb");
			bzero(buff_DATA,sizeof(buff_DATA));
			if (fileptr==NULL){
				printf("\n erreur sur l'ouverture du fichier");
			}
			else {
				open_flag = 0; 
			}
		}
		//CALCULER LE NOMBRE DE SEGMENT
		fseek(fileptr,0,SEEK_END); //déplace le pointeur vers la fin du fichier pr touver la taille apres 
		long file_len = ftell(fileptr); // Dit la taille en byte du offset par rapport au debut di ficher -> la taille du fichier dans ce cas
		rewind(fileptr); //remet au début du file ou fseek(fileptr,0,SEEK_STart)
		nb_seg = (file_len / (BUFF_SIZE-6))  + 1; //-6 car 6 attribué aux numéro de séquence 
		//pthread_t thread_ack_id;
		//pthread_create(thread_ack_id,NULL,thread_ack,datasocket); //lancer le thread pour écouter les ACK en parrallele d'envoyer les segments


		 
		//chercker si on est en slow start quand 


		int lendata;
		long compteur = 0;
		while(last_ACK < nb_seg){ //tant qu'on est pas à la fin 
			//gestion du time out 
			if (timeout_flag){
				//to do 
				//si timesout atteint on remet max_window = 1
				//seuil = maxwindows/2 (a changer si possible)
				
				timeout_flag = 0;
				}
		
			while (Swindow > 0 & last_SND < nb_seg){
				compteur++;
				bzero(buff_DATA,sizeof(buff_DATA));
				sprintf(buff_DATA, "%06d\n", compteur);
				fseek(fileptr,last_SND*(BUFF_SIZE-6),SEEK_SET); //se deplacer dans le file (seek_set = on part du début du fichier et on avance numéro seg * taille buff-6
				lendata=fread(buff_DATA+6, 1,BUFF_SIZE, fileptr);//ranger la data a position 6
				//printf("%d\n",lendata);
				sendto(datasocket, buff_DATA, lendata, 0, (struct sockaddr*)&addr, sizeof(addr));
				//sleep(1);
				//recvfrom(datasocket, bufferACK, sizeof(bufferACK)-1024, 0,(struct sockaddr*)&addr, &len);
				//puts(bufferACK);
				//recvfrom(datasocket, buff_DATA, sizeof(buff_DATA), 0,(struct sockaddr*)&addr, &len);
				//start timeoutthread
				last_SND ++;
				Swindow --;
			}
			bzero(buff_DATA,sizeof(buff_DATA));
			//int var_ACK;
			//pthread_mutex_lock(&ackMutex);
			//var_ACK = last_ACK;
			//pthread_mutex_unlock(&ackMutex);
			last_ACK ++;
			Swindow = last_ACK - (last_SND - max_window); // taille de data que tu peux encore envoyer dans ta fenetre de mla taille max_window
			if (Swindow < 0){
				Swindow = 0;
			} 	
			
			

		}
		//sleep(1);
		strcpy(buff_DATA, "FIN");
  		sendto(datasocket, buff_DATA, BUFF_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));

		fclose(fileptr);
		connection_flag=0;
	}			
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

	int listenfd, connfd, socketudp, nready, maxfdp1,datasocket,mb_octet;
	char buff_CON[BUFF_SIZE];
	bzero(buff_CON, BUFF_SIZE);
	fd_set rset;
	ssize_t n,b;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr,dataaddr;
	char* message = "SYN-ACK1222"; //nouveau port pour socket d'écoute avec le client 
	//void sig_chld(int);


	//listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	// binding server addr structure to listenfd
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	//listen(listenfd, 10);

	// create UDP socket 
	int optvaludp=1;
	socketudp = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(socketudp,SOL_SOCKET,SO_REUSEADDR,(const void *)&optvaludp,sizeof(int)); //eviter blocage du port
	// binding server addr structure to udp sockfd
	bind(socketudp, (struct sockaddr*)&servaddr, sizeof(servaddr));

	// clear the descriptor set
	FD_ZERO(&rset);

	//create new udp socket with new port
	bzero(&dataaddr, sizeof(dataaddr));
	dataaddr.sin_family = AF_INET;
	dataaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	dataaddr.sin_port = htons(1222);
	datasocket = socket(AF_INET, SOCK_DGRAM, 0);

	int optvaldata = 1;
	setsockopt(datasocket,SOL_SOCKET,SO_REUSEADDR,(const void *)&optvaldata,sizeof(int)); //eviter blocage du port
    // Bind the socket with the server address 
  	bind(datasocket, (struct sockaddr*)&dataaddr, sizeof(servaddr));



	for (;;) { //mettre la boucle for apres l'ouvertiure de connexion + rajouterfork pr gerer plusisuers client 

        //OUVERTURE DE CONNEXION
		//if udp socket is readable receive the message.
		len = sizeof(cliaddr);
		bzero(buff_CON, sizeof(buff_CON));
		printf("Message du Client sur socketudp: ");
		n = recvfrom(socketudp, buff_CON, sizeof(buff_CON), 0,
					(struct sockaddr*)&cliaddr, &len);

		if (strcmp(buff_CON,"SYN") ==0 ){
			puts(buff_CON);
			//puts(message);
			mb_octet = sendto(socketudp, (const char*)message, strlen(message), 0,
			(struct sockaddr*)&cliaddr, sizeof(cliaddr));
			//printf("octet:%d\n", mb_octet);

			
		}
		if (strcmp(buff_CON,"ACK") ==0 ){
			puts(buff_CON);
			printf("Fin d'ouverture de connexion \n");
			//pid_t pid = fork();
			//if (pid==0){
				close(socketudp);
				transfert_data(datasocket, cliaddr);
				exit(0);
			//}
		}
	}


}



/*
printf("\nMessage du Client sur datasocket: ");
			while(1){
				//ouverture nouvelle socket pr comm exclusivement avec le client 
				//bzero(buff_DATA, sizeof(buff_DATA));
				//printf("\nMessage from Client on newsocket: ");
				//b = recvfrom(datasocket, buff_DATA, sizeof(buff_DATA), 0,(struct sockaddr*)&cliaddr, &len);
				//puts(buff_DATA);
				//sendto(datasocket, (const char*)message2, strlen(message2), 0,(struct sockaddr*)&cliaddr, sizeof(cliaddr));


				// Sending the file data to the server
				int n,flag,lendata;
				//numéro de sequence 
				long compteur = 0;
				char buff_CON[BUFF_SIZE];
				bzero(buff_CON, BUFF_SIZE);
				char numero[SEGMENT_LENGTH];
				bzero(numero, SEGMENT_LENGTH);
				struct timespec start, finish, delta;
				flag = 1;
				// Sending the data

				//  FILE* f2 = fopen("kjh.pdf", "wb");

				while (1) //je lit tout d'un coup
				{
					lendata=fread(buff_CON, 1,BUFF_SIZE, fp);//taille que j'ai reussi a lire dans mon file
					flag = !(lendata<1024); //flag=0 si on atteint la fin du file

					// convertir int to char en respectant le nb ce charcatere 
					// mettre au début du buff_CON
					// remplir le buff_CON avec les datas
					//printf("Data: %s\n",buff_CON);
					compteur++;
					printf("%d\n",lendata);

					//long to char 
					sprintf(numero, "%06d\n", compteur);
					printf("numero : %s\n",numero);
					//add compteur to buff_CON
					memcpy(numero+6,buff_CON,lendata);

					//printf("[SENDING] Data: %s\n",buff_CON);
					n = sendto(datasocket, numero, lendata+6, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));

    				//clock_gettime(CLOCK_REALTIME, &start);
					if (n == -1)
					{
					perror("[ERROR] sending data to the server.");
					exit(1);
					}
					recvfrom(datasocket, buff_DATA, sizeof(buff_DATA), 0,(struct sockaddr*)&cliaddr, &len);
					//clock_gettime(CLOCK_REALTIME, &finish);
					puts(buff_DATA);

					bzero(buff_CON, BUFF_SIZE);
					bzero(numero, SEGMENT_LENGTH);
				}
				// fclose(f2);

				// Sending the 'FIN'
				//sleep(1);
				strcpy(buff_CON, "FIN");
				sendto(datasocket, buff_CON, BUFF_SIZE, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));

				fclose(fp);
				// Sending the file data to the server
				//send_file_data(fp, datasocket, cliaddr);
				printf("[SUCCESS] Data transfer complete.\n");
				
				close(datasocket);
    			//clock_gettime(CLOCK_REALTIME, &start);
    			//sleep(1);
    			//clock_gettime(CLOCK_REALTIME, &finish);
    			//sub_timespec(start, finish, &delta);
   				//printf("%d.%.9ld\n", (int)delta.tv_sec, delta.tv_nsec);



			} 
*/
