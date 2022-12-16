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
#include <semaphore.h>

#define SEM_NAME "/semaphore"
#define BUFF_SIZE 1500

/*-------------------------------------------------------------- */
/*------------------DECLARATION DE VARIABLE -------------------- */
/*-------------------------------------------------------------- */

pthread_mutex_t mutex;
sem_t *semaphore;

int nb_seg;
int Last_ACK_Updated = 0;
int last_SND = 0;
int ACK_perdu_flag = 0;
int nbfoiswindow;
int diff;
int window = 8;

struct timeval timeout_RTT_time;
struct thread_args
{
	int sockfd;
	FILE *fileptr;
	struct sockaddr_in addr;
};

/*-------------------------------------------------------------- */
/*-------------------------THREADS------------------------------ */
/*-------------------------------------------------------------- */

uint64_t time_now()
{
	struct timeval current;
	gettimeofday(&current, 0);
	return current.tv_sec * 1000000 + current.tv_usec;
}

void *thread_ack(void *param)
{

	// écoute et recoit les ack du client + renvoie les paquets perdus au client

	struct thread_args *p = (struct thread_args *)param;
	char bufferACK[10];
	char buff_DATAT[BUFF_SIZE];
	char numero_buff[7];
	int windowthread = 8;
	int lendata;
	int len = sizeof((*p).addr);
	char *ptr;
	long last_ACK;
	int tab[2] = {0, -1};

	fd_set desc;
	FD_ZERO(&desc);
	struct timeval timeout;

	while (Last_ACK_Updated < nb_seg)
	{

		int res = select((*p).sockfd + 1, &desc, NULL, NULL, &timeout);
		bzero(bufferACK, sizeof(bufferACK));
		bzero(numero_buff, sizeof(numero_buff));
		FD_SET((*p).sockfd, &desc);
		timeout.tv_sec = 0;
		timeout.tv_usec = 8000; // à faire varier

		// si le timeout n'est pas atteint ou on a recu l'ack
		if (res > 0)
		{

			recvfrom((*p).sockfd, bufferACK, sizeof(bufferACK), 0, (struct sockaddr *)&(*p).addr, &len);
			memcpy(numero_buff, bufferACK + 3, 6);

			last_ACK = atoi(numero_buff); // convertit str en int base 10
			if (last_ACK > Last_ACK_Updated)
			{
				// diff = last_ACK - Last_ACK_Updated;
				Last_ACK_Updated = last_ACK;
				// sem_post(semaphore); //on libere la sémaphore
			}

			if (Last_ACK_Updated == windowthread)
			{
				// printf("\non libere la sema pour l ACK n° = %d ", Last_ACK_Updated);
				sem_post(semaphore);
				windowthread += 8;
			}

			// printf("\nlast ack update = %d", Last_ACK_Updated);

			if (last_ACK == Last_ACK_Updated)
			{
				tab[1] = tab[0];
				tab[0] = last_ACK;
			}

			if (tab[1] == tab[0])
			{

				ACK_perdu_flag = tab[0] + 1;
				bzero(buff_DATAT, sizeof(buff_DATAT));
				sprintf(buff_DATAT, "%06d\n", ACK_perdu_flag);

				pthread_mutex_lock(&mutex);
				fseek((*p).fileptr, (ACK_perdu_flag - 1) * (BUFF_SIZE - 6), SEEK_SET);
				lendata = fread(buff_DATAT + 6, 1, BUFF_SIZE - 6, (*p).fileptr); // ranger la data a position 6
				pthread_mutex_unlock(&mutex);

				sendto((*p).sockfd, buff_DATAT, lendata + 6, 0, (struct sockaddr *)&(*p).addr, sizeof((*p).addr));

				tab[1] = -2;
				tab[0] = -1;
			}
		}
		// gerer timeout donc retransmission du last ack si on a atteint la fin du timeout

		else
		{

			bzero(buff_DATAT, sizeof(buff_DATAT));
			sprintf(buff_DATAT, "%06d\n", Last_ACK_Updated);

			pthread_mutex_lock(&mutex);
			fseek((*p).fileptr, (Last_ACK_Updated - 1) * (BUFF_SIZE - 6), SEEK_SET);
			lendata = fread(buff_DATAT + 6, 1, BUFF_SIZE - 6, (*p).fileptr);
			pthread_mutex_unlock(&mutex);

			int n = sendto((*p).sockfd, buff_DATAT, lendata + 6, 0, (struct sockaddr *)&(*p).addr, sizeof((*p).addr));
			// printf("\nrenvoyé le n° prcq time out atteint %d", Last_ACK_Updated);
		}
	}
}

