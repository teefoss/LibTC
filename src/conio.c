#include "conio.h"
#include "internal.h"

#include <stdarg.h>

#define CLAMP(x,low,high)  (((x)>(high))?(high):(((x)<(low))?(low):(x)))

inbuf_t kb;
inbuf_t _mousebuf;

static void _newline()
{
    textinfo.curx = _base;
    
    if ( textinfo.cury < textinfo.screenheight - 1 + _base ) {
        textinfo.cury++;
    }
}


static void _advancecursor(int amount)
{
    textinfo.curx += amount;
    
    if ( textinfo.curx > textinfo.screenwidth - 1 + _base ) {
        _newline();
    }
}


static void _tab()
{
    while ( textinfo.curx % 4 != 0 )
        _advancecursor(1);
}


#pragma mark - PUBLIC

void clreol(void)
{
    short *cell;
    int count;
    int x, y;
    
    cell = _curtxtbufcell();
    x = textinfo.curx - _base;
    y = textinfo.cury - _base;
    count = textinfo.screenwidth - x;
    
    for ( int i = 0; i < count; i++, cell++ ) {
        printf("x: %d\n", x);
        *cell &= 0xFF00; /* clear char info */
        _drawchar(*cell, x++, y);
    }
}


void clrscr()
{
    if ( _textbuffer ) {
        memset(_textbuffer, 0, TXTBUFSIZE);
    }
    
    textinfo.attribute = LIGHTGRAY;
    textinfo.curx = _base;
    textinfo.cury = _base;
    
    _setcga(BLACK);
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
    
    if (left < _base
        || top < _base
        || right > _maxtextx()
        || bottom >= _maxtexty()) {
        return 0;
    }
    
    if ( left > right || top > bottom ) {
        return 0;
    }
    
    x = left - _base;
    y = top - _base;
    w = right - left + 1;
    h = bottom - top + 1;
    cell = _txtbufcell(x, y);
    
    for ( int i = 0; i < h; i++ ) {
        check = memcpy(destin, (void *)cell, w * sizeof(short));
        
        if ( check != destin ) {
            return 0;
        }
        
        cell += textinfo.screenwidth;
        destin += w * 2;
    }
    
    return 1;
}


short *gettextbuffer(void)
{
    return _textbuffer;
}


void gettextinfo(TEXT_INFO *r)
{
    *r = textinfo;
}


void gotoxy(int x, int y)
{
    textinfo.curx = x;
    textinfo.cury = y;
}


int puttext(int left, int top, int right, int bottom, void *source)
{
    /* TODO: error handling */
    int x, y, w, h;
    short *cell;
    int i, x1, y1;
    
    x = left - _base;
    y = top - _base;
    w = right - left + 1;
    h = bottom - top + 1;
    
    cell = (short *)source;

    for ( y1=y, i=0 ; y1<y+h ; y1++ ) {
        for ( x1=x ; x1<x+w ; x++ ) {
            _textbuffer[y1 * textinfo.screenwidth + x1] = cell[i++];
        }
    }
    
    return 1;
}


void textattr(int newattr)
{
    textinfo.attribute = newattr;
}


void textbackground(int newcolor)
{
    textinfo.attribute &= 0x0F;
    textinfo.attribute |= (newcolor << 4);
}


void textcolor(int newcolor)
{
    textinfo.attribute &= 0xF0;
    textinfo.attribute |= newcolor;
}


void textmode(int newmode)
{    
    switch ( newmode ) {
        case BW40:
        case C40:
            _textheight = MODE40H;
            textinfo.screenwidth = 40;
            fontdata = fontdata40;
            break;
        case BW80:
        case C80:
            _textheight = MODE80H;
            textinfo.screenwidth = 80;
            fontdata = fontdata80;
            break;
        default:
            return; /* bad mode */
    }
    
    textinfo.screenheight = 25;
    textinfo.currmode = newmode;
    clrscr();
}


int  wherex(void)
{
    return textinfo.curx;
}


int  wherey(void)
{
    return textinfo.cury;
}


int mousex(void)
{
    int x;
    int maxx;
    
    SDL_GetMouseState(&x, NULL);
    x /= _scale();
    x -= bordersize;
    maxx = textinfo.screenwidth * _textwidth - 1;
    x = _clamp(x, 0, maxx);
    
    return x;
}


int mousey(void)
{
    int y, maxy;
    
    SDL_GetMouseState(NULL, &y);
    y /= _scale();
    y -= bordersize;
    maxy = textinfo.screenheight * _textheight - 1;
    y = _clamp(y, 0, maxy);
    
    return y;
}


void window(int left, int top, int right, int bottom)
{
    textinfo.winleft = left;
    textinfo.wintop = top;
    textinfo.winright = right;
    textinfo.winbottom = bottom;
}


int kbhit()
{
    return kb.top;
}


int mousehit()
{
    return _mousebuf.top;
}


int getch()
{
    if ( kb.top ) {
        return kb.buffer[--kb.top];
    }
    return 0;
}


int getmouse()
{
    if ( _mousebuf.top ) {
        return _mousebuf.buffer[--_mousebuf.top];
    }
    return 0;
}


//char *cgets(char *str)
//{
//
//}


int cprintf(const char *format, ...)
{
    const int size = 0x80;
    va_list args;
    char buffer[size] = { 0 };
    int ret;
    
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    ret = cputs(buffer);
    va_end(args);
    
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
                _newline();
                break;
            case '\t':
                _tab();
                break;
            default:
                ret = putch(*c);
                break;
        }
        c++;
    }
    
    return ret;
}


//int  cscanf(const char *format, ...)
//{
//
//}


int  getche(void)
{
    return 0;
}


int putch(int c)
{
    short * cell;
    int x, y;

    x = textinfo.curx - _base;
    y = textinfo.cury - _base;
    
//    if ( x < 0 || y < 0 || (x + _base) * (y + _base) > TXTBUFSIZE )
//        return -1;
    
    cell = &_textbuffer[y * textinfo.screenwidth + x];
    *cell = (unsigned char)c;
    *cell |= textinfo.attribute << 8;
    
    _drawchar(*cell, x, y);
    _advancecursor(1);
    
    return c;
}


int  ungetch(int ch)
{
    (void)ch;
    return 0;
}


void setbase(int index)
{
    _base = index;
}

