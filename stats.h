/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

/**
 * @file stats.h
 * @brief File per la gestione delle statistiche
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


/**
 * @struct statistics
 * @brief è la struttura per la gestione delle statistiche
 * @var nusers indica il numero di utenti registrati
 * @var nonline indica il numero di utenti connessi
 * @var ndelivered indica il numero di messaggi testuali consegnati
 * @var nnotdelivered indica il numero di messaggi testuali non ancora consegnati
 * @var nfiledelivered indica il numero di file consegnati
 * @var nfilenotdelivered indica il numero di file non ancora consegnati
 * @var nerrors indica il numero di messaggi di errore
 */
struct statistics {
    unsigned long nusers;                       // n. di utenti registrati
    unsigned long nonline;                      // n. di utenti connessi
    unsigned long ndelivered;                   // n. di messaggi testuali consegnati
    unsigned long nnotdelivered;                // n. di messaggi testuali non ancora consegnati
    unsigned long nfiledelivered;               // n. di file consegnati
    unsigned long nfilenotdelivered;            // n. di file non ancora consegnati
    unsigned long nerrors;                      // n. di messaggi di errore
};

/** 
 * @var mutex_stat variabile per la mutua-esclusione delle statistiche
*/
pthread_mutex_t mutex_stat=PTHREAD_MUTEX_INITIALIZER;

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento 
 */
static inline int printStats(FILE *fout) {
    extern struct statistics chattyStats;
    if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
		(unsigned long)time(NULL),
		chattyStats.nusers, 
		chattyStats.nonline,
		chattyStats.ndelivered,
		chattyStats.nnotdelivered,
		chattyStats.nfiledelivered,
		chattyStats.nfilenotdelivered,
		chattyStats.nerrors
		) < 0) return -1;
    fflush(fout);
    return 0;
}

/**
 * @function StatFile
 * @brief Apre il file in cui stampare le statistiche e passa il puntatore alla funzione che stampa su file
 */
void StatFile(char *statfilename){
  FILE *fp=fopen(statfilename,"w"); //apro il file in lettura
  //controllo se l'apertura sia andata a buon fine
  if(fp==NULL){
    perror(statfilename);
    return;
  }
  pthread_mutex_lock(&mutex_stat); //prendo la mutua-esclusione sulle statistiche
  printStats(fp);
  pthread_mutex_unlock(&mutex_stat); //rilascio la mutua-esclusione sulle statistiche
  fclose(fp); //chiudo il file
}

#endif /* MEMBOX_STATS_ */
