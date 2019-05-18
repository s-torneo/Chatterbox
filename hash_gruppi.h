/**
 * @file hash_gruppi.h
 * @brief File per l'implementazione della hash dei gruppi e le varie funzionalità
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#include <message.h>

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
 * @function hash_pjw2
 * @brief Calcola la funzione hash
 * @param key indica la chiave su cui calcolare la funzione hash
 * @return ritorna un intero che rappresenta il valore della funzione hash calcolata
 */
unsigned int hash_pjw2(void* key);

/**
 * @function CreateHash_G
 * @brief Crea la struttura hash
 * @param nzone indica il numero di zone per la divisione della hash
 */
void CreateHash_G(int nzone);

/**
 * @function Search_G
 * @brief Cerca il gruppo all'interno della hash
 * @param nome indica il nome del gruppo da cercare
 * @return ritorna un puntatore di tipo Hash se trova il gruppo, altrimenti NULL
 */
Hash_g * Search_G(char *nome);

/**
 * @function Insert_G
 * @brief Inserisce l'utente all'interno dell'hash
 * @param nome indica il nome del gruppo
 * @param user indica il nome dell'utente da inserire
 */
void Insert_G(char *nome, char *user);

/**
 * @function FreeAll_G
 * @brief Dealloca delle variabili della struttura hash
 * @param curr indica il puntatore alle variabili da deallocare
 */
void FreeAll_G(Hash_g *curr);
  
/**
 * @function Delete_G
 * @brief Elimina il gruppo dall'hash
 * @param nome indica il nome del gruppo da eliminare
 */
void Delete_G(char *nome);

/**
 * @function DestroyHash_G
 * @brief Elimina la struttura hash
 */
void DestroyHash_G();

/**
 * @function SearchUser
 * @brief Cerca l'utente all'interno del gruppo
 * @var user indica il nome dell'utente da cercare
 * @var l puntatore al gruppo in cui cercare l'utente
 * @return 1 se l'utente è stato trovato, 0 altrimenti
 */
int SearchUser(char *user, Hash_g *l);

/**
 * @function NewUser
 * @brief Inserisce l'utente all'interno del gruppo
 * @var nome indica il nome del gruppo
 * @var user indica il nome dell'utente da inserire
 * @return 1 se l'utente è stato inserito, 0 altrimenti
 */
int NewUser(char *nome, char *user);

/**
 * @function DeleteUser
 * @brief Elimina l'utente dal gruppo e cancella il gruppo se non rimangono più utenti iscritti
 * @var nome indica il nome del gruppo
 * @var user indica il nome dell'utente da eliminare
 * @return 1 se l'utente è stato inserito, -1 se si è raggiunto il massimo numero di iscritti o 0 altrimenti
 */
int DeleteUser(char *nome, char *user);

/**
 * @function DeleteGroup
 * @brief Esegue gli opportuni controlli e se non ci sono errori e richiama la funzione che cancella un gruppo se esiste
 * @param fd indica il descrittore del client che ha fatto la richiesta di cancellazione di un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1 se il gruppo esiste ed è stato cancellato, 0 altrimenti
 */
int DeleteGroup(long fd, message_t *msg);

/**
 * @function FindGroup
 * @brief Invia un messaggio di tipo op ad un gruppo se esiste
 * @param fd indica il descrittore del client che ha fatto la richiesta di inviare un messaggio di tipo op ad un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @param op indica il tipo di messaggio
 * @return 1 se il gruppo esiste e il messaggio è stato inviato, 0 altrimenti
 */
int FindGroup(long fd, message_t *msg, op_t op);

/**
 * @function CreaGruppo
 * @brief Esegue gli opportuni controlli e se non ci sono errori richiama la funzione che crea un gruppo
 * @param fd indica il descrittore del client che vuole creare un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int CreaGruppo(long fd, message_t *msg);

/**
 * @function AggiungiAlGruppo
 * @brief Esegue gli opportuni controlli e se non ci sono errori richiama la funzione che aggiunge un utente ad un gruppo
 * @param fd indica il descrittore del client che vuole aggiungersi ad un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int AggiungiAlGruppo(long fd, message_t *msg);

/**
 * @function EliminaDalGruppo
 * @brief Esegue gli opportuni controlli e se non ci sono errori richiama la funzione che elimina l'utente dal gruppo
 * @param fd indica il descrittore del client che si vuole eliminare da un gruppo
 * @param msg puntatore per l'accesso ai campi della struttura message_t
 * @return 1
 */
int EliminaDalGruppo(long fd, message_t *msg);