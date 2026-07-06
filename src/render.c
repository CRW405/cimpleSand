#include "render.h"
#include "term_ops.h"

static char gui_buffer[256];
int frame_buffer_offset;

char *get_cell_color(int cell) {
	switch (cell) {
	case EMPTY:
		return BLACK;
	case WALL:
		return WHITE;
	case SAND:
		return YELLOW;
	case WATER:
		return BLUE;
	default:
		return RED;
	}
}

char *get_cell_bg_color(int cell) {
	switch (cell) {
	case EMPTY:
		return BG_BLACK;
	case WALL:
		return BG_WHITE;
	case SAND:
		return BG_YELLOW;
	case WATER:
		return BG_BLUE;
	default:
		return BG_RED;
	}
}

char *random_color() {
	int random_int = rand() % 8;
	switch (random_int) {
	case 0:
		return BLACK;
	case 1:
		return RED;
	case 2:
		return GREEN;
	case 3:
		return YELLOW;
	case 4:
		return BLUE;
	case 5:
		return MAGENTA;
	case 6:
		return CYAN;
	case 7:
		return WHITE;
	}
	return WHITE;
}

char *random_bg_color() {
	int random_int = rand() % 8;
	switch (random_int) {
	case 0:
		return BG_BLACK;
	case 1:
		return BG_RED;
	case 2:
		return BG_GREEN;
	case 3:
		return BG_YELLOW;
	case 4:
		return BG_BLUE;
	case 5:
		return BG_MAGENTA;
	case 6:
		return BG_CYAN;
	case 7:
		return BG_WHITE;
	}
	return BG_WHITE;
}

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
			char *top_color = get_cell_bg_color(top_cell);

			unsigned char bottom_cell = grid[((y + 1) * screen_width) + x];
			char *bottom_color = get_cell_color(bottom_cell);

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
