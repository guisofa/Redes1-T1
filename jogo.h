#ifndef __JOGO__
#define __JOGO__

#include <stdio.h>
#include <stdlib.h>

#define TAM_TABULEIRO 8
#define TAM_LOG 15 // mudar isso se quiser um log maior

// o tabuleiro sera uma matriz de tiles
typedef struct {
    char passou;
    char tem_tesouro;
    char* tesouro_id;
} tile;

// lista duplamente encadeada circular para formar o log
struct movimento {
    int x, y;
    struct movimento* prox;
    struct movimento* ant;
};
typedef struct movimento movimento;

/* aloca memoria para o tabuleiro completo
   iniciando todos os parametros e sorteando a localizacao dos tesouros */
tile** inicica_jogo(int eh_server);

/* libera toda a memoria alocada ao tabuleiro */
tile** libera_tabuleiro(tile** t);

/* aloca memoria para a lista duplamente encadeada circular
   de tamanho TAM_LOG. inicializa os valores de x e y com -1
   para dizer que nao foram dados TAM_LOG passos 
   O primeiro item da lista comeca com x,y 0,0               */
movimento* inicia_log();

/* libera toda a memoria alocada ao log */
movimento* libera_log(movimento* head);

/* adiciona um movimento ao log tirando o movimento
   mais antigo e colocando o novo                   */
movimento* adiciona_passo(movimento* head, int x, int y);

/* imprime o mapa e o log de movimentos caso seja server 
   dentro da funcao no .c explico o que cada simbolo significa */
void imprime_mapa(tile** t, movimento* log, int eh_server, int x, int y, int encontrados);

#endif