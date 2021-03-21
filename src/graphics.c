#include "graphics.h"
#include "internal.h"

int bkcolor;

void cleardevice(void)
{
    clrscr();
    dos_setcga(bkcolor);
    SDL_RenderClear(renderer);
}

void setbkcolor(int color)
{
    bkcolor = color;
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
