/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <fstream>
#include <list>


#define WANT_CHEAT

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "tedit.h"
#include "constants.h"
#include "reports.h"
#include "clan.h"
#include "dg_scripts.h"

extern int world_start_room[NUM_WORLDS];
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern const char *class_menu;
extern const char *race_menu;
extern const char *class_help;
extern const char *race_help;
extern const char *char_help;
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct user_data *user_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern long NUM_PLAYERS;

/* external functions */
void set_race_specials(struct char_data *ch);
extern char *GREETINGS;
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch);
int parse_class(char *arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
void read_aliases(struct char_data *ch);
int parse_race_name(char *arg);
void roll_real_abils(struct char_data * ch);
int level_exp(struct char_data *ch, int level);
void read_saved_vars(struct char_data *ch);

/* local functions */
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig);
int reserved_word(char *argument);
int find_name(char *name);
int _parse_name(char *arg, char *name);
void show_remorts(struct descriptor_data *d);

/* prototypes for all do_x functions. */

ACMD(do_modifiers);
ACMD(do_stats);

ACMD(do_suicide);
ACMD(do_torch);
ACMD(do_meditate);
ACMD(do_timers);
ACMD(do_poisonblade);
ACMD(do_first_aid);
/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mjunk);
ACMD(do_mdoor);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mzoneecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mexp);
ACMD(do_mgold);
ACMD(do_mhunt);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mdamage);
ACMD(do_mrestore);
ACMD(do_vdelete);

ACMD(do_read);
ACMD(do_recall);
ACMD(do_reporting);
ACMD(do_violent_skill);

ACMD(do_friend);
ACMD(do_corpse);
ACMD(do_show_hint);

ACMD(do_purse);

ACMD(do_charge);
ACMD(do_disarm);
ACMD(do_clot_wounds);

ACMD(do_burgle);
ACMD(do_trade);
ACMD(do_vote);
ACMD(do_event);
ACMD(do_bounties);
ACMD(do_sense);
ACMD(do_ritual);
ACMD(do_repair);
ACMD(do_escape);
ACMD(do_memorise);
ACMD(do_disguise);
ACMD(do_remort);
ACMD(do_info);
ACMD(do_immlist);
ACMD(do_search);

ACMD(do_mount);
ACMD(do_breakin);
ACMD(do_dismount);  

ACMD(do_ambush);
ACMD(do_attend_wounds);
ACMD(do_adrenaline);
//ACMD(do_battlecry);
//ACMD(do_headbutt);
//ACMD(do_piledrive);
//ACMD(do_trip);
//ACMD(do_bearhug);
//ACMD(do_bodyslam);

ACMD(do_action);
ACMD(do_advance);
ACMD(do_affects);  
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_auction);
ACMD(do_autoassist);
//ACMD(do_backstab);
ACMD(do_ban);
//ACMD(do_bash);
ACMD(do_berserk);
ACMD(do_bite);
ACMD(do_blackjack); 
ACMD(do_cast);
ACMD(do_change);   
//ACMD(do_clan_table); -- ARTUS
//ACMD(do_clans); -- ARTUS
ACMD(do_clan); /* ARTUS */
ACMD(do_cltalk); /* ARTUS */
ACMD(do_classes);
ACMD(do_color);
ACMD(do_combine);
ACMD(do_commands);
ACMD(do_compare);
ACMD(do_compost);
ACMD(do_consider);
ACMD(do_cream);
ACMD(do_credits);
ACMD(do_ctalk); /* ARTUS */
ACMD(do_date);
ACMD(do_dc);
ACMD(do_debug_cmd);
ACMD(do_deimmort);
ACMD(do_diagnose);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_edit);         /* Mainly intended as a test function. */
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_events);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_exp);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_go);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hide);
//ACMD(do_hit);
ACMD(do_house);
ACMD(do_ignore);
ACMD(do_immort);
//ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_join); 
ACMD(do_kick);
//ACMD(do_kill);
ACMD(do_last);
ACMD(do_laston); 
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_listen);
ACMD(do_load);
ACMD(do_loadweapon);
ACMD(do_look);
/* ACMD(do_move); -- interpreter.h */
ACMD(do_moon);
ACMD(do_mortal_kombat);
ACMD(do_not_here);
ACMD(do_oasis);
ACMD(do_offer);
ACMD(do_olc);
ACMD(do_oasislist);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pagelength);
ACMD(do_pagewidth);
ACMD(do_pinch);
ACMD(do_pkill); /* ARTUS */
ACMD(do_pkset);   
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quest);
ACMD(do_quest_log);
ACMD(do_queston);
ACMD(do_questoff);
ACMD(do_quit);
ACMD(do_race);
ACMD(do_realtime);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_retreat);
ACMD(do_return);
ACMD(do_save);
ACMD(do_say);
ACMD(do_scan);
ACMD(do_score);
//ACMD(do_scream);
ACMD(do_send);
ACMD(do_set);
ACMD(do_setcolour);
ACMD(do_shoot);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_skillshow);
ACMD(do_slay);
ACMD(do_sleep);
ACMD(do_slots);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spellinfo);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_spy);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tag);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_throw);
ACMD(do_tic);   
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_trap);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whostr);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);
ACMD(do_punish); /* ARTUS - Punish */
ACMD(do_sentence); /* ARTUS - Sentence */
ACMD(do_balance); /* ARTUS - Rebalance */

#ifdef WANT_CHEAT
ACMD(do_cheat); // Artus> Can't set myself back up.. Oops. 
#endif
/* clan do_x commands -Hal 
ACMD(do_knight);
ACMD(do_banish);
ACMD(do_demote);
ACMD(do_recruit);
ACMD(do_promote);
ACMD(do_signup);
-- Not any more... -- ARTUS
*/

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

//cpp_extern const struct command_info cmd_info[] = {
struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH, CMD_MOVE, FALSE },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST, CMD_MOVE, FALSE }, 
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH, CMD_MOVE, FALSE }, 
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST, CMD_MOVE, FALSE }, 
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP, CMD_MOVE, FALSE }, 
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN, CMD_MOVE, FALSE }, 

  /* now, the main list */
  { "at"       , POS_DEAD    , do_at       , LVL_ANGEL, 0, CMD_WIZ, FALSE },
  { "adrenaline",POS_FIGHTING, do_adrenaline, 0, 0, CMD_SKILL, FALSE },
  { "advance"  , POS_DEAD    , do_advance  , LVL_IMPL, 0, CMD_WIZ, FALSE }, 
  { "affects"  , POS_DEAD    , do_affects  , 5, 0, CMD_INFO, FALSE }, 
  { "afk"      , POS_DEAD    , do_gen_tog  , 0, SCMD_AFK, CMD_INFO, FALSE }, 
  { "agree"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "alias"    , POS_DEAD    , do_alias    , 0, 0, CMD_UTIL, FALSE }, 
  { "accuse"   , POS_SITTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "ambush"   , POS_STANDING, do_ambush   , 0, 0, CMD_SKILL, FALSE }, 
  { "angnet"   , POS_DEAD    , do_wiznet   , LVL_ANGEL, SCMD_ANGNET, CMD_WIZ, FALSE }, 
  { "-"        , POS_DEAD    , do_wiznet   , LVL_ANGEL, SCMD_ANGNET, CMD_WIZ, FALSE }, 
  { "angel"    , POS_RESTING , do_action   , LVL_ANGEL, 0, CMD_WIZ, FALSE }, 
  { "apologise", POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "applaud"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "areas"    , POS_DEAD    , do_gen_ps   , 0, SCMD_AREAS, CMD_INFO, FALSE }, 
  { "assassinate", POS_FIGHTING, do_not_here , 1, 0, CMD_COMBAT, FALSE }, 
  { "assist"   , POS_FIGHTING, do_assist   , 1, 0, CMD_COMBAT, FALSE }, 
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK, CMD_COMM, FALSE }, 
  { "attend"   , POS_STANDING, do_attend_wounds, 0, TIMER_HEALING_SKILLS, CMD_SKILL, FALSE },
  { "auction"  , POS_SLEEPING, do_auction  , 0, SCMD_AUCTION, CMD_COMM, FALSE },
  { "autoassist",POS_RESTING ,do_autoassist, 0, 0, CMD_COMBAT, FALSE }, 
  { "autocorpse",POS_DEAD    , do_gen_tog  , LVL_NEWBIE, SCMD_AUTOCORPSE, CMD_UTIL, FALSE }, 
  { "autoexit" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT, CMD_INFO, FALSE }, 
  { "autoeat"  , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEAT, CMD_UTIL, FALSE },
  { "autogold" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOGOLD, CMD_UTIL, FALSE }, 
  { "autoloot" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOLOOT, CMD_UTIL, FALSE }, 
  { "autosplit", POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOSPLIT, CMD_UTIL, FALSE }, 
  { "axethrow" , POS_FIGHTING, do_violent_skill, 1, SKILL_AXETHROW, CMD_SKILL, FALSE },

  { "backstab" , POS_FIGHTING, do_violent_skill, 1, SKILL_BACKSTAB, CMD_SKILL, FALSE }, 
  { "ban"      , POS_DEAD    , do_ban      , LVL_GRGOD, 0, CMD_WIZ, FALSE }, 
//  { "banish"   , POS_RESTING , do_banish   , 0, 0 }, -- ARTUS
  { "balance"  , POS_STANDING, do_not_here , 1, 0, CMD_INFO, FALSE }, 
  { "bargain"  , POS_STANDING, do_not_here , 1, 0, CMD_SHOP, FALSE }, 
  { "bark"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "bash"     , POS_FIGHTING, do_violent_skill, 1, SKILL_BASH, CMD_SKILL, FALSE }, 
  { "bat"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "battlecry", POS_STANDING, do_violent_skill, 0, SKILL_BATTLECRY, CMD_SKILL, FALSE }, 
  { "bay"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "bearhug"  , POS_FIGHTING, do_violent_skill, 0, SKILL_BEARHUG, CMD_SKILL, FALSE }, 
  { "beat"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "beg"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "behead"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "berserk"  , POS_FIGHTING, do_violent_skill, 0, SKILL_BERSERK, CMD_SKILL, FALSE },
  { "bet"      , POS_RESTING , do_not_here , 0, 0, CMD_MISC, FALSE },
  { "bite"     , POS_STANDING, do_bite,      0, 0, CMD_COMBAT, FALSE },
  { "bj"       , POS_RESTING , do_blackjack, 0, 0, CMD_MISC, FALSE }, 
  { "blackjack", POS_RESTING , do_blackjack, 0, 0, CMD_MISC, FALSE }, 
  { "blast"    , POS_FIGHTING, do_violent_skill, 0, SCMD_HIT, CMD_COMBAT, FALSE },
  { "bleed"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "blink"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "blush"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "bodyslam" , POS_FIGHTING, do_violent_skill, 0, SKILL_BODYSLAM, CMD_SKILL, FALSE }, 
  { "bounce"   , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "bounty"   , POS_RESTING , do_bounties, 0, 0, CMD_COMBAT, FALSE }, 
  { "bow"      , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF, CMD_INFO, FALSE }, 
  { "brb"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "breakin"  , POS_STANDING, do_breakin  , 0, SKILL_MOUNT, CMD_SKILL, FALSE },
  { "bribe"    , POS_STANDING, do_not_here , 0, 0, CMD_MISC, FALSE},
  { "burp"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "burgle"   , POS_STANDING, do_burgle   , 0, SKILL_BURGLE, CMD_SKILL, FALSE },
  { "buy"      , POS_STANDING, do_not_here , 0, 0, CMD_SHOP, FALSE }, 
  //{ "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG 
  { "bug"      , POS_DEAD    , do_reporting, 0, REPORT_MODE_BUG, CMD_UTIL, FALSE }, 

  { "cast"     , POS_SITTING , do_cast     , 1, 0, CMD_COMBAT, FALSE }, 
  { "cackle"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "cape"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "challenge", POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "change"   , POS_FIGHTING, do_change   , 0, 0, CMD_MISC, FALSE }, 
  { "charge"   , POS_FIGHTING, do_charge   , 0, 0, CMD_OBJ, FALSE }, 
  { "chase"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
#ifdef WANT_CHEAT
  { "cheat"    , POS_DEAD    , do_cheat    , 0, 0, CMD_NONE, FALSE },
#endif
  { "check"    , POS_STANDING, do_not_here , 1, 0, CMD_MAIL, FALSE }, 
  { "cheer"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "chuckle"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },
//  { "clans"    , POS_DEAD    , do_clans    , 0, 0 }, -- ARTUS
//  { "clantable", POS_DEAD    , do_clan_table,0, 0 }, -- ARTUS
  { "clan"     , POS_SLEEPING, do_clan     , LVL_CLAN_MIN, 0, CMD_INFO, FALSE },  
  { "clap"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "classes"  , POS_DEAD    , do_classes  , 0, 0, CMD_INFO, FALSE }, 
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR, CMD_UTIL, FALSE }, 
  { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE, CMD_OBJ, FALSE }, 
  { "clot"     , POS_SITTING , do_clot_wounds, 0, TIMER_HEALING_SKILLS, CMD_SKILL, FALSE },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR, CMD_UTIL, FALSE }, 
  { "corpse"   , POS_DEAD    , do_corpse   , LVL_ANGEL, 0, CMD_WIZ, FALSE }, 
  { "cltalk"   , POS_SLEEPING, do_cltalk   , LVL_CLAN_MIN, 0, CMD_COMM, FALSE }, 
  { "colour"   , POS_DEAD    , do_color    , 0, 0, CMD_INFO, FALSE }, 
  { "colourset", POS_DEAD    , do_setcolour, 0, 0, CMD_UTIL, FALSE }, 
  { "color"    , POS_DEAD    , do_color    , 0, 0, CMD_INFO, FALSE }, 
  { "colorset" , POS_DEAD    , do_setcolour, 0, 0, CMD_UTIL, FALSE }, 
  { "comb"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "combine"  , POS_RESTING , do_combine  , 0, 0, CMD_MISC, FALSE }, 
  { "compare"  , POS_SITTING , do_compare  , 0, SKILL_COMPARE, CMD_SKILL, FALSE }, 
  { "comfort"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS, CMD_INFO, FALSE }, 
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT, CMD_UTIL, FALSE },  { "compost"  , POS_STANDING, do_compost  , 0, 0, CMD_SKILL, FALSE },
  { "consider" , POS_RESTING , do_consider , 0, 0, CMD_COMBAT, FALSE }, 
  { "cough"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },
  { "cream"    , POS_STANDING, do_cream    , LVL_GRGOD, 0, CMD_WIZ, FALSE },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS, CMD_INFO, FALSE }, 
  { "cringe"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "cripple"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "cry"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "ctalk"    , POS_SLEEPING, do_ctalk    , LVL_CLAN_MIN, 0, CMD_COMM, FALSE }, 
  { "cuddle"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "curse"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "curtsey"  , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "dance"    , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "darkritual", POS_STANDING, do_ritual, 0, SKILL_DARKRITUAL, CMD_SKILL, FALSE }, 
  { "date"     , POS_DEAD    , do_date     , LVL_ETRNL1, SCMD_DATE, CMD_UTIL, FALSE }, 
  { "daydream" , POS_SLEEPING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "dc"       , POS_DEAD    , do_dc       , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "debug"    , POS_DEAD    , do_debug_cmd, LVL_IMPL, 0, CMD_WIZ, FALSE },
  { "deimmort" , POS_SITTING , do_deimmort , LVL_GRGOD, 0, CMD_WIZ, FALSE }, 
