#include "dos.h"
#include "conio.h"
#include "internal.h"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define CLAMP(x,low,high)  (((x)>(high))?(high):(((x)<(low))?(low):(x)))

STACK keybuf;
STACK mousebuf;

/* move line at y = 'from' to y = 'to' */
/* from, to in win coords */
static void move_line(int from, int to)
{
    short   *dst, *src;
    int     x, w;
    
    x = WINX2ABS(text.info.curx) - 1;
    src = dos_cell(x, WINY2ABS(from) - 1);
    dst = dos_cell(x, WINY2ABS(to) - 1);
    
    w = text.info.winright - text.info.winleft + 1;
    for ( x = 0; x < w; x++ ) {
        *dst++ = *src++;
    }
    
    refresh_region(WINX2ABS(text.info.curx), WINY2ABS(from), w, 1);
    refresh_region(WINX2ABS(text.info.curx), WINY2ABS(to),   w, 1);
}


static void newline()
{
    text.info.curx = 1;
    
    if ( text.info.cury < dos_maxy() ) {
        text.info.cury++;
    }
}


static void advancecursor(int amount)
{
    text.info.curx += amount;
    
    if ( text.info.curx > dos_maxx() ) {
        newline();
    }
}


static void tab() /* TODO: tab size? */
{
    while ( text.info.curx % 4 != 0 ) {
        advancecursor(1);
    }
}


void clreol(void)
{
    short *cell;
    int count;
    int x, y;
    int i;
    
    cell = dos_currentcell();
    x = text.info.curx + text.info.winleft - 2;
    y = text.info.cury + text.info.wintop - 2;
    count = text.info.winright - text.info.winleft + 1;
    
    for ( i = 0; i < count; i++, cell++, x++ ) {
        *cell &= 0xFF00; /* clear char info */
        dos_drawchar(*cell, x, y);
    }
}


void clrscr()
{
    short *cell;
    int x, y, w, h;
    
    for ( y = text.info.wintop; y <= text.info.winbottom; y++ ) {
        for ( x = text.info.winleft; x <= text.info.winright; x++ ) {
            gotoxy(x, y);
            cell = dos_currentcell();
            *cell = LIGHTGRAY << 8;
        }
    }
    
    text.info.attribute = LIGHTGRAY;
    text.info.curx = 1;
    text.info.cury = 1;
    
    /*dos_setcga(BLACK);*/
    /*SDL_RenderClear(renderer); // target is screen texture */
    w = text.info.winright - text.info.winleft + 1;
    h = text.info.winbottom - text.info.wintop + 1;
    refresh_region(text.info.winleft, text.info.wintop, w, h);
}


void delline()
{
    short *     cell;
    int         max_row;
    int         x, y, w;
    int         x1;
    
    /* move all following rows up by one */
    max_row = dos_maxy();
    for ( y = text.info.cury; y < max_row; y++ ) {
        move_line(y + 1, y);
    }
    
    /* clear the last line */
    y = text.info.winbottom - 1;
    x = text.info.winleft - 1;
    cell = dos_cell(x, y);
    w = dos_maxx();
    for ( x1 = x; x1 < x + w; x1++ ) {
        *cell = text.info.attribute << 8;
        dos_drawchar(*cell, x1, y);
        cell++;
    }
}


/* check gettext and puttext coords */
static BOOL bad_coords(int left, int top, int right, int bottom)
{
    if (left < 1
        || top < 1
        || right > text.info.screenwidth
        || bottom > text.info.screenheight) {
        return YES;
    }
    
    if ( left > right || top > bottom ) {
        return YES;
    }
    
    return NO;
}


int gettext(int left, int top, int right, int bottom, void *destin)
{
    void *check;
    short *cell;
    int x, y; /* buffer indices */
    int w, h;
    int i;
    
    if ( bad_coords(left, top, right, bottom) ) {
        return 0;
    }
    
    x = left - 1;
    y = top - 1;
    w = right - left + 1;
    h = bottom - top + 1;
    cell = dos_cell(x, y);
    
    for ( i = 0; i < h; i++ ) {
        if ( memcpy(destin, (void *)cell, w * sizeof(short)) != destin ) {
            return 0;
        }
        
        cell += text.info.screenwidth;
        destin = (char *)destin + w * 2;
    }
    
    return 1;
}


void gettextinfo(TEXT_INFO *r)
{
    *r = text.info;
}


void gotoxy(int x, int y)
{
    text.info.curx = x;
    text.info.cury = y;
}


void highvideo(void)
{
    text.info.attribute |= 0x8;
}

/* FIXME: ?*/
void insline(void)
{
    int x, y, w, x1;
    short * cell;
    
    for ( y = dos_maxy() - 1; y >= text.info.cury; --y ) {
        move_line(y, y + 1);
    }
    
    /* delete current line */
    
    y = text.info.wintop - 1;
    x = text.info.winleft - 1;
    w = dos_maxx();
    cell = dos_cell(x, y);
    
    for ( x1 = x; x1 < w; x1++ ) {
        *cell &= 0xFF00;
        dos_drawchar(*cell, x1, y);
        cell++;
    }
}


void lowvideo(void)
{
    text.info.attribute &= ~0x8;
}


