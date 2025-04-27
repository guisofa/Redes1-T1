#include "pacote.h"

uchar* gera_mensagem(pacote* p) {
    int bytes_extra = TAM_MIN - (p->tamanho + 4);
    if (bytes_extra < 0) bytes_extra = 0;

    uchar* msg = malloc((p->tamanho + 4 + bytes_extra) * sizeof(uchar));

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
    pacote *p = malloc(sizeof(pacote));
    p->tamanho = (msg[1] & 0xFE) >> 1;
    p->sequencia = ((msg[1] & 0x01) << 4) | ((msg[2] & 0xF0) >> 4);
    p->tipo = msg[2] & 0x0F;
    p->checksum = msg[3];
    p->dados = malloc(p->tamanho * sizeof(uchar));
    memcpy(p->dados, &msg[4], p->tamanho);

    return p;
}