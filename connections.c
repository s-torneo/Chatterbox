/**
 * @file connections.c
 * @brief Contiene le funzioni che implementano il protocollo tra i clients ed il server
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */	

#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <connections.h>
#include <pthread.h>
#include <message.h>
#include <rnwn.h>

//macro per chiamate di sistema
#define SYSCALL(r,c) \
    if((r=c)==-1) { return -1; }

//macro per allocazioni dinamiche
#define SYSCALL_D(r,c, e) \
    if((r=c)==NULL) { perror(e); exit(-1);}

/**
 * @var notused usata come variabile di ritorno dalle funzioni passate come argomento alle MACRO
 */
int notused;


/* @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server 
 *
 * @param path Path del socket AF_UNIX 
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs){
  //dichiaro il descrittore associato alla connessione
  long fd_skt;
  struct sockaddr_un sa;
  //setto il campo path della struttura con il path passato come parametro, la cui lunghezza è un parametro data dal file di configurazione
  strcpy(sa.sun_path,path);
  sa.sun_family=AF_UNIX;
  //apro una nuova connessione
  SYSCALL(fd_skt, socket(AF_UNIX,SOCK_STREAM,0));
  //itera mentre il client non riesce a connettersi al server e il numero di tentativi (ntimes) è > 0
  while((connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa))==-1)&&(ntimes>0)){
    sleep(1);
    //decremento il numero di tentativi di connessione
    ntimes--;
  }
  if(!ntimes)return -1;
  return fd_skt;
}

// -------- server side ----- 
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return -1 se c'è stato un errore, n byte letti altrimenti
 */
int readHeader(long connfd, message_hdr_t *hdr){
  return readn(connfd,hdr,sizeof(message_hdr_t));
}

/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'è stato un errore, 1 altrimenti
 */
int readData(long fd, message_data_t *data){
  //leggo l'header del body
  SYSCALL(notused, readn(fd,&(data->hdr),sizeof(message_data_hdr_t)));
  //alloco memoria per il messaggio
  SYSCALL_D(data->buf, calloc(data->hdr.len,sizeof(char)), "calloc");
  //leggo il messaggio
  int n=readn(fd,data->buf,data->hdr.len);
  if(n<0){
    //se la readn ha ritornato un valore <0 allora libero la memoria allocata per il messaggio
    free(data->buf);
    return n;
  }
  return 1;
}

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return <0 se c'è stato un errore, 1 altrimenti
 */
int readMsg(long fd, message_t *msg){
  //leggo l'header del messaggio
  SYSCALL(notused, readn(fd,&(msg->hdr),sizeof(message_hdr_t)));
  //leggo l'header del body
  SYSCALL(notused, readn(fd,&(msg->data.hdr),sizeof(message_data_hdr_t))); 
  //alloco memoria per il messaggio
  SYSCALL_D(msg->data.buf, malloc(sizeof(char)*msg->data.hdr.len), "malloc");
  //leggo il messaggio
  int n=readn(fd,msg->data.buf,msg->data.hdr.len);
  if(n<0){
    //se la readn ha ritornato un valore <0 allora libero la memoria allocata per il messaggio
    free(msg->data.buf);
    return n;
  }
  return 1;
}

// ------- client side ------

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendData(long fd, message_data_t *msg){
  SYSCALL(notused, writen(fd,&(msg->hdr),sizeof(message_data_hdr_t)));
  SYSCALL(notused, writen(fd,msg->buf,msg->hdr.len));
  return 1;
}

/**
 * @function sendHeader
 * @brief Invia l'header del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendHeader(long fd, message_hdr_t *msg){
  SYSCALL(notused, writen(fd,msg,sizeof(message_hdr_t)));
  return 1;
}

/**
 * @function sendMsg
 * @brief Invia l'intero messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendMsg(long fd, message_t *msg){ 
  //invio l'header
  SYSCALL(notused, writen(fd,&(msg->hdr),sizeof(message_hdr_t)));
  //invio l'header del body
  SYSCALL(notused, writen(fd,&(msg->data.hdr),sizeof(message_data_hdr_t)));
  //invio il messaggio
  SYSCALL(notused, writen(fd,msg->data.buf,msg->data.hdr.len));
  return 1;
}

/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server 
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendRequest(long fd, message_t *msg){
  //invio l'header del messaggio
  writen(fd,&(msg->hdr),sizeof(message_hdr_t));
  //controllo il tipo dell'operazione e faccio l'operazione opportuna
  switch(msg->hdr.op){
    case UNREGISTER_OP:
    case POSTTXT_OP: 
    case POSTTXTALL_OP:
    case POSTFILE_OP: 
    case GETFILE_OP:{
      //invio il body del messaggio
      sendData(fd,&(msg->data));
    }break;
    case CREATEGROUP_OP:
    case ADDGROUP_OP:
    case DELGROUP_OP:{
      //invio l'header del body del messaggio
      SYSCALL(notused, writen(fd,&(msg->data.hdr),sizeof(message_data_hdr_t)));
    }break;
    default:{}
  }
  return 1;
}

#endif /* CONNECTIONS_H_ */
