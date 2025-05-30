/*
 * File: display.c
 * Purpose: Display the character on the screen or in a file
 *
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


#include "angband.h"


int16_t cfg_fps = 50;


/*
 * Panel utilities
 */


/*
 * Panel line type
 */
struct panel_line
{
    uint8_t attr;
    const char *label;
    char value[20];
};


/*
 * Panel holder type
 */
struct panel
{
    size_t len;
    size_t max;
    struct panel_line *lines;
};


/*
 * Allocate some panel lines
 */
static struct panel *panel_allocate(int n)
{
    struct panel *p = mem_zalloc(sizeof(*p));

    p->len = 0;
    p->max = n;
    p->lines = mem_zalloc(p->max * sizeof(*p->lines));

    return p;
}


/*
 * Free up panel lines
 */
static void panel_free(struct panel *p)
{
    my_assert(p);
    mem_free(p->lines);
    mem_free(p);
}


/*
 * Add a new line to the panel
 */
static void panel_line(struct panel *p, uint8_t attr, const char *label, const char *fmt, ...)
{
    va_list vp;
    struct panel_line *pl;

    /* Get the next panel line */
    my_assert(p);
    my_assert(p->len != p->max);
    pl = &p->lines[p->len++];

    /* Set the basics */
    pl->attr = attr;
    pl->label = label;

    /* Set the value */
    va_start(vp, fmt);
    vstrnfmt(pl->value, sizeof(pl->value), fmt, vp);
    va_end(vp);
}


/*
 * Add a spacer line in a panel
 */
static void panel_space(struct panel *p)
{
    my_assert(p);
    my_assert(p->len != p->max);
    p->len++;
}


/*** Display buffer ***/


static char display_buffer[NORMAL_HGT][NORMAL_WID + 1];


/*** Display hooks ***/


errr (*clear_hook)(void);
void (*region_erase_hook)(const region *loc);
errr (*put_ch_hook)(int x, int y, uint16_t a, char c);
errr (*put_str_hook)(int x, int y, int n, uint16_t a, const char *s);
bool use_bigtile_hook;


/*** Buffer access functions ***/


/*
 * Clear the buffer
 */
errr buffer_clear(void)
{
    int row, col;

    /* Clear the buffer */
    for (row = 0; row < NORMAL_HGT; row++)
    {
        for (col = 0; col < NORMAL_WID; col++)
            display_buffer[row][col] = ' ';
        display_buffer[row][NORMAL_WID] = '\0';
    }

    return 0;
}


/*
 * Add a character to the buffer
 */
errr buffer_put_ch(int x, int y, uint16_t a, char c)
{
    display_buffer[y - 1][x] = c;

    return 0;
}


/*
 * Add a string to the buffer
 */
errr buffer_put_str(int x, int y, int n, uint16_t a, const char *s)
{
    char *cursor = display_buffer[y - 1], *str = (char *)s;
    int col = x, size = n;

    /* Position cursor, dump the text */
    while (*str && size && (col < NORMAL_WID))
    {
        cursor[col++] = (*str);
        str++;
        if (size > 0) size--;
    }

    return 0;
}


/*
 * Return one line of the buffer
 */
char *buffer_line(int row)
{
    return display_buffer[row];
}


/*** Utility display functions ***/


/*
 * List of resistances and abilities to display
 */