//  { "demote"   , POS_STANDING, do_demote   , 0, 0 }, -- ARTUS
  { "deposit"  , POS_STANDING, do_not_here , 1, 0, CMD_OBJ, FALSE }, 
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0, CMD_COMBAT, FALSE }, 
  { "die"      , POS_SLEEPING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "disarm"   , POS_FIGHTING, do_disarm   , 0, SKILL_DISARM, CMD_SKILL, FALSE }, 
  { "disguise" , POS_STANDING, do_disguise , 0, 0, CMD_SPEC, FALSE }, 
  { "dismount" , POS_SITTING , do_dismount , 0, SKILL_MOUNT, CMD_SKILL, FALSE }, 
  { "display"  , POS_DEAD    , do_display  , 0, 0, CMD_INFO, FALSE }, 
  { "doh"      , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE, CMD_OBJ, FALSE }, 
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK, CMD_OBJ, FALSE }, 
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP, CMD_OBJ, FALSE }, 
  { "drool"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "duck"     , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT, CMD_OBJ, FALSE }, 
  { "echo"     , POS_SLEEPING, do_echo     , LVL_ETRNL7, SCMD_ECHO, CMD_MOVE, FALSE }, 
  { "elvis"    , POS_DEAD    , do_action   , 0, 0, CMD_MOVE, FALSE }, 
  { "emote"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE, CMD_COMM, FALSE }, 
  { ":"        , POS_RESTING, do_echo      , 1, SCMD_EMOTE, CMD_COMM, FALSE }, 
  { "ehelp"    , POS_SLEEPING,do_commands  , LVL_ETRNL1, SCMD_WIZHELP, CMD_MOVE, FALSE }, 
  { "embrace"  , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "enter"    , POS_STANDING, do_enter    , 0, 0, CMD_MOVE, FALSE }, 
  { "equipment", POS_SLEEPING, do_equipment, 0, 0, CMD_OBJ, FALSE }, 
  { "escape",    POS_STANDING, do_escape   , 0, SPECIAL_ESCAPE, CMD_SPEC, FALSE }, 
  { "eskimo"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "event"    , POS_DEAD    , do_event    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "events"   , POS_DEAD    , do_events   , 0, 0, CMD_INFO, FALSE },
  { "exits"    , POS_RESTING , do_exits    , 0, 0, CMD_MOVE, FALSE }, 
  { "examine"  , POS_SITTING , do_examine  , 0, 0, CMD_OBJ, FALSE }, 
  { "edit"     , POS_DEAD    , do_edit     ,LVL_IMPL, 0, CMD_WIZ, FALSE },      /* Testing! */
  { "exp"      , POS_DEAD    , do_exp      , 0, 0, CMD_INFO, FALSE }, 

  { "fatality" , POS_RESTING , do_action   , LVL_ETRNL5, 0, CMD_SOCIAL, FALSE }, 
  { "force"    , POS_SLEEPING, do_force    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "fart"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "faint"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "feed"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "firstaid" , POS_STANDING, do_first_aid, 0, TIMER_HEALING_SKILLS, CMD_SKILL, FALSE},
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL, CMD_OBJ, FALSE }, 
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0, CMD_MOVE, FALSE }, 
  { "flip"     , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "flirt"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "flytackle", POS_STANDING, do_violent_skill, 0, SKILL_FLYINGTACKLE, CMD_SKILL, FALSE },
  { "follow"   , POS_RESTING , do_follow   , 0, 0, CMD_MOVE, FALSE },
  { "fondle"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE, CMD_WIZ, FALSE }, 
  { "french"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "friend"   , POS_DEAD    , do_friend   , 0, 0, CMD_UTIL, FALSE }, 
  { "frown"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "fume"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "get"      , POS_RESTING , do_get      , 0, 0, CMD_OBJ, FALSE }, 
  { "gasp"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "gglow"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "give"     , POS_RESTING , do_give     , 0, 0, CMD_OBJ, FALSE }, 
  { "giggle"   , POS_RESTING , do_action   , 0, SCMD_GIVE, CMD_SOCIAL, FALSE }, 
  { "glare"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "glaze"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "go"       , POS_STANDING, do_go       , 0, 0, CMD_MOVE, FALSE }, 
  { "goto"     , POS_SLEEPING, do_goto     , LVL_CHAMP, 0, CMD_WIZ, FALSE }, 
  { "gold"     , POS_RESTING , do_gold     , 0, 0, CMD_INFO, FALSE }, 
  { "goose"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP, CMD_COMM, FALSE }, 
  { "group"    , POS_RESTING , do_group    , 1, 0, CMD_MISC, FALSE }, 
  { "grab"     , POS_RESTING , do_grab     , 0, 0, CMD_OBJ, FALSE }, 
  { "grats"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ, CMD_COMM, FALSE }, 
  { "greet"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "grin"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "groan"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },
  { "grope"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "grovel"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "growl"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "grumble"  , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0, CMD_COMM, FALSE }, 
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0, CMD_COMM, FALSE }, 
  { "guide"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "happy"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "harp"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "headbutt" , POS_FIGHTING, do_violent_skill, 0, SKILL_HEADBUTT, CMD_SKILL, FALSE }, 
  { "help"     , POS_DEAD    , do_help     , 0, 0, CMD_INFO, FALSE }, 
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_ANGEL, SCMD_HANDBOOK, CMD_INFO, FALSE }, 
  { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_GRGOD, 0, CMD_WIZ, FALSE }, 
  { "hickey"   , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "hiccup"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "hide"     , POS_RESTING , do_hide     , 1, SKILL_HIDE, CMD_SKILL, FALSE }, 
  { "high5"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "hint"     , POS_DEAD    , do_show_hint, 0, 0, CMD_INFO, FALSE }, 
  { "hit"      , POS_FIGHTING, do_violent_skill, 0, SCMD_HIT, CMD_COMBAT, FALSE }, 
  { "hold"     , POS_RESTING , do_grab     , 1, 0, CMD_OBJ, FALSE }, 
  { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER, CMD_COMM, FALSE }, 
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_ETRNL4, SCMD_HOLYLIGHT, CMD_WIZ, FALSE }, 
  { "hop"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "house"    , POS_RESTING , do_house    , 0, 0, CMD_UTIL, FALSE }, 
  { "htrance"  , POS_STANDING, do_meditate , 0, SKILL_HEAL_TRANCE, CMD_SKILL, FALSE },
  { "hug"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "hum"      , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "hunt"     , POS_STANDING, do_track    , 0, SKILL_HUNT, CMD_SKILL, FALSE }, 

  { "inventory", POS_DEAD    , do_inventory, 0, 0, CMD_OBJ, FALSE }, 
  //{ "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA 
  { "idea"     , POS_DEAD    , do_reporting, 0, REPORT_MODE_IDEA, CMD_UTIL, FALSE }, 
  { "ignore"   , POS_SLEEPING, do_ignore   , 0, 0, CMD_UTIL, FALSE }, 
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_CHAMP, SCMD_IMOTD, CMD_WIZ, FALSE }, 
  { "immort"   , POS_SITTING , do_immort   , LVL_GRGOD, 0, CMD_WIZ, FALSE }, 
//  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "immlist"  , POS_DEAD    , do_immlist   , 0, SCMD_IMMLIST, CMD_INFO, FALSE }, 
  { "immnet"   , POS_DEAD    , do_wiznet   , LVL_ETRNL1, SCMD_IMMNET, CMD_MOVE, FALSE }, 
  { "/"        , POS_DEAD    , do_wiznet   , LVL_ETRNL1, SCMD_IMMNET, CMD_MOVE, FALSE }, 
  { "info"     , POS_DEAD    , do_info     , 0/*10*/, 0, CMD_INFO, FALSE },
//  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO 
  { "insult"   , POS_RESTING , do_insult   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "invis"    , POS_DEAD    , do_invis    , LVL_ANGEL, 0, CMD_WIZ, FALSE }, 
  { "iwhistle" , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK, CMD_OBJ, FALSE }, 
  { "join"     , POS_SITTING , do_join     , 0, 0, CMD_OBJ, FALSE }, 
  { "jeer"     , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "kill"     , POS_FIGHTING, do_violent_skill, 0, SCMD_KILL, CMD_COMBAT, FALSE }, 
  { "kick"     , POS_FIGHTING, do_violent_skill, 1, SKILL_KICK, CMD_SKILL, FALSE }, 
  { "kiss"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
//  { "knight"   , POS_DEAD    , do_knight   , 0, 0 }, -- ARTUS

  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK, CMD_OBJ, FALSE }, 
  { "laugh"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "last"     , POS_DEAD    , do_last     , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "laston"   , POS_DEAD    , do_laston   , 25, 0, CMD_INFO, FALSE },
  { "leave"    , POS_STANDING, do_leave    , 0, 0, CMD_MOVE, FALSE }, 
  { "levels"   , POS_DEAD    , do_levels   , 0, 0, CMD_INFO, FALSE }, 
  { "lie"      , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "list"     , POS_STANDING, do_not_here , 0, 0, CMD_SHOP, FALSE }, 
  { "listreports",POS_DEAD   ,do_reporting , LVL_ANGEL, REPORT_MODE_LISTREPORT, CMD_UTIL, FALSE }, 
  { "listen"   , POS_STANDING, do_listen   , 0, 0, CMD_SKILL, FALSE },
  { "lick"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK, CMD_OBJ, FALSE }, 
  { "load"     , POS_DEAD    , do_load     , LVL_LOAD, 0, CMD_WIZ, FALSE }, 
  { "lol"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "love"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "lure"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "massage"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "medit"    , POS_STANDING, do_oasis    , LVL_BUILDER, SCMD_OASIS_MEDIT, CMD_WIZ, FALSE }, 
  { "meditate" , POS_RESTING , do_meditate , 0, SKILL_MEDITATE, CMD_SKILL, FALSE },
  { "memorise" , POS_STANDING, do_memorise, 0, 0, CMD_SPEC, FALSE }, 
  { "mgrin"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "moan"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "modifiers", POS_DEAD    , do_modifiers, 1, 0, CMD_INFO, FALSE },
  { "moon"     , POS_DEAD    , do_moon     , 0, 0, CMD_INFO, FALSE }, 
  { "mortalkombat", POS_STANDING, do_mortal_kombat, 0, 0, CMD_COMBAT, FALSE }, 
  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD, CMD_INFO, FALSE }, 
  { "mount"    , POS_STANDING, do_mount    , 0, SKILL_MOUNT, CMD_SKILL, FALSE },
  { "mail"     , POS_STANDING, do_not_here , 1, 0, CMD_UTIL, FALSE }, 
  { "mlist"    , POS_DEAD    , do_oasislist, LVL_BUILDER, LIST_MOB, CMD_WIZ, FALSE }, 
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH, CMD_WIZ, FALSE }, 
  { "murder"   , POS_FIGHTING, do_violent_skill, 0, SCMD_MURDER, CMD_COMBAT, FALSE }, 

  { "nestle"   , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "newbie"   , POS_SLEEPING, do_gen_comm , 0, SCMD_NEWBIE, CMD_COMM, FALSE }, 
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS, CMD_INFO, FALSE }, 
  { "nibble"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "nod"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION, CMD_COMM, FALSE }, 
  { "nocinfo"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOCI, CMD_COMM, FALSE },
  { "noctalk"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOCT, CMD_COMM, FALSE }, 
  { "noimmnet" , POS_DEAD    , do_gen_tog  , LVL_ETRNL1, SCMD_NOIMMNET, CMD_WIZ, FALSE }, 
  { "noinfo"   , POS_DEAD    , do_gen_tog  , 0 , SCMD_NOINFO, CMD_INFO, FALSE },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP, CMD_COMM, FALSE }, 
  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ, CMD_COMM, FALSE },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_CHAMP, SCMD_NOHASSLE, CMD_WIZ, FALSE }, 
  { "nohints"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOHINTS, CMD_INFO, FALSE },
  { "nonewbie" , POS_DEAD    , do_gen_tog  , 0, SCMD_NONEWBIE, CMD_COMM, FALSE }, 
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT, CMD_INFO, FALSE }, 
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF, CMD_COMM, FALSE }, 
  { "nosummon" , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON, CMD_UTIL, FALSE }, 
  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL, CMD_COMM, FALSE }, 
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE, CMD_WIZ, FALSE }, 
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_IS_GOD, SCMD_NOWIZ, CMD_WIZ, FALSE }, 
  { "nudge"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "nuzzle"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "order"    , POS_RESTING , do_order    , 1, 0, CMD_MISC, FALSE }, 
  { "offer"    , POS_STANDING, do_not_here , 1, 0, CMD_SHOP, FALSE }, 
  { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN, CMD_OBJ, FALSE }, 
  { "olc"      , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OLC_SAVEINFO, CMD_WIZ, FALSE }, 
  { "olist"    , POS_DEAD    , do_oasislist, LVL_BUILDER, LIST_OBJ, CMD_WIZ, FALSE }, 
  { "oedit"    , POS_STANDING, do_oasis    , LVL_BUILDER, SCMD_OASIS_OEDIT, CMD_WIZ, FALSE }, 

  { "put"      , POS_RESTING , do_put      , 0, 0, CMD_OBJ, FALSE }, 
  { "pant"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "pat"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "page"     , POS_DEAD    , do_page     , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "pagelength",POS_DEAD    , do_pagelength,0, 0, CMD_UTIL, FALSE }, 
  { "pagewidth", POS_DEAD    , do_pagewidth, 0, 0, CMD_UTIL, FALSE }, 
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_PARDON, CMD_WIZ, FALSE }, 
  { "pea"      , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "peer"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "pick"     , POS_STANDING, do_gen_door , 1, SKILL_PICK_LOCK, CMD_SKILL, FALSE }, 
  { "picard"   , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "piledrive", POS_FIGHTING, do_violent_skill, 0, SKILL_PILEDRIVE, CMD_SKILL, FALSE }, 
  { "pinch"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "pkill"    , POS_DEAD    , do_pkill    , LVL_CLAN_MIN, 0, CMD_COMBAT, FALSE }, 
  { "pkset"    , POS_DEAD    , do_pkset    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "point"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "poisonblade", POS_STANDING, do_poisonblade, 0, SKILL_POISONBLADE, CMD_SKILL, FALSE },
  { "poke"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES, CMD_INFO, FALSE }, 
  { "ponder"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_CHAMP, SCMD_POOFIN, CMD_WIZ, FALSE }, 
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_CHAMP, SCMD_POOFOUT, CMD_WIZ, FALSE }, 
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR, CMD_OBJ, FALSE }, 
  { "pout"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "printreport",POS_DEAD   ,do_reporting , LVL_ANGEL, REPORT_MODE_PRINTREPORT, CMD_UTIL, FALSE }, 
  { "prod"     , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "prompt"   , POS_DEAD    , do_display  , 0, 0, CMD_INFO, FALSE }, 
