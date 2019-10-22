How It Works
============

This document describes how PWMAngband actually *works* at a high level.

The Game
--------

As you probably know if you're reading this, PWMAngband is a roguelike game set
in a high-fantasy universe. The game world is made up of levels, numbered from
zero ("the town") to some maximum depth. Levels are increasingly dangerous the
deeper they are into the dungeon. Levels are filled with monsters, traps, and
objects. Monsters move and act on their own, traps react to creatures entering
their square, and objects are inert unless used by a creature. The objective of
the game is to find Morgoth at depth 100 and kill him.

Data Structures
---------------

There are three important top-level data structures in PWMAngband: the 'chunk',
the player, and the static data tables.

The Chunk
*********

A chunk represents an area of dungeon, and contains everything inside it; this
includes any monsters, objects, or traps inside the bounds of that chunk. A
chunk also keeps a map of the terrain in its area. For unpleasant historical
reasons, all monsters/objects/traps in a chunk are stored in arrays and usually
referred to by index; each square of a chunk knows the indexes (if any) of
monsters/objects/traps contained in it. A chunk also stores AI pathfinding data
for its contained area. All data in the 'current' chunk is lost when leaving the
level.

The Player
**********

The player is a global object containing information about, well, the player.
All the information in the player is level-independent. This structure contains
stats, any current effects, hunger status, sex/race/class, the player's
inventory, and a grab-bag of other information. Although there is a global
player object, many functions instead take a player object explicitly to make
them easier to test.

The Static Data
***************

PWMAngband's static data - player and monster races, object types, artifacts, et
cetera - is loaded from the `gamedata Files`_. Once loaded, this
data is stored in global tables, sometimes referred to as the 'info arrays'.
These arrays are generally declared in the header files of the code that uses
them most, but they are mostly initialized by the edit-file code. The sizes of
these arrays are stored in a 'maxima' structure, called z_info.

The Z Layer
-----------

The lowest-level code in PWMAngband is the "Z" layer, which provides
platform-independent abstractions and generic data structures. Currently, the Z
layer provides:

===============   ========================================
"z-bitflag"       Densely-packed bit flag arrays
"z-color"         Colors
"z-dice"          Dice expressions
"z-expression"    Mathematical expressions
"z-file"          File I/O
"z-form"          String formatting
"z-quark"         String interning
"z-queue"         Queues
"z-rand"          Randomness
"z-set"           Sets
"z-textblock"     Wrapped text
"z-type"          Basic types
"z-util"          Random utility macros
"z-virt"          malloc() wrappers
===============   ========================================

Code in the Z layer may not depend on files outside the Z layer.

Key Abstractions
----------------

Certain game-specific abstractions are important and widely used in PWMAngband
to glue the UI code to the game engine. These are the command queue, which sends
player commands to the game engine, and events, which indicate to the UI that
the state of the game changed.

The command queue
*****************

TBD

Files
-----

PWMAngband uses three types of files for storing data: gamedata files, which
contain the game's static data, pref files, which contain UI settings,
and save files, which contain the state of a game in progress.

Gamedata Files
**************

Gamedata files use a line-oriented format where fields are separated by colons.
The parser for this format is in "parser.h". These files are mostly loaded at
initialization time (see `init.c - init_angband`_) and used to fill in the
static data arrays (see `The Static Data`_).

Pref Files
**********

TBD

Savefiles
*********

Currently, a savefile is a series of concatenated blocks. Each block has a name
describing what type it is and a version tag. The version tag allows for old
savefiles to be loaded, although the load/save code will only write new
savefiles. Numbers in savefiles are stored in little-endian byte order and
strings are stored null-terminated.

Control Flow
------------

The flow of control through PWMAngband is complicated and can be very
non-obvious due to overuse of global variables as special-behavior hooks. That
said, this section gives a high-level overview of the control flow of a game
session.

Startup
*******

Execution begins in main.c, which runs frontend-independent initialization code,
then continues in the appropriate "main-xxx.c" file for the current frontend.
After the game engine is initialized, the player is loaded (or generated) and
gameplay begins.

"main.c" and "main-xxx.c"
*************************

