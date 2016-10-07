/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
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
#include "house.h"
#include "constants.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;

/* JA new sector types, ther are 4 types of sector attributes
   each taking up 3 bits in the sector type integer, the lower
   4 bits remain as the base for the movement loss */
#define SECT_THIN             1
#define SECT_UNBREATHABLE     2
#define SECT_VACUUM           3
#define SECT_CORROSIVE        4
#define SECT_HOT              1
#define SECT_SCORCH           2
#define SECT_INCINERATE       3
#define SECT_COLD             4
#define SECT_FREEZING         5
#define SECT_ABSZERO          6
#define SECT_DOUBLEGRAV       1
#define SECT_TRIPLEGRAV       2
#define SECT_CRUSH            3
#define SECT_RAD1             1
#define SECT_DISPELL          2
 
/* macros to decode the bit map in the sect type */
#define BASE_SECT(n) ((n) & 0x000f)
 
#define ATMOSPHERE(n)  (((n) & 0x0070) >> 4)
#define TEMPERATURE(n) (((n) & 0x0380) >> 7)
#define GRAVITY(n)     (((n) & 0x1c00) >> 10)
#define ENVIRON(n)     (((n) & 0xe000) >> 13)
 
int check_environment_effect(struct char_data *ch);

/* external functs */
void add_follower(struct char_data *ch, struct char_data *leader);
int special(struct char_data *ch, int cmd, char *arg);
void death_cry(struct char_data *ch);
int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
void trigger_trap(struct char_data *ch, struct obj_data *obj);
int Escape(struct char_data *ch);

/* local functions */
int has_boat(struct char_data *ch);
int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
int has_key(struct char_data *ch, obj_vnum key);
void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd);
ACMD(do_escape);
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);


void EscapeAll(struct char_data *escaper, struct char_data *master, int room) {

	struct follow_type *tmp;
	int escapecount = 0;

	for(tmp = master->followers; tmp; tmp = tmp->next) {
		if (tmp->follower == escaper) // leave self for last
			continue;
		if (tmp->follower->in_room != escaper->in_room)
			continue;

	        act("$n manages to find $s way out, and your group escapes.\r\n", FALSE,
			escaper, 0, tmp->follower, TO_VICT);
		char_from_room(tmp->follower);
		char_to_room(tmp->follower, room);
		act("$n has arrives, looking hurried.", FALSE, tmp->follower, 0, 0, TO_ROOM);
		look_at_room(tmp->follower, 0);
		escapecount++;
	}		

	// Now move the master
	if (master != escaper) {
	   act("$n manages to find $s way out, and your group escapes.\r\n", FALSE,
		escaper, 0, master, TO_VICT);
           char_from_room(master);
           char_to_room(master, room);
           act("$n has arrives, looking hurried.", FALSE, master, 0, 0, TO_ROOM);
           look_at_room(master, 0);
	   escapecount++;
	}
	
	// At this stage, the escaper is the only one left, whether master or not
	act("$n leads $s group to safety.", FALSE, escaper, 0, 0, TO_ROOM);
	if (escapecount)		
	  act("You manage to find your way out, and your group escapes with you.\r\n", 
		FALSE, escaper, 0, 0, TO_CHAR);
	else
	  act("You manage to find your way out, but none of your group were nearby to follow.\r\n", 
		FALSE, escaper, 0, 0, TO_CHAR);
	char_from_room(escaper);
	char_to_room(escaper, room);
 	act("$n has arrives, looking hurried.", FALSE, escaper, 0, 0, TO_ROOM);
	look_at_room(escaper, 0);
}

ACMD(do_escape) {

     int room;

     if (IS_NPC(ch) ) {
	send_to_char("Nup, you're stuck as an NPC forever!\r\n", ch);
	return;
     }

     if (!IS_SET(GET_SPECIALS(ch), SPECIAL_ESCAPE)) {
	send_to_char("You try to escape the drudgery of your life.\r\n", ch);
	act("$n goes into denial.", FALSE, ch, 0, 0, TO_ROOM);
	return;
     }

     if (SECT(ch->in_room)  != SECT_INSIDE  && !ROOM_FLAGGED(ch->in_room, ROOM_INDOORS)) {
	send_to_char("You're already outside!\r\n", ch);
	return;
     }

     if ( (room = Escape(ch)) != NOWHERE ) {
	// Check if they're groupped
	if (AFF_FLAGGED(ch, AFF_GROUP)) {
		if (ch->master) 
		  EscapeAll(ch, ch->master, room);
		else
		  EscapeAll(ch, ch, room);
        }
	else {
          act("You manage to find your way out, and escape your surroundings.\r\n",FALSE,
		ch, 0, 0, TO_CHAR);
	  char_from_room(ch);
	  char_to_room(ch, room);
	  act("$n arrives looking hurried.\r\n", FALSE, ch, 0, 0, TO_ROOM);
	  look_at_room(ch, 0);
	}
     }
     else 
	send_to_char("You try to remember your way back out, and escape, but are unable to.\r\n", ch);

}

