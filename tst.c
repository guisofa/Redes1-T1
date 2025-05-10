#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pacote.h"
#include <unistd.h>
#include <termios.h>

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


/* minha ideia era tirar o char* msg, int tam para chamar a funcao somente com o pacote. separar
a criação do pacote do envio, assim da pra criar pacotes com tipos variados sem misturar tanto
as coisas.

seguindo nessa ideia a funcao cria_pacote é usada fora dessa funcao para criar um pacote de
determinado tipo e depois submetido ao envio
*/
int manda_pacote(int soquete, pacote* pac) {

    uchar* buffer = gera_mensagem(pac);

    /* // nao tenho ideia do que isso faz, mas não funciona sem.
          update: agora funciona, mas vou deixar aqui so pra ter certeza
    struct sockaddr_ll destino = {0};
    destino.sll_family = AF_PACKET;
    destino.sll_protocol = htons(ETH_P_ALL);
    destino.sll_ifindex = if_nametoindex("lo");
    */

    // se o tamanho da msg for menor que TAM_MIN entao bytes extras foram colocados
    if (send(soquete, buffer, (pac->tamanho+4 < TAM_MIN) ? TAM_MIN : (pac->tamanho + 4), 0) == -1) return -1;
    
    //destroi_pacote(pac);
    free(buffer);

    return 0;
}

pacote* recebe_pacote(int soquete){
    uchar buffer[PACOTE_TAM_MAX];

    while (1) {
        int bytes_recebidos = recv(soquete, buffer, PACOTE_TAM_MAX, 0);
        if (bytes_recebidos < 0) printf("erro recv\n");
        if (bytes_recebidos >= 4/*tamanho minimo da msg*/ && buffer[0] == MARCADORINI) {
            pacote* pac = gera_pacote(buffer);
            printf("recebido:\n");
            printf("marcador:  %#02X\n", MARCADORINI);
            printf("tamanho:   %d\n", pac->tamanho);
            printf("sequencia: %d\n", pac->sequencia);
            printf("tipo:      %d\n", pac->tipo);
            printf("dados: ");
            for (int i = 0; i < pac->tamanho; i++) {
                printf("%c", pac->dados[i]);
            }
            printf("\n");
            return pac;;
        }
    }

}

int pacote_erro(pacote* pac){ return 0;}

pacote* recebe_mensagem(int soquete) {
    pacote* pac;
    pacote* nack;

    pac = recebe_pacote(soquete);

    while (pacote_erro(pac)) {
        destroi_pacote(pac);
        nack = cria_pacote("", 0, 1);
        manda_pacote(soquete, nack);
        pac = recebe_pacote(soquete);
        destroi_pacote(nack);
    }

    return pac;
}

int main() {
    int soq = cria_raw_socket("lo");
    pacote* meuPacote;
    pacote* resposta;

    meuPacote = cria_pacote("alo", 4, 0);
    
    if (manda_pacote(soq, meuPacote) == -1) {printf("Erro ao mandar pacote\n"); return 1;}

    recebe_mensagem(soq);

    destroi_pacote(meuPacote);

    configurarModoRaw();

    printf("Pressione teclas (Q para sair):\n");

    char c;
    while (1) {
        read(STDIN_FILENO, &c, 1);
        if (c == 'w') {
            printf("Você apertou W!\n");
            meuPacote = cria_pacote("w", 2, 0);
        } else if (c == 'a') {
            printf("Você apertou A!\n");
            meuPacote = cria_pacote("a", 2, 0);
        } else if (c == 's') {
            printf("Você apertou S!\n");
            meuPacote = cria_pacote("s", 2, 0);
        } else if (c == 'd') {
            printf("Você apertou D!\n");
            meuPacote = cria_pacote("d", 2, 0);
        } else if (c == 'q') {
            printf("Saindo...\n");
            break;
        }
        if(meuPacote){
            manda_pacote(soq, meuPacote);

            resposta = recebe_mensagem(soq);
            /*
                recebe resposta e processa, se for ok move para onde quer mover e atualiza uma interface
                se nao for ok n faz nada apenas deleta o pacote e tenta de novo

                o timeout precisa ser do lado do cliente
            */

            if(resposta->tipo == 2){
                printf("permite ele mover para a direção solicitada\n");
            }

            destroi_pacote(meuPacote);
            destroi_pacote(resposta);
        }
    }

    restaurarModoTerminal();
    return 0;
}
