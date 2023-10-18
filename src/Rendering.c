#include "include/Halfpint.h"

void drawStatusbar()
{
    dynbuf_Append(editor.buffer, "\x1b[40;33m", 8); // colors

    char status[64], renderstatus[64];
    int len = snprintf(
        status,
        sizeof(status),
        "%.20s -- %s --  %s",                            // %.20s max 20 cols
        editor.filename ? editor.filename : "[No Name]", // if the filename is NULL display no name
        Halfpint_Modename(),
        editor.saved ? "" : "[+]");

    int fprecent = ((float)(editor.cursorY + 1) / (float)editor.rownum) * 100.0f; // get precentage of file

    int renderlen = snprintf(
        renderstatus,
        sizeof(renderstatus),
        "[%s]   %d:%d  -  %d%%",   // show filetype, current position and precentage of file we're in
        editor.syntax? editor.syntax->filetype : "no filetype", // show current filetype
        editor.cursorY + 1, // current line we're on
        editor.cursorX + 1, // current column
        fprecent);

    dynbuf_Append(editor.buffer, status, len);

    // draw inverted colors line
    while (len < editor.cols)
    {
        // write renderstatus at end of line
        if (editor.cols - len == renderlen)
        {
            dynbuf_Append(editor.buffer, renderstatus, renderlen);
            break;
        }

        dynbuf_Append(editor.buffer, " ", 1);
        len++;
    }

    dynbuf_Append(editor.buffer, "\x1b[m", 3); // reset colors
    dynbuf_Append(editor.buffer, "\r\n", 2);   // go to statusmsg
}

void drawMessage()
{
    dynbuf_Append(editor.buffer, "\x1b[K", 3); // clear the message bar

    int msglen = strlen(editor.statusmsg);
    if (msglen > editor.cols)
        msglen = editor.cols; // cut message

    if (msglen && time(NULL) - editor.statusmsg_time < 5) // let message display for 5 seconds

        dynbuf_Append(editor.buffer, editor.statusmsg, msglen);
}

void Halfpint_RenderScreen()
{
    Halfpint_ScrollEditor();

    dynbuf_Append(editor.buffer, "\x1b[?25l", 6); // hide cursor
    dynbuf_Append(editor.buffer, "\x1b[H", 3);    // position the cursor to top left

    Halfpint_DrawRows();
    drawStatusbar();
    drawMessage();

    // position the cursor to cursorX and cursorY
    char buf[32]; // buffer is 32 chars because there could be big numbers in the cursor position

    // translate file position to screen position and start from after the line number on x axis
    int curypos = ((editor.cursorY - editor.rowoffset) + 1);
    // +1 because we have 1 spaces after line number
    int curxpos = ((editor.renderX - editor.coloffset) + 1) + editor.rownumdig + 1;


    // top left starts at 1, 1
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", curypos, curxpos);
    dynbuf_Append(editor.buffer, buf, strlen(buf));

    dynbuf_Append(editor.buffer, "\x1b[?25h", 6); // show the cursor

    // set cursor shape
    switch (editor.mode)
    {
    case mode_insert:
        // blinking bar
        dynbuf_Append(editor.buffer, "\x1b[\x35 q", 5);
        break;
    // if not insert 
    // we do blinking block
    default:
        // blinking block
        dynbuf_Append(editor.buffer, "\x1b[\x30 q", 5);
        break;
    }

    write(STDOUT_FILENO, editor.buffer->b, editor.buffer->len);
    dynbuf_Free(editor.buffer);
}

void Halfpint_DrawRows()
{
    char welcome[80];
    int welcomelen = snprintf(welcome, sizeof(welcome), "Halfpint Editor -- Version: %s", HALFPINT_VERSION);
    if (welcomelen > editor.cols)
        welcomelen = editor.cols; // if message is to long cut it
    int padding = (editor.cols - welcomelen) / 2;

    for (int filerow = 0; filerow < editor.rows; filerow++)
    {
        int currentrow = filerow + editor.rowoffset;
        if (currentrow >= editor.rownum)
        {
            // display welcome if there is no file to open or file is empty
            if (editor.rownum == 0 && filerow == editor.rows / 2)
            {
                // center the message
                if (padding)
                {
                    dynbuf_Append(editor.buffer, "~", 1);
                    padding--;
                }
                while (padding--)
                    dynbuf_Append(editor.buffer, " ", 1); // padd the text to the center with spaces

                dynbuf_Append(editor.buffer, welcome, welcomelen);
            }
            else
                dynbuf_Append(editor.buffer, "~", 1);
        }
        else
        {
            char linenum[128];
            int numlen = snprintf(linenum, sizeof(linenum), "%*d ", editor.rownumdig, currentrow + 1); // make the string be the max amount of character width

            dynbuf_Append(editor.buffer, "\x1b[90m", 5);
            dynbuf_Append(editor.buffer, linenum, numlen); // line number
            dynbuf_Append(editor.buffer, "\x1b[0m", 4);

            int len = editor.erows[currentrow].rlen - editor.coloffset;
            if (len < 0)
                len = 0;
            if (len > editor.cols - numlen)
                len = editor.cols - numlen; // this numlen is to keep line rendering corectly

            char *c = &editor.erows[currentrow].render[editor.coloffset]; // get the line in a string 
            unsigned char *hl = &editor.erows[currentrow].hl[editor.coloffset]; // get how the line is highlighted
            int currentcolor = -1;
            for (int j = 0; j < len; j++)
            {
                if (hl[j] == hl_normal)
                {
                    if (currentcolor != -1)
                    {
                        dynbuf_Append(editor.buffer, "\x1b[39m", 5);
                        currentcolor = -1;
                    }

                    dynbuf_Append(editor.buffer, &c[j], 1);
                }
                else 
                {
                    // get the syntax as a color
                    int color = Halfpint_SyntaxToColors(hl[j]);
                    
                    if (color != currentcolor)
                    {
                        currentcolor = color;
                        char buf[16];
                        // color the character 
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        dynbuf_Append(editor.buffer, buf, clen);
                    }
                    dynbuf_Append(editor.buffer, &c[j], 1);
                }
            }
            dynbuf_Append(editor.buffer, "\x1b[39m", 5); // reset the hl
        }

        dynbuf_Append(editor.buffer, "\x1b[K", 3); // clear line
        dynbuf_Append(editor.buffer, "\r\n", 2);
    }
}

