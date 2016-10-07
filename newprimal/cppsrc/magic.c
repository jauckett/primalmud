/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "dg_scripts.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index; 

extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;
extern int pk_allowed;
extern char *spell_wear_off_msg[];

byte saving_throws(struct char_data *ch, int type); /* class.c */
void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
// /*unused? - artus*/ extern struct spell_info_type spell_info[];
void clan_rel_inc(struct char_data *ch, struct char_data *vict, int amt);

/* local functions */
int mag_materials(struct char_data * ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, struct char_data * ch, struct char_data * tch, int spellnum, int savetype);
int mag_savingthrow(struct char_data * ch, int type, int modifier);
void affect_update(void);

/*
 * Saving throws are now in class.c as of bpl13.
 */


/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */
int mag_savingthrow(struct char_data * ch, int type, int modifier)
{
  int save;

  save = saving_throws(ch, type);
//  save += GET_SAVE(ch, type);
  save += modifier;

  /* Throwing a 0 is always a failure. */
  if (MAX(1, save) < number(0, 100))
    return (TRUE);

  /* Oops, failed. Sorry. */
  return (FALSE);
}


/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  struct affected_type *af, *next;
  struct char_data *i;
  struct timer_type *tim, *next_timer;
  int found, eq;

  for (i = character_list; i; i = i->next)
  {
    // Remove timers-structures
    for (tim = CHAR_TIMERS(i); tim; tim = next_timer)
    {
      next_timer = tim->next;
      // Decrement time
      if (tim->duration > 1)
        tim->duration--;
      else if (tim->duration == -1)      /* No action */
        tim->duration = -1;
      // Remove Timer 
      else
        timer_remove_char(i, tim); 
    }
    for (af = i->affected; af; af = next)
    {
      next = af->next;
      if (af->duration >= 1)
	af->duration--;
      else if (af->duration == -1)	/* No action */
	af->duration = -1;	/* GODs only! unlimited */
      else
      {
/* modification to check if magic eq is still worn, if not remove
 * its spell. - Vader
 */
        found = 0; /* 0 found or nothing wears off! */
        for(eq = 0; eq < NUM_WEARS; eq++)
          if(i->equipment[eq] != NULL)
            if((GET_OBJ_TYPE(i->equipment[eq]) == ITEM_MAGIC_EQ) &&
               ((GET_OBJ_VAL(i->equipment[eq],0) == af->type) ||
                (GET_OBJ_VAL(i->equipment[eq],1) == af->type) ||
                (GET_OBJ_VAL(i->equipment[eq],2) == af->type)))
	    {
              found = 1;
              break;
	    }
        if(found)
          continue;
	if ((af->type > 0) && (af->type <= MAX_SPELLS))
	  if (!af->next || (af->next->type != af->type) ||
	      (af->next->duration > 0))
	    if (*spell_wear_off_msg[af->type])
	    {
              /* DM - NOHASSLE spell set PRF_NOHASSLE off */
              if (af->type == SPELL_NOHASSLE)
                REMOVE_BIT(PRF_FLAGS(i), PRF_NOHASSLE);
	      send_to_char("&r", i);
	      send_to_char(spell_wear_off_msg[af->type], i);
	      send_to_char("&n\r\n", i);
	    }
	affect_remove(i, af);
      }
    }
  }
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data * ch, int item0, int item1, int item2,
		      int extract, int verbose)
{
  struct obj_data *tobj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (number(0, 2)) {
      case 0:
	send_to_char("A wart sprouts on your nose.\r\n", ch);
	break;
      case 1:
	send_to_char("Your hair falls out in clumps.\r\n", ch);
	break;
      case 2:
	send_to_char("A huge corn develops on your big toe.\r\n", ch);
	break;
      }
    }
    return (FALSE);
  }
  if (extract) {
    if (item0 < 0)
      extract_obj(obj0);
    
    if (item1 < 0)
      extract_obj(obj1);
    
    if (item2 < 0)
      extract_obj(obj2);
    
  }
  if (verbose) {
    send_to_char("A puff of smoke rises from your pack.\r\n", ch);
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}

/* Artus> Fireball, Fire Breath, and anything else that burns should call
 *        this routine. */
void handle_fireball(struct char_data *ch)
{
  struct obj_data *i, *j, *obj = NULL;

  if ((IS_NPC(ch)) || !LR_FAIL(ch, LVL_IS_GOD) || UNDERWATER(ch))
    return;
  if (!(ch->carrying))
    return;
  for (i = ch->carrying; i; i = i->next_content)
  {
    switch (GET_OBJ_TYPE(i))
    {
      case ITEM_POTION:
        if (number(1, 20) == 1)
	  obj = i;
	break;
      case ITEM_SCROLL:
      case ITEM_NOTE:
	if (number(1, 15) == 1)
	  obj = i;
	break;
      case ITEM_BATTERY:
	if (number(1, 50) == 1)
	  obj = i;
	break;
      case ITEM_CONTAINER:
	if ((OBJVAL_FLAGGED(i, CONT_CLOSED)) || 
	    !(i->contains) || (number(1, 5) > 1))
	  break;
	for (j = i->contains; j; j = j->next_content)
	{
	  switch(GET_OBJ_TYPE(j))
	  {
	    case ITEM_POTION:
	      if (number(1, 20) == 1)
		obj = j;
	      break;
	    case ITEM_SCROLL:
	    case ITEM_NOTE:
	      if (number(1, 15) == 1)
		obj = j;
	      break;
	    case ITEM_BATTERY:
	      if (number(1, 50) == 1)
		obj = j;
	      break;
	  }
	  if (obj)
	    break;
	}
	break;
    }
    if (obj)
      break;
  }
  // No Damage.
  if (!(obj))
    return;
  switch (GET_OBJ_TYPE(obj))
  {
    case ITEM_POTION:
      sprintf(buf, "&[&r... &5%s &rboils and explodes in your hands!&]\r\n", 
	      OBJS(obj, ch));
      send_to_char(buf, ch);
      damage(NULL, ch, MIN(GET_HIT(ch), dice(2, GET_LEVEL(ch))), 
	     TYPE_UNDEFINED, FALSE);
      if (!OBJ_FLAGGED(obj, ITEM_QEQ))
	extract_obj(obj);
      return;
    case ITEM_NOTE:
    case ITEM_SCROLL:
      sprintf(buf, "&[&r... &5%s &rburns to a cinder in your hands!&]\r\n", 
	      OBJS(obj, ch));
      send_to_char(buf, ch);
      damage(NULL, ch, MIN(GET_HIT(ch), number(1, GET_LEVEL(ch))),
	     TYPE_UNDEFINED, FALSE);
      if (!OBJ_FLAGGED(obj, ITEM_QEQ))
	extract_obj(obj);
      return;
      break;
    case ITEM_BATTERY:
      sprintf(buf, "&[&r... &5%s &rexplodes in your hand, covering you with acid!&]\r\n", OBJS(obj, ch));
      send_to_char(buf, ch);
      damage(NULL, ch, 
	     MIN(GET_HIT(ch), dice(3, GET_LEVEL(ch))+GET_OBJ_VAL(obj, 2)), 
	     TYPE_UNDEFINED, FALSE);
      if (!OBJ_FLAGGED(obj, ITEM_QEQ))
	extract_obj(obj);
      return;
      break;
  }
}

