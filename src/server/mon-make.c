/*
 * File: mon-make.c
 * Purpose: Monster creation / placement code.
 *
 * Copyright (c) 1997-2007 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2018 MAngband and PWMAngband Developers
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


static s16b alloc_race_size;
static alloc_entry *alloc_race_table;


static void init_race_allocs(void)
{
    int i;
    struct monster_race *race;
    alloc_entry *table;
    s16b *num = mem_zalloc(z_info->max_depth * sizeof(s16b));
    s16b *aux = mem_zalloc(z_info->max_depth * sizeof(s16b));

    /*** Analyze monster allocation info ***/

    /* Size of "alloc_race_table" */
    alloc_race_size = 0;

    /* Scan the monsters */
    for (i = 1; i < z_info->r_max; i++)
    {
        /* Get the i'th race */
        race = &r_info[i];

        /* Legal monsters */
        if (race->rarity)
        {
            /* Count the entries */
            alloc_race_size++;

            /* Group by level */
            num[race->level]++;
        }
    }

    /* Collect the level indexes */
    for (i = 1; i < z_info->max_depth; i++)
    {
        /* Group by level */
        num[i] += num[i - 1];
    }

    /* Paranoia */
    if (!num[0]) quit("No town monsters!");

    /*** Initialize monster allocation info ***/

    /* Allocate the alloc_race_table */
    alloc_race_table = mem_zalloc(alloc_race_size * sizeof(alloc_entry));

    /* Access the table entry */
    table = alloc_race_table;

    /* Scan the monsters */
    for (i = 1; i < z_info->r_max; i++)
    {
        /* Get the i'th race */
        race = &r_info[i];

        /* Count valid pairs */
        if (race->rarity)
        {
            int p, x, y, z;

            /* Extract the base level */
            x = race->level;

            /* Extract the base probability */
            p = (100 / race->rarity);

            /* Skip entries preceding our locale */
            y = ((x > 0)? num[x - 1]: 0);

            /* Skip previous entries at this locale */
            z = y + aux[x];

            /* Load the entry */
            table[z].index = i;
            table[z].level = x;
            table[z].prob1 = p;
            table[z].prob2 = p;
            table[z].prob3 = p;

            /* Another entry complete for this locale */
            aux[x]++;
        }
    }

    mem_free(aux);
    mem_free(num);
}


static void cleanup_race_allocs(void)
{
    mem_free(alloc_race_table);
}


static bool clear_vis(struct player *p, struct worldpos *wpos, int m)
{
    /* If he's not here, skip him */
    if (!COORDS_EQUAL(&p->wpos, wpos)) return false;

    /* Clear some fields */
    mflag_wipe(p->mflag[m]);
    p->mon_det[m] = 0;

    return true;
}


/*
 * Deletes a monster by index.
 *
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(struct chunk *c, int m_idx)
{
    int x, y, i;
    struct object *obj, *next;
    struct monster *mon;
    struct source who_body;
    struct source *who = &who_body;

    my_assert(m_idx > 0);

    mon = cave_monster(c, m_idx);
    source_monster(who, mon);

    /* Monster location */
    y = mon->fy;
    x = mon->fx;
    my_assert(square_in_bounds(c, y, x));

    /* Unique is dead */
    mon->race->lore.spawned = 0;

    /* Hack -- decrease the number of clones */
    if (mon->clone) c->num_clones--;

    /* Remove him from everybody's view */
    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);
        struct source *health_who = &p->upkeep->health_who;

        /* If he's not here, skip him */
        if (!clear_vis(p, &c->wpos, m_idx)) continue;

        /* Hack -- remove target monster */
        if (target_equals(p, who)) target_set_monster(p, NULL);

        /* Hack -- remove tracked monster */
        if (source_equal(health_who, who)) health_track(p->upkeep, NULL);

        /* Hack -- one less slave */
        if (p->id == mon->master) p->slaves--;
    }

    /* Monster is gone */
    c->squares[y][x].mon = 0;

    /* Delete objects */
    obj = mon->held_obj;
    while (obj)
    {
        next = obj->next;

        /* Preserve unseen artifacts */
        preserve_artifact(obj);

        /* Delete the object */
        object_delete(&obj);
        obj = next;
    }

    /* Delete mimicked objects */
    obj = mon->mimicked_obj;
    if (obj)
    {
        square_excise_object(c, y, x, obj);
        object_delete(&obj);
    }

    /* Delete mimicked features */
    if (mon->race->base == lookup_monster_base("feature mimic"))
        square_set_feat(c, y, x, mon->feat);

    /* Wipe the Monster */
    mem_free(mon->blow);
    memset(mon, 0, sizeof(struct monster));

    /* Count monsters */
    c->mon_cnt--;

    /* Visual update */
    square_light_spot(c, y, x);
}


/*
 * Deletes the monster, if any, at the given location.
 */
void delete_monster(struct chunk *c, int y, int x)
{
    /* Paranoia */
    if (!c) return;

    my_assert(square_in_bounds(c, y, x));

    /* Delete the monster (if any) */
    if (c->squares[y][x].mon > 0)
        delete_monster_idx(c, c->squares[y][x].mon);
}


/*
 * Move a monster from index i1 to index i2 in the monster list.
 */
static void compact_monsters_aux(struct chunk *c, int i1, int i2)
{
    int y, x, i;
    struct monster *mon, *newmon;
    struct object *obj;
    struct source mon1_body;
    struct source *mon1 = &mon1_body;
    struct source mon2_body;
    struct source *mon2 = &mon2_body;
    struct monster_blow *blows;

    /* Do nothing */
    if (i1 == i2) return;

    /* Old monster */
    mon = cave_monster(c, i1);
    source_monster(mon1, mon);
    y = mon->fy;
    x = mon->fx;

    /* New monster */
    newmon = cave_monster(c, i2);
    source_monster(mon2, newmon);

    /* Update the cave */
    c->squares[y][x].mon = i2;

    /* Update midx */
    mon->midx = i2;

    /* Repair objects being carried by monster */
    for (obj = mon->held_obj; obj; obj = obj->next)
        obj->held_m_idx = i2;

    /* Move mimicked objects */
    if (mon->mimicked_obj)
        mon->mimicked_obj->mimicking_m_idx = i2;

    /* Copy the visibility and los flags for the players */
    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);
        struct source *health_who = &p->upkeep->health_who;

        /* If he's not here, skip him */
        if (!COORDS_EQUAL(&p->wpos, &c->wpos)) continue;

        mflag_copy(p->mflag[i2], p->mflag[i1]);
        p->mon_det[i2] = p->mon_det[i1];

        /* Hack -- update the target */
        if (target_equals(p, mon1)) target_set_monster(p, mon2);

        /* Hack -- update the health bar */
        if (source_equal(health_who, mon1)) health_track(p->upkeep, mon2);
    }

    /* Hack -- move monster */
    blows = newmon->blow;
    memcpy(newmon, mon, sizeof(struct monster));
    newmon->blow = blows;
    if (!newmon->blow)
        newmon->blow = mem_zalloc(z_info->mon_blows_max * sizeof(struct monster_blow));
    memcpy(newmon->blow, mon->blow, z_info->mon_blows_max * sizeof(struct monster_blow));

    /* Hack -- wipe hole */
    mem_free(mon->blow);
    memset(mon, 0, sizeof(struct monster));
}


