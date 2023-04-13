#include "include/Halfpint.h"


void drawStatusbar(Halfpint *halfpint)
{
    dynbuf_Append(halfpint->buffer, "\x1b[40;33m", 8); // colors
    
    char status[64], renderstatus[64];
    int len = snprintf(
            status, 
            sizeof(status), 
            "%.20s -- %s --  %s", // %.20s max 20 cols
            halfpint->filename ? halfpint->filename : "[No Name]", // if the filename is NULL display no name
            Halfpint_Modename(halfpint),
            halfpint->saved? "" : "[+]");


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

void Halfpint_RenderScreen(Halfpint *halfpint)
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
    // +1 because we have 1 spaces after line number
    int curxpos = ((halfpint->renderX - halfpint->coloffset) + 1) + halfpint->rownumdig + 1;

    // top left starts at 1, 1
    snprintf(buf, sizeof(buf),"\x1b[%d;%dH", curypos, curxpos); 
    dynbuf_Append(halfpint->buffer, buf, strlen(buf));

    dynbuf_Append(halfpint->buffer, "\x1b[?25h", 6); // show the cursor 

    // set cursor shape
    switch (halfpint->mode)
    {
    case mode_normal:
        // blinking block
        dynbuf_Append(halfpint->buffer, "\x1b[\x30 q", 5);
        break;
    case mode_insert:
        // blinking bar
        dynbuf_Append(halfpint->buffer, "\x1b[\x35 q", 5);
        break;
    }
    
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
            char linenum[128];
            int numlen = snprintf(linenum, sizeof(linenum), "%*d ", halfpint->rownumdig, currentrow + 1); // make the string be the max amount of character width

            dynbuf_Append(halfpint->buffer, "\x1b[90m", 5);
            dynbuf_Append(halfpint->buffer, linenum, numlen); // line number
            dynbuf_Append(halfpint->buffer, "\x1b[0m", 4);

            int len = halfpint->erows[currentrow].rlen - halfpint->coloffset;
            if (len < 0) len = 0;
            if (len > halfpint->cols - numlen) len = halfpint->cols - numlen; // this numlen is to keep line rendering corectly

            dynbuf_Append(halfpint->buffer, &halfpint->erows[currentrow].render[halfpint->coloffset], len);
        }

        dynbuf_Append(halfpint->buffer, "\x1b[K", 3); // clear line
        dynbuf_Append(halfpint->buffer, "\r\n", 2);
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
