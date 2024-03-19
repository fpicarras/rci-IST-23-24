#include "functionalities.h"

Encaminhamento *e = NULL;

void sigHandler(int sig){
    printf("\nCTRL+C detected -> Closing program and Sockets\n");
    loop = 0;
}

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
    int attempts = 0;
    char buffer[128];
    Select *udp_t = newSelect();
    addFD(udp_t, getFD_Socket(server));

    sprintf(buffer, "REG %s %s %s %s", ring, n->selfID, n->selfIP, n->selfTCP);
    Send(server, buffer);
    while(attempts < UDP_ATTEMPTS){
        if(listenSelect(udp_t, UDP_TIME_OUT)>0){
            Recieve(server, buffer);
            break;
        }
        else {
            printf("Server unresponsive... Retrying (%d/%d)\n", (attempts++)+1, UDP_ATTEMPTS);
            Send(server, buffer);
        }
    }
    freeSelect(udp_t);

    if(strcmp(buffer, "OKREG")==0){
        printf("Registred as %s in ring %s!\n\n", n->selfID, ring);
        return 1;
    }else{
        printf("%s\n", buffer);
        return 0;
    }
}

int unregisterInServer(Socket *server, char *ring, Nodes *n){
    int attempts = 0;
    char buffer[64];
    Select *udp_t = newSelect();
    addFD(udp_t, getFD_Socket(server));

    sprintf(buffer, "UNREG %s %s", ring, n->selfID);
    Send(server, buffer);
    while(attempts < UDP_ATTEMPTS){
        if(listenSelect(udp_t, UDP_TIME_OUT)>0){
            Recieve(server, buffer);
            break;
        }
        else {
            printf("Server unresponsive... Retrying (%d/%d)\n", (attempts++)+1, UDP_ATTEMPTS);
            Send(server, buffer);
        }
    }
    freeSelect(udp_t);
    if(strcmp(buffer, "OKUNREG")==0){
        printf("Leaving ring %s...\n\n", ring);
        return 1;
    }else {
        printf("%s\n", buffer);
        return 0;
    }
}

