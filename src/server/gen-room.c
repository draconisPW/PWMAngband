/*
 * File: gen-room.c
 * Purpose: Dungeon room generation
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2013 Erik Osheim, Nick McConnell
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
#include <math.h>


/*
 * This file covers everything to do with generation of individual rooms in
 * the dungeon. It consists of room generating helper functions plus the
 * actual room builders (which are referred to in the room profiles in
 * generate.c).
 *
 * The room builders all take as arguments the chunk they are being generated
 * in, and the co-ordinates of the room centre in that chunk. Each room
 * builder is also able to find space for itself in the chunk using the
 * find_space() function; the chunk generating functions can ask it to do that
 * by passing too large centre co-ordinates.
 */


/*
 * Selection of random templates
 */


/*
 * Chooses a room template of a particular kind at random.
 *
 * typ template room type to select
 * rating template room rating to select
 *
 * Returns a pointer to the room template.
 */
static struct room_template *random_room_template(int typ, int rating)
{
    struct room_template *t = room_templates;
    struct room_template *r = NULL;
    int n = 1;

    do
    {
        if ((t->typ == typ) && (t->rat == rating))
        {
            if (one_in_(n)) r = t;
            n++;
        }
        t = t->next;
    }
    while (t);

    return r;
}


/*
 * Chooses a vault of a particular kind at random.
 *
 * depth the current depth, for vault bounds checking
 * typ vault type
 *
 * Returns a pointer to the vault template.
 */
struct vault *random_vault(int depth, const char *typ)
{
    struct vault *v = vaults;
    struct vault *r = NULL;
    int n = 1;

    do
    {
        if (streq(v->typ, typ) && (v->min_lev <= depth) && (v->max_lev >= depth))
        {
            if (one_in_(n)) r = v;
            n++;
        }
        v = v->next;
    }
    while (v);

#ifdef DEBUG_MODE
    if (r) cheat(format("+v %s", r->name));
#endif

    return r;
}


/*
 * Helper functions to fill in information in the global dun (see also
 * find_space() and room_build() which set cent_n and cent in that structure)
 */


/*
 * Append a grid to the marked entrances for a room in the global dun.
 * grid Is the location for the entrance
 * Only call after the centre has been set and cent_n incremented.
 */
static void append_entrance(struct loc *grid)
{
    int ridx;
    struct loc sw;

    if (dun->cent_n <= 0 || dun->cent_n > z_info->level_room_max) return;
    ridx = dun->cent_n - 1;

    /* Expand allocated space if needed. */
    my_assert(dun->ent_n[ridx] >= 0);
    loc_init(&sw, -1, -1);
    if (!dun->ent[ridx] || loc_eq(&dun->ent[ridx][dun->ent_n[ridx]], &sw))
    {
        int alloc_n = ((dun->ent_n[ridx] > 0)? 2 * dun->ent_n[ridx]: 8);
        int i;

        dun->ent[ridx] = mem_realloc(dun->ent[ridx], alloc_n * sizeof(*dun->ent[ridx]));
        for (i = dun->ent_n[ridx] + 1; i < alloc_n - 1; ++i)
            memset(&dun->ent[ridx][i], 0, sizeof(struct loc));

        /* Add sentinel to track allocated size. */
        loc_copy(&dun->ent[ridx][alloc_n - 1], &sw);
    }

    /* Record the entrance */
    loc_copy(&dun->ent[ridx][dun->ent_n[ridx]], grid);
    ++dun->ent_n[ridx];
    dun->ent2room[grid->y][grid->x] = ridx;
}


/*
 * Room build helper functions
 */


/*
 * Mark squares as being in a room, and optionally light them.
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * light whether or not to light the room
 */
static void generate_room(struct chunk *c, int y1, int x1, int y2, int x2, int light)
{
    struct loc begin, end;
    struct loc_iterator iter;

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    do
    {
        sqinfo_on(square(c, &iter.cur)->info, SQUARE_ROOM);
        if (light) sqinfo_on(square(c, &iter.cur)->info, SQUARE_GLOW);
    }
    while (loc_iterator_next(&iter));
}


/*
 * Mark a rectangle with a set of info flags
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * flag the SQUARE_* flag we are marking with
 */
void generate_mark(struct chunk *c, int y1, int x1, int y2, int x2, int flag)
{
    struct loc begin, end;
    struct loc_iterator iter;

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    do
    {
        sqinfo_on(square(c, &iter.cur)->info, flag);
    }
    while (loc_iterator_next(&iter));
}


/*
 * Unmark a rectangle with a set of info flags
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * flag the SQUARE_* flag we are marking with
 */
void generate_unmark(struct chunk *c, int y1, int x1, int y2, int x2, int flag)
{
    struct loc begin, end;
    struct loc_iterator iter;

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    do
    {
        sqinfo_off(square(c, &iter.cur)->info, flag);
    }
    while (loc_iterator_next(&iter));
}


/*
 * Fill a rectangle with a feature.
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * feat the terrain feature
 * flag the SQUARE_* flag we are marking with
 */
void fill_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat, int flag)
{
    struct loc begin, end;
    struct loc_iterator iter;

    /* Paranoia */
    if ((x1 > x2) || (y1 > y2)) return;

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    do
    {
        square_set_feat(c, &iter.cur, feat);
    }
    while (loc_iterator_next(&iter));

    if (flag) generate_mark(c, y1, x1, y2, x2, flag);
}


/*
 * Fill the edges of a rectangle with a feature.
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * feat the terrain feature
 * flag the SQUARE_* flag we are marking with
 * overwrite_perm whether to overwrite features already marked as permanent
 */
void draw_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat, int flag,
    bool overwrite_perm)
{
    int y, x;
    struct loc grid;

    for (y = y1; y <= y2; y++)
    {
        loc_init(&grid, x1, y);
        if (overwrite_perm || !square_isperm(c, &grid)) square_set_feat(c, &grid, feat);
        loc_init(&grid, x2, y);
        if (overwrite_perm || !square_isperm(c, &grid)) square_set_feat(c, &grid, feat);
    }
    if (flag)
    {
        generate_mark(c, y1, x1, y2, x1, flag);
        generate_mark(c, y1, x2, y2, x2, flag);
    }

    for (x = x1; x <= x2; x++)
    {
        loc_init(&grid, x, y1);
        if (overwrite_perm || !square_isperm(c, &grid)) square_set_feat(c, &grid, feat);
        loc_init(&grid, x, y2);
        if (overwrite_perm || !square_isperm(c, &grid)) square_set_feat(c, &grid, feat);
    }
    if (flag)
    {
        generate_mark(c, y1, x1, y1, x2, flag);
        generate_mark(c, y2, x1, y2, x2, flag);
    }
}


/*
 * Fill a horizontal range with the given feature/info.
 *
 * c the current chunk
 * y, x1, x2 inclusive range boundaries
 * feat the terrain feature
 * flag the SQUARE_* flag we are marking with
 * light lit or not
 */
static void fill_xrange(struct chunk *c, int y, int x1, int x2, int feat, int flag, bool light)
{
    int x;

    for (x = x1; x <= x2; x++)
    {
        struct loc grid;

        loc_init(&grid, x, y);
        square_set_feat(c, &grid, feat);
        sqinfo_on(square(c, &grid)->info, SQUARE_ROOM);
        if (flag) sqinfo_on(square(c, &grid)->info, flag);
        if (light) sqinfo_on(square(c, &grid)->info, SQUARE_GLOW);
    }
}


/*
 * Fill a vertical range with the given feature/info.
 *
 * c the current chunk
 * x, y1, y2 inclusive range boundaries
 * feat the terrain feature
 * flag the SQUARE_* flag we are marking with
 * light lit or not
 */
static void fill_yrange(struct chunk *c, int x, int y1, int y2, int feat, int flag, bool light)
{
    int y;

    for (y = y1; y <= y2; y++)
    {
        struct loc grid;

        loc_init(&grid, x, y);
        square_set_feat(c, &grid, feat);
        sqinfo_on(square(c, &grid)->info, SQUARE_ROOM);
        if (flag) sqinfo_on(square(c, &grid)->info, flag);
        if (light) sqinfo_on(square(c, &grid)->info, SQUARE_GLOW);
    }
}


/*
 * Fill a circle with the given feature/info.
 *
 * c the current chunk
 * y0, x0 the circle centre
 * radius the circle radius
 * border the width of the circle border
 * feat the terrain feature
 * flag the SQUARE_* flag we are marking with
 * light lit or not
 */
static void fill_circle(struct chunk *c, int y0, int x0, int radius, int border, int feat, int flag,
    bool light)
{
    int i, last = 0;

    /* r2i2k2 is radius * radius - i * i - k * k. */
    int k, r2i2k2;

    for (i = 0, k = radius, r2i2k2 = 0; i <= radius; i++)
    {
        int b = border;

        if (border && (last > k)) b++;

        fill_xrange(c, y0 - i, x0 - k - b, x0 + k + b, feat, flag, light);
        fill_xrange(c, y0 + i, x0 - k - b, x0 + k + b, feat, flag, light);
        fill_yrange(c, x0 - i, y0 - k - b, y0 + k + b, feat, flag, light);
        fill_yrange(c, x0 + i, y0 - k - b, y0 + k + b, feat, flag, light);
        last = k;

        /* Update r2i2k2 and k for next i. */
        if (i < radius)
        {
            r2i2k2 -= 2 * i + 1;
            while (1)
            {
                /* The change to r2i2k2 if k is decreased by one. */
                int adj = 2 * k - 1;

                if (abs(r2i2k2 + adj) >= abs(r2i2k2)) break;
                --k;
                r2i2k2 += adj;
            }
        }
    }
}


/*
 * Fill the lines of a cross/plus with a feature.
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * feat the terrain feature
 * flag the SQUARE_* flag we are marking with
 *
 * When combined with draw_rectangle() this will generate a large rectangular
 * room which is split into four sub-rooms.
 */
static void generate_plus(struct chunk *c, int y1, int x1, int y2, int x2, int feat, int flag)
{
    int y, x;
    struct loc grid;

    /* Find the center */
    int y0 = (y1 + y2) / 2;
    int x0 = (x1 + x2) / 2;

    my_assert(c);

    for (y = y1; y <= y2; y++)
    {
        loc_init(&grid, x0, y);
        square_set_feat(c, &grid, feat);
    }
    if (flag) generate_mark(c, y1, x0, y2, x0, flag);
    for (x = x1; x <= x2; x++)
    {
        loc_init(&grid, x, y0);
        square_set_feat(c, &grid, feat);
    }
    if (flag) generate_mark(c, y0, x1, y0, x2, flag);
}


/*
 * Generate helper -- open all sides of a rectangle with a feature
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * feat the terrain feature
 */
static void generate_open(struct chunk *c, int y1, int x1, int y2, int x2, int feat)
{
    int y0, x0;
    struct loc grid;

    /* Center */
    y0 = (y1 + y2) / 2;
    x0 = (x1 + x2) / 2;

    /* Open all sides */
    loc_init(&grid, x0, y1);
    square_set_feat(c, &grid, feat);
    loc_init(&grid, x1, y0);
    square_set_feat(c, &grid, feat);
    loc_init(&grid, x0, y2);
    square_set_feat(c, &grid, feat);
    loc_init(&grid, x2, y0);
    square_set_feat(c, &grid, feat);
}


/*
 * Generate helper -- open one side of a rectangle with a feature
 *
 * c the current chunk
 * y1, x1, y2, x2 inclusive room boundaries
 * feat the terrain feature
 */
static void generate_hole(struct chunk *c, int y1, int x1, int y2, int x2, int feat)
{
    /* Find the center */
    int y0 = (y1 + y2) / 2;
    int x0 = (x1 + x2) / 2;

    struct loc grid;

    my_assert(c);

    /* Open random side */
    switch (randint0(4))
    {
        case 0: loc_init(&grid, x0, y1); break;
        case 1: loc_init(&grid, x1, y0); break;
        case 2: loc_init(&grid, x0, y2); break;
        case 3: loc_init(&grid, x2, y0); break;
    }

    square_set_feat(c, &grid, feat);

    /* Remove permanent flag */
    sqinfo_off(square(c, &grid)->info, SQUARE_FAKE);
}


/*
 * True if the square is normal open floor.
 * That floor may contain a feature mimic!
 *
 * c the current chunk
 * grid the square grid
 */
static bool square_isfloor_hack(struct chunk *c, struct loc *grid)
{
    struct monster *mon;

    if (square_isfloor(c, grid)) return true;
    mon = square_monster(c, grid);
    if (mon && (mon->race->base == lookup_monster_base("feature mimic")))
        return feat_is_floor(mon->feat);
    return false;
}


/*
 * Place a square of granite with a flag
 *
 * c the current chunk
 * y, x the square co-ordinates
 * flag the SQUARE_* flag we are marking with
 */
void set_marked_granite(struct chunk *c, struct loc *grid, int flag)
{
    square_set_feat(c, grid, FEAT_GRANITE);
    if (flag) generate_mark(c, grid->y, grid->x, grid->y, grid->x, flag);
}


/*
 * Given a room (with all grids converted to floors), convert floors on the
 * edges to outer walls so no floor will be adjacent to a grid that is not a
 * floor or outer wall.
 *
 * c the current chunk
 * y1 lower y bound for room's bounding box
 * x1 lower x bound for room's bounding box
 * y2 upper y bound for rooms' bounding box
 * x2 upper x bound for rooms' bounding box
 *
 * Will not properly handle cases where rooms are close enough that their
 * minimal bounding boxes overlap.
 */
