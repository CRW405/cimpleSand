#include "input.h"

static bool left_mouse_held = false;
static bool right_mouse_held = false;
static bool has_mouse_position = false;

extern void set_cell(int x, int y, char cell);
extern void paint(int x_center, int y_center, int radius, char cell);

enum {
	ESCAPE_SEQUENCE_CAPACITY = 64,
	MOUSE_SEQUENCE_OFFSET = 2,
	MOUSE_X_ORIGIN_OFFSET = 1,
	MOUSE_Y_ORIGIN_OFFSET = 1,
	SIM_Y_SCALE = 2
};

typedef void (*KeyAction)(void);

typedef struct {
	char key;
	KeyAction action;
} KeyBinding;

typedef struct {
	char escape_final;
	char mapped_key;
} EscapeBinding;

typedef struct {
	char event_type;
	int button_id;
	bool *target;
	bool value;
} MouseStateBinding;

static int read_char(char *out) { return read(STDIN_FILENO, out, 1); }

static bool is_escape_terminator(char c) {
	return c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'M' ||
	       c == 'm' || c == 'I' || c == 'O';
}

static int read_escape_sequence(char *seq, size_t seq_size) {
	size_t i = 0;
	while (isInput() && i < seq_size - 1) {
		if (read_char(&seq[i]) != 1) {
			break;
		}
		if (is_escape_terminator(seq[i])) {
			i++;
			break;
		}
		i++;
	}
	seq[i] = '\0';
	return (int)i;
}

static void paint_at_cursor(char cell) {
	if (cur_radius > 1) {
		paint(sim_mouse_x, sim_mouse_y, cur_radius, cell);
		return;
	}
	set_cell(sim_mouse_x, sim_mouse_y, cell);
	set_cell(sim_mouse_x, sim_mouse_y + 1, cell);
}

static void quit_simulation(void) { running = false; }

static void decrease_radius(void) {
	if (cur_radius > 1) {
		cur_radius--;
	}
}

static void increase_radius(void) { cur_radius++; }

static const KeyBinding key_registry[] = {
	{ 'q', quit_simulation },
	{ '_', decrease_radius },
	{ '-', decrease_radius },
	{ '=', increase_radius },
	{ '+', increase_radius }
};

static const EscapeBinding arrow_registry[] = {
	{ 'A', '^' },
	{ 'B', 'V' },
	{ 'C', '>' },
	{ 'D', '<' }
};

static const MouseStateBinding mouse_state_registry[] = {
	{ 'M', 0, &left_mouse_held,  true  },
	{ 'M', 2, &right_mouse_held, true  },
	{ 'm', 0, &left_mouse_held,  false },
	{ 'm', 3, &left_mouse_held,  false },
	{ 'm', 2, &right_mouse_held, false },
	{ 'm', 3, &right_mouse_held, false }
};

static bool apply_element_binding(char key) {
	if (key < '1' || key > ('0' + STONE)) {
		return false;
	}
	current_cell = key - '0';
	return true;
}

static void apply_key_binding(char key) {
	if (apply_element_binding(key)) {
		return;
	}

	for (size_t i = 0; i < sizeof(key_registry) / sizeof(key_registry[0]); i++) {
		if (key_registry[i].key == key) {
			key_registry[i].action();
			return;
		}
	}
}

static void apply_arrow_binding(const char *seq) {
	for (size_t i = 0; i < sizeof(arrow_registry) / sizeof(arrow_registry[0]); i++) {
		if (arrow_registry[i].escape_final == seq[1]) {
			last_input = arrow_registry[i].mapped_key;
			return;
		}
	}
}

static void update_mouse_button_state(char event_type, int mouse_button) {
	int button_id = mouse_button & 0b11;
	for (size_t i = 0;
	     i < sizeof(mouse_state_registry) / sizeof(mouse_state_registry[0]); i++) {
		const MouseStateBinding *binding = &mouse_state_registry[i];
		if (binding->event_type == event_type && binding->button_id == button_id) {
			*binding->target = binding->value;
		}
	}
}

static void set_mouse_position(int x, int y) {
	mouse_x = x;
	mouse_y = y;
	has_mouse_position = true;
	sim_mouse_x = mouse_x - MOUSE_X_ORIGIN_OFFSET;
	sim_mouse_y = (mouse_y - MOUSE_Y_ORIGIN_OFFSET) * SIM_Y_SCALE;
}

static void handle_mouse_event(const char *seq, int seq_len) {
	char event_type = seq[seq_len - 1];
	int parsed_button = 0;
	int parsed_x = 0;
	int parsed_y = 0;

	if (sscanf(seq + MOUSE_SEQUENCE_OFFSET, "%d;%d;%d",
	           &parsed_button, &parsed_x, &parsed_y) == 3) {
		set_mouse_position(parsed_x, parsed_y);
		update_mouse_button_state(event_type, parsed_button);
	}

	last_input = event_type;
}

static void handle_escape_input(void) {
	char seq[ESCAPE_SEQUENCE_CAPACITY];
	int seq_len = read_escape_sequence(seq, sizeof(seq));
	if (seq_len < 2 || seq[0] != '[') {
		return;
	}

	if (seq[1] == '<') {
		handle_mouse_event(seq, seq_len);
		return;
	}

	apply_arrow_binding(seq);
}

int isInput(void) {
	Timeval timeout = { 0, 0 };
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
}

void handle_input() {
	while (isInput()) {
		char input_char = '\0';
		if (read_char(&input_char) != 1) {
			continue;
		}

		if (input_char == '\e') {
			handle_escape_input();
			continue;
		}

		apply_key_binding(input_char);
		last_input = input_char;
	}

	if (has_mouse_position) {
		if (left_mouse_held) {
			paint_at_cursor(current_cell);
		}
		if (right_mouse_held) {
			paint_at_cursor(EMPTY);
		}
	}
}
