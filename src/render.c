#include "render.h"
#include "term_ops.h"

static char gui_buffer[256];

static char *gui(int cell_count) {
	snprintf(gui_buffer, sizeof(gui_buffer),
	         "FPS: %d/%d | Cells: %d | Last Input: %c | Mouse: %d, %d (%d) | "
	         "Selected: %d (%s) | Brush Size: %d\033[K",
	         fps, target_fps, cell_count, last_input, mouse_x, mouse_y,
	         sim_mouse_y, current_cell, element_registry[current_cell].name,
	         cur_radius);
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

			if (element_registry[top_cell].bg_color != last_top_color) {
				size_t len = element_registry[top_cell].bg_color_len;
				memcpy(frame_buffer + frame_buffer_offset, element_registry[top_cell].bg_color, len);
				frame_buffer_offset += len;
				last_top_color = element_registry[top_cell].bg_color;
			}

			if (element_registry[bottom_cell].color != last_bottom_color) {
				size_t len = element_registry[bottom_cell].color_len;
				memcpy(frame_buffer + frame_buffer_offset, element_registry[bottom_cell].color, len);
				frame_buffer_offset += len;
				last_bottom_color = element_registry[bottom_cell].color;
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