static void set_bordering_walls(struct chunk *c, int y1, int x1, int y2, int x2)
{
    int nx;
    struct loc grid;
    bool *walls;

    my_assert((x2 >= x1) && (y2 >= y1));

    /* Set up storage to track which grids to convert. */
    nx = x2 - x1 + 1;
    walls = mem_zalloc((x2 - x1 + 1) * (y2 - y1 + 1) * sizeof(*walls));

    /* Find the grids to convert. */
    y1 = MAX(0, y1);
    y2 = MIN(c->height - 1, y2);
    x1 = MAX(0, x1);
    x2 = MIN(c->width - 1, x2);
    for (grid.y = y1; grid.y <= y2; grid.y++)
    {
        int adjy1 = MAX(0, grid.y - 1);
        int adjy2 = MIN(c->height - 1, grid.y + 1);

        for (grid.x = x1; grid.x <= x2; grid.x++)
        {
            if (square_isfloor_hack(c, &grid))
            {
                int adjx1 = MAX(0, grid.x - 1);
                int adjx2 = MIN(c->width - 1, grid.x + 1);

                my_assert(square_isroom(c, &grid));

                if ((adjy2 - adjy1 != 2) || (adjx2 - adjx1 != 2))
                {
                    /*
                     * Adjacent grids are out of bounds.
                     * Make it an outer wall.
                     */
                    walls[grid.x - x1 + nx * (grid.y - y1)] = true;
                }
                else
                {
                    int nfloor = 0;
                    struct loc adj;

                    for (adj.y = adjy1; adj.y <= adjy2; adj.y++)
                    {
                        for (adj.x = adjx1; adj.x <= adjx2; adj.x++)
                        {
                            bool floor = square_isfloor_hack(c, &adj);

                            my_assert(floor == square_isroom(c, &adj));
                            if (floor) ++nfloor;
                        }
                    }
                    if (nfloor != 9)
                    {
                        /*
                         * At least one neighbor is not
                         * in the room. Make it an
                         * outer wall.
                         */
                        walls[grid.x - x1 + nx * (grid.y - y1)] = true;
                    }
                }
            }
            else
            {
                my_assert(!square_isroom(c, &grid));
            }
        }
    }

    /* Perform the floor to wall conversions. */
    for (grid.y = y1; grid.y <= y2; grid.y++)
    {
        for (grid.x = x1; grid.x <= x2; grid.x++)
        {
            if (walls[grid.x - x1 + nx * (grid.y - y1)])
            {
                my_assert(square_isfloor_hack(c, &grid) && square_isroom(c, &grid));
                set_marked_granite(c, &grid, SQUARE_WALL_OUTER);
            }
        }
    }

    mem_free(walls);
}


/*
 * Make a starburst room.
 *
 * c the current chunk
 * y1, x1, y2, x2 boundaries which will contain the starburst
 * light lit or not
 * feat the terrain feature to make the starburst of
 * special_ok allow wacky cloverleaf rooms
 *
 * Starburst rooms are made in three steps:
 * 1: Choose a room size-dependent number of arcs.  Large rooms need to 
 *    look less granular and alter their shape more often, so they need 
 *    more arcs.
 * 2: For each of the arcs, calculate the portion of the full circle it 
 *    includes, and its maximum effect range (how far in that direction 
 *    we can change features in).  This depends on room size, shape, and 
 *    the maximum effect range of the previous arc.
 * 3: Use the table "get_angle_to_grid" to supply angles to each grid in 
 *    the room.  If the distance to that grid is not greater than the 
 *    maximum effect range that applies at that angle, change the feature 
 *    if appropriate (this depends on feature type).
 *
 * Usage notes:
 * - This function uses a table that cannot handle distances larger than 
 *   20, so it calculates a distance conversion factor for larger rooms.
 * - This function is not good at handling rooms much longer along one axis 
 *   than the other, so it divides such rooms up, and calls itself to handle
 *   each section.  
 * - It is safe to call this function on areas that might contain vaults or 
 *   pits, because "icky" and occupied grids are left untouched.
 *
 * - Mixing these rooms (using normal floor) with rectangular ones on a 
 *   regular basis produces a somewhat chaotic looking dungeon.  However, 
 *   this code does works well for lakes, etc.
 */
bool generate_starburst_room(struct chunk *c, int y1, int x1, int y2, int x2, bool light, int feat,
    bool special_ok)
{
    int ny, nx;
    int i, d;
    int dist, max_dist, dist_conv, dist_check;
    int height, width;
    int degree_first, center_of_arc, degree;

    /* Special variant room. Discovered by accident. */
    bool make_cloverleaf = false;

    /* Holds first degree of arc, maximum effect distance in arc. */
    int arc[45][2];

    /* Number (max 45) of arcs. */
    int arc_num;

    struct loc begin, end, grid0;
    struct loc_iterator iter;

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);

    /* Make certain the room does not cross the dungeon edge. */
    if (!square_in_bounds(c, &begin) || !square_in_bounds(c, &end))
        return false;

    /* Robustness -- test sanity of input coordinates. */
    if ((y1 + 2 >= y2) || (x1 + 2 >= x2))
        return false;

    /* Get room height and width. */
    height = 1 + y2 - y1;
    width = 1 + x2 - x1;

    /* Handle long, narrow rooms by dividing them up. */
    if ((height > 5 * width / 2) || (width > 5 * height / 2))
    {
        int tmp_ay, tmp_ax, tmp_by, tmp_bx;

        /* Get bottom-left borders of the first room. */
        tmp_ay = y2;
        tmp_ax = x2;
        if (height > width)
            tmp_ay = y1 + 2 * height / 3;
        else
            tmp_ax = x1 + 2 * width / 3;

        /* Make the first room. */
        generate_starburst_room(c, y1, x1, tmp_ay, tmp_ax, light, feat, false);

        /* Get top_right borders of the second room. */
        tmp_by = y1;
        tmp_bx = x1;
        if (height > width)
            tmp_by = y1 + 1 * height / 3;
        else
            tmp_bx = x1 + 1 * width / 3;

        /* Make the second room. */
        generate_starburst_room(c, tmp_by, tmp_bx, y2, x2, light, feat, false);

        /*
         * If floor, extend a "corridor" between room centers, to ensure
         * that the rooms are connected together.
         */
        if (feat_is_floor(feat))
        {
            loc_init(&begin, (x1 + tmp_ax) / 2, (y1 + tmp_ay) / 2);
            loc_init(&end, (tmp_bx + x2) / 2, (tmp_by + y2) / 2);
            loc_iterator_first(&iter, &begin, &end);

            do
            {
                square_set_feat(c, &iter.cur, feat);
            }
            while (loc_iterator_next(&iter));
        }

        /*
         * Otherwise fill any gap between two starbursts.
         */
        else
        {
            int tmp_cy1, tmp_cx1, tmp_cy2, tmp_cx2;

            if (height > width)
            {
                tmp_cy1 = y1 + (height - width) / 2;
                tmp_cx1 = x1;
                tmp_cy2 = tmp_cy1 - (height - width) / 2;
                tmp_cx2 = x2;
            }
            else
            {
                tmp_cy1 = y1;
                tmp_cx1 = x1 + (width - height) / 2;
                tmp_cy2 = y2;
                tmp_cx2 = tmp_cx1 + (width - height) / 2;
            }

            /* Make the third room. */
            generate_starburst_room(c, tmp_cy1, tmp_cx1, tmp_cy2, tmp_cx2, light, feat, false);
        }

        /* Return. */
        return true;
    }

    /* Get a shrinkage ratio for large rooms, as table is limited. */
    if ((width > 44) || (height > 44))
    {
        if (width > height)
            dist_conv = 10 * width / 44;
        else
            dist_conv = 10 * height / 44;
    }
    else
        dist_conv = 10;

    /* Make a cloverleaf room sometimes. */
    if (special_ok && (height > 10) && (randint0(20) == 0))
    {
        arc_num = 12;
        make_cloverleaf = true;
    }

    /* Usually, we make a normal starburst. */
    else
    {
        /* Ask for a reasonable number of arcs. */
        arc_num = 8 + (height * width / 80);
        arc_num = arc_num + 3 - randint0(7);
        if (arc_num < 8)
            arc_num = 8;
        if (arc_num > 45)
            arc_num = 45;
    }

    /* Get the center of the starburst. */
    loc_init(&grid0, x1 + width / 2, y1 + height / 2);

    /* Start out at zero degrees. */
    degree_first = 0;

    /* Determine the start degrees and expansion distance for each arc. */
    for (i = 0; i < arc_num; i++)
    {
        /* Get the first degree for this arc. */
        arc[i][0] = degree_first;

        /* Get a slightly randomized start degree for the next arc. */
        degree_first += (180 + randint0(arc_num)) / arc_num;
        if (degree_first < 180 * (i + 1) / arc_num)
            degree_first = 180 * (i + 1) / arc_num;
        if (degree_first > (180 + arc_num) * (i + 1) / arc_num)
            degree_first = (180 + arc_num) * (i + 1) / arc_num;

        /* Get the center of the arc. */
        center_of_arc = degree_first + arc[i][0];

        /* Calculate a reasonable distance to expand vertically. */
        if (((center_of_arc > 45) && (center_of_arc < 135)) ||
            ((center_of_arc > 225) && (center_of_arc < 315)))
        {
            arc[i][1] = height / 4 + randint0((height + 3) / 4);
        }

        /* Calculate a reasonable distance to expand horizontally. */
        else if (((center_of_arc < 45) || (center_of_arc > 315)) ||
            ((center_of_arc < 225) && (center_of_arc > 135)))
        {
            arc[i][1] = width / 4 + randint0((width + 3) / 4);
        }

        /* Handle arcs that count as neither vertical nor horizontal */
        else if (i != 0)
        {
            if (make_cloverleaf)
                arc[i][1] = 0;
            else
                arc[i][1] = arc[i - 1][1] + 3 - randint0(7);
        }

        /* Keep variability under control. */
        if (!make_cloverleaf && (i != 0) && (i != arc_num - 1))
        {
            /* Water edges must be quite smooth. */
            if (feat_is_smooth(feat))
            {
                if (arc[i][1] > arc[i - 1][1] + 2)
                    arc[i][1] = arc[i - 1][1] + 2;

                if (arc[i][1] > arc[i - 1][1] - 2)
                    arc[i][1] = arc[i - 1][1] - 2;
            }
            else
            {
                if (arc[i][1] > 3 * (arc[i - 1][1] + 1) / 2)
                    arc[i][1] = 3 * (arc[i - 1][1] + 1) / 2;

                if (arc[i][1] < 2 * (arc[i - 1][1] - 1) / 3)
                    arc[i][1] = 2 * (arc[i - 1][1] - 1) / 3;
            }
        }

        /* Neaten up final arc of circle by comparing it to the first. */
        if ((i == arc_num - 1) && (ABS(arc[i][1] - arc[0][1]) > 3))
        {
            if (arc[i][1] > arc[0][1])
                arc[i][1] -= randint0(arc[i][1] - arc[0][1]);
            else if (arc[i][1] < arc[0][1])
                arc[i][1] += randint0(arc[0][1] - arc[i][1]);
        }
    }

    /* Precalculate check distance. */
    dist_check = 21 * dist_conv / 10;

    loc_init(&begin, x1 + 1, y1 + 1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    /* Change grids between (and not including) the edges. */
    do
    {
        /* Do not touch vault grids. */
        if (square_isvault(c, &iter.cur)) continue;

        /* Do not touch occupied grids. */
        if (square_monster(c, &iter.cur)) continue;
        if (square_object(c, &iter.cur)) continue;

        /* Get distance to grid. */
        dist = distance(&grid0, &iter.cur);

        /* Reject grid if outside check distance. */
        if (dist >= dist_check)
            continue;

        /* Convert and reorient grid for table access. */
        ny = 20 + 10 * (iter.cur.y - grid0.y) / dist_conv;
        nx = 20 + 10 * (iter.cur.x - grid0.x) / dist_conv;

        /* Illegal table access is bad. */
        if ((ny < 0) || (ny > 40) || (nx < 0) || (nx > 40))
            continue;

        /* Get angle to current grid. */
        degree = get_angle_to_grid[ny][nx];

        /* Scan arcs to find the one that applies here. */
        for (i = arc_num - 1; i >= 0; i--)
        {
            if (arc[i][0] <= degree)
            {
                max_dist = arc[i][1];

                /* Must be within effect range. */
                if (max_dist >= dist)
                {
                    /* If new feature is not passable, or floor, always place it. */
                    if (feat_is_floor(feat) || !feat_is_passable(feat))
                    {
                        square_set_feat(c, &iter.cur, feat);

                        if (feat_is_floor(feat))
                        {
                            sqinfo_on(square(c, &iter.cur)->info, SQUARE_ROOM);
                            sqinfo_on(square(c, &iter.cur)->info, SQUARE_NO_STAIRS);
                        }
                        else
                            sqinfo_off(square(c, &iter.cur)->info, SQUARE_ROOM);

                        if (light)
                            sqinfo_on(square(c, &iter.cur)->info, SQUARE_GLOW);
                        else
                            square_unglow(c, &iter.cur);
                    }

                    /* If new feature is non-floor passable terrain, place it only over floor. */
                    else
                    {
                        /* Replace old feature entirely in some cases. */
                        if (feat_is_smooth(feat))
                        {
                            if (square_isfloor(c, &iter.cur))
                                square_set_feat(c, &iter.cur, feat);
                        }

                        /* Make denser in the middle. */
                        else
                        {
                            if (square_isfloor(c, &iter.cur) && (randint1(max_dist + 5) >= dist + 5))
                                square_set_feat(c, &iter.cur, feat);
                        }

                        /* Light grid. */
                        if (light)
                            sqinfo_on(square(c, &iter.cur)->info, SQUARE_GLOW);
                    }
                }

                /* Arc found. End search */
                break;
            }
        }
    }
    while (loc_iterator_next_strict(&iter));

    /*
    * If we placed floors or dungeon granite, all dungeon granite next
    * to floors needs to become outer wall.
    */
    if (feat_is_floor(feat) || (feat == FEAT_GRANITE))
    {
        loc_init(&begin, x1 + 1, y1 + 1);
        loc_init(&end, x2, y2);
        loc_iterator_first(&iter, &begin, &end);

        do
        {
            /* Floor grids only */
            if (square_isfloor(c, &iter.cur))
            {
                /* Look in all directions. */
                for (d = 0; d < 8; d++)
                {
                    struct loc adjacent;

                    /* Extract adjacent location */
                    loc_sum(&adjacent, &iter.cur, &ddgrid_ddd[d]);

                    /* Join to room, forbid stairs */
                    sqinfo_on(square(c, &adjacent)->info, SQUARE_ROOM);
                    sqinfo_on(square(c, &adjacent)->info, SQUARE_NO_STAIRS);

                    /* Illuminate if requested. */
                    if (light) sqinfo_on(square(c, &adjacent)->info, SQUARE_GLOW);

                    /* Look for dungeon granite. */
                    if (square(c, &adjacent)->feat == FEAT_GRANITE)
                    {
                        /* Mark as outer wall. */
                        set_marked_granite(c, &adjacent, SQUARE_WALL_OUTER);
                    }
                }
            }
        }
        while (loc_iterator_next_strict(&iter));
    }

    /* Success */
    return true;
}


