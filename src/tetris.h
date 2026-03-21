// tetris.h - Tetris game definitions for C64 (cc65 compatible)

#ifndef TETRIS_H
#define TETRIS_H

// Board dimensions (10x20 visible, plus 2 hidden rows at top)
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 22  // 20 visible + 2 hidden
#define VISIBLE_HEIGHT 20

// Tetromino definitions
#define TETROMINO_COUNT 7
#define TETROMINO_SIZE 4

// Piece types
typedef enum {
    PIECE_I = 0,
    PIECE_J,
    PIECE_L,
    PIECE_O,
    PIECE_S,
    PIECE_T,
    PIECE_Z
} PieceType;

// Rotation states
typedef enum {
    ROT_0 = 0,
    ROT_90,
    ROT_180,
    ROT_270
} RotationState;

// Game state
typedef struct {
    unsigned char board[BOARD_WIDTH * BOARD_HEIGHT];
    int score;
    unsigned int lines;
    unsigned char level;
    unsigned char next_piece;
    unsigned char current_piece;
    unsigned char rotation;
    signed char piece_x;
    signed char piece_y;
    unsigned int drop_timer;
    unsigned char lock_delay;
    unsigned char game_over;
} GameState;

// Tetromino shapes [piece][rotation][y][x]
extern const unsigned char tetrominoes[7][4][4][4];

// Function prototypes
void game_init(GameState* game);
void game_spawn_piece(GameState* game);
unsigned char check_collision(const GameState* game, signed char x, signed char y, unsigned char rotation);
void lock_piece(GameState* game);
unsigned char clear_lines(GameState* game);
void game_move_left(GameState* game);
void game_move_right(GameState* game);
void game_rotate(GameState* game);
void game_drop(GameState* game);
void game_hard_drop(GameState* game);
void game_update(GameState* game);

#endif // TETRIS_H