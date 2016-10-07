/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <fcntl.h>  // open() used in john_in, cassandra_in 

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

/*   external vars  */
extern const float race_modifiers[];
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
extern char *class_abbrevs[];
extern time_t boot_time;
extern zone_rnum top_of_zone_table;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern int top_of_p_table;
extern const float class_modifiers[NUM_CLASSES];
extern int spell_sort_info[NUM_SORT_TYPES][NUM_CLASSES][MAX_SKILLS+1]; 
extern const char *special_ability_bits[];
extern int num_of_houses;
extern struct house_control_rec house_control[];

/* for chars */
extern const char *pc_class_types[];
extern const char *pc_race_types[];

/* extern functions */
int level_exp(struct char_data *ch, int level);
void show_shops(struct char_data * ch, char *value);
void hcontrol_list_houses(struct char_data *ch);
void do_start(struct char_data *ch);
void appear(struct char_data *ch);
void reset_zone(zone_rnum zone);
void roll_real_abils(struct char_data *ch);
int parse_class(char *arg);
void remove_class_specials(struct char_data *ch);
void set_class_specials(struct char_data *ch);
void set_race_specials(struct char_data *ch);
void apply_specials(struct char_data *ch, bool initial);

/* local functions */
int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
void perform_immort_invis(struct char_data *ch, int level);
ACMD(do_echo);
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
void print_zone_to_buf(char *bufptr, zone_rnum zone);
ACMD(do_show);
ACMD(do_set);

