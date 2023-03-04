#include "include/dynbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dynbuf_Append(struct dynbuf *buffer, const char *s, int len)
{
    char *new = realloc(buffer->b, buffer->len + len);
    if (new == NULL)
    {
        fprintf(stdout, "Couldn't append '%s' to buffer\n", s); 
        return;
    }

    memcpy(&new[buffer->len], s, len); // Copies s to the end of new
    buffer->b = new;
    buffer->len += len;
}

void dynbuf_Free(struct dynbuf *buffer)
{
    if (buffer->b != NULL)
    {
        free(buffer->b);
        buffer->b = NULL;
        buffer->len = 0;
    }
}
