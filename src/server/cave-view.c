/*
 * File: cave-view.c
 * Purpose: Line-of-sight and view calculations
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
 * Approximate Distance between two points.
 *
 * When either the X or Y component dwarfs the other component,
 * this function is almost perfect, and otherwise, it tends to
 * over-estimate about one grid per fifteen grids of distance.
 *
 * Algorithm: hypot(dy,dx) = max(dy,dx) + min(dy,dx) / 2
 */
int distance(struct loc *grid1, struct loc *grid2)
{
    /* Find the absolute y/x distance components */
    int ay = abs(grid2->y - grid1->y);
    int ax = abs(grid2->x - grid1->x);

    /* Approximate the distance */
    return (ay > ax)? (ay + (ax >> 1)): (ax + (ay >> 1));
}


/*
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.
 *
 * This function returns true if a "line of sight" can be traced from the
 * center of the grid (x1,y1) to the center of the grid (x2,y2), with all
 * of the grids along this path (except for the endpoints) being non-wall
 * grids.  Actually, the "chess knight move" situation is handled by some
 * special case code which allows the grid diagonally next to the player
 * to be obstructed, because this yields better gameplay semantics.  This
 * algorithm is totally reflexive, except for "knight move" situations.
 *
 * Because this function uses (short) ints for all calculations, overflow
 * may occur if dx and dy exceed 90.
 *
 * Once all the degenerate cases are eliminated, we determine the "slope"
 * ("m"), and we use special "fixed point" mathematics in which we use a
 * special "fractional component" for one of the two location components
 * ("qy" or "qx"), which, along with the slope itself, are "scaled" by a
 * scale factor equal to "abs(dy*dx*2)" to keep the math simple.  Then we
 * simply travel from start to finish along the longer axis, starting at
 * the border between the first and second tiles (where the y offset is
 * thus half the slope), using slope and the fractional component to see
 * when motion along the shorter axis is necessary.  Since we assume that
 * vision is not blocked by "brushing" the corner of any grid, we must do
 * some special checks to avoid testing grids which are "brushed" but not
 * actually "entered".
 *
 * Angband three different "line of sight" type concepts, including this
 * function (which is used almost nowhere), the "project()" method (which
 * is used for determining the paths of projectables and spells and such),
 * and the "update_view()" concept (which is used to determine which grids
 * are "viewable" by the player, which is used for many things, such as
 * determining which grids are illuminated by the player's torch, and which
 * grids and monsters can be "seen" by the player, etc).
 */
bool los(struct chunk *c, struct loc *grid1, struct loc *grid2)
{
    /* Delta */
    int dx, dy;

    /* Absolute */
    int ax, ay;

    /* Signs */
    int sx, sy;

    /* Fractions */
    int qx, qy;

    /* Scanners */
    struct loc scan;

    /* Scale factors */
    int f1, f2;

    /* Slope, or 1/Slope, of LOS */
    int m;

    /* Extract the offset */
    dy = grid2->y - grid1->y;
    dx = grid2->x - grid1->x;

    /* Extract the absolute offset */
    ay = ABS(dy);
    ax = ABS(dx);

    /* Handle adjacent (or identical) grids */
    if ((ax < 2) && (ay < 2)) return true;

    /* Directly South/North */
    if (!dx)
    {
        /* South -- check for walls */
        if (dy > 0)
        {
            scan.x = grid1->x;
            for (scan.y = grid1->y + 1; scan.y < grid2->y; scan.y++)
            {
                if (!square_isprojectable(c, &scan)) return false;
            }
        }

        /* North -- check for walls */
        else
        {
            scan.x = grid1->x;
            for (scan.y = grid1->y - 1; scan.y > grid2->y; scan.y--)
            {
                if (!square_isprojectable(c, &scan)) return false;
            }
        }

        /* Assume los */
        return true;
    }

    /* Directly East/West */
    if (!dy)
    {
        /* East -- check for walls */
        if (dx > 0)
        {
            scan.y = grid1->y;
            for (scan.x = grid1->x + 1; scan.x < grid2->x; scan.x++)
            {
                if (!square_isprojectable(c, &scan)) return false;
            }
        }

        /* West -- check for walls */
        else
        {
            scan.y = grid1->y;
            for (scan.x = grid1->x - 1; scan.x > grid2->x; scan.x--)
            {
                if (!square_isprojectable(c, &scan)) return false;
            }
        }

        /* Assume los */
        return true;
    }

    /* Extract some signs */
    sx = (dx < 0) ? -1 : 1;
    sy = (dy < 0) ? -1 : 1;

    /* Vertical and horizontal "knights" */
    loc_init(&scan, grid1->x, grid1->y + sy);
    if ((ax == 1) && (ay == 2) && square_isprojectable(c, &scan))
        return true;
    loc_init(&scan, grid1->x + sx, grid1->y);
    if ((ay == 1) && (ax == 2) && square_isprojectable(c, &scan))
        return true;

    /* Calculate scale factor div 2 */
    f2 = (ax * ay);

    /* Calculate scale factor */
    f1 = f2 << 1;

    /* Travel horizontally */
    if (ax >= ay)
    {
        /* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
        qy = ay * ay;
        m = qy << 1;

        scan.x = grid1->x + sx;

        /* Consider the special case where slope == 1. */
        if (qy == f2)
        {
            scan.y = grid1->y + sy;
            qy -= f1;
        }
        else
            scan.y = grid1->y;

        /* Note (below) the case (qy == f2), where */
        /* the LOS exactly meets the corner of a tile. */
        while (grid2->x - scan.x)
        {
            if (!square_isprojectable(c, &scan)) return false;

            qy += m;

            if (qy < f2)
                scan.x += sx;
            else if (qy > f2)
            {
                scan.y += sy;
                if (!square_isprojectable(c, &scan)) return false;
                qy -= f1;
                scan.x += sx;
            }
            else
            {
                scan.y += sy;
                qy -= f1;
                scan.x += sx;
            }
        }
    }

    /* Travel vertically */
    else
    {
        /* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
        qx = ax * ax;
        m = qx << 1;

        scan.y = grid1->y + sy;

        if (qx == f2)
        {
            scan.x = grid1->x + sx;
            qx -= f1;
        }
        else
            scan.x = grid1->x;

        /* Note (below) the case (qx == f2), where */
        /* the LOS exactly meets the corner of a tile. */
        while (grid2->y - scan.y)
        {
            if (!square_isprojectable(c, &scan)) return false;

            qx += m;

            if (qx < f2)
                scan.y += sy;
            else if (qx > f2)
            {
                scan.x += sx;
                if (!square_isprojectable(c, &scan)) return false;
                qx -= f1;
                scan.y += sy;
            }
            else
            {
                scan.x += sx;
                qx -= f1;
                scan.y += sy;
            }
        }
    }

    /* Assume los */
    return true;
}


