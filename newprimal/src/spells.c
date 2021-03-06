/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "constants.h"
#include "interpreter.h"

extern room_rnum r_mortal_start_room;
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;
extern int pk_allowed;

void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_savingthrow(struct char_data * ch, int type, int modifier);
void name_to_drinkcon(struct obj_data * obj, int type);
void name_from_drinkcon(struct obj_data * obj);
int compute_armor_class(struct char_data *ch);

/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}


ASPELL(spell_recall)
{
  int room_num;

  if (victim == NULL || IS_NPC(victim))
    return;

  if (PRF_FLAGGED(ch, PRF_MORTALK))
        REMOVE_BIT(PRF_FLAGS(ch), PRF_MORTALK);
  if (PRF_FLAGGED(victim, PRF_MORTALK))
        REMOVE_BIT(PRF_FLAGS(victim), PRF_MORTALK);

  room_num = real_room(ENTRY_ROOM(victim,get_world(victim))); 

  if (!room_num) {
    sprintf(buf,"SYSERR: Recall: %s entry room for world %d invalid", 
        GET_NAME(victim), get_world(victim));
    mudlog(buf, BRF, LVL_IMMORT, TRUE);
    return;  
  }
  
  log("room_num: %d",room_num);

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);

  char_to_room(victim, room_num);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}


ASPELL(spell_teleport)
{
  room_rnum to_room;
  extern room_rnum top_of_world;

  if (victim == NULL || IS_NPC(victim))
    return;
 
  if (IS_SET(zone_table[world[ch->in_room].zone].zflag, ZN_NO_TELE)){
     send_to_char("A blinding flash of light disrupts your magic.  \r\nYou Fail!.\r\n", ch);     return;
  } 

  do {
    to_room = number(0, top_of_world);
  } while ( ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH)
	|| (world[to_room].zone != world[ch->in_room].zone) );

  act("$n slowly fades out of existence and is gone.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
  int flag;
  extern int allowed_zone(struct char_data * ch,int flag);
  extern int allowed_room(struct char_data * ch,int flag);

  if (ch == NULL || victim == NULL)
    return;

  /* DM - nosummon for certain zones */
 
  /* Reception */
  if (zone_table[world[ch->in_room].zone].number == 5) {
    send_to_char("Can't you wait until you get into the game?\r\n",ch);
    return;
  }
 
  /* Haven - no mobs */
  if (IS_NPC(victim))
    if (zone_table[world[ch->in_room].zone].number == 1) {
      send_to_char("Go somewhere else and summon mobs!\r\n",ch);
      return;
    } 

  if(!IS_NPC(victim))
    if(!victim->desc || world[victim->in_room].number == 2) {
      send_to_char("Go cheat someplace else...\r\n",ch);
      return;
      }
 
  if (PRF_FLAGGED(victim, PRF_MORTALK)){
    send_to_char("Summon someone from Mortal Kombat ARENA.  I THINK NOT!!!\r\n", ch);
    return;
  }
  if (PRF_FLAGGED(ch, PRF_MORTALK))     {
        send_to_char("You cannot summon other players to the Mortal Kombat Arena!", ch);
        return;
  } 

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  if (!pk_allowed) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_TUNNEL))
  {
    send_to_char("You must be out of your mind, this passage is far too narrow!\r\n", ch);
    return;
  }
  /* check if the room we're going to is level restricted   bm 3/95*/
  flag = zone_table[world[ch->in_room].zone].zflag;
 
  if (!allowed_zone(ch,flag))
    return;
 
  if ( IS_SET(flag, ZN_PK_ALLOWED) )
  {
    send_to_char("You can't summon players to a Player Killing zone.\r\n",ch);
    return;
  }
 
  flag = world[ch->in_room].room_flags;
  if (!allowed_room(ch,flag))
    return;

    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      sprintf(buf, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      GET_NAME(ch), world[ch->in_room].name,
	      (ch->player.sex == SEX_MALE) ? "He" : "She");
      send_to_char(buf, victim);

      sprintf(buf, "You failed because %s has summon protection on.\r\n",
	      GET_NAME(victim));
      send_to_char(buf, ch);

      sprintf(buf, "%s failed summoning %s to %s.",
	      GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      mudlog(buf, BRF, LVL_IMMORT, TRUE);
      return;
    }
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
      (IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL, 0))) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, ch->in_room);

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
  if (IS_NPC(victim))
  {
    act("$n says 'I'll get you for that!!'", TRUE, victim, 0, 0, TO_ROOM);
    hit(victim, ch, TYPE_UNDEFINED);
/*    GET_HIT(ch) /= 2;
    GET_MANA(ch) = 0;
    GET_MOVE(ch) = 0; */
  }  
}



ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;
  bool house = TRUE;

  /*
   * FIXME: This is broken.  The spell parser routines took the argument
   * the player gave to the spell and located an object with that keyword.
   * Since we're passed the object and not the keyword we can only guess
   * at what the player originally meant to search for. -gg
   */
  strcpy(name, fname(obj->name));
  j = level / 2;

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname(name, i->name))
      continue;

    if (i->carried_by)
      sprintf(buf, "%s is being carried by %s.\r\n",
	      i->short_description, PERS(i->carried_by, ch));
    else if (i->in_room != NOWHERE)
      sprintf(buf, "%s is in %s.\r\n", i->short_description,
	      world[i->in_room].name);
    else if (i->in_obj)
      sprintf(buf, "%s is in %s.\r\n", i->short_description,
	      i->in_obj->short_description);
    else if (i->worn_by)
      sprintf(buf, "%s is being worn by %s.\r\n",
	      i->short_description, PERS(i->worn_by, ch));
    else
      sprintf(buf, "%s's location is uncertain.\r\n",
	      i->short_description);

  if(!ROOM_FLAGGED(i->in_room, ROOM_HOUSE)) {
    CAP(buf);
    send_to_char(buf, ch);
    house = FALSE;
    }
    j--;
  }

  if ((j == level / 2) || house)
    send_to_char("You sense nothing.\r\n", ch);
}



