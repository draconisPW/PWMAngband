/*
 * File: mon-util.c
 * Purpose: Monster manipulation utilities.
 *
 * Copyright (c) 1997-2007 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2016 MAngband and PWMAngband Developers
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
 * Return whether the given base matches any of the names given.
 *
 * Accepts a variable-length list of name strings. The list must end with NULL.
 */
bool match_monster_bases(const struct monster_base *base, ...)
{
    bool ok = false;
    va_list vp;
    char *name;

    va_start(vp, base);
    while (!ok && ((name = va_arg(vp, char *)) != NULL))
        ok = (base == lookup_monster_base(name));
    va_end(vp);

    return ok;
}


/*
 * Nonliving monsters are immune to life drain
 */
bool monster_is_nonliving(struct monster_race *race)
{
    return flags_test(race->flags, RF_SIZE, RF_DEMON, RF_UNDEAD, RF_NONLIVING, FLAG_END);
}


/*
 * Nonliving and stupid monsters are destroyed rather than dying
 */
bool monster_is_unusual(struct monster_race *race)
{
    return flags_test(race->flags, RF_SIZE, RF_DEMON, RF_UNDEAD, RF_STUPID, RF_NONLIVING, FLAG_END);
}


void player_desc(struct player *p, char *desc, size_t max, struct player *q, bool capitalize)
{
    int who = get_player_index(get_connection(q->conn));

    if (mflag_has(p->pflag[who], MFLAG_VISIBLE))
        my_strcpy(desc, q->name, max);
    else
        my_strcpy(desc, "someone", max);
    if (capitalize) my_strcap(desc);
}


static bool is_detected_m(struct player *p, const bitflag mflags[RF_SIZE], int d_esp)
{
    /* Full ESP */
    if (player_of_has(p, OF_ESP_ALL)) return true;

    /* Partial ESP */
    if (rf_has(mflags, RF_ORC) && player_of_has(p, OF_ESP_ORC)) return true;
    if (rf_has(mflags, RF_TROLL) && player_of_has(p, OF_ESP_TROLL)) return true;
    if (rf_has(mflags, RF_GIANT) && player_of_has(p, OF_ESP_GIANT)) return true;
    if (rf_has(mflags, RF_DRAGON) && player_of_has(p, OF_ESP_DRAGON)) return true;
    if (rf_has(mflags, RF_DEMON) && player_of_has(p, OF_ESP_DEMON)) return true;
    if (rf_has(mflags, RF_UNDEAD) && player_of_has(p, OF_ESP_UNDEAD)) return true;
    if (rf_has(mflags, RF_EVIL) && player_of_has(p, OF_ESP_EVIL)) return true;
    if (rf_has(mflags, RF_ANIMAL) && player_of_has(p, OF_ESP_ANIMAL)) return true;

    /* Radius ESP */
    if (player_of_has(p, OF_ESP_RADIUS)) return (d_esp <= z_info->max_sight);

    /* No ESP */
    return false;
}


/*
 * Returns true if the given monster is currently mimicking an ignored item.
 */
static bool is_mimicking_ignored(struct player *p, struct monster *mon)
{
    my_assert(mon != NULL);

    if (!(mon->unaware && mon->mimicked_obj)) return false;

    return ignore_item_ok(p, mon->mimicked_obj);
}


