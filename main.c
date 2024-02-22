#include "sockets.h"

#include <sys/select.h>

#define REGIP "193.136.138.142"
#define REGUDP "59000"

typedef struct _node{
    int id;
    char *IP;
    char *port;
}Node;

Node p, s, ss, self;

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

int main(int argc, char *argv[]){
    char IP[16], TCP[6];
    char regIP[16] = REGIP, regUDP[6] = REGUDP;

    if(validateArguments(argc, argv, IP, TCP, regIP, regUDP)) return 0;
    
    return 0;
}