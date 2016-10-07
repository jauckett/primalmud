/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern int rev_dir[];
extern char *dirs[];
extern int movement_loss[];
extern struct zone_data *zone_table;
long flag=0;

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
#define SECT_DISPELL	      2

/* macros to decode the bit map in the sect type */
#define BASE_SECT(n) ((n) & 0x000f)

#define ATMOSPHERE(n)  (((n) & 0x0070) >> 4)
#define TEMPERATURE(n) (((n) & 0x0380) >> 7) 
#define GRAVITY(n)     (((n) & 0x1c00) >> 10)
#define ENVIRON(n)     (((n) & 0xe000) >> 13)

int check_environment_effect(struct char_data *ch);

/* external functs */
int special(struct char_data * ch, int cmd, char *arg);
void death_cry(struct char_data * ch);
/* void raw_kill(struct char_data *ch, struct char_data *killer); */

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
  if ( (IS_SET(flag, ROOM_LR_5) && GET_LEVEL(ch) < 5)
      ||(IS_SET(flag, ROOM_LR_10) && GET_LEVEL(ch) < 10)
      ||(IS_SET(flag, ROOM_LR_15) && GET_LEVEL(ch) < 15)
      ||(IS_SET(flag, ROOM_LR_20) && GET_LEVEL(ch) < 20)
      ||(IS_SET(flag, ROOM_LR_25) && GET_LEVEL(ch) < 25)
      ||(IS_SET(flag, ROOM_LR_30) && GET_LEVEL(ch) < 30)
      ||(IS_SET(flag, ROOM_LR_35) && GET_LEVEL(ch) < 35)
      ||(IS_SET(flag, ROOM_LR_ET) && GET_LEVEL(ch) < LVL_ETRNL1) 
      ||(IS_SET(flag, ROOM_LR_IMM) && GET_LEVEL(ch) < LVL_CHAMP) 
      ||(IS_SET(flag, ROOM_LR_IMP) && GET_LEVEL(ch) < LVL_IMPL) )

  {
      send_to_char("An overwhelming fear stops you from going any further.\r\n",ch);
      return 0;
  }
  return 1;
}

/* Special quest items should have a special message put in here */
void special_item_mount_message(struct char_data *ch) {

	struct obj_data *mount = MOUNTING_OBJ(ch);
	char temp[MAX_STRING_LENGTH];
	
	if( !mount ) 		// Dud call
		return;
	
	// Edit here for special object lookup

	// Default:
	sprintf(temp, "$n, mounted on %s, has arrived.", mount->short_description);
	act(temp, TRUE, ch, mount, 0, TO_ROOM);
}

/* do_simple_move assumes
 *	1. That there is no master and no followers.
 *	2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */

/* This must be modified for MOUNT code if mobs are ever 
   classified (AQUATIC, TERRA, etc) - Talisman */