/*
 * Compacts and reorders the monster list.
 *
 * This function can be very dangerous, use with caution!
 *
 * When `num_to_compact` is 0, we just reorder the monsters into a more compact
 * order, eliminating any "holes" left by dead monsters. If `num_to_compact` is
 * positive, then we delete at least that many monsters and then reorder.
 * We try not to delete monsters that are high level or close to the player.
 * Each time we make a full pass through the monster list, if we haven't
 * deleted enough monsters, we relax our bounds a little to accept
 * monsters of a slightly higher level, and monsters slightly closer to
 * the player.
 */
void compact_monsters(struct chunk *c, int num_to_compact)
{
    int m_idx, num_compacted, iter;
    int max_lev, min_dis, chance;

    /* Message (only if compacting) */
    if (num_to_compact) plog("Compacting monsters...");

    /* Compact at least 'num_to_compact' monsters */
    for (num_compacted = 0, iter = 1; num_compacted < num_to_compact; iter++)
    {
        /* Get more vicious each iteration */
        max_lev = 5 * iter;

        /* Get closer each iteration */
        min_dis = 5 * (20 - iter);

        /* Check all the monsters */
        for (m_idx = 1; m_idx < cave_monster_max(c); m_idx++)
        {
            struct monster *mon = cave_monster(c, m_idx);

            /* Skip "dead" monsters */
            if (!mon->race) continue;

            /* High level monsters start out "immune" */
            if (mon->race->level > max_lev) continue;

            /* Ignore nearby monsters */
            if ((min_dis > 0) && (mon->cdis < min_dis)) continue;

            /* Saving throw chance */
            chance = 90;

            /* Only compact "Quest" Monsters in emergencies */
            if (rf_has(mon->race->flags, RF_QUESTOR) && (iter < 1000)) chance = 100;

            /* Try not to compact Unique Monsters */
            if (monster_is_unique(mon->race)) chance = 99;

            /* Monsters outside of the dungeon don't have much of a chance */
            if (c->wpos.depth == 0) chance = 70;

            /* All monsters get a saving throw */
            if (magik(chance)) continue;

            /* Delete the monster */
            delete_monster_idx(c, m_idx);

            /* Count the monster */
            num_compacted++;
        }
    }

    /* Excise dead monsters (backwards!) */
    for (m_idx = cave_monster_max(c) - 1; m_idx >= 1; m_idx--)
    {
        struct monster *mon = cave_monster(c, m_idx);

        /* Skip real monsters */
        if (mon->race) continue;

        /* Move last monster into open hole */
        compact_monsters_aux(c, cave_monster_max(c) - 1, m_idx);

        /* Compress "cave->mon_max" */
        c->mon_max--;
    }
}


/*
 * Deletes all the monsters when the player leaves the level.
 *
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 *
 * Note that we must delete the objects the monsters are carrying, but we
 * do nothing with mimicked objects.
 */
void wipe_mon_list(struct chunk *c)
{
    int m_idx, i;

    /* Delete all the monsters */
    for (m_idx = cave_monster_max(c) - 1; m_idx >= 1; m_idx--)
    {
        struct monster *mon = cave_monster(c, m_idx);
        struct object *held_obj;

        /* Skip dead monsters */
        if (!mon->race) continue;

        held_obj = mon->held_obj;

        /* Delete all the objects */
        while (held_obj)
        {
            struct object *next = held_obj->next;

            /* Go through all held objects and check for artifacts */
            preserve_artifact(held_obj);
            object_delete(&held_obj);
            held_obj = next;
        }

        /* Unique is dead */
        mon->race->lore.spawned = 0;

        /* Remove him from everybody's view */
        for (i = 1; i <= NumPlayers; i++)
        {
            struct player *p = player_get(i);

            clear_vis(p, &c->wpos, m_idx);

            /* Hack -- one less slave */
            if (p->id == mon->master) p->slaves--;
        }

        /* Monster is gone */
        c->squares[mon->fy][mon->fx].mon = 0;

        /* Wipe the Monster */
        mem_free(mon->blow);
        memset(mon, 0, sizeof(struct monster));
    }

    /* Reset "cave->mon_max" */
    c->mon_max = 1;

    /* Reset "mon_cnt" */
    c->mon_cnt = 0;

    /* Hack -- reset the number of clones */
    c->num_clones = 0;

    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);

        /* If he's not here, skip him */
        if (!COORDS_EQUAL(&p->wpos, &c->wpos)) continue;

        /* Hack -- no more target */
        target_set_monster(p, NULL);

        /* Hack -- no more tracking */
        health_track(p->upkeep, NULL);
    }
}


/*
 * Returns the index of a "free" monster, or 0 if no slot is available.
 *
 * This routine should almost never fail, but it *can* happen.
 * The calling code must check for and handle a 0 return.
 */
static s16b mon_pop(struct chunk *c)
{
    int m_idx;

    /* Normal allocation */
    if (cave_monster_max(c) < z_info->level_monster_max)
    {
        /* Get the next hole */
        m_idx = cave_monster_max(c);

        /* Expand the array */
        c->mon_max++;

        /* Count monsters */
        c->mon_cnt++;

        /* Return the index */
        return m_idx;
    }

    /* Recycle dead monsters if we've run out of room */
    for (m_idx = 1; m_idx < cave_monster_max(c); m_idx++)
    {
        struct monster *mon = cave_monster(c, m_idx);

        /* Skip live monsters */
        if (!mon->race)
        {
            /* Count monsters */
            c->mon_cnt++;

            /* Use this monster */
            return m_idx;
        }
    }

    /* Warn the player if no index is available */
    if (!ht_zero(&c->generated)) plog("Too many monsters!");

    /* Try not to crash */
    return 0;
}


/*
 * Apply a "monster restriction function" to the "monster allocation table".
 * This way, we can use get_mon_num() to get a level-appropriate monster that
 * satisfies certain conditions (such as belonging to a particular monster
 * family).
 */
void get_mon_num_prep(bool (*get_mon_num_hook)(struct monster_race *race))
{
    int i;

    /* Scan the allocation table */
    for (i = 0; i < alloc_race_size; i++)
    {
        struct monster_race *r;

        /* Get the entry */
        alloc_entry *entry = &alloc_race_table[i];

        /* Skip non-entries */
        r = &r_info[entry->index];
        if (!r->name)
        {
            entry->prob2 = 0;
            continue;
        }

        /* Accept monsters which pass the restriction, if any */
        if (!get_mon_num_hook || (*get_mon_num_hook)(r))
        {
            /* Accept this monster */
            entry->prob2 = entry->prob1;
        }

        /* Do not use this monster */
        else
        {
            /* Decline this monster */
            entry->prob2 = 0;
        }
    }
}


/*
 * Helper function for get_mon_num(). Scans the prepared monster allocation
 * table and picks a random monster. Returns the index of a monster in
 * `table`.
 */
static struct monster_race *get_mon_race_aux(long total, const alloc_entry *table)
{
    int i;

    /* Pick a monster */
    long value = randint0(total);

    /* Find the monster */
    for (i = 0; i < alloc_race_size; i++)
    {
        /* Found the entry */
        if (value < table[i].prob3) break;

        /* Decrement */
        value -= table[i].prob3;
    }

    return &r_info[table[i].index];
}


/* Scan all players on the level and see if at least one can find the unique */
static bool allow_unique_level(struct monster_race *race, struct worldpos *wpos)
{
    int i;

    /* Must not have spawned */
    if (race->lore.spawned) return false;

    /* Normal uniques cannot be generated in the wilderness */
    if (in_wild(wpos) && !special_level(wpos) && !rf_has(race->flags, RF_WILD_ONLY)) return false;

    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);
        struct monster_lore *lore = get_lore(p, race);

        /* Is the player on the level and did he kill the unique already ? */
        if ((is_dm_p(p) || !lore->pkills) && COORDS_EQUAL(&p->wpos, wpos))
        {
            /* One is enough */
            return true;
        }
    }

    /* Yeah but we need at least ONE */
    return false;
}