/* returns 1 if ch is allowed in zone */
int allowed_zone(struct char_data * ch,int flag)
{
  if (GET_LEVEL(ch)==LVL_IMPL)
    return 1;
 
  /* check if the room we're going to is level restricted */
  if ( (IS_SET(flag, ZN_NEWBIE) && GET_LEVEL(ch) > LVL_NEWBIE) )
  {
    send_to_char("You are too high a level to enter this zone!",ch);
    return 0;
  }
  if ( (IS_SET(flag, ZN_LR_5) && GET_LEVEL(ch) < 5)
     ||(IS_SET(flag, ZN_LR_10) && GET_LEVEL(ch) < 10)
     ||(IS_SET(flag, ZN_LR_15) && GET_LEVEL(ch) < 15)
     ||(IS_SET(flag, ZN_LR_20) && GET_LEVEL(ch) < 20)
     ||(IS_SET(flag, ZN_LR_25) && GET_LEVEL(ch) < 25)
     ||(IS_SET(flag, ZN_LR_30) && GET_LEVEL(ch) < 30)
     ||(IS_SET(flag, ZN_LR_35) && GET_LEVEL(ch) < 35)
     ||(IS_SET(flag, ZN_LR_40) && GET_LEVEL(ch) < 40)
     ||(IS_SET(flag, ZN_LR_45) && GET_LEVEL(ch) < 45)
     ||(IS_SET(flag, ZN_LR_50) && GET_LEVEL(ch) < 50)
     ||(IS_SET(flag, ZN_LR_55) && GET_LEVEL(ch) < 55)
     ||(IS_SET(flag, ZN_LR_60) && GET_LEVEL(ch) < 60)
     ||(IS_SET(flag, ZN_LR_65) && GET_LEVEL(ch) < 65)
     ||(IS_SET(flag, ZN_LR_70) && GET_LEVEL(ch) < 70)
     ||(IS_SET(flag, ZN_LR_75) && GET_LEVEL(ch) < 75)
     ||(IS_SET(flag, ZN_LR_80) && GET_LEVEL(ch) < 80)
     ||(IS_SET(flag, ZN_LR_85) && GET_LEVEL(ch) < 85)
     ||(IS_SET(flag, ZN_LR_90) && GET_LEVEL(ch) < 90)
     ||(IS_SET(flag, ZN_LR_95) && GET_LEVEL(ch) < 95)
     ||(IS_SET(flag, ZN_LR_ET) && GET_LEVEL(ch) < LVL_ETRNL1)
     ||(IS_SET(flag, ZN_LR_IMM) && GET_LEVEL(ch) < LVL_ANGEL)
     ||(IS_SET(flag, ZN_LR_IMP) && GET_LEVEL(ch) < LVL_IMPL) ) 
  {
      send_to_char("An overwhelming fear stops you from going any further.\r\n", ch);
      return 0;
  }
  return 1;
} 

/* returns 1 if ch is allowed in room */
int allowed_room(struct char_data * ch,int flag)
{
  if (GET_LEVEL(ch)==LVL_IMPL)
    return 1;
 
  if ( (IS_SET(flag, ROOM_NEWBIE) && GET_LEVEL(ch) > LVL_NEWBIE) )
  {
    send_to_char("You are too high a level to enter this zone!",ch);
    return 0;
  }

  if ( (IS_SET(flag, ROOM_LR_ET) && GET_LEVEL(ch) < LVL_ETRNL1)
      ||(IS_SET(flag, ROOM_LR_IMM) && GET_LEVEL(ch) < LVL_IMMORT)
      ||(IS_SET(flag, ROOM_LR_ANG) && GET_LEVEL(ch) < LVL_ANGEL)
      ||(IS_SET(flag, ROOM_LR_GOD) && GET_LEVEL(ch) < LVL_GOD)
      ||(IS_SET(flag, ROOM_LR_IMP) && GET_LEVEL(ch) < LVL_IMPL) )
  {
      send_to_char("An overwhelming fear stops you from going any further.\r\n",ch);
      return 0;
  }
  return 1;
} 

/* simple function to determine if char can walk on water */
int has_boat(struct char_data *ch)
{
  struct obj_data *obj;
  int i;
/*
  if (ROOM_IDENTITY(ch->in_room) == DEAD_SEA)
    return (1);
*/

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK))
    return (1);

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return (1);

  /* Check for the type of mount they have, to see whether they can walk on water */
  if (MOUNTING(ch) ) {
	i = GET_CLASS(MOUNTING(ch));
	if (i == CLASS_AQUATIC || i == CLASS_DRAGON)
		return (1);
  }

  return (0);
}


