/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __CONFIG_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data definition for structs.h */

#define TRUE	1
#define YES	1
#define FALSE	0
#define NO	0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/


/* GAME PLAY OPTIONS */

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
char pk_allowed = NO;

/* is playerthieving allowed? */
char pt_allowed = NO;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 1;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* exp change limits */
int max_exp_gain = 10000000;	/* max gainable per kill */
int max_exp_loss = 500000;	/* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 20;

/* How many ticks before a player is sent to the void or idle-rented. */
int idle_void = 8;
int idle_rent_time = 20;

/* This level and up is immune to idling, LVL_IMPL+1 will disable it. */
int idle_max_level = LVL_OWNER;

/* should items in death traps automatically be junked? */
// char dts_are_dumps = NO; // Artus> No longer used.

/* Rooms defining start rooms of worlds */
int world_start_room[NUM_WORLDS] = {1115, 13235, 22031};

/*
 * Whether you want items that immortals load to appear on the ground or not.
 * It is most likely best to set this to 'YES' so that something else doesn't
 * grab the item before the immortal does, but that also means people will be
 * able to carry around things like boards.  That's not necessarily a bad
 * thing, but this will be left at a default of 'NO' for historic reasons.
 */
char load_into_inventory = YES;

/* "okay" etc. */
const char *OK="Okay.\r\n";
const char *NOPERSON="No-one by that name here.\r\n";
const char *NOEFFECT="Nothing seems to happen.\r\n";

const char *PEACEROOM="This room just has such a peaceful, easy feeling...\r\n";
const char *CHARFIGHTING="No way.  You're fighting for your life!\r\n";
const char *RESTSKILL="You cannot summon the energy to perform this.\r\n";
const char *UNFAMILIARSKILL="You are unfamiliar with that skill.\r\n";
const char *UNFAMILIARSPELL="You are unfamiliar with that spell.\r\n";

/*
 * You can define or not define TRACK_THOUGH_DOORS, depending on whether
 * or not you want track to find paths which lead through closed or
 * hidden doors. A setting of 'NO' means to not go through the doors
 * while 'YES' will pass through doors to find the target.
 */
char track_through_doors = YES;

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.)
 */
char free_rent = YES;

/* maximum number of items players are allowed to rent */
int max_obj_save = 100;

/* receptionist's surcharge on top of item costs */
int min_rent_cost = 0;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.  This
 * option has an added meaning past bpl13.  If auto_save is YES, then
 * the 'save' command will be disabled to prevent item duplication via
 * game crashes.
 */
char auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 5;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 178;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 356;


/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
room_vnum mortal_start_room = 1115;

/* virtual number of room that immorts should enter at by default */
room_vnum immort_start_room = 1200;

/* virtual number of room that frozen players should enter at */
room_vnum frozen_start_room = 501;

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.item.c if you change the number of non-NOWHERE
 * donation rooms.
 */
room_vnum donation_room_1 = 1190;
room_vnum donation_room_2 = NOWHERE;	/* unused - room for expansion */
room_vnum donation_room_3 = NOWHERE;	/* unused - room for expansion */


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/*
 * This is the default port on which the game should run if no port is
 * given on the command-line.  NOTE WELL: If you're using the
 * 'autorun' script, the port number there will override this setting.
 * Change the PORT= line in autorun instead of (or in addition to)
 * changing this.
 */
ush_int DFLT_PORT = 4444;

/*
 * IP address to which the MUD should bind.  This is only useful if
 * you're running Circle on a host that host more than one IP interface,
 * and you only want to bind to *one* of them instead of all of them.
 * Setting this to NULL (the default) causes Circle to bind to all
 * interfaces on the host.  Otherwise, specify a numeric IP address in
 * dotted quad format, and Circle will only bind to that IP address.  (Of
 * course, that IP address must be one of your host's interfaces, or it
 * won't work.)
 */
const char *DFLT_IP = NULL; /* bind to all interfaces */
/* const char *DFLT_IP = "192.168.1.1";  -- bind only to one interface */

/* default directory to use as data directory */
const char *DFLT_DIR = "lib";

/*
 * What file to log messages to (ex: "log/syslog").  Setting this to NULL
 * means you want to log to stderr, which was the default in earlier
 * versions of Circle.  If you specify a file, you don't get messages to
 * the screen. (Hint: Try 'tail -f' if you have a UNIX machine.)
 */
const char *LOGNAME = NULL;
/* const char *LOGNAME = "log/syslog";  -- useful for Windows users */

/* maximum number of players allowed before game starts to turn people away */
int max_playing = 300;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 3;

/*
 * Rationale for enabling this, as explained by naved@bird.taponline.com.
 *
 * Usually, when you select ban a site, it is because one or two people are
 * causing troubles while there are still many people from that site who you
 * want to still log on.  Right now if I want to add a new select ban, I need
 * to first add the ban, then SITEOK all the players from that site except for
 * the one or two who I don't want logging on.  Wouldn't it be more convenient
 * to just have to remove the SITEOK flags from those people I want to ban
 * rather than what is currently done?
 */
char siteok_everyone = TRUE;

/*
 * Some nameservers are very slow and cause the game to lag terribly every 
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

// After I did a slowns and set nameserber_is_slow to YES, it appeared
// to somehow do something to MENU - below - perhaps set that variable instead
// in that case to some pointer? it is a const, shall see if it occurs after a
// make clean and make again ... - DM.
// names are resolved - thinking it was possibly something to do with the size
// of the variable - ie. a char vs int?
// yes it was declared in act.other.c as extern int nameserver_is_slow, should
// be sweet now ...
// Artus> Going to default this to yes... These days hostnames are too damn
//        long.. Much easier to deal with IP addresses.
char nameserver_is_slow = YES;


const char *MENU =
"\r\n"
"&g      [ Primal MUD ]&n\r\n"
"&y0&n -  Disconnect from Primal\r\n"
"&y1&n -  Enter the land\r\n"
"&y2&n -  Enter description\r\n"
"&y3&n -  Read the background story\r\n"
"&y4&n -  Change password.\r\n"
"&y5&n -  Self delete\r\n"
"\r\n"
"   Make your choice>&n ";



const char *WELC_MESSG =
"\r\n"
"Welcome to the primal lands. Explore, enjoy."
"\r\n\r\n";

const char *START_MESSG = 
" &yW&nelcome to PrimalMUD.\r\n\r\n"
"You have been given the basics to survive initially, but you should seek\r\n"
"allies in your quest for supremacy. At least .. initially.\r\n\r\n";

/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/*
 * Should the game automatically create a new wizlist/immlist every time
 * someone immorts, or is promoted to a higher (or lower) god level?
 * NOTE: this only works under UNIX systems.
 */
char use_autowiz = YES;

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_GOD;