int
movetext(int left, int top, int right, int bottom, int destleft, int desttop)
{
    int w, h;
    short *buf;
    int result;
    
    w = right - left + 1;
    h = bottom - top + 1;
    
    buf = malloc(sizeof(*buf) * w * h);
    
    result = gettext(left, top, right, bottom, buf);
    if ( result == 0 ) {
        return 0;
    }
    
    result = puttext(destleft, desttop, destleft + w - 1, desttop + h - 1, buf);
    
    return result;
}


void normvideo(void)
{
    text.info.attribute = LIGHTGRAY;
}


int puttext(int left, int top, int right, int bottom, void *source)
{
    int x, y, r, b; /* buffer indices */
    int x1, y1;
    short *src_cell, *dst_cell;
    int i;
    int result;
    
    if ( bad_coords(left, top, right, bottom) ) {
        return 0;
    }
    
    src_cell = (short *)source;
    x = left - 1;
    y = top - 1;
    r = right - 1;
    b = bottom - 1;

    result = 0;
    for ( y1 = y; y1 <= b; y1++ ) {
        for ( x1 = x; x1 <= r; x1++ ) {
            dst_cell = dos_cell(x1, y1);
            *dst_cell = *src_cell++;
            dos_drawchar(*dst_cell, x1, y1);
            result++;
        }
    }
        
    return result;
}


void textattr(int newattr)
{
    text.info.attribute = newattr;
}


void textbackground(int newcolor)
{
    text.info.attribute &= 0x0F;
    text.info.attribute |= (newcolor << 4);
}


void textcolor(int newcolor)
{
    text.info.attribute &= 0xF0;
    text.info.attribute |= newcolor;
}


void textmode(int newmode)
{    
    switch ( newmode ) {
        case BW40:
        case C40:
            text.char_h = MODE40H;
            text.info.screenwidth = 40;
            break;
        case BW80:
        case C80:
            text.char_h = MODE80H;
            text.info.screenwidth = 80;
            break;
        default:
            return; /* bad mode */
    }
    
    text.info.screenheight = 25;
    text.info.currmode = newmode;
    window(1, 1, text.info.screenwidth, text.info.screenheight);
    clrscr();
}


int  wherex(void)
{
    return text.info.curx;
}


int  wherey(void)
{
    return text.info.cury;
}


void window(int left, int top, int right, int bottom)
{
    if ( bad_coords(left, top, right, bottom) ) {
        return;
    }
    
    text.info.winleft   = left;
    text.info.wintop    = top;
    text.info.winright  = right;
    text.info.winbottom = bottom;
}


int mousex(void)
{
    int x;
    int maxx;
    
    SDL_GetMouseState(&x, NULL);
    x /= dos_scale();
    x -= bordersize;
    maxx = text.info.screenwidth * text.char_w - 1;
    x = dos_clamp(x, 0, maxx);
    
    return x;
}


int mousey(void)
{
    int y, maxy;
    
    SDL_GetMouseState(NULL, &y);
    y /= dos_scale();
    y -= bordersize;
    maxy = text.info.screenheight * text.char_h - 1;
    y = dos_clamp(y, 0, maxy);
    
    return y;
}


int kbhit()
{
    return keybuf.top + 1;
}


int mousehit()
{
    return mousebuf.top + 1;
}


int getch()
{
    if ( kbhit() ) {
        return dos_pop(&keybuf);
    }
    /* TODO: wait for key */
    return EOF;
}


int getche(void)
{
    int key;
    
    if ( kbhit() ) {
        key = dos_pop(&keybuf);

        switch ( key ) {
            case '\r':
                text.info.curx = 1;
                break;
            case '\b':
                text.info.curx--;
                if ( text.info.curx < 1 ) {
                    text.info.curx = 1;
                }
                break;
            default:
                if ( isprint(key) ) {
                    putch(key);
                }
                break;
        }
        return key;
    }
    
    return EOF;
}



int getmouse()
{
    if ( mousebuf.top >= 0 ) {
        return dos_pop(&mousebuf);
    }
    
    return 0;
}


char *cgets(char *str)
{
    int key = 0;
    int c = 0;
    
    while ( 1 ) {
        if ( kbhit() ) {
            key = getch();
            if ( key == '\r' ) {
                str[c] = '\0';
                return str;
            }
            putch(key);
            str[c++] = key;
        }
            
        refresh();
    }
}


int cprintf(const char *format, ...)
{
    const int size = 0x80;
    va_list args;
    char *buffer;
    int ret;
    
    buffer = calloc(size, sizeof(char));
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    ret = cputs(buffer);
    va_end(args);
    
    free(buffer);
    return ret;
}


int cputs(const char *str)
{
    const char *c = str;
    int ret = -1;
    
    while ( *c )
    {
        switch ( *c ) {
            case '\n':
                newline();
                break;
            case '\t':
                tab();
                break;
            default:
                ret = putch(*c);
                break;
        }
        c++;
    }
    
    return ret;
}


int putch(int c)
{
    short * cell;
    int bufx, bufy;

    bufx = WINX2ABS(text.info.curx) - 1;
    bufy = WINY2ABS(text.info.cury) - 1;
        
    cell = &text.buf[bufy * text.info.screenwidth + bufx];
    *cell = (unsigned char)c;
    *cell |= text.info.attribute << 8;
    
    dos_drawchar(*cell, bufx, bufy);
    advancecursor(1);
    
    return c;
}


int ungetch(int ch)
{
    return dos_push(&keybuf, ch) ? ch : EOF;
}


short *textbuffer()
{
    return text.buf;
}
