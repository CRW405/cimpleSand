#include "sim.h"

// Track active region in order to reduce useless cell updates
int min_active_x = 0;
int max_active_x = 0;
int min_active_y = 0;
int max_active_y = 0;

int cell_densities[ELEMENT_COUNT];

void sim_sand(int x, int y);
void sim_water(int x, int y);
void sim_stone(int x, int y);
void sim_oil(int x, int y);
void sim_fire(int x, int y);
void sim_steam(int x, int y);
void sim_lava(int x, int y);
void sim_wood(int x, int y);
void sim_ash(int x, int y);
void sim_ember(int x, int y);

const Element element_registry[] = {
	[EMPTY] = { .name = "Empty",
               .color = BLACK,
               .bg_color = BG_BLACK,
               .color_len = sizeof(BLACK) - 1,
               .bg_color_len = sizeof(BG_BLACK) - 1,
               .density = 0,
               .sim_fn = NULL      },

	[WALL] = { .name = "Wall",
	           .color = WHITE,
	           .bg_color = BG_WHITE,
	           .color_len = sizeof(WHITE) - 1,
	           .bg_color_len = sizeof(BG_WHITE) - 1,
	           .density = 255,
	           .sim_fn = NULL      },

	[SAND] = { .name = "Sand",
	           .color = YELLOW,
	           .bg_color = BG_YELLOW,
	           .color_len = sizeof(YELLOW) - 1,
	           .bg_color_len = sizeof(BG_YELLOW) - 1,
	           .density = 100,
	           .sim_fn = sim_sand  },

	[STONE] = { .name = "Stone",
               .color = GRAY,
               .bg_color = BG_GRAY,
               .color_len = sizeof(GRAY) - 1,
               .bg_color_len = sizeof(BG_GRAY) - 1,
               .density = 100,
               .sim_fn = sim_stone },

	[WATER] = { .name = "Water",
               .color = BLUE,
               .bg_color = BG_BLUE,
               .color_len = sizeof(BLUE) - 1,
               .bg_color_len = sizeof(BG_BLUE) - 1,
               .density = 50,
               .sim_fn = sim_water },

	[OIL] = { .name = "Oil",
	           .color = PURPLE,
	           .bg_color = BG_PURPLE,
	           .color_len = sizeof(PURPLE) - 1,
	           .bg_color_len = sizeof(BG_PURPLE) - 1,
	           .density = 10,
	           .sim_fn = sim_oil   },

	[FIRE] = { .name = "Fire",
	           .color = RED,
	           .bg_color = BG_RED,
	           .color_len = sizeof(RED) - 1,
	           .bg_color_len = sizeof(BG_RED) - 1,
	           .density = 1,
	           .sim_fn = sim_fire  },

	[STEAM] = { .name = "Steam",
               .color = WHITE,
               .bg_color = BG_WHITE,
               .color_len = sizeof(WHITE) - 1,
               .bg_color_len = sizeof(BG_WHITE) - 1,
               .density = 1,
               .sim_fn = sim_steam },

	[LAVA] = { .name = "Lava",
	           .color = RED,
	           .bg_color = BG_RED,
	           .color_len = sizeof(RED) - 1,
	           .bg_color_len = sizeof(BG_RED) - 1,
	           .density = 50,
	           .sim_fn = sim_lava  },

	[WOOD] = { .name = "Wood",
	           .color = BROWN,
	           .bg_color = BG_BROWN,
	           .color_len = sizeof(BROWN) - 1,
	           .bg_color_len = sizeof(BG_BROWN) - 1,
	           .density = 100,
	           .sim_fn = sim_wood  },

	[ASH] = { .name = "Ash",
	           .color = LGRAY,
	           .bg_color = BG_LGRAY,
	           .color_len = sizeof(LGRAY) - 1,
	           .bg_color_len = sizeof(BG_LGRAY) - 1,
	           .density = 100,
	           .sim_fn = sim_ash   },

	[EMBER] = { .name = "Ember",
               .color = ORANGE,
               .bg_color = BG_ORANGE,
               .color_len = sizeof(ORANGE) - 1,
               .bg_color_len = sizeof(BG_ORANGE) - 1,
               .density = 100,
               .sim_fn = sim_ember }
};

