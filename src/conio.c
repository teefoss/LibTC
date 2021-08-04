#include "dos.h"
#include "conio.h"
#include "internal.h"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define CLAMP(x,low,high)  (((x)>(high))?(high):(((x)<(low))?(low):(x)))

/* from, to in win coords */
static void move_line(int from_y, int to_y)
{
    winpt_t from, to;
    short   *dst, *src;
    int     w, i;
    
    from.x = 1;
    from.y = from_y;
    to.x = 1;
    to.y = to_y;
    
    src = dos_cell( win_to_buf(from) );
    dst = dos_cell( win_to_buf(to) );
    
    w = dos_cwin_w();
    for ( i = 0; i < w; i++ ) {
        *dst++ = *src++;
    }
    
    dos_refresh_region(win_to_con(from), w, 1);
    dos_refresh_region(win_to_con(to),   w, 1);
}


static void clear_current_window_line()
{
    winpt_t w;
    int x;
    short * cell;
    
    /* get start of current line */
    w.x = 1;
    w.y = text.info.cury;
    cell = dos_cell(win_to_buf(w));
    
    for ( x = 1; x <= dos_maxx(); x++) {
        *cell &= 0xFF00;
    }
}


static void newline()
{
    text.info.curx = text.info.winleft;
    
    if ( text.info.cury < dos_maxy() ) {
        text.info.cury++; /* scroll if at bottom */
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
    int i;
    bufpt_t b;
    winpt_t curs;
    
    cell = dos_currentcell();
    curs.x = text.info.curx;
    curs.y = text.info.cury;
    b = win_to_buf(curs);
    count = dos_cwin_w();
    
    for ( i = 0; i < count; i++, cell++, b.x++ ) {
        *cell &= 0xFF00; /* clear char info */
        dos_drawchar(*cell, b);
    }
}


void clrscr()
{
    short * cell;
    winpt_t w;
    
    for ( w.y = 1; w.y <= text.info.winbottom; w.y++ ) {
        for ( w.x = 1; w.x <= text.info.winright; w.x++ ) {
            cell = dos_cell(win_to_buf(w));
            *cell = LIGHTGRAY << 8;
            dos_refresh_region(win_to_con(w), 1, 1);
        }
    }
    
    text.info.curx = 1;
    text.info.cury = 1;    
}


void delline()
{
    short *     cell;
    int         y;
    winpt_t     bottom_line;
    
    /* move all following rows up by one */
    for ( y = text.info.cury; y < dos_maxy(); y++ ) {
        move_line(y + 1, y);
    }
    
    /* clear the last line */
    bottom_line.x = 1;
    bottom_line.y = dos_maxy();
    cell = dos_cell(win_to_buf(bottom_line));
    
    for ( ; bottom_line.x <= dos_maxx(); bottom_line.x++ ) {
        *cell = text.info.attribute << 8; /* clear but keep attribute */
        dos_drawchar( *cell, win_to_buf(bottom_line) );
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
    short *cell;
    conpt_t c;
    int w, h;
    int i;
    
    if ( bad_coords(left, top, right, bottom) ) {
        return 0;
    }
    
    c.x = left;
    c.y = top;
    w = right - left + 1;
    h = bottom - top + 1;
    cell = dos_cell(con_to_buf(c));
    
    for ( i = 0; i < h; i++ ) {
        if ( memcpy(destin, (void *)cell, w * sizeof(short)) != destin ) {
            return 0;
        }
        
        cell += text.info.screenwidth;
        destin = (char *)destin + w * 2;
    }
    
    return 1;
}


// TODO: this!
unsigned char getscreench(int x, int y)
{
    winpt_t winpt = { x, y };
    bufpt_t bufpt = win_to_buf(winpt);
    
    return *dos_cell(bufpt) & CH_MASK;
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
    text.info.attribute |= FG_IBIT;
}

/* FIXME: ?*/
void insline(void)
{
    int y, w;
    short * cell;
    winpt_t win;
    
    for ( y = text.info.winbottom - 1; y >= text.info.cury; --y ) {
        move_line(y, y + 1);
    }

    /* delete current line */
    win.x = 1;
    win.y = text.info.cury;
    w = dos_cwin_w();
    cell = dos_cell(win_to_buf(win));
    
    for ( ; win.x <= w; win.x++ ) {
        *cell &= 0xFF00; /* clear char but not attribute */
        dos_drawchar(*cell, win_to_buf(win));
        cell++;
    }
}


void lowvideo(void)
{
    text.info.attribute &= ~FG_IBIT;
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
    conpt_t pt;
    bufpt_t b;
    short *src_cell, *dst_cell;
    int result;
    
    if ( bad_coords(left, top, right, bottom) ) {
        return 0;
    }
    
    src_cell = (short *)source;
    result = 0;
    for ( pt.y = top; pt.y <= bottom; pt.y++ ) {
        for ( pt.x = left; pt.x <= right; pt.x++ ) {
            b = con_to_buf(pt);
            dst_cell = dos_cell(b);
            *dst_cell = *src_cell++;
            dos_drawchar(*dst_cell, b);
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
            fontdata = fontdata40;
            break;
        case BW80:
        case C80:
            text.char_h = MODE80H;
            text.info.screenwidth = 80;
            fontdata = fontdata80;
            break;
        default:
            return; /* bad mode */
    }
    
    text.info.screenheight = 25;
    text.info.currmode = newmode;
    window(1, 1, text.info.screenwidth, text.info.screenheight);
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
    winpt_t win;
    bufpt_t buf;

    win.x = text.info.curx;
    win.y = text.info.cury;
    buf = win_to_buf(win);
        
    cell = dos_currentcell();
    *cell = (unsigned char)c;
    *cell |= text.info.attribute << 8;
    
    dos_drawchar(*cell, buf);
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
