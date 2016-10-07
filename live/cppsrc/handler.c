/* ************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
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
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "clan.h"
#include "corpses.h"
#include "dg_scripts.h"

/* external vars */
extern struct char_data *combat_list;
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern const char *MENU;
extern CorpseData corpseData;

/* local functions */
int apply_ac(struct char_data * ch, int eq_pos, struct obj_data *obj);
void update_object(struct obj_data * obj, int use);
void update_char_objects(struct char_data * ch);

/* external functions */
int invalid_class(struct char_data *ch, struct obj_data *obj);
void remove_follower(struct char_data * ch);
void clearMemory(struct char_data * ch);
//void die_ffrench		fr_FR.ISO-88data * ch);
void charm_room(struct char_data *ch);
ACMD(do_return);

char *fname(const char *namelist)
{
  static char holder[30];
  register char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

#define WHITESPACE " \t" 

int isname(const char *str, const char *namelist)
{
  char *newlist;
  char *curtok;
 
  newlist = strdup(namelist); /* make a copy since strtok 'modifies' strings */
 
  for (curtok=strtok(newlist, WHITESPACE); curtok; curtok=strtok(NULL, WHITESPACE))
    if (curtok && is_abbrev(str, curtok)) {
      free(newlist);
      return 1;
    }
  free(newlist);
  return 0; 
}

/*
int isname(const char *str, const char *namelist)
  const char *curname, *curstr;

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if (!*curstr && !isalpha(*curname))
	return (1);

      if (!*curname)
	return (0);

      if (!*curstr || *curname == ' ')
	break;

      if (LOWER(*curstr) != LOWER(*curname))
	break;
    }

    // skip to next name 

    for (; isalpha(*curname); curname++);
    if (!*curname)
      return (0);
    curname++;			// first char of new name 
  }
}
*/



void affect_modify(struct char_data * ch, byte loc, sbyte mod, 
                   bitvector_t bitv, bool add)
{
  int maxabil; 

  if (add)
  {
    SET_BIT(AFF_FLAGS(ch), bitv);
  } else {
    REMOVE_BIT(AFF_FLAGS(ch), bitv);
    mod = -mod;
  }

// Base the affected stats from the MAX_STAT_VALUE(ch) <- ie. classes on remort
// level 2 have different max stats to those on remort level 1
  //maxabil = (IS_NPC(ch) ? MAX_STAT_VALUE-1 : MAX_STAT_VALUE(ch));
  maxabil = MAX_STAT_VALUE(ch);

  switch (loc)
  {
    case APPLY_NONE: break;
    case APPLY_STR:
      if (mod>0)
      {
       if (GET_REAL_STR(ch) < maxabil)
       {
         if (GET_AFF_STR(ch) + mod > maxabil)
           GET_AFF_STR(ch) = maxabil;
         else
           GET_AFF_STR(ch)+=mod;
       }
      }
      else
        GET_AFF_STR(ch) += mod;
      break;
    case APPLY_DEX:
      if (mod>0)
      {
	if (GET_REAL_DEX(ch) < maxabil)
	{
	  if (GET_AFF_DEX(ch) + mod > maxabil)
	    GET_AFF_DEX(ch) = maxabil;
	  else
	    GET_AFF_DEX(ch)+=mod;
	}
      } else
	GET_AFF_DEX(ch)+=mod;
      break;
    case APPLY_INT:
      if (mod>0)
      {
	if (GET_AFF_INT(ch) + mod <= MAX(maxabil, GET_REAL_INT(ch)))
	  GET_AFF_INT(ch) += mod;
	else
	  GET_AFF_INT(ch) = MAX(maxabil, GET_REAL_INT(ch));
      } else
	GET_AFF_INT(ch) += mod;
      break;
    case APPLY_WIS:
      if (mod>0)
      {
	if (GET_REAL_WIS(ch) < maxabil)
	{
	  if (GET_AFF_WIS(ch) + mod > maxabil)
	    GET_AFF_WIS(ch)=maxabil;
	  else
	    GET_AFF_WIS(ch)+=mod;
	}
      } else
	GET_AFF_WIS(ch) += mod;
      break;
    case APPLY_CON:
      if (mod>0)
      {
	if (GET_REAL_CON(ch) < maxabil)
	{
	  if (GET_AFF_CON(ch) + mod > maxabil)
	    GET_AFF_CON(ch) = maxabil;
	  else
	    GET_AFF_CON(ch)+=mod;
	}
      } else
	GET_AFF_CON(ch) += mod;
      break;
    case APPLY_CHA:
      if (mod>0)
      {
	if (GET_REAL_CHA(ch) < maxabil)
	{
	  if (GET_AFF_CHA(ch) + mod > maxabil)
	    GET_AFF_CHA(ch) = maxabil;
	  else
	    GET_AFF_CHA(ch)+=mod;
	}
      } else
	GET_AFF_CHA(ch) += mod;
      break;
    case APPLY_CLASS: break;
    case APPLY_LEVEL: break;
    case APPLY_AGE:
      ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
      break;
    case APPLY_CHAR_WEIGHT:
      GET_WEIGHT(ch) += mod;
      break;
    case APPLY_CHAR_HEIGHT:
      GET_HEIGHT(ch) += mod;
      break;
    case APPLY_MANA:
      GET_MAX_MANA(ch) = MAX(1, GET_MAX_MANA(ch) + mod);
      break;
    case APPLY_HIT:
      GET_MAX_HIT(ch) = MAX(1, GET_MAX_HIT(ch) + mod);
      break;
    case APPLY_MOVE:
      GET_MAX_MOVE(ch) = MAX(1, GET_MAX_MOVE(ch) + mod);
      break;
    case APPLY_GOLD: break;
    case APPLY_EXP: break;
    case APPLY_AC:
      GET_AC(ch) += mod;
      break;
    case APPLY_HITROLL:
      GET_HITROLL(ch) += mod;
      break;
    case APPLY_DAMROLL:
      GET_DAMROLL(ch) += mod;
      break;
    case APPLY_SAVING_PARA:
      GET_SAVE(ch, SAVING_PARA) += mod;
      break;
    case APPLY_SAVING_ROD:
      GET_SAVE(ch, SAVING_ROD) += mod;
      break;
    case APPLY_SAVING_PETRI:
      GET_SAVE(ch, SAVING_PETRI) += mod;
      break;
    case APPLY_SAVING_BREATH:
      GET_SAVE(ch, SAVING_BREATH) += mod;
      break;
    case APPLY_SAVING_SPELL:
      GET_SAVE(ch, SAVING_SPELL) += mod;
      break;
    default:
      basic_mud_log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
      break;
  } /* switch */
}

/******************************************************************************
 * Affect functions
 *****************************************************************************/

/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(struct char_data * ch)
{
  struct affected_type *af;
  int i, j;

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && 
	(eq_pos_ok(ch, i) || OBJ_FLAGGED(GET_EQ(ch, i), ITEM_QEQ)))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
		      GET_EQ(ch, i)->affected[j].modifier,
		      GET_EQ(ch, i)->obj_flags.bitvector, FALSE);

  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);

  ch->aff_abils = ch->real_abils;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, i) &&
	(eq_pos_ok(ch, i) || OBJ_FLAGGED(GET_EQ(ch, i), ITEM_QEQ)))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
		      GET_EQ(ch, i)->affected[j].modifier,
		      GET_EQ(ch, i)->obj_flags.bitvector, TRUE);
  }

  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

  /* Make certain values are between 0..25(i), not < 0 and not > 25(i)! */

  i = MAX_STAT_VALUE(ch);

  GET_AFF_DEX(ch) = MAX(0, MIN(GET_AFF_DEX(ch), i));
  GET_AFF_INT(ch) = MAX(0, MIN(GET_AFF_INT(ch), i));
  GET_AFF_WIS(ch) = MAX(0, MIN(GET_AFF_WIS(ch), i));
  GET_AFF_CON(ch) = MAX(0, MIN(GET_AFF_CON(ch), i));
  GET_AFF_STR(ch) = MAX(0, GET_AFF_STR(ch));

  if (IS_NPC(ch))
    GET_AFF_STR(ch) = MIN(GET_AFF_STR(ch), i);
  // else {
