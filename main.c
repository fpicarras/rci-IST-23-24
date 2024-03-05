#include "sockets.h"
#include "select.h"
#include "functionalities.h"


#define REGIP "193.136.138.142"
#define REGUDP "59000"

int main(int argc, char *argv[]){
    char IP[16], TCP[6], buffer[BUFFER_SIZE];
    char regIP[16] = REGIP, regUDP[6] = REGUDP;
    char protocol[8], message[128], dest[4], origin[4];
    
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

    Encaminhamento *e = initEncaminhamento();

    /**
     * @brief Neste loop estaremos a ouvir os descritores, para este exemplo, apenas stdin e o porto de esctuta TCP.
     * Quando algum deles se acusar, a funç~ao listenSelect desbloqueia e tratamos desse respetivo fd.
     */
    while(1){
        if(!listenSelect(s, -1)) printf("Timed out!\n");
        else{
            //Keyboard Input
            if(checkFD(s, 0)) 
                if(consoleInput(regSERV, n, s, e)) break;
            //Handle a new connection
            if(checkFD(s, getFD_Socket(listenTCP))){
                new = TCPserverAccept(listenTCP);
                if(new != NULL){
                    Recieve(new, buffer);
                    //Get message content
                    printf("[TCP listen]: %s\n", buffer);
                    handleNewConnection(n, s, new, buffer);
                }
            }
            //Succesor sent something
            if((n->succSOCK != NULL) && checkFD(s, getFD_Socket(n->succSOCK))){
                if(Recieve(n->succSOCK, buffer)==0){
                    //Succ disconnected
                    handleSuccDisconnect(n, s, e);
                }else {
                    //Handle remaining commands from succ
                    printf("[succ]: %s\n", buffer);
                    //printf ("%d", n);
                    handleSuccCommands(n, s, buffer);
                }
            }
            //Predecesor sent something
            if((n->predSOCK != NULL) && checkFD(s, getFD_Socket(n->predSOCK))){
                if(Recieve(n->predSOCK, buffer)==0){
                    //Pred disconnected
                    handlePredDisconnect(n, s, e);
                }else {
                    //Handle remaining commands from pred
                    printf("[pred]: %s\n", buffer);
                    if (sscanf(buffer, "%s", protocol) == 1){
                        if (strcmp(protocol, "CHAT") == 0){
                            if (sscanf(buffer + 5, "%s %s %[^\n]\n", origin, dest, message) == 3){
                                if (strcmp (dest, n->selfID) == 0){
                                    printf ("%s \n\n", message);
                                    continue;
                                }
                                sprintf(buffer, "CHAT %s %s %s\n", n->succID, dest, message);
                                Send(n->succSOCK, buffer);  /* Alterar para seguir para o nó da tabela de expedição no indice do destino !!! */
                            }
                        }
                    }
                    //handlePredCommands(n, s, buffer);
                }
            }
        }
        
    }

    //Remoç~ao dos fds de escuta e fecho das sockets.
    removeFD(s, 0);
    removeFD(s, getFD_Socket(listenTCP));
    closeSocket(regSERV, 1); closeSocket(listenTCP, 1);
    deleteEncaminhamento(e);
    freeSelect(s);
    free(n);

    return 0;
}