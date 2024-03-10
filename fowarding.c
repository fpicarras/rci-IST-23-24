#include "fowarding.h"

Encaminhamento *initEncaminhamento (char *self){
    int i, j;
    Encaminhamento *new = NULL;
    
    new = (Encaminhamento*) calloc (1, sizeof(Encaminhamento));
    /* Primeira linha tem uma flag para verificar se somos adjacentes ao nó em questão */
    new->routing = (char***) calloc (101, sizeof(char**));          
    // Atenção aos acessos nos indices (index = node + 1)
    for(i = 0; i < 101; i++){
        new->routing[i] = (char**) calloc (100, sizeof(char*));      
        for (j = 0; j < 100; j++){
            new->routing[i][j] = (char*) calloc (100, sizeof(char));
            if (i > 0) strcpy (new->routing[i][j], "-");
            else strcpy(new->routing[i][j], "");
        }
    }
    new->shorter_path = (char**) calloc (100, sizeof(char*));
    for (i = 0; i < 100; i++) new->shorter_path[i] = (char*) calloc (100, sizeof(char));
    new->fowarding = (char**) calloc (100, sizeof(char*));
    for (i = 0; i < 100; i++) new->fowarding[i] = (char*) calloc (4, sizeof(char));

    sprintf(new->shorter_path[atoi(self)], "%d", atoi(self));
    sprintf(new->fowarding[atoi(self)], "%d", atoi(self));

    return new;
}

void deleteEncaminhamento (Encaminhamento *e){
    int i, j;

    for (i = 0; i < 101; i++){
        for (j = 0; j < 100; j++) free(e->routing[i][j]);
        free (e->routing[i]);
    }
    for (i = 0; i < 100; i++){
        free(e->shorter_path[i]); 
        free(e->fowarding[i]); 
    }
    free (e->routing);
    free (e->shorter_path);
    free (e->fowarding);
    free (e);
    return;
}

// Shows a node's expedition table
void ShowFowarding (char** fowarding){
    int i, aux = 0;

    for (i = 0; i < 100; i++){
        if (strcmp (fowarding[i], "") == 0) continue;
        if (aux == 0) printf ("Expedition Table:\n");
        aux++;
        printf ("Neighbor: %s, Destination: %02d \n", fowarding[i], i);
    }
    if (aux <= 1) printf("Is the only node in the ring!\n");
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

    for (i = 0; i < 100; i++){
        if (strcmp(routing[0][i], "") == 0) aux++;
    }
    // It is not possible to reach the node -> Is not in the ring
    if (aux == 100){
        printf("Invalid destination, node %02d is not in the ring!\n\n", node);
        return;
    }
    printf ("Routing Table: \n");
    for (i = 0; i < 100; i++){
        if (strcmp(routing[node+1][i], "-") == 0) continue;
        printf ("Path to %02d by %02d: %s\n", node, i, routing[node+1][i]);
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

//Returns the tindex of the second shortest path in the routing table, -1 if there is no other
int findSecondShortest(Encaminhamento *e, int node_leaving, int dest){
    int aux = -1;

    for(int col = 0; col < 100; col ++){
        if(e->routing[dest+1][col][0] != '-'){
            if(aux == -1){
                aux = col;
            }else {
                if(strlen(e->routing[dest+1][aux]) > strlen(e->routing[dest+1][col]) && (strcmp(e->routing[dest+1][col], e->shorter_path[dest])!=0)){
                    aux = col;
                }
            }
        }
    }
    return aux;
}

/* Adds a path to the tables, returns 1 if the f and sp tables were updated, 0 otherwise*/
int addPath(Encaminhamento *e, char *self, char *origin, char *dest, char *path){
    int n_origin = atoi(origin), n_dest = atoi(dest), aux;

    if(path == NULL){
        if(strcmp(e->shorter_path[n_dest], e->routing[n_dest+1][n_origin])==0){
            strcpy(e->routing[n_dest+1][n_origin], "-");
            if((aux = findSecondShortest(e, n_origin, n_dest))==-1){
                printf("%s <- null\n", e->shorter_path[n_dest]);
                strcpy (e->shorter_path[n_dest], "");
                strcpy (e->fowarding[n_dest], "");
                return 1;
            }else {
                printf("%s <- %s\n", e->shorter_path[n_dest], e->routing[n_dest+1][aux]);
                strcpy (e->shorter_path[n_dest], e->routing[n_dest+1][aux]);
                sprintf(e->fowarding[n_dest], "%d", aux);
                return 1;
            }
        }
        strcpy(e->routing[n_dest+1][n_origin], "-");
        return 0;
    }

    if(ValidPath(path, self)){
        e->routing[0][n_origin][0] = '1';

        sprintf(e->routing[n_dest+1][n_origin], "%s-%s", self, path);
        
        if(e->shorter_path[n_dest][0] == '\0'){ //It is empty
            printf("%s <- ", e->shorter_path[n_dest]);
            sprintf(e->shorter_path[n_dest], "%s-%s", self, path);
            sprintf(e->fowarding[n_dest], "%s", origin);
            printf("%s\n", e->shorter_path[n_dest]);
            return 1;
        }else{
            //The path there is already the shortest
            //printf("Compare: %s %s\n", e->shorter_path[n_dest], e->routing[n_dest+1][n_origin]);
            if(strlen(e->shorter_path[n_dest]) <= strlen(e->routing[n_dest+1][n_origin])){
                return 0;
            }else { //If we have a new shortest path, we replace the old one
                printf("%s <- ", e->shorter_path[n_dest]);
                sprintf(e->shorter_path[n_dest], "%s-%s", self, path);
                sprintf(e->fowarding[n_dest], "%s", origin);
                printf("%s\n", e->shorter_path[n_dest]);
                return 1;
            }
        }
    }
    return 0;
}

// Quando um nó é removido a tabela na sua coluna é eliminada
/* Se os caminhos mais curtos forem atualizados, retorna um vetor com os indices destino
   que viram o seu caminho atualizado, este vetor ´e terminado em -1, retrona NULL se nenhum foi atualizado */
int *removeAdj (Encaminhamento *e, char* node){
    printf("Removing Adj with: %s\n", node);
    int i, n_node = atoi(node);
    int *updated_paths = (int*)calloc(100, sizeof(int)), n_updated = 0, aux;

    for (i = 0; i < 101; i++){
        if(i > 0){
            
            if(e->routing[i][n_node][0] != '-'){
                if(strcmp(e->shorter_path[i-1], e->routing[i][n_node])==0){
                    if((aux = findSecondShortest(e, n_node, i-1))==-1){
                        printf("%s <- null\n", e->shorter_path[i-1]);
                        strcpy (e->shorter_path[i-1], "");
                        strcpy (e->fowarding[i-1], "");
                        updated_paths[n_updated++] = i-1;
                    }else {
                        printf("%s <- %s\n", e->shorter_path[i-1], e->routing[i-1][aux]);
                        strcpy (e->shorter_path[i-1], e->routing[i][aux]);
                        sprintf(e->fowarding[i-1], "%d", aux);
                        updated_paths[n_updated++] = i-1;
                    }
                }
                strcpy (e->routing[i][n_node], "-");
            }
        }
        else{
            strcpy(e->routing[i][n_node], "");
        }

    }
    strcpy (e->shorter_path[n_node], "");
    strcpy (e->fowarding[n_node], "");

    if(n_updated > 0){
        updated_paths[n_updated] = -1;
        return updated_paths;
    }else {
        free(updated_paths); return NULL;
    }
}