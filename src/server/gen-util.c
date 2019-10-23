/*
 * File: gen-util.c
 * Purpose: Dungeon generation utilities
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2013 Erik Osheim, Nick McConnell
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
#include <math.h>


/*
 * This file contains various utility functions for dungeon generation - mostly
 * for finding appropriate grids for some purposes, or placing things.
 */


/* Number of trees in town */
byte trees_in_town;


/*
 * Accept values for y and x (considered as the endpoints of lines) between
 * 0 and 40, and return an angle in degrees (divided by two).
 *
 * This table's input and output need some processing:
 *
 * Because this table gives degrees for a whole circle, up to radius 20, its
 * origin is at (x,y) = (20, 20).  Therefore, the input code needs to find
 * the origin grid (where the lines being compared come from), and then map
 * it to table grid 20,20.  Do not, however, actually try to compare the
 * angle of a line that begins and ends at the origin with any other line -
 * it is impossible mathematically, and the table will return the value "255".
 *
 * The output of this table also needs to be massaged, in order to avoid the
 * discontinuity at 0/180 degrees.  This can be done by:
 *   rotate = 90 - first value
 *   this rotates the first input to the 90 degree line)
 *   tmp = ABS(second value + rotate) % 180
 *   diff = ABS(90 - tmp) = the angular difference (divided by two) between
 *   the first and second values.
 *
 * Note that grids diagonal to the origin have unique angles.
 */
