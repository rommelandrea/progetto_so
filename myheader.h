/*
 * myheader.h
 *
 *  Created on: Jul 13, 2012
 *      Author: Andrea Romanello, Amir Curic
 */
#include "header_proj.h"

#ifndef MYHEADER_H_
#define MYHEADER_H_

typedef struct{
	int visita_radiologica;
	int visita_oculistica;
	int visita_ortopedica;

	int costo_priorita;
	int priorita_default;
} configurazione;

void down(int, int);
void up(int, int);
void leggi_conf(configurazione *);
void send_socket(char *, int);
int calcola_prezzo(int, int, int);
int calcola_turno(int, int);

#endif /* MYHEADER_H_ */
