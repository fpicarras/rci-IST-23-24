#include "sockets.h"
#include "select.h"

#ifndef FOWA_H_
#define FOWA_H_

typedef struct _encaminhamento
{
    char ***routing;
    char **shorter_path;
    char **forwarding;
} Encaminhamento;

/**
 * @brief Creates an encaminhamento struct. Fills the entries related to our
 * own Node, using selfID
 * 
 * @param selfID String with our ID
 * 
 * @return Encaminhamento* struct
 */
Encaminhamento *initEncaminhamento(char* selfID);

/**
 * @brief Frees the mem allocated for e
 * 
 * @param e struct to free
 */
void deleteEncaminhamento(Encaminhamento* e);

/**
 * @brief Prints the forwarding table
 * 
 * @param e Encaminhamento struct
 */
void Showforwarding(Encaminhamento *e);

/**
 * @brief Prints the shortest path to n_dest
 * 
 * @param e Encaminhamento struct
 * @param n_dest Destination Node
 */
void ShowPath(Encaminhamento *e, int n_dest);

/**
 * @brief Prints the routing table to n_dest
 * 
 * @param e Encaminhamento struct
 * @param n_dest Destination node
 */
void ShowRouting(Encaminhamento *e, int n_dest);

/**
 * @brief Given a path (format: N1-N2-N3) adds it to the routing data structures if it is valid.
 * 
 * If our shortest_path and forwarding tables are changed, the value 1 will be returned, meaning the program should
 * tell the other nodes that it's new shortest path to Dest.
 * 
 * If path is parsed as NULL, this will be interpreted as the connection from origin to dest no loger being possible.
 * 
 * @param e Encaminhamento struct
 * @param self String with our ID
 * @param origin ID where the routing message originated
 * @param dest ID of the destination whose path is being told to us
 * @param path shortest path from Origin to Dest
 * @return int 1 if forwarding and shortest paths to dest were changed, 0 otherwise
 */
int addPath(Encaminhamento *e, char *self, char *origin, char *dest, char *path);

/**
 * @brief Removes an entire collum from the table, returns an array with all the idxs of shortest paths changed.
 * This array is terminated with -1, however if it is NULL means that no shortest path was changed
 * 
 * @param e Encaminhamento struct
 * @param node String with the node to romove paths from
 * @return int* NULL if nothing was changed or an array os ints terminated in -1 if something was
 */
int *removeAdj(Encaminhamento *e, char* node);

#endif