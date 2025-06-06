/*
 * File: obj-ui.c
 * Purpose: Lists of objects and object pictures
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
 * Return the "attr" for a given item kind.
 * Use "flavor" if available.
 * Default to user definitions.
 */
uint8_t object_kind_attr(struct player *p, const struct object_kind *kind)
{
    return (p->kind_aware[kind->kidx]? p->k_attr[kind->kidx]: p->d_attr[kind->kidx]);
}


/*
 * Return the "char" for a given item kind.
 * Use "flavor" if available.
 * Default to user definitions.
 */
char object_kind_char(struct player *p, const struct object_kind *kind)
{
    return (p->kind_aware[kind->kidx]? p->k_char[kind->kidx]: p->d_char[kind->kidx]);
}


/*
 * Return the "attr" for a given item.
 * Use "flavor" if available.
 * Default to user definitions.
 */
uint8_t object_attr(struct player *p, const struct object *obj)
{
    return object_kind_attr(p, obj->kind);
}


/*
 * Return the "char" for a given item.
 * Use "flavor" if available.
 * Default to user definitions.
 */
char object_char(struct player *p, const struct object *obj)
{
    return object_kind_char(p, obj->kind);
}


void display_item(struct player *p, struct object *obj, uint8_t equipped)
{
    struct object_xtra info_xtra;
    char o_name[NORMAL_WID];
    char o_name_terse[NORMAL_WID];
    char o_name_base[NORMAL_WID];
    int wgt;
    int32_t price = 0;

    memset(&info_xtra, 0, sizeof(info_xtra));

    /* Obtain an item description */
    object_desc(p, o_name, sizeof(o_name), obj, ODESC_PREFIX | ODESC_FULL);
    object_desc(p, o_name_terse, sizeof(o_name_terse), obj,
        ODESC_PREFIX | ODESC_FULL | ODESC_TERSE);
    object_desc(p, o_name_base, sizeof(o_name_base), obj, ODESC_BASE | ODESC_PLURAL);

    /* Display the weight if needed */
    wgt = (obj->tval? obj->weight * obj->number: 0);

    /* Display the price if needed */
    if (in_store(p) && (store_at(p)->feat <= FEAT_STORE_XBM))
        price = price_item(p, obj, true, obj->number);

    /* Get the info */
    get_object_info(p, obj, equipped, &info_xtra);

    /* Get the "sellable" flag */
    info_xtra.sellable = store_will_buy_tester(p, obj);

    /* Get the "ignore" flags */
    info_xtra.quality_ignore = ignore_level_of(p, obj);
    info_xtra.ignored = (uint8_t)object_is_ignored(p, obj);
    info_xtra.eidx = ((obj->ego && obj->known->ego)? (int16_t)obj->ego->eidx: -1);

    info_xtra.equipped = equipped;
    if (of_has(obj->flags, OF_AMMO_MAGIC)) info_xtra.magic = 1;
    info_xtra.bidx = (int16_t)object_to_book_index(p, obj);
    if (of_has(obj->flags, OF_THROWING)) info_xtra.throwable = 1;

    my_strcpy(info_xtra.name, o_name, sizeof(info_xtra.name));
    my_strcpy(info_xtra.name_terse, o_name_terse, sizeof(info_xtra.name_terse));
    my_strcpy(info_xtra.name_base, o_name_base, sizeof(info_xtra.name_base));

    /* Send the info to the client */
    Send_item(p, obj, wgt, price, &info_xtra);
}


void set_redraw_inven(struct player *p, struct object *obj)
{
    /* Full redraw */
    if (obj == NULL)
    {
        p->upkeep->redraw_inven = NULL;
        p->upkeep->skip_redraw_inven = true;
        p->upkeep->redraw |= (PR_INVEN);
        return;
    }

    /* Nothing to do */
    if (object_is_equipped(p->body, obj) || !object_is_carried(p, obj)) return;

    /* Same object to redraw */
    if (p->upkeep->redraw_inven == obj)
    {
        p->upkeep->redraw |= (PR_INVEN);
        return;
    }

    /* Single inventory object to redraw */
    if ((p->upkeep->redraw_inven == NULL) && !p->upkeep->skip_redraw_inven)
        p->upkeep->redraw_inven = obj;

    /* Skip redraw_inven object */
    else
    {
        p->upkeep->redraw_inven = NULL;
        p->upkeep->skip_redraw_inven = true;
    }

    p->upkeep->redraw |= (PR_INVEN);
}


