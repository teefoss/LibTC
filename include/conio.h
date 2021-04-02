#ifndef conio_h_
#define conio_h_

/* use this macro in combination with a value in the keys enum */
#define KEY(x)  (x|(1<<30))
typedef enum keys
{
    KEY_CAPSLOCK = 57,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_PRINTSCREEN,
    KEY_SCROLLLOCK,
    KEY_PAUSE,
    KEY_INSERT,
    KEY_HOME,
    KEY_PAGEUP,
    KEY_DELETE,
    KEY_END,
    KEY_PAGEDOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_DOWN,
    KEY_UP,
    KEY_NUMLOCKCLEAR,
    KP_DIVIDE,
    KP_MULTIPLY,
    KP_MINUS,
    KP_PLUS,
    KP_ENTER,
    KP_1,
    KP_2,
    KP_3,
    KP_4,
    KP_5,
    KP_6,
    KP_7,
    KP_8,
    KP_9,
    KP_0
} KEY;

typedef enum
{
    MOD_NONE    = 0x0000,
    MOD_LSHIFT  = 0x0001,
    MOD_RSHIFT  = 0x0002,
    MOD_LCTRL   = 0x0040,
    MOD_RCTRL   = 0x0080,
    MOD_LALT    = 0x0100,
    MOD_RALT    = 0x0200,
    MOD_LGUI    = 0x0400,
    MOD_RGUI    = 0x0800,
    MOD_CTRL    = MOD_LCTRL | MOD_RCTRL,
    MOD_SHIFT   = MOD_LSHIFT | MOD_RSHIFT,
    MOD_ALT     = MOD_LALT | MOD_RALT,
    MOD_GUI     = MOD_LGUI | MOD_RGUI
} KEYMOD;

typedef enum mouse_button
{
    BUTTON_LEFT = 1,
    BUTTON_MIDDLE,
    BUTTON_RIGHT
} MOUSEBUTTON;

typedef struct text_info
{
    unsigned char winleft;
    unsigned char wintop;
    unsigned char winright;
    unsigned char winbottom;
    unsigned char attribute;
    unsigned char normattr;
    unsigned char currmode;
    unsigned char screenheight;
    unsigned char screenwidth;
    unsigned char curx;
    unsigned char cury;
} TEXT_INFO;

enum text_modes { BW40, C40, BW80, C80 };

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
    DARKGRAY,        /* light colors */
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
} COLOR;
#endif

#define BLINK        128    /* blink bit */

/* Turbo C functions */

void clreol(void);
void clrscr(void);
void delline(void); /* TODO */
int  gettext(int left, int top, int right, int bottom, void *destin);
void gettextinfo(TEXT_INFO *r);
void gotoxy(int x, int y);
void highvideo(void);
void insline(void); /* TODO */
void lowvideo(void);
int  movetext(int left, int top, int right, int bottom,
              int destleft, int desttop); /* TODO */
void normvideo(void);
int  puttext(int left, int top, int right, int bottom, void *source);
void textattr(int newattr);
void textbackground(int newcolor);
void textcolor(int newcolor);
void textmode(int newmode);
int  wherex(void);
int  wherey(void);
void window(int left, int top, int right, int bottom); /* TODO */

char *cgets(char *str); /* TODO */
int  cprintf(const char *format, ...);
int  cputs(const char *str);
int  getch(void);
int  getche(void); /* TODO: check original implementation */
int  kbhit(void);
int  putch(int c);
int  ungetch(int ch); /* TODO */

/* LibDOS functions */

int  mousehit(void);
int  mousex(void);
int  mousey(void);
int  getmouse(void);
void setbase(int index);
short *textbuffer(void);

#endif /* conio_h_*/
