#include "include/Halfpint.h"
#include "include/dynbuf.h"

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

Halfpint *Halfpint_Init()
{
    Halfpint *halfpint = calloc(1, sizeof(struct halfpint));

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

    halfpint->saved = 1;

    return halfpint;
}

// put render chars in render 
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

void insertRow(Halfpint *halfpint, int at, char *s, size_t len)
{
    if (at < 0 || at > halfpint->rownum) return;

    halfpint->erows = realloc(halfpint->erows, sizeof(struct dynbuf) * (halfpint->rownum + 1));
    memmove(&halfpint->erows[at + 1], &halfpint->erows[at], sizeof(struct dynbuf) * (halfpint->rownum - at));

    halfpint->erows[at].len = len;
    halfpint->erows[at].b = malloc(len + 1);
    
    memcpy(halfpint->erows[at].b, s, len);
    halfpint->erows[at].b[len] = '\0'; 

    halfpint->erows[at].rlen = 0;
    halfpint->erows[at].render = NULL;
    
    updateRow(halfpint, &halfpint->erows[at]);

    halfpint->rownum++; 

    // + 1 will give the amount digits in the number + 1 again to add padding 
    halfpint->rownumdig = log10(halfpint->rownum + 1) + 1;

    halfpint->saved = 0;
}

void rowInsertChar(Halfpint *halfpint, struct dynbuf *row, int at, char c)
{
    if (at < 0 || at > row->len) at = row->len;

    row->b = realloc(row->b, row->len + 2); // make room for '\0' and c 
    memmove(&row->b[at + 1], &row->b[at], row->len - at + 1);

    row->len++;
    row->b[at] = c; // insert char

    updateRow(halfpint, row);

    halfpint->saved = 0;
}

void insertNewline(Halfpint *halfpint)
{
    if (halfpint->cursorX == 0)
    {
        insertRow(halfpint, halfpint->cursorY, "", 0);

        halfpint->cursorY++;
        halfpint->cursorX = 0;

        return;
    }

    // split line into to lines
    struct dynbuf *row = &halfpint->erows[halfpint->cursorY];
    insertRow(halfpint, halfpint->cursorY + 1, &row->b[halfpint->cursorX], row->len - halfpint->cursorX);
    row = &halfpint->erows[halfpint->cursorY];
    row->len = halfpint->cursorX;
    row->b[row->len] = '\0';
    updateRow(halfpint, row);

    halfpint->cursorY++;
    halfpint->cursorX = 0;
}

void insertChar(Halfpint *halfpint, char c)
{
    if (halfpint->cursorY == halfpint->rownum) // insert a newline after file
        insertRow(halfpint, halfpint->rownum, "", 0);

    rowInsertChar(halfpint, &halfpint->erows[halfpint->cursorY], halfpint->cursorX, c);
    halfpint->cursorX++;
}

char *rowsToString(Halfpint *halfpint, int *buflen)
{
    int totlen = 0;
    int j;
    
    for (j = 0; j < halfpint->rownum; j++)
        totlen += halfpint->erows[j].len + 1;
    
    *buflen = totlen;
    char *buf = malloc(totlen);
    char *p = buf;

    for (j = 0; j < halfpint->rownum; j++) 
    {
        memcpy(p, halfpint->erows[j].b, halfpint->erows[j].len);
        p += halfpint->erows[j].len;
        *p = '\n';
        p++;
    }

    return buf;
}

void rowAppendString(Halfpint* halfpint, struct dynbuf *row, char *s, int len)
{
    row->b = realloc(row->b, row->len + len + 1);
    memcpy(&row->b[row->len], s, len);

    row->len += len;
    row->b[row->len] = '\0';
    
    updateRow(halfpint, row);

    halfpint->saved = 0;
}

void rowDelChar(Halfpint *halfpint, struct dynbuf *row, int at)
{
    if (at < 0 || at >= row->len) return; // Invalid at

    memmove(&row->b[at], &row->b[at + 1], row->len - at);

    row->len--;
    updateRow(halfpint, row);

    halfpint->saved = 0;
}

void delRow(Halfpint* halfpint, int at)
{
    if (at < 0 || at >= halfpint->rownum) return; // Invalid at
    
    dynbuf_Free(&halfpint->erows[at]);
    memmove(&halfpint->erows[at], &halfpint->erows[at + 1], sizeof(struct dynbuf) * (halfpint->rownum - at - 1));

    halfpint->rownum--;

    halfpint->saved = 0;
}


