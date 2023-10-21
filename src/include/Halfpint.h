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

// modes enum
typedef enum
{
    mode_normal,
    mode_insert,
    mode_command
} Halfpint_Mode;


// syntax:
// struct containing the syntax highlight alternatives
typedef enum
{
    hl_normal = 0,
    hl_number,
    hl_match, // highlighting the founded string with find
} Halfpint_Highlight;

// syntax:
// struct for filetype syntax definintion
typedef struct 
{
    const char *filetype;
    const char **filematch;
    int flags;
} Halfpint_SyntaxDef;


// detect the filetype to apply syntax
void Halfpint_SyntaxDetect();

// struct that contains all information for the editor
typedef struct halfpint
{
    int cols, rows;       // screen resolution
    int cursorX, cursorY; // cursor position
    int renderX;          // cursor position in render chars

    struct termios g_termios;
    struct dynbuf *buffer; // buffer which prints text to screen

    int rownum;               // number of rows in file 
    int rownumdig;            // the amount of digits in rownum
    struct dynbuf *erows;     // editor rows
    int rowoffset, coloffset; // where the user is scrolled to in file

    char *filename;     // the open file's name
    Halfpint_Mode mode; // the mode we're in (normal/insert)

    char statusmsg[64];    // text underneeth the statusbar dissapiears
    time_t statusmsg_time; // time of statusmsg

    unsigned saved : 1; // flag which keeps track wether file is saved or not

    Halfpint_SyntaxDef *syntax; // the syntax definition for the file
} Halfpint;

// gets the mode in a string
char *Halfpint_Modename();

// initializes an editor
void Halfpint_Init();

// opens a file in the editor
void Halfpint_OpenEditor(char *filename);

// saves the file to disk
void Halfpint_Save();

// scrolls the editor when file is bigger than screen
void Halfpint_ScrollEditor();

// kills program and prints state of last function
void die(const char *s);

// dissables rawmode *use after closing program*
void Halfpint_DisableRawMode();

// enables raw mode so we have control of terminal
void Halfpint_EnableRawMode();

// moves the cursor on the screen
void Halfpint_MoveCursor(char key);

// reads keys pressed of user
int Halfpint_ReadKey();

// process the key presses
void Halfpint_ProcessKeypress();

// render screen by printing file
void Halfpint_RenderScreen();

// print the file and draw all rows
void Halfpint_DrawRows();

// gets the window size into the `editor rows and cols`
int Halfpint_GetWindowSize();

// updates the syntax of a row
void Halfpint_UpdateSyntax(struct dynbuf *row);

// translate the syntax to colors 
int Halfpint_SyntaxToColors(int hl);

// prompts the user for a query to find in the text
void Halfpint_Find();

// prompts the user for a command
void Halfpint_RunCmd();

// prints a message to the statusmsg bar
void Halfpint_SetStatusMessage(const char *s, ...);

// propmts the user from the statusmsg bar and returns the user input
char *Halfpint_Prompt(char *prompt, void (*callback)(char *, int)); 

// quits the app and asks to save if file not saved
void Halfpint_Quit();

// row functions

// put render chars in render
void updateRow(struct dynbuf *row);

void insertRow(int at, char *s, size_t len);

void rowInsertChar(struct dynbuf *row, int at, char c);

void insertNewline();

void insertChar(char c);

char *rowsToString(int *buflen);

void rowAppendString(struct dynbuf *row, char *s, int len);

void rowDelChar(struct dynbuf *row, int at);

void delRow(int at);

void delChar();

extern Halfpint editor;

#define HALFPINT_VERSION "0.1.1"
