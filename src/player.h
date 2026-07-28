#ifndef PLAYER_H
#define PLAYER_H

/**
 * @file player.h
 * @brief Player character state, physics, and world interaction.
 */

#include "common.h"

typedef struct {
	int x, y;             /**< Top-left grid cell position. */
	int width, height;    /**< Hitbox size in cells. */
	const char *color;    /**< ANSI foreground (bottom half-block glyph). */
	const char *bg_color; /**< ANSI background (top half-block glyph). */
	int jump_count;       /**< Remaining jumps before landing refills it (double/triple jump). */
	int speed;            /**< Horizontal speed, in fixed-point units/tick. */
	int jump_height;      /**< Initial upward velocity on jump, in fixed-point units/tick. */
	int vy;                /**< Current vertical velocity, fixed-point units/tick. */
	int accum_x, accum_y; /**< Fixed-point sub-cell movement remainders. */
	bool grounded;        /**< Standing on a solid cell this frame. */
	bool in_liquid;        /**< Overlapping a liquid cell (swimming) this frame. */
	bool sprinting;        /**< Shift+direction held this frame. */
} Player;

/** @brief The single player character instance. */
extern Player player;

/**
 * @brief Initializes the player at the top-center of the screen.
 *
 * Must be called after the grid has been allocated (screen_width/height set).
 */
void init_player(void);

/**
 * @brief Resets the player to the top of the screen, clearing any terrain
 * in the way so a buried player is never stuck on respawn.
 */
void respawn_player(void);

/**
 * @brief Advances player physics by one tick: movement, gravity/swimming,
 * jumping, and collision against the grid.
 */
void update_player(void);

/**
 * @brief Checks whether a grid cell falls within the player's hitbox.
 * @param x Cell X coordinate.
 * @param y Cell Y coordinate.
 * @return true if (x, y) is occupied by the player.
 */
bool player_occupies(int x, int y);

#endif