/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype)
{
  int dam = 0;
  int tmpdam = 0;
 
  if (victim == NULL || ch == NULL)
    return (0);
 
 
  switch (spellnum) {
    /* Mostly mages */
  case SPELL_MAGIC_MISSILE:
  case SPELL_CHILL_TOUCH:       /* chill touch also has an affect */
      dam = dice(1, 8) + 1;
      clan_rel_inc(ch, victim, -1);
    break;
  case SPELL_BURNING_HANDS:
      dam = dice(3, 8) + 3;
      clan_rel_inc(ch, victim, -1);
    break;
  case SPELL_SHOCKING_GRASP:
      dam = dice(5, 8) + 5;
      clan_rel_inc(ch, victim, -2);
    break;
  case SPELL_LIGHTNING_BOLT:
      dam = dice(7, 8) + 7;
      clan_rel_inc(ch, victim, -3);
      handle_fireball(victim);
    break;
  case SPELL_COLOR_SPRAY:
      dam = dice(9, 8) + 9;
      clan_rel_inc(ch, victim, -3);
    break;
  case SPELL_FIREBALL:
      dam = dice(MINMAX(5, 10, (int)(level/10)), 8) + 11;
      clan_rel_inc(ch, victim, -5);
      handle_fireball(victim);
    break;
  case SPELL_PLASMA_BLAST:
      dam = dice(10, 10) + (level/2);
      clan_rel_inc(ch, victim, -7);
    break;
  case SPELL_WRAITH_TOUCH:
    dam = (int) (dice(20,25) + (level*0.75));
    if(IS_AFFECTED(victim, AFF_SANCTUARY))
      tmpdam = dam/2;
    else
      tmpdam= dam;
    /*
    if ((GET_HIT(ch)+tmpdam)>GET_MAX_HIT(ch))
      GET_HIT(ch) = GET_MAX_HIT(ch);
    else
      GET_HIT(ch)+=tmpdam;
    // Artus> I think this is a better way:- */
    if (GET_HIT(ch) < GET_MAX_HIT(ch))
      GET_HIT(ch) = MIN(GET_HIT(ch)+tmpdam, GET_MAX_HIT(ch));
    break;
    /* Mostly clerics */
  case SPELL_DISPEL_EVIL:
    if (IS_EVIL(victim)) 
    {
      // Artus> This should be a little more balanced.
      dam = MAX(GET_LEVEL(ch), MIN(GET_LEVEL(ch)*5, (int)(0-(GET_ALIGNMENT(victim) - 350)/4)));
      clan_rel_inc(ch, victim, -1);
    } else if (!IS_EVIL(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      dam = 0;
      return (0);
    }
    break;

  case SPELL_DISPEL_GOOD:
    if (IS_GOOD(victim)) {
      // Artus> Same as DISPEL_EVIL
      dam = MAX(GET_LEVEL(ch), MIN(GET_LEVEL(ch)*5, (int)((GET_ALIGNMENT(victim) - 350)/4)));
      clan_rel_inc(ch, victim, -1);
    } else if (!IS_GOOD(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      dam = 0;
      return (0);
    }
    break;
  case SPELL_CALL_LIGHTNING:
    dam = dice(MINMAX(1,15,GET_LEVEL(ch)), 8);
    clan_rel_inc(ch, victim, -4);
    handle_fireball(victim);
    break;
 
  case SPELL_HARM:
    dam = dice(8, 8) + 8; 
    clan_rel_inc(ch, victim, -4);
    break;
 
  case SPELL_FINGERDEATH:
    dam = dice(10, 20);
    break;
 
  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2)
      dam = 100;
    else
      dam = dice(1, 10);
    break;

    /* Area spells */
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    break;
 
  case SPELL_WHIRLWIND:
    dam = (dice(1, 3) + 4) *  level;
    break;
 
  case SPELL_METEOR_SWARM:
    dam = (int)(dice(11, 8) + (level*0.75));
    break;
 
  case SPELL_CLOUD_KILL:
    if (GET_LEVEL(victim) <=20) {
      GET_HIT(victim)=0;
      dam = 50;
    } else
      dam = (int)((dice(50,16) + (level*0.75)));
    break;

  }                             /* switch(spellnum) */
 
  if (mag_savingthrow(victim, savetype, 0))
    dam >>= 1;
  // failure
  else {
    // exp gains/losses in energy drain (npc victims only)
    if (spellnum == SPELL_ENERGY_DRAIN && IS_NPC(victim)) {
      int exp = MINMAX(1, 40000, level * 5000);

      if (GET_EXP(victim) < exp) {
        exp = GET_EXP(victim) / 2;
      }
      exp /= 4;
      GET_EXP(victim) -= exp;
      gain_exp(ch, exp);
    }
  }
 
  // Modify damage for some classes
  if (((GET_CLASS(ch) == CLASS_DRUID) && (SECT(ch->in_room) == SECT_FOREST))
    || (GET_CLASS(ch) == CLASS_BATTLEMAGE) || (GET_CLASS(ch) == CLASS_MASTER) ) 
	dam = (int)(dam * GET_MODIFIER(ch));

  return (damage(ch, victim, dam, spellnum, TRUE));

  /************************** stock 30bpl17 damage spells
    // Mostly mages 
  case SPELL_MAGIC_MISSILE:
  case SPELL_CHILL_TOUCH:	// chill touch also has an affect 
    if (IS_MAGIC_USER(ch))
      dam = dice(1, 8) + 1;
    else
      dam = dice(1, 6) + 1;
    break;
  case SPELL_BURNING_HANDS:
    if (IS_MAGIC_USER(ch))
      dam = dice(3, 8) + 3;
    else
      dam = dice(3, 6) + 3;
    break;
  case SPELL_SHOCKING_GRASP:
    if (IS_MAGIC_USER(ch))
      dam = dice(5, 8) + 5;
    else
      dam = dice(5, 6) + 5;
    break;
  case SPELL_LIGHTNING_BOLT:
    if (IS_MAGIC_USER(ch))
      dam = dice(7, 8) + 7;
    else
      dam = dice(7, 6) + 7;
    break;
  case SPELL_COLOR_SPRAY:
    if (IS_MAGIC_USER(ch))
      dam = dice(9, 8) + 9;
    else
      dam = dice(9, 6) + 9;
    break;
  case SPELL_FIREBALL:
    if (IS_MAGIC_USER(ch))
      dam = dice(11, 8) + 11;
    else
      dam = dice(11, 6) + 11;
    break;

    // Mostly clerics 
  case SPELL_DISPEL_EVIL:
    dam = dice(6, 8) + 6;
    if (IS_EVIL(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_GOOD(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;
  case SPELL_DISPEL_GOOD:
    dam = dice(6, 8) + 6;
    if (IS_GOOD(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_EVIL(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;


  case SPELL_CALL_LIGHTNING:
    dam = dice(7, 8) + 7;
    break;

  case SPELL_HARM:
    dam = dice(8, 8) + 8;
    break;

  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2)
      dam = 100;
    else
      dam = dice(1, 10);
    break;

    // Area spells 
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    break;

  } // switch(spellnum)


  // divide damage by two if victim makes his saving throw 
  if (mag_savingthrow(victim, savetype, 0))
    dam /= 2;

  // and finally, inflict the damage
  return (damage(ch, victim, dam, spellnum));
  ***************************************************/
}


/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
*/

#define MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, struct char_data * ch, struct char_data * victim,
		      int spellnum, int savetype)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;

  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) 
  {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  switch (spellnum) {
  case SPELL_CHILL_TOUCH: //////////////////////////////// Chill Touch
    af[0].location = APPLY_STR;
    if (mag_savingthrow(victim, savetype, 0))
      af[0].duration = 1;
    else
      af[0].duration = 4;
    af[0].modifier = -1;
    accum_duration = TRUE;
    accum_affect = TRUE;
    to_vict = "You feel your strength wither!";
    break;
  case SPELL_ARMOR: ////////////////////////////////////// Armor
    if (affected_by_spell(victim, SPELL_ARMOR))
      to_vict = "You feel a little more protected.";
    else
      to_vict = "You feel someone protecting you.";
    af[0].location = APPLY_AC;
    af[0].modifier = -15;
    af[0].duration = 12;
    accum_duration = FALSE;
    clan_rel_inc(ch, victim, 1);
/**************** stock 30bpl19 armor
    af[0].location = APPLY_AC;
    af[0].modifier = -20;
    af[0].duration = 24;
    accum_duration = TRUE;
    to_vict = "You feel someone protecting you.";
**********************************************/
    break;
 case SPELL_SPIRIT_ARMOR: //////////////////////////////// Spirit Armor
    if (affected_by_spell(victim, SPELL_SPIRIT_ARMOR))
      to_vict = "You feel the spirits gathering closer around you.";
    else
      to_vict = "You feel divine forces protecting you.";
    af[0].duration = 8;
    af[0].modifier = -30;
    af[0].location = APPLY_AC;
    break;
  case SPELL_STONESKIN: ////////////////////////////////// StoneSkin
    if (affected_by_spell(victim, SPELL_STONESKIN))
      to_vict = "Your skin of stone feels a little more solid.";
    else
    {
      to_vict = "A skin of stone creates itself around you.";
      to_room = "A skin of stone creates itself around $n.";
    }
    af[0].duration = 9;
    af[0].modifier = -40;
    af[0].location = APPLY_AC;
    break;
 case SPELL_LIGHT_SHIELD: //////////////////////////////// Lightning Shield
    if (affected_by_spell(victim, SPELL_FIRE_SHIELD) ||
	affected_by_spell(victim, SPELL_FIRE_WALL))
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    if (affected_by_spell(victim, SPELL_LIGHT_SHIELD))
      to_vict = "Your shield begins quivering a little more violently.";
    else
    {
      to_vict = "Your shield jolts as lightning bolts quiver around it's surface!";
      to_room = "Lightning bolts quiver on $n's shield!";
    }
    af[0].duration = 5;
    af[0].modifier = -15;
    af[0].location = APPLY_AC;
    break;
  case SPELL_FIRE_SHIELD: //////////////////////////////// Fire Shield
/* Artus> Shouldn't this be handled below?
    if (affected_by_spell(victim, SPELL_FIRE_SHIELD))
    {
      send_to_char("Nothing seems to happen!\r\n", ch);
      return;
    }
*/
    if (affected_by_spell(victim, SPELL_LIGHT_SHIELD) ||
	affected_by_spell(victim, SPELL_FIRE_WALL))
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    if (affected_by_spell(victim, SPELL_FIRE_SHIELD))
      to_vict = "The fire surrounding your shield burns a little brighter.";
    else 
    {
      to_vict = "You feel a wave of heat as fire consumes the surface of your shield!";
      to_room = "A wave of fire consumes $n's shield!";
    }
    af[0].duration = 6;
    af[0].modifier = -20;
    af[0].location = APPLY_AC;
    break;
  case SPELL_FIRE_WALL: ////////////////////////////////// Fire Wall
    if (affected_by_spell(victim, SPELL_LIGHT_SHIELD) ||
       affected_by_spell(victim, SPELL_FIRE_SHIELD))
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    if (affected_by_spell(victim, SPELL_FIRE_WALL))
      to_vict = "The wall around you burns a little brighter.";
    else
    {
      to_vict = "You feel a wave of heat as a wall of fire surrounds yourself!";
      to_room = "A wall of fire ignites around $n!";
    }
    af[0].duration = 7;
    af[0].modifier = -20;
    af[0].location = APPLY_AC;
    break;
  case SPELL_HASTE: ////////////////////////////////////// Haste
    if (affected_by_spell(victim, SPELL_HASTE))
      to_vict = "You feel a little more speedy.";
    else if (IS_AFFECTED(victim, AFF_HASTE))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else
      to_vict = "Whoooooahhhh, what a RUSH!!!! You are speeding!";
    af[0].duration = 6;
    af[0].bitvector = AFF_HASTE;
    break; 
  case SPELL_BLESS: ////////////////////////////////////// Bless
    if (affected_by_spell(victim, SPELL_BLESS))
      to_vict = "You feel a little more righteous.";
    else 
      to_vict = "You feel righteous.";

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 2;
    af[0].bitvector = 0;
    af[0].duration = 6;

    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -1;
    af[1].duration = 6;

    af[2].location = APPLY_DAMROLL;
    af[2].modifier = 1;
    af[2].bitvector = 0;
    af[2].duration = 6;

    clan_rel_inc(ch, victim, 2);
    break;
/**************** stock 30bpl19 bless
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 2;
    af[0].duration = 6;

    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -1;
    af[1].duration = 6;

    accum_duration = TRUE;
    to_vict = "You feel righteous.";
    break;
******************************************/

  case SPELL_DIVINE_PROTECTION: ////////////////////////// Divine Protection
    if (affected_by_spell(victim, SPELL_DIVINE_PROTECTION))
      to_vict = "You feel your deity protecting you a little closer.";
    else
      to_vict = "You feel your deity protecting you.";
    
    af[0].location = APPLY_AC;
    af[0].modifier = -40;
    af[0].duration = 6;

    accum_duration = FALSE;
    accum_affect = FALSE;
    break;
  case SPELL_HOLY_AID: /////////////////////////////////// Holy Aid
    if (affected_by_spell(victim, SPELL_HOLY_AID))
      to_vict = "You feel your deity's confidence in you grow.";
    else 
      to_vict = "Your God hears your prayer and assists you.";

    af[0].location = APPLY_AC;
    af[0].modifier = -10;
    af[0].duration = 7;
 
    af[1].location = APPLY_HITROLL;
    af[1].modifier = 3;
    af[1].bitvector = 0;
    af[1].duration = 7;
 
    af[2].location = APPLY_DAMROLL;
    af[2].modifier = 4;
    af[2].bitvector = 0;
    af[2].duration = 7;
    break; 
  case SPELL_DRAGON: ///////////////////////////////////// Dragon
    if (affected_by_spell(victim, SPELL_DRAGON))
      to_vict = "You feel more like a dragon.";
    else
    {
      to_vict = "You feel the blood of a dragon coursing through your veins.";
      to_room = "$n looks more like a dragon now.";
    }
    if (LR_FAIL(ch, 40))
    {
      if (IS_GOOD(ch))
      {
	af[0].location = APPLY_HITROLL;
	af[0].duration = 8;
	af[0].modifier = 1;
      } else if (IS_EVIL(ch)) {
	af[0].location = APPLY_DAMROLL;
	af[0].duration = 8;
	af[0].modifier = 1;
      } else { /* Neutral */
	af[0].location = APPLY_AC;
	af[0].duration = 8;
	af[0].modifier = -10;
      }
    } else if (LR_FAIL(ch, 60)) {
      if (IS_GOOD(ch)) {
	af[0].location = APPLY_HITROLL;
	af[0].duration = 9;
	af[0].modifier = 2;
      } else if (IS_EVIL(ch)) {
	af[0].location = APPLY_DAMROLL;
	af[0].duration = 9;
	af[0].modifier = 2;
      } else { /* Neutral */
	af[0].location = APPLY_AC;
	af[0].duration = 9;
	af[0].modifier = -15;
      }
    } else {
      if (IS_GOOD(ch))
      {
	af[0].location = APPLY_HITROLL;
	af[0].modifier = 3;
	af[0].duration = 10;
	af[0].bitvector = AFF_FLY;
      } else if (IS_EVIL(ch)) {
	af[0].location = APPLY_DAMROLL;
	af[0].modifier = 3;
	af[0].duration = 10;
	af[0].bitvector = AFF_FLY;
      } else { /* Neutral */
	af[0].location = APPLY_AC;
	af[0].modifier = -20;
	af[0].duration = 10;
	af[0].bitvector = AFF_FLY;
      }
    }
    break; 
  case SPELL_BLINDNESS: ////////////////////////////////// Blind
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(victim))
    {
      send_to_char("You are not allowed to Blind other players in this Zone.", ch);
      return;
    }
    if ((MOB_FLAGGED(victim, MOB_NOBLIND)) || 
	mag_savingthrow(victim, savetype, 0))
    { // Lets be cheeky :p~
      act("$N looks at you, and smiles.", FALSE, ch, NULL, victim, TO_CHAR);
      return;
    }
    if (affected_by_spell(victim, SPELL_BLINDNESS))
      to_vict = "The world seems a little darker.";
    else if (IS_AFFECTED(victim, AFF_BLIND))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_room = "$n seems to be blinded!";
      to_vict = "You have been blinded!";
    }
 
    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = 2;
    af[0].bitvector = AFF_BLIND;
 
    af[1].location = APPLY_AC;
    af[1].modifier = 40;
    af[1].duration = 2;
    af[1].bitvector = AFF_BLIND;

    clan_rel_inc(ch, victim, -2);
    break;

    /************************ stock circle 30bpl19 blindness
    if (MOB_FLAGGED(victim,MOB_NOBLIND) || mag_savingthrow(victim, savetype, 0)) {
      send_to_char("You fail.\r\n", ch);
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = 2;
    af[0].bitvector = AFF_BLIND;

    af[1].location = APPLY_AC;
    af[1].modifier = 40;
    af[1].duration = 2;
    af[1].bitvector = AFF_BLIND;

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;
    ********************************************/

  case SPELL_CURSE: ////////////////////////////////////// Curse
    // DM
    if (mag_savingthrow(victim, savetype, 0)) 
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    if (affected_by_spell(victim, SPELL_CURSE))
      to_vict = "You feel a little more uncomfortable.";
    else if (IS_AFFECTED(victim, AFF_CURSE))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_room = "$n briefly glows &rred&n!";
      to_vict = "You feel very uncomfortable.";
    }

    af[0].location = APPLY_HITROLL;
    af[0].duration = 1 + (GET_LEVEL(ch) / 2);
    af[0].modifier = -1;
    af[0].bitvector = AFF_CURSE;

    af[1].location = APPLY_DAMROLL;
    af[1].duration = 1 + (GET_LEVEL(ch) / 2);
    af[1].modifier = -1;
    af[1].bitvector = AFF_CURSE;

    clan_rel_inc(ch, victim, -1);
    break;

  case SPELL_DETECT_ALIGN: /////////////////////////////// Detect Alignment
    if (affected_by_spell(victim, SPELL_DETECT_ALIGN))
      to_vict = "You sense the difference between good and evil a little clearer.";
    else if (IS_AFFECTED(victim, AFF_DETECT_ALIGN))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else
      to_vict = "You become aware of the difference between good and evil.";
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_ALIGN;
    accum_duration = FALSE;
    break;
  case SPELL_DETECT_INVIS: /////////////////////////////// Detect Invis.
    if (affected_by_spell(victim, SPELL_DETECT_INVIS))
      to_vict = "Your eyes tingle a little stronger.";
    else if (IS_AFFECTED(victim, AFF_DETECT_INVIS))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else
      to_vict = "Your eyes tingle.";
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_INVIS;
    accum_duration = FALSE;
    clan_rel_inc(ch, victim, 2);
    break;
  case SPELL_DETECT_MAGIC: /////////////////////////////// Detect Magic.
    if (affected_by_spell(victim, SPELL_DETECT_MAGIC))
      to_vict = "You feel your sensitivity to power strengthen.";
    else if (IS_AFFECTED(victim, AFF_DETECT_MAGIC))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else
      to_vict = "You feel sensitive to the presence of power.";
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_MAGIC;
    accum_duration = FALSE;
    break;
  case SPELL_DETECT_POISON: ////////////////////////////// Detect Poison.
    if (victim == ch)
      if (IS_AFFECTED(victim, AFF_POISON))
        send_to_char("You can sense poison in your blood.\r\n", ch);
      else
        send_to_char("You feel healthy.\r\n", ch);
    else if (IS_AFFECTED(victim, AFF_POISON))
      act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
    else
      act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    return;
  case SPELL_INFRAVISION: //////////////////////////////// Infravision
    if (affected_by_spell(victim, SPELL_INFRAVISION))
      to_vict = "Your night vision becomes a little clearer.";
    else if (IS_AFFECTED(victim, AFF_INFRAVISION))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_vict = "Your eyes glow red.";
      to_room = "$n's eyes glow red.";
    }
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_INFRAVISION;
    accum_duration = FALSE;
    clan_rel_inc(ch, victim, 1);
    break;
  case SPELL_INVISIBLE: ////////////////////////////////// Invisibility
    if (!victim)
      victim = ch;
    if (affected_by_spell(victim, SPELL_INVISIBLE))
      to_vict = "You feel the spell of invisibility hug you a little tighter.";
    else if (IS_AFFECTED(victim, AFF_INVISIBLE))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_vict = "You vanish.";
      to_room = "$n slowly fades out of existence.";
    }
    af[0].duration = 12 + (GET_LEVEL(ch) / 4);
    af[0].modifier = -40;
    af[0].location = APPLY_AC;
    af[0].bitvector = AFF_INVISIBLE;
    accum_duration = FALSE;
    clan_rel_inc(ch, victim, 2);
    break;
  case SPELL_POISON: ///////////////////////////////////// Poison
    if (mag_savingthrow(victim, savetype, 0)) 
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    if (affected_by_spell(victim, SPELL_POISON))
      to_vict = "You feel even sicker.";
    else if (IS_AFFECTED(victim, AFF_POISON))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_vict = "You feel very sick.";
      to_room = "$n gets violently ill!";
    }
    af[0].location = APPLY_STR;
    af[0].duration = GET_LEVEL(ch);
    af[0].modifier = -2;
    af[0].bitvector = AFF_POISON;
    break;
  case SPELL_PROT_FROM_GOOD: ///////////////////////////// Protection from Good
    if (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && 
	!(affected_by_spell(victim, SPELL_PROT_FROM_GOOD)))
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].duration = 24;
    af[0].bitvector = AFF_PROTECT_GOOD;
    accum_duration = FALSE;
    to_vict = "You feel invulnerable!";
    break;

  case SPELL_PROT_FROM_EVIL: ///////////////////////////// Protection from Evil
    if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) && 
	!(affected_by_spell(victim, SPELL_PROT_FROM_EVIL)))
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].duration = 24;
    af[0].bitvector = AFF_PROTECT_EVIL;
    accum_duration = FALSE;
    to_vict = "You feel invulnerable!";
    break;
  case SPELL_SANCTUARY: ////////////////////////////////// Sanctuary
    if (affected_by_spell(victim, SPELL_SANCTUARY))
      to_vict = "The white aura around you glows more vibrantly.";
    else if (IS_AFFECTED(victim, AFF_SANCTUARY))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_vict = "A white aura momentarily surrounds you.";
      to_room = "$n is surrounded by a white aura.";
    }
    af[0].duration = 4 + (int)(GET_LEVEL(ch) / 10);
    af[0].bitvector = AFF_SANCTUARY;
    //if (IS_AFFECTED(victim, AFF_SANCTUARY))
    //  af[0].duration = 1;
    accum_duration = FALSE;
    clan_rel_inc(ch, victim, 3);
    break;
  case SPELL_SLEEP: ////////////////////////////////////// Sleep
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag, ZN_PK_ALLOWED)
	&& !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP | MOB_NOKILL))
    {
      switch(number(0, 2))
      { // Artus> I think this is better.
	case 0 : act("$N resists.\r\n",   FALSE,ch,NULL,victim,TO_CHAR); break;
	case 1 : act("$N yawns.\r\n",     FALSE,ch,NULL,victim,TO_CHAR); break;
	default: act("$N stretches.\r\n", FALSE,ch,NULL,victim,TO_CHAR); break;
      }
      return;
    }

    if (mag_savingthrow(victim, savetype, 0))
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].duration = 4 + (GET_LEVEL(ch) / 4);
    af[0].bitvector = AFF_SLEEP;
    if (GET_POS(victim) > POS_SLEEPING)
    {
      send_to_char("You feel very sleepy...zzzzzz\r\n", victim);
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    clan_rel_inc(ch, victim, -1);
    break;

  case SPELL_STRENGTH: /////////////////////////////////// Strength
    if (GET_ADD(victim) == 100)
      return;
    af[0].location = APPLY_STR;
    af[0].duration = (GET_LEVEL(ch) / 2) + 4;
    af[0].modifier = 1 + (level > 18);
    accum_duration = TRUE;
    accum_affect = TRUE;
    to_vict = "You feel stronger!";
    clan_rel_inc(ch, victim, 1);
    break;
  case SPELL_SENSE_LIFE: ///////////////////////////////// Sense Life
    if (!victim)
      victim = ch;
    if (IS_AFFECTED(victim, AFF_SENSE_WOUNDS) && 
	!(affected_by_spell(victim, SPELL_SENSE_LIFE)))
    {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].duration = GET_LEVEL(ch)/2;
    af[0].bitvector = AFF_SENSE_LIFE;
    to_vict = "You feel your awareness improve.";
    to_room = "$n's awareness improves."; 
    accum_duration = FALSE;
    clan_rel_inc(ch, victim, 2);
    break;
  case SPELL_SENSE_WOUNDS: /////////////////////////////// Sense Wounds
    if (!victim)
      victim = ch;
    if (affected_by_spell(victim, SPELL_SENSE_WOUNDS))
      to_vict = "You feel more aware to the pain of people around you.";
    else if (IS_AFFECTED(victim, AFF_SENSE_WOUNDS))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_vict = "You feel aware to the pain of people around you.";
      to_room = "$n shudders momentarily.";
    }
    af[0].duration = (int)(GET_LEVEL(ch)/4);
    af[0].bitvector = AFF_SENSE_WOUNDS;
    accum_duration = FALSE;
    break;
  case SPELL_WATERWALK: ////////////////////////////////// WaterWalk
    if (affected_by_spell(victim, SPELL_WATERWALK))
      to_vict = "The webbing between your toes feels a little stronger.";
    else if (IS_AFFECTED(victim, AFF_WATERWALK))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else
      to_vict = "You feel webbing between your toes.";
    af[0].duration = 24;
    af[0].bitvector = AFF_WATERWALK;
    accum_duration = FALSE;
    break;
  case SPELL_SERPENT_SKIN: /////////////////////////////// SerpentSkin
    if(affected_by_spell(ch, SPELL_SERPENT_SKIN))
      to_vict = "Your skin begins to sparkle a little brighter!";
    else if (IS_AFFECTED(victim, AFF_REFLECT))
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else  {
      to_vict = "Your skin begins to sparkle!";
      to_room = "Shiny scales appear on $n's skin!";
    }
    if(mag_materials(ch,8400,0,0,1,1)) 
    {
      af[0].duration = 3;
      af[0].bitvector = AFF_REFLECT;
    } else {
      send_to_char("You seem to missing a major ingredient...\r\n", ch);
      return;
    }
    break;
  case SPELL_NOHASSLE: /////////////////////////////////// NoHassle
    af[0].duration = 10;
    af[0].bitvector = AFF_NOHASSLE;
    SET_BIT(PRF_FLAGS(ch),PRF_NOHASSLE);
    if (affected_by_spell(ch, SPELL_NOHASSLE))
      to_vict = "Your untouchability has been refreshed.";
    else if (IS_AFFECTED(victim, AFF_NOHASSLE)) 
    {
      send_to_char(NOEFFECT, ch);
      return;
    } else {
      to_vict = "You start to feel untouchable.";
      to_room = "$n starts to look untouchable.";
    }
    break;
  case SPELL_FLY: //////////////////////////////////////// Fly
    af[0].duration = 8;
    af[0].bitvector = AFF_FLY;
    
    if (IS_AFFECTED(victim, AFF_FLY))
      to_vict = "Your ability to fly is refreshed.";
    else {
      to_vict = "You begin to float off the ground.";
      to_room = "$n begins to float off the ground.";
    }
    break;
  case SPELL_PARALYZE: /////////////////////////////////// Paralyse
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(victim))
      return;
