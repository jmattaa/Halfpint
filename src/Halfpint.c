#include "include/Halfpint.h"
#include "include/dynbuf.h"
#include <string.h>

Halfpint editor = {.buffer = NULL};

char *Halfpint_Modename()
{
    switch (editor.mode)
    {
    case mode_normal:
        return "NORMAL";
        break;
    case mode_insert:
        return "INSERT";
        break;
    case mode_command:
        return "COMMAND";
        break;
    }

    return NULL;
}

void Halfpint_Init()
{
    editor.cursorX = 0;
    editor.cursorY = 0;
    editor.renderX = 0;
    editor.renderX = 0;

    editor.rownum = 0;
    editor.rownumdig = 0;
    editor.erows = INIT_DYNBUF;
    editor.rowoffset = 0;
    editor.coloffset = 0;

    Halfpint_GetWindowSize();
    // make space for the statusbar and status msg
    editor.rows -= 2;

    Halfpint_EnableRawMode();

    editor.buffer = INIT_DYNBUF;

    editor.filename = NULL;

    editor.mode = mode_normal;

    editor.statusmsg[0] = '\0';
    editor.statusmsg_time = 0;

    editor.saved = 1;

    editor.syntax = NULL;
}

void Halfpint_OpenEditor(char *filename)
{
    free(editor.filename);
    editor.filename = strdup(filename);

    Halfpint_SyntaxDetect();

    FILE *fp = fopen(filename, "r");
    if (!fp)
        die("fopen");

    char *line = NULL;
    size_t linecapacity = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecapacity, fp)) != -1) // get the current line
    {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        insertRow(editor.rownum, line, linelen);
    }

    free(line);
    fclose(fp);

    editor.saved = 1;
}

void Halfpint_Save()
{
    if (editor.filename == NULL)
    {
        editor.filename = Halfpint_Prompt("Save file as: %s (Press ESC to cancel)", NULL);

        if (editor.filename == NULL) // user canceled
        {
            Halfpint_SetStatusMessage("Save canceled");
            return;
        }
        Halfpint_SyntaxDetect();
    }

    int len;
    char *buffer = rowsToString(&len);

    int fd = open(editor.filename, O_RDWR | O_CREAT, 0644); // 0644 - owner can read and write and others can only read

    if (fd != -1)
    {
        if (ftruncate(fd, len) != -1)
        {
            if (write(fd, buffer, len) == len)
            {
                close(fd);
                free(buffer);
                Halfpint_SetStatusMessage("%dB written to '%s'", len, editor.filename);

                editor.saved = 1;

                return;
            }
        }
        close(fd);
    }

    free(buffer);
    Halfpint_SetStatusMessage("Couldn't save ERR: %s", strerror(errno));
}

// helper function to Halfpint_ScrollEditor
int rowCxToRx(struct dynbuf *row, int cx)
{
    int rx = 0;

    for (int j = 0; j < cx; j++)
    {
        if (row->b[j] == '\t')
            rx += (HALFPINT_TABSTOP - 1) - (rx % HALFPINT_TABSTOP);
        rx++;
    }

    return rx;
}

// opposite of rowCxToRx
int rowRxToCx(struct dynbuf *row, int rx)
{
    int cursor_rx = 0;
    int cx;
    for (cx = 0; cx < row->len; cx++)
    {
        if (row->b[cx] == '\t')
            cursor_rx += (HALFPINT_TABSTOP - 1) - (cursor_rx % HALFPINT_TABSTOP);
        cursor_rx++;
        if (cursor_rx > rx) return cx;
    }

    return cx;
}

void Halfpint_ScrollEditor()
{
    editor.renderX = editor.cursorX;

    if (editor.cursorX < editor.rownum)
        editor.renderX = rowCxToRx(&editor.erows[editor.cursorY], editor.cursorX);

    // y
    if (editor.cursorY < editor.rowoffset)
        editor.rowoffset = editor.cursorY;
    if (editor.cursorY >= editor.rowoffset + editor.rows)
        editor.rowoffset = editor.cursorY - editor.rows + 1;

    // x
    if (editor.renderX < editor.coloffset)
        editor.coloffset = editor.renderX;
    if (editor.renderX >= editor.coloffset + editor.cols)
        editor.coloffset = editor.renderX - editor.cols + 1;
}

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // position the cursor to top left

    perror(s);
    exit(1);
}

void Halfpint_DisableRawMode()
{
    int c = tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.g_termios);
    if (c == - 1)
        die("tcsetattr");
}

void Halfpint_EnableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &editor.g_termios) == -1)
        die("tcgetattr");

    struct termios raw = editor.g_termios;

    atexit(Halfpint_DisableRawMode);
    // set some attributes
    // i ain't writing what everything does
    // do your research Jona!
    // ICRNL - make carriage return ('\r') not be translated to newline ('\n') this means that enter is \r
    // IXON - dissable software flow control shortcuts
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // turn off:
    // ECHO - so nothing prints when we press any key
    // ICANON - so we read byte by byte not lines
    // IEXTEN - so we can use ctrl-v
    // ISIG - so we can read ctrl-c and ctrl-z and ctrl-y so we stop signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // turn off
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    // read timieout
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    // send the attributes
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

