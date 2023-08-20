// TODO: fix flickering when typing
// prolly from to much being rendered?
// clearing screen may fix?

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "include/Halfpint.h"

int main(int argc, char *argv[])
{
    Halfpint_Init();

    if (argc >= 2)
        Halfpint_OpenEditor(argv[1]);

    while (1)
    {
        Halfpint_RenderScreen();
        Halfpint_ProcessKeypress();
    }

    Halfpint_DisableRawMode();

    return 0;
}
