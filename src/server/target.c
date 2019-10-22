/*
 * File: target.c
 * Purpose: Targeting code
 *
 * Copyright (c) 1997-2007 Angband contributors
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


/*** Functions ***/


/*
 * Given a "source" and "target" location, extract a "direction",
 * which will move one step from the "source" towards the "target".
 *
 * Note that we use "diagonal" motion whenever possible.
 *
 * We return "5" if no motion is needed.
 */
int motion_dir(int y1, int x1, int y2, int x2)
{
    /* No movement required */
    if ((y1 == y2) && (x1 == x2)) return (5);

    /* South or North */
    if (x1 == x2) return ((y1 < y2) ? 2 : 8);

    /* East or West */
    if (y1 == y2) return ((x1 < x2) ? 6 : 4);

    /* South-east or South-west */
    if (y1 < y2) return ((x1 < x2) ? 3 : 1);

    /* North-east or North-west */
    if (y1 > y2) return ((x1 < x2) ? 9 : 7);

    /* Paranoia */
    return (5);
}


/*
 * Health description (unhurt, wounded, etc)
 */
static const char *look_health_desc(bool living, int chp, int mhp)
{
    int perc;

    /* Dead */
    if (chp < 0)
    {
        /* No damage */
        return (living? "dead": "destroyed");
    }

    /* Healthy */
    if (chp >= mhp)
    {
        /* No damage */
        return (living? "unhurt": "undamaged");
    }

    /* Calculate a health "percentage" */
    perc = 100L * chp / mhp;

    if (perc >= 60)
        return (living? "somewhat wounded": "somewhat damaged");

    if (perc >= 25)
        return (living? "wounded": "damaged");

    if (perc >= 10)
        return (living? "badly wounded": "badly damaged");

    return (living? "almost dead": "almost destroyed");
}


/*
 * Monster health description
 */
void look_mon_desc(struct monster *mon, char *buf, size_t max)
{
    bool living = true;

    /* Determine if the monster is "living" (vs "undead") */
    if (monster_is_nonliving(mon->race)) living = false;

    /* Apply health description */
    my_strcpy(buf, look_health_desc(living, mon->hp, mon->maxhp), max);

    /* Effect status */
    if (mon->m_timed[MON_TMD_SLEEP]) my_strcat(buf, ", asleep", max);
    if (mon->m_timed[MON_TMD_HOLD]) my_strcat(buf, ", held", max);
    if (mon->m_timed[MON_TMD_CONF]) my_strcat(buf, ", confused", max);
    if (mon->m_timed[MON_TMD_FEAR]) my_strcat(buf, ", afraid", max);
    if (mon->m_timed[MON_TMD_STUN]) my_strcat(buf, ", stunned", max);
    if (mon->m_timed[MON_TMD_SLOW]) my_strcat(buf, ", slowed", max);
    if (mon->m_timed[MON_TMD_FAST]) my_strcat(buf, ", hasted", max);

    /* PWMAngband */
    if (mon->m_timed[MON_TMD_BLIND]) my_strcat(buf, ", blind", max);
    if (mon->m_timed[MON_TMD_POIS]) my_strcat(buf, ", poisoned", max);
    if (mon->m_timed[MON_TMD_CUT]) my_strcat(buf, ", bleeding", max);

    /* Monster-specific conditions */
    switch (mon->status)
    {
        case MSTATUS_GUARD: my_strcat(buf, ", guarding", max); break;
        case MSTATUS_FOLLOW: my_strcat(buf, ", following", max); break;
        case MSTATUS_ATTACK: my_strcat(buf, ", attacking", max); break;
    }
}


/*
 * Player health Description
 */
void look_player_desc(struct player *p, char *buf, size_t max)
{
    bool living = true;

    /* Determine if the player is alive */
    if (p->ghost) living = false;

    /* Apply health description */
    my_strcpy(buf, look_health_desc(living, p->chp, p->mhp), max);

    /* Effect status */
    if (p->timed[TMD_PARALYZED]) my_strcat(buf, ", paralyzed", max);
    if (p->timed[TMD_CONFUSED]) my_strcat(buf, ", confused", max);
    if (player_of_has(p, OF_AFRAID) || p->timed[TMD_AFRAID]) my_strcat(buf, ", afraid", max);
    if (p->timed[TMD_STUN]) my_strcat(buf, ", stunned", max);
    if (p->timed[TMD_BLIND]) my_strcat(buf, ", blind", max);
    if (p->timed[TMD_POISONED]) my_strcat(buf, ", poisoned", max);
    if (p->timed[TMD_CUT]) my_strcat(buf, ", bleeding", max);

    /* Player-specific conditions */
    if (player_is_resting(p)) my_strcat(buf, ", resting", max);
}


