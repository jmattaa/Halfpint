#include "include/Halfpint.h"

const char *C_hl_filetypes[] = {".c", ".cpp", ".h", ".hpp", NULL};
Halfpint_SyntaxDef Halfpint_HLDB[] = 
{
    {
        "c",
        C_hl_filetypes,
        hl_number,
    }
};

int Halfpint_HLDB_Entries = 1;

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

    int prev_sep = 1;
    int i = 0;
    while (i < row->rlen)
    {
        char c = row->render[i];

        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : hl_normal;

        // highlight number if syntax highlights number
        if (editor.syntax->flags & hl_number) {
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
        case hl_number:
            return 31; // fg number red
        case hl_match:
            return 34; // fg match blue
        default:
            return 37; // fg default white 
    }
}

