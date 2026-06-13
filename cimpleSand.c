#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

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
#define EABLE_FOCUS "\e[?1004h"
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

#define TARGET_FPS 120
#define FRAME_TIME (1000000 / TARGET_FPS)

#define EMPTY 0
#define WALL 1
#define SAND 2
#define WATER 3

int current_cell = WALL;

char *get_cell_color(int cell) {
	switch (cell) {
	case EMPTY:
		return BLACK;
	case WALL:
		return WHITE;
	case SAND:
		return YELLOW;
	case WATER:
		return BLUE;
	default:
		return RED;
	}
}

char *get_cell_bg_color(int cell) {
	switch (cell) {
	case EMPTY:
		return BG_BLACK;
	case WALL:
		return BG_WHITE;
	case SAND:
		return BG_YELLOW;
	case WATER:
		return BG_BLUE;
	default:
		return BG_RED;
	}
}

void term_op(int op_count, bool flush, ...) {
	va_list args;
	va_start(args, flush);

	for (int i = 0; i < op_count; i++) {
		char *code = va_arg(args, char *);
		printf("%s", code);
	}

	va_end(args);

	if (flush)
		fflush(stdout);
}

void init_screen() {
	term_op(6, true, CLEAR, HIDE_CUR, ALT_SCREEN, ENABLE_MOUSE_SGR,
	        ENABLE_MOUSE, EABLE_FOCUS);
}
void reset_term() {
	term_op(5, true, SHOW_CUR, MAIN_SCREEN, DISABLE_MOUSE_SGR, DISABLE_MOUSE,
	        DISABLE_FOCUS);
}

bool running = true;

void handle_sigint(int sig) {
	(void)sig;
	running = false;
}

char *random_color() {
	int random_int = rand() % 8;
	switch (random_int) {
	case 0:
		return BLACK;
	case 1:
		return RED;
	case 2:
		return GREEN;
	case 3:
		return YELLOW;
	case 4:
		return BLUE;
	case 5:
		return MAGENTA;
	case 6:
		return CYAN;
	case 7:
		return WHITE;
	}
	return WHITE;
}

char *random_bg_color() {
	int random_int = rand() % 8;
	switch (random_int) {
	case 0:
		return BG_BLACK;
	case 1:
		return BG_RED;
	case 2:
		return BG_GREEN;
	case 3:
		return BG_YELLOW;
	case 4:
		return BG_BLUE;
	case 5:
		return BG_MAGENTA;
	case 6:
		return BG_CYAN;
	case 7:
		return BG_WHITE;
	}
	return BG_WHITE;
}

int screen_width;
int screen_height;
int grid_size;
unsigned char *grid;
char *frame_buffer;
int frame_buffer_size;
static char gui_buffer[256];

void init_grid(int width, int height) {
	screen_width = width;
	screen_height = (height % 2 == 0) ? height : height + 1;
	grid_size = width * height;
	grid = calloc(grid_size, sizeof(unsigned char));
	frame_buffer_size = (grid_size * 25) + sizeof(gui_buffer);
	frame_buffer = malloc(frame_buffer_size);
}

int fps;
int cell_count;
char last_input = ' ';
int mouse_x = 0;
int mouse_y = 0;
int sim_mouse_x = 0;
int sim_mouse_y = 0;

char *gui() {
	snprintf(gui_buffer, sizeof(gui_buffer),
	         "FPS: %d | Cells: %d | Last Input: %c | Mouse: %d, %d (%d)", fps,
	         cell_count, last_input, mouse_x, mouse_y, sim_mouse_y);
	return gui_buffer;
}

int frame_buffer_offset;

void render() {
	term_op(1, false, CUR_TO_TOP);
	frame_buffer_offset = 0;

	for (int y = 0; y < screen_height; y += 2) {
		for (int x = 0; x < screen_width; x++) {
			// frame_buffer_offset +=
			//     snprintf(frame_buffer + frame_buffer_offset,
			//              frame_buffer_size - frame_buffer_offset, "%s%s▄",
			//              random_bg_color(), random_color());

			unsigned char top_cell = grid[((y)*screen_width) + x];
			char *top_color = get_cell_bg_color(top_cell);

			unsigned char bottom_cell = grid[((y + 1) * screen_width) + x];
			char *bottom_color = get_cell_color(bottom_cell);

			frame_buffer_offset +=
			    snprintf(frame_buffer + frame_buffer_offset,
			             frame_buffer_size - frame_buffer_offset, "%s%s▄",
			             top_color, bottom_color);
		}
		frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
		                                frame_buffer_size - frame_buffer_offset,
		                                "%s\n", RESET_STYLE);
	}
	frame_buffer_offset +=
	    snprintf(frame_buffer + frame_buffer_offset,
	             frame_buffer_size - frame_buffer_offset, "%s", gui());
	printf("%s", frame_buffer);

	fflush(stdout);
}

