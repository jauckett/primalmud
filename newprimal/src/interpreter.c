/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

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

extern int world_start_room[NUM_WORLDS];
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern const char *class_menu;
extern const char *race_menu;
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern long NUM_PLAYERS;
extern const float class_modifiers[NUM_CLASSES];
extern const float race_modifiers[];

#define BASE_SECT(n)  ((n) & 0x000f)

/* external functions */
extern char *GREETINGS;
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch);
int parse_class(char *arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void read_aliases(struct char_data *ch);

/* local functions */
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig);
int reserved_word(char *argument);
int find_name(char *name);
int _parse_name(char *arg, char *name);

/* prototypes for all do_x functions. */
ACMD(do_armourcraft);
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
ACMD(do_battlecry);
ACMD(do_headbutt);
ACMD(do_piledrive);
ACMD(do_trip);
ACMD(do_bearhug);
ACMD(do_bodyslam);

ACMD(do_action);
ACMD(do_advance);
ACMD(do_affects);  
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_auction);
ACMD(do_autoassist);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_blackjack); 
ACMD(do_cast);
ACMD(do_change);   
ACMD(do_clan_table);
ACMD(do_clans);
ACMD(do_classes);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_compare);
ACMD(do_consider);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
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
ACMD(do_hit);
ACMD(do_house);
ACMD(do_ignore);
ACMD(do_immort);
ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_join); 
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_laston); 
ACMD(do_leave);
ACMD(do_levels);
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
ACMD(do_order);
ACMD(do_page);
ACMD(do_pagelength);
ACMD(do_pagewidth);
ACMD(do_pinch);
ACMD(do_pkset);   
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
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
ACMD(do_scream);
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
ACMD(do_spellshow);
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
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);

