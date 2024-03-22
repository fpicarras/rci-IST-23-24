#include "forwarding.h"

// Initializes forwarding tables for a node
Encaminhamento *initEncaminhamento(char *self) {
    int i, j, n_self = atoi(self);
    Encaminhamento *new = NULL;

    new = (Encaminhamento *)calloc(1, sizeof(Encaminhamento));

    // Initializes the routing table with placeholders
    new->routing = (char ***)calloc(101, sizeof(char **));
    for (i = 0; i < 101; i++) {
        new->routing[i] = (char **)calloc(100, sizeof(char *));
        for (j = 0; j < 100; j++) {
            new->routing[i][j] = (char *)calloc(100, sizeof(char));
            if (i > 0)
                strcpy(new->routing[i][j], "-"); // Placeholder for uninitialized paths
            else
                strcpy(new->routing[i][j], ""); // Empty string for the self node
        }
    }

    // Initializes the shorter_path and forwarding tables
    new->shorter_path = (char **)calloc(100, sizeof(char *));
    for (i = 0; i < 100; i++)
        new->shorter_path[i] = (char *)calloc(100, sizeof(char));
    new->forwarding = (char **)calloc(100, sizeof(char *));
    for (i = 0; i < 100; i++)
        new->forwarding[i] = (char *)calloc(4, sizeof(char)); // Assuming node IDs are 3 digits

    // Initializes the self node in the tables
    sprintf(new->shorter_path[n_self], "%d", n_self);
    sprintf(new->forwarding[n_self], "%d", n_self);
    new->routing[0][n_self][0] = '1'; // Marks the presence of the self node in the routing table

    return new;
}

// Deletes forwarding tables and frees memory
void deleteEncaminhamento(Encaminhamento *e) {
    int i, j;
    if(e == NULL) return;
    // Free memory allocated for routing table
    for (i = 0; i < 101; i++) {
        for (j = 0; j < 100; j++)
            free(e->routing[i][j]);
        free(e->routing[i]);
    }
    free(e->routing);

    // Free memory allocated for shorter_path and forwarding tables
    for (i = 0; i < 100; i++) {
        free(e->shorter_path[i]);
        free(e->forwarding[i]);
    }
    free(e->shorter_path);
    free(e->forwarding);

    // Free memory allocated for the structure itself
    free(e);
}

// Shows the forwarding table of a node
void Showforwarding(Encaminhamento *e, char* n) {
    int i, aux = 0;
    char **forwarding = e->forwarding;

    for (i = 0; i < 100; i++) {
        if (strcmp(forwarding[i], "") == 0) // Empty entry in forwarding table
            continue;
        if (aux == 0){
            printf("\n\n    %s FORWARDING TABLE\n\n", n);
            printf (" --------------------------\n");
            printf(" | Destination | Neighbor |\n");
            printf (" --------------------------\n");
        }
        aux++;
        printf(" |     %02d      |    %02d    |\n", atoi(forwarding[i]), i);
        printf (" --------------------------\n");
    }
    if (aux == 0) printf("\n\n %s is the only node in the ring!\n", n);
    printf("\n\n");
}

// Shows the shortest path from a node to a destination
void ShowPath(Encaminhamento *e, int dest, char* origin) {
    char **path = e->shorter_path;
    
    if (strcmp(path[dest], "") == 0) // No valid path exists for the destination
        printf("\n\n Invalid destination! Node %02d is not in the ring!\n", dest);
    else {
        printf("\n\n    %s SHORTEST PATH TO %02d\n\n", origin, dest);
        printf (" ------\n");
        printf(" | %02d |   %s\n", dest, path[dest]);
        printf (" ------\n");
    }
    printf("\n\n");
}

// Shows the all shortest path from a node to a destination
void ShowAllPaths(Encaminhamento *e, char* origin){
    char **path = e->shorter_path;
    int i, aux = 0, flag = 0;

    for (i = 0; i < 100; i++) {
        if (strcmp(path[i], "") == 0) // Empty entry in forwarding table
            continue;
        if (aux == 0){
            printf("\n\n    %s SHORTEST PATH TO ALL NODES\n\n", origin);
        }
        printf (" ------\n");
        printf(" | %02d |   %s\n", i, path[i]);
        flag = 1;
        aux++;
    }
    if (flag == 1) printf (" ------\n");
    if (aux == 0) printf("\n\n %s is the only node in the ring!\n", origin);
    printf("\n\n");
}

