/**
 * @file rnwn.h
 * @brief File contenente le funzioni per la scrittura e la lettura dei dati
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore.
 */

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>

/**
 * @function readn
 * @brief Legge un dato di qualsiasi tipo
 * @param fd indica il descrittore
 * @param buf indica il dato da leggere
 * @param size indica la dimensione del dato da leggere
 * @return ritorna il numero di caratteri letti
 */
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // gestione chiusura socket
        left    -= r;
	bufptr  += r;
    }
    return size;
}

/**
 * @function writen
 * @brief Scrive un dato di qualsiasi tipo
 * @param fd indica il descrittore
 * @param buf indica il dato da scrivere
 * @param size indica la dimensione del dato da scrivere
 * @return 1 se l'operazione è andata a buon fine, <=0 altrimenti
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}
