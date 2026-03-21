// clean.c - Clean, working C64 Tetris

#include <c64.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdlib.h>

#define WIDTH 10
#define HEIGHT 22
#define VISIBLE 20

// Game state
unsigned char board[220];
unsigned char piece;
unsigned char next;
signed char px;
signed char py;
unsigned char rot;
unsigned long score;
unsigned int lines;
unsigned char level;
unsigned char over;

// Colors for pieces
unsigned char colors[7] = {3,6,8,7,5,4,2};

// Shapes [piece][rot][y][x]
unsigned char shapes[7][4][4][4] = {
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
     {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
    {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
    {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
     {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
    {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
    {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
     {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}}
};

// Check if piece fits
unsigned char check(signed char x, signed char y, unsigned char r) {
    unsigned char i, j;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            if (shapes[piece][r][i][j]) {
                signed char bx = x + j;
                signed char by = y + i;
                if (bx < 0 || bx >= WIDTH || by >= HEIGHT) return 1;
                if (by >= 0 && board[by * WIDTH + bx]) return 1;
            }
        }
    }
    return 0;
}

// New piece
void spawn() {
    piece = next;
    next = rand() % 7;
    rot = 0;
    px = WIDTH / 2 - 2;
    py = 0;
    if (check(px, py, rot)) over = 1;
}

// Start new game
void init() {
    unsigned int i;
    for (i = 0; i < 220; ++i) board[i] = 0;
    score = 0;
    lines = 0;
    level = 1;
    over = 0;
    next = rand() % 7;
    spawn();
}

// Lock piece
void lock() {
    unsigned char i, j;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            if (shapes[piece][rot][i][j]) {
                signed char bx = px + j;
                signed char by = py + i;
                if (bx >= 0 && bx < WIDTH && by >= 0 && by < HEIGHT) {
                    board[by * WIDTH + bx] = piece + 1;
                }
            }
        }
    }
    spawn();
}

// Clear lines
void clear() {
    unsigned char x, y, yy;
    unsigned char cleared = 0;
    
    for (y = HEIGHT - 1; y >= 0; --y) {
        unsigned char full = 1;
        for (x = 0; x < WIDTH; ++x) {
            if (!board[y * WIDTH + x]) {
                full = 0;
                break;
            }
        }
        if (full) {
            cleared++;
            for (yy = y; yy > 0; --yy) {
                for (x = 0; x < WIDTH; ++x) {
                    board[yy * WIDTH + x] = board[(yy - 1) * WIDTH + x];
                }
            }
            for (x = 0; x < WIDTH; ++x) board[x] = 0;
            y++;
        }
    }
    
    if (cleared) {
        lines += cleared;
        switch (cleared) {
            case 1: score += 40 * level; break;
            case 2: score += 100 * level; break;
            case 3: score += 300 * level; break;
            case 4: score += 1200 * level; break;
        }
        level = (lines / 10) + 1;
    }
}

// Controls
void left() { if (!check(px - 1, py, rot)) px--; }
void right() { if (!check(px + 1, py, rot)) px++; }
void rotate_piece() {
    unsigned char new_rot = (rot + 1) & 3;
    if (!check(px, py, new_rot)) rot = new_rot;
}
void soft() {
    if (!check(px, py + 1, rot)) {
        py++;
        score++;
    } else {
        lock();
        clear();
    }
}
void hard() {
    while (!check(px, py + 1, rot)) {
        py++;
        score += 2;
    }
    lock();
    clear();
}

// Drawing
void draw_board() {
    unsigned char x, y;
    for (y = 0; y < VISIBLE; ++y) {
        for (x = 0; x < WIDTH; ++x) {
            unsigned char cell = board[(y + 2) * WIDTH + x];
            if (cell) {
                cputcxy(6 + x, 3 + y, 0x40);
                textcolor(colors[cell - 1]);
            } else {
                cputcxy(6 + x, 3 + y, ' ');
            }
        }
    }
    textcolor(1);
}

void draw_piece() {
    unsigned char i, j;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            if (shapes[piece][rot][i][j]) {
                signed char bx = px + j;
                signed char by = py + i - 2;
                if (bx >= 0 && bx < WIDTH && by >= 0 && by < VISIBLE) {
                    cputcxy(6 + bx, 3 + by, 0x40);
                    textcolor(colors[piece]);
                }
            }
        }
    }
    textcolor(1);
}

void draw_next() {
    unsigned char i, j;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            cputcxy(18 + j, 3 + i, shapes[next][0][i][j] ? 0x40 : ' ');
        }
    }
}

void draw_score() {
    char buf[10];
    unsigned long temp = score;
    unsigned char i = 0, j;
    char t;
    
    if (temp == 0) {
        buf[0] = '0';
        buf[1] = '\0';
    } else {
        while (temp) {
            buf[i++] = '0' + (temp % 10);
            temp /= 10;
        }
        buf[i] = '\0';
        for (j = 0; j < i / 2; ++j) {
            t = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = t;
        }
    }
    cputsxy(18, 8, buf);
}

// Input
unsigned char joy() {
    unsigned char j = PEEK(0xDC00);
    return ~j & 0x1F;
}

// Main
int main() {
    unsigned char j, last = 0;
    unsigned char frame;
    unsigned char drop = 0;
    
    // Setup
    clrscr();
    textcolor(1);
    bgcolor(0);
    bordercolor(0);
    
    // UI
    cputsxy(10, 0, "TETRIS");
    for (j = 0; j < VISIBLE + 2; ++j) {
        cputcxy(5, 3 + j, 0x40);
        cputcxy(16, 3 + j, 0x40);
    }
    for (j = 0; j < WIDTH + 2; ++j) {
        cputcxy(5 + j, 3 + VISIBLE, 0x40);
    }
    cputsxy(18, 2, "NEXT:");
    cputsxy(18, 7, "SCORE:");
    cputsxy(18, 9, "LINES:");
    cputsxy(18, 11, "LEVEL:");
    
    // Start
    init();
    
    // Loop
    while (1) {
        j = joy();
        
        if ((j & 2) && !(last & 2)) left();
        if ((j & 1) && !(last & 1)) right();
        if ((j & 8) && !(last & 8)) rotate_piece();
        if ((j & 16) && !(last & 16)) hard();
        if (j & 4) soft();
        
        last = j;
        
        drop++;
        if (drop >= 48 - (level * 5)) {
            soft();
            drop = 0;
        }
        
        draw_board();
        draw_piece();
        draw_next();
        draw_score();
        
        if (over) {
            cputsxy(8, 12, "GAME OVER");
            cputsxy(6, 14, "FIRE TO RESTART");
            while (!(joy() & 16)) {}
            init();
        }
        
        for (frame = 0; frame < 100; ++frame) asm("nop");
    }
}