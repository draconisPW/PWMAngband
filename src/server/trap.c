/*
 * File: trap.c
 * Purpose: The trap layer - player traps, runes and door locks
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2025 MAngband and PWMAngband Developers
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
 * Is there a specific kind of trap in this square?
 */
bool square_trap_specific(struct chunk *c, struct loc *grid, unsigned int tidx)
{
    struct trap *trap = square_trap(c, grid);
	
    /* First, check the trap marker */
    if (!square_istrap(c, grid)) return false;

    /* Scan the square trap list */
    while (trap)
    {
        /* We found a trap of the right kind */
        if (trap->kind->tidx == tidx) return true;
        trap = trap->next;
    }

    /* Report failure */
    return false;
}


/*
 * Is there a trap with a given flag in this square?
 */
bool square_trap_flag(struct chunk *c, struct loc *grid, int flag)
{
    struct trap *trap = square_trap(c, grid);

    /* First, check the trap marker */
    if (!square_istrap(c, grid)) return false;

    /* Scan the square trap list */
    while (trap)
    {
        /* We found a trap with the right flag */
        if (trf_has(trap->flags, flag)) return true;
        trap = trap->next;
    }

    /* Report failure */
    return false;
}


/*
 * General trap routines
 */


/*
 * Free memory for all traps on a grid
 */
void square_free_trap(struct chunk *c, struct loc *grid)
{
    struct trap *next, *trap = square_trap(c, grid);

    while (trap)
    {
        next = trap->next;
        mem_free(trap);
        trap = next;
    }
}


/*
 * Remove one trap from a grid.
 *
 * Return true if traps were removed.
 */
bool square_remove_trap(struct chunk *c, struct loc *grid, struct trap *trap, bool memorize)
{
    struct trap *cursor = square(c, grid)->trap;
    struct trap *prev_trap = NULL;
    bool removed = false;

    while (cursor)
    {
        struct trap *next_trap = trap->next;

        if (cursor == trap)
        {
            my_assert(loc_eq(grid, &trap->grid));
            removed = true;
            mem_free(trap);
            if (prev_trap)
                prev_trap->next = next_trap;
            else
            {
                square_set_trap(c, grid, next_trap);
                if (!next_trap)
                {
                    /* There are no more traps here. */
                    sqinfo_off(square(c, grid)->info, SQUARE_TRAP);
                }
            }

            /* Refresh grids that the character can see */
            if (memorize) square_light_spot(c, grid);
            break;
        }

        prev_trap = trap;
        cursor = next_trap;
    }

    return removed;
}


/*
 * Remove all traps from a grid.
 *
 * Return true if traps were removed.
 */
bool square_remove_all_traps(struct chunk *c, struct loc *grid)
{
    struct trap *trap;
    bool were_there_traps;

    trap = square(c, grid)->trap;
    were_there_traps = ((trap == NULL)? false: true);

    while (trap)
    {
        struct trap *next_trap = trap->next;

        mem_free(trap);
        trap = next_trap;
    }

    square_set_trap(c, grid, NULL);
    sqinfo_off(square(c, grid)->info, SQUARE_TRAP);

    /* Refresh grids that the character can see */
    square_light_spot(c, grid);

    return were_there_traps;
}


/*
 * Remove all traps with the given index.
 *
 * Return true if traps were removed.
 */
bool square_remove_all_traps_of_type(struct chunk *c, struct loc *grid, unsigned int t_idx_remove)
{
    bool removed = false;
    struct trap *prev_trap = NULL;
    struct trap *trap;

    /* Look at the traps in this grid */
    trap = square(c, grid)->trap;
    while (trap)
    {
        struct trap *next_trap = trap->next;

        if (t_idx_remove == trap->kind->tidx)
        {
            mem_free(trap);
            removed = true;

            if (prev_trap)
                prev_trap->next = next_trap;
            else
            {
                square_set_trap(c, grid, next_trap);
                if (!next_trap)
                    sqinfo_off(square(c, grid)->info, SQUARE_TRAP);
            }
        }
        else
            prev_trap = trap;

        trap = next_trap;
    }

    /* Refresh grids that the character can see */
    if (removed) square_light_spot(c, grid);

    return removed;
}


/*
 * Player traps
 */


/*
 * Determine if a cave grid is allowed to have player traps in it.
 */
bool square_player_trap_allowed(struct chunk *c, struct loc *grid)
{
    /*
     * We currently forbid multiple traps in a grid under normal conditions.
     * If this changes, various bits of code elsewhere will have to change too.
     */
    if (square_istrap(c, grid)) return false;

    /* We currently forbid traps in a grid with objects. */
    if (square_object(c, grid)) return false;

    /* Check it's a trappable square */
    return (square_istrappable(c, grid));
}