/*    if (GET_AFF_STR(ch) > 18) {
      i = GET_AFF_ADD(ch) + ((GET_AFF_STR(ch) - 18) * 10);
      GET_AFF_ADD(ch) = MIN(i, 100);
    }
  } */
}

bool char_has_eq_abil(struct char_data *ch, struct affected_type *af, int type)
{
  struct affected_type *cur, *next;
  for (cur = ch->affected; cur; cur = next)
  {
    next = cur->next;
    if (cur->duration == type)
    {
      if ((af->type == cur->type) && (af->location == cur->location) &&
	  (af->bitvector == cur->bitvector))
	return (TRUE);
    }
  }
  return (FALSE);
}

bool char_has_eq_affect(struct char_data *ch, struct affected_type *af)
{
  return (char_has_eq_abil(ch, af, CLASS_ITEM));
}

bool char_has_ability(struct char_data *ch, struct affected_type *af)
{
  return (char_has_eq_abil(ch, af, CLASS_ABILITY));
}

/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data * ch, struct affected_type * af)
{
  struct affected_type *affected_alloc;

  // Now we have abilities, we want to check if they already have that type 
  // affect available thus. 
  if (char_has_ability(ch, af))
    return;

  // Artus> Check for EQ affects.
  if ((af->duration != CLASS_ABILITY) && (char_has_eq_affect(ch, af)))
    return;

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
  affect_total(ch);
}

void apply_eq_affects(struct char_data *ch)
{
  void obj_wear_spells(struct char_data *ch, struct obj_data *obj);
  struct affected_type *cur, *next;
  unsigned int i;

  for (cur = ch->affected; cur; cur = next)
  {
    next = cur->next;
    if (cur->duration != CLASS_ITEM)
      continue;
    affect_remove(ch, cur, 0);
  }

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, i) &&
	(eq_pos_ok(ch, i) || OBJ_FLAGGED(GET_EQ(ch, i), ITEM_QEQ)))
      obj_wear_spells(ch, GET_EQ(ch, i));
  }
  return;
}

/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(struct char_data * ch, struct affected_type * af, int trig)
{
  struct affected_type *temp;

  if (ch->affected == NULL) 
  {
    core_dump();
    return;
  }

  // Artus> Events trigggered when affect is removed.
  if (trig)
    switch (af->type)
    {
      case SPELL_CHANGED: // Reset Hit/Mana/Move values on lose wolf/vampire.
	if (IS_NPC(ch) || PRF_FLAGGED(ch, PRF_WOLF))
	{
	  if (GET_HIT(ch) > GET_MAX_HIT(ch))
	    GET_HIT(ch) = GET_MAX_HIT(ch);
	}
	else if (IS_NPC(ch) || 
		 (PRF_FLAGGED(ch, PRF_VAMPIRE) && 
		  !char_affected_by_timer(ch, TIMER_DARKRITUAL)))
	{
	  if (GET_MANA(ch) > GET_MAX_MANA(ch))
	    GET_MANA(ch) = GET_MAX_MANA(ch);
	}
	if (GET_MOVE(ch) > GET_MAX_MOVE(ch))
	  GET_MOVE(ch) = GET_MAX_MOVE(ch);
	break;
      case SPELL_SLEEP: // Wake sleeping NPCs.
	if (IS_NPC(ch))
	  GET_POS(ch) = POS_STANDING;
	act("$n wakes up and climbs to $s feet.", FALSE, ch, NULL, 0, TO_ROOM);
	break;
    }

  affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
  REMOVE_FROM_LIST(af, ch->affected, next);
  free(af);
  affect_total(ch);
}

