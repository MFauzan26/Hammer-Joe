#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "wasm4.h"

#define spriteWidth  8
#define spriteHeight 8
#define spriteFlags  BLIT_2BPP

uint8_t previousGamepad;

// map ///////////////////
int scene  = 0; // 0.title 1.ready 2.game 3.over

const uint8_t block[2][16] =
{
    { 0x00,0x00,0x05,0x60,0x1a,0xa8,0x1a,0xa8,0x2a,0xa0,0x2a,0x88,0x0a,0x20,0x00,0x00 },
    { 0x00,0x00,0x6a,0x05,0xaa,0x1a,0xa8,0x2a,0x00,0x00,0x05,0x6a,0x1a,0xaa,0x2a,0xa8 }
};

int cellHeight=15, cellWidth=20;
int mapLevel1[15][20] =
{
    {2,2,2,2,2,2,2,2,0,0,0,0,2,2,2,2,2,2,2,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,1,1,1,1,0,0,0,0,0,0,1,1,1,1,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,2},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {2,2,2,2,2,2,2,2,0,0,0,0,2,2,2,2,2,2,2,2}
};

void renderMap()
{
    for(int i=0; i<cellHeight; i++)
    {
        for(int j=0; j<cellWidth; j++)
        {
            *DRAW_COLORS = 0x1230;
            if(mapLevel1[i][j]!=0) blit(block[mapLevel1[i][j]-1], j*8, i*8, spriteWidth, spriteHeight, spriteFlags);
        }
    }
} // drawing map

int solidMap(int mx, int my)
{
    for(int i=0; i<cellHeight; i++)
    {
        for(int j=0; j<cellWidth; j++)
        {
            if(mapLevel1[i][j]!=0)
            {
                if(mx>j*8 && mx<(j*8)+8 && my>i*8 && my<(i*8)+8) return 1;
            }
        }
    }
    return 0;
} // map collision

//////////////////////////

// convert int to string
char* inttostr(int val) // thanks dave
{
    static char buf[32] = {0};
    int count = 0,i=0, n=val;
    if(val == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return &buf[0];
    }
    while(n!=0)  
    {  
       n=n/10;
       count++;  
    }  
    count--;

    while(val!=0)  
    {  
       char digit = val%10;
       val=val/10;
       buf[count-i] = digit + 48;
       i++;  
    }

    buf[count+1] = '\0';
    return &buf[0];
} // int to string

int intLength(int x)
{
    int l=1;
    while(x>9){ l++; x/=10; }
    return l;
}  // length of int

int t = 0;

int score=0, timer=0, life=0;
void HUD()
{
    // frame
    *DRAW_COLORS = 0x20;
    rect(1, 126, 158, 34);
    *DRAW_COLORS = 0x30;
    rect(2, 127, 156, 32);
    *DRAW_COLORS = 2;
    line(48, 126, 48, 160);
    line(96, 126, 96, 160);
    *DRAW_COLORS = 3;
    line(47, 128, 47, 158);
    line(49, 128, 49, 158);
    line(97, 128, 97, 158);
    line(95, 128, 95, 158);
    // text
    // life /////////////////////////////////////
    *DRAW_COLORS = 3;
    text("LIFE", 8, 134);
    *DRAW_COLORS = 4;
    text("X", 15, 144);
    text(inttostr(life), 25, 144);
    // time /////////////////////////////////////
    *DRAW_COLORS = 3;
    text("TIME", 56, 134);
    int tX = intLength(timer);
    *DRAW_COLORS = 4;
    text(inttostr(timer), 80-((tX-1)*8), 144);
    // score ///////////////////////////////////
    *DRAW_COLORS = 3;
    text("SCORE", 102, 134);
    int sX = intLength(score);
    *DRAW_COLORS = 4;
    text(inttostr(score), 144-((sX-1)*8), 144);
    ///////////////////////////////////////////
    // limit skor
    if(score>999999) score=999999;
    if(score<0)      score=0;
} // HUD

// object ///////////////// thanks BizarroFreak_999
struct playerStruct
{
    int    faceLeft, anim, att, tAtt, isDed, tDed;
    float  x, y, vx, vy;
}p;