byte get_angle_to_grid[41][41] =
{
  {  68,  67,  66,  65,  64,  63,  62,  62,  60,  59,  58,  57,  56,  55,  53,  52,  51,  49,  48,  46,  45,  44,  42,  41,  39,  38,  37,  35,  34,  33,  32,  31,  30,  28,  28,  27,  26,  25,  24,  24,  23 },
  {  69,  68,  67,  66,  65,  64,  63,  62,  61,  60,  59,  58,  56,  55,  54,  52,  51,  49,  48,  47,  45,  43,  42,  41,  39,  38,  36,  35,  34,  32,  31,  30,  29,  28,  27,  26,  25,  24,  24,  23,  22 },
  {  69,  69,  68,  67,  66,  65,  64,  63,  62,  61,  60,  58,  57,  56,  54,  53,  51,  50,  48,  47,  45,  43,  42,  40,  39,  37,  36,  34,  33,  32,  30,  29,  28,  27,  26,  25,  24,  24,  23,  22,  21 },
  {  70,  69,  69,  68,  67,  66,  65,  64,  63,  61,  60,  59,  58,  56,  55,  53,  52,  50,  48,  47,  45,  43,  42,  40,  38,  37,  35,  34,  32,  31,  30,  29,  27,  26,  25,  24,  24,  23,  22,  21,  20 },
  {  71,  70,  69,  69,  68,  67,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  49,  47,  45,  43,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  23,  22,  21,  20,  19 },
  {  72,  71,  70,  69,  69,  68,  67,  65,  64,  63,  62,  60,  59,  58,  56,  54,  52,  51,  49,  47,  45,  43,  41,  39,  38,  36,  34,  32,  31,  30,  28,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18 },
  {  73,  72,  71,  70,  69,  69,  68,  66,  65,  64,  63,  61,  60,  58,  57,  55,  53,  51,  49,  47,  45,  43,  41,  39,  37,  35,  33,  32,  30,  29,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18,  17 },
  {  73,  73,  72,  71,  70,  70,  69,  68,  66,  65,  64,  62,  61,  59,  57,  56,  54,  51,  49,  47,  45,  43,  41,  39,  36,  34,  33,  31,  29,  28,  26,  25,  24,  23,  21,  20,  20,  19,  18,  17,  17 },
  {  75,  74,  73,  72,  72,  71,  70,  69,  68,  66,  65,  63,  62,  60,  58,  56,  54,  52,  50,  47,  45,  43,  40,  38,  36,  34,  32,  30,  28,  27,  25,  24,  23,  21,  20,  19,  18,  18,  17,  16,  15 },
  {  76,  75,  74,  74,  73,  72,  71,  70,  69,  68,  66,  65,  63,  61,  59,  57,  55,  53,  50,  48,  45,  42,  40,  37,  35,  33,  31,  29,  27,  25,  24,  23,  21,  20,  19,  18,  17,  16,  16,  15,  14 },
  {  77,  76,  75,  75,  74,  73,  72,  71,  70,  69,  68,  66,  64,  62,  60,  58,  56,  53,  51,  48,  45,  42,  39,  37,  34,  32,  30,  28,  26,  24,  23,  21,  20,  19,  18,  17,  16,  15,  15,  14,  13 },
  {  78,  77,  77,  76,  75,  75,  74,  73,  72,  70,  69,  68,  66,  64,  62,  60,  57,  54,  51,  48,  45,  42,  39,  36,  33,  30,  28,  26,  24,  23,  21,  20,  18,  17,  16,  15,  15,  14,  13,  13,  12 },
  {  79,  79,  78,  77,  77,  76,  75,  74,  73,  72,  71,  69,  68,  66,  63,  61,  58,  55,  52,  49,  45,  41,  38,  35,  32,  29,  27,  24,  23,  21,  19,  18,  17,  16,  15,  14,  13,  13,  12,  11,  11 },
  {  80,  80,  79,  79,  78,  77,  77,  76,  75,  74,  73,  71,  69,  68,  65,  63,  60,  57,  53,  49,  45,  41,  37,  33,  30,  27,  25,  23,  21,  19,  17,  16,  15,  14,  13,  13,  12,  11,  11,  10,  10 },
  {  82,  81,  81,  80,  80,  79,  78,  78,  77,  76,  75,  73,  72,  70,  68,  65,  62,  58,  54,  50,  45,  40,  36,  32,  28,  25,  23,  20,  18,  17,  15,  14,  13,  12,  12,  11,  10,  10,   9,   9,   8 },
  {  83,  83,  82,  82,  81,  81,  80,  79,  79,  78,  77,  75,  74,  72,  70,  68,  64,  60,  56,  51,  45,  39,  34,  30,  26,  23,  20,  18,  16,  15,  13,  12,  11,  11,  10,   9,   9,   8,   8,   7,   7 },
  {  84,  84,  84,  83,  83,  83,  82,  81,  81,  80,  79,  78,  77,  75,  73,  71,  68,  63,  58,  52,  45,  38,  32,  27,  23,  19,  17,  15,  13,  12,  11,  10,   9,   9,   8,   7,   7,   7,   6,   6,   6 },
  {  86,  86,  85,  85,  85,  84,  84,  84,  83,  82,  82,  81,  80,  78,  77,  75,  72,  68,  62,  54,  45,  36,  28,  23,  18,  15,  13,  12,  10,   9,   8,   8,   7,   6,   6,   6,   5,   5,   5,   4,   4 },
  {  87,  87,  87,  87,  86,  86,  86,  86,  85,  85,  84,  84,  83,  82,  81,  79,  77,  73,  68,  58,  45,  32,  23,  17,  13,  11,   9,   8,   7,   6,   6,   5,   5,   4,   4,   4,   4,   3,   3,   3,   3 },
  {  89,  88,  88,  88,  88,  88,  88,  88,  88,  87,  87,  87,  86,  86,  85,  84,  83,  81,  77,  68,  45,  23,  13,   9,   7,   6,   5,   4,   4,   3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,   1 },
  {  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  {  91,  92,  92,  92,  92,  92,  92,  92,  92,  93,  93,  93,  94,  94,  95,  96,  97,  99, 103, 113, 135, 158, 167, 171, 173, 174, 175, 176, 176, 177, 177, 177, 178, 178, 178, 178, 178, 178, 178, 178, 179 },
  {  93,  93,  93,  93,  94,  94,  94,  94,  95,  95,  96,  96,  97,  98,  99, 101, 103, 107, 113, 122, 135, 148, 158, 163, 167, 169, 171, 172, 173, 174, 174, 175, 175, 176, 176, 176, 176, 177, 177, 177, 177 },
  {  94,  94,  95,  95,  95,  96,  96,  96,  97,  98,  98,  99, 100, 102, 103, 105, 108, 113, 118, 126, 135, 144, 152, 158, 162, 165, 167, 168, 170, 171, 172, 172, 173, 174, 174, 174, 175, 175, 175, 176, 176 },
  {  96,  96,  96,  97,  97,  97,  98,  99,  99, 100, 101, 102, 103, 105, 107, 109, 113, 117, 122, 128, 135, 142, 148, 153, 158, 161, 163, 165, 167, 168, 169, 170, 171, 171, 172, 173, 173, 173, 174, 174, 174 },
  {  97,  97,  98,  98,  99,  99, 100, 101, 101, 102, 103, 105, 106, 108, 110, 113, 116, 120, 124, 129, 135, 141, 146, 150, 154, 158, 160, 162, 164, 165, 167, 168, 169, 169, 170, 171, 171, 172, 172, 173, 173 },
  {  98,  99,  99, 100, 100, 101, 102, 102, 103, 104, 105, 107, 108, 110, 113, 115, 118, 122, 126, 130, 135, 140, 144, 148, 152, 155, 158, 160, 162, 163, 165, 166, 167, 168, 168, 169, 170, 170, 171, 171, 172 },
  { 100, 100, 101, 101, 102, 103, 103, 104, 105, 106, 107, 109, 111, 113, 115, 117, 120, 123, 127, 131, 135, 139, 143, 147, 150, 153, 155, 158, 159, 161, 163, 164, 165, 166, 167, 167, 168, 169, 169, 170, 170 },
  { 101, 101, 102, 103, 103, 104, 105, 106, 107, 108, 109, 111, 113, 114, 117, 119, 122, 125, 128, 131, 135, 139, 142, 145, 148, 151, 153, 156, 158, 159, 161, 162, 163, 164, 165, 166, 167, 167, 168, 169, 169 },
  { 102, 103, 103, 104, 105, 105, 106, 107, 108, 110, 111, 113, 114, 116, 118, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 152, 154, 156, 158, 159, 160, 162, 163, 164, 165, 165, 166, 167, 167, 168 },
  { 103, 104, 105, 105, 106, 107, 108, 109, 110, 111, 113, 114, 116, 118, 120, 122, 124, 127, 129, 132, 135, 138, 141, 143, 146, 148, 150, 152, 154, 156, 158, 159, 160, 161, 162, 163, 164, 165, 165, 166, 167 },
  { 104, 105, 106, 106, 107, 108, 109, 110, 111, 113, 114, 115, 117, 119, 121, 123, 125, 127, 130, 132, 135, 138, 140, 143, 145, 147, 149, 151, 153, 155, 156, 158, 159, 160, 161, 162, 163, 164, 164, 165, 166 },
  { 105, 106, 107, 108, 108, 109, 110, 111, 113, 114, 115, 117, 118, 120, 122, 124, 126, 128, 130, 133, 135, 137, 140, 142, 144, 146, 148, 150, 152, 153, 155, 156, 158, 159, 160, 161, 162, 162, 163, 164, 165 },
  { 107, 107, 108, 109, 110, 110, 111, 113, 114, 115, 116, 118, 119, 121, 123, 124, 126, 129, 131, 133, 135, 137, 139, 141, 144, 146, 147, 149, 151, 152, 154, 155, 156, 158, 159, 160, 160, 161, 162, 163, 163 },
  { 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 119, 120, 122, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 148, 150, 151, 153, 154, 155, 156, 158, 159, 159, 160, 161, 162, 163 },
  { 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 120, 121, 122, 124, 126, 128, 129, 131, 133, 135, 137, 139, 141, 142, 144, 146, 148, 149, 150, 152, 153, 154, 155, 157, 158, 159, 159, 160, 161, 162 },
  { 109, 110, 111, 112, 113, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 159, 159, 160, 161 },
  { 110, 111, 112, 113, 114, 114, 115, 116, 117, 119, 120, 121, 122, 124, 125, 127, 128, 130, 132, 133, 135, 137, 138, 140, 142, 143, 145, 146, 148, 149, 150, 151, 153, 154, 155, 156, 157, 158, 159, 159, 160 },
  { 111, 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 122, 123, 124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 141, 143, 144, 146, 147, 148, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 159 },
  { 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 121, 122, 124, 125, 126, 128, 129, 131, 132, 133, 135, 137, 138, 139, 141, 142, 144, 145, 146, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159 },
  { 113, 114, 114, 115, 116, 117, 118, 118, 120, 121, 122, 123, 124, 125, 127, 128, 129, 131, 132, 134, 135, 136, 138, 139, 141, 142, 143, 145, 146, 147, 148, 149, 150, 152, 152, 153, 154, 155, 156, 157, 158 }
};


/*
 * Used to convert (x, y) into an array index (i) in a chunk of width w.
 *
 * y, x co-ordinates
 * w area width
 *
 * Returns the grid index.
 */
int yx_to_i(int y, int x, int w)
{
    return y * w + x;
}


/*
 * Used to convert an array index (i) into (x, y) in a chunk of width w.
 *
 * i grid index
 * w area width
 * y, x co-ordinates
 */
void i_to_yx(int i, int w, int *y, int *x)
{
    *y = i / w;
    *x = i % w;
}


/*
 * Shuffle an array using Knuth's shuffle.
 *
 * arr array
 * n number of shuffle moves
 */
void shuffle(int *arr, int n)
{
    int i, j, k;

    for (i = 0; i < n; i++)
    {
        j = randint0(n - i) + i;
        k = arr[j];
        arr[j] = arr[i];
        arr[i] = k;
    }
}


/*
 * Locate a square in y1 <= y < y2, x1 <= x < x2 which satisfies the given
 * predicate.
 *
 * c current chunk
 * y found y co-ordinate
 * y1, y2 y-range
 * x found x co-ordinate
 * x1, x2 x-range
 * pred square_predicate specifying what we're looking for
 *
 * Returns success
 */
static bool _find_in_range(struct chunk *c, int *y, int y1, int y2, int *x, int x1, int x2,
    square_predicate pred)
{
    int yd = y2 - y1;
    int xd = x2 - x1;
    int i, n = yd * xd;
    bool found = false;

    /* Allocate the squares, and randomize their order */
    int *squares = mem_alloc(n * sizeof(int));

    for (i = 0; i < n; i++) squares[i] = i;

    /* Test each square in (random) order for openness */
    for (i = 0; i < n && !found; i++)
    {
        int j = randint0(n - i) + i;
        int k = squares[j];

        squares[j] = squares[i];
        squares[i] = k;

        *y = (k / xd) + y1;
        *x = (k % xd) + x1;
        if (pred(c, *y, *x)) found = true;
    }

    mem_free(squares);

    /* Return whether we found an empty square or not. */
    return found;
}


/*
 * Locate a square in the dungeon which satisfies the given predicate.
 *
 * c current chunk
 * y found y co-ordinate
 * x found x co-ordinate
 * pred square_predicate specifying what we're looking for
 *
 * Returns success
 */
static bool square_find(struct chunk *c, int *y, int *x, square_predicate pred)
{
    return _find_in_range(c, y, 0, c->height, x, 0, c->width, pred);
}


/*
 * Locate a square in y1 <= y < y2, x1 <= x < x2 which satisfies the given
 * predicate.
 *
 * c current chunk
 * y found y co-ordinate
 * y1, y2 y-range
 * x found x co-ordinate
 * x1, x2 x-range
 * pred square_predicate specifying what we're looking for
 *
 * Returns success
 */
bool square_find_in_range(struct chunk *c, int *y, int y1, int y2, int *x, int x1, int x2,
    square_predicate pred)
{
    return _find_in_range(c, y, y1, y2, x, x1, x2, pred);
}


/*
 * Locate an empty square for 0 <= y < ymax, 0 <= x < xmax.
 *
 * c current chunk
 * y found y co-ordinate
 * x found x co-ordinate
 *
 * Returns success
 */
bool find_empty(struct chunk *c, int *y, int *x)
{
    return square_find(c, y, x, square_isempty);
}


/*
 * Locate a grid nearby (y0, x0) within +/- yd, xd.
 *
 * c current chunk
 * y found y co-ordinate
 * y0 starting y co-ordinate
 * yd y-range
 * x found x co-ordinate
 * x0 starting x co-ordinate
 * xd x-range
 *
 * Returns success
 */
bool find_nearby_grid(struct chunk *c, int *y, int y0, int yd, int *x, int x0, int xd)
{
    int y1 = y0 - yd;
    int x1 = x0 - xd;
    int y2 = y0 + yd + 1;
    int x2 = x0 + xd + 1;

    return square_find_in_range(c, y, y1, y2, x, x1, x2, square_in_bounds);
}


/*
 * Given two points, pick a valid cardinal direction from one to the other.
 *
 * rdir found row change (up or down)
 * cdir found column change (left or right)
 * y1, x1 starting co-ordinates
 * y2, x2 target co-ordinates
 */
void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2, int x2)
{
    /* Extract vertical and horizontal directions */
    *rdir = CMP(y2, y1);
    *cdir = CMP(x2, x1);

    /* If we only have one direction to go, then we're done */
    if (!*rdir || !*cdir) return;

    /* If we need to go diagonally, then choose a random direction */
    if (magik(50))
        *rdir = 0;
    else
        *cdir = 0;
}