int do_simple_move(struct char_data * ch, int dir, int need_specials_check)
{
  int was_in, need_movement, has_boat = 0;
  struct obj_data *obj;
  struct char_data *tmp_ch;
  extern struct index_data *mob_index;

  int special(struct char_data * ch, int cmd, char *arg);

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, ""))
    return 0;

  /* charmed? */
  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master && ch->in_room == ch->master->in_room) {
    send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  if (!allowed_zone(ch,zone_table[world[EXIT(ch, dir)->to_room].zone].zflag))
    return 0;
  if (!allowed_room(ch,world[EXIT(ch,dir)->to_room].room_flags))
    return 0;

  /* DM - check for mobs in nomob rooms - was a bug with charmed mobs */
  if (IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_NOMOB))
    if (IS_NPC(ch))
      if (!IS_CLONE(ch)) 
        return 0; 

  /* check if destination is a TUNNEL and isn't occupied */

  if (IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_TUNNEL) && GET_LEVEL(ch) < LVL_IS_GOD) 
    for (tmp_ch = world[EXIT(ch, dir)->to_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
    {
      if (tmp_ch == NULL)
        break;
      if (!IS_NPC(tmp_ch) && !IS_CLONE(ch))
      {
        send_to_char("The passage is far too narrow for more than one person to occupy.\r\n\r\n", ch);
        return 0;
      }
    }
  /* if this room or the one we're going to needs a boat, check for one */
  if ((BASE_SECT(world[ch->in_room].sector_type) == SECT_WATER_NOSWIM) ||
      (BASE_SECT(world[EXIT(ch, dir)->to_room].sector_type) == SECT_WATER_NOSWIM)) 
  {
    if(IS_AFFECTED(ch,AFF_WATERWALK) || IS_AFFECTED(ch,AFF_FLY) || IS_AFFECTED(ch,AFF_WATERBREATHE)) 
    {
      has_boat = TRUE;
    }  
    else 
    { 
      for (obj = ch->carrying; obj; obj = obj->next_content)
        if (GET_OBJ_TYPE(obj) == ITEM_BOAT)
	  has_boat = TRUE;
        if (!has_boat) 
        {
          send_to_char("You need a boat to go there.\r\n", ch);
          return 0;
        }
    }
  }

/* check if the room we're going to is a fly room - Vader */
  if(!IS_AFFECTED(ch,AFF_FLY) && ((BASE_SECT(world[ch->in_room].sector_type) == SECT_FLYING) ||
     (BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_FLYING))) {
    send_to_char("You need to be able to fly to go there.\r\n",ch);
    return 0;
    }

/* check if the room we're going to is under water- Drax */
  if(!IS_NPC(ch) && !IS_AFFECTED(ch,AFF_WATERBREATHE) && ((BASE_SECT(world[ch->in_room].sector_type) == SECT_UNDERWATER) ||
     (BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_UNDERWATER))){
    send_to_char("You take a deep breath of water. OUCH!\r\n",ch);
    send_to_char("Your chest protests terrebly causing great pain. \r\n", ch);
    act("$n suddenly turns a deep blue color holding $s throat.", FALSE, ch, 0, 0, TO_ROOM);
    GET_HIT(ch)-= GET_LEVEL(ch)*5;
    if (GET_HIT(ch) <0){
	send_to_char("Your life flashes before your eyes.  You have Drowned. RIP!", ch);
        act("$n suddenly turns a deep blue color holding $s throat.", FALSE, ch, 0, 0, TO_ROOM);
        act("$n has drowned. RIP!.", FALSE, ch, 0, 0, TO_ROOM);
        if( MOUNTING(ch) ) {
        	send_to_char("Your mount suffers as it dies.\r\n", ch);
		raw_kill(MOUNTING(ch), NULL);
	}
	raw_kill(ch,NULL);
    }
    return 0;
    }

/* if flying movement is only 1 */
  if (BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_UNDERWATER)
    need_movement = 10;
  else
  if (BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_WATER_NOSWIM)
    need_movement = 5;
  else
  if(IS_AFFECTED(ch,AFF_FLY))
    need_movement = 1;
  else
  if( MOUNTING(ch) || MOUNTING_OBJ(ch) )	// Mounts go anywhere for 1 move
	need_movement = 1;
  else
    need_movement = (movement_loss[BASE_SECT(world[ch->in_room].sector_type)] +
		    movement_loss[BASE_SECT(world[world[ch->in_room].dir_option[dir]->to_room].sector_type)]) >> 1;

  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char("You are too exhausted to follow.\r\n", ch);
    else
      send_to_char("You are too exhausted.\r\n", ch);

    return 0;
  }
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, world[EXIT(ch, dir)->to_room].number)) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return 0;
    }
    //Can't ride mobs into houses
    if( MOUNTING(ch) && IS_SET(ROOM_FLAGS(world[EXIT(ch, dir)->to_room].number), ROOM_ATRIUM)  ){  
  	send_to_char("You can't go there while mounted!\r\n", ch);
	return 0;
    }
  }
  if (GET_LEVEL(ch) < LVL_IS_GOD && !IS_NPC(ch))
    if (!PRF_FLAGGED(ch, PRF_TAG)) {
      if (affected_by_spell(ch, SPELL_HASTE))	
        GET_MOVE(ch) -= MAX(1, need_movement/2);
      else
        GET_MOVE(ch) -= need_movement;
     }
			

  if (!IS_AFFECTED(ch, AFF_SNEAK)) {
    if( MOUNTING(ch) )
	sprintf(buf2, "$n, mounted on $N, leaves %s.", dirs[dir]);
    else if( MOUNTING_OBJ(ch) )
	sprintf(buf2, "$n, mounted on %s, leaves %s.", MOUNTING_OBJ(ch)->short_description,dirs[dir]);
    else
        sprintf(buf2, "$n leaves %s.", dirs[dir]);
    
    if( MOUNTING(ch) )
	act(buf2, TRUE, ch, 0, MOUNTING(ch), TO_ROOM);
    else
	act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }

  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  if (!IS_AFFECTED(ch, AFF_SNEAK)) {
        if( MOUNTING(ch) ) 
	    act("$n, mounted on $N, has arrived.", TRUE, ch, 0, MOUNTING(ch), TO_ROOM);
	else if( MOUNTING_OBJ(ch) )
	    special_item_mount_message(ch);
	else
	    act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
  }
  look_at_room(ch, 0);

  if (check_environment_effect(ch)) {      /* returns 1 if they snuffed it */
     if( MOUNTING(ch) ) {
	send_to_char("Your mount screams in agony as it dies.\r\n", ch);
	death_cry(MOUNTING(ch));
	extract_char(MOUNTING(ch));
     }
     return 1;
  }

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH) && GET_LEVEL(ch) < LVL_IS_GOD && !IS_NPC(ch)) {
    GET_HIT(ch) = 0;
    GET_POS(ch) = POS_STUNNED;
  }

