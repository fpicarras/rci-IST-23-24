#include "sockets.h"

extern int errno; // Declaration of errno variable from the standard C library.

typedef struct _socket{
    int fd; // File descriptor for the socket.
    int type; // Type of the socket: 1 for TCP, 2 for UDP.

    struct addrinfo *res; // Pointer to address information.
    char *port; // Port number as a string.
    socklen_t addrlen; // Length of socket address.
    struct sockaddr addr; // Socket address structure.
} Socket;

// Function to determine the type of socket.
int typeSocket(Socket *sock){
    int type = sock->res->ai_socktype; // Extracting the socket type from address information.
    return type;
}

// Function to verify IPv4 address
int check_ipv4 (char *IP){
    struct in_addr dest;
    
    if (inet_pton(AF_INET, IP, &dest) == 0){
        printf ("\n %s is not a valid IPv4 address!\n\n", IP);
        return 1;
    }
    return 0;
}

// Function to initialize a generic socket structure.
Socket *initSocket(int domain, int type, int protocol, int flags, char *node, char *service){
    struct addrinfo hints, *res;
    
    int errcode;

    memset(&hints, 0, sizeof hints); // Clearing the hints structure.
    hints.ai_family = domain; // Setting the address family.
    hints.ai_socktype = type; // Setting the socket type.
    hints.ai_flags = flags; // Setting additional flags.

    // Obtaining address information based on node (hostname/IP) and service (port).
    if((errcode = getaddrinfo(node, service, &hints, &res)) != 0){
        fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(errcode));
        return NULL;
    }   

    // Allocating memory for a new Socket structure.
    Socket *new_sock = (Socket*)calloc(1, sizeof(Socket));
    
    // Assigning obtained address information to the new socket.
    new_sock->res = res;
    new_sock->fd = socket(domain, type, protocol); // Creating a socket.

    // Storing the port number.
    if(service != NULL){
        new_sock->port = (char*)malloc((strlen(service) + 1) * sizeof(char));
        strcpy(new_sock->port, service);
    } else {
        new_sock->port = NULL;
    }

    new_sock->type = typeSocket(new_sock); // Determining the type of the socket.
    return new_sock;
}

// Function to close a socket and free allocated memory.
void closeSocket(Socket *sock, int close_fd){
    if(sock->res != NULL) freeaddrinfo(sock->res); // Freeing address information.
    if(sock->port != NULL) free(sock->port); // Freeing the port string.
    if(close_fd) close(sock->fd); // Closing the socket file descriptor.

    free(sock); // Freeing the socket structure itself.
}

// Function to retrieve and optionally store the hostname.
void hostName(char *s){
    char buffer[BUFFER_SIZE];
    if(gethostname(buffer, BUFFER_SIZE) == -1)
        fprintf(stderr, "Error: %s\n", strerror(errno));
    else 
        printf("\n Host name: %s\n\n", buffer);

    if(s != NULL) sprintf(s, "%s", buffer); // Storing hostname if provided.
}

// Function to print the address of a socket.
char *getAddress(Socket* sock){
    char *buffer = (char*)malloc(INET_ADDRSTRLEN*sizeof(char));
    struct in_addr *addr;
    struct addrinfo *p;

    if(sock->res == NULL){
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)(&(sock->addr));
        inet_ntop(AF_INET, &(ipv4->sin_addr), buffer, INET_ADDRSTRLEN);
        return buffer;
    }

    for(p = sock->res; p != NULL; p = p->ai_next){
        struct sockaddr_in *ip = (struct sockaddr_in *)p->ai_addr;
        addr = &(ip->sin_addr);
        printf("\n %s\n\n", inet_ntop(p->ai_family, addr, buffer, sizeof(buffer)));
    }
    free(buffer);
    return NULL;
}

/* CLIENT SIDE CODE */
// Function to create a simple UDP socket structure.
Socket *UDPSocket(char *node, char *service){
    return initSocket(AF_INET, SOCK_DGRAM, 0, AI_CANONNAME, node, service);
}

// Function to create a simple TCP socket structure.
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
        printf("\n Error connecting to %s...\n\n", new->res->ai_canonname);
        closeSocket(new, 1);
        return NULL;
    }
    return new;
}

