/*
 * File: effects-info.c
 * Purpose: Implement interfaces for displaying information about effects
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2021 MAngband and PWMAngband Developers
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


static struct
{
    int index;
    int args;
    int efinfo_flag;
    const char *desc;
} base_descs[] =
{
    {EF_NONE, 0, EFINFO_NONE, ""},
    #define EFFECT(x, a, b, c, d, e) {EF_##x, c, d, e},
    #include "list-effects.h"
    #undef EFFECT
    {EF_MAX, 0, EFINFO_NONE, ""}
};


/*
 * Get the possible dice strings.
 */
static void format_dice_string(const random_value *v, int multiplier, size_t len, char* dice_string)
{
    if (v->dice && v->base)
    {
        if (multiplier == 1)
            strnfmt(dice_string, len, "{%d+%dd%d}", v->base, v->dice, v->sides);
        else
        {
            strnfmt(dice_string, len, "{%d+%d*(%dd%d)}", multiplier * v->base, multiplier, v->dice,
                v->sides);
        }
    }
    else if (v->dice)
    {
        if (multiplier == 1)
            strnfmt(dice_string, len, "{%dd%d}", v->dice, v->sides);
        else
            strnfmt(dice_string, len, "{%d*(%dd%d)}", multiplier, v->dice, v->sides);
    }
    else
        strnfmt(dice_string, len, "{%d}", multiplier * v->base);
}


void print_effect(struct player *p, const char *d)
{
    char desc[MSG_LEN];
    char *t;
    bool colored = false;

    /* Print a colourised description */
    my_strcpy(desc, d, sizeof(desc));
    t = strtok(desc, "{}");
    while (t)
    {
        if (colored) text_out_c(p, COLOUR_L_GREEN, t);
        else text_out(p, t);
        colored = !colored;
        t = strtok(NULL, "{}");
    }
}