void init_cell_densities(void) {
	for (int i = 0; i < ELEMENT_COUNT; i++) {
		cell_densities[i] = element_registry[i].density;
	}
}

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

static inline void swap_cells(int x1, int y1, int x2, int y2) {
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

static inline bool try_fall_down(int x, int y, char element_id) {
	if (y + 1 >= screen_height)
		return false;

	int base_index = (y * screen_width) + x;
	int below_index = base_index + screen_width;
	char below = grid[below_index] & ACTIVE_MASK;
	char current_density = cell_densities[(unsigned char)element_id];

	if (current_density > cell_densities[(unsigned char)below]) {
		swap_cells(x, y, x, y + 1);
		return true;
	}
	return false;
}

bool try_fall_diagonal(int x, int y, char element_id) {
	if (y + 1 >= screen_height)
		return false;

	int base_index = (y * screen_width) + x;
	int below_index = base_index + screen_width;
	char current_density = cell_densities[(unsigned char)element_id];

	int diag = (rand() % 2 == 0) ? 1 : -1;
	int new_x = x + diag;

	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = grid[below_index + diag] & ACTIVE_MASK;
		if (current_density > cell_densities[(unsigned char)diag_below]) {
			swap_cells(x, y, new_x, y + 1);
			return true;
		}
	}

	new_x = x - diag;
	if (new_x >= 0 && new_x < screen_width) {
		char diag_below = grid[below_index - diag] & ACTIVE_MASK;
		if (current_density > cell_densities[(unsigned char)diag_below]) {
			swap_cells(x, y, new_x, y + 1);
			return true;
		}
	}

	return false;
}

bool try_liquid_flow(int x, int y, char element_id, int flow_limit) {
	int dir = (rand() % 2 == 0) ? 1 : -1;
	char current_density = cell_densities[(unsigned char)element_id];

	for (int d = 0; d < 2; d++) {
		int current_dir = (d == 0) ? dir : -dir;
		int target_x = x;

		for (int i = 1; i <= flow_limit; i++) {
			int check_x = x + (current_dir * i);
			if (check_x < 0 || check_x >= screen_width)
				break;

			char side_cell = get_cell(check_x, y);
			if (current_density <= cell_densities[(unsigned char)side_cell])
				break;

			target_x = check_x;

			char cell_below = get_cell(check_x, y + 1);
			if (current_density > cell_densities[(unsigned char)cell_below]) {
				swap_cells(x, y, check_x, y + 1);
				return true;
			}
		}

		if (target_x != x) {
			swap_cells(x, y, target_x, y);
			return true;
		}
	}

	return false;
}

bool try_liquid_evaporation(int x, int y, char element_id, int chance) {
	char above = get_cell(x, y - 1);
	char left = get_cell(x - 1, y);
	char leftleft = get_cell(x - 2, y);
	char right = get_cell(x + 1, y);
	char rightright = get_cell(x + 2, y);

	if (above != element_id && left != element_id && right != element_id &&
	    leftleft != element_id && rightright != element_id) {
		if (rand() % chance == 0) {
			set_cell(x, y, EMPTY);
			return true;
		}
	}

	return false;
}

static inline bool try_rise_up(int x, int y, char element_id) {
	if (y - 1 < 0) {
		return false;
	}

	int base_index = (y * screen_width) + x;
	int above_index = base_index - screen_width;
	char above = grid[above_index] & ACTIVE_MASK;
	char current_density = cell_densities[(unsigned char)element_id];

	// Restored: Gas (positive density) is heavier than empty air (0), so it rises if current > above
	if (current_density > cell_densities[(unsigned char)above] && above != WALL) {
		swap_cells(x, y, x, y - 1);
		return true;
	}
	return false;
}

