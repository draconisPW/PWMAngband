/*
 * File: win-layout.c
 * Purpose: Shape an initial or default display on windows systems.
 *
 * Copyright (c) 2012 Brett Reid
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
#include "../win/win-term.h"


bool arg_graphics_nice = true;
int arg_graphics = GRAPHICS_NONE;


/*
 * Graphics modes (hardcoded)
 */
#define GRAPHICS_32x32  3
#define GRAPHICS_16x16  4
#define GRAPHICS_64x64  5


/*
 * Default window layout function
 *
 * Just a big list of what to do for a specific screen resolution.
 *
 * Note: graphics modes are hardcoded, using info from
 * angband/lib/tiles/graphics.txt at the time of this writing
 *
 * Return values:    0 - Success
 *                  -1 - Invalid argument
 *                  -3 - Out of memory
 */
int default_layout_win(term_data *data, int maxterms)
{
    int sx, sy;
    int cap, lcap, bar, borderx, bordery, bottom;
    int mult_wid, mult_hgt;
    int fx, fy, tx, ty, fx2, fy2;
    int i, mode;
    RECT r;
    const char *main_font;
    const char *sub_font;

    /* Make sure the numbers used in this function are valid */
    if (maxterms < MAX_TERM_DATA) return -1;

    /* Get the various sizes that we need from windows */
    sx = GetSystemMetrics(SM_CXSCREEN);
    sy = GetSystemMetrics(SM_CYSCREEN);

    cap = GetSystemMetrics(SM_CYSMCAPTION);
    lcap = GetSystemMetrics(SM_CYCAPTION);
    bar = GetSystemMetrics(SM_CYMENU);
    borderx = GetSystemMetrics(SM_CXSIZEFRAME);
    bordery = GetSystemMetrics(SM_CYSIZEFRAME);

    SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);

    bottom = sy - r.bottom;

    if ((sx == 1024) && (sy == 768))
    {
        arg_graphics = GRAPHICS_32x32;
        arg_graphics_nice = false;
        tile_width = 4;
        tile_height = 2;

        string_free(data[0].font_want);
        data[0].font_want = string_make("8x12x.fon");
        data[0].font_wid = 8;
        data[0].font_hgt = 12;
        data[0].tile_wid = 8;
        data[0].tile_hgt = 16;
        data[0].pos_x = -1;
        data[0].pos_y = -1;
        data[0].cols = 103;
        data[0].rows = 35;
        data[0].visible = 1;
        data[0].maximized = 0;

        /* Messages window */
        string_free(data[1].font_want);
        data[1].font_want = string_make("6x10x.fon");
        data[1].font_wid = 6;
        data[1].font_hgt = 10;
        data[1].tile_wid = 6;
        data[1].tile_hgt = 10;
        data[1].pos_x = 0;
        data[1].pos_y = 596;
        data[1].cols = 80;
        data[1].rows = 10;
        data[1].visible = 1;
        data[1].maximized = 0;

        /* Equipment window */
        string_free(data[2].font_want);
        data[2].font_want = string_make("6x10x.fon");
        data[2].font_wid = 6;
        data[2].font_hgt = 10;
        data[2].tile_wid = 6;
        data[2].tile_hgt = 10;
        data[2].pos_x = 832;
        data[2].pos_y = -1;
        data[2].cols = 31;
        data[2].rows = 24;
        data[2].visible = 1;
        data[2].maximized = 0;

        /* Inventory window */
        string_free(data[3].font_want);
        data[3].font_want = string_make("6x10x.fon");
        data[3].font_wid = 6;
        data[3].font_hgt = 10;
        data[3].tile_wid = 6;
        data[3].tile_hgt = 10;
        data[3].pos_x = 832;
        data[3].pos_y = 266;
        data[3].cols = 31;
        data[3].rows = 30;
        data[3].visible = 1;
        data[3].maximized = 0;

        /* Message chat window */
        string_free(data[4].font_want);
        data[4].font_want = string_make("6x10x.fon");
        data[4].font_wid = 6;
        data[4].font_hgt = 10;
        data[4].tile_wid = 6;
        data[4].tile_hgt = 10;
        data[4].pos_x = 486;
        data[4].pos_y = 596;
        data[4].cols = 56;
        data[4].rows = 10;
        data[4].visible = 1;
        data[4].maximized = 0;

        /* Monster list window */
        string_free(data[5].font_want);
        data[5].font_want = string_make("6x10x.fon");
        data[5].font_wid = 6;
        data[5].font_hgt = 10;
        data[5].tile_wid = 6;
        data[5].tile_hgt = 10;
        data[5].pos_x = 832;
        data[5].pos_y = 596;
        data[5].cols = 31;
        data[5].rows = 10;
        data[5].visible = 1;
        data[5].maximized = 0;

        /* The rest of the terms were set by the load pref function */
        return 0;
    }
    if ((sx == 1280) && (sy == 1024))
    {
        arg_graphics = GRAPHICS_64x64;
        arg_graphics_nice = false;
        tile_width = 4;
        tile_height = 2;

        string_free(data[0].font_want);
        data[0].font_want = string_make("8x12x.fon");
        data[0].font_wid = 8;
        data[0].font_hgt = 12;
        data[0].tile_wid = 8;
        data[0].tile_hgt = 16;
        data[0].pos_x = -1;
        data[0].pos_y = -1;
        data[0].cols = 134;
        data[0].rows = 49;
        data[0].visible = 1;
        data[0].maximized = 0;

        /* Messages window */
        string_free(data[1].font_want);
        data[1].font_want = string_make("6x10x.fon");
        data[1].font_wid = 6;
        data[1].font_hgt = 10;
        data[1].tile_wid = 6;
        data[1].tile_hgt = 10;
        data[1].pos_x = 0;
        data[1].pos_y = 824;
        data[1].cols = 80;
        data[1].rows = 11;
        data[1].visible = 1;
        data[1].maximized = 0;

        /* Equipment window */
        string_free(data[2].font_want);
        data[2].font_want = string_make("6x10x.fon");
        data[2].font_wid = 6;
        data[2].font_hgt = 10;
        data[2].tile_wid = 6;
        data[2].tile_hgt = 10;
        data[2].pos_x = 1087;
        data[2].pos_y = 0;
        data[2].cols = 31;
        data[2].rows = 24;
        data[2].visible = 1;
        data[2].maximized = 0;

        /* Inventory window */
        string_free(data[3].font_want);
        data[3].font_want = string_make("6x10x.fon");
        data[3].font_wid = 6;
        data[3].font_hgt = 10;
        data[3].tile_wid = 6;
        data[3].tile_hgt = 10;
        data[3].pos_x = 1087;
        data[3].pos_y = 274;
        data[3].cols = 31;
        data[3].rows = 52;
        data[3].visible = 1;
        data[3].maximized = 0;

        /* Message chat window */
        string_free(data[4].font_want);
        data[4].font_want = string_make("6x10x.fon");
        data[4].font_wid = 6;
        data[4].font_hgt = 10;
        data[4].tile_wid = 6;
        data[4].tile_hgt = 10;
        data[4].pos_x = 493;
        data[4].pos_y = 824;
        data[4].cols = 70;
        data[4].rows = 11;
        data[4].visible = 1;
        data[4].maximized = 0;

        /* Monster list window */
        string_free(data[5].font_want);
        data[5].font_want = string_make("6x10x.fon");
        data[5].font_wid = 6;
        data[5].font_hgt = 10;
        data[5].tile_wid = 6;
        data[5].tile_hgt = 10;
        data[5].pos_x = 923;
        data[5].pos_y = 824;
        data[5].cols = 57;
        data[5].rows = 11;
        data[5].visible = 1;
        data[5].maximized = 0;

        /* The rest of the terms were set by the load pref function */
        return 0;
    }

    /* Size the main and subwindows procedurally */

    /* Only consider the working area */
    sy = sy - bottom;

    /* Get the basic info from the height of the screen */
    if (sy <= 250)
    {
        main_font = "5x8x.fon";
        fx = 5;
        fy = 8;
        tx = 4;
        ty = 8;
        sub_font = NULL;
        fx2 = 4;
        fy2 = 8;
        mode = 0;
        mult_wid = 1;
        mult_hgt = 1;
    }
    else if (sy <= 600)
    {
        main_font = "8x12x.fon";
        fx = 8;
        fy = 12;
        tx = 8;
        ty = 16;
        sub_font = NULL;
        fx2 = 6;
        fy2 = 10;
        mode = GRAPHICS_16x16;
        mult_wid = 1;
        mult_hgt = 1;
    }
    else if (sy <= 800)
    {
        main_font = "8x12x.fon";
        fx = 8;
        fy = 12;
        tx = 8;
        ty = 16;
        sub_font = "6x10x.fon";
        fx2 = 6;
        fy2 = 10;
        mode = GRAPHICS_32x32;
        mult_wid = 4;
        mult_hgt = 2;
    }
    else if (sy <= 1024)
    {
        main_font = "8x12x.fon";
        fx = 8;
        fy = 12;
        tx = 8;
        ty = 16;
        sub_font = "6x10x.fon";
        fx2 = 6;
        fy2 = 10;
        mode = GRAPHICS_64x64;
        mult_wid = 6;
        mult_hgt = 3;
    }
    else if (sy <= 1600)
    {
        main_font = "16x24x.fon";
        fx = 16;
        fy = 24;
        tx = 16;
        ty = 24;
        sub_font = "8x12x.fon";
        fx2 = 8;
        fy2 = 12;
        mode = GRAPHICS_64x64;
        mult_wid = 3;
        mult_hgt = 2;
    }
    else
    {
        main_font = "16x24x.fon";
        fx = 16;
        fy = 24;
        tx = 16;
        ty = 32;
        sub_font = "12x18x.fon";
        fx2 = 12;
        fy2 = 18;
        mode = GRAPHICS_64x64;
        mult_wid = 4;
        mult_hgt = 2;
    }

    /* Setup main window */
    arg_graphics = mode;
    arg_graphics_nice = false;
    tile_width = mult_wid;
    tile_height = mult_hgt;

    string_free(data[0].font_want);
    data[0].font_want = string_make(main_font);
    data[0].font_wid = fx;
    data[0].font_hgt = fy;
    data[0].tile_wid = tx;
    data[0].tile_hgt = ty;
    data[0].pos_x = 0;
    data[0].pos_y = 0;
    data[0].cols = (sx - 2 * borderx) / tx;
    data[0].rows = (sy - 2 * bordery - cap - bar) / ty;
    data[0].visible = 1;
    data[0].maximized = 0;

    /* Make sure there is a border around the map area */
    if (data[0].cols % mult_wid == 0)
        data[0].cols -= 1;
    if (data[0].rows % mult_hgt == 0)
        data[0].rows -= 1;

    /* No room for sub windows */
    if (!sub_font) return 0;

    data[0].cols = 1 + data[0].cols * 8 / 10;
    data[0].rows = 1 + data[0].rows * 8 / 10;

    /* Make sure there is a border around the map area */
    if (data[0].cols % mult_wid == 0)
        data[0].cols += 1;
    if (data[0].rows % mult_hgt == 0)
        data[0].rows += 1;

    /* Setup sub windows */
    for (i = 1; i < maxterms; i++)
    {
        string_free(data[i].font_want);
        data[i].font_want = string_make(sub_font);
        data[i].font_wid = fx2;
        data[i].font_hgt = fy2;
        data[i].tile_wid = fx2;
        data[i].tile_hgt = fy2;
        data[i].pos_x = i * tx;
        data[i].pos_y = i * ty;
        data[i].cols = (2 * ((sx - 2 * borderx) / tx) / 10) * tx / fx2;
        data[i].rows = (2 * ((sy - 2 * bordery - cap - bar) / ty) / 10) * ty / fy2;
        data[i].visible = 0;
        data[i].maximized = 0;
    }

    /* Position the specific sub windows */
    data[1].pos_x = 0;
    data[1].pos_y = (lcap + bar + (data[0].rows * ty) + 2 * bordery) - cap;
    data[1].rows = (sy - data[1].pos_y - 2 * cap) / fy2;

    data[2].pos_x = (2 * borderx + (data[0].cols * tx));
    data[2].pos_y = 0;
    data[2].cols = (sx - data[2].pos_x) / fx2;

    if (data[0].cols * tx > 160 * fx2)
    {
        data[1].cols = 80;
        data[4].cols = 60;
        data[5].visible = 1;
    }
    else if (data[0].cols * tx > 120 * fx2)
    {
        data[1].cols = 70;
        data[4].cols = ((data[0].cols * tx) - (data[1].cols * fx2)) / fx2;
        data[5].visible = 1;
    }
    else
    {
        data[1].cols = (data[0].cols * tx) * 6 / 10 / fx2;
        data[4].cols = (sx - (data[1].cols * fx2)) / fx2;
    }
    data[4].pos_x = -1 + borderx + (data[1].cols * fx2);
    data[4].pos_y = data[1].pos_y;
    data[4].rows = data[1].rows;
    if (data[5].visible)
    {
        data[5].pos_x = -1 + 2 * borderx + ((data[1].cols + data[4].cols) * fx2);
        data[5].pos_y = data[1].pos_y;
        data[5].rows = data[1].rows;
        data[5].cols = (sx - data[5].pos_x + 1) / fx2;
    }

    data[1].visible = 1;
    data[4].visible = 1;

    if (data[5].visible)
    {
        i = lcap + (data[0].rows * ty);
        if ((data[0].rows * ty) > (40 * fy2))
        {
            data[2].rows = 24;
            data[3].rows = (i - (data[2].rows * fy2) - 2 * cap - 2 * bordery) / fy2;
        }
        else if ((data[0].rows * ty) > (24 * fy2))
        {
            data[2].rows = 10;
            data[3].rows = (i - (data[2].rows * fy2) - 2 * cap - 2 * bordery) / fy2;
        }
        else
        {
            data[5].visible = 0;
            data[2].rows = (sy / 2) / fy2;
            if (data[2].rows > 24)
                data[2].rows = 24;

            if (data[2].rows < 13)
                data[2].rows = 3;
            data[3].rows = (sy - (data[2].rows * fy2) + cap - 2 * bordery) / fy2;
        }
    }
    else
    {
        i = lcap + (data[0].rows * ty);
        if ((data[0].rows * ty) > (40 * fy2))
        {
            data[2].rows = 24;
            data[3].rows = (i - (data[2].rows * fy2) - 2 * cap - 2 * bordery) / fy2;
        }
        else if ((data[0].rows * ty) > (24 * fy2))
        {
            data[2].rows = 10;
            data[3].rows = (i - (data[2].rows * fy2) - 2 * cap - 2 * bordery) / fy2;
        }
        else
        {
            data[2].rows = 10;
            data[3].rows = (i - (data[2].rows * fy2) - 2 * cap - 2 * bordery) / fy2;
        }
    }
    data[3].pos_x = data[2].pos_x;
    data[3].pos_y = -1 + 2 * bordery + (data[2].rows * fy2) + cap;
    data[3].cols = data[2].cols;

    data[2].visible = 1;
    data[3].visible = 1;

    return 0;
}
