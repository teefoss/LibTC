#include <stdbool.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <graphics.h>

#define BOARDSIZE   3
#define CELLTYPES   3

int board[BOARDSIZE][BOARDSIZE];

void drawcell(int type, int x, int y)
{
    gotoxy(x, y);
    
    if ( type == 0 ) {
        textbackground(RED);
        textcolor(YELLOW);
        putch('G');
    } else if ( type == 1 ) {
        textbackground(MAGENTA);
        textcolor(LIGHTCYAN);
        putch('E');
    } else if ( type == 2 ) {
        textbackground(CYAN);
        textcolor(WHITE);
        putch('J');
    }
}


void drawboard()
{
    clrscr();
    
    for ( int y=0 ; y<BOARDSIZE ; y++ ) {
        for ( int x=0 ; x<BOARDSIZE ; x++ ) {
            drawcell(board[y][x], x+1, y+1);
        }
    }
}


// x, y: text coords
void togglecell(int x, int y)
{
    if ( x < 1 || x > BOARDSIZE || y < 1 || y > BOARDSIZE )
        return;
        
    board[y-1][x-1]++;
    board[y-1][x-1] %= CELLTYPES;
}


// x, y: text coords
void clickcell(int x, int y)
{
    togglecell(x + 0, y - 1);
    togglecell(x + 0, y + 1);
    togglecell(x - 1, y - 0);
    togglecell(x + 1, y - 0);
}


void randboard()
{
    setname("Randomizing...");
    
    for ( int y=0 ; y<BOARDSIZE ; y++ ) {
        for ( int x=0 ; x<BOARDSIZE ; x++ ) {
            
            int times = (rand() % 10) + 1;
            
            while ( times-- ) {
                clickcell(x + 1, y + 1);
                sound((rand() % 1000) + 800, 25);
                drawboard();
                refresh();
            }
        }
    }
    
    setname("GEJ");
}


bool checkforwin()
{
    int type = board[0][0];
    
    for ( int y=0 ; y<BOARDSIZE ; y++ )
        for ( int x=0 ; x<BOARDSIZE ; x++ )
            if ( board[y][x] != type )
                return false;
    
    return true;
}


void fanfare()
{
    sound(220, 200);
    sound(330, 200);
    sound(440, 200);
    sound(550, 200);
    sound(660, 200);
    sound(880, 200);
    
    for ( int i=0 ; i<15 ; i++ ) {
        sound(i % 2 ? 977 : 880, 75);
    }
}

void win()
{
    setname("You win!");
    delay(500);
    
    fanfare();
    
    setname("Congrats!");
    sleep(2);
    
    randboard();
}


void mouseinput()
{
    int b = getmouse();
    
    if ( b == BUTTON_LEFT ) {
        int x = mousex() / textwidth("A") + 1;
        int y = mousey() / textheight() + 1;
        
        clickcell(x, y);
        drawboard();
        sound(2000, 50);
        
        if ( checkforwin() ) {
            refresh();
            win();
        }
    }
}


int main()
{
    randomize();
    
    textmode(C40);
    setscreensize(BOARDSIZE, BOARDSIZE);
    setscale(10);
    setbordersize(1);
    bordercolor(DARKGRAY);
    setcursor(0);
    
    initdos();
    
    randboard();
    
    while (1) {
        if ( mousehit() ) {
            mouseinput();
        }
        refresh();
    }
}
