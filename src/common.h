#ifndef COMMON_H
#define COMMON_H

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define TARGET_FPS 120
#define FRAME_TIME (1000000 / TARGET_FPS)

#define EMPTY 0
#define WALL 1
#define SAND 2
#define WATER 3

#define CLEAR "\e[2J"
#define CUR_TO_TOP "\e[H"
#define HIDE_CUR "\e[?25l"
#define SHOW_CUR "\e[?25h"
#define ALT_SCREEN "\e[?1049h"
#define MAIN_SCREEN "\e[?1049l"
#define ENABLE_MOUSE "\e[?1003h"
#define DISABLE_MOUSE "\e[?1003l"
#define ENABLE_MOUSE_SGR "\e[?1006h"
#define DISABLE_MOUSE_SGR "\e[?1006l"
#define ENABLE_FOCUS "\e[?1004h"
#define DISABLE_FOCUS "\e[?1004l"

#define BG_BLACK "\e[40m"
#define BG_RED "\e[41m"
#define BG_GREEN "\e[42m"
#define BG_YELLOW "\e[43m"
#define BG_BLUE "\e[44m"
#define BG_MAGENTA "\e[45m"
#define BG_CYAN "\e[46m"
#define BG_WHITE "\e[47m"
#define BLACK "\e[30m"
#define RED "\e[31m"
#define GREEN "\e[32m"
#define YELLOW "\e[33m"
#define BLUE "\e[34m"
#define MAGENTA "\e[35m"
#define CYAN "\e[36m"
#define WHITE "\e[37m"
#define RESET_STYLE "\e[0m"

typedef struct termios Termios;
typedef struct timeval Timeval;
typedef struct timespec Timespec;

extern bool running;
extern int current_cell;
extern int screen_width;
extern int screen_height;
extern int grid_size;
extern unsigned char *grid;
extern char *frame_buffer;
extern int frame_buffer_size;
extern int fps;
extern int cell_count;
extern char last_input;
extern int mouse_x;
extern int mouse_y;
extern int sim_mouse_x;
extern int sim_mouse_y;
extern int cur_radius;

typedef void (*ElementSimFn)(int x, int y);

typedef struct {
	const char *name;
	const char *color;
	const char *bg_color;
	const int density;
	ElementSimFn sim_fn;
} Element;

extern const Element element_registry[];

#endif
