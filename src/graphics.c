#include "graphics.h"
#include "internal.h"

int bkcolor;
int drawcolor = WHITE;
static int cpx;
static int cpy;

void circle(int x, int y, int radius)
{
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int x1 = 0;
    int y1 = radius;
 
    dos_setcga(drawcolor);
    
    SDL_RenderDrawPoint(renderer, x, y + radius);
    SDL_RenderDrawPoint(renderer, x, y - radius);
    SDL_RenderDrawPoint(renderer, x + radius, y);
    SDL_RenderDrawPoint(renderer, x - radius, y);
 
    while ( x1 < y1 ) {
        if ( f >= 0 ) {
            y1--;
            ddF_y += 2;
            f += ddF_y;
        }
        x1++;
        ddF_x += 2;
        f += ddF_x + 1;
        SDL_RenderDrawPoint(renderer, x + x1, y + y1);
        SDL_RenderDrawPoint(renderer, x - x1, y + y1);
        SDL_RenderDrawPoint(renderer, x + x1, y - y1);
        SDL_RenderDrawPoint(renderer, x - x1, y - y1);
        SDL_RenderDrawPoint(renderer, x + y1, y + x1);
        SDL_RenderDrawPoint(renderer, x - y1, y + x1);
        SDL_RenderDrawPoint(renderer, x + y1, y - x1);
        SDL_RenderDrawPoint(renderer, x - y1, y - x1);
    }
}


void cleardevice(void)
{
    clrscr();
    cpx = 0;
    cpy = 0;
    dos_setcga(bkcolor);
    SDL_RenderClear(renderer);
}


static void floodfill_r(int x, int y, int color, int replace)
{
    int p;
    if ( replace == color )
        return;
    
    if ( x < 0 || x >= getmaxx() || y < 0 || y >= getmaxy() )
        return;
    
    p = getpixel(x, y);
    if ( p != replace )
        return;

    dos_setcga(color);
    SDL_RenderDrawPoint(renderer, x, y);
    
    floodfill_r(x + 0, y + 1, color, replace);
    floodfill_r(x + 0, y - 1, color, replace);
    floodfill_r(x - 1, y + 0, color, replace);
    floodfill_r(x + 1, y + 0, color, replace);
}


void floodfill(int x, int y, int color)
{
    int replace;
    
    replace = getpixel(x, y);
    floodfill_r(x, y, color, replace);
}


int getbkcolor(void)
{
    return bkcolor;
}


int getcolor(void)
{
    return drawcolor;
}


void getimage(int left, int top, int right, int bottom, void *bitmap)
{
    int x, y;
    int w, h;
    
    w = right - left + 1;
    h = bottom - top + 1;
    
    *(short *)bitmap = w;
    bitmap = (short *)bitmap + 1;
    *(short *)bitmap = h;
    bitmap = (short *)bitmap + 1;
    
    for ( y = top; y <= bottom; y++ ) {
        for ( x = left; x <= right; x++ ) {
            *(char *)bitmap = getpixel(x, y);
            bitmap = (char *)bitmap + 1;
        }
    }
}



int  getmaxx(void)
{
    return text.info.screenwidth * text.char_w - 1;
}


int  getmaxy(void)
{
    return text.info.screenheight * text.char_h - 1;
}


unsigned getpixel(int x, int y)
{
    SDL_Rect    rect;
    uint32_t    pixel;
    uint32_t    format;
    int         pitch;
    int         comp[3];
    int         irgb;
    int         i;
    
    rect.x = x;
    rect.y = y;
    rect.w = rect.h = 1;
    
    pitch = (getmaxx() + 1) * 4;
    format = SDL_PIXELFORMAT_RGBA8888;
    
    SDL_RenderReadPixels(renderer, &rect, format, &pixel, pitch);
        
    if ( pixel == 0xAA5500FF ) {
        return BROWN;
    }
    
    comp[2] = (pixel & 0xFF000000) >> 24; /* r */
    comp[1] = (pixel & 0x00FF0000) >> 16; /* g */
    comp[0] = (pixel & 0x0000FF00) >> 8;  /* b */
    irgb    = 0;
    
    for ( i = 0; i < 3; i++ ) {
        if ( comp[i] == 0x55 || comp[i] == 0xFF ) { /* it's bright */
            irgb |= 8;
        }
        
        if ( comp[i] >= 0xAA ) { /* yes */
            irgb |= 1 << i;
        }
    }
    
    return irgb;
}


unsigned imagesize(int left, int top, int right, int bottom)
{
    int w, h;
    
    w = right - left + 1;
    h = bottom - top + 1;
    
    return w * h + 4;
}


void line(int x1, int y1, int x2, int y2)
{
    dos_setcga(drawcolor);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}


void linerel(int dx, int dy)
{
    line(cpx, cpy, cpx + dx, cpy + dy);
    moverel(dx, dy);
}


void lineto(int x, int y)
{
    line(cpx, cpy, x, y);
    moveto(x, y);
}


void moverel(int dx, int dy)
{
    cpx += dx;
    cpy += dy;
}


void moveto(int x, int y)
{
    cpx = x;
    cpy = y;
}


void putimage(int left, int top, void *bitmap, int op)
{
    short w, h;
    short *p16;
    char *p8;
    int x, y;
    
    p16 = (short *)bitmap;
    w = *p16++;
    h = *p16++;
    p8 = (char *)p16;
    
    for ( y = top; y < top + h; y++ ) {
        for ( x = left; x < left + w; x++ ) {
            int current = getpixel(x, y);
            int new;
            
            switch ( op ) { /* check if this is right */
                default:
                case COPY_PUT:
                    new = *p8;
                    break;
                case XOR_PUT:
                    new = *p8 ^ current;
                    break;
                case OR_PUT:
                    new = *p8 | current;
                    break;
                case AND_PUT:
                    new = *p8 & current;
                    break;
                case NOT_PUT:
                    new = !*p8;
                    break;
            }
            putpixel(x, y, new);
            p8++;
        }
    }
}


void putpixel(int x, int y, int color)
{
    dos_setcga(color);
    SDL_RenderDrawPoint(renderer, x, y);
}


void rectangle(int left, int top, int right, int bottom)
{
    SDL_Rect r;
    
    r.x = left;
    r.y = top;
    r.w = right - left + 1;
    r.h = bottom - top + 1;
    
    dos_setcga(drawcolor);
    SDL_RenderDrawRect(renderer, &r);
}


void setbkcolor(int color)
{
    bkcolor = color;
}


void setcolor(int color)
{
    drawcolor = color;
}


int textheight(void)
{
    return text.char_h;
}


int textwidth(char *textstring)
{
    if ( textstring ) {
        return strlen(textstring) * text.char_w;
    }
    return 0;
}