char *getNodesServer(Socket *server, char *ring){
    char *buffer =(char*)malloc(sizeof(char)*BUFFER_SIZE);
    int attempts = 0;

    Select *udp_t = newSelect();
    addFD(udp_t, getFD_Socket(server));

    sprintf(buffer, "NODES %s", ring);
    Send(server, buffer);
    while(attempts < UDP_ATTEMPTS){
        if(listenSelect(udp_t, UDP_TIME_OUT)>0){
            Recieve(server, buffer);
            break;
        }
        else {
            printf("Server unresponsive... Retrying (%d/%d)\n", (attempts++)+1, UDP_ATTEMPTS);
            Send(server, buffer);
        }
    }
    freeSelect(udp_t);
    if(attempts == UDP_ATTEMPTS){
        free(buffer);
        return NULL;
    }
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

void sendAllPaths(Socket *s, char *self){
    char buffer[BUFFER_SIZE];

    for(int i = 0; i < 100; i++){
        if(e->shorter_path[i][0] != '\0'){
            sprintf(buffer, "ROUTE %d %d %s\n", atoi(self), i, e->shorter_path[i]);
            Send(s, buffer);
        }
    }
}

void broadcast(Nodes *n, char *msg){
    Chord *aux = n->c;
    if(n->succSOCK != NULL) Send(n->succSOCK, msg);
    if(n->predSOCK != NULL && (strcmp(n->succID, n->predID)!=0)) Send(n->predSOCK, msg);
    if(n->chordSOCK != NULL) Send(n->chordSOCK, msg);
    while (aux != NULL){
        if(aux->s != NULL) Send(aux->s, msg);
        aux = aux->next;
    }
}

Chord *deleteChord(Chord *head, char *ID){
    Chord *aux1 = head, *aux2 = head;

    if(head == NULL) return NULL;
    while(aux1 != NULL){
        if(strcmp(aux1->ID, ID)==0){
            if(aux1 == head){
                aux2 = aux1->next;
                closeSocket(aux1->s, 1);
                free(aux1);
                return aux2;
            } else {
                aux2->next = aux1->next;
                closeSocket(aux1->s, 1);
                free(aux1);
                return head;
            }
        }
        aux2 = aux1;
        aux1 = aux1->next;
    }
    return head;
}

void deleteALLChords(Chord *head, Select *s){
    Chord *aux = head, *aux2 = NULL;

    while(aux != NULL){
        aux2 = aux->next;
        removeFD(s, getFD_Socket(aux->s));
        closeSocket(aux->s, 1);
        free(aux);
        aux = aux2;
    }
}

void createCHORD(Nodes *n, Select *s, Socket *server, char *ring, char *target){
    char ID[4], IP[16], TCP[8], buffer[16];
    char *aux = NULL, *aux1 = NULL;

    if (strcmp (n->predID, n->ssuccID) != 0){ // Há mais de 3 nós
        if (strcmp (n->chordID, "") == 0){
            aux = getNodesServer(server, ring);
            if (aux == NULL){
                printf("Failed to get Nodes from server...\n");
                free(aux);
                return;
            }
            aux1 = aux + 14;
            while(sscanf(aux1, "%s %s %s", ID, IP, TCP)==3){
                if (e->routing[0][atoi(ID)][0] == '\0'){
                    //If we have a target, we only try to connect to it
                    if(target != NULL){
                        if(strcmp(target, ID) != 0){
                            aux1 += 3+strlen(ID)+strlen(IP)+strlen(TCP);
                            continue;
                        }
                    }

                    n->chordSOCK = TCPSocket(IP, TCP);
                    if(n->chordSOCK == NULL){
                        printf("Unable to connec to target node %s\n", ID);
                        free(aux);
                        return;
                    }
                    //If connection as succeceful fill n with the succ info -> s(self) = succ
                    addFD(s, getFD_Socket(n->chordSOCK));
                    strcpy(n->chordID, ID); strcpy(n->chordIP, IP); strcpy(n->chordTCP, TCP);
                    //Sending the ENTRY command
                    sprintf(buffer, "CHORD %s\n", n->selfID);
                    Send(n->chordSOCK, buffer);
                    
                    //Waiting for response!
                    addFD(s, getFD_Socket(n->chordSOCK));
                    
                    sendAllPaths(n->chordSOCK, n->selfID);

                    break;
                }
                aux1 += 3+strlen(ID)+strlen(IP)+strlen(TCP);
            }
            free(aux);
        } else printf ("Already in a chord ...");
    } else printf ("3 nodes in the ring...");
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
        freeSelect(s); free(buffer);
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
    printf("\nJoined!\nSUCC: %s [%s:%s]\nSSUCC: %s [%s:%s]\nPRED: %s\n\n", n->succID, n->succIP, n->succTCP, n->ssuccID, n->ssuccIP, n->ssuccTCP, n->predID);
    
    //Send to our new neighbour our paths
    sendAllPaths(n->predSOCK, n->selfID);
    //Send to our new neighbour our paths
    if(strcmp(n->succID, n->predID) != 0) sendAllPaths(n->succSOCK, n->selfID);
    //printf("here\n"); fflush(stdin);
    free(buffer);
    freeSelect(s);
    return 1;
}

int join(Socket *regSERV, Nodes *n, Select *sel, char *ring){
    char succID[3], succIP[16], succTCP[8];
    //Gets the list of connected Nodes
    char *aux = getNodesServer(regSERV, ring);
    if (aux == NULL){
        printf("Failed to get Nodes from server...\n");
        return 0;
    }
    //Ensures that out id is unique, if it isn't we try to get a new one
    isNodeInServer(aux, n->selfID);
    //Skip NODESLIST r\n
    if(sscanf(aux+14, "%s %s %s", succID, succIP, succTCP)!=3){
        //The ring is empty
        strcpy(n->succID, n->selfID); strcpy(n->succIP, n->selfIP); strcpy(n->succTCP, n->selfTCP);
        strcpy(n->ssuccID, n->selfID); strcpy(n->ssuccIP, n->selfIP); strcpy(n->ssuccTCP, n->selfTCP);
        strcpy(n->predID, n->selfID);
        strcpy (n->chordID, ""); strcpy (n->chordIP, ""); strcpy (n->chordTCP, "");
        free(aux);
        return 1;
    }else{
        free(aux);
        return directJoin(n, sel, succID, succIP, succTCP);
    }

}

void handleROUTE(Nodes *n, char *msg){
    char dest[4], origin[4], path[128], buffer[BUFFER_SIZE];

    if(sscanf(msg, "ROUTE %s %s %s\n", origin, dest, path) == 3){ //ROUTE 30 15 30-20-16-15<LF>
        if(addPath(e, n->selfID, origin, dest, path)){
            sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), atoi(dest), e->shorter_path[atoi(dest)]);
            broadcast(n, buffer);
        }
    }else if(sscanf(msg, "ROUTE %s %s\n", origin, dest)==2){//ROUTE 30 15<LF>
        if(addPath(e, n->selfID, origin, dest, NULL)){
            if(strcmp(e->shorter_path[atoi(dest)], "")==0){
                sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), atoi(dest));
            }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), atoi(dest), e->shorter_path[atoi(dest)]);
            broadcast(n, buffer);
        }
    }
}