/*
 * Some comments on the dungeon related data structures and functions...
 *
 * Angband is primarily a dungeon exploration game, and it should come as
 * no surprise that the internal representation of the dungeon has evolved
 * over time in much the same way as the game itself, to provide semantic
 * changes to the game itself, to make the code simpler to understand, and
 * to make the executable itself faster or more efficient in various ways.
 *
 * There are a variety of dungeon related data structures, and associated
 * functions, which store information about the dungeon, and provide methods
 * by which this information can be accessed or modified.
 *
 * Some of this information applies to the dungeon as a whole, such as the
 * list of unique monsters which are still alive.  Some of this information
 * only applies to the current dungeon level, such as the current depth, or
 * the list of monsters currently inhabiting the level.  And some of the
 * information only applies to a single grid of the current dungeon level,
 * such as whether the grid is illuminated, or whether the grid contains a
 * monster, or whether the grid can be seen by the player.  If Angband was
 * to be turned into a multi-player game, some of the information currently
 * associated with the dungeon should really be associated with the player,
 * such as whether a given grid is viewable by a given player.
 *
 * Currently, a lot of the information about the dungeon is stored in ways
 * that make it very efficient to access or modify the information, while
 * still attempting to be relatively conservative about memory usage, even
 * if this means that some information is stored in multiple places, or in
 * ways which require the use of special code idioms.  For example, each
 * monster record in the monster array contains the location of the monster,
 * and each cave grid has an index into the monster array, or a zero if no
 * monster is in the grid.  This allows the monster code to efficiently see
 * where the monster is located, while allowing the dungeon code to quickly
 * determine not only if a monster is present in a given grid, but also to
 * find out which monster.  The extra space used to store the information
 * twice is inconsequential compared to the speed increase.
 *
 * Several pieces of information about each cave grid are stored in the
 * info field of the "cave->squares" array, which is a special array of
 * bitflags.
 *
 * The "SQUARE_ROOM" flag is used to determine which grids are part of "rooms",
 * and thus which grids are affected by "illumination" spells.
 *
 * The "SQUARE_VAULT" flag is used to determine which grids are part of
 * "vaults", and thus which grids cannot serve as the destinations of player
 * teleportation.
 *
 * The "SQUARE_NO_TELEPORT" flag is used to determine which grids are part of "pits",
 * and thus which grids cannot serve as origin for player/monster teleportation.
 *
 * The "SQUARE_GLOW" flag is used to determine which grids are "permanently
 * illuminated". This flag is used by the update_view() function to help
 * determine which viewable grids may be "seen" by the player. This flag
 * has special semantics for wall grids (see "update_view()").
 *
 * The "SQUARE_VIEW" flag is used to determine which grids are currently in
 * line of sight of the player.  This flag is set by (and used by) the
 * "update_view()" function.  This flag is used by any code which needs to
 * know if the player can "view" a given grid.  This flag is used by the
 * "map_info()" function for some optional special lighting effects.  The
 * "square_isview()" function wraps an abstraction around this flag, but
 * certain code idioms are much more efficient.  This flag is used to check
 * if a modification to a terrain feature might affect the player's field of
 * view.  This flag is used to see if certain monsters are "visible" to the
 * player.  This flag is used to allow any monster in the player's field of
 * view to "sense" the presence of the player.
 *
 * The "SQUARE_SEEN" flag is used to determine which grids are currently in
 * line of sight of the player and also illuminated in some way.  This flag
 * is set by the "update_view()" function, using computations based on the
 * "SQUARE_VIEW" and "SQUARE_GLOW" flags and terrain of various grids.
 * This flag is used by any code which needs to know if the player can "see" a
 * given grid.  This flag is used by the "map_info()" function both to see
 * if a given "boring" grid can be seen by the player, and for some optional
 * special lighting effects.  The "square_isseen()" function wraps an
 * abstraction around this flag, but certain code idioms are much more
 * efficient.  This flag is used to see if certain monsters are "visible" to
 * the player.  This flag is never set for a grid unless "SQUARE_VIEW" is also
 * set for the grid.  Whenever the terrain or "SQUARE_GLOW" flag changes
 * for a grid which has the "SQUARE_VIEW" flag set, the "SQUARE_SEEN" flag must
 * be recalculated.  The simplest way to do this is to call "update_view()"
 * whenever the terrain or "SQUARE_GLOW" flags change for a grid which has
 * "SQUARE_VIEW" set.
 *
 * The "SQUARE_WASSEEN" flag is used to determine if the "SQUARE_SEEN" flag for a
 * grid has changed during the "update_view()" function. This flag must always
 * be cleared by any code which sets it.
 *
 * The "SQUARE_CLOSE_PLAYER" flag is set for squares that are seen and either
 * in the player's light radius or the UNLIGHT detection radius. It is used
 * by "map_info()" to select which lighting effects to apply to a square.
 *
 * The "update_view()" function is an extremely important function.  It is
 * called only when the player moves, significant terrain changes, or the
 * player's blindness or torch radius changes.  Note that when the player
 * is resting, or performing any repeated actions (like digging, disarming,
 * farming, etc), there is no need to call the "update_view()" function, so
 * even if it was not very efficient, this would really only matter when the
 * player was "running" through the dungeon.  It sets the "SQUARE_VIEW" flag
 * on every cave grid in the player's field of view.  It also checks the torch
 * radius of the player, and sets the "SQUARE_SEEN" flag for every grid which
 * is in the "field of view" of the player and which is also "illuminated",
 * either by the player's torch (if any), light from monsters, light from
 * bright terrain, or by any permanent light source (as marked by SQUARE_GLOW).
 * It could use and help maintain information about multiple light sources,
 * which would be helpful in a multi-player version of Angband.
 *
 * Note that the "update_view()" function allows, among other things, a room
 * to be "partially" seen as the player approaches it, with a growing cone
 * of floor appearing as the player gets closer to the door.  Also, by not
 * turning on the "memorize perma-lit grids" option, the player will only
 * "see" those floor grids which are actually in line of sight.  And best
 * of all, you can now activate the special lighting effects to indicate
 * which grids are actually in the player's field of view by using dimmer
 * colors for grids which are not in the player's field of view, and/or to
 * indicate which grids are illuminated only by the player's torch by using
 * the color yellow for those grids.
 *
 * It seems as though slight modifications to the "update_view()" functions
 * would allow us to determine "reverse" line-of-sight as well as "normal"
 * line-of-sight", which would allow monsters to have a more "correct" way
 * to determine if they can "see" the player, since right now, they "cheat"
 * somewhat and assume that if the player has "line of sight" to them, then
 * they can "pretend" that they have "line of sight" to the player.  But if
 * such a change was attempted, the monsters would actually start to exhibit
 * some undesirable behavior, such as "freezing" near the entrances to long
 * hallways containing the player, and code would have to be added to make
 * the monsters move around even if the player was not detectable, and to
 * "remember" where the player was last seen, to avoid looking stupid.
 *
 * Note that the "SQUARE_GLOW" flag means that a grid is permanently lit in
 * some way. However, for the player to "see" the grid, as determined by
 * the "SQUARE_SEEN" flag, the player must not be blind, the grid must have
 * the "SQUARE_VIEW" flag set, and if the grid is a "wall" grid, and it is
 * not lit by some other light source, then it must touch a projectable grid
 * which has both the "SQUARE_GLOW" and "SQUARE_VIEW" flags set. This last
 * part about wall grids is induced by the semantics of "SQUARE_GLOW" as
 * applied to wall grids, and checking the technical requirements can be very
 * expensive, especially since the grid may be touching some "illegal" grids.
 * Luckily, it is more or less correct to restrict the "touching" grids from
 * the eight "possible" grids to the (at most) three grids which are touching
 * the grid, and which are closer to the player than the grid itself, which
 * eliminates more than half of the work, including all of the potentially
 * "illegal" grids, if at most one of the three grids is a "diagonal" grid.
 * In addition, in almost every situation, it is possible to ignore the
 * "SQUARE_VIEW" flag on these three "touching" grids, for a variety of
 * technical reasons. Finally, note that in most situations, it is only
 * necessary to check a single "touching" grid, in fact, the grid which is
 * strictly closest to the player of all the touching grids, and in fact,
 * it is normally only necessary to check the "SQUARE_GLOW" flag of that grid,
 * again, for various technical reasons. However, one of the situations which
 * does not work with this last reduction is the very common one in which the
 * player approaches an illuminated room from a dark hallway, in which the
 * two wall grids which form the "entrance" to the room would not be marked
 * as "SQUARE_SEEN", since of the three "touching" grids nearer to the player
 * than each wall grid, only the farthest of those grids is itself marked
 * "SQUARE_GLOW".
 *
 *
 * Here are some pictures of the legal "light source" radius values, in
 * which the numbers indicate the "order" in which the grids could have
 * been calculated, if desired.  Note that the code will work with larger
 * radiuses, though currently yields such a radius, and the game would
 * become slower in some situations if it did.
 *
 *       Rad=0     Rad=1      Rad=2        Rad=3
 *      No-Light Torch,etc   Lantern     Artifacts
 *
 *                                          333
 *                             333         43334
 *                  212       32123       3321233
 *         @        1@1       31@13       331@133
 *                  212       32123       3321233
 *                             333         43334
 *                                          333
 */


