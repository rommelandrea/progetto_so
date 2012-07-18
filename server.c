#include "header_proj.h"
#include "myheader.h"

/**
 * funzione che decrementa il semaforo
 */
void down(int semid, int semnum) {
	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	semop(semid, &sb, 1);
}

/**
 * funzione che incrementa il semaforo
 */
void up(int semid, int semnum) {
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
void leggi_conf(configurazione *c) {
	FILE *fp = fopen("config.conf", "r");
	char line[265];
	char * pline;
	int price;

	while (!feof(fp)) {
		//fscanf(fp, "%s", line);

		fgets(line, 256, fp);
		if (fnmatch("#*", line, 0)) {
			//printf("%s", line);

			if (!fnmatch("VISITA_RADIOLOGICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_radiologica = price;
				//printf("il prezzo e' %d\n", price);
			}

			if (!fnmatch("VISITA_ORTOPEDICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_ortopedica = price;
				//printf("il prezzo e' %d\n", price);
			}

			if (!fnmatch("VISITA_OCULISTICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_oculistica = price;
				//printf("il prezzo e' %d\n", price);
			}
			if (!fnmatch("COSTO_PRIORITA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->costo_priorita = price;
				//printf("il prezzo e' %d\n", price);
			}
			if (!fnmatch("PRIORITY*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->priorita_default = price;
				//printf("il prezzo e' %d\n", price);
			}
		}
	}

	fclose(fp);
}

/**
 * creo socket e invio i dati al client
 * nel caso in cui la connect non vada a buon fine ritento la connessione ogni secondo
 */
void send_socket(char * s, int p) {

	/**
	 * sleep temporanea per la sincronizzazione dei processi
	 */
	sleep(3);

	int sd, n;
	struct sockaddr_un srvaddr;

	char sock[20];
	sprintf(sock, "/tmp/%d.sock", p);

	printf("\npronto per inviare al socket %s\n\n", sock);

	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0) {
		perror("Unable to create client socket");
		exit(1);
	}

	/**
	 * memset pulisce srvaddr
	 */
	memset(&srvaddr, 0, sizeof(srvaddr));

	srvaddr.sun_family = AF_UNIX;
	strcpy(srvaddr.sun_path, sock);

	/*
	 * tentativo d'inserimento del timeout sul socket
	 if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout,
	 sizeof(struct timeval)) < 0) {
	 perror("Unable to set socket");
	 exit(1);
	 }
	 */

	printf("CONNECT....\n");

	n = 10;
	while (connect(sd, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0
			&& n > 0) {
		sleep(1);
		printf("sleep ... ");
		n--;
	}

	if (n == 0) {
		perror("Unable to connect to socket server");
		exit(1);
	}

	n = write(sd, s, sizeof(char) * 200);

	close(sd);
	unlink(sock);
}

/**
 * la funzione calcola il prezzo della prestazione
 * in base al costo base, alla priorità e al costo sulla priorita'
 */
int calcola_prezzo(int base, int priorita, int costo_p) {
	int totale;

	totale = base + (costo_p * priorita);

	return totale;
}

/**
 * la funzione calcola il numero del turno in base alla priorita'
 * nel caso il turno venga minore di 1 viene messo = a 1
 */
int calcola_turno(int s, int p) {
	int t = 0;
	t = s - p;
	if (t < 1)
		t = 1;

	return t;
}

void clean(int q1, int q2, int q3, int q4, int s, int m) {
	/**
	 * chiudo le code di messaggi
	 */
	msgctl(q1, IPC_RMID, 0);
	msgctl(q2, IPC_RMID, 0);
	msgctl(q3, IPC_RMID, 0);
	msgctl(q4, IPC_RMID, 0);

	/**
	 * disalloco il semaforo
	 */
	semctl(s, 0, IPC_RMID);

	/**
	 * disalloco la memoria condivisa
	 */
	shmctl(m, IPC_RMID, 0);
}

int main(int argc, char **argv) {
	/**
	 * creo le variabili
	 */
	int MSG_Q__main_bus, MSG_Q__oculistica, MSG_Q__radiologia, MSG_Q__ortopedia,
			SEM_server;
	request *richiesta = malloc(sizeof(request));

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
	 * prelevo un segmento di memoria condivisa
	 * e lo inizializzo a 0
	 */

	key_t shm_id;
	shm_id = shmget(IPC_PRIVATE, sizeof(int), 0666);
	if (shm_id < 0) {
		perror("Unable to get shared memory");
		exit(1);
	}

	int* shm;
	shm = (int *) shmat(shm_id, 0, 0);
	if (shm < 0) {
		perror("Unable to attach memory");
		exit(1);
	}

	*shm = 0;

	shmdt(shm);

	/**
	 * Ciclo infinito che accetta le richieste,
	 * genera i figli
	 */
	while (TRUE) {
		printf("\tAspetto un msg sulla coda\n");
		msgrcv(MSG_Q__main_bus, richiesta, sizeof(request), TOSRV, 0);
		printf("\t\t==> Ricevuto messaggio\n");

		son = fork();
		if (son == 0) {
			int costo = 0; 
			int sequenza = 0;
			int turno = 0;
			int reparto = richiesta->kindof_service;
			int priorita = richiesta->priority;
			int pid_cli = richiesta->clientId;
			char bill[200];
			response *risposta = malloc(sizeof(response));

			int* shm_ptr;
			shm_ptr = (int *) shmat(shm_id, 0, 0);
			if (shm_ptr < 0) {
				perror("Unable to attach memory");
				exit(1);
			}

			printf("\tSono il figlio\n");

			down(SEM_server, 0);
			*shm_ptr = *shm_ptr + 1;
			sequenza = *shm_ptr;
			up(SEM_server, 0);

			switch (reparto) {
			//caso oculistica
			case 0:
				printf("caso oculistica");
				costo = calcola_prezzo(conf->visita_oculistica, priorita,
						conf->costo_priorita);
				turno = calcola_turno(sequenza, priorita);
				risposta->clientId = pid_cli;
				risposta->kindof_service = reparto;
				risposta->mtype = TOCLI;
				risposta->price = costo;
				risposta->turn = turno;
				msgsnd(MSG_Q__main_bus, risposta, sizeof(response), TOCLI);

				sprintf(bill,
						"RICEVUTA\nper la prenotazione di una vistita ortopedica lei ha speso %d,00 euro\n"
								"grazie per aver utilizzato i nostri servizi",
						costo);

				printf("PID %d", pid_cli);
				send_socket(bill, pid_cli);
				break;
		//caso ortopedia
			case 1:
				printf("caso ortopedia");
				costo = calcola_prezzo(conf->visita_ortopedica, priorita,
						conf->costo_priorita);
				turno = calcola_turno(sequenza, priorita);
				risposta->clientId = pid_cli;
				risposta->kindof_service = reparto;
				risposta->mtype = TOCLI;
				risposta->price = costo;
				risposta->turn = turno;
				msgsnd(MSG_Q__main_bus, risposta, sizeof(response), TOCLI);

				sprintf(bill,
						"RICEVUTA\nper la prenotazione di una vistita oculistica lei ha speso %d,00 euro\n"
								"grazie per aver utilizzato i nostri servizi",
						costo);

				printf("PID %d", pid_cli);
				send_socket(bill, pid_cli);
				break;
		//caso radiologia
			case 2:
				printf("caso radiologia");

				costo = calcola_prezzo(conf->visita_radiologica, priorita,
						conf->costo_priorita);
				turno = calcola_turno(sequenza, priorita);
				risposta->clientId = pid_cli;
				risposta->kindof_service = reparto;
				risposta->mtype = TOCLI;
				risposta->price = costo;
				risposta->turn = turno;
				msgsnd(MSG_Q__main_bus, risposta, sizeof(response), TOCLI);

				sprintf(bill,
						"RICEVUTA\nper la prenotazione di una vistita radiologica lei ha speso %d,00 euro\n"
								"grazie per aver utilizzato i nostri servizi",
						costo);

				printf("PID %d", pid_cli);
				send_socket(bill, pid_cli);
				break;
			}
			printf("TURNO %d\n", turno);
			free(risposta);
		}
	}
	wait(0);

	/**
	 * libero tutte le memorie
	 */
	free(richiesta);
	free(conf);

	/**
	 * chiamo la funzione per la pulizia delle risorse ipc
	 */
	clean(MSG_Q__main_bus, MSG_Q__oculistica, MSG_Q__radiologia,
			MSG_Q__ortopedia, SEM_server, shm_id);

	printf("TERMINO!!!!!!!!!!\n\n");

	return 0;
}
