#include "include/Halfpint.h"
#include <string.h>

int Halfpint_ReadKey()
{
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
        if (nread == -1 && errno != EAGAIN)
            die("read");

    if (c == '\x1b')
    {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        // convert arrow keys to be interpreted as hjkl
        if (seq[0] == '[')
        {
            switch (seq[1])
            {
            case 'A':
                return UP_ARR;
            case 'B':
                return DOWN_ARR;
            case 'C':
                return RIGHT_ARR;
            case 'D':
                return LEFT_ARR;
            }
        }

        return '\x1b';
    }

    return c;
}

void Halfpint_ProcessKeypress()
{
    int c = Halfpint_ReadKey();

    switch (c)
    {
    case '\r': // enter
        insertNewline();
        break;

    case CTRL_('q'):
        Halfpint_Quit();
        break;

    case CTRL_('s'):
        Halfpint_Save();

    case FIRST:
    case END:
        Halfpint_MoveCursor(c);
        break;

    case backspace_key:
        // delete character if in insert mode
        if (editor.mode == mode_insert)
            delChar();
        else if (editor.mode == mode_normal) // move cursor if normal mode
            Halfpint_MoveCursor(LEFT);
        break;

    case UP_ARR:
        Halfpint_MoveCursor(UP);
        break;
    case DOWN_ARR:
        Halfpint_MoveCursor(DOWN);
        break;
    case LEFT_ARR:
        Halfpint_MoveCursor(LEFT);
        break;
    case RIGHT_ARR:
        Halfpint_MoveCursor(RIGHT);
        break;

    case escape_key:
        // go to normal mode if mode is insert or command
        if (editor.mode == mode_insert || editor.mode == mode_command)
            editor.mode = mode_normal;
        break;

    // refresh screen
    // NOTE: screen refreshs after keypress
    case CTRL_('l'):
        break;

    //!! this needs to be last
    // because we want to write the letters
    // hjkl and i if we're in insert mode and other moving chars
    case UP:
    case DOWN:
    case LEFT:
    case RIGHT:
    case '$': // move to end of line
    case '0': // move to begining of line
        if (editor.mode == mode_normal)
        {
            Halfpint_MoveCursor(c);
            break;
        }
    // write i
    case 'i':
        // go to insert mode if the mode is normal
        if (editor.mode == mode_normal)
        {
            editor.mode = mode_insert;
            break;
        }
    case 'a':
        // go to insert mode on the right of cursor
        if (editor.mode == mode_normal)
        {
            Halfpint_MoveCursor(RIGHT);
            editor.mode = mode_insert;
            break;
        }
    case 'o':
        // make newline and enter insert mode
        if (editor.mode == mode_normal)
        {
            Halfpint_MoveCursor('$');
            insertNewline();
            editor.mode = mode_insert;
            break;
        }
    case '/': // find
        if (editor.mode == mode_normal)
        {
            Halfpint_Find();
            break;
        }
    // enter commnd mode
    case ':':
        // command mode if its normal
        if (editor.mode == mode_normal)
        {
            editor.mode = mode_command;
            Halfpint_RunCmd();
            break;
        }

    default:
        if (editor.mode == mode_insert)
            insertChar(c);
        break;
    }
}