/* Primal Functions */
void john_in(struct char_data *ch);
void cassandra_in(struct char_data *ch);
ACMD(do_demort);
ACMD(do_immort);
ACMD(do_pinch);
ACMD(do_pkset);
ACMD(do_queston);
ACMD(do_questoff);
ACMD(do_skillshow);
ACMD(do_tic);

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
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}


ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Send what to who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
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

  if (!*roomstr) {
    send_to_char("You must supply a room number or name.\r\n", ch);
    return (NOWHERE);
  }
  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return (NOWHERE);
    }
  } else if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)) != NULL)
    location = target_mob->in_room;
  else if ((target_obj = get_obj_vis(ch, roomstr)) != NULL) {
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("That object is not available.\r\n", ch);
      return (NOWHERE);
    }
  } else {
    send_to_char("No such creature or object around.\r\n", ch);
    return (NOWHERE);
  }

  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (GET_LEVEL(ch) < LVL_GRGOD) {
    if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return (NOWHERE);
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	world[location].people && world[location].people->next_in_room) {
      send_to_char("There's a private conversation going on in that room.\r\n", ch);
      return (NOWHERE);
    }
    if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
	!House_can_enter(ch, GET_ROOM_VNUM(location))) {
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
  if(world[location].number >= 500 && world[location].number <= 599 &&
     GET_LEVEL(ch) < LVL_GOD) {
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
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  room_rnum location;
  struct char_data *target_mob;
  char name_mob[MAX_INPUT_LENGTH];
  extern int allowed_zone(struct char_data * ch,int flag);
  extern int allowed_room(struct char_data * ch,int flag);

  if (PRF_FLAGGED(ch, PRF_MORTALK) && GET_LEVEL(ch)<LVL_GOD){
        send_to_char("You cannot goto out of mortalk arena.  Use Recall!\r\n", ch);
        return;
  }
  if (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_IMMORT)
  {
    one_argument (argument,name_mob);
    if(((target_mob = get_char_vis(ch,name_mob,FALSE)) == NULL)
      || (world[target_mob->in_room].zone == GOD_ROOMS_ZONE)
      || (!same_world(target_mob,ch)))
    {
      send_to_char("That person does not appear to be anywhere!",ch);
      return;
    }
    if(IS_NPC(target_mob))
    {
      send_to_char("You can only go to Player Characters!",ch);
      return;
    }
  } 

  if ((location = find_target_room(ch, argument)) < 0)
    return;

/* stops imms going to reception - Vader */
  if(world[location].number >= 500 && world[location].number <= 599 &&
     GET_LEVEL(ch) < LVL_GOD) {
  send_to_char("As you fly through time and space towards the reception area\r\n"
               "a guardian spirit tells you in a nice kind voice, \"PISS OFF!\"\r\n",ch);
    return;
    }
 
/* stops imms going to clan halls - Hal*/
 
  if(world[location].number >= 103 && world[location].number <= 150 && GET_LEVEL(ch) < LVL_GOD)
  {
    send_to_char("As you fly through time and space towards the clan area\r\n"
               "a guardian spirit tells you in a nice kind voice,\"umm no sorry can't go there\"\r\n",ch);
    return;
  }  

  if (!allowed_zone(ch,zone_table[world[location].zone].zflag))
    return;
  if (!allowed_room(ch,world[location].room_flags))
    return; 
/*
  if (POOFOUT(ch))
    sprintf(buf, "$n %s", POOFOUT(ch));
  else
    strcpy(buf, "$n disappears in a puff of smoke.");
 */
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);

  if (!strcmp(ch->player.name, "John"))
    john_in(ch);
  else
    if (!strcmp(ch->player.name, "Cassandra"))
      cassandra_in(ch);
    else { 
/*
      if (POOFIN(ch))
        sprintf(buf, "$n %s", POOFIN(ch));
      else
        strcpy(buf, "$n appears with an ear-splitting bang.");
  
      act(buf, TRUE, ch, 0, 0, TO_ROOM); */
    }
  look_at_room(ch, 0);
}



ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to transfer?\r\n", ch);
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
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
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char("I think not.\r\n", ch);
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
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
  else if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
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
  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj"))) {
    send_to_char("Usage: vnum { obj | mob } <name>\r\n", ch);
    return;
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

  sprintf(buf, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
	  CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], Type: %s\r\n",
	  zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
	  CCNRM(ch, C_NRM), ch->in_room, buf2);
  send_to_char(buf, ch);

  sprintbit(rm->room_flags, room_bits, buf2);
  sprintf(buf, "SpecProc: %s, Flags: %s\r\n",
	  (rm->func == NULL) ? "None" : "Exists", buf2);
  send_to_char(buf, ch);

  send_to_char("Description:\r\n", ch);
  if (rm->description)
    send_to_char(rm->description, ch);
  else
    send_to_char("  None.\r\n", ch);

  if (rm->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  sprintf(buf, "Chars present:%s", CCYEL(ch, C_NRM));
  for (found = 0, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;
    sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
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
    sprintf(buf, "Contents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;
      sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
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
}

// DM - TODO check if we need to add other objects ....
void do_list_obj_values(struct obj_data *j, char * buf)
{
  switch (GET_OBJ_TYPE(j)) {

  case ITEM_LIGHT:
    sprintf(buf, "Color: [%d], Type: [%d], Hours: [%d]",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    sprintf(buf, "Spells: %s (%d), %s (%d), %s (%d), %s (%d)",
		skill_name(GET_OBJ_VAL(j,0)), GET_OBJ_VAL(j, 0),
                skill_name(GET_OBJ_VAL(j,1)), GET_OBJ_VAL(j, 1), 
		skill_name(GET_OBJ_VAL(j, 2)), GET_OBJ_VAL(j, 2), 
		skill_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprintf(buf, "Spell: %s (%d), Mana: %d", skill_name(GET_OBJ_VAL(j, 0)), GET_OBJ_VAL(j, 0),
            GET_OBJ_VAL(j, 1));
    break;
  case ITEM_FIREWEAPON:
  case ITEM_WEAPON:
    sprintf(buf, "Tohit: %d, Todam: %dd%d, Type: %d", GET_OBJ_VAL(j, 0),
            GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_MISSILE:
    sprintf(buf, "Tohit: %d, Todam: %d, Type: %d", GET_OBJ_VAL(j, 0),
            GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_ARMOR:
    sprintf(buf, "AC-apply: [%d]", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_TRAP:
    sprintf(buf, "Loaded: %s, Damage: %dd%d, Protecting Item: %d",
      (GET_OBJ_VAL(j, 0) == 0 ? "No" : (GET_OBJ_VAL(j, 0) == 1 ? "Yes" : "Yes, Room")),
      GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break; 
  case ITEM_CONTAINER:
    sprintf(buf, "Max-contains: %d, Locktype: %d, Corpse: %s",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No");
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN: 
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
    sprintf(buf, "Max-contains: %d, Contains: %d, Poisoned: %s, Liquid: %s",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No", buf2);
    break;
  case ITEM_NOTE:
    sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    sprintf(buf, "Keytype: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_FOOD:
    sprintf(buf, "Makes full: %d, Poisoned: %d",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 3));
    break;
  default:
    sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
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
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s %+d to %s", found++ ? "," : "",
	      j->affected[i].modifier, buf2);
      send_to_char(buf, ch);
    }
  if (!found)
    send_to_char(" None", ch);

  send_to_char("\r\n", ch);
}


void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;
  char buf3[MAX_STRING_LENGTH];
  int ETERNAL;
  struct assisters_type *assisters; 

  ETERNAL = (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_ETRNL9);   

  sprinttype(GET_SEX(k), genders, buf);
  sprintf(buf2, " %s '%s'  IDNum: [%5ld], In room [%5d]",
	  (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	  GET_NAME(k), GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)));

  if (!IS_NPC(k)) {
    if( GET_INVIS_TYPE(ch) == -2 )
      sprintf(buf3,", Invis to [%s%s%s]",CCYEL(ch, C_NRM),get_name_by_id(GET_INVIS_LEV(k)),CCNRM(ch,C_NRM));
    else if( GET_INVIS_TYPE(ch) == -1 )
      sprintf(buf3,", Invis to Lvl [%s%d%s]",CCYEL(ch,C_NRM),GET_INVIS_LEV(k),CCNRM(ch,C_NRM));
    else if( GET_INVIS_TYPE(ch) == 0 )
      sprintf(buf3,", Invis Lvl [%s%d%s]",CCYEL(ch,C_NRM),GET_INVIS_LEV(k),CCNRM(ch,C_NRM));
    else
      sprintf(buf3,", Invis to Lvls [%s%d-%d%s]",CCYEL(ch,
            C_NRM),GET_INVIS_LEV(k),GET_INVIS_TYPE(ch), CCNRM(ch,C_NRM));
 
    strcat(buf2,buf3);
  }

  strcat(buf2,"\r\n");
  send_to_char(strcat(buf, buf2), ch);

  if (IS_MOB(k)) {
    sprintf(buf, "Alias: %s, VNum: [%5d], RNum: [%5d]\r\n",
	    k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    send_to_char(buf, ch);
  }

  sprintf(buf, "Title: %s\r\n", (k->player.title ? k->player.title : "<None>"));
  send_to_char(buf, ch);

  sprintf(buf, "L-Des: %s", (k->player.long_descr ? k->player.long_descr : "<None>\r\n"));
  send_to_char(buf, ch);

  if (IS_NPC(k)) {	/* Use GET_CLASS() macro? */
    strcpy(buf, "Monster Class: ");
    sprinttype(/*k->player.class*/GET_CLASS(k), npc_class_types, buf2);
  } else {
    sprinttype(GET_CLASS(k), pc_class_types, buf2);
    sprinttype(GET_RACE(k), pc_race_types, buf3); 
    sprintf(buf, "Class: &B%s&n, Race: &B%s&n, Exp Modifier: [&R%.3f&n]\r\n", buf2, buf3,GET_MODIFIER(k));
  }
  strcat(buf, buf2);


  if (IS_NPC(k)) {
  sprintf(buf2, ", Lev: [%s%2d%s], XP: [%s%8d%s], Align: [%4d]\r\n",
          CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
          CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
          GET_ALIGNMENT(k));
  } else {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      sprintf(buf2, ", Lev: [%s%3d%s], XP: [%s%8d%s], XP_NL: [%s%8d%s], Align: [%4d]\r\n",
          CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
          CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
          CCYEL(ch, C_NRM), level_exp(ch,GET_LEVEL(k))-GET_EXP(k), CCNRM(ch, C_NRM),
          GET_ALIGNMENT(k));
    else
      sprintf(buf2, ", Lev: [%s%3d%s], XP: [%s%8d%s], Align: [%4d]\r\n",
          CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
          CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
          GET_ALIGNMENT(k));

  } 
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
    strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
    buf1[10] = buf2[10] = '\0';

    sprintf(buf, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
	    buf1, buf2, k->player.time.played / 3600,
	    ((k->player.time.played % 3600) / 60), age(k)->year);
    send_to_char(buf, ch);

    sprintf(buf, "Hometown: [%d], Speaks: [%d/%d/%d], (STL[%d]/per[%d]/NSTL[%d])",
         k->player.hometown, GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2),
            GET_PRACTICES(k), int_app[GET_INT(k)].learn,
            wis_app[GET_WIS(k)].bonus);

    /*. Display OLC zone for immorts .*/
    if (GET_LEVEL(k) >= LVL_IMMORT)
      sprintf(buf + strlen(buf), ", OLC[%d]", GET_OLC_ZONE(k));
    strcat(buf, "\r\n");

    send_to_char(buf, ch);
    
    sprintf(buf,"Start Rooms: [%d, %d, %d]\r\n",ENTRY_ROOM(ch,WORLD_MEDIEVAL),
        ENTRY_ROOM(ch,WORLD_WEST), ENTRY_ROOM(ch, WORLD_FUTURE));
    send_to_char(buf,ch); 
  }
  sprintf(buf, "Str: &C%d/%d&n(&c%d/%d&n) Int: &C%d&n(&c%d&n) Wis: &C%d&n(&c%d&n) "
	  "Dex: &C%d&n(&c%d&n) Con: &C%d&n(&c%d&n) Cha: &C%d&n(&c%d&n)\r\n",
	  GET_AFF_STR(k), GET_AFF_ADD(k), GET_REAL_STR(k), GET_REAL_ADD(k), 
	  GET_AFF_INT(k), GET_REAL_INT(k), 
	  GET_AFF_WIS(k), GET_REAL_WIS(k), 
	  GET_AFF_DEX(k), GET_REAL_DEX(k), 
	  GET_AFF_CON(k), GET_REAL_CON(k), 
	  GET_AFF_CHA(k), GET_REAL_CHA(k));
  send_to_char(buf, ch);

  if (!ETERNAL) {
    sprintf(buf, "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
	  CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprintf(buf, "Coins: [&Y%9d&n], Bank: [&Y%9d&n] (Total: &Y%d&n)\r\n",
	  GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));
    send_to_char(buf, ch);

    sprintf(buf, "AC: [%d%+d/10], Hitroll: [&r%2d&n], Damroll: [&r%2d&n], Saving throws: [%d/%d/%d/%d/%d]\r\n",
	  GET_AC(k), dex_app[GET_DEX(k)].defensive, k->points.hitroll,
	  k->points.damroll, GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2),
	  GET_SAVE(k, 3), GET_SAVE(k, 4));
    send_to_char(buf, ch);
  }

  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "Pos: %s, Fighting: %s", buf2,
	  (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));

  if (MOUNTING(k)) {
	sprintf(buf2, ", Mounted on: %s", (IS_NPC(k) ? "Yes" : GET_NAME(MOUNTING(k))) );
	strcat(buf, buf2);
  }
  if (MOUNTING_OBJ(k)) {
	sprintf(buf2, ", Mounted on: %s", MOUNTING_OBJ(k)->short_description);
	strcat(buf, buf2);
  }
  if (IS_NPC(k)) {
    strcat(buf, ", Attack type: ");
    strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
  }
  if (k->desc) {
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

  if (IS_NPC(k)) {
    sprintbit(MOB_FLAGS(k), action_bits, buf2);
    sprintf(buf, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
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
/*    sprintf(buf, "TIMERS: [");
    for (i=0; i < MAX_TIMERS; i++) {
      if (i != (MAX_TIMERS - 1))
        sprintf(buf2," %d,", TIMER(k,i));
      else
        sprintf(buf2," %d]\r\n", TIMER(k,i));
      strcat(buf,buf2);
    }
    send_to_char(buf,ch); */
    send_to_char(buf, ch);
   
  }
  if (!IS_NPC(k) && IS_SET(GET_SPECIALS(k), SPECIAL_DISGUISE)) {
	sprintf(buf, "Mob vnum memorised: %ld, Disguised: %s.\r\n", CHAR_MEMORISED(k), 
		CHAR_DISGUISED(k) == 0 ? "No" : "Yes");
	send_to_char(buf, ch);
  }
  if (IS_MOB(k)) {
    sprintf(buf, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
	    (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice);
    send_to_char(buf, ch);
  }
  sprintf(buf, "Carried: weight: %d, items: %d; ",
	  IS_CARRYING_W(k), IS_CARRYING_N(k));

  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
  sprintf(buf + strlen(buf), "Items in: inventory: %d, ", i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  sprintf(buf2, "eq: %d\r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    sprintf(buf, "Hunger: %d, Thirst: %d, Drunk: %d\r\n",
	  GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
    send_to_char(buf, ch);
  }

  sprintf(buf, "Master is: %s, Followers are:",
	  ((k->master) ? GET_NAME(k->master) : "<none>"));

  for (fol = k->followers; fol; fol = fol->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
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
 
  for (assisters=k->autoassisters; assisters; assisters=assisters->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(assisters->assister, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (assisters->next)
        send_to_char(strcat(buf, ",\r\n"), ch);
      else
        send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }
 
  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);

  /* DM - clan info */
  if (!IS_NPC(k)) {
    i = GET_CLAN_NUM(k);
 
    if (GET_LEVEL(ch) >= LVL_ANGEL)
      if ((i >= 0) && (i < CLAN_NUM)) {
        sprintf(buf, "Clan: &B%s&n, Rank: &B%s&n\r\n",
            clan_info[i].disp_name, clan_info[i].ranks[GET_CLAN_LEV(k)]);
        send_to_char(buf,ch);
      }
  }

  /* Showing the bitvector */
  sprintbit(AFF_FLAGS(k), affected_bits, buf2);
  sprintf(buf, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      *buf2 = '\0';
      if( aff->duration == -1 ) {
        sprintf(buf, "ABL: (Unlim) &c%-21s &n ", skill_name(aff->type));
        if (aff->modifier) {
          sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
          strcat(buf, buf2);
        }
      }
      else {
        sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1,
              CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));
        if (aff->modifier) {
          sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
          strcat(buf, buf2);
        }
      }      
      if (aff->bitvector) {
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
}


ACMD(do_stat)
{
  struct char_data *victim;
  struct obj_data *object;
  struct char_file_u tmp_store;
  int ETERNAL, tmp;

  half_chop(argument, buf1, buf2);

  ETERNAL = (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_ETRNL9);
  if (!*buf1) {
    send_to_char("Stats on who or what?\r\n", ch);
    return;
  } else if(ETERNAL) {
    if((victim = get_char_room_vis(ch,buf1)))
      do_stat_character(ch,victim);
    else send_to_char("That person does not appear to be here.",ch);
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char("Stats on which mobile?\r\n", ch);
    else {
      if ((victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char("No such mobile around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char("No such player around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1) {
	store_to_char(&tmp_store, victim);
	victim->player.time.logon = tmp_store.last_logon;
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
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char("Stats on which object?\r\n", ch);
    else {
      if ((object = get_obj_vis(ch, buf2)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char("No such object around.\r\n", ch);
    }
  } else {
    if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)) != NULL)
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room].contents)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, buf1)) != NULL)
      do_stat_object(ch, object);
    else
      send_to_char("Nothing around by that name.\r\n", ch);
  }
}


ACMD(do_shutdown)
{
  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("If you want to shut something down, say so!\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
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
  else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
    send_to_char("No such person around.\r\n", ch);
  else if (!victim->desc)
    send_to_char("There's no link.. nothing to snoop.\r\n", ch);
  else if (victim == ch)
    stop_snooping(ch);
  /* only IMPS can snoop players in private rooms BM 3/95 */
  else if (!(GET_LEVEL(ch)>=LVL_IMPL) && IS_SET(world[victim->in_room].room_flags,ROOM_HOUSE))
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
  else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
    send_to_char("No such character.\r\n", ch);
  else if (ch == victim)
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
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
    send_to_char("Usage: load { obj | mob } <number>\r\n", ch);
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
    char_to_room(mob, ch->in_room);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    if (load_into_inventory)
      obj_to_char(obj, ch);
    else
      obj_to_room(obj, ch->in_room);
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
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
    send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
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

  if (*buf) {			/* argument supplied. destroy single object
				 * or char */
    if ((vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)) != NULL) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
	send_to_char("Fuuuuuuuuu!\r\n", ch);
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents)) != NULL) {
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

    for (vict = world[ch->in_room].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict) && !MOUNTING(vict))
	extract_char(vict);
    }

    for (obj = world[ch->in_room].contents; obj; obj = next_o) {
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
    send_to_char("Usage: syslog { Off | Brief | Normal | Complete }\r\n", ch);
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
  char *name = arg, *level = buf2;
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
      send_to_char("That player is not here.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Advance who?\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("Maybe that's not such a great idea.\r\n", ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("NO!  Not on NPC's.\r\n", ch);
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char("That's not a level!\r\n", ch);
    return;
  }
  if (newlevel > LVL_IMPL) {
    sprintf(buf, "%d is the highest possible level.\r\n", LVL_IMPL);
    send_to_char(buf, ch);
    return;
  }
  if (newlevel > GET_LEVEL(ch)) {
    send_to_char("Yeah, right.\r\n", ch);
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char("They are already at that level.\r\n", ch);
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    send_to_char("You have been demoted!!!. Serves you RIGHT!\r\n", victim);
    demote_level(victim, newlevel);
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
//    log("(GC) %s demoted %s from level %d to %d.",
//		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
//  else
//    log("(GC) %s has advanced %s to level %d (from %d)",
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
  else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
    send_to_char(NOPERSON, ch);
  else {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

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
       if ( isdigit(buf1[0]) ) {
          toplevel = atoi(buf1);
          if (toplevel <= level || toplevel >= GET_LEVEL(ch)) {
             send_to_char("The level range is invalid.\r\n", ch);
             return;
          } // Valid range?
          else
             GET_INVIS_TYPE(ch) = toplevel;
       } // Specified a range?
       else { /* Specified a particular invisibility level? */
          if( strcmp(buf1, "single") == 0 ) {
             GET_INVIS_TYPE(ch) = INVIS_SINGLE;
          }
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
    send_to_char("Usage: DC <user number> (type USERS for a list)\r\n", ch);
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
    log("(GC) Connection closed by %s.", GET_NAME(ch));
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
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

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
  sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
	  chdata.char_specials_saved.idnum, (int) chdata.level,
	  class_abbrevs[(int) chdata.chclass], chdata.name, chdata.host,
	  ctime(&chdata.last_logon));
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
  sprintf(buf, "%s was last on: %s\r\n",chdata.name,ctime(&chdata.last_logon));
  send_to_char(buf, ch);
} 

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char("Whom do you wish to force do what?\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
      send_to_char(NOPERSON, ch);
    else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char("No, no, no!\r\n", ch);
    else {
      send_to_char(OK, ch);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
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
      send_to_char("Usage: immnet <text> | #<level> <text> | *<emotetext> |\r\n"
                   "       immnet @<level> *<emotetext> | imm @\r\n",ch);
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
      if (level > GET_LEVEL(ch)) {
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
 
  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && (GET_LEVEL(d->character) >= level) &&
        ((!PRF_FLAGGED(d->character, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
        (subcmd == SCMD_IMMNET && !PRF_FLAGGED(d->character,PRF_NOIMMNET))) &&
        (!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING))
        && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      if(subcmd == SCMD_IMMNET)
        send_to_char(CCBMAG(d->character, C_NRM), d->character);
      else if(subcmd == SCMD_ANGNET)
        send_to_char(CCBBLU(d->character, C_NRM), d->character);
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
  if (*arg == '*') {
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
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Yes, but for whom?!?\r\n", ch);
  else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
    send_to_char("There is no such player.\r\n", ch);
  else if (IS_NPC(vict))
    send_to_char("You can't do that to a mob!\r\n", ch);
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    send_to_char("Hmmm...you'd better not.\r\n", ch);
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char("Rerolled...\r\n", ch);
      roll_real_abils(vict);
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      sprintf(buf, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	      GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      send_to_char(buf, ch);
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
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
      sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Your victim is already pretty cold.\r\n", ch);
	return;
      }
      SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
      send_to_char("Frozen.\r\n", ch);
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
	   GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	send_to_char(buf, ch);
	return;
      }
      sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
      send_to_char("Thawed.\r\n", ch);
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected) {
	while (vict->affected)
	  affect_remove(vict, vict->affected);
	send_to_char("There is a brief flash of light!\r\n"
		     "You feel slightly different.\r\n", vict);
	send_to_char("All spells removed.\r\n", ch);
      } else {
	send_to_char("Your victim does not have any affections!\r\n", ch);
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
    }
    save_char(vict, NOWHERE);
  }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, zone_rnum zone)
{
// DM - TODO - color this in ...
  extern const char *zone_flagbits[];
  char bufstr[80];
  sprintbit(zone_table[zone].zflag, zone_flagbits, bufstr);

  sprintf(bufptr, "%s&B%3d&n %-20.20s &gAge&n: &y%3d&n; &gReset&n: &y%3d&n &b(&r%1d&b)&n; 
&gTop&n: &c%5d&n &gFlags&n: %s\r\n", bufptr, zone_table[zone].number, zone_table[zone].name,
          zone_table[zone].age, zone_table[zone].lifespan,
          zone_table[zone].reset_mode, zone_table[zone].top,bufstr);

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
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], birth[80];

  struct show_struct {
    const char *cmd;
    const char level;
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

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (!strcmp(value, "."))
    self = 1;
  buf[0] = '\0';
  switch (l) {
  case 1:			/* zone */
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, world[ch->in_room].zone);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, zrn);
      else {
	send_to_char("That is not a valid zone.\r\n", ch);
	return;
      }
    } else
      for (zrn = 0; zrn <= top_of_zone_table; zrn++)
	print_zone_to_buf(buf, zrn);
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
    strcpy(birth, ctime(&vbuf.birth));
    sprintf(buf + strlen(buf),
	    "Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
	    birth, ctime(&vbuf.last_logon), (int) (vbuf.played / 3600),
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
    strcpy(buf, "Current stats:\r\n");
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
    send_to_char(buf, ch);
    break;
  case 5:
    strcpy(buf, "Errant Rooms\r\n------------\r\n");
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
	if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0)
	  sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, GET_ROOM_VNUM(i),
		  world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 6:
    strcpy(buf, "Death Traps\r\n-----------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j,
		GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 7:
    strcpy(buf, "Godrooms\r\n--------------------------\r\n");
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
    hcontrol_list_houses(ch);
    break;
  case 10:
    *buf = '\0';
    send_to_char("People currently snooping:\r\n", ch);
    send_to_char("--------------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next) {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      sprintf(buf + strlen(buf), "%-10s - snooped by %s.\r\n",
               GET_NAME(d->snooping->character), GET_NAME(d->character));
    }
    send_to_char(*buf ? buf : "No one is currently snooping.\r\n", ch);
    break; /* snoop */
  default:
    send_to_char("Sorry, I don't understand that.\r\n", ch);
    break;
  }
}


void show_specials_to_char(struct char_data *ch) {

  send_to_char("\r\n &BSpecials available:&n\r\n", ch);
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
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] = {
   { "brief",		LVL_GOD, 	PC, 	BINARY },  /* 0 */
   { "invstart", 	LVL_GOD, 	PC, 	BINARY },  /* 1 */
   { "title",		LVL_GOD, 	PC, 	MISC },
   { "nosummon", 	LVL_GRGOD, 	PC, 	BINARY },
   { "maxhit",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "maxmana", 	LVL_GRGOD, 	BOTH, 	NUMBER },  /* 5 */
   { "maxmove", 	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "hit", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "mana",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "move",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "align",		LVL_GOD, 	BOTH, 	NUMBER },  /* 10 */
   { "str",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "stradd",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "int", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "wis", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "dex", 		LVL_GRGOD, 	BOTH, 	NUMBER },  /* 15 */
   { "con", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "cha",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "ac", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "gold",		LVL_GOD, 	BOTH, 	NUMBER },
   { "bank",		LVL_GOD, 	PC, 	NUMBER },  /* 20 */
   { "exp", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "hitroll", 	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "damroll", 	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "invis",		LVL_IMPL, 	PC, 	NUMBER },
   { "nohassle", 	LVL_GRGOD, 	PC, 	BINARY },  /* 25 */
   { "frozen",		LVL_FREEZE, 	PC, 	BINARY },
   { "practices", 	LVL_GRGOD, 	PC, 	NUMBER },
   { "lessons", 	LVL_GRGOD, 	PC, 	NUMBER },
   { "drunk",		LVL_GRGOD, 	BOTH, 	MISC },
   { "hunger",		LVL_GRGOD, 	BOTH, 	MISC },    /* 30 */
   { "thirst",		LVL_GRGOD, 	BOTH, 	MISC },
   { "killer",		LVL_GOD, 	PC, 	BINARY },
   { "thief",		LVL_GOD, 	PC, 	BINARY },
   { "level",		LVL_IMPL, 	BOTH, 	NUMBER },
   { "room",		LVL_IMPL, 	BOTH, 	NUMBER },  /* 35 */
   { "roomflag", 	LVL_GRGOD, 	PC, 	BINARY },
   { "siteok",		LVL_GRGOD, 	PC, 	BINARY },
   { "deleted", 	LVL_IMPL, 	PC, 	BINARY },
   { "class",		LVL_GRGOD, 	BOTH, 	MISC },
   { "nowizlist", 	LVL_GOD, 	PC, 	BINARY },  /* 40 */
   { "quest",		LVL_GOD, 	PC, 	BINARY },
   { "loadroom", 	LVL_GRGOD, 	PC, 	MISC },
   { "color",		LVL_GOD, 	PC, 	BINARY },
   { "idnum",		LVL_IMPL, 	PC, 	NUMBER },
   { "passwd",		LVL_IMPL, 	PC, 	MISC },    /* 45 */
   { "nodelete", 	LVL_GOD, 	PC, 	BINARY },
   { "sex", 		LVL_GRGOD, 	BOTH, 	MISC },
   { "age",		LVL_GRGOD,	BOTH,	NUMBER },
   { "height",		LVL_GOD,	BOTH,	NUMBER },
   { "weight",		LVL_GOD,	BOTH,	NUMBER },  /* 50 */
   { "olc",             LVL_IMPL,       PC,     NUMBER },

// Primal set commands
   { "timer",           LVL_IMPL,       PC,     NUMBER },
   { "holylight",       LVL_GRGOD,      PC,     BINARY },  
   { "infection",       LVL_GRGOD,      PC,     MISC },
   { "tag",             LVL_GOD,        PC,     BINARY },  /* 55 */
   { "palign",          LVL_GOD,        PC,     BINARY },
   { "noignore",        LVL_GOD,        PC,     BINARY },
   { "leader",          LVL_GOD,        PC,     MISC   },  
   { "fix",             LVL_IMPL,       PC,     BINARY },
   { "clan",            LVL_GOD,        PC,     MISC },    /* 60 */
   { "autogold",        LVL_GOD,        PC,     BINARY },
   { "autoloot",        LVL_GOD,        PC,     BINARY },
   { "autoassist",      LVL_GOD,        PC,     MISC   },   
   { "autoassisters",   LVL_GOD,        BOTH,   MISC   },
   { "autosplit",       LVL_GOD,        BOTH,   MISC },    /* 65 */ 
   { "race", 		LVL_GOD, 	PC,   MISC },
   { "special",         LVL_GRGOD,      PC,   MISC },
   { "\n", 0, BOTH, MISC }
  };


int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  int i, on = 0, off = 0, value = 0;
  room_rnum rnum;
  room_vnum rvnum;
  char output[MAX_STRING_LENGTH];
  int cnum;

  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) != LVL_IMPL) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char("Maybe that's not such a great idea...\r\n", ch);
      return (0);
    }
  }
  if (GET_LEVEL(ch) < set_fields[mode].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
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
    vict->points.max_hit = RANGE(1, 32000);
    affect_total(vict);
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 32000);
    affect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 32000);
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
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
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
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 21:
    vict->points.exp = RANGE(0, 50000000);
    break;
  case 22:
    vict->points.hitroll = RANGE(-120, 120);
    affect_total(vict);
    break;
  case 23:
    vict->points.damroll = RANGE(-120, 120);
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
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
  case 28:
    GET_PRACTICES(vict) = RANGE(0, 100);
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
    if (value > GET_LEVEL(ch) || value > LVL_IMPL) {
      send_to_char("You can't do that.\r\n", ch);
      return (0);
    }
    RANGE(0, LVL_IMPL);
    vict->player.level = (byte) value;
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
      sprintf(buf, "Class returned: %d\r\n", i);
      send_to_char(buf, ch);
      send_to_char("That is not a class.\r\n", ch);
      return (0);
    }
    remove_class_specials(ch);
    GET_CLASS(vict) = i;
    GET_MODIFIER(ch) = race_modifiers[GET_RACE(ch)] + class_modifiers[i] +
	special_modifier(ch); 
    set_class_specials(ch);
    apply_specials(ch, FALSE);
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "off")) {
      REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
    } else if (is_number(val_arg)) {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE) {
        SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
	GET_LOADROOM(vict) = rvnum;
	sprintf(output, "%s will enter at room #%d.", GET_NAME(vict),
		GET_LOADROOM(vict));
      } else {
	send_to_char("That room does not exist!\r\n", ch);
	return (0);
      }
    } else {
      send_to_char("Must be 'off' or a room's virtual number.\r\n", ch);
      return (0);
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
    break;
  case 44:
    if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
      return (0);
    GET_IDNUM(vict) = value;
    break;
  case 45:
/*
    if (GET_IDNUM(ch) > 1) {
      send_to_char("Please don't use this command, yet.\r\n", ch);
      return (0);
    }
 */ if (GET_LEVEL(vict) >= LVL_GRGOD) {
      send_to_char("You cannot change that.\r\n", ch);
      return (0);
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
        return;
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
      GET_IGN1(vict) = 0 ;
      GET_IGN2(vict) = 0 ;
      GET_IGN3(vict) = 0 ;
      GET_IGN_LEVEL(vict) = 0;
      GET_IGN_NUM(vict) = 0 ;
    }
    break;

  case 58: // Leader (clan)
    if (str_cmp(val_arg, "on") == 0) {
      if (GET_CLAN_NUM(vict) < 0) {
        sprintf(buf, "%s is not in a clan.\r\n", GET_NAME(vict));
        send_to_char(buf, ch);
      } else {
        sprintf(buf, "Leader ON for %s.\n\r", GET_NAME(vict));
        send_to_char(buf, ch);
 
        if (!EXT_FLAGGED(vict, EXT_LEADER))
          SET_BIT(EXT_FLAGS(vict) , EXT_LEADER );
      }
    }
 
    if (str_cmp(val_arg, "off") == 0) {
      if (EXT_FLAGGED(vict, EXT_LEADER))
        REMOVE_BIT(EXT_FLAGS(vict), EXT_LEADER);
 
      sprintf(buf, "Leader OFF for %s.\r\n", GET_NAME(vict));
      send_to_char(buf, ch);
    }
    break ;

  case 59: // Fix
    // REMOVE_BIT(PRF_FLAGS(vict), PRF_FIX);
    break;

  case 60: // Clan
    if (str_cmp(val_arg, "off") == 0) {
      if (GET_CLAN_NUM(vict) < 0)
        sprintf(buf, "%s is already clanless.", GET_NAME(vict));
      else {
        sprintf(buf, "%s is now clanless.", GET_NAME(vict));

      if (vict != ch) {
          sprintf (buf, "You have been banished from the %s.\r\n", get_clan_disp(vict));
          send_to_char(buf, vict);
        }
 
        GET_CLAN_NUM(vict) = -1;
        GET_CLAN_LEV(vict) = 0 ;
 
        REMOVE_BIT(EXT_FLAGS(vict), EXT_CLAN);
 
        if (EXT_FLAGGED(vict, EXT_LEADER))
          REMOVE_BIT(EXT_FLAGS(vict), EXT_LEADER);
 
        if (EXT_FLAGGED(vict, EXT_SUBLEADER))
          REMOVE_BIT(EXT_FLAGS(vict), EXT_SUBLEADER);
 
      }
    } else {
      for (i=0; i < CLAN_NUM; i++)  {
        if (!strn_cmp(val_arg, get_clan_name(i), 3))
          cnum=i;
      }
 
      if (cnum == 0)
        sprintf(buf, "That is not a clan.");
      else {
        GET_CLAN_NUM(vict) = cnum;
        SET_BIT (EXT_FLAGS(vict), EXT_CLAN);
        sprintf(buf, "%s is now a member of the %s.", GET_NAME(vict), get_clan_disp(vict));
        if( vict != ch) {
          sprintf(buf, "You are now a member of the %s.\r\n", get_clan_disp(vict));
          send_to_char(buf, vict);
        }
      }
    }

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
        return;
      }
    } else {
      send_to_char("Options: OFF\r\n",ch);
      return;
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
        return;
      }
    } else {
      send_to_char("Options: OFF\r\n",ch);
      return;
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
    GET_RACE(vict) = i;
    GET_MODIFIER(ch) = race_modifiers[i] + class_modifiers[GET_CLASS(ch)]
	+ special_modifier(ch);
   set_race_specials(ch); 
   break;
  case 67:
   if (strcmp(val_arg, "list") ==  0 ) {
     show_specials_to_char(ch);
     return (0);
   }
   if( !isdigit(val_arg[0]) ) {
	send_to_char("You must specify the value of the special.\r\n Use &gset <player> special list&n to see specials available.\r\n", ch);
   	return (0);
   }
   
   if( atoi(val_arg) <= 0 || atoi(val_arg) > MAX_SPECIALS ) {
	send_to_char("That's not a valid option!\r\nUser &gset <player> special list&n to see options.\r\n", ch);
   	return (0);
   }
   else {
	if (IS_SET(GET_SPECIALS(ch), (1 << atoi(val_arg) - 1))) {
		send_to_char("Bit removed\r\n", ch);
		REMOVE_BIT(GET_SPECIALS(ch), (1 << atoi(val_arg) - 1));
	}	
	else {
		send_to_char("Bit set\r\n", ch);
		SET_BIT(GET_SPECIALS(ch), (1 << atoi(val_arg) - 1));
	}
   }
  
   sprintf(buf, "&g%s&n toggled &W%s&n for &B%s&n.\r\n", special_ability_bits[atoi(val_arg)-1], 
	IS_SET(GET_SPECIALS(vict), (1 << atoi(val_arg) -1)) ? "on" : "off", GET_NAME(vict));
   send_to_char(buf, ch);
   apply_specials(vict, FALSE);
   break;
  default:
    send_to_char("Can't set that!\r\n", ch);
    return (0);
  }

  strcat(output, "\r\n");
  send_to_char(CAP(output), ch);
  return (1);
}


ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  struct char_file_u tmp_store;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],
	val_arg[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field) {
    send_to_char("Usage: set <victim> <field> <value>\r\n", ch);
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, FIND_CHAR_WORLD))) {
	send_to_char("There is no such player.\r\n", ch);
	return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
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

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict, NOWHERE);
    if (is_file) {
      char_to_store(vict, &tmp_store);
      fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
      fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
      send_to_char("Saved in file.\r\n", ch);
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

// DM - TODO - Decide on sethelp (add in wizhelp file?)

ACMD(do_deimmort)
{
  struct char_data *vict;
 
  one_argument(argument,arg);
 
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))){
     send_to_char("I cannot seem to find that person!!!\r\n", ch);
     return;
  }
 
  if (IS_NPC(vict)){
        send_to_char("DEMOTE A MONSTER!!!!.... Go find something more productive to do!\r\n", ch);
        return;
  }
 
  if (GET_LEVEL(vict)!=LVL_ANGEL){
        send_to_char("You can only demote ANGELS!!\r\n", ch);
        return;
  }
 
  GET_LEVEL(vict)=LVL_IMMORT;
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
 
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))){
     send_to_char("I cannot seem to find that person!!!\r\n", ch);
     return;
  }
 
  if (IS_NPC(vict)){
        send_to_char("You want to immort a monster!!!!!. YOU MUST BE BORED!\r\n", ch);
        return;
  }
  if (GET_LEVEL(vict)<LVL_IMMORT){
        send_to_char("They are not an IMMORTAL!!.  Can't do it!\r\n", ch);
        return;
  }
  if (GET_LEVEL(vict)==LVL_ANGEL){
        send_to_char("They are already angel silly!\r\n", ch);
        return;
  }
  if (GET_LEVEL(vict)>LVL_ANGEL){
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
 
  if (!*arg) {
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

ACMD(do_queston)
{
  struct char_data *vict;
  char buff[120];
  half_chop(argument, arg, buf);
 
  if (!*arg) {
    send_to_char("Quest On for who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg,FIND_CHAR_WORLD))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  SET_BIT(PRF_FLAGS(vict),PRF_QUEST);
  sprintf(buff,"%s is now in the quest.\r\n",vict->player.name);
  send_to_char(buff,ch);
}
 
ACMD(do_questoff)
{
  struct char_data *vict;
  char buff[120];
  half_chop(argument, arg, buf);
 
  if (!*arg) {
    send_to_char("Quest Off for who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg,FIND_CHAR_WORLD))) {
    send_to_char(NOPERSON, ch);
    return;
  }            REMOVE_BIT(PRF_FLAGS(vict),PRF_QUEST);
  sprintf(buff,"%s is no longer in the quest.\r\n",vict->player.name);
  send_to_char(buff,ch);
} 

/* this is a must! clock over one mud hour */
ACMD(do_tic)
{
  extern pulse;
  pulse=-1;
  send_to_char("Time moves forward one hour.\r\n",ch);
} 

/* skillshow - display the spell/skill  */
ACMD(do_skillshow)
{
  extern char *spells[];
  extern struct spell_info_type spell_info[];
  int i, sortpos, spellnum, mana;
  struct char_data *vict;
  int class_index;

  one_argument(argument,arg);
 
  if (!*arg) {
    vict=ch;
  } else if(!(vict = get_char_vis(ch,arg,FIND_CHAR_WORLD))) {
    send_to_char(NOPERSON,ch);
    return;
  }
 
  sprintf(buf, "Spell/Skill abilities for &B%s&n.\r\n",GET_NAME(vict));
  sprintf(buf, "%sPractice Sessions: &M%d&n.\r\n",buf,GET_PRACTICES(vict));
 
  sprintf(buf, "%s%-20s %-10s\r\n",buf,"Spell/Skill","Ability");
  strcpy(buf2, buf);

  class_index = GET_CLASS(vict); 

  for (sortpos = 1; sortpos < MAX_SKILLS; sortpos++) {
    i = spell_sort_info[SORT_ALPHA][0][sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) {
      mana = mag_manacost(ch, i);
      sprintf(buf, "%-20s %-10.10s %-3d ", spell_info[i].name, GET_SKILL(vict, i), mana);
      strncat(buf2, buf, strlen(buf)); 

      /* Display the stat requirements */
      if (spell_info[i].str[class_index]!=0){
        if (GET_REAL_STR(ch) >= spell_info[i].str[class_index])
          sprintf(buf,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].str[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].str[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].intl[class_index]!=0){
        if (GET_REAL_INT(ch) >= spell_info[i].intl[class_index])
          sprintf(buf,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].intl[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].intl[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].wis[class_index]!=0){
        if (GET_REAL_WIS(ch) >= spell_info[i].wis[class_index])
          sprintf(buf,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].wis[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].wis[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].dex[class_index]!=0){
        if (GET_REAL_DEX(ch) >= spell_info[i].dex[class_index])
          sprintf(buf,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].dex[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].dex[class_index],CCNRM(ch,C_NRM));   
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].con[class_index]!=0){
        if (GET_REAL_CON(ch) >= spell_info[i].con[class_index])
          sprintf(buf,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].con[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].con[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
      if (spell_info[i].cha[class_index]!=0){
        if (GET_REAL_CHA(ch) >= spell_info[i].cha[class_index])
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
}  

/* Vulcan Neck Pinch - basically just stuns the victim - Vader */
/* this was gunna be a skill but i didnt no if it fit :)       */
ACMD(do_pinch)
{
  struct char_data *vict;
 
  one_argument(argument,arg);
 
  if(!(vict = get_char_room_vis(ch,arg))) {
    if(FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Upon whom do you wish to perform the Vulcan neck pinch??\r\n",ch);
      return;
      }
    }
 
  if(!AWAKE(vict)) {
    send_to_char("It appears that your victim is already incapacitated...\r\n",ch);
    return;
    }
 
  if(vict == ch) {
    send_to_char("You reach up and perform the Vulcan neck pinch on yourself and pass out...\r\n",ch);
    act("$n calmly reaches up and performs the Vulcan neck pinch on $mself before passing out...",FALSE,ch,0,0,TO_ROOM);
    GET_POS(ch) = POS_STUNNED;
    return;
    }
 
  if(GET_LEVEL(vict) >= GET_LEVEL(ch)) {
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
 
  fp = open("/primal/lib/text/cassandra.poofin", O_RDONLY);
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

void enhance_quest_item(struct obj_data *qitem, struct char_data *ch, int itemnumber) { 
	
	int i, j, k, l, found = 0;
	struct obj_data *tmp;
	for( i = 0; i < MAX_QUEST_ITEMS; i++ ) {
		if (found)
			break;
		if( GET_QUEST_ITEM(ch, i) == 0 )
			continue;

		if( GET_QUEST_ITEM(ch, i) == GET_OBJ_VNUM(qitem))
			itemnumber--;

		// We have the enhancement
		if(itemnumber == 0 ) {
		  // For every available enhancement
		  for( j = 0; j < MAX_NUM_ENHANCEMENTS; j++ ) {
			if (found)
				break;
			if( GET_QUEST_ENHANCEMENT(ch, i, j) == 0 )
				continue;
		
			for( k = 0; k < MAX_ENHANCEMENT_VALUES; k++ ) {
			   // for every affect on the object
			   found = 0;
			   for( l = 0; l < MAX_OBJ_AFFECT; l++ ) {
			      if(qitem->affected[l].location == GET_QUEST_ENHANCEMENT(ch, i, j) ){
			  	qitem->affected[l].modifier += GET_QUEST_ENHANCEMENT_VALUE(ch,i,j, k);
				found = 1;
				break;
			      }
			    }
			    if( !found ) { // New affect
			       for( l = 0 ; l < MAX_OBJ_AFFECT; l++ )
			         if( qitem->affected[l].location = APPLY_NONE ) {
			            found = 1;
				    qitem->affected[l].modifier = GET_QUEST_ENHANCEMENT_VALUE(ch, i, j, k);
				    qitem->affected[l].location = GET_QUEST_ENHANCEMENT(ch, i, j);
				    break;
				} 
			    } // If not found
		        } // For every enhancement value
		    } // For every enhancement
		} // If this is the enhancement we want
	} // For every quest item


}

void apply_quest_enhancements(struct char_data *ch) {

  struct obj_data *qitem, *itemList[2048], *tmp;
  long qitemID;
  int itemcount, enhanced = 0, megacount = 0, i, j, k, found = 0;

  // Create the inv/eq/house item megalist
  for( tmp = ch->carrying; tmp; tmp = tmp->next_content) {   // Inventory
	itemList[megacount] = tmp;
	megacount++;
  }
 
  for ( i = 0; i < NUM_WEARS; i++ ) {
	if ( GET_EQ(ch, i) != NULL) {
	 itemList[megacount] = ch->equipment[i];
	 megacount++;
	}
  }

  for( i = 0; i < num_of_houses; i++ ) {
 	if ( house_control[i].owner == get_id_by_name(GET_NAME(ch)) )  {
          for( tmp = world[real_room(house_control[i].vnum)].contents; tmp; tmp = tmp->next) {
		itemList[megacount] = tmp;
		megacount++;
	  } 
	}
  }

  // Go through each quest item
  for( i = 0; i < MAX_QUEST_ITEMS; i++ ) {
	if (GET_QUEST_ITEM(ch, i) == 0 )
		continue;

	itemcount = 0;
	// Count the number of instances of enhancements on this item
	for( j = 0; j < MAX_QUEST_ITEMS; j++ ) 
	   if (GET_QUEST_ITEM(ch, j) == GET_QUEST_ITEM(ch, i) )
		itemcount++;

	// Find all items of this type, enhance them
	if( itemcount >= 1 ) {
	  enhanced = 1;
	  for( k = 0; k < megacount; k++ ) {

		if ( GET_OBJ_VNUM(itemList[k]) == GET_QUEST_ITEM(ch, i) ) {
		   if (itemcount <= 0  ) 
			break;
		   // Replace this item with a freshly loaded one, to remove
		   // previous enhancements and enhancement replication
		   RECREATE(itemList[k], struct obj_data, 1);
		   itemList[k] = read_object(GET_QUEST_ITEM(ch, i), VIRTUAL);
		   enhance_quest_item(itemList[k], ch, enhanced);
		   enhanced++;
		   itemcount--;  
		}
		   
	  }
	}
  }  

  if (enhanced) {
	sprintf(buf, "%d enhancement%s applied.\r\n", enhanced, enhanced == 1 ? "" : "s");
	send_to_char(buf, ch);
  }
  else
	send_to_char("Failed to apply enhancement.\r\n", ch);

}

show_enhancements_to_player(struct char_data *ch) {

  send_to_char("&BQuest Item Enhancements:&n\r\n", ch);
  send_to_char( "&y1&n  - Strength\r\n"
		"&y2&n  - Dexterity\r\n"
		"&y3&n  - Intelligence\r\n"
		"&y4&n  - Wisdom\r\n"
		"&y5&n  - Constitution\r\n"
		"&y6&n  - Charisma\r\n"
		"&y7&n  - Class      (Not used)\r\n"
		"&y8&n  - Level      (Not used)\r\n"
		"&y9&n  - Age\r\n"
		"&y10&n - Weight\r\n"
		"&y11&n - Height\r\n"
		"&y12&n - Mana\r\n"
		"&y13&n - Hit\r\n"
		"&y14&n - Move\r\n"
		"&y15&n - Gold       (Not used)\r\n"
		"&y16&n - Experience (Not used)\r\n"
		"&y17&n - Armour Class\r\n"
		"&y18&n - Hitroll\r\n"
		"&y19&n - Damroll\r\n"
		"&y20&n - Save vs Paralysation\r\n"
		"&y21&n - Save vs Rod, Staff, Wand\r\n"
		"&y22&n - Save vs Petrification\r\n"
		"&y23&n - Save vs Breath Weapon\r\n"
		"&y24&n - Save vs Spells\r\n", ch);

}

void remove_enhancement(struct char_data *ch, struct char_data *vict, int itemno, int enhno) {
	
	int i = 0;

	sprintf(buf, "Quest item enhancement (set on %s) for &y%s&n removed.\r\n",
		enhancement_names[GET_QUEST_ENHANCEMENT(vict, itemno, enhno)], GET_NAME(ch));
	send_to_char(buf, ch);

	for( i = 0; i < MAX_ENHANCEMENT_VALUES; i++ )
		GET_QUEST_ENHANCEMENT_VALUE(vict, itemno, enhno, i) = 0;
	GET_QUEST_ENHANCEMENT(vict, itemno, enhno) = 0; 

	
}


// DM - TODO - add clear (clear all of the array)..., nicen the output & messages, 
// DM - TODO - log it, add something like OBJ_IS_QUEST(struct obj_data *obj) and check for it ...
// TALI - Added enhancement capability for quest EQ and STAT'ing so we can see what their
//        quest eq is looking like after enhancements
ACMD(do_quest_log)
{
  struct obj_data *obj, *itemList[2048], *tmp; 
  struct char_data *vict, *cbuf;
  struct char_file_u tmp_store;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  obj_rnum rnum, objnum;
  int i, j, k, is_file=0, player_i = 0, found = 0, dot_mode, qsearch, value, 
	enhancement =0, foundEnh = 0, counter = 1, megacount = 0;

  subcmd=-1;
 
  if (IS_NPC(ch))
    return;
 
  skip_spaces(&argument);
  half_chop(argument, arg1, rest);
 
  if (!str_cmp(arg1,"list"))
    subcmd = SCMD_QUEST_LIST;
  if (!str_cmp(arg1,"add"))
    subcmd = SCMD_QUEST_ADD;
  if (!str_cmp(arg1,"del"))
    subcmd = SCMD_QUEST_DELETE;
  if (!str_cmp(arg1,"enhance"))
     subcmd = SCMD_QUEST_ENHANCE;
  if (!str_cmp(arg1,"stat"))
     subcmd = SCMD_QUEST_STAT;
 
  if (subcmd == -1) {
    send_to_char("That'll either be &clist&n/&cadd&n/&cdel&n/&cenhance&n/&cstat&n.\r\n",ch);
    return;
  }           
 
  half_chop(rest, arg2, rest);
 
  if (!*arg2) {
    send_to_char("On whom?\r\n",ch);
    return;
  }
 
  if (!(vict = get_player_vis(ch, arg2, FIND_CHAR_WORLD))) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if ((player_i = load_char(arg2, &tmp_store)) > -1) {
      store_to_char(&tmp_store, cbuf);
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch)) {
        free_char(cbuf);
        send_to_char("Sorry, you can't do that.\r\n", ch);
        return;
      }
      vict = cbuf;
      is_file = 1;
    } else {
      free(cbuf);
      // See if they want to list the quest enhancements
      if (strcmp(arg2, "list") != 0 && (subcmd != SCMD_QUEST_ENHANCE) ) {
         send_to_char("There is no such player.\r\n", ch);
         return;
      }
    }
  }
 
  strcpy(buf,"");
 
  switch(subcmd) {
  case SCMD_QUEST_LIST:
    for (i=0; i < MAX_QUEST_ITEMS; i++) {
      if (GET_QUEST_ITEM(vict,i) > 0) {  
        if ( (rnum = real_object(GET_QUEST_ITEM(vict,i) )) >=0) {
          obj = read_object(rnum, REAL);
 
          // problem loading the obj?? lets clear the space.
          if (!obj) {
            GET_QUEST_ITEM(vict,i) = 0;
            GET_QUEST_ITEM_NUMB(vict,i) = 0;
            continue;
          }
          foundEnh = 0;
          // Check object for enhancements
          for( j = 0; j < MAX_NUM_ENHANCEMENTS; j++ )
                if (GET_QUEST_ENHANCEMENT(vict, i, j) != 0 )
                        foundEnh = 1;
          found = 1;
          sprintf(buf2, "&c[%5d]&n %s (&Rx%d&n) Enhanced: &y%s&n\r\n",
                GET_OBJ_VNUM(obj), obj->name, GET_QUEST_ITEM_NUMB(vict,i),
                foundEnh == 1 ? "Yes" : "No");
          extract_obj(obj);
 
          strcat(buf,buf2);
        } else {
          sprintf(buf2,"Quest Log Error: object %d belonging to %s no longer exits. Removed",
                GET_QUEST_ITEM(vict,i),GET_NAME(vict));
          GET_QUEST_ITEM(vict,i) = 0;
          GET_QUEST_ITEM_NUMB(vict,i) = 0;
          mudlog(buf2, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
        }
      }
    }
 
    if (found) {
      sprintf(buf2,"Quest items for &B%s&n:\r\n",GET_NAME(vict));
      strcat(buf2,buf);
      send_to_char(buf2,ch);
    } else {      
      send_to_char("They have no quest items...\r\n",ch);
    }
  break;
 
  case SCMD_QUEST_ADD:
 
    half_chop(rest, arg3, rest);
 
    if (!(isdigit(*arg3))) {
      send_to_char("Usage: questlog add <name> &c<obj vnum>&n\r\n",ch);
      break;
    }
 
    if (!(objnum = atoi(arg3))) {
      send_to_char("Thats not a vnum.\r\n",ch);
      break;
    }
 
    if ((rnum = real_object(objnum)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      break;
    } 
 /*  -- Tal - I dont want to allow this. Easier (for me) to just list all items separately --
    // first parse check to see if the object already exists ...
    for (i=0; (i < MAX_QUEST_ITEMS) && (!found); i++) {
      if (GET_QUEST_ITEM(vict,i) == objnum) {
        found = 1;
        GET_QUEST_ITEM_NUMB(vict,i) += 1;
      }
    }
  */ 
    // find first available space ...
    for (i=0; (i < MAX_QUEST_ITEMS) && (!found); i++) {
      if (GET_QUEST_ITEM(vict,i) <= 0) {
        found = 1;
        GET_QUEST_ITEM(vict,i) = objnum;
        GET_QUEST_ITEM_NUMB(vict,i) = 1;
      }
    }
 
    if (found) {
      send_to_char("Object Added.\r\n",ch);
    } else {
      send_to_char("Maximum number of objects exceeded.\r\n",ch);
    }
 
  break;
 
  case SCMD_QUEST_DELETE:
 
    half_chop(rest, arg3, rest);
 
    if (!(isdigit(*arg3))) {
      send_to_char("Usage: questlog del <name> &c<obj vnum>&n\r\n",ch);
      break;
    }
 
    // Get either the <x>. or the rnum
    qsearch = atoi(arg3);
    strcpy(arg2, arg3);
 
    // See if they are looking for a particular instance
    i = 0;
    while( (arg3[i] != '.') && (arg3[i] != ' ') && (i < strlen(arg3)) )
        i++;
 
    // Allocate the rnum, will be 0 if there was no <x>.<rnum>
    objnum = atoi(&arg3[i+1]);
    // Check if there was no . in the arg
    if (objnum == 0 ) {
        objnum = qsearch;         // QSearch is holding the rnum then
        qsearch = 1;            // Set to first occurance
    }
 
    // If it's still 0, they haven't set a vnum
    if ( objnum == 0 ) {
        send_to_char("You must specify an object vnum.\r\n", ch);
        break;
    }

    for (i=0; i < MAX_QUEST_ITEMS; i++) {
      if (GET_QUEST_ITEM(vict,i) == objnum) {
        if (qsearch == 1) {
           found = 1;
           GET_QUEST_ITEM(vict, i) = 0;
           break;
        }
        else
	  qsearch--;
      }

/* -- Tal - Removed GET_QUEST_NUMB capability --
        GET_QUEST_ITEM_NUMB(vict,i) -= 1;
        if (GET_QUEST_ITEM_NUMB(vict,i) <= 0) {
          GET_QUEST_ITEM(vict,i) = 0;
          GET_QUEST_ITEM_OBJ(vict, i) = NULL;
        } 
  */
      }
       
 
    if (found) {
      send_to_char("Object Removed.\r\n",ch);
    } else {
      send_to_char("Object not found in list.\r\n",ch);
    }
 
  break;
  
  /** QUEST ITEM ENHANCING **/ 
  
  // Usage: qlog enhance <person> <x>.<vnum> <enhancement #> <value>
  case SCMD_QUEST_ENHANCE:
    // See if there's no player to enhance, it means they want to list the enhancements
    if( vict == NULL ) {
	show_enhancements_to_player(ch);
	break;
    }

    half_chop(rest, arg3, rest);
    if( !*arg3 ) {
	send_to_char("Enhancement usage: questlog enhance <player> <x>.<obj vnum> <enh #> <val>\r\n"
		     "       Optionally: questlog enhance <player> <list | apply | delete>\r\n", ch);
        break;
    }

    // If they're not enhancing they can list, apply or delete
    found = 0;
    if (!(isdigit(*arg3))) {
      // Apply enhancements to players items
      if (strcmp(arg3, "apply") == 0) {
	apply_quest_enhancements(vict);
	break;
      }
      else
      // Delete
      if( strcmp(arg3, "delete") == 0 ) {
	    half_chop(rest, arg3, rest);
	    if (!isdigit(*arg3)) {
		if (strcmp(arg3, "all") == 0) {
		   for( i = 0; i < MAX_QUEST_ITEMS; i++ )
		       if( GET_QUEST_ITEM(ch, i) != 0 )
		         for( j = 0; j < MAX_NUM_ENHANCEMENTS; j++)
		           if(GET_QUEST_ENHANCEMENT(vict, i, j) != 0 )
				remove_enhancement(ch, vict, i, j); 
		  break;
		}
		else {
		  send_to_char("Deletion of enhancements must be either 'all' or a number.\r\n",ch);
		  break;
		}	
	     }
	     else {
	       found = 0;
	       for ( i = 0; i < MAX_QUEST_ITEMS; i++ )
		   if( GET_QUEST_ITEM(ch, i) != 0 )
	             for( j = 0; j < MAX_NUM_ENHANCEMENTS; j++ )
			if (GET_QUEST_ENHANCEMENT(ch, i, j) != 0 ) {
			   if( counter == atoi(arg3)) {
			     remove_enhancement(ch, vict, i, j);
			     found = 1; 
			     break;
			   }
			   counter++;
			}	
		if( !found )
		  send_to_char("That enhancement doesn't exist!\r\n", ch);
		break;
	     }
	    
      } // end of delete enhancement
      else
      // Are they listing?
      if( strcmp(arg3, "list") == 0 )
      {
	// Prepare the list
	sprintf(buf, "Quest item enhancements for &B%s&n:\r\n", GET_NAME(vict));
	for( i = 0; i < MAX_QUEST_ITEMS; i++) 
	{
		if( GET_QUEST_ITEM(vict, i) != 0 ) 
		{
		   found++;
		   for( j = 0; j < MAX_NUM_ENHANCEMENTS; j++ ) 
		   {
                     if (GET_QUEST_ENHANCEMENT(vict, i, j) != 0 ){ 
			for( k = 0; k < MAX_ENHANCEMENT_VALUES; k++ ) 
			  if( GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k) != 0 ){ 
			     foundEnh++;
			     sprintf(buf + strlen(buf), "  Item #&c%2d&n(&C%5d&n) Enh #&g%d&n - adds &y%s&n with value &W%d&n\r\n",
		  		i+1, GET_QUEST_ITEM(vict, i), j + 1,
				enhancement_names[GET_QUEST_ENHANCEMENT(vict, i, j)],
				GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k));
			  }
		     } 
		   }
		}
	} 
	if( !found )
		send_to_char("They have no quest items..\r\n", ch);
	else if( !foundEnh )
		send_to_char("They have no enhancements on their items.\r\n", ch);
	else
	       send_to_char(buf, ch);
	break;
      } 
      else {
        send_to_char("Usage: questlog enhance <name> &c<num in list>.<obj vnum>&n <enh #> <val>\r\n",ch);
        break;
      }
    }
 
    // Get either the <x>. or the rnum
    qsearch = atoi(arg3);
    strcpy(arg2, arg3);
 
    // See if they are looking for a particular instance
    i = 0;
    while( (arg3[i] != '.') && (arg3[i] != ' ') && (i < strlen(arg3)) )
        i++;
 
    // Allocate the rnum, will be 0 if there was no <x>.<rnum>
    rnum = atoi(&arg3[i+1]);
    // Check if there was no . in the arg
    if (rnum == 0 ) {
        rnum = qsearch;         // QSearch is holding the rnum then
        qsearch = 1;            // Set to first occurance
    }
 
    // If it's still 0, they haven't set a vnum
    if ( rnum == 0 ) {
        send_to_char("You must specify an object vnum.\r\n", ch);
        break;
    }
 
    // Get the enhancement
    half_chop(rest, arg3, rest);
    if( !isdigit(*arg3) ) {
        send_to_char("The enhancement must be an integer value, 1 to 24.\r\n", ch);
        break;
    }
    enhancement = atoi(arg3);
 
    if( enhancement == 0 ) {
        send_to_char("You must specify a valid enhancement number and value.\r\n" , ch);
        break;
    }
 
    // Get the value
    half_chop(rest, arg3, rest);
    if( !isdigit(*arg3) ) {
        send_to_char("The value must be an integer value.\r\n", ch);
        break;
    }
    value = atoi(arg3);
 
    if( (value == 0) || (value < -125) || (value > 125) ) {
        send_to_char("You must provide a valid value for the enhancement.\r\n", ch);
        break;
    }
 
    sprintf(buf, "Seeking to enhance item number %d with vnum %d.\r\n", qsearch, rnum);
    send_to_char(buf, ch);
	
    found = 0;
    // OKay, we have all values, do enhancement 
    for (i = 0; i < MAX_QUEST_ITEMS; i++)
    {
	// If this quest item has the rnum we're looking for
	if( GET_QUEST_ITEM(vict, i) == rnum ) {
		if( qsearch != 1 ) {
			qsearch--;
			continue;
		}
	}
	else
		continue;

	found = 1;
	// rnum and qsearch are now valid, so set the quest item enhancement
	GET_QUEST_ITEM(vict, i) = rnum;
	GET_QUEST_ITEM_OBJ(vict, i) = 0;	// Not sure if i'm going to use this yet
	// Search for an enhancement slot, either with same enhancement, or blank
	for( j = 0; j < MAX_NUM_ENHANCEMENTS; j ++ )
	   if( (GET_QUEST_ENHANCEMENT(vict, i, j) == 0) || 
	       (GET_QUEST_ENHANCEMENT(vict, i, j) == enhancement) ) 
		break;

	if( j >= MAX_NUM_ENHANCEMENTS ) {
		send_to_char("Item already has as many enhancements as possible.\r\n", ch);	
		break;
	}
	
	GET_QUEST_ENHANCEMENT(vict, i, j) = enhancement;
	// Look for a free slot to place the value in
	for(k = 0; k < MAX_ENHANCEMENT_VALUES; k++)
		if( GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k) == 0 )
			break;

	if( k >= MAX_ENHANCEMENT_VALUES ) {
		sprintf(buf, "That item already has as many boosts to %s as possible.\r\n",
			enhancement_names[GET_QUEST_ENHANCEMENT(ch, i, j)]);
		send_to_char(buf, ch);
		break;
	}

	GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k) = value;

	sprintf(buf, "Quest item #&c%d&n(&c%d&n) for &b%s&n was enhanced by &W%d&n on &y%s&n.\r\n", 
		qsearch, rnum, GET_NAME(vict), value, enhancement_names[enhancement]);
	send_to_char(buf, ch);
	break;
    }

    if( !found ) 
	send_to_char("Could not locate that quest item in the log!\r\n",ch);
   
    break;
  case SCMD_QUEST_STAT:
    // Usage: qlod stat <person> <x>.<vnum>
    half_chop(rest, arg3, rest);
    if( !*arg3 ) {
	send_to_char("Stat which quest item (vnum) ?\r\n", ch);
	break;
    }
    if (!isdigit(*arg3)) {
	send_to_char("The quest item id must be a vnum.\r\n", ch);
	break;
    }
    // Get either the <x>. or the rnum
    qsearch = atoi(arg3);
    strcpy(arg2, arg3);
    
    // See if they are looking for a particular instance
    i = 0;
    while( (arg3[i] != '.') && (arg3[i] != ' ') && (i < strlen(arg3)) )
	i++;
    // Allocate the rnum, will be 0 if there was no <x>.<rnum>
    rnum = atoi(&arg3[i+1]);
    // Check if there was no . in the arg
    if (rnum == 0 ) {
	rnum = qsearch;		// QSearch is holding the rnum then
	qsearch = 1;		// Set to first occurance
    // If it's still 0, they haven't set a vnum
    if ( rnum == 0 ) {
	send_to_char("You must specify an object vnum.\r\n", ch);
	break;
    }
   
    // Create a list of all their items
    // Create the inv/eq/house item megalist
    for( tmp = ch->carrying; tmp; tmp = tmp->next_content) {   // Inventory
	itemList[megacount] = tmp;
	megacount++;
    }
 
    for ( i = 0; i < NUM_WEARS; i++ ) {
	if ( GET_EQ(ch, i) != NULL) {
	 itemList[megacount] = ch->equipment[i];
	 megacount++;
	}
    }
    for( i = 0; i < num_of_houses; i++ ) {
 	if ( house_control[i].owner == get_id_by_name(GET_NAME(ch)) )  {
          for( tmp = world[real_room(house_control[i].vnum)].contents; tmp; tmp = tmp->next) {
		itemList[megacount] = tmp;
		megacount++;
	  } 
	}
    }
    
    // Go through the list, finding the instance of the object they're after
    found = 0;
    for (i = 0; i < megacount; i++) {
	if (GET_OBJ_VNUM(itemList[i]) == rnum  ) {
		if( qsearch != 1 ) {
			qsearch--;
			continue;
		}
        	do_stat_object(ch, itemList[i]);
		found = 1;
		break;
        }
    }    

    if( !found ) 
	send_to_char("They don't have that item on them.\r\n", ch);	

    break;

  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}                                                                            
}                                        
// DM - TODO - fix for user levels
ACMD(do_spellshow)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  char *s, *t;
  int spellnum, i, found, index, j;
  extern struct spell_info_type spell_info[];
  extern const char *pc_class_types[];
  int sort_index, class_index, spell_index, sort_type;

  /* get: blank, spell name, target name */
  s = strtok(argument, "'");
 
  if (s == NULL) {
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
      sprintf(buf,"&1Usage: &4%s &n{ '<spellname>' | <classname> [ <sort type> ] }\r\n", CMD_NAME);
    } else {
      sprintf(buf,"&1Usage: &4%s &n<classname> [ <sort type> ]\r\n", CMD_NAME);
    }
    send_to_char(buf,ch);
    return;
  }

  s = strtok(NULL, "'");
  if (s == NULL) {

    // First check if we are going to list the spells for a particular class
    found = FALSE;
    half_chop(argument,arg1,rest);

    class_index=search_block_case_insens(arg1,pc_class_types,FALSE);
    if (class_index>=0) {
      found = TRUE;
        
      half_chop(rest,arg2,rest);
      if (!*arg2) {
        sort_type=SORT_ALPHA; 
      } else if ((sort_type=search_block_case_insens(arg2,sort_names,FALSE))<0) {
        sprintf(buf2,"Invalid sort type. Valid types are: ");
        for (i=0;i<NUM_SORT_TYPES;i++) {
          sprintf(buf1,"%s ",sort_names[i]);
          strcat(buf2,buf1);
        }
        send_to_char(buf2,ch);
        return;
      }
    }

    if (found) {
      sprintf(buf2,"Spells and Skills for the &B%s&n sorted &b%s&n:\r\n", 
        CLASS_NAME(class_index),sort_names[sort_type]);
      for (spell_index = 1; spell_index < MAX_SKILLS; spell_index++) {
        if (sort_type == SORT_ALPHA)
	  spellnum = spell_sort_info[0][0][spell_index];
        else 
	  spellnum = spell_sort_info[sort_type][class_index][spell_index];

	if (spell_info[spellnum].min_level[class_index] != LVL_OWNER+1) {
	  // sprintf(buf2,"&R%-20s&n (%d)\r\n", spell_info[spellnum].name,
	  //	spell_info[spellnum].min_level[class_index]);
          //strcat(buf,buf2);

          sprintf(buf1, "%-20s %-3d ", spell_info[spellnum].name,
              spell_info[spellnum].min_level[class_index]);
          strncat(buf2, buf1, strlen(buf1));
 
          /* Display the stat requirements */
          if (spell_info[spellnum].str[class_index]!=0){
              sprintf(buf1,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM), 
                spell_info[spellnum].str[class_index],CCNRM(ch,C_NRM));
 
            strncat(buf2, buf1, strlen(buf1));
          }
 
          if (spell_info[spellnum].intl[class_index]!=0){
              sprintf(buf1,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),
                spell_info[spellnum].intl[class_index],CCNRM(ch,C_NRM));
 
            strncat(buf2, buf1, strlen(buf1));
          }
 
          if (spell_info[spellnum].wis[class_index]!=0){
              sprintf(buf1,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),
                spell_info[spellnum].wis[class_index],CCNRM(ch,C_NRM));
 
            strncat(buf2, buf1, strlen(buf1));
          }
 
          if (spell_info[spellnum].dex[class_index]!=0){
              sprintf(buf1,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),
                spell_info[spellnum].dex[class_index],CCNRM(ch,C_NRM));
 
            strncat(buf2, buf1, strlen(buf1));
          }
 
          if (spell_info[spellnum].con[class_index]!=0){
              sprintf(buf1,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),
                spell_info[spellnum].con[class_index],CCNRM(ch,C_NRM));
 
            strncat(buf2, buf1, strlen(buf1));
          }

          if (spell_info[spellnum].cha[class_index]!=0){
              sprintf(buf1,"%sCha: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),
                spell_info[spellnum].cha[class_index],CCNRM(ch,C_NRM));
 
            strncat(buf2, buf1, strlen(buf1));
          }
 
          sprintf(buf1,"\r\n");
          strncat(buf2, buf1, strlen(buf1));
        }
      }
      page_string(ch->desc,buf2,TRUE);
      return;
    } else {
      send_to_char("Invalid Class Name.\r\n",ch);
      return;
    }



    send_to_char("Spell names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
    return;
  }
  t = strtok(NULL, "\0");

  if (GET_LEVEL(ch) < LVL_IMMORT) {
    sprintf(buf,"&1Usage: &4%s &n<classname> [ <sort type> ]\r\n", CMD_NAME);
    send_to_char(buf,ch);
    return;
  }
 
  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s); 
 
  if (spellnum == -1) {
    send_to_char("Spell/Skill name not found.\r\n",ch);
    return;
  }

  sprintf(buf,"Name: &R%s&n\r\n", spell_info[spellnum].name);

  for (i = 0; i < NUM_CLASSES; i++) {
    if (spell_info[spellnum].min_level[i] != LVL_OWNER+1) {

      sprintf(buf2,"&B%s:&n\r\nLevel: &M%d&n Mana_min: &G%d&n Mana_max: &G%d&n Mana_change: &G%d&n\r\n",
	      pc_class_types[i], spell_info[spellnum].min_level[i],  
	      spell_info[spellnum].mana_min[i],  
	      spell_info[spellnum].mana_max[i],  
	      spell_info[spellnum].mana_change[i]);

      strcat(buf,buf2);

      sprintf(buf2, "&gInt: &c%d &gWis: &c%d &gStr: &c%d &gCon: &c%d &gDex: &c%d &gCha: &c%d&n\r\n",
	      spell_info[spellnum].intl[i], spell_info[spellnum].wis[i], 
	      spell_info[spellnum].str[i], spell_info[spellnum].con[i], 
	      spell_info[spellnum].dex[i], spell_info[spellnum].cha[i]); 
      strcat(buf,buf2);

      sprintf(buf2, "Class Effeciency: &R%d&n%% Class Mana Percentage: &R%d&n%%&n\r\n\r\n",
	      spell_info[spellnum].spell_effec[i],
	      spell_info[spellnum].mana_perc[i]);
      strcat(buf,buf2);
    }
  }
  page_string(ch->desc,buf,TRUE);
}