/* Call affect_remove with every spell of spelltype "skill" */
// Modified to prevent ability removal (use other func for that)
void affect_from_char(struct char_data * ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) 
  {
    next = hjp->next;
    if (hjp->type == type && hjp->duration != CLASS_ABILITY &&
	hjp->duration != CLASS_ITEM)
       affect_remove(ch, hjp);
  }
}

// As above for item affects.
void eq_affect_from_char(struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;
  for (hjp = ch->affected; hjp; hjp = next)
  {
    next = hjp = hjp->next;
    if (hjp->type == type && hjp->duration == CLASS_ITEM)
      affect_remove(ch, hjp);
  }
}

// As above for abilities.
void ability_from_char(struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type && hjp->duration == CLASS_ABILITY)
       affect_remove(ch, hjp);
  }
}

/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(struct char_data * ch, int type)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == type)
      return (TRUE);

  return (FALSE);
}

void affect_join(struct char_data * ch, struct affected_type * af,
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = next) {
    next = hjp->next;

    if ((hjp->type == af->type) && (hjp->location == af->location)) {
      // Artus> Don't fuck with abilities or items.
      if ((hjp->duration == CLASS_ABILITY) || (hjp->duration == CLASS_ITEM))
	return;
      if (add_dur)
	af->duration += hjp->duration;
      if (avg_dur)
	af->duration /= 2;

      if (add_mod)
	af->modifier += hjp->modifier;
      if (avg_mod)
	af->modifier /= 2;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}


/******************************************************************************
 * Timer functions
 *****************************************************************************/

// All the defines for the timer types be given in this function
struct timer_type *timer_new(int timertype) {
  struct timer_type *newtimer;

  CREATE(newtimer, struct timer_type, 1);

  newtimer->type = timertype;
  newtimer->uses = 0;
  newtimer->next = NULL;

  switch(timertype) {

    case TIMER_HEALING_SKILLS:
      newtimer->max_uses = 5;
      newtimer->duration = 3;
      break;

    case TIMER_DARKRITUAL:
      newtimer->max_uses = 1;
      newtimer->duration = 2;
      break;

    case TIMER_POISONBLADE:
      newtimer->max_uses = -1;
      newtimer->duration = 2;
      break;

    case TIMER_BERSERK:
      newtimer->max_uses = 1;
      newtimer->duration = 3;
      break;

    case TIMER_TRAP_PIT:
      newtimer->max_uses = 1;
      newtimer->duration = 3;
      break;

    case TIMER_MEDITATE:
      newtimer->max_uses = -1;
      newtimer->duration = 5;
      break;

    case TIMER_HEAL_TRANCE:
      newtimer->max_uses = -1;
      newtimer->duration = 2;
      break;

    case TIMER_SHIELD:
      newtimer->max_uses = -1;
      newtimer->duration = -1;
      break;

    default:
      basic_mud_log("SYSERR: Timer type given to timer_new() not found.");
      free(newtimer);
      return NULL; 
  }
  return (newtimer);
}

/* Insert an affect_type in a char_data structure */
void timer_to_char(struct char_data * ch, struct timer_type *timer)
{
  struct timer_type *timer_alloc;
           
  CREATE(timer_alloc, struct timer_type, 1);

  *timer_alloc = *timer;
  timer_alloc->next = CHAR_TIMERS(ch);
  CHAR_TIMERS(ch) = timer_alloc;
}        

/* Insert an affect_type in a char_data structure */
void timer_to_obj(struct obj_data * obj, struct timer_type *timer)
{
  struct timer_type *timer_alloc;
           
  CREATE(timer_alloc, struct timer_type, 1);

  *timer_alloc = *timer;
  timer_alloc->next = OBJ_TIMERS(obj);
  OBJ_TIMERS(obj) = timer_alloc;
}        

/* Call timer_remove_obj with every timer of type "type" */
void timer_from_obj(struct obj_data * obj, int type) {
  struct timer_type *hjp, *next;
           
  for (hjp = obj->timers; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      timer_remove_obj(obj, hjp);
  }
}    

/* Call timer_remove_char with every timer of type "type" */
void timer_from_char(struct char_data * ch, int type)
{
  struct timer_type *hjp, *next;
           
  for (hjp = CHAR_TIMERS(ch); hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      timer_remove_char(ch, hjp);
  }
}    

/*
 * Remove a timer_type structure from an object (called when duration
 * reaches zero). Pointer *timer must never be NIL!  Frees memory. 
 */
void timer_remove_obj(struct obj_data *obj, struct timer_type *timer)
{
  struct timer_type *temp;
           
  if (obj->timers == NULL)
  {
    core_dump();
    return;
  }
   
  REMOVE_FROM_LIST(timer, obj->timers, next);
  free(timer);
}  

/*
 * Remove a timer_type structure from a char (called when duration
 * reaches zero). Pointer *timer must never be NIL!  Frees memory. 
 */
void timer_remove_char(struct char_data * ch, struct timer_type *timer)
{
  struct timer_type *temp;
           
  if (!(ch) || CHAR_TIMERS(ch) == NULL) 
  {
    core_dump();
    return;
  }

  if (timer->duration == 0)
    switch (timer->type)
    {
      case TIMER_TRAP_PIT:
	send_to_char("&rYou may now use your trap building skills again.&n\r\n", ch);
	break;
      case TIMER_HEALING_SKILLS:
	send_to_char("&rYou may now use your healing skills again.&n\r\n", ch);
	break;
      case TIMER_DARKRITUAL:
	send_to_char("&rYou may now use dark ritual again.&n\r\n", ch);
	break;
      case TIMER_MEDITATE:
	send_to_char("&rYou stop meditating.&n\r\n", ch);
	break;
      case TIMER_HEAL_TRANCE:
	send_to_char("&rYou wake from your healing trance.&n\r\n", ch);
	break;
    }
  REMOVE_FROM_LIST(timer, CHAR_TIMERS(ch), next);
  free(timer);
}  

/*
 * Searches through the timers for the type "type", increments uses,
 * and returns 1 if the uses does not exceed max_uses, otherwise it
 * returns 0. If the timer_type is not found, -1 is returned
 */ 
int timer_use_obj(struct obj_data *obj, int type) {
  struct timer_type *hjp;
 
  for (hjp = OBJ_TIMERS(obj); hjp; hjp = hjp->next)
    if (hjp->type == type) {

      // may as well keep track of how many times we have tried to use it
      hjp->uses++;

      if (hjp->uses <= hjp->max_uses)
        return 1;
      else
        return FALSE;
    }

  return -1;
}

/*
 * Searches through the timers for the type "type", increments uses,
 * and returns 1 if the uses does not exceed max_uses, otherwise it
 * returns 0. If the timer_type is not found, -1 is returned
 */ 
int timer_use_char(struct char_data *ch, int type) {
  struct timer_type *hjp;
 
  for (hjp = CHAR_TIMERS(ch); hjp; hjp = hjp->next)
    if (hjp->type == type) {

      // may as well keep track of how many times we have tried to use it
      hjp->uses++;

      if (hjp->uses <= hjp->max_uses)
        return 1;
      else if (LR_FAIL(ch, LVL_IS_GOD))
        return 0;
      else
	return 1;
    }

  return -1;
}

bool obj_affected_by_timer(struct obj_data *obj, int type) {
  struct timer_type *hjp;
 
  for (hjp = OBJ_TIMERS(obj); hjp; hjp = hjp->next)
    if (hjp->type == type)
      return (TRUE);
  
  return (FALSE);
}

struct timer_type *char_affected_by_timer(struct char_data *ch, int type)
{
  struct timer_type *hjp;
 
  for (hjp = CHAR_TIMERS(ch); hjp; hjp = hjp->next)
    if (hjp->type == type)
      return (hjp);
  
  return NULL;
}

/******************************************************************************
 * Enod of Timer functions
 *****************************************************************************/


/* move a player out of a room */
void char_from_room(struct char_data * ch)
{
  struct char_data *temp;
	
  if (ch == NULL || ch->in_room == NOWHERE)
  {
    basic_mud_log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    exit(1);
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light is ON */
	world[ch->in_room].light--;

  REMOVE_FROM_LIST(ch, world[ch->in_room].people, next_in_room);
  
  ch->last_room = ch->in_room;
  ch->in_room = NOWHERE;
  ch->next_in_room = NULL;

  if (MOUNTING(ch) && MOUNTING(ch)->in_room != NOWHERE)
	char_from_room(MOUNTING(ch));
  if (MOUNTING_OBJ(ch) && 
      ((find_obj_list(ch, MOUNTING_OBJ(ch)->name, ch->carrying) != NULL) ||
       (find_obj_eqpos(ch, MOUNTING_OBJ(ch)->name) >= 0)) &&
      (MOUNTING_OBJ(ch)->in_room != NOWHERE))
    obj_from_room(MOUNTING_OBJ(ch));
}


/* place a character in a room */
void char_to_room(struct char_data * ch, room_rnum room)
{
  if (ch == NULL || room < 0 || room > top_of_world) {
    basic_mud_log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
		room, top_of_world, ch);
    // DM 
    if (ch == NULL)
      return;
  } else {
    // Artus> Sanity.
    if ((world[room].people != NULL) && (world[room].people->in_room != room))
    {
      sprintf(buf, "SYSERR: Room's people not found! (Room: %d/%d)", room,
	      world[room].number);
      mudlog(buf, NRM, LVL_IMPL, TRUE);
      world[room].people = NULL;
      for (struct char_data *k = character_list; k; k = k->next)
	if (k->in_room == room)
	{
	  sprintf(buf, "%s was supposed to be in there.", GET_NAME(k));
	  mudlog(buf, NRM, LVL_IMPL, TRUE);
	  k->in_room = NOWHERE;
	  char_to_room(k, room);
	}
    }
    ch->next_in_room = world[room].people;
    world[room].people = ch;
    ch->in_room = room;

    if (GET_EQ(ch, WEAR_LIGHT))
      if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light ON */
	  world[room].light++;

    /* Stop fighting now, if we left. */
    if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }
  }
  // Do the mount
  if (MOUNTING(ch) && MOUNTING(ch)->in_room == NOWHERE)
	char_to_room(MOUNTING(ch), room);

  // Item mount and not carried -- Nup, this is bullshit. Items must be carried/worn. 
  // I may reconsider this once we have special items like harleys. - Tal.
//  if (MOUNTING_OBJ(ch) && 
//	! ( get_obj_in_list_vis(ch, MOUNTING_OBJ(ch)->name, ch->carrying)
//	 || get_object_in_equip_vis(ch, MOUNTING_OBJ(ch)->name, ch->equipment, &tmp))
//	&& (MOUNTING_OBJ(ch)->in_room == NOWHERE))
// obj_to_room(MOUNTING_OBJ(ch), ch->in_room);

  if (!IS_NPC(ch) )
     if (IS_SET(GET_SPECIALS(ch), SPECIAL_CHARMER))
	charm_room(ch);

}


