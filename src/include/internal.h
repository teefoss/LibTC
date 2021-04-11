#ifndef dos_internal_h
#define dos_internal_h

#include "conio.h"
#include <stdbool.h>
#include <SDL2/SDL.h>

#define VERSION     "0.1"

#define TXTBUFSIZE  (text.info.screenwidth * text.info.screenheight * 2)
#define STACK_SIZE  0x80
#define MODE40H     8
#define MODE80H     16

/* text buffer cell macros */
/* TODO: this should be the other way round */
#define CELL_CH(cell)  ((cell) & 0x00FF)
#define CELL_BG(cell)  (((cell) & 0xF000) >> 12)
#define CELL_FG(cell)  (((cell) & 0x0F00) >> 8)

#define WINX2ABS(x) ((x) + text.info.winleft - 1)
#define WINY2ABS(y) ((y) + text.info.wintop - 1)

typedef enum { NO, YES } BOOL;
typedef unsigned char   uchar;
typedef unsigned short  ushort;

typedef struct
{
    int top;
    int data[STACK_SIZE];
} STACK;

typedef struct
{
    short * buf;
    TEXT_INFO info;
    int char_w;
    int char_h;
} text_t;

extern SDL_Renderer * renderer;

extern STACK    keybuf;
extern STACK    mousebuf;
extern text_t   text;
extern uchar    bordersize;
extern int      bkcolor;
/*extern VIEWPORT vp;*/

BOOL    dos_push(STACK * s, int data);
int     dos_pop(STACK * s);
void    dos_empty(STACK * s);

int     dos_clamp(int x, int min, int max);
int     dos_scale(void);
void    dos_setcga(COLOR c);
void    dos_drawchar(short cell, int x, int y);
void    dos_refresh_region(int x, int y, int w, int h);
short * dos_cell(int x, int y); /* text buf cell at x, y */
short * dos_currentcell(void); /* text buf cell for current cursor */
int     dos_maxx(); /* text window max x value */
int     dos_maxy();

#endif /* dos_internal_h */