void special_item_mount_message(struct char_data *ch) {
	
	struct obj_data *mount = MOUNTING_OBJ(ch);
	char temp[MAX_STRING_LENGTH];

	if (!mount)
		return;

	/* Place specific item messages here */

	// else
	sprintf(temp, "$n, mounted on %s, has arrived.", mount->short_description);
	act(temp, FALSE, ch, mount, 0, TO_ROOM);

}
  

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  int need_movement;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, "")) /* XXX: Evaluate NULL */
    return (0);

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && ch->in_room == ch->master->in_room) {
    send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  /* A mount? */
  if (IS_NPC(ch) && MOUNTING(ch)) {
	return (0);
  }

  /* if this room or the one we're going to needs a boat, check for one */
  if ((BASE_SECT(ch->in_room) == SECT_WATER_NOSWIM) ||
      (BASE_SECT(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)) {
    if (!has_boat(ch)) {
      send_to_char("You need a boat to go there.\r\n", ch);
      return (0);
    }
  }


  /* move points needed is avg. move loss for src and destination sect type */
  if (!MOUNTING(ch) || !MOUNTING_OBJ(ch))
     need_movement = movement_loss[BASE_SECT(SECT(ch->in_room))] +
                   movement_loss[BASE_SECT(SECT(EXIT(ch, dir)->to_room))] >> 1;   
  else
     need_movement = 1;

  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char("You are too exhausted to follow.\r\n", ch);
    else
      send_to_char("You are too exhausted.\r\n", ch);

    return (0);
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(EXIT(ch, dir)->to_room))) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return (0);
    }

    if (MOUNTING(ch)) {
	send_to_char("You cannot go there while mounted!\r\n", ch);
	return (0);
    }
  }
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) > 1) {
    send_to_char("There isn't enough room there for more than one person!\r\n", ch);
    return (0);
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
	GET_LEVEL(ch) < LVL_GRGOD) {
    send_to_char("You aren't godly enough to use that room!\r\n", ch);
    return (0);
  }

  /* Now we know we're allow to go into the room. */
  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch))
    GET_MOVE(ch) -= need_movement;

  if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
    if (!MOUNTING(ch) || !MOUNTING_OBJ(ch))
      sprintf(buf2, "$n leaves %s.", dirs[dir]);
    if (MOUNTING(ch))
      sprintf(buf2, "$n, mounted on $N, leaves %s.", dirs[dir]);
    if (MOUNTING_OBJ(ch))
     sprintf(buf2, "$n, mounted on %s, leaves %s.", MOUNTING_OBJ(ch)->short_description,dirs[dir]);

    if (MOUNTING(ch))
	act(buf2, TRUE, ch, 0, MOUNTING(ch), TO_ROOM);
    else
        act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }
  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
	if (MOUNTING(ch))
	   act("$n, mounted on $N, has arrived.", TRUE, ch, 0, MOUNTING(ch), TO_ROOM);
        else if (MOUNTING_OBJ(ch))
	   special_item_mount_message(ch);
	else
	   act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
  }

  if (ch->desc != NULL)
    look_at_room(ch, 0);

  if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMMORT) {
    log_death_trap(ch);
    if (MOUNTING(ch)) {
	send_to_char("Your mount screams in agony as it dies.\r\n", ch);
	death_cry(MOUNTING(ch));
	extract_char(MOUNTING(ch));
    }
    death_cry(ch);
    extract_char(ch);
    return (0);
  }
  return (1);
}


int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k, *next;
  int this_skill=0, best_skill=0, percent=0, prob=0;
  struct char_data *best_char;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
    return (0);
  else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE)
    send_to_char("Alas, you cannot go that way...\r\n", ch);
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
    if (EXIT(ch, dir)->keyword) {
      sprintf(buf2, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf2, ch);
    } else
      send_to_char("It seems to be closed.\r\n", ch);
  } else {

// DM - check for SKILL_DETECT_DEATH

    // Single char movement
    if (!ch->followers) {
      if (!IS_NPC(ch) && (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_DEATH))) {
        if ((prob=GET_SKILL(ch, SKILL_DETECT_DEATH)) > 0) { 
          percent = number(1, 101);     /* 101% is a complete failure */
          prob = GET_SKILL(ch, SKILL_DETECT_DEATH);
 
          if (percent <= prob) {
            send_to_char("You sense that moving in that direction will lead to your death.\r\n",ch);
            return 0; 
          } 
        }
      }
      return (do_simple_move(ch, dir, need_specials_check));
    }

    // Group Movement
    was_in = ch->in_room;

    if (!do_simple_move(ch, dir, need_specials_check))
      return (0);

    // Find the char with the best SKILL_DETECT_DEATH
    for (k = ch->followers; k; k = next) {
      next = k->next;
      if (!IS_NPC(k->follower) && (k->follower->in_room == was_in)) {
        this_skill = GET_SKILL(k->follower, SKILL_DETECT_DEATH);
        if (this_skill > best_skill) {
          best_skill = this_skill;
          best_char = k->follower;
        }
      }
    }

    if (best_skill > 0) {
      percent = number(1, 101);     /* 101% is a complete failure */
      prob = GET_SKILL(ch, SKILL_DETECT_DEATH);
 
      if (percent <= prob) {
        for (k = ch->followers; k; k = next) {
          next = k->next;
          if (k->follower->in_room == was_in) {
            if (k->follower == best_char)
              send_to_char("You sense that moving in that direction will lead to your death.\r\n",ch);
            else
	      act("$N senses moving in that direction will lead to the group's death.\r\n", FALSE, best_char, 0, ch, TO_CHAR);
          }
        }
        return (0); 
      }
    } 

    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((k->follower->in_room == was_in) &&
     	  (GET_POS(k->follower) >= POS_STANDING)) {
	act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
    }
    return (1);
  }
  return (0);
}


