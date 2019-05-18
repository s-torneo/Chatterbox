/**
 * @file online.c
 * @brief File per l'implementazione della gestione degli utenti online
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <connections.h>
#include <rnwn.h>

//macro per allocazioni dinamiche
#define SYSCALL_D(r,c,e) \
    if((r=c)==NULL) { perror(e); exit(-1); }

/**
 * @struct Online
 * @brief è la struttura che rappresenta la lista degli utenti online
 * @var nick indica il nome dell'utente
 * @var fd indica il descrittore
 * @var next puntatore all'elemento successivo
 * @var mutexon è una variabile di mutex associata ad ogni utente
 */
typedef struct Online1{
  char *nick;
  long fd;
  struct Online1 *next;
  pthread_mutex_t mutexon;
}Online;

/** 
 * @var online è un puntatore di tipo Online, che punta al primo elemento della struttura
*/
Online *online=NULL;

/** 
 * @var last_online è un puntatore di tipo Online, che punta all'ultimo elemento della struttura
*/
Online *last_online=NULL;

/** 
 * @var mutex2 è una variabile per la gestione della mutua-esclusione
*/
pthread_mutex_t mutex2=PTHREAD_MUTEX_INITIALIZER;

/** 
 * @var nutenti indica il numero di utenti online
*/
int nutenti=0;

/**
 * @function SearchFd
 * @brief Cerca il descrittore all'interno della lista degli online
 * @param fd indica il descrittore da cercare
 * @return ritorna un puntatore di tipo Online se trova il descrittore, altrimenti NULL
 */
Online * SearchFd(long fd){
  Online *curr=online;
  //cerco il descrittore e se lo trovo allora ritorno il puntatore
  while(curr!=NULL){
    if(fd==curr->fd)
      return curr;
    curr=curr->next;
  }
  return NULL;
}

/**
 * @function SendMsg_mutex
 * @brief Invia l'intero messaggio in mutua-esclusione
 * @param fd indica il descrittore
 * @param msg variabile tramite cui accedere ai campi della struttura message_t
 * @param op indica il tipo di operazione
 * @return 1 se l'operazione è andata a buon fine, 0 altrimenti
 */
int SendMsg_mutex(long fd, message_t *msg, int op){
  //prendo la mutua-esclusione sull'intera struttura online
  pthread_mutex_lock(&mutex2);
  //vedo se l'utente è online, in tal caso curr è !=NULL
  Online *curr=SearchFd(fd);
  if(curr!=NULL){
    //prendo la mutua-esclusione sul descrittore dell'utente a cui si vuole inviare il messaggio
    pthread_mutex_lock(&(curr->mutexon));
    //rilascio la mutua-esclusione sull'intera struttura online
    pthread_mutex_unlock(&mutex2);
    msg->hdr.op=op;
    //invio il messaggio
    sendMsg(fd,msg);
    //rilascio la mutua-esclusione sul descrittore
    pthread_mutex_unlock(&(curr->mutexon));
    return 1;
  }
  else{
    //rilascio la mutua-esclusione sull'intera struttura online
    pthread_mutex_unlock(&mutex2);
    return 0;
  }
}

/**
 * @function SendData_mutex
 * @brief Invia il body del messaggio in mutua-esclusione
 * @param fd indica il descrittore
 * @param msg variabile tramite cui accedere ai campi della struttura message_t
 */
void SendData_mutex(long fd, message_data_t *msg){
  //prendo la mutua-esclusione sull'intera struttura online
  pthread_mutex_lock(&mutex2);
  //vedo se l'utente è online, in tal caso curr è !=NULL
  Online *curr=SearchFd(fd);
  if(curr!=NULL){
    //prendo la mutua-esclusione sul descrittore dell'utente a cui si vuole inviare il messaggio
    pthread_mutex_lock(&(curr->mutexon));
    //rilascio la mutua-esclusione sull'intera struttura online
    pthread_mutex_unlock(&mutex2);
    //invio il body
    sendData(fd,msg);
    //rilascio la mutua-esclusione sul descrittore
    pthread_mutex_unlock(&(curr->mutexon));
  }
  else
    //rilascio la mutua-esclusione sull'intera struttura online
    pthread_mutex_unlock(&mutex2);
}

/**
 * @function SendHdr_mutex
 * @brief Invia l'header del messaggio in mutua-esclusione
 * @param fd indica il descrittore
 * @param msg variabile tramite cui accedere ai campi della struttura message_t
 * @param op indica il tipo di operazione
 */
void SendHdr_mutex(long fd, message_hdr_t *msg, int op){
  //prendo la mutua-esclusione sull'intera struttura online
  pthread_mutex_lock(&mutex2);
  //vedo se l'utente è online, in tal caso curr è !=NULL
  Online *curr=SearchFd(fd);
  if(curr!=NULL)
    //prendo la mutua-esclusione sul descrittore dell'utente a cui si vuole inviare il messaggio
    pthread_mutex_lock(&(curr->mutexon));
  //rilascio la mutua-esclusione sull'intera struttura online
  pthread_mutex_unlock(&mutex2);
  //setto il campo della struttura message_t in modo da specificare il tipo di operazione
  msg->op=op;
  //invio l'header
  sendHeader(fd,msg);
  if(curr!=NULL)
    //rilascio la mutua-esclusione sul descrittore
    pthread_mutex_unlock(&(curr->mutexon));
}

