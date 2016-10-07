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
#include "dg_scripts.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;

int check_environment_effect(struct char_data *ch);

/* external functs */
void add_follower(struct char_data *ch, struct char_data *leader);
int special(struct char_data *ch, int cmd, char *arg);
void death_cry(struct char_data *ch);
int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
void ch_trigger_trap(struct char_data *ch, struct obj_data *obj);
int Escape(struct char_data *ch);
int pun_aggro_check (struct char_data *ch);

/* local functions */
int has_boat(struct char_data *ch);
int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname, bool dispMessages);
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
ACMD(do_violent_skill);
ACMD(do_recall);

void EscapeAll(struct char_data *escaper, struct char_data *master, int room)
{
  struct follow_type *tmp;
  int escapecount = 0;

  for(tmp = master->followers; tmp; tmp = tmp->next)
  {
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
  if (master != escaper)
  {
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

ACMD(do_recall)
{
  room_rnum room_num;
  extern int world_start_room[NUM_WORLDS];

  if (GET_LEVEL(ch) > LVL_NEWBIE)
  {
    send_to_char("Perhaps it's time you bought a scroll of recall.\r\n", ch);
    return;
  }
  if (ENTRY_ROOM(ch, get_world(ch->in_room)) != -1)
    room_num = real_room(ENTRY_ROOM(ch, get_world(ch->in_room)));
  else
    room_num = real_room(world_start_room[get_world(ch->in_room)]);

  if (PRF_FLAGGED(ch, PRF_MORTALK))
    REMOVE_BIT(PRF_FLAGS(ch), PRF_MORTALK);

  if (room_num <= 0)
  {
    sprintf(buf,"SYSERR: Recall: %s entry room for %d invalid",
            GET_NAME(ch), get_world(ch->in_room));
    mudlog(buf, BRF, LVL_GOD, TRUE);
    return;
  }
  act("$n disappears.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, room_num);
  act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

ACMD(do_escape)
{
  int room;

  if (IS_NPC(ch))
  {
    send_to_char("Nup, you're stuck as an NPC forever!\r\n", ch);
    return;
  }
  if (!IS_SET(GET_SPECIALS(ch), SPECIAL_ESCAPE))
  {
    send_to_char("You try to escape the drudgery of your life.\r\n", ch);
    act("$n goes into denial.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
  if ((BASE_SECT(ch->in_room) != SECT_INSIDE) && 
      !ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
  {
    send_to_char("You're already outside!\r\n", ch);
    return;
  }

  if ((room = Escape(ch)) != NOWHERE)
  {
    // Check if they're groupped
    if (AFF_FLAGGED(ch, AFF_GROUP))
    {
      if (ch->master) 
	EscapeAll(ch, ch->master, room);
      else
	EscapeAll(ch, ch, room);
    } else {
      act("You manage to find your way out, and escape your surroundings.\r\n",
	  FALSE, ch, 0, 0, TO_CHAR);
      char_from_room(ch);
      char_to_room(ch, room);
      act("$n arrives looking hurried.\r\n", FALSE, ch, 0, 0, TO_ROOM);
      look_at_room(ch, 0);
    }
  } else 
    send_to_char("You try to remember your way back out, and escape, but are unable to.\r\n", ch);
}

/* returns 1 if ch is allowed in zone */
int allowed_zone(struct char_data * ch, int flag, bool show)
{
  int reqlvl = 0;

  if (!LR_FAIL(ch, LVL_IMPL))
    return 1;
 
  /* check if the room we're going to is level restricted */
  if ((IS_SET(flag, ZN_NEWBIE) && GET_LEVEL(ch) > LVL_NEWBIE))
  {
    send_to_char("You are too high a level to enter this zone!",ch);
    return 0;
  }

  if (flag & 0xffffc == 0) // No zone LR.
    return 1;

  if (IS_SET(flag, ZN_LR_IMP))
    reqlvl = LVL_IMPL;
  else if (IS_SET(flag, ZN_LR_IMM))
    reqlvl = LVL_ANGEL;
  else if (IS_SET(flag, ZN_LR_ET))
    reqlvl = LVL_ETRNL1;
  else if (IS_SET(flag, ZN_LR_95))
    reqlvl = 95;
  else if (IS_SET(flag, ZN_LR_90))
    reqlvl = 90;
  else if (IS_SET(flag, ZN_LR_85))
    reqlvl = 85;
  else if (IS_SET(flag, ZN_LR_80))
    reqlvl = 80;
  else if (IS_SET(flag, ZN_LR_75))
    reqlvl = 75;
  else if (IS_SET(flag, ZN_LR_70))
    reqlvl = 70;
  else if (IS_SET(flag, ZN_LR_65))
    reqlvl = 65;
  else if (IS_SET(flag, ZN_LR_60))
    reqlvl = 60;
  else if (IS_SET(flag, ZN_LR_55))
    reqlvl = 55;
  else if (IS_SET(flag, ZN_LR_50))
    reqlvl = 50;
  else if (IS_SET(flag, ZN_LR_45))
    reqlvl = 45;
  else if (IS_SET(flag, ZN_LR_40))
    reqlvl = 40;
  else if (IS_SET(flag, ZN_LR_35))
    reqlvl = 35;
  else if (IS_SET(flag, ZN_LR_30))
    reqlvl = 30;
  else if (IS_SET(flag, ZN_LR_25))
    reqlvl = 25;
  else if (IS_SET(flag, ZN_LR_20))
    reqlvl = 20;
  else if (IS_SET(flag, ZN_LR_15))
    reqlvl = 15;
  else if (IS_SET(flag, ZN_LR_10))
    reqlvl = 10;
  else if (IS_SET(flag, ZN_LR_5))
    reqlvl = 5;

  if (!LR_FAIL(ch, reqlvl)) // Base check.
    return 1;

  if ((GET_CLASS(ch) > CLASS_WARRIOR) && (reqlvl <= GET_MAX_LVL(ch)))
  {
    if (GET_CLASS(ch) == CLASS_MASTER)
      reqlvl -= 20;
    else
      reqlvl -= 10;
    if (!LR_FAIL(ch, reqlvl))
      return 1;
  }

  if (show)
    send_to_char("An overwhelming fear stops you from going any further.\r\n", ch);
  return 0;
} 

/* returns 1 if ch is allowed in room */
int allowed_room(struct char_data * ch, int flag, bool show)
{
  if (!LR_FAIL(ch, LVL_IMPL))
    return TRUE;

  if (IS_NPC(ch) && (!ch->master) && IS_SET(flag, ROOM_NOMOB))
  {
    if (show)
      send_to_char("I think you should leave that room to the players.\r\n",ch);
    return FALSE;
  }

  if ((IS_SET(flag, ROOM_NEWBIE) && GET_LEVEL(ch) > LVL_NEWBIE))
  {
    if (show)
      send_to_char("You are too high a level to enter this zone!",ch);
    return FALSE;
  }
  if ((IS_SET(flag, ROOM_LR_ET) && LR_FAIL_MAX(ch, LVL_ETRNL1)) ||
      (IS_SET(flag, ROOM_LR_IMM) && LR_FAIL(ch, LVL_CHAMP))     ||
      (IS_SET(flag, ROOM_LR_ANG) && LR_FAIL(ch, LVL_ANGEL))     ||
      (IS_SET(flag, ROOM_LR_GOD) && LR_FAIL(ch, LVL_GOD))       ||
      (IS_SET(flag, ROOM_LR_IMP))) // Artus> IMPL is checked already.
  {
    if (show)
      send_to_char("An overwhelming fear stops you from going any further.\r\n",ch);
    return FALSE;
  }
  return TRUE;
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

  if (AFF_FLAGGED(ch, AFF_WATERWALK))
    return TRUE;
  if (AFF_FLAGGED(ch, AFF_WATERBREATHE))
    return TRUE;
  if (AFF_FLAGGED(ch, AFF_FLY))
    return TRUE;
  if (!LR_FAIL(ch, LVL_IMPL))
    return TRUE;
  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return TRUE;

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return TRUE;

  /* Check for the type of mount they have, to see whether they can walk on water */
  if (MOUNTING(ch)) 
  {
    i = GET_CLASS(MOUNTING(ch));
    if (i == CLASS_AQUATIC || i == CLASS_DRAGON)
      return TRUE;
  }

  return FALSE;
}

/* Artus> Generic movement check. */
int char_can_enter(struct char_data *ch, struct room_data *room, bool show)
{
  if (!(ch && room))
  {
    if (show)
      send_to_char("Maybe that's not such a great idea...\r\n", ch);
    return FALSE;
  }

  // Whatever. I do what I want.
  if (!LR_FAIL(ch, LVL_IMPL))
    return TRUE;
  // Zone Allowed
  if (!allowed_zone(ch, zone_table[room->zone].zflag, show))
    return FALSE;
  // Room Allowed.
  if (!allowed_room(ch, room->room_flags, show))
    return FALSE;

  // A mount? 
  if (IS_NPC(ch) && MOUNTING(ch))
    return FALSE;

  // if this room or the one we're going to needs a boat, check for one
  if (((BASE_SECT(world[ch->in_room].sector_type) == SECT_WATER_NOSWIM) ||
       (BASE_SECT(room->sector_type) == SECT_WATER_NOSWIM)) && 
      (!has_boat(ch)))
  {
    if (show)
      send_to_char("You need a boat to go there.\r\n", ch);
    return FALSE;
  }
 
  // check if the room we're going to is a fly room - Vader 
  if (!IS_AFFECTED(ch,AFF_FLY) && 
      ((BASE_SECT(world[ch->in_room].sector_type) == SECT_FLYING) ||
       (BASE_SECT(room->sector_type) == SECT_FLYING))) 
  {
    if (show)
      send_to_char("You need to be able to fly to go there.\r\n",ch);
    return FALSE;
  }

  // Artus> Houses.
  if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM))
  {
    if (!House_can_enter(ch, room->number))
    {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return (0);
    }
    if (MOUNTING(ch))
    {
      send_to_char("You cannot go there while mounted!\r\n", ch);
      return (0);
    }
  }

  if (!IS_NPC(ch) && (!LR_FAIL(ch, LVL_IS_GOD)) &&
      !IS_AFFECTED(ch,AFF_WATERBREATHE) &&
      ((UNDERWATER(ch)) ||
       (BASE_SECT(room->sector_type) == SECT_UNDERWATER)))
  {
    send_to_char("You take a deep breath of water.  OUCH!\r\n",ch);
    send_to_char("Your chest protests terrebly causing great pain. \r\n", ch);
    act("$n suddenly turns a deep blue color holding $s throat.", 
	FALSE, ch, 0, 0, TO_ROOM);
    // GET_HIT(ch)-= GET_LEVEL(ch)*5;
    damage(NULL, ch, GET_LEVEL(ch)*5, TYPE_UNDEFINED, FALSE);
    if (GET_HIT(ch) <0) 
    {
      send_to_char("Your life flashes before your eyes.  You have Drowned.  RIP!", ch);
      act("$n suddenly turns a deep blue color holding $s throat.", 
	  FALSE, ch, 0, 0, TO_ROOM);
      act("$n has drowned. RIP!.", FALSE, ch, 0, 0, TO_ROOM);
      if (MOUNTING(ch)) 
      {
        send_to_char("Your mount suffers as it dies.\r\n", ch);
	death_cry(MOUNTING(ch));
        raw_kill(MOUNTING(ch), NULL);
      }
      death_cry(ch);
      die(ch,NULL,"drowning");
    }
    return FALSE;
  }
  return TRUE;
}

void special_item_mount_message(struct char_data *ch) 
{
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

/* JA some data for reversing directions */
static int DIR_REVERSE[6] = {2, 3, 0, 1, 5, 4};
static char *SECT_ATMOSPHERE_ARRIVE[7]  = {"", "", "", "", "", "", ""};
static char *SECT_TEMPERATURE_ARRIVE[7] = {"", "", "", "", "", "", ""};
static char *SECT_GRAVITY_ARRIVE[7]     = {"", "", "", "", "", "", ""};
static char *SECT_ENVIRON_ARRIVE[7]     = {"", "", "", "$n is moving backwards.", "", "", ""};
static char *SECT_ATMOSPHERE_LEAVE[7]   = {"", "", "", "", "", "", ""};
static char *SECT_TEMPERATURE_LEAVE[7]  = {"", "", "", "", "", "", ""};
static char *SECT_GRAVITY_LEAVE[7]      = {"", "", "", "", "", "", ""};
static char *SECT_ENVIRON_LEAVE[7]      = {"", "", "", "$n leaves back to front.", "", "", ""};

static void do_environ_leave_message(struct char_data *ch, int dir)
{
  int sect_type;
  sect_type =  world[ch->in_room].sector_type & 0xfff0;
  sprintf(buf2, SECT_ATMOSPHERE_LEAVE[ATMOSPHERE(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf2, SECT_TEMPERATURE_LEAVE[TEMPERATURE(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf2, SECT_GRAVITY_LEAVE[GRAVITY(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf2, SECT_ENVIRON_LEAVE[ENVIRON(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
}

static void do_environ_arrive_message(struct char_data *ch, int dir)
{
  int sect_type;
  sect_type =  world[ch->in_room].sector_type & 0xfff0;
  sprintf(buf2, SECT_ATMOSPHERE_ARRIVE[ATMOSPHERE(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf2, SECT_TEMPERATURE_ARRIVE[TEMPERATURE(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf2, SECT_GRAVITY_ARRIVE[GRAVITY(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf2, SECT_ENVIRON_ARRIVE[ENVIRON(sect_type)], dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
}

int do_simple_move(struct char_data *ch, int dir, int need_specials_check,
                   int subcmd, bool show)
{
  room_rnum was_in;
  int need_movement;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, "")) /* XXX: Evaluate NULL */
    return (0);

  // charmead?
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master) && 
      (ch->in_room == ch->master->in_room))
  {
    send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  if (!char_can_enter(ch, &world[EXIT(ch,dir)->to_room], TRUE))
    return 0;

  /* if flying movement is only 1 */
  if (BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_UNDERWATER)
    need_movement = 10;
  else if (BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_WATER_NOSWIM)
    need_movement = 5;
  else if (IS_AFFECTED(ch,AFF_FLY))
    need_movement = 1;
  else if (MOUNTING(ch) || MOUNTING_OBJ(ch))  // Mounts go anywhere for 1 move
    need_movement = 1;
  else
  /* move points needed is avg. move loss for src and destination sect type */
    need_movement = (movement_loss[BASE_SECT(world[ch->in_room].sector_type)] + movement_loss[BASE_SECT(world[world[ch->in_room].dir_option[dir]->to_room].sector_type)]) >> 1;

  if (!IS_NPC(ch) && need_movement > 1 && GET_SKILL(ch, SKILL_MOUNTAINEER) && 
     (BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_MOUNTAIN ||
      BASE_SECT(world[EXIT(ch,dir)->to_room].sector_type) == SECT_HILLS))
    if (number(1, 101) < GET_SKILL(ch, SKILL_MOUNTAINEER))
      need_movement = 2;
  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch))
  {
     if (need_specials_check && ch->master)
       send_to_char("You are too exhausted to follow.\r\n", ch);
     else
       send_to_char("You are too exhausted.\r\n", ch);
     return (0);
  }
  // Tunnels -- Artus> Modified to allow groups to pass through.
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) && !IS_NPC(ch) &&
      num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) >= 1)
  {
    if (!(ch->master && AFF_FLAGGED(ch, AFF_GROUP) &&
	  (IN_ROOM(ch->master) == EXIT(ch, dir)->to_room)))
    {
      send_to_char("There isn't enough room there for more than one person!\r\n", ch);
      return (0);
    }
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
	!LR_FAIL(ch, LVL_GRGOD))
  {
    send_to_char("You aren't godly enough to use that room!\r\n", ch);
    return (0);
  }

  /* Now we know we're allow to go into the room. */

  // Just incase ...
  if (char_affected_by_timer(ch, TIMER_MEDITATE))
    timer_from_char(ch, TIMER_MEDITATE);
  if (char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
    timer_from_char(ch, TIMER_HEAL_TRANCE);
  
  if (!IS_NPC(ch) && LR_FAIL(ch, LVL_IMMORT))
    GET_MOVE(ch) -= need_movement;

  if (AFF_FLAGGED(ch, AFF_SNEAK) && (subcmd == SCMD_MOVE) &&
      (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_TAG)))
    show = false;

  if (show)
  {
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
    do_environ_leave_message(ch, dir);
    if (ROOM_FLAGGED(ch->in_room, ROOM_BACKWARD))
    {
      sprintf(buf2, "$n is leaves back to front!");
      act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch))
    return 0;
  if (!enter_wtrigger(&world[EXIT(ch, dir)->to_room], ch, dir))
    return 0;

  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  if (show)
  {
    if (MOUNTING(ch))
      act("$n, mounted on $N, has arrived.",
	  TRUE, ch, 0, MOUNTING(ch), TO_ROOM);
    else if (MOUNTING_OBJ(ch))
      special_item_mount_message(ch);
    else
      act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
    do_environ_arrive_message(ch, dir);
    if (ROOM_FLAGGED(ch->in_room, ROOM_BACKWARD))
    { 
      sprintf(buf2, "$n is moving backwards!");
      act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  if (ch->desc != NULL)
    look_at_room(ch, 0);
  if (check_environment_effect(ch))
  {
    if (MOUNTING(ch))
    {
      send_to_char("Your mount screams in agony as it dies.\r\n", ch);
      death_cry(MOUNTING(ch));
      raw_kill(MOUNTING(ch), NULL);
    }
    return 1;
  }

  // Backwards..
  if (ROOM_FLAGGED(ch->in_room, ROOM_BACKWARD))
    send_to_char("Everything here is moving backwards!\r\n", ch);

  if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && LR_FAIL(ch, LVL_IMMORT))
  {
    log_death_trap(ch, DT_DEATH);
    if (MOUNTING(ch))
    {
      send_to_char("Your mount screams in agony as it dies.\r\n", ch);
      death_cry(MOUNTING(ch));
      raw_kill(MOUNTING(ch), NULL);
    }
    // DM - corpses need to be made ...
    death_cry(ch);
    char_from_room(ch);
    char_to_room(ch, was_in);
    // Artus> Make corpse here.. (WAS extract_char(ch))
    raw_kill(ch, NULL);
    return (0);
  }
  
  entry_memory_mtrigger(ch);
  if (!greet_mtrigger(ch, dir))
  {
    char_from_room(ch);
    char_to_room(ch, was_in);
    look_at_room(ch, 0);
  } else
    greet_memory_mtrigger(ch);
  return (1);
}

/* Artus - Post movement checks to be added in here. *
 * Called from perform_move().. Should be OK.        */
void post_move_checks(struct char_data *ch)
{
//  extern struct index_data *mob_index;	/* For aggravate check. */
  struct char_data *i;
  bool aggro_attack(struct char_data *ch, struct char_data *vict, int type);
  SPECIAL(shop_keeper); // For aggravate check.

  // ARTUS - Someone seems to have forgotten to put aggro checks in...
  // DM - fixed - problems for death traps when ch->in_room == NOWHERE (-1)
  // DM - fixed - added IS_GHOST and CAN_SEE checks on "ch is player" and gave 
  //              hit() the correct args ... (i and ch were around wrong way)
  if (IN_ROOM(ch) == NOWHERE)
    return;
  if (IS_NPC(ch))
  {
    for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
    {
      if ((i == ch) || !CAN_SEE(ch, i))
	continue;
      if (IS_NPC(i))
      {
	// If we're a helper, and they're fighting, get in on the action.
	if (MOB_FLAGGED(ch, MOB_HELPER) && FIGHTING(i) &&
	    aggro_attack(ch, i, AGGRA_HELPER))
	  return;
	continue;
      }
      // i is a PC.
      if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && 
	  aggro_attack(ch, i, AGGRA_AGGRESSIVE))
	return;
      else if (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(i) &&
	  aggro_attack(ch, i, AGGRA_AGGR_EVIL))
	return;
      else if (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(i) &&
	  aggro_attack(ch, i, AGGRA_AGGR_GOOD))
	return;
      // Aggravate punishment.
      if (PUN_FLAGGED(i, PUN_AGGRAVATE) && aggro_attack(ch, i, AGGRA_PUNISH))
	return;
    }
    return;
  } // End of NPC Movement Checks.
  // ch is a player.
  if (IS_GHOST(ch) || PRF_FLAGGED(ch, PRF_NOHASSLE))
    return;

  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
  {
    // Basic checks. (i == ch check made not needed with !IS_NPC check)
    if (!IS_NPC(i) || FIGHTING(i) || (GET_POS(i) <= POS_SLEEPING) ||
	!CAN_SEE(ch, i))
      continue;
    if (MOB_FLAGGED(i, MOB_AGGRESSIVE) && aggro_attack(i, ch, AGGRA_AGGRESSIVE))
      return;
    else if (MOB_FLAGGED(i, MOB_AGGR_EVIL) && IS_EVIL(ch) &&
	     aggro_attack(i, ch, AGGRA_AGGR_EVIL))
      return;
    else if (MOB_FLAGGED(i, MOB_AGGR_GOOD) && IS_GOOD(ch) &&
	     aggro_attack(i, ch, AGGRA_AGGR_GOOD))
      return;
    if (PUN_FLAGGED(ch, PUN_AGGRAVATE) && aggro_attack(i, ch, AGGRA_PUNISH))
      return;
  } /* Room/Char List */
}

int perform_move(struct char_data *ch, int aDir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k, *next;
  int this_skill=0, best_skill=0, percent=0, prob=0;
  struct char_data *best_char = NULL;
  bool death_room = FALSE;
  int dir = aDir;
  
  /* JA for backwards rooms */
//  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_BACKWARD))  r
//  sprintf(buf2, "checking sect flag : %d\n", world[ch->in_room].sector_type);
//  sprintf(buf2, "checking sect flag : %d\n", world[ch->in_room].sector_type);

 
//  send_to_char(buf2, ch);
//  sprintf(buf2, "checking room flag : %d\n", world[ch->in_room].room_flags );
//  send_to_char(buf2, ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_BACKWARD))
  {
    assert(aDir>=0 && aDir<=5);
    dir = DIR_REVERSE[aDir];
//    send_to_char("Everything here is moving backwards.\n\r", ch);
  }

//  int sect_type = world[ch->in_room].sector_type & 0xfff0;
//  if (ENVIRON(sect_type) == SECT_BACKWARD) {
//    assert(aDir>=0 && aDir<=5);
//    dir = DIR_REVERSE[aDir];
//  }

  if (IS_AFFECTED(ch, AFF_PARALYZED))
  {
    send_to_char("PANIC! Your legs refuse to move!!\r\n", ch);
    return(0);
  }

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
    return (0);
  if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE)
  {
    send_to_char("Alas, you cannot go that way...\r\n", ch);
    return (0);
  }
  if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
  {
    if (EXIT(ch, dir)->keyword)
    {
      sprintf(buf2, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf2, ch);
      return (0);
    }
    send_to_char("It seems to be closed.\r\n", ch);
    return (0);
  }
  // DM - check for SKILL_DETECT_DEATH
  if (!IS_NPC(ch) && is_death_room(EXIT(ch, dir)->to_room))
    death_room = TRUE;
    // Single char movement
  if (!ch->followers)
  {
    if (!IS_NPC(ch) && (is_death_room(EXIT(ch, dir)->to_room)))
    {
      if ((prob=GET_SKILL(ch, SKILL_DETECT_DEATH)) > 0)
      { 
	percent = number(1, 101);     /* 101% is a complete failure */
	if (percent <= prob)
	{
	  send_to_char("You sense that moving in that direction will lead to your death.\r\n",ch);
	  apply_spell_skill_abil(ch, SKILL_DETECT_DEATH);
	  return 0; 
	} 
      }
    }
    if (do_simple_move(ch, dir, need_specials_check) == 1)
    {
      post_move_checks(ch);
      return 1;
    } 
    return 0;
  }

  // Group Movement
  was_in = ch->in_room;

  if (!do_simple_move(ch, dir, need_specials_check))
    return (0);

  if (death_room)
  {
    // Find the char with the best SKILL_DETECT_DEATH
    for (k = ch->followers; k; k = next)
    {
      next = k->next;
      if (!IS_NPC(k->follower) && (k->follower->in_room == was_in))
      {
	this_skill = GET_SKILL(k->follower, SKILL_DETECT_DEATH);
	if (this_skill > best_skill)
	{
	  best_skill = this_skill;
	  best_char = k->follower;
	}
      }
    }

    if (best_skill > 0)
    {
      percent = number(1, 101);     /* 101% is a complete failure */
      prob = GET_SKILL(ch, SKILL_DETECT_DEATH);

      if (percent <= prob)
      {
	for (k = ch->followers; k; k = next)
	{
	  next = k->next;
	  if (k->follower->in_room == was_in)
	  {
	    if (k->follower == best_char)
	      send_to_char("You sense that moving in that direction will lead to your death.\r\n",ch);
	    else
	      act("$N senses moving in that direction will lead to the group's demise.\r\n", FALSE, best_char, 0, ch, TO_CHAR);
	  }
	}
	apply_spell_skill_abil(ch, SKILL_DETECT_DEATH);
	return (0); 
      }
    } 
  }

  for (k = ch->followers; k; k = next)
  {
    next = k->next;
    if ((k->follower->in_room == was_in) &&
	(GET_POS(k->follower) >= POS_STANDING))
    {
      act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
      perform_move(k->follower, dir, 1);
    }
  }
  return (1);
}

ACMD(do_move)
{
  if (IS_AFFECTED(ch, AFF_PARALYZED))
  {
    send_to_char("You cannot move you are paralyzed.\r\n", ch);
    return;
  }
  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  if (perform_move(ch, subcmd - 1, 0) == 0) 
    return;
}


int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname, bool dispMessages)
{
  int door;
  if (*dir)
  {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1)
    {	/* Partial Match */
      if (dispMessages)
        send_to_char("That's not a direction.\r\n", ch);
      return (-1);
    }
    if (EXIT(ch, door))
    {	/* Braces added according to indent. -gg */
      if (EXIT(ch, door)->keyword)
      {
	if (isname(type, EXIT(ch, door)->keyword))
	  return (door);
	else
	{
          if (dispMessages)
	  {
	    sprintf(buf2, "I see no %s there.\r\n", type);
	    send_to_char(buf2, ch);
          }
	  return (-1);
        }
      } else
	return (door);
    } else {
      if (dispMessages)
      {
        sprintf(buf2, "I really don't see how you can %s anything there.\r\n", 
		cmdname);
        send_to_char(buf2, ch);
      }
      return (-1);
    }
  } else {			/* try to locate the keyword */
    if (!*type)
    {
      if (dispMessages)
      {
        sprintf(buf2, "What is it you want to %s?\r\n", cmdname);
        send_to_char(buf2, ch);
      }
      return (-1);
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
      if ((EXIT(ch, door)) && (EXIT(ch, door)->keyword) &&
	  (isname(type, EXIT(ch, door)->keyword)))
	return (door);
    if (dispMessages)
    {
      sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
      send_to_char(buf2, ch);
    }
    return (-1);
  }
}

#define SKELETON_KEY_VNUM	4523
int has_key(struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;

  for (o = ch->carrying; o; o = o->next_content)
  {
    if (GET_OBJ_VNUM(o) == key)
      return (1);
    if (GET_OBJ_VNUM(o) == SKELETON_KEY_VNUM)
    {
      for (int i = 0; i < MAX_QUEST_ITEMS; i++)
	if (GET_QUEST_ITEM(ch,i) == SKELETON_KEY_VNUM)
	  return (1);
      return (0);
    }
  }

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

  // a broken container - must be fixed before you can peform use it 
  if ((obj) && (GET_OBJ_DAMAGE(obj) == 0))
  {
    send_to_char("Nope, its broken. Perhaps you should fix it!\r\n", ch);
    return;
  }
  
  // TODO: check that no other skills are passed in
  if (scmd == SKILL_PICK_LOCK)
    sprintf(buf, "$n %ss ", cmd_door[SCMD_PICK]);
  else
    sprintf(buf, "$n %ss ", cmd_door[scmd]);

  if ((!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE)) &&
      ((back = world[other_room].dir_option[rev_dir[door]]) != NULL) &&
      (back->to_room != ch->in_room))
    back = 0;

  switch (scmd)
  {
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
    case /*SCMD_PICK*/SKILL_PICK_LOCK:
      LOCK_DOOR(ch->in_room, obj, door);
      if (back)
	LOCK_DOOR(other_room, obj, rev_dir[door]);
      send_to_char("The lock quickly yields to your skills.\r\n", ch);
      strcpy(buf, "$n skillfully picks the lock on ");
      break;
  }
  /* Notify the room */
  sprintf(buf2, "%s%s.", ((obj) ? "" : "the "),
          (obj) ? "$p" : (EXIT(ch, door)->keyword ? "$F" : "door"));
  strncat(buf, buf2, strlen(buf2));
  //sprintf(buf + strlen(buf), "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" :
	//  (EXIT(ch, door)->keyword ? "$F" : "door"));
  if (!(obj) || (obj->in_room != NOWHERE))
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back)
  {
    sprintf(buf, "The %s is %s%s from the other side.",
	    (back->keyword ? fname(back->keyword) : "door"), cmd_door[scmd],
	    (scmd == SCMD_CLOSE) ? "d" : "ed");
    if (world[EXIT(ch, door)->to_room].people)
    {
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_ROOM);
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_CHAR);
    }
  }
}

int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
  int percent;

  percent = number(1, 101);
  if (scmd == SKILL_PICK_LOCK/*SCMD_PICK*/)
  {
    if (keynum < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if (pickproof)
      send_to_char("It resists your attempts to pick it.\r\n", ch);
    else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
      send_to_char("You failed to pick the lock.\r\n", ch);
    else
    {
      apply_spell_skill_abil(ch, scmd);
      return (1);
    }
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
  int cmdindex = subcmd;

  if (cmdindex == SKILL_PICK_LOCK)
    cmdindex = SCMD_PICK;
  
  skip_spaces(&argument);
  if (!*argument)
  {
    sprintf(buf, "%s what?\r\n", cmd_door[cmdindex]);
    send_to_char(CAP(buf), ch);
    return;
  }
  two_arguments(argument, type, dir);
  // DM - switched these two checks, first we'll look for a door, and then an
  // object: to get over problem of trying to open doors when you have a piece
  // of equipment with the same name as the door alias.
  if ((door = find_door(ch, type, dir, cmd_door[cmdindex], FALSE)) < 0)
    generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj);
  // We failed to find a door or an object - give the door messages
  if (!obj)
    find_door(ch, type, dir, cmd_door[cmdindex], TRUE);

  //if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
  //  door = find_door(ch, type, dir, cmd_door[cmdindex]);

  // TODO: Tali: is this the inteded functionality for traps on objects, ie. 
  // at this stage all you know is that the arg given matched an obj ... DM
  
  // Do some trap checking
  if ((obj) && (GET_OBJ_TYPE(obj) == ITEM_TRAP))
  {
    if (IS_THIEF(ch))
      chance = 15;
    else if (IS_MAGIC_USER(ch))
      chance = 5;
    else
      chance = 2;
    if (number(1, chance) == chance) 
      ch_trigger_trap(ch, obj);
    else
      send_to_char("Strange, it won't budge.\r\n", ch);
    return;
  }
  if ((obj) || (door >= 0))
  {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("You can't $F that!", FALSE, ch, 0, cmd_door[cmdindex], TO_CHAR);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[cmdindex], NEED_OPEN))
      send_to_char("But it's already closed!\r\n", ch);
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[cmdindex], NEED_CLOSED))
      send_to_char("But it's currently open!\r\n", ch);
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[cmdindex], NEED_LOCKED))
      send_to_char("Oh.. it wasn't locked, after all..\r\n", ch);
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[cmdindex], NEED_UNLOCKED))
      send_to_char("It seems to be locked.\r\n", ch);
    else if (!has_key(ch, keynum) && LR_FAIL(ch, LVL_GOD) &&
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
  if (*buf)
  {			/* an argument was supplied, search for door
			 * keyword */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door) && (EXIT(ch, door)->keyword) &&
	  !str_cmp(EXIT(ch, door)->keyword, buf))
      {
	perform_move(ch, door, 1);
	return;
      }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
    send_to_char("You are already indoors.\r\n", ch);
  else
  {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door) && (EXIT(ch, door)->to_room != NOWHERE) &&
	  !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	  ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS))
      {
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
  else
  {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door) && (EXIT(ch, door)->to_room != NOWHERE) &&
	  !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	  !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS))
      {
	perform_move(ch, door, 1);
	return;
      }
    send_to_char("I see no obvious exits to the outside.\r\n", ch);
  }
}

