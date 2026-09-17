#include "raylib_mode.h"

#include <math.h>

#include "history.h"
#include "player.h"
#include "sim.h"
#undef BLACK
#undef RED
#undef GREEN
#undef YELLOW
#undef BLUE
#undef MAGENTA
#undef WHITE
#undef GRAY
#undef PURPLE
#undef BROWN
#undef ORANGE
#undef LIME
#include <raylib.h>

static Color element_color(int cell) {
	/* Raylib colors replace the terminal ANSI palette used by the default mode. */
	switch (cell) {
	case WALL:
		return (Color){ 235, 235, 235, 255 };
	case SAND:
		return (Color){ 220, 190, 70, 255 };
	case WATER:
		return (Color){ 55, 125, 230, 255 };
	case STONE:
		return (Color){ 110, 110, 115, 255 };
	case OIL:
		return (Color){ 125, 65, 150, 255 };
	case FIRE:
		return (Color){ 240, 75, 35, 255 };
	case STEAM:
		return (Color){ 205, 215, 225, 220 };
	case LAVA:
		return (Color){ 230, 55, 25, 255 };
	case WOOD:
		return (Color){ 130, 75, 35, 255 };
	case ASH:
		return (Color){ 160, 160, 160, 255 };
	case EMBER:
		return (Color){ 245, 125, 25, 255 };
	case GUNPOWDER:
		return (Color){ 75, 75, 35, 255 };
	case ACID:
		return (Color){ 75, 235, 75, 255 };
	case LIGHTNING:
		return (Color){ 255, 245, 70, 255 };
	default:
		return (Color){ 22, 24, 30, 255 };
	}
}

typedef struct {
	Vector2 position;
	Vector2 velocity;
	bool grounded;
} RaylibPlayer;

/* The graphical player uses sub-cell coordinates; the simulation player is
 * updated with rounded coordinates only for compatibility with shared state. */
static RaylibPlayer ray_player;

static bool raylib_player_blocked(Vector2 position) {
	/* Convert the floating-point hitbox to the grid cells it overlaps. */
	int left = (int)floorf(position.x);
	int top = (int)floorf(position.y);
	int right = (int)ceilf(position.x + (float)player.width) - 1;
	int bottom = (int)ceilf(position.y + (float)player.height) - 1;
	for (int y = top; y <= bottom; y++)
		for (int x = left; x <= right; x++)
			if (element_registry[(unsigned char)get_cell(x, y)].category == CAT_SOLID)
				return true;
	return false;
}

static bool raylib_player_in_liquid(void) {
	int left = (int)floorf(ray_player.position.x);
	int top = (int)floorf(ray_player.position.y);
	int right = (int)ceilf(ray_player.position.x + (float)player.width) - 1;
	int bottom = (int)ceilf(ray_player.position.y + (float)player.height) - 1;
	for (int y = top; y <= bottom; y++)
		for (int x = left; x <= right; x++)
			if (element_registry[(unsigned char)get_cell(x, y)].category == CAT_LIQUID)
				return true;
	return false;
}

static void reset_raylib_player(void) {
	ray_player.position = (Vector2){ (float)screen_width * 0.5f, 0.0f };
	ray_player.velocity = (Vector2){ 0.0f, 0.0f };
	ray_player.grounded = false;
}

static void move_raylib_player_axis(float delta, bool horizontal) {
	/* Substep movement so fast motion cannot skip through a solid cell. */
	int steps = (int)ceilf(fabsf(delta));
	float step = steps > 0 ? delta / (float)steps : 0.0f;
	for (int i = 0; i < steps; i++) {
		Vector2 next = ray_player.position;
		if (horizontal)
			next.x += step;
		else
			next.y += step;
		if (raylib_player_blocked(next)) {
			if (!horizontal)
				ray_player.grounded = step > 0.0f;
			if (horizontal && !raylib_player_blocked((Vector2){
					next.x, next.y - 1.0f }))
				ray_player.position.y -= 1.0f;
			else if (!horizontal)
				ray_player.velocity.y = 0.0f;
			break;
		}
		ray_player.position = next;
	}
}

static void update_raylib_player(float dt, bool shift) {
	/* Raylib-only physics: input and movement are measured in cells per second. */
	if (!enable_player || paused)
		return;
	if (IsKeyPressed(KEY_R)) {
		reset_raylib_player();
		return;
	}

	int direction = (IsKeyDown(KEY_D) ? 1 : 0) - (IsKeyDown(KEY_A) ? 1 : 0);
	float speed = shift ? 24.0f : 12.0f;
	move_raylib_player_axis((float)direction * speed * dt, true);

	bool in_liquid = raylib_player_in_liquid();
	if (in_liquid) {
		ray_player.velocity.y =
			((IsKeyDown(KEY_S) ? 1.0f : 0.0f) -
		     (IsKeyDown(KEY_W) ? 1.0f : 0.0f)) * 12.0f;
	} else {
		if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W)) && ray_player.grounded)
			ray_player.velocity.y = -18.0f;
		ray_player.velocity.y += 35.0f * dt;
		if (ray_player.velocity.y > 25.0f)
			ray_player.velocity.y = 25.0f;
	}
	ray_player.grounded = false;
	move_raylib_player_axis(ray_player.velocity.y * dt, false);
	player.x = (int)ray_player.position.x;
	player.y = (int)ray_player.position.y;
}

