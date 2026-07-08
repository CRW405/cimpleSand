#include "render.h"
#include "term_ops.h"

static char gui_buffer[256];
int frame_buffer_offset;

char *gui() {
	snprintf(gui_buffer, sizeof(gui_buffer),
	         "FPS: %d | Cells: %d | Last Input: %c | Mouse: %d, %d (%d) | "
	         "Selected: %d | Brush Size: %d",
	         fps, cell_count, last_input, mouse_x, mouse_y, sim_mouse_y,
	         current_cell, cur_radius);
	return gui_buffer;
}

void render() {
	term_op(1, false, CUR_TO_TOP);
	frame_buffer_offset = 0;

	for (int y = 0; y < screen_height; y += 2) {
		for (int x = 0; x < screen_width; x++) {
			unsigned char top_cell = grid[((y)*screen_width) + x];
			const char *top_color = element_registry[top_cell].bg_color;

			unsigned char bottom_cell = grid[((y + 1) * screen_width) + x];
			const char *bottom_color = element_registry[bottom_cell].color;

			frame_buffer_offset +=
			    snprintf(frame_buffer + frame_buffer_offset,
			             frame_buffer_size - frame_buffer_offset, "%s%s▄",
			             top_color, bottom_color);
		}
		frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
		                                frame_buffer_size - frame_buffer_offset,
		                                "%s\n", RESET_STYLE);
	}
	frame_buffer_offset +=
	    snprintf(frame_buffer + frame_buffer_offset,
	             frame_buffer_size - frame_buffer_offset, "%s", gui());
	printf("%s", frame_buffer);

	fflush(stdout);
}
