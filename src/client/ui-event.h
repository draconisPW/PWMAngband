/*
 * File: ui-event.h
 * Purpose: Generic input event handling
 */

#ifndef INCLUDED_UI_EVENT_H
#define INCLUDED_UI_EVENT_H

/*
 * The various UI events that can occur
 */
typedef enum
{
    EVT_NONE        = 0x0000,

    /* Basic events */
    EVT_KBRD        = 0x0001,   /* Keypress */
    EVT_RESIZE      = 0x0004,   /* Display resize */

    /* 'Abstract' events */
    EVT_ESCAPE      = 0x0010,   /* Get out of this menu */
    EVT_MOVE        = 0x0020,   /* Menu movement */
    EVT_SELECT      = 0x0040,   /* Menu selection */
    EVT_SWITCH      = 0x0080,   /* Menu switch */

    /* PWMAngband events */
    EVT_DONE        = 0x0200,   /* Done */
    EVT_ABORT       = 0x0400,   /* Abort */
    EVT_ERROR       = 0x0800,   /* Error */
    EVT_DELAY       = 0x1000    /* Excessive delay */

} ui_event_type;

/*
 * Key modifiers.
 */
#define KC_MOD_CONTROL  0x01
#define KC_MOD_SHIFT    0x02
#define KC_MOD_ALT      0x04
#define KC_MOD_META     0x08
#define KC_MOD_KEYPAD   0x10

/*
 * The game assumes that in certain cases, the effect of a modifier key will
 * be encoded in the keycode itself (e.g. 'A' is shift-'a').  In these cases
 * (specified below), a keypress' 'mods' value should not encode them also.
 *
 * If the character has come from the keypad:
 *   Include all mods
 * Else if the character is in the range 0x01-0x1F, and the keypress was
 * from a key that without modifiers would be in the range 0x40-0x5F or 0x61-0x7A:
 *   CONTROL is encoded in the keycode, and should not be in mods
 * Else if the character is in the range 0x21-0x2F, 0x3A-0x60 or 0x7B-0x7E:
 *   SHIFT is often used to produce these should not be encoded in mods
 *
 * (All ranges are inclusive.)
 *
 * You can use these macros for part of the above conditions.
 */
#define MODS_INCLUDE_CONTROL(v) \
    ((((v) >= 0x01) && ((v) <= 0x1F))? false: true)

#define MODS_INCLUDE_SHIFT(v) \
    (((((v) >= 0x21) && ((v) <= 0x60)) || \
        (((v) >= 0x7B) && ((v) <= 0x7E)))? false: true)

/*
 * If keycode you're trying to apply control to is between 0x40-0x5F
 * inclusive or 0x61-0x7A inclusive, then you should bitwise-and the keycode
 * with 0x1f and leave KC_MOD_CONTROL unset. Otherwise, leave the keycode
 * alone and set KC_MOD_CONTROL in mods.
 */
#define ENCODE_KTRL(v) \
    ((((v) >= 0x40 && (v) <= 0x5F) || ((v) >= 0x61 && (v) <= 0x7A))? true :false)

/*
 * Given a character X, turn it into a control character.
 */
#define KTRL(X) \
    ((X) & 0x1F)

/*
 * Given a control character X, turn it into its lowercase ASCII equivalent
 * unless it is 0x00 or 0x1B to 0x1F, then use the punctuation characters
 * that flank the uppercase ASCII letters.
 */
#define UN_KTRL(X) \
    (((X) < 0x01 || (X) > 0x1B)? (X) + 64 :(X) + 96)

/*
 * Given a control character X, turn it into its uppercase ASCII equivalent.
 * Prefer using UN_KTRL() over this except for inscription testing and menu
 * shortcuts where there are clashes for the rogue-like keyset (UN_KTRL()
 * for the rogue-like ignore command, '^d', gives 'd' which clashes with the
 * drop command).
 */
#define UN_KTRL_CAP(X) \
    ((X) + 64)

/*
 * Keyset mappings for various keys.
 */
#define ARROW_DOWN      0x80
#define ARROW_LEFT      0x81
#define ARROW_RIGHT     0x82
#define ARROW_UP        0x83

#define KC_F1           0x84
#define KC_F2           0x85
#define KC_F3           0x86
#define KC_F4           0x87
#define KC_F5           0x88
#define KC_F6           0x89
#define KC_F7           0x8A
#define KC_F8           0x8B
#define KC_F9           0x8C
#define KC_F10          0x8D
#define KC_F11          0x8E
#define KC_F12          0x8F
#define KC_F13          0x90
#define KC_F14          0x91
#define KC_F15          0x92

#define KC_HELP         0x93
#define KC_HOME         0x94
#define KC_PGUP         0x95
#define KC_END          0x96
#define KC_PGDOWN       0x97
#define KC_INSERT       0x98
#define KC_PAUSE        0x99
#define KC_BREAK        0x9A
#define KC_BEGIN        0x9B
#define KC_ENTER        0x9C /* ASCII \r */
#define KC_TAB          0x9D /* ASCII \t */
#define KC_DELETE       0x9E
#define KC_BACKSPACE    0x9F /* ASCII \h */

#define KC_F16          0xA0
#define KC_F17          0xA1
#define KC_F18          0xA2
#define KC_F19          0xA3
#define KC_F20          0xA4
#define KC_F21          0xA5
#define KC_F22          0xA6
#define KC_F23          0xA7
#define KC_F24          0xA8

#define ESCAPE          0xE000

/*
 * Analogous to isdigit() etc in ctypes
 */
#define isarrow(c)  ((c >= ARROW_DOWN) && (c <= ARROW_UP))

/*
 * Type capable of holding any input key we might want to use.
 */
typedef uint32_t keycode_t;

/*
 * Struct holding all relevant info for keypresses.
 */
struct keypress
{
    ui_event_type type;
    keycode_t code;
    uint8_t mods;
};

/*
 * Union type to hold information about any given event.
 */
typedef union
{
    ui_event_type type;
    struct keypress key;
} ui_event;

/*
 * Easy way to initialize a ui_event without seeing the gory bits.
 */
#define EVENT_EMPTY {EVT_NONE}

#define EVENT_ABORT {EVT_ABORT}

/* Escape event */
#define is_escape(evt) \
    ((((evt).type == EVT_KBRD) && ((evt).key.code == ESCAPE)) || ((evt).type == EVT_ESCAPE))

/* Abort event */
#define is_abort(evt) \
    (((evt).type == EVT_ERROR) || ((evt).type == EVT_ABORT) || ((evt).type == EVT_DELAY))

/* Exit event */
#define is_exit(evt) \
    (is_escape(evt) || is_abort(evt))

/* Return on abort (handle escape separately) */
#define return_on_abort(evt) \
    if (is_escape(evt)) return 2; \
    if (is_abort(evt)) return 1

/*** Functions ***/

/*
 * Given a string (and that string's length), return the corresponding keycode
 */
extern keycode_t keycode_find_code(const char *str, size_t len);

/*
 * Given a keycode, return its description
 */
extern const char *keycode_find_desc(keycode_t kc);

/*
 * Convert a textual representation of keypresses into actual keypresses
 */
extern void keypress_from_text(struct keypress *buf, size_t len, const char *str);

/*
 * Convert a string of keypresses into their textual representation
 */
extern void keypress_to_text(char *buf, size_t len, const struct keypress *src,
    bool expand_backslash);

/*
 * Convert a keypress into something the user can read (not designed to be used
 * internally)
 */
extern void keypress_to_readable(char *buf, size_t len, struct keypress src);

#endif