static void handle_raylib_input(float cell_size, float offset_x, float offset_y,
                                float dt) {
	bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
	if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q))
		running = false;
	if (IsKeyPressed(KEY_P))
		paused = step_mode ? !paused : paused;
	if (step_mode && paused && IsKeyPressed(KEY_LEFT_BRACKET))
		history_step_back();
	if (step_mode && paused && IsKeyPressed(KEY_RIGHT_BRACKET))
		history_step_forward();
	if (IsKeyPressed(KEY_MINUS) && cur_radius > 1)
		cur_radius--;
	if (IsKeyPressed(KEY_EQUAL))
		cur_radius++;
	const int keys[] = { KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
		                 KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE };
	for (int i = 0; i < 9; i++) {
		if (IsKeyPressed(keys[i])) {
			if (shift && i < 5)
				current_cell = (int[]){ STONE, ASH, LAVA, EMBER, FIRE }[i];
			else
				current_cell = i + 1;
		}
	}

	int wheel = (int)GetMouseWheelMove();
	if (wheel != 0) {
		if (shift) {
			cur_radius += wheel > 0 ? 1 : -1;
			if (cur_radius < 1)
				cur_radius = 1;
		} else {
			current_cell += wheel > 0 ? 1 : -1;
			if (current_cell >= ELEMENT_COUNT)
				current_cell = 1;
			if (current_cell < 1)
				current_cell = ELEMENT_COUNT - 1;
		}
	}
	Vector2 mouse = GetMousePosition();
	/* Convert window pixels back into simulation coordinates, accounting for
	 * the centered square-cell viewport. */
	sim_mouse_x = (int)((mouse.x - offset_x) / cell_size);
	sim_mouse_y = (int)((mouse.y - offset_y) / cell_size);
	if (sim_mouse_x >= 0 && sim_mouse_x < screen_width &&
	    sim_mouse_y >= 0 && sim_mouse_y < screen_height) {
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			paint(sim_mouse_x, sim_mouse_y, cur_radius, current_cell);
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			paint(sim_mouse_x, sim_mouse_y, cur_radius, EMPTY);
	}

	update_raylib_player(dt, shift);
}

void run_raylib_mode(bool player_enabled, bool history_enabled, int history_capacity) {
	enable_player = player_enabled;
	step_mode = history_enabled;
	InitWindow(RAYLIB_DEFAULT_WIDTH, RAYLIB_DEFAULT_HEIGHT, "CimpleSand - raylib prototype");
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetExitKey(KEY_NULL);
	SetTargetFPS(target_fps > 0 ? target_fps : 0);
	if (step_mode)
		init_history(history_capacity);
	if (enable_player)
		reset_raylib_player();

	while (running && !WindowShouldClose()) {
		/* Recompute the viewport every frame so window resizing rescales cells. */
		float cell_size = (float)GetScreenWidth() / (float)screen_width;
		float height_size = (float)GetScreenHeight() / (float)screen_height;
		if (height_size < cell_size)
			cell_size = height_size;
		float offset_x = ((float)GetScreenWidth() - cell_size * screen_width) * 0.5f;
		float offset_y = ((float)GetScreenHeight() - cell_size * screen_height) * 0.5f;
		handle_raylib_input(cell_size, offset_x, offset_y, GetFrameTime());
		if (!paused) {
			simulate();
			if (step_mode)
				history_push();
		}
		BeginDrawing();
		/* Black is outside the simulation; gray is the clickable simulation area. */
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
		DrawRectangle((int)offset_x, (int)offset_y,
		              (int)(cell_size * screen_width),
		              (int)(cell_size * screen_height),
		              (Color){ 35, 38, 45, 255 });
		for (int y = 0; y < screen_height; y++) {
			for (int x = 0; x < screen_width; x++) {
				int cell = get_cell(x, y);
				if (cell != EMPTY)
					DrawRectangleRec((Rectangle){ offset_x + x * cell_size,
					                              offset_y + y * cell_size,
					                              cell_size + 0.5f, cell_size + 0.5f },
					                 element_color(cell));
			}
		}
		if (enable_player)
			DrawRectangleRec((Rectangle){
				offset_x + ray_player.position.x * cell_size,
				offset_y + ray_player.position.y * cell_size,
				player.width * cell_size, player.height * cell_size }, GREEN);
		DrawRectangle(8, 8, 460, 28, (Color){ 0, 0, 0, 175 });
		DrawText(TextFormat("FPS: %d | Cells: %d | Selected: %s | Brush: %d%s",
		                    GetFPS(), cell_count, element_registry[current_cell].name,
		                    cur_radius, paused ? " | PAUSED" : ""),
		         16, 14, 16, RAYWHITE);
		EndDrawing();
	}

	if (step_mode)
		shutdown_history();
	CloseWindow();
}
