/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern int pk_allowed;
extern struct zone_data *zone_table;
extern struct index_data *obj_index;   
extern struct spell_info_type spell_info[];
extern int num_rooms_burgled;
extern const char *dirs[];
extern Burglary *burglaries;
/* extern functions */
void check_killer(struct char_data * ch, struct char_data * vict);
int compute_armor_class(struct char_data *ch, bool divide);
struct obj_data *unequip_char(struct char_data * ch, int pos);
bool basic_skill_test(struct char_data * ch, int spellnum, bool mod_abil);
long get_burgle_room_type(long lArea);

extern struct time_info_data time_info;
void clan_rel_inc(struct char_data *ch, struct char_data *vict, int amt);

cpp_extern const struct trap_type trap_types[] = {
  {"!UNUSED!", TYPE_UNDEFINED, 0},
  {"pit", SKILL_TRAP_PIT, 2},      // Inside Only
  {"magic", SKILL_TRAP_MAGIC, 7}   // Inside, Outside, Underwater
};

/* local functions */
room_rnum create_burgle_rooms(struct char_data *ch, int area_type, int dir);
ACMD(do_throw);
ACMD(do_ambush);
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
//ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);
ACMD(do_piledrive);
ACMD(do_trap);
ACMD(do_trip);
ACMD(do_headbutt);
ACMD(do_bearhug);
//ACMD(do_battlecry);
ACMD(do_disarm);

/* DM - disarm
 *
 * If successful unarm the vict, making their weapon fall to the ground.
 */
ACMD(do_disarm)
{
  int percent, prob;
  struct char_data *vict;
  struct obj_data *wielded;

  prob = GET_SKILL(ch, SKILL_DISARM);

  // Dont have the skill
  if (!prob)
  {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }

  // Dont have stats
  if (!has_stats_for_skill(ch, SKILL_DISARM, TRUE))
    return;

  one_argument(argument, arg);

  // No arg
  if (!(*arg || FIGHTING(ch)))
  {
    send_to_char("Disarm who?\r\n",ch);
    return;
  }

  // Can we find them?
  if (!(vict = generic_find_char(ch, arg, FIND_CHAR_ROOM)) &&
      !(vict = FIGHTING(ch)))
  {
    send_to_char(NOPERSON, ch);
    return; 
  }
  
  // Disarm self - yeah right
  if (vict == ch)
  {
    send_to_char("That'd be a bright idea!\r\n",ch); 
    return;
  }

  // Make sure we are fighting them
  if (FIGHTING(ch) != vict)
  {
    send_to_char("Perhaps you should try fighting them first.\r\n",ch);
    return;
  }
  
  // Check they are using a weapon
  if (!(vict->equipment[WEAR_WIELD]))
  {
    act("$M isn't even using a weapon!\r\n", 0, ch, 0, vict, TO_CHAR);
    return;
  }

  // Check for the AFF flag
  if (AFF_FLAGGED(vict, AFF_NODISARM))
  {
    switch (number(1,4))
    {
      case 1:
        act("Your disarm attempt is blocked by $N!",
	    FALSE, ch, 0, vict, TO_CHAR);
        act("You block $n's disarm attempt!",
	    FALSE, ch, 0, vict, TO_VICT);
        act("$n's disarm attempt is blocked by $N.",
	    FALSE, ch, 0, vict, TO_NOTVICT);
        break;
      case 2:
        act("$N smirks as $M blocks your disarm attempt!",
	    FALSE, ch, 0 , vict, TO_CHAR);
        act("You smirk as you block $n's disarm attempt!",
	    FALSE, ch, 0 , vict, TO_VICT);
        act("$N smirks as $M blocks $n's disarm attempt!",
	    FALSE, ch, 0 , vict, TO_NOTVICT);
        break;
      case 3:
        act("$N cleverly avoids your disarm attempt.",
	    FALSE, ch, 0 , vict, TO_CHAR);
        act("You cleverly avoid $n's disarm attempt.",
	    FALSE, ch, 0 , vict, TO_VICT);
        act("$N cleverly avoids $n's disarm attempt.",
	    FALSE, ch, 0 , vict, TO_NOTVICT);
        break;
      case 4:
        act("$N steps to the side avoiding your disarm attempt!",
	    FALSE, ch, 0 , vict, TO_CHAR);
        act("You step to the side avoiding $n's disarm attempt!",
	    FALSE, ch, 0 , vict, TO_VICT);
        act("$N steps to the side avoiding $n's disarm attempt!",
	    FALSE, ch, 0 , vict, TO_NOTVICT);
        break;
    }
    return;
  }      

  percent = number(1, 101);     /* 101% is a complete failure */
  int val = MIN(25, MAX(-10, (GET_LEVEL(ch) - GET_LEVEL(vict) - 10))) * 5 +
    		dex_app_skill[GET_REAL_DEX(ch)].p_pocket; 
  
  // level  x    Dex 17 18 19 20 21 
  //  diff  5     (pick pocket app)      val
  // ------------------------------
  // -30   -250       5 10 15 15 20  -----------
  // -20   -200       5 10 15 15 20  -----------
  // -15   -150       5 10 15 15 20  ----------- 
  // -10   -100       5 10 15 15 20  (-95 - -80)
  // -5    -75        5 10 15 15 20  (-70 - -50)
  // 0     -50        5 10 15 15 20  (-45 - -30) 
  // +5    -25        5 10 15 15 20  (-20 -   0)
  // +10   0          5 10 15 15 20  (  5 -  20)
  // +15   25         5 10 15 15 20  ( 30 -  45)
  // +20   50         5 10 15 15 20  ( 55 -  70)
  // +25   75         5 10 15 15 20  ( 80 -  95)
  // +30   100        5 10 15 15 20  -----------
  // +40   150        5 10 15 15 20  -----------
  // +50   200        5 10 15 15 20  -----------
  // +60   250        5 10 15 15 20  -----------
    
  percent = percent - val;
    
  if (percent > prob)
  {
    act("You fail your disarm attempt on $N!", 0, ch, 0, vict, TO_CHAR);
    act("$N fails a disarm attempt on you!", 0, ch, 0, vict, TO_VICT);
    act("$n fails a disarm attempt on $N.", FALSE, ch, 0, vict, TO_NOTVICT);
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 1;
  } else {
    // this will core_dump in unequip_char if not sucessful
    wielded = unequip_char(vict, WEAR_WIELD, TRUE);
    // now take the object and place it on the ground
    obj_from_char(wielded);
    if (OBJ_RIDDEN(wielded))
    {
      MOUNTING_OBJ(OBJ_RIDDEN(wielded)) = NULL;
      //wielded->ridden_by->char_specials->mounting_obj = NULL;
      send_to_char("Your mount has been disarmed from under you! How strange...\r\n", wielded->ridden_by);
      wielded->ridden_by = NULL;
    }
    obj_to_room(wielded, ch->in_room);
    // Mesg to Char 
    sprintf(buf,"You disarm $N. %s &5%s&n falls to the ground.",
	    AN(wielded->short_description), wielded->short_description);
    act(buf, 0, ch, 0, vict, TO_CHAR);
    // Mesg to Vict
    sprintf(buf,"$n disarms you! Your &5%s&n falls to the ground.",
	    wielded->short_description);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    // Mesg to onlookers
    sprintf(buf,"$n disarms $N, %s &5%s&n falls to the ground.",
	    AN(wielded->short_description), wielded->short_description);
    act(buf, 0, ch, 0, vict, TO_NOTVICT);
    // Make both the vict and attacker wait 1 violence pulse
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 1;
    GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 1;
  }
}
        