static const char *player_flag_table[(RES_PANELS + 3) * RES_ROWS] =
{
    "Acid:", /* ELEM_ACID */
    "Elec:", /* ELEM_ELEC */
    "Fire:", /* ELEM_FIRE */
    "Cold:", /* ELEM_COLD */
    "Pois:", /* ELEM_POIS */
    "Lite:", /* ELEM_LIGHT */
    "Dark:", /* ELEM_DARK */
    "Soun:", /* ELEM_SOUND */
    "Shar:", /* ELEM_SHARD */
    "Nexu:", /* ELEM_NEXUS */
    "Neth:", /* ELEM_NETHER */
    "Chao:", /* ELEM_CHAOS */
    "Dise:", /* ELEM_DISEN */

    "Fear:", /* OF_PROT_FEAR */
    "Blnd:", /* OF_PROT_BLIND */
    "Conf:", /* OF_PROT_CONF */
    "Stun:", /* OF_PROT_STUN */
    "HLif:", /* OF_HOLD_LIFE */
    " Rgn:", /* OF_REGEN */
    " ESP:", /* OF_ESP_XXX */
    "SInv:", /* OF_SEE_INVIS */
    "FAct:", /* OF_FREE_ACT */
    " Lev:", /* OF_FEATHER */
    "SDig:", /* OF_SLOW_DIGEST */
    "Trap:", /* OF_TRAP_IMMUNE */
    "Blss:", /* OF_BLESSED */

    " -HP:", /* OF_IMPAIR_HP */
    " -SP:", /* OF_IMPAIR_MANA */
    "Afrd:", /* OF_AFRAID */
    "Aggr:", /* OF_AGGRAVATE */
    "-Tel:", /* OF_NO_TELEPORT */
    "-Exp:", /* OF_DRAIN_EXP */
    "Stck:", /* OF_STICKY */
    "Frag:", /* OF_FRAGILE */
    "LTel:", /* OF_LIMITED_TELE */
    "",
    "Time:", /* ELEM_TIME */
    "Mana:", /* ELEM_MANA */
    "Wate:", /* ELEM_WATER */

    "Stea:", /* OBJ_MOD_STEALTH */
    "Sear:", /* OBJ_MOD_SEARCH */
    "Infr:", /* OBJ_MOD_INFRA */
    "Tunn:", /* OBJ_MOD_TUNNEL */
    " Spd:", /* OBJ_MOD_SPEED */
    "Blow:", /* OBJ_MOD_BLOWS */
    "Shot:", /* OBJ_MOD_SHOTS */
    "Mght:", /* OBJ_MOD_MIGHT */
    "PLit:", /* OBJ_MOD_LIGHT */
    "DRed:", /* OBJ_MOD_DAM_RED */
    "Move:", /* OBJ_MOD_MOVES */
    "",
    "",

    "Radi:", /* OF_ESP_RADIUS */
    "Evil:", /* OF_ESP_EVIL */
    "Anim:", /* OF_ESP_ANIMAL */
    "Unde:", /* OF_ESP_UNDEAD */
    "Demo:", /* OF_ESP_DEMON */
    " Orc:", /* OF_ESP_ORC */
    "Trol:", /* OF_ESP_TROLL */
    "Gian:", /* OF_ESP_GIANT */
    "Drag:", /* OF_ESP_DRAGON */
    "",
    "",
    "",
    "",

    "BAci:", /* ACID_ */
    "BEle:", /* ELEC_ */
    "BFir:", /* FIRE_ */
    "BCld:", /* COLD_ */
    "BPoi:", /* POIS_ */
    "BStn:", /* STUN_ */
    "BCut:", /* CUT_ */
    "BVmp:", /* VAMPIRIC */
    "",
    "",
    "",
    "",
    "",

    "SEvi:", /* EVIL_ */
    "SAni:", /* ANIMAL_ */
    "SOrc:", /* ORC_ */
    "STro:", /* TROLL_ */
    "SGia:", /* GIANT_ */
    "SDem:", /* DEMON_ */
    "SDra:", /* DRAGON_ */
    "SUnd:", /* UNDEAD_ */
    "",
    "",
    "",
    "",
    ""
};


/*
 * Equippy chars (ASCII representation of gear in equipment slot order)
 */
static void display_equippy(struct player *p, int row, int col)
{
    int i;
    uint8_t a;
    char c;

    /* No equippy chars in distorted mode */
    if (use_bigtile_hook) return;

    /* Dump equippy chars */
    for (i = 0; i < p->body.count; i++)
    {
        /* Get attr/char for display */
        a = p->hist_flags[0][i].a;
        c = p->hist_flags[0][i].c;

        /* Dump */
        put_ch_hook(col + i, row, a, c);
    }
}


static void display_resistance_panel(struct player *p, const char **rec, const region *bounds)
{
    size_t i;
    int j;
    int col = bounds->col;
    int row = bounds->row;
    int off = 1 + STAT_MAX + RES_ROWS * col / (p->body.count + 7);

    /* Special case: ESP flags + brands/slays */
    if (col >= RES_PANELS * (p->body.count + 7)) col -= RES_PANELS * (p->body.count + 7);

    /* Equippy */
    display_equippy(p, row++, col + 5);

    /* Header */
    put_str_hook(col, row++, -1, COLOUR_WHITE, "     abcdefgimnopq@");

    /* Lines */
    for (i = 0; i < RES_ROWS; i++, row++)
    {
        uint8_t name_attr = COLOUR_WHITE;

        /* Draw dots */
        for (j = 0; j <= p->body.count; j++)
        {
            uint8_t attr = p->hist_flags[off + i][j].a;
            char sym = (strlen(rec[i])? p->hist_flags[off + i][j].c: ' ');
            bool rune = false;

            /* Hack -- rune is known */
            if (attr >= BASIC_COLORS)
            {
                attr -= BASIC_COLORS;
                rune = true;
            }

            /* Dump proper character */
            put_ch_hook(col + 5 + j, row, attr, sym);

            /* Name color */

            /* Unknown rune */
            if (!rune) name_attr = COLOUR_SLATE;
            if (name_attr == COLOUR_SLATE) continue;

            /* Immunity */
            if (sym == '*') name_attr = COLOUR_GREEN;
            if (name_attr == COLOUR_GREEN) continue;

            /* Vulnerability */
            if (sym == '-') name_attr = COLOUR_L_RED;
            if (name_attr == COLOUR_L_RED) continue;

            /* Resistance */
            if (sym == '+') name_attr = COLOUR_L_BLUE;
            if (name_attr == COLOUR_L_BLUE) continue;

            /* Other known properties */
            if ((sym != '.') && (sym != '?') && (sym != '!') && (sym != '~'))
                name_attr = COLOUR_L_BLUE;
        }

        /* Name */
        if (strlen(rec[i])) put_str_hook(col, row, -1, name_attr, rec[i]);
    }
}