ACMD(do_stand)
{
  // DM - meditate, cant stand whilst meditating
  if (!IS_NPC(ch) && (char_affected_by_timer(ch, TIMER_MEDITATE) || char_affected_by_timer(ch, TIMER_HEAL_TRANCE))) 
  {
    send_to_char("Stop mucking around and get back to meditating.\r\n", ch);
    return;
  }
  
  switch (GET_POS(ch))
  {
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
  switch (GET_POS(ch))
  {
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
  if (MOUNTING(ch))
  {
    send_to_char("You cannot rest while mounted.\r\n", ch);
    return;
  }
  // DM - meditate, cant stand whilst meditating
  if (!IS_NPC(ch) && (char_affected_by_timer(ch, TIMER_MEDITATE) ||
	              char_affected_by_timer(ch, TIMER_HEAL_TRANCE))) 
  {
    send_to_char("Stop mucking around and get back to meditating.\r\n", ch);
    return;
  }
  switch (GET_POS(ch))
  {
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
  // DM - meditate, cant stand whilst meditating
  if (!IS_NPC(ch) && (char_affected_by_timer(ch, TIMER_MEDITATE) ||
	              char_affected_by_timer(ch, TIMER_HEAL_TRANCE))) 
  {
    send_to_char("Stop mucking around and get back to meditating.\r\n", ch);
    return;
  }
  if (MOUNTING(ch))
  {
    send_to_char("You cannot sleep while mounted.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOSLEEP))
  {
    send_to_char("You can't possibly go to sleep here!\r\n", ch);
    return;
  }
  switch (GET_POS(ch))
  {
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
  if (*arg)
  {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char("Maybe you should wake yourself up first.\r\n", ch);
    else if ((vict = generic_find_char(ch, arg, FIND_CHAR_ROOM)) == NULL)
      send_to_char(NOPERSON, ch);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else
    {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_STANDING;
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char("You can't wake up!\r\n", ch);
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char("You are already awake...\r\n", ch);
  else
  {
    send_to_char("You awaken, and stand up.\r\n", ch);
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
  }
}

ACMD(do_follow)
{
  struct char_data *leader;

  one_argument(argument, buf);

  if (*buf)
  {
    if (!(leader = generic_find_char(ch, buf, FIND_CHAR_ROOM)))
    {
      send_to_char(NOPERSON, ch);
      return;
    }
  } else {
    send_to_char("Whom do you wish to follow?\r\n", ch);
    return;
  }

  if (ch->master == leader)
  {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))
  {
    act("But you only feel like following $N!",
	FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch)
    {
      if (!ch->master)
      {
	send_to_char("You are already following yourself.\r\n", ch);
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader))
      {
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

// Generic vampire check.
int check_vampire(struct char_data *ch)
{
  extern struct weather_data weather_info;

  if (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_VAMPIRE))
    return(0);
  if (LR_FAIL(ch, LVL_IS_GOD) || GET_DEBUG(ch))
    return(0);
  if ((weather_info.sunlight == SUN_DARK) || !OUTSIDE(ch))
    return(0);
  for (int i = 0; i < NUM_WEARS; i++)
  {
    if (!(GET_EQ(ch, i)))
      continue;
    switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
    {
      case ITEM_HEATRES:
      case ITEM_HEATPROOF:
      case ITEM_STASIS:
	return(0);
    }
  }
  send_to_char("The sun burns your skin!\r\n", ch);
  act("Smoke rises from $n's skin!",FALSE,ch,0,0,TO_ROOM);
  damage(NULL, ch, MAX(GET_HIT(ch)-GET_LEVEL(ch)/2, 0), TYPE_UNDEFINED, FALSE);
  if (GET_HIT(ch) <= 0)
  {
    send_to_char("The sun burns you to death!\r\n",ch);
    act("The sun causes $n to burst into flames!", TRUE, ch, 0, 0, TO_ROOM);
    if (MOUNTING(ch))
    {
      send_to_char("Your vampiric mount dies with you.\r\n", ch);
      die(MOUNTING(ch), NULL, "sunlight");
    }
    death_cry(ch);
    die(ch,NULL,"sunlight");
    send_to_outdoor("The scream of a burning vampire echoes throughout the land...\r\n");
    return(1);
  }
  return(0);
}

// Artus> Called by check_environment_effect.
int check_env_atmosphere(struct char_data *ch, int atmos)
{
  int i, dam, modifier = 1;

#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch)) 
  {
    sprintf(buf, "DBG: Atmosphere = %d\r\n", atmos);
    send_to_char(buf, ch);
  }
#endif

  switch (atmos)
  {
    case SECT_THIN: ////////////////////////////////////// Thin
      sprintf(buf, "The %s here is very thin.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_RESPIRATE:
	  case ITEM_BREATHER:
	  case ITEM_VACSUIT:
	  case ITEM_ENVIRON:
	    return(0);
	}
      }
      send_to_char("You are gasping for breath!\n\r", ch);
      dam = (int)(MIN(GET_HIT(ch), GET_LEVEL(ch)) / modifier);
      damage(NULL, ch, dam, TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) <= 0)
      {
	GET_POS(ch) = POS_STUNNED;
	act("$n falls down gasping for breath an goes into a coma.\n\r", TRUE, ch, 0, 0, TO_ROOM);
      }
      return (0);
    case SECT_UNBREATHABLE: ////////////////////////////// Unbreathable
      sprintf(buf, "The %s here is unbreathable.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!(GET_EQ(ch, i)))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_RESPIRATE:
	    modifier++;
	    break;
	  case ITEM_BREATHER:
	  case ITEM_VACSUIT:
	  case ITEM_ENVIRON:
	    return(0);
	}
      }
      send_to_char("You can't breath, you are asphyxiating!\n\r", ch);
      damage(NULL, ch, MIN(GET_HIT(ch), GET_LEVEL(ch)*3), TYPE_UNDEFINED,
	     FALSE);
      if (GET_HIT(ch) < 0)
      {
	send_to_char("You suffocate to death!\n\r", ch);
	act("$n falls down gasping for breath and dies.", TRUE, ch, 0, 0, TO_ROOM); 
	death_cry(ch);
	die(ch,NULL,"unbreathable air");
	return(1);
      }
      return(0);
    case SECT_VACUUM: //////////////////////////////////// Vacuum
      send_to_char("You are in a vacuum.\n\r", ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_VACSUIT:
	  case ITEM_ENVIRON:
	    return (0);
	}
      }
      send_to_char("You walk into a total vacuum, and your whole body explodes!\n\r", ch);
      act("$n enters a total vacuum and explodes, covering you with blood and gutsi!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      death_cry(ch);
      die(ch, NULL, "vacuum");
      return(1);
    case SECT_CORROSIVE: ///////////////////////////////// Corrosive
      sprintf(buf, "The %s here is thick with toxic chemicals.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!(GET_EQ(ch, i)))
	  continue;
	switch GET_OBJ_TYPE(GET_EQ(ch, i))
	{
	  case ITEM_VACSUIT:
	    modifier++;
	  case ITEM_BREATHER:
	    modifier++;
	  case ITEM_RESPIRATE:
	    modifier++;
	    break;
	  case ITEM_ENVIRON:
	    return(0);
	}
      }
      sprintf(buf, "The chemicals in the %s burn your lungs.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      //GET_HIT(ch) -= 135;
      dam = (int)(GET_LEVEL(ch) * 5 / modifier);
      damage(NULL, ch, dam, TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) < 0)
      {
	send_to_char("You fall down coughing up blood and die.\n\r", ch);
	act("$n falls down coughing up blood and gasping for breath, then dies!\n\r", TRUE, ch, 0, 0, TO_ROOM);
	death_cry(ch);
	die(ch,NULL,"corrosion");
	return(1); 
      }
      return(0);
  }
  return (0);
}

// Called by check_environment_effect
int check_env_temperature(struct char_data *ch, int temperature)
{
  int i, modifier=1, dam;

#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: Temperature = %d\r\n", temperature);
    send_to_char(buf, ch);
  }
#endif

  switch(temperature)
  {
    case SECT_HOT: /////////////////////////////////////// Hot
      send_to_char("It is much hotter here than you are used to.\n\r", ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_HEATRES:
	  case ITEM_HEATPROOF:
	  case ITEM_STASIS:
	    return (0);
	}
      }
      send_to_char("The heat is really affecting you!\n\r", ch);
      //GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch), 0);
      dam = MIN(GET_HIT(ch), GET_LEVEL(ch));
      damage(NULL, ch, dam, TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) <= 0)
      {
        GET_POS(ch) = POS_STUNNED;
        act("$n falls down from heat exhaustion!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return (0);
    case SECT_SCORCH: //////////////////////////////////// Scorch
      send_to_char("Its almost hot enough to melt lead here.\n\r", ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_HEATRES:
	    modifier++;
	    break;
	  case ITEM_HEATPROOF:
	  case ITEM_STASIS:
	    return (0);
	}
      }
      dam = (int)((MIN(GET_HIT(ch), GET_LEVEL(ch)*2))/modifier);
      send_to_char("Your skin is smouldering and blistering!\n\r", ch);
      damage(NULL, ch, dam, TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("Your whole body is reduced to a blistered mess, you fall down dead!\n\r", ch);
	act("$n falls down dead in a smouldering heap!", TRUE, ch, 0, 0, TO_ROOM);
	death_cry(ch);
	die(ch,NULL,"scorching");
	return(1);
      }
      return(0);
    case SECT_INCINERATE: //////////////////////////////// Incinerate
      send_to_char("You estimate that it is well over 3000 degrees here.\n\r", ch);
      send_to_char("Your skin catches fire! You are burnt to a cinder\n\r", ch);
      act("$n spontaneously combusts and is reduced to a charred mess on the ground!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      death_cry(ch);
      log_death_trap(ch, DT_INCINERATE);
      raw_kill(ch, NULL);
      return(1);
    case SECT_COLD: ////////////////////////////////////// Cold
      send_to_char("Its much colder here than you are used to.\n\r", ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_COLD:
	  case ITEM_SUBZERO:
	  case ITEM_STASIS:
	    return (0);
	}
      }
      send_to_char("You shiver uncontrollably from the cold!\n\r", ch);
      dam = MIN(GET_HIT(ch), GET_LEVEL(ch));
      damage(NULL, ch, dam, TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("You fall down unconcious from hyperthermia!\n\r", ch);
	GET_POS(ch) = POS_STUNNED;
	act("$n falls down in a coma from hyperthermia!",
	    TRUE, ch, 0, 0, TO_ROOM);
      }
      return(0);
    case SECT_FREEZING: ////////////////////////////////// Freezing
      sprintf(buf, "The %s here is freezing.\r\n",
	      (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_COLD:
	    modifier++;
	    break;
	  case ITEM_SUBZERO:
	  case ITEM_STASIS:
	    return (0);
	}
      }
      send_to_char("You are suffering from severe frostbite!\n\r", ch);
      dam = (int)(MIN(GET_HIT(ch), (GET_LEVEL(ch)*2))/modifier);
      damage(NULL, ch, dam, TYPE_UNDEFINED, 0);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("You fall down dead from frostbite!\n\r", ch);
	act("$n falls down dead from frostbite and hyperthermia!", TRUE, ch, 0, 0, TO_ROOM);
	death_cry(ch);
	die(ch,NULL,"freezing cold");
	return(1);
      }
      break;   
    case SECT_ABSZERO: /////////////////////////////////// Absolute Zero
      send_to_char("It dosn't get much colder than this... absolute zero!\n\r",
	           ch);
      send_to_char("Your body is frozen rock hard and shatters into many pieces!\n\r", ch);
      act("$n is frozen solid, tries to move and shatters into thousands of pieces!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      death_cry(ch);
      die(ch,NULL,"absolute zero");
      return(1);
      break;  
  }
  return(0);
}

// Called by check_environment_effect
int check_env_gravity(struct char_data *ch, int gravity)
{
  int i, modifier=1, dam;

#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: Gravity = %d\r\n", gravity);
    send_to_char(buf, ch);
  }
#endif

  switch (gravity)
  {
    case SECT_DOUBLEGRAV: //////////////////////////////// Doublegrav
      send_to_char("You feel much heavier than you are used to.\r\n", ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_GRAV1:
	  case ITEM_GRAV3:
	    return(0);
	}
      }
      send_to_char("The extra weight sends pain right through your legs!\r\n",
	           ch);
      dam = MIN(GET_HIT(ch), GET_LEVEL(ch));
      damage(NULL, ch, dam, TYPE_UNDEFINED, 0);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("Your legs can no longer sustrain your weight.\r\n", ch);
	act("$n falls to the floor, $s legs collapsing beneath $m.", TRUE, ch,
	    0, 0, TO_ROOM);
	GET_POS(ch) = POS_STUNNED;
      }
      return(0);
    case SECT_TRIPLEGRAV: //////////////////////////////// Triplegrav
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	send_to_char("You feel like you weigh a tonne!\r\n", ch);
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_GRAV1:
	    modifier++;
	    break;
	  case ITEM_GRAV3:
	    return(0);
	}
      }
      send_to_char("Your legs spasm with the pain of the extra weight you feel here.\r\n", ch);
      dam = (int)(MIN(GET_HIT(ch), GET_LEVEL(ch)*3)/modifier);
      damage(NULL, ch, dam, TYPE_UNDEFINED, 0);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("You are crushed to a pulp!\r\n", ch);
	act("$n is crushed to a pulp by the extremely strong gravity, here.",
	    TRUE, ch, 0, 0, TO_ROOM);
	death_cry(ch);
	die(ch, NULL, "gravity");
	return(1);
      }
      return (0);
    case SECT_CRUSH: ///////////////////////////////////// Crush
      send_to_char("You must be very near to a singularity, the gravity is approaching infinity.\r\nYour body cannot take it anymore!\r\n", ch);
      act("$n is crushed to a single molecule!", TRUE, ch, 0, 0, TO_ROOM);
      death_cry(ch);
      die(ch,NULL,"gravity");
      return(1);
  }
  return 0;
}


