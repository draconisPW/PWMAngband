/*
 * File: mon-util.h
 * Purpose: Functions for monster utilities.
 */

#ifndef MONSTER_UTILITIES_H
#define MONSTER_UTILITIES_H

/** Constants **/

/*
 * Maximum number of picked up/stolen objects a monster can carry
 */
#define MAX_MONSTER_BAG 25

/* Monster status (hostile by default) */
#define MSTATUS_HOSTILE     0   /* hostile */
#define MSTATUS_SUMMONED    1   /* hostile, summoned by the player */
#define MSTATUS_GUARD       2   /* guard, controlled by the player */
#define MSTATUS_FOLLOW      3   /* follower, controlled by the player */
#define MSTATUS_ATTACK      4   /* attacker, controlled by the player */

/** Functions **/
extern const char *describe_race_flag(int flag);
extern void create_mon_flag_mask(bitflag *f, ...);
extern bool match_monster_bases(const struct monster_base *base, ...);
extern void player_desc(struct player *p, char *desc, size_t max, struct player *q,
    bool capitalize);
extern void update_mon(struct monster *mon, struct chunk *c, bool full);
extern void update_monsters(struct chunk *c, bool full);
extern bool monster_carry(struct monster *mon, struct object *obj, bool force);
extern void monster_swap(struct chunk *c, struct loc *grid1, struct loc *grid2);
extern void monster_wake(struct player *p, struct monster *mon, bool notify, int aware_chance);
extern void aware_player(struct player *p, struct player *q);
extern void become_aware(struct player *p, struct chunk *c, struct monster *mon);
extern void update_smart_learn(struct monster *mon, struct player *p, int flag, int pflag,
    int element);
extern bool find_any_nearby_injured_kin(struct chunk *c, const struct monster *mon);
extern struct monster *choose_nearby_injured_kin(struct chunk *c, const struct monster *mon);
extern void monster_death(struct player *p, struct chunk *c, struct monster *mon);
extern bool mon_take_hit(struct player *p, struct chunk *c, struct monster *mon, int dam,
    bool *fear, int note);
extern void monster_take_terrain_damage(struct chunk *c, struct monster *mon);
extern bool monster_taking_terrain_damage(struct chunk *c, struct monster *mon);
extern void update_player(struct player *q);
extern void update_players(void);
extern bool is_humanoid(const struct monster_race *race);
extern bool is_half_humanoid(const struct monster_race *race);
extern void update_monlist(struct monster *mon);
extern bool resist_undead_attacks(struct player *p, struct monster_race *race);
extern void update_view_all(struct worldpos *wpos, int skip);
extern bool monster_change_shape(struct player *p, struct monster *mon);
extern bool monster_revert_shape(struct player *p, struct monster *mon);

#endif /* MONSTER_UTILITIES_H */