ACMD(do_move)
{
  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  perform_move(ch, subcmd - 1, 0);
}


int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir) {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1) {	/* Partial Match */
      send_to_char("That's not a direction.\r\n", ch);
      return (-1);
    }
    if (EXIT(ch, door)) {	/* Braces added according to indent. -gg */
      if (EXIT(ch, door)->keyword) {
	if (isname(type, EXIT(ch, door)->keyword))
	  return (door);
	else {
	  sprintf(buf2, "I see no %s there.\r\n", type);
	  send_to_char(buf2, ch);
	  return (-1);
        }
      } else
	return (door);
    } else {
      sprintf(buf2, "I really don't see how you can %s anything there.\r\n", cmdname);
      send_to_char(buf2, ch);
      return (-1);
    }
  } else {			/* try to locate the keyword */
    if (!*type) {
      sprintf(buf2, "What is it you want to %s?\r\n", cmdname);
      send_to_char(buf2, ch);
      return (-1);
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (isname(type, EXIT(ch, door)->keyword))
	    return (door);

    sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    send_to_char(buf2, ch);
    return (-1);
  }
}


int has_key(struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return (1);

  if (GET_EQ(ch, WEAR_HOLD))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
      return (1);

  return (0);
}



#define NEED_OPEN	(1 << 0)
#define NEED_CLOSED	(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED	(1 << 3)

const char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};

const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};


#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  int other_room = 0;
  struct room_direction_data *back = 0;

  sprintf(buf, "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
      if (back->to_room != ch->in_room)
	back = 0;

  switch (scmd) {
  case SCMD_OPEN:
  case SCMD_CLOSE:
    OPEN_DOOR(ch->in_room, obj, door);
    if (back)
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(OK, ch);
    break;
  case SCMD_UNLOCK:
  case SCMD_LOCK:
    LOCK_DOOR(ch->in_room, obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char("*Click*\r\n", ch);
    break;
  case SCMD_PICK:
    LOCK_DOOR(ch->in_room, obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char("The lock quickly yields to your skills.\r\n", ch);
    strcpy(buf, "$n skillfully picks the lock on ");
    break;
  }

  /* Notify the room */
  sprintf(buf + strlen(buf), "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" :
	  (EXIT(ch, door)->keyword ? "$F" : "door"));
  if (!(obj) || (obj->in_room != NOWHERE))
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back) {
    sprintf(buf, "The %s is %s%s from the other side.",
	 (back->keyword ? fname(back->keyword) : "door"), cmd_door[scmd],
	    (scmd == SCMD_CLOSE) ? "d" : "ed");
    if (world[EXIT(ch, door)->to_room].people) {
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_ROOM);
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_CHAR);
    }
  }
}


int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
  int percent;

  percent = number(1, 101);

  if (scmd == SCMD_PICK) {
    if (keynum < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if (pickproof)
      send_to_char("It resists your attempts to pick it.\r\n", ch);
    else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
      send_to_char("You failed to pick the lock.\r\n", ch);
    else
      return (1);
    return (0);
  }
  return (1);
}


#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
			(EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : \
			(EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, 2)) : \
					(EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 1)) : \
					(EXIT(ch, door)->exit_info))

