#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define BUF_SIZE 1024
#define BUF_INFOS 100
#define MAX_INPUT_SIZE 100
#define MAX_USERS 1000
#define MAX_TOPICS 500

//DECLARAÇÃO DE FUNÇÕES
void countFileLines(char fileName[]);
void saveChanges(char filename[]);
void accessAdminConsole(int porto_config, char filename[]);
void userAccess(int client);
void accessReader(int client, int pox);
void accessEditor(int client, int pox);
int generateIP();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct USER{
    char nome[100];
    char password[100];
    char permission[100];
    int posicao;
} USER;



typedef struct TOPIC{
    char nome[100];
    int ID;
    char adress[16];
    struct sockaddr_in sock_udp;
    int sockID;
    USER ReadersSubscribed[1000];
    USER associatedEditors[100];
} TOPIC;

USER *usersList;
TOPIC *topicsList;
int *usersCount;
int *topicsCount;
int shmID;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//FUNÇÃO QUE IMPRIME OS ERROS E FAZ EXIT
void erro(char* msg){
    printf(">> ERRO: %s\n", msg);
    exit(-1);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//FUNÇÃO PARA CONTAR QUANTAS LINHAS TEM O FICHEIRO, OU SEJA, QUANTOS USERS ESTÃO REGISTADOS
void countFileLines(char fileName[]){
    FILE *configFile;
    configFile= fopen(fileName, "r");
    if (configFile==NULL){
        erro("FALHA NA ABERTURA DO FICHEIRO DE CONFIGURAÇÃO");
    }
    char lineFromFile[100];
    while(fgets(lineFromFile, sizeof(lineFromFile),configFile)!=NULL){
        (*usersCount)++;
    }
    fclose(configFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
////FUNÇÃO PARA ATUALIZAR FICHEIRO DE CONFIGURAÇÃO
void saveChanges(char filename[]){
    //ABRIR O FICHEIRO EM MODO WRITE PARA APAGAR TUDO E ESCREVER A NOVA LISTA
    FILE *configFile;
    configFile = fopen(filename, "w");
    if (configFile == NULL) {
        erro("FALHA NA ABERTURA DO FICHEIRO DE CONFIGURAÇÃO");
    }
    //ESCRITA DA LISTA PARA O FICHEIRO
    for(int i=0; i<*usersCount; i++){
        fprintf(configFile, "%s;%s;%s\n", usersList[i].nome, usersList[i].password, usersList[i].permission);
    }
    fclose(configFile);
}
/**---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//FUNÇÃO PARA COMUNICAR COM A CONSOLA DE ADDMINISTRRAÇÃO
void accessAdminConsole(int porto_config, char filename[]) {
    //INICIALIZAÇÃO DE VARIÁVEIS
    struct sockaddr_in serverUDP, admin;
    socklen_t cLen = sizeof(admin);
    char buf_udp[BUF_SIZE];
    int listenUDP, bind_udp;
    int msg_recebida;
    char utilizador[100], password[100], permission[100];

/**-------------------------------SOCKET UDP------------------------------------------------------------------*/
    //CRIAÇÃO DO SOCKET UDP
    if((listenUDP=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0){
        erro("PROBLEMA AO CRIAR SOCKET UDP");
    }

    //PREENCHIMENTO DO SOCKET
    serverUDP.sin_family=AF_INET;
    serverUDP.sin_addr.s_addr=INADDR_ANY;
    serverUDP.sin_port=htons(porto_config);

    //BINDING DO SKET NUM ENDEREÇO DE MEMÓRIA
    if((bind_udp = bind(listenUDP, (struct sockaddr *) &serverUDP, sizeof(serverUDP)))<0){
        close(listenUDP);
        erro("PROBLEMA NO BIND DE TCP");
    }
/**--------------------------------------ESPERAR LIGAÇÃO AO SOCKET-----------------------------------------------------------*/
    while(1) {
        //ESPERA RECEÇÃO DE MENSAGEM COM USERNAME
        if ((msg_recebida = recvfrom(listenUDP, buf_udp, BUF_SIZE, 0, (struct sockaddr *) &admin,
                                     (socklen_t *) &cLen)) == -1) {
            close(listenUDP);
            erro("FALHA AO RECEBER MENSAGEM UDP");
        }
        buf_udp[msg_recebida - 1] = '\0';
        strcpy(utilizador, buf_udp);
/**-----------------------------------------------------VERIFICAR OS DADOS DO CLIENTE--------------------------------------------*/
        //VERIFICAÇÃO DO USERNAME E DA PASS
        int comp_user = -1, comp_pass = -1;
        for (int i = 0; i < *usersCount; i++) {
            comp_user = strcmp(utilizador, usersList[i].nome);
            if (comp_user == 0) {
                strcpy(buf_udp, ">> UTILIZADOR ENCONTRADO. INSIRA A SUA PASSWORD:\n");
                sendto(listenUDP, (const char *) buf_udp, strlen(buf_udp), 0, (struct sockaddr *) &admin, cLen);
                if ((msg_recebida = recvfrom(listenUDP, buf_udp, BUF_SIZE, 0, (struct sockaddr *) &admin,
                                             (socklen_t *) &cLen)) == -1) {
                    close(listenUDP);close(listenUDP);
                    erro("FALHA AO RECEBER MENSAGEM UDP");
                }
                buf_udp[msg_recebida - 1] = '\0';
                strcpy(password, buf_udp);
                comp_pass = strcmp(password, usersList[i].password);
                if (comp_pass != 0) {
                    strcpy(buf_udp, ">> PASSWORD ERRADA\n");
                    sendto(listenUDP, (const char *) buf_udp, strlen(buf_udp), MSG_CONFIRM, (struct sockaddr *) &admin,
                           cLen);
                    break;
                }
                else{
                    strcpy(buf_udp, ">> PASSWORD CORRETA, AGUARDE...\n");
                    sendto(listenUDP, (const char *) buf_udp, strlen(buf_udp), MSG_CONFIRM, (struct sockaddr *) &admin,
                           cLen);
                    strcpy(permission, usersList[i].permission);
                    break;
                }
            }

        }
        if (comp_user != 0) {
            strcpy(buf_udp, ">> USER NÃO ENCONTRADO\n");
            sendto(listenUDP, (const char *) buf_udp, strlen(buf_udp), 0, (struct sockaddr *) &admin, cLen);
        }

        //VERIFICAÇÃO DE PERMISSÕES DE ADMIN
        if ((strcmp(permission, "administrator")) != 0) {
            strcpy(buf_udp, ">> SEM PERMISSÃO DE ADMINISTRADOR\n");
            sendto(listenUDP, (const char *) buf_udp, strlen(buf_udp), MSG_CONFIRM, (struct sockaddr *) &admin, cLen);
        } else {
            strcpy(buf_udp, ">> PERMISSÃO CONCEDIDA!\n");
            sendto(listenUDP, (const char *) buf_udp, strlen(buf_udp), MSG_CONFIRM, (struct sockaddr *) &admin, cLen);

/**---------------------------------------MENU DA CONSOLA----------------------------------------------------------*/
            //CICLO PARA RECEBER COMANDOS A EXECUTAR NA CONSOLA
            char opcao1[] = "DEL", opcao2[] = "LIST", opcao3[] = "QUIT", opcao4[] = "QUIT_SERVER", opcao5[] = "ADD_USER";
            while (1) {
                strcpy(buf_udp, ">> INSIRA A OPÇÃO QUE PRETENDE EXECUTAR:\n");
                sendto(listenUDP, buf_udp, strlen(buf_udp), 0, (struct sockaddr *) &admin, cLen);

                if ((msg_recebida = recvfrom(listenUDP, (char *) buf_udp, BUF_SIZE, MSG_WAITALL,
                                             (struct sockaddr *) &admin, &cLen)) == -1) {
                    close(listenUDP);
                    erro("FALHA AO RECEBER MENSAGEM UDP");
                }
                buf_udp[msg_recebida - 1] = '\0';

                char option[4][MAX_INPUT_SIZE];
                char *opt;
                opt = strtok(buf_udp, " ");
                int i = 0;
                while (opt != NULL) {
                    strcpy(option[i], opt);
                    opt = strtok(NULL, " ");
                    i++;
                }

                //DELETE
                if (strcmp(option[0], opcao1) == 0) {
                    int pos = -1;
                    for (int l = 0; l < *usersCount; l++) {
                        if ((strcmp(option[1], usersList[l].nome)) == 0) {
                            pos = 1;
                            if ((strcmp(usersList[l].permission, "administrator")) == 0) {
                                strcpy(buf_udp, ">> NÃO PODE ELIMINAR UM ADMINISTRADOR\n");
                                sendto(listenUDP, buf_udp, strlen(buf_udp), 0, (struct sockaddr *) &admin, cLen);
                                break;
                            }
                        }
                        if (pos == 1 && l != *usersCount - 1) {
                            strcpy(usersList[l].nome, usersList[l + 1].nome);
                            strcpy(usersList[l].password, usersList[l + 1].password);
                            usersList[l].posicao = usersList[l + 1].posicao;
                        }
                        if (l == *usersCount - 1) {
                            (*usersCount)--;
                            break;
                        }
                    }
                }

                    //LIST
                else if (strcmp(option[0], opcao2) == 0) {
                    for (int j = 0; j < *usersCount; j++) {
                        sprintf(buf_udp, ">> username:%s, password:%s, permissão:%s\n", usersList[j].nome,
                                usersList[j].password, usersList[j].permission);
                        sendto(listenUDP, buf_udp, strlen(buf_udp), 0, (struct sockaddr *) &admin, cLen);
                    }
                }

                    //QUIT
                else if (strcmp(option[0], opcao3) == 0) {
                    strcpy(buf_udp, ">> A ENCERRAR CONSOLA...\n");
                    sendto(listenUDP, buf_udp, strlen(buf_udp), 0, (struct sockaddr *) &admin, cLen);
                    saveChanges(filename);
                    break;
                }

                    //QUIT_SERVER
                else if (strcmp(option[0], opcao4) == 0) {
                    printf(">> A ENCERRAR SEVIRDOR...\n");
                    saveChanges(filename);
                    exit(0);
                }

                    //ADD_USER
                else if (strcmp(option[0], opcao5) == 0) {
                    int pox = *usersCount;
                    strcpy(usersList[pox].nome, option[1]);
                    strcpy(usersList[pox].password, option[2]);
                    strcpy(usersList[pox].permission, option[3]);
                    (*usersCount)++;
                } else {
                    strcpy(buf_udp,
                           ">> OPÇÃO INVÁLIDA. OPÇÕES QUE DEVE UTILIZAR: DEL {nome} ; LIST ; ADD_USER {nome} {palavra passe} {permissão} ; QUIT ; QUIT_SERVER\n");
                    sendto(listenUDP, buf_udp, strlen(buf_udp), 0, (struct sockaddr *) &admin, cLen);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//FUNÇÃO PARA ACESSO DO UTILIZADOR
void userAccess(int client){
    //INICIALIZAÇÃO DE VARIÁVEIS
    char buf_tcp[BUF_SIZE];
    int  client_msg=0;
    char utilizador[100], password[100], permission[100]; //VARIÁVEIS PARA GUARDAR AS CREDENCIAIS DO CLIENTE
    int pox_user_logged;

    //RECEBER CREDENCIAIS DO CLIENTE
    client_msg=read(client, buf_tcp, sizeof(buf_tcp)-1);
    if(client_msg<0){
        erro("FALHA AO RECEBER MENSAGEM TCP");
    }

    //OBTENÇÃO DO USERNAME E PASSWORD
    char* token = strtok(buf_tcp, " ");
    strcpy(utilizador, token);
    token = strtok(NULL, " ");
    strcpy(password, token);
    buf_tcp[0]='\0';
    //VERIFICAÇÃO DAS CREDENCIAIS
    int comp_user = -1, comp_pass=-1;//VARIÁVEL PARA COMPARAR O USERNAME INSERIDO COM OS CONSTANTES NA LISTA
    int number=*usersCount;
    for (int i = 0; i <number; i++) {
        comp_user = strcmp(utilizador, usersList[i].nome);
        if (comp_user == 0){
            comp_pass = strcmp(password, usersList[i].password);
            if(comp_pass!=0){
                strcpy(buf_tcp, "PASSWORD ERRADA");
                if((write(client, buf_tcp, sizeof (buf_tcp)))<0){
                    erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                }
            }
            else {
                strcpy(buf_tcp, "UTILIZADOR ENCONTRADO, AGUARDE...");
                if((write(client, buf_tcp, sizeof (buf_tcp)))<0){
                    close(client);
                    erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                }
                strcpy(permission, usersList[i].permission);
                pox_user_logged=i;
                break;
            }
        }
    }

    if(comp_user!=0){
        buf_tcp[0]='\0';
        strcpy(buf_tcp, "UTILIZADOR NAO ENCONTRADO");
        if((write(client, buf_tcp, sizeof (buf_tcp)))<0){
            close(client);
            erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
        }
    }

    //VERIFICAÇÃO DAS PERMISSÕES
    if((strcmp(permission, "reader"))==0){
        strcpy(buf_tcp, "A SUA PERMISSÃO: CLIENTE");
        if((write(client, buf_tcp, sizeof(buf_tcp)))<0){
            close(client);
            erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
        }
        accessReader(client, pox_user_logged);
    }
    else if((strcmp(permission, "publisher"))==0){
        strcpy(buf_tcp, "A SUA PERMISSÃO: JORNALISTA");
        if((write(client, buf_tcp, sizeof(buf_tcp)))<0){
            close(client);
            erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
        }
        accessEditor(client, pox_user_logged);
    }
    else if((strcmp(permission, "administrator"))==0){
        strcpy(buf_tcp, "A SUA PERMISSÃO: ADMINISTRADOR - PARA ACEDER À CONSOLA DE ADMINISTRAÇÃO, UTILIZE LIGAÇÃO UDP");
        if((write(client, buf_tcp, strlen(buf_tcp)))<0){
            close(client);
            erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
        }
    }
}

void accessReader(int client, int pox_user_logged){
    char buf_tcp[BUF_SIZE];
    char opt1[]="LIST_TOPICS", opt2[]="SUBSCRIBE_TOPIC", opt3[]="EXIT";

    while(1) {
        //RECEBER OPÇÃO ESCOLHIDA PELO CLIENTE
        buf_tcp[0]='\0';
        int client_msg=0;
        client_msg=read(client, buf_tcp, BUF_SIZE-1);

        char *token;
        token=strtok(buf_tcp," ");
        fflush(stdout);

        //LISTAR TÓPICOS
        if(strcmp(token, opt1)==0){
            buf_tcp[0]='\0';
            char topic_info[118];
            strcpy(buf_tcp, "TOPICS AVAILABLE: TOPIC -> ID\n");
            for (int t=0; t<*topicsCount; t++){
                sprintf(topic_info, "\t %s -> %d\n", topicsList[t].nome,topicsList[t].ID);
                strcat(buf_tcp, topic_info);
            }
            if(write(client, buf_tcp, 1+strlen(buf_tcp))<0){
                erro("PROBLEMAS A ESCREVER PARA O CLIENTE");
            }
        }

            //SUBSCREVER TÓPICO
        else if(strcmp(token, opt2)==0){
            int ID;
            token= strtok(NULL, " ");
            if(token==NULL){
                buf_tcp[0] = '\0';
                strcpy(buf_tcp, "ERRO AO SUBSCREVER TÓPICO - DADOS EM FALTA\n");
                if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                    close(client);
                    erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                }
            }
            int ID_recebido=atoi(token);
            for (int t=0; t<*topicsCount; t++) {
                if(topicsList[t].ID==ID_recebido){
                    int pox=0;
                    while(strcmp(topicsList[t].ReadersSubscribed[pox].nome, "")!=0){
                        pox++;
                    }
                    topicsList[t].ReadersSubscribed[pox]=usersList[pox_user_logged];
                    buf_tcp[0]='\0';
                    strcpy(buf_tcp, topicsList[t].adress);
                    if((write(client, buf_tcp, 1+strlen(buf_tcp)))<0){
                        close(client);
                        erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                    }
                    break;
                }
            }
        }
        else if(strcmp(token, opt3)==0){
            return;
        }
        else{
            strcpy(buf_tcp, "FALHA NO SERVIDOR! OPÇÃO RECEBIDA INVÁLIDA\n");
            if((write(client, buf_tcp, strlen(buf_tcp)))<0){
                close(client);
                erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
            }

        }
    }
}

void accessEditor(int client, int pox_user_logged){
    char buf_tcp[BUF_SIZE];
    char opt1[]="LIST_TOPICS", opt2[]="SUBSCRIBE_TOPIC", opt3[]="CREATE_TOPIC", opt4[]="SEND_NEWS",opt5[]="EXIT";

    while(1) {
        //RECEBER OPÇÃO ESCOLHIDA PELO CLIENTE
        CONTINUE:
        buf_tcp[0]='\0';
        int client_msg=0;
        client_msg=read(client, buf_tcp, BUF_SIZE-1);

        char *token;
        token=strtok(buf_tcp," ");
        fflush(stdout);

        //LISTAR TÓPICOS
        if(strcmp(token, opt1)==0){
            buf_tcp[0]='\0';
            char topic_info[118];
            strcpy(buf_tcp, "TOPICS AVAILABLE: TOPIC -> ID\n");
            for (int t=0; t<*topicsCount; t++){
                sprintf(topic_info, "\t %s -> %d\n", topicsList[t].nome,topicsList[t].ID);
                strcat(buf_tcp, topic_info);
            }
            if(write(client, buf_tcp, 1+strlen(buf_tcp))<0){
                erro("PROBLEMAS A ESCREVER PARA O CLIENTE");
            }
        }

        //SUBSCREVER TÓPICO
        else if(strcmp(token, opt2)==0) {
            int ID;
            token = strtok(NULL, " ");
            if (token == NULL) {
                buf_tcp[0] = '\0';
                strcpy(buf_tcp, "ERRO AO SUBSCREVER TÓPICO - DADOS EM FALTA\n");
                if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                    close(client);
                    erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                }
            }
            else{
                int ID_recebido = atoi(token);
                for (int t = 0; t < *topicsCount; t++) {
                    if (topicsList[t].ID == ID_recebido) {
                        int pox = 0;
                        while (strcmp(topicsList[t].ReadersSubscribed[pox].nome, "") != 0) {
                            pox++;
                        }
                        topicsList[t].ReadersSubscribed[pox] = usersList[pox_user_logged];
                        buf_tcp[0] = '\0';
                        strcpy(buf_tcp, topicsList[t].adress);
                        if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                            close(client);
                            erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                        }
                        break;
                    }
                }
            }
        }

        //CRIAR TÓPICO
        else if(strcmp(token, opt3)==0) {
            int ID;
            char nome[100];
            token = strtok(NULL, " ");
            ID = atoi(token);
            token = strtok(NULL, " ");
            if (ID==0||token==NULL) {
                buf_tcp[0] = '\0';
                strcpy(buf_tcp, "ERRO AO CRIAR TÓPICO - DADOS EM FALTA\n");
                if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                    close(client);
                    erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                }
            } else {
                strcpy(nome, token);
                for(int t=0; t<*topicsCount; t++){
                    if(topicsList[t].ID==ID){
                        buf_tcp[0] = '\0';
                            strcpy(buf_tcp, "ID DE NOTICIA JÁ REGISTADO\n");
                        if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                            close(client);
                            erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                        }
                        goto CONTINUE;
                    }
                }
                //CRIA NOVA ESTRUTURA DE TÓPICO
                TOPIC newTopic = {"", ID, "", 0};
                strcpy(newTopic.nome, nome);
                newTopic.associatedEditors[0] = usersList[pox_user_logged];//AQUI É PARA GUARDAR O UTILIZADOR NO ARRAY QUE GUARDA OS USERS ASSOCIADOS AO TOPICO
                int position = *topicsCount;
                topicsList[position] = newTopic;//PONTEIRO DA SHARED MEMORY FICA A APONTAR PARA NOVA ESTRUTRA FICANDO ESTA NA SHARED MEMORY
                (*topicsCount)++;
                char address[16] = "224.0.";
                char add[4];
                int add_int;
                add_int = generateIP();
                sprintf(add, "%d", add_int);
                strcat(address, add);
                strcat(address, ".");
                add_int = generateIP();
                sprintf(add, "%d", add_int);
                strcat(address, add);
                strcpy(topicsList[position].adress, address);
                //CRIAÇÃO DO SOCKET PARA O TÓPICO
                if ((topicsList[position].sockID = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                    buf_tcp[0] = '\0';
                    strcpy(buf_tcp, "ERRO AO CRIAR TÓPICO\n");
                    if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                        close(client);
                        erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                    }
                }

                memset(&topicsList[position].sock_udp, 0, sizeof(topicsList[position].sock_udp));
                topicsList[position].sock_udp.sin_family = AF_INET;
                topicsList[position].sock_udp.sin_addr.s_addr = inet_addr(address);
                topicsList[position].sock_udp.sin_port = htons(5000);
                //ASSOCIAÇÃO DO SOCKET AO GRUPO MULTICAST
                int enable = 1;
                if (setsockopt(topicsList[position].sockID, IPPROTO_IP, IP_MULTICAST_TTL, &enable, sizeof(enable)) <
                    0) {
                    buf_tcp[0] = '\0';
                    strcpy(buf_tcp, "ERRO AO CRIAR TÓPICO\n");
                    if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                        close(client);
                        erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                    }
                }
                //ENVIO DE MENSAGEM DE CONFIRMAÇÃO AO CLIENTE
                buf_tcp[0] = '\0';
                strcpy(buf_tcp, "TÓPICO CRIADO\n");
                if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                    close(client);
                    erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                }
            }
        }

        //ENVIAR NOTICIA
        else if(strcmp(token, opt4)==0) {
            int ID;
            char addres[16];
            char noticia[1000];
            token = strtok(NULL, " ");
            ID = atoi(token);
            token = strtok(NULL, "\0");
            if (ID==0||token == NULL) {
                buf_tcp[0] = '\0';
                strcpy(buf_tcp, "ERRO AO ENVIAR NOTICIA - DADOS EM FALTA\n");
                if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                    close(client);
                    erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                }

            } else {
                strcpy(noticia, token);
                int position = 0;
                for (int t = 0; t < *topicsCount; t++) {
                    if (topicsList[t].ID == ID) {
                        position = t;
                        break;
                    }
                }
                buf_tcp[0] = '\0';
                strcpy(buf_tcp, noticia);
                if (sendto(topicsList[position].sockID, buf_tcp, sizeof(buf_tcp), 0,
                           (struct sockaddr *) &topicsList[position].sock_udp, sizeof(topicsList[position].sock_udp)) <
                    0) {
                    buf_tcp[0] = '\0';
                    strcpy(buf_tcp, "FALHA AO ENVIAR NOTICIA\n");
                    if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                        close(client);
                        erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                    }
                } else {
                    buf_tcp[0] = '\0';
                    strcpy(buf_tcp, "NOTICIA ENVIADA\n");
                    if ((write(client, buf_tcp, 1 + strlen(buf_tcp))) < 0) {
                        close(client);
                        erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
                    }
                }
            }
        }
        else if(strcmp(token, opt5)==0){
            return;
        }
        else{
            buf_tcp[0]='\0';
            strcpy(buf_tcp, "FALHA NO SERVIDOR! OPÇÃO RECEBIDA INVÁLIDA\n");
            if((write(client, buf_tcp, 1+strlen(buf_tcp)))<0){
                close(client);
                erro("PROBLEMA AO ESCREVER PARA O CLIENTE TCP");
            }
        }
    }
}

