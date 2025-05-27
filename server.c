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
#include <dirent.h>
#include "pacote.h"
#include "jogo.h"

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

int descobrir_extensao(char* base, uchar* nome) {
    int tipo = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(".");

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, base, strlen(base)) == 0) {
                const char *p = dir->d_name + strlen(base);
                if (*p == '.') {
                    if (strcmp(p, ".jpg") == 0) tipo = IMAGEM;
                    else if (strcmp(p, ".txt") == 0) tipo = TEXTO;
                    else if (strcmp(p, ".mp4") == 0) tipo = VIDEO;
                }
            }
        }
        closedir(d);
    } else {
        perror("Nao foi possível abrir o diretorio");
    }
    strcpy((char*)nome, base);
    if (tipo == IMAGEM) strcat((char*)nome, ".jpg");
    else if (tipo == TEXTO) strcat((char*)nome, ".txt");
    else if (tipo == VIDEO) strcat((char*)nome, ".mp4");
    return tipo;
}

int manda_arquivo(int soq, FILE* arq) {
    printf("Enviando arquivo\n");
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
            if (pacr->checksum == calcula_checksum(pacr)) {
                if (pacr->tipo == ACK) ack = ACK;
            }
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
    printf("Terminou de enviar\n");
    return 1;
}

int main(){
    srand(0);
    int soq = cria_raw_socket("lo");

    int posx = 0, posy = 0;
    tile** tabuleiro = inicica_jogo(1);
    imprime_mapa(tabuleiro, 1, posx, posy);

    int arq_enviados = 0;
    pacote *pacr, *pacs;
    int ack;
    while (arq_enviados < 8) {
        ack = NACK;

        pacr = recebe_pacote(soq);
        if (calcula_checksum(pacr) == pacr->checksum) {
            ack = ACK;
            if (pacr->tipo == CIMA && posy < TAM_TABULEIRO-1) {
                posy++;
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1;
                ack = OK_ACK;
            } else if (pacr->tipo == ESQUERDA && posx > 0) {
                posx--;
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1;
                ack = OK_ACK;
            } else if (pacr->tipo == BAIXO && posy > 0) {
                posy--;
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1;
                ack = OK_ACK;
            }
            else if (pacr->tipo == DIREITA && posx < TAM_TABULEIRO-1) {
                posx++;
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1;
                ack = OK_ACK;
            }

            if (tabuleiro[posx][posy].tem_tesouro && tabuleiro[posx][posy].passou != 2) {
                tabuleiro[posx][posy].passou = 2;
                imprime_mapa(tabuleiro, 1, posx, posy);

                uchar nome[64];
                ack = descobrir_extensao(tabuleiro[posx][posy].tesouro_id, nome);
                FILE* arq = fopen((char*)nome, "r"); if (!arq) {printf("ERRO AO ABRIR ARQ %s", (char*)nome); exit(1);}

                if (!(pacs = cria_pacote(nome, strlen((char*)nome)+1, pacr->sequencia, ack))) return 0;

                ack = NACK;
                while (ack == NACK) {
                    manda_pacote(soq, pacs);
                           
                    pacr = recebe_pacote(soq);
                    if (pacr->checksum == calcula_checksum(pacr)) {
                        if (pacr->tipo != NACK) ack = pacr->tipo;
                    }
                    destroi_pacote(pacr);
                }
                destroi_pacote(pacs);

                manda_arquivo(soq, arq);
                fclose(arq);
                arq_enviados++;
                continue;
            }
        }

        if (!(pacs = cria_pacote((uchar*)"", 0, pacr->sequencia, ack))) return 0;
        if (!(manda_pacote(soq, pacs))) return 0;

        destroi_pacote(pacr);
        destroi_pacote(pacs);

        imprime_mapa(tabuleiro, 1, posx, posy);
    }

    tabuleiro = libera_tabuleiro(tabuleiro);
    return 0;
}