/*
 * Mark the currently seen grids, then wipe in preparation for recalculating
 */
static void mark_wasseen(struct player *p, struct chunk *c)
{
    struct loc begin, end;
    struct loc_iterator iter;

    loc_init(&begin, 0, 0);
    loc_init(&end, c->width, c->height);
    loc_iterator_first(&iter, &begin, &end);

    /* Save the old "view" grids for later */
    do
    {
        if (square_isseen(p, &iter.cur))
            sqinfo_on(square_p(p, &iter.cur)->info, SQUARE_WASSEEN);

        /* PWMAngband: save the old "SQUARE_CLOSE_PLAYER" flag */
        if (sqinfo_has(square_p(p, &iter.cur)->info, SQUARE_CLOSE_PLAYER))
            sqinfo_on(square_p(p, &iter.cur)->info, SQUARE_WASCLOSE);

        /* PWMAngband: save the old "lit" flag */
        if (square_islit(p, &iter.cur))
            sqinfo_on(square_p(p, &iter.cur)->info, SQUARE_WASLIT);

        sqinfo_off(square_p(p, &iter.cur)->info, SQUARE_VIEW);
        sqinfo_off(square_p(p, &iter.cur)->info, SQUARE_SEEN);
        sqinfo_off(square_p(p, &iter.cur)->info, SQUARE_CLOSE_PLAYER);
    }
    while (loc_iterator_next_strict(&iter));
}