/*
 * Determine if a monster (or player) makes a reasonable target
 *
 * The concept of "targeting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster (or player).
 *
 * Currently, a monster (or player) is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating. This allows use of "use closest target" macros.
 */
bool target_able(struct player *p, struct source *who)
{
    struct chunk *c = chunk_get(&p->wpos);

    /* No target */
    if (source_null(who)) return false;

    /* Target is a player */
    if (who->player)
    {
        return (COORDS_EQUAL(&p->wpos, &who->player->wpos) && player_is_visible(p, who->idx) &&
            !who->player->k_idx &&
            projectable(c, p->py, p->px, who->player->py, who->player->px, PROJECT_NONE, true) &&
            !p->timed[TMD_IMAGE]);
    }

    return (who->monster->race && monster_is_obvious(p, who->idx, who->monster) &&
        projectable(c, p->py, p->px, who->monster->fy, who->monster->fx, PROJECT_NONE, true) &&
        !p->timed[TMD_IMAGE]);
}


/*
 * Update (if necessary) and verify (if possible) the target.
 *
 * We return true if the target is "okay" and false otherwise.
 */
bool target_okay(struct player *p)
{
    struct source *target_who = &p->target_who;

    /* No target */
    if (!p->target_set) return false;

    /* Allow a direction without a monster */
    if (source_null(target_who))
    {
        if (p->target_x || p->target_y) return true;
        return false;
    }

    /* Check "monster" targets */
    if (target_who->monster)
    {
        /* Accept reasonable targets */
        if (target_able(p, target_who))
        {
            /* Get the monster location */
            p->target_y = target_who->monster->fy;
            p->target_x = target_who->monster->fx;

            /* Good target */
            return true;
        }
    }

    /* Check "player" targets */
    if (target_who->player)
    {
        /* Accept reasonable targets */
        if (target_able(p, target_who))
        {
            /* Get the player location */
            p->target_y = target_who->player->py;
            p->target_x = target_who->player->px;

            /* Good target */
            return true;
        }
    }

    /* Assume no target */
    return false;
}


/*
 * Set the target to a monster/player (or nobody)
 */
bool target_set_monster(struct player *p, struct source *who)
{
    /* Acceptable target */
    if (target_able(p, who))
    {
        /* Save target info */
        p->target_set = true;
        memcpy(&p->target_who, who, sizeof(struct source));
        if (who->monster)
        {
            p->target_y = who->monster->fy;
            p->target_x = who->monster->fx;
        }
        else
        {
            p->target_y = who->player->py;
            p->target_x = who->player->px;
        }

        return true;
    }

    /* Reset target info */
    p->target_set = false;
    memset(&p->target_who, 0, sizeof(p->target_who));
    p->target_y = 0;
    p->target_x = 0;

    return false;
}


/*
 * Set the target to a location
 */
void target_set_location(struct player *p, int y, int x)
{
    struct chunk *c = chunk_get(&p->wpos);

    /* Legal target */
    if (square_in_bounds_fully(c, y, x))
    {
        struct source who_body;
        struct source *who = &who_body;

        square_actor(c, y, x, who);

        /* Save target info */
        p->target_set = true;
        memset(&p->target_who, 0, sizeof(p->target_who));
        if (target_able(p, who))
            memcpy(&p->target_who, who, sizeof(struct source));
        p->target_y = y;
        p->target_x = x;

        return;
    }

    /* Reset target info */
    p->target_set = false;
    memset(&p->target_who, 0, sizeof(p->target_who));
    p->target_y = 0;
    p->target_x = 0;
}


/*
 * Sorting hook -- comp function -- by "distance to player"
 */
int cmp_distance(const void *a, const void *b)
{
    const struct cmp_loc *pa = a;
    const struct cmp_loc *pb = b;
    int da, db, kx, ky;
    struct player *pa_ptr = pa->data;
    struct player *pb_ptr = pb->data;

    /* Absolute distance components */
    kx = pa->x; kx -= pa_ptr->px; kx = ABS(kx);
    ky = pa->y; ky -= pa_ptr->py; ky = ABS(ky);

    /* Approximate Double Distance to the first point */
    da = ((kx > ky)? (kx + kx + ky): (ky + ky + kx));

    /* Absolute distance components */
    kx = pb->x; kx -= pb_ptr->px; kx = ABS(kx);
    ky = pb->y; ky -= pb_ptr->py; ky = ABS(ky);

    /* Approximate Double Distance to the first point */
    db = ((kx > ky)? (kx + kx + ky): (ky + ky + kx));

    /* Compare the distances */
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}


/*
 * Help select a location. This function picks the closest from a set in
 * (roughly) a given direction.
 */
