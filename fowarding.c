#include "fowarding.h"

Encaminhamento *initEncaminhamento (){
    int i, j;
    Encaminhamento *new = NULL;
    
    new = (Encaminhamento*) calloc (1, sizeof(Encaminhamento));
    // Só tem 3 (succ, pred e 1 chord)!! Convem ver com a questão das cordas que partem de outros nós
    new->routing = (char***) calloc (3, sizeof(char**));
    for(i = 0; i < 3; i++){
        new->routing[i] = (char**) calloc (101, sizeof(char*));      // Atenção aos acessos nos indices (index = node + 1)
        new->routing[i][0] = (char*) calloc (4, sizeof(char));
        for (j = 1; j < 101; j++) new->routing[i][j] = (char*) calloc (100, sizeof(char));
    }
    new->shorter_path = (char**) calloc (100, sizeof(char*));
    for (i = 0; i < 100; i++) new->shorter_path[i] = (char*) calloc (100, sizeof(char));
    new->fowarding = (int*) malloc (100*sizeof(int));
    for (i = 0; i < 100; i++) new->fowarding[i] = -1;

    return new;
}

void deleteEncaminhamento (Encaminhamento *e){
    int i, j;

    for (i = 0; i < 3; i++){
        for (j = 0; j < 101; j++) free(e->routing[i][j]);
        free (e->routing[i]);
    }
    for (i = 0; i < 100; i++){
        free(e->shorter_path[i]); 
    }
    free (e->routing);
    free (e->shorter_path);
    free (e->fowarding);
    free (e);
    return;
}

// Shows a node's expedition table
void ShowFowarding (int* fowarding){
    int i, aux = 0;

    printf ("Expedition Table:\n");
    for (i = 0; i < 100; i++){
        if (fowarding[i] == -1) continue;
        aux++;
        printf ("Neighbor: %02d, Destiny: %02d \n", i, fowarding[i]);
    }
    if (aux == 0) printf("Is the only node in the ring!\n");
    printf ("\n");
    return;
}

//Shows the shortest path from a node to the dest destination
void ShowPath (int node, char** path){
    // It is not possible to reach the node -> Is not in the ring
    if (strcmp(path[node], "") == 0) printf ("Invalid destiny, node %2d is not in the ring!\n", node);
    else printf ("Shorter Path to Node %02d: %s\n", node, path[node]);
    printf ("\n");
    return;
}

//Poderá ter que sofrer alterações consoante a resposta do stor em relação às cordas !!!!!
// Shows a node's forwarding table entry relative to the dest destination 
void ShowRouting (int node, char*** routing){
    int i, aux = 0;

    for (i = 0; i < 3; i++){
        if (strcmp(routing[i][0], "") == 0) aux++;
    }
    // It is not possible to reach the node -> Is not in the ring
    if (aux == 3){
        printf("Invalid destiny, node %02d is not in the ring!\n\n", node);
        return;
    }
    printf ("Routing Table: \n");
    for (i = 0; i < 3; i++){
        if (strcmp(routing[i][0], "") == 0) continue;
        printf ("Path to %02d by %s: %s\n", node, routing[i][0], routing[i][node+1]);
    }
    printf ("\n");
    return;
}

// Checks if the path is valid
int ValidPath(char *path, char *node) {
    int i, j, flag;

    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] == node[0]) {
            flag = 1;
            for (j = 0; node[j] != '\0'; j++) {
                if (path[i + j] != node[j]) {
                    flag = 0;
                    break;
                }
            }
            // Invalid path (contains the starting node)
            if (flag == 1) {
                return 0;
            }
        }
    }
    // Valid path
    return 1; 
}