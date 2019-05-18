/**
 * @file coda.h
 * @brief File per la gestione della coda delle richieste
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/**
 * @struct Coda
 * @brief è la struttura usata per la gestione delle richieste al server
 * @fd indica il descrittore
 * @var next è un puntatore all'elemento successivo
 */
typedef struct Coda1{
  long fd;
  struct Coda1 *next;
}Coda;

/**
 * @var coda è il puntatore al primo elemento della lista
 */
Coda *coda=NULL;

/**
 * @var coda è il puntatore all'ultimo elemento della lista
 */
Coda *ultimo=NULL;

/**
 * @var mutex variabile per la gestione della mutua-esclusione
 */
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

/**
 * @var cond variabile di condizione
 */
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;

/**
 * @function Push
 * @brief Inserisce un descrittore nella coda
 * @param fd indica il descrittore da inserire
 */
void Push(long fd){
  //prendo la mutua-esclusione 
  pthread_mutex_lock(&mutex);
  Coda *new=malloc(sizeof(Coda));
  if(new!=NULL){
    new->fd=fd;
    new->next=NULL;
    if(coda==NULL){
      ultimo=new;
      coda=new;
    }
    else{
      ultimo->next=new;
      ultimo=new;
    }
    //se il descrittore è != da -1 risveglio un solo thread in attesa
    if(fd!=-1)
      pthread_cond_signal(&cond);
    //altrimenti li risveglio tutti
    else 
      pthread_cond_broadcast(&cond);
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex);
}

/**
 * @function Pop
 * @brief Elimina un descrittore dalla coda
 * @return il valore del descrittore
 */
long Pop(){
  //prendo la mutua-esclusione
  pthread_mutex_lock(&mutex);
  //se la coda è vuota mi metto in attesa sulla variabile di condizione
  while(coda==NULL)
    pthread_cond_wait(&cond,&mutex);
  long ele=coda->fd;
  //se il descrittore è != -1 allora lo elimino dalla coda
  if(ele!=-1){
    Coda *com=coda;
    coda=coda->next;
    free(com);
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex);
  return ele;
}