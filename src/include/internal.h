#ifndef dos_internal_h
#define dos_internal_h

#include "conio.h"
#include <SDL2/SDL.h>

#define TXTBUFSIZE  (textinfo.screenwidth * textinfo.screenheight * 2)
#define INBUFSIZE  0x80
#define MODE40H     8
#define MODE80H     16

/* text buffer cell macros */
#define CELL_CH(cell)  ((cell) & 0x00FF)
#define CELL_BG(cell)  (((cell) & 0xF000) >> 12)
#define CELL_FG(cell)  (((cell) & 0x0F00) >> 8)

// input buffer
typedef struct
{
    int top;
    int buffer[INBUFSIZE];
} inbuf_t;

extern const unsigned char fontdata40[];
extern const unsigned char fontdata80[];
const unsigned char * fontdata; /* current data, 40 or 80 */

extern SDL_Renderer * renderer;

extern inbuf_t kb;
extern inbuf_t _mousebuf;

extern short *      _textbuffer;
extern TEXT_INFO    textinfo;
extern const int    _textwidth;
extern int          _textheight;

extern unsigned char bordersize;

extern int _base;

/* graphics.c */

extern int bkcolor;

int  _clamp(int x, int min, int max);
int  _scale();
void _setcga(COLOR c);
void _drawchar(short cell, int x, int y);

short * _txtbufcell(int x, int y);
short * _curtxtbufcell(void);
int _maxtextx();
int _maxtexty();

#endif /* dos_internal_h */
