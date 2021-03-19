#include "dos.h"
#include "conio.h"
#include "internal.h"

#include <stdbool.h>

#define TEXT_BLINK_INTERVAL     250
#define CURSOR_BLINK_INTERVAL   150

//static const unsigned char * fontdata = fontdata80;

static SDL_Window *     _window;
static SDL_Texture *    screen;
static SDL_Rect         screenrect;
/* TODO: dirtyrect? */

static char * winname;
static int scale = 1;

unsigned char bordersize = 4;
static unsigned char _bordercolor = BLACK;

static int curstype = CURSOR_NORMAL;
static unsigned seed;

//static unsigned _frequency; /* for _sound_callback() */
static SDL_AudioSpec spec;
static SDL_AudioDeviceID device;

SDL_Renderer *  renderer;
const int       _textwidth = 8;
int             _textheight = MODE80H;
short *         _textbuffer;

const unsigned char * fontdata = fontdata80;

int _base = 1;

TEXT_INFO textinfo =
{
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
};

#if 0
static const SDL_Color palette[] = {
    { 0x00, 0x00, 0x00, 0xff },
    { 0x00, 0x00, 0xa8, 0xff },
    { 0x00, 0xa8, 0x00, 0xff },
    { 0x00, 0xa8, 0xa8, 0xff },
    { 0xa8, 0x00, 0x00, 0xff },
    { 0xa8, 0x00, 0xa8, 0xff },
    { 0xa8, 0x54, 0x00, 0xff },
    { 0xa8, 0xa8, 0xa8, 0xff },
    { 0x54, 0x54, 0x54, 0xff },
    { 0x54, 0x54, 0xfe, 0xff },
    { 0x54, 0xfe, 0x54, 0xff },
    { 0x54, 0xfe, 0xfe, 0xff },
    { 0xfe, 0x54, 0x54, 0xff },
    { 0xfe, 0x54, 0xfe, 0xff },
    { 0xfe, 0xfe, 0x54, 0xff },
    { 0xfe, 0xfe, 0xfe, 0xff },
};
#endif

#pragma mark - PRIVATE

short * _txtbufcell(int x, int y)
{
    return _textbuffer + y * textinfo.screenwidth + x;
}


short * _curtxtbufcell()
{
    int x = textinfo.curx - _base;
    int y = textinfo.cury - _base;
    
    return _txtbufcell(x, y);
}


int _maxtextx()
{
    return textinfo.screenwidth - 1 + _base;
}


int _maxtexty()
{
    return textinfo.screenheight - 1 + _base;
}


int _curxi()
{
    return textinfo.cury - _base;
}

int _clamp(int x, int min, int max)
{
    if ( x < min )
        x = min;
    else if (x > max)
        x = max;
    
    return x;
}

int _scale()
{
    if ( textinfo.currmode == BW40 || textinfo.currmode == C40) {
        return scale + 1;
    }
    return scale;
}

static SDL_Color _cgacolor(COLOR c)
{
    SDL_Color result;
    int i;
    
    i = c & 8 ? 0x55 : 0;
    
    result.r = ((c & 4) >> 2) * 0xAA + i;
    result.g = ((c & 2) >> 1) * 0xAA + i;
    result.b = ((c & 1) >> 0) * 0xAA + i;
    
    if ( c == BROWN ) {
        result.g -= 0x55; // tweak dark yellow -> brown
    }
    
    return result;
}