/* Artus> Below
    if (IS_AFFECTED(victim, AFF_PARALYZED))
    {
      send_to_char("They are already paralysed!.\r\n", ch);
      return;
    }
*/
    clan_rel_inc(ch, victim, -1);
    if (mag_savingthrow(victim, savetype, 0)) 
    {
      af[0].duration = 5;
      af[0].bitvector = AFF_PARALYZED;
      to_vict = "You feel your muscles tense up and lock.  You are paralyzed!!";
      to_room = "$n's legs completely freeze up.  $n looks pretty paralyzed!";
    } else
      return;

    break;

  case SPELL_WATERBREATHE: /////////////////////////////// WaterBreath
    if (IS_AFFECTED(victim,AFF_WATERBREATHE))
      to_vict = "Your ability to breathe underwater strengthens.";
    else 
    {
      to_vict = "A pair of magical gills appear on your neck.";
      to_room = "A pair of magical gills appear on $n's neck.";
    }

    af[0].duration = GET_LEVEL(ch)/2;
    af[0].bitvector = AFF_WATERBREATHE;
    break; 
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF_FLAGGED(victim, af[i].bitvector)) 
      {
	send_to_char(NOEFFECT, ch);
	return;
      }

  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  /* Artus> Going to redo this.
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect))
  {
    send_to_char(NOEFFECT, ch);
    return;
  } */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect))
    affect_from_char(victim, spellnum);


  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE)) {
      // modified for some classes to have extended duration
      if( (GET_CLASS(ch) == CLASS_DRUID && SECT(ch->in_room) == SECT_FOREST)
        || (GET_CLASS(ch) == CLASS_MASTER) ) {
           af[i].duration += (int)(af[i].duration * 0.10);
           af[i].modifier += (int)(af[i].modifier * 0.10);
      }
      affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);
    } 

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */

void perform_mag_groups(int level, struct char_data * ch,
			struct char_data * tch, int spellnum, int savetype)
{
  switch (spellnum) {
    case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL, savetype);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL);
    break;
  case SPELL_GROUP_SANCTUARY:
    mag_affects(level, ch, tch, SPELL_SANCTUARY, savetype);
    break; 
  }
}


/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */

void mag_groups(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (tch->in_room != ch->in_room)
      continue;
    if (!AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum, savetype);
  }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
    perform_mag_groups(level, ch, k, spellnum, savetype);
  perform_mag_groups(level, ch, ch, spellnum, savetype);
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented as of Circle 3.0.
 */

void mag_masses(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *tch_next;

  for (tch = world[ch->in_room].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    switch (spellnum) {
    }
  }
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
*/

void mag_areas(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *next_tch;
  const char *to_char = NULL, *to_room = NULL;

  if (ch == NULL)
    return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum) {
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room ="$n gracefully gestures and the earth begins to shake violently!";
    break;
  }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);
  

  for (tch = world[ch->in_room].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;
    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     */
    if (!IS_NPC(tch))
    {
      // No Hit Players
      if (!pk_allowed && !IS_NPC(ch))
	continue;
      // No Hit Imms -- Redundant
      // if (!LR_FAIL(tch, LVL_IMMORT))
      //   continue;
    } else {
      // No Hit Charmed Mobs.
      if (AFF_FLAGGED(tch, AFF_CHARM))
	continue;
    }
    if (tch == ch)
      continue;

    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    mag_damage(level, ch, tch, spellnum, 1);
  }
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {
  "\r\n",
  "$n makes a strange magical gesture; you feel a strong breeze!",
  "$n animates a corpse!",
  "$N appears from a cloud of thick blue smoke!",
  "$N appears from a cloud of thick green smoke!",
  "$N appears from a cloud of thick red smoke!",
  "$N disappears in a thick black cloud!"
  "As $n makes a strange magical gesture, you feel a strong breeze.",
  "As $n makes a strange magical gesture, you feel a searing heat.",
  "As $n makes a strange magical gesture, you feel a sudden chill.",
  "As $n makes a strange magical gesture, you feel the dust swirl.",
  "$n magically divides!",
  "$n animates a corpse!"
};