bool effect_describe(struct player *p, const struct object *obj, const struct effect *e)
{
    char desc[MSG_LEN];
    int random_choices = 0;
    struct source actor_body;
    struct source *data = &actor_body;
    bool same_effect = false, same_value = false;

    source_player(data, 0, p);

    while (e)
    {
        int roll = 0;
        random_value value;
        char dice_string[20];
        int level, boost;

        /* Skip blank descriptions */
        if (!effect_desc(e))
        {
            e = e->next;
            continue;
        }

        level = (obj->artifact? get_artifact_level(obj): obj->kind->level);
        boost = MAX((p->state.skills[SKILL_DEVICE] - level) / 2, 0);

        memset(&value, 0, sizeof(value));
        if (e->dice != NULL)
            roll = dice_roll(e->dice, (void *)data, &value);

        /* Deal with special random effect */
        if (e->index == EF_RANDOM) random_choices = roll + 1;

        /* Get the possible dice strings */
        format_dice_string(&value, 1, sizeof(dice_string), dice_string);

        /* Check all the possible types of description format */
        switch (base_descs[e->index].efinfo_flag)
        {
            /* Straight copy */
            case EFINFO_NONE:
            {
                my_strcpy(desc, effect_desc(e), sizeof(desc));
                break;
            }

            case EFINFO_HURT:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e), dice_string);
                break;
            }

            /* Healing sometimes has a minimum percentage */
            case EFINFO_HEAL:
            {
                char min_string[50];

                if (value.m_bonus)
                {
                    strnfmt(min_string, sizeof(min_string),
                        " (or {%d%%} of max HP, whichever is greater)", value.m_bonus);
                }
                else
                    strnfmt(min_string, sizeof(min_string), "");
                strnfmt(desc, sizeof(desc), effect_desc(e), dice_string, min_string);
                break;
            }

            /* Use dice string */
            case EFINFO_CONST:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e), dice_string);
                break;
            }

            /* PWMAngband: use dice string and apply digestion rate */
            case EFINFO_FOOD:
            {
                /* Basic digestion rate based on speed */
                int rate = player_digest(p);

                /* Adjust for player speed */
                int multiplier = turn_energy(p->state.speed);

                char *fed = "feeds you";
                char turn_dice_string[20];

                if (e->subtype)
                {
                    if (e->subtype > 1) fed = "leaves you nourished";
                    else fed = "uses enough food value";
                }

                format_dice_string(&value, z_info->food_value * multiplier / rate,
                    sizeof(turn_dice_string), turn_dice_string);

                /* Hack -- check previous effect */
                if (same_effect)
                {
                    same_effect = false;

                    /* Identical: skip */
                    if (same_value)
                    {
                        same_value = false;
                        desc[0] = '\0';
                    }

                    /* Different values: display the value */
                    else
                    {
                        strnfmt(desc, sizeof(desc), "for %s turns (%s percent)", turn_dice_string,
                            dice_string);
                    }
                }
                else
                    strnfmt(desc, sizeof(desc), effect_desc(e), fed, turn_dice_string, dice_string);

                /* Hack -- check next effect */
                if (e->next && (e->next->index == e->index) && (e->next->subtype == e->subtype))
                {
                    random_value nextvalue;

                    memset(&nextvalue, 0, sizeof(nextvalue));
                    if (e->next->dice != NULL)
                        dice_roll(e->next->dice, (void *)data, &nextvalue);

                    same_effect = true;
                    if ((nextvalue.base == value.base) && (nextvalue.dice == value.dice) &&
                        (nextvalue.sides == value.sides))
                    {
                        same_value = true;
                    }
                }

                break;
            }

            /* Timed effect description */
            case EFINFO_CURE:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e), timed_effects[e->subtype].desc);
                break;
            }

            /* Timed effect description + duration */
            case EFINFO_TIMED:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e), timed_effects[e->subtype].desc,
                    dice_string);
                break;
            }

            /* Stat name */
            case EFINFO_STAT:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e),
                    lookup_obj_property(OBJ_PROPERTY_STAT, e->subtype)->name);
                break;
            }

            /* PWMAngband: restore original description (spell effect description + dice string) */
            case EFINFO_SEEN:
            {
                const char *proj_desc = projections[e->subtype].desc;

                /* Hack -- some effects have a duration */
                if (strstr(proj_desc, "%s"))
                {
                    char tmp[100];

                    strnfmt(tmp, sizeof(tmp), proj_desc, dice_string);
                    strnfmt(desc, sizeof(desc), effect_desc(e), tmp);
                }
                else
                    strnfmt(desc, sizeof(desc), effect_desc(e), proj_desc);
                break;
            }

            /* Summon effect description */
            case EFINFO_SUMM:
            {
                /* Hack -- check previous effect */
                if (same_effect)
                {
                    same_effect = false;

                    /* Identical: skip */
                    desc[0] = '\0';
                }
                else
                    strnfmt(desc, sizeof(desc), effect_desc(e), dice_string, summon_desc(e->subtype));

                /* Hack -- check next effect */
                if (e->next && (e->next->index == e->index) && (e->next->subtype == e->subtype))
                {
                    random_value nextvalue;

                    memset(&nextvalue, 0, sizeof(nextvalue));
                    if (e->next->dice != NULL)
                        dice_roll(e->next->dice, (void *)data, &nextvalue);

                    if ((nextvalue.base == value.base) && (nextvalue.dice == value.dice) &&
                        (nextvalue.sides == value.sides))
                    {
                        same_effect = true;
                    }
                }

                /* Hack -- only display suffix if last */
                if (!e->next)
                    my_strcat(desc, " generated at the current dungeon level", sizeof(desc));

                break;
            }

            /* PWMAngband: just use dice string since it's only used for objects */
            case EFINFO_TELE:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e), dice_string);
                break;
            }

            /* PWMAngband: using dice string or radius because it's not always a constant */
            case EFINFO_QUAKE:
            {
                if (e->radius)
                    strnfmt(dice_string, sizeof(dice_string), "{%d}", e->radius);
                strnfmt(desc, sizeof(desc), effect_desc(e), dice_string);
                break;
            }

            /* Object generated balls are elemental */
            /* PWMAngband: restore original description (reverse radius and description) */
            case EFINFO_BALL:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e), e->radius,
                    projections[e->subtype].desc, dice_string);
                if (boost)
                {
                    my_strcat(desc,
                        format(", which your device skill increases by {%d} percent", boost),
                        sizeof(desc));
                }
                break;
            }

            /* Object generated breaths are elemental */
            /* PWMAngband: restore original description (effect + damage) */
            case EFINFO_BREATH:
            {
                /* Hack -- check next effect */
                if (e->next && (e->next->index == e->index) && (e->index != EF_BREATH))
                {
                    random_value nextvalue;

                    memset(&nextvalue, 0, sizeof(nextvalue));
                    if (e->next->dice != NULL)
                        dice_roll(e->next->dice, (void *)data, &nextvalue);
                    if ((nextvalue.base == value.base) && (nextvalue.dice == value.dice) &&
                        (nextvalue.sides == value.sides))
                    {
                        same_effect = true;
                    }
                }

                /* Hack -- check previous effect */
                if (same_value)
                {
                    if (same_effect) same_effect = false;
                    else same_value = false;

                    /* Same values: display the effect, only display damage if last */
                    if (!e->next)
                    {
                        strnfmt(desc, sizeof(desc), "%s for %s points of damage",
                            projections[e->subtype].desc, dice_string);
                    }
                    else
                        my_strcpy(desc, projections[e->subtype].desc, sizeof(desc));
                }

                /* Hack -- check next effect */
                else if (same_effect)
                {
                    same_effect = false;
                    same_value = true;

                    strnfmt(desc, sizeof(desc), "produces a cone of %s",
                            projections[e->subtype].desc);
                }

                /* Normal case */
                else
                {
                    strnfmt(desc, sizeof(desc), effect_desc(e), projections[e->subtype].desc,
                        dice_string);
                }

                /* Hack -- only display boost if last */
                if (!e->next && boost && (e->index != EF_BREATH))
                {
                    my_strcat(desc,
                        format(", which your device skill increases by {%d} percent", boost),
                        sizeof(desc));
                }

                break;
            }

            /* Currently no object generated lashes */
            case EFINFO_LASH:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e),
                    projections[e->subtype].lash_desc, e->subtype);
                break;
            }

            /* Bolts that inflict status */
            case EFINFO_BOLT:
            {
                const char *proj_desc = projections[e->subtype].desc;

                /* Hack -- some effects have a duration */
                if (strstr(proj_desc, "%s"))
                {
                    char tmp[100];

                    strnfmt(tmp, sizeof(tmp), proj_desc, dice_string);
                    strnfmt(desc, sizeof(desc), effect_desc(e), tmp);
                }
                else
                    strnfmt(desc, sizeof(desc), effect_desc(e), proj_desc);
                break;
            }

            /* Bolts and beams that damage */
            case EFINFO_BOLTD:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e),
                    projections[e->subtype].desc, dice_string);
                if (boost)
                {
                    my_strcat(desc,
                        format(", which your device skill increases by {%d} percent", boost),
                        sizeof(desc));
                }
                break;
            }

            /* PWMAngband: restore original description (spell effect description + dice string) */
            case EFINFO_TOUCH:
            {
                const char *proj_desc = projections[e->subtype].desc;

                /* Hack -- some effects have a duration */
                if (strstr(proj_desc, "%s"))
                {
                    char tmp[100];

                    strnfmt(tmp, sizeof(tmp), proj_desc, dice_string);
                    strnfmt(desc, sizeof(desc), effect_desc(e), tmp);
                }
                else
                    strnfmt(desc, sizeof(desc), effect_desc(e), proj_desc);
                break;
            }

            case EFINFO_TAP:
            {
                strnfmt(desc, sizeof(desc), effect_desc(e), dice_string);
                break;
            }

            /* PWMAngband: restore mana can restore a fixed amount of mana points, or all of them */
            case EFINFO_MANA:
            {
                if (!value.base) my_strcpy(dice_string, "all your", sizeof(dice_string));
                strnfmt(desc, sizeof(desc), effect_desc(e), dice_string);
                break;
            }

            /* PWMAngband: restore original description */
            case EFINFO_ENCHANT:
            {
                const char *what;

                switch (e->subtype)
                {
                    case 1: what = "a weapon's to-hit bonus"; break;
                    case 2: what = "a weapon's to-dam bonus"; break;
                    case 3: what = "a weapon's to-hit and to-dam bonuses"; break;
                    case 4: what = "a piece of armor"; break;
                    default: what = "something"; /* XXX */
                }
                strnfmt(desc, sizeof(desc), effect_desc(e), what, dice_string);
                break;
            }

            default:
            {
                msg(p, "Bad effect description passed to describe_effect(). Please report this bug.");
                return false;
            }
        }

        if (STRZERO(desc))
        {
            if (random_choices >= 1) random_choices--;
            e = e->next;
            continue;
        }
        else
            print_effect(p, desc);

        /*
         * Random choices need special treatment - note that this code
         * assumes that RANDOM and the random choices will be the last
         * effect in the object/activation description
         */
        if (random_choices >= 1)
        {
            if (e->index == EF_RANDOM) {}
            else if (random_choices > 2) text_out(p, ", ");
            else if (random_choices == 2) text_out(p, " or ");
            random_choices--;
        }
        else if (e->next)
        {
            struct effect *next = e->next;

            if (next->next && (next->index != EF_RANDOM) && effect_desc(next))
                text_out(p, ", ");
            else
                text_out(p, " and ");
        }
        e = e->next;
    }

    return true;
}
