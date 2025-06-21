#!/bin/bash

for arquivo in ./objetos/?.* 
do
    diff "$arquivo" "$(basename ${arquivo})copia"
done