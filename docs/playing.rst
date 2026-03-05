Playing the Game
================

Most of your interaction with PWMAngband will take the form of "commands".
Every PWMAngband command consists of an "underlying command" plus a variety of
optional or required arguments, such as a repeat count, a direction, or the
index of an inventory object. Commands are normally specified by typing a
series of keypresses, from which the underlying command is extracted, along
with any encoded arguments. You may choose how the standard "keyboard keys"
are mapped to the "underlying commands" by choosing one of the two standard
"keysets", the "original" keyset or the "roguelike" keyset.

The original keyset is very similar to the "underlying" command set, with a
few additions (such as the ability to use the numeric "directions" to
"walk" or the ``5`` key to "stay still"). The roguelike keyset provides
similar additions, and also allows the use of the
``h``/``j``/``k``/``l``/``y``/``u``/``b``/``n`` keys to "walk" (or, in
combination with the shift or control keys, to run or alter), which thus
requires a variety of key mappings to allow access to the underlying
commands used for walking/running/altering. In particular, the "roguelike"
keyset includes many more "capital" and "control" keys, as shown below.

Note that any keys that are not required for access to the underlying
command set may be used by the user to extend the "keyset" which is being
used, by defining new "keymaps". To avoid the use of any "keymaps", press
backslash (``\``) plus the "underlying command" key. You may enter
"control-keys" as a caret (``^``) plus the key (so ``^`` + ``p`` yields
'^p').

Some commands allow an optional "repeat count", which allows you to tell
the game that you wish to do the command multiple times, unless you press a
key or are otherwise disturbed. In PWMAngband, such commands are set on
auto-repeat and a repeat count of 99 is normally applied.

Some commands will prompt for extra information, such as a direction, an
inventory or equipment item, a spell, a textual inscription, the symbol of
a monster race, a sub-command, a verification, an amount of time, a
quantity, a file name, or various other things. Normally you can hit return
to choose the "default" response, or escape to cancel the command entirely.

Some commands will prompt for a spell or an inventory item. Pressing space
(or ``*``) will give you a list of choices. Pressing ``-`` (minus) selects
the item on the floor. Pressing a lowercase letter selects the given item.
Pressing a capital letter selects the given item after verification.
Pressing a numeric digit ``#`` selects the first item (if any) whose
inscription contains '@#' or '@x#', where ``x`` is the current
"underlying command". You may only specify items which are "legal" for the
command. Whenever an item inscription contains '!*' or '!x' (with ``x``
as above) you must verify its selection.

Some commands will prompt for a direction. You may enter a "compass"
direction using any of the "direction keys" shown below. Sometimes, you may
specify that you wish to use the current "target", by pressing ``t`` or
``5``, or that you wish to select a new target, by pressing ``*`` (see
"Target" below).

        Original Keyset Directions
                 =  =  =
                 7  8  9
                 4     6
                 1  2  3
                 =  =  =

        Roguelike Keyset Directions
                 =  =  =
                 y  k  u
                 h     l
                 b  j  n
                 =  =  =

Each of the standard keysets provides some short-cuts over the "underlying
commands". For example, both keysets allow you to "walk" by simply pressing
an "original" direction key (or a "roguelike" direction key if you are
using the roguelike keyset), instead of using the "walk" command plus a
direction. The roguelike keyset allows you to "run" or "alter" by simply
holding the shift or control modifier key down while pressing a "roguelike"
direction key, instead of using the "run" or "alter" command plus a
direction. Both keysets allow the use of the ``5`` key to "stand still",
which is most convenient when using the original keyset.

Original Keyset Command Summary
-------------------------------