/*
 * Pick a random cardinal direction.
 *
 * rdir, cdir direction co-ordinates
 */
void rand_dir(int *rdir, int *cdir)
{
    /* Pick a random direction and extract the dy/dx components */
    int i = randint0(4);

    *rdir = ddy_ddd[i];
    *cdir = ddx_ddd[i];
}


/*
 * Determine whether the given coordinate is a valid starting location.
 *
 * c current chunk
 * y, x co-ordinates
 *
 * Returns success
 */
static bool square_isstart(struct chunk *c, int y, int x)
{
    if (!square_isempty(c, y, x)) return false;
    if (square_isvault(c, y, x)) return false;
    return true;
}


/*
 * Pick random co-ordinates
 */
static void rand_xy(struct chunk *c, int *py, int *px)
{
    /* Try to find a good place to put the player */
    square_find_in_range(c, py, 0, c->height, px, 0, c->width, square_isstart);
}


/*
 * Place the player at a random starting location.
 *
 * c current chunk
 * p the player
 */
void new_player_spot(struct chunk *c, struct player *p)
{
    int y, x;

    /* Place the player */
    rand_xy(c, &y, &x);

    /* Save the new grid */
    c->level_rand_y = y;
    c->level_rand_x = x;

    /* Disconnected stairs */
    if (cfg_limit_stairs)
    {
        /* Set this to be the starting location for people going down */
        rand_xy(c, &y, &x);
        c->level_down_y = y;
        c->level_down_x = x;

        /* Set this to be the starting location for people going up */
        rand_xy(c, &y, &x);
        c->level_up_y = y;
        c->level_up_x = x;
    }

    /* Hack -- stay in bounds (to avoid asserts during cave generation) */
    p->py = MIN(MAX(p->py, 1), c->height - 2);
    p->px = MIN(MAX(p->px, 1), c->width - 2);
}


