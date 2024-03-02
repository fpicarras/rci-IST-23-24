#include "sockets.h"
#include "select.h"

#include "functionalities.c"

#define REGIP "193.136.138.142"
#define REGUDP "59000"

int consoleInput(Socket *regSERV, Nodes *n, Select *s){
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
                if(join(regSERV, n, s, ring)){
                    if(registerInServer(regSERV, ring, n)) connected = 1;
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
                if(directJoin(n, s, arg2, arg3, arg4)) connected = 1;
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
            if(connected){
                printf("NODE TOPOLOGY:\n\nRing: %s\n", ring);
                printf("Pred: %s\n", n->predID);
                printf("Self: %s [%s:%s]\n", n->selfID, n->selfIP, n->selfTCP);
                printf("Succ: %s [%s:%s]\n", n->succID, n->succIP, n->succTCP);
                printf("Ssucc: %s [%s:%s]\n\n", n->ssuccID, n->ssuccIP, n->ssuccTCP); 
            }else printf("Not connected...\n\n");         
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
                    strcpy(ring, "---");
                }
                if(n->predSOCK != NULL && n->succSOCK != NULL){
                    removeFD(s, getFD_Socket(n->predSOCK)); removeFD(s, getFD_Socket(n->succSOCK));
                    closeSocket(n->predSOCK, 1); closeSocket(n->succSOCK, 1);
                    n->predSOCK = NULL; n->succSOCK = NULL;
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
                    strcpy(ring, "---");
                }
                if(n->predSOCK != NULL && n->succSOCK != NULL){
                    closeSocket(n->predSOCK, 1); closeSocket(n->succSOCK, 1);
                }
                connected = 0;
            }
            return 1;             
        }
        else if(strcmp(command, "clear") == 0){
            if(sscanf(str+6, "%s", arg1)){
                char *aux = getNodesServer(regSERV, arg1), *aux1;
                char ID[4], IP[16], TCP[8], buffer[32];
                aux1 = aux + 14;
                while(sscanf(aux1, "%s %s %s", ID, IP, TCP)==3){
                    sprintf(buffer, "UNREG %s %s", arg1, ID);
                    Send(regSERV, buffer); Recieve(regSERV, buffer);
                    printf("%s\n", buffer);
                    aux1 += 3+strlen(ID)+strlen(IP)+strlen(TCP);
                }
                free(aux);
            }
        }
        else if(strcmp(command, "nodes") == 0){
            if(sscanf(str+6, "%s", arg1)){
                char *aux = getNodesServer(regSERV, arg1);
                printf("%s\n", aux);
                free(aux);
            }
        }
        else {
            printf("Invalid interface command!\n");
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    char IP[16], TCP[6], buffer[BUFFER_SIZE], command[16];
    char regIP[16] = REGIP, regUDP[6] = REGUDP;
    char auxID[4], auxIP[16], auxTCP[8];
    
    //Validamos os argumentos
    if(validateArguments(argc, argv, IP, TCP, regIP, regUDP)) return 0;

    //Cria-se uma nova estrutura de select
    Select *s = newSelect();
    //Adicionar o stdin `a lista de descritores
    addFD(s, 0);

    //Inicia-se um no, neste caso o nosso no
    Nodes *n = (Nodes*)calloc(1, sizeof(Nodes)); 
    strcpy(n->selfIP, IP); strcpy(n->selfTCP, TCP);

    //Ciramos um servidor TCP para ouvir outros n´os e um a comunicaç~ao com o server UDP
    Socket *listenTCP = TCPserverSocket(TCP, 15);
    if(listenTCP == NULL) return 1;
    Socket *regSERV = UDPSocket(regIP, regUDP);
    
    Socket *new = NULL;

    n->selfSOCK = listenTCP;
    //Adicionar o porto de escuta
    addFD(s, getFD_Socket(listenTCP));  

    /**
     * @brief Neste loop estaremos a ouvir os descritores, para este exemplo, apenas stdin e o porto de esctuta TCP.
     * Quando algum deles se acusar, a funç~ao listenSelect desbloqueia e tratamos desse respetivo fd.
     */
    while(1){
        if(!listenSelect(s, -1)) printf("Timed out!\n");
        else{
            //Keyboard Input
            if(checkFD(s, 0)) 
                if(consoleInput(regSERV, n, s)) break;
            //Handle a new connection
            if(checkFD(s, getFD_Socket(listenTCP))){
                new = TCPserverAccept(listenTCP);
                if(new == NULL) continue;
                Recieve(new, buffer);
                //Get message content
                printf("From [new connection]: %s\n", buffer);
                if(sscanf(buffer, "%s", command)){
                    if(strcmp(command, "ENTRY")==0) handleENTRY(n, new, s, buffer);
                    if(strcmp(command, "PRED")==0){
                        sscanf(buffer, "PRED %s\n", n->predID);
                        n->predSOCK = new; addFD(s, getFD_Socket(new));
                        sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
                        Send(new, buffer);
                    }
                }
            }
            //Succesor sent something
            if((n->succSOCK != NULL) && checkFD(s, getFD_Socket(n->succSOCK))){
                if(Recieve(n->succSOCK, buffer)==0){
                    //Succ disconnected
                    removeFD(s, getFD_Socket(n->succSOCK)); closeSocket(n->succSOCK, 1);
                    if(strcmp(n->selfID, n->ssuccID)!=0){
                        new = TCPSocket(n->ssuccIP, n->ssuccTCP);
                        strcpy(n->succID, n->ssuccID); strcpy(n->succIP, n->ssuccIP); strcpy(n->succTCP, n->ssuccTCP);
                        n->succSOCK = new; addFD(s, getFD_Socket(new));
                        sprintf(buffer, "PRED %s\n", n->selfID);
                        Send(new, buffer);

                        sprintf(buffer, "SUCC %s %s %s", n->succID, n->succIP, n->succTCP);
                        Send(n->predSOCK, buffer);
                    }else {//In case we are the only node left, we dont connect to ourself...
                        strcpy(n->succID, n->ssuccID); strcpy(n->succIP, n->ssuccIP); strcpy(n->succTCP, n->ssuccTCP);
                        strcpy(n->predID, n->ssuccID);
                    }
                    continue;
                }
                printf("From [succ]: %s\n", buffer);
                if(sscanf(buffer, "%s", command)){
                    if(strcmp(command, "ENTRY")==0){
                        sscanf(buffer, "ENTRY %s %s %s\n", auxID, auxIP, auxTCP);

                        //Notifies it's pred of his new ssucc
                        sprintf(buffer, "SUCC %s %s %s\n", auxID, auxIP, auxTCP);
                        Send(n->predSOCK, buffer);

                        //demote succ to ssucc and closes connection
                        strcpy(n->ssuccID, n->succID); strcpy(n->ssuccIP, n->succIP); strcpy(n->ssuccTCP, n->succTCP);
                        removeFD(s, getFD_Socket(n->succSOCK));
                        closeSocket(n->succSOCK, 1);

                        //Set new node to succ
                        strcpy(n->succID, auxID); strcpy(n->succIP, auxIP); strcpy(n->succTCP, auxTCP);
                        sprintf(buffer, "PRED %s\n", n->selfID);
                        new = TCPSocket(auxIP, auxTCP);
                        addFD(s, getFD_Socket(new)); n->succSOCK = new;
                        Send(new, buffer);
                    }else if(strcmp(command, "SUCC")==0){
                        sscanf(buffer, "SUCC %s %s %s\n", n->ssuccID, n->ssuccIP, n->ssuccTCP);
                    }
                }
            }
            //Predecesor sent something
            if((n->predSOCK != NULL) && checkFD(s, getFD_Socket(n->predSOCK))){
                if(Recieve(n->predSOCK, buffer)==0){
                    removeFD(s, getFD_Socket(n->predSOCK)); closeSocket(n->predSOCK, 1);
                }
            }
        }
        
    }

    //Remoç~ao dos fds de escuta e fecho das sockets.
    removeFD(s, 0);
    removeFD(s, getFD_Socket(listenTCP));
    closeSocket(regSERV, 1); closeSocket(listenTCP, 1);
    freeSelect(s);
    free(n);

    return 0;
}