/* this next bit is to burn vampires in the day. copied from the enviro
 * HEAT code - Vader
 */
  if(PRF_FLAGGED(ch,PRF_VAMPIRE) && !IS_SET(ROOM_FLAGS(ch->in_room),ROOM_DARK) &&
     !(weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET) &&
     OUTSIDE(ch)) {
    if (!is_wearing(ch, ITEM_HEATRES) && !is_wearing(ch, ITEM_HEATPROOF))
        if (!is_wearing(ch, ITEM_STASIS))
           {
             send_to_char("The sun burns your skin!\r\n", ch);
             act("Smoke rises from $n's skin!",FALSE,ch,0,0,TO_ROOM);
             GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch)/2, 0);
           }
           if (GET_HIT(ch) <= 0)
           { 
             send_to_char("The sun burns you to death!\r\n",ch);
             act("The sun causes $n to burst into flames!", TRUE, ch, 0, 0, TO_ROOM);
             if( MOUNTING(ch) ) {
		send_to_char("Your vampiric mount dies with you.\r\n", ch);
		raw_kill(MOUNTING(ch), NULL);
	      }
	     raw_kill(ch,NULL);
             send_to_outdoor("The scream of a burning vampire echoes throughout the land...\r\n");
           }
    }
    
  return 1;
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
              if( MOUNTING(ch) ) {
		send_to_char("Your mount suffers as it dies.\r\n", ch);
		raw_kill(MOUNTING(ch), NULL);
	      }
	      sprintf(buf2,"%s killed by asphyxiation at %s",GET_NAME(ch),world[ch->in_room].name);
	      mudlog(buf2,NRM,LVL_GOD,0);
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
          if( MOUNTING(ch) ) {
		send_to_char("Your mount suffers as it dies.\r\n", ch);
		raw_kill(MOUNTING(ch), NULL);
	  }
	  sprintf(buf2,"%s killed by asphyxiation at %s",GET_NAME(ch),world[ch->in_room].name);
	  mudlog(buf2,NRM,LVL_GOD,0);
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
          if( MOUNTING(ch) ) {
		send_to_char("Your mount suffers as it dies.\r\n", ch);
		raw_kill(MOUNTING(ch), NULL);
          }
	  sprintf(buf2,"%s killed by corrosion at %s",GET_NAME(ch),world[ch->in_room].name);
	  mudlog(buf2,NRM,LVL_GOD,0);
          raw_kill(ch,NULL);
          return(1); 
        }
      }
      break;
    default : break;
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
        if( MOUNTING(ch) ) {
		send_to_char("Your mount suffers as it dies.\r\n", ch);
		raw_kill(MOUNTING(ch), NULL);
        }
	sprintf(buf2,"%s killed by scorching at %s",GET_NAME(ch),world[ch->in_room].name);
	mudlog(buf2,NRM,LVL_GOD,0);
	raw_kill(ch,NULL);
      }
      break;   
      
    case SECT_INCINERATE :
      send_to_char("You estimate that it is well over 3000 degrees here.\n\r", ch);
      send_to_char("Your skin catches fire! You are burnt to a cinder\n\r", ch);
      act("$n spontaneously combusts and is reduced to a charred mess on the ground!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      if( MOUNTING(ch) ) {
	send_to_char("Your mount suffers as it dies.\r\n", ch);
	raw_kill(MOUNTING(ch), NULL);
      }
      sprintf(buf2,"%s killed by incineration at %s",GET_NAME(ch),world[ch->in_room].name);
      mudlog(buf2,NRM,LVL_GOD,0);
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
        if( MOUNTING(ch) ) {
           send_to_char("Your mount suffers as it dies.\r\n", ch);
	   raw_kill(MOUNTING(ch), NULL);
	}
        sprintf(buf2,"%s killed by hypothermia at %s",GET_NAME(ch),world[ch->in_room].name);
	mudlog(buf2,NRM,LVL_GOD,0);
        raw_kill(ch,NULL);
      }
      break;   

    case SECT_ABSZERO :
      send_to_char("It dosn't get much colder than this... absolute zero!\n\r", ch);
      send_to_char("Your body is frozen rock hard and shatters into many pieces!\n\r", ch);
      act("$n is frozen solid, tries to move and shatters into thousands of pieces!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      if( MOUNTING(ch) ) {
	send_to_char("Your mount suffers as it dies.\r\n", ch);
	raw_kill(MOUNTING(ch), NULL);
      }
      sprintf(buf2,"%s killed by hypothermia at %s",GET_NAME(ch),world[ch->in_room].name);
      mudlog(buf2,NRM,LVL_GOD,0);
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