main.c's "main()" is the entry point for PWMAngband execution except on
Windows, where main-win.c's "WinMain()" is used, and on Nintendo DS, where a
special "main()" in main-nds.c is used. The "main()" function is responsible
for dropping permissions if PWMAngband is running setuid, parsing command line
arguments, then finding a frontend to use and initializing it. Once "main()"
finds a frontend, it sets up signal handlers, sets up the display, then calls
"play_game()".

dungeon.c - play_game
*********************

This function is responsible for driving the remaining initialization. It first
calls `init.c - init_angband`_, which loads all the `gamedata files`_ and
initializes other static data used by the game. It then configures subwindows,
loads a saved game if there is a valid save (see `savefiles`_), sets up the RNG,
loads pref files (see `prefs.c - process_pref_file`_), and enters the game main
loop (see `dungeon.c - the game main loop`_).

init.c - init_angband
*********************

The init_angband() function in init.c is responsible for loading and setting up
static data needed by the game engine. Inside init.c, there is a list of 'init
modules' that have startup-time static data they need to initialize, these are
registered in an array of module pointers in init.c, and init_angband() calls
their initialization hooks before doing any other work. The init_angband()
function then loads the top-level pref file (see `pref files`_), initializes the
command queue (see `the command queue`_), then waits for the UI to enqueue
either QUIT, NEWGAME, or LOADFILE. This function returns true if the player
wants to roll a new character, and false if they want to load an existing
character.

prefs.c - process_pref_file
***************************

The process_pref_file() function in prefs.c is responsible for loading user pref
files, which can live at multiple paths. User preference files override default
preference files. See `pref files`_ for more details.

Gameplay
********

Once the simulation is set up, the game main loop in `dungeon.c - play_game`_ is
responsible for stepping the simulation.

dungeon.c - the game main loop
******************************

The main loop of the game is inside play_game() in typical understated
PWMAngband style. This loop runs once per time that either the level is
regenerated, the player dies, or the player quits the game. Each iteration
through, the this loop runs the level main loop to completion for an individual
level.

dungeon.c - the level main loop
*******************************

The main loop for the level is implemented in dungeon() in dungeon.c. The
dungeon() function is called when the player enters a level, and returns only
when the player exits the level, either by changing levels, dying, or quitting.
This function is responsible for tracking the player's max level/depth,
autosaving at level entry, and running the main simulation loop. Each iteration
of the main simulation loop is one "turn" in PWMAngband parlance, or one step of
the simulator. During each turn:

* All monsters with more energy than the player act
* The player acts
* All other monsters act
* The UI updates
* The world acts
* End-of-turn housekeeping is done

mon-melee2.c - process_monsters()
*********************************

In PWMAngband, creatures act in order of "energy", which roughly determines how
many actions they can take per step through the simulation. The
process_monsters() function in mon-melee2.c is responsible for walking through
the list of all monsters in the current chunk (see `the chunk`_) and having each
monster act by calling process_monster(), which implements the highest level AI
for monsters.

dungeon.c - process_player()
****************************

The process_player() function allows the player to act repeatedly until they do
something that uses energy. Commands like looking around or inscribing items do
not use energy; movement, attacking, casting spells, using items, and so on do.
The rule of thumb is that a command that does not alter game engine state does
not use energy, because it does not represent an action the character in the
simulation is doing. The guts of the process_player() function are actually
handled by process_command() in cmd-core.c, which looks up commands in the
game_cmds table in that file.

Keeping the UI up to date
*************************

Four related horribly-named functions in player-calcs.h are responsible for
keeping the UI in sync with the simulated character's state:

==================  ===========================================================
"notice_stuff()"    which deals with pack combining and dropping ignored items;
"update_stuff()"    which recalculates derived bonuses, AI data, vision, seen
                      monsters, and other things based on the flags in
                      "player->upkeep->update";
"redraw_stuff()"    which signals the UI to redraw changed sections of the
                      game state;
"handle_stuff()"    which calls update_stuff() and redraw_stuff() if needed.
==================  ===========================================================

These functions are called during every game loop, after the player and all
monsters have acted.

dungeon.c - process_world()
***************************

The process_world() function only runs every 10 turns. It is responsible for the
day/night transition in town, restocking the stores, generating new creatures
over time, dealing poison/cut damage, applying hunger, regeneration, ticking
down timed effects, consuming light fuel, and applying a litany of spell effects
that happen 'at random' from the player's point of view.
