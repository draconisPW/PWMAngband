/*
 * File: score.c
 * Purpose: Highscore handling
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
 * Read in a highscore file
 */
size_t highscore_read(struct high_score scores[], size_t sz)
{
    char fname[MSG_LEN];
    ang_file *scorefile;
    size_t i;

    /* Wipe current scores */
    memset(scores, 0, sz * sizeof(struct high_score));

    path_build(fname, sizeof(fname), ANGBAND_DIR_SCORES, "scores.raw");
    scorefile = file_open(fname, MODE_READ, FTYPE_RAW);

    if (!scorefile) return 0;

    for (i = 0; i < sz; i++)
    {
        if (file_read(scorefile, (char *)&scores[i], sizeof(struct high_score)) <= 0) break;
    }

    file_close(scorefile);

    /*
     * On a short read, also check the record one past the end in case
     * it was partially overwritten.
     */
    highscore_regularize(scores, (i < sz) ? i + 1 : sz);

    return i;
}


/*
 * Place an entry into a high score array
 */
size_t highscore_add(const struct high_score *entry, struct high_score scores[], size_t sz)
{
    size_t slot = highscore_where(entry, scores, sz);

    memmove(&scores[slot + 1], &scores[slot], sizeof(struct high_score) * (sz - 1 - slot));
    memcpy(&scores[slot], entry, sizeof(struct high_score));

    return slot;
}


static size_t highscore_count(const struct high_score scores[], size_t sz)
{
    size_t i;

    for (i = 0; i < sz; i++)
    {
        if (scores[i].what[0] == '\0') break;
    }

    return i;
}


/*
 * Actually place an entry into the high score file
 */
static void highscore_write(const struct high_score scores[], size_t sz)
{
    size_t n;
    ang_file *lok;
    ang_file *scorefile;
    char old_name[MSG_LEN];
    char cur_name[MSG_LEN];
    char new_name[MSG_LEN];
    char lok_name[MSG_LEN];

    path_build(old_name, sizeof(old_name), ANGBAND_DIR_SCORES, "scores.old");
    path_build(cur_name, sizeof(cur_name), ANGBAND_DIR_SCORES, "scores.raw");
    path_build(new_name, sizeof(new_name), ANGBAND_DIR_SCORES, "scores.new");
    path_build(lok_name, sizeof(lok_name), ANGBAND_DIR_SCORES, "scores.lok");

    /* Read in and add new score */
    n = highscore_count(scores, sz);

    /* Lock scores */
    if (file_exists(lok_name))
    {
        plog("Lock file in place for scorefile; not writing.");
        return;
    }

    lok = file_open(lok_name, MODE_WRITE, FTYPE_RAW);

    if (!lok)
    {
        plog("Failed to create lock for scorefile; not writing.");
        return;
    }
    else
        file_lock(lok);

    /* Open the new file for writing */
    scorefile = file_open(new_name, MODE_WRITE, FTYPE_RAW);

    if (!scorefile)
    {
        plog("Failed to open new scorefile for writing.");

        file_close(lok);
        file_delete(lok_name);
        return;
    }

    file_write(scorefile, (const char *)scores, sizeof(struct high_score) * n);
    file_close(scorefile);

    /* Now move things around */
    if (file_exists(old_name) && !file_delete(old_name))
        plog("Couldn't delete old scorefile");

    if (file_exists(cur_name) && !file_move(cur_name, old_name))
        plog("Couldn't move old scores.raw out of the way");

    if (!file_move(new_name, cur_name))
        plog("Couldn't rename new scorefile to scores.raw");

    /* Remove the lock */
    file_close(lok);
    file_delete(lok_name);
}


/*
 * Fill in a score record for the given player.
 *
 * entry points to the record to fill in.
 * p is the player whose score should be recorded.
 * died_from is the reason for death. In typical use, that will be
 * p->died_from, but when the player isn't dead yet, the caller may want to
 * use something else: "nobody (yet!)" is traditional.
 * death_time points to the time at which the player died. May be NULL
 * when the player isn't dead.
 */
