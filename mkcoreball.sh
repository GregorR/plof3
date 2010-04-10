#!/bin/bash
for (( i = 0;; i++ ))
do
    if [ ! -e "coreball.$i.tar.bz2" ]
    then
        hg id > hgid
        tar jcf coreball.$i.tar.bz2 hgid core cplof/src/cplof `hg status -nam`
        echo coreball.$i.tar.bz2
        exit 0
    fi
done