/* give an object to a char   */
void obj_to_char(struct obj_data * object, struct char_data * ch, char *file, int line)
{
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    object->in_room = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    // Set the room number of corpse to NOWHERE
    if (GET_CORPSEID(object) > 0) {
      corpseData.addCorpse(object, NOWHERE, 0);
    }

    /* set flag for crash-save system, but not on mobs! */
    if (!IS_NPC(ch))
      SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
  } else {
    basic_mud_log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
    basic_mud_log("        From file: %s, line %d", file, line);
  }
}


/* take an object from a char */
void obj_from_char(struct obj_data * object)
{
  struct obj_data *temp;

  if (object == NULL) {
    basic_mud_log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }

  if (object->carried_by == NULL) {
    basic_mud_log("SYSERR: NULL object carrier passed to obj_from_char.");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
    SET_BIT(PLR_FLAGS(object->carried_by), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
}



/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(struct char_data * ch, int eq_pos, struct obj_data *obj, 
    bool manual)
{
  int factor;

  if (GET_EQ(ch, eq_pos) == NULL)
  {
    core_dump();
    return (0);
  }

  if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
    return (0);

  if (!(eq_pos_ok(ch, eq_pos) || OBJ_FLAGGED(obj, ITEM_QEQ)))
    return (0);

  switch (eq_pos)
  {
    case WEAR_BODY:
      factor = 3;
      break;			/* 30% */
    case WEAR_HEAD:
      factor = 2;
      break;			/* 20% */
    case WEAR_LEGS:
      factor = 2;
      break;			/* 20% */
    case WEAR_SHIELD:
      // Removing shield ...
      if (obj->worn_on == -1)
      {
	// if obj was affected with TIMER_SHIELD the ch previous used shield 
	// mastery to get a factor bonus of 2 
	if (timer_use_obj(GET_EQ(ch, WEAR_SHIELD), TIMER_SHIELD) != -1)
	{
	  timer_from_obj(GET_EQ(ch, WEAR_SHIELD), TIMER_SHIELD);
	  factor = 2;
#ifndef IGNORE_DEBUG
	  if (GET_DEBUG(ch) && manual)
	  {
	    sprintf(buf, "removing shield previously applied with sucessful shield mastery, ac = %d, factor = 2\r\n", 
		GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
	    send_to_char(buf, ch);
	  }
#endif
	  break;
	}
	factor = 1;
#ifndef IGNORE_DEBUG
	if (GET_DEBUG(ch) && manual)
	{
	  sprintf(buf, "removing shield not previously applied with sucessful shield mastery, ac = %d, factor = 1\r\n", 
		GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
	  send_to_char(buf, ch);
	}
#endif
      // Adding Shield
      } else {
	// DM: Shield mastery
	if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_SHIELDMASTERY) && 
	    has_stats_for_skill(ch, SKILL_SHIELDMASTERY, FALSE))
	{
	  if (number(0, 101) < GET_SKILL(ch, SKILL_SHIELDMASTERY))
	  {
	    // Add timer 
	    if (timer_use_obj(obj, TIMER_SHIELD) == -1)
	    {
	      timer_to_obj(obj, timer_new(TIMER_SHIELD));  
	      timer_use_obj(obj, TIMER_SHIELD);
	    }
	    // Only show message if it was manually worn
	    // ie. save_char calls equip_char which calls this - hence we
	    // get messages when we dont want to - I guess really shield mastery
	    // should be elsewhere - but hey dodgy and will always apply as is.
	    if (manual)
	    {
	      send_to_char("You apply your shield mastery.\r\n", ch);
	      apply_spell_skill_abil(ch, SKILL_SHIELDMASTERY);
	    }
#ifndef IGNORE_DEBUG
	    if (GET_DEBUG(ch))
	    {
	      sprintf(buf, "adding shield (with mastery),  ac = %d, factor = 2\r\n", 
		     GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
	      send_to_char(buf, ch);
	    }
#endif
	    factor = 2;
	    //factor = MAX(1, (int)(5 * (GET_SKILL(ch, SKILL_SHIELDMASTERY) / 100)
	    //	* (SPELL_EFFEC(ch, SKILL_SHIELDMASTERY) / 100)));
	    //factor = MIN(1, (int)(5 * (GET_SKILL(ch, SKILL_SHIELDMASTERY) / 100)
	    //	* (SPELL_EFFEC(ch, SKILL_SHIELDMASTERY) / 100)));
	    break;
	  }
	}
	factor = 1;
      }
      break;
    default:
      factor = 1;
      break;			/* all others 10% */
  }
  return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int invalid_align(struct char_data *ch, struct obj_data *obj)
{
  if (IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
    return TRUE;
  if (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
    return TRUE;
  if (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
    return TRUE;
  return FALSE;
}

bool equip_char(struct char_data * ch, struct obj_data * obj, int pos, 
    bool manual)
{
  int i, j;
#ifdef WANT_QL_ENHANCE
  int k, l;
#endif
  extern struct obj_data *obj_proto;

  if (pos < 0 || pos >= NUM_WEARS)
  {
    core_dump();
    return (FALSE);
  }

  if (GET_EQ(ch, pos))
  {
    basic_mud_log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	    obj->short_description);
    return (FALSE);
  }
  if (obj->carried_by)
  {
    basic_mud_log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return (FALSE);
  }
  if (obj->in_room != NOWHERE)
  {
    basic_mud_log("SYSERR: EQUIP: Obj is in_room when equip.");
    return (FALSE);
  }
  if (invalid_align(ch, obj) || invalid_class(ch, obj))
  {
    act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj, 0, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(obj, ch, __FILE__, __LINE__);
    return (FALSE);
  }

  // DM - apply the quest item enhancements 
  // In case anything goes wrong with the affects on this instance of
  // the quest item, we restore the "prototype" of the item and then
  // apply the enhancements. (And fills in the unfinished questlog code)...
  //
  // This is essentially the same as enhance_quest_item() except rather than
  // just adding the modifier to the item, the prototype defaults are restored 
  // first and then the enhancements are applied.
  if (OBJ_FLAGGED(obj, ITEM_QUEST))
  {
    // Find the quest item in the  players list
    for (i = 0; i < MAX_QUEST_ITEMS; i++)
    {
      if (GET_OBJ_VNUM(obj) == GET_QUEST_ITEM(ch, i))
      {
	obj_rnum ornum = real_object(GET_OBJ_VNUM(obj));

	// copy the prototype affects
	for (int t = 0; t < MAX_OBJ_AFFECT; t++)
	{
          obj->affected[t].location = obj_proto[ornum].affected[t].location;
	  obj->affected[t].modifier = obj_proto[ornum].affected[t].modifier;
	}
#ifdef WANT_QL_ENHANCE	
	for (j = 0; j < MAX_NUM_ENHANCEMENTS; j++)
	{
          // If enhancement j for item i = 0
	  if (GET_QUEST_ENHANCEMENT(ch, i, j) == 0)
            continue;	
	  // For every actual value (k) on enhancement j
          for (k = 0; k < MAX_ENHANCEMENT_VALUES; k++)
	  {
            int found = 0;
	    if (GET_QUEST_ENHANCEMENT_VALUE(ch, i, j, k) == 0)
	      continue;
  	    // for every affect on the object
            for (l = 0; l < MAX_OBJ_AFFECT; l++)
	    {
              if (obj->affected[l].location == GET_QUEST_ENHANCEMENT(ch, i, j))
	      {
                obj->affected[l].modifier += GET_QUEST_ENHANCEMENT_VALUE(ch,i,j, k);
	        found = 1;
	        break;
	      }
	    }
	    if (!found)
	    { // New affect
              for (l = 0 ; l < MAX_OBJ_AFFECT; l++)
	      {
                if (obj->affected[l].location == APPLY_NONE)
		{
	          obj->affected[l].modifier = GET_QUEST_ENHANCEMENT_VALUE(ch, i, j, k);
	          obj->affected[l].location = GET_QUEST_ENHANCEMENT(ch, i, j);
	          break;
                }
              }
            } 
          } 
        }
#endif
      } 
    }
  }
  

  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) -= apply_ac(ch, pos, obj, manual);

  if (ch->in_room != NOWHERE)
  {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
	world[ch->in_room].light++;
  } else
    basic_mud_log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));

  if (eq_pos_ok(ch, pos) || OBJ_FLAGGED(obj, ITEM_QEQ))
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch, obj->affected[j].location,
		    obj->affected[j].modifier,
		    obj->obj_flags.bitvector, TRUE);
  affect_total(ch);
  return (TRUE);
}



struct obj_data *unequip_char(struct char_data * ch, int pos, bool manual)
{
  int j;
  struct obj_data *obj;
  bool apply = false;

  if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL)
  {
    core_dump();
    return (NULL);
  }

  obj = GET_EQ(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (eq_pos_ok(ch, pos) || OBJ_FLAGGED(obj, ITEM_QEQ))
    apply = true;
  
  if (apply && (GET_OBJ_TYPE(obj) == ITEM_ARMOR))
    GET_AC(ch) += apply_ac(ch, pos, obj, manual);

  if (ch->in_room != NOWHERE)
  {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
	world[ch->in_room].light--;
  } else
    basic_mud_log("SYSERR: ch->in_room = NOWHERE when unequipping char %s.", GET_NAME(ch));

  GET_EQ(ch, pos) = NULL;

  if (apply)
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch, obj->affected[j].location,
		    obj->affected[j].modifier,
		    obj->obj_flags.bitvector, FALSE);
  affect_total(ch);
  return (obj);
}


int get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.')) != NULL) {
    *ppos++ = '\0';
    strcpy(number, *name);
    strcpy(*name, ppos);

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
	return (0);

    return (atoi(number));
  }
  return (1);
}



