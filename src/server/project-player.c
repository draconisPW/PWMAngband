/*
 * File: project-player.c
 * Purpose: Projection effects on players
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2019 MAngband and PWMAngband Developers
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */


#include "s-angband.h"


/*
 * Is player susceptible to an attack?
 *
 * true means normal damage, false means no damage
 */
static bool is_susceptible(struct monster_race *race, int type)
{
    switch (type)
    {
        /* Check these attacks against vulnerability (polymorphed players) */
        case PROJ_LIGHT_WEAK: return (race && rf_has(race->flags, RF_HURT_LIGHT));
        case PROJ_KILL_WALL: return (race && rf_has(race->flags, RF_HURT_ROCK));
        case PROJ_DISP_EVIL: return (race && monster_is_evil(race));
        case PROJ_DISP_UNDEAD: return (race && rf_has(race->flags, RF_UNDEAD));

        /* Check these attacks against immunity (polymorphed players) */
        case PROJ_PSI_DRAIN:
        case PROJ_MON_DRAIN:
        case PROJ_DRAIN: return !(race && monster_is_nonliving(race));

        /* Everything else will cause normal damage */
        default: return true;
    }
}


/*
 * Is player vulnerable to an attack?
 *
 * true means extra damage, false means normal damage
 */
static bool is_vulnerable(struct monster_race *race, int type)
{
    switch (type)
    {
        /* Check these attacks against vulnerability (polymorphed players) */
        case PROJ_FIRE: return (race && rf_has(race->flags, RF_HURT_FIRE));
        case PROJ_COLD:
        case PROJ_ICE: return (race && rf_has(race->flags, RF_HURT_COLD));
        case PROJ_LIGHT: return (race && rf_has(race->flags, RF_HURT_LIGHT));

        /* Everything else will cause normal damage */
        default: return false;
    }
}


/*
 * Adjust damage according to resistance or vulnerability.
 *
 * type is the attack type we are checking.
 * dam is the unadjusted damage.
 * dam_aspect is the calc we want (min, avg, max, random).
 * resist is the degree of resistance (-1 = vuln, 3 = immune).
 */
int adjust_dam(struct player *p, int type, int dam, aspect dam_aspect, int resist)
{
    int i, denom = 0;

    /* If an actual player exists, get their actual resist */
    if (p && (type < ELEM_MAX))
    {
        /* Hack -- ice damage checks against cold resistance */
        int res_type = ((type == PROJ_ICE)? PROJ_COLD: type);

        resist = p->state.el_info[res_type].res_level;

        /* Notice element stuff */
        equip_learn_element(p, res_type);
    }

    if (dam <= 0) return 0;

    /* Immune */
    if (resist == 3) return 0;

    /* Hack -- acid damage is halved by armour, holy orb is halved */
    if (((type == PROJ_ACID) && p && minus_ac(p)) || (type == PROJ_HOLY_ORB))
        dam = (dam + 1) / 2;

    /* Hack -- biofeedback halves "sharp" damage */
    if (p && p->timed[TMD_BIOFEEDBACK])
    {
        switch (type)
        {
            case PROJ_MISSILE:
            case PROJ_ARROW_X:
            case PROJ_ARROW_1:
            case PROJ_ARROW_2:
            case PROJ_ARROW_3:
            case PROJ_ARROW_4:
            case PROJ_BOULDER:
            case PROJ_SHARD:
            case PROJ_SOUND:
                dam = (dam + 1) / 2;
                break;
        }
    }

    /* Hack -- no damage from certain attacks unless vulnerable */
    if (p && !is_susceptible(p->poly_race, type)) dam = 0;

    /* Hack -- extra damage from certain attacks if vulnerable */
    if (p && is_vulnerable(p->poly_race, type)) dam = dam * 4 / 3;

    /* Vulnerable */
    if (resist == -1) return (dam * 4 / 3);

    /*
     * Variable resists vary the denominator, so we need to invert the logic
     * of dam_aspect. (m_bonus is unused)
     */
    switch (dam_aspect)
    {
        case MINIMISE:
            denom = randcalc(projections[type].denominator, 0, MAXIMISE);
            break;
        case MAXIMISE:
            denom = randcalc(projections[type].denominator, 0, MINIMISE);
            break;
        case AVERAGE:
        case RANDOMISE:
            denom = randcalc(projections[type].denominator, 0, dam_aspect);
            break;
    }

    for (i = resist; i > 0; i--)
    {
        if (denom) dam = dam * projections[type].numerator / denom;
    }

    return dam;
}


