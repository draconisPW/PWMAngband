/*
 * File: mon-make.h
 * Purpose: Structures and functions for monster creation / deletion.
 */

#ifndef MONSTER_MAKE_H
#define MONSTER_MAKE_H

/** Constants **/

/*
 * Flags for monster generation
 */
#define MON_SLEEP   0x01    /* Generate asleep */
#define MON_GROUP   0x02    /* Add an escort */
#define MON_CLONE   0x04    /* Generate as clone */

extern void delete_monster_idx(struct chunk *c, int m_idx);
extern void delete_monster(struct chunk *c, int y, int x);
extern void compact_monsters(struct chunk *c, int num_to_compact);
extern void wipe_mon_list(struct chunk *c);
extern void get_mon_num_prep(bool (*get_mon_num_hook)(struct monster_race *race));
extern struct monster_race *get_mon_num(struct chunk *c, int level);
extern struct monster_race *get_mon_num_poly(int level);
extern void mon_create_drops(struct player *p, struct chunk *c, struct monster *mon);
extern s16b place_monster(struct player *p, struct chunk *c, struct monster *mon, byte origin);
extern int mon_hp(const struct monster_race *race, aspect hp_aspect);
extern bool place_new_monster(struct player *p, struct chunk *c, int y, int x,
    struct monster_race *race, byte mon_flag, byte origin);
extern bool pick_and_place_monster(struct player *p, struct chunk *c, int y, int x, int depth,
    byte mon_flag, byte origin);
extern bool pick_and_place_distant_monster(struct player *p, struct chunk *c, int dis,
    byte mon_flag);
extern void monster_give_xp(struct player *p, struct chunk *c, struct monster *mon, bool split);
extern bool mon_take_hit(struct player *p, struct chunk *c, struct monster *mon, int dam,
    bool *fear, int note);
extern int mon_create_drop_count(const struct monster_race *race, bool maximize);
extern void monster_drop_carried(struct player *p, struct chunk *c, struct monster *mon,
    int num, bool visible, int *dump_item, int *dump_gold);
extern void monster_drop_corpse(struct player *p, struct chunk *c, struct monster *mon);

#endif /* MONSTER_MAKE_H */
