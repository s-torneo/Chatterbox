/**
 * @file hash_gruppi.c
 * @brief File per l'implementazione della hash dei gruppi e le varie funzionalità
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
#include <hash_history.h>

//dimensione della hash
#define DIM_HASH 1024

//massimo numero di utenti in ogni gruppo
#define max_users_group 32

//macro per allocazioni dinamiche
#define SYSCALL_D(r,c,e) \
    if((r=c)==NULL) { perror(e); exit(-1); }

/**
 * @struct node_g
 * @brief è la struttura che rappresenta la hash
 * @var nome indica il nome del gruppo
 * @var utente indica la lista degli utenti che fanno parte di un gruppo
 * @var n_utenti è il numero di utenti di un gruppo
 * @var next puntatore all'elemento successivo
 */
typedef struct node_g{
  char *nome;
  char **utente;
  int n_utenti;
  struct node_g *next;
}Hash_g;

/**
 * @var G rappresenta il puntatore alla struttura hash
 */
Hash_g **G;

/**
 * @var last rappresenta il puntatore alla fine della struttura hash
 */
Hash_g *last=NULL;

/**
 * @var zone_g indica il numero di zone che dividono l'hash
 */
int zone_g=0;

/**
 * @var mutex5 è la variabile per la gestione della mutua-esclusione
 */
pthread_mutex_t *mutex5;

/**
 * @function hash_pjw2
 * @brief Calcola la funzione hash
 * @param key indica la chiave su cui calcolare la funzione hash
 * @return ritorna un intero che rappresenta il valore della funzione hash calcolata
 */
