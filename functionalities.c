#include "functionalities.h"

Encaminhamento *e = NULL;

// Function to validate command-line arguments
int validateArguments(int argc, char **argv, char *IP, char *TCP, char *regIP, char *regUDP){
    if(argc != 3 && argc != 5){
        printf("\n Invalid Arguments!\nUsage: COR IP TCP [regIP] [regUDP]\n\n");
        return 1;
    } else {
        if (check_ipv4(argv[1])) return 1;
        strcpy(IP, argv[1]); strcpy(TCP, argv[2]);
        if(argc == 5){
            if (check_ipv4(argv[3])) return 1;
            strcpy(regIP, argv[3]); strcpy(regUDP, argv[4]);
        }
        return 0;
    }
}

// Function to register a node in the server
int registerInServer(Socket *server, char *ring, Nodes *n){
    int attempts = 0;
    char buffer[128];
    Select *udp_t = newSelect();
    addFD(udp_t, getFD_Socket(server));

    // Formulate registration message
    sprintf(buffer, "REG %s %s %s %s", ring, n->selfID, n->selfIP, n->selfTCP);
    Send(server, buffer); // Send registration message
    while(attempts < UDP_ATTEMPTS){
        if(listenSelect(udp_t, UDP_TIME_OUT)>0){
            Recieve(server, buffer); // Receive response
            break;
        } else {
            printf("\n Server unresponsive... Retrying (%d/%d)\n\n", (attempts++)+1, UDP_ATTEMPTS);
            Send(server, buffer); // Retry sending registration message
        }
    }
    freeSelect(udp_t);

    // Check registration response
    if(strcmp(buffer, "OKREG")==0){
        printf("\n Registered as %s in ring %s!\n\n", n->selfID, ring);
        return 1;
    } else {
        printf("\n %s\n\n", buffer); // Print error message
        return 0;
    }
}

// Function to unregister a node from the server
int unregisterInServer(Socket *server, char *ring, Nodes *n){
    int attempts = 0;
    char buffer[64];
    Select *udp_t = newSelect();
    addFD(udp_t, getFD_Socket(server));

    // Formulate unregistration message
    sprintf(buffer, "UNREG %s %s", ring, n->selfID);
    Send(server, buffer); // Send unregistration message
    while(attempts < UDP_ATTEMPTS){
        if(listenSelect(udp_t, UDP_TIME_OUT)>0){
            Recieve(server, buffer); // Receive response
            break;
        } else {
            printf("\n Server unresponsive... Retrying (%d/%d)\n\n", (attempts++)+1, UDP_ATTEMPTS);
            Send(server, buffer); // Retry sending unregistration message
        }
    }
    freeSelect(udp_t);

    // Check unregistration response
    if(strcmp(buffer, "OKUNREG")==0){
        printf("\n Leaving ring %s...\n\n", ring);
        return 1;
    } else {
        printf("\n %s\n\n", buffer); // Print error message
        return 0;
    }
}

// Function to retrieve nodes from the server
char *getNodesServer(Socket *server, char *ring){
    char *buffer =(char*)malloc(sizeof(char)*BUFFER_SIZE);
    int attempts = 0;

    Select *udp_t = newSelect();
    addFD(udp_t, getFD_Socket(server));

    // Formulate request for nodes
    sprintf(buffer, "NODES %s", ring);
    Send(server, buffer); // Send request for nodes
    while(attempts < UDP_ATTEMPTS){
        if(listenSelect(udp_t, UDP_TIME_OUT)>0){
            Recieve(server, buffer); // Receive response
            break;
        } else {
            printf("\n Server unresponsive... Retrying (%d/%d)\n\n", (attempts++)+1, UDP_ATTEMPTS);
            Send(server, buffer); // Retry sending request for nodes
        }
    }
    freeSelect(udp_t);
    if(attempts == UDP_ATTEMPTS){
        free(buffer); // Free buffer in case of failure
        return NULL;
    }
    return buffer; // Return received nodes information
}

// Function to check if a node is in the server
void isNodeInServer(char *nodeslist, char *selfID){
    // Skip NODESLIST
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
                if(id[0]=='9' && id[1]=='9'){
                    strcpy(id, "00");
                    break;
                }
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
    strcpy(selfID, id); // Update selfID with the chosen ID
    return;
}

// Function to send all paths
void sendAllPaths(Socket *s, char *self){
    char buffer[BUFFER_SIZE];

    for(int i = 0; i < 100; i++){
        if(e->shorter_path[i][0] != '\0'){
            // Formulate path information and send
            sprintf(buffer, "ROUTE %d %d %s\n", atoi(self), i, e->shorter_path[i]);
            Send(s, buffer);
        }
    }
}

// Function to broadcast a message to all relevant nodes in the network
void broadcast(Nodes *n, char *msg){
    Chord *aux = n->c;
    // Send message to successor node if it exists
    if(n->succSOCK != NULL) Send(n->succSOCK, msg);
    // Send message to predecessor node if it exists and is not the same as the successor
    if(n->predSOCK != NULL && (strcmp(n->succID, n->predID)!=0)) Send(n->predSOCK, msg);
    // Send message to chord node if it exists
    if(n->chordSOCK != NULL) Send(n->chordSOCK, msg);
    // Send message to all nodes in the chord list
    while (aux != NULL){
        if(aux->s != NULL) Send(aux->s, msg);
        aux = aux->next;
    }
}

