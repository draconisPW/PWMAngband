/*
 * File: mon-timed.c
 * Purpose: Monster timed effects.
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
 * Monster timed effects.
 */
static struct mon_timed_effect
{
    const char *name;
    int message_begin;
    int message_end;
    int message_increase;
    int flag_resist;
    int max_timer;
} effects[] =
{
    #define MON_TMD(a, b, c, d, e, f) {#a, b, c, d, e, f},
    #include "../common/list-mon-timed.h"
    #undef MON_TMD
    {"MAX", 0, 0, 0, 0, 0}
};


int mon_timed_name_to_idx(const char *name)
{
    int i;

    for (i = 0; !streq(effects[i].name, "MAX"); i++)
    {
        if (streq(name, effects[i].name)) return i;
    }

    return -1;
}


/*
 * Determines whether the given monster successfully resists the given effect.
 *
 * If MON_TMD_FLG_NOFAIL is set in `flag`, this returns false.
 * Then we determine if the monster resists the effect for some racial
 * reason. For example, the monster might have the NO_SLEEP flag, in which
 * case it always resists sleep. Or if it breathes chaos, it always resists
 * confusion. If the given monster doesn't resist for any of these reasons,
 * then it makes a saving throw. If MON_TMD_MON_SOURCE is set in `flag`,
 * indicating that another monster caused this effect, then the chance of
 * success on the saving throw just depends on the monster's native depth.
 * Otherwise, the chance of success decreases as `timer` increases.
 *
 * Also marks the lore for any appropriate resists.
 */
static bool mon_resist_effect(struct player *p, const struct monster *mon, int ef_idx, int timer,
    u16b flag)
{
    struct mon_timed_effect *effect;
    int resist_chance;
    struct monster_lore *lore;
    bool visible;

    my_assert((ef_idx >= 0) && (ef_idx < MON_TMD_MAX));
    my_assert(mon);

    effect = &effects[ef_idx];
    lore = (p? get_lore(p, mon->race): NULL);
    visible = (p? mflag_has(p->mflag[mon->midx], MFLAG_VISIBLE): false);

    /* Hasting never fails */
    if (ef_idx == MON_TMD_FAST) return false;

    /* Some effects are marked to never fail */
    if (flag & MON_TMD_FLG_NOFAIL) return false;

    /* A sleeping monster resists further sleeping */
    if ((ef_idx == MON_TMD_SLEEP) && mon->m_timed[ef_idx]) return true;

    /* If the monster resists innately, learn about it */
    if (rf_has(mon->race->flags, effect->flag_resist))
    {
        if (visible) rf_on(lore->flags, effect->flag_resist);

        return true;
    }

    /* Monsters with specific breaths resist stunning/paralysis */
    if (((ef_idx == MON_TMD_STUN) || (ef_idx == MON_TMD_HOLD)) &&
        (rsf_has(mon->race->spell_flags, RSF_BR_SOUN) ||
        rsf_has(mon->race->spell_flags, RSF_BR_WALL)))
    {
        /* Add the lore */
        if (visible)
        {
            if (rsf_has(mon->race->spell_flags, RSF_BR_SOUN))
                rsf_on(lore->spell_flags, RSF_BR_SOUN);
            if (rsf_has(mon->race->spell_flags, RSF_BR_WALL))
                rsf_on(lore->spell_flags, RSF_BR_WALL);
        }

        return true;
    }

    /* Monsters with specific breaths resist confusion */
    if ((ef_idx == MON_TMD_CONF) && rsf_has(mon->race->spell_flags, RSF_BR_CHAO))
    {
        /* Add the lore */
        if (visible)
        {
            if (rsf_has(mon->race->spell_flags, RSF_BR_CHAO))
                rsf_on(lore->spell_flags, RSF_BR_CHAO);
        }

        return true;
    }

    /* Monsters with specific breaths resist cut */
    if ((ef_idx == MON_TMD_CUT) && rsf_has(mon->race->spell_flags, RSF_BR_SHAR))
    {
        /* Add the lore */
        if (visible)
        {
            if (rsf_has(mon->race->spell_flags, RSF_BR_SHAR))
                rsf_on(lore->spell_flags, RSF_BR_SHAR);
        }

        return true;
    }

    /* Inertia breathers resist slowing */
    if ((ef_idx == MON_TMD_SLOW) && rsf_has(mon->race->spell_flags, RSF_BR_INER))
    {
        if (lore) rsf_on(lore->spell_flags, RSF_BR_INER);
        return true;
    }

    /* Hack -- sleep uses much bigger numbers */
    if (ef_idx == MON_TMD_SLEEP) timer /= 25;

    /* Calculate the chance of the monster making its saving throw. */
    if (flag & MON_TMD_MON_SOURCE)
        resist_chance = mon->race->level;
    else
        resist_chance = mon->race->level + 40 - (timer / 2);

    if (randint0(100) < resist_chance) return true;

    /* Uniques are doubly hard to affect */
    if (rf_has(mon->race->flags, RF_UNIQUE))
    {
        if (randint0(100) < resist_chance) return true;
    }

    return false;
}