/*
 * Hook for picking monsters appropriate to a nest/pit or region.
 *
 * race the race being tested for inclusion
 *
 * Returns if the race is acceptable.
 *
 * Requires dun->pit_type to be set.
 */
bool mon_pit_hook(struct monster_race *race)
{
    bool match_base = true;
    bool match_color = true;
    int freq_spell;

    my_assert(dun->pit_type);
    my_assert(race);

    freq_spell = dun->pit_type->freq_spell;

    /* Decline unique monsters */
    if (race_is_unique(race)) return false;

    /* Decline breeders */
    if (rf_has(race->flags, RF_MULTIPLY)) return false;

    /* Decline monsters that can kill other monsters */
    if (rf_has(race->flags, RF_KILL_BODY)) return false;

    /* Decline NO_PIT monsters */
    if (rf_has(race->flags, RF_NO_PIT)) return false;

    /* Decline PWMANG_BASE dragons */
    if (rf_has(race->flags, RF_DRAGON) && rf_has(race->flags, RF_PWMANG_BASE)) return false;

    if (!rf_is_subset(race->flags, dun->pit_type->flags)) return false;
    if (rf_is_inter(race->flags, dun->pit_type->forbidden_flags)) return false;
    if (!rsf_is_subset(race->spell_flags, dun->pit_type->spell_flags)) return false;
    if (rsf_is_inter(race->spell_flags, dun->pit_type->forbidden_spell_flags)) return false;
    if (race->freq_spell < freq_spell) return false;
    if (dun->pit_type->forbidden_monsters)
    {
        struct pit_forbidden_monster *monster;

        for (monster = dun->pit_type->forbidden_monsters; monster; monster = monster->next)
        {
            if (race == monster->race)
                return false;
        }
    }
    if (dun->pit_type->bases)
    {
        struct pit_monster_profile *bases;

        match_base = false;

        for (bases = dun->pit_type->bases; bases; bases = bases->next)
        {
            if (race->base == bases->base)
                match_base = true;
        }
    }

    if (dun->pit_type->colors)
    {
        struct pit_color_profile *colors;

        match_color = false;

        for (colors = dun->pit_type->colors; colors; colors = colors->next)
        {
            if (race->d_attr == colors->color)
                match_color = true;
        }
    }

    return (match_base && match_color);
}


/*
 * Pick a type of monster for pits (or other purposes), based on the level.
 *
 * We scan through all pit profiles, and for each one generate a random depth
 * using a normal distribution, with the mean given in pit.txt, and a
 * standard deviation of 10. Then we pick the profile that gave us a depth that
 * is closest to the player's actual depth.
 *
 * Sets dun->pit_type, which is required for mon_pit_hook.
 * depth is the pit profile depth to aim for in selection
 * type is 1 for pits, 2 for nests, 0 for any profile
 */
void set_pit_type(int depth, int type)
{
    int i;
    struct pit_profile *pit_type = NULL;

    /* Set initial distance large */
    int pit_dist = 999;

    for (i = 0; i < z_info->pit_max; i++)
    {
        int offset, dist;
        struct pit_profile *pit = &pit_info[i];

        /* Skip empty pits or pits of the wrong room type */
        if (type && (!pit->name || (pit->room_type != type))) continue;

        offset = Rand_normal(pit->ave, 10);
        dist = ABS(offset - depth);

        if ((dist < pit_dist) && one_in_(pit->rarity))
        {
            /* This pit is the closest so far */
            pit_type = pit;
            pit_dist = dist;
        }
    }

    dun->pit_type = pit_type;
}


/*
 * Find a good spot for the next room.
 *
 * "centre" centre of the room
 * height, width dimensions of the room
 *
 * Find and allocate a free space in the dungeon large enough to hold
 * the room calling this function.
 *
 * We allocate space in blocks.
 *
 * Be careful to include the edges of the room in height and width!
 *
 * Return true and values for the center of the room if all went well.
 * Otherwise, return false.
 */
static bool find_space(struct loc *centre, int height, int width)
{
    int i;
    int by, bx, by1, bx1, by2, bx2;
    bool filled;

    /* Find out how many blocks we need. */
    int blocks_high = 1 + ((height - 1) / dun->block_hgt);
    int blocks_wide = 1 + ((width - 1) / dun->block_wid);

    /* We'll allow twenty-five guesses. */
    for (i = 0; i < 25; i++)
    {
        filled = false;

        /* Pick a top left block at random */
        by1 = randint0(dun->row_blocks);
        bx1 = randint0(dun->col_blocks);

        /* Extract bottom right corner block */
        by2 = by1 + blocks_high - 1;
        bx2 = bx1 + blocks_wide - 1;

        /* Never run off the screen */
        if (by1 < 0 || by2 >= dun->row_blocks) continue;
        if (bx1 < 0 || bx2 >= dun->col_blocks) continue;

        /* Verify open space */
        for (by = by1; by <= by2; by++)
        {
            for (bx = bx1; bx <= bx2; bx++)
            {
                if (dun->room_map[by][bx])
                    filled = true;
            }
        }

        /* If space filled, try again. */
        if (filled) continue;

        /* Get the location of the room */
        centre->y = ((by1 + by2 + 1) * dun->block_hgt) / 2;
        centre->x = ((bx1 + bx2 + 1) * dun->block_wid) / 2;

        /* Save the room location */
        if (dun->cent_n < z_info->level_room_max)
        {
            loc_copy(&dun->cent[dun->cent_n], centre);
            dun->cent_n++;
        }

        /* Reserve some blocks */
        for (by = by1; by <= by2; by++)
            for (bx = bx1; bx <= bx2; bx++)
                dun->room_map[by][bx] = true;

        /* Success. */
        return true;
    }

    /* Failure. */
    return false;
}


/*
 * Build a room template from its string representation.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invoke find_space()
 * room the room template
 *
 * Returns success.
 */
static bool build_room_template(struct player *p, struct chunk *c, struct loc *centre,
    struct room_template *room)
{
    int dx, dy, rnddoors, doorpos;
    const char *t;
    bool rndwalls, light;

    /* Room dimensions */
    int ymax = room->hgt;
    int xmax = room->wid;

    /* Door position */
    int doors = room->dor;

    /* Room template text description */
    const char *data = room->text;

    /* Object type for any included objects */
    int tval = room->tval;

    /* Flags for the room */
    const bitflag *flags = room->flags;

    my_assert(c);

    /* Occasional light */
    light = ((c->wpos.depth <= randint1(25))? true: false);

    /*
     * Set the random door position here so it generates doors in all squares
     * marked with the same number
     */
    rnddoors = randint1(doors);

    /* Decide whether optional walls will be generated this time */
    rndwalls = one_in_(2);

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, ymax + 2, xmax + 2))
            return false;
    }

    /* Place dungeon features, objects, and monsters for specific grids. */
    for (t = data, dy = 0; (dy < ymax) && *t; dy++)
    {
        for (dx = 0; (dx < xmax) && *t; dx++, t++)
        {
            struct loc grid;

            /* Extract the location */
            loc_init(&grid, centre->x - (xmax / 2) + dx, centre->y - (ymax / 2) + dy);

            /* Skip non-grids */
            if (*t == ' ') continue;

            /* Lay down a floor */
            square_set_feat(c, &grid, FEAT_FLOOR);

            /* Debugging assertion */
            my_assert(square_isempty(c, &grid));

            /* Analyze the grid */
            switch (*t)
            {
                case '%':
                {
                    set_marked_granite(c, &grid, SQUARE_WALL_OUTER);
                    if (roomf_has(flags, ROOMF_FEW_ENTRANCES)) append_entrance(&grid);
                    break;
                }
                case '#': set_marked_granite(c, &grid, SQUARE_WALL_SOLID); break;
                case '+': place_closed_door(c, &grid); break;
                case '^': if (one_in_(4)) place_trap(c, &grid, -1, c->wpos.depth); break;
                case 'x':
                {
                    /* If optional walls are generated, put a wall in this square */
                    if (rndwalls) set_marked_granite(c, &grid, SQUARE_WALL_SOLID);
                    break;
                }
                case '(':
                {
                    /* If optional walls are generated, put a door in this square */
                    if (rndwalls) place_secret_door(c, &grid);
                    break;
                }
                case ')':
                {
                    /* If no optional walls are generated, put a door in this square */
                    if (!rndwalls)
                        place_secret_door(c, &grid);
                    else
                        set_marked_granite(c, &grid, SQUARE_WALL_SOLID);
                    break;
                }
                case '8':
                {
                    /* Put something nice in this square: Object (80%) or Stairs (20%) */
                    if (magik(80))
                        place_object(p, c, &grid, c->wpos.depth, false, false, ORIGIN_SPECIAL, 0);
                    else
                        place_random_stairs(c, &grid);

                    /* Place nearby guards in second pass. */
                    break;
                }
                case '9':
                {
                    /* Everything is handled in the second pass. */
                    break;
                }
                case '[':
                {
                    /* Place an object of the template's specified tval */
                    place_object(p, c, &grid, c->wpos.depth, false, false, ORIGIN_SPECIAL, tval);
                    break;
                }
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                {
                    /* Check if this is chosen random door position */
                    doorpos = (int)(*t - '0');

                    if (doorpos == rnddoors)
                        place_secret_door(c, &grid);
                    else
                        set_marked_granite(c, &grid, SQUARE_WALL_SOLID);

                    break;
                }
            }

            /* Part of a room */
            sqinfo_on(square(c, &grid)->info, SQUARE_ROOM);
            if (light) sqinfo_on(square(c, &grid)->info, SQUARE_GLOW);
        }
    }

    /*
     * Perform second pass for placement of monsters and objects at
     * unspecified locations after all the features are in place.
     */
    for (t = data, dy = 0; (dy < ymax) && *t; dy++)
    {
        for (dx = 0; (dx < xmax) && *t; dx++, t++)
        {
            struct loc grid;

            /* Extract the location */
            loc_init(&grid, centre->x - (xmax / 2) + dx, centre->y - (ymax / 2) + dy);

            /* Analyze the grid */
            switch (*t)
            {
                case '#':
                {
                    /* Check consistency with first pass. */
                    my_assert(square_isroom(c, &grid) && square_isrock(c, &grid) &&
                        sqinfo_has(square(c, &grid)->info, SQUARE_WALL_SOLID));

                    /* Convert to SQUARE_WALL_INNER if it does not touch the outside of the room. */
                    if (count_neighbors(NULL, c, &grid, square_isroom, false) == 8)
                    {
                        sqinfo_off(square(c, &grid)->info, SQUARE_WALL_SOLID);
                        sqinfo_on(square(c, &grid)->info, SQUARE_WALL_INNER);
                    }

                    break;
                }
                case '8':
                {
                    /* Check consistency with first pass. */
                    my_assert(square_isroom(c, &grid) &&
                        (square_isfloor_hack(c, &grid) || square_isstairs(c, &grid)));

                    /* Add some monsters to guard it */
                    vault_monsters(p, c, &grid, c->wpos.depth + 2, randint0(2) + 3);

                    break;
                }
                case '9':
                {
                    struct loc off2, off3, vgrid;

                    /* Create some interesting stuff nearby */
                    loc_init(&off2, 2, -2);
                    loc_init(&off3, 3, 3);

                    /* Check consistency with first pass. */
                    my_assert(square_isroom(c, &grid) && square_isfloor_hack(c, &grid));

                    /* Add a few monsters */
                    loc_diff(&vgrid, &grid, &off3);
                    vault_monsters(p, c, &vgrid, c->wpos.depth + randint0(2), randint1(2));
                    loc_sum(&vgrid, &grid, &off3);
                    vault_monsters(p, c, &vgrid, c->wpos.depth + randint0(2), randint1(2));

                    /* And maybe a bit of treasure */
                    loc_sum(&vgrid, &grid, &off2);
                    if (one_in_(2)) vault_objects(p, c, &vgrid, 1 + randint0(2));
                    loc_diff(&vgrid, &grid, &off2);
                    if (one_in_(2)) vault_objects(p, c, &vgrid, 1 + randint0(2));

                    break;
                }

                default:
                {
                    /* Everything was handled in the first pass. */
                    break;
                }
            }
        }
    }

    return true;
}


/*
 * Helper function for building room templates.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * typ template room type to select
 * rating template room rating to select
 *
 * Returns success.
 */
static bool build_room_template_type(struct player *p, struct chunk *c, struct loc *centre, int typ,
    int rating)
{
    struct room_template *room = random_room_template(typ, rating);

    if (room == NULL) return false;

    /* Build the room */
    if (!build_room_template(p, c, centre, room)) return false;

    return true;
}


/*
 * Build a vault from its string representation.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * v pointer to the vault template
 *
 * Returns success.
 */