/*
 * Some dungeon types restrict the possible monsters.
 * Return TRUE is the monster is OK and FALSE otherwise
 */
static bool apply_rule(struct monster_race *race, struct dun_rule *rule)
{
    /* No restriction */
    if (rule->all) return true;

    /* Flags match */
    if (rf_is_inter(rule->flags, race->flags)) return true;

    /* Spell flags match */
    if (rsf_is_inter(rule->spell_flags, race->spell_flags)) return true;

    /* Race symbol matches */
    if (strchr(rule->sym, race->d_char)) return true;

    /* All checks failed */
    return false;
}


/*
 * Some dungeon types restrict the possible monsters.
 * Return the percent chance of generating a monster in a specific dungeon
 */
static int restrict_monster_to_dungeon(struct monster_race *race, struct worldpos *wpos)
{
    struct worldpos dpos;
    struct location *dungeon;
    int i, percents = 0;

    /* Get the dungeon */
    COORDS_SET(&dpos, wpos->wy, wpos->wx, 0);
    dungeon = get_dungeon(&dpos);

    /* No dungeon here, allow everything */
    if (!dungeon || !wpos->depth) return 100;

    /* Process all rules */
    for (i = 0; i < 5; i++)
    {
        struct dun_rule *rule = &dungeon->rules[i];

        /* Break if not valid */
        if (!rule->percent) break;

        /* Apply the rule */
        if (apply_rule(race, rule)) percents += rule->percent;
    }

    /* Return percentage */
    return percents;
}


/* Checks if a monster race can be generated at that location */
static bool allow_race(struct monster_race *race, struct worldpos *wpos)
{
    /* Only one copy of a a unique must be around at the same time */
    if (monster_is_unique(race) && !allow_unique_level(race, wpos))
        return false;

    /* Some monsters never appear out of depth */
    if (rf_has(race->flags, RF_FORCE_DEPTH) && (race->level > wpos->depth))
        return false;

    /* Some monsters never appear out of their dungeon/town (normal servers) */
    if (!cfg_diving_mode && race->wpos &&
        !((race->wpos->wy == wpos->wy) && (race->wpos->wx == wpos->wx)))
    {
        return false;
    }

    /* Some monsters only appear in the wilderness */
    if (rf_has(race->flags, RF_WILD_ONLY) && !in_wild(wpos))
        return false;

    /* Handle PWMAngband base monsters */
    if (rf_has(race->flags, RF_PWMANG_BASE) && !cfg_base_monsters)
        return false;

    /* Handle PWMAngband extra monsters */
    if (rf_has(race->flags, RF_PWMANG_EXTRA) && !cfg_extra_monsters)
        return false;

    return true;
}


/*
 * Chooses a monster race that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "monster allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" monster, in
 * a relatively efficient manner.
 *
 * Note that "town" monsters will *only* be created in the towns, and
 * "normal" monsters will *never* be created in the towns.
 *
 * There is a small chance (1/50) of "boosting" the given depth by
 * a small amount (up to four levels), except in the towns.
 *
 * It is (slightly) more likely to acquire a monster of the given level
 * than one of a lower level.  This is done by choosing several monsters
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no monsters are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
struct monster_race *get_mon_num(struct chunk *c, int level, bool summon)
{
    int i, p;
    long total;
    struct monster_race *race;
    alloc_entry *table = alloc_race_table;

    /* No monsters in the base town (no_recall servers) */
    if ((cfg_diving_mode == 2) && in_base_town(&c->wpos)) return (0);

    /* No monsters in dynamically generated towns */
    if (dynamic_town(&c->wpos)) return (0);

    /* No monsters on special towns */
    if (special_town(&c->wpos)) return (0);

    /* Limit the total number of townies */
    if (in_town(&c->wpos) && (cfg_max_townies != -1) && (cave_monster_count(c) >= cfg_max_townies))
        return (0);

    /* Occasionally produce a nastier monster in the dungeon */
    if ((c->wpos.depth > 0) && one_in_(z_info->ood_monster_chance))
        level += MIN(level / 4 + 2, z_info->ood_monster_amount);

    total = 0L;

    /* Process probabilities */
    for (i = 0; i < alloc_race_size; i++)
    {
        /* Monsters are sorted by depth */
        if (table[i].level > level) break;

        /* Default */
        table[i].prob3 = 0;

        /* No town monsters outside of towns */
        if (!in_town(&c->wpos) && (table[i].level <= 0)) continue;

        /* Get the chosen monster */
        race = &r_info[table[i].index];

        /* Hack -- check if monster race can be generated at that location */
        if (!allow_race(race, &c->wpos)) continue;

        /* Accept */
        table[i].prob3 = table[i].prob2;

        /* Hack -- some dungeon types restrict the possible monsters (except for summons) */
        p = (summon? 100: restrict_monster_to_dungeon(race, &c->wpos));
        table[i].prob3 = table[i].prob3 * p / 100;
        if (p && table[i].prob2 && !table[i].prob3) table[i].prob3 = 1;

        /* Total */
        total += table[i].prob3;
    }

    /* No legal monsters */
    if (total <= 0) return NULL;

    /* Pick a monster */
    race = get_mon_race_aux(total, table);

    /* Always try for a "harder" monster if too weak */
    if (race->level < (level / 2))
    {
        struct monster_race *old = race;

        /* Pick a new monster */
        race = get_mon_race_aux(total, table);

        /* Keep the deepest one */
        if (race->level < old->level) race = old;
    }

    /* Always try for a "harder" monster deep in the dungeon */
    if (level >= 100)
    {
        struct monster_race *old = race;

        /* Pick a new monster */
        race = get_mon_race_aux(total, table);

        /* Keep the deepest one */
        if (race->level < old->level) race = old;
    }

    /* Try for a "harder" monster once (50%) or twice (10%) */
    p = randint0(100);
    if (p < 60)
    {
        struct monster_race *old = race;

        /* Pick a new monster */
        race = get_mon_race_aux(total, table);

        /* Keep the deepest one */
        if (race->level < old->level) race = old;
    }

    /* Try for a "harder" monster twice (10%) */
    if (p < 10)
    {
        struct monster_race *old = race;

        /* Pick a new monster */
        race = get_mon_race_aux(total, table);

        /* Keep the deepest one */
        if (race->level < old->level) race = old;
    }

    /* Result */
    return race;
}


/*
 * Chooses a monster race for rings of polymorphing that seems "appropriate" to the given level.
 * This function uses most of the code from get_mon_num(), except depth checks.
 */
struct monster_race *get_mon_num_poly(int level)
{
    int i, p;
    long total;
    struct monster_race *race;
    alloc_entry *table = alloc_race_table;

    /* Occasionally produce a nastier monster */
    if (one_in_(z_info->ood_monster_chance))
        level += MIN(level / 4 + 2, z_info->ood_monster_amount);

    total = 0L;

    /* Process probabilities */
    for (i = 0; i < alloc_race_size; i++)
    {
        /* Monsters are sorted by depth */
        if (table[i].level > level) break;

        /* Default */
        table[i].prob3 = 0;

        /* Get the chosen monster */
        race = &r_info[table[i].index];

        /* Skip uniques */
        if (monster_is_unique(race)) continue;

        /* Handle PWMAngband base monsters */
        if (rf_has(race->flags, RF_PWMANG_BASE) && !cfg_base_monsters)
            continue;

        /* Handle PWMAngband extra monsters */
        if (rf_has(race->flags, RF_PWMANG_EXTRA) && !cfg_extra_monsters)
            continue;

        /* Accept */
        table[i].prob3 = table[i].prob2;

        /* Total */
        total += table[i].prob3;
    }

