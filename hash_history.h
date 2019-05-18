/**
 * @file hash_history.h
 * @brief File per l'implementazione della hash e della history
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#include <message.h>

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
 * @var H è un puntatore ai campi della stuttura History
 * @var nickname indica il nome dell'utente
 * @var cont indica il numero dei messaggi presenti nella history di un utente
 * @var start è una variabile che punta all'inizio della history di un utente
 * @var end è una variabile che punta alla fine della history di un utente
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
 * @function CreateHash
 * @brief Crea la struttura hash
 * @param nzone indica il numero di zone per la divisione della hash
 * @param maxhist intero che indica il numero massimo di messaggi nella history di ogni utente
 * @param maxmsg intero che indica la dimensione massima di un messaggio
 */
void CreateHash(int nzone, int maxhist, int maxmsg);

/**
 * @function hash_pjw
 * @brief Calcola la funzione hash
 * @param key indica la chiave su cui calcolare la funzione hash
 * @return ritorna un intero che rappresenta il valore della funzione hash calcolata
 */
unsigned int hash_pjw(void* key);

/**
 * @function Search
 * @brief Cerca l'utente all'interno della hash
 * @param utente indica il nome dell'utente da cercare
 * @return un puntatore di tipo Hash se trova l'utente, altrimenti NULL
 */
Hash * Search(char *utente);

/**
 * @function Insert
 * @brief Inserisce l'utente all'interno dell'hash
 * @param utente indica il nome dell'utente da inserire
 */
void Insert(char *utente);

/**
 * @function FreeAll_H
 * @brief Dealloca delle variabili della struttura history e hash
 * @param curr è un puntatore alle variabili da deallocare
 */
void FreeAll_H(Hash *curr);

/**
 * @function Delete
 * @brief Elimina l'utente dall'hash
 * @param utente indica il nome dell'utente da eliminare
 */
void Delete(char *utente);

/**
 * @function DestroyHash
 * @brief Elimina la struttura hash
 */
void DestroyHash();

/**
 * @function Add_H
 * @brief Aggiunge un messaggio all'interno della history
 * @param msg è un puntatore di tipo message_t
 * @param op indica il tipo di messaggio
 */
void Add_H(message_t *msg, op_t op);

/**
 * @function CreaLista
 * @brief Crea un array di stringhe
 * @return un puntatore che rappresenta l'array di stringhe
 */
char ** CreaLista();

/**
 * @function CancellaLista
 * @brief Elimina la memoria allocata per l'allocazione dell'array di stringhe
 */
void CancellaLista(char **lista);

/**
 * @function AddtoAll_H
 * @brief Aggiunge un messaggio alla history di tutti gli utenti, eccetto di chi l'ha inviato
 * @param msg è un puntatore di tipo message_t
 * @param lista è un array di stringhe da riempire con i nomi degli utenti a cui inviare il messaggio
 * @return il numero degli utenti a cui è stato inviato il messaggio
 */
int AddtoAll_H(message_t *msg, char **lista);

/**
 * @function AggiungiHAll_G
 * @brief Aggiunge un messaggio alla history di tutti gli utenti appartenenti al gruppo
 * @param lista contiene l'elenco degli utenti appartenenti al gruppo
 * @param nutenti indica il numero di utenti appartenenti al gruppo
 * @param msg è un puntatore di tipo message_t
 * @param op indica il tipo di messaggio
 */
void AddtoAll_G(char **lista, int nutenti, message_t *msg, op_t op);

/**
 * @function GetHistory
 * @brief Invia la lista dei messaggi ricevuti da un utente
 * @param fd indica il descrittore
 * @param msg è un puntatore di tipo message_t per accedere ai vari campi della struttura
 * @param file_inviati è una variabile che conterrà il numero di file consegnati, conteggiati all'interno della funzione
 * @return ritorna il numero dei messaggi consegnati
 */
int GetHistory(long fd, message_t *msg, int *file_consegnati);