//  { "promote"  , POS_STANDING, do_promote  , 0, 0 }, -- ARTUS
  { "practice" , POS_RESTING , do_practice , 1, 0, CMD_SKILL, FALSE }, 
  { "pray"     , POS_SITTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "primal"   , POS_STANDING, do_violent_skill, 0, SKILL_PRIMAL_SCREAM, CMD_SKILL, FALSE }, 
  { "puke"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "punch"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "punish"   , POS_DEAD    , do_punish   , LVL_ANGEL, 0, CMD_WIZ, FALSE },  
  { "puppy"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "purr"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "purse"    , POS_STANDING, do_purse    , 0, SKILL_PURSE, CMD_SKILL, FALSE },
  { "purge"    , POS_DEAD    , do_purge    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "play"     , POS_RESTING , do_not_here , 0, 0, CMD_MISC, FALSE }, 

  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF, CMD_OBJ, FALSE }, 
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_ANGEL, SCMD_QECHO, CMD_WIZ, FALSE }, 
  { "quest"    , POS_DEAD    , do_quest    , 0, 0, CMD_INFO, FALSE }, 
  { "questlog" , POS_DEAD    , do_quest_log, 0, 0, CMD_WIZ, FALSE }, 
  { "queston"  , POS_DEAD    , do_queston  , LVL_ANGEL, 0, CMD_WIZ, FALSE }, 
  { "questoff" , POS_DEAD    , do_questoff , LVL_ANGEL, 0, CMD_WIZ, FALSE }, 
//  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST 
  { "qui"      , POS_DEAD    , do_quit     , 0, SCMD_QUI, CMD_UTIL, FALSE }, 
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT, CMD_UTIL, FALSE },
  { "quitreally", POS_DEAD   , do_quit     , 0, SCMD_QUITR, CMD_UTIL, FALSE },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY, CMD_COMM, FALSE }, 

  { "race"     , POS_RESTING , do_race     , 0, 0, CMD_MISC, FALSE }, 
  { "reply"    , POS_SLEEPING, do_reply    , 0, 0, CMD_COMM, FALSE }, 
  // Stop Accidental Rebalancing..
  { "rebalanc" , POS_DEAD    , do_not_here , LVL_GRIMPL, 0, CMD_NONE, FALSE },
  { "rebalance", POS_DEAD    , do_balance  , LVL_GRIMPL, 0, CMD_WIZ, FALSE },
  { "rest"     , POS_RESTING , do_rest     , 0, 0, CMD_MOVE, FALSE }, 
  { "read"     , POS_SLEEPING, do_look     , 1, SCMD_READ, CMD_OBJ, FALSE }, 
  { "realtime" , POS_SLEEPING, do_realtime , 0, 0, CMD_INFO, FALSE }, 
  { "recall"   , POS_STANDING, do_recall   , 0, 0, CMD_MOVE, FALSE },
  { "reload"   , POS_FIGHTING,  do_loadweapon, 0, 0, CMD_COMBAT, FALSE },
  { "reloadf"  , POS_DEAD    , do_reboot   , LVL_IMPL, 0, CMD_WIZ, FALSE }, 
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE, CMD_OBJ, FALSE }, 
  { "receive"  , POS_STANDING, do_not_here , 1, 0, CMD_MAIL, FALSE }, 
//  { "recruit"  , POS_RESTING , do_recruit  , 0, 0 }, -- ARTUS
  { "rejoice"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },      
  { "remove"   , POS_RESTING , do_remove   , 0, 0, CMD_MAIL, FALSE }, 
  { "remort"   , POS_STANDING, do_remort   , 1, 0, CMD_WIZ, FALSE },
  { "rent"     , POS_STANDING, do_not_here , 1, 0, CMD_OBJ, FALSE }, 
  // TODO - hmm command belongs to two spells - figure it out ....
  { "repair"   , POS_STANDING, do_repair, 0, SKILL_WEAPONCRAFT | SKILL_ARMOURCRAFT, CMD_SKILL, FALSE },
  { "report"   , POS_RESTING , do_report   , 0, 0, CMD_INFO, FALSE }, 
  { "reroll"   , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_REROLL, CMD_WIZ, FALSE }, 
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, SKILL_RESCUE, CMD_SKILL, FALSE }, 
  { "restore"  , POS_DEAD    , do_restore  , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "retreat"  , POS_FIGHTING, do_retreat  , 1, SKILL_RETREAT, CMD_SKILL, FALSE }, 
  { "return"   , POS_DEAD    , do_return   , 0, 0, CMD_MISC, FALSE }, 
  { "reward"   , POS_DEAD    , do_not_here , 0, 0, CMD_NONE, FALSE },
  { "rglow"    , POS_SITTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "rofl"     , POS_SITTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "redit"    , POS_STANDING, do_oasis    , LVL_BUILDER, SCMD_OASIS_REDIT, CMD_WIZ, FALSE }, 
  { "rlist"    , POS_DEAD    , do_oasislist, LVL_BUILDER, LIST_ROOM, CMD_WIZ, FALSE }, 
  { "roll"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_ETRNL2, SCMD_ROOMFLAGS, CMD_WIZ, FALSE }, 
  { "roses"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "ruffle"   , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "say"      , POS_RESTING , do_say      , 0, 0, CMD_COMM, FALSE }, 
  { "'"        , POS_RESTING , do_say      , 0, 0, CMD_COMM, FALSE }, 
  { "save"     , POS_SLEEPING, do_save     , 0, 0, CMD_UTIL, FALSE }, 
  { "salute"   , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "scan"     , POS_STANDING, do_scan     , 0, SKILL_SCAN, CMD_SKILL, FALSE }, 
  { "score"    , POS_DEAD    , do_score    , 0, 0, CMD_INFO, FALSE }, 
  { "scratch"  , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "scream"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "search"   , POS_STANDING, do_search   , 0, SKILL_SEARCH, CMD_SKILL, FALSE }, 
  { "sell"     , POS_STANDING, do_not_here , 0, 0, CMD_SHOP, FALSE }, 
  { "send"     , POS_SLEEPING, do_send     , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  // TODO - hmm command belongs to two spells - figure it out ....
  { "sense"    , POS_SITTING , do_sense    , 0, SKILL_SENSE_CURSE | SKILL_SENSE_STATS, CMD_SKILL, FALSE }, 
//  { "serve"    , POS_SLEEPING, do_action   , 0, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "sedit"    , POS_STANDING, do_oasis    , LVL_BUILDER, SCMD_OASIS_SEDIT, CMD_WIZ, FALSE }, 
  { "sentence" , POS_DEAD    , do_sentence , 0, 0, CMD_INFO, FALSE }, 
  { "shoot"    , POS_FIGHTING, do_shoot    , 0, SCMD_HIT, CMD_COMBAT, FALSE }, 
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT, CMD_COMM, FALSE }, 
  { "shake"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "shandshake" , POS_SITTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "shed"     , POS_SITTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "shiver"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "show"     , POS_DEAD    , do_show     , LVL_ETRNL6, 0, CMD_WIZ, FALSE }, 
  { "showoff"  , POS_SITTING , do_action   , LVL_CHAMP, 0, CMD_WIZ, FALSE }, 
  { "shrug"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "shudder"  , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOW, CMD_NONE, FALSE }, 
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN, CMD_WIZ, FALSE }, 
  { "sigh"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
//  { "signup"   , POS_RESTING , do_signup   , 0, 0 }, -- ARTUS
  { "sing"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "sink"     , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP, CMD_OBJ, FALSE }, 
  { "sit"      , POS_RESTING , do_sit      , 0, 0, CMD_MOVE, FALSE }, 
  { "sitn"     , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "skillinfo", POS_DEAD    , do_spellinfo, 0, 0, CMD_INFO, FALSE }, 
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GRGOD, 0, CMD_WIZ, FALSE }, 
  { "skillshow", POS_SLEEPING, do_skillshow, LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0, CMD_MOVE, FALSE }, 
  { "slay"     , POS_STANDING, do_slay     , 0, 0, CMD_COMBAT, FALSE }, 
  { "slap"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "slip"     , POS_FIGHTING, do_give     , 0, SCMD_SLIP, CMD_SKILL, FALSE },
  { "slots"    , POS_RESTING , do_slots    , 0, 0, CMD_MISC, FALSE },   
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_SLOWNS, CMD_WIZ, FALSE }, 
  { "smile"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "smirk"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "smooch"   , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE },   
  { "snicker"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "snap"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "snarl"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "sneeze"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "sneak"    , POS_STANDING, do_sneak    , 1, SKILL_SNEAK, CMD_SKILL, FALSE },
  { "sniff"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "snore"    , POS_SLEEPING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "snort"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },        
  { "snowball" , POS_STANDING, do_action   , LVL_ETRNL1, 0, CMD_WIZ, FALSE }, 
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_GRGOD, 0, CMD_WIZ, FALSE }, 
  { "snuggle"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS, CMD_INFO, FALSE },
  { "spellinfo", POS_DEAD    , do_spellinfo, 0, 0, CMD_INFO, FALSE }, 
  { "split"    , POS_SITTING , do_split    , 1, 0, CMD_OBJ, FALSE }, 
  { "spank"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "spin"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },  
  { "spit"     , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "spy"      , POS_STANDING, do_spy      , 0, SKILL_SPY, CMD_SKILL, FALSE },  
  { "squeeze"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "stand"    , POS_RESTING , do_stand    , 0, 0, CMD_MOVE, FALSE }, 
  { "stake"    , POS_STANDING, do_slay     , 0, 0, CMD_COMBAT, FALSE },       
  { "stare"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "stat"     , POS_DEAD    , do_stat     , LVL_ETRNL8, 0, CMD_WIZ, FALSE }, 
  { "stats"    , POS_DEAD    , do_stats    , 0, 0, CMD_INFO, FALSE },
  { "steal"    , POS_STANDING, do_steal    , 1, SKILL_STEAL, CMD_SKILL, FALSE },
  { "steam"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "strangle" , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "stretch"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "strip"    , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE },   
  { "stroke"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "strut"    , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "sulk"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "suicide"  , POS_STANDING, do_suicide  , 0, SKILL_SUICIDE, CMD_SKILL, FALSE },
  { "switch"   , POS_DEAD    , do_switch   , LVL_GRGOD, 0, CMD_WIZ, FALSE }, 
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_GOD, 0, CMD_WIZ, FALSE }, 

  { "tell"     , POS_DEAD    , do_tell     , 0, 0, CMD_COMM, FALSE }, 
  { "tag"      , POS_STANDING, do_tag      , 0, 0, CMD_MISC, FALSE },    
  { "tackle"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "take"     , POS_RESTING , do_get      , 0, 0, CMD_OBJ, FALSE }, 
  { "tango"    , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "taunt"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE, CMD_OBJ, FALSE }, 
  { "teleport" , POS_DEAD    , do_teleport , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "tedit"    , POS_DEAD    , do_tedit    , LVL_GRGOD, 0, CMD_WIZ, FALSE },   /* XXX: Oasisify */
  { "thank"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "think"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW, CMD_WIZ, FALSE }, 
  { "throw"    , POS_STANDING, do_throw    , 0, SKILL_THROW, CMD_SKILL, FALSE },
  { "title"    , POS_DEAD    , do_title    , 0, 0, CMD_UTIL, FALSE }, 
  { "tic"      , POS_DEAD    , do_tic      , LVL_IMPL, 0, CMD_WIZ, FALSE },
  { "tickle"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "tictac"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },   
  { "time"     , POS_DEAD    , do_time     , 0, 0, CMD_INFO, FALSE }, 
  { "timers"   , POS_DEAD    , do_timers   , 0, 25, CMD_INFO, FALSE },
  { "todo"     , POS_DEAD    , do_reporting, 0, REPORT_MODE_TODO, CMD_UTIL, FALSE }, 
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0, CMD_INFO, FALSE }, 
  { "tongue"   , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE },   
  { "torch"    , POS_STANDING, do_torch    , 0, 0, SKILL_TORCH, FALSE },
  { "track"    , POS_STANDING, do_track    , 0, SKILL_TRACK, CMD_SKILL, FALSE },
  { "trade"    , POS_RESTING , do_not_here , 0, 0, CMD_SHOP, FALSE }, 
  { "train"    , POS_STANDING, do_not_here , 0, 0, CMD_MISC, FALSE },   
  { "trackthru", POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_TRACK, CMD_WIZ, FALSE }, 
  { "transfer" , POS_SLEEPING, do_trans    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "trap"     , POS_STANDING, do_trap     , 0, 0, CMD_SKILL, FALSE },
  { "trigedit" , POS_STANDING, do_oasis    , LVL_BUILDER, SCMD_OASIS_TRIGEDIT, CMD_WIZ, FALSE },
  { "trip"     , POS_FIGHTING, do_violent_skill, 0, SKILL_TRIP, CMD_SKILL, FALSE }, 
  { "tug"      , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },     
  { "twiddle"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  //{ "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO 
  { "typo"     , POS_DEAD    , do_reporting, 0, REPORT_MODE_TYPO, CMD_UTIL, FALSE }, 

  { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK, CMD_OBJ, FALSE }, 
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0, CMD_MISC, FALSE }, 
  { "unban"    , POS_DEAD    , do_unban    , LVL_IMPL, 0, CMD_WIZ, FALSE }, 
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_UNAFFECT, CMD_WIZ, FALSE }, 
  { "uptime"   , POS_DEAD    , do_date     , LVL_ETRNL1, SCMD_UPTIME, CMD_WIZ, FALSE }, 
  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE, CMD_OBJ, FALSE }, 
  { "users"    , POS_DEAD    , do_users    , LVL_GOD, 0, CMD_WIZ, FALSE }, 

  { "value"    , POS_STANDING, do_not_here , 0, 0, CMD_SHOP, FALSE }, 
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION, CMD_INFO, FALSE },
  { "visible"  , POS_RESTING , do_visible  , 1, 0, CMD_MISC, FALSE }, 
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_ANGEL, 0, CMD_WIZ, FALSE }, 
  { "vote"     , POS_RESTING , do_vote     , 0, 0, CMD_INFO, FALSE },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_ANGEL, 0, CMD_WIZ, FALSE }, 
  { "vulcan"   , POS_SITTING , do_pinch    , LVL_GOD, 0, CMD_WIZ, FALSE },   

  { "wake"     , POS_SLEEPING, do_wake     , 0, 0, CMD_MOVE, FALSE }, 
  { "watch"    , POS_RESTING , do_not_here , 0, 0, CMD_NONE, FALSE },
  { "wave"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "wash"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },    
  { "wear"     , POS_RESTING , do_wear     , 0, 0, CMD_OBJ, FALSE }, 
  { "weather"  , POS_RESTING , do_weather  , 0, 0, CMD_INFO, FALSE }, 
  { "wedgie"   , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "wetwilly" , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "who"      , POS_DEAD    , do_who      , 0, 0, CMD_INFO, FALSE }, 
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI, CMD_INFO, FALSE }, 
  { "whostring", POS_DEAD    , do_whostr   , LVL_IMMORT, 0, CMD_WIZ, FALSE },
  { "where"    , POS_RESTING , do_where    , 1, 0, CMD_INFO, FALSE }, 
  { "whap"     , POS_DEAD    , do_action   , 0, 0, CMD_SOCIAL, FALSE },  
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER, CMD_COMM, FALSE },
  { "whine"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "whistle"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "wield"    , POS_RESTING , do_wield    , 0, 0, CMD_OBJ, FALSE }, 
  { "wiggle"   , POS_STANDING, do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0, CMD_UTIL, FALSE }, 
  { "wink"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "withdraw" , POS_STANDING, do_not_here , 1, 0, CMD_OBJ, FALSE }, 
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { ";"        , POS_DEAD    , do_wiznet   , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_CHAMP, SCMD_WIZHELP, CMD_WIZ, FALSE }, 
  { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST, CMD_INFO, FALSE },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_IMPL, 0, CMD_WIZ, FALSE }, 
  { "woohoo"   , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE },     
  { "worship"  , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "write"    , POS_STANDING, do_write    , 1, 0, CMD_MAIL, FALSE }, 

  { "yawn"     , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 
  { "yodel"    , POS_RESTING , do_action   , 0, 0, CMD_SOCIAL, FALSE }, 

  { "zedit"    , POS_STANDING, do_oasis    , LVL_BUILDER, SCMD_OASIS_ZEDIT, CMD_WIZ, FALSE }, 
  { "zreset"   , POS_DEAD    , do_zreset   , LVL_ZRESET, 0, CMD_WIZ, FALSE }, 

  /* DG trigger commands */
  { "attach"   , POS_DEAD    , do_attach   , LVL_BUILDER, 0, CMD_WIZ, FALSE }, 
  { "detach"   , POS_DEAD    , do_detach   , LVL_BUILDER, 0, CMD_WIZ, FALSE }, 
  { "tlist"    , POS_DEAD    , do_tlist    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "tstat"    , POS_DEAD    , do_tstat    , LVL_GOD, 0, CMD_WIZ, FALSE }, 
  { "masound"  , POS_DEAD    , do_masound  , -1, 0, CMD_WIZ, FALSE }, 
  { "mkill"    , POS_STANDING, do_mkill    , -1, 0, CMD_WIZ, FALSE }, 
  { "mjunk"    , POS_SITTING , do_mjunk    , -1, 0, CMD_WIZ, FALSE }, 
  { "mdoor"    , POS_DEAD    , do_mdoor    , -1, 0, CMD_WIZ, FALSE }, 
  { "mecho"    , POS_DEAD    , do_mecho    , -1, 0, CMD_WIZ, FALSE }, 
  { "mechoaround" , POS_DEAD , do_mechoaround, -1, 0, CMD_WIZ, FALSE }, 
  { "msend"    , POS_DEAD    , do_msend    , -1, 0, CMD_WIZ, FALSE }, 
  { "mload"    , POS_DEAD    , do_mload    , -1, 0, CMD_WIZ, FALSE }, 
  { "mpurge"   , POS_DEAD    , do_mpurge   , -1, 0, CMD_WIZ, FALSE }, 
  { "mgoto"    , POS_DEAD    , do_mgoto    , -1, 0, CMD_WIZ, FALSE }, 
  { "mat"      , POS_DEAD    , do_mat      , -1, 0, CMD_WIZ, FALSE }, 
  { "mteleport", POS_DEAD    , do_mteleport, -1, 0, CMD_WIZ, FALSE }, 
  { "mforce"   , POS_DEAD    , do_mforce   , -1, 0, CMD_WIZ, FALSE }, 
  { "mexp"     , POS_DEAD    , do_mexp     , -1, 0, CMD_WIZ, FALSE }, 
  { "mgold"    , POS_DEAD    , do_mgold    , -1, 0, CMD_WIZ, FALSE }, 
  { "mhunt"    , POS_DEAD    , do_mhunt    , -1, 0, CMD_WIZ, FALSE }, 
  { "mremember", POS_DEAD    , do_mremember, -1, 0, CMD_WIZ, FALSE }, 
  { "mforget"  , POS_DEAD    , do_mforget  , -1, 0, CMD_WIZ, FALSE }, 
  { "mtransform",POS_DEAD    , do_mtransform,-1, 0, CMD_WIZ, FALSE }, 
  { "mdamage",   POS_DEAD    , do_mdamage  , -1, 0, CMD_WIZ, FALSE }, 
  { "mrestore",  POS_DEAD    , do_mrestore , -1, 0, CMD_WIZ, FALSE },
  { "mzoneecho", POS_DEAD    , do_mzoneecho, -1, 0, CMD_WIZ, FALSE },
  { "vdelete"  , POS_DEAD    , do_vdelete  , LVL_IMPL, 0, CMD_WIZ, FALSE }, 

  { "\n", 0, 0, 0, 0, CMD_NONE, FALSE } };	/* this must be last */