    /* No legal monsters */
    if (total <= 0) return NULL;

    /* Pick a monster */
    race = get_mon_race_aux(total, table);

    /* Try for a "harder" monster once (50%) or twice (10%) */
    p = randint0(100);
    if (p < 60)
    {
        struct monster_race *old = race;

        /* Pick a new monster */
        race = get_mon_race_aux(total, table);

        /* Keep the deepest one */
        if (race->level < old->level) race = old;
    }

    /* Try for a "harder" monster twice (10%) */
    if (p < 10)
    {
        struct monster_race *old = race;

        /* Pick a new monster */
        race = get_mon_race_aux(total, table);

        /* Keep the deepest one */
        if (race->level < old->level) race = old;
    }

    /* Result */
    return race;
}


/*
 * Return the number of things dropped by a monster.
 *
 * race is the monster race.
 * maximize should be set to false for a random number, true to find out the maximum count.
 */
int mon_create_drop_count(const struct monster_race *race, bool maximize)
{
    int number = 0;

    if (maximize)
    {
        if (rf_has(race->flags, RF_DROP_20)) number++;
        if (rf_has(race->flags, RF_DROP_40)) number++;
        if (rf_has(race->flags, RF_DROP_60)) number++;
        if (rf_has(race->flags, RF_DROP_4)) number += 6;
        if (rf_has(race->flags, RF_DROP_3)) number += 4;
        if (rf_has(race->flags, RF_DROP_2)) number += 3;
        if (rf_has(race->flags, RF_DROP_1)) number++;
    }
    else
    {
        if (rf_has(race->flags, RF_DROP_20) && magik(20)) number++;
        if (rf_has(race->flags, RF_DROP_40) && magik(40)) number++;
        if (rf_has(race->flags, RF_DROP_60) && magik(60)) number++;
        if (rf_has(race->flags, RF_DROP_4)) number += rand_range(2, 6);
        if (rf_has(race->flags, RF_DROP_3)) number += rand_range(2, 4);
        if (rf_has(race->flags, RF_DROP_2)) number += rand_range(1, 3);
        if (rf_has(race->flags, RF_DROP_1)) number++;
    }

    return number;
}


static bool mon_drop_carry(struct player *p, struct object **obj_address, struct monster *mon,
    byte origin, int num, quark_t quark, bool ok)
{
    /* Set origin details */
    set_origin(*obj_address, origin, mon->wpos.depth, mon->race);
    (*obj_address)->number = num;
    (*obj_address)->note = quark;

    if (object_has_standard_to_h(*obj_address)) (*obj_address)->known->to_h = 1;
    if (object_flavor_is_aware(p, *obj_address)) object_id_set_aware(*obj_address);

    /* Try to carry */
    if (ok && monster_carry(mon, *obj_address, true)) return true;
    if ((*obj_address)->artifact && (*obj_address)->artifact->created)
        (*obj_address)->artifact->created--;
    object_delete(obj_address);
    return false;
}


/*
 * Creates a specific monster's drop, including any drops specified
 * in the monster.txt file.
 */
static bool mon_create_drop(struct player *p, struct chunk *c, struct monster *mon, byte origin)
{
    struct monster_drop *drop;
    bool great, good, gold_ok, item_ok;
    bool extra_roll = false;
    bool any = false;
    int number = 0, level, j, monlevel;
    struct object *obj;
    quark_t quark = 0;

    my_assert(mon);

    great = rf_has(mon->race->flags, RF_DROP_GREAT);
    good = (rf_has(mon->race->flags, RF_DROP_GOOD) || great);
    gold_ok = !rf_has(mon->race->flags, RF_ONLY_ITEM);
    item_ok = !rf_has(mon->race->flags, RF_ONLY_GOLD);

    /* Hack -- inscribe items that a unique drops */
    if (monster_is_unique(mon->race)) quark = quark_add(mon->race->name);

    /* Determine how much we can drop */
    number = mon_create_drop_count(mon->race, false);

    /* Give added bonus for unique monters */
    monlevel = mon->level;
    if (monster_is_unique(mon->race))
    {
        monlevel = MIN(monlevel + 15, monlevel * 2);
        extra_roll = true;
    }

    /*
     * Take the best of (average of monster level and current depth)
     * and (monster level) - to reward fighting OOD monsters
     */
    level = MAX((monlevel + object_level(&c->wpos)) / 2, monlevel);
    level = MIN(level, 100);

    /* Morgoth currently drops all artifacts with the QUEST_ART flag */
    if (mon->race->base == lookup_monster_base("Morgoth"))
    {
        /* Search all the artifacts */
        for (j = 0; j < z_info->a_max; j++)
        {
            struct artifact *art = &a_info[j];
            struct object_kind *kind = lookup_kind(art->tval, art->sval);
            if (!kf_has(kind->kind_flags, KF_QUEST_ART)) continue;

            /* Allocate by hand, prep, apply magic */
            obj = object_new();
            object_prep(p, obj, kind, mon->level, RANDOMISE);
            obj->artifact = art;
            copy_artifact_data(obj, obj->artifact);
            obj->artifact->created++;
            if (p)
            {
                if (!ht_zero(&c->generated))
                    set_artifact_info(p, obj, ARTS_GENERATED);
                else
                    p->art_info[obj->artifact->aidx] += ARTS_CREATED;
            }

            if (mon_drop_carry(p, &obj, mon, origin, 1, quark, true)) any = true;
        }
    }

    /* Specified drops */
    for (drop = mon->race->drops; drop; drop = drop->next)
    {
        bool ok = false;
        bool drop_nazgul = (tval_is_ring_k(drop->kind) &&
            (drop->kind->sval == lookup_sval(TV_RING, "Black Ring of Power")));
        int num = randint0(drop->max - drop->min) + drop->min;

        if ((unsigned int)randint0(100) >= drop->percent_chance) continue;

        /* Allocate by hand, prep, apply magic */
        obj = object_new();
        object_prep(p, obj, drop->kind, level, RANDOMISE);

        /* Hack -- "Nine rings for mortal men doomed to die" */
        if (drop_nazgul)
        {
            /* Only if allowed */
            if (p && cfg_random_artifacts)
            {
                int i;

                /* Make it a randart */
                for (i = z_info->a_max; i < z_info->a_max + 9; i++)
                {
                    /* Attempt to change the object into a random artifact */
                    if (!create_randart_drop(p, c, &obj, i, false)) continue;

                    /* Success */
                    ok = true;
                    break;
                }
            }
        }

        /* Drop an object */
        else
        {
            apply_magic(p, c, obj, level, true, good, great, extra_roll);
            ok = true;
        }

        if (mon_drop_carry(p, &obj, mon, origin, num, quark, ok)) any = true;
    }

    /* Make some objects */
    for (j = 0; j < number; j++)
    {
        if (gold_ok && (!item_ok || magik(50)))
            obj = make_gold(p, level, "any");
        else
        {
            obj = make_object(p, c, level, good, great, extra_roll, NULL, 0);
            if (!obj) continue;
            obj->note = quark;
        }

        if (mon_drop_carry(p, &obj, mon, origin, obj->number, obj->note, true)) any = true;
    }

    return any;
}


