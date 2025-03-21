/*
 * File: prefs-ui.c
 * Purpose: Pref file handling code
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


uint8_t *monster_x_attr;
char *monster_x_char;
uint8_t *kind_x_attr;
char *kind_x_char;
uint8_t (*feat_x_attr)[LIGHTING_MAX];
char (*feat_x_char)[LIGHTING_MAX];
uint8_t (*trap_x_attr)[LIGHTING_MAX];
char (*trap_x_char)[LIGHTING_MAX];
uint8_t *flavor_x_attr;
char *flavor_x_char;


/*** Pref file parser ***/


/*
 * Private data for pref file parsing.
 */
struct prefs_data
{
    bool bypass;
    bool skip;
};


/*
 * Load another file.
 */
static enum parser_error parse_prefs_load(struct parser *p)
{
    struct prefs_data *d = parser_priv(p);
    const char *file;

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    file = parser_getstr(p, "file");
    process_pref_file(file, true);

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_expr(struct parser *p)
{
    struct prefs_data *d = parser_priv(p);

    my_assert(d != NULL);

    /* Hack -- do not load any Evaluated Expressions */
    parser_getstr(p, "expr");

    /* Set flag */
    d->bypass = d->skip;

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_object(struct parser *p)
{
    int tvi, svi;
    struct object_kind *kind;
    const char *tval, *sval;
    struct prefs_data *d = parser_priv(p);

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    tval = parser_getsym(p, "tval");
    sval = parser_getsym(p, "sval");

    /* object:*:* means handle all objects and flavors */
    if (streq(tval, "*"))
    {
        uint8_t attr = (uint8_t)parser_getint(p, "attr");
        char chr = (char)parser_getint(p, "char");
        int i;

        if (strcmp(sval, "*")) return PARSE_ERROR_UNRECOGNISED_SVAL;

        for (i = 0; i < z_info->k_max; i++)
        {
            kind = &k_info[i];

            kind_x_attr[kind->kidx] = attr;
            kind_x_char[kind->kidx] = chr;

            if (!kind->flavor) continue;

            flavor_x_attr[kind->flavor->fidx] = attr;
            flavor_x_char[kind->flavor->fidx] = chr;
        }
    }
    else
    {
        tvi = tval_find_idx(tval);
        if (tvi < 0) return PARSE_ERROR_UNRECOGNISED_TVAL;

        /* object:tval:* means handle all objects and flavors with this tval */
        if (streq(sval, "*"))
        {
            uint8_t attr = (uint8_t)parser_getint(p, "attr");
            char chr = (char)parser_getint(p, "char");
            int i;

            for (i = 0; i < z_info->k_max; i++)
            {
                kind = &k_info[i];

                if (kind->tval != tvi) continue;

                kind_x_attr[kind->kidx] = attr;
                kind_x_char[kind->kidx] = chr;

                if (!kind->flavor) continue;

                flavor_x_attr[kind->flavor->fidx] = attr;
                flavor_x_char[kind->flavor->fidx] = chr;
            }
        }
        else
        {
            svi = lookup_sval(tvi, parser_getsym(p, "sval"));
            if (svi < 0) return PARSE_ERROR_UNRECOGNISED_SVAL;

            kind = lookup_kind(tvi, svi);
            if (!kind) return PARSE_ERROR_UNRECOGNISED_SVAL;

            kind_x_attr[kind->kidx] = (uint8_t)parser_getint(p, "attr");
            kind_x_char[kind->kidx] = (char)parser_getint(p, "char");
        }
    }

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_monster(struct parser *p)
{
    const char *name;
    struct monster_race *monster;
    struct prefs_data *d = parser_priv(p);

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    name = parser_getsym(p, "name");
    monster = lookup_monster(name);
    if (!monster) return PARSE_ERROR_NO_KIND_FOUND;

    monster_x_attr[monster->ridx] = (uint8_t)parser_getint(p, "attr");
    monster_x_char[monster->ridx] = (char)parser_getint(p, "char");

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_monster_base(struct parser *p)
{
    const char *name;
    struct monster_base *mb;
    int i;
    struct prefs_data *d = parser_priv(p);
    uint8_t a;
    char c;

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    name = parser_getsym(p, "name");
    mb = lookup_monster_base(name);
    if (!mb) return PARSE_ERROR_INVALID_MONSTER_BASE;

    a = (uint8_t)parser_getint(p, "attr");
    c = (char)parser_getint(p, "char");

    for (i = 0; i < z_info->r_max; i++)
    {
        struct monster_race *race = &r_info[i];

        if (race->base != mb) continue;

        monster_x_attr[race->ridx] = a;
        monster_x_char[race->ridx] = c;
    }

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_feat_aux(struct parser *p)
{
    int idx;
    const char *lighting;
    int light_idx;
    struct prefs_data *d = parser_priv(p);

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    idx = lookup_feat_code(parser_getsym(p, "idx"));
    if (idx < 0 || idx >= FEAT_MAX) return PARSE_ERROR_OUT_OF_BOUNDS;

    lighting = parser_getsym(p, "lighting");
    if (streq(lighting, "torch"))
        light_idx = LIGHTING_TORCH;
    else if (streq(lighting, "los"))
        light_idx = LIGHTING_LOS;
    else if (streq(lighting, "lit"))
        light_idx = LIGHTING_LIT;
    else if (streq(lighting, "dark"))
        light_idx = LIGHTING_DARK;
    else if (streq(lighting, "*"))
        light_idx = LIGHTING_MAX;
    else
        return PARSE_ERROR_INVALID_LIGHTING;

    if (light_idx < LIGHTING_MAX)
    {
        feat_x_attr[idx][light_idx] = (uint8_t)parser_getint(p, "attr");
        feat_x_char[idx][light_idx] = (char)parser_getint(p, "char");
    }
    else
    {
        for (light_idx = 0; light_idx < LIGHTING_MAX; light_idx++)
        {
            feat_x_attr[idx][light_idx] = (uint8_t)parser_getint(p, "attr");
            feat_x_char[idx][light_idx] = (char)parser_getint(p, "char");
        }
    }

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_feat(struct parser *p)
{
    return parse_prefs_feat_aux(p);
}


static enum parser_error parse_prefs_feat_win(struct parser *p)
{
#ifdef WINDOWS
    return parse_prefs_feat_aux(p);
#else
    return PARSE_ERROR_NONE;
#endif
}


static void set_trap_graphic(int trap_idx, int light_idx, uint8_t attr, char ch)
{
    if (light_idx < LIGHTING_MAX)
    {
        trap_x_attr[trap_idx][light_idx] = attr;
        trap_x_char[trap_idx][light_idx] = ch;
    }
    else
    {
        for (light_idx = 0; light_idx < LIGHTING_MAX; light_idx++)
        {
            trap_x_attr[trap_idx][light_idx] = attr;
            trap_x_char[trap_idx][light_idx] = ch;
        }
    }
}


static enum parser_error parse_prefs_trap(struct parser *p)
{
    const char *idx_sym;
    const char *lighting;
    int trap_idx;
    int light_idx;
    struct prefs_data *d = parser_priv(p);
    uint8_t attr;
    char chr;

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    /* idx can be "*" or a name */
    idx_sym = parser_getsym(p, "idx");

    if (streq(idx_sym, "*"))
        trap_idx = -1;
    else
    {
        struct trap_kind *trap = lookup_trap(idx_sym);

        if (!trap) return PARSE_ERROR_UNRECOGNISED_TRAP;
        trap_idx = trap->tidx;
    }

    lighting = parser_getsym(p, "lighting");
    if (streq(lighting, "torch"))
        light_idx = LIGHTING_TORCH;
    else if (streq(lighting, "los"))
        light_idx = LIGHTING_LOS;
    else if (streq(lighting, "lit"))
        light_idx = LIGHTING_LIT;
    else if (streq(lighting, "dark"))
        light_idx = LIGHTING_DARK;
    else if (streq(lighting, "*"))
        light_idx = LIGHTING_MAX;
    else
        return PARSE_ERROR_INVALID_LIGHTING;

    attr = (uint8_t)parser_getint(p, "attr");
    chr = (char)parser_getint(p, "char");

    if (trap_idx == -1)
    {
        int i;

        for (i = 0; i < z_info->trap_max; i++)
            set_trap_graphic(i, light_idx, attr, chr);
    }
    else
        set_trap_graphic(trap_idx, light_idx, attr, chr);

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_gf(struct parser *p)
{
    bool types[PROJ_MAX];
    const char *direction;
    int motion, motion2 = 0;
    char *s, *t;
    size_t i;
    struct prefs_data *d = parser_priv(p);

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    memset(types, 0, PROJ_MAX * sizeof(bool));

    /* Parse the type, which is a | separated list of PROJ_ constants */
    s = string_make(parser_getsym(p, "type"));
    t = strtok(s, "| ");
    while (t)
    {
        if (streq(t, "*"))
            memset(types, true, PROJ_MAX * sizeof(bool));
        else
        {
            int idx = proj_name_to_idx(t);

            if (idx == -1) return PARSE_ERROR_INVALID_VALUE;

            types[idx] = true;
        }

        t = strtok(NULL, "| ");
    }

    string_free(s);

    direction = parser_getsym(p, "direction");
    if (streq(direction, "static"))
        motion = BOLT_NO_MOTION;
    else if (streq(direction, "0"))
    {
        motion = BOLT_0;
        motion2 = BOLT_180;
    }
    else if (streq(direction, "45"))
    {
        motion = BOLT_45;
        motion2 = BOLT_225;
    }
    else if (streq(direction, "90"))
    {
        motion = BOLT_90;
        motion2 = BOLT_270;
    }
    else if (streq(direction, "135"))
    {
        motion = BOLT_135;
        motion2 = BOLT_315;
    }
    else if (streq(direction, "180"))
        motion = BOLT_180;
    else if (streq(direction, "225"))
        motion = BOLT_225;
    else if (streq(direction, "270"))
        motion = BOLT_270;
    else if (streq(direction, "315"))
        motion = BOLT_315;
    else
        return PARSE_ERROR_INVALID_VALUE;

    for (i = 0; i < PROJ_MAX; i++)
    {
        if (!types[i]) continue;

        proj_to_attr[i][motion] = (uint8_t)parser_getuint(p, "attr");
        proj_to_char[i][motion] = (char)parser_getuint(p, "char");

        /* Default values */
        if (motion2)
        {
            proj_to_attr[i][motion2] = (uint8_t)parser_getuint(p, "attr");
            proj_to_char[i][motion2] = (char)parser_getuint(p, "char");
        }
    }

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_flavor(struct parser *p)
{
    unsigned int idx;
    struct flavor *flavor;
    struct prefs_data *d = parser_priv(p);

    my_assert(d != NULL);
    if (d->bypass) return PARSE_ERROR_NONE;

    idx = parser_getuint(p, "idx");
    for (flavor = flavors; flavor; flavor = flavor->next)
    {
        if (flavor->fidx == idx) break;
    }

    if (flavor)
    {
        flavor_x_attr[idx] = (uint8_t)parser_getint(p, "attr");
        flavor_x_char[idx] = (char)parser_getint(p, "char");
    }

    return PARSE_ERROR_NONE;
}


static enum parser_error parse_prefs_rf(struct parser *p)
{
    /* Hack -- parser hook for female player presets */
    return parse_prefs_monster(p);
}


static struct parser *init_parse_prefs(void)
{
    struct parser *p = parser_new();
    struct prefs_data *d = mem_zalloc(sizeof(struct prefs_data));

    d->skip = true;

    parser_setpriv(p, d);
    parser_reg(p, "% str file", parse_prefs_load);
    parser_reg(p, "? str expr", parse_prefs_expr);
    parser_reg(p, "object sym tval sym sval int attr int char", parse_prefs_object);
    parser_reg(p, "monster sym name int attr int char", parse_prefs_monster);
    parser_reg(p, "monster-base sym name int attr int char", parse_prefs_monster_base);
    parser_reg(p, "feat sym idx sym lighting int attr int char", parse_prefs_feat);
    parser_reg(p, "feat-win sym idx sym lighting int attr int char", parse_prefs_feat_win);
    parser_reg(p, "trap sym idx sym lighting int attr int char", parse_prefs_trap);
    parser_reg(p, "GF sym type sym direction uint attr uint char", parse_prefs_gf);
    parser_reg(p, "flavor uint idx int attr int char", parse_prefs_flavor);

    /* Hack -- parser hook for female player presets */
    parser_reg(p, "RF sym name int attr int char", parse_prefs_rf);

    return p;
}


static void print_error(const char *name, struct parser *p)
{
    struct parser_state s;

    parser_getstate(p, &s);
    plog_fmt("Parse error in %s line %d column %d: %s: %s", name,
        s.line, s.col, s.msg, parser_error_str[s.error]);
}


/*
 * Process the "user pref file" with the given name
 * "quiet" means "don't complain about not finding the file.
 *
 * Returns true if everything worked OK, false otherwise
 */
bool process_pref_file(const char *name, bool quiet)
{
    char buf[MSG_LEN];
    ang_file *f;
    struct parser *p;
    errr e = 0;
    int line_no = 0;

    /* Build the filename */
    path_build(buf, sizeof(buf), ANGBAND_DIR_CUSTOMIZE, name);
    if (!file_exists(buf)) path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

    f = file_open(buf, MODE_READ, FTYPE_TEXT);
    if (!f)
    {
        if (!quiet) plog_fmt("Cannot open '%s'.", buf);
    }
    else
    {
        char line[MSG_LEN];

        p = init_parse_prefs();

        while (file_getl(f, line, sizeof(line)))
        {
            line_no++;

            e = parser_parse(p, line);
            if (e != PARSE_ERROR_NONE)
            {
                print_error(buf, p);
                break;
            }
        }

        file_close(f);
        mem_free(parser_priv(p));
        parser_destroy(p);
    }

    /* Result */
    return (e == PARSE_ERROR_NONE);
}


/*
 * Reset the "visual" lists
 *
 * This involves resetting various things to their "default" state.
 */
static void reset_visuals(void)
{
    int i, j;
    struct flavor *f;

    /* Extract default attr/char code for features */
    for (i = 0; i < FEAT_MAX; i++)
    {
        struct feature *feat = &f_info[i];

        /* Assume we will use the underlying values */
        for (j = 0; j < LIGHTING_MAX; j++)
        {
            feat_x_attr[i][j] = feat->d_attr;
            feat_x_char[i][j] = feat->d_char;
        }
    }

    /* Extract default attr/char code for objects */
    for (i = 0; i < z_info->k_max; i++)
    {
        struct object_kind *kind = &k_info[i];

        /* Default attr/char */
        kind_x_attr[i] = kind->d_attr;
        kind_x_char[i] = kind->d_char;
    }

    /* Extract default attr/char code for monsters */
    for (i = 0; i < z_info->r_max; i++)
    {
        struct monster_race *race = &r_info[i];

        /* Default attr/char */
        monster_x_attr[i] = race->d_attr;
        monster_x_char[i] = race->d_char;
    }

    /* Extract default attr/char code for traps */
    for (i = 0; i < z_info->trap_max; i++)
    {
        struct trap_kind *trap = &trap_info[i];

        /* Default attr/char */
        for (j = 0; j < LIGHTING_MAX; j++)
        {
            trap_x_attr[i][j] = trap->d_attr;
            trap_x_char[i][j] = trap->d_char;
        }
    }

    /* Extract default attr/char code for flavors */
    for (f = flavors; f; f = f->next)
    {
        flavor_x_attr[f->fidx] = f->d_attr;
        flavor_x_char[f->fidx] = f->d_char;
    }
}


/*
 * Initialize the glyphs for monsters, objects, traps, flavors and terrain
 */
void textui_prefs_init(void)
{
    struct flavor *f;
    unsigned int flavor_max = 0;

    monster_x_attr = mem_zalloc(z_info->r_max * sizeof(uint8_t));
    monster_x_char = mem_zalloc(z_info->r_max * sizeof(char));
    kind_x_attr = mem_zalloc(z_info->k_max * sizeof(uint8_t));
    kind_x_char = mem_zalloc(z_info->k_max * sizeof(char));
    feat_x_attr = mem_zalloc(FEAT_MAX * sizeof(byte_lit));
    feat_x_char = mem_zalloc(FEAT_MAX * sizeof(char_lit));
    trap_x_attr = mem_zalloc(z_info->trap_max * sizeof(byte_lit));
    trap_x_char = mem_zalloc(z_info->trap_max * sizeof(char_lit));
    for (f = flavors; f; f = f->next)
    {
        if (f->fidx > flavor_max) flavor_max = f->fidx;
    }
    flavor_x_attr = mem_zalloc((flavor_max + 1) * sizeof(uint8_t));
    flavor_x_char = mem_zalloc((flavor_max + 1) * sizeof(char));

    reset_visuals();
}


/*
 * Free the glyph arrays for monsters, objects, traps, flavors and terrain
 */
void textui_prefs_free(void)
{
    mem_free(monster_x_attr);
    monster_x_attr = NULL;
    mem_free(monster_x_char);
    monster_x_char = NULL;
    mem_free(kind_x_attr);
    kind_x_attr = NULL;
    mem_free(kind_x_char);
    kind_x_char = NULL;
    mem_free(feat_x_attr);
    feat_x_attr = NULL;
    mem_free(feat_x_char);
    feat_x_char = NULL;
    mem_free(trap_x_attr);
    trap_x_attr = NULL;
    mem_free(trap_x_char);
    trap_x_char = NULL;
    mem_free(flavor_x_attr);
    flavor_x_attr = NULL;
    mem_free(flavor_x_char);
    flavor_x_char = NULL;
}
