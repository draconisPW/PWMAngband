/*
 * File: c-cmd-obj.c
 * Purpose: Handle objects in various ways
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2007-9 Andi Sidwell, Chris Carr, Ed Graham, Erik Osheim
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


#include "c-angband.h"


/*** Taking off/putting on ***/


/* Can only take off non-stuck items */
bool obj_can_takeoff(struct player *p, const struct object *obj)
{
    return !obj->info_xtra.stuck;
}


/* Can only throw an item that is not equipped or the equipped weapon if it can be taken off. */
bool obj_can_throw(struct player *p, const struct object *obj)
{
    return !object_is_equipped(p->body, obj) ||
        (tval_is_melee_weapon(obj) && obj_can_takeoff(p, obj));
}


/* Determine if an object is designed for throwing */
bool obj_is_throwing(struct player *p, const struct object *obj)
{
    return (obj->info_xtra.throwable? true: false);
}


/* Can only uninscribe inscribed items */
bool obj_has_inscrip(struct player *p, const struct object *obj)
{
    return (obj->note? true: false);
}


/*** Casting and browsing ***/


/*
 * Return the number of castable spells in the spellbook 'obj'.
 */
static int spell_book_count_spells(const struct object *obj, bool (*tester)(int, int spell))
{
    int book = obj->info_xtra.bidx;
    int i = 0, n_spells = 0;

    /* Paranoia */
    if (book < 0) return 0;
    if (book >= player->clazz->magic.num_books) return 0;

    /* Check for end of the book */
    while (book_info[book].spell_info[i].info[0] != '\0')
    {
        /* Spell is available */
        if (tester(book, i)) n_spells++;

        i++;
    }

    return n_spells;
}


bool obj_can_browse(struct player *p, const struct object *obj)
{
    /* Hack -- "tval_first" holds the tval of the first book of the realm */
    return (obj->tval == p->clazz->magic.tval_first);
}


/* A prerequisite to browsing */
bool obj_browse_pre(void)
{
    if (player->ghost && !player_can_undead(player))
    {
        textui_book_browse(0);
        return false;
    }

    if (player_has(player, PF_MONSTER_SPELLS))
    {
        int page = 0;
        int i, num;
        char tmp[NORMAL_WID];

        /* Number of pages */
        do
        {
            i = 0;
            num = 0;

            /* Check for end of the book */
            while (book_info[page].spell_info[i].info[0] != '\0')
            {
                /* Spell is available */
                num++;

                i++;
            }
            if (num > 0) page++;
        }
        while ((num > 0) && (page < MAX_PAGES));

        /* Forms with no spells */
        if (!page)
        {
            c_msg_print("You don't know any monster spells.");
            return false;
        }

        /* Pick a page and display it */
        strnfmt(tmp, sizeof(tmp), "Select a page (1-%d): ", page);
        page = get_quantity(tmp, page);
        if (page) textui_book_browse(page - 1);
        return false;
    }

    if (!player->clazz->magic.total_spells)
    {
        c_msg_print("You cannot read books!");
        return false;
    }

    return true;
}


bool obj_can_study(struct player *p, const struct object *obj)
{
    return (obj_can_browse(p, obj) &&
        (spell_book_count_spells(obj, spell_okay_to_study) > 0));
}


/* A prerequisite to studying */
bool obj_study_pre(void)
{
    if (!player->clazz->magic.total_spells || (player->ghost && !player_can_undead(player)) ||
        player_has(player, PF_MONSTER_SPELLS))
    {
        c_msg_print("You cannot gain spells!");
        return false;
    }

    return true;
}


bool obj_can_cast_from(struct player *p, const struct object *obj)
{
    return (obj_can_browse(p, obj) &&
        (spell_book_count_spells(obj, spell_okay_to_cast) > 0));
}


