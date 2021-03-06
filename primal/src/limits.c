/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
   Modifications done by Brett Murphy to introduce character races
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "house.h"

#define BASE_SECT(n) ((n) & 0x000f)

extern struct char_data *character_list;
extern struct obj_data *object_list;
extern int level_exp[LVL_CHAMP + 1];

extern struct room_data *world;
extern int max_exp_gain;
extern int max_exp_loss;

extern struct index_data *obj_index;

/* void raw_kill(struct char_data * ch, struct char_data *killer); */

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (age < 15)
    return (p0);		/* < 15   */
  else if (age <= 29)
    return (int) (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (age <= 44)
    return (int) (p2 + (((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (age <= 59)
    return (int) (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (age <= 79)
    return (int) (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);		/* >= 80 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data * ch)
{
  int gain;

  if (IS_NPC(ch)) 
	{
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } 
	else 
	{
    /* gain = graf(age(ch).year, 4, 8, 12, 16, 12, 10, 8); */
    gain = graf(age(ch).year, 7, 8, 9, 10, 8, 7, 7);

    /* Class calculations */

    /* Skill/Spell calculations */

		/* WIS adjustment */
		if (GET_AFF_WIS(ch) > 17)
			gain += GET_AFF_WIS(ch) - 17;

    /* Position calculations    */
    switch (GET_POS(ch)) 
		{
    	case POS_SLEEPING:
    	  gain <<= 1;
    	  break;
    	case POS_RESTING:
    	  gain += (gain >> 1);	/* Divide by 2 */
    	  break;
    	case POS_SITTING:
    	  gain += (gain >> 2);	/* Divide by 4 */
    	  break;
			case POS_FIGHTING:
				gain = 1;
    }

   /*   gain <<= 1; */
  }


  /* JA make gain percentage based */
  gain = (gain * GET_MAX_MANA(ch)) / 100;
  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

	/* no one should have less than 10 gain */
	gain = MAX(gain, 10);

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;

  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
    gain = (gain * 3) / 2;

  // Artus> House buying incentive.
  if (IS_SET(world[ch->in_room].room_flags,ROOM_HOUSE))
  {
    int i;
    extern struct house_control_rec house_control[MAX_HOUSES];
    if ((i = find_house(world[ch->in_room].number) >= 0) &&
        (GET_IDNUM(ch) != house_control[i].owner))
      gain = (gain * 3) / 2;
  }

  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
    gain >>= 1;

  return (gain);
}


int hit_gain(struct char_data * ch)
/* Hitpoint gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    gain = GET_LEVEL(ch);
    /* Neat and fast */
  } else {

    /* gain = graf(age(ch).year, 8, 12, 20, 32, 16, 10, 4); */
    gain = graf(age(ch).year, 4, 5, 6, 7, 6, 5, 4);

    /* Class/Level calculations */

    /* Skill/Spell calculations */

		/* CON adjustment */
		if (GET_AFF_CON(ch) > 17)
			gain += GET_AFF_CON(ch) - 18;

    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
		case POS_FIGHTING:
			gain = 1;
    }


      /* JA make gain percent based */
  		gain = (gain * GET_MAX_HIT(ch)) / 100;
  }

	/* no one should have less than 10 gain */
	gain = MAX(gain, 10);

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;

  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
    gain = (gain * 3) / 2;

  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
    gain >>= 1;

  return (gain);
}



int move_gain(struct char_data * ch)
/* move gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    return (GET_LEVEL(ch));
    /* Neat and fast */
  } else {
    gain = graf(age(ch).year, 5, 7, 10, 15, 14, 12, 9);

    /* Class/Level calculations */

    /* Skill/Spell calculations */


    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
		case POS_FIGHTING:
			gain = 1;
    }
  }

  /* JA make gain percent based */
	gain = (gain * GET_MAX_MOVE(ch)) / 100;

	/* no one should have less than 10 gain */
	gain = MAX(gain, 10);

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;

  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
    gain = (gain * 3) / 2;

  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
    gain >>= 1;

  return (gain);
}