/*
 * Creates monster drops, if not yet created
 */
void mon_create_drops(struct player *p, struct chunk *c, struct monster *mon)
{
    /* Create monster drops, if not yet created */
    if (mon->origin)
    {
        mon_create_drop(p, c, mon, mon->origin);
        mon->origin = ORIGIN_NONE;
    }
}


/*
 * Creates the object a mimic is imitating.
 */
void mon_create_mimicked_object(struct player *p, struct chunk *c, struct monster *mon, int index)
{
    struct object *obj;
    struct object_kind *kind = NULL;
    bool dummy = true;

    /* Hack -- random mimics */
    if (mon->race->base == lookup_monster_base("random mimic"))
    {
        /* Random symbol from object set */
        while (1)
        {
            /* Select a random object */
            mon->mimicked_k_idx = randint0(z_info->k_max - 1) + 1;

            kind = &k_info[mon->mimicked_k_idx];

            /* Skip non-entries */
            if (!kind->name) continue;

            /* Skip empty entries */
            if (!kind->d_attr || !kind->d_char) continue;

            /* Skip non-kinds */
            if (!kind->tval) continue;

            /* Skip insta arts! */
            if (kf_has(kind->kind_flags, KF_INSTA_ART) || kf_has(kind->kind_flags, KF_QUEST_ART))
                continue;

            /* Force race attr */
            if (kind->d_attr != mon->race->d_attr) continue;

            /* Success */
            break;
        }
    }

    /* Hack -- object mimics */
    else if (mon->race->mimic_kinds)
    {
        struct monster_mimic *mimic_kind;
        int i = 1;

        /* Pick a random object kind to mimic */
        for (mimic_kind = mon->race->mimic_kinds; mimic_kind; mimic_kind = mimic_kind->next, i++)
        {
            if (one_in_(i)) kind = mimic_kind->kind;
        }
    }

    if (!kind) return;

    if (tval_is_money_k(kind))
        obj = make_gold(p, object_level(&c->wpos), kind->name);
    else
    {
        obj = object_new();
        object_prep(p, obj, kind, mon->race->level, RANDOMISE);
        apply_magic(p, c, obj, mon->race->level, false, false, false, false);
        if (object_has_standard_to_h(obj)) obj->known->to_h = 1;
        if (object_flavor_is_aware(p, obj)) object_id_set_aware(obj);
        obj->number = 1;
    }

    set_origin(obj, ORIGIN_DROP_MIMIC, mon->wpos.depth, NULL);
    obj->mimicking_m_idx = index;
    mon->mimicked_obj = obj;

    /* Put the object on the floor if it goes, otherwise no mimicry */
    if (!floor_carry(p, c, mon->fy, mon->fx, obj, &dummy))
    {
        /* Clear the mimicry */
        obj->mimicking_m_idx = 0;
        mon->mimicked_obj = NULL;
        mon->camouflage = false;

        /* Give the object to the monster if appropriate */
        /* Otherwise delete the mimicked object */
        if (!rf_has(mon->race->flags, RF_MIMIC_INV) || !monster_carry(mon, obj, true))
            object_delete(&obj);
    }
}


/*
 * Attempts to place a copy of the given monster at the given position in
 * the dungeon.
 *
 * All of the monster placement routines eventually call this function. This
 * is what actually puts the monster in the dungeon (i.e., it notifies the cave
 * and sets the monsters position). The dungeon loading code also calls this
 * function directly.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.) The dungeon loading code calls this with origin = 0,
 * which prevents the monster's drops from being generated again.
 *
 * Returns the m_idx of the newly copied monster, or 0 if the placement fails.
 */
s16b place_monster(struct player *p, struct chunk *c, struct monster *mon, byte origin)
{
    s16b m_idx;
    struct monster *new_mon;
    int y = mon->fy;
    int x = mon->fx;

    /* Paranoia: cave can be NULL (wilderness) */
    if (!c) return 0;

    my_assert(square_in_bounds(c, y, x));
    my_assert(!square_monster(c, y, x));

    /* Get a new record */
    m_idx = mon_pop(c);
    if (!m_idx) return 0;

    /* Copy the monster */
    new_mon = cave_monster(c, m_idx);
    memcpy(new_mon, mon, sizeof(struct monster));
    new_mon->blow = mem_zalloc(z_info->mon_blows_max * sizeof(struct monster_blow));
    memcpy(new_mon->blow, mon->blow, z_info->mon_blows_max * sizeof(struct monster_blow));

    /* Set the ID */
    new_mon->midx = m_idx;

    /* Set the location */
    c->squares[y][x].mon = new_mon->midx;
    my_assert(square_monster(c, y, x) == new_mon);

    /* Hack -- increase the number of clones */
    if (new_mon->race->ridx && new_mon->clone) c->num_clones++;

    /* Done */
    if (!origin) return m_idx;

    /* The dungeon is ready: create the monster's drop, if any */
    if (!ht_zero(&c->generated))
        mon_create_drop(p, c, new_mon, origin);

    /* The dungeon is not ready: just set origin for later creation */
    else
        new_mon->origin = origin;

    /* Make mimics start mimicking */
    mon_create_mimicked_object(p, c, new_mon, m_idx);

    /* Hack -- feature mimics */
    if (new_mon->race->base == lookup_monster_base("feature mimic"))
    {
        /* Save original feature */
        new_mon->feat = c->squares[y][x].feat;

        /* Place corresponding feature under the monster */
        switch (new_mon->race->d_char)
        {
            /* Place a door */
            case '+':
            {
                /* Push objects off the grid */
                if (square_object(c, y, x)) push_object(p, c, y, x);

                /* Create a door */
                square_close_door(c, y, x);

                break;
            }

            /* Place an up staircase */
            case '<':
            {
                /* Push objects off the grid */
                if (square_object(c, y, x)) push_object(p, c, y, x);

                /* Create a staircase */
                square_add_stairs(c, y, x, FEAT_LESS);

                break;
            }

            /* Place a down staircase */
            case '>':
            {
                /* Push objects off the grid */
                if (square_object(c, y, x)) push_object(p, c, y, x);

                /* Create a staircase */
                square_add_stairs(c, y, x, FEAT_MORE);

                break;
            }
        }
    }

    /* Result */
    return m_idx;
}


/*
 * Calculates hp for a monster. This function assumes that the Rand_normal
 * function has limits of +/- 4x std_dev. If that changes, this function
 * will become inaccurate.
 *
 * race is the race of the monster in question.
 * hp_aspect is the hp calc we want (min, max, avg, random).
 */
int mon_hp(const struct monster_race *race, aspect hp_aspect)
{
    int std_dev = (((race->avg_hp * 10) / 8) + 5) / 10;

    if (race->avg_hp > 1) std_dev++;

    switch (hp_aspect)
    {
        case MINIMISE:
            return (race->avg_hp - (4 * std_dev));
        case MAXIMISE:
            return (race->avg_hp + (4 * std_dev));
        case AVERAGE:
            return race->avg_hp;
        case RANDOMISE:
            return Rand_normal(race->avg_hp, std_dev);
    }

    return 0;
}


int sleep_value(const struct monster_race *race)
{
    if (race->sleep) return (race->sleep * 2 + randint1(race->sleep * 10));
    return 0;
}


