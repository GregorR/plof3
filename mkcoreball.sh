#!/bin/bash
for (( i = 0;; i++ ))
do
    if [ ! -e "coreball.$i.tar.bz2" ]
    then
        hg id > hgid

        echo 'Creating backtrace...'
        gdb --batch -x autotests/bt.gdb ./cplof/src/cplof core > backtrace

        echo 'Creating coreball...'
        tar jcf coreball.$i.tar.bz2 hgid core backtrace cplof/src/cplof `hg status -nam`

        echo coreball.$i.tar.bz2
        exit 0
    fi
done
