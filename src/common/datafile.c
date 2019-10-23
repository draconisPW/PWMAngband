/*
 * File: datafile.c
 * Purpose: Data file reading and writing routines
 *
 * Copyright (c) 2017 Nick McConnell
 * Copyright (c) 2018 MAngband and PWMAngband Developers
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


const char *parser_error_str[PARSE_ERROR_MAX + 1] =
{
    #define PARSE_ERROR(a, b) b,
    #include "list-parser-errors.h"
    #undef PARSE_ERROR
    NULL
};


/*
 * Datafile parsing routines
 */


/*
 * More angband-specific bits of the parser
 * These require hooks into other parts of the code, and are a candidate for
 * moving elsewhere.
 */
static void print_error(struct file_parser *fp, struct parser *p)
{
    struct parser_state s;

    parser_getstate(p, &s);
    plog_fmt("Parse error in %s line %d column %d: %s: %s", fp->name,
        s.line, s.col, s.msg, parser_error_str[s.error]);
    quit_fmt("Parse error in %s line %d column %d.", fp->name, s.line, s.col);
}


errr run_parser(struct file_parser *fp)
{
    struct parser *p = fp->init();
    errr r;

    if (!p) return PARSE_ERROR_GENERIC;
    r = fp->run(p);
    if (r)
    {
        print_error(fp, p);
        return r;
    }
    r = fp->finish(p);
    if (r) print_error(fp, p);

    return r;
}


/*
 * The basic file parsing function. Attempt to load filename through
 * parser and perform a quit if the file is not found.
 */
errr parse_file_quit_not_found(struct parser *p, const char *filename)
{
    errr parse_err = parse_file(p, filename);

    if (parse_err == PARSE_ERROR_NO_FILE_FOUND)
        quit_fmt("Cannot open '%s.txt'", filename);

    return parse_err;
}


/*
 * The basic file parsing function.
 */
errr parse_file(struct parser *p, const char *filename)
{
    char path[MSG_LEN];
    char buf[MSG_LEN];
    ang_file *fh;
    errr r = 0;

    /* The player can put a customised file in the user directory */
    path_build(path, sizeof(path), ANGBAND_DIR_USER, format("%s.txt", filename));
    fh = file_open(path, MODE_READ, FTYPE_TEXT);

    /* If no custom file, just load the standard one */
    if (!fh)
    {
        path_build(path, sizeof(path), ANGBAND_DIR_GAMEDATA, format("%s.txt", filename));
        fh = file_open(path, MODE_READ, FTYPE_TEXT);
    }

    /* File wasn't found, return the error */
    if (!fh) return PARSE_ERROR_NO_FILE_FOUND;

    /* Parse it */
    while (file_getl(fh, buf, sizeof(buf)))
    {
        r = parser_parse(p, buf);
        if (r) break;
    }
    file_close(fh);

    return r;
}


void cleanup_parser(struct file_parser *fp)
{
    fp->cleanup();
}


int lookup_flag(const char **flag_table, const char *flag_name)
{
    int i = FLAG_START;

    while (flag_table[i] && !streq(flag_table[i], flag_name)) i++;

    /* End of table reached without match */
    if (!flag_table[i]) i = FLAG_END;

    return i;
}


/*
 * Gets a name and argument for a value expression of the form NAME[arg]
 *
 * name_and_value is the expression
 * string is the random value string to return (NULL if not required)
 * num is the integer to return (NULL if not required)
 */
static bool find_value_arg(char *value_name, char *string, int *num)
{
    char *t;

    /* Find the first bracket */
    for (t = value_name; *t && (*t != '['); ++t);

    /* Choose random_value value or int or fail */
    if (string)
    {
        /* Get the dice */
        if (1 != sscanf(t + 1, "%s", string))
            return false;
    }
    else if (num)
    {
        /* Get the value */
        if (1 != sscanf(t + 1, "%d", num))
            return false;
    }
    else
        return false;

    /* Terminate the string */
    *t = '\0';

    /* Success */
    return true;
}


/*
 * Get the random value argument from a value expression and put it into the
 * appropriate place in an array
 *
 * value the target array of values
 * value_type the possible value strings
 * name_and_value the value expression being matched
 *
 * Returns 0 if successful, otherwise an error value
 */
errr grab_rand_value(random_value *value, const char **value_type, const char *name_and_value)
{
    int i = 0;
    char value_name[NORMAL_WID];
    char dice_string[40];

    /* Get a rewritable string */
    my_strcpy(value_name, name_and_value, strlen(name_and_value));

    /* Parse the value expression */
    if (!find_value_arg(value_name, dice_string, NULL))
        return PARSE_ERROR_INVALID_VALUE;

    while (value_type[i] && !streq(value_type[i], value_name))
        i++;

    if (value_type[i])
    {
        dice_t *dice = dice_new();

        if (!dice_parse_string(dice, dice_string))
        {
            dice_free(dice);
            return PARSE_ERROR_NOT_RANDOM;
        }
        dice_random_value(dice, NULL, &value[i]);
        dice_free(dice);
    }

    return (value_type[i]? PARSE_ERROR_NONE: PARSE_ERROR_INTERNAL);
}


/*
 * Get the integer argument from a value expression and put it into the
 * appropriate place in an array
 *
 * value the target array of integers
 * value_type the possible value strings
 * name_and_value the value expression being matched
 *
 * Returns 0 if successful, otherwise an error value
 */
errr grab_int_value(int *value, const char **value_type, const char *name_and_value)
{
    int val, i = 0;
    char value_name[NORMAL_WID];

    /* Get a rewritable string */
    my_strcpy(value_name, name_and_value, strlen(name_and_value));

    /* Parse the value expression */
    if (!find_value_arg(value_name, NULL, &val))
        return PARSE_ERROR_INVALID_VALUE;

    while (value_type[i] && !streq(value_type[i], value_name))
        i++;

    if (value_type[i])
        value[i] = val;

    return (value_type[i]? PARSE_ERROR_NONE: PARSE_ERROR_INTERNAL);
}


/*
 * Get the integer argument from a value expression and the index in the
 * value_type array of the suffix used to build the value string
 *
 * value the integer value
 * index the information on where to put it (eg array index)
 * value_type the variable suffix of the possible value strings
 * prefix the constant prefix of the possible value strings
 * name_and_value the value expression being matched
 *
 * Returns 0 if successful, otherwise an error value
 */
errr grab_index_and_int(int *value, int *index, const char **value_type, const char *prefix,
    const char *name_and_value)
{
    int i;
    char value_name[NORMAL_WID];
    char value_string[NORMAL_WID];

    /* Get a rewritable string */
    my_strcpy(value_name, name_and_value, strlen(name_and_value));

    /* Parse the value expression */
    if (!find_value_arg(value_name, NULL, value))
        return PARSE_ERROR_INVALID_VALUE;

    /* Compose the value string and look for it */
    for (i = 0; value_type[i]; i++)
    {
        my_strcpy(value_string, prefix, sizeof(value_string));
        my_strcat(value_string, value_type[i], sizeof(value_string) - strlen(value_string));
        if (streq(value_string, value_name)) break;
    }

    if (value_type[i])
        *index = i;

    return (value_type[i]? PARSE_ERROR_NONE: PARSE_ERROR_INTERNAL);
}


/*
 * Get the integer argument from a slay value expression and the monster base
 * name it is slaying
 *
 * value the integer value
 * base the monster base name
 * name_and_value the value expression being matched
 *
 * Returns 0 if successful, otherwise an error value
 */
errr grab_base_and_int(int *value, char **base, const char *name_and_value)
{
    char value_name[NORMAL_WID];

    /* Get a rewritable string */
    my_strcpy(value_name, name_and_value, strlen(name_and_value));

    /* Parse the value expression */
    if (!find_value_arg(value_name, NULL, value))
        return PARSE_ERROR_INVALID_VALUE;

    /* Must be a slay */
    if (strncmp(value_name, "SLAY_", 5))
        return PARSE_ERROR_INVALID_VALUE;

    *base = string_make(value_name + 5);

    /* If we've got this far, assume it's a valid monster base name */
    return PARSE_ERROR_NONE;
}


errr grab_name(const char *from, const char *what, const char *list[], int max, int *num)
{
    int i;

    /* Scan list */
    for (i = 0; i < max; i++)
    {
        if (list[i] && streq(what, list[i]))
        {
            *num = i;
            return PARSE_ERROR_NONE;
        }
    }

    /* Oops */
    plog_fmt("Unknown %s '%s'.", from, what);

    /* Error */
    return PARSE_ERROR_GENERIC;
}


errr grab_flag(bitflag *flags, const size_t size, const char **flag_table, const char *flag_name)
{
    int flag = lookup_flag(flag_table, flag_name);

    if (flag == FLAG_END) return PARSE_ERROR_INVALID_FLAG;

    flag_on(flags, size, flag);

    return 0;
}


errr remove_flag(bitflag *flags, const size_t size, const char **flag_table, const char *flag_name)
{
    int flag = lookup_flag(flag_table, flag_name);

    if (flag == FLAG_END) return PARSE_ERROR_INVALID_FLAG;

    flag_off(flags, size, flag);

    return 0;
}