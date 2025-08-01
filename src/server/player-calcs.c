/*
 * File: player-calcs.c
 * Purpose: Player status calculation, signalling ui events based on status changes.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2014 Nick McConnell
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
 * Stat Table (INT) -- magic devices
 */
static const int adj_int_dev[STAT_RANGE] =
{
    0   /* 3 */,
    0   /* 4 */,
    0   /* 5 */,
    0   /* 6 */,
    0   /* 7 */,
    1   /* 8 */,
    1   /* 9 */,
    1   /* 10 */,
    1   /* 11 */,
    1   /* 12 */,
    1   /* 13 */,
    1   /* 14 */,
    2   /* 15 */,
    2   /* 16 */,
    2   /* 17 */,
    3   /* 18/00-18/09 */,
    3   /* 18/10-18/19 */,
    3   /* 18/20-18/29 */,
    3   /* 18/30-18/39 */,
    3   /* 18/40-18/49 */,
    4   /* 18/50-18/59 */,
    4   /* 18/60-18/69 */,
    5   /* 18/70-18/79 */,
    5   /* 18/80-18/89 */,
    6   /* 18/90-18/99 */,
    6   /* 18/100-18/109 */,
    7   /* 18/110-18/119 */,
    7   /* 18/120-18/129 */,
    8   /* 18/130-18/139 */,
    8   /* 18/140-18/149 */,
    9   /* 18/150-18/159 */,
    9   /* 18/160-18/169 */,
    10  /* 18/170-18/179 */,
    10  /* 18/180-18/189 */,
    11  /* 18/190-18/199 */,
    11  /* 18/200-18/209 */,
    12  /* 18/210-18/219 */,
    13  /* 18/220+ */
};


/*
 * Stat Table (WIS) -- saving throw
 */
static const int adj_wis_sav[STAT_RANGE] =
{
    0   /* 3 */,
    0   /* 4 */,
    0   /* 5 */,
    0   /* 6 */,
    0   /* 7 */,
    1   /* 8 */,
    1   /* 9 */,
    1   /* 10 */,
    1   /* 11 */,
    1   /* 12 */,
    1   /* 13 */,
    1   /* 14 */,
    2   /* 15 */,
    2   /* 16 */,
    2   /* 17 */,
    3   /* 18/00-18/09 */,
    3   /* 18/10-18/19 */,
    3   /* 18/20-18/29 */,
    3   /* 18/30-18/39 */,
    3   /* 18/40-18/49 */,
    4   /* 18/50-18/59 */,
    4   /* 18/60-18/69 */,
    5   /* 18/70-18/79 */,
    5   /* 18/80-18/89 */,
    6   /* 18/90-18/99 */,
    7   /* 18/100-18/109 */,
    8   /* 18/110-18/119 */,
    9   /* 18/120-18/129 */,
    10  /* 18/130-18/139 */,
    11  /* 18/140-18/149 */,
    12  /* 18/150-18/159 */,
    13  /* 18/160-18/169 */,
    14  /* 18/170-18/179 */,
    15  /* 18/180-18/189 */,
    16  /* 18/190-18/199 */,
    17  /* 18/200-18/209 */,
    18  /* 18/210-18/219 */,
    19  /* 18/220+ */
};


/*
 * Stat Table (DEX) -- disarming
 */
static const int adj_dex_dis[STAT_RANGE] =
{
    0   /* 3 */,
    0   /* 4 */,
    0   /* 5 */,
    0   /* 6 */,
    0   /* 7 */,
    1   /* 8 */,
    1   /* 9 */,
    1   /* 10 */,
    1   /* 11 */,
    1   /* 12 */,
    2   /* 13 */,
    2   /* 14 */,
    3   /* 15 */,
    4   /* 16 */,
    4   /* 17 */,
    7   /* 18/00-18/09 */,
    7   /* 18/10-18/19 */,
    7   /* 18/20-18/29 */,
    8   /* 18/30-18/39 */,
    9   /* 18/40-18/49 */,
    10  /* 18/50-18/59 */,
    11  /* 18/60-18/69 */,
    13  /* 18/70-18/79 */,
    14  /* 18/80-18/89 */,
    16  /* 18/90-18/99 */,
    18  /* 18/100-18/109 */,
    18  /* 18/110-18/119 */,
    19  /* 18/120-18/129 */,
    20  /* 18/130-18/139 */,
    21  /* 18/140-18/149 */,
    23  /* 18/150-18/159 */,
    24  /* 18/160-18/169 */,
    25  /* 18/170-18/179 */,
    26  /* 18/180-18/189 */,
    27  /* 18/190-18/199 */,
    29  /* 18/200-18/209 */,
    29  /* 18/210-18/219 */,
    29  /* 18/220+ */
};


/*
 * Stat Table (INT) -- disarming
 */
static const int adj_int_dis[STAT_RANGE] =
{
    0   /* 3 */,
    0   /* 4 */,
    0   /* 5 */,
    0   /* 6 */,
    0   /* 7 */,
    1   /* 8 */,
    1   /* 9 */,
    1   /* 10 */,
    1   /* 11 */,
    1   /* 12 */,
    2   /* 13 */,
    2   /* 14 */,
    3   /* 15 */,
    4   /* 16 */,
    4   /* 17 */,
    7   /* 18/00-18/09 */,
    7   /* 18/10-18/19 */,
    7   /* 18/20-18/29 */,
    8   /* 18/30-18/39 */,
    9   /* 18/40-18/49 */,
    10  /* 18/50-18/59 */,
    11  /* 18/60-18/69 */,
    13  /* 18/70-18/79 */,
    14  /* 18/80-18/89 */,
    16  /* 18/90-18/99 */,
    18  /* 18/100-18/109 */,
    18  /* 18/110-18/119 */,
    19  /* 18/120-18/129 */,
    20  /* 18/130-18/139 */,
    21  /* 18/140-18/149 */,
    23  /* 18/150-18/159 */,
    24  /* 18/160-18/169 */,
    25  /* 18/170-18/179 */,
    26  /* 18/180-18/189 */,
    27  /* 18/190-18/199 */,
    29  /* 18/200-18/209 */,
    29  /* 18/210-18/219 */,
    29  /* 18/220+ */
};


/*
 * Stat Table (DEX) -- bonus to ac
 */
static const int adj_dex_ta[STAT_RANGE] =
{
    -4  /* 3 */,
    -3  /* 4 */,
    -2  /* 5 */,
    -1  /* 6 */,
    0   /* 7 */,
    0   /* 8 */,
    0   /* 9 */,
    0   /* 10 */,
    0   /* 11 */,
    0   /* 12 */,
    0   /* 13 */,
    0   /* 14 */,
    1   /* 15 */,
    1   /* 16 */,
    1   /* 17 */,
    2   /* 18/00-18/09 */,
    2   /* 18/10-18/19 */,
    2   /* 18/20-18/29 */,
    2   /* 18/30-18/39 */,
    2   /* 18/40-18/49 */,
    3   /* 18/50-18/59 */,
    3   /* 18/60-18/69 */,
    3   /* 18/70-18/79 */,
    4   /* 18/80-18/89 */,
    5   /* 18/90-18/99 */,
    6   /* 18/100-18/109 */,
    7   /* 18/110-18/119 */,
    8   /* 18/120-18/129 */,
    9   /* 18/130-18/139 */,
    9   /* 18/140-18/149 */,
    10  /* 18/150-18/159 */,
    11  /* 18/160-18/169 */,
    12  /* 18/170-18/179 */,
    13  /* 18/180-18/189 */,
    14  /* 18/190-18/199 */,
    15  /* 18/200-18/209 */,
    15  /* 18/210-18/219 */,
    15  /* 18/220+ */
};


/*
 * Stat Table (STR) -- bonus to dam
 */
const int adj_str_td[STAT_RANGE] =
{
    -2  /* 3 */,
    -2  /* 4 */,
    -1  /* 5 */,
    -1  /* 6 */,
    0   /* 7 */,
    0   /* 8 */,
    0   /* 9 */,
    0   /* 10 */,
    0   /* 11 */,
    0   /* 12 */,
    0   /* 13 */,
    0   /* 14 */,
    0   /* 15 */,
    1   /* 16 */,
    2   /* 17 */,
    2   /* 18/00-18/09 */,
    2   /* 18/10-18/19 */,
    3   /* 18/20-18/29 */,
    3   /* 18/30-18/39 */,
    3   /* 18/40-18/49 */,
    3   /* 18/50-18/59 */,
    3   /* 18/60-18/69 */,
    4   /* 18/70-18/79 */,
    5   /* 18/80-18/89 */,
    5   /* 18/90-18/99 */,
    6   /* 18/100-18/109 */,
    7   /* 18/110-18/119 */,
    8   /* 18/120-18/129 */,
    9   /* 18/130-18/139 */,
    10  /* 18/140-18/149 */,
    11  /* 18/150-18/159 */,
    12  /* 18/160-18/169 */,
    13  /* 18/170-18/179 */,
    14  /* 18/180-18/189 */,
    15  /* 18/190-18/199 */,
    16  /* 18/200-18/209 */,
    18  /* 18/210-18/219 */,
    20  /* 18/220+ */
};


/*
 * Stat Table (DEX) -- bonus to hit
 */
const int adj_dex_th[STAT_RANGE] =
{
    -3  /* 3 */,
    -2  /* 4 */,
    -2  /* 5 */,
    -1  /* 6 */,
    -1  /* 7 */,
    0   /* 8 */,
    0   /* 9 */,
    0   /* 10 */,
    0   /* 11 */,
    0   /* 12 */,
    0   /* 13 */,
    0   /* 14 */,
    0   /* 15 */,
    1   /* 16 */,
    2   /* 17 */,
    3   /* 18/00-18/09 */,
    3   /* 18/10-18/19 */,
    3   /* 18/20-18/29 */,
    3   /* 18/30-18/39 */,
    3   /* 18/40-18/49 */,
    4   /* 18/50-18/59 */,
    4   /* 18/60-18/69 */,
    4   /* 18/70-18/79 */,
    4   /* 18/80-18/89 */,
    5   /* 18/90-18/99 */,
    6   /* 18/100-18/109 */,
    7   /* 18/110-18/119 */,
    8   /* 18/120-18/129 */,
    9   /* 18/130-18/139 */,
    9   /* 18/140-18/149 */,
    10  /* 18/150-18/159 */,
    11  /* 18/160-18/169 */,
    12  /* 18/170-18/179 */,
    13  /* 18/180-18/189 */,
    14  /* 18/190-18/199 */,
    15  /* 18/200-18/209 */,
    15  /* 18/210-18/219 */,
    15  /* 18/220+ */
};


/*
 * Stat Table (STR) -- bonus to hit
 */
static const int adj_str_th[STAT_RANGE] =
{
    -3  /* 3 */,
    -2  /* 4 */,
    -1  /* 5 */,
    -1  /* 6 */,
    0   /* 7 */,
    0   /* 8 */,
    0   /* 9 */,
    0   /* 10 */,
    0   /* 11 */,
    0   /* 12 */,
    0   /* 13 */,
    0   /* 14 */,
    0   /* 15 */,
    0   /* 16 */,
    0   /* 17 */,
    1   /* 18/00-18/09 */,
    1   /* 18/10-18/19 */,
    1   /* 18/20-18/29 */,
    1   /* 18/30-18/39 */,
    1   /* 18/40-18/49 */,
    1   /* 18/50-18/59 */,
    1   /* 18/60-18/69 */,
    2   /* 18/70-18/79 */,
    3   /* 18/80-18/89 */,
    4   /* 18/90-18/99 */,
    5   /* 18/100-18/109 */,
    6   /* 18/110-18/119 */,
    7   /* 18/120-18/129 */,
    8   /* 18/130-18/139 */,
    9   /* 18/140-18/149 */,
    10  /* 18/150-18/159 */,
    11  /* 18/160-18/169 */,
    12  /* 18/170-18/179 */,
    13  /* 18/180-18/189 */,
    14  /* 18/190-18/199 */,
    15  /* 18/200-18/209 */,
    15  /* 18/210-18/219 */,
    15  /* 18/220+ */
};


