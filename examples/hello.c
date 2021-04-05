#include <dos.h>
#include <conio.h>

int main()
{
    initdos();
    
    cprintf("Hello, world!");
    refresh();
    sleep(2);
    
    return 0;
}
