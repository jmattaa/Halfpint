#pragma once

#include <stdlib.h>

// dynamic buffer
struct dynbuf
{
    char *b;
    int len;
};

void dynbuf_Append(struct dynbuf *buffer, const char *s, int len);
void dynbuf_Free(struct dynbuf *buffer);

#define INIT_DYNBUF calloc(1, sizeof(struct dynbuf)) 