s16b target_pick(int y1, int x1, int dy, int dx, struct point_set *targets)
{
    int i, v;
    int x2, y2, x3, y3, x4, y4;
    int b_i = -1, b_v = 9999;

    /* Scan the locations */
    for (i = 0; i < point_set_size(targets); i++)
    {
        /* Point 2 */
        x2 = targets->pts[i].x;
        y2 = targets->pts[i].y;

        /* Directed distance */
        x3 = (x2 - x1);
        y3 = (y2 - y1);

        /* Verify quadrant */
        if (dx && (x3 * dx <= 0)) continue;
        if (dy && (y3 * dy <= 0)) continue;

        /* Absolute distance */
        x4 = ABS(x3);
        y4 = ABS(y3);

        /* Verify quadrant */
        if (dy && !dx && (x4 > y4)) continue;
        if (dx && !dy && (y4 > x4)) continue;

        /* Approximate Double Distance */
        v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));

        /* Track best */
        if ((b_i >= 0) && (v >= b_v)) continue;

        /* Track best */
        b_i = i; b_v = v;
    }

    /* Result */
    return (b_i);
}


/*
 * Determine if a given location is "interesting"
 */
bool target_accept(struct player *p, int y, int x)
{
    struct object *obj;
    struct chunk *c = chunk_get(&p->wpos);
    struct source who_body;
    struct source *who = &who_body;

    square_actor(c, y, x, who);

    /* Player grids are always interesting */
    if (who->player && (p == who->player)) return true;

    /* Handle hallucination */
    if (p->timed[TMD_IMAGE]) return false;

    /* Obvious players */
    if (who->player && player_is_visible(p, who->idx) && !who->player->k_idx)
        return true;

    /* Obvious monsters */
    if (who->monster && monster_is_obvious(p, who->idx, who->monster))
        return true;

    /* Traps */
    if (square_known_trap(p, c, y, x))
        return true;

    /* Scan all objects in the grid */
    for (obj = square_known_pile(p, c, y, x); obj; obj = obj->next)
    {
        /* Memorized object */
        if (!ignore_item_ok(p, obj)) return true;
    }

    /* Interesting memorized features */
    if (square_isknown(p, y, x) && square_isinteresting(c, y, x))
        return true;

    /* Nope */
    return false;
}


/*
 * Describe a location relative to the player position.
 * e.g. "12 S, 35 W" or "0 N, 33 E" or "0 N, 0 E"
 */
void coords_desc(struct player *p, char *buf, int size, int y, int x)
{
    const char *east_or_west;
    const char *north_or_south;
    int py = p->py;
    int px = p->px;

    if (y > py) north_or_south = "S";
    else north_or_south = "N";

    if (x < px) east_or_west = "W";
    else east_or_west = "E";

    strnfmt(buf, size, "%d %s, %d %s", ABS(y - py), north_or_south, ABS(x - px), east_or_west);
}


/*
 * Obtains the location the player currently targets.
 */
void target_get(struct player *p, int *x, int *y)
{
    *x = p->target_x;
    *y = p->target_y;
}


/*
 * Returns whether the given monster (or player) is the currently targeted monster (or player).
 */
bool target_equals(struct player *p, struct source *who)
{
    struct source *target_who = &p->target_who;

    return source_equal(target_who, who);
}


void draw_path_grid(struct player *p, int y, int x, byte a, char c)
{
    int dispy, dispx;

    /* Draw, Highlight, Fresh, Pause, Erase */
    dispx = x - p->offset_x;
    dispy = y - p->offset_y + 1;

    /* Remember the projectile */
    p->scr_info[dispy][dispx].c = c;
    p->scr_info[dispy][dispx].a = a;

    /* Tell the client */
    Send_char(p, dispx, dispy, a, c, p->trn_info[dispy][dispx].a,
        p->trn_info[dispy][dispx].c);
}


void flush_path_grid(struct player *p, struct chunk *cv, int y, int x, byte a, char c)
{
    /* Draw, Highlight, Fresh, Pause, Erase */
    draw_path_grid(p, y, x, a, c);

    /* Flush and wait */
    Send_flush(p, true, true);

    /* Restore */
    square_light_spot_aux(p, cv, y, x);

    Send_flush(p, true, false);
}


static int player_wounded(struct player *p)
{
    return p->chp * 1000 / p->mhp;
}


static int cmp_wounded(const void *a, const void *b)
{
    const struct cmp_loc *pa = a;
    const struct cmp_loc *pb = b;
    struct player *pa_ptr = pa->data;
    struct player *pb_ptr = pb->data;
    int idx1 = 0 - chunk_get(&pa_ptr->wpos)->squares[pa->y][pa->x].mon;
    int idx2 = 0 - chunk_get(&pb_ptr->wpos)->squares[pb->y][pb->x].mon;
    int w1 = player_wounded(player_get(idx1));
    int w2 = player_wounded(player_get(idx2));

    if (w1 < w2) return -1;
    if (w1 > w2) return 1;
    return 0;
}