/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data * list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return (i);

  return (NULL);
}



/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr)
{
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return (i);

  return (NULL);
}



/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, room_rnum room)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return (NULL);

  for (i = world[room].people; i && (j <= number); i = i->next_in_room)
    if (isname(tmp, i->player.name))
      if (++j == number)
	return (i);

  return (NULL);
}



/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return (i);

  return (NULL);
}



/* put an object in a room */
void obj_to_room(struct obj_data * object, room_rnum room)
{
  if (!object || room < 0 || room > top_of_world)
    basic_mud_log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
	room, top_of_world, object);
  else {
    object->next_content = world[room].contents;
    world[room].contents = object;
    object->in_room = room;
    object->carried_by = NULL;
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);

    // Corpse being transfered to room - update the room number
    if (GET_CORPSEID(object) > 0) {
      corpseData.addCorpse(object, GET_ROOM_VNUM(room), 0); 
    }
  }
}


/* Take an object from a room */
void obj_from_room(struct obj_data * object)
{
  struct obj_data *temp;

  if (!object || object->in_room == NOWHERE) {
    basic_mud_log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
	object, object->in_room);
    return;
  }

  REMOVE_FROM_LIST(object, world[object->in_room].contents, next_content);

  if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
    SET_BIT(ROOM_FLAGS(object->in_room), ROOM_HOUSE_CRASH);
  object->in_room = NOWHERE;
  object->next_content = NULL;
}


