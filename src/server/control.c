/*
 * File: control.c
 * Purpose: Support for the "remote console"
 *
 * Copyright (c) 2016 MAngband and PWMAngband Developers
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


#define CONSOLE_AUTH    1
#define CONSOLE_LISTEN  0
#define CONSOLE_WRITE   true
#define CONSOLE_READ    false


typedef void (*console_cb) (int ind, char *params);

typedef struct
{
    char* name;
    console_cb call_back;
    char* comment;
} console_command_ops;

extern console_command_ops console_commands[];

extern int command_len;


/*
 * Output some text to the console, if we are listening
 */
void console_print(char *msg, int chan)
{
    int i;
    sockbuf_t *console_buf_w;
    char terminator = '\n';
    byte *chan_ptr;
    bool hint;

    for (i = 0; i < MAX_PLAYERS; i++)
    {
        if (Conn_is_alive(i))
        {
            chan_ptr = Conn_get_console_channels(i);
            hint = false;
            if (chan_ptr[chan] || (chan == 0 &&
                ((hint = Conn_get_console_setting(i, CONSOLE_LISTEN)) == true)))
            {
                console_buf_w = (sockbuf_t*)console_buffer(i, false);
                if (!hint)
                {
                    /* Name channel */
                    Packet_printf(console_buf_w, "%s", channels[chan].name);
                    Packet_printf(console_buf_w, "%s", " ");
                }
                Packet_printf(console_buf_w, "%S%c", msg, (int)terminator);
                Sockbuf_flush(console_buf_w);
            }
        }
    }
}


/*
 * Return the list of players
 */
static void console_who(int ind, char *dummy)
{
    int k, num = 0;
    char brave[30];
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);

    /* Hack -- count players */
    for (k = 1; k <= NumPlayers; k++)
    {
        struct player *p = player_get(k);
        if (!(p->dm_flags & DM_SECRET_PRESENCE)) num++;
    }

    /* Packet header */
    Packet_printf(console_buf_w, "%s", format("%d players online\n", num));

    /* Scan the player list */
    for (k = 1; k <= NumPlayers; k++)
    {
        struct player *p = player_get(k);
        const char *batty = "";
        char *entry;

        /* Challenge options */
        strnfmt(brave, sizeof(brave), "a%s%s%s level",
            OPT_P(p, birth_no_ghost)? " brave": "",
            OPT_P(p, birth_no_recall)? " hardcore": "",
            OPT_P(p, birth_force_descend)? " diving": "");

        if (OPT_P(p, birth_fruit_bat)) batty = "(batty) ";

        /* Add an entry */
        entry = format("%s is %s %d %s %s %sat %d ft\n", p->name, brave, p->lev,
            p->race->name, p->clazz->name, batty, p->depth * 50);
        Packet_printf(console_buf_w, "%S", entry);
    }
    Sockbuf_flush(console_buf_w);
}


/*  
 * Utility function, change locally as required when testing
 */
static void console_debug(int ind, char *dummy)
{
    return;
}


/*  
 * Start listening to game server messages
 */
static void console_listen(int ind, char *channel)
{
    int i;
    byte *chan;

    if (channel && !STRZERO(channel))
    {
        chan = Conn_get_console_channels(ind);
        for (i = 0; i < MAX_CHANNELS; i++)
        {
            if (!strcmp(channels[i].name, channel))
            {
                chan[i] = 1;
                break;
            }
        }
    }
    Conn_set_console_setting(ind, CONSOLE_LISTEN, true);
}


/*
 * Return information about a specific player
 */
static void console_whois(int ind, char *name)
{
    int i, len;
    u16b major, minor, patch, extra;
    struct player *p = NULL, *p_ptr_search;
    char brave[30];
    const char *batty = "";
    char *entry;
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    char terminator = '\n';

    /* Find this player */
    for (i = 1; i <= NumPlayers; i++)
    {
        p_ptr_search = player_get(i);
        len = strlen(p_ptr_search->name);
        if (!my_strnicmp(p_ptr_search->name, name, len))
            p = p_ptr_search;
    }
    if (!p)
    {
        Packet_printf(console_buf_w, "%s%c", "No such player", (int)terminator);
        Sockbuf_flush(console_buf_w);
        return;
    }

    /* Output player information */
    strnfmt(brave, sizeof(brave), "a%s%s%s level",
        OPT_P(p, birth_no_ghost)? " brave": "",
        OPT_P(p, birth_no_recall)? " hardcore": "",
        OPT_P(p, birth_force_descend)? " diving": "");
    if (OPT_P(p, birth_fruit_bat)) batty = "(batty) ";

    /* General character description */
    entry = format("%s is %s %d %s %s %sat %d ft\n", p->name, brave, p->lev,
            p->race->name, p->clazz->name, batty, p->depth * 50);
    Packet_printf(console_buf_w, "%S", entry);

    /* Breakup the client version identifier */
    major = (p->version & 0xF000) >> 12;
    minor = (p->version & 0xF00) >> 8;
    patch = (p->version & 0xF0) >> 4;
    extra = (p->version & 0xF);

    /* Player connection info */
    Packet_printf(console_buf_w, "%S", format("(%s@%s [%s] v%d.%d.%d.%d)\n",
        p->other.full_name, p->hostname, p->addr, major, minor, patch, extra));

    /* Other interesting factoids */
    if (p->lives > 0)
        Packet_printf(console_buf_w, "%s", format("Has resurrected %d times.\n", p->lives));
    if (p->max_depth == 0)
        Packet_printf(console_buf_w, "%s%c", "Has never left the town!", (int)terminator);
    else
    {
        Packet_printf(console_buf_w, "%s",
            format("Has ventured down to %d ft\n", p->max_depth * 50));
    }
    i = p->msg_hist_ptr - 1;
    if (i >= 0)
    {
        if (!STRZERO(p->msg_log[i]))
            Packet_printf(console_buf_w, "%S", format("Last message: %s\n", p->msg_log[i]));
    }

    Sockbuf_flush(console_buf_w);
}


static void console_message(int ind, char *buf)
{
    /* Send the message */
    do_cmd_message(NULL, buf);
}


static void console_kick_player(int ind, char *name)
{
    int i, len;
    struct player *p, *p_ptr_search;
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    char terminator = '\n';

    p = 0;

    /* Check the players in the game */
    for (i = 1; i <= NumPlayers; i++)
    {
        p_ptr_search = player_get(i);
        len = strlen(p_ptr_search->name);
        if (!my_strnicmp(p_ptr_search->name, name, len))
        {
            p = p_ptr_search;
            break;
        }
    }

    /* Check name */
    if (p)
    {
        /* Kick him */
        Destroy_connection(p->conn, "Kicked out");

        /* Success */
        Packet_printf(console_buf_w, "%s%c", "Kicked player", (int)terminator);
    }
    else
    {
        /* Failure */
        Packet_printf(console_buf_w, "%s%c", "No such player", (int)terminator);
    }
    Sockbuf_flush(console_buf_w);
}


/*  
 * Test the integrity of the RNG
 */
static void console_rng_test(int ind, char *dummy)
{
    u32b outcome;
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    char terminator = '\n';

    /* This is the expected outcome, generated on our reference platform */
    u32b reference = 0x08EACDD3;

    /* Don't run this if any players are connected */
    if (NumPlayers > 0)
    {
        Packet_printf(console_buf_w, "%s%c", "Can't run the RNG test with players connected!",
            (int)terminator);
        Sockbuf_flush(console_buf_w);
        return;
    }

    /* Let the operator know we are busy */
    Packet_printf(console_buf_w, "%s%c", "Torturing the RNG for 100 million iterations...",
        (int)terminator);
    Sockbuf_flush(console_buf_w);

    /* Torture the RNG for a hundred million iterations */
    outcome = Rand_test(0xDEADDEAD);

    /* Display the results */
    if (outcome == reference)
        Packet_printf(console_buf_w, "%s%c", "RNG is working perfectly", (int)terminator);
    else
    {
        Packet_printf(console_buf_w, "%s%c", "RNG integrity check FAILED", (int)terminator);
        Packet_printf(console_buf_w, "%s",
            format("Outcome was 0x%08X, expected 0x%08X\n", outcome, reference));
    }
    Sockbuf_flush(console_buf_w);
}


static void console_reload(int ind, char *mod)
{
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    bool done = false;
    char terminator = '\n';

    if (mod && !strcmp(mod, "config"))
    {
        /* Reload the server preferences */
        load_server_cfg();

        done = true;
    }
    else if (mod && !strcmp(mod, "news"))
    {
        /* Reload the news file */
        Init_setup();

        done = true;
    }

    /* Let mangconsole know that the command was a success */
    if (done)
    {
        /* Packet header */
        Packet_printf(console_buf_w, "%s%c", "Reloaded", (int)terminator);
    }
    else
    {
        /* Packet header */
        Packet_printf(console_buf_w, "%s%c", "Reload failed", (int)terminator);
    }

    /* Write the output */
    Sockbuf_flush(console_buf_w);
}


static void console_shutdown(int ind, char *dummy)
{
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    char terminator = '\n';

    /* Packet header */
    Packet_printf(console_buf_w, "%s%c", "Server shutdown", (int)terminator);

    /* Write the output */
    Sockbuf_flush(console_buf_w);

    /* Shutdown */
    shutdown_server();
}


static void console_wrath(int ind, char *name)
{
    int i;
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    char terminator = '\n';

    /* Check the players in the game */
    for (i = 1; i <= NumPlayers; i++)
    {
        struct player *p = player_get(i);

        /* Check name */
        if (!strcmp(name, p->name))
        {
            /* Mark as permanent death */
            p->alive = false;

            /* Note cause of death */
            my_strcpy(p->died_from, "divine wrath", sizeof(p->died_from));

            /* Record cause of death */
            player_death_info(p, "divine wrath");

            /* Mark as cheater */
            p->noscore = 1;

            /* Kill him */
            player_death(p);

            /* Success */
            Packet_printf(console_buf_w, "%s%c", "Wrathed player", (int)terminator);
            Sockbuf_flush(console_buf_w);

            return;
        }
    }

    /* Failure */
    Packet_printf(console_buf_w, "%s%c", "No such player", (int)terminator);
    Sockbuf_flush(console_buf_w);
}


/* Return list of available console commands */
static void console_help(int ind, char *name)
{
    sockbuf_t *console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    int i;
    bool done = false;
    char terminator = '\n';

    /* Root */
    if (!name || name[0] == ' ' || name[0] == '\0')
    {
        for (i = 0; i < command_len; i++)
        {
            Packet_printf(console_buf_w, "%s", console_commands[i].name);
            Packet_printf(console_buf_w, "%s", " ");
        }
        Packet_printf(console_buf_w, "%c", (int)terminator);
        done = true;
    }

    /* Specific command */
    else
    {
        for (i = 0; i < command_len; i++)
        {
            /* Found it */
            if (!strcmp(console_commands[i].name, name))
            {
                Packet_printf(console_buf_w, "%s", console_commands[i].name);
                Packet_printf(console_buf_w, "%s", " ");
                Packet_printf(console_buf_w, "%s", console_commands[i].comment);
                Packet_printf(console_buf_w, "%c", (int)terminator);
                done = true;
            }
        }
    }

    if (!done)
        Packet_printf(console_buf_w, "%s%c", "Unrecognized command", (int)terminator);

    Sockbuf_flush(console_buf_w);
}


/*
 * This is the response function when incoming data is received on the
 * control pipe.
 */
void NewConsole(int read_fd, int arg)
{
    char passwd[MSG_LEN], buf[MSG_LEN];
    char *params;
    int bytes, buflen, ind;
    int newsock = 0;
    sockbuf_t *console_buf_w = NULL;
    sockbuf_t *console_buf_r = NULL;
    int i, j;
    char terminator = '\n';

    if (arg < 0)
    {
        ind = abs(arg) - 1;
        arg = 1;
    }
    else
    {
        ind = arg;
        arg = 0;
    }

    console_buf_w = (sockbuf_t*)console_buffer(ind, CONSOLE_WRITE);
    console_buf_r = (sockbuf_t*)console_buffer(ind, CONSOLE_READ);

    /* Make a TCP connection */
    /* Hack -- check if this data has arrived on the contact socket or not.
     * If it has, then we have not created a connection with the client yet,
     * and so we must do so.
     */
    if (arg)
    {
        newsock = read_fd;
        if (newsock) remove_input(newsock);
        console_buf_r->sock = console_buf_w->sock = newsock;
        if (SetSocketNonBlocking(newsock, 1) == -1)
            plog("Can't make contact socket non-blocking");
        install_input(NewConsole, newsock, ind);
        Conn_set_console_setting(ind, CONSOLE_AUTH, false);
        Conn_set_console_setting(ind, CONSOLE_LISTEN, false);
        Sockbuf_clear(console_buf_w);
        Packet_printf(console_buf_w, "%s%c", "Connected", (int)terminator);
        Sockbuf_flush(console_buf_w);
        return;
    }

    newsock = console_buf_r->sock;

    /* Clear the buffer */
    Sockbuf_clear(console_buf_r);

    /* Read the message */
    bytes = DgramReceiveAny(read_fd, console_buf_r->buf, console_buf_r->size);

    /* If this happens our TCP connection has probably been severed. Remove the input. */
    if (!bytes && (errno != EAGAIN) && (errno != EWOULDBLOCK))
    {
        Destroy_connection(ind, "Console down");
        return;
    }
    if (bytes < 0)
    {
        /* Hack -- ignore these errors */
        if ((errno == EAGAIN) || (errno == EINTR))
        {
            GetSocketError(newsock);
            return;
        }

        /* We have a socket error, disconnect */
        Destroy_connection(ind, "Console down");
        return;
    }

    /* Set length */
    console_buf_r->len = bytes;

    /* Get the password if not authenticated */
    if (!Conn_get_console_setting(ind, CONSOLE_AUTH))
    {
        Packet_scanf(console_buf_r, "%N", passwd);

        /* Hack -- comply with telnet */
        buflen = strlen(passwd);
        if (buflen && passwd[buflen-1] == '\r') passwd[buflen-1] = '\0';

        /* Check for illegal accesses */
        if (!cfg_console_password || strcmp(passwd, cfg_console_password))
        {
            /* Clear buffer */
            Sockbuf_clear(console_buf_w);

            /* Put an "illegal access" reply in the buffer */
            Packet_printf(console_buf_w, "%s%c", "Invalid password", (int)terminator);

            /* Send it */
            DgramWrite(read_fd, console_buf_w->buf, console_buf_w->len);

            /* Log this to the local console */
            plog_fmt("Incorrect console password from %s.", DgramLastname());

            /* Kill him */
            Destroy_connection(ind, "Console down");
            return;
        }
        else
        {
            /* Clear buffer */
            Sockbuf_clear(console_buf_w);
            Conn_set_console_setting(ind, CONSOLE_AUTH, true);
            Packet_printf(console_buf_w, "%s%c", "Authenticated", (int)terminator);
            Sockbuf_flush(console_buf_w);
            return;
        }
    }

    /* Acquire command in the form: <command> <params> */
    Packet_scanf(console_buf_r, "%N", buf);
    buflen = strlen(buf);

    /* Hack -- comply with telnet */
    if (buflen && buf[buflen-1] == '\r') buf[buflen-1] = '\0';

    /* Split up command and params */
    params = strstr(buf, " ");
    if (params)
        *params++ = '\0';
    else
        params = NULL;

    /* Clear buffer */
    Sockbuf_clear(console_buf_r);

    /* Paranoia to ease ops-coder's life later */
    if (STRZERO(buf)) return;

    /* Execute console command */
    buflen = strlen(buf);
    for (i = 0; i < command_len; i++)
    {
        if (!strncmp(buf, console_commands[i].name,
            (j = strlen(console_commands[i].name))) && (buflen <= j || buf[j] == ' '))
        {
            (console_commands[i].call_back)(ind, params);
            break;
        }
    }
}


console_command_ops console_commands[] =
{
    {"help", console_help, "[TOPIC]\nExplain a command or list all avaliable"},
    {"listen", console_listen, "[CHANNEL]\nAttach self to #public or specified"},
    {"who", console_who, "\nList players"},
    {"shutdown", console_shutdown, "\nKill server"},
    {"msg", console_message, "MESSAGE\nBroadcast a message"},
    {"kick", console_kick_player, "PLAYERNAME\nKick player from the game"},
    {"wrath", console_wrath, "PLAYERNAME\nDelete (cheating) player from the game"},
    {"reload", console_reload, "config|news\nReload mangband.cfg or news.txt"},
    {"whois", console_whois, "PLAYERNAME\nDetailed player information"},
    {"rngtest", console_rng_test, "\nPerform RNG test"},
    {"debug", console_debug, "\nUnused"}
};

int command_len = sizeof(console_commands) / sizeof(console_command_ops);