/*
 * Return how many cardinal directions around (x, y) contain walls.
 *
 * c current chunk
 * y, x co-ordinates
 *
 * Returns the number of walls
 */
static int next_to_walls(struct chunk *c, int y, int x)
{
    int k = 0;

    my_assert(square_in_bounds(c, y, x));

    if (square_iswall(c, y + 1, x)) k++;
    if (square_iswall(c, y - 1, x)) k++;
    if (square_iswall(c, y, x + 1)) k++;
    if (square_iswall(c, y, x - 1)) k++;

    return (k);
}


/*
 * Place rubble at (x, y).
 *
 * c current chunk
 * y, x co-ordinates
 */
static void place_rubble(struct chunk *c, int y, int x)
{
    int i, j;

    /* Create rubble */
    square_set_feat(c, y, x, FEAT_RUBBLE);

    for (j = -1; j < 2; j++)
    {
        for (i = -1; i < 2; i++)
        {
            /* Skip corners */
            if (abs(i + j) != 1) continue;
            
            /* Check Bounds */
            if (!square_in_bounds_fully(c, y + j, x + i)) continue;

            /* Totally useless AKA Require a certain number of adjacent walls */
            if (next_to_walls(c, y + j, x + i) < 2) continue;
            
            /* Require wall grid */
            if (square_isempty(c, y + j, x + i)) continue;

            /* Require an empty grid on the opposite side */
            if (!square_isempty(c, y - j, x - i)) continue;
            
            /* Place on the opposite side */
            square_set_feat(c, y - j, x - i, FEAT_RUBBLE);
            
            /* Done */
            return;     
        }
    }
}