// Function to delete a chord node from the chord list
Chord *deleteChord(Chord *head, char *ID){
    Chord *aux1 = head, *aux2 = head;

    if(head == NULL) return NULL;
    // Traverse the chord list to find and delete the node with the specified ID
    while(aux1 != NULL){
        if(strcmp(aux1->ID, ID)==0){
            if(aux1 == head){
                // If the node to delete is the head of the list
                aux2 = aux1->next;
                closeSocket(aux1->s, 1);
                free(aux1);
                return aux2; // Return the new head of the list
            } else {
                // If the node to delete is not the head of the list
                aux2->next = aux1->next;
                closeSocket(aux1->s, 1);
                free(aux1);
                return head; // Return the head of the list remains unchanged
            }
        }
        aux2 = aux1;
        aux1 = aux1->next;
    }
    return head; // Return the head of the list if the specified ID is not found
}

// Function to delete all chord nodes from the chord list
void deleteALLChords(Chord *head, Select *s){
    Chord *aux = head, *aux2 = NULL;

    // Traverse the chord list and delete each node
    while(aux != NULL){
        aux2 = aux->next;
        removeFD(s, getFD_Socket(aux->s));
        closeSocket(aux->s, 1);
        free(aux);
        aux = aux2;
    }
}

// Function to create a chord node and join it to the network
void createCHORD(Nodes *n, Select *s, Socket *server, char *ring, char *target){
    char ID[4], IP[16], TCP[8], buffer[16];
    char *aux = NULL, *aux1 = NULL;
    int flag = 0;

    if (!(strcmp(n->predID, n->ssuccID) == 0 || strcmp(n->predID, n->succID) == 0)){ // More than 3 nodes exist
         if (strcmp (n->chordID, "") == 0){ // Node is not already part of a chord
            aux = getNodesServer(server, ring);
            if (aux == NULL){
                printf("\n Failed to get Nodes from server...\n\n");
                free(aux);
                return;
            }
            aux1 = aux + 14;
            // Iterate through nodes received from the server
            while(sscanf(aux1, "%s %s %s", ID, IP, TCP)==3){
                if (e->routing[0][atoi(ID)][0] == '\0'){ // Check if the node is not already in the routing table
                    // If a target node is specified, only try to connect to it
                    if(target != NULL){
                        if(strcmp(target, ID) == 0){
                            flag = 1;
                            break;
                        }
                    }
                }
                aux1 += 3+strlen(ID)+strlen(IP)+strlen(TCP);
            }
            if (flag == 0){
                aux1 = aux + 14;
                while(sscanf(aux1, "%s %s %s", ID, IP, TCP)==3){
                    if (e->routing[0][atoi(ID)][0] == '\0'){
                        if (target != NULL) printf ("\n Node %s is not resgistred in server!\n\n", target);
                        break;
                    }
                    aux1 += 3+strlen(ID)+strlen(IP)+strlen(TCP);
                }
            }
            printf ("\n Your chord is %s!\n\n", ID);
            n->chordSOCK = TCPSocket(IP, TCP);
            if(n->chordSOCK == NULL){
                printf("\n Unable to connect to target node %s\n\n", ID);
                free(aux);
                return;
            }
            // If connection is successful, update node's information and send the ENTRY command
            addFD(s, getFD_Socket(n->chordSOCK));
            strcpy(n->chordID, ID); strcpy(n->chordIP, IP); strcpy(n->chordTCP, TCP);
            sprintf(buffer, "CHORD %s\n", n->selfID);
            Send(n->chordSOCK, buffer); // Send ENTRY command to the target node
            addFD(s, getFD_Socket(n->chordSOCK)); // Wait for response
            sendAllPaths(n->chordSOCK, n->selfID); // Send all paths to the newly connected node
            free(aux);
        } else printf ("\n Already in a chord ...\n\n"); // Node is already part of a chord
    } else printf ("\n Less than 4 nodes in the ring...\n\n"); // Only 3 nodes exist in the ring
}