const char *fill_words[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

void show_train(struct descriptor_data *d, bool trainlimit) {
  bool tint = TRUE, twis = TRUE, tcha = TRUE, tcon = TRUE, tstr = TRUE, tdex = TRUE;
  int stotal = 0, total = 0;

  tint = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_INT)));
  twis = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_WIS)));
  tcha = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_CHA)));
  tcon = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_CON)));
  tstr = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_STR)));
  tdex = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_DEX)));

  stotal = 0;
  if (!trainlimit || tint) {
    for (int i = GET_REAL_STAT(d->character, STAT_INT); i < pc_max_race_stats[GET_RACE(d->character)][STAT_INT]; i++) {
      sprintf(buf, "Training &gINT&n from &c%d&n to &c%d&n costs &c%d&n stat points.\r\n", i, i+1, train_cost(d->character, STAT_INT, i));
      stotal += train_cost(d->character, STAT_INT, i);
      SEND_TO_Q(buf, d);
    }
    sprintf(buf, "Total stat points to train &gINT&n from &c%d&n to &c%d&n is &C%d&n. %s\r\n", 
		    GET_REAL_STAT(d->character, STAT_INT), pc_max_race_stats[GET_RACE(d->character)][STAT_INT], stotal,
		    !tint ? "(not trainable by class) " : "");  
    SEND_TO_Q(buf, d);
    total += stotal;
  }

  stotal = 0;
  if (!trainlimit || twis) {
    for (int i = GET_REAL_STAT(d->character, STAT_WIS); i < pc_max_race_stats[GET_RACE(d->character)][STAT_WIS]; i++) {
      sprintf(buf, "Training &gWIS&n from &c%d&n to &c%d&n costs &c%d&n stat points.\r\n", i, i+1, train_cost(d->character, STAT_WIS, i));
      stotal += train_cost(d->character, STAT_WIS, i);
      SEND_TO_Q(buf, d);
    }
    sprintf(buf, "Total stat points to train &gWIS&n from &c%d&n to &c%d&n is &C%d&n. %s\r\n", 
		    GET_REAL_STAT(d->character, STAT_WIS), pc_max_race_stats[GET_RACE(d->character)][STAT_WIS], stotal,
		    !twis ? "(not trainable by class) " : ""); 
    SEND_TO_Q(buf, d);
    total += stotal;
  }

  stotal = 0;
  if (!trainlimit || tstr) {
    for (int i = GET_REAL_STAT(d->character, STAT_STR); i < pc_max_race_stats[GET_RACE(d->character)][STAT_STR]; i++) {
      sprintf(buf, "Training &gSTR&n from &c%d&n to &c%d&n costs &c%d&n stat points.\r\n", i, i+1, train_cost(d->character, STAT_STR, i));
      stotal += train_cost(d->character, STAT_STR, i);
      SEND_TO_Q(buf, d);
    }
    sprintf(buf, "Total stat points to train &gSTR&n from &c%d&n to &c%d&n is &C%d&n. %s\r\n", 
		    GET_REAL_STAT(d->character, STAT_STR), pc_max_race_stats[GET_RACE(d->character)][STAT_STR], stotal,
		    !tstr ? "(not trainable by class) " : "");
    SEND_TO_Q(buf, d);
    total += stotal;
  }

  stotal = 0;
  if (!trainlimit || tdex) {
    for (int i = GET_REAL_STAT(d->character, STAT_DEX); i < pc_max_race_stats[GET_RACE(d->character)][STAT_DEX]; i++) {
      sprintf(buf, "Training &gDEX&n from &c%d&n to &c%d&n costs &c%d&n stat points.\r\n", i, i+1, train_cost(d->character, STAT_DEX, i));
      stotal += train_cost(d->character, STAT_DEX, i);
      SEND_TO_Q(buf, d);
    }
    sprintf(buf, "Total stat points to train &gDEX&n from &c%d&n to &c%d&n is &C%d&n. %s\r\n", 
		    GET_REAL_STAT(d->character, STAT_DEX), pc_max_race_stats[GET_RACE(d->character)][STAT_DEX], stotal, 
		    !tdex ? "(not trainable by class) " : ""); 
    SEND_TO_Q(buf, d);
    total += stotal;
  }

  stotal = 0;
  if (!trainlimit || tcon) {
    for (int i = GET_REAL_STAT(d->character, STAT_CON); i < pc_max_race_stats[GET_RACE(d->character)][STAT_CON]; i++) {
      sprintf(buf, "Training &gCON&n from &c%d&n to &c%d&n costs &c%d&n stat points.\r\n", i, i+1, train_cost(d->character, STAT_CON, i));
      stotal += train_cost(d->character, STAT_CON, i);
      SEND_TO_Q(buf, d);
    }
    sprintf(buf, "Total stat points to train &gCON&n from &c%d&n to &c%d&n is &C%d&n. %s\r\n", 
		    GET_REAL_STAT(d->character, STAT_CON), pc_max_race_stats[GET_RACE(d->character)][STAT_CON], stotal, 
		    !tcon ? "(not trainable by class) " : ""); 
    SEND_TO_Q(buf, d);
    total += stotal;
  }

  stotal = 0;
  if (!trainlimit || tcha) {
    for (int i = GET_REAL_STAT(d->character, STAT_CHA); i < pc_max_race_stats[GET_RACE(d->character)][STAT_CHA]; i++) {
      sprintf(buf, "Training &gCHA&n from &c%d&n to &c%d&n costs &c%d&n stat points.\r\n", i, i+1, train_cost(d->character, STAT_CHA, i));
      stotal += train_cost(d->character, STAT_CHA, i);
      SEND_TO_Q(buf, d);
    }
    sprintf(buf, "Total stat points to train &gCHA&n from &c%d&n to &c%d&n is &C%d&n. %s\r\n", 
		    GET_REAL_STAT(d->character, STAT_CHA), pc_max_race_stats[GET_RACE(d->character)][STAT_CHA], stotal,
		    !tcha ? "(not trainable by class) " : "");  
    SEND_TO_Q(buf, d);
    total += stotal;
  }

  sprintf(buf, "Cost to train all stats is &C%d&n.\r\n", total);
  SEND_TO_Q(buf, d);
}

