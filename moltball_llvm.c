/*
 * MOLTBALL - JezzBall for C64
 * v8: parallel arrays, integer pixel movement, CIA TOD timer, smooth framerate.
 */

#include <c64.h>
#include <string.h>
#include <stdlib.h>
#include <peekpoke.h>

/* waitvsync — wait for raster line 255 (start of vblank) */
static void waitvsync(void) {
    while (PEEK(0xD012) != 255) {}
}

/* conio replacements — direct hardware access */
static unsigned char kbhit(void) {
    /* C64 keyboard buffer count at $C6 */
    return PEEK(0xC6);
}
static unsigned char cgetc(void) {
    unsigned char c;
    while (!kbhit()) {}
    c = PEEK(0x0277);  /* first key in buffer */
    POKE(0xC6, PEEK(0xC6) - 1);
    return c;
}

#define PF_X       2
#define PF_Y       3
#define PF_W      36
#define PF_H      19

#define PF_PX_L   (PF_X * 8)
#define PF_PX_R   ((PF_X + PF_W) * 8 - 1)
#define PF_PY_T   (PF_Y * 8)
#define PF_PY_B   ((PF_Y + PF_H) * 8 - 1)

#define SPR_OFS_X  24
#define SPR_OFS_Y  50

#define VIC_BASE     0xD000
#define VIC_SPR_EN   0xD015
#define VIC_SPR_XHI  0xD010
#define VIC_SPR_COL  0xD027
#define VIC_SPR_MC   0xD01C
#define VIC_SPR_MCR0 0xD025
#define VIC_SPR_MCR1 0xD026
#define VIC_SPR_XE   0xD01D
#define VIC_SPR_YE   0xD017
#define VIC_SPR_PRI  0xD01B
#define VIC_D018     0xD018
#define VIC_BG       0xD021
#define VIC_BORDER   0xD020

#define SCREEN_BASE  0x4C00
#define COLRAM_BASE  0xD800
#define SPRITE_PTR   (SCREEN_BASE + 0x3F8)
#define SPRITE_DATA  0x4800
#define CHARSET_DATA 0x4000

#define SPR_CUR_H   (0x0800 / 64)
#define SPR_CUR_V   (0x0840 / 64)
#define SPR_BALL_0   (0x0880 / 64)
#define BALL_FRAMES  6

#define CH_TILE      128
#define CH_FILLED    32
#define CH_WALL      128

#define COL_BG       COLOR_BLACK
#define COL_TILE     COLOR_GRAY3
#define COL_WALL_A   COLOR_RED
#define COL_WALL_B   COLOR_BLUE
#define COL_WALL_DONE COLOR_BLACK

#define JOY2_PORT  0xDC00
#define JOY2_UP    0x01
#define JOY2_DOWN  0x02
#define JOY2_LEFT  0x04
#define JOY2_RIGHT 0x08
#define JOY2_FIRE  0x10

#define MAX_BALLS    7
#define MAX_WALLS    2
#define CURSOR_SPEED 6

#define PF_OPEN      0
#define PF_SOLID     1
#define PF_WALL_A    2
#define PF_WALL_B    3
#define PF_MARK      4

/* Ball hitbox offsets from sprite origin */
#define BHB_L  6
#define BHB_T  4
#define BHB_R  17
#define BHB_B  15

/* Fast 16-bit >> 3 when result fits in 8 bits (max input ~320) */
#define SHR3(v) ((unsigned char)((unsigned char)((unsigned int)(v) >> 8) << 5 | (unsigned char)((unsigned char)(v) >> 3)))

/* --- Ball data as parallel arrays (cc65-friendly) --- */
static unsigned int  bx[MAX_BALLS];   /* pixel X position */
static unsigned char by[MAX_BALLS];   /* pixel Y position (always < 200) */
signed char   bdx[MAX_BALLS];  /* X direction: +1 or -1 */
signed char   bdy[MAX_BALLS];  /* Y direction: +1 or -1 */
static unsigned char bact[MAX_BALLS];  /* active flag */
static unsigned char banim[MAX_BALLS]; /* animation phase */

/* Wall builders */
static unsigned char wcx[MAX_WALLS], wcy[MAX_WALLS];
static signed char wdcx[MAX_WALLS], wdcy[MAX_WALLS];
static unsigned char wact[MAX_WALLS], wcol[MAX_WALLS], wpfv[MAX_WALLS];

static unsigned char num_balls, level, lives, game_over, joy_prev, frame_count, wall_dir, last_cursor_dir;
static unsigned char joy_hold_frames;  /* frames joystick held in same direction */
static unsigned int score, cursor_px, total_cells, filled_cells;
static unsigned char cursor_py;
static unsigned int fill_target;
static unsigned int time_left;
static unsigned char frames_per_sec;
static unsigned char time_frames;

/* Collision grid */
static unsigned char pfmap[1000];
unsigned char *pfr[25];
unsigned int scr_row[25];

/* Flood fill — two byte arrays instead of one int array (saves 700 bytes) */
#define FILL_STACK_SZ 200
unsigned char fsx[FILL_STACK_SZ];  /* x coords */
unsigned char fsy[FILL_STACK_SZ];  /* y coords */
unsigned char fsp;

/* Deferred visual fill */
unsigned char fv_active, fv_row;