ASPELL(spell_charm)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if (victim == ch)
    send_to_char("You like yourself even better!\r\n", ch);
  else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
    send_to_char("You fail because SUMMON protection is on!\r\n", ch);
  else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
    send_to_char("Your victim is protected by sanctuary!\r\n", ch);
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char("Your victim resists!\r\n", ch);
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char("You can't have any followers of your own!\r\n", ch);
  else if (AFF_FLAGGED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char("You fail.\r\n", ch);
  /* player charming another player - no legal reason for this */ 
  else if (!pk_allowed && !IS_NPC(victim))
    send_to_char("You fail - shouldn't be doing it anyway.\r\n", ch);
  else if (circle_follow(victim, ch))
    send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);
  else if (mag_savingthrow(victim, SAVING_PARA, 0))
    send_to_char("Your victim resists!\r\n", ch);
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    af.type = SPELL_CHARM;

    if (GET_INT(victim))
      af.duration = 24 * 18 / GET_INT(victim);
    else
      af.duration = 24 * 18;

    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim)) {
      REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
    }
  }
}



ASPELL(spell_identify)
{
  char lr_buf[9];
  int i;
  int found;
 
  struct time_info_data *age(struct char_data * ch);
 
  extern char *spells[];
 
  extern const char *item_types[];
  extern const char *extra_bits[];
  extern const char *apply_types[];
  extern const char *affected_bits[];
 
  if (obj) {
    send_to_char("You feel informed:\r\n", ch);
    sprintf(buf, "Object '%s', Item type: ", obj->short_description);
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    send_to_char(buf, ch); 
 
    if (obj->obj_flags.bitvector) {
      send_to_char("Item will give you following abilities:  ", ch);
      sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
    if (GET_OBJ_LR(obj))
    {
      sprintf(lr_buf, " LR_%d", (int)GET_OBJ_LR(obj));
      strcat(buf, lr_buf);
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
 
    sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
            GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
    send_to_char(buf, ch);
 
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

      if (GET_OBJ_VAL(obj, 1) >= 1)
        sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 1)));
      if (GET_OBJ_VAL(obj, 2) >= 1)
        sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 2)));
      if (GET_OBJ_VAL(obj, 3) >= 1)
        sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 3)));
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
      sprintf(buf + strlen(buf), " %s\r\n", skill_name(GET_OBJ_VAL(obj, 3))); 
      sprintf(buf, "%sIt has %d maximum charge%s and %d remaining.\r\n", buf,
              GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
              GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_WEAPON:
      sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
              GET_OBJ_VAL(obj, 2));
      sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
              (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
      send_to_char(buf, ch);
      break;
    case ITEM_ARMOR:
      sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0)) {
        if (!found) {
          send_to_char("Can affect you as :\r\n", ch);
          found = TRUE;
        }
        sprinttype(obj->affected[i].location, apply_types, buf2);
        sprintf(buf, "   Affects: %s By %d\r\n", buf2, obj->affected[i].modifier);
        send_to_char(buf, ch);
      }
    }
  } else if (victim) {          /* victim */ 
    sprintf(buf, "Name: %s\r\n", GET_NAME(victim));
    send_to_char(buf, ch);
    if (!IS_NPC(victim)) {
      sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
              GET_NAME(victim), age(victim)->year, age(victim)->month,
              age(victim)->day, age(victim)->hours);
      send_to_char(buf, ch);
    }
    sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
            GET_HEIGHT(victim), GET_WEIGHT(victim));
    sprintf(buf, "%sLevel: %d, Hits: %d, Mana: %d\r\n", buf,
            GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
    sprintf(buf, "%sAC: %d, Hitroll: %d, Damroll: %d\r\n", buf,
            GET_AC(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
    sprintf(buf, "%sStr: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
            buf, GET_REAL_STR(victim), GET_REAL_ADD(victim), GET_REAL_INT(victim),
        GET_REAL_WIS(victim), GET_REAL_DEX(victim), GET_REAL_CON(victim), GET_REAL_CHA(victim));
    send_to_char(buf, ch);
 
  }
} 


/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */
ASPELL(spell_enchant_weapon)
{
  int i;

  if (ch == NULL || obj == NULL)
    return;

  /* Either already enchanted or not a weapon. */
  if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
    return;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location != APPLY_NONE)
      return;

  SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj->affected[0].location = APPLY_HITROLL;
  obj->affected[0].modifier = 1 + (level >= 18);

  obj->affected[1].location = APPLY_DAMROLL;
  obj->affected[1].modifier = 1 + (level >= 20);

  if (IS_GOOD(ch)) {
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
    act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
  } else if (IS_EVIL(ch)) {
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
    act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
  } else
    act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
}


ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char("You can sense poison in your blood.\r\n", ch);
      else
        send_to_char("You feel healthy.\r\n", ch);
    } else {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char("You sense that it should not be consumed.\r\n", ch);
    }
  }
}

