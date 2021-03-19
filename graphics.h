#ifndef graphics_h_
#define graphics_h_

#ifndef __COLORS
#define __COLORS

typedef enum colors {
    BLACK,            /* dark colors */
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,            /* light colors */
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
} COLOR;
#endif

#define HORIZ_DIR   0    /* left to right */
#define VERT_DIR    1    /* bottom to top */

enum putimage_ops { /* BitBlt operators for putimage */
    COPY_PUT,
    XOR_PUT,
    OR_PUT,
    AND_PUT,
    NOT_PUT
};

struct viewporttype {
    int left, top, right, bottom;
    int clip;
};

void circle(int x, int y, int radius);
void cleardevice(void);
void clearviewport(void);
void floodfill(int x, int y, int border);
int  getbkcolor(void);
int  getcolor(void);
void getimage(int left, int top, int right, int bottom, void *bitmap);
int  getmaxx(void);
int  getmaxy(void);
unsigned getpixel(int x, int y);
void getviewsettings(struct viewporttype *viewport);
int  getx(void);
int  gety(void);
void graphdefaults(void);
unsigned imagesize(int left, int top, int right, int bottom);
void line(int x1, int y1, int x2, int y2);
void linerel(int dx, int dy);
void lineto(int x, int y);
void moverel(int dx, int dy);
void moveto(int x, int y);
void putimage(int left, int top, void *bitmap, int op);
void putpixel(int x, int y, int color);
void rectangle(int left, int top, int right, int bottom);
void setbkcolor(int color);
void setcolor(int color);
void setviewport(int left, int top, int right, int bottom, int clip);
int  textheight(void);
int  textwidth(char *textstring);

#endif /* graphics_h_ */