static void display_player_flag_info(struct player *p)
{
    unsigned i;
    int res_cols = p->body.count + 6;
    region resist_region[] =
    {
        {0, 8, 0, RES_ROWS + 2},
        {0, 8, 0, RES_ROWS + 2},
        {0, 8, 0, RES_ROWS + 2},
        {0, 8, 0, RES_ROWS + 2}
    };

    for (i = 0; i < N_ELEMENTS(resist_region); i++)
    {
        resist_region[i].col = i * (res_cols + 1);
        resist_region[i].width = res_cols;
    }

    for (i = 0; i < RES_PANELS; i++)
        display_resistance_panel(p, player_flag_table + i * RES_ROWS, &resist_region[i]);
}


static void display_player_other_info(struct player *p)
{
    unsigned i;
    int res_cols = p->body.count + 6;
    region resist_region[] =
    {
        {0, 8, 0, RES_ROWS + 2},
        {0, 8, 0, RES_ROWS + 2},
        {0, 8, 0, RES_ROWS + 2}
    };

    for (i = RES_PANELS; i < RES_PANELS + 3; i++)
    {
        resist_region[i - RES_PANELS].col = i * (res_cols + 1);
        resist_region[i - RES_PANELS].width = res_cols;
    }

    for (i = RES_PANELS; i < RES_PANELS + 3; i++)
        display_resistance_panel(p, player_flag_table + i * RES_ROWS, &resist_region[i - RES_PANELS]);
}


/*
 * Special display, part 2b
 */
static void display_player_stat_info(struct player *p)
{
    int i, row, col, r_adj;
    char buf[NORMAL_WID];

    /* Row */
    row = 2;

    /* Column */
    col = 42;

    /* Print out the labels for the columns */
    put_str_hook(col + 5, row - 1, -1, COLOUR_WHITE, "  Self");
    put_str_hook(col + 12, row - 1, -1, COLOUR_WHITE, " RB");
    put_str_hook(col + 16, row - 1, -1, COLOUR_WHITE, " CB");
    put_str_hook(col + 20, row - 1, -1, COLOUR_WHITE, " EB");
    put_str_hook(col + 24, row - 1, -1, COLOUR_WHITE, "  Best");

    /* Display the stats */
    for (i = 0; i < STAT_MAX; i++)
    {
        /* Reduced or normal */
        if (p->stat_cur[i] < p->stat_max[i])
        {
            /* Use lowercase stat name */
            put_str_hook(col, row + i, -1, COLOUR_WHITE, stat_names_reduced[i]);
        }
        else
        {
            /* Assume uppercase stat name */
            put_str_hook(col, row + i, -1, COLOUR_WHITE, stat_names[i]);
        }

        /* Indicate natural maximum */
        if (p->stat_max[i] == 18+100)
            put_str_hook(col + 3, row + i, -1, COLOUR_WHITE, "!");

        /* Internal "natural" maximum value */
        cnv_stat(p->stat_max[i], buf, sizeof(buf));
        put_str_hook(col + 5, row + i, -1, COLOUR_L_GREEN, buf);

        /* Race Bonus */
        /* Polymorphed players only get half adjustment from race */
        r_adj = race_modifier(p->race, i, p->lev, p->poly_race? true: false);
        strnfmt(buf, sizeof(buf), "%+3d", r_adj);
        put_str_hook(col + 12, row + i, -1, COLOUR_L_BLUE, buf);

        /* Class Bonus */
        strnfmt(buf, sizeof(buf), "%+3d", class_modifier(p->clazz, i, p->lev));
        put_str_hook(col + 16, row + i, -1, COLOUR_L_BLUE, buf);

        /* Equipment Bonus */
        strnfmt(buf, sizeof(buf), "%+3d", p->state.stat_add[i]);
        put_str_hook(col + 20, row + i, -1, COLOUR_L_BLUE, buf);

        /* Resulting "modified" maximum value */
        cnv_stat(p->state.stat_top[i], buf, sizeof(buf));
        put_str_hook(col + 24, row + i, -1, COLOUR_L_GREEN, buf);

        /* Only display stat_use if not maximal */
        if (p->stat_cur[i] < p->stat_max[i])
        {
            cnv_stat(p->state.stat_use[i], buf, sizeof(buf));
            put_str_hook(col + 31, row + i, -1, COLOUR_YELLOW, buf);
        }
    }
}


