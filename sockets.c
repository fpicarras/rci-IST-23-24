#include "sockets.h"

extern int errno;

typedef struct _socket{
    int fd;
    int type;

    struct addrinfo *res;
    char *port;
    socklen_t addrlen;
    struct sockaddr addr;
}Socket;

/**
 * Returns type of Socket:
 * 
 * 1 - TCP
 * 2 - UDP 
 */
int typeSocket(Socket *sock){
    int type = sock->res->ai_socktype;
    //printf("Type: %d\n", type);
    return type;
}

/*Creates a generic socket structure*/
Socket *initSocket(int domain, int type, int protocol, int flags, char *node, char *service){
    struct addrinfo hints, *res;
    
    int errcode;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = domain;
    hints.ai_socktype = type;
    hints.ai_flags = flags;

    if((errcode=getaddrinfo(node,service,&hints,&res))!=0){
        fprintf(stderr,"Error: getaddrinfo: %s\n",gai_strerror(errcode));
        return NULL;
    }   

    Socket *new_sock = (Socket*)calloc(1, sizeof(Socket));
    
    new_sock->res = res;
    new_sock->fd = socket(domain, type, protocol);

    if(service != NULL){
        new_sock->port = (char*)malloc((strlen(service)+1)*sizeof(char));
        strcpy(new_sock->port, service);
    }else
        new_sock->port = NULL;

    new_sock->type = typeSocket(new_sock);
    return new_sock;
}

/*Closes a socket*/
void closeSocket(Socket *sock, int close_fd){
    if(sock->res != NULL) freeaddrinfo(sock->res);
    if(sock->port != NULL) free(sock->port);
    if(close_fd) close(sock->fd);

    free(sock);
}

/*Places the hostname string in s, also prints it*/
void hostName(char *s){
    char buffer[BUFFER_SIZE];
    if(gethostname(buffer, BUFFER_SIZE)==-1)
        fprintf(stderr, "Error: %s\n", strerror(errno));
    else printf("Host name: %s\n", buffer);

    if(s != NULL) sprintf(s, "%s", buffer);

    return;
}

/*Prints the Address of a socket*/
/**
 * @todo return the string of ip... maybe??
 */
void getAdress(Socket* sock){
    char buffer[INET_ADDRSTRLEN];
    struct in_addr *addr;
    struct addrinfo *p;

    //printf("canonical hostname: %s\n",sock->res->ai_canonname);
    for(p=sock->res;p!=NULL;p=p->ai_next){
        struct sockaddr_in *ip = (struct sockaddr_in *)p->ai_addr;
        addr = &(ip->sin_addr);
        printf("%s\n", inet_ntop(p->ai_family, addr, buffer, sizeof(buffer)));
    }
}

/*Return a simple UDP socket structure*/
Socket *UDPSocket(char *node, char *service){
    return initSocket(AF_INET, SOCK_DGRAM, 0, AI_CANONNAME, node, service);
}

Socket *TCPSocket(char *node, char *service){
    int n;
    Socket *new = initSocket(AF_INET, SOCK_STREAM, 0, AI_CANONNAME, node, service);

    /* Signal Handling */
    //If connection is lost, any writes return -1, instead of killing the process
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &act, NULL)==-1) exit(1);

    n = connect(new->fd, new->res->ai_addr, new->res->ai_addrlen);
    if(n == -1){
        printf("Error connecting to %s...\n", new->res->ai_canonname);
        closeSocket(new, 1);
        return NULL;
    }
    return new;
}

int Send(Socket *sock, char *message){
    char *ptr, *buffer = (char*)calloc(BUFFER_SIZE, sizeof(char));
    //Ensures that any string can enter the buffer, if size > BUFFER_SIZE, string won't be fully sent
    strcpy(buffer, message);

    //Is UDP
    if(sock->type == 2){
        int n = sendto(sock->fd, buffer, BUFFER_SIZE, 0, sock->res->ai_addr, sock->res->ai_addrlen);
        if(n == -1){
            printf("Error Sending message: %s...\n", message);
            return 0;
        }
    }else if(sock->type == 1){
        //Is TCP
        int nleft = BUFFER_SIZE, nwritten;
        ptr = buffer;
        while(nleft>0){
            nwritten = write(sock->fd, ptr, nleft);
            if(nwritten < 0){
                printf("Error sending message: %s\n", message);
                return 0;
            }
            nleft -= nwritten; ptr += nwritten;
        }
    }

    free(buffer);
    return 1;
}

