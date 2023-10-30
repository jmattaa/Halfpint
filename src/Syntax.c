#include "include/Halfpint.h"

char *C_hl_filetypes[] = {".c", ".cpp", ".h", ".hpp", NULL};
char *TXT_hl_filetypes[] = {".txt", NULL};
Halfpint_SyntaxDef Halfpint_HLDB[] = 
{
    {
        "c",
        C_hl_filetypes,
        "//",
        HL_HIGHLIGHT_NUMBER | HL_HIGHLIGHT_STRING,
    }, 
    {
        "txt",
        TXT_hl_filetypes,
        NULL, // no singleline comment
        HL_HIGHLIGHT_NUMBER,
    },
};

int Halfpint_HLDB_Entries = 2;

// update syntax helper function
int isSeperator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void Halfpint_UpdateSyntax(struct dynbuf *row)
{
    row->hl = realloc(row->hl, row->rlen);
    memset(row->hl, hl_normal, row->rlen);

    if (editor.syntax == NULL) return; // don't highlight if there is no detected ft

    char *singleline_cmt = editor.syntax->singleline_comment_start;
    int singleline_cmt_len = singleline_cmt? strlen(singleline_cmt) : 0;

    int prev_sep = 1;
    int in_string = 0;

    int i = 0;
    while (i < row->rlen)
    {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : hl_normal;

        if (singleline_cmt_len && !in_string)
        {
            if (!strncmp(&row->render[i], singleline_cmt, singleline_cmt_len)) 
            {
                memset(&row->hl[i], hl_comment, row->rlen - i);
                break;
            }
        }

        // highlight string if we syntax string
        if (editor.syntax->flags & HL_HIGHLIGHT_STRING) 
        {
            if (in_string)
            {
                row->hl[i] = hl_string;
                // don't let escape characters close our string
                if (c == '\\' && i + 1 < row->rlen) 
                {
                    row->hl[i + 1] = hl_string;
                    i += 2;
                    continue;
                }

                if (c == in_string) in_string = 0;

                i++;
                prev_sep = 1;

                continue;
            }
            else if (c == '"' || c == '\'')
            {
                in_string = c;

                row->hl[i] = hl_string;
                i++;

                continue;
            }
        }

        // highlight number if syntax highlights number
        if (editor.syntax->flags & HL_HIGHLIGHT_NUMBER) 
        {
            if ((isdigit(c) && (prev_sep || prev_hl == hl_number)) || 
                    ((c == '.') && prev_hl == hl_number))
            {
                row->hl[i] = hl_number;
                i++;
                prev_sep = 0;
                continue;
            }
        }

        prev_sep = isSeperator(c);
        i++;
    }
}

int Halfpint_SyntaxToColors(int hl)
{
    switch (hl) 
    {
        case hl_comment:
            return 90; // gray - bright black
        case hl_string:
            return 32; // fg string green
        case hl_number:
            return 31; // fg number red
        case hl_match:
            return 34; // fg match blue
        default:
            return 37; // fg default white 
    }
}

void Halfpint_SyntaxDetect()
{
    editor.syntax = NULL;
    // newly opened file no filetype to detect
    if (editor.filename == NULL) return; 

    char *ext = strrchr(editor.filename, '.');
    if (!ext) return;
    
    for (int i = 0; i < Halfpint_HLDB_Entries; i++)
    {
        Halfpint_SyntaxDef *s = &Halfpint_HLDB[i];
        int j = 0;
        while (s->filematch[j] != NULL)
        {
            if (strcmp(ext, s->filematch[j]) == 0)
            {
                editor.syntax = s;

                // directly re-syntax if we change the filetype
                for (int r = 0; r < editor.rownum; r++)
                    Halfpint_UpdateSyntax(&editor.erows[r]);

                return;
            }

            j++;
        }
    }
}

