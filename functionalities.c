#include "functionalities.h"

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
    Send(server, buffer);
    Recieve(server, buffer);

    if(strcmp(buffer, "OKREG")==0){
        printf("Registred as %s in ring %s!\n\n", n->selfID, ring);
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
        printf("Leaving ring %s...\n\n", ring);
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

void handleSuccDisconnect(Nodes *n, Select *s, Encaminhamento* e){
    char buffer[BUFFER_SIZE];
    Socket *new;
    removeAdj (e, n->succID);
    removeFD(s, getFD_Socket(n->succSOCK)); closeSocket(n->succSOCK, 1);
    n->succSOCK = NULL;
    
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
}

void handlePredDisconnect(Nodes *n, Select *s, Encaminhamento* e){
    removeAdj (e, n->predID);
    removeFD(s, getFD_Socket(n->predSOCK)); closeSocket(n->predSOCK, 1);
    n->predSOCK = NULL;
}

void handleSuccCommands(Nodes *n, Select *s, char *msg){
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
            closeSocket(n->succSOCK, 1);

            //Set new node to succ
            strcpy(n->succID, auxID); strcpy(n->succIP, auxIP); strcpy(n->succTCP, auxTCP);
            sprintf(buffer, "PRED %s\n", n->selfID);
            new = TCPSocket(auxIP, auxTCP);
            addFD(s, getFD_Socket(new)); n->succSOCK = new;
            Send(new, buffer);
        }else if(strcmp(command, "SUCC")==0){
            sscanf(msg, "SUCC %s %s %s\n", n->ssuccID, n->ssuccIP, n->ssuccTCP);
        }
    }
}

void handlePredCommands(Nodes *n, Select *s, char *msg){
    /**/
}

void handleNewConnection(Nodes *n, Select *s, Socket *new, char *msg){
    char buffer[BUFFER_SIZE], command[16];

    if(sscanf(msg, "%s", command)){
        if(strcmp(command, "ENTRY")==0) handleENTRY(n, new, s, msg);
        if(strcmp(command, "PRED")==0){
            sscanf(msg, "PRED %s\n", n->predID);
            n->predSOCK = new; addFD(s, getFD_Socket(new));
            sprintf(buffer, "SUCC %s %s %s\n", n->succID, n->succIP, n->succTCP);
            Send(new, buffer);
        }
    }
}

int consoleInput(Socket *regSERV, Nodes *n, Select *s, Encaminhamento *e){
    char str[256], command[8], arg1[8], arg2[16], arg3[16], arg4[16], message[128], buffer[256], path_buffer[128];
    int offset = 0, i;
    static int connected = 0, n_node = 0;
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
            if(connected){      
                /****/   
            } else printf("Not connected...\n\n");
        }
        // REMOVE CHORD
        else if (strcmp(command, "rc") == 0) {          
            if(connected){      
                /****/   
            } else printf("Not connected...\n\n");            
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
            if(connected){
                if (sscanf(str + offset, "%s", arg1) == 1){
                    ShowRouting (atoi(arg1), e->routing);
                } else printf("Invalid interface command!\n");
            } else printf("Not connected...\n\n");
        }
        // SHOW PATH [dest]
        else if (strcmp(command, "sp") == 0) {
            if(connected){
                if (sscanf(str + offset, "%s", arg1) == 1){
                    ShowPath (atoi(arg1), e->shorter_path);
                } else printf("Invalid interface command!\n");
            } else printf("Not connected...\n\n");  
        }
        // SHOW FORWARDING
        else if (strcmp(command, "sf") == 0) {          
            if(connected){      
                ShowFowarding (e->fowarding); 
            } else printf("Not connected...\n\n"); 
        }
        // MESSAGE [dest] [message]
        else if (strcmp(command, "m") == 0) {
            if(connected){ 
                if (sscanf(str + 2, "%s", arg1) == 1){
                    if (sscanf(str + 5, "%[^\n]", message) != 1) exit (0);
                    if (strcmp (arg1, n->selfID) == 0) printf ("%s\n\n", message);
                    else {
                        sprintf(buffer, "CHAT %s %s %s\n", n->selfID, arg1, message);
                        Send(n->succSOCK, buffer);  /* Alterar para seguir para o nó da tabela de expedição no indice do destino !!! */
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
                }
                connected = 0;
            } else printf("Not connected...\n\n");            
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