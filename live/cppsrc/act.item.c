/*
************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "corpses.h"
#include "dg_scripts.h"
#include "genzon.h"
#include "quest.h"

/* extern variables */
extern room_rnum donation_room_1;
#if 0
extern room_rnum donation_room_2;  /* uncomment if needed! */
extern room_rnum donation_room_3;  /* uncomment if needed! */
#endif
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct index_data *obj_index;	/* index table for object file	 */
extern CorpseData corpseData;

/* extern functions */
int check_environment_effect(struct char_data *ch); 
struct char_data *get_player_by_id(long idnum);
void send_to_zone(const char *, zone_rnum);
int handle_quest_item_take(struct char_data *ch, struct obj_data *obj);
ACMD(do_qcomm);

/* local functions */
int isPlayerFriend(struct char_data *ch, long idnumOwner);
void perform_repair(struct char_data *ch, struct obj_data *obj);
int can_take_obj(struct char_data * ch, struct obj_data * obj);
void get_check_money(struct char_data * ch, struct obj_data * obj);
int perform_get_from_room(struct char_data * ch, struct obj_data * obj);
void get_from_room(struct char_data * ch, char *arg, int amount);
void perform_give_gold(struct char_data * ch, struct char_data * vict, int amount);
void perform_give(struct char_data * ch, struct char_data * vict, struct obj_data * obj);
int perform_drop(struct char_data * ch, struct obj_data * obj, byte mode, const char *sname, room_rnum RDR);
void perform_drop_gold(struct char_data * ch, int amount, byte mode, room_rnum RDR);
struct char_data *give_find_vict(struct char_data * ch, char *arg);
void weight_change_object(struct obj_data * obj, int weight);
void perform_put(struct char_data * ch, struct obj_data * obj, struct obj_data * cont);
/* Artus> I think these should go..
void name_from_drinkcon(struct obj_data * obj);
void name_to_drinkcon(struct obj_data * obj, int type); */
void get_from_container(struct char_data * ch, struct obj_data * cont, char *arg, int mode, int amount, int *corpseModified);
void wear_message(struct char_data * ch, struct obj_data * obj, int where);
void perform_wear(struct char_data * ch, struct obj_data * obj, int where);
int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
void perform_get_from_container(struct char_data * ch, struct obj_data * obj, struct obj_data * cont, int mode, int *corpseModified);
void perform_remove(struct char_data * ch, int pos);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);

/* Artus> Futureworld Bus. */
void move_bus(void)
{
  static int loc = 5;
  struct obj_data *obj = NULL;
  room_rnum rd, rs;

  switch (loc)
  {
    case 0: /* Angel Grove */
      rd = real_room(20002);
      rs = real_room(11698);
      send_to_room("The 406 bus pulls up at the curb.\r\n", rd);
      send_to_room("The bus has stopped at angel grove.\r\n"
	           "The door opens.\r\n", rs);
      send_to_room("The bus has stopped at angel grove.\r\n"
	           "The door at the front of the bus opens.\r\n",
		   real_room(11699));
      obj = read_object(11693, VIRTUAL);
      GET_OBJ_VAL(obj, 0) = 20002;
      GET_OBJ_VAL(obj, 1) = 50;
      GET_OBJ_VAL(obj, 3) = 0;
      GET_OBJ_LEVEL(obj) = 50;
      obj_to_room(obj, rs);
      obj = read_object(11694, VIRTUAL);
      GET_OBJ_VAL(obj, 3) = 10000;
      obj_to_room(obj, rd);
      loc++;
      return;
    case 1: /* En Route -> Concert Hall */
      rd = real_room(20002);
      rs = real_room(11698);
      send_to_room("The bus door closes, and the bus pulls away from the curb.\r\n", rd);
      send_to_room("The bus door closes.  You feel the bus begin to move.\r\n",
	           rs);
      send_to_room("You feel the bus begin to move beneath you.\r\n",
	           real_room(11699));
      if ((obj = get_obj_in_list_num(real_object(11693), world[rs].contents)))
      { /* Remove the bus door */
	obj_from_room(obj);
	extract_obj(obj);
      }
      if ((obj = get_obj_in_list_num(real_object(11694), world[rd].contents)))
      { /* Remove the bus */
	obj_from_room(obj);
	extract_obj(obj);
      }
      loc++;
      return;
    case 2: /* At Concert Hall */
      rd = real_room(11600);
      rs = real_room(11698);
      send_to_room("The 406 bus pulls up at the curb.\r\n", rd);
      send_to_room("The bus has stopped at the city hall.\r\n"
	           "The door opens.\r\n", rs);
      send_to_room("The bus has stopped at the city hall.\r\n"
	           "The door at the front of the bus opens.\r\n",
		   real_room(11699));
      obj = read_object(11693, VIRTUAL);
      GET_OBJ_VAL(obj, 0) = 11600;
      GET_OBJ_VAL(obj, 1) = 50;
      GET_OBJ_VAL(obj, 3) = 5000;
      GET_OBJ_LEVEL(obj) = 50;
      obj_to_room(obj, rs);
      obj = read_object(11694, VIRTUAL);
      GET_OBJ_VAL(obj, 3) = 5000;
      obj_to_room(obj, rd);
      loc++;
      return;
    case 3: /* En Route -> Hollywood */
      rd = real_room(11600);
      rs = real_room(11698);
      send_to_room("The bus door closes, and the bus pulls away from the curb.\r\n", rd);
      send_to_room("The bus door closes.  You feel the bus begin to move.\r\n",
	           rs);
      send_to_room("You feel the bus begin to move beneath you.\r\n",
	           real_room(11699));
      if ((obj = get_obj_in_list_num(real_object(11693), world[rs].contents)))
      { /* Remove the bus door */
	obj_from_room(obj);
	extract_obj(obj);
      }
      if ((obj = get_obj_in_list_num(real_object(11694), world[rd].contents)))
      { /* Remove the bus */
	obj_from_room(obj);
	extract_obj(obj);
      }
      loc++;
      return;
    case 4: /* Hollywood */
      rd = real_room(26100);
      rs = real_room(11698);
      send_to_room("The 406 bus pulls up at the curb.\r\n", rd);
      send_to_room("The bus has stopped at hollywood.\r\n"
	           "The door opens.\r\n", rs);
      send_to_room("The bus has stopped at the city hall.\r\n"
	           "The door at the front of the bus opens.\r\n",
		   real_room(11699));
      obj = read_object(11693, VIRTUAL);
      GET_OBJ_VAL(obj, 0) = 26100;
      GET_OBJ_VAL(obj, 1) = 80;
      GET_OBJ_VAL(obj, 3) = 10000;
      GET_OBJ_LEVEL(obj) = 80;
      obj_to_room(obj, rs);
      obj = read_object(11694, VIRTUAL);
      GET_OBJ_VAL(obj, 3) = 0;
      obj_to_room(obj, rd);
      loc++;
      return;
    case 5: /* En Route - Angel Grove */
      rd = real_room(26100);
      rs = real_room(11698);
      send_to_room("The bus door closes, and the bus pulls away from the curb.\r\n", rd);
      send_to_room("The bus door closes.  You feel the bus begin to move.\r\n",
	           rs);
      send_to_room("You feel the bus begin to move beneath you.\r\n",
	           real_room(11699));
      if ((obj = get_obj_in_list_num(real_object(11693), world[rs].contents)))
      { /* Remove the bus door */
	obj_from_room(obj);
	extract_obj(obj);
      }
      if ((obj = get_obj_in_list_num(real_object(11694), world[rd].contents)))
      { /* Remove the bus */
	obj_from_room(obj);
	extract_obj(obj);
      }
      loc = 0;
      return;
  }
  sprintf(buf, "SYSERR: Made it to end of move_bus() without doing anything! [%d]", loc);
  loc = 0;
  mudlog(buf, BRF, LVL_GOD, TRUE); 
}

/* JA 2/4/95 */
void move_ship(void)
{
  extern struct descriptor_data *descriptor_list;
  extern struct zone_data *zone_table;
  static int loc = 0;
  room_vnum chrv;
  struct obj_data *obj;
  struct descriptor_data *d;
  struct char_data *ch;
  const char *pship_messages[][3] =
  { // Room, Zone-Haven, Zone-Danger
    {"The crew tie up and roll out the gangway.\r\n",
     "A cannon fires as the pirate ship arrives at Haven wharf.\r\n",
     "A bell tolls, the ship has dropped anchor at Haven wharf.\r\n"
    },
    {"The crew remove the gangway and weigh the anchor.\r\n",
     "A cannon fires as the Pirate ship sails away from Haven wharf.\r\n",
     "A cannon fires as the Pirate ship sails away from Haven wharf.\r\n"
    },
    {"The crew tie up and roll out the gangway.\r\n",
     "You hear the sound of a far off cannon shot. This Pirate ship has arrived.\r\n",
     "A cannon fires, the ship has dropped anchor at the island.\r\n"
    },
    {"The crew tie remove the gangway and weigh anchor.\r\n",
     "You hear the echo of a distant cannon shot. The Pirate ship has left the island.\r\n",
     "The ship has weighed anchor and sailed away from the island.\r\n"
    }
  };
  for (d=descriptor_list; d; d=d->next)
  {
    ch = d->character;
    if (!(ch) || (STATE(d) != CON_PLAYING) || IS_NPC(ch))
      continue;
    chrv = world[IN_ROOM(ch)].number;
    switch (zone_table[world[IN_ROOM(ch)].zone].number)
    {
      case 88: // Danger Island
	if (loc < 2) 
	  break;
	if (chrv == 8854)
	{
	  send_to_char(pship_messages[loc][0], ch);
	  break;
	}
	if (!EXT_FLAGGED(ch, EXT_NOHINTS))
	  send_to_char(pship_messages[loc][2], ch);
	break;
      case 11: // Haven
	if (loc > 1)
	  break;
	if (!EXT_FLAGGED(ch, EXT_NOHINTS))
	  send_to_char(pship_messages[loc][1], ch);
	break;
      case 86: // Pirate Ship
	if (loc < 2)
	{
	  if ((chrv == 8690) || (chrv == 8669))
	  {
	    send_to_char(pship_messages[loc][0], ch);
	    break;
	  }
	} else {
	  if (chrv == 8629)
	  {
	    send_to_char(pship_messages[loc][0], ch);
	    break;
	  }
	}
	if (!EXT_FLAGGED(ch, EXT_NOHINTS))
	  send_to_char(pship_messages[loc][2], ch);
	break;
      default: // Anywhere else.
	break;
    }
  }
  switch (loc)
  {
    case 0:              /* arrive to haven */
      obj_to_room(read_object(8615, VIRTUAL), real_room(8690)); 
      obj_to_room(read_object(8616, VIRTUAL), real_room(8669)); 
      loc = 1;
      return;
    case 1:              /* en-route to island */
      if ((obj = get_obj_in_list_num(real_object(8615), world[real_room(8690)].contents)))
      {     /* remove the ship */
        obj_from_room(obj);
        extract_obj(obj);
      }     /* remove the gangway */
      if ((obj = get_obj_in_list_num(real_object(8616), world[real_room(8669)].contents)))
      {
        obj_from_room(obj);
        extract_obj(obj);
      }
      loc = 2;
      return;
    case 2:              /*  at island */
      obj_to_room(read_object(8617, VIRTUAL), real_room(8854)); 
      obj_to_room(read_object(8618, VIRTUAL), real_room(8629));
      loc = 3;
      return;
    case 3:              /* en-route to haven */
      if ((obj = get_obj_in_list_num(real_object(8617), world[real_room(8854)].contents)))
      {
        obj_from_room(obj);
        extract_obj(obj);
      }
      if ((obj = get_obj_in_list_num(real_object(8618), world[real_room(8629)].contents)))
      {
        obj_from_room(obj);
        extract_obj(obj);
      }
      loc = 0;
      return;
  }
}

