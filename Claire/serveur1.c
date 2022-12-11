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
#include <sys/time.h>
#include <pthread.h>



#define BUFF_SIZE 1500


int nb_seg;
int last_ACK = 0; //dernier ack recu 
int last_SND = 0; //dernier segment envoyé
int timeout_flag = 0; //=1 si le timeout s'écoule et que ack non recu
int max_window = 1;
int ssthresh = 350;
int slowstart_flag = 0;
int ACK_perdu_flag = 0;
int window;
int flag_fin = 0;


struct timeval timeout_RTT_time;




struct thread_args //structure pour les arguments du thread
{
	int sockfd;
	char* buff_DATA;
	FILE *fileptr;
	struct sockaddr_in addr;
};

uint64_t time_now()
{
	struct timeval current;
	gettimeofday(&current, 0);
	return current.tv_sec * 1000000 + current.tv_usec;
}


void *thread_ack(void *param){
	//recevoir buffer d_u client 
	//recuperer le numero
	//si numero > last ack on change last ack sinon on fait rien 
	struct thread_args *p = (struct thread_args*)param;
	char bufferACK[9];
	char numero_buff[6];
	int i;
	int lendata;
	int len = sizeof((*p).addr);
	char *ptr;
   	long numero_int;
	//int tab[3]={0,-1,-2};
	int tab[2]={0,-1};
	int compteur2=0;


	while(last_ACK<nb_seg){
		//printf("nbr de threads : %d\n", compteur);
		printf("\n%d",last_ACK);

		if (last_ACK == nb_seg){
			//flag_fin = 1;
			pthread_exit(NULL);
			printf("\nlast ack envoyé gfgff g");
		}

		recvfrom((*p).sockfd,bufferACK,sizeof(bufferACK),0,(struct sockaddr*)&(*p).addr, &len);
		//puts(bufferACK);
		memcpy(numero_buff,bufferACK+3,6); //recuperer les numéros de séquence
   		numero_int = atoi(numero_buff); //conv str en int base 10
		if (numero_int > last_ACK){
			last_ACK = numero_int;
		}

		
		//tab[2] = tab[1];
		tab[1] = tab[0];
		tab[0] = numero_int;
		
		if(tab[1] == tab[0]){
			//printf("bug");
			ACK_perdu_flag = tab[0]+1;
			//printf("%d",ACK_perdu_flag);
			bzero((*p).buff_DATA,sizeof((*p).buff_DATA));
			sprintf((*p).buff_DATA, "%06d\n", ACK_perdu_flag);
			fseek((*p).fileptr,ACK_perdu_flag*(BUFF_SIZE-6),SEEK_SET);
			lendata=fread((*p).buff_DATA+6, 1,BUFF_SIZE, (*p).fileptr);//ranger la data a position 6
			int n = sendto((*p).sockfd, (*p).buff_DATA, lendata, 0, (struct sockaddr*)&(*p).addr, sizeof((*p).addr));
			printf("\nsendto result :  %d",n);
			printf("\nsegment renvoyé n°, %d", ACK_perdu_flag);

			//tab[2] = -3;
			tab[1] = -2;
			tab[0] = -1;		
		}	
	}
}

void transfert_data(int datasocket, struct sockaddr_in addr){

	char buff_DATA[BUFF_SIZE];
	memset((char*)&addr,0,sizeof(addr));
	int connection_flag = 1; //tant qu'on a pas recu le ackFIN 
	int Swindow = max_window;
	uint64_t startTime = time_now(); 
	
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
		printf("nbr segment :\n %d",nb_seg);
		window = nb_seg/50;
	
		struct thread_args param;
			param.fileptr = fileptr;
			param.addr = addr;
			param.buff_DATA = buff_DATA;
			param.sockfd = datasocket;
				
			pthread_t thread_ack_id;
			pthread_create(&thread_ack_id,NULL,thread_ack,&param); //lancer le thread pour écouter les ACK en parrallele d'envoyer les segments

		int lendata;
		int compteurwindow = 0;
		int flagwindow = 50;
		long compteur = 0;

		while(last_ACK < nb_seg){
			//tant qu'on est pas à la fin 
			while (compteurwindow != window+1){ //Swindow > 0 & //last_SND < nb_seg

				sleep(1);
				while (last_SND < flagwindow && last_SND < nb_seg)
				{
					
				compteur++;
				bzero(buff_DATA,sizeof(buff_DATA));
				sprintf(buff_DATA, "%06d\n", compteur);
				fseek(fileptr,last_SND*(BUFF_SIZE-6),SEEK_SET); //se deplacer dans le file (seek_set = on part du début du fichier et on avance numéro seg * taille buff-6
				lendata=fread(buff_DATA+6, 1,BUFF_SIZE, fileptr);//ranger la data a position 6
				int n = sendto(datasocket, buff_DATA, lendata, 0, (struct sockaddr*)&addr, sizeof(addr));

				if (n == -1){
					perror("[ERROR] sending data to the client.");
				}
				bzero(buff_DATA,sizeof(buff_DATA));

				last_SND ++;     
				}
				compteurwindow ++;
				flagwindow += 50;
				bzero(buff_DATA,sizeof(buff_DATA));

			}
		}
		pthread_join(thread_ack_id,NULL);
		strcpy(buff_DATA, "FIN");
  		sendto(datasocket, buff_DATA, BUFF_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
		uint64_t endtime = time_now();
		double timeTaken = (endtime - startTime)/1000000.0;
		printf("temps de transmission: %f s\n",timeTaken);
		double debit=((file_len-6.0)*0.000001)/timeTaken;
		printf("taille du fichier %d \n",file_len);
		printf("débit: %f MO/s \n",debit);
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

	/* create UDP socket */
	//int optvaludp=1;
	socketudp = socket(AF_INET, SOCK_DGRAM, 0);
	//setsockopt(socketudp,SOL_SOCKET,SO_REUSEADDR,(const void *)&optvaludp,sizeof(int)); //eviter blocage du port
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

	//int optvaldata = 1;
	//setsockopt(datasocket,SOL_SOCKET,SO_REUSEADDR,(const void *)&optvaldata,sizeof(int)); //eviter blocage du port
    // Bind the socket with the server address 
  	bind(datasocket, (struct sockaddr*)&dataaddr, sizeof(servaddr));





	for (;;) { //mettre la boucle for apres l'ouvertiure de connexion + rajouterfork pr gerer plusisuers client 

        //OUVERTURE DE CONNEXION
		// if udp socket is readable receive the message.
		len = sizeof(cliaddr);
		bzero(buff_CON, sizeof(buff_CON));
		printf("\nMessage du Client sur socketudp: ");
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
			printf("\nFin d'ouverture de connexion ");
			//pid_t pid = fork();

			//if (pid == 0){
				close(socketudp);
				transfert_data(datasocket,cliaddr);
				exit(0);
			//}
		}
		
		
	}


}