#define MAX_DISPAIR_MESSAGES 3
char *DISPAIR_CHAR_MESSAGES[MAX_DISPAIR_MESSAGES] = {
  "This place is getting you down a bit.",
  "This place is quite depressing.",
  "This place is making you suicidal."
};

// Artus> Used by check_environment_effect
int check_env_environ(struct char_data *ch, int environ)
{
  int i, dam;
  struct affected_type *af, *next_af;

#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: Environ = %d\r\n", environ);
    send_to_char(buf, ch);
  }
#endif

  switch(environ)
  {
    case SECT_RAD1: ////////////////////////////////////// Radiation
      send_to_char("If you had a geiger counter it would be off the scale.\r\n", ch);
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (!GET_EQ(ch, i))
	  continue;
	switch (GET_OBJ_TYPE(GET_EQ(ch, i)))
	{
	  case ITEM_RAD1PROOF:
	  case ITEM_STASIS:
	    return(0);
	}
      }
      send_to_char("You feel decidedly sick in the stomach. The radiation is really affecting you.\r\n", ch);
      dam = MIN(GET_HIT(ch), GET_LEVEL(ch));
      damage(NULL, ch, dam, TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) <= 0)
      {
        act("$n passes out from radiation poisining!\r\n", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You pass out from radiation poisoining!\r\n",ch);
        GET_POS(ch) = POS_STUNNED;
      }
      return(0);
    case SECT_DISPELL: /////////////////////////////////// Dispell
      dam = 0;
      /* Artus> This will bork at abilities.
      while (ch->affected)
        affect_remove(ch, ch->affected); */
      for (af = ch->affected; af; af=next_af)
      {
        next_af = af->next;
	/* Don't remove abilities.. */
	if ((af->duration != CLASS_ABILITY) && (af->duration != CLASS_ITEM))
	{
	  affect_remove(ch, af);
	  dam++;
	}
      }
      // Artus> So we don't keep repeating this message :o)
      if (dam > 0)
	send_to_char("Your body shivers as a flash of bright light strips you of all spells!!!\r\n", ch);
      break;
    case SECT_DISPAIR1 :
    case SECT_DISPAIR2 :
    case SECT_DISPAIR3 :

      int depression = environ - 2; // gives a value 1 to 3

      // send a message to the char
      sprintf(buf2, "You are depressed by %d points\nSect type is %d\n", depression, BASE_SECT(world[ch->in_room].sector_type));
      send_to_char(buf2, ch);
      GET_MOVE(ch) -= 1+(depression * depression)/2;
      if (GET_MOVE(ch) < 0) {
        GET_MOVE(ch) = 0;
      }
      GET_MANA(ch) -= 1+(depression * depression);
      if (GET_MANA(ch) < 0) {
        GET_MANA(ch) = 0;
        // suicide
	  send_to_char("You can't take it any more, you commit suicide!\n\r", ch);
	  act("$n commits suicide and dies.", TRUE, ch, 0, 0, TO_ROOM); 
	  raw_kill(ch,NULL);
	  return(1);
      }
                                                                                                    
      break;

      return(0);
  }
  return(0);
}