/*
 * Special display, part 2c
 *
 * How to print out the modifications and sustains.
 * Positive mods with no sustain will be light green.
 * Positive mods with a sustain will be dark green.
 * Sustains (with no modification) will be a dark green 's'.
 * Negative mods (from a curse) will be red.
 * Huge mods (>9), like from MICoMorgoth, will be a '*'
 * No mod, no sustain, will be a slate '.'
 */
static void display_player_sust_info(struct player *p)
{
    int i, row, col, stat;
    uint8_t a;
    char c;

    /* Row */
    row = 2;

    /* Column */
    col = 24;

    /* Header */
    put_str_hook(col, row - 1, -1, COLOUR_WHITE, "abcdefgimnopq@");

    for (stat = 0; stat < STAT_MAX; ++stat)
    {
        for (i = 0; i <= p->body.count; ++i)
        {
            a = p->hist_flags[stat + 1][i].a;
            c = p->hist_flags[stat + 1][i].c;

            /* Dump proper character */
            put_ch_hook(col + i, row + stat, a, c);
        }
    }
}


static void display_panel(const struct panel *p, bool left_adj, const region *bounds)
{
    size_t i;
    int col = bounds->col;
    int row = bounds->row;
    int w = bounds->width;
    int offset = 0;

    if (region_erase_hook) region_erase_hook(bounds);

    if (left_adj)
    {
        for (i = 0; i < p->len; i++)
        {
            struct panel_line *pl = &p->lines[i];
            int len = (pl->label? strlen(pl->label): 0);

            if (offset < len) offset = len;
        }
        offset += 2;
    }

    for (i = 0; i < p->len; i++, row++)
    {
        int len;
        struct panel_line *pl = &p->lines[i];

        if (!pl->label) continue;
        put_str_hook(col, row, strlen(pl->label), COLOUR_WHITE, pl->label);

        len = strlen(pl->value);
        len = ((len < w - offset)? len: (w - offset - 1));
        if (left_adj)
            put_str_hook(col + offset, row, len, pl->attr, pl->value);
        else
            put_str_hook(col + w - len, row, len, pl->attr, pl->value);
    }
}


static const char *show_adv_exp(struct player *p)
{
    if (p->lev < PY_MAX_LEVEL)
    {
        static char buffer[30];

        strnfmt(buffer, sizeof(buffer), "%ld", (long)adv_exp(p->lev, p->expfact));
        return buffer;
    }

    return "********";
}


static const char *show_depth(struct player *p)
{
    static char buffer[13];

    if (p->max_depth == 0)
    {
        /* Hack -- compatibility with Angband ladder */
        if (p->dump_gen) return "Town";

        return "Surface";
    }

    strnfmt(buffer, sizeof(buffer), "%d' (L%d)", p->max_depth * 50, p->max_depth);
    return buffer;
}


static const char *show_speed(struct player *p)
{
    static char buffer[10];
    int16_t speed = get_speed(p);

    if (speed == 0) return "Normal";

    strnfmt(buffer, sizeof(buffer), "%d", speed);
    return buffer;
}


static uint8_t max_color(int val, int max)
{
    return ((val < max)? COLOUR_YELLOW: COLOUR_L_GREEN);
}


/*
 * Colours for table items
 */
static const uint8_t colour_table[] =
{
    COLOUR_RED, COLOUR_RED, COLOUR_RED, COLOUR_L_RED, COLOUR_ORANGE,
    COLOUR_YELLOW, COLOUR_YELLOW, COLOUR_GREEN, COLOUR_GREEN, COLOUR_L_GREEN,
    COLOUR_L_BLUE
};


static struct panel *get_panel_topleft(struct player *pplayer)
{
    struct panel *p = panel_allocate(7);
    const char *player_title = get_title(pplayer);

    if (pplayer->ghost) player_title = "Ghost";

    panel_line(p, COLOUR_L_BLUE, "Name", "%s", pplayer->name);
    panel_line(p, COLOUR_L_BLUE, "Sex", "%s", pplayer->sex->title);
    panel_line(p, COLOUR_L_BLUE, "Race",  "%s", pplayer->race->name);
    panel_line(p, COLOUR_L_BLUE, "Class", "%s", pplayer->clazz->name);
    panel_line(p, COLOUR_L_BLUE, "Title", "%s", player_title);
    panel_line(p, COLOUR_L_BLUE, "HP", "%d/%d", pplayer->chp, pplayer->mhp);
    panel_line(p, COLOUR_L_BLUE, "SP", "%d/%d", pplayer->csp, pplayer->msp);

    return p;
}


static struct panel *get_panel_midleft(struct player *pplayer)
{
    struct panel *p = panel_allocate(9);
    int diff = get_diff(pplayer);
    uint8_t attr = ((diff < 0)? COLOUR_L_RED: COLOUR_L_GREEN);

