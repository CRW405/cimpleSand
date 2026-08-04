#ifndef HISTORY_H
#define HISTORY_H

/**
 * @file history.h
 * @brief Ring-buffer history of world snapshots for pause/step mode (--step).
 */

#include "common.h"

/** @brief Default number of past frames retained when --memory is omitted. */
#define DEFAULT_HISTORY_CAPACITY 1000

/**
 * @brief Initializes the history ring buffer to hold up to @p capacity frames.
 *
 * Must be called after the grid has been allocated. Capacity values below 1
 * fall back to @ref DEFAULT_HISTORY_CAPACITY.
 * @param capacity Number of frames to retain.
 */
void init_history(int capacity);

/**
 * @brief Frees all memory held by the history buffer.
 */
void shutdown_history(void);

/**
 * @brief Appends a snapshot of the current world (grid + player) and advances
 * the ring buffer head. Meant to run once per simulated frame while unpaused.
 */
void history_push(void);

/**
 * @brief Rewinds one frame, restoring the previous snapshot into the grid.
 * @return true if there was an older frame to restore.
 */
bool history_step_back(void);

/**
 * @brief Invalidates all snapshots newer than the currently displayed one.
 *
 * Call whenever the user edits the world while viewing a rewound frame (or
 * otherwise continues forward from a rewound state), so future steps run the
 * sim forward from the changed world instead of restoring stale snapshots.
 */
void history_diverge(void);

/**
 * @brief Advances one frame: restores the next future snapshot when rewound,
 * otherwise simulates the world forward one step and snapshots it.
 * @return true if the world advanced.
 */
bool history_step_forward(void);

/**
 * @brief How many frames the displayed snapshot is behind the newest one.
 * @return 0 at the newest frame, increasing as the view rewinds.
 */
int history_frames_back(void);

/**
 * @brief Number of snapshots currently retained in the buffer.
 */
int history_total(void);

#endif
