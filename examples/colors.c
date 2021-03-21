#include <dos.h>
#include <conio.h>

const char *color_names[] =
{
    "BLACK", "BLUE", "GREEN", "CYAN",
    "RED", "MAGENTA", "BROWN", "LIGHTGRAY",
    "DARKGRAY", "LIGHTBLUE", "LIGHTGREEN", "LIGHTCYAN",
    "LIGHTRED", "LIGHTMAGENTA", "YELLOW", "WHITE"
};

int main()
{
    setscale(3);
    setscreensize(32, 18);
    setcursor(0);
    
    initdos();
    
    cputs("examples/colors.c\n\n");
    cputs("        (COLOR  0: BLACK)\n");
    
    for ( int i = 1; i < 16; i++ )
    {
        textcolor(i);
        
        for ( int x = 1; x <= 8; x++ )
            putch(219);
        
        cprintf(" COLOR %2d: %s\n", i, color_names[i]);
    }
        
    while ( 1 )
        refresh();
}