/*
 * Instantiate a player trap
 */
static int pick_trap(int feat, int trap_level, int max_depth)
{
    int i, pick;
    int *trap_probs = NULL;
    int trap_prob_max = 0;

    /* Paranoia */
    if (!feat_is_trap_holding(feat))
		return -1;

    /* Get trap probabilities */
    trap_probs = mem_zalloc(z_info->trap_max * sizeof(int));
    for (i = 0; i < z_info->trap_max; i++)
    {
        /* Get this trap */
        struct trap_kind *kind = &trap_info[i];

        trap_probs[i] = trap_prob_max;

        /* Ensure that this is a valid player trap */
        if (!kind->name) continue;
        if (!kind->rarity) continue;
        if (!trf_has(kind->flags, TRF_TRAP)) continue;

        /* Require that trap_level not be too low */
		if (kind->min_depth > trap_level) continue;

		/* Floor? */
		if (feat_is_floor(feat) && !trf_has(kind->flags, TRF_FLOOR))
			continue;

		/* Check legality of trapdoors. */
		if (trf_has(kind->flags, TRF_DOWN))
	    {
			/* No trap doors on the deepest level */
			if (trap_level == max_depth - 1) continue;
	    }

        /* Trap is okay, store the cumulative probability */
        trap_probs[i] += (100 / kind->rarity);
        trap_prob_max = trap_probs[i];
    }

    /* No valid trap */
    if (trap_prob_max == 0)
    {
        mem_free(trap_probs);
        return -1;
    }

    /* Pick at random. */
    pick = randint0(trap_prob_max);
    for (i = 0; i < z_info->trap_max; i++)
    {
        if (pick < trap_probs[i]) break;
    }

    mem_free(trap_probs);

    /* Return our chosen trap */
    return ((i < z_info->trap_max)? i: -1);
}


/*
 * Make a new trap of the given type. Return true if successful.
 *
 * We choose a player trap at random if the index is not legal.
 *
 * This should be the only function that places traps in the dungeon
 * except the savefile loading code.
 */
void place_trap(struct chunk *c, struct loc *grid, int tidx, int trap_level)
{
    struct trap *new_trap;

    /* We've been called with an illegal index; choose a random trap */
    if ((tidx <= 0) || (tidx >= z_info->trap_max))
    {
        /* Require the correct terrain */
        if (!square_player_trap_allowed(c, grid)) return;

        tidx = pick_trap(square(c, grid)->feat, trap_level,
            get_wt_info_at(&c->wpos.grid)->max_depth);
    }

    /* Failure */
    if (tidx < 0) return;

    /* Allocate a new trap for this grid (at the front of the list) */
    new_trap = mem_zalloc(sizeof(*new_trap));
    new_trap->next = square_trap(c, grid);
    square_set_trap(c, grid, new_trap);

    /* Set the details */
    new_trap->kind = &trap_info[tidx];
    loc_copy(&new_trap->grid, grid);
    new_trap->power = randcalc(new_trap->kind->power, trap_level, RANDOMISE);
    trf_copy(new_trap->flags, trap_info[tidx].flags);

    /* Toggle on the trap marker */
    sqinfo_on(square(c, grid)->info, SQUARE_TRAP);

    /* Redraw the grid */
    square_light_spot(c, grid);
}


/*
 * Reveal some of the player traps in a square
 */
bool square_reveal_trap(struct player *p, struct loc *grid, bool always, bool domsg)
{
    int found_trap = 0;
    struct chunk *c = chunk_get(&p->wpos);
    struct trap *trap = square_trap(c, grid);

    /* Check there is a player trap */
    if (!square_isplayertrap(c, grid)) return false;

    /* Scan the grid */
    while (trap)
    {
		/* Skip non-player traps */
        if (!trf_has(trap->flags, TRF_TRAP))
        {
            trap = trap->next;
            continue;
        }

        /* Skip traps the player doesn't notice */
        if (!always && (p->state.skills[SKILL_SEARCH] < trap->power))
        {
            trap = trap->next;
            continue;
        }

        /* See the trap */
        trf_on(trap->flags, TRF_VISIBLE);

        /* We found a trap */
        found_trap++;

        trap = trap->next;
    }

    /* We found at least one trap */
    if (found_trap)
    {
		/* We want to talk about it */
		if (domsg)
		{
			if (found_trap == 1) msg(p, "You have found a trap.");
			else msg(p, "You have found %d traps.", found_trap);
		}

		/* Memorize */
        square_memorize(p, c, grid);
        square_memorize_trap(p, c, grid);

		/* Redraw */
		square_light_spot(c, grid);
    }
    
    /* Return true if we found any traps */
    return (found_trap != 0);
}


