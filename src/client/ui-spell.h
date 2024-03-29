/*
 * File: ui-spell.h
 * Purpose: Spell UI handing
 */

#ifndef UI_SPELL_H
#define UI_SPELL_H


/* Maximum number of spell pages */
#define MAX_PAGES    10

struct spell_info
{
    char info[NORMAL_WID];
    spell_flags flag;
    char desc[MSG_LEN];
    char name[NORMAL_WID];
};

struct book_info
{
    struct spell_info spell_info[MAX_SPELLS_PER_PAGE];
    const struct magic_realm *realm;
};

/* Spell information array */
extern struct book_info book_info[MAX_PAGES];

extern void textui_book_browse(int book);
extern int textui_get_spell(int book, const char *verb, bool (*spell_filter)(int, int));
extern bool spell_okay_to_study(int book, int spell_index);
extern bool spell_okay_to_cast(int book, int spell);
extern int textui_obj_cast(int book, int *dir);
extern int textui_obj_project(int book, int *dir);
extern errr get_spell_by_name(int *book, int *spell);
extern int spell_count_pages(void);

#endif