/*
 * Player handlers
 */


/*
 * Drain stats at random
 *
 * p is the affected player
 * num is the number of points to drain
 */
static void project_player_drain_stats(struct player *p, int num)
{
    int i, k = 0;
    const char *act = NULL;

    for (i = 0; i < num; i++)
    {
        switch (randint1(STAT_MAX))
        {
            case 1: k = STAT_STR; act = "strong"; break;
            case 2: k = STAT_INT; act = "bright"; break;
            case 3: k = STAT_WIS; act = "wise"; break;
            case 4: k = STAT_DEX; act = "agile"; break;
            case 5: k = STAT_CON; act = "hale"; break;
        }

        msg(p, "You're not as %s as you used to be...", act);
        player_stat_dec(p, k, false);
    }
}


/*
 * Swap stats at random to temporarily scramble the player's stats.
 */
void project_player_swap_stats(struct player *p)
{
    int max1, cur1, max2, cur2, i, j, swap;

    /* Fisher-Yates shuffling algorithm. */
    for (i = STAT_MAX - 1; i > 0; --i)
    {
        j = randint0(i);

        max1 = p->stat_max[i];
        cur1 = p->stat_cur[i];
        max2 = p->stat_max[j];
        cur2 = p->stat_cur[j];

        p->stat_max[i] = max2;
        p->stat_cur[i] = cur2;
        p->stat_max[j] = max1;
        p->stat_cur[j] = cur1;

        /* Record what we did */
        swap = p->stat_map[i];
        p->stat_map[i] = p->stat_map[j];
        p->stat_map[j] = swap;
    }

    player_inc_timed(p, TMD_SCRAMBLE, randint0(20) + 20, true, true);
}


void project_player_time_effects(struct player *p, struct source *who)
{
    /* Life draining */
    if (one_in_(2))
    {
        int drain = 100 + (p->exp / 100) * z_info->life_drain_percent;

        msg(p, "You feel your life force draining away!");
        player_exp_lose(p, drain, false);
    }

    /* Drain some stats */
    else if (!one_in_(5))
    {
        int num = 1;

        /* PWMAngband: time resistance prevents nastiest effect */
        if (!player_resists(p, ELEM_TIME)) num = 2;

        project_player_drain_stats(p, num);
    }

    /* Drain all stats */
    else
    {
        int perma = p->state.el_info[ELEM_TIME].res_level;

        if (p->timed[TMD_ANCHOR]) perma--;

        if (who->monster)
            update_smart_learn(who->monster, p, 0, 0, ELEM_TIME);

        /* PWMAngband: permanent time resistance prevents the effect completely */
        if (perma)
            msg(p, "You resist the effect!");

        /* PWMAngband: space/time anchor prevents nastiest effect */
        else if (p->timed[TMD_ANCHOR])
        {
            /* Life draining */
            if (randint1(9) < 6)
            {
                msg(p, "You feel your life force draining away!");
                player_exp_lose(p, 100 + (p->exp / 100) * z_info->life_drain_percent, false);
            }

            /* Drain one stat */
            else
                project_player_drain_stats(p, 1);
        }

        /* Normal case */
        else
        {
            int i;

            msg(p, "You're not as powerful as you used to be...");
            for (i = 0; i < STAT_MAX; i++) player_stat_dec(p, i, false);
        }
    }
}


typedef struct project_player_handler_context_s
{
    /* Input values */
    struct source *origin;
    int r;
    struct chunk *cave;
    int y;
    int x;
    int dam;
    int type;

    /* Return values */
    bool obvious;
} project_player_handler_context_t;


typedef void (*project_player_handler_f)(project_player_handler_context_t *);


static void project_player_handler_ACID(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, 0, 0, ELEM_ACID);
    if (player_is_immune(p, ELEM_ACID))
    {
        msg(p, "You resist the effect!");
        return;
    }

    inven_damage(p, PROJ_ACID, MIN(context->dam * 5, 300));
}


static void project_player_handler_ELEC(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, 0, 0, ELEM_ELEC);
    if (player_is_immune(p, ELEM_ELEC))
    {
        msg(p, "You resist the effect!");
        return;
    }

    inven_damage(p, PROJ_ELEC, MIN(context->dam * 5, 300));
}