bool build_vault(struct player *p, struct chunk *c, struct loc *centre, struct vault *v, bool find)
{
    const char *data = v->text;
    int y1, x1, y2, x2;
    int races_local = 0;
    const char *t;
    char racial_symbol[30] = "";
    bool icky;
    struct wild_type *w_ptr;
    struct loc begin, end, grid;
    struct loc_iterator iter;

    my_assert(c);

    /* Find and reserve some space in the dungeon. Get center of room. */
    if (((centre->y >= c->height) || (centre->x >= c->width)) && find)
    {
        if (!find_space(centre, v->hgt + 2, v->wid + 2))
            return false;
    }

    w_ptr = get_wt_info_at(&c->wpos.grid);

    /* Get the room corners */
    y1 = centre->y - (v->hgt / 2);
    x1 = centre->x - (v->wid / 2);
    y2 = y1 + v->hgt - 1;
    x2 = x1 + v->wid - 1;

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    /* Don't generate if we go out of bounds or if there is already something there */
    do
    {
        /* Be sure we are "in bounds" */
        if (!square_in_bounds_fully(c, &iter.cur)) return false;

        /* No object */
        if (square(c, &iter.cur)->obj) return false;

        /* Skip the DM */
        if ((square(c, &iter.cur)->mon < 0) && is_dm_p(player_get(0 - square(c, &iter.cur)->mon)))
            continue;

        /* No monster/player */
        if (square(c, &iter.cur)->mon) return false;
    }
    while (loc_iterator_next(&iter));

    /* No random monsters in vaults. */
    generate_mark(c, y1, x1, y2, x2, SQUARE_MON_RESTRICT);

    /* Place dungeon features and objects */
    for (t = data, grid.y = y1; (grid.y <= y2) && *t; grid.y++)
    {
        for (grid.x = x1; (grid.x <= x2) && *t; grid.x++, t++)
        {
            /* Skip non-grids */
            if (*t == ' ') continue;

            /* Lay down a floor */
            square_set_feat(c, &grid, FEAT_FLOOR);

            /* By default vault squares are marked icky */
            icky = true;

            /* Analyze the grid */
            switch (*t)
            {
                case '%':
                {
                    /*
                     * In this case, the square isn't really part of the
                     * vault, but rather is part of the "door step" to the
                     * vault. We don't mark it icky so that the tunneling
                     * code knows its allowed to remove this wall.
                     */
                    set_marked_granite(c, &grid, SQUARE_WALL_OUTER);
                    if (roomf_has(v->flags, ROOMF_FEW_ENTRANCES)) append_entrance(&grid);
                    icky = false;
                    break;
                }

                /* Inner or non-tunnelable outside granite wall */
                case '#': set_marked_granite(c, &grid, SQUARE_WALL_SOLID); break;

                /* Permanent wall */
                case '@': square_set_feat(c, &grid, FEAT_PERM); break;

                /* Gold seam */
                case '*':
                {
                    square_set_feat(c, &grid, (one_in_(2)? FEAT_MAGMA_K: FEAT_QUARTZ_K));
                    break;
                }

                /* Rubble */
                case ':':
                {
                    square_set_feat(c, &grid, (one_in_(2)? FEAT_PASS_RUBBLE: FEAT_RUBBLE));
                    break;
                }

                /* Secret door */
                case '+': place_secret_door(c, &grid); break;

                /* Trap */
                case '^': if (one_in_(4)) square_add_trap(c, &grid); break;

                /* Treasure or a trap */
                case '&':
                {
                    if (magik(75))
                        place_object(p, c, &grid, c->wpos.depth, false, false, ORIGIN_VAULT, 0);
                    else if (one_in_(4))
                        square_add_trap(c, &grid);
                    break;
                }

                /* Stairs */
                case '<':
                {
                    if (cfg_limit_stairs < 2) square_set_feat(c, &grid, FEAT_LESS);
                    break;
                }
                case '>':
                {
                    /* No down stairs at bottom */
                    if (c->wpos.depth == w_ptr->max_depth - 1)
                    {
                        if (cfg_limit_stairs < 2) square_set_feat(c, &grid, FEAT_LESS);
                    }
                    else
                        square_set_feat(c, &grid, FEAT_MORE);
                    break;
                }

                /* Lava */
                case '`': square_set_feat(c, &grid, FEAT_LAVA); break;

                /* Water */
                case '/': square_set_feat(c, &grid, FEAT_WATER); break;

                /* Tree */
                case ';': square_set_feat(c, &grid, FEAT_TREE); break;
            }

            /* Part of a vault */
            sqinfo_on(square(c, &grid)->info, SQUARE_ROOM);
            if (icky) sqinfo_on(square(c, &grid)->info, SQUARE_VAULT);
        }
    }

    /* Place regular dungeon monsters and objects, convert inner walls */
    for (t = data, grid.y = y1; (grid.y <= y2) && *t; grid.y++)
    {
        for (grid.x = x1; (grid.x <= x2) && *t; grid.x++, t++)
        {
            /* Skip non-grids */
            if (*t == ' ') continue;

            /* Most alphabetic characters signify monster races. */
            if (isalpha(*t) && (*t != 'x') && (*t != 'X'))
            {
                /* If the symbol is not yet stored, ... */
                if (!strchr(racial_symbol, *t))
                {
                    /* ... store it for later processing. */
                    if (races_local < 30) racial_symbol[races_local++] = *t;
                }
            }

            /* Otherwise, analyze the symbol */
            else switch (*t)
            {
                /* An ordinary monster, object (sometimes good), or trap. */
                case '1':
                {
                    if (one_in_(2))
                    {
                        pick_and_place_monster(p, c, &grid, c->wpos.depth, MON_ASLEEP | MON_GROUP,
                            ORIGIN_DROP_VAULT);
                    }
                    else if (one_in_(2))
                        place_object(p, c, &grid, c->wpos.depth, one_in_(8), false, ORIGIN_VAULT, 0);
                    else if (one_in_(4))
                        square_add_trap(c, &grid);
                    break;
                }

                /* Slightly out of depth monster. */
                case '2':
                {
                    pick_and_place_monster(p, c, &grid, c->wpos.depth + 5, MON_ASLEEP | MON_GROUP,
                        ORIGIN_DROP_VAULT);
                    break;
                }

                /* Slightly out of depth object. */
                case '3':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, false, false, ORIGIN_VAULT, 0);
                    break;
                }

                /* Monster and/or object */
                case '4':
                {
                    if (one_in_(2))
                    {
                        pick_and_place_monster(p, c, &grid, c->wpos.depth + 3,
                            MON_ASLEEP | MON_GROUP, ORIGIN_DROP_VAULT);
                    }
                    if (one_in_(2))
                        place_object(p, c, &grid, c->wpos.depth + 7, false, false, ORIGIN_VAULT, 0);
                    break;
                }

                /* Out of depth object. */
                case '5':
                {
                    place_object(p, c, &grid, c->wpos.depth + 7, false, false, ORIGIN_VAULT, 0);
                    break;
                }

                /* Out of depth monster. */
                case '6':
                {
                    pick_and_place_monster(p, c, &grid, c->wpos.depth + 11, MON_ASLEEP | MON_GROUP,
                        ORIGIN_DROP_VAULT);
                    break;
                }

                /* Very out of depth object. */
                case '7':
                {
                    place_object(p, c, &grid, c->wpos.depth + 15, false, false, ORIGIN_VAULT, 0);
                    break;
                }

                /* Very out of depth monster. */
                case '0':
                {
                    pick_and_place_monster(p, c, &grid, c->wpos.depth + 20, MON_ASLEEP | MON_GROUP,
                        ORIGIN_DROP_VAULT);
                    break;
                }

                /* Meaner monster, plus treasure */
                case '9':
                {
                    pick_and_place_monster(p, c, &grid, c->wpos.depth + 9, MON_ASLEEP | MON_GROUP,
                        ORIGIN_DROP_VAULT);
                    place_object(p, c, &grid, c->wpos.depth + 7, true, false, ORIGIN_VAULT, 0);
                    break;
                }

                /* Nasty monster and treasure */
                case '8':
                {
                    pick_and_place_monster(p, c, &grid, c->wpos.depth + 40, MON_ASLEEP | MON_GROUP,
                        ORIGIN_DROP_VAULT);
                    place_object(p, c, &grid, c->wpos.depth + 20, true, true, ORIGIN_VAULT, 0);
                    break;
                }

                /* A chest. */
                case '~':
                {
                    place_object(p, c, &grid, c->wpos.depth + 5, false, false, ORIGIN_VAULT,
                        TV_CHEST);
                    break;
                }

                /* Treasure. */
                case '$':
                {
                    place_gold(p, c, &grid, c->wpos.depth, ORIGIN_VAULT);
                    break;
                }

                /* Armour. */
                case ']':
                {
                    int temp = (one_in_(3)? randint0(9): randint0(8));

                    place_object(p, c, &grid, c->wpos.depth + 3, true, false, ORIGIN_VAULT,
                        TV_BOOTS + temp);
                    break;
                }

                /* Weapon (PWMAngband: allow diggers and mage staves). */
                case '|':
                {
                    int temp = randint0(6);

                    place_object(p, c, &grid, c->wpos.depth + 3, true, false, ORIGIN_VAULT,
                        TV_BOW + temp);
                    break;
                }

                /* Ring. */
                case '=':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, one_in_(4), false, ORIGIN_VAULT,
                        TV_RING);
                    break;
                }

                /* Amulet. */
                case '"':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, one_in_(4), false, ORIGIN_VAULT,
                        TV_AMULET);
                    break;
                }

                /* Potion. */
                case '!':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, one_in_(4), false, ORIGIN_VAULT,
                        TV_POTION);
                    break;
                }

                /* Scroll. */
                case '?':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, one_in_(4), false, ORIGIN_VAULT,
                        TV_SCROLL);
                    break;
                }

                /* Staff. */
                case '_':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, one_in_(4), false, ORIGIN_VAULT,
                        TV_STAFF);
                    break;
                }

                /* Wand or rod. */
                case '-':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, one_in_(4), false, ORIGIN_VAULT,
                        (one_in_(2)? TV_WAND: TV_ROD));
                    break;
                }

                /* Food. */
                case ',':
                {
                    place_object(p, c, &grid, c->wpos.depth + 3, one_in_(4), false, ORIGIN_VAULT,
                        TV_FOOD);
                    break;
                }

                /* Inner or non-tunnelable outside granite wall */
                case '#':
                {
                    /* Check consistency with first pass. */
                    my_assert(square_isroom(c, &grid) && square_isvault(c, &grid) &&
                        square_isrock(c, &grid) &&
                        sqinfo_has(square(c, &grid)->info, SQUARE_WALL_SOLID));

                    /* Convert to SQUARE_WALL_INNER if it does not touch the outside of the vault. */
                    if (count_neighbors(NULL, c, &grid, square_isroom, false) == 8)
                    {
                        sqinfo_off(square(c, &grid)->info, SQUARE_WALL_SOLID);
                        sqinfo_on(square(c, &grid)->info, SQUARE_WALL_INNER);
                    }

                    break;
                }

                /* Permanent wall */
                case '@':
                {
                    /* Check consistency with first pass. */
                    my_assert(square_isroom(c, &grid) && square_isvault(c, &grid) &&
                        square_isperm(c, &grid));

                    /* Mark as SQUARE_WALL_INNER if it does not touch the outside of the vault. */
                    if (count_neighbors(NULL, c, &grid, square_isroom, false) == 8)
                        sqinfo_on(square(c, &grid)->info, SQUARE_WALL_INNER);

                    break;
                }
            }
        }
    }

    /* Place specified monsters */
    get_vault_monsters(p, c, racial_symbol, v->typ, data, y1, y2, x1, x2);

    /* Success */
    return true;
}


/*
 * Helper function for building vaults.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * typ the vault type
 *
 * Returns success.
 */
static bool build_vault_type(struct player *p, struct chunk *c, struct loc *centre, const char *typ)
{
    struct vault *v = random_vault(c->wpos.depth, typ);

    if (v == NULL) return false;

    /* Medium vaults with a high rating have a rarity of (rating / 10) */
    if (streq(v->typ, "Medium vault") && !one_in_(v->rat / 10)) return false;

    /* Build the vault */
    if (!build_vault(p, c, centre, v, true))
        return false;

    /* Boost the rating */
    add_to_monster_rating(c, v->rat);

    return true;
}


/*
 * Helper for rooms of chambers; builds a marked wall grid if appropriate
 *
 * c the chunk the room is being built in
 * y, x co-ordinates
 */
static void make_inner_chamber_wall(struct chunk *c, struct loc *grid)
{
    if ((square(c, grid)->feat != FEAT_GRANITE) && (square(c, grid)->feat != FEAT_MAGMA))
        return;
    if (square_iswall_outer(c, grid)) return;
    if (square_iswall_solid(c, grid)) return;
    set_marked_granite(c, grid, SQUARE_WALL_INNER);
}


/*
 * Helper for rooms of chambers. Fill a room matching
 * the rectangle input with magma, and surround it with inner wall.
 * Create a door in a random inner wall grid along the border of the
 * rectangle.
 *
 * c the chunk the room is being built in
 * y1, x1, y2, x2 chamber dimensions
 */
static void make_chamber(struct chunk *c, int y1, int x1, int y2, int x2)
{
    int i, d;
    int count;
    struct loc grid;

    /* Fill with soft granite (will later be replaced with floor). */
    fill_rectangle(c, y1 + 1, x1 + 1, y2 - 1, x2 - 1, FEAT_MAGMA, SQUARE_NONE);

    /* Generate inner walls over dungeon granite and magma. */
    for (grid.y = y1; grid.y <= y2; grid.y++)
    {
        /* Left wall */
        grid.x = x1;
        make_inner_chamber_wall(c, &grid);

        /* Right wall */
        grid.x = x2;
        make_inner_chamber_wall(c, &grid);
    }

    for (grid.x = x1; grid.x <= x2; grid.x++)
    {
        /* Top wall */
        grid.y = y1;
        make_inner_chamber_wall(c, &grid);

        /* Bottom wall */
        grid.y = y2;
        make_inner_chamber_wall(c, &grid);
    }

    /* Try a few times to place a door. */
    for (i = 0; i < 20; i++)
    {
        /* Pick a square along the edge, not a corner. */
        if (one_in_(2))
        {
            /* Somewhere along the (interior) side walls. */
            grid.x = (one_in_(2)? x1: x2);
            grid.y = y1 + randint0(1 + ABS(y2 - y1));
        }
        else
        {
            /* Somewhere along the (interior) top and bottom walls. */
            grid.y = (one_in_(2)? y1: y2);
            grid.x = x1 + randint0(1 + ABS(x2 - x1));
        }

        /* If not an inner wall square, try again. */
        if (!square_iswall_inner(c, &grid)) continue;

        /* Paranoia */
        if (!square_in_bounds_fully(c, &grid)) continue;

        /* Reset wall count */
        count = 0;

        /* If square has not more than two adjacent walls, and no adjacent doors, place door. */
        for (d = 0; d < 9; d++)
        {
            struct loc adjacent;

            /* Extract adjacent (legal) location */
            loc_sum(&adjacent, &grid, &ddgrid_ddd[d]);

            /* No doors beside doors. */
            if (square(c, &adjacent)->feat == FEAT_OPEN) break;

            /* Count the inner walls. */
            if (square_iswall_inner(c, &adjacent)) count++;

            /* No more than two walls adjacent (plus the one we're on). */
            if (count > 3) break;

            /* Checked every direction? */
            if (d == 8)
            {
                /* Place an open door. */
                square_set_feat(c, &grid, FEAT_OPEN);

                /* Success. */
                return;
            }
        }
    }
}


