/*
 * File: ui-menu.h
 * Purpose: Generic menu interaction functions
 */

#ifndef INCLUDED_UI_MENU_H
#define INCLUDED_UI_MENU_H

/*** Constants ***/

/* Colors for interactive menus */
enum
{
    CURS_UNKNOWN = 0,   /* Use gray / dark blue for cursor */
    CURS_KNOWN = 1      /* Use white / light blue for cursor */
};

/*
 * Type wrapper for various row styles.
 */
typedef enum _menu_row_style_t
{
    MN_ROW_STYLE_DISABLED = CURS_UNKNOWN,
    MN_ROW_STYLE_ENABLED = CURS_KNOWN
} menu_row_style_t;

/*
 * Type wrapper for row validity.
 */
typedef enum _menu_row_validity_t
{
    MN_ROW_INVALID = 0,
    MN_ROW_VALID = 1,
    MN_ROW_HIDDEN = 2
} menu_row_validity_t;

/* Cursor colours for different states */
extern const uint8_t curs_attrs[2][2];

/* Standard menu orderings */
extern const char lower_case[];         /* abc..z */
extern const char upper_case[];         /* ABC..Z */
extern const char all_letters[];        /* abc..zABC..Z */
extern const char all_letters_nohjkl[]; /* abc..gim..zABC..Z */

/*
  Together, these classes define the constant properties of
  the various menu classes.

  A menu consists of:
   - menu_iter, which describes how to handle the type of "list" that's 
     being displayed as a menu
   - a menu_skin, which describes the layout of the menu on the screen.
   - various bits and bobs of other data (e.g. the actual list of entries)
 */
struct menu;

/*** Predefined menu "skins" ***/

/*
 * Types of predefined skins available.
 */
typedef enum
{
    /*
     * A simple list of actions with an associated name and id.
     * Private data: an array of menu_action
     */
    MN_ITER_ACTIONS = 1,

    /*
     * A list of strings to be selected from - no associated actions.
     * Private data: an array of const char *
     */
    MN_ITER_STRINGS = 2
} menu_iter_id;

/*
 * Primitive menu item with bound action.
 */
typedef struct
{
    int flags;
    char tag;
    const char *name;
    void (*action)(const char *title, int row);
} menu_action;

/*
 * Flags for menu_actions.
 */
#define MN_ACT_GRAYED   0x0001  /* Allows selection but no action */
#define MN_ACT_HIDDEN   0x0002  /* Row is hidden, but may be selected via tag */

/*
 * Underlying function set for displaying lists in a certain kind of way.
 */
typedef struct
{
    /* Returns menu item tag (optional) */
    char (*get_tag)(struct menu *menu, int oid);

    /*
     * Validity checker (optional -- all rows are assumed valid if not present)
     * Return values will be interpreted as: 0 = no, 1 = yes, 2 = hide.
     */
    int (*valid_row)(struct menu *menu, int oid);

    /* Displays a menu row */
    void (*display_row)(struct menu *menu, int oid, bool cursor, int row, int col, int width);

    /* Handle 'positive' events (selections or cmd_keys) */
    /* XXX split out into a select handler and a cmd_key handler */
    bool (*row_handler)(struct menu *menu, const ui_event *event, int oid);

    /* Called when the screen resizes */
    void (*resize)(struct menu *m);
} menu_iter;

/*** Menu skins ***/

/*
 * Identifiers for the kind of layout to use
 */
typedef enum
{
    MN_SKIN_SCROLL = 1, /* Ordinary scrollable single-column list */
    MN_SKIN_OBJECT = 2, /* Special single-column list for object choice */
    MN_SKIN_COLUMNS = 3 /* Multicolumn view */
} skin_id;

/* Class functions for menu layout */
typedef struct
{
    /* Determines the cursor index given a (mouse) location */
    int (*get_cursor)(int row, int col, int n, int top, region *loc);

    /* Displays the current list of visible menu items */
    void (*display_list)(struct menu *menu, int cursor, int *top, region *);

    /* Specifies the relative menu item given the state of the menu */
    char (*get_tag)(struct menu *menu, int pos);

    /* Process a direction */
    ui_event (*process_dir)(struct menu *menu, int dir);
} menu_skin;

/*** Base menu structure ***/

/*
 * Flags for menu appearance & behaviour
 */
enum
{
    /* Tags are associated with the view, not the element */
    MN_REL_TAGS = 0x01,

    /* No tags -- movement key only */
    MN_NO_TAGS = 0x02,

    /* Tags to be generated by the display function */
    MN_PVT_TAGS = 0x04,

    /* Tag selections can be made regardless of the case of the key pressed. 
     * i.e. 'a' activates the line tagged 'A'. */
    MN_CASELESS_TAGS = 0x08,

    /* Double tap (or keypress) for selection; single tap is cursor movement */
    MN_DBL_TAP = 0x10,

    /* No select events to be triggered */
    MN_NO_ACTION = 0x20,

    /* Tags can be selected via an inscription */
    MN_INSCRIP_TAGS = 0x40
};

/* Base menu type */
struct menu
{
    /** Public variables **/
    const char *header;
    const char *title;
    const char *prompt;

    /* Keyboard shortcuts for menu selection -- shouldn't overlap cmd_keys */
    const char *selections;