static inline bool try_rise_diagonal(int x, int y, char element_id) {
	if (y - 1 < 0)
		return false;

	int base_index = (y * screen_width) + x;
	int above_index = base_index - screen_width;
	char current_density = cell_densities[(unsigned char)element_id];

	int diag = (rand() % 2 == 0) ? 1 : -1;
	int new_x = x + diag;

	if (new_x >= 0 && new_x < screen_width) {
		char diag_above = grid[above_index + diag] & ACTIVE_MASK;
		if (current_density > cell_densities[(unsigned char)diag_above] && diag_above != WALL) {
			swap_cells(x, y, new_x, y - 1);
			return true;
		}
	}

	new_x = x - diag;
	if (new_x >= 0 && new_x < screen_width) {
		char diag_above = grid[above_index - diag] & ACTIVE_MASK;
		if (current_density > cell_densities[(unsigned char)diag_above] && diag_above != WALL) {
			swap_cells(x, y, new_x, y - 1);
			return true;
		}
	}

	return false;
}

static inline bool try_gas_drift(int x, int y, char element_id, int drift_limit) {
	int dir = (rand() % 2 == 0) ? 1 : -1;
	char current_density = cell_densities[(unsigned char)element_id];

	for (int d = 0; d < 2; d++) {
		int current_dir = (d == 0) ? dir : -dir;
		int target_x = x;

		for (int i = 1; i <= drift_limit; i++) {
			int check_x = x + (current_dir * i);
			if (check_x < 0 || check_x >= screen_width)
				break;

			char side_cell = get_cell(check_x, y);
			// Restored: Gas drifts sideways into less dense spaces (like empty air)
			if (current_density < cell_densities[(unsigned char)side_cell] || side_cell == WALL)
				break;

			target_x = check_x;

			char cell_above = get_cell(check_x, y - 1);
			if (current_density > cell_densities[(unsigned char)cell_above] && cell_above != WALL) {
				swap_cells(x, y, check_x, y - 1);
				return true;
			}
		}

		if (target_x != x) {
			swap_cells(x, y, target_x, y);
			return true;
		}
	}

	return false;
}

// Allows steam/gas to flow outward horizontally under ceilings, matching your positive density setup
bool try_gas_flow(int x, int y, char element_id, int flow_limit) {
	int dir = (rand() % 2 == 0) ? 1 : -1;
	char current_density = cell_densities[(unsigned char)element_id];

	for (int d = 0; d < 2; d++) {
		int current_dir = (d == 0) ? dir : -dir;
		int target_x = x;

		for (int i = 1; i <= flow_limit; i++) {
			int check_x = x + (current_dir * i);
			if (check_x < 0 || check_x >= screen_width)
				break;

			char side_cell = get_cell(check_x, y);
			// Stop flowing if we hit something denser than us (like water, stone, or a wall)
			if (current_density < cell_densities[(unsigned char)side_cell] || side_cell == WALL)
				break;

			target_x = check_x;

			// Slip upward mid-drift if there is a less dense cell above
			char cell_above = get_cell(check_x, y - 1);
			if (current_density > cell_densities[(unsigned char)cell_above] && cell_above != WALL) {
				swap_cells(x, y, check_x, y - 1);
				return true;
			}
		}

		if (target_x != x) {
			swap_cells(x, y, target_x, y);
			return true;
		}
	}

	return false;
}

void sim_sand(int x, int y) {
	if (try_fall_down(x, y, SAND))
		return;
	try_fall_diagonal(x, y, SAND);
}

void sim_stone(int x, int y) {
	try_fall_down(x, y, STONE);
}

void sim_water(int x, int y) {
	if (try_fall_down(x, y, WATER))
		return;
	if (try_fall_diagonal(x, y, WATER))
		return;
	if (try_liquid_evaporation(x, y, WATER, 10))
		return;

	try_liquid_flow(x, y, WATER, 10);
}

