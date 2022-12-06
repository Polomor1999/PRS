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

#define MAXBUFLEN 1024
#define BUFF_SIZE 1030
#define SEGMENT_LENGTH 1030
#define FRAMES 60000
#define ACK 3


int nb_seg;
int64_t timeOut, estimatedRTT = 1000, deviation = 1, difference = 0;
int last_ACK = 1; //dernier ack recu 
int last_SND = 0; //dernier segment envoyé
int timeout_flag = 0; //=1 si le timeout s'écoule et que ack non recu
int max_window = 1;
int ssthresh = 350;
int slowstart_flag = 1;
int ACK_perdu_flag = 0;
int LFS = -1; // last frame sent
uint8_t ACKed[FRAMES];
uint8_t sent[FRAMES];
//int ffrts_flag = 0;
//int ffrts_ACK = 0;
//int ffrts_max = 2;

//double timeout_RTT = 10;
//double estimated_RTT = 0;
//double dev_RTT = 0;
//double RTT = 0;

struct timeval timeout_RTT_time;
pthread_mutex_t ackMutex = PTHREAD_MUTEX_INITIALIZER;//pb baguettes chinoises 


typedef struct {   
	uint64_t sent_time; //sent time in microseconds for RTT calculations   
	uint16_t code;
	uint16_t seq_no; //sequence number for sender and expected sequence number for receiver
}TCP_hearder;

struct thread_args //threads pour continuer le code quand les timesout dorment 
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

void update_timeout(uint64_t sentTime)
{
	uint64_t sampleRTT = time_now() - sentTime;
	estimatedRTT = 0.875 * estimatedRTT + 0.125 * sampleRTT; // alpha = 0.875
	deviation += (0.25 * ( abs(sampleRTT - estimatedRTT) - deviation)); //delta = 0.25
	timeOut = (estimatedRTT + 4 * deviation); // mu = 1, phi = 4
	timeOut = timeOut/5;
}
uint16_t win_size()
{
	return 50 * timeOut /MAXBUFLEN;  // (100/8) MBps * timeOut (usec) / MAXBUFLEN 
}
void *timeout_THREAD(void* param){
	//lancer le time out quand on recoit un ack$
	//le fermer quand on recoit le bon ack
}
int set_timeout(int sockfd, int usec)
{
	if (usec < 0)
		return -1;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10; 
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    	perror("sender: setsockopt");
	}
	return 0;
}