void move_falcon(void)
{
  static int loc = 2;
  struct obj_data *obj;

  switch (loc)
  {
    case 0 :              /* arrive to haven */
      send_to_room("\n\rThe Millenium Falcon has landed", real_room(25019));
      send_to_room("\n\rThe Millenium Falcon has landed", real_room(25044));
      send_to_room("\n\rThe exit ramp automatically opens.", real_room(25044));
      obj_to_room(read_object(25015, VIRTUAL), real_room(25019)); 
      obj_to_room(read_object(25016, VIRTUAL), real_room(25044)); 
      loc = 1;
      return;
    case 1 :              /* en-route to island */
      send_to_room("\n\rThe Millenium Falcon takes off!", real_room(25019));
      send_to_room("\n\rThe Millenium Falcon takes off!", real_room(25044));
      if ((obj = get_obj_in_list_num(real_object(25015), world[real_room(25019)].contents)))
      {     /* remove the ship */
        obj_from_room(obj);
        extract_obj(obj);
      }     /* remove the gangway */
      if ((obj = get_obj_in_list_num(real_object(25016), world[real_room(25044)].contents)))
      {
        obj_from_room(obj);
        extract_obj(obj);
      }
      loc = 2;
      return;
    case 2 :              /*  at island */
      send_to_room("\n\rThe Millenium Falcon has landed", real_room(22036));
      send_to_room("\n\rThe Millenium Falcon has landed", real_room(25044));
      send_to_room("\n\rThe exit ramp automatically opens.", real_room(25044));
      obj_to_room(read_object(25015, VIRTUAL), real_room(22036)); 
      obj_to_room(read_object(25017, VIRTUAL), real_room(25044));
      loc = 3;
      return;
    case 3 :              /* en-route to haven */
      send_to_room("\n\rThe Millenium Falcon takes off!", real_room(22036));
      send_to_room("\n\rThe Millenium Falcon takes off!", real_room(25044));
      if ((obj = get_obj_in_list_num(real_object(25015), world[real_room(22036)].contents)))
      {
        obj_from_room(obj);
        extract_obj(obj);
      }
      if ((obj = get_obj_in_list_num(real_object(25017), world[real_room(25044)].contents)))
      {
        obj_from_room(obj);
        extract_obj(obj);
      }
      loc = 0;
      return;
  }
}
/*
 * Returns TRUE if the idnum of ch is a friend of the player for idnumOwner.
 * First the player table in memory is checked, then we resort to file.
 */
int isPlayerFriend(struct char_data *ch, long idnumOwner)
{
  struct char_data *owner = NULL;
  struct char_file_u tmp_store;
  int i;
        
  // First check player table in memory
  if ((owner = get_player_by_id(idnumOwner)))
    for (i = 0; i < MAX_FRIENDS; i++)
      if (GET_FRIEND(owner, i) == GET_IDNUM(ch))
        return TRUE;
  // Now resort to file
  if (load_char(get_name_by_id(idnumOwner), &tmp_store))
    for (i = 0; i < MAX_FRIENDS; i++)
      if (tmp_store.player_specials_primalsaved.friends[i] == GET_IDNUM(ch))
        return TRUE;
  return FALSE;
}

/* ARMOURCRAFT and WEAPONCRAFT */
ACMD(do_repair)
{
  struct obj_data *o;

  one_argument(argument, buf);
  if (!*buf)
  {
    send_to_char("Repair what?!\r\n", ch);
    return;
  }
  if (!(o = generic_find_obj(ch, buf, FIND_OBJ_INV)))
  {
    send_to_char("You don't seem to have that item.\r\n", ch);
    return;
  }
  if ((GET_OBJ_TYPE(o) == ITEM_WEAPON && !GET_SKILL(ch, SKILL_WEAPONCRAFT)) ||
      (GET_OBJ_TYPE(o) == ITEM_ARMOR  && !GET_SKILL(ch, SKILL_ARMOURCRAFT)))
  {
    send_to_char("You don't have the appropriate skill"
                 " to repair that item.\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(o) != ITEM_WEAPON && GET_OBJ_TYPE(o) != ITEM_ARMOR)
  {
    send_to_char("That item cannot be repaired by normal means.\r\n", ch);
    return;
  }
  // Check the damage status of the item
  if ((GET_OBJ_MAX_DAMAGE(o) == -1) ||
      (GET_OBJ_DAMAGE(o) == GET_OBJ_MAX_DAMAGE(o)))
  {
    send_to_char("It's in perfect condition already.\r\n", ch);
    return;
  }
  // Do a check here for some items that might be required to repair
  // ie - anvil, hammer, etc
  perform_repair(ch, o);	
}

void perform_repair(struct char_data *ch, struct obj_data *obj)
{
  int skillAmt = (GET_OBJ_TYPE(obj) == ITEM_WEAPON ? 
                  GET_SKILL(ch, SKILL_WEAPONCRAFT) :
	          GET_SKILL(ch, SKILL_ARMOURCRAFT));
  int modifier = 0, str = GET_STR(ch), con = GET_CON(ch);
  float maxAmt = GET_OBJ_MAX_DAMAGE(obj) / 10;
	
  if (maxAmt < 1)
    maxAmt = 1;	// Minimum of at least 1 point up or down
  modifier += (str >= 21 ? 3 : (str >= 18 ? 2 : (str >= 15 ? 1 : 
              (str >= 10 ? 0 : str >= 7 ? -1 : -2))));
  modifier += (con >= 21 ? 3 : (con >= 18 ? 2 : (con >= 15 ? 1 : 
              (con >= 10 ? 0 : con >= 7 ? -1 : -2))));
  skillAmt += modifier;
  // Skill amount is now a percentage, max is 95
  if (skillAmt > MAX_SKILL_ABIL)
    skillAmt = MAX_SKILL_ABIL;
  if (number(1, 100) > skillAmt)
  {
    // Fucked up, damaged the item
    act("&r$n strikes badly, further damaging $p!&n", 
	FALSE, ch, obj, 0, TO_ROOM);
    act("&rYou struck badly, damaging $p!&n", FALSE, ch, obj, 0, TO_CHAR);
    GET_OBJ_DAMAGE(obj) -= (sh_int)maxAmt;
  } else {	// Did a repair on the sucker 
    act("&g$n strikes truely, repairing $p.&n", FALSE, ch, obj, 0, TO_ROOM);
    act("&gYou strike well, repairing $p.&n", FALSE, ch, obj, 0, TO_CHAR);
    GET_OBJ_DAMAGE(obj) += (sh_int)maxAmt;
  }
  // Some checking
  if ((GET_OBJ_DAMAGE(obj) <= 0) && (GET_OBJ_MAX_DAMAGE(obj) != -1))
  {
    act("&r... $p is destroyed completely.&n", FALSE, ch, obj, 0, TO_ROOM);
    act("&r... you destroyed $p completely.&n", FALSE, ch, obj, 0, TO_CHAR);
    extract_obj(obj);
  }
  if ((GET_OBJ_DAMAGE(obj) > GET_OBJ_MAX_DAMAGE(obj)) || 
      (GET_OBJ_MAX_DAMAGE(obj) == -1))
    GET_OBJ_DAMAGE(obj) = GET_OBJ_MAX_DAMAGE(obj);
}

void perform_put(struct char_data * ch, struct obj_data * obj,
		 struct obj_data * cont)
{
  int House_can_enter(struct char_data * ch, room_vnum house);
  if (!drop_otrigger(obj, ch, SCMD_IS_PUT))
    return;
  if (OBJ_RIDDEN(obj))
  {
    act("$p - You're riding that!\r\n", FALSE, ch, obj, NULL, TO_CHAR);
    return;
  }
  if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0))
  {
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
  {
    act("$p - You can't put a container inside a container!", FALSE, ch, obj, NULL, TO_CHAR);
    return;
  }
  if ((cont->carried_by != ch) && IS_OBJ_STAT(obj, ITEM_NODROP) &&
      !House_can_enter(ch, world[IN_ROOM(ch)].number))
  {
    act("You can't seem to let go of $p.", TRUE, ch, obj, NULL, TO_CHAR);
    return;
  }
  /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
  /* Artus> gg, you am teh sux0r. We'll check when we go to drop, in stead.
  if (IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_OBJ_STAT(cont, ITEM_NODROP)) {
    SET_BIT(GET_OBJ_EXTRA(cont), ITEM_NODROP);
    act("You get a strange feeling as you put $p in $P.", FALSE,
	      ch, obj, cont, TO_CHAR);
    return;
  } else */
  // Artus> Curse objects put inside cursed container.
  if (IS_OBJ_STAT(cont, ITEM_NODROP) && !IS_OBJ_STAT(obj, ITEM_NODROP)) 
  {
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
    act("You get a strange feeling as you put $p in $P.", FALSE, ch, obj,
	cont, TO_CHAR);
  } else {
    act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
    act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
  }
  obj_from_char(obj);
  obj_to_obj(obj, cont);
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
  char *theobj, *thecont;

  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);

  if (*arg3 && is_number(arg1))
  {
    howmany = atoi(arg1);
    theobj = arg2;
    thecont = arg3;
  } else {
    theobj = arg1;
    thecont = arg2;
  }
  obj_dotmode = find_all_dots(theobj);
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char("Put what in what?\r\n", ch);
  else if (cont_dotmode != FIND_INDIV)
    send_to_char("You can only put things into one container at a time.\r\n",
	         ch);
  else if (!*thecont)
  {
    sprintf(buf, "What do you want to put %s in?\r\n",
	    ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
    send_to_char(buf, ch);
  } else {
    generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
    if (!cont)
    {
      sprintf(buf, "You don't see %s %s here.\r\n", AN(thecont), thecont);
      send_to_char(buf, ch);
    } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
    else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
      send_to_char("You'd better open it first!\r\n", ch);
    else
    {
      if (obj_dotmode == FIND_INDIV)
      {	/* put <obj> <container> */
	if (!(obj = generic_find_obj(ch, theobj, FIND_OBJ_INV)))
	{
	  sprintf(buf, "You aren't carrying %s %s.\r\n", AN(theobj), theobj);
	  send_to_char(buf, ch);
	} else if (obj == cont)
	  send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
	else
	{
	  struct obj_data *next_obj;
	  while(obj && howmany--)
	  {
	    next_obj = obj->next_content;
	    perform_put(ch, obj, cont);
	    obj = find_obj_list(ch, theobj, next_obj);
	  }
	}
      } else {
	for (obj = ch->carrying; obj; obj = next_obj)
	{
	  next_obj = obj->next_content;
	  if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
	      (obj_dotmode == FIND_ALL || isname(theobj, obj->name)))
	  {
	    found = 1;
	    perform_put(ch, obj, cont);
	  }
	}
	if (!found)
	{
	  if (obj_dotmode == FIND_ALL)
	    send_to_char("You don't seem to have anything to put in it.\r\n",
		         ch);
	  else
	  {
	    sprintf(buf, "You don't seem to have any %ss.\r\n", theobj);
	    send_to_char(buf, ch);
	  }
	}
      }
    }
  }
}