/*
 * Choice window "shadow" of the "show_inven()" function
 */
void display_inven(struct player *p)
{
    struct object *obj;
    int i;

    /* Clear */
    obj = object_new();
    object_prep(p, chunk_get(&p->wpos), obj, pile_kind, 0, MINIMISE);
    display_item(p, obj, 0);
    object_delete(&obj);

    /* Display the pack */
    for (obj = p->gear; obj; obj = obj->next)
    {
        /* Skip equipped items */
        if (object_is_equipped(p->body, obj)) continue;

        /* Send the info to the client */
        display_item(p, obj, 0);
    }

    /* Hack -- wait for creation */
    if (!p->alive) return;

    /* Send quiver indices and count to client */
    for (i = 0; i < z_info->quiver_size; i++)
        Send_index(p, i, (p->upkeep->quiver[i]? p->upkeep->quiver[i]->oidx: -1), 0);
    Send_count(p, 1, p->upkeep->quiver_cnt);

    /* Send inventory indices to client */
    for (i = 0; i < z_info->pack_size; i++)
        Send_index(p, i, (p->upkeep->inven[i]? p->upkeep->inven[i]->oidx: -1), 1);
}


void set_redraw_equip(struct player *p, struct object *obj)
{
    /* Full redraw */
    if (obj == NULL)
    {
        p->upkeep->redraw_equip = NULL;
        p->upkeep->skip_redraw_equip = true;
        p->upkeep->redraw |= (PR_EQUIP);
        return;
    }

    /* Nothing to do */
    if (!object_is_equipped(p->body, obj)) return;

    /* Same object to redraw */
    if (p->upkeep->redraw_equip == obj)
    {
        p->upkeep->redraw |= (PR_EQUIP);
        return;
    }

    /* Single equipment object to redraw */
    if ((p->upkeep->redraw_equip == NULL) && !p->upkeep->skip_redraw_equip)
        p->upkeep->redraw_equip = obj;

    /* Skip redraw_equip object */
    else
    {
        p->upkeep->redraw_equip = NULL;
        p->upkeep->skip_redraw_equip = true;
    }

    p->upkeep->redraw |= (PR_EQUIP);
}


/*
 * Choice window "shadow" of the "show_equip()" function
 */
void display_equip(struct player *p)
{
    struct object *obj;
    int i;

    /* Clear */
    obj = object_new();
    object_prep(p, chunk_get(&p->wpos), obj, pile_kind, 0, MINIMISE);
    display_item(p, obj, 1);
    object_delete(&obj);

    /* Display the equipment */
    for (obj = p->gear; obj; obj = obj->next)
    {
        /* Skip non-equipped items */
        if (!object_is_equipped(p->body, obj)) continue;

        /* Send the info to the client */
        display_item(p, obj, 1);
    }

    /* Hack -- wait for creation */
    if (!p->alive) return;

    /* Send equipment indices and count to client */
    for (i = 0; i < p->body.count; i++)
        Send_index(p, i, (p->body.slots[i].obj? p->body.slots[i].obj->oidx: -1), 2);
    Send_count(p, 0, p->upkeep->equip_cnt);
}


/*
 * Choice window "shadow" of the "show_floor()" function
 */