void messageHANDLER(Nodes *n, char *msg){
    char origin[4], dest[4] = "", message[128], buffer[BUFFER_SIZE];
    int n_dest;
    Chord *c_aux = n->c;

    sscanf(msg, "CHAT %s %s %[^\n]\n", origin, dest, message);
    n_dest = atoi(dest);
    /***
     * Let's say that the [origin] send a message to [dest] moments before the routing message
     * arrives saying [dest] left, somewhere in the path this condition will send a message back
     * reporting that it will never arrive, for this node already left the ring...
    */
    if(e->fowarding[n_dest][0] == '\0'){
        sprintf(message, "Node [%s] is no longer in the ring...", dest);
        sprintf(buffer, "CHAT %s %s %s\n", n->selfID, origin, message);
        messageHANDLER(n, buffer);
    }else if(strcmp(dest, n->selfID) == 0){
        //We are the destination for the message
        printf("[%s]: %s\n", origin, message);
    }else {
        //Forward this message
        if(atoi(e->fowarding[n_dest])==atoi(n->predID)) Send(n->predSOCK, msg);
        else if(atoi(e->fowarding[n_dest])==atoi(n->succID)) Send(n->succSOCK, msg);
        else if(atoi(e->fowarding[n_dest])==atoi(n->chordID)) Send(n->chordSOCK, msg);
        else {
            while (c_aux != NULL){
                if (atoi(e->fowarding[n_dest])==atoi(c_aux->ID)) Send(c_aux->s, msg);
                c_aux = c_aux->next; //Isto da merda (acho eu)                                  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            }
        }
    }
}

void handleENTRY(Nodes *n, Socket *new_node, Select *s, char *msg){
    int *aux = NULL;
    char buffer[64], newID[4], newIP[16], newTCP[8];

    sscanf(msg, "ENTRY %s %s %s\n", newID, newIP, newTCP);

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

        //Send to our new neighbour our paths
        sendAllPaths(n->succSOCK, n->selfID);
    }else{
        sprintf(buffer, "ENTRY %s %s %s\n", newID, newIP, newTCP);
        Send(n->predSOCK, buffer);
        sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
        Send(new_node, buffer);
        //p(self) = new
        //Removes the fd of pred from current select
        removeFD(s, getFD_Socket(n->predSOCK));
        closeSocket(n->predSOCK, 0); n->predSOCK = NULL;

        
        //If we are more than 2 in the ring
        if(strcmp(n->predID, n->succID)!=0){
            aux = removeAdj(e, n->predID);
            for(int i = 0; aux != NULL && aux[i] != -1; i++){
                if(strcmp(e->shorter_path[aux[i]], "")==0){
                    sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
                }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
                broadcast(n, buffer);
            }
            if(aux != NULL) free(aux);
        }
        


        strcpy(n->predID, newID); n->predSOCK = new_node;
        //Adds the new fd to the select
        addFD(s, getFD_Socket(n->predSOCK));

        //Send to our new neighbour our paths
        sendAllPaths(n->predSOCK, n->selfID);
    }
}