ACMD(do_gen_door)
{
  int door = -1, chance = 0;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces(&argument);
  if (!*argument) {
    sprintf(buf, "%s what?\r\n", cmd_door[subcmd]);
    send_to_char(CAP(buf), ch);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  // Do some trap checking
  if( obj ) 
	if( GET_OBJ_TYPE(obj) == ITEM_TRAP ) {
	   if( IS_THIEF(ch) )
		chance = 15;
	   else if( IS_MAGIC_USER(ch) )
		chance = 5;
	   else
		chance = 2;

	   if( number(1, chance) == chance ) 
		trigger_trap(ch, obj);
	   else
		send_to_char("Strange, it won't budge.\r\n", ch);

	   return;
	}

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char("But it's already closed!\r\n", ch);
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char("But it's currently open!\r\n", ch);
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char("Oh.. it wasn't locked, after all..\r\n", ch);
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char("It seems to be locked.\r\n", ch);
    else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
	     ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}



ACMD(do_enter)
{
  int door;

  one_argument(argument, buf);

  if (*buf) {			/* an argument was supplied, search for door
				 * keyword */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (!str_cmp(EXIT(ch, door)->keyword, buf)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
    send_to_char("You are already indoors.\r\n", ch);
  else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	      ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("You can't seem to find anything to enter.\r\n", ch);
  }
}


ACMD(do_leave)
{
  int door;

  if (OUTSIDE(ch))
    send_to_char("You are outside.. where do you want to go?\r\n", ch);
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("I see no obvious exits to the outside.\r\n", ch);
  }
}


ACMD(do_stand)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char("You are already standing.\r\n", ch);
    break;
  case POS_SITTING:
    send_to_char("You stand up.\r\n", ch);
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    /* Will be sitting after a successful bash and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    break;
  case POS_RESTING:
    send_to_char("You stop resting, and stand up.\r\n", ch);
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_SLEEPING:
    send_to_char("You have to wake up first!\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Do you not consider fighting as standing?\r\n", ch);
    break;
  default:
    send_to_char("You stop floating around, and put your feet on the ground.\r\n", ch);
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}


ACMD(do_sit)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char("You sit down.\r\n", ch);
    act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char("You're sitting already.\r\n", ch);
    break;
  case POS_RESTING:
    send_to_char("You stop resting, and sit up.\r\n", ch);
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    send_to_char("You have to wake up first.\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Sit down while fighting? Are you MAD?\r\n", ch);
    break;
  default:
    send_to_char("You stop floating around, and sit down.\r\n", ch);
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_rest)
{

  if (MOUNTING(ch)) {
	send_to_char("You cannot rest while mounted.\r\n", ch);
	return;
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char("You sit down and rest your tired bones.\r\n", ch);
    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    send_to_char("You rest your tired bones.\r\n", ch);
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    send_to_char("You are already resting.\r\n", ch);
    break;
  case POS_SLEEPING:
    send_to_char("You have to wake up first.\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Rest while fighting?  Are you MAD?\r\n", ch);
    break;
  default:
    send_to_char("You stop floating around, and stop to rest your tired bones.\r\n", ch);
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_sleep)
{
  if (MOUNTING(ch)) {
	send_to_char("You cannot sleep while mounted.\r\n", ch);
	return;
  }
 
  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
    send_to_char("You go to sleep.\r\n", ch);
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char("You are already sound asleep.\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Sleep while fighting?  Are you MAD?\r\n", ch);
    break;
  default:
    send_to_char("You stop floating around, and lie down to sleep.\r\n", ch);
    act("$n stops floating around, and lie down to sleep.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
}


ACMD(do_wake)
{
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char("Maybe you should wake yourself up first.\r\n", ch);
    else if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)) == NULL)
      send_to_char(NOPERSON, ch);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char("You can't wake up!\r\n", ch);
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char("You are already awake...\r\n", ch);
  else {
    send_to_char("You awaken, and sit up.\r\n", ch);
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
  }
}


ACMD(do_follow)
{
  struct char_data *leader;

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
      send_to_char(NOPERSON, ch);
      return;
    }
  } else {
    send_to_char("Whom do you wish to follow?\r\n", ch);
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
	send_to_char("You are already following yourself.\r\n", ch);
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
	send_to_char("Sorry, but following in loops is not allowed.\r\n", ch);
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}

int check_environment_effect(struct char_data *ch)
{
  int sect_type; 
  int modifier = 1; /* modifier is so half damage is done in freeze/scorch */
                    /* rooms if wearing minimal protection                 */
/* JA new sector flags for different worlds */

if (!IS_NPC(ch))  /* can't have all the mobs dying */
{
  sect_type = world[ch->in_room].sector_type & 0xfff0;
  switch(ATMOSPHERE(sect_type))
  {
    case SECT_THIN : 
      send_to_char("The air here is very thin.\n\r",ch);
      if (is_wearing(ch, ITEM_RESPIRATE))
        break;
      else
        if (is_wearing(ch, ITEM_BREATHER))
          break;
        else
          if (is_wearing(ch, ITEM_VACSUIT))
            break;
          else
            if (is_wearing(ch, ITEM_ENVIRON))
              break;
            else
            {
              send_to_char("You are gasping for breath!\n\r", ch);
              GET_HIT(ch) = MAX(GET_HIT(ch)-GET_LEVEL(ch), 0);
              if (GET_HIT(ch) <= 0)
              {
                GET_POS(ch) = POS_STUNNED;
                act("$n falls down gasping for breath an goes into a coma.\n\r", TRUE, ch, 0, 0, TO_ROOM);
              }
            }
      break;
    case SECT_UNBREATHABLE :
      send_to_char("The air here is unbreathable.\n\r", ch);
      if (is_wearing(ch, ITEM_BREATHER))
        break;
      else
        if (is_wearing(ch, ITEM_VACSUIT))
          break;
        else
          if (is_wearing(ch, ITEM_ENVIRON))
            break;
          else
          {
            send_to_char("You can't breath, you are asphyxiating!\n\r", ch);
            GET_HIT(ch) = MAX(GET_HIT(ch)-(GET_LEVEL(ch)*3), 0);
            if (GET_HIT(ch) < 0)
            {
              send_to_char("You suffocate to death!\n\r", ch);
              act("$n falls down gasping for breath and dies.", TRUE, ch, 0, 0, TO_ROOM); 
              raw_kill(ch,NULL);
              return(1);
            }
          }
      break;
    case SECT_VACUUM :
      send_to_char("You are in a vacuum.\n\r", ch);
      if (is_wearing(ch, ITEM_VACSUIT))
        break;
      else
        if (is_wearing(ch, ITEM_ENVIRON))
          break;
        else
        {
          send_to_char("You walk into a total vacuum, and your whole body explodes!\n\r", ch);
          act("$n enters a total vacuum and explodes, covering you with blood and gutsi!\n\r", TRUE, ch, 0, 0, TO_ROOM);
          raw_kill(ch,NULL);
          return(1);
        }  
      break;
    case SECT_CORROSIVE :
      send_to_char("The are here is thick with toxic chemicals.\n\r", ch);
      if (!is_wearing(ch, ITEM_ENVIRON))
      {
        send_to_char("The chemicals in the air burn your lungs.\n\r", ch);
        GET_HIT(ch) -= 135;
        if (GET_HIT(ch) < 0)
        {
          send_to_char("You fall down coughing up blood and die.\n\r", ch);
          act("$n falls down coughing up blood and gasping for breath, then dies!\n\r", TRUE, ch, 0, 0, TO_ROOM);   
          raw_kill(ch,NULL);
          return(1); 
        }
      }
      break;
    default :
  }
  switch(TEMPERATURE(sect_type))
  {
    case SECT_HOT :
      send_to_char("It is much hotter here than you are used to.\n\r", ch);
      if (is_wearing(ch, ITEM_HEATRES))
        break;
      else if (is_wearing(ch, ITEM_HEATPROOF))
             break;
         else if (!is_wearing(ch, ITEM_STASIS))   
           {
             send_to_char("The heat is really affecting you!\n\r", ch);
             GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch), 0);
           }
           if (GET_HIT(ch) <= 0)
           {
             GET_POS(ch) = POS_STUNNED;
             act("$n falls down from heat exhaustion!", TRUE, ch, 0, 0, TO_ROOM);
           }

      break;
    case SECT_SCORCH :
      send_to_char("Its almost hot enough to melt lead here.\n\r", ch);
      if (is_wearing(ch, ITEM_HEATPROOF) || is_wearing(ch, ITEM_STASIS))   
        break;

      if (is_wearing(ch, ITEM_HEATRES))
        modifier = 2;
     
      send_to_char("Your skin is smouldering and blistering!\n\r", ch);
      GET_HIT(ch) = MAX((GET_HIT(ch) - GET_LEVEL(ch)*2/modifier), 0);
      if (GET_HIT(ch) <= 0)
      {
        send_to_char("Your whole body is reduced to a blistered mess, you fall down dead!\n\r", ch);
        act("$n falls down dead in a smouldering heap!", TRUE, ch, 0, 0, TO_ROOM);
        raw_kill(ch,NULL);
      }
      break;   
      
    case SECT_INCINERATE :
      send_to_char("You estimate that it is well over 3000 degrees here.\n\r", ch);
      send_to_char("Your skin catches fire! You are burnt to a cinder\n\r", ch);
      act("$n spontaneously combusts and is reduced to a charred mess on the ground!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      raw_kill(ch,NULL);
      return(1);

      break;
    case SECT_COLD :
      send_to_char("Its much colder here than you are used to.\n\r", ch);
      if (is_wearing(ch, ITEM_COLD))
        break;
      else if (is_wearing(ch, ITEM_SUBZERO))
             break;
         else if (!is_wearing(ch, ITEM_STASIS))   
           {
             send_to_char("You shiver uncontrollably from the cold!\n\r", ch);
             GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch), 0);
             if (GET_HIT(ch) <= 0)
             {
               send_to_char("You fall down unconcious from hyperthermia!\n\r", ch);
               GET_POS(ch) = POS_STUNNED;
               act("$n falls down in a coma from hyperthermia!", TRUE, ch, 0, 0, TO_ROOM);
             }
           }
      break;
   
    case SECT_FREEZING :
      send_to_char("The air here is freezing.\n\r", ch); 
      if (is_wearing(ch, ITEM_SUBZERO) || is_wearing(ch, ITEM_STASIS))   
        break;

      if (is_wearing(ch, ITEM_COLD))
        modifier = 2;
     
      send_to_char("You are suffering from severe frostbite!\n\r", ch);
      GET_HIT(ch) = MAX(GET_HIT(ch) - (GET_LEVEL(ch)*2/modifier), 0);
      if (GET_HIT(ch) <= 0)
      {
        send_to_char("You fall down dead from frostbite!\n\r", ch);
        act("$n falls down dead from frostbite and hyperthermia!", TRUE, ch, 0, 0, TO_ROOM);
        raw_kill(ch,NULL);
      }
      break;   

    case SECT_ABSZERO :
      send_to_char("It dosn't get much colder than this... absolute zero!\n\r", ch);
      send_to_char("Your body is frozen rock hard and shatters into many pieces!\n\r", ch);
      act("$n is frozen solid, tries to move and shatters into thousands of pieces!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      raw_kill(ch,NULL);

      break;  
  }

  switch(GRAVITY(sect_type))
  {
    case SECT_DOUBLEGRAV :
      send_to_char("The gravity here is much higher than normal.\n\r", ch);
      break;
    case SECT_TRIPLEGRAV :
      send_to_char("The gravity here is is extremely strong.\n\r", ch);
      break;
    case SECT_CRUSH :
      send_to_char("You must be very near to a singularity, the gravity is approaching infinity.\n\r", ch);
      break;
  }

  switch(ENVIRON(sect_type))
  {
    case SECT_RAD1 :
      send_to_char("If you had a geiger counter it would be off the scale.\n\r", ch);
      if (is_wearing(ch, ITEM_RAD1PROOF))
        break;
	else if (!is_wearing(ch, ITEM_STASIS))
           {
             send_to_char("You feel decidedly sick in the stomach.  The radiation is really affecting you.\n\r", ch);
             GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch), 0);
           }
           if (GET_HIT(ch) <= 0)
           {
             act("$n passes out from radiation poisining!\n\r", TRUE, ch, 0, 0, TO_ROOM);
             send_to_char("You pass out from radiation poisoining!\n\r",ch);
             GET_POS(ch) = POS_STUNNED;
           }
      break;
     case SECT_DISPELL :
	{
		send_to_char("Your body shivers as a flash of bright light stripps you of all spells!!!.\n\r", ch);
		while (ch->affected)
    		affect_remove(ch, ch->affected);
	} break;
  }
  return(0);
 } /* end if (!IS_NPC(ch)) */
  return(0);
} 