int perform_move(struct char_data * ch, int dir, int need_specials_check)
{
  int was_in;
  struct follow_type *k, *next;
  extern struct index_data *mob_index;

  if (IS_AFFECTED(ch, AFF_PARALYZED)){
     send_to_char("PANIC! Your legs refuse to move!!\r\n", ch);
     return(0);
  }

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS)
    return 0;
  else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE){
    switch (number(1,5)){
        case 1:
                send_to_char("Alas, you cannot go that way...\r\n", ch);
		break;
        case 2:
                send_to_char("Do you make a habbit of walking into solid objects!..\r\n", ch);
		break;
        case 3:
                send_to_char("Oh wait let me make an exit there for you! DOPE!\r\n", ch);
		break;
        case 4:
                send_to_char("I don't think you can go there.\r\n", ch);
		break;
        case 5:
                send_to_char("Nup can't go that way!!...\r\n", ch);
		break;
    }
  }
  else if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
    if (EXIT(ch, dir)->keyword) {
      sprintf(buf2, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf2, ch);
    } else
      send_to_char("It seems to be closed.\r\n", ch);
  } else {
    if (!ch->followers)
      return (do_simple_move(ch, dir, need_specials_check));

    was_in = ch->in_room;
    if (!do_simple_move(ch, dir, need_specials_check))
      return 0;

/* JA modified this so charmed mobs stop following if PC's leave them */
    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((was_in == k->follower->in_room) &&
	  (GET_POS(k->follower) >= POS_STANDING)) 
      {
	act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
      else
        if (IS_NPC(k->follower))
        {
          /* DM - always have clones following master */
          if (!(IS_CLONE(k->follower))) {
            SET_BIT(MOB_FLAGS(k->follower), MOB_MEMORY);
            remember(k->follower, ch);
            stop_follower(k->follower);
          } else 
            return 0;
        }
    }
    return 1;
  }
  return 0;
}

/* Modified to give portals the ability to limit what levels can 
   use them. May also implement an alignment restriction as well at
   some stage. - Talisman */
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
    // Level restrictions - low 
    if( ( GET_LEVEL(ch) < GET_OBJ_VAL(obj, 1) ) && 
	( GET_OBJ_VAL(obj, 1) != 0) && ( GET_LEVEL(ch) < LVL_GOD ) ){
	sprintf(buf, "%s is too powerful for you to use.\r\n", obj->short_description);
	send_to_char(buf,ch);
	return;
    }
    // Level restrictions - high
    if( ( GET_LEVEL(ch) > GET_OBJ_VAL(obj, 2) ) && 
        ( GET_OBJ_VAL(obj, 2) != 0) && (GET_LEVEL(ch) < LVL_GOD) ){
	sprintf(buf, "%s is too weak to transport you.\r\n",obj->short_description);
	send_to_char(buf,ch);
	return;
    }
 

      if (!IS_AFFECTED(ch, AFF_SNEAK)) 
      {
        sprintf(buf2, "$n leaves via %s.", obj->short_description);
        act(buf2, TRUE, ch, 0, 0, TO_ROOM);
      }

      was_in = ch->in_room;
      char_from_room(ch);
      char_to_room(ch, real_room(GET_OBJ_VAL(obj, 0)));

      if (!IS_AFFECTED(ch, AFF_SNEAK)) {
	sprintf(buf, "Reality warps as $n arrives.", obj->short_description);
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
      }
      look_at_room(ch, 0);
    }
  }
}