/*
 * This function updates the monster record of the given monster
 *
 * This involves extracting the distance to the player (if requested),
 * and then checking for visibility (natural, infravision, see-invis,
 * telepathy), updating the monster visibility flag, redrawing (or
 * erasing) the monster when its visibility changes, and taking note
 * of any interesting monster flags (cold-blooded, invisible, etc).
 *
 * Note the new "mflag" field which encodes several monster state flags,
 * including "view" for when the monster is currently in line of sight,
 * and "mark" for when the monster is currently visible via detection.
 *
 * The only monster fields that are changed here are "cdis" (the
 * distance from the player), "ml" (visible to the player), and
 * "mflag" (to maintain the "MFLAG_VIEW" flag).
 *
 * Note the special "update_monsters()" function which can be used to
 * call this function once for every monster.
 *
 * Note the "full" flag which requests that the "cdis" field be updated;
 * this is only needed when the monster (or the player) has moved.
 *
 * Every time a monster moves, we must call this function for that
 * monster, and update the distance, and the visibility.  Every time
 * the player moves, we must call this function for every monster, and
 * update the distance, and the visibility.  Whenever the player "state"
 * changes in certain ways ("blindness", "infravision", "telepathy",
 * and "see invisible"), we must call this function for every monster,
 * and update the visibility.
 *
 * Routines that change the "illumination" of a grid must also call this
 * function for any monster in that grid, since the "visibility" of some
 * monsters may be based on the illumination of their grid.
 *
 * Note that this function is called once per monster every time the
 * player moves.  When the player is running, this function is one
 * of the primary bottlenecks, along with "update_view()" and the
 * "process_monsters()" code, so efficiency is important.
 *
 * Note the optimized "inline" version of the "distance()" function.
 *
 * A monster is "visible" to the player if (1) it has been detected
 * by the player, (2) it is close to the player and the player has
 * telepathy, or (3) it is close to the player, and in line of sight
 * of the player, and it is "illuminated" by some combination of
 * infravision, torch light, or permanent light (invisible monsters
 * are only affected by "light" if the player can see invisible).
 *
 * Monsters which are not on the current panel may be "visible" to
 * the player, and their descriptions will include an "offscreen"
 * reference.  Currently, offscreen monsters cannot be targeted
 * or viewed directly, but old targets will remain set.  XXX XXX
 *
 * The player can choose to be disturbed by several things, including
 * "disturb_near" (monster which is "easily" viewable moves in some
 * way).  Note that "moves" includes "appears" and "disappears".
 */