void _setcga(COLOR c)
{
    SDL_Color set;
    
    switch ( textinfo.currmode ) {
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
    
    set = _cgacolor(c);
    SDL_SetRenderDrawColor(renderer, set.r, set.g, set.b, 255);
}


static bool _blink(int interval)
{
    return ( (SDL_GetTicks() / interval) & 1 ) == 0;
}


static void _limitframerate(int fps)
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


static void _rendercursor(void)
{
    SDL_Rect r;
    
    r.x = (textinfo.curx - _base) * _textwidth;
    r.w = _textwidth;
    
    switch ( curstype ) {
        case CURSOR_NORMAL:
            r.h = 2;
            r.y = ((textinfo.cury - _base) * _textheight) + _textheight - r.h;
            break;
        case CURSOR_SOLID:
            r.h = _textheight;
            r.y = (textinfo.cury - _base) * _textheight;
            break;
        default:
            break;
    }

    _setcga(textinfo.attribute & 0x0F);
    
    SDL_RenderSetViewport(renderer, &screenrect);
    SDL_RenderFillRect(renderer, &r);
    SDL_RenderSetViewport(renderer, NULL);
}


void _drawchar(short cell, int x, int y)
{
    unsigned char fg, bg;
    const unsigned char * data;
    int start, stop, step; /* font data row draw order */
    
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
    if ( textinfo.currmode == C40 || textinfo.currmode == BW40 ) {
        start = _textwidth - 1;
        stop = -1;
        step = -1;
    } else { // 80
        start = 0;
        stop = _textheight;
        step = 1;
    }
    
    data = &fontdata[(CELL_CH(cell)) * _textheight];
    for ( int cy = 0; cy < _textheight ; cy++ ) {
        
        for ( int cx = start, draw_x = 0; cx != stop; cx += step ) {
            *data & (1 << cx) ? _setcga(fg) : _setcga(bg);
            SDL_RenderDrawPoint(renderer,
                                x * _textwidth + draw_x++,
                                y * _textheight + cy);
        }
        data++;
    }
}


static void
_initwin (void)
{
    int x, y, w, h;
    int border;
        
    border = bordersize * 2 * _scale();
    
    x = SDL_WINDOWPOS_CENTERED;
    y = SDL_WINDOWPOS_CENTERED;
    w = (textinfo.screenwidth * _textwidth * _scale()) + border;
    h = (textinfo.screenheight * _textheight * _scale()) + border;
        
    _window = SDL_CreateWindow(winname, x, y, w, h, 0);
    
    if ( _window == NULL ) {
        printf("initwin() failed: %s\n", SDL_GetError());
        quitdos();
    }
}


static void
_initrenderer ()
{
    renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_SOFTWARE);
    
    if ( renderer == NULL ) {
        printf("initrenderer() failed: %s\n", SDL_GetError());
        quitdos();
    }
    
    SDL_RenderSetScale(renderer, _scale(), _scale());
}


static void
_initscreen ()
{
    int format;
    int access;
    int w, h;
    
    format = SDL_PIXELFORMAT_RGBA8888;
    access = SDL_TEXTUREACCESS_TARGET;
    w = textinfo.screenwidth * _textwidth;
    h = textinfo.screenheight * _textheight;
    
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
_inittextbuffer (void)
{
    _textbuffer = malloc(TXTBUFSIZE);
    clrscr();
}


static void
_initsound (void)
{
    memset(&spec, 0, sizeof(spec));
    
    spec.freq = 44100;
    spec.format = AUDIO_S8;
    spec.channels = 1;
    spec.samples = 512;
    spec.callback = NULL;

    device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
}


static void
_input (void)
{
    SDL_Event event;

    kb.top = 0;
    _mousebuf.top = 0;
    
    while ( SDL_PollEvent(&event) ) {
        
        if ( kb.top == INBUFSIZE ) {
            return;
        }
        
        switch ( event.type ) {
            case SDL_QUIT:
                quitdos();
                break;
            case SDL_KEYDOWN:
                switch ( event.key.keysym.sym ) {
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                    case SDLK_RCTRL:
                    case SDLK_LCTRL:
                    case SDLK_RALT:
                    case SDLK_LALT:
                    case SDLK_RGUI:
                    case SDLK_LGUI:
                    case SDLK_NUMLOCKCLEAR:
                    case SDLK_CAPSLOCK:
                        return;
                    case SDLK_UP:
                        printf("up: %d\n", event.key.keysym.sym);
                    default:
                        kb.buffer[kb.top++] = event.key.keysym.sym;
                        break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                _mousebuf.buffer[_mousebuf.top++] = event.button.button;
                break;
            default:
                break;
        }
    }
}


#pragma mark - PUBLIC


void
initdos (void)
{
    seed = time(NULL);
    
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ) {
        printf("could not init sdl: %s\n", SDL_GetError());
        exit(1);
    }
    
    _initwin();
    _initrenderer();
    _initscreen();
    _inittextbuffer();
    _initsound();
    
    refresh(); /* make sure the window appears */
}


void
quitdos (void)
{
    if ( winname ) {
        free(winname);
    }
    
    if ( _textbuffer ) {
        free(_textbuffer);
    }
    
    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(_window);
    
    SDL_CloseAudio();
    SDL_Quit();
    
    exit(0);
}

void
refresh (void)
{
    _input();
    
    _setcga(_bordercolor);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderClear(renderer);
    
    SDL_RenderCopy(renderer, screen, NULL, &screenrect);

    if ( curstype != CURSOR_NONE && _blink(CURSOR_BLINK_INTERVAL) ) {
        _rendercursor();
    }
    
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, screen);
    
    _limitframerate(60);
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
        
    for ( int i = 0; i < len; i++ ) {
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
    textinfo.screenwidth = (unsigned char)width;
    textinfo.screenheight = (unsigned char)height;
}


void setbordersize(int newsize)
{
    if ( _window == NULL ) {
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
        
        if ( _window ) {
            SDL_SetWindowTitle(_window, winname);
        }
    }
}


void bordercolor(int newcolor)
{
    _bordercolor = (unsigned char)newcolor;
}
