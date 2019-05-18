/**
 * @file hash_history.c
 * @brief File per l'implementazione della hash e della history
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#include<limits.h>
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <connections.h>
#include <online.h>
#include <message.h>

//macro per allocazioni dinamiche
#define SYSCALL_D(r,c,e) \
    if((r=c)==NULL) { perror(e); exit(-1);}

#define DIM_HASH 1024

/**
 * @struct History
 * @brief è la struttura che rappresenta la history
 * @var msg è un puntatore alla struttura message_t
 * @var consegnato indica se un messaggio è stato consegnato o meno
 */
typedef struct History{
  message_t *msg;
  int consegnato;
}Hist;

/**
 * @struct node
 * @brief è la struttura che rappresenta l'hash
 * @var H è un puntatore alla stuttura History
 * @var nickname indica il nome dell'utente
 * @var cont indica il numero dei messaggi presenti nella history di un utente
 * @var start è una variabile che indica l'inizio della history di un utente
 * @var end è una variabile che indica la fine della history di un utente
 * @var controllo è una variabile usata per la gestione della history
 * @var next è il puntatore all'elemento successivo
 */
typedef struct node{
  Hist *H; 
  char *nickname;
  int cont;
  int start;
  int end;
  int controllo;
  struct node *next;
}Hash;


/**
 * @var T è il puntatore dell'hash
 */
Hash **T;

/**
 * @var zone indica il numero di zone che dividono l'hash
 */
int zone=0;

/**
 * @var mutex è la variabile per la gestione della mutua-esclusione
 */
pthread_mutex_t *mutex3;

/**
 * @var maxhistmsgs indica il numero massimo di messaggi nella history di ogni utente
 * @var maxmsgsize indica la dimensione massima di un messaggio
 */
int maxhistmsgs, maxmsgsize;

/**
 * @function CreateHash
 * @brief Crea la struttura hash
 * @param nzone indica il numero di zone per la divisione della hash
 * @param maxhist intero che indica il numero massimo di messaggi nella history di ogni utente
 * @param maxmsg intero che indica la dimensione massima di un messaggio
 */
void CreateHash(int nzone, int maxhist, int maxmsg){
  maxhistmsgs=maxhist;
  maxmsgsize=maxmsg;
  SYSCALL_D(T,malloc(sizeof(Hash*)*(DIM_HASH)), "malloc");
  //inizializzo la variabile T che rappresenta la struttura hash
  for(int i=0;i<DIM_HASH;i++)
    T[i]=NULL;
  zone=nzone;
  //alloco la variabile di mutex
  SYSCALL_D(mutex3, malloc(sizeof(pthread_mutex_t)*zone) , "malloc");
  //inizializzo ogni elemento dell'array di mutex
  for(int i=0;i<zone;i++)
    pthread_mutex_init(&(mutex3[i]),NULL);
}

/**
 * @function hash_pjw
 * @brief Calcola la funzione hash
 * @param key indica la chiave su cui calcolare la funzione hash
 * @return ritorna un intero che rappresenta il valore della funzione hash calcolata
 */
