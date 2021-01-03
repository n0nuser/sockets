#!/bin/bash
#
## Fichero: run.sh
## Autores:
## Sergio García González
## Pablo Jesús González Rubio
#
./killPS.sh
make clean
make
./lanzaServidor.sh