/* Sprite bit lookup */
static const unsigned char spr_bit[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

/* Sprite screen position arrays — filled by game logic, flushed by asm */
unsigned char spr_x_lo[8];  /* low 8 bits of screen X */
unsigned char spr_x_hi[8];  /* bit 0 = MSB of screen X (0 or 1) */
unsigned char spr_y[8];     /* screen Y */


/* ---- Screen helpers ---- */

static void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char col) {
    unsigned int o = scr_row[y] + x;
    POKE(SCREEN_BASE + o, ch);
    POKE(COLRAM_BASE + o, col);
}

static void scrwrite(unsigned char x, unsigned char y, const char *s, unsigned char col) {
    unsigned int off = scr_row[y] + x;
    while (*s) {
        unsigned char c = *s;
        if (c >= 'a' && c <= 'z') c = c - 'a' + 1;
        else if (c >= 'A' && c <= 'Z') c = c - 'A' + 1;
        else if (c >= '0' && c <= '9') c = c - '0' + 48;
        else if (c == ' ') c = 32;
        else if (c == ':') c = 58;
        else if (c == '%') c = 37;
        else if (c == '!') c = 33;
        POKE(SCREEN_BASE + off, c);
        POKE(COLRAM_BASE + off, col);
        ++off; ++s;
    }
}

static void scr_num(unsigned char x, unsigned char y, unsigned int val, unsigned char col) {
    char buf[6];
    unsigned char i = 5;
    buf[5] = 0;
    if (val == 0) { buf[4] = '0'; i = 4; }
    else while (val && i > 0) { buf[--i] = '0' + (val % 10); val /= 10; }
    scrwrite(x, y, buf + i, col);
}

/* Write number with zero-padding to specified width */
static void scr_num_pad(unsigned char x, unsigned char y, unsigned int val, unsigned char width, unsigned char col) {
    char buf[6];
    unsigned char i;
    for (i = 0; i < 5; ++i) buf[i] = '0';
    buf[5] = 0;
    i = 5;
    if (val == 0) { buf[4] = '0'; }
    else while (val && i > 0) { buf[--i] = '0' + (val % 10); val /= 10; }
    /* Write last 'width' chars */
    scrwrite(x, y, buf + 5 - width, col);
}

static void clear_screen(void) {
    unsigned int i;
    for (i = 0; i < 1000; ++i) { POKE(SCREEN_BASE + i, 32); POKE(COLRAM_BASE + i, COL_BG); }
    memset(pfmap, PF_SOLID, 1000);
}

static void init_tables(void) {
    unsigned char i;
    unsigned int off = 0;
    for (i = 0; i < 25; ++i) { scr_row[i] = off; pfr[i] = pfmap + off; off += 40; }
}

/* ---- VIC setup ---- */

static void detect_system(void) {
    unsigned int raster;
    while (PEEK(VIC_BASE + 0x12) != 0) {}
    while (PEEK(VIC_BASE + 0x12) < 2) {}
    while (PEEK(VIC_BASE + 0x12) > 1) {
        raster = PEEK(VIC_BASE + 0x12) | ((PEEK(VIC_BASE + 0x11) & 0x80) << 1);
        if (raster > 290) { frames_per_sec = 50; return; }
    }
    frames_per_sec = 60;
}

static void setup_vic(void) {
    unsigned char *dst;
    detect_system();
    init_tables();
    asm volatile("sei");
    POKE(0x0001, PEEK(0x0001) & ~0x04);
    memcpy((void*)CHARSET_DATA, (void*)0xD000, 2048);
    POKE(0x0001, PEEK(0x0001) | 0x04);
    asm volatile("cli");
    /* Char 128: tile block */
    dst = (unsigned char *)(CHARSET_DATA + 128 * 8);
    dst[0]=0xFE; dst[1]=0xFE; dst[2]=0xFE; dst[3]=0xFE;
    dst[4]=0xFE; dst[5]=0xFE; dst[6]=0xFE; dst[7]=0x00;
    /* Char 129: border horizontal top */
    dst = (unsigned char *)(CHARSET_DATA + 129 * 8);
    dst[0]=0xFF; dst[1]=0xFF; dst[2]=0x00; dst[3]=0x00;
    dst[4]=0x00; dst[5]=0x00; dst[6]=0x00; dst[7]=0x00;
    /* Char 130: border horizontal bottom */
    dst = (unsigned char *)(CHARSET_DATA + 130 * 8);
    dst[0]=0x00; dst[1]=0x00; dst[2]=0x00; dst[3]=0x00;
    dst[4]=0x00; dst[5]=0x00; dst[6]=0xFF; dst[7]=0xFF;
    /* Char 131: border vertical left */
    dst = (unsigned char *)(CHARSET_DATA + 131 * 8);
    dst[0]=0xC0; dst[1]=0xC0; dst[2]=0xC0; dst[3]=0xC0;
    dst[4]=0xC0; dst[5]=0xC0; dst[6]=0xC0; dst[7]=0xC0;
    /* Char 132: border vertical right */
    dst = (unsigned char *)(CHARSET_DATA + 132 * 8);
    dst[0]=0x03; dst[1]=0x03; dst[2]=0x03; dst[3]=0x03;
    dst[4]=0x03; dst[5]=0x03; dst[6]=0x03; dst[7]=0x03;
    /* Char 133: top-left corner */
    dst = (unsigned char *)(CHARSET_DATA + 133 * 8);
    dst[0]=0xFF; dst[1]=0xFF; dst[2]=0xC0; dst[3]=0xC0;
    dst[4]=0xC0; dst[5]=0xC0; dst[6]=0xC0; dst[7]=0xC0;
    /* Char 134: top-right corner */
    dst = (unsigned char *)(CHARSET_DATA + 134 * 8);
    dst[0]=0xFF; dst[1]=0xFF; dst[2]=0x03; dst[3]=0x03;
    dst[4]=0x03; dst[5]=0x03; dst[6]=0x03; dst[7]=0x03;
    /* Char 135: bottom-left corner */
    dst = (unsigned char *)(CHARSET_DATA + 135 * 8);
    dst[0]=0xC0; dst[1]=0xC0; dst[2]=0xC0; dst[3]=0xC0;
    dst[4]=0xC0; dst[5]=0xC0; dst[6]=0xFF; dst[7]=0xFF;
    /* Char 136: bottom-right corner */
    dst = (unsigned char *)(CHARSET_DATA + 136 * 8);
    dst[0]=0x03; dst[1]=0x03; dst[2]=0x03; dst[3]=0x03;
    dst[4]=0x03; dst[5]=0x03; dst[6]=0xFF; dst[7]=0xFF;
    POKE(0xDD00, (PEEK(0xDD00) & 0xFC) | 0x02);
    POKE(VIC_D018, 0x30);
    POKE(VIC_BG, COL_BG);
    POKE(VIC_BORDER, COLOR_BLACK);
}

