/**
 * @file parser.h
 * @brief File per l'implementazione del parser
 *
 * Autore: Stefano Torneo 545261
 *
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera originale dell'autore. 
 */

/**
 * @var maxconnections indica il numero massimo di connessioni pendenti
 * @var threadsinpool indica il numero di thread nel pool 
 * @var maxmsgsize indica la dimensione massima di un messaggio testuale
 * @var maxfilesize indica la dimensione massima di un file accettato dal server in kilobytes
 * @var maxhistmsgs indica il numero massimo di messaggi che il server memorizza per ogni utente
 */
int maxconnections,threadsinpool,maxmsgsize,maxfilesize,maxhistmsgs;

/**
 * @var unixpath indica il path utilizzato per la creazione del socket AF_UNIX
 * @var dirname indica la directory dove memorizzare i files da inviare agli utenti
 * @var statfilename indica il file nel quale verranno scritte le statistiche del server
 */
char *unixpath,*dirName,*statfilename;

//macro per allocazioni dinamiche
#define SYSCALL_D(r,c,e) \
    if((r=c)==NULL) { perror(e); exit(-1); }

/**
 * @function Leggi
 * @brief Legge stringhe da file
 * @param fp puntatore al file
 * @param buf stringa che conterrà il contenuto letto da file
 */
void Leggi(FILE *fp, char *buf){
  fscanf(fp,"%s",buf);
  fscanf(fp,"%s",buf);
}

/**
 * @function Parser
 * @brief Parsa il file di configurazione memorizzando le informazioni necessarie in delle variabili
 * @param filename indica il nome del file di configurazione da parsare
 */
void Parser(char *filename){
  //apro il file in lettura
  FILE *fp=fopen(filename,"r"); 
  if(fp==NULL){
    perror(filename);
    return;
  }
  SYSCALL_D(unixpath,malloc(sizeof(char)),"malloc");
  SYSCALL_D(dirName,malloc(sizeof(char)),"malloc");
  SYSCALL_D(statfilename,malloc(sizeof(char)),"malloc");
  char buf[100];
  //itero fino alla fine del file e memorizzo nella variabile buf ogni stringa letta
  while(fscanf(fp,"%s",buf)!=EOF){
    if(!strcmp("UnixPath",buf)){
      Leggi(fp,buf);
      char *tmp=realloc(unixpath,sizeof(char)*strlen(buf)+1);
      if(tmp==NULL)
        return;
      unixpath=tmp;
      strncpy(unixpath,buf,strlen(buf)+1);
    }
    else if(!strcmp("MaxConnections",buf)){
      Leggi(fp,buf);
      maxconnections=atoi(buf);
    }
    else if(!strcmp("ThreadsInPool",buf)){
      Leggi(fp,buf);
      threadsinpool=atoi(buf);
    }
    else if(!strcmp("MaxMsgSize",buf)){
      Leggi(fp,buf);
      maxmsgsize=atoi(buf); 
    }
    else if(!strcmp("MaxFileSize",buf)){
      Leggi(fp,buf);
      maxfilesize=atoi(buf);
    }
    else if(!strcmp("MaxHistMsgs",buf)){
      Leggi(fp,buf);
      maxhistmsgs=atoi(buf);
    }
    else if(!strcmp("DirName",buf)){
      Leggi(fp,buf);
      char *tmp=realloc(dirName,sizeof(char)*strlen(buf)+2);
      if(tmp==NULL)
        return;
      dirName=tmp;
      strncpy(dirName,buf,strlen(buf)+1);
      strncat(dirName,"/",1);
    }
    else if(!strcmp("StatFileName",buf)){
      Leggi(fp,buf);
      char *tmp=realloc(statfilename,sizeof(char)*strlen(buf)+1);
      if(tmp==NULL)
        return;
      statfilename=tmp;
      strncpy(statfilename,buf,strlen(buf)+1);
    }
  }
  //chiudo il file
  fclose(fp);
}