int can_take_obj(struct char_data * ch, struct obj_data * obj)
{
  struct event_data *find_quest_event();

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } 
  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
  {
    act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } 
  if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)))
  {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  // Artus> Quest Items ala QUEST_ITEM_HUNT.
  if (GET_OBJ_TYPE(obj) == ITEM_QUEST)
    return (handle_quest_item_take(ch, obj));
  return (1);
}

void get_check_money(struct char_data * ch, struct obj_data * obj)
{
  int value = GET_OBJ_VAL(obj, 0);

  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;
  obj_from_char(obj);
  extract_obj(obj);
  GET_GOLD(ch) += value;
  if (value == 1)
    send_to_char("There was &Y1&n coin.\r\n", ch);
  else
  {
    sprintf(buf, "There were &Y%d&n coins.\r\n", value);
    send_to_char(buf, ch);
  }
}

void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
				struct obj_data * cont, int mode, 
				int *corpseModified)
{
  if ((mode == FIND_OBJ_INV) || can_take_obj(ch, obj))
  {
    if (GET_OBJ_TYPE(obj) == ITEM_QUEST)
    {
      handle_quest_item_take(ch, obj);
      return;
    }
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
    else if (get_otrigger(obj, ch))
    {
      // Dont let NPCS touch PC corpses
      if (IS_NPC(ch))
      {
        send_to_char("Bugger off!", ch);
        return;
      }
      // DM - set corpseModified value indicating to save the CorpseData when 
      // we return to do_get function 
      if (GET_CORPSEID(cont) > 0)
      {
        *corpseModified = 1;
        sprintf(buf, "%s got obj %s from corpse of %s", GET_NAME(ch), 
		obj->name, get_name_by_id(GET_CORPSEID(cont))); 
        mudlog(buf, CMP, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE); 
      }
      obj_from_obj(obj);
      obj_to_char(obj, ch, __FILE__, __LINE__);
      act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      get_check_money(ch, obj);
    }
  }
}


void get_from_container(struct char_data * ch, struct obj_data * cont,
			char *arg, int mode, int howmany,
			int *corpseModified)
{
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  // DM: check if container is corpse, and if ch is allowed to modify it
  if (GET_CORPSEID(cont) > 0)
  {
    if (GET_IDNUM(ch) != GET_CORPSEID(cont) && LR_FAIL(ch, LVL_ANGEL) && 
	!isPlayerFriend(ch, GET_CORPSEID(cont)))
    {
      sprintf(buf, "Umm, you'd better not touch that. Ask &7%s&n for permission.", get_name_by_id(GET_CORPSEID(cont)));
      send_to_char(buf, ch);
      return;
    }  
  }
  obj_dotmode = find_all_dots(arg);
  if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV)
  {
    if (!(obj = find_obj_list(ch, arg, cont->contains)))
    {
      sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg), arg);
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
    } else {
      struct obj_data *obj_next;
      while(obj && howmany--)
      {
        obj_next = obj->next_content;
        perform_get_from_container(ch, obj, cont, mode, corpseModified);
        obj = find_obj_list(ch, arg, obj_next);
      }
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg)
    {
      send_to_char("Get all of what?\r\n", ch);
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (obj_dotmode == FIND_ALL || isname(arg, obj->name)))
      {
	found = 1;
	perform_get_from_container(ch, obj, cont, mode, corpseModified);
	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
	{
	  send_to_char("Your arms are now full.\r\n", ch);
	  break;
	}
      }
    }
    if (!found)
    {
      if (obj_dotmode == FIND_ALL)
	act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      else
      {
	sprintf(buf, "You can't seem to find any %ss in $p.", arg);
	act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
}

int perform_get_from_room(struct char_data * ch, struct obj_data * obj)
{
  if (can_take_obj(ch, obj) && get_otrigger(obj, ch))
  {
    int sleight = 0;
    // DM: check if obj is corpse, and if ch is allowed to modify it
    if ((GET_CORPSEID(obj) > 0) && (GET_IDNUM(ch) != GET_CORPSEID(obj)) && 
	LR_FAIL(ch, LVL_IS_GOD) && !isPlayerFriend(ch, GET_CORPSEID(obj)))
    {
      sprintf(buf, "Umm, you'd better not touch that. Ask &7%s&n for permission.", get_name_by_id(GET_CORPSEID(obj)));
      send_to_char(buf, ch);
      return(0);
    } 
    if (!IS_NPC(ch) && GET_SKILL(ch,SKILL_SLEIGHT) && 
	has_stats_for_skill(ch,SKILL_SLEIGHT,FALSE) &&
	(number(0, 101) < GET_SKILL(ch, SKILL_SLEIGHT)) &&
	(CAN_CARRY_W(ch) > (GET_OBJ_WEIGHT(obj) * 3)))
      sleight = 1;
    obj_from_room(obj);
    obj_to_char(obj, ch, __FILE__, __LINE__);
    if (sleight == 0)
    {
      act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
    } else {
      act("You get $p while noone is looking.", FALSE, ch, obj, 0, TO_CHAR);
      apply_spell_skill_abil(ch, SKILL_SLEIGHT);
    }
    get_check_money(ch, obj);
    return (1);
  }
  return (0);
}

void get_from_room(struct char_data * ch, char *arg, int howmany)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV)
  {
    if (!(obj = find_obj_list(ch, arg, world[IN_ROOM(ch)].contents)))
    {
      sprintf(buf, "You don't see %s %s here.\r\n", AN(arg), arg);
      send_to_char(buf, ch);
    } else {
      struct obj_data *obj_next;
      while(obj && howmany--)
      {
	obj_next = obj->next_content;
        perform_get_from_room(ch, obj);
        obj = find_obj_list(ch, arg, obj_next);
      }
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*arg)
    {
      send_to_char("Get all of what?\r\n", ch);
      return;
    }
    for (obj = world[ch->in_room].contents; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  ((dotmode == FIND_ALL) || isname(arg, obj->name)))
      {
	found = 1;
	perform_get_from_room(ch, obj);
      }
    }
    if (!found)
    {
      if (dotmode == FIND_ALL)
	send_to_char("There doesn't seem to be anything here.\r\n", ch);
      else
      {
	sprintf(buf, "You don't see any %ss here.\r\n", arg);
	send_to_char(buf, ch);
      }
    }
  }
}

ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;
  int corpseModified = 0;

  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);