/*
 * Help glow_can_light_wall(), add_light() and calc_lighting(): check for
 * whether a wall can appear to be lit, as viewed by the player, by a light
 * source regardless of line-of-sight details.
 *
 * c Is the chunk in which to do the evaluation.
 * p Is the player to test.
 * sgrid Is the location of the light source.
 * wgrid Is the location of the wall.
 *
 * Return true if the wall will appear to be lit for the player.
 * Otherwise, return false.
 */
static bool source_can_light_wall(struct chunk *c, struct player *p, struct loc *sgrid,
    struct loc *wgrid)
{
    struct loc sn, pn, cn;

    /*
     * If the light source is coincident with the wall, all faces will be
     * lit, and the player can potentially see it if it's within range and
     * the line of sight isn't broken.
     */
    next_grid(&sn, wgrid, motion_dir(wgrid, sgrid));
    if (loc_eq(&sn, wgrid)) return true;

    /*
     * If the player is coincident with the wall, all faces of the wall are
     * visible to the player and the player can see whichever of those is
     * lit by the light source.
     */
    next_grid(&pn, wgrid, motion_dir(wgrid, &p->grid));
    if (loc_eq(&pn, wgrid)) return true;

    /*
     * For the lit face of the wall to be visible to the player, the
     * view directions from the wall to the player and the wall to the
     * light source must share at least one component.
     */
    if (sn.x == pn.x)
    {
        /*
         * If the view directions share both components, the lit face
         * will be visible to the player if in range and the line of
         * sight isn't broken.
         */
        if (sn.y == pn.y) return true;
        cn.x = sn.x;
        cn.y = 0;
    }
    else if (sn.y == pn.y)
    {
        cn.x = 0;
        cn.y = sn.y;
    }
    else
    {
        /*
         * If the view directions don't share a component, the lit face
         * is not visible to the player.
         */
        return false;
    }

