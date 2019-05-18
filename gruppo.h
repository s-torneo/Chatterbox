/**
 * @file group.h
 * @brief File per l'implementazione delle funzionalità dei gruppi
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

/**
 * @struct node_g
 * @brief è la struttura che rappresenta la hash
 * @var nome indica il nome del gruppo
 * @var utente indica la lista degli utenti che fanno parte di un gruppo
 * @var n_utenti è il numero di utenti di un gruppo
 * @var next puntatore all'elemento successivo
 */
/**
 * @function DeleteGroup
 * @brief Cancella un gruppo se esiste
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
 * @brief Crea un gruppo
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
 * @brief Aggiunge un utente ad un gruppo
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
 * @brief Elimina un utente da un gruppo
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