static void update_mon_aux(struct player *p, struct monster *mon, struct chunk *c, bool full,
    bool *blos, int *dis_to_closest, struct player **closest, int *lowhp)
{
    struct monster_lore *lore;
    int d, d_esp;
    struct actor who_body;
    struct actor *who = &who_body;

    /* Current location */
    int fy, fx;

    /* Seen at all */
    bool flag = false;

    /* Seen by vision */
    bool easy = false;

    /* ESP permitted */
    bool telepathy_ok = true;

    /* Basic telepathy */
    bool basic = false;

    my_assert(mon != NULL);
    ACTOR_MONSTER(who, mon);

    lore = get_lore(p, mon->race);

    fy = mon->fy;
    fx = mon->fx;

    /* Compute distance */
    d = distance(p->py, p->px, fy, fx);

    /* Restrict distance */
    if (d > 255) d = 255;

    /* PWMAngband: telepathic awareness */
    d_esp = distance(p->py, p->px / 3, fy, fx / 3);
    if (d_esp > 255) d_esp = 255;

    /* Find the closest player */
    if (full)
    {
        /* Hack -- skip him if he's shopping */
        /* Hack -- make the dungeon master invisible to monsters */
        /* Skip player if dead or gone */
        if (!in_store(p) && !(p->dm_flags & DM_MONSTER_FRIEND) &&
            p->alive && !p->is_dead && !p->upkeep->new_level_method)
        {
            /* Check if monster has LOS to the player */
            bool new_los = los(c, fy, fx, p->py, p->px);

            /* Remember this player if closest */
            if (is_closest(p, mon, *blos, new_los, d, *dis_to_closest, *lowhp))
            {
                *blos = new_los;
                *dis_to_closest = d;
                *closest = p;
                *lowhp = p->chp;
            }
        }
    }

    /* Detected */
    if (p->mon_det[mon->midx]) flag = true;

    /* Check if telepathy works */
    if (square_isno_esp(c, fy, fx) || square_isno_esp(c, p->py, p->px))
        telepathy_ok = false;

    /* Nearby */
    if ((d <= z_info->max_sight) || !cfg_limited_esp)
    {
        bool isDM = ((p->dm_flags & DM_SEE_MONSTERS)? true: false);
        bool hasESP = is_detected_m(p, mon->race->flags, d_esp);
        bool isTL = (player_has(p, PF_THUNDERLORD) &&
            (d_esp <= (p->lev * z_info->max_sight / PY_MAX_LEVEL)));

        basic = (isDM || ((hasESP || isTL) && telepathy_ok));

        /* Basic telepathy */
        if (basic)
        {
            /* Empty mind, no telepathy */
            if (rf_has(mon->race->flags, RF_EMPTY_MIND))
            {
                /* Nothing! */
            }

            /* Weird mind, occasional telepathy */
            else if (rf_has(mon->race->flags, RF_WEIRD_MIND))
            {
                /* One in ten individuals are detectable */
                if ((mon->midx % 10) == 5)
                {
                    /* Detectable */
                    flag = true;

                    /* Check for LOS so that MFLAG_VIEW is set later */
                    if (square_isview(p, fy, fx)) easy = true;
                }
            }

            /* Normal mind, allow telepathy */
            else
            {
                /* Detectable */
                flag = true;

                /* Check for LOS so that MFLAG_VIEW is set later */
                if (square_isview(p, fy, fx)) easy = true;
            }

            /* DM has perfect ESP */
            if (isDM)
            {
                /* Detectable */
                flag = true;

                /* Check for LOS so that MFLAG_VIEW is set later */
                if (square_isview(p, fy, fx)) easy = true;
            }
        }

        /* Normal line of sight and player is not blind */
        if (square_isview(p, fy, fx) && !p->timed[TMD_BLIND])
        {
            /* Use "infravision" */
            if (d <= p->state.see_infra)
            {
                /* Learn about warm/cold blood */
                rf_on(lore->flags, RF_COLD_BLOOD);

                /* Handle "warm blooded" monsters */
                if (!rf_has(mon->race->flags, RF_COLD_BLOOD))
                {
                    /* Easy to see */
                    easy = flag = true;
                }
            }

            /* Use "illumination" */
            if (square_isseen(p, fy, fx))
            {
                /* Learn it emits light */
                rf_on(lore->flags, RF_HAS_LIGHT);

                /* Learn about invisibility */
                rf_on(lore->flags, RF_INVISIBLE);

                /* Handle "invisible" monsters */
                if (rf_has(mon->race->flags, RF_INVISIBLE))
                {
                    /* See invisible */
                    if (player_of_has(p, OF_SEE_INVIS))
                    {
                        /* Easy to see */
                        easy = flag = true;
                    }
                }

                /* Handle "normal" monsters */
                else
                {
                    /* Easy to see */
                    easy = flag = true;
                }
            }
        }
    }

    /* If a mimic looks like an ignored item, it's not seen */
    if (is_mimicking_ignored(p, mon))
        easy = flag = false;

    /* The monster is now visible */
    if (flag)
    {
        /* Learn about the monster's mind */
        if (basic)
        {
            flags_set(lore->flags, RF_SIZE, RF_EMPTY_MIND, RF_WEIRD_MIND, RF_SMART, RF_STUPID,
                FLAG_END);
        }

        /* It was previously unseen */
        if (!mflag_has(p->mflag[mon->midx], MFLAG_VISIBLE))
        {
            /* Mark as visible */
            mflag_on(p->mflag[mon->midx], MFLAG_VISIBLE);

            /* Draw the monster */
            square_light_spot_aux(p, c, fy, fx);

            /* Update health bar as needed */
            update_health(who);

            /* Hack -- count "fresh" sightings */
            mon->race->lore.seen = 1;
            lore->pseen = 1;

            /* Redraw */
            p->upkeep->redraw |= (PR_MONLIST);
        }

        /* Efficiency -- notice multi-hued monsters */
        if (monster_shimmer(mon->race) && allow_shimmer(p))
            c->scan_monsters = true;
    }

    /* The monster is not visible */
    else
    {
        /* It was previously seen */
        if (mflag_has(p->mflag[mon->midx], MFLAG_VISIBLE))
        {
            /* Treat mimics differently */
            if (!mon->mimicked_obj || ignore_item_ok(p, mon->mimicked_obj))
            {
                /* Mark as not visible */
                mflag_off(p->mflag[mon->midx], MFLAG_VISIBLE);

                /* Erase the monster */
                square_light_spot_aux(p, c, fy, fx);

                /* Update health bar as needed */
                update_health(who);

                /* Redraw */
                p->upkeep->redraw |= (PR_MONLIST);
            }
        }
    }

    /* The monster is now easily visible */
    if (easy)
    {
        /* Change */
        if (!mflag_has(p->mflag[mon->midx], MFLAG_VIEW))
        {
            /* Mark as easily visible */
            mflag_on(p->mflag[mon->midx], MFLAG_VIEW);

            /* Disturb on appearance (except townies, friendlies and unaware mimics) */
            if (OPT_P(p, disturb_near) && (mon->level > 0) && pvm_check(p, mon) && !is_mimicking(mon))
                disturb(p, 1);

            /* Redraw */
            p->upkeep->redraw |= (PR_MONLIST);
        }
    }

    /* The monster is not easily visible */
    else
    {
        /* Change */
        if (mflag_has(p->mflag[mon->midx], MFLAG_VIEW))
        {
            /* Mark as not easily visible */
            mflag_off(p->mflag[mon->midx], MFLAG_VIEW);

            /* Redraw */
            p->upkeep->redraw |= (PR_MONLIST);
        }
    }
}


