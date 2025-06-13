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

// se estiver rodando em loopback o programa muda pra 1
// eh usado para tirar da rede msgs duplicadas
char eh_loopback = 0;


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

/* recebe um nome de arquivo sem extensao e encontra um 
   arquivo no dir atual com essa extensao. Coloca o nome
   de arquivo completo, com extensao, no parametro nome e 
   retorna o tipo da extensao                              */
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
        printf("ERRO AO ABRIR DIRETORIO\n");
        exit(1);
    }
    strcpy((char*)nome, base);
    if (tipo == IMAGEM) strcat((char*)nome, ".jpg");
    else if (tipo == TEXTO) strcat((char*)nome, ".txt");
    else if (tipo == VIDEO) strcat((char*)nome, ".mp4");
    return tipo;
}

/* manda o arquivo arq. */
int manda_arquivo(int soq, FILE* arq, int* seq, pacote** ultimo) {
    printf("Enviando arquivo\n");

    uchar buffer[DADOS_TAM_MAX];
    pacote *pacr, *pacs; // pacote recieved/sent
    int bytes_lidos;
    char ack;
    char to = 0;

    // enquanto ler algo do arquivo
    while ((bytes_lidos = fread(buffer, 1, DADOS_TAM_MAX, arq)) > 0) {
        ack = NACK;
        
        // loop para enviar os dados e esperar um ack
        while (ack != ACK) {
             // pacote com dados a serem enviados
            if (!(pacs = cria_pacote(buffer, bytes_lidos, *seq, DADOS))) return 0;

            if (!manda_pacote(soq, pacs, eh_loopback)) return 0;

            to = 1;
            while (to == 1) {pacr = recebe_pacote(soq, eh_loopback, &to);}

            // se tiver diferenca no checksum admite que eh um NACK e manda denovo 
            if (pacr->checksum == calcula_checksum(pacr)) {
                if (pacr->sequencia == *seq) { // cliente nao recebeu os dados
                    pacs = destroi_pacote(pacs);
                    pacr = destroi_pacote(pacr);
                    continue;
                }
                if (pacr->tipo == ACK) ack = ACK;
                *seq = pacr->sequencia;
            }

            pacs = destroi_pacote(pacs);
            pacr = destroi_pacote(pacr);
        }
    }

    // mandando msg de fim de arquivo
    ack = NACK;
    while (ack != ACK) {
        pacs = destroi_pacote(pacs);
        if (!(pacs = cria_pacote((uchar*)"", 0, *seq, FIM_ARQ))) return 0;

        if (!manda_pacote(soq, pacs, eh_loopback)) return 0;

        to = 1;
        while (to == 1) {pacr = recebe_pacote(soq, eh_loopback, &to);}

        if (pacr->checksum == calcula_checksum(pacr)) {
            if (pacr->sequencia == *seq) { // cliente nao recebeu os FIM_ARQ
                pacs = destroi_pacote(pacs);
                pacr = destroi_pacote(pacr);
                continue;
            }
            if (pacr->tipo == ACK) ack = ACK;
            *seq = pacr->sequencia;
        }
        pacr = destroi_pacote(pacr);
    }
    *ultimo = destroi_pacote(*ultimo);
    *ultimo = pacs;

    printf("Terminou de enviar\n");
    return 1;
}

