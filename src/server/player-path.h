/*
 * File: player-path.h
 * Purpose: Pathfinding and running code.
 */

#ifndef PLAYER_PATH_H
#define PLAYER_PATH_H

/* Ensure a variable fits into ddx/ddy array bounds */
#define VALID_DIR(D) (((D) >= 0) && ((D) < 10))

extern bool run_step(struct player *p, int dir);

#endif /* PLAYER_PATH_H */
