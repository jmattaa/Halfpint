#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "include/Halfpint.h"

int main(int argc, char *argv[]) 
{
    Halfpint *editor = calloc(1, sizeof(Halfpint));
    Halfpint_Init(editor);

    while (1)
    {
        Halfpint_RefreshScreen(editor);
        Halfpint_ProcessKeypress(editor);
    }

    Halfpint_DisableRawMode(editor);

    return 0;
}
