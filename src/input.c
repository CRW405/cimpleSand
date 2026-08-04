#include "input.h"
#include "sim.h"
#include "history.h"

static bool left_mouse_held = false;
static bool right_mouse_held = false;
static bool has_mouse_position = false;

enum {
	ESCAPE_SEQUENCE_CAPACITY = 64,
	MOUSE_SEQUENCE_OFFSET = 2,
	MOUSE_X_ORIGIN_OFFSET = 1,
	MOUSE_Y_ORIGIN_OFFSET = 1,
	SIM_Y_SCALE = 2
};

/* Kitty keyboard protocol (CSI u progressive enhancement) is required, not
 * negotiated: 1 = disambiguate escape codes, 2 = report event types
 * (press/repeat/release), 8 = report all keys (incl. plain letters) as
 * escape codes, 16 = report associated UTF-8 text so shifted symbols (e.g.
 * Shift+1 -> '!') round-trip correctly. This gives real held/released state
 * for every key with no timeout heuristics; terminals without support won't
 * report movement keys at all. */
#define KITTY_FLAGS (1 | 2 | 8 | 16)

static bool held[KEY_ACTION_COUNT];
static bool held_prev[KEY_ACTION_COUNT];
static bool shift_for_key[KEY_ACTION_COUNT];

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

static void
set_key(GameKey key, bool active, bool shift) {
	held[key] = active;
	if (active) {
		shift_for_key[key] = shift;
	}
}

bool key_held(GameKey key) { return held[key]; }

bool key_just_pressed(GameKey key) { return held[key] && !held_prev[key]; }

bool sprint_held(void) {
	return (held[KEY_LEFT] && shift_for_key[KEY_LEFT]) ||
	       (held[KEY_RIGHT] && shift_for_key[KEY_RIGHT]);
}

static void snapshot_prev_keys(void) {
	for (int k = 0; k < KEY_ACTION_COUNT; k++) {
		held_prev[k] = held[k];
	}
}

static int read_char(char *out) { return read(STDIN_FILENO, out, 1); }

static bool is_escape_terminator(char c) {
	return c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'M' ||
	       c == 'm' || c == 'I' || c == 'O' || c == 'u';
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
	} else {
		if (get_cell(sim_mouse_x, sim_mouse_y) == EMPTY || cell == EMPTY)
			set_cell(sim_mouse_x, sim_mouse_y, cell);
		if (get_cell(sim_mouse_x, sim_mouse_y + 1) == EMPTY || cell == EMPTY)
			set_cell(sim_mouse_x, sim_mouse_y + 1, cell);
	}

	/* Editing a rewound frame invalidates every snapshot after it, so the
	 * next step runs forward from the changed world instead of restoring a
	 * stale future that would clobber the edit. */
	if (step_mode) {
		history_diverge();
	}
}

static void quit_simulation(void) { running = false; }

static void decrease_radius(void) {
	if (cur_radius > 1) {
		cur_radius--;
	}
}

static void increase_radius(void) { cur_radius++; }

static void toggle_pause(void) {
	if (!step_mode) {
		return;
	}
	paused = !paused;
}

static void step_back_time(void) {
	if (step_mode && paused) {
		history_step_back();
	}
}

static void step_forward_time(void) {
	if (step_mode && paused) {
		history_step_forward();
	}
}

static const KeyBinding key_registry[] = {
	{ 'q', quit_simulation },
	{ '_', decrease_radius },
	{ '-', decrease_radius },
	{ '=', increase_radius },
	{ '+', increase_radius },
	{ 'p', toggle_pause },
	{ '[', step_back_time },
	{ ']', step_forward_time }
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
	switch (key) {
	case '1':
		current_cell = WALL;
		return true;
	case '2':
		current_cell = SAND;
		return true;
	case '3':
		current_cell = WATER;
		return true;
	case '4':
		current_cell = WOOD;
		return true;
	case '5':
		current_cell = STEAM;
		return true;
	case '6':
		current_cell = OIL;
		return true;
	case '7':
		current_cell = GUNPOWDER;
		return true;
	case '8':
		current_cell = ACID;
		return true;
	case '9':
		current_cell = LIGHTNING;
		return true;
	case '!':
		current_cell = STONE;
		return true;
	case '@':
		current_cell = ASH;
		return true;
	case '#':
		current_cell = LAVA;
		return true;
	case '$':
		current_cell = EMBER;
		return true;
	case '%':
		current_cell = FIRE;
		return true;
	default:
		return false;
	}
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

		// Wheel events are 64/65 (up/down); the SGR shift modifier (0x04)
		// turns them into Shift+scroll, which adjusts the brush size instead
		// of cycling the selected material.
		if ((parsed_button & 0x40) != 0) {
			bool scroll_up = (parsed_button & 0x01) == 0;
			if ((parsed_button & 0x04) != 0) {
				if (scroll_up) {
					increase_radius();
				} else {
					decrease_radius();
				}
			} else if (scroll_up) {
				int next = current_cell + 1;
				if (next >= ELEMENT_COUNT) {
					next = 1;
				}
				current_cell = next;
			} else {
				int next = current_cell - 1;
				if (next < 1) {
					next = ELEMENT_COUNT - 1;
				}
				current_cell = next;
			}
		} else {
			// Only update click tracking if it's not a scroll event
			update_mouse_button_state(event_type, parsed_button);
		}
	}

	last_input = event_type;
}