/*
 * Keep the \r\n because these use send_to_char.
 */
const char *mag_summon_fail_msgs[] = {
  "\r\n",
  "There are no such creatures.\r\n",
  "Uh oh...\r\n",
  "Oh dear.\r\n",
  "Oh shit!\r\n",
  "The elements resist!\r\n",
  "You failed.\r\n",
  "There is no corpse!\r\n"
};

/* These mobiles do not exist. */
#define MOB_MONSUM_I		130
#define MOB_MONSUM_II		140
#define MOB_MONSUM_III		150
#define MOB_GATE_I		160
#define MOB_GATE_II		170
#define MOB_GATE_III		180

/* Defined mobiles. - Where the fuck did u pull these numbers from Tali?? */
// #define MOB_ELEMENTAL_BASE	20	/* Only one for now. */
// #define MOB_CLONE		10
// #define MOB_ZOMBIE		11
// #define MOB_AERIALSERVANT	19
#define MOB_ELEMENTAL_BASE      110
#define MOB_AERIALSERVANT       10

void mag_summons(int level, struct char_data * ch, struct obj_data * obj,
		      int spellnum, int savetype)
{
  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int pfail = 0, msg = 0, fmsg = 0, handle_corpse = FALSE, counter = 0;
  mob_vnum mob_num;
  struct follow_type *fol; 

  if (ch == NULL)
    return;

  switch (spellnum)
  {
    case SPELL_CLONE:
      msg = 10;
      fmsg = number(2, 6);	// Random fail message.
      mob_num = MOB_CLONE;
      pfail = 8;
      // DM - Check follower list for number of clones - they always fol master
      for (fol = ch->followers; fol; fol = fol->next)
	if (IS_CLONE(fol->follower) && fol->follower->master == ch)
	  counter++;
      if (counter >= 2)
      {
	send_to_char("You dont wanna risk fiddling with your DNA any further...\r\n",                ch);
	return;
      }
      break;
    case SPELL_ANIMATE_DEAD:
      if ((obj == NULL) || (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) ||
          (!GET_OBJ_VAL(obj, 3)) || (GET_OBJ_VNUM(obj) == 22300))
      {
	act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
	return;
      }
      handle_corpse = TRUE;
      msg = 11;
      mob_num = MOB_ZOMBIE;
      break;
/******** stock 30bpl17 Animate Dead
    if (obj == NULL || !IS_CORPSE(obj)) {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    handle_corpse = TRUE;
    msg = 11;
    fmsg = number(2, 6);	// Random fail message. 
    mob_num = MOB_ZOMBIE;
    pfail = 10;	// 10% failure, should vary in the future. 
****************************************/

    default:
      return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM))
  {
    send_to_char("You are too giddy to have any followers!\r\n", ch);
    return;
  }
  if (number(0, 101) < pfail)
  {
    send_to_char(mag_summon_fail_msgs[fmsg], ch);
    return;
  }
  // for (i = 0; i < num; i++) {
  if (!(mob = read_mobile(mob_num, VIRTUAL)))
  {
    send_to_char("You don't quite remember how to make that creature.\r\n",
                 ch);
    return;
  }
  switch (spellnum)
  {
    case SPELL_ANIMATE_DEAD:
      sprintf(buf,"The zombie of %s is standing here, looking undead.\r\n",
              &obj->short_description[14]); 
      mob->player.long_descr = str_dup(buf);
      GET_LEVEL(mob) = MIN(LVL_CHAMP, GET_OBJ_COST(obj));
      GET_MAX_HIT(mob) = dice(10, GET_LEVEL(mob)) + GET_LEVEL(mob);
      GET_HIT(mob) = GET_MAX_HIT(mob);
      mob->char_specials.timer = number(1,5);
      break;
    case SPELL_CLONE:
      sprintf(buf,"%s %s &yis standing here.\r\n",
                      GET_NAME(ch),ch->player.title);
      mob->player.long_descr = str_dup(buf);
      mob->player.short_descr = str_dup(GET_NAME(ch));
      sprintf(buf,"If you didn't know any better you'd think this was the "
                  "real %s...\r\n",GET_NAME(ch));
      mob->player.description = str_dup(buf);
      GET_SEX(mob) = GET_SEX(ch);
      GET_LEVEL(mob) = MIN(LVL_CHAMP, GET_LEVEL(ch));
      if (AFF_FLAGGED(ch, AFF_INFRAVISION))
	SET_BIT(AFF_FLAGS(mob), AFF_INFRAVISION);
      if (AFF_FLAGGED(ch, AFF_SENSE_LIFE))
	SET_BIT(AFF_FLAGS(mob), AFF_SENSE_LIFE);
      if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
	SET_BIT(AFF_FLAGS(mob), AFF_DETECT_INVIS);
      GET_REAL_INT(mob) = GET_REAL_INT(ch);
      GET_REAL_WIS(mob) = GET_REAL_WIS(ch);
      GET_REAL_STR(mob) = GET_REAL_STR(ch);
      GET_REAL_DEX(mob) = GET_REAL_DEX(ch);
      GET_REAL_CON(mob) = GET_REAL_CON(ch);
      GET_REAL_CHA(mob) = GET_REAL_CHA(ch);
      GET_MAX_HIT(mob) = GET_MAX_HIT(ch);
      GET_MAX_MANA(mob) = GET_MAX_MANA(ch);
      GET_MAX_MOVE(mob) = GET_MAX_MOVE(ch);
      GET_HIT(mob) = GET_HIT(ch);
      GET_MANA(mob) = GET_MANA(ch);
      GET_MOVE(mob) = GET_MOVE(ch);
      GET_AFF_STR(mob) = GET_AFF_STR(ch);
      GET_AFF_DEX(mob) = GET_AFF_DEX(ch);
      GET_AFF_INT(mob) = GET_AFF_INT(ch);
      GET_AFF_WIS(mob) = GET_AFF_WIS(ch);
      GET_AFF_CON(mob) = GET_AFF_CON(ch);
      GET_AFF_CHA(mob) = GET_AFF_CHA(ch);
      // DM: Look - possibly lower these stats?
      GET_HITROLL(mob) = GET_HITROLL(ch);
      GET_DAMROLL(mob) = GET_DAMROLL(ch);
      GET_AC(mob) = GET_AC(ch);
      SET_BIT(MOB_FLAGS(ch), MOB_NOCHARM);
      break;
    default:
      return;
  }
  char_to_room(mob, ch->in_room);
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;
  SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
  if (spellnum == SPELL_CLONE)
  {   /* Don't mess up the proto with strcpy. */
    sprintf(buf, "clone c%s", GET_NAME(ch));
    mob->player.name = str_dup(buf);
    mob->player.short_descr = str_dup(GET_NAME(ch));
  }
  act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
  load_mtrigger(mob);
  add_follower(mob, ch);
