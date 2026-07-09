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
int target_fps = TARGET_FPS;
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
	grid_size = screen_width * screen_height;
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sim

void sim_sand(int x, int y);
void sim_water(int x, int y);

const Element element_registry[] = {
	[EMPTY] = { .name = "Empty",
               .color = BLACK,
               .bg_color = BG_BLACK,
               .density = 0,
               .sim_fn = NULL      },
	[WALL] = { .name = "Wall",
	           .color = WHITE,
	           .bg_color = BG_WHITE,
	           .density = 1000000,
	           .sim_fn = NULL      },
	[SAND] = { .name = "Sand",
	           .color = YELLOW,
	           .bg_color = BG_YELLOW,
	           .density = 100,
	           .sim_fn = sim_sand  },
	[WATER] = { .name = "Water",
               .color = BLUE,
               .bg_color = BG_BLUE,
               .density = 10,
               .sim_fn = sim_water }
};

static inline bool can_displace(char moving_cell, char target_cell) {
	return element_registry[(unsigned char)moving_cell].density >
	       element_registry[(unsigned char)target_cell].density;
}

void swap_cells(int x1, int y1, int x2, int y2) {
	char temp = getCell(x1, y1);
	set_cell(x1, y1, getCell(x2, y2));
	set_cell(x2, y2, temp);
}

void sim_sand(int x, int y) {
	if (y + 1 >= screen_height)
		return;

	char below = getCell(x, y + 1);

	// 1. Move straight down (into empty air or lighter liquids like water)
	if (can_displace(SAND, below)) {
		swap_cells(x, y, x, y + 1);
		return;
	}

	// 2. Pile logic: Choose a random direction diagonal down (left or right)
	int diag = (rand() % 2 == 0) ? 1 : -1;
	int new_x = x + diag;

	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = getCell(new_x, y + 1);
		if (can_displace(SAND, diag_below)) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}

	// Try the opposite diagonal direction if the first choice was blocked
	new_x = x - diag;
	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = getCell(new_x, y + 1);
		if (can_displace(SAND, diag_below)) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}
}

static void try_spread_water(int x, int y) {
	int spread_amount = 10;
	int spread_distance = 1 + (rand() % spread_amount);
	int dir = (rand() % 2 == 0) ? 1 : -1;

	// Check prime chosen direction
	int target_x = x;
	for (int i = 1; i <= spread_distance; i++) {
		int check_x = x + (dir * i);
		if (check_x < 0 || check_x >= screen_width)
			break;
		if (getCell(check_x, y) != EMPTY)
			break;
		target_x = check_x;
	}

	if (target_x != x) {
		swap_cells(x, y, target_x, y);
		return;
	}

	// Fallback to secondary direction
	target_x = x;
	for (int i = 1; i <= spread_distance; i++) {
		int check_x = x - (dir * i);
		if (check_x < 0 || check_x >= screen_width)
			break;
		if (getCell(check_x, y) != EMPTY)
			break;
		target_x = check_x;
	}

	if (target_x != x) {
		swap_cells(x, y, target_x, y);
	}
}

void sim_water(int x, int y) {
	if (y + 1 >= screen_height) {
		return;
	}

	char below = getCell(x, y + 1);

	// 1. Fall straight down
	if (can_displace(WATER, below)) {
		swap_cells(x, y, x, y + 1);
		return;
	}

	// 2. Try falling diagonally down (left/right)
	int diag = (rand() % 2 == 0) ? 1 : -1;
	int new_x = x + diag;

	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = getCell(new_x, y + 1);
		if (can_displace(WATER, diag_below)) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}

	new_x = x - diag;
	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = getCell(new_x, y + 1);
		if (can_displace(WATER, diag_below)) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}

	// 3. Spreading step if downward tracks are fully locked out
	try_spread_water(x, y);
}

void simulate() {
	static unsigned frame = 0;
	frame++;

	for (int y = screen_height - 1; y >= 0; y--) {
		if (frame & 1) {
			for (int x = 0; x < screen_width; x++) {
				unsigned char cell = getCell(x, y);
				ElementSimFn sim_fn = element_registry[cell].sim_fn;
				if (sim_fn) {
					sim_fn(x, y);
				}
			}
		} else {
			for (int x = screen_width - 1; x >= 0; x--) {
				unsigned char cell = getCell(x, y);
				ElementSimFn sim_fn = element_registry[cell].sim_fn;
				if (sim_fn) {
					sim_fn(x, y);
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	int opt;
	int set_width = 1;
	int set_height = 1;
	bool width_set = false;
	bool height_set = false;
	int term_width = 0;
	int term_height = 0;

	while ((opt = getopt(argc, argv, "w:h:f:")) != -1) {
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
		}
	}

	if (get_terminal_bounds(&term_width, &term_height)) {
		if (!width_set) {
			set_width = term_width;
		}
		if (!height_set) {
			set_height = term_height;
		}

		if (set_width > term_width) {
			set_width = term_width;
		}
		if (set_height > term_height) {
			set_height = term_height;
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

	srand((unsigned)time(NULL));

	while (running) {
		clock_gettime(CLOCK_MONOTONIC, &start_time);

		simulate();
		render();
		handle_input();

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
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
	reset_term();
	return EXIT_SUCCESS;
}