ACMD(do_ambush)
{
  int percent, prob;
  struct char_data *vict;
  char arg2[MAX_INPUT_LENGTH];

  prob = GET_SKILL(ch, SKILL_AMBUSH);
  if (!prob)
  {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }
  
 // if (FIGHTING(ch)) {
 //   send_to_char("You're fighting! How can you ambush?\r\n",ch);
 //   return;
 // }
  
  if (!has_stats_for_skill(ch, SKILL_AMBUSH, TRUE))
    return;

  if (!AFF_FLAGGED(ch, AFF_HIDE))
  {
    send_to_char("Your not even hiding!\r\n",ch);
    return;
  }
  
  arg2[0] = '\0';
  //one_argument(argument, arg);
  two_arguments(argument, arg, arg2);

  if (!*arg)
  {
    send_to_char("Ambush who?\r\n",ch);
    return;
  }

  if (!(vict = generic_find_char(ch, arg, FIND_CHAR_ROOM)))
  {
    send_to_char(NOPERSON, ch);
    return; 
  }
  
  if (!IS_NPC(vict) && strcmp(arg2, "really") != 0)
  {
    send_to_char("Fine, but use &4ambush <player> really&n to ambush a player.\r\n", ch);
    return;
  }
	
  if (vict == ch)
  {
    send_to_char("Go ambush someone else!\r\n",ch); 
    return;
  }

  percent = number(1, 101);     /* 101% is a complete failure */

  if (percent > prob)
  {
    act("You fail your ambush on $N!", 0, ch, 0, vict, TO_CHAR);
    act("$N fails an ambush attempt on you!", 0, vict, 0, ch, TO_CHAR);
    act("$n fails an ambush attempt on $N.", FALSE, ch, 0, vict, TO_NOTVICT);
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
    damage(ch, vict, 0, TYPE_UNDEFINED, FALSE);
  } else {
    act("You ambush $N!", 0, ch, 0, vict, TO_CHAR);
    act("$N ambushes you!", 0, vict, 0, ch, TO_CHAR);
    act("$n ambushes $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 3;
    damage(ch, vict, 5*GET_LEVEL(ch), TYPE_UNDEFINED, FALSE);
  }
}


ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch))
  {
    send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Whom do you wish to assist?\r\n", ch);
  else if (!(helpee = generic_find_char(ch, arg, FIND_CHAR_ROOM)))
    send_to_char(NOPERSON, ch);
  else if (helpee == ch)
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  else
  {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
      for (opponent = world[ch->in_room].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room) ;
    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!pk_allowed && !IS_NPC(opponent))	/* prevent accidental pkill */
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else
    {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}

/*
void list_fighters_to_char(struct char_data *ch)
{
  struct char_data *c;
  int nFound = 0;

  for(c = world[ch->in_room].people; c; c = c->next) {
    if (FIGHTING(c) == ch) {
      nFound++;
      if (nFound == 1)
        sprintf(buf, "You are fighting %s%s&n", IS_NPC(c) ? "&6" : "&7", GET_NAME(c));
      else
        sprintf(buf + strlen(buf), "&n, %s%s", IS_NPC(c) ? "&6" : "&7", GET_NAME(c));
    }
  }

  if (nFound != 0)
    sprintf(buf + strlen(buf), "&n.\r\n");
  else
    sprintf(buf, "Hit whom?\r\n");

  send_to_char(buf, ch);
}

*/
/*
ACMD(do_hit)
{
  struct char_data *vict;
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index; 

  one_argument(argument, arg);

  // Check that no mounted char is trying to do an attack by itself
  if (!IS_NPC(ch) && MOUNTING(ch) && !FIGHTING(MOUNTING(ch)))
	return;

  if (IS_AFFECTED(ch,AFF_CHARM) && !ch->master)
    REMOVE_BIT(AFF_FLAGS(ch),AFF_CHARM);

  if (!*arg)
   list_fighters_to_char(ch);
   // send_to_char("Hit who?\r\n", ch);
  else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster))
        send_to_char("You cannot attack the postmaster!!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist))
        send_to_char("You cannot attack the Receptionist!\r\n", ch);               
  else {
     // Attacking a mount is like attacking the player
     if (MOUNTING(vict) && IS_NPC(vict))
	vict = MOUNTING(vict);

    if (!pk_allowed) {
      if (!IS_NPC(vict) && !IS_NPC(ch)) {
	if (subcmd != SCMD_MURDER) {
	  send_to_char("Use 'murder' to hit another player.\r\n", ch);
	  return;
	} else {
	  check_killer(ch, vict);
	}
      }
      if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			// you can't order a charmed pet to attack a
				// player 
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      if (wielded && OBJ_IS_GUN(wielded)) {
        send_to_char("You would do better to shoot this weapon!\n\r", ch);
        return;
      } 
      GET_WAIT_STATE(ch) = PULSE_VIOLENCE + 2;
      hit(ch, vict, TYPE_UNDEFINED);
      // Order mount to attack 
      if (!IS_NPC(ch) && MOUNTING(ch) && !FIGHTING(MOUNTING(ch))) {
	send_to_char("You order your mount to attack!\r\n", ch);
	act("$n orders $s mount to attack.\r\n", FALSE, ch, 0, 0, TO_ROOM);
	do_hit(MOUNTING(ch), arg, 0, SCMD_HIT);
      }
    } else if (FIGHTING(vict) != ch)  // Allow target switching
    {
	send_to_char("You're already fighting someone!\r\n", ch);
    }
    // Switch player target
    else if (FIGHTING(vict) == ch && FIGHTING(ch) != vict)
    {
	act("You turn to fight $N!", FALSE, ch, 0, vict, TO_CHAR);
	act("$n turns to fight $N.", FALSE, ch, 0, vict, TO_ROOM);
	act("$n turns to fight you!", FALSE, ch, 0, vict, TO_VICT);
	FIGHTING(ch) = vict;
    }
    else
      send_to_char("You do the best you can!\r\n", ch);
  }
}
*/

/*
ACMD(do_kill)
{
  struct char_data *vict;

  if ((GET_LEVEL(ch) < LVL_IMPL) || IS_NPC(ch)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
	list_fighters_to_char(ch);
//    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
      send_to_char("They aren't here.\r\n", ch);
    else if (ch == vict)
      send_to_char("Your mother would be so sad.. :(\r\n", ch);
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict,ch);
    }
  }
}

*/


/*
ACMD(do_backstab)
{
  struct char_data *vict;
  byte percent, prob;
  SPECIAL(receptionist);
  SPECIAL(postmaster);
  extern struct index_data *mob_index; 
 
  one_argument(argument, buf);
 
  if (!has_stats_for_skill(ch, SKILL_BACKSTAB, TRUE))
    return;
 
  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char("Backstab who?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("How can you sneak up on yourself?\r\n", ch);
    return;
  }
  if (!ch->equipment[WEAR_WIELD]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(ch->equipment[WEAR_WIELD], 3) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }
  if (FIGHTING(vict) && (!IS_SET(GET_SPECIALS(ch), SPECIAL_BACKSTAB)) ) {
    send_to_char("You can't backstab a fighting person -- they're too alert!\r\n", ch);
    return;
  }
 
  if (MOUNTING(vict) && IS_NPC(vict)) {
	send_to_char("You can't backstab someone's mount!\r\n", ch);
	return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE)){
    switch (number(1,4)){
    case 1:
        send_to_char("Your weapon hits an invisible barrier and fails to penetrate it.\r\n", ch);
        break;
    case 2:
        act("$N skillfuly blocks your backstab and attacks with rage!", FALSE, ch, 0 , vict, TO_CHAR);
        break;
    case 3:
        act("$N cleverly avoids your backstab.", FALSE, ch, 0 , vict, TO_CHAR);
        break;
    case 4:
        act("$N steps to the side avoiding your backstab!", FALSE, ch, 0 , vict, TO_CHAR);
        break;
    }
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster)) {
        send_to_char("The postmaster easily dodges your pitiful attempt to backstab him.\r\n", ch);
        act("$N laughs in $n's face as $E dances out of the way of $n's backstab.", FALSE, ch,0,vict,TO_ROOM);
        return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist)){
        send_to_char("The Receptionist easily dodges your pitiful attempt to backstab her.\r\n", ch);
        act("$N laughs in $n's face as $E dances out of the way of $n's backstab.", FALSE, ch,0,vict,TO_ROOM);
        return;
  }
 
  percent = number(1, 101);     // 101% is a complete failure 
  int nBonus = IS_SET(GET_SPECIALS(ch), SPECIAL_THIEF) ? 15 : 0;	// bonus for enhanced thieves
  
  prob = GET_SKILL(ch, SKILL_BACKSTAB) ;
 
  if (AWAKE(vict) && (percent > (prob + nBonus)))
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);
  GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
}
*/


ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH],
       clone_order[MAX_INPUT_LENGTH];
  bool found = FALSE;
  room_rnum org_room;
  struct char_data *vict, *clone, *master;
  struct follow_type *k, *j;
  extern struct index_data *mob_index; 

  half_chop(argument, name, message);

  if (!*name || !*message)
  {
    send_to_char("Order who to do what?\r\n", ch);
    return;
  }
  /* DM - add check for order clones */
  if ((!(vict = generic_find_char(ch, name, FIND_CHAR_ROOM)) && 
	!is_abbrev(name, "followers")) && !is_abbrev(name, "clones"))
  {
    send_to_char("That person isn't here.\r\n", ch);
    return;
  }
  if (ch == vict)
  {
    send_to_char("You obviously suffer from skitzofrenia.\r\n", ch);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM)) 
  {
    send_to_char("Your superior would not aprove of you giving orders.\r\n",
	         ch);
    return;
  }
  if (vict)
  {
    sprintf(buf, "$N orders you to '%s'", message);
    act(buf, FALSE, vict, 0, ch, TO_CHAR);
    act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);
    if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
    {
      act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      return;
    }
    send_to_char(OK, ch);
    command_interpreter(vict, message);
  }
  /* This is order "followers" */
  sprintf(buf, "$n issues the order '%s'.", message);
  act(buf, FALSE, ch, 0, vict, TO_ROOM);

  org_room = ch->in_room;
  /* DM - changed "order followers" to a do while in the case that the follower
	  dies whilst performing the command - CLONES (order followers die)
	  moved die_clone in here now due to problems */
  if ((k = ch->followers))
    do 
    {
      j=k->next;
      if ((org_room == k->follower->in_room) || IS_CLONE(k->follower))
	if (IS_AFFECTED(k->follower, AFF_CHARM)) 
	{
	  found = TRUE;
	  if (IS_CLONE(k->follower))
	  {
	    strcpy(clone_order,message);
	    basic_mud_log("Ordering clone");
	    basic_mud_log(clone_order);
	    if (!(str_cmp(clone_order,"die")))
	    {
	      basic_mud_log("Ordered clone die");
	      die_clone(k->follower,NULL);
	    } else if (!(str_cmp(clone_order,"serve"))) {
	      basic_mud_log("Ordered clone serve");
	      clone = k->follower;
	      master = ch;
	      if (clone->in_room != master->in_room)
	      {
		/* Min level 73 */
		if (LR_FAIL(ch, 73))
		  send_to_char("Your clones have no idea how!\r\n",master);
		/* Check master stats - wis 20, int 20, 10 move, 10 mana */
                else 
		{
		  if (GET_WIS(master) < 20) 
		  {
		    send_to_char("You don't have the wisdom to perform this act\r\n", master);
		  } else if (GET_INT(master) < 20) {
		    send_to_char("You don't have the intelligence to perform this act\r\n", master);
		  } else if (GET_MANA(master) < 50) {
		    send_to_char("You haven't got the energy to perform this act!\r\n", master);
		  } else if (GET_MOVE(master) < 10) {
		    send_to_char("You are too exhausted to perform this act!\r\n", master);
		  } else {
		    act("$n calls upon $s clone for service.",
			TRUE, master, 0, 0, TO_ROOM);
		    act("$N calls you for service.",
			FALSE, clone, 0 , master, TO_CHAR);
		    if (FIGHTING(clone))
		    {
		      stop_fighting(FIGHTING(clone));
		      stop_fighting(clone);
		    }
		    if (GET_POS(clone) != POS_STANDING)
		    {
		      GET_POS(clone) = POS_STANDING;
		      update_pos(clone);
		    }
		    act("$n is called away to serve $s master.",
			TRUE, clone, 0, 0, TO_ROOM);
		    char_from_room(clone);
		    char_to_room(clone,master->in_room);
		    act("$N arrives to serve $S master.",
			TRUE, master, 0, clone, TO_NOTVICT);
		    act("$N arrives to serve you.",
			TRUE, master, 0, clone, TO_CHAR);
		    look_at_room(clone,0);
		    GET_MANA(master) -= 50;
		    GET_MOVE(master) -= 10;
		  }
		}               
	      }
	    } else {
	      basic_mud_log("Using command interpreter on clone");
	      command_interpreter(k->follower, message);
	    }        
	  } else
	    command_interpreter(k->follower, message);
	}
      k=j;
    } while (k);
  if (found)
    send_to_char(OK, ch);
  else
    send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
}

ACMD(do_flee)
{
  int i, attempt, loss;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) 
  {
    if (GET_POS(ch) == POS_SITTING)
      send_to_char("Your down and unable to flee!\r\n", ch);
    else
      send_to_char("You are in pretty bad shape, unable to flee!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(ch, AFF_PARALYZED))
  {
    send_to_char("PANIC! Your legs refuse to move!!\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && IS_AFFECTED(ch, AFF_BERSERK))
  {
    send_to_char("You are too entranced to flee.\r\n", ch);
    return;
  }
  for (i = 0; i < 6; i++)
  {
    attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH) && 
        !(IS_NPC(ch) && 
	  (IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_NOMOB))))
    {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);
      if (do_simple_move(ch, attempt, TRUE, SCMD_FLEE))
      {
	send_to_char("You flee head over heels.\r\n", ch);
	if (was_fighting && !IS_NPC(ch))
	{
	  loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
	  loss *= GET_LEVEL(was_fighting);
          if (loss>300000)
            loss = 300000;
          gain_exp(ch, -loss);
        }
        if (was_fighting && FIGHTING(was_fighting) ==  ch)
	{
          stop_fighting(was_fighting);
          GET_WAIT_STATE(was_fighting) = 0;
        }
        stop_fighting(ch);
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}

/*
ACMD(do_bash)
{
  struct char_data *vict;
  byte percent, prob;
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index; 
 
  one_argument(argument, arg);
 
  if (!has_stats_for_skill(ch, SKILL_BASH, TRUE))
    return;
 
  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }
  
  // Check if the victim is mount and mounted and not already fighting it
  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
	send_to_char("That's someone's mount! Use 'murder' to attack another player.\r\n", ch);
	return;
  }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
 
  if (!ch->equipment[WEAR_WIELD]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char(PEACEROOM, ch);
    return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster)) {
        send_to_char("You cannot bash the postmaster!!\r\n", ch);
        return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist)){
        send_to_char("You cannot bash the Receptionist!\r\n", ch);
        return;
  }
  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(vict))
  {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n just tried to bash $N, and fell flat on their face!", FALSE, ch, 0, vict, TO_NOTVICT);
    GET_POS(ch) = POS_SITTING;
    return;
  }
 
  if (GET_POS(vict) < POS_FIGHTING){
        send_to_char("Your victim is already down!.\r\n", ch);
        return;
  }
 
  percent = number(1, 111);  // 101% is a complete failure 
  prob = GET_SKILL(ch, SKILL_BASH);
 
  if (MOB_FLAGGED(vict, MOB_NOBASH))
        percent= 101;
  if (PRF_FLAGGED(ch,PRF_MORTALK))
        percent=101;

  if (IS_AFFECTED(ch, AFF_ADRENALINE))
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
  else
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
  } else {
    //
    // If we bash a player and they wimp out, they will move to the previous
    // room before we set them sitting.  If we try to set the victim sitting
    // first to make sure they don't flee, then we can't bash them!  So now
    // we only set them sitting if they didn't flee. -gg 9/21/98
    // -1 = dead, 0 = miss 
    if (damage(ch, vict, GET_LEVEL(ch), SKILL_BASH) > 0) { 
      GET_WAIT_STATE(vict) = PULSE_VIOLENCE;
      if (IN_ROOM(ch) == IN_ROOM(vict)) {
        GET_POS(vict) = POS_SITTING;
        GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 2;
      }
    }
  }
*/
  /****************************** stock 30bpl19 bash
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BASH)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  percent = number(1, 101);	// 101% is a complete failure 
  prob = GET_SKILL(ch, SKILL_BASH);

  if (MOB_FLAGGED(vict, MOB_NOBASH))
    percent = 101;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
  } else {
    //
    // If we bash a player and they wimp out, they will move to the previous
    // room before we set them sitting.  If we try to set the victim sitting
    // first to make sure they don't flee, then we can't bash them!  So now
    // we only set them sitting if they didn't flee. -gg 9/21/98
     //
    if (damage(ch, vict, 1, SKILL_BASH) > 0) {	// -1 = dead, 0 = miss 
      WAIT_STATE(vict, PULSE_VIOLENCE);
      if (IN_ROOM(ch) == IN_ROOM(vict))
        GET_POS(vict) = POS_SITTING;
    }
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
  **********************************************/
//}


ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  if (!has_stats_for_skill(ch, SKILL_RESCUE, TRUE))
    return;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_RESCUE))
  {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!(vict = generic_find_char(ch, arg, FIND_CHAR_ROOM)))
  {
    send_to_char("Whom do you want to rescue?\r\n", ch);
    return;
  }
  if (vict == ch)
  {
    send_to_char("What about fleeing instead?\r\n", ch);
    return;
  }
  if (FIGHTING(ch) == vict)
  {
    send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
    return;
  }
  for (tmp_ch = world[ch->in_room].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch)
  {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  
  if (IS_AFFECTED(tmp_ch, AFF_BERSERK))
  {
    act("$N is going berserk, a rescue attempt would be too dangerous.\r\n", TRUE, ch, 0, tmp_ch, TO_CHAR);
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_RESCUE);

  if (percent > prob) {
    send_to_char("You fail the rescue!\r\n", ch);
    return;
  }
  send_to_char("Banzai!  To the rescue...\r\n", ch);
  act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch, SKILL_RESCUE);
  set_fighting(tmp_ch, ch, SKILL_RESCUE);

  GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 2;
  clan_rel_inc(ch, vict, 10);
}


