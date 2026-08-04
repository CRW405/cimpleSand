#include "element.h"
#include "element_utils.h"
#include "sim.h"

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

	try_liquid_flow(x, y, OIL, 5);
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
		case GUNPOWDER:
			trigger_explosion(nx, ny, 0);
			return;
		}
	}

	// 10% chance to extinguish fire each frame
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

	// 0.2% chance to condense into water, 0.8% chance to dissipate
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
		case GUNPOWDER:
			trigger_explosion(nx, ny, 0);
			return;
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
		if (neighbor == GUNPOWDER) {
			trigger_explosion(x + dx[i], y + dy[i], 0);
			return;
		}
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

void sim_gunpowder(int x, int y) {
	int dx[] = { 0, 0, -1, 1 };
	int dy[] = { -1, 1, 0, 0 };

	for (int i = 0; i < 4; i++) {
		char neighbor = get_cell(x + dx[i], y + dy[i]);
		if (neighbor == FIRE || neighbor == LAVA || neighbor == EMBER) {
			trigger_explosion(x, y, 0);
			return;
		}
	}

	if (try_fall_down(x, y, GUNPOWDER))
		return;
	try_fall_diagonal(x, y, GUNPOWDER);
}

static bool try_acid_dissolve(int x, int y, int tx, int ty, int acid_power) {
	unsigned char target = get_cell(tx, ty);
	if (target == EMPTY || target == ACID || target == WALL)
		return false;

	int target_density = cell_densities[target];
	if (target_density <= 0)
		return false;

	if (rand() % (acid_power + target_density) >= acid_power)
		return false;

	bool consumed = rand() % (acid_power + target_density) < target_density;

	set_cell(tx, ty, EMPTY);
	if (consumed) {
		set_cell(x, y, EMPTY);
	} else {
		swap_cells(x, y, tx, ty);
	}
	return true;
}

void sim_acid(int x, int y) {
	int acid_power = 25;

	if (try_fall_down(x, y, ACID))
		return;
	if (try_fall_diagonal(x, y, ACID))
		return;

	int dx[] = { 0, 0, -1, 1 };
	int dy[] = { -1, 1, 0, 0 };

	for (int i = 0; i < 4; i++) {
		if (try_acid_dissolve(x, y, x + dx[i], y + dy[i], acid_power))
			return;
	}

	try_liquid_flow(x, y, ACID, 10);
}

static unsigned bolt_hash(int x, int y) {
	unsigned h = (unsigned)(x * 0x9E3779B1u) ^ (unsigned)(y * 0x85EBCA6Bu);
	h ^= h >> 13;
	h *= 0xC2B2AE35u;
	h ^= h >> 16;
	return h;
}

static void lightning_strike(int x, int y) {
	unsigned char hit = get_cell(x, y);

	switch (hit) {
	case WOOD:
		set_cell(x, y, EMBER);
		break;
	case OIL:
		set_cell(x, y, FIRE);
		break;
	case GUNPOWDER:
		trigger_explosion(x, y, 0);
		return;
	case WATER:
		set_cell(x, y, STEAM);
		break;
	}

	int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	int dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };

	for (int i = 0; i < 8; i++) {
		unsigned char c = get_cell(x + dx[i], y + dy[i]);
		switch (c) {
		case WOOD:
			set_cell(x + dx[i], y + dy[i], EMBER);
			break;
		case OIL:
			set_cell(x + dx[i], y + dy[i], FIRE);
			break;
		case GUNPOWDER:
			trigger_explosion(x + dx[i], y + dy[i], 0);
			break;
		case WATER:
			set_cell(x + dx[i], y + dy[i], STEAM);
			break;
		}
	}
}

void sim_lightning(int x, int y) {
	int lightning_speed = 1;

	if (y + 1 >= screen_height) {
		lightning_strike(x, y);
		set_cell(x, y, EMPTY);
		return;
	}

	int cur_x = x;
	int cur_y = y;

	for (int i = 0; i < lightning_speed; i++) {
		int next_y = cur_y + 1;
		if (next_y >= screen_height) {
			lightning_strike(cur_x, cur_y);
			set_cell(x, y, EMPTY);
			return;
		}

		int jitter = 0;
		unsigned h = bolt_hash(cur_x, cur_y);
		jitter = (int)(h % 3) - 1;
		if (h % 4 == 0)
			jitter = (h & 1u) ? 2 : -2;

		int next_x = cur_x + jitter;
		if (next_x < 0)
			next_x = 0;
		if (next_x >= screen_width)
			next_x = screen_width - 1;

		unsigned char cell = get_cell(next_x, next_y);
		if (cell != EMPTY && cell != LIGHTNING) {
			lightning_strike(next_x, next_y);
			set_cell(x, y, EMPTY);
			return;
		}

		cur_x = next_x;
		cur_y = next_y;
	}

	set_cell(x, y, EMPTY);
	set_cell(cur_x, cur_y, LIGHTNING);
}
