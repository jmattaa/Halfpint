#include "include/Halfpint.h"
#include "include/dynbuf.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>


void Halfpint_Init(Halfpint *halfpint)
{
    // init cursor position to top left
    halfpint->cursorX = 0;
    halfpint->cursorY = 0;

    Halfpint_GetWindowSize(halfpint);
    Halfpint_EnableRawMode(halfpint);

    halfpint->buffer = INIT_DYNBUF;  
}

void die(const char *s) 
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3); // position the cursor to top left

    perror(s);
    exit(1);
}

void Halfpint_DisableRawMode(Halfpint *g_halfpint)
{
    int c = tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_halfpint->g_termios);
    if (c - 1) die("tcsetattr");
}

void Halfpint_EnableRawMode(Halfpint *g_halfpint)
{
    if (tcgetattr(STDIN_FILENO, &g_halfpint->g_termios) == -1) die("tcgetattr");

    struct termios raw = g_halfpint->g_termios;

    // set some attributes
    // i ain't writing what everything does
    // do your research Jona!
    // ICRNL - make carriage return ('\r') not be translated to newline ('\n')
    // IXON - dissable software flow control shortcuts
    raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | ISTRIP | IXON );
    // turn off:
    // ECHO - so nothing prints when we press any key
    // ICANON - so we read byte by byte not lines
    // IEXTEN - so we can use ctrl-v
    // ISIG - so we can read ctrl-c and ctrl-z and ctrl-y so we stop signals
    raw.c_lflag &= ~( ECHO | ICANON | IEXTEN | ISIG ); // turn off
    raw.c_oflag &= ~( OPOST );
    raw.c_cflag |= ( CS8 );
    // read timieout
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    // send the attributes
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void Halfpint_MoveCursor(Halfpint *halfpint, char key)
{
    switch (key)
    {
        case 'h': // left
            halfpint->cursorX--;
            break;
        case 'j': // down
            halfpint->cursorY++;
            break;
        case 'k': // up
            halfpint->cursorY--;
            break;
        case 'l': // right
            halfpint->cursorX++;
            break;
    }
}

char Halfpint_ReadKey(Halfpint *g_halfpint)
{
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
        if (nread == -1 && errno != EAGAIN) die("read");

    return c;
}

void Halfpint_ProcessKeypress(Halfpint *halfpint)
{
    char c = Halfpint_ReadKey(halfpint);

    switch (c) 
    {
    case CTRL_('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
        write(STDOUT_FILENO, "\x1b[H", 3); // position the cursor to top left

        exit(0);
        break;
    case 'h':
    case 'k':
    case 'j':
    case 'l':
        Halfpint_MoveCursor(halfpint, c);
        break;

    default: break;
    }
}

void Halfpint_RefreshScreen(Halfpint *halfpint)
{
    dynbuf_Append(halfpint->buffer, "\x1b[?25l", 6); // hide cursor
    dynbuf_Append(halfpint->buffer, "\x1b[H", 3); // position the cursor to top left
    
    Halfpint_DrawRows(halfpint);

    // position the cursor to cursorX and cursorY
    char buf[32];// buffer is 32 chars because there could be big numbers in the cursor position
    // top left starts at 1, 1
    snprintf(buf, sizeof(buf),"\x1b[%d;%dH", halfpint->cursorY + 1, halfpint->cursorX + 1);
    dynbuf_Append(halfpint->buffer, buf, strlen(buf));

    dynbuf_Append(halfpint->buffer, "\x1b[?25h", 6); // show the cursor 
    
    write(STDOUT_FILENO, halfpint->buffer->b, halfpint->buffer->len);
    dynbuf_Free(halfpint->buffer);
}

void Halfpint_DrawRows(Halfpint *halfpint)
{
    char welcome[80];
    int welcomelen = snprintf(welcome, sizeof(welcome), "Halfpint Editor -- Version: %s", HALFPINT_VERSION);
    if (welcomelen > halfpint->cols) welcomelen = halfpint->cols; // if message is to long cut it
    int padding = (halfpint->cols - welcomelen) / 2; 
 
    for (int y = 0; y < halfpint->rows; y++) 
    {
        // display welcome 
        if (y == halfpint->rows / 2)
        {
            // center the message
            if (padding) 
            {
                dynbuf_Append(halfpint->buffer, "~", 1);
                padding--;
            }
            while (padding--) dynbuf_Append(halfpint->buffer, " ", 1); // padd the text to the center with spaces

            dynbuf_Append(halfpint->buffer, welcome, welcomelen);
        }
        else
            dynbuf_Append(halfpint->buffer, "~", 1);

        dynbuf_Append(halfpint->buffer, "\x1b[K", 3); // clear line
        if (y < halfpint->rows - 1)
            dynbuf_Append(halfpint->buffer, "\r\n", 2);
    }
}

int Halfpint_GetWindowSize(Halfpint *halfpint)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) 
        return -1;
    else 
    {
        halfpint->cols = ws.ws_col;
        halfpint->rows = ws.ws_row;
        return 0;
    }
}

