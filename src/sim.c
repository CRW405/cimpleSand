#include "sim.h"

// Track active region in order to reduce uselss cell updates
int min_active_x = 0;
int max_active_x = 0;
int min_active_y = 0;
int max_active_y = 0;

int cell_densities[ELEMENT_COUNT];

void sim_sand(int x, int y);
void sim_water(int x, int y);
void sim_stone(int x, int y);

const Element element_registry[] = {
	[EMPTY] = { .name = "Empty",
               .color = BLACK,
               .bg_color = BG_BLACK,
               .color_len = 5,
               .bg_color_len = 5,
               .density = 0,
               .sim_fn = NULL      },

	[WALL] = { .name = "Wall",
	           .color = WHITE,
	           .bg_color = BG_WHITE,
	           .color_len = 5,
	           .bg_color_len = 5,
	           .density = 1000000,
	           .sim_fn = NULL      },

	[SAND] = { .name = "Sand",
	           .color = YELLOW,
	           .bg_color = BG_YELLOW,
	           .color_len = 5,
	           .bg_color_len = 5,
	           .density = 100,
	           .sim_fn = sim_sand  },

	[STONE] = { .name = "Stone",
               .color = GRAY,
               .bg_color = BG_GRAY,
               .color_len = 11,
               .bg_color_len = 13,
               .density = 100,
               .sim_fn = sim_stone },

	[WATER] = { .name = "Water",
               .color = BLUE,
               .bg_color = BG_BLUE,
               .color_len = 5,
               .bg_color_len = 5,
               .density = 10,
               .sim_fn = sim_water }
};

static inline void mark_active(int x, int y) {
	if (x < min_active_x)
		min_active_x = x;
	if (x > max_active_x)
		max_active_x = x;
	if (y < min_active_y)
		min_active_y = y;
	if (y > max_active_y)
		max_active_y = y;
}

char getCell(int x, int y) {
	if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
		return WALL;
	}
	return grid[(y * screen_width) + x] & ACTIVE_MASK;
}

void set_cell(int x, int y, char cell) {
	if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
		return;
	}
	int index = (y * screen_width) + x;
	char old_cell = grid[index] & ACTIVE_MASK;

	if (old_cell != EMPTY && cell == EMPTY)
		cell_count--;
	if (old_cell == EMPTY && cell != EMPTY)
		cell_count++;

	grid[index] = cell;
	mark_active(x, y);
}

void swap_cells(int x1, int y1, int x2, int y2) {
	int index1 = (y1 * screen_width) + x1;
	int index2 = (y2 * screen_width) + x2;
	unsigned char cell1 = grid[index1];
	unsigned char cell2 = grid[index2];
	grid[index1] = (cell2 & ACTIVE_MASK) | ACTIVE_BIT;
	grid[index2] = (cell1 & ACTIVE_MASK) | ACTIVE_BIT;

	mark_active(x1, y1);
	mark_active(x2, y2);
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

void sim_sand(int x, int y) {
	if (y + 1 >= screen_height)
		return;

	int base_index = (y * screen_width) + x;
	int below_index = base_index + screen_width;
	char below = grid[below_index] & ACTIVE_MASK;

	if (cell_densities[SAND] > cell_densities[(unsigned char)below]) {
		swap_cells(x, y, x, y + 1);
		return;
	}

	int diag = (rand() % 2 == 0) ? 1 : -1;
	int new_x = x + diag;

	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = grid[below_index + diag] & ACTIVE_MASK;
		if (cell_densities[SAND] > cell_densities[(unsigned char)diag_below]) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}

	new_x = x - diag;
	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = grid[below_index - diag] & ACTIVE_MASK;
		if (cell_densities[SAND] > cell_densities[(unsigned char)diag_below]) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}
}

void sim_stone(int x, int y) {
	if (y + 1 >= screen_height)
		return;
	int base_index = (y * screen_width) + x;
	char below = grid[base_index + screen_width] & ACTIVE_MASK;

	if (cell_densities[STONE] > cell_densities[(unsigned char)below]) {
		swap_cells(x, y, x, y + 1);
		return;
	}
}

