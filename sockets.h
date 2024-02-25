//Libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <errno.h>

#define BUFFER_SIZE 1024

typedef struct _socket Socket;
typedef struct _tcp_server_connection TCPserverConn;

//CLIENT SIDED

/**
 * @brief Creates a UDP socket (attempts to connecto to node on port "service").
 * 
 * UDP sockets are faster but do not garantee package reception or order.
 * 
 * @param node String with the node/IP to connect to.
 * @param service String with service/port to connect to.
 * @return Socket* -> NULL if something goes wrong, e.g. can't connect.
 */
Socket *UDPSocket(char *node, char *service);

/**
 * @brief Creates a TCP socket (attempts to connecto to node on port "service").
 * 
 * TCP sockets are "slower" but garantee package reception.
 * 
 * @param node String with the node/IP to connect to.
 * @param service String with service/port to connect to. 
 * @return Socket* -> NULL if something goes wrong, e.g. can't connect.
 */
Socket *TCPSocket(char *node, char *service);

/**
 * @brief Sends a message to a given UDP/TCP socket.
 * 
 * @param sock Socket* to send message to.
 * @param message String with message, max lenght should be BUFFER_SIZE.
 * @return int -> 1 if succecefuly sent, 0 otherwise.
 */
int Send(Socket *sock, char *message);

/**
 * @brief Waits for a message to be recieved from a UDP/TCP socket.
 * 
 * @warning If nothing is sent from the otherside, the code will be blocked until it recieves something.
 * 
 * @todo Make timeout system...
 * 
 * @param sock Socket to wait for message.
 * @param buffer String to write recieving message to.
 */
void Recieve(Socket *sock, char *buffer);

/**
 * @brief Closes a socket and frees the memory allocated for the structure.
 * 
 * @param sock Socket* to close and free.
 */
void closeSocket(Socket *sock);

/**
 * @brief Get the file descriptor of a Socket.
 * 
 * @param sock Pointer to socket
 * @return int File Descriptor relative to given Socket
 */
int getFD_Socket(Socket *sock);

//SERVED SIDED

/**
 * @brief Creates a listening UDP socket on the local machine (0.0.0.0) on port "service".
 * 
 * @param service String with port to listen to.
 * @return Socket* -> NULL if something goes wrong.
 */
Socket *UDPserverSocket(char *service);

/**
 * @brief Wait for a message to be recieved on the given Socket.
 * 
 * @param sock Socket* to listen to.
 * @param addr sockaddr* to write sender address.
 * @param buffer String to write recieved message from sender, max lenght is BUFFER_SIZE
 */
void UDPserverRecieve(Socket *sock, struct sockaddr *addr, char *buffer);

/**
 * @brief Send a message to an address on a socket
 * 
 * @param sock Socket to send from.
 * @param addr sockaddr to send message to.
 * @param message String with message to send, max lenght is BUFFER_SIZE
 * @return int -> 1 if succecefuly sent, 0 otherwise.
 */
int UDPserverSend(Socket *sock, struct sockaddr *addr, char *message);

/**
 * @brief Creates a listening TCP socket on the local machine (0.0.0.0) on port "service".
 * 
 * @param service String with port to listen to.
 * @param queue_size Number of machines that can be at the TCP connection queue at the same time, more than this
 * and they won't be able to connect.
 * @return Socket* -> NULL if something goes wrong.
 */
Socket *TCPserverSocket(char *service, int queue_size);

/**
 * @brief Accepts an incoming connection from a machine, creates a new socket to communicate
 * with said machine.
 * 
 * @warning This is a possible point for the program to block, since it will wait here for new connections.
 * 
 * @param sock Recieving Socket*
 * @return TCPserverConn* -> New connection (socket), NULL in case of error.
 */
TCPserverConn *TCPserverAccept(Socket *sock);

/**
 * @brief Closes a connection with another machine.
 * 
 * @param conn Connection to close.
 */
void TCPserverCloseConnection(TCPserverConn *conn);

/**
 * @brief 
 * 
 * @param conn Connesction to send message to.
 * @param message Message to send, max size should be BUFFER_SIZE.
 * @return int -> 1 if succecefuly sent, 0 otherwise.
 */
int TCPserverSend(TCPserverConn *conn, char *message);

/**
 * @brief Waits for a message to be sent to this machine.
 * 
 * @warning The code will be blocked here waiting for new messages from said connection.
 * 
 * @param conn Connection to wait for the message.
 * @param buffer String to write recieved message.
 */
void TCPserverRecieve(TCPserverConn *conn, char *buffer);

/**
 * @brief Get the File Descriptor of a TCP connection (server-sided)
 * 
 * @param conn Pointer to TCP client connection
 * @return int File Descriptor relative to given connection;
 */
int getFD_TCPConn(TCPserverConn *conn);