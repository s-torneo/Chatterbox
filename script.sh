#!/bin/bash

#controllo che il numero di parametri sia 2

if [ ! $# -eq 2 ];
  then
    echo "usa $0 file.conf time" 1>&2
    exit 1;
fi

#controllo che i due parametri siano diversi di "-help"

if [ $1 == "-help" -o $2 == "-help" ];
  then
    echo "usa $0 file.conf time" 1>&2
    exit 1;
fi


#entro nella directory del file di configurazione

cd DATA

#controllo che il primo parametro sia un file

if [ ! -f $1 ];
  then
    echo "usa $0 file.conf time" 1>&2
    exit 1;
fi

#estraggo la cartella relativa a DirName

while read line ; do
 array=( $line )
 if [[ ${array[0]} == "DirName" ]]; then
   cartella=(${array[2]})
 fi
done < $1

#controllo che la variabile cartella sia effettivamente una directory

if [ ! -d $cartella ];
  then
    echo "usa $0 file.conf time" 1>&2
    exit 1;
fi

#entro nella cartella specificata in DirName

cd $cartella

#controllo se t è = 0, in tal caso stampa i file presenti nella cartella specificata in DirName

if [ $2 -eq 0 ]; then
  ls $cartella
  exit 1;
fi

#trova la lista dei file più vecchi di t min, crea il tar e li mette dentro

find . -type f -mmin +$2 -exec tar --remove-files -czf cartella.tar.gz {} +