/*
 * File: mon-init.h
 * Purpose: Parsing functions for monsters and monster base types
 */

#ifndef MONSTER_INIT_H
#define MONSTER_INIT_H

extern const char *r_info_flags[];
extern const char *r_info_spell_flags[];
extern struct file_parser mon_spell_parser;
extern struct file_parser mon_base_parser;
extern struct file_parser monster_parser;

#endif /* MONSTER_INIT_H */
