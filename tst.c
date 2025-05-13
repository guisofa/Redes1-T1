#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "pacote.h"

 
int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}

int manda_pacote(int soquete, pacote* pac) {
    uchar* buffer = gera_mensagem(pac);

    // se o tamanho da msg for menor que TAM_MIN entao bytes extras foram colocados
    if (send(soquete, buffer, (pac->tamanho+4 < TAM_MIN) ? TAM_MIN : (pac->tamanho + 4), 0) == -1) return 0;
    free(buffer);

    return 1;
}

pacote* recebe_pacote(int soquete) {
    uchar buffer[PACOTE_TAM_MAX];

    while (1) {
        int bytes_recebidos = recv(soquete, buffer, PACOTE_TAM_MAX, 0);
        if (bytes_recebidos < 0) {printf("erro recv\n"); return NULL;}
        if (bytes_recebidos >= 4/*tamanho minimo da msg*/ && buffer[0] == MARCADORINI) {
            pacote* pac = gera_pacote(buffer);
            if (pac->tipo == ACK || pac->tipo == NACK) {destroi_pacote(pac); continue;}
            return pac;
        }
    }
    return NULL;
}

int recebe_arquivo(int soq) {
    FILE* arq = fopen("2.txt", "w");
    pacote *pacr, *pacs;
    char ack;
    unsigned int ultima_seq = 100;
    
    while (1) {
        ack = NACK;

        pacr = recebe_pacote(soq);
        if (calcula_checksum(pacr) == pacr->checksum) {
            if (pacr->tipo == FIM_ARQ) {
                destroi_pacote(pacr);
                break;
            }
            ack = ACK;
            if (ultima_seq != pacr->sequencia) {
                fwrite(pacr->dados, 1, pacr->tamanho, arq);
                ultima_seq = pacr->sequencia;
            }
            else {
                //printf("%d/%d: %.*s\n\n", ultima_seq, pacr->sequencia, pacr->tamanho, pacr->dados);
            }
        }

        if (!(pacs = cria_pacote((uchar*)"", 0, pacr->sequencia, ack))) return 0;
        if (!(manda_pacote(soq, pacs))) return 0;

        destroi_pacote(pacr);
        destroi_pacote(pacs);
    }
    fclose(arq);
    return 1;
}

int main() {
    int soq = cria_raw_socket("lo");

    recebe_arquivo(soq);

    return 0;
}