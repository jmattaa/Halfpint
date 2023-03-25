#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "include/Halfpint.h"

int main(int argc, char *argv[])
{
    Halfpint *editor = Halfpint_Init();

    if (argc >= 2)
        Halfpint_OpenEditor(editor, argv[1]);

    Halfpint_SetStatusMessage(editor, "HELP: Ctrl-Q = quit");

    while (1)
    {
        Halfpint_RefreshScreen(editor);
        Halfpint_ProcessKeypress(editor);
    }

    Halfpint_DisableRawMode(editor);

    return 0;
}
