/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore. 
 */


#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define UNIX_PATH_MAX 108

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <config.h>
#include <stats.h>
#include <fcntl.h>
#include <libgen.h>

#include <parser.h>
#include <rnwn.h>
#include <connections.h>
#include <message.h>
#include <coda.h>
#include <stats.h>
#include <online.h>
#include <hash_history.h>
#include <hash_gruppi.h>

//macro per chiamate di sistema
#define SYSCALL(r,c) \
    if((r=c)==-1) { return -1; }

//macro per chiamate di sistema
#define SYSCALL2(r,c,e) \
    if((r=c)==-1) { perror(e); exit(-1); }

//macro per allocazioni dinamiche
#define SYSCALL_D(r,c,e) \
    if((r=c)==NULL) { perror(e); exit(-1); }

/* struttura che memorizza le statistiche del server, struct statistics 
 * e' definita in stats.h.
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };

/**
* @var fine variabile usata per la gestione dei segnali 
*/
volatile sig_atomic_t fine=0;

/**
* @var pfd usata per la gestione della pipe
  @var notused usata come variabile di ritorno dalle funzioni passate come argomento alle MACRO
*/
int pfd[2], notused;

/**
 * @function IncrError
 * @brief Modifica le statistiche sugli errori, in mutua-esclusione
 */
void IncrError(){
  //prendo la mutua-esclusione sulle statistiche
  pthread_mutex_lock(&mutex_stat);
  //incremento il numero di errori
  chattyStats.nerrors++;
  //rilascio la mutua-esclusione sulle statistiche
  pthread_mutex_unlock(&mutex_stat);
}