static void restore_vic(void) {
    POKE(0xDD00, (PEEK(0xDD00) & 0xFC) | 0x03);
    POKE(VIC_D018, 0x15);
}

/* ---- Sprites ---- */

static void gen_ball(unsigned int addr, unsigned char wl, unsigned char wr) {
    /* 6 MC wide x 12 rows, profile: 2->4->6->6->6->6->6->6->4->2 */
    unsigned char *p = (unsigned char *)addr;
    unsigned char row, col, sc, ec, mc, bi, bp;
    memset(p, 0, 63);
    for (row = 4; row <= 15; ++row) {
        if (row == 4 || row == 15)       { sc = 5; ec = 6; }
        else if (row == 5 || row == 14)  { sc = 4; ec = 7; }
        else                             { sc = 3; ec = 8; }
        for (col = sc; col <= ec; ++col) {
            mc = 2;
            if (col >= wl && col <= wr) mc = 3;
            if (row == 7 && col == 6) mc = 3; /* shine */
            bi = col >> 2;
            bp = (3 - (col & 3)) << 1;
            p[row * 3 + bi] |= (mc << bp);
        }
    }
}

static void make_cursor(unsigned int addr, unsigned char vert) {
    unsigned char *p = (unsigned char *)addr;
    memset(p, 0, 63);
    if (!vert) {
        p[21]=0x02; p[22]=0x00; p[23]=0x40;
        p[24]=0x06; p[25]=0x00; p[26]=0x60;
        p[27]=0x0F; p[28]=0xFF; p[29]=0xF0;
        p[30]=0x1F; p[31]=0xFF; p[32]=0xF8;
        p[33]=0x0F; p[34]=0xFF; p[35]=0xF0;
        p[36]=0x06; p[37]=0x00; p[38]=0x60;
        p[39]=0x02; p[40]=0x00; p[41]=0x40;
    } else {
        p[12]=0x00; p[13]=0x18; p[14]=0x00;
        p[15]=0x00; p[16]=0x3C; p[17]=0x00;
        p[18]=0x00; p[19]=0x7E; p[20]=0x00;
        p[21]=0x00; p[22]=0xFF; p[23]=0x00;
        p[24]=0x00; p[25]=0x18; p[26]=0x00;
        p[27]=0x00; p[28]=0x18; p[29]=0x00;
        p[30]=0x00; p[31]=0x18; p[32]=0x00;
        p[33]=0x00; p[34]=0x18; p[35]=0x00;
        p[36]=0x00; p[37]=0x18; p[38]=0x00;
        p[39]=0x00; p[40]=0xFF; p[41]=0x00;
        p[42]=0x00; p[43]=0x7E; p[44]=0x00;
        p[45]=0x00; p[46]=0x3C; p[47]=0x00;
        p[48]=0x00; p[49]=0x18; p[50]=0x00;
    }
}

static void init_sprites(void) {
    unsigned char i;
    make_cursor(SPRITE_DATA, 0);
    make_cursor(SPRITE_DATA + 64, 1);
    gen_ball(SPRITE_DATA + 128, 99, 0);
    gen_ball(SPRITE_DATA + 192, 3, 5);
    gen_ball(SPRITE_DATA + 256, 3, 7);
    gen_ball(SPRITE_DATA + 320, 3, 8);
    gen_ball(SPRITE_DATA + 384, 6, 8);
    gen_ball(SPRITE_DATA + 448, 8, 8);
    POKE(SPRITE_PTR, SPR_CUR_H);
    for (i = 1; i < 8; ++i) POKE(SPRITE_PTR + i, SPR_BALL_0);
    POKE(VIC_SPR_COL, COLOR_WHITE);
    for (i = 1; i < 8; ++i) POKE(VIC_SPR_COL + i, COLOR_RED);
    POKE(VIC_SPR_MC, 0xFE);
    POKE(VIC_SPR_MCR0, COLOR_BROWN);
    POKE(VIC_SPR_MCR1, COLOR_WHITE);
    POKE(VIC_SPR_XE, 0); POKE(VIC_SPR_YE, 0);
    POKE(VIC_SPR_PRI, 0); POKE(VIC_SPR_EN, 0x01);
}

