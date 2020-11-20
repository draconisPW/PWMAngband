/*
 * File: keymap.h
 * Purpose: Keymap handling
 */

#ifndef UI_KEYMAP_H
#define UI_KEYMAP_H

/*
 * Maximum number of keypresses a trigger can map to.
 */
#define KEYMAP_ACTION_MAX 30

/*
 * Keymap modes.
 */
enum
{
    KEYMAP_MODE_ORIG = 0,
    KEYMAP_MODE_ROGUE,

    KEYMAP_MODE_MAX
};

/*
 * Given a keymap mode and a keypress, return any attached action.
 */
extern const struct keypress *keymap_find(int keymap, struct keypress kc);

/*
 * Given a keymap mode, a trigger, and an action, store it in the keymap list.
 */
extern void keymap_add(int keymap, struct keypress trigger, struct keypress *actions, bool user);

/*
 * Given a keypress, remove any keymap that would trigger on that key.
 */
extern bool keymap_remove(int keymap, struct keypress trigger);

/*
 * Free all keymaps.
 */
extern void keymap_free(void);

/*
 * Save keymaps to the specified file.
 */
extern void keymap_dump(ang_file *fff);

extern int keymap_browse(int o, int *j);
extern void command_as_keystroke(unsigned char cmd, struct keypress *kp, size_t *n);
extern unsigned char command_by_item(struct object *obj, int mode);
extern void item_as_keystroke(struct object *obj, unsigned char cmd, struct keypress *kp, size_t *n);
extern void spell_as_keystroke(int book, int spell, bool project, unsigned char cmd,
    struct keypress *kp, size_t *n);

#endif /* UI_KEYMAP_H */