/*  int sleight = 0;		// Sleighting?
  if (!IS_NPC(ch))
    if (GET_SKILL(ch, SKILL_SLEIGHT) && 
        has_stats_for_skill(ch, SKILL_SLEIGHT,FALSE)) 
      if (number(0, 101) < GET_SKILL(ch, SKILL_SLEIGHT))
	sleight := 1; */
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    send_to_char("Your arms are already full!\r\n", ch);
  else if (!*arg1)
    send_to_char("Get what?\r\n", ch);
  else if (!*arg2)
    get_from_room(ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(ch, arg2, atoi(arg1));
  else
  {
    int amount = 1;
    if (is_number(arg1))
    {
      amount = atoi(arg1);
      strcpy(arg1, arg2);
      strcpy(arg2, arg3);
    }
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV)
    {
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
      if (!cont)
      {
	sprintf(buf, "You don't have %s %s.\r\n", AN(arg2), arg2);
	send_to_char(buf, ch);
      } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
	act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else {
	get_from_container(ch, cont, arg1, mode, amount, &corpseModified);
        // DM - call addCorpse to update the data for this corpse and save
        if (corpseModified == 1)
	{
          corpseData.addCorpse(cont, GET_ROOM_VNUM(ch->in_room), 0);
          save_char(ch, NOWHERE);
        }
      }
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2)
      {
	send_to_char("Get from all of what?\r\n", ch);
	return;
      }
      for (cont = ch->carrying; cont; cont = cont->next_content)
      {
        corpseModified = 0;
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
	{
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
	  {
	    found = 1;
	    get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount, 
                            &corpseModified);
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    found = 1;
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	  }
	}
        // DM - call addCorpse to update the data for this corpse and save
        if (corpseModified == 1)
	{
          corpseData.addCorpse(cont, GET_ROOM_VNUM(ch->in_room), 0);
          save_char(ch, NOWHERE);
        }
      }
      for (cont = world[ch->in_room].contents; cont; cont = cont->next_content)
      {
        corpseModified = 0;
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
	{
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
	  {
	    get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount, 
			       &corpseModified);
	    found = 1;
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	    found = 1;
	  }
	}
	// DM - call addCorpse to update the data for this corpse and save
	if (corpseModified == 1)
	{
          corpseData.addCorpse(cont, GET_ROOM_VNUM(ch->in_room), 0);
          save_char(ch, NOWHERE);
        }
      }
      if (!found)
      {
	if (cont_dotmode == FIND_ALL)
	  send_to_char("You can't seem to find any containers.\r\n", ch);
	else
	{
	  sprintf(buf, "You can't seem to find any %ss here.\r\n", arg2);
	  send_to_char(buf, ch);
	}
      }
    }
  }
}

void perform_drop_gold(struct char_data * ch, int amount, byte mode,
                       room_rnum RDR)
{
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
  else if (GET_GOLD(ch) < amount)
    send_to_char("You don't have that many coins!\r\n", ch);
  else
  {
    if (mode != SCMD_JUNK)
    {
      WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
      obj = create_money(amount);
      if (mode == SCMD_DONATE)
      {
	sprintf(buf, "You throw some gold into the %s where it disappears in a puff of smoke!\r\n", (UNDERWATER(ch)) ? "water" : "air");
	send_to_char(buf, ch);
	sprintf(buf, "$n throws some gold into the %s where it disappears in a puff of smoke!", (UNDERWATER(ch)) ? "water" : "air");
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, RDR);
	act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        if (!drop_wtrigger(obj, ch))
	{
          extract_obj(obj);
          return;
        }
	send_to_char("You drop some gold.\r\n", ch);
	sprintf(buf, "$n drops %s.", money_desc(amount));
	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, ch->in_room);
      }
    } else {
      sprintf(buf, "$n drops %s which disappears in a puff of smoke!",
	      money_desc(amount));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You drop some gold which disappears in a puff of smoke!\r\n", ch);
    }
    GET_GOLD(ch) -= amount;
  }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")
int perform_drop(struct char_data * ch, struct obj_data * obj,
		     byte mode, const char *sname, room_rnum RDR)
{
  int value;
  if (!drop_otrigger(obj, ch, mode))
    return 0;
  if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
    return 0;
  if (IS_OBJ_STAT(obj, ITEM_NODROP) && LR_FAIL(ch,LVL_ANGEL)) 
  {
    sprintf(buf, "You can't %s $p, it must be CURSED!", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  if (!IS_NPC(ch) && (GET_OBJ_TYPE(obj) == ITEM_REWARD) && (mode != SCMD_JUNK))
  {
    send_to_char("You can't drop rewards!\r\n", ch);
    return (0);
  }
  if (OBJ_RIDDEN(obj))
  {
    sprintf(buf, "You can't %s $p, it's being ridden!", sname);
    act( buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && (obj->contains) && 
      LR_FAIL(ch, LVL_ANGEL))
  {
    if ((mode == SCMD_JUNK) || (mode == SCMD_DONATE))
    {
      sprintf(buf, "You must empty $p before you %s it!\r\n", sname);
      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
    for (struct obj_data *myobj = obj->contains; myobj; 
	 myobj = myobj->next_content)
    {
      if (OBJ_FLAGGED(myobj, ITEM_NODROP))
      {
	sprintf(buf, "You can't %s $p, it contains cursed items!", sname);
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	return(0);
      }
      if (!IS_NPC(ch) && (GET_OBJ_TYPE(obj) == ITEM_REWARD) && 
	  (mode != SCMD_JUNK))
      {
	sprintf(buf, "You can't %s $p, it contains rewards!", sname);
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	return(0);
      }
    }
    sprintf(buf, "You drop $p. You hear something rattle inside.");
  } else
    sprintf(buf, "You %s $p.%s", sname, VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);
  sprintf(buf, "$n %ss $p.%s", sname, VANISH(mode));
  act(buf, TRUE, ch, obj, 0, TO_ROOM);
  obj_from_char(obj);

  if ((mode == SCMD_DONATE) && IS_OBJ_STAT(obj, ITEM_NODONATE))
    mode = SCMD_JUNK;

  switch (mode)
  {
    case SCMD_DROP:
      obj_to_room(obj, ch->in_room);
      return (0);
    case SCMD_DONATE:
      obj_to_room(obj, RDR);
      act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
      return (0);
    case SCMD_JUNK:
      value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
      extract_obj(obj);
      return (value);
    default:
      basic_mud_log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
      break;
  }
  return (0);
}

ACMD(do_drop)
{
  struct obj_data *obj, *next_obj;
  room_rnum RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0, multi;
  const char *sname;

  switch (subcmd)
  {
    case SCMD_JUNK:
      sname = "junk";
      mode = SCMD_JUNK;
      break;
    case SCMD_DONATE:
      sname = "donate";
      mode = SCMD_DONATE;
      switch (number(0, 2))
      {
	case 0:
	  mode = SCMD_JUNK;
	  break;
	case 1:
	case 2:
	  RDR = real_room(donation_room_1);
	  break;
      }
      if (RDR == NOWHERE)
      {
	send_to_char("Sorry, you can't donate anything right now.\r\n", ch);
	return;
      }
      break;
    default:
      sname = "drop";
      break;
  }

  argument = one_argument(argument, arg);

  if (!*arg)
  {
    sprintf(buf, "What do you want to %s?\r\n", sname);
    send_to_char(buf, ch);
    return;
  } else if (is_number(arg)) {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
      perform_drop_gold(ch, multi, mode, RDR);
    else if (multi <= 0)
      send_to_char("Yeah, that makes sense.\r\n", ch);
    else if (!*arg)
    {
      sprintf(buf, "What do you want to %s %d of?\r\n", sname, multi);
      send_to_char(buf, ch);
    } else if (!(obj = find_obj_list(ch, arg, ch->carrying))) {
      sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
      send_to_char(buf, ch);
    } else {
      do
      {
        next_obj = find_obj_list(ch, arg, obj->next_content);
        amount += perform_drop(ch, obj, mode, sname, RDR);
        obj = next_obj;
      } while (obj && --multi);
    }
  } else {
    dotmode = find_all_dots(arg);
    /* Can't junk or donate all, DM: unless you are lvl immort + */
    if ((dotmode == FIND_ALL) && 
	(subcmd == SCMD_JUNK || subcmd == SCMD_DONATE) &&
        LR_FAIL(ch, LVL_CHAMP))
    {
      if (subcmd == SCMD_JUNK)
	send_to_char("Go to the dump if you want to junk EVERYTHING!\r\n", ch);
      else
	send_to_char("Go do the donation room if you want to donate "
		     "EVERYTHING!\r\n", ch);
      return;
    }
    if (dotmode == FIND_ALL) 
    {
      if (!ch->carrying)
      {
	send_to_char("You don't seem to be carrying anything.\r\n", ch);
	return;
      }
      for (obj = ch->carrying; obj; obj = next_obj)
      {
	next_obj = obj->next_content;
	amount += perform_drop(ch, obj, mode, sname, RDR);
      }
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) 
      {
	sprintf(buf, "What do you want to %s all of?\r\n", sname);
	send_to_char(buf, ch);
	return;
      }
      if (!(obj = find_obj_list(ch, arg, ch->carrying))) 
      {
	sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
	send_to_char(buf, ch);
	return;
      }
      while (obj) 
      {
	next_obj = find_obj_list(ch, arg, obj->next_content);
	amount += perform_drop(ch, obj, mode, sname, RDR);
	obj = next_obj;
      }
    } else {
      if (!(obj = find_obj_list(ch, arg, ch->carrying)))
      {
	sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf, ch);
      } else
	amount += perform_drop(ch, obj, mode, sname, RDR);
    }
  }
  if (amount && (subcmd == SCMD_JUNK))
  {
    send_to_char("You have been rewarded by the gods!\r\n", ch);
    act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
    GET_GOLD(ch) += amount;
  }
}

void perform_give(struct char_data * ch, struct char_data * vict,
		  struct obj_data * obj, int subcmd)
{
  if (LR_FAIL(ch, LVL_ANGEL))
  {
    if (IS_OBJ_STAT(obj, ITEM_NODROP)) 
    {
      act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    if (!IS_NPC(ch) && GET_OBJ_TYPE(obj) == ITEM_REWARD)
    {
      send_to_char("You can't give rewards away!\r\n", ch);
      return;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
      for (struct obj_data *myobj = obj->contains; myobj; 
	   myobj = myobj->next_content)
      {
	if (OBJ_FLAGGED(myobj, ITEM_NODROP))
	{
	  act("You can't let go of $p, it contains cursed items!", FALSE, ch, obj, 0, TO_CHAR);
	  return;
	}
	if (!IS_NPC(ch) && GET_OBJ_TYPE(myobj) == ITEM_REWARD)
	{
	  act("You can't let go of $p, it contains rewards!", FALSE, ch, obj, 0, TO_CHAR);
	  return;
	}
      }
  }
  if (OBJ_RIDDEN(obj))
  {
    send_to_char("You can't give that, it's being ridden!\r\n", ch);
    return;
  }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict))
  {
    act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
  {
    act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if ((subcmd != SCMD_SLIP) && (!give_otrigger(obj, ch, vict) || !receive_mtrigger(vict, ch, obj)))
    return;
  switch (subcmd)
  {
    case SCMD_SLIP:
      if ((basic_skill_test(ch, SKILL_SLIP, FALSE) == 0) ||
	  (number(0, GET_DEX(vict)) > GET_DEX(ch)))
      {
	act("$N catches you in the act!\r\n", FALSE, ch, obj, vict, TO_CHAR);
	act("You catch $n trying to slip $p into your pack.\r\n", FALSE, ch, obj, vict, TO_VICT);
	return;
      }
      apply_spell_skill_abil(ch, SKILL_SLIP);
      act("You slip $p into $N's pack.", FALSE, ch, obj, vict, TO_CHAR);
      if (!LR_FAIL(ch, LVL_ANGEL))
	act("$n has just slipped $p into your pack.\r\n", FALSE, ch, obj, vict, TO_VICT);
      break;
    case SCMD_GIVE:
    default:
      act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
      act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
      act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
  }
  obj_from_char(obj);
  obj_to_char(obj, vict, __FILE__, __LINE__);
}

/* utility function for give */
struct char_data *give_find_vict(struct char_data * ch, char *arg)
{
  struct char_data *vict;

  if (!*arg)
  {
    send_to_char("To who?\r\n", ch);
    return (NULL);
  } else if (!(vict = generic_find_char(ch, arg, FIND_CHAR_ROOM))) {
    send_to_char(NOPERSON, ch);
    return (NULL);
  } else if (vict == ch) {
    send_to_char("What's the point of that?\r\n", ch);
    return (NULL);
  } else
    return (vict);
}


void perform_give_gold(struct char_data * ch, struct char_data * vict,
		            int amount)
{
  if (amount <= 0)
  {
    send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || LR_FAIL(ch, LVL_GOD)))
  {
    send_to_char("You don't have that many coins!\r\n", ch);
    return;
  }
  send_to_char(OK, ch);
  sprintf(buf, "$n gives you %d gold coin%s.", amount, amount == 1 ? "" : "s");
  act(buf, FALSE, ch, 0, vict, TO_VICT);
  sprintf(buf, "$n gives %s to $N.", money_desc(amount));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || LR_FAIL(ch, LVL_GOD))
    GET_GOLD(ch) -= amount;
  GET_GOLD(vict) += amount;
  bribe_mtrigger(vict, ch, amount);
}


ACMD(do_give)
{
  int amount, dotmode;
  struct char_data *vict;
  struct obj_data *obj, *next_obj;

  argument = one_argument(argument, arg);

  if ((subcmd == SCMD_SLIP) &&
      !has_stats_for_skill(ch, SKILL_SLIP, TRUE))
    return;
  if (!*arg)
    send_to_char("Give what to who?\r\n", ch);
  else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
    {
      one_argument(argument, arg);
      if ((vict = give_find_vict(ch, arg)) != NULL)
	perform_give_gold(ch, vict, amount);
      return;
    } else if (!*arg) {	/* Give multiple code. */
      sprintf(buf, "What do you want to give %d of?\r\n", amount);
      send_to_char(buf, ch);
    } else if (!(vict = give_find_vict(ch, argument))) {
      return;
    } else if (!(obj = find_obj_list(ch, arg, ch->carrying))) {
      sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
      send_to_char(buf, ch);
    } else {
      while (obj && amount--)
      {
	next_obj = find_obj_list(ch, arg, obj->next_content);
	perform_give(ch, vict, obj, subcmd);
	obj = next_obj;
      }
    }
  } else {
    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV)
    {
      if (!(obj = find_obj_list(ch, arg, ch->carrying)))
      {
	sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf, ch);
      } else
	perform_give(ch, vict, obj, subcmd);
    } else {
      if (dotmode == FIND_ALLDOT && !*arg)
      {
	send_to_char("All of what?\r\n", ch);
	return;
      }
      if (!ch->carrying)
	send_to_char("You don't seem to be holding anything.\r\n", ch);
      else
	for (obj = ch->carrying; obj; obj = next_obj)
	{
	  next_obj = obj->next_content;
	  if (CAN_SEE_OBJ(ch, obj) &&
	      ((dotmode == FIND_ALL || isname(arg, obj->name))))
	    perform_give(ch, vict, obj, subcmd);
	}
    }
  }
}