/* clan do_x commands -Hal */
ACMD(do_knight);
ACMD(do_banish);
ACMD(do_demote);
ACMD(do_recruit);
ACMD(do_promote);
ACMD(do_signup);


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

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN },

  /* now, the main list */
  { "at"       , POS_DEAD    , do_at       , LVL_ANGEL, 0 },
  { "adrenaline",POS_FIGHTING, do_adrenaline, 0, 0 },
  { "advance"  , POS_DEAD    , do_advance  , LVL_IMPL, 0 },
  { "affects"  , POS_DEAD    , do_affects  , 0, 0 },   
  { "afk"      , POS_DEAD    , do_gen_tog  , 0, SCMD_AFK },   
  { "agree"    , POS_DEAD    , do_action   , 0, 0 },   
  { "alias"    , POS_DEAD    , do_alias    , 0, 0 },
  { "accuse"   , POS_SITTING , do_action   , 0, 0 },
  { "ambush"   , POS_STANDING, do_ambush   , 0, 0 },
  { "angnet"   , POS_DEAD    , do_wiznet   , LVL_ANGEL, SCMD_ANGNET },
  { "-"        , POS_DEAD    , do_wiznet   , LVL_ANGEL, SCMD_ANGNET }, 
  { "angel"    , POS_RESTING , do_action   , LVL_ANGEL, 0 },  
  { "apologise", POS_RESTING , do_action   , 0, 0 },    
  { "applaud"  , POS_RESTING , do_action   , 0, 0 },
  { "areas"    , POS_DEAD    , do_gen_ps   , 0, SCMD_AREAS }, 
  { "assasinate", POS_FIGHTING, do_not_here , 1, 0 }, 
  { "assist"   , POS_FIGHTING, do_assist   , 1, 0 },
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
  { "attend"   , POS_STANDING, do_attend_wounds, 0, 0},
  { "auction"  , POS_SLEEPING, do_auction  , 0, SCMD_AUCTION },
  { "autoassist",POS_RESTING ,do_autoassist, 0, 0 },      
  { "autoexit" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT },
  { "autogold" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOGOLD },
  { "autoloot" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOLOOT },
  { "autosplit", POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOSPLIT },

  { "backstab" , POS_FIGHTING, do_backstab , 1, 0 }, 
  { "ban"      , POS_DEAD    , do_ban      , LVL_GRGOD, 0 },
  { "banish"   , POS_RESTING , do_banish   , 0, 0 },
  { "balance"  , POS_STANDING, do_not_here , 1, 0 },
  { "bark"     , POS_RESTING , do_action   , 0, 0 },   
  { "bash"     , POS_FIGHTING, do_bash     , 1, 0 },
  { "bat"      , POS_RESTING , do_action   , 0, 0 },    
  { "battlecry", POS_STANDING, do_battlecry, 0, 0 },
  { "bay"      , POS_RESTING , do_action   , 0, 0 },
  { "bearhug"  , POS_FIGHTING, do_bearhug  , 0, 0 },
  { "beat"     , POS_RESTING , do_action   , 0, 0 },   
  { "beg"      , POS_RESTING , do_action   , 0, 0 },
  { "behead"   , POS_RESTING , do_action   , 0, 0 },
  { "bet"      , POS_RESTING , do_not_here , 0, 0 },   
  { "bj"       , POS_RESTING , do_blackjack, 0, 0 },
  { "blackjack", POS_RESTING , do_blackjack, 0, 0 },
  { "blast"    , POS_FIGHTING, do_hit      , 0, SCMD_HIT}, 
  { "bleed"    , POS_RESTING , do_action   , 0, 0 },
  { "blink"    , POS_RESTING , do_action   , 0, 0 }, 
  { "blush"    , POS_RESTING , do_action   , 0, 0 },
  { "bodyslam" , POS_FIGHTING, do_bodyslam , 0, 0 },
  { "bounce"   , POS_STANDING, do_action   , 0, 0 },
  { "bow"      , POS_STANDING, do_action   , 0, 0 },
  { "brb"      , POS_RESTING , do_action   , 0, 0 },
  { "breakin"  , POS_STANDING, do_breakin  , 0, 0 },
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "burp"     , POS_RESTING , do_action   , 0, 0 },
  { "buy"      , POS_STANDING, do_not_here , 0, 0 },
  { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG },

  { "cast"     , POS_SITTING , do_cast     , 1, 0 },
  { "cackle"   , POS_RESTING , do_action   , 0, 0 },
  { "cape"     , POS_RESTING , do_action   , 0, 0 },
  { "challenge", POS_DEAD    , do_action   , 0, 0 },
  { "change"   , POS_FIGHTING, do_change   , 0, 0 },
  { "chase"    , POS_RESTING , do_action   , 0, 0 },   
  { "check"    , POS_STANDING, do_not_here , 1, 0 },
  { "cheer"    , POS_DEAD    , do_action   , 0, 0 }, 
  { "chuckle"  , POS_RESTING , do_action   , 0, 0 },
  { "clans"    , POS_DEAD    , do_clans    , 0, 0 },
  { "clantable", POS_DEAD    , do_clan_table,0, 0 },
  { "clap"     , POS_RESTING , do_action   , 0, 0 },
  { "classes"  , POS_DEAD    , do_classes  , 0, 0 },
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "colour"   , POS_DEAD    , do_color    , 0, 0 },
  { "colourset", POS_DEAD    , do_setcolour, 0, 0 },
  { "comb"     , POS_RESTING , do_action   , 0, 0 },  
  { "compare"  , POS_SITTING , do_compare  , 0, 0 },  
  { "comfort"  , POS_RESTING , do_action   , 0, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
  { "consider" , POS_RESTING , do_consider , 0, 0 },
  { "cough"    , POS_RESTING , do_action   , 0, 0 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },
  { "cringe"   , POS_RESTING , do_action   , 0, 0 },
  { "cripple"  , POS_RESTING , do_action   , 0, 0 },  
  { "cry"      , POS_RESTING , do_action   , 0, 0 },
  { "ctalk"    , POS_SLEEPING, do_gen_comm , 0, SCMD_CTALK },     
  { "cuddle"   , POS_RESTING , do_action   , 0, 0 },
  { "curse"    , POS_RESTING , do_action   , 0, 0 },
  { "curtsey"  , POS_STANDING, do_action   , 0, 0 },

  { "dance"    , POS_STANDING, do_action   , 0, 0 },
  { "date"     , POS_DEAD    , do_date     , LVL_ETRNL1, SCMD_DATE },
  { "daydream" , POS_SLEEPING, do_action   , 0, 0 },
  { "dc"       , POS_DEAD    , do_dc       , LVL_GOD, 0 },
  { "deimmort" , POS_SITTING , do_deimmort , LVL_GRGOD, 0 },  
  { "demote"   , POS_STANDING, do_demote   , 0, 0 },
  { "deposit"  , POS_STANDING, do_not_here , 1, 0 },
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0 },
  { "die"      , POS_SLEEPING, do_action   , 0, 0 }, 
  { "disguise" , POS_STANDING, do_disguise , 0, 0 },
  { "dismount" , POS_SITTING , do_dismount , 0, 0 },
  { "display"  , POS_DEAD    , do_display  , 0, 0 },
  { "doh"      , POS_DEAD    , do_action   , 0, 0 }, 
  { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE },
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP },
  { "drool"    , POS_RESTING , do_action   , 0, 0 },
  { "duck"     , POS_DEAD    , do_action   , 0, 0 }, 

  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_ETRNL7, SCMD_ECHO },
  { "elvis"    , POS_DEAD    , do_action   , 0, 0 },    
  { "emote"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE },
  { ":"        , POS_RESTING, do_echo      , 1, SCMD_EMOTE },
  { "ehelp"    , POS_SLEEPING,do_commands  , LVL_ETRNL1, SCMD_WIZHELP },   
  { "embrace"  , POS_STANDING, do_action   , 0, 0 },
  { "enter"    , POS_STANDING, do_enter    , 0, 0 },
  { "equipment", POS_SLEEPING, do_equipment, 0, 0 },
  { "escape",    POS_STANDING, do_escape   , 0, 0 },
  { "eskimo"   , POS_RESTING , do_action   , 0, 0 },   
  { "exits"    , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , POS_SITTING , do_examine  , 0, 0 },
  { "edit"     , POS_DEAD    , do_edit     ,LVL_IMPL, 0 },     /* Testing! */
  { "exp"      , POS_DEAD    , do_exp      , 0, 0 },  

  { "fatality" , POS_RESTING , do_action   , LVL_ETRNL5, 0 },
  { "force"    , POS_SLEEPING, do_force    , LVL_GOD, 0 },
  { "fart"     , POS_RESTING , do_action   , 0, 0 },
  { "faint"    , POS_RESTING , do_action   , 0, 0 },    
  { "feed"     , POS_RESTING , do_action   , 0, 0 },        
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0 },
  { "flip"     , POS_STANDING, do_action   , 0, 0 },
  { "flirt"    , POS_RESTING , do_action   , 0, 0 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0 },
  { "fondle"   , POS_RESTING , do_action   , 0, 0 },
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE },
  { "french"   , POS_RESTING , do_action   , 0, 0 },
  { "frown"    , POS_RESTING , do_action   , 0, 0 },
  { "fume"     , POS_RESTING , do_action   , 0, 0 },

  { "get"      , POS_RESTING , do_get      , 0, 0 },
  { "gasp"     , POS_RESTING , do_action   , 0, 0 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_GOD, 0 },
  { "gglow"    , POS_RESTING , do_action   , 0, 0 },
  { "give"     , POS_RESTING , do_give     , 0, 0 },
  { "giggle"   , POS_RESTING , do_action   , 0, 0 },
  { "glare"    , POS_RESTING , do_action   , 0, 0 },
  { "glaze"    , POS_RESTING , do_action   , 0, 0 },
  { "go"       , POS_STANDING, do_go       , 0, 0 },        
  { "goto"     , POS_SLEEPING, do_goto     , LVL_IMMORT, 0 },
  { "gold"     , POS_RESTING , do_gold     , 0, 0 },
  { "goose"    , POS_DEAD    , do_action   , 0, 0 }, 
  { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
  { "group"    , POS_RESTING , do_group    , 1, 0 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0 },
  { "grats"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ },
  { "greet"    , POS_RESTING , do_action   , 0, 0 },
  { "grin"     , POS_RESTING , do_action   , 0, 0 },
  { "groan"    , POS_RESTING , do_action   , 0, 0 },
  { "grope"    , POS_RESTING , do_action   , 0, 0 },
  { "grovel"   , POS_RESTING , do_action   , 0, 0 },
  { "growl"    , POS_RESTING , do_action   , 0, 0 },
  { "grumble"  , POS_DEAD    , do_action   , 0, 0 }, 
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0 },
  { "guide"    , POS_DEAD    , do_action   , 0, 0 },  
  { "happy"    , POS_DEAD    , do_action   , 0, 0 },
  { "harp"     , POS_RESTING , do_action   , 0, 0 },    
  { "headbutt" , POS_FIGHTING, do_headbutt , 0, 0 },
  { "help"     , POS_DEAD    , do_help     , 0, 0 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_ANGEL, SCMD_HANDBOOK },
  { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_GRGOD, 0 },
  { "hickey"   , POS_DEAD    , do_action   , 0, 0 },  
  { "hiccup"   , POS_RESTING , do_action   , 0, 0 },
  { "hide"     , POS_RESTING , do_hide     , 1, 0 },
  { "high5"    , POS_DEAD    , do_action   , 0, 0 }, 
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , POS_RESTING , do_grab     , 1, 0 },
  { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER },
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_ETRNL4, SCMD_HOLYLIGHT },
  { "hop"      , POS_RESTING , do_action   , 0, 0 },
  { "house"    , POS_RESTING , do_house    , 0, 0 },
  { "hug"      , POS_RESTING , do_action   , 0, 0 },
  { "hum"      , POS_DEAD    , do_action   , 0, 0 },
  { "hunt"     , POS_STANDING, do_track    , 0, SCMD_HUNT },

  { "inventory", POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },
  { "ignore"   , POS_SLEEPING, do_ignore   , 0, 0 },  
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD },
  { "immort"   , POS_SITTING , do_immort   , LVL_GRGOD, 0 }, 
