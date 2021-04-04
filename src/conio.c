#include "dos.h"
#include "conio.h"
#include "internal.h"

#include <string.h>
#include <stdarg.h>

#define CLAMP(x,low,high)  (((x)>(high))?(high):(((x)<(low))?(low):(x)))

queue_t keybuf;
queue_t mousebuf;
int     modifiers;

/* move entire line at y = 'from' to y = 'to' */
static void move_line(int from, int to)
{
    short   *dst, *src;
    int     x;
    
    src = dos_cell(0, from - base);
    dst = dos_cell(0, to - base);
    
    for ( x = 0; x < text.info.screenwidth; x++ ) {
        *dst++ = *src++;
        
    }
    
    refresh_region(base, from,  text.info.screenwidth, 1);
    refresh_region(base, to,    text.info.screenwidth, 1);
}


static void newline()
{
    text.info.curx = base;
    
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
    while ( text.info.curx % 4 != 0 )
        advancecursor(1);
}


void clreol(void)
{
    short *cell;
    int count;
    int x, y;
    int i;
    
    cell = dos_currentcell();
    x = text.info.curx - base;
    y = text.info.cury - base;
    count = text.info.screenwidth - x;
    
    for ( i = 0; i < count; i++, cell++ ) {
        printf("x: %d\n", x);
        *cell &= 0xFF00; /* clear char info */
        dos_drawchar(*cell, x++, y);
    }
}


void clrscr()
{
    if ( text.buf ) {
        memset(text.buf, 0, TXTBUFSIZE);
    }
    
    text.info.attribute = LIGHTGRAY;
    text.info.curx = base;
    text.info.cury = base;
    
    dos_setcga(BLACK);
    SDL_RenderClear(renderer); /* target is screen texture */
}


void delline()
{
    short *     cell;
    int         max_row;
    int         x, y;
    
    /* move all following rows up by one */
    max_row = dos_maxy();
    for ( y = text.info.cury; y < max_row; y++ ) {
        move_line(y + 1, y);
    }
    
    /* clear the last line */
    y = text.info.screenheight - 1;
    cell = dos_cell(0, y);
    for ( x = 0; x < text.info.screenwidth; x++ ) {
        *cell = text.info.attribute << 8;
        dos_drawchar(*cell, x, y);
        cell++;
    }
}


/* check gettext and puttext coords */
static int bad_coords(int left, int top, int right, int bottom)
{
    if (left < base
        || top < base
        || right > dos_maxx()
        || bottom > dos_maxy()) {
        return 1;
    }
    
    if ( left > right || top > bottom ) {
        return 1;
    }
    
    return 0;
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
    
    x = left - base;
    y = top - base;
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


short *gettextbuffer(void)
{
    return text.buf;
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


void insline(void)
{
    int x, y;
    short * cell;
    
    for ( y = dos_maxy() - 1; y >= text.info.cury; --y ) {
        move_line(y, y + 1);
    }
    
    /* delete current line */
    
    y = text.info.cury - base;
    cell = dos_cell(0, y);
    
    for ( x = 0; x < text.info.screenwidth; x++ ) {
        *cell &= 0xFF00;
        dos_drawchar(*cell, x, y);
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
    x = left - base;
    y = top - base;
    r = right - base;
    b = bottom - base;

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

/* TODO: should have done this first! */
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
    return keybuf.count;
}


int mousehit()
{
    return mousebuf.count;
}


int getch()
{
    if ( keybuf.count ) {
        return keybuf.data[--keybuf.count];
    }
    
    return EOF;
}


int getche(void)
{
    if ( keybuf.count ) {
        return putch(keybuf.data[--keybuf.count]);
    }
    
    return EOF;
}



int getmouse()
{
    if ( mousebuf.count ) {
        return mousebuf.data[--mousebuf.count];
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
    int x, y;

    x = text.info.curx - base;
    y = text.info.cury - base;
        
    cell = &text.buf[y * text.info.screenwidth + x];
    *cell = (unsigned char)c;
    *cell |= text.info.attribute << 8;
    
    dos_drawchar(*cell, x, y);
    advancecursor(1);
    
    return c;
}


int ungetch(int ch)
{
    if ( keybuf.count + 1 < QUEUE_SIZE ) {
        return (keybuf.data[keybuf.count++] = ch);
    }
    
    return EOF;
}


void setbase(int index)
{
    base = index;
}


short *textbuffer()
{
    return text.buf;
}