#define TS_INITIAL_SIZE  20


/*
 * Get the borders of the area the player can see (the "panel")
 */
static void get_panel(struct player *p, int *min_y, int *min_x, int *max_y, int *max_x)
{
    int screen_hgt = p->screen_rows / p->tile_hgt;
    int screen_wid = p->screen_cols / p->tile_wid;

    *min_y = p->offset_y;
    *min_x = p->offset_x;
    *max_y = p->offset_y + screen_hgt;
    *max_x = p->offset_x + screen_wid;
}


/*
 * Check to see if a map grid is in the panel
 */
bool panel_contains(struct player *p, int y, int x)
{
    return (((unsigned)(y - p->offset_y) < (unsigned)(p->screen_rows / p->tile_hgt)) &&
        ((unsigned)(x - p->offset_x) < (unsigned)(p->screen_cols / p->tile_wid)));
}


/*
 * Return a target set of target_able monsters.
 */
struct point_set *target_get_monsters(struct player *p, int mode)
{
    int y, x;
    int min_y, min_x, max_y, max_x;
    struct point_set *targets = point_set_new(TS_INITIAL_SIZE);
    struct chunk *c = chunk_get(&p->wpos);

    /* Get the current panel */
    get_panel(p, &min_y, &min_x, &max_y, &max_x);

    /* Scan the current panel */
    for (y = min_y; y < max_y; y++)
    {
        for (x = min_x; x < max_x; x++)
        {
            int feat;
            struct source who_body;
            struct source *who = &who_body;

            /* Check bounds */
            if (!square_in_bounds_fully(c, y, x)) continue;

            /* Require line of sight */
            if (!square_isview(p, y, x)) continue;

            /* Require "interesting" contents */
            if (!target_accept(p, y, x)) continue;

            feat = c->squares[y][x].feat;
            square_actor(c, y, x, who);

            /* Special modes */
            if (mode & (TARGET_KILL))
            {
                /* Must be a targetable monster (or player) */
                if (!target_able(p, who)) continue;

                /* Skip non hostile monsters */
                if (who->monster && !pvm_check(p, who->monster)) continue;

                /* Don't target yourself */
                if (who->player && (who->player == p)) continue;

                /* Ignore players we aren't hostile to */
                if (who->player && !pvp_check(p, who->player, PVP_CHECK_BOTH, true, feat))
                    continue;
            }
            else if (mode & (TARGET_HELP))
            {
                /* Must contain a player */
                if (!who->player) continue;

                /* Must be a targetable player */
                if (!target_able(p, who)) continue;

                /* Don't target yourself */
                if (who->player == p) continue;

                /* Ignore players we aren't friends with */
                if (pvp_check(p, who->player, PVP_CHECK_BOTH, true, 0x00)) continue;
            }

            /* Save the location */
            add_to_point_set(targets, p, y, x);
        }
    }

    /* Sort the positions */
    sort(targets->pts, point_set_size(targets), sizeof(*(targets->pts)),
        ((mode & TARGET_HELP)? cmp_wounded: cmp_distance));

    return targets;
}


/*
 * Set target to closest monster (or player)
 */
bool target_set_closest(struct player *p, int mode)
{
    int y, x;
    char m_name[NORMAL_WID];
    struct point_set *targets;
    struct source who_body;
    struct source *who = &who_body;
    struct chunk *c = chunk_get(&p->wpos);

    /* Paranoia */
    if (!c) return false;

    /* Cancel old target */
    target_set_monster(p, NULL);

    /* Get ready to do targeting */
    targets = target_get_monsters(p, mode);

    /* If nothing was prepared, then return */
    if (point_set_size(targets) < 1)
    {
        msg(p, "No available target.");
        point_set_dispose(targets);
        return false;
    }

    /* Find the first monster in the queue */
    y = targets->pts[0].y;
    x = targets->pts[0].x;
    square_actor(c, y, x, who);

    /* Target the monster, if possible */
    if (!target_able(p, who))
    {
        msg(p, "No available target.");
        point_set_dispose(targets);
        return false;
    }

    /* Target the monster */
    if (who->monster)
        monster_desc(p, m_name, sizeof(m_name), who->monster, MDESC_CAPITAL);

    /* Target the player */
    else
        player_desc(p, m_name, sizeof(m_name), who->player, true);

    if (!(mode & TARGET_QUIET)) msg(p, "%s is targeted.", m_name);

    /* Set up target information */
    if (who->monster) monster_race_track(p->upkeep, who);
    health_track(p->upkeep, who);
    target_set_monster(p, who);

    point_set_dispose(targets);
    return true;
}
