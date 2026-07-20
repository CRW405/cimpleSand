#include "sim.h"

int min_active_x = 0;
int max_active_x = 0;
int min_active_y = 0;
int max_active_y = 0;

int cell_densities[ELEMENT_COUNT];

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

char get_cell(int x, int y) {
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
	mark_active(x - 1, y - 1);
	mark_active(x + 1, y + 1);
}

void swap_cells(int x1, int y1, int x2, int y2) {
	int index1 = (y1 * screen_width) + x1;
	int index2 = (y2 * screen_width) + x2;
	unsigned char cell1 = grid[index1];
	unsigned char cell2 = grid[index2];
	grid[index1] = (cell2 & ACTIVE_MASK) | ACTIVE_BIT;
	grid[index2] = (cell1 & ACTIVE_MASK) | ACTIVE_BIT;

	mark_active(x1 - 1, y1 - 1);
	mark_active(x1 + 1, y1 + 1);
	mark_active(x2 - 1, y2 - 1);
	mark_active(x2 + 1, y2 + 1);
}

void paint(int x_center, int y_center, int radius, char cell) {
	for (int x = x_center - radius; x <= x_center; x++) {
		for (int y = y_center - radius; y <= y_center; y++) {
			int dx = x - x_center;
			int dy = y - y_center;

			if ((dx * dx) + (dy * dy) <= (radius * radius)) {
				int x_sym = x_center - dx;
				int y_sym = y_center - dy;

				if (get_cell(x, y) == EMPTY || cell == EMPTY)
					set_cell(x, y, cell);

				if (get_cell(x_sym, y) == EMPTY || cell == EMPTY)
					set_cell(x_sym, y, cell);

				if (get_cell(x, y_sym) == EMPTY || cell == EMPTY)
					set_cell(x, y_sym, cell);

				if (get_cell(x_sym, y_sym) == EMPTY || cell == EMPTY)
					set_cell(x_sym, y_sym, cell);
			}
		}
	}
}

void simulate() {
	if (min_active_x > max_active_x || min_active_y > max_active_y) {
		return;
	}

	static unsigned frame = 0;
	frame++;

	int start_y = (max_active_y + 2 < screen_height) ? max_active_y + 2 : screen_height - 1;
	int end_y = (min_active_y - 2 >= 0) ? min_active_y - 2 : 0;
	int start_x = (min_active_x - 2 >= 0) ? min_active_x - 2 : 0;
	int end_x = (max_active_x + 2 < screen_width) ? max_active_x + 2 : screen_width - 1;

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
