#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

int manda_pacote(int soquete, char* msg) {
    pacote* pacote = malloc(sizeof(pacote));
    pacote->tamanho = (uchar)(strlen(msg)+1);
    pacote->sequencia = 0;
    pacote->tipo = 0;
    pacote->checksum = 0;
    pacote->dados = (uchar*) malloc((pacote->tamanho) * sizeof(uchar));
    memcpy((char*)pacote->dados, msg, pacote->tamanho);

    uchar* buffer = gera_mensagem(pacote);

    /* // nao tenho ideia do que isso faz, mas não funciona sem.
       // update: agora funciona, mas vou deixar aqui so pra ter certeza
    struct sockaddr_ll destino = {0};
    destino.sll_family = AF_PACKET;
    destino.sll_protocol = htons(ETH_P_ALL);
    destino.sll_ifindex = if_nametoindex("lo");
    */
    send(soquete, buffer, pacote->tamanho + 4, 0);
    free(pacote->dados);
    free(pacote);
    free(buffer);
    return 0;
}

void recebe_mensagem(int soquete) {
    uchar buffer[PACOTE_TAM_MAX];

    while (1) {
        int bytes_recebidos = recv(soquete, buffer, PACOTE_TAM_MAX, 0);
        if (bytes_recebidos < 0) printf("erro recv\n");
        if (bytes_recebidos > 4/*tamanho minimo da msg*/ && buffer[0] == MARCADORINI) {
            pacote* pacote = gera_pacote(buffer);
            printf("recebido:\n");
            printf("marcador:  %#02X\n", MARCADORINI);
            printf("tamanho:   %d\n", pacote->tamanho);
            printf("sequencia: %d\n", pacote->sequencia);
            printf("tipo:      %d\n", pacote->tipo);
            printf("dados: ");
            for (int i = 0; i < pacote->tamanho; i++) {
                printf("%c", pacote->dados[i]);
            }
            printf("\n");
        }
    }
}

int main() {
    int soq = cria_raw_socket("lo");

    manda_pacote(soq, "Ola, socket!");

    recebe_mensagem(soq);

    return 0;
}