#include "sockets.h"
#include "select.h"

#ifndef FOWA_H_
#define FOWA_H_

typedef struct _encaminhamento
{
    char ***routing;
    char **shorter_path;
    char **fowarding;
} Encaminhamento;

Encaminhamento *initEncaminhamento (char*);

void deleteEncaminhamento (Encaminhamento*);

void ShowFowarding (char**);

void ShowPath (int, char**);

void ShowRouting (int, char***);

int ValidPath(char*, char*);

int addPath(Encaminhamento *e, char *self, char *origin, char *dest, char *path);

int *removeAdj (Encaminhamento *e, char* node);

#endif