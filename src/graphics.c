#include "graphics.h"
#include "internal.h"

int bkcolor;

void cleardevice(void)
{
    clrscr();
    _setcga(bkcolor);
    SDL_RenderClear(renderer);
}

void setbkcolor(int color)
{
    bkcolor = color;
}


int  textheight(void)
{
    return _textheight;
}


int textwidth(char *textstring)
{
    if ( textstring ) {
        return strlen(textstring) * _textwidth;
    }
    return 0;
}
