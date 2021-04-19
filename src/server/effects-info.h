/*
 * File: effects-info.h
 * Purpose: Implement interfaces for displaying information about effects
 */


#ifndef EFFECTS_INFO_H
#define EFFECTS_INFO_H

/*
 * Flags for effect descriptions
 */
enum
{
    EFINFO_NONE,
    EFINFO_DICE,
    EFINFO_HEAL,
    EFINFO_CONST,
    EFINFO_FOOD,
    EFINFO_CURE,
    EFINFO_TIMED,
    EFINFO_STAT,
    EFINFO_SEEN,
    EFINFO_SUMM,
    EFINFO_TELE,
    EFINFO_QUAKE,
    EFINFO_BALL,
    EFINFO_BREATH,
    EFINFO_LASH,
    EFINFO_BOLT,
    EFINFO_BOLTD,
    EFINFO_TOUCH,
    EFINFO_MANA,
    EFINFO_ENCHANT
};

extern void print_effect(struct player *p, const char *d);
extern bool effect_describe(struct player *p, const struct object *obj, const struct effect *e);
extern struct effect *effect_next(struct effect *effect, void *data);
extern bool effect_damages(const struct effect *effect, void *data, const char *name);
extern int effect_avg_damage(const struct effect *effect, void *data, const char *name);
extern const char *effect_projection(const struct effect *effect, void *data);

#endif /* EFFECTS_INFO_H */
