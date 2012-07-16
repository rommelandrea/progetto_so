#include "header_proj.h"
#include "myheader.h"

#include <fnmatch.h>

int numTurno = 0;

/**
 * funzione che decrementa il semaforo
 */
void down(int semid, int semnum){
	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	semop(semid, &sb, 1);
}

/**
 * funzione che incrementa il semaforo
 */
void up(int semid, int semnum){
	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = 1;
	sb.sem_flg = 0;
	semop(semid, &sb, 1);
}

/**
 * leggo il file di configurazione e scrivo i dati nel puntatore
 * alla struttura
 */
void leggi_conf(configurazione *c){
	FILE *fp = fopen("config.conf", "r");
	char line[265];
	char * pline;
	int price;

	while (!feof(fp)) {
		//fscanf(fp, "%s", line);

		fgets(line, 256, fp);
		if (fnmatch("#*", line, 0)) {
			printf("%s", line);

			if (!fnmatch("VISITA_RADIOLOGICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_radiologica = price;
				printf("il prezzo e' %d\n", price);
			}

			if (!fnmatch("VISITA_ORTOPEDICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_ortopedica = price;
				printf("il prezzo e' %d\n", price);
			}

			if (!fnmatch("VISITA_OCULISTICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_oculistica = price;
				printf("il prezzo e' %d\n", price);
			}
			if (!fnmatch("COSTO_PRIORITA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->costo_priorita = price;
				printf("il prezzo e' %d\n", price);
			}
			if (!fnmatch("PRIORITY*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->priorita_default = price;
				printf("il prezzo e' %d\n", price);
			}
		}
	}

	fclose(fp);
}

/**
 * creo socket e invio i dati al client
 */
void send_socket(char * s, int p) {
	sleep(3);
	printf("\npronto per inviare\n");
	int sd, n;
	struct sockaddr_un srvaddr;

	char sock[20];
	sprintf(sock, "%d.sock", p);
	printf("\nSOCKET %s\n", sock);

	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sd < 0){
		perror("Unable to create client socket");
		exit(1);
	}

	memset(&srvaddr, 0, sizeof(srvaddr));

	srvaddr.sun_family = AF_UNIX;
	strcpy(srvaddr.sun_path, sock);

	//n = connect(sd, (struct sockaddr *) &srvaddr, SUN_LEN(&srvaddr));
	n = connect(sd, (struct sockaddr *) &srvaddr, sizeof(srvaddr));
	if(n < 0){
		perror("Unable to connect to socket server");
		exit(1);
	}

	char buf[100];
	int costo = 60;
	char cosa[] = "visita ortopedica";
	sprintf(buf, "mi devi %d perche' hai prenotato questo: %s", costo, cosa);

	//strcpy(buf, "dati inviati");
	n = write(sd, buf, sizeof(buf));

	close(sd);
	unlink(sock);
}


int main(int argc, char **argv) {
	/**
	 * creo le variabili
	 */
	int MSG_Q__main_bus, MSG_Q__oculistica, MSG_Q__radiologia, MSG_Q__ortopedia,
			SEM_server;
	request *richiesta;
	richiesta = malloc(sizeof(request));

	int son;

	/**
	 * creo le code di messaggi
	 * e controllo che la creazione sia andata a buon fine
	 */
	MSG_Q__main_bus = msgget(SERVER_KEY, IPC_CREAT | 0666);
	MSG_Q__oculistica = msgget(OPH_queue_KEY, IPC_CREAT | 0666);
	MSG_Q__radiologia = msgget(RAD_queue_KEY, IPC_CREAT | 0666);
	MSG_Q__ortopedia = msgget(ORT_queue_KEY, IPC_CREAT | 0666);

	if (MSG_Q__main_bus < 0 && MSG_Q__oculistica < 0 && MSG_Q__radiologia < 0
			&& MSG_Q__ortopedia < 0) {
		perror("Cannot create message queue");
		exit(1);
	}

	/**
	 * creo il semaforo e lo inizializzo a 1
	 */
	SEM_server = semget(SEM_KEY, 1, IPC_CREAT);
	if (SEM_server < 0) {
		perror("Cannot create semaphore");
		exit(1);
	}

	semctl(SEM_server, 0, SETVAL, 1);

	/**
	 * creo la struttura con immagazzinati
	 * i dati di configurazione
	 * chiamo la funzione che legge il file di conf e riempie la struttura
	 */
	configurazione *conf;
	conf = malloc(sizeof(configurazione));
	leggi_conf(conf);

	/**
	 * Ciclo infinito che accetta le richieste,
	 * genera i figli
	 */
//	while (TRUE) {
		printf("aspetto un msg sulla coda\n");
		msgrcv(MSG_Q__main_bus, richiesta, sizeof(request), TOSRV, 0);
		printf("ricevuto messaggio\n");

		son = fork();
		if (son == 0) {
			int costo = 0;
			int reparto = richiesta->kindof_service;
			int priorita = richiesta->priority;
			int pid_cli = richiesta->clientId;
			char *bill = "ricevuta";

			printf("sono il figlio\n");

			down(SEM_server, 0);
			numTurno++;
			up(SEM_server, 0);

			switch(reparto){
			case 0:
				printf("caso oculistica");
				costo = conf->visita_oculistica + (priorita * conf->costo_priorita);
				send_socket(bill, pid_cli);
				break;
			case 1:
				printf("caso ortopedia");
				break;
			case 2:
				printf("caso radiologia");
				break;
			}
		}
		printf("pid client: %d reparto: %d\n", richiesta->clientId,
				richiesta->kindof_service);
		printf("num turno %d\n", numTurno);

//	}
	wait(0);

	/**
	 * chiudo le code di messaggi
	 */
	msgctl(MSG_Q__main_bus, IPC_RMID, 0);
	msgctl(MSG_Q__oculistica, IPC_RMID, 0);
	msgctl(MSG_Q__radiologia, IPC_RMID, 0);
	msgctl(MSG_Q__ortopedia, IPC_RMID, 0);

	semctl(SEM_server, 0, IPC_RMID);

	/**
	 * libero tutte le memorie
	 */
	free(richiesta);
	free(conf);

	return 0;
}