void sim_oil(int x, int y) {
	if (try_fall_down(x, y, OIL))
		return;
	if (try_fall_diagonal(x, y, OIL))
		return;

	try_liquid_flow(x, y, OIL, 1);
}

void sim_fire(int x, int y) {
	int dx[] = { 0, 0, -1, 1 };
	int dy[] = { -1, 1, 0, 0 };

	for (int i = 0; i < 4; i++) {
		int nx = x + dx[i];
		int ny = y + dy[i];
		char neighbor = get_cell(nx, ny);

		switch (neighbor) {
		case WATER:
			set_cell(nx, ny, STEAM);
			set_cell(x, y, EMPTY);
			return;
		case OIL:
			set_cell(nx, ny, FIRE);
			return;
		case WOOD:
			set_cell(nx, ny, EMBER);
			break;
		}
	}
	if (rand() % 10 == 0) {
		set_cell(x, y, EMPTY);
		return;
	}

	if (try_rise_up(x, y, FIRE))
		return;
	if (try_rise_diagonal(x, y, FIRE))
		return;

	try_gas_drift(x, y, FIRE, 1);
}

void sim_steam(int x, int y) {
	if (try_rise_up(x, y, STEAM))
		return;
	if (try_rise_diagonal(x, y, STEAM))
		return;
	int roll = rand() % 1000;
	if (roll < 2) {
		set_cell(x, y, WATER);
	} else if (roll < 10) {
		set_cell(x, y, EMPTY);
	}
	if (try_gas_flow(x, y, STEAM, 5))
		return;
}

void sim_lava(int x, int y) {
	if (try_fall_down(x, y, LAVA))
		return;
	if (try_fall_diagonal(x, y, LAVA))
		return;

	int dx[] = { 0, 0, -1, 1 };
	int dy[] = { -1, 1, 0, 0 };

	for (int i = 0; i < 4; i++) {
		int nx = x + dx[i];
		int ny = y + dy[i];
		char neighbor = get_cell(nx, ny);

		switch (neighbor) {
		case WATER:
			set_cell(nx, ny, STEAM);
			set_cell(x, y, STONE);
			return;
		case OIL:
			set_cell(nx, ny, FIRE);
			return;
		case WOOD:
			set_cell(nx, ny, EMBER);
			break;
		}
	}

	try_liquid_flow(x, y, LAVA, 1);
}

void sim_wood(int x, int y) {
	int dx[] = { 0, 0, -1, 1, -1, -1, 1, 1 };
	int dy[] = { -1, 1, 0, 0, -1, 1, -1, 1 };

	for (int i = 0; i < 8; i++) {
		char neighbor = get_cell(x + dx[i], y + dy[i]);
		if (neighbor == FIRE || neighbor == LAVA) {
			set_cell(x, y, EMBER);
			return;
		}
	}
}

void sim_ember(int x, int y) {
	int dx[] = { 0, 0, -1, 1 };
	int dy[] = { -1, 1, 0, 0 };

	for (int i = 0; i < 4; i++) {
		char neighbor = get_cell(x + dx[i], y + dy[i]);
		if (neighbor == WATER) {
			set_cell(x + dx[i], y + dy[i], STEAM);
			set_cell(x, y, WOOD);
			return;
		}
	}

	if (rand() % 15 == 0) {
		if (y - 1 >= 0 && get_cell(x, y - 1) == EMPTY)
			set_cell(x, y - 1, FIRE);
	}
	if (rand() % 120 == 0) {
		set_cell(x, y, ASH);
		if (y - 1 >= 0 && get_cell(x, y - 1) == EMPTY)
			set_cell(x, y - 1, FIRE);
		return;
	}
}

void sim_ash(int x, int y) {
	if (try_fall_down(x, y, ASH))
		return;
	try_fall_diagonal(x, y, ASH);
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
