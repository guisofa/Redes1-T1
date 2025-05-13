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
    printf("mandado: %d\n", pac->sequencia);
    return 1;
}

pacote* recebe_pacote(int soquete) {
    uchar buffer[PACOTE_TAM_MAX];

    while (1) {
        int bytes_recebidos = recv(soquete, buffer, PACOTE_TAM_MAX, 0);
        if (bytes_recebidos < 0) {printf("erro recv\n"); return NULL;}
        if (bytes_recebidos >= 4/*tamanho minimo da msg*/ && buffer[0] == MARCADORINI) {
            pacote* pac = gera_pacote(buffer);
            if (pac->tipo == DADOS || pac->tipo == FIM_ARQ) {destroi_pacote(pac); continue;}
            printf("recebido: %d\n", pac->tipo);
            return pac;
        }
    }
    return NULL;
}

int manda_arquivo(int soq, FILE* arq) {
    uchar buffer[DADOS_TAM_MAX];
    pacote *pacr, *pacs; // pacote recieved/sent
    int bytes_lidos;
    int seq = 0;
    char ack;

    while ((bytes_lidos = fread(buffer, 1, DADOS_TAM_MAX, arq)) > 0) {
        ack = NACK;
        if (!(pacs = cria_pacote(buffer, bytes_lidos, seq, DADOS))) return 0;
        
        while (ack != ACK) {
            if (!manda_pacote(soq, pacs)) return 0;

            pacr = recebe_pacote(soq);
            if (pacr->checksum == calcula_checksum(pacr))
                if (pacr->tipo == ACK) ack = ACK;
            destroi_pacote(pacr);
        }
        seq = (seq + 1) % 32;
        destroi_pacote(pacs);
    }

    if (!(pacs = cria_pacote((uchar*)"", 0, seq, FIM_ARQ))) return 0;
    while (1) {
        if (!manda_pacote(soq, pacs)) return 0;
        pacr = recebe_pacote(soq);
        if (pacr->tipo == ACK) {
            destroi_pacote(pacr);
            break;
        }
    }
    return 1;
}

int main(){
    int soq = cria_raw_socket("lo");

    struct stat st;
    if (stat("1.txt", &st) == -1) {printf("arquivo nao existe\n"); return 1;}

    FILE* arquivo = fopen("1.txt", "r");
    if (!manda_arquivo(soq, arquivo)) {printf("erro ao enviar arquivo\n"); return 1;}

    fclose(arquivo);
    return 0;
}
