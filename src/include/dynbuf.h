#pragma once

#include <stdlib.h>

// dynamic buffer
struct dynbuf
{
    char *b; // buffer text
    char *render; // the characters to render
    int len; // length of b in characters
    int rlen; // length of render characters
};

void dynbuf_Append(struct dynbuf *buffer, const char *s, int len);
void dynbuf_Free(struct dynbuf *buffer);

#define INIT_DYNBUF calloc(1, sizeof(struct dynbuf)) 
