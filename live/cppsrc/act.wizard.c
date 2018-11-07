/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#include <fcntl.h>  // open() used in john_in, cassandra_in 
#include <unistd.h>

#include <list>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "genzon.h"
#include "balance.h"
#include "clan.h" // -- ARTUS
#include "dg_scripts.h"
#include "quest.h"

/*   external vars  */
extern const char *enhancement_names[];
extern const char *level_bits[];
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern time_t boot_time;
extern zone_rnum top_of_zone_table;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern int top_of_p_table;
extern int spell_sort_info[NUM_SORT_TYPES][NUM_CLASSES][MAX_SKILLS+1]; 
extern const char *special_ability_bits[];
extern int num_of_houses;
extern struct house_control_rec house_control[];
extern struct event_list events;
extern int num_rooms_burgled;
extern const char *social_ranks[];
/* for chars */
extern const char *pc_class_types[];
extern const char *pc_race_types[];
extern Burglary *burglaries;
extern const char *unused_spellname;

/* extern functions */
int level_exp(struct char_data *ch, int level);
void show_shops(struct char_data * ch, char *value);
void do_start(struct char_data *ch);
void appear(struct char_data *ch);
void reset_zone(zone_rnum zone);
void roll_real_abils(struct char_data *ch);
int parse_class(char *arg);
void remove_class_specials(struct char_data *ch);
void set_class_specials(struct char_data *ch);
void set_race_specials(struct char_data *ch);
void remove_race_specials(struct char_data *ch);
void apply_specials(struct char_data *ch, bool initial);
int parse_race_name(char *arg);
int mag_manacost(struct char_data * ch, int spellnum);
void hcontrol_list_houses(struct char_data * ch, char *arg);
void send_to_zone(const char *msg, zone_rnum zone);
void send_to_not_zone_world(const char *msg, zone_rnum zone);
void list_events_to_char(struct char_data *ch, int specific);
int compute_armor_class(struct char_data *ch, bool divide);
byte saving_throws(struct char_data *ch, int type); 
void generate_zone_data(void);
void perform_remove(struct char_data * ch, int pos);
void remove_mud_event(struct event_data *ev);
ACMD(do_qcomm);


/* local functions */
int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
void perform_immort_invis(struct char_data *ch, int level);
ACMD(do_echo);
ACMD(do_balance);
ACMD(do_debug_cmd);
ACMD(do_send);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
void do_stat_room(struct char_data * ch);
void do_stat_object(struct char_data * ch, struct obj_data * j);
void do_stat_character(struct char_data * ch, struct char_data * k);
ACMD(do_stat);
ACMD(do_shutdown);
void stop_snooping(struct char_data * ch);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
void perform_immort_vis(struct char_data *ch);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
void print_zone_to_buf(char *bufptr, zone_rnum zone, int level, bool detailed);
ACMD(do_show);
ACMD(do_set);

/* Primal Functions */
void john_in(struct char_data *ch);
void cassandra_in(struct char_data *ch);
void artus_in(struct char_data *ch);
void artus_out(struct char_data *ch);
ACMD(do_demort);
ACMD(do_immort);
ACMD(do_pinch);
ACMD(do_pkset);
ACMD(do_queston);
ACMD(do_questoff);
ACMD(do_skillshow);
ACMD(do_tic);
ACMD(do_whostr);

void do_eq_find(struct char_data *ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int where = -1, minlev = 0, maxlev = 100, found = 0, i;
  bool light = false, stab = false;
 
  if (!argument || !*argument)
  {
    send_to_char("Usage: debug eqfind <where> <min-max>\r\n", ch);
    return;
  }
  skip_spaces(&argument);
  half_chop(argument, arg1, arg2);
  if (is_abbrev(arg1, "wield"))		where = ITEM_WEAR_WIELD;
  else if (is_abbrev(arg1, "about"))	where = ITEM_WEAR_ABOUT;
  else if (is_abbrev(arg1, "ankle"))	where = ITEM_WEAR_ANKLE;
  else if (is_abbrev(arg1, "arms"))	where = ITEM_WEAR_ARMS;
  else if (is_abbrev(arg1, "body"))	where = ITEM_WEAR_BODY;
  else if (is_abbrev(arg1, "ear"))	where = ITEM_WEAR_EAR;
  else if (is_abbrev(arg1, "eyes"))	where = ITEM_WEAR_EYE;
  else if (is_abbrev(arg1, "feet"))	where = ITEM_WEAR_FEET;
  else if (is_abbrev(arg1, "finger"))	where = ITEM_WEAR_FINGER;
  else if (is_abbrev(arg1, "hands"))	where = ITEM_WEAR_HANDS;
  else if (is_abbrev(arg1, "head"))	where = ITEM_WEAR_HEAD;
  else if (is_abbrev(arg1, "hold"))	where = ITEM_WEAR_HOLD;
  else if (is_abbrev(arg1, "legs"))	where = ITEM_WEAR_LEGS;
  else if (is_abbrev(arg1, "neck"))	where = ITEM_WEAR_NECK;
  else if (is_abbrev(arg1, "shield"))	where = ITEM_WEAR_SHIELD;
  else if (is_abbrev(arg1, "waist"))	where = ITEM_WEAR_WAIST;
  else if (is_abbrev(arg1, "wrist"))	where = ITEM_WEAR_WRIST;
  else if (is_abbrev(arg1, "light"))
  {
    where = ITEM_WEAR_HOLD;
    light = true;
  } else if (is_abbrev(arg1, "stab")) {
    where = ITEM_WEAR_WIELD;
    stab = true;
  } else {
    send_to_char("Where exactly is that?\r\n", ch);
    return;
  }
  // Artus> Hacky level stuff.
  if (*arg2)
    for (i = 0; i < (int)strlen(arg2); i++)
    {
      if (arg2[i] == '\0')
      {
	minlev = atoi(arg2);
	break;
      }
      if (arg2[i] == '-')
      {
	arg2[i] = '\0';
	if ((i > 0) && is_number(arg2))
	  minlev = atoi(arg2);
	if (is_number(&arg2[i+1]))
	  maxlev = atoi(&arg2[i+1]);
	break;
      }
    }

  send_to_char("Vnum  - Name\r\n", ch);
  // Should be safe to assume we have a wear bit, and a min level.
  for (i = 0; i < top_of_objt; i++)
  {
    if (!CAN_WEAR(&obj_proto[i], where))
      continue;
    if (light && (GET_OBJ_TYPE(&obj_proto[i]) != ITEM_LIGHT))
      continue;
    if ((GET_OBJ_LEVEL(&obj_proto[i]) < minlev) ||
        (GET_OBJ_LEVEL(&obj_proto[i]) > maxlev))
      continue;
    if (stab && (obj_proto[i].obj_flags.value[3] != TYPE_PIERCE - TYPE_HIT))
      continue;
    sprintf(buf, "&c%5d&n -&g %s&n\r\n", obj_index[i].vnum, 
	    obj_proto[i].short_description);
    send_to_char(buf, ch);
    found++;
    if (found >= 100)
    {
      send_to_char("Too many to list, try a smaller level range.\r\n", ch);
      return;
    }
  }
  if (found < 1)
    send_to_char("Couldn't find any objects matching those parameters.\r\n",ch);
}

// Cream pies.
ACMD(do_cream)
{
  struct char_data *vict;
  struct affected_type af;
  one_argument(argument,arg);
  
  if (!(vict = generic_find_char(ch, arg, FIND_CHAR_WORLD)))
  {
    send_to_char("Who is it that you wish to cream?\r\n", ch);
    return;
  }
  if (IS_NPC(vict))
  {
    send_to_char("You can't cream mobs!\r\n", ch);
    return;
  }

  if (GET_LEVEL(vict) > GET_LEVEL(ch))
  {
    send_to_char("You miss!\r\n", ch);
    sprintf(buf, "%s attempts to hit you with a cream pie, but misses completely\r\n", GET_NAME(ch));
    send_to_char(buf, vict);
    return;
  }

  sprintf(buf, "You hit %s in the face with a cream pie!\r\n", GET_NAME(vict));
  send_to_char(buf, ch);

  send_to_char("A cream pie falls from the heavens and hits you in the face!\r\nYick, you've been creamed!\r\nAn evil laugh echoes over the land.\r\n", vict);

  af.type = SPELL_CREAMED;
  af.location = APPLY_HITROLL;
  af.modifier = -4;
  af.duration = 3;
  af.bitvector = AFF_BLIND;
  affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);
  return;
}


ACMD(do_debug_cmd)
{
  skip_spaces(&argument);
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  void init_gold_rush(void);
  void create_event_happy(struct char_data *ch);
  struct event_data *find_quest_event(void);
  int i, count;

  if(!*argument)
  {
#ifndef IGNORE_DEBUG
    sprintf(buf, "Debug Status : %s\r\n", (GET_DEBUG(ch) ? "On" : "Off"));
#endif
    strcpy(buf, "Debug Options:\r\n");
    strcat(buf, 
#ifndef IGNORE_DEBUG
	        "  &cOn&n        - Enable Debugging\r\n"
		"  &cOff&n       - Disable Debugging\r\n"
#endif
		"  &calevil&n    - Really evil aligned mobs.\r\n"
		"  &calgood&n    - Really good aligned mobs.\r\n"
		"  &cch&n        - Am I on the character list?\r\n"
		"  &cev&n        - Event List\r\n"
		"  &codam&n      - Object Damage (<item> <new dam val>)\r\n"
		"  &cportals&n   - List portals and their destinations.\r\n"
		"  &cqmobact&n   - Active mobs with QUEST bit set.\r\n"
		"  &cqmobproto&n - Mob prototypes with QUEST bit set.\r\n"
		"  &cqobjact&n   - Active objs with QUEST bit set.\r\n"
		"  &cqobjproto&n - Mob prototypes with QUEST bit set.\r\n"
		"  &crmsm&n      - Room's Small Bits\r\n"
		"  &cwentroom&n  - Rooms leading to this room.\r\n"
		"  &cwentgate&n  - Objects leading to this room.\r\n"
		"  &czentroom&n  - Rooms leading to this zone.\r\n"
		"  &czentgate&n  - Objects leading to this zone.\r\n");
    send_to_char(buf, ch);
    return;
  }
  half_chop(argument,arg1,arg2);
#ifndef IGNORE_DEBUG
  if (!str_cmp(arg1, "off"))
  {
    REMOVE_BIT(SMALL_BITS(ch), SMB_DEBUG);
    send_to_char("Debugging Off.\r\n", ch);
    return;
  }
  if (!str_cmp(arg1, "on"))
  {
    SET_BIT(SMALL_BITS(ch), SMB_DEBUG);
    send_to_char("Debugging On.\r\n", ch);
    return;
  }
#endif
  // Artus> EQ :o)
  if (!str_cmp(arg1, "eqfind"))
  {
    do_eq_find(ch, arg2);
    return;
  }
  // Artus> Active mobs with QUEST bit set.
  if (!str_cmp(arg1, "qmobact"))
  {
    count = 0;
    strcpy(buf, "&gRoom  &n- &cVnum  &n- Name\r\n");
    for (struct char_data *k = character_list; k; k = k->next)
      if (IS_NPC(k) && MOB_FLAGGED(k, MOB_QUEST))
      {
	count++;
	sprintf(buf, "%s&g%5d - %5d - %s\r\n", buf, world[IN_ROOM(k)].number,
	        mob_index[k->nr].vnum, GET_NAME(k));
	if (count > 100)
	{
	  strcat(buf, "Too many to list.\r\n");
	  break;
	}
      }
    if (count > 0)
      page_string(ch->desc, buf, TRUE);
    else
      send_to_char("No active quest mobiles found.\r\n", ch);
    return;
  }
  // Artus> Prototype mobs with QUEST bit set.
  if (!str_cmp(arg1, "qmobproto"))
  {
    count = 0;
    strcpy(buf, "&gVNum  &n- Name\r\n");
    for (i = 0; i < top_of_mobt; i++)
      if (MOB_FLAGGED(&mob_proto[i], MOB_QUEST))
      {
	count++;
	sprintf(buf, "%s&g%5d&n - %s\r\n", buf, mob_index[i].vnum,
	        GET_NAME(&mob_proto[i]));
	if (count > 100)
	{
	  strcat(buf, "Too many to list.\r\n");
	  break;
	}
      }
    if (count > 0)
      page_string(ch->desc, buf, TRUE);
    else
      send_to_char("No prototype quest mobiles found. (This is good :o)\r\n",
	           ch);
    return;
  }
  // Artus> Active ITEM_QUEST objects.
  if (!str_cmp(arg1, "qobjact"))
  {
    count = 0;
    strcpy(buf, "&g Vnum &n- Name\r\n");
    for (struct obj_data *o = object_list; o; o = o->next)
      if (GET_OBJ_TYPE(o) == ITEM_QUEST)
      {
	count++;
	sprintf(buf, "%s&g%5d&n - %s\r\n", buf, o->item_number,
	        o->short_description);
	if (count > 100)
	{
	  strcat(buf, "Too many to list.\r\n");
	  break;
	}
      }
    if (count > 0)
      page_string(ch->desc, buf, TRUE);
    else
      send_to_char("No active quest objects found.\r\n", ch);
    return;
  }
  // Artus> Prototype ITEM_QUEST objects.
  if (!str_cmp(arg1, "qobjproto"))
  {
    count = 0;
    strcpy(buf, "&g VNum &n- Name\r\n");
    for (i = 0; i < top_of_objt; i++)
      if (GET_OBJ_TYPE(&obj_proto[i]) == ITEM_QUEST)
      {
	count++;
	sprintf(buf, "%s&g%5d&n - %s\r\n", buf, obj_index[i].vnum,
	        obj_proto[i].short_description);
	if (count > 100)
	{
	  strcat(buf, "Too many to list.\r\n");
	  break;
	}
      }
    if (count > 0)
      page_string(ch->desc, buf, TRUE);
    else
      send_to_char("No prototype quest objects found (This is good).\r\n", ch);
    return;
  }
  // Artus> Rooms exiting to current room.
  if (!str_cmp(arg1, "wentroom"))
  {
    bool found = false;
    strcpy(buf, "Rooms leading here:");
    for (i = 0; i < top_of_world; i++)
      for (int j = 0; j < NUM_OF_DIRS; j++)
	if (W_EXIT(i, j) && (W_EXIT(i, j)->to_room == IN_ROOM(ch)))
	{
	  sprintf(buf, "%s\r\n&c%5d &g%-5s", buf, world[i].number, dirs[j]);
	  found = true;
	}
    if (found)
    {
      strcat(buf, "&n\r\n");
      page_string(ch->desc, buf, TRUE);
    } else {
      send_to_char("Couldn't find any rooms leading here.\r\n", ch);
    }
    return;
  }
  // Artus> Objs exiting to current room.
  if (!str_cmp(arg1, "wentgate"))
  {
    bool found = false;
    room_vnum myvnum = world[IN_ROOM(ch)].number;
    strcpy(buf, "Gateways leading here:\r\n");
    for (i = 0; i < top_of_objt; i++)
      if ((GET_OBJ_TYPE(&obj_proto[i]) == ITEM_GATEWAY) &&
	  (GET_OBJ_VAL(&obj_proto[i], 0) == myvnum))
      {
	found = true;
	sprintf(buf, "%s&c%5d &n(&5%s&n)\r\n", buf, obj_index[i].vnum,
	        obj_proto[i].short_description);
      }
    if (found)
      page_string(ch->desc, buf, TRUE);
    else
      send_to_char("Couldn't find any objects leading here.\r\n", ch);
    return;
  }
  // Artus> Rooms exiting to current ZONE.
  if (!(str_cmp(arg1, "zentroom")))
  {
    bool found = false;
    zone_rnum targzone = world[IN_ROOM(ch)].zone;
    strcpy(buf, "Rooms leading to this zone:");
    for (i = 0; i < top_of_world; i++)
      if (world[i].zone != targzone)
	for (int j = 0; j < NUM_OF_DIRS; j++)
	  if (W_EXIT(i, j) && (W_EXIT(i, j)->to_room > NOWHERE) &&
	      (world[W_EXIT(i, j)->to_room].zone == targzone))
	  {
	    found = true;
	    sprintf(buf, "%s\r\n&c%5d &g%5s&n - &c%5d", buf,
		    world[i].number, dirs[j],
		    world[W_EXIT(i, j)->to_room].number);
	  }
    if (found)
    {
      strcat(buf, "&n\r\n");
      page_string(ch->desc, buf, TRUE);
    } else {
      send_to_char("Couldn't find any rooms leading to this zone.\r\n", ch);
    }
    return;
  }
  // Artus> Objects exiting to current ZONE.
  if (!(str_cmp(arg1, "zentgate")))
  {
    bool found = false;
    room_vnum zone_bottom, zone_top;
    zone_bottom = zone_table[world[IN_ROOM(ch)].zone].number * 100;
    zone_top = zone_table[world[IN_ROOM(ch)].zone].top;
    strcpy(buf, "Objects leading to this zone:\r\n");
    for (i = 0; i < top_of_objt; i++)
      if ((GET_OBJ_TYPE(&obj_proto[i]) == ITEM_GATEWAY) &&
	  ((GET_OBJ_VAL(&obj_proto[i], 0) >= zone_bottom) &&
	   (GET_OBJ_VAL(&obj_proto[i], 0) <= zone_top)))
      {
	found = true;
	sprintf(buf, "%s&c%5d &n- &c%5d &n(&g%s&n)\r\n", buf, obj_index[i].vnum,
	        GET_OBJ_VAL(&obj_proto[i], 0), obj_proto[i].short_description);
      }
    if (found)
      page_string(ch->desc, buf, TRUE);
    else
      send_to_char("Couldn't find any objects leading to this zone.\r\n", ch);
    return;
  }
  // Artus> Good Aligned Mobs.
  if (!str_cmp(arg1, "algood"))
  {
    bool found = false;
    for (i = 0; i < top_of_mobt; i++)
      if (GET_ALIGNMENT(&mob_proto[i]) >= 1000)
      {
	if (!(found))
	{
	  strcpy(buf, "Align -  Vnum - Name\r\n"
	              "------------------------------------------------\r\n");
	  found = true;
	}
	sprintf(buf, "%s%5d   %5d   %s\r\n", buf, GET_ALIGNMENT(&mob_proto[i]),
	        mob_index[i].vnum, GET_NAME(&mob_proto[i]));
      }
    if (found)
    {
      strcat(buf, "------------------------------------------------\r\n");
      page_string(ch->desc, buf, TRUE);
    } else {
      send_to_char("No good mobs found :o(\r\n", ch);
    }
    return;
  }
  // Artus> Evil Aligned Mobs.
  if (!str_cmp(arg1, "alevil"))
  {
    bool found = false;
    for (i = 0; i < top_of_mobt; i++)
      if (GET_ALIGNMENT(&mob_proto[i]) <= -1000)
      {
	if (!(found))
	{
	  strcpy(buf, "Align -  Vnum - Name\r\n"
		      "------------------------------------------------\r\n");
	  found = true;
	}
	sprintf(buf, "%s%5d   %5d   %s\r\n", buf, GET_ALIGNMENT(&mob_proto[i]),
	        mob_index[i].vnum, GET_NAME(&mob_proto[i]));
      }
    if (found)
    {
      strcat(buf, "------------------------------------------------\r\n");
      page_string(ch->desc, buf, TRUE);
    } else {
      send_to_char("No evil mobs found :o(\r\n", ch);
    }
    return;
  }
  // Artus> List Portals.
  if (!str_cmp(arg1, "portals"))
  {
    send_to_char("------------------------------------------------\r\n", ch);
    for (i = 0; i < top_of_objt; i++)
      if (GET_OBJ_TYPE(&obj_proto[i]) == ITEM_GATEWAY)
      {
	sprintf(buf, "&c%5d&n Teleports to &c%5d&n. (Obj Name: &g%s&n)\r\n", 
	        obj_index[i].vnum, GET_OBJ_VAL(&obj_proto[i], 0),
		obj_proto[i].short_description);
	send_to_char(buf, ch);
      }
    send_to_char("------------------------------------------------\r\n", ch);
    return;
  }
  if (is_abbrev(arg1,"ev"))
  {
    sprintf(buf, "&gDBG: Num Events: %d.\r\n", events.num_events);
    send_to_char(buf, ch);
    for (struct event_data *ev = events.list; ev; ev = ev->next)
    {
      sprintf(buf, "DBG: Ev={chID=%ld, time_taken={hours=%d,day=%d,month=%d,year=%d}, room=%d, type=%d, info1=%ld, info2=%ld, info3=%ld, desc=%s, next=%s}\r\n",
	      ev->chID, ev->time_taken.hours, ev->time_taken.day, ev->time_taken.month, ev->time_taken.year, (ev->room) ? ev->room->number : NOWHERE, ev->type, ev->info1, ev->info2, ev->info3, ev->desc, ((ev->next) ? "present" : "absent"));
      send_to_char(buf, ch);
    }
    return;
  }
  if (is_abbrev(arg1,"rmsm"))
  {
    sprintf(buf, "RMSM_FLAGS(IN_ROOM(ch)=%d): %d\r\n", RMSM_FLAGS(IN_ROOM(ch)),
	    world[IN_ROOM(ch)].small_bits);
    send_to_char(buf, ch);
    return;
  }
  if (is_abbrev(arg1,"ch"))
  {
    for (struct char_data *k = character_list; k; k = k->next)
      if (k == ch)
      {
	send_to_char("DBG: It's You!\r\n", ch);
	return;
      }
    send_to_char("DBG: I can't find you.. This is probably bad.\r\n", ch);
    return;
  }
  if (is_abbrev(arg1, "odam"))
  {
    struct obj_data *tobj = NULL;
    struct char_data *tmpch;
    int newamt = 0;
    half_chop(arg2,arg1,argument);
    if (!(*arg1) || !(*argument)) 
    {
      send_to_char("Syntax: debug odam <item name> <damage amount>\r\n", ch);
      return;
    }
    if (!is_number(argument))
    {
      send_to_char("Damage amount must be a number.\r\n", ch);
      return;
    }
//int generic_find(char *arg, bitvector_t bitvector, struct char_data * ch,
//		     struct char_data ** tar_ch, struct obj_data ** tar_obj)
    if (generic_find(arg1, FIND_OBJ_INV, ch, &tmpch, &tobj) == 0)
    {
      sprintf(buf, "You don't seem to have a %s\r\n", arg1);
      send_to_char(buf, ch);
      return;
    }
    newamt = atoi(argument);
    if (newamt < 0)
      newamt = 0;
    GET_OBJ_DAMAGE(tobj) = MIN(GET_OBJ_MAX_DAMAGE(tobj), newamt);
    send_to_char("Done.\r\n", ch);
    return;
  }
  send_to_char("Debug what?!?\r\n", ch);
}