void trap_msg_death(struct player *p, struct trap *trap, char *msg, int len)
{
    if (trap->kind->msg_death)
    {
        switch (trap->kind->msg_death_type)
        {
            case 0:
            {
                my_strcpy(msg, trap->kind->msg_death, len);
                break;
            }
            case 1:
            {
                const char *poss = player_poss(p);

                strnfmt(msg, len, trap->kind->msg_death, poss);
                break;
            }
            case 2:
            {
                const char *pself = player_self(p);

                strnfmt(msg, len, trap->kind->msg_death, pself);
                break;
            }
        }
    }
    else
    {
        char *article = (is_a_vowel(trap->kind->desc[0])? "an ": "a ");

        strnfmt(msg, len, "was killed by %s%s", article, trap->kind->desc);
    }
}


/*
 * Hit a trap
 */
void hit_trap(struct player *p, struct loc *grid, int delayed)
{
    bool ident = false;
    struct trap *trap, *next_trap = NULL;
    struct effect *effect;
    struct chunk *c = chunk_get(&p->wpos);
    int target_depth = dungeon_get_next_level(p, p->wpos.depth, 1);
    struct worldpos wpos;

    /* Ghosts are safe from all traps */
    if (p->ghost) return;

    wpos_init(&wpos, &p->wpos.grid, target_depth);
    
    /* Look at the traps in this grid */
    for (trap = square_trap(c, grid); trap; trap = next_trap)
    {
		int flag;
        bool saved = false, valid = true;

        next_trap = trap->next;

        /* Require that trap be capable of affecting the character */
        if (!trf_has(trap->kind->flags, TRF_TRAP)) continue;
        if (trap->timeout) continue;

        if ((delayed != trf_has(trap->kind->flags, TRF_DELAY)) && (delayed != -1)) continue;

        if (player_is_trapsafe(p))
        {
            /* Trap immune player learns the rune */
            if (player_of_has(p, OF_TRAP_IMMUNE))
                equip_learn_flag(p, OF_TRAP_IMMUNE);

            /* Trap becomes visible. */
            trf_on(trap->flags, TRF_VISIBLE);
            square_memorize(p, c, grid);
            square_memorize_trap(p, c, grid);
            continue;
        }

        /* Disturb the player */
        disturb(p, 0);

        if (trf_has(trap->kind->flags, TRF_DOWN))
        {
            /* Verify basic quests */
            if (is_quest_active(p, p->wpos.depth))
            {
                msg(p, "You feel quite certain something really awful just happened...");
                valid = false;
            }

            /* Hack -- DM redesigning the level */
            if (chunk_inhibit_players(&wpos))
            {
                msg(p, "You feel quite certain something really awful just happened...");
                valid = false;
            }
        }

        if (valid)
        {
            /* Give a message */
            if (trap->kind->msg) msg(p, "%s", trap->kind->msg);

            /* Test for save due to flag */
            for (flag = of_next(trap->kind->save_flags, FLAG_START); flag != FLAG_END;
                flag = of_next(trap->kind->save_flags, flag + 1))
            {
                equip_learn_flag(p, flag);
                if (player_of_has(p, flag)) saved = true;
            }

            /* Test for save due to armor */
            if (trf_has(trap->kind->flags, TRF_SAVE_ARMOR) && !check_hit(p, 125))
                saved = true;

            /* Test for save due to saving throw */
            if (trf_has(trap->kind->flags, TRF_SAVE_THROW) && magik(p->state.skills[SKILL_SAVE]))
                saved = true;

            /* Save, or fire off the trap */
            if (saved)
            {
                if (trap->kind->msg_good) msg(p, "%s", trap->kind->msg_good);
            }
            else
            {
                struct source who_body;
                struct source *who = &who_body;

                source_player(who, get_player_index(get_connection(p->conn)), p);
                who->trap = trap;

                if (trap->kind->msg_bad) msg(p, "%s", trap->kind->msg_bad);

                effect = trap->kind->effect;
                effect_do(effect, who, &ident, false, 0, NULL, 0, 0, NULL);

                /* Trap may have gone or the player may be dead */
                if (!square_trap(c, grid) || p->is_dead) break;

                /* Do any extra effects */
                if (trap->kind->effect_xtra && one_in_(2))
                {
                    if (trap->kind->msg_xtra) msg(p, "%s", trap->kind->msg_xtra);
                    effect = trap->kind->effect_xtra;
                    effect_do(effect, who, &ident, false, 0, NULL, 0, 0, NULL);

                    /* Trap may have gone or the player may be dead */
                    if (!square_trap(c, grid) || p->is_dead) break;
                }
            }

            /* Some traps drop you a dungeon level */
            if (trf_has(trap->kind->flags, TRF_DOWN))
                dungeon_change_level(p, c, &wpos, LEVEL_RAND);

            /* Some traps drop you onto them */
            if (trf_has(trap->kind->flags, TRF_PIT) && !loc_eq(&p->grid, &trap->grid))
            {
                monster_swap(c, &p->grid, &trap->grid);

                /*
                 * Don't retrigger the trap, but handle the
                 * other side effects of moving the player.
                 */
                player_handle_post_move(p, c, false, true, 0, true);
            }

            /* Some traps disappear after activating, all have a chance to */
            if (trf_has(trap->kind->flags, TRF_ONETIME) || one_in_(3))
            {
                if (!square_remove_trap(c, grid, trap, false))
                {
                    my_assert(0);
                }
            }
            else
            {
                /* Trap becomes visible */
                trf_on(trap->flags, TRF_VISIBLE);
                square_memorize(p, c, grid);
                square_memorize_trap(p, c, grid);
            }
        }
        else
        {
            /* Trap becomes visible */
            trf_on(trap->flags, TRF_VISIBLE);
            square_memorize(p, c, grid);
            square_memorize_trap(p, c, grid);
        }
    }

    /* Update the player's view. */
    square_light_spot(c, grid);
}