void update_mon(struct monster *mon, struct chunk *c, bool full)
{
    int i;
    bool blos = false;
    struct player *closest = NULL;
    int dis_to_closest = 9999, lowhp = 9999;
    struct actor who_body;
    struct actor *who = &who_body;

    my_assert(mon != NULL);
    ACTOR_MONSTER(who, mon);

    /* Check for each player */
    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);

        /* Make sure he's on the same dungeon level */
        if (p->depth != mon->depth) continue;

        update_mon_aux(p, mon, c, full, &blos, &dis_to_closest, &closest, &lowhp);
    }

    /* Track closest player */
    if (full)
    {
        /* Forget player status */
        if (closest != mon->closest_player)
        {
            of_wipe(mon->known_pstate.flags);
            pf_wipe(mon->known_pstate.pflags);
            for (i = 0; i < ELEM_MAX; i++)
                mon->known_pstate.el_info[i].res_level = 0;
        }

        /* Always track closest player */
        mon->closest_player = closest;

        /* Paranoia -- make sure we found a closest player */
        if (closest) mon->cdis = dis_to_closest;
    }

    /* Update the cursor */
    update_cursor(who);
}


/*
 * Updates all the (non-dead) monsters via update_mon().
 */
void update_monsters(struct chunk *c, bool full)
{
    int i;

    /* Efficiency -- clear multi-hued flag */
    c->scan_monsters = false;

    /* Update each (live) monster */
    for (i = 1; i < cave_monster_max(c); i++)
    {
        struct monster *mon = cave_monster(c, i);

        /* Update the monster if alive */
        if (mon->race) update_mon(mon, c, full);
    }
}


/*  
 * See if a monster can carry an object (it will pick up either way)
 */
static bool monster_can_carry(struct monster *mon, struct object *obj, bool force)
{
    int total_number = 0;
    struct object *held_obj;

    /* Always carry artifacts */
    if (obj->artifact) return true;

    /* Clones don't carry stuff */
    if (mon->clone) return false;

    /* Force to carry monster drops */
    if (force) return true;

    /* Only carry stuff in the dungeon */
    if (mon->depth <= 0) return false;

#if !defined(MAX_MONSTER_BAG)
    return true;
#else
    /* Scan objects already being held for combination */
    for (held_obj = mon->held_obj; held_obj; held_obj = held_obj->next)
        total_number++;

    /*
     * Chance-based response. The closer monster to his limit, the less the chance is.
     * If he reached the limit, he will not pick up
     * XXX XXX XXX -- double chance && strict limit
     */
    if ((randint0(MAX_MONSTER_BAG) * 2 > total_number) && (total_number < MAX_MONSTER_BAG))
        return true;

    return false;
#endif
}


/*
 * Add the given object to the given monster's inventory.
 *
 * Returns true if the object is successfully added, false otherwise.
 */
bool monster_carry(struct monster *mon, struct object *obj, bool force)
{
    struct object *held_obj;

    /* See if the monster can carry the object */
    if (!monster_can_carry(mon, obj, force)) return false;

    /* Scan objects already being held for combination */
    for (held_obj = mon->held_obj; held_obj; held_obj = held_obj->next)
    {
        /* Check for combination */
        if (object_similar(NULL, held_obj, obj, OSTACK_MONSTER))
        {
            /* Combine the items */
            object_absorb(held_obj, obj);

            /* Result */
            return true;
        }
    }

    /* Forget location */
    obj->iy = obj->ix = 0;

    /* Hack -- reset index */
    obj->oidx = 0;

    /* Link the object to the monster */
    obj->held_m_idx = mon->midx;
    obj->depth = mon->depth;

    /* Add the object to the monster's inventory */
    pile_insert(&mon->held_obj, obj);

    /* Result */
    return true;
}