ACMD(do_balance)
{
  balance_world(top_of_mobt, top_of_objt);
  send_to_char("Regenerating Balance Files...\r\n", ch);
}

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes.. but what?\r\n", ch);
  else {
    if (subcmd == SCMD_EMOTE)
      sprintf(buf, "$n %s", argument);
    else
      strcpy(buf, argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (!IS_NPC(ch)) // Don't Send Anything to Mobs. - ARTUS
    {
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
      else
	act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
  }
}


ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg)
  {
    send_to_char("Send what to who?\r\n", ch);
    return;
  }
  if (!(vict = generic_find_char(ch, arg, FIND_CHAR_WORLD)))
  {
    send_to_char(NOPERSON, ch);
    return;
  }
  send_to_char(buf, vict);
  send_to_char("\r\n", vict);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("Sent.\r\n", ch);
  else {
    sprintf(buf2, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
    send_to_char(buf2, ch);
  }
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data * ch, char *rawroomstr)
{
  room_vnum tmp;
  room_rnum location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr)
  {
    send_to_char("You must supply a room number or name.\r\n", ch);
    return (NOWHERE);
  }
  if (isdigit(*roomstr) && !strchr(roomstr, '.'))
  {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0)
    {
      send_to_char("No room exists with that number.\r\n", ch);
      return (NOWHERE);
    }
  } else if ((tmp = generic_find(roomstr, FIND_CHAR_WORLD | FIND_OBJ_WORLD, ch, 
	                  &target_mob, &target_obj)) < 1)
  {
    send_to_char("No such creature or object around.\r\n", ch);
    return (NOWHERE);
  } else {
    if (tmp == FIND_CHAR_WORLD)
    {
      location = target_mob->in_room;
      if (location == NOWHERE)
      {
	sprintf(buf, "The creature, &6%s&n, is not vailable.\r\n", 
	        PERS(ch, target_mob));
	send_to_char(buf, ch);
	return (NOWHERE);
      }
    } else {
      location = target_obj->in_room;
      if (location == NOWHERE)
      {
	sprintf(buf, "The object, &5%s&n, is not available.\r\n", 
	        OBJS(target_obj, ch));
	send_to_char(buf, ch);
	return (NOWHERE);
      }
    }
  }
  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (LR_FAIL(ch, LVL_IMPL))
  {
    if (ROOM_FLAGGED(location, ROOM_GODROOM))
    {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return (NOWHERE);
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	world[location].people && world[location].people->next_in_room)
    {
      send_to_char("There's a private conversation going on in that room.\r\n", ch);
      return (NOWHERE);
    }
    if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
	!House_can_enter(ch, GET_ROOM_VNUM(location))) 
    {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return (NOWHERE);
    }
  }
  return (location);
}



ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("You must supply a room number or a name.\r\n", ch);
    return;
  }

  if (!*command) {
    send_to_char("What do you want to do there?\r\n", ch);
    return;
  }

  if ((location = find_target_room(ch, buf)) < 0)
    return;

/* stops imms going to reception - Vader */
  if(world[location].number <= 599 && world[location].number >= 500 &&
     LR_FAIL(ch, LVL_IMPL))
  {
    send_to_char("As you fly through time and space towards the reception area\r\n"
               "a black gloved hand grabs you and rips you back to where you came from...\r\n",ch);
    return;
  }

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location)
  {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  room_rnum location;
  struct char_data *target_mob;
  char name_mob[MAX_INPUT_LENGTH];
  extern int allowed_zone(struct char_data * ch,int flag,bool show);
  extern int allowed_room(struct char_data * ch,int flag,bool show);

  // TODO: dont allow goto tag/mortal
  if (PRF_FLAGGED(ch, PRF_MORTALK) && LR_FAIL(ch, LVL_IMPL))
  {
    send_to_char("You cannot goto out of mortalk arena.  Use Recall!\r\n", ch);
    return;
  }
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    if (GET_LEVEL(ch) < LVL_CHAMP)
    {
      send_to_char("Hah, you wish!\r\n", ch);
      return;
    }
    one_argument (argument,name_mob);
    
    if (is_number(name_mob))
    { /* Artus - Allow champs to goto their OLC zone */
      if (GET_OLC_ZONE(ch) < 1)
      {
	send_to_char("You can only go to Player Characters.\r\n", ch);
	return;
      }
      if ((location = real_room(atoi(name_mob))) == NOWHERE)
      {
	send_to_char("No room by that number.\r\n", ch);
	return;
      }
      if (zone_table[world[location].zone].number != GET_OLC_ZONE(ch))
      {
	send_to_char("You can only goto rooms within your OLC zone.\r\n", ch);
	return;
      }
      if (POOFOUT(ch))
        sprintf(buf, "&7$n&n %s", POOFOUT(ch));
      else
        strcpy(buf, "&7$n&n disappears in a puff of smoke.");
 
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
      char_from_room(ch);
      char_to_room(ch, location);

      if (POOFIN(ch))
        sprintf(buf, "&7$n&n %s", POOFIN(ch));
      else
        strcpy(buf, "&7$n&n appears with an ear-splitting bang.");
  
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
      look_at_room(ch, 0);
      return;
    }
    // get_char_vis(xxx, xxx, FALSE == GET_CHAR_ROOM) <- chaned from FALSE to
    // FIND_CHAR_WORLD ... <- DM
    if(((target_mob = generic_find_char(ch, name_mob, FIND_CHAR_WORLD)) == NULL)
      || (world[target_mob->in_room].zone == GOD_ROOMS_ZONE)
      || (!same_world(target_mob,ch)))
    {
      send_to_char("That person does not appear to be anywhere!\r\n",ch);
      return;
    }
    if(IS_NPC(target_mob))
    {
      send_to_char("You can only go to Player Characters!\r\n",ch);
      return;
    }
  } 

  if ((location = find_target_room(ch, argument)) < 0)
    return;

/* stops imms going to reception - Vader */
  if(world[location].number <= 599 && world[location].number >= 500 &&
     LR_FAIL(ch, LVL_IMPL))
  {
    send_to_char("As you fly through time and space towards the reception area\r\n"
               "a guardian spirit tells you in a nice kind voice, \"PISS OFF!\"\r\n",ch);
    return;
  }
 
/* stops imms going to clan halls - Hal
 * ARTUS - Modified 16/04/2001 - Clan zone... */
 
  if ((zone_table[world[location].zone].number == CLAN_ZONE) && 
      (LR_FAIL(ch, LVL_CLAN_GOD)))
  {
    send_to_char("A ward placed over the clan zone prevents you entering via arcane means.\r\n", ch);
    return;
  }

  if (!allowed_zone(ch,zone_table[world[location].zone].zflag,TRUE))
    return;
  if (!allowed_room(ch,world[location].room_flags,TRUE))
    return; 

  if (POOFOUT(ch))
  {
    sprintf(buf, "&7$n&n %s", POOFOUT(ch));
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
  } else {
    if (!str_cmp(ch->player.name, "Artus"))
      artus_out(ch);
    else
    {
      strcpy(buf, "&7$n&n disappears in a puff of smoke.");
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
    }
  }
 
  char_from_room(ch);
  char_to_room(ch, location);

  // DM: Make angels and below wait two violence round
  if (GET_LEVEL(ch) <= LVL_ANGEL)
    WAIT_STATE(ch, PULSE_VIOLENCE*2);

  if (POOFIN(ch))
  {
    sprintf(buf, "&7$n&n %s", POOFIN(ch));
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
  } else {
    if (!strcmp(ch->player.name, "John"))
      john_in(ch);
    else if (!strcmp(ch->player.name, "Cassandra"))
      cassandra_in(ch);
    else if (!strcmp(ch->player.name, "Artus"))
      artus_in(ch);
    else 
    {
      strcpy(buf, "&7$n&n appears with an ear-splitting bang.");
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  look_at_room(ch, 0);
}

ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
  {
    send_to_char("Whom do you wish to transfer?\r\n", ch);
  } else if (str_cmp("all", buf)) {
    if (!(victim = generic_find_char(ch, buf, FIND_CHAR_WORLD)))
      send_to_char(NOPERSON, ch);
    else if (victim == ch)
      send_to_char("That doesn't make much sense, does it?\r\n", ch);
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char("Go transfer someone your own size.\r\n", ch);
	return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, ch->in_room);
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_GRGOD)
    {
      send_to_char("I think not.\r\n", ch);
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch)
      {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, ch->in_room);
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
      }
    send_to_char(OK, ch);
  }
}

ACMD(do_teleport)
{
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char("Whom do you wish to teleport?\r\n", ch);
  else if (!(victim = generic_find_char(ch, buf, FIND_CHAR_WORLD)))
    send_to_char(NOPERSON, ch);
  else if (victim == ch)
    send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("Maybe you shouldn't do that.\r\n", ch);
  else if (!*buf2)
    send_to_char("Where do you wish to send this person?\r\n", ch);
  else if ((target = find_target_room(ch, buf2)) >= 0) {
    send_to_char(OK, ch);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
  }
}

ACMD(do_vnum)
{
  int i, number, found = FALSE;
  char larg1[MAX_INPUT_LENGTH], larg2[MAX_INPUT_LENGTH];
  char larg3[MAX_INPUT_LENGTH], larg4[MAX_INPUT_LENGTH];

  larg1[0] = '\0';
  larg2[0] = '\0';
  larg3[0] = '\0';
  larg4[0] = '\0';

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && 
                          !is_abbrev(buf, "obj") && !is_abbrev(buf, "list"))) {
    send_to_char("&1Usage: &4vnum [&2list&4] { obj &2[type]&4 | mob &2[type]&4 | &2room&4 } <name|&2zonenumber&4>\r\n", ch);
    return;
  }

  // DM: added list abilities
  if (is_abbrev(buf, "list")) {
    half_chop(buf2, larg1, larg2);

    if (!is_abbrev(larg1, "mob") && !is_abbrev(larg1, "obj") && 
                    !is_abbrev(larg1, "room")) {
      send_to_char("&1Usage: &4vnum list { obj [type] | mob [type] | room } <zonenumber>\r\n", ch);
      return;
    }

    // now we have: vnum list <mob|obj|room> (larg2 -> [type] zonenumber)
    half_chop(larg2, larg3, larg4);

    // so if larg1 is mob|obj, larg3 is either type of mob|obj or zonenumber
    
    // vnum list mob
    if (is_abbrev(larg1, "mob")) {
      for (i = 0; i < NUM_NPC_CLASSES; i++) {
        if (is_abbrev(larg3, npc_class_types[i])) {
          found = TRUE;
          break;
        }
      }
      
      if (found) {
        if (real_zone(atoi(larg4)) == NOWHERE) {
          send_to_char("Invalid virtual zone number.\r\n", ch);
          return;
        }
        list_mobiles(ch, atoi(larg4), i);
      } else {
        if (real_zone(atoi(larg3)) == NOWHERE) {
          send_to_char("Invalid mobile type or virtual zone number.\r\n", ch);
          return;
        }
        list_mobiles(ch, atoi(larg3), NOTHING);
      }

    // vnum list obj
    } else if (is_abbrev(larg1, "obj")) {
      for (i = 0; i < NUM_ITEM_TYPES; i++) {
        if (is_abbrev(larg3, item_types[i])) {
          found = TRUE;
          break;
        }
      }
      
      if (found) {
        if (real_zone(atoi(larg4)) == NOWHERE) {
          send_to_char("Invalid virtual zone number.\r\n", ch);
          return;
        }
        list_objects(ch, atoi(larg4), i);
      } else {
        if (real_zone(atoi(larg3)) == NOWHERE) {
          send_to_char("Invalid object type or virtual zone number.\r\n", ch);
          return;
        }
        list_objects(ch, atoi(larg3), NOTHING);
      }

    // vnum list room 
    } else {
      if (real_zone(atoi(larg3)) == NOWHERE) {
        send_to_char("Invalid virtual zone number.\r\n", ch);
        return;
      } 
      number = atoi(larg3);
      found = 0;
      buf[0] = '\0';
      for (i = 0; i < top_of_world; i++) {
        if (zone_table[world[i].zone].number == number) {
          sprintf(buf2, "%3d. &8[%5d] %s&n\r\n", ++found, 
                  world[i].number, world[i].name);
          strncat(buf, buf2, strlen(buf2));
        }
      }
      if (found) {
        page_string(ch->desc, buf, TRUE);
      }
    }
  }
  

  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
      send_to_char("No mobiles by that name.\r\n", ch);

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(buf2, ch))
      send_to_char("No objects by that name.\r\n", ch);
}