void display_floor(struct player *p, struct chunk *c, struct object **floor_list, int floor_num,
    bool force)
{
    int i;
    struct object *dummy_item;
    struct object_xtra info_xtra;
    char o_name[NORMAL_WID];
    char o_name_terse[NORMAL_WID];
    char o_name_base[NORMAL_WID];

    /* Limit displayed floor items to z_info->floor_size */
    if (floor_num > z_info->floor_size) floor_num = z_info->floor_size;

    /* Effectiveness */
    if (!floor_num && !p->delta_floor_item) return;
    p->delta_floor_item = floor_num;

    /* Clear */
    dummy_item = object_new();
    dummy_item->known = object_new();
    memset(&info_xtra, 0, sizeof(info_xtra));
    info_xtra.slot = -1;
    info_xtra.bidx = -1;
    Send_floor(p, 0, dummy_item, &info_xtra, 0);

    /* Display the floor */
    for (i = 0; i < floor_num; i++)
    {
        memset(&info_xtra, 0, sizeof(info_xtra));

        /* Obtain an item description */
        object_desc(p, o_name, sizeof(o_name), floor_list[i], ODESC_PREFIX | ODESC_FULL);
        object_desc(p, o_name_terse, sizeof(o_name_terse), floor_list[i],
            ODESC_PREFIX | ODESC_FULL | ODESC_TERSE);
        object_desc(p, o_name_base, sizeof(o_name_base), floor_list[i],
            ODESC_BASE | ODESC_PLURAL);

        /* Get the info */
        get_object_info(p, floor_list[i], 0, &info_xtra);
        if (inven_carry_okay(p, floor_list[i])) info_xtra.carry = 1;

        /* Get the "ignore" flags */
        info_xtra.quality_ignore = ignore_level_of(p, floor_list[i]);
        info_xtra.ignored = (uint8_t)object_is_ignored(p, floor_list[i]);
        info_xtra.eidx = ((floor_list[i]->ego && floor_list[i]->known->ego)?
            (int16_t)floor_list[i]->ego->eidx: -1);

        if (of_has(floor_list[i]->flags, OF_AMMO_MAGIC)) info_xtra.magic = 1;
        info_xtra.bidx = (int16_t)object_to_book_index(p, floor_list[i]);
        if (of_has(floor_list[i]->flags, OF_THROWING)) info_xtra.throwable = 1;

        my_strcpy(info_xtra.name, o_name, sizeof(info_xtra.name));
        my_strcpy(info_xtra.name_terse, o_name_terse, sizeof(info_xtra.name_terse));
        my_strcpy(info_xtra.name_base, o_name_base, sizeof(info_xtra.name_base));

        /* Send the info to the client */
        Send_floor(p, i, floor_list[i], &info_xtra, 0);
    }

    /* Force response */
    if (force)
    {
        memset(&info_xtra, 0, sizeof(info_xtra));
        info_xtra.slot = -1;
        info_xtra.bidx = -1;
        Send_floor(p, 0, dummy_item, &info_xtra, 1);
    }
    object_delete(&dummy_item);
}


/*
 * Display the floor.
 */
void show_floor(struct player *p, int mode)
{
    Send_show_floor(p, (uint8_t)mode);
}


bool get_item(struct player *p, uint8_t tester_hook, char *dice_string)
{
    /* Pending */
    p->current_value = ITEM_PENDING;

    Send_item_request(p, tester_hook, dice_string);

    return true;
}


/*
 * Dump yet another object, currently wielded and matching
 * the wield_slot of reference object.
 */
static void compare_object_info(struct player *p, const struct object *obj)
{
    struct object *current;

    /* Check for a usable slot */
    int slot = wield_slot(p, obj);

    if ((slot < 0) || (slot >= p->body.count)) return;

    /* Find object currently equipped in that slot */
    current = slot_object(p, slot);
    if ((current != obj) && (obj->tval != TV_RING))
    {
        char o_name[NORMAL_WID];

        text_out(p, "\n\n\n");
        text_out(p, "Currently equipped: ");
        object_desc(p, o_name, sizeof(o_name), current, ODESC_PREFIX | ODESC_FULL);
        text_out(p, o_name);
        if (current)
        {
            text_out(p, "\n\n");
            object_info(p, current, OINFO_NONE);
        }
    }
}


/*
 * Display object recall modally and wait for a keypress.
 */
void display_object_recall_interactive(struct player *p, const struct object *obj, char *header)
{
    /* Let the player scroll through this info */
    p->special_file_type = SPECIAL_FILE_OTHER;

    /* Prepare player structure for text */
    text_out_init(p);

    /* Dump info into player */
    object_info(p, obj, OINFO_NONE);

    /* Hack -- dump similar wielded object */
    if (OPT(p, expand_inspect)) compare_object_info(p, obj);

    /* Restore height and width of current dungeon level */
    text_out_done(p);

    /* Notify player */
    notify_player(p, header, NTERM_WIN_OBJECT, false);
}