/*
 * Disable traps for the specified number of turns in the given location
 *
 * c is the chunk to modify.
 * grid is the square to modify.
 * domsg will, if true, cause a message to be displayed when a trap
 * is disabled.
 * t_idx will, if non-negative, cause only traps with the given trap
 * index to be disabled.
 * time is the number of turns to disable the trap.
 *
 * return whether any traps were disabled.
 */
bool square_set_trap_timeout(struct player *p, struct chunk *c, struct loc *grid, bool domsg,
    unsigned int tidx, int time)
{
    bool disabled = false;
    struct trap *current_trap = NULL;

    /* Bounds check */
    my_assert(square_in_bounds(c, grid));

    /* Look at the traps in this grid */
    current_trap = square(c, grid)->trap;
    while (current_trap)
    {
        /* Get the next trap (may be NULL) */
        struct trap *next_trap = current_trap->next;

        /* If called with a specific index, skip others */
        if ((tidx > 0) && (tidx != current_trap->kind->tidx))
        {
            if (!next_trap) break;
            current_trap = next_trap;
            continue;
        }

        /* Set the timer */
        current_trap->timeout = time;
        disabled = true;

        /* Message if requested */
        if (p && domsg)
            msg(p, "You have disabled the %s.", current_trap->kind->name);

        /* Replace with the next trap */
        current_trap = next_trap;
    }

    /* Refresh grids that the character can see */
    square_light_spot(c, grid);

    return disabled;
}


/*
 * Give the remaining time for a trap to be disabled; note it chooses the first
 * appropriate trap on the grid
 */
int square_trap_timeout(struct chunk *c, struct loc *grid, unsigned int tidx)
{
    struct trap *current_trap = square(c, grid)->trap;

    while (current_trap)
    {
        /* Get the next trap (may be NULL) */
        struct trap *next_trap = current_trap->next;

        /* If called with a specific index, skip others */
        if ((tidx > 0) && (tidx != current_trap->kind->tidx))
        {
            if (!next_trap) break;
            current_trap = next_trap;
            continue;
        }

        /* If the timer is set, return the value */
        if (current_trap->timeout) return current_trap->timeout;

        /* Replace with the next trap */
        current_trap = next_trap;
    }

    return 0;
}


/*
 * Door locks
 */


/*
 * Lock a closed door to a given power
 */
void square_set_door_lock(struct chunk *c, struct loc *grid, int power)
{
    struct trap_kind *lock = lookup_trap("door lock");
    struct trap *trap;

    /* Verify it's a closed door */
    if (!square_iscloseddoor(c, grid)) return;

    /* If there's no lock there, add one */
    if (!square_trap_specific(c, grid, lock->tidx))
        place_trap(c, grid, lock->tidx, 0);

    /* Set the power (of all locks - there should be only one) */
    trap = square_trap(c, grid);
    while (trap)
    {
        if (trap->kind == lock) trap->power = power;
        trap = trap->next;
    }
}


/*
 * Return the power of the lock on a door
 */
int square_door_power(struct chunk *c, struct loc *grid)
{
    struct trap_kind *lock = lookup_trap("door lock");
    struct trap *trap;

    /* Verify it's a closed door */
    if (!square_iscloseddoor(c, grid)) return 0;

    /* If there's no lock there, add one */
    if (!square_trap_specific(c, grid, lock->tidx)) return 0;

    /* Get the power and return it */
    trap = square_trap(c, grid);
    while (trap)
    {
        if (trap->kind == lock) return trap->power;
        trap = trap->next;
    }

    return 0;
}