#ifndef SIM_H
#define SIM_H

/**
 * @file sim.h
 * @brief Core simulation operations and grid mutation utilities.
 */

#include "common.h"

/** @brief Left bound of dirty/active area touched in the current frame. */
extern int min_active_x;

/** @brief Right bound of dirty/active area touched in the current frame. */
extern int max_active_x;

/** @brief Top bound of dirty/active area touched in the current frame. */
extern int min_active_y;

/** @brief Bottom bound of dirty/active area touched in the current frame. */
extern int max_active_y;

/** @brief Fast density lookup table indexed by element ID. */
extern int cell_densities[ELEMENT_COUNT];

/**
 * @brief Gets the element ID at a grid position, clamping out-of-bounds to WALL.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @return Element ID at (x, y), or WALL if coordinates are out of bounds.
 */
char get_cell(int x, int y);

/**
 * @brief Sets an element ID at a grid position, updating cell_count and active region.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param cell Element ID to set.
 */
void set_cell(int x, int y, char cell);

/**
 * @brief Swaps two cells atomically, marking both as active for next frame.
 * @param x1 First cell X.
 * @param y1 First cell Y.
 * @param x2 Second cell X.
 * @param y2 Second cell Y.
 */
void swap_cells(int x1, int y1, int x2, int y2);

/**
 * @brief Paints a filled circular brush stamp centered at a coordinate.
 * @param x_center Brush center X coordinate.
 * @param y_center Brush center Y coordinate.
 * @param radius Brush radius in cells.
 * @param cell Element ID to paint.
 */
void paint(int x_center, int y_center, int radius, char cell);

/**
 * @brief Executes one simulation step over the active simulation region.
 */
void simulate(void);

#endif
