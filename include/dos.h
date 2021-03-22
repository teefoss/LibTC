#ifndef dos_h_
#define dos_h_

#define CURSOR_NONE     0
#define CURSOR_NORMAL   1
#define CURSOR_SOLID    2

void initdos(void);
void quitdos(void);

void refresh(void);
void sleep(unsigned seconds);
void delay(unsigned milliseconds);
void randomize(void);
unsigned dosrand(unsigned num);

void setscale(int newscale);
void setscreensize(int width, int height);
void setbordersize(int newsize);
void setcursor(int type);
void setname(const char * newname);
void bordercolor(int newcolor);

void sound(unsigned frequency, unsigned milliseconds);

void savescr(const char *file);

#endif /* dos_h_ */