//  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "immlist"  , POS_DEAD    , do_immlist   , 0, SCMD_IMMLIST },
  { "immnet"   , POS_DEAD    , do_wiznet   , LVL_ETRNL1, SCMD_IMMNET },
  { "/"        , POS_DEAD    , do_wiznet   , LVL_ETRNL1, SCMD_IMMNET },
  { "info"     , POS_DEAD    , do_info     , 10, 0},
//  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "insult"   , POS_RESTING , do_insult   , 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_ANGEL, 0 },
  { "iwhistle" , POS_DEAD    , do_action   , 0, 0 }, 

  { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK },
  { "join"     , POS_SITTING , do_join     , 0, 0 },
  { "jeer"     , POS_DEAD    , do_action   , 0, 0 },  

  { "kill"     , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , POS_FIGHTING, do_kick     , 1, 0 },
  { "kiss"     , POS_RESTING , do_action   , 0, 0 },
  { "knight"   , POS_DEAD    , do_knight   , 0, 0 },

  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK },
  { "laugh"    , POS_RESTING , do_action   , 0, 0 },
  { "last"     , POS_DEAD    , do_last     , LVL_GOD, 0 },
  { "laston"   , POS_DEAD    , do_laston   , 25, 0},
  { "leave"    , POS_STANDING, do_leave    , 0, 0 },
  { "levels"   , POS_DEAD    , do_levels   , 0, 0 },
  { "lie"      , POS_DEAD    , do_action   , 0, 0 },   
  { "list"     , POS_STANDING, do_not_here , 0, 0 },
  { "lick"     , POS_RESTING , do_action   , 0, 0 },
  { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK },
  { "load"     , POS_DEAD    , do_load     , LVL_GOD, 0 },
  { "lol"      , POS_RESTING , do_action   , 0, 0 },   
  { "love"     , POS_RESTING , do_action   , 0, 0 },
  { "lure"     , POS_RESTING , do_action   , 0, 0 }, 

  { "massage"  , POS_RESTING , do_action   , 0, 0 },
  { "memorise" , POS_STANDING, do_memorise, 0, 0 },
  { "mgrin"    , POS_RESTING , do_action   , 0, 0 },   
  { "moan"     , POS_RESTING , do_action   , 0, 0 },
  { "moon"     , POS_DEAD    , do_moon     , 0, 0 },
  { "mortalkombat", POS_STANDING, do_mortal_kombat, 0, 0 },   
  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
  { "mount"    , POS_STANDING, do_mount    , 0, 0},
  { "mail"     , POS_STANDING, do_not_here , 1, 0 },
  { "medit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_MEDIT },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER },

  { "nestle"   , POS_DEAD    , do_action   , 0, 0 },
  { "newbie"   , POS_SLEEPING, do_gen_comm , 0, SCMD_NEWBIE },   
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "nibble"   , POS_RESTING , do_action   , 0, 0 },
  { "nod"      , POS_RESTING , do_action   , 0, 0 },
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
  { "noctalk"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOCTALK },
  { "noimmnet" , POS_DEAD    , do_gen_tog  , LVL_ETRNL1, SCMD_NOIMMNET },
  { "noinfo"   , POS_DEAD    , do_gen_tog  , 0 , SCMD_NOINFO },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHASSLE },
  { "nonewbie" , POS_DEAD    , do_gen_tog  , 0, SCMD_NONEWBIE },
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF },
  { "nosummon" , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_IS_GOD, SCMD_NOWIZ },
  { "nudge"    , POS_RESTING , do_action   , 0, 0 },
  { "nuzzle"   , POS_RESTING , do_action   , 0, 0 },

  { "order"    , POS_RESTING , do_order    , 1, 0 },
  { "offer"    , POS_STANDING, do_not_here , 1, 0 },
  { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN },
  { "olc"      , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OLC_SAVEINFO },
  { "oedit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_OEDIT },

  { "put"      , POS_RESTING , do_put      , 0, 0 },
  { "pant"     , POS_RESTING , do_action   , 0, 0 },   
  { "pat"      , POS_RESTING , do_action   , 0, 0 },
  { "page"     , POS_DEAD    , do_page     , LVL_GOD, 0 },
  { "pagelength",POS_DEAD    , do_pagelength,0, 0 },
  { "pagewidth", POS_DEAD    , do_pagewidth, 0, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_PARDON },
  { "pea"      , POS_DEAD    , do_action   , 0, 0 },  
  { "peer"     , POS_RESTING , do_action   , 0, 0 },
  { "pick"     , POS_STANDING, do_gen_door , 1, SCMD_PICK },
  { "picard"   , POS_DEAD    , do_action   , 0, 0 },  
  { "piledrive", POS_FIGHTING, do_piledrive, 0, 0 },
  { "pinch"    , POS_DEAD    , do_action   , 0, 0 },   
  { "pkset"    , POS_DEAD    , do_pkset    , LVL_GOD, 0 },
  { "point"    , POS_RESTING , do_action   , 0, 0 },
  { "poke"     , POS_RESTING , do_action   , 0, 0 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "ponder"   , POS_RESTING , do_action   , 0, 0 },
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFIN },
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFOUT },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "pout"     , POS_RESTING , do_action   , 0, 0 },
  { "prod"     , POS_DEAD    , do_action   , 0, 0 },      
  { "prompt"   , POS_DEAD    , do_display  , 0, 0 },
  { "promote"  , POS_STANDING, do_promote  , 0, 0 },
  { "practice" , POS_RESTING , do_practice , 1, 0 },
  { "pray"     , POS_SITTING , do_action   , 0, 0 },
  { "primal"   , POS_STANDING, do_scream   , 0, 0 },
  { "puke"     , POS_RESTING , do_action   , 0, 0 },
  { "punch"    , POS_RESTING , do_action   , 0, 0 },
  { "puppy"    , POS_DEAD    , do_action   , 0, 0 },     
  { "purr"     , POS_RESTING , do_action   , 0, 0 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_GOD, 0 },
  { "play"     , POS_RESTING , do_not_here , 0, 0 },  

  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_ANGEL, SCMD_QECHO },
  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
  { "questlog" , POS_DEAD    , do_quest_log, LVL_GOD, 0 },
  { "queston"  , POS_DEAD    , do_queston  , LVL_ANGEL, 0 },
  { "questoff" , POS_DEAD    , do_questoff , LVL_ANGEL, 0 },  
