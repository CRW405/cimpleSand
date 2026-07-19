#ifndef COMMON_H
#define COMMON_H

/**
 * @file common.h
 * @brief Shared constants, types, and global runtime state for the simulation.
 */

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define TARGET_FPS 60

#define EMPTY 0
#define WALL 1
#define SAND 2
#define WATER 3
#define STONE 4
#define OIL 5
#define FIRE 6
#define STEAM 7
#define LAVA 8

#define ELEMENT_COUNT (LAVA + 1)

#define CLEAR "\033[2J"
#define CUR_TO_TOP "\033[H"
#define HIDE_CUR "\033[?25l"
#define SHOW_CUR "\033[?25h"
#define ALT_SCREEN "\033[?1049h"
#define MAIN_SCREEN "\033[?1049l"
#define ENABLE_MOUSE "\033[?1003h"
#define DISABLE_MOUSE "\033[?1003l"
#define ENABLE_MOUSE_SGR "\033[?1006h"
#define DISABLE_MOUSE_SGR "\033[?1006l"
#define ENABLE_FOCUS "\033[?1004h"
#define DISABLE_FOCUS "\033[?1004l"

#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"
#define BG_GRAY "\033[48;5;240m"
#define BG_PURPLE "\033[48;5;129m"

#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define GRAY "\033[38;5;240m"
#define PURPLE "\033[38;5;129m"

#define RESET_STYLE "\033[0m"

#define ACTIVE_MASK 0b01111111
#define ACTIVE_BIT ((unsigned char)~ACTIVE_MASK)

/** @brief Alias for POSIX terminal settings type. */
typedef struct termios Termios;

/** @brief Alias for POSIX timeval type. */
typedef struct timeval Timeval;

/** @brief Alias for POSIX timespec type. */
typedef struct timespec Timespec;

/** @brief Main loop control flag; false exits the app. */
extern bool running;

/** @brief Currently selected element ID used for painting. */
extern int current_cell;

/** @brief Simulation grid width in cells. */
extern int screen_width;

/** @brief Simulation grid height in cells. */
extern int screen_height;

/** @brief Total number of cells in @ref grid. */
extern int grid_size;

/** @brief Simulation grid storing element IDs and active flags. */
extern unsigned char *grid;

/** @brief Render output buffer written to stdout each frame. */
extern char *frame_buffer;

/** @brief Capacity of @ref frame_buffer in bytes. */
extern int frame_buffer_size;

/** @brief Last measured frames-per-second value. */
extern int fps;

/** @brief User target frames-per-second cap. */
extern int target_fps;

/** @brief Last processed input key/event marker. */
extern char last_input;

/** @brief Last terminal mouse X coordinate (1-based terminal space). */
extern int mouse_x;

/** @brief Last terminal mouse Y coordinate (1-based terminal space). */
extern int mouse_y;

/** @brief Mapped simulation mouse X coordinate (0-based simulation space). */
extern int sim_mouse_x;

/** @brief Mapped simulation mouse Y coordinate (0-based simulation space). */
extern int sim_mouse_y;

/** @brief Current paint brush radius. */
extern int cur_radius;

/** @brief Number of non-empty cells currently present in the simulation. */
extern int cell_count;

/**
 * @typedef ElementSimFn
 * @brief Simulation callback for a single cell.
 * @param x Cell X coordinate.
 * @param y Cell Y coordinate.
 */
typedef void (*ElementSimFn)(int x, int y);

typedef struct {
	const char *name;     /**< Human-readable element name shown in the UI. */
	const char *color;    /**< ANSI foreground color sequence for rendering. */
	const char *bg_color; /**< ANSI background color sequence for rendering. */
	size_t color_len;     /**< Cached byte length of @ref color. */
	size_t bg_color_len;  /**< Cached byte length of @ref bg_color. */
	const char density;   /**< Relative density used by movement rules. */
	ElementSimFn sim_fn;  /**< Per-cell behavior callback, or NULL if static. */
} Element;

/** @brief Static registry of all element definitions indexed by element ID. */
extern const Element element_registry[];

#endif