ACMD(do_go)
{
  /* this will be somewhat more sophisticated and allow players
     to board different obects, but for now we only new it for
     the pirate ship, so I'm just hard coding it for that */
 
  char arg1[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int was_in;
 
  one_argument(argument, arg1);
 
  if (!*arg1)
  {
    send_to_char("Go where??\n\r", ch);
    return;
  }
 
  if (!(obj = get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents)))
  {
    sprintf(buf, "You don't see %s %s here.\r\n", AN(arg1), arg1);
    send_to_char(buf, ch);
  }
  else
  {
    if (GET_OBJ_TYPE(obj) != ITEM_GATEWAY)
    {
      send_to_char("You can't 'go' to that!!\n\r", ch);
      return;
    }
    else
    {
/* this isn't right so we won't have movemment for 'go' for now
       if (GET_LEVEL(ch) < LVL_IS_GOD && !IS_NPC(ch))
        GET_MOVE(ch) -= need_movement;
*/
      if (!IS_AFFECTED(ch, AFF_SNEAK))
      {
        sprintf(buf2, "$n goes to the %s.", arg1);
        act(buf2, TRUE, ch, 0, 0, TO_ROOM); 
      }
 
      was_in = ch->in_room;
      char_from_room(ch);
      char_to_room(ch, real_room(GET_OBJ_VAL(obj, 0)));
 
      if (!IS_AFFECTED(ch, AFF_SNEAK))
        act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
      look_at_room(ch, 0);
    }
  }
}   
/* 
 * 	Code for mounts in Primal. Support for both mob and obj mount types.
 *
 *	Author: 	Jason Theofilos (Talisman)
 *	Date Started:	3rd December, 1999
 *	Last Modified: 	10th December, 1999
 *
 *	Location:	Melbourne, Australia
 * 	Language: 	C
 *
 */