int generateIP(){
    return rand() % 256;
}

/**---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char* argv[]){
    if(argc!=4){
        erro(">> DEVE UTILIZADOR O COMANDO: news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {FICHEIRO DE CONFIGURAÇÃO}");
    }
    else {
        /***************************************ABERTURA E LEITURA DO FICHEIRO DE CONFIG****************************************/
        // CRIAÇÃO DA MEMORIA PARTILHADA;
        int size=sizeof(struct TOPIC)*MAX_TOPICS+sizeof(int)*2+sizeof(struct USER)*MAX_USERS;
        shmID=shmget(IPC_PRIVATE, size, IPC_CREAT|0666);
        topicsCount=(int *)shmat(shmID, NULL,0);
        usersCount=(int *)(topicsCount+1);
        usersList=(USER *)(usersCount+1);
        topicsList=(TOPIC *)(usersList+MAX_USERS);
        *topicsCount=0;

        *usersCount=0;

        FILE *configFile;
        countFileLines(argv[3]);
        //ABERTURA DO FICHEIRO NO MODO DE LEITURA E APPEND PARA PODERMOS ESCREVER NO FIM DELE
        configFile = fopen(argv[3], "a+");
        if (configFile == NULL) {
            erro("FALHA NA ABERTURA DO FICHEIRO DE CONFIGURAÇÃO");
        }
        char line[BUF_INFOS];
        char *token;
        //LEITURA DO FICHEIRO LINHA A LINHA PARA OBTER OS NOMES, PASSWORDS E PERMISSÕES DOS USERS
        int number=*usersCount;
        for (int i = 0; i<number; i++) {
            fgets(line, sizeof(line), configFile);
            line[strlen(line) - 1] = '\0';
            token = strtok(line, ";");
            if(token!=NULL) {
                strcpy(usersList[i].nome, token);
                token = strtok(NULL, ";");
            }
            if(token!=NULL) {
                strcpy(usersList[i].password, token);
                token = strtok(NULL, ";");
            }
            if(token!=NULL){
                strcpy(usersList[i].permission, token);
                token = strtok(NULL, ";");
            }
            usersList[i].posicao = i;
        }
        fclose(configFile);

