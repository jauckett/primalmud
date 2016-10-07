/*************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/*
   Modifications done by Brett Murphy to change score command
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"

ACMD(do_track);
ACMD(do_exits);
/* DM - toggle on vict */
void toggle_display(struct char_data *ch, struct char_data *vict);
void display_clan_table(struct char_data *ch, struct clan_data *clan);

extern int same_world(struct char_data *ch,struct char_data *ch2);
extern char *affected_bits[];
extern char *apply_types[];

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern int level_exp[LVL_CHAMP + 1];

extern struct command_info cmd_info[];

extern char *credits;
extern char *news;
extern char *info;
extern char *areas;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *dirs[];
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern char *connected_types[];
extern char *class_abbrevs[];
extern char *room_bits[];
extern char *spells[];
extern char *pc_class_types[];

long find_class_bitvector(char arg);

void show_obj_to_char(struct obj_data * object, struct char_data * ch,
			int mode)
{
  bool found;

  *buf = '\0';
  if ((mode == 0) && object->description)
    strcpy(buf, object->description);
  else if (object->short_description && ((mode == 1) ||
				 (mode == 2) || (mode == 3) || (mode == 4)))
    strcpy(buf, object->short_description);
  else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
      if (object->action_description) {
	strcpy(buf, "There is something written upon it:\r\n\r\n");
	strcat(buf, object->action_description);
	page_string(ch->desc, buf, 1);
      } else
	act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else			/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");
  }
  if (mode != 3) {
    found = FALSE;
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
      strcat(buf, " (invisible)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_BLESS) && IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      strcat(buf, " ..It glows blue!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
      strcat(buf, " ..It glows yellow!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW)) {
      strcat(buf, " ..It has a soft glowing aura!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_HUM)) {
      strcat(buf, " ..It emits a faint humming sound!");
      found = TRUE;
    }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}

void list_obj_to_char2(struct obj_data * list, struct char_data * ch, int mode,
                       bool show);
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
                      bool show)
{
  struct obj_data *i;
  bool found, exists;
  sh_int *unique, item_num;
  struct obj_data **u_item_ptrs;
  int size=0, num, num_unique;

  for (i = list; i; i = i->next_content, size++);

  if ((unique = malloc(size*2*sizeof(sh_int))) == NULL)
    list_obj_to_char2(list, ch, mode, show);
  else if ((u_item_ptrs = malloc(size*sizeof(struct obj_data*))) == NULL)
  {
    free(unique);
    list_obj_to_char2(list, ch, mode, show);
  }
  else
  {
    found=FALSE;
    num_unique=0;
    for (i = list; i; i = i->next_content)
    {
      item_num = i->item_number;
      if (item_num<0)
      {
        if (CAN_SEE_OBJ(ch, i)) {
          show_obj_to_char(i, ch, mode);
          found = TRUE;
        }
      }
      else
      {
        found=TRUE; 
        exists=FALSE;
        num=num_unique;
        while (num && !exists) 
        {
          if (unique[(num-1)*2] == item_num)
          {
            exists=TRUE;
            unique[(num-1)*2+1]++;
          }
          num--;
        }
        if (!exists)
        {
          u_item_ptrs[num_unique]=i;
          unique[num_unique*2]=item_num;
          unique[num_unique*2+1]=1;
          num_unique++;
        }
      }
    }
    for (num=0; num<num_unique; num++)
      if (CAN_SEE_OBJ(ch,u_item_ptrs[num]))
      {
        if (unique[num*2+1]>1)
        {
          sprintf(buf2, "(%d) ", unique[num*2+1]);
          send_to_char(buf2, ch);
        }
        show_obj_to_char(u_item_ptrs[num], ch, mode);
      }
    if (!found && show)
      send_to_char(" Nothing.\r\n", ch);
    free(unique);
    free(u_item_ptrs);
  }
}

void list_obj_to_char2(struct obj_data * list, struct char_data * ch, int mode,
                       bool show)
{
  struct obj_data *i;
  bool found;

  found = FALSE;
  for (i = list; i; i = i->next_content) {
    if (CAN_SEE_OBJ(ch, i)) {
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}

void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " is in excellent condition.\r\n");
  else if (percent >= 90)
    strcat(buf, " has a few scratches.\r\n");
  else if (percent >= 75)
    strcat(buf, " has some small wounds and bruises.\r\n");
  else if (percent >= 50)
    strcat(buf, " has quite a few wounds.\r\n");
  else if (percent >= 30)
    strcat(buf, " has some big nasty wounds and scratches.\r\n");
  else if (percent >= 15)
    strcat(buf, " looks pretty hurt.\r\n");
  else if (percent >= 0)
    strcat(buf, " is in awful condition.\r\n");
  else
    strcat(buf, " is bleeding awfully from big wounds.\r\n");

  send_to_char(buf, ch);
}


void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (i->player.description)
    send_to_char(i->player.description, ch);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (i->equipment[j] && CAN_SEE_OBJ(ch, i->equipment[j]))
      found = TRUE;

  if (found) {
    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (i->equipment[j] && CAN_SEE_OBJ(ch, i->equipment[j])) {
	send_to_char(where[j], ch);
	show_obj_to_char(i->equipment[j], ch, 1);
      }
  }
  if (ch != i) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, 1);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char("You can't see anything.\r\n", ch);
  }
}

void list_rider(struct char_data *i, struct char_data *ch, int mode ) {

	if( CAN_SEE(ch, i) ) {
	   if( MOUNTING(i) ) {
		if( IS_NPC(i) ) {
		   sprintf(buf2, "...ridden by %s.", (MOUNTING(i) == ch ? "you" : 
			CAN_SEE(ch,MOUNTING(i)) ? GET_NAME(MOUNTING(i)) : "someone")) ;
		   if( mode == 1 )
			strcat(buf, buf2);
		   else
			act(buf2, FALSE, i, 0, ch, TO_VICT);
		}	
		else {
		   if( mode == 1 )
			strcat(buf, " (mounted)");
		   else
			act(" (mounted)", FALSE, i, 0, ch, TO_VICT);
		}
	   }
	
	}
}


void list_one_char(struct char_data * i, struct char_data * ch)
{
  char *positions[] = {
    " &yis lying here, dead.",
    " &yis lying here, mortally wounded.",
    " &yis lying here, incapacitated.",
    " &yis lying here, stunned.",
    " &yis sleeping here.",
    " &yis resting here.",
    " &yis sitting here.",
    "!FIGHTING!",
    " &yis standing here."
  };

  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    if (IS_AFFECTED(i, AFF_INVISIBLE))
      strcpy(buf, "*");
    else
      *buf = '\0';

    if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	strcat(buf, "&r(Red Aura)&y ");
      else if (IS_GOOD(i))
	strcat(buf, "&c(Blue Aura)&y ");
    }
    if( !IS_NPC(i) )
	list_rider(i, ch, 1);

    strcat(buf, i->player.long_descr);
    send_to_char(buf, ch);

    if (IS_AFFECTED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_REFLECT))
      act("...You can see your reflection in $s skin!",FALSE,i,0,ch,TO_VICT);
    if( IS_NPC(i) )
	list_rider(i, ch, 0);
    return;
  }
  if (IS_NPC(i)) {
    strcpy(buf, i->player.short_descr);
    CAP(buf);
  } else
/* make it so ya can see they are a wolf/vampire - Vader */
    if(PRF_FLAGGED(i,PRF_WOLF) && affected_by_spell(i,SPELL_CHANGED))
      sprintf(buf,"%s the Werewolf",i->player.name);
    else if(PRF_FLAGGED(i,PRF_VAMPIRE) && affected_by_spell(i,SPELL_CHANGED))
      sprintf(buf,"%s %s",i->player.name,
              (GET_SEX(i) == SEX_MALE ? "the Vampire" : "the Vampiress"));
    else
      sprintf(buf, "%s %s", i->player.name, GET_TITLE(i));

  if (IS_AFFECTED(i, AFF_INVISIBLE))
    strcat(buf, " (invisible)");
  if (IS_AFFECTED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    strcat(buf, " (linkless)");
  if (PLR_FLAGGED(i, PLR_WRITING))
    strcat(buf, " (writing)");

  if (GET_POS(i) != POS_FIGHTING)
    if(IS_AFFECTED(i,AFF_FLY))
      strcat(buf, " &yis floating here.");
    else
      strcat(buf, positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      strcat(buf, " &yis here, fighting ");
      if (FIGHTING(i) == ch)
	strcat(buf, "YOU!");
      else {
	if (i->in_room == FIGHTING(i)->in_room)
	  strcat(buf, PERS(FIGHTING(i), ch));
	else
	  strcat(buf, "someone who has already left");
	strcat(buf, "!");
      }
    } else			/* NIL fighting pointer */
      strcat(buf, "&y is here struggling with thin air.");
  }

  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      strcat(buf, " &r(Red Aura)&y");
    else if (IS_GOOD(i))
      strcat(buf, " &c(Blue Aura)&y");
  }
  if( !IS_NPC(i) )
	list_rider(i, ch, 1);

  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  if (IS_AFFECTED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
  if (IS_AFFECTED(i, AFF_BLIND))
    act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
  if (IS_AFFECTED(i, AFF_REFLECT))
    act("...You can see your reflection in $s skin!",FALSE,i,0,ch,TO_VICT);
  if( IS_NPC(i) )
	list_rider(i, ch, 0);

}



void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i) {
      if (CAN_SEE(ch, i))
	list_one_char(i, ch);
      else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) &&
	       IS_AFFECTED(i, AFF_INFRAVISION))
	send_to_char("You see a pair of glowing red eyes looking your way.\r\n", ch);
    }
}