/*
ACMD(do_kick)
{
  struct char_data *vict;
  int percent, prob;
  SPECIAL(receptionist);
  SPECIAL(postmaster);
  extern struct index_data *mob_index; 
  
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }

  if (!has_stats_for_skill(ch, SKILL_KICK, TRUE))
    return;

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }

  // Check if the victim is mount and mounted and not already fighting it
  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
	send_to_char("That's someone's mount! Use 'murder' to attack another player.\r\n", ch);
	return;
  }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster)) {
        send_to_char("The postmaster is too fast for you to kick.\r\n", ch);
        return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist)){
        send_to_char("The receptionist is too fast for you to kick.\r\n", ch);
        return;
  }
 
  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(vict))
  {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n tries to kick $N, and fails miserably!",FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  } 

  // 101% is a complete failure 
  percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
  prob = GET_SKILL(ch, SKILL_KICK);

  if (!IS_AFFECTED(ch, AFF_ADRENALINE))
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
  else
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK);
  } else
    if (damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK) > 0)
    {
	if (GET_SKILL(ch, SKILL_DOUBLE_KICK) && 
	    number(1, 101 - (GET_LEVEL(vict) - GET_LEVEL(ch))) < GET_SKILL(ch, SKILL_DOUBLE_KICK))
	{
	    act("...$n jumps delivering a quick and powerful second kick!", FALSE, ch, 0, 0, TO_ROOM);
	    act("...$n jumps delivering another powerful kick to you!", FALSE, ch, 0, vict, TO_VICT);
	    act("...you quickly jump and deliver another powerful kick to $N!", FALSE, ch, 0, vict, TO_CHAR);
	    damage(ch, vict, GET_LEVEL(ch), SKILL_KICK);
	}
    }
}
*/

/* primal scream skill. hits all enemies in room. can only be used at start
 * not actually during the fight.. - Vader
 *
ACMD(do_scream)
{
  byte percent, prob;
  struct char_data *vict, *next_vict;
  sh_int room;
  int door,dam,skip = 0;
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index;
 
  if (!has_stats_for_skill(ch, SKILL_PRIMAL_SCREAM, TRUE))
    return;
 
  prob = GET_SKILL(ch, SKILL_PRIMAL_SCREAM);
  percent = number(1,101); // 101 is a complete failure 
 
  if(FIGHTING(ch)) {
    send_to_char("You can't prepare yourself properly while fighting!\r\n",ch);
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
  } else if(percent > prob) {
           act("$n lets out a feeble little wimper as $e attempts a primal scream.",FALSE,ch,0,0,TO_ROOM);
           act("You let out a sad little wimper.",FALSE,ch,0,0,TO_CHAR);
         } else {
           act("$n inhales deeply and lets out an ear shattering scream!!\r\n",FALSE,ch,0,0,TO_ROOM);
           act("You fill your lungs to capacity and let out an ear shattering scream!!",FALSE,ch,0,0,TO_CHAR);
           room = ch->in_room;
          for(door=0; door<NUM_OF_DIRS; door++)
                { // this shood make it be heard in a 4 room radius 
                if (!world[ch->in_room].dir_option[door])
                                continue;
 
                ch->in_room = world[ch->in_room].dir_option[door]->to_room;
                if(room != ch->in_room && ch->in_room != NOWHERE)
                  act("You hear a frightening scream coming from somewhere nearby...",FALSE,ch,0,0,TO_ROOM);
                ch->in_room = room;
          }
           for (vict = world[ch->in_room].people; vict; vict = next_vict) {
             next_vict = vict->next_in_room;
 
             skip = 0;
             if(vict == ch)
               skip = 1; // ch is the victim skip to next person 
             if(IS_NPC(ch) && IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
               skip = 1; // if ch is a mob only hit other mobs if they are charmed 
             if(!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_IS_GOD)
               skip = 1; // dont bother gods with it 
             if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster))
                skip = 1;
             if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist))
                skip = 1;
             if(!IS_NPC(ch) && !IS_NPC(vict))
               skip = 1; // dont hit players with it 
             if(!IS_NPC(ch) && IS_NPC(vict) && IS_AFFECTED(vict, AFF_CHARM))
               skip = 1; // dont hit charmed mobs 
 
             if(!(skip)) {
               dam = dice(4, GET_AFF_DEX(ch)) + 6; // max dam of around 90 
               damage(ch,vict,dam,SKILL_PRIMAL_SCREAM);
               }
             }
             GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
         }
} 
*/

/* looks around for people nearby. cant be used indoors. how far you see
 * depends on height - Vader
 */
ACMD(do_scan)
{
  byte prob, percent;
  int dist,door,sqcount,none = TRUE;
  struct room_direction_data *pexit;
  struct char_data * rch;
  char dirs[NUM_OF_DIRS][7];
 
  if (!has_stats_for_skill(ch, SKILL_SCAN, TRUE))
    return;
  sprintf(dirs[NORTH],"north");
  sprintf(dirs[SOUTH],"south");
  sprintf(dirs[EAST],"east");
  sprintf(dirs[WEST],"west");
  sprintf(dirs[UP],"up");
  sprintf(dirs[DOWN],"down");
  *buf = '\0';
  if (GET_SKILL(ch, SKILL_SCAN) < 1)
  {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }
  prob = GET_SKILL(ch, SKILL_SCAN);
  percent = number(1,101);
  if(FIGHTING(ch)) 
  {
    send_to_char("Wouldn't stopping to have a look round now be a bit stupid??\r\n",ch);
    return;
  }
  if(IS_AFFECTED(ch, AFF_BLIND))
  {
    send_to_char("All you can see is the blackness of blindness.\r\n", ch);
    return;
  }
  if(percent > prob) 
  {
    send_to_char("You fail to notice anything of interest.\r\n", ch);
    return;
  }
  if((IS_DARK(ch->in_room)) && (!CAN_SEE_IN_DARK(ch))) 
  {
    send_to_char ("You see lots of blackness.",ch);
    return;
  }
 
  *buf2 = '\0';
 
  send_to_char("You scan your surroundings and see the following:\r\n\n",ch);
 
  dist = GET_HEIGHT(ch) / 50; /* calc how far they can see */
  if(dist < 2)
    dist = 1; /* make sure that they can at least see 1 square */
 
  if(IS_SET(ROOM_FLAGS(ch->in_room),ROOM_INDOORS))
    dist = 0; /* if indoors you can only see the current room */
 
  if (world[ch->in_room].people != NULL)
  {
    for(rch=world[ch->in_room].people; rch!=NULL; rch=rch->next_in_room) 
    {
      if((rch==ch) || (!CAN_SEE(ch,rch)))
	continue;
      sprintf (buf,"\t\t\t  - %s\r\n",GET_NAME(rch));
      strcat(buf2,buf);
    }
 
    if(*buf != '\0') 
    {
      none = FALSE;
      send_to_char("Right here you see: \r\n",ch);
      send_to_char(buf2,ch);
    }
  }
 
  for(door=0; door<NUM_OF_DIRS; door++) 
  {
    pexit = EXIT(ch,door);
    for(sqcount=1; sqcount<dist+1; sqcount++) 
    {
      if(pexit && pexit->to_room != NOWHERE && !IS_SET(pexit->exit_info,EX_CLOSED))
      {
	if(world[pexit->to_room].people == NULL)
	  continue;
 
        *buf2 = '\0';
 
	for(rch=world[pexit->to_room].people; rch!=NULL; rch=rch->next_in_room) {
	  if((rch==ch) || (!CAN_SEE(ch,rch)))
	    continue;    
	  sprintf (buf,"\t\t\t  - %s\r\n",GET_NAME(rch));
	  strcat (buf2,buf);
	}
 
	if(*buf2 != '\0') 
	{
	  none = FALSE;
	  switch (sqcount) 
	  {
	    case 1: 
	      sprintf(buf, "Just %s of here you see:\r\n",dirs[door]);
	      break;
	    case 2: 
	      sprintf(buf, "Nearby %s of here you see:\r\n",dirs[door]);
	      break;
	    case 3: 
	      sprintf(buf, "In the distance %s of here you see:\r\n",dirs[door]);
	      break;
	    case 4:
	      sprintf(buf, "Very far %s of here:\r\n",dirs[door]);
	      break;
	    default:
	      sprintf(buf,"Somewhere far, far, %s of here you see:\r\n",dirs[door]);
	      break;
	  } 
	  send_to_char(buf,ch);
	  send_to_char(buf2,ch);
	}
 
	pexit = world[pexit->to_room].dir_option[door];
      } else {
	break;
      }
    }
  }
  if (none == TRUE)
    send_to_char ("You can't see anyone.\r\n",ch);
 
  act("$n scans $s surroundings.",FALSE,ch,0,0,TO_ROOM);
} 