static void project_player_handler_FIRE(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, 0, 0, ELEM_FIRE);
    if (player_is_immune(p, ELEM_FIRE))
    {
        msg(p, "You resist the effect!");
        return;
    }

    inven_damage(p, PROJ_FIRE, MIN(context->dam * 5, 300));
}


static void project_player_handler_COLD(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, 0, 0, ELEM_COLD);
    if (player_is_immune(p, ELEM_COLD))
    {
        msg(p, "You resist the effect!");
        return;
    }

    inven_damage(p, PROJ_COLD, MIN(context->dam * 5, 300));
}


static void project_player_handler_POIS(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (player_resists(p, ELEM_POIS))
    {
        msg(p, "You resist the effect!");
        return;
    }

    player_inc_timed(p, TMD_POISONED, 10 + randint1(context->dam), true, check);
}


static void project_player_handler_LIGHT(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, OF_PROT_BLIND, 0, -1);
    if (player_resists(p, ELEM_LIGHT) || player_of_has(p, OF_PROT_BLIND))
    {
        msg(p, "You resist the effect!");
        return;
    }

    player_inc_timed(p, TMD_BLIND, 2 + randint1(5), true, check);
}


static void project_player_handler_DARK(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, OF_PROT_BLIND, 0, -1);
    if (player_resists(p, ELEM_DARK) || player_of_has(p, OF_PROT_BLIND))
    {
        msg(p, "You resist the effect!");
        return;
    }

    player_inc_timed(p, TMD_BLIND, 2 + randint1(5), true, check);
}


static void project_player_handler_SOUND(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, OF_PROT_STUN, 0, -1);
    if (player_resists(p, ELEM_SOUND) || player_of_has(p, OF_PROT_STUN))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Stun */
    player_inc_timed(p, TMD_STUN, MIN(5 + randint1(context->dam / 3), 35), true, check);
}


static void project_player_handler_SHARD(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (player_resists(p, ELEM_SHARD))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Cuts */
    player_inc_timed(p, TMD_CUT, randint1(context->dam), true, check);
}


static void project_player_handler_NEXUS(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    struct source who_body;
    struct source *who = &who_body;

    source_player(who, get_player_index(get_connection(p->conn)), p);

    if (player_resists(p, ELEM_NEXUS))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Stat swap */
    if (magik(p->state.skills[SKILL_SAVE]))
        msg(p, "You avoid the effect!");
    else
        project_player_swap_stats(p);

    /* Teleport to */
    if (one_in_(3))
    {
        if (context->origin->monster)
        {
            who->monster = context->origin->monster;
            effect_simple(EF_TELEPORT_TO, who, "0", 0, 0, 0, NULL);
        }
        else
        {
            effect_simple(EF_TELEPORT_TO, who, "0", context->origin->player->py,
                context->origin->player->px, 0, NULL);
        }

        /* Hack -- get new location */
        context->y = p->py;
        context->x = p->px;
    }

    /* Teleport level */
    else if (one_in_(4))
    {
        if (magik(p->state.skills[SKILL_SAVE]))
        {
            msg(p, "You avoid the effect!");
            return;
        }
        effect_simple(EF_TELEPORT_LEVEL, who, "0", 0, 0, 0, NULL);
    }

    /* Teleport */
    else
    {
        effect_simple(EF_TELEPORT, who, "200", 0, 0, 0, NULL);

        /* Hack -- get new location */
        context->y = p->py;
        context->x = p->px;
    }
}


static void project_player_handler_NETHER(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    int drain = 20 + (p->exp / 50) * z_info->life_drain_percent;

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, OF_HOLD_LIFE, 0, -1);
    if (player_resists(p, ELEM_NETHER) || player_of_has(p, OF_HOLD_LIFE))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Life draining */
    msg(p, "You feel your life force draining away!");
    player_exp_lose(p, drain, false);
}


static void project_player_handler_CHAOS(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (context->origin->monster)
    {
        update_smart_learn(context->origin->monster, p, OF_PROT_CONF, 0, -1);
        update_smart_learn(context->origin->monster, p, OF_HOLD_LIFE, 0, -1);
    }
    if (player_resists(p, ELEM_CHAOS))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Hallucination */
    player_inc_timed(p, TMD_IMAGE, randint1(10), true, check);

    /* Confusion */
    if (player_of_has(p, OF_PROT_CONF))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_CONFUSED, 10 + randint0(20), true, check);

    /* Life draining */
    if (player_of_has(p, OF_HOLD_LIFE))
        msg(p, "You resist the effect!");
    else
    {
        int drain = 50 + (p->exp / 50) * z_info->life_drain_percent;

        msg(p, "You feel your life force draining away!");
        player_exp_lose(p, drain, false);
    }
}