ASPELL(spell_control_weather)
{
 weather_info.sky = (weather_info.sky+1);
 if (weather_info.sky>SKY_LIGHTNING)
   weather_info.sky=SKY_CLOUDLESS;
  switch (weather_info.sky) {
  case SKY_CLOUDLESS:
    send_to_outdoor("The clouds disappear.\r\n");
    break;
  case SKY_CLOUDY:
    send_to_outdoor("The sky starts to get cloudy.\r\n");
    break;
  case SKY_RAINING:
    send_to_outdoor("It starts to rain.\r\n");
    break;
  case SKY_LIGHTNING:
    send_to_outdoor("Lightning starts to show in the sky.\r\n");
    break;
  default:
    break;
  }
}

ASPELL(spell_fingerdeath)
{
  int lev_diff;
  int dam;
  int chance;
  SPECIAL(receptionist);
  SPECIAL(postmaster);
  extern struct index_data *mob_index;
 
  if (ch == NULL || victim == NULL)
    return;
  if (!IS_NPC(victim) && !(GET_LEVEL(ch) > LVL_IMPL))
  {
    send_to_char("You can't use this spell on other players.\r\n",ch);
    return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(victim) == postmaster)) {
        send_to_char("The postmaster gives you the finger.  Same to you BUDDY!.\r\n", ch);
        act("$N raises $S finger up at $n and says, 'Same to you BUDDY!'.", FALSE, ch, 0 , victim, TO_ROOM);
        return;
  }
 
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(victim) == receptionist)){
        send_to_char("The receptionist is too fast for you to kick.\r\n", ch);
        act("$N raises $S finger up at $n and says, 'Same to you BUDDY!'.", FALSE, ch, 0 , victim, TO_ROOM);
        return;
  }
        send_to_char("You point your deadly finger...\n\r", ch);
 
/*  dam=dice(10,10)+75;
  damage(ch,victim,dam,SPELL_FINGERDEATH);*/
 
  lev_diff=GET_LEVEL(ch)-GET_LEVEL(victim);
  /* chance of outright kill here */
 
  act("$n points $s deadly finger at $N.", FALSE, ch, 0, victim, TO_NOTVICT);
  if (lev_diff>0)
  {
    chance = MIN(50, ((lev_diff*lev_diff)/20) + 1);
/* if it succeeds the victim's hps are set to 1 */
    if (number(0,100)<=chance)
                {
                act("$n screams in agony.", FALSE, victim, 0, ch, TO_ROOM);
                GET_HIT(victim) = 1;
                damage(ch,victim,0,SPELL_FINGERDEATH);
                }else
                {
                  dam=dice(10,10)+75;
                  damage(ch,victim,dam,SPELL_FINGERDEATH);
                }
  }else
       {
         dam=dice(10,10)+75;
         damage(ch,victim,dam,SPELL_FINGERDEATH);
       }
} 

/* spell to scare mobs away. doesnt work if they have sanct, if theyre a
 * higher level then you or if theyre charmed.. - VADER
 */
