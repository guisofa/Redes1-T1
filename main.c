#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pacote.h"

//teste de criação de branch

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

void insereBits(unsigned char* buffer,int* guia, unsigned char valor, int tam){
    for(int i = tam - 1; i >= 0; i--){
        unsigned char bit = (valor >> i) & 1;

        int byte_index = (*guia) / 8;
        int bit_index = 7 - ((*guia) % 8);

        buffer[byte_index] |= (bit << bit_index);
        (*guia)++;
    }
}

unsigned char* serializaPacote(pacote package){
    unsigned char* buffer;
    int bitGuia = 0;
    int tam = (int) package.tamanho;

    buffer = malloc(sizeof(unsigned char) * tam + 32);

    insereBits(buffer, &bitGuia, package.marcador, 8);
    insereBits(buffer, &bitGuia, package.tamanho, 7);
    insereBits(buffer, &bitGuia, package.sequencia, 5);
    insereBits(buffer, &bitGuia, package.tipo, 4);
    insereBits(buffer, &bitGuia, package.checksum, 8);
    insereBits(buffer, &bitGuia, package.dados, package.tamanho); //como é multiplo de 8 e a posição inicial tbm da pra pensar em uma copia direta
                                                                  //nao se preocupar se é mais de 127 bytes, deixa isso para uma camada superior
                                                                  //que vai construir o pacote, ela que vai particionar...
    return buffer;
}

void imprimePacoteArray(unsigned char* buffer, int tam){
    for(int i = 0; i < tam; i++){
        printf("%02X ", buffer[i]);
    }
    return;
}

int main(){
    int testeSoquete;

    testeSoquete = cria_raw_socket("lo");

    pacote testePacote;
    testePacote.marcador = 0x7E;
    testePacote.tamanho = 0;
    testePacote.sequencia = 4;
    testePacote.tipo = 3;
    testePacote.checksum = 0x2F;

    unsigned char pacote[32];
    memset(pacote, 100, sizeof(pacote));
    strcpy((char*) pacote, "testando...");

    send(testeSoquete, pacote, 32, 0);

    close(testeSoquete);

    return 0;
}