    panel_line(p, max_color(pplayer->lev, pplayer->max_lev), "Level", "%d", pplayer->lev);
    panel_line(p, max_color(pplayer->exp, pplayer->max_exp), "Cur Exp", "%d", pplayer->exp);
    panel_line(p, COLOUR_L_GREEN, "Max Exp", "%d", pplayer->max_exp);
    panel_line(p, COLOUR_L_GREEN, "Adv Exp", "%s", show_adv_exp(pplayer));
    panel_space(p);
    panel_line(p, COLOUR_L_GREEN, "Gold", "%d", pplayer->au);
    panel_line(p, attr, "Burden", "%.1f lb", pplayer->upkeep->total_weight / 10.0F);
    panel_line(p, attr, "Overweight", "%d.%d lb", -diff / 10, abs(diff) % 10);
    panel_line(p, COLOUR_L_GREEN, "MaxDepth", "%s", show_depth(pplayer));

    return p;
}


static struct panel *get_panel_combat(struct player *pplayer)
{
    struct panel *p = panel_allocate(9);
    int bth, dam, hit;
    int melee_dice, melee_sides;
    int show_mhit, show_mdam;
    int show_shit, show_sdam;

    get_plusses(pplayer, &pplayer->known_state, &melee_dice, &melee_sides, &show_mhit, &show_mdam,
        &show_shit, &show_sdam);

    /* AC */
    panel_line(p, COLOUR_L_BLUE, "Armor", "[%d,%+d]", pplayer->known_state.ac,
        pplayer->known_state.to_a);

    /* Melee */
    bth = get_melee_skill(pplayer);
    dam = show_mdam;
    hit = show_mhit;
    if (pplayer->known_state.bless_wield) hit += 2;

    panel_space(p);
    panel_line(p, COLOUR_L_BLUE, "Melee", "%dd%d,%+d", melee_dice, melee_sides, dam);
    panel_line(p, COLOUR_L_BLUE, "To-hit", "%d,%+d", bth / 10, hit);
    panel_line(p, COLOUR_L_BLUE, "Blows", "%d.%d/turn", pplayer->state.num_blows / 100,
        (pplayer->state.num_blows / 10 % 10));

    /* Ranged */
    bth = get_ranged_skill(pplayer);
    hit = show_shit;
    dam = show_sdam;

    panel_space(p);
    panel_line(p, COLOUR_L_BLUE, "Shoot to-dam", "%+d", dam);
    panel_line(p, COLOUR_L_BLUE, "To-hit", "%d,%+d", bth / 10, hit);
    panel_line(p, COLOUR_L_BLUE, "Shots", "%d.%d/turn", pplayer->state.num_shots / 10,
        pplayer->state.num_shots % 10);

    return p;
}


static struct panel *get_panel_skills(struct player *pplayer)
{
    struct panel *p = panel_allocate(8);
    int skill;
    uint8_t attr;
    const char *desc;
    int depth = pplayer->wpos.depth;

    #define BOUND(x, min, max) MIN(max, MAX(min, x))

    /* Saving throw */
    skill = BOUND(pplayer->state.skills[SKILL_SAVE], 0, 100);
    panel_line(p, colour_table[skill / 10], "Saving Throw", "%d%%", skill);

    /* Stealth */
    desc = likert(pplayer->state.skills[SKILL_STEALTH], 1, &attr);
    panel_line(p, attr, "Stealth", "%s", desc);

    /* Physical disarming: assume we're disarming a dungeon trap */
    skill = BOUND(pplayer->state.skills[SKILL_DISARM_PHYS] - depth / 5, 2, 100);
    panel_line(p, colour_table[skill / 10], "Disarm - phys.", "%d%%", skill);

    /* Magical disarming */
    skill = BOUND(pplayer->state.skills[SKILL_DISARM_MAGIC] - depth / 5, 2, 100);
    panel_line(p, colour_table[skill / 10], "Disarm - magic", "%d%%", skill);

    /* Magic devices */
    skill = pplayer->state.skills[SKILL_DEVICE];
    panel_line(p, colour_table[MIN(skill, 130) / 13], "Magic Devices", "%d", skill);

    /* Searching ability */
    skill = BOUND(pplayer->state.skills[SKILL_SEARCH], 0, 100);
    panel_line(p, colour_table[skill / 10], "Searching", "%d%%", skill);

    /* Infravision */
    panel_line(p, COLOUR_L_GREEN, "Infravision", "%d ft", pplayer->state.see_infra * 10);

    /* Speed */
    skill = get_speed(pplayer);
    attr = ((skill < 0)? COLOUR_L_RED: COLOUR_L_GREEN);
    panel_line(p, attr, "Speed", "%s", show_speed(pplayer));

