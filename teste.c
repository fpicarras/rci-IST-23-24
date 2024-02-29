#include "sockets.h"
#include <stdio.h>

#define TESTE "NODESLIST 001\n01 127.5.0.123 12345\n17 0.0.0.0 12345\n96 127.0.0.1 54321\n"

int isNodeInServer(char *nodeslist, char *selfID){
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
    return 1;
}

int main(){
    char buffer[1024];
    int i = 0;
    char IP[16], TCP[6], ID[3];
    printf("%d\n", isNodeInServer(TESTE, "96"));

    /*
    Socket *new = TCPserverSocket("8123", 15);
    Socket *conn, *pred;
    if(new == NULL) return 1;

    while(i < 10){
        conn = TCPserverAccept(new);
        Recieve(conn, buffer);
        sscanf(buffer, "ENTRY %s %s %s", ID, IP, TCP);
        printf("Node %s [%s:%s] is connecting...\n", ID, IP, TCP);
        
        Send(conn, "SUCC 17 192.168.0.17 1234");
        pred = TCPSocket(IP, TCP);
        Send(pred, "PRED 80\n");

        closeSocket(conn); closeSocket(pred);
        i++;
    }
    
    closeSocket(new);
    */
    return 0;
}