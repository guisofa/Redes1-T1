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
    if (!pac) return NULL;

    free(pac->dados);
    free(pac);

    return NULL;
}

// eh um pouco complexo de entender essa daqui entao cuidado
uchar* gera_mensagem(pacote* p, int* bytes_adicionados) {
    int bytes_extra = TAM_MIN - (p->tamanho + 4); // bytes nulos extras
    if (bytes_extra < 0) bytes_extra = 0;

    int offset = 0; // bytes 0xff que tiveram que ser colocados

    // alocando duas vezes a memoria necessaria caso tenhamos que colocar bytes 0xff em todos os bytes
    uchar* msg = calloc(2 * (p->tamanho + 4 + bytes_extra), sizeof(uchar));
    if (!msg) return NULL;

    msg[0] = MARCADORINI;

    // 7 bits de tamanho e o mais significativo de sequencia formam um byte
    msg[1] = ((p->tamanho & 0x7F) << 1) | ((p->sequencia & 0x10) >> 4);
    // caso seja o byte proibido, entao o proximo tem que ser 0xff e offset eh incrementado
    if (msg[1] == 0x88 || msg[1] == 0x81) {
        msg[2] = 0xff;
        offset++;
    }

    // 4 bits menos significativos de sequencia e tipo formam outro byte
    msg[2 + offset] = ((p->sequencia & 0x0F) << 4) | (p->tipo & 0x0F);
    if (msg[2 + offset] == 0x88 || msg[2 + offset] == 0x81) { // usamos offset para saber quantos bytes 0xff temos que pular
        msg[3 + offset] = 0xff;
        offset++;
    }

    msg[3 + offset] = p->checksum;
    if (msg[3 + offset] == 0x88 || msg[3 + offset] == 0x81) {
        msg[4 + offset] = 0xff;
        offset++;
    }

    for (int i = 0; i < p->tamanho; i++) {
        msg[4 + offset + i] = p->dados[i];
        if (msg[4 + offset + i] == 0x88 || msg[4 + offset + i] == 0x81) {
            offset++;
            msg[4 + offset + i] = 0xff;
        }
    }

    *bytes_adicionados = bytes_extra + offset;

    return msg;
}

// usa a mesma logica da funcao anterior, mas ao contrario
// para ignorar todos os bytes 0xff
pacote* gera_pacote(uchar* msg) {
    pacote *p = malloc(sizeof(pacote)); if (!p) return NULL;

    int offset = 0;

    if (msg[1] == 0x88 || msg[1] == 0x81) {
        offset++;
    }


    p->tamanho = (msg[1] & 0xFE) >> 1;
    p->sequencia = ((msg[1] & 0x01) << 4) | ((msg[2 + offset] & 0xF0) >> 4);
    p->tipo = msg[2 + offset] & 0x0F;

    if (msg[2 + offset] == 0x88 || msg[2 + offset] == 0x81) {
        offset++;
    }

    p->checksum = msg[3 + offset];

    if (msg[3 + offset] == 0x88 || msg[3 + offset] == 0x81) {
        offset++;
    }

    p->dados = malloc(p->tamanho * sizeof(uchar)); if (!p->dados) return NULL;

    for (int i = 0; i < p->tamanho; i++) {
        p->dados[i] = msg[4 + offset + i];
        if (msg[4 + offset + i] == 0x88 || msg[4 + offset + i] == 0x81) {
            offset++;
        }
    }

    return p;
}

int manda_pacote(int soquete, pacote* pac, char eh_loopback) {
    int bytes_extras;
    uchar* buffer = gera_mensagem(pac, &bytes_extras);

    // se o tamanho da msg for menor que TAM_MIN entao bytes extras foram colocados
    if (send(soquete, buffer, pac->tamanho + 4 + bytes_extras, 0) == -1) return 0;
    free(buffer);


    // recebendo a propria msg enviada quando em lo.
    if (eh_loopback){
        buffer = malloc(PACOTE_TAM_MAX * sizeof(uchar));
        while (1) {
            int bytes_recebidos = recv(soquete, buffer, PACOTE_TAM_MAX, 0);
            if (bytes_recebidos < 0) {printf("erro recv\n"); return 0;}
            if (bytes_recebidos >= 4/*tamanho minimo da msg*/ && buffer[0] == MARCADORINI) {
                break;
            }
        }
        free(buffer);
    }

    return 1;
}

pacote* recebe_pacote(int soquete, char eh_loopback) {
    uchar buffer[PACOTE_TAM_MAX];
    pacote* pac = NULL;

    while (1) {
        int bytes_recebidos = recv(soquete, buffer, PACOTE_TAM_MAX, 0);
        if (bytes_recebidos < 0) {printf("erro recv\n"); return NULL;}
        if (bytes_recebidos >= 4/*tamanho minimo da msg*/ && buffer[0] == MARCADORINI) {
            pac = gera_pacote(buffer);
            break;
        }
    }

    // parte em loop para tirar duplos quando em lo.
    if (eh_loopback) {
        destroi_pacote(pac);
        while (1) {
            int bytes_recebidos = recv(soquete, buffer, PACOTE_TAM_MAX, 0);
            if (bytes_recebidos < 0) {printf("erro recv\n"); return NULL;}
            if (bytes_recebidos >= 4/*tamanho minimo da msg*/ && buffer[0] == MARCADORINI) {
                pac = gera_pacote(buffer);
                break;
            }
        }
    }

    return pac;
}

uchar calcula_checksum(pacote* p) {
    int soma = ((p->tamanho & 0x7F) << 1) | ((p->sequencia & 0x10) >> 4);
    soma += ((p->sequencia & 0x0F) << 4) | (p->tipo & 0x0F);
    for (int i = 0; i < p->tamanho; i++) {
        soma += p->dados[i];
    }
    return soma & 0xFF;
}