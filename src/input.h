#ifndef INPUT_H
#define INPUT_H

/**
 * @file input.h
 * @brief Input polling and input-to-action handling for the simulation.
 */

#include "common.h"

/**
 * @brief Checks whether stdin currently has pending input.
 * @return Non-zero when input is available; zero when no input is pending.
 */
int isInput(void);

/**
 * @brief Consumes and processes all pending keyboard/mouse input.
 *
 * Updates global interaction state (selected element, cursor/brush behavior,
 * and quit signal) and applies paint/erase actions for held mouse buttons.
 */
void handle_input(void);

#endif