void weight_change_object(struct obj_data * obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj->in_room != NOWHERE)
    GET_OBJ_WEIGHT(obj) += weight;
  else if ((tmp_ch = obj->carried_by))
  {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch, __FILE__, __LINE__);
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    basic_mud_log("SYSERR: Unknown attempt to subtract weight from an object.");
  }
}


/* Artus> I think these should go.
void name_from_drinkcon(struct obj_data * obj)
{
  int i;
  char *new_name;

  for (i = 0; (*((obj->name) + i) != ' ') && (*((obj->name) + i) != '\0'); i++);

  if (*((obj->name) + i) == ' ') {
    new_name = str_dup((obj->name) + i + 1);
    if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
      free(obj->name);
    obj->name = new_name;
  }
}

void name_to_drinkcon(struct obj_data * obj, int type)
{
  char *new_name;

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", drinknames[type], obj->name);
  if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);
  obj->name = new_name;
}
*/


ACMD(do_drink)
{
  struct obj_data *temp;
  struct affected_type af;
  int amount, weight;
  int on_ground = 0;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg)
  {
    send_to_char("Drink from what?\r\n", ch);
    return;
  }
  if (!(temp = find_obj_list(ch, arg, ch->carrying)))
  {
    if (!(temp = find_obj_list(ch, arg, world[ch->in_room].contents)))
    {
      send_to_char("You can't find it!\r\n", ch);
      return;
    } else
      on_ground = 1;
  }
  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN))
  {
    send_to_char("You can't drink from that!\r\n", ch);
    return;
  }
  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON))
  {
    send_to_char("You have to be holding that to drink from it.\r\n", ch);
    return;
  }
  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0))
  {
    /* The pig is drunk */
    send_to_char("You can't seem to get close enough to your mouth.\r\n", ch);
    act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0))
  {
    send_to_char("Your stomach can't contain anymore!\r\n", ch);
    return;
  }
  if (!GET_OBJ_VAL(temp, 1))
  {
    send_to_char("It's empty.\r\n", ch);
    return;
  }
  if (subcmd == SCMD_DRINK)
  {
    sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, 2)]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);
    sprintf(buf, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    send_to_char(buf, ch);
    if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
    else
      amount = number(3, 10);
  } else {
    act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
    sprintf(buf, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    send_to_char(buf, ch);
    amount = 1;
  }
  amount = MIN(amount, GET_OBJ_VAL(temp, 1));
  /* You can't subtract more than the object weighs */
  weight = MIN(amount, GET_OBJ_WEIGHT(temp));
  weight_change_object(temp, -weight);	/* Subtract amount */
  gain_condition(ch, DRUNK,
	  (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount) / 4);
  gain_condition(ch, FULL,
	  (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount) / 4);
  gain_condition(ch, THIRST,
	  (int) ((int) drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount) / 4);
  if (GET_COND(ch, DRUNK) > 10)
    send_to_char("You feel drunk.\r\n", ch);
  if (GET_COND(ch, THIRST) > 20)
    send_to_char("You don't feel thirsty any more.\r\n", ch);
  if (GET_COND(ch, FULL) > 20)
    send_to_char("You are full.\r\n", ch);
  if (GET_OBJ_VAL(temp, 3))
  {	/* The shit was poisoned ! */
    send_to_char("Oops, it tasted rather strange!\r\n", ch);
    act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);
    af.type = SPELL_POISON;
    af.duration = amount * 3;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  /* empty the container, and no longer poison. */
  GET_OBJ_VAL(temp, 1) -= amount;
  if (!GET_OBJ_VAL(temp, 1))
  {	/* The last bit */
    GET_OBJ_VAL(temp, 2) = 0;
    GET_OBJ_VAL(temp, 3) = 0;
    /* Artus> I think these should go.
    name_from_drinkcon(temp); */
  }
  return;
}

void perform_eat(struct char_data *ch, struct obj_data *food, int subcmd)
{
  int amount = GET_OBJ_VAL(food, 0);

  if ((amount > 1) && subcmd != SCMD_EAT)
    amount = 1;
  if (subcmd == SCMD_EAT) 
  {
    act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
  } else {
    act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
  }
  if (amount > 0)
  {
    gain_condition(ch, FULL, amount);
    if (GET_COND(ch, FULL) > 20)
      send_to_char("You are full.\r\n", ch);
  } else
    send_to_char("That wasn't satisfying, at all.\r\n", ch);
  
  if (GET_OBJ_VAL(food, 3) && LR_FAIL(ch, LVL_IMMORT))
  {
    struct affected_type af;
    /* The shit was poisoned ! */
    send_to_char("Oops, that tasted rather strange!\r\n", ch);
    act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);
    af.type = SPELL_POISON;
    af.duration = amount * 2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  if (subcmd == SCMD_EAT)
    extract_obj(food);
  else if (--GET_OBJ_VAL(food, 0) < 1)
  {
    send_to_char("There's nothing left now.\r\n", ch);
    extract_obj(food);
  }
}