/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data * obj, struct obj_data * obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to)
  {
    basic_mud_log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
	obj, obj, obj_to);
    return;
  }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;

  for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

  /* top level object.  Subtract weight from inventory if necessary. */
  GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by)
    IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
}


/* remove an object from an object */
void obj_from_obj(struct obj_data * obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    basic_mud_log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container */
  for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

  /* Subtract weight from char that carries the object */
  GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by)
    IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);

  obj->in_obj = NULL;
  obj->next_content = NULL;
}


/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data * list, struct char_data * ch)
{
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}


/* Extract an object from the world */
void extract_obj(struct obj_data * obj)
{
  struct obj_data *temp;

  if (OBJ_RIDDEN(obj))
  {
    send_to_char("Your mount disappears!\r\n", OBJ_RIDDEN(obj));
    act("$n's mount disappears!\r\n", FALSE, OBJ_RIDDEN(obj), 0, 0, TO_ROOM);
    MOUNTING_OBJ(OBJ_RIDDEN(obj)) = NULL;
  }

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on, FALSE) != obj)
      basic_mud_log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj->in_room != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
  {
    MOUNTING_OBJ(obj->carried_by) = NULL;
    obj_from_char(obj);
  }
  else if (obj->in_obj)
    obj_from_obj(obj);
//  log ("3");


  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
    extract_obj(obj->contains);

  REMOVE_FROM_LIST(obj, object_list, next);
//  log ("4");


  // Artus> Lets not have the count go negative!
  if ((GET_OBJ_RNUM(obj) >= 0) && (obj_index[GET_OBJ_RNUM(obj)].number > 0))
    (obj_index[GET_OBJ_RNUM(obj)].number)--;