ASPELL(spell_fear)
{
  ACMD(do_flee);
 
  if(victim == NULL || ch == NULL)
    return;
 
  if(victim == ch)
    send_to_char("Boo! Are you scared yet?\r\n",ch);
  else if(IS_AFFECTED(victim, AFF_SANCTUARY))
    send_to_char("Your victim is protected by sanctuary!\r\n",ch);
  else if(IS_AFFECTED(victim, AFF_CHARM))
    send_to_char("Your victim can't be scared enough to leave their master.\r\n",ch);
  else if(level < GET_LEVEL(victim))
    send_to_char("You fail.\r\n",ch);
  else if(!pk_allowed && !IS_NPC(victim))
    send_to_char("Try sneaking up behind them and screaming 'Boo!'\r\n",ch);
  else if(mag_savingthrow(victim, SAVING_PARA, 0))
    send_to_char("Your victim resists!\r\n",ch);
  else {
    do_flee(victim,"",1,0);
    }
} 

/* this opens a gateway to a player.. sorta like reverse summon but it
 * leaves the door there for an amount of time for anyone to use
 * blind the player if it fails - VADER
 */
ASPELL(spell_gate)
{
  struct affected_type af;
  int flag, vict_room;
  byte percent;
  extern int allowed_zone(struct char_data * ch,int flag);
  extern int allowed_room(struct char_data * ch,int flag);
  struct obj_data *gate;
  struct obj_data *next_obj;
 
  if (ch == NULL || victim == NULL)
    return;
 
  if (PRF_FLAGGED(ch, PRF_MORTALK)){
    send_to_char("You cannot open a portal in space and time while in MORTAL KOMBAT!\r\n", ch);
    return;
  }
 
  if(IS_NPC(victim)) {
    send_to_char("That'd be a bit easy wouldn't it? This only works on players.\r\n",ch);
    return;
    }
 
  if (GET_LEVEL(victim) > MIN(LVL_ANGEL - 1, level + 3)) {
    send_to_char("You failed.\r\n", ch);
    return;
  }
 
  if (IS_SET(ROOM_FLAGS(victim->in_room), ROOM_TUNNEL))
  {
    send_to_char("You must be out of your mind, the passage they are in is far too narrow!\r\n", ch);
    return;
  }
 
  if (IS_SET(ROOM_FLAGS(victim->in_room), ROOM_HOUSE))
  {
    send_to_char("That player is in a private house.\r\nYou Fail.\r\n", ch);
    return;
  }
 
  if (!pk_allowed) {
 
  /* check if the room we're going to is level restricted */
  flag = zone_table[world[victim->in_room].zone].zflag;
 
  if (!allowed_zone(ch,flag))
    return;
 
  if ( IS_SET(flag, ZN_PK_ALLOWED) )
  {
    send_to_char("You can't open a dimensional gate to a player in a Player Killing zone.\r\n",ch);
    return;
  }
 
  flag = world[victim->in_room].room_flags;
  if (!allowed_room(ch,flag))
    return;
 
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
        !PLR_FLAGGED(victim, PLR_KILLER)) {
 
      sprintf(buf, "You failed because %s has summon protection on.\r\n",
              GET_NAME(victim));
      send_to_char(buf, ch);
 
      return;
    }
  }
  /* check if victim in reception */
  if(world[victim->in_room].number >= 500 && world[victim->in_room].number <= 599) {
    send_to_char("Cannot find the target of your spell!\r\n",ch);
    return;
    }
 
  /* DM - clan rooms */
  vict_room=world[victim->in_room].number;
  if (vict_room == 103 || (vict_room >= 105 && vict_room <= 131)) {
    send_to_char("You can't open a dimensional gate to a player in a clan area.\r\n",ch);
    return;
  }
 
  /* check for other vortexes in this room */
  for(gate = world[ch->in_room].contents; gate; gate = next_obj) {
    next_obj = gate->next_content;
    if(GET_OBJ_VNUM(gate) == 22300) {
      send_to_char("There is too much of a temporal disturbance in this room already.\r\n",ch);
      return;
      }
    }
  percent = number(0,101); /* 101% is total failure */
  if(percent < 9) {
    af.type = SPELL_BLINDNESS;
    af.duration = 24;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_BLIND;
    affect_to_char(ch, &af);
    act("The room appears to bend slightly as an unstable dimensional gate appears!",FALSE,ch,0,0,TO_CHAR);
    act("The room disappears in a blinding flash as the dimensional gate collapses!",FALSE,ch,0,0,TO_CHAR);
    act("The room lights up briefly as the time/space continuum refuses to bend!",FALSE,ch,0,0,TO_ROOM);
  } else { /* ok then make the gate */
    gate = read_object(22300,VIRTUAL);
    GET_OBJ_VAL(gate,0) = world[victim->in_room].number;
    GET_OBJ_VAL(gate,3) = 1;
    GET_OBJ_TIMER(gate) = number(2,5);
    obj_to_room(gate,ch->in_room);
    act("The room appears to bend slightly as a dimensional gate appears!",FALSE,ch,0,0,TO_CHAR);
    act("You feel the room bend!\r\n$n has created a dimensional gate!",FALSE,ch,0,0,TO_ROOM);
    }
} 
