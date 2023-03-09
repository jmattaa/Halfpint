#pragma once

#include "dynbuf.h"
#include <termios.h>
#include <time.h>

#define CTRL_(k) ((k)&0x1f) // combine control with key

typedef enum 
{
    mode_normal,
    mode_insert
} Halfpint_Mode;

// struct that contains all information for the editor
typedef struct halfpint
{
    int cols, rows; // screen resolution
    int cursorX, cursorY; // cursor position
    int renderX; // cursor position in render chars

    struct termios g_termios;
    struct dynbuf *buffer; // buffer which prints text to screen

    int rownum; // number of rows in exitor
    struct dynbuf *erows; // editor rows
    int rowoffset, coloffset; // where the user is scrolled to in file

    char *filename; // the open file's name
    Halfpint_Mode mode; // the mode we're in (normal/insert) 
    
    char statusmsg[64]; // text underneeth the statusbar dissapiears
    time_t statusmsg_time; // time of statusmsg 
} Halfpint;

// gets the mode in a string
char *Halfpint_Modename(Halfpint_Mode mode);

// initializes an editor
void Halfpint_Init(Halfpint *halfpint);

// opens a file in the editor
void Halfpint_OpenEditor(Halfpint *halfpint, char *filename);

// scrolls the editor when file is bigger than screen
void Halfpint_ScrollEditor(Halfpint *halfpint);

// kills program and prints state of last function
void die(const char *s);

// dissables rawmode *use after closing program*
void Halfpint_DisableRawMode(Halfpint *g_halfpint);

// enables raw mode so we have control of terminal
void Halfpint_EnableRawMode(Halfpint *g_halfpint);

// moves the cursor on the screen
void Halfpint_MoveCursor(Halfpint *halfpint, char key);

// reads keys pressed of user
char Halfpint_ReadKey(Halfpint *g_halfpint);

// process the key presses
void Halfpint_ProcessKeypress(Halfpint *halfpint);

// refresh screen by clearing and printing file
void Halfpint_RefreshScreen(Halfpint *halfpint);

// print the file and draw all rows
void Halfpint_DrawRows(Halfpint *halfpint);

// gets the window size into the `(struct halfpint *)->rows and cols` 
int Halfpint_GetWindowSize(Halfpint *halfpint);

// prints a message to the statusmsg bar
void Halfpint_SetStatusMessage(Halfpint *halfpint, const char *s, ...);

#define HALFPINT_VERSION "0.0.1"
