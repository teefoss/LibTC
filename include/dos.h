#ifndef dos_h_
#define dos_h_

#define CURSOR_NONE     0
#define CURSOR_NORMAL   1
#define CURSOR_SOLID    2

void delay(unsigned milliseconds);
int  getbordercolor(void);
int  getmod(void);
void initdos(void);
void quitdos(void);
void randomize(void);
void refresh(void);
void setbordercolor(int color);
void setbordersize(int newsize);
void setcursor(int type);
void setname(const char * newname);
void setscale(int newscale);
void setscreensize(int width, int height);
void sleep(unsigned seconds);
void sound(unsigned frequency, unsigned milliseconds);
void savescr(const char *file);

#endif /* dos_h_ */
