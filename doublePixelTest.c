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

void init_screen() { term_op(3, true, CLEAR, HIDE_CUR, ALT_SCREEN); }
void reset_term() { term_op(2, true, SHOW_CUR, MAIN_SCREEN); }

bool running = true;

void handle_sigint(int sig) {
	(void)sig;
	running = false;
}

char *random_color() {
	int r = rand() % 8;
	switch (r) {
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
	int r = rand() % 8;
	switch (r) {
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
	screen_height = height;
	grid_size = width * height;
	grid = malloc(grid_size);

	// Each character text cell now takes up to ~20 bytes due to matching
	// both foreground + background ANSI escapes plus multi-byte UTF8 "▄"
	// characters.
	frame_buffer_size = (grid_size * 25) + sizeof(gui_buffer);
	frame_buffer = malloc(frame_buffer_size);
}

int fps;
int cell_count;
char last_input = ' ';

char *gui() {
	snprintf(gui_buffer, sizeof(gui_buffer),
	         "FPS: %d | Cells: %d | Last Input: %c", fps, cell_count,
	         last_input);
	return gui_buffer;
}

int frame_buffer_offset;

void render() {
	term_op(1, false, CUR_TO_TOP);
	frame_buffer_offset = 0;

	// Notice: We advance the visual text row loop by 2 lines of resolution per
	// pass!
	for (int y = 0; y < screen_height; y += 2) {
		for (int x = 0; x < screen_width; x++) {

			// For this test preview, we directly generate random colors for the
			// Top Half (Background color) and Bottom Half (Foreground color)
			char *top_color = random_bg_color();
			char *bottom_color = random_color();

			frame_buffer_offset +=
			    snprintf(frame_buffer + frame_buffer_offset,
			             frame_buffer_size - frame_buffer_offset, "%s%s▄",
			             top_color, bottom_color);
		}

		// Reset styles cleanly at the boundary of each row before appending the
		// newline
		frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
		                                frame_buffer_size - frame_buffer_offset,
		                                "%s\n", RESET_STYLE);
	}

	// Append the interactive GUI metadata text bar to the final string
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

char input_char;

void handle_input() {
	if (isInput()) {
		if (read(STDIN_FILENO, &input_char, 1) > 0) {
			switch (input_char) {
			case 'q':
				running = false; // Fixed: Restored quit interaction switch loop
				                 // boundary
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
	srand(time(
	    NULL)); // Seeds random generator so patterns shift dynamically on boot
	signal(SIGINT, handle_sigint);

	// Setting up a grid. 50x50 means 50 horizontal pixels by 50 vertical
	// pixels. Thanks to our half-block optimizations, this will take up a 50x25
	// character viewport box!
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
			fps = (int)(1000000 / total_frame_time);
		}
	}

	free(grid);
	free(frame_buffer); // Freeing the custom frame buffer allocation
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
	reset_term();
	return EXIT_SUCCESS;
}