/*
 * Convert existing terrain type to fountain
 */
static void place_fountain(struct chunk *c, int y, int x)
{
    int feat;

    /* 25% chance of being dried out */
    if (magik(75)) feat = FEAT_FOUNTAIN;
    else feat = FEAT_FNT_DRIED;
    
    /* Create fountain */
    square_set_feat(c, y, x, feat);
}


/*
 * Place stairs (of the requested type 'feat' if allowed) at (x, y).
 *
 * c current chunk
 * y, x co-ordinates
 * feat requested feature
 *
 * All stairs from town go down. All stairs from bottom go up.
 */
static void place_stairs(struct chunk *c, int y, int x, int feat)
{
    if (!c->depth)
        square_set_feat(c, y, x, FEAT_MORE);
    else if (c->depth == z_info->max_depth - 1)
    {
        /* Clear previous contents, add up stairs */
        if (cfg_limit_stairs < 2) square_set_feat(c, y, x, FEAT_LESS);

        /* Set this to be the starting location for people going down */
        c->level_down_y = y;
        c->level_down_x = x;
    }
    else
    {
        if (feat == FEAT_LESS)
        {
            /* Clear previous contents, add up stairs */
            if (cfg_limit_stairs < 2) square_set_feat(c, y, x, FEAT_LESS);

            /* Set this to be the starting location for people going down */
            c->level_down_y = y;
            c->level_down_x = x;
        }
        if (feat == FEAT_MORE)
        {
            /* Clear previous contents, add down stairs */
            square_set_feat(c, y, x, FEAT_MORE);

            /* Set this to be the starting location for people going up */
            c->level_up_y = y;
            c->level_up_x = x;
        }
    }
}


