#include <getopt.h>

#include "common.h"
#include "history.h"
#include "input.h"
#include "player.h"
#include "render.h"
#include "sim.h"
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
int target_fps = TARGET_FPS;
char last_input = ' ';
int mouse_x = 0;
int mouse_y = 0;
int sim_mouse_x = 0;
int sim_mouse_y = 0;
int cur_radius = 1;
int cell_count = 0;
bool enable_player = false;
bool step_mode = false;
bool paused = false;

void handle_sigint(int sig) {
	(void)sig;
	running = false;
}

void init_grid(int width, int height) {
	screen_width = width;
	screen_height = (height % 2 == 0) ? height : height + 1;
	grid_size = screen_width * screen_height;
	grid = calloc(grid_size, sizeof(unsigned char));
	frame_buffer_size = (grid_size * 25) + 256;
	frame_buffer = malloc(frame_buffer_size);

	if (!grid || !frame_buffer) {
		free(grid);
		free(frame_buffer);
		fprintf(stderr, "Failed to allocate simulation buffers.\n");
		exit(EXIT_FAILURE);
	}

	min_active_x = screen_width;
	max_active_x = 0;
	min_active_y = screen_height;
	max_active_y = 0;
}

int main(int argc, char *argv[]) {
	int opt;
	int set_width = 1;
	int set_height = 1;
	bool width_set = false;
	bool height_set = false;
	int term_width = 0;
	int term_height = 0;
	int history_capacity = DEFAULT_HISTORY_CAPACITY;
	enable_player = false;
	step_mode = false;
	paused = false;

	static const struct option long_options[] = {
		{ "step", no_argument, NULL, 's' },
		{ "memory", required_argument, NULL, 'm' },
		{ NULL, 0, NULL, 0 }
	};

	while ((opt = getopt_long(argc, argv, "w:h:f:p", long_options, NULL)) != -1) {
		switch (opt) {
		case 'w':
			set_width = atoi(optarg) > 1 ? atoi(optarg) : set_width;
			width_set = true;
			break;
		case 'h':
			set_height = atoi(optarg) > 1 ? atoi(optarg) : set_height;
			height_set = true;
			break;
		case 'f':
			target_fps = atoi(optarg) != 0 ? atoi(optarg) : TARGET_FPS;
			break;
		case 'p':
			enable_player = true;
			break;
		case 's':
			step_mode = true;
			break;
		case 'm':
			history_capacity = atoi(optarg) > 0 ? atoi(optarg) : DEFAULT_HISTORY_CAPACITY;
			break;
		}
	}

	if (get_terminal_bounds(&term_width, &term_height)) {
		if (!width_set)
			set_width = term_width;
		if (!height_set)
			set_height = term_height;
		if (set_width > term_width)
			set_width = term_width;
		if (set_height > term_height)
			set_height = term_height;
	}

	signal(SIGINT, handle_sigint);
	init_grid(set_width, set_height);
	if (enable_player) {
		init_player();
	}
	if (step_mode) {
		init_history(history_capacity);
	}
	init_screen();

	// precompute cell densities in a way optimized for cache locality
	for (int i = 0; i < ELEMENT_COUNT; i++) {
		cell_densities[i] = element_registry[i].density;
	}

	Termios orig_term = enable_raw_term();
	init_input();
	Timespec start_time, end_time;
	long elapsed_time, total_frame_time, sleep_time;

	srand((unsigned)time(NULL));

	// fps is limeted by sleeping until target fps is reached
	while (running) {
		clock_gettime(CLOCK_MONOTONIC, &start_time);

		if (!paused) {
			simulate();
		}
		render();
		handle_input();

		if (enable_player && !paused)
			update_player();

		if (step_mode && !paused)
			history_push();

		clock_gettime(CLOCK_MONOTONIC, &end_time);
		elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
		               (end_time.tv_nsec - start_time.tv_nsec) / 1000;

		total_frame_time = elapsed_time;
		int frame_time = 1000000 / target_fps;
		if (elapsed_time < frame_time) {
			sleep_time = frame_time - elapsed_time;
			usleep(sleep_time);
			total_frame_time += sleep_time;
		}

		if (total_frame_time > 0) {
			fps = 1000000 / total_frame_time;
		}
	}

	free(grid);
	free(frame_buffer);
	if (step_mode) {
		shutdown_history();
	}
	shutdown_input();
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
	reset_term();
	return EXIT_SUCCESS;
}