/**
 * @function DeRegistra
 * @brief Deregistra un utente o cancella un gruppo
 * @param fd indica il descrittore del client che ha fatto la richiesta di deregistrazione o di cancellazione di un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int DeRegistra(long fd, message_t *msg){
  //controllo prima se l'operazione richiesta è la cancellazione di un gruppo
  if(DeleteGroup(fd,msg))
    return 1;
  //altrimenti cerco se l'utente che ha fatto richiesta di deregistrarsi esiste e se coincide con l'utente che vuole deregistrare
  if(Search(msg->hdr.sender)==NULL || strcmp(msg->data.hdr.receiver,msg->hdr.sender)!=0){
    //se non esiste allora invio un messaggio di errore
    SendHdr_mutex(fd, &(msg->hdr), OP_NICK_UNKNOWN);
    IncrError(); //incremento il numero di errori
  }
  else{
    //se esiste invece invio un messaggio di ok
    SendHdr_mutex(fd, &(msg->hdr), OP_OK);
    //elimino l'utente dalla hash
    Delete(msg->hdr.sender);
    //elimino l'utente dalla lista degli online
    DeleteOnline(fd);
    pthread_mutex_lock(&mutex_stat);
    //decremento il numero di utenti online
    chattyStats.nusers--;
    pthread_mutex_unlock(&mutex_stat);
  }
  return 1;
}

/**
 * @function Connetti
 * @brief Connette un utente 
 * @param fd indica il descrittore del client che ha fatto la richiesta di connettersi
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int Connetti(long fd, message_t *msg){
  //cerco se l'utente che ha fatto richiesta di connettersi esiste
  if(Search(msg->hdr.sender)!=NULL){
    //se esiste allora lo aggiungo alla lista degli online se non è già online
    if(PushOnline(fd, msg)){
      //prendo la mutua-esclusione sulle statistiche
      pthread_mutex_lock(&mutex_stat);
      //incremento il numero di utenti online
      chattyStats.nonline++;
      //rilascio la mutua-esclusione sulle statistiche
      pthread_mutex_unlock(&mutex_stat); 
    }
    //invio un messaggio di ok al client
    SendHdr_mutex(fd, &(msg->hdr), OP_OK);
    //mando la lista degli online al client
    ListaOnline(fd, msg);
  }
  else{
    //se non esiste allora invio un messaggio di errore
    SendHdr_mutex(fd, &(msg->hdr), OP_NICK_UNKNOWN);
    IncrError(); //incremento il numero di errori
  }
  return 1;
}

/**
 * @function Registra
 * @brief Registra un utente 
 * @param fd indica il descrittore del client che ha fatto la richiesta di registrarsi
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int Registra(long fd, message_t *msg){
  //cerco se l'utente che ha fatto richiesta di registrarsi esiste già
  if(Search(msg->hdr.sender)!=NULL){
    //se esiste allora mando un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_NICK_ALREADY);
    IncrError(); //incremento il numero di errori
  }
  else{
    //se non esiste allora lo aggiungo agli online
    PushOnline(fd, msg);
    //invio un messaggio di ok al client
    SendHdr_mutex(fd, &(msg->hdr), OP_OK);
    //inserisco l'utente nell'hash
    Insert(msg->hdr.sender);
    //invio la lista degli utenti online al client
    ListaOnline(fd, msg);
    //elimino l'utente dalla lista degli utenti online
    DeleteOnline(fd);
    pthread_mutex_lock(&mutex_stat);
    //incremento il numero di utenti registrati
    chattyStats.nusers++;
    pthread_mutex_unlock(&mutex_stat);
  }
  return 1;
}

/**
 * @function PostTxt
 * @brief Invia un messaggio testuale ad un utente o ad un gruppo
 * @param fd indica il descrittore del client che ha fatto la richiesta di inviare un messaggio testuale ad un altro utente o ad un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int PostTxt(long fd, message_t *msg){
  //controllo se la lunghezza del messaggio è maggiore di quella prevista nel file di configurazione
  if(msg->data.hdr.len>maxmsgsize){
    //se lo è allora invio un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_MSG_TOOLONG);
    IncrError();
    return 1;
  }
   //controllo se l'operazione richiesta è l'invio di un messaggio testuale ad un gruppo
  if(FindGroup(fd,msg,TXT_MESSAGE)){
    //invio un messaggio di ok al client
    SendHdr_mutex(fd, &(msg->hdr), OP_OK);
    return 1;
  }
  //cerco se l'utente a cui inviare il messaggio esiste
  if(Search(msg->data.hdr.receiver)==NULL){
    //se non esiste allora invio un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_NICK_UNKNOWN);
    IncrError();
    return 1;
  }
  //aggiungo il messaggio alla history del destinatario
  Add_H(msg,TXT_MESSAGE);
  //ottengo il descrittore del destinatario
  long fd2=GetFd(msg->data.hdr.receiver);
  //se è online gli invio il messaggio e incremento il numero di messaggi testuali consegnati
  if(SendMsg_mutex(fd2,msg,TXT_MESSAGE)){
    pthread_mutex_lock(&mutex_stat);
    chattyStats.ndelivered++;
    pthread_mutex_unlock(&mutex_stat);
  }
  else{
    pthread_mutex_lock(&mutex_stat);
    //altrimenti incremento il numero di messaggi testuali non ancora consegnati
    chattyStats.nnotdelivered++;
    pthread_mutex_unlock(&mutex_stat);
  }
  //invio un messaggio di ok al client
  SendHdr_mutex(fd, &(msg->hdr), OP_OK);
  return 1;
}

/**
 * @function CreaLista
 * @brief Crea un array di stringhe
 * @return un puntatore che rappresenta l'array di stringhe
 */
char ** CreaLista(){
  char **lista=malloc(sizeof(char*)*maxhistmsgs);
  if(lista!=NULL)
    for(int j=0;j<maxhistmsgs;j++)
     lista[j]=malloc(sizeof(char)*(MAX_NAME_LENGTH+1));
  return lista;
}

/**
 * @function CancellaLista
 * @brief Elimina la memoria allocata per l'allocazione dell'array di stringhe
 */
void CancellaLista(char **lista){
  for(int j=0;j<maxhistmsgs;j++)
    free(lista[j]);
  free(lista);
}

