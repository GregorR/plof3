#!/bin/bash
gcc -O2 -lgc gctest.c gc.c -o gctest
./gctest
time ./gctest

gcc -O2 -lgc -DREAL_GC gctest.c gc.c -o gctest
./gctest
time ./gctest