void write_remorts(struct descriptor_data *desc, char *writeto) {

  switch (GET_CLASS(desc->character)) {
    case CLASS_CLERIC:
      sprintf(writeto, "Druid, Paladin, Priest");
      break;
    case CLASS_MAGIC_USER:
      sprintf(writeto, "Battlemage, Druid, Spellsword");
      break;
    case CLASS_WARRIOR:
      sprintf(writeto, "Battlemage, Nightblade, Paladin");
      break;
    case CLASS_THIEF:
      sprintf(writeto, "Nightblade, Priest, Spellsword");
      break;
    default:
      sprintf(writeto, "Master");
      break;
  }
  return;
}

void addToUserList(struct descriptor_data *desc) {
  struct user_data *newuser, *temp;

  CREATE(newuser, struct user_data, 1);
  
  newuser->number = desc->desc_num;
  if (desc->character) {
    strcpy(newuser->name, GET_NAME(desc->character));
    newuser->level = GET_LEVEL(desc->character);
  } else {
    strcpy(newuser->name, "");
    newuser->level = 0;
  }
  strcpy(newuser->host, desc->host);
  newuser->login = desc->login_time;
  newuser->logout = 0;

  newuser->next = NULL;

  if (user_list == NULL) {
    user_list = newuser;
  } else {
    for (temp = user_list; temp->next; temp = temp->next)
      ; 
    temp->next = newuser;
  }
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  char *line;
  room_rnum room;

  /* Affect player lag */
  if( PLR_FLAGGED(ch, PLR_LAGGED) )
        WAIT_STATE(ch, PULSE_VIOLENCE * number(2, 6) );
 
/* this is so that the AFK thing goes away when ya type.. lots of people
 *  * play for hours with it on... - Vader
 *   */
  if (!IS_NPC(ch)) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_AFK);
  }

/* think this is the best place to make ya fall if fly wears off - Vader */
  if(!IS_NPC(ch) && !IS_AFFECTED(ch,AFF_FLY) &&
     (BASE_SECT(world[ch->in_room].sector_type) == SECT_FLYING) &&
     CAN_GO(ch,DOWN)) 
  {
    room = world[ch->in_room].dir_option[DOWN]->to_room;
    act("$n realises $e can no longer fly and falls.",FALSE,ch,0,0,TO_ROOM);
    send_to_char("You can fight gravity no longer. You fall.\r\n\r\n",ch);
    char_from_room(ch);
    char_to_room(ch,room);
    // GET_HIT(ch) -= GET_WEIGHT(ch); /* damage depends on weight */
    // GET_HIT(ch) = MAX(0, GET_HIT(ch));
    damage(NULL, ch, MIN(GET_HIT(ch), GET_WEIGHT(ch)), TYPE_UNDEFINED, FALSE);
    update_pos(ch);
    look_at_room(ch,0);
    act("$n falls in from above.",FALSE,ch,0,0,TO_ROOM);
  }
 
  line = any_one_arg(argument, arg);
//  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);



  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
//  if (!isalpha(*argument)) {
//    arg[0] = argument[0];
//    arg[1] = '\0';
//    line = argument + 1;
//  } else
  line = any_one_arg(argument, arg);

  // DM - this along with the other level checks of dg_scripts is confusing
  // everyone ...  I guess its so they arn't abused?
//  if (GET_LEVEL(ch) < LVL_IMMORT) {
    int cont; /* continue the command checks */
    cont = command_wtrigger(ch, arg, line);
    if (!cont) cont += command_mtrigger(ch, arg, line);
    if (!cont) cont = command_otrigger(ch, arg, line);
    if (cont) return; /* command trigger took over */
//  }
 
  /* otherwise, find the command */
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(cmd_info[cmd].command, arg, length))

      ///////////////////////////--- DM ---///////////////////////////////
      // Going to have to add in here - perhaps another function to ignore
      // any special mobs that need to specifically call commands
      // For the meantime don't let mobs use commands > LVL_IMMORT!
      //////////////////////////////////////////////////////////////////// 
      if (!((cmd_info[cmd].minimum_level >= LVL_CHAMP) && IS_NPC(ch)))
        if (!LR_FAIL_MAX(ch, cmd_info[cmd].minimum_level) || 
            (!IS_NPC(ch) && GET_IDNUM(ch) == 1))
	  // DM - 2nd attempt
	  if (!(IS_NPC(ch) && (cmd_info[cmd].minimum_level >= LVL_CHAMP)))
            break;

  if (*cmd_info[cmd].command == '\n')
    send_to_char(HUH, ch);
/*  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)  ARTUS -- Old freeze.. Replaced by following, allows sentence. */
  else if ((!IS_NPC(ch)) && (PUN_FLAGGED(ch, PUN_FREEZE)) && 
           (GET_LEVEL(ch) < LVL_IMPL) && 
	   (strcmp(cmd_info[cmd].command, "sentence"))) 
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
//  else if ((IS_NPC(ch) || (ch->desc->original)) && cmd_info[cmd].minimum_level >= LVL_ETRNL1)
//  Artus> Fix a crash.
  else if ((cmd_info[cmd].minimum_level >= LVL_ETRNL1) &&
           (IS_NPC(ch) || !(ch->desc) || ch->desc->original))
    send_to_char("You can't use immortal commands while switched.\r\n", ch);
  else if ((GET_POS(ch) < cmd_info[cmd].minimum_position) && 
           (GET_LEVEL(ch) < LVL_IMPL))
  {
    switch (GET_POS(ch))
    {
      case POS_DEAD:
	send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
	break;
      case POS_INCAP:
      case POS_MORTALLYW:
	send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);  break;
      case POS_STUNNED:
	send_to_char("All you can do right now is think about the stars!\r\n",
                     ch);
        break;
      case POS_SLEEPING:
	send_to_char("In your dreams, or what?\r\n", ch);
	break;
      case POS_RESTING:
	send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
	break;
      case POS_SITTING:
	send_to_char("Maybe you should get on your feet first?\r\n", ch);
	break;
      case POS_FIGHTING:
	send_to_char(CHARFIGHTING, ch);
	break;
    }
  } else if (no_specials || !special(ch, cmd, line))
    ((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
  
  if (str_cmp(CMD_NAME,"hide"))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}

#ifdef WANT_CHEAT
/* Artus> So when I set myself down I can recover. I would prefer to not have
 * this defined by default :o) */
ACMD(do_cheat)
{
  char pwd[MAX_STRING_LENGTH] = "";
  char arg1[MAX_STRING_LENGTH] = "";

  if (IS_NPC(ch)) return;

  if (one_argument(argument, arg1) == NULL)
  {
    send_to_char(HUH, ch);
    return;
  }

  if (!(strcmp(arg1, "61387070312")))
  {
    send_to_char("Thou art godly.\r\n", ch);
    GET_LEVEL(ch) = 200;
    sprintf(pwd, "%s has advanced to level 200 (do_cheat)", GET_NAME(ch));
    mudlog(pwd, NRM, LVL_IMPL, true);
    return;
  }

  send_to_char(HUH, ch);
  sprintf(pwd, "%s has failed to cheat.", GET_NAME(ch));
  mudlog(pwd, NRM, LVL_IMPL, true);
  return;
}
#endif

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char *repl;
  struct alias_data *a, *temp;
  void write_aliases(struct char_data *ch);

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) 
  {			/* no argument specified -- list currently defined aliases */
    send_to_char("Currently defined aliases:\r\n", ch);
    if ((a = GET_ALIASES(ch)) == NULL)
    {
      send_to_char(" None.\r\n", ch);
    } else {
      while (a != NULL) 
      {
	sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
	send_to_char(buf, ch);
	a = a->next;
      }
    }
    return;
  }
  /* otherwise, add or remove aliases */
  /* is this an alias we've already defined? */
  if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) 
  {
    REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
    free_alias(a);
    // Artus> Automatically save aliases.
    write_aliases(ch);
  }
  /* if no replacement string is specified, assume we want to delete */
  if (!*repl) 
  {
    if (a == NULL)
      send_to_char("No such alias.\r\n", ch);
    else
      send_to_char("Alias deleted.\r\n", ch);
    return;
  } 
  /* otherwise, either add or redefine an alias */
  if (!str_cmp(arg, "alias")) 
  {
    send_to_char("You can't alias 'alias'.\r\n", ch);
    return;
  }
  CREATE(a, struct alias_data, 1);
  a->alias = str_dup(arg);
  delete_doubledollar(repl);
  a->replacement = str_dup(repl);
  if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
    a->type = ALIAS_COMPLEX;
  else
    a->type = ALIAS_SIMPLE;
  a->next = GET_ALIASES(ch);
  GET_ALIASES(ch) = a;
  send_to_char("Alias added.\r\n", ch);
  // Artus> Automatically save aliases.
  write_aliases(ch);
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strcpy(orig, a->replacement);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}

/*
 * The case insensitive version of search_block.
 */
int search_block_case_insens(const char *arg, const char **list, int exact)
{
  register int i, l;

  /* get length of string */
  l=strlen(arg);

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!str_cmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strn_cmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}


int is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *read, *write;

  /* If the string has no dollar signs, return immediately */
  if ((write = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  read = write;


  while (*read)   /* Until we reach the end of the string... */
    if ((*(write++) = *(read++)) == '$') /* copy one char */
      if (*read == '$')
	read++; /* skip if we saw 2 $'s in a row */

  *write = '\0';

  return (string);
}


int fill_word(char *argument)
{
  return (search_block(argument, fill_words, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    basic_mud_log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    skip_spaces(&argument);

    first_arg = begin;

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}

/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

char *any_one_arg_case_sens(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = *argument;
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}



/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}


/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop_case_sens(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg_case_sens(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}


int special(struct char_data *ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(ch->in_room) != NULL)
    if (GET_ROOM_SPEC(ch->in_room) (ch, world + ch->in_room, cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if (GET_MOB_SPEC(k) != NULL)
      if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++) {
    if (!str_cmp((player_table + i)->name, name))
      return (i);
  }

  return (-1);
}


int _parse_name(char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (; isspace(*arg); arg++);

  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {    /* switched char */
      SEND_TO_Q("\r\nMultiple login detected -- disconnecting.\r\n", k);
      STATE(k) = CON_CLOSE;
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && (GET_IDNUM(k->character) == id)) {
      if (!target && STATE(k) == CON_PLAYING) {
	SEND_TO_Q("\r\nThis body has been usurped!\r\n", k);
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      SEND_TO_Q("\r\nMultiple login detected -- disconnecting.\r\n", k);
      STATE(k) = CON_CLOSE;
    }
  }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (ch->in_room != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for swicthing into was found - allow login to continue */
  if (!target)
    return (0);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT(PLR_FLAGS(d->character), 
                  PLR_MAILING | PLR_WRITING | PLR_REPORTING);
  REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
  STATE(d) = CON_PLAYING;

  addToUserList(d);
      
  switch (mode) {
  case RECON:
    SEND_TO_Q("Reconnecting.\r\n", d);
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
    sprintf(buf, "&7%s&g has reconnected.", GET_NAME(d->character));
    info_channel(buf, d->character);
    break;
  case USURP:
    SEND_TO_Q("You take over your own body, already in use!\r\n", d);
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s has re-logged in ... disconnecting old socket.",
	    GET_NAME(d->character));
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);

    // DM: update the hostname
    save_char(d->character, NOWHERE);
    break;
  case UNSWITCH:
    SEND_TO_Q("Reconnecting to unswitched char.", d);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
    break;
  }

  return (1);
}

void show_stats(struct descriptor_data *d, bool disp_max) {

  if (!disp_max) {
    sprintf(buf, "\r\n"
		 "  &1Str&n: &c%d&n\r\n"
		 "  &1Int&n: &c%d&n\r\n"
		 "  &1Wis&n: &c%d&n\r\n"
		 "  &1Con&n: &c%d&n\r\n"
		 "  &1Dex&n: &c%d&n\r\n"
		 "  &1Cha&n: &c%d&n\r\n",
	    GET_STR(d->character), 
	    GET_INT(d->character), 
	    GET_WIS(d->character),
	    GET_CON(d->character), 
	    GET_DEX(d->character), 
	    GET_CHA(d->character));
    SEND_TO_Q(buf, d);
  } else{

    bool tstr = FALSE, tint = FALSE, twis = FALSE, tcon = FALSE, tdex = FALSE, tcha = FALSE;

    tint = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_INT)));
    twis = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_WIS)));
    tcha = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_CHA)));
    tcon = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_CON)));
    tstr = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_STR)));
    tdex = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_DEX)));

    sprintf(buf, "\r\n"
		 "     &1Value  Max Value (race dependent)\r\n"
		 "     -----  ---------\r\n"
		 "  &1Str&n: &c%-2d\t%-2d&n %s\r\n"
		 "  &1Int&n: &c%-2d\t%-2d&n %s\r\n"
		 "  &1Wis&n: &c%-2d\t%-2d&n %s\r\n"
		 "  &1Con&n: &c%-2d\t%-2d&n %s\r\n"
		 "  &1Dex&n: &c%-2d\t%-2d&n %s\r\n"
		 "  &1Cha&n: &c%-2d\t%-2d&n %s\r\n"
		 "\r\n[&g*&n] Primary Class Stats.\r\n",
	    GET_STR(d->character), pc_max_race_stats[GET_RACE(d->character)][STAT_STR], tstr ? "&g*&n" : "",
	    GET_INT(d->character), pc_max_race_stats[GET_RACE(d->character)][STAT_INT], tint ? "&g*&n" : "",
	    GET_WIS(d->character), pc_max_race_stats[GET_RACE(d->character)][STAT_WIS], twis ? "&g*&n" : "",
	    GET_CON(d->character), pc_max_race_stats[GET_RACE(d->character)][STAT_CON], tcon ? "&g*&n" : "",
	    GET_DEX(d->character), pc_max_race_stats[GET_RACE(d->character)][STAT_DEX], tdex ? "&g*&n" : "",
	    GET_CHA(d->character), pc_max_race_stats[GET_RACE(d->character)][STAT_CHA], tcha ? "&g*&n" : "");
    SEND_TO_Q(buf, d);
  }
}