// Check environment of one specific char.
int check_environment_effect(struct char_data *ch)
{
  int sect_type;
/* JA new sector flags for different worlds */

  if (IS_NPC(ch)) 
    return(0);
  sect_type = world[ch->in_room].sector_type & 0xfff0;
  if (!LR_FAIL(ch, LVL_IS_GOD)) // Artus> Changed from IMPL.
  {
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch))
    {
      sprintf(buf, "DBG: Sect_Type: %d\r\n", sect_type);
      send_to_char(buf, ch);
    } else
#endif
      return(0);
  }
  
  if (check_env_atmosphere(ch, ATMOSPHERE(sect_type)))
    return 1;
  if (check_env_temperature(ch, TEMPERATURE(sect_type)))
    return 1;
  if (check_env_gravity(ch, GRAVITY(sect_type)))
    return 1;
  if (check_env_environ(ch, ENVIRON(sect_type)))
    return 1;

  if (check_vampire(ch))
    return 1;

  return(0);
  // This has all been replaced by the above.
#if 0 // check_atmosphere
  switch(ATMOSPHERE(sect_type))
  {
    case SECT_THIN: 
      sprintf(buf, "The %s here is very thin.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      if (is_wearing(ch, ITEM_RESPIRATE))
        break;
      else if (is_wearing(ch, ITEM_BREATHER))
	break;
      else if (is_wearing(ch, ITEM_VACSUIT))
	break;
      else if (is_wearing(ch, ITEM_ENVIRON))
	break;
      else
      {
	send_to_char("You are gasping for breath!\n\r", ch);
	damage(NULL, ch, MIN(GET_HIT(ch), GET_LEVEL(ch)), TYPE_UNDEFINED, 
	       FALSE);
	// GET_HIT(ch) = MAX(GET_HIT(ch)-GET_LEVEL(ch), 0);
	if (GET_HIT(ch) <= 0)
	{
	  GET_POS(ch) = POS_STUNNED;
	  act("$n falls down gasping for breath an goes into a coma.\n\r", TRUE, ch, 0, 0, TO_ROOM);
	}
      }
      break;
    case SECT_UNBREATHABLE :
      sprintf(buf, "The %s here is unbreathable.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      if (is_wearing(ch, ITEM_BREATHER))
        break;
      else if (is_wearing(ch, ITEM_VACSUIT))
	break;
      else if (is_wearing(ch, ITEM_ENVIRON))
	break;
      else
      {
	send_to_char("You can't breath, you are asphyxiating!\n\r", ch);
	//GET_HIT(ch) = MAX(GET_HIT(ch)-(GET_LEVEL(ch)*3), 0);
	damage(NULL, ch, MIN(GET_HIT(ch), GET_LEVEL(ch)*3), TYPE_UNDEFINED,
	       FALSE);
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
      else if (is_wearing(ch, ITEM_ENVIRON))
	break;
      else
      {
	send_to_char("You walk into a total vacuum, and your whole body explodes!\n\r", ch);
	act("$n enters a total vacuum and explodes, covering you with blood and gutsi!\n\r", TRUE, ch, 0, 0, TO_ROOM);
	raw_kill(ch,NULL);
	return(1);
      }  
      break;
    case SECT_CORROSIVE:
      sprintf(buf, "The %s here is thick with toxic chemicals.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch);
      if (!is_wearing(ch, ITEM_ENVIRON))
      {
	sprintf(buf, "The chemicals in the %s burn your lungs.\r\n", (UNDERWATER(ch)) ? "water" : "air");
        send_to_char(buf, ch);
        //GET_HIT(ch) -= 135;
	damage(NULL, ch, 135, TYPE_UNDEFINED, FALSE);
        if (GET_HIT(ch) < 0)
        {
          send_to_char("You fall down coughing up blood and die.\n\r", ch);
          act("$n falls down coughing up blood and gasping for breath, then dies!\n\r", TRUE, ch, 0, 0, TO_ROOM);   
          raw_kill(ch,NULL);
          return(1); 
        }
      }
      break;
    default:
      // TODO - handle this
      break;
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
	//GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch), 0);
	damage(NULL, ch, MIN(GET_HIT(ch), GET_LEVEL(ch)), TYPE_UNDEFINED,
	       FALSE);
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
      //GET_HIT(ch) = MAX((GET_HIT(ch) - GET_LEVEL(ch)*2/modifier), 0);
      damage(NULL, ch, MIN(GET_HIT(ch), (int)(GET_LEVEL(ch)*2/modifier)),
	     TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("Your whole body is reduced to a blistered mess, you fall down dead!\n\r", ch);
	act("$n falls down dead in a smouldering heap!", TRUE, ch, 0, 0, TO_ROOM);
	raw_kill(ch,NULL);
	return(1);
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
	//GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch), 0);
	damage(NULL, ch, MIN(GET_HIT(ch), GET_LEVEL(ch)), TYPE_UNDEFINED,
	       FALSE);
	if (GET_HIT(ch) <= 0)
	{
	  send_to_char("You fall down unconcious from hyperthermia!\n\r", ch);
	  GET_POS(ch) = POS_STUNNED;
	  act("$n falls down in a coma from hyperthermia!", TRUE, ch, 0, 0, TO_ROOM);
	}
      }
      break;
    case SECT_FREEZING :
      sprintf(buf, "The %s here is freezing.\r\n", (UNDERWATER(ch)) ? "water" : "air");
      send_to_char(buf, ch); 
      if (is_wearing(ch, ITEM_SUBZERO) || is_wearing(ch, ITEM_STASIS))   
	break;
      if (is_wearing(ch, ITEM_COLD))
	modifier = 2;
      send_to_char("You are suffering from severe frostbite!\n\r", ch);
      //GET_HIT(ch) = MAX(GET_HIT(ch) - (GET_LEVEL(ch)*2/modifier), 0);
      damage(NULL, ch, MIN(GET_HIT(ch), (int)(GET_LEVEL(ch)*2/modifier)),
	     TYPE_UNDEFINED, 0);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("You fall down dead from frostbite!\n\r", ch);
	act("$n falls down dead from frostbite and hyperthermia!", TRUE, ch, 0, 0, TO_ROOM);
	raw_kill(ch,NULL);
	return(1);
      }
      break;   
    case SECT_ABSZERO:
      send_to_char("It dosn't get much colder than this... absolute zero!\n\r", ch);
      send_to_char("Your body is frozen rock hard and shatters into many pieces!\n\r", ch);
      act("$n is frozen solid, tries to move and shatters into thousands of pieces!\n\r", TRUE, ch, 0, 0, TO_ROOM);
      raw_kill(ch,NULL);
      return(1);
      break;  
  }
  // Artus> Gravity isn't doing anything.. Lets fix that.. :o)
  if ((modifier = GRAVITY(sect_type)) > 0)
  {
    // Artus> For calculating damage in gravity.
    struct obj_data *weighing = NULL;
    int eqweight = 0;
    // First We'll Handle Crush.
    if (modifier == SECT_CRUSH)
    {
      send_to_char("You must be very near to a singularity, the gravity is approaching infinity.\r\nYour body cannot take it anymore!\r\n", ch);
      sprintf(buf, "%s killed by gravity at %s", GET_NAME(ch), world[ch->in_room].name);
      mudlog(buf, BRF, MAX(LVL_ANGEL, GET_INVIS_LEV(ch)), TRUE);
      raw_kill(ch,NULL);
      return(1);
    }

    for (int i = 0; i < NUM_WEARS; i++)
    {
      if (GET_EQ(ch, i))
      {
	if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_GRAV1)
	{
	  if (GRAVITY(sect_type) == SECT_DOUBLEGRAV)
	  {
	    eqweight = -1;
	    break;
	  } else
	    eqweight += (int)(GET_EQ_WEIGHT(ch, i)/2);
	  continue;
	}
	if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_GRAV3)
	{
	  eqweight = -2;
	  break;
	}
	eqweight += GET_EQ_WEIGHT(ch, i);
      } // Is Equipped.
    } // Check Wears.
    if (eqweight >= 0)
    {
      int maxdam = 0;
      if (modifier == SECT_DOUBLEGRAV)
      {
	send_to_char("You feel a little heavier than you are used to.\r\n", ch);
	eqweight *= 2;
	maxdam = GET_MAX_HIT(ch) / 10;
	maxdam = MAX(GET_MAX_HIT(ch) / 25, MIN(GET_MAX_HIT(ch)-1, (int)(maxdam * IS_CARRYING_W(ch) / CAN_CARRY_W(ch))));
      }
      if (modifier == SECT_TRIPLEGRAV)
      {
	send_to_char("You feel much heavier than you are used to.\r\n", ch);
	eqweight *= 3;
	maxdam = GET_MAX_HIT(ch) / 5;
	maxdam = MAX(GET_MAX_HIT(ch) / 15, MIN(GET_MAX_HIT(ch)-1, (int)(maxdam * IS_CARRYING_W(ch) / CAN_CARRY_W(ch))));
      }
      
      //GET_HIT(ch) = MAX(0,GET_HIT(ch) - number((int)(maxdam * 0.9),maxdam));
      damage(NULL, ch, MIN(GET_HIT(ch), number((int)(maxdam*0.9),maxdam)),
	     TYPE_UNDEFINED, FALSE);
      if (GET_HIT(ch) <= 0)
      {
	send_to_char("Your body can no longer support your weight.\r\n", ch);
	sprintf(buf, "$n is crushed to death by $s own %s!", 
		(eqweight == 0) ? "inventory" : "equipment");
	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	sprintf(buf, "%s killed by gravity at %s", GET_NAME(ch), world[ch->in_room].name);
	mudlog(buf, BRF, MAX(LVL_ANGEL, GET_INVIS_LEV(ch)), TRUE);
	raw_kill(ch, NULL);
	return(1);
      }
    } else { // Eq found with gravity type. Send them a message anyway.
      send_to_char("Gravity hugs you a little tighter.\r\n", ch);
    }
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
        //GET_HIT(ch) = MAX(GET_HIT(ch) - GET_LEVEL(ch), 0);
	damage(NULL, ch, MIN(GET_HIT(ch), GET_LEVEL(ch)), TYPE_UNDEFINED, 
	       FALSE);
      }
      if (GET_HIT(ch) <= 0)
      {
        act("$n passes out from radiation poisining!\n\r", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You pass out from radiation poisoining!\n\r",ch);
        GET_POS(ch) = POS_STUNNED;
      }
      break;
    case SECT_DISPELL :
      send_to_char("Your body shivers as a flash of bright light stripps you of all spells!!!.\n\r", ch);
      while (ch->affected)
        affect_remove(ch, ch->affected);
      break;
    case SECT_DISPAIR1 :
    case SECT_DISPAIR2 :
    case SECT_DISPAIR3 :
      int depression = ENVIRON(sec_type) - 2; // gives a value 1 to 3
      // use the base sect type to remove some mana and mv
      // int depression = BASE_SECT(world[ch->in_room].sector_type) + 1;
      GET_MOVE(ch) -= 1+(depression * depression)/2;
      GET_MANA(ch) -= 1+depression * depression;
       
      // send a message to the char
      sprintf(buf2, "You are depressed by %d points\nSect type is %d\n", depression, BASE_SECT(world[ch->in_room].sector_type));
      send_to_char(buf2, ch);
      break;
  }
  return(0);
#endif
} 