void build_score(struct player *p, struct high_score *entry, const char *died_from,
    time_t *death_time)
{
    char psex;
    struct player_death_info score_info;

    memset(entry, 0, sizeof(struct high_score));

    switch (p->psex)
    {
        case SEX_MALE: psex = 'm'; break;
        case SEX_FEMALE: psex = 'f'; break;
        default: psex = 'n'; break;
    }

    /* Score info */
    memset(&score_info, 0, sizeof(score_info));
    if (death_time)
    {
        /* Hack -- take the saved cause of death of the character, not the ghost */
        memcpy(&score_info, &p->death_info, sizeof(struct player_death_info));
    }
    else
    {
        /* Take the current info */
        score_info.max_lev = p->max_lev;
        score_info.lev = p->lev;
        score_info.max_exp = p->max_exp;
        score_info.au = p->au;
        score_info.max_depth = p->max_depth;
        memcpy(&score_info.wpos, &p->wpos, sizeof(struct worldpos));
    }

    /* Save the version */
    strnfmt(entry->what, sizeof(entry->what), "%s", version_build(NULL, false));

    /* Calculate and save the points */
    strnfmt(entry->pts, sizeof(entry->pts), "%9ld",
        total_points(p, score_info.max_exp, score_info.max_depth));

    /* Save the current gold */
    strnfmt(entry->gold, sizeof(entry->gold), "%9ld", (long)score_info.au);

    /* Save the current turn */
    my_strcpy(entry->turns, ht_show(&turn), sizeof(entry->turns));

    /* Time of death */
    if (death_time)
        strftime(entry->day, sizeof(entry->day), "@%Y%m%d", localtime(death_time));
    else
        my_strcpy(entry->day, "TODAY", sizeof(entry->day));

    /* Save the player name (15 chars) */
    strnfmt(entry->who, sizeof(entry->who), "%-.15s", p->name);

    /* Save the player info */
    strnfmt(entry->uid, sizeof(entry->uid), "%7u", 0);
    strnfmt(entry->sex, sizeof(entry->sex), "%c", psex);
    strnfmt(entry->p_r, sizeof(entry->p_r), "%2d", p->race->ridx);
    strnfmt(entry->p_c, sizeof(entry->p_c), "%2d", p->clazz->cidx);

    /* Save the level and such */
    strnfmt(entry->cur_lev, sizeof(entry->cur_lev), "%3d", score_info.lev);
    strnfmt(entry->cur_dun, sizeof(entry->cur_dun), "%3d", score_info.wpos.depth);
    strnfmt(entry->max_lev, sizeof(entry->max_lev), "%3d", score_info.max_lev);
    strnfmt(entry->max_dun, sizeof(entry->max_dun), "%3d", score_info.max_depth);

    /* Save the cause of death (31 chars) */
    strnfmt(entry->how, sizeof(entry->how), "%-.31s", died_from);
}


/*
 * Enter a player's name on a hi-score table, if "legal".
 *
 * p is the player to enter
 * death_time points to the time at which the player died; may be NULL
 * for a player that's not dead yet
 */
void enter_score(struct player *p, time_t *death_time)
{
    struct high_score entry;
    struct high_score scores[MAX_HISCORES];

    /* Add a new entry, if allowed */
    if (p->noscore)
    {
        msg(p, "Score not registered for wizards, quitters and cheaters.");
        return;
    }

    /* Add a new entry to the score list, see where it went */
    build_score(p, &entry, p->death_info.died_from, death_time);

    highscore_read(scores, N_ELEMENTS(scores));
    highscore_add(&entry, scores, N_ELEMENTS(scores));
    highscore_write(scores, N_ELEMENTS(scores));
}


/*
 * Calculates the total number of points earned
 */
long total_points(struct player *p, int32_t max_exp, int16_t max_depth)
{
    /* Standard scoring */
    return max_exp + (100 * max_depth);
}