/**
 * @function PostAll
 * @brief Invia un messaggio testuale a tutti gli utenti
 * @param fd indica il descrittore del client che vuole inviare un messaggio a tutti
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int PostAll(long fd, message_t *msg){
  //controllo se la lunghezza del messaggio è maggiore di quella prevista nel file di configurazione
  if(msg->data.hdr.len>maxmsgsize){
    //se lo è allora invio un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_MSG_TOOLONG);
    IncrError();
    return 1;
  }
  //creo un array di stringhe per contenere la lista degli utenti a cui inviare il messaggio
  char **lista=CreaLista();
  //aggiungo il messaggio nella history di tutti gli utenti e mi faccio restituire il numero di utenti a cui è stato inviato il messaggio
  int cont=AddtoAll_H(msg,lista);
  for(int i=0;i<cont;++i){
    //ottengo il descrittore dell'utente a cui inviare il messaggio
    long fd=GetFd(lista[i]);
    //se l'utente è online allora invia il messaggio
    if(SendMsg_mutex(fd,msg,TXT_MESSAGE)){
      pthread_mutex_lock(&mutex_stat);
      //incrementa il numero di messaggi testuali consegnati
      chattyStats.ndelivered++;
      pthread_mutex_unlock(&mutex_stat);
    }
    else{
      pthread_mutex_lock(&mutex_stat);
      //incrementa il numero di messaggi testuali non ancora consegnati
      chattyStats.nnotdelivered++;
      pthread_mutex_unlock(&mutex_stat);
    }  
  }
  //cancella l'array di stringhe precedentemente allocato
  CancellaLista(lista);
  //invio un messaggio di ok al client che ha fatto richiesta
  SendHdr_mutex(fd, &(msg->hdr), OP_OK);
  return 1;
}

/**
 * @function GetMessage
 * @brief Ottiene l'insieme dei messaggi ricevuti da un utente e li invia al client che ne ha fatto richiesta
 * @param fd indica il descrittore del client che vuole ricevere i messaggi ricevuti
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int GetMessage(long fd, message_t *msg){
  //cerco se l'utente esiste
  if(Search(msg->hdr.sender)==NULL){
    //se non esiste allora invio un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_NICK_UNKNOWN);
    IncrError();
  }
  else{
    //se esiste invio un messaggio di ok
    SendHdr_mutex(fd, &(msg->hdr), OP_OK);
    int mex_inviati=0, file_inviati=0;  
    //ottengo e invio i messaggi ricevuti dal client che me l'ha richiesti, e mi faccio restituire il numero di file e messaggi testuali consegnati
    mex_inviati=GetHistory(fd,msg,&file_inviati);
    pthread_mutex_lock(&mutex_stat);
    //incrmento il numero di messaggi testuali consegnati
    chattyStats.ndelivered+=mex_inviati;
    //incremento il numero di file consegnati
    chattyStats.nfiledelivered+=file_inviati;
    pthread_mutex_unlock(&mutex_stat);
  }
  return 1;
}

/**
 * @function UserList
 * @brief Ottiene la lista degli utenti online
 * @param fd indica il descrittore del client che vuole ricevere la lista degli utenti online
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int UserList(long fd, message_t *msg){
  //invio un messaggio di ok
  SendHdr_mutex(fd, &(msg->hdr), OP_OK);
  //invio la lista degli utenti online al client che ne ha fatto richiesta
  ListaOnline(fd, msg);
  return 1;
}

/**
 * @function CreaFile
 * @brief Crea un file
 * @param fd indica il descrittore del client che vuole inviare il file
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return <=0 in caso di errori, 1 altrimenti
 */