/*
 * Attempts to place a monster of the given race at the given location.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 *
 * To give the player a sporting chance, some especially dangerous
 * monsters are marked as "FORCE_SLEEP" in monster.txt, which will
 * cause them to be placed with low energy. This helps ensure that
 * if such a monster suddenly appears in line-of-sight (due to a
 * summon, for instance), the player gets a chance to move before
 * they do.
 *
 * This routine refuses to place out-of-depth "FORCE_DEPTH" monsters.
 *
 * This is the only function which may place a monster in the dungeon,
 * except for the savefile loading code, which calls place_monster()
 * directly.
 *
 * mon_flag = (MON_ASLEEP, MON_CLONE)
 */
static bool place_new_monster_one(struct player *p, struct chunk *c, int y, int x,
    struct monster_race *race, byte mon_flag, byte origin)
{
    int i, mlvl;
    struct monster *mon;
    struct monster monster_body;
    byte mspeed;

    my_assert(square_in_bounds(c, y, x));
    my_assert(race && race->name);

    /* Not where monsters already are */
    if (square_monster(c, y, x)) return false;

    /* Not where players already are */
    if (square_isplayer(c, y, x)) return false;

    /* Prevent monsters from being placed where they cannot walk, but allow other feature types */
    if (!square_is_monster_walkable(c, y, x)) return false;

    /* No creation on glyph of warding */
    if (square_iswarded(c, y, x)) return false;

    /* Hack -- no creation inside houses */
    if (town_area(&c->wpos) && square_isvault(c, y, x)) return false;

    /* Hack -- check if monster race can be generated at that location */
    if (!allow_race(race, &c->wpos)) return false;

    /* Get local monster */
    mon = &monster_body;

    /* Clean out the monster */
    memset(mon, 0, sizeof(struct monster));

    /* Save the race */
    mon->race = race;

    /* Enforce sleeping if needed */
    if (mon_flag & MON_ASLEEP)
        mon->m_timed[MON_TMD_SLEEP] = sleep_value(race);

    /* Uniques get a fixed amount of HP */
    if (monster_is_unique(race))
        mon->maxhp = race->avg_hp;
    else
    {
        mon->maxhp = mon_hp(race, RANDOMISE);
        mon->maxhp = MAX(mon->maxhp, 1);
    }

    /* Extract the monster base values */
    mspeed = race->speed;
    mon->ac = race->ac;
    mon->blow = mem_zalloc(z_info->mon_blows_max * sizeof(struct monster_blow));
    for (i = 0; i < z_info->mon_blows_max; i++)
    {
        mon->blow[i].method = race->blow[i].method;
        mon->blow[i].effect = race->blow[i].effect;
        mon->blow[i].dice.dice = race->blow[i].dice.dice;
        mon->blow[i].dice.sides = race->blow[i].dice.sides;
    }
    mon->level = race->level;

    /* Deep monsters are more powerful */
    if (c->wpos.depth > race->level)
    {
        /* Calculate a new level (up to +20) */
        mon->level = race->level + ((race->level > 20)? 20: race->level) *
            (c->wpos.depth - race->level) / (z_info->max_depth - 1 - race->level);

        for (i = 0; i < (mon->level - race->level); i++)
        {
            /* Increase hp */
            mon->maxhp += randint0(2 + race->avg_hp / 20);

            /* Increase speed */
            mspeed += randint0(2);

            /* Increase ac */
            mon->ac += randint0(2 + race->ac / 50);
        }

        /* Increase melee damage */
        for (i = 0; i < z_info->mon_blows_max; i++)
        {
            mon->blow[i].dice.dice = (race->blow[i].dice.dice * (mon->level - race->level) * 3) % 200;
            mon->blow[i].dice.dice = ((mon->blow[i].dice.dice >= 100)?
                race->blow[i].dice.dice + 1: race->blow[i].dice.dice);
            mon->blow[i].dice.dice += race->blow[i].dice.dice * (mon->level - race->level) * 3 / 200;
            mon->blow[i].dice.sides = (race->blow[i].dice.sides * (mon->level - race->level) * 3) % 200;
            mon->blow[i].dice.sides = ((mon->blow[i].dice.sides >= 100)?
                race->blow[i].dice.sides + 1: race->blow[i].dice.sides);
            mon->blow[i].dice.sides += race->blow[i].dice.sides * (mon->level - race->level) * 3 / 200;
        }
    }

    /* And start out fully healthy */
    mon->hp = mon->maxhp;

    /* Extract the monster base speed */
    mon->mspeed = mspeed;

    /* Give a random starting energy */
    mon->energy = randint0(move_energy(0) >> 1);

    /* Force monster to wait for player */
    if (rf_has(race->flags, RF_FORCE_SLEEP))
        mon->energy = randint0(move_energy(0) >> 4);

    /* Radiate light? */
    if (rf_has(race->flags, RF_HAS_LIGHT)) update_view_all(&c->wpos, 0);

    /* Is this obviously a monster? (Mimics etc. aren't) */
    if (rf_has(race->flags, RF_UNAWARE))
        mon->camouflage = true;
    else
        mon->camouflage = false;

    /* Unique has spawned */
    race->lore.spawned = 1;

    /* Hack -- increase the number of clones */
    if (mon_flag & MON_CLONE) mon->clone = 1;

    /* Place the monster in the dungeon */
    mon->old_fy = mon->fy = y;
    mon->old_fx = mon->fx = x;
    memcpy(&mon->wpos, &c->wpos, sizeof(struct worldpos));
    if (!place_monster(p, c, mon, origin)) return false;

    /* Add to level feeling */
    c->mon_rating += race->level * race->level;

    /* Check out-of-depth-ness */
    mlvl = monster_level(&c->wpos);
    if (race->level > mlvl)
    {
        /* Boost rating by power per 10 levels OOD */
        c->mon_rating += (race->level - mlvl) * race->level * race->level / 10;
    }

    for (i = 1; i <= NumPlayers; i++)
        clear_vis(player_get(i), &c->wpos, c->squares[y][x].mon);

    /* Update the monster */
    update_mon(square_monster(c, y, x), c, true);

    /* Success */
    return true;
}


/*
 * Maximum size of a group of monsters
 */
#define GROUP_MAX   25


/*
 * Attempts to place a group of monsters of race `race` around
 * the given location. The number of monsters to place is `total`.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 *
 * mon_flag = (MON_ASLEEP)
 */
static bool place_new_monster_group(struct player *p, struct chunk *c, int y, int x,
    struct monster_race *race, byte mon_flag, int total, byte origin)
{
    int n, i;
    int hack_n;

    /* x and y coordinates of the placed monsters */
    byte hack_y[GROUP_MAX];
    byte hack_x[GROUP_MAX];

    my_assert(race);

    /* Start on the monster */
    hack_n = 1;
    hack_x[0] = x;
    hack_y[0] = y;

    /* Puddle monsters, breadth first, up to total */
    for (n = 0; (n < hack_n) && (hack_n < total); n++)
    {
        /* Grab the location */
        int hx = hack_x[n];
        int hy = hack_y[n];

        /* Check each direction, up to total */
        for (i = 0; (i < 8) && (hack_n < total); i++)
        {
            int mx = hx + ddx_ddd[i];
            int my = hy + ddy_ddd[i];

            /* Walls and Monsters block flow */
            if (!square_isemptyfloor(c, my, mx)) continue;

            /* Attempt to place another monster */
            if (place_new_monster_one(p, c, my, mx, race, mon_flag, origin))
            {
                /* Add it to the "hack" set */
                hack_y[hack_n] = my;
                hack_x[hack_n] = mx;
                hack_n++;
            }
        }
    }

    /* Success */
    return true;
}


/* Maximum distance from center for a group of monsters */
#define GROUP_DISTANCE 5


static struct monster_base *place_monster_base = NULL;


