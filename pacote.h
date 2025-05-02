#ifndef __PACOTE__
#define __PACOTE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARCADORINI 126 // 01111110
#define PACOTE_TAM_MAX 131
#define DADOS_TAM_MAX 127
#define TAM_MIN 14 // tamanho minimo da msg a ser enviada por send

typedef unsigned char uchar;

typedef struct pacote{
    uchar tamanho;
    uchar sequencia;
    uchar tipo;
    uchar checksum;
    uchar* dados;
} pacote;

/* Recebe um pacote e transforma em uma string onde
   o primeiro byte eh o marcador de inicio
   o segundo eh 7 bits de tam e o mais significativo de seq
   o terceiro eh os 4 menos significativos de seq e os 4 de tipo
   o quarto eh o checksum
   o quinto pra frente sao os dados;
   Caso a mensagem tenha menos de TAM_MIN bytes entao bytes nulos
   serao colocados apos os dados para alcancar o valor minimo  */
uchar* gera_mensagem(pacote* p);

/* Recebe uma string e gera uma struct pacote a partir dela;
   Bytes nulos extras serao ignorados  */
pacote* gera_pacote(uchar* msg);

/* Recebe um pacote e calcula o checksum em cima dos campos
   tamanho, sequencia, tipo e dados;
   Leva em conta que tam, seq e tipo sao apenas 2 bytes */
uchar calcula_checksum(pacote* p);

#endif