//  }
  if (handle_corpse)
  {
    for (tobj = obj->contains; tobj; tobj = next_obj)
    {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob, __FILE__, __LINE__);
    }
    extract_obj(obj);
  }
}


void mag_points(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype)
{
  int hit = 0, move = 0, mana = 0, healingSkill = 0;
  double healingEffect = 1;

  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_HEALING_EFFICIENCY))
  {
	healingSkill = GET_SKILL(ch, SKILL_HEALING_EFFICIENCY);
  	healingEffect = 1.1;
  }
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_HEALING_MASTERY))
  {
	healingSkill = GET_SKILL(ch, SKILL_HEALING_MASTERY);
	healingEffect = 1.3;
  }

  if (victim == NULL)
    return;

  switch (spellnum)
  {
    case SPELL_CURE_LIGHT:
      hit = dice(1, 8) + 1 + (int)(level/4);
      if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_MASTER ||
          GET_CLASS(ch) == CLASS_PALADIN)
        hit += (int)(hit * 0.10);
      if (healingSkill && (number(0, 101) < healingSkill))
	hit = (int)(hit *healingEffect);
      send_to_char("You feel better.\r\n", victim);
      clan_rel_inc(ch, victim, 1);
      break;
    case SPELL_CURE_CRITIC:
      hit = dice(3, 8) + 3 + (int)(level/4);
      if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_MASTER ||
	  GET_CLASS(ch) == CLASS_PALADIN)
        hit += (int)(hit * 0.10);
      if (healingSkill && (number(0, 101) < healingSkill))
	hit = (int)(hit *healingEffect);
      send_to_char("You feel a lot better!\r\n", victim);
      clan_rel_inc(ch, victim, 2);
      break;
    case SPELL_HEAL:
      hit = 100 + dice(3, 8);
      if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_MASTER ||
          GET_CLASS(ch) == CLASS_PALADIN)
        hit += (int)(hit * 0.10);
      if (healingSkill && (number(0, 101) < healingSkill))
	hit = (int)(hit *healingEffect);
      send_to_char("A warm feeling floods your body.\r\n", victim);
      clan_rel_inc(ch, victim, 5);
      break;
    case SPELL_ADV_HEAL:
      hit = 200 + level;
      if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_MASTER ||
          GET_CLASS(ch) == CLASS_PALADIN)
        hit += (int)(hit * 0.10);
      if (healingSkill && (number(0, 101) < healingSkill))
	hit = (int)(hit *healingEffect);
      send_to_char("You feel your wounds heal!\r\n", victim);
      clan_rel_inc(ch, victim, 10);
      break;
    case SPELL_DIVINE_HEAL:
      hit = 300 + level;
      if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_MASTER ||
          GET_CLASS(ch) == CLASS_PALADIN )
        hit += (int)(hit * 0.10);
      if (healingSkill && (number(0, 101) < healingSkill))
	hit = (int)(hit *healingEffect);
      send_to_char("A divine feeling floods your body!\r\n",victim);
      clan_rel_inc(ch, victim, 20);
      break;
    case SPELL_REFRESH:
      move = 50 + dice(3, 8);
      if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_MASTER ||
          GET_CLASS(ch) == CLASS_PALADIN)
        move += (int)(move * 0.10);
      send_to_char("You feel refreshed.\r\n", victim);
      clan_rel_inc(ch, victim, 10);
      break;
    case SPELL_MANA:
      mana = 100 + dice(3, 8);
      if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_MASTER ||
	  GET_CLASS(ch) == CLASS_PALADIN)
        mana += (int)(mana * 0.10);
      send_to_char("Your skin tingles as magical energy surges through your body.\r\n", victim);
      break;
  }
  if (GET_HIT(victim) < GET_MAX_HIT(victim))
    GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + hit);
  if (GET_MOVE(victim) < GET_MAX_MOVE(victim))
    GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
  if (GET_MANA(victim) < GET_MAX_MANA(victim))
    GET_MANA(victim) = MIN(GET_MAX_MANA(victim), GET_MANA(victim) + mana);
  update_pos(victim);
}


