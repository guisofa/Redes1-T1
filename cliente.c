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

// se estiver rodando em loopback o programa muda pra 1
// eh usado para tirar da rede msgs duplicadas
char eh_loopback = 0;


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

/* Recebe um arquivo e armazena ele em arq */
int recebe_arquivo(int soq, FILE* arq, int *seq, pacote* ultimo) {
    printf("Recebendo arquivo\n");
    
    pacote *pacr, *pacs; // pacote recieved/sent
    int ultima_seq = *seq - 1; if (ultima_seq < 0) ultima_seq = 31;
    char ack;
    char to = 0;
    
    while (1) {
        ack = NACK;

        pacr = recebe_pacote(soq, eh_loopback, &to); if (to) {manda_pacote(soq, ultimo, eh_loopback); continue;}
        if (calcula_checksum(pacr) == pacr->checksum) { // se o checksum estiver correto
            if (pacr->sequencia == ultima_seq) {
                manda_pacote(soq, ultimo, eh_loopback);
                pacr = destroi_pacote(pacr);
                continue;
            }
            ultima_seq = *seq;
            (*seq)++;
            ack = ACK;
            if (pacr->tipo == FIM_ARQ) // verifica se eh fim de arq
                break;
            // se o checksum estiver correto e nao for fim de arq, escreve no arquivo
            fwrite(pacr->dados, 1, pacr->tamanho, arq);
        }

        if (!(pacs = cria_pacote((uchar*)"", 0, *seq, ack))) return 0;
        if (!(manda_pacote(soq, pacs, eh_loopback))) return 0;

        pacr = destroi_pacote(pacr);
        ultimo = destroi_pacote(ultimo);
        ultimo = pacs;
    }

    // manda o ack do fim de arquivo
    if (!(pacs = cria_pacote((uchar*)"", 0, *seq, ACK))) return 0;
    if (!(manda_pacote(soq, pacs, eh_loopback))) return 0;

    pacr = destroi_pacote(pacr);
    ultimo = destroi_pacote(ultimo);
    ultimo = pacs;

    // recebe o ultimo ack
    ack = NACK;
    while (ack != ACK) {
        pacr = recebe_pacote(soq, eh_loopback, &to); if (to) {manda_pacote(soq, ultimo, eh_loopback); continue;}
        if (calcula_checksum(pacr) != pacr->checksum) {
            manda_pacote(soq, ultimo, eh_loopback);
            pacr = destroi_pacote(pacr);
            continue;
        }
        if (pacr->tipo != ACK) {
            manda_pacote(soq, ultimo, eh_loopback);
            pacr = destroi_pacote(pacr);
            continue;
        }
        pacr = destroi_pacote(pacr);
        break;
    }
    ultimo = destroi_pacote(ultimo);

    printf("Arquivo recebido\n");

    // fiz isso para nao encher o buffer com movimentos pq o usuario nao percebeu que achou arquivo
    printf("APERTE ENTER PARA CONTINUAR\n");
    char c;
    do read(STDIN_FILENO, &c, 1); while (c != '\n');
    return 1;
}

int main(int argc, char** argv){
    if (argc != 2) {printf("USO: ./%s <interface_de_rede>\n", argv[0]); return 1;}
    if (!strcmp(argv[1], "lo")) eh_loopback = 1;
    int soq = cria_raw_socket("lo");


    int posx = 0, posy = 0, arq_recebidos = 0;
    tile** tabuleiro = inicica_jogo(0);
    imprime_mapa(tabuleiro, NULL, 0, posx, posy, arq_recebidos);

    configurarModoRaw();

    char c;
    pacote *pacr, *pacs;
    int seq = 0;
    int ack;
    char to = 0; // flag de timeout
    // loop principal para ler movimento e mandar pacote
    while (arq_recebidos < 8) {
        ack = NACK;
        // le o movimento
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
        
        // manda o pacote e espera ack
        while (ack == NACK) {
            manda_pacote(soq, pacs, eh_loopback);
            
            pacr = recebe_pacote(soq, eh_loopback, &to); if (to) {to = 0; continue;}
            if (pacr->checksum == calcula_checksum(pacr)) {
                if (pacr->tipo != NACK) {
                    ack = pacr->tipo;
                    break;
                }

            }
            pacr = destroi_pacote(pacr);
        }
        pacs = destroi_pacote(pacs);
        seq = (seq + 1) % 32;

        if (ack == ACK) { // se pacote foi enviado com sucesso, mas movimento invalido
            imprime_mapa(tabuleiro, NULL, 0, posx, posy, arq_recebidos);
            pacr = destroi_pacote(pacr);
        }
        else if (ack == OK_ACK) { // se movimento foi realizado
            pacr = destroi_pacote(pacr);
            atualiza_posicao_cliente(tabuleiro, c, &posx, &posy);
            imprime_mapa(tabuleiro, NULL, 0, posx, posy, arq_recebidos);
        } 
        else if (ack == IMAGEM || ack == TEXTO || ack == VIDEO) { // se movimento foi realizado e tinha um tesouro
            atualiza_posicao_cliente(tabuleiro, c, &posx, &posy);
            imprime_mapa(tabuleiro, NULL, 0, posx, posy, arq_recebidos);

            // mandando ack para dizer que entendemos que vamos receber um arquivo
            pacs = cria_pacote((uchar*)"", 0, seq, ACK);
            if (!(manda_pacote(soq, pacs, eh_loopback))) return 0;

            // pega nome do arquivo dos dados e abrimos o arquivo
            uchar nome[64]; strcpy((char*)nome, (char*)pacr->dados);
            strcat((char*)nome, "copia");
            FILE* arq = fopen((char*)nome, "wb");

            recebe_arquivo(soq, arq, &seq, pacs);
            seq = (seq + 1) % 32;
            arq_recebidos++;
            imprime_mapa(tabuleiro, NULL, 0, posx, posy, arq_recebidos);

            pacs = NULL; // depois de recebe_arquivo pacs esta apontando para bloco liberado
            pacr = destroi_pacote(pacr);
            fclose(arq);

        } else if (ack == DADOS) exit(1); // usei pra debuggar, deixa ai por enquanto
    }
    printf("TODOS OS ARQUIVOS RECEBIDOS\n");

    restaurarModoTerminal();


    tabuleiro = libera_tabuleiro(tabuleiro);
    return 0;
}