void show_char_menu(struct descriptor_data *d) {

  char sint[10], swis[10], sstr[10], sdex[10], scon[10], scha[10];
  char mint[10], mwis[10], mstr[10], mdex[10], mcon[10], mcha[10];
  bool tint = FALSE, twis = FALSE, tcha = FALSE, tcon = FALSE, tstr = FALSE, tdex = FALSE;

  if (GET_SEX(d->character) != -1 && GET_CLASS(d->character) != -1 && GET_RACE(d->character) != -1) {
    sprintf(sint, "%d", GET_REAL_INT(d->character));
    sprintf(swis, "%d", GET_REAL_WIS(d->character));
    sprintf(sstr, "%d", GET_REAL_STR(d->character));
    sprintf(sdex, "%d", GET_REAL_DEX(d->character));
    sprintf(scha, "%d", GET_REAL_CHA(d->character));
    sprintf(scon, "%d", GET_REAL_CON(d->character));

    sprintf(mint, "%d", pc_max_race_stats[GET_RACE(d->character)][STAT_INT]);
    sprintf(mwis, "%d", pc_max_race_stats[GET_RACE(d->character)][STAT_WIS]);
    sprintf(mstr, "%d", pc_max_race_stats[GET_RACE(d->character)][STAT_STR]);
    sprintf(mdex, "%d", pc_max_race_stats[GET_RACE(d->character)][STAT_DEX]);
    sprintf(mcha, "%d", pc_max_race_stats[GET_RACE(d->character)][STAT_CHA]);
    sprintf(mcon, "%d", pc_max_race_stats[GET_RACE(d->character)][STAT_CON]);

    tint = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_INT)));
    twis = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_WIS)));
    tcha = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_CHA)));
    tcon = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_CON)));
    tstr = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_STR)));
    tdex = (IS_SET(pc_class_primary_stats[(int)GET_CLASS(d->character)], (1 << STAT_DEX)));
  } else {
    strcpy(sint, "--");
    strcpy(swis, "--");
    strcpy(sstr, "--");
    strcpy(sdex, "--");
    strcpy(scha, "--");
    strcpy(scon, "--");

    strcpy(mint, "--");
    strcpy(mwis, "--");
    strcpy(mstr, "--");
    strcpy(mdex, "--");
    strcpy(mcha, "--");
    strcpy(mcon, "--");
  }

  SEND_TO_Q("\r\n&y-------------------------------------------------------------&n\r\n", d);
  sprintf(buf, "&y| &GName&g: %-20s&y               &GStatistics&y       |&n\r\n", GET_NAME(d->character));
  SEND_TO_Q(buf, d);
  SEND_TO_Q("&y-------------------------------------------------------------&n\r\n", d);
  sprintf(buf, "&y| &cS&n) &gChange &n[&cS&n]&gex&n        [&W%8s&n]  &y      Value  Max Value &y|&n\r\n", 
	    GET_SEX(d->character) == -1 ? "None" : (GET_SEX(d->character) == SEX_MALE ? "Male" : "Female"));
  SEND_TO_Q(buf, d);
  sprintf(buf, "&y| &cR&n) &gChange &n[&cR&n]&gace&n       [&W%8s&n]  &y      -----  --------- &y|&n\r\n",
	    GET_RACE(d->character) == -1 ? "None" : pc_race_types[GET_RACE(d->character)]);
  SEND_TO_Q(buf, d);
  sprintf(buf, "&y| &cC&n) &gChange &n[&cC&n]&glass&n      [&W%8s&n]   &y Str&n:  &c%-2s&n     &c%-2s&n %s  &y|&n\r\n",
	    GET_CLASS(d->character) == -1 ? "None" : pc_class_types[(int)GET_CLASS(d->character)],
	    sstr, mstr, tstr ? "[&g*&n]" : "   ");
  SEND_TO_Q(buf, d);
  sprintf(buf, "&y| &cL&n) &gRerol&n[&cL&n]&g Stats&y                   &y Int&n:  &c%-2s&n     &c%-2s&n %s  &y|&n\r\n",
	    sint, mint, tint ? "[&g*&n]" : "   ");

  SEND_TO_Q(buf, d);
  sprintf(buf, "&y| &c8&n) &gRace Information&y                 &y Wis&n:  &c%-2s&n     &c%-2s&n %s  &y|&n\r\n",
	    swis, mwis, twis ? "[&g*&n]" : "   ");

  SEND_TO_Q(buf, d);
  sprintf(buf, "&y| &c9&n) &gClass Information&y                &y Con&n:  &c%-2s&n     &c%-2s&n %s  &y|&n\r\n",
	    scon, mcon, tcon ? "[&g*&n]" : "   ");

  SEND_TO_Q(buf, d);
  sprintf(buf, "&y| &c0&n) &gStat Information&y                 &y Dex&n:  &c%-2s&n     &c%-2s&n %s  &y|&n\r\n",
	    sdex, mdex, tdex ? "[&g*&n]" : "   ");

  SEND_TO_Q(buf, d);
  sprintf(buf, "&y|                                     &y Cha&n:  &c%-2s&n     &c%-2s&n %s  &y|&n\r\n",
	    scha, mcha, tcha ? "[&g*&n]" : "   ");

  SEND_TO_Q(buf, d);
  sprintf(buf, "&y| &rQ&n) &gCancel and &n[&rQ&n]&guit&y        &n[&g*&n] &yTrainable Stats (In Game) |&n\r\n");
  SEND_TO_Q(buf, d);
  SEND_TO_Q("&y| &cH&n) &n[&cH&n]&gelp                                                 &y|&n\r\n", d);
  SEND_TO_Q("&y| &cP&n) &gCreate Char and &n[&cP&n]&glay game&y                            |&n\r\n", d);
  if (GET_CLASS(d->character) != -1) {
    SEND_TO_Q("&y-------------------------------------------------------------&n\r\n", d);
    write_remorts(d, buf2);
    sprintf(buf, "&y| Remort Classes&n: &W%-42s&y|&n\r\n", buf2);
    SEND_TO_Q(buf, d);
  }
  SEND_TO_Q("&y-------------------------------------------------------------&n\r\n", d);
  SEND_TO_Q("&gEnter Choice&n: ", d);
}


