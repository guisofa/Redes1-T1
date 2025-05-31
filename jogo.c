#include "jogo.h"

tile** inicica_jogo(int eh_server) {
    tile** tabuleiro = malloc(TAM_TABULEIRO * sizeof(tile*));
    // usando o metodo de icc para alocar matriz
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
    tabuleiro[0][0].passou = 1; // usuario comeca em (0,0)

    if (!eh_server) return tabuleiro; // se nao for server para aqui

    i = 0;
    while (i < TAM_TABULEIRO) { // sorteando cada tesouro a uma tile
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

void imprime_mapa(tile** t, movimento* log, int eh_server, int x, int y, int tesouros_encontrados) {
    printf("\033[H\033[J"); // limpa a tela
    printf("TESOUROS: %d/8\n", tesouros_encontrados); // imprime o numero de tesouros encontrados
    for (int i = 0; i < TAM_TABULEIRO+2; i++) {
        printf(" #"); // teto
    }
    printf("\n");

    for (int j = TAM_TABULEIRO-1; j >= 0; j--) {
        printf(" #"); // parede esquerda
        for (int i = 0; i < TAM_TABULEIRO; i++) {
            if (i == x && j == y) 
                printf(" @"); // usuario
            else if (t[i][j].passou)
                printf(" *"); // tile ja visitada
            else if (eh_server && t[i][j].tem_tesouro)
                printf(" X"); // tesouro, so aparece se for server
            else
                printf(" O"); // tile nao visitada
        }
        printf(" #\n"); // parede direita
    }

    for (int i = 0; i < TAM_TABULEIRO+2; i++) {
        printf(" #"); // chao
    }
    printf("\n");
    if (!eh_server) return; // se nao for server para aqui

    // log de movimentos
    printf("LOG:\n");
    for (int i = 0; i < TAM_LOG; i++) {
        if (log->x == -1) return;
        printf("(%d, %d)\n", log->x, log->y);
        log = log->prox;
    }
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

movimento* inicia_log() {
    movimento* head = malloc(sizeof(movimento));
    head->prox = NULL;
    head->ant = NULL;
    head->x = head->y = 0;

    movimento* ultimo = head;
    for (int i = 1; i < TAM_LOG; i++) {
        movimento* novo = malloc(sizeof(movimento));
        novo->ant = ultimo;
        novo->prox = NULL;
        ultimo->prox = novo;
        novo->x = novo->y = -1;
        ultimo = novo;
    }
    ultimo->prox = head;
    head->ant = ultimo;

    return head;
}

movimento* adiciona_passo(movimento* head, int x, int y) {
    movimento* new_head = head->ant;
    new_head->x = x;
    new_head->y = y;
    return new_head;
}

movimento* libera_log(movimento* head) {
    for (int i = 0; i < TAM_LOG; i++) {
        movimento* temp = head;
        head = head->prox;
        free(temp);
    }
    return NULL;
}