//  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
  { "qui"      , POS_DEAD    , do_quit     , 0, 0 },
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY },

  { "race"     , POS_RESTING , do_race     , 0, 0 },    
  { "reply"    , POS_SLEEPING, do_reply    , 0, 0 },
  { "rest"     , POS_RESTING , do_rest     , 0, 0 },
  { "read"     , POS_RESTING , do_look     , 0, SCMD_READ },
  { "realtime" , POS_SLEEPING, do_realtime , 0, 0 },   
  { "reload"   , POS_FIGHTING,  do_loadweapon, 0, 0},  
  { "reloadf"  , POS_DEAD    , do_reboot   , LVL_IMPL, 0 },
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE },
  { "receive"  , POS_STANDING, do_not_here , 1, 0 },
  { "recruit"  , POS_RESTING , do_recruit  , 0, 0 },
  { "rejoice"  , POS_RESTING , do_action   , 0, 0 },     
  { "remove"   , POS_RESTING , do_remove   , 0, 0 },
  { "remort"   , POS_STANDING, do_remort   , LVL_IMMORT, 0},
  { "rent"     , POS_STANDING, do_not_here , 1, 0 },
  { "repair"   , POS_STANDING, do_armourcraft, 0, 0},
  { "report"   , POS_RESTING , do_report   , 0, 0 },
  { "reroll"   , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_REROLL },
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_GOD, 0 },
  { "retreat"  , POS_FIGHTING, do_retreat  , 1, 0 }, 
  { "return"   , POS_DEAD    , do_return   , 0, 0 },
  { "rglow"    , POS_SITTING , do_action   , 0, 0 },
  { "rofl"     , POS_SITTING , do_action   , 0, 0 },   
  { "redit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_REDIT },
  { "roll"     , POS_RESTING , do_action   , 0, 0 },
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_ETRNL2, SCMD_ROOMFLAGS },
  { "roses"    , POS_RESTING , do_action   , 0, 0 },    
  { "ruffle"   , POS_STANDING, do_action   , 0, 0 },

  { "say"      , POS_RESTING , do_say      , 0, 0 },
  { "'"        , POS_RESTING , do_say      , 0, 0 },
  { "save"     , POS_SLEEPING, do_save     , 0, 0 },
  { "salute"   , POS_DEAD    , do_action   , 0, 0 },   
  { "scan"     , POS_STANDING, do_scan     , 0, 0 },
  { "score"    , POS_DEAD    , do_score    , 0, 0 },
  { "scratch"  , POS_DEAD    , do_action   , 0, 0 },
  { "scream"   , POS_RESTING , do_action   , 0, 0 },
  { "search"   , POS_STANDING, do_search   , 0, 0 },
  { "sell"     , POS_STANDING, do_not_here , 0, 0 },
  { "send"     , POS_SLEEPING, do_send     , LVL_GOD, 0 },
  { "serve"    , POS_SLEEPING, do_action   , 0, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_GOD, 0 },
  { "sedit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_SEDIT },
  { "shoot"    , POS_FIGHTING, do_shoot    , 0, SCMD_HIT }, 
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "shake"    , POS_RESTING , do_action   , 0, 0 },
  { "shandshake" , POS_SITTING , do_action   , 0, 0 },
  { "shed"     , POS_SITTING , do_action   , 0, 0 },        
  { "shiver"   , POS_RESTING , do_action   , 0, 0 },
  { "show"     , POS_DEAD    , do_show     , LVL_ETRNL6, 0 },  
  { "showoff"  , POS_SITTING , do_action   , LVL_IMMORT, 0 },   
  { "shrug"    , POS_RESTING , do_action   , 0, 0 },
  { "shudder"  , POS_DEAD    , do_action   , 0, 0 },     
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_IMPL, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN },
  { "sigh"     , POS_RESTING , do_action   , 0, 0 },
  { "signup"   , POS_RESTING , do_signup   , 0, 0 }, 
  { "sing"     , POS_RESTING , do_action   , 0, 0 },
  { "sink"     , POS_STANDING, do_action   , 0, 0 },    
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "sit"      , POS_RESTING , do_sit      , 0, 0 },
  { "sitn"     , POS_DEAD    , do_action   , 0, 0 },
  { "skills"   , POS_DEAD    , do_spellshow, 0, 0 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GRGOD, 0 },
  { "skillshow", POS_SLEEPING, do_skillshow, LVL_GOD, 0 },    
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slay"     , POS_STANDING, do_slay     , 0, 0 },   
  { "slap"     , POS_RESTING , do_action   , 0, 0 },
  { "slots"    , POS_RESTING , do_slots    , 0, 0 },  
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_SLOWNS },
  { "smile"    , POS_RESTING , do_action   , 0, 0 },
  { "smirk"    , POS_RESTING , do_action   , 0, 0 },
  { "smooch"   , POS_DEAD    , do_action   , 0, 0 },  
  { "snicker"  , POS_RESTING , do_action   , 0, 0 },
  { "snap"     , POS_RESTING , do_action   , 0, 0 },
  { "snarl"    , POS_RESTING , do_action   , 0, 0 },
  { "sneeze"   , POS_RESTING , do_action   , 0, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 1, 0 },
  { "sniff"    , POS_RESTING , do_action   , 0, 0 },
  { "snore"    , POS_SLEEPING, do_action   , 0, 0 },
  { "snort"    , POS_RESTING , do_action   , 0, 0 },       
  { "snowball" , POS_STANDING, do_action   , LVL_ETRNL1, 0 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_GRGOD, 0 },
  { "snuggle"  , POS_RESTING , do_action   , 0, 0 },
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
  { "spells"   , POS_DEAD    , do_spellshow, 0, 0 },
  { "spellshow", POS_DEAD    , do_spellshow, 0, 0 },
  { "split"    , POS_SITTING , do_split    , 1, 0 },
  { "spank"    , POS_RESTING , do_action   , 0, 0 },
  { "spin"     , POS_RESTING , do_action   , 0, 0 }, 
  { "spit"     , POS_STANDING, do_action   , 0, 0 },
  { "spy"      , POS_STANDING, do_spy      , 0, 0 }, 
  { "squeeze"  , POS_RESTING , do_action   , 0, 0 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0 },
  { "stake"    , POS_STANDING, do_slay     , 0, 0 },      
  { "stare"    , POS_RESTING , do_action   , 0, 0 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_ETRNL8, 0 },
  { "steal"    , POS_STANDING, do_steal    , 1, 0 },
  { "steam"    , POS_RESTING , do_action   , 0, 0 },
  { "strangle" , POS_RESTING , do_action   , 0, 0 },
  { "stretch"  , POS_RESTING , do_action   , 0, 0 },
  { "strip"    , POS_DEAD    , do_action   , 0, 0 },  
  { "stroke"   , POS_RESTING , do_action   , 0, 0 },
  { "strut"    , POS_STANDING, do_action   , 0, 0 },
  { "sulk"     , POS_RESTING , do_action   , 0, 0 },
  { "switch"   , POS_DEAD    , do_switch   , LVL_GRGOD, 0 },
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_GOD, 0 },

  { "tell"     , POS_DEAD    , do_tell     , 0, 0 },
  { "tag"      , POS_STANDING, do_tag      , 0, 0 },   
  { "tackle"   , POS_RESTING , do_action   , 0, 0 },
  { "take"     , POS_RESTING , do_get      , 0, 0 },
  { "tango"    , POS_STANDING, do_action   , 0, 0 },
  { "taunt"    , POS_RESTING , do_action   , 0, 0 },
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "teleport" , POS_DEAD    , do_teleport , LVL_GOD, 0 },
  { "tedit"    , POS_DEAD    , do_tedit    , LVL_GRGOD, 0 },  /* XXX: Oasisify */
  { "thank"    , POS_RESTING , do_action   , 0, 0 },
  { "think"    , POS_RESTING , do_action   , 0, 0 },
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW },
  { "throw"    , POS_STANDING, do_throw    , 0, 0 },
  { "title"    , POS_DEAD    , do_title    , 0, 0 },
  { "tic"      , POS_DEAD    , do_tic      , LVL_IMPL, 0 },       
  { "tickle"   , POS_RESTING , do_action   , 0, 0 },
  { "tictac"   , POS_RESTING , do_action   , 0, 0 },  
  { "time"     , POS_DEAD    , do_time     , 0, 0 },
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },
  { "tongue"   , POS_DEAD    , do_action   , 0, 0 },  
  { "track"    , POS_STANDING, do_track    , 0, 0 },
  { "train"    , POS_STANDING, do_not_here , 0, 0 },  
  { "trackthru", POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_TRACK },
  { "transfer" , POS_SLEEPING, do_trans    , LVL_GOD, 0 },
  { "trip"     , POS_FIGHTING, do_trip     , 0, 0 },
  { "tug"      , POS_RESTING , do_action   , 0, 0 },    
  { "twiddle"  , POS_RESTING , do_action   , 0, 0 },
  { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },

  { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0 },
  { "unban"    , POS_DEAD    , do_unban    , LVL_IMPL, 0 },
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_UNAFFECT },
  { "uptime"   , POS_DEAD    , do_date     , LVL_ETRNL1, SCMD_UPTIME },
  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE },
  { "users"    , POS_DEAD    , do_users    , LVL_GOD, 0 },

  { "value"    , POS_STANDING, do_not_here , 0, 0 },
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "visible"  , POS_RESTING , do_visible  , 1, 0 },
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_ANGEL, 0 },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_ANGEL, 0 },
  { "vulcan"   , POS_SITTING , do_pinch    , LVL_GOD, 0 },  

  { "wake"     , POS_SLEEPING, do_wake     , 0, 0 },
  { "wave"     , POS_RESTING , do_action   , 0, 0 },
  { "wash"     , POS_RESTING , do_action   , 0, 0 },   
  { "wear"     , POS_RESTING , do_wear     , 0, 0 },
  { "weather"  , POS_RESTING , do_weather  , 0, 0 },
  { "wedgie"   , POS_DEAD    , do_action   , 0, 0 },
  { "wetwilly" , POS_DEAD    , do_action   , 0, 0 },
  { "who"      , POS_DEAD    , do_who      , 0, 0 },
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "where"    , POS_RESTING , do_where    , 1, 0 },
  { "whap"     , POS_DEAD    , do_action   , 0, 0 }, 
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "whine"    , POS_RESTING , do_action   , 0, 0 },
  { "whistle"  , POS_RESTING , do_action   , 0, 0 },
  { "wield"    , POS_RESTING , do_wield    , 0, 0 },
  { "wiggle"   , POS_STANDING, do_action   , 0, 0 },
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },
  { "wink"     , POS_RESTING , do_action   , 0, 0 },
  { "withdraw" , POS_STANDING, do_not_here , 1, 0 },
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_GOD, 0 },
  { ";"        , POS_DEAD    , do_wiznet   , LVL_GOD, 0 },
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_IMMORT, SCMD_WIZHELP },
  { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_IMPL, 0 },
  { "woohoo"  , POS_RESTING , do_action   , 0, 0 },    
  { "worship"  , POS_RESTING , do_action   , 0, 0 },
  { "write"    , POS_STANDING, do_write    , 1, 0 },

  { "yawn"     , POS_RESTING , do_action   , 0, 0 },
  { "yodel"    , POS_RESTING , do_action   , 0, 0 },

  { "zedit"    , POS_DEAD    , do_oasis    , LVL_BUILDER, SCMD_OASIS_ZEDIT },
  { "zreset"   , POS_DEAD    , do_zreset   , LVL_GOD, 0 },

  { "\n", 0, 0, 0, 0 } };	/* this must be last */