unsigned int hash_pjw(void* key){
  char *datum = (char *)key;
  unsigned int hash_value, i;
  if(!datum) return 0;
  for(hash_value = 0; *datum; ++datum) {
    hash_value = (hash_value << ONE_EIGHTH) + *datum;
    if ((i = hash_value & HIGH_BITS) != 0)
      hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
  return (hash_value)%DIM_HASH;
}

/**
 * @function Search
 * @brief Cerca l'utente all'interno della hash
 * @param utente indica il nome dell'utente da cercare
 * @return ritorna un puntatore di tipo Hash se trova l'utente, altrimenti NULL
 */
Hash * Search(char *utente){
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome dell'utente
  int key=hash_pjw(utente);
  //prendo la lock per eseguire il codice in mutua-esclusione
  pthread_mutex_lock(&mutex3[key%zone]);
  Hash *l=T[key];
  //scorro la lista di trabocco individuata tramite la chiave (key) calcolata
  while(l!=NULL){
    //controllo se un utente di quella lista corrisponde all'utente che si sta cercando, se sì allora ritorno il puntatore e rilascio la mutua-esclusione
    if(!strcmp(l->nickname,utente)){
      pthread_mutex_unlock(&mutex3[key%zone]); 
      return l;
    }
    l=l->next;
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex3[key%zone]);
  return NULL;
}

/**
 * @function Inserisci
 * @brief Inserisce l'utente all'interno dell'hash
 * @param utente indica il nome dell'utente da inserire
 */
void Insert(char *utente){
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome dell'utente
  int key=hash_pjw(utente);
  //prendo la lock per eseguire il codice in mutua-esclusione
  pthread_mutex_lock(&mutex3[key%zone]);
  Hash *new=malloc(sizeof(Hash));
  new->next=NULL;
  SYSCALL_D(new->H,malloc(sizeof(Hist)*maxhistmsgs), "malloc");
  SYSCALL_D(new->nickname,malloc(sizeof(char)*(MAX_NAME_LENGTH+1)), "malloc");
  strncpy(new->nickname,utente,(MAX_NAME_LENGTH+1));
  new->start=0;
  new->end=0;
  new->controllo=0;
  new->cont=0;
  for(int i=0;i<maxhistmsgs;i++){
    SYSCALL_D(new->H[i].msg,calloc(1,sizeof(message_t)), "calloc");
    SYSCALL_D(new->H[i].msg->data.buf,malloc(sizeof(char)*maxmsgsize), "malloc");
    new->H[i].consegnato=0;
  }
  if(T[key]==NULL)
    T[key]=new; 
  else{
    new->next=T[key];
    T[key]=new;
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex3[key%zone]);
}

/**
 * @function FreeAll_H
 * @brief Dealloca delle variabili della struttura history e hash
 * @param curr è un puntatore alle variabili da deallocare
 */
void FreeAll_H(Hash *curr){
  for(int i=0;i<maxhistmsgs;i++){
    free(curr->H[i].msg->data.buf);
    free(curr->H[i].msg);
  }
  free(curr->H);
  free(curr->nickname);
  free(curr);
}

/**
 * @function Delete
 * @brief Elimina l'utente dall'hash
 * @param utente indica il nome dell'utente da eliminare
 */
void Delete(char *utente){
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome dell'utente
  int key=hash_pjw(utente);
  //prendo la lock per eseguire il codice in mutua-esclusione
  pthread_mutex_lock(&mutex3[key%zone]);
  Hash *curr=T[key],*prec=NULL,*tmp=NULL;
  //cerco l'utente nella struttura Hash, se lo trovo allora lo elimino
  while(curr!=NULL){
    if(!strcmp(curr->nickname,utente)){
      if(prec==NULL){
        T[key]=curr->next;
        FreeAll_H(curr);
        pthread_mutex_unlock(&mutex3[key%zone]);
        return;
      }
      else{
        tmp=curr;
        prec->next=curr->next;
        FreeAll_H(tmp);
        curr=prec->next;    
        pthread_mutex_unlock(&mutex3[key%zone]);
        return;
      }
    }
    else{prec=curr;curr=curr->next;}
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex3[key%zone]);
}

/**
 * @function DestroyHash
 * @brief Elimina la struttura hash
 */
void DestroyHash(){
  if(T==NULL) return;
  Hash *curr,*tmp;
  for(int i=0; i<DIM_HASH; i++){
    curr = T[i]; tmp=NULL;
    while(curr!=NULL){
      tmp=curr;
      curr=curr->next;
      FreeAll_H(tmp);
    } 
  }
  free(mutex3);
  if(T!=NULL) free(T);
}

/**
 * @function Add_H
 * @brief Aggiunge un messaggio all'interno della history
 * @param msg è un puntatore di tipo message_t
 */
void Add_H(message_t *msg, op_t op){
  //cerco il puntatore alla history dell'utente passato come parametro
  Hash *l=Search(msg->data.hdr.receiver);
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome dell'utente
  int key=hash_pjw(msg->data.hdr.receiver);
  //prendo la lock per eseguire il codice in mutua-esclusione
  pthread_mutex_lock(&mutex3[key%zone]);
  Hist h2 = l->H[l->end];
  //controllo se i "puntatori" start ed end siano uguali e che la variabile controllo sia non 0, in tal caso incremento il contatore start
  //la variabile controllo mi serve a non far avanzare start la prima volta che start ed end puntano alla stessa posizione
  if((l->start==l->end)&&(l->controllo))l->start=(l->start+1)%maxhistmsgs;
  strncpy(h2.msg->hdr.sender,msg->hdr.sender,(MAX_NAME_LENGTH+1));
  strncpy(h2.msg->data.buf,msg->data.buf, maxmsgsize);
  h2.msg->hdr.op=op;
  if(l->cont<maxhistmsgs)
    l->cont++;
  l->end=(l->end+1)%maxhistmsgs;
  if(!l->end)l->controllo=1;
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex3[key%zone]);
} 

/**
 * @function AddtoAll_H
 * @brief Aggiunge un messaggio alla history di tutti gli utenti, eccetto di chi l'ha inviato
 * @param msg è un puntatore di tipo message_t
 * @param lista è un array di stringhe da riempire con i nomi degli utenti a cui inviare il messaggio
 * @return il numero degli utenti a cui è stato inviato il messaggio
 */
int AddtoAll_H(message_t *msg, char **lista){
  int cont=0;
  //scorro tutta la hash
  for(int i=0;i<DIM_HASH;i++){
    Hash *l=T[i];
    //prendo la mutua-esclusione
    pthread_mutex_lock(&mutex3[i%zone]);
    while(l!=NULL){
      Hist h2 = l->H[l->end];
      if(strcmp(l->nickname,msg->hdr.sender)){
        if((l->start==l->end)&&(l->controllo))l->start=(l->start+1)%maxhistmsgs;
        strncpy(h2.msg->hdr.sender,msg->hdr.sender, (MAX_NAME_LENGTH+1));
  	strncpy(h2.msg->data.buf,msg->data.buf, maxmsgsize);	
        strncpy(lista[cont],l->nickname, (MAX_NAME_LENGTH+1));
	cont++;
        h2.msg->hdr.op=TXT_MESSAGE;
        if(l->cont<maxhistmsgs)
          l->cont++;
        l->end=(l->end+1)%maxhistmsgs;
        if(!l->end)l->controllo=1;
      }
      l=l->next;
    }
    //rilascio la mutua-esclusione
    pthread_mutex_unlock(&mutex3[i%zone]);
  }
  return cont;
}