static bool get_mimic_spell_by_name(int *book, int *spell)
{
    char buf[NORMAL_WID];
    int i, num;
    int cur_page = 0;

    buf[0] = '\0';
    prompt_quote_hack = true;

    if (!get_string("Spell name: ", buf, NORMAL_WID) || STRZERO(buf)) return false;

    do
    {
        i = 0;
        num = 0;

        /* Check for end of the book */
        while (book_info[cur_page].spell_info[i].info[0] != '\0')
        {
            /* Spell is available */
            num++;

            if (streq(book_info[cur_page].spell_info[i].name, buf))
            {
                *book = cur_page;
                *spell = i;
                return true;
            }

            i++;
        }
        if (num > 0) cur_page++;
    }
    while ((num > 0) && (cur_page < MAX_PAGES));

    return false;
}


/* A prerequisite to casting */
bool obj_cast_pre(void)
{
    /* Use a ghost ability */
    if (player->ghost && !player_can_undead(player))
    {
        int dir = 0;
        int spell = textui_obj_cast(0, &dir);

        if (spell != -1) Send_ghost(spell, dir);
        return false;
    }

    /* Cast a monster spell */
    if (player_has(player, PF_MONSTER_SPELLS))
    {
        int chosen_page;
        char tmp[NORMAL_WID];
        char buf[NORMAL_WID];

        /* Number of pages */
        int page = spell_count_pages();

        /* Forms with no spells */
        if (!page)
        {
            c_msg_print("You don't know any monster spells.");
            return false;
        }

        /* Check the player can cast mimic spells at all */
        switch (player->cannot_cast_mimic)
        {
            case 1: c_msg_print("You cannot cast monster spells!"); return false;
            case 2: c_msg_print("You are too confused!"); return false;
            default: break;
        }

        /* Hack -- don't get out of icky screen if disturbed */
        allow_disturb_icky = false;

        /* Pick a page */
        strnfmt(tmp, sizeof(tmp), "Select a page (1-%d, *=select by flag, #=select by name): ", page);
        my_strcpy(buf, "#", sizeof(buf));
        prompt_quote_hack = true;
        if (!get_string(tmp, buf, NORMAL_WID))
        {
            allow_disturb_icky = true;
            return false;
        }

        chosen_page = atoi(buf);

        /* Select spell by name */
        if (buf[0] == '#')
        {
            int book, spell;
            spell_flags flag;
            int dir = 0;

            if (!get_mimic_spell_by_name(&book, &spell))
            {
                allow_disturb_icky = true;
                return false;
            }

            flag = book_info[book].spell_info[spell].flag;

            /* Check mana */
            if ((flag.smana > player->csp) && !OPT(player, risky_casting))
            {
                c_msg_print(format("You do not have enough mana to %s this %s.",
                    book_info[book].realm->verb, book_info[book].realm->spell_noun));
                return false;
            }

            /* Needs a direction */
            if (flag.dir_attr)
            {
                if (!get_aim_dir(&dir))
                {
                    allow_disturb_icky = true;
                    return false;
                }
            }

            if (spell != -1) Send_mimic(book, spell, dir);
            allow_disturb_icky = true;
            return false;
        }

        /* Select spell by flag */
        else if ((buf[0] == '*') || isalpha((unsigned char)buf[0]))
            chosen_page = page + 1;

        if (chosen_page > page + 1) chosen_page = page + 1;
        if (chosen_page < 0) chosen_page = 0;

        allow_disturb_icky = true;

        /* Cast a spell directly by using spell flag */
        if (chosen_page == page + 1)
        {
            int dir = 0;
            int spell = textui_obj_cast(-1, &dir);

            if (spell != -1) Send_mimic(-1, spell, dir);
        }

        /* Cast a spell by using page/spell number */
        else if (chosen_page)
        {
            int dir = 0;
            int spell = textui_obj_cast(chosen_page - 1, &dir);

            if (spell != -1) Send_mimic(chosen_page - 1, spell, dir);
        }

        return false;
    }

    /* Check the player can cast spells at all */
    switch (player->cannot_cast)
    {
        case 1: c_msg_print("You cannot pray or produce magics."); return false;
        case 2: c_msg_print("You cannot see!"); return false;
        case 3: c_msg_print("You are too confused!"); return false;
        default: break;
    }

    return true;
}


