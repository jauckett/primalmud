/* ************************************************************************
*   File: interpreter.h                                 Part of CircleMUD *
*  Usage: header file: public procs, macro defs, subcommand defines       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define ACMD(name)  \
   void name(struct char_data *ch, char *argument, int cmd, int subcmd)

ACMD(do_move);

#define CMD_NAME (cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcmp(cmd_name, cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (cmd_info[cmdnum].command_pointer == do_move)

void	command_interpreter(struct char_data *ch, char *argument);
int	search_block(char *arg, const char **list, int exact);
int	search_block_case_insens(const char *arg, const char **list, int exact);
char	lower( char c );
char	*one_argument(char *argument, char *first_arg);
char	*one_word(char *argument, char *first_arg);
char	*any_one_arg(char *argument, char *first_arg);
char	*any_one_arg_case_sens(char *argument, char *first_arg);
char	*two_arguments(char *argument, char *first_arg, char *second_arg);
int	fill_word(char *argument);
void	half_chop(char *string, char *arg1, char *arg2);
void	half_chop_case_sens(char *string, char *arg1, char *arg2);
void	nanny(struct descriptor_data *d, char *arg);
int	is_abbrev(const char *arg1, const char *arg2);
int	is_number(const char *str);
int	find_command(const char *command);
void	skip_spaces(char **string);
char	*delete_doubledollar(char *string);

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)


struct command_info {
   const char *command;
   byte minimum_position;
   void	(*command_pointer)
	   (struct char_data *ch, char * argument, int cmd, int subcmd);
   sh_int minimum_level;
   int	subcmd;
   int type;
   bool helpAssigned;
};

/*
 * Necessary for CMD_IS macro.  Borland needs the structure defined first
 * so it has been moved down here.
 */
#ifndef __INTERPRETER_C__
extern const struct command_info cmd_info[];
#endif

/*
 * Alert! Changed from 'struct alias' to 'struct alias_data' in bpl15
 * because a Windows 95 compiler gives a warning about it having similiar
 * named member.
 */
