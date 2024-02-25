#include "sockets.h"
#include "select.h"

#define REGIP "193.136.138.142"
#define REGUDP "59000"

typedef struct _node{
    char id[3];
    char IP[16];
    char port[6];
}Node;

int validateArguments(int argc, char **argv, char *IP, char *TCP, char *regIP, char *regUDP){
    if(argc != 3 && argc != 5){
        printf("Invalid Arguments!\nUsage: COR IP TCP [regIP] [regUDP]\n");
        return 1;
    }else{
        strcpy(IP, argv[1]); strcpy(TCP, argv[2]);
        if(argc == 5){
            strcpy(regIP, argv[3]); strcpy(regUDP, argv[4]);
        }
        return 0;
    }
}

int registerInServer(Socket *server, char *ring, Node *n){
    char buffer[128];

    sprintf(buffer, "REG %s %s %s %s", ring, n->id, n->IP, n->port);
    printf("Command: %s\n", buffer);
    Send(server, buffer);
    Recieve(server, buffer);

    if(strcmp(buffer, "OKREG")==0){
        printf("Registred %s in ring %s!\n", n->id, ring);
        return 1;
    }else{
        printf("%s\n", buffer);
        return 0;
    }
}

int unregisterInServer(Socket *server, char *ring, Node *n){
    char buffer[64];

    sprintf(buffer, "UNREG %s %s", ring, n->id);
    Send(server, buffer);
    Recieve(server, buffer);

    if(strcmp(buffer, "OKUNREG")==0){
        printf("Leaving ring %s...\n", ring);
        return 1;
    }else {
        printf("%s\n", buffer);
        return 0;
    }
}

void getNodesServer(Socket *server, char *ring){
    char buffer[1024];

    sprintf(buffer, "NODES %s", ring);
    Send(server, buffer);
    Recieve(server, buffer);

    printf("%s\n", buffer);
}

int consoleInput(Socket *regSERV, Node *self){
    char str[100], command[8], arg1[5], arg2[8], arg3[8], arg4[8], message[100];
    int offset = 0;
    static int connected = 0;
    static char ring[4];

    fgets(str, 100, stdin);

    if (sscanf(str + offset, "%s", command) == 1) {
        offset += strlen(command) + 1; /* +1 para o espaço em branco */
        // JOIN [ring] [id]
        if (strcmp(command, "j") == 0) {
            if(connected){
                printf("Already connected! Leave current connection to enter a new one...\n");
                return 0;
            }
            if (sscanf(str + offset, "%s %s", arg1, arg2) == 2){
                strcpy(self->id, arg2); strcpy(ring, arg1);
                if(registerInServer(regSERV, arg1, self)){
                    getNodesServer(regSERV, arg1);
                    connected = 1;
                } 
            } else printf("Invalid interface command!\n");
        }
        // DIRECT JOIN [id] [succid] [succIP] [succTCP]
        else if (strcmp(command, "dj") == 0) {
            if (sscanf(str + offset, "%s %s %s %s", arg1, arg2, arg3, arg4) == 4){
                /****/
            } else printf("Invalid interface command!\n");
        }
        // CHORD
        else if (strcmp(command, "c") == 0) {          
            /****/             
        }
        // REMOVE CHORD
        else if (strcmp(command, "rc") == 0) {          
            /****/             
        }
        // SHOW TOPOLOGY
        else if (strcmp(command, "st") == 0) {          
            /****/             
        }
        // SHOW ROUTING [dest]
        else if (strcmp(command, "sr") == 0) {
            if (sscanf(str + offset, "%s", arg1) == 1){
                /****/
            } else printf("Invalid interface command!\n");
        }
        // SHOW PATH [dest]
        else if (strcmp(command, "sp") == 0) {
            if (sscanf(str + offset, "%s", arg1) == 1){
                /****/
            } else printf("Invalid interface command!\n");
        }
        // SHOW FORWARDING
        else if (strcmp(command, "sf") == 0) {          
            /****/             
        }
        // MESSAGE [dest] [message]
        else if (strcmp(command, "m") == 0) {
            offset += strlen(arg1) + 1;
            if (sscanf(str + offset, "%s", arg1) == 1){
                if (sscanf(str + offset + 5, "%[^\n]", message) == 1){
                printf ("%s", message);
                    /****/
                }
            } else printf("Invalid interface command!\n");
        }
        // LEAVE
        else if (strcmp(command, "l") == 0) {          
            if(connected && unregisterInServer(regSERV, ring, self)){
                getNodesServer(regSERV, ring);
                connected = 0; 
            }         
        }
        // EXIT
        else if (strcmp(command, "x") == 0) {          
            printf("Application Closing!\n");
            if(connected && unregisterInServer(regSERV, ring, self) ){
                getNodesServer(regSERV, ring);
                connected = 0; 
            } 
            return 1;             
        }
        else {
            printf("Invalid interface command!\n");
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    char IP[16], TCP[6];
    char regIP[16] = REGIP, regUDP[6] = REGUDP;
    
    //Validamos os argumentos
    if(validateArguments(argc, argv, IP, TCP, regIP, regUDP)) return 0;

    //Cria-se uma nova estrutura de select
    Select *s = newSelect();
    //Adicionar o stdin `a lista de descritores
    addFD(s, 0);

    //Inicia-se um no, neste caso o nosso no
    Node n; strcpy(n.IP, IP); strcpy(n.port, TCP);

    //Ciramos um servidor TCP para ouvir outros n´os e um a comunicaç~ao com o server UDP
    Socket *listenTCP = TCPserverSocket(TCP, 15);
    Socket *regSERV = UDPSocket(regIP, regUDP);

    //Adicionar o porto de escuta
    addFD(s, getFD_Socket(listenTCP));  

    /**
     * @brief Neste loop estaremos a ouvir os descritores, para este exemplo, apenas stdin e o porto de esctuta TCP.
     * Quando algum deles se acusar, a funç~ao listenSelect desbloqueia e tratamos desse respetivo fd.
     */
    while(1){
        //Metemos timeout de 30 seg por exemplo
        if(!listenSelect(s, 30)) printf("Timed out!\n");
        else{
            if(checkFD(s, 0)) 
                if(consoleInput(regSERV, &n)) break;
            //Para exemplo, temos um server TCP que quando tem clientes aceita-os e envia uma mensagem, fechando a conex~ao.
            if(checkFD(s, getFD_Socket(listenTCP))){
                TCPserverConn *new = TCPserverAccept(listenTCP);
                TCPserverSend(new, "Away RAT!\n");
                TCPserverCloseConnection(new);
            }
        }
        
    }

    //Remoç~ao dos fds de escuta e fecho das sockets.
    removeFD(s, 0);
    removeFD(s, getFD_Socket(listenTCP));
    closeSocket(regSERV); closeSocket(listenTCP);
    freeSelect(s);

    return 0;
}