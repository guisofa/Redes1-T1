#ifndef __PACOTE__
#define __PACOTE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARCADORINI 126 // 01111110
#define PACOTE_TAM_MAX 130
#define DADOS_TAM_MAX 127

typedef unsigned char uchar;

typedef struct pacote{
    uchar tamanho;
    uchar sequencia;
    uchar tipo;
    uchar checksum;
    uchar* dados;
} pacote;

uchar* gera_mensagem(pacote* p);
pacote* gera_pacote(uchar* msg);

#endif