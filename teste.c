#include "sockets.h"
#include <stdio.h>
#include "fowarding.h"

int main(){
    Encaminhamento *e = initEncaminhamento("30");

    ShowFowarding(e->fowarding);

    addPath(e, "30", "8", "8", "8");
    addPath(e, "30", "12", "8", "12-8");
    addPath(e, "30", "15", "8", "15-21-10");

    addPath(e, "30", "8", "10", "8-10");
    addPath(e, "30", "12", "10", "12-8-10");
    addPath(e, "30", "15", "10", "15-21-10");

    addPath(e, "30", "8", "12", "8-12");
    addPath(e, "30", "12", "12", "12");

    addPath(e, "30", "15", "15", "15");

    addPath(e, "30", "8", "21", "8-10-21");
    addPath(e, "30", "12", "21", "12-8-10-21");
    addPath(e, "30", "15", "21", "15-21");

    ShowFowarding(e->fowarding);
    ShowPath(21, e->shorter_path);

    int *aux = removeAdj(e, "15");
    for(int i = 0; aux != NULL && aux[i] != -1; i++) printf("Atualizado: %d\n", aux[i]);
    ShowPath(21, e->shorter_path);
    ShowPath(15, e->shorter_path);

    deleteEncaminhamento(e);
    return 0;
}