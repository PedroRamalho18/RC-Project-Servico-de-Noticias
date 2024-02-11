#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define MAX_COMMAND_SIZE 1024
#define MAX_SUBSCRIPTIONS 500
//DECLARAÇÃO DE FUNÇÕES--------------------------------------------------------------------------------------------------
void leitor();
void erro(char* msg);
void jornalista();

void *readNewsThreadFunct();
//VARIÁVEIS GLOBAIS------------------------------------------------------------------------------------------------------
int server_socket;
int socket_UDP;
struct hostent *hostPtr;
struct sockaddr_in server_addr, udp_aadr;
pthread_t *readNewsThread;
struct ip_mreq IPs_subscribed[500];
int subsCount;
//FUNÇÃO QUE IMPRIME OS ERROS E FAZ EXIT--------------------------------------------------------------------------------
void erro(char* msg){
    printf(">> ERRO: %s\n", msg);
    close(socket_UDP);
    close(server_socket);
    exit(-1);
}

//FUNÇÃO DO LEITOR------------------------------------------------------------------------------------------------------
void leitor(){
    char message[MAX_COMMAND_SIZE];
    char opt1[] = "LIST_TOPICS", opt2[] = "SUBSCRIBE_TOPIC", opt3[] = "EXIT";
    subsCount=0;
    int server_msg;
    pthread_create((pthread_t *) &readNewsThread, 0, readNewsThreadFunct, 0);
    while (1) {
        printf(">> INSIRA A OPÇÃO QUE PRETENDE EXECUTAR\n");
        scanf("%[^\n]", message);
        getchar();
        write(server_socket, message, 1+strlen(message));
        // OBTENÇÃO DAS DUAS COMPONENTES CASO SE INDIQUE A OPT2
        char *token;
        token = strtok(message, " ");

        //OPÇÃO DE LISTAR TÓPICOS
        if (strcmp(token, opt1) == 0) {
            server_msg=read(server_socket, message, MAX_COMMAND_SIZE-1);
            message[server_msg]='\0';
            printf(">> %s\n", message);
            fflush(stdout);
        }
            //OPÇÃO DE SUBSCREVER TÓPICO
        else if(strcmp(token, opt2)==0) {
            server_msg=read(server_socket, message, MAX_COMMAND_SIZE-1);
            message[server_msg]='\0';
            if(strcmp(message,"ERRO AO SUBSCREVER TÓPICO - DADOS EM FALTA\n")==0){
                printf(">> %s\n", message);
                fflush(stdout);
            }
            else {
                char adress[16];
                strcpy(adress, message);
                //ASSOCIAR O IP AO GRUPO MULTICAST E ASSOCIAR O SOCKET UDP A ELE (UDP FOI CRIADO NO MAIN E A LEITURA DAS MENSAGENS ESTÁ NUMA THREAD)
                IPs_subscribed[subsCount].imr_multiaddr.s_addr = inet_addr(adress);
                IPs_subscribed[subsCount].imr_interface.s_addr = INADDR_ANY;
                if (setsockopt(socket_UDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, &IPs_subscribed[subsCount],
                               sizeof(IPs_subscribed[subsCount])) < 0) {
                    erro("PROBLEMA NA ASSOCIAÇÃO DO SOCKET AO GRUPO MULTICAST");
                }
                subsCount++;
            }
        }
            //OPÇÃO DE SAIR
        else if (strcmp(token, opt3) == 0) {
            //SAIR DOS GRUPOS MULTICAST
            for (int s=0; s<subsCount; s++){
                if(setsockopt(socket_UDP, IPPROTO_IP, IP_DROP_MEMBERSHIP, &IPs_subscribed[s], sizeof(IPs_subscribed[s]))<0){
                    erro("PROBLEMA AO SAIR DE GRUPO MULTICAST");
                }
            }
            close(socket_UDP);
            close(server_socket);
            exit(0);
        }
        else{
            server_msg=read(server_socket, message, MAX_COMMAND_SIZE-1);
            message[server_msg]='\0';
            printf(">> %s\n", message);
            fflush(stdout);
        }
    }

}