const char *fill[] =
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

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  char *line;

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

  /* otherwise, find the command */
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
	break;

  if (*cmd_info[cmd].command == '\n') {
    send_to_char("Huh?!?\r\n", ch);
  }
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
  else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IS_GOD)
    send_to_char("You can't use immortal commands while switched.\r\n", ch);
  else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
      break;
    case POS_STUNNED:
      send_to_char("All you can do right now is think about the stars!\r\n", ch);
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


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char("Currently defined aliases:\r\n", ch);
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(" None.\r\n", ch);
    else {
      while (a != NULL) {
	sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
	send_to_char(buf, ch);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char("No such alias.\r\n", ch);
      else
	send_to_char("Alias deleted.\r\n", ch);
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
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
    }
  }
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
  return (search_block(argument, fill, TRUE) >= 0);
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
    log("SYSERR: one_argument received a NULL pointer!");
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
 * returnss 1 if arg1 is an abbreviation of arg2
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
  REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
  REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
    SEND_TO_Q("Reconnecting.\r\n", d);
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
    sprintf(buf, "&G%s&g has reconnected.", GET_NAME(d->character));
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
    break;
  case UNSWITCH:
    SEND_TO_Q("Reconnecting to unswitched char.", d);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
    break;
  }

  return (1);
}