void do_stat_room(struct char_data * ch)
{
  struct extra_descr_data *desc;
  struct room_data *rm = &world[ch->in_room];
  int i, found;
  struct obj_data *j;
  struct char_data *k;
  char sectbase[20], sectatmos[20], secttemp[20], sectgrav[20], sectenviro[20];

  sprintf(buf, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
	  CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprinttype(BASE_SECT(rm->sector_type), sector_types[SECT_TYPE_BASE], sectbase);
  sprinttype(ATMOSPHERE(rm->sector_type), sector_types[SECT_TYPE_ATMOS], sectatmos);
  sprinttype(TEMPERATURE(rm->sector_type), sector_types[SECT_TYPE_TEMP], secttemp);
  sprinttype(GRAVITY(rm->sector_type), sector_types[SECT_TYPE_GRAV], sectgrav);
  sprinttype(ENVIRON(rm->sector_type), sector_types[SECT_TYPE_ENVIRO], sectenviro);

  sprintf(buf, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d]\r\n",
	  zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
	  CCNRM(ch, C_NRM), ch->in_room);
  send_to_char(buf, ch); 

  sprintf(buf, "&0Sectors&n:\r\n"
               "&1Base&n        : &c%s&n\r\n"
               "&1Atmosphere&n  : &c%s&n\r\n"
               "&1Temperature&n : &c%s&n\r\n"
               "&1Gravity&n     : &c%s&n\r\n" 
               "&1Environment&n : &c%s&n\r\n",
                  sectbase, sectatmos, secttemp, sectgrav, sectenviro);
  send_to_char(buf,ch);

  sprintbit(rm->room_flags, room_bits, buf2);
  sprintbit(rm->burgle_flags, burgle_rooms, buf1);
  sprintf(buf, "&0SpecProc&n: &c%s\r\n"
               "&0Flags&n: &c%s\r\n"
               "&0Burgle bits: &c%s\r\n",
	  (rm->func == NULL) ? "None" : "Exists", buf2, buf1);
  send_to_char(buf, ch);

  send_to_char("&0Description&n:\r\n", ch);
  if (rm->description)
    send_to_char(rm->description, ch);
  else
    send_to_char("  None.\r\n", ch);

  if (rm->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      if (desc->keyword)
	strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  sprintf(buf, "&0Chars present&n:%s", CCYEL(ch, C_NRM));
  for (found = 0, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;
    // Stupid - I know - cant be bothered rewriting it.
    if (!IS_NPC(k))
      sprintf(buf2, "%s &7%s&n(&7%s&n)", found++ ? "," : "", GET_NAME(k),
	    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
    else
      sprintf(buf2, "%s &6%s&n(&6%s&n)", found++ ? "," : "", GET_NAME(k),
	    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));

    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (k->next_in_room)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);
  send_to_char(CCNRM(ch, C_NRM), ch);

  if (rm->contents) {
    sprintf(buf, "&0Contents&n:%s", CCGRN(ch, C_NRM));
    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;
      sprintf(buf2, "%s &5%s&n", found++ ? "," : "", j->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (rm->dir_option[i]) {
      if (rm->dir_option[i]->to_room == NOWHERE)
	sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
      else
	sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
		GET_ROOM_VNUM(rm->dir_option[i]->to_room), CCNRM(ch, C_NRM));
      sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
      sprintf(buf, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n ",
	      CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1, rm->dir_option[i]->key,
	   rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None",
	      buf2);
      send_to_char(buf, ch);
      if (rm->dir_option[i]->general_description)
	strcpy(buf, rm->dir_option[i]->general_description);
      else
	strcpy(buf, "  No exit description.\r\n");
      send_to_char(buf, ch);
    }
  }
  /* check the room for a script */
  do_sstat_room(ch);
}

// DM - TODO check if we need to add other objects ....
// checked - double check before release
void do_list_obj_values(struct obj_data *j, char * buf)
{
  switch (GET_OBJ_TYPE(j)) {

  case ITEM_LIGHT:
    sprintf(buf, "Color (unused): [&c%d&n], Type (unused): [&c%d&n], Hours: [&c%d&n]",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    sprintf(buf, "Spells Level: &c%d&n, Spells: &g%s&n (&c%d&n), &g%s&n (&c%d&n), &g%s&n (&c%d&n)",
		GET_OBJ_VAL(j, 0),
                skill_name(GET_OBJ_VAL(j, 1)), GET_OBJ_VAL(j, 1), 
		skill_name(GET_OBJ_VAL(j, 2)), GET_OBJ_VAL(j, 2), 
		skill_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprintf(buf, "Spell Level: &g%d&n, Max Charges: &c%d&n, Rem Charges: &c%d&n, Spell: &g%s&n (&c%d&n)", 
                GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), 
                skill_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_FIREWEAPON:
  case ITEM_WEAPON:
    sprintf(buf, "Tohit: &c%d&n, Todam: &r%d&nd&r%d&n, Type: &c%d&n", 
	GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), 
	GET_OBJ_VAL(j, 3));
    if (IS_OBJ_STAT(j, ITEM_MAGIC)) {
      sprintf(buf, "%s\r\nCasts: &g%s&n", buf, 
	  spell_info[GET_OBJ_VAL(j, 0)].name);
    }
    break;
  case ITEM_MISSILE:
    sprintf(buf, "Tohit: &c%d&n, Todam: &c%d&n, Type: &c%d&n", GET_OBJ_VAL(j, 0),
            GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_ARMOR:
    sprintf(buf, "AC-apply: [&c%d&n]", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_TRAP:
    sprintf(buf, "Loaded: &g%s&n, Damage: &r%d&nd&r%d&n, Protecting Item: &c%d&n",
      (GET_OBJ_VAL(j, 0) == 0 ? "No" : (GET_OBJ_VAL(j, 0) == 1 ? "Yes" : "Yes, Room")),
      GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break; 
  case ITEM_CONTAINER:
    if (!IS_CORPSE(j)) {
      sprintf(buf, "Max-contains: &c%d&n, Locktype: &c%d&n",
              GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    } else {
      sprintf(buf, "Corpse idnum (pc idnum): &c%d&n",
              GET_CORPSEID(j));        
    }
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN: 
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
    sprintf(buf, "Max-contains: &c%d&n, Contains: &c%d&n, Poisoned: &g%s&n, Liquid: &g%s&n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No", buf2);
    break;
  case ITEM_NOTE:
    sprintf(buf, "Tongue (unused): &c%d&n", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    sprintf(buf, "Keytype: &c%d&n", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_FOOD:
    sprintf(buf, "Makes full: &c%d&n, Poisoned: &g%s&n",
            GET_OBJ_VAL(j, 0), (GET_OBJ_VAL(j, 3) == 0 ? "No" : "Yes"));
    break;
  case ITEM_MAGIC_EQ:
    sprintf(buf, "Spells: &g%s&n (&c%d&n), &g%s&n (&c%d&n), &g%s&n (&c%d&n), Charge: &c%d&n",
		skill_name(GET_OBJ_VAL(j,0)), GET_OBJ_VAL(j, 0),
                skill_name(GET_OBJ_VAL(j,1)), GET_OBJ_VAL(j, 1), 
		skill_name(GET_OBJ_VAL(j, 2)), GET_OBJ_VAL(j, 2), 
                GET_OBJ_VAL(j,3));
    break;
  case ITEM_JOINABLE:
    sprintf(buf, "Joins with: &c%d&n, Makes: &c%d&n", GET_OBJ_VAL(j,0), GET_OBJ_VAL(j,3));
    break;
  case ITEM_BATTERY:
    sprintf(buf, "Battery Type: &g%s&n, Max Value: &c%d&n, Current Value: &c%d&n, Ratio: &c%d&n:&c1&n",
      battery_types[GET_OBJ_VAL(j,0)], GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_GATEWAY:
    sprintf(buf, "Teleports to: &c%d&n, Min Level: &c%d&n, Max Level: &c%d&n, Entry Fee: &c%d&n.",
      GET_OBJ_VAL(j,0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
      GET_OBJ_VAL(j,3));
    break;
  default:
    sprintf(buf, "Values 0-3: [&c%d&n] [&c%d&n] [&c%d&n] [&c%d&n]",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  }
} 

void do_stat_object(struct char_data * ch, struct obj_data * j)
{
  char lr_buf[9];
  int i, found;
  obj_vnum vnum;
  struct obj_data *j2;
  struct extra_descr_data *desc;
  struct timer_type *timer;

  vnum = GET_OBJ_VNUM(j);
  sprintf(buf, "Name: '%s%s%s', Aliases: %s\r\n", CCYEL(ch, C_NRM),
	  ((j->short_description) ? j->short_description : "<None>"),
	  CCNRM(ch, C_NRM), j->name);
  send_to_char(buf, ch);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  if (GET_OBJ_RNUM(j) >= 0)
    strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None"));
  else
    strcpy(buf2, "None");
  sprintf(buf, "VNum: [%s%5d%s], RNum: [%5d], Type: %s, SpecProc: %s, Damage: &r%d&n/&r%d&n\r\n",
   CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), buf1, buf2,
   GET_OBJ_DAMAGE(j), GET_OBJ_MAX_DAMAGE(j));
  send_to_char(buf, ch);
  sprintf(buf, "L-Des: %s\r\n", ((j->description) ? j->description : "None"));
  send_to_char(buf, ch);

  if (j->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = j->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      if (desc->keyword)
	strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  send_to_char("Can be worn on: ", ch);
  sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Set char bits : ", ch);
  sprintbit(j->obj_flags.bitvector, affected_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Extra flags   : ", ch);
  sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
  sprintf(lr_buf, " LR_%d", (int)GET_OBJ_LR(j));
  strcat(buf, lr_buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  if (OBJ_FLAGGED(j, ITEM_RIDDEN)) {
	sprintf(buf, "Ridden by: %s\r\n", (OBJ_RIDDEN(j) ? GET_NAME(OBJ_RIDDEN(j)) : "Noone"));
	send_to_char(buf, ch);
  }

//  sprintf(buf, "Level Restrict: %d\r\n", (int)GET_OBJ_LR(j) );
//  send_to_char(buf, ch);

  sprintf(buf, "Weight: %d, Value: &Y%d&n, Cost/day: &Y%d&n, Timer: %d\r\n",
     GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j));
  send_to_char(buf, ch);

  strcpy(buf, "In room: ");
  if (j->in_room == NOWHERE)
    strcat(buf, "Nowhere");
  else {
    sprintf(buf2, "%d", GET_ROOM_VNUM(IN_ROOM(j)));
    strcat(buf, buf2);
  }
  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  strcat(buf, ", In object: ");
  strcat(buf, j->in_obj ? j->in_obj->short_description : "None");
  strcat(buf, ", Carried by: ");
  strcat(buf, j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  strcat(buf, ", Worn by: ");
  strcat(buf, j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  do_list_obj_values(j,buf);
  strcat(buf,"\r\n");
  send_to_char(buf, ch);

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains) {
    sprintf(buf, "\r\nContents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j2->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  found = 0;
  send_to_char("Affections:", ch);
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      found = 1;
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s %+d to %s", found++ ? "," : "",
	      j->affected[i].modifier, buf2);
      send_to_char(buf, ch);
    }

  if (!found)
    send_to_char(" None", ch);

  send_to_char("\r\n", ch);

  found = 0;
  /* Routine to show what timers an item is affected by */
  send_to_char("Timers:", ch);
  if (j->timers) {
    for (timer = j->timers; timer; timer = timer->next) {
      found = 1;
      sprintf(buf,"\r\nTIM: (%3dhr) %s%-22s%s", timer->duration, 
	  CCCYN(ch, C_NRM), timer_types[timer->type], CCNRM(ch, C_NRM));
      sprintf(buf2,"Uses: (%2d) of max: (%2d)\r\n",timer->uses,timer->max_uses);
      strcat(buf, buf2);
      send_to_char(buf,ch);
    } 
  }

  if (!found)
    send_to_char(" None", ch);



  send_to_char("\r\nMaterials :", ch);
  if (j->materials == NULL) {
    send_to_char(" None", ch);
  } else {
    list<ObjMaterialClass>::iterator itr;
    itr = j->materials->begin();
    strcpy(buf2, "");
    char buf3[255];
    while (itr != j->materials->end()) {
      char *oname = obj_proto[real_object((*itr).vnum)].name;
      sprintf(buf3, " (%s :%d x %d) ",  oname, (*itr).vnum, (*itr).number);
      strcat(buf2, buf3);
      itr++;
    }
    send_to_char(buf2, ch);
  }
  send_to_char("\r\nMakes :", ch);
  if (j->products == NULL) {
    send_to_char(" None", ch);
  } else {
    list<ObjProductClass>::iterator itr;
    itr = j->products->begin();
    strcpy(buf2, "");
    char buf3[255];
    while (itr != j->products->end()) {
      char *oname = obj_proto[real_object((*itr).vnum)].name;
      sprintf(buf3, " (%s :%d  S:%d L:%d) ",  oname, (*itr).vnum, (*itr).skill, (*itr).level);
      strcat(buf2, buf3);
      itr++;
    }
    send_to_char(buf2, ch);
  }
  
    

  send_to_char("\r\n", ch);

  /* check the object for a script */
  do_sstat_object(ch, j);
}


void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;
  struct timer_type *timer;
  char buf3[MAX_STRING_LENGTH];
  int ETERNAL;
#ifdef NO_LOCALTIME
  struct tm lt;
#endif
  struct assisters_type *assisters; 

  ETERNAL = (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_ETRNL9);   

  sprinttype(GET_SEX(k), genders, buf);
  sprintf(buf2, " %s '%s'  IDNum: [%5ld], In room [%5d]",
	  (!IS_NPC(k) ? "PC"  :
	  (!IS_MOB(k) ? "NPC" : "MOB")),
	  GET_NAME(k), GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)));

  if (!IS_NPC(k))
  {
    if (GET_INVIS_TYPE(k) == INVIS_SPECIFIC)
      sprintf(buf3,", Invis to [%s%s%s]", CCYEL(ch, C_NRM),
	      get_name_by_id(GET_INVIS_LEV(k)), CCNRM(ch,C_NRM));
    else if (GET_INVIS_TYPE(k) == INVIS_SINGLE)
      sprintf(buf3,", Invis to Lvl [%s%ds%s]", CCYEL(ch,C_NRM),
	      GET_INVIS_LEV(k), CCNRM(ch,C_NRM));
    else if (GET_INVIS_TYPE(k) == INVIS_NORMAL)
      sprintf(buf3,", Invis Lvl [%s%d%s]", CCYEL(ch,C_NRM),
	      GET_INVIS_LEV(k), CCNRM(ch,C_NRM));
    else
      sprintf(buf3,", Invis to Lvls [%s%d-%d%s]",CCYEL(ch, C_NRM),
	      GET_INVIS_LEV(k), GET_INVIS_TYPE(ch), CCNRM(ch,C_NRM));
    strcat(buf2,buf3);
  }

  strcat(buf2,"\r\n");
  send_to_char(strcat(buf, buf2), ch);
  if (IS_MOB(k))
  {
    sprintf(buf, "Alias: %s, VNum: [%5d], RNum: [%5d]\r\n",
	    k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    send_to_char(buf, ch);
  }
  sprintf(buf, "Title: %s\r\n", (k->player.title ? k->player.title : "<None>"));
  send_to_char(buf, ch);
  sprintf(buf, "L-Des: %s", (k->player.long_descr ? k->player.long_descr : 
	                    "<None>\r\n"));
  send_to_char(buf, ch);

  if (IS_NPC(k))
  {	/* Use GET_CLASS() macro? */
    sprinttype(GET_CLASS(k), npc_class_types, buf2);
    sprintf(buf, "&nMonster Class: &B%s&n\r\n", buf2);
  } else {
    sprinttype(GET_CLASS(k), pc_class_types, buf2);
    sprinttype(GET_RACE(k), pc_race_types, buf3); 
    sprintf(buf, "Class: &B%s&n, Race: &B%s&n, Exp Modifier: [&R%.3f&n]\r\n", buf2, buf3,GET_MODIFIER(k));
  }
  if (IS_NPC(k))
  {
    sprintf(buf2, "Lev: [%s%2d%s], XP: [%s%8d%s], Align: [%4d]\r\n",
	    CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	    CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
	    GET_ALIGNMENT(k));
  } else {
    if (CAN_LEVEL(k))
    {
      sprintf(buf2, "Lev: [%s%3d%s], XP: [%s%8d%s], XP_NL: [%s%8d%s], Align: [%4d]\r\n",
	      CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	      CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
	      level_exp(ch,GET_LEVEL(k))-GET_EXP(k), CCNRM(ch, C_NRM),
	      GET_ALIGNMENT(k));
    } else {
      sprintf(buf2, "Lev: [%s%3d%s], XP: [%s%8d%s], Align: [%4d]\r\n",
          CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
          CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
          GET_ALIGNMENT(k));
    }
  } 
  strcat(buf, buf2);
  send_to_char(buf, ch);
  if (!IS_NPC(k))
  {
#ifndef NO_LOCALTIME
    strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
    strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
#else
    if (jk_localtime(&lt, k->player.time.birth))
    {
      basic_mud_log("Error in jk_localtime (birth: %d) [%s:%d]\n",
	            k->player.time.birth, __FILE__, __LINE__);
      strcpy(buf1, "ERROR!");
    } else {
      strcpy(buf1, (char *)asctime(&lt));
    }
    if (jk_localtime(&lt, k->player.time.logon))
    {
      basic_mud_log("Error in jk_localtime (birth: %d) [%s:%d]\n",
	            k->player.time.logon, __FILE__, __LINE__);
      strcpy(buf2, "ERROR!");
    } else {
      strcpy(buf2, (char *)asctime(&lt));
    }
#endif
    buf1[10] = buf2[10] = '\0';
    sprintf(buf, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
	    buf1, buf2, k->player.time.played / 3600,
	    ((k->player.time.played % 3600) / 60), age(k)->year);
    send_to_char(buf, ch);
    sprintf(buf, "Hometown: [%d], Speaks: [%d/%d/%d], (STL[%d]/per[%d]/NSTL[%d])",          k->player.hometown, GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2),
            GET_PRACTICES(k), int_app[GET_INT(k)].learn,
            wis_app[GET_WIS(k)].bonus);

    /*. Display OLC zone for immorts .*/
    if (GET_LEVEL(k) >= LVL_CHAMP)
      sprintf(buf + strlen(buf), ", OLC[%d]", GET_OLC_ZONE(k));
    strcat(buf, "\r\n");

    send_to_char(buf, ch);
    
    sprintf(buf,"Start Rooms: [%d, %d, %d], Stat Points: [%4d]\r\n",
        ENTRY_ROOM(k,WORLD_MEDIEVAL), ENTRY_ROOM(k,WORLD_WEST), 
        ENTRY_ROOM(k, WORLD_FUTURE), GET_STAT_POINTS(k));
    send_to_char(buf,ch); 
  }
  sprintf(buf, "Str: &C%d/%d&n(&c%d/%d&n) Int: &C%d&n(&c%d&n) Wis: &C%d&n(&c%d&n) Dex: &C%d&n(&c%d&n) Con: &C%d&n(&c%d&n) Cha: &C%d&n(&c%d&n)\r\n",
	  GET_AFF_STR(k), GET_AFF_ADD(k), GET_REAL_STR(k), GET_REAL_ADD(k), 
	  GET_AFF_INT(k), GET_REAL_INT(k), GET_AFF_WIS(k), GET_REAL_WIS(k), 
	  GET_AFF_DEX(k), GET_REAL_DEX(k), GET_AFF_CON(k), GET_REAL_CON(k), 
	  GET_AFF_CHA(k), GET_REAL_CHA(k));
  send_to_char(buf, ch);

  if (!ETERNAL)
  {
    sprintf(buf, "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
	    CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
	    CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k),
	    mana_gain(k), CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k),
	    GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprintf(buf, "Coins: [&Y%9d&n], Bank: [&Y%9d&n] (Total: &Y%d&n)\r\n",
	    GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));
    send_to_char(buf, ch);

    sprintf(buf, "AC: [&c%d&n], Thac0: [&c%d&n] Hitroll: [&r%2d&n], Damroll: [&r%2d&n], Saving throws: [%d/%d/%d/%d/%d]\r\n",
	    compute_armor_class(k, 0), thaco(k, NULL),
	    k->points.hitroll, k->points.damroll, saving_throws(k, 0),
	    saving_throws(k, 1), saving_throws(k, 2), saving_throws(k, 3),
	    saving_throws(k, 4));
    send_to_char(buf, ch);
  }

  if (!IS_NPC(k))
  {
    sprintf(buf, "Social Points: %ld, Social Rank: %s\r\n", 
	    GET_SOCIAL_POINTS(k), social_ranks[GET_SOCIAL_STATUS(k)]);
    send_to_char(buf, ch);
  }
  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "Pos: %s, Fighting: %s", buf2,
	  (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));
  if (MOUNTING(k))
  {
    sprintf(buf2, ", Mounted on: %s", 
	    (IS_NPC(k) ? "Yes" : GET_NAME(MOUNTING(k))));
    strcat(buf, buf2);
  }
  if (MOUNTING_OBJ(k))
  {
    sprintf(buf2, ", Mounted on: %s", MOUNTING_OBJ(k)->short_description);
    strcat(buf, buf2);
  }
  if (IS_NPC(k))
  {
    strcat(buf, ", Attack type: ");
    strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
  }
  if (k->desc)
  {
    sprinttype(STATE(k->desc), connected_types, buf2);
    strcat(buf, ", Connected: ");
    strcat(buf, buf2);
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  strcpy(buf, "Default position: ");
  sprinttype((k->mob_specials.default_pos), position_types, buf2);
  strcat(buf, buf2);

  sprintf(buf2, ", Idle Timer (in tics) [%d]\r\n", k->char_specials.timer);
  strcat(buf, buf2);
  send_to_char(buf, ch);
  if (IS_NPC(k))
  {
    sprintbit(MOB_FLAGS(k), action_bits, buf2);
    sprintf(buf, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, 
	    CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
  } else {
    sprintbit(PLR_FLAGS(k), player_bits, buf2);
    sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit(PRF_FLAGS(k), preference_bits, buf2);
    sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit(EXT_FLAGS(k), extended_bits, buf2);
    sprintf(buf, "EXT: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit(GET_SPECIALS(k), special_ability_bits, buf2);
    sprintf(buf, "ABL: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    /* Punishment Bits - ARTUS */
    i2 = 0;
    sprintf(buf, "PUN:");
    for (i = 0; i < NUM_PUNISHES; i++)
      if (PUN_FLAGGED(k, i))
      {
        sprintf(buf, "%s %s[%d]", buf, punish_types[i], PUN_HOURS(k, i));
        i2 = 1;
      }

    if (i2 == 0)
      strcat(buf, " None.");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    /* Offence Counts - ARTUS */
    i2 = 0;
    sprintf(buf, "OFF:");
    for (i = 0; i < NUM_OFFENCES; i++)
      if (HAS_OFFENDED(k,i) > 0)
      {
        sprintf(buf, "%s %s[&g%d&n]", buf, offence_types[i], HAS_OFFENDED(k,i));
        i2 = 1;
      }
    if (i2 == 0)
      strcat(buf, " None.");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    /* Clan Rank, Clan Name - ARTUS */
    sprintf(buf, "Clan:&g %d&n (&g%s&n) Rank: &g%d&n of&g %d&n\r\n",
	    GET_CLAN(k), 
	    ((GET_CLAN(k) > 0) ? clan[find_clan_by_id(GET_CLAN(k))].name :
	     "none"), GET_CLAN_RANK(k), 
	    ((GET_CLAN(k) > 0) ? clan[find_clan_by_id(GET_CLAN(k))].ranks : 0));
    send_to_char(buf, ch);

    /* Remort Levels - ARTUS */
    sprintf(buf, "Remort Levels:  One[&g%3d&n] Two[&g%3d&n] Max[&g%3d&n] Total[&g%3d&n]\r\n", GET_REM_ONE(k), GET_REM_TWO(k), GET_MAX_LVL(k), 
	      (GET_REM_ONE(k) + GET_REM_TWO(k) + GET_LEVEL(k)));
    send_to_char(buf, ch);

    /* Kill Counts - Artus */ // ... You want to get stats off the target, Artus
    sprintf(buf, "Kills: Imm[&g%ld&n] By Imm[&g%ld&n] Mob[&g%ld&n] By Mob[&g%ld&n] PC[&g%ld&n] By PC[&g%ld&n]; Unholiness: %d\r\n", GET_IMMKILLS(k), 
	    GET_KILLSBYIMM(k), GET_MOBKILLS(k), GET_KILLSBYMOB(k),
	    GET_PCKILLS(k), GET_KILLSBYPC(k), GET_UNHOLINESS(k));
    send_to_char(buf, ch);

/*    sprintf(buf, "TIMERS: [");
    for (i=0; i < MAX_TIMERS; i++) {
      if (i != (MAX_TIMERS - 1))
        sprintf(buf2," %d,", TIMER(k,i));
      else
        sprintf(buf2," %d]\r\n", TIMER(k,i));
      strcat(buf,buf2);
    } 
    send_to_char(buf, ch); */
   
  }
  if (!IS_NPC(k) && IS_SET(GET_SPECIALS(k), SPECIAL_DISGUISE))
  {
    sprintf(buf, "Mob vnum memorised: %ld, Disguised: %s.\r\n",
	    CHAR_MEMORISED(k), CHAR_DISGUISED(k) == 0 ? "No" : "Yes");
    send_to_char(buf, ch);
  }
  if (IS_MOB(k))
  {
    sprintf(buf, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
	    (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice);
    send_to_char(buf, ch);
  }
  sprintf(buf, "Carried: weight: %d, items: %d; ",
	  IS_CARRYING_W(k), IS_CARRYING_N(k));

  for (i=0, j=k->carrying; j; j=j->next_content, i++);
  sprintf(buf + strlen(buf), "Items in: inventory: %d, ", i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  sprintf(buf2, "eq: %d\r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k))
  {
    sprintf(buf, "Hunger: %d, Thirst: %d, Drunk: %d\r\n",
	    GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
    send_to_char(buf, ch);
  }

  sprintf(buf, "Master is: %s, Followers are:",
	  ((k->master) ? GET_NAME(k->master) : "<none>"));
  for (fol = k->followers; fol; fol = fol->next)
  {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62)
    {
      if (fol->next)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);

  sprintf(buf, "Autoassisting: %s, Autoassisters are:",
          (AUTOASSIST(k) ? GET_NAME(AUTOASSIST(k)) : "<none>"));
  found=0;
  for (assisters=k->autoassisters; assisters; assisters=assisters->next)
  {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(assisters->assister, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62)
    {
      if (assisters->next)
        send_to_char(strcat(buf, ",\r\n"), ch);
      else
        send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }
 
  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);

  /* Showing the bitvector */
  sprintbit(AFF_FLAGS(k), affected_bits, buf2);
  sprintf(buf, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  /* Routine to show what spells a char is affected by */
  if (k->affected)
  {
    for (aff = k->affected; aff; aff = aff->next)
    {
      *buf2 = '\0';
      if(aff->duration == CLASS_ABILITY)
      {
        sprintf(buf, "ABL: (Unlim) &c%-21s &n ", skill_name(aff->type));
        if (aff->modifier)
	{
          sprintf(buf2, "%+d to %s", aff->modifier, 
	          apply_types[(int)aff->location]);
          strcat(buf, buf2);
        }
      } else if (aff->duration == CLASS_ITEM) {
	sprintf(buf, "OBJ: (Unlim) &c%-21s &n ", skill_name(aff->type));
	if (aff->modifier)
	{
	  sprintf(buf2, "%+d to %s", aff->modifier, 
	          apply_types[(int)aff->location]);
	  strcat(buf, buf2);
	}
      } else {
        // ROD - here make Unlim for perm spells from eq
        // It appears the affect is removed when time is 1,
        // spell affects from magic eq are given with time 0
        // for abilities they are given time -1
 
        // I hate this dodgy code of vaders ....
        // ok go through the eq list and find if the affect is given by eq

        // This code is DUPLICATED in act.informative.c for affects
	/* Artus> This is now redundant.
        bool found = FALSE;
        for (int i = 0; i < NUM_WEARS; i++) {
          if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_MAGIC_EQ) {
            for (int j = 0; j < 3; j++) {
              if (GET_OBJ_VAL(GET_EQ(ch, i), j) == aff->type) {
                found = TRUE;
                break;
              }
            }
          }
        }         

        if (found) {
          sprintf(buf, "SPL: (Unlim) %s%-21s%s ", CCCYN(ch, C_NRM),
              skill_name(aff->type), CCNRM(ch, C_NRM));
        } else { */
          sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1,
              CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));
        // }    
        if (aff->modifier)
	{
          sprintf(buf2, "%+d to %s", aff->modifier, 
	          apply_types[(int) aff->location]);
          strcat(buf, buf2);
        }
      }      
      if (aff->bitvector)
      {
	if (*buf2)
	  strcat(buf, ", sets ");
	else
	  strcat(buf, "sets ");
	sprintbit(aff->bitvector, affected_bits, buf2);
	strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }

  /* Routine to show what timers a char is affected by */
  if (k->timers)
  {
    for (timer = k->timers; timer; timer = timer->next)
    {

      sprintf(buf, "TIM: (%3dhr) %s%-22s%s", timer->duration, CCCYN(ch, C_NRM),
	      timer_types[timer->type], CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf), "Uses: (%2d) of max: (%2d)\r\n", timer->uses,
	      timer->max_uses);
      send_to_char(buf,ch);
    } 
  }

  /* check mobiles for a script */
  if (IS_NPC(k))
  {
    do_sstat_character(ch, k);
    if (SCRIPT_MEM(k))
    {
      struct script_memory *mem = SCRIPT_MEM(k);
      send_to_char("Script memory:\r\n  Remember             Command\r\n", ch);
      while (mem)
      {
        struct char_data *mc = find_char(mem->id);
        if (!mc)
	  send_to_char("  ** Corrupted!\r\n", ch);
        else {
          if (mem->cmd)
	    sprintf(buf, "  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
          else
	    sprintf(buf,"  %-20.20s <default>\r\n",GET_NAME(mc));
          send_to_char(buf, ch);
        }
        mem = mem->next;
      }
    }
  } else {
    /* this is a PC, display their global variables */
    if (k->script && k->script->global_vars)
    {
      struct trig_var_data *tv;
      char name[MAX_INPUT_LENGTH];
      void find_uid_name(char *uid, char *name);

      send_to_char("Global Variables:\r\n", ch);

      /* currently, variable context for players is always 0, so it is */
      /* not displayed here. in the future, this might change */
      for (tv = k->script->global_vars; tv; tv = tv->next)
      {
        if (*(tv->value) == UID_CHAR)
	{
          find_uid_name(tv->value, name);
          sprintf(buf, "    %10s:  [UID]: %s\r\n", tv->name, name);
        } else
          sprintf(buf, "    %10s:  %s\r\n", tv->name, tv->value);

        send_to_char(buf, ch);
      }
    }
  }
}


ACMD(do_stat)
{
  struct char_data *victim;
  struct obj_data *object;
  struct char_file_u tmp_store;
  int ETERNAL, tmp;

  if (IS_NPC(ch))
    return;

  half_chop(argument, buf1, buf2);

  ETERNAL = (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_ETRNL9);
  if (!*buf1)
  {
    send_to_char("Stats on who or what?\r\n", ch);
    return;
  } 
  if(ETERNAL)
  {
    if((victim = generic_find_char(ch,buf1,FIND_CHAR_ROOM)))
      do_stat_character(ch,victim);
    else 
      send_to_char("That person does not appear to be here.\r\n",ch);
    return;
  } 
  if (is_abbrev(buf1, "room"))
  {
    do_stat_room(ch);
    return;
  } 
  if (is_abbrev(buf1, "mob"))
  {
    if (!*buf2)
      send_to_char("Stats on which mobile?\r\n", ch);
    else
    {
      if ((victim = generic_find_char(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char("No such mobile around.\r\n", ch);
    }
    return;
  } 
  if (is_abbrev(buf1, "player"))
  {
    if (!*buf2)
      send_to_char("Stats on which player?\r\n", ch);
    else
    {
      if ((victim = get_player_online(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char("No such player around.\r\n", ch);
    }
    return;
  }
  if (is_abbrev(buf1, "file"))
  {
    if (!*buf2)
      send_to_char("Stats on which player?\r\n", ch);
    else
    {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1) {
	store_to_char(&tmp_store, victim);
	victim->player.time.logon = tmp_store.player_specials_primalsaved.last_logon;
	char_to_room(victim, 0);
	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char("Sorry, you can't do that.\r\n", ch);
	else
	  do_stat_character(ch, victim);
	extract_char(victim);
      } else {
	send_to_char("There is no such player.\r\n", ch);
	free(victim);
      }
    }
    return;
  } 
  if (is_abbrev(buf1, "object"))
  {
    if (!*buf2)
      send_to_char("Stats on which object?\r\n", ch);
    else
    {
      if ((object = generic_find_obj(ch, buf2, FIND_OBJ_WORLD)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char("No such object around.\r\n", ch);
    }
    return;
  }
  // Artus> Everything else.
  tmp = generic_find(buf1, FIND_CHAR_ROOM | FIND_CHAR_WORLD | FIND_OBJ_INV |
                           FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_WORLD,
		     ch, &victim, &object);
  switch (tmp)
  {
    case FIND_CHAR_ROOM:
    case FIND_CHAR_WORLD:
      do_stat_character(ch, victim);
      break;
    case FIND_OBJ_EQUIP:
    case FIND_OBJ_ROOM:
    case FIND_OBJ_WORLD:
    case FIND_OBJ_INV:
      do_stat_object(ch, object);
      break;
    default:
      send_to_char("Nothing around by that name.\r\n", ch);
  }
  return;
}


ACMD(do_shutdown)
{
  char *confirm = buf;

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("If you want to shut something down, say so!\r\n", ch);
    return;
  }

  confirm = one_argument(argument, arg);
  skip_spaces(&confirm);

  if (files_need_saving() && str_cmp(confirm, "really") ) {
    send_to_char("OLC files need saving.\r\n"
            "Use &4shutdown <option> really&n if you want to shutdown.\r\n",ch);
    return;
  }
  if (!*arg) {
    basic_mud_log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "now")) {
    basic_mud_log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in about 10 seconds. [Chars NOT Saved]\r\n");
    circle_shutdown = 1;
    circle_reboot = 2;
  } else if (!str_cmp(arg, "reboot")) {
    basic_mud_log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in about 10 seconds. [Chars Saved]\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    basic_mud_log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    basic_mud_log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else
    send_to_char("Unknown shutdown option.\r\n", ch);

  // DM - TODO - check if command below is needed (not in original circle code)
  House_save_all(); 
}


void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping)
    send_to_char("You aren't snooping anyone.\r\n", ch);
  else {
    send_to_char("You stop snooping.\r\n", ch);
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}


ACMD(do_snoop)
{
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = generic_find_char(ch, arg, FIND_CHAR_WORLD)))
    send_to_char("No such person around.\r\n", ch);
  else if (!victim->desc)
    send_to_char("There's no link.. nothing to snoop.\r\n", ch);
  else if (victim == ch)
    stop_snooping(ch);
  /* only IMPS can snoop players in private rooms BM 3/95 */
  else if (GET_LEVEL(ch) < LVL_IMPL && 
                  IS_SET(world[victim->in_room].room_flags, ROOM_HOUSE))
    send_to_char("That player is in a private room, sorry!\n",ch);
  else if (victim->desc->snoop_by)
    send_to_char("Busy already. \r\n", ch);
  else if (victim->desc->snooping == ch->desc)
    send_to_char("Don't be stupid.\r\n", ch);
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
      send_to_char("You can't.\r\n", ch);
      return;
    }
    send_to_char(OK, ch);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}



ACMD(do_switch)
{
  SPECIAL(shop_keeper);  
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char("You're already switched.\r\n", ch);
  else if (!*arg)
    send_to_char("Switch with who?\r\n", ch);
  else if (!(victim = generic_find_char(ch, arg, 
					FIND_CHAR_ROOM | FIND_CHAR_WORLD)))
    send_to_char("No such character.\r\n", ch);
  else if (ch == victim)
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
/*  // DM - stop char switching
  else if (!IS_NPC(victim)) 
    send_to_char("No you will not switch into playing characters!\r\n", ch); */
  // Artus - I think this does it better :o)
  else if ((GET_LEVEL(ch) < GET_LEVEL(victim)) || (!IS_NPC(victim) && GET_IDNUM(victim) <= 3))
    send_to_char("Their mind is too strong, you cannot takeover their body.\r\n", ch);
  else if (victim->desc)
    send_to_char("You can't do that, the body is already in use!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
    send_to_char("You are not godly enough to use that room!\r\n", ch);
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE)
		&& !House_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim))))
    send_to_char("That's private property -- no trespassing!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(victim) == shop_keeper))
        send_to_char("Switching into Shopkeepers in NOT ALLOWED!\r\n", ch);
  else {
    send_to_char(OK, ch);

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char("You return to your original body.\r\n", ch);

    /*
     * If someone switched into your original body, disconnect them.
     *   - JE 2/22/95
     *
     * Zmey: here we put someone switched in our body to disconnect state
     * but we must also NULL his pointer to our character, otherwise
     * close_socket() will damage our character's pointer to our descriptor
     * (which is assigned below in this function). 12/17/99
     */
    if (ch->desc->original->desc) {
      ch->desc->original->desc->character = NULL;
      STATE(ch->desc->original->desc) = CON_DISCONNECT;
    }

    /* Now our descriptor points to our original body. */
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    /* And our body's pointer to descriptor now points to our descriptor. */
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}



ACMD(do_load)
{
  struct char_data *mob;
  struct obj_data *obj;
  mob_vnum number;
  mob_rnum r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("&1Usage: &4load { obj | mob } <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    // DM - allow zone owners to load mobiles of their zone
    } else if (GET_LEVEL(ch) < LVL_GOD && (GET_OLC_ZONE(ch) != 0 &&
          mob_index[r_num].vznum != GET_OLC_ZONE(ch))) {
      send_to_char("Sorry, you can only load mobiles of your zone.\r\n", ch);
      return; 
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, ch->in_room);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    load_mtrigger(mob);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    // DM - allow zone owners to load objects of their zone
    } else if (GET_LEVEL(ch) < LVL_GOD && (GET_OLC_ZONE(ch) != 0 && 
          obj_index[r_num].vznum != GET_OLC_ZONE(ch))) {
      send_to_char("Sorry, you can only load objects of your zone.\r\n", ch);
      return; 
    }
    obj = read_object(r_num, REAL);
    if (load_into_inventory)
      obj_to_char(obj, ch, __FILE__, __LINE__);
    else
      obj_to_room(obj, ch->in_room);
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    load_otrigger(obj);
    load_otrigger(obj);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}

ACMD(do_vstat)
{
  struct char_data *mob;
  struct obj_data *obj;
  mob_vnum number;	/* or obj_vnum ... */
  mob_rnum r_num;	/* or obj_rnum ... */

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("&1Usage: &4vstat { obj | mob } <vnum>&n\r\n", ch);
    return;
  }

  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf)
  {			/* argument supplied. destroy single object
				 * or char */
    if ((vict = generic_find_char(ch, buf, FIND_CHAR_ROOM)) != NULL)
    {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict)))
      {
	send_to_char("Fuuuuuuuuu!\r\n", ch);
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict))
      {
	sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	if (vict->desc)
	{
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = find_obj_list(ch, buf, world[ch->in_room].contents)) != NULL) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("Nothing here by that name.\r\n", ch);
      return;
    }

    send_to_char(OK, ch);
  } else {			/* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

    for (vict = world[ch->in_room].people; vict; vict = next_v)
    {
      next_v = vict->next_in_room;
      if (IS_NPC(vict) && !MOUNTING(vict))
	extract_char(vict);
    }

    for (obj = world[ch->in_room].contents; obj; obj = next_o)
    {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}



const char *logtypes[] = {
  "off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog)
{
  int tp;

  one_argument(argument, arg);

  if (!*arg) {
    tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
    sprintf(buf, "Your syslog is currently %s.\r\n", logtypes[tp]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char("&1Usage: &4syslog { Off | Brief | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
  SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

  sprintf(buf, "Your syslog is now %s.\r\n", logtypes[tp]);
  send_to_char(buf, ch);
}



ACMD(do_advance)
{
  struct char_data *victim;
  char reasonString[MAX_INPUT_LENGTH];
  char *name = arg, *level = buf2, *reason = reasonString;
  int newlevel, oldlevel;

  reasonString[0] = '\0';
  reason = two_arguments(argument, name, level);

  if (*reason)
    reason++;

  sprintf(buf, "reason: '%s'\r\n", reason);
  send_to_char(buf, ch);

  if (*name)
  {
    if (!(victim = generic_find_char(ch, name, FIND_CHAR_ROOM | FIND_CHAR_WORLD)))
    {
      send_to_char("That player is not here.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Advance who?\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim))
  {
    send_to_char("Maybe that's not such a great idea.\r\n", ch);
    return;
  }
  if (IS_NPC(victim))
  {
    send_to_char("NO!  Not on NPC's.\r\n", ch);
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0)
  {
    send_to_char("That's not a level!\r\n", ch);
    return;
  }
  if (newlevel > LVL_IMPL)
  {
    sprintf(buf, "%d is the highest possible level.\r\n", LVL_IMPL);
    send_to_char(buf, ch);
    return;
  }
  if (newlevel > GET_LEVEL(ch))
  {
    send_to_char("Yeah, right.\r\n", ch);
    return;
  }
  if (newlevel == GET_LEVEL(victim))
  {
    send_to_char("They are already at that level.\r\n", ch);
    return;
  }
  if (!*reason || (*reason && (strlen(reason) < 10)))
  {
    send_to_char("Please supply a reason of no less than 10 characters.\r\n",
                    ch);
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim))
  {
    send_to_char("You have been demoted!!!. Serves you RIGHT!\r\n", victim);
    demote_level(victim, newlevel, reason);
    /* DM_exp */
    GET_EXP(victim)=0;
    return;
  } else {
    act("$n makes some strange gestures.\r\n"
	"A strange feeling comes upon you,\r\n"
	"Like a giant hand, light comes down\r\n"
	"from above, grabbing your body, that\r\n"
	"begins to pulse with colored lights\r\n"
	"from inside.\r\n\r\n"
	"Your head seems to be filled with demons\r\n"
	"from another plane as your body dissolves\r\n"
	"to the elements of time and space itself.\r\n"
	"Suddenly a silent explosion of light\r\n"
	"snaps you back to reality.\r\n\r\n"
	"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(OK, ch);

//  if (newlevel < oldlevel)
//    basic_mud_log("(GC) %s demoted %s from level %d to %d.",
//		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
//  else
//    basic_mud_log("(GC) %s has advanced %s to level %d (from %d)",
//		GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  gain_exp_regardless(victim, newlevel); // level_exp(victim, newlevel) - GET_EXP(victim)); 
  save_char(victim, NOWHERE);
}



ACMD(do_restore)
{
  struct char_data *vict;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to restore?\r\n", ch);
  else if (!(vict = generic_find_char(ch, buf, FIND_CHAR_ROOM | FIND_CHAR_WORLD)))
    send_to_char(NOPERSON, ch);
  else {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);
    
    if (!IS_NPC(vict)) {
      // Remove any 'bad' affects ... 
      if (affected_by_spell(vict, SPELL_POISON))
        mag_unaffects(0, ch, vict, SPELL_REMOVE_POISON, 0);         // poison
      if (affected_by_spell(vict, SPELL_PARALYZE))
        mag_unaffects(0, ch, vict, SPELL_REMOVE_PARA, 0);           // paralyze
      if (affected_by_spell(vict, SPELL_CURSE))
        mag_unaffects(0, ch, vict, SPELL_GREATER_REMOVE_CURSE, 0);  // curses
      if (IS_SET(AFF_FLAGS(vict), AFF_BLIND))
        mag_unaffects(0, ch, vict, SPELL_HEAL, 0);                  // blindness
      if (IS_GHOST(vict))
	REMOVE_BIT(EXT_FLAGS(vict), EXT_GHOST);

      if ((GET_LEVEL(ch) >= LVL_GRGOD) && (GET_LEVEL(vict) >= LVL_ANGEL)) {
        for (i = 1; i <= MAX_SKILLS; i++)
	  SET_SKILL(vict, i, 100);

        if (GET_LEVEL(vict) >= LVL_GRGOD) {
	  vict->real_abils.str_add = 100;
	  vict->real_abils.intel = 25;
	  vict->real_abils.wis = 25;
	  vict->real_abils.dex = 25;
	  vict->real_abils.str = 25;
	  vict->real_abils.con = 25;
	  vict->real_abils.cha = 25;
        }
        vict->aff_abils = vict->real_abils;
      }
    }
    update_pos(vict);
    send_to_char(OK, ch);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}


void perform_immort_vis(struct char_data *ch)
{
  struct descriptor_data *d;
 
  if (GET_INVIS_LEV(ch) == 0 && !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)) {
    send_to_char("You are already fully visible.\r\n", ch);
    return;
  }
 
  sprintf(buf, "You feel privilidged as %s materialises nearby.\r\n",
        GET_NAME(ch) );
 
  for( d = descriptor_list; d; d = d->next )
    if (d->character)
        if( (d->character->in_room == ch->in_room) && (d->character != ch))
           if( !CAN_SEE(d->character, ch) )
                send_to_char(buf, d->character);
 
  GET_INVIS_LEV(ch) = 0;
  GET_INVIS_TYPE(ch) = INVIS_NORMAL;
  send_to_char("You are now fully visible.\r\n", ch);
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;
 
  if (IS_NPC(ch))
    return;
 
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
 
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level) {
 
       // character specific invis?
       if( GET_INVIS_TYPE(ch) == INVIS_SPECIFIC ) {
          if( GET_IDNUM(tch) == GET_INVIS_LEV(ch)) {
            if( GET_LEVEL(ch) > GET_LEVEL(tch) )
               act("You blink and realise that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
            else {
                send_to_char("You can't go invis to that person!\r\n", ch);
                GET_INVIS_TYPE(ch) = 0;
                GET_INVIS_LEV(ch) = 0;
                return;
            }
          }
 
        }
        else
       // Standard invis?
       if( GET_INVIS_TYPE(ch) == INVIS_NORMAL )
          act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
       else {
          // Specific invis?
          if ( GET_INVIS_TYPE(ch) == INVIS_SINGLE && GET_LEVEL(tch) == level - 1 )
               act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
          else {
             // They are in a range invis
             if ( GET_LEVEL(tch) >= level && GET_LEVEL(tch) <= GET_INVIS_TYPE(ch) ) {
                  act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT); 
             }
          }
       }
    }
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
           act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,tch, TO_VICT);
 
  }
 
  if( GET_INVIS_TYPE(ch) == INVIS_SPECIFIC ) {
        sprintf(buf, "You are now invisible to %s.\r\n",
                get_name_by_id(GET_INVIS_LEV(ch)) );
        send_to_char(buf, ch);
        return;
  }
 
  GET_INVIS_LEV(ch) = level;
  if (GET_INVIS_TYPE(ch) == INVIS_SINGLE)
     sprintf(buf2, "s.");
  else if (GET_INVIS_TYPE(ch) == INVIS_NORMAL)
     sprintf(buf2, ".");
  else
    sprintf(buf2, " - %d.", GET_INVIS_TYPE(ch));
 
  sprintf(buf, "Your invisibility level is %d%s\r\n", level, buf2);
  send_to_char(buf, ch);
}     


ACMD(do_invis)
{
  int level, toplevel;
  long pid;     // player id
  struct char_file_u tmp;
  struct char_data *tch;
 
  if (IS_NPC(ch)) {
    send_to_char("You can't do that!\r\n", ch);
    return;
  }
 
  two_arguments(argument, arg, buf1);
 
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    // invis to a person?
    if( !isdigit(arg[0]) ) {
        if( (pid = get_id_by_name(arg)) < 0 ) {
          send_to_char("No such player exists.\r\n", ch);
          return;
        }
 
        if( pid == GET_IDNUM(ch) ) {
          send_to_char("You put your hands over your eyes and hide.\r\n",ch);
          return;
        }
        // Load player
        CREATE(tch, struct char_data, 1);
        load_char(arg, &tmp);
        clear_char(tch);
        store_to_char(&tmp, tch);
	tch->player.time.logon = tmp.player_specials_primalsaved.last_logon;
        char_to_room(tch, 0);
        if( GET_LEVEL(ch) <= GET_LEVEL(tch) ) {
            send_to_char("You can't go invisible to that person!\r\n",ch); 
            extract_char(tch);
            return;
        }
        extract_char(tch);
        GET_INVIS_LEV(ch) = get_id_by_name(arg);
        GET_INVIS_TYPE(ch) = INVIS_SPECIFIC;
        perform_immort_invis(ch, GET_LEVEL(ch));
        return;
    }
 
    level = atoi(arg);
 
    if ( level > GET_LEVEL(ch)  )
      send_to_char("You can't go invisible above your own level.\r\n", ch);
    else if (level < 1)
      perform_immort_vis(ch);
    else {
      // If there is a second argument, evaluate it
      if (*buf1) {
        // Are they specifying a range?
        if (isdigit(buf1[0])) {
          toplevel = atoi(buf1);
          if (toplevel <= level || toplevel >= GET_LEVEL(ch)) {
            send_to_char("The level range is invalid.\r\n", ch);
            return;
          } // Valid range?
          else {
            GET_INVIS_TYPE(ch) = toplevel;
          }
        /* Specified a particular invisibility level? */
        } else if (is_abbrev(buf1, "single")) {
            GET_INVIS_TYPE(ch) = INVIS_SINGLE;
        // Invalid second arg ...
        } else {
          send_to_char("Invalid 2nd argument. Expecting 'single' or "
              "<high range level>\r\n", ch);
          return;
        }
      }
      else // No second argument
         GET_INVIS_TYPE(ch) = INVIS_NORMAL;
 
      perform_immort_invis(ch, level); 
    }
  }
}   


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char("That must be a mistake...\r\n", ch);
  else {
    sprintf(buf, "%s\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next)
      if (STATE(pt) == CON_PLAYING && pt->character && pt->character != ch)
	send_to_char(buf, pt->character);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      send_to_char(buf, ch);
  }
}


ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
  case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
  case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
  default:    return;
  }

  skip_spaces(&argument);

  if (strlen(argument) > POOF_LENGTH) {
    sprintf(buf,"Poof length exceded %d char's, not changed\r\n", POOF_LENGTH);
    send_to_char(buf,ch);
    return;
  } 

  if (*msg)
    free(*msg);

  if (!*argument)
    *msg = NULL;
  else
    *msg = str_dup(argument);
/*  else
    switch (subcmd) {
       case SCMD_POOFIN:     strcpy(POOFIN(ch),argument);    break;
       case SCMD_POOFOUT:    strcpy(POOFOUT(ch),argument);   break;
       default:      return;
    } 
 */
//  save_char(ch, NOWHERE);
  send_to_char(OK, ch);
}



ACMD(do_dc)
{
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char("&4Usage: &1dc <user number>&n (type USERS for a list)\r\n", ch);
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char("No such connection.\r\n", ch);
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char("No such connection.\r\n", ch);
    else
      send_to_char("Umm.. maybe that's not such a good idea...\r\n", ch);
    return;
  }

  /* We used to just close the socket here using close_socket(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor. -je
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   *
   * For those unlucky souls who actually manage to get disconnected
   * by two different immortals in the same 1/10th of a second, we have
   * the below 'if' check. -gg 12/17/99
   */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    send_to_char("They're already being disconnected.\r\n", ch);
  else {
    /*
     * Remember that we can disconnect people not in the game and
     * that rather confuses the code when it expected there to be
     * a character context.
     */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
    send_to_char(buf, ch);
    basic_mud_log("(GC) Connection closed by %s.", GET_NAME(ch));
  }
}



ACMD(do_wizlock)
{
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char("Invalid wizlock value.\r\n", ch);
      return;
    }
    circle_restrict = value;
    when = "now";
  } else
    when = "currently";

  switch (circle_restrict) {
  case 0:
    sprintf(buf, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    sprintf(buf, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    sprintf(buf, "Only level %d and above may enter the game %s.\r\n",
	    circle_restrict, when);
    break;
  }
  send_to_char(buf, ch);
}


ACMD(do_date)
{
  char *tmstr;
  int d, h, m;
  time_t mytime;
#ifndef NO_LOCALTIME

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;
  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';
#else
  struct tm lt;
  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  if (jk_localtime(&lt, mytime))
  {
    send_to_char("Bug! Please report.\r\n", ch);
    basic_mud_log("Error in jk_localtime (mytime: %ld) [%s:%d]", mytime, __FILE__, __LINE__);
      return;
  }
  tmstr = asctime(&lt);
  tmstr[strlen(tmstr) - 1] = '\0';
#endif

  if (subcmd == SCMD_DATE)
    sprintf(buf, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
	    ((d == 1) ? "" : "s"), h, m);
  }

  send_to_char(buf, ch);
}



ACMD(do_last)
{
  struct char_file_u chdata;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("For whom do you wish to search?\r\n", ch);
    return;
  }
  if (load_char(arg, &chdata) < 0) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if ((chdata.level > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL)) {
    send_to_char("You are not sufficiently godly for that!\r\n", ch);
    return;
  }
  sprintf(buf, "[%5ld] [%3d %s] %-12s : %-18s : %s",
	  chdata.char_specials_saved.idnum, (int) chdata.level,
	  class_abbrevs[(int) chdata.chclass], chdata.name, 
          chdata.player_specials_primalsaved.host,
	  asctime(localtime(&chdata.player_specials_primalsaved.last_logon)));
//	  ctime(&chdata.player_specials_primalsaved.last_logon));
  send_to_char(buf, ch);
  sprintf(buf, "%17s%-12s : %-18s : %s",
          "", "Unsuccessful", 
          chdata.player_specials_primalsaved.lastUnsuccessfulHost, 
	  (chdata.player_specials_primalsaved.lastUnsuccessfulLogon < 1) ?  "None\r\n" : asctime(localtime(&chdata.player_specials_primalsaved.lastUnsuccessfulLogon)));
	  // ctime(&chdata.player_specials_primalsaved.lastUnsuccessfulLogon));
  send_to_char(buf, ch);
}

ACMD(do_laston)
{
  struct char_file_u chdata;
 
  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("For whom do you wish to search?\r\n", ch);
    return;
  }
  if (load_char(arg, &chdata) < 0) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if (((chdata.level >= LVL_IS_GOD) && (GET_LEVEL(ch) != LVL_IMPL)) ||
((chdata.level >= LVL_ISNOT_GOD) && (GET_LEVEL(ch) < LVL_ISNOT_GOD)))
  {
    send_to_char("Sorry that player has a CLASSIFIED account!.\r\n", ch);
    return;
  }
  sprintf(buf, "%s was last on: %s\r\n",chdata.name,
    asctime(localtime(&chdata.player_specials_primalsaved.last_logon)));
    //ctime(&chdata.player_specials_primalsaved.last_logon));
  send_to_char(buf, ch);
} 

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

