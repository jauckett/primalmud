/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"

extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct room_data *world;
extern struct spell_info_type spell_info[];
extern int max_exp_gain;
extern int max_exp_loss;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int free_rent;

// External Functions.
int has_stats_for_prac(struct char_data *ch, int skillnum, bool show);

/* local functions */
void check_autowiz(struct char_data * ch);

void Crash_rentsave(struct char_data *ch, int cost);
int level_exp(struct char_data *ch, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
void update_char_objects(struct char_data * ch);	/* handler.c */
void reboot_wizlists(void);
void punish_update(struct char_data * ch);		/* ARTUS */
void gain_clan_exp(struct char_data *ch, long amount);	/* ARTUS */

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

#if 0 // Artus> No longer required.
void dbg_managain_calc(struct char_data *ch, char *arg)
{
  int gain, base, dyear, dint, dwis, dclass, dlevel, dage;
  char arg1[MAX_INPUT_LENGTH]="";
  int parse_class(char *arg);
  extern const char *class_abbrevs[];

  if (*arg)
    skip_spaces(&arg);

  // Set Defaults.
  dyear = age(ch)->year;
  dint = GET_INT(ch);
  dwis = GET_WIS(ch);
  dlevel = GET_LEVEL(ch);
  dage = GET_AGE(ch);
  dclass = GET_CLASS(ch);
  if (*arg)
  {
    arg = one_argument(arg, arg1);
    if (!is_number(arg1))
    {
      send_to_char("Syntax: debug managain [level] [classabbrev] [age] [int] [wis]\r\n", ch);
      return;
    }
    dlevel = atoi(arg1);
    if (*arg)
      skip_spaces(&arg);
    if (*arg)
    {
      arg = one_argument(arg, arg1);
      dclass = parse_class(arg1);
      if (dclass == CLASS_UNDEFINED)
	dclass = GET_CLASS(ch);
      arg = one_argument(arg, arg1);
      if (*arg)
	skip_spaces(&arg);
      if (is_number(arg1))
      {
	dage = atoi(arg1);
	if (*arg)
	  skip_spaces(&arg);
	if (*arg)
	{
	  arg = one_argument(arg, arg1);
	  if (is_number(arg1))
	  {
	    dint = atoi(arg1);
	    if (*arg)
	      skip_spaces(&arg);
	    arg = one_argument(arg, arg1);
	    if (is_number(arg1))
	      dwis = atoi(arg1);
	  }
	}
      }
    }
  }

  sprintf(buf, "[DBG] Level: %d, Class: %s, Age: %d, Int: %d, Wis: %d\r\n",
          dlevel, class_abbrevs[dclass], dage, dint, dwis);
  send_to_char(buf, ch);
  
  base = dint + dwis;
  sprintf(buf, "[DBG] Int + Wis: %d", base);

  if (base < 25)
    base = 5 + (int)(dlevel / 2);
  else if (base < 30)
    base = 8 + (int)(dlevel * 0.75);
  else if (base < 35)
    base = 10 + dlevel;
  else if (base == 35)
    base = 12 + (int)(dlevel * 1.25);
  else if (base == 36)
    base = 15 + (int)(dlevel * 1.5);
  else if (base == 37)
    base = 17 + (int)(dlevel * 1.75);
  else
    base = 20 + (dlevel << 1);

  sprintf(buf + strlen(buf), ", Base: %d, Year: %d", base, dyear);

  if (dage < 15)
    base = (int)(base * 0.75);
  else if (dage < 25);
  else if (dage < 30) 
    base = (int)(base * 1.5);
  else if (dage < 45)
    base <<= 1;
  else if (dage < 55);
  else
    base = (int)(base * 0.75);
  
  sprintf(buf + strlen(buf), ", Post Age: %d\r\n", base);
  switch (dclass)
  {
    case CLASS_WARRIOR:
      gain = (int)(base * 0.75);
      break;
    case CLASS_MAGIC_USER:
    case CLASS_CLERIC:
      gain = (base << 1);
      break;
    case CLASS_DRUID:
    case CLASS_MASTER:
      gain = (int)(base * 2.5);
      break;
    case CLASS_PRIEST:
    case CLASS_BATTLEMAGE:
    case CLASS_SPELLSWORD:
    case CLASS_PALADIN:
      gain = (int)(base * 1.5);
      break;
    case CLASS_NIGHTBLADE:
      gain = (int)(base * 0.5);
      break;
    default: // Thief.
      gain = base;
      break;
  }
  sprintf(buf + strlen(buf), "[DBG] Result: %d\r\n", gain);
  send_to_char(buf, ch);
}
#endif

/* Artus> Lets try improving on this. */
int mana_gain(struct char_data *ch)
{
  int gain, base, chyear;

  if (IS_NPC(ch))
    return GET_LEVEL(ch);
  
  base = GET_INT(ch) + GET_WIS(ch);

  if (base < 25)
    base = 5 + (int)(GET_LEVEL(ch) / 2);
  else if (base < 30)
    base = 8 + (int)(GET_LEVEL(ch) * 0.75);
  else if (base < 35)
    base = 10 + GET_LEVEL(ch);
  else if (base == 35)
    base = 12 + (int)(GET_LEVEL(ch) * 1.25);
  else if (base == 36)
    base = 15 + (int)(GET_LEVEL(ch) * 1.5);
  else if (base == 37)
    base = 17 + (int)(GET_LEVEL(ch) * 1.75);
  else
    base = 20 + GET_LEVEL(ch) << 1;

  chyear = age(ch)->year;

  if (chyear < 15)
    base = (int)(base * 0.75);
  else if (chyear < 25);
  else if (chyear < 30) 
    base = (int)(base * 1.5);
  else if (chyear < 45)
    base <<= 1;
  else if (chyear < 55);
  else
    base = (int)(base * 0.75);
  
  switch (GET_CLASS(ch))
  {
    case CLASS_WARRIOR:
      gain = (int)(base * 0.75);
      break;
    case CLASS_MAGIC_USER:
    case CLASS_CLERIC:
      gain = (base << 1);
      break;
    case CLASS_DRUID:
    case CLASS_MASTER:
      gain = (int)(base * 2.5);
      break;
    case CLASS_PRIEST:
    case CLASS_BATTLEMAGE:
    case CLASS_SPELLSWORD:
    case CLASS_PALADIN:
      gain = (int)(base * 1.5);
      break;
    case CLASS_NIGHTBLADE:
      gain = (int)(base * 0.5);
      break;
    default: // Thief
      gain = base;
      break;
  }

  // Some restrictions.
  gain = MAX(GET_LEVEL(ch) + 5, 
             MIN((int)(GET_MAX_MANA(ch) * 0.125), gain));

  if (GET_POS(ch) == POS_SLEEPING)			// Sleeping
  {
    if ((GET_SKILL(ch, SKILL_CAMPING) &&
	  (SECT(ch->in_room) == SECT_FOREST ||
	   SECT(ch->in_room) == SECT_MOUNTAIN ||
	   SECT(ch->in_room) == SECT_FIELD ||
	   SECT(ch->in_room) == SECT_HILLS)) &&
	has_stats_for_prac(ch, SKILL_CAMPING, false))
      gain *= 3;
    else
      gain <<= 1;
  } else if (GET_POS(ch) == POS_RESTING) {		// Resting
    if ((GET_SKILL(ch, SKILL_CAMPING) &&
	  (SECT(ch->in_room) == SECT_FOREST ||
	   SECT(ch->in_room) == SECT_MOUNTAIN ||
	   SECT(ch->in_room) == SECT_FIELD ||
	   SECT(ch->in_room) == SECT_HILLS)) &&
	has_stats_for_prac(ch, SKILL_CAMPING, false))
      gain <<= 1;
    else
      gain = (int)(gain * 1.5);
  } else if (GET_POS(ch) == POS_SITTING) {		// Sitting
    if (char_affected_by_timer(ch, TIMER_MEDITATE))
      gain += MAX(5, (int)APPLY_SPELL_EFFEC(ch, SKILL_MEDITATE, (gain << 1)));
    if (char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
      gain += MAX(10, (int)APPLY_SPELL_EFFEC(ch, SKILL_HEAL_TRANCE, gain * 3));
  } else if (GET_POS(ch) == POS_FIGHTING) {		// Fighting
    gain >>= 2;
  }

  // Hunger/Thirst
  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 1;

  // Double Regen
  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
    gain <<= 1;
  // Half Regen
  else if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
    gain >>= 1;

  // Low Regen Punishment.
  if (PUN_FLAGGED(ch, PUN_LOWREGEN))
    gain >>= 1;

  // Poison.
  if (AFF_FLAGGED(ch, AFF_POISON))
    gain >>= 2;

  return (gain);
}

#if 0 // Artus> Old Routine.
/* manapoint gain pr. game hour */
int old_mana_gain(struct char_data * ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    //gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);
    gain = graf(age(ch)->year, 7, 8, 9, 10, 8, 7, 7);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* WIS adjustment */
    if (GET_AFF_WIS(ch) > 17)
      gain += GET_AFF_WIS(ch) - 17;

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      if (GET_SKILL(ch, SKILL_CAMPING) && 
		(SECT(ch->in_room) == SECT_FOREST || 
                 SECT(ch->in_room) == SECT_MOUNTAIN || 
                 SECT(ch->in_room) == SECT_FIELD || 
                 SECT(ch->in_room) == SECT_HILLS) )
	gain *= 3;
      else
	gain <<= 1;
      break;
    case POS_RESTING:
      if (GET_SKILL(ch, SKILL_CAMPING) && 
		(SECT(ch->in_room) == SECT_FOREST || 
                 SECT(ch->in_room) == SECT_MOUNTAIN || 
                 SECT(ch->in_room) == SECT_FIELD || 
                 SECT(ch->in_room) == SECT_HILLS) )
         gain += gain;
      else
	 gain += (gain / 2);	/* 1.5* gain */
      break;
    case POS_SITTING:
      if (char_affected_by_timer(ch, TIMER_MEDITATE))
	gain += MAX(5, (int)(gain * 2 * (GET_SKILL(ch, SKILL_MEDITATE) / 100) * (SPELL_EFFEC(ch, SKILL_MEDITATE) / 100)));
      if (!IS_NPC(ch) && char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
	gain += MAX(10, (int)(gain * 3 * (GET_SKILL(ch, SKILL_HEAL_TRANCE) / 100) * (SPELL_EFFEC(ch, SKILL_HEAL_TRANCE) / 100)));

      if (GET_SKILL(ch, SKILL_CAMPING) && 
		(SECT(ch->in_room) == SECT_FOREST || 
                 SECT(ch->in_room) == SECT_MOUNTAIN || 
                 SECT(ch->in_room) == SECT_FIELD || 
                 SECT(ch->in_room) == SECT_HILLS) )
	gain += (gain /2);
      else
        gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_FIGHTING:
      gain = 1;
    }

    /* JA make gain percentage based * /
    Artus> I don't think this works for me...
    gain = MAX((gain * GET_MAX_MANA(ch)) / 100, 10); 
    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
      gain *= 2;
    */
    if (GET_CLASS(ch) <= CLASS_WARRIOR)
      gain = MAX((gain * (GET_MAX_MANA(ch) / 100)), 10);
    else if (GET_CLASS(ch) < CLASS_MASTER)
      gain = MAX((gain * (GET_MAX_MANA(ch) / 85)), 20);
    else
      gain = MAX((gain * (GET_MAX_MANA(ch) / 70)), 30);

    if (!(IS_MAGIC_USER(ch) || IS_CLERIC(ch)))
      gain >>= 1;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain >>= 2;

    if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
      gain = (gain * 3) / 2;

    if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
      gain >>= 1;

    if (PUN_FLAGGED(ch, PUN_LOWREGEN)) /* ARTUS - Low Regen Punishment */
      gain >>= 1;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain >>= 2;

  return (gain);
}
#endif

int hit_gain(struct char_data *ch)
{
  int gain, base, chyear;

  if (IS_NPC(ch))
    return GET_LEVEL(ch);
  
  base = GET_STR(ch) + GET_CON(ch);

  if (base < 25)
    base = 5 + (int)(GET_LEVEL(ch) / 2);
  else if (base < 30)
    base = 8 + (int)(GET_LEVEL(ch) * 0.75);
  else if (base < 35)
    base = 10 + GET_LEVEL(ch);
  else if (base == 35)
    base = 12 + (int)(GET_LEVEL(ch) * 1.25);
  else if (base == 36)
    base = 15 + (int)(GET_LEVEL(ch) * 1.5);
  else if (base == 37)
    base = 17 + (int)(GET_LEVEL(ch) * 1.75);
  else
    base = 20 + GET_LEVEL(ch) << 1;

  chyear = age(ch)->year;

  if (chyear < 14)
    base = (int)(base * 1.75);
  else if (chyear < 26)
    base <<= 1;
  else if (chyear < 33) 
    base = (int)(base * 1.75);
  else if (chyear < 40)
    base = (int)(base * 1.5);
  else if (chyear < 50);
  else
    base = (int)(base * 0.75);
  
  switch (GET_CLASS(ch))
  {
    case CLASS_WARRIOR:
    case CLASS_MASTER:
      gain = (base << 1);
      break;
    case CLASS_MAGIC_USER:
    case CLASS_CLERIC:
      gain = (int)(base * 0.75);
      break;
    case CLASS_DRUID:
      gain = (int)(base * 0.5);
      break;
      break;
    case CLASS_PRIEST:
    case CLASS_BATTLEMAGE:
    case CLASS_SPELLSWORD:
    case CLASS_PALADIN:
      gain = (int)(base * 1.5);
      break;
    case CLASS_NIGHTBLADE:
      gain = (int)(base * 2.5);
      break;
    default: // Thief
      gain = base;
      break;
  }

  // Some restrictions.
  gain = MAX(GET_LEVEL(ch) + 5, 
             MIN((int)(GET_MAX_HIT(ch) * 0.125), gain));

  if (GET_POS(ch) == POS_SLEEPING)			// Sleeping
  {
    if ((GET_SKILL(ch, SKILL_CAMPING) &&
	  (SECT(ch->in_room) == SECT_FOREST ||
	   SECT(ch->in_room) == SECT_MOUNTAIN ||
	   SECT(ch->in_room) == SECT_FIELD ||
	   SECT(ch->in_room) == SECT_HILLS)) &&
	has_stats_for_prac(ch, SKILL_CAMPING, false))
      gain *= 3;
    else
      gain <<= 1;
  } else if (GET_POS(ch) == POS_RESTING) {		// Resting
    if ((GET_SKILL(ch, SKILL_CAMPING) &&
	  (SECT(ch->in_room) == SECT_FOREST ||
	   SECT(ch->in_room) == SECT_MOUNTAIN ||
	   SECT(ch->in_room) == SECT_FIELD ||
	   SECT(ch->in_room) == SECT_HILLS)) &&
	has_stats_for_prac(ch, SKILL_CAMPING, false))
      gain <<= 1;
    else
      gain = (int)(gain * 1.5);
  } else if (GET_POS(ch) == POS_SITTING) {		// Sitting
    if (char_affected_by_timer(ch, TIMER_MEDITATE))
      gain += MAX(5, (int)APPLY_SPELL_EFFEC(ch, SKILL_MEDITATE, (gain << 1)));
    if (char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
      gain += MAX(10, (int)APPLY_SPELL_EFFEC(ch, SKILL_HEAL_TRANCE, gain * 3));
  } else if (GET_POS(ch) == POS_FIGHTING) {		// Fighting
    gain >>= 2;
  }

  // Hunger/Thirst
  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 1;

  // Double Regen
  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
    gain <<= 1;
  // Half Regen
  else if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
    gain >>= 1;

  // Low Regen Punishment.
  if (PUN_FLAGGED(ch, PUN_LOWREGEN))
    gain >>= 1;

  // Poison.
  if (AFF_FLAGGED(ch, AFF_POISON))
    gain >>= 2;

  return (gain);
}

#if 0 // Artus> Old Routine.
/* Hitpoint gain pr. game hour */
int old_hit_gain(struct char_data * ch)
{
  int gain;

  if (IS_NPC(ch))
  {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {

  //gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);
    gain = graf(age(ch)->year, 4, 5, 6, 7, 6, 5, 4);

  /* CON adjustment */
    if (GET_AFF_CON(ch) > 17)
      gain += GET_AFF_CON(ch) - 18;

    /* Class/Level calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */

    switch (GET_POS(ch))
    {
      case POS_SLEEPING:
	if (GET_SKILL(ch, SKILL_CAMPING) && 
		  (SECT(ch->in_room) == SECT_FOREST || 
		   SECT(ch->in_room) == SECT_MOUNTAIN || 
		   SECT(ch->in_room) == SECT_FIELD || 
		   SECT(ch->in_room) == SECT_HILLS) )
	  gain += gain;
	else
	  gain += (gain / 2);	/* Divide by 2 */
	break;
      case POS_RESTING:
	if (GET_SKILL(ch, SKILL_CAMPING) && 
		  (SECT(ch->in_room) == SECT_FOREST || 
		   SECT(ch->in_room) == SECT_MOUNTAIN || 
		   SECT(ch->in_room) == SECT_FIELD || 
		   SECT(ch->in_room) == SECT_HILLS) )
	  gain += (gain /2);
	else
	  gain += (gain / 4);	/* Divide by 4 */
	break;
      case POS_SITTING:
	if (!IS_NPC(ch) && char_affected_by_timer(ch, TIMER_MEDITATE))
	  gain += MAX(5, (int)(gain * 2 * (GET_SKILL(ch, SKILL_MEDITATE) / 100) * (SPELL_EFFEC(ch, SKILL_MEDITATE) / 100)));
	if (!IS_NPC(ch) && char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
	  gain += MAX(10, (int)(gain * 3 * (GET_SKILL(ch, SKILL_HEAL_TRANCE) / 100) * (SPELL_EFFEC(ch, SKILL_HEAL_TRANCE) / 100)));	

	if (GET_SKILL(ch, SKILL_CAMPING) && 
		  (SECT(ch->in_room) == SECT_FOREST || 
		   SECT(ch->in_room) == SECT_MOUNTAIN || 
		   SECT(ch->in_room) == SECT_FIELD || 
		   SECT(ch->in_room) == SECT_HILLS) )
	  gain += (gain / 4);
	else
	  gain += (gain / 8);	/* Divide by 8 */


	break;
    }

    /* Artus> This is wrong too.. 
    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
      gain /= 2;	/ * Ouch. */
    if (IS_WARRIOR(ch))
      gain <<= 1;
    else if (IS_THIEF(ch))
      gain = (int)(gain * 1.5);

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain >>= 2;

    /* JA make gain percent based * /
     * Artus> Lets make this a little nicer.
    gain = MAX((gain * GET_MAX_HIT(ch)) / 100, 10); */

    if (GET_CLASS(ch) <= CLASS_WARRIOR)
      gain = MAX((gain * (GET_MAX_HIT(ch) / 100)), 10);
    else if (GET_CLASS(ch) < CLASS_MASTER)
      gain = MAX((gain * (GET_MAX_HIT(ch) / 85)), 20);
    else
      gain = MAX((gain * (GET_MAX_HIT(ch) / 70)), 30);

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain >>= 2;

    if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
      gain = (gain * 3) / 2;

    if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
      gain >>= 1;

    if (PUN_FLAGGED(ch, PUN_LOWREGEN)) /* ARTUS - Low Regen Punishment */
      gain >>= 1;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain >>= 2;

  return (gain);
}
#endif

int move_gain(struct char_data *ch)
{
  int gain, base, chyear;

  if (IS_NPC(ch))
    return GET_LEVEL(ch);
  
  base = GET_DEX(ch) + GET_STR(ch);

  if (base < 25)
    base = 2 + (int)(GET_LEVEL(ch) * 0.2);
  else if (base < 30)
    base = 3 + (int)(GET_LEVEL(ch) * 0.3);
  else if (base < 35)
    base = 5 + (int)(GET_LEVEL(ch) * 0.4);
  else if (base == 35)
    base = 7 + (int)(GET_LEVEL(ch) * 0.5);
  else if (base == 36)
    base = 10 + (int)(GET_LEVEL(ch) * 0.6);
  else if (base == 37)
    base = 12 + (int)(GET_LEVEL(ch) * 0.7);
  else
    base = 15 + (int)(GET_LEVEL(ch) * 0.8);

  chyear = age(ch)->year;

  if (chyear < 14)
    base = (int)(base * 0.75);
  else if (chyear < 20)
    base <<= 1;
  else if (chyear < 28) 
    base = (int)(base * 1.75);
  else if (chyear < 35)
    base = (int)(base * 1.5);
  else if (chyear < 45);
  else
    base = (int)(base * 0.75);
  
  switch (GET_CLASS(ch))
  {
    case CLASS_WARRIOR:
    case CLASS_PRIEST:
    case CLASS_BATTLEMAGE:
    case CLASS_SPELLSWORD:
    case CLASS_PALADIN:
      gain = (int)(base * 1.25);
      break;
    case CLASS_THIEF:
    case CLASS_NIGHTBLADE:
    case CLASS_MASTER:
      gain = (int)(base * 1.5);
      break;
    default: // Druid, Magic User, Cleric
      gain = base;
      break;
  }

  // Some restrictions.
  gain = MAX(GET_LEVEL(ch) + 5, 
             MIN((int)(GET_MAX_MOVE(ch) * 0.125), gain));

  if (GET_POS(ch) == POS_SLEEPING)			// Sleeping
  {
    if ((GET_SKILL(ch, SKILL_CAMPING) &&
	  (SECT(ch->in_room) == SECT_FOREST ||
	   SECT(ch->in_room) == SECT_MOUNTAIN ||
	   SECT(ch->in_room) == SECT_FIELD ||
	   SECT(ch->in_room) == SECT_HILLS)) &&
	has_stats_for_prac(ch, SKILL_CAMPING, false))
      gain *= 3;
    else
      gain <<= 1;
  } else if (GET_POS(ch) == POS_RESTING) {		// Resting
    if ((GET_SKILL(ch, SKILL_CAMPING) &&
	  (SECT(ch->in_room) == SECT_FOREST ||
	   SECT(ch->in_room) == SECT_MOUNTAIN ||
	   SECT(ch->in_room) == SECT_FIELD ||
	   SECT(ch->in_room) == SECT_HILLS)) &&
	has_stats_for_prac(ch, SKILL_CAMPING, false))
      gain <<= 1;
    else
      gain = (int)(gain * 1.5);
  } else if (GET_POS(ch) == POS_SITTING) {		// Sitting
    if (char_affected_by_timer(ch, TIMER_MEDITATE))
      gain += MAX(5, (int)APPLY_SPELL_EFFEC(ch, SKILL_MEDITATE, (gain << 1)));
    if (char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
      gain += MAX(10, (int)APPLY_SPELL_EFFEC(ch, SKILL_HEAL_TRANCE, gain * 3));
  } else if (GET_POS(ch) == POS_FIGHTING) {		// Fighting
    gain >>= 2;
  }

  // Hunger/Thirst
  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 1;

  // Double Regen
  if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
    gain <<= 1;
  // Half Regen
  else if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
    gain >>= 1;

  // Low Regen Punishment.
  if (PUN_FLAGGED(ch, PUN_LOWREGEN))
    gain >>= 2;

  // Poison.
  if (AFF_FLAGGED(ch, AFF_POISON))
    gain >>= 2;

  return (gain);
}

#if 0 // Artus> Old Routine.
/* move gain pr. game hour */
int old_move_gain(struct char_data * ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    //gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);
    gain = graf(age(ch)->year, 5, 7, 10, 15, 14, 12, 9);

    /* Class/Level calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) 
    {
      case POS_SLEEPING:
	if (GET_SKILL(ch, SKILL_CAMPING) && 
		  (SECT(ch->in_room) == SECT_FOREST || 
		   SECT(ch->in_room) == SECT_MOUNTAIN || 
		   SECT(ch->in_room) == SECT_FIELD || 
		   SECT(ch->in_room) == SECT_HILLS) )
	  gain += gain;
	else
	  gain += (gain / 2);	/* Divide by 2 */
	break;
      case POS_RESTING:
	if (GET_SKILL(ch, SKILL_CAMPING) && 
		  (SECT(ch->in_room) == SECT_FOREST || 
		   SECT(ch->in_room) == SECT_MOUNTAIN || 
		   SECT(ch->in_room) == SECT_FIELD || 
		   SECT(ch->in_room) == SECT_HILLS) )
	  gain += (gain /2);
	else
	  gain += (gain / 4);	/* Divide by 4 */
	break;
      case POS_SITTING:
	if (!IS_NPC(ch) && char_affected_by_timer(ch, TIMER_MEDITATE))
	  gain += MAX(5, (int)(gain * 2 * (GET_SKILL(ch, SKILL_MEDITATE) / 100) * (SPELL_EFFEC(ch, SKILL_MEDITATE) / 100)));
	if (!IS_NPC(ch) && char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
	  gain += MAX(10, (int)(gain * 2 * (GET_SKILL(ch, SKILL_HEAL_TRANCE) / 100) * (SPELL_EFFEC(ch, SKILL_HEAL_TRANCE) / 100)));

	if (GET_SKILL(ch, SKILL_CAMPING) && 
		  (SECT(ch->in_room) == SECT_FOREST || 
		   SECT(ch->in_room) == SECT_MOUNTAIN || 
		   SECT(ch->in_room) == SECT_FIELD || 
		   SECT(ch->in_room) == SECT_HILLS) )
	  gain += (gain / 4);
	else
	  gain += (gain / 8);	/* Divide by 8 */
	break;
      case POS_FIGHTING:
	gain = 1;
    }

    /* JA make gain percent based * /
     * Artus> Interferes again :o)..
    gain = MAX((gain * GET_MAX_MOVE(ch)) / 100, 10); */
    if (GET_CLASS(ch) <= CLASS_WARRIOR)
      gain = MAX((gain * (GET_MAX_MOVE(ch) / 100)), 10);
    else if (GET_CLASS(ch) < CLASS_MASTER)
      gain = MAX((gain * (GET_MAX_MOVE(ch) / 85)), 20);
    else
      gain = MAX((gain * (GET_MAX_MOVE(ch) / 70)), 30);

    if (IS_THIEF(ch))
      gain = (int)(gain * 1.7);
    else if (IS_WARRIOR(ch))
      gain = (int)(gain * 1.25);

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain >>= 2;
    
    if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_2))
      gain = (gain * 3) / 2;

    if (IS_SET(world[ch->in_room].room_flags,ROOM_REGEN_HALF))
      gain >>= 1;

    if (PUN_FLAGGED(ch, PUN_LOWREGEN)) /* ARTUS - Low Regen Punishment */
      gain >>= 1;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain >>= 2;

  return (gain);
}
#endif

void set_title(struct char_data * ch, char *title)
{
  if (title == NULL) {
    if (GET_TITLE(ch) != NULL)
      free(GET_TITLE(ch));

    GET_TITLE(ch) = str_dup("");
    return;
  }

  if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  GET_TITLE(ch) = str_dup(title);
}


// Dont use unless u fix up the code in util/autowiz.c <- DM
// (causes a crash)
void check_autowiz(struct char_data * ch)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (use_autowiz && GET_LEVEL(ch) >= LVL_IMPL)
  {
    char xbuf[128];
#if defined(CIRCLE_UNIX)
    sprintf(xbuf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMPL, IMMLIST_FILE, (int) getpid());
#elif defined(CIRCLE_WINDOWS)
    sprintf(xbuf, "autowiz %d %s %d %s", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMPL, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    mudlog("Initiating autowiz.", CMP, LVL_IMPL, FALSE);
    system(xbuf);
    reboot_wizlists();
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

/* Artus> Changed to int, will return the actual amount gained. */
int gain_exp(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int newgain;
  int num_levels = 0;
  char buf[128];

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1) || !CAN_LEVEL(ch)))
    return 0;

  if (IS_NPC(ch)) 
  {
    GET_EXP(ch) += gain;
    return gain;
  }

  // The famous exp cap :) TODO: decide whether we want to lessen this, at
  // least for remort characters...
  if (gain > 0) 
  {
    // DM: apply the exp modifier ...
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch)) 
    {
      sprintf(buf, "raw exp = %d", gain); 
      send_to_char(buf, ch);
    }
#endif
    gain = (int)((float)(gain * (1 / GET_MODIFIER(ch))));
    newgain = MIN(GET_LEVEL(ch) * 10000, gain);
    if ((newgain < gain) && (GET_CLASS(ch) > CLASS_WARRIOR))
    {
      if (GET_CLASS(ch) == CLASS_MASTER)
	newgain = MIN(GET_LEVEL(ch) * 15000, gain);
      else
	newgain = MIN(GET_LEVEL(ch) * 12500, gain);
    }

    /* Artus: Increase modifier for remorts.
    if (!IS_NPC(ch) && (GET_CLASS(ch) > CLASS_WARRIOR) && (newgain < gain))
      newgain = (int)(newgain * 1.5); 
     * Artus> Why??? */
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch)) 
    {
      sprintf(buf, "gain = %d, newgain = %d\r\n", gain, newgain); 
      send_to_char(buf, ch);
    }
#endif
    gain_clan_exp(ch, gain); /* ARTUS - Give clan exp.. */
    if (newgain < gain)
    {
      sprintf(buf,"You receive less experience (%d) due to help.\n",newgain);
      send_to_char(buf,ch);
      gain=newgain;
    } 

//    gain = MIN(max_exp_gain, gain);	/* put a cap on the max gain per kill */
    GET_EXP(ch) += gain;

   /* DM_exp - change for new exp system */
    GET_EXP(ch)=MIN(GET_EXP(ch),level_exp((ch),LVL_IMMORT-1)*2);
 
    while (CAN_LEVEL(ch) && (GET_EXP(ch) >= level_exp((ch),GET_LEVEL(ch)))) 
    {
  //    send_to_char("You rise a level!\r\n", ch);
      GET_LEVEL(ch) += 1;
      num_levels++;
  /* DM_exp - reset the exp */
      GET_EXP(ch) = GET_EXP(ch)-level_exp((ch),GET_LEVEL(ch)-1);
      advance_level(ch);
      is_altered = TRUE;
    }
    if (is_altered) 
    {
      if (num_levels == 1)
      {
        send_to_char("You rise a level!\r\n", ch);
      } else {
	sprintf(buf, "You rise %d levels!\r\n", num_levels);
	send_to_char(buf, ch);
      }
    }
  } else if (gain < 0) {
    /* CAP removed bm 3/95 */
    /* gain = MAX(-max_exp_loss, gain);*/       /* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
  return (gain);
}

/* DM_exp - changed to take the number of levels advanced */  
void gain_exp_regardless(struct char_data * ch, int /*gain*/ newlevel)
{
  int is_altered = FALSE;
  int num_levels = 0;

  GET_EXP(ch) = 0; 

//  GET_EXP(ch) += gain;
//  if (GET_EXP(ch) < 0)
//    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) 
  {
    if (newlevel > LVL_IMPL)
      newlevel = LVL_IMPL;
    while (LR_FAIL(ch, LVL_IMPL) && GET_LEVEL(ch) < newlevel)
    { 
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
//      sprintf(buf, "%s advanced %d level%s to level %d.",
//		GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
//		GET_LEVEL(ch));
//      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      if (num_levels == 1)
        send_to_char("You rise a level!\r\n", ch);
      else {
	sprintf(buf, "You rise %d levels!\r\n", num_levels);
	send_to_char(buf, ch);
      }
//      set_title(ch, NULL);
      //check_autowiz(ch);
    }
  }
}


void gain_condition(struct char_data * ch, int condition, int value)
{
  void perform_eat(struct char_data *ch, struct obj_data *food, int subcmd);
  bool intoxicated;
  int i, check_cond=0, move_val;
  struct obj_data *food, *fewd;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
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
      if (IS_OBJ_STAT(ch->equipment[i],check_cond)) {
        /* to make sure they dont keep getting warnings */
        if (GET_COND(ch, condition)==0)
          GET_COND(ch, condition)=1; 
        return;
      }

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition)
  {
    case FULL:
      send_to_char("You are hungry.\r\n", ch);
      // Artus> AutoEat.
      // Possible TODO: If Detect Poison, Skip Bad Food...
      if (!(EXT_FLAGGED(ch, EXT_AUTOEAT) && (ch->carrying)))
	return;
      if (GET_POS(ch) < POS_RESTING) // No eating while sleeping :o)
	return;
      move_val = 5;
      if (FIGHTING(ch)) 
	move_val += 5;
      if (GET_MOVE(ch) < move_val)
	return;
      for (food = ch->carrying; food; food=food->next_content)
      {
	fewd = NULL;
	if (!CAN_SEE_OBJ(ch, food))
	  continue;
	if (GET_OBJ_TYPE(food) == ITEM_FOOD)
	{
	  fewd = food;
	  break;
	}
	if ((GET_OBJ_TYPE(food) == ITEM_CONTAINER) &&
	    (!OBJVAL_FLAGGED(food, CONT_CLOSED)) && (food->contains) &&
	    (GET_MOVE(ch) >= move_val + 5))
	  for (fewd = food->contains; fewd; fewd = fewd->next_content)
	    if ((GET_OBJ_TYPE(fewd) == ITEM_FOOD) && CAN_SEE_OBJ(ch, fewd))
	    {
	      sprintf(buf, "You get &5%s&n from &5%s&n.\r\n", 
		      OBJS(fewd, ch), OBJS(food, ch));
	      send_to_char(buf, ch);
	      move_val += 5;
	      break;
	    }
	if ((fewd) && (GET_OBJ_TYPE(fewd) == ITEM_FOOD))
	  break;
      }
      if (!(fewd))
	return;
      GET_MOVE(ch) -= move_val;
      perform_eat(ch, fewd, SCMD_EAT);
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

#define IDLE_ROOM_VNUM  1102

void check_idling(struct char_data * ch)
{
  extern room_rnum r_mortal_start_room;

  // DM - changed idle_max_level to LVL_OWNER, just added the angel line here
  if (++(ch->char_specials.timer) > idle_void && LR_FAIL(ch, LVL_IS_GOD))
  {
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE)
    {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch))
      {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
      save_char(ch, NOWHERE);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, real_room(IDLE_ROOM_VNUM));
    } else if (ch->char_specials.timer > idle_rent_time)
    {
      if (ch->in_room != NOWHERE)
	char_from_room(ch);
      if (GET_WAS_IN(ch) != NOWHERE)
	char_to_room(ch, GET_WAS_IN(ch));
      else 
	char_to_room(ch, real_room(r_mortal_start_room));
      sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
      if (ch->desc)
      {
	mudlog(buf, NRM, LVL_GOD, TRUE);
	sprintf(buf, "&7%s &ghas left the game.", GET_NAME(ch));
	info_channel(buf, ch);
	STATE(ch->desc) = CON_DISCONNECT;
	/*
	 * For the 'if (d->character)' test in close_socket().
	 * -gg 3/1/98 (Happy anniversary.)
	 */
	ch->desc->character = NULL;
	ch->desc = NULL;
      } else {
	mudlog(buf, CMP, LVL_GOD, TRUE);
      }
      if (free_rent)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);
      extract_char(ch);
    }
  }
}



