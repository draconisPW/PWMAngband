/*
 * File: ui-command.c
 * Purpose: Deal with UI only command processing.
 *
 * Copyright (c) 1997-2014 Angband developers
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


#include "c-angband.h"


/*
 * Redraw the screen
 */
void do_cmd_redraw(void)
{
    Send_redraw();
}


/*
 * Display the options and redraw afterward.
 */
void do_cmd_xxx_options(void)
{
    do_cmd_options();
    Send_redraw();
}


/*
 * Invoked when the command isn't recognized.
 */
void do_cmd_unknown(void)
{
    prt("Type '?' for help.", 0, 0);
}


/*
 * Verify the retire command
 */
void textui_cmd_retire(void)
{
    struct keypress ch;

    /* Verify */
    if (!get_check("Do you really want to retire? ")) return;

    /* Check again */
    topline_icky = true;
    prt("Please verify RETIRING THIS CHARACTER by typing the '@' sign: ", 0, 0);
    event_signal(EVENT_INPUT_FLUSH);
    ch = inkey();
    topline_icky = false;
    prt("", 0, 0);
    if (ch.code != '@') return;

    /* Send it */
    Send_retire();
}


/*
 * Get input for the rest command
 */
void textui_cmd_rest(void)
{
    const char *p = "Rest (1-9999, '!' HP or SP, '*' both, '&' full, 'm' till morning): ";
    char out_val[5] = "&";
    int16_t resting;

    /* Ask for duration */
    if (!get_string(p, out_val, sizeof(out_val))) return;

    /* Rest... */
    if (out_val[0] == '&') resting = REST_COMPLETE; /* ...until done */
    else if (out_val[0] == '*') resting = REST_ALL_POINTS; /* ...a lot */
    else if (out_val[0] == '!') resting = REST_SOME_POINTS; /* ...until HP or SP filled */
    else if (out_val[0] == 'm') resting = REST_MORNING; /* ...until morning */
    else if (out_val[0] == 'x') resting = REST_COMPLETE_NODISTURB; /* ...until done (no disturb) */
    else
    {
        /* ...some */
        resting = atoi(out_val);
        if (resting <= 0) return;
    }

    /* Paranoia */
    if (resting > 9999) resting = 9999;

    Send_rest(resting);
}


/*
 * Quit the game.
 */
void textui_quit(void)
{
    quit(NULL);
}


/*
 * Screenshot saving code
 */


/*
 * At a given location, determine the "current" attr and char.
 * Display walls and floors properly.
 */
static void Term_what_hack(int x, int y, uint16_t *a, char *c)
{
    /* Get the attr/char */
    Term_what(x, y, a, c);

    /* Hack -- display walls and floors properly */
    if (*c == 7) *c = '.';
    if (*c == 8) *c = '.';
    if (*c == 127) *c = '#';
}


static void write_html_escape_char(ang_file *fp, char c)
{
    switch (c)
    {
        case '<':
            file_put(fp, "&lt;");
            break;
        case '>':
            file_put(fp, "&gt;");
            break;
        case '&':
            file_put(fp, "&amp;");
            break;
        default:
            file_putf(fp, "%c", c);
            break;
    }
}


/*
 * Take an html screenshot
 */
static void html_screenshot(const char *path, int mode, term *other_term)
{
    /* Put the contents of the other terminal on the right by default. */
    bool other_left = false;

    int y, x;
    int main_wid, main_hgt, other_wid, other_hgt, wid, hgt;
    int main_xst, other_xst;
    uint16_t a = COLOUR_WHITE;
    uint16_t oa = COLOUR_WHITE;
    uint16_t fg_colour = COLOUR_WHITE;
    uint16_t bg_colour = COLOUR_DARK;
    char c = ' ';
    term *main_term = Term;
    const char *new_color_fmt = "<font color=\"#%02X%02X%02X\" style=\"background-color: #%02X%02X%02X\">";
    const char *change_color_fmt = ((mode == 0)?
        "</font><font color=\"#%02X%02X%02X\" style=\"background-color: #%02X%02X%02X\">":
        "[/COLOR][COLOR=\"#%02X%02X%02X\"]");
    const char *close_color_str = "</font>";
    ang_file *fp;

    fp = file_open(path, MODE_WRITE, FTYPE_TEXT);

    /* Oops */
    if (!fp)
    {
        plog_fmt("Cannot write the '%s' file!", path);
        return;
    }

    /* Retrieve current screen size */
    Term_get_size(&main_wid, &main_hgt);
    if (other_term)
    {
        Term_activate(other_term);
        Term_get_size(&other_wid, &other_hgt);
        Term_activate(main_term);
    }
    else
    {
        other_wid = 0;
        other_hgt = 0;
    }
    if (other_left)
    {
        other_xst = 0;
        main_xst = (other_wid > 0)? other_wid + 1: 0;
    }
    else
    {
        other_xst = main_wid + 1;
        main_xst = 0;
    }
    hgt = MAX(main_hgt, other_hgt);
    wid = (other_wid > 0)? main_wid + other_wid + 1: main_wid;

    if (mode == 0)
    {
        file_put(fp, "<!DOCTYPE html><html><head>\n");
        file_putf(fp, "  <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\n");
        file_putf(fp, "  <meta name='generator' content='%s'>\n", version_build(VERSION_NAME, false));
        file_putf(fp, "  <title>%s</title>\n", path);
        file_put(fp, "</head>\n\n");
        file_putf(fp, "<body style='color: #%02X%02X%02X; background: #%02X%02X%02X;'>\n",
            angband_color_table[COLOUR_WHITE][1],
            angband_color_table[COLOUR_WHITE][2],
            angband_color_table[COLOUR_WHITE][3],
            angband_color_table[COLOUR_DARK][1],
            angband_color_table[COLOUR_DARK][2],
            angband_color_table[COLOUR_DARK][3]);
        file_put(fp, "<pre>\n");
    }
    else
    {
        file_putf(fp, "[CODE][TT][BC=\"#%02X%02X%02X\"][COLOR=\"#%02X%02X%02X\"]\n",
            angband_color_table[COLOUR_DARK][1],
            angband_color_table[COLOUR_DARK][2],
            angband_color_table[COLOUR_DARK][3],
            angband_color_table[COLOUR_WHITE][1],
            angband_color_table[COLOUR_WHITE][2],
            angband_color_table[COLOUR_WHITE][3]);
    }

    /* Dump the screen */
    for (y = 0; y < hgt; y++)
    {
        for (x = 0; x < wid; x++)
        {
            /* Get the attr/char */
            if (x >= main_xst && x < main_xst + main_wid && y < main_hgt)
                Term_what_hack(x - main_xst, y, &a, &c);
            else if (x >= other_xst && x < other_xst + other_wid && y < other_hgt)
            {
                if (x == other_xst) Term_activate(other_term);
                Term_what_hack(x - other_xst, y, &a, &c);
                if (x == other_xst + other_wid - 1) Term_activate(main_term);
            }
            else
            {
                a = COLOUR_WHITE;
                c = ' ';
            }

            /* Set the foreground and background */
            fg_colour = (a % MAX_COLORS);
            switch (a / MULT_BG)
            {
                case BG_BLACK: bg_colour = COLOUR_DARK; break;
                case BG_SAME: bg_colour = fg_colour; break;
                case BG_DARK: bg_colour = COLOUR_SHADE; break;
                default: assert(a < BG_MAX * MULT_BG);
            }

            /*
             * Color change (for forum text, ignore changes if the character is
             * a space since the forum software strips [COLOR][/COLOR] elements that
             * only contain whitespace)
             */
            if (oa != a && (mode == 0 || c != L' '))
            {
                /* From the default white to another color */
                if (oa == COLOUR_WHITE && mode == 0)
                {
                    file_putf(fp, new_color_fmt,
                        angband_color_table[fg_colour][1],
                        angband_color_table[fg_colour][2],
                        angband_color_table[fg_colour][3],
                        angband_color_table[bg_colour][1],
                        angband_color_table[bg_colour][2],
                        angband_color_table[bg_colour][3]);
                }

                /* From another color to the default white */
                else if ((fg_colour == COLOUR_WHITE) && (bg_colour == COLOUR_DARK) && (mode == 0))
                    file_putf(fp, "%s", close_color_str);

                /* Change colors */
                else
                {
                    file_putf(fp, change_color_fmt,
                        angband_color_table[fg_colour][1],
                        angband_color_table[fg_colour][2],
                        angband_color_table[fg_colour][3],
                        angband_color_table[bg_colour][1],
                        angband_color_table[bg_colour][2],
                        angband_color_table[bg_colour][3]);
                }

                /* Remember the last color */
                oa = a;
            }

            /* Write the character and escape special HTML characters */
            if (mode == 0) write_html_escape_char(fp, c);
            else file_putf(fp, "%c", c);
        }

        /* End the row */
        file_put(fp, "\n");
    }

    /* Close the last font-color tag if necessary */
    if (oa != COLOUR_WHITE && mode == 0) file_putf(fp, "%s", close_color_str);

    if (mode == 0)
    {
        file_put(fp, "</pre>\n");
        file_put(fp, "</body>\n");
        file_put(fp, "</html>\n");
    }
    else
        file_put(fp, "[/COLOR][/BC][/TT][/CODE]\n");

    /* Close it */
    file_close(fp);
}


/*
 * Hack -- save a screen dump to a file in html format
 */
static void do_cmd_save_screen_html(int mode, term *other_term)
{
    char tmp_val[256];

    /* Ask for a file */
    if (!get_file(((mode == 0)? "dump.html": "dump.txt"), tmp_val, sizeof(tmp_val))) return;

    c_msg_print(NULL);

    /* Dump the screen with raw character attributes */
    html_screenshot(tmp_val, mode, other_term);

    c_msg_print(mode? "Forum text screen dump saved.": "HTML screen dump saved.");
}


/*
 * Hack -- save a screen dump to a file
 */
void do_cmd_save_screen(void)
{
    ui_event ke;
    int mode;
    term *ml_term;

    /* Doesn't work when graphics are on */
    if (use_graphics)
    {
        c_msg_print("This feature is only implemented in ASCII mode.");
        return;
    }

    c_msg_print("Dump as (h)tml or (f)orum text?");

    ke = inkey_ex();
    if (is_exit(ke) || (ke.type != EVT_KBRD))
    {
        c_msg_print(NULL);
        return;
    }

    switch (ke.key.code)
    {
        case 'h': mode = 0; break;
        case 'f': mode = 1; break;
        default: c_msg_print(NULL); return;
    }

    ml_term = find_first_subwindow(PW_MONLIST);
    if (ml_term)
    {
        if (!get_check("Include monster list? ")) ml_term = NULL;
    }
    do_cmd_save_screen_html(mode, ml_term);
}


/*** Misc commands ***/


/*
 * Return whether a key triggers a running action.
 */
static bool is_running_keymap(struct keypress ch)
{
    int mode = (OPT(player, rogue_like_commands)? KEYMAP_MODE_ROGUE: KEYMAP_MODE_ORIG);
    const struct keypress *act = keymap_find(mode, ch);

    if (act)
    {
        unsigned char run_key = cmd_lookup_key(CMD_RUN, mode);
        const struct keypress *cur;

        for (cur = act; cur->type == EVT_KBRD; cur++)
        {
            if ((unsigned char)cur->code == run_key) return true;
        }
    }

    return false;
}


/*
 * Handle "target" and "look".
 *
 * Note that this code can be called from "get_aim_dir()".
 *
 * Currently, when "flag" is true, that is, when
 * "interesting" grids are being used, and a directional key is used, we
 * only scroll by a single panel, in the direction requested, and check
 * for any interesting grids on that panel.  The "correct" solution would
 * actually involve scanning a larger set of grids, including ones in
 * panels which are adjacent to the one currently scanned, but this is
 * overkill for this function.  XXX XXX
 *
 * Hack -- targeting/observing an "outer border grid" may induce
 * problems, so this is not currently allowed.
 *
 * The player can use the direction keys to move among "interesting"
 * grids in a heuristic manner, or the "space", "+", and "-" keys to
 * move through the "interesting" grids in a sequential manner, or
 * can enter "location" mode, and use the direction keys to move one
 * grid at a time in any direction.  The "t" (set target) command will
 * only target a monster (as opposed to a location) if the monster is
 * target_able and the "interesting" mode is being used.
 *
 * The current grid is described using the "look" method above, and
 * a new command may be entered at any time, but note that if the
 * "TARGET_LOOK" bit flag is set (or if we are in "location" mode,
 * where "space" has no obvious meaning) then "space" will scan
 * through the description of the current grid until done, instead
 * of immediately jumping to the next "interesting" grid.  This
 * allows the "target" command to retain its old semantics.
 *
 * The "*", "+", and "-" keys may always be used to jump immediately
 * to the next (or previous) interesting grid, in the proper mode.
 *
 * The "return" key may always be used to scan through a complete
 * grid description (forever).
 *
 * This command will cancel any old target, even if used from
 * inside the "look" command.
 *
 * 'mode' is one of TARGET_LOOK or TARGET_KILL.
 * 'x' and 'y' are the initial position of the target to be highlighted,
 * or -1 if no location is specified.
 * Returns true if a target has been successfully set, false otherwise.
 */
bool cmd_target_interactive(int mode)
{
    bool done = false;
    struct keypress query;

    target_icky_screen = true;

    Term->cursor_icky = true;

    /* Tell the server to init targeting */
    Send_target_interactive(mode, '\0', 1);

    /* Interact */
    while (!done)
    {
        keycode_t code;
        int step;

        /* Describe and Prompt */
        query = inkey();
        if (!query.code) continue;

        /* Hack -- roguelike keyset */
        code = (keycode_t)target_dir(query);
        if (code == 0) code = query.code;

        step = (is_running_keymap(query)? 10: 1);

        Send_target_interactive(mode, code, step);

        switch (query.code)
        {
            case ESCAPE:
            case 'q':
            case 'r':
            case 't':
            case '5':
            case '0':
            case '.':
            {
                done = true;
                break;
            }
        }
    }

    /* Reset cursor stuff */
    Term->cursor_icky = false;
    Term_set_cursor(false);

    /* Clear top line */
    prt("", 0, 0);

    target_icky_screen = false;
    if (full_icky_screen) Term_redraw();

    return true;
}


void cmd_chat_close(int n)
{
    char buf[NORMAL_WID];

    if (n)
    {
        /* Request channel leave */
        if (channels[n].name[0] == '#')
        {
            strnfmt(buf, sizeof(buf), "-%s", channels[n].name);
            Send_chan(buf);
        }

        /* Close locally */
        else
        {
            if (view_channel == n)
                cmd_chat_cycle(-1);

            channels[n].name[0] = '\0';
            channels[n].id = 0;

            if (player->main_channel == n)
                player->main_channel = 0;
            if (STRZERO(channels[view_channel].name))
                cmd_chat_cycle(+1);

            /* Redraw */
            player->upkeep->redraw |= PR_MESSAGE_CHAT;
        }
    }
    else
        Send_chan("");
}


void cmd_chat_cycle(int dir)
{
    int16_t new_channel = view_channel;

    while (true)
    {
        new_channel += dir;

        if (new_channel >= MAX_CHANNELS || new_channel < 0) return;
        if (STRZERO(channels[new_channel].name)) continue;

        break;
    }

    if (new_channel != view_channel)
    {
        /* Set new */
        view_channel = new_channel;
        player->on_channel[view_channel] = 0;

        /* Redraw */
        player->upkeep->redraw |= PR_MESSAGE_CHAT;
    }
}