/* Store sprite position into arrays (no VIC writes yet) */
static void set_spr(unsigned char spr, unsigned int x, unsigned char y) {
    spr_x_lo[spr] = (unsigned char)x;
    spr_x_hi[spr] = (unsigned char)(x >> 8) & 1;
    spr_y[spr] = y;
}

/* Flush all sprite positions to VIC-II registers */
static void flush_sprites(void) {
    unsigned char i, xhi = 0;
    for (i = 0; i < 8; ++i) {
        POKE(0xD000 + (i << 1), spr_x_lo[i]);
        POKE(0xD001 + (i << 1), spr_y[i]);
        if (spr_x_hi[i]) xhi |= (1 << i);
    }
    POKE(0xD010, xhi);
}

/* ---- Field ---- */

#define CH_BORDER_H_TOP  129
#define CH_BORDER_H_BOT  130
#define CH_BORDER_V_L    131
#define CH_BORDER_V_R    132
#define CH_BORDER_TL     133
#define CH_BORDER_TR     134
#define CH_BORDER_BL     135
#define CH_BORDER_BR     136
#define COL_BORDER       COLOR_GRAY2

static void draw_border(void) {
    unsigned char i;
    /* Top edge */
    for (i = PF_X; i < PF_X + PF_W; ++i)
        scr_put(i, PF_Y - 1, CH_BORDER_H_TOP, COL_BORDER);
    /* Bottom edge */
    for (i = PF_X; i < PF_X + PF_W; ++i)
        scr_put(i, PF_Y + PF_H, CH_BORDER_H_BOT, COL_BORDER);
    /* Left edge */
    for (i = PF_Y; i < PF_Y + PF_H; ++i)
        scr_put(PF_X - 1, i, CH_BORDER_V_L, COL_BORDER);
    /* Right edge */
    for (i = PF_Y; i < PF_Y + PF_H; ++i)
        scr_put(PF_X + PF_W, i, CH_BORDER_V_R, COL_BORDER);
    /* Corners */
    scr_put(PF_X - 1, PF_Y - 1, CH_BORDER_TL, COL_BORDER);
    scr_put(PF_X + PF_W, PF_Y - 1, CH_BORDER_TR, COL_BORDER);
    scr_put(PF_X - 1, PF_Y + PF_H, CH_BORDER_BL, COL_BORDER);
    scr_put(PF_X + PF_W, PF_Y + PF_H, CH_BORDER_BR, COL_BORDER);
}

static void clear_field(void) {
    unsigned char x, y;
    for (y = PF_Y; y < PF_Y + PF_H; ++y)
        for (x = PF_X; x < PF_X + PF_W; ++x) {
            scr_put(x, y, CH_TILE, COL_TILE);
            pfr[y][x] = PF_OPEN;
        }
}

/* HUD layout:
 * Row 0:  "Lives:XX"  left,  "Level:XX" centered,  "Time:XXX" right
 * Row 24: "Score:XXXXX" left,  "Cleared:XX%" right */

static void draw_hud_labels(void) {
    /* Left edge = col 1 (border), right edge = col 38 (border) */
    scrwrite(1, 0, "Lives", COLOR_WHITE);
    scrwrite(16, 0, "Level", COLOR_WHITE);
    scrwrite(31, 0, "Time", COLOR_WHITE);
    scrwrite(1, 24, "Score", COLOR_WHITE);
    scrwrite(28, 24, "Cleared", COLOR_WHITE);
}

static void draw_hud(void) {
    unsigned int t4 = total_cells >> 2;
    unsigned char pct = 0;
    if (t4 > 0) pct = (unsigned char)((filled_cells * (unsigned int)25) / t4);
    if (pct > 100) pct = 100;
    scr_num_pad(7, 0, lives, 2, COLOR_WHITE);
    scr_num_pad(22, 0, level, 2, COLOR_WHITE);
    scr_num_pad(36, 0, time_left, 3, (time_left <= 10) ? COLOR_RED : COLOR_WHITE);
    scr_num_pad(7, 24, score, 5, COLOR_WHITE);
    scr_num_pad(36, 24, pct, 2, COLOR_WHITE);
    scrwrite(38, 24, "%", COLOR_WHITE);
}

/* ---- Ball logic ---- */

static void init_balls(unsigned char count) {
    unsigned char i, spr_en;
    num_balls = count;
    if (count > MAX_BALLS) count = MAX_BALLS;
    for (i = 0; i < count; ++i) {
        bact[i] = 1;
        bx[i] = PF_X * 8 + 20 + (rand() % (PF_W * 8 - 40));
        by[i] = (unsigned char)(PF_Y * 8 + 20 + (rand() % (PF_H * 8 - 40)));
        bdx[i] = (rand() & 1) ? 1 : -1;
        bdy[i] = (rand() & 1) ? 1 : -1;
        banim[i] = rand() % BALL_FRAMES;
        POKE(VIC_SPR_COL + 1 + i, COLOR_RED);
    }
    spr_en = 0x01;
    for (i = 0; i < count; ++i) spr_en |= spr_bit[i + 1];
    POKE(VIC_SPR_EN, spr_en);
}

static void erase_wall(unsigned char idx);

/* Global temps for current ball — avoids parameter passing and repeated array indexing.
 * cc65 generates much better code for globals than locals or parameters. */