    /*
     * When only one component of the view directions is shared, take the
     * common component and test whether there's a wall there that would
     * block the player's view of the lit face. That prevents instances
     * like this:
     *  p
     * ###1#
     *  @
     * where both the light-emitting monster, 'p', and the player, '@',
     * have line of sight to the wall, '1', but the face of '1' that would
     * be lit is blocked by the wall immediately to the left of '1'.
     */
    return square_allowslos(c, &cn);
}


/*
 * Help calc_lighting(): check for whether a wall marked with SQUARE_GLOW
 * can appear to be lit, as viewed by the player regardless of line-of-sight
 * details.
 *
 * c Is the chunk in which to do the evaluation.
 * p Is the player to test.
 * wgrid Is the location of the wall.
 *
 * Return true if the wall will appear to be lit for the player.
 * Otherwise, return false.
 */
static bool glow_can_light_wall(struct chunk *c, struct player *p, struct loc *wgrid)
{
    struct loc pn, chk;

    /* If the player is in the wall grid, the player will see the lit face. */
    next_grid(&pn, wgrid, motion_dir(wgrid, &p->grid));
    if (loc_eq(&pn, wgrid)) return true;

    /*
     * If the grid in the direction of the player is not a wall and is
     * glowing, it'll illuminate the wall.
     */
    if (square_allowslos(c, &pn) && square_isglow(c, &pn))
        return true;

    /*
     * Try the two neighboring squares adjacent to the one in the direction
     * of the player to see if one or more will illuminate the wall by
     * glowing. Those could be out of bounds if the direction isn't
     * diagonal.
     */
    if (pn.x != wgrid->x)
    {
        if (pn.y != wgrid->y)
        {
            chk.x = pn.x;
            chk.y = wgrid->y;
            if (square_allowslos(c, &chk) && square_isglow(c, &chk) &&
                source_can_light_wall(c, p, &chk, wgrid))
            {
                return true;
            }
            chk.x = wgrid->x;
            chk.y = pn.y;
            if (square_allowslos(c, &chk) && square_isglow(c, &chk) &&
                source_can_light_wall(c, p, &chk, wgrid))
            {
                return true;
            }
        }
        else
        {
            chk.x = pn.x;
            chk.y = wgrid->y - 1;
            if (square_in_bounds(c, &chk) && square_allowslos(c, &chk) && square_isglow(c, &chk) &&
                source_can_light_wall(c, p, &chk, wgrid))
            {
                return true;
            }
            chk.y = wgrid->y + 1;
            if (square_in_bounds(c, &chk) && square_allowslos(c, &chk) && square_isglow(c, &chk) &&
                source_can_light_wall(c, p, &chk, wgrid))
            {
                return true;
            }
        }
    }
    else
    {
        chk.y = pn.y;
        chk.x = wgrid->x - 1;
        if (square_in_bounds(c, &chk) && square_allowslos(c, &chk) && square_isglow(c, &chk) &&
            source_can_light_wall(c, p, &chk, wgrid))
        {
            return true;
        }
        chk.x = wgrid->x + 1;
        if (square_in_bounds(c, &chk) && square_allowslos(c, &chk) && square_isglow(c, &chk) &&
            source_can_light_wall(c, p, &chk, wgrid))
        {
            return true;
        }
    }

    /* The adjacent squares towards the player won't light the wall by glowing */
    return false;
}


/*
 * Help calc_lighting(): add in the effect of a light source.
 *
 * c Is the chunk to use.
 * p Is the player to use.
 * sgrid Is the location of the light source.
 * radius Is the radius, in grids, of the light source.
 * inten Is the intensity of the light source.
 *
 * This is a brute force approach. Some computation probably could be saved by
 * propagating the light out from the source and terminating paths when they
 * reach a wall.
 */