// Shows the routing table entry of a node relative to a destination
void ShowRouting(Encaminhamento *e, int dest, char *origin) {
    int i, aux = 0, flag = 0;
    char ***routing = e->routing;

    // Check if the destination node exists in the ring
    for (i = 0; i < 100; i++) {
        if (strcmp(routing[0][i], "") == 0) aux++;
    }

    if (aux == 100) {
        printf("\n\n Invalid destination! Node %02d is not in the ring!\n", dest);
    } else {
        if (dest == atoi(origin)) printf ("\n\n Current node!\n");
        else {
            // Display routing table entries for the destination node
            printf("\n\n    %s ROUTING TABLE TO %02d\n\n", origin, dest);   
            for (i = 0; i < 100; i++) {
                if (strcmp(routing[dest + 1][i], "-") == 0) continue;
                printf (" ------\n");
                printf(" | %02d |   %s\n", i, routing[dest + 1][i]);
                flag = 1;
            }
            if (flag == 1) printf (" ------\n");
        }
    }
    printf("\n\n");
}

// Checks if the given path is valid
int ValidPath(char *path, char *node) {
    char str[128];
    strcpy(str, path);
    char *token = strtok(str, "-");

    // Iterate through the path tokens and check if the node is present
    while (token != NULL) {
        if (strcmp(node, token) == 0)
            return 0; // Invalid path: node is present
        token = strtok(NULL, "-");
    }
    return 1; // Valid path: node is not present
}

// Function to calculate the size of a path
int pathSize(char *path) {
    int count = 0, sz = strlen(path);

    // If the path is a placeholder ('-'), return a large value (assumed infinity)
    if (strcmp("-", path) == 0) return 10000;

    // Iterate through the characters in the path
    for (int i = 0; i < sz; i++) {
        // If a dash ('-') is encountered, increment the count
        if (path[i] == '-') count++;
    }
    // Return the count, which represents the number of dashes in the path
    return count;
}


// Returns the index of the second shortest path in the routing table, -1 if none
int findSecondShortest(Encaminhamento *e, int node_leaving, int dest) {
    int aux = -1;

    // Iterate through routing table entries for the destination node
    for (int col = 0; col < 100; col++) {
        if (e->routing[dest + 1][col][0] != '-') {
            if (aux == -1) {
                aux = col;
            } else {
                // Compare path sizes and ensure it's not the same as the shortest path
                if (pathSize(e->routing[dest + 1][aux]) > pathSize(e->routing[dest + 1][col]) && \
                    (strcmp(e->routing[dest + 1][col], e->shorter_path[dest]) != 0)) {
                    aux = col;
                }
            }
        }
    }
    return aux; // Index of the second shortest path
}