int CreaFile(long fd, message_t *msg){
  //estraggo il nome del file dal messaggio ricevuto
  char *str=basename(msg->data.buf);
  int dim=(strlen(dirName)+strlen(str)+2);
  char *pathname;
  SYSCALL_D(pathname,malloc(sizeof(char)*dim),"malloc");
  strncpy(pathname,dirName,dim);
  strncat(pathname,str,msg->data.hdr.len);
  free(msg->data.buf);
  //leggo il contenuto del messaggio
  if(readData(fd,&(msg->data))<0)
    return -1;
  //controllo che la lunghezza del file non sia maggiore di quella consentita dal file di configurazione
  if(msg->data.hdr.len>(maxfilesize*1024))
    return 0;
  //apro il file in scrittura
  int fd2=open(pathname,O_CREAT|O_WRONLY|O_TRUNC,0777);
  free(pathname);
  if(fd2<0)
    return -1;
  //scrivo nel file il contenuto ricevuto
  SYSCALL(notused, writen(fd2,msg->data.buf,msg->data.hdr.len));
  //chiudo il file
  SYSCALL(notused, close(fd2)); 
  return 1;
}

/**
 * @function PostFile
 * @brief Invia un file
 * @param fd indica il descrittore del client che vuole inviare il file ad un utente o a tutti gli utenti di un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int PostFile(long fd, message_t *msg){
  //controllo se la lunghezza del messaggio è maggiore di quella prevista nel file di configurazione
  if(msg->data.hdr.len>maxmsgsize){
    //se lo è allora invio un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_MSG_TOOLONG);
    readData(fd,&(msg->data));
    free(msg->data.buf); 
    IncrError();
    return 1;
  }
  //controllo se l'operazione richiesta è l'invio di un messaggio ad un gruppo
  if(!FindGroup(fd,msg,FILE_MESSAGE)){
    //controllo se il destinatario del file esiste
    if(Search(msg->data.hdr.receiver)==NULL){
      //se non esiste allora invio un messaggio di errore al client
      SendHdr_mutex(fd, &(msg->hdr), OP_NICK_UNKNOWN);
      IncrError();
      return 1;
    }
    //altrimenti aggiungo il file alla history dell'utente
    Add_H(msg,FILE_MESSAGE);
    //ottengo il descrittore dell'utente a cui inviare il file
    long fd2=GetFd(msg->data.hdr.receiver);
    //se è online allora invio il file
    if(SendMsg_mutex(fd2,msg,FILE_MESSAGE)){
      pthread_mutex_lock(&mutex_stat);
      //incremento il numero di file consegnati
      chattyStats.nfiledelivered++;
      pthread_mutex_unlock(&mutex_stat);
    }
    else{
      pthread_mutex_lock(&mutex_stat);
      //incremento il numero di file non ancora consegnati
      chattyStats.nfilenotdelivered++;
      pthread_mutex_unlock(&mutex_stat);
    }
  }
  //creo il file e in caso di successo invio un messaggio di ok
  if(CreaFile(fd, msg))
    SendHdr_mutex(fd, &(msg->hdr), OP_OK);
  else{ 
    //altrimenti invio un messaggio di errore
    SendHdr_mutex(fd, &(msg->hdr), OP_MSG_TOOLONG);
    IncrError();
  }
  free(msg->data.buf);
  return 1;
}

/**
 * @function ApriFile
 * @brief Apre un file in lettura
 * @param fd indica il descrittore del client che vuole leggere il contenuto di un file
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1 in caso di successo, -1 altrimenti
 */
int ApriFile(message_t *msg){
  //estraggo il nome del file dal messaggio ricevuto
  char *str=basename(msg->data.buf);
  int dim=(strlen(dirName)+strlen(str)+2);
  char *pathname;
  SYSCALL_D(pathname,malloc(sizeof(char)*dim),"malloc");
  strncpy(pathname,dirName,dim);
  strncat(pathname,str,msg->data.hdr.len);
  //apro il file in lettura
  int fd=open(pathname,O_RDONLY|O_TRUNC,0666);
  free(pathname);
  if(fd<0)
    return -1;
  struct stat filestat;
  //ottengo informazioni sul file
  SYSCALL(notused, fstat(fd,&filestat));
  //imposto la lunghezza del messaggio
  msg->data.hdr.len=filestat.st_size;
  free(msg->data.buf);
  msg->data.buf=calloc(msg->data.hdr.len,sizeof(char));
  //leggo il contenuto del file e lo metto dentro la variabile buf della struttura message_t
  SYSCALL(notused, readn(fd,msg->data.buf,filestat.st_size));
  return 1;
}