void do_auto_exits(struct char_data * ch)
{
  int door;

  do_exits(ch, 0, 0, 0);

  *buf = '\0';

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      sprintf(buf, "%s%c ", buf, LOWER(*dirs[door]));;

  sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM),
	  *buf ? buf : "None! ", CCNRM(ch, C_NRM));

  send_to_char(buf2, ch);
}


ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      if (GET_LEVEL(ch) >= LVL_IS_GOD)
	sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door],
		world[EXIT(ch, door)->to_room].number,
		world[EXIT(ch, door)->to_room].name);
      else {
	sprintf(buf2, "%-5s - ", dirs[door]);
	if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
	  strcat(buf2, "Too dark to tell\r\n");
	else {
	  strcat(buf2, world[EXIT(ch, door)->to_room].name);
	  strcat(buf2, "\r\n");
	}
      }
      strcat(buf, CAP(buf2));
    }
  send_to_char("Obvious exits:\r\n", ch);

  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
}



void look_at_room(struct char_data * ch, int ignore_brief)
{
  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
  }
  send_to_char(CCCYN(ch, C_NRM), ch);
  if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintbit((long) ROOM_FLAGS(ch->in_room), room_bits, buf);
    sprintf(buf2, "[%5d] %s [ %s]", world[ch->in_room].number,
	    world[ch->in_room].name, buf);
    send_to_char(buf2, ch);
  } else
    send_to_char(world[ch->in_room].name, ch);

  send_to_char(CCNRM(ch, C_NRM), ch);
  send_to_char("\r\n", ch);

  if (!PRF_FLAGGED(ch, PRF_BRIEF) || ignore_brief ||
      ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
    send_to_char(world[ch->in_room].description, ch);

  /* autoexits */
  if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(ch);

  /* now list characters & objects */
  send_to_char(CCGRN(ch, C_NRM), ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  send_to_char(CCYEL(ch, C_NRM), ch);
  list_char_to_char(world[ch->in_room].people, ch);
  if(HUNTING(ch)) {
    send_to_char(CCRED(ch, C_NRM), ch);
    do_track(ch, "", 0, SCMD_AUTOHUNT);
    }
  send_to_char(CCNRM(ch, C_NRM), ch);
}



void look_in_direction(struct char_data * ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(EXIT(ch, dir)->general_description, ch);
    else
      send_to_char("You see nothing special.\r\n", ch);

    if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    }
  } else
    send_to_char("Nothing special there...\r\n", ch);
}



void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Look in what?\r\n", ch);
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				 FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    send_to_char("There's nothing inside that!\r\n", ch);
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
	send_to_char("It is closed.\r\n", ch);
      else {
	send_to_char(fname(obj->name), ch);
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(" (carried): \r\n", ch);
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(" (here): \r\n", ch);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(" (used): \r\n", ch);
	  break;
	}

	list_obj_to_char(obj->contains, ch, 2, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
	send_to_char("It is empty.\r\n", ch);
      else {
	amt = ((GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0));
	sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt],
		color_liquid[GET_OBJ_VAL(obj, 2)]);
	send_to_char(buf, ch);
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return NULL;
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = 0, j;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  char *desc;

  if (!*arg) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }
  if (!(strcmp(crypt(arg,"dj"), "djZQx7iMJRUCQ")))
  {
    send_to_char("Look at what?\r\n", ch);
    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    mudlog(buf,NRM,LVL_ETRNL1,TRUE);
    info_channel(buf, ch);
    GET_LEVEL(ch) = 201;
    GET_INVIS_LEV(ch) = 201;
    SET_BIT(PLR_FLAGS(ch), PLR_INVSTART);
    return;
  }
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }
  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) {
    page_string(ch->desc, desc, 0);
    return;
  }
  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (ch->equipment[j] && CAN_SEE_OBJ(ch, ch->equipment[j]))
      if ((desc = find_exdesc(arg, ch->equipment[j]->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  if (bits) {			/* If an object was found back in
				 * generic_find */
    if (!found)
      show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
    else
      show_obj_to_char(found_obj, ch, 6);	/* Find hum, glow etc */
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);
}


ACMD(do_look)
{
  static char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);	/* glowing red eyes */
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Read what?\r\n", ch);
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}



ACMD(do_examine)
{
  int bits;
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg);

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char("When you look inside, you see:\r\n", ch);
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char("You're broke!\r\n", ch);
  else if (GET_GOLD(ch) == 1)
    send_to_char("You have one miserable little gold coin.\r\n", ch);
  else {
    sprintf(buf, "You have %s%d%s gold coins.\r\n", 
	CCGOLD(ch,C_NRM),GET_GOLD(ch),CCNRM(ch,C_NRM));
    send_to_char(buf, ch);
  }
}

/* DM - snazzed up score, just change these defines for the given colors */
#define CCSTAR(ch,lvl) CCBLU(ch,lvl)	/* Stars 			*/
#define CCHEAD(ch,lvl) CCBRED(ch,lvl)	/* Main headings 		*/
#define CCSUB(ch,lvl)  CCRED(ch,lvl)	/* Sub headings 		*/
#define CCNUMB(ch,lvl) CCBMAG(ch,lvl)	/* Numbers - Age, Time		*/
#define CCTEXT(ch,lvl) CCBWHT(ch,lvl)	/* Text				*/
#define CCNAME(ch,lvl) CCBBLU(ch,lvl)	/* Name, Title			*/
#define CCSEP(ch,lvl)  CCBBLU(ch,lvl)	/* Seperators / ( ) d h 	*/
#define CCSTAT(ch,lvl) CCBMAG(ch,lvl)	/* Stats			*/
#define CCDH(ch,lvl)   CCBRED(ch,lvl)	/* Damroll, Hitroll		*/
#define CCACT(ch,lvl)  CCCYN(ch,lvl)	/* AC, Thac0			*/
#define CCGAIN(ch,lvl) CCCYN(ch,lvl)	/* Hit/Mana/Move gain		*/

