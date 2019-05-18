/**
 * @file online.h
 * @brief File per l'implementazione della gestione degli utenti online
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#include <pthread.h>
#include <message.h>

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
 * @function DeleteOnline
 * @brief Elimina l'utente dalla lista degli online
 * @param fd indica il descrittore dell'utente da eliminare
 */
void DeleteOnline(long fd);

/**
 * @function SearchFd
 * @brief Cerca il descrittore all'interno della lista degli online
 * @param fd indica il descrittore da cercare
 * @return ritorna un puntatore di tipo Online se trova il descrittore, altrimenti NULL
 */
Online * SearchFd(long fd);

/**
 * @function ListaOnline
 * @brief Invia la lista degli utenti online
 * @param fd indica il descrittore dell'utente a cui inviare la lista
 * @param msg puntatore per accedere alla struttura message_t
 */
void ListaOnline(long fd, message_t *msg);

/**
 * @function PushOnline
 * @brief Aggiunge un utente alla lista degli utenti online
 * @param fd indica il descrittore dell'utente
 * @param msg puntatore per accedere alla struttura message_t
 * @return 0 se l'utente era già online, 1 altrimenti
 */
int PushOnline(long fd, message_t *msg);

/**
 * @function DestroyList
 * @brief Elimina la struttura degli utenti online
 */
void DestroyList();

/**
 * @function GetFd
 * @brief Cerca un utente
 * @param nick indica il nome dell'utente da cercare
 * @return ritorna il descrittore associato all'utente se lo trova, altrimenti -1
 */
long GetFd(char *nick);

/**
 * @function SendMsg_mutex
 * @brief Invia l'intero messaggio in mutua-esclusione
 * @param fd indica il descrittore
 * @param msg variabile tramite cui accedere ai campi della struttura message_t
 * @param op indica il tipo di operazione
 * @return 1 se l'operazione è andata a buon fine, 0 altrimenti
 */
int SendMsg_mutex(long fd, message_t *msg, int op);

/**
 * @function SendData_mutex
 * @brief Invia il body del messaggio in mutua-esclusione
 * @param fd indica il descrittore
 * @param msg variabile tramite cui accedere ai campi della struttura message_t
 */
void SendData_mutex(long fd, message_data_t *msg);

/**
 * @function SendHdr_mutex
 * @brief Invia l'header del messaggio in mutua-esclusione
 * @param fd indica il descrittore
 * @param msg variabile tramite cui accedere ai campi della struttura message_t
 * @param op indica il tipo di operazione
 */
void SendHdr_mutex(long fd, message_hdr_t *hdr, int op);