// DM - TODO - double check nanny function

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  char buf[128];
  int player_i, load_result, trigger_colour = 0;
  char tmp_name[MAX_INPUT_LENGTH];
  struct char_file_u tmp_store;
  room_vnum load_room;
  int tmp = CLASS_UNDEFINED;
  char strarg[10];
  void perform_change(struct char_data *ch);


  /* OasisOLC states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char*);
  } olc_functions[] = {
    { CON_OEDIT, oedit_parse },
    { CON_ZEDIT, zedit_parse },
    { CON_SEDIT, sedit_parse },
    { CON_MEDIT, medit_parse },
    { CON_REDIT, redit_parse },
    { CON_TRIGEDIT, trigedit_parse },
    { -1, NULL }
  };

  /* Report states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char*);
  } report_functions[] = {
    { CON_REPORT_ADD, report_parse },
    { CON_REPORT_EDIT, report_parse }, 
    { -1, NULL }
  };

  skip_spaces(&arg);

  /*
   * Quick check for the OLC states.
   */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state)
    {
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /*
   * Quick check for the BUG states.
   */
  for (player_i = 0; report_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == report_functions[player_i].state)
    {
      (*report_functions[player_i].func)(d, arg);
      return;
    }
   
  /* Not in OLC. */ 
  switch (STATE(d))
  {
    case CON_QCOLOUR:
      /*
      if (d->character == NULL) {
	CREATE(d->character, struct char_data, 1);
	clear_char(d->character);
	CREATE(d->character->player_specials, struct player_special_data, 1);
	d->character->desc = d;
      }*/
      d->idle_tics = 0;
      switch(LOWER(*arg))
      {
	case 'y':
	  SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_2); break;
	default: 
	  break;
      }
      //SEND_TO_Q(GREETINGS, d);
      //STATE(d) = CON_GET_NAME;
      show_char_menu(d);
      STATE(d) = CON_QCHAR; 
      break;
    case CON_GET_NAME:		/* wait for input of name */
      d->idle_tics = 0;
      if (!*arg)
      {
	STATE(d) = CON_CLOSE;
      } else {
	if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	    strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name, TRUE) ||
	    fill_word(strcpy(buf, tmp_name)) || reserved_word(buf))
	{
	  SEND_TO_Q("Invalid name, please try another.\r\n"
		    "Name: ", d);
	  return;
	}
	if ((player_i = load_char(tmp_name, &tmp_store)) > -1)
	{
	  if (PRF_FLAGGED(d->character, PRF_COLOR_2))
	    trigger_colour = 1;
	  store_to_char(&tmp_store, d->character);
	  GET_PFILEPOS(d->character) = player_i;
	  if (PLR_FLAGGED(d->character, PLR_DELETED))
	  {
	    /* We get a false positive from the original deleted character. */
	    // Umm so why use colour on a deleted char ...?
	    // Taking this out as we will prompt shortly ... DM
	    //if( PRF_FLAGGED(d->character, PRF_COLOR_2) )
		  //trigger_colour = 1;
	    free_char(d->character);
	    /* Check for multiple creations... */
	    if (!Valid_Name(tmp_name, TRUE))
	    {
	      SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
	      return;
	    }
	    CREATE(d->character, struct char_data, 1);
	    clear_char(d->character);
	    CREATE(d->character->player_specials, struct player_special_data,1);
	    d->character->desc = d;
	    CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	    strcpy(d->character->player.name, CAP(tmp_name));
	    GET_PFILEPOS(d->character) = player_i;
	    if (trigger_colour)
	      SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_2); 
	    sprintf(buf, "&gDid I get that right, &7%s&n (&cY&n/&cN&n)? ",
		    tmp_name);
	    SEND_TO_Q(buf, d);
	    STATE(d) = CON_NAME_CNFRM;
	  } else {
	    /* undo it just in case they are set */
	    REMOVE_BIT(PLR_FLAGS(d->character),
		       PLR_WRITING | PLR_MAILING | PLR_CRYO | PLR_REPORTING);
	    REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
	    SEND_TO_Q("&gPassword&n: ", d);
	    echo_off(d);
	    STATE(d) = CON_PASSWORD;
	  }
	} else {
	  /* player unknown -- make new character */
	  /* Check for multiple creations of a character. */
	  if (!Valid_Name(tmp_name, TRUE))
	  {
	    SEND_TO_Q("&rInvalid name, please try another.&n\r\nName: ", d);
	    return;
	  }
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  sprintf(buf, "&gDid I get that right, %s (Y/N)?&n ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	}
      }
      break;
    case CON_NAME_CNFRM:		/* wait for conf. of new name    */
      d->idle_tics = 0;
      if (UPPER(*arg) == 'Y')
      {
	if (isbanned(d->host) >= BAN_NEW)
	{
	  sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
		  GET_PC_NAME(d->character), d->host);
	  mudlog(buf, NRM, LVL_GOD, TRUE);
	  SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n",
	            d);
	  STATE(d) = CON_CLOSE;
	  return;
	}
	if (circle_restrict)
	{
	  SEND_TO_Q("&RSorry, new players can't be created at the moment.\r\n&n", d);
	  sprintf(buf, "Request for new char %s denied from [%s] (wizlock)",
		  GET_PC_NAME(d->character), d->host);
	  mudlog(buf, NRM, LVL_GOD, TRUE);
	  STATE(d) = CON_CLOSE;
	  return;
	}
	SEND_TO_Q("   &WNew character! &n\r\n", d);
	sprintf(buf, "&gWhat password shall identify you, &7%s&n: ",
	        GET_PC_NAME(d->character));
	GET_LEVEL(d->character) = 0;
	SEND_TO_Q(buf, d);
	echo_off(d);
	STATE(d) = CON_NEWPASSWD;
      } else if (*arg == 'n' || *arg == 'N') {
	SEND_TO_Q("&yOkay, what IS it, then?&n ", d);
	free(d->character->player.name);
	d->character->player.name = NULL;
	STATE(d) = CON_GET_NAME;
      } else {
	SEND_TO_Q("Please type Yes or No: ", d);
      }
      break;
    case CON_PASSWORD:		/* get pwd for known player      */
      /*
       * To really prevent duping correctly, the player's record should
       * be reloaded from disk at this point (after the password has been
       * typed).  However I'm afraid that trying to load a character over
       * an already loaded character is going to cause some problem down the
       * road that I can't see at the moment.  So to compensate, I'm going to
       * (1) add a 15 or 20-second time limit for entering a password, and (2)
       * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
       */
      d->idle_tics = 0;
      echo_on(d);    /* turn echo back on */

      // New echo_on() eats the return on telnet. Extra space better than none.
      SEND_TO_Q("\r\n", d);
      if (!*arg)
      {
	STATE(d) = CON_CLOSE;
      } else {
	if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), 
	            GET_PASSWD(d->character), MAX_PWD_LENGTH))
	{
	  sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	  mudlog(buf, BRF, LVL_GOD, TRUE);
	  (++(d->bad_pws));
	  /* DM: this is now checked in save_char (for sucessful and unsucessful
	   * logins):
	   * GET_BAD_PWS(d->character)++;
	   */
	  save_char(d->character, NOWHERE);
	  if ((d->bad_pws) >= max_bad_pws)
	  {	/* 3 strikes and you're out. */
	    SEND_TO_Q("&RWrong password... &ndisconnecting.\r\n", d);
	    STATE(d) = CON_CLOSE;
	  } else {
	    SEND_TO_Q("&rWrong password.&n\r\n&gPassword&n: ", d);
	    echo_off(d);
	  }
	  return;
	}
	/* Password was correct. */
	load_result = GET_BAD_PWS(d->character);
	GET_BAD_PWS(d->character) = 0;
	d->bad_pws = 0;
	if (isbanned(d->host) == BAN_SELECT &&
	    !PLR_FLAGGED(d->character, PLR_SITEOK))
	{
	  SEND_TO_Q("&RSorry, this char has not been cleared for login from your site!&n\r\n", d);
	  STATE(d) = CON_CLOSE;
	  sprintf(buf, "Connection attempt for %s denied from %s",
		  GET_NAME(d->character), d->host);
	  mudlog(buf, NRM, LVL_GOD, TRUE);
	  return;
	}
	if (GET_LEVEL(d->character) < circle_restrict)
	{
	  SEND_TO_Q("The game is currently restricted.. try again later.\r\n",
	            d);
	  STATE(d) = CON_CLOSE;
	  sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
		  GET_NAME(d->character), d->host);
	  mudlog(buf, NRM, LVL_GOD, TRUE);
	  return;
	}
	/* check and make sure no other copies of this player are logged in */
	if (perform_dupe_check(d))
	  return;
	addToUserList(d);
	if (GET_LEVEL(d->character) >= LVL_ANGEL)
	  SEND_TO_Q(imotd, d);
	else
	  SEND_TO_Q(motd, d);
	sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
	mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
	if (load_result)
	{
	  sprintf(buf, "\r\n\r\n\007\007\007"
		  "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
		  CCRED(d->character, C_SPR), load_result,
		  (load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
	  SEND_TO_Q(buf, d);
	  GET_BAD_PWS(d->character) = 0;
	}
	SEND_TO_Q("\r\n &gWelcome to PrimalMUD. &R*&n Enter when ready : ", d);
	STATE(d) = CON_RMOTD;
      }
      break;
    case CON_NEWPASSWD:
    case CON_CHPWD_GETNEW:
      d->idle_tics = 0;
      if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	  !str_cmp(arg, GET_PC_NAME(d->character)))
      {
	SEND_TO_Q("\r\n&rIllegal password.&n\r\n", d);
	SEND_TO_Q("&gPassword&n: ", d);
	return;
      }
      strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), 
	      MAX_PWD_LENGTH);
      *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';
      SEND_TO_Q("\r\n&gPlease retype password&n: ", d);
      if (STATE(d) == CON_NEWPASSWD)
	STATE(d) = CON_CNFPASSWD;
      else
	STATE(d) = CON_CHPWD_VRFY;
      break;
    case CON_CNFPASSWD:
    case CON_CHPWD_VRFY:
      d->idle_tics = 0;
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), 
	          GET_PASSWD(d->character), MAX_PWD_LENGTH))
      {
	SEND_TO_Q("\r\n&rPasswords don't match... start over.&n\r\n", d);
	SEND_TO_Q("&gPassword&n: ", d);
	if (STATE(d) == CON_CNFPASSWD)
	  STATE(d) = CON_NEWPASSWD;
	else
	  STATE(d) = CON_CHPWD_GETNEW;
	return;
      }
      echo_on(d);
      if (STATE(d) == CON_CNFPASSWD)
      {
	if (GET_PFILEPOS(d->character) < 0)
	  GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
	// Now GET_NAME() will work properly. 
	init_char(d->character);
	// Set char deleted so we don't have half created chars on 
	// disconnection during character creation process
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
	save_char(d->character, NOWHERE);
	NUM_PLAYERS++;
	GET_CLASS(d->character) = -1;
	GET_RACE(d->character) = -1;
	GET_SEX(d->character) = -1;
	// God gets no rerolls
	if (GET_IDNUM(d->character) == 1)
	{
	  GET_CLASS(d->character) = CLASS_MASTER; 
	  GET_RACE(d->character) = RACE_DEVA;
	  GET_SEX(d->character) = SEX_MALE;
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_DELETED);
	  d->character->real_abils.intel = 25;
	  d->character->real_abils.wis = 25;
	  d->character->real_abils.dex = 25;
	  d->character->real_abils.str = 25;
	  d->character->real_abils.str_add = 100;
	  d->character->real_abils.con = 25;
	  d->character->real_abils.cha = 25;
	  save_char(d->character, NOWHERE);
	  SEND_TO_Q("\r\n&RWelcome master. * Press &cENTER&R to continue :&n ",
	            d);
	  STATE(d) = CON_RMOTD;
	  break;
	} 
	// Prompt for colour now ...
	//show_char_menu(d);
	//STATE(d) = CON_QCHAR; 
	SEND_TO_Q("\r\nWould you like colour? ", d);
	STATE(d) = CON_QCOLOUR; 
      } else {
	save_char(d->character, NOWHERE);
	echo_on(d);
	SEND_TO_Q("\r\nDone.\r\n", d);
	SEND_TO_Q(MENU, d);
	STATE(d) = CON_MENU;
      }
      break;
    case CON_QCHAR:
      d->idle_tics = 0;
      switch (*arg)
      {
	case 's':
	case 'S':
	  SEND_TO_Q("\r\n&gWhat is your characters sex&n (&cM&n/&cF&n)? ", d);
	  STATE(d) = CON_QSEX;
	  break;
	case 'r':
	case 'R':
	  SEND_TO_Q(race_menu, d);	
	  STATE(d) = CON_QRACE;
	  break;
	case 'c':
	case 'C':
	  SEND_TO_Q(class_menu, d);	
	  STATE(d) = CON_QCLASS;
	  break;
	case 'l':
	case 'L':
	  if (GET_SEX(d->character) == -1)
	  {
	    SEND_TO_Q("&rNo Sex Selected!&n\r\n", d);
	    SEND_TO_Q("\r\n&gWhat is your characters sex&n (&cM&n/&cF&n)? ", d);
	    STATE(d) = CON_QSEX;
	    return;
	  }
	  if (GET_RACE(d->character) == -1)
	  {
	    SEND_TO_Q("&rNo Race Selected!&n\r\n", d);
	    SEND_TO_Q(race_menu, d);
	    STATE(d) = CON_QRACE;
	    return;
	  }
	  if (GET_CLASS(d->character) == -1)
	  {
	    SEND_TO_Q("&rNo Class Selected!&n\r\n", d);
	    SEND_TO_Q(class_menu, d);
	    STATE(d) = CON_QCLASS;
	    return;
	  }
	  roll_real_abils(d->character);
	  show_char_menu(d);
	  STATE(d) = CON_QCHAR;
	  return;
	  /*
	case '1':
	  if (d->character->player_specials->saved.stat_pool > 0)
	  {
	    if (GET_REAL_STR(d->character) ==
	        pc_max_race_stats[GET_RACE(d->character)][STAT_STR])
	    {
	      SEND_TO_Q("&rMaximum value already reached.&n\r\n", d);
	    } else {
	      GET_REAL_STR(d->character)++;
	      d->character->player_specials->saved.stat_pool--;
	      SEND_TO_Q("&bAdded 1 stat point to STR&n\r\n", d);
	    }
	  } else {
	    SEND_TO_Q("&rYou don't have any more points available.&n\r\n", d);
	  }
	  show_char_menu(d);
	  return;
	case '2':
	  if (d->character->player_specials->saved.stat_pool > 0)
	  {
	    if (GET_REAL_INT(d->character) ==
	        pc_max_race_stats[GET_RACE(d->character)][STAT_INT])
	    {
	      SEND_TO_Q("&rMaximum value already reached.&n\r\n", d);
	    } else {
	      GET_REAL_INT(d->character)++;
	      d->character->player_specials->saved.stat_pool--;
	      SEND_TO_Q("&bAdded 1 stat point to INT&n\r\n", d);
	    }
	  } else {
	    SEND_TO_Q("&rYou don't have any more points available.&n\r\n", d);
	  }
	  show_char_menu(d);
	  return;
	case '3':
	  if (d->character->player_specials->saved.stat_pool > 0)
	  {
	    if (GET_REAL_WIS(d->character) ==
	        pc_max_race_stats[GET_RACE(d->character)][STAT_WIS])
	    {
	      SEND_TO_Q("&rMaximum value already reached.&n\r\n", d);
	    } else {
	      GET_REAL_WIS(d->character)++;
	      d->character->player_specials->saved.stat_pool--;
	      SEND_TO_Q("&bAdded 1 stat point to WIS&n\r\n", d);
	    }
	  } else {
	    SEND_TO_Q("&rYou don't have any more points available.&n\r\n", d);
	  }
	  show_char_menu(d);
	  return;
	case '4':
	  if (d->character->player_specials->saved.stat_pool > 0)
	  {
	    if (GET_REAL_CON(d->character) ==
	        pc_max_race_stats[GET_RACE(d->character)][STAT_CON])
	    {
	      SEND_TO_Q("&rMaximum value already reached.&n\r\n", d);
	    } else {
	      GET_REAL_CON(d->character)++;
	      d->character->player_specials->saved.stat_pool--;
	      SEND_TO_Q("&bAdded 1 stat point to CON&n\r\n", d);
	    }
	  } else {
	    SEND_TO_Q("&rYou don't have any more points available.&n\r\n", d);
	  }
	  show_char_menu(d);
	  return;
	case '5':
	  if (d->character->player_specials->saved.stat_pool > 0)
	  {
	    if (GET_REAL_DEX(d->character) ==
		pc_max_race_stats[GET_RACE(d->character)][STAT_DEX])
	    {
	      SEND_TO_Q("&rMaximum value already reached.&n\r\n", d);
	    } else {
	      GET_REAL_DEX(d->character)++;
	      d->character->player_specials->saved.stat_pool--;
	      SEND_TO_Q("&bAdded 1 stat point to DEX&n\r\n", d);
	    }
	  } else {
	    SEND_TO_Q("&rYou don't have any points available.&n\r\n", d);
	  }
	  show_char_menu(d);
	  return;
	case '6':
	  if (d->character->player_specials->saved.stat_pool > 0)
	  {
	    if (GET_REAL_CHA(d->character) ==
		pc_max_race_stats[GET_RACE(d->character)][STAT_CHA])
	    {
	      SEND_TO_Q("&rMaximum value already reached.&n\r\n", d);
	    } else {
	      GET_REAL_CHA(d->character)++;
	      d->character->player_specials->saved.stat_pool--;
	      SEND_TO_Q("&bAdded 1 stat point to CHA&n\r\n", d);
	    }
	  } else {
	    SEND_TO_Q("&rYou don't have any more points available.&n\r\n", d);
	  }
	  show_char_menu(d);
	  return;
	*/
	case '8':
	  SEND_TO_Q(race_help, d);
	  STATE(d) = CON_QRACE_HELP;
	  return;
	case '9':
	  SEND_TO_Q(class_help, d);
	  STATE(d) = CON_QCLASS_HELP;
	  return;
	case '0':
	  show_train(d, TRUE);
	  STATE(d) = CON_QPAUSE;
	  SEND_TO_Q("\r\nPress &cEnter&n to continue.", d);
	  return;
	case 'h':
	case 'H':
	  SEND_TO_Q(char_help, d);
	  STATE(d) = CON_QPAUSE;
	  SEND_TO_Q("\r\nPress &cEnter&n to continue.", d);
	  return;
	case 'q':
	case 'Q':
	  close_socket(d);
	  return;
	case 'p':
	case 'P':
	  if (GET_SEX(d->character) == -1)
	  {
	    SEND_TO_Q("&rNo Sex Selected!&n\r\n", d);
	    SEND_TO_Q("\r\n&gWhat is your characters sex&n (&cM&n/&cF&n)? ", d);
	    STATE(d) = CON_QSEX;
	    return;
	  }
	  if (GET_RACE(d->character) == -1)
	  {
	    SEND_TO_Q("&rNo Race Selected!&n\r\n", d);
	    SEND_TO_Q(race_menu, d);
	    STATE(d) = CON_QRACE;
	    return;
	  }
	  if (GET_CLASS(d->character) == -1)
	  {
	    SEND_TO_Q("&rNo Class Selected!&n\r\n", d);
	    SEND_TO_Q(class_menu, d);
	    STATE(d) = CON_QCLASS;
	    return;
	  }
	  // Clear the deleted bit and we're cooking with gas ...
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_DELETED);
	  save_char(d->character, NOWHERE);
	  SEND_TO_Q(motd, d);
	  SEND_TO_Q("\r\n Character created. * Press &cENTER&n to continue: ",
	            d);
	  STATE(d) = CON_RMOTD;
	  addToUserList(d);
	  sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
	  mudlog(buf, NRM, LVL_GOD, TRUE);
	  break;
	default:
	  SEND_TO_Q("&rInvalid option. Try Again&n: ", d);
	  return;
      }
      break;
    case CON_QCLASS_HELP:
      d->idle_tics = 0;
      sprintf(strarg, "%c", *arg);
      if (*arg != '\0')
      {
	// first check letters using parse_class
	tmp = parse_class(arg);
	// now check numbers
	if (tmp == CLASS_UNDEFINED)
	{
	  if (atoi(strarg) >= 0)
	    tmp = atoi(strarg);
	  /* Refer to class help array - pos 0 is general help, pos 
	   * [1, NUM_CLASSES + 1] is for the individual class help, since we 
	   * have a CLASS_XXX value, we need to use the next position in the
	   * help array...
	   */
	} else {
	  tmp++;
	}
	// Still didn't find a class - must be an invalid option ...
	if (tmp == CLASS_UNDEFINED)
	{
	  SEND_TO_Q("&rInvalid option. Try Again&n: ", d);
	  return;
	// Send the class help and go into class pause state 
	} else {
	  tmp--;
	  switch (tmp)
	  {
	    case CLASS_MAGIC_USER:
	      do_help(d->character, " Mage", FALSE, FALSE);
	      break;
	    case CLASS_THIEF:
	      do_help(d->character, " Thief", FALSE, FALSE);
	      break;
	    case CLASS_WARRIOR:
	      do_help(d->character, " Warrior", FALSE, FALSE);
	      break;
	    case CLASS_CLERIC:
	      do_help(d->character, " Cleric", FALSE, FALSE);
	      break;
	    case CLASS_DRUID:
	      do_help(d->character, " Druid", FALSE, FALSE);
	      break;
	    case CLASS_PRIEST:
	      do_help(d->character, " Priest", FALSE, FALSE);
	      break;
	    //case CLASS_PALADIN:
	    case -1:
	      do_help(d->character, " Paladin", FALSE, FALSE);
	      break;
	    case CLASS_SPELLSWORD:
	      do_help(d->character, " Spellsword", FALSE, FALSE);
	      break;
	    case CLASS_NIGHTBLADE:
	      do_help(d->character, " Nightblade", FALSE, FALSE);
	      break;
	    case CLASS_BATTLEMAGE:
	      do_help(d->character, " Battlemage", FALSE, FALSE);
	      break;
	    case CLASS_MASTER:
	      do_help(d->character, " Master", FALSE, FALSE);
	      break;
	    default:
	      SEND_TO_Q(class_help, d);
	      return;
	  }
	  SEND_TO_Q("\r\nPress &cEnter&n to continue.", d);
	  STATE(d) = CON_QCLASS_PAUSE;
	  return;
	}
      }
      // We got an empty string (Return) go back to main creation menu
      show_char_menu(d);
      STATE(d) = CON_QCHAR;
      return;
    case CON_QCLASS_PAUSE:
      d->idle_tics = 0;
      SEND_TO_Q(class_help, d);
      STATE(d) = CON_QCLASS_HELP;
      return;
    case CON_QRACE_HELP:
      d->idle_tics = 0;
      sprintf(strarg, "%c", *arg);
      if (*arg != '\0')
      {
	// first check letters using parse_class
	tmp = parse_race_name(arg);
	// now check numbers
	if (tmp == RACE_UNDEFINED)
	{
	  if (atoi(strarg) > 0 && atoi(strarg) < MAX_RACES + 1)
	    tmp = atoi(strarg);
	  /* Refer to race help array - pos 0 is general help, pos
	   * [1, MAX_RACES + 1] is for the individual race help, since we have
	   * a RACE_XXX value, we need to use the next position in the help
	   * array...
	   */
	} else {
	  tmp++;
	}
	// Still didn't find a class - must be an invalid option ...
	if (tmp == RACE_UNDEFINED)
	{
	  SEND_TO_Q("&rInvalid option. Try Again&n: ", d);
	  return;
	// Send the class help and go into class pause state 
	} else {
	  switch (tmp-1)
	  {
	    case RACE_DEVA:
	      do_help(d->character, " Deva", FALSE, FALSE);
	      break;
	    case RACE_MINOTAUR:
	      do_help(d->character, " Minotaur", FALSE, FALSE);
	      break;
	    case RACE_OGRE:
	      do_help(d->character, " Ogre", FALSE, FALSE);
	      break;
	    case RACE_ELF:
	      do_help(d->character, " Elf", FALSE, FALSE);
	      break;
	    case RACE_HUMAN:
	      do_help(d->character, " Human", FALSE, FALSE);
	      break;
	    case RACE_DWARF:
	      do_help(d->character, " Dwarf", FALSE, FALSE);
	      break;
	    case RACE_PIXIE:
	      do_help(d->character, " Pixie", FALSE, FALSE);
	      break;
	    default:
	      SEND_TO_Q(race_help, d);
	      return;
	  }
	  SEND_TO_Q("\r\nPress &cEnter&n to continue.", d);
	  STATE(d) = CON_QRACE_PAUSE;
	  return;
	}
      }
      // We got an empty string (Return) go back to main creation menu
      show_char_menu(d);
      STATE(d) = CON_QCHAR;
      return;
    case CON_QRACE_PAUSE:
      d->idle_tics = 0;
      SEND_TO_Q(race_help, d);
      STATE(d) = CON_QRACE_HELP;
      return;
    /* Just for Press Return to continue stuff (before continuing to creation 
     *                                          menu)  */
    case CON_QPAUSE:
      d->idle_tics = 0;
      show_char_menu(d);
      STATE(d) = CON_QCHAR;
      break;
    case CON_QSEX:		/* query sex of new user         */
      d->idle_tics = 0;
      switch (*arg)
      {
	case 'm':
	case 'M':
	  d->character->player.sex = SEX_MALE;
	  break;
	case 'f':
	case 'F':
	  d->character->player.sex = SEX_FEMALE;
	  break;
	default:
	  SEND_TO_Q("&rThat is not a sex..&n\r\n"
		    "&gWhat IS your characters sex?&n ", d);
	  return;
      }
      show_char_menu(d);
      STATE(d) = CON_QCHAR;
      break;
    case CON_QRACE:
      d->idle_tics = 0;
      load_result = parse_race_name(arg);
      if (load_result == RACE_UNDEFINED)
      {
	SEND_TO_Q("\r\n&rThat's not a race.&n\r\n&gRace&n: ", d);
	return;
      }
      GET_RACE(d->character) = load_result;
      if (GET_CLASS(d->character) != -1)
	roll_real_abils(d->character);
	/* Artus> Reset specials so if they choose a different race they don't
	 * keep them. */
      GET_SPECIALS(d->character) = 0;
      set_race_specials(d->character);
      show_char_menu(d);
      STATE(d) = CON_QCHAR;
      break;
    case CON_QCLASS:
      d->idle_tics = 0;
      load_result = parse_class(arg);
      if (load_result == CLASS_UNDEFINED && *arg != '?') 
      {
	SEND_TO_Q("\r\n&rThat's not a class.&n\r\n&gClass&n: ", d);
	return;
      }
      if (load_result > CLASS_WARRIOR)
      {
	SEND_TO_Q("\r\n&rThat's not a valid class.&n\r\n&gClass&n: ", d);
	return;
      }
      GET_CLASS(d->character) = load_result;
      if (GET_RACE(d->character) != -1)
        roll_real_abils(d->character);
      show_char_menu(d);
      STATE(d) = CON_QCHAR;
      break;
    case CON_RMOTD:		/* read CR after printing motd   */
      d->idle_tics = 0;
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
      break;
    case CON_MENU:		/* get selection from main menu  */
      d->idle_tics = 0;
      switch (*arg)
      {
	case '0':
	  SEND_TO_Q("Goodbye.\r\n", d);
	  STATE(d) = CON_CLOSE;
	  break;
	case '1':
	  reset_char(d->character);
	  read_aliases(d->character);
	  if (PLR_FLAGGED(d->character, PLR_INVSTART))
	  {
	    GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
	    GET_INVIS_TYPE(d->character) = INVIS_NORMAL;  
	  }
	/**
	 * DM: allow all characters to have either default or specific entry
	 * rooms for each world. Ie. the start room vnums for each world are
	 * stored in an array accessible by the macro ENTRY_ROOM(ch, <world>)
	 * When the player saves or quits (do_save, do_quit), the current world
	 * is saved. If they are in a the default world start room, the array
	 * value is set to NOWHERE, otherwise if it is in a house, the vnum is
	 * stored.  The entry rooms can/could also be set by the use of a skill,
	 * which would allow players to recall or enter to that room. 
	 */
	//if (START_WORLD(d->character) < 0 || ... is a ubyte 
	  if (START_WORLD(d->character) > NUM_WORLDS)
	    START_WORLD(d->character) = WORLD_MEDIEVAL;
	  if (ENTRY_ROOM(d->character, START_WORLD(d->character)) == NOWHERE)
	    load_room = real_room(world_start_room[START_WORLD(d->character)]);
	  else
	    load_room = real_room(ENTRY_ROOM(d->character, 
				  START_WORLD(d->character)));
	/*
	 * We have to place the character in a room before equipping them
	 * or equip_char() will gripe about the person in NOWHERE.
	 */
	  if (PLR_FLAGGED(d->character, PLR_FROZEN))
	    load_room = r_frozen_start_room;
	  send_to_char(WELC_MESSG, d->character);
	  d->character->next = character_list;
	  character_list = d->character;
	  char_to_room(d->character, load_room);
	  load_result = Crash_load(d->character);
	  // with the copyover patch, this next line goes in enter_player_game()
	  GET_ID(d->character) = GET_IDNUM(d->character);
	  save_char(d->character, NOWHERE);
	  // Remove meditate
	  if (char_affected_by_timer(d->character, TIMER_MEDITATE))
	    timer_from_char(d->character, TIMER_MEDITATE);
	  sprintf(buf,"&7%s&g has entered the game.", GET_NAME(d->character));
	  info_channel(buf, d->character);  
	  act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
	  // with the copyover patch, this next line goes in enter_player_game()
	  read_saved_vars(d->character);
	  greet_mtrigger(d->character, -1);
	  greet_memory_mtrigger(d->character);
	  STATE(d) = CON_PLAYING;
	  if (GET_LEVEL(d->character) == 0) 
	  {
	    do_start(d->character);
	    send_to_char(START_MESSG, d->character);
	  }
	  look_at_room(d->character, 0);
	  if (has_mail(GET_IDNUM(d->character)))
	    send_to_char("\007You have mail waiting.\r\n", d->character);
	  // Artus. Change them, if it's a full moon, and the time is night.
	  if (PRF_FLAGGED(d->character, PRF_WOLF | PRF_VAMPIRE) &&
	      (weather_info.moon == MOON_FULL) && 
	      (weather_info.sunlight == SUN_DARK) &&
	      !affected_by_spell(d->character, SPELL_CHANGED))
	    perform_change(d->character);
	  /* DM_exp check if using old exp system on login 
	   * (have more exp than required at this level) */
#if 0 // Artus> This is probably redundant, now.
	  if (GET_EXP(d->character) > level_exp((d->character), 
						GET_LEVEL(d->character)))
	    GET_EXP(d->character) = 0;
#endif
	  if (GET_EXP(d->character) < 0)
	    GET_EXP(d->character) = 0;
	  // Apply specials modifiers now
	  calc_modifier(d->character);
	  if (load_result == 2)	/* rented items lost */
	  {
	    send_to_char("\r\n\007You could not afford your rent!\r\n"
	      "Your possesions have been donated to the Salvation Army!\r\n",
			 d->character);
	  }
	  d->has_prompt = 0;
	  break;
	case '2':
	  if (d->character->player.description)
	  {
	    SEND_TO_Q("\r\nCurrent description:\r\n", d);
	    SEND_TO_Q(d->character->player.description, d);
	    SEND_TO_Q("\r\n", d);
	    d->backstr = str_dup(d->character->player.description);
	  }
	  SEND_TO_Q("Enter the new text you'd like others to see when they look at you.\r\n", d);
	  SEND_TO_Q("/s or @ on a newline saves, /h for help.\r\n", d);
	  d->str = &d->character->player.description;
	  d->max_str = EXDSCR_LENGTH;
	  STATE(d) = CON_EXDESC;
	  break;
	case '3':
	  page_string(d, background, 0);
	  STATE(d) = CON_RMOTD;
	  break;
	case '4':
	  SEND_TO_Q("\r\nEnter your old password: ", d);
	  echo_off(d);
	  STATE(d) = CON_CHPWD_GETOLD;
	  break;
	case '5':
	  SEND_TO_Q("\r\nEnter your password for verification: ", d);
	  echo_off(d);
	  STATE(d) = CON_DELCNF1;
	  break;
	default:
	  SEND_TO_Q("\r\nThat's not a menu choice!\r\n", d);
	  SEND_TO_Q(MENU, d);
	  break;
      }
      break;
    case CON_CHPWD_GETOLD:
      d->idle_tics = 0;
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), 
	          GET_PASSWD(d->character), MAX_PWD_LENGTH))
      {
	echo_on(d);
	SEND_TO_Q("\r\nIncorrect password.\r\n", d);
	SEND_TO_Q(MENU, d);
	STATE(d) = CON_MENU;
      } else {
	SEND_TO_Q("\r\nEnter a new password: ", d);
	STATE(d) = CON_CHPWD_GETNEW;
      }
      return;
    case CON_DELCNF1:
      d->idle_tics = 0;
      echo_on(d);
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), 
	          GET_PASSWD(d->character), MAX_PWD_LENGTH))
      {
	SEND_TO_Q("\r\nIncorrect password.\r\n", d);
	SEND_TO_Q(MENU, d);
	STATE(d) = CON_MENU;
      } else {
	SEND_TO_Q("\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		  "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		  "Please type \"yes\" to confirm: ", d);
	STATE(d) = CON_DELCNF2;
      }
      break;
    case CON_DELCNF2:
      d->idle_tics = 0;
      if (!strcmp(arg, "yes") || !strcmp(arg, "YES"))
      {
	if (PLR_FLAGGED(d->character, PLR_FROZEN))
	{
	  SEND_TO_Q("You try to kill yourself, but the ice stops you.\r\n", d);
	  SEND_TO_Q("Character not deleted.\r\n\r\n", d);
	  STATE(d) = CON_CLOSE;
	  return;
	}
	if (GET_LEVEL(d->character) < LVL_GRGOD)
	  SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
	save_char(d->character, NOWHERE);
	Crash_delete_file(GET_NAME(d->character));
	sprintf(buf, "Character '%s' deleted!\r\n"
		"Goodbye.\r\n", GET_NAME(d->character));
	SEND_TO_Q(buf, d);
	sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character),
		GET_LEVEL(d->character));
	mudlog(buf, NRM, LVL_GOD, TRUE);
	NUM_PLAYERS--;
	STATE(d) = CON_CLOSE;
	return;
      }
      SEND_TO_Q("\r\nCharacter not deleted.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
      break;
    /*
     * It's possible, if enough pulses are missed, to kick someone off
     * while they are at the password prompt. We'll just defer to let
     * the game_loop() axe them.
     */
    case CON_CLOSE:
      break;
    default:
      basic_mud_log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",  STATE(d), 
	            d->character ? GET_NAME(d->character) : "<unknown>");
      STATE(d) = CON_DISCONNECT;	/* Safest to do. */
      break;
  }
}

ACMD(do_stats)
{
  if (!IS_NPC(ch))
    show_train(ch->desc, FALSE);
  return;
}