void handleSuccDisconnect(Nodes *n, Select *s){
    int *aux;
    char buffer[BUFFER_SIZE];
    Socket *new;
    removeFD(s, getFD_Socket(n->succSOCK)); closeSocket(n->succSOCK, 1);
    n->succSOCK = NULL;

    aux = removeAdj (e, n->succID);
    for(int i = 0; aux != NULL && aux[i] != -1; i++){
        if(strcmp(e->shorter_path[aux[i]], "")==0){
            sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
        }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
        broadcast(n, buffer);
    }
    if(aux != NULL) free(aux);

    if(strcmp(n->selfID, n->ssuccID)!=0){
        new = TCPSocket(n->ssuccIP, n->ssuccTCP);
        strcpy(n->succID, n->ssuccID); strcpy(n->succIP, n->ssuccIP); strcpy(n->succTCP, n->ssuccTCP);
        n->succSOCK = new; addFD(s, getFD_Socket(new));
        sprintf(buffer, "PRED %s\n", n->selfID);
        Send(new, buffer);

        sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
        Send(n->predSOCK, buffer);

        //Send to our new neighbour our paths
        sendAllPaths(n->succSOCK, n->selfID);
    }else {//In case we are the only node left, we dont connect to ourself...
        strcpy(n->succID, n->ssuccID); strcpy(n->succIP, n->ssuccIP); strcpy(n->succTCP, n->ssuccTCP);
        strcpy(n->predID, n->ssuccID);
    }
}

void handlePredDisconnect(Nodes *n, Select *s){
    int *aux;
    char buffer[BUFFER_SIZE];

    removeFD(s, getFD_Socket(n->predSOCK)); closeSocket(n->predSOCK, 1);
    n->predSOCK = NULL;

    
    if(strcmp(n->predID, n->succID)!=0){
        aux = removeAdj (e, n->predID);
        for(int i = 0; aux != NULL && aux[i] != -1; i++){
            if(strcmp(e->shorter_path[aux[i]], "")==0){
                sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
            }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
            broadcast(n, buffer);
        }
        if(aux != NULL) free(aux);
    }
}

void handleOurChordDisconnect(Nodes *n, Select *s){
    int *aux;
    char buffer[BUFFER_SIZE];
    removeFD(s, getFD_Socket(n->chordSOCK)); closeSocket(n->chordSOCK, 1);
    n->chordSOCK = NULL;

    aux = removeAdj(e, n->chordID);
    for(int i = 0; aux != NULL && aux[i] != -1; i++){
        if(strcmp(e->shorter_path[aux[i]], "")==0){
            sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
        }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
        broadcast(n, buffer);
    }
    if(aux != NULL) free(aux);
    strcpy (n->chordID, ""); strcpy (n->chordIP, "");  strcpy (n->chordTCP, ""); 
}

void handleChordsDisconnect(Nodes *n, Select *s, Chord* c){
    int *aux;
    char buffer[BUFFER_SIZE];

    removeFD(s, getFD_Socket(c->s));

    aux = removeAdj (e, c->ID);

    n->c = deleteChord(n->c, c->ID);

    for(int i = 0; aux != NULL && aux[i] != -1; i++){
        if(strcmp(e->shorter_path[aux[i]], "")==0){
            sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
        }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
        broadcast(n, buffer);
    }
    if(aux != NULL) free(aux);
    
}

void handleSuccCommands(Nodes *n, Select *s, char *msg){
    int *aux;
    char auxID[4], auxIP[16], auxTCP[8], buffer[BUFFER_SIZE], command[16];
    Socket *new = NULL;

    if(sscanf(msg, "%s", command)){
        if(strcmp(command, "ENTRY")==0){
            sscanf(msg, "ENTRY %s %s %s\n", auxID, auxIP, auxTCP);

            //Notifies it's pred of his new ssucc
            sprintf(buffer, "SUCC %s %s %s\n", auxID, auxIP, auxTCP);
            Send(n->predSOCK, buffer);

            //demote succ to ssucc and closes connection
            strcpy(n->ssuccID, n->succID); strcpy(n->ssuccIP, n->succIP); strcpy(n->ssuccTCP, n->succTCP);
            removeFD(s, getFD_Socket(n->succSOCK));
            closeSocket(n->succSOCK, 1); n->succSOCK = NULL;

            
            //If we are more than 2 in the ring
            if(strcmp(n->predID, n->succID)!=0){
                aux = removeAdj(e, n->succID);
                for(int i = 0; aux != NULL && aux[i] != -1; i++){
                    if(strcmp(e->shorter_path[aux[i]], "")==0){
                        sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
                    }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
                    broadcast(n, buffer);
                }
                if(aux != NULL) free(aux);
            }

            if(strcmp(n->succID, n->chordID)==0){
                handleOurChordDisconnect(n, s);
            }
            
            //Set new node to succ
            strcpy(n->succID, auxID); strcpy(n->succIP, auxIP); strcpy(n->succTCP, auxTCP);
            sprintf(buffer, "PRED %s\n", n->selfID);
            new = TCPSocket(auxIP, auxTCP);
            addFD(s, getFD_Socket(new)); n->succSOCK = new;
            Send(new, buffer);

            //Send to our new neighbour our paths
            sendAllPaths(n->succSOCK, n->selfID);
        }else if(strcmp(command, "SUCC")==0){
            sscanf(msg, "SUCC %s %s %s\n", n->ssuccID, n->ssuccIP, n->ssuccTCP);
        }else if(strcmp(command, "ROUTE")==0){
            handleROUTE(n, msg);
        }else if(strcmp(command, "CHAT")==0){
            messageHANDLER(n, msg);
        }
    }
}