/**
 * @function DeleteOnline
 * @brief Elimina l'utente dalla lista degli online
 * @param fd indica il descrittore dell'utente da eliminare
 */
void DeleteOnline(long fd){
  //prendo la mutua-esclusione sull'intera struttura online
  pthread_mutex_lock(&mutex2);
  if(online==NULL){
    pthread_mutex_unlock(&mutex2);
    return;
  }
  Online *curr=online;
  //controllo se il primo fd della lista corrisponde a quello cercato, in tal caso elimino l'utente dalla lista liberando la memoria e decrementando il numero di utenti online
  if(curr->fd==fd){
    online=curr->next;
    free(curr->nick);
    free(curr);
    nutenti--;
  }
  else{
    Online *prev=online; curr=online->next;
    int trovato=0;
    //cerco il descrittore nella lista
    while(curr!=NULL&&!trovato){
      if(curr->fd==fd)
        trovato=1;
      else{
        curr=curr->next;
        prev=prev->next;
      }
    }
    //se lo trovo allora elimino l'utente dalla lista liberando la memoria e decrementando il numero di utenti online
    if(trovato){
      prev->next=curr->next;
      if(prev->next==NULL)last_online=prev;
      free(curr->nick);
      free(curr); 
      nutenti--;
    }
  }
  //rilascio la mutua-esclusione sull'intera struttura online
  pthread_mutex_unlock(&mutex2);
}

/**
 * @function ListaOnline
 * @brief Invia la lista degli utenti online
 * @param fd indica il descrittore dell'utente a cui inviare la lista
 * @param msg puntatore per accedere alla struttura message_t
 */
void ListaOnline(long fd, message_t *msg){
  //prendo la mutua-esclusione sull'intera struttura online
  pthread_mutex_lock(&mutex2);
  Online *curr=online;
  //setto la lunghezza del messaggio
  msg->data.hdr.len=(MAX_NAME_LENGTH+1)*nutenti;
  //invio il body del messaggio contenente in particolare la lunghezza del messaggio da inviare
  writen(fd,&(msg->data.hdr),sizeof(message_data_hdr_t));
  //cerco se l'utente è online tramite il descrittore associato
  Online *tmp=SearchFd(fd);
  if(tmp!=NULL){
    //se trovo l'utente allora prendo la mutua-esclusione sul descrittore
    pthread_mutex_lock(&(tmp->mutexon));
    //scorro la lista degli utenti online
    while(curr!=NULL){
      //invio il nome dell'utente
      writen(fd,curr->nick,(MAX_NAME_LENGTH+1));
      curr=curr->next;
    }
    //rilascio la mutua-esclusione sul descrittore
    pthread_mutex_unlock(&(tmp->mutexon));
  }
  //rilascio la mutua-esclusione sull'intera struttura online
  pthread_mutex_unlock(&mutex2);
}

/**
 * @function GetFd
 * @brief Cerca un utente
 * @param nick indica il nome dell'utente da cercare
 * @return ritorna il descrittore associato all'utente se lo trova, altrimenti -1
 */
long GetFd(char *nick){
  //prendo la mutua-esclusione sull'intera struttura degli utenti online
  pthread_mutex_lock(&mutex2);
  Online *curr=online;
  //cerco l'utente
  while(curr!=NULL){
    //se lo trovo rilascio la mutua-esclusione e ritorno il descrittore associato
    if(!strcmp(nick,curr->nick)){
      pthread_mutex_unlock(&mutex2); 
      return curr->fd;
    }
    curr=curr->next;
  }
  //rilascio la mutua-esclusione sull'intera struttura degli utenti online
  pthread_mutex_unlock(&mutex2);
  return -1;
}

/**
 * @function PushOnline
 * @brief Aggiunge un utente alla lista degli utenti online
 * @param fd indica il descrittore dell'utente
 * @param msg puntatore per accedere alla struttura message_t
 * @return 0 se l'utente era già online, 1 altrimenti
 */
int PushOnline(long fd, message_t *msg){
  //controllo se l'utente è già online, se lo è allora non lo reinserisco
  long fd2=GetFd(msg->hdr.sender);
  if(fd2>=0)
    return 0;
  //prendo la mutua-esclusione sull'intera struttura online
  pthread_mutex_lock(&mutex2);
  Online *new;
  SYSCALL_D(new,malloc(sizeof(Online)), "malloc");
  SYSCALL_D(new->nick,calloc((MAX_NAME_LENGTH+1),sizeof(char)), "calloc");
  strncpy(new->nick,msg->hdr.sender,(MAX_NAME_LENGTH+1));
  new->fd=fd;
  new->next=NULL;
  //inizializzo la variabile di mutua-esclusione dedicata all'utente che si sta inserendo
  pthread_mutex_init(&(new->mutexon),NULL);
  if(online==NULL){
    last_online=new;
    online=new;
  }
  else{
    last_online->next=new;
    last_online=new;
  }
  //incremento il numero degli utenti online
  nutenti++;
  //rilascio la mutua-esclusione sull'intera struttura online
  pthread_mutex_unlock(&mutex2);
  return 1;
}

/**
 * @function DestroyList
 * @brief Elimina la struttura degli utenti online
 */
void DestroyList(){
  Online *curr=NULL;
  while(online!=NULL){
    curr=online;
    online=online->next;
    free(curr->nick);
    free(curr);
  }
}