/*********************************** SOCKET TCP ************************************/
        //INICIALIZAÇÃO DE VARIÁVEIS
        struct sockaddr_in serverTCP, client;
        socklen_t clientLen = sizeof(client);
        char buf[BUF_SIZE];
        int listenTCP, bind_tcp, connectTCP, msg_recebida, connect;
        pid_t udpPid;

        srand(time(NULL));

/**-------------------------------------------------------------------------------------------------*/
        bzero(&serverTCP, sizeof(serverTCP));

        //PREENCHIMENTO ESTRUTURA DO SOCKET TCP
        serverTCP.sin_family = AF_INET;
        serverTCP.sin_port = htons(atoi(argv[1]));
        serverTCP.sin_addr.s_addr = htonl(INADDR_ANY);

        // CRIAÇÃO DO SOCKET PARA TCP
        listenTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenTCP < 0) {
            erro("PROBLEMA NA CRIAÇÃO DO SOCKET TCP DO SERVIDOR");
        }

        //ASSOCIAÇÃO DO SOCKET À INFO DE ENDEREÇAMENTO TCP
        if((bind_tcp = bind(listenTCP, (struct sockaddr *) &serverTCP, sizeof(serverTCP)))<0){
            erro("PROBLEMA NO BIND DE TCP");
        }

        //LISTEN
        if(listen(listenTCP, 10)<0){
            erro("PROBLEMA NO LISTEN DO TCP");
        }
/**-------------------------------------------------------------------------------------------------*/
//CRIAÇÃO DO SOCKET PARA UDP NOUTRO PROCESSO PARA QUE POSSAM CORRER AO MESMO TEMPO
        udpPid=fork();
        if(udpPid==0){
            accessAdminConsole(atoi(argv[2]), argv[3]);
            exit(0);
        }

/**-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
        while(1) {
            while (waitpid(-1, NULL, WNOHANG) > 0);
            connectTCP = accept(listenTCP, (struct sockaddr *) &client, (socklen_t *) &clientLen);
            if (connectTCP > 0) {
                if(fork()==0){
                    //close(listenTCP);
                    userAccess(connectTCP);
                }
                close(connectTCP);
            }
        }

    }
    return 0;
}
