//
// collect.c
// LibDOS demo game
// note: this is not extensively tested!
//
#include <dos.h>
#include <conio.h>
#include <graphics.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h> // temp

#define ROWS            15
#define COLS            40
#define PLAYER_CH       2
#define MONSTER_CH      148
#define TREE_CH1        5
#define TREE_CH2        6
#define GEM_CH          4
#define MAX_TREES       50
#define MAX_MONSTERS    10
#define MAX_GEMS        3
#define CORPSE_CH       '@'
#define JUMPS_PER_LEVEL 2

#define SIGN(x) ((x) < 0 ? -1 : (x) > 0 ? 1 : 0)

typedef struct {
    int x, y;
} POINT;

typedef enum {
    OBJ_PLAYER,
    OBJ_TREE,
    OBJ_MONSTER,
    OBJ_GEM,
    OBJ_CORPSE
} OBJ_TYPE;

typedef enum {
    SND_NONE,
    SND_DIE,
    SND_COLLECT
} SOUND;

typedef struct
{
    OBJ_TYPE    type;
    int         color;
    int         ch;
    POINT       pt;
    bool        alive;
} OBJ;

int level;
int points;
int jumps;
int num_monsters;
int num_trees;
OBJ player;
OBJ monsters[ MAX_MONSTERS ];
OBJ trees[ MAX_TREES ];
OBJ gems[ MAX_GEMS ];
SOUND play;


void draw_obj(OBJ *obj)
{
    gotoxy(obj->pt.x, obj->pt.y);
    textcolor(obj->color);
    putch(obj->ch);
}


void erase_obj(OBJ *obj)
{
    gotoxy(obj->pt.x, obj->pt.y);
    putch(0);
}


int get_char(int x, int y)
{
    short attr;
    gettext(x, y, x, y, &attr);
    
    return attr & 0xFF;
}


int get_color(int x, int y)
{
    short attr;
    gettext(x, y, x, y, &attr);
    
    return (attr & 0xFF00) >> 8;
}

POINT random_point()
{
    POINT list[ROWS * COLS];
    int count = 0;
    
    // make a list of available points
    for ( int y = 1; y <= ROWS; y++ ) {
        for ( int x = 1; x <= COLS; x++ ) {
            if ( !get_char(x, y) ) {
                list[count].x = x;
                list[count].y = y;
                count++;
            }
        }
    }
    
    return list[rand() % count];
}


bool same_point(POINT *p1, POINT *p2)
{
    return p1->x == p2->x && p1->y == p2->y;
}


int random_between(int min, int max)
{
    return rand() % (max - min + 1) + min;
}


void draw_status()
{
    gotoxy(1, 1);
    textcolor(YELLOW);
    cprintf("LEVEL %3d  ", level);
    textcolor(CYAN);
    cprintf("POINTS %3d  ", points);
    textcolor(MAGENTA);
    cprintf("JUMPS %3d", jumps);
}


void draw_everything()
{
    setbordercolor(DARKGRAY);
    clrscr();
    
    for ( int i = 0; i < num_trees; i++ ) {
        draw_obj(&trees[i]);
    }

    for ( int i = 0; i < num_monsters; i++ ) {
        draw_obj(&monsters[i]);
    }
    
    for ( int i = 0; i < MAX_GEMS; i++ ) {
        draw_obj(&gems[i]);
    }
    
    draw_obj(&player);
    draw_status();
}


