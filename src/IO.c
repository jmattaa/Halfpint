#include "include/Halfpint.h"
#include <stddef.h>
#include <stdlib.h>

void Halfpint_SetStatusMessage(const char *s, ...)
{
    va_list argp;

    va_start(argp, s);
    vsnprintf(editor.statusmsg, sizeof(editor.statusmsg), s, argp);
    va_end(argp);

    editor.statusmsg_time = time(NULL);
}


char *Halfpint_Prompt(char *prompt)
{
    size_t inpsize = 256;
    char *inp = malloc(inpsize);

    size_t inplen = 0;
    inp[0] = '\0';

    while (1)
    {
        Halfpint_SetStatusMessage(prompt, inp);
        Halfpint_RenderScreen();

        int c = Halfpint_ReadKey();

        if (c == '\x1b')
        {
            Halfpint_SetStatusMessage("");
            free(inp);
            return NULL;
        }
        else if (c == backspace_key)
        {
            if(inplen != 0) inp[--inplen] = '\0';
        }
        else if (c == '\r')
        {
            if (inplen != 0)
            {
                Halfpint_SetStatusMessage("");
                return inp;
            }
        }
        else if (!iscntrl(c) && c < 128) 
        {
            if (inplen == inpsize - 1) 
            {
                inpsize *= 2;
                inp = realloc(inp, inpsize);
            }

            inp[inplen++] = c;
            inp[inplen] = '\0';
        }
    }
}