//FUNÇÃO DA THREAD DE LEITURA PARA ESTAR SEMRPE A LER MENSAGENS RECEBIDAS PELO UDP
void *readNewsThreadFunct(){
    char msg[1024];
    while(1){
        int n_msg;
        socklen_t addrlen=sizeof (udp_aadr);
        if((n_msg= recvfrom(socket_UDP, msg, sizeof(msg),0,(struct sockaddr *)&udp_aadr, &addrlen))<0){
            erro("FALHA AO LER DO GRUPO MULTICAST");
        }
        printf(">> NOTÍCIA RECEBIDA: %s\n", msg);
    }
}

//FUNÇÃO DO JORNALISTA--------------------------------------------------------------------------------------------------
void jornalista() {
    char message[MAX_COMMAND_SIZE];
    char opt1[] = "LIST_TOPICS", opt2[] = "SUBSCRIBE_TOPIC", opt3[] = "EXIT", opt4[] = "CREATE_TOPIC", opt5[] = "SEND_NEWS";
    subsCount=0;
    int server_msg;
    pthread_create((pthread_t *) &readNewsThread, 0, readNewsThreadFunct, 0);
    while (1) {
        //message[0]='\0';
        printf(">> INSIRA A OPÇÃO QUE PRETENDE EXECUTAR\n");
        scanf("%[^\n]", message);
        getchar();

        write(server_socket, message, 1+strlen(message));
        // OBTENÇÃO DAS DUAS COMPONENTES CASO SE INDIQUE A OPT2
        char *token;
        token = strtok(message, " ");


        //OPÇÃO DE LISTAR TÓPICOS
        if (strcmp(token, opt1) == 0) {
            server_msg=read(server_socket, message, MAX_COMMAND_SIZE-1);
            message[server_msg]='\0';
            printf(">> %s\n", message);
            fflush(stdout);
        }
            //OPÇÃO DE SUBSCREVER TÓPICO
        else if(strcmp(token, opt2)==0) {
            server_msg = read(server_socket, message, MAX_COMMAND_SIZE - 1);
            message[server_msg] = '\0';
            if (strcmp(message, "ERRO AO SUBSCREVER TÓPICO - DADOS EM FALTA\n") == 0) {
                printf(">> %s\n", message);
                fflush(stdout);
            } else {
                char adress[16];
                strcpy(adress, message);
                //ASSOCIAR O IP AO GRUPO MULTICAST E ASSOCIAR O SOCKET UDP A ELE (UDP FOI CRIADO NO MAIN E A LEITURA DAS MENSAGENS ESTÁ NUMA THREAD)
                IPs_subscribed[subsCount].imr_multiaddr.s_addr = inet_addr(adress);
                IPs_subscribed[subsCount].imr_interface.s_addr = INADDR_ANY;
                if (setsockopt(socket_UDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, &IPs_subscribed[subsCount],
                               sizeof(IPs_subscribed[subsCount])) < 0) {
                    erro("PROBLEMA NA ASSOCIAÇÃO DO SOCKET AO GRUPO MULTICAST");
                }
                subsCount++;
            }
        }
            //OPÇÃO DE SAIR
        else if (strcmp(token, opt3) == 0) {
            //SAIR DOS GRUPOS MULTICAST
            for (int s=0; s<subsCount; s++){
                if(setsockopt(socket_UDP, IPPROTO_IP, IP_DROP_MEMBERSHIP, &IPs_subscribed[s], sizeof(IPs_subscribed[s]))<0){
                    erro("PROBLEMA AO SAIR DE GRUPO MULTICAST");
                }
            }
            close(socket_UDP);
            close(server_socket);
            exit(0);
        }
            //OPÇÃO DE CRIAR TÓPICO
        else if (strcmp(token, opt4) == 0) {
            message[0]='\0';
            server_msg=read(server_socket, message, MAX_COMMAND_SIZE-1);
            message[server_msg]='\0';
            printf(">> %s\n", message);
            fflush(stdout);
        }
        //OPÇÃO DE ENVIAR NOTICIAS
        else if (strcmp(token, opt5) == 0) {
            server_msg=read(server_socket, message, MAX_COMMAND_SIZE-1);
            message[server_msg]='\0';
            printf(">> %s\n", message);
            fflush(stdout);
        }
        else {
            server_msg=read(server_socket, message, MAX_COMMAND_SIZE-1);
            message[server_msg]='\0';
            printf(">> %s\n", message);
            fflush(stdout);
        }
    }
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        erro(">> DEVE UTILIZAR O COMANDO: news_client <endereço do servidor> <PORTO_NOTICIAS>\n");
    }
