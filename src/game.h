// game.h - Game definitions for Mr. Molty's Maze Chase

#ifndef GAME_H
#define GAME_H

#include <stdint.h>

// ==================== GAME CONSTANTS ====================

// Maze dimensions (simplified from original Pac-Man)
#define MAZE_WIDTH     28      // Original: 28
#define MAZE_HEIGHT    31      // Original: 31
#define TILE_SIZE      8       // 8x8 pixels per tile

// Screen position for maze (character coordinates)
#define MAZE_X         2       // Left margin
#define MAZE_Y         3       // Top margin

// Game states
typedef enum {
    STATE_TITLE = 0,
    STATE_GAME,
    STATE_READY,    // "READY!" screen
    STATE_LEVEL_COMPLETE,
    STATE_GAME_OVER,
    STATE_HIGH_SCORE
} GameState;

// Direction
typedef enum {
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,
    DIR_NONE
} Direction;

// Ghost modes (Coolers)
typedef enum {
    GHOST_SCATTER = 0,  // In corner
    GHOST_CHASE,        // Chase Mr. Molty
    GHOST_FRIGHTENED,   // Blue/white when inferno active
    GHOST_EYES          // Returning to base after eaten
} GhostMode;

// Ghost personality (Cooler types)
typedef enum {
    COOLER_BLAZE = 0,   // Red - aggressive chaser
    COOLER_SMOLDER,     // Pink - ambusher
    COOLER_CINDER,      // Cyan - unpredictable
    COOLER_ASH          // Orange - cautious
} CoolerType;

// Tile types
typedef enum {
    TILE_EMPTY = 0,     // Open space
    TILE_WALL,          // Maze wall
    TILE_DOT,           // Regular ember (10 points)
    TILE_POWER,         // Inferno pellet (50 points)
    TILE_TUNNEL         // Warp tunnel
} TileType;

// ==================== GAME STRUCTURES ====================

// Position in maze coordinates
typedef struct {
    int8_t x, y;
} MazePos;

// Position in pixel coordinates
typedef struct {
    int16_t x, y;
} PixelPos;

// Character (Mr. Molty or Cooler)
typedef struct {
    MazePos maze_pos;       // Current maze position
    PixelPos pixel_pos;     // Current pixel position
    Direction dir;          // Current direction
    Direction next_dir;     // Next direction (queued input)
    uint8_t speed;          // Movement speed (pixels per frame)
    uint8_t anim_frame;     // Animation frame
    uint8_t sprite_index;   // Which sprite to use
    uint8_t color;          // Sprite color
} Character;

// Cooler (ghost) specific data
typedef struct {
    Character base;
    CoolerType type;
    GhostMode mode;
    uint8_t mode_timer;     // Timer for mode changes
    MazePos target;         // Current target tile
    uint8_t frightened_timer; // Frightened mode timer
} Cooler;

// Game state
typedef struct {
    // Game flow
    GameState state;
    uint8_t level;
    uint8_t lives;
    uint16_t score;
    uint16_t high_score;
    
    // Maze
    uint8_t maze[MAZE_HEIGHT][MAZE_WIDTH];
    uint8_t dots_remaining;
    
    // Characters
    Character molty;
    Cooler coolers[4];
    
    // Game timing
    uint8_t frame_counter;
    uint8_t global_timer;
    
    // Power pellet state
    uint8_t inferno_active;
    uint8_t inferno_timer;
    
    // Ghost eaten chain (for scoring)
    uint8_t ghost_eaten_chain;
} GameStateData;

// ==================== GAME FUNCTIONS ====================

// Core game functions
void game_init(GameStateData* game);
void game_reset_level(GameStateData* game);
void game_update(GameStateData* game);

// Character movement
void move_molty(GameStateData* game);
void move_cooler(Cooler* cooler, GameStateData* game);
uint8_t can_move(const MazePos* pos, Direction dir, uint8_t maze[MAZE_HEIGHT][MAZE_WIDTH]);

// Input handling
void handle_input(GameStateData* game);

// Collision detection
void check_collisions(GameStateData* game);

// Scoring
void add_score(GameStateData* game, uint16_t points);
void eat_dot(GameStateData* game, uint8_t x, uint8_t y);
void eat_power(GameStateData* game, uint8_t x, uint8_t y);
void eat_cooler(GameStateData* game, uint8_t cooler_index);

// Ghost AI
void update_cooler_ai(Cooler* cooler, const GameStateData* game);
MazePos get_cooler_target(const Cooler* cooler, const GameStateData* game);

// ==================== COLOR DEFINITIONS ====================

// Mr. Molty colors (fire theme)
#define MOLTY_COLOR     COLOR_ORANGE
#define MOLTY_EYES      COLOR_WHITE

// Cooler (ghost) colors
#define COOLER_BLAZE_COLOR  COLOR_RED
#define COOLER_SMOLDER_COLOR COLOR_PURPLE
#define COOLER_CINDER_COLOR  COLOR_CYAN
#define COOLER_ASH_COLOR    COLOR_ORANGE
#define COOLER_FRIGHTENED_COLOR COLOR_WHITE
#define COOLER_EYES_COLOR   COLOR_WHITE

// Maze colors
#define WALL_COLOR      COLOR_BLUE
#define DOT_COLOR       COLOR_WHITE
#define POWER_COLOR     COLOR_RED

// ==================== DRAWING FUNCTIONS ====================
// (to be implemented in graphics.c)
void draw_maze(const GameStateData* game);
void draw_molty(const Character* molty);
void draw_cooler(const Cooler* cooler);
void draw_score(const GameStateData* game);
void draw_lives(const GameStateData* game);

#endif // GAME_H