ACMD(do_eat)
{
  struct obj_data *food;

  one_argument(argument, arg);
  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
    return;
  if (!*arg)
  {
    send_to_char("Eat what?\r\n", ch);
    return;
  }
  if (!(food = find_obj_list(ch, arg, ch->carrying)))
  {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  }
  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			       (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN)))
  {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && LR_FAIL(ch, LVL_GOD))
  {
    send_to_char("You can't eat THAT!\r\n", ch);
    return;
  }
  if (GET_COND(ch, FULL) > 20) 
  {/* Stomach full */
    send_to_char("You are too full to eat more!\r\n", ch);
    return;
  }
  perform_eat(ch, food, subcmd);
  return;
}

ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount;

  two_arguments(argument, arg1, arg2);
  if (subcmd == SCMD_POUR)
  {
    if (!*arg1)		/* No arguments */
    {
      send_to_char("From what do you want to pour?\r\n", ch);
      return;
    }
    if (!(from_obj = find_obj_list(ch, arg1, ch->carrying)))
    {
      send_to_char("You can't find it!\r\n", ch);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      send_to_char("You can't pour from that!\r\n", ch);
      return;
    }
  }
  if (subcmd == SCMD_FILL)
  {
    if (!*arg1)
    {		/* no arguments */
      send_to_char("What do you want to fill?  And what are you filling it from?\r\n", ch);
      return;
    }
    if (!(to_obj = find_obj_list(ch, arg1, ch->carrying)))
    {
      send_to_char("You can't find it!\r\n", ch);
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON)
    {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2)
    {		/* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = find_obj_list(ch, arg2, world[ch->in_room].contents)))
    {
      sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
    {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, 1) == 0)
  {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR)
  {	/* pour */
    if (!*arg2)
    {
      send_to_char("Where do you want it?  Out or in what?\r\n", ch);
      return;
    }
    if (!str_cmp(arg2, "out"))
    {
      act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
      act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */
      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
      /* Artus> I think these should go.
      name_from_drinkcon(from_obj); */
      return;
    }
    if (!(to_obj = find_obj_list(ch, arg2, ch->carrying)))
    {
      send_to_char("You can't find it!\r\n", ch);
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
	(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN))
    {
      send_to_char("You can't pour anything into that.\r\n", ch);
      return;
    }
  }
  if (to_obj == from_obj)
  {
    send_to_char("A most unproductive effort.\r\n", ch);
    return;
  }
  if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
      (GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2)))
  {
    send_to_char("There is already another liquid in it!\r\n", ch);
    return;
  }
  if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)))
  {
    send_to_char("There is no room for more.\r\n", ch);
    return;
  }
  if (subcmd == SCMD_POUR)
  {
    sprintf(buf, "You pour the %s into the %s.",
	    drinks[GET_OBJ_VAL(from_obj, 2)], arg2);
    send_to_char(buf, ch);
  }
  if (subcmd == SCMD_FILL)
  {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  /* Artus> I think these should go.
  if (GET_OBJ_VAL(to_obj, 1) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2)); */

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

  /* Then how much to pour */
  GET_OBJ_VAL(from_obj, 1) -= (amount =
			 (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

  GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

  if (GET_OBJ_VAL(from_obj, 1) < 0)
  {	/* There was too little */
    GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
    amount += GET_OBJ_VAL(from_obj, 1);
    GET_OBJ_VAL(from_obj, 1) = 0;
    GET_OBJ_VAL(from_obj, 2) = 0;
    GET_OBJ_VAL(from_obj, 3) = 0;
    /* Artus> I think these should go.
    name_from_drinkcon(from_obj); */
  }
  /* Then the poison boogie */
  GET_OBJ_VAL(to_obj, 3) =
    (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

  /* And the weight boogie */
  weight_change_object(from_obj, -amount);
  weight_change_object(to_obj, amount);	/* Add weight */
}

void wear_message(struct char_data * ch, struct obj_data * obj, int where)
{
  const char *wear_messages[][2] =
  {
    {"$n lights $p and holds it.",
    "You light $p and hold it."},

    {"$n slides $p on to $s finger.",
    "You slide $p on to your finger."},

    {"$n slides $p on to $s finger.",
    "You slide $p on to your finger."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p on $s body.",
    "You wear $p on your body."},

    {"$n wears $p on $s head.",
    "You wear $p on your head."},

    {"$n puts $p on $s legs.",
    "You put $p on your legs."},

    {"$n wears $p on $s feet.",
    "You wear $p on your feet."},

    {"$n puts $p on $s hands.",
    "You put $p on your hands."},

    {"$n wears $p on $s arms.",
    "You wear $p on your arms."},

    {"$n straps $p around $s arm as a shield.",
    "You start to use $p as a shield."},

    {"$n wears $p about $s body.",
    "You wear $p around your body."},

    {"$n wears $p around $s waist.",
    "You wear $p around your waist."},

    {"$n puts $p on around $s right wrist.",
    "You put $p on around your right wrist."},

    {"$n puts $p on around $s left wrist.",
    "You put $p on around your left wrist."},

    {"$n wields $p.",
    "You wield $p."},

    {"$n grabs $p.",
    "You grab $p."},

    {"$n slides $p on to $s finger.",
    "You slide $p on to your finger."},

    {"$n slides $p on to $s finger.",
    "You slide $p on to your finger."},

    {"$n slides $p on to $s finger.",
    "You slide $p on to your finger."},

    {"$n places $p over $s eyes.",
    "You place $p over your eyes."},

    {"$n inserts $p into $s ear.",
    "You insert $p into your ear."},

    {"$n inserts $p into $s ear.",
    "You insert $p into your ear."},

    {"$n wears $p around $s ankle.",
    "You wear $p around your ankle."},

    {"$n wears $p around $s ankle.",
    "You wear $p around your ankle."}
  };
  act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
  act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

// Returns 0 if can use,
//         1 if cant use due to level or class
int invalid_level(struct char_data *ch, struct obj_data *object, bool display)
{
  int lr=0, questfound = 0;
  char buf[80];
  int invalid_class(struct char_data *ch, struct obj_data *obj);
 
  if (!LR_FAIL(ch, LVL_IMPL))
    return 0;

  // Quest eq checks
  if (OBJ_FLAGGED(object, ITEM_QEQ))
  {
    for (int i = 0; i < MAX_QUEST_ITEMS; i++)
      if (GET_QUEST_ITEM(ch,i) == GET_OBJ_VNUM(object))
      {
        questfound = GET_QUEST_ITEM_NUMB(ch,i);
	break;
      }
    if (questfound < 1)
    {
      if (display) 
	send_to_char("You only wish you could use this item!\r\n", ch);
      return 1;
    }
    for (int i = 0; i < NUM_WEARS; i++)
    {
      if (!GET_EQ(ch, i))
	continue;
      if (GET_OBJ_VNUM(GET_EQ(ch, i)) == GET_OBJ_VNUM(object))
	questfound--;
    }
    if (questfound < 1)
    {
      if (display)
	send_to_char("You cannot use any more of this item!\r\n", ch);
      return 1;
    }
  }

  lr = GET_OBJ_LR(object);
  // Artus> Only deduct lr if they're available to lower remorts..
  if (lr <= GET_MAX_LVL(ch))
  {
    if (GET_CLASS(ch) == CLASS_MASTER)
    {
      if (!OBJ_FLAGGED(object, ITEM_ANTI_WARRIOR) ||
	 !OBJ_FLAGGED(object, ITEM_ANTI_THIEF) ||
	 !OBJ_FLAGGED(object, ITEM_ANTI_CLERIC) ||
	 !OBJ_FLAGGED(object, ITEM_ANTI_MAGIC_USER))
	lr -= 20;
      else if (!OBJ_FLAGGED(object, ITEM_ANTI_NIGHTBLADE) ||
	       !OBJ_FLAGGED(object, ITEM_ANTI_BATTLEMAGE) ||
	       !OBJ_FLAGGED(object, ITEM_ANTI_SPELLSWORD) ||
	       !OBJ_FLAGGED(object, ITEM_ANTI_PALADIN) ||
	       !OBJ_FLAGGED(object, ITEM_ANTI_PRIEST) ||
	       !OBJ_FLAGGED(object, ITEM_ANTI_DRUID))
	lr -= 10;
    } else if ((GET_CLASS(ch) > CLASS_WARRIOR) && 
	(!OBJ_FLAGGED(object, ITEM_ANTI_WARRIOR) ||
	 !OBJ_FLAGGED(object, ITEM_ANTI_THIEF) ||
	 !OBJ_FLAGGED(object, ITEM_ANTI_CLERIC) ||
	 !OBJ_FLAGGED(object, ITEM_ANTI_MAGIC_USER)))
      lr -= 10;
  }
  
  if (LR_FAIL(ch, lr))
  {
    if (display) {
      if (lr < GET_OBJ_LR(object))
	sprintf(buf, "You are not knowledgable enough to use this item.\r\n");
      else
        sprintf(buf,"You can't figure out how to use this item?!?\r\n");
      act(buf, FALSE,ch,object, 0, TO_CHAR);
    }
    return 1;
  }

  if (invalid_class(ch, object))
  {
    if (display)
    {
      sprintf(buf, "%ss like yourself cannot use this item.\r\n", pc_class_types[(int)GET_CLASS(ch)]);
      send_to_char(buf, ch);
    }
    return 1;
  }
  return 0;
}     

// Artus> Apply items spells to a char.
void obj_wear_spells(struct char_data *ch, struct obj_data *obj)
{
  struct affected_type *af, *next;
  bool spellsused[3];
  int i = 0;
  
  if (!(ch) || !(obj))
    return;

  if (GET_OBJ_TYPE(obj) != ITEM_MAGIC_EQ)
    return;

  /* lower the charge */
  if (GET_OBJ_VAL(obj, 3) > 0)
    GET_OBJ_VAL(obj, 3)--;

/*  Artus> This is gay.
    // DM - Let them use the spell, iff they have the stats for it, and know it 
    for (int i = 0; i < 3; i ++)
    {
      if (GET_OBJ_VAL(obj, i) > 0)
      {
 *        if (has_stats_for_skill(ch, GET_OBJ_VAL(obj, i), FALSE) 
 *                        && GET_SKILL(ch, GET_OBJ_VAL(obj, i)) > 0)
 *        {  * /
          call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i), 
              / *DM - hmm why 2? 2 * / GET_LEVEL(ch), CAST_MAGIC_OBJ);
          spellsused[i] = TRUE;
//        }
      }
    } */
  for (i = 0; i < 3; i++)
  {
    spellsused[i] = FALSE;
    if (GET_OBJ_VAL(obj, i) > 0)
    {
      call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i), GET_LEVEL(ch),
		 CAST_MAGIC_OBJ);
      spellsused[i] = TRUE;
    }
  }

  for (af = ch->affected; af; af = next)
  {
    next = af->next;
    if (((spellsused[0] && af->type == GET_OBJ_VAL(obj, 0)) ||
	 (spellsused[1] && af->type == GET_OBJ_VAL(obj, 1)) ||
	 (spellsused[2] && af->type == GET_OBJ_VAL(obj, 2))) &&
	(af->duration != CLASS_ABILITY))
      af->duration = CLASS_ITEM;
  }

    /* if its out of power get rid of it */
  if (GET_OBJ_VAL(obj, 3) == 0)
  {
    act("$p disappears as its power deminishes.", FALSE, ch, obj, 0, TO_CHAR);
    extract_obj(obj);
  }
}

void perform_wear(struct char_data * ch, struct obj_data * obj, int where)
{
  bool dual_wield = FALSE;
  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */
  const int wear_bitvectors[] = 
  {
    ITEM_WEAR_TAKE, 
    ITEM_WEAR_FINGER, 
    ITEM_WEAR_FINGER, 
    ITEM_WEAR_NECK, 
    ITEM_WEAR_NECK, 
    ITEM_WEAR_BODY, 
    ITEM_WEAR_HEAD, 
    ITEM_WEAR_LEGS, 
    ITEM_WEAR_FEET, 
    ITEM_WEAR_HANDS, 
    ITEM_WEAR_ARMS, 
    ITEM_WEAR_SHIELD, 
    ITEM_WEAR_ABOUT, 
    ITEM_WEAR_WAIST, 
    ITEM_WEAR_WRIST, 
    ITEM_WEAR_WRIST, 
    ITEM_WEAR_WIELD, 
    ITEM_WEAR_TAKE,
    ITEM_WEAR_FINGER, 
    ITEM_WEAR_FINGER, 
    ITEM_WEAR_FINGER, 
    ITEM_WEAR_EYE, 
    ITEM_WEAR_EAR, 
    ITEM_WEAR_EAR, 
    ITEM_WEAR_ANKLE, 
    ITEM_WEAR_ANKLE
  };

  const char *pos_unavailable[] =
  {
    "Examining yourself, you realise you can't use a light.\r\n",
    "Examining yourself, you realise you can't wear anything on your fingers.\r\n",
    "You can't wear anything else on your fingers.\r\n",   
    "Examining yourself, you realise you can't wear anything around your neck.\r\n",
    "You can't wear anything else around your neck.\r\n",
    "Examining yourself, you realise you can't wear anything on your body.\r\n",
    "Examining yourself, you realise you can't wear anything on your head.\r\n",
    "Examining yourself, you realise you can't wear anything on your legs.\r\n",
    "Examining yourself, you realise you can't wear anything on your feet.\r\n",
    "Examining yourself, you realise you can't wear anything on your hands.\r\n",
    "Examining yourself, you realise you can't wear anything on your arms.\r\n",
    "Examining yourself, you realise you can't use a shield.\r\n",
    "Examining yourself, you realise you can't wear anything about your body.\r\n",
    "Examining yourself, you realise you can't wear anything around your waist.\r\n",
    "Examining yourself, you realise you can't wear anything on your wrists.\r\n",
    "You can't wear anything else on your wrists.\r\n",
    "Examining yourself, you realise you can't wield a weapon.\r\n",
    "Examining yourself, you realise you can't hold anything.\r\n",
    "You can't wear anything else on your fingers.\r\n",  
    "You can't wear anything else on your fingers.\r\n", 
    "You can't wear anything else on your fingers.\r\n",
    "Examining yourself, you realise you can't wear anything on your eyes.\r\n",
    "Examining yourself, you realise you can't wear anything on your ears.\r\n",
    "You can't wear anything else on your ears.\r\n",      // NOT USED
    "Examining yourself, you realise you can't wear anything around your ankles.\r\n",
    "You can't wear anything else around your ankles.\r\n" // NOT USED
  };


  const char *already_wearing[] =
  {
    "You're already using a light.\r\n",
    "You can't wear anything else on your fingers.\r\n",   // NOT USED
    "You can't wear anything else on your fingers.\r\n",   // NOT USED
    "You can't wear anything else around your neck.\r\n",
    "You can't wear anything else around your neck.\r\n",
    "You're already wearing something on your body.\r\n",
    "You're already wearing something on your head.\r\n",
    "You're already wearing something on your legs.\r\n",
    "You're already wearing something on your feet.\r\n",
    "You're already wearing something on your hands.\r\n",
    "You're already wearing something on your arms.\r\n",
    "You're already using a shield.\r\n",
    "You're already wearing something about your body.\r\n",
    "You already have something around your waist.\r\n",
    "You can't wear anything on your wrists.\r\n",
    "You can't wear anything else on your wrists.\r\n",
    "You're already wielding a weapon.\r\n",
    "You're already holding something.\r\n",
    "You can't wear anything else on your fingers.\r\n",
    "You can't wear anything else on your fingers.\r\n", 
    "You can't wear anything else on your fingers.\r\n",  
    "You're already wearing something on your eyes.\r\n",
    "You can't wear anything on your ears.\r\n",         
    "You're already wearing something on both your ears.\r\n", 
    "You can't wear anything around your ankles.\r\n",    
    "You can't wear anything else around your ankles.\r\n" 
  };

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where]))
  {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  // Find first available finger
  if (where == WEAR_FINGER_1) 
  {
    if (GET_EQ(ch, where))
      where = WEAR_FINGER_2;
    if (GET_EQ(ch, where)) 
      where = WEAR_FINGER_3;
    if (GET_EQ(ch, where)) 
      where = WEAR_FINGER_4;
    if (GET_EQ(ch, where)) 
      where = WEAR_FINGER_5;
  }

  /* for neck, ear, ankle and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_EAR_1) || (where == WEAR_NECK_1) || 
                  (where == WEAR_WRIST_R) || (where == WEAR_ANKLE_1))
    if (GET_EQ(ch, where))
      where++;

  // Ambidexterity - can wield 2 weapons, but not also a shield ...
  if (where == WEAR_WIELD && (GET_EQ(ch, WEAR_WIELD)) && IS_DUAL_CAPABLE(ch))
  {
    // wielding in 2nd wield position - shield check
    if (GET_EQ(ch, WEAR_SHIELD)) 
    {
      send_to_char("Your shield is in the way.\r\n", ch);
      return;
    }
    if (GET_EQ(ch, WEAR_HOLD))
    {
      if (CAN_WEAR(GET_EQ(ch, WEAR_HOLD), ITEM_WEAR_WIELD))
      {
	send_to_char("You're already wielding two weapons!\r\n", ch);
	return;
      }
      sprintf(buf, "You'll need to stop holding &5%s&n first.\r\n", 
	      GET_EQ(ch, WEAR_HOLD)->name);
      send_to_char(buf, ch);
      return;
    }
    where = WEAR_HOLD;
    dual_wield = TRUE;
  }

  if (where == WEAR_SHIELD && IS_DUAL_WIELDING(ch)) 
  {
    send_to_char("Your hands are tied up with weapons.\r\n", ch);
    return;
  }
  
  if (where == WEAR_HOLD && IS_DUAL_WIELDING(ch))
  {
    send_to_char("You try to grow a third arm, but fail. Try removing a weapon, first.\r\n", ch);
    return;
  }

  // Now check if the position is race restricted, or class restricted (fingers)
  if (!CAN_WEAR_POS(ch, obj, wear_bitvectors[where], where))
  {
    switch (GET_OBJ_TYPE(obj))
    {
      case ITEM_ENVIRON:
      case ITEM_GRAV1:
      case ITEM_SUBZERO:
      case ITEM_GRAV3:
      case ITEM_VACSUIT:
      case ITEM_RESPIRATE:
      case ITEM_HEATRES:
      case ITEM_COLD:
      case ITEM_HEATPROOF:
      case ITEM_STASIS:
      case ITEM_BREATHER:
	break;
      default:
	if (!OBJ_FLAGGED(obj, ITEM_QEQ))
	{
	  send_to_char(pos_unavailable[where], ch);
	  return;
	}
    }
  }

  if (GET_EQ(ch, where)) 
  {
    if (dual_wield)
      send_to_char(already_wearing[WEAR_WIELD], ch);
    else
      send_to_char(already_wearing[where], ch);
    return;
  }

  if (invalid_level(ch, obj, TRUE))
    return;
  
  // DM - quest item check
  if (!quest_obj_ok(ch, obj))
  {
    act("You can't use $p.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
      
  if (!wear_otrigger(obj, ch, where)) // where not used in wear_trigger ..
    return;

  if (dual_wield)
    wear_message(ch, obj, WEAR_WIELD);
  else
    wear_message(ch, obj, where);

  obj_from_char(obj);

  if (!equip_char(ch, obj, where, TRUE))
    return;
 
/* 
 * modification to add spells to objects. 25/12/95 Vader
 * object type ITEM_MAGIC_EQ flagged ITEM_MAGIC will contain an affection
 * in VAL[0]
 * modified 17/3/96: VAL[1] and VAL[2] now have spells aswell
 * VAL[3] is the charge of the item -1 is unlimited
 *
 * 7/2/2001: Neatened up, and checked if player has stats for skill.. DM
 * 9/4/2001: checked if player knows spell, fixed typo checking obj values
 */
  if (eq_pos_ok(ch, where) || OBJ_FLAGGED(obj, ITEM_QEQ))
    obj_wear_spells(ch, obj);
}