unsigned int  cur_px, cur_npx;
unsigned char cur_py, cur_npy;
unsigned char cur_i;

/* Are walls currently being built? Cached to skip wall checks when idle. */
unsigned char walls_building;

/* Wall hit index: 0 or 1 = hit wall[idx], 0xFF = no hit */
/* Move one ball 1 pixel with collision detection.
 * LLVM-MOS generates excellent code from plain C — no hand asm needed. */
static void move_one(void) {
    unsigned char cl, cr, ct, cb, v;
    unsigned char *rowt, *rowb;

    cur_npx = cur_px + bdx[cur_i];
    cur_npy = cur_py + bdy[cur_i];

    cl = (unsigned char)((cur_npx + BHB_L) >> 3);
    cr = (unsigned char)((cur_npx + BHB_R) >> 3);

    /* Wall-in-progress check (only when walls active) */
    if (walls_building) {
        ct = (cur_npy + BHB_T) >> 3;
        cb = (cur_npy + BHB_B) >> 3;
        v = pfr[ct][cl];
        if (v < PF_WALL_A) v = pfr[ct][cr];
        if (v < PF_WALL_A) v = pfr[cb][cl];
        if (v < PF_WALL_A) v = pfr[cb][cr];
        if (v >= PF_WALL_A) {
            v = (v == PF_WALL_A) ? 0 : 1;
            if (wact[v]) {
                erase_wall(v);
                wact[v] = 0;
                walls_building = wact[0] | wact[1];
                if (lives > 0) --lives;
            }
            cur_px = cur_npx;
            cur_py = cur_npy;
            return;
        }
    }

    /* Solid bounce X — corners at (npx, cur_py) */
    ct = (cur_py + BHB_T) >> 3;
    cb = (cur_py + BHB_B) >> 3;
    rowt = pfr[ct]; rowb = pfr[cb];
    if (rowt[cl] || rowt[cr] || rowb[cl] || rowb[cr]
        || cur_npx + BHB_L <= PF_PX_L || cur_npx + BHB_R >= PF_PX_R) {
        bdx[cur_i] = -bdx[cur_i];
    } else {
        cur_px = cur_npx;
    }

    /* Solid bounce Y — corners at (cur_px, npy) */
    cl = (unsigned char)((cur_px + BHB_L) >> 3);
    cr = (unsigned char)((cur_px + BHB_R) >> 3);
    ct = (cur_npy + BHB_T) >> 3;
    cb = (cur_npy + BHB_B) >> 3;
    rowt = pfr[ct]; rowb = pfr[cb];
    if (rowt[cl] || rowt[cr] || rowb[cl] || rowb[cr]
        || cur_npy + BHB_T <= PF_PY_T || cur_npy + BHB_B >= PF_PY_B) {
        bdy[cur_i] = -bdy[cur_i];
    } else {
        cur_py = cur_npy;
    }
}

static void update_balls(void) {
    unsigned char do_anim = ((frame_count & 7) == 0);
    for (cur_i = 0; cur_i < num_balls; ++cur_i) {
        if (!bact[cur_i]) continue;

        cur_px = bx[cur_i];
        cur_py = by[cur_i];

        move_one();

        bx[cur_i] = cur_px;
        by[cur_i] = cur_py;

        if (do_anim) {
            unsigned char a = banim[cur_i] + 1;
            if (a >= BALL_FRAMES) a = 0;
            banim[cur_i] = a;
            POKE(SPRITE_PTR + 1 + cur_i, SPR_BALL_0 + a);
        }

        set_spr(cur_i + 1, cur_px + SPR_OFS_X, cur_py + SPR_OFS_Y);
    }
}

/* ---- Wall building ---- */

static void erase_wall(unsigned char idx) {
    signed char ddx = -wdcx[idx], ddy = -wdcy[idx];
    unsigned char wx = wcx[idx], wy = wcy[idx];
    unsigned char tv = wpfv[idx];
    while (wx >= PF_X && wx < PF_X + PF_W && wy >= PF_Y && wy < PF_Y + PF_H) {
        if (pfr[wy][wx] == tv) {
            scr_put(wx, wy, CH_TILE, COL_TILE);
            pfr[wy][wx] = PF_OPEN;
        } else break;
        wx += ddx; wy += ddy;
    }
}

static void solidify_wall(unsigned char idx) {
    signed char ddx = -wdcx[idx], ddy = -wdcy[idx];
    unsigned char wx = wcx[idx], wy = wcy[idx];
    unsigned char tv = wpfv[idx];
    while (wx >= PF_X && wx < PF_X + PF_W && wy >= PF_Y && wy < PF_Y + PF_H) {
        if (pfr[wy][wx] == tv) {
            POKE(COLRAM_BASE + scr_row[wy] + wx, COL_WALL_DONE);
            pfr[wy][wx] = PF_SOLID;
        } else break;
        wx += ddx; wy += ddy;
    }
}

/* ---- Fill ---- */