/*
 * Swap the players/monsters (if any) at two locations.
 */
void monster_swap(struct chunk *c, int y1, int x1, int y2, int x2)
{
    struct player *p;
    int m1, m2;
    struct monster *mon;

    /* Monsters */
    m1 = c->squares[y1][x1].mon;
    m2 = c->squares[y2][x2].mon;

    /* Update grids */
    c->squares[y1][x1].mon = m2;
    c->squares[y2][x2].mon = m1;

    /* Monster 1 */
    if (m1 > 0)
    {
        mon = cave_monster(c, m1);

        /* Move monster */
        mon->fy = y2;
        mon->fx = x2;

        /* Update monster */
        update_mon(mon, c, true);

        /* Radiate light? */
        if (rf_has(mon->race->flags, RF_HAS_LIGHT))
        {
            int i;

            for (i = 1; i <= NumPlayers; i++)
            {
                struct player *q = player_get(i);

                if (q->depth == c->depth) q->upkeep->update |= PU_UPDATE_VIEW;
            }
        }
    }

    /* Player 1 */
    else if (m1 < 0)
    {
        p = player_get(0 - m1);

        /* Hack -- save previous player location */
        p->old_py = p->py;
        p->old_px = p->px;

        /* Move player */
        p->py = y2;
        p->px = x2;

        /* Update the trap detection status */
        p->upkeep->redraw |= (PR_DTRAP);

        /* Redraw */
        p->upkeep->redraw |= (PR_FLOOR | PR_MONLIST | PR_ITEMLIST);
        p->upkeep->redraw |= (PR_SPELL | PR_STUDY);

        /* Update the panel */
        verify_panel(p);

        /* Update the visuals (and monster distances) */
        p->upkeep->update |= (PU_UPDATE_VIEW | PU_DISTANCE);

        /* Update the flow */
        p->upkeep->update |= (PU_UPDATE_FLOW);

        /* Radiate light? */
        if (p->state.cur_light)
        {
            int i;

            for (i = 1; i <= NumPlayers; i++)
            {
                struct player *q = player_get(i);

                if ((q->depth == c->depth) && (i != 0 - m1))
                    q->upkeep->update |= PU_UPDATE_VIEW;
            }
        }
    }

    /* Monster 2 */
    if (m2 > 0)
    {
        mon = cave_monster(c, m2);

        /* Move monster */
        mon->fy = y1;
        mon->fx = x1;

        /* Update monster */
        update_mon(mon, c, true);

        /* Radiate light? */
        if (rf_has(mon->race->flags, RF_HAS_LIGHT))
        {
            int i;

            for (i = 1; i <= NumPlayers; i++)
            {
                struct player *q = player_get(i);

                if (q->depth == c->depth) q->upkeep->update |= PU_UPDATE_VIEW;
            }
        }
    }

    /* Player 2 */
    else if (m2 < 0)
    {
        p = player_get(0 - m2);

        /* Hack -- save previous player location */
        p->old_py = p->py;
        p->old_px = p->px;

        /* Move player */
        p->py = y1;
        p->px = x1;

        /* Update the trap detection status */
        p->upkeep->redraw |= (PR_DTRAP);

        /* Redraw */
        p->upkeep->redraw |= (PR_FLOOR | PR_MONLIST | PR_ITEMLIST);
        p->upkeep->redraw |= (PR_SPELL | PR_STUDY);

        /* Update the panel */
        verify_panel(p);

        /* Update the visuals (and monster distances) */
        p->upkeep->update |= (PU_UPDATE_VIEW | PU_DISTANCE);

        /* Update the flow */
        p->upkeep->update |= (PU_UPDATE_FLOW);

        /* Radiate light? */
        if (p->state.cur_light)
        {
            int i;

            for (i = 1; i <= NumPlayers; i++)
            {
                struct player *q = player_get(i);

                if ((q->depth == c->depth) && (i != 0 - m2))
                    q->upkeep->update |= PU_UPDATE_VIEW;
            }
        }
    }

    /* Redraw */
    square_light_spot(c, y1, x1);
    square_light_spot(c, y2, x2);
}


/*
 * Make player fully aware of the given player.
 */