//INICIALIZAÇÃO DE VARIÁVEIS -------------------------------------------------------------------------------------------

    char message[MAX_COMMAND_SIZE];
    char username[100];
    char pass[100];

    if ((hostPtr = gethostbyname(argv[1])) == 0) {
        erro("FALHA AO OBTER ENDEREÇO");
    }
//CRIAÇÃO DO SOCKET CLIENTE --------------------------------------------------------------------------------------------
    bzero((void *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ((struct in_addr *) (hostPtr->h_addr))->s_addr;
    server_addr.sin_port = htons((short) atoi(argv[2]));

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        erro("ERRO AO CRIAR O SOCKET");
    }

//CRIAÇÃO DO SOCKET UDP PARA ASSOCIAR AOS GRUPOS MULTICAST
    if((socket_UDP=socket(AF_INET, SOCK_DGRAM, 0))<0){
        erro("ERRO AO CRIAR SOCKET UDP");
    }
    memset(&udp_aadr, 0, sizeof(udp_aadr));
    int reuse = 1;
    if(setsockopt(socket_UDP, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0){
        erro("PROBLEMA AO REUTILIZAR ADDR UDP");
    }
    udp_aadr.sin_family=AF_INET;
    udp_aadr.sin_addr.s_addr=INADDR_ANY;
    udp_aadr.sin_port=htons(5000);
    if(bind(socket_UDP, (struct sockaddr *)&udp_aadr, sizeof(udp_aadr))<0){
        erro("PROBLEMA NO BIND DO SOCKET UDP");
    }


    if (connect(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        erro("ERRO A CONECTAR AO SERVIDOR");
    }

//RECEBER AS CREDENCIAIS DO UTILIZADOR -----------------------------------------------------------------------------------------
    printf(">> INSIRA O SEU USERNAME:\n");
    scanf("%s", username);
    printf(">> INSIRA A SUA PASSWORD:\n");
    scanf("%s", pass);
    message[0]='\0';
    strcat(message, username);
    strcat(message, " ");
    strcat(message, pass);

//ENVIAR AS CREDENCIAIS AO SERVER --------------------------------------------------------------------------------------------------------
    write(server_socket, message, 1 + strlen(message));

//RECEBER VALIDAÇÃO --------------------------------------------------------------------------------------------------------
    message[0]='\0';
    read(server_socket, message, sizeof(message));
    printf("%s\n", message);
    if (strcmp(message, "PASSWORD ERRADA") == 0 || strcmp(message, "UTILIZADOR NAO ENCONTRADO") == 0) {
        printf(">> %s\n", message);
        close(server_socket);
        exit(-1);
    }

//VERIFICAR SE É EDITOR OU LEITOR --------------------------------------------------------------------------------------------------------
    message[0]='\0';
    read(server_socket, message, sizeof(message));
    if (strcmp(message,"A SUA PERMISSÃO: CLIENTE") == 0) {
        printf(">> %s\n", message);
        leitor();
    } else if (strcmp(message, "A SUA PERMISSÃO: JORNALISTA") == 0) {
        printf(">> %s\n", message);
        jornalista();
    } else if (strcmp(message, "A SUA PERMISSÃO: ADMINISTRADOR - PARA ACEDER À CONSOLA DE ADMINISTRAÇÃO, UTILIZE LIGAÇÃO UDP") == 0) {
        printf(">> CREDENCIAIS RECEBIDAS CORRESPONDEM A ADMINISTRADOR. TENTE ACEDER POR UDP\n");
    }else{
        printf(">> ERRO NA COMUNICAÇÃO COM TCP\n");
    }

    return 0;
}