// circle
struct circleStruct {
    float x, y;
    int   ang, t;
};
struct circleStruct* circle_array = NULL;
unsigned int circle_array_length;
void circleAdd(int x, int y, int a)
{
    unsigned int new_length = circle_array_length + 1;
    struct circleStruct* new_array = realloc(circle_array, new_length*(sizeof(struct circleStruct)));
    if(new_array==NULL) {return;}
    circle_array                        = new_array;
    circle_array_length                 = new_length;
    struct circleStruct new_element     = {.x=x, .y=y, .ang=a};
    circle_array[circle_array_length-1] = new_element;
}
void circleRemove(unsigned int index)
{
    if(circle_array_length == 1)
    {
         free(circle_array);
         circle_array = NULL;   
    }
    else
    {
        memmove(&circle_array[index], &circle_array[index+1], sizeof(struct circleStruct)*circle_array_length-index-1);
    }
    --circle_array_length;
}
void circleDraw()
{
    for(int i=(int)circle_array_length-1; i>=0; --i)
    {
        ++circle_array[i].t;
        if(circle_array[i].t==20) circleRemove(i);
        // movement
        float dir = circle_array[i].ang * (M_PI / 180);
        circle_array[i].x += sin(dir) * 2;
        circle_array[i].y -= cos(dir) * 2;
        // draw
        *DRAW_COLORS = 0x44;
        oval(
            circle_array[i].x,
            circle_array[i].y,
            4,
            4
       );
    }
} // circle

// enemy //
const uint8_t enemy1_1[2][16] = 
{    
    { 0x05,0x40,0x15,0x50,0x5a,0xa8,0x5b,0xac,0x6b,0xac,0x6a,0xa8,0x26,0xa0,0x04,0x10 },
    { 0x00,0x00,0x05,0x40,0x15,0x50,0x5a,0xa8,0x5b,0xac,0x6b,0xac,0x6a,0xa8,0x26,0xa0 }
};
const uint8_t ghost[2][16] = 
{
    { 0x01,0x54,0x05,0x55,0x05,0x99,0x05,0x99,0x15,0x55,0x51,0x54,0x00,0x00,0x00,0x00 },
    { 0x00,0x00,0x01,0x54,0x05,0x55,0x45,0x99,0x55,0x99,0x15,0x55,0x01,0x54,0x00,0x00 }
};