// Adds a path to the tables and updates the shortest path if necessary
int addPath(Encaminhamento *e, char *self, char *origin, char *dest, char *path) {
    int n_origin = atoi(origin), n_dest = atoi(dest), n_self = atoi(self), aux;
    char old_path[64];

    if (path == NULL) {
        // Handle case where path is NULL (remove the path)
        if (strcmp(e->shorter_path[n_dest], e->routing[n_dest + 1][n_origin]) == 0) {
            strcpy(e->routing[n_dest + 1][n_origin], "-"); // Remove the path
            if ((aux = findSecondShortest(e, n_origin, n_dest)) == -1) {
                //printf("%s <- null\n", e->shorter_path[n_dest]); *)#
                strcpy(e->shorter_path[n_dest], "");
                strcpy(e->forwarding[n_dest], "");
                return 1; // No alternate path available
            } else {
                //printf("%s <- %s\n", e->shorter_path[n_dest], e->routing[n_dest + 1][aux]); *)#
                strcpy(e->shorter_path[n_dest], e->routing[n_dest + 1][aux]);
                sprintf(e->forwarding[n_dest], "%d", aux);
                return 1; // Alternate path set
            }
        }
        strcpy(e->routing[n_dest + 1][n_origin], "-"); // Remove the path
        return 0; // No change in shortest path
    }

    char aux_c[4];
    sprintf(aux_c, "%d", n_self);

    if (ValidPath(path, aux_c)) {
        // Add the path to the routing table
        e->routing[0][n_origin][0] = '1'; // Mark origin node presence in routing table

        strcpy(old_path, e->routing[n_dest + 1][n_origin]);
        sprintf(e->routing[n_dest + 1][n_origin], "%d-%s", n_self, path);

        if (e->shorter_path[n_dest][0] == '\0') {
            // Set new path as the shortest path
            //printf("%s <- ", e->shorter_path[n_dest]);  *)#
            sprintf(e->shorter_path[n_dest], "%d-%s", n_self, path);
            sprintf(e->forwarding[n_dest], "%d", n_origin);
            //printf("%s\n", e->shorter_path[n_dest]);
            return 1; // New path added
        } else {
            if (strcmp(old_path, e->shorter_path[n_dest]) == 0) {
                // Update shortest path if necessary
                aux = findSecondShortest(e, n_origin, n_dest);
                //printf("%s <- %s\n", e->shorter_path[n_dest], e->routing[n_dest + 1][aux]);  *)#
                strcpy(e->shorter_path[n_dest], e->routing[n_dest + 1][aux]);
                sprintf(e->forwarding[n_dest], "%d", aux);
                return 1; // Shortest path updated
            } else if (pathSize(e->shorter_path[n_dest]) <= pathSize(e->routing[n_dest + 1][n_origin])) {
                return 0; // No change in shortest path
            } else {
                // Set new path as the shortest path
                //printf("%s <-", e->shorter_path[n_dest]);  *)#
                sprintf(e->shorter_path[n_dest], "%d-%s", n_self, path);
                sprintf(e->forwarding[n_dest], "%d", n_origin);
                //printf(" %s\n", e->shorter_path[n_dest]);  *)#
                return 1; // New path added
            }
        }
    } else {
        // Invalid path provided
        if (strcmp(e->shorter_path[n_dest], e->routing[n_dest + 1][n_origin]) == 0) {
            strcpy(e->routing[n_dest + 1][n_origin], "-"); // Remove the path
            if ((aux = findSecondShortest(e, n_origin, n_dest)) == -1) {
                //printf("%s <- null\n", e->shorter_path[n_dest]);  *)#
                strcpy(e->shorter_path[n_dest], "");
                strcpy(e->forwarding[n_dest], "");
                return 1; // No alternate path available
            } else {
                //printf("%s <- %s\n", e->shorter_path[n_dest], e->routing[n_dest + 1][aux]);  *)#
                strcpy(e->shorter_path[n_dest], e->routing[n_dest + 1][aux]);
                sprintf(e->forwarding[n_dest], "%d", aux);
                return 1; // Alternate path set
            }
        }
        strcpy(e->routing[n_dest + 1][n_origin], "-"); // Remove the path
        return 0; // No change in shortest path
    }
    return 0; // No change in shortest path
}

// Removes an adjacent node and updates the tables
int *removeAdj(Encaminhamento *e, char *node) {
    int i, n_node = atoi(node);
    int *updated_paths = (int *)calloc(100, sizeof(int)), n_updated = 0, aux;

    printf("Removed Adj: %s\n", node);
    // Iterate through routing table and update paths affected by the removed node
    for (i = 0; i < 101; i++) {
        if (i > 0) {
            if (e->routing[i][n_node][0] != '-') {
                if (strcmp(e->shorter_path[i - 1], e->routing[i][n_node]) == 0) {
                    strcpy(e->routing[i][n_node], "-"); // Remove the path
                    if ((aux = findSecondShortest(e, n_node, i - 1)) == -1) {
                        //printf("%s <- null\n", e->shorter_path[i - 1]);  *)#
                        strcpy(e->shorter_path[i - 1], "");
                        strcpy(e->forwarding[i - 1], "");
                        updated_paths[n_updated++] = i - 1;
                    } else {
                        //printf("%s <- %s\n", e->shorter_path[i - 1], e->routing[i][aux]);   *)#
                        strcpy(e->shorter_path[i - 1], e->routing[i][aux]);
                        sprintf(e->forwarding[i - 1], "%d", aux);
                        updated_paths[n_updated++] = i - 1;
                    }
                }
                strcpy(e->routing[i][n_node], "-"); // Remove the path
            }
        } else {
            strcpy(e->routing[i][n_node], ""); // Clear routing entry for the self node
        }
    }

    // Return indices of updated paths
    if (n_updated > 0) {
        updated_paths[n_updated] = -1;
        return updated_paths;
    } else {
        free(updated_paths);
        return NULL;
    }
}
