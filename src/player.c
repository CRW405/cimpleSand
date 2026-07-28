#include "player.h"
#include "input.h"
#include "sim.h"

enum {
	SPEED_SCALE = 15,
	WALK_SPEED_UNITS = 5,
	SPRINT_MULTIPLIER = 2,
	GRAVITY_UNITS = 2,
	MAX_FALL_UNITS = 100,
	JUMP_VELOCITY_UNITS = 25,
	SWIM_SPEED_UNITS = 10,
	MAX_JUMPS = 2,
	MAX_STEP_HEIGHT = 1
};

Player player;

static bool is_solid(int x, int y) {
	return element_registry[(unsigned char)get_cell(x, y)].category == CAT_SOLID;
}

static bool is_liquid(int x, int y) {
	return element_registry[(unsigned char)get_cell(x, y)].category == CAT_LIQUID;
}

static bool aabb_blocked(int x, int y, int width, int height) {
	for (int dy = 0; dy < height; dy++) {
		for (int dx = 0; dx < width; dx++) {
			if (is_solid(x + dx, y + dy)) {
				return true;
			}
		}
	}
	return false;
}

static bool aabb_in_liquid(int x, int y, int width, int height) {
	for (int dy = 0; dy < height; dy++) {
		for (int dx = 0; dx < width; dx++) {
			if (is_liquid(x + dx, y + dy)) {
				return true;
			}
		}
	}
	return false;
}

static bool has_ground_below(void) {
	int below_y = player.y + player.height;
	for (int dx = 0; dx < player.width; dx++) {
		if (is_solid(player.x + dx, below_y)) {
			return true;
		}
	}
	return false;
}

static void clear_spawn_area(int x, int y, int width, int height) {
	for (int dy = 0; dy < height; dy++) {
		for (int dx = 0; dx < width; dx++) {
			set_cell(x + dx, y + dy, EMPTY);
		}
	}
}

/* Converts a fixed-point velocity into whole cells to move this tick,
 * keeping the sub-cell remainder in accum so slow speeds still move smoothly
 * over multiple ticks instead of rounding down to zero forever. Uses floor
 * (not truncating) division so a negative velocity always yields at least
 * one cell of movement once the magnitude crosses the scale - truncation
 * toward zero could round a small jump impulse down to zero cells on the
 * tick it's applied, leaving the player looking exactly as grounded as
 * before it jumped. */
static int step_axis(int *accum, int velocity_units) {
	*accum += velocity_units;
	int cells = (*accum >= 0) ? (*accum / SPEED_SCALE)
	                          : -((-*accum + SPEED_SCALE - 1) / SPEED_SCALE);
	*accum -= cells * SPEED_SCALE;
	return cells;
}

/* Tries to lift the player up by 1..MAX_STEP_HEIGHT cells so a short bump
 * doesn't block horizontal movement outright - the smallest lift that clears
 * it wins, so a 1-cell step doesn't overshoot onto a 3-cell-tall ledge. */
static bool try_step_up(int nx) {
	for (int lift = 1; lift <= MAX_STEP_HEIGHT; lift++) {
		if (!aabb_blocked(nx, player.y - lift, player.width, player.height)) {
			player.y -= lift;
			return true;
		}
	}
	return false;
}

static void move_horizontal(int cells) {
	int step = (cells > 0) ? 1 : -1;
	int n = (cells > 0) ? cells : -cells;
	for (int i = 0; i < n; i++) {
		int nx = player.x + step;
		if (aabb_blocked(nx, player.y, player.width, player.height) && !try_step_up(nx)) {
			break;
		}
		player.x = nx;
	}
}

static void move_vertical(int cells) {
	int step = (cells > 0) ? 1 : -1;
	int n = (cells > 0) ? cells : -cells;
	for (int i = 0; i < n; i++) {
		int ny = player.y + step;
		if (aabb_blocked(player.x, ny, player.width, player.height)) {
			player.vy = 0;
			break;
		}
		player.y = ny;
	}
}

static void reset_player_state(void) {
	player.x = screen_width / 2;
	player.y = 0;
	player.vy = 0;
	player.accum_x = 0;
	player.accum_y = 0;
	player.jump_count = MAX_JUMPS;
	player.grounded = false;
	player.in_liquid = false;
	player.sprinting = false;
	clear_spawn_area(player.x, player.y, player.width, player.height);
}

void init_player(void) {
	player.width = 2;
	player.height = 5;
	player.color = GREEN;
	player.bg_color = BG_GREEN;
	player.speed = WALK_SPEED_UNITS;
	player.jump_height = JUMP_VELOCITY_UNITS;
	reset_player_state();
}

void respawn_player(void) { reset_player_state(); }

void update_player(void) {
	if (key_just_pressed(KEY_RESPAWN)) {
		respawn_player();
		return;
	}

	player.in_liquid = aabb_in_liquid(player.x, player.y, player.width, player.height);
	player.sprinting = sprint_held();

	/* Refill from *last* tick's resting state before this tick's jump can
	 * consume a charge - refilling from a post-move recheck further down
	 * would immediately undo the decrement below whenever the jump's own
	 * first-tick movement isn't enough to clear the ground-probe cell. */
	if (!player.in_liquid && has_ground_below()) {
		player.jump_count = MAX_JUMPS;
	}

	int dx_dir = (key_held(KEY_RIGHT) ? 1 : 0) - (key_held(KEY_LEFT) ? 1 : 0);
	int h_units = player.speed * (player.sprinting ? SPRINT_MULTIPLIER : 1) * dx_dir;
	int move_x = step_axis(&player.accum_x, h_units);
	if (move_x != 0) {
		move_horizontal(move_x);
	}

	bool jump_edge = key_just_pressed(KEY_JUMP) ||
	                 (!player.in_liquid && key_just_pressed(KEY_UP));

	if (player.in_liquid) {
		int swim_dir = (key_held(KEY_UP) ? -1 : 0) + (key_held(KEY_DOWN) ? 1 : 0);
		player.vy = swim_dir * SWIM_SPEED_UNITS;
	} else if (jump_edge && player.jump_count > 0) {
		player.vy = -player.jump_height;
		player.jump_count--;
	} else {
		player.vy += GRAVITY_UNITS;
		if (player.vy > MAX_FALL_UNITS) {
			player.vy = MAX_FALL_UNITS;
		}
	}

	int move_y = step_axis(&player.accum_y, player.vy);
	if (move_y != 0) {
		move_vertical(move_y);
	}

	player.grounded = !player.in_liquid && has_ground_below();
	if (player.grounded && player.vy > 0) {
		player.vy = 0;
	}
}

bool player_occupies(int x, int y) {
	return x >= player.x && x < player.x + player.width &&
	       y >= player.y && y < player.y + player.height;
}