    return p;
}


static struct panel *get_panel_misc(struct player *pplayer)
{
    struct panel *p = panel_allocate(7);
    uint32_t game_turn = ht_div(&pplayer->game_turn, cfg_fps);
    uint32_t player_turn = ht_div(&pplayer->player_turn, 1);
    uint32_t active_turn = ht_div(&pplayer->active_turn, 1);

    panel_line(p, COLOUR_L_BLUE, "Age", "%d", pplayer->age);
    panel_line(p, COLOUR_L_BLUE, "Height", "%d'%d\"", pplayer->ht / 12, pplayer->ht % 12);
    panel_line(p, COLOUR_L_BLUE, "Weight", "%dst %dlb", pplayer->wt / 14, pplayer->wt % 14);
    panel_line(p, COLOUR_L_BLUE, "Turns used:", "");
    if (!game_turn)
        panel_line(p, COLOUR_SLATE, "Game", "%s", "N/A");
    else
        panel_line(p, COLOUR_L_BLUE, "Game", "%d", game_turn);
    if (!player_turn)
        panel_line(p, COLOUR_SLATE, "Player", "%s", "N/A");
    else
        panel_line(p, COLOUR_L_BLUE, "Player", "%d", player_turn);
    if (!active_turn)
        panel_line(p, COLOUR_SLATE, "Active", "%s", "N/A");
    else
        panel_line(p, COLOUR_L_BLUE, "Active", "%d", active_turn);

    return p;
}


/*
 * Panels for main character screen
 */
static const struct
{
    region bounds;
    bool align_left;
    struct panel *(*panel)(struct player *);
} panels[] =
{
    /* x  y width rows */
    {{ 1, 1, 40, 7}, true, get_panel_topleft}, /* Name, Class, ... */
    {{22, 1, 18, 7}, false, get_panel_misc}, /* Age, ht, wt, ... */
    {{ 1, 9, 24, 9}, false, get_panel_midleft}, /* Cur Exp, Max Exp, ... */
    {{29, 9, 19, 9}, false, get_panel_combat},
    {{52, 9, 20, 7}, false, get_panel_skills}
};


static void display_player_xtra_info(struct player *pplayer)
{
    size_t i;

    for (i = 0; i < N_ELEMENTS(panels); i++)
    {
        struct panel *p = panels[i].panel(pplayer);

        display_panel(p, panels[i].align_left, &panels[i].bounds);
        panel_free(p);
    }

    /* History */
    for (i = 0; i < N_HIST_LINES; i++)
        put_str_hook(1, i + 19, -1, COLOUR_WHITE, pplayer->history[i]);
}


/*
 * Display the character on the screen or in a file (three different modes)
 *
 * The top two lines, and the bottom line (or two) are left blank.
 *
 * Mode 0 = standard display with skills/history
 * Mode 1 = special display with equipment flags
 * Mode 2 = special display with equipment flags (ESP flags)
 */
void display_player(struct player *pplayer, uint8_t mode)
{
    /* Clear */
    clear_hook();

    /* Stat info */
    display_player_stat_info(pplayer);

    /* Special display */
    if (mode == 2)
    {
        struct panel *p = panels[0].panel(pplayer);

        display_panel(p, panels[0].align_left, &panels[0].bounds);
        panel_free(p);

        /* Stat/Sustain flags */
        display_player_sust_info(pplayer);

        /* Other flags */
        display_player_other_info(pplayer);
    }
    else if (mode == 1)
    {
        struct panel *p = panels[0].panel(pplayer);

        display_panel(p, panels[0].align_left, &panels[0].bounds);
        panel_free(p);

        /* Stat/Sustain flags */
        display_player_sust_info(pplayer);

        /* Other flags */
        display_player_flag_info(pplayer);
    }
    else
    {
        /* Extra info */
        display_player_xtra_info(pplayer);
    }
}


/*** Status line display functions ***/


size_t display_depth(struct player *p, int row, int col)
{
    char *text;

    /* Display the depth */
    text = format("%-12s", p->depths);
    put_str_hook(col, row, -1, COLOUR_WHITE, text);

    return (strlen(text) + 1);
}


/*
 * Struct to describe different timed effects
 */
struct state_info
{
    int value;
    const char *str;
    size_t len;
    uint8_t attr;
};


/*
 * Print all timed effects.
 */
static size_t prt_tmd(struct player *p, int row, int col)
{
    size_t i, len = 0;

    for (i = 0; i < TMD_MAX; i++)
    {
        if (p->timed[i])
        {
            struct timed_grade *grade = get_grade(i);

            while ((p->timed[i] > grade->max) || ((p->timed[i] < 0) && grade->next))
                grade = grade->next;
            put_str_hook(col + len, row, -1, grade->color, grade->name);
            len += strlen(grade->name) + 1;

            /* Food meter */
            if (i == TMD_FOOD)
            {
                char *meter = format("%d %%", p->timed[i] / 100);

                put_str_hook(col + len, row, -1, grade->color, meter);
                len += strlen(meter) + 1;
            }
        }
    }

    return len;
}