void *thread_ack(void *param){
	//recevoir buffer d_u client 
	//recuperer le numero
	//si numero > last ack on change last ack sinon on fait rien 
	struct thread_args *p = (struct thread_args*)param;
	char bufferACK[9];
	char numero_buff[6];
	int i, b;
	int lendata;
	int len = sizeof((*p).addr);
	char *ptr;
   	long numero_int;
	int tab[3]={0,-1,-2};

	while(1){
		recvfrom((*p).sockfd,bufferACK,sizeof(bufferACK),0,(struct sockaddr*)&(*p).addr, &len);
		puts(bufferACK);
		b ++;
		printf("%d\n",b);
		memcpy(numero_buff,bufferACK+3,6); //recuperer les numéros de séquence
   		numero_int = atoi(numero_buff); //conv str en int base 10
		if (numero_int > last_ACK){
			last_ACK = numero_int;
		}

		for (i = 0; i < 3; i++){
			//decaler indice
			tab[2] = tab[1];
			tab[1] = tab[0];
			tab[0] = numero_int;

			if (tab[i] ==  tab[i-1] & tab[i] == tab[i-2]){ //on recoit 3 fois le ack donc ca n'a pas été retransmit
				//ACK_perdu_flag = tab[i]+1;
				bzero((*p).buff_DATA,sizeof((*p).buff_DATA));
				fseek((*p).fileptr,ACK_perdu_flag*(BUFF_SIZE-6),SEEK_SET); //se place au niveau du segment manquant 
				lendata=fread((*p).buff_DATA+6, 1,BUFF_SIZE, (*p).fileptr);//ranger la data a position 6
				sendto((*p).sockfd, (*p).buff_DATA, lendata+6, 0, (struct sockaddr*)&(*p).addr, sizeof((*p).addr));
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
	int Swindow = win_size();
	char bufferACK[9];
	uint16_t seq, length;
	uint64_t startTime = time_now(); 
	uint64_t sentTime[FRAMES] = {0};
	uint64_t currTime = 0;
	int last_LFS = -1;
	int bloquant;
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
				printf(" erreur sur l'ouverture du fichier\n");
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
		int final_seg_size = file_len % (BUFF_SIZE-6);
		//pthread_t thread_ack_id;
		//pthread_create(thread_ack_id,NULL,thread_ack,datasocket); //lancer le thread pour écouter les ACK en parrallele d'envoyer les segments


		 
		//chercker si on est en slow start quand 
		TCP_hearder my_header, their_header;
		int TCP_size = (int) sizeof(my_header);
		int double_sent = 0;
		int lendata,numbytes;
		int flag=2;
		long compteur = 0;
		while(last_ACK < nb_seg){ //tant qu'on est pas à la fin 
			//gestion du time out 
			/*if (slowstart_flag < 30){
				//to do 
				//si timesout atteint on remet max_window = 1
				//seuil = maxwindows/2 (a changer si possible)
				Swindow = (Swindow)/flag;
				slowstart_flag++;
				flag = 3;
				}
			for (int i = 0; i < Swindow; i++)
		{
			seq = last_ACK+1+i;
			currTime = time_now();
			if (currTime - sentTime[seq] < timeOut * 5)
				continue;
			if(ACKed[seq] == 0 && seq < nb_seg)
			{
				if (sent[seq] == 1)
					double_sent++;
				sent[seq] = 1;
				my_header.seq_no = seq;
				my_header.sent_time = currTime;
				memcpy(buff_DATA, &my_header, TCP_size);
				if (SEEK_CUR != seq * (BUFF_SIZE-6))
					fseek(fileptr, (BUFF_SIZE-6) * seq, SEEK_SET);
				if (seq == nb_seg-1 && final_seg_size > 0)
					length = final_seg_size;	
				else
					length = sizeof(buff_DATA-6);
				fread(buff_DATA + TCP_size, 1, length, fileptr);
				if (numbytes = sendto(datasocket, buff_DATA, length + TCP_size, 0,
			 		(struct sockaddr*)&addr, sizeof(addr)) == -1) 
				{	
					perror("sender: sendto");
					exit(1);
				}
				if (last_SND < seq)
					last_SND = seq;						
			}
			if (last_LFS == last_SND)
				set_timeout(datasocket, timeOut * 2);
			last_LFS = last_SND;

		}*/
		/*while(1)
		{
			numbytes = recvfrom(datasocket, bufferACK, sizeof(bufferACK)-1024, 0,(struct sockaddr*)&addr, &len);
			if(numbytes == TCP_size)
			{
				memcpy(&their_header, bufferACK, TCP_size);
				if (their_header.code == ACK)
				ACKed[their_header.seq_no] = 1;
				update_timeout(their_header.sent_time);			
			}
			else 
				break;	
		}
		set_timeout(datasocket, timeOut);
		int i = last_ACK;
		i++;
		while(ACKed[i])
			i++;	
		last_ACK = i-1;
		Swindow =win_size();
		if (Swindow > 1000)
			Swindow = 500;*/
		
			while (Swindow > 0 & last_SND < nb_seg){
				
				compteur++;
				bzero(buff_DATA,sizeof(buff_DATA));
				sprintf(buff_DATA, "%06d\n", compteur);
				fseek(fileptr,last_SND*(BUFF_SIZE-6),SEEK_SET); //se deplacer dans le file (seek_set = on part du début du fichier et on avance numéro seg * taille buff-6
				lendata=fread(buff_DATA+6, 1,BUFF_SIZE, fileptr);//ranger la data a position 6
				//printf("%d\n",lendata);
				sendto(datasocket, buff_DATA, lendata, 0, (struct sockaddr*)&addr, sizeof(addr));
				//sleep(1);
				set_timeout(datasocket, 1000);
				bloquant = recvfrom(datasocket, bufferACK, sizeof(bufferACK)-1024, 0,(struct sockaddr*)&addr, &len);
				printf("%d\n",bloquant);

				if (bloquant == -1) {
					sendto(datasocket, buff_DATA, lendata, 0, (struct sockaddr*)&addr, sizeof(addr));
    // aucun octet recu donc boucler a nouveau , c la que je pense que la boucle passerait souvent
					}
				if (bloquant > 0) {
				last_SND ++;
				Swindow --;
			// octets recus ... faire le traitement
					}
				
				puts(bufferACK);
				//start timeoutthread
			}
			if (bloquant > 0) {
			bzero(buff_DATA,sizeof(buff_DATA));
			//int var_ACK;
			//pthread_mutex_lock(&ackMutex);
			//var_ACK = last_ACK;
			//pthread_mutex_unlock(&ackMutex);
			last_ACK ++;
			Swindow = last_ACK - (last_SND - max_window); // taille de data que tu peux encore envoyer dans ta fenetre de mla taille max_window
			if (Swindow < 0){
				Swindow = 0;}
			} 	
			
			

		}
		//sleep(1);
		strcpy(buff_DATA, "FIN");
  		sendto(datasocket, buff_DATA, BUFF_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
		uint64_t endtime = time_now();
		double timeTaken = (endtime - startTime)/1000000.0;
		printf("temps de transmission: %f s\n",timeTaken);
		double debit=((file_len-6.0)*0.000001)/timeTaken;
		printf("taille du fichier %d \n",file_len-6);
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
		printf("Message du Client sur socketudp: \n");
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

			//if (pid == 0){
				close(socketudp);
				printf("cest chaud la");
				transfert_data(datasocket,cliaddr);
				exit(0);
			//}
		}
		
		
	}


}

//to do 
//sliding windows pour envoyer que a max 94 pr la fleur 
//RTT
//fast retransmit 

