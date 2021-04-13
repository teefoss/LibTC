#include <dos.h>
#include <conio.h>

int main()
{
    setscale(10);
    setcursor(0);
    setscreensize(5, 1);
    initdos();
    
    textcolor(LIGHTRED);
    putch('L');
    textcolor(LIGHTMAGENTA);
    putch('i');
    textcolor(YELLOW);
    putch('b');
    textcolor(WHITE);
    cputs("TC");
    
    while (1) {
        refresh();
    }
}