#define CAN_FORCE_PC(ch, vict)	((GET_LEVEL(ch) >= LVL_GRGOD) && \
                                 (GET_LEVEL(ch) >= GET_LEVEL(vict)))
#define CAN_FORCE_NPC(ch, vict)	((GET_LEVEL(ch) >= LVL_GRGOD) || \
                                 ((GET_OLC_ZONE(ch) > 0) && \
				  (GET_MOB_VZNUM(vict) == GET_OLC_ZONE(ch)) && \
				  (zone_table[world[IN_ROOM(vict)].zone].number == GET_OLC_ZONE(ch))))
#define CAN_FORCE(ch, vict)	((!IS_NPC(vict) && CAN_FORCE_PC(ch, vict)) || \
                                 (IS_NPC(vict) && CAN_FORCE_NPC(ch, vict)))

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
  {
    send_to_char("Whom do you wish to force do what?\r\n", ch);
    return;
  }
  if ((GET_LEVEL(ch) < LVL_GRGOD) || 
      (str_cmp("all", arg) && str_cmp("room", arg))) 
  {
    if (!(vict = generic_find_char(ch, arg, FIND_CHAR_ROOM | FIND_CHAR_WORLD)))
    {
      send_to_char(NOPERSON, ch);
      return;
    } 
    if (vict == ch)
    {
      send_to_char("Your other personality doesn't listen to you.\r\n", ch);
      return;
    }
    if (!IS_NPC(vict)) 
    {
      if (!(vict->desc) || (STATE(vict->desc) != CON_PLAYING))
      {
	send_to_char("They are in no state to do much of anything.\r\n", ch);
	return;
      } else if (!CAN_FORCE_PC(ch, vict)) {
	send_to_char("You are not holy enough to force them!\r\n", ch);
	return;
      }
    } else if (!CAN_FORCE_NPC(ch, vict)) {
      send_to_char("You can only force mobs from your zone, in your zone.\r\n", ch);
      return;
    } // Can force pc/npc checks.
    send_to_char(OK, ch);
    act(buf1, TRUE, ch, NULL, vict, TO_VICT);
    sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
    command_interpreter(vict, to_force);
    return;
  } // Individual Victim Only...
  if (!str_cmp("room", arg)) 
  {
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!CAN_FORCE(ch, vict) || (vict == ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
    return;
  } // Room...
  if (!strcmp("all", arg))
  {
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
    for (i = descriptor_list; i; i = next_desc) 
    {
      next_desc = i->next;
      if (STATE(i) != CON_PLAYING || !(vict = i->character) || IS_NPC(vict) || (!CAN_FORCE_PC(ch, vict)) || (vict == ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } // All...
}


#define LVL_IMMNET LVL_ETRNL1

ACMD(do_wiznet)
{
  struct descriptor_data *d; 
  char emote = FALSE;
  char any = FALSE;
  int level = LVL_GOD;
  int LVL = LVL_GOD;
 
  if(subcmd == SCMD_IMMNET)
    level = LVL = LVL_IMMNET;
  if(subcmd == SCMD_ANGNET)
    level = LVL = LVL_ANGEL;
 
  skip_spaces(&argument);
  delete_doubledollar(argument);
 
  if (!*argument) {
    if(subcmd == SCMD_IMMNET) {
      send_to_char("&1Usage: &4immnet <text> | #<level> <text> | *<emotetext> |\r\n"
                   "       &4immnet @<level> *<emotetext> | imm @\r\n",ch);
    } else {
    send_to_char("Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n"
                 "       wiznet @<level> *<emotetext> | wiz @\r\n", ch);
    }
    return;
  }
  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), LVL);
      if (level > GET_MAX_LVL(ch)) {
        send_to_char("You can't wizline above your own level.\r\n", ch);
        return;
      }         
    } else if (emote)
      argument++;
    break;
  case '@':
    for (d = descriptor_list; d; d = d->next) {
     if (STATE(d) == CON_PLAYING && GET_LEVEL(d->character) >= LVL &&
          ((!PRF_FLAGGED(d->character, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
          (subcmd == SCMD_IMMNET && !PRF_FLAGGED(d->character,PRF_NOIMMNET))) &&
          (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL)) {
        if (!any) {
          strcpy(buf1, "Gods online:\r\n");
          any = TRUE;
        }
        sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
        if (PLR_FLAGGED(d->character, PLR_WRITING))
          strcat(buf1, " (Writing)\r\n");
        else if (PLR_FLAGGED(d->character, PLR_MAILING))
          strcat(buf1, " (Writing mail)\r\n");
        else if (PLR_FLAGGED(d->character, PLR_REPORTING))
          strcat(buf1, " (Reporting)\r\n");
        else
          strcat(buf1, "\r\n");   
      }
    }
    any = FALSE;
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) == CON_PLAYING && GET_LEVEL(d->character) >= LVL &&
        ((PRF_FLAGGED(d->character, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
          (subcmd == SCMD_IMMNET && PRF_FLAGGED(d->character,PRF_NOIMMNET))) &&
          CAN_SEE(ch, d->character)) {
        if (!any) {
          strcat(buf1, "Gods offline:\r\n");
          any = TRUE;
        }
        sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
      }
    }
    send_to_char(buf1, ch);
    return; 
    break;
  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if ((PRF_FLAGGED(ch, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
      (subcmd == SCMD_IMMNET && PRF_FLAGGED(ch, PRF_NOIMMNET))) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);
 
  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }
  if (level > LVL) {
    if(subcmd == SCMD_IMMNET) {
      sprintf(buf1, "(%d) %s> %s%s\r\n", level, GET_NAME(ch),
              emote ? "<--- " : "", argument);
      sprintf(buf2, "(%d) Someone> %s%s\r\n", level, emote ? "<--- " : "",
              argument);
    } else if(subcmd == SCMD_ANGNET) {
      sprintf(buf1, "(%d:%s) %s%s\r\n", level, GET_NAME(ch),
              emote ? "<--- " : "", argument);
      sprintf(buf2, "(%d:Someone) %s%s\r\n", level, emote ? "<--- " : "",
              argument);
    } else {
      sprintf(buf1, "<%d> %s: %s%s\r\n", level, GET_NAME(ch),
              emote ? "<--- " : "", argument);
      sprintf(buf2, "<%d> Someone: %s%s\r\n", level, emote ? "<--- " : "",
              argument);
      }
  } else {
    if(subcmd == SCMD_IMMNET) {
      sprintf(buf1, "%s> %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
              argument);
      sprintf(buf2, "Someone> %s%s\r\n", emote ? "<--- " : "", argument);
    } else if(subcmd == SCMD_ANGNET) {
      sprintf(buf1, "(%s) %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "", 
      argument);
      sprintf(buf2, "(Someone) %s%s\r\n", emote ? "<--- " : "", argument);
    } else {
      sprintf(buf1, "%s: %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
              argument);
      sprintf(buf2, "Someone: %s%s\r\n", emote ? "<--- " : "", argument);
      }
  }
 
  for (d = descriptor_list; d; d = d->next)
  {
    if ((STATE(d) == CON_PLAYING) && !LR_FAIL_MAX(d->character, level) &&
        ((!PRF_FLAGGED(d->character, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
        (subcmd == SCMD_IMMNET && !PRF_FLAGGED(d->character,PRF_NOIMMNET))) &&
        (!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING | PLR_REPORTING))
        && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      if(subcmd == SCMD_IMMNET)
        send_to_char(CCBMAG(d->character, C_NRM), d->character);
      else if(subcmd == SCMD_ANGNET)
        send_to_char(CCCYN(d->character, C_NRM), d->character);
      else
        send_to_char(CCBCYN(d->character, C_NRM), d->character);
      if (CAN_SEE(d->character, ch))
        send_to_char(buf1, d->character);
      else
        send_to_char(buf2, d->character);
      send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }
 
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
}



ACMD(do_zreset)
{
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("You must specify a zone.\r\n", ch);
    return;
  }
  if (*arg == '*' && GET_LEVEL(ch) >= LVL_GOD) {
    for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i);
    send_to_char("Reset world.\r\n", ch);
    sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
    return;
  } else if (*arg == '.')
    i = world[ch->in_room].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i >= 0 && i <= top_of_zone_table) {
    // DM - allow zone owners to reset their zone.
    if (GET_LEVEL(ch) < LVL_GOD && (GET_OLC_ZONE(ch) != 0 && GET_OLC_ZONE(ch)
          != zone_table[i].number)) {
      send_to_char("Sorry, you can only reset your zone.\r\n", ch);
      return;
    }
    reset_zone(i);
    sprintf(buf, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number,
	    zone_table[i].name);
    send_to_char(buf, ch);
    sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
  } else
    send_to_char("Invalid zone number.\r\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  struct char_data *vict;
  struct affected_type *affect;
  long result=0;
  char reasonString[MAX_INPUT_LENGTH], *reason = reasonString;

  reasonString[0] = '\0';
  reason = one_argument(argument, arg);

  // remove leading space
  reason++;

  if (!*arg)
  {
    send_to_char("Yes, but for whom?!?\r\n", ch);
    return;
  }
  if (!(vict = generic_find_char(ch, arg, FIND_CHAR_ROOM | FIND_CHAR_WORLD)))
  {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if (IS_NPC(vict))
  {
    send_to_char("You can't do that to a mob!\r\n", ch);
    return;
  }
  if (GET_LEVEL(vict) > GET_LEVEL(ch))
  {
    send_to_char("Hmmm...you'd better not.\r\n", ch);
    return;
  }
  switch (subcmd)
  {
    case SCMD_REROLL:
      send_to_char("Rerolled...\r\n", ch);
      roll_real_abils(vict);
      basic_mud_log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      sprintf(buf, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	      GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      send_to_char(buf, ch);
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER))
      {
	send_to_char("Your victim is not flagged.\r\n", ch);
	return;
      }
      REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
      send_to_char("Pardoned.\r\n", ch);
      send_to_char("You have been pardoned by the Gods!\r\n", vict);
      sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      if (result && (!*reason || (strlen(reason) < 10)))
      {
	PLR_TOG_CHK(vict, PLR_NOTITLE);
	send_to_char("Please supply a reason of no less than 10 characters.\r\n" , ch);
	return;
      }
      sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      if (result)
      {
	sprintf(buf, "&7%s&g has had title privilleges removed for %s",
		GET_NAME(vict), reason);
	mudlog(buf, NRM, 0, FALSE);
      }
      break;
    case SCMD_SQUELCH:
      result = PUN_TOG_CHK(vict, PUN_MUTE);
      if (result && (!*reason || (strlen(reason) < 10)))
      {
	PUN_TOG_CHK(vict, PUN_MUTE);
	send_to_char("Please supply a reason of no less than 10 characters.\r\n" , ch);
	return;
      }
      sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      if (result)
      {
	sprintf(buf, "&7%s&g has been muted for %s", GET_NAME(vict), reason);
	mudlog(buf, NRM, 0, FALSE);
      }
      break;
    case SCMD_FREEZE:
      if (ch == vict)
      {
	send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
	return;
      }
      if (PUN_FLAGGED(vict, PUN_FREEZE))
      {
	send_to_char("Your victim is already pretty cold.\r\n", ch);
	return;
      }
      if (!*reason || (strlen(reason) < 10))
      {
	send_to_char("Please supply a reason of no less than 10 characters.\r\n" , ch);
	return;
      }
//      SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      SET_BIT(PUN_FLAGS(vict), (1 << PUN_FREEZE));
      PUN_HOURS(vict, PUN_FREEZE) = -1;
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
      send_to_char("Frozen.\r\n", ch);
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      if (result)
      {
	sprintf(buf, "&7%s&g has been frozen for %s", GET_NAME(vict), reason);
	mudlog(buf, NRM, 0, FALSE);
      }
      break;
    case SCMD_THAW:
//      if (!PLR_FLAGGED(vict, PLR_FROZEN)) { Replaced with PUN_ -- ARTUS
      if (!PUN_FLAGGED(vict, PUN_FREEZE))
      {
	send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch))
      {
	sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n", GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	send_to_char(buf, ch);
	return;
      }
      sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      REMOVE_BIT(PUN_FLAGS(vict), (1 << PUN_FREEZE));
      PUN_HOURS(vict, PUN_FREEZE) = 0;
      send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
      send_to_char("Thawed.\r\n", ch);
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected)
      {
	for (affect = vict->affected; affect; affect = affect->next)
	{
	//while (vict->affected)
	  if (vict->affected->duration != -1)
	    affect_remove(vict, vict->affected);
	}
	send_to_char("There is a brief flash of light!\r\n"
		     "You feel slightly different.\r\n", vict);
	send_to_char("All spells removed.\r\n", ch);
      } else {
	send_to_char("Your victim does not have any affections!\r\n", ch);
	return;
      }
      break;
    default:
      basic_mud_log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
  }
  save_char(vict, NOWHERE);
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, zone_rnum zone, int level, bool detailed)
{
// DM - TODO - color this in ...
  extern const char *zone_flagbits[];
  char bufstr[80];
  sprintbit(zone_table[zone].zflag, zone_flagbits, bufstr);

  sprintf(bufptr, "%s&B%3d&n %-20.20s &gAge&n: &y%3d&n; &gReset&n: &y%3d&n &b(&r%1d&b)&n; &gTop&n: &c%5d&n &gFlags&n: %s\r\n", bufptr, zone_table[zone].number, zone_table[zone].name,
          zone_table[zone].age, zone_table[zone].lifespan,
          zone_table[zone].reset_mode, zone_table[zone].top,bufstr);


  if (detailed) {

    // TODO - shouldnt need generate once working
    //generate_zone_data();

    //sprintf(bufptr, "%s&RSTILL FIXING&n:\r\n", bufptr); 
    sprintf(bufptr, "%sNumber of rooms: %3d\r\n", 
        bufptr, zone_table[zone].nowlds);
    sprintf(bufptr, "%sNumber of trigs: %3d\r\n", 
        bufptr, zone_table[zone].notrgs);
    sprintf(bufptr, "%sNumber of shops: %3d\r\n", 
        bufptr, zone_table[zone].noshps);
    sprintf(bufptr, "%sNumber of mobs:  %3d\r\n", 
        bufptr, zone_table[zone].nomobs);
    sprintf(bufptr, "%sNumber of objs:  %3d\r\n", 
        bufptr, zone_table[zone].noobjs);
    sprintf(bufptr, "%sNumber of hints: %3d\r\n",
	bufptr, zone_table[zone].nohnts);
    // ARTUS> Show OLC Users for that zone.
    if (level >= LVL_GRGOD)
    {
      bool found = FALSE;
      int i;
      extern struct player_index_element *player_table;
      strcat(bufptr, "OLC Players: ");
      for (i = 0; i <= top_of_p_table; i++)
      {
	struct char_file_u chdata;
	load_char(player_table[i].name, &chdata);
	if (chdata.player_specials_saved.olc_zone == zone_table[zone].number)
	{
	  if (found) strcat(bufptr, ", ");
	  found = TRUE;
	  strcat(bufptr, chdata.name);
	}
      }
      if (!found) strcat(bufptr, "None");
      strcat(bufptr, ".\r\n");
    }
  }
//  sprintf(bufptr, "%s%3d %-30.30s Age: %3d; Reset: %3d (%1d); Top: %5d\r\n",
//	  bufptr, zone_table[zone].number, zone_table[zone].name,
//	  zone_table[zone].age, zone_table[zone].lifespan,
//	  zone_table[zone].reset_mode, zone_table[zone].top);
}


