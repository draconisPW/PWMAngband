/*
 * File: mon-move.h
 * Purpose: Monster movement
 */

#ifndef MONSTER_MOVE_H
#define MONSTER_MOVE_H

extern bool race_hates_grid(struct chunk *c, struct monster_race *race, struct loc *grid);
extern bool monster_hates_grid(struct chunk *c, struct monster *mon, struct loc *grid);
extern bool multiply_monster(struct player *p, struct chunk *c, struct monster *mon);
extern void process_monsters(struct chunk *c, bool more_energy);
extern void reset_monsters(struct chunk *c);
extern bool is_closest(struct player *p, struct chunk *c, struct monster *mon, bool blos,
    bool new_los, int j, int dis_to_closest, int lowhp);

#endif /* MONSTER_MOVE_H */
