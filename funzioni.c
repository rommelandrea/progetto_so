/*
 * funzioni.c
 *
 *  Created on: Jul 23, 2012
 *      Author: Andrea Romanello and Amir Curic
 *
 *  This file contain function for server
 */
#include "myheader.h"

/**
 * la funzione calcola il prezzo della prestazione
 * in base al costo base, alla priorita' e al costo sulla priorita'
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

/**
 * Funzione per la pulizia delle risorse ipc
 * Non più utilizzata
 */
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
		fgets(line, 256, fp);
		if (fnmatch("#*", line, 0)) {

			if (!fnmatch("VISITA_RADIOLOGICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_radiologica = price;
			}

			if (!fnmatch("VISITA_ORTOPEDICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_ortopedica = price;
			}

			if (!fnmatch("VISITA_OCULISTICA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->visita_oculistica = price;
			}
			if (!fnmatch("COSTO_PRIORITA*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->costo_priorita = price;
			}
			if (!fnmatch("PRIORITY*", line, 0)) {
				pline = strtok(line, " ");
				pline = strtok(NULL, " ");
				price = atoi(pline);
				c->priorita_default = price;
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
	int sd, n;
	struct sockaddr_un srvaddr;
	char sock[20];
	/**
	 * sleep temporanea per la sincronizzazione dei processi
	 */
	sleep(3);

	sprintf(sock, "/tmp/%d.sock", p);

	printf("\n\tPronto per inviare al socket %s\n\n", sock);

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

	printf("\nCONNECT....\n");

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
