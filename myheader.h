/*
 * myheader.h
 *
 *  Created on: Jul 13, 2012
 *      Author: rommel
 */

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
#endif /* MYHEADER_H_ */