ACMD(do_move)
{
  register struct char_data *vict, *tmob;
  int found;
  /*
   * This is basically a mapping of cmd numbers to perform_move indexes.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  if (IS_AFFECTED(ch, AFF_PARALYZED)){
     send_to_char("You cannot move you are paralyzed.\r\n", ch);
     return;
  }
  perform_move(ch, cmd - 1, 0);

/* check if they snuffed it from the move */
  if (ch->in_room == -1)
    return;

  for (tmob = world[ch->in_room].people; tmob; tmob = tmob->next_in_room)
  {
    if (!IS_NPC(tmob) || !MOB_FLAGGED(tmob, MOB_AGGRESSIVE))
      continue;

    found = FALSE;
    for (vict = world[tmob->in_room].people; vict && !found; vict = vict->next_in_room)
        {
          if (IS_NPC(vict) || !CAN_SEE(tmob, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
            continue;
          if (MOB_FLAGGED(tmob, MOB_WIMPY) && AWAKE(vict))
            continue;

          if (MOB_FLAGGED(tmob, MOB_AGGRESSIVE) ||
           (MOB_FLAGGED(tmob, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
           (MOB_FLAGGED(tmob, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
           (MOB_FLAGGED(tmob, MOB_AGGR_GOOD) && IS_GOOD(vict))) {
		if (number(0, 25) <= GET_REAL_CHA(vict)) {
	          act("$n looks at $N with an indifference.",FALSE, tmob, 0, vict, TO_NOTVICT);
	          act("$N looks at you with an indifference.",FALSE, vict, 0, tmob, TO_CHAR);
	        } else {
            		hit(tmob, vict, TYPE_UNDEFINED);
            		found = TRUE;
		       }
          }
    }
  }
}


int find_door(struct char_data * ch, char *type, char *dir)
{
  int door;

  if (*dir) {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1) {	/* Partial Match */
      send_to_char("That's not a direction.\r\n", ch);
      return -1;
    }
    if (EXIT(ch, door))
      if (EXIT(ch, door)->keyword)
	if (isname(type, EXIT(ch, door)->keyword))
	  return door;
	else {
	  sprintf(buf2, "I see no %s there.\r\n", type);
	  send_to_char(buf2, ch);
	  return -1;
	}
      else
	return door;
    else {
      send_to_char("I really don't see how you can close anything there.\r\n", ch);
      return -1;
    }
  } else {			/* try to locate the keyword */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (isname(type, EXIT(ch, door)->keyword))
	    return door;

    sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    send_to_char(buf2, ch);
    return -1;
  }
}



ACMD(do_open)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;

  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Open what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
			ch, &victim, &obj))
    /* this is an object */

    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("But it's already open!\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))
      send_to_char("You can't do that.\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("It seems to be locked.\r\n", ch);
    else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
      send_to_char(OK, ch);
      act("$n opens $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* perhaps it is a door */

    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's impossible, I'm afraid.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("It's already open!\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It seems to be locked.\r\n", ch);
    else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if (EXIT(ch, door)->keyword)
	act("$n opens the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword,
	    TO_ROOM);
      else
	act("$n opens the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char(OK, ch);
      /* now for opening the OTHER side of the door! */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
	if ((back = world[other_room].dir_option[rev_dir[door]]))
	  if (back->to_room == ch->in_room) {
	    REMOVE_BIT(back->exit_info, EX_CLOSED);
	    if (back->keyword) {
	      sprintf(buf, "The %s is opened from the other side.\r\n",
		      fname(back->keyword));
	      send_to_room(buf, EXIT(ch, door)->to_room);
	    } else
	      send_to_room("The door is opened from the other side.\r\n",
			   EXIT(ch, door)->to_room);
	  }
    }
}


ACMD(do_close)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;


  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Close what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("But it's already closed!\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))
      send_to_char("That's impossible.\r\n", ch);
    else {
      SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
      send_to_char(OK, ch);
      act("$n closes $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* Or a door */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("It's already closed!\r\n", ch);
    else {
      SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if (EXIT(ch, door)->keyword)
	act("$n closes the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
	    TO_ROOM);
      else
	act("$n closes the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char(OK, ch);
      /* now for closing the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
	if ((back = world[other_room].dir_option[rev_dir[door]]))
	  if (back->to_room == ch->in_room) {
	    SET_BIT(back->exit_info, EX_CLOSED);
	    if (back->keyword) {
	      sprintf(buf, "The %s closes quietly.\r\n", back->keyword);
	      send_to_room(buf, EXIT(ch, door)->to_room);
	    } else
	      send_to_room("The door closes quietly.\r\n", EXIT(ch, door)->to_room);
	  }
    }
}


int has_key(struct char_data * ch, int key)
{
  struct obj_data *o;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return 1;

  if (ch->equipment[WEAR_HOLD])
    if (GET_OBJ_VNUM(ch->equipment[WEAR_HOLD]) == key)
      return 1;

  return 0;
}


ACMD(do_lock)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;


  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Lock what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
			ch, &victim, &obj))
    /* this is an object */

    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("Maybe you should close it first...\r\n", ch);
    else if (GET_OBJ_VAL(obj, 2) < 0)
      send_to_char("That thing can't be locked.\r\n", ch);
    else if (!has_key(ch, GET_OBJ_VAL(obj, 2)))
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("It is locked already.\r\n", ch);
    else {
      SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*Cluck*\r\n", ch);
      act("$n locks $p - 'cluck', it says.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* a door, perhaps */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You have to close it first, I'm afraid.\r\n", ch);
    else if (EXIT(ch, door)->key < 0)
      send_to_char("There does not seem to be a keyhole.\r\n", ch);
    else if (!has_key(ch, EXIT(ch, door)->key) && GET_LEVEL(ch) < LVL_IS_GOD)
      send_to_char("You don't have the proper key.\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It's already locked!\r\n", ch);
    else {
      SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
	act("$n locks the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
	    TO_ROOM);
      else
	act("$n locks the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("*Click*\r\n", ch);
      /* now for locking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
	if ((back = world[other_room].dir_option[rev_dir[door]]))
	  if (back->to_room == ch->in_room)
	    SET_BIT(back->exit_info, EX_LOCKED);
    }
}


ACMD(do_unlock)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;


  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Unlock what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM,
			ch, &victim, &obj))
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("Silly - it ain't even closed!\r\n", ch);
    else if (GET_OBJ_VAL(obj, 2) < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if (!has_key(ch, GET_OBJ_VAL(obj, 2)))
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("Oh.. it wasn't locked, after all.\r\n", ch);
    else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*Click*\r\n", ch);
      act("$n unlocks $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  else if ((door = find_door(ch, type, dir)) >= 0)
    /* it is a door */

    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("Heck.. it ain't even closed!\r\n", ch);
    else if (EXIT(ch, door)->key < 0)
      send_to_char("You can't seem to spot any keyholes.\r\n", ch);
    else if (!has_key(ch, EXIT(ch, door)->key) && GET_LEVEL(ch) < LVL_IS_GOD)
      send_to_char("You do not have the proper key for that.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("It's already unlocked, it seems.\r\n", ch);
    else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
	act("$n unlocks the $F.", 0, ch, 0, EXIT(ch, door)->keyword,
	    TO_ROOM);
      else
	act("$n unlocks the door.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("*click*\r\n", ch);
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
	if ((back = world[other_room].dir_option[rev_dir[door]]))
	  if (back->to_room == ch->in_room)
	    REMOVE_BIT(back->exit_info, EX_LOCKED);
    }
}





ACMD(do_pick)
{
  ubyte percent;
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *v;

  two_arguments(argument, type, dir);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (!*type)
    send_to_char("Pick what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &v, &obj)) {
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      send_to_char("That's not a container.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
      send_to_char("Silly - it isn't even closed!\r\n", ch);
    else if (GET_OBJ_VAL(obj, 2) < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED))
      send_to_char("Oho! This thing is NOT locked!\r\n", ch);
    else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF))
      send_to_char("It resists your attempts at picking it.\r\n", ch);
    else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
      send_to_char("You failed to pick the lock.\r\n", ch);
    else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*Click*\r\n", ch);
      act("$n fiddles with $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  } else if ((door = find_door(ch, type, dir)) >= 0)
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR))
      send_to_char("That's absurd.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      send_to_char("You realize that the door is already open.\r\n", ch);
    else if (EXIT(ch, door)->key < 0)
      send_to_char("You can't seem to spot any lock to pick.\r\n", ch);
    else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
      send_to_char("Oh.. it wasn't locked at all.\r\n", ch);
    else if (IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF))
      send_to_char("You seem to be unable to pick this lock.\r\n", ch);
    else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
      send_to_char("You failed to pick the lock.\r\n", ch);
    else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword)
	act("$n skillfully picks the lock of the $F.", 0, ch, 0,
	    EXIT(ch, door)->keyword, TO_ROOM);
      else
	act("$n picks the lock of the door.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("The lock quickly yields to your skills.\r\n", ch);
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE)
	if ((back = world[other_room].dir_option[rev_dir[door]]))
	  if (back->to_room == ch->in_room)
	    REMOVE_BIT(back->exit_info, EX_LOCKED);
    }
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
  } else if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
    send_to_char("You are already indoors.\r\n", ch);
  else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
	      IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("You can't seem to find anything to enter.\r\n", ch);
  }
}


ACMD(do_leave)
{
  int door;

  if (!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
    send_to_char("You are outside.. where do you want to go?\r\n", ch);
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
	      !IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("I see no obvious exits to the outside.\r\n", ch);
  }
}


ACMD(do_stand)
{
/* remove fly spell - Vader */
  if(affected_by_spell(ch,SPELL_FLY)) {
    send_to_char("You float back down to earth.\r\n",ch);
    affect_from_char(ch,SPELL_FLY);
    }

  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SITTING:
    act("You stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    if (FIGHTING(ch))
      GET_POS(ch) = POS_FIGHTING;
    else
      GET_POS(ch) = POS_STANDING;
    break;
  case POS_RESTING:
    act("You stop resting, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_SLEEPING:
    act("You have to wake up first!", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Do you not consider fighting as standing?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and put your feet on the ground.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}


ACMD(do_sit)
{
/* remove fly spell - Vader */
  if(affected_by_spell(ch,SPELL_FLY)) {
    send_to_char("You float back down to earth.\r\n",ch);   
    affect_from_char(ch,SPELL_FLY);
    }

  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You sit down.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char("You're sitting already.\r\n", ch);
    break;
  case POS_RESTING:
    act("You stop resting, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Sit down while fighting? are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and sit down.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_rest)
{
   if( MOUNTING(ch) || MOUNTING_OBJ(ch) ) {
	send_to_char("You cannot rest while mounted.\r\n", ch);
	return;
   }

/* remove fly spell - Vader */
  if(affected_by_spell(ch,SPELL_FLY)) {
    send_to_char("You float back down to earth.\r\n",ch);   
    affect_from_char(ch,SPELL_FLY);
    }

  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You sit down and rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    act("You rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    act("You are already resting.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Rest while fighting?  Are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and stop to rest your tired bones.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_sleep)
{
   if( MOUNTING(ch) || MOUNTING_OBJ(ch) ) {
	send_to_char("You cannot sleep while mounted.\r\n", ch);
	return;
   }

/* remove fly spell - Vader */
  if(affected_by_spell(ch,SPELL_FLY)) {
    send_to_char("You float back down to earth.\r\n",ch);   
    affect_from_char(ch,SPELL_FLY);
    }

  if(IS_SET(ROOM_FLAGS(ch->in_room),ROOM_NOSLEEP))
  {
    send_to_char("You feel troubled... I don't think you can get to sleep here.\r\n",ch);
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
    act("You stop floating around, and lie down to sleep.",
	FALSE, ch, 0, 0, TO_CHAR);
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
      send_to_char("You can't wake people up if you're asleep yourself!\r\n", ch);
    else if ((vict = get_char_room_vis(ch, arg)) == NULL)
      send_to_char(NOPERSON, ch);
    else if (vict == ch)
      self = 1;
    else if (GET_POS(vict) > POS_SLEEPING)
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (IS_AFFECTED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }
  if (IS_AFFECTED(ch, AFF_SLEEP))
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
  extern struct index_data *mob_index;

  void stop_follower(struct char_data * ch);
  void add_follower(struct char_data * ch, struct char_data * leader);

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    }
  } else {
    send_to_char("Whom do you wish to follow?\r\n", ch);
    return;
  }

  /* DM - for clones */
  if (IS_CLONE(ch)) {
    send_to_char("No, you gave me life. I shall always follow you!\r\n",
	ch->master); 
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master)) {
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
	act("Sorry, but following in loops is not allowed.", FALSE, ch, 0, 0, TO_CHAR);
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}


ACMD(do_mount) {

	int bits;
	struct char_data *found_char = NULL;
	struct obj_data *found_obj = NULL;

	if( IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if( !*arg ) {
		send_to_char("Mount what?!\r\n", ch);
		return;
	}	
	// Check character is not already mounted
	if( MOUNTING(ch) || MOUNTING_OBJ(ch) ) {
		send_to_char("You are already mounted!\r\n", ch);	
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
		// check if it's tamed
		if( !AFF_FLAGGED(found_char, AFF_BROKEN_IN) ) {
			send_to_char("This beastie's a bit too wild for you.\r\n", ch);
			act("$n chases $N around, trying to mount $M.", FALSE, ch, 0, found_char, TO_ROOM);
			return;
		}
		// Check that noone is riding it already
		if( MOUNTING(found_char) ) {
			act("Someone is already mounting $M.", FALSE, ch, 0, found_char, TO_CHAR);
			return;
		}
		// Mount the beast
		act("With a flourish you mount $N.", FALSE, ch, 0, found_char, TO_CHAR);
		act("$n gracefully mounts $N.", FALSE, ch, 0, found_char, TO_ROOM);
		MOUNTING(ch) = found_char;
		MOUNTING(found_char) = ch;
		return;
	}

	// Found an object
	if( found_obj != NULL) {
		// Check if the item is mountable
	//	if( !OBJ_FLAGGED(found_obj, ITEM_RIDDEN) ) {
		if( IS_SET(GET_OBJ_EXTRA(found_obj), ITEM_RIDDEN) ) {
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
		sprintf(buf, "%s mounts %s.\r\n", GET_NAME(ch), found_obj->short_description);
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
		
	if( IS_NPC(ch) )
		return;

	// May as well make it a social too
	if( !MOUNTING(ch) && !MOUNTING_OBJ(ch) ) {
		send_to_char("You get off your high horse.\r\n", ch);
		act("$n gets off $s high horse.", FALSE, ch, 0, 0, TO_ROOM);
		return;
	}	

	// mobile dismount
	if( MOUNTING(ch) ) {
		// Get the mount
		mount = MOUNTING(ch);
		// Get a keyword
		one_argument(mount->player.name, arg);
		// If it is not visible ...
		if( mount != get_char_vis(ch, arg, FIND_CHAR_ROOM) ) {
			send_to_char("Oh dear you seem to have lost your ride.\r\n", ch);
			send_to_char(" ... you fall off your imaginary mount.\r\n", ch);
			act("$n falls off $s imaginary mount.\r\n", FALSE, ch, 0, 0, TO_ROOM);
		}
		else {
			// Dismount successful
			send_to_char("You dismount.\r\n", ch);
			act("$n easily hops off $N.", FALSE, ch, 0, mount, TO_ROOM);	
		}
		// Regardless, dismount
		MOUNTING(mount) = NULL;
		MOUNTING(ch) = NULL;
		GET_POS(ch) = POS_STANDING;
		return;
	}

	// Item dismount
	if( MOUNTING_OBJ(ch) ) {
		// Get the object
		obj_mount = MOUNTING_OBJ(ch);
		// Check if it's carried or int he room
		if( !(obj_mount != get_obj_in_list_vis(ch, obj_mount->name, ch->carrying) &&
		      obj_mount != get_object_in_equip_vis(ch, obj_mount->name, ch->equipment,&tmp) &&
		      obj_mount != get_obj_in_list_vis(ch, obj_mount->name,world[ch->in_room].contents)) ){
			send_to_char("... you fall off your imaginary mount.\r\n", ch);
			act("$n falls off $s imaginary mount.", FALSE, ch, 0, 0, TO_ROOM);
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

/* Tames a mob in */
ACMD(do_breakin) {

	struct char_data *victim;

	if( IS_NPC(ch) )
		return;

	// Find one target
	one_argument(argument, arg);
	
	if( !*arg ) {
		send_to_char("Just what would you like to breakin?\r\n", ch);
		return;
	}

	// Check the target is present and vis
	if( !(victim = get_char_vis( ch, arg, FIND_CHAR_ROOM)) ) {
		send_to_char("Noone here by that name.\r\n", ch);
		return;
 	}

	// fighting?
	if( FIGHTING(ch) || FIGHTING(victim) ) {
		send_to_char("You can't do that now!\r\n", ch);
		return;
 	}	

	// taming themselves?!
	if( ch == victim ) {
		send_to_char("You break yourself in. You feel quite tame.\r\n", ch);
		return;
	}
	if( AFF_FLAGGED(victim, AFF_BROKEN_IN) ) {
		act("$N is already broken in.\r\n", FALSE, ch, 0, victim, TO_CHAR);
		return;
	}

	// Is the mob mountable?!
	if( MOB_FLAGGED(victim, MOB_MOUNTABLE) ) {
		/* Check for skill here, should one be put in */
		act("You successfully break $N in.", FALSE, ch, 0, victim, TO_CHAR);
		SET_BIT(AFF_FLAGS(victim), AFF_BROKEN_IN);
	}
	else {
		act("You try to break $N in, but $M does not respond well.", FALSE, ch, 0, victim, TO_CHAR);
		if( GET_LEVEL(ch) < LVL_GOD )
			do_hit(victim, ch->player.name, 0, 0); 
	
	}
}
