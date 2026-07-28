#ifndef INPUT_H
#define INPUT_H

/**
 * @file input.h
 * @brief Input polling and input-to-action handling for the simulation.
 */

#include "common.h"

/**
 * @brief Physical keys tracked as continuous "held" state for player movement.
 *
 * Resolved from real press/repeat/release events reported by the Kitty
 * keyboard protocol (see @ref init_input) - terminals without support for it
 * won't report these keys at all.
 */
typedef enum {
	KEY_LEFT,   /**< 'a' / 'A' */
	KEY_RIGHT,  /**< 'd' / 'D' */
	KEY_UP,     /**< 'w' / 'W' - jump when grounded/airborne, swim up in liquid */
	KEY_DOWN,   /**< 's' / 'S' - swim down in liquid */
	KEY_JUMP,   /**< Space */
	KEY_RESPAWN, /**< 'r' / 'R' */
	KEY_ACTION_COUNT
} GameKey;

/**
 * @brief Checks whether a game key is currently considered held down.
 * @param key Key to query.
 * @return true while the key is held (real or approximated).
 */
bool key_held(GameKey key);

/**
 * @brief Checks whether a game key transitioned from not-held to held this frame.
 * @param key Key to query.
 * @return true only on the frame the key was first pressed.
 */
bool key_just_pressed(GameKey key);

/**
 * @brief Checks whether Shift is currently held alongside the left/right movement keys.
 * @return true when sprinting should be active.
 */
bool sprint_held(void);

/**
 * @brief Enables the Kitty keyboard protocol (real key press/release events)
 * unconditionally - required for player movement input, not negotiated.
 *
 * Must be called after raw mode is enabled and before the main loop starts.
 */
void init_input(void);

/**
 * @brief Restores the terminal's keyboard protocol state changed by @ref init_input.
 */
void shutdown_input(void);

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
 * Also resolves held/just-pressed state for the player's movement keys.
 */
void handle_input(void);

#endif
