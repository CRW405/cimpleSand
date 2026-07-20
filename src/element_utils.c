#include "sim.h"

static void explode_at(int cx, int cy, int radius, int power, int depth);

bool try_fall_down(int x, int y, char element_id) {
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

// Attempts to flow a liquid element left or right, up to a specified flow limit in an attempt to settle.
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

bool try_rise_up(int x, int y, char element_id) {
	if (y - 1 < 0) {
		return false;
	}

	int base_index = (y * screen_width) + x;
	int above_index = base_index - screen_width;
	char above = grid[above_index] & ACTIVE_MASK;
	char current_density = cell_densities[(unsigned char)element_id];

	if (current_density > cell_densities[(unsigned char)above] && above != WALL) {
		swap_cells(x, y, x, y - 1);
		return true;
	}
	return false;
}

bool try_rise_diagonal(int x, int y, char element_id) {
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

// Makes gases spread out as it rises
bool try_gas_drift(int x, int y, char element_id, int drift_limit) {
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

// Kinda like try_liquid_flow but for gases
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

void trigger_explosion(int start_x, int start_y, int depth) {
	// --- TUNE THESE ---
	int max_depth = 5;
	int power_multiplier = 200;
	int radius_per_density = 100;
	int min_radius = 2;
	int max_radius = 15;
	int max_cells = 4096;
	// -------------------

	if (depth > max_depth)
		return;
	if (get_cell(start_x, start_y) != GUNPOWDER)
		return;

	static int pxs[8192], pys[8192];
	int count = 0;
	int total_density = 0;

	int ndx[] = { 0, 0, -1, 1 };
	int ndy[] = { -1, 1, 0, 0 };

	pxs[0] = start_x;
	pys[0] = start_y;
	count = 1;
	total_density += cell_densities[GUNPOWDER];
	set_cell(start_x, start_y, EMPTY);

	int head = 0;
	while (head < count) {
		int cx = pxs[head];
		int cy = pys[head];
		head++;

		for (int i = 0; i < 4; i++) {
			int nx = cx + ndx[i];
			int ny = cy + ndy[i];
			if (nx < 0 || nx >= screen_width || ny < 0 || ny >= screen_height)
				continue;
			if (get_cell(nx, ny) == GUNPOWDER && count < max_cells) {
				pxs[count] = nx;
				pys[count] = ny;
				total_density += cell_densities[GUNPOWDER];
				set_cell(nx, ny, EMPTY);
				count++;
			}
		}
	}

	int sx = 0, sy = 0;
	for (int i = 0; i < count; i++) {
		sx += pxs[i];
		sy += pys[i];
	}
	int blast_cx = sx / count;
	int blast_cy = sy / count;

	int power = total_density * power_multiplier / 100;
	int radius = power / radius_per_density;
	if (radius < min_radius)
		radius = min_radius;
	if (radius > max_radius)
		radius = max_radius;

	explode_at(blast_cx, blast_cy, radius, power, depth);
}

static void explode_at(int cx, int cy, int radius, int power, int depth) {
	// --- TUNE THESE ---
	int max_depth = 5;
	float destroy_threshold = 1.0f;
	float damage_threshold = 0.5f;
	float scorch_threshold = 0.2f;
	int ember_density = 3;
	// -------------------

	if (depth > max_depth)
		return;

	float radius_sq = (float)(radius * radius);

	for (int dy = -radius; dy <= radius; dy++) {
		for (int dx = -radius; dx <= radius; dx++) {
			int x = cx + dx;
			int y = cy + dy;
			if (x < 0 || x >= screen_width || y < 0 || y >= screen_height)
				continue;

			float dist_sq = (float)(dx * dx + dy * dy);
			if (dist_sq > radius_sq)
				continue;

			unsigned char cell = get_cell(x, y);
			if (cell == EMPTY) {
				if (dist_sq < radius_sq * 0.09f)
					set_cell(x, y, FIRE);
				continue;
			}

			float force = (float)power * (1.0f - dist_sq / radius_sq);
			unsigned char cell_dens = cell_densities[cell];
			float ratio = force / (float)cell_dens;

			if (cell == GUNPOWDER) {
				set_cell(x, y, EMPTY);
				trigger_explosion(x, y, depth + 1);
				if (get_cell(x, y) == EMPTY)
					set_cell(x, y, FIRE);
				continue;
			}

			if (ratio > destroy_threshold) {
				set_cell(x, y, (dist_sq < radius_sq * 0.09f) ? FIRE : EMPTY);
			} else if (ratio > damage_threshold) {
				switch (cell) {
				case WOOD:
					set_cell(x, y, EMBER);
					break;
				case OIL:
					set_cell(x, y, FIRE);
					break;
				case WATER:
					set_cell(x, y, STEAM);
					break;
				}
			} else if (ratio > scorch_threshold) {
				if (cell == OIL)
					set_cell(x, y, FIRE);
			}
		}
	}

	for (int i = 0; i < radius * ember_density; i++) {
		int rx = cx + (rand() % (radius * 2 + 1)) - radius;
		int ry = cy + (rand() % (radius * 2 + 1)) - radius;
		if (rx < 0 || rx >= screen_width || ry < 0 || ry >= screen_height)
			continue;
		int dd = (rx - cx) * (rx - cx) + (ry - cy) * (ry - cy);
		if (dd > radius * radius || dd == 0)
			continue;
		if (get_cell(rx, ry) == EMPTY)
			set_cell(rx, ry, (rand() % 3 == 0) ? EMBER : FIRE);
	}
}
