/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

/**
 * @file connections.h
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

#include <message.h>

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
int openConnection(char* path, unsigned int ntimes, unsigned int secs);

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
int readHeader(long connfd, message_hdr_t *hdr);

/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int readData(long fd, message_data_t *data);

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd	 descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int readMsg(long fd, message_t *msg);


// ------- client side ------

/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server 
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendRequest(long fd, message_t *msg);

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendData(long fd, message_data_t *msg);

/**
 * @function sendMsg
 * @brief Invia l'intero messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendMsg(long fd, message_t *msg);

/**
 * @function sendHeader
 * @brief Invia l'header del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return -1 se c'è stato un errore, 1 altrimenti
 */
int sendHeader(long fd, message_hdr_t *msg);

#endif /* CONNECTIONS_H_ */