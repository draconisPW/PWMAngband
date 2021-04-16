/*
 * File: effects-info.h
 * Purpose: Implement interfaces for displaying information about effects
 */


#ifndef EFFECTS_INFO_H
#define EFFECTS_INFO_H

extern void print_effect(struct player *p, const char *d);
extern bool effect_describe(struct player *p, const struct object *obj, const struct effect *e);
extern struct effect *effect_next(struct effect *effect, void *data);
extern bool effect_damages(const struct effect *effect, void *data);
extern int effect_avg_damage(const struct effect *effect, void *data);
extern const char *effect_projection(const struct effect *effect, void *data);

#endif /* EFFECTS_INFO_H */