ACMD(do_throw)
{
  struct char_data *vict = NULL, *tmp_char, *tch;
  struct obj_data *weap;
  char target[MAX_INPUT_LENGTH], weapon[MAX_INPUT_LENGTH];
  char direction[MAX_INPUT_LENGTH];
  int dir_num, found, calc_thaco, dam, diceroll, prob, percent;
  extern const char *dirs[];
  extern int rev_dir[];
  extern int pk_allowed;
  void check_killer(struct char_data *ch, struct char_data *vict);
  bool violence_check(struct char_data *ch,struct char_data *vict,int skillnum);
 
  half_chop(argument, weapon, buf);
 
  if (!has_stats_for_skill(ch, SKILL_THROW, TRUE))
        return;
 
  if (!generic_find(weapon, FIND_OBJ_INV, ch, &tmp_char, &weap)) {
        send_to_char("Throw what? At whom? In which direction?\r\n", ch);
        return; }
  two_arguments(buf, direction, target);
  dir_num = search_block(direction, dirs, FALSE);
  if (dir_num < 0)      {
        send_to_char("Throw in which direction?\r\n", ch);
        return; }
  if (GET_OBJ_WEIGHT(weap) > str_app[GET_AFF_STR(ch)].wield_w/2) 
  {
        send_to_char("It's too heavy to throw!\r\n", ch);
        return; 
  }
  if (LR_FAIL(ch, LVL_ANGEL))
  {
    if (OBJ_FLAGGED(weap, ITEM_NODROP))
    {
      act("You can't let go of $p!!  Yeech!", FALSE, ch, weap, 0, TO_CHAR);
      return;
    }
    if (GET_OBJ_TYPE(weap) == ITEM_REWARD)
    {
      send_to_char("What, throw your reward away?!\r\n", ch);
      return;
    }
    if (GET_OBJ_TYPE(weap) == ITEM_CONTAINER)
      for (struct obj_data *myobj = weap->contains; myobj; myobj = myobj->next_content)
      {
	if (OBJ_FLAGGED(myobj, ITEM_NODROP))
	{
	  act("You can't throw $p, it contains cursed items!", FALSE, ch, weap, 0, TO_CHAR);
	  return;
	}
	if (GET_OBJ_TYPE(myobj) == ITEM_REWARD)
	{
	  act("You can't throw $p, it contains cursed items!", FALSE, ch, weap, 0, TO_CHAR);
	  return;
	}
      }
      
  }
  found = 0;
  if (!CAN_GO(ch, dir_num))     {
    send_to_char("You can't throw things through walls!\r\n", ch);
    return;     }


  if(ROOM_FLAGGED(world[ch->in_room].dir_option[dir_num]->to_room,ROOM_HOUSE))
  {
    send_to_char("Go egg ya own house.\r\n",ch);  
    return; 
  }
  if (OBJ_RIDDEN(weap))
  {
    send_to_char("You can't throw that, its being ridden!\r\n", ch);
    return;
  } 

  for (tch = world[world[ch->in_room].dir_option[dir_num]->to_room].people;
       tch != NULL; 
       tch = tch->next_in_room)   
  {
    if (isname(target, (tch)->player.name) && CAN_SEE(ch, tch)) 
    {
        found = 1;
        vict = tch;     
    }
  }
  obj_from_char(weap);
  if (found == 0)       
  {
    sprintf(buf, "$n throws $p %s.", dirs[dir_num]);
    act(buf, FALSE, ch, weap, 0, TO_ROOM);
    sprintf(buf, "You throw $p %s.", dirs[dir_num]);
    act(buf, FALSE, ch, weap, 0, TO_CHAR);
    sprintf(buf, "%s is thrown in from the %s exit.\r\n",
        weap->short_description, dirs[rev_dir[dir_num]]);
    send_to_room(buf, world[ch->in_room].dir_option[dir_num]->to_room);
    obj_to_room(weap, world[ch->in_room].dir_option[dir_num]->to_room);
    return; 
  }
  // Artus> If we get here, we're throwing obj at someone. Violence check
  //        applies.
  if (!violence_check(ch, vict, SKILL_THROW))
    return;

  sprintf(buf, "You throw $p %s at $N.", dirs[dir_num]);
  act(buf, FALSE, ch, weap, vict, TO_CHAR);
  sprintf(buf, "$n throws $p %s.", dirs[dir_num]);
  act(buf, FALSE, ch, weap, vict, TO_ROOM);
  sprintf(buf, "$p is thrown in from the %s exit.",
dirs[rev_dir[dir_num]]);
  act(buf, FALSE, vict, weap, 0, TO_ROOM);
 
  percent = number(1, 101);
  prob = GET_SKILL(ch, SKILL_THROW);
 
  calc_thaco = thaco(ch, vict);
// ART - Dex is factored in by thaco() calc_thaco -= GET_AFF_DEX(ch)/5;
 
  dam = GET_OBJ_WEIGHT(weap) + str_app[GET_AFF_STR(ch)].todam/2;
  if (GET_OBJ_TYPE(weap) == ITEM_WEAPON)
        dam += dice(GET_OBJ_VAL(weap, 1), GET_OBJ_VAL(weap, 2))/4;
  diceroll = number(1, 20);
#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: Thaco(%s)/AC(%s)/Diceroll: %d/%d/%d\r\n", GET_NAME(ch),
	GET_NAME(vict), calc_thaco,
	GET_AC(vict), diceroll);
    send_to_char(buf, ch);
  }
  if (GET_DEBUG(vict))
  {
    sprintf(buf, "DBG: Thaco(%s)/AC(%s)/Diceroll: %d/%d/%d\r\n", GET_NAME(ch),
	GET_NAME(vict), calc_thaco, GET_AC(vict), diceroll);
    send_to_char(buf, vict);
  }
#endif
  if (((calc_thaco - diceroll) > GET_AC(vict)) || (diceroll == 1)
        || (GET_LEVEL(ch) < dam) || percent > prob) {       
    send_to_char("You missed!\r\n", ch);
    obj_to_room(weap, vict->in_room);
    act("$n threw $p at you and missed.", FALSE, ch, weap, vict, TO_VICT);
    return;
    }
  act("You strike $N with $p.", FALSE, ch, weap, vict, TO_CHAR);
  act("$n is struck by $p.", FALSE, vict, weap, 0, TO_ROOM);
 
  if (IS_AFFECTED(vict, AFF_SANCTUARY))
        dam >>= 1;
  if (!pk_allowed ||
      (!IS_SET(zone_table[world[ch->in_room].zone].zflag, ZN_PK_ALLOWED)) ||
      (!IS_SET(zone_table[world[vict->in_room].zone].zflag, ZN_PK_ALLOWED) &&
       !IS_NPC(vict))) 
  {
    check_killer(ch, vict);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != vict))
      dam = 0;
  }
  dam = damage(ch, vict, dam, TYPE_UNDEFINED, FALSE);
  sprintf(buf, "$n threw $p at you from the %s and hit for %d damage.",
                dirs[rev_dir[dir_num]], dam);
  act(buf, FALSE, ch, weap, vict, TO_VICT);
  // GET_HIT(vict) -= dam;
  obj_to_char(weap, vict, __FILE__, __LINE__);
  if (MOB_FLAGGED(vict, MOB_MEMORY) && !IS_NPC(ch))
        remember(vict, ch);
  update_pos(vict);
  if (GET_POS(vict) == POS_DEAD)
        die(vict, ch);
}
                           
ACMD(do_loadweapon)
{
  char load_message[] = "$n reloads $p.";
  int gun_ammo, max_gun_ammo, ammo, ammo_needed;
  int successful = 0;
  struct obj_data *proto_gun, *obj, *next_obj, *wielded = ch->equipment[WEAR_WIELD];
 
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
    proto_gun = read_object(GET_OBJ_RNUM(wielded), REAL);
    gun_ammo = GET_OBJ_VAL(wielded, 0);
    max_gun_ammo = GET_OBJ_VAL(proto_gun, 0);
    ammo_needed = max_gun_ammo - gun_ammo;
 
    if (OBJ_IS_GUN(wielded)) { 
      if (ammo_needed <= 0) {
        send_to_char("The weapon is already fully loaded.\n\r", ch);
        return;
      }
 
      /* look thru the players inventory and try to load the weapon */
      for (obj=ch->carrying; obj; obj=next_obj) {
        next_obj = obj->next_content;
 
        if (OBJ_RIDDEN(next_obj))
	    continue;

        if (AMMO_BELONGS_TO(obj, wielded)) {
          ammo = GET_OBJ_VAL(obj, 0);

          /* just load as much as I can into the gun */
          if (ammo < ammo_needed) { 
            GET_OBJ_VAL(wielded, 0) += ammo;
            obj_from_char(obj);
            extract_obj(obj);
            successful += ammo;
          
          } else {     /* load it up full */
            GET_OBJ_VAL(obj, 0) -= ammo_needed;
            GET_OBJ_VAL(wielded, 0) = max_gun_ammo;
            act(load_message, TRUE, ch, wielded, 0, TO_ROOM);
            send_to_char("The weapon is fully loaded now.\n\r", ch);
            if (GET_OBJ_VAL(obj, 0) <= 0) {
              obj_from_char(obj);
              extract_obj(obj);
            }
            return;
          }
        }
      }
      if (!successful)
        send_to_char("You don't seem to have the right ammo for this weapon.\n\r", ch);
      else {
        act(load_message, TRUE, ch, wielded, 0, TO_ROOM);
        if (ammo_needed)
          send_to_char("You didn't have enuough ammo to fully load the weapon.\n\r", ch);
        else
          send_to_char("The weapon is fully loaded now.\n\r", ch);
      }
    } else { /* object not gun */
      send_to_char("You cannot reload that!\n\r", ch);
    }
  }  /* object not weapon */
  else
    send_to_char("You cannot reload that!\n\r", ch);
} 

ACMD(do_retreat)
{
  int i, attempt, testroll;
 
  if (!GET_SKILL(ch, SKILL_RETREAT)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch, AFF_PARALYZED)){
     send_to_char("PANIC! Your legs refuse to move!!\r\n", ch);
     return;
  }
 
  if (IS_AFFECTED(ch, AFF_BERSERK)) {
    send_to_char("You are too entranced to retreat.\r\n", ch);
    return;
  }

  for (i = 0; i < 6; i++)
  {
    attempt = number(0, NUM_OF_DIRS - 1);       /* Select a random direction */
    if (CAN_GO(ch, attempt) &&
        !IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_DEATH))
    {
      testroll = number(0,100);
      if (testroll>=50)
      {
	if (do_simple_move(ch, attempt, TRUE, SCMD_FLEE))
	{
	  if (FIGHTING(ch))
	  {
	    if (FIGHTING(FIGHTING(ch)) == ch)
	      stop_fighting(FIGHTING(ch));
	    stop_fighting(ch);
	    act("$n strategicaly withdraws from the battle!",
		TRUE, ch, 0, 0, TO_ROOM);
	    send_to_char("You strategicaly withdraw from the battle.\r\n", ch);
	  } else {
	    act("$n strategicaly withdraws from the room!",
		TRUE, ch, 0, 0, TO_ROOM);
	    send_to_char("You strategicaly withdraw from the room.\r\n", ch);
	  }
	}
      } else {
	act("$n tries to retreat, but fails!", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("Your retreat is blocked off. PANIC!\r\n", ch);
      }
    } else {
      act("$n tries to retreat, but fails!", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("Your retreat is blocked off. PANIC!\r\n", ch);
    }
    return;
  }
} 