void handlePredCommands(Nodes *n, Select *s, char *msg){
    char protocol[8];

    if (sscanf(msg, "%s", protocol) == 1){
        if(strcmp(protocol, "ROUTE") == 0){
            handleROUTE(n, msg);
        }else if(strcmp(protocol, "CHAT")==0){
            messageHANDLER(n, msg);
        }
    }
}

/*void handleChordsCommands(Nodes *n, Select *s, char *msg){
    
}*/

void handleNewConnection(Nodes *n, Select *s, Chord **c_head, Socket *new, char *msg){
    char buffer[BUFFER_SIZE], command[16];
    Chord *new_c = NULL;

    if(sscanf(msg, "%s", command)){
        if(strcmp(command, "ENTRY")==0) handleENTRY(n, new, s, msg);
        if(strcmp(command, "PRED")==0){
            sscanf(msg, "PRED %s\n", n->predID);
            if(strcmp(n->predID, n->chordID) == 0){
                handleOurChordDisconnect(n, s);
            }
            n->predSOCK = new; addFD(s, getFD_Socket(new));

            //Send to our new neighbour our paths
            sendAllPaths(n->predSOCK, n->selfID);

            sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
            Send(new, buffer);
        }
        if(strcmp(command, "CHORD")==0){
            new_c = (Chord*)calloc(1, sizeof(Chord));
            sscanf(msg, "CHORD %s\n", new_c->ID);
            new_c->s = new; addFD(s, getFD_Socket(new));

            new_c->next = *c_head;
            *c_head = new_c;

            //Send to our new neighbour our paths
            sendAllPaths(new, n->selfID);
        }
    }
}

//Validates if s only has numbers and '.', example:
//Valid:   192.168.0.144
//Invalid: 192-169.ab .b
//Returns 1 if valid, 0 otherwise
int validateInput(char *s){
    if(s == NULL) return 0;
    for(int sz = strlen(s), i = 0; i < sz; i++){
        if(s[i] > '9' || s[i] < '0'){
            if(s[i]!='.') return 0;
        }
    }
    return 1;
}

