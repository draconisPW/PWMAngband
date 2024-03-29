/*
 * File: player-attack.h
 * Purpose: Attack interface
 */

#ifndef PLAYER_ATTACK_H
#define PLAYER_ATTACK_H

struct side_effects
{
    bool do_poison;
    int do_stun;
    int do_cut;
    int do_leech;
    int count;
};

struct attack_result
{
    bool success;
    int dmg;
    uint32_t msg_type;
    char verb[30];
    struct side_effects effects;
};

/*
 * A list of the different hit types and their associated special message
 */
struct hit_types
{
    uint32_t msg_type;
    const char *text;
};

/*
 * ranged_attack is a function pointer, used to execute a kind of attack.
 *
 * This allows us to abstract details of throwing, shooting, etc. out while
 * keeping the core projectile tracking, monster cleanup, and display code
 * in common.
 */
typedef struct attack_result (*ranged_attack) (struct player *p, struct object *obj,
    struct loc *grid);

extern bool do_cmd_fire(struct player *p, int dir, int item);
extern bool do_cmd_fire_at_nearest(struct player *p);
extern bool do_cmd_throw(struct player *p, int dir, int item);
extern void py_attack(struct player *p, struct chunk *c, struct loc *grid);
extern int chance_of_melee_hit_base(const struct player *p, const struct object *weapon);
extern void un_power(struct player *p, struct source *who, bool* obvious);
extern void eat_fud(struct player *p, struct player *q, bool* obvious);
extern void drain_xp(struct player *p, int amt);
extern void drop_weapon(struct player *p, int damage);
extern int breakage_chance(const struct object *obj, bool hit_target);
extern bool test_hit(int to_hit, int ac);
extern void hit_chance(random_chance *chance, int to_hit, int ac);

#endif /* PLAYER_ATTACK_H */
