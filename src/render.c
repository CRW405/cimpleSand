#include "render.h"
#include "term_ops.h"

static char gui_buffer[256];

static char *gui(int cell_count) {
	snprintf(gui_buffer, sizeof(gui_buffer),
	         "FPS: %d/%d | Cells: %d | Last Input: %c | Mouse: %d, %d (%d) | "
	         "Selected: %d (%s) | Brush Size: %d\e[K",
	         fps, target_fps, cell_count, last_input, mouse_x, mouse_y,
	         sim_mouse_y, current_cell, element_registry[current_cell].name,
	         cur_radius);
	return gui_buffer;
}

void render() {
	int frame_buffer_offset = 0;
	int cell_count = 0;

	// Direct string copy for cursor reset rather than variable argument parsing
	memcpy(frame_buffer + frame_buffer_offset, CUR_TO_TOP, sizeof(CUR_TO_TOP) - 1);
	frame_buffer_offset += sizeof(CUR_TO_TOP) - 1;

	for (int y = 0; y < screen_height; y += 2) {
		int top_row_idx = y * screen_width;
		int bot_row_idx = (y + 1) * screen_width;

		for (int x = 0; x < screen_width; x++) {
			unsigned char top_cell = grid[top_row_idx + x];
			if (top_cell != EMPTY)
				cell_count++;
			const char *top_color = element_registry[top_cell].bg_color;

			unsigned char bottom_cell = grid[bot_row_idx + x];
			if (bottom_cell != EMPTY)
				cell_count++;
			const char *bottom_color = element_registry[bottom_cell].color;

			// Optimizing the Hot Path: Fast inline block copies instead of snprintf
			size_t len;

			len = strlen(top_color);
			memcpy(frame_buffer + frame_buffer_offset, top_color, len);
			frame_buffer_offset += len;

			len = strlen(bottom_color);
			memcpy(frame_buffer + frame_buffer_offset, bottom_color, len);
			frame_buffer_offset += len;

			// Copy the literal block "▄" (3 bytes in UTF-8: 0xE2, 0x96, 0x84)
			memcpy(frame_buffer + frame_buffer_offset, "▄", 3);
			frame_buffer_offset += 3;
		}

		size_t r_len = sizeof(RESET_STYLE) - 1;
		memcpy(frame_buffer + frame_buffer_offset, RESET_STYLE, r_len);
		frame_buffer_offset += r_len;
		frame_buffer[frame_buffer_offset++] = '\n';
	}

	// Handle GUI at the end safely using snprintf
	frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
	                                frame_buffer_size - frame_buffer_offset,
	                                "%s", gui(cell_count));

	if (frame_buffer_offset >= frame_buffer_size) {
		frame_buffer[frame_buffer_size - 1] = '\0';
	} else {
		frame_buffer[frame_buffer_offset] = '\0';
	}

	// Direct write to stdout fd beats standard printf parsing overhead
	write(STDOUT_FILENO, frame_buffer, frame_buffer_offset);
}