// DM: TODO - make a shoot skill - guncrafts or something? handle this either 
// inside do_violent_skill
ACMD(do_shoot)
{
  char shoot_message[] = "$n aims a $p at $N and starts shooting.\n\r";
  char shooter_message[] = "You aim a $p at $N and start shooting\n\r";
  char finger_mess[] = "$n points their finger at $N and says \"BANG!\".\n\r";
  char fingerer[] = "You point you finger at $N and pretend to shoot them.\n\r";
  //DM - dont know why it was hold, anyhow I changed it to wield 
  //struct obj_data *wielded = ch->equipment[WEAR_HOLD];
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  struct char_data *vict;
  int w_type;
  SPECIAL(receptionist);
  SPECIAL(postmaster);
  extern struct index_data *mob_index;
 
  one_argument(argument, arg);
 
  if (!*arg)
    send_to_char("Shoot who?\r\n", ch);
  else if (!(vict = generic_find_char(ch, arg, FIND_CHAR_ROOM)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if ((GET_MOB_SPEC(vict) == postmaster) && LR_FAIL(ch, LVL_GOD))
    send_to_char("Try SHOOTING someone who cares!!.\r\n", ch);
  else if ((GET_MOB_SPEC(vict) == receptionist) && LR_FAIL(ch, LVL_GOD))
    send_to_char("Try SHOOTING someone who cares!!.\r\n", ch);
  else if (vict == ch)
  {
    // shooting self
    if (wielded)
    {
      if (OBJ_IS_GUN(wielded))
      {
        send_to_char("You shoot yourself...OUCH!.\r\n", ch);
        act("$n shoots $mself, and dies!", FALSE, ch, 0, vict, TO_ROOM);
        raw_kill(ch,NULL);
      }
    } else {
      send_to_char("You point your finger at your head and shout \"POW!\".",
	           ch);
      act("$n points a finger at their head and shouts \"POW!\"",
	  FALSE, ch, 0, vict, TO_ROOM);
    }          
  // shooting master
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict)) {
    act("$N is just such a good friend, you simply can't shoot $M.",
	FALSE, ch, 0, vict, TO_CHAR);
  } else {
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED))
    {
      if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER))
      {
        send_to_char("Use 'murder' to shoot another player.\r\n", ch);
        return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
        return;                 /* you can't order a charmed pet to attack a
                                 * player */
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
    {
      // nothing wielded
      if (!wielded)
      {
        act(finger_mess, TRUE, ch, 0, vict, TO_ROOM);
        act(fingerer, TRUE, ch, 0, vict, TO_CHAR);
        return;
      }
      w_type = GET_OBJ_VAL(wielded, 3) & GUN_BITS;
      // non-gun weapon wielded
      if ((w_type < BASE_GUN_TYPE) || 
	  (w_type > (BASE_GUN_TYPE + MAX_GUN_TYPES)))
      {
        send_to_char("You can't shoot that!\n\r", ch);
        return;
      }
      // shoot away ...
      act(shoot_message, TRUE, ch, wielded, vict, TO_ROOM);
      act(shooter_message, TRUE, ch, wielded, vict, TO_CHAR);
      hit(ch, vict, TYPE_UNDEFINED);
      GET_WAIT_STATE(ch) = PULSE_VIOLENCE + 2;
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }
} 

#define KILL_WOLF_VNUM  22301
#define KILL_VAMP_VNUM  22302
 