static void fill_regions(void) {
    unsigned char i, gx, gy, x, y;

    for (i = 0; i < num_balls; ++i) {
        if (!bact[i]) continue;
        gx = SHR3(bx[i] + (BHB_L + BHB_R) / 2);
        gy = (by[i] + (BHB_T + BHB_B) / 2) >> 3;
        if (pfr[gy][gx] != PF_OPEN) {
            if (pfr[gy][gx-1] == PF_OPEN) --gx;
            else if (pfr[gy][gx+1] == PF_OPEN) ++gx;
            else if (pfr[gy-1][gx] == PF_OPEN) --gy;
            else if (pfr[gy+1][gx] == PF_OPEN) ++gy;
            else continue;
        }
        fsp = 0;
        pfr[gy][gx] = PF_MARK;
        fsx[0] = gx; fsy[0] = gy; fsp = 1;

        /* Flood fill inner loop */
        while (fsp > 0) {
            unsigned char fx, fy;
            unsigned char *row, *rowu, *rowd;
            --fsp;
            fx = fsx[fsp]; fy = fsy[fsp];
            row = pfr[fy];
            rowu = pfr[fy - 1];
            rowd = pfr[fy + 1];
            if (fsp < FILL_STACK_SZ && row[fx + 1] == PF_OPEN) { row[fx + 1] = PF_MARK; fsx[fsp] = fx + 1; fsy[fsp] = fy; ++fsp; }
            if (fsp < FILL_STACK_SZ && row[fx - 1] == PF_OPEN) { row[fx - 1] = PF_MARK; fsx[fsp] = fx - 1; fsy[fsp] = fy; ++fsp; }
            if (fsp < FILL_STACK_SZ && rowd[fx] == PF_OPEN)    { rowd[fx] = PF_MARK;    fsx[fsp] = fx; fsy[fsp] = fy + 1; ++fsp; }
            if (fsp < FILL_STACK_SZ && rowu[fx] == PF_OPEN)    { rowu[fx] = PF_MARK;    fsx[fsp] = fx; fsy[fsp] = fy - 1; ++fsp; }
        }
    }

    {
        unsigned int prev_filled = filled_cells;
        filled_cells = 0;
        for (y = PF_Y; y < PF_Y + PF_H; ++y) {
            for (x = PF_X; x < PF_X + PF_W; ++x) {
                if (pfr[y][x] == PF_OPEN) {
                    pfr[y][x] = PF_SOLID;
                } else if (pfr[y][x] == PF_MARK) {
                    pfr[y][x] = PF_OPEN;
                }
                if (pfr[y][x] == PF_SOLID) ++filled_cells;
            }
        }
        /* Award points for newly sealed cells */
        if (filled_cells > prev_filled) {
            score += (filled_cells - prev_filled) * level;
        }
    }

    /* Push any ball stuck inside solid cells to nearest open space */
    for (i = 0; i < num_balls; ++i) {
        unsigned char cl2, cr2, ct2, cb2, stuck;
        if (!bact[i]) continue;
        cl2 = SHR3(bx[i] + BHB_L);
        cr2 = SHR3(bx[i] + BHB_R);
        ct2 = (by[i] + BHB_T) >> 3;
        cb2 = (by[i] + BHB_B) >> 3;
        stuck = pfr[ct2][cl2] == PF_SOLID || pfr[ct2][cr2] == PF_SOLID ||
                pfr[cb2][cl2] == PF_SOLID || pfr[cb2][cr2] == PF_SOLID;
        if (stuck) {
            unsigned char tries = 40;
            while (tries-- > 0) {
                bx[i] += bdx[i];
                by[i] += bdy[i];
                cl2 = SHR3(bx[i] + BHB_L);
                cr2 = SHR3(bx[i] + BHB_R);
                ct2 = (by[i] + BHB_T) >> 3;
                cb2 = (by[i] + BHB_B) >> 3;
                if (pfr[ct2][cl2] != PF_SOLID && pfr[ct2][cr2] != PF_SOLID &&
                    pfr[cb2][cl2] != PF_SOLID && pfr[cb2][cr2] != PF_SOLID) break;
            }
            /* Boundary clamp */
            if (bx[i] + BHB_L <= PF_PX_L) { bx[i] = PF_PX_L - BHB_L + 1; bdx[i] = 1; }
            if (bx[i] + BHB_R >= PF_PX_R) { bx[i] = PF_PX_R - BHB_R - 1; bdx[i] = -1; }
            if (by[i] + BHB_T <= PF_PY_T) { by[i] = PF_PY_T - BHB_T + 1; bdy[i] = 1; }
            if (by[i] + BHB_B >= PF_PY_B) { by[i] = PF_PY_B - BHB_B - 1; bdy[i] = -1; }
        }
    }

    fv_active = 1;
    fv_row = PF_Y;
}

/* Process one row of visual fill */
static void fill_visual_row(void) {
    unsigned char x;
    unsigned char *row = pfr[fv_row];
    unsigned int roff = scr_row[fv_row];
    for (x = PF_X; x < PF_X + PF_W; ++x) {
        if (row[x] == PF_SOLID && PEEK(SCREEN_BASE + roff + x) == CH_TILE) {
            POKE(SCREEN_BASE + roff + x, CH_FILLED);
            POKE(COLRAM_BASE + roff + x, COL_BG);
        }
    }
}

static void fill_visual_step(void) {
    unsigned char end_y;
    if (!fv_active) return;
    end_y = fv_row + 3;
    if (end_y > PF_Y + PF_H) end_y = PF_Y + PF_H;
    while (fv_row < end_y) {
        fill_visual_row();
        ++fv_row;
    }
    if (fv_row >= PF_Y + PF_H) fv_active = 0;
}

static unsigned char wall_tick;