static const uint8_t obj_feeling_color[] =
{
    /* Colors used to display each obj feeling */
    COLOUR_WHITE, /* "this looks like any other level." */
    COLOUR_L_PURPLE, /* "you sense an item of wondrous power!" */
    COLOUR_L_RED, /* "there are superb treasures here." */
    COLOUR_ORANGE, /* "there are excellent treasures here." */
    COLOUR_YELLOW, /* "there are very good treasures here." */
    COLOUR_YELLOW, /* "there are good treasures here." */
    COLOUR_L_GREEN, /* "there may be something worthwhile here." */
    COLOUR_L_GREEN, /* "there may not be much interesting here." */
    COLOUR_L_GREEN, /* "there aren't many treasures here." */
    COLOUR_L_BLUE, /* "there are only scraps of junk here." */
    COLOUR_L_BLUE /* "there is naught but cobwebs here." */
};


static const uint8_t mon_feeling_color[] =
{
    /* Colors used to display each monster feeling */
    COLOUR_WHITE, /* "You are still uncertain about this place" */
    COLOUR_RED, /* "Omens of death haunt this place" */
    COLOUR_ORANGE, /* "This place seems murderous" */
    COLOUR_ORANGE, /* "This place seems terribly dangerous" */
    COLOUR_YELLOW, /* "You feel anxious about this place" */
    COLOUR_YELLOW, /* "You feel nervous about this place" */
    COLOUR_GREEN, /* "This place does not seem too risky" */
    COLOUR_GREEN, /* "This place seems reasonably safe" */
    COLOUR_BLUE, /* "This seems a tame, sheltered place" */
    COLOUR_BLUE /* "This seems a quiet, peaceful place" */
};


/*
 * Prints level feelings at status if they are enabled.
 */
static size_t prt_level_feeling(struct player *p, int row, int col)
{
    char obj_feeling_str[6];
    char mon_feeling_str[6];
    int new_col;
    uint8_t obj_feeling_color_print;

    /* No feeling */
    if ((p->obj_feeling == -1) && (p->mon_feeling == -1)) return 0;

    /*
     * Convert object feeling to a symbol easier to parse for a human.
     *   0 -> '*' "this looks like any other level."
     *   1 -> '$' "you sense an item of wondrous power!" (special feeling)
     *   2 to 10 are feelings from 2 meaning superb feeling to 10 meaning naught but cobwebs
     * It is easier for the player to have poor feelings as a low number and superb feelings
     * as a higher one. So for display we reverse this numbers and subtract 1. Thus (2-10)
     * becomes ('1'-'9' reversed). But before that check if the player has explored enough
     * to get a feeling. If not display as '?'.
     */
    if (p->obj_feeling == -1)
    {
        my_strcpy(obj_feeling_str, "?", sizeof(obj_feeling_str));
        obj_feeling_color_print = COLOUR_WHITE;
    }
    else
    {
        obj_feeling_color_print = obj_feeling_color[p->obj_feeling];
        if (p->obj_feeling == 0)
            my_strcpy(obj_feeling_str, "*", sizeof(obj_feeling_str));
        else if (p->obj_feeling == 1)
            my_strcpy(obj_feeling_str, "$", sizeof(obj_feeling_str));
        else
            strnfmt(obj_feeling_str, 5, "%d", (unsigned int)(11 - p->obj_feeling));
    }

    /*
     * Convert monster feeling to a symbol easier to parse for a human.
     *   0 -> '?' "this looks like any other level."
     *   1 to 9 are feelings from omens of death to quiet, peaceful
     * We also reverse this so that what we show is a danger feeling.
     */
    if (p->mon_feeling == 0)
        my_strcpy(mon_feeling_str, "?", sizeof(mon_feeling_str));
    else
        strnfmt(mon_feeling_str, 5, "%d", (unsigned int)(10 - p->mon_feeling));

    /* Display it */
    put_str_hook(col, row, -1, COLOUR_WHITE, "LF:");
    new_col = col + 3;
    put_str_hook(new_col, row, -1, mon_feeling_color[p->mon_feeling], mon_feeling_str);
    new_col += strlen(mon_feeling_str);
    put_str_hook(new_col, row, -1, COLOUR_WHITE, "-");
    new_col++;
    put_str_hook(new_col, row, -1, obj_feeling_color_print, obj_feeling_str);
    new_col += strlen(obj_feeling_str) + 1;
    return new_col - col;
}


/*
 * Prints player grid light level
 */