/* OTHER FILES ALTERED:
	utils.h
	structs.h
	interpreter.c
	act.informative.c
	act.movement.c
	utils.c (circle_follow)
	act.offensive.c
	fight.c
	mobact.c
	db.c (mobloading)
	constants.c (flags) & (npc classes)
	handler.c
	act.wizard.c
 */


/* Breaks in (tames) a mob for riding */
ACMD(do_breakin) {

	struct char_data *victim;

	if( IS_NPC(ch) )
		return;

	// Find the target
	one_argument(argument, arg);
	
	if( !*arg ) {
		send_to_char("Just what would you like to break in?\r\n", ch);
		return;
	}	
	// Check the target is present
	if( !(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)) ) {
		send_to_char("Noone here by that name.\r\n", ch);
		return;
	}
	// Fighting?
	if( FIGHTING(ch) || FIGHTING(victim) ) {
		send_to_char("You can't do that now!\r\n", ch);
		return;
	}
	// Taming themselves?
	if( victim == ch ) {
		send_to_char("You break yourself in, and feel quite susceptible to being ridden.\r\n",ch);
		return;
	}

	if( AFF_FLAGGED(victim, AFF_BROKEN_IN) ) {
		act("$N is already broken in.", FALSE, ch, 0, victim, TO_CHAR );
		return;
	}		
	
	// Is the mob mountable? 
	if( MOB_FLAGGED(victim, MOB_MOUNTABLE) ) {
		/* Check for skill here */
		act("You successfully break $N in.", FALSE, ch, 0, victim, TO_CHAR);
		SET_BIT(AFF_FLAGS(victim), AFF_BROKEN_IN);
	}
	else {
		act("You try to break $N in, but $M does not respond well.", FALSE, ch, 0, victim, TO_CHAR);
		if( GET_LEVEL(ch) < LVL_IMMORT )
			do_hit(victim, ch->player.name, 0, 0);		
	}
}

