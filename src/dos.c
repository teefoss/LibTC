#include "dos.h"
#include "conio.h"
#include "internal.h"

#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#define TEXTblink_INTERVAL     250
#define CURSORblink_INTERVAL   150

static SDL_Window *     win;
static SDL_Texture *    screen;
static SDL_Rect         screenrect;
static char *           winname;
static int              scale = 1;
static unsigned char    bdrcolor = BLACK;
static int              curstype = CURSOR_NORMAL;
static int              modifiers;

static SDL_AudioSpec        spec;
static SDL_AudioDeviceID    device;

unsigned char   bordersize = 4;
SDL_Renderer *  renderer;

text_t text = {
    .buf = NULL,
    .info = {
        .winleft = 1,
        .wintop = 1,
        .winright = 80,
        .winbottom = 25,
        .attribute = LIGHTGRAY,
        .normattr = LIGHTGRAY,
        .currmode = C80,
        .screenheight = 25,
        .screenwidth = 80,
        .curx = 1,
        .cury = 1
    },
    .char_w = 8,
    .char_h = MODE80H
};

int base = 1;


short * dos_cell(int x, int y)
{
    return text.buf + (y * text.info.screenwidth + x);
}


short * dos_currentcell()
{
    int x = text.info.curx - base;
    int y = text.info.cury - base;
    
    return dos_cell(x, y);
}

/* translate a cell coord to buffer index */
short * coord_to_cell(int x, int y)
{
    int ix = x - base;
    int iy = y - base;
    
    return dos_cell(x, y);
}


int dos_maxx()
{
    return text.info.screenwidth - 1 + base;
}


int dos_maxy()
{
    return text.info.screenheight - 1 + base;
}


int dos_clamp(int x, int min, int max)
{
    if ( x < min )
        x = min;
    else if (x > max)
        x = max;
    
    return x;
}


int dos_scale()
{
    if ( text.info.currmode == BW40 || text.info.currmode == C40) {
        return scale + 1;
    }
    return scale;
}


static SDL_Color cgacolor(COLOR c)
{
    SDL_Color result;
    int i;
    
    i = c & 8 ? 0x55 : 0;
    
    result.r = ((c & 4) >> 2) * 0xAA + i;
    result.g = ((c & 2) >> 1) * 0xAA + i;
    result.b = ((c & 1) >> 0) * 0xAA + i;
    
    if ( c == BROWN ) {
        result.g -= 0x55; /* tweak dark yellow -> brown */
    }
    
    return result;
}


void dos_setcga(COLOR c)
{
    SDL_Color set;
    
    switch ( text.info.currmode ) {
        case BW40:
        case BW80:
            if ( c >= BLUE && c <= BROWN ) {
                c = DARKGRAY;
            } else if ( c >= LIGHTBLUE && c <= YELLOW ) {
                c = LIGHTGRAY;
            }
            break;
        default:
            break;
    }
    
    set = cgacolor(c);
    SDL_SetRenderDrawColor(renderer, set.r, set.g, set.b, 255);
}


static BOOL blink(int interval)
{
    return ( (SDL_GetTicks() / interval) & 1 ) == 0;
}


static void limitframerate(int fps)
{
    static int last = 0;
    int now;
    int elapsed_ms;
    int interval;
    
    interval = 1000 / fps;
    
    do {
        now = SDL_GetTicks();
        elapsed_ms = now - last;
        
        if ( elapsed_ms > interval ) {
            break;
        }
        
        SDL_Delay(1);
    } while ( elapsed_ms < interval );

    last = now;
}


static void rendercursor(void)
{
    SDL_Rect r;
    
    r.x = (text.info.curx - base) * text.char_w;
    r.w = text.char_w;
    
    switch ( curstype ) {
        case CURSOR_NORMAL:
            r.h = text.char_h / 5;
            r.y = ((text.info.cury - base) * text.char_h) + text.char_h - r.h;
            break;
        case CURSOR_SOLID:
            r.h = text.char_h;
            r.y = (text.info.cury - base) * text.char_h;
            break;
        default:
            break;
    }

    dos_setcga(text.info.attribute & 0x0F);
    
    SDL_RenderSetViewport(renderer, &screenrect);
    SDL_RenderFillRect(renderer, &r);
    SDL_RenderSetViewport(renderer, NULL);
}