// Function to directly join a ring by connecting to the successor node and waiting for the predecessor to connect
int directJoin(Nodes *n, Select *sel, char *succID, char *succIP, char *succTCP){
    // Allocate memory for buffer
    char *buffer = (char*)malloc(BUFFER_SIZE*sizeof(char));
    
    // New select structure to listen to incoming data from sockets
    Select *s = newSelect();
    
    // Attempt to connect to the successor node
    Socket *succ = TCPSocket(succIP, succTCP);
    if(succ == NULL){
        printf("\n Unable to connect to successor %s\n\n", succID);
        freeSelect(s); 
        free(buffer);
        return 0;
    }
    
    // If connection was successful, fill n with the successor info -> s(self) = succ
    addFD(sel, getFD_Socket(succ));
    strcpy(n->succID, succID); 
    strcpy(n->succIP, succIP); 
    strcpy(n->succTCP, succTCP);
    n->succSOCK = succ;
    
    // Sending the ENTRY command
    sprintf(buffer, "ENTRY %s %s %s\n", n->selfID, n->selfIP, n->selfTCP);
    Send(succ, buffer);
    
    // Waiting for response
    addFD(s, getFD_Socket(succ));
    if(listenSelect(s, TIME_OUT) > 0){
        // If the successor sent a response
        if(checkFD(s, getFD_Socket(succ))){
            if(Recieve(succ, buffer)<=0){
                // In case of timeout
                printf("\n Successor %s disconnected from us...\n\n", succID);
                removeFD(sel, getFD_Socket(succ));
                closeSocket(succ, 1);
                free(buffer);
                freeSelect(s);
                n->succSOCK = NULL;
                return 0;
            }
            sscanf(buffer, "SUCC %s %s %s\n", n->ssuccID, n->ssuccIP, n->ssuccTCP);
            // Fill the ssucc info in n
        }
    }else{
        // In case of timeout
        printf("\n Timed-out waiting for successor %s after %d seconds...\n\n", succID, TIME_OUT);
        removeFD(sel, getFD_Socket(succ));
        closeSocket(succ, 1);
        free(buffer);
        freeSelect(s);
        n->succSOCK = NULL;
        return 0;
    }

    // Now we will wait for the predecessor to connect announcing itself
    // We will listen to our TCP port
    removeFD(s, getFD_Socket(succ));
    addFD(s, getFD_Socket(n->selfSOCK));
    if(listenSelect(s, TIME_OUT) > 0){
        // If the predecessor connected, we will wait for its response
        if(checkFD(s, getFD_Socket(n->selfSOCK))){
            n->predSOCK = TCPserverAccept(n->selfSOCK);
            if(Recieve(n->predSOCK, buffer)<=0){
                printf("\n Predecessor disconnected from us...\n\n");
                removeFD(sel, getFD_Socket(n->succSOCK));
                closeSocket(succ, 1);
                closeSocket(n->predSOCK, 1);
                free(buffer);
                freeSelect(s);
                n->predSOCK = NULL;
                n->succSOCK = NULL;
                return 0;
            }
            sscanf(buffer, "PRED %s\n", n->predID);
            addFD(sel, getFD_Socket(n->predSOCK));
        }
    }else{
        // In case of timeout
        printf("\n Timed-out waiting for predecessor after %d seconds...\n\n", TIME_OUT);
        free(buffer);
        freeSelect(s);
        removeFD(sel, getFD_Socket(n->succSOCK));
        closeSocket(succ, 1);
        closeSocket(n->predSOCK, 1);
        n->succSOCK = NULL;
        return 0;
    }
    
    printf("\n Joined!\n SUCC: %s [%s:%s]\n SSUCC: %s [%s:%s]\n PRED: %s\n\n", n->succID, n->succIP, n->succTCP, n->ssuccID, n->ssuccIP, n->ssuccTCP, n->predID);
    
    // Send to our new neighbor our paths
    sendAllPaths(n->predSOCK, n->selfID);
    // Send to our new neighbor our paths
    if(strcmp(n->succID, n->predID) != 0) 
        sendAllPaths(n->succSOCK, n->selfID);
    
    free(buffer);
    freeSelect(s);
    return 1;
}

// Function to join a ring by connecting to the successor node and waiting for the predecessor to connect
int join(Socket *regSERV, Nodes *n, Select *sel, char *ring, char *suc){
    char succID[3], succIP[16], succTCP[8];
    char *aux = NULL, *aux2 = NULL;
    int flag = 0;

    // Gets the list of connected Nodes
    aux = getNodesServer(regSERV, ring);

    if (aux == NULL){
        printf("\n Failed to get Nodes from server...\n\n");
        return 0;
    }
    
    // Ensures that our id is unique, if it isn't we try to get a new one
    isNodeInServer(aux, n->selfID);
    e = initEncaminhamento(n->selfID);
    
    // Skip NODESLIST
    if(sscanf(aux+14, "%s %s %s", succID, succIP, succTCP)!=3){
        // The ring is empty
        strcpy(n->succID, n->selfID); 
        strcpy(n->succIP, n->selfIP); 
        strcpy(n->succTCP, n->selfTCP);
        strcpy(n->ssuccID, n->selfID); 
        strcpy(n->ssuccIP, n->selfIP); 
        strcpy(n->ssuccTCP, n->selfTCP);
        strcpy(n->predID, n->selfID);
        strcpy(n->chordID, ""); 
        strcpy(n->chordIP, ""); 
        strcpy(n->chordTCP, "");
        free(aux);
        return 1;
    } else {
        if (suc != NULL){
            aux2 = aux + 14;
            while (sscanf(aux2, "%s %s %s\n", succID, succIP, succTCP)==3){
                if (strcmp(suc, succID) == 0){
                    flag = 1;
                    break;
                }
                aux2 += (strlen(succID) + strlen(succIP) + strlen(succTCP) + 3);
            }
            if (flag == 0){
                if (sscanf(aux+14, "%s %s %s", succID, succIP, succTCP)==3){
                    printf ("\n Node %s is not resgistred in server!\n\n", suc);
                }
            }
        }
        printf ("\n Your successor is %s!\n\n", succID);
        free(aux);
        return directJoin(n, sel, succID, succIP, succTCP);
    }
}

// Handles routing messages received from other nodes in the network. Parses the incoming message and updates the routing table accordingly.
void handleROUTE(Nodes *n, char *msg){
    char dest[4], origin[4], path[128], buffer[BUFFER_SIZE];

    // Check if the message contains complete routing information
    if(sscanf(msg, "ROUTE %s %s %s\n", origin, dest, path) == 3){ //ROUTE 30 15 30-20-16-15<LF>
        // Add the received path information to the routing table
        if(addPath(e, n->selfID, origin, dest, path)){
            // Broadcast the updated routing information
            sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), atoi(dest), e->shorter_path[atoi(dest)]);
            broadcast(n, buffer);
        }
    }else if(sscanf(msg, "ROUTE %s %s\n", origin, dest) == 2){//ROUTE 30 15<LF>
        // Add the received routing information to the routing table
        if(addPath(e, n->selfID, origin, dest, NULL)){
            // Broadcast the updated routing information
            if(strcmp(e->shorter_path[atoi(dest)], "") == 0){
                sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), atoi(dest));
            }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), atoi(dest), e->shorter_path[atoi(dest)]);
            broadcast(n, buffer);
        }
    }
}