/*
 * Place random stairs at (x, y).
 *
 * c current chunk
 * y, x co-ordinates
 */
void place_random_stairs(struct chunk *c, int y, int x)
{
    int feat = (magik(50)? FEAT_LESS: FEAT_MORE);

    if (square_canputitem(c, y, x)) place_stairs(c, y, x, feat);
}


/*
 * Place a random object at (x, y).
 *
 * p the player
 * c current chunk
 * y, x co-ordinates
 * level generation depth
 * good is it a good object?
 * great is it a great object?
 * origin item origin
 * tval specified tval, if any
 */
void place_object(struct player *p, struct chunk *c, int y, int x, int level, bool good,
    bool great, byte origin, int tval)
{
    s32b rating = 0;
    struct object *new_obj;

    my_assert(square_in_bounds(c, y, x));

    if (!square_canputitem(c, y, x)) return;

    new_obj = make_object(p, c, level, good, great, false, &rating, tval);
    if (!new_obj) return;

    set_origin(new_obj, origin, c->depth, 0);

    /* Give it to the floor */
    if (!floor_carry(p, c, y, x, new_obj, false))
    {
        object_delete(&new_obj);
        return;
    }

    if (new_obj->artifact) c->good_item = true;

    /* Avoid overflows */
    if (rating > 2500000) rating = 2500000;

    c->obj_rating += (rating / 100) * (rating / 100);
}


