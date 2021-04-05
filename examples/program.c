//
// program.c
// example of a basic program
//
#include <dos.h>
#include <conio.h>
#include <stdlib.h>

void display_message(int color, int count)
{
    clrscr();
    gotoxy(1, 1);
    textcolor(color);
    
    for ( int i = 0; i < count; i++ ) {
        cprintf("This program doesn't do much.\n");
    }
}

int main()
{
    randomize();
    initdos();
    
    display_message(LIGHTGRAY, 10);
    
    while ( 1 ) {
        if ( kbhit() ) {
            int key = getch();

            switch ( key ) {
                case 'c':
                    display_message(rand() % 16, (rand() % 10) + 5);
                    break;
                case 'q':
                    return 0;
                default:
                    break;
            }
        }
                
        refresh();
    }
}
