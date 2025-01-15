/*
 * File: generate.h
 * Purpose: Dungeon generation interface
 */

#ifndef GENERATE_H
#define GENERATE_H

/*
 * Dungeon allocation places and types, used with alloc_object().
 */
enum
{
    SET_CORR = 0x01,    /* Hallway */
    SET_ROOM = 0x02,    /* Room */
    SET_BOTH = 0x03     /* Anywhere */
};

enum
{
    TYP_RUBBLE,     /* Rubble */
    TYP_FOUNTAIN,   /* Fountain */
    TYP_TRAP,       /* Trap */
    TYP_GOLD,       /* Gold */
    TYP_OBJECT,     /* Object */
    TYP_GOOD,       /* Good object */
    TYP_GREAT       /* Great object */
};

/*
 * Flag for room types
 */
enum
{
    #define ROOMF(a, b) ROOMF_##a,
    #include "list-room-flags.h"
    #undef ROOMF
    ROOMF_MAX
};

#define ROOMF_SIZE              FLAG_SIZE(ROOMF_MAX)

#define roomf_has(f, flag)      flag_has_dbg(f, ROOMF_SIZE, flag, #f, #flag)
#define roomf_next(f, flag)     flag_next(f, ROOMF_SIZE, flag)
#define roomf_is_empty(f)       flag_is_empty(f, ROOMF_SIZE)
#define roomf_is_full(f)        flag_is_full(f, ROOMF_SIZE)
#define roomf_is_inter(f1, f2)  flag_is_inter(f1, f2, ROOMF_SIZE)
#define roomf_is_subset(f1, f2) flag_is_subset(f1, f2, ROOMF_SIZE)
#define roomf_is_equal(f1, f2)  flag_is_equal(f1, f2, ROOMF_SIZE)
#define roomf_on(f, flag)       flag_on_dbg(f, ROOMF_SIZE, flag, #f, #flag)
#define roomf_off(f, flag)      flag_off(f, ROOMF_SIZE, flag)
#define roomf_wipe(f)           flag_wipe(f, ROOMF_SIZE)
#define roomf_setall(f)         flag_setall(f, ROOMF_SIZE)
#define roomf_negate(f)         flag_negate(f, ROOMF_SIZE)
#define roomf_copy(f1, f2)      flag_copy(f1, f2, ROOMF_SIZE)
#define roomf_union(f1, f2)     flag_union(f1, f2, ROOMF_SIZE)
#define roomf_inter(f1, f2)     flag_inter(f1, f2, ROOMF_SIZE)
#define roomf_diff(f1, f2)      flag_diff(f1, f2, ROOMF_SIZE)

/*
 * Profile indexes
 */
enum
{
    #define DUN(a, b) dun_##b,
    #include "list-dun-profiles.h"
    #undef DUN
    dun_max
};

/*
 * Monster base for a pit
 */
struct pit_monster_profile
{
     struct pit_monster_profile *next;
     struct monster_base *base;
};

/*
 * Monster color for a pit
 */
struct pit_color_profile
{
     struct pit_color_profile *next;
     uint8_t color;
};

/*
 * Monster forbidden from a pit
 */
struct pit_forbidden_monster
{
    struct pit_forbidden_monster *next;
    struct monster_race *race;
};

/*
 * Profile for choosing monsters for pits, nests or other themed areas
 */
struct pit_profile
{
    struct pit_profile *next;                           /* Pointer to next pit profile */
    unsigned int pit_idx;                               /* Index in pit_info */
    char *name;
    int room_type;                                      /* Is this a pit or a nest? */
    int ave;                                            /* Level where this pit is most common */
    int rarity;                                         /* How unusual this pit is */
    int obj_rarity;                                     /* How rare objects are in this pit */
    bitflag flags[RF_SIZE];                             /* Required flags */
    bitflag forbidden_flags[RF_SIZE];                   /* Forbidden flags */
    int freq_spell;                                     /* Minimum spell frequency */
    bitflag spell_flags[RSF_SIZE];                      /* Required spell flags */
    bitflag forbidden_spell_flags[RSF_SIZE];            /* Forbidden spell flags */
    struct pit_monster_profile *bases;                  /* List of valid monster bases */
    struct pit_color_profile *colors;                   /* List of valid monster colors */
    struct pit_forbidden_monster *forbidden_monsters;   /* Forbidden monsters */
};

extern struct pit_profile *pit_info;

/*
 * Structure to hold all "dungeon generation" data
 */
struct dun_data
{
    /* The profile used to generate the level */
    const struct cave_profile *profile;

    /* Array of centers of rooms */
    int cent_n;
    struct loc *cent;

    /* Array (cent_n elements) for counts of marked entrance points */
    int *ent_n;

    /* Array of arrays (cent_n by ent_n[i]) for locations of marked entrance points */
    struct loc **ent;

    /* Lookup for room number of a room entrance by (y,x) for the entrance */
    int **ent2room;

    /* Array of possible door locations */
    int door_n;
    struct loc *door;

    /* Array of wall piercing locations */
    int wall_n;
    struct loc *wall;

    /* Array of tunnel grids */
    int tunn_n;
    struct loc *tunn;
    uint8_t *tunn_flag;

    /* Number of grids in each block (vertically) */
    int block_hgt;

    /* Number of grids in each block (horizontally) */
    int block_wid;

    /* Number of blocks along each axis */
    int row_blocks;
    int col_blocks;

    /* Array of which blocks are used */
    bool **room_map;

    /* Number of pits/nests on the level */
    int pit_num;

    /* Current pit profile in use */
    struct pit_profile *pit_type;

    /*  Whether or not this is a quest level */
    bool quest;
};

struct tunnel_profile
{
    const char *name;
    int rnd;    /* % chance of choosing random direction */
    int chg;    /* % chance of changing direction */
    int con;    /* % chance of extra tunneling */
    int pen;    /* % chance of placing doors at room entrances */
    int jct;    /* % chance of doors at tunnel junctions */
};

struct streamer_profile
{
    const char *name;
    int den;    /* Density of streamers */
    int rng;    /* Width of streamers */
    int mag;    /* Number of magma streamers */
    int mc;     /* 1/chance of treasure per magma */
    int qua;    /* Number of quartz streamers */
    int qc;     /* 1/chance of treasure per quartz */
};

/*
 * cave_builder is a function pointer which builds a level
 */
typedef struct chunk * (*cave_builder) (struct player *p, struct worldpos *wpos, int h, int w,
    const char **p_error);

struct cave_profile
{
    struct cave_profile *next;

    char *name;
    cave_builder builder;               /* Function used to build the level */
    int block_size;                     /* Default height and width of dungeon blocks */
    int dun_rooms;                      /* Number of rooms to attempt */
    int dun_unusual;                    /* Level/chance of unusual room */
    int max_rarity;                     /* Max number of rarity levels used in room generation */
    int n_room_profiles;                /* Number of room profiles */
    struct tunnel_profile tun;          /* Used to build tunnels */
    struct streamer_profile str;        /* Used to build mineral streamers */
    struct room_profile *room_profiles; /* Used to build rooms */
    int alloc;                          /* Used to see if we should try this dungeon */
    int min_level;                      /* Shallowest level to use this profile */
    random_value up;
    random_value down;
};

/*
 * room_builder is a function pointer which builds rooms in the cave given
 * anchor coordinates.
 */
typedef bool (*room_builder) (struct player *p, struct chunk *c, struct loc *centre, int rating);

/*
 * This tracks information needed to generate the room, including the room's
 * name and the function used to build it.
 */
struct room_profile
{
    struct room_profile *next;

    char *name;
    room_builder builder;   /* Function used to build fixed size rooms */
    int rating;             /* Extra control for template rooms */
    int height, width;      /* Space required in grids */
    int level;              /* Minimum dungeon level */
    bool pit;               /* Whether this room is a pit/nest or not */
    int rarity;             /* How unusual this room is */
    int cutoff;             /* Upper limit of 1-100 random roll for room generation */
};

/*
 * Information about "vault generation"
 */
struct vault
{
    char *name;                 /* Name */
    char *text;                 /* Text */
    struct vault *next;         /* Pointer to next vault template */
    char *typ;                  /* Vault type */
    bitflag flags[ROOMF_SIZE];  /* Vault flags */
    uint8_t rat;                   /* Vault rating */
    uint8_t hgt;                   /* Vault height */
    uint8_t wid;                   /* Vault width */
    uint8_t min_lev;               /* Minimum allowable level, if specified. */
    uint8_t max_lev;               /* Maximum allowable level, if specified. */
};

/*
 * Information about "room generation"
 */
struct room_template
{
    char *name;                 /* Name */
    char *text;                 /* Text */
    bitflag flags[ROOMF_SIZE];  /* Room flags */
    struct room_template *next; /* Pointer to next room template */
    uint8_t typ;                   /* Room type */
    uint8_t rat;                   /* Room rating */
    uint8_t hgt;                   /* Room height */
    uint8_t wid;                   /* Room width */
    uint8_t dor;                   /* Random door options */
    uint16_t tval;                  /* tval for objects in this room */
};

extern struct dun_data *dun;
extern struct vault *vaults;
extern struct room_template *room_templates;

/* gen-cave.c */
extern struct chunk *classic_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *labyrinth_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *cavern_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *town_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *modified_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *moria_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *hard_centre_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *lair_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *gauntlet_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *mang_town_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);
extern struct chunk *arena_gen(struct player *p, struct worldpos *wpos, int min_height,
    int min_width, const char **p_error);

/* gen-chunk.c */
extern void chunk_list_add(struct chunk *c);
extern void chunk_list_remove(struct chunk *c);
extern void chunk_validate_objects(struct chunk *c);
extern struct chunk *chunk_get(struct worldpos *wpos);
extern bool chunk_inhibit_players(struct worldpos *wpos);
extern void chunk_decrease_player_count(struct worldpos *wpos);
extern void chunk_set_player_count(struct worldpos *wpos, int16_t value);
extern void chunk_increase_player_count(struct worldpos *wpos);
extern bool chunk_has_players(struct worldpos *wpos);
extern int16_t chunk_get_player_count(struct worldpos *wpos);

/* gen-monster.c */
extern bool mon_restrict(const char *monster_type, int depth, int current_depth, bool unique_ok);
extern void spread_monsters(struct player *p, struct chunk *c, const char *type, int depth, int num,
    int y0, int x0, int dy, int dx, uint8_t origin);
extern void get_vault_monsters(struct player *p, struct chunk *c, char racial_symbol[],
    char *vault_type, const char *data, int y1, int y2, int x1, int x2);
extern void get_chamber_monsters(struct player *p, struct chunk *c, int y1, int x1, int y2, int x2,
    char *name, int area);

/* gen-room.c */
extern void fill_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat, int flag);
extern struct vault *random_vault(int depth, const char *typ);
extern void generate_mark(struct chunk *c, int y1, int x1, int y2, int x2, int flag);
extern void generate_unmark(struct chunk *c, int y1, int x1, int y2, int x2, int flag);
extern void draw_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat, int flag,
    bool overwrite_perm);
extern void set_marked_granite(struct chunk *c, struct loc *grid, int flag);
extern bool generate_starburst_room(struct chunk *c, int y1, int x1, int y2, int x2, bool light,
    int feat, bool special_ok);
extern bool mon_pit_hook(struct monster_race *race);
extern void set_pit_type(int depth, int type);
extern bool build_vault(struct player *p, struct chunk *c, struct loc *centre, struct vault *v,
    bool find);
extern bool build_circular(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_simple(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_overlap(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_crossed(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_large(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_nest(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_pit(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_template(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_interesting(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_lesser_vault(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_lesser_new_vault(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_medium_vault(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_medium_new_vault(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_greater_vault(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_greater_new_vault(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_moria(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_room_of_chambers(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool build_huge(struct player *p, struct chunk *c, struct loc *centre, int rating);
extern bool room_build(struct player *p, struct chunk *c, int by0, int bx0,
    struct room_profile profile, bool finds_own_space);

/* gen-util.c */
extern uint8_t get_angle_to_grid[41][41];

extern int grid_to_i(struct loc *grid, int w);
extern void i_to_grid(int i, int w, struct loc *grid);
extern void shuffle(int *arr, int n);
extern int *cave_find_init(struct loc *top_left, struct loc *bottom_right);
extern void cave_find_reset(int *state);
extern bool cave_find_get_grid(struct loc *grid, int *state);
extern bool cave_find_in_range(struct chunk *c, struct loc *grid, struct loc *top_left,
    struct loc *bottom_right, square_predicate pred);
extern bool cave_find(struct chunk *c, struct loc *grid, square_predicate pred);
extern bool find_empty(struct chunk *c, struct loc *grid);
extern bool find_emptywater(struct chunk *c, struct loc *grid);
extern bool find_training(struct chunk *c, struct loc *grid);
extern bool find_nearby_grid(struct chunk *c, struct loc *grid, struct loc *centre, int yd, int xd);
extern void correct_dir(struct loc *offset, struct loc *grid1, struct loc *grid2);
extern void rand_dir(struct loc *offset);
extern bool find_start(struct chunk *c, struct loc *grid);
extern void add_down_stairs(struct chunk *c);
extern bool new_player_spot(struct chunk *c, struct player *p);
extern void place_stairs(struct chunk *c, struct loc *grid, int feat);
extern void place_random_stairs(struct chunk *c, struct loc *grid);
extern void place_object(struct player *p, struct chunk *c, struct loc *grid, int level,
    bool good, bool great, uint8_t origin, int tval);
extern void place_gold(struct player *p, struct chunk *c, struct loc *grid, int level, uint8_t origin);
extern void place_secret_door(struct chunk *c, struct loc *grid);
extern void place_closed_door(struct chunk *c, struct loc *grid);
extern void place_random_door(struct chunk *c, struct loc *grid);
extern int alloc_stairs(struct chunk *c, int feat, int num, int minsep);
extern void vault_objects(struct player *p, struct chunk *c, struct loc *grid, int num);
extern void vault_traps(struct chunk *c, struct loc *grid, int yd, int xd, int num);
extern void vault_monsters(struct player *p, struct chunk *c, struct loc *grid, int depth,
    int num);

extern void uncreate_artifacts(struct chunk *c);
extern void dump_level_simple(const char *basefilename, const char *title, struct chunk *c);
extern void dump_level(ang_file *fo, const char *title, struct chunk *c, int **dist);
extern void dump_level_header(ang_file *fo, const char *title);
extern void dump_level_body(ang_file *fo, const char *title, struct chunk *c, int **dist);
extern void dump_level_footer(ang_file *fo);

extern int alloc_objects(struct player *p, struct chunk *c, int set, int typ, int num,
    int depth, uint8_t origin);
extern bool alloc_object(struct player *p, struct chunk *c, int set, int typ, int depth,
    uint8_t origin);

/* generate.c */
extern void cave_wipe(struct chunk *c);
extern bool allow_location(struct monster_race *race, struct worldpos *wpos);
extern struct chunk *prepare_next_level(struct player *p);
extern void player_place_feeling(struct player *p, struct chunk *c);

#endif /* GENERATE_H */
