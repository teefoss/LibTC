#include "conio.h"
#include "internal.h"

#include <stdarg.h>

#define CLAMP(x,low,high)  (((x)>(high))?(high):(((x)<(low))?(low):(x)))

queue_t keybuf;
queue_t mousebuf;

static void newline()
{
    text.info.curx = base;
    
    if ( text.info.cury < text.info.screenheight - 1 + base ) {
        text.info.cury++;
    }
}


static void advancecursor(int amount)
{
    text.info.curx += amount;
    
    if ( text.info.curx > text.info.screenwidth - 1 + base ) {
        newline();
    }
}


static void tab()
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
    
}


int gettext(int left, int top, int right, int bottom, void *destin)
{
    void *check;
    short *cell;
    int x, y;
    int w, h;
    int i;
    
    if (left < base
        || top < base
        || right > dos_maxx()
        || bottom >= dos_maxy()) {
        return 0;
    }
    
    if ( left > right || top > bottom ) {
        return 0;
    }
    
    x = left - base;
    y = top - base;
    w = right - left + 1;
    h = bottom - top + 1;
    cell = dos_cell(x, y);
    
    for ( i = 0; i < h; i++ ) {
        check = memcpy(destin, (void *)cell, w * sizeof(short));
        
        if ( check != destin ) {
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


void lowvideo(void)
{
    text.info.attribute &= ~0x8;
}


void normvideo(void)
{
    text.info.attribute = LIGHTGRAY;
}


/* TODO: this is fucked */
int puttext(int left, int top, int right, int bottom, void *source)
{
    /* TODO: error handling */
    int x, y;
    short *src_cell, *dst_cell;
    int i;
        
    src_cell = (short *)source;

    for ( y = top, i = 0; y <= bottom; y++ ) {
        for ( x = left; x <= right; x++ ) {
            dst_cell = coord_to_cell(x, y);
            *dst_cell = src_cell[i++];
            printf("dst set to %x\n", *dst_cell);
        }
    }
    
    return 1;
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


void window(int left, int top, int right, int bottom)
{
    text.info.winleft = left;
    text.info.wintop = top;
    text.info.winright = right;
    text.info.winbottom = bottom;
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
    return 0;
}


int getmouse()
{
    if ( mousebuf.count ) {
        return mousebuf.data[--mousebuf.count];
    }
    
    return 0;
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


int  getche(void)
{
    return 0;
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


int  ungetch(int ch)
{
    (void)ch;
    return 0;
}


void setbase(int index)
{
    base = index;
}

