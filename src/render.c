#include "render.h"
#include "player.h"
#include "term_ops.h"
#include "history.h"

static char gui_buffer[256];

static char *gui(int cell_count) {
	if (enable_player) {
		snprintf(gui_buffer, sizeof(gui_buffer),
		         "FPS: %d/%d | Cells: %d  | Mouse: %d, %d (%d) | "
		         "Selected: %d (%s) | Brush Size: %d | Player: (%d,%d) Jumps: %d%s\033[K",
		         fps, target_fps, cell_count, mouse_x, mouse_y,
		         sim_mouse_y, current_cell, element_registry[current_cell].name,
		         cur_radius, player.x, player.y, player.jump_count,
		         player.in_liquid ? " Swimming" : (player.grounded ? " Grounded" : " Airborne"));
	} else {
		snprintf(gui_buffer, sizeof(gui_buffer),
		         "FPS: %d/%d | Cells: %d | Mouse: %d, %d (%d) | "
		         "Selected: %d (%s) | Brush Size: %d\033[K",
		         fps, target_fps, cell_count, mouse_x, mouse_y, sim_mouse_y,
		         current_cell, element_registry[current_cell].name, cur_radius);
	}

	if (step_mode) {
		int len = (int)strlen(gui_buffer);
		int frames_back = history_frames_back();
		snprintf(gui_buffer + len, sizeof(gui_buffer) - (size_t)len,
		         " | Frame: %d/%d%s%s",
		         history_total() > 0 ? history_total() - frames_back : 0,
		         history_total(),
		         frames_back > 0 ? " (rewound) " : " ",
		         paused ? "PAUSED" : "RUNNING");
	}

	return gui_buffer;
}

void render() {
	int frame_buffer_offset = 0;

	memcpy(frame_buffer + frame_buffer_offset, CUR_TO_TOP, sizeof(CUR_TO_TOP) - 1);
	frame_buffer_offset += sizeof(CUR_TO_TOP) - 1;

	memcpy(frame_buffer + frame_buffer_offset, RESET_STYLE, sizeof(RESET_STYLE) - 1);
	frame_buffer_offset += sizeof(RESET_STYLE) - 1;

	const char *last_top_color = NULL;
	const char *last_bottom_color = NULL;

	for (int y = 0; y < screen_height; y += 2) {
		int top_row_index = y * screen_width;
		int bot_row_index = (y + 1) * screen_width;

		for (int x = 0; x < screen_width; x++) {
			unsigned char top_cell = grid[top_row_index + x] & ACTIVE_MASK;

			unsigned char bottom_cell = grid[bot_row_index + x] & ACTIVE_MASK;

			const char *top_bg = element_registry[top_cell].bg_color;
			size_t top_bg_len = element_registry[top_cell].bg_color_len;
			if (player_occupies(x, y)) {
				top_bg = player.bg_color;
				top_bg_len = strlen(player.bg_color);
			}

			const char *bottom_fg = element_registry[bottom_cell].color;
			size_t bottom_fg_len = element_registry[bottom_cell].color_len;
			if (player_occupies(x, y + 1)) {
				bottom_fg = player.color;
				bottom_fg_len = strlen(player.color);
			}

			if (top_bg != last_top_color) {
				memcpy(frame_buffer + frame_buffer_offset, top_bg, top_bg_len);
				frame_buffer_offset += top_bg_len;
				last_top_color = top_bg;
			}

			if (bottom_fg != last_bottom_color) {
				memcpy(frame_buffer + frame_buffer_offset, bottom_fg, bottom_fg_len);
				frame_buffer_offset += bottom_fg_len;
				last_bottom_color = bottom_fg;
			}

			memcpy(frame_buffer + frame_buffer_offset, "▄", 3);
			frame_buffer_offset += 3;
		}

		size_t r_len = sizeof(RESET_STYLE) - 1;
		memcpy(frame_buffer + frame_buffer_offset, RESET_STYLE, r_len);
		frame_buffer_offset += r_len;
		frame_buffer[frame_buffer_offset++] = '\n';

		last_top_color = NULL;
		last_bottom_color = NULL;
	}

	frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
	                                frame_buffer_size - frame_buffer_offset,
	                                "%s", gui(cell_count));

	if (frame_buffer_offset >= frame_buffer_size) {
		frame_buffer[frame_buffer_size - 1] = '\0';
	} else {
		frame_buffer[frame_buffer_offset] = '\0';
	}

	write(STDOUT_FILENO, frame_buffer, frame_buffer_offset);
}
