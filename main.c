#include "sockets.h"
#include "select.h"

#include "functionalities.c"

#define REGIP "193.136.138.142"
#define REGUDP "59000"

int consoleInput(Socket *regSERV, Nodes *n){
    char str[100], command[8], arg1[5], arg2[16], arg3[16], arg4[16], message[128];
    int offset = 0;
    static int connected = 0;
    static char ring[] = "---";

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
                strcpy(n->selfID, arg2); strcpy(ring, arg1);
                if(registerInServer(regSERV, arg1, n)){
                    getNodesServer(regSERV, arg1);
                    connected = 1;
                } 
            } else printf("Invalid interface command!\n");
        }
        // DIRECT JOIN [id] [succid] [succIP] [succTCP]
        else if (strcmp(command, "dj") == 0) {
            if(connected){
                printf("Already connected! Leave current connection to enter a new one...\n");
                return 0;
            }
            if (sscanf(str + offset, "%s %s %s %s", arg1, arg2, arg3, arg4) == 4){
                strcpy(n->selfID, arg1);
                if(join(n, arg2, arg3, arg4)) connected = 1;
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
            if(connected){
                /* DISCONNECT FROM NODES */
                if(!(strcmp(ring, "---")==0) && unregisterInServer(regSERV, ring, n)){
                    getNodesServer(regSERV, ring);
                    strcpy(ring, "---");
                }
                connected = 0;
            }             
        }
        // EXIT
        else if (strcmp(command, "x") == 0) {          
            printf("Application Closing!\n");
            if(connected){
                /* DISCONNECT FROM NODES */
                if(!(strcmp(ring, "---")==0) && unregisterInServer(regSERV, ring, n)){
                    getNodesServer(regSERV, ring);
                    strcpy(ring, "---");
                }
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
    Nodes n; strcpy(n.selfIP, IP); strcpy(n.selfTCP, TCP);

    //Ciramos um servidor TCP para ouvir outros n´os e um a comunicaç~ao com o server UDP
    Socket *listenTCP = TCPserverSocket(TCP, 15);
    Socket *regSERV = UDPSocket(regIP, regUDP);

    n.selfSOCK = listenTCP;
    //Adicionar o porto de escuta
    addFD(s, getFD_Socket(listenTCP));  

    /**
     * @brief Neste loop estaremos a ouvir os descritores, para este exemplo, apenas stdin e o porto de esctuta TCP.
     * Quando algum deles se acusar, a funç~ao listenSelect desbloqueia e tratamos desse respetivo fd.
     */
    while(1){
        //Metemos timeout de 30 seg por exemplo
        if(!listenSelect(s, -1)) printf("Timed out!\n");
        else{
            if(checkFD(s, 0)) 
                if(consoleInput(regSERV, &n)) break;
            //Para exemplo, temos um server TCP que quando tem clientes aceita-os e envia uma mensagem, fechando a conex~ao.
            if(checkFD(s, getFD_Socket(listenTCP))){
                char buffer[1024];
                Socket *new = TCPserverAccept(listenTCP);
                Recieve(new, buffer);
                printf("%s\n", buffer);
                Send(new, "Away RAT!\n");
                closeSocket(new);
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