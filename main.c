#include "sockets.h"
#include "select.h"
#include "functionalities.h"

#define REGIP "193.136.138.142"
#define REGUDP "59000"

//Set to 1 to see the messages recieved, 0 not to
#define VERBOSE 1

int main(int argc, char *argv[]){
    char IP[16], TCP[6], buffer[BUFFER_SIZE];
    char regIP[16] = REGIP, regUDP[6] = REGUDP;
    
    //Validamos os argumentos
    if(validateArguments(argc, argv, IP, TCP, regIP, regUDP)) return 0;

    //signal(SIGINT, sigHandler);

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
    Chord *c_aux = NULL;

    n->selfSOCK = listenTCP;
    //Adicionar o porto de escuta
    addFD(s, getFD_Socket(listenTCP));  

    /**
     * @brief Neste loop estaremos a ouvir os descritores, para este exemplo, apenas stdin e o porto de esctuta TCP.
     * Quando algum deles se acusar, a funç~ao listenSelect desbloqueia e tratamos desse respetivo fd.
     */
    while(loop){
        c_aux = n->c;

        if(!listenSelect(s, -1)) printf("Timed out!\n");
        else{
            //Keyboard Input
            if(checkFD(s, 0)) 
                if(consoleInput(regSERV, n, s)) break;
            //Handle a new connection
            if(checkFD(s, getFD_Socket(listenTCP))){
                new = TCPserverAccept(listenTCP);
                if(new != NULL){
                    Recieve(new, buffer);
                    //Get message content
                    if(VERBOSE) printf("[TCP listen]: %s\n", buffer);
                    handleNewConnection(n, s, &n->c, new, buffer);
                }
            }
            //Succesor sent something
            if((n->succSOCK != NULL) && checkFD(s, getFD_Socket(n->succSOCK))){
                if(Recieve(n->succSOCK, buffer)==0){
                    //Succ disconnected
                    handleSuccDisconnect(n, s);
                }else {
                    //Handle remaining commands from succ
                    if(VERBOSE) printf("[succ]: %s\n", buffer);
                    //printf ("%d", n);
                    handleSuccCommands(n, s, buffer);
                }
            }
            //Predecesor sent something
            if((n->predSOCK != NULL) && checkFD(s, getFD_Socket(n->predSOCK))){
                if(Recieve(n->predSOCK, buffer)==0){
                    //Pred disconnected
                    handlePredDisconnect(n, s);
                }else {
                    //Handle remaining commands from pred
                    if(VERBOSE) printf("[pred]: %s\n", buffer);
                    handlePredCommands(n, s, buffer);
                }
            }
            //Our Chord sent something
            if((n->chordSOCK != NULL) && checkFD(s, getFD_Socket(n->chordSOCK))){
                if(Recieve(n->chordSOCK, buffer)==0){
                    handleOurChordDisconnect(n, s);
                }else {
                    if(VERBOSE) printf("[c-%s]: %s\n", n->chordID, buffer);
                    handlePredCommands (n, s, buffer);
                }
            }
            //Iterate over all ramaining chords
            while(c_aux != NULL){
                //Some other chord sent something
                if(checkFD(s, getFD_Socket(c_aux->s))){
                    if(Recieve(c_aux->s, buffer)==0){
                        handleChordsDisconnect(n, s, c_aux);
                    }else {
                        if(VERBOSE) printf("[c-%s]: %s\n", n->c->ID, buffer);
                        handlePredCommands(n, s, buffer);
                    }
                }
                c_aux = c_aux->next;
            }
        }
        
    }

    //Remoç~ao dos fds de escuta e fecho das sockets.
    if(n->predSOCK != NULL) closeSocket(n->predSOCK, 1);
    if(n->succSOCK != NULL) closeSocket(n->succSOCK, 1);
    removeFD(s, 0);
    removeFD(s, getFD_Socket(listenTCP));
    closeSocket(regSERV, 1); closeSocket(listenTCP, 1);
    freeSelect(s);
    free(n);

    return 0;
}