/*
 * File: grafmode.h
 * Purpose: Load a list of possible graphics modes.
 */

#ifndef INCLUDED_GRAFMODE_H
#define INCLUDED_GRAFMODE_H

/*
 * Default graphic mode
 */
#define GRAPHICS_NONE   0

/*
 * Specifications for graphics modes.
 */
typedef struct _graphics_mode
{
    struct _graphics_mode *pNext;
    uint8_t grafID;        /* Id of tile set should be >0 and unique for anything new */
    uint8_t alphablend;    /* Whether or not the tileset needs alpha blending */
    uint8_t overdrawRow;   /* Row in the file where tiles in that row or lower draw the tile above as well */
    uint8_t overdrawMax;   /* Row in the file where tiles in that row or above draw the tile above as well */
    uint16_t cell_width;    /* Width of an individual tile in pixels */
    uint16_t cell_height;   /* Height of an individual tile in pixels */
    char path[256];     /* Path of prf file */
    char pref[32];      /* Name of prf file */
    char ppref[32];     /* Name of pprf file */
    char file[32];      /* Name of png file (if any) */
    char menuname[32];  /* Name of the tileset in menu */
} graphics_mode;

extern graphics_mode *graphics_modes;
extern graphics_mode *current_graphics_mode;
extern int graphics_mode_high_id;

extern bool init_graphics_modes(void);
extern void close_graphics_modes(void);
extern graphics_mode *get_graphics_mode(uint8_t id, bool set);
extern bool is_current_graphics_mode(uint8_t id);
extern bool is_tile_distorted(uint8_t id, uint8_t width, uint8_t height);
extern graphics_mode *get_graphics_mode_by_name(const char *name);

#endif /* INCLUDED_GRAFMODE_H */