/*
 * Predicate function for get_mon_num_prep
 * Check to see if the monster race has the same base as
 * place_monster_base.
 */
static bool place_monster_base_okay(struct monster_race *race)
{
    my_assert(place_monster_base);
    my_assert(race);

    /* Check if it matches */
    if (race->base != place_monster_base) return false;

    /* No uniques */
    if (monster_is_unique(race)) return false;

    return true;
}


/*
 * Helper function to place monsters that appear as friends or escorts
 */
static bool place_friends(struct player *p, struct chunk *c, int y, int x,
    struct monster_race *race, struct monster_race *friends_race, int total, byte mon_flag,
    byte origin)
{
    int extra_chance;

    /* Find the difference between current dungeon depth and monster level */
    int level_difference = c->wpos.depth - friends_race->level + 5;

    /* Handle unique monsters */
    bool is_unique = monster_is_unique(friends_race);

    /* Make sure the unique hasn't been killed already */
    if (is_unique)
        total = (allow_unique_level(friends_race, &c->wpos)? 1: 0);

    /* More than 4 levels OoD, no groups allowed */
    if ((level_difference <= 0) && !is_unique) return false;

    /* Reduce group size within 5 levels of natural depth */
    if ((level_difference < 10) && !is_unique)
    {
        extra_chance = (total * level_difference) % 10;
        total = total * level_difference / 10;

        /*
         * Instead of flooring the group value, we use the decimal place
         * as a chance of an extra monster
         */
        if (randint0(10) > extra_chance) total += 1;
    }

    /* No monsters in this group */
    if (total > 0)
    {
        int j, nx = 0, ny = 0;
        bool success;

        /* Handle friends same as original monster */
        if (race->ridx == friends_race->ridx)
            return place_new_monster_group(p, c, y, x, race, mon_flag, total, origin);

        /* Find a nearby place to put the other groups */
        for (j = 0; j < 50; j++)
        {
            if (!scatter(c, &ny, &nx, y, x, GROUP_DISTANCE, false)) continue;
            if (!square_isopen(c, ny, nx)) continue;

            /* Place the monsters */
            success = place_new_monster_one(p, c, ny, nx, friends_race, mon_flag, origin);
            if (total > 1)
            {
                success = place_new_monster_group(p, c, ny, nx, friends_race, mon_flag, total,
                    origin);
            }

            return success;
        }
    }

    return false;
}


/*
 * Attempts to place a monster of the given race at the given location.
 *
 * Note that certain monsters are placed with a large group of
 * identical or similar monsters. However, if `group_okay` is false,
 * then such monsters are placed by themselves.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 *
 * mon_flag = (MON_ASLEEP, MON_GROUP, MON_CLONE)
 */
bool place_new_monster(struct player *p, struct chunk *c, int y, int x,
    struct monster_race *race, byte mon_flag, byte origin)
{
    struct monster_friends *friends;
    struct monster_friends_base *friends_base;
    int total;

    my_assert(c);
    my_assert(race);

    /* Place one monster, or fail */
    if (!place_new_monster_one(p, c, y, x, race, mon_flag & ~(MON_GROUP), origin))
        return false;

    /* We're done unless the group flag is set */
    if (!(mon_flag & MON_GROUP)) return true;
    mon_flag &= ~(MON_GROUP | MON_CLONE);

    /* Go through friends flags */
    for (friends = race->friends; friends; friends = friends->next)
    {
        if ((unsigned int)randint0(100) >= friends->percent_chance)
            continue;

        /* Calculate the base number of monsters to place */
        total = damroll(friends->number_dice, friends->number_side);

        place_friends(p, c, y, x, race, friends->race, total, mon_flag, origin);
    }

    /* Go through the friends_base flags */
    for (friends_base = race->friends_base; friends_base; friends_base = friends_base->next)
    {
        struct monster_race *friends_race;

        /* Check if we pass chance for the monster appearing */
        if ((unsigned int)randint0(100) >= friends_base->percent_chance)
            continue;

        total = damroll(friends_base->number_dice, friends_base->number_side);

        /* Set the escort index base */
        place_monster_base = friends_base->base;

        /* Prepare allocation table */
        get_mon_num_prep(place_monster_base_okay);

        /* Pick a random race */
        friends_race = get_mon_num(c, race->level, false);

        /* Reset allocation table */
        get_mon_num_prep(NULL);

        /* Handle failure */
        if (!friends_race) break;

        place_friends(p, c, y, x, race, friends_race, total, mon_flag, origin);
    }

    /* Success */
    return true;
}


/*
 * Picks a monster race, makes a new monster of that race, then attempts to
 * place it in the dungeon. The monster race chosen will be appropriate for
 * dungeon level equal to `depth`.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * If `group_okay` is true, we allow the placing of a group, if the chosen
 * monster appears with friends or an escort.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 *
 * Returns true if we successfully place a monster.
 *
 * mon_flag = (MON_ASLEEP, MON_GROUP)
 */
bool pick_and_place_monster(struct player *p, struct chunk *c, int y, int x, int depth,
    byte mon_flag, byte origin)
{
    /* Pick a monster race */
    struct monster_race *race = get_mon_num(c, depth, false);

    if (race) return place_new_monster(p, c, y, x, race, mon_flag, origin);
    return false;
}


/*
 * Picks a monster race, makes a new monster of that race, then attempts to
 * place it in the dungeon at least `dis` away from the player. The monster
 * race chosen will be appropriate for dungeon level equal to `depth`.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * Returns true if we successfully place a monster.
 *
 * mon_flag = (MON_ASLEEP)
 */
bool pick_and_place_distant_monster(struct player *p, struct chunk *c, int dis, byte mon_flag)
{
    int y = 0, x = 0;
    int attempts_left = 10000;

    my_assert(c);

    /* Find a legal, distant, unoccupied, space */
    while (--attempts_left)
    {
        int i, d, min_dis = 999;

        /* Pick a location */
        y = randint0(c->height);
        x = randint0(c->width);

        /* Require "naked" floor grid */
        if (!square_isempty(c, y, x)) continue;

        /* Do not put random monsters in marked rooms. */
        if (square_ismon_restrict(c, y, x)) continue;

        /* Get min distance from all players on the level */
        for (i = 1; i <= NumPlayers; i++)
        {
            struct player *player = player_get(i);

            /* Skip him if he's not on this level */
            if (!COORDS_EQUAL(&player->wpos, &c->wpos)) continue;

            d = distance(y, x, player->py, player->px);
            if (d < min_dis) min_dis = d;
        }

        /* Accept far away grids */
        if (min_dis >= dis) break;
    }

    /* Abort */
    if (!attempts_left) return false;

    /* Attempt to place the monster, allow groups */
    if (pick_and_place_monster(p, c, y, x, monster_level(&c->wpos), mon_flag | MON_GROUP,
        ORIGIN_DROP))
    {
        return true;
    }

    /* Nope */
    return false;
}


/*
 * Split some experience between master and slaves.
 */
static void master_exp_gain(struct player *p, struct chunk *c, s32b *amount)
{
    int i;
    struct monster *mon;
    s32b average_lev = p->lev, num_members = 1, modified_level;

    /* Calculate the average level */
    for (i = 1; i < cave_monster_max(c); i++)
    {
        mon = cave_monster(c, i);

        /* Skip dead monsters */
        if (!mon->race) continue;

        /* Skip non slaves */
        if (p->id != mon->master) continue;

        /* Increase the "divisor" */
        average_lev += mon->level;
        num_members++;
    }

    /* Calculate the master's experience */
    if (p->lev * num_members < average_lev)
    {
        /* Below average */
        if ((average_lev - p->lev * num_members) > 2 * num_members)
            modified_level = p->lev * num_members + 2 * num_members;
        else
            modified_level = average_lev;
    }
    else
    {
        /* Above average */
        if ((p->lev * num_members - average_lev) > 2 * num_members)
            modified_level = p->lev * num_members - 2 * num_members;
        else
            modified_level = average_lev;
    }

    *amount = (*amount * modified_level) / (average_lev * num_members);

    /* Always award 1 point */
    if (*amount < 1) *amount = 1;
}


