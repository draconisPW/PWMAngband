/*
 * File: visuals-ui.h
 * Purpose: Appearance for screen elements
 */

#ifndef INCLUDED_VISUALS_UI_H
#define INCLUDED_VISUALS_UI_H

extern uint8_t visuals_cycler_get_attr_for_frame(const char *group_name, const char *cycle_name,
    const size_t frame);
extern void visuals_cycler_set_cycle_for_race(const struct monster_race *race,
    const char *group_name, const char *cycle_name);
extern uint8_t visuals_cycler_get_attr_for_race(const struct monster_race *race, const size_t frame);
extern uint8_t visuals_flicker_get_attr_for_frame(const uint8_t selection_attr, const size_t frame);

#endif /* INCLUDED_MAP_UI_H */