/**
 * @function GetFile
 * @brief Ottiene il contenuto di un file
 * @param fd indica il descrittore del client che vuole leggere il contenuto del file
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int GetFile(long fd, message_t *msg){
  //apre il file e legge il suo contenuto
  ApriFile(msg);
  //invia un messaggio di ok al client
  SendHdr_mutex(fd, &(msg->hdr), OP_OK);
  //invia il contenuto del file al client
  SendData_mutex(fd,&(msg->data));
  return 1;
}

/**
 * @function Gestisci
 * @brief riceve le richieste da parte dei client e richiama le funzioni opportune per la gestione
 * @param fd indica il descrittore del client che ha inviato la richiesta
 */
void Gestisci(long fd){
  message_t *msg=calloc(1,sizeof(message_t));
  //leggo l'header del messaggio
  int c=readHeader(fd, &(msg->hdr));
  //controllo se la lettura è andata a buon fine
  if(c<=0){
    //in caso contrario elimino l'utente dalla lista online
    DeleteOnline(fd);
    //chiudo il descrittore
    SYSCALL2(notused, close(fd), "close");
    pthread_mutex_lock(&mutex_stat);
    if(chattyStats.nonline)
      chattyStats.nonline--;
    pthread_mutex_unlock(&mutex_stat);
    free(msg);
    return;
  }
  int n=0;
  switch(msg->hdr.op){
    case REGISTER_OP:{
      n=Registra(fd,msg); 
    } break;
    case CONNECT_OP:{
      n=Connetti(fd,msg); 
    }break;
    case UNREGISTER_OP:{
      if(readn(fd,&(msg->data),sizeof(message_data_hdr_t))){
        n=DeRegistra(fd,msg); 
        free(msg->data.buf); 
      }
    }break;
    case USRLIST_OP:{
      n=UserList(fd,msg);
      free(msg->data.buf); 
    }break;
    case GETPREVMSGS_OP:{
      n=GetMessage(fd,msg); 
    }break;
    case POSTTXT_OP:{
      if(readData(fd,&(msg->data))){
        n=PostTxt(fd,msg);
        free(msg->data.buf); 
      }
    }break;
    case POSTTXTALL_OP:{
      if(readData(fd,&(msg->data))){
        n=PostAll(fd,msg);
        free(msg->data.buf); 
      }
    }break;
    case POSTFILE_OP:{
      if(readData(fd,&(msg->data))){
        n=PostFile(fd,msg);
      }
    }break;
    case GETFILE_OP: {
      if(readData(fd, &(msg->data))){
        n=GetFile(fd, msg);
        free(msg->data.buf);
      }
    }break;
    case CREATEGROUP_OP:{
      if(readn(fd,&(msg->data),sizeof(message_data_hdr_t))){
        n=CreaGruppo(fd,msg);
      }
    }break;
    case ADDGROUP_OP:{
      if(readn(fd,&(msg->data),sizeof(message_data_hdr_t))){
        n=AggiungiAlGruppo(fd,msg);
      }
    }break;
    case DELGROUP_OP:{
      if(readn(fd,&(msg->data),sizeof(message_data_hdr_t))){
        n=EliminaDalGruppo(fd,msg);
      }
    }break;
    default:{
      //invio un messaggio di errore se l'operazione ricevuta non corrisponde con nessuna di quelle trattate
      setHeader(&(msg->hdr), OP_FAIL, "");
    }
  }
  free(msg);
  if(n>0){
    //scrivo sulla pipe il descrittore che ha fatto richiesta
    SYSCALL2(notused, writen(pfd[1],&fd,sizeof(long)), "writen");
  }
}

/**
 * @function Worker
 * @brief Thread che gestisce le richieste dei client
 */
static void* Worker(){
  while(1){
    //estrae un descrittore dalla coda
    long ele=Pop();
    //controlla che il descrittore sia > 0
    if(ele<0)break;
    //richiama la funzione che gestisce la richiesta
    Gestisci(ele);
  }
  return (void *)NULL;
}

