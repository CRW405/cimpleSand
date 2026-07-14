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
 * @brief Returns the cell type at a grid coordinate.
 * @param x Grid X coordinate.
 * @param y Grid Y coordinate.
 * @return Element ID at (x, y), or WALL when out of bounds.
 */
char getCell(int x, int y);

/**
 * @brief Sets one grid cell to a new element type.
 * @param x Grid X coordinate.
 * @param y Grid Y coordinate.
 * @param cell New element ID.
 *
 * Out-of-bounds coordinates are ignored.
 */
void set_cell(int x, int y, char cell);

/**
 * @brief Paints a filled circular brush stamp centered at a coordinate.
 * @param x_center Brush center X coordinate.
 * @param y_center Brush center Y coordinate.
 * @param radius Brush radius in cells.
 * @param cell Element ID to paint.
 */
void paint(int x_center, int y_center, int radius, char cell);

/**
 * @brief Swaps two cells and marks both as updated for this frame.
 * @param x1 First cell X coordinate.
 * @param y1 First cell Y coordinate.
 * @param x2 Second cell X coordinate.
 * @param y2 Second cell Y coordinate.
 */
void swap_cells(int x1, int y1, int x2, int y2);

/**
 * @brief Executes one simulation step over the active simulation region.
 */
void simulate(void);

#endif