static void project_player_handler_DISEN(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    struct source who_body;
    struct source *who = &who_body;

    if (player_resists(p, ELEM_DISEN))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Disenchant gear */
    source_player(who, get_player_index(get_connection(p->conn)), p);
    effect_simple(EF_DISENCHANT, who, "0", 0, 0, 0, NULL);
}


static void project_player_handler_WATER(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (context->origin->monster)
    {
        update_smart_learn(context->origin->monster, p, OF_PROT_CONF, 0, -1);
        update_smart_learn(context->origin->monster, p, OF_PROT_STUN, 0, -1);
    }

    /* Confusion */
    if (player_of_has(p, OF_PROT_CONF))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_CONFUSED, 5 + randint1(5), true, check);

    /* Stun */
    if (player_of_has(p, OF_PROT_STUN))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_STUN, randint1(40), true, check);
}


static void project_player_handler_ICE(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (context->origin->monster)
    {
        update_smart_learn(context->origin->monster, p, 0, 0, ELEM_SHARD);
        update_smart_learn(context->origin->monster, p, OF_PROT_STUN, 0, -1);
        update_smart_learn(context->origin->monster, p, 0, 0, ELEM_COLD);
    }

    if (player_is_immune(p, ELEM_COLD))
        msg(p, "You resist the effect!");
    else
        inven_damage(p, PROJ_COLD, MIN(context->dam * 5, 300));

    /* Cuts */
    if (player_resists(p, ELEM_SHARD))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_CUT, damroll(5, 8), true, check);

    /* Stun */
    if (player_of_has(p, OF_PROT_STUN))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_STUN, randint1(15), true, check);
}


static void project_player_handler_GRAVITY(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    msg(p, "Gravity warps around you.");

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, OF_PROT_STUN, 0, -1);

    /* Blink */
    if (randint1(127) > p->lev)
    {
        struct source who_body;
        struct source *who = &who_body;

        source_player(who, get_player_index(get_connection(p->conn)), p);
        effect_simple(EF_TELEPORT, who, "5", 0, 0, 0, NULL);

        /* Hack -- get new location */
        context->y = p->py;
        context->x = p->px;
    }

    /* Slow */
    player_inc_timed(p, TMD_SLOW, 4 + randint0(4), true, check);

    /* Stun */
    if (player_of_has(p, OF_PROT_STUN))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_STUN, MIN(5 + randint1(context->dam / 3), 35), true, check);
}


static void project_player_handler_INERTIA(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    /* Slow */
    player_inc_timed(p, TMD_SLOW, 4 + randint0(4), true, check);
}


static void project_player_handler_FORCE(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);
    char grids_away[5];
    struct loc centre;
    struct source who_body;
    struct source *who = &who_body;

    /* Get location of caster (assumes index of caster is not zero) */
    origin_get_loc(&centre, context->origin);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, OF_PROT_STUN, 0, -1);
    if (player_of_has(p, OF_PROT_STUN))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Stun */
    player_inc_timed(p, TMD_STUN, randint1(20), true, check);

    /* Thrust player away. */
    strnfmt(grids_away, sizeof(grids_away), "%d", 3 + context->dam / 20);
    source_player(who, get_player_index(get_connection(p->conn)), p);
    who->trap = context->origin->trap;
    effect_simple(EF_THRUST_AWAY, who, grids_away, centre.y, centre.x, 0, NULL);

    /* Hack -- get new location */
    context->y = p->py;
    context->x = p->px;
}


static void project_player_handler_TIME(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    project_player_time_effects(p, context->origin);
}


static void project_player_handler_PLASMA(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool check = (context->origin->monster? false: true);

    if (context->origin->monster)
        update_smart_learn(context->origin->monster, p, OF_PROT_STUN, 0, -1);
    if (player_of_has(p, OF_PROT_STUN))
    {
        msg(p, "You resist the effect!");
        return;
    }

    /* Stun */
    player_inc_timed(p, TMD_STUN, MIN(5 + randint1(context->dam * 3 / 4), 35), true, check);
}