====== ============================= ====== ============================
``a``  Aim a wand                    ``A``  Activate an object
``b``  Browse a book                 ``B``  (unused)
``c``  Close a door                  ``C``  Display character sheet
``d``  Drop an item                  ``D``  Disarm a trap or lock a door
``e``  List equipped items           ``E``  Eat some food
``f``  Fire an item                  ``F``  Fuel your lantern/torch
``g``  Get objects on floor          ``G``  Gain new spells/prayers
``h``  Fire default ammo at target   ``H``  Buy a house
``i``  List contents of pack         ``I``  Inspect an item
``j``  (unused)                      ``J``  (unused)
``k``  Ignore an item                ``K``  Toggle ignore
``l``  Look around                   ``L``  Locate player on map
``m``  Cast a spell                  ``M``  Display map of entire level
``n``  (unused)                      ``N``  (unused)
``o``  Open a door or chest          ``O``  (unused)
``p``  Project a spell               ``P``  Access party menu
``q``  Quaff a potion                ``Q``  Retire character & quit
``r``  Read a scroll                 ``R``  Rest for a period
``s``  Steal                         ``S``  Toggle stealth mode
``t``  Take off equipment            ``T``  Dig a tunnel
``u``  Use a staff                   ``U``  Use an item
``v``  Throw an item                 ``V``  Polymorph into a monster
``w``  Wear/wield equipment          ``W``  Walk into a trap
``x``  Swap equipment                ``X``  (unused)
``y``  Use dragon breath attack      ``Y``  (unused)
``z``  Zap a rod                     ``Z``  (unused)
``!``  (unused)                      ``^a`` Do autopickup
``@``  Display connected players     ``^b`` (unused)
``#``  See abilities                 ``^c`` Retire character & quit
``$``  Drop gold                     ``^d`` Describe object
``%``  Interact with keymaps         ``^e`` Toggle inven/equip window
``^``  (special - control key)       ``^f`` Repeat level feeling
``&``  Enter Dungeon Master menu     ``^g`` (special - bell)
``*``  Target monster or location    ``^h`` (unused)
``(``  Target friendly player        ``^i`` (special - tab)
``)``  Dump screen to a file         ``^j`` (special - linefeed)
``{``  Inscribe an object            ``^k`` (unused)
``}``  Uninscribe an object          ``^l`` Center map
``[``  Display visible monster list  ``^m`` (special - return)
``]``  Display visible object list   ``^n`` (unused)
``-``  (unused)                      ``^o`` Show previous message
``_``  Drink/fill an empty bottle    ``^p`` Show previous messages
``+``  Alter grid                    ``^q`` Get a quest
``=``  Set options                   ``^r`` Redraw the screen
``;``  Walk (with pickup)            ``^s`` Socials
``:``  Enter chat mode               ``^t`` (unused)
``'``  Target closest monster        ``^u`` (unused)
``"``  Enter a user pref command     ``^v`` (unused)
``,``  Stay still (with pickup)      ``^w`` Full wilderness map
``<``  Go up staircase               ``^x`` Save and quit
``.``  Run                           ``^y`` (unused)
``>``  Go down staircase             ``^z`` Use chat command
``\``  (special - bypass keymap)     ``~``  Check knowledge
  \`   (special - escape)            ``?``  Display help
``/``  Identify monster              ``|``  List contents of quiver
====== ============================= ====== ============================

Roguelike Keyset Command Summary
--------------------------------

======= ============================= ====== ============================
 ``a``  Zap a rod (Activate)          ``A``  Activate an object
 ``b``  (walk - south west)           ``B``  (run - south west)
 ``c``  Close a door                  ``C``  Display character sheet
 ``d``  Drop an item                  ``D``  Disarm a trap or lock a door
 ``e``  List equipped items           ``E``  Eat some food
 ``f``  Use dragon breath attack      ``F``  Fuel your lantern/torch
 ``g``  Get objects on floor          ``G``  Gain new spells/prayers
 ``h``  (walk - west)                 ``H``  (run - west)
 ``i``  List contents of pack         ``I``  Inspect an item
 ``j``  (walk - south)                ``J``  (run - south)
 ``k``  (walk - north)                ``K``  (run - north)
 ``l``  (walk - east)                 ``L``  (run - east)
 ``m``  Cast a spell                  ``M``  Display map of entire level
 ``n``  (walk - south east)           ``N``  (run - south east)
 ``o``  Open a door or chest          ``O``  Toggle ignore
 ``p``  Project a spell               ``P``  Browse a book
 ``q``  Quaff a potion                ``Q``  Retire character & quit
 ``r``  Read a scroll                 ``R``  Rest for a period
 ``s``  Steal                         ``S``  Toggle stealth mode
 ``t``  Fire an item                  ``T``  Take off equipment
 ``u``  (walk - north east)           ``U``  (run - north east)
 ``v``  Throw an item                 ``V``  Polymorph into a monster
 ``w``  Wear/wield equipment          ``W``  Locate player on map (Where)
 ``x``  Look around                   ``X``  Use an item
 ``y``  (walk - north west)           ``Y``  (run - north west)
 ``z``  Aim a wand (Zap)              ``Z``  Use a staff (Zap)
 ``!``  Access party menu             ``^a`` Do autopickup
 ``@``  Center map                    ``^b`` (alter - south west)
 ``#``  See abilities                 ``^c`` Retire character & quit
 ``$``  Drop gold                     ``^d`` Ignore an item
 ``%``  Interact with keymaps         ``^e`` Toggle inven/equip window
 ``^``  (special - control key)       ``^f`` Repeat level feeling
 ``&``  Enter Dungeon Master menu     ``^g`` (special - bell)
 ``*``  Target monster or location    ``^h`` (alter - west)
 ``(``  Target friendly player        ``^i`` (special - tab)
 ``)``  Dump screen to a file         ``^j`` (alter - south)
 ``{``  Inscribe an object            ``^k`` (alter - north)
 ``}``  Uninscribe an object          ``^l`` (alter - east)
 ``[``  Display visible monster list  ``^m`` (special - return)
 ``]``  Display visible object list   ``^n`` (alter - south east)
 ``-``  Walk into a trap              ``^o`` Show previous message
 ``_``  Drink/fill an empty bottle    ``^p`` Show previous messages
 ``+``  Alter grid                    ``^q`` Get a quest
 ``=``  Set options                   ``^r`` Redraw the screen
 ``;``  Walk (with pickup)            ``^s`` Socials
 ``:``  Enter chat mode               ``^t`` Dig a tunnel
 ``'``  Target closest monster        ``^u`` (alter - north east)
 ``"``  Enter a user pref command     ``^v`` Display connected players
 ``,``  Run                           ``^w`` Full wilderness map
 ``<``  Go up staircase               ``^x`` Save and quit
 ``.``  Stay still (with pickup)      ``^y`` (alter - north west)
 ``>``  Go down staircase             ``^z`` Use chat command
 ``\``  (special - bypass keymap)     ``~``  Check knowledge
  \`    (special - escape)            ``?``  Display help
 ``/``  Identify monster              ``|``  List contents of quiver
 ``£``  Buy a house
``TAB`` Fire default ammo at target
``BKS`` Describe object
======= ============================= ====== ============================

Note: 'BKS' is equal to the BACKSPACE key.

Disturb
-------

For commands that repeat, like resting, tunnneling or running, certain events
will disturb the player. That stops the command, flushes queued input from
the keyboard or mouse, and drops any unprocessed keys from a partially
processed keymap. The events that disturb the player are:

* The command reached its goal: the wall was tunnelled, the door or chest was
  opened, the specific criteria for resting was met.
* While running, following a precomputed path, resting, or executing a repeated
  command (either with an explicit repeat count or one that repeats
  automatically if no repeat count was specified), a keypress or, if the front
  end supports it, a mouse button event, interrupts the command by disturbing
  the player. Since the keypress could come from a keymap, such commands will
  have to be last in the keymap (no further keystrokes in the action besides
  what the command would normally consume) to have the intended effect. While
  resting, checking for an input event that disturbs the rest only happens every
  128 turns.
* A monster's melee attack hits the player, regardless of whether any damage was
  done.
* A monster uses a ranged attack or spell, regardless of whether the attack
  hits.
* A monster that is visible and in the line of sight and either moves or
  performs a melee attack on the player will disturb if the `Disturb whenever
  viewable monster moves` option is on. With that option on, a monster that
  enters the line of sight and is visible or leaves the line of sight while
  visible, also disturbs the player.
* A monster smashes open a door.
* The player loses hit points.
* The player makes a melee attack.
* A projection affects the player, regardless of whether it does damage.
* Changes to a timed effect, like confusion, can disturb the player. If the
  change was initiated by the player by casting a spell or using an item, the
  change will not disturb the player with the exception for when an unidentified
  item is used. For effects that have more than two gradations, like cuts,
  stunning, and the hunger meter, going up a grade will disturb the player and
  going down a grade will disturb if the effect has a message for entering the
  grade from a higher one. For effects that have two gradations (off and on),
  going from off to on will disturb the player. Otherwise, the change will
  disturb if the effect in question does not duplicate a state the player has
  from another source and the change is not due to decreasing the effect's
  duration because time passed.
* The player tries to move into impassable terrain.
* While following a precomputed path or on the second or later step of running,
  a visible object, including gold, or visible monster in the next grid stops
  running.
* While following a precomputed path or on the second or later step of running,
  the next grid contains a known trap and the player is not immune to traps.
* If on the second or later step of a run or following a precomputed path, the
  player's current grid is in an area that has had trap detection cast on it but
  the next grid has not had trap detection cast on it.
* While following a precomputed path, a known impassable grid that cannot be
  autmatically dealt with as the next step on the path disturbs the player.
* The player triggers a trap. That disturbs even if the player passed the saving
  throw for the trap's effects.
* The player finds a secret door or a trap on a chest.
* The player faints because of hunger.
* The player's pack overflows.
* A curse on an equipped item triggers.
* The delayed effect of deep descent or a word of recall activates.
* The player's light source has run out of fuel or is close to running out of
  fuel and a new "growing faint" reminder message is displayed.
* An item that is equipped or in the pack has recharged and that item's
  inscription includes '!!' or the `notify on object recharge` option is on.
* The player is on a visible stack of objects that are not all gold and a scan
  for autopickup happens (either by holding with autopickup, explicitly invoking
  autopickup, or having just moved to the grid).
* The player enters a store.
* The player enters a new level.

Special Keys
------------
 
Certain special keys may be intercepted by the operating system or the host
machine, causing unexpected results. In general, these special keys are
control keys, and often, you can disable their special effects.
 
It is often possible to specify "control-keys" without actually pressing
the control key, by typing a caret (``^``) followed by the key. This is
useful for specifying control-key commands which might be caught by the
operating system as explained above.

Pressing backslash (``\``) before a command will bypass all keymaps, and
the next keypress will be interpreted as an "underlying command" key,
unless it is a caret (``^``), in which case the keypress after that will be
turned into a control-key and interpreted as a command in the underlying
PWMAngband keyset. The backslash key is useful for creating actions which are
not affected by any keymap definitions that may be in force, for example,
the sequence ``\`` + ``.`` + ``6`` will always mean "run east", even if the
``.`` key has been mapped to a different underlying command.

The ``0`` and ``^`` and ``\`` keys all have special meaning when entered at
the command prompt, and there is no "useful" way to specify any of them as
an "underlying command", which is okay, since they would have no effect.

For many input requests or queries, the special character 'ESCAPE' will
abort the command. The '[y/n]' prompts may be answered with ``y`` or
``n``, or 'escape'.
 
Selection of Objects
--------------------
 
Many commands will prompt for a particular object to be used.
For example, the command to read a scroll will ask you which of the
scrolls that you are carrying that you wish to read. In such cases, the
selection is made by typing a letter of the alphabet (or a number if choosing
from the quiver). The prompt will indicate the possible letters/numbers,
and you will also be shown a list of the appropriate items. Often you will
be able to press ``/`` to switch between inventory and equipment, or ``|`` to
select the quiver, or ``-`` to select the floor.  Using the right arrow also
rotates selection between equipment, inventory, quiver, floor and back to
equipment; the left arrow rotates in the opposite direction.
 
The particular object may be selected by an upper case or a lower case
letter. If lower case is used, the selection takes place immediately. If
upper case is used, then the particular option is described, and you are
given the option of confirming or retracting that choice. Upper case
selection is thus safer, but requires an extra key stroke.