static void add_light(struct chunk *c, struct player *p, struct loc *sgrid, int radius, int inten)
{
    struct loc begin, end;
    struct loc_iterator iter;

    loc_init(&begin, -radius, -radius);
    loc_init(&end, radius, radius);
    loc_iterator_first(&iter, &begin, &end);

    do
    {
        struct loc grid;
        int dist;

        /* Get valid grids within the player's light effect radius */
        loc_sum(&grid, sgrid, &iter.cur);
        dist = distance(sgrid, &grid);
        if (!square_in_bounds(c, &grid)) continue;
        if (dist > radius) continue;

        /* Don't propagate the light through walls. */
        if (!los(c, sgrid, &grid)) continue;

        /* Only light a wall if the face lit is possibly visible to the player. */
        if (!square_allowslos(c, &grid) && !source_can_light_wall(c, p, sgrid, &grid)) continue;

        /* Adjust the light level */
        if (inten > 0)
        {
            /* Light getting less further away */
            square_p(p, &grid)->light += inten - dist;
        }
        else
        {
            /* Light getting greater further away */
            square_p(p, &grid)->light += inten + dist;
        }
    }
    while (loc_iterator_next(&iter));
}


/*
 * Calculate light level for every grid in view - stolen from Sil
 */
static void calc_lighting(struct player *p, struct chunk *c)
{
    int dir, k;
    int light = p->state.cur_light, radius = ABS(light) - 1;
    int old_light = p->square_light;
    struct loc begin, end;
    struct loc_iterator iter;

    loc_init(&begin, 0, 0);
    loc_init(&end, c->width, c->height);
    loc_iterator_first(&iter, &begin, &end);

    /* Starting values based on permanent light */
    do
    {
        if (square_isglow(c, &iter.cur) &&
            (square_allowslos(c, &iter.cur) || glow_can_light_wall(c, p, &iter.cur)))
        {
            square_p(p, &iter.cur)->light = 1;
        }
        else
            square_p(p, &iter.cur)->light = 0;

        /* Squares with bright terrain have intensity 2 */
        if (square_isbright(c, &iter.cur))
        {
            square_p(p, &iter.cur)->light += 2;
            for (dir = 0; dir < 8; dir++)
            {
                struct loc adj_grid;

                loc_sum(&adj_grid, &iter.cur, &ddgrid_ddd[dir]);
                if (!square_in_bounds(c, &adj_grid)) continue;

                /* Only brighten a wall if the player is in position to view the face that's lit up. */
                if (!square_allowslos(c, &adj_grid) &&
                    !source_can_light_wall(c, p, &iter.cur, &adj_grid))
                {
                    continue;
                }
                square_p(p, &adj_grid)->light += 1;
            }
        }
    }
    while (loc_iterator_next_strict(&iter));

    /* Light around the player */
    if (light) add_light(c, p, &p->grid, radius, light);

    /* Scan monster list and add monster light or darkness */
    for (k = 1; k < cave_monster_max(c); k++)
    {
        /* Check the k'th monster */
        struct monster *mon = cave_monster(c, k);

        /* Skip dead monsters */
        if (!mon->race) continue;

        /* Skip if the monster is hidden */
        if (monster_is_camouflaged(mon)) continue;

        /* Get light info for this monster */
        light = mon->race->light;
        radius = ABS(light) - 1;

        /* Skip monsters not affecting light */
        if (!light) continue;

        /* Skip if the player can't see it. */
        if (distance(&p->grid, &mon->grid) - radius > z_info->max_sight) continue;

        /* Light or darken around the monster */
        add_light(c, p, &mon->grid, radius, light);
    }

    /* Scan player list and add player lights */
    for (k = 1; k <= NumPlayers; k++)
    {
        /* Check the k'th player */
        struct player *q = player_get(k);

        /* Ignore the player that we're updating */
        if (q == p) continue;

        /* Skip players not on this level */
        if (!wpos_eq(&q->wpos, &p->wpos)) continue;

        /* Skip if the player is hidden */
        if (q->k_idx) continue;

        /* Get light info for this player */
        light = q->state.cur_light;
        radius = ABS(light) - 1;

        /* Skip players not affecting light */
        if (!light) continue;

        /* Skip if the player can't see it. */
        if (distance(&p->grid, &q->grid) - radius > z_info->max_sight) continue;

        /* Light or darken around the player */
        add_light(c, p, &q->grid, radius, light);
    }

    /* Update light level indicator */
    if (square_light(p, &p->grid) != old_light)
    {
        p->square_light = square_light(p, &p->grid);
        p->upkeep->redraw |= PR_STATE;
    }
}


/*
 * Make a square part of the current view
 */