static void project_player_handler_METEOR(project_player_handler_context_t *context) {}
static void project_player_handler_MISSILE(project_player_handler_context_t *context) {}
static void project_player_handler_MANA(project_player_handler_context_t *context) {}
static void project_player_handler_HOLY_ORB(project_player_handler_context_t *context) {}
static void project_player_handler_ARROW_X(project_player_handler_context_t *context) {}
static void project_player_handler_ARROW_1(project_player_handler_context_t *context) {}
static void project_player_handler_ARROW_2(project_player_handler_context_t *context) {}
static void project_player_handler_ARROW_3(project_player_handler_context_t *context) {}
static void project_player_handler_ARROW_4(project_player_handler_context_t *context) {}
static void project_player_handler_BOULDER(project_player_handler_context_t *context) {}
static void project_player_handler_LIGHT_WEAK(project_player_handler_context_t *context) {}
static void project_player_handler_DARK_WEAK(project_player_handler_context_t *context) {}
static void project_player_handler_KILL_WALL(project_player_handler_context_t *context) {}
static void project_player_handler_KILL_DOOR(project_player_handler_context_t *context) {}
static void project_player_handler_KILL_TRAP(project_player_handler_context_t *context) {}
static void project_player_handler_MAKE_DOOR(project_player_handler_context_t *context) {}
static void project_player_handler_MAKE_TRAP(project_player_handler_context_t *context) {}
static void project_player_handler_STONE_WALL(project_player_handler_context_t *context) {}
static void project_player_handler_RAISE(project_player_handler_context_t *context) {}


/* PvP handlers */


static void project_player_handler_AWAY_EVIL(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Only evil players */
    if (p->poly_race && monster_is_evil(p->poly_race))
    {
        char dice[5];
        struct source who_body;
        struct source *who = &who_body;

        strnfmt(dice, sizeof(dice), "%d", context->dam);
        source_player(who, get_player_index(get_connection(p->conn)), p);
        effect_simple(EF_TELEPORT, who, dice, 0, 0, 0, NULL);

        /* Hack -- get new location */
        context->y = p->py;
        context->x = p->px;
    }
    else
        msg(p, "You resist the effect!");
}


static void project_player_handler_AWAY_ALL(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    char dice[5];
    struct source who_body;
    struct source *who = &who_body;

    strnfmt(dice, sizeof(dice), "%d", context->dam);
    source_player(who, get_player_index(get_connection(p->conn)), p);
    effect_simple(EF_TELEPORT, who, dice, 0, 0, 0, NULL);

    /* Hack -- get new location */
    context->y = p->py;
    context->x = p->px;
}


static void project_player_handler_TURN_UNDEAD(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Only undead players */
    if (p->poly_race && rf_has(p->poly_race->flags, RF_UNDEAD))
    {
        /* Fear */
        if (player_of_has(p, OF_PROT_FEAR))
            msg(p, "You resist the effect!");
        else
            player_inc_timed(p, TMD_AFRAID, 3 + randint1(4), true, true);
    }
    else
        msg(p, "You resist the effect!");
}


static void project_player_handler_TURN_ALL(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Fear */
    if (player_of_has(p, OF_PROT_FEAR))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_AFRAID, 3 + randint1(4), true, true);
}


static void project_player_handler_DISP_UNDEAD(project_player_handler_context_t *context) {}
static void project_player_handler_DISP_EVIL(project_player_handler_context_t *context) {}
static void project_player_handler_DISP_ALL(project_player_handler_context_t *context) {}


static void project_player_handler_MON_CLONE(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Disable */
    msg(p, "You resist the effect!");
}


static void project_player_handler_MON_POLY(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    if (player_resists(p, ELEM_NEXUS))
        msg(p, "You resist the effect!");

    /* Swap stats */
    else if (one_in_(2))
    {
        if (magik(p->state.skills[SKILL_SAVE]))
            msg(p, "You avoid the effect!");
        else
            project_player_swap_stats(p);
    }

    /* Poly bat */
    else
    {
        char killer[NORMAL_WID];

        my_strcpy(killer, context->origin->player->name, sizeof(killer));
        poly_bat(p, 10 + context->dam * 4, killer);
    }
}


static void project_player_handler_MON_HEAL(project_player_handler_context_t *context)
{
    project_player_handler_MON_CLONE(context);
}