/*
 * Expand in every direction from a start point, turning magma into rooms.
 * Stop only when the magma and the open doors totally run out.
 *
 * c the chunk the room is being built in
 * grid location to start hollowing
 */
static void hollow_out_room(struct chunk *c, struct loc *grid)
{
    int d;

    for (d = 0; d < 9; d++)
    {
        struct loc adjacent;

        /* Extract adjacent location */
        loc_sum(&adjacent, grid, &ddgrid_ddd[d]);

        /* Change magma to floor. */
        if (square(c, &adjacent)->feat == FEAT_MAGMA)
        {
            square_set_feat(c, &adjacent, FEAT_FLOOR);

            /* Hollow out the room. */
            hollow_out_room(c, &adjacent);
        }

        /* Change open door to broken door. */
        else if (square(c, &adjacent)->feat == FEAT_OPEN)
        {
            square_set_feat(c, &adjacent, FEAT_BROKEN);

            /* Hollow out the (new) room. */
            hollow_out_room(c, &adjacent);
        }
    }
}


/*
 * Room builders
 */


/*
 * Build a circular room (interior radius 4-7).
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 *
 * Returns success
 */
bool build_circular(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int radius;
    bool light;
    struct worldpos dpos;
    struct location *dungeon;

    /* Get the dungeon */
    wpos_init(&dpos, &c->wpos.grid, 0);
    dungeon = get_dungeon(&dpos);

    /* Some dungeons have circular and simple rooms swapped */
    if (dungeon && c->wpos.depth && df_has(dungeon->flags, DF_CIRCULAR_ROOMS) && !c->gen_hack)
    {
        c->gen_hack = true;
        return build_simple(p, c, centre, rating);
    }

    /* Pick a room size */
    radius = 2 + randint1(2) + randint1(3);

    /* Occasional light */
    light = ((c->wpos.depth <= randint1(25))? true: false);

    /* Find and reserve lots of space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, 2 * radius + 10, 2 * radius + 10))
        {
            c->gen_hack = false;
            return false;
        }
    }

    /* DF_CIRCULAR_ROOMS dungeons and arena levels use the old method */
    if (c->gen_hack || (c->profile == dun_arena))
    {
        /* Generate outer walls and inner floors */
        fill_circle(c, centre->y, centre->x, radius + 1, 1, FEAT_GRANITE, SQUARE_WALL_OUTER, light);
        fill_circle(c, centre->y, centre->x, radius, 0, FEAT_FLOOR, SQUARE_NONE, light);
    }
    else
    {
        /* Mark as a room. */
        fill_circle(c, centre->y, centre->x, radius + 1, 0, FEAT_FLOOR, SQUARE_NONE, light);

        /* Convert some floors to be the outer walls. */
        set_bordering_walls(c, centre->y - radius - 2, centre->x - radius - 2,
            centre->y + radius + 2, centre->x + radius + 2);
    }
    c->gen_hack = false;

    /* Especially large circular rooms will have a middle chamber */
    if ((radius - 4 > 0) && (randint0(4) < radius - 4))
    {
        /* Choose a random direction */
        struct loc offset, grid;

        rand_dir(&offset);

        /* Draw a room with a closed door on a random side */
        draw_rectangle(c, centre->y - 2, centre->x - 2, centre->y + 2, centre->x + 2, FEAT_GRANITE,
            SQUARE_WALL_INNER, false);
        loc_init(&grid, centre->x + offset.x * 2, centre->y + offset.y * 2);
        place_closed_door(c, &grid);

        /* Place a treasure in the vault */
        vault_objects(p, c, centre, randint0(2));

        /* Create some monsters */
        vault_monsters(p, c, centre, c->wpos.depth + 1, randint0(3));
    }

    return true;
}


/*
 * Builds a normal rectangular room.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 *
 * Returns success
 */
bool build_simple(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int y1, x1, y2, x2;
    int light = false;
    int height;
    int width;
    struct worldpos dpos;
    struct location *dungeon;

    /* Get the dungeon */
    wpos_init(&dpos, &c->wpos.grid, 0);
    dungeon = get_dungeon(&dpos);

    /* Some dungeons have circular and simple rooms swapped */
    if (dungeon && c->wpos.depth && df_has(dungeon->flags, DF_CIRCULAR_ROOMS) && !c->gen_hack)
    {
        c->gen_hack = true;
        return build_circular(p, c, centre, rating);
    }
    c->gen_hack = false;

    /* Pick a room size */
    height = 1 + randint1(4) + randint1(3);
    width = 1 + randint1(11) + randint1(11);

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, height + 2, width + 2))
            return false;
    }

    /* Set bounds */
    y1 = centre->y - height / 2;
    x1 = centre->x - width / 2;
    y2 = y1 + height - 1;
    x2 = x1 + width - 1;

    /* Occasional light */
    if (c->wpos.depth <= randint1(25)) light = true;
    
    /* Generate new room */
    generate_room(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, light);

    /* Generate outer walls and inner floors */
    draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);
    fill_rectangle(c, y1, x1, y2, x2, FEAT_FLOOR, SQUARE_NONE);

    /* Sometimes make a pillar room */
    if (one_in_(20))
    {
        struct loc grid;

        /*
         * If a dimension is even, don't always put a pillar in the
         * upper left corner.
         */
        int offx = ((x2 - x1) % 2 == 0)? 0: randint0(2);
        int offy = ((y2 - y1) % 2 == 0)? 0: randint0(2);

        for (grid.y = y1 + offy; grid.y <= y2; grid.y += 2)
            for (grid.x = x1 + offx; grid.x <= x2; grid.x += 2)
                set_marked_granite(c, &grid, SQUARE_WALL_INNER);

        /*
         * Drop room/outer wall flags on corners if not adjacent to a
         * floor. Lets tunnels enter those grids.
         */
        if (!offy)
        {
            if (!offx)
            {
                loc_init(&grid, x1 - 1, y1 - 1);
                sqinfo_off(square(c, &grid)->info, SQUARE_ROOM);
                sqinfo_off(square(c, &grid)->info, SQUARE_WALL_OUTER);
            }
            if ((x2 - x1 - offx) % 2 == 0)
            {
                loc_init(&grid, x2 + 1, y1 - 1);
                sqinfo_off(square(c, &grid)->info, SQUARE_ROOM);
                sqinfo_off(square(c, &grid)->info, SQUARE_WALL_OUTER);
            }
        }

        if ((y2 - y1 - offy) % 2 == 0)
        {
            if (!offx)
            {
                loc_init(&grid, x1 - 1, y2 + 1);
                sqinfo_off(square(c, &grid)->info, SQUARE_ROOM);
                sqinfo_off(square(c, &grid)->info, SQUARE_WALL_OUTER);
            }
            if ((x2 - x1 - offx) % 2 == 0)
            {
                loc_init(&grid, x2 + 1, y2 + 1);
                sqinfo_off(square(c, &grid)->info, SQUARE_ROOM);
                sqinfo_off(square(c, &grid)->info, SQUARE_WALL_OUTER);
            }
        }
    }

    /* Sometimes make a ragged-edge room */
    else if (one_in_(50))
    {
        struct loc grid;

        /*
         * If a dimension is even, don't always put the first
         * indentations at (x1, y1 + 2) and (x1 + 2, y1).
         */
        int offx = ((x2 - x1) % 2 == 0)? 0: randint0(2);
        int offy = ((y2 - y1) % 2 == 0)? 0: randint0(2);

        for (grid.y = y1 + 2 + offy; grid.y <= y2 - 2; grid.y += 2)
        {
            grid.x = x1;
            set_marked_granite(c, &grid, SQUARE_WALL_INNER);
            grid.x = x2;
            set_marked_granite(c, &grid, SQUARE_WALL_INNER);
        }
        for (grid.x = x1 + 2 + offx; grid.x <= x2 - 2; grid.x += 2)
        {
            grid.y = y1;
            set_marked_granite(c, &grid, SQUARE_WALL_INNER);
            grid.y = y2;
            set_marked_granite(c, &grid, SQUARE_WALL_INNER);
        }
    }

    return true;
}


/*
 * Builds an overlapping rectangular room.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 *
 * Returns success
 */
bool build_overlap(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int y1a, x1a, y2a, x2a;
    int y1b, x1b, y2b, x2b;
    int height, width;
    int light = false;

    /* Occasional light */
    if (c->wpos.depth <= randint1(25)) light = true;

    /* Determine extents of room (a) */
    y1a = randint1(4);
    x1a = randint1(11);
    y2a = randint1(3);
    x2a = randint1(10);

    /* Determine extents of room (b) */
    y1b = randint1(3);
    x1b = randint1(10);
    y2b = randint1(4);
    x2b = randint1(11);

    /* Calculate height and width */
    height = 2 * MAX(MAX(y1a, y2a), MAX(y1b, y2b)) + 1;
    width = 2 * MAX(MAX(x1a, x2a), MAX(x1b, x2b)) + 1;

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, height + 2, width + 2))
            return false;
    }

    /* locate room (a) */
    y1a = centre->y - y1a;
    x1a = centre->x - x1a;
    y2a = centre->y + y2a;
    x2a = centre->x + x2a;

    /* locate room (b) */
    y1b = centre->y - y1b;
    x1b = centre->x - x1b;
    y2b = centre->y + y2b;
    x2b = centre->x + x2b;

    /* Generate new room (a) */
    generate_room(c, y1a - 1, x1a - 1, y2a + 1, x2a + 1, light);

    /* Generate new room (b) */
    generate_room(c, y1b - 1, x1b - 1, y2b + 1, x2b + 1, light);

    /* Generate outer walls (a) */
    draw_rectangle(c, y1a - 1, x1a - 1, y2a + 1, x2a + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);

    /* Generate outer walls (b) */
    draw_rectangle(c, y1b - 1, x1b - 1, y2b + 1, x2b + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);

    /* Generate inner floors (a) */
    fill_rectangle(c, y1a, x1a, y2a, x2a, FEAT_FLOOR, SQUARE_NONE);

    /* Generate inner floors (b) */
    fill_rectangle(c, y1b, x1b, y2b, x2b, FEAT_FLOOR, SQUARE_NONE);

    return true;
}


/*
 * Builds a cross-shaped room.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 *
 * Returns success
 *
 * Room "a" runs north/south, and Room "b" runs east/west
 * So a "central pillar" would run from x1a,y1b to x2a,y2b.
 *
 * Note that currently, the "center" is always 3x3, but I think that the code
 * below will work for 5x5 (and perhaps even for unsymetric values like 4x3 or
 * 5x3 or 3x4 or 3x5).
 */
bool build_crossed(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int y, x;
    int height, width;
    int y1a, x1a, y2a, x2a;
    int y1b, x1b, y2b, x2b;
    int dy, dx, wy, wx;
    int light = false;

    /* Occasional light */
    if (c->wpos.depth <= randint1(25)) light = true;

    /* Pick inner dimension */
    wy = 1;
    wx = 1;

    /* Pick outer dimension */
    dy = rand_range(3, 4);
    dx = rand_range(3, 11);

    /* Determine extents of room (a) */
    y1a = dy;
    x1a = wx;
    y2a = dy;
    x2a = wx;

    /* Determine extents of room (b) */
    y1b = wy;
    x1b = dx;
    y2b = wy;
    x2b = dx;

    /* Calculate height and width */
    height = MAX(y1a + y2a + 1, y1b + y2b + 1);
    width = MAX(x1a + x2a + 1, x1b + x2b + 1);

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, height + 2, width + 2))
            return false;
    }

    /* locate room (a) */
    y1a = centre->y - dy;
    x1a = centre->x - wx;
    y2a = centre->y + dy;
    x2a = centre->x + wx;

    /* locate room (b) */
    y1b = centre->y - wy;
    x1b = centre->x - dx;
    y2b = centre->y + wy;
    x2b = centre->x + dx;
    
    /* Generate new room (a) */
    generate_room(c, y1a - 1, x1a - 1, y2a + 1, x2a + 1, light);

    /* Generate new room (b) */
    generate_room(c, y1b - 1, x1b - 1, y2b + 1, x2b + 1, light);

    /* Generate outer walls (a) */
    draw_rectangle(c, y1a - 1, x1a - 1, y2a + 1, x2a + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);

    /* Generate outer walls (b) */
    draw_rectangle(c, y1b - 1, x1b - 1, y2b + 1, x2b + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);

    /* Generate inner floors (a) */
    fill_rectangle(c, y1a, x1a, y2a, x2a, FEAT_FLOOR, SQUARE_NONE);

    /* Generate inner floors (b) */
    fill_rectangle(c, y1b, x1b, y2b, x2b, FEAT_FLOOR, SQUARE_NONE);

    /* Special features */
    switch (randint1(4))
    {
        /* Nothing */
        case 1: break;

        /* Large solid middle pillar */
        case 2:
        {
            /* Generate a small inner solid pillar */
            fill_rectangle(c, y1b, x1a, y2b, x2a, FEAT_GRANITE, SQUARE_WALL_INNER);

            break;
        }

        /* Inner treasure vault */
        case 3:
        {
            /* Generate a small inner vault */
            draw_rectangle(c, y1b, x1a, y2b, x2a, FEAT_GRANITE, SQUARE_WALL_INNER, false);

            /* Open the inner vault with a secret door */
            generate_hole(c, y1b, x1a, y2b, x2a, FEAT_SECRET);

            /* Place a treasure in the vault */
            place_object(p, c, centre, c->wpos.depth, false, false, ORIGIN_SPECIAL, 0);

            /* Let's guard the treasure well */
            vault_monsters(p, c, centre, c->wpos.depth + 2, randint0(2) + 3);

            /* Traps naturally */
            vault_traps(c, centre, 4, 4, randint0(3) + 2);

            break;
        }

        /* Something else */
        case 4:
        {
            /* Occasionally pinch the center shut */
            if (one_in_(3))
            {
                /* Pinch the east/west sides */
                for (y = y1b; y <= y2b; y++)
                {
                    struct loc grid;

                    if (y == centre->y) continue;
                    loc_init(&grid, x1a - 1, y);
                    set_marked_granite(c, &grid, SQUARE_WALL_INNER);
                    loc_init(&grid, x2a + 1, y);
                    set_marked_granite(c, &grid, SQUARE_WALL_INNER);
                }

                /* Pinch the north/south sides */
                for (x = x1a; x <= x2a; x++)
                {
                    struct loc grid;

                    if (x == centre->x) continue;
                    loc_init(&grid, x, y1b - 1);
                    set_marked_granite(c, &grid, SQUARE_WALL_INNER);
                    loc_init(&grid, x, y2b + 1);
                    set_marked_granite(c, &grid, SQUARE_WALL_INNER);
                }

                /* Open sides with doors */
                if (one_in_(3))
                    generate_open(c, y1b - 1, x1a - 1, y2b + 1, x2a + 1, FEAT_CLOSED);
            }

            /* Occasionally put a "plus" in the center */
            else if (one_in_(3))
                generate_plus(c, y1b, x1a, y2b, x2a, FEAT_GRANITE, SQUARE_WALL_INNER);

            /* Occasionally put a "pillar" in the center */
            else if (one_in_(3))
                set_marked_granite(c, centre, SQUARE_WALL_INNER);

            break;
        }
    }

    return true;
}