// Handles incoming chat messages in the network. Forwards the message to the appropriate destination or handles it locally.
void messageHANDLER(Nodes *n, char *msg){
    char origin[4], dest[4] = "", message[128], buffer[BUFFER_SIZE];
    int n_dest;
    Chord *c_aux = n->c;

    // Extract information from the incoming message
    sscanf(msg, "CHAT %s %s %[^\n]\n", origin, dest, message);
    n_dest = atoi(dest);

    // Check if the destination is still in the ring
    if(e->forwarding[n_dest][0] == '\0'){
        // Send a message back indicating that the destination node is no longer in the ring
        sprintf(message, "Node [%s] is no longer in the ring...", dest);
        sprintf(buffer, "CHAT %s %s %s\n", n->selfID, origin, message);
        messageHANDLER(n, buffer);
    } else if (n_dest == atoi (n->selfID)){
        // Print the message locally if the node is the destination
        printf("[%s] %s\n", origin, message);
    }else {
        // Forward the message to the appropriate node
        if(atoi(e->forwarding[n_dest]) == atoi(n->predID)) Send(n->predSOCK, msg);
        else if(atoi(e->forwarding[n_dest]) == atoi(n->succID)) Send(n->succSOCK, msg);
        else if(strcmp(n->chordID, "") != 0 && atoi(e->forwarding[n_dest]) == atoi(n->chordID)) Send(n->chordSOCK, msg);
        else {
            // Forward the message through the chord connections
            while (c_aux != NULL){
                if (atoi(e->forwarding[n_dest]) == atoi(c_aux->ID)) Send(c_aux->s, msg);
                c_aux = c_aux->next;
            }
        }
    }
}

// Handles a new node attempting to join the network. Determines the appropriate actions to take based on the current network state
// and the information provided by the new node.
void handleENTRY(Nodes *n, Socket *new_node, Select *s, char *msg){
    int *aux = NULL;
    char buffer[64], newID[4], newIP[16], newTCP[8], *aux_ip;

    // Extract information from the incoming message
    sscanf(msg, "ENTRY %s %s %s\n", newID, newIP, newTCP);
    aux_ip = getAddress(new_node);

    // Check if the provided IP matches the actual IP of the new node
    if(strcmp(newIP, aux_ip) != 0){
        printf("\n New connection announced a different IP from its own...\nDisconnecting from them...\n\n");
        closeSocket(new_node, 1);
        return;
    }

    // Check if the new node has the same ID as the current node
    if(strcmp(newID, n->selfID) == 0){
        printf("\n New connection trying the same ID...\nDisconnecting from them...\n\n");
        closeSocket(new_node, 1);
        return;
    }

    // Handle the case when the successor ID is the same as the current node's ID
    if(strcmp(n->succID, n->selfID) == 0){
        // Set the new node as predecessor and successor
        strcpy(n->predID, newID); n->predSOCK = new_node;
        addFD(s, getFD_Socket(new_node));

        // Notify the new node of its successor status
        sprintf(buffer, "SUCC %s %s %s\n", newID, newIP, newTCP);
        Send(new_node, buffer);

        // Set the current node as successor of the new node
        n->succSOCK = TCPSocket(newIP, newTCP);
        sprintf(buffer, "PRED %s\n", n->selfID);
        Send(n->succSOCK, buffer);
        strcpy(n->succID, newID); strcpy(n->succIP, newIP); strcpy(n->succTCP, newTCP);
        addFD(s, getFD_Socket(n->succSOCK));

        // Send paths to the new successor
        sendAllPaths(n->succSOCK, n->selfID);
    }else{
        // Forward the entry message to the predecessor
        sprintf(buffer, "ENTRY %s %s %s\n", newID, newIP, newTCP);
        Send(n->predSOCK, buffer);
        // Notify the new node of its successor
        sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
        Send(new_node, buffer);

        // Set the new node as the predecessor
        removeFD(s, getFD_Socket(n->predSOCK));
        closeSocket(n->predSOCK, 0); n->predSOCK = NULL;

        // If there are more than two nodes in the ring, update paths
        if(strcmp(n->predID, n->succID) != 0){
            aux = removeAdj(e, n->predID);
            for(int i = 0; aux != NULL && aux[i] != -1; i++){
                if(strcmp(e->shorter_path[aux[i]], "") == 0){
                    sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
                }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
                broadcast(n, buffer);
            }
            if(aux != NULL) free(aux);
        }

        // Set the new node as predecessor
        strcpy(n->predID, newID); n->predSOCK = new_node;
        addFD(s, getFD_Socket(n->predSOCK));

        // Send paths to the new predecessor
        sendAllPaths(n->predSOCK, n->selfID);
    }
}