ACMD(do_show)
{
  struct char_file_u vbuf;
  int i, j, k, l, con;		/* i, j, k to specifics? */
  zone_rnum zrn;
  zone_vnum zvn;
  char self = 0;
  struct char_data *vict;
  struct obj_data *obj;
  struct descriptor_data *d;
  extern struct hunt_data *hunt_list;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH];

  struct show_struct
  {
    const char *cmd;
    const unsigned char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_ETRNL6 },			/* 1 */
    { "player",		LVL_GOD },
    { "rent",		LVL_GOD },
    { "stats",		LVL_ETRNL8 },
    { "errors",		LVL_IMPL },			/* 5 */
    { "death",		LVL_ANGEL },
    { "godrooms",	LVL_ANGEL },
    { "shops",		LVL_ETRNL7 },
    { "houses",		LVL_GOD },
    { "snoop",		LVL_IMPL },			/* 10 */
    { "events",         LVL_GOD }, 
    { "hunts",		LVL_GOD },
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	sprintf(buf + strlen(buf), "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_IDNUM(ch) > 3) {
    if (GET_LEVEL(ch) < fields[l].level) {
      send_to_char("You are not godly enough for that!\r\n", ch);
      return;
    }
  }

  if (!strcmp(value, "."))
    self = 1;
  buf[0] = '\0';
  switch (l) {
  case 1:			/* zone */
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, world[ch->in_room].zone, GET_LEVEL(ch), TRUE);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, zrn, GET_LEVEL(ch), TRUE);
      else {
	send_to_char("That is not a valid zone.\r\n", ch);
	return;
      }
    } else
      for (zrn = 0; zrn <= top_of_zone_table; zrn++)
	print_zone_to_buf(buf, zrn, GET_LEVEL(ch), FALSE);
    page_string(ch->desc, buf, TRUE);
    break;
  case 2:			/* player */
    if (!*value) {
      send_to_char("A name would help.\r\n", ch);
      return;
    }

    if (load_char(value, &vbuf) < 0) {
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
    sprintf(buf, "Player: %-12s (%s) [%2d %s]\r\n", vbuf.name,
      genders[(int) vbuf.sex], vbuf.level, class_abbrevs[(int) vbuf.chclass]);
    sprintf(buf + strlen(buf),
	 "Au: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
	    vbuf.points.gold, vbuf.points.bank_gold, vbuf.points.exp,
	    vbuf.char_specials_saved.alignment,
	    vbuf.player_specials_saved.spells_to_learn);
    sprintf(buf + strlen(buf),
	    "Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
	    asctime(localtime(&vbuf.birth)),
	    asctime(localtime(&vbuf.player_specials_primalsaved.last_logon)), 
//	    ctime(&vbuf.birth),
//	    ctime(&vbuf.player_specials_primalsaved.last_logon), 
            (int) (vbuf.played / 3600),
	    (int) (vbuf.played / 60 % 60));
    send_to_char(buf, ch);
    break;
  case 3:
    if (!*value) {
      send_to_char("A name would help.\r\n", ch);
      return;
    }
    Crash_listrent(ch, value);
    break;
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;

// DM - TODO - color in
    strcpy(buf, "&1Current stats:&n\r\n");
    sprintf(buf + strlen(buf), "  %5d players in game  %5d connected\r\n",
		i, con);
    sprintf(buf + strlen(buf), "  %5d registered\r\n",
		top_of_p_table + 1);
    sprintf(buf + strlen(buf), "  %5d mobiles          %5d prototypes\r\n",
		j, top_of_mobt + 1);
    sprintf(buf + strlen(buf), "  %5d objects          %5d prototypes\r\n",
		k, top_of_objt + 1);
    sprintf(buf + strlen(buf), "  %5d rooms            %5d zones\r\n",
		top_of_world + 1, top_of_zone_table + 1);
    sprintf(buf + strlen(buf), "  %5d large bufs\r\n",
		buf_largecount);
    sprintf(buf + strlen(buf), "  %5d buf switches     %5d overflows\r\n",
		buf_switches, buf_overflows);
    sprintf(buf +strlen(buf),  "  %5d burgled rooms    %5d events\r\n",
		num_rooms_burgled, events.num_events); 
    send_to_char(buf, ch);
    break;
  case 5:
    strcpy(buf, "&1Errant Rooms\r\n------------&n\r\n");
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
	if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0)
	  sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, GET_ROOM_VNUM(i),
		  world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 6:
    strcpy(buf, "&1Death Traps\r\n-----------&n\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j,
		GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 7:
    strcpy(buf, "&1Godrooms\r\n--------------------------&n\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
    if (ROOM_FLAGGED(i, ROOM_GODROOM))
      sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n",
		++j, GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 8:
    show_shops(ch, value);
    break;
  case 9:
    hcontrol_list_houses(ch, NULL);
    break;
  case 10:
    *buf = '\0';
    send_to_char("&1People currently snooping:\r\n", ch);
    send_to_char("&1--------------------------&n\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
    {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      sprintf(buf + strlen(buf), "&7%-10s&n - snooped by &7%s&n.\r\n",
               GET_NAME(d->snooping->character), GET_NAME(d->character));
    }
    send_to_char(*buf ? buf : "No one is currently snooping.\r\n", ch);
    break; /* snoop */
  case 11:
    list_events_to_char(ch, -1);
    break;
  case 12: /* Hunting */
    send_to_char("&1People currently hunting:\r\n"
	         "&1-------------------------&n\r\n", ch);
    *buf = '\0';
    for (struct hunt_data *hcur = hunt_list; hcur; hcur = hcur->next)
    {
      if (CAN_SEE(ch, hcur->hunter) && CAN_SEE(ch, hcur->victim))
      {
	sprintf(buf + strlen(buf), "&7%s&n [&8%d&n] is hunting &7%s&n [&8%d&n]\r\n",            GET_NAME(hcur->hunter), world[IN_ROOM(hcur->hunter)].number,
	        GET_NAME(hcur->victim), world[IN_ROOM(hcur->victim)].number);
      }
    }
    if (*buf)
      send_to_char(buf, ch);
    else
      send_to_char("No one is currently hunting.\r\n", ch);
    break; /* Hunting */
  default:
    send_to_char("Sorry, I don't understand that.\r\n", ch);
    break;
  }
}


void show_specials_to_char(struct char_data *ch)
{
  send_to_char("\r\n &0Specials available:&n\r\n", ch);
  send_to_char("&y1 &n - Permanant invisibility\r\n", ch);
  send_to_char("&y2 &n - Permanant sneak\r\n", ch);
  send_to_char("&y3 &n - Multiple weapon usage\r\n", ch);
  send_to_char("&y4 &n - Greater spell power in forests\r\n", ch);
  send_to_char("&y5 &n - Forest allies\r\n", ch);
  send_to_char("&y6 &n - Healer (more effective healing spells)\r\n", ch);
  send_to_char("&y7 &n - Priest (charge for spell casting)\r\n", ch);
  send_to_char("&y8 &n - Backstab during battle\r\n", ch);
  send_to_char("&y9 &n - Battlemage (more melee and spell damage)\r\n", ch);
  send_to_char("&y10&n - Mana thief (drain mana during battle)\r\n", ch);
  send_to_char("&y11&n - Holy warrior (destroy undead)\r\n", ch);
  send_to_char("&y12&n - Disguise (changling ability)\r\n", ch);
  send_to_char("&y13&n - Escape (can find nearest outdoors)\r\n", ch);
  send_to_char("&y14&n - Permanant infravision\r\n", ch);
  send_to_char("&y15&n - Dwarf (bonus to battle while indoors)\r\n", ch);
  send_to_char("&y16&n - Permanant group sneak\r\n", ch);
  send_to_char("&y17&n - Thief (enhanced thief ability)\r\n", ch);
  send_to_char("&y18&n - Gore (extra horn damage in battle)\r\n", ch);
  send_to_char("&y19&n - Minotaur (+5% to damroll)\r\n", ch);
  send_to_char("&y20&n - Charmer (auto charm demi humans)\r\n", ch);
  send_to_char("&y21&n - Superman (+2% damroll, AC bonus, 21str, 21con)\r\n", ch);
  send_to_char("&y22&n - Permanant fly\r\n", ch);
  send_to_char("&y23&n - Elf (bonus to battle in forests)\r\n", ch);
  send_to_char("&y24&n - Tracker (Can track through !TRACK rooms)\r\n", ch);

}

/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
  struct set_struct {
    const char *cmd;
    const int level;
    const char pcnpc;
    const char type;
    const char *help;	// Some brief help for the command
  } set_fields[] = {
   { "brief",		LVL_GOD, 	PC, 	BINARY, "On or Off"},  /* 0*/
   { "invstart", 	LVL_GOD, 	PC, 	BINARY, "Level to start invis at"},  /* 1 */
   { "title",		LVL_GOD, 	PC, 	MISC , 	"Target's new title" },
   { "nosummon", 	LVL_GRGOD, 	PC, 	BINARY, "On or Off"},
   { "maxhit",		LVL_GRGOD, 	BOTH, 	NUMBER, "Maximum hit points" },
   { "maxmana", 	LVL_GRGOD, 	BOTH, 	NUMBER, "Maximum mana points" },  /* 5 */
   { "maxmove", 	LVL_GRGOD, 	BOTH, 	NUMBER, "Maximum movement points" },
   { "hit", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Current hit points" },
   { "mana",		LVL_GRGOD, 	BOTH, 	NUMBER, "Current mana points" },
   { "move",		LVL_GRGOD, 	BOTH, 	NUMBER, "Current movement points" },
   { "align",		LVL_GOD, 	BOTH, 	NUMBER, "Alignment (-1k -> 1k)" },  /* 10 */
   { "str",		LVL_GRGOD, 	BOTH, 	NUMBER, "Strength (3->21 or 25)" },
   { "stradd",		LVL_GRGOD, 	BOTH, 	NUMBER, "Strength percent (for Str 18)" },
   { "int", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Intelligence (3->21 or 25)" },
   { "wis", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Wisdom (3->21 or 25)" },
   { "dex", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Dexterity (3->21 or 25)" },  /* 15 */
   { "con", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Constitution (3->21 or 25)" },
   { "cha",		LVL_GRGOD, 	BOTH, 	NUMBER, "Charisma (3->21 or 25)" },
   { "ac", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Armour Class (-ve = better)" },
   { "gold",		LVL_GOD, 	BOTH, 	NUMBER, "Gold on hand" },
   { "bank",		LVL_GOD, 	PC, 	NUMBER, "Gold in bank" },  /* 20 */
   { "exp", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Experience earnt current level" },
   { "hitroll", 	LVL_GRGOD, 	BOTH, 	NUMBER, "Hitroll bonus" },
   { "damroll", 	LVL_GRGOD, 	BOTH, 	NUMBER, "Damroll bonus" },
   { "invis",		LVL_IMPL, 	PC, 	NUMBER, "Invisibility level" },
   { "nohassle", 	LVL_GRGOD, 	PC, 	BINARY, "On or Off" },  /* 25 */
   { "frozen",		LVL_FREEZE, 	PC, 	BINARY, "On or Off" },
   { "practices", 	LVL_GRGOD, 	PC, 	NUMBER, "Number of practices" },
   { "lessons", 	LVL_GRGOD, 	PC, 	NUMBER, "Number of practices" },
   { "drunk",		LVL_GRGOD, 	BOTH, 	MISC, 	"State (0->24) or Off" },
   { "hunger",		LVL_GRGOD, 	BOTH, 	MISC, 	"State (0->24) or Off" },    /* 30 */
   { "thirst",		LVL_GRGOD, 	BOTH, 	MISC, 	"State (0->24) or Off" },
   { "killer",		LVL_GOD, 	PC, 	BINARY, "On or Off" },
   { "thief",		LVL_GOD, 	PC, 	BINARY, "On or Off" },
   { "level",		LVL_IMPL, 	BOTH, 	NUMBER, "New level within class" },
   { "room",		LVL_IMPL, 	BOTH, 	NUMBER, "Room vnum to put target in" },  /* 35 */
   { "roomflag", 	LVL_GRGOD, 	PC, 	BINARY, "On or Off" },
   { "siteok",		LVL_GRGOD, 	PC, 	BINARY, "On or Off" },
   { "deleted", 	LVL_IMPL, 	PC, 	BINARY, "On or Off" },
   { "class",		LVL_GRGOD, 	BOTH, 	MISC, 	"First few letters of class" },
   { "nowizlist", 	LVL_GOD, 	PC, 	BINARY, "On or Off" },  /* 40 */
   { "quest",		LVL_GOD, 	PC, 	BINARY, "On or Off" },
   { "loadroom", 	LVL_GRGOD, 	PC, 	MISC, 	"Room to load player at (vnum) or 'defaults'" },
   { "color",		LVL_GOD, 	PC, 	BINARY, "On or Off" },
   { "idnum",		LVL_IMPL, 	PC, 	NUMBER, "Set NPC's ID (Why?)" },
   { "passwd",		LVL_IMPL, 	PC, 	MISC, 	"Set player's password" },    /* 45 */
   { "nodelete", 	LVL_GOD, 	PC, 	BINARY, "On or Off" },
   { "sex", 		LVL_GRGOD, 	BOTH, 	MISC, 	"Male/Female/Neutral" },
   { "age",		LVL_GRGOD,	BOTH,	NUMBER, "New age (3->199)" },
   { "height",		LVL_GOD,	BOTH,	NUMBER, "New height" },
   { "weight",		LVL_GOD,	BOTH,	NUMBER, "New weight" },  /* 50 */
   { "olc",             LVL_IMPL,       PC,     NUMBER, "OLC Zone (0: all access, -1: no access, N: only for zone N)" },

// Primal set commands
   { "timer",           LVL_IMPL,       PC,     NUMBER, "Targets timer value" },
   { "holylight",       LVL_GRGOD,      PC,     BINARY, "On or Off" },  
   { "infection",       LVL_GRGOD,      PC,     MISC, 	"Werewolf/Vampire/None" },
   { "tag",             LVL_GOD,        PC,     BINARY, "On or Off" },  /* 55 */
   { "palign",          LVL_GOD,        PC,     BINARY, "Display Alignment prompt (On or Off)" },
   { "noignore",        LVL_CLAN_GOD,   PC,     BINARY, "On or Off" },
   { "clan",            LVL_GOD,        PC,     NUMBER, "Clan Number"   },  
   { "fix",             LVL_IMPL,       PC,     BINARY, "Bullshit" },
   { "clanrank",        LVL_CLAN_GOD,   PC,     NUMBER, "Clan Rank Number" },    /* 60 */
   { "autogold",        LVL_GOD,        PC,     BINARY, "On or Off" },
   { "autoloot",        LVL_GOD,        PC,     BINARY, "On or Off" },
   { "autoassist",      LVL_GOD,        PC,     MISC, 	"Off or player to autoassist"   },   
   { "autoassisters",   LVL_GOD,        BOTH,   MISC, 	"Off"  },
   { "autosplit",       LVL_GOD,        BOTH,   MISC, 	"On or Off" },    /* 65 */ 
   { "race", 		LVL_GOD, 	PC,   	MISC, 	"First few letters of race name" },
   { "special",         LVL_GRGOD,      PC,   	MISC, 	"Special # to set On or Off or 'list'" },
   { "nohints",         LVL_GOD,        PC,   	BINARY, "On or Off" },
   { "statpoints",     	LVL_GOD,        PC,   	NUMBER, "Stat Points" }, 
   { "pkill",           LVL_GOD,        PC,   	BINARY, "On of Off" }, /* 70 */
   { "whostring",       LVL_GRGOD,	PC,	MISC, 	"Targets Who String (Max 20 Chars)" },
   { "remortlev1",	LVL_IMPL,	PC, 	NUMBER, "First remort level" },
   { "remortlev2",	LVL_IMPL,	PC,	NUMBER,	"Second remort level" },
   { "debug",		LVL_IMPL,	PC,	BINARY, "On or Off" },
   { "unholiness",	LVL_GRGOD,	PC,	NUMBER, "Unholy kill counter" },
   { "\n", 0, BOTH, MISC }
  };


int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  int i, on = 0, off = 0, value = 0;
  room_rnum rnum;
  room_vnum rvnum;
  char output[MAX_STRING_LENGTH];
  void add_to_immlist(char *name, long idnum, long immkills, ubyte unholiness);

  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) < LVL_OWNER)
  {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch)
    {
      send_to_char("Maybe that's not such a great idea...\r\n", ch);
      return (0);
    }
  }
  if (GET_IDNUM(ch) > 3)
  {
    if (GET_LEVEL(ch) < set_fields[mode].level)
    {
      send_to_char("You are not godly enough for that!\r\n", ch);
      return (0);
    }
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC))
  {
    send_to_char("You can't do that to a beast!\r\n", ch);
    return (0);
  } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char("That can only be done to a beast!\r\n", ch);
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char("Value must be 'on' or 'off'.\r\n", ch);
      return (0);
    }
    sprintf(output, "%s %s for %s.", set_fields[mode].cmd, ONOFF(on),
	    GET_NAME(vict));
  } else if (set_fields[mode].type == NUMBER) {
    value = atoi(val_arg);
    sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
	    set_fields[mode].cmd, value);
  } else {
    strcpy(output, "Okay.");  /* can't use OK macro here 'cause of \r\n */
  }

  switch (mode) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 2:
    set_title(vict, val_arg);
    sprintf(output, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    sprintf(output, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 4:
    vict->points.max_hit = RANGE(1, 32768);
    affect_total(vict);
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 32768);
    affect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 32768);
    affect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    affect_total(vict);
    break;
  case 8:
    vict->points.mana = RANGE(0, vict->points.max_mana);
    affect_total(vict);
    break;
  case 9:
    vict->points.move = RANGE(0, vict->points.max_move);
    affect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-5000, 5000);
    affect_total(vict);
    break;
  case 11:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.str = value;
    vict->real_abils.str_add = 0;
    affect_total(vict);
    break;
  case 12:
    vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
    break;
  case 13:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.cha = value;
    affect_total(vict);
    break;
  case 18:
    vict->points.armor = RANGE(-200, 200);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 1000000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 1000000000);
    break;
  case 21:
    vict->points.exp = RANGE(0, level_exp(vict,GET_LEVEL(vict)));
    break;
  case 22:
    vict->points.hitroll = RANGE(0, 1000000);
    affect_total(vict);
    break;
  case 23:
    vict->points.damroll = RANGE(0, 1000000);
    affect_total(vict);
    break;
  case 24:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char("You aren't godly enough for that!\r\n", ch);
      return (0);
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char("You aren't godly enough for that!\r\n", ch);
      return (0);
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict && on) {
      send_to_char("Better not -- could be a long winter!\r\n", ch);
      return (0);
    }
    SET_OR_REMOVE(PUN_FLAGS(vict), PUN_FREEZE);
    if (PUN_FLAGGED(vict, PUN_FREEZE))
      PUN_HOURS(vict, PUN_FREEZE) = -1;
    else
      PUN_HOURS(vict, PUN_FREEZE) = 0;
    break;
  case 27:
  case 28:
    GET_PRACTICES(vict) = RANGE(0, 1000);
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off")) {
      GET_COND(vict, (mode - 29)) = (char) -1; /* warning: magic number here */
      sprintf(output, "%s's %s now off.", GET_NAME(vict), set_fields[mode].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (mode - 29)) = (char) value; /* and here too */
      sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
	      set_fields[mode].cmd, value);
    } else {
      send_to_char("Must be 'off' or a value from 0 to 24.\r\n", ch);
      return (0);
    }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    if (value <= 0 || value > LVL_OWNER)
    {
      send_to_char("Invalid level.\r\n", ch);
      return (0);
    }
    if (GET_IDNUM(ch) > 3)
    {
      if (GET_LEVEL(ch) < GET_LEVEL(vict) || value > GET_LEVEL(ch) || 
          value > LVL_OWNER)
      {
        send_to_char("You can't do that.\r\n", ch);
        return (0);
      }
    }
    vict->player.level = (byte) value;
    vict->player.level = RANGE(1, LVL_OWNER);
    if ((GET_CLASS(vict) == CLASS_MASTER) && (GET_LEVEL(vict) >= LVL_CHAMP) &&
	(GET_LEVEL(vict) < LVL_ANGEL))
      add_to_immlist(GET_NAME(vict), GET_IDNUM(vict), GET_IMMKILLS(vict),
	             GET_UNHOLINESS(vict));
    break;
  case 35:
    if ((rnum = real_room(value)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return (0);
    }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
      char_from_room(vict);
    char_to_room(vict, rnum);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = parse_class(val_arg)) == CLASS_UNDEFINED) {
      send_to_char("That is not a class.\r\n", ch);
      return (0);
    }
    remove_class_specials(ch);
    GET_CLASS(vict) = i;
    set_class_specials(vict);
    apply_specials(vict, FALSE);
    calc_modifier(ch);
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;

  /**
   * DM: Determine the world for the given vnum, and set the entry room for
   * that world. It is possible to set an entry room for each world simply by
   * passing the vnum of a room in each world.
   * 
   * The entry rooms are cleared when the given argument is an abbreviation of
   * "defaults", indicating that the default world_start_rooms are to be used. 
   */
  case 42:
    if (is_abbrev(val_arg, "defaults")) {
      for (int i = 0; i < NUM_WORLDS; i++) {
        ENTRY_ROOM(vict, i) = NOWHERE; 
      }
    } else if (is_number(val_arg)) {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE) {
        ENTRY_ROOM(vict, get_world(real_room(rvnum))) = rvnum;
	sprintf(output, "%s will enter %s at room #%d.", 
                GET_NAME(vict),
		world_names[get_world(real_room(rvnum))], 
                ENTRY_ROOM(vict, get_world(real_room(rvnum))));
      } else {
	send_to_char("That room does not exist!\r\n", ch);
	return (0);
      }
    } else {
      send_to_char("Must be 'defaults' or a room's virtual number.\r\n", ch);
      return (0);
    }
    break;

  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
    break;
  case 44:
    if (!IS_NPC(vict))
      return (0);
    GET_IDNUM(vict) = value;
    break;
  case 45:
/*  // Artus> This was already commented out, Guess it was while debugging
    if (GET_IDNUM(ch) > 1) {
      send_to_char("Please don't use this command, yet.\r\n", ch);
      return (0);
    }
*/
/*
  if (GET_LEVEL(vict) >= LVL_GRGOD) {
      send_to_char("You cannot change that.\r\n", ch);
      return (0);
    }
*/
    // Artus> Changed to this..
    if ((GET_LEVEL(vict) >= GET_LEVEL(ch)) && (GET_IDNUM(ch) != 1))
    {
      send_to_char("You can't do that, perhaps you better see a higher power!\r\n", ch);
      return(0);
    }
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    sprintf(output, "Password changed to '%s'.", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    if ((i = search_block(val_arg, genders, FALSE)) < 0) {
      send_to_char("Must be 'male', 'female', or 'neutral'.\r\n", ch);
      return (0);
    }
    GET_SEX(vict) = i;
    break;
  case 48:	/* set age */
    if (value < 2 || value > 200) {	/* Arbitrary limits. */
      send_to_char("Ages 2 to 200 accepted.\r\n", ch);
      return (0);
    }
    /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
    vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
    break;

  case 49:	/* Blame/Thank Rick Glover. :) */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 50:
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 51:
    GET_OLC_ZONE(vict) = value;
    break;

// Primal set commands:

  case 52: // Timer
    vict->char_specials.timer=value;
    break; 

  case 53: // Holylight
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_HOLYLIGHT);
    break;

  case 54: // Infection
      if(!str_cmp(val_arg,"werewolf")) {
        REMOVE_BIT(PRF_FLAGS(vict),PRF_VAMPIRE);
        SET_BIT(PRF_FLAGS(vict),PRF_WOLF);
      } else if(!str_cmp(val_arg,"vampire")) {
        REMOVE_BIT(PRF_FLAGS(vict),PRF_WOLF);
        SET_BIT(PRF_FLAGS(vict),PRF_VAMPIRE);
      } else if(!str_cmp(val_arg,"none")) {
        REMOVE_BIT(PRF_FLAGS(vict),PRF_WOLF | PRF_VAMPIRE);
      } else {
        send_to_char("Must be 'werewolf', 'vampire', or 'none'.\r\n",ch);
        return (0);
        }
      break;
 
  case 55: // Tag
    SET_OR_REMOVE(PRF_FLAGS(vict),PRF_TAG);
    break;

  case 56: // Palign
    SET_OR_REMOVE(PRF_FLAGS(vict),PRF_DISPALIGN);
    break;

  case 57: // Noignore
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOIGNORE );
    if (!str_cmp(val_arg, "on") )
    {
      for (i = 0; i < MAX_IGNORE; i++) {
        GET_IGNORE(vict, i) = 0;
        GET_IGNORE_ALL(vict, i) = TRUE;
        GET_IGN_LVL(vict) = 0;
        GET_IGN_LVL_ALL(vict) = FALSE;
      }
    }
    break;

  case 58: // Clan
    if ((value < 0) || (value > MAX_CLANS)) {
      sprintf(buf, "Value must be between 0 and %d.\r\n", MAX_CLANS);
      send_to_char(buf, ch);
      return (0);
    }
    if (value == 0)
      GET_CLAN_RANK(vict) = 0;
 
    if ((find_clan_by_id(value) < 0) && (value != 0)) {
      send_to_char("That clan doesn't seem to exist.\r\n", ch);
      return (0);
    }

    GET_CLAN(vict) = value;

    if (GET_CLAN_RANK(vict) > clan[find_clan_by_id(value)].ranks)
      GET_CLAN_RANK(vict) = clan[find_clan_by_id(value)].ranks;

    sprintf(output, "Clan changed to %d (%s).", GET_CLAN(vict), ((find_clan_by_id(GET_CLAN(vict)) >= 0) ? clan[find_clan_by_id(GET_CLAN(vict))].name : "Undefined"));
    break;

  case 59: // Fix
    // REMOVE_BIT(PRF_FLAGS(vict), PRF_FIX);
    break;

  case 60: // Clanrank
    if (value < 0) {
      send_to_char("Clan rank cannot be less than 0.\r\n", ch);
      return (0);
    }
    if (find_clan_by_id(GET_CLAN(vict)) >= 0) {
      if (value > clan[find_clan_by_id(GET_CLAN(vict))].ranks) {
        send_to_char("Clan rank cannot exceed clans maximum rank.\r\n", ch);
        return (0);
      }
    }
    GET_CLAN_RANK(vict) = value;
    sprintf(output, "Clan rank changed to %d.", GET_CLAN_RANK(vict));
    break;

  case 61: // Autogold
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_AUTOGOLD);
    break;

  case 62: // Autoloot
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_AUTOLOOT);
    break;

  case 63: // Autoassist
    if (!str_cmp(val_arg,"off")) {
      if (AUTOASSIST(vict)) {
        sprintf(buf, "%s is no longer autoassisting %s.", 
		GET_NAME(vict),GET_NAME(AUTOASSIST(vict)));
        stop_assisting(vict);
      } else {
        sprintf(buf,"And who is %s autoassisting?.",GET_NAME(vict));
        send_to_char(buf,ch);
        return (0);
      }
    } else {
      send_to_char("Options: OFF\r\n",ch);
      return (0);
    }
    break;

  case 64: // Autoassisters
    if (!str_cmp(val_arg,"off")) {
      if (vict->autoassisters) {
        sprintf(buf, "%s is no longer being autoassisted.", GET_NAME(vict));
        stop_assisters(vict);
      } else {
        sprintf(buf,"And who autoassisting %s?.",GET_NAME(vict));
        send_to_char(buf,ch);
        return (0);
      }
    } else {
      send_to_char("Options: OFF\r\n",ch);
      return (0);
    }
    break;

  case 65: // Autosplit
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_AUTOSPLIT);
    break;

  case 66:
    if ((i = parse_race_name(val_arg)) == RACE_UNDEFINED) {
      send_to_char("That is not a race.\r\n", ch);
      return (0);
    }
    remove_race_specials(vict);
    GET_RACE(vict) = i;
   set_race_specials(vict); 
    GET_MODIFIER(vict) = race_modifiers[i] + 
                 class_modifiers[(int)GET_CLASS(vict)] + 
		 special_modifier(vict) + elitist_modifier(vict);
   break;

  case 67:
    if (strcmp(val_arg, "list") ==  0 )
    {
      show_specials_to_char(ch);
      return (0);
    }
    if (!isdigit(val_arg[0]))
    {
      send_to_char("You must specify the value of the special.\r\n Use &4set <player> special list&n to see specials available.\r\n", ch);
      return (0);
    }
    if (atoi(val_arg) <= 0 || atoi(val_arg) > MAX_SPECIALS)
    {
      send_to_char("That's not a valid option!\r\nUser &4set <player> special list&n to see options.\r\n", ch);
      return (0);
    } else {
      if (IS_SET(GET_SPECIALS(vict), (1 << atoi(val_arg) - 1)))
      {
        REMOVE_BIT(GET_SPECIALS(vict), (1 << atoi(val_arg) - 1));
	// Artus> Reroll str/con after removing superman.
	if (atoi(val_arg) == 21)
	{
	  GET_REAL_STR(vict) = 13 + number(1, 5);
	  GET_REAL_CON(vict) = 13 + number(1, 5);
	}
      } else
        SET_BIT(GET_SPECIALS(vict), (1 << atoi(val_arg) - 1));
    }
    sprintf(buf, "&g%s&n toggled &4%s&n for %s.\r\n", 
            special_ability_bits[atoi(val_arg)-1], 
	    IS_SET(GET_SPECIALS(vict), (1 << atoi(val_arg) -1)) ? "on" : "off",
	    GET_NAME(vict));
    send_to_char(buf, ch);
    apply_specials(vict, FALSE);
    // Recalc the players modifier
    GET_MODIFIER(vict) = race_modifiers[GET_RACE(vict)] + 
                         class_modifiers[(int)GET_CLASS(vict)] +
	 		 special_modifier(vict) +
			 elitist_modifier(vict) +
			 unholiness_modifier(ch);
  
    break;
  // Nohints
  case 68:
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_NOHINTS);
    break;

  // Quest Points
  case 69:
    GET_STAT_POINTS(vict) = RANGE(1, 10000);
    break;
    
  // PKill Flag
  case 70:
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_PKILL);
    break;

  // Who String
  case 71:
    if (strlen(val_arg) > 19)
    {
      send_to_char("WhoString must not be longer than 19 chars.\r\n", ch);
      return (0);
    }
    sprintf(GET_WHO_STR(vict), "%s", val_arg);
    break;

  // First remort level
  case 72:
    if (value < 0 || value > LVL_OWNER) {
      send_to_char("Invalid level.\r\n", ch);
      return (0);
    }
    if (GET_IDNUM(ch) > 3) {
      if (GET_LEVEL(ch) < GET_LEVEL(vict) || value > GET_LEVEL(ch) ||
          value > LVL_OWNER) {
        send_to_char("You can't do that.\r\n", ch);
        return (0);
      }
    }
    GET_REM_ONE(vict) = (byte) value;
    GET_REM_ONE(vict) = RANGE(0, LVL_OWNER);
    break;

  // Second remort level
  case 73:
    if (value < 0 || value > LVL_OWNER) {
      send_to_char("Invalid level.\r\n", ch);
      return (0);
    }
    if (GET_IDNUM(ch) > 3) {
      if (GET_LEVEL(ch) < GET_LEVEL(vict) || value > GET_LEVEL(ch) ||
          value > LVL_OWNER) {
        send_to_char("You can't do that.\r\n", ch);
        return (0);
      }
    }
    GET_REM_TWO(vict) = (byte) value;
    GET_REM_TWO(vict) = RANGE(0, LVL_OWNER);
    break;

  // Debug Flag.
  case 74:
    SET_OR_REMOVE(SMALL_BITS(vict), SMB_DEBUG);
    break;

  // Unholiness
  case 75:
    if (value < 0 || value > 250)
    {
      send_to_char("Outside Range (0-250)\r\n", ch);
      return (0);
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict))
    {
      send_to_char("You can't do that.\r\n", ch);
      return (0);
    }
    GET_UNHOLINESS(vict) = (byte) value;
    calc_modifier(ch);
    break;

  default:
    send_to_char("Can't set that!\r\n", ch);
    return (0);
  }

  strcat(output, "\r\n");
  send_to_char(CAP(output), ch);
//  -- What is this supposed to do? All we have at this point
//    to log is 'on' or 'off' or something equally ambgious.
//    I added the log to 'do_set' instead.
//  sprintf(buf, "(GC) %s: %s", GET_NAME(ch), val_arg);
//  mudlog(buf, NRM, GET_LEVEL(ch) , TRUE);
//
  return (1);
}

