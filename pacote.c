#include "pacote.h"

pacote* cria_pacote(uchar* msg, int tam, int seq, int tipo) {
    pacote* pac = malloc(sizeof(pacote)); if (!pac) return NULL;
    pac->tamanho = tam;
    pac->sequencia = seq;
    pac->tipo = tipo;

    pac->dados = (uchar*) malloc((pac->tamanho) * sizeof(uchar)); if (!pac->dados) return NULL;
    memcpy(pac->dados, msg, pac->tamanho);

    pac->checksum = calcula_checksum(pac);

    return pac;
}

pacote* destroi_pacote(pacote* pac) {
    free(pac->dados);
    free(pac);

    return NULL;
}

uchar* gera_mensagem(pacote* p) {
    int bytes_extra = TAM_MIN - (p->tamanho + 4);
    if (bytes_extra < 0) bytes_extra = 0;

    uchar* msg = malloc((p->tamanho + 4 + bytes_extra) * sizeof(uchar));
    if (!msg) return NULL;

    msg[0] = MARCADORINI;
    // 7 bits de tamanho e o mais significativo de sequencia formam um byte
    msg[1] = ((p->tamanho & 0x7F) << 1) | ((p->sequencia & 0x10) >> 4);
    // 4 bits menos significativos de sequencia e tipo formam outro byte
    msg[2] = ((p->sequencia & 0x0F) << 4) | (p->tipo & 0x0F);
    msg[3] = p->checksum;
    memcpy(&msg[4], p->dados, p->tamanho);
    memset(&msg[4 + p->tamanho], 0, bytes_extra);

    return msg;
}

pacote* gera_pacote(uchar* msg) {
    pacote *p = malloc(sizeof(pacote)); if (!p) return NULL;
    p->tamanho = (msg[1] & 0xFE) >> 1;
    p->sequencia = ((msg[1] & 0x01) << 4) | ((msg[2] & 0xF0) >> 4);
    p->tipo = msg[2] & 0x0F;
    p->checksum = msg[3];
    p->dados = malloc(p->tamanho * sizeof(uchar)); if (!p->dados) return NULL;
    memcpy(p->dados, &msg[4], p->tamanho);

    return p;
}

uchar calcula_checksum(pacote* p) {
    int soma = ((p->tamanho & 0x7F) << 1) | ((p->sequencia & 0x10) >> 4);
    soma += ((p->sequencia & 0x0F) << 4) | (p->tipo & 0x0F);
    for (int i = 0; i < p->tamanho; i++) {
        soma += p->dados[i];
    }
    return soma & 0xFF;
}