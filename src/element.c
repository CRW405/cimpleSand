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
