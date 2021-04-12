#include "internal.h"

BOOL dos_push(STACK * s, int data)
{
    if ( s->top == STACK_SIZE - 1 ) {
        return NO;
    }
    
    s->data[++s->top] = data;
    return YES;
}


int dos_pop(STACK * s)
{
    if ( s->top < 0 ) {
        printf("error: stack underflow\n");
        exit(EXIT_FAILURE);
    }
    
    return s->data[s->top--];
}


void dos_empty(STACK * s)
{
    s->top = -1;
}


bufpt_t con_to_buf(conpt_t c)
{
    bufpt_t b;
    b.x = c.x - 1;
    b.y = c.y - 1;
    
    return b;
}

conpt_t win_to_con(winpt_t w)
{
    conpt_t c;
    c.x = w.x + text.info.winleft - 1;
    c.y = w.y + text.info.wintop - 1;
    
    return c;
}

bufpt_t win_to_buf(winpt_t w)
{
    conpt_t c = win_to_con(w);
    
    return con_to_buf(c);
}


short * dos_cell(bufpt_t b)
{
    return text.buf + (b.y * text.info.screenwidth + b.x);
}


short * dos_currentcell()
{
    winpt_t cur;
    
    cur.x = text.info.curx;
    cur.y = text.info.cury;
    
    return dos_cell( win_to_buf(cur) );
}


/* max coord in current window */
int dos_maxx()
{
    return (text.info.winright - text.info.winleft) + 1;
}


int dos_maxy()
{
    return (text.info.winbottom - text.info.wintop) + 1;
}


/* console window w and h are the same as the max coord */
int dos_cwin_w()
{
    return dos_maxx();
}


int dos_cwin_h()
{
    return dos_maxy();
}



int dos_clamp(int x, int min, int max)
{
    if ( x < min )
        x = min;
    else if (x > max)
        x = max;
    
    return x;
}


static SDL_Color dos_cgacolor(COLOR c)
{
    SDL_Color result;
    int i;
    
    i = c & 8 ? 0x55 : 0;
    
    result.r = ((c & 4) >> 2) * 0xAA + i;
    result.g = ((c & 2) >> 1) * 0xAA + i;
    result.b = ((c & 1) >> 0) * 0xAA + i;
    
    if ( c == BROWN ) {
        result.g -= 0x55; /* tweak dark yellow -> brown */
    }
    
    return result;
}


void dos_setcga(COLOR c)
{
    SDL_Color set;
    
    switch ( text.info.currmode ) {
        case BW40:
        case BW80:
            if ( c >= BLUE && c <= BROWN ) {
                c = DARKGRAY;
            } else if ( c >= LIGHTBLUE && c <= YELLOW ) {
                c = LIGHTGRAY;
            }
            break;
        default:
            break;
    }
    
    set = dos_cgacolor(c);
    SDL_SetRenderDrawColor(renderer, set.r, set.g, set.b, 255);
}