int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg)
{
  int where = -1;

  /* \r to prevent explicit wearing. Don't use \n, it's end-of-array marker. */
  const char *keywords[] =
  {
    "\r!RESERVED!", // light
    "finger",
    "\r!RESERVED!", // finger 2
    "neck",
    "\r!RESERVED!", // neck 2
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "shield",
    "about",
    "waist",
    "wrist",
    "\r!RESERVED!", // wrist 2
    "\r!RESERVED!", // wield
    "\r!RESERVED!", // hold
    "\r!RESERVED!", // finger 3
    "\r!RESERVED!", // finger 4
    "\r!RESERVED!", // finger 5
    "eyes", 
    "ear",
    "\r!RESERVED!", // ear 2 
    "ankle",
    "\r!RESERVED!", // ankle 2
    "\n"
  };

  if ((obj) && (!arg || !*arg))
  {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_1;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
    if (CAN_WEAR(obj, ITEM_WEAR_EAR))         where = WEAR_EAR_1;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
    if (CAN_WEAR(obj, ITEM_WEAR_ANKLE))       where = WEAR_ANKLE_1;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_SHIELD;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
    if (CAN_WEAR(obj, ITEM_WEAR_EYE))         where = WEAR_EYES;
  } else if (arg) {
    //if ((where = search_block(arg, keywords, FALSE)) < 0) {
    if (((where = search_block(arg, keywords, FALSE)) < 0) || (*arg=='!')) {
      sprintf(buf, "'%s'?  What part of your body is THAT?\r\n", arg);
      send_to_char(buf, ch);
      return -1;
    }
  }
  return (where);
}

ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1)
  {
    send_to_char("Wear what?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV))
  {
    send_to_char("You can't specify the same body location for more than one item!\r\n", ch);
    return;
  }
  if (dotmode == FIND_ALL)
  {
    for (obj = ch->carrying; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0)
      {
	items_worn++;
	perform_wear(ch, obj, where);
      }
    }
    if (!items_worn)
      send_to_char("You don't seem to have anything wearable.\r\n", ch);
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) 
    {
      send_to_char("Wear all of what?\r\n", ch);
      return;
    }
    if (!(obj = find_obj_list(ch, arg1, ch->carrying))) 
    {
      sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
      send_to_char(buf, ch);
    } else {
      while (obj) 
      {
	next_obj = find_obj_list(ch, arg1, obj->next_content);
	if ((where = find_eq_pos(ch, obj, 0)) >= 0)
	  perform_wear(ch, obj, where);
	else
	  act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
	obj = next_obj;
      }
    }
  } else {
    if (!(obj = find_obj_list(ch, arg1, ch->carrying)))
    {
      sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
      send_to_char(buf, ch);
    } else { // Artus> Removed level check. This is handled by peform_wear.
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
	perform_wear(ch, obj, where);
      else if (!*arg2)
	act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}



ACMD(do_wield)
{
  struct obj_data *obj;

  one_argument(argument, arg);
  if (!*arg)
    send_to_char("Wield what?\r\n", ch);
  else if (!(obj = find_obj_list(ch, arg, ch->carrying)))
  {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else {
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
      send_to_char("You can't wield that.\r\n", ch);
    // DM - wand/staff restriction ...
    else if (!(IS_CLERIC(ch) || IS_MAGIC_USER(ch)) && 
	GET_OBJ_TYPE(obj) == ITEM_STAFF || GET_OBJ_TYPE(obj) == ITEM_WAND)
      send_to_char("You do not know how to use this kind of item.\r\n", ch);
    else if (GET_OBJ_WEIGHT(obj)>str_app[STRENGTH_AFF_APPLY_INDEX(ch)].wield_w)
      send_to_char("It's too heavy for you to use.\r\n", ch);
    // DM, 11/99 - changed to obj level (was based on amount of dam it did)
    else if (!invalid_level(ch, obj, TRUE))
      perform_wear(ch, obj, WEAR_WIELD);  
  }
}

ACMD(do_grab)
{
  struct obj_data *obj;

  one_argument(argument, arg);
  if (!*arg)
    send_to_char("Hold what?\r\n", ch);
  else if (!(obj = find_obj_list(ch, arg, ch->carrying)))
  {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      perform_wear(ch, obj, WEAR_LIGHT);
    else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && (GET_OBJ_TYPE(obj) != ITEM_WAND) &&
                    (GET_OBJ_TYPE(obj) != ITEM_STAFF) &&
                    (GET_OBJ_TYPE(obj) != ITEM_SCROLL) &&
                    (GET_OBJ_TYPE(obj) != ITEM_POTION))
	send_to_char("You can't hold that.\r\n", ch);
      else
	perform_wear(ch, obj, WEAR_HOLD);
    }
  }
}

