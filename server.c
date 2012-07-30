/*
 * server.c
 *
 *      Author: Andrea Romanello and Amir Curic
 *
 * This file contain main of the server
 * the server open 4 queue, 1 master for communicate with client
 * and 3 queue (1 for each division) for sent messages.
 * when receive message from client, it call fork and the son
 * send message to division queue, send ok message to client and send
 * bill to client from socket
 */
#include "myheader.h"
#include "funzioni.h"

/**
 * funzione che riceve l'handler e fa la pulizia delle risorse ipc
 */
void handler(int sig){
	int qs, qrad, qoph, qort, shm, sem;

	printf("\n\tRicevuto handler\n\tAdesso faccio pulizia e muoio\n\n");

	qs = msgget(SERVER_KEY, 0);
	qrad = msgget(RAD_queue_KEY, 0);
	qort = msgget(ORT_queue_KEY, 0);
	qoph = msgget(OPH_queue_KEY, 0);

	msgctl(qs, IPC_RMID, 0);
	msgctl(qrad, IPC_RMID, 0);
	msgctl(qort, IPC_RMID, 0);
	msgctl(qoph, IPC_RMID, 0);

	sem = semget(SEM_KEY, 1, IPC_CREAT);
	if(sem<0){
		perror("HANDLER: Unable to get semaphore");
	}
	semctl(sem, 0, IPC_RMID);

	shm = shmget(SHM_KEY, sizeof(int), IPC_CREAT);
	if(shm<0){
		perror("HANDLER: Unabel to get shared memory");
	}
	shmctl(shm, IPC_RMID, 0);

	printf("\tPulizia terminata\n");
	exit(sig);
}

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
 * Main function
 */
int main(int argc, char **argv) {
	int MSG_Q__main_bus, MSG_Q__oculistica, MSG_Q__radiologia, MSG_Q__ortopedia,
	SEM_server;
	request *richiesta;
	int son;
	int* shm;
	int shm_id;

	configurazione *conf;
	/**
	 * registro l'handler
	 */
	signal(SIGQUIT, handler);

	richiesta = calloc(1, sizeof(request));
	
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
	SEM_server = semget(SEM_KEY, 1, IPC_CREAT|0666);
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
	conf = calloc(1, sizeof(configurazione));
	leggi_conf(conf);
	
	/**
	 * prelevo un segmento di memoria condivisa
	 * e lo inizializzo a 0
	 */
	shm_id = shmget(SHM_KEY, sizeof(int), IPC_CREAT|0666);
	if (shm_id < 0) {
		perror("Unable to get shared memory");
		exit(EXIT_FAILURE);
	}
	

	shm = (int *) shmat(shm_id, 0, 0);
	if (*shm < 0) {
		perror("Unable to attach memory");
		exit(EXIT_FAILURE);
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
		printf("\t==> Ricevuto messaggio\n");
		
		son = fork();
		if (son == 0) {
			int costo = 0; 
			int sequenza = 0;
			int turno = 0;
			int reparto = richiesta->kindof_service;
			int priorita = richiesta->priority;
			int pid_cli = richiesta->clientId;
			char bill[200];
			response *risposta = calloc(1, sizeof(response));
			reservation *prenotazione = calloc(1, sizeof(reservation));
			
			int* shm_ptr;
			shm_ptr = (int *) shmat(shm_id, 0, 0);
			if (*shm_ptr < 0) {
				perror("Unable to attach memory");
				exit(EXIT_FAILURE);
			}
			
			printf("\tSono il figlio\n");
			
			down(SEM_server, 0);
			*shm_ptr = *shm_ptr + 1;
			sequenza = *shm_ptr;
			up(SEM_server, 0);
			
			switch (reparto) {
				/* coda oculistica*/
				case 0:
					printf("\tCaso oculistica");
					costo = calcola_prezzo(conf->visita_oculistica, priorita,
										   conf->costo_priorita);
					turno = calcola_turno(sequenza, priorita);
					risposta->clientId = pid_cli;
					risposta->kindof_service = reparto;
					risposta->mtype = TOCLI;
					risposta->price = costo;
					risposta->turn = turno;
					msgsnd(MSG_Q__main_bus, risposta, sizeof(response), TOCLI);
					prenotazione->clientId = pid_cli;
					prenotazione->kindof_service = reparto;
					prenotazione->mtype = TORES;
					prenotazione->price = costo;
					prenotazione->turn = turno;
					msgsnd(MSG_Q__oculistica, prenotazione, sizeof(reservation), TORES);
					
					sprintf(bill,
							"RICEVUTA\nPer la prenotazione di una vistita oculistica lei ha speso %d,00 euro.\n"
							"Grazie per aver utilizzato i nostri servizi.\n",
							costo);
					
					printf(" PID %d", pid_cli);
					send_socket(bill, pid_cli);
					break;
					/*caso ortopedia*/
				case 1:
					printf("\tCaso ortopedia");
					costo = calcola_prezzo(conf->visita_ortopedica, priorita,
										   conf->costo_priorita);
					turno = calcola_turno(sequenza, priorita);
					risposta->clientId = pid_cli;
					risposta->kindof_service = reparto;
					risposta->mtype = TOCLI;
					risposta->price = costo;
					risposta->turn = turno;
					msgsnd(MSG_Q__main_bus, risposta, sizeof(response), TOCLI);

					prenotazione->clientId = pid_cli;
					prenotazione->kindof_service = reparto;
					prenotazione->mtype = TORES;
					prenotazione->price = costo;
					prenotazione->turn = turno;
					msgsnd(MSG_Q__ortopedia, prenotazione, sizeof(reservation), TORES);
					
					sprintf(bill,
							"RICEVUTA\nPer la prenotazione di una vistita ortopedica lei ha speso %d,00 euro.\n"
							"Grazie per aver utilizzato i nostri servizi.\n",
							costo);
					
					printf(" PID %d", pid_cli);
					send_socket(bill, pid_cli);
					break;
					/*caso radiologia*/
				case 2:
					printf("\tCaso radiologia");
					
					costo = calcola_prezzo(conf->visita_radiologica, priorita,
										   conf->costo_priorita);
					turno = calcola_turno(sequenza, priorita);
					risposta->clientId = pid_cli;
					risposta->kindof_service = reparto;
					risposta->mtype = TOCLI;
					risposta->price = costo;
					risposta->turn = turno;
					msgsnd(MSG_Q__main_bus, risposta, sizeof(response), TOCLI);

					prenotazione->clientId = pid_cli;
					prenotazione->kindof_service = reparto;
					prenotazione->mtype = TORES;
					prenotazione->price = costo;
					prenotazione->turn = turno;
					msgsnd(MSG_Q__radiologia, prenotazione, sizeof(reservation), TORES);
					
					sprintf(bill,
							"RICEVUTA\nPer la prenotazione di una vistita radiologica lei ha speso %d,00 euro.\n"
							"Grazie per aver utilizzato i nostri servizi.\n",
							costo);
					
					printf(" PID %d", pid_cli);
					send_socket(bill, pid_cli);
					break;
			}
			printf("\tTURNO %d\n", turno);
			free(risposta);
			free(prenotazione);
			exit(0);
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
	
	printf("TERMINO!\n\n");
	
	return EXIT_SUCCESS;
}
