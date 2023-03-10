#include "include/Halfpint.h"
#include "include/dynbuf.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>

char *Halfpint_Modename(Halfpint *halfpint)
{
    switch (halfpint->mode)
    {
    case mode_normal:
        return "NORMAL";
        break;
    case mode_insert:
        return "INSERT";
        break;
    }

    return NULL;
}

void Halfpint_Init(Halfpint *halfpint)
{
    // init cursor position to top left
    halfpint->cursorX = 0;
    halfpint->cursorY = 0;
    halfpint->renderX = 0;
    halfpint->renderX = 0;

    halfpint->rownum = 0;
    halfpint->rownumdig = 0;
    halfpint->erows = INIT_DYNBUF;
    halfpint->rowoffset = 0;
    halfpint->coloffset = 0;

    Halfpint_GetWindowSize(halfpint);
    // make space for the statusbar and status msg
    halfpint->rows -= 2;

    Halfpint_EnableRawMode(halfpint);

    halfpint->buffer = INIT_DYNBUF;

    halfpint->filename = NULL;

    halfpint->mode = mode_normal;

    halfpint->statusmsg[0] = '\0';
    halfpint->statusmsg_time = 0;
}

// helper function to appendRow
void updateRow(Halfpint *halfpint, struct dynbuf *row)
{
    int tabs = 0;
    int j;
    for (j = 0; j < row->len; j++)
        if (row->b[j] == '\t') tabs++;

    free(row->render);
    row->render = malloc(row->len + tabs * (HALFPINT_TABSTOP - 1) + 1);

    int i = 0;
    for (j = 0; j < row->len; j++) 
    {
        if (row->b[j] == '\t') 
        {
            row->render[i++] = ' ';
            while (i % HALFPINT_TABSTOP != 0) row->render[i++] = ' ';
        } 
        else 
            row->render[i++] = row->b[j];
    }

    row->render[i] = '\0';
    row->rlen = i;
}

// helper function to openeditor
void appendRow(Halfpint *halfpint, char *s, size_t len)
{
    halfpint->erows = realloc(halfpint->erows, sizeof(struct dynbuf) * (halfpint->rownum + 1));

    int rownum = halfpint->rownum;
    halfpint->erows[rownum].len = len;
    halfpint->erows[rownum].b = malloc(len + 1);
    
    memcpy(halfpint->erows[rownum].b, s, len);
    halfpint->erows[rownum].b[len] = '\0'; 

    halfpint->erows[rownum].rlen = 0;
    halfpint->erows[rownum].render = NULL;
    
    updateRow(halfpint, &halfpint->erows[rownum]);

    halfpint->rownum++; 

    // + 1 will give the amount digits in the number
    halfpint->rownumdig = log10(rownum) + 1;
}

void Halfpint_OpenEditor(Halfpint *halfpint, char *filename)
{
    free(halfpint->filename);
    halfpint->filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecapacity = 0;
    ssize_t linelen;
    
    while ((linelen = getline(&line, &linecapacity, fp)) != -1) // get the current line
    {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        appendRow(halfpint, line, linelen);
    }

    free(line);
    fclose(fp);
}

// helper function to Halfpint_ScrollEditor
int rowCxToRx(struct dynbuf *row, int cx) 
{
    int rx = 0;
    int j;

    for (j = 0; j < cx; j++) 
    {
        if (row->b[j] == '\t')
            rx += (HALFPINT_TABSTOP - 1) - (rx % HALFPINT_TABSTOP);
        rx++;
    }
    return rx;
}

void Halfpint_ScrollEditor(Halfpint *halfpint)
{
    halfpint->renderX = halfpint->cursorX;

    if (halfpint->cursorX < halfpint->rownum)
        halfpint->renderX = rowCxToRx(&halfpint->erows[halfpint->cursorY], halfpint->cursorX);

    // y
    if (halfpint->cursorY < halfpint->rowoffset)
        halfpint->rowoffset = halfpint->cursorY;
    if (halfpint->cursorY >= halfpint->rowoffset + halfpint->rows) 
        halfpint->rowoffset = halfpint->cursorY - halfpint->rows + 1;
    
    // x
    if (halfpint->renderX < halfpint->coloffset)
        halfpint->coloffset = halfpint->renderX;
    if (halfpint->renderX >= halfpint->coloffset + halfpint->cols)
        halfpint->coloffset = halfpint->renderX - halfpint->cols + 1;
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
    struct dynbuf *currentrow = (halfpint->cursorY >= halfpint->rownum)? NULL : &halfpint->erows[halfpint->cursorY];

    switch (key)
    {
        case LEFT: 
            if (halfpint->cursorX != 0)
                halfpint->cursorX--;
            else if (halfpint->cursorY > 0) // we're not at last line
            {
                // go to end of last line
                halfpint->cursorY--;
                halfpint->cursorX = halfpint->erows[halfpint->cursorY].len;
            }
            break;
        case DOWN: 
            if (halfpint->cursorY < halfpint->rownum) // so we can scroll until end of file 
                halfpint->cursorY++;
            break;
        case UP: 
            if (halfpint->cursorY != 0)
                halfpint->cursorY--;
            break;
        case RIGHT: 
            if (currentrow && halfpint->cursorX < currentrow->len) // so we dont move past than line length
                halfpint->cursorX++; 
            else if (currentrow && halfpint->cursorX == currentrow->len) // we're at the end of the line
            {
                // go to begining of next line
                halfpint->cursorY++;
                halfpint->cursorX = 0;
            }
            break;
        case FIRST:
            // place the cursor at the begining of file
            halfpint->cursorY = 0;
            halfpint->cursorX = 0;
            break;
        case END:
            // place the cursor at the last line
            halfpint->cursorY = halfpint->rownum;
            break;

    }

    // snap the cursor to end of line
    currentrow = (halfpint->cursorY >= halfpint->rownum)? NULL : &halfpint->erows[halfpint->cursorY];
    int rowlen = currentrow? currentrow->len : 0;
    if (halfpint->cursorX > rowlen)
        halfpint->cursorX = rowlen;
}

