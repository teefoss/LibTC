#ifndef dos_internal_h
#define dos_internal_h

#include "conio.h"
#include <SDL2/SDL.h>

#define TXTBUFSIZE  (text.info.screenwidth * text.info.screenheight * 2)
#define QUEUE_SIZE  0x80
#define MODE40H     8
#define MODE80H     16

/* text buffer cell macros */
#define CELL_CH(cell)  ((cell) & 0x00FF)
#define CELL_BG(cell)  (((cell) & 0xF000) >> 12)
#define CELL_FG(cell)  (((cell) & 0x0F00) >> 8)

typedef enum { NO, YES } BOOL;
typedef unsigned char   uchar;
typedef unsigned short  ushort;

typedef struct
{
    int count;
    int data[QUEUE_SIZE];
} queue_t;

typedef struct
{
    short * buf;
    TEXT_INFO info;
    int char_w;
    int char_h;
} text_t;

extern SDL_Renderer * renderer;

extern queue_t  keybuf;
extern queue_t  mousebuf;
extern text_t   text;
extern uchar    bordersize;
extern int      base;
extern int      bkcolor;

int     dos_clamp(int x, int min, int max);
int     dos_scale(void);
void    dos_setcga(COLOR c);
void    dos_drawchar(short cell, int x, int y);
short * dos_cell(int x, int y); /* text buf cell at x, y */
short * dos_currentcell(void); /* text buf cell for current cursor */
short * coord_to_cell(int x, int y);
int     dos_maxx(); /* text buffer cell x value */
int     dos_maxy();

#endif /* dos_internal_h */
