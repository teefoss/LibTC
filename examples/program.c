#include <dos.h> /* for initialization and basic library functions */
#include <conio.h> /* for console input/output */
#include <graphics.h> /* for graphics functions (drawing shapes, etc.) */

int main()
{
    /* initialization options should come before calling initdos() */
    setscale(2);
    setcursor(CURSOR_SOLID);

    initdos(); /* initialize LibDOS */

    /* main loop */
    while ( 1 )
    {
        int key;
        if ( kbhit() )
            key = getch(); /* get a keystroke */

        if ( key == 'p' )
        {
            textcolor(RED);
            gotoxy(5, 5); /* move cursor position */
            cprintf("Hello, world!"); /* display red text */
        }
        else if ( key == 'c' )
        {
            clrscr(); /* clear the screen */
        }

        setcolor(LIGHTBLUE);
        circle(64, 64, 48); /* draw a light blue circle at (64, 64) with radius 48. */

        refresh(); /* make any changes appear */
    }
}