// Handles the disconnection of the successor node. Determines the appropriate actions to take based on the current network state.
void handleSuccDisconnect(Nodes *n, Select *s){
    int *aux;
    char buffer[BUFFER_SIZE];
    Socket *new;

    // Close the socket of the disconnected successor
    removeFD(s, getFD_Socket(n->succSOCK)); closeSocket(n->succSOCK, 1);
    n->succSOCK = NULL;

    // Update adjacent nodes and broadcast the changes
    aux = removeAdj(e, n->succID);
    for(int i = 0; aux != NULL && aux[i] != -1; i++){
        if(strcmp(e->shorter_path[aux[i]], "")==0){
            sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
        }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
        broadcast(n, buffer);
    }
    if(aux != NULL) free(aux);

    // If the secondary successor is available, connect to it
    if(strcmp(n->selfID, n->ssuccID) != 0){
        // If the secondary successor is connected via the chord, remove the chord
        if(strcmp(n->chordID, n->ssuccID) == 0) handleOurChordDisconnect(n, s);

        // Establish connection with the secondary successor
        new = TCPSocket(n->ssuccIP, n->ssuccTCP);
        strcpy(n->succID, n->ssuccID); strcpy(n->succIP, n->ssuccIP); strcpy(n->succTCP, n->ssuccTCP);
        n->succSOCK = new; addFD(s, getFD_Socket(new));
        sprintf(buffer, "PRED %s\n", n->selfID);
        Send(new, buffer);

        // Notify the predecessor of the new successor
        sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
        Send(n->predSOCK, buffer);

        // Send paths to the new successor
        sendAllPaths(n->succSOCK, n->selfID);
    }else {
        // In case the current node is the only node left, set successor and predecessor to itself
        strcpy(n->succID, n->ssuccID); strcpy(n->succIP, n->ssuccIP); strcpy(n->succTCP, n->ssuccTCP);
        strcpy(n->predID, n->ssuccID);
    }
}

// Handles disconnection of the predecessor node. Removes predecessor's socket from the select structure, closes the socket, 
// and updates node information accordingly. If there was a pending predecessor connection, it sets the pending connection as the new predecessor.
void handlePredDisconnect(Nodes *n, Select *s){
    int *aux;
    char buffer[BUFFER_SIZE];

    // Remove predecessor's socket from the select structure and close the socket
    removeFD(s, getFD_Socket(n->predSOCK)); 
    closeSocket(n->predSOCK, 1);
    n->predSOCK = NULL;

    // If there are more than two nodes in the ring, handle routing information
    if(strcmp(n->predID, n->succID) != 0){
        aux = removeAdj (e, n->predID);
        for(int i = 0; aux != NULL && aux[i] != -1; i++){
            if(strcmp(e->shorter_path[aux[i]], "") == 0){
                sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
            }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
            broadcast(n, buffer);
        }
        if(aux != NULL) free(aux);
    }

    // If there was a pending predecessor connection, set it as the new predecessor
    if(n->possible_predSOCK != NULL){
        printf("\n Setting %s as pred\n\n", n->possible_predID);
        strcpy(n->predID, n->possible_predID);
        n->predSOCK = n->possible_predSOCK;
        n->possible_predSOCK = NULL;

        // Handle chord disconnection if the new predecessor is our own chord
        if(strcmp(n->predID, n->chordID) == 0){
            handleOurChordDisconnect(n, s);
        }
        
        addFD(s, getFD_Socket(n->predSOCK)); // Add the new predecessor's socket to the select structure

        // Send successor information to the new predecessor
        sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
        Send(n->predSOCK, buffer);

        // Send our paths to the new predecessor
        sendAllPaths(n->predSOCK, n->selfID);
    }
}

// Handles disconnection of the node's own chord. Removes the chord's socket from the select structure, closes the socket, and updates node's chord information.
void handleOurChordDisconnect(Nodes *n, Select *s){
    int *aux;
    char buffer[BUFFER_SIZE];

    // Remove chord's socket from the select structure and close the socket
    removeFD(s, getFD_Socket(n->chordSOCK)); 
    closeSocket(n->chordSOCK, 1);
    n->chordSOCK = NULL;

    // Handle routing information for the disconnected chord
    aux = removeAdj(e, n->chordID);
    for(int i = 0; aux != NULL && aux[i] != -1; i++){
        if(strcmp(e->shorter_path[aux[i]], "") == 0){
            sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
        }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
        broadcast(n, buffer);
    }
    if(aux != NULL) free(aux);

    // Clear node's own chord information
    strcpy(n->chordID, ""); 
    strcpy(n->chordIP, "");  
    strcpy(n->chordTCP, ""); 
}

// Handles disconnection of a chord node. Removes the chord's socket from the select structure, updates node's chord list, and handles routing information.
void handleChordsDisconnect(Nodes *n, Select *s, Chord* c){
    int *aux;
    char buffer[BUFFER_SIZE];

    // Remove chord's socket from the select structure
    removeFD(s, getFD_Socket(c->s));

    // Handle routing information for the disconnected chord
    aux = removeAdj (e, c->ID);

    // Remove the chord node from the chord list
    n->c = deleteChord(n->c, c->ID);
    for(int i = 0; aux != NULL && aux[i] != -1; i++){
        if(strcmp(e->shorter_path[aux[i]], "")==0){
            sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
        }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
        broadcast(n, buffer);
    }
    if(aux != NULL) free(aux);
}