/* Performs the mounting manouevre on a mobile or appropriate object */
/* Is it just me or does all this sound a bit sus?  - Tal */
ACMD(do_mount) {

	int bits;
	struct char_data *found_char = NULL;
	struct obj_data *found_obj = NULL;

	if (IS_NPC(ch) ) return;

	one_argument(argument, arg);

	if(!*arg) {
		send_to_char("Mount what?\r\n", ch);
		return;
	}

	// Check character is not already mounted
	if( MOUNTING(ch) || MOUNTING_OBJ(ch) ) {
		send_to_char("You're already mounted!\r\n", ch);
		return; 
	}

	bits = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP, 
		ch, &found_char, &found_obj);

	// Character found 
	if( found_char != NULL ) {
		// Check that the char is mountable
		if( !MOB_FLAGGED(found_char, MOB_MOUNTABLE) ) {
			send_to_char("You can't mount that!\r\n", ch);
			return;
		}	
		// Check if it's tamed
		if( !AFF_FLAGGED(found_char, AFF_BROKEN_IN) ) {
			send_to_char("This beastie's a bit too wild for you.\r\n", ch);
			act("$n hopelessly chases $N around, trying to mount $M.", FALSE, ch, 0,found_char, TO_ROOM);
			return;
		}	
		// Check that noone is riding it already
		if( MOUNTING(found_char) ) {
			act("Someone is already riding $M.", FALSE, ch, 0, found_char, TO_CHAR);
			return;
		}	
		// Mount the beast.
		act("With a flourish you mount $N.", FALSE, ch, 0, found_char,TO_CHAR);
		act("$n gracefully mounts $N.", FALSE, ch, 0, found_char, TO_ROOM);
		MOUNTING(ch) = found_char;
		MOUNTING(found_char) = ch;
		return;
	}

	// Is it an item
	if( found_obj != NULL ) {
		// Check that the target item is mountable
		if( !OBJ_FLAGGED(found_obj, ITEM_RIDDEN) ) {
			send_to_char("You can't mount that!\r\n", ch);
			return;
		}
		// Check it is not already being ridden
		if( OBJ_RIDDEN(found_obj) ) {
			send_to_char("It is already being ridden.\r\n", ch);
			return;
		}	
		// Mount it
		sprintf(buf, "You mount %s.\r\n", found_obj->short_description);
		send_to_char(buf, ch);
		sprintf(buf, "%s mounts %s.\r\n", GET_NAME(ch),found_obj->short_description);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);
		MOUNTING_OBJ(ch) = found_obj;
		OBJ_RIDDEN(found_obj) = ch;		
		return;	
	}
	
	send_to_char("Nothing here by that name.\r\n", ch);
	
}

ACMD(do_dismount) {

	struct char_data *mount;
	struct obj_data *obj_mount;
        int tmp = 0;

	if( IS_NPC(ch) ) return;

	// May as well make it a social too
	if( !MOUNTING(ch) && !MOUNTING_OBJ(ch) ) {
		send_to_char("You get off your high horse.\r\n", ch);
		act("$n gets off $s high horse.", FALSE, ch, 0, 0, TO_ROOM);
		return;
	}

	// Mobile dismount
	if( MOUNTING(ch) ) {
		// Get the mount
		mount = MOUNTING(ch);
		// Get a keyword
		one_argument(mount->player.name, arg);
		// If it is not visible ...
		if ( mount != get_char_vis(ch, arg, FIND_CHAR_ROOM) ) {
			send_to_char("Oh dear, you seem to have lost your ride.\r\n", ch);
			send_to_char("... you fall off your imaginary mount.\r\n", ch);
			act("$n falls off $s imaginary mount.\r\n", FALSE, ch, 0, 0, TO_ROOM);
		}
		else {
			// Dismount successful.
			send_to_char("You dismount.\r\n", ch);
			act("$n easily hops off $N.", FALSE, ch, 0, mount, TO_ROOM);
		}
		// Regardless of whether the player can see it or not, dismount. 
		MOUNTING(mount) = NULL;
		MOUNTING(ch) = NULL;
		GET_POS(ch) = POS_STANDING;
		return;
	}

	// Item dismount
	if( MOUNTING_OBJ(ch) ) {
		// Get the object
		obj_mount = MOUNTING_OBJ(ch);
		// Check it's being carried or is in the room
		if( !((obj_mount != get_obj_in_list_vis(ch, obj_mount->name, ch->carrying)) || 
                    (obj_mount != get_obj_in_list_vis(ch, obj_mount->name,world[ch->in_room].contents)) ||
		    (obj_mount != get_object_in_equip_vis(ch, obj_mount->name, ch->equipment, &tmp))) ) {
			send_to_char("Oh dear, you seem to have lost your ride.\r\n", ch);
			send_to_char("... you fall off your imaginary mount.\r\n", ch);
			act("$n falls off $s imaginary mount.\r\n", FALSE, ch, 0, 0, TO_ROOM);
		} 
		else {
			send_to_char("You dismount.\r\n", ch);
			act("$n easily hops off $N.", FALSE, ch, obj_mount, 0, TO_ROOM);
		}
		// Dismount
		OBJ_RIDDEN(obj_mount) = NULL;
		MOUNTING_OBJ(ch) = NULL;
		GET_POS(ch) = POS_STANDING;
		return;
	}

}

/*********************************** END mount.c ********************/