static void project_player_handler_MON_SPEED(project_player_handler_context_t *context)
{
    project_player_handler_MON_CLONE(context);
}


static void project_player_handler_MON_SLOW(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Slow */
    if (player_of_has(p, OF_FREE_ACT))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_SLOW, 3 + randint1(4), true, true);
}


static void project_player_handler_MON_CONF(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Confusion */
    if (player_of_has(p, OF_PROT_CONF))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_CONFUSED, 3 + randint1(4), true, true);
}


static void project_player_handler_MON_SLEEP(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Paralysis */
    if (player_of_has(p, OF_FREE_ACT))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_PARALYZED, 3 + randint1(4), true, true);
}


static void project_player_handler_MON_HOLD(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Paralysis */
    if (player_of_has(p, OF_FREE_ACT))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_PARALYZED, 3 + randint1(4), true, true);
}


static void project_player_handler_MON_STUN(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Stun */
    if (player_of_has(p, OF_PROT_STUN))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_STUN, 3 + randint1(4), true, true);
}


static void project_player_handler_MON_DRAIN(project_player_handler_context_t *context) {}


static void project_player_handler_PSI(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    switch (randint1(4))
    {
        /* Confusion */
        case 1:
        {
            if (player_of_has(p, OF_PROT_CONF))
                msg(p, "You resist the effect!");
            else
                player_inc_timed(p, TMD_CONFUSED, 3 + randint1(4), true, true);
            break;
        }

        /* Stun */
        case 2:
        {
            if (player_of_has(p, OF_PROT_STUN))
                msg(p, "You resist the effect!");
            else
                player_inc_timed(p, TMD_STUN, 3 + randint1(4), true, true);
            break;
        }

        /* Fear */
        case 3:
        {
            if (player_of_has(p, OF_PROT_FEAR))
                msg(p, "You resist the effect!");
            else
                player_inc_timed(p, TMD_AFRAID, 3 + randint1(4), true, true);
            break;
        }

        /* Paralysis */
        default:
        {
            if (player_of_has(p, OF_FREE_ACT))
                msg(p, "You resist the effect!");
            else
                player_inc_timed(p, TMD_PARALYZED, 3 + randint1(4), true, true);
            break;
        }
    }
}


static void project_player_handler_DEATH(project_player_handler_context_t *context) {}

static void project_player_handler_PSI_DRAIN(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    int drain = context->dam;
    char dice[5];

    if (!drain) return;

    if (drain > p->chp) drain = p->chp;
    strnfmt(dice, sizeof(dice), "%d", 1 + 3 * drain / 4);
    effect_simple(EF_RESTORE_MANA, context->origin, dice, 0, 0, 0, NULL);
}

static void project_player_handler_CURSE(project_player_handler_context_t *context) {}


static void project_player_handler_CURSE2(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Cuts */
    player_inc_timed(p, TMD_CUT, damroll(10, 10), true, true);
}


static void project_player_handler_DRAIN(project_player_handler_context_t *context) {}
static void project_player_handler_GUARD(project_player_handler_context_t *context) {}
static void project_player_handler_FOLLOW(project_player_handler_context_t *context) {}


static void project_player_handler_TELE_TO(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    struct source who_body;
    struct source *who = &who_body;

    source_player(who, get_player_index(get_connection(p->conn)), p);
    effect_simple(EF_TELEPORT_TO, who, "0", context->origin->player->py, context->origin->player->px,
        0, NULL);

    /* Hack -- get new location */
    context->y = p->py;
    context->x = p->px;
}


static void project_player_handler_TELE_LEVEL(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    if (player_resists(p, ELEM_NEXUS))
        msg(p, "You resist the effect!");
    else
    {
        struct source who_body;
        struct source *who = &who_body;

        source_player(who, get_player_index(get_connection(p->conn)), p);
        effect_simple(EF_TELEPORT_LEVEL, who, "0", 0, 0, 0, NULL);
    }
}


static void project_player_handler_MON_BLIND(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Blindness */
    if (player_of_has(p, OF_PROT_BLIND))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_BLIND, 11 + randint1(4), true, true);
}


static void project_player_handler_DRAIN_MANA(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);
    bool seen = (!p->timed[TMD_BLIND] && player_is_visible(p, context->origin->idx));

    drain_mana(p, context->origin, (randint1(context->dam) / 2) + 1, seen);
}