void set_title(struct char_data * ch, char *title)
{
  extern char *pc_class_types[]; 
  char temp[MAX_TITLE_LENGTH+1];
  strcpy(temp,title);
  if (title == NULL)
  {
    sprintf(temp,"the ");
    strcat(temp,pc_class_types[(int)GET_CLASS(ch)]);
 
  }
  
  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));
  GET_TITLE(ch) = str_dup(temp);
}


void check_autowiz(struct char_data * ch)
{
  char buf[100];
  extern int use_autowiz;
  extern int min_wizlist_lev;

  if (use_autowiz && GET_LEVEL(ch) >= LVL_IS_GOD) {
    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IS_GOD, IMMLIST_FILE, getpid());
    mudlog("Initiating autowiz.", CMP, LVL_IS_GOD, FALSE);
    system(buf);
  }
}


void gain_exp(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int newgain;
  char s[100];
  struct char_data *gainer=ch;

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_ISNOT_GOD)))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }

  if (gain > 0) {
    /* apply the overall cap REMOVED  */
    /* gain = MIN(max_exp_gain, gain);*/	/* put a cap on the max gain per kill */
    /* modified cap to be based on level */
    newgain = MIN(GET_LEVEL(gainer) * 10000, gain); 
    if (newgain < gain)
    {
      sprintf(s,"You receive less experience (%d) due to help.\n",newgain);
      send_to_char(s,gainer);
      gain=newgain;
    }
    GET_EXP(gainer) += gain;

   /* DM_exp - change for new exp system */
    GET_EXP(gainer)=MIN(GET_EXP(gainer),level_exp[LVL_CHAMP-1]*2);

    while (GET_LEVEL(gainer) < LVL_ISNOT_GOD &&
	GET_EXP(gainer) >= level_exp[GET_LEVEL(gainer)]) {
      send_to_char("You rise a level!\r\n", gainer);
      GET_LEVEL(gainer) += 1;
  /* DM_exp - reset the exp */
      GET_EXP(gainer) = GET_EXP(gainer)-level_exp[GET_LEVEL(gainer)-1];
      advance_level(gainer);
      is_altered = TRUE;
    }

    if (is_altered) {
      /*set_title(gainer, NULL);*/
      check_autowiz(gainer);
    }
  } else if (gain < 0) {
    /* CAP removed bm 3/95 */
    /* gain = MAX(-max_exp_loss, gain);*/	/* Cap max exp lost per death */
    GET_EXP(gainer) += gain;
    if (GET_EXP(gainer) < 0)
      GET_EXP(gainer) = 0; 
  }
}

/* DM_exp - changed to take the number of levels advanced */
void gain_exp_regardless(struct char_data * ch, int newlevel)
{
  int is_altered = FALSE;

  GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMPL &&
	GET_LEVEL(ch) < newlevel) {
      send_to_char("You rise a level!\r\n", ch);
      GET_LEVEL(ch) += 1;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      /*set_title(ch, NULL);*/
      check_autowiz(ch);
    }
  }
}


void gain_condition(struct char_data * ch, int condition, int value)
{
  bool intoxicated;
  int i;
  int check_cond;

/* Artus> Eternals and beyond dont get hungry */
  if ((GET_LEVEL(ch) >= LVL_ETRNL1) && (condition != DRUNK))
    return;

  if (GET_COND(ch, condition) == -1)	/* No change */
    return;

  switch (condition)
  {
    case FULL: check_cond = ITEM_NOHUNGER;break;
    case THIRST: check_cond=ITEM_NOTHIRST;break;
    case DRUNK: check_cond=ITEM_NODRUNK;break;
  }
  /* check for items with NOHUNGER, NOTHIRST, or NODRUNK */   
  for (i=0; i<NUM_WEARS;i++)
    if (ch->equipment[i])
      if (IS_OBJ_STAT(ch->equipment[i],check_cond))
      {
	if (GET_COND(ch, condition)==0)
	  GET_COND(ch, condition)=1; /* to make sure they dont keep getting warnings */  
        return; 
      }
  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case FULL:
    send_to_char("You are hungry.\r\n", ch);
    return;
  case THIRST:
    send_to_char("You are thirsty.\r\n", ch);
    return;
  case DRUNK:
    if (intoxicated)
      send_to_char("You are now sober.\r\n", ch);
    return;
  default:
    break;
  }

}

