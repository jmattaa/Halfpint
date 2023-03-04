#pragma once

#include "dynbuf.h"
#include <termios.h>

#define CTRL_(k) ((k) & 0x1f) // combine control with key

typedef struct halfpint
{
    int cols, rows;
    int cursorX, cursorY;
    struct termios g_termios;
    struct dynbuf *buffer;
} Halfpint;

void Halfpint_Init(Halfpint *halfpint);
void die(const char *s); 
void Halfpint_DisableRawMode(Halfpint *g_halfpint);
void Halfpint_EnableRawMode(Halfpint *g_halfpint);
void Halfpint_MoveCursor(Halfpint *halfpint, char key);
char Halfpint_ReadKey(Halfpint *g_halfpint);
void Halfpint_ProcessKeypress(Halfpint *halfpint);
void Halfpint_RefreshScreen(Halfpint *halfpint);
void Halfpint_DrawRows(Halfpint *halfpint);
int Halfpint_GetWindowSize(Halfpint *halfpint);

#define HALFPINT_VERSION "0.0.1"

