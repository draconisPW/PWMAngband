/*
 * File: list-mon-message.h
 * Purpose: List of monster message types
 *
 * id            - the message constant name
 * msg           - MSG_ type for the printed message
 * omit_subject  - true to omit a monster name beforehand (see mon-msg.c)
 * text          - the message text
 */

/* Dummy action */
MON_MSG(NONE, MSG_GENERIC, false, "[is|are] hurt.")
/* From project_m */
MON_MSG(DIE, MSG_KILL, false, "die[s].")
MON_MSG(DESTROYED, MSG_KILL, false, "[is|are] destroyed.")
MON_MSG(RESIST_A_LOT, MSG_GENERIC, false, "resist[s] a lot.")
MON_MSG(HIT_HARD, MSG_GENERIC, false, "[is|are] hit hard.")
MON_MSG(RESIST, MSG_GENERIC, false, "resist[s].")
MON_MSG(IMMUNE, MSG_GENERIC, false, "[is|are] immune.")
MON_MSG(RESIST_SOMEWHAT, MSG_GENERIC, false, "resist[s] somewhat.")
MON_MSG(UNAFFECTED, MSG_GENERIC, false, "[is|are] unaffected!")
MON_MSG(SPAWN, MSG_GENERIC, false, "spawn[s]!")
MON_MSG(HEALTHIER, MSG_GENERIC, false, "look[s] healthier.")
MON_MSG(FALL_ASLEEP, MSG_GENERIC, false, "fall[s] asleep!")
MON_MSG(WAKES_UP, MSG_GENERIC, false, "wake[s] up.")
MON_MSG(CRINGE_LIGHT, MSG_GENERIC, false, "cringe[s] from the light!")
MON_MSG(SHRIVEL_LIGHT, MSG_KILL, false, "shrivel[s] away in the light!")
MON_MSG(LOSE_SKIN, MSG_GENERIC, false, "lose[s] some skin!")
MON_MSG(DISSOLVE, MSG_KILL, false, "dissolve[s]!")
MON_MSG(CATCH_FIRE, MSG_GENERIC, false, "catch[es] fire!")
MON_MSG(BADLY_FROZEN, MSG_GENERIC, false, "[is|are] badly frozen.")
MON_MSG(SHUDDER, MSG_GENERIC, false, "shudder[s].")
MON_MSG(CHANGE, MSG_GENERIC, false, "change[s]!")
MON_MSG(DISAPPEAR, MSG_GENERIC, false, "disappear[s]!")
MON_MSG(MORE_DAZED, MSG_GENERIC, false, "[is|are] even more stunned.")
MON_MSG(DAZED, MSG_GENERIC, false, "[is|are] stunned.")
MON_MSG(NOT_DAZED, MSG_GENERIC, false, "[is|are] no longer stunned.")
MON_MSG(MORE_CONFUSED, MSG_GENERIC, false, "look[s] more confused.")
MON_MSG(CONFUSED, MSG_GENERIC, false, "look[s] confused.")
MON_MSG(NOT_CONFUSED, MSG_GENERIC, false, "[is|are] no longer confused.")
MON_MSG(MORE_SLOWED, MSG_GENERIC, false, "look[s] more slowed.")
MON_MSG(SLOWED, MSG_GENERIC, false, "look[s] slowed.")
MON_MSG(NOT_SLOWED, MSG_GENERIC, false, "speed[s] up.")
MON_MSG(MORE_HASTED, MSG_GENERIC, false, "look[s] even faster!")
MON_MSG(HASTED, MSG_GENERIC, false, "start[s] moving faster.")
MON_MSG(NOT_HASTED, MSG_GENERIC, false, "slow[s] down.")
MON_MSG(MORE_AFRAID, MSG_GENERIC, false, "look[s] more terrified!")
MON_MSG(FLEE_IN_TERROR, MSG_GENERIC, false, "flee[s] in terror!")
MON_MSG(NOT_AFRAID, MSG_GENERIC, false, "[is|are] no longer afraid.")
MON_MSG(HELD, MSG_GENERIC, false, "[is|are] frozen to the spot!")
MON_MSG(NOT_HELD, MSG_GENERIC, false, "can move again.")
MON_MSG(MORIA_DEATH, MSG_KILL, true, "You hear [a|several] scream[|s] of agony!")
MON_MSG(DISINTEGRATES, MSG_KILL, false, "disintegrate[s]!")
MON_MSG(FREEZE_SHATTER, MSG_KILL, false, "freeze[s] and shatter[s]!")
MON_MSG(MANA_DRAIN, MSG_GENERIC, false, "lose[s] some mana!")
MON_MSG(BRIEF_PUZZLE, MSG_GENERIC, false, "look[s] briefly puzzled.")
MON_MSG(MAINTAIN_SHAPE, MSG_GENERIC, false, "maintain[s] the same shape.")
/* From message_pain */
MON_MSG(UNHARMED, MSG_GENERIC, false, "[is|are] unharmed.")
/* Dummy messages for monster pain - we use edit file info instead. */
MON_MSG(95, MSG_GENERIC, false, "")
MON_MSG(75, MSG_GENERIC, false, "")
MON_MSG(50, MSG_GENERIC, false, "")
MON_MSG(35, MSG_GENERIC, false, "")
MON_MSG(20, MSG_GENERIC, false, "")
MON_MSG(10, MSG_GENERIC, false, "")
MON_MSG(0, MSG_GENERIC, false, "")
/* PWMAngband */
MON_MSG(BLEED, MSG_GENERIC, false, "bleed[s] from open wounds!")
MON_MSG(NOT_BLEEDING, MSG_GENERIC, false, "[is|are] no longer bleeding.")
MON_MSG(MORE_BLEEDING, MSG_GENERIC, false, "bleed[s] even more.")
MON_MSG(POISONED, MSG_GENERIC, false, "[is|are] poisoned!")
MON_MSG(NOT_POISONED, MSG_GENERIC, false, "[is|are] no longer poisoned.")
MON_MSG(MORE_POISONED, MSG_GENERIC, false, "look[s] more poisoned.")
MON_MSG(BLIND, MSG_GENERIC, false, "grope[s] around blindly!")
MON_MSG(NOT_BLIND, MSG_GENERIC, false, "[is|are] no longer blind.")
MON_MSG(MORE_BLIND, MSG_GENERIC, false, "look[s] more blind.")
MON_MSG(DROP_DEAD, MSG_KILL, false, "drop[s] dead.")
MON_MSG(HATE, MSG_GENERIC, false, "hate[s] you too much!")
MON_MSG(REACT, MSG_GENERIC, false, "react[s] to your order.")
MON_MSG(RETURN, MSG_GENERIC, false, "return[s]!")
MON_MSG(TORN, MSG_GENERIC, false, "[is|are] torn apart by gravity.")
MON_MSG(ICE, MSG_GENERIC, false, "[is|are] blasted by shards of ice.")
MON_MSG(EMBEDDED, MSG_GENERIC, false, "[is|are] embedded in the rock!")
MON_MSG(WAIL, MSG_GENERIC, false, "wail[s] out in pain!")
MON_MSG(CROAK, MSG_GENERIC, false, "croak[s] in agony.")
MON_MSG(BURN, MSG_KILL, false, "[is|are] burnt up.")
MON_MSG(DROWN, MSG_KILL, false, "drown[s].")
MON_MSG(DRAIN, MSG_KILL, false, "[is|are] drained.")