//  log ("5");

  // Remove the corpse from corpseData, removeCorpse shall handle whether
  // or not that this obj is the original corpse
  if (IS_CORPSE(obj) && (GET_CORPSEID(obj) > -1)) {
    corpseData.removeCorpse(obj);
  }

  if (SCRIPT(obj))
    extract_script(SCRIPT(obj));

  free_obj(obj);
}



void update_object(struct obj_data * obj, int use)
{
  /* dont update objects with a timer trigger */
  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && (GET_OBJ_TIMER(obj) > 0))
    GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}


void update_char_objects(struct char_data * ch)
{
  int i;

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) > 0) {
	i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2);
	if (i == 1) {
	  send_to_char("Your light begins to flicker and fade.\r\n", ch);
	  act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
	} else if (i == 0) {
	  send_to_char("Your light sputters out and dies.\r\n", ch);
	  act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
	  world[ch->in_room].light--;
	}
      }

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      update_object(GET_EQ(ch, i), 2);

  if (ch->carrying)
    update_object(ch->carrying, 1);
}



/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(struct char_data * ch)
{

  struct char_data *k, *temp;
  struct descriptor_data *t_desc;
  struct obj_data *obj;
  int i, freed = 0;

  void die_follower(struct char_data * ch);
  void die_assisting(struct char_data *ch);
  void stop_hunting(struct char_data *ch);
  void stop_hunters(struct char_data *ch);

  if (!IS_NPC(ch) && !ch->desc)
  {
    for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
      if (t_desc->original == ch)
	do_return(t_desc->character, NULL, 0, 0);
  }
  if (ch->in_room == NOWHERE)
  {
    basic_mud_log("SYSERR: NOWHERE extracting char %s. (%s, extract_char)",
	GET_NAME(ch), __FILE__);
    core_dump();
    exit(1);
  }
  if (ch->followers || ch->master)
    die_follower(ch);

  if (AUTOASSIST(ch) || ch->autoassisters)
    die_assisting(ch);

  if (HUNTING(ch))
    stop_hunting(ch);
  stop_hunters(ch);

  /* Forget snooping, if applicable */
  if (ch->desc)
  {
    if (ch->desc->snooping)
    {
      ch->desc->snooping->snoop_by = NULL;
      ch->desc->snooping = NULL;
    }
    if (ch->desc->snoop_by)
    {
      SEND_TO_Q("Your victim is no longer among us.\r\n",
		ch->desc->snoop_by);
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }
  }
  /* transfer objects to room, if any */
  while (ch->carrying)
  {
    obj = ch->carrying;
    obj_from_char(obj);
    OBJ_RIDDEN(obj) = NULL;
    obj_to_room(obj, ch->in_room);
  }

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_room(unequip_char(ch, i, FALSE), ch->in_room);

  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp)
  {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }

  if (MOUNTING(ch))
  {
	MOUNTING(MOUNTING(ch)) = NULL;
	MOUNTING(ch) = NULL;
  }
  if (MOUNTING_OBJ(ch))
  {
	OBJ_RIDDEN(MOUNTING_OBJ(ch)) = NULL;
	MOUNTING_OBJ(ch) = NULL;

  }

  char_from_room(ch);

  /* pull the char from the list */
  REMOVE_FROM_LIST(ch, character_list, next);

  if (ch->desc && ch->desc->original)
    do_return(ch, NULL, 0, 0);

  if (!IS_NPC(ch))
  {
    save_char(ch, NOWHERE);
    Crash_delete_crashfile(ch);
  } else {
    if (GET_MOB_RNUM(ch) > -1)		/* if mobile */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch);		/* Only NPC's can have memory */

    if (SCRIPT(ch))
      extract_script(SCRIPT(ch));
    if (SCRIPT_MEM(ch))
      extract_script_mem(SCRIPT_MEM(ch));

    free_char(ch);
    freed = 1;
  }

  if (!freed && ch->desc != NULL)
  {
    STATE(ch->desc) = CON_MENU;
    SEND_TO_Q(MENU, ch->desc);
  } else {  /* if a player gets purged from within the game */
    if (!freed)
      free_char(ch);
  }
}



/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */


/* DM search all over the world for a player id, return char ptr */
struct char_data *get_player_by_id(long idnum)
{
  struct char_data *i;
  for (i = character_list; i; i = i->next ) 
    if (!IS_NPC(i) && (idnum == GET_IDNUM(i)))
      return i;
  return NULL;
}

#if 0 // ARTUS> REPLACED. SEE locate.c
struct char_data *get_player_vis(struct char_data * ch, char *name, int inroom)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next) {
    if (IS_NPC(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && i->in_room != ch->in_room)
      continue;
    if (str_cmp(i->player.name, name)) /* If not same, continue */
      continue;
    if (!CAN_SEE(ch, i))
      continue;
    return (i);
  }

  return (NULL);
}


struct char_data *get_char_room_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* JE 7/18/94 :-) :-) */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return (ch);

  /* 0.<name> means PC with name */
  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return (get_player_vis(ch, tmp, FIND_CHAR_ROOM));

  for (i = world[ch->in_room].people; i && j <= number; i = i->next_in_room)
    if (isname(tmp, i->player.name))
      if (CAN_SEE(ch, i))
	if (++j == number)
	  return (i);

  return (NULL);
}

struct char_data *get_char_online(struct char_data *ch, char  *name, int where)
{
	struct char_data *i;

	int j = 0, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;
	
	/* check room first */
	if(where == FIND_CHAR_ROOM)
		return get_char_room_vis(ch, name);
	else if (where == FIND_CHAR_WORLD)
		if ((i = get_char_room_vis(ch, name)) != NULL)
			return i;

	strcpy(tmp, name);
	if (!(number = get_number(&tmp)))
		return get_player_vis(ch, tmp, 0);
	
	for (i = character_list; i && (j <= number); i = i->next)
	{
		if (isname(tmp, i->player.name))
			if (++j == number)
				return i;

	}
	