void set_list(struct char_data *ch)
{
  int j, i = 0;
 
   for (j = 0,i = 1; set_fields[i].level; i++)
      if (set_fields[i].level <= GET_LEVEL(ch))
	sprintf(buf + strlen(buf), "&g%-10s&n - %s\r\n", set_fields[i].cmd, set_fields[i].help);
    strcat(buf, "\r\n");
    page_string(ch->desc, buf, TRUE);
}


ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  struct char_file_u tmp_store;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],
	val_arg[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  if (IS_NPC(ch))
    return;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file"))
  {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob")) {
    half_chop(buf, name, buf);
  } 

  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field)
  {
    send_to_char("&1Usage: &4set <victim> <field> <value>&n\r\n", ch);
    set_list(ch);
    return;
  }

  /* find the target */
  if (!is_file)
  {
    if (is_player)
    {
      if (!(vict = get_player_online(ch, name, FIND_CHAR_WORLD)))
      {
	send_to_char("There is no such player.\r\n", ch);
	return;
      }
    } else { /* is_mob */
      if (!(vict = generic_find_char(ch, name, FIND_CHAR_ROOM | FIND_CHAR_WORLD)))
      {
	send_to_char("There is no such creature.\r\n", ch);
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if ((player_i = load_char(name, &tmp_store)) > -1) {
      store_to_char(&tmp_store, cbuf);
      if (GET_LEVEL(cbuf) >= GET_LEVEL(ch)) {
	free_char(cbuf);
	send_to_char("Sorry, you can't do that.\r\n", ch);
	return;
      }
      vict = cbuf;
    } else {
      free(cbuf);
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;

  /* perform the set */
  retval = perform_set(ch, vict, mode, val_arg);

  /* Log the operation */
  if (retval) {
    sprintf(buf, "SET: %s '%s%s'", GET_NAME(ch), arg, argument);
    mudlog(buf, CMP, MAX(LVL_GOD, GET_LEVEL(ch)), TRUE);
  }

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict, NOWHERE);
    if (is_file) {
      char_to_store(vict, &tmp_store);
      fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
      fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
      fflush(player_fl);
      send_to_char("Saved in file.\r\n", ch);
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}


// do_global_set: Process the arguments and perform the setting and saving
//                of global variables.
ACMD(do_global_set)
{
  char field[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH];

  send_to_char(argument,ch);
  half_chop(argument, field, val_arg);

  sprintf(buf,"field: %s, value: %s\r\n",field,val_arg);
  send_to_char(buf,ch);

//  if (!*field) {
    send_to_char("&1Usage: &4global <field> <value>&n\r\n", ch);
    return;
//  }

  /* perform the set */
//  retval = globals.setVariable(field, val_arg);

  /* save the global vars file if a change was made 
  if (retval)
    globals.SaveAll();
  else {
    send_to_char("Field not found. Type &4global&n to see a list of fields.\r\n");
    return;
  } */
}

ACMD(do_deimmort)
{
  struct char_data *vict;
 
  one_argument(argument,arg);
 
  if (!(vict = get_player_online(ch, arg, FIND_CHAR_WORLD)))
  {
    send_to_char("I cannot seem to find that person!!!\r\n", ch);
    return;
  }
  if (IS_NPC(vict))
  {
    send_to_char("DEMOTE A MONSTER!!!!.... Go find something more productive to do!\r\n", ch);
    return;
  }
  if (GET_LEVEL(vict) != LVL_ANGEL)
  {
    send_to_char("You can only demote ANGELS!!\r\n", ch);
    return;
  }
  GET_LEVEL(vict) = LVL_CHAMP + MIN(5, GET_UNHOLINESS(vict));
  send_to_char("You have DEMOTED them!.\r\n", ch);
  send_to_char("You have been demoted!!!.  You become a fallen angel!\r\nServes you RIGHT!\r\n", vict);
} 

ACMD(do_immort)
{
  struct char_data *vict;
  int i;
 
  if (IS_NPC(ch))
    return;

  one_argument(argument,arg);
 
  if (!(vict = get_player_online(ch, arg, FIND_CHAR_ROOM)))
  {
     send_to_char("I cannot seem to find that person!!!\r\n", ch);
     return;
  }
  if (IS_NPC(vict))
  {
    send_to_char("You want to immort a monster!!!!!. YOU MUST BE BORED!\r\n", ch);
    return;
  }
  if (GET_LEVEL(vict) < LVL_IMMORT)
  {
    send_to_char("They are not an IMMORTAL!!.  Can't do it!\r\n", ch);
    return;
  }
  if (GET_LEVEL(vict) == LVL_ANGEL)
  {
    send_to_char("They are already angel silly!\r\n", ch);
    return;
  }
  if (GET_LEVEL(vict) > LVL_ANGEL)
  {
    send_to_char("Yer right get serious!!!\r\n", ch);
    return;
  }

  for (i = 0; i < 3; i++)
    GET_COND(vict, i) = (char) -1;

  GET_LEVEL(vict) = LVL_ANGEL;
  send_to_char("Done...\r\n", ch);
  send_to_char("You are touched by the hand of god. You soul shivers for a second.\r\n", vict);
  send_to_char("You feel like an ANGEL!.\r\n", vict);
} 

ACMD(do_pkset)
{
  char buff[120];
  half_chop(argument, arg, buf);
 
  if (!*arg)
  {
     if (IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED))
     sprintf(arg,"on");
     else sprintf(arg,"off");
     sprintf(buff,"In this zone, Player Killing is %s.", arg);
     send_to_char(buff,ch);
     return;
  }
  if (!strcmp(arg, "on"))
     SET_BIT(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED);
  else if (!strcmp(arg, "off"))
     REMOVE_BIT(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED);
  sprintf(buff,"Player Killing in this zone is now %s.", arg);
  send_to_char(buff,ch);
}  

/* Change your whostring. */
ACMD(do_whostr)
{
  if (IS_NPC(ch))
    return;
  if (!(*argument))
  {
    send_to_char("Just what did you want to set it to ('none' will remove it)?\r\n", ch);
    return;
  }
  skip_spaces(&argument);
  if (strlen(argument) >= 20)
  {
    send_to_char("Whostring cannot exceed 19 characters.\r\n", ch);
    return;
  }
  if (!str_cmp(argument, "none"))
  {
    strcpy(GET_WHO_STR(ch), "");
    send_to_char("Whostring removed.\r\n", ch);
    return;
  }
  // Don't allow players to fake level strings.
  if (GET_LEVEL(ch) < LVL_IS_GOD)
    for (unsigned int i = 0; i < strlen(argument); i++)
      if (argument[i] == '/')
      {
	send_to_char("Whostring cannot contain '/' characters.\r\n", ch);
	return;
      }
  strncpy(GET_WHO_STR(ch), argument, 20);
  GET_WHO_STR(ch)[19] = '\0';
  sprintf(buf, "Whostring set to '%s&n'.\r\n", GET_WHO_STR(ch));
  send_to_char(buf, ch);
}

/* this is a must! clock over one mud hour */
ACMD(do_tic)
{
  extern int pulse;
  pulse=-1;
  send_to_char("Time moves forward one hour.\r\n",ch);
} 

/* skillshow - display the spell/skill  */
ACMD(do_skillshow)
{
  extern struct spell_info_type spell_info[];
  int i, sortpos, mana, is_file = FALSE;
  struct char_data *vict, *cbuf=NULL;
  struct char_file_u tmp_store;
  int class_index;

  if (IS_NPC(ch))
    return;
  
  one_argument(argument,arg);
 
  if (!*arg)
  {
    vict=ch;
    basic_mud_log("skillshow: no arg, vict=ch");
  } else if(!(vict = get_player_online(ch,arg,FIND_CHAR_WORLD))) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if (load_char(arg, &tmp_store) > -1)
    {
      store_to_char(&tmp_store, cbuf);
      basic_mud_log("skillshow: loaded player %s from disk", GET_NAME(cbuf));
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch))
      {
        free_char(cbuf);
        send_to_char("Sorry, you can't do that.\r\n", ch);
        return;
      }
      vict = cbuf;
      is_file = TRUE;
    } else {
      basic_mud_log("skillshow: no player found");
      send_to_char(NOPERSON,ch);
      return;
    }
  }
 
  sprintf(buf, "&1Spell/Skill abilities for: &B%s&n\r\n",GET_NAME(vict));
  sprintf(buf, "%s&1Practice Sessions: &M%d&n\r\n\r\n",buf,GET_PRACTICES(vict));
 
  sprintf(buf, "%s%-20s %-10s\r\n",buf,"&1Spell/Skill","Ability Mana&n");
  strcpy(buf2, buf);

  class_index = GET_CLASS(vict); 

  for (sortpos = 1; sortpos < MAX_SKILLS; sortpos++) {
    i = spell_sort_info[SORT_ALPHA][0][sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if (GET_LEVEL(vict) >= spell_info[i].min_level[(int) GET_CLASS(vict)]) {
      mana = mag_manacost(vict, i);
      sprintf(buf, "%-20s %-3d %-3d ", 
                      spell_info[i].name, GET_SKILL(vict, i), mana);
      strncat(buf2, buf, strlen(buf)); 

      /* Display the stat requirements */ 
      if (spell_info[i].str[class_index]!=0){
        if (GET_REAL_STR(vict) >= spell_info[i].str[class_index])
          sprintf(buf,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].str[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].str[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].intl[class_index]!=0){
        if (GET_REAL_INT(vict) >= spell_info[i].intl[class_index])
          sprintf(buf,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].intl[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].intl[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].wis[class_index]!=0){
        if (GET_REAL_WIS(vict) >= spell_info[i].wis[class_index])
          sprintf(buf,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].wis[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].wis[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].dex[class_index]!=0){
        if (GET_REAL_DEX(vict) >= spell_info[i].dex[class_index])
          sprintf(buf,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].dex[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].dex[class_index],CCNRM(ch,C_NRM));   
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].con[class_index]!=0){
        if (GET_REAL_CON(vict) >= spell_info[i].con[class_index])
          sprintf(buf,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].con[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].con[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
      if (spell_info[i].cha[class_index]!=0){
        if (GET_REAL_CHA(vict) >= spell_info[i].cha[class_index])
          sprintf(buf,"%sCha: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].cha[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sCha: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].cha[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      sprintf(buf,"\r\n");
      strncat(buf2, buf, strlen(buf));
    }
  }
 
  page_string(ch->desc, buf2, 1);

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}  

/* Vulcan Neck Pinch - basically just stuns the victim - Vader */
/* this was gunna be a skill but i didnt no if it fit :)       */
ACMD(do_pinch)
{
  struct char_data *vict;
 
  one_argument(argument,arg);
 
  if(!(vict = generic_find_char(ch,arg,FIND_CHAR_ROOM)))
  {
    if(FIGHTING(ch))
      vict = FIGHTING(ch);
    else
    {
      send_to_char("Upon whom do you wish to perform the Vulcan neck pinch??\r\n",ch);
      return;
    }
  }
 
  if(!AWAKE(vict))
  {
    send_to_char("It appears that your victim is already incapacitated...\r\n",ch);
    return;
  }
 
  if(vict == ch)
  {
    send_to_char("You reach up and perform the Vulcan neck pinch on yourself and pass out...\r\n",ch);
    act("$n calmly reaches up and performs the Vulcan neck pinch on $mself before passing out...",FALSE,ch,0,0,TO_ROOM);
    GET_POS(ch) = POS_STUNNED;
    return;
  }
 
  if(GET_LEVEL(vict) >= GET_LEVEL(ch))
  {
    send_to_char("It is not logical to incapacitate your fellow Gods...\r\n",ch);
    return;
  }
 
  act("You skillfully perform the Vulcan neck pinch on $N who instantly falls to the ground.",FALSE,ch,0,vict,TO_CHAR);
  act("$n gently puts $s hand on your shoulder and Vulcan neck pinches you! You pass out instantly...",FALSE,ch,0,vict,TO_VICT);
  act("$n skillfully performs the Vulcan neck pinch on $N who falls to the ground, stunned.",FALSE,ch,0,vict,TO_NOTVICT);
 
  stop_fighting(vict);
  GET_POS(vict) = POS_STUNNED;
} 

void john_in(struct char_data *ch)
{
  int fp;
  int nread;
 
  fp = open("/primal/lib/text/john.poofin", O_RDONLY);
  if (fp == -1)
    strcpy(buf, "***********John Has Arrived*************\n\n");
  else
  {
    nread = read(fp, buf, MAX_STRING_LENGTH);
    buf[nread] = '\0';
    close(fp);
  }
 
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
}
 
void cassandra_in(struct char_data *ch)
{
  int fp;
  int nread;
 
  fp = open("/home/mud/live/lib/text/cassandra.poofin", O_RDONLY);
  if (fp == -1)
    strcpy(buf, "***********Cassandra Has Arrived*************\n\n");
  else
  {
    nread = read(fp, buf, MAX_STRING_LENGTH);
    buf[nread] = '\0';
    close(fp);
  }
 
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
} 

void artus_out(struct char_data *ch)
{
  sprintf(buf, "You feel a deep sense of loss, as &[&7%s&] fades away before you.", GET_NAME(ch));
  for (struct char_data *k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
  {
    if ((IS_NPC(k)) || (k == ch) || (!CAN_SEE(k, ch)) || (!k->desc))
      continue;
    send_to_char(buf, k);
  }
}

void artus_in(struct char_data *ch)
{
  for (struct char_data *k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
  {
    if ((IS_NPC(k)) || (k == ch) || (!CAN_SEE(k, ch)) || (!k->desc))
      continue;
    act("You bow deeply in awe, as $n materialises before you.", TRUE, ch,
	NULL, k, TO_VICT);
    act("$N bows deeply before $n, awestruck.", TRUE, ch, NULL, k, TO_NOTVICT);
    if (GET_SEX(k) == SEX_FEMALE)
    {
      act("You try and get a little closer to $n.", TRUE, ch, NULL, k, TO_VICT);
      act("$N tries to push passed you to be closer to $n.", TRUE, ch, NULL, k,
	  TO_NOTVICT);
    }
  }
}

struct obj_data *get_obj_from(struct obj_data *list, long objID, int itemNumber) {

  struct obj_data *obj;
  int counter = 1;

  for( obj = list; obj; obj = obj->next ) {
	if (GET_OBJ_VNUM(obj) == objID && counter == itemNumber ) 
		return obj;
	else
		counter++;
  }

  return NULL;
}

bool race_has_stats_for_skill(int race_index, int class_index, int spellnum, int statNum)
{
  int statVal, i;
  extern struct spell_info_type spell_info[];

  if (!(race_index >= 0 && race_index < MAX_RACES && 
                  spellnum >= 0 && spellnum < TOP_SPELL_DEFINE &&
                  class_index >= 0 && class_index < NUM_CLASSES)) {
    basic_mud_log("SYSERR: invalid race (%d) class (%d) spellnum (%d) to "
             "race_has_stats_for_skill", race_index, class_index, spellnum);
    return (false);
  }

  if (!str_cmp(spell_info[spellnum].name, unused_spellname)) {
    basic_mud_log("SYSERR: unused spellnum (%d) passed to " 
             "race_has_stats_for_skill", spellnum);
    return (false);
    
  }

  // Specific Stat - only check one given
  if (statNum >= 0 && statNum < STAT_HIT) {
    if (class_index >= 0 && class_index < NUM_CLASSES) {
      statVal = pc_max_race_stats[race_index][statNum];
      if (class_index == CLASS_MASTER) statVal = 21;
      switch (statNum) {

      case STAT_STR:
        return (statVal >= spell_info[spellnum].str[class_index]);
      case STAT_INT:
        return (statVal >= spell_info[spellnum].intl[class_index]);
      case STAT_WIS:
        return (statVal >= spell_info[spellnum].wis[class_index]);
      case STAT_DEX:
        return (statVal >= spell_info[spellnum].dex[class_index]);
      case STAT_CON:
        return (statVal >= spell_info[spellnum].con[class_index]);
      case STAT_CHA:
        return (statVal >= spell_info[spellnum].cha[class_index]);
      }
    }
  }

  // Non-Specific Stat - check all stats
  for (i = 0; i < STAT_HIT; i++) {
    statVal = pc_max_race_stats[race_index][i];
    if (class_index == CLASS_MASTER) statVal = 21;
    switch (i) {
    
    case STAT_STR:
      if (statVal < spell_info[spellnum].str[class_index])
        return (FALSE);
      break;

    case STAT_INT:
      if (statVal < spell_info[spellnum].intl[class_index])
        return (FALSE);
      break;
      
    case STAT_WIS:
      if (statVal < spell_info[spellnum].wis[class_index])
        return (FALSE);
      break;

    case STAT_DEX:
      if (statVal < spell_info[spellnum].dex[class_index])
        return (FALSE);
      break;

    case STAT_CON:
      if (statVal < spell_info[spellnum].con[class_index])
        return (FALSE);
      break;

    case STAT_CHA:
      if (statVal < spell_info[spellnum].cha[class_index])
        return (FALSE);
      break;
    }
  }
  return (TRUE);
}

void print_spells(struct char_data *ch, int class_index, int race_index, int sort_type) {
  bool race = FALSE;
  char info[MAX_STRING_LENGTH];
  int spell_index, spellnum;
  extern struct spell_info_type spell_info[];

  if (sort_type < 0 || sort_type > NUM_SORT_TYPES) {
    basic_mud_log("SYSERR: invalid sort_type passed to print_spells");
    return;
  }

  if (class_index == -1 && race_index == -1) {
    basic_mud_log("SYSERR: no class or race index to print_spells");
    return;
  }

  // class only
  if (class_index >= 0 && class_index < NUM_CLASSES && race_index == -1) {
    sprintf(info, "&1Spells and Skills for the &B%s&1 sorted &b%s&n\r\n\r\n", 
        CLASS_NAME(class_index), sort_names[sort_type]);
  // class and race
  } else if (race_index >= 0 && race_index < MAX_RACES && 
             class_index >= 0 && class_index < NUM_CLASSES) {
    race = TRUE;
    sprintf(info, "&1Spells and Skills for the &B%s&1, &B%s&1 sorted &b%s&n"
                 "\r\n\r\n", 
                 CLASS_NAME(class_index), pc_race_types[race_index], 
                 sort_names[sort_type]);
  // invalid
  } else {
    basic_mud_log("SYSERR: Invalid race (%d) or class (%d) passed to print_spells", 
                    race_index, class_index);
    return;
  }


  sprintf(info, "%s&1%-20s %-3s %s&n\r\n",
                      info, "Spell/Skill", "Lvl", "Requirements");

  for (spell_index = 1; spell_index < MAX_SKILLS; spell_index++) {
    if (sort_type == SORT_ALPHA)
      spellnum = spell_sort_info[SORT_ALPHA][0][spell_index];
    else 
      spellnum = spell_sort_info[sort_type][class_index][spell_index];

    if (spell_info[spellnum].min_level[class_index] == LVL_OWNER+1) 
      continue;

    // spellname - colour green if available, red otherwise
    if (!race || 
         race_has_stats_for_skill(race_index, class_index, spellnum, -1)) {
      sprintf(info, "%s&g", info);
    } else {
      sprintf(info, "%s&r", info);
    }

    sprintf(info, "%s%-20s&n", info, spell_info[spellnum].name);
    sprintf(info, "%s%-3d ", info, spell_info[spellnum].min_level[class_index]);
 
    /* Display the stat requirements */
    if (spell_info[spellnum].str[class_index] != 0) {
      sprintf(info, "%s&gStr: %s%2d%s ", info, 
          (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_STR)) ? 
            "&c" : "&r",
          spell_info[spellnum].str[class_index], CCNRM(ch,C_NRM));
    }
 
    if (spell_info[spellnum].intl[class_index] != 0) {
      sprintf(info, "%s&gInt: %s%2d%s ", info,
          (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_INT)) ? 
            "&c" : "&r",
          spell_info[spellnum].intl[class_index], CCNRM(ch,C_NRM));
    }
 
    if (spell_info[spellnum].wis[class_index] != 0) {
      sprintf(info, "%s&gWis: %s%2d%s ", info,
          (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_WIS)) ? 
            "&c" : "&r",
          spell_info[spellnum].wis[class_index], CCNRM(ch,C_NRM));
    }
 
    if (spell_info[spellnum].dex[class_index] != 0) {
      sprintf(info, "%s&gDex: %s%2d%s ", info, 
          (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_DEX)) ? 
            "&c" : "&r",
          spell_info[spellnum].dex[class_index], CCNRM(ch,C_NRM));
    }
 
    if (spell_info[spellnum].con[class_index] != 0) {
      sprintf(info, "%s&gCon: %s%2d%s ", info,
          (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_CON)) ? 
            "&c" : "&r",
          spell_info[spellnum].con[class_index], CCNRM(ch,C_NRM));
    }

    if (spell_info[spellnum].cha[class_index] != 0) {
      sprintf(info, "%s&gCha: %s%2d%s ", info,
          (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_CHA)) ? 
            "&c" : "&r",
          spell_info[spellnum].cha[class_index], CCNRM(ch,C_NRM));
    }

    sprintf(info, "%s\r\n", info);
  }
  page_string(ch->desc, info, TRUE);
}

void print_spell_info(struct char_data *ch, int spellnum, int race_index) {

  bool race = FALSE;
  int class_index;
  char info[MAX_STRING_LENGTH];
  extern struct spell_info_type spell_info[];

  if (spellnum <= 0 || spellnum > TOP_SPELL_DEFINE) {
    basic_mud_log("SYSERR: Invalid spellnum (%d) passed to print_spell_info", 
                    spellnum);
    return;
  }

  if (!str_cmp(spell_info[spellnum].name, unused_spellname)) {
    basic_mud_log("SYSERR: Unused spellnum (%d) passed to print_spell_info", 
                    spellnum);
    return;
  }

  if (race_index > MAX_RACES) {
    basic_mud_log("SYSERR: Invalid race number (%d) passed to print_spell_info", 
                    race_index);
    return;
  }

  if (race_index >= 0 && race_index < MAX_RACES)
    race = TRUE;

  sprintf(info, "Spell Name: &B%s&n", spell_info[spellnum].name);
  if (race) {
    sprintf(info, "%s for Race: &B%s&n (&rred&n is unmatched requirements)\r\n", 
                    info, pc_race_types[race_index]);  
  } else { 
    strcat(info, "\r\n");
  }

  for (class_index = 0; class_index < NUM_CLASSES; class_index++) {
    if (spell_info[spellnum].min_level[class_index] != LVL_OWNER+1) {

      if ((race_index < 0) || (race_index >= 0 && 
          race_has_stats_for_skill(race_index, class_index, spellnum, -1))) {
        sprintf(info, "%s&g", info);
      } else {
        sprintf(info, "%s&r", info);
      }

      sprintf(info, "%s%s:&n\r\nLevel: &M%d&n Mana_min: &G%d&n "
                    "Mana_max: &G%d&n Mana_change: &G%d&n\r\n",
              info, pc_class_types[class_index], 
              spell_info[spellnum].min_level[class_index],  
	      spell_info[spellnum].mana_min[class_index],  
	      spell_info[spellnum].mana_max[class_index],  
	      spell_info[spellnum].mana_change[class_index]);

      /* Display the stat requirements */
      if (spell_info[spellnum].str[class_index] != 0) {
        sprintf(info, "%s&gStr: %s%2d%s ", info, 
            (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_STR)) ? 
              "&c" : "&r",
            spell_info[spellnum].str[class_index], CCNRM(ch,C_NRM));
      }
 
      if (spell_info[spellnum].intl[class_index] != 0) {
        sprintf(info, "%s&gInt: %s%2d%s ", info,
            (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_INT)) ? 
              "&c" : "&r",
            spell_info[spellnum].intl[class_index], CCNRM(ch,C_NRM));
      }
 
      if (spell_info[spellnum].wis[class_index] != 0) {
        sprintf(info, "%s&gWis: %s%2d%s ", info,
            (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_WIS)) ? 
              "&c" : "&r",
            spell_info[spellnum].wis[class_index], CCNRM(ch,C_NRM));
      }
 
      if (spell_info[spellnum].dex[class_index] != 0) {
        sprintf(info, "%s&gDex: %s%2d%s ", info, 
            (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_DEX)) ? 
              "&c" : "&r",
            spell_info[spellnum].dex[class_index], CCNRM(ch,C_NRM));
      }
 
      if (spell_info[spellnum].con[class_index] != 0) {
        sprintf(info, "%s&gCon: %s%2d%s ", info,
            (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_CON)) ? 
              "&c" : "&r",
            spell_info[spellnum].con[class_index], CCNRM(ch,C_NRM));
      }

      if (spell_info[spellnum].cha[class_index] != 0) {
        sprintf(info, "%s&gCha: %s%2d%s ", info,
            (!race || race_has_stats_for_skill(race_index, class_index, spellnum, STAT_CHA)) ? 
              "&c" : "&r",
            spell_info[spellnum].cha[class_index], CCNRM(ch,C_NRM));
      }

      sprintf(info, "%s&n", info);

      sprintf(info, "%s\r\nClass Effeciency: &R%d&n%% "
                    "Class Mana Percentage: &R%d&n%%&n\r\n\r\n",
              info, 
	      spell_info[spellnum].spell_effec[class_index],
	      spell_info[spellnum].mana_perc[class_index]);
    }
  }
  page_string(ch->desc, info, TRUE); 
}

