#include "include/Halfpint.h"

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

    Halfpint_UpdateSyntax(row);
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
    editor.erows[at].hl = NULL;

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

