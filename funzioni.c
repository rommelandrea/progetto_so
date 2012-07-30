/*
 * funzioni.c
 *
 *  Created on: Jul 23, 2012
 *      Author: rommel
 */

#include <stdio.h>

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
