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

char *getNodesServer(Socket *server, char *ring){
    char *buffer =(char*)malloc(sizeof(char)*BUFFER_SIZE);

    sprintf(buffer, "NODES %s", ring);
    Send(server, buffer);
    Recieve(server, buffer);

    //printf("%s\n", buffer);

    return buffer;
}

void isNodeInServer(char *nodeslist, char *selfID){
    //Skip NODESLIST r\n
    char *aux;
    char succID[4], succIP[16], succTCP[8];

    char id[4]; strcpy(id, selfID);

    int available, i=0;
    while(1){
        aux = nodeslist+14;
        available = 1;
        while(sscanf(aux, "%s %s %s", succID, succIP, succTCP)==3){
            if(strcmp(id, succID)==0){
                available = 0;
                i = (id[0]-48)*10 + (id[1]-48);
                i++;
                id[0] = (i%100 - i%10)/10 + 48;
                id[1] = (i%10) + 48;
                break;
            }
            aux += 3+strlen(succID)+strlen(succIP)+strlen(succTCP);
        }
        if(available) break;
    }
    printf("Id choosen: %s\n", id);
    strcpy(selfID, id);
    return;
}

int directJoin(Nodes *n, Select *sel, char *succID, char *succIP, char *succTCP){
    char *buffer = (char*)malloc(BUFFER_SIZE*sizeof(char));
    //printf("Attempting to join to: %s %s:%s\n", succID, succIP, succTCP);
    //New select structure to listen to incoming data from sockets
    Select *s = newSelect();
    //Attempt to connect to the succ node
    Socket *succ = TCPSocket(succIP, succTCP);
    if(succ == NULL){
        printf("Unable to connec to succesor %s\n", succID);
        return 0;
    }
    //If connection as succeceful fill n with the succ info -> s(self) = succ
    addFD(sel, getFD_Socket(succ));
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
        closeSocket(succ, 1);
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
            // p(self) = pred
            n->predSOCK = TCPserverAccept(n->selfSOCK);
            Recieve(n->predSOCK, buffer);
            sscanf(buffer, "PRED %s\n", n->predID);
            addFD(sel, getFD_Socket(n->predSOCK));
        }
    }else{
        //In case of time-out
        printf("Timed-out waiting for pred after %d seconds...\n", TIME_OUT);
        closeSocket(succ, 1);
        free(buffer);
        freeSelect(s);
        return 0;
    }
    printf("Joined!\nSUCC: %s [%s:%s]\nSSUCC: %s [%s:%s]\nPRED: %s\n\n", n->succID, n->succIP, n->succTCP, n->ssuccID, n->ssuccIP, n->ssuccTCP, n->predID);
    free(buffer);
    freeSelect(s);
    return 1;
}

int join(Socket *regSERV, Nodes *n, Select *sel, char *ring){
    char succID[3], succIP[16], succTCP[8];
    //Gets the list of connected Nodes
    char *aux = getNodesServer(regSERV, ring);
    //Ensures that out id is unique, if it isn't we try to get a new one
    isNodeInServer(aux, n->selfID);
    //Skip NODESLIST r\n
    if(sscanf(aux+14, "%s %s %s", succID, succIP, succTCP)!=3){
        //The ring is empty
        strcpy(n->succID, n->selfID); strcpy(n->succIP, n->selfIP); strcpy(n->succTCP, n->selfTCP);
        strcpy(n->ssuccID, n->selfID); strcpy(n->ssuccIP, n->selfIP); strcpy(n->ssuccTCP, n->selfTCP);
        strcpy(n->predID, n->selfID);
        free(aux);
        return 1;
    }else{
        free(aux);
        return directJoin(n, sel, succID, succIP, succTCP);
    }

}

void handleENTRY(Nodes *n, Socket *new_node, Select *s, char *msg){
    char buffer[64], newID[4], newIP[16], newTCP[8];

    sscanf(msg, "ENTRY %s %s %s\n", newID, newIP, newTCP);
    printf("ENTRY: %s %s %s\n", newID, newIP, newTCP);

    if(strcmp(newID, n->selfID)==0){
        printf("New connection trying the same id... Disconnecting from them...\n");
        closeSocket(new_node, 1);
        return;
    }

    //Same IDs means we are alone in the ring
    if(strcmp(n->succID, n->selfID)==0){
        //p(self) = new
        strcpy(n->predID, newID); n->predSOCK = new_node;
        addFD(s, getFD_Socket(new_node));

        sprintf(buffer, "SUCC %s %s %s\n", newID, newIP, newTCP);
        Send(new_node, buffer);

        //s(self) = new
        n->succSOCK = TCPSocket(newIP, newTCP);
        sprintf(buffer, "PRED %s\n", n->selfID);
        Send(n->succSOCK, buffer);
        strcpy(n->succID, newID); strcpy(n->succIP, newIP); strcpy(n->succTCP, newTCP);
        addFD(s, getFD_Socket(n->succSOCK));
    }else{
        sprintf(buffer, "ENTRY %s %s %s\n", newID, newIP, newTCP);
        Send(n->predSOCK, buffer);
        sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
        Send(new_node, buffer);
        //p(self) = new
        //Removes the fd of pred from current select
        removeFD(s, getFD_Socket(n->predSOCK));
        closeSocket(n->predSOCK, 0);
        strcpy(n->predID, newID); n->predSOCK = new_node;
        //Adds the new fd to the select
        addFD(s, getFD_Socket(n->predSOCK));
    }
}