void new_level(int num, int new_jumps)
{
    level = num;
    
    player.pt = random_point();
    player.alive = true;
    player.ch = PLAYER_CH;
    player.color = LIGHTBLUE;
    jumps = new_jumps;
    
    num_monsters = random_between(2, MAX_MONSTERS);
    for ( int i = 0; i < num_monsters; i++ ) {
        monsters[i].pt = random_point();
        monsters[i].alive = true;
        monsters[i].ch = MONSTER_CH;
        monsters[i].color = RED;
    }
    
    num_trees = random_between(0, MAX_TREES);
    for ( int i=0 ; i<num_trees ; i++ ) {
        trees[i].color = rand() % 1000 < 500 ? GREEN : LIGHTGREEN;
        trees[i].ch = rand() % 1000 < 500 ? TREE_CH1 : TREE_CH2;
        trees[i].pt = random_point();
    }
    
    for ( int i = 0; i < MAX_GEMS; i++ ) {
        gems[i].pt = random_point();
    }
    
    draw_everything();
}


void clamp(POINT *pt)
{
    if ( pt->x < 1 )
        pt->x = 1;
    else if ( pt->x > COLS )
        pt->x = COLS;
    if ( pt->y < 1 )
        pt->y = 1;
    else if ( pt->y > ROWS )
        pt->y = ROWS;
}


void collect_sound()
{
    sound(440, 50);
    sound(550, 50);
    sound(660, 50);
    sound(880, 50);
}

void collect_gem(int color)
{
    switch ( color ) {
        case LIGHTMAGENTA:  points +=  25; break;
        case BLUE:          points +=  50; break;
        case WHITE:         points += 100; break;
        default: break;
    }
    
    play = SND_COLLECT;
    new_level(level + 1, jumps + JUMPS_PER_LEVEL);
}


void death_sound()
{
    sound(220, 50);
    sound(200, 50);
    sound(190, 50);
}


void kill(OBJ *obj)
{
    obj->alive = false;
    obj->ch = CORPSE_CH;
    obj->color = DARKGRAY;
    draw_obj(obj);
    if ( play != SND_COLLECT ) { // collect sound takes precedence
        play = SND_DIE;
    }
}


void kill_player()
{
    kill(&player);
    int prev = level - 1;
    if ( prev < 1 )
        prev = 1;
    
    points -= 50;
    if ( points < 0 )
        points = 0;
    
    new_level(prev, jumps);
}


bool move_obj(OBJ *obj, int dx, int dy)
{
    POINT try;

    try.x = obj->pt.x + dx;
    try.y = obj->pt.y + dy;
    clamp(&try);
    
    int hit = get_char(try.x, try.y);
    
    switch ( hit ) {
        case PLAYER_CH:
            if ( obj->type == OBJ_MONSTER ) {
                kill_player();
            }
            return false;
        case MONSTER_CH:
            if ( obj->type == OBJ_PLAYER) {
                kill_player();
            } else if ( obj->type == OBJ_MONSTER ) {
                kill(obj);
                for ( int i = 0; i < num_monsters; i++ ) {
                    if ( same_point(&monsters[i].pt, &try) ) {
                        kill(&monsters[i]);
                        break;
                    }
                }
            }
        case GEM_CH:
            if ( obj->type == OBJ_PLAYER ) {
                int color = get_color(try.x, try.y);
                collect_gem(color);
            } else {
                return false; // nothing else should move through gems
            }
            break;
        case CORPSE_CH:
            if ( obj->type == OBJ_MONSTER ) {
                kill(obj);
            }
            break;
        case TREE_CH1:
        case TREE_CH2:
            return false;
        default:
            break;
    }

    erase_obj(obj);
    
    obj->pt.x = try.x;
    obj->pt.y = try.y;
    
    draw_status();
    draw_obj(obj);
    
    return true;
}


void move_monsters()
{
    OBJ *m;
    int dx, dy;
    
    for ( int i = 0; i < num_monsters; i++ ) {
        m = &monsters[i];
        
        if ( !m->alive )
            continue;
        
        dx = SIGN(player.pt.x - m->pt.x);
        dy = SIGN(player.pt.y - m->pt.y);
        
        move_obj(m, dx, dy);
    }
}


void move_player(int dx, int dy)
{
    bool moved;
    
    moved = move_obj(&player, dx, dy);
    
    if ( moved ) {
        sound(1000, 20);
        points--;
        if ( points < 0 )
            points = 0;

        move_monsters();
    }
}


void jump()
{
    if ( jumps == 0 ) {
        sound(220, 100);
        sound(156, 100);
        return;
    }
    
    --jumps;
    POINT new = random_point();
    OBJ tmp = player;
    
    for ( int i = 0; i < 20; i++ ) {
        tmp.color = rand() % 16;
        draw_obj(&tmp);
        refresh();
        sound(random_between(500, 1000), 40);
    }
    
    erase_obj(&player);
    player.pt = new;
    draw_status();
    draw_obj(&player);
}


void init_objs()
{
    player.type = OBJ_PLAYER;
    
    for ( int i = 0; i < MAX_GEMS; i++ ) {
        gems[i].type = OBJ_GEM;
        gems[i].ch = GEM_CH;
    }
    gems[0].color = LIGHTMAGENTA;
    gems[1].color = BLUE;
    gems[2].color = WHITE;

    for ( int i = 0; i < MAX_TREES; i++ ) {
        trees[i].type = OBJ_TREE;
    }
    
    for ( int i = 0; i < MAX_MONSTERS; i++ ) {
        monsters[i].type = OBJ_MONSTER;
        monsters[i].color = RED;
        monsters[i].ch = MONSTER_CH;
    }
}

void title()
{
    clrscr();
    textcolor(YELLOW);  cprintf("LibDOS Demo Game\n\n");
    textcolor(WHITE);   cprintf("Movement:\n");
    
    textcolor(LIGHTGRAY);
    {
        cprintf("    %c\n", 24);
        cprintf("  Q W E\n");
        cprintf("%c A S D %c\n", 27, 26);
        cprintf("  Z X C\n");
        cprintf("    %c\n", 25);
    }
    textcolor(WHITE);       cprintf("Jump: ");
    textcolor(LIGHTGRAY);   cprintf("SPACE\n");
    textcolor(WHITE);       cprintf("Quit: ");
    textcolor(LIGHTGRAY);   cprintf("ESC\n\n");
    
    textcolor(WHITE);       cprintf("Collect: ");
    {
        textcolor(BLUE);
        putch(GEM_CH);
        textcolor(MAGENTA);
        putch(GEM_CH);
        textcolor(WHITE);
        putch(GEM_CH);
    }
    cprintf("\nAvoid: "); textcolor(RED); putch(MONSTER_CH);
    
    gotoxy(1, ROWS);
    textcolor(LIGHTGREEN);
    cprintf("\n\nPress any key to start...");
    
    while ( 1 ) {
        while ( kbhit() ) {
            getch();
            return;
        }
                
        refresh();
    }
}


int main()
{
    randomize();
    
    setscreensize(COLS, ROWS);
    setbordersize(8);
    setscale(3);
    setcursor(0);
    setname("collect.c");
    
    initdos();

    title();
    init_objs();
    new_level(1, JUMPS_PER_LEVEL);
    
    while ( 1 )
    {
        if ( kbhit() ) {
            int key = getch();
            
            switch ( key ) {
                case 'w': move_player(+0, -1); break;
                case 'a': move_player(-1, +0); break;
                case 'x': move_player(+0, +1); break;
                case 'd': move_player(+1, +0); break;
                case 'q': move_player(-1, -1); break;
                case 'e': move_player(+1, -1); break;
                case 'z': move_player(-1, +1); break;
                case 'c': move_player(+1, +1); break;
                case 's':
                    move_monsters();
                    sound(100, 40);
                    break;
                case ' ':
                    jump();
                    break;
                case 27:
                    return 0;
                default:
                    break;
            }
        }
        
        
        if ( play ) { // only one of these sounds should play per frame
            switch ( play ) {
                case SND_DIE: death_sound(); break;
                case SND_COLLECT: collect_sound(); break;
                default: break;
            }
            play = SND_NONE;
        }
        
        refresh();
    }
}
