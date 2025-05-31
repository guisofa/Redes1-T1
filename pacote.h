#ifndef __PACOTE__
#define __PACOTE__

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARCADORINI 126 // 01111110
#define PACOTE_TAM_MAX 131 * 2 // possiveis bytes 0xff extras apos 0x88 e 0x81
#define DADOS_TAM_MAX 127
#define TAM_MIN 14 // tamanho minimo da msg a ser enviada por send

#define ACK 0
#define NACK 1
#define OK_ACK 2
//#define ??? 3
#define TAMANHO 4
#define DADOS 5
#define TEXTO 6
#define VIDEO 7
#define IMAGEM 8
#define FIM_ARQ 9
#define DIREITA 10
#define CIMA 11
#define BAIXO 12
#define ESQUERDA 13
//#define ??? 14
#define ERRO 15

typedef unsigned char uchar;

typedef struct pacote{
    uchar tamanho;
    uchar sequencia;
    uchar tipo;
    uchar checksum;
    uchar* dados;
} pacote;

/* Cria uma struct pacote completa usando os parametros recebidos */
pacote* cria_pacote(uchar* msg, int tam, int seq, int tipo);

/* Libera a memoria alocada em pac e retorna NULL */
pacote* destroi_pacote(pacote* pac);

/* Recebe um pacote e transforma em uma string onde
   o primeiro byte eh o marcador de inicio
   o segundo eh 7 bits de tam e o mais significativo de seq
   o terceiro eh os 4 menos significativos de seq e os 4 de tipo
   o quarto eh o checksum
   o quinto pra frente sao os dados;
   Caso a mensagem tenha menos de TAM_MIN bytes entao bytes nulos
   serao colocados apos os dados para alcancar o valor minimo  
   Coloca bytes 0xff depois de bytes 0x88 e 0x81
   A soma de todos os bytes extras e colocada em bytes_adicionados */
uchar* gera_mensagem(pacote* p, int* bytes_adicionados);

/* Recebe uma string e gera uma struct pacote a partir dela;
   Bytes nulos extras serao ignorados  */
pacote* gera_pacote(uchar* msg);

/* manda um pacote pac pela rede
   Se estiver em loopback tira as msgs duplicadas */
int manda_pacote(int soquete, pacote* pac, char eh_loopback);

/* recebe um pacote
   Se estiver em loopback tira as msgs duplicadas */
pacote* recebe_pacote(int soquete, char eh_loopback);

/* Recebe um pacote e calcula o checksum em cima dos campos
   tamanho, sequencia, tipo e dados;
   Leva em conta que tam, seq e tipo sao apenas 2 bytes */
uchar calcula_checksum(pacote* p);

#endif