void mag_unaffects(int level, struct char_data * ch, struct char_data * victim,
		        int spellnum, int type)
{
  int spell = 0;
  const char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum)
  {
    case SPELL_ADV_HEAL:
    case SPELL_DIVINE_HEAL:
    case SPELL_CURE_BLIND:
    case SPELL_HEAL:
      spell = SPELL_BLINDNESS;
      to_vict = "Your vision returns!";
      to_room = "There's a momentary gleam in $n's eyes.";
      if (!affected_by_spell(ch, spell))
	return;
      clan_rel_inc(ch, victim, 4);
      break;
    case SPELL_REMOVE_POISON:
      spell = SPELL_POISON;
      to_vict = "A warm feeling runs through your body!";
      to_room = "$n looks better.";
      clan_rel_inc(ch, victim, 2);
      break;
    case SPELL_REMOVE_CURSE:
      spell = SPELL_CURSE;
      to_vict = "You don't feel so unlucky.";
      clan_rel_inc(ch, victim, 3);
      break;
    case SPELL_GREATER_REMOVE_CURSE:
      spell = SPELL_CURSE;
      to_vict = "You feel cleansed of all curses.";
      clan_rel_inc(ch, victim, 6);
      break;
    case SPELL_REMOVE_PARA:
      spell = SPELL_PARALYZE;
      to_vict = "Your muscles suddenly relax you feel like u can move again!";
      to_room = "$n stumbles slightly as $s legs start working again.";
      clan_rel_inc(ch, victim, 4);
      break;
    default:
      basic_mud_log("SYSERR: unknown spellnum %d passed to mag_unaffects.",
	            spellnum);
      return;
  }

  // Remove curse from char and items char is wearing
  // TODO: verify if any checks are needed for special items ...
  if (spellnum == SPELL_GREATER_REMOVE_CURSE)
  {
    // remove from vict
    if (affected_by_spell(victim, spell))
      affect_from_char(victim, spell);
    // remove from victims equip list
    for (int i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(victim, i) && IS_OBJ_STAT(GET_EQ(victim, i), ITEM_NODROP))
        mag_alter_objs(0, ch, GET_EQ(victim, i), SPELL_REMOVE_CURSE, 0); 
    // remove for items in inventory
    for (struct obj_data *tobj=victim->carrying; tobj; tobj=tobj->next_content)
      if (IS_OBJ_STAT(tobj, ITEM_NODROP))
        mag_alter_objs(0, ch, tobj, SPELL_REMOVE_CURSE, 0); 
  } else {
    if (affected_by_spell(victim, spell))
      affect_from_char(victim, spell);
  }
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);

}


