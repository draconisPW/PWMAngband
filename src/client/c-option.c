/*
 * File: c-option.c
 * Purpose: Options table and definitions.
 *
 * Copyright (c) 1997 Ben Harrison
 * Copyright (c) 2020 MAngband and PWMAngband Developers
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


#include "c-angband.h"


/*
 * Set an option, return true if successful
 */
bool option_set(bool *opts, const char *name, size_t val)
{
    size_t opt;

    /* Try normal options first */
    if (opts)
    {
        for (opt = 0; opt < OPT_MAX; opt++)
        {
            if (!option_name(opt) || !streq(option_name(opt), name)) continue;
            opts[opt] = (val? true: false);

            return true;
        }

        return false;
    }

    if (streq(name, "hp_warn_factor"))
    {
        /* Bounds */
        if (val > 9) val = 9;
        player->opts.hitpoint_warn = val;

        return true;
    }
    if (streq(name, "delay_factor"))
    {
        /* Bounds */
        if (val > 255) val = 255;
        player->opts.delay_factor = val;

        return true;
    }
    if (streq(name, "lazymove_delay"))
    {
        /* Bounds */
        if (val > 9) val = 9;
        player->opts.lazymove_delay = val;

        return true;
    }

    return false;
}


/*
 * Set player default options
 */
void options_init_defaults(struct player_options *opts)
{
    int i;

    /* 40ms for the delay factor */
    opts->delay_factor = 40;

    /* 30% of HP */
    opts->hitpoint_warn = 3;

    /* Initialize extra parameters */
    for (i = ITYPE_NONE; i < ITYPE_MAX; i++) opts->ignore_lvl[i] = IGNORE_BAD;
}


/*
 * Record the birth options for later recall.
 *
 * Return true if successful. Return false if the operation failed.
 */
bool options_save_custom_birth(bool *opts)
{
    bool success = true;
    char path[MSG_LEN];
    ang_file *f;

    path_build(path, sizeof(path), ANGBAND_DIR_USER, "customized_birth_options.txt");
    f = file_open(path, MODE_WRITE, FTYPE_TEXT);
    if (f)
    {
        int opt;

        if (!file_put(f, "# These are customized defaults for the birth options.\n"))
            success = false;
        if (!file_put(f, "# All lines begin with \"option:\" followed by the internal option name.\n"))
            success = false;
        if (!file_put(f, "# After the name is a colon followed by yes or no for the option's state.\n"))
            success = false;
        for (opt = 0; opt < OPT_MAX; opt++)
        {
            if (option_type(opt) == OP_BIRTH)
            {
                if (!file_putf(f, "# %s\n", option_desc(opt)))
                    success = false;
                if (!file_putf(f, "option:%s:%s\n", option_name(opt), (opts[opt]? "yes": "no")))
                    success = false;
            }
        }
        if (!file_close(f))
            success = false;
    }
    else
        success = false;

    return success;
}


/*
 * Reset the birth options to the customized defaults.
 *
 * Return true if successful. That includes the case where no customized
 * defaults are available. When that happens, the birth options are reset
 * to the maintainer's defaults. Return false if the customized defaults
 * are present but unreadable.
 */
bool options_restore_custom_birth(bool *opts)
{
    bool success = true;
    char path[MSG_LEN];

    path_build(path, sizeof(path), ANGBAND_DIR_USER, "customized_birth_options.txt");
    if (file_exists(path))
    {
        /* Could use run_parser(), but that exits the application if there are syntax errors */
        ang_file *f = file_open(path, MODE_READ, FTYPE_TEXT);

        if (f)
        {
            int linenum = 1;
            char buf[MSG_LEN];

            while (file_getl(f, buf, sizeof(buf)))
            {
                int offset = 0;

                if (sscanf(buf, "option:%n%*s", &offset) == 0 && offset > 0)
                {
                    int opt = 0;

                    while (1)
                    {
                        size_t lname;

                        if (opt >= OPT_MAX)
                        {
                            plog_fmt("Unrecognized birth option at line %d of the customized birth options.", linenum);
                            break;
                        }
                        if (!option_name(opt))
                        {
                            ++opt;
                            continue;
                        }
                        lname = strlen(option_name(opt));
                        if (strncmp(option_name(opt), buf + offset, lname) == 0 &&
                            buf[offset + lname] == ':')
                        {
                            if (strncmp("yes", buf + offset + lname + 1, 3) == 0 &&
                                contains_only_spaces(buf + offset + lname + 4))
                            {
                                opts[opt] = true;
                            }
                            else if (strncmp("no", buf + offset + lname + 1, 2) == 0 &&
                                contains_only_spaces(buf + offset + lname + 3))
                            {
                                opts[opt] = false;
                            }
                            else
                                plog_fmt("Value at line %d of the customized birth options is not yes or no.", linenum);
                            break;
                        }
                        ++opt;
                    }
                }
                else
                {
                    offset = 0;
                    if (sscanf(buf, "#%n*s", &offset) == 0 && offset == 0 &&
                        !contains_only_spaces(buf))
                    {
                        plog_fmt("Line %d of the customized birth options is not parseable.", linenum);
                    }
                }
                ++linenum;
            }
            if (!file_close(f))
                success = false;
        }
        else
            success = false;
    }
    else
        options_reset_birth(opts);

    return success;
}


/*
 * Reset the birth options to the maintainer's defaults.
 */
void options_reset_birth(bool *opts)
{
    int opt;

    for (opt = 0; opt < OPT_MAX; opt++)
        opts[opt] = option_normal(opt);
}


/*
 * Initialise options package
 */
void init_options(bool *opts)
{
    /* Allocate options to pages */
    option_init();

    /* Set defaults */
    options_reset_birth(opts);

    /* Override with customized birth options. */
    options_restore_custom_birth(opts);
}