static void become_viewable(struct player *p, struct chunk *c, struct loc *grid, bool close)
{
    int x = grid->x;
    int y = grid->y;

    /* Already viewable, nothing to do */
    if (square_isview(p, grid)) return;

    /* Add the grid to the view, make seen if it's close enough to the player */
    sqinfo_on(square_p(p, grid)->info, SQUARE_VIEW);
    if (close)
    {
        sqinfo_on(square_p(p, grid)->info, SQUARE_SEEN);
        sqinfo_on(square_p(p, grid)->info, SQUARE_CLOSE_PLAYER);
    }

    /* Mark lit grids, and walls near to them, as seen */
    if (square_islit(p, grid))
    {
        /* For walls, check for a lit grid closer to the player */
        if (!square_allowslos(c, grid))
        {
            struct loc gridc;

            loc_init(&gridc, ((x < p->grid.x)? (x + 1): (x > p->grid.x)? (x - 1): x),
                ((y < p->grid.y)? (y + 1): (y > p->grid.y)? (y - 1): y));
            if (square_islit(p, &gridc))
                sqinfo_on(square_p(p, grid)->info, SQUARE_SEEN);
        }
        else
            sqinfo_on(square_p(p, grid)->info, SQUARE_SEEN);
    }
}


/*
 * Decide whether to include a square in the current view
 */
static void update_view_one(struct player *p, struct chunk *c, struct loc *grid)
{
    int d = distance(grid, &p->grid);
    bool close = ((d < p->state.cur_light)? true: false);
    struct loc cgrid;

    loc_copy(&cgrid, grid);

    /* Too far away */
    if (d > z_info->max_sight) return;

    /* UNLIGHT players have a special radius of view */
    /*if (player_has(p, PF_UNLIGHT) && (p->state.cur_light <= 1))
        close = (d < (2 + p->lev / 6 - p->state.cur_light)? true: false);*/

    /*
     * Special case for wall lighting. If we are a wall and the square in
     * the direction of the player is in LOS, we are in LOS. This avoids
     * situations like:
     *
     * #1#############
     * #............@#
     * ###############
     *
     * where the wall cell marked '1' would not be lit because the LOS
     * algorithm runs into the adjacent wall cell.
     */
    if (!square_allowslos(c, grid))
    {
        int dx = grid->x - p->grid.x;
        int dy = grid->y - p->grid.y;
        int ax = ABS(dx);
        int ay = ABS(dy);
        int sx = (dx > 0)? 1: -1;
        int sy = (dy > 0)? 1: -1;

        loc_init(&cgrid,
            ((grid->x < p->grid.x)? (grid->x + 1): (grid->x > p->grid.x)? (grid->x - 1): grid->x),
            ((grid->y < p->grid.y)? (grid->y + 1): (grid->y > p->grid.y)? (grid->y - 1): grid->y));

        /*
         * Check that the cell we're trying to steal LOS from isn't a
         * wall. If we don't do this, double-thickness walls will have
         * both sides visible.
         */
        if (!square_allowslos(c, &cgrid)) loc_copy(&cgrid, grid);

        /* Check if we got here via the 'knight's move' rule and, if so, don't steal LOS. */
        if ((ax == 2) && (ay == 1))
        {
            struct loc grid1, grid2;

            loc_init(&grid1, grid->x - sx, grid->y);
            loc_init(&grid2, grid->x - sx, grid->y - sy);
            if (square_in_bounds(c, &grid1) && square_allowslos(c, &grid1) &&
                square_in_bounds(c, &grid2) && !square_allowslos(c, &grid2))
            {
                loc_copy(&cgrid, grid);
            }
        }
        else if ((ax == 1) && (ay == 2))
        {
            struct loc grid1, grid2;

            loc_init(&grid1, grid->x, grid->y - sy);
            loc_init(&grid2, grid->x - sx, grid->y - sy);
            if (square_in_bounds(c, &grid1) && square_allowslos(c, &grid1) &&
                square_in_bounds(c, &grid2) && !square_allowslos(c, &grid2))
            {
                loc_copy(&cgrid, grid);
            }
        }
    }

    if (los(c, &p->grid, &cgrid))
        become_viewable(p, c, grid, close);
}


/*
 * Update view for a single square
 */