unsigned int hash_pjw2(void* key){
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
 * @function CreateHash_G
 * @brief Crea la struttura hash
 * @param nzone indica il numero di zone per la divisione della hash
 */
void CreateHash_G(int nzone){
  SYSCALL_D(G, malloc(sizeof(Hash_g*)*(DIM_HASH)), "malloc");
  //inizializzo la variabile G che rappresenta la struttura hash
  for(int i=0;i<DIM_HASH;i++)
    G[i]=NULL;
  zone_g=nzone;
  //alloco la variabile mutex
  SYSCALL_D(mutex5, malloc(sizeof(pthread_mutex_t)*zone_g), "malloc");
  //inizializzo ogni elemento dell'array di mutex
  for(int i=0;i<zone_g;i++)
    pthread_mutex_init(&(mutex5[i]),NULL);
}

/**
 * @function Search_G
 * @brief Cerca il gruppo all'interno della hash
 * @param nome indica il nome del gruppo da cercare
 * @return ritorna un puntatore di tipo Hash se trova il gruppo, altrimenti NULL
 */
Hash_g * Search_G(char *nome){
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome del gruppo
  int key=hash_pjw2(nome);
  //prendo la mutua-esclusione
  pthread_mutex_lock(&mutex5[key%zone_g]);
  //prendo il puntatore alla lista di trabocco corretta
  Hash_g *l=G[key];
  //scorro la lista e confronto il nome di ogni gruppo con quello da cercare
  while(l!=NULL){
    //se lo trovo
    if(!strcmp(l->nome,nome)){
      //rilascio la mutua-esclusione
      pthread_mutex_unlock(&mutex5[key%zone_g]); 
      return l;
    }
    l=l->next;
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex5[key%zone_g]);
  return NULL;
}

/**
 * @function Insert_G
 * @brief Inserisce l'utente all'interno dell'hash
 * @param nome indica il nome del gruppo
 * @param user indica il nome dell'utente da inserire
 */
void Insert_G(char *nome, char *user){
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome del gruppo
  int key=hash_pjw2(nome);
  //prendo la mutua-esclusione
  pthread_mutex_lock(&mutex5[key%zone_g]);
  Hash_g *new;
  //inizializzo i campi della struttura hash
  SYSCALL_D(new, malloc(sizeof(Hash_g)), "malloc");
  new->next=NULL;
  SYSCALL_D(new->nome, malloc(sizeof(char)*(MAX_NAME_LENGTH+1)), "malloc");
  strncpy(new->nome,nome, (MAX_NAME_LENGTH+1));
  SYSCALL_D(new->utente, calloc(max_users_group,sizeof(char*)), "calloc");
  for(int i=0;i<max_users_group;i++)
    SYSCALL_D(new->utente[i], calloc((MAX_NAME_LENGTH+1),sizeof(char)), "calloc");
  strncpy(new->utente[0],user, (MAX_NAME_LENGTH+1));
  new->n_utenti=1;
  if(G[key]==NULL){
    G[key]=new;
    last=new;
  } 
  else{
    last->next=new;
    last=new;
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex5[key%zone_g]);
}

/**
 * @function FreeAll_G
 * @brief Dealloca delle variabili della struttura hash
 * @param curr indica il puntatore alle variabili da deallocare
 */
void FreeAll_G(Hash_g *curr){
  for(int i=0;i<max_users_group;i++)
    free(curr->utente[i]);
  free(curr->utente);
  free(curr->nome);
  free(curr);
}

/**
 * @function Delete_G
 * @brief Elimina il gruppo dall'hash
 * @param nome indica il nome del gruppo da eliminare
 */
void Delete_G(char *nome){
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome del gruppo
  int key=hash_pjw2(nome);
  //prendo la mutua-esclusione
  pthread_mutex_lock(&mutex5[key%zone_g]);
  //prendo il puntatore alla lista di trabocco corretta
  Hash_g *curr=G[key],*prec=NULL,*tmp=NULL;
  //scorro la lista e se trovo il gruppo allora lo elimino
  while(curr!=NULL){
    if(!strcmp(curr->nome,nome)){
      if(prec==NULL){
        G[key]=curr->next;
        FreeAll_G(curr);
        pthread_mutex_unlock(&mutex5[key%zone_g]);
        return;
      }
      else{
        tmp=curr;
        prec->next=curr->next;
        FreeAll_G(tmp);
        curr=prec->next;    
        pthread_mutex_unlock(&mutex5[key%zone_g]);
        return;
      }
    }
    else{prec=curr;curr=curr->next;}
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex5[key%zone_g]);
}

/**
 * @function DestroyHash_G
 * @brief Elimina la struttura hash
 */
void DestroyHash_G(){
  if(G==NULL) return;
  Hash_g *curr,*tmp;
  for(int i=0; i<DIM_HASH; i++){
    curr = G[i]; tmp=NULL;
    while(curr!=NULL){
      tmp=curr;
      curr=curr->next;
      FreeAll_G(tmp);
    } 
  }
  free(mutex5);
  if(G!=NULL) free(G);
}

/**
 * @function SearchUser
 * @brief Cerca l'utente all'interno del gruppo
 * @var user indica il nome dell'utente da cercare
 * @var l puntatore al gruppo in cui cercare l'utente
 * @return 1 se l'utente è stato trovato, 0 altrimenti
 */
int SearchUser(char *user, Hash_g *l){
  if(l==NULL)return 0;
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome del gruppo
  int key=hash_pjw2(l->nome);
  //prendo la mutua-esclusione
  pthread_mutex_lock(&mutex5[key%zone_g]);
  //cerco l'utente nel gruppo
  for(int i=0;i<l->n_utenti;i++){
    //se lo trovo
    if(!strcmp(user,l->utente[i])){
      //rilascio la mutua-esclusione
      pthread_mutex_unlock(&mutex5[key%zone_g]);
      return 1;
    }
  }
  //rilascio la mutua-esclusione
  pthread_mutex_unlock(&mutex5[key%zone_g]);
  return 0;
}

/**
 * @function NewUser
 * @brief Inserisce l'utente all'interno del gruppo
 * @var nome indica il nome del gruppo
 * @var user indica il nome dell'utente da inserire
 * @return 1 se l'utente è stato inserito, -1 se si è raggiunto il massimo numero di iscritti o 0 altrimenti
 */
int NewUser(char *nome, char *user){
   //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome del gruppo
  int key=hash_pjw2(nome);
  //cerco il gruppo tramite il nome e mi faccio restituire il puntatore alla lista
  Hash_g *l=Search_G(nome);
  //controllo se si è raggiunto il massimo numero di iscritti
  if(l->n_utenti==max_users_group)
    return -1;
  //cerco l'utente all'interno del gruppo e se non lo trovo allora lo aggiungo alla lista degli utenti
  if(!SearchUser(user,l)){
    //prendo la mutua-esclusione
    pthread_mutex_lock(&mutex5[key%zone_g]);
    //copio l'utente nell'array degli utenti di quel gruppo
    strncpy(l->utente[l->n_utenti],user,(MAX_NAME_LENGTH+1));
    //incremento il numero degli utenti
    l->n_utenti++;
    //rilascio la mutua-esclusione
    pthread_mutex_unlock(&mutex5[key%zone_g]);
    return 1;
  }
  return 0;
}

/**
 * @function DeleteUser
 * @brief Elimina l'utente dal gruppo e cancella il gruppo se non rimangono più utenti iscritti
 * @var nome indica il nome del gruppo
 * @var user indica il nome dell'utente da eliminare
 * @return 1 se l'utente viene rimosso, 0 altrimenti
 */
int DeleteUser(char *nome, char *user){
  //cerco il gruppo tramite il nome e mi faccio restituire il puntatore alla lista
  Hash_g *l=Search_G(nome);
  //mi faccio restituire la chiave calcolata dalla funzione dandogli come parametro il nome del gruppo
  int key=hash_pjw2(l->nome);
  //prendo la mutua-esclusione
  pthread_mutex_lock(&mutex5[key%zone_g]);
  int i=0, trovato=0;
  //cerco l'utente all'interno della lista del gruppo
  while(!trovato){
    if(!strcmp(user,l->utente[i])){
      trovato=1;
      //elimino l'utente shiftando le posizioni dell'array
      for(;i<l->n_utenti-1;i++)
        strcpy(l->utente[i],l->utente[i+1]);
      strcpy(l->utente[l->n_utenti-1],"");
      //decremento il numero di utenti del gruppo
      l->n_utenti--;
    }
    else i++;
  }
  //se non ci sono più utenti in quel gruppo allora cancello il gruppo
  if(!l->n_utenti){
    pthread_mutex_unlock(&mutex5[key%zone_g]);
    Delete_G(nome);
  }
  else
    //rilascio la mutua-esclusione
    pthread_mutex_unlock(&mutex5[key%zone_g]);
  return trovato;
}

/**
 * @function DeleteGroup
 * @brief Esegue gli opportuni controlli e se non ci sono errori e richiama la funzione che cancella un gruppo se esiste
 * @param fd indica il descrittore del client che ha fatto la richiesta di cancellazione di un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1 se il gruppo esiste ed è stato cancellato, 0 altrimenti
 */
int DeleteGroup(long fd, message_t *msg){
  //cerco se esiste il gruppo
  Hash_g *l=Search_G(msg->data.hdr.receiver);
  if(l!=NULL){
    //se esiste allora vedo se chi ha fatto richiesta di deregistrazione corrisponde con chi ha creato il gruppo
    if(!strcmp(msg->hdr.sender,l->utente[0])){
      //se corrisponde allora invio un messaggio di ok al client
      SendHdr_mutex(fd, &(msg->hdr), OP_OK);
      //elimino il gruppo
      Delete_G(l->nome);
    }
    else{
      //se chi ha fatto richiesta di cancellazione del gruppo non è il creatore allora invio un messaggio di errore
      SendHdr_mutex(fd, &(msg->hdr), OP_FAIL);
    }
    return 1;
  }
  return 0;
}

/**
 * @function FindGroup
 * @brief Invia un messaggio di tipo op ad un gruppo se esiste
 * @param fd indica il descrittore del client che ha fatto la richiesta di inviare un messaggio di tipo op ad un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @param op indica il tipo di messaggio
 * @return 1 se il gruppo esiste e il messaggio è stato inviato, 0 altrimenti
 */
int FindGroup(long fd, message_t *msg, op_t op){
  //cerco il gruppo
  Hash_g *l=Search_G(msg->data.hdr.receiver);
  //cerco l'utente nel gruppo
  if(SearchUser(msg->hdr.sender,l)){
    //aggiungo il file alla history di tutti gli utenti del gruppo
    AddtoAll_G(l->utente,l->n_utenti,msg,op);
    //invio il messaggio agli utenti online appartenenti al gruppo
    for(int i=0;i<l->n_utenti;++i){
      //ottengo il descrittore dell'utente
      long fd2=GetFd(l->utente[i]);
      //invio il messaggio
      SendMsg_mutex(fd2,msg,op);
    }
    return 1;
  }
  return 0;
}

/**
 * @function CreaGruppo
 * @brief Esegue gli opportuni controlli e se non ci sono errori richiama la funzione che crea un gruppo
 * @param fd indica il descrittore del client che vuole creare un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int CreaGruppo(long fd, message_t *msg){
  //controlla se il gruppo esiste già
  if(Search_G(msg->data.hdr.receiver)!=NULL){
    //se esiste allora invia un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_NICK_ALREADY);
  }
  else{
    //altrimenti invia un messaggio di ok
    SendHdr_mutex(fd, &(msg->hdr), OP_OK);
    //inserisce il gruppo nella hash dei gruppi
    Insert_G(msg->data.hdr.receiver, msg->hdr.sender);
  }
  return 1;
}

/**
 * @function AggiungiAlGruppo
 * @brief Esegue gli opportuni controlli e se non ci sono errori richiama la funzione che aggiunge un utente ad un gruppo
 * @param fd indica il descrittore del client che vuole aggiungersi ad un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int AggiungiAlGruppo(long fd, message_t *msg){
  //controlla se il gruppo a cui il client vuole aggiungersi esiste
  if(Search_G(msg->data.hdr.receiver)==NULL)
    //se non esiste allora invia un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_FAIL);
  else{
    //provo ad aggiungere l'utente al gruppo
    int res=NewUser(msg->data.hdr.receiver, msg->hdr.sender);
    if(!res)
      //se è già presente allora invia un messaggio di errore
      SendHdr_mutex(fd, &(msg->hdr), OP_NICK_ALREADY);
    else if(res<0)
      //se si è raggiunto il numero massimo di iscritti allora invia un messaggio di errore
      SendHdr_mutex(fd, &(msg->hdr), OP_FAIL);
    else
      //altrimenti invia un messaggio di ok
      SendHdr_mutex(fd, &(msg->hdr), OP_OK);
  }
  return 1;
}

/**
 * @function EliminaDalGruppo
 * @brief Esegue gli opportuni controlli e se non ci sono errori richiama la funzione che elimina l'utente dal gruppo
 * @param fd indica il descrittore del client che si vuole eliminare da un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int EliminaDalGruppo(long fd, message_t *msg){
  //controllo che il gruppo esista
  if(Search_G(msg->data.hdr.receiver)==NULL){
    //se non esiste allora invio un messaggio di errore al client
    SendHdr_mutex(fd, &(msg->hdr), OP_FAIL);
    return 1;
  }
  else{
    //elimino l'utente se è presente nel gruppo
    if(DeleteUser(msg->data.hdr.receiver, msg->hdr.sender))
      //e invio un messaggio di ok
      SendHdr_mutex(fd, &(msg->hdr), OP_OK);
    else
      //altrimenti invio un messaggio di errore
      SendHdr_mutex(fd, &(msg->hdr), OP_FAIL);
  }
  return 1;
}