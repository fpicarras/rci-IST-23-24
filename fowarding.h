#include "sockets.h"
#include "select.h"

#ifndef FOWA_H_
#define FOWA_H_

typedef struct _encaminhamento
{
    char ***routing;
    char **shorter_path;
    int *fowarding;
} Encaminhamento;

Encaminhamento *initEncaminhamento ();

void deleteEncaminhamento (Encaminhamento*);

void ShowFowarding (int*);

void ShowPath (int, char**);

void ShowRouting (int, char***);

int ValidPath(char*, char*);

#endif