// envoie le fichier
void transfert_data(int datasocket, struct sockaddr_in addr)
{

	int connection_flag = 1; // tant qu'on a pas recu le ackFIN
	int compteur2 = 0;
	int n;
	char buff_DATA[BUFF_SIZE];

	memset((char *)&addr, 0, sizeof(addr));
	uint64_t startTime = time_now();
	semaphore = sem_open("/semaphore", O_CREAT, 0600, 1); // semaphore = un pointeur vers la semaphore ouverte

	if (semaphore == SEM_FAILED)
	{
		printf("\nERROR CREATION SEMAPHORE");
		exit(1);
	}

	pthread_mutex_init(&mutex, NULL);

	while (connection_flag)
	{

		int len = sizeof(addr);
		int open_flag = 1; // passe à 0 si on a ouvert le fichier du client suivant (sinon means il y a encore des ack de celui d'avant )
		bzero(buff_DATA, sizeof(buff_DATA));
		FILE *fileptr;

		while (open_flag)
		{

			recvfrom(datasocket, buff_DATA, sizeof(buff_DATA), 0, (struct sockaddr *)&addr, &len);
			fileptr = fopen(buff_DATA, "rb");
			bzero(buff_DATA, sizeof(buff_DATA));

			if (fileptr == NULL)
			{
				printf("\nerreur sur l'ouverture du fichier");
			}
			else
			{
				open_flag = 0;
			}
		}

		// CALCULER LE NOMBRE DE SEGMENT
		fseek(fileptr, 0, SEEK_END);			   // déplace le pointeur vers la fin du fichier pr touver la taille apres
		long file_len = ftell(fileptr);			   // Dit la taille en byte du offset par rapport au debut di ficher -> la taille du fichier dans ce cas
		rewind(fileptr);						   // remet au début du file ou fseek(fileptr,0,SEEK_STart)
		nb_seg = (file_len / (BUFF_SIZE - 6)) + 1; //-6 car 6 attribué aux numéro de séquence
		printf("nbr segment :\n %d", nb_seg);
		nbfoiswindow = nb_seg / 8;

		struct thread_args *param = malloc(sizeof(struct thread_args));
		param->fileptr = fileptr;
		param->addr = addr;
		param->sockfd = datasocket;

		pthread_t thread_ack_id;
		pthread_create(&thread_ack_id, NULL, thread_ack, (void *)param); // lancer le thread pour écouter les ACK en parrallele d'envoyer les segments

		int lendata;
		int compteurwindow = 0;
		long compteur = 0;

		while (Last_ACK_Updated < nb_seg)
		{

			while (compteurwindow != nbfoiswindow + 1)
			{

				// nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
				if (last_SND == window)
				{
					window += 8;
					compteur2++;
					// printf("\nsema bloquee au n° %d", last_SND);
					sem_wait(semaphore);
				}

				while (last_SND < window && last_SND < nb_seg)
				{
					compteur++;
					bzero(buff_DATA, sizeof(buff_DATA));
					sprintf(buff_DATA, "%06d\n", compteur);

					pthread_mutex_lock(&mutex);
					fseek(fileptr, last_SND * (BUFF_SIZE - 6), SEEK_SET);	   // se deplacer dans le file (seek_set = on part du début du fichier et on avance numéro seg * taille buff-6
					lendata = fread(buff_DATA + 6, 1, BUFF_SIZE - 6, fileptr); // ranger la data a position 6
					pthread_mutex_unlock(&mutex);

					int n = sendto(datasocket, buff_DATA, lendata + 6, 0, (struct sockaddr *)&addr, sizeof(addr));
					if (n == -1)
					{
						perror("[ERROR] sending data to the client.");
					}
					bzero(buff_DATA, sizeof(buff_DATA));
					last_SND++;
					if (last_SND == nb_seg)
					{
					}
				}

				compteurwindow++;
				bzero(buff_DATA, sizeof(buff_DATA));
			}
		}

		strcpy(buff_DATA, "FIN");
		sendto(datasocket, buff_DATA, BUFF_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
		pthread_join(thread_ack_id, NULL);
		uint64_t endtime = time_now();
		double timeTaken = (endtime - startTime) / 1000000.0;
		printf("temps de transmission: %f s\n", timeTaken);
		double debit = ((file_len - 6.0) * 0.000001) / timeTaken;
		printf("taille du fichier %d \n", file_len);
		printf("débit: %f MO/s \n", debit);
		sem_destroy(semaphore);
		fclose(fileptr);
		pthread_mutex_destroy(&mutex);
		connection_flag = 0;
		//|grep drop |wc
		//|grep timeout |wc
	}
}

/*-------------------------------------------------------------- */
/*---------------------------MAIN------------------------------- */
/*-------------------------------------------------------------- */

int main(int argc, char *argv[])
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

	int listenfd, connfd, socketudp, nready, maxfdp1, datasocket, mb_octet;
	char buff_CON[BUFF_SIZE];
	bzero(buff_CON, BUFF_SIZE);
	fd_set rset;
	ssize_t n, b;
	socklen_t len;
	struct sockaddr_in cliaddr, servaddr, dataaddr;
	char *message = "SYN-ACK1222"; // nouveau port pour socket d'écoute avec le client

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	// binding server addr structure to listenfd
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	/* create UDP socket */

	socketudp = socket(AF_INET, SOCK_DGRAM, 0);
	// binding server addr structure to udp sockfd
	bind(socketudp, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// clear the descriptor set
	FD_ZERO(&rset);

	// create new udp socket with new port
	bzero(&dataaddr, sizeof(dataaddr));
	dataaddr.sin_family = AF_INET;
	dataaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	dataaddr.sin_port = htons(1222);
	datasocket = socket(AF_INET, SOCK_DGRAM, 0);

	// Bind the socket with the server address
	bind(datasocket, (struct sockaddr *)&dataaddr, sizeof(servaddr));

	for (;;)
	{ // mettre la boucle for apres l'ouvertiure de connexion + rajouterfork pr gerer plusisuers client

		// OUVERTURE DE CONNEXION
		len = sizeof(cliaddr);
		bzero(buff_CON, sizeof(buff_CON));
		printf("\nMessage du Client sur socketudp: ");
		n = recvfrom(socketudp, buff_CON, sizeof(buff_CON), 0,
					 (struct sockaddr *)&cliaddr, &len);

		if (strcmp(buff_CON, "SYN") == 0)
		{
			puts(buff_CON);
			mb_octet = sendto(socketudp, (const char *)message, strlen(message), 0,
							  (struct sockaddr *)&cliaddr, sizeof(cliaddr));
		}

		if (strcmp(buff_CON, "ACK") == 0)
		{
			puts(buff_CON);
			printf("\nFin d'ouverture de connexion ");
			// pid_t pid = fork();

			// if (pid == 0){
			close(socketudp);
			transfert_data(datasocket, cliaddr);
			exit(0);
			//}
		}
	}
}