// Handles commands received from the successor node. Parses the incoming message and takes appropriate actions based on the command received.
void handleSuccCommands(Nodes *n, Select *s, char *msg){
    int *aux;
    char auxID[4], auxIP[16], auxTCP[8], buffer[BUFFER_SIZE], command[16];
    Socket *new = NULL;

    // Parse the command from the incoming message
    if(sscanf(msg, "%s", command)){
        // Handle the ENTRY command
        if(strcmp(command, "ENTRY")==0){
            sscanf(msg, "ENTRY %s %s %s\n", auxID, auxIP, auxTCP);

            // Notifies its predecessor of its new successor
            sprintf(buffer, "SUCC %s %s %s\n", auxID, auxIP, auxTCP);
            Send(n->predSOCK, buffer);

            // Demote successor to secondary successor and close connection
            strcpy(n->ssuccID, n->succID); strcpy(n->ssuccIP, n->succIP); strcpy(n->ssuccTCP, n->succTCP);
            removeFD(s, getFD_Socket(n->succSOCK));
            closeSocket(n->succSOCK, 1); n->succSOCK = NULL;

            // If there are more than two nodes in the ring
            if(strcmp(n->predID, n->succID) != 0){
                aux = removeAdj(e, n->succID);
                for(int i = 0; aux != NULL && aux[i] != -1; i++){
                    if(strcmp(e->shorter_path[aux[i]], "") == 0){
                        sprintf(buffer, "ROUTE %d %d\n", atoi(n->selfID), aux[i]);
                    }else sprintf(buffer, "ROUTE %d %d %s\n", atoi(n->selfID), aux[i], e->shorter_path[aux[i]]);
                    broadcast(n, buffer);
                }
                if(aux != NULL) free(aux);
            }

            // Handle chord disconnection if the successor is our own chord
            if(strcmp(n->succID, n->chordID) == 0){
                handleOurChordDisconnect(n, s);
            }
            
            // Set new node as successor
            strcpy(n->succID, auxID); strcpy(n->succIP, auxIP); strcpy(n->succTCP, auxTCP);
            sprintf(buffer, "PRED %s\n", n->selfID);
            new = TCPSocket(auxIP, auxTCP);
            addFD(s, getFD_Socket(new)); n->succSOCK = new;
            Send(new, buffer);

            // Send our paths to the new successor
            sendAllPaths(n->succSOCK, n->selfID);
        }else if(strcmp(command, "SUCC")==0){
            sscanf(msg, "SUCC %s %s %s\n", n->ssuccID, n->ssuccIP, n->ssuccTCP);
        }else if(strcmp(command, "ROUTE")==0){
            // Handle routing information
            handleROUTE(n, msg);
        }else if(strcmp(command, "CHAT")==0){
            // Handle chat messages
            messageHANDLER(n, msg);
        }
    }
}

// Handles commands received from the predecessor node. Parses the incoming message and takes appropriate actions based on the command received.
void handlePredCommands(Nodes *n, Select *s, char *msg){
    char protocol[8];

    // Parse the protocol from the incoming message
    if (sscanf(msg, "%s", protocol) == 1){
        // Handle routing information
        if(strcmp(protocol, "ROUTE") == 0){
            handleROUTE(n, msg);
        // Handle chat messages
        }else if(strcmp(protocol, "CHAT")==0){
            messageHANDLER(n, msg);
        }
    }
}
// Handles a new connection received by the server. Parses the incoming message and takes appropriate actions based on the command received.
void handleNewConnection(Nodes *n, Select *s, Chord **c_head, Socket *new, char *msg){
    char buffer[BUFFER_SIZE], command[16];
    Chord *new_c = NULL;

    // Parse the command from the incoming message
    if(sscanf(msg, "%s", command)){
        // Handle the ENTRY command
        if(strcmp(command, "ENTRY")==0) 
            handleENTRY(n, new, s, msg);
        // Handle the PRED command
        if(strcmp(command, "PRED")==0){
            // Pred is still connected to us, we leave this new connection on hold
            if(n->predSOCK != NULL){
                sscanf(msg, "PRED %s\n", n->possible_predID);
                n->possible_predSOCK = new;
                return;
            }

            sscanf(msg, "PRED %s\n", n->predID);
            // Check if the predecessor is our own chord, indicating a disconnect
            if(strcmp(n->predID, n->chordID) == 0){
                handleOurChordDisconnect(n, s);
            }
            n->predSOCK = new; // Set the new socket as predecessor
            addFD(s, getFD_Socket(new)); // Add socket to the select structure

            // Send our successor information to the new neighbor
            sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
            Send(new, buffer);

            // Send our paths to the new neighbor
            sendAllPaths(new, n->selfID);
        }
        // Handle the CHORD command
        if(strcmp(command, "CHORD")==0){
            // Allocate memory for a new Chord struct
            new_c = (Chord*)calloc(1, sizeof(Chord));
            sscanf(msg, "CHORD %s\n", new_c->ID);
            new_c->s = new; // Set the socket for the new Chord node
            addFD(s, getFD_Socket(new)); // Add socket to the select structure

            // Add the new Chord node to the Chord list
            new_c->next = *c_head;
            *c_head = new_c;

            // Send our paths to the new neighbor
            sendAllPaths(new, n->selfID);
        }
    }
}

