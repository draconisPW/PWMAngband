/*
 * File: trap-common.h
 * Purpose: Flags, structures and variables for traps
 */

#ifndef INCLUDED_TRAP_COMMON_H
#define INCLUDED_TRAP_COMMON_H

/*** Trap flags ***/

enum
{
    #define TRF(a,b) TRF_##a,
    #include "list-trap-flags.h"
    #undef TRF
    TRF_MAX
};

#define TRF_SIZE                FLAG_SIZE(TRF_MAX)

#define trf_has(f, flag)        flag_has_dbg(f, TRF_SIZE, flag, #f, #flag)
#define trf_next(f, flag)       flag_next(f, TRF_SIZE, flag)
#define trf_is_empty(f)         flag_is_empty(f, TRF_SIZE)
#define trf_is_full(f)          flag_is_full(f, TRF_SIZE)
#define trf_is_inter(f1, f2)    flag_is_inter(f1, f2, TRF_SIZE)
#define trf_is_subset(f1, f2)   flag_is_subset(f1, f2, TRF_SIZE)
#define trf_is_equal(f1, f2)    flag_is_equal(f1, f2, TRF_SIZE)
#define trf_on(f, flag)         flag_on_dbg(f, TRF_SIZE, flag, #f, #flag)
#define trf_off(f, flag)        flag_off(f, TRF_SIZE, flag)
#define trf_wipe(f)             flag_wipe(f, TRF_SIZE)
#define trf_setall(f)           flag_setall(f, TRF_SIZE)
#define trf_negate(f)           flag_negate(f, TRF_SIZE)
#define trf_copy(f1, f2)        flag_copy(f1, f2, TRF_SIZE)
#define trf_union(f1, f2)       flag_union(f1, f2, TRF_SIZE)
#define trf_inter(f1, f2)       flag_inter(f1, f2, TRF_SIZE)
#define trf_diff(f1, f2)        flag_diff(f1, f2, TRF_SIZE)

/*
 * A trap template.
 */
struct trap_kind
{
    char *name;                     /* Name */
    char *text;                     /* Text */
    char *desc;                     /* Short description */
    char *msg;                      /* Message on hitting */
    char *msg_good;                 /* Message on saving */
    char *msg_bad;                  /* Message on failing to save */
    char *msg_xtra;                 /* Message on getting an extra effect */

    struct trap_kind *next;
    unsigned int tidx;              /* Trap kind index */

    byte d_attr;                    /* Default trap attribute */
    char d_char;                    /* Default trap character */

    int rarity;                     /* Rarity */
    int min_depth;                  /* Minimum depth */
    int max_num;                    /* Unused */
    random_value power;             /* Visibility of player trap */

    bitflag flags[TRF_SIZE];        /* Trap flags (all traps of this kind) */
    bitflag save_flags[OF_SIZE];    /* Save flags (player with these saves) */

    struct effect *effect;          /* Effect on entry to grid */
    struct effect *effect_xtra;     /* Possible extra effect */

    /* PWMAngband */
    int msg_death_type;             /* Type of death message */
    char *msg_death;                /* Message on death */
};

extern struct trap_kind *trap_info;

/*
 * An actual trap.
 */
struct trap
{
    struct trap_kind *kind;     /* Trap kind */
    struct trap *next;          /* Next trap in this location */

    byte fy;                    /* Location of trap */
    byte fx;

    byte power;                 /* Power for door locks, visibility for traps */
    byte timeout;               /* Timer for disabled traps */

    bitflag flags[TRF_SIZE];    /* Trap flags (only this particular trap) */
};

#endif /* INCLUDED_TRAP_COMMON_H */