struct enemyStruct
{
    int   type, anim, isDam, isStun, isHamm, tHamm;
    float x, y, vx, vy;
};
struct enemyStruct* enemy_array = NULL;
unsigned int enemy_array_length;
void enemyAdd(int x, int y, int t)
{
    unsigned int new_length = enemy_array_length + 1;
    struct enemyStruct* new_array = realloc(enemy_array, new_length*(sizeof(struct enemyStruct)));
    if(new_array==NULL) {return;}
    enemy_array                       = new_array;
    enemy_array_length                = new_length;
    struct enemyStruct new_element    = {.x=x, .y=y, .vx=0.4, .type=t};
    enemy_array[enemy_array_length-1] = new_element;
}
void enemyRemove(unsigned int index)
{
    if(enemy_array_length == 1)
    {
         free(enemy_array);
         enemy_array = NULL;   
    }
    else
    {
        memmove(&enemy_array[index], &enemy_array[index+1], sizeof(struct enemyStruct)*enemy_array_length-index-1);
    }
    --enemy_array_length;
}
void enemyDraw()
{
    for(int i=(int)enemy_array_length-1; i>=0; --i)
    {
        // collision
        int l = enemy_array[i].x + enemy_array[i].vx,
            r = enemy_array[i].x + enemy_array[i].vx + 8,
            u = enemy_array[i].y + enemy_array[i].vy,
            d = enemy_array[i].y + enemy_array[i].vy + 8;
        if(l<0)   enemy_array[i].x=160;
        if(l>160) enemy_array[i].x=0;
        if(u<0)   enemy_array[i].y=120;
        if(u>120) enemy_array[i].y=0;
        // enemy 1
        if(enemy_array[i].type == 1)
        {
            if(!enemy_array[i].isHamm)
            {
                // move
                if(solidMap(l,u+5) || solidMap(r,u+5)) enemy_array[i].vx*=-1;
                // gravity
                if   (solidMap(l+1,d+1) || solidMap(r-1,d+1) || l<2 || l>158) enemy_array[i].vy=0;
                else enemy_array[i].vy += 0.2;
            }
            // draw
            if(!enemy_array[i].isDam) enemy_array[i].anim = t%16/8;
            int flags     = enemy_array[i].vx < 0 ? BLIT_FLIP_X :  0;
            *DRAW_COLORS  = enemy_array[i].isDam  ? 0x0430      : 0x0320;
            blit(
                enemy1_1[enemy_array[i].anim],
                enemy_array[i].x,
                enemy_array[i].y,
                spriteWidth,
                spriteHeight,
                spriteFlags | flags
            );
        }
        // enemy 2
        else if(enemy_array[i].type == 2)
        {
            if(!enemy_array[i].isHamm)
            {
                if(enemy_array[i].isDam)
                {
                    // move
                    if(solidMap(l,u+5) || solidMap(r,u+5)) enemy_array[i].vx*=-1;
                    // gravity
                    if   (solidMap(l+1,d+1) || solidMap(r-1,d+1) || l<2 || l>158) enemy_array[i].vy=0;
                    else enemy_array[i].vy += 0.2;
                }
                else
                {
                    // move
                    float dx   = p.x - l;
                    float dy   = p.y - u;
                    float dist = sqrt((dx*dx) + (dy*dy));
                    enemy_array[i].vx = dx / dist * 0.4;
                    enemy_array[i].vy = dy / dist * 0.4;
                }
            }
            // draw
            if(!enemy_array[i].isDam) enemy_array[i].anim = t%16/8;
            int flags     = enemy_array[i].x > p.x ? BLIT_FLIP_X :  0;
            *DRAW_COLORS  = enemy_array[i].isDam   ? 0x0430      : 0x0320;
            blit(
                ghost[enemy_array[i].anim],
                enemy_array[i].x,
                enemy_array[i].y,
                spriteWidth,
                spriteHeight,
                spriteFlags | flags
            );
        }
        // attack
        if(!enemy_array[i].isDam && p.x+4>l && p.x+4<r && p.y+4>u && p.y+4<d)
        {
            p.isDed = 1;
            if(enemy_array[i].type==2) {enemyRemove(i);}
        }
        // damaged by player
        if(p.att && l+4>p.x-10 && l+4<p.x+14 && u+4>p.y && u+4<p.y+8)
        {
            enemy_array[i].isDam = 1;
            enemy_array[i].vx    = 0;
        }
        // damaged by enemy
        for(int j=0; j<(int)enemy_array_length; ++j)
        {
            if(!enemy_array[i].isDam && enemy_array[j].isDam && l+4>enemy_array[j].x && l+4<enemy_array[j].x+8 && u+4>enemy_array[j].y && u+4<enemy_array[j].y+8)
            {
                enemy_array[i].isDam = 1;
                enemy_array[i].vx    = 0;
            }
        }
        // stun
        if(enemy_array[i].isDam)
        {
            ++enemy_array[i].tHamm;
            if(enemy_array[i].tHamm == 12) enemy_array[i].isStun = 1;
            // by player body
            if(l+4>p.x && l+4<p.x+8 && u+4>p.y && u+4<p.y+8)
            {
                enemy_array[i].vx = l+4>p.x+4 ? 2 : -2;
            } 
            // by hammer
            if(enemy_array[i].isStun && p.att && l+4>p.x-10 && l+4<p.x+14 && u+4>p.y && u+4<p.y+8)
            {
                enemy_array[i].isHamm =  1;
                enemy_array[i].vy     = -2;
                enemy_array[i].vx     =  l+4>p.x+4 ? 2 : -2;
            }
            if(enemy_array[i].isHamm)
            {
                if(solidMap(l,u+4) || solidMap(r,u+4)) enemy_array[i].vx*=-1;
                if(solidMap(l+4,u) || solidMap(l+4,d)) enemy_array[i].vy*=-1;
            }
        }
        // update movement
        enemy_array[i].x += enemy_array[i].vx;
        enemy_array[i].y += enemy_array[i].vy;
        // dead
        if(enemy_array[i].tHamm==60*5)
        {
            score += 50;
            enemyRemove(i);
        }
    }
} // enemy