// Function to process user input for network interaction, including joining/leaving networks, message exchange, and displaying network topology.
int consoleInput(Socket *regSERV, Nodes *n, Select *s){
    // Declare necessary variables
    char str[256], command[8], arg1[8], arg2[16], arg3[16], arg4[16], message[128], buffer[256];
    int offset = 0, aux1 = 0;
    static int connected = 0; // Static variable to keep track of connection status
    static char ring[] = "---"; // Static variable to store ring information
    Chord *aux = NULL;

    // Read input from console
    if(fgets(str, 100, stdin) == NULL) return 0;

    // Extract command from input
    if (sscanf(str, "%s", command) == 1) {
        offset += strlen(command) + 1; /* +1 for the space */
        // JOIN [ring] [id]
        if (strcmp(command, "j") == 0) {
            // Check if already connected
            if(connected){
                printf("\n Already connected! Leave current connection to enter a new one...\n\n");
                return 0;
            }
            // Parse input arguments
            if (sscanf(str + offset, "%s %s %s", arg1, arg2, arg3) == 3){
                // Validate input arguments
                if (strlen(arg1) == 3 && strlen(arg2) == 2 && strlen(arg3) == 2 \
                    && atoi(arg1) >= 0 && atoi(arg1) <= 999 && atoi(arg2) >= 0 \
                    && atoi(arg2) <= 99 && atoi(arg3) >= 0 && atoi(arg3) <= 99){
                    // Copy arguments to node structure
                    strcpy(n->selfID, arg2); strcpy(ring, arg1);
                    // Join the ring
                    if(join(regSERV, n, s, ring, arg3)){
                        // Register node in server
                        if(registerInServer(regSERV, ring, n)) connected = 1;
                    }else{
                        // Handle failure
                        deleteEncaminhamento(e);
                    }
                } else printf ("\n Invalid nodes or ring command!\n\n");
            } else {
                // Handle different input format
                if (sscanf(str + offset, "%s %s", arg1, arg2) == 2){
                    if (strlen(arg1) == 3 && strlen(arg2) == 2 && atoi(arg1) >= 0 \
                        && atoi(arg1) <= 999 && atoi(arg2) >= 0 && atoi(arg2) <= 99){
                        strcpy(n->selfID, arg2); strcpy(ring, arg1);
                        if(join(regSERV, n, s, ring, NULL)){
                            if(registerInServer(regSERV, ring, n)) connected = 1;
                        }else {
                            deleteEncaminhamento(e);
                        } 
                    } else printf ("\n Invalid node or ring command1!\n\n");
                } else printf("\n Invalid interface command!\n\n");
            }
        }
        // DIRECT JOIN [id] [succid] [succIP] [succTCP]
        else if (strcmp(command, "dj") == 0) {
            // Check if already connected
            if(connected){
                printf("\n Already connected! Leave current connection to enter a new one...\n\n");
                return 0;
            }
            // Parse input arguments
            if (sscanf(str + offset, "%s %s %s %s", arg1, arg2, arg3, arg4) == 4){
                if (strlen(arg1) == 2 && strlen(arg2) == 2 && atoi(arg1) >= 0 \
                    && atoi(arg1) <= 99 && atoi(arg2) >= 0 && atoi(arg2) <= 99){
                    // Copy argument to node structure
                    strcpy(n->selfID, arg1);
                    // Initialize encaminhamento
                    e = initEncaminhamento(n->selfID);
                    // Perform direct join
                    if(directJoin(n, s, arg2, arg3, arg4)){
                        connected = 1;
                    }else deleteEncaminhamento(e);
                } else printf ("\n Invalid nodes command!\n\n");
            } else printf("\n Invalid interface command!\n\n");
        }
        // CHORD
        else if (strcmp(command, "c") == 0) {                     
            if(connected){   
                // Handle chord creation
                if(sscanf(str, "c %s", arg1)==1){
                    if (strlen(arg1) == 2 && atoi(arg1) >= 0 && atoi(arg1) <= 99){
                        createCHORD(n, s, regSERV, ring, arg1);
                    } else printf ("\n Invalid node command!\n\n");
                }
                else createCHORD(n, s, regSERV, ring, NULL);
            } else printf("\n Not connected...\n\n");
        }
        // REMOVE CHORD
        else if (strcmp(command, "rc") == 0) {          
            if(connected){      
                // Handle chord removal
                if (strcmp (n->chordID, "") != 0){
                    handleOurChordDisconnect(n, s);                  
                } else printf("\n Not connected by chord...\n\n");  
            } else printf("\n Not connected...\n\n");            
        }
        // SHOW TOPOLOGY
        else if (strcmp(command, "st") == 0) {
            // Display network topology
            if(connected){
                // Print network topology
                printf("\n\n     TOPOLOGY\n\n");
                printf (" ----------------- \n");
                printf(" |  Ring  |  %s |\n", ring);
                printf (" ----------------- \n");
                // Print node information
                printf(" |  Pred  |  %s  |\n", n->predID);
                printf (" ----------------- \n");
                printf(" | \e[1m Self \e[m | \e[1m %s \e[m |   [%s : %s]\n", n->selfID, n->selfIP, n->selfTCP);
                printf (" ----------------- \n");
                printf(" |  Succ  |  %s  |   [%s : %s]\n", n->succID, n->succIP, n->succTCP);
                printf (" ----------------- \n");
                printf(" |  Ssucc |  %s  |   [%s : %s]\n", n->ssuccID, n->ssuccIP, n->ssuccTCP); 
                printf (" ----------------- \n");
                // Print chord information if available
                if (strcmp (n->chordID, "") != 0){
                    printf(" |  Chord |  %s  |   [%s : %s]\n", n->chordID, n->chordIP, n->chordTCP);
                    printf (" ----------------- \n");
                }
                // Print information about other chords
                if (n->c != NULL){
                    printf(" | Others Chords |");
                    // Iterate through other chords and print their IDs
                    aux = n->c;
                    while (aux != NULL){
                        if (aux1 == 0){
                            printf ("   %s ", aux->ID);
                            aux1++;
                        } else printf (" /  %s ", aux->ID);
                        aux = aux->next;
                    }
                    printf ("\n ----------------- \n");
                }
                
            } else printf("\n Not connected...\n");      
            printf ("\n\n");   
        }
        // SHOW ROUTING [dest]
        else if (strcmp(command, "sr") == 0) {
            // Show routing information
            if(connected){
                // Parse destination node ID
                if (sscanf(str + offset, "%s", arg1) == 1){
                    // Display routing information for the specified destination
                    ShowRouting (e, atoi(arg1), n->selfID);
                } else printf("\n Invalid interface command!\n\n");
            } else printf("\n Not connected...\n\n");
        }
        // SHOW PATH [dest]
        else if (strcmp(command, "sp") == 0) {
            // Show path information
            if(connected){
                // Parse destination node ID
                if (sscanf(str + offset, "%s", arg1) == 1){
                    // Display path information for the specified destination
                    if (strcmp(arg1, "all") == 0) ShowAllPaths (e, n->selfID);
                    else ShowPath (e, atoi(arg1), n->selfID);
                } else printf("\n Invalid interface command!\n\n");
            } else printf("\n Not connected...\n\n");  
        }
        // SHOW FORWARDING
        else if (strcmp(command, "sf") == 0) {          
            if(connected){      
                // Show forwarding information
                Showforwarding (e, n->selfID); 
            } else printf("\n Not connected...\n\n"); 
        }
        // MESSAGE [dest] [message]
        else if (strcmp(command, "m") == 0) {
            if(connected){ 
                // Send message to destination node
                if (sscanf(str + 2, "%s", arg1) == 1){
                    // Read message
                    if (sscanf(str + 3 + strlen(arg1), "%[^\n]", message) != 1) return 0;
                    // Check if destination is self
                    if (strcmp (arg1, n->selfID) == 0) printf ("%s\n\n", message);
                    else {
                        // Format message and pass it to message handler
                        sprintf(buffer, "CHAT %s %s %s\n", n->selfID, arg1, message);
                        messageHANDLER(n, buffer);
                    } 
                    
                } else printf("\n Invalid interface command!\n\n");
            } else printf("\n Not connected...\n\n"); 
        }
        // LEAVE
        else if (strcmp(command, "l") == 0) {     
            if(connected){
                // Disconnect from nodes
                if(!(strcmp(ring, "---")==0) && unregisterInServer(regSERV, ring, n)){
                    strcpy(ring, "---");
                    connected = 0;
                }
                if(n->predSOCK != NULL && n->succSOCK != NULL){
                    removeFD(s, getFD_Socket(n->predSOCK)); removeFD(s, getFD_Socket(n->succSOCK));
                    closeSocket(n->succSOCK, 1); closeSocket(n->predSOCK, 1);
                    n->predSOCK = NULL; n->succSOCK = NULL;
                    if (strcmp (n->chordID, "") != 0){
                        removeFD(s, getFD_Socket(n->chordSOCK)); closeSocket(n->chordSOCK, 1);
                        n->chordSOCK = NULL;
                        strcpy (n->chordID, ""); strcpy (n->chordIP, "");  strcpy (n->chordTCP, "");
                    }
                    deleteALLChords(n->c, s);
                    n->c = NULL;
                    deleteEncaminhamento(e); 
                    connected = 0;
                }    
            }else printf("\n Not connected...\n\n");  
        }
        // EXIT
        else if (strcmp(command, "x") == 0) {          
            printf("\n Application Closing!\n\n");
            if(connected){
                // Disconnect from nodes
                if(!(strcmp(ring, "---")==0) && unregisterInServer(regSERV, ring, n)){
                    strcpy(ring, "---");
                }
                if(n->predSOCK != NULL && n->succSOCK != NULL){
                    removeFD(s, getFD_Socket(n->predSOCK)); removeFD(s, getFD_Socket(n->succSOCK));
                    closeSocket(n->succSOCK, 1); closeSocket(n->predSOCK, 1);
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
            } else printf("\n Not connected...\n\n");  
            return 1;             
        }
        // Clear
        else if(strcmp(command, "clear") == 0){
            if(sscanf(str+6, "%s", arg1)){
                // Validate ring command
                if (strlen(arg1) == 3 && atoi(arg1) >= 0 && atoi(arg1) <= 99){
                    int attempts = 0;
                    char buffer[32];
                    Select *udp_t = newSelect();
                    addFD(udp_t, getFD_Socket(regSERV));

                    // Send reset command to the server
                    sprintf(buffer, "RST %s", arg1);
                    while(attempts < UDP_ATTEMPTS){
                        if(listenSelect(udp_t, UDP_TIME_OUT)>0){
                            Recieve(regSERV, buffer);
                            printf("\n %s cleared!\n\n", arg1);
                            break;
                        }
                        else {
                            printf("\n Server unresponsive... Retrying (%d/%d)\n\n", (attempts++)+1, UDP_ATTEMPTS);
                            Send(regSERV, buffer);
                        }
                    }
                    freeSelect (udp_t); 
                } else printf ("\n Invalid ring command!\n\n");
            }
        }
        // Nodes
        else if(strcmp(command, "nodes") == 0){
            if(sscanf(str+6, "%s", arg1)){
                // Validate ring command
                if (strlen(arg1) == 3 && atoi(arg1) >= 0 && atoi(arg1) <= 99){
                    char *aux = getNodesServer(regSERV, arg1);
                    if (aux == NULL){
                        printf("\n Failed to get Nodes from server...\n\n");
                        return 0;
                    }
                    printf("\n %s\n\n", aux);
                    free(aux);
                } else printf ("\n Invalid ring command!\n\n");
            }
        }
        else {
            printf("\n Invalid interface command!\n\n");
        }
    }
    fflush(stdout);     // Flush the output buffer
    return 0;
}