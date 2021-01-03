#!/bin/bash
#
## Fichero: lanzaServidor.sh
## Autores:
## Sergio García González
## Pablo Jesús González Rubio
#

clear

./servidor
./cliente nogal TCP ordenes1.txt &
./cliente nogal TCP ordenes2.txt &
./cliente nogal TCP ordenes3.txt &
./cliente nogal UDP ordenes1.txt &
./cliente nogal UDP ordenes2.txt &
./cliente nogal UDP ordenes3.txt &

#./cliente localhost TCP ordenes1.txt &
#./cliente localhost TCP ordenes2.txt &
#./cliente localhost TCP ordenes3.txt &
#./cliente localhost UDP ordenes1.txt &
#./cliente localhost UDP ordenes2.txt &
#./cliente localhost UDP ordenes3.txt &