void print_sort_types(struct char_data *ch) {

  int i;
  char sorttypes[MAX_INPUT_LENGTH];

  sorttypes[0] = '\0';

  for (i = 0; i < NUM_SORT_TYPES; i++) {
    sprintf(sorttypes, "%s%s ", sorttypes, sort_names[i]);
  }
  send_to_char(sorttypes, ch);
}

// DM - TODO - fix for user levels
ACMD(do_spellinfo)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  char spellname[MAX_INPUT_LENGTH];
  int spellnum, i;
  int class_index, sort_type, race_index = -1;

  spellname[0] = '\0';
  arg1[0] = '\0';
  arg2[0] = '\0';
  arg3[0] = '\0';
  rest[0] = '\0';

  //s = strtok(argument, "'");
 
  //if (s == NULL) {
  if (!*argument)
  {
    if (LR_FAIL(ch, LVL_IMMORT))
      sprintf(buf,"&1Usage: &4%s &n<classname> [racename] [ <sort type> ]\r\n",
              CMD_NAME);
    else
      sprintf(buf,"&1Usage: &4%s &n { '<spellname>' | <classname> }\r\n"
                  "                 [racename] [ <sort type> ]\r\n", CMD_NAME);
    send_to_char(buf,ch);
    return;
  }

  skip_spaces(&argument);

  /* get: blank, spell name, target name */
  if (argument[0] == '\'')
  {
    *argument++;
    i = 0;
    do
    {
      spellname[i++] = *argument++;
    } while (!(*argument == '\0' || *argument == '\'')); 
    *argument++;
    spellname[i++] = '\0';
  }

  //s = strtok(NULL, "'");
  //if (s == NULL) {
  if (spellname[0] == '\0')
  {
    half_chop(argument, arg1, rest);
    class_index = search_block_case_insens(arg1, pc_class_types, FALSE);
    if (class_index >= 0)
    {
      half_chop(rest, arg2, rest);
      // spellinfo classname
      if (!*arg2)
        sort_type = SORT_ALPHA; 
      else if ((race_index = search_block_case_insens(arg2, pc_race_types, FALSE)) >= 0)
      { // spellinfo classname racename
        half_chop(rest, arg3, rest);
        if (!*arg3) // spellinfo classname racename sorttype
          sort_type = SORT_ALPHA;
        else if ((sort_type = search_block_case_insens(arg3, sort_names, FALSE)) < 0)
	{
          send_to_char("Invalid sort type. Valid types are: ", ch);
          print_sort_types(ch);
          return;
        }
      // spellinfo classname sorttype
      } else if ((sort_type = search_block_case_insens(arg2, sort_names, FALSE)) < 0)
      {
        send_to_char("Invalid race or sort type. Valid sort types are: ", ch);
        print_sort_types(ch);
        return;
      }
      print_spells(ch, class_index, race_index, sort_type);
      return;
    } else {
      send_to_char("Invalid Class Name.\r\n",ch);
      return;
    }
  } else {
    if (LR_FAIL(ch, LVL_IMMORT))
    {
      sprintf(buf,"&1Usage: &4%s &n<classname> [ <sort type> ]\r\n", CMD_NAME);
      send_to_char(buf,ch);
      return;
    }
    if ((spellnum = find_skill_num(spellname)) == -1)
    { 
      send_to_char("Spell/Skill name not found.\r\n",ch);
      return;
    }
    half_chop(argument, arg2, rest);
    if (*arg2)
    {
      race_index = search_block_case_insens(arg2, pc_race_types, FALSE);
      if (race_index < 0)
        send_to_char("Ignoring invalid race ...\r\n", ch);
    }
    print_spell_info(ch, spellnum, race_index);
  }
}



                
extern int num_rooms_burgled;
extern int vote_level;
/* extern functions */
void add_mud_event(struct event_data *ev);
int check_for_event(int event, zone_rnum zone);

/* local functions */
ACMD(do_event);

void destroy_event_election(struct char_data *ch, struct event_data *ev)
{
  if (ev->type != EVENT_ELECTION)
  {
    mudlog("SYSERR: Non election event passed to destroy_event_election().",
	   NRM, LVL_IMPL, TRUE);
    return;
  }
  send_to_all("&gThe election has ended. Polling results.&n\r\n");
  sprintf(buf, "Election ended by %s (Results: #1: %ld, #2: %ld, #3: %ld)",
	  ((ch) ? GET_NAME(ch) : "timeout"), ev->info1, ev->info2, ev->info3);
  mudlog(buf, NRM, LVL_GOD, TRUE);
  sprintf(buf, "Official results of election - #1: %ld, #2: %ld, #3: %ld.\r\n",
	  ev->info1, ev->info2, ev->info3);
  send_to_all(buf);
  ev->type=EVENT_OVER;
  remove_mud_event(ev);
  return;
}

void destroy_event_curfew(struct char_data *ch, struct event_data *ev)
{
  if (ev->type != EVENT_CURFEW)
  {
    mudlog("SYSERR: Non curfew event passed to destroy_event_curfew().", 
	   BRF, LVL_IMPL, TRUE);
    return;
  }
  sprintf(buf, "Curfew in %s lifted by %s.",
	  zone_table[ev->room->zone].name, ((ch) ? GET_NAME(ch) : "timeout"));
  send_to_zone("\r\n&b[ Curfew has been lifted! ]&n\r\n", ev->room->zone);
  mudlog(buf, BRF, LVL_GOD, TRUE);
  ev->type = EVENT_OVER;
  remove_mud_event(ev);
  return;
}

// Destroy and Clean Up after a goldrush.
void destroy_event_goldrush(struct char_data *ch, struct event_data *ev)
{
  struct obj_data *gold, *next_gold;
  if (ev->type != EVENT_GOLD_RUSH)
  {
    mudlog("SYSERR: Non goldrush event passed to destroy_event_goldrush().", 
	   BRF, LVL_IMPL, TRUE);
    return;
  }
  for (gold=object_list; gold; gold=next_gold)
  {
    next_gold = gold->next;
    if ((GET_OBJ_VNUM(gold) == GOLD_OBJ_VNUM) &&
	(gold->in_room >= 0) &&
	world[gold->in_room].zone == ev->room->zone)
      extract_obj(gold);
  }
  send_to_zone("\r\n&y[ The gold rush has ended. ]&n\r\n", ev->room->zone);
  sprintf(buf, "&WYou stop hearing of gold findings at %s.&n\r\n",
	  zone_table[ev->room->zone].name);
  send_to_not_zone_world(buf, ev->room->zone);
  sprintf(buf, "Goldrush in %s ended by %s.",
	  zone_table[ev->room->zone].name, ((ch) ? GET_NAME(ch) : "timeout"));
  mudlog(buf, NRM, LVL_GOD, TRUE);
  ev->type = EVENT_OVER;
  remove_mud_event(ev);
  return;
}

void destroy_event_happy(struct char_data *ch, struct event_data *ev)
{
  if (ev->type != EVENT_HAPPY_HR)
  {
    mudlog("SYSERR: Non happy hour event passed to destroy_event_happy().", 
	   NRM, LVL_IMPL, TRUE);
    return;
  }
  send_to_all("&WHappy hour is over now, kiddies.&n\r\n");
  sprintf(buf, "Happy hour ended by %s.", 
	  ((ch) ? GET_NAME(ch) : "grumpiness."));
  mudlog(buf, NRM, LVL_GOD, TRUE);
  ev->type = EVENT_OVER;
  remove_mud_event(ev);
}

void destroy_event_fire(struct char_data *ch, struct event_data *ev)
{
  if (ev->type != EVENT_FIRE)
  {
    mudlog("SYSERR: Non fire event passed to destroy_event_fire().",
	   BRF, LVL_IMPL, TRUE);
    return;
  }
  send_to_zone("&yThe fire is finally under control.&n\r\n", ev->room->zone);
  sprintf(buf, "&WYou hear rumours that the fire at %s has been"
	       "extinguished.&n\r\n", zone_table[ev->room->zone].name);
  send_to_not_zone_world(buf, ev->room->zone);
  sprintf(buf, "Fire in %s ended by %s.", zone_table[ev->room->zone].name, 
	  ((ch) ? GET_NAME(ch) : "lack of fuel."));
  mudlog(buf, NRM, LVL_GOD, TRUE);
  for (room_rnum room = 0; room <= top_of_world; room++)
    if (world[room].zone == ev->room->zone)
      REMOVE_BIT(RMSM_FLAGS(room), RMSM_BURNED);
  ev->type = EVENT_OVER;
  remove_mud_event(ev);
}

void perform_event_destroy(struct char_data *ch, char *arg)
{
  int evcount = 0, evno = 0;
  struct event_data *ev;
  
  if ((!*arg) || !is_number(arg))
  {
    send_to_char("Syntax: event destroy <event #> (Use &4event list&n for a list.\r\n", ch);
    return;
  }
  evno = atoi(arg);
  for (ev = events.list; ev; ev=ev->next)
  {
    evcount++;
    if (evcount == evno)
      switch (ev->type)
      {
	case EVENT_QUEST:
	  send_to_char("Use quest end, rather than destroy event.\r\n", ch);
	  return;
	case EVENT_CURFEW:    destroy_event_curfew(ch, ev);   return;
	case EVENT_GOLD_RUSH: destroy_event_goldrush(ch, ev); return;
	case EVENT_FIRE:      destroy_event_fire(ch, ev);     return;
	case EVENT_ELECTION:  destroy_event_election(ch, ev); return;
	case EVENT_HAPPY_HR:  destroy_event_happy(ch, ev);    return;
	case EVENT_BURGLARY:
	case EVENT_BOUNTY_HUNT:
	case EVENT_OVER:
	  send_to_char("That type of event cannot be destroyed.\r\n", ch);
	  return;
	default:
	  mudlog("SYSERR: Default case reached in perform_event_destroy().",
	         BRF, LVL_IMPL, FALSE);
	  return;
      }
  }
  send_to_char("Destroy which event? (&4event list&n for a list.)\r\n", ch);
}
void create_event_election(struct char_data *ch, char *arg)
{

  struct event_data *ev = NULL;
  struct descriptor_data *d;
  
  if (check_for_event(EVENT_ELECTION, -1))
  {
    send_to_char("There's already an election going on. Wait for the poll.\r\n",ch);
    return;
  }

  // Every player currently online, may vote
  for (d = descriptor_list; d; d = d->next)
  {
    if( STATE(d) != CON_PLAYING)
      continue;

    GET_VOTED(d->character) = FALSE;
  }

  CREATE(ev, struct event_data, 1);
  ev->chID = GET_IDNUM(ch);
  ev->room = NULL;
  ev->type = EVENT_ELECTION;
  ev->info1 = 0;
  ev->info2 = 0;
  ev->info3 = 0;

  add_mud_event(ev);

  if (*arg)
  {
    sprintf(buf1, "&G-ELECTION NOTICE-&g\r\n%s\r\n", arg); 
    send_to_all(buf1);
  }	
  sprintf(buf, "&gThe election has begun.\r\n&nType 'vote <number>' to register your vote.\r\n");
  send_to_all(buf);
  sprintf(buf, "Election initiated by %s", GET_NAME(ch));
  mudlog(buf, BRF, LVL_GOD, TRUE);
}

void create_event_curfew(struct char_data *ch, char *arg)
{

  struct event_data *ev = NULL;
  zone_rnum zone = world[ch->in_room].zone;

  if (check_for_event(EVENT_CURFEW, zone))
  {
    send_to_char("Curfew has already been imposed on this part of the land.\r\n",ch);
    return;
  }

  CREATE(ev, struct event_data, 1);
  ev->chID = GET_IDNUM(ch);
  ev->type = EVENT_CURFEW;
  ev->room = &world[ch->in_room];
  ev->info3 = get_id_by_name(GET_NAME(ch));

  add_mud_event(ev);

  // Let those in the zone know
  if (*arg)
  {
    sprintf(buf1, "&G-CURFEW NOTICE-&g\r\n%s\r\n", arg);
    send_to_zone(buf1, zone);
  }
  send_to_zone("&R\r\n[ Curfew has been imposed in this area. Seek shelter at night. ]&n\r\n", zone);
  // Log the event
  sprintf(buf, "Curfew imposed by %s within %s.", GET_NAME(ch), zone_table[zone].name);
  mudlog(buf, BRF, LVL_GOD, TRUE); 
}

void create_event_happy(struct char_data *ch)
{
  struct event_data *ev;

  if (check_for_event(EVENT_HAPPY_HR, -1))
  {
    if (ch && ch->desc)
      send_to_char("Happy hour is already happening!\r\n", ch);
    return;
  }
  CREATE(ev, struct event_data, 1);
  ev->type = EVENT_HAPPY_HR;
  ev->info1 = 3600;
  if (ch)
  {
    ev->chID = GET_IDNUM(ch);
    ev->room = &world[IN_ROOM(ch)];
    sprintf(buf, "(GC) Happy hour initiated by %s.", GET_NAME(ch));
  } else {
    ev->chID = NOBODY;
    ev->room = NULL;
    sprintf(buf, "Happy hour initiated by Haven's residents.");
  }
  mudlog(buf, NRM, LVL_GOD, TRUE); 
  add_mud_event(ev);
  send_to_all("&WHappy hour has been declared! DOUBLE EXP!!!&n\r\n");
}


void create_event_fire(struct char_data *ch, char *arg)
{
  struct event_data *ev;
  zone_rnum zone = world[IN_ROOM(ch)].zone;

  if (check_for_event(EVENT_FIRE, zone))
  {
    send_to_char("There's already a fire in this part of the land!\r\n", ch);
    return;
  }
  if (SECT(IN_ROOM(ch)) > SECT_MOUNTAIN)
  {
    send_to_char("There's no way you're going to burn anything here!\r\n", ch);
    return;
  }
  if (world[IN_ROOM(ch)].number == 1200)
  {
    send_to_char("This room seems to be impervious to fire!\r\n", ch);
    return;
  }
  CREATE(ev, struct event_data, 1);
  ev->chID = GET_IDNUM(ch);
  ev->type = EVENT_FIRE;
  ev->room = &world[IN_ROOM(ch)];
  add_mud_event(ev);

  sprintf(buf, "&Y[ Look out!! %s is on FIRE! ]&n\r\n", zone_table[zone].name);
  send_to_zone(buf, zone);
  sprintf(buf, "&WYou hear of a fire in %s!&n\r\n", zone_table[zone].name);
  send_to_not_zone_world(buf, zone);
  sprintf(buf, "Fire initiated by %s at %s.", GET_NAME(ch), zone_table[zone].name);
  mudlog(buf, BRF, LVL_GOD, TRUE);
}

void create_event_goldrush(struct char_data *ch, char *arg) 
{
  struct event_data *ev;

  zone_rnum zone = world[ch->in_room].zone;

  if(check_for_event(EVENT_GOLD_RUSH, zone))
  {
    send_to_char("There's already a gold rush in this part of the land.\r\n",ch);
    return;
  }

  CREATE(ev, struct event_data, 1);
  ev->chID = GET_IDNUM(ch);
  ev->type = EVENT_GOLD_RUSH;
  ev->room = &world[ch->in_room];

  add_mud_event(ev);
	
  // Register the event
  sprintf(buf, "&Y[ A GOLD RUSH has just started in %s! ]&n\r\n", zone_table[zone].name);
  send_to_zone(buf, zone);
  sprintf(buf, "&WYou hear rumours of a gold rush at %s!&n\r\n", zone_table[zone].name);
  send_to_not_zone_world(buf, zone);
  sprintf(buf, "Gold rush initiated by %s at %s.", GET_NAME(ch), zone_table[zone].name);
  mudlog(buf, BRF, LVL_GOD, TRUE);
}

void list_events_to_char(struct char_data *ch, int specific) 
{
  struct event_data *ev;
  char *target, *type, *info1, *info2, *info3, *roomdesc, *room;
  int evcounter = 0, found = FALSE;

  for (ev = events.list; ev; ev = ev->next)
  {
    evcounter++;
    if (specific != evcounter && specific != -1)
      continue;
    if (specific != -1)
      found = TRUE;
    if( ev->type == EVENT_OVER )
      continue;

    switch(ev->type)
    {
      case EVENT_BURGLARY:
	target = "Burglar: ";
	type = "&rBurglary&n";
	roomdesc = "First room: ";
	room = ev->room->name;
	info1 = "Codes: ";
	info2 = "Direction: ";
	info3 = "Loot so far: ";
	break;
      case EVENT_BOUNTY_HUNT:
	target = "Victim: ";
	type = "&bBounty Hunt&n";
	roomdesc = "Return to: ";
	room = ev->room->name;
	info1 = "Reward: ";
	info2 = "Bounty ID: ";
	info3 = "Initiator ID: ";
	break;
      case EVENT_GOLD_RUSH:
	target = "Initiator: ";
	type = "&yGold rush&n";
	roomdesc = "Sampled room: ";
	room = ev->room->name;
	info1 = "Secs Running: ";
	info2 = "Chunks Made: ";
	info3 = "Unused: ";
	break;
      case EVENT_FIRE:
	target = "Firebug: ";
	type = "&rFIRE!&n";
	roomdesc = "Sampled room: ";
	room = ev->room->name;
	info3 = info2 = info1 = "Unused: ";
	break;
      case EVENT_CURFEW:
	target = "Initiated by: ";
	type = "&RCurfew&n";
	roomdesc = "Sampled room: ";
	room = ev->room->name;
	info1 = info2 = "Unused: ";
	info3 = "Initiator ID: ";
	break;
      case EVENT_ELECTION:
	target = "Initiated by: ";
	type = "&BElection &n";
	roomdesc = "Room: ";
	room = "N/A";
	info1 = "Votes for #1: ";
	info2 = "Votes for #2: ";
	info3 = "Votes for #3: ";
	break;
      case EVENT_QUEST:
	target = "Created by: ";
	type = "&GQuest&n";
	roomdesc = "Sampled room: ";
	room = ev->room->name;
	info1 = "Quest Type: ";
	info2 = "Sample: ";
	info3 = "Reward: ";
	break;
      case EVENT_HAPPY_HR:
	target = "Created By: ";
	type = "&GHappy Hour&n";
	roomdesc = "Room: ";
	room = "N/A";
	info1 = "Seconds Remaining: ";
	info3 = info2 = "Unused: ";
	break;
      default:
	target = "Character: ";
	type = "Other";
	roomdesc = "Roomname: ";
	if (ev->room != NULL)
	  room = ev->room->name;
	else
	  room = "N/A";
	info1 = "Info1: ";
	info2 = "Info2: ";
	info3 = "Info3: "; 
	break;				
    }

    sprintf(buf, "&g(%2d)&n %s\r\n"
		 "     %s%s\r\n"
		 "     %s%d - %s\r\n"
		 "     %s%ld, %s%ld, %s%ld\r\n",
	    evcounter, type, 
	    target, (ev->chID != -1 ? get_name_by_id(ev->chID) : "Noone"),
	    roomdesc, ev->room != NULL ? ev->room->number : -1, room,
	    info1, ev->info1, info2, ev->info2, info3, ev->info3); 			    
    send_to_char(buf, ch);
  }

  // Sort out an appropriate message 
  if (specific != -1)
  {
    if (!found)
      sprintf(buf, "That event does not exist!\r\n");
    else
      sprintf(buf, "&gEvent found.&n\r\n");
  } else if( evcounter != 0 ) {
    sprintf(buf, "&g%d event%s listed.&n\r\n", evcounter, evcounter == 1 ? "" : "s");
  } else {
    sprintf(buf, "No events exist.\r\n");
  }
  send_to_char(buf, ch);		
	
  if (burglaries != NULL)
    send_to_char("&RBurglaries-&n\r\n", ch);
  // List burglaries 
  Burglary *tmpBurglary = burglaries;
  while (tmpBurglary != NULL)
  {
    tmpBurglary->DescribeSelf(ch);
    tmpBurglary = tmpBurglary->next;
  }
}

/* Command to control events as well as list them */
ACMD(do_event)
{
  if (IS_NPC(ch)) 
    return;

  half_chop(argument, arg, buf1);
  //two_arguments(argument, arg, buf1);
	
  if (!*arg)
  {
    send_to_char("Usage: event <list [event #] | create | destroy>\r\n", ch);
    return;
  }

  arg[0] = LOWER(arg[0]);

  if (is_abbrev(arg, "list")) 
  {
    if (!*buf1) 
    {
      list_events_to_char(ch, -1);
      return;
    } else if(!isdigit(buf1[0])) {
      send_to_char("You may either list a single event, or all.\r\n"
		   "Usage: event <list | list #>\r\n", ch);
      return;
    }
    if (atoi(buf1) < 1)
    {
      send_to_char("You must specify a positive event number!\r\n", ch);
      return;
    }
    list_events_to_char(ch, atoi(buf1));
    return;
  }

  if (is_abbrev(arg, "create")) 
  {
    half_chop(buf1, arg, buf2);
    if (!*arg) 
    {
      send_to_char("Usage: event create <curfew | goldrush"
		   " | election | fire | happyhr>\r\n",ch);
      return;
    }
    if (is_abbrev(arg, "curfew"))
    {
      create_event_curfew(ch, buf2);			
      return;
    }
    if (is_abbrev(arg, "goldrush"))
    {
      create_event_goldrush(ch, buf2);
      return;
    }
    if(is_abbrev(arg, "election"))
    {
      create_event_election(ch, buf2);
      return;
    }
    if (is_abbrev(arg, "fire")) 
    {
      create_event_fire(ch, buf2);
      return;
    }
    if (is_abbrev(arg, "happyhr"))
    {
      create_event_happy(ch);
      return;
    }
    send_to_char("Usage: event create <curfew | goldrush |"
		 " election | fire> <statement(optional)>\r\n",ch);
    return;
  }

  if (is_abbrev(arg, "destroy")) 
  {
    half_chop(buf1, arg, buf2);
    perform_event_destroy(ch, arg);
    return;
  }
  
  send_to_char("That is NOT a valid option!\r\n", ch);
  return;
/*
    half_chop(buf1, arg, buf2);
    if (!*arg)
    {
      send_to_char("Usage: event destroy <curfew | goldrush |" 
		   " quest | election | fire>\r\n",ch);
      return;
    }
    if (is_abbrev(arg, "curfew")) 
    {
      destroy_event_curfew(ch);
      return;
    }
    if (is_abbrev(arg, "goldrush"))
    {
      destroy_event_goldrush(ch);
      return;
    }
    if (is_abbrev(arg, "happyhr"))
    {
      destroy_event_happy(ch);
      return;
    }
    if (is_abbrev(arg, "quest"))
    {
      destroy_event_quest(ch, buf2);
      return;
    }
    if (is_abbrev(arg, "election"))
    {
      destroy_event_election(ch);
      return;
    }
    if (is_abbrev(arg, "fire"))
    {
      destroy_event_fire(ch, world[IN_ROOM(ch)].zone);
      return;
    }
    send_to_char("Usage: event destroy <curfew | goldrush |"
		 " quest | election | fire>\r\n",ch);
    return;
  }
  send_to_char("That is NOT a valid option!\r\n", ch);
*/
}