/**
 * @function AddtoAll_G
 * @brief Aggiunge un messaggio alla history di tutti gli utenti appartenenti al gruppo
 * @param lista contiene l'elenco degli utenti appartenenti al gruppo
 * @param nutenti indica il numero di utenti appartenenti al gruppo
 * @param msg è un puntatore di tipo message_t
 */
void AddtoAll_G(char **lista, int nutenti, message_t *msg, op_t op){
  for(int i=0;i<nutenti;i++){
    //mi faccio restituire la chiave
    int key=hash_pjw(lista[i]);
    //prendo la mutua-esclusione
    pthread_mutex_lock(&mutex3[key%zone]);
    Hash *l=T[key];
    //scorro la lista che punta alla history dell'utente del gruppo
    while(l!=NULL){
      Hist h2 = l->H[l->end];
      //se trovo l'utente nella lista allora copio nelle variabili della history le informazioni prese dalle variabili della struttura message_t
      if(!strcmp(l->nickname,lista[i])){
        if((l->start==l->end)&&(l->controllo))l->start=(l->start+1)%maxhistmsgs;
        strncpy(h2.msg->hdr.sender,msg->hdr.sender,(MAX_NAME_LENGTH+1));
        strncpy(h2.msg->data.buf,msg->data.buf, maxmsgsize);	
        h2.msg->hdr.op=op;
        if(l->cont<maxhistmsgs)
          l->cont++;
        l->end=(l->end+1)%maxhistmsgs;
        if(!l->end)l->controllo=1;
      }
      l=l->next;
    }
    //rilascio la mutua-esclusione
    pthread_mutex_unlock(&mutex3[key%zone]);
  }
}

/**
 * @function GetHistory
 * @brief Invia la lista dei messaggi ricevuti da un utente
 * @param fd indica il descrittore
 * @param msg è un puntatore di tipo message_t per accedere ai vari campi della struttura
 * @param file_inviati è una variabile che conterrà il numero di file consegnati, conteggiati all'interno della funzione
 * @return ritorna il numero dei messaggi consegnati
 */
int GetHistory(long fd, message_t *msg, int *file_consegnati){
  //cerco l'utente
  Hash *l=Search(msg->hdr.sender);
  //mi faccio restituire la chiave
  int key=hash_pjw(msg->hdr.sender);
  //prendo la mutua-esclusione
  pthread_mutex_lock(&mutex3[key%zone]);
  int cont;
  if(l==NULL){
    cont=0;
    msg->data.hdr.len=cont;
  }
  else{
    cont=l->cont;
    msg->data.hdr.len=sizeof(size_t);
  }
  msg->data.buf=calloc(msg->data.hdr.len,sizeof(char));
  if(msg->data.buf==NULL)
    return 0;
  snprintf(msg->data.buf,msg->data.hdr.len,"%s",(char*)&cont);
  //invio i dati
  SendData_mutex(fd,&(msg->data));
  free(msg->data.buf);
  int mex_consegnati=0, i=l->start;
  Hist *h2 = l->H;
  //itero mentre j è minore del numero di messaggi presenti nella history dell'utente
  for(int j=0;j<cont;++j){
      //copio le variabili della struttura history nelle variabili delle struttura message_t
      msg->data.hdr.len=maxmsgsize;
      msg->data.buf=malloc(msg->data.hdr.len*sizeof(char));
      if(msg->data.buf==NULL)
        return -1;
      strncpy(msg->hdr.sender,h2[i].msg->hdr.sender, (MAX_NAME_LENGTH+1));
      strncpy(msg->data.buf,h2[i].msg->data.buf,msg->data.hdr.len);
      //controlllo se il messaggio è di tipo file
      if(h2[i].msg->hdr.op==FILE_MESSAGE){
        //controllo se il messaggio non è stato inviato, in tal caso setto la variabile = 1 e aumento il contatore dei file inviati
        if(!h2[i].consegnato){
          h2[i].consegnato=1;
          (*file_consegnati)++;
        }
      }
      else{
      //se il messaggio è di tipo testuale allora controllo se il messaggio non è stato inviato, in tal caso setto la variabile = 1 e aumento il contatore dei messaggi testuali inviati
        if(!h2[i].consegnato){
          h2[i].consegnato=1;
          mex_consegnati++;
        }
      }
      //invio il messaggio
      SendMsg_mutex(fd,msg,h2[i].msg->hdr.op);
      free(msg->data.buf);
      i=(i+1)%maxhistmsgs;
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex3[key%zone]);
  return mex_consegnati;
}