// Function to send data through a socket.
int Send(Socket *sock, char *message){
    char *ptr, *buffer = (char*)calloc(BUFFER_SIZE, sizeof(char));
    //Ensures that any string can enter the buffer, if size > BUFFER_SIZE, string won't be fully sent
    strcpy(buffer, message);

    //Is UDP
    if(sock->type == 2){
        int n = sendto(sock->fd, buffer, BUFFER_SIZE, 0, sock->res->ai_addr, sock->res->ai_addrlen);
        if(n == -1){
            printf("[!] Error Sending message: %s...\n\n", message);
            free(buffer);
            return 0;
        }
    }else if(sock->type == 1){
        //Is TCP
        int nleft = strlen(message), nwritten;
        ptr = buffer;
        while(nleft>0){
            nwritten = write(sock->fd, ptr, nleft);
            if(nwritten < 0){
                printf("[!] Error sending message: %s...\n\n", message);
                free(buffer);
                return 0;
            }
            nleft -= nwritten; ptr += nwritten;
        }
    }

    free(buffer);
    return 1;
}

// Function to receive data through a socket.
int Recieve(Socket *sock, char *buffer){
    int errcode;
    char host[NI_MAXHOST], service[NI_MAXSERV];

    if(sock->type == 2){
        //Is UDP
        struct sockaddr addr;
        socklen_t addrlen = sizeof(addr);

        int n = recvfrom(sock->fd, buffer, BUFFER_SIZE, 0, &addr, &addrlen);

        if(n == -1){
            printf("[!] Error receiving...\n\n");
            return -1;
        }
        buffer[n] = '\0';

        if((errcode=getnameinfo(&addr,addrlen,host,sizeof(host),service,sizeof(service),0))!=0)
            fprintf(stderr,"error: getnameinfo: %s\n",gai_strerror(errcode));

        return n;
    }else if(sock->type == 1){
        //Is TCP
        char *ptr = buffer;
        int nread = 0;
        while((nread = read(sock->fd, ptr, 1))>0){
            if(ptr[0]=='\n') break;
            ptr += nread;
        }
        ptr[1] = '\0';
        if(nread == -1) printf("[!] Error receiving...\n\n");
        return nread;
    }
    return -1;
}

// Function to get the file descriptor of a socket.
int getFD_Socket(Socket *sock){
    return sock->fd;
}

/* SERVER SIDE CODE */
// Function to create a UDP server socket.
Socket *UDPserverSocket(char *service){
    Socket *new = initSocket(AF_INET, SOCK_DGRAM, 0, AI_PASSIVE, NULL, service);

    if(bind(new->fd, new->res->ai_addr, new->res->ai_addrlen)==-1){
        printf("\n Error binding port!\n\n"); 
        closeSocket(new, 1);
        return NULL;
    }
    return new;
}

// Function to receive data on a UDP server socket.
void UDPserverRecieve(Socket *sock, struct sockaddr *addr, char *buffer){
    socklen_t addrlen = sizeof(*addr);
    if(recvfrom(sock->fd, buffer, BUFFER_SIZE, 0, addr, &addrlen)==-1)
        printf("\n Error receiving...\n\n");
}

// Function to send data from a UDP server socket.
int UDPserverSend(Socket *sock, struct sockaddr *addr, char *message){
    char *buffer = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    strcpy(buffer, message);
    socklen_t addrlen = sizeof(*addr);
    if(sendto(sock->fd, buffer, BUFFER_SIZE, 0, addr, addrlen)==-1){
        printf("\n Error sending...\n\n");
        return 0;
    }

    free(buffer);
    return 1;
}

// Function to create a TCP server socket.
Socket *TCPserverSocket(char *service, int queue_size){
    /* Signal Handling */
    //If connection is lost, any writes return -1, instead of killing the process
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &act, NULL)==-1) exit(1);

    Socket *new = initSocket(AF_INET, SOCK_STREAM, 0, AI_PASSIVE, NULL, service);
    if(bind(new->fd, new->res->ai_addr, new->res->ai_addrlen)==-1){
        printf("\n Error binding port!\n\n");
        closeSocket(new, 1);
        return NULL;
    }
    if(listen(new->fd, queue_size)==-1){
        printf("\n Error on commanding Kernel to accept incoming connections...\n\n");
        closeSocket(new, 1);
        return NULL;
    }
    return new;
}

// Function to accept a connection on a TCP server socket.
Socket *TCPserverAccept(Socket *sock){
    int newfd;
    struct sockaddr addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    Socket *new = (Socket*)calloc(1, sizeof(Socket));
    if((newfd=accept(sock->fd, &addr, &addrlen))==-1){
        printf("\n [FD: %d] Error accepting connections...\n\n", sock->fd);
        return NULL;
    }

    new->addr = addr; new->addrlen = addrlen; new->fd = newfd;
    new->type = 1;

    return new;
}