/* command used to kill wolf/vamps in one shot - Vader */
ACMD(do_slay)
{
  struct char_data *vict;
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  int exp;
 
  one_argument(argument,arg);
 
  if (!*arg)
  {
    send_to_char("Which vile fiend are you intending to slay??\r\n",ch);
    return;
  }
  if (!(vict = generic_find_char(ch,arg,FIND_CHAR_ROOM)))
  {
    send_to_char("They don't seem to be here...\r\n",ch);
    return;
  }
  if (vict == ch)
  {
    send_to_char("I agree you need to die, but lets let someone else do it, shall we?\r\n",ch);
    return;
  }
  if (IS_AFFECTED(ch,AFF_CHARM) && (ch->master == vict))
  {
    act("How could you even consider slaying $N??",FALSE,ch,0,vict,TO_CHAR);
    return;
  }
  if (!wielded || 
      !(((GET_OBJ_VNUM(wielded) == KILL_WOLF_VNUM) && 
	 PRF_FLAGGED(vict,PRF_WOLF)) || 
	((GET_OBJ_VNUM(wielded) == KILL_VAMP_VNUM) &&
	 PRF_FLAGGED(vict,PRF_VAMPIRE))))
  {
    send_to_char("You need to wield the right weapon to do a proper job.\r\n",
	         ch);
    return;
  }
  if (!affected_by_spell(vict,SPELL_CHANGED))
  {
    send_to_char("Maybe you should wait til they change before you slay them.\r\n",ch);
    return;
  }
  if (IS_AFFECTED(ch,AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
    return;
  if((PRF_FLAGGED(ch,PRF_WOLF) && PRF_FLAGGED(vict,PRF_WOLF)) ||
     (PRF_FLAGGED(ch,PRF_VAMPIRE) && PRF_FLAGGED(vict,PRF_VAMPIRE)))
  {
    send_to_char("Attempting to kill your own kind! How dare you?!\r\n",ch);
    SET_BIT(PLR_FLAGS(ch),PLR_KILLER);
    sprintf(buf, "PC Killer bit set on %s for slaying kinsman %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    mudlog(buf, BRF, LVL_ANGEL, TRUE);
  }
  if(PRF_FLAGGED(vict,PRF_WOLF))
  {
    if((number(0,2) < 2) && LR_FAIL(ch, LVL_GOD))
    {
      act("You drive $p deep into $N's ribcage!",
	  FALSE, ch, wielded, vict, TO_CHAR);
      act("$n drives $p deep into $N's ribcage!",
	  FALSE, ch, wielded, vict, TO_NOTVICT); 
      act("$n drives $p deep into your ribcage, killing the beast within you.",
	  FALSE, ch, wielded, vict, TO_VICT);
      act("$n changes back to normal as $e dies.",
	  FALSE, vict, 0, 0, TO_ROOM);
      send_to_char("You revert to your original form as you die.\r\n",vict);
      exp = GET_LEVEL(vict) * 1000; /* give em some xp for it */
      gain_exp(ch,exp);
      sprintf(buf,"You receive &c%d&n experience points.\r\n",exp);
      send_to_char(buf,ch);
      sprintf(buf,"%s (werewolf) slain by %s",GET_NAME(vict),GET_NAME(ch));
      mudlog(buf,BRF,LVL_GOD,TRUE);
      gain_exp(vict,-(exp * 4));
      raw_kill(vict,ch);
      send_to_outdoor("The howl of a dying werewolf echoes across the land.\r\n");
      return;
    }
    act("Too slow! $N sees you, spins round, and rips your face off.",
	FALSE, ch, wielded, vict, TO_CHAR);
    act("$N dodges $n's attack, spins round, and rips $s face off.",
	FALSE, ch, wielded, vict, TO_NOTVICT);
    act("You see $n coming from miles away, and quickly do away with $m.",
	FALSE, ch, wielded, vict, TO_VICT);
    act("$n is dead!  R.I.P.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("You are dead!  Sorry...\r\n",ch);
    exp = GET_LEVEL(ch) * 1000; /* you get xp for dodging */
    gain_exp(vict,exp);
    sprintf(buf,"You receive &c%d&n experience points.\r\n",exp);
    send_to_char(buf,vict);
    sprintf(buf,"%s killed while attempting to slay %s (werewolf)",
	    GET_NAME(ch),GET_NAME(vict));
    mudlog(buf,BRF,LVL_GOD,TRUE);
    gain_exp(ch,-(exp * 4));
    raw_kill(ch,vict);
    return;
  }
  if(PRF_FLAGGED(vict,PRF_VAMPIRE))
  {
    if((number(0,2) < 2) && LR_FAIL(vict, LVL_GOD))
    {
      act("You stab $p firmly into $N's heart!",
	  FALSE, ch, wielded, vict, TO_CHAR);
      act("$n stabs $p firmly into $N's heart!",
	  FALSE, ch, wielded, vict, TO_NOTVICT);
      act("$n stabs $p firmly into your heart!",
	  FALSE, ch, wielded, vict, TO_VICT);
      act("$n explodes.", FALSE, vict, 0, 0, TO_ROOM);
      send_to_char("You explode violently as the spell is broken.\r\n", vict);
      exp = GET_LEVEL(vict) * 1000;
      gain_exp(ch,exp);
      sprintf(buf,"You receive &c%d&n experience points.\r\n",exp);
      send_to_char(buf,ch);
      sprintf(buf,"%s (vampire) slain by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf,BRF,LVL_GOD,TRUE);
      gain_exp(vict,-(exp * 4));
      raw_kill(vict,ch);
      send_to_outdoor("You hear the sound of an exploding vampire in the distance.\r\n");
      return;
    }
    act("$N is suddenly behind you! You feel $S teeth sink into your neck...",
	FALSE, ch, 0, vict, TO_CHAR);
    act("$N dodges $n's attack, grabs $m from behind and bites $s neck!",
	FALSE, ch, 0, vict, TO_NOTVICT);
    act("You dodge $n's attack, grab $m by the neck, and drink.",
	FALSE, ch, 0, vict, TO_VICT);
    act("$n is dead!  R.I.P.", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("You are dead!  Sorry...\r\n",ch);
    exp = GET_LEVEL(ch) * 1000;
    gain_exp(vict,exp);
    sprintf(buf,"You receive &c%d&n experience points.\r\n",exp);
    send_to_char(buf,vict);
    sprintf(buf,"%s killed while attempting to slay %s (vampire)",
	    GET_NAME(ch),GET_NAME(vict));
    mudlog(buf,BRF,LVL_GOD,TRUE);
    gain_exp(ch,-(exp * 4));
    raw_kill(ch,vict);
    return;
  }
}

/*
ACMD(do_headbutt)
{
  struct char_data *vic t ;
  int prob, percent;
  

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HEADBUTT)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
     
  if (!has_stats_for_skill(ch, SKILL_HEADBUTT, TRUE))
    return;
       
  one_argument(argument, arg);
         
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
        vict = FIGHTING(ch);
    } else {
      send_to_char("Headbutt who?\r\n", ch);
      return;
    }
  } 

  // Check if the victim is mount and mounted and not already fighting it
  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
    send_to_char("That's someone's mount! Use 'murder' to attack another player.\r\n", ch);
    return;
  }
  
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } 

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) &&
      !IS_NPC(vict)) {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n tries to headbutt $N, and fails miserably!",FALSE, ch, 0, 
        vict, TO_NOTVICT);
    return;
  }
     
  // 101% is a complete failure 
  percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
  prob = GET_SKILL(ch, SKILL_HEADBUTT);
         
  if (IS_AFFECTED(ch, AFF_ADRENALINE))
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
  else
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;

  // DM - TODO determine percent, make it damage ch if they are not wearing a 
  // helmet, determine damage inflicted, messages 
  if (percent > prob) {
    damage(ch, vict, 0, SKILL_HEADBUTT);
  } else
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_HEADBUTT);
                   
}
*/

/*
ACMD(do_piledrive)
{
  struct char_data *vict;
  int prob, percent;
  

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_PILEDRIVE)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
     
  if (!has_stats_for_skill(ch, SKILL_PILEDRIVE, TRUE))
    return;
       
  one_argument(argument, arg);
         
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
        vict = FIGHTING(ch);
    } else {
      send_to_char("Piledrive who?\r\n", ch);
      return;
    }
  } 

  // Check if the victim is mount and mounted and not already fighting it
  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
    send_to_char("That's someone's mount! Use 'murder' to attack another player.\r\n", ch);
    return;
  }
  
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } 

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) &&
      !IS_NPC(vict)) {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n tries to piledrive $N, and fails miserably!",FALSE, ch, 0, 
        vict, TO_NOTVICT);
    return;
  }
     
  // 101% is a complete failure 
  percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
  prob = GET_SKILL(ch, SKILL_PILEDRIVE);
         
  if (IS_AFFECTED(ch, AFF_ADRENALINE))
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
  else
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;

  // DM - TODO determine percent, determine damage inflicted, messages 
  if (percent > prob) {
    damage(ch, vict, 0, SKILL_PILEDRIVE);
  } else
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_PILEDRIVE);
                   
}
*/

ACMD(do_trap)
{
  int damage_val = 0;
  int trap_num = 0, i;
  struct obj_data *trap;
  int calc_dam_amt(struct char_data *ch, struct char_data *vict, int skillnum);
  int spring_trap(struct char_data *ch, struct obj_data *obj);

  const sh_int magic_values[65] = {
    SPELL_CALL_LIGHTNING, SPELL_CALL_LIGHTNING, SPELL_CHILL_TOUCH,   // 1-3
    SPELL_CHILL_TOUCH,    SPELL_CHILL_TOUCH,    SPELL_COLOR_SPRAY,   // 4-6
    SPELL_COLOR_SPRAY,    SPELL_CURE_BLIND,     SPELL_CURE_CRITIC,   // 7-9
    SPELL_CURE_CRITIC,    SPELL_CURE_CRITIC,    SPELL_CURE_LIGHT,    // 10-12
    SPELL_CURE_LIGHT,     SPELL_CURE_LIGHT,     SPELL_CURSE,         // 13-15
    SPELL_CURSE,          SPELL_DISPEL_EVIL,    SPELL_DISPEL_EVIL,   // 16-18
    SPELL_ENERGY_DRAIN,   SPELL_ENERGY_DRAIN,   SPELL_FIREBALL,      // 19-21
    SPELL_FIREBALL,       SPELL_HARM,           SPELL_HARM,          // 22-24
    SPELL_HEAL,           SPELL_HEAL,           SPELL_LIGHTNING_BOLT,// 25-27
    SPELL_LIGHTNING_BOLT, SPELL_LIGHTNING_BOLT, SPELL_MAGIC_MISSILE, // 28-30
    SPELL_MAGIC_MISSILE,  SPELL_MAGIC_MISSILE,  SPELL_MAGIC_MISSILE, // 31-33
    SPELL_MAGIC_MISSILE,  SPELL_MAGIC_MISSILE,  SPELL_POISON,        // 34-36
    SPELL_POISON,         SPELL_REMOVE_CURSE,   SPELL_REMOVE_CURSE,  // 37-39
    SPELL_SHOCKING_GRASP, SPELL_SHOCKING_GRASP, SPELL_SHOCKING_GRASP,// 40-42
    SPELL_SHOCKING_GRASP, SPELL_SLEEP,          SPELL_SLEEP,         // 43-45
    SPELL_WORD_OF_RECALL, SPELL_WORD_OF_RECALL, SPELL_REMOVE_POISON, // 46-48
    SPELL_DISPEL_GOOD,    SPELL_DISPEL_GOOD,    SPELL_FINGERDEATH,   // 49-51
    SPELL_ADV_HEAL,       SPELL_REFRESH,        SPELL_REFRESH,       // 52-54
    SPELL_FEAR,           SPELL_FEAR,           SPELL_PLASMA_BLAST,  // 55-57
    SPELL_PLASMA_BLAST,   SPELL_PARALYZE,       SPELL_PARALYZE,      // 58-60
    SPELL_REMOVE_PARA,    SPELL_WRAITH_TOUCH,   SPELL_MANA,          // 61-63
    SPELL_DIVINE_HEAL,    SPELL_GREATER_REMOVE_CURSE };              // 64-65
    
  
  one_argument(argument,arg);
  if (!*arg)
  {
    send_to_char("Just what kind of trap were you hoping to build? Type 'trap list' for a list.\r\n", ch);
    return;
  }
  if (is_abbrev(arg, "list"))
  {
    sprintf(buf, "You are able to build the following traps:\r\n");
    for (i = 1; i < NUM_TRAPS; i++)
      if (has_stats_for_skill(ch, trap_types[i].skillnum, FALSE))
      {
	if (trap_num > 0) 
	  strcat(buf, ", ");
	else
	  strcat(buf, "    ");
	strcat(buf, trap_types[i].trap_desc);
	trap_num++;
      }
    if (trap_num <= 0)
    {
      send_to_char("You are unable to build any traps.\r\n", ch);
      return;
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  } // is list

  for (i = 1; i < NUM_TRAPS; i++)
    if (is_abbrev(arg, trap_types[i].trap_desc))
    {
      trap_num = i;
      break;
    }
  if ((trap_num < 1) || (trap_num >= NUM_TRAPS))
  {
    send_to_char("Unknown trap type specified. Try 'trap list' for a list of available traps.\r\n", ch);
    return;
  }
  // Test Trap Flags...
  if (trap_types[trap_num].flag == 0)
  {
    send_to_char("This trap type appears to be broken.\r\n", ch);
    return;
  }
  if (!IS_SET(trap_types[trap_num].flag, TRAP_INDOORS) && !OUTSIDE(ch))
  {
    sprintf(buf, "You cannot build a %s trap inside!\r\n", 
	    trap_types[trap_num].trap_desc);
    send_to_char(buf, ch);
    return;
  }
  if (!IS_SET(trap_types[trap_num].flag, TRAP_OUTDOORS) && OUTSIDE(ch))
  {
    sprintf(buf, "You cannot build a %s trap outside!\r\n", 
	    trap_types[trap_num].trap_desc);
    send_to_char(buf, ch);
    return;
  }
  if (!IS_SET(trap_types[trap_num].flag, TRAP_UNDERWATER) && UNDERWATER(ch))
  {
    sprintf(buf, "You cannot build a %s trap underwater!\r\n",
	    trap_types[trap_num].trap_desc);
    send_to_char(buf, ch);
    return;
  }
  if (!IS_SET(trap_types[trap_num].flag, TRAP_FLYING) && 
      (BASE_SECT(world[ch->in_room].sector_type) == SECT_FLYING))
  {
    sprintf(buf, "You cannot build a %s trap in mid-air!\r\n",
	    trap_types[trap_num].trap_desc);
    send_to_char(buf, ch);
    return;
  }
  // Test Timers...
  if (!(char_affected_by_timer(ch, TIMER_TRAP_PIT))) 
    timer_to_char(ch,timer_new(TIMER_TRAP_PIT));
  if (timer_use_char(ch,TIMER_TRAP_PIT))
  {
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
  } else {
    send_to_char(RESTSKILL,ch);
    return;
  }
  // Ok, It's possible to build the trap.. Lets do it.
  if (basic_skill_test(ch, trap_types[trap_num].skillnum, 0) == 0)
    return;
  switch (trap_num)
  {
    case TRAP_PIT:
      damage_val = calc_dam_amt(ch, ch, SKILL_TRAP_PIT);
      break;
    case TRAP_MAGIC:
      damage_val = magic_values[number(0, 64)];
      break;
    default:
      send_to_char("This trap type appears to be broken.\r\n", ch);
      return;
  }

  trap = read_object(TRAP_OBJ, VIRTUAL);
  GET_OBJ_VAL(trap, 0) = trap_num;
  GET_OBJ_VAL(trap, 1) = damage_val;
  GET_OBJ_VAL(trap, 2) = GET_IDNUM(ch);
  GET_OBJ_VAL(trap, 3) = GET_LEVEL(ch);
  obj_to_room(trap, ch->in_room);
  if (number(0, 22) > GET_DEX(ch))
  {
    sprintf(buf, "As you are finishing your %s trap, you slip and fall into it.\r\n", trap_types[trap_num].trap_desc);
    send_to_char(buf, ch);
    sprintf(buf, "$n falls into a %s trap of $s own creation.",
	    trap_types[trap_num].trap_desc);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    spring_trap(ch, trap);
    return;
  }
  sprintf(buf, "You set a %s trap here.\r\n", trap_types[trap_num].trap_desc);
  send_to_char(buf, ch);
}

/*
ACMD(do_trip)
{
  struct char_data *vict;
  int prob, percent;
  

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRIP)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
     
  if (!has_stats_for_skill(ch, SKILL_TRIP, TRUE))
    return;
       
  one_argument(argument, arg);
         
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
        vict = FIGHTING(ch);
    } else {
      send_to_char("Trip who?\r\n", ch);
      return;
    }
  } 

  // Check if the victim is mount and mounted and not already fighting it
  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
    send_to_char("That's someone's mount! Use 'murder' to attack another player.\r\n", ch);
    return;
  }
  
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } 

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) &&
      !IS_NPC(vict)) {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n tries to trip $N, and fails miserably!",FALSE, ch, 0, 
        vict, TO_NOTVICT);
    return;
  }
  
  if (GET_POS(vict) == POS_SITTING) {
    send_to_char("They are already down.\r\n",ch);
    return;
  }
     
  // 101% is a complete failure 
  percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
  prob = GET_SKILL(ch, SKILL_TRIP);
         
  if (IS_AFFECTED(ch, AFF_ADRENALINE))
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
  else
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;

  // DM - TODO determine percent, determine damage inflicted, messages 
  if (percent > prob) {
    damage(ch, vict, 0, SKILL_TRIP);
  } else {
    // Fix so vict cant flee as they should be sitting ...
    GET_POS(vict) = POS_SITTING;
    GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 2;
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_TRIP); 
  }
}
*/

/*
ACMD(do_bearhug)
{
  struct char_data *vict;
  int prob, percent;
  

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BEARHUG)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
     
  if (!has_stats_for_skill(ch, SKILL_BEARHUG, TRUE))
    return;
       
  one_argument(argument, arg);
         
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
        vict = FIGHTING(ch);
    } else {
      send_to_char("Bearhug who?\r\n", ch);
      return;
    }
  } 

  // Check if the victim is mount and mounted and not already fighting it
  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
    send_to_char("That's someone's mount! Use 'murder' to attack another player.\r\n", ch);
    return;
  }
  
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } 

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) &&
      !IS_NPC(vict)) {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n tries to bearhug $N, and fails miserably!",FALSE, ch, 0, 
        vict, TO_NOTVICT);
    return;
  }
     
  // 101% is a complete failure 
  percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
  prob = GET_SKILL(ch, SKILL_HEADBUTT);
         
  if (IS_AFFECTED(ch, AFF_ADRENALINE))
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
  else
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;

  // DM - TODO determine percent, determine damage inflicted, messages 
  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BEARHUG);
  } else
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_BEARHUG);
                   
}
*/

/*
ACMD(do_battlecry)
{
  struct char_data *vict;
  bool kick = TRUE, bearhug = FALSE, headbutt = TRUE, 
      piledrive = FALSE, bash = TRUE;
  int prob, percent;
 
  one_argument(argument, arg);

  
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BATTLECRY)) {
    send_to_char(UNFAMILIARSKILL,ch);
    return;
  }

  if (!has_stats_for_skill(ch, SKILL_BATTLECRY, TRUE))
      return;
  
  if (!*arg) {
    send_to_char("Do a battlecry on who?\r\n",ch);
    return;
  }

  if (!(vict=get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    send_to_char(NOPERSON,ch);
    return;  
  }
  
  if (ch == vict) {
    send_to_char("Oh, your one of those comedians, right?\r\n",ch);
    return;  
  }
  
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char(PEACEROOM,ch);
    return;
  }

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) &&
      !IS_NPC(vict)) {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n attempts a battlecry on $N, and fails miserably!",FALSE, ch, 0, 
        vict, TO_NOTVICT);
    return;
  }
  
  prob = GET_SKILL(ch, SKILL_BATTLECRY);
  percent=number(1,101);
  
  // DM - TODO - decide on chances of getting skills
  // kick, bearhug, headbutt, piledrive, trip, bash
  // dex   str      str       str        dex   str
  if (percent > prob) { 
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
    hit(ch, vict, TYPE_UNDEFINED);
  } else {
    act("You scream 'AAARRGGGG' as you perform a battlecry on $N!", 
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n screams 'AAARRGGGG' as $m performs $s battlecry on you!", 
        TRUE, ch, 0, vict, TO_VICT);
    act("$n screams 'AAARRGGGG' as $m performs $s battlecry on $N!", 
        TRUE, ch, 0, vict, TO_NOTVICT);

    if (kick && vict && IN_ROOM(ch) == IN_ROOM(vict) && 
        GET_SKILL(ch, SKILL_KICK) && 
	has_stats_for_skill(ch, SKILL_KICK, FALSE)) {
      sprintf(buf, " %s", arg);
      do_kick(ch, buf, 0, 0);
    }
      
    if (bearhug && vict && IN_ROOM(ch) == IN_ROOM(vict) && 
        GET_SKILL(ch, SKILL_BEARHUG) && 
	has_stats_for_skill(ch, SKILL_BEARHUG, FALSE)){
      sprintf(buf, " %s", arg);
      do_bearhug(ch, buf, 0, 0);
    }

    if (headbutt && vict && IN_ROOM(ch) == IN_ROOM(vict) &&
        GET_SKILL(ch, SKILL_HEADBUTT) && 
        has_stats_for_skill(ch, SKILL_HEADBUTT, FALSE)) {
      sprintf(buf, " %s", arg);
      do_headbutt(ch, buf, 0, 0); 
    }

    if (piledrive && vict && IN_ROOM(ch) == IN_ROOM(vict) && 
        GET_SKILL(ch, SKILL_PILEDRIVE) && 
        has_stats_for_skill(ch, SKILL_PILEDRIVE, FALSE)) {
      sprintf(buf, " %s", arg);
      do_piledrive(ch, buf, 0, 0);
    }

    if (bash && vict && IN_ROOM(ch) == IN_ROOM(vict) && 
        GET_SKILL(ch, SKILL_BASH) && 
	has_stats_for_skill(ch, SKILL_BASH, FALSE) && 
        (ch->equipment[WEAR_WIELD])) { 
      sprintf(buf, " %s", arg);
      do_bash(ch, buf, 0, 0);
    }

    GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 4;
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
  }
}
*/

/*
ACMD(do_bodyslam)
{
  struct char_data *vict, *was_fighting;
  int prob, percent, attempt, i;
  

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BODYSLAM)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
     
  if (!has_stats_for_skill(ch, SKILL_BODYSLAM, FALSE))
    return;
       
  one_argument(argument, arg);
         
  if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
        vict = FIGHTING(ch);
    } else {
      send_to_char("Bodyslam who?\r\n", ch);
      return;
    }
  } 

  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char(PEACEROOM, ch);
    return;
  }

  // Check if the victim is mount and mounted and not already fighting it
  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
    send_to_char("That's someone's mount! Use 'murder' to attack another player.\r\n", ch);
    return;
  }
  
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } 

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) &&
      !IS_NPC(vict)) {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n tries to bodyslam $N, and fails miserably!",FALSE, ch, 0, 
        vict, TO_NOTVICT);
    return;
  }
     
  // 101% is a complete failure 
  percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
  prob = GET_SKILL(ch, SKILL_BODYSLAM);
         
  GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

  // DM - TODO determine percent, determine damage inflicted, messages 
  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BODYSLAM);
  } else {
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_BODYSLAM);

    if (vict && IN_ROOM(ch) == IN_ROOM(vict)) {
      for (i = 0; i < 6; i++) {
        attempt = number(0, NUM_OF_DIRS - 1);    // Select a random direction 
        if (CAN_GO(vict, attempt) && !ROOM_FLAGGED(EXIT(vict, attempt)->to_room,
            ROOM_DEATH)) {
          act("$N is bodyslammed from the room by $n!", TRUE, ch, 0, vict, 
              TO_ROOM);
          act("You bodyslam $N out of the room!", TRUE, ch, 0, vict, TO_CHAR);
          if (do_simple_move(vict, attempt, TRUE)) {
            act("You are bodyslammed out of the room by $n!", TRUE, ch, 0, vict,
                TO_VICT);

            stop_fighting(vict);
            stop_fighting(ch);
            GET_WAIT_STATE(ch) = 0;
            return;
          }
        }
      }
      act("$N bounces back into the room from $n's bodyslam!", 
          TRUE, ch, 0, vict, TO_ROOM);
      act("$N bounces back into the room from your bodyslam!", 
          TRUE, ch, 0, vict, TO_CHAR);
      act("You bounce back into the room from $n's bodyslam!", 
          TRUE, ch, 0, vict, TO_VICT);
    }
  }
}
*/

// Need to allocate an area for this, or find a better way to do it
#define BURGLE_ROOMS_START	30900
#define BURGLE_ROOMS_END	30999

/* Test the 'area', for burgling */
int area_warehousing(room_rnum r) {

	if( BURGLE_FLAGGED(r, ROOM_AREA_WAREHOUSE_RICH) ||
	    BURGLE_FLAGGED(r, ROOM_AREA_WAREHOUSE_REGULAR) ||
	    BURGLE_FLAGGED(r, ROOM_AREA_WAREHOUSE_POOR) )
	 	return 1;

	return 0;
}

/* gives opposite dir number */
int reverse_dir(int dir) {
	if( dir <= 3 )
		return (dir + 2) % 4; // NESW -> SWNE
	else if( dir == 4 )
		return 5;  // Down
	
	return 4;	// Up is reverse 
}

int area_suburban(room_rnum r) {

	if( BURGLE_FLAGGED(r, ROOM_AREA_HOME_RICH) ||
	    BURGLE_FLAGGED(r, ROOM_AREA_HOME_REGULAR) ||
	    BURGLE_FLAGGED(r, ROOM_AREA_HOME_POOR) )
		return 1;

	return 0;
}
int area_shop(room_rnum r) {
	
	if( BURGLE_FLAGGED(r, ROOM_AREA_SHOP_RICH) ||
	    BURGLE_FLAGGED(r, ROOM_AREA_SHOP_REGULAR ) ||
	    BURGLE_FLAGGED(r, ROOM_AREA_SHOP_POOR) )
		return 1;

	return 0;
}

/* Returns the type of area being burgled */
int get_burgle_area(room_rnum r) {

	if( BURGLE_FLAGGED(r, ROOM_AREA_WAREHOUSE_POOR) ) 
		return ROOM_AREA_WAREHOUSE_POOR;
	if( BURGLE_FLAGGED(r, ROOM_AREA_WAREHOUSE_REGULAR) ) 
		return ROOM_AREA_WAREHOUSE_REGULAR;
	if( BURGLE_FLAGGED(r, ROOM_AREA_WAREHOUSE_RICH) ) 
		return ROOM_AREA_WAREHOUSE_RICH;
	if( BURGLE_FLAGGED(r, ROOM_AREA_SHOP_POOR) )
		return ROOM_AREA_SHOP_POOR;
	if( BURGLE_FLAGGED(r, ROOM_AREA_SHOP_REGULAR) )
		return ROOM_AREA_SHOP_REGULAR;
	if( BURGLE_FLAGGED(r, ROOM_AREA_SHOP_RICH) )
		return ROOM_AREA_SHOP_RICH;
	if( BURGLE_FLAGGED(r, ROOM_AREA_HOME_POOR) )
		return ROOM_AREA_HOME_POOR;
	if( BURGLE_FLAGGED(r, ROOM_AREA_HOME_REGULAR) )
		return ROOM_AREA_HOME_REGULAR;
	if( BURGLE_FLAGGED(r, ROOM_AREA_HOME_RICH) )
		return ROOM_AREA_HOME_RICH;

	return 0;
}

bool is_burgling(struct char_data * ch)
{
   Burglary *tmp = burglaries;
   long id = GET_IDNUM(ch);

   while (tmp != NULL)
   {
	if (id == tmp->chID)
		return TRUE;

	tmp = tmp->next;
   }
   return FALSE;
}

/* Functions gives xp according to player's level and the 
   wealthiness of the burgled area */
void burgle_exp(struct char_data *ch, int areatype) 
{
/*
  if (GET_LEVEL(ch) >= LVL_IMMORT)
	return;
*/
}

ACMD(do_burgle)
{

  int position = GET_POS(ch);
  int dir = 0, area = 0;
  room_rnum newroom;

  // "Can't burgle here" for each sector type
  const char *non_burgle_msg[] = {
	"You're already inside!\r\n", 
	"Better not, the neighbours are watching.\r\n",
	"You break into a rabbit hole.\r\n",
	"You disrupt some squirrels as you try to steal their nuts.\r\n",
	"You pocket some stones, ecstatic at your find.\r\n", 
	"Nothing out here but mountain goats.\r\n",
	"You quickly put as much water into your backpack as you can.\r\n",
  	"You quickly put as much water into your backpack as you can.\r\n",
  	"You're too busy flapping your arms trying to stay afloat?\r\n",
	"Wouldn't want to upset Poseidon. Touchy sort of fellow.\r\n"
  };

  // Know the skill?
  if( !GET_SKILL(ch, SKILL_BURGLE) ) {
	send_to_char("Better leave that sort of thing to the professionals.\r\n", ch);
	return;
  }

  // Fighting?
  if(position == POS_FIGHTING)  {
	send_to_char("You're too distracted at the moment.\r\n", ch);
  	return;
  }

  // Paralysed?
  if( AFF_FLAGGED(ch, AFF_PARALYZED)) {
	send_to_char("You try to burgle, but find your limbs won't respond. You're paralysed!\r\n", ch);
  	return;
  }

  // Get the type of area 
  area = get_burgle_area(ch->in_room);

/* Replace this with a high skill check
  // Quick check to allow only burglar's to burgle rich areas
  if( ( area == ROOM_AREA_WAREHOUSE_RICH || area == ROOM_AREA_HOME_RICH 
     || area == ROOM_AREA_SHOP_RICH) && ( class != CLASS_BURGLAR ) ){
	send_to_char("This area is too well protected for your piddling burgling skills.\r\n",ch);
  	return;
  } 
*/
  // Correct eq?
  if( !find_obj_list(ch, "toolkit", ch->carrying) )
  {
	send_to_char("Need the right tools for the right job.\r\n", ch);
	return;
  }

  if (is_burgling(ch))
  {
	send_to_char("You've been busy recently, better wait a bit longer.\r\n", ch);
	return;
  }
  

  // Given a direction?
  one_argument(argument, arg);

  if( !*arg )
  {
	send_to_char("Which direction would you like to burgle in??\r\n", ch);
	return;	
  }

  // Evaluate arg
  switch( LOWER(arg[0]) )
  {
	case 'u': 
	case 'd': send_to_char("Very funny.\r\n", ch); return;
	case 'e': dir = 1; break;
	case 'w': dir = 3; break;
	case 'n': dir = 0; break;
	case 's': dir = 2; break;
	default: 
	  send_to_char("What kind of a direction is that?!\r\n", ch);
	  return;
  }

  // Is that a blank wall?
  if( world[ch->in_room].dir_option[dir] != NULL 
   && world[ch->in_room].dir_option[dir]->to_room != NOWHERE ) {
	send_to_char("You can't burgle there!\r\n", ch);
	return;
  }

  // Reverse the direction, to create the exit from the burgled place
  dir = reverse_dir(dir);
  bool bBurgle = FALSE;

  // Is it a warehousing area?
  if( area_warehousing(ch->in_room)  ) {
	send_to_char("You begin to burgle into a warehouse.\r\n",ch);
	bBurgle = TRUE;
  }
  // Is it suburban?
  if( area_suburban(ch->in_room)  ) {
  	send_to_char("You begin to burgle into someone's home.\r\n",ch);
	bBurgle = TRUE;
  }

  // Shopping area?
  if( area_shop(ch->in_room)) {
       
        if ((time_info.hours >= 0 && time_info.hours < 6))
        {
  	  send_to_char("Breaking into a store at any time except night would be suicide!\r\n", ch);
        }
	else
	{
	  send_to_char("Against your better judgement, you begin to burgle into a store.\r\n", ch);
	  bBurgle = TRUE;
	}
  }

 
  if (bBurgle)
  {
	if( (newroom = create_burgle_rooms(ch, area, dir)) != NOWHERE ) {
		char_from_room(ch);
		char_to_room(ch, newroom);
		look_at_room(ch, TRUE);

		// Give some XP as well
		burgle_exp(ch, area);
	}
	else 
	  send_to_char("Unable to break in, you give up in disgust.\r\n",ch);
  }
  else
    // None, not a valid area, give an appropriate message.
    send_to_char(non_burgle_msg[SECT(ch->in_room)], ch);

}

/* BURGLE CODE, the real work */
/* Function to return a semi random name of a room, given a type */
char *rand_name(int type) {

  int nTime = time_info.hours;

  switch(type)
  {
    case ROOM_WAREHOUSE:
	if (nTime >= 0 && nTime < 6)
		return "A dark warehouse";
	if (nTime >= 6 && nTime < 19)
		return "A busy warehouse";

	return "A poorly lit warehouse";
    case ROOM_HOME:
	if (nTime >= 0 && nTime < 6)
		return "A dark house";
	if (nTime >= 6 && nTime < 19)
		return "Someone's home";
	
	return "A family home";
    case ROOM_SHOP:
	if (nTime >= 0 && nTime < 6)
		return "An empty shop";
	if (nTime >= 6 && nTime < 22)
		return "A busy shop";
	
	return "A closing shop"; 

     default:
	return "???";
  }
}

/* Function to return a semi random description, given a type */
char *rand_desc(int type) {

  int nTime = time_info.hours;

  switch(type)
  {
    case ROOM_WAREHOUSE:
	if (nTime >= 0 && nTime < 6)
	  return "Little can be seen through the darkness, "
		 "utensils and tools litter the floor where "
		 "the workers left them before leaving for "
		 "the night. The place is at your mercy!\r\n";
	if (nTime >= 6 && nTime < 19)
	  return "Workers are hard at work with their tasks "
		 "all around you as you try and pass off "
		 "as one of them. With some luck and skill "
		 "you should be able to slip a few tidbits "
		 "away for later.\r\n";
	return   "Few workers remain at this time, some "
		 "tidying up and finishing their tasks "
		 "before leaving for home. It should be "
		 "easy to get a few mementos, but it's "
		 "getting more and more likely that you will "
		 "be noticed.\r\n";
    case ROOM_HOME:
	if (nTime >= 0 && nTime < 6)
	  return "No lights are on to show your way through "
		 "this private home in the dead of night. "
		 "Some small sounds coming from various parts "
		 "of the house must be its tenants, blissfully "
		 "unaware of the prowler in their midst.\r\n";
	if (nTime >= 6 && nTime < 19)
	  return "You try to be unobtrusive as you make your way "
		 "through this private residence in broad daylight "
		 "Your skills had better be in tip top shape or "
		 "this could be a costly visit. None the less, "
		 "the place looks like it could have a few "
		 "interesting items lying about.\r\n";
	return   "You stick to shadows and stalk quietly through "
		 "this private residence as twilight threatens to "
		 "become night. Some sounds can be heard as "
		 "the residents go about their business, but with some "
		 "luck and skill a few interesting items might "
		 "be rescued.\r\n";
    case ROOM_SHOP:
	if (nTime >= 0 && nTime < 6)
	  return "Small glimmers of light reveal the abundance "
		 "of treasures that this shop can add to your "
		 "private stash. Through the dead of night you "
		 "rifle into the stores and grin with glee at "
		 "the possibilities. ...the possibility of capture "
		 "in this dangerous place and the punishment it "
		 "would bring is close to mind as well.\r\n";

	return  "Treasures are all around you as you burgle into "
		"a shop during the day, but you know it's only a "
		"short matter of time until the law is onto you and "
		"your life forfeit, and the joy is short lived.\r\n";
    default: 
	return "Not much can be seen...\r\n";
  }

}

bool is_exit_available(long room, int dir)
{
  if (world[real_room(room)].dir_option[dir] != NULL &&
      world[real_room(room)].dir_option[dir]->to_room != NOWHERE)
      return FALSE;
 
  return TRUE;
} 

int has_available_exit(long room, short limit)
{
  for (int i = 0; i < limit; i++)
  {
     if (world[real_room(room)].dir_option[i] != NULL &&
         world[real_room(room)].dir_option[i]->to_room != NOWHERE)
	continue;

     return i;
  }

  return -1;
}

int get_random_exit(long lRoom)
{
  // Firstly, see if there are any available exits
  if (has_available_exit(lRoom, 4) == -1)
	return -1;
  
  bool bSelectedExit = FALSE;
  int nDir = 0;

  do {
    nDir = number( 0, 3 );
    if (is_exit_available(lRoom, nDir))
	bSelectedExit = TRUE;
  } while(!bSelectedExit);

  return nDir;
}

bool attach_rooms(long lCurrentRoom, long lConnectRoom)
{
    // If current room has no available exit, no use continuing
    if (has_available_exit(lCurrentRoom, 4) == -1 || 
	has_available_exit(lConnectRoom, 4) == -1)
    {
	return FALSE;
    }

    int  nRealConnect = real_room(lConnectRoom), 
	 nRealCurrent = real_room(lCurrentRoom); 

    int nDir = -1, nCount = 0;

    do {
	nCount++;
	// Get a random exit direction
	nDir = number(0,3);
	if (!is_exit_available(lCurrentRoom, nDir) || 
	    !is_exit_available(lConnectRoom, reverse_dir(nDir)) )
		nRealConnect = nRealCurrent;

    } while( nRealConnect == nRealCurrent && nCount < 10);

    if (nCount >= 10)
    {
	return FALSE;
    }

    // Attach the two rooms
   world[nRealCurrent].dir_option[nDir]->exit_info = EX_ISDOOR; // | EX_CLOSED;
   world[nRealCurrent].dir_option[nDir]->to_room = nRealConnect;
   world[nRealCurrent].dir_option[nDir]->keyword = str_dup(dirs[nDir]);
   
   nDir = reverse_dir(nDir);
   world[nRealConnect].dir_option[nDir]->exit_info = EX_ISDOOR; // | EX_CLOSED;
   world[nRealConnect].dir_option[nDir]->to_room = nRealCurrent;
   world[nRealConnect].dir_option[nDir]->keyword = str_dup(dirs[nDir]);
 
   return TRUE;
    
}

Burglary *create_new_burglary(struct char_data *ch)
{
	Burglary *burglary = new Burglary(GET_IDNUM(ch));
	
	if (burglaries == NULL)
		burglaries = burglary;
	else
	{
	   Burglary *tmpBurglary = burglaries, *previousBurglary = NULL;
	   while (tmpBurglary != NULL)
	   {
		previousBurglary = tmpBurglary;
		tmpBurglary = tmpBurglary->next;
	   }
	   previousBurglary->next = burglary;
	}

	return burglary;
}

long is_room_burgled(int nRoom)
{
	if (BURGLE_FLAGGED(nRoom, ROOM_SHOP))
		return ROOM_SHOP;
	if (BURGLE_FLAGGED(nRoom, ROOM_WAREHOUSE))
		return ROOM_WAREHOUSE;
	if (BURGLE_FLAGGED(nRoom, ROOM_HOME))
		return ROOM_HOME;

	return 0;
}

long is_room_burgled(long lRoom)
{
  int nRoom = real_room(lRoom);
  return is_room_burgled(nRoom);
}

void remove_burglary(Burglary *target)
{
   if (burglaries == NULL)
	return;

   if (burglaries == target)
   {
     burglaries = burglaries->next;
     target->next = NULL;
     delete target;
     target = NULL;
     return;
   }

   Burglary *tmp = burglaries->next, *previous = burglaries;
   do {
 	if (tmp == target)
	{
	   previous->next = tmp->next;
	   tmp->next = NULL;
	   delete tmp;
	   tmp = NULL;
	   return;
	}

	previous = tmp;
	tmp = tmp->next;
        
   } while (tmp != NULL);
}

void remove_burglary(struct char_data *ch)
{
	long id = GET_IDNUM(ch);
	Burglary *tmpBurglary = burglaries, *tmpBurglaryPrevious = NULL;

	while (tmpBurglary != NULL)
	{
		if (tmpBurglary->chID == id)
		{
			if (tmpBurglaryPrevious != NULL)
				tmpBurglaryPrevious->next = tmpBurglary->next;
			else // It was the first one
				burglaries = tmpBurglary->next;

			tmpBurglary->next = NULL;
			delete tmpBurglary;
                        tmpBurglary = NULL;
			break;
		}
		
		tmpBurglaryPrevious = tmpBurglary;
		tmpBurglary = tmpBurglary->next;
	}

}

/* Function to create new set of rooms for burglar to explore */
room_rnum create_burgle_rooms(struct char_data *ch, int area_type, int dir) {
	long num_rooms;

	switch(area_type) {
		case ROOM_AREA_WAREHOUSE_RICH: 
			num_rooms = number(8, 12); break;
		case ROOM_AREA_WAREHOUSE_REGULAR:
			num_rooms = number(4, 7); break;
		case ROOM_AREA_WAREHOUSE_POOR:
			num_rooms = number(2, 3); break;
		case ROOM_AREA_SHOP_RICH:
			num_rooms = number(10, 15); break;
		case ROOM_AREA_SHOP_REGULAR:
			num_rooms = number(8, 12); break;
		case ROOM_AREA_SHOP_POOR:
			num_rooms = number(4, 7); break;
		case ROOM_AREA_HOME_RICH:
			num_rooms = number(3, 5); break;
		case ROOM_AREA_HOME_REGULAR:
			num_rooms = number(2, 4); break;
		case ROOM_AREA_HOME_POOR:
			num_rooms = number(1, 2); break;
		default: 
			basic_mud_log("Unknown housing type : burgle area type");
			return 0;
	}

	// New burglary
	Burglary *newBurglary = create_new_burglary(ch);
	
	// report the event
	sprintf(buf, "%s burgled in %s", GET_NAME(ch), zone_table[world[ch->in_room].zone].name);
	mudlog(buf, NRM, LVL_GOD, TRUE);

	// Find the num_rooms
	int nCount = 0;
	long lBurgled[MAX_BURGLED_ROOMS];
	for (int i = 0; i < num_rooms; i++)
	{
		for (int j = BURGLE_ROOMS_START; j < BURGLE_ROOMS_END; j++)
		{
			if (is_room_burgled(real_room(j)) == 0)  // Not burgled ?
			{
			   lBurgled[i] = j;
			   newBurglary->burgledRooms[i].rNum = real_room(j);
			   newBurglary->burgledRooms[i].lArea = area_type;
			   SET_BIT(BURGLE_FLAGS(real_room(j)),
				area_warehousing > 0 ? ROOM_WAREHOUSE : 
				area_suburban > 0 ? ROOM_HOME : ROOM_SHOP );
			   nCount++;
			   break;	
			}
		} 
	}	

	if (nCount < num_rooms)
	{
		send_to_char("This area is burgled enough, better wait a while.\r\n", ch);
		remove_burglary(ch);
		return 0;
	}

	// We now have a burglary object with some rooms selected
	num_rooms_burgled = num_rooms_burgled + num_rooms;

	return newBurglary->Initialise(ch->in_room, dir);
}

long get_burgle_room_type(long lArea)
{
	if (lArea == ROOM_AREA_WAREHOUSE_RICH ||
	    lArea == ROOM_AREA_WAREHOUSE_REGULAR ||
	    lArea == ROOM_AREA_WAREHOUSE_POOR)
		return ROOM_WAREHOUSE;

	if (lArea == ROOM_AREA_HOME_RICH ||
	    lArea == ROOM_AREA_HOME_REGULAR ||
	    lArea == ROOM_AREA_HOME_POOR)
		return ROOM_HOME;

	if (lArea == ROOM_AREA_SHOP_RICH ||
	    lArea == ROOM_AREA_SHOP_REGULAR ||
	    lArea == ROOM_AREA_SHOP_POOR)
		return ROOM_SHOP;

        mudlog("get_burgle_room_type : No room type declared ...", NRM, LVL_GOD, TRUE);
	return 0;
}

bool room_has_exit_to(int nFirst, int nSecond)
{
   for (int i = 0; i < 6; i++)
   {
        if (world[nFirst].dir_option[i] == NULL)
		continue;

	if (world[nFirst].dir_option[i]->to_room == nSecond)
	   return TRUE;
   }

   return FALSE;
}