static void update_one(struct player *p, struct chunk *c, struct loc *grid)
{
    bool is_close = sqinfo_has(square_p(p, grid)->info, SQUARE_CLOSE_PLAYER);
    bool was_close = sqinfo_has(square_p(p, grid)->info, SQUARE_WASCLOSE);
    bool is_lit = square_islit(p, grid);
    bool was_lit = sqinfo_has(square_p(p, grid)->info, SQUARE_WASLIT);

    /* Remove view if blind, check visible squares for traps */
    if (p->timed[TMD_BLIND])
    {
        sqinfo_off(square_p(p, grid)->info, SQUARE_SEEN);
        sqinfo_off(square_p(p, grid)->info, SQUARE_CLOSE_PLAYER);
    }
    else if (square_isseen(p, grid) && square_issecrettrap(c, grid))
        square_reveal_trap(p, grid, false, true);

    /* Square went from unseen -> seen */
    if (square_isseen(p, grid) && !square_wasseen(p, grid))
    {
        if (square_isfeel(c, grid) && square_ispfeel(p, grid))
        {
            p->cave->feeling_squares++;
            sqinfo_off(square_p(p, grid)->info, SQUARE_FEEL);

            /* Don't display feeling if it will display for the new level */
            if (p->cave->feeling_squares == z_info->feeling_need)
            {
                display_feeling(p, true);
                p->upkeep->redraw |= (PR_STATE);
            }
        }

        square_note_spot_aux(p, c, grid);
        square_light_spot_aux(p, c, grid);
    }

    /* Square went from seen -> unseen */
    if (!square_isseen(p, grid) && square_wasseen(p, grid))
        square_light_spot_aux(p, c, grid);

    /* PWMAngband: square went from torchlit to lit or lit to torchlit */
    if ((is_close && !was_close) || (!is_close && was_close))
        square_light_spot_aux(p, c, grid);

    /* PWMAngband: square went from unlit to lit or lit to unlit */
    if ((is_lit && !was_lit) || (!is_lit && was_lit))
        square_light_spot_aux(p, c, grid);

    sqinfo_off(square_p(p, grid)->info, SQUARE_WASSEEN);

    /* PWMAngband: also clear "SQUARE_WASCLOSE" and "SQUARE_WASLIT" flags */
    sqinfo_off(square_p(p, grid)->info, SQUARE_WASCLOSE);
    sqinfo_off(square_p(p, grid)->info, SQUARE_WASLIT);
}


/*
 * Update the player's current view
 */
void update_view(struct player *p, struct chunk *c)
{
    struct loc begin, end;
    struct loc_iterator iter;

    /* Record the current view */
    mark_wasseen(p, c);

    /* Calculate light levels */
    calc_lighting(p, c);

    /* Assume we can view the player grid */
    sqinfo_on(square_p(p, &p->grid)->info, SQUARE_VIEW);
    /*if ((p->state.cur_light > 0) || square_islit(p, &p->grid) || player_has(p, PF_UNLIGHT))*/
    if ((p->state.cur_light > 0) || square_islit(p, &p->grid))
    {
        sqinfo_on(square_p(p, &p->grid)->info, SQUARE_SEEN);
        sqinfo_on(square_p(p, &p->grid)->info, SQUARE_CLOSE_PLAYER);
    }

    loc_init(&begin, 0, 0);
    loc_init(&end, c->width, c->height);
    loc_iterator_first(&iter, &begin, &end);

    /*
     * If the player is blind and in terrain that was remembered to be
     * impassable, forget the remembered terrain.
     */
    if (p->timed[TMD_BLIND] && square_isknown(p, &p->grid) && !square_ispassable_p(p, &p->grid))
    {
        /* PWMAngband: player must not be able to move through impassable terrain */
        if (!player_passwall(p)) square_forget(p, &p->grid);
    }

    /* Squares we have LOS to get marked as in the view, and perhaps seen */
    do
    {
        update_view_one(p, c, &iter.cur);
    }
    while (loc_iterator_next_strict(&iter));

    loc_iterator_first(&iter, &begin, &end);

    /* Update each grid */
    do
    {
        update_one(p, c, &iter.cur);
    }
    while (loc_iterator_next_strict(&iter));
}


/*
 * Returns true if the player's grid is dark
 */
bool no_light(struct player *p)
{
    return (!square_isseen(p, &p->grid));
}


#if 0
static void add_monster_lights(struct player *p, struct chunk *c)
{
    bool in_los = los(c, &p->grid, &mon->grid);

    /* If the monster isn't visible we can only light open tiles */
    if (!in_los && !square_isprojectable(c, &grid)) continue;

    /* If the tile itself isn't in LOS, don't light it */
    if (!los(c, &p->grid, &grid)) continue;

    /* Mark the square lit and seen */
    sqinfo_on(square_p(p, &grid)->info, SQUARE_VIEW);
    sqinfo_on(square_p(p, &grid)->info, SQUARE_SEEN);
}

static void add_player_lights(struct player *p, struct chunk *c)
{
    bool in_los = los(c, &p->grid, &q->grid);

    /* If the player isn't visible we can only light open tiles */
    if (!in_los && !square_isprojectable(c, &grid)) continue;

    /* If the tile itself isn't in LOS, don't light it */
    if (!los(c, &p->grid, &grid)) continue;

    /* If the tile isn't in LOS of the other player, don't light it either */
    if (!los(c, &q->grid, &grid)) continue;

    /* Mark the square lit and seen */
    sqinfo_on(square_p(p, &grid)->info, SQUARE_VIEW);
    sqinfo_on(square_p(p, &grid)->info, SQUARE_SEEN);
}
#endif