int debugit(int rnum,struct char_data *ch,int num, int vnum)
{
  char s[200];
  extern int num_items_room(struct obj_data *tobj);
  sprintf(s,"DEBUG: %d call to num_items_room, rnum=%d ch=%s\n",num,vnum,ch->player.name);
  log(s);
  return num_items_room(world[rnum].contents);
}

void check_idling(struct char_data * ch)
{
  struct obj_data *obj, *next_obj, *max;
  extern int free_rent;
  int i,j,rnum,hashouse;
  extern int num_of_houses;
  extern int num_items_room(struct obj_data *tobj);
  extern struct house_control_rec house_control[];
  void Crash_rentsave(struct char_data *ch, int cost);
  if (++(ch->char_specials.timer) > 8)
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
      save_char(ch, NOWHERE);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
/* bm increased from 1 to 2 hours */
    } else if (ch->char_specials.timer > 96) {
     /* bm handle house owners here */
      hashouse=0;
      for (i=0; i<num_of_houses; i++)
        if (house_control[i].mode==HOUSE_PRIVATE)
          if (GET_IDNUM(ch)==house_control[i].owner)
          {
            hashouse=1;
            break;
          }
/* bm removed , too complicated for players 
      if (!hashouse)
        for (i=0; i<num_of_houses; i++)
        {
          if (house_control[i].mode==HOUSE_PRIVATE)
            for (j=0;j<house_control[i].num_of_guests;j++)
              if (GET_IDNUM(ch)==house_control[i].guests[j])
              {
                hashouse=1;
                break;
              }
          if (hashouse)
            break;
        }
*/ 
      if (hashouse)
      {
        /* transfer character to their house */
        rnum =real_room( house_control[i].vnum);
        char_from_room(ch);
        char_to_room(ch,rnum);
        /* remove all eq */
        for (j=0; j< NUM_WEARS; j++)
          if (ch->equipment[j])
            obj_to_char(unequip_char(ch,j),ch);
        /* loop through all items, dropping those that are no_rent */
        for (obj=ch->carrying;obj;obj=next_obj)
        {
          next_obj=obj->next_content;
          if (IS_OBJ_STAT(obj, ITEM_NORENT))
          {
            if (debugit(rnum,ch,1,house_control[i].vnum) /*num_items_room(world[rnum].contents)*/>MAX_HOUSE_CONTENT)
              break;
            obj_from_char(obj);
            obj_to_room(obj,ch->in_room);
          }
        }
        /* now start with most expensive item and drop */
        while (ch->carrying && debugit(rnum,ch,2,house_control[i].vnum) /*num_items_room(world[rnum].contents)*/<=MAX_HOUSE_CONTENT)
        {
          next_obj=ch->carrying;
          max=next_obj;
          for (obj=next_obj;obj;obj=obj->next_content) 
            if (GET_OBJ_RENT(obj) > GET_OBJ_RENT(max))
              max=obj;
          obj_from_char(max);
          obj_to_room(max,ch->in_room);
        }
      }
      if (ch->in_room != NOWHERE)
        char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc)
	close_socket(ch->desc);
      ch->desc = NULL;
      if (free_rent)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);
      sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
      mudlog(buf, CMP, LVL_IS_GOD, TRUE);
      extract_char(ch);
    }
}