	return NULL;
}

struct char_data *get_char_vis(struct char_data * ch, char *name, int where)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* check the room first */
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(ch, name);
  else if (where == FIND_CHAR_WORLD || where == FIND_CHAR_INWORLD) {
    if ((i = get_char_room_vis(ch, name)) != NULL)
      return (i);

    strcpy(tmp, name);
    if (!(number = get_number(&tmp)))
      return get_player_vis(ch, tmp, 0);

    for (i = character_list; i && (j <= number); i = i->next)
      if (isname(tmp, i->player.name) && CAN_SEE(ch, i))
        if (++j == number)
	  if (where == FIND_CHAR_INWORLD) {
            if (get_world(i->in_room) == get_world(ch->in_room))
	      return (i);
	  } else {
            return (i);
	  }
  }

  return (NULL);
}



struct obj_data *get_obj_in_list_vis(struct char_data * ch, char *name,
				              struct obj_data * list)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return (NULL);

  for (i = list; i && (j <= number); i = i->next_content)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (++j == number)
	  return (i);

  return (NULL);
}




/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data * ch, char *name)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
    return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) != NULL)
    return (i);

  strcpy(tmp, name);
  if ((number = get_number(&tmp)) == 0)
    return (NULL);

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && (j <= number); i = i->next)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (++j == number)
	  return (i);

  return (NULL);
}



struct obj_data *get_object_in_equip_vis(struct char_data * ch,
		           char *arg, struct obj_data * equipment[], int *j)
{
  for ((*j) = 0; (*j) < NUM_WEARS; (*j)++)
    if (equipment[(*j)])
      if (CAN_SEE_OBJ(ch, equipment[(*j)]))
	if (isname(arg, equipment[(*j)]->name))
	  return (equipment[(*j)]);

  return (NULL);
}
#endif

char *money_desc(int amount)
{
  static char buf[128];

  if (amount <= 0) {
    basic_mud_log("SYSERR: Try to create negative or 0 money (%d).", amount);
    return (NULL);
  }
  if (amount == 1)
    strcpy(buf, "a gold coin");
  else if (amount <= 10)
    strcpy(buf, "a tiny pile of gold coins");
  else if (amount <= 20)
    strcpy(buf, "a handful of gold coins");
  else if (amount <= 75)
    strcpy(buf, "a little pile of gold coins");
  else if (amount <= 200)
    strcpy(buf, "a small pile of gold coins");
  else if (amount <= 1000)
    strcpy(buf, "a pile of gold coins");
  else if (amount <= 5000)
    strcpy(buf, "a big pile of gold coins");
  else if (amount <= 10000)
    strcpy(buf, "a large heap of gold coins");
  else if (amount <= 20000)
    strcpy(buf, "a huge mound of gold coins");
  else if (amount <= 75000)
    strcpy(buf, "an enormous mound of gold coins");
  else if (amount <= 150000)
    strcpy(buf, "a small mountain of gold coins");
  else if (amount <= 250000)
    strcpy(buf, "a mountain of gold coins");
  else if (amount <= 500000)
    strcpy(buf, "a huge mountain of gold coins");
  else if (amount <= 1000000)
    strcpy(buf, "an enormous mountain of gold coins");
  else
    strcpy(buf, "an absolutely colossal mountain of gold coins");

  return (buf);
}


struct obj_data *create_money(int amount)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200];

  if (amount <= 0) {
    basic_mud_log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = str_dup("coin gold");
    obj->short_description = str_dup("a gold coin");
    obj->description = str_dup("One miserable gold coin is lying here.");
    new_descr->keyword = str_dup("coin gold");
    new_descr->description = str_dup("It's just one miserable little gold coin.");
  } else {
    obj->name = str_dup("coins gold");
    obj->short_description = str_dup(money_desc(amount));
    sprintf(buf, "%s is lying here.", money_desc(amount));
    obj->description = str_dup(CAP(buf));

    new_descr->keyword = str_dup("coins gold");
    if (amount < 10) {
      sprintf(buf, "There are %d coins.", amount);
      new_descr->description = str_dup(buf);
    } else if (amount < 100) {
      sprintf(buf, "There are about %d coins.", 10 * (amount / 10));
      new_descr->description = str_dup(buf);
    } else if (amount < 1000) {
      sprintf(buf, "It looks to be about %d coins.", 100 * (amount / 100));
      new_descr->description = str_dup(buf);
    } else if (amount < 100000) {
      sprintf(buf, "You guess there are, maybe, %d coins.",
	      1000 * ((amount / 1000) + number(0, (amount / 1000))));
      new_descr->description = str_dup(buf);
    } else
      new_descr->description = str_dup("There are a LOT of coins.");
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
  GET_OBJ_VAL(obj, 0) = amount;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return (obj);
}


#if 0 // ARTUS> REPLACED -- SEE locate.c
/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, bitvector_t bitvector, struct char_data * ch,
		     struct char_data ** tar_ch, struct obj_data ** tar_obj)
{
  int i, found;
  char name[256];

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	/* Find person in room */
    if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_ROOM)) != NULL)
      return (FIND_CHAR_ROOM);
  }
  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_vis(ch, name, FIND_CHAR_WORLD)) != NULL)
      return (FIND_CHAR_WORLD);
  }
  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
      if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name)) {
	*tar_obj = GET_EQ(ch, i);
	found = TRUE;
      }
    if (found)
      return (FIND_OBJ_EQUIP);
  }
  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)) != NULL)
      return (FIND_OBJ_INV);
  }
  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)) != NULL)
      return (FIND_OBJ_ROOM);
  }
  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name)))
      return (FIND_OBJ_WORLD);
  }
  return (0);
}
#endif

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
  if (!strcmp(arg, "all"))
    return (FIND_ALL);
  else if (!strncmp(arg, "all.", 4)) {
    strcpy(arg, arg + 4);
    return (FIND_ALLDOT);
  } else
    return (FIND_INDIV);
}