// Artus> Check environment of all playing players.
void check_world_environment(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d=d->next)
  {
    if (!(IS_PLAYING(d) && (d->character)))
      continue;
    check_environment_effect(d->character);
  }  
}

ACMD(do_go)
{
  /* this will be somewhat more sophisticated and allow players
     to board different obects, but for now we only new it for
     the pirate ship, so I'm just hard coding it for that */
  int invalid_level(struct char_data *ch, struct obj_data *object,
                    bool display);
  char arg1[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int was_in;
  room_rnum exitroom;

  one_argument(argument, arg1);
  if (!*arg1)
  {
    send_to_char("Go where??\n\r", ch);
    return;
  }
  if (!(obj = find_obj_list(ch, arg1, world[ch->in_room].contents)))
  {
    sprintf(buf, "You don't see %s %s here.\r\n", AN(arg1), arg1);
    send_to_char(buf, ch);
    return;
  }
  if (GET_OBJ_TYPE(obj) != ITEM_GATEWAY)
  {
    send_to_char("You can't 'go' to that!!\n\r", ch);
    return;
  }
  // Artus> Use Level Restrictions..
  if (invalid_level(ch, obj, FALSE) || IS_NPC(ch) ||
      LR_FAIL(ch, GET_OBJ_VAL(obj, 1)) || 
      ((GET_OBJ_VAL(obj, 2) > 0) && 
       (GET_OBJ_VAL(obj, 2) < GET_LEVEL(ch))))
  {
    sprintf(buf, "You cannot seem to enter %s.\r\n", OBJN(obj, ch));
    send_to_char(buf, ch);
    return;
  }
  // Artus> Tollway's.. Heh.
  if ((GET_OBJ_VAL(obj, 3) > 0) && (GET_LEVEL(ch) < LVL_IS_GOD))
  {
    if (GET_GOLD(ch) < GET_OBJ_VAL(obj, 3))
    {
      sprintf(buf, "It costs &Y%d&n to enter %s, which you can't afford.\r\n",
	      GET_OBJ_VAL(obj, 3), OBJN(obj, ch));
      send_to_char(buf, ch);
      return;
    }
  }
  if (((exitroom = real_room(GET_OBJ_VAL(obj, 0))) == NOWHERE) ||
      (exitroom > top_of_world))
  {
    sprintf(buf,"SYSERR: Portal %d: Invalid exit room (%d/%d) for %s.",
	    GET_OBJ_VNUM(obj), GET_OBJ_VAL(obj, 0), exitroom, OBJN(obj, ch));
    mudlog(buf, BRF, LVL_ANGEL, TRUE);
    send_to_char("It's broken!\r\n", ch);
    return;
  }
  // Artus> Make sure the room on the other side is safe.
  if (!char_can_enter(ch, &world[exitroom], TRUE))
    return;
  
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
  if ((GET_OBJ_VAL(obj, 3) > 0) && (GET_LEVEL(ch) < LVL_IS_GOD))
  {
    GET_GOLD(ch) -= GET_OBJ_VAL(obj, 3);
    sprintf(buf, "You notice you have &Y%d&n less coins.\r\n", 
	    GET_OBJ_VAL(obj, 3));
    send_to_char(buf, ch);
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
ACMD(do_breakin) 
{
  struct char_data *victim;

  if (IS_NPC(ch))
    return;

  // Artus - Tali your slack again - could have made this a skill ...
  if (GET_SKILL(ch, SKILL_MOUNT) > 0) 
  {
    if (!has_stats_for_skill(ch, SKILL_MOUNT, TRUE))
      return;
  } else {
    send_to_char("You know nothing about mounting.\r\n", ch);
    return;
  }

  // Find the target
  one_argument(argument, arg);
	
  if (!*arg)
  {
    send_to_char("Just what would you like to break in?\r\n", ch);
    return;
  }	

  // Check the target is present
  if (!(victim = generic_find_char(ch, arg, FIND_CHAR_ROOM)))
  {
    send_to_char("Noone here by that name.\r\n", ch);
    return;
  }

  // Fighting?
  if (FIGHTING(ch) || FIGHTING(victim))
  {
    send_to_char("You can't do that now!\r\n", ch);
    return;
  }

  // Taming themselves?
  if (victim == ch)
  {
    send_to_char("You break yourself in, and feel quite susceptible to being "
                 "ridden.\r\n",ch);
    return;
  }

  if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
    act("$N is too aggressive to be broken in.", FALSE, ch, 0, victim,
        TO_CHAR);
    return;
  }

  if (AFF_FLAGGED(victim, AFF_BROKEN_IN)) {
    act("$N is already broken in.", FALSE, ch, 0, victim, TO_CHAR );
    return;
  }		
	
  // Is the mob mountable? 
  if (MOB_FLAGGED(victim, MOB_MOUNTABLE) && 
      (number(0, 101) < GET_SKILL(ch, SKILL_MOUNT))) //Artus> 101 = Failure.
  {
    /* Check for skill here */
    act("You successfully break $N in.", FALSE, ch, 0, victim, TO_CHAR);
      SET_BIT(AFF_FLAGS(victim), AFF_BROKEN_IN);
  } else {
    act("You try to break $N in, but $E does not respond well.", 
                    FALSE, ch, 0, victim, TO_CHAR);
    if (LR_FAIL(ch, LVL_IMMORT))
      do_violent_skill(victim, ch->player.name, 0, SCMD_HIT);	
  }
}

/* Performs the mounting manouevre on a mobile or appropriate object */
/* Is it just me or does all this sound a bit sus?  - Tal */
ACMD(do_mount) 
{
  int bits;
  struct char_data *found_char = NULL;
  struct obj_data *found_obj = NULL;

  if (IS_NPC(ch)) 
    return;

  one_argument(argument, arg);

  // DM - Tali your slack again - could have made this a skill ...
  if (GET_SKILL(ch, SKILL_MOUNT) > 0) 
  {
    if (!has_stats_for_skill(ch, SKILL_MOUNT, TRUE))
      return;
  } else {
    send_to_char("You know nothing about mounting.\r\n", ch);
    return;
  }

  if (!*arg)
  {
    send_to_char("Mount what?\r\n", ch);
    return;
  }

  // Check character is not already mounted
  if (MOUNTING(ch) || MOUNTING_OBJ(ch))
  {
    send_to_char("You're already mounted!\r\n", ch);
    return; 
  }

  bits = generic_find(arg, FIND_CHAR_ROOM | /* FIND_OBJ_ROOM |*/ 
                           FIND_OBJ_INV |  FIND_OBJ_EQUIP, ch, 
			   &found_char, &found_obj);

  // Character found 
  if (found_char != NULL)
  {
    // Check that the char is mountable
    if (!MOB_FLAGGED(found_char, MOB_MOUNTABLE))
    {
      send_to_char("You can't mount that!\r\n", ch);
      return;
    }	

    // Check if it's tamed
    if (!AFF_FLAGGED(found_char, AFF_BROKEN_IN))
    {
      send_to_char("This beastie's a bit too wild for you.\r\n", ch);
      act("$n hopelessly chases $N around, trying to mount $M.", 
                      FALSE, ch, 0,found_char, TO_ROOM);
      return;
    }	

    // Check that noone is riding it already
    if (MOUNTING(found_char))
    {
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
  if (found_obj != NULL)
  {
    // Check that the target item is mountable
    if (!OBJ_FLAGGED(found_obj, ITEM_RIDDEN))
    {
      send_to_char("You can't mount that!\r\n", ch);
      return;
    }

    // Check it is not already being ridden
    if (OBJ_RIDDEN(found_obj))
    {
      send_to_char("It is already being ridden.\r\n", ch);
      return;
    }	

    // Mount it
    sprintf(buf, "You mount %s.\r\n", found_obj->short_description);
    send_to_char(buf, ch);
    sprintf(buf, "%s mounts %s.", 
                    GET_NAME(ch),found_obj->short_description);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    MOUNTING_OBJ(ch) = found_obj;
    OBJ_RIDDEN(found_obj) = ch;		
    return;	
  }
	
  // Nothing found
  send_to_char("Nothing here by that name.\r\n", ch);
}

ACMD(do_dismount)
{
  struct char_data *mount;
  struct obj_data *obj_mount;

  if (IS_NPC(ch)) 
    return;

  // May as well make it a social too
  if (!MOUNTING(ch) && !MOUNTING_OBJ(ch))
  {
    send_to_char("You get off your high horse.\r\n", ch);
    act("$n gets off $s high horse.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  // Mobile dismount
  if (MOUNTING(ch))
  {
    // Get the mount
    mount = MOUNTING(ch);
    // Get a keyword
    one_argument(mount->player.name, arg);
    // If it is not visible ...
    if (mount != generic_find_char(ch, arg, FIND_CHAR_ROOM))
    {
      send_to_char("Oh dear, you seem to have lost your ride.\r\n", ch);
      send_to_char("... you fall off your imaginary mount.\r\n", ch);
      act("$n falls off $s imaginary mount.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    } else {
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
  if (MOUNTING_OBJ(ch))
  {
    // Get the object
    obj_mount = MOUNTING_OBJ(ch);
    // Check it's being carried or is in the room
    if (!((obj_mount != 
           find_obj_list(ch, obj_mount->name, ch->carrying)) || 
       (obj_mount != 
        find_obj_list(ch, obj_mount->name,world[ch->in_room].contents)) ||
       (obj_mount != generic_find_obj(ch, obj_mount->name, FIND_OBJ_EQUIP))))
    {
      send_to_char("Oh dear, you seem to have lost your ride.\r\n", ch);
      send_to_char("... you fall off your imaginary mount.\r\n", ch);
      act("$n falls off $s imaginary mount.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    } else {
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