/*** Using items the traditional way ***/


/* Basic tval testers */
bool obj_is_staff(struct player *p, const struct object *obj)
{
    return tval_is_staff(obj);
}


bool obj_is_wand(struct player *p, const struct object *obj)
{
    return tval_is_wand(obj);
}


bool obj_is_rod(struct player *p, const struct object *obj)
{
    return tval_is_rod(obj);
}


bool obj_is_food(struct player *p, const struct object *obj)
{
    return tval_is_edible(obj);
}


bool obj_is_potion(struct player *p, const struct object *obj)
{
    return tval_is_potion(obj);
}


bool obj_is_scroll(struct player *p, const struct object *obj)
{
    return tval_is_scroll(obj);
}


/* Determine if an object has charges */
bool obj_has_charges(const struct object *obj)
{
    if (!tval_can_have_charges(obj)) return false;

    if (obj->pval <= 0) return false;

    return true;
}


/* Determine if an object is zappable */
bool obj_can_zap(const struct object *obj)
{
    /* Check activation flag */
    return (obj->info_xtra.act == ACT_NORMAL);
}


/* Determine if an object can be picked up */
bool inven_carry_okay(const struct object *obj)
{
    /* Check carry flag */
    return (obj->info_xtra.carry > 0);
}


/* Determine if an object is activatable */
bool obj_is_activatable(struct player *p, const struct object *obj)
{
    /* Check activation flag */
    return (obj->info_xtra.act != ACT_NONE);
}


/* Determine if an object can be activated now */
bool obj_can_activate(const struct object *obj)
{
    /* Check activation flag */
    return (obj->info_xtra.act == ACT_NORMAL);
}


/*
 * Hook to determine if an object can be fired
 */
bool item_tester_hook_fire(struct player *p, const struct object *obj)
{
    /* Examine the "current bow" */
    struct object *shooter = equipped_item_by_slot_name(p, "shooting");

    /* Handle current shooter */
    return tval_can_be_fired(shooter, obj);
}


bool obj_is_useable(struct player *p, const struct object *obj)
{
    if (item_tester_hook_fire(p, obj)) return true;
    if (tval_is_useable(obj)) return true;
    if (obj_is_activatable(p, obj)) return true;
    return false;
}


int need_dir(struct object *obj)
{
    /* Needs a direction */
    if (obj->info_xtra.aim) return DIR_UNKNOWN;

    return DIR_SKIP;
}


/*** Refuelling ***/


#define is_lamp(T) \
    (((T)->sval == lookup_sval((T)->tval, "Lantern")) || ((T)->sval == lookup_sval((T)->tval, "Lamp")))


/*
 * Check if an object can be used to refuel other objects.
 */
bool obj_can_refill(struct player *p, const struct object *obj)
{
    const struct object *light = equipped_item_by_slot_name(p, "light");

    /* A lamp can be refueled from a flask or another lamp */
    if (light && is_lamp(light))
    {
        if (tval_is_fuel(obj)) return true;

        /* Non-empty, non-everburning lamps are okay */
        if (tval_is_light(obj) && is_lamp(obj) && obj->info_xtra.fuel) return true;
    }

    return false;
}


bool obj_refill_pre(void)
{
    struct object *obj;

    /* Get the light */
    obj = equipped_item_by_slot_name(player, "light");

    /* It is nothing */
    if (!obj || !tval_is_light(obj))
    {
        c_msg_print("You are not wielding a light.");
        return false;
    }

    /* No light to refill */
    if (!obj->info_xtra.fuel)
    {
        c_msg_print("Your light cannot be refilled.");
        return false;
    }

    return true;
}