typedef struct termios Termios;

Termios enable_raw_term() {
	Termios orig, raw;
	tcgetattr(STDIN_FILENO, &orig);
	raw = orig;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	return orig;
}

typedef struct timeval Timeval;

int isInput() {
	Timeval timeout = {0, 0};
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
}

char getCell(int x, int y) {
	int index = (y * screen_width) + x;
	return grid[index];
}

void set_cell(int x, int y, char cell) {
	int index = (y * screen_width) + x;
	grid[index] = cell;
}

char input_char;
int mouse_button;
char mouse_event_type;

void handle_input() {
	while (isInput()) {
		read(STDIN_FILENO, &input_char, 1);

		if (input_char == '\e') {
			int seq_size = 64;
			char seq[seq_size];
			int i = 0;

			// read full escape sequences
			while (isInput() && i < seq_size - 1) {
				read(STDIN_FILENO, &seq[i], 1);
				if (seq[i] == 'A' || seq[i] == 'B' || seq[i] == 'C' ||
				    seq[i] == 'D' || seq[i] == 'M' || seq[i] == 'm' ||
				    seq[i] == 'I' || seq[i] == 'O') {
					i++;
					break;
				}
				i++;
			}
			seq[i] = '\0';

			// Arrow keys
			if (seq[0] == '[' && seq[1] != '<') {
				switch (seq[1]) {
				case 'A':
					last_input = '^';
					break;
				case 'B':
					last_input = 'V';
					break;
				case 'C':
					last_input = '>';
					break;
				case 'D':
					last_input = '<';
					break;
				}
			}
			// Mouse events (SGR format)
			else if (seq[0] == '[' && seq[1] == '<') {
				mouse_event_type = seq[i - 1];
				int tmpx = 0, tmpy = 0, tmpbtn = 0;
				if (sscanf(seq + 2, "%d;%d;%d", &tmpbtn, &tmpx, &tmpy) == 3) {
					mouse_button = tmpbtn;
					mouse_x = tmpx;
					mouse_y = tmpy;

					sim_mouse_x = mouse_x - 1;
					sim_mouse_y = ((mouse_y - 1) * 2);

					if (mouse_event_type == 'M') {
						if (mouse_button == 0 || mouse_button == 32) {
							set_cell(sim_mouse_x, sim_mouse_y, current_cell);
							set_cell(sim_mouse_x, sim_mouse_y + 1,
							         current_cell);
						}
						if (mouse_button == 2 || mouse_button == 34) {
							set_cell(sim_mouse_x, sim_mouse_y, EMPTY);
							set_cell(sim_mouse_x, sim_mouse_y + 1, EMPTY);
						}
					}
				}
				last_input = mouse_event_type;
			}
		}
		// Regular key input_char
		else {
			switch (input_char) {
			case 'q':
				running = false;
				break;
			case '1':
				current_cell = WALL;
				break;
			case '2':
				current_cell = SAND;
				break;
			case '3':
				current_cell = WATER;
				break;
			default:
				break;
			}
			last_input = input_char;
		}
	}
}

typedef struct timespec Timespec;

int main() {
	signal(SIGINT, handle_sigint);
	init_grid(50, 50);
	init_screen();

	Termios orig_term = enable_raw_term();

	Timespec start_time, end_time;
	long elapsed_time;
	long total_frame_time;
	long sleep_time;

	while (running) {
		clock_gettime(CLOCK_MONOTONIC, &start_time);

		render();
		handle_input();

		clock_gettime(CLOCK_MONOTONIC, &end_time);
		elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
		               (end_time.tv_nsec - start_time.tv_nsec) / 1000;

		total_frame_time = elapsed_time;

		if (elapsed_time < FRAME_TIME) {
			sleep_time = FRAME_TIME - elapsed_time;
			usleep(sleep_time);
			total_frame_time += sleep_time;
		}

		if (total_frame_time > 0) {
			fps = 1000000 / total_frame_time;
		}
	}

	free(grid);
	free(frame_buffer);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
	reset_term();
	return EXIT_SUCCESS;
}
