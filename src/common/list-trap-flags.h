/*
 * File: list-trap-flags.h
 * Purpose: Trap properties
 */

/* symbol descr */
TRF(NONE, "")
TRF(RUNE, "Is a rune")
TRF(TRAP, "Is a player trap")
TRF(VISIBLE, "Is visible")
TRF(INVISIBLE, "Is invisible")
TRF(FLOOR, "Can be set on a floor")
TRF(DOWN, "Takes the player down a level")
TRF(PIT, "Moves the player onto the trap")
TRF(ONETIME, "Disappears after being activated")
TRF(MAGICAL, "Has magical activation (absence of this flag means physical)")
TRF(SAVE_THROW, "Allows a save from all effects by standard saving throw")
TRF(SAVE_ARMOR, "Allows a save from all effects due to AC")
TRF(LOCK, "Is a door lock")