/* Movement/jump/respawn keys get real held-state tracking (with the shift
 * modifier recorded per-key for sprint detection). Everything else is a
 * one-shot action, so it's routed through the exact same dispatch path a
 * plain byte would have used - this keeps every non-movement control (quit,
 * material selection, brush size) working identically whether or not the
 * terminal is escaping plain keys via the Kitty protocol. */
static void update_key_state(int key_code, bool shift, int event_type, int text_cp) {
	bool active = (event_type != 3);

	switch (key_code) {
	case 'a':
	case 'A':
		set_key(KEY_LEFT, active, shift);
		return;
	case 'd':
	case 'D':
		set_key(KEY_RIGHT, active, shift);
		return;
	case 'w':
	case 'W':
		set_key(KEY_UP, active, shift);
		return;
	case 's':
	case 'S':
		set_key(KEY_DOWN, active, shift);
		return;
	case ' ':
		set_key(KEY_JUMP, active, shift);
		return;
	case 'r':
	case 'R':
		set_key(KEY_RESPAWN, active, shift);
		return;
	default:
		break;
	}

	if (!active) {
		return;
	}

	char c = (char)(text_cp ? text_cp : key_code);
	apply_key_binding(c);
	last_input = c;
}

/* Parses a Kitty keyboard protocol CSI u body of the form
 * "key[:alt:base][;mods[:event]][;text[:more]]" (already stripped of the
 * leading '[' and trailing 'u' by the caller). */
static void handle_kitty_key_event(char *params) {
	char *semi1 = strchr(params, ';');
	if (semi1) {
		*semi1 = '\0';
	}
	char *key_colon = strchr(params, ':');
	if (key_colon) {
		*key_colon = '\0';
	}
	int key_code = atoi(params);

	int modifiers = 1;
	int event_type = 1;
	int text_cp = 0;

	if (semi1) {
		char *mod_part = semi1 + 1;
		char *semi2 = strchr(mod_part, ';');
		if (semi2) {
			*semi2 = '\0';
		}
		char *mod_colon = strchr(mod_part, ':');
		if (mod_colon) {
			event_type = atoi(mod_colon + 1);
			*mod_colon = '\0';
		}
		if (*mod_part) {
			modifiers = atoi(mod_part);
		}

		if (semi2) {
			char *text_part = semi2 + 1;
			char *text_colon = strchr(text_part, ':');
			if (text_colon) {
				*text_colon = '\0';
			}
			if (*text_part) {
				text_cp = atoi(text_part);
			}
		}
	}

	bool shift = ((modifiers - 1) & 1) != 0;
	update_key_state(key_code, shift, event_type, text_cp);
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

	if (seq[seq_len - 1] == 'u') {
		seq[seq_len - 1] = '\0';
		handle_kitty_key_event(seq + 1);
		return;
	}

	apply_arrow_binding(seq);
}

void init_input(void) {
	char enable[16];
	int len = snprintf(enable, sizeof(enable), "\033[>%uu", (unsigned)KITTY_FLAGS);
	if (len > 0) {
		write(STDOUT_FILENO, enable, (size_t)len);
	}
}

void shutdown_input(void) {
	const char pop[] = "\033[<u";
	write(STDOUT_FILENO, pop, sizeof(pop) - 1);
}

int isInput(void) {
	Timeval timeout = { 0, 0 };
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
}

void handle_input() {
	snapshot_prev_keys();

	while (isInput()) {
		char input_char = '\0';
		if (read_char(&input_char) != 1) {
			continue;
		}

		if (input_char == (unsigned char)27) {
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