static void try_spread_water(int x, int y) {
	int spread_amount = 10;
	int dir = (rand() % 2 == 0) ? 1 : -1;

	for (int d = 0; d < 2; d++) {
		int current_dir = (d == 0) ? dir : -dir;
		int target_x = x;

		for (int i = 1; i <= spread_amount; i++) {
			int check_x = x + (current_dir * i);
			if (check_x < 0 || check_x >= screen_width)
				break;
			if (getCell(check_x, y) != EMPTY)
				break;

			target_x = check_x;

			char cell_below = getCell(check_x, y + 1);
			if (cell_densities[WATER] > cell_densities[(unsigned char)cell_below]) {
				swap_cells(x, y, check_x, y + 1);
				break;
			}
		}

		if (target_x != x) {
			swap_cells(x, y, target_x, y);
			return;
		}
	}
}

void sim_water(int x, int y) {
	if (y + 1 >= screen_height)
		return;

	int base_index = (y * screen_width) + x;
	int below_index = base_index + screen_width;
	char below = grid[below_index] & ACTIVE_MASK;

	if (cell_densities[WATER] > cell_densities[(unsigned char)below]) {
		swap_cells(x, y, x, y + 1);
		return;
	}

	int diag = (rand() % 2 == 0) ? 1 : -1;
	int new_x = x + diag;

	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = grid[below_index + diag] & ACTIVE_MASK;
		if (cell_densities[WATER] > cell_densities[(unsigned char)diag_below]) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}

	new_x = x - diag;
	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = grid[below_index - diag] & ACTIVE_MASK;
		if (cell_densities[WATER] > cell_densities[(unsigned char)diag_below]) {
			swap_cells(x, y, new_x, y + 1);
			return;
		}
	}

	if (y > 0 && x >= 2 && x < screen_width - 2) {
		char above = grid[base_index - screen_width] & ACTIVE_MASK;
		char left = grid[base_index - 1] & ACTIVE_MASK;
		char leftleft = grid[base_index - 2] & ACTIVE_MASK;
		char right = grid[base_index + 1] & ACTIVE_MASK;
		char rightright = grid[base_index + 2] & ACTIVE_MASK;

		int evap_chance = 10;
		if (above != WATER && left != WATER && right != WATER &&
		    leftleft != WATER && rightright != WATER) {
			if (rand() % evap_chance == 0) {
				set_cell(x, y, EMPTY);
				return;
			}
		}
	}

	try_spread_water(x, y);
}

void simulate() {
	if (min_active_x > max_active_x || min_active_y > max_active_y) {
		return;
	}

	static unsigned frame = 0;
	frame++;

	int start_y = (max_active_y + 1 < screen_height) ? max_active_y + 1 : screen_height - 1;
	int end_y = (min_active_y - 1 >= 0) ? min_active_y - 1 : 0;
	int start_x = (min_active_x - 1 >= 0) ? min_active_x - 1 : 0;
	int end_x = (max_active_x + 1 < screen_width) ? max_active_x + 1 : screen_width - 1;

	min_active_x = screen_width;
	max_active_x = 0;
	min_active_y = screen_height;
	max_active_y = 0;

	for (int y = start_y; y >= end_y; y--) {
		int row_offset = y * screen_width;

		if (frame & 1) {
			for (int x = start_x; x <= end_x; x++) {
				unsigned char raw_cell = grid[row_offset + x];
				if (raw_cell & ACTIVE_BIT)
					continue;

				unsigned char cell = raw_cell & ACTIVE_MASK;
				ElementSimFn sim_fn = element_registry[cell].sim_fn;
				if (sim_fn) {
					mark_active(x, y);
					sim_fn(x, y);
				}
			}
		} else {
			for (int x = end_x; x >= start_x; x--) {
				unsigned char raw_cell = grid[row_offset + x];
				if (raw_cell & ACTIVE_BIT)
					continue;

				unsigned char cell = raw_cell & ACTIVE_MASK;
				ElementSimFn sim_fn = element_registry[cell].sim_fn;
				if (sim_fn) {
					mark_active(x, y);
					sim_fn(x, y);
				}
			}
		}
	}

	for (int y = start_y; y >= end_y; y--) {
		int row_offset = y * screen_width;
		for (int x = start_x; x <= end_x; x++) {
			grid[row_offset + x] &= ACTIVE_MASK;
		}
	}
}
