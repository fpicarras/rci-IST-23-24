#include "sockets.h"
#include <stdio.h>

int main(){
    char buffer[1024];
    int i = 0;
    char IP[16], TCP[6], ID[3];
    Socket *new = TCPserverSocket("8123", 15);
    Socket *conn, *pred;
    if(new == NULL) return 1;

    while(i < 10){
        /* Apos 1/2 conexç~oes ele morre aqui: Error accepting connections... */
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
    return 0;
}