void Halfpint_MoveCursor(char key)
{
    struct dynbuf *currentrow = (editor.cursorY >= editor.rownum) ? NULL : &editor.erows[editor.cursorY];

    switch (key)
    {
    case LEFT:
        if (editor.cursorX != 0)
            editor.cursorX--;

        // Uncomment to move line when we press right
        // else if (editor.cursorY > 0) // we're not at last line
        //{
        //   // go to end of last line
        //    editor.cursorY--;
        //    editor.cursorX = editor.erows[editor.cursorY].len;
        //}
        break;
    case DOWN:
        if (editor.cursorY < editor.rownum - 1) // so we can scroll until end of screen
            editor.cursorY++;
        break;
    case UP:
        if (editor.cursorY != 0)
            editor.cursorY--;
        break;
    case RIGHT:
        if (currentrow && editor.cursorX < currentrow->len) // so we dont move past than line length
            editor.cursorX++;

        // Uncomment to move line when we press right
        // else if (currentrow && editor.cursorX == currentrow->len) // we're at the end of the line
        //
        //    // go to begining of next line
        //    editor.cursorY++;
        //    editor.cursorX = 0;
        //}
        break;
    case '$': // move to end of line
        editor.cursorX = currentrow->len;
        break;
    case '0': // move to beginig of line
        editor.cursorX = 0;
        break;
    case FIRST:
        // place the cursor at the begining of file
        editor.cursorY = 0;
        editor.cursorX = 0;
        break;
    case END:
        // place the cursor at the last line
        editor.cursorY = editor.rownum - 1;
        break;
    }

    // snap the cursor to end of line
    currentrow = (editor.cursorY >= editor.rownum) ? NULL : &editor.erows[editor.cursorY];
    int rowlen = currentrow ? currentrow->len : 0;
    if (editor.cursorX > rowlen)
        editor.cursorX = rowlen;
}

int Halfpint_GetWindowSize()
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
        return -1;
    else
    {
        editor.cols = ws.ws_col;
        editor.rows = ws.ws_row;
        return 0;
    }
}

void Halfpint_Quit()
{
    if (!editor.saved)
    {
        char *ans = Halfpint_Prompt(
                "Unsaved file. Quit without saving (y/n)? %s", NULL
        );

        if (strcmp(ans, "y") != 0)
            return;
    }

    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);  // position the cursor to top left

    exit(0);
}


// helper function for find
void findCallback(char *query, int key)
{
    static int last_match = -1;
    static int direction = 1;

    static int saved_hl_line;
    static char *saved_hl = NULL;
    if (saved_hl) {
        memcpy(editor.erows[saved_hl_line].hl, saved_hl, 
                editor.erows[saved_hl_line].rlen);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == CTRL_('n')) { // next search result
        direction = 1;
    } else if (key == CTRL_('p')) { // previous search result
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1) 
        direction = 1;
    int current = last_match;

    for (int i = 0; i < editor.rownum; i++)
    {
        current += direction;
        if (current == -1) current = editor.rownum - 1;
        else if (current == editor.rownum) current = 0;

        struct dynbuf *row = &editor.erows[current];
        char *match = strstr(row->render, query);
    
        if (match) // found match
        {
            last_match = current;

            // place cursor at the match
            editor.cursorY = current;
            editor.cursorX = rowRxToCx(row, match - row->render);

            // this will scroll to the bottom
            // so when we call Halfpint_ScrollEditor we get our 
            // line in the top of the screen
            editor.rowoffset = editor.rownum;
            

            saved_hl_line = current;
            saved_hl = malloc(row->rlen);
            memcpy(saved_hl, row->hl, row->rlen);
            // set syntax for match
            memset(&row->hl[match - row->render], hl_match, strlen(query));

            break;
        }
    }
}

void Halfpint_Find()
{
    int saved_cx = editor.cursorX;
    int saved_cy = editor.cursorY;
    int saved_coloff = editor.coloffset;
    int saved_rowoff = editor.rowoffset;

    char *query = Halfpint_Prompt("/%s", findCallback);

    if (query)
        free(query);
    else
    {
        editor.cursorY = saved_cx;
        editor.cursorY = saved_cy;
        editor.coloffset = saved_coloff;
        editor.rowoffset = saved_rowoff;
    }
}

void Halfpint_RunCmd() 
{
    char* cmd = Halfpint_Prompt(":%s", NULL);

    if(!cmd) // cancel if we dont give some command
    {
        editor.mode = mode_normal;
        return;
    }
    
    if (strcmp(cmd, "q") == 0)
    {
        Halfpint_Quit();
    }
    else if (strcmp(cmd, "w") == 0)
    {
        Halfpint_Save();
    }
    else if (strcmp(cmd, "wq") == 0) 
    {
        Halfpint_Save();
        Halfpint_Quit();
    }
    else
    {
        Halfpint_SetStatusMessage("Command not found");
    }

    // after we've got the command we leave command mode
    editor.mode = mode_normal;
}


