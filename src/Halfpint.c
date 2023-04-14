#include "include/Halfpint.h"
#include "include/dynbuf.h"

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
}

// put render chars in render
void updateRow(struct dynbuf *row)
{
    int tabs = 0;
    int j;
    for (j = 0; j < row->len; j++)
        if (row->b[j] == '\t')
            tabs++;

    free(row->render);
    row->render = malloc(row->len + tabs * (HALFPINT_TABSTOP - 1) + 1);

    int i = 0;
    for (j = 0; j < row->len; j++)
    {
        if (row->b[j] == '\t')
        {
            row->render[i++] = ' ';
            while (i % HALFPINT_TABSTOP != 0)
                row->render[i++] = ' ';
        }
        else
            row->render[i++] = row->b[j];
    }

    row->render[i] = '\0';
    row->rlen = i;
}

void insertRow(int at, char *s, size_t len)
{
    if (at < 0 || at > editor.rownum)
        return;

    editor.erows = realloc(editor.erows, sizeof(struct dynbuf) * (editor.rownum + 1));
    memmove(&editor.erows[at + 1], &editor.erows[at], sizeof(struct dynbuf) * (editor.rownum - at));

    editor.erows[at].len = len;
    editor.erows[at].b = malloc(len + 1);

    memcpy(editor.erows[at].b, s, len);
    editor.erows[at].b[len] = '\0';

    editor.erows[at].rlen = 0;
    editor.erows[at].render = NULL;

    updateRow(&editor.erows[at]);

    editor.rownum++;

    // + 1 will give the amount digits in the number + 1 again to add padding
    editor.rownumdig = log10(editor.rownum + 1) + 1;

    editor.saved = 0;
}

void rowInsertChar(struct dynbuf *row, int at, char c)
{
    if (at < 0 || at > row->len)
        at = row->len;

    row->b = realloc(row->b, row->len + 2); // make room for '\0' and c
    memmove(&row->b[at + 1], &row->b[at], row->len - at + 1);

    row->len++;
    row->b[at] = c; // insert char

    updateRow(row);

    editor.saved = 0;
}

void insertNewline()
{
    if (editor.cursorX == 0)
    {
        insertRow(editor.cursorY, "", 0);

        editor.cursorY++;
        editor.cursorX = 0;

        return;
    }

    // split line into to lines
    struct dynbuf *row = &editor.erows[editor.cursorY];
    insertRow(editor.cursorY + 1, &row->b[editor.cursorX], row->len - editor.cursorX);
    row = &editor.erows[editor.cursorY];
    row->len = editor.cursorX;
    row->b[row->len] = '\0';
    updateRow(row);

    editor.cursorY++;
    editor.cursorX = 0;
}

void insertChar(char c)
{
    if (editor.cursorY == editor.rownum) // insert a newline after file
        insertRow(editor.rownum, "", 0);

    rowInsertChar(&editor.erows[editor.cursorY], editor.cursorX, c);
    editor.cursorX++;
}

char *rowsToString(int *buflen)
{
    int totlen = 0;
    int j;

    for (j = 0; j < editor.rownum; j++)
        totlen += editor.erows[j].len + 1;

    *buflen = totlen;
    char *buf = malloc(totlen);
    char *p = buf;

    for (j = 0; j < editor.rownum; j++)
    {
        memcpy(p, editor.erows[j].b, editor.erows[j].len);
        p += editor.erows[j].len;
        *p = '\n';
        p++;
    }

    return buf;
}

void rowAppendString(struct dynbuf *row, char *s, int len)
{
    row->b = realloc(row->b, row->len + len + 1);
    memcpy(&row->b[row->len], s, len);

    row->len += len;
    row->b[row->len] = '\0';

    updateRow(row);

    editor.saved = 0;
}

void rowDelChar(struct dynbuf *row, int at)
{
    if (at < 0 || at >= row->len)
        return; // Invalid at

    memmove(&row->b[at], &row->b[at + 1], row->len - at);

    row->len--;
    updateRow(row);

    editor.saved = 0;
}

void delRow(int at)
{
    if (at < 0 || at >= editor.rownum)
        return; // Invalid at

    dynbuf_Free(&editor.erows[at]);
    memmove(&editor.erows[at], &editor.erows[at + 1], sizeof(struct dynbuf) * (editor.rownum - at - 1));

    editor.rownum--;

    editor.saved = 0;
}

void delChar()
{
    if (editor.cursorY == editor.rownum)
        return;
    if (editor.cursorY == 0 && editor.cursorX == 0)
        return;

    struct dynbuf *row = &editor.erows[editor.cursorY];

    if (editor.cursorX <= 0) // delete row
    {
        editor.cursorX = editor.erows[editor.cursorY - 1].len;
        rowAppendString(&editor.erows[editor.cursorY - 1], row->b, row->len);

        delRow(editor.cursorY);
        editor.cursorY--;

        return;
    }

    rowDelChar(row, editor.cursorX - 1);
    editor.cursorX--;
}

void Halfpint_OpenEditor(char *filename)
{
    free(editor.filename);
    editor.filename = strdup(filename);

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
        // TODO let create file
        return;

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
    int j;

    for (j = 0; j < cx; j++)
    {
        if (row->b[j] == '\t')
            rx += (HALFPINT_TABSTOP - 1) - (rx % HALFPINT_TABSTOP);
        rx++;
    }
    return rx;
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
    if (c - 1)
        die("tcsetattr");
}

void Halfpint_EnableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &editor.g_termios) == -1)
        die("tcgetattr");

    struct termios raw = editor.g_termios;

    // set some attributes
    // i ain't writing what everything does
    // do your research Jona!
    // ICRNL - make carriage return ('\r') not be translated to newline ('\n')
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
        if (editor.cursorY < editor.rownum) // so we can scroll until end of screen
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
    case FIRST:
        // place the cursor at the begining of file
        editor.cursorY = 0;
        editor.cursorX = 0;
        break;
    case END:
        // place the cursor at the last line
        editor.cursorY = editor.rownum;
        break;
    }

    // snap the cursor to end of line
    currentrow = (editor.cursorY >= editor.rownum) ? NULL : &editor.erows[editor.cursorY];
    int rowlen = currentrow ? currentrow->len : 0;
    if (editor.cursorX > rowlen)
        editor.cursorX = rowlen;
}

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
    static int quit_times = 2;

    int c = Halfpint_ReadKey();

    switch (c)
    {
    case '\r': // enter
        insertNewline();
        break;

    case CTRL_('q'):
        if (!editor.saved && quit_times > 0)
        {
            Halfpint_SetStatusMessage("WARNING: File has unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
            quit_times--;
            break;
        }

        write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
        write(STDOUT_FILENO, "\x1b[H", 3);  // position the cursor to top left

        exit(0);
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
        // go to normal mode if mode is insert
        if (editor.mode == mode_insert)
            editor.mode = mode_normal;
        break;

    // refresh screen
    // NOTE: screen refreshs after keypress
    case CTRL_('l'):
        break;

    //!! this needs to be last
    // because we want to write the letters
    // hjkl and i if we're in insert mode
    case UP:
    case DOWN:
    case LEFT:
    case RIGHT:
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

    default:
        if (editor.mode == mode_insert)
            insertChar(c);
        break;
    }
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