void delChar(Halfpint *halfpint)
{
    if (halfpint->cursorY == halfpint->rownum) return;
    if (halfpint->cursorY == 0 && halfpint->cursorX == 0) return;

    struct dynbuf *row = &halfpint->erows[halfpint->cursorY];
    
    if (halfpint->cursorX <= 0) // delete row
    {
        halfpint->cursorX = halfpint->erows[halfpint->cursorY - 1].len;
        rowAppendString(halfpint, &halfpint->erows[halfpint->cursorY - 1], row->b, row->len);

        delRow(halfpint, halfpint->cursorY);
        halfpint->cursorY--;

        return;
    }

    rowDelChar(halfpint, row, halfpint->cursorX - 1);
    halfpint->cursorX--;
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
        insertRow(halfpint, halfpint->rownum, line, linelen);
    }

    free(line);
    fclose(fp);

    halfpint->saved = 1;
}

void Halfpint_Save(Halfpint *halfpint)
{
    if (halfpint->filename == NULL)
        // TODO let create file
        return;

    int len;
    char *buffer = rowsToString(halfpint, &len);

    int fd = open(halfpint->filename, O_RDWR | O_CREAT, 0644); // 0644 - owner can read and write and others can only read 

    if (fd != -1)
    {
        if (ftruncate(fd, len) != -1)
        {
            if (write(fd, buffer, len) == len)
            {
                close(fd);
                free(buffer);
                Halfpint_SetStatusMessage(halfpint, "%dB written to '%s'", len, halfpint->filename);

                halfpint->saved = 1;

                return;
            }
        }
        close(fd);
    }
    
    free(buffer);
    Halfpint_SetStatusMessage(halfpint, "Couldn't save ERR: %s", strerror(errno));
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

            // Uncomment to move line when we press right
            //else if (halfpint->cursorY > 0) // we're not at last line
            //{
            //   // go to end of last line
            //    halfpint->cursorY--;
            //    halfpint->cursorX = halfpint->erows[halfpint->cursorY].len;
            //}
            break;
        case DOWN: 
            if (halfpint->cursorY < halfpint->rownum) // so we can scroll until end of screen
                halfpint->cursorY++;
            break;
        case UP: 
            if (halfpint->cursorY != 0)
                halfpint->cursorY--;
            break;
        case RIGHT: 
            if (currentrow && halfpint->cursorX < currentrow->len) // so we dont move past than line length
                halfpint->cursorX++; 

            // Uncomment to move line when we press right
            //else if (currentrow && halfpint->cursorX == currentrow->len) // we're at the end of the line
            //
            //    // go to begining of next line
            //    halfpint->cursorY++;
            //    halfpint->cursorX = 0;
            //}
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

int Halfpint_ReadKey(Halfpint *g_halfpint)
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

void Halfpint_ProcessKeypress(Halfpint *halfpint)
{
    static int quit_times = 2;

    int c = Halfpint_ReadKey(halfpint);

    switch (c) 
    {
    case '\r': // enter
        insertNewline(halfpint);
        break;

    case CTRL_('q'):
        if (!halfpint->saved && quit_times > 0) 
        {
            Halfpint_SetStatusMessage(halfpint, "WARNING: File has unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
            quit_times--;
            break;
        }

        write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
        write(STDOUT_FILENO, "\x1b[H", 3); // position the cursor to top left
    
        exit(0);
        break;

    case CTRL_('s'):
        Halfpint_Save(halfpint);

    case FIRST:
    case END:
        Halfpint_MoveCursor(halfpint, c);
        break;

    case backspace_key:
        // delete character if in insert mode
        if (halfpint->mode == mode_insert)
            delChar(halfpint);
        else if (halfpint->mode == mode_normal) // move cursor if normal mode 
            Halfpint_MoveCursor(halfpint, LEFT);
        break;

    case UP_ARR:
        Halfpint_MoveCursor(halfpint, UP);
        break;
    case DOWN_ARR:
        Halfpint_MoveCursor(halfpint, DOWN);
        break;
    case LEFT_ARR:
        Halfpint_MoveCursor(halfpint, LEFT);
        break;
    case RIGHT_ARR: 
        Halfpint_MoveCursor(halfpint, RIGHT);
        break;

    case escape_key:
        // go to normal mode if mode is insert
        if (halfpint->mode == mode_insert)
            halfpint->mode = mode_normal;
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
        if (halfpint->mode == mode_normal)
        {
            Halfpint_MoveCursor(halfpint, c);
            break;
        } 
    // write i 
    case 'i':
        // go to insert mode if the mode is normal
        if (halfpint->mode == mode_normal)
        {
            halfpint->mode = mode_insert;
            break;
        }

    default: 
        if (halfpint->mode == mode_insert)
            insertChar(halfpint, c);
        break;
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

