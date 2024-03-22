#include "forwarding.h" 
#include "sockets.h"
#include "select.h"

#ifndef FUNC_H_
#define FUNC_H_

#define TIME_OUT 15
#define UDP_TIME_OUT 2
#define UDP_ATTEMPTS 3

static volatile int loop = 1;

typedef struct _chords_struct {
    Socket *s;            // Pointer to a socket
    char ID[4];           // ID of the chord (assuming it's a string of length 3 plus '\0')
    struct _chords_struct *next;  // Pointer to the next Chord structure in the linked list
} Chord;

typedef struct _nodes{
    // Self node information
    char selfID[3];
    char selfIP[16];
    char selfTCP[6];
    Socket *selfSOCK;

    // Successor node information
    char succID[3];
    char succIP[16];
    char succTCP[6];
    Socket *succSOCK;

    // Second successor node information
    char ssuccID[3];
    char ssuccIP[16];
    char ssuccTCP[6];

    // Predecessor node information
    char predID[3];
    Socket *predSOCK;

    // Chord information
    char chordID[3];
    char chordIP[16];
    char chordTCP[6];
    Socket *chordSOCK;

    // Possible predecessor information
    char possible_predID[3];
    Socket *possible_predSOCK;

    // Pointer to chord structure
    Chord *c;

} Nodes; // Structure definition ends here


/**
 * @brief Validates the lauch arguments that the user inserted, the user is free to use regIP and regUDP or not.
 * If there is an invalid use of arguments the function will print the correct usage of the program
 * 
 * @param argc number of arguments
 * @param argv Array of strings with the arguments
 * @param IP String to put chosen IP
 * @param TCP String to put chosen TCP port
 * @param regIP String to put chosen nodes server IP
 * @param regUDP String to put chosen server UDP port
 * @return int 1 if something is wrong, 0 otherwise
 */
int validateArguments(int argc, char **argv, char *IP, char *TCP, char *regIP, char *regUDP);

/**
 * @brief Function to handle the connections coming from the listening TCP port.
 * This routine is responsible for processing new nodes connecting to us, new pred connections and also new chords.
 * 
 * @param n Nodes struct with our neighbours information.
 * @param s Select struct with the file descriptors to listen to.
 * @param new Socket* with the new connection.
 * @param msg String with the message sent from the new connection.
 */
void handleNewConnection(Nodes *n, Select *s, Chord **c_head, Socket *new, char *msg);

/**
 * @brief Function to handle commands from the succ.
 * 
 * @param n Nodes struct with our neighbours information.
 * @param s Select struct with the file descriptors to listen to.
 * @param msg String with the message sent from our succ.
 */
void handleSuccCommands(Nodes *n, Select *s, char *msg);

/**
 * @brief Function to run the routine when our succ disconnects.
 * 
 * @param n Nodes struct with our neighbours information.
 * @param s Select struct with the file descriptors to listen to.
 * @param e 
 */
void handleSuccDisconnect(Nodes *n, Select *s);

/**
 * @brief Function to handle commands from the pred.
 * 
 * @param n Nodes struct with our neighbours information.
 * @param s Select struct with the file descriptors to listen to.
 * @param msg String with the message sent from our pred.
 */
void handlePredCommands(Nodes *n, Select *s, char *msg);

/**
 * @brief Function to run the routine when our pred disconnects.
 * 
 * @param n Nodes struct with our neighbours information.
 * @param s Select struct with the file descriptors to listen to.
 * @param e
 */
void handlePredDisconnect(Nodes *n, Select *s);

/**
 * @brief Handles the disconnection of a chord node by removing its socket from the select structure,
   updating the node's chord list, and handling routing information accordingly.
 * 
 * @param n Pointer to a struct Nodes representing the network node.
 * @param s Pointer to a struct Select representing the select structure.
 * @param c Pointer to a struct Chord representing the chord node to be disconnected.
 */
void handleChordsDisconnect(Nodes *n, Select *s, Chord* c);

/**
 * @brief Handles the disconnection of the node's own chord by removing its socket from the select structure,
   closing the socket, and updating the node's chord information.
 * 
 * @param n Pointer to a struct Nodes representing the network node.
 * @param s Pointer to a struct Select representing the select structure.
 */
void handleOurChordDisconnect(Nodes *n, Select *s);

/**
 * @brief Function to handle the inputs from stdin.
 * 
 * @param regSERV Socket struct connected to the Nodes server.
 * @param n Nodes struct with all our neightbours information (succ, pred, etc.).
 * @param s Select struct with the file descriptors to be read when using the select() command.
 * @return int 1 if the program can end, 0 otherwise.
 */
int consoleInput(Socket *regSERV, Nodes *n, Select *s);

#endif