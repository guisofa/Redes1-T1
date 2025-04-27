#include "pacote.h"

uchar* gera_mensagem(pacote* p) {
    uchar* msg = malloc((4 + p->tamanho) * sizeof(uchar));
    msg[0] = MARCADORINI;
    // 7 bits de tamanho e o mais significativo de sequencia formam um byte
    msg[1] = ((p->tamanho & 0x7F) << 1) | ((p->sequencia & 0x10) >> 4);
    // 4 bits menos significativos de sequencia e tipo formam outro byte
    msg[2] = ((p->sequencia & 0x0F) << 4) | (p->tipo & 0x0F);
    msg[3] = p->checksum;
    memcpy(&msg[4], p->dados, p->tamanho);

    return msg;
}

pacote* gera_pacote(uchar* msg) {
    pacote *p = malloc(sizeof(pacote));
    p->tamanho = (msg[1] & 0xFE) >> 1;
    p->sequencia = ((msg[1] & 0x01) << 4) | ((msg[2] & 0xF0) >> 4);
    p->tipo = msg[2] & 0x0F;
    p->checksum = msg[3];
    p->dados = malloc(p->tamanho * sizeof(uchar));
    memcpy(p->dados, &msg[4], p->tamanho);

    return p;
}