int main(int argc, char** argv){
    if (argc != 2) {printf("USO: ./%s <interface_de_rede>\n", argv[0]); return 1;}
    if (!strcmp(argv[1], "lo")) eh_loopback = 1;

    srand(time(NULL));
    int soq = cria_raw_socket(argv[1]);

    int posx = 0, posy = 0, arq_enviados = 0;
    tile** tabuleiro = inicica_jogo(1);
    movimento* log = inicia_log();
    imprime_mapa(tabuleiro, log, 1, posx, posy, arq_enviados);

    pacote *pacr = NULL, *pacs = NULL; // pacote recieved/sent
    pacote *ultimo = NULL;
    int ack; int ultima_seq = 100;
    char to = 0; // flag de timeout
    while (arq_enviados < 8) {
        ack = NACK;

        // recebe pacote falando o movimento
        to = 1;
        while (to == 1) pacr = recebe_pacote(soq, eh_loopback, &to);
        if (calcula_checksum(pacr) == pacr->checksum) {
            ack = ACK;
            if (pacr->sequencia == ultima_seq) {
                manda_pacote(soq, ultimo, eh_loopback);
                pacr = destroi_pacote(pacr);
                continue;
            }
            ultima_seq = pacr->sequencia;

            if (pacr->tipo == CIMA && posy < TAM_TABULEIRO-1) { // verifica direcao e se movimento possivel
                posy++; // altera a posicao do player
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1; // muda o parametro passou caso nao tenha passado
                log = adiciona_passo(log, posx, posy); // adiciona o movimento no log
                ack = OK_ACK;
            } else if (pacr->tipo == ESQUERDA && posx > 0) {
                posx--;
                log = adiciona_passo(log, posx, posy);
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1;
                ack = OK_ACK;
            } else if (pacr->tipo == BAIXO && posy > 0) {
                posy--;
                log = adiciona_passo(log, posx, posy);
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1;
                ack = OK_ACK;
            }
            else if (pacr->tipo == DIREITA && posx < TAM_TABULEIRO-1) {
                posx++;
                log = adiciona_passo(log, posx, posy);
                if (tabuleiro[posx][posy].passou == 0) tabuleiro[posx][posy].passou = 1;
                ack = OK_ACK;
            }

            // se tiver tesouro na tile que nao tenha sido ja pego
            if (tabuleiro[posx][posy].tem_tesouro && tabuleiro[posx][posy].passou != 2) {
                tabuleiro[posx][posy].passou = 2; // 2 significa que tinha tesouro e ja foi pego
                imprime_mapa(tabuleiro, log, 1, posx, posy, arq_enviados);

                uchar nome[64];
                // descobre o nome completo e tipo do arquivo
                ack = descobrir_extensao(tabuleiro[posx][posy].tesouro_id, nome);
                // abre como binario por padrao
                FILE* arq = fopen((char*)nome, "rb"); if (!arq) {printf("ERRO AO ABRIR ARQ %s", (char*)nome); exit(1);}

                // pacote com tipo do tesouro que sera enviado ao cliente
                if (!(pacs = cria_pacote(nome, strlen((char*)nome)+1, pacr->sequencia, ack))) return 0;


                // loop para avisar o cliente que achou um tesouro e esperar ack
                ack = NACK;
                while (ack != ACK) {
                    pacr = destroi_pacote(pacr);                    
                    manda_pacote(soq, pacs, eh_loopback);
                           
                    to = 1;
                    while (to == 1) pacr = recebe_pacote(soq, eh_loopback, &to);
                    if (pacr->checksum == calcula_checksum(pacr)) {
                        if (pacr->sequencia != ultima_seq) {
                            ack = ACK;
                            ultima_seq = pacr->sequencia;
                        }
                    }
                }
                ultimo = destroi_pacote(ultimo);
                ultimo = pacs;

                manda_arquivo(soq, arq, &ultima_seq, &ultimo);
                arq_enviados++;
                imprime_mapa(tabuleiro, log, 1, posx, posy, arq_enviados);

                fclose(arq);
            }
        }

        if (!(pacs = cria_pacote((uchar*)"", 0, ultima_seq, ack))) return 0;
        if (!(manda_pacote(soq, pacs, eh_loopback))) return 0;

        pacr = destroi_pacote(pacr);
        ultimo = destroi_pacote(ultimo);
        ultimo = pacs;

        imprime_mapa(tabuleiro, log, 1, posx, posy, arq_enviados);
    }
    printf("TODOS OS ARQUIVOS ENVIADOS\n");

    ultimo = destroi_pacote(ultimo);
    log = libera_log(log);
    tabuleiro = libera_tabuleiro(tabuleiro);
    return 0;
}