/*
 * Place a random amount of gold at (x, y).
 *
 * p the player
 * c current chunk
 * y, x co-ordinates
 * level generation depth
 * origin item origin
 */
void place_gold(struct player *p, struct chunk *c, int y, int x, int level, byte origin)
{
    struct object *money = NULL;

    my_assert(square_in_bounds(c, y, x));

    if (!square_canputitem(c, y, x)) return;

    /* Make some gold */
    money = make_gold(p, level, "any");

    set_origin(money, origin, c->depth, 0);

    /* Give it to the floor */
    if (!floor_carry(NULL, c, y, x, money, false))
        object_delete(&money);
}


/*
 * Place a secret door at (x, y).
 *
 * c current chunk
 * y, x co-ordinates
 */
void place_secret_door(struct chunk *c, int y, int x)
{
    square_set_feat(c, y, x, FEAT_SECRET);
}


/*
 * Place a closed door at (x, y).
 *
 * c current chunk
 * y, x co-ordinates
 */
void place_closed_door(struct chunk *c, int y, int x)
{
    square_close_door(c, y, x);
    if (one_in_(4))
        square_set_door_lock(c, y, x, randint1(7));
}


/*
 * Place a random door at (x, y).
 *
 * c current chunk
 * y, x co-ordinates
 *
 * The door generated could be closed, open, broken, or secret.
 */
void place_random_door(struct chunk *c, int y, int x)
{
    int tmp = randint0(100);

    /* Create open door */
    if (tmp < 30) square_open_door(c, y, x);

    /* Create broken door */
    else if (tmp < 40) square_smash_door(c, y, x);

    /* Create secret door */
    else if (tmp < 60) square_set_feat(c, y, x, FEAT_SECRET);

    /* Create closed door */
    else place_closed_door(c, y, x);
}


/*
 * Place some staircases near walls
 *
 * c current chunk
 * feat the stair terrain type
 * num number of staircases to place
 * walls number of walls to surround the stairs (negotiable)
 */
void alloc_stairs(struct chunk *c, int feat, int num, int walls)
{
    int y, x, i, j, done;

    /* Place "num" stairs */
    for (i = 0; i < num; i++)
    {
        /* Place some stairs */
        for (done = false; !done; )
        {
            /* Try several times, then decrease "walls" */
            for (j = 0; !done && j <= 1000; j++)
            {
                find_empty(c, &y, &x);

                /* Require a certain number of adjacent walls */
                if (next_to_walls(c, y, x) < walls) continue;

                place_stairs(c, y, x, feat);

                /* All done */
                done = true;
            }

            /* Require fewer walls */
            if (walls) walls--;
        }
    }
}


/*
 * Allocates 'num' random objects in the dungeon.
 *
 * p the player
 * c current chunk
 * set where the entity is placed (corridor, room or either)
 * typ what is placed (rubble, trap, gold, item)
 * num number to place
 * depth generation depth
 * origin item origin (if appropriate)
 *
 * See alloc_object() for more information.
 */
void alloc_objects(struct player *p, struct chunk *c, int set, int typ, int num, int depth,
    byte origin)
{
    int k, l = 0;

    for (k = 0; k < num; k++)
    {
         bool ok = alloc_object(p, c, set, typ, depth, origin);

         if (!ok) l++;
    }
}


/*
 * Allocates a single random object in the dungeon.
 *
 * p the player
 * c current chunk
 * set where the entity is placed (corridor, room or either)
 * typ what is placed (rubble, trap, gold, item)
 * depth generation depth
 * origin item origin (if appropriate)
 *
 * 'set' controls where the object is placed (corridor, room, either).
 * 'typ' controls the kind of object (rubble, trap, gold, item).
 */
