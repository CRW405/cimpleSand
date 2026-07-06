#include "input.h"

static char input_char;
static int mouse_button;
static char mouse_event_type;

extern void set_cell(int x, int y, char cell);
extern void paint(int x_center, int y_center, int radius, char cell);

int isInput() {
	Timeval timeout = {0, 0};
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
}

void handle_input() {
	while (isInput()) {
		read(STDIN_FILENO, &input_char, 1);

		if (input_char == '\e') {
			int seq_size = 64;
			char seq[seq_size];
			int i = 0;

			while (isInput() && i < seq_size - 1) {
				read(STDIN_FILENO, &seq[i], 1);
				if (seq[i] == 'A' || seq[i] == 'B' || seq[i] == 'C' ||
				    seq[i] == 'D' || seq[i] == 'M' || seq[i] == 'm' ||
				    seq[i] == 'I' || seq[i] == 'O') {
					i++;
					break;
				}
				i++;
			}
			seq[i] = '\0';

			if (seq[0] == '[' && seq[1] != '<') {
				switch (seq[1]) {
				case 'A':
					last_input = '^';
					break;
				case 'B':
					last_input = 'V';
					break;
				case 'C':
					last_input = '>';
					break;
				case 'D':
					last_input = '<';
					break;
				}
			} else if (seq[0] == '[' && seq[1] == '<') {
				mouse_event_type = seq[i - 1];
				int tmpx = 0, tmpy = 0, tmpbtn = 0;
				if (sscanf(seq + 2, "%d;%d;%d", &tmpbtn, &tmpx, &tmpy) == 3) {
					mouse_button = tmpbtn;
					mouse_x = tmpx;
					mouse_y = tmpy;

					sim_mouse_x = mouse_x - 1;
					sim_mouse_y = ((mouse_y - 1) * 2);

					if (mouse_event_type == 'M') {
						if (mouse_button == 0 || mouse_button == 32) {
							if (cur_radius > 1) {
								paint(sim_mouse_x, sim_mouse_y, cur_radius,
								      current_cell);
							} else {
								set_cell(sim_mouse_x, sim_mouse_y,
								         current_cell);
								set_cell(sim_mouse_x, sim_mouse_y + 1,
								         current_cell);
							}
						}
						if (mouse_button == 2 || mouse_button == 34) {
							if (cur_radius > 1) {
								paint(sim_mouse_x, sim_mouse_y, cur_radius,
								      EMPTY);
							} else {
								set_cell(sim_mouse_x, sim_mouse_y, EMPTY);
								set_cell(sim_mouse_x, sim_mouse_y + 1, EMPTY);
							}
						}
					}
				}
				last_input = mouse_event_type;
			}
		} else {
			switch (input_char) {
			case 'q':
				running = false;
				break;
			case '1':
				current_cell = WALL;
				break;
			case '2':
				current_cell = SAND;
				break;
			case '3':
				current_cell = WATER;
				break;
			case '_':
			case '-':
				if (cur_radius > 1)
					cur_radius--;
				break;
			case '=':
			case '+':
				cur_radius++;
				break;
			}
			last_input = input_char;
		}
	}
}