char Halfpint_ReadKey(Halfpint *g_halfpint)
{
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
        if (nread == -1 && errno != EAGAIN) die("read");

    if (c == '\x1b') 
    {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        // convert arrow keys to be interpreted as hjkl
        if (seq[0] == '[') 
        {
            switch (seq[1]) 
            {
            case 'A': 
                return UP;
            case 'B': 
                return DOWN;
            case 'C': 
                return RIGHT;
            case 'D': 
                return LEFT;
            }
        }

        return '\x1b';
    }

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
    case 'i':
        // go to insert mode if the mode is normal
        if (halfpint->mode == mode_normal)
            halfpint->mode = mode_insert;
        break;
    case escape_key:
        // go to normal mode if mode is insert
        if (halfpint->mode == mode_insert)
            halfpint->mode = mode_normal;
        break;
    case UP:
    case DOWN:
    case LEFT:
    case RIGHT:
    case FIRST:
    case END:
        Halfpint_MoveCursor(halfpint, c);
        break;

    default: break;
    }
}

void drawStatusbar(Halfpint *halfpint)
{
    dynbuf_Append(halfpint->buffer, "\x1b[40;33m", 8); // colors
    
    char status[64], renderstatus[64];
    int len = snprintf(
            status, 
            sizeof(status), 
            "%.20s -- %s --", // %.20s max 20 cols
            halfpint->filename ? halfpint->filename : "[No Name]", // if the filename is NULL display no name
            Halfpint_Modename(halfpint));


    int fprecent = ((float)(halfpint->cursorY + 1) / (float)halfpint->rownum) * 100.0f; // get precentage of file

    int renderlen = snprintf(
            renderstatus,
            sizeof(renderstatus),
            "%d:%d  -  %d%%", // show current position and precentage of file we're in
            halfpint->cursorY + 1, // current line we're on
            halfpint->cursorX + 1, // current column
            fprecent);

    dynbuf_Append(halfpint->buffer, status, len);
    
    // draw inverted colors line
    while (len < halfpint->cols)
    {
        // write renderstatus at end of line
        if (halfpint->cols - len == renderlen)
        {
            dynbuf_Append(halfpint->buffer, renderstatus, renderlen);
            break;
        }

        dynbuf_Append(halfpint->buffer, " ", 1);
        len++;
    }

    dynbuf_Append(halfpint->buffer, "\x1b[m", 3); // reset colors
    dynbuf_Append(halfpint->buffer, "\r\n", 2); // go to statusmsg
}

void drawMessage(Halfpint *halfpint)
{
    dynbuf_Append(halfpint->buffer, "\x1b[K", 3); // clear the message bar
    
    int msglen = strlen(halfpint->statusmsg);
    if (msglen > halfpint->cols) msglen = halfpint->cols; // cut message

    if (msglen && time(NULL) - halfpint->statusmsg_time < 5) // let message display for 5 seconds 

    dynbuf_Append(halfpint->buffer, halfpint->statusmsg, msglen);
}

void Halfpint_RefreshScreen(Halfpint *halfpint)
{
    Halfpint_ScrollEditor(halfpint);

    dynbuf_Append(halfpint->buffer, "\x1b[?25l", 6); // hide cursor
    dynbuf_Append(halfpint->buffer, "\x1b[H", 3); // position the cursor to top left
    
    Halfpint_DrawRows(halfpint);
    drawStatusbar(halfpint);
    drawMessage(halfpint);

    // position the cursor to cursorX and cursorY
    char buf[32];// buffer is 32 chars because there could be big numbers in the cursor position

    // translate file position to screen position and start from after the line number on x axis
    int curypos = ((halfpint->cursorY - halfpint->rowoffset) + 1);
    // +2 because we have 2 spaces after line number
    int curxpos = ((halfpint->renderX - halfpint->coloffset) + 1) + halfpint->rownumdig + 2;

    // top left starts at 1, 1
    snprintf(buf, sizeof(buf),"\x1b[%d;%dH", curypos, curxpos); 
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
 
    for (int filerow = 0; filerow < halfpint->rows; filerow++) 
    {
        int currentrow = filerow + halfpint->rowoffset;
        if (currentrow >= halfpint->rownum)
        {
            // display welcome if there is no file to open or file is empty
            if (halfpint->rownum == 0 && filerow == halfpint->rows / 2)
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
        }
        else 
        {
            int len = halfpint->erows[currentrow].rlen - halfpint->coloffset;
            if (len > halfpint->cols) len = halfpint->cols;
            
            char linenum[128];

            int numlen = snprintf(linenum, sizeof(linenum), "%*d  ", halfpint->rownumdig, currentrow + 1); // make the string be the max amount of character width

            dynbuf_Append(halfpint->buffer, "\x1b[90m", 5);
            dynbuf_Append(halfpint->buffer, linenum, numlen); // line number
            dynbuf_Append(halfpint->buffer, "\x1b[0m", 4);

            dynbuf_Append(halfpint->buffer, &halfpint->erows[currentrow].render[halfpint->coloffset], len);
        }

        dynbuf_Append(halfpint->buffer, "\x1b[K", 3); // clear line
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

void Halfpint_SetStatusMessage(Halfpint *halfpint, const char *s, ...)
{
    va_list argp;

    va_start(argp, s);
    vsnprintf(halfpint->statusmsg, sizeof(halfpint->statusmsg), s, argp);
    va_end(argp);

    halfpint->statusmsg_time = time(NULL);
}