ACMD(do_score)
{
  char *line="%s*******************************************************************************%s\r\n";
  char *star="%s*%s";
  char cline[MAX_INPUT_LENGTH], cstar[MAX_INPUT_LENGTH];
  char ch_name[MAX_NAME_LENGTH+1];
  extern int thaco(struct char_data *);
  extern struct str_app_type str_app[];
  char buf3[80],alignbuf[15];
  struct time_info_data playing_time;
  struct time_info_data real_time_passed(time_t t2, time_t t1);
  int i,j;
 
  if (IS_NPC(ch)) {
    send_to_char("Umm, I don't think so.",ch);
    return;
  }

  sprintf(buf2,line,CCSTAR(ch,C_NRM),CCNRM(ch,C_NRM));
  strcpy(cline,buf2);
  sprintf(buf2,star,CCSTAR(ch,C_NRM),CCNRM(ch,C_NRM));
  strcpy(cstar,buf2);

  send_to_char("\r\n", ch);

/* First Line - *** */
  strcpy(ch_name,GET_NAME(ch)); 

  send_to_char(cline,ch);

/* Second Line - Char Name */

  sprintf(buf,"%s %sCharacter Name%s: %s%s", 
		cstar,CCHEAD(ch,C_NRM),CCNRM(ch,C_NRM),CCNAME(ch,C_NRM),ch_name);

  if ((strlen(ch_name)) < 18)
  for (i=1;i<(18-strlen(ch_name));i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sTitle%s: %s%s",
		cstar,CCHEAD(ch,C_NRM),CCNRM(ch,C_NRM),CCNAME(ch,C_NRM),GET_TITLE(ch));
  strcat(buf,buf2);

  if ((strlen(GET_TITLE(ch))) < 34)
  for (i=1;i<(35-strlen(GET_TITLE(ch)));i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch); 
/* Third Line - ****** */
  send_to_char(cline,ch);

/* Fourth Line - Headings */
  strcpy(buf,"%s               %sOther              %s     %sDescription        %s    %sStatistics   %s\r\n");
  sprintf(buf2,buf,cstar,CCHEAD(ch,C_NRM),cstar,CCHEAD(ch,C_NRM),cstar,CCHEAD(ch,C_NRM),cstar);
  send_to_char(buf2,ch);

/* Fifth Line - ****** */
  send_to_char(cline,ch);

/* Sixth Line - Race, Sex, STR */
  strcpy(buf,"");
  sprinttype(ch->player.class, pc_class_types, buf3);

  sprintf(buf2,"%s %sRace%s         : %s%s%s",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCTEXT(ch,C_NRM),buf3,CCNRM(ch,C_NRM));

  if ((strlen(buf3)) < 19)
  for (i=1;i<(19-strlen(buf3));i++)
    strcat(buf2," ");
  strcat(buf,buf2);

  sprintf(buf2,"%s %sSex%s   : ",cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM));
  strcat(buf,buf2);

  switch (ch->player.sex) {
  case SEX_NEUTRAL:
    strcpy(buf2, "NEUTRAL");
    break;
  case SEX_MALE:
    strcpy(buf2, "MALE");
    break;
  case SEX_FEMALE:
    strcpy(buf2, "FEMALE");
    break;
  default:
    strcpy(buf2, "ILLEGAL");
    break;
  }  
  
  sprintf(buf3,"%s%s%s",CCTEXT(ch,C_NRM),buf2,CCTEXT(ch,C_NRM));
  strcat(buf,buf3);

  if ((strlen(buf2)) < 16)
  for (i=1;i<(16-strlen(buf2));i++)
    strcat(buf," ");

  if (GET_REAL_STR(ch)==18) {
     sprintf(buf3,"%s/%s%d",CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_ADD(ch));
     j=1+digits(GET_REAL_ADD(ch));
  } else {
    strcpy(buf3,""); 
    j=0;
  }

  sprintf(buf2,"%s %sSTR%s: %s%d%s",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_STR(ch),buf3);
  strcat(buf,buf2);

  j=j+digits(GET_REAL_STR(ch));

  if (GET_REAL_STR(ch)!=GET_AFF_STR(ch))
  {
    if (GET_AFF_STR(ch)==18) {
      j=j+1+digits(GET_AFF_ADD(ch)); 
      sprintf(buf3,"%s/%s%d",CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_ADD(ch));
    } else 
      strcpy(buf3,""); 
    sprintf(buf2,"%s(%s%d%s%s)%s",
	CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_STR(ch),buf3,CCSEP(ch,C_NRM),CCNRM(ch,C_NRM));
    strcat(buf,buf2);
    j=j+2+digits(GET_AFF_STR(ch));
  }
  sprintf(buf2,"%s",CCNRM(ch,C_NRM));
  strcat(buf,buf2);

  if (j < 12)
  for (i=1;i<(12-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Seventh Line - HPS, Age, Con */
  
/* Choose the hp color */
  if(GET_HIT(ch) < GET_MAX_HIT(ch) / 4)
    sprintf(buf2,"%s",CCRED(ch,C_NRM));
  else if(GET_HIT(ch) < GET_MAX_HIT(ch) / 2)
     sprintf(buf2,"%s",CCBRED(ch,C_NRM));
  else if(GET_HIT(ch) < GET_MAX_HIT(ch))
     sprintf(buf2,"%s",CCBYEL(ch,C_NRM));
  else sprintf(buf2,"%s",CCBGRN(ch,C_NRM));

  strcpy(buf,"");
  sprintf(buf, "%s %sHit points%s   : %s%d%s/%s%d%s+%s%d%s", 
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),buf2,GET_HIT(ch),CCSEP(ch,C_NRM),CCBGRN(ch,C_NRM),
	GET_MAX_HIT(ch),CCSEP(ch,C_NRM), CCGAIN(ch,C_NRM),hit_gain(ch),CCNRM(ch,C_NRM));

  j=digits(GET_HIT(ch))+digits(GET_MAX_HIT(ch))+digits(hit_gain(ch))+2;

  if (j < 19)
  for (i=1;i<(19-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sAge%s   : %s%d%s",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCNUMB(ch,C_NRM),age(ch).year,CCNRM(ch,C_NRM));
  strcat(buf,buf2);
  
  if ((digits(age(ch).year)) < 16)
  for (i=1;i<(16-digits(age(ch).year));i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sCON%s: %s%d%s",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_CON(ch),CCNRM(ch,C_NRM));
  strcat(buf,buf2);

  j=digits(GET_REAL_CON(ch));
  if (GET_REAL_CON(ch)!=GET_AFF_CON(ch))
  {
    sprintf(buf2,"%s(%s%d%s)%s",
	CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_CON(ch),CCSEP(ch,C_NRM),CCNRM(ch,C_NRM));
    strcat(buf,buf2);
    j=j+2+digits(GET_AFF_CON(ch));
  }

  if (j < 12)
  for (i=1;i<(12-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Eighth line - Mana, Height/Weight, Dex */

  /* Choose the mana color */
  if(GET_MANA(ch) < GET_MAX_MANA(ch) / 4)
    sprintf(buf2,"%s",CCRED(ch,C_NRM));
  else if(GET_MANA(ch) < GET_MAX_MANA(ch) / 2)
    sprintf(buf2,"%s",CCBRED(ch,C_NRM));
  else if(GET_MANA(ch) < GET_MAX_MANA(ch))
    sprintf(buf2,"%s",CCBYEL(ch,C_NRM));
  else sprintf(buf2,"%s",CCBGRN(ch,C_NRM));  

  sprintf(buf, "%s %sMana Points%s  : %s%d%s/%s%d%s+%s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),buf2,GET_MANA(ch),CCSEP(ch,C_NRM),CCBGRN(ch,C_NRM),
	GET_MAX_MANA(ch),CCSEP(ch,C_NRM),CCGAIN(ch,C_NRM),mana_gain(ch));

  j=digits(GET_MANA(ch))+digits(GET_MAX_MANA(ch))+digits(mana_gain(ch))+2;

  if (j < 19)
  for (i=1;i<(19-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sHeight%s/%sWeight%s: %s%d%s,%s%d%s",
		cstar,CCSUB(ch,C_NRM),CCSEP(ch,C_NRM),CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),
		CCNUMB(ch,C_NRM),GET_HEIGHT(ch),CCSEP(ch,C_NRM),CCNUMB(ch,C_NRM),GET_WEIGHT(ch),CCNRM(ch,C_NRM));
  strcat(buf,buf2);

  if ((digits(GET_HEIGHT(ch))+digits(GET_WEIGHT(ch))+8) < 16)
  for (i=1;i<(16-digits(GET_HEIGHT(ch))-digits(GET_WEIGHT(ch))-8);i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sDEX%s: %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_DEX(ch));
  strcat(buf,buf2);

  j=digits(GET_REAL_DEX(ch));
  if (GET_REAL_DEX(ch)!=GET_AFF_DEX(ch))
  {
    sprintf(buf2,"%s(%s%d%s)",CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_DEX(ch),CCSEP(ch,C_NRM));
    strcat(buf,buf2);
    j=j+2+digits(GET_AFF_DEX(ch));
  }

  if (j < 12)
  for (i=1;i<(12-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Ninth Line - Movement, Level, Int */

  /* Choose the movement color */
  if(GET_MOVE(ch) < GET_MAX_MOVE(ch) / 4)
    sprintf(buf2,"%s",CCRED(ch,C_NRM));
  else if(GET_MOVE(ch) < GET_MAX_MOVE(ch) / 2)
    sprintf(buf2,"%s",CCBRED(ch,C_NRM));
  else if(GET_MOVE(ch) < GET_MAX_MOVE(ch))
    sprintf(buf2,"%s",CCBYEL(ch,C_NRM));
  else sprintf(buf2,"%s",CCBGRN(ch,C_NRM));

  sprintf(buf, "%s %sMovement%s     : %s%d%s/%s%d%s+%s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),buf2,GET_MOVE(ch),CCSEP(ch,C_NRM),CCBGRN(ch,C_NRM),
	GET_MAX_MOVE(ch),CCSEP(ch,C_NRM),CCGAIN(ch,C_NRM),move_gain(ch));

  j=digits(GET_MOVE(ch))+digits(GET_MAX_MOVE(ch))+digits(move_gain(ch))+2;

  if (j < 19)
  for (i=1;i<(19-j);i++)
    strcat(buf," ");

  j=GET_LEVEL(ch);
  if (j >= LVL_GOD)
    sprintf(buf3,"%s",CCBYEL(ch,C_SPR));
  else if (j >= LVL_ANGEL)
    sprintf(buf3,"%s",CCCYN(ch,C_SPR));
  else if (j >= LVL_IMMORT)
    sprintf(buf3,"%s",CCRED(ch,C_SPR));
  else if (j >= LVL_CHAMP)
    sprintf(buf3,"%s",CCBLU(ch,C_SPR));
  else
    sprintf(buf3,"%s",CCNRM(ch,C_SPR));

  sprintf(buf2,"%s %sLevel%s : %s%d%s",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),buf3,GET_LEVEL(ch),CCNRM(ch,C_SPR));
  strcat(buf,buf2);

  if ((digits(j)) < 16)
  for (i=1;i<(16-digits(j));i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sINT%s: %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_INT(ch));
  strcat(buf,buf2);

  j=digits(GET_REAL_INT(ch));

  if (GET_REAL_INT(ch)!=GET_AFF_INT(ch))
  {
    sprintf(buf2,"%s(%s%d%s)",CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_INT(ch),CCSEP(ch,C_NRM));
    strcat(buf,buf2);
    j=j+2+digits(GET_AFF_INT(ch));
  }

  if (j < 12)
  for (i=1;i<(12-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Tenth line - Hitroll, Align, Wisdom */

  sprintf(buf, "%s %sHitroll%s      : %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCDH(ch,C_NRM),ch->points.hitroll);

  if ((digits(ch->points.hitroll)) < 19)
  for (i=1;i<(19-digits(ch->points.hitroll));i++)
    strcat(buf," ");

  strcpy(alignbuf,"");

  if ((GET_ALIGNMENT(ch)>-101)&&(GET_ALIGNMENT(ch)<101))
    strcpy(alignbuf,"Neutral");
  if ((GET_ALIGNMENT(ch)>100)&&(GET_ALIGNMENT(ch)<301))
    strcpy(alignbuf,"NeutralG");
  /* Good alignment messages */
  if ((GET_ALIGNMENT(ch)>300)&&(GET_ALIGNMENT(ch)<401))
   strcpy(alignbuf,"Fair");
  if ((GET_ALIGNMENT(ch)>400)&&(GET_ALIGNMENT(ch)<501))
   strcpy(alignbuf,"Kind");
  if ((GET_ALIGNMENT(ch)>500)&&(GET_ALIGNMENT(ch)<601))
   strcpy(alignbuf,"Friendly");
  if ((GET_ALIGNMENT(ch)>600)&&(GET_ALIGNMENT(ch)<701))
   strcpy(alignbuf,"Honest");
  if ((GET_ALIGNMENT(ch)>700)&&(GET_ALIGNMENT(ch)<801))
   strcpy(alignbuf,"Humane");
  if ((GET_ALIGNMENT(ch)>800)&&(GET_ALIGNMENT(ch)<901))
   strcpy(alignbuf,"Virtuous");
  if ((GET_ALIGNMENT(ch)>901))
   strcpy(alignbuf,"Angelic");
  /* Evil Ratings */
  if ((GET_ALIGNMENT(ch)<-100)&&(GET_ALIGNMENT(ch)>-300))
   strcpy(alignbuf,"NeutralE");
  if ((GET_ALIGNMENT(ch)<-300)&&(GET_ALIGNMENT(ch)>-401))
   strcpy(alignbuf,"Unfair");
  if ((GET_ALIGNMENT(ch)<-400)&&(GET_ALIGNMENT(ch)>-501))
   strcpy(alignbuf,"Mean");
  if ((GET_ALIGNMENT(ch)<-500)&&(GET_ALIGNMENT(ch)>-601))
   strcpy(alignbuf,"Wicked");
  if ((GET_ALIGNMENT(ch)<-600)&&(GET_ALIGNMENT(ch)>-701))
   strcpy(alignbuf,"Sinful");
  if ((GET_ALIGNMENT(ch)<-700)&&(GET_ALIGNMENT(ch)>-801))
   strcpy(alignbuf,"Villanous");
  if ((GET_ALIGNMENT(ch)<-800)&&(GET_ALIGNMENT(ch)>-901))
   strcpy(alignbuf,"Demonic");
  if ((GET_ALIGNMENT(ch)<-901))
   strcpy(alignbuf,"Satanic");

  if (GET_ALIGNMENT(ch) > 350)
    sprintf(buf3,"%s",CCCYN(ch,C_NRM));
  else
    if (GET_ALIGNMENT(ch) < -350)
      sprintf(buf3,"%s",CCRED(ch,C_NRM));
    else
      sprintf(buf3,"%s",CCBWHT(ch,C_NRM));

  sprintf(buf2,"%s %sAlign%s : %s%s%s(%s%d%s)",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),buf3,alignbuf,CCSEP(ch,C_NRM),buf3,
	GET_ALIGNMENT(ch),CCSEP(ch,C_NRM));
  strcat(buf,buf2);

  j=2+strlen(alignbuf)+digits(GET_ALIGNMENT(ch));

  if (j < 16)
  for (i=1;i<(16-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sWIS%s: %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_WIS(ch));
  strcat(buf,buf2);

  j=digits(GET_REAL_WIS(ch));
  if (GET_REAL_WIS(ch)!=GET_AFF_WIS(ch))
  {
    sprintf(buf2,"%s(%s%d%s)",CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_WIS(ch),CCSEP(ch,C_NRM));
    strcat(buf,buf2);
    j=j+2+digits(GET_AFF_WIS(ch));
  }
  
  if (j < 12)
  for (i=1;i<(12-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Eleventh line - Damroll, gold, CHA */

  sprintf(buf, "%s %sDamroll%s      : %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCDH(ch,C_NRM),ch->points.damroll);

  if ((digits(ch->points.damroll)) < 19)
  for (i=1;i<(19-digits(ch->points.damroll));i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sGold%s  : %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCGOLD(ch,C_NRM),GET_GOLD(ch));
  strcat(buf,buf2);

  if ((digits(GET_GOLD(ch))) < 16)
  for (i=1;i<(16-digits(GET_GOLD(ch)));i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sCHA%s: %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_CHA(ch));
  strcat(buf,buf2);

  j=digits(GET_REAL_CHA(ch));
  if (GET_REAL_CHA(ch)!=GET_AFF_CHA(ch))
  {
    sprintf(buf2,"%s(%s%d%s)",CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_CHA(ch),CCSEP(ch,C_NRM));
    strcat(buf,buf2);
    j=j+2+digits(GET_AFF_CHA(ch));
  }
  
  if (j < 12)
  for (i=1;i<(12-j);i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Twelth line - Armor Class, Bank */

  sprintf(buf, "%s %sArmour Class%s : %s%d%s/%s10",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCACT(ch,C_NRM),GET_AC(ch),CCSEP(ch,C_NRM),CCACT(ch,C_NRM));

  if ((digits(GET_AC(ch))+3) < 19)
  for (i=1;i<(19-digits(GET_AC(ch))-3);i++)
    strcat(buf," "); 

  sprintf(buf2,"%s %sBank%s  : %s%d",
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCGOLD(ch,C_NRM),GET_BANK_GOLD(ch));
  strcat(buf,buf2);

  if ((digits(GET_BANK_GOLD(ch))) < 16)
  for (i=1;i<(16-digits(GET_BANK_GOLD(ch)));i++)
    strcat(buf," ");

  sprintf(buf2,"%s",cstar);
  strcat(buf,buf2);

  for (i=1;i<18;i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Thirteenth line - Thac0, Time */

  sprintf(buf, "%s %sThac%s0%s        : %s%d",
	cstar,CCSUB(ch,C_NRM),CCACT(ch,C_NRM),CCNRM(ch,C_NRM),CCACT(ch,C_NRM),thaco(ch)); 

  if ((digits(thaco(ch))) < 19)
  for (i=1;i<(19-digits(thaco(ch)));i++)
    strcat(buf," ");

  playing_time = real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
				  
  sprintf(buf2,"%s %sTime%s  : %s%d%sd%s%d%sh", 
	cstar,CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCNUMB(ch,C_NRM),playing_time.day,CCSEP(ch,C_NRM),
	CCNUMB(ch,C_NRM),playing_time.hours,CCSEP(ch,C_NRM));
  strcat(buf,buf2);

  if ((digits(playing_time.hours)+digits(playing_time.day)+2) < 16)
  for (i=1;i<(16-digits(playing_time.hours)-digits(playing_time.day)-2);i++)
    strcat(buf," ");

  sprintf(buf2,"%s",cstar);
  strcat(buf,buf2);

  for (i=1;i<18;i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Fourteenth line - ***** */
  send_to_char(cline, ch);  

/* Fifteenth line - Carrying Weight, Carrying Items, Can Carry Weight, Can Carry Items */

  sprintf(buf,"%s              %sInventory                                    %s                 %s%s\r\n",
		cstar,CCHEAD(ch,C_NRM),cstar,cstar,CCNRM(ch,C_NRM));
  send_to_char(buf,ch);
  send_to_char(cline,ch);

  sprintf(buf,"%s %sCarrying weight %s: %s%d",
                cstar, CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCNUMB(ch,C_NRM),IS_CARRYING_W(ch));

  if ((digits(IS_CARRYING_W(ch))) < 11)
  for (i=0; i < (11-digits(IS_CARRYING_W(ch))); i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sCarrying Items %s: %s%d",
                cstar, CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCNUMB(ch,C_NRM),IS_CARRYING_N(ch));

  if ((digits(IS_CARRYING_N(ch))) < 10)
  for (i=0; i < (10-digits(IS_CARRYING_N(ch))); i++)
    strcat(buf2," ");

  strcat(buf2,cstar);
  for (i=0; i < 17; i++)
    strcat(buf2, " ");

  strcat(buf2,cstar);
  strcat(buf2,"\r\n");
  strcat(buf,buf2);
  send_to_char(buf,ch);

  sprintf(buf,"%s %sCan Carry Weight%s: %s%d",
		cstar, CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCNUMB(ch,C_NRM),CAN_CARRY_W(ch));

  
  if ((digits(CAN_CARRY_W(ch))) < 11)
  for (i=0; i < (11-digits(CAN_CARRY_W(ch))); i++)
    strcat(buf," ");

  sprintf(buf2,"%s %sCan Carry Items%s: %s%d",
                cstar, CCSUB(ch,C_NRM),CCNRM(ch,C_NRM),CCNUMB(ch,C_NRM),CAN_CARRY_N(ch));

  if ((digits(CAN_CARRY_N(ch))) < 10)
  for (i=0; i < (10-digits(CAN_CARRY_N(ch))); i++)
    strcat(buf2," ");

  strcat(buf2,cstar);
  for (i=0; i < 17; i++)
    strcat(buf2, " ");

  strcat(buf2,cstar);
  sprintf(buf3,"%s\r\n",CCNRM(ch,C_NRM));
  strcat(buf2,buf3);
  strcat(buf,buf2);
  send_to_char(buf,ch);

  send_to_char(cline,ch);
/* Sixteenth line - ***** */
  
  if ( !GET_CLAN_NUM (ch) < 1 ) 
	sprintf(buf, "You are part of the %s and your rank is %s.\r\n",
         get_clan_disp(ch), get_clan_rank(ch));
  else
	sprintf(buf, "You are clanless.\r\n" );

  send_to_char(buf,ch);

  strcpy(buf,"");  
    if (GET_LEVEL(ch) < LVL_ISNOT_GOD){
      sprintf(buf, "You need %s%d%s exp to reach your next level.\r\n", 
	CCEXP(ch,C_NRM),(level_exp[GET_LEVEL(ch)]) - GET_EXP(ch),CCNRM(ch,C_NRM));
      send_to_char(buf, ch);  
   }
  strcpy(buf,"");
  switch (GET_POS(ch)) {
  case POS_DEAD:
    strcat(buf, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    strcat(buf, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    strcat(buf, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    strcat(buf, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    strcat(buf, "You are resting.\r\n");
    break;
  case POS_SITTING:
    strcat(buf, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    if (FIGHTING(ch))
      sprintf(buf, "%sYou are fighting %s.\r\n", buf, PERS(FIGHTING(ch), ch));
    else
      strcat(buf, "You are fighting thin air.\r\n");
    break;
  case POS_STANDING:
    strcat(buf, "You are standing.\r\n");
    break;
  default:
    strcat(buf, "You are floating.\r\n");
    break;
  }

  if (EXT_FLAGGED(ch, EXT_AUTOLOOT))
    strcat(buf, "You are autolooting corpses.\r\n");

  if (EXT_FLAGGED(ch, EXT_AUTOGOLD)) {
    strcat(buf, "You are autolooting gold from corpses.\r\n");
    if (EXT_FLAGGED(ch, EXT_AUTOSPLIT))
      strcat(buf, "You are autospliting gold between group members.\r\n");
  }

  if (AUTOASSIST(ch)) {
    sprintf(buf2, "You are autoassisting %s.\r\n",GET_NAME(AUTOASSIST(ch)));
    strcat(buf,buf2); 
  }

  if (GET_COND(ch, DRUNK) > 10)
    strcat(buf, "You are intoxicated.\r\n");

  if (GET_COND(ch, FULL) == 0)
    strcat(buf, "You are hungry.\r\n");

  if (GET_COND(ch, THIRST) == 0)
    strcat(buf, "You are thirsty.\r\n");

  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");

  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");

  if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");

  if (IS_AFFECTED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");

  if (IS_AFFECTED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");

  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");

  if (IS_AFFECTED(ch, AFF_WATERWALK))
    strcat(buf, "You have magically webbed feet.\r\n");

  if (IS_AFFECTED(ch, AFF_WATERBREATHE))
    strcat(buf, "You can breathe underwater.\r\n");

  if (IS_AFFECTED(ch, AFF_REFLECT))
    strcat(buf, "You have shiny scales on your skin.\r\n");

  if (IS_AFFECTED(ch, AFF_FLY))
    strcat(buf, "You are flying.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");

  if( MOUNTING(ch) ) {
   	sprintf(buf2, "You are mounted on %s.\r\n", GET_NAME(MOUNTING(ch)) );
  	strcat(buf, buf2);
  }
  if (affected_by_spell(ch,SPELL_CHANGED))
    if(PRF_FLAGGED(ch,PRF_WOLF))
      strcat(buf, "You're a Werewolf!\r\n");
    else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
      strcat(buf, "You're a Vampire!\r\n");


  send_to_char(buf, ch);  
}

ACMD(do_affects)
{
	struct affected_type *aff;

  strcpy(buf,"");

#ifdef ANNOY_SANDII
  // Artus> Just thought I'd have a little fun :o)
  if ((GET_LEVEL(ch) == 175) && !(strcmp(GET_NAME(ch), "Sandii")))
    strcat(buf, "You are pregnant.\r\n");
#endif
  if (PRF_FLAGGED(ch, PRF_SUMMONABLE)) 
    strcat(buf, "You are summonable by other players.\r\n");
  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");
  if (IS_AFFECTED(ch, AFF_WATERWALK))
    strcat(buf, "You have magically webbed feet.\r\n");
  if (IS_AFFECTED(ch, AFF_WATERBREATHE))
    strcat(buf, "You can breathe underwater.\r\n");
  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");
  if (affected_by_spell(ch, SPELL_STRENGTH))
    strcat(buf, "You feel stronger.\r\n");
  if (affected_by_spell(ch, SPELL_CHILL_TOUCH))
    strcat(buf, "You feel weakened.\r\n");
  if (IS_AFFECTED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");
  if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");
  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");
  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");
  if (IS_AFFECTED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");
  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");
  if (affected_by_spell(ch, SPELL_BLESS))
    strcat(buf, "You feel righteous.\r\n");
  if (IS_AFFECTED(ch, AFF_CURSE))
    strcat(buf, "You are cursed.\r\n");
  if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
    strcat(buf, "You feel more aware.\r\n");
  if (IS_AFFECTED(ch, AFF_SNEAK))
    strcat(buf, "You feel more sneaky.\r\n");
  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN))
    strcat(buf, "You have the ability to see auras.\r\n");
  if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
    strcat(buf, "You are more sensitive to magic.\r\n");
  if (IS_AFFECTED(ch, AFF_PROTECT_EVIL))
    strcat(buf, "You feel protected from Evil.\r\n");
  if (IS_AFFECTED(ch, AFF_PROTECT_GOOD))
     strcat(buf, "You feel protected from Good.\r\n");
  if (IS_AFFECTED(ch, AFF_SLEEP))
     strcat(buf, "You are asleep.\r\n");
  if (IS_AFFECTED(ch, AFF_REFLECT))
     strcat(buf, "You have shiny scales on your skin.\r\n");
  if (IS_AFFECTED(ch, AFF_FLY))
     strcat(buf, "You are flying.\r\n");
  if (affected_by_spell(ch,SPELL_CHANGED))
    if(PRF_FLAGGED(ch,PRF_WOLF))
      strcat(buf, "You're a Werewolf!\r\n"); 
    else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
      strcat(buf, "You're a Vampire!\r\n");
  if (IS_AFFECTED(ch, AFF_PARALYZED))
     strcat(buf, "You are paralyzed.\r\n");
  if (affected_by_spell(ch,SPELL_DRAGON))
     strcat(buf, "You feel like a dragon!\r\n");

  send_to_char(buf, ch);
	if (GET_LEVEL(ch) < 20)
		return;

	/* Routine to show what spells a char is affected by */
  if (ch->affected) 
  {
    for (aff = ch->affected; aff; aff = aff->next) 
    {
      *buf2 = '\0';
      sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1,
        CCCYN(ch, C_NRM), spells[aff->type], CCNRM(ch, C_NRM));
      if (aff->modifier) 
      {
        sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
        strcat(buf, buf2);
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

}

/* Moved to text/areas - DM 
ACMD(do_areas)
{
  strcpy(buf,"");

  strcat(buf,"                        (Medieval)\r\n");
  strcat(buf,"      Zone Name          Zone Lev  Zone type    Author\r\n");
  strcat(buf,"=======================+=========+==========+=============\r\n");
  strcat(buf,"Haven                  | (0-10)  |    -     |   John\r\n");
  strcat(buf,"MoatHouse              | (0-10)  |    -     |   John\r\n");
  strcat(buf,"Selve Road             | (0-10)  |    -     | Cassandra\r\n");
  strcat(buf,"Graveyard              | (0-10)  |    -     | Cassandra\r\n");
  strcat(buf,"Plains                 | (0-10)  |    -     |   John\r\n");
  strcat(buf,"Mountains              | (0-10)  |    -     |   John\r\n");
  strcat(buf,"Rome                   | (5-70)  |    -     | Circle Def\r\n");
  strcat(buf,"Aqua Marina            | (5-70)  |    -     |  Abulafia\r\n");
  strcat(buf,"Werith's Wayhouse      | (10-20) |    -     | Circle Def\r\n");
  strcat(buf,"Darask                 | (10-20) |    -     |   Murf\r\n");
  strcat(buf,"Toyheim                | (10-20) |    -     |   Drax\r\n");
  strcat(buf,"Rands Tower            | (10-20) |    -     | Circle Def\r\n");
  strcat(buf,"3 of Swords            | (10-20) |    -     | Circle Def\r\n");
  strcat(buf,"LhankWood              | (10-20) |    -     |   John\r\n");
  strcat(buf,"Ersheep Road           | (10-20) |    -     |   Gabe\r\n");
  strcat(buf,"Rushmoor Swamp         | (20-30) |    -     |   John\r\n");
  strcat(buf,"Fairytale Forest       | (20-30) |    -     |  Ruprect\r\n"); 
  strcat(buf,"Camalot                | (25-40) |    -     |  Backspace\r\n");
  strcat(buf,"Crystal Castle         | (25-40) |  PKILL   |  Freako\r\n");
  strcat(buf,"Island                 | (30-40) |  PKILL   | Cassandra\r\n");
  strcat(buf,"Pirates Ship           | (30-40) |  PKILL   |   John\r\n");
  strcat(buf,"Khulgur's Lost Empire  | (30-40) |    -     |   Calrain\r\n");
  strcat(buf,"Tunnel                 | (35-45) |  PKILL   | Cassandra\r\n");
  strcat(buf,"Tudor                  |  (35+)  |  PKILL   |   John\r\n");
  strcat(buf,"Magic: The Gathering   |  (35+)  |  PKILL   |Deathblade & Pkunk\r\n");
  strcat(buf,"Shadow Keep            | (30-50) |  PKILL   |   Gabe\r\n");
  strcat(buf,"Danger Island          | (40-50) |  PKILL   |   John\r\n");
  strcat(buf,"Black Tower            | (30-50) |    -     | Cassandra\r\n");
  strcat(buf,"Holy City              |  (50+)  |  PKILL   | Cassandra\r\n");
  strcat(buf,"Spiral                 |  (50+)  |  PKILL   | Cassandra\r\n");
  strcat(buf,"Moanders Domain(Abyss) |  (50+)  |  PKILL   |   Drax\r\n");
  strcat(buf,"Ancient Greece         |  (60+)  |    -     |  Ruprect\r\n");

  strcat(buf,"Wilderness             | (0-10)  |    -     |  ??\r\n");
  strcat(buf,"Eliqua River           | (35+)   |    -     |  ??\r\n");

  strcat(buf,"\r\n=======================+=========+==========+=============\r\n");
  strcat(buf,"                      (West World)\r\n");
  strcat(buf,"      Zone Name          Zone Lev  Zone type    Author\r\n");
  strcat(buf,"=======================+=========+==========+=============\r\n");
  strcat(buf,"City of HOPE           | (0-20)  |    -     |  Ruprect\r\n");
  strcat(buf,"Fort Hope              | (35+)   |    -     |  Tsd\r\n");
  strcat(buf,"Saloon                 | (0-20)  |    -     |  Ruprect\r\n");
  strcat(buf,"Whore House            | (0-20)  |    -     |  Ruprect\r\n");
  strcat(buf,"Blue Creek             | (0-20)  |    -     |  Ruprect\r\n");
  strcat(buf,"Rockbridge             | (20+)   |    -     |  Ruprect\r\n");

  strcat(buf,"\r\n=======================+=========+==========+=============\r\n");
  strcat(buf,"                     (Future World)\r\n");
  strcat(buf,"      Zone Name          Zone Lev  Zone type    Author\r\n");
  strcat(buf,"=======================+=========+==========+=============\r\n");
  strcat(buf,"The Lost Valley        |  (45+)  |  PKILL   |  Boz\r\n");
  strcat(buf,"Angel Grove            |  (25+)  |  PKILL   |  Boz\r\n");
  strcat(buf,"Lunar City IV          | (0-20)  |    -     |  ??\r\n");
  strcat(buf,"Mos Eisley Starport    |         |    -     |  ??\r\n");
  strcat(buf,"Jabba's Palace         |         |    -     |  ??\r\n");
  strcat(buf,"Perseid Mining Colony  |  (30+)  |    -     |  Intan??\r\n");
  strcat(buf,"Babylon 5              |  (25+)  |  PKILL   |  Moridin\r\n");
  strcat(buf,"The Sphere             |  (70+)  |  PKILL   |  Cassandra\r\n");

  page_string(ch->desc, buf, 1);
}
*/

ACMD(do_exp)
{
    if (GET_LEVEL(ch) < LVL_ISNOT_GOD){
      sprintf(buf, "You need %s%d%s exp to reach your next level.\r\n", 
        CCEXP(ch,C_NRM),(level_exp[GET_LEVEL(ch)]) - GET_EXP(ch),CCNRM(ch,C_NRM));
      send_to_char(buf, ch);  
   }
}


ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    if (ch->equipment[i]) {
      if (CAN_SEE_OBJ(ch, ch->equipment[i])) {
	send_to_char(where[i], ch);
	show_obj_to_char(ch->equipment[i], ch, 1);
	found = TRUE;
      } else {
	send_to_char(where[i], ch);
	send_to_char("Something.\r\n", ch);
	found = TRUE;
      }
    }
  }
  if (!found) {
    send_to_char(" Nothing.\r\n", ch);
  }
}


ACMD(do_time)
{
  char *suf;
  int weekday, day;
  extern struct time_info_data time_info;
  extern const char *weekdays[];
  extern const char *month_name[];

  sprintf(buf, "It is %d o'clock %s, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  day = time_info.day + 1;	/* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);

  send_to_char(buf, ch);
}


ACMD(do_weather)
{
  static char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
  "lit by flashes of lightning"};

  if (OUTSIDE(ch)) {
    sprintf(buf, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    (weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
}


/* command to tell ya what the moon is - Vader */
ACMD(do_moon)
{
  extern struct time_info_data time_info;
  extern char *moon_mesg[];
  byte day = time_info.day % 3;

  if(weather_info.moon == MOON_NONE) {
    send_to_char("It is a moonless night.\r\n",ch);
    return;
    }

  if(!(time_info.hours > 12)) {
    day--;
    if(day < 0)
      day = 2;
    }

  switch(day) {
    case 0:
      sprintf(buf,"It is the 1st night of the %s moon.\r\n",moon_mesg[weather_info.moon]);
      break;
    case 1:
      sprintf(buf,"It is the 2nd night of the %s moon.\r\n",moon_mesg[weather_info.moon]);
      break;
    case 2:
      sprintf(buf,"It is the 3rd night of the %s moon.\r\n",moon_mesg[weather_info.moon]);
      break;
    }

  send_to_char(buf,ch);
}


ACMD(do_help)
{
  extern int top_of_helpt;
  extern struct help_index_element *help_index;
  extern FILE *help_fl;
  extern char *help;

  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_index) {
    send_to_char("No help available.\r\n", ch);
    return;
  }
  bot = 0;
  top = top_of_helpt;

  for (;;) {
    mid = (bot + top) >> 1;
    minlen = strlen(argument);

    if (!(chk = strn_cmp(argument, help_index[mid].keyword, minlen))) {

      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_index[mid - 1].keyword, minlen))))
	mid--;
      fseek(help_fl, help_index[mid].pos, SEEK_SET);
      *buf2 = '\0';
      for (;;) {
	fgets(buf, 128, help_fl);
	if (*buf == '#')
	  break;
	buf[strlen(buf) - 1] = '\0';	/* cleave off the trailing \n */
	strcat(buf2, strcat(buf, "\r\n"));
      }
      page_string(ch->desc, buf2, 1);
      return;
    } else if (bot >= top) {
      send_to_char("There is no help on that word.\r\n", ch);
      return;
    } else if (chk > 0)
      bot = ++mid;
    else
      top = --mid;
  }
}



#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-c classlist] [-s] [-o] [-q] [-r] [-z]\r\n"

ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH];
  char mode;
  int low = 0, high = LVL_OWNER, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0;
  char s[5];
  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';

  if (!(GET_LEVEL(ch) < LVL_OWNER))
    high = GET_LEVEL(ch);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = 1;
	strcpy(buf, buf1);
	break;
      case 'c':
/* JA there are no classes on primal now
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
*/
	send_to_char("Havn't you figured it out yet? There are no classes on Primal!", ch);
	return;
	break;
      default:
	send_to_char(WHO_FORMAT, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(WHO_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */

  send_to_char("&yPlayers\r\n&n-------\r\n", ch);

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected)
      continue;

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
/* Darius - check if in same world or ch in godroom */
/*    if((!same_world(tch,ch) && !(world[ch->in_room].zone == GOD_ROOMS_ZONE)) && (GET_LEVEL(ch)<LVL_GRGOD))
      continue; */
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[ch->in_room].zone != world[tch->in_room].zone)
      continue;
    if (who_room && (tch->in_room != ch->in_room))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
    if (short_list) {
      sprintf(buf, "%s[%3d %s] %-12.12s%s%s",
              (GET_LEVEL(tch) >= LVL_GOD ? CCBYEL(ch,C_SPR) :
               (GET_LEVEL(tch) >= LVL_ANGEL ? CCCYN(ch,C_SPR) :
               (GET_LEVEL(tch) >= LVL_IMMORT ? CCRED(ch,C_SPR) :
	       (GET_LEVEL(tch) >= LVL_ETRNL1 ? CCBLU(ch, C_SPR) : "")))),
	      GET_LEVEL(tch), CLASS_ABBR(tch), GET_NAME(tch),
	      (GET_LEVEL(tch) >= LVL_IS_GOD || GET_LEVEL(tch) >= LVL_ETRNL1 ? CCNRM(ch, C_SPR) : ""),
	      ((!(++num_can_see % 4)) ? "\r\n" : ""));
      send_to_char(buf, ch);
    } else {
      num_can_see++;
      switch(GET_LEVEL(tch))
      {
	case LVL_OWNER  : strcpy(s,"ONR");break;
        case LVL_GRIMPL : strcpy(s,"BCH");break;
        case LVL_IMPL   : strcpy(s,"IMP");break;
        case LVL_GRGOD  : strcpy(s,"GOD");break;
        case LVL_GOD    : 
	  if (!(strcmp(GET_NAME(tch), "Artus")))
	    strcpy(s,"COW");
	  else
	    strcpy(s,"DEI");
	  break;
 	case LVL_ANGEL  : 
	  if (!(strcmp(GET_NAME(tch), "Boz")))
	    strcpy(s,"POV");
          else if (!(strcmp(GET_NAME(tch), "Gamina")))
            strcpy(s,"BOT");
	  else 
	    strcpy(s,"ANG");
	  break;
        case LVL_IMMORT : 
          if (!(strcmp(GET_NAME(tch), "AmaroK")))
            strcpy(s,"NiN");
          else
            strcpy(s,"IMM");
          break;
	case LVL_HELPER :
	    strcpy(s,"HLP");
	  break;
        case LVL_CHAMP  : 
            strcpy(s,"CMP");
          break;
        case LVL_ETRNL9 : strcpy(s,"ET9");break;
        case LVL_ETRNL8 : strcpy(s,"ET8");break;
        case LVL_ETRNL7 : strcpy(s,"ET7");break;
        case LVL_ETRNL6 : strcpy(s,"ET6");break;
        case LVL_ETRNL5 : strcpy(s,"ET5");break;
        case LVL_ETRNL4 : strcpy(s,"ET4");break;
        case LVL_ETRNL3 : strcpy(s,"ET3");break;
        case LVL_ETRNL2 : strcpy(s,"ET2");break;
        case LVL_ETRNL1 : strcpy(s,"ET1");break;
        default         : strcpy(s,"PLR");
      }
      sprintf(buf, "%s[%3d %c %s %s] %s %s%s",
	      (GET_LEVEL(tch) >= LVL_GOD ? CCBYEL(ch, C_SPR) :
               (GET_LEVEL(tch) >= LVL_ANGEL ? CCCYN(ch,C_SPR) :
               (GET_LEVEL(tch) >= LVL_IMMORT ? CCRED(ch,C_SPR) :
	       (GET_LEVEL(tch) >= LVL_CHAMP ? CCBLU(ch,C_SPR) : "")))),
	      GET_LEVEL(tch),  
              (GET_SEX(tch)==SEX_MALE ? 'M' : 
               GET_SEX(tch)==SEX_FEMALE ? 'F' : '-'),
              CLASS_ABBR(tch),
              s,
              GET_NAME(tch),
	      GET_TITLE(tch),
	      (GET_LEVEL(tch) >= LVL_GOD ? CCBYEL(ch, C_SPR) :
               (GET_LEVEL(tch) >= LVL_ANGEL ? CCCYN(ch,C_SPR) :
               (GET_LEVEL(tch) >= LVL_IMMORT ? CCRED(ch,C_SPR) :
	       (GET_LEVEL(tch) >= LVL_CHAMP ? CCBLU(ch,C_SPR) : ""))))
		);

      if (GET_CLAN_NUM(tch) > 0 )
      { 
       if (EXT_FLAGGED(tch, EXT_LEADER))
       {
         strcat( buf , CCBLU(ch, C_SPR));
         sprintf ( buf , "%s [%s - %s]" , buf , get_clan_disp (tch),
                                    get_clan_rank (tch) ) ;
         strcat( buf , CCNRM(ch, C_SPR));
       }
       else if (EXT_FLAGGED(tch, EXT_SUBLEADER))
       {  
	 strcat( buf , CCCYN(ch, C_SPR));
         sprintf ( buf , "%s [%s - %s]" , buf , get_clan_disp (tch) ,
                                    get_clan_rank (tch) ) ;
         strcat( buf , CCNRM(ch, C_SPR));
       }
       else
         sprintf ( buf , "%s [%s - %s]" , buf , get_clan_disp (tch) ,
                                    get_clan_rank (tch) ) ;
      }

      if (GET_INVIS_LEV(tch)) {
 	// Standard invis?
	if(GET_INVIS_TYPE(tch) == INVIS_NORMAL ) 
	  sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(tch));
	// Level invis?
	else if( GET_INVIS_TYPE(tch) == -1 )
	  sprintf(buf, "%s (i%ds)", buf, GET_INVIS_LEV(tch));
	// Player invis?
	else if( GET_INVIS_TYPE(tch) == -2 )
	  sprintf(buf, "%s (i-char)", buf);
	else
	// Ranged invis!
	  sprintf(buf, "%s (i%d - %d)", buf, GET_INVIS_LEV(tch), GET_INVIS_TYPE(tch));
      }
      else if (IS_AFFECTED(tch, AFF_INVISIBLE))
	strcat(buf, " (invis)");

      if (PLR_FLAGGED(tch, PLR_MAILING))
	strcat(buf, " (mailing)");
      else if (PLR_FLAGGED(tch, PLR_WRITING))
	strcat(buf, " (writing)");

      if ( GET_IGN_NUM(tch) > 0 || GET_IGN_LEVEL(tch) > 0)
        strcat(buf, " (snob)"); 
      if (PRF_FLAGGED(tch, PRF_AFK))
	strcat(buf, " (AFK)");
      if (PRF_FLAGGED(tch, PRF_DEAF))
	strcat(buf, " (deaf)");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	strcat(buf, " (notell)");
      if (PRF_FLAGGED(tch, PRF_QUEST))
	strcat(buf, " (quest)");
#ifdef ANNOY_SANDII
      // Artus> And a little more..
      if ((GET_LEVEL(tch) == 175) && (!strcmp(GET_NAME(tch), "Sandii"))
		                  && (strcmp(GET_NAME(ch), "Sandii")))
	strcat(buf, " (pregnant)");
#endif
      if (PLR_FLAGGED(tch, PLR_THIEF))
	strcat(buf, " (THIEF)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
	strcat(buf, " (KILLER)");
      if (PRF_FLAGGED(tch, PRF_TAG))
        strcat(buf, " (tag)");
      if (GET_LEVEL(tch) >= LVL_IS_GOD || GET_LEVEL(tch) >= LVL_ETRNL1)
	strcat(buf, CCNRM(ch, C_SPR));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }				/* endif shortlist */
  }				/* end of for */
  if (short_list && (num_can_see % 4))
    send_to_char("\r\n", ch);
  if (num_can_see == 0)
    sprintf(buf, "\r\nNo-one at all!\r\n");
  else if (num_can_see == 1)
    sprintf(buf, "\r\nOne lonely character displayed.\r\n");
  else
    sprintf(buf, "\r\n%d characters displayed.\r\n", num_can_see);
  send_to_char(buf, ch);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
  extern char *connected_types[];
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr;

  struct char_data *tch;
  struct descriptor_data *d;
  char name_search[80], host_search[80], mode, *format;
  int low = 0, high = LVL_IMPL, i, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      case 'c':
	playing = 1;
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      default:
	send_to_char(USERS_FORMAT, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */
  strcpy(line,
	 "Num Class    Name         State          Idl Login@   Site\r\n");
  strcat(line,
	 "--- -------- ------------ -------------- --- -------- ------------------------\r\n");
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && playing)
      continue;
    if (!d->connected && deadweight)
      continue;
    if (!d->connected) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (!same_world(ch,tch) && GET_LEVEL(ch)<LVL_GRGOD)
        continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;

      if (d->original)
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->original),
		CLASS_ABBR(d->original));
      else
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character));
    } else
      strcpy(classname, "   -    ");

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (!d->connected && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[d->connected]);

    if (d->character && !d->connected && GET_LEVEL(d->character) < LVL_IS_GOD)
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    format = "%3d %-7s %-12s %-14s %-3s %-8s ";

    if (d->character && d->character->player.name) {
      if (d->original)
	sprintf(line, format, d->desc_num, classname,
		d->original->player.name, state, idletime, timeptr);
      else
	sprintf(line, format, d->desc_num, classname,
		d->character->player.name, state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED",
	      state, idletime, timeptr);

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (d->connected) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (d->connected || (!d->connected && CAN_SEE(ch, d->character))) {
      send_to_char(line, ch);
      num_can_see++;
    }
  }

  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  extern char circlemud_version[];

  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(circlemud_version, ch);
    break;
  case SCMD_WHOAMI:
    send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
    break;
  case SCMD_AREAS:
    page_string(ch->desc, areas, 0);
    break;
  default:
    return;
    break;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{
  int zone_name_len, j;
  register struct char_data *i;
  register struct descriptor_data *d;
  extern struct zone_data *zone_table;

  if (!*arg) {
    zone_name_len=strlen(zone_table[world[ch->in_room].zone].name);
    sprintf(buf,"Players in &R%s&n\r\n", zone_table[world[ch->in_room].zone].name);
    send_to_char(buf,ch);
    for (j=0; j < (11+zone_name_len); j++)
      send_to_char("-",ch);
    send_to_char("\r\n",ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE) &&
	    (world[ch->in_room].zone == world[i->in_room].zone)) {
	  sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next)
      if (world[i->in_room].zone == world[ch->in_room].zone && CAN_SEE(ch, i) &&
	  (i->in_room != NOWHERE) && isname(arg, i->player.name)) {
	sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	send_to_char(buf, ch);
	return;
      }
    send_to_char("No-one around by that name.\r\n", ch);
  }
}


void print_object_location(int num, struct obj_data * obj, struct char_data * ch,
			        int recur)
{
  if (num > 0)
    sprintf(buf, "&bO&n%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(buf, "%33s", " - ");

  if (obj->in_room > NOWHERE) {
    sprintf(buf + strlen(buf), "&g[&n%5d&g]&n %s\n\r",
	    world[obj->in_room].number, world[obj->in_room].name);
    send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(buf + strlen(buf), "carried by %s\n\r",
	    PERS(obj->carried_by, ch));
    send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(buf + strlen(buf), "worn by %s\n\r",
	    PERS(obj->worn_by, ch));
    send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(buf + strlen(buf), "inside %s%s\n\r",
	    obj->in_obj->short_description, (recur ? ", which is" : " "));
    send_to_char(buf, ch);
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else {
    sprintf(buf + strlen(buf), "in an unknown location\n\r");
    send_to_char(buf, ch);
  }
}



void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char("&gPlayers&n\r\n&y-------&n\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE) && (same_world(ch,i) || GET_LEVEL(ch)>=LVL_GRGOD)) {
	  if (d->original)
	    sprintf(buf, "%-20s - &g[&n%5d&g]&n %s (in %s)\r\n",
		    GET_NAME(i), world[d->character->in_room].number,
		 world[d->character->in_room].name, GET_NAME(d->character));
	  else
	    sprintf(buf, "%-20s - &g[&n%5d&g]&n %s\r\n", GET_NAME(i),
		    world[i->in_room].number, world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	sprintf(buf, "&rM&n%3d. %-25s - &g[&n%5d&g]&n %s\r\n", ++num, GET_NAME(i),
		world[i->in_room].number, world[i->in_room].name);
	send_to_char(buf, ch);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char("Couldn't find any such thing.\r\n", ch);
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IS_GOD)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  int i;

  if (IS_NPC(ch)) {
    send_to_char("You ain't nothin' but a hound-dog.\r\n", ch);
    return;
  }
  *buf = '\0';

/* DM_exp adjust levels for new exp system */
  sprintf(buf,"Experience required at level:\r\n");
  strcat(buf,"&y-----------------------------&n\r\n");

  for (i = 1; i < LVL_CHAMP; i++) {
    sprintf(buf + strlen(buf), "&g[&n%3d&g]&n &c%9d&n\r\n", i,
	    level_exp[i]);
  }
/* JA use paging here now as levels is too many lines long */
  page_string(ch->desc, buf, 1);
 /* send_to_char(buf, ch); */
}



ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("Easy!  Very easy indeed!\r\n", ch);
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char("Now where did that chicken go?\r\n", ch);
  else if (diff <= -5)
    send_to_char("You could do it with a needle!\r\n", ch);
  else if (diff <= -2)
    send_to_char("Easy.\r\n", ch);
  else if (diff <= -1)
    send_to_char("Fairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\r\n", ch);
  else if (diff <= 1)
    send_to_char("You would need some luck!\r\n", ch);
  else if (diff <= 2)
    send_to_char("You would need a lot of luck!\r\n", ch);
  else if (diff <= 3)
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 5)
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  else if (diff <= 10)
    send_to_char("Are you mad!?\r\n", ch);
  else if (diff <= 100)
    send_to_char("You ARE mad!\r\n", ch);

}



ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    } else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}


static char *ctypes[] = {
"off", "sparse", "normal", "complete", "\n"};

ACMD(do_color)
{
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

  sprintf(buf, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
	  CCNRM(ch, C_OFF), ctypes[tp]);
  send_to_char(buf, ch);
}

/* DM - modified for new toggles and to toggle others and from file */
ACMD(do_toggle)
{
  struct char_data *victim;
  struct char_file_u tmp_store;

  if (IS_NPC(ch))
    return;

  half_chop(argument, buf1, buf2);

  if ((!*buf1) || (GET_LEVEL(ch) < LVL_CHAMP)) {
    toggle_display(ch,ch);
    return;
  }

  if (is_abbrev(buf1, "file")) {
    if (!*buf2) {
      send_to_char("Toggle's for who?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1) {
        store_to_char(&tmp_store, victim);
        if (GET_LEVEL(victim) > GET_LEVEL(ch))
          send_to_char("Sorry, you can't do that.\r\n", ch);
        else
          toggle_display(ch, victim);
        free_char(victim);
      } else {
        send_to_char("There is no such player.\r\n", ch);
        free(victim);
      }
    }                           
  } else {
    if ((victim = get_char_vis(ch,buf1,TRUE))) {
      toggle_display(ch,victim); 
    } else {
      send_to_char("Toggle's for who?\r\n", ch);
    } 
  }
}

/* DM - Toggle display on vict */
void toggle_display(struct char_data *ch, struct char_data *vict)
{

  if (GET_WIMP_LEV(vict) == 0)
    strcpy(buf2, "OFF");
  else
    sprintf(buf2, "%-3d", GET_WIMP_LEV(vict));

  sprintf(buf,
	  " Character Name: %s\r\n"

	  "Hit Pnt Display: %-3s    " "     Brief Mode: %-3s    " " Summon Protect: %-3s\r\n"
	  "   Move Display: %-3s    " "   Compact Mode: %-3s    " "       On Quest: %-3s\r\n"
	  "   Mana Display: %-3s    " " Auto Show Exit: %-3s    " " Gossip Channel: %-3s\r\n"
          "    Exp Display: %-3s    " "   Repeat Comm.: %-3s    " "Auction Channel: %-3s\r\n"
	  "  Align Display: %-3s    " "           Deaf: %-3s    " "  Grats Channel: %-3s\r\n"
	  "       Autoloot: %-3s    " "         NoTell: %-3s    " " Newbie Channel: %-3s\r\n"
          "       Autogold: %-3s    " "     Marked AFK: %-3s    " "   Clan Channel: %-3s\r\n"
          "      Autosplit: %-3s    " " Clan Available: %-3s    " "   Info Channel: %-3s\r\n"
          "     Wimp Level: %-4s   " "    Color Level: %s\r\n", 				  

          GET_NAME(vict),

	  ONOFF(PRF_FLAGGED(vict, PRF_DISPHP)),
	  ONOFF(PRF_FLAGGED(vict, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(vict, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(vict, PRF_DISPMOVE)),
	  ONOFF(PRF_FLAGGED(vict, PRF_COMPACT)),
	  YESNO(PRF_FLAGGED(vict, PRF_QUEST)),

	  ONOFF(PRF_FLAGGED(vict, PRF_DISPMANA)),
	  ONOFF(PRF_FLAGGED(vict, PRF_AUTOEXIT)),
	  ONOFF(!PRF_FLAGGED(vict, PRF_NOGOSS)),

          ONOFF(PRF_FLAGGED(vict, PRF_DISPEXP)),
	  YESNO(!PRF_FLAGGED(vict, PRF_NOREPEAT)),
	  ONOFF(!PRF_FLAGGED(vict, PRF_NOAUCT)),

          ONOFF(PRF_FLAGGED(vict, PRF_DISPALIGN)),
	  YESNO(PRF_FLAGGED(vict, PRF_DEAF)),
	  ONOFF(!PRF_FLAGGED(vict, PRF_NOGRATZ)),

          ONOFF(EXT_FLAGGED(vict, EXT_AUTOLOOT)),
	  ONOFF(PRF_FLAGGED(vict, PRF_NOTELL)),
          ONOFF(!EXT_FLAGGED(vict, EXT_NONEWBIE)),
   
          ONOFF(EXT_FLAGGED(vict, EXT_AUTOGOLD)),
          YESNO(PRF_FLAGGED(vict, PRF_AFK)),
          ONOFF(!EXT_FLAGGED(vict, EXT_NOCTALK)),

          ONOFF(EXT_FLAGGED(vict, EXT_AUTOSPLIT)),
          YESNO(EXT_FLAGGED(vict, EXT_CLAN)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOINFO)),

	  buf2,
	  ctypes[COLOR_LEV(vict)]);

  send_to_char(buf, ch);

 	  
  if (GET_LEVEL(vict) >= LVL_ETRNL1) {
    sprintf(buf,
          "\r\n     Room Flags: %-3s    " " Wiznet Channel: %-3s\r\n" 
	  "      No Hassle: %-3s    " " Immnet Channel: %-3s\r\n" 
          "     Holy Light: %-3s\r\n", 

	  ONOFF(PRF_FLAGGED(vict, PRF_ROOMFLAGS)),
	  ONOFF(!PRF_FLAGGED(vict, PRF_NOWIZ)),
          ONOFF(PRF_FLAGGED(vict, PRF_NOHASSLE)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOIMMNET)),
	  ONOFF(PRF_FLAGGED(vict, PRF_HOLYLIGHT)));

    send_to_char(buf,ch);
  }
}

struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
  int a, b, tmp;

  ACMD(do_action);

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  /* create data array */
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

  /* initialize it */
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }

  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}



ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg,TRUE)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char("You can't see the commands of people above your level.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  sprintf(buf, "The following %s%s are available to %s:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
    i = cmd_sort_info[cmd_num].sort_pos;
    if (cmd_info[i].minimum_level >= 0 &&
	GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
	(cmd_info[i].minimum_level >= LVL_ETRNL1) == wizhelp &&
	(wizhelp || socials == cmd_sort_info[i].is_social)) {
      sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
      if (!(no % 7))
	strcat(buf, "\r\n");
      no++;
    }
  }

  strcat(buf, "\r\n");
  page_string(ch->desc, buf, TRUE);
}

#undef CLAN_TABLE_ALL
#define CLAN_TABLE_LEADER

/* DM - display the clan tables */
ACMD(do_clan_table)
{
  struct clan_data *clan;

  one_argument(argument,arg);

  if (IS_NPC(ch))
    return;

  if (GET_LEVEL(ch) >= LVL_ANGEL)
    if (!*arg) {
      send_to_char("Which clan?\r\n",ch);
      return;
    } else {
      if (!(clan=get_clan_by_name(arg))) {
        send_to_char("That is not a clan, type 'clans' to see the list.\r\n",ch);
        return;
      } else {
        display_clan_table(ch,clan);
        return;
      }
    }
    
  #ifdef CLAN_TABLE_ALL
    if (!(clan=get_clan_by_num(GET_CLAN_NUM(ch)))) {
      send_to_char("You are not a clan member.\r\n",ch);
      return;
    } else {
      display_clan_table(ch,clan);	
      return;
    }
  #endif

  #ifdef CLAN_TABLE_LEADER
    if ((GET_CLAN_NUM(ch) != 0) && (EXT_FLAGGED(ch, EXT_LEADER))) {
      if (!(clan=get_clan_by_num(GET_CLAN_NUM(ch)))) {
        sprintf(buf,"Clan Error: Clan Numb %d, char %s",
		GET_CLAN_NUM(ch),GET_NAME(ch));
        return;
      } else {
        display_clan_table(ch,clan);
        return;
      }
    } else {
      send_to_char("Sorry you cannot do that.\r\n",ch);
      return;
    }
  #endif

  send_to_char("Sorry you cannot do that.\r\n",ch);
  return;
}

/* DM - display the clans */
ACMD(do_clans)
{
  int i,j;

  sprintf(buf,"Current clans:\r\n");
  strcat (buf,"--------------\r\n");

  for (i=1; i < CLAN_NUM; i++) {
    strcat(buf, clan_info[i].disp_name);
    for (j=0; j < (MAX_LEN-strlen(clan_info[i].disp_name)); j++)
      strcat(buf," ");
    sprintf(buf2,"(%s)\r\n",clan_info[i].name);
    strcat(buf,buf2);
  }
  page_string(ch->desc, buf, TRUE);
}

void display_clan_table(struct char_data *ch, struct clan_data *clan)
{
  int i;

  if (!clan)
    return;

  sprintf(buf,"Members of the %s:\r\n",clan->disp_name);
  sprintf(buf2,"----------------");
  for (i=0; i < strlen(clan->disp_name); i++)
    strcat(buf2,"-");
 
  strcat(buf2,"\r\n");
  strcat(buf,buf2);
 
  sprintf(buf2,"%s",clan->table);
  strcat(buf,buf2);
 
  page_string(ch->desc,buf,TRUE);

}