static void advance_walls(void) {
    unsigned char i, nx, ny;
    /* 50% slower: skip every other frame */
    ++wall_tick;
    if (wall_tick & 1) return;
    for (i = 0; i < MAX_WALLS; ++i) {
        if (!wact[i]) continue;
        nx = wcx[i] + wdcx[i];
        ny = wcy[i] + wdcy[i];
        if (nx < PF_X || nx >= PF_X + PF_W || ny < PF_Y || ny >= PF_Y + PF_H || pfr[ny][nx] != PF_OPEN) {
            solidify_wall(i);
            wact[i] = 0;
            walls_building = wact[0] | wact[1];
            if (!walls_building) {
                fill_regions();
            }
            continue;
        }
        scr_put(nx, ny, CH_WALL, wcol[i]);
        pfr[ny][nx] = wpfv[i];
        wcx[i] = nx; wcy[i] = ny;
    }
}

static void start_wall(void) {
    unsigned char cx = (unsigned char)((cursor_px + 12) >> 3);
    unsigned char cy = (unsigned char)((cursor_py + 10) >> 3);
    if (pfr[cy][cx] != PF_OPEN) return;
    if (wact[0] || wact[1]) return;
    wact[0] = 1; wcx[0] = cx; wcy[0] = cy;
    wdcx[0] = wall_dir ? 0 : 1; wdcy[0] = wall_dir ? 1 : 0;
    wcol[0] = COL_WALL_A; wpfv[0] = PF_WALL_A;
    wact[1] = 1; wcx[1] = cx; wcy[1] = cy;
    wdcx[1] = wall_dir ? 0 : -1; wdcy[1] = wall_dir ? -1 : 0;
    wcol[1] = COL_WALL_B; wpfv[1] = PF_WALL_B;
    scr_put(cx, cy, CH_WALL, COL_WALL_A);
    pfr[cy][cx] = PF_WALL_A;
    walls_building = 1;
}

/* ---- Input ---- */

#define JOY_MOVE_DELAY 3  /* frames before cursor starts moving */

static void read_input(void) {
    unsigned char joy = ~PEEK(JOY2_PORT) & 0x1F;
    unsigned char fire_edge = (joy & JOY2_FIRE) && !(joy_prev & JOY2_FIRE);
    unsigned char dirs = joy & 0x0F;

    /* Direction change is instant, movement requires holding */
    if (dirs) {
        /* Set wall direction immediately on any directional input */
        if (dirs & (JOY2_LEFT | JOY2_RIGHT)) wall_dir = 0;
        if (dirs & (JOY2_UP | JOY2_DOWN))    wall_dir = 1;

        /* Track hold duration */
        if (dirs == (joy_prev & 0x0F)) {
            if (joy_hold_frames < 255) ++joy_hold_frames;
        } else {
            joy_hold_frames = 0;
        }

        /* Only move cursor after holding for a few frames */
        if (joy_hold_frames >= JOY_MOVE_DELAY) {
            unsigned int npx = cursor_px;
            unsigned char npy = (unsigned char)cursor_py;
            if (dirs & JOY2_UP)    { if (npy >= PF_PY_T + CURSOR_SPEED) npy -= CURSOR_SPEED; else npy = PF_PY_T; }
            if (dirs & JOY2_DOWN)  { if (npy + CURSOR_SPEED <= PF_PY_B) npy += CURSOR_SPEED; else npy = PF_PY_B; }
            if (dirs & JOY2_LEFT)  { if (npx >= PF_PX_L + CURSOR_SPEED) npx -= CURSOR_SPEED; else npx = PF_PX_L; }
            if (dirs & JOY2_RIGHT) { if (npx + CURSOR_SPEED <= PF_PX_R) npx += CURSOR_SPEED; else npx = PF_PX_R; }
            cursor_px = npx; cursor_py = npy;
        }
    } else {
        joy_hold_frames = 0;
    }

    if (fire_edge) start_wall();
    joy_prev = joy;
}

static void update_cursor(void) {
    if (wall_dir != last_cursor_dir) {
        POKE(SPRITE_PTR, wall_dir ? SPR_CUR_V : SPR_CUR_H);
        last_cursor_dir = wall_dir;
    }
    set_spr(0, cursor_px + SPR_OFS_X, cursor_py + SPR_OFS_Y);
}

/* ---- Game flow ---- */

static void init_level(void) {
    unsigned char i, ball_count;
    clear_screen();
    POKE(VIC_BG, COL_BG); POKE(VIC_BORDER, COLOR_BLACK);
    draw_border();
    clear_field();
    total_cells = (unsigned int)PF_W * PF_H;
    filled_cells = 0;
    fill_target = (total_cells >> 2) * 3;
    fv_active = 0;
    cursor_px = PF_PX_L + (PF_W * 4);
    cursor_py = PF_PY_T + (PF_H * 4);
    wall_dir = 0; last_cursor_dir = 255;
    wact[0] = 0; wact[1] = 0; walls_building = 0;
    ball_count = level + 1;
    if (ball_count > MAX_BALLS) ball_count = MAX_BALLS;
    lives = ball_count;
    time_left = (unsigned int)ball_count * 30;
    time_frames = 0;
    draw_hud_labels();
    draw_hud();
    /* Hide sprites during level intro */
    POKE(VIC_SPR_EN, 0x00);
    /* Centered "LEVEL N" with 1 black block padding each side */
    {
        unsigned char nw, tw, sx, ax;
        nw = (level >= 100) ? 3 : (level >= 10) ? 2 : 1;
        tw = 1 + 6 + nw + 1;
        sx = 20 - (tw >> 1);
        scr_put(sx, 12, ' ', COLOR_BLACK);
        scrwrite(sx + 1, 12, "LEVEL ", COLOR_WHITE);
        scr_num(sx + 7, 12, level, COLOR_WHITE);
        scr_put(sx + 7 + nw, 12, ' ', COLOR_BLACK);
        for (i = 0; i < 60; ++i) waitvsync();
        for (ax = sx; ax < sx + tw; ++ax) { scr_put(ax, 12, CH_TILE, COL_TILE); pfr[12][ax] = PF_OPEN; }
    }
    /* Now init and show balls */
    init_balls(ball_count);
}