void mag_alter_objs(int level, struct char_data * ch, struct obj_data * obj,
		         int spellnum, int savetype)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum)
  {
    case SPELL_BLESS:
      if (!IS_OBJ_STAT(obj, ITEM_BLESS) &&
	  (GET_OBJ_WEIGHT(obj) <= 5 * GET_LEVEL(ch)))
      {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly.";
      }
      break;
    case SPELL_CURSE:
      if (!IS_OBJ_STAT(obj, ITEM_NODROP))
      {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (!IS_OBJ_STAT(obj, ITEM_MAGIC) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	  GET_OBJ_VAL(obj, 2)--;
	to_char = "$p briefly glows &rred&n.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!IS_OBJ_STAT(obj, ITEM_NOINVIS | ITEM_INVISIBLE))
      {
        SET_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3))
      {
	GET_OBJ_VAL(obj, 3) = 1;
	to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (IS_OBJ_STAT(obj, ITEM_NODROP))
      {
        REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          GET_OBJ_VAL(obj, 2)++;
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_REMOVE_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3))
      {
        GET_OBJ_VAL(obj, 3) = 0;
        to_char = "$p steams briefly.";
      }
      break;
  }

  if (to_char == NULL)
    send_to_char(NOEFFECT, ch);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);

}



void mag_creations(int level, struct char_data * ch, int spellnum)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum)
  {
    case SPELL_CREATE_FOOD:
      z = 10016;
      break;
    default:
      send_to_char("Spell unimplemented, it would seem.\r\n", ch);
      return;
  }

  if (!(tobj = read_object(z, VIRTUAL)))
  {
    send_to_char("I seem to have goofed.\r\n", ch);
    basic_mud_log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    return;
  }
  obj_to_char(tobj, ch, __FILE__, __LINE__);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);
}