/*
 * Handle the "death" of a monster: give experience
 */
void monster_give_xp(struct player *p, struct chunk *c, struct monster *mon, bool split)
{
    s32b new_exp, new_exp_frac, amount_exp;

    /* Amount of experience earned */
    amount_exp = (long)mon->race->mexp * mon->level;

    /* Split experience between master and slaves */
    if (amount_exp && split) master_exp_gain(p, c, &amount_exp);

    /* Split experience if in a party */
    if (p->party)
    {
        /* Give experience to that party */
        party_exp_gain(p, p->party, amount_exp);
    }
    else
    {
        /* Give some experience */
        new_exp = amount_exp / p->lev;
        new_exp_frac = ((amount_exp % p->lev) * 0x10000L / p->lev) + p->exp_frac;

        /* Keep track of experience */
        if (new_exp_frac >= 0x10000L)
        {
            new_exp++;
            p->exp_frac = new_exp_frac - 0x10000L;
        }
        else
            p->exp_frac = new_exp_frac;

        /* Gain experience */
        player_exp_gain(p, new_exp);
    }
}


/*
 * Handle the "death" of a monster: drop carried objects
 */
void monster_drop_carried(struct player *p, struct chunk *c, struct monster *mon, int num,
    bool visible, int *dump_item, int *dump_gold)
{
    struct object *obj, *next;

    /* Create monster drops, if not yet created */
    if (mon->origin)
    {
        mon_create_drop(p, c, mon, mon->origin);
        mon->origin = ORIGIN_NONE;
    }

    /* Drop objects being carried */
    obj = mon->held_obj;
    while (obj)
    {
        next = obj->next;

        /* Object no longer held */
        obj->held_m_idx = 0;
        pile_excise(&mon->held_obj, obj);

        /* Count it and drop it - refactor once origin is a bitflag */
        if (dump_gold && tval_is_money(obj) && (obj->origin != ORIGIN_STOLEN))
            (*dump_gold)++;
        else if (dump_item && !tval_is_money(obj) && ((obj->origin == ORIGIN_DROP) ||
            (obj->origin == ORIGIN_DROP_PIT) || (obj->origin == ORIGIN_DROP_VAULT) ||
            (obj->origin == ORIGIN_DROP_SUMMON) || (obj->origin == ORIGIN_DROP_SPECIAL) ||
            (obj->origin == ORIGIN_DROP_BREED) || (obj->origin == ORIGIN_DROP_POLY)))
        {
            (*dump_item)++;
        }

        /* Change origin if monster is invisible */
        if (!visible) obj->origin = ORIGIN_DROP_UNKNOWN;

        /* Special handling of Grond/Morgoth */
        if (obj->artifact && kf_has(obj->kind->kind_flags, KF_QUEST_ART))
        {
            if (num > 0) obj->number = num;
            else
            {
                obj = next;
                continue;
            }
        }

        drop_near(p, c, &obj, 0, mon->fy, mon->fx, true, DROP_FADE);
        obj = next;
    }

    /* Forget objects */
    mon->held_obj = NULL;
}


/*
 * Handle the "death" of a monster: drop corpse
 */
void monster_drop_corpse(struct player *p, struct chunk *c, struct monster *mon)
{
    int y, x;

    /* Get the location */
    y = mon->fy;
    x = mon->fx;

    /* Sometimes, a dead monster leaves a corpse */
    if (rf_has(mon->race->flags, RF_DROP_CORPSE) && one_in_(20))
    {
        struct object *corpse = object_new();
        s32b timeout;
        int sval;

        /* Is the monster humanoid? */
        bool human = is_humanoid(mon->race);

        /* Half chance to get a humanoid corpse from half-humanoids */
        if (is_half_humanoid(mon->race)) human = magik(50);

        /* Prepare to make the corpse */
        if (human) sval = lookup_sval(TV_CORPSE, "corpse (humanoid)");
        else sval = lookup_sval(TV_CORPSE, "corpse (other)");
        object_prep(p, corpse, lookup_kind(TV_CORPSE, sval), 0, MINIMISE);

        /* Remember the type of corpse */
        corpse->pval = mon->race->ridx;

        /* Calculate length of time before decay */
        timeout = 5 + 2 * mon->race->weight + randint0(2 * mon->race->weight);
        if (timeout > 32000) timeout = 32000;
        corpse->decay = corpse->timeout = timeout;

        /* Set weight */
        corpse->weight = (mon->race->weight + randint0(mon->race->weight) / 10) + 1;

        /* Set origin */
        set_origin(corpse, ORIGIN_DROP, mon->wpos.depth, mon->race);

        /* Drop it in the dungeon */
        drop_near(p, c, &corpse, 0, y, x, true, DROP_FADE);
    }

    /* Sometimes, a dead monster leaves a skeleton */
    else if (rf_has(mon->race->flags, RF_DROP_SKELETON) && one_in_(mon->wpos.depth? 40: 200))
    {
        struct object *skeleton = object_new();
        int sval;

        /* Get the sval from monster race */
        if (mon->race->base == lookup_monster_base("canine"))
            sval = lookup_sval(TV_SKELETON, "Canine Skeleton");
        else if (mon->race->base == lookup_monster_base("rodent"))
            sval = lookup_sval(TV_SKELETON, "Rodent Skeleton");
        else if ((mon->race->base == lookup_monster_base("humanoid")) &&
            (strstr(mon->race->name, "elf") || strstr(mon->race->name, "elven")))
        {
            sval = lookup_sval(TV_SKELETON, "Elf Skeleton");
        }
        else if (mon->race->base == lookup_monster_base("kobold"))
            sval = lookup_sval(TV_SKELETON, "Kobold Skeleton");
        else if (mon->race->base == lookup_monster_base("orc"))
            sval = lookup_sval(TV_SKELETON, "Orc Skeleton");
        else if (mon->race->base == lookup_monster_base("person"))
            sval = lookup_sval(TV_SKELETON, "Human Skeleton");
        else if (streq(mon->race->name, "Ettin"))
            sval = lookup_sval(TV_SKELETON, "Ettin Skeleton");
        else if (mon->race->base == lookup_monster_base("troll"))
            sval = lookup_sval(TV_SKELETON, "Troll Skeleton");
        else if (mon->race->level >= 15)
            sval = lookup_sval(TV_SKELETON, "Skull");
        else if (one_in_(2))
            sval = lookup_sval(TV_SKELETON, "Broken Skull");
        else
            sval = lookup_sval(TV_SKELETON, "Broken Bone");

        /* Prepare to make the skeleton */
        object_prep(p, skeleton, lookup_kind(TV_SKELETON, sval), 0, MINIMISE);

        /* Set origin */
        set_origin(skeleton, ORIGIN_DROP, mon->wpos.depth, mon->race);

        /* Drop it in the dungeon */
        drop_near(p, c, &skeleton, 0, y, x, true, DROP_FADE);
    }
}


struct init_module mon_make_module =
{
    "mon-make",
    init_race_allocs,
    cleanup_race_allocs
};
