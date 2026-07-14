#ifndef RENDER_H
#define RENDER_H

/**
 * @file render.h
 * @brief Frame rendering for terminal output.
 */

#include "common.h"

/**
 * @brief Renders the current simulation frame into the terminal.
 *
 * Builds the frame in @ref frame_buffer and writes it to stdout, including
 * both the simulation view and the status line.
 */
void render(void);

#endif
