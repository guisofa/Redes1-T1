#ifndef __JOGO__
#define __JOGO__

#include <stdio.h>
#include <stdlib.h>

#define TAM_TABULEIRO 8

typedef struct {
    char passou;
    char tem_tesouro;
    char* tesouro_id;
} tile;

tile** libera_tabuleiro(tile** t);
tile** inicica_jogo(int eh_server);
void imprime_mapa(tile** t, int eh_server, int x, int y);

#endif