/*
 * Stat Table (STR) -- weight limit in deca-pounds
 */
static const int adj_str_wgt[STAT_RANGE] =
{
    5   /* 3 */,
    6   /* 4 */,
    7   /* 5 */,
    8   /* 6 */,
    9   /* 7 */,
    10  /* 8 */,
    11  /* 9 */,
    12  /* 10 */,
    13  /* 11 */,
    14  /* 12 */,
    15  /* 13 */,
    16  /* 14 */,
    17  /* 15 */,
    18  /* 16 */,
    19  /* 17 */,
    20  /* 18/00-18/09 */,
    22  /* 18/10-18/19 */,
    24  /* 18/20-18/29 */,
    26  /* 18/30-18/39 */,
    28  /* 18/40-18/49 */,
    30  /* 18/50-18/59 */,
    30  /* 18/60-18/69 */,
    30  /* 18/70-18/79 */,
    30  /* 18/80-18/89 */,
    30  /* 18/90-18/99 */,
    30  /* 18/100-18/109 */,
    30  /* 18/110-18/119 */,
    30  /* 18/120-18/129 */,
    30  /* 18/130-18/139 */,
    30  /* 18/140-18/149 */,
    30  /* 18/150-18/159 */,
    30  /* 18/160-18/169 */,
    30  /* 18/170-18/179 */,
    30  /* 18/180-18/189 */,
    30  /* 18/190-18/199 */,
    30  /* 18/200-18/209 */,
    30  /* 18/210-18/219 */,
    30  /* 18/220+ */
};


/*
 * Stat Table (STR) -- weapon weight limit in pounds
 */
const int adj_str_hold[STAT_RANGE] =
{
    4   /* 3 */,
    5   /* 4 */,
    6   /* 5 */,
    7   /* 6 */,
    8   /* 7 */,
    10  /* 8 */,
    12  /* 9 */,
    14  /* 10 */,
    16  /* 11 */,
    18  /* 12 */,
    20  /* 13 */,
    22  /* 14 */,
    24  /* 15 */,
    26  /* 16 */,
    28  /* 17 */,
    30  /* 18/00-18/09 */,
    30  /* 18/10-18/19 */,
    35  /* 18/20-18/29 */,
    40  /* 18/30-18/39 */,
    45  /* 18/40-18/49 */,
    50  /* 18/50-18/59 */,
    55  /* 18/60-18/69 */,
    60  /* 18/70-18/79 */,
    65  /* 18/80-18/89 */,
    70  /* 18/90-18/99 */,
    80  /* 18/100-18/109 */,
    80  /* 18/110-18/119 */,
    80  /* 18/120-18/129 */,
    80  /* 18/130-18/139 */,
    80  /* 18/140-18/149 */,
    90  /* 18/150-18/159 */,
    90  /* 18/160-18/169 */,
    90  /* 18/170-18/179 */,
    90  /* 18/180-18/189 */,
    90  /* 18/190-18/199 */,
    100 /* 18/200-18/209 */,
    100 /* 18/210-18/219 */,
    100 /* 18/220+ */
};


/*
 * Stat Table (STR) -- digging value
 */
static const int adj_str_dig[STAT_RANGE] =
{
    0   /* 3 */,
    0   /* 4 */,
    1   /* 5 */,
    2   /* 6 */,
    3   /* 7 */,
    4   /* 8 */,
    4   /* 9 */,
    5   /* 10 */,
    5   /* 11 */,
    6   /* 12 */,
    6   /* 13 */,
    7   /* 14 */,
    7   /* 15 */,
    8   /* 16 */,
    8   /* 17 */,
    9   /* 18/00-18/09 */,
    10  /* 18/10-18/19 */,
    12  /* 18/20-18/29 */,
    15  /* 18/30-18/39 */,
    20  /* 18/40-18/49 */,
    25  /* 18/50-18/59 */,
    30  /* 18/60-18/69 */,
    35  /* 18/70-18/79 */,
    40  /* 18/80-18/89 */,
    45  /* 18/90-18/99 */,
    50  /* 18/100-18/109 */,
    55  /* 18/110-18/119 */,
    60  /* 18/120-18/129 */,
    65  /* 18/130-18/139 */,
    70  /* 18/140-18/149 */,
    75  /* 18/150-18/159 */,
    80  /* 18/160-18/169 */,
    85  /* 18/170-18/179 */,
    90  /* 18/180-18/189 */,
    95  /* 18/190-18/199 */,
    100 /* 18/200-18/209 */,
    100 /* 18/210-18/219 */,
    100 /* 18/220+ */
};


/*
 * Stat Table (DEX) -- chance of avoiding "theft" and "falling"
 */
const int adj_dex_safe[STAT_RANGE] =
{
    0   /* 3 */,
    1   /* 4 */,
    2   /* 5 */,
    3   /* 6 */,
    4   /* 7 */,
    5   /* 8 */,
    5   /* 9 */,
    6   /* 10 */,
    6   /* 11 */,
    7   /* 12 */,
    7   /* 13 */,
    8   /* 14 */,
    8   /* 15 */,
    9   /* 16 */,
    9   /* 17 */,
    10  /* 18/00-18/09 */,
    10  /* 18/10-18/19 */,
    15  /* 18/20-18/29 */,
    15  /* 18/30-18/39 */,
    20  /* 18/40-18/49 */,
    25  /* 18/50-18/59 */,
    30  /* 18/60-18/69 */,
    35  /* 18/70-18/79 */,
    40  /* 18/80-18/89 */,
    45  /* 18/90-18/99 */,
    50  /* 18/100-18/109 */,
    60  /* 18/110-18/119 */,
    70  /* 18/120-18/129 */,
    80  /* 18/130-18/139 */,
    90  /* 18/140-18/149 */,
    100 /* 18/150-18/159 */,
    100 /* 18/160-18/169 */,
    100 /* 18/170-18/179 */,
    100 /* 18/180-18/189 */,
    100 /* 18/190-18/199 */,
    100 /* 18/200-18/209 */,
    100 /* 18/210-18/219 */,
    100 /* 18/220+ */
};


/*
 * Stat Table (CON) -- base regeneration rate
 */
const int adj_con_fix[STAT_RANGE] =
{
    0   /* 3 */,
    0   /* 4 */,
    0   /* 5 */,
    0   /* 6 */,
    0   /* 7 */,
    0   /* 8 */,
    0   /* 9 */,
    0   /* 10 */,
    0   /* 11 */,
    0   /* 12 */,
    0   /* 13 */,
    1   /* 14 */,
    1   /* 15 */,
    1   /* 16 */,
    1   /* 17 */,
    2   /* 18/00-18/09 */,
    2   /* 18/10-18/19 */,
    2   /* 18/20-18/29 */,
    2   /* 18/30-18/39 */,
    2   /* 18/40-18/49 */,
    3   /* 18/50-18/59 */,
    3   /* 18/60-18/69 */,
    3   /* 18/70-18/79 */,
    3   /* 18/80-18/89 */,
    3   /* 18/90-18/99 */,
    4   /* 18/100-18/109 */,
    4   /* 18/110-18/119 */,
    5   /* 18/120-18/129 */,
    6   /* 18/130-18/139 */,
    6   /* 18/140-18/149 */,
    7   /* 18/150-18/159 */,
    7   /* 18/160-18/169 */,
    8   /* 18/170-18/179 */,
    8   /* 18/180-18/189 */,
    8   /* 18/190-18/199 */,
    9   /* 18/200-18/209 */,
    9   /* 18/210-18/219 */,
    9   /* 18/220+ */
};


/*
 * Stat Table (CON) -- extra 1/100th hitpoints per level
 */
static const int adj_con_mhp[STAT_RANGE] =
{
    -250    /* 3 */,
    -150    /* 4 */,
    -100    /* 5 */,
     -75    /* 6 */,
     -50    /* 7 */,
     -25    /* 8 */,
     -10    /* 9 */,
      -5    /* 10 */,
       0    /* 11 */,
       5    /* 12 */,
      10    /* 13 */,
      25    /* 14 */,
      50    /* 15 */,
      75    /* 16 */,
     100    /* 17 */,
     150    /* 18/00-18/09 */,
     175    /* 18/10-18/19 */,
     200    /* 18/20-18/29 */,
     225    /* 18/30-18/39 */,
     250    /* 18/40-18/49 */,
     275    /* 18/50-18/59 */,
     300    /* 18/60-18/69 */,
     350    /* 18/70-18/79 */,
     400    /* 18/80-18/89 */,
     450    /* 18/90-18/99 */,
     500    /* 18/100-18/109 */,
     550    /* 18/110-18/119 */,
     600    /* 18/120-18/129 */,
     650    /* 18/130-18/139 */,
     700    /* 18/140-18/149 */,
     750    /* 18/150-18/159 */,
     800    /* 18/160-18/169 */,
     900    /* 18/170-18/179 */,
    1000    /* 18/180-18/189 */,
    1100    /* 18/190-18/199 */,
    1250    /* 18/200-18/209 */,
    1250    /* 18/210-18/219 */,
    1250    /* 18/220+ */
};


/*
 * Stat Table (INT/WIS) -- number of half-spells per level
 */
static const int adj_mag_study[] =
{
      0 /* 3 */,
      0 /* 4 */,
     10 /* 5 */,
     20 /* 6 */,
     30 /* 7 */,
     40 /* 8 */,
     50 /* 9 */,
     60 /* 10 */,
     70 /* 11 */,
     80 /* 12 */,
     85 /* 13 */,
     90 /* 14 */,
     95 /* 15 */,
    100 /* 16 */,
    105 /* 17 */,
    110 /* 18/00-18/09 */,
    115 /* 18/10-18/19 */,
    120 /* 18/20-18/29 */,
    130 /* 18/30-18/39 */,
    140 /* 18/40-18/49 */,
    150 /* 18/50-18/59 */,
    160 /* 18/60-18/69 */,
    170 /* 18/70-18/79 */,
    180 /* 18/80-18/89 */,
    190 /* 18/90-18/99 */,
    200 /* 18/100-18/109 */,
    210 /* 18/110-18/119 */,
    220 /* 18/120-18/129 */,
    230 /* 18/130-18/139 */,
    240 /* 18/140-18/149 */,
    250 /* 18/150-18/159 */,
    250 /* 18/160-18/169 */,
    250 /* 18/170-18/179 */,
    250 /* 18/180-18/189 */,
    250 /* 18/190-18/199 */,
    250 /* 18/200-18/209 */,
    250 /* 18/210-18/219 */,
    250 /* 18/220+ */
};


/*
 * Stat Table (INT/WIS) -- extra half-mana-points per level
 */
static const int adj_mag_mana[] =
{
      0 /* 3 */,
     10 /* 4 */,
     20 /* 5 */,
     30 /* 6 */,
     40 /* 7 */,
     50 /* 8 */,
     60 /* 9 */,
     70 /* 10 */,
     80 /* 11 */,
     90 /* 12 */,
    100 /* 13 */,
    110 /* 14 */,
    120 /* 15 */,
    130 /* 16 */,
    140 /* 17 */,
    150 /* 18/00-18/09 */,
    160 /* 18/10-18/19 */,
    170 /* 18/20-18/29 */,
    180 /* 18/30-18/39 */,
    190 /* 18/40-18/49 */,
    200 /* 18/50-18/59 */,
    225 /* 18/60-18/69 */,
    250 /* 18/70-18/79 */,
    300 /* 18/80-18/89 */,
    350 /* 18/90-18/99 */,
    400 /* 18/100-18/109 */,
    450 /* 18/110-18/119 */,
    500 /* 18/120-18/129 */,
    550 /* 18/130-18/139 */,
    600 /* 18/140-18/149 */,
    650 /* 18/150-18/159 */,
    700 /* 18/160-18/169 */,
    750 /* 18/170-18/179 */,
    800 /* 18/180-18/189 */,
    800 /* 18/190-18/199 */,
    800 /* 18/200-18/209 */,
    800 /* 18/210-18/219 */,
    800 /* 18/220+ */
};


/*
 * Average of the player's spell stats across all the realms they can cast
 * from, rounded up
 *
 * If the player can only cast from a single realm, this is simple the stat
 * for that realm
 */
static int average_spell_stat(struct player *p, struct player_state *state)
{
    int i, count = 0, sum = 0;
    char realm[120];
    struct class_book *book = &p->clazz->magic.books[0];

    my_strcpy(realm, book->realm->name, sizeof(realm));

    sum += state->stat_ind[book->realm->stat];
    count++;

    for (i = 1; i < p->clazz->magic.num_books; i++)
    {
        book = &p->clazz->magic.books[i];

        if (!strstr(realm, book->realm->name))
        {
            my_strcat(realm, "/", sizeof(realm));
            my_strcat(realm, book->realm->name, sizeof(realm));

            sum += state->stat_ind[book->realm->stat];
            count++;
        }
    }

    return (sum + count - 1) / count;
}


/*
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 *
 * Note that this function induces various "status" messages,
 * which must be bypassed until the character is created.
 */
static void calc_spells(struct player *p)
{
    int i, j, k, levels;
    int num_allowed, num_known, num_total = p->clazz->magic.total_spells, num_forgotten;
    int percent_spells;
    const struct class_spell *spell;
    int16_t old_spells;

    /* Must be literate */
    if (!p->clazz->magic.total_spells) return;

    /* Save the new_spells value */
    old_spells = p->upkeep->new_spells;

    /* Determine the number of spells allowed */
    levels = p->lev - p->clazz->magic.spell_first + 1;

    /* No negative spells */
    if (levels < 0) levels = 0;

    /* Number of 1/100 spells per level */
    percent_spells = adj_mag_study[average_spell_stat(p, &p->state)];

    /* Extract total allowed spells (rounded up) */
    num_allowed = (((percent_spells * levels) + 50) / 100);

    /* Assume none known */
    num_known = 0;
    num_forgotten = 0;

    /* Count the number of spells we know */
    for (j = 0; j < num_total; j++)
    {
        if (p->spell_flags[j] & PY_SPELL_LEARNED) num_known++;
        if (p->spell_flags[j] & PY_SPELL_FORGOTTEN) num_forgotten++;
    }

    /* See how many spells we must forget or may learn */
    p->upkeep->new_spells = num_allowed - num_known;

    /* Forget spells which are too hard */
    for (i = num_total - 1; i >= 0; i--)
    {
        /* Efficiency -- all done */
        if (num_known == 0) break;

        /* Access the spell */
        j = p->spell_order[i];

        /* Skip non-spells */
        if (j >= 99) continue;

        /* Get the spell */
        spell = spell_by_index(&p->clazz->magic, j);

        /* Skip spells we are allowed to know */
        if (spell->slevel <= p->lev) continue;

        /* Is it known? */
        if (p->spell_flags[j] & PY_SPELL_LEARNED)
        {
            /* Mark as forgotten */
            p->spell_flags[j] |= PY_SPELL_FORGOTTEN;

            /* No longer known */
            p->spell_flags[j] &= ~PY_SPELL_LEARNED;

            /* Message */
            msg(p, "You have forgotten the %s of %s.", spell->realm->spell_noun, spell->name);

            /* One more can be learned */
            p->upkeep->new_spells++;
            num_known--;
            num_forgotten++;
        }
    }

    /* Forget spells if we know too many spells */
    for (i = num_total - 1; i >= 0; i--)
    {
        /* Stop when possible */
        if (p->upkeep->new_spells >= 0) break;

        /* Efficiency -- all done */
        if (num_known == 0) break;

        /* Get the (i+1)th spell learned */
        j = p->spell_order[i];

        /* Skip unknown spells */
        if (j >= 99) continue;

        /* Get the spell */
        spell = spell_by_index(&p->clazz->magic, j);

        /* Forget it (if learned) */
        if (p->spell_flags[j] & PY_SPELL_LEARNED)
        {
            /* Mark as forgotten */
            p->spell_flags[j] |= PY_SPELL_FORGOTTEN;

            /* No longer known */
            p->spell_flags[j] &= ~PY_SPELL_LEARNED;

            /* Message */
            msg(p, "You have forgotten the %s of %s.", spell->realm->spell_noun, spell->name);

            /* One more can be learned */
            p->upkeep->new_spells++;
            num_known--;
            num_forgotten++;
        }
    }

    /* Check for spells to remember */
    for (i = 0; i < num_total; i++)
    {
        /* None left to remember */
        if (p->upkeep->new_spells <= 0) break;

        /* Efficiency -- all done */
        if (num_forgotten == 0) break;

        /* Get the next spell we learned */
        j = p->spell_order[i];

        /* Skip unknown spells */
        if (j >= 99) break;

        /* Access the spell */
        spell = spell_by_index(&p->clazz->magic, j);

        /* Skip spells we cannot remember */
        if (spell->slevel > p->lev) continue;

        /* First set of spells */
        if (p->spell_flags[j] & PY_SPELL_FORGOTTEN)
        {
            /* No longer forgotten */
            p->spell_flags[j] &= ~PY_SPELL_FORGOTTEN;

            /* Known once more */
            p->spell_flags[j] |= PY_SPELL_LEARNED;

            /* Message */
            msg(p, "You have remembered the %s of %s.", spell->realm->spell_noun, spell->name);

            /* One less can be learned */
            p->upkeep->new_spells--;
            num_forgotten--;
        }
    }      

    /* Assume no spells available */
    k = 0;

    /* Count spells that can be learned */
    for (j = 0; j < num_total; j++)
    {
        /* Access the spell */
        spell = spell_by_index(&p->clazz->magic, j);

        /* Skip spells we cannot remember or don't exist */
        if (!spell) continue;
        if ((spell->slevel > p->lev) || (spell->slevel == 0)) continue;

        /* Skip spells we already know */
        if (p->spell_flags[j] & PY_SPELL_LEARNED) continue;

        /* Count it */
        k++;
    }

    /* Cannot learn more spells than exist */
    if (p->upkeep->new_spells > k) p->upkeep->new_spells = k;

    /* Wait for creation */
    if (!p->alive) return;

    /* Spell count changed */
    /* Delay messages after character creation */
    if (p->delayed_display || (old_spells != p->upkeep->new_spells))
    {
        /* Message if needed */
        if (p->upkeep->new_spells)
        {
            char buf[120];
            struct class_book *book = &p->clazz->magic.books[0];
            int i;

            my_strcpy(buf, book->realm->spell_noun, sizeof(buf));
            if (p->upkeep->new_spells > 1) my_strcat(buf, "s", sizeof(buf));

            for (i = 1; i < p->clazz->magic.num_books; i++)
            {
                book = &p->clazz->magic.books[i];

                if (!strstr(buf, book->realm->spell_noun))
                {
                    my_strcat(buf, "/", sizeof(buf));
                    my_strcat(buf, book->realm->spell_noun, sizeof(buf));
                    if (p->upkeep->new_spells > 1) my_strcat(buf, "s", sizeof(buf));
                }
            }

            /* Message */
            msg(p, "You can learn %d more %s.", p->upkeep->new_spells, buf);
        }

        /* Redraw Study Status */
        p->upkeep->redraw |= (PR_STUDY);
    }
}


/*
 * Calculate maximum mana. You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 *
 * This function induces status messages.
 */
static void calc_mana(struct player *p, struct player_state *state, bool update)
{
    int i, msp, levels, cur_wgt, max_wgt, adj;
    struct object *obj;
    int exmsp = 0;
    int32_t modifiers[OBJ_MOD_MAX];

    /* Shapechangers get arbitrary mana */
    if (player_has(p, PF_MONSTER_SPELLS))
    {
        /* Arbitrary value (should be enough) */
        msp = 2 * p->lev;
    }

    /* Must be literate */
    else if (!p->clazz->magic.total_spells)
    {
        p->msp = 0;
        p->csp = 0;
        p->csp_frac = 0;
        return;
    }

    /* Extract "effective" player level */
    else
    {
        levels = (p->lev - p->clazz->magic.spell_first) + 1;
        if (levels > 0)
        {
            msp = 1;
            msp += adj_mag_mana[average_spell_stat(p, state)] * levels / 100;
        }
        else
        {
            levels = 0;
            msp = 0;
        }
    }

    /* Assume player not encumbered by armor */
    state->cumber_armor = false;

    /* Weigh the armor */
    cur_wgt = 0;
    for (i = 0; i < p->body.count; i++)
    {
        struct object *slot_obj = slot_object(p, i);

        /* Ignore non-armor */
        if (slot_type_is(p, i, EQUIP_WEAPON)) continue;
        if (slot_type_is(p, i, EQUIP_BOW)) continue;
        if (slot_type_is(p, i, EQUIP_RING)) continue;
        if (slot_type_is(p, i, EQUIP_AMULET)) continue;
        if (slot_type_is(p, i, EQUIP_LIGHT)) continue;
        if (slot_type_is(p, i, EQUIP_TOOL)) continue;

        /* Add weight */
        if (slot_obj) cur_wgt += slot_obj->weight;
    }

    /* Determine the weight allowance */
    max_wgt = p->clazz->magic.spell_weight;

    /* Heavy armor penalizes mana */
    if (((cur_wgt - max_wgt) / 10) > 0)
    {
        /* Encumbered */
        state->cumber_armor = true;

        /* Reduce mana */
        msp -= ((cur_wgt - max_wgt) / 10);
    }

    /* Get the gloves */
    obj = equipped_item_by_slot_name(p, "hands");

    object_modifiers(obj, modifiers);

    /* Extra mana capacity from gloves */
    exmsp += modifiers[OBJ_MOD_MANA];

    /* Get the weapon */
    obj = equipped_item_by_slot_name(p, "weapon");

    object_modifiers(obj, modifiers);

    /* Extra mana capacity from weapon */
    exmsp += modifiers[OBJ_MOD_MANA];

    /* Cap extra mana capacity from items at +10 */
    if (exmsp > 10) exmsp = 10;

    /* Polymorphed players only get half adjustment from race */
    adj = race_modifier(p->race, OBJ_MOD_MANA, p->lev, p->poly_race? true: false);

    adj += class_modifier(p->clazz, OBJ_MOD_MANA, p->lev);

    /* Extra mana capacity from race/class bonuses */
    exmsp += adj;

    /* Cap extra mana capacity at +15 */
    if (exmsp > 15) exmsp = 15;

    /* 1 point = 10% more mana */
    msp = ((10 + exmsp) * msp) / 10;

    /* Meditation increase mana at the cost of hp */
    if (p->timed[TMD_MEDITATE]) msp = (3 * msp) / 2;

    /* Mana can never be negative */
    if (msp < 0) msp = 0;

    /* Return if no updates */
    if (!update) return;

    /* Maximum mana has changed */
    if (p->msp != msp)
    {
        int old_num = get_player_num(p);

        /* Player has no mana now */
        if (!msp) player_clear_timed(p, TMD_MANASHIELD, true);

        /* Save new limit */
        p->msp = msp;

        /* Enforce new limit */
        if (p->csp >= msp)
        {
            p->csp = msp;
            p->csp_frac = 0;
        }

        /* Redraw picture */
        redraw_picture(p, old_num);

        /* Display mana later */
        p->upkeep->redraw |= (PR_MANA);
    }
}


/*
 * Calculate the players (maximal) hit points
 *
 * Adjust current hitpoints if necessary
 */
static void calc_hitpoints(struct player *p, struct player_state *state, bool update)
{
    long bonus;
    int mhp;

    /* Get "1/100th hitpoint bonus per level" value */
    bonus = adj_con_mhp[state->stat_ind[STAT_CON]];

    /* Calculate hitpoints */
    mhp = p->player_hp[p->lev - 1] + (bonus * p->lev / 100);

    /* Always have at least one hitpoint per level */
    if (mhp < p->lev + 1) mhp = p->lev + 1;

    /* Handle polymorphed players */
    if (p->poly_race)
        mhp = mhp * 3 / 5 + (1400 * p->poly_race->avg_hp) / (p->poly_race->avg_hp + 4200);

    /* Meditation increase mana at the cost of hp */
    if (p->timed[TMD_MEDITATE]) mhp = mhp * 3 / 5;

    /* Return if no updates */
    if (!update) return;

    /* New maximum hitpoints */
    if (p->mhp != mhp)
    {
        int old_num = get_player_num(p);

        /* Save new limit */
        p->mhp = mhp;

        /* Enforce new limit */
        if (p->chp >= mhp)
        {
            p->chp = mhp;
            p->chp_frac = 0;
        }

        /* Redraw picture */
        redraw_picture(p, old_num);

        /* Display hitpoints (later) */
        p->upkeep->redraw |= (PR_HP);
    }
}


/*
 * Calculate and set the current light radius.
 *
 * The light radius will be the total of all lights carried.
 */
static void calc_light(struct player *p, struct player_state *state, bool update)
{
    int i, adj;

    /* Assume no light */
    state->cur_light = 0;

    /* Ascertain lightness if outside of the dungeon */
    if ((p->wpos.depth == 0) && is_daytime())
    {
        /* Update the visuals if necessary */
        if (update && (p->state.cur_light != state->cur_light))
            p->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

        return;
    }

    /* Examine all wielded objects */
    for (i = 0; i < p->body.count; i++)
    {
        int amt = 0;
        struct object *obj = slot_object(p, i);
        int32_t modifiers[OBJ_MOD_MAX];

        /* Skip empty slots */
        if (!obj) continue;

        object_modifiers(obj, modifiers);

        /* Light radius - innate plus modifier */
        if (of_has(obj->flags, OF_LIGHT_2)) amt = 2;
        else if (of_has(obj->flags, OF_LIGHT_3)) amt = 3;
        else if (of_has(obj->flags, OF_LIGHT_4)) amt = 4;
        amt += modifiers[OBJ_MOD_LIGHT];

        /* Lights without fuel provide no light */
        if (tval_is_light(obj) && !of_has(obj->flags, OF_NO_FUEL) && (obj->timeout == 0))
            amt = 0;

        /* Alter state->cur_light if reasonable */
        state->cur_light += amt;
    }

    /* Polymorphed players only get half adjustment from race */
    adj = race_modifier(p->race, OBJ_MOD_LIGHT, p->lev, p->poly_race? true: false);

    adj += class_modifier(p->clazz, OBJ_MOD_LIGHT, p->lev);

    /* Extra light from race/class bonuses */
    state->cur_light += adj;
}


/*
 * Populates `chances` with the player's chance of digging through
 * the diggable terrain types in one turn out of 1600.
 */
void calc_digging_chances(struct player *p, struct player_state *state, int chances[DIGGING_MAX])
{
    int i;

    chances[DIGGING_TREE] = (state->skills[SKILL_DIGGING] + wielding_cut(p) * 10) * 4;
    chances[DIGGING_RUBBLE] = state->skills[SKILL_DIGGING] * 8;
    chances[DIGGING_MAGMA] = (state->skills[SKILL_DIGGING] - 10) * 4;
    chances[DIGGING_QUARTZ] = (state->skills[SKILL_DIGGING] - 20) * 2;
    chances[DIGGING_GRANITE] = state->skills[SKILL_DIGGING] - 40;
    chances[DIGGING_DOORS] = (state->skills[SKILL_DIGGING] - 30) * 4 / 3;

    /* Don't let any negative chances through */
    for (i = 0; i < DIGGING_MAX; i++) chances[i] = MAX(0, chances[i]);
}


/*
 * Return the chance, out of 100, for unlocking a locked door with the given
 * lock power.
 *
 * p is the player trying to unlock the door.
 * lock_power is the power of the lock.
 * lock_unseen, if true, assumes the player does not have sufficient
 * light to work with the lock.
 */
int calc_unlocking_chance(const struct player *p, int lock_power, bool lock_unseen)
{
    int skill = p->state.skills[SKILL_DISARM_PHYS];

    return calc_skill(p, skill, 4 * lock_power, lock_unseen);
}


int calc_skill(const struct player *p, int skill, int power, bool unseen)
{
    if (unseen || p->timed[TMD_BLIND]) skill /= 10;
    if (p->timed[TMD_CONFUSED] || p->timed[TMD_IMAGE]) skill /= 10;

    /* Always have a small chance of success */
    return MAX(2, skill - power);
}


bool obj_kind_can_browse(struct player *p, const struct object_kind *kind)
{
    int i;

    if (!p) return true;

    for (i = 0; i < p->clazz->magic.num_books; i++)
    {
        struct class_book *book = &p->clazz->magic.books[i];

        if ((kind->tval == book->tval) && (kind->sval == book->sval))
            return true;
    }

    return false;
}


bool obj_can_browse(struct player *p, const struct object *obj)
{
    return obj_kind_can_browse(p, obj->kind);
}


/*
 * Decide which object comes earlier in the standard inventory listing,
 * defaulting to the first if nothing separates them.
 *
 * Returns whether to replace the original object with the new one.
 */
bool earlier_object(struct player *p, struct object *orig, struct object *newobj, bool store)
{
    /* Check we have actual objects */
    if (!newobj) return false;
    if (!orig) return true;

    /* Readable books always come first */
    if (!store)
    {
        if (obj_can_browse(p, orig) && !obj_can_browse(p, newobj)) return false;
        if (!obj_can_browse(p, orig) && obj_can_browse(p, newobj)) return true;
    }

    /* Usable ammo is before other ammo */
    if (tval_is_ammo(orig) && tval_is_ammo(newobj))
    {
        /* First favour usable ammo */
        if (p && (p->state.ammo_tval == orig->tval) && (p->state.ammo_tval != newobj->tval))
            return false;
        if (p && (p->state.ammo_tval != orig->tval) && (p->state.ammo_tval == newobj->tval))
            return true;
    }

    /* Objects sort by decreasing type */
    if (orig->tval > newobj->tval) return false;
    if (orig->tval < newobj->tval) return true;

    /* Non-aware (flavored) items always come last */
    if (!store)
    {
        if (!object_flavor_is_aware(p, newobj)) return false;
        if (!object_flavor_is_aware(p, orig)) return true;
    }

    /* Objects sort by increasing sval */
    if (orig->sval < newobj->sval) return false;
    if (orig->sval > newobj->sval) return true;

    /* Unaware objects always come last */
    if (!store)
    {
        if (!object_is_known(p, newobj)) return false;
        if (!object_is_known(p, orig)) return true;
    }

    /* Lights sort by decreasing fuel */
    if (!store && tval_is_light(orig))
    {
        if (orig->pval > newobj->pval) return false;
        if (orig->pval < newobj->pval) return true;
    }

    /* Objects sort by decreasing value, except ammo */
    if (tval_is_ammo(orig))
    {
        if (object_value_real(p, orig, 1) < object_value_real(p, newobj, 1)) return false;
        if (object_value_real(p, orig, 1) > object_value_real(p, newobj, 1)) return true;
    }
    else
    {
        if (object_value_real(p, orig, 1) > object_value_real(p, newobj, 1)) return false;
        if (object_value_real(p, orig, 1) < object_value_real(p, newobj, 1)) return true;
    }

    /* No preference */
    return false;
}


/*
 * Put the player's inventory and quiver into easily accessible arrays. The
 * pack may be overfull by one item.
 */
void calc_inventory(struct player *p)
{
    int old_inven_cnt = p->upkeep->inven_cnt;
    int n_stack_split = 0;
    int n_pack_remaining = z_info->pack_size - pack_slots_used(p);
    int n_max = 1 + z_info->pack_size + z_info->quiver_size + p->body.count;
    struct object **old_quiver = mem_zalloc(z_info->quiver_size * sizeof(*old_quiver));
    struct object **old_pack = mem_zalloc(z_info->pack_size * sizeof(*old_pack));
    bool *assigned = mem_alloc(n_max * sizeof(*assigned));
    struct object *current;
    int i, j;
    bool redraw = false;

    /*
     * Equipped items are already taken care of. Only the others need
     * to be tested for assignment to the quiver or pack.
     */
    for (current = p->gear, j = 0; current; current = current->next, ++j)
    {
        my_assert(j < n_max);
        assigned[j] = object_is_equipped(p->body, current);
    }
    for (; j < n_max; ++j) assigned[j] = false;

    /* Prepare to fill the quiver */
    p->upkeep->quiver_cnt = 0;

    /* Copy the current quiver and then leave it empty. */
    for (i = 0; i < z_info->quiver_size; i++)
    {
        if (p->upkeep->quiver[i])
        {
            old_quiver[i] = p->upkeep->quiver[i];
            p->upkeep->quiver[i] = NULL;
        }
    }

    /* Fill quiver. First, allocate inscribed items. */
    for (current = p->gear, j = 0; current; current = current->next, ++j)
    {
        int prefslot;

        /* Skip already assigned (i.e. equipped) items. */
        if (assigned[j]) continue;

        prefslot = preferred_quiver_slot(p, current);
        if (prefslot >= 0 && prefslot < z_info->quiver_size && !p->upkeep->quiver[prefslot])
        {
            /*
             * The preferred slot is empty. Split the stack if
             * necessary. Don't allow splitting if it could
             * result in overfilling the pack by more than one slot.
             */
            int mult = (tval_is_ammo(current)? 1: z_info->thrown_quiver_mult);
            struct object *to_quiver;

            if (current->number * mult <= z_info->quiver_slot_size)
                to_quiver = current;
            else
            {
                int nsplit = z_info->quiver_slot_size / mult;

                my_assert(nsplit < current->number);
                if ((nsplit > 0) && (n_stack_split <= n_pack_remaining))
                {
                    /*
                     * Split off the portion that go to the pack. Since the
                     * stack in the quiver is earlier in the gear list
                     * it will prefer to remain in the quiver in future
                     * calls to calc_inventory() and will be the preferential
                     * destination for merges in combine_pack().
                     */
                    to_quiver = current;
                    gear_insert_end(p, object_split(current, current->number - nsplit));
                    ++n_stack_split;
                }
                else
                    to_quiver = NULL;
            }

            if (to_quiver)
            {
                to_quiver->oidx = z_info->pack_size + p->body.count + prefslot;
                p->upkeep->quiver[prefslot] = to_quiver;
                p->upkeep->quiver_cnt += to_quiver->number * mult;

                /* That part of the gear has been dealt with. */
                assigned[j] = true;
            }
        }
    }

    /* Now fill the rest of the slots in order. */
    for (i = 0; i < z_info->quiver_size; ++i)
    {
        struct object *first = NULL;
        int jfirst = -1;

        /* If the slot is full, move on. */
        if (p->upkeep->quiver[i]) continue;

        /* Find the quiver object that should go there. */
        j = 0;
        current = p->gear;
        while (1)
        {
            if (!current) break;
            my_assert(j < n_max);

            /*
             * Only try to assign if not assigned, ammo, and,
             * if necessary to split, have room for the split
             * stacks.
             */
            if (!assigned[j] && tval_is_ammo(current) &&
                (current->number <= z_info->quiver_slot_size ||
                (z_info->quiver_slot_size > 0 && n_stack_split <= n_pack_remaining)))
            {
                /* Choose the first in order. */
                if (earlier_object(p, first, current, false))
                {
                    first = current;
                    jfirst = j;
                }
            }

            current = current->next;
            ++j;
        }

        /* Stop looking if there's nothing left. */
        if (!first) break;

        /* Put the item in the slot, splitting (if needed) to fit. */
        if (first->number > z_info->quiver_slot_size)
        {
            my_assert(z_info->quiver_slot_size > 0 && n_stack_split <= n_pack_remaining);

            /* As above, split off the portion going to the pack. */
            gear_insert_end(p, object_split(first, first->number - z_info->quiver_slot_size));
        }

        first->oidx = z_info->pack_size + p->body.count + i;
        p->upkeep->quiver[i] = first;
        p->upkeep->quiver_cnt += first->number;

        /* That part of the gear has been dealt with. */
        assigned[jfirst] = true;
    }

    /* Note reordering */
    for (i = 0; i < z_info->quiver_size; i++)
    {
        if (old_quiver[i] && (p->upkeep->quiver[i] != old_quiver[i]))
        {
            msg(p, "You re-arrange your quiver.");
            break;
        }
    }

    for (i = 0; i < z_info->quiver_size; i++)
    {
        if (p->upkeep->quiver[i] != old_quiver[i])
        {
            redraw = true;
            break;
        }
    }

    /* Copy the current pack */
    for (i = 0; i < z_info->pack_size; i++) old_pack[i] = p->upkeep->inven[i];

    /* Prepare to fill the inventory */
    p->upkeep->inven_cnt = 0;

    for (i = 0; i <= z_info->pack_size; i++)
    {
        struct object *first = NULL;
        int jfirst = -1;

        /* Find the object that should go there. */
        j = 0;
        current = p->gear;
        while (1)
        {
            if (!current) break;
            my_assert(j < n_max);

            /* Consider it if it hasn't already been handled. */
            if (!assigned[j])
            {
                /* Choose the first in order. */
                if (earlier_object(p, first, current, false))
                {
                    first = current;
                    jfirst = j;
                }
            }

            current = current->next;
            ++j;
        }

        /* Allocate */
        if (first) first->oidx = i;
        p->upkeep->inven[i] = first;
        if (first)
        {
            ++p->upkeep->inven_cnt;
            assigned[jfirst] = true;
        }
    }

    /* Note reordering */
    if (p->upkeep->inven_cnt == old_inven_cnt)
    {
        for (i = 0; i < z_info->pack_size; i++)
        {
            if (old_pack[i] && (p->upkeep->inven[i] != old_pack[i]) &&
                !object_is_equipped(p->body, old_pack[i]))
            {
                msg(p, "You re-arrange your pack.");
                break;
            }
        }
    }

    for (i = 0; i < z_info->pack_size; i++)
    {
        if (p->upkeep->inven[i] != old_pack[i])
        {
            redraw = true;
            break;
        }
    }

    /* Redraw */
    if (redraw) set_redraw_inven(p, NULL);

    mem_free(assigned);
    mem_free(old_pack);
    mem_free(old_quiver);
}


/*
 * Calculate the blows a player would get.
 *
 * obj is the object for which we are calculating blows
 * state is the player state for which we are calculating blows
 * extra_blows is the number of +blows available from this object and this state
 *
 * Note: state->num_blows is now 100x the number of blows
 *
 * PWMAngband: extra_blows is now 10x the number of extra blows to allow +0.1bpr per level for monks
 */
static int calc_blows(struct player *p, const struct object *obj, struct player_state *state,
    int extra_blows)
{
    int blows = 100;
    int weight = ((obj == NULL)? 0: obj->weight);

    /* Monks get special barehanded attacks */
    if (!player_has(p, PF_MARTIAL_ARTS))
        blows = calc_blows_aux(p, weight, state->stat_ind[STAT_STR], state->stat_ind[STAT_DEX]);

    /* Require at least one blow */
    return MAX(blows + 10 * extra_blows, 100);
}


/*
 * Computes current weight limit.
 */
int weight_limit(struct player_state *state)
{
    int i;

    /* Weight limit based only on strength */
    i = adj_str_wgt[state->stat_ind[STAT_STR]] * 100;

    /* Return the result */
    return (i);
}


/*
 * Computes weight remaining before burdened.
 */
int weight_remaining(struct player *p)
{
    int i;

    /* Weight limit based only on strength */
    i = 60 * adj_str_wgt[p->state.stat_ind[STAT_STR]] - p->upkeep->total_weight - 1;

    /* Return the result */
    return (i);
}


/*
 * Adjust a value by a relative factor of the absolute value. Mimics the
 * inline calculations of value = (value * (den + num)) / num when value is
 * positive.
 *
 * v Is a pointer to the value to adjust.
 * num Is the numerator of the relative factor. Use a negative value
 * for a decrease in the value, and a positive value for an increase.
 * den Is the denominator for the relative factor. Must be positive.
 * minv Is the minimum absolute value of v to use when computing the
 * adjustment; use zero for this to get a pure relative adjustment.
 * Must be non-negative.
 */
static void adjust_skill_scale(int *v, int num, int den, int minv)
{
    if (num >= 0)
        *v += (MAX(minv, ABS(*v)) * num) / den;
    else
    {
        /*
         * To mimic what (value * (den + num)) / den would give for
         * positive value, need to round up the adjustment.
         */
        *v -= (MAX(minv, ABS(*v)) * -num + den - 1) / den;
    }
}


static int getAvgDam(struct monster_race *race)
{
    int m, tot = 0, avg;

    for (m = 0; m < z_info->mon_blows_max; m++)
    {
        /* Skip non-attacks */
        if (!race->blow[m].method) continue;

        /* Extract the attack info */
        tot += race->blow[m].dice.dice * (race->blow[m].dice.sides + 1);
    }

    /* Average damage per attack */
    avg = tot / (2 * z_info->mon_blows_max);
    if (avg == 0) return 0;

    /* Mitigate to avoid very high values */
    return 1 + (50 * avg) / (avg + 50);
}


/*
 * Computes extra ac for monks wearing very light or no armour at all.
 *
 * obj -- the armor part to check
 * bonus -- ac bonus for this armor part when wearing no armor
 * k_min -- threshold for light armor (half bonus)
 * k_max -- threshold for heavy armor (no bonus)
 * level -- player level
 */
static int monk_get_extra_ac(const struct object *obj, int bonus, const struct object_kind *k_min,
    const struct object_kind *k_max, int level)
{
    int min, max, extra_ac = bonus * level / 50;

    /* No armor: full bonus */
    if (!obj) return extra_ac;

    /* No capacity: no bonus */
    if (!k_min || !k_max) return 0;

    min = k_min->weight;
    max = k_min->weight + (k_max->weight - k_min->weight) * level / 50;

    /* Light armor: half bonus */
    if (obj->weight <= min) return extra_ac / 2;

    /* Heavy armor: no bonus */
    if ((min >= max) || (obj->weight >= max)) return 0;

    /* Partial bonus */
    return (extra_ac * (max - obj->weight) / (max - min) / 2);
}


/*
 * Checks whether the player knows the given modifier of an object
 *
 * obj is the object
 * mod is the modifier
 */
static bool object_modifier_is_known(const struct object *obj, int mod, bool aware)
{
    size_t i;

    if (mod < 0 || mod >= OBJ_MOD_MAX) return false;

    /* Object has been exposed to the modifier means OK */
    if (easy_know(obj, aware) || obj->known->modifiers[mod])
        return true;

    /* Check curses */
    for (i = 0; obj->known->curses && (i < (size_t)z_info->curse_max); i++)
    {
        if (obj->known->curses[i].power == 0) continue;
        if (curses[i].obj->modifiers[mod])
            return true;
    }

    return false;
}


/*
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 *
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * If known_only is true, calc_bonuses() will only use the known
 * information of objects; thus it returns what the player _knows_
 * the character state to be.
 */
void calc_bonuses(struct player *p, struct player_state *state, bool known_only, bool update)
{
    int i, j, hold;
    int extra_blows = 0;
    int extra_shots = 0;
    int extra_might = 0;
    int extra_moves = 0;
    struct object *launcher = equipped_item_by_slot_name(p, "shooting");
    struct object *weapon = equipped_item_by_slot_name(p, "weapon");
    bitflag f[OF_SIZE], f2[OF_SIZE];
    bitflag collect_f[OF_SIZE];
    bool vuln[ELEM_MAX];
    bool unencumbered_monk = monk_armor_ok(p);
    bool restrict_ = (player_has(p, PF_MARTIAL_ARTS) && !unencumbered_monk);
    uint8_t cumber_shield = 0;
    struct element_info el_info[ELEM_MAX];
    struct object *tool = equipped_item_by_slot_name(p, "tool");
    int eq_to_a = 0;

    create_obj_flag_mask(f2, 0, OFT_ESP, OFT_MAX);

    /* Set various defaults */
    state->speed = 110;
    state->num_blows = 100;

    /* Extract race/class info */
    for (i = 0; i < SKILL_MAX; i++)
        state->skills[i] = p->race->r_skills[i] + p->clazz->c_skills[i];
    player_elements(p, el_info);
    for (i = 0; i < ELEM_MAX; i++)
    {
        vuln[i] = false;
        if (el_info[i].res_level[0] == -1)
            vuln[i] = true;
        else
            state->el_info[i].res_level[0] = el_info[i].res_level[0];
    }
    pf_wipe(state->pflags);
    pf_copy(state->pflags, p->race->pflags);
    pf_union(state->pflags, p->clazz->pflags);

    /* Extract the player flags */
    player_flags(p, collect_f);

    /* Ghost */
    if (p->ghost) state->see_infra += 3;

    /* UNLIGHT gives extra infravision */
    if (player_has(p, PF_UNLIGHT)) state->see_infra += (p->lev / 10 + 1);

    /* Handle polymorphed players */
    if (p->poly_race)
    {
        state->to_d += getAvgDam(p->poly_race);

        /* Fruit bat mode: get regular speed bonus */
        if (OPT(p, birth_fruit_bat)) state->speed += (p->poly_race->speed - 110);

        /* At low level, we get MOVES instead */
        else if (p->lev < 20) extra_moves = (p->poly_race->speed - 110) / 2;

        /* At higher level, we get 50% of speed bonus */
        else state->speed += (p->poly_race->speed - 110) / 2;
    }

    /* Analyze equipment */
    for (i = 0; i < p->body.count; i++)
    {
        int dig = 0;
        struct object *obj = slot_object(p, i);
        int32_t modifiers[OBJ_MOD_MAX];
        bool aware;

        /* Skip non-objects */
        if (!obj) continue;

        aware = object_flavor_is_aware(p, obj);

        /* Extract the item flags */
        if (known_only)
            object_flags_known(obj, f, aware);
        else
            object_flags(obj, f);

        of_union(collect_f, f);

        object_modifiers(obj, modifiers);
        object_elements(obj, el_info);

        for (j = 0; j < OBJ_MOD_MAX; j++)
        {
            if (known_only && !object_is_known(p, obj) && !object_modifier_is_known(obj, j, aware))
                modifiers[j] = 0;
        }

        /* Affect stats */
        state->stat_add[STAT_STR] += modifiers[OBJ_MOD_STR];
        state->stat_add[STAT_INT] += modifiers[OBJ_MOD_INT];
        state->stat_add[STAT_WIS] += modifiers[OBJ_MOD_WIS];
        state->stat_add[STAT_DEX] += modifiers[OBJ_MOD_DEX];
        state->stat_add[STAT_CON] += modifiers[OBJ_MOD_CON];

        /* Affect stealth */
        state->skills[SKILL_STEALTH] += modifiers[OBJ_MOD_STEALTH];

        /* Affect searching ability (factor of five) */
        state->skills[SKILL_SEARCH] += (modifiers[OBJ_MOD_SEARCH] * 5);

        /* Affect infravision */
        state->see_infra += modifiers[OBJ_MOD_INFRA];

        /* Affect digging (innate effect, plus bonus, times 20) */
        if (tval_is_digger(obj))
        {
            if (of_has(obj->flags, OF_DIG_1)) dig = 1;
            else if (of_has(obj->flags, OF_DIG_2)) dig = 2;
            else if (of_has(obj->flags, OF_DIG_3)) dig = 3;
        }
        dig += modifiers[OBJ_MOD_TUNNEL];
        state->skills[SKILL_DIGGING] += (dig * 20);

        /* Affect speed */
        state->speed += modifiers[OBJ_MOD_SPEED];

        /* Affect damage reduction */
        state->dam_red += modifiers[OBJ_MOD_DAM_RED];

        /* Affect blows */
        extra_blows += (modifiers[OBJ_MOD_BLOWS] * 10);

        /* Affect shots */
        extra_shots += modifiers[OBJ_MOD_SHOTS];

        /* Affect Might */
        extra_might += modifiers[OBJ_MOD_MIGHT];

        /* Affect movement speed */
        extra_moves += modifiers[OBJ_MOD_MOVES];

        /* Affect resists */
        for (j = 0; j < ELEM_MAX; j++)
        {
            if (!known_only || object_is_known(p, obj) || object_element_is_known(obj, j, aware))
            {
                /* Note vulnerability for later processing */
                if (el_info[j].res_level[0] == -1)
                    vuln[j] = true;

                /* OK because res_level has not included vulnerability yet */
                if (el_info[j].res_level[0] > state->el_info[j].res_level[0])
                    state->el_info[j].res_level[0] = el_info[j].res_level[0];
            }
        }

        /* Shield encumberance */
        if (kf_has(obj->kind->kind_flags, KF_TWO_HANDED)) cumber_shield++;
        if (slot_type_is(p, i, EQUIP_SHIELD) && cumber_shield) cumber_shield++;

        /* Modify the base armor class */
        state->ac += obj->ac;

        /* Apply the bonuses to armor class */
        if (!known_only || object_is_known(p, obj) || obj->known->to_a)
            eq_to_a += object_to_ac(obj);

        /* Do not apply weapon and bow bonuses until combat calculations */
        if (slot_type_is(p, i, EQUIP_WEAPON)) continue;
        if (slot_type_is(p, i, EQUIP_BOW)) continue;

        /* Apply the bonuses to hit/damage */
        if (!known_only || object_is_known(p, obj) || (obj->known->to_h && obj->known->to_d))
        {
            int16_t to_h, to_d;

            to_h = object_to_hit(obj);
            to_d = object_to_dam(obj);

            state->to_h += to_h;
            state->to_d += to_d;

            /* Unencumbered monks get double bonuses from gloves (if positive) */
            if (unencumbered_monk && slot_type_is(p, i, EQUIP_GLOVES))
            {
                if (to_h > 0) state->to_h += to_h;
                if (to_d > 0) state->to_d += to_d;
            }
        }
    }

    /* Handle polymorphed players */
    if (p->poly_race && (p->poly_race->ac > eq_to_a))
    {
        state->to_a += (p->poly_race->ac + eq_to_a) / 2;
    }
    else
        state->to_a += eq_to_a;

    /* Apply the collected flags */
    of_union(state->flags, collect_f);

    /* Handle polymorphed players */
    if (p->poly_race)
    {
        if (monster_is_stupid(p->poly_race)) state->stat_add[STAT_INT] -= 2;
        if (race_is_smart(p->poly_race)) state->stat_add[STAT_INT] += 2;
        if (p->poly_race->freq_spell == 33) state->stat_add[STAT_INT] += 1;
        if (p->poly_race->freq_spell == 50) state->stat_add[STAT_INT] += 3;
        if (p->poly_race->freq_spell == 100) state->stat_add[STAT_INT] += 5;
    }

    /* Adrenaline effects (part 1) */
    if (p->timed[TMD_ADRENALINE])
    {
        int16_t fx = (p->timed[TMD_ADRENALINE] - 1) / 20;

        /* Increase strength, dexterity, constitution */
        state->stat_add[STAT_STR] += fx;
        state->stat_add[STAT_DEX] += (fx + 1) / 2;
        state->stat_add[STAT_CON] += fx;
    }

    /* Elemental harmony */
    if (p->timed[TMD_HARMONY])
    {
        int16_t fx = (p->timed[TMD_HARMONY] - 1) / 20;

        /* Increase strength, dexterity, constitution */
        state->stat_add[STAT_STR] += fx;
        state->stat_add[STAT_DEX] += (fx + 1) / 2;
        state->stat_add[STAT_CON] += fx;
    }

    /* Extra growth */
    if (p->timed[TMD_GROWTH])
    {
        state->stat_add[STAT_STR] += 3;
        state->stat_add[STAT_INT] += 3;
        state->stat_add[STAT_WIS] += 3;
        state->stat_add[STAT_DEX] += 3;
        state->stat_add[STAT_CON] += 3;
    }

    /* Calculate the various stat values */
    for (i = 0; i < STAT_MAX; i++)
    {
        int add, use, r_adj;

        add = state->stat_add[i];

        /* Polymorphed players only get half adjustment from race */
        r_adj = race_modifier(p->race, i, p->lev, p->poly_race? true: false);

        add += (r_adj + class_modifier(p->clazz, i, p->lev));
        state->stat_top[i] = modify_stat_value(p->stat_max[i], add);
        use = modify_stat_value(p->stat_cur[i], add);

        state->stat_use[i] = use;

        /* Save the new index */
        state->stat_ind[i] = calc_stat_ind(use);
    }

    /* Apply race/class modifiers */
    for (i = STAT_MAX; i < OBJ_MOD_MAX; i++)
    {
        int r_adj, c_adj;

        /* Polymorphed players only get half adjustment from race */
        r_adj = race_modifier(p->race, i, p->lev, p->poly_race? true: false);

        c_adj = class_modifier(p->clazz, i, p->lev);

        /* Affect stealth */
        if (i == OBJ_MOD_STEALTH) state->skills[SKILL_STEALTH] += (r_adj + c_adj);

        /* Affect searching ability (factor of five) */
        if (i == OBJ_MOD_SEARCH) state->skills[SKILL_SEARCH] += ((r_adj + c_adj) * 5);

        /* Affect infravision */
        if (i == OBJ_MOD_INFRA) state->see_infra += (r_adj + c_adj);

        /* Affect digging (factor of 20) */
        if (i == OBJ_MOD_TUNNEL) state->skills[SKILL_DIGGING] += ((r_adj + c_adj) * 20);

        /* Affect speed */
        if (i == OBJ_MOD_SPEED)
        {
            /* Unencumbered monks get speed bonus */
            if (restrict_) c_adj = 0;

            state->speed += (r_adj + c_adj);
        }

        /* Affect damage reduction */
        if (i == OBJ_MOD_DAM_RED) state->dam_red += (r_adj + c_adj);

        /* Affect blows */
        if (i == OBJ_MOD_BLOWS)
        {
            /* Encumbered monks only get half the extra blows */
            if (restrict_) c_adj /= 2;

            extra_blows += (r_adj + c_adj);
        }

        /* Affect shots */
        if (i == OBJ_MOD_SHOTS) extra_shots += (r_adj + c_adj);

        /* Affect Might */
        if (i == OBJ_MOD_MIGHT) extra_might += (r_adj + c_adj);

        /* Affect movement speed */
        if (i == OBJ_MOD_MOVES) extra_moves += (r_adj + c_adj);
    }

    /* Unencumbered monks get extra ac for wearing very light or no armour at all */
    if (unencumbered_monk)
    {
        struct object_kind *k_min, *k_max;
        int extra_ac;

        /* Soft armor */
        k_min = lookup_kind_by_name(TV_SOFT_ARMOR, "Robe");
        k_max = lookup_kind_by_name(TV_SOFT_ARMOR, "Leather Scale Mail");
        extra_ac = monk_get_extra_ac(equipped_item_by_slot_name(p, "body"), 54, k_min, k_max,
            p->lev);
        state->to_a += extra_ac;

        /* Cloaks */
        k_max = lookup_kind_by_name(TV_CLOAK, "Fur Cloak");
        extra_ac = monk_get_extra_ac(equipped_item_by_slot_name(p, "back"), 12, k_max, k_max,
            p->lev);
        state->to_a += extra_ac;

        /* No bonus for wearing a shield */
        extra_ac = monk_get_extra_ac(equipped_item_by_slot_name(p, "arm"), 12, NULL, NULL,
            p->lev);
        state->to_a += extra_ac;

        /* Caps and crowns */
        k_max = lookup_kind_by_name(TV_CROWN, "Jewel Encrusted Crown");
        extra_ac = monk_get_extra_ac(equipped_item_by_slot_name(p, "head"), 12, k_max, k_max,
            p->lev);
        state->to_a += extra_ac;

        /* Gloves */
        k_max = lookup_kind_by_name(TV_GLOVES, "Set of Caestus");
        extra_ac = monk_get_extra_ac(equipped_item_by_slot_name(p, "hands"), 18, k_max, k_max,
            p->lev);
        state->to_a += extra_ac;

        /* Leather boots */
        k_max = lookup_kind_by_name(TV_BOOTS, "Pair of Leather Boots");
        extra_ac = monk_get_extra_ac(equipped_item_by_slot_name(p, "feet"), 12, k_max, k_max,
            p->lev);
        state->to_a += extra_ac;
    }

    /* Now deal with vulnerabilities */
    for (i = 0; i < ELEM_MAX; i++)
    {
        if (vuln[i] && (state->el_info[i].res_level[0] < 3))
            state->el_info[i].res_level[0]--;
    }

    /* Effects of food outside the "Fed" range */
    if (!player_timed_grade_eq(p, TMD_FOOD, "Fed"))
    {
        int excess = p->timed[TMD_FOOD] - PY_FOOD_FULL;
        int lack = PY_FOOD_HUNGRY - p->timed[TMD_FOOD];

        /* Scale to units 1/10 of the range and subtract from speed */
        if ((excess > 0) && !p->timed[TMD_ATT_VAMP])
        {
            excess = (excess * 10) / (PY_FOOD_MAX - PY_FOOD_FULL);
            state->speed -= excess;
        }

        /* Scale to units 1/20 of the range */
        else if (lack > 0)
        {
            lack = (lack * 20) / PY_FOOD_HUNGRY;

            /* Apply effects progressively */
            state->to_h -= lack;
            state->to_d -= lack;
            if ((lack > 10) && (lack <= 15))
                adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 10, 0);
            else if ((lack > 15) && (lack <= 18))
            {
                adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 5, 0);
                state->skills[SKILL_DISARM_PHYS] *= 9;
                state->skills[SKILL_DISARM_PHYS] /= 10;
                state->skills[SKILL_DISARM_MAGIC] *= 9;
                state->skills[SKILL_DISARM_MAGIC] /= 10;
            }
            else if (lack > 18)
            {
                adjust_skill_scale(&state->skills[SKILL_DEVICE], -3, 10, 0);
                state->skills[SKILL_DISARM_PHYS] *= 8;
                state->skills[SKILL_DISARM_PHYS] /= 10;
                state->skills[SKILL_DISARM_MAGIC] *= 8;
                state->skills[SKILL_DISARM_MAGIC] /= 10;
                state->skills[SKILL_SAVE] *= 9;
                state->skills[SKILL_SAVE] /= 10;
                state->skills[SKILL_SEARCH] *= 9;
                state->skills[SKILL_SEARCH] /= 10;
            }
        }
    }

    /* Other timed effects */
    player_flags_timed(p, state->flags);

    if (player_timed_grade_eq(p, TMD_STUN, "Heavy Stun"))
    {
        state->to_h -= 20;
        state->to_d -= 20;
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 5, 0);
        if (update) p->timed[TMD_FASTCAST] = 0;
    }
    else if (player_timed_grade_eq(p, TMD_STUN, "Stun"))
    {
        state->to_h -= 5;
        state->to_d -= 5;
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 10, 0);
        if (update) p->timed[TMD_FASTCAST] = 0;
    }
    if (p->timed[TMD_ADRENALINE])
    {
        int16_t fx = (p->timed[TMD_ADRENALINE] - 1) / 20;

        if (fx >= 2) state->to_d += 8;
        if (fx >= 3) extra_blows += 10;
    }
    if (p->timed[TMD_INVULN])
        state->to_a += 100;
    if (p->timed[TMD_BLESSED])
    {
        state->to_a += 5;
        state->to_h += 10;
        adjust_skill_scale(&state->skills[SKILL_DEVICE], 1, 20, 0);
    }
    if (p->timed[TMD_SHIELD])
        state->to_a += 50;
    if (p->timed[TMD_ICY_AURA])
        state->to_a += 10;
    if (p->timed[TMD_STONESKIN])
    {
        state->to_a += 40;
        state->speed -= 5;
    }
    if (p->timed[TMD_HERO])
    {
        state->to_h += 12;
        adjust_skill_scale(&state->skills[SKILL_DEVICE], 1, 20, 0);
    }
    if (p->timed[TMD_SHERO])
    {
        state->to_h += 24;
        state->to_a -= 10;
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 10, 0);
    }
    if (p->timed[TMD_FAST] || p->timed[TMD_SPRINT]) state->speed += 10;
    if (p->timed[TMD_FLIGHT]) state->speed += 5;
    if (p->timed[TMD_SLOW]) state->speed -= 10;
    if (p->timed[TMD_SINFRA]) state->see_infra += 5;
    if (of_has(state->flags, OF_ESP_ALL))
    {
        of_diff(state->flags, f2);
        of_on(state->flags, OF_ESP_ALL);
    }
    if (p->timed[TMD_TERROR]) state->speed += 10;
    for (i = 0; i < TMD_MAX; ++i)
    {
        if (p->timed[i] && timed_effects[i].temp_resist != -1 &&
            state->el_info[timed_effects[i].temp_resist].res_level[0] < 2)
        {
            state->el_info[timed_effects[i].temp_resist].res_level[0]++;
        }
    }
    if (p->timed[TMD_ANCHOR])
    {
        state->el_info[ELEM_TIME].res_level[0]++;
        state->el_info[ELEM_GRAVITY].res_level[0] = 1;
    }
    if (p->timed[TMD_CONFUSED])
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 4, 0);
    if (p->timed[TMD_AMNESIA])
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 5, 0);
    if (p->timed[TMD_POISONED])
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 20, 0);
    if (p->timed[TMD_IMAGE])
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 5, 0);
    if (p->timed[TMD_BLOODLUST])
    {
        state->to_d += p->timed[TMD_BLOODLUST] / 2;
        extra_blows += p->timed[TMD_BLOODLUST] / 2;
    }
    if (p->timed[TMD_STEALTH])
        state->skills[SKILL_STEALTH] += 10;

    /* Analyze flags - check for fear */
    if (of_has(state->flags, OF_AFRAID))
    {
        state->to_h -= 20;
        state->to_a += 8;
        adjust_skill_scale(&state->skills[SKILL_DEVICE], -1, 20, 0);
    }

    /* Analyze weight */
    j = p->upkeep->total_weight;
    if (j > (1 << 14)) j = (1 << 14);
    i = weight_limit(state);
    if (j > i / 2) state->speed -= ((j - (i / 2)) / (i / 10));

    /* Adding "stealth mode" for rogues */
    if (p->stealthy)
    {
        state->speed -= 10;
        state->skills[SKILL_STEALTH] *= 3;
    }

    /* Sanity check on extreme speeds */
    if (state->speed < 0) state->speed = 0;
    if (state->speed > 199) state->speed = 199;

    /* Apply modifier bonuses (Un-inflate stat bonuses) */
    state->to_a += adj_dex_ta[state->stat_ind[STAT_DEX]];
    state->to_d += adj_str_td[state->stat_ind[STAT_STR]];
    state->to_h += adj_dex_th[state->stat_ind[STAT_DEX]];
    state->to_h += adj_str_th[state->stat_ind[STAT_STR]];

    /* Modify skills */
    state->skills[SKILL_DISARM_PHYS] += adj_dex_dis[state->stat_ind[STAT_DEX]];
    state->skills[SKILL_DISARM_MAGIC] += adj_int_dis[state->stat_ind[STAT_INT]];
    state->skills[SKILL_DEVICE] += adj_int_dev[state->stat_ind[STAT_INT]];
    state->skills[SKILL_SAVE] += adj_wis_sav[state->stat_ind[STAT_WIS]];
    if (p->timed[TMD_SAFE]) state->skills[SKILL_SAVE] = 100;
    state->skills[SKILL_DIGGING] += adj_str_dig[state->stat_ind[STAT_STR]];
    if (p->poly_race && rf_has(p->poly_race->flags, RF_KILL_WALL))
        state->skills[SKILL_DIGGING] = 2000;
    if (p->poly_race && rf_has(p->poly_race->flags, RF_SMASH_WALL))
        state->skills[SKILL_DIGGING] = 2000;
    for (i = 0; i < SKILL_MAX; i++)
        state->skills[i] += (p->clazz->x_skills[i] * p->lev / 10);
    if (p->poly_race)
    {
        if (p->poly_race->weight == 0) state->skills[SKILL_STEALTH] += 0;
        else if (p->poly_race->weight <= 50) state->skills[SKILL_STEALTH] += 2;
        else if (p->poly_race->weight <= 100) state->skills[SKILL_STEALTH] += 1;
        else if (p->poly_race->weight <= 150) state->skills[SKILL_STEALTH] += 0;
        else if (p->poly_race->weight <= 450) state->skills[SKILL_STEALTH] -= 1;
        else if (p->poly_race->weight <= 2000) state->skills[SKILL_STEALTH] -= 2;
        else if (p->poly_race->weight <= 10000) state->skills[SKILL_STEALTH] -= 3;
        else state->skills[SKILL_STEALTH] -= 4;
    }

    if (state->skills[SKILL_DIGGING] < 1) state->skills[SKILL_DIGGING] = 1;
    if (state->skills[SKILL_STEALTH] > 30) state->skills[SKILL_STEALTH] = 30;
    if (state->skills[SKILL_STEALTH] < 0) state->skills[SKILL_STEALTH] = 0;
    hold = adj_str_hold[state->stat_ind[STAT_STR]];

    if (state->skills[SKILL_DEVICE] < 0) state->skills[SKILL_DEVICE] = 0;

    /* Analyze launcher */
    state->heavy_shoot = false;
    if (launcher)
    {
        if (hold < launcher->weight / 10)
        {
            state->to_h += 2 * (hold - launcher->weight / 10);
            state->heavy_shoot = true;
        }

        state->num_shots = 10;

        /* Type of ammo */
        if (kf_has(launcher->kind->kind_flags, KF_SHOOTS_SHOTS))
            state->ammo_tval = TV_SHOT;
        else if (kf_has(launcher->kind->kind_flags, KF_SHOOTS_ARROWS))
            state->ammo_tval = TV_ARROW;
        else if (kf_has(launcher->kind->kind_flags, KF_SHOOTS_BOLTS))
            state->ammo_tval = TV_BOLT;

        /* Multiplier */
        state->ammo_mult = launcher->pval;

        /* Apply special flags */
        if (!state->heavy_shoot)
        {
            state->num_shots += extra_shots;
            state->ammo_mult += extra_might;
        }

        /* Handle polymorphed players */
        if (p->poly_race && (rsf_has(p->poly_race->spell_flags, RSF_SHOT) ||
            rsf_has(p->poly_race->spell_flags, RSF_ARROW) ||
            rsf_has(p->poly_race->spell_flags, RSF_BOLT)))
        {
            state->num_shots += 5;
        }

        /* Require at least one shot */
        if (state->num_shots < 10) state->num_shots = 10;

        /* PWMAngband: require at least a multiplier of one */
        if (state->ammo_mult < 1) state->ammo_mult = 1;
    }

    /* Temporary "Farsight" */
    if (p->timed[TMD_FARSIGHT])
    {
        int bonus = (p->lev - 7) / 10;

        state->to_h += bonus;
        state->see_infra += bonus;
    }
    if (p->timed[TMD_ZFARSIGHT])
        state->see_infra += p->lev / 4;

    /* Analyze weapon */
    state->heavy_wield = false;
    state->bless_wield = false;
    if (weapon)
    {
        /* It is hard to hold a heavy weapon */
        if (hold < weapon->weight / 10)
        {
            state->to_h += 2 * (hold - weapon->weight / 10);
            state->heavy_wield = true;
        }

        /* Normal weapons */
        if (!state->heavy_wield)
        {
            state->num_blows = calc_blows(p, weapon, state, extra_blows);
            if (!tool || !tval_is_digger(tool))
                state->skills[SKILL_DIGGING] += (weapon->weight / 10);
        }

        /* Divine weapon bonus for blessed weapons */
        if (pf_has(state->pflags, PF_BLESS_WEAPON) &&
            (weapon->tval == TV_HAFTED || of_has(state->flags, OF_BLESSED)))
        {
            state->to_d += 2;
            state->bless_wield = true;
        }
    }
    else
    {
        /* Unarmed */
        state->num_blows = calc_blows(p, NULL, state, extra_blows);
    }

    /* Unencumbered monks get a bonus tohit/todam */
    if (unencumbered_monk)
    {
        state->to_h += p->lev * 2 / 5;

        /* Polymorphed monks get half the to-dam bonus */
        if (p->poly_race)
            state->to_d += p->lev / 5;
        else
            state->to_d += p->lev * 2 / 5;
    }

    /* Assume no shield encumberance */
    state->cumber_shield = false;

    /* It is hard to wield a two-handed weapon with a shield */
    if (cumber_shield == 2)
    {
        if (state->to_h > 0) state->to_h = 2 * state->to_h / 3;
        state->to_h -= 2;
        state->cumber_shield = true;
    }

    /* Boost digging skill by digger weight */
    if (tool && tval_is_digger(tool))
        state->skills[SKILL_DIGGING] += (tool->weight / 10);

    /* Movement speed */
    state->num_moves = extra_moves;
    if (update && (p->state.num_moves != state->num_moves))
        p->upkeep->redraw |= (PR_STATE);

    /* Call individual functions for other state fields */
    calc_light(p, state, update);
    calc_mana(p, state, update);
    if (!p->msp) pf_on(state->pflags, PF_NO_MANA);
    calc_hitpoints(p, state, update);

    /* PWMAngband: display a message when a monk becomes encumbered */
    if (player_has(p, PF_MARTIAL_ARTS) && !unencumbered_monk)
        state->cumber_armor = true;
}


/*
 * Calculate bonuses, and print various things on changes
 */
static void update_bonuses(struct player *p)
{
    int i, flag;
    struct player_state state, known_state;
    int old_show_dd, show_dd;
    int old_show_ds, show_ds;
    int old_show_mhit, show_mhit;
    int old_show_mdam, show_mdam;
    int old_show_shit, show_shit;
    int old_show_sdam, show_sdam;
    bitflag f[OF_SIZE];

    /* Save the old hit/damage bonuses */
    get_plusses(p, &p->known_state, &old_show_dd, &old_show_ds, &old_show_mhit,
        &old_show_mdam, &old_show_shit, &old_show_sdam);

    /*
     * Calculate bonuses
     */

    memset(&state, 0, sizeof(state));
    memset(&known_state, 0, sizeof(known_state));
    calc_bonuses(p, &state, false, true);
    calc_bonuses(p, &known_state, true, true);

    /*
     * Notice changes
     */

    /* Analyze stats */
    for (i = 0; i < STAT_MAX; i++)
    {
        /* Notice changes */
        if (state.stat_top[i] != p->state.stat_top[i])
        {
            /* Redisplay the stats later */
            p->upkeep->redraw |= (PR_STATS);
        }

        /* Notice changes */
        if (state.stat_use[i] != p->state.stat_use[i])
        {
            /* Redisplay the stats later */
            p->upkeep->redraw |= (PR_STATS);
        }

        /* Notice changes */
        if (state.stat_ind[i] != p->state.stat_ind[i])
        {
            /* Change in stats may affect spells */
            p->upkeep->update |= (PU_SPELLS);
            p->upkeep->redraw |= PR_SPELL;
        }
    }

    /* Telepathy change */
    create_obj_flag_mask(f, 0, OFT_ESP, OFT_MAX);
    for (flag = of_next(f, FLAG_START); flag != FLAG_END; flag = of_next(f, flag + 1))
    {
        if (of_has(state.flags, flag) != of_has(p->state.flags, flag))
            p->upkeep->update |= (PU_MONSTERS);
    }

    /* See invis change */
    if (of_has(state.flags, OF_SEE_INVIS) != of_has(p->state.flags, OF_SEE_INVIS))
        p->upkeep->update |= (PU_MONSTERS);

    /* Redraw speed (if needed) */
    if (state.speed != p->state.speed) p->upkeep->redraw |= (PR_SPEED);

    /* Redraw armor (if needed) */
    if ((known_state.ac != p->known_state.ac) || (known_state.to_a != p->known_state.to_a))
    {
        /* Redraw */
        p->upkeep->redraw |= (PR_ARMOR);
    }

    /* Redraw plusses to hit/damage if necessary */
    get_plusses(p, &known_state, &show_dd, &show_ds, &show_mhit, &show_mdam, &show_shit,
        &show_sdam);
    if ((show_dd != old_show_dd) || (show_ds != old_show_ds) ||
        (show_mhit != old_show_mhit) || (show_mdam != old_show_mdam) ||
        (show_shit != old_show_shit) || (show_sdam != old_show_sdam))
    {
        /* Redraw plusses */
        p->upkeep->redraw |= (PR_PLUSSES);
    }

    /* Notice changes in the "light radius" */
    if (p->state.cur_light != state.cur_light)
    {
        /* Update the visuals */
        p->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
    }

    /* Notice changes to the weight limit */
    if (weight_limit(&p->state) != weight_limit(&state))
        set_redraw_inven(p, NULL);

    /* Wait for creation */
    if (!p->alive)
    {
        memcpy(&p->state, &state, sizeof(p->state));
        memcpy(&p->known_state, &known_state, sizeof(p->known_state));

        return;
    }

    /* Delay messages after character creation */
    if (!p->delayed_display)
    {
        /* Take note when "heavy bow" changes */
        if (p->state.heavy_shoot != state.heavy_shoot)
        {
            /* Message */
            if (state.heavy_shoot)
                msg(p, "You have trouble wielding such a heavy bow.");
            else if (equipped_item_by_slot_name(p, "shooting"))
                msg(p, "You have no trouble wielding your bow.");
            else
                msg(p, "You feel relieved to put down your heavy bow.");
        }

        /* Take note when "heavy weapon" changes */
        if (p->state.heavy_wield != state.heavy_wield)
        {
            /* Message */
            if (state.heavy_wield)
                msg(p, "You have trouble wielding such a heavy weapon.");
            else if (equipped_item_by_slot_name(p, "weapon"))
                msg(p, "You have no trouble wielding your weapon.");
            else
                msg(p, "You feel relieved to put down your heavy weapon.");
        }

        /* Take note when "blessed weapon" changes */
        if (p->state.bless_wield != state.bless_wield)
        {
            /* Message */
            if (state.bless_wield)
                msg(p, "You feel attuned to your weapon.");
            else if (equipped_item_by_slot_name(p, "weapon"))
                msg(p, "You feel less attuned to your weapon.");
        }

        /* Take note when "shield encumberance" changes */
        if (p->state.cumber_shield != state.cumber_shield)
        {
            /* Message */
            if (state.cumber_shield)
                msg(p, "You have trouble wielding your weapon with a shield.");
            else if (equipped_item_by_slot_name(p, "weapon"))
                msg(p, "You have no trouble wielding your weapon.");
            else
                msg(p, "You feel more comfortable after removing your weapon.");
        }

        /* Take note when "armor state" changes */
        if (p->state.cumber_armor != state.cumber_armor)
        {
            /* Message */
            if (state.cumber_armor)
                msg(p, "The weight of your armor reduces your maximum SP.");
            else
                msg(p, "Your maximum SP is no longer reduced by armor weight.");
        }
    }

    memcpy(&p->state, &state, sizeof(p->state));
    memcpy(&p->known_state, &known_state, sizeof(p->known_state));

    /* Send skills and weight */
    Send_skills(p);
    Send_weight(p, p->upkeep->total_weight, weight_remaining(p));

    /* Delay messages after character creation */
    if (p->delayed_display)
    {
        /* Message */
        if (p->state.heavy_shoot)
            msg(p, "You have trouble wielding such a heavy bow.");
        if (p->state.heavy_wield)
            msg(p, "You have trouble wielding such a heavy weapon.");
        if (p->state.bless_wield)
            msg(p, "You feel attuned to your weapon.");
        if (p->state.cumber_shield)
            msg(p, "You have trouble wielding your weapon with a shield.");
        if (p->state.cumber_armor)
            msg(p, "The weight of your armor reduces your maximum SP.");
    }
}


/*
 * Monster and object tracking functions
 */


/*
 * Track the given monster (or player)
 */
void health_track(struct player_upkeep *upkeep, struct source *who)
{
    if (source_null(who))
        memset(&upkeep->health_who, 0, sizeof(upkeep->health_who));
    else
        memcpy(&upkeep->health_who, who, sizeof(upkeep->health_who));

    upkeep->redraw |= (PR_HEALTH);
}


/*
 * Track the given monster race (or player)
 */
void monster_race_track(struct player_upkeep *upkeep, struct source *who)
{
    bool redraw = false;
    struct actor_race *monster_race = &upkeep->monster_race;

    /* Don't track anything */
    if (source_null(who))
        memset(monster_race, 0, sizeof(struct actor_race));

    /* Track this player */
    else if (who->player)
    {
        redraw = !ACTOR_PLAYER_EQUAL(monster_race, who);

        /* Save this player ID */
        monster_race->player = who->player;
        monster_race->race = NULL;
    }

    /* Track the given monster race */
    else if (who->monster)
    {
        redraw = !ACTOR_RACE_EQUAL(monster_race, who->monster);

        /* Save this monster ID */
        monster_race->player = NULL;
        monster_race->race = who->monster->race;
    }

    /* Redraw */
    if (redraw) upkeep->redraw |= (PR_MONSTER);
}


/*
 * Track the given object
 */
void track_object(struct player_upkeep *upkeep, struct object *obj)
{
    /* Paranoia */
    if (!upkeep) return;

    /* Redraw */
    if (upkeep->object != obj) upkeep->redraw |= (PR_OBJECT);

    /* Save this object */
    upkeep->object = obj;
}


/*
 * Is the given item tracked?
 */
bool tracked_object_is(struct player_upkeep *upkeep, struct object *obj)
{
    return (upkeep && (upkeep->object == obj));
}


/*
 * Cursor-track a new monster (or player)
 */
void cursor_track(struct player *p, struct source *who)
{
    /* Don't track anything */
    if (source_null(who))
        memset(&p->cursor_who, 0, sizeof(p->cursor_who));

    /* Track a new monster (or player) */
    else
        memcpy(&p->cursor_who, who, sizeof(p->cursor_who));
}


/*
 * Generic "deal with" functions
 */


/*
 * Handle "p->upkeep->notice"
 */
void notice_stuff(struct player *p)
{
    /* Nothing to do */
    if (!p->upkeep->notice) return;
    if (p->upkeep->notice & PN_WAIT) return;

    /* Deal with ignored stuff */
    /* PWMAngband: only on random levels (to avoid littering towns, wilderness and static levels) */
    /* Note: we also handle the newbies_cannot_drop option to avoid spamming useless messages */
    if (random_level(&p->wpos) && !newbies_cannot_drop(p) && (p->upkeep->notice & PN_IGNORE))
    {
        p->upkeep->notice &= ~(PN_IGNORE);
        cmd_ignore_drop(p);
    }

    /* Combine the pack */
    if (p->upkeep->notice & PN_COMBINE)
    {
        p->upkeep->notice &= ~(PN_COMBINE);
        combine_pack(p);
    }

    /* Dump the monster messages */
    if (p->upkeep->notice & PN_MON_MESSAGE)
    {
        p->upkeep->notice &= ~(PN_MON_MESSAGE);

        /* Make sure this comes after all of the monster messages */
        show_monster_messages(p);
    }
}


/*
 * Handle "p->upkeep->update"
 */
void update_stuff(struct player *p, struct chunk *c)
{
    /* Nothing to do */
    if (!p->upkeep->update) return;

    if (p->upkeep->update & PU_INVEN)
    {
        p->upkeep->update &= ~(PU_INVEN);
        calc_inventory(p);
    }

    if (p->upkeep->update & PU_BONUS)
    {
        p->upkeep->update &= ~(PU_BONUS);
        update_bonuses(p);
    }

    if (p->upkeep->update & PU_SPELLS)
    {
        p->upkeep->update &= ~(PU_SPELLS);
        if (p->clazz->magic.total_spells > 0) calc_spells(p);
    }

    /* Character is not ready yet, no map updates */
    if (!p->alive) return;

    if (p->upkeep->update & PU_UPDATE_VIEW)
    {
        p->upkeep->update &= ~(PU_UPDATE_VIEW);
        update_view(p, c);
    }

    if (p->upkeep->update & PU_DISTANCE)
    {
        p->upkeep->update &= ~(PU_DISTANCE);
        p->upkeep->update &= ~(PU_MONSTERS);
        update_monsters(c, true);
        update_players();
    }

    if (p->upkeep->update & PU_MONSTERS)
    {
        p->upkeep->update &= ~(PU_MONSTERS);
        update_monsters(c, false);
        update_players();
    }
}


/*
 * Handle "p->upkeep->update" and "p->upkeep->redraw"
 */
void handle_stuff(struct player *p)
{
    /* Delay updating */
    if (p->upkeep->new_level_method) return;

    update_stuff(p, chunk_get(&p->wpos));
    redraw_stuff(p);
}


/*
 * Handle "p->upkeep->notice", "p->upkeep->update" and "p->upkeep->redraw"
 */
void refresh_stuff(struct player *p)
{
    /* Delay updating */
    if (p->upkeep->new_level_method) return;

    /* Notice stuff */
    notice_stuff(p);

    /* Handle stuff */
    handle_stuff(p);
}


/* Monks cannot use heavy armor */
bool monk_armor_ok(struct player *p)
{
    uint16_t monk_arm_wgt = 0;
    int i;

    if (!player_has(p, PF_MARTIAL_ARTS)) return false;

    /* Weight the armor */
    for (i = 0; i < p->body.count; i++)
    {
        struct object *slot_obj = slot_object(p, i);

        /* Ignore non-armor */
        if (slot_type_is(p, i, EQUIP_WEAPON)) continue;
        if (slot_type_is(p, i, EQUIP_BOW)) continue;
        if (slot_type_is(p, i, EQUIP_RING)) continue;
        if (slot_type_is(p, i, EQUIP_AMULET)) continue;
        if (slot_type_is(p, i, EQUIP_LIGHT)) continue;
        if (slot_type_is(p, i, EQUIP_TOOL)) continue;

        /* Add weight */
        if (slot_obj) monk_arm_wgt += slot_obj->weight;
    }

    /* Little bonus for kings because of the crown (20 lbs) */
    if (p->total_winner) return (monk_arm_wgt <= 350);

    return (monk_arm_wgt <= (100 + (p->lev * 4)));
}
