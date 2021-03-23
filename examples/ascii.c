#include <dos.h>
#include <conio.h>

#define WIDTH 16
#define HEIGHT 16

int main()
{
    textmode(C80);
    setscreensize(WIDTH, HEIGHT);
    setscale(3);
    setcursor(CURSOR_NONE);
    setname("ascii.c");
    setbase(0);
    
    initdos();

    for ( int ch = 0; ch <= 255; ch++ )
    {
        gotoxy(ch % WIDTH, ch / WIDTH);
        putch(ch);
    }
        
    while ( 1 )
        refresh();
}