void aware_player(struct player *p, struct player *q)
{
    if (!q->k_idx) return;

    q->k_idx = 0;
    p->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
}


/*
 * Make player fully aware of the given mimic.
 *
 * When a player becomes aware of a mimic, we update the monster memory
 * and delete the "fake item" that the monster was mimicking.
 */
void become_aware(struct player *p, struct chunk *c, struct monster *mon)
{
    struct monster_lore *lore = (p? get_lore(p, mon->race): NULL);

    if (!mon->unaware) return;

    mon->unaware = false;

    /* Learn about mimicry */
    if (lore && rf_has(mon->race->flags, RF_UNAWARE)) rf_on(lore->flags, RF_UNAWARE);

    /* Delete any false items */
    if (mon->mimicked_obj)
    {
        struct object *obj = mon->mimicked_obj;

        /* Print a message */
        if (p && square_isseen(p, obj->iy, obj->ix))
        {
            char o_name[NORMAL_WID];

            object_desc(p, o_name, sizeof(o_name), obj, ODESC_BASE);
            msg(p, "The %s was really a monster!", o_name);
        }

        /* Clear the mimicry */
        obj->mimicking_m_idx = 0;
        mon->mimicked_obj = NULL;

        square_excise_object(c, obj->iy, obj->ix, obj);

        /* Give the object to the monster if appropriate */
        /* Otherwise delete the mimicked object */
        if (!rf_has(mon->race->flags, RF_MIMIC_INV) || !monster_carry(mon, obj, true))
            object_delete(&obj);
    }

    /* Delete any false features */
    if (mon->race->base == lookup_monster_base("feature mimic"))
    {
        /* Print a message */
        if (p)
            msg(p, "The %s was really a monster!", f_info[c->squares[mon->fy][mon->fx].feat].name);

        /* Clear the feature */
        square_set_feat(c, mon->fy, mon->fx, mon->feat);
    }

    /* Update monster and item lists */
    if (p)
    {
        p->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
        p->upkeep->redraw |= (PR_MONLIST | PR_ITEMLIST);
    }

    square_note_spot(c, mon->fy, mon->fx);
    square_light_spot(c, mon->fy, mon->fx);
}


/*
 * Returns true if the given monster is currently mimicking.
 */
bool is_mimicking(struct monster *mon)
{
    return mon->unaware;
}


/*
 * The given monster learns about an "observed" resistance or other player
 * state property, or lack of it.
 *
 * Note that this function is robust to being called with `element` as an
 * arbitrary GF_ type.
 */
void update_smart_learn(struct monster *mon, struct player *p, int flag, int pflag,
    int element)
{
    bool element_ok = ((element >= 0) && (element < ELEM_MAX));

    /* Sanity check */
    if (!flag && !element_ok) return;

    /* Anything a monster might learn, the player should learn */
    if (flag) equip_notice_flag(p, flag);
    if (element_ok) equip_notice_element(p, element);

    /* Not allowed to learn */
    if (!cfg_ai_learn) return;

    /* Too stupid to learn anything */
    if (rf_has(mon->race->flags, RF_STUPID)) return;

    /* Not intelligent, only learn sometimes */
    if (!rf_has(mon->race->flags, RF_SMART) && one_in_(2)) return;

    /* Analyze the knowledge; fail very rarely */
    if (one_in_(100)) return;

    /* Learn the flag */
    if (flag)
    {
        if (player_of_has(p, flag))
            of_on(mon->known_pstate.flags, flag);
        else
            of_off(mon->known_pstate.flags, flag);
    }

    /* Learn the pflag */
    if (pflag)
    {
        if (pf_has(p->state.pflags, pflag))
            of_on(mon->known_pstate.pflags, pflag);
        else
            of_off(mon->known_pstate.pflags, pflag);
    }

    /* Learn the element */
    if (element_ok)
        mon->known_pstate.el_info[element].res_level = p->state.el_info[element].res_level;
}


