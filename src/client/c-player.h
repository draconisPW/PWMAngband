/*
 * File: c-player.h
 * Purpose: Player information (client side)
 */

#ifndef C_PLAYER_H
#define C_PLAYER_H

extern struct player *player;
extern char title[NORMAL_WID];
extern char party_info[160];
extern bool map_active;
extern int16_t last_line_info;
extern int special_line_type;
extern char special_line_header[ANGBAND_TERM_MAX][NORMAL_WID];
extern int16_t stat_roll[STAT_MAX + 1];
extern bool party_mode;
extern struct timed_grade *timed_grades[TMD_MAX];

#endif /* C_PLAYER_H */
