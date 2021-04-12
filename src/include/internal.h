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
/* TODO: this should be the other way round? */
#define CH_MASK         0x00FF
#define FG_MASK         0x0F00
#define BG_MASK         0xF000
#define FG_IBIT         0x0008
#define CELL_CH(cell)  ((cell) & CH_MASK)
#define CELL_BG(cell)  (((cell) & BG_MASK) >> 12)
#define CELL_FG(cell)  (((cell) & FG_MASK) >> 8)

#define WINX2ABS(x) ((x) + text.info.winleft - 1)
#define WINY2ABS(y) ((y) + text.info.wintop - 1)

typedef enum { NO, YES } BOOL;
typedef unsigned char   uchar;
typedef unsigned short  ushort;

typedef struct { int x; int y; } bufpt_t; /* buffer point (index) */
typedef struct { int x; int y; } conpt_t; /* console point */
typedef struct { int x; int y; } winpt_t; /* console window point */

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

bufpt_t con_to_buf(conpt_t c);
conpt_t win_to_con(winpt_t w);
bufpt_t win_to_buf(winpt_t w);

int     dos_clamp(int x, int min, int max);
int     dos_scale(void);
void    dos_setcga(COLOR c);
void    dos_drawchar(short cell, bufpt_t b);
void    dos_refresh_region(conpt_t conpt, int w, int h);
short * dos_cell(bufpt_t b);
short * dos_currentcell(void); /* text buf cell for current cursor */
int     dos_maxx(); /* console window max x */
int     dos_maxy(); /* console window max y */
int     dos_cwin_w(); /* console window w */
int     dos_cwin_h(); /* console window h */

#endif /* dos_internal_h */