static bool is_detected_p(struct player *p, struct player *q, int dis_esp)
{
    /* Full ESP */
    if (player_of_has(p, OF_ESP_ALL)) return true;

    /* Partial ESP */
    if (player_has(q, PF_ORC) && player_of_has(p, OF_ESP_ORC)) return true;
    if (player_has(q, PF_TROLL) && player_of_has(p, OF_ESP_TROLL)) return true;
    if (player_has(q, PF_GIANT) && player_of_has(p, OF_ESP_GIANT)) return true;
    if (player_has(q, PF_THUNDERLORD) && player_of_has(p, OF_ESP_DRAGON)) return true;
    if (player_has(q, PF_ANIMAL) && player_of_has(p, OF_ESP_ANIMAL)) return true;
    if (player_has(q, PF_DRAGON) && player_of_has(p, OF_ESP_DRAGON)) return true;

    /* Radius ESP */
    if (player_of_has(p, OF_ESP_RADIUS)) return (dis_esp <= z_info->max_sight);

    /* No ESP */
    return false;
}


static void update_player_aux(struct player *p, struct player *q, struct chunk *c)
{
    int d, d_esp;
    int id = get_player_index(get_connection(q->conn));

    /* Current location */
    int py, px;

    /* Seen at all */
    bool flag = false;

    /* Seen by vision */
    bool easy = false;

    /* ESP permitted */
    bool telepathy_ok = true;

    py = q->py;
    px = q->px;

    /* Compute distance */
    d = distance(py, px, p->py, p->px);

    /* Restrict distance */
    if (d > 255) d = 255;

    /* PWMAngband: telepathic awareness */
    d_esp = distance(py, px / 3, p->py, p->px / 3);
    if (d_esp > 255) d_esp = 255;

    /* Detected */
    if (p->play_det[id]) flag = true;

    /* Check if telepathy works */
    if (square_isno_esp(c, py, px) || square_isno_esp(c, p->py, p->px))
        telepathy_ok = false;

    /* Nearby */
    if ((d <= z_info->max_sight) || !cfg_limited_esp)
    {
        bool isDM = ((p->dm_flags & DM_SEE_PLAYERS)? true: false);
        bool hasESP = is_detected_p(p, q, d_esp);
        bool isTL = (player_has(p, PF_THUNDERLORD) &&
            (d_esp <= (p->lev * z_info->max_sight / PY_MAX_LEVEL)));

        /* Basic telepathy */
        if (isDM || ((hasESP || isTL) && telepathy_ok))
        {
            /* Empty mind, no telepathy */
            if (q->poly_race && rf_has(q->poly_race->flags, RF_EMPTY_MIND)) {}

            /* Weird mind, occasional telepathy */
            else if (q->poly_race && rf_has(q->poly_race->flags, RF_WEIRD_MIND))
            {
                /* One in ten individuals are detectable */
                if ((id % 10) == 5)
                {
                    /* Detectable */
                    flag = true;

                    /* Check for LOS so that MFLAG_VIEW is set later */
                    if (square_isview(p, py, px)) easy = true;
                }
            }

            /* Normal mind, allow telepathy */
            else
            {
                /* Detectable */
                flag = true;

                /* Check for LOS so that MFLAG_VIEW is set later */
                if (square_isview(p, py, px)) easy = true;
            }

            /* DM has perfect ESP */
            if (isDM)
            {
                /* Detectable */
                flag = true;

                /* Check for LOS so that MFLAG_VIEW is set later */
                if (square_isview(p, py, px)) easy = true;
            }
        }

        /* Normal line of sight, and not blind */
        if (square_isview(p, py, px) && !p->timed[TMD_BLIND])
        {
            /* Use "infravision" */
            if (d <= p->state.see_infra)
            {
                /* Handle "cold blooded" players */
                if (q->poly_race && rf_has(q->poly_race->flags, RF_COLD_BLOOD)) {}

                /* Handle "warm blooded" players */
                else
                {
                    /* Easy to see */
                    easy = flag = true;
                }
            }

            /* Use "illumination" */
            if (square_isseen(p, py, px))
            {
                /* Handle "invisible" players */
                if ((q->poly_race && rf_has(q->poly_race->flags, RF_INVISIBLE)) ||
                    q->timed[TMD_INVIS])
                {
                    /* See invisible */
                    if (player_of_has(p, OF_SEE_INVIS))
                    {
                        /* Easy to see */
                        easy = flag = true;
                    }
                }

                /* Handle "normal" monsters */
                else
                {
                    /* Easy to see */
                    easy = flag = true;
                }
            }
        }

        /* Players in the same party are always visible */
        if (in_party(p, q->party)) easy = flag = true;

        /* Hack -- dungeon masters are invisible */
        if (q->dm_flags & DM_SECRET_PRESENCE) easy = flag = false;
    }

    /* Player is now visible */
    if (flag)
    {
        /* It was previously unseen */
        if (!mflag_has(p->pflag[id], MFLAG_VISIBLE))
        {
            /* Mark as visible */
            mflag_on(p->pflag[id], MFLAG_VISIBLE);

            /* Draw the player */
            square_light_spot_aux(p, c, py, px);
        }
        else
        {
            /* Player color may have changed! */
            square_light_spot_aux(p, c, py, px);
        }

        /* Efficiency -- notice multi-hued players */
        if (q->poly_race && monster_shimmer(q->poly_race) && allow_shimmer(p))
            q->shimmer = true;

        /* Efficiency -- notice party leaders */
        if (is_party_owner(p, q) && OPT_P(p, highlight_leader))
            q->shimmer = true;

        /* Efficiency -- notice elementalists */
        if (player_has(q, PF_ELEMENTAL_SPELLS) && allow_shimmer(p))
            q->shimmer = true;
    }

    /* The player is not visible */
    else
    {
        /* It was previously seen */
        if (mflag_has(p->pflag[id], MFLAG_VISIBLE))
        {
            /* Mark as not visible */
            mflag_off(p->pflag[id], MFLAG_VISIBLE);

            /* Erase the player */
            square_light_spot_aux(p, c, py, px);
        }
    }

    /* The player is now easily visible */
    if (easy)
    {
        /* Change */
        if (!mflag_has(p->pflag[id], MFLAG_VIEW))
        {
            /* Mark as easily visible */
            mflag_on(p->pflag[id], MFLAG_VIEW);

            /* Disturb on appearance (except friendlies and unaware mimics) */
            if (OPT_P(p, disturb_near) && pvp_check(p, q, PVP_CHECK_ONE, true, 0x00) && !q->k_idx)
            {
                /* Disturb */
                disturb(p, 1);
            }
        }
    }

    /* The player is not easily visible */
    else
    {
        /* Change */
        if (mflag_has(p->pflag[id], MFLAG_VIEW))
        {
            /* Mark as not easily visible */
            mflag_off(p->pflag[id], MFLAG_VIEW);
        }
    }
}