static void title_screen(void) {
    clear_screen();
    POKE(VIC_BG, COLOR_BLACK); POKE(VIC_BORDER, COLOR_BLACK);
    POKE(VIC_SPR_EN, 0x00);
    scrwrite(12, 3, "M O L T B A L L", COLOR_WHITE);
    scrwrite(8, 7, "A JEZZBALL CLONE FOR C64", COLOR_GRAY3);
    scrwrite(6, 10, "JOYSTICK PORT 2", COLOR_WHITE);
    scrwrite(6, 12, "LEFT/RIGHT", COLOR_RED);
    scrwrite(16, 12, " = HORIZONTAL", COLOR_GRAY2);
    scrwrite(6, 13, "UP/DOWN", COLOR_BLUE);
    scrwrite(13, 13, "    = VERTICAL", COLOR_GRAY2);
    scrwrite(6, 14, "FIRE", COLOR_WHITE);
    scrwrite(10, 14, "       = BUILD WALL", COLOR_GRAY2);
    scrwrite(8, 17, "SEAL 75% TO CLEAR A LEVEL", COLOR_YELLOW);
    scrwrite(11, 21, "PRESS FIRE TO START", COLOR_LIGHTGREEN);
    for (;;) { unsigned char j; waitvsync(); j = ~PEEK(JOY2_PORT) & 0x1F; if (j & JOY2_FIRE) break; if (kbhit()) { cgetc(); break; } }
    while (~PEEK(JOY2_PORT) & JOY2_FIRE) waitvsync();
}

static void game_over_screen(void) {
    unsigned char i;
    POKE(VIC_SPR_EN, 0x00);
    for (i = 0; i < 10; ++i) { POKE(VIC_BORDER, COLOR_RED); waitvsync(); waitvsync(); waitvsync(); POKE(VIC_BORDER, COLOR_BLACK); waitvsync(); waitvsync(); waitvsync(); }
    if (time_left == 0) scrwrite(14, 10, "TIME UP! ", COLOR_RED);
    else scrwrite(14, 10, "GAME OVER", COLOR_RED);
    scrwrite(12, 12, "SCORE: ", COLOR_WHITE);
    scr_num(19, 12, score, COLOR_WHITE);
    scrwrite(11, 14, "LEVEL REACHED: ", COLOR_GRAY2);
    scr_num(26, 14, level, COLOR_GRAY2);
    scrwrite(10, 17, "PRESS FIRE TO RESTART", COLOR_LIGHTGREEN);
    for (;;) { unsigned char j; waitvsync(); j = ~PEEK(JOY2_PORT) & 0x1F; if (j & JOY2_FIRE) break; if (kbhit()) { cgetc(); break; } }
    while (~PEEK(JOY2_PORT) & JOY2_FIRE) waitvsync();
}

int main(void) {
    setup_vic();
    init_sprites();
restart:
    title_screen();
    level = 1; score = 0; lives = 2;
    game_over = 0; joy_prev = 0; frame_count = 0;
    init_level();
    while (!game_over) {
        waitvsync();
        ++frame_count;

        /* Timer tick — count vsyncs */
        ++time_frames;
        if (time_frames >= frames_per_sec) {
            time_frames = 0;
            if (time_left > 0) --time_left;
            if (time_left == 0) game_over = 1;
        }

        read_input();
        advance_walls();
        fill_visual_step();
        update_balls();
        update_cursor();
        /* Wait for raster to be in lower border, then flush all sprites at once — no tearing */
        while (PEEK(0xD012) < 250) {}
        flush_sprites();
        if ((frame_count & 63) == 0) draw_hud();

        /* Level complete? Wait for fill visual to finish first */
        if (!fv_active && filled_cells >= fill_target) {
            /* 1. Final HUD update */
            draw_hud();
            { unsigned char w; for (w = 0; w < 30; ++w) waitvsync(); }

            /* 2. Score update */
            score += (unsigned int)level * 100 + time_left; /* level complete bonus */
            draw_hud();
            { unsigned char w; for (w = 0; w < 20; ++w) waitvsync(); }

            /* 3. Hide balls */
            POKE(VIC_SPR_EN, 0x00);
            { unsigned char w; for (w = 0; w < 15; ++w) waitvsync(); }

            /* 4. Victory flash */
            { unsigned char w; for (w = 0; w < 15; ++w) { POKE(VIC_BORDER, COLOR_GREEN); waitvsync(); waitvsync(); POKE(VIC_BORDER, COLOR_BLACK); waitvsync(); waitvsync(); } }

            /* 5. Next level */
            ++level;
            init_level();
        }
        if (lives == 0) game_over = 1;
    }
    if (game_over == 1) { game_over_screen(); goto restart; }
    restore_vic();
    POKE(VIC_SPR_EN, 0x00);
    return 0;
}
