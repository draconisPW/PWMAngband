/*
 * File: trap.h
 * Purpose: Trap predicates, structs and functions
 */

#ifndef TRAP_H
#define TRAP_H

extern bool square_trap_specific(struct chunk *c, int y, int x, unsigned int tidx);
extern bool square_trap_flag(struct chunk *c, int y, int x, int flag);
extern bool square_player_trap_allowed(struct chunk *c, int y, int x);
extern void place_trap(struct chunk *c, int y, int x, int tidx, int trap_level);
extern void square_free_trap(struct chunk *c, int y, int x);
extern bool square_reveal_trap(struct player *p, int y, int x, bool always, bool domsg);
extern bool trap_check_hit(struct player *p, int power);
extern void trap_msg_death(struct player *p, struct trap *trap, char *msg, int len);
extern void hit_trap(struct player *p);
extern void wipe_trap_list(struct chunk *c);
extern bool square_remove_all_traps(struct chunk *c, int y, int x);
extern bool square_remove_trap(struct chunk *c, int y, int x, unsigned int t_idx_remove);
extern bool square_set_trap_timeout(struct player *p, struct chunk *c, int y, int x, bool domsg,
    unsigned int tidx, int time);
extern int square_trap_timeout(struct chunk *c, int y, int x, unsigned int tidx);
extern void square_set_door_lock(struct chunk *c, int y, int x, int power);
extern int square_door_power(struct chunk *c, int y, int x);

#endif /* TRAP_H */