/* Update PCs, NPCs, and objects */
void point_update(void)
{
  extern struct index_data *obj_index;
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  struct timer_type *timer, *next_timer;
  void death_cry(struct char_data * ch);

  /* characters */
  for (i = character_list; i; i = next_char)
  {
    next_char = i->next;
	
    if (GET_POS(i) >= POS_STUNNED) 
    {
/* below ifs are so you can have more HIT/MANA/MOVE then MAX_HIT/MANA/MOVE
 * when wolf/vamp. this is so ya can have BIG bonuses instead of the max
 * of 128 that ya get with an affection cos the number is an sbyte - Vader */
      if (GET_HIT(i) < GET_MAX_HIT(i))
        GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));

      // Remove Ghost Status
      if (!IS_NPC(i) && IS_GHOST(i) && (GET_HIT(i) >= GET_MAX_HIT(i)))
        REMOVE_BIT(EXT_FLAGS(i), EXT_GHOST);

      if(GET_MANA(i) < GET_MAX_MANA(i))
        GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));

      if(GET_MOVE(i) < GET_MAX_MOVE(i))
        GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));

      // Artus> Remove meditate.
      if ((GET_POS(i) == POS_SITTING) && !IS_NPC(i) &&
	  ((timer = char_affected_by_timer(i, TIMER_MEDITATE)) ||
	   (timer = char_affected_by_timer(i, TIMER_HEAL_TRANCE))) &&
	  (GET_HIT(i) >= GET_MAX_HIT(i)) &&
	  (GET_MANA(i) >= GET_MAX_MANA(i)) &&
	  (GET_MOVE(i) >= GET_MAX_MOVE(i)))
      {
	timer->duration = 0;
	timer_remove_char(i, timer);
      }

      if (IS_AFFECTED(i, AFF_POISON))
	if (damage(i, i, 2, SPELL_POISON, FALSE) == -1)
	  continue;	/* Oops, they died. -gg 6/24/98 */

      if(!IS_NPC(i) && !IS_AFFECTED(i,AFF_WATERBREATHE) &&
	 (UNDERWATER(i)) && LR_FAIL(i, LVL_IS_GOD))
      {
        send_to_char("You take a deep breath of water. OUCH!\r\n",i);
        send_to_char("Your chest protests terrebly causing great pain.\r\n", i);
        act("$n suddenly turns a deep blue color holding $s throat.", 
            TRUE, i, 0, 0 , TO_ROOM);
        // GET_HIT(i)-= GET_LEVEL(i)*5;
	damage(NULL, i, GET_LEVEL(i)*5, TYPE_UNDEFINED, FALSE);
	if (GET_HIT(i) <0)
	{
	  send_to_char("Your life flashes before your eyes.  You have Drowned.  RIP!\r\n", i);
	  act("$n suddenly turns a deep blue color holding $s throat.", 
	      TRUE, i, 0, 0 , TO_ROOM);
	  act("$n has drowned. RIP!.", TRUE, i, 0, 0, TO_ROOM);
	  if (MOUNTING(i))
	  {
	    send_to_char("Your mount suffers as it dies.\r\n", i);
	    death_cry(MOUNTING(i));
	    raw_kill(MOUNTING(i), NULL);
	  }
	  death_cry(i);
	  die(i,NULL,"drowning");
	  continue;
	}
      }                  
      if (GET_POS(i) <= POS_STUNNED)
	update_pos(i);
    } else if (GET_POS(i) == POS_INCAP) {
      if (FIGHTING(i))
      { // Artus> Let the opponent get exp :o)
	if (damage(i, FIGHTING(i), 1, TYPE_SUFFERING, FALSE) == -1)
	  continue;
      } else {
	if (damage(i, i, 1, TYPE_SUFFERING, FALSE) == -1)
	  continue;
      }
    } else if (GET_POS(i) == POS_MORTALLYW) {
      if (FIGHTING(i))
      {
	if (damage(i, FIGHTING(i), 2, TYPE_SUFFERING, FALSE) == -1)
	  continue;
      } else {
	if (damage(i, i, 2, TYPE_SUFFERING, FALSE) == -1)
	  continue;
      }
    }
    if (!IS_NPC(i))
    {
      update_char_objects(i);
      if (GET_LEVEL(i) < idle_max_level)
	check_idling(i);
    }
    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);
    punish_update(i);
  }

  /* objects */
  for (j = object_list; j; j = next_thing)
  {
    next_thing = j->next;	/* Next in object list */
    // Remove timers-structures 
    for (timer = OBJ_TIMERS(j); timer; timer = next_timer)
    {
      next_timer = timer->next;
                      
      // Decrement time
      if (timer->duration > 1)
        timer->duration--;
      else if (timer->duration == -1)      /* No action */
        timer->duration = -1;
      // Remove Timer
      else
        timer_remove_obj(j, timer);
    }

    /* If this is a corpse */
    if (IS_CORPSE(j))
    {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j))
      {
	if (j->carried_by)
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
	else if ((j->in_room != NOWHERE) && (world[j->in_room].people))
	{
          if(GET_OBJ_VNUM(j) == 22300)
	  { /* check if its a gate */
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
	// Artus> Only empty player corpses. Mob corpses can die.
	if (GET_CORPSEID(j) > 0)
	  for (jj = j->contains; jj; jj = next_thing2) 
	  {
	    next_thing2 = jj->next_content;	/* Next in inventory */
	    obj_from_obj(jj);

	    if (j->in_obj)
	      obj_to_obj(jj, j->in_obj);
	    else if (j->carried_by)
	      obj_to_room(jj, j->carried_by->in_room);
	    else if (j->in_room != NOWHERE)
	      obj_to_room(jj, j->in_room);
	    else
	      core_dump();
	  }
	extract_obj(j);
      }
    /* If the timer is set, count it down and at 0, try the trigger */
    /* note to .rej hand-patchers: make this last in your point-update() */
    } else if (GET_OBJ_TIMER(j) > 0) {
      GET_OBJ_TIMER(j)--;
      if (!GET_OBJ_TIMER(j))
        timer_otrigger(j);
    }
  }
}
