#include "sockets.h"
#include "select.h"
#include "functionalities.h"

#define REGIP "193.136.138.142"
#define REGUDP "59000"

// Set to 1 to see the messages received, 0 not to
#define VERBOSE 1

int main(int argc, char *argv[]) {
    char IP[16], TCP[6], buffer[BUFFER_SIZE];
    char regIP[16] = REGIP, regUDP[6] = REGUDP;
    
    // Validate the arguments
    if (validateArguments(argc, argv, IP, TCP, regIP, regUDP)) return 0;

    // Create a new select structure
    Select *s = newSelect();
    // Add stdin to the list of descriptors
    addFD(s, 0);

    // Initialize a node, in this case, our node
    Nodes *n = (Nodes*)calloc(1, sizeof(Nodes)); 
    strcpy(n->selfIP, IP); strcpy(n->selfTCP, TCP);

    // Create a TCP server to listen to other nodes and communication with the UDP server
    Socket *listenTCP = TCPserverSocket(TCP, 15);
    if (listenTCP == NULL) return 1;
    Socket *regSERV = UDPSocket(regIP, regUDP);
    
    Socket *new = NULL;
    Chord *c_aux = NULL, *c_aux2 = NULL;

    n->selfSOCK = listenTCP;
    // Add the listening port
    addFD(s, getFD_Socket(listenTCP));  

    /**
     * @brief In this loop, we listen to the descriptors, for this example, only stdin and the TCP listening port.
     * When any of them indicates activity, the listenSelect function unblocks and we handle that respective fd.
     */
    while (loop) {
        if (!listenSelect(s, -1)) printf("Timed out!\n");
        else {
            // Keyboard Input
            if (checkFD(s, 0)) 
                if (consoleInput(regSERV, n, s)) break;
            // Handle a new connection
            if (checkFD(s, getFD_Socket(listenTCP))) {
                new = TCPserverAccept(listenTCP);
                if (new != NULL) {
                    Recieve(new, buffer);
                    // Get message content
                    if (VERBOSE) printf("[TCP listen]: %s\n", buffer);
                    handleNewConnection(n, s, &n->c, new, buffer);
                }
            }
            // Successor sent something
            if ((n->succSOCK != NULL) && checkFD(s, getFD_Socket(n->succSOCK))) {
                if (Recieve(n->succSOCK, buffer) == 0) {
                    // Successor disconnected
                    printf("Succ disconnected!\n");
                    handleSuccDisconnect(n, s);
                } else {
                    // Handle remaining commands from successor
                    if (VERBOSE) printf("[succ]: %s\n", buffer);
                    handleSuccCommands(n, s, buffer);
                }
            }
            // Predecessor sent something
            if ((n->predSOCK != NULL) && checkFD(s, getFD_Socket(n->predSOCK))) {
                printf("Pred message!\n");
                if (Recieve(n->predSOCK, buffer) == 0) {
                    // Predecessor disconnected
                    printf("Pred diconnected!\n");
                    handlePredDisconnect(n, s);
                } else {
                    // Handle remaining commands from predecessor
                    if (VERBOSE) printf("[pred]: %s\n", buffer);
                    handlePredCommands(n, s, buffer);
                }
            }
            // Our Chord sent something
            if ((n->chordSOCK != NULL) && checkFD(s, getFD_Socket(n->chordSOCK))) {
                if (Recieve(n->chordSOCK, buffer) == 0) {
                    // Chord (ours) disconnected
                    handleOurChordDisconnect(n, s);
                } else {
                    // Handle remaining commands from chord (ours)
                    if (VERBOSE) printf("[c-%s]: %s\n", n->chordID, buffer);
                    handlePredCommands(n, s, buffer);
                }
            }
            c_aux = n->c;
            // Iterate over all remaining chords
            while (c_aux != NULL) {
                // Some other chord sent something
                if (checkFD(s, getFD_Socket(c_aux->s))) {
                    c_aux2 = c_aux;
                    c_aux = c_aux->next;
                    if (Recieve(c_aux2->s, buffer) == 0) {
                        // Chord (others) disconnected
                        handleChordsDisconnect(n, s, c_aux2);
                    } else {
                        // Handle remaining commands from chord (others)
                        if (VERBOSE) printf("[c-%s]: %s\n", n->c->ID, buffer);
                        handlePredCommands(n, s, buffer);
                    }
                } else c_aux = c_aux->next;
            }
        }
        
    }

    // Removal of listening fds and closing of sockets.
    if (n->predSOCK != NULL) closeSocket(n->predSOCK, 1);
    if (n->succSOCK != NULL) closeSocket(n->succSOCK, 1);
    removeFD(s, 0);
    removeFD(s, getFD_Socket(listenTCP));
    closeSocket(regSERV, 1); closeSocket(listenTCP, 1);
    freeSelect(s);
    free(n);

    return 0;
}