void dos_drawchar(short cell, int x, int y)
{
    unsigned char fg;
    unsigned char bg;
    const unsigned char *data;
    const unsigned char *fontdata;
    int start; /* font data row draw order */
    int stop;
    int step;
    extern const unsigned char fontdata40[];
    extern const unsigned char fontdata80[];
    int cx;
    int cy;
    int draw_x;
    
    fg = CELL_FG(cell);
    bg = CELL_BG(cell);
    
    if ( bg & 0x8 ) {
        bg &= ~0x8;
        if ( ((SDL_GetTicks() / 250) % 2) == 0 ) {
            fg = bg;
        }
    }
    
    /* the bits in fontdata40 are in reverse order to fontdata80 */
    /* TODO: find data of matching format! */
    if ( text.info.currmode == C40 || text.info.currmode == BW40 ) {
        fontdata = fontdata40;
        start = text.char_w - 1;
        stop = -1;
        step = -1;
    } else { /* 80 */
        fontdata = fontdata80;
        start = 0;
        stop = text.char_w;
        step = 1;
    }
    
    data = &fontdata[(CELL_CH(cell)) * text.char_h];
    for ( cy = 0; cy < text.char_h ; cy++ ) {
        
        for ( cx = start, draw_x = 0; cx != stop; cx += step ) {
            *data & (1 << cx) ? dos_setcga(fg) : dos_setcga(bg);
            SDL_RenderDrawPoint(renderer,
                                x * text.char_w + draw_x++,
                                y * text.char_h + cy);
        }
        data++;
    }
}


/* x, y: text coordinates */
void refresh_region(int x, int y, int w, int h)
{
    short * cell;
    int     ix, iy; /* text buffer indices */
    int     x1, y1;
    
    for ( y1 = y; y1 < y + h; y1++ ) {
        for ( x1 = x; x1 < x + w; x1++ ) {
            ix = x1 - base;
            iy = y1 - base;
            cell = dos_cell(ix, iy);
            dos_drawchar(*cell, ix, iy);
        }
    }
}


static void
initwin (void)
{
    int x, y, w, h;
    int border;
        
    border = bordersize * 2 * dos_scale();
    
    x = y = SDL_WINDOWPOS_CENTERED;
    w = (text.info.screenwidth * text.char_w * dos_scale()) + border;
    h = (text.info.screenheight * text.char_h * dos_scale()) + border;
        
    win = SDL_CreateWindow(winname, x, y, w, h, 0);
    
    if ( win == NULL ) {
        printf("initwin() failed: %s\n", SDL_GetError());
        quitdos();
    }
}


static void
initrenderer ()
{
    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    
    if ( renderer == NULL ) {
        printf("initrenderer() failed: %s\n", SDL_GetError());
        quitdos();
    }
    
    SDL_RenderSetScale(renderer, dos_scale(), dos_scale());
}


static void
initscreen ()
{
    int format;
    int access;
    int w, h;
    
    format = SDL_PIXELFORMAT_RGBA8888;
    access = SDL_TEXTUREACCESS_TARGET;
    w = text.info.screenwidth * text.char_w;
    h = text.info.screenheight * text.char_h;
    
    if ( screen ) {
        SDL_DestroyTexture(screen);
    }
    screen = SDL_CreateTexture(renderer, format, access, w, h);
    
    if ( screen == NULL ) {
        printf("initscreen() failed: %s\n", SDL_GetError());
        quitdos();
    }
    
    printf("initialized screen with size %d x %d\n", w, h);
    
    screenrect.x = bordersize;
    screenrect.y = bordersize;
    screenrect.w = w;
    screenrect.h = h;
    
    /* by default all drawing operations are done on the screen */
    SDL_SetRenderTarget(renderer, screen);
}


static void
inittextbuffer (void)
{
    text.buf = malloc(TXTBUFSIZE);
    clrscr();
}


static void
initsound (void)
{
    memset(&spec, 0, sizeof(spec));
    
    spec.freq = 44100;
    spec.format = AUDIO_S8;
    spec.channels = 1;
    spec.samples = 512;
    spec.callback = NULL;

    device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
}


/* get any events and put them in keybuf and mousebuf */
static void
fill_input_buffers (void)
{
    SDL_Event event;
    int sym;

    keybuf.count = 0;
    mousebuf.count = 0;
    modifiers = SDL_GetModState();
    
    while ( SDL_PollEvent(&event) ) {
        
        if ( keybuf.count == QUEUE_SIZE ) {
            return;
        }
        
        switch ( event.type ) {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                keybuf.data[keybuf.count++] = event.key.keysym.sym;
                break;
            case SDL_MOUSEBUTTONDOWN:
                mousebuf.data[mousebuf.count++] = event.button.button;
                break;
            default:
                break;
        }
    }
    
    SDL_StopTextInput();
}


void
initdos (void)
{    
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ) {
        printf("could not init sdl: %s\n", SDL_GetError());
        exit(1);
    }
    
    initwin();
    initrenderer();
    initscreen();
    inittextbuffer();
    initsound();
    
    refresh(); /* make sure the window appears */
    
    atexit(quitdos);
}


void
quitdos (void)
{
    if ( winname ) {
        free(winname);
    }
    
    if ( text.buf ) {
        free(text.buf);
    }
    
    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    
    SDL_CloseAudio();
    SDL_Quit();
}

void
refresh (void)
{
    fill_input_buffers();
    
    dos_setcga(bdrcolor);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderClear(renderer);
    
    SDL_RenderCopy(renderer, screen, NULL, &screenrect);

    if ( curstype != CURSOR_NONE && blink(CURSORblink_INTERVAL) ) {
        rendercursor();
    }
    
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, screen);
    
    limitframerate(60);
}


void sleep (unsigned seconds)
{
    SDL_Delay(seconds * 1000);
}


void delay(unsigned milliseconds)
{
    SDL_Delay(milliseconds);
}


void sound(unsigned frequency, unsigned milliseconds)
{
    int period = (float)spec.freq / (float)frequency / 2.0f;
    int len = (float)spec.freq * ((float)milliseconds / 1000.0f);
    int volume = 5;
    int i;
        
    for ( i = 0; i < len; i++ ) {
        int8_t sample = (i / period) % 2 ? volume : -volume;
        SDL_QueueAudio(device, &sample, sizeof(sample));
    }

    SDL_PauseAudioDevice(device, 0);
    while ( SDL_GetQueuedAudioSize(device) )
        ;
    SDL_PauseAudioDevice(device, 1);
}


void setscale(int newscale)
{
    if ( newscale < 1 )
        newscale = 1;
    
    scale = newscale;
}


void setscreensize(int width, int height)
{
    text.info.screenwidth = (unsigned char)width;
    text.info.screenheight = (unsigned char)height;
}


void setbordersize(int newsize)
{
    if ( win == NULL ) {
        bordersize = (unsigned char)newsize;
    }
}


void setcursor(int type)
{
    curstype = type;
}


void setname(const char * newname)
{
    if ( newname ) {
        int len = strlen(newname);
        
        if ( winname ) {
            winname = realloc(winname, len);
        } else {
            winname = malloc(len);
        }
        
        strcpy(winname, newname);
        
        if ( win ) {
            SDL_SetWindowTitle(win, winname);
        }
    }
}


void setbordercolor(int color)
{
    bdrcolor = (unsigned char)color;
}


int getbordercolor()
{
    return bdrcolor;
}


void savescr(const char *file)
{
    SDL_Surface *save;

    refresh();
    save = SDL_GetWindowSurface(win);
    
    if ( save == NULL ) {
        puts("savescr: could not create surface!");
        return;
    }
    
    if ( SDL_SaveBMP(save, file) != 0 ) {
        printf("savescr: could not save bmp!");
    }
}


void randomize(void)
{
    srand((unsigned)time(NULL));
}


int getmod()
{
    return modifiers;
}
