/*
 * File: obj-info.h
 * Purpose: Object description code.
 */

#ifndef OBJECT_INFO_H
#define OBJECT_INFO_H

/*
 * Modes for object_info()
 */
enum
{
    OINFO_NONE  = 0x00, /* No options */
    OINFO_TERSE = 0x01 /* Keep descriptions brief, e.g. for dumps */
};

extern void object_info(struct player *p, const struct object *obj, int mode);
extern void object_info_chardump(struct player *p, ang_file *f, const struct object *obj);

#endif /* OBJECT_INFO_H */