/* Update PCs, NPCs, and objects */
void point_update(void)
{
  void update_char_objects(struct char_data * ch);	/* handler.c */
  void extract_obj(struct obj_data * obj);	/* handler.c */
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2, *prev;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    if (GET_POS(i) >= POS_STUNNED) {
/* below ifs are so you can have more HIT/MANA/MOVE then MAX_HIT/MANA/MOVE
 * when wolf/vamp. this is so ya can have BIG bonuses instead of the max
 * of 128 that ya get with an affection cos the number is an sbyte - Vader
 */
      if(!(GET_HIT(i) > GET_MAX_HIT(i) && affected_by_spell(i,SPELL_CHANGED)))
        GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      if(!(GET_MANA(i) > GET_MAX_MANA(i) && affected_by_spell(i,SPELL_CHANGED)))
        GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      if(!(GET_MOVE(i) > GET_MAX_MOVE(i) && affected_by_spell(i,SPELL_CHANGED)))
        GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
      if (IS_AFFECTED(i, AFF_POISON))
	damage(i, i, 2, SPELL_POISON);
      if(!IS_NPC(i) && !IS_AFFECTED(i,AFF_WATERBREATHE) && ((BASE_SECT(world[i->in_room].sector_type) == SECT_UNDERWATER))){ 
	send_to_char("You take a deep breath of water. OUCH!\r\n",i);
	send_to_char("Your chest protests terrebly causing great pain.\r\n", i);
        act("$n suddenly turns a deep blue color holding $s throat.", TRUE, i, 0, 0 , TO_ROOM);
    GET_HIT(i)-= GET_LEVEL(i)*5;
    if (GET_HIT(i) <0){
        send_to_char("Your life flashes before your eyes.  You have Drowned. RIP!\r\n", i);
        act("$n suddenly turns a deep blue color holding $s throat.", TRUE, i, 0, 0 , TO_ROOM);
        act("$n has drowned. RIP!.", TRUE, i, 0, 0, TO_ROOM);
        raw_kill(i,NULL);
    }
}
      if (GET_POS(i) <= POS_STUNNED)
	update_pos(i);
    } else if (GET_POS(i) == POS_INCAP)
      damage(i, i, 1, TYPE_SUFFERING);
    else if (GET_POS(i) == POS_MORTALLYW)
      damage(i, i, 2, TYPE_SUFFERING);
    if (!IS_NPC(i)) {
      update_char_objects(i);
      if (GET_LEVEL(i) <= LVL_CHAMP) // Artus> Was LVL_IS_GOD
	check_idling(i);
    }
    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);
  }

  // DM - will this tell us anything in the core - prev??
  prev=object_list;
  /* objects */
  for (j = object_list; j; prev = j, j = next_thing) {
    next_thing = j->next;	/* Next in object list */

    /* If this is a corpse */
    if ((GET_OBJ_TYPE(j) == ITEM_CONTAINER) && GET_OBJ_VAL(j, 3)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

	if (j->carried_by)
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
	else if ((j->in_room != NOWHERE) && (world[j->in_room].people)) {
          if(GET_OBJ_VNUM(j) == 22300) { /* check if its a gate */
            act("The space/time continuum heals itself causing $p to disappear.",
                TRUE, world[j->in_room].people, j, 0, TO_ROOM);
            act("The space/time continuum heals itself causing $p to disappear.",
                TRUE, world[j->in_room].people, j, 0, TO_CHAR);
            } else {
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[j->in_room].people, j, 0, TO_CHAR);
            }
	}
	for (jj = j->contains; jj; jj = next_thing2) {
	  next_thing2 = jj->next_content;	/* Next in inventory */
	  obj_from_obj(jj);

/* JA corpses now take all their objs with them when they rot */
	  if (j->in_obj)
	    /* obj_to_obj(jj, j->in_obj); */
            extract_obj(jj);
	  else if (j->carried_by)
	    /* obj_to_room(jj, j->carried_by->in_room); */
            extract_obj(jj);
	  else if (j->in_room != NOWHERE)
	    /* obj_to_room(jj, j->in_room); */
            extract_obj(jj);
	  else
	    assert(FALSE);
	}
	extract_obj(j);
      }
    }
  }
}
