#pragma once

#include "dynbuf.h"
#include <termios.h>
#include <time.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#define CTRL_(k) ((k)&0x1f) // combine control with key

#define UP 'k'
#define DOWN 'j'
#define LEFT 'h'
#define RIGHT 'l'

#define FIRST CTRL_('f')
#define END CTRL_('e')

#define UP_ARR 1000
#define DOWN_ARR 1100
#define LEFT_ARR 1200
#define RIGHT_ARR 1300

#define escape_key 0x1b
#define backspace_key 0x7f

#define HALFPINT_TABSTOP 8

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
    int rownumdig; // the amount of digits in rownum
    struct dynbuf *erows; // editor rows
    int rowoffset, coloffset; // where the user is scrolled to in file

    char *filename; // the open file's name
    Halfpint_Mode mode; // the mode we're in (normal/insert) 
    
    char statusmsg[64]; // text underneeth the statusbar dissapiears
    time_t statusmsg_time; // time of statusmsg 
    
    unsigned saved:1; // flag which keeps track wether file is saved or not
} Halfpint;

// gets the mode in a string
char *Halfpint_Modename(Halfpint *halfpint);

// initializes an editor
Halfpint *Halfpint_Init();

// opens a file in the editor
void Halfpint_OpenEditor(Halfpint *halfpint, char *filename);

// saves the file to disk
void Halfpint_Save(Halfpint *halfpint);

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
int Halfpint_ReadKey(Halfpint *g_halfpint);

// process the key presses
void Halfpint_ProcessKeypress(Halfpint *halfpint);

// render screen by clearing and printing file
void Halfpint_RenderScreen(Halfpint *halfpint);

// print the file and draw all rows
void Halfpint_DrawRows(Halfpint *halfpint);

// gets the window size into the `(struct halfpint *)->rows and cols` 
int Halfpint_GetWindowSize(Halfpint *halfpint);

// prints a message to the statusmsg bar
void Halfpint_SetStatusMessage(Halfpint *halfpint, const char *s, ...);

#define HALFPINT_VERSION "0.0.1"