void show_stats(struct descriptor_data *d) {

	sprintf(buf, "\r\n  Str: %d\r\n  Int: %d\r\n  Wis: %d\r\n  Con: %d\r\n  Dex: %d\r\n  Cha: %d\r\n",
	    GET_STR(d->character), GET_INT(d->character), GET_WIS(d->character),
	    GET_CON(d->character), GET_DEX(d->character), GET_CHA(d->character));
	SEND_TO_Q(buf, d);

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
    { -1, NULL }
  };

  skip_spaces(&arg);

  /*
   * Quick check for the OLC states.
   */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */ 
  switch (STATE(d)) {
  case CON_QCOLOUR:
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }
    switch(LOWER(*arg)) {
	case 'y':
	      SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_2); break;
	default: 
	      break;
    }
    SEND_TO_Q(GREETINGS, d);
    STATE(d) = CON_GET_NAME;
    break;
  case CON_GET_NAME:		/* wait for input of name */
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
	SEND_TO_Q("Invalid name, please try another.\r\n"
		  "Name: ", d);
	return;
      }
      if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {
        if( PRF_FLAGGED(d->character, PRF_COLOR_2))
		trigger_colour = 1;
	store_to_char(&tmp_store, d->character);
	GET_PFILEPOS(d->character) = player_i;

	if (PLR_FLAGGED(d->character, PLR_DELETED)) {
	  /* We get a false positive from the original deleted character. */
	  if( PRF_FLAGGED(d->character, PRF_COLOR_2) )
		trigger_colour = 1;
	  free_char(d->character);
	  /* Check for multiple creations... */
	  if (!Valid_Name(tmp_name)) {
	    SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
	    return;
	  }
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  d->character->desc = d;
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  GET_PFILEPOS(d->character) = player_i;
	  if( trigger_colour )
	        SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_2); 
	  sprintf(buf, "&gDid I get that right, %s (Y/N)?&n ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT(PLR_FLAGS(d->character),
		     PLR_WRITING | PLR_MAILING | PLR_CRYO);
	  REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
	  SEND_TO_Q("Password: ", d);
	  echo_off(d);
	  d->idle_tics = 0;
	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */
	/* Check for multiple creations of a character. */
	if (!Valid_Name(tmp_name)) {
	  SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
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
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
		GET_PC_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	SEND_TO_Q("&RSorry, new players can't be created at the moment.\r\n&n", d);
	sprintf(buf, "Request for new char %s denied from [%s] (wizlock)",
		GET_PC_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	STATE(d) = CON_CLOSE;
	return;
      }
      SEND_TO_Q("   &WNew character! &n\r\n", d);
      sprintf(buf, "What password shall identify you, %s: ",GET_PC_NAME(d->character));
      SEND_TO_Q(buf, d);
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      SEND_TO_Q("Okay, what IS it, then? ", d);
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

    echo_on(d);    /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    SEND_TO_Q("\r\n", d);

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	mudlog(buf, BRF, LVL_GOD, TRUE);
	GET_BAD_PWS(d->character)++;
	save_char(d->character, NOWHERE);
	if (++(d->bad_pws) >= max_bad_pws) {	/* 3 strikes and you're out. */
	  SEND_TO_Q("&RWrong password... &ndisconnecting.\r\n", d);
	  STATE(d) = CON_CLOSE;
	} else {
	  SEND_TO_Q("&rWrong password.&n\r\nPassword: ", d);
	  echo_off(d);
	}
	return;
      }

      /* Password was correct. */
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	SEND_TO_Q("Sorry, this char has not been cleared for login from your site!\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Connection attempt for %s denied from %s",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      if (GET_LEVEL(d->character) < circle_restrict) {
	SEND_TO_Q("The game is currently restricted.. try again later.\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      /* check and make sure no other copies of this player are logged in */
      if (perform_dupe_check(d))
	return;

      if (GET_LEVEL(d->character) >= LVL_ANGEL)
	SEND_TO_Q(imotd, d);
      else
	SEND_TO_Q(motd, d);

      sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);

      if (load_result) {
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
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_PC_NAME(d->character))) {
      SEND_TO_Q("\r\nIllegal password.\r\n", d);
      SEND_TO_Q("Password: ", d);
      return;
    }
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    SEND_TO_Q("\r\nPlease retype password: ", d);
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;

    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nPasswords don't match... start over.\r\n", d);
      SEND_TO_Q("Password: ", d);
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD) {
      SEND_TO_Q("\r\nWhat is your characters sex (M/F)? ", d);
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character, NOWHERE);
      echo_on(d);
      SEND_TO_Q("\r\nDone.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }

    break;

  case CON_QSEX:		/* query sex of new user         */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      SEND_TO_Q("That is not a sex..\r\n"
		"What IS your characters sex? ", d);
      return;
    }

    SEND_TO_Q(race_menu, d);
    SEND_TO_Q("\r\nRace: ", d);
    STATE(d) = CON_QRACE;
    break;

  case CON_QRACE:
	load_result = parse_race_name(arg);
	if( load_result == RACE_UNDEFINED ) {
		SEND_TO_Q("\r\nThat's not a race.\r\nRace: ", d);
		return;
	}
	else { 
	   GET_RACE(d->character) = load_result;
        }

    	SEND_TO_Q(class_menu, d);
    	SEND_TO_Q("Class: ", d);
	STATE(d) = CON_QCLASS;
	break;
  case CON_QCLASS:
    load_result = parse_class(arg);
    if (load_result == CLASS_UNDEFINED) {
      SEND_TO_Q("\r\nThat's not a class.\r\nClass: ", d);
      return;
    } else {
      GET_CLASS(d->character) = load_result;
      GET_MODIFIER(d->character) = class_modifiers[load_result]; // no specials yet
    }

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    save_char(d->character, NOWHERE);
    NUM_PLAYERS++;

    // God gets no rerolls
    if( GET_IDNUM(d->character) == 1 ) {
	SEND_TO_Q("\r\nWelcome master. * Press ENTER to continue : ", d);
	STATE(d) = CON_RMOTD;
	break;
    } 
    roll_real_abils(d->character); 
    show_stats(d);
    SEND_TO_Q("\r\nAccept these stats [y/n]: ", d);
    STATE(d) = CON_QSTATCHECK;
    break;
  case CON_QSTATCHECK:
    switch(LOWER(*arg)) {
	case 'y': break;
	case 'n': 
		  roll_real_abils(d->character); 
		  show_stats(d); 
		  SEND_TO_Q("\r\nAccept these stats [y/n]: ", d); 
		  return;
	default: SEND_TO_Q("\r\nPlease press 'y' or 'n' only : \r\n", d);
		 return;
    }

    SEND_TO_Q(motd, d);
    SEND_TO_Q("\r\n Character created. * Press ENTER to continue: ", d);
    STATE(d) = CON_RMOTD;

    sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, LVL_ETRNL1, TRUE);
    break;

  case CON_RMOTD:		/* read CR after printing motd   */
    SEND_TO_Q(MENU, d);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU:		/* get selection from main menu  */
    switch (*arg) {
    case '0':
      SEND_TO_Q("Goodbye.\r\n", d);
      STATE(d) = CON_CLOSE;
      break;

    case '1':
      reset_char(d->character);
      read_aliases(d->character);

      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

      /*
       * We have to place the character in a room before equipping them
       * or equip_char() will gripe about the person in NOWHERE.
       */
      if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
	load_room = real_room(load_room);

      /* If char was saved with NOWHERE, or real_room above failed... */
      if (load_room == NOWHERE) {
	if (GET_LEVEL(d->character) >= LVL_IMMORT)
	  load_room = r_immort_start_room;
	else
	  load_room = r_mortal_start_room;
      }

      if (PLR_FLAGGED(d->character, PLR_FROZEN))
	load_room = r_frozen_start_room;

      send_to_char(WELC_MESSG, d->character);
      d->character->next = character_list;
      character_list = d->character;
      char_to_room(d->character, load_room);
      load_result = Crash_load(d->character);
      save_char(d->character, NOWHERE);

      sprintf(buf,"world_start_rooms: %d %d %d",world_start_room[0],
          world_start_room[1], world_start_room[2]);
      log(buf);

      if(real_room(ENTRY_ROOM(d->character,WORLD_MEDIEVAL)) <= 0)
        ENTRY_ROOM(d->character,WORLD_MEDIEVAL) =
          world_start_room[WORLD_MEDIEVAL];
      if(real_room(ENTRY_ROOM(d->character,WORLD_WEST)) <= 0)
        ENTRY_ROOM(d->character,WORLD_WEST) = 
          world_start_room[WORLD_WEST];
      if(real_room(ENTRY_ROOM(d->character,WORLD_FUTURE)) <= 0)
        ENTRY_ROOM(d->character,WORLD_FUTURE) = 
          world_start_room[WORLD_FUTURE]; 

      sprintf(buf,"&G%s&g has entered the game.", GET_NAME(d->character));
      info_channel(buf, d->character);  

      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

      STATE(d) = CON_PLAYING;
      if (GET_LEVEL(d->character) == 0) {
	do_start(d->character);
	send_to_char(START_MESSG, d->character);
      }
      look_at_room(d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char("You have mail waiting.\r\n", d->character);

      /* DM_exp check if using old exp system on login (have more exp than required at this level) */
      if (GET_EXP(d->character) > level_exp((d->character),GET_LEVEL(d->character))) {
        GET_EXP(d->character) = 0;
      } 

      if (GET_EXP(d->character) < 0)
        GET_EXP(d->character) = 0;

      if (load_result == 2) {	/* rented items lost */
	send_to_char("\r\n\007You could not afford your rent!\r\n"
	  "Your possesions have been donated to the Salvation Army!\r\n",
		     d->character);
      }
      d->has_prompt = 0;
      break;

    case '2':
      if (d->character->player.description) {
	SEND_TO_Q("Old description:\r\n", d);
	SEND_TO_Q(d->character->player.description, d);
	free(d->character->player.description);
	d->character->player.description = NULL;
      }
      SEND_TO_Q("Enter the new text you'd like others to see when they look at you.\r\n", d);
      SEND_TO_Q("Terminate with a '@' on a new line.\r\n", d);
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
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
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
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
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
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
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
    } else {
      SEND_TO_Q("\r\nCharacter not deleted.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }
    break;

  /*
   * It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them.
   */
  case CON_CLOSE:
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}