static void project_player_handler_FORGET(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Amnesia */
    player_inc_timed(p, TMD_AMNESIA, 8, true, true);
}


static void project_player_handler_BLAST(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Confusion */
    if (player_of_has(p, OF_PROT_CONF))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_CONFUSED, 3 + randint1(4), true, true);

    /* Amnesia */
    player_inc_timed(p, TMD_AMNESIA, 4, true, true);
}


static void project_player_handler_SMASH(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    /* Slow */
    player_inc_timed(p, TMD_SLOW, 3 + randint1(4), true, true);

    /* Blindness */
    if (player_of_has(p, OF_PROT_BLIND))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_BLIND, 7 + randint1(8), true, true);

    /* Confusion */
    if (player_of_has(p, OF_PROT_CONF))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_CONFUSED, 3 + randint1(4), true, true);

    /* Paralysis */
    if (player_of_has(p, OF_FREE_ACT))
        msg(p, "You resist the effect!");
    else
        player_inc_timed(p, TMD_PARALYZED, 3 + randint1(4), true, true);

    /* Amnesia */
    player_inc_timed(p, TMD_AMNESIA, 4, true, true);
}


static void project_player_handler_ATTACK(project_player_handler_context_t *context) {}
static void project_player_handler_CONTROL(project_player_handler_context_t *context) {}


static void project_player_handler_PROJECT(project_player_handler_context_t *context)
{
    struct player *p = player_get(0 - context->cave->squares[context->y][context->x].mon);

    int cidx = context->origin->player->clazz->cidx;

    if (context->origin->player->ghost && !player_can_undead(context->origin->player))
        cidx = CLASS_GHOST;

    /* "dam" is used as spell index */
    cast_spell_proj(p, cidx, context->dam, false);
}


static const project_player_handler_f player_handlers[] =
{
    #define ELEM(a) project_player_handler_##a,
    #include "../common/list-elements.h"
    #undef ELEM
    #define PROJ(a) project_player_handler_##a,
    #include "../common/list-projections.h"
    #undef PROJ
    NULL
};


static bool project_p_is_threat(int type)
{
    bool threat = false;

    /* Is this type of attack a threat? */
    switch (type)
    {
        case PROJ_AWAY_ALL:
        case PROJ_PROJECT:
        case PROJ_TELE_TO:
        case PROJ_TELE_LEVEL: break;
        default: threat = true; break;
    }

    return threat;
}


/*
 * Called from project() to affect the player
 *
 * Called for projections with the PROJECT_PLAY flag set, which includes
 * bolt, beam, ball and breath effects.
 *
 * origin is the origin of the effect
 * r is the distance from the centre of the effect
 * c is the current cave
 * (y, x) the coordinates of the grid being handled
 * dam is the "damage" from the effect at distance r from the centre
 * typ is the projection (PROJ_) type
 * what is the message to other players if the target dies
 * did_hit is true if a player was hit
 * was_obvious is true if the effects were obvious
 *
 * If "r" is non-zero, then the blast was centered elsewhere; the damage
 * is reduced in project() before being passed in here. This can happen if a
 * monster breathes at the player and hits a wall instead.
 *
 * We assume the player is aware of some effect, and always return "true".
 */
