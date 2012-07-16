#include "header_proj.h"
#include "myheader.h"

#include <fnmatch.h>

int numTurno = 0;

void down(int semid, int semnum){
	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	semop(semid, &sb, 1);
}

void up(int semid, int semnum){
	struct sembuf sb;

	sb.sem_num = semnum;
	sb.sem_op = 1;
	sb.sem_flg = 0;
	semop(semid, &sb, 1);
}

int main(int argc, char **argv) {
	/**
	 * creo le variabili
	 */
	int MSG_Q__main_bus, MSG_Q__oculistica, MSG_Q__radiologia, MSG_Q__ortopedia,
			SEM_server;
	request richiesta;

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
	 */
	configurazione conf;
	conf = malloc(sizeof(configurazione));


	/**
	 * apro in lettura il file di configurazione
	 */
	FILE *fp = fopen("config.conf", "r");
	if(fp == NULL){
		perrror("Cannot open the file");
		exit(1);
	}


	if(!fclose(fp)){
		perrror("Cannot close the file");
		exit(1);
	}

	/**
	 * Ciclo infinito che accetta le richieste,
	 * genera i figli
	 */
//	while (TRUE) {
		printf("aspetto un msg sulla coda\n");
		msgrcv(MSG_Q__main_bus, &richiesta, sizeof(request), TOSRV, 0);
		printf("ricevuto messaggio\n");

		son = fork();
		if (son == 0) {
			int reparto = richiesta.kindof_service;

			printf("sono il figlio\n");

			down(SEM_server, 0);
			numTurno++;
			up(SEM_server, 0);

			switch(reparto){
			case 0:
				printf("caso 0");
				break;
			case 1:
				printf("caso 1");
				break;
			case 2:
				printf("caso 2");
				break;
			}
		}
		printf("pid client: %d reparto: %d\n", richiesta.clientId,
				richiesta.kindof_service);
		printf("num turno %d\n", numTurno);

//	}
	/**
	 * chiudo le code di messaggi
	 */
	msgctl(MSG_Q__main_bus, IPC_RMID, 0);
	msgctl(MSG_Q__oculistica, IPC_RMID, 0);
	msgctl(MSG_Q__radiologia, IPC_RMID, 0);
	msgctl(MSG_Q__ortopedia, IPC_RMID, 0);

	semctl(SEM_server, 0, IPC_RMID);

	return 0;
}
