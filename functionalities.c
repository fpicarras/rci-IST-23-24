#include "sockets.h"
#include "select.h"

#define TIME_OUT 15

typedef struct _nodes{
    char selfID[3];
    char selfIP[16];
    char selfTCP[6];
    Socket *selfSOCK;

    char succID[3];
    char succIP[16];
    char succTCP[6];
    Socket *succSOCK;

    char ssuccID[3];
    char ssuccIP[16];
    char ssuccTCP[6];
    Socket *ssuccSOCK;

    char predID[3];
    Socket *predSOCK;
}Nodes;

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

int registerInServer(Socket *server, char *ring, Nodes *n){
    char buffer[128];

    sprintf(buffer, "REG %s %s %s %s", ring, n->selfID, n->selfIP, n->selfTCP);
    printf("Command:\n%s\n", buffer);
    Send(server, buffer);
    Recieve(server, buffer);

    if(strcmp(buffer, "OKREG")==0){
        printf("Registred %s in ring %s!\n", n->selfID, ring);
        return 1;
    }else{
        printf("%s\n", buffer);
        return 0;
    }
}

int unregisterInServer(Socket *server, char *ring, Nodes *n){
    char buffer[64];

    sprintf(buffer, "UNREG %s %s", ring, n->selfID);
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

int join(Nodes *n, char *succID, char *succIP, char *succTCP){
    char *buffer = (char*)malloc(BUFFER_SIZE*sizeof(char));
    printf("Attempting to join to: %s %s:%s\n", succID, succIP, succTCP);
    //New select structure to listen to incoming data from sockets
    Select *s = newSelect();
    //Attempt to connect to the succ node
    Socket *succ = TCPSocket(succIP, succTCP);
    if(succ == NULL){
        printf("Unable to connec to to succesor %s\n", succID);
        return 0;
    }
    //If connection as succeceful fill n with the succ info
    strcpy(n->succID, succID); strcpy(n->succIP, succIP); strcpy(n->succTCP, succTCP);
    n->succSOCK = succ;
    //Sending the ENTRY command
    sprintf(buffer, "ENTRY %s %s %s\n", n->selfID, n->selfIP, n->selfTCP);
    Send(succ, buffer);
    
    //Waiting for response!
    addFD(s, getFD_Socket(succ));
    if(listenSelect(s, TIME_OUT) > 0){
        //If the succ sent a response
        if(checkFD(s, getFD_Socket(succ))){
            Recieve(succ, buffer);
            sscanf(buffer, "SUCC %s %s %s\n", n->ssuccID, n->ssuccIP, n->ssuccTCP);
            //Fill the ssucc info in n             
        }
    }else{
        //In case of time-out
        printf("Timed-out waiting for succ %s after %d seconds...\n", succID, TIME_OUT);
        closeSocket(succ);
        free(buffer);
        freeSelect(s);
        return 0;
    }

    //Now we will wait for the pred to connect announcing itself
        //We will listen to out TCP port
    removeFD(s, getFD_Socket(succ));
    addFD(s, getFD_Socket(n->selfSOCK));
    if(listenSelect(s, TIME_OUT) > 0){
        //If the pred connected we will wait for its response
        if(checkFD(s, getFD_Socket(n->selfSOCK))){
            n->predSOCK = TCPserverAccept(n->selfSOCK);
            Recieve(n->predSOCK, buffer);
            sscanf(buffer, "PRED %s\n", n->predID);
        }
    }else{
        //In case of time-out
        printf("Timed-out waiting for pred after %d seconds...\n", TIME_OUT);
        closeSocket(succ);
        free(buffer);
        freeSelect(s);
        return 0;
    }
    printf("Joined!\nSUCC: %s [%s:%s]\nSSUCC: %s [%s:%s]\nPRED: %s\n\n", n->succID, n->succIP, n->succTCP, n->ssuccID, n->ssuccIP, n->ssuccTCP, n->predID);
    free(buffer);
    freeSelect(s);
    return 1;
}