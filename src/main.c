#include "common.h"
#include "input.h"
#include "render.h"
#include "term_ops.h"

bool running = true;
int current_cell = WALL;
int screen_width;
int screen_height;
int grid_size;
unsigned char *grid;
char *frame_buffer;
int frame_buffer_size;
int fps;
int cell_count;
char last_input = ' ';
int mouse_x = 0;
int mouse_y = 0;
int sim_mouse_x = 0;
int sim_mouse_y = 0;
int cur_radius = 1;

void handle_sigint(int sig) {
	(void)sig;
	running = false;
}

void init_grid(int width, int height) {
	screen_width = width;
	screen_height = (height % 2 == 0) ? height : height + 1;
	grid_size = width * height;
	grid = calloc(grid_size, sizeof(unsigned char));
	frame_buffer_size = (grid_size * 25) + 256;
	frame_buffer = malloc(frame_buffer_size);
}

char getCell(int x, int y) {
	if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
		return WALL;
	}
	return grid[(y * screen_width) + x];
}

void set_cell(int x, int y, char cell) {
	if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
		return;
	}
	grid[(y * screen_width) + x] = cell;
}

void paint(int x_center, int y_center, int radius, char cell) {
	for (int x = x_center - radius; x <= x_center; x++) {
		for (int y = y_center - radius; y <= y_center; y++) {
			int dx = x - x_center;
			int dy = y - y_center;

			if ((dx * dx) + (dy * dy) <= (radius * radius)) {
				int x_sym = x_center - dx;
				int y_sym = y_center - dy;

				set_cell(x, y, cell);
				set_cell(x_sym, y, cell);
				set_cell(x, y_sym, cell);
				set_cell(x_sym, y_sym, cell);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int opt;
	int set_width = 100;
	int set_height = 100;

	while ((opt = getopt(argc, argv, "w:h:")) != -1) {
		switch (opt) {
		case 'w':
			set_width = atoi(optarg);
			break;
		case 'h':
			set_height = atoi(optarg);
			break;
		}
	}

	signal(SIGINT, handle_sigint);
	init_grid(set_width, set_height);
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
