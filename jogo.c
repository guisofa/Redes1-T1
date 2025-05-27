#include "jogo.h"

tile** inicica_jogo(int eh_server) {
    tile** tabuleiro = malloc(TAM_TABULEIRO * sizeof(tile*));
    tile* mapa = malloc(TAM_TABULEIRO*TAM_TABULEIRO * sizeof(tile));

    int i;
    for (i = 0; i < TAM_TABULEIRO; i++) {
        tabuleiro[i] = mapa + TAM_TABULEIRO*i;
    }

    for (i = 0; i < TAM_TABULEIRO; i++)
        for (int j = 0; j < TAM_TABULEIRO;  j++) {
            tabuleiro[i][j].passou = 0;
            tabuleiro[i][j].tem_tesouro = 0;
            tabuleiro[i][j].tesouro_id = malloc(2);
        }
    tabuleiro[0][0].passou = 1;

    if (!eh_server) return tabuleiro;

    i = 0;
    while (i < TAM_TABULEIRO) {
        int x = rand() % TAM_TABULEIRO;
        int y = rand() % TAM_TABULEIRO;

        if (tabuleiro[x][y].tem_tesouro == 0 && !(x == 0 && y == 0)) {
            tabuleiro[x][y].tem_tesouro = 1;
            tabuleiro[x][y].tesouro_id[0] = '1' + i;
            tabuleiro[x][y].tesouro_id[1] = '\0';            
            i++;
        }
    }

    return tabuleiro;
}

void imprime_mapa(tile** t, int eh_server, int x, int y) {
    printf("\033[H\033[J");
    for (int i = 0; i < TAM_TABULEIRO+2; i++) {
        printf(" #");
    }
    printf("\n");

    for (int j = TAM_TABULEIRO-1; j >= 0; j--) {
        printf(" #");
        for (int i = 0; i < TAM_TABULEIRO; i++) {
            if (i == x && j == y) 
                printf(" @");
            else if (t[i][j].passou)
                printf(" *");
            else if (eh_server && t[i][j].tem_tesouro)
                printf(" X");
            else
                printf(" O");
        }
        printf(" #\n");
    }

    for (int i = 0; i < TAM_TABULEIRO+2; i++) {
        printf(" #");
    }
    printf("\n");
}

tile** libera_tabuleiro(tile** t) {
    for (int i = 0; i < TAM_TABULEIRO; i++) {
        for (int j = 0; j < TAM_TABULEIRO; j++) {
            free(t[i][j].tesouro_id);
        }
    }
    free(t[0]);
    free(t);
    return NULL;
}