/**
 * @function UpdateMax
 * @brief Aggiorna l'indice del descrittore maggiore
 * @param set indica l'insieme dei descrittori attivi
 * @param fd_num indica l'indice del descrittore maggiore
 * @return l'indice del descrittore maggiore
 */
long UpdateMax(fd_set *set, long fd_num){
  for(long fd = 0; fd<=fd_num;fd++){
    //controllo se il descrittore è settato
    if(FD_ISSET(fd, set))
      //se il descrittore è maggiore del massimo allora aggiorno il massimo
      if(fd>fd_num) fd_num=fd;
  }
  return fd_num;
}

/**
 * @function Listener
 * @brief Thread dedicato alla gestione delle richieste da parte dei client
 */
static void* Listener(){
  int fd_pipe;
  long fd_sk=0,fd_c=0,fd=0,fd_num=0;
  //creo una pipe
  SYSCALL2(fd_pipe, pipe(pfd), "pipe");
  fd_set set, rdset;
  //dichiaro la struttura per il timer utilizzato dalla select
  struct timeval timer;
  struct sockaddr_un psa;
  //setto il percorso in cui è memorizzato il file socket
  strcpy(psa.sun_path,unixpath);
  psa.sun_family=AF_UNIX;
  //creo il socket
  SYSCALL2(fd_sk, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
  //assegno un indirizzo al socket
  SYSCALL2(notused, bind(fd_sk, (struct sockaddr*)&psa,sizeof(psa)), "bind");
  //setto il numero massimo di connessioni che il server può ricevere contemporaneamente
  SYSCALL2(notused, listen(fd_sk, maxconnections), "listen");
  //mantengo il massimo indice di descrittore attivo in fd_num
  if (fd_sk > fd_num) fd_num = fd_sk;
  //inizializzo la maschera
  FD_ZERO(&set);
  //inserisco il descrittore di lettura della pipe nella maschera dei descrittori attivi
  FD_SET(pfd[0],&set); 
  //inserisco il descrittore del socket nella maschera dei descrittori attivi
  FD_SET(fd_sk,&set); 
  while(1){
    //do un valore ai campi della struttura timeval
    timer.tv_sec=0;
    timer.tv_usec=1000;
    //inizializzo la maschera dei descrittori attesi in lettura
    FD_ZERO(&rdset);
    //bisogna inizializzare ogni volta rdset perchè la select lo modifica
    rdset=set; 
    //funzione che gestisce le richieste
    int s=select(fd_num+1,&rdset,NULL,NULL,&timer);
    //controllo se la select restituisce -1 o la variabile globale fine è stata settata, e in tal caso esco dal ciclo
    if(s==-1 || fine==1)
      break;
    else if(fine==2){
      StatFile(statfilename);
      fine=0;
    }
    else{
      for (fd = 0; fd<=fd_num;fd++){
        //se il descrittore è settato a 1 nella maschera dei descrittori
        if (FD_ISSET(fd,&rdset)){
              //allora vedo se il descrittore coincide con quello ritornato dalla socket
  	      if (fd==fd_sk){
                //in tal caso accetto la connessione
                SYSCALL2(fd_c, accept(fd_sk,NULL,0), "accept");
                //aggiungo il descrittore nella maschera dei descrittori settandolo ad 1
	        FD_SET(fd_c, &set);
                //mantengo il massimo indice di descrittore attivo in fd_num
	        if (fd_c>fd_num) fd_num = fd_c;
	      }
            //altrimenti se il descrittore coincide con quello della pipe
            else if (fd==pfd[0]){
              long fd_c;
              //leggo il contenuto della pipe
              SYSCALL2(notused, readn(pfd[0],&fd_c,sizeof(long)), "readn");
              //aggiungo il descrittore nella maschera dei descrittori settandolo ad 1
	      FD_SET(fd_c, &set);
              //mantengo il massimo indice di descrittore attivo in fd_num
	      if (fd_c>fd_num) fd_num = fd_c;
	    }
            else{
                //inserisco il descrittore nella coda delle richieste
                Push(fd);
                //metto a zero il descrittore nella maschera dei descrittori
                FD_CLR(fd,&set);
                //aggiorno l'indice del descrittore maggiore
	        fd_num = UpdateMax(&set, fd_num);
            }
        } 
      } 
    }
  }
  //inserisco -1 nella coda per far terminare i vari thread
  Push(-1);
  //chiudo il socket
  SYSCALL2(notused, close(fd_sk), "close");
  return (void*) NULL;
}

/**
 * @function gestore
 * @brief funzione usata per gestire i segnali che devono terminare il server
 * @param signum indica il segnale ricevuto
 */
static void gestore(int signum){
  //setto a 1 la variabile globale che mi indica se far terminare il server o meno
  fine=1;
} 

/**
 * @function gestore2
 * @brief Funzione usata per la gestione dei segnali che non devono terminare il server
 * @param signum indica il segnale ricevuto
 */
static void gestore2(int signum){
  fine=2;
} 

/**
 * @function exec_sigaction
 * @brief Funzione usata per la gestione dei segnali
 */
void exec_sigaction(){
  sigset_t set;
  //maschero tutti i segnali finchè i gestori non sono stati installati
  sigfillset(&set);
  pthread_sigmask(SIG_SETMASK,&set,NULL);
  struct sigaction s;
  //resetto la struttura
  memset(&s,0,sizeof(s));
  //imposto il gestore per i segnali che devono terminare il server
  s.sa_handler=gestore;
  //imposto i segnali da collegare al gestore
  SYSCALL2(notused, sigaction(SIGINT,&s,NULL), "sigaction");
  SYSCALL2(notused, sigaction(SIGQUIT,&s,NULL), "sigaction");
  SYSCALL2(notused, sigaction(SIGTERM,&s,NULL), "sigaction");
  //imposto il gestore per i segnali che non devono terminare il server
  s.sa_handler=gestore2;
  //imposto i segnali da collegare al gestore
  SYSCALL2(notused, sigaction(SIGUSR1,&s,NULL), "sigaction");
  //ignoro SIGPIPE
  s.sa_handler=SIG_IGN;
  SYSCALL2(notused, sigaction(SIGPIPE,&s,NULL), "sigaction");
  //tolgo la maschera
  sigemptyset(&set);
  pthread_sigmask(SIG_SETMASK,&set,NULL);
}

/**
 * @function main
 * @brief Funzione principale in cui vengono richiamate le funzioni per la configuarazione e la gestione del server
 */
int main(int argc,char **argv){
  exec_sigaction(); //richiamo la funzione per la gestione dei segnali
  Parser(argv[2]); //libero la memoria allocata per la hash degli utenti
  CreateHash(threadsinpool, maxhistmsgs, maxmsgsize); //creo la hash per gli utenti e i relativi messaggi
  CreateHash_G(threadsinpool); //creo la hash per i gruppi
  pthread_t master, *workers;
  SYSCALL_D(workers, malloc(sizeof(pthread_t)*threadsinpool), "malloc");
  pthread_create(&master, NULL, Listener, NULL); //mando in esecuzione il thread Listener
  for(int i=0;i<threadsinpool;i++)
    pthread_create(&workers[i], NULL, Worker, NULL); //mando in esecuzione i thread Worker
  pthread_join(master,NULL); //aspetto la terminazione del thread Listener
  for(int i=0;i<threadsinpool;i++)
    pthread_join(workers[i],NULL); //aspetto la terminazione dei thread Worker
  free(coda); //libero la memoria allocata per la coda
  free(workers); //libero la memoria allocata per i workers
  DestroyHash_G(); //libero la memoria allocata per la hash dei gruppi
  DestroyList(); //libero la memoria allocata per la lista degli utenti online
  DestroyHash(); //libero la memoria allocata per la hash degli utenti
  free(unixpath);
  free(dirName);
  free(statfilename);
  return 0;
} 