/*
 * Attempts to set the timer of the given monster effect to `timer`.
 *
 * Checks to see if the monster resists the effect, using mon_resist_effect().
 * If not, the effect is set to `timer` turns. If `timer` is 0, or if the
 * effect timer was 0, or if MON_TMD_FLG_NOTIFY is set in `flag`, then a
 * message is printed, unless MON_TMD_FLG_NOMESSAGE is set in `flag`.
 *
 * Give messages if the right flags are set.
 * Check if the monster is able to resist the spell.  Mark the lore.
 * Returns true if the monster was affected.
 * Return false if the monster was unaffected.
 */
static bool mon_set_timed(struct player *p, struct monster *mon, int ef_idx, int timer,
    u16b flag, bool id)
{
    bool check_resist = false;
    bool resisted = false;
    struct mon_timed_effect *effect;
    int m_note = 0;
    int old_timer;
    bool visible = false;

    my_assert((ef_idx >= 0) && (ef_idx < MON_TMD_MAX));
    effect = &effects[ef_idx];

    my_assert(mon);
    my_assert(mon->race);

    old_timer = mon->m_timed[ef_idx];
    if (p) visible = (mflag_has(p->mflag[mon->midx], MFLAG_VISIBLE) && !mon->unaware);

    /* No change */
    if (old_timer == timer) return false;

    /* Turning off, usually mention */
    if (timer == 0)
    {
        m_note = effect->message_end;
        flag |= MON_TMD_FLG_NOTIFY;
    }

    /* Turning on, usually mention */
    else if (old_timer == 0)
    {
        flag |= MON_TMD_FLG_NOTIFY;
        m_note = effect->message_begin;
        check_resist = true;
    }

    /* Different message for increases, but don't automatically mention. */
    else if (timer > old_timer)
    {
        m_note = effect->message_increase;
        check_resist = true;
    }

    /* Decreases don't get a message */

    /* Determine if the monster resisted or not, if appropriate */
    if (check_resist)
        resisted = mon_resist_effect(p, mon, ef_idx, timer, flag);

    if (resisted)
        m_note = MON_MSG_UNAFFECTED;
    else
        mon->m_timed[ef_idx] = timer;

    if (visible)
    {
        struct actor who_body;
        struct actor *who = &who_body;

        ACTOR_MONSTER(who, mon);
        update_health(who);
    }

    /* Update the visuals, as appropriate. */
    if (ef_idx == MON_TMD_SLEEP) update_monlist(mon);

    /*
     * Print a message if there is one, if the effect allows for it, and if
     * either the monster is visible, or we're trying to ID something
     */
    if (m_note && (visible || id) && !(flag & MON_TMD_FLG_NOMESSAGE) && (flag & MON_TMD_FLG_NOTIFY))
    {
        char m_name[NORMAL_WID];

        monster_desc(p, m_name, sizeof(m_name), mon, MDESC_IND_HID);
        add_monster_message(p, m_name, mon, m_note, true);
    }

    return !resisted;
}


/*
 * Increases the timed effect `ef_idx` by `timer`.
 *
 * Calculates the new timer, then passes that to mon_set_timed().
 * Note that each effect has a maximum number of turns it can be active for.
 * If this function would put an effect timer over that cap, it sets it for
 * that cap instead.
 *
 * Returns true if the monster's timer changed.
 */
bool mon_inc_timed(struct player *p, struct monster *mon, int ef_idx, int timer, u16b flag,
    bool id)
{
    struct mon_timed_effect *effect;

    my_assert((ef_idx >= 0) && (ef_idx < MON_TMD_MAX));
    effect = &effects[ef_idx];

    /* For negative amounts, we use mon_dec_timed instead */
    my_assert(timer > 0);

    /* Make it last for a mimimum # of turns if it is a new effect */
    if (!mon->m_timed[ef_idx] && (timer < 2)) timer = 2;

    /* New counter amount - prevent overflow */
    if (SHRT_MAX - timer < mon->m_timed[ef_idx])
        timer = SHRT_MAX;
    else
        timer += mon->m_timed[ef_idx];

    /* Reduce to max_timer if necessary */
    if (timer > effect->max_timer) timer = effect->max_timer;

    return mon_set_timed(p, mon, ef_idx, timer, flag, id);
}


/*
 * Decreases the timed effect `ef_idx` by `timer`.
 *
 * Calculates the new timer, then passes that to mon_set_timed().
 * If a timer would be set to a negative number, it is set to 0 instead.
 * Note that decreasing a timed effect should never fail.
 *
 * Returns true if the monster's timer changed.
 */
bool mon_dec_timed(struct player *p, struct monster *mon, int ef_idx, int timer, u16b flag,
    bool id)
{
    my_assert((ef_idx >= 0) && (ef_idx < MON_TMD_MAX));

    my_assert(timer > 0);

    /* Decreasing is never resisted */
    flag |= MON_TMD_FLG_NOFAIL;

    /* New counter amount */
    timer = mon->m_timed[ef_idx] - timer;
    if (timer < 0) timer = 0;

    return mon_set_timed(p, mon, ef_idx, timer, flag, id);
}


/*
 * Clears the timed effect `ef_idx`.
 *
 * Returns true if the monster's timer changed.
 */
bool mon_clear_timed(struct player *p, struct monster *mon, int ef_idx, u16b flag, bool id)
{
    my_assert((ef_idx >= 0) && (ef_idx < MON_TMD_MAX));

    if (!mon->m_timed[ef_idx]) return false;

    /* Clearing never fails */
    flag |= MON_TMD_FLG_NOFAIL;

    return mon_set_timed(p, mon, ef_idx, 0, flag, id);
}