int consoleInput(Socket *regSERV, Nodes *n, Select *s){
    char str[256], command[8], arg1[8], arg2[16], arg3[16], arg4[16], message[128], buffer[256];
    int offset = 0;
    static int connected = 0;
    static char ring[] = "---";

    if(fgets(str, 100, stdin) == NULL) return 0;

    if (sscanf(str, "%s", command) == 1) {
        offset += strlen(command) + 1; /* +1 para o espaço em branco */
        // JOIN [ring] [id]
        if (strcmp(command, "j") == 0) {
            if(connected){
                printf("Already connected! Leave current connection to enter a new one...\n");
                return 0;
            }
            if (sscanf(str + offset, "%s %s", arg1, arg2) == 2){
                strcpy(n->selfID, arg2); strcpy(ring, arg1);
                e = initEncaminhamento(n->selfID);
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
                e = initEncaminhamento(n->selfID);
                if(directJoin(n, s, arg2, arg3, arg4)){
                    connected = 1;
                }
            } else printf("Invalid interface command!\n");
        }
        // CHORD
        else if (strcmp(command, "c") == 0) {                     
            if(connected){   
                if(sscanf(str, "c %s", arg1)==1) createCHORD(n, s, regSERV, ring, arg1);
                else createCHORD(n, s, regSERV, ring, NULL);
            } else printf("Not connected...\n\n");
        }
        // REMOVE CHORD
        else if (strcmp(command, "rc") == 0) {          
            if(connected){      
                if (strcmp (n->chordID, "") != 0){
                    handleOurChordDisconnect(n, s);                  
                } else printf("Not connected by chord...\n\n");  
            } else printf("Not connected...\n\n");            
        }
        // SHOW TOPOLOGY
        else if (strcmp(command, "st") == 0) {
            if(connected){
                printf("NODE TOPOLOGY:\n\nRing: %s\n", ring);
                printf("Pred: %s\n", n->predID);
                printf("Self: %s [%s:%s]\n", n->selfID, n->selfIP, n->selfTCP);
                printf("Succ: %s [%s:%s]\n", n->succID, n->succIP, n->succTCP);
                printf("Ssucc: %s [%s:%s]\n", n->ssuccID, n->ssuccIP, n->ssuccTCP); 
                printf("Chord: %s [%s:%s]\n\n", n->chordID, n->chordIP, n->chordTCP);
            }else printf("Not connected...\n\n");         
        }
        // SHOW ROUTING [dest]
        else if (strcmp(command, "sr") == 0) {
            if(connected){
                if (sscanf(str + offset, "%s", arg1) == 1){
                    ShowRouting (e, atoi(arg1));
                } else printf("Invalid interface command!\n");
            } else printf("Not connected...\n\n");
        }
        // SHOW PATH [dest]
        else if (strcmp(command, "sp") == 0) {
            if(connected){
                if (sscanf(str + offset, "%s", arg1) == 1){
                    ShowPath (e, atoi(arg1));
                } else printf("Invalid interface command!\n");
            } else printf("Not connected...\n\n");  
        }
        // SHOW FORWARDING
        else if (strcmp(command, "sf") == 0) {          
            if(connected){      
                ShowFowarding (e); 
            } else printf("Not connected...\n\n"); 
        }
        // MESSAGE [dest] [message]
        else if (strcmp(command, "m") == 0) {
            if(connected){ 
                if (sscanf(str + 2, "%s", arg1) == 1){
                    if (sscanf(str + 5, "%[^\n]", message) != 1) return 0;                  /* EXIT 0 ??? tas maluco*/
                    if (strcmp (arg1, n->selfID) == 0) printf ("%s\n\n", message);
                    else {
                        sprintf(buffer, "CHAT %s %s %s\n", n->selfID, arg1, message);
                        messageHANDLER(n, buffer);
                    } 
                    
                } else printf("Invalid interface command!\n");
            } else printf("Not connected...\n\n"); 
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
                    if (strcmp (n->chordID, "") != 0){
                        removeFD(s, getFD_Socket(n->chordSOCK)); closeSocket(n->chordSOCK, 1);
                        n->chordSOCK = NULL;
                        strcpy (n->chordID, ""); strcpy (n->chordIP, "");  strcpy (n->chordTCP, "");
                    }
                    deleteALLChords(n->c, s);
                    n->c = NULL;
                    deleteEncaminhamento(e);
                    
                } else printf("Not connected...\n\n");            
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
                    removeFD(s, getFD_Socket(n->predSOCK)); removeFD(s, getFD_Socket(n->succSOCK));
                    closeSocket(n->predSOCK, 1); closeSocket(n->succSOCK, 1);
                    n->predSOCK = NULL; n->succSOCK = NULL;
                    if (strcmp (n->chordID, "") != 0){
                        removeFD(s, getFD_Socket(n->chordSOCK)); closeSocket(n->chordSOCK, 1);
                        n->chordSOCK = NULL;
                    }
                }
                deleteALLChords(n->c, s);
                n->c = NULL;
                deleteEncaminhamento(e);
                connected = 0;
            }
            return 1;             
        }
        else if(strcmp(command, "clear") == 0){
            if(sscanf(str+6, "%s", arg1)){
                int attempts = 0;
                char buffer[32];
                Select *udp_t = newSelect();
                addFD(udp_t, getFD_Socket(regSERV));

                sprintf(buffer, "RST %s", arg1);
                while(attempts < UDP_ATTEMPTS){
                    if(listenSelect(udp_t, UDP_TIME_OUT)>0){
                        Recieve(regSERV, buffer);
                        printf("%s cleared!\n", arg1);
                        break;
                    }
                    else {
                        printf("Server unresponsive... Retrying (%d/%d)\n", (attempts++)+1, UDP_ATTEMPTS);
                        Send(regSERV, buffer);
                    }
                }
            }
        }
        else if(strcmp(command, "nodes") == 0){
            if(sscanf(str+6, "%s", arg1)){
                char *aux = getNodesServer(regSERV, arg1);
                if (aux == NULL){
                    printf("Failed to get Nodes from server...\n");
                    return 0;
                }
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