void perform_remove(struct char_data * ch, int pos)
{
  struct obj_data *obj;
  struct affected_type *af, *next;
  extern const char *spell_wear_off_msg[];
  bool was_dual = FALSE, apply = false;
 
  if (!(obj = ch->equipment[pos])) 
  {
    basic_mud_log("Error in perform_remove: bad pos passed.");
    return;
  } 
  if (IS_OBJ_STAT(obj, ITEM_NODROP) && LR_FAIL(ch, LVL_ANGEL)) 
  {
    act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  } 
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) 
  {
    act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  
  if (!remove_otrigger(obj, ch))
    return;

  // Artus> Handle Dual Wield. (Better)
  if (pos == WEAR_WIELD && IS_DUAL_WIELDING(ch))
    was_dual = TRUE;

  if (eq_pos_ok(ch, pos) || OBJ_FLAGGED(obj, ITEM_QEQ))
    apply = true;

  obj_to_char(unequip_char(ch, pos, TRUE), ch, __FILE__, __LINE__);
  act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
  if ((obj->obj_flags.type_flag >= BASE_PROTECT_GEAR) &&
      (obj->obj_flags.type_flag < (BASE_PROTECT_GEAR + MAX_PROTECT_GEAR)))
    check_environment_effect(ch);

  if (was_dual == TRUE)
  {
    GET_EQ(ch, WEAR_HOLD)->worn_on = WEAR_WIELD;
    GET_EQ(ch, WEAR_WIELD) = GET_EQ(ch, WEAR_HOLD);
    GET_EQ(ch, WEAR_HOLD) = NULL;
  }
 
  /* do a check for lightning and fire shields - DM */
  if (pos == WEAR_SHIELD) 
  {
    if (affected_by_spell(ch, SPELL_LIGHT_SHIELD)) 
    {
      for (af = ch->affected; af; af = next) 
      {
	next = af->next;
	if ((af->type == SPELL_LIGHT_SHIELD) && (af->duration > 0)) 
	{
	  send_to_char("The lightning upon your shield fades as you stop using it.\r\n", ch);
	  affect_remove(ch,af);
	}
      }
    } // SPELL_LIGHT_SHIELD
    if (affected_by_spell(ch, SPELL_FIRE_SHIELD)) 
    {
      for (af = ch->affected; af; af = next) 
      {
	next = af->next;
	if ((af->type == SPELL_FIRE_SHIELD) && (af->duration > 0)) 
	{
	  send_to_char("The flaming fire of your shield disappears as you stop using it.\r\n", ch);
	  affect_remove(ch,af);
	}
      }
    } // SPELL_FIRE_SHIELD
  } // WEAR_SHIELD

  /* modifications to remove magic eq AFF flags when eq is remove - Vader */
  if (GET_OBJ_TYPE(obj) == ITEM_MAGIC_EQ && apply)
  {
    for(af = ch->affected; af; af = next) 
    {
      next = af->next;
      if (((af->type == GET_OBJ_VAL(obj,0)) ||
	   (af->type == GET_OBJ_VAL(obj,1)) ||
	   (af->type == GET_OBJ_VAL(obj,2))) &&
	  (af->duration == CLASS_ITEM))
      {
	if(!af->next || (af->next->type != af->type) ||
	   (af->next->duration > 0))
	  if(*spell_wear_off_msg[af->type]) 
	  {
	    send_to_char("&r", ch);
	    send_to_char(spell_wear_off_msg[af->type], ch);
	    send_to_char("&n\r\n",ch);
	  }
	affect_remove(ch,af);
      } // Char affect matched obj affect.
    } // Character Affect Loop
  } // ITEM_MAGIC_EQ
}  

ACMD(do_remove)
{
  int i, dotmode, found;

  one_argument(argument, arg);
  if (!*arg)
  {
    send_to_char("Remove what?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(arg);
  if (dotmode == FIND_ALL)
  {
    found = 0;
    for (i = NUM_WEARS-1; i >= 0; i--) // Artus> Loop down, Held before Wielded.
      if (GET_EQ(ch, i)) 
      {
	perform_remove(ch, i);
	found = 1;
      }
    if (!found)
      send_to_char("You're not using anything.\r\n", ch);
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg)
      send_to_char("Remove all of what?\r\n", ch);
    else
    {
      found = 0;
      for (i = 0; i < NUM_WEARS; i++)
	if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
	    isname(arg, GET_EQ(ch, i)->name))
	{
	  perform_remove(ch, i);
	  // Artus> Run through again if dual wielding.
	  if (IS_DUAL_WIELDING(ch) && (i == WEAR_WIELD)) i--;
	  found = 1;
	}
      if (!found)
      {
	sprintf(buf, "You don't seem to be using any %ss.\r\n", arg);
	send_to_char(buf, ch);
      }
    }
  } else {
    /* Returns object pointer but we don't need it, just true/false. */
    if ((i = find_obj_eqpos(ch, arg)) < 0)
    {
      sprintf(buf, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
      send_to_char(buf, ch);
    } else
      perform_remove(ch, i);
  }
}
