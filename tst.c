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
#include <termios.h>
#include "pacote.h"
#include "jogo.h"


void configurarModoRaw() {
    struct termios ttystate;

    tcgetattr(STDIN_FILENO, &ttystate);        // Pega configurações atuais
    ttystate.c_lflag &= ~(ICANON | ECHO);      // Desativa modo canônico e echo
    ttystate.c_cc[VMIN] = 1;                   // Espera 1 caractere
    ttystate.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate); // Aplica as novas configurações
}

void restaurarModoTerminal() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag |= (ICANON | ECHO); // Restaura modo canônico e echo
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

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

int recebe_arquivo(int soq, FILE* arq) {
    printf("Recebendo arquivo\n");
    pacote *pacr, *pacs; // pacote recieved/sent
    char ack;
    
    while (1) {
        ack = NACK;

        pacr = recebe_pacote(soq);
        if (calcula_checksum(pacr) == pacr->checksum) {
            ack = ACK;
            if (pacr->tipo == FIM_ARQ)
                break;
            fwrite(pacr->dados, 1, pacr->tamanho, arq);
        }

        if (!(pacs = cria_pacote((uchar*)"", 0, pacr->sequencia, ack))) return 0;
        if (!(manda_pacote(soq, pacs))) return 0;

        destroi_pacote(pacr);
        destroi_pacote(pacs);
    }
    if (!(pacs = cria_pacote((uchar*)"", 0, pacr->sequencia, ACK))) return 0;
    if (!(manda_pacote(soq, pacs))) return 0;

    destroi_pacote(pacr);
    destroi_pacote(pacs);

    printf("Arquivo recebido\n");
    return 1;
}

int main() {
    int soq = cria_raw_socket("lo");


    int posx = 0, posy = 0;
    tile** tabuleiro = inicica_jogo(0);
    imprime_mapa(tabuleiro, 0, posx, posy);

    configurarModoRaw();

    char c;
    int arq_recebidos = 0;
    pacote *pacr, *pacs;
    int seq = 0;
    int ack;
    while (arq_recebidos < 8) {
        ack = NACK;
        read(STDIN_FILENO, &c, 1);
        if (c == 'w') {
            pacs = cria_pacote((uchar*)"", 0, seq, CIMA);
        } else if (c == 'a') {
            pacs = cria_pacote((uchar*)"", 0, seq, ESQUERDA);
        } else if (c == 's') {
            pacs = cria_pacote((uchar*)"", 0, seq, BAIXO);
        } else if (c == 'd') {
            pacs = cria_pacote((uchar*)"", 0, seq, DIREITA);
        } else if (c == 'q') {
            break;
        } else {
            continue;
        }
        
        while (ack == NACK) {
            manda_pacote(soq, pacs);
            
            pacr = recebe_pacote(soq);
            if (pacr->checksum == calcula_checksum(pacr)) {
                if (pacr->tipo != NACK) {
                    ack = pacr->tipo;
                    break;
                }

            }
            destroi_pacote(pacr);
        }
        destroi_pacote(pacs);
        seq = (seq + 1) % 32;

        if (ack == ACK) {
            imprime_mapa(tabuleiro, 0, posx, posy);
            destroi_pacote(pacr);
            continue;
        }
        else if (ack == OK_ACK) {
            destroi_pacote(pacr);
            if (c == 'w') {
                posy++;
                tabuleiro[posx][posy].passou = 1;
            } else if (c == 'a') {
                posx--;
                tabuleiro[posx][posy].passou = 1;
            } else if (c == 's') {
                posy--;
                tabuleiro[posx][posy].passou = 1;
            } else {
                posx++;
                tabuleiro[posx][posy].passou = 1;
            }
            imprime_mapa(tabuleiro, 0, posx, posy);
        } else if (ack == IMAGEM || ack == TEXTO || ack == VIDEO) {
            if (c == 'w') {
                posy++;
                tabuleiro[posx][posy].passou = 1;
            } else if (c == 'a') {
                posx--;
                tabuleiro[posx][posy].passou = 1;
            } else if (c == 's') {
                posy--;
                tabuleiro[posx][posy].passou = 1;
            } else {
                posx++;
                tabuleiro[posx][posy].passou = 1;
            }
            imprime_mapa(tabuleiro, 0, posx, posy);

            pacs = cria_pacote((uchar*)"", 0, 0, ACK);
            if (!(manda_pacote(soq, pacs))) return 0;

            uchar nome[64]; strcpy((char*)nome, (char*)pacr->dados);
            strcat((char*)nome, "copia");
            FILE* arq = fopen((char*)nome, "w");

            recebe_arquivo(soq, arq);
            destroi_pacote(pacr);
            fclose(arq);
            arq_recebidos++;
        } else if (ack == DADOS) exit(1);
    }

    restaurarModoTerminal();


    tabuleiro = libera_tabuleiro(tabuleiro);
    return 0;
}