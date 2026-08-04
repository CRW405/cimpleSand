#include "history.h"
#include "player.h"
#include "sim.h"

typedef struct {
	unsigned char *grid;
	Player player;
	bool has_player;
} HistoryFrame;

static HistoryFrame *frames = NULL;
static int capacity = 0;
static int head = -1;
static int count = 0;
static int pos = 0;

void init_history(int cap) {
	if (cap < 1) {
		cap = DEFAULT_HISTORY_CAPACITY;
	}
	capacity = cap;

	frames = calloc((size_t)capacity, sizeof(HistoryFrame));
	if (!frames) {
		fprintf(stderr, "Failed to allocate history buffer.\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < capacity; i++) {
		frames[i].grid = malloc((size_t)grid_size);
		if (!frames[i].grid) {
			fprintf(stderr, "Failed to allocate history grid.\n");
			exit(EXIT_FAILURE);
		}
	}

	head = -1;
	count = 0;
	pos = 0;
}

void shutdown_history(void) {
	if (!frames) {
		return;
	}
	for (int i = 0; i < capacity; i++) {
		free(frames[i].grid);
	}
	free(frames);
	frames = NULL;
	capacity = 0;
	head = -1;
	count = 0;
	pos = 0;
}

static int oldest_index(void) {
	if (count == 0) {
		return 0;
	}
	return (head - count + 1 + capacity) % capacity;
}

/* Drops every snapshot newer than the current view, making it the newest.
 * Stale "future" frames must never be restored once the world has moved on
 * from them, so they're removed from the valid range immediately. */
static void truncate_at_pos(void) {
	if (count == 0 || pos == head) {
		return;
	}
	count = (pos - oldest_index() + capacity) % capacity + 1;
	head = pos;
}

void history_diverge(void) { truncate_at_pos(); }

void history_push(void) {
	truncate_at_pos();
	head = (head + 1) % capacity;
	memcpy(frames[head].grid, grid, (size_t)grid_size);

	if (enable_player) {
		frames[head].player = player;
		frames[head].has_player = true;
	} else {
		frames[head].has_player = false;
	}

	if (count < capacity) {
		count++;
	}
	pos = head;
}

static void restore_at(int idx) {
	memcpy(grid, frames[idx].grid, (size_t)grid_size);
	if (frames[idx].has_player) {
		player = frames[idx].player;
	}

	int n = 0;
	for (int i = 0; i < grid_size; i++) {
		if (grid[i] & ACTIVE_MASK) {
			n++;
		}
	}
	cell_count = n;

	reset_active_region();
}

bool history_step_back(void) {
	if (count == 0 || pos == oldest_index()) {
		return false;
	}
	pos = (pos - 1 + capacity) % capacity;
	restore_at(pos);
	return true;
}

bool history_step_forward(void) {
	if (count == 0 || pos == head) {
		simulate();
		history_push();
		return true;
	}
	pos = (pos + 1) % capacity;
	restore_at(pos);
	return true;
}

int history_frames_back(void) {
	if (count == 0 || pos == head) {
		return 0;
	}
	return (head - pos + capacity) % capacity;
}

int history_total(void) { return count; }
