/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/*
   This file has been modified by Brett Murphy
*/

#define __CONFIG_C__

#include "structs.h"

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

/* is playerthieving allowed? */
int pt_allowed = NO;
int pk_allowed = NO;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 2;

/* number of movement points it costs to holler */
int holler_move_cost = 50;

/* exp change limits */
int max_exp_gain = 3000000;	/* max gainable per kill */
int max_exp_loss = 1000000;	/* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 40;

/* should items in death traps automatically be junked? */
int dts_are_dumps = NO;

/* "okay" etc. */
char *OK = "Okay.\r\n";
char *NOPERSON = "No-one by that name here.\r\n";
char *NOEFFECT = "Nothing seems to happen.\r\n";

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.
 */
int free_rent = YES;

/* use this to determine if rent is per day or one off */
int rent_per_day = YES;

/* discount for rent */
int rent_discount = 90;

/* maximum number of items players are allowed to rent */
int max_obj_save = 90;

/* receptionist's surcharge on top of item costs */
int min_rent_cost = 10;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 5;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 10;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 35;


/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
sh_int mortal_start_room = 10;

/* virtual number of room that eternals should enter at by default */
sh_int eternal_start_room = 102;

/* virtual number of room that immorts should enter at by default */
sh_int immort_start_room = 100;

/* virtual number of room that frozen players should enter at */
sh_int frozen_start_room = 501;

/* virtual room numbers for starting places in each world... */
sh_int world_start_room[3] = {15,13235,22031};

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.obj1.c if you change the number of non-NOWHERE
 * donation rooms.
 */
sh_int donation_room_1 = 90;
sh_int donation_room_2 = NOWHERE; 	/* unused - room for expansion */
sh_int donation_room_3 = 22031;	/* unused - room for expansion */


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/* default port the game should run on if no port given on command-line */
int DFLT_PORT = 4000;

/* default directory to use as data directory */
char *DFLT_DIR = "lib";

/* default directory for alias saving */
char *ALIAS_DIRNAME = "alias/";

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 50;

/* maximum size of bug, typo and idea files (to prevent bombing) */
int max_filesize = 50000;

/* Some nameservers (such as the one here at JHU) are slow and cause the
 *  game to lag terribly every time someone logs in.  The lag is caused by
 * the gethostbyaddr() function -- the function which resolves a numeric
 * IP address (such as 128.220.13.30) into an alphabetic name (such as
 * circle.cs.jhu.edu).
 *
 * The nameserver at JHU can get so bad at times that the incredible lag
 * caused by gethostbyaddr() isn't worth the luxury of having names
 * instead of numbers for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = NO;


char *MENU =
"\r\n"
"Welcome to PrimalMUD!\r\n"
"0) Exit from PrimalMUD.\r\n"
"1) Enter the game.\r\n"
"2) Enter description.\r\n"
"3) Read the background story.\r\n"
"4) Change password.\r\n"
"5) Delete this character.\r\n"
"\r\n"
"   Make your choice: ";

char *MAINTEN =

"\r\n\r\n"
"                                 PrimalMUD 1.50\r\n"
"\n\r"
"                                  Created By \n\r"
"\n\r"
"                                 John Auckett \r\n"
"                                      &\n\r"
"                                 Brett Murphy\n\r"
"\r\n"
"                       Using a Game System by Steven Wark\r\n"
"\n\r"
"                            Based on CircleMUD 3.0,\r\n"
"                            Created by Jeremy Elson\r\n"
"\r\n"
"\r\n\r\n"
"                 PrimalMUD is currently having maintenence performed.\r\n "
"		        Please come back in a little while.\r\n\r\n";

char *GREETINGS =

"\r\n\r\n"
"                                 PrimalMUD 1.50\r\n"
"\r\n"
"                                  Created By \r\n"
"\r\n"
"                                 John Auckett \r\n"
"                                      &\r\n"
"                                 Brett Murphy\n\r"
"\r\n"
"                       Using a Game System by Steven Wark\r\n"
"\n\r"
"                            Based on CircleMUD 3.0,\r\n"
"                            Created by Jeremy Elson\r\n"
"\r\n\r\n"
"By what name do you wish to be known? ";


char *WELC_MESSG =
"\r\n"
"Welcome to the land of PrimalMUD! Enjoy your stay."  
"\r\n\r\n";

char *START_MESSG =
"-------------------------------------------------------------------------\r\n"
"Welcome.  This is your new PrimalMUD character!  You can now earn gold,\r\n"
"gain experience, find weapons and equipment, and much more -- while\r\n"
"meeting people from around the world!\r\n\r\n"
"You have been given a map of the Medieval World town of Haven. You are\r\n"
"located at Wayfarer's inn (top left hand corner of map).\r\n"
"----------------------------------------------------------------------\r\n\r\n\r\n";


/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
int use_autowiz = NO;

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_IS_GOD;