/*VER A QUESTÃO DOS TIME-OUTS*/
int Recieve(Socket *sock, char *buffer){
    int errcode;
    char host[NI_MAXHOST], service[NI_MAXSERV];

    if(sock->type == 2){
        //Is UDP
        struct sockaddr addr;
        socklen_t addrlen = sizeof(addr);

        int n = recvfrom(sock->fd, buffer, BUFFER_SIZE, 0, &addr, &addrlen);

        if(n == -1){
            printf("Error recieving...\n");
            return -1;
        }
        buffer[n] = '\0';

        if((errcode=getnameinfo(&addr,addrlen,host,sizeof(host),service,sizeof(service),0))!=0)
            fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode));
        else
            //printf("Recieved from: [%s:%s]\n",host,service);
        return n;
    }else if(sock->type == 1){
        //Is TCP
        char *ptr = buffer;
        int nleft = BUFFER_SIZE, nread;
        while(nleft > 0){
            nread = read(sock->fd, ptr, nleft);
            if(nread <= 0) break;
            nleft -= nread;
            ptr += nread;
        }
        return nread;
    }
    return -1;
}

int getFD_Socket(Socket *sock){
    return sock->fd;
}

/**
 * SERVER SIDE CODE
 */

Socket *UDPserverSocket(char *service){
    Socket *new = initSocket(AF_INET, SOCK_DGRAM, 0, AI_PASSIVE, NULL, service);

    if(bind(new->fd, new->res->ai_addr, new->res->ai_addrlen)==-1){
        printf("Error binding port!\n"); 
        closeSocket(new, 1);
        return NULL;
    }
    return new;
}

void UDPserverRecieve(Socket *sock, struct sockaddr *addr, char *buffer){
    socklen_t addrlen = sizeof(*addr);
    if(recvfrom(sock->fd, buffer, BUFFER_SIZE, 0, addr, &addrlen)==-1)
        printf("Error recieving...\n");
}

int UDPserverSend(Socket *sock, struct sockaddr *addr, char *message){
    char *buffer = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    strcpy(buffer, message);
    socklen_t addrlen = sizeof(*addr);
    if(sendto(sock->fd, buffer, BUFFER_SIZE, 0, addr, addrlen)==-1){
        printf("Error sending...\n");
        return 0;
    }

    free(buffer);
    return 1;
}

Socket *TCPserverSocket(char *service, int queue_size){
    /* Signal Handling */
    //If connection is lost, any writes return -1, instead of killing the process
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &act, NULL)==-1) exit(1);

    Socket *new = initSocket(AF_INET, SOCK_STREAM, 0, AI_PASSIVE, NULL, service);
    if(bind(new->fd, new->res->ai_addr, new->res->ai_addrlen)==-1){
        printf("Error binding port!\n");
        closeSocket(new, 1);
        return NULL;
    }
    if(listen(new->fd, queue_size)==-1){
        printf("Error on commanding Kernel to accept incomming connections...\n");
        closeSocket(new, 1);
        return NULL;
    }
    return new;
}

Socket *TCPserverAccept(Socket *sock){
    int newfd;
    struct sockaddr addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    Socket *new = (Socket*)calloc(1, sizeof(Socket));
    if((newfd=accept(sock->fd, &addr, &addrlen))==-1){
        printf("[FD: %d] Error accepting connections...\n", sock->fd);
        return NULL;
    }

    new->addr = addr; new->addrlen = addrlen; new->fd = newfd;
    new->type = 1;

    return new;
}
/*
int TCPserverSend(Socket *conn, char *message){
    int nleft = BUFFER_SIZE, nwritten;
    char *ptr, *buffer = (char*)calloc(BUFFER_SIZE, sizeof(char));
    strcpy(buffer, message);
    ptr = buffer;
    while(nleft>0){
        nwritten = write(conn->fd, ptr, nleft);
        if(nwritten < 0){
            printf("Error sending message: %s\n", message);
            return 0;
        }
        nleft -= nwritten; ptr += nwritten;
    }
    return 1;
}

void TCPserverRecieve(Socket *conn, char *buffer){
    char *ptr = buffer;
    int nleft = BUFFER_SIZE, nread;
    while(nleft > 0){
        nread = read(conn->fd, ptr, nleft);
        if(nread <= 0) break;
        nleft -= nread;
        ptr += nread;
    }
}
*/