/*
 * Build a large room with an inner room.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 *
 * Returns success.
 *
 * Possible sub-types:
 *  1 - An inner room
 *  2 - An inner room with a small inner room
 *  3 - An inner room with a pillar or pillars
 *  4 - An inner room with a checkerboard
 *  5 - An inner room with four compartments
 */
bool build_large(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int y1, x1, y2, x2;
    int height = 9;
    int width = 23;
    int light = false;

    /* Occasional light */
    if (c->wpos.depth <= randint1(25)) light = true;

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, height + 2, width + 2))
            return false;
    }

    /* Large room */
    y1 = centre->y - height / 2;
    y2 = centre->y + height / 2;
    x1 = centre->x - width / 2;
    x2 = centre->x + width / 2;

    /* Generate new room */
    generate_room(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, light);

    /* Generate outer walls */
    draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);

    /* Generate inner floors */
    fill_rectangle(c, y1, x1, y2, x2, FEAT_FLOOR, SQUARE_NONE);

    /* The inner room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* Generate inner walls */
    draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE, SQUARE_WALL_INNER, false);

    /* Inner room variations */
    switch (randint1(5))
    {
        /* An inner room */
        case 1:
        {
            /* Open the inner room with a door and place a monster */
            generate_hole(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_CLOSED);
            vault_monsters(p, c, centre, c->wpos.depth + 2, 1);

            break;
        }

        /* An inner room with a small inner room */
        case 2:
        {
            struct loc begin, end;
            struct loc_iterator iter;

            /* Open the inner room with a door */
            generate_hole(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_CLOSED);

            /* Place another inner room */
            draw_rectangle(c, centre->y - 1, centre->x - 1, centre->y + 1, centre->x + 1,
                FEAT_GRANITE, SQUARE_WALL_INNER, false);

            loc_init(&begin, centre->x - 1, centre->y - 1);
            loc_init(&end, centre->x + 1, centre->y + 1);
            loc_iterator_first(&iter, &begin, &end);

            /* Open the inner room with a locked door */
            generate_hole(c, centre->y - 1, centre->x - 1, centre->y + 1, centre->x + 1, FEAT_CLOSED);
            do
            {
                if (square_iscloseddoor(c, &iter.cur))
                    square_set_door_lock(c, &iter.cur, randint1(7));
            }
            while (loc_iterator_next(&iter));

            /* Monsters to guard the treasure */
            vault_monsters(p, c, centre, c->wpos.depth + 2, randint1(3) + 2);

            /* Object (80%) or Stairs (20%) */
            if (magik(80))
                place_object(p, c, centre, c->wpos.depth, false, false, ORIGIN_SPECIAL, 0);
            else
                place_random_stairs(c, centre);

            /* Traps to protect the treasure */
            vault_traps(c, centre, 4, 10, 2 + randint1(3));

            break;
        }

        /* An inner room with an inner pillar or pillars */
        case 3:
        {
            /* Open the inner room with a door */
            generate_hole(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_CLOSED);

            /* Inner pillar */
            fill_rectangle(c, centre->y - 1, centre->x - 1, centre->y + 1, centre->x + 1,
                FEAT_GRANITE, SQUARE_WALL_INNER);

            /* Occasionally, two more Large Inner Pillars */
            if (one_in_(2))
            {
                if (one_in_(2))
                {
                    fill_rectangle(c, centre->y - 1, centre->x - 7, centre->y + 1, centre->x - 5,
                        FEAT_GRANITE, SQUARE_WALL_INNER);
                    fill_rectangle(c, centre->y - 1, centre->x + 5, centre->y + 1, centre->x + 7,
                        FEAT_GRANITE, SQUARE_WALL_INNER);
                }
                else
                {
                    fill_rectangle(c, centre->y - 1, centre->x - 6, centre->y + 1, centre->x - 4,
                        FEAT_GRANITE, SQUARE_WALL_INNER);
                    fill_rectangle(c, centre->y - 1, centre->x + 4, centre->y + 1, centre->x + 6,
                        FEAT_GRANITE, SQUARE_WALL_INNER);
                }
            }

            /* Occasionally, some Inner rooms */
            if (one_in_(3))
            {
                struct loc grid;

                /* Inner rectangle */
                draw_rectangle(c, centre->y - 1, centre->x - 5, centre->y + 1, centre->x + 5,
                    FEAT_GRANITE, SQUARE_WALL_INNER, false);

                /* Secret doors (random top/bottom) */
                loc_init(&grid, centre->x - 3, centre->y - 3 + (randint1(2) * 2));
                place_secret_door(c, &grid);
                loc_init(&grid, centre->x + 3, centre->y - 3 + (randint1(2) * 2));
                place_secret_door(c, &grid);

                /* Monsters */
                loc_init(&grid, centre->x - 2, centre->y);
                vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(2));
                loc_init(&grid, centre->x + 2, centre->y);
                vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(2));

                /* Objects */
                loc_init(&grid, centre->x - 2, centre->y);
                if (one_in_(3))
                    place_object(p, c, &grid, c->wpos.depth, false, false, ORIGIN_SPECIAL, 0);
                loc_init(&grid, centre->x + 2, centre->y);
                if (one_in_(3))
                    place_object(p, c, &grid, c->wpos.depth, false, false, ORIGIN_SPECIAL, 0);
            }

            break;
        }

        /* An inner room with a checkerboard */
        case 4:
        {
            struct loc grid;
            struct loc begin, end;
            struct loc_iterator iter;

            /* Open the inner room with a door */
            generate_hole(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_CLOSED);

            loc_init(&begin, x1, y1);
            loc_init(&end, x2, y2);
            loc_iterator_first(&iter, &begin, &end);

            /* Checkerboard */
            do
            {
                if ((iter.cur.x + iter.cur.y) & 0x01)
                    set_marked_granite(c, &iter.cur, SQUARE_WALL_INNER);
            }
            while (loc_iterator_next(&iter));

            /* Monsters just love mazes. */
            loc_init(&grid, centre->x - 5, centre->y);
            vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(3));
            loc_init(&grid, centre->x + 5, centre->y);
            vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(3));

            /* Traps make them entertaining. */
            loc_init(&grid, centre->x - 3, centre->y);
            vault_traps(c, &grid, 2, 8, randint1(3));
            loc_init(&grid, centre->x + 3, centre->y);
            vault_traps(c, &grid, 2, 8, randint1(3));

            /* Mazes should have some treasure too. */
            vault_objects(p, c, centre, 3);

            break;
        }

        /* Four small rooms. */
        case 5:
        {
            struct loc grid;

            /* Inner "cross" */
            generate_plus(c, y1, x1, y2, x2, FEAT_GRANITE, SQUARE_WALL_INNER);

            /* Doors into the rooms */
            if (magik(50))
            {
                int i = randint1(10);

                loc_init(&grid, centre->x - i, y1 - 1);
                place_closed_door(c, &grid);
                loc_init(&grid, centre->x + i, y1 - 1);
                place_closed_door(c, &grid);
                loc_init(&grid, centre->x - i, y2 + 1);
                place_closed_door(c, &grid);
                loc_init(&grid, centre->x + i, y2 + 1);
                place_closed_door(c, &grid);
            }
            else
            {
                int i = randint1(3);

                loc_init(&grid, x1 - 1, centre->y + i);
                place_closed_door(c, &grid);
                loc_init(&grid, x1 - 1, centre->y - i);
                place_closed_door(c, &grid);
                loc_init(&grid, x2 + 1, centre->y + i);
                place_closed_door(c, &grid);
                loc_init(&grid, x2 + 1, centre->y - i);
                place_closed_door(c, &grid);
            }

            /* Treasure, centered at the center of the cross */
            vault_objects(p, c, centre, 2 + randint1(2));

            /* Gotta have some monsters */
            loc_init(&grid, centre->x - 4, centre->y + 1);
            vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(4));
            loc_init(&grid, centre->x + 4, centre->y + 1);
            vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(4));
            loc_init(&grid, centre->x - 4, centre->y - 1);
            vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(4));
            loc_init(&grid, centre->x + 4, centre->y - 1);
            vault_monsters(p, c, &grid, c->wpos.depth + 2, randint1(4));

            break;
        }
    }

    return true;
}


/*
 * Build a monster nest
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 *
 * Returns success.
 *
 * A monster nest consists of a rectangular moat around a room containing
 * monsters of a given type.
 *
 * The monsters are chosen from a set of 64 randomly selected monster races,
 * to allow the nest creation to fail instead of having "holes".
 *
 * Note the use of the "get_mon_num_prep()" function to prepare the
 * "monster allocation table" in such a way as to optimize the selection
 * of "appropriate" non-unique monsters for the nest.
 *
 * The available monster nests are specified in gamedata/pit.txt.
 *
 * Note that get_mon_num() function can fail, in which case the nest will be
 * empty, and will not affect the level rating.
 *
 * Monster nests will never contain unique monsters.
 */
bool build_nest(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int y1, x1, y2, x2;
    int i;
    int alloc_obj;
    struct monster_race *what[64];
    bool empty = false;
    int light = false;
    int size_vary = randint0(4);
    int height = 9;
    int width = 11 + 2 * size_vary;
    struct monster_group_info info = {0, 0};
    struct loc begin, end;
    struct loc_iterator iter;

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, height + 2, width + 2))
            return false;
    }

    /* Large room */
    y1 = centre->y - height / 2;
    y2 = centre->y + height / 2;
    x1 = centre->x - width / 2;
    x2 = centre->x + width / 2;
    
    /* Generate new room */
    generate_room(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, light);

    /* Generate outer walls */
    draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);

    /* Generate inner floors */
    fill_rectangle(c, y1, x1, y2, x2, FEAT_FLOOR, SQUARE_NONE);

    /* Advance to the center room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* PWMAngband -- generate pit floors */
    fill_rectangle(c, y1, x1, y2, x2, FEAT_FLOOR_PIT, SQUARE_NONE);

    /* Generate inner walls; add one door as entrance */
    /* PWMAngband -- make them permanent to prevent monsters from escaping */
    draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE, SQUARE_FAKE, false);
    generate_hole(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_CLOSED);

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    /* PWMAngband -- make it "icky" and "NO_TELEPORT" to prevent teleportation */
    do
    {
        sqinfo_on(square(c, &iter.cur)->info, SQUARE_VAULT);
        sqinfo_on(square(c, &iter.cur)->info, SQUARE_NO_TELEPORT);
    }
    while (loc_iterator_next(&iter));

    /* Decide on the pit type */
    set_pit_type(c->wpos.depth, 2);

    /* Chance of objects on the floor */
    alloc_obj = dun->pit_type->obj_rarity;

    /* Prepare allocation table */
    get_mon_num_prep(mon_pit_hook);

    /* Pick some monster types */
    for (i = 0; i < 64; i++)
    {
        /* Get a (hard) monster type */
        what[i] = get_mon_num(c, c->wpos.depth + 10, false);

        /* Notice failure */
        if (!what[i]) empty = true;
    }

    /* Prepare allocation table */
    get_mon_num_prep(NULL);

    /* Oops */
    if (empty) return false;

    /* Increase the level rating */
    add_to_monster_rating(c, size_vary + dun->pit_type->ave / 20);

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    /* Place some monsters */
    do
    {
        /* Figure out what monster is being used, and place that monster */
        struct monster_race *race = what[randint0(64)];

        place_new_monster(p, c, &iter.cur, race, 0, &info, ORIGIN_DROP_PIT);

        /* Occasionally place an item, making it good 1/3 of the time */
        if (magik(alloc_obj))
            place_object(p, c, &iter.cur, c->wpos.depth + 10, one_in_(3), false, ORIGIN_PIT, 0);
    }
    while (loc_iterator_next(&iter));

    return true;
}


/*
 * Build a monster pit
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 *
 * Monster pits are laid-out similarly to monster nests.
 *
 * The available monster pits are specified in gamedata/pit.txt.
 *
 * The inside room in a monster pit appears as shown below, where the
 * actual monsters in each location depend on the type of the pit
 *
 *   #############
 *   #11000000011#
 *   #01234543210#
 *   #01236763210#
 *   #01234543210#
 *   #11000000011#
 *   #############
 *
 * Note that the monsters in the pit are chosen by using get_mon_num() to
 * request 16 "appropriate" monsters, sorting them by level, and using the
 * "even" entries in this sorted list for the contents of the pit.
 *
 * Note the use of get_mon_num_prep() to prepare the monster allocation
 * table in such a way as to optimize the selection of appropriate non-unique
 * monsters for the pit.
 *
 * The get_mon_num() function can fail, in which case the pit will be empty,
 * and will not effect the level rating.
 *
 * Like monster nests, monster pits will never contain unique monsters.
 */
