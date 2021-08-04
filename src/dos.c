#include "dos.h"
#include "conio.h"
#include "internal.h"

#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#define TEXT_BLINK_INTERVAL     250
#define CURSOR_BLINK_INTERVAL   150

const unsigned char * fontdata = fontdata80; /* font data for current mode */

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
STACK           keybuf;
STACK           mousebuf;

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


int dos_scale()
{
    if ( text.info.currmode == BW40 || text.info.currmode == C40) {
        return scale + 1;
    }
    return scale;
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
    
    r.x = (WINX2ABS(text.info.curx) - 1) * text.char_w;
    r.y = (WINY2ABS(text.info.cury) - 1) * text.char_h;
    r.w = text.char_w;
    
    switch ( curstype ) {
        case CURSOR_NORMAL:
            r.h = text.char_h / 5;
            r.y += text.char_h - r.h;
            break;
        case CURSOR_SOLID:
            r.h = text.char_h;
            break;
        default:
            break;
    }

    dos_setcga(text.info.attribute & 0x0F);
    
    SDL_RenderSetViewport(renderer, &screenrect);
    SDL_RenderFillRect(renderer, &r);
    SDL_RenderSetViewport(renderer, NULL);
}


void dos_drawchar(short cell, bufpt_t b)
{
    unsigned char fg, bg;
    const unsigned char *data;
    int cx, cy;
    int draw_x;
    
    fg = CELL_FG(cell);
    bg = CELL_BG(cell);
    
    if ( bg & 0x8 ) {
        bg &= ~0x8;
        if ( ((SDL_GetTicks() / TEXT_BLINK_INTERVAL) % 2) == 0 ) {
            fg = bg;
        }
    }
        
    data = &fontdata[(CELL_CH(cell)) * text.char_h];
    for ( cy = 0; cy < text.char_h ; cy++ ) {
        for ( cx = text.char_w - 1, draw_x = 0; cx >= 0; cx-- ) {
            *data & (1 << cx) ? dos_setcga(fg) : dos_setcga(bg);
            SDL_RenderDrawPoint(renderer,
                                b.x * text.char_w + draw_x++,
                                b.y * text.char_h + cy);
        }
        data++;
    }
}


void dos_refresh_region(conpt_t conpt, int w, int h)
{
    short *     cell;
    bufpt_t     i;
    conpt_t     c;
    
    for ( c.y = conpt.y; c.y < conpt.y + h; c.y++ ) {
        for ( c.x = conpt.x; c.x < conpt.x + w; c.x++ ) {
            i = con_to_buf(c);
            cell = dos_cell(i);
            dos_drawchar(*cell, i);
        }
    }
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
    
    printf("Init window with size (%d x %d).\n", w, h);
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
    printf("Init renderer with scale %d.\n", dos_scale());
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
    
    printf("Init screen with size (%d x %d).\n", w, h);
    
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
    printf("Init text buffer with size (%d x %d).\n",
           text.info.screenwidth, text.info.screenheight);
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
    printf("Init sound.\n");
}


/* get any events and put them in keybuf and mousebuf */
static void
fill_input_buffers (void)
{
    SDL_Event event;

    dos_empty(&keybuf);
    dos_empty(&mousebuf);
    
    modifiers = SDL_GetModState();
    
    while ( SDL_PollEvent(&event) ) {
        switch ( event.type ) {
            case SDL_QUIT:
                exit(EXIT_SUCCESS);
                break;
            case SDL_KEYDOWN:
                dos_push(&keybuf, event.key.keysym.sym);
                break;
            case SDL_MOUSEBUTTONDOWN:
                dos_push(&mousebuf, event.button.button);
                break;
            default:
                break;
        }
    }
    
    /* SDL_StopTextInput(); TODO: Text input? */
}


void
initdos (void)
{
    printf("\nLibDOS v. "VERSION"\n");
    printf("Thomas Foster (www.github/teefoss)\n\n");
    
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ) {
        printf("could not init sdl: %s\n", SDL_GetError());
        exit(1);
    }
    printf("Init SDL2.\n");
    
    initwin();
    initrenderer();
    initscreen();
    inittextbuffer();
    initsound();
    printf("Init complete.\n");
    
    refresh(); /* make sure the window appears */
    
    atexit(quitdos);
}


void
refresh (void)
{
    fill_input_buffers();
    
    dos_setcga(bdrcolor);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderClear(renderer);
    
    SDL_RenderCopy(renderer, screen, NULL, &screenrect);

    if ( curstype != CURSOR_NONE && blink(CURSOR_BLINK_INTERVAL) ) {
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
    
    /* reset text window and cursor */
    text.info.curx = 1;
    text.info.cury = 1;
    text.info.winleft = 1;
    text.info.wintop = 1;
    text.info.winright = width;
    text.info.winbottom = height;
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


void savescr(const char * file)
{
    SDL_Texture * target;
    SDL_Surface * surface;
    int w, h;
    
    target = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, screen);
    SDL_QueryTexture(screen, NULL, NULL, &w, &h);
    surface = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
    
    SDL_RenderReadPixels(renderer,
                         NULL,
                         surface->format->format,
                         surface->pixels,
                         surface->pitch);
    
    if ( SDL_SaveBMP(surface, file) != 0 ) {
        printf("savescr: could not save %s!\n", file);
    }
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, target);
}


void randomize(void)
{
    srand((unsigned)time(NULL));
}


int getmod()
{
    return modifiers;
}