    /* Menu selections corresponding to inscriptions */
    char *inscriptions;

    /* String of characters that when pressed, menu handler should be called */
    /* Mustn't overlap with 'selections' or some items may be unselectable */
    const char *cmd_keys;

    /* String of characters that when pressed, return an EVT_SWITCH */
    /* Mustn't overlap with 'selections' or some items may be unselectable */
    const char *switch_keys;

    /* Auxiliary browser help function */
    void (*browse_hook)(int oid, void *db, const region *loc);

    /* Flags specifying the behavior of this menu */
    int flags;

    /** Private variables **/

    /* Stored boundary, set by menu_layout().  This is used to calculate
     * where the menu should be displayed on display & resize */
    region boundary;

    int filter_count;        /* number of rows in current view */
    const int *filter_list;  /* optional filter (view) of menu objects */

    int count;               /* number of rows in underlying data set */
    void *menu_data;         /* the data used to access rows. */

    const menu_skin *skin;      /* menu display style functions */
    const menu_iter *row_funcs; /* menu skin functions */

    /* State variables */
    int cursor;             /* Currently selected row */
    int top;                /* Position in list for partial display */
    region active;          /* Subregion actually active for selection */
    int cursor_x_offset;    /* Adjustment to the default position of the cursor on a line. */
};

/*** Menu API ***/

/*
 * Allocate and return a new, initialised, menu.
 */
extern struct menu *menu_new(skin_id id, const menu_iter *iter);
extern struct menu *menu_new_action(menu_action *acts, size_t n);

/*
 * Initialize a menu, using the skin and iter functions specified.
 */
extern void menu_init(struct menu *menu, skin_id id, const menu_iter *iter);

/*
 * Given a predefined menu kind, return its iter functions.
 */
extern const menu_iter *menu_find_iter(menu_iter_id iter_id);

/*
 * Set menu private data and the number of menu items.
 *
 * Menu private data is then available from inside menu callbacks using
 * menu_priv().
 */
extern void menu_setpriv(struct menu *menu, int count, void *data);

/*
 * Return menu private data, set with menu_setpriv().
 */
extern void *menu_priv(struct menu *menu);

/*
 * Set a filter on what items a menu can display.
 *
 * Use this if your menu private data has 100 items, but you want to choose
 * which ones of those to display at any given time, e.g. in an inventory menu.
 * object_list[] should be an array of indexes to display, and n should be its
 * length.
 */
extern void menu_set_filter(struct menu *menu, const int object_list[], int n);

/*
 * Remove any filters set on a menu by menu_set_filer().
 */
extern void menu_release_filter(struct menu *menu);

/*
 * Ready a menu for display in the region specified.
 *
 * XXX not ready for dynamic resizing just yet
 */
extern bool menu_layout(struct menu *menu, const region *loc);

/*
 * Display a menu.
 * If reset_screen is true, it will reset the screen to the previously saved
 * state before displaying.
 */
extern void menu_refresh(struct menu *menu, bool reset_screen);

/*
 * Run a menu.
 *
 * 'notify' is a bitwise OR of ui_event_type events that you want to
 * menu_select to return to you if they're not handled inside the menu loop.
 * e.g. if you want to handle key events without specifying a menu_iter->handle
 * function, you can set notify to EVT_KBRD, and any non-navigation keyboard
 * events will stop the menu loop and return them to you.
 *
 * Some events are returned by default, and else are EVT_ESCAPE and EVT_SELECT.
 * 
 * Event types that can be returned:
 *   EVT_ESCAPE: no selection; go back (by default)
 *   EVT_SELECT: menu->cursor is the selected menu item (by default)
 *   EVT_MOVE:   the cursor has moved
 *   EVT_KBRD:   unhandled keyboard events
 *   EVT_RESIZE: resize events
 *
 * XXX remove 'notify'
 *
 * If popup is true, the screen background is saved before starting the menu,
 * and restored before each redraw. This allows variably-sized information
 * at the bottom of the menu.
 */
extern ui_event menu_select(struct menu *menu, int notify, bool popup);

/*
 * Set the menu cursor to the next valid row.
 */
extern void menu_ensure_cursor_valid(struct menu *m);

/* Internal menu stuff */
extern bool menu_handle_keypress(struct menu *menu, const ui_event *in, ui_event *out);

/*
 * Allow adjustment of the cursor's default x offset.
 */
extern void menu_set_cursor_x_offset(struct menu *m, int offset);

/*** Dynamic menu handling ***/

extern struct menu *menu_dynamic_new(void);
extern void menu_dynamic_add(struct menu *m, const char *text, int value);
extern void menu_dynamic_add_valid(struct menu *m, const char *text, int value,
    menu_row_validity_t valid);
extern void menu_dynamic_add_label(struct menu *m, const char *text, const char label, int value,
    char *label_list);
extern void menu_dynamic_add_label_valid(struct menu *m, const char *text, const char label,
    int value, char *label_list, menu_row_validity_t valid);
extern size_t menu_dynamic_longest_entry(struct menu *m);
extern void menu_dynamic_calc_location(struct menu *m);
extern int menu_dynamic_select(struct menu *m);
extern void menu_dynamic_free(struct menu *m);


#endif /* INCLUDED_UI_MENU_H */