bool alloc_object(struct player *p, struct chunk *c, int set, int typ, int depth,
    byte origin)
{
    int x = 0, y = 0;
    int tries = 0;

    /* Pick a "legal" spot */
    while (tries < 2000)
    {
        tries++;

        find_empty(c, &y, &x);

        /* If we are ok with a corridor and we're in one, we're done */
        if (set & SET_CORR && !square_isroom(c, y, x)) break;

        /* If we are ok with a room and we're in one, we're done */
        if (set & SET_ROOM && square_isroom(c, y, x)) break;
    }

    if (tries == 2000) return false;

    /* Place something */
    switch (typ)
    {
        case TYP_RUBBLE: place_rubble(c, y, x); break;
        case TYP_FOUNTAIN: place_fountain(c, y, x); break;
        case TYP_TRAP: square_add_trap(c, y, x); break;
        case TYP_GOLD:
        {
            place_gold(p, c, y, x, depth, origin);
            break;
        }
        case TYP_OBJECT:
        {
            place_object(p, c, y, x, depth, false, false, origin, 0);
            break;
        }
        case TYP_GOOD:
        {
            place_object(p, c, y, x, depth, true, false, origin, 0);
            break;
        }
        case TYP_GREAT:
        {
            place_object(p, c, y, x, depth, true, true, origin, 0);
            break;
        }
    }

    return true;
}


/*
 * Create up to 'num' objects near the given coordinates in a vault.
 *
 * p the player
 * c current chunk
 * y, x co-ordinates
 * num number of objects
 */
void vault_objects(struct player *p, struct chunk *c, int y, int x, int num)
{
    int i, j, k;

    /* Attempt to place 'num' objects */
    for (; num > 0; --num)
    {
        /* Try up to 11 spots looking for empty space */
        for (i = 0; i < 11; ++i)
        {
            /* Pick a random location */
            find_nearby_grid(c, &j, y, 2, &k, x, 3);

            /* Require "clean" floor space */
            if (!square_canputitem(c, j, k)) continue;

            /* Place an item or gold */
            if (magik(75))
                place_object(p, c, j, k, object_level(c->depth), false, false, ORIGIN_SPECIAL, 0);
            else
                place_gold(p, c, j, k, object_level(c->depth), ORIGIN_SPECIAL);

            /* Placement accomplished */
            break;
        }
    }
}


/*
 * Place a trap near (x, y), with a given displacement.
 *
 * c current chunk
 * y, x co-ordinates to place the trap near
 * yd, xd how far afield to look for a place
 */
static void vault_trap_aux(struct chunk *c, int y, int x, int yd, int xd)
{
    int tries, y1, x1;

    /* Find a nearby empty grid and place a trap */
    for (tries = 0; tries <= 5; tries++)
    {
        find_nearby_grid(c, &y1, y, yd, &x1, x, xd);
        if (!square_isempty(c, y1, x1)) continue;

        square_add_trap(c, y1, x1);

        break;
    }
}


/*
 * Place 'num' traps near (x, y), with a given displacement.
 *
 * c current chunk
 * y, x co-ordinates to place the trap near
 * yd, xd how far afield to look for a place
 * num number of traps to place
 */
void vault_traps(struct chunk *c, int y, int x, int yd, int xd, int num)
{
    int i;

    for (i = 0; i < num; i++)
        vault_trap_aux(c, y, x, yd, xd);
}


/*
 * Place 'num' sleeping monsters near (x, y).
 *
 * p the player
 * c current chunk
 * y1, x1 co-ordinates to place the monsters near
 * depth generation depth
 * num number of monsters to place
 */
void vault_monsters(struct player *p, struct chunk *c, int y1, int x1, int depth, int num)
{
    int k, i, y, x;

    /* Try to summon "num" monsters "near" the given location */
    for (k = 0; k < num; k++)
    {
        /* Try nine locations */
        for (i = 0; i < 9; i++)
        {
            int d = 1;

            /* Pick a nearby location */
            scatter(c, &y, &x, y1, x1, d, true);
            if (!square_in_bounds_fully(c, y, x)) continue;

            /* Require "empty" floor grids */
            if (!square_isemptyfloor(c, y, x)) continue;

            /* Place the monster (allow groups) */
            pick_and_place_monster(p, c, y, x, depth, MON_SLEEP | MON_GROUP, ORIGIN_DROP_SPECIAL);

            break;
        }
    }
}