bool build_pit(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    struct monster_race *what[16];
    int i, j, y, x, y1, x1, y2, x2;
    bool empty = false;
    int light = false;
    int alloc_obj;
    int height = 9;
    int width = 15;
    int group_index = 0;
    struct monster_group_info info = {0, 0};
    struct loc begin, end, grid;
    struct loc_iterator iter;

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, height + 2, width + 2))
            return false;
    }

    /* Large room */
    y1 = centre->y - height / 2;
    y2 = centre->y + height / 2;
    x1 = centre->x - width / 2;
    x2 = centre->x + width / 2;

    /* Generate new room, outer walls and inner floor */
    generate_room(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, light);
    draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE, SQUARE_WALL_OUTER, false);
    fill_rectangle(c, y1, x1, y2, x2, FEAT_FLOOR, SQUARE_NONE);

    /* Advance to the center room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* PWMAngband -- generate pit floors */
    fill_rectangle(c, y1, x1, y2, x2, FEAT_FLOOR_PIT, SQUARE_NONE);

    /* Generate inner walls; add one door as entrance */
    /* PWMAngband -- make them permanent to prevent monsters from escaping */
    draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE, SQUARE_FAKE, false);
    generate_hole(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_CLOSED);

    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);
    loc_iterator_first(&iter, &begin, &end);

    /* PWMAngband -- make it "icky" and "NO_TELEPORT" to prevent teleportation */
    do
    {
        sqinfo_on(square(c, &iter.cur)->info, SQUARE_VAULT);
        sqinfo_on(square(c, &iter.cur)->info, SQUARE_NO_TELEPORT);
    }
    while (loc_iterator_next(&iter));

    /* Decide on the pit type */
    set_pit_type(c->wpos.depth, 1);

    /* Chance of objects on the floor */
    alloc_obj = dun->pit_type->obj_rarity;

    /* Prepare allocation table */
    get_mon_num_prep(mon_pit_hook);

    /* Pick some monster types */
    for (i = 0; i < 16; i++)
    {
        /* Get a (hard) monster type */
        what[i] = get_mon_num(c, c->wpos.depth + 10, false);

        /* Notice failure */
        if (!what[i]) empty = true;
    }

    /* Prepare allocation table */
    get_mon_num_prep(NULL);

    /* Oops */
    if (empty) return false;

    /* Sort the entries */
    for (i = 0; i < 16 - 1; i++)
    {
        for (j = 0; j < 16 - 1; j++)
        {
            int i1 = j;
            int i2 = j + 1;
            int p1 = what[i1]->level;
            int p2 = what[i2]->level;

            /* Bubble */
            if (p1 > p2)
            {
                struct monster_race *tmp = what[i1];

                what[i1] = what[i2];
                what[i2] = tmp;
            }
        }
    }

    /* Select every other entry */
    for (i = 0; i < 8; i++) what[i] = what[i * 2];

    /* Increase the level rating */
    add_to_monster_rating(c, 3 + dun->pit_type->ave / 20);

    /* Get a group ID */
    group_index = monster_group_index_new(c);

    /* Center monster */
    info.index = group_index;
    info.role = MON_GROUP_LEADER;
    place_new_monster(p, c, centre, what[7], 0, &info, ORIGIN_DROP_PIT);

    /* Remaining monsters are servants */
    info.role = MON_GROUP_SERVANT;

    /* Top and bottom rows (middle) */
    for (x = centre->x - 3; x <= centre->x + 3; x++)
    {
        loc_init(&grid, x, centre->y - 2);
        place_new_monster(p, c, &grid, what[0], 0, &info, ORIGIN_DROP_PIT);
        loc_init(&grid, x, centre->y + 2);
        place_new_monster(p, c, &grid, what[0], 0, &info, ORIGIN_DROP_PIT);
    }

    /* Corners */
    for (x = centre->x - 5; x <= centre->x - 4; x++)
    {
        loc_init(&grid, x, centre->y - 2);
        place_new_monster(p, c, &grid, what[1], 0, &info, ORIGIN_DROP_PIT);
        loc_init(&grid, x, centre->y + 2);
        place_new_monster(p, c, &grid, what[1], 0, &info, ORIGIN_DROP_PIT);
    }

    for (x = centre->x + 4; x <= centre->x + 5; x++)
    {
        loc_init(&grid, x, centre->y - 2);
        place_new_monster(p, c, &grid, what[1], 0, &info, ORIGIN_DROP_PIT);
        loc_init(&grid, x, centre->y + 2);
        place_new_monster(p, c, &grid, what[1], 0, &info, ORIGIN_DROP_PIT);
    }

    /* Middle columns */
    for (y = centre->y - 1; y <= centre->y + 1; y++)
    {
        loc_init(&grid, centre->x - 5, y);
        place_new_monster(p, c, &grid, what[0], 0, &info, ORIGIN_DROP_PIT);
        loc_init(&grid, centre->x + 5, y);
        place_new_monster(p, c, &grid, what[0], 0, &info, ORIGIN_DROP_PIT);

        loc_init(&grid, centre->x - 4, y);
        place_new_monster(p, c, &grid, what[1], 0, &info, ORIGIN_DROP_PIT);
        loc_init(&grid, centre->x + 4, y);
        place_new_monster(p, c, &grid, what[1], 0, &info, ORIGIN_DROP_PIT);

        loc_init(&grid, centre->x - 3, y);
        place_new_monster(p, c, &grid, what[2], 0, &info, ORIGIN_DROP_PIT);
        loc_init(&grid, centre->x + 3, y);
        place_new_monster(p, c, &grid, what[2], 0, &info, ORIGIN_DROP_PIT);

        loc_init(&grid, centre->x - 2, y);
        place_new_monster(p, c, &grid, what[3], 0, &info, ORIGIN_DROP_PIT);
        loc_init(&grid, centre->x + 2, y);
        place_new_monster(p, c, &grid, what[3], 0, &info, ORIGIN_DROP_PIT);
    }

    /* Corners around the middle monster */
    loc_init(&grid, centre->x - 1, centre->y - 1);
    place_new_monster(p, c, &grid, what[4], 0, &info, ORIGIN_DROP_PIT);
    loc_init(&grid, centre->x + 1, centre->y - 1);
    place_new_monster(p, c, &grid, what[4], 0, &info, ORIGIN_DROP_PIT);
    loc_init(&grid, centre->x - 1, centre->y + 1);
    place_new_monster(p, c, &grid, what[4], 0, &info, ORIGIN_DROP_PIT);
    loc_init(&grid, centre->x + 1, centre->y + 1);
    place_new_monster(p, c, &grid, what[4], 0, &info, ORIGIN_DROP_PIT);

    /* Above/Below the center monster */
    loc_init(&grid, centre->x, centre->y + 1);
    place_new_monster(p, c, &grid, what[5], 0, &info, ORIGIN_DROP_PIT);
    loc_init(&grid, centre->x, centre->y - 1);
    place_new_monster(p, c, &grid, what[5], 0, &info, ORIGIN_DROP_PIT);

    /* Next to the center monster */
    loc_init(&grid, centre->x + 1, centre->y);
    place_new_monster(p, c, &grid, what[6], 0, &info, ORIGIN_DROP_PIT);
    loc_init(&grid, centre->x - 1, centre->y);
    place_new_monster(p, c, &grid, what[6], 0, &info, ORIGIN_DROP_PIT);

    /* Place some objects */
    for (grid.y = centre->y - 2; grid.y <= centre->y + 2; grid.y++)
    {
        for (grid.x = centre->x - 9; grid.x <= centre->x + 9; grid.x++)
        {
            /* Occasionally place an item, making it good 1/3 of the time */
            if (magik(alloc_obj))
                place_object(p, c, &grid, c->wpos.depth + 10, one_in_(3), false, ORIGIN_PIT, 0);
        }
    }

    return true;
}


/*
 * Build a template room
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating template room rating to select
 *
 * Returns success.
 */
bool build_template(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    /* All room templates currently have type 1 */
    return build_room_template_type(p, c, centre, 1, rating);
}


/*
 * Build an interesting room.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_interesting(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    return build_vault_type(p, c, centre, "Interesting room");
}


/*
 * Build a lesser vault.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_lesser_vault(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    return build_vault_type(p, c, centre, "Lesser vault");
}


/*
 * Build a lesser new-style vault.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_lesser_new_vault(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    return build_vault_type(p, c, centre, "Lesser vault (new)");
}


/*
 * Build a medium vault.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_medium_vault(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    return build_vault_type(p, c, centre, "Medium vault");
}


/*
 * Build a medium new-style vault.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_medium_new_vault(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    return build_vault_type(p, c, centre, "Medium vault (new)");
}


/*
 * Help greater_vault() or greater_new_vault().
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * name is the name of the type to build
 *
 * Returns success.
 *
 * Classic profile:
 * Since Greater Vaults are so large (4x6 blocks, in a 6x18 dungeon) there is
 * a 63% chance that a randomly chosen quadrant to start a GV on won't work.
 * To balance this, we give Greater Vaults an artificially high probability
 * of being attempted, and then in this function use a depth check to cancel
 * vault creation except at deep depths.
 *
 * Newer profiles:
 * We reject 2/3 of attempts which pass other checks to get roughly the same
 * chance of a GV as the classic profile.
 *
 * The following code should make a greater vault with frequencies:
 *
 *  dlvl  freq
 *  100+  18.0%
 *  90-99 16.0 - 18.0%
 *  80-89 10.0 - 11.0%
 *  70-79  5.7 -  6.5%
 *  60-69  3.3 -  3.8%
 *  50-59  1.8 -  2.1%
 *  0-49   0.0 -  1.0%
 *
 */
static bool help_greater_vault(struct player *p, struct chunk *c, struct loc *centre,
    const char *name)
{
    int i;
    int numerator = 1;
    int denominator = 3;

    /*
     * Only try to build a GV as the first room. If not finding space,
     * cent_n has already been incremented.
     */
    if (dun->cent_n > ((centre->y >= c->height || centre->x >= c->width)? 0: 1))
        return false;

    /* Level 90+ has a 1/3 chance, level 80-89 has 2/9, ... */
    for (i = 90; i > c->wpos.depth; i -= 10)
    {
        numerator *= 2;
        denominator *= 3;
    }

    /* Attempt to pass the depth check and build a GV */
    if (randint0(denominator) >= numerator) return false;

    /* Non-classic profiles need to adjust the probability */
    if (!streq(dun->profile->name, "classic") && !one_in_(3)) return false;

    return build_vault_type(p, c, centre, name);
}


/*
 * Build a greater vault.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_greater_vault(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    return help_greater_vault(p, c, centre, "Greater vault");
}


/*
 * Build a greater new-style vault.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_greater_new_vault(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    return help_greater_vault(p, c, centre, "Greater vault (new)");
}


/*
 * Moria room (from Oangband). Uses the "starburst room" code.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 */
bool build_moria(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int y1, x1, y2, x2;
    int i;
    int height, width;
    bool light = (c->wpos.depth <= randint1(35));

    /* Pick a room size */
    height = 8 + randint0(5);
    width = 10 + randint0(5);

    /* Try twice to find space for a room. */
    for (i = 0; i < 2; i++)
    {
        /* Really large room - only on first try. */
        if ((i == 0) && one_in_(15))
        {
            height *= 1 + randint1(2);
            width *= 2 + randint1(3);
        }

        /* Long, narrow room. Sometimes tall and thin. */
        else if (!one_in_(4))
        {
            if (one_in_(15))
                height *= 2 + randint0(2);
            else
                width *= 2 + randint0(3);
        }

        /* Find and reserve some space in the dungeon. Get center of room. */
        if ((centre->y >= c->height) || (centre->x >= c->width))
        {
            if (!find_space(centre, height, width))
            {
                if (i == 0) continue; /* Failed first attempt */
                if (i == 1) return false; /* Failed second attempt */
            }
            else break; /* Success */
        }
        else break; /* Not finding space */
    }

    /* Locate the room */
    y1 = centre->y - height / 2;
    x1 = centre->x - width / 2;
    y2 = y1 + height - 1;
    x2 = x1 + width - 1;

    /* Generate starburst room. Return immediately if out of bounds. */
    if (!generate_starburst_room(c, y1, x1, y2, x2, light, FEAT_FLOOR, true))
        return false;

    /* Sometimes, the room may have rubble in it. */
    if (one_in_(10))
    {
        generate_starburst_room(c, y1 + randint0(height / 4), x1 + randint0(width / 4),
            y2 - randint0(height / 4), x2 - randint0(width / 4), false, FEAT_PASS_RUBBLE, false);
    }

    /* Success */
    return true;
}


/*
 * Rooms of chambers
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 *
 * Build a room, varying in size between 22x22 and 44x66, consisting of
 * many smaller, irregularly placed, chambers all connected by doors or
 * short tunnels.
 *
 * Plop down an area-dependent number of magma-filled chambers, and remove
 * blind doors and tiny rooms.
 *
 * Hollow out a chamber near the center, connect it to new chambers, and
 * hollow them out in turn. Continue in this fashion until there are no
 * remaining chambers within two squares of any cleared chamber.
 *
 * Clean up doors. Neaten up the wall types. Turn floor grids into rooms,
 * illuminate if requested.
 *
 * Fill the room with up to 35 (sometimes up to 50) monsters of a creature
 * race or type that offers a challenge at the character's depth. This is
 * similar to monster pits, except that we choose among a wider range of
 * monsters.
 */
bool build_room_of_chambers(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    int i, d;
    int area, num_chambers;
    int y1, x1, y2, x2;
    int height, width, count;
    char name[40];

    /* Deeper in the dungeon, chambers are less likely to be lit. */
    bool light = ((randint0(45) > c->wpos.depth)? true: false);

    struct loc begin, end, grid;
    struct loc_iterator iter;

    /* Calculate a level-dependent room size. */
    height = 20 + m_bonus(20, c->wpos.depth);
    width = 20 + randint1(20) + m_bonus(20, c->wpos.depth);

    /* Find and reserve some space in the dungeon. Get center of room. */
    if ((centre->y >= c->height) || (centre->x >= c->width))
    {
        if (!find_space(centre, height, width)) return false;
    }

    /* Calculate the borders of the room. */
    y1 = centre->y - (height / 2);
    x1 = centre->x - (width / 2);
    y2 = centre->y + (height - 1) / 2;
    x2 = centre->x + (width - 1) / 2;
    loc_init(&begin, x1, y1);
    loc_init(&end, x2, y2);

    /* Make certain the room does not cross the dungeon edge. */
    if (!square_in_bounds(c, &begin) || !square_in_bounds(c, &end)) return false;

    /* Determine how much space we have. */
    area = ABS(y2 - y1) * ABS(x2 - x1);

    /* Calculate the number of smaller chambers to make. */
    num_chambers = 10 + area / 80;

    /* Build the chambers. */
    for (i = 0; i < num_chambers; i++)
    {
        int c_y1, c_x1, c_y2, c_x2;
        int size, width_local, height_local;

        /* Determine size of chamber. */
        size = 3 + randint0(4);
        width_local = size + randint0(10);
        height_local = size + randint0(4);

        /* Pick an upper-left corner at random. */
        c_y1 = y1 + randint0(1 + y2 - y1 - height_local);
        c_x1 = x1 + randint0(1 + x2 - x1 - width_local);

        /* Determine lower-right corner of chamber. */
        c_y2 = c_y1 + height_local;
        if (c_y2 > y2) c_y2 = y2;

        c_x2 = c_x1 + width_local;
        if (c_x2 > x2) c_x2 = x2;

        /* Make me a (magma filled) chamber. */
        make_chamber(c, c_y1, c_x1, c_y2, c_x2);
    }

    loc_iterator_first(&iter, &begin, &end);

    /* Remove useless doors, fill in tiny, narrow rooms. */
    do
    {
        count = 0;

        /* Stay legal. */
        if (!square_in_bounds_fully(c, &iter.cur)) continue;

        /* Check all adjacent grids. */
        for (d = 0; d < 8; d++)
        {
            struct loc adjacent;

            /* Extract adjacent location */
            loc_sum(&adjacent, &iter.cur, &ddgrid_ddd[d]);

            /* Count the walls and dungeon granite. */
            if ((square(c, &adjacent)->feat == FEAT_GRANITE) &&
                !square_iswall_outer(c, &adjacent) && !square_iswall_solid(c, &adjacent))
            {
                count++;
            }
        }

        /* Five adjacent walls: Change non-chamber to wall. */
        if ((count == 5) && (square(c, &iter.cur)->feat != FEAT_MAGMA))
            set_marked_granite(c, &iter.cur, SQUARE_WALL_INNER);

        /* More than five adjacent walls: Change anything to wall. */
        else if (count > 5)
            set_marked_granite(c, &iter.cur, SQUARE_WALL_INNER);
    }
    while (loc_iterator_next(&iter));

    /* Pick a random magma spot near the center of the room. */
    for (i = 0; i < 50; i++)
    {
        grid.y = y1 + ABS(y2 - y1) / 4 + randint0(ABS(y2 - y1) / 2);
        grid.x = x1 + ABS(x2 - x1) / 4 + randint0(ABS(x2 - x1) / 2);
        if (square(c, &grid)->feat == FEAT_MAGMA) break;
    }

    /* Hollow out the first room. */
    square_set_feat(c, &grid, FEAT_FLOOR);
    hollow_out_room(c, &grid);

    /* Attempt to change every in-room magma grid to open floor. */
    for (i = 0; i < 100; i++)
    {
        /* Assume this run will do no useful work. */
        bool joy = false;

        loc_iterator_first(&iter, &begin, &end);

        /* Make new doors and tunnels between magma and open floor. */
        do
        {
            /* Stay legal. */
            if (!square_in_bounds_fully(c, &iter.cur)) continue;

            /* Current grid must be magma. */
            if (square(c, &iter.cur)->feat != FEAT_MAGMA) continue;

            /* Check only horizontal and vertical directions. */
            for (d = 0; d < 4; d++)
            {
                struct loc adjacent1, adjacent2, adjacent3;

                /* Extract adjacent location */
                loc_sum(&adjacent1, &iter.cur, &ddgrid_ddd[d]);

                /* Need inner wall. */
                if (!square_iswall_inner(c, &adjacent1)) continue;

                /* Keep going in the same direction, if in bounds. */
                loc_sum(&adjacent2, &adjacent1, &ddgrid_ddd[d]);
                if (!square_in_bounds(c, &adjacent2)) continue;

                /* If we find open floor, place a door. */
                if (square(c, &adjacent2)->feat == FEAT_FLOOR)
                {
                    joy = true;

                    /* Make a broken door in the wall grid. */
                    square_set_feat(c, &adjacent1, FEAT_BROKEN);

                    /* Hollow out the new room. */
                    square_set_feat(c, &iter.cur, FEAT_FLOOR);
                    hollow_out_room(c, &iter.cur);

                    break;
                }

                /* If we find more inner wall... */
                if (square_iswall_inner(c, &adjacent2))
                {
                    /* ...Keep going in the same direction. */
                    loc_sum(&adjacent3, &adjacent2, &ddgrid_ddd[d]);
                    if (!square_in_bounds(c, &adjacent3)) continue;

                    /* If we /now/ find floor, make a tunnel. */
                    if (square(c, &adjacent3)->feat == FEAT_FLOOR)
                    {
                        joy = true;

                        /* Turn both wall grids into floor. */
                        square_set_feat(c, &adjacent1, FEAT_FLOOR);
                        square_set_feat(c, &adjacent2, FEAT_FLOOR);

                        /* Hollow out the new room. */
                        square_set_feat(c, &iter.cur, FEAT_FLOOR);
                        hollow_out_room(c, &iter.cur);

                        break;
                    }
                }
            }
        }
        while (loc_iterator_next_strict(&iter));

        /* If we could find no work to do, stop. */
        if (!joy) break;
    }

    loc_iterator_first(&iter, &begin, &end);

    /* Turn broken doors into a random kind of door, remove open doors. */
    do
    {
        if (square(c, &iter.cur)->feat == FEAT_OPEN)
            set_marked_granite(c, &iter.cur, SQUARE_WALL_INNER);
        else if (square(c, &iter.cur)->feat == FEAT_BROKEN)
            place_random_door(c, &iter.cur);
    }
    while (loc_iterator_next(&iter));

    loc_init(&begin, (x1 - 1 > 0 ? x1 - 1 : 0), (y1 - 1 > 0 ? y1 - 1 : 0));
    loc_init(&end, (x2 + 2 < c->width ? x2 + 2 : c->width), (y2 + 2 < c->height ? y2 + 2 : c->height));
    loc_iterator_first(&iter, &begin, &end);

    /* Turn all walls and magma not adjacent to floor into dungeon granite. */
    /* Turn all floors and adjacent grids into rooms, sometimes lighting them */
    do
    {
        if (square_iswall_inner(c, &iter.cur) || (square(c, &iter.cur)->feat == FEAT_MAGMA))
        {
            for (d = 0; d < 9; d++)
            {
                struct loc adjacent;

                /* Extract adjacent location */
                loc_sum(&adjacent, &iter.cur, &ddgrid_ddd[d]);

                /* Stay legal */
                if (!square_in_bounds(c, &adjacent)) continue;

                /* No floors allowed */
                if (square(c, &adjacent)->feat == FEAT_FLOOR) break;

                /* Turn me into dungeon granite. */
                if (d == 8) set_marked_granite(c, &iter.cur, SQUARE_NONE);
            }
        }

        if (square_isfloor(c, &iter.cur))
        {
            for (d = 0; d < 9; d++)
            {
                struct loc adjacent;

                /* Extract adjacent location */
                loc_sum(&adjacent, &iter.cur, &ddgrid_ddd[d]);

                /* Stay legal */
                if (!square_in_bounds(c, &adjacent)) continue;

                /* Turn into room, forbid stairs. */
                sqinfo_on(square(c, &adjacent)->info, SQUARE_ROOM);
                sqinfo_on(square(c, &adjacent)->info, SQUARE_NO_STAIRS);

                /* Illuminate if requested. */
                if (light) sqinfo_on(square(c, &adjacent)->info, SQUARE_GLOW);
            }
        }
    }
    while (loc_iterator_next_strict(&iter));

    loc_init(&begin, (x1 - 1 > 0 ? x1 - 1 : 0), (y1 - 1 > 0 ? y1 - 1 : 0));
    loc_init(&end, (x2 + 2 < c->width ? x2 + 2 : c->width), (y2 + 2 < c->height ? y2 + 2 : c->height));
    loc_iterator_first(&iter, &begin, &end);

    /* Turn all inner wall grids adjacent to dungeon granite into outer walls */
    do
    {
        /* Stay legal. */
        if (!square_in_bounds_fully(c, &iter.cur)) continue;

        if (square_iswall_inner(c, &iter.cur))
        {
            for (d = 0; d < 9; d++)
            {
                struct loc adjacent;

                /* Extract adjacent location */
                loc_sum(&adjacent, &iter.cur, &ddgrid_ddd[d]);

                /* Look for dungeon granite */
                if ((square(c, &adjacent)->feat == FEAT_GRANITE) &&
                    !square_iswall_inner(c, &adjacent) &&
                    !square_iswall_outer(c, &adjacent) &&
                    !square_iswall_solid(c, &adjacent))
                {
                    /* Turn me into outer wall. */
                    set_marked_granite(c, &iter.cur, SQUARE_WALL_OUTER);

                    /* Done */
                    break;
                }
            }
        }
    }
    while (loc_iterator_next_strict(&iter));

    /*** Now we get to place the monsters. ***/
    get_chamber_monsters(p, c, y1, x1, y2, x2, name, height * width);

    /* Increase the level rating */
    add_to_monster_rating(c, 10);

    /* Success. */
    return true;
}


/*
 * A single starburst-shaped room of extreme size, usually dotted or
 * even divided with irregularly-shaped fields of rubble. No special
 * monsters. Appears deeper than level 40.
 *
 * p the player
 * c the chunk the room is being built in
 * centre the room centre; out of chunk centre invokes find_space()
 * rating is not used for this room type
 *
 * Returns success.
 *
 * These are the largest, most difficult to position, and thus highest-
 * priority rooms in the dungeon. They should be rare, so as not to
 * interfere with greater vaults.
 */
bool build_huge(struct player *p, struct chunk *c, struct loc *centre, int rating)
{
    bool finding_space = (centre->y >= c->height || centre->x >= c->width);
    bool light;
    int i, count;
    int y1, x1, y2, x2;
    int y1_tmp, x1_tmp, y2_tmp, x2_tmp;
    int width_tmp, height_tmp;

    int height = 30 + randint0(10);
    int width = 45 + randint0(50);

    /*
     * Only try to build a huge room as the first room. If not finding
     * space, cent_n has already been incremented.
     */
    if (dun->cent_n > (finding_space? 0: 1)) return false;

    /* Flat 5% chance */
    if (!one_in_(20)) return false;

    /* This room is usually lit. */
    light = !one_in_(3);

    /* Find and reserve some space. Get center of room. */
    if (finding_space)
    {
        if (!find_space(centre, height, width)) return false;
    }

    /* Locate the room */
    y1 = centre->y - height / 2;
    x1 = centre->x - width / 2;
    y2 = y1 + height - 1;
    x2 = x1 + width - 1;

    /* Make a huge starburst room with optional light. */
    if (!generate_starburst_room(c, y1, x1, y2, x2, light, FEAT_FLOOR, false))
        return false;

    /* Often, add rubble to break things up a bit. */
    if (randint1(5) > 2)
    {
        /* Determine how many rubble fields to add (between 1 and 6). */
        count = height * width * randint1(2) / 1100;

        /* Make the rubble fields. */
        for (i = 0; i < count; i++)
        {
            height_tmp = 8 + randint0(16);
            width_tmp = 10 + randint0(24);

            /* Semi-random location. */
            y1_tmp = y1 + randint0(height - height_tmp);
            x1_tmp = x1 + randint0(width - width_tmp);
            y2_tmp = y1_tmp + height_tmp;
            x2_tmp = x1_tmp + width_tmp;

            /* Make the rubble field. */
            generate_starburst_room(c, y1_tmp, x1_tmp, y2_tmp, x2_tmp, false, FEAT_PASS_RUBBLE,
                false);
        }
    }

    /* Success. */
    return true;
}


/*
 * Attempt to build a room of the given type at the given block
 *
 * p the player
 * c the chunk the room is being built in
 * by0, bx0 block co-ordinates of the top left block
 * profile the profile of the rooom we're trying to build
 * finds_own_space whether we are allowing the room to place itself
 *
 * Returns success.
 *
 * Note that this code assumes that profile height and width are the maximum
 * possible grid sizes, and then allocates a number of blocks that will always
 * contain them.
 *
 * Note that we restrict the number of pits/nests to reduce
 * the chance of overflowing the monster list during level creation.
 */
bool room_build(struct player *p, struct chunk *c, int by0, int bx0, struct room_profile profile,
    bool finds_own_space)
{
    /* Extract blocks */
    int by1 = by0;
    int bx1 = bx0;
    int by2 = by0 + profile.height / dun->block_hgt;
    int bx2 = bx0 + profile.width / dun->block_wid;

    struct loc centre;
    int by, bx;

    /* Enforce the room profile's minimum depth */
    if (c->wpos.depth < profile.level) return false;

    /* Only allow at most two pit/nests room per level */
    if ((dun->pit_num >= z_info->level_pit_max) && profile.pit) return false;

    /* Expand the number of blocks if we might overflow */
    if (profile.height % dun->block_hgt) by2++;
    if (profile.width % dun->block_wid) bx2++;

    /* Does the profile allocate space, or the room find it? */
    if (finds_own_space)
    {
        /* Try to build a room, pass silly place so room finds its own */
        loc_init(&centre, c->width, c->height);
        if (!profile.builder(p, c, &centre, profile.rating)) return false;
    }
    else
    {
        /* Never run off the screen */
        if ((by1 < 0) || (by2 >= dun->row_blocks)) return false;
        if ((bx1 < 0) || (bx2 >= dun->col_blocks)) return false;

        /* Verify open space */
        for (by = by1; by <= by2; by++)
        {
            for (bx = bx1; bx <= bx2; bx++)
            {
                /* Previous rooms prevent new ones */
                if (dun->room_map[by][bx]) return false;
            }
        }

        /* Get the location of the room */
        centre.y = ((by1 + by2 + 1) * dun->block_hgt) / 2;
        centre.x = ((bx1 + bx2 + 1) * dun->block_wid) / 2;

        /*
         * Save the room location (must be before builder call to
         * properly store entrance information).
         */
        if (dun->cent_n < z_info->level_room_max)
        {
            loc_copy(&dun->cent[dun->cent_n], &centre);
            dun->cent_n++;
        }

        /* Try to build a room */
        if (!profile.builder(p, c, &centre, profile.rating))
        {
            --dun->cent_n;
            return false;
        }

        /* Reserve some blocks */
        for (by = by1; by < by2; by++)
            for (bx = bx1; bx < bx2; bx++)
                dun->room_map[by][bx] = true;
    }

    /* Count pit/nests */
    if (profile.pit) dun->pit_num++;

    /* Success */
    return true;
}