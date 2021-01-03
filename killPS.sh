#!/bin/bash
#
## Fichero: killPS.sh
## Autores:
## Sergio García González
## Pablo Jesús González Rubio
#

pkill --signal SIGTERM servidor
pkill --signal SIGTERM cliente