/*
 * This function updates the visibility flags for everyone who may see
 * this player.
 */
void update_player(struct player *q)
{
    int i, id = get_player_index(get_connection(q->conn));
    struct actor who_body;
    struct actor *who = &who_body;

    /* Efficiency -- clear "shimmer" flag */
    q->shimmer = false;

    /* Check for every other player */
    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);

        /* Skip players not on this depth */
        if (p->depth != q->depth)
        {
            mflag_wipe(p->pflag[id]);
            p->play_det[id] = 0;
            continue;
        }

        /* Player can always see himself */
        if (q == p) continue;

        update_player_aux(p, q, chunk_get(p->depth));
    }

    ACTOR_PLAYER(who, 0, q);
    update_cursor(who);
}

/*
 * This function simply updates all the players (see above).
 */
void update_players(void)
{
    int i;

    /* Update each player */
    for (i = 1; i <= NumPlayers; i++)
    {
        /* Update the player */
        update_player(player_get(i));
    }
}


bool is_humanoid(const struct monster_race *race)
{
    return rf_has(race->flags, RF_HUMANOID);
}


/*
 * Half humanoid monsters: nagas (half snake/half human), hybrids,
 * driders (half spider/half human), human metamorphs
 */
bool is_half_humanoid(const struct monster_race *race)
{
    if ((race->base == lookup_monster_base("naga")) || strstr(race->name, "harpy") ||
        strstr(race->name, "taur") || streq(race->name, "Sphinx") ||
        streq(race->name, "Gorgon") || streq(race->name, "Drider") ||
        strstr(race->name, "Were")) return true;

    return false;
}


void update_monlist(struct monster *mon)
{
    int i;

    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);

        if (p->depth == mon->depth)
            p->upkeep->redraw |= PR_MONLIST;
    }
}


bool resist_undead_attacks(struct player *p, struct monster_race *race)
{
    return (rf_has(race->flags, RF_UNDEAD) && player_has(p, PF_UNDEAD_POWERS) &&
        (p->lev >= 6) && magik(40 + p->lev));
}