static size_t prt_light(struct player *p, int row, int col)
{
    int light = p->square_light;

    if (light > 0)
        put_str_hook(col, row, -1, COLOUR_YELLOW, format("Light %d ", light));
    else
        put_str_hook(col, row, -1, COLOUR_PURPLE, format("Light %d ", light));

    return 8 + ((ABS(light) > 9)? 1: 0) + ((light < 0)? 1: 0);
}


/*
 * Prints the movement speed of a character.
 */
static size_t prt_moves(struct player *p, int row, int col)
{
    int i = p->state.num_moves;

    /* Display the number of moves */
    if (i > 0)
        put_str_hook(col, row, -1, COLOUR_L_TEAL, format("Moves +%d ", i));
    else if (i < 0)
        put_str_hook(col, row, -1, COLOUR_L_TEAL, format("Moves -%d ", ABS(i)));

    /* Shouldn't be double digits, but be paranoid */
    return ((i != 0)? (9 + ABS(i) / 10): 0);
}


/*
 * Print "unignoring" status
 */
static size_t prt_unignore(struct player *p, int row, int col)
{
    /* Unignoring */
    if (p->unignoring)
    {
        const char *text = "Unignoring";

        put_str_hook(col, row, -1, COLOUR_WHITE, text);
        return (strlen(text) + 1);
    }

    return 0;
}


/*
 * Print recall status
 */
static size_t prt_recall(struct player *p, int row, int col)
{
    if (p->word_recall)
    {
        const char *text = "Recall";

        put_str_hook(col, row, -1, COLOUR_WHITE, text);
        return (strlen(text) + 1);
    }

    return 0;
}


/*
 * Print deep descent status
 */
static size_t prt_descent(struct player *p, int row, int col)
{
    if (p->deep_descent)
    {
        const char *text = "Descent";

        put_str_hook(col, row, -1, COLOUR_WHITE, text);
        return (strlen(text) + 1);
    }

    return 0;
}


/*
 * Prints Resting or Stealth Mode status
 */
static size_t prt_state(struct player *p, int row, int col)
{
    uint8_t attr = COLOUR_WHITE;
    const char *text = "";

    /* Resting */
    if (p->upkeep->resting)
        text = "Resting";

    /* Stealth mode */
    else if (p->stealthy)
    {
        attr = COLOUR_L_DARK;
        text = "Stealth Mode";
    }

    /* Display the info (or blanks) */
    put_str_hook(col, row, -1, attr, text);

    return (text[0]? (strlen(text) + 1): 0);
}


/*
 * Print how many spells the player can study.
 */
static size_t prt_study(struct player *p, int row, int col)
{
    char *text;
    int attr = COLOUR_WHITE;

    /* Can the player learn new spells? */
    if (p->upkeep->new_spells)
    {
        /*
         * If the player does not carry a book with spells they can study,
         * the message is displayed in a darker colour
         */
        if (!p->can_study_book) attr = COLOUR_L_DARK;

        /* Print study message */
        text = format("Study (%d)", p->upkeep->new_spells);
        put_str_hook(col, row, -1, attr, text);
        return (strlen(text) + 1);
    }

    return 0;
}


/*
 * Prints trap detection status
 */
static size_t prt_dtrap(struct player *p, int row, int col)
{
    uint8_t attr = COLOUR_WHITE;
    const char *text = "";
    uint8_t dtrap = get_dtrap(p);

    if (dtrap == 2)
    {
        attr = COLOUR_YELLOW;
        text = "DTrap";
    }
    if (dtrap == 1)
    {
        attr = COLOUR_L_GREEN;
        text = "DTrap";
    }

    /* Display the info (or blanks) */
    put_str_hook(col, row, -1, attr, text);

    return (text[0]? (strlen(text) + 1): 0);
}


/*
 * Prints player trap (if any) or terrain
 */
static size_t prt_terrain(struct player *p, int row, int col)
{
    if (!p->terrain[0]) return 0;

    put_str_hook(col, row, -1, p->terrain[0], p->terrain + 1);

    return strlen(p->terrain) - 1;
}


/*
 * Descriptive typedef for status handlers
 */
typedef size_t status_f(struct player *p, int row, int col);


/*
 * Status line indicators.
 */
static status_f *status_handlers[] =
{
    prt_level_feeling, prt_light, prt_moves, prt_unignore, prt_recall, prt_descent, prt_state,
        prt_study, prt_tmd, prt_dtrap, prt_terrain
};


/*
 * Print the status line.
 */
void display_statusline(struct player *p, int row, int col)
{
    size_t i;

    /* Display those which need redrawing */
    for (i = 0; i < N_ELEMENTS(status_handlers); i++)
        col += status_handlers[i](p, row, col);
}


/*
 * Print the status display subwindow
 */
void display_status_subwindow(struct player *p, int row, int col)
{
    size_t i;

    for (i = 0; i < N_ELEMENTS(status_handlers); i++)
        status_handlers[i](p, row++, col);
}