void project_p(struct source *origin, int r, struct chunk *c, int y, int x, int dam, int typ,
    const char *what, bool *did_hit, bool *was_obvious, int *newy, int *newx)
{
    bool blind, seen;
    bool obvious = true;
    bool dead = false;

    /* Monster name (for attacks) */
    char m_name[NORMAL_WID];

    /* Monster or trap name (for damage) */
    char killer[NORMAL_WID];

    project_player_handler_f player_handler;
    project_player_handler_context_t context;

    /* Projected spell */
    int index = dam;

    struct player *p = player_get(0 - c->squares[y][x].mon);

    *did_hit = false;
    *was_obvious = false;
    *newy = y;
    *newx = x;

    /* No player here */
    if (!p) return;

    /* Never affect projector (except when trying to polymorph self) */
    if ((origin->player == p) && (typ != PROJ_MON_POLY)) return;

    /* Obtain player info */
    blind = (p->timed[TMD_BLIND]? true: false);
    seen = !blind;

    /* Hack -- polymorph self */
    if ((origin->player == p) && (typ == PROJ_MON_POLY)) {}

    /* Hit by a trap */
    else if (origin->trap)
    {
        /* Get the trap name */
        strnfmt(killer, sizeof(killer), "%s%s",
            (is_a_vowel(origin->trap->kind->desc[0])? "an ": "a "), origin->trap->kind->desc);
    }

    /* The caster is a monster */
    else if (origin->monster)
    {
        /* Check it is visible */
        if (!monster_is_visible(p, origin->idx)) seen = false;

        /* Get the monster name */
        monster_desc(p, m_name, sizeof(m_name), origin->monster, MDESC_CAPITAL);

        /* Get the monster's real name */
        monster_desc(p, killer, sizeof(killer), origin->monster, MDESC_DIED_FROM);

        /* Check hostility for threatening spells */
        if (!pvm_check(p, origin->monster)) return;
    }

    /* The caster is a player */
    else if (origin->player)
    {
        /* Check it is visible */
        if (!player_is_visible(p, origin->idx)) seen = false;

        /* Get the player name */
        player_desc(p, m_name, sizeof(m_name), origin->player, true);

        /* Get the player's real name */
        my_strcpy(killer, origin->player->name, sizeof(killer));

        /* Check hostility for threatening spells */
        if (project_p_is_threat(typ))
        {
            struct source target_body;
            struct source *target = &target_body;
            int mode;

            source_player(target, 0, p);
            mode = (target_equals(origin->player, target)? PVP_DIRECT: PVP_INDIRECT);

            if (!pvp_check(origin->player, p, mode, true, c->squares[y][x].feat))
                return;
        }
    }

    /* Let player know what is going on */
    if (!seen && projections[typ].blind_desc) msg(p, "You %s!", projections[typ].blind_desc);

    /* Hack -- polymorph self */
    if ((origin->player == p) && (typ == PROJ_MON_POLY))
    {
        /* XXX */
        dam = adjust_dam(p, PROJ_MON_POLY, dam, RANDOMISE, 0);
    }

    /* Hit by a trap */
    else if (origin->trap)
    {
        /* Adjust damage for resistance, immunity or vulnerability, and apply it */
        dam = adjust_dam(p, typ, dam, RANDOMISE, 0);
        if (dam)
        {
            char df[160];

            trap_msg_death(p, origin->trap, df, sizeof(df));
            dead = take_hit(p, dam, killer, false, df);
        }
    }

    /* The caster is a monster */
    else if (origin->monster)
    {
        /* Adjust damage for resistance, immunity or vulnerability, and apply it */
        dam = adjust_dam(p, typ, dam, RANDOMISE, 0);
        if (dam)
        {
            char df[160];

            strnfmt(df, sizeof(df), "was %s by %s", what, killer);
            dead = take_hit(p, dam, killer, true, df);
        }
    }

    /* The caster is a player */
    else if (origin->player)
    {
        /* Try a saving throw if available */
        if ((projections[typ].flags & ATT_SAVE) && magik(p->state.skills[SKILL_SAVE]))
        {
            msg(p, "You resist the effects!");

            /* Hack */
            dead = true;
        }
        else
        {
            bool non_physical = ((projections[typ].flags & ATT_NON_PHYS)? true: false);

            /* Adjust damage for resistance, immunity or vulnerability, and apply it */
            dam = adjust_dam(p, typ, dam, RANDOMISE, 0);
            if (dam && (projections[typ].flags & ATT_DAMAGE))
            {
                char df[160];

                strnfmt(df, sizeof(df), "was %s by %s", what, killer);
                dead = take_hit(p, dam, killer, non_physical, df);
            }

            /* Give a message */
            if (dam && (projections[typ].flags & ATT_DAMAGE) && !dead)
                player_pain(origin->player, p, dam);

            /* Projected spell */
            if (projections[typ].flags & ATT_RAW) dam = index;
        }
    }

    context.origin = origin;
    context.r = r;
    context.cave = c;
    context.y = y;
    context.x = x;
    context.dam = dam;
    context.type = typ;
    context.obvious = obvious;

    player_handler = player_handlers[typ];

    /* Handle side effects */
    if ((player_handler != NULL) && !dead) player_handler(&context);

    obvious = context.obvious;

    /* Disturb */
    disturb(p, 1);

    /* Track this player */
    *did_hit = true;
    *newy = context.y;
    *newx = context.x;

    /* Return "Anything seen?" */
    *was_obvious = obvious;
}