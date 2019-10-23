/*
 * File: s-angband.h
 * Purpose: Main Angband header file (server)
 */

#ifndef INCLUDED_S_ANGBAND_H
#define INCLUDED_S_ANGBAND_H

/*
 * Include the low-level includes
 */
#include "../common/angband.h"

/*
 * Include the mid-level includes
 */
#include "z-quark.h"
#include "z-queue.h"

/*
 * Include the high-level includes
 */
#include "alloc.h"
#include "cave.h"
#include "channel.h"
#include "cmds.h"
#include "display-ui.h"
#include "effects.h"
#include "game-world.h"
#include "generate.h"
#include "help-ui.h"
#include "history-ui.h"
#include "house.h"
#include "init.h"
#include "map-ui.h"
#include "message.h"
#include "monster.h"
#include "mon-attack.h"
#include "mon-blow-methods.h"
#include "mon-blow-effects.h"
#include "mon-desc.h"
#include "mon-init.h"
#include "mon-list.h"
#include "mon-list-ui.h"
#include "mon-lore.h"
#include "mon-lore-ui.h"
#include "mon-make.h"
#include "mon-move.h"
#include "mon-msg.h"
#include "mon-power.h"
#include "mon-spell.h"
#include "mon-summon.h"
#include "mon-timed.h"
#include "mon-util.h"
#include "netserver.h"
#include "object.h"
#include "obj-chest.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-identify.h"
#include "obj-ignore.h"
#include "obj-info.h"
#include "obj-inscrip.h"
#include "obj-list.h"
#include "obj-list-ui.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-power.h"
#include "obj-properties.h"
#include "obj-randart.h"
#include "obj-slays.h"
#include "obj-ui.h"
#include "obj-util.h"
#include "party.h"
#include "player.h"
#include "player-attack.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-path.h"
#include "player-quest.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-ui.h"
#include "player-util.h"
#include "prefs-ui.h"
#include "project.h"
#include "savefile.h"
#include "sched-win.h"
#include "score.h"
#include "score-ui.h"
#include "store.h"
#include "s-util.h"
#include "target.h"
#include "target-ui.h"
#include "trap.h"
#include "wilderness.h"
#include "z-textblock.h"

#endif
