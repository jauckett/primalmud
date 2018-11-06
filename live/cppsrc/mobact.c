/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"

/* external structs */
extern struct char_data *character_list;
extern struct index_data *mob_index, *obj_index;
extern struct room_data *world;
extern int no_specials;

ACMD(do_get);

/* local functions */
void mobile_activity(void);
void object_activity(void);
void world_activity(void);
void clearMemory(struct char_data * ch);
#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

/* external functions */
int _is_empty(zone_rnum zone_nr);

void mobile_activity(void)
{
  register struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  struct char_data *mob;
  struct script_data *sc;
  int door, found, max;
  memory_rec *names;

  for (ch = character_list; ch; ch = next_ch) 
  {
    next_ch = ch->next;
    if (!IS_MOB(ch))
      continue;
    // DM - hack error checking - extract mob
    // extract_char will exit if in_room == NOWHERE, hence to avoid an
    // exit due to a mob being NOWHERE, put the char in IDLE_ROOM_VNUM
    // and then extract the mob.
    if (ch->in_room == NOWHERE)
    {
      char_to_room(ch, real_room(IDLE_ROOM_VNUM));
      extract_char(ch);
      continue;
    }
    /* Examine call for special procedure */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
    {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL)
      {
	basic_mud_log("SYSERR: %s (#%d): Attempting to call non-existing mob function.", GET_NAME(ch), GET_MOB_VNUM(ch));
	REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
      } else {
	/* XXX: Need to see if they can handle NULL instead of "". */
	if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, ""))
	  continue;		/* go to next char */
      }
    }

    /* Artus> Moved DGScript Check Here. */
    if (SCRIPT(ch))
    {
      sc = SCRIPT(ch);
      //if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
      //   (!is_empty(world[IN_ROOM(ch)].zone) ||
      //    IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
      // random_mtrigger(ch);
      if (IS_SET(SCRIPT_TYPES(sc), MTRIG_RANDOM) &&
         (!_is_empty(world[IN_ROOM(ch)].zone) ||
          IS_SET(SCRIPT_TYPES(sc), MTRIG_GLOBAL)))
        random_mtrigger(ch);
    }

    /* If the mob has no specproc, do the default actions */
    if (FIGHTING(ch) || !AWAKE(ch))
      continue;

    /* Scavenger (picking up objects) */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !FIGHTING(ch) && AWAKE(ch))
      if (world[ch->in_room].contents && !number(0, 10))
      {
	max = 1;
	best_obj = NULL;
	for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
	  if ((CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) &&
	      (GET_OBJ_VNUM(obj) != GOLD_OBJ_VNUM))
	  {
	    best_obj = obj;
	    max = GET_OBJ_COST(obj);
	  }
	  if (best_obj != NULL)
	  {
	    obj_from_room(best_obj);
	    obj_to_char(best_obj, ch, __FILE__, __LINE__);
	    act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
	  }
      }
    /* Mob Movement */
    if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
	((door = number(0, 18)) < NUM_OF_DIRS) && CAN_GO(ch, door) &&
	!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH) &&
	(!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
	 (world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone)))
      perform_move(ch, door, 1);

    /* Aggressive Mobs */
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN))
    {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room) 
      {
	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;
	if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
	  continue;
	if (!MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN) ||
	    (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
	    (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
	    (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)))
	{
	  // Do a check, creatures wont attack players disguised as them, or 
	  // same class (ie demihuman)
	  if (!IS_NPC(vict) && IS_SET(GET_SPECIALS(vict), SPECIAL_DISGUISE) && CHAR_DISGUISED(vict)) 
	  {
	    mob = read_mobile(CHAR_DISGUISED(vict), VIRTUAL);
	    if ((CHAR_DISGUISED(vict) == GET_MOB_VNUM(ch)) || (GET_CLASS(mob) == GET_CLASS(ch)) ) 
	      found = FALSE;
	    else
	    {
	      hit(ch, vict, TYPE_UNDEFINED);
	      found = TRUE;
	    }
          }
        }
      }
    }

    /* Mob Memory */
    if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
    {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room)
      {
	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;
	for (names = MEMORY(ch); names && !found; names = names->next)
	  if (names->id == GET_IDNUM(vict))
	  {
	    found = TRUE;
	    act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.",
		FALSE, ch, 0, 0, TO_ROOM);
	    hit(ch, vict, TYPE_UNDEFINED);
	  }
      }
    }

    /* Helper Mobs */
    if (MOB_FLAGGED(ch, MOB_HELPER) && !AFF_FLAGGED(ch, AFF_BLIND | AFF_CHARM))
    {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room) 
      {
	if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
	  continue;
	if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
	  continue;
	act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
	hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
	found = TRUE;
      }
    }
    /* Add new mobile actions here */

  }				/* end for() */
}



/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data * ch, struct char_data * victim)
{
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}


/* make ch forget victim */
void forget(struct char_data * ch, struct char_data * victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;			/* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}


/* erase ch's memory */
void clearMemory(struct char_data * ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}


/////////////////////////// World Activity ////////////////////////////////
void world_activity(void)
{
  room_rnum nr;
  struct room_data *room;
  struct script_data *sc;
  extern int pulse;
  void check_world_environment(void);

  if (!(pulse % PULSE_DG_SCRIPT * 3)) // Every 3 DG_SCRIPT_PULSES
    check_world_environment();
      
  for (nr = 0; nr <= top_of_world; nr++)
  {
    // Artus> Room Specials..
    if (world[nr].func != NULL)
      if ((world[nr].func) (NULL, &world[nr], 0, ""))
	continue;		/* go to next room */

    // Artus> Moved DGScript Check Here..
    if (SCRIPT(&world[nr]))
    {
      room = &world[nr];
      sc = SCRIPT(room);
      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
         (!_is_empty(room->zone) ||
          IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
       random_wtrigger(room);
    }
  }
}

// Island Forever, Mostly.
void rapid_blasts(void)
{
  extern struct descriptor_data *descriptor_list;
  extern struct zone_data *zone_table;
  struct descriptor_data *d, *nextd;
  struct char_data *ch;
  void island_blast(struct char_data *vict, char btype);

  for (d = descriptor_list; d; d=nextd)
  {
    nextd = d->next;
    if (!(IS_PLAYING(d) && (ch = d->character)))
      continue;
    switch (zone_table[world[IN_ROOM(ch)].zone].number)
    {
      case 101: // Island Forever
        switch (world[IN_ROOM(ch)].number)
	{
	  case 10208:
	  case 10209:
	  case 10210:
	    island_blast(ch, ISBLAST_GAUNTLET);
	    break;
	  case 10106:
	    island_blast(ch, ISBLAST_ARROW);
	    break;
	}
    }
  }
}

/////////////////////////// Object Activity //////////////////////////////
void object_activity(void)
{
  struct script_data *sc;
  struct obj_data *obj;
  extern struct obj_data *object_list;

  for (obj = object_list; obj; obj = obj->next)
  {
    // Artus> For mail, corpses, etc.
    if (obj->item_number < 0)
      continue;

    // Artus> Object Specials
    if (obj_index[GET_OBJ_RNUM(obj)].func != NULL)
      if ((obj_index[GET_OBJ_RNUM(obj)].func) (NULL, obj, 0, ""))
	continue;		/* go to next obj */

    // Artus> Moved DGScript Check Here..
    if (SCRIPT(obj))
    {
      sc = SCRIPT(obj);
      if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
       random_otrigger(obj);
    }
  }
}