struct alias_data {
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

#define ALIAS_SIMPLE	0
#define ALIAS_COMPLEX	1

#define ALIAS_SEP_CHAR	';'
#define ALIAS_VAR_CHAR	'$'
#define ALIAS_GLOB_CHAR	'*'

/*
 * COMMANDS
 *   Each command is now classified as one of the following types.
 *   
 *   CMD_NONE types are not displayed under command sub menus. 
 *   CMD_SKILL commands unknown to the player are not shown in sub menus.
 *   CMD_WIZ commands are only shown to players that meet the level restriction.
 *
 *   All other command types are shown in command sub menus.
 */

#define CMD_NONE        0       // Special, hidden commands ...
#define CMD_WIZ         1       // Wizard Commands
#define CMD_SKILL       2       // Skill commands

#define CMD_COMM        3       // Communication
#define CMD_COMBAT      4       // Combat/Fighting
#define CMD_INFO        5       // Informative
#define CMD_MOVE        6       // Movement 
#define CMD_OBJ         7       // Object (Item) related
#define CMD_SOCIAL      8       // Socials
#define CMD_UTIL        9       // Utilities
#define CMD_SHOP        10      // Shop commands
#define CMD_MISC        11      // Misc, uncategorised commands
#define CMD_MAIL        12      // Mail, board commands
#define CMD_SPEC        13      // Player Special commands

#define NUM_CMD_TYPES   14
  
  
/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_NORTH	1
#define SCMD_EAST	2
#define SCMD_SOUTH	3
#define SCMD_WEST	4
#define SCMD_UP		5
#define SCMD_DOWN	6

/* do_gen_ps */
#define SCMD_INFO       0
#define SCMD_HANDBOOK   1 
#define SCMD_CREDITS    2
#define SCMD_NEWS       3
#define SCMD_WIZLIST    4
#define SCMD_POLICIES   5
#define SCMD_VERSION    6
#define SCMD_IMMLIST    7
#define SCMD_MOTD	8
#define SCMD_IMOTD	9
#define SCMD_CLEAR	10
#define SCMD_WHOAMI	11
#define SCMD_AREAS	12

/* do_gen_tog */
#define SCMD_NOSUMMON   0
#define SCMD_NOHASSLE   1
#define SCMD_BRIEF      2
#define SCMD_COMPACT    3
#define SCMD_NOTELL	4
#define SCMD_NOAUCTION	5
#define SCMD_DEAF	6
#define SCMD_NOGOSSIP	7
#define SCMD_NOGRATZ	8
#define SCMD_NOWIZ	9
#define SCMD_ROOMFLAGS	11
#define SCMD_NOREPEAT	12
#define SCMD_HOLYLIGHT	13
#define SCMD_SLOWNS	14
#define SCMD_AUTOEXIT	15
#define SCMD_NOIMMNET   16
#define SCMD_AFK        17
#define SCMD_NOINFO     18
#define SCMD_NONEWBIE   19
#define SCMD_AUTOLOOT   20
#define SCMD_AUTOGOLD   21
#define SCMD_AUTOSPLIT  22
#define SCMD_TRACK	23
#define SCMD_NOHINTS    24
#define SCMD_NOCT       25 /* ARTUS - Clan Channels */
#define SCMD_NOCI       26 /* ARTUS - Clan Info     */
#define SCMD_AUTOCORPSE 27
#define SCMD_AUTOEAT    28 /* Artus> Automatic Eating */

/* do_wizutil */
#define SCMD_REROLL	0
#define SCMD_PARDON     1
#define SCMD_NOTITLE    2
#define SCMD_SQUELCH    3
#define SCMD_FREEZE	4
#define SCMD_THAW	5
#define SCMD_UNAFFECT	6
#define SCMD_IMMNET     7
#define SCMD_ANGNET     8 

/* do_spec_com */
#define SCMD_WHISPER	0
#define SCMD_ASK	1

/* do_gen_com */
#define SCMD_HOLLER	0
#define SCMD_SHOUT	1
#define SCMD_GOSSIP	2
#define SCMD_AUCTION	3
#define SCMD_GRATZ	4
#define SCMD_NEWBIE     5
//#define SCMD_CTALK      6 

/* do_shutdown */
#define SCMD_SHUTDOW	0
#define SCMD_SHUTDOWN   1

/* do_quit */
#define SCMD_QUI	0
#define SCMD_QUIT	1
#define SCMD_QUITR	2 // Artus> Quitting outside haven.

/* do_date */
#define SCMD_DATE	0
#define SCMD_UPTIME	1

/* do_commands */
#define SCMD_COMMANDS	0
#define SCMD_SOCIALS	1
#define SCMD_WIZHELP	2

/* do_drop */
#define SCMD_DROP	0
#define SCMD_JUNK	1
#define SCMD_DONATE	2
#define SCMD_IS_PUT	3 // Artus> for drop_otrigger.

/* do_gen_write */
#define SCMD_BUG	0
#define SCMD_TYPO	1
#define SCMD_IDEA	2

/* do_look */
#define SCMD_LOOK	0
#define SCMD_READ	1

/* do_qcomm */
#define SCMD_QSAY	0
#define SCMD_QECHO	1

/* do_pour */
#define SCMD_POUR	0
#define SCMD_FILL	1

/* do_poof */
#define SCMD_POOFIN	0
#define SCMD_POOFOUT	1

/* do_hit */
#define SCMD_HIT	0
#define SCMD_MURDER	1
#define SCMD_KILL       2

/* do_eat */
#define SCMD_EAT	0
#define SCMD_TASTE	1
#define SCMD_DRINK	2
#define SCMD_SIP	3

/* do_use */
#define SCMD_USE	0
#define SCMD_QUAFF	1
#define SCMD_RECITE	2

/* do_echo */
#define SCMD_ECHO	0
#define SCMD_EMOTE	1

/* do_gen_door */
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4

/*. do_olc .*/
#define SCMD_OASIS_REDIT       0
#define SCMD_OASIS_OEDIT       1
#define SCMD_OASIS_ZEDIT       2
#define SCMD_OASIS_MEDIT       3
#define SCMD_OASIS_SEDIT       4
#define SCMD_OASIS_TRIGEDIT    5
#define SCMD_OLC_SAVEINFO      7

/* do_track (for hunt) */
#define SCMD_HUNT       1
#define SCMD_AUTOHUNT   2

/* do_give */
#define SCMD_GIVE               0
#define SCMD_SLIP               1 /* SKILL_SLIP ... */

/* do_auction */
#define AUC_NONE		0
#define AUC_BID			1
#define AUC_CANCEL		2
#define AUC_LIST		3
#define AUC_SELL		4
#define AUC_SOLD		5
#define AUC_STAT		6
#define AUC_PURGE		7