// spawner //
const uint8_t spawner[16] = { 0x00,0x55,0x54,0xaa,0xa8,0x55,0x54,0x55,0x54,0x55,0x54,0x55,0x00,0x55,0x00,0x00 };
struct spawnStruct{int x, y, flip, type, timeT, time;};
struct spawnStruct* spawn_array = NULL;
unsigned int spawn_array_length;
void spawnAdd(int x, int y, int flip, int type, int time)
{
    unsigned int new_length = spawn_array_length + 1;
    struct spawnStruct* new_array = realloc(spawn_array, new_length*(sizeof(struct spawnStruct)));
    if(new_array==NULL) {return;}
    spawn_array                       = new_array;
    spawn_array_length                = new_length;
    struct spawnStruct new_element    = {.x=x, .y=y, .flip=flip, .type=type, .time=time};
    spawn_array[spawn_array_length-1] = new_element;
}
void spawnRemove(unsigned int index)
{
    if(spawn_array_length == 1)
    {
         free(spawn_array);
         spawn_array = NULL;   
    }
    else
    {
        memmove(&spawn_array[index], &spawn_array[index+1], sizeof(struct spawnStruct)*spawn_array_length-index-1);
    }
    --spawn_array_length;
}
void spawnDraw()
{
    for(int i=(int)spawn_array_length-1; i>=0; --i)
    {
        // spawn enemy
        ++spawn_array[i].timeT;
        if(enemy_array_length<4 && spawn_array[i].timeT>spawn_array[i].time)
        {
            enemyAdd(spawn_array[i].x, spawn_array[i].y, spawn_array[i].type);
            spawn_array[i].timeT = 0;
        }
        // draw
        *DRAW_COLORS  = 0x4321;
        blit(
            spawner,
            spawn_array[i].x,
            spawn_array[i].y,
            spriteWidth,
            spriteHeight,
            spriteFlags | spawn_array[i].flip
        );
    }
} // spawner

// player
const uint8_t player[2][16] =
{
    { 0x15,0x51,0x0a,0xa1,0x7f,0xf5,0x7e,0x25,0x7c,0x05,0x5f,0xd5,0x5f,0xd5,0x51,0x15 },
    { 0x00,0x00,0x40,0x04,0x5a,0xa4,0x3f,0xf0,0x3e,0x60,0x3d,0x50,0x0f,0xc0,0x10,0x10 }
};
void drawPlayer()
{
    // input
    uint8_t gamepad          = *GAMEPAD1;
    uint8_t pressedThisFrame = gamepad & (gamepad ^ previousGamepad);
    previousGamepad          = gamepad;
    if(gamepad & BUTTON_LEFT)  
    {
        p.vx       = -1;
        p.faceLeft =  1;
        p.anim     = t%10/5;
    }
    else if(gamepad & BUTTON_RIGHT) 
    {
        p.vx       = 1;
        p.faceLeft = 0;
        p.anim     = t%10/5;
    }
    else
    {
        p.vx   = 0;
        p.anim = 0;
    }
    // attack
    if(!p.att && pressedThisFrame & BUTTON_1) p.att=1;
    if(p.att)
    {
        ++p.tAtt;
        if(p.tAtt==10) {p.att=0; p.tAtt=0;}
    }
    // collision
    int l=p.x+p.vx, r=p.x+p.vx+8, u=p.y+p.vy, d=p.y+p.vy+8;
    if(solidMap(l,u+2) || solidMap(l,d) || solidMap(r,u+2) || solidMap(r,d)) p.vx=0;
    if(l<0)   p.x=160;
    if(l>160) p.x=0;
    if(u<0)   p.y=120;
    if(u>120) p.y=0;
    // up collision
    if(solidMap(l+1,u+1) || solidMap(r-1,u+1)) p.vy=0;
    // gravity
    if   (solidMap(l+1,d+1) || solidMap(r-1,d+1) || l<2 || l>158) p.vy=0;
    else p.vy += 0.2;
    // jump
    if(p.vy==0 && pressedThisFrame & BUTTON_2)
    {
        p.vy=-3;
        tone(262 | (523 << 16), 20, 100, TONE_PULSE1);
    }
    // update movement
    p.x += p.vx;
    p.y += p.vy;
    // death
    if(p.isDed)
    {
        ++p.tDed;t=0;
        if(p.tDed==1) {for(int i=0; i<360; i+=45) circleAdd(p.x, p.y, i);}
        if(life>0)
        {
            if     (p.tDed==1)  {tone(262 | (523 << 16), 40, 100, TONE_NOISE); life-=1;}
            else if(p.tDed==50) {p.x=76; p.y=56; p.tDed=0; p.isDed=0;}
        }
        else
        {
            if(p.tDed==70) 
            {
                scene=3;
            }
        }
    }
    // draw
    if(!p.isDed)
    {
        int flags    = p.faceLeft ? BLIT_FLIP_X : 0;
        *DRAW_COLORS = p.anim     ? 0x2430      : 0x2403;
        blit(
            player[p.anim],
            p.x,
            p.y,
            spriteWidth,
            spriteHeight,
            spriteFlags | flags
        );
    }
} // player

const uint8_t hammer[2][16] = 
{
    { 0x00,0x40,0x02,0x50,0x05,0x94,0x25,0x64,0x59,0x50,0x16,0x60,0x05,0x08,0x00,0x02 },
    { 0x00,0x54,0x01,0x55,0x01,0xa9,0xa9,0x55,0x01,0xa9,0x01,0x55,0x00,0x54,0x00,0x00 }
    
};
void drawHammer()
{
    int hammerSpr, offsetX, offsetY;
    if(p.att)
    {
        hammerSpr =  1;
        offsetY   =  0;
        offsetX   = p.faceLeft ? -5 : 5;
    }
    else
    {
        hammerSpr =  0;
        offsetY   = -5;
        offsetX   = p.faceLeft ? 5 : -5;
    }
    if(!p.isDed)
    {
        int flags     = p.faceLeft ? BLIT_FLIP_X :  0;
        *DRAW_COLORS  = 0x1320;
        blit(
            hammer[hammerSpr],
            p.x+offsetX,
            p.y+offsetY,
            spriteWidth,
            spriteHeight,
            spriteFlags | flags
        );
    }
} // hammer

///////////////////////////

void palette() // EN4
{
    PALETTE[0] = 0x20283d; // hitam  (1)
    PALETTE[1] = 0x426e5d; // hijau  (2)
    PALETTE[2] = 0xe5b083; // krem   (3)
    PALETTE[3] = 0xfbf7f3; // putih  (4)
} // palette

int timeRe = 0; // time ready and over
int init   = 0;
void gameInit()
{
    spawnAdd(9, 80, 0, 1, 300);spawnAdd(143, 80, BLIT_FLIP_X, 1, 300);
    spawnAdd(36, 9, BLIT_FLIP_X|BLIT_ROTATE, 2, 600);spawnAdd(116, 9, BLIT_FLIP_X|BLIT_ROTATE, 2, 900);
    timer=0;
    p.x=76; p.y=56;
}

void draw()
{
    uint8_t gamepad = *GAMEPAD1;
    palette();
    // title
    if(scene == 0)
    {
        *DRAW_COLORS = (t%40/20)+1;
        text("PRESS X TO START!", 14, 80);
        *DRAW_COLORS = 4;
        text("GAME BY MFAUZAN", 20, 120);
        *DRAW_COLORS = (t%18/9)+2;
        text("HAMMER JOE", 40, 32);
        uint8_t gamepad          = *GAMEPAD1;
        uint8_t pressedThisFrame = gamepad & (gamepad ^ previousGamepad);
        previousGamepad          = gamepad;
        if(pressedThisFrame & BUTTON_1)
        {
            // reset
            life   =3;
            score  =0;
            ////////
            scene=1;
        }
    }
    // get ready
    else if(scene == 1)
    {
        // reset
        init=0;p.isDed=0;p.tDed=0;
        ////////
        ++timeRe;
        *DRAW_COLORS = 3;
        text("GET READY!", 40, 80);
        if(timeRe==180) {scene=2;timeRe=0;}
    }
    // game
    else if(scene == 2)
    {
        if(!init) {gameInit();init=1;}
        renderMap();
        // object
        enemyDraw();
        drawHammer();
        drawPlayer();
        spawnDraw();
        /////////
        HUD();
        circleDraw();
    }
    // game over
    else if(scene == 3)
    {
        t=0;
        p.tDed=0;
        *DRAW_COLORS = 2;
        text("GAME OVER", 40, 56);
        *DRAW_COLORS = 3;
        text("FINAL SCORE", 36, 88);
        text(inttostr(score+(timer*10)), 76-(intLength(score)*8)/2, 104);
        // delete object
        if(timeRe<10) ++timeRe;
        if(timeRe==10)
        {
            free(enemy_array);
            free(spawn_array);
            enemy_array = NULL;
            spawn_array = NULL; 
            enemy_array_length = 0;
            spawn_array_length = 0;
        }
        ///////////////////////
        uint8_t gamepad          = *GAMEPAD1;
        uint8_t pressedThisFrame = gamepad & (gamepad ^ previousGamepad);
        previousGamepad          = gamepad;
        if(pressedThisFrame & BUTTON_1) {scene=0;timeRe=0;}
    }
}
/*
void debug()
{
    *DRAW_COLORS = 4;
    text(inttostr(enemy_array_length),  16, 16);
    text(inttostr(spawn_array_length),  16, 24);
    text(inttostr(circle_array_length), 16, 32);
    text(inttostr(timeRe), 16, 48);
}
*/
void update () 
{
    ++t;
    if(t==60) {t=0;timer+=1;}
    //debug();
    draw();
}
