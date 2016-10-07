 /*
************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"

/*   external vars  */
extern FILE *player_fl;
extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern int level_exp[LVL_CHAMP + 1];

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct int_app_type int_app[36];
extern struct wis_app_type wis_app[36];
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int restrict;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;

/* for objects */
extern char *item_types[];
extern char *wear_bits[];
extern char *extra_bits[];
extern char *drinks[];

/* for rooms */
extern char *dirs[];
extern char *room_bits[];
extern char *exit_bits[];
extern char *sector_types[];

/* for chars */
extern char *spells[];
extern char *equipment_types[];
extern char *affected_bits[];
extern char *apply_types[];
extern char *pc_class_types[];
extern char *npc_class_types[];
extern char *action_bits[];
extern char *player_bits[];
extern char *preference_bits[];
extern char *extended_bits[];
extern char *position_types[];
extern char *connected_types[];

extern int same_world(struct char_data *ch,struct char_data *ch2);
extern int mag_manacost(struct char_data * ch, int spellnum); 

void john_in(struct char_data *ch);
void cassandra_in(struct char_data *ch);

/* this is a must! clock over one mud hour */
ACMD(do_tic)
{
  extern pulse;
  pulse=-1;
  send_to_char("Time moves forward one hour.\r\n",ch);
}

/* skillshow - display the spell/skill  */
ACMD(do_skillshow)
{
 extern char *spells[];
  extern struct spell_info_type spell_info[];
  extern spell_sort_info[];
  int i, sortpos, spellnum, mana;
  struct char_data *vict;
  one_argument(argument,arg);

  if (!*arg) {
    vict=ch;
  } else if(!(vict = get_char_vis(ch,arg,TRUE))) {
    send_to_char(NOPERSON,ch);
    return;
  }

  sprintf(buf, "Spell/Skill abilities for &B%s&n.\r\n",GET_NAME(vict));
  sprintf(buf, "%sPractice Sessions: &M%d&n.\r\n",buf,GET_PRACTICES(vict));

  sprintf(buf, "%s%-20s %-10s\r\n",buf,"Spell/Skill","Ability");
  strcpy(buf2, buf);

  for (sortpos = 1; sortpos < MAX_SKILLS; sortpos++) {
    i = spell_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if (GET_LEVEL(vict) >= spell_info[i].min_level[0]) {
      spellnum = find_skill_num(spells[i]);
      mana = mag_manacost(vict, spellnum);
      sprintf(buf, "%-20s %-10d %-3d ", spells[i], GET_SKILL(vict, i), mana);
      strcat(buf2, buf);
      if (spell_info[i].str!=0){
                sprintf(buf,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].str,CCNRM(ch,C_NRM));
                strcat(buf2, buf);
      }
      if (spell_info[i].intl!=0){
                sprintf(buf,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].intl,CCNRM(ch,C_NRM));
                strcat(buf2, buf);
      }
      if (spell_info[i].wis!=0){
                sprintf(buf,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].wis,CCNRM(ch,C_NRM));
                strcat(buf2, buf);
      }
      if (spell_info[i].dex!=0){
                sprintf(buf,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].dex,CCNRM(ch,C_NRM));
                strcat(buf2, buf);
      }
      if (spell_info[i].con!=0){
                sprintf(buf,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].con,CCNRM(ch,C_NRM));
                strcat(buf2, buf);
      }
      if (spell_info[i].cha!=0){
                sprintf(buf,"%sCha: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].cha,CCNRM(ch,C_NRM));
                strcat(buf2, buf);
      }
      sprintf(buf,"\r\n");
      strcat(buf2, buf);
    }
  }

  page_string(ch->desc, buf2, 1);
}

/* Vulcan Neck Pinch - basically just stuns the victim - Vader */
/* this was gunna be a skill but i didnt no if it fit :)       */
ACMD(do_pinch)
{
  struct char_data *vict;

  one_argument(argument,arg);

  if(!(vict = get_char_room_vis(ch,arg))) {
    if(FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Upon whom do you wish to perform the Vulcan neck pinch??\r\n",ch);
      return;
      }
    }

  if(!AWAKE(vict)) {
    send_to_char("It appears that your victim is already incapacitated...\r\n",ch);
    return;
    }

  if(vict == ch) {
    send_to_char("You reach up and perform the Vulcan neck pinch on yourself and pass out...\r\n",ch);
    act("$n calmly reaches up and performs the Vulcan neck pinch on $mself before passing out...",FALSE,ch,0,0,TO_ROOM);
    GET_POS(ch) = POS_STUNNED;
    return;
    }

  if(GET_LEVEL(vict) >= GET_LEVEL(ch)) {
    send_to_char("It is not logical to incapacitate your fellow Gods...\r\n",ch);
    return;
    }

  act("You skillfully perform the Vulcan neck pinch on $N who instantly falls to the ground.",FALSE,ch,0,vict,TO_CHAR);
  act("$n gently puts $s hand on your shoulder and Vulcan neck pinches you! You pass out instantly...",FALSE,ch,0,vict,TO_VICT);
  act("$n skillfully performs the Vulcan neck pinch on $N who falls to the ground, stunned.",FALSE,ch,0,vict,TO_NOTVICT);

  stop_fighting(vict);
  GET_POS(vict) = POS_STUNNED;
}

ACMD(do_cream)
{
  struct char_data *vict;
  struct affected_type af;
  one_argument(argument,arg);
  
  if (!(vict = get_char_vis(ch, arg, FALSE)))
  {
    send_to_char("Who is it that you wish to cream?\r\n", ch);
    return;
  }
  if (IS_NPC(vict))
  {
    send_to_char("You can't cream mobs!\r\n", ch);
    return;
  }

  if (GET_LEVEL(vict) > GET_LEVEL(ch))
  {
    send_to_char("You miss!\r\n", ch);
    sprintf(buf, "%s attempts to hit you with a cream pie, but misses completely\r\n", GET_NAME(ch));
    send_to_char(buf, vict);
    return;
  }

  sprintf(buf, "You hit %s in the face with a cream pie!\r\n", GET_NAME(vict));
  send_to_char(buf, ch);

  send_to_char("A cream pie falls from the heavens and hits you in the face!\r\nYick, you've been creamed!\r\nAn evil laugh echoes over the land.\r\n", vict);

  af.type = SPELL_BLINDNESS;
  af.location = APPLY_HITROLL;
  af.modifier = -4;
  af.duration = 1;
  af.bitvector = AFF_BLIND;
  affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);
  return;
}

ACMD(do_immort)
{
  struct char_data *vict;

  one_argument(argument,arg);

  if (!(vict = get_char_vis(ch, arg,FALSE))){
     send_to_char("I cannot seem to find that person!!!\r\n", ch);
     return;
  }

  if (IS_NPC(vict)){
	send_to_char("You whant to immort a monster!!!!!. YOU MUST BE BORED!\r\n", ch);
	return;
  }
  if (GET_LEVEL(vict)<LVL_CHAMP){
	send_to_char("They are not a CHAMPION!!.  Can't do it!\r\n", ch);
	return;
  }
  if (GET_LEVEL(vict)==LVL_ANGEL){
	send_to_char("They are already angel silly!\r\n", ch);
	return;
  }
  if (GET_LEVEL(vict)>LVL_ANGEL){
	send_to_char("Yer right get serious!!!\r\n", ch);
	return;
  }
  GET_LEVEL(vict) = LVL_ANGEL;
  send_to_char("Done...\r\n", ch);
  send_to_char("You are touched by the hand of god. You soul shivers for a second.\r\n", vict);
  send_to_char("You feel IMMORTAL!.\r\n", vict);
}

ACMD(do_deimmort)
{
  struct char_data *vict;

  one_argument(argument,arg);

  if (!(vict = get_char_vis(ch, arg,FALSE))){
     send_to_char("I cannot seem to find that person!!!\r\n", ch);
     return;
  }

  if (IS_NPC(vict)){
        send_to_char("DEMOTE A MONSTER!!!!.... Go find something more productive to do!\r\n", ch);
        return;
  }

  if (GET_LEVEL(vict)!=LVL_ANGEL){
	send_to_char("You can only demote ANGELS!!\r\n", ch);
	return;
  }
  
  GET_LEVEL(vict)=LVL_CHAMP;
  send_to_char("You have DEMOTED them!.\r\n", ch);
  send_to_char("You have been demoted!!!.  You become a fallen angel!\r\nServes you RIGHT!\r\n", vict);
}

ACMD(do_pkset)
{
  char buff[120];
  half_chop(argument, arg, buf);

  if (!*arg) {
     if (IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED))
     sprintf(arg,"on");
     else sprintf(arg,"off");
     sprintf(buff,"In this zone, Player Killing is %s.", arg);
     send_to_char(buff,ch);
     return;
  }
  if (!strcmp(arg, "on"))
     SET_BIT(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED);
  else if (!strcmp(arg, "off"))
     REMOVE_BIT(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED);
  sprintf(buff,"Player Killing in this zone is now %s.", arg);
  send_to_char(buff,ch);
}

ACMD(do_queston)
{
  struct char_data *vict;
  char buff[120];
  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Quest On for who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg,FALSE))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  SET_BIT(PRF_FLAGS(vict),PRF_QUEST);
  sprintf(buff,"%s is now in the quest.\r\n",vict->player.name);
  send_to_char(buff,ch);
}
     
ACMD(do_questoff)
{
  struct char_data *vict;
  char buff[120];
  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Quest Off for who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg,FALSE))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(vict),PRF_QUEST);
  sprintf(buff,"%s is no longer in the quest.\r\n",vict->player.name);
  send_to_char(buff,ch);
}
     
ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes.. but what?\r\n", ch);
  else {
    if (subcmd == SCMD_EMOTE)
      sprintf(buf, "$n %s", argument);
    else
      strcpy(buf, argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}


ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Send what to who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg,FALSE))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  send_to_char(buf, vict);
  send_to_char("\r\n", vict);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("Sent.\r\n", ch);
  else {
    sprintf(buf2, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
    send_to_char(buf2, ch);
  }
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
sh_int find_target_room(struct char_data * ch, char *rawroomstr)
{
  int tmp;
  sh_int location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if(!*roomstr) {
    send_to_char("You must supply a room number or name.\r\n",ch);
    return NOWHERE;
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return NOWHERE;
    }
  } else if ((target_mob = get_char_vis(ch, roomstr,TRUE)))
    location = target_mob->in_room;
  else if ((target_obj = get_obj_vis(ch, roomstr))) {
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("That object is not available.\r\n", ch);
      return NOWHERE;
    }
  } else {
    send_to_char("No such creature or object around.\r\n", ch);
    return NOWHERE;
  }

  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (GET_LEVEL(ch) < LVL_GRGOD) {
    if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	world[location].people && world[location].people->next_in_room) {
      send_to_char("There's a private conversation going on in that room.\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
	!House_can_enter(ch, world[location].number)) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return NOWHERE;
    }
  }
  return location;
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  int location, original_loc;


  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("You must supply a room number or a name.\r\n", ch);
    return;
  }

  if (!*command) {
    send_to_char("What do you want to do there?\r\n", ch);
    return;
  }

  if ((location = find_target_room(ch, buf)) < 0)
    return;

/* stops imms going to reception - Vader */
  if(world[location].number >= 500 && world[location].number <= 599 &&
     GET_LEVEL(ch) < LVL_GOD) {
  send_to_char("As you fly through time and space towards the reception area\r\n"
               "a black gloved hand grabs you and rips you back to where you came from...\r\n",ch);
    return;
    }


  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}

ACMD(do_goto)
{
  /* int virtual_loc; */
  sh_int location;
  struct char_data *target_mob;
  char name_mob[MAX_INPUT_LENGTH];
  extern int allowed_zone(struct char_data * ch,int flag);
  extern int allowed_room(struct char_data * ch,int flag);

 
  if (PRF_FLAGGED(ch, PRF_MORTALK) && GET_LEVEL(ch)<LVL_GOD){
	send_to_char("You cannot goto out of mortalk arena.  Use Recall!\r\n", ch);
	return;
  }
  if (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_CHAMP) 
  {
    one_argument (argument,name_mob);
    if(((target_mob = get_char_vis(ch,name_mob,FALSE)) == NULL)
      || (world[target_mob->in_room].zone == GOD_ROOMS_ZONE) 
      || (!same_world(target_mob,ch))) 
    {
      send_to_char("That person does not appear to be anywhere!",ch);
      return;
    }
    if(IS_NPC(target_mob)) 
    {
      send_to_char("You can only go to Player Characters!",ch);
      return;
    }
  }
 
 
  if ((location = find_target_room(ch, argument)) < 0)
    return;

/* stops imms going to reception - Vader */
  if(world[location].number >= 500 && world[location].number <= 599 &&
     GET_LEVEL(ch) < LVL_GOD) {
  send_to_char("As you fly through time and space towards the reception area\r\n"
               "a guardian spirit tells you in a nice kind voice, \"PISS OFF!\"\r\n",ch);
    return;
    }

/* stops imms going to clan halls - Hal*/

  if(world[location].number >= 103 && world[location].number <= 150 && GET_LEVEL(ch) < LVL_GOD)
  {
    send_to_char("As you fly through time and space towards the clan area\r\n"
               "a guardian spirit tells you in a nice kind voice,\"umm no sorry can't go there\"\r\n",ch);
    return;
  }

  if ((world[location].number >= 4500) && (world[location].number <= 4599) &&
      (GET_LEVEL(ch) < LVL_HELPER))
  {
    send_to_char("Cannot goto designated quest area.\r\n", ch);
    return;
  }

  if (!allowed_zone(ch,zone_table[world[location].zone].zflag))
    return;
  if (!allowed_room(ch,world[location].room_flags))
    return;

  if (POOFOUT(ch))
    sprintf(buf, "$n %s", POOFOUT(ch));
  else
    strcpy(buf, "$n disappears in a puff of smoke.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);
  if (!strcmp(ch->player.name, "John")) 
    john_in(ch);
  else
    if (!strcmp(ch->player.name, "Cassandra"))
      cassandra_in(ch);
    else
    {
      if (POOFIN(ch))
        sprintf(buf, "$n %s", POOFIN(ch));
      else
        strcpy(buf, "$n appears with an ear-splitting bang.");

     act(buf, TRUE, ch, 0, 0, TO_ROOM);
    }
  look_at_room(ch, 0);
}

void john_in(struct char_data *ch)
{
  int fp;
  int nread;

  fp = open("/primal/lib/text/john.poofin", O_RDONLY);
  if (fp == -1)
    strcpy(buf, "***********John Has Arrived*************\n\n");
  else
  {
    nread = read(fp, buf, MAX_STRING_LENGTH);
    buf[nread] = '\0';
    close(fp);
  }
 
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
}

void cassandra_in(struct char_data *ch)
{
  int fp;
  int nread;

  fp = open("/primal/lib/text/cassandra.poofin", O_RDONLY);
  if (fp == -1)
    strcpy(buf, "***********Cassandra Has Arrived*************\n\n");
  else
  {
    nread = read(fp, buf, MAX_STRING_LENGTH);
    buf[nread] = '\0';
    close(fp);
  }
 
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
}

ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to transfer?\r\n", ch);
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf,TRUE)))
      send_to_char(NOPERSON, ch);
    else if (victim == ch)
      send_to_char("That doesn't make much sense, does it?\r\n", ch);
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char("Go transfer someone your own size.\r\n", ch);
	return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, ch->in_room);
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char("I think not.\r\n", ch);
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i->character && i->character != ch) {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, ch->in_room);
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
      }
    send_to_char(OK, ch);
  }
}



ACMD(do_teleport)
{
  struct char_data *victim;
  sh_int target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char("Whom do you wish to teleport?\r\n", ch);
  else if (!(victim = get_char_vis(ch, buf,FALSE)))
    send_to_char(NOPERSON, ch);
  else if (victim == ch)
    send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("Maybe you shouldn't do that.\r\n", ch);
  else if (!*buf2)
    send_to_char("Where do you wish to send this person?\r\n", ch);
  else if ((target = find_target_room(ch, buf2)) >= 0) {
    send_to_char(OK, ch);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
  }
}



ACMD(do_vnum)
{
  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj"))) {
    send_to_char("Usage: vnum { obj | mob } <name>\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
      send_to_char("No mobiles by that name.\r\n", ch);

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(buf2, ch))
      send_to_char("No objects by that name.\r\n", ch);
}



void do_stat_room(struct char_data * ch)
{
  struct extra_descr_data *desc;
  struct room_data *rm = &world[ch->in_room];
  int i, found = 0;
  struct obj_data *j = 0;
  struct char_data *k = 0;

  sprintf(buf, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
	  CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], Type: %s\r\n",
	  rm->zone, CCGRN(ch, C_NRM), rm->number, CCNRM(ch, C_NRM), ch->in_room, buf2);
  send_to_char(buf, ch);

  sprintbit((long) rm->room_flags, room_bits, buf2);
  sprintf(buf, "SpecProc: %s, Flags: %s\r\n",
	  (rm->func == NULL) ? "None" : "Exists", buf2);
  send_to_char(buf, ch);

  send_to_char("Description:\r\n", ch);
  if (rm->description)
    send_to_char(rm->description, ch);
  else
    send_to_char("  None.\r\n", ch);

  if (rm->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  sprintf(buf, "Chars present:%s", CCYEL(ch, C_NRM));
  for (found = 0, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;
    sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
	    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (k->next_in_room)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);
  send_to_char(CCNRM(ch, C_NRM), ch);

  if (rm->contents) {
    sprintf(buf, "Contents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;
      sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (rm->dir_option[i]) {
      if (rm->dir_option[i]->to_room == NOWHERE)
	sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
      else
	sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
		world[rm->dir_option[i]->to_room].number, CCNRM(ch, C_NRM));
      sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
      sprintf(buf, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n ",
	      CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1, rm->dir_option[i]->key,
	   rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None",
	      buf2);
      send_to_char(buf, ch);
      if (rm->dir_option[i]->general_description)
	strcpy(buf, rm->dir_option[i]->general_description);
      else
	strcpy(buf, "  No exit description.\r\n");
      send_to_char(buf, ch);
    }
  }
}

void do_list_obj_values(struct obj_data *j, char * buf)
{
  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    sprintf(buf, "Color: [%d], Type: [%d], Hours: [%d]",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    sprintf(buf, "Spells: %d, %d, %d, %d", GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprintf(buf, "Spell: %d, Mana: %d", GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 1));
    break;
  case ITEM_FIREWEAPON:
  case ITEM_WEAPON:
    sprintf(buf, "Tohit: %d, Todam: %dd%d, Type: %d", GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_MISSILE:
    sprintf(buf, "Tohit: %d, Todam: %d, Type: %d", GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_ARMOR:
    sprintf(buf, "AC-apply: [%d]", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_TRAP:
    sprintf(buf, "Spell: %d, - Hitpoints: %d",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_CONTAINER:
    sprintf(buf, "Max-contains: %d, Locktype: %d, Corpse: %s",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 3) ? "Yes" : "No");
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
    sprintf(buf, "Max-contains: %d, Contains: %d, Poisoned: %s, Liquid: %s",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 3) ? "Yes" : "No", buf2);
    break;
  case ITEM_NOTE:
    sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    sprintf(buf, "Keytype: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_FOOD:
    sprintf(buf, "Makes full: %d, Poisoned: %d",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 3));
    break;
  default:
    sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  }
 
}

void do_stat_object(struct char_data * ch, struct obj_data * j)
{
  char lr_buf[9];
  int i, virtual, found;
  struct obj_data *j2;
  struct extra_descr_data *desc;

  virtual = GET_OBJ_VNUM(j);
  sprintf(buf, "Name: '%s%s%s', Aliases: %s\r\n", CCYEL(ch, C_NRM),
	  ((j->short_description) ? j->short_description : "<None>"),
	  CCNRM(ch, C_NRM), j->name);
  send_to_char(buf, ch);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  if (GET_OBJ_RNUM(j) >= 0)
    strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None"));
  else
    strcpy(buf2, "None");
  sprintf(buf, "VNum: [%s%5d%s], RNum: [%5d], Type: %s, SpecProc: %s\r\n",
   CCGRN(ch, C_NRM), virtual, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), buf1, buf2);
  send_to_char(buf, ch);
  sprintf(buf, "L-Des: %s\r\n", ((j->description) ? j->description : "None"));
  send_to_char(buf, ch);

  if (j->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = j->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  send_to_char("Can be worn on: ", ch);
  sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Set char bits : ", ch);
  sprintbit(j->obj_flags.bitvector, affected_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Extra flags   : ", ch);
  sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
  sprintf(lr_buf, " LR_%d", (int)GET_OBJ_LR(j));
  strcat(buf, lr_buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  sprintf(buf, "Weight: %d, Value: %d, Cost/day: %d, Timer: %d\r\n",
     GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j));
  send_to_char(buf, ch);

  strcpy(buf, "In room: ");
  if (j->in_room == NOWHERE)
    strcat(buf, "Nowhere");
  else {
    sprintf(buf2, "%d", world[j->in_room].number);
    strcat(buf, buf2);
  }
  strcat(buf, ", In object: ");
  strcat(buf, j->in_obj ? j->in_obj->short_description : "None");
  strcat(buf, ", Carried by: ");
  strcat(buf, j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  strcat(buf, ", Worn by: ");
  strcat(buf, j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  do_list_obj_values(j,buf);  
  send_to_char(buf, ch);

  strcpy(buf, "\r\nEquipment Status: ");
  if (!j->carried_by)
    strcat(buf, "None");
  else {
    found = FALSE;
    for (i = 0; i < NUM_WEARS; i++) {
      if (j->carried_by->equipment[i] == j) {
	sprinttype(i, equipment_types, buf2);
	strcat(buf, buf2);
 	found = TRUE;
      }
    }
    if (!found)
      strcat(buf, "Inventory");
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  if (j->contains) {
    sprintf(buf, "Contents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j2->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  found = 0;
  send_to_char("Affections:", ch);
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s %+d to %s", found++ ? "," : "",
	      j->affected[i].modifier, buf2);
      send_to_char(buf, ch);
    }
  if (!found)
    send_to_char(" None", ch);

  send_to_char("\r\n", ch);
}

void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  struct assisters_type *assisters;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;
  extern struct attack_hit_type attack_hit_text[];
  extern int thaco(struct char_data *);
  char buf3[MAX_INPUT_LENGTH], ORDERSTRING[][4]= {"???","STR","CON","DEX","INT","WIS","CHA"};
  int ETERNAL;
  struct char_file_u chdata;
  
  ETERNAL = (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_ETRNL9);
  switch (k->player.sex) {
  case SEX_NEUTRAL:
    strcpy(buf, "NEUTRAL-SEX");
    break;
  case SEX_MALE:
    strcpy(buf, "MALE");
    break;
  case SEX_FEMALE:
    strcpy(buf, "FEMALE");
    break;
  default:
    strcpy(buf, "ILLEGAL-SEX!!");
    break;
  }

  sprintf(buf2, " %s '%s'  IDNum: [%5ld], In room [%5d]",
	  (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	  GET_NAME(k), GET_IDNUM(k), world[k->in_room].number);

  if (!IS_NPC(k)) {
    if( GET_INVIS_TYPE(ch) == -2 )
      sprintf(buf3,", Invis to [%s%s%s]",CCYEL(ch, C_NRM),get_name_by_id(GET_INVIS_LEV(k)),CCNRM(ch,C_NRM));
    else if( GET_INVIS_TYPE(ch) == -1 )
      sprintf(buf3,", Invis to Lvl [%s%d%s]",CCYEL(ch,C_NRM),GET_INVIS_LEV(k),CCNRM(ch,C_NRM));
    else if( GET_INVIS_TYPE(ch) == 0 )
      sprintf(buf3,", Invis Lvl [%s%d%s]",CCYEL(ch,C_NRM),GET_INVIS_LEV(k),CCNRM(ch,C_NRM));
    else
      sprintf(buf3,", Invis to Lvls [%s%d-%d%s]",CCYEL(ch,
            C_NRM),GET_INVIS_LEV(k),GET_INVIS_TYPE(ch), CCNRM(ch,C_NRM));
      	
    strcat(buf2,buf3);
  }

  strcat(buf2,"\r\n");

  send_to_char(strcat(buf, buf2), ch);
  if (IS_MOB(k)) {
    sprintf(buf, "Alias: %s, VNum: [%5d], RNum: [%5d]\r\n",
	    k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    send_to_char(buf, ch);
  }
  sprintf(buf, "Title: %s\r\n", (k->player.title ? k->player.title : "<None>"));
  send_to_char(buf, ch);

  sprintf(buf, "L-Des: %s", (k->player.long_descr ? k->player.long_descr : "<None>\r\n"));
  send_to_char(buf, ch);

  if (IS_NPC(k)) {
    strcpy(buf, "Monster Class: ");
    sprinttype(k->player.class, npc_class_types, buf2);
  } else {
    strcpy(buf, "Class: ");
    sprinttype(k->player.class, pc_class_types, buf2);
  }
  strcat(buf, buf2);

  if (IS_NPC(k)) {
  sprintf(buf2, ", Lev: [%s%2d%s], XP: [%s%8d%s], Align: [%4d]\r\n",
	  CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	  CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
	  GET_ALIGNMENT(k));
  } else {
  sprintf(buf2, ", Lev: [%s%2d%s], XP: [%s%8d%s], XP_NL: [%s%8d%s], Align: [%4d]\r\n",
	  CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	  CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
          CCYEL(ch, C_NRM), level_exp[GET_LEVEL(k)]-GET_EXP(k), CCNRM(ch, C_NRM),
	  GET_ALIGNMENT(k));
  }
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
    if (load_char(GET_NAME(k), &chdata) != -1)
      strcpy(buf2, (char *) asctime(localtime(&chdata.last_logon)));
    else
      strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
      
    buf1[10] = buf2[10] = '\0';

    sprintf(buf, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
	    buf1, buf2, k->player.time.played / 3600,
	    ((k->player.time.played / 3600) % 60), age(k).year);
    send_to_char(buf, ch);

    sprintf(buf, "Hometown: [%d], Speaks: [%d/%d/%d], (STL[%d]/per[%d]/NSTL[%d])\r\n",
	 k->player.hometown, GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2),
	    GET_PRACTICES(k), int_app[GET_AFF_INT(k)].learn,
	    wis_app[GET_AFF_WIS(k)].bonus);
    send_to_char(buf, ch);
  }
  if (!IS_NPC(k))
  {
    sprintf(buf, "Str: %d/%d(%d/%d) Int: %d(%d) Wis: %d(%d) "
	  "Dex: %d(%d) Con: %d(%d) Cha: %d(%d)\r\n",
	  GET_AFF_STR(k), GET_AFF_ADD(k),GET_REAL_STR(k), GET_REAL_ADD(k),
	  GET_AFF_INT(k), GET_REAL_INT(k), 
	  GET_AFF_WIS(k), GET_REAL_WIS(k), 
	  GET_AFF_DEX(k), GET_REAL_DEX(k), 
	  GET_AFF_CON(k), GET_REAL_CON(k), 
	  GET_AFF_CHA(k), GET_REAL_CHA(k));
    send_to_char(buf, ch);
  }
  if (!ETERNAL) {
    sprintf(buf, "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
	  CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprintf(buf, "Coins: [%9d], Bank: [%9d] (Total: %d)\r\n",
	  GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));
    send_to_char(buf, ch);

    sprintf(buf, "AC: %d/10, Thaco: %d, Hitroll: %2d, Damroll: %2d, Saving %d/%d/%d/%d/%d\r\n",
	  GET_AC(k), thaco(k), k->points.hitroll, k->points.damroll, GET_SAVE(k, 0),
	  GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3), GET_SAVE(k, 4));
    send_to_char(buf, ch);
  }
  if (!IS_NPC(k))
  {
    sprintf(buf, "Order %s %s %s %s %s %s    Prac %d\r\n",
	     ORDERSTRING[k->player_specials->saved.stat_order[0]],
	     ORDERSTRING[k->player_specials->saved.stat_order[1]],
	     ORDERSTRING[k->player_specials->saved.stat_order[2]],
	     ORDERSTRING[k->player_specials->saved.stat_order[3]],
	     ORDERSTRING[k->player_specials->saved.stat_order[4]],
	     ORDERSTRING[k->player_specials->saved.stat_order[5]],
             GET_PRACTICES(k));
    send_to_char(buf,ch);
  }	     
  
  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "Pos: %s, Fighting: %s", buf2,
	  (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));

  if( MOUNTING(k) ) {
	sprintf(buf2, ", Mounted on: %s", (IS_NPC(k) ? "Yes" : GET_NAME(MOUNTING(k))) );
	strcat(buf, buf2);
  }
  if( MOUNTING_OBJ(k) ) {
	sprintf(buf2, ", Mounted on: %s", MOUNTING_OBJ(k)->short_description);
	strcat(buf, buf2);
  }

  if (IS_NPC(k)) {
    strcat(buf, ", Attack type: ");
    strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
  }
  if (k->desc) {
    sprinttype(k->desc->connected, connected_types, buf2);
    strcat(buf, ", Connected: ");
    strcat(buf, buf2);
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  strcpy(buf, "Default position: ");
  sprinttype((k->mob_specials.default_pos), position_types, buf2);
  strcat(buf, buf2);

  sprintf(buf2, ", Idle Timer (in tics) [%d]\r\n", k->char_specials.timer);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (IS_NPC(k)) {
    sprintbit(MOB_FLAGS(k), action_bits, buf2);
    sprintf(buf, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
  } else {
    sprintbit(PLR_FLAGS(k), player_bits, buf2);
    sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit(PRF_FLAGS(k), preference_bits, buf2);
    sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit(EXT_FLAGS(k), extended_bits, buf2);
    sprintf(buf, "EXT: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);  
  }

  if (IS_MOB(k)) {
    sprintf(buf, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
	    (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice);
    send_to_char(buf, ch);
  }
  sprintf(buf, "Carried: weight: %d, items: %d; ",
	  IS_CARRYING_W(k), IS_CARRYING_N(k));

  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
  sprintf(buf, "%sItems in: inventory: %d, ", buf, i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (k->equipment[i])
      i2++;
  sprintf(buf2, "eq: %d\r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  sprintf(buf, "Hunger: %d, Thirst: %d, Drunk: %d\r\n",
	  GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
  send_to_char(buf, ch);

  sprintf(buf, "Master is: %s, Followers are:",
	  ((k->master) ? GET_NAME(k->master) : "<none>"));

  for (fol = k->followers; fol; fol = fol->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (fol->next)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);

  sprintf(buf, "Autoassisting: %s, Autoassisters are:",
	  (AUTOASSIST(k) ? GET_NAME(AUTOASSIST(k)) : "<none>"));
  found=0;

  for (assisters=k->autoassisters; assisters; assisters=assisters->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(assisters->assister, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (assisters->next)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);

  /* DM - clan info */
  i = GET_CLAN_NUM(k);

  if (GET_LEVEL(ch) >= LVL_ANGEL)
  if ((i > 0) && (i < CLAN_NUM)) {
    sprintf(buf, "Clan: %s, Rank: %s\r\n", 
	clan_info[i].disp_name, clan_info[i].ranks[GET_CLAN_LEV(k)]);
    send_to_char(buf,ch);
  }

  /* Showing the bitvector */
  sprintbit(AFF_FLAGS(k), affected_bits, buf2);
  sprintf(buf, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  /* Routine to show what spells a char is affected by */
  if (k->affected) 
	{
    for (aff = k->affected; aff; aff = aff->next) 
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


ACMD(do_stat)
{
  struct char_data *victim = 0;
  struct obj_data *object = 0;
  struct char_file_u tmp_store;
  int ETERNAL;

  half_chop(argument, buf1, buf2);

  ETERNAL = (GET_LEVEL(ch) >= LVL_ETRNL1 && GET_LEVEL(ch) <= LVL_ETRNL9);
  if (!*buf1) {
    send_to_char("Stats on who or what?\r\n", ch);
    return;
  } else if(ETERNAL) {
    if((victim = get_char_room_vis(ch,buf1)))
      do_stat_character(ch,victim);
    else send_to_char("That person does not appear to be here.",ch);
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char("Stats on which mobile?\r\n", ch);
    else {
      if ((victim = get_char_vis(ch, buf2,TRUE)))
	do_stat_character(ch, victim);
      else
	send_to_char("No such mobile around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      if ((victim = get_player_vis(ch, buf2)))
	do_stat_character(ch, victim);
      else
	send_to_char("No such player around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1) {
	store_to_char(&tmp_store, victim);
	if (GET_LEVEL(victim) > LVL_OWNER)
	{
	  send_to_char("There is no such player.\r\n", ch);
	  free(victim);
	  return;
	}
	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char("Sorry, you can't do that.\r\n", ch);
	else
	  do_stat_character(ch, victim);
	free_char(victim);
      } else {
	send_to_char("There is no such player.\r\n", ch);
	free(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char("Stats on which object?\r\n", ch);
    else {
      if ((object = get_obj_vis(ch, buf2)))
	do_stat_object(ch, object);
      else
	send_to_char("No such object around.\r\n", ch);
    }
  } else {
    if ((victim = get_char_vis(ch, buf1,TRUE)))
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, buf1)))
      do_stat_object(ch, object);
    else
      send_to_char("Nothing around by that name.\r\n", ch);
  }
}


ACMD(do_shutdown)
{
  extern int circle_shutdown, circle_reboot;

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("If you want to shut something down, say so!\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    sprintf(buf, "(GC) Reboot by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    touch("../.fastboot");
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Shutting down for maintenance.\r\n");
    touch("../.killscript");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    log(buf);
    send_to_all("Shutting down for maintenance.\r\n");
    touch("../pause");
    circle_shutdown = 1;
  } else
    send_to_char("Unknown shutdown option.\r\n", ch);
  House_save_all();
}


void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping)
    send_to_char("You aren't snooping anyone.\r\n", ch);
  else {
    send_to_char("You stop snooping.\r\n", ch);
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}


ACMD(do_snoop)
{
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg,TRUE)))
    send_to_char("No such person around.\r\n", ch);
  else if (!victim->desc)
    send_to_char("There's no link.. nothing to snoop.\r\n", ch);
  else if (victim == ch)
    stop_snooping(ch);
  /* only IMPS can snoop players in private rooms BM 3/95 */
  else if (!(GET_LEVEL(ch)>LVL_IMPL) && IS_SET(world[victim->in_room].room_flags,ROOM_HOUSE))
    send_to_char("That player is in a private room, sorry!\n",ch);
  else if (victim->desc->snoop_by)
    send_to_char("Busy already. \r\n", ch);
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
      send_to_char("You can't.\r\n", ch);
      return;
    }
    send_to_char(OK, ch);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}



ACMD(do_switch)
{
  struct char_data *victim;
  SPECIAL(shop_keeper);
  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char("You're already switched.\r\n", ch);
  else if (!*arg)
    send_to_char("Switch with who?\r\n", ch);
  else if (!(victim = get_char_vis(ch, arg,TRUE)))
    send_to_char("No such character.\r\n", ch);
  else if (ch == victim)
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
  else if (victim->desc)
    send_to_char("You can't do that, the body is already in use!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(victim) == shop_keeper))
	send_to_char("Switching into Shopkeepers in NOT ALLOWED!\r\n", ch);
  else if ((GET_LEVEL(ch) <= LVL_ANGEL) && (GET_LEVEL(victim) >= 99))
  { // Stop them switching into tough ass mobs..
    send_to_char("Their mind is too strong. You fail. It hurts.\r\n", ch);
    GET_HIT(ch) = 1;
    GET_POS(ch) = POS_STUNNED;
  }
  else if ((GET_LEVEL(ch) <= LVL_ANGEL) && (GET_MOB_VNUM(victim) == 21))
  {
    send_to_char("The postmaster laughs as he removes your will to live.", ch);
    raw_kill(ch, victim);
    return;
  }
  else {
    send_to_char(OK, ch);

    sprintf(buf, "(GC) %s switched into %s.", GET_NAME(ch), GET_NAME(victim));
    mudlog(buf, BRF, LVL_GOD, TRUE);
    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char("You return to your original body.\r\n", ch);

    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}



ACMD(do_load)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: load { obj | mob } <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, ch->in_room);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    obj_to_room(obj, ch->in_room);
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}



ACMD(do_vstat)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf) {			/* argument supplied. destroy single object
				 * or char */
    if ((vict = get_char_room_vis(ch, buf))) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
	send_to_char("Fuuuuuuuuu!\r\n", ch);
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	mudlog(buf, BRF, LVL_GOD, TRUE);
	if (vict->desc) {
	  close_socket(vict->desc);
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("Nothing here by that name.\r\n", ch);
      return;
    }

    send_to_char(OK, ch);
  } else {			/* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

    for (vict = world[ch->in_room].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict) && !MOUNTING(vict))
	extract_char(vict);
    }

    for (obj = world[ch->in_room].contents; obj; obj = next_o) {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}



ACMD(do_advance)
{
  struct char_data *victim;
  char name[100], level[100];
  int newlevel;
  void do_start(struct char_data *ch);

  void gain_exp(struct char_data * ch, int gain);

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name,TRUE))) {
      send_to_char("That player is not here.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Advance who?\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("Maybe that's not such a great idea.\r\n", ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("NO!  Not on NPC's.\r\n", ch);
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char("That's not a level!\r\n", ch);
    return;
  }
  if (newlevel > LVL_OWNER) {
    sprintf(buf, "%d is the highest possible level.\r\n", LVL_OWNER);
    send_to_char(buf, ch);
    return;
  }
  if ((newlevel > GET_LEVEL(ch)) && (GET_IDNUM(ch) > 2)) {
    send_to_char("Yeah, right.\r\n", ch);
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char("Good one, they are already on that level.\r\n",ch);
    return;
  }
  /* DM_exp */ 
  if (newlevel < GET_LEVEL(victim)) {
    send_to_char("You have been demoted!!!. Serves you RIGHT!\r\n", victim);
    demote_level(victim, newlevel);
    GET_EXP(victim)=0;
    return;
  } else {
    act("$n makes some strange gestures.\r\n"
	"A strange feeling comes upon you,\r\n"
	"Like a giant hand, light comes down\r\n"
	"from above, grabbing your body, that\r\n"
	"begins to pulse with colored lights\r\n"
	"from inside.\r\n\r\n"
	"Your head seems to be filled with demons\r\n"
	"from another plane as your body dissolves\r\n"
	"to the elements of time and space itself.\r\n"
	"Suddenly a silent explosion of light\r\n"
	"snaps you back to reality.\r\n\r\n"
	"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(OK, ch);

  sprintf(buf, "(GC) %s has advanced %s to level %d (from %d)",
	  GET_NAME(ch), GET_NAME(victim), newlevel, GET_LEVEL(victim));
  log(buf);

/* DM_exp - perform the advance */
  gain_exp_regardless(victim, newlevel); 

  save_char(victim, NOWHERE);
}



ACMD(do_restore)
{
  struct char_data *vict;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to restore?\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf,TRUE)))
    send_to_char(NOPERSON, ch);
  else {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

    if ((GET_LEVEL(ch) >= LVL_GRGOD) && (GET_LEVEL(vict) >= LVL_ANGEL)) {
      for (i = 1; i <= MAX_SKILLS; i++)
	SET_SKILL(vict, i, 100);

      if (GET_LEVEL(vict) >= LVL_GRGOD) {
	vict->real_abils.str_add = 100;
	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
      }
      vict->aff_abils = vict->real_abils;
    }
    update_pos(vict);
    send_to_char(OK, ch);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}



void perform_immort_vis(struct char_data *ch)
{
  struct descriptor_data *d;

  if (GET_INVIS_LEV(ch) == 0 && !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)) {
    send_to_char("You are already fully visible.\r\n", ch);
    return;
  }

  sprintf(buf, "You feel privilidged as %s materialises nearby.\r\n",
	GET_NAME(ch) );

  for( d = descriptor_list; d; d = d->next ) 
    if (d->character)
	if( (d->character->in_room == ch->in_room) && (d->character != ch))
   	   if( !CAN_SEE(d->character, ch) )
		send_to_char(buf, d->character);

  GET_INVIS_LEV(ch) = 0;
  GET_INVIS_TYPE(ch) = INVIS_NORMAL;
  send_to_char("You are now fully visible.\r\n", ch);
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  if (IS_NPC(ch))
    return;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;

    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level) {

       // character specific invis?
       if( GET_INVIS_TYPE(ch) == INVIS_SPECIFIC ) {
	  if( GET_IDNUM(tch) == GET_INVIS_LEV(ch)) {
            if( GET_LEVEL(ch) > GET_LEVEL(tch) )
	       act("You blink and realise that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
	    else {
		send_to_char("You can't go invis to that person!\r\n", ch);
		GET_INVIS_TYPE(ch) = 0;
		GET_INVIS_LEV(ch) = 0;
		return;
	    }
	  }
	  	    
	}
	else
       // Standard invis?
       if( GET_INVIS_TYPE(ch) == INVIS_NORMAL )
          act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
       else {
          // Specific invis?
          if ( GET_INVIS_TYPE(ch) == INVIS_SINGLE && GET_LEVEL(tch) == level - 1 ) 
               act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
          else { 
             // They are in a range invis
             if ( GET_LEVEL(tch) >= level && GET_LEVEL(tch) <= GET_INVIS_TYPE(ch) ) {
                  act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
             }
	  }            	         
       }
    }
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
           act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,tch, TO_VICT);

  }

  if( GET_INVIS_TYPE(ch) == INVIS_SPECIFIC ) {
	sprintf(buf, "You are now invisible to %s.\r\n", 
		get_name_by_id(GET_INVIS_LEV(ch)) );
	send_to_char(buf, ch);
	return;
  }

  GET_INVIS_LEV(ch) = level;
  if (GET_INVIS_TYPE(ch) == INVIS_SINGLE) 
     sprintf(buf2, "s.");
  else if (GET_INVIS_TYPE(ch) == INVIS_NORMAL)
     sprintf(buf2, ".");
  else
    sprintf(buf2, " - %d.", GET_INVIS_TYPE(ch));

  sprintf(buf, "Your invisibility level is %d%s\r\n", level, buf2);
  send_to_char(buf, ch);
}
  

ACMD(do_invis)
{
  int level, toplevel;
  long pid;	// player id
  struct char_file_u tmp;
  struct char_data *tch;

  if (IS_NPC(ch)) {
    send_to_char("You can't do that!\r\n", ch);
    return;
  }

  two_arguments(argument, arg, buf1);
  
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else 
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else { 
    // invis to a person?
    if( !isdigit(arg[0]) ) {
	if( (pid = get_id_by_name(arg)) < 0 ) {
	  send_to_char("No such player exists.\r\n", ch);
	  return;
	}

	if( pid == GET_IDNUM(ch) ) {
	  send_to_char("You put your hands over your eyes and hide.\r\n",ch);
	  return;
	}
	// Load player
	CREATE(tch, struct char_data, 1);
	load_char(arg, &tmp);
	clear_char(tch);
	store_to_char(&tmp, tch);
	char_to_room(tch, 0);
	if( GET_LEVEL(ch) <= GET_LEVEL(tch) ) {
	    send_to_char("You can't go invisible to that person!\r\n",ch);
	    extract_char(tch);
	    return;
	}
	extract_char(tch);
	GET_INVIS_LEV(ch) = get_id_by_name(arg);
	GET_INVIS_TYPE(ch) = INVIS_SPECIFIC;
	perform_immort_invis(ch, GET_LEVEL(ch));
	return;
    }

    level = atoi(arg);

    if ( level > GET_LEVEL(ch)  )
       send_to_char("You can't go invisible above your own level.\r\n", ch);
    else if (level < 1)
       perform_immort_vis(ch);
    else {
      // If there is a second argument, evaluate it
      if (*buf1) { 
       // Are they specifying a range?
       if ( isdigit(buf1[0]) ) {
          toplevel = atoi(buf1);
	  if (toplevel <= level || toplevel >= GET_LEVEL(ch)) {
             send_to_char("The level range is invalid.\r\n", ch);
	     return;
          } // Valid range?
          else 
             GET_INVIS_TYPE(ch) = toplevel;
       } // Specified a range?
       else { /* Specified a particular invisibility level? */
          if( strcmp(buf1, "single") == 0 ) {
	     GET_INVIS_TYPE(ch) = INVIS_SINGLE;	
          } 
       }
      }
      else // No second argument
         GET_INVIS_TYPE(ch) = INVIS_NORMAL;

      perform_immort_invis(ch, level);
    } 
      
  }
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);

  if (!*argument)
    send_to_char("That must be a mistake...\r\n", ch);
  else {
    sprintf(buf, "%s\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next)
      if (!pt->connected && pt->character && pt->character != ch)
	send_to_char(buf, pt->character);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      send_to_char(buf, ch);
  }
}


ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
  case SCMD_POOFIN:
    msg = &(POOFIN(ch));
    break;
  case SCMD_POOFOUT:
    msg = &(POOFOUT(ch));
    break;
  default:
    return;
    break;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);


  if (!*argument) {
    *msg = NULL;
    switch (subcmd) {
      case SCMD_POOFIN:
        send_to_char("Poofin set to default.\r\n",ch);
        return;
        break;
      case SCMD_POOFOUT:
        send_to_char("Poofout set to default.\r\n",ch);
        return;
        break;
      default:
        return;
        break;
    }
  } else
    *msg = str_dup(argument);

  send_to_char(OK, ch);
}



ACMD(do_dc)
{
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char("Usage: DC <connection number> (type USERS for a list)\r\n", ch);
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char("No such connection.\r\n", ch);
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    send_to_char("Umm.. maybe that's not such a good idea...\r\n", ch);
    return;
  }
  close_socket(d);
  sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
  send_to_char(buf, ch);
  sprintf(buf, "(GC) Connection closed by %s.", GET_NAME(ch));
  log(buf);
}



ACMD(do_wizlock)
{
  int value;
  char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char("Invalid wizlock value.\r\n", ch);
      return;
    }
    restrict = value;
    when = "now";
  } else
    when = "currently";

  switch (restrict) {
  case 0:
    sprintf(buf, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    sprintf(buf, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    sprintf(buf, "Only level %d and above may enter the game %s.\r\n",
	    restrict, when);
    break;
  }
  send_to_char(buf, ch);
}


ACMD(do_date)
{
  char *tmstr;
  long mytime;
  int d, h, m;
  extern long boot_time;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    sprintf(buf, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
	    ((d == 1) ? "" : "s"), h, m);
  }

  send_to_char(buf, ch);
}



ACMD(do_last)
{
  struct char_file_u chdata;
  extern char *class_abbrevs[];

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("For whom do you wish to search?\r\n", ch);
    return;
  }
  if (load_char(arg, &chdata) < 0) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if (chdata.level > LVL_OWNER)
  {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if ((chdata.level > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL)) {
    send_to_char("You are not sufficiently godly for that!\r\n", ch);
    return;
  }
  sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
	  chdata.char_specials_saved.idnum, (int) chdata.level,
	  class_abbrevs[(int) chdata.class], chdata.name, chdata.host,
	  ctime(&chdata.last_logon));
  send_to_char(buf, ch);
}

ACMD(do_laston)
{
  struct char_file_u chdata;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("For whom do you wish to search?\r\n", ch);
    return;
  }
  if (load_char(arg, &chdata) < 0) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if (((chdata.level >= LVL_IS_GOD) && (GET_LEVEL(ch) != LVL_IMPL)) || 
((chdata.level >= LVL_ISNOT_GOD) && (GET_LEVEL(ch) < LVL_ISNOT_GOD)))
  {
    send_to_char("Sorry that player has a CLASSIFIED account!.\r\n", ch);
    return;
  }
  sprintf(buf, "%s was last on: %s\r\n",chdata.name,ctime(&chdata.last_logon));
  send_to_char(buf, ch);
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char("Whom do you wish to force do what?\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg,TRUE)))
      send_to_char(NOPERSON, ch);
    else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char("No, no, no!\r\n", ch);
    else {
      send_to_char(OK, ch);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced room %d to %s", GET_NAME(ch), world[ch->in_room].number, to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (i->connected || !(vict = i->character) || GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}



#define LVL_IMMNET LVL_ETRNL1

ACMD(do_wiznet)
{
  struct descriptor_data *d;
  char emote = FALSE;
  char any = FALSE;
  int level = LVL_GOD;
  int LVL = LVL_GOD;

  if(subcmd == SCMD_IMMNET) 
    level = LVL = LVL_IMMNET;
  if(subcmd == SCMD_ANGNET)
    level = LVL = LVL_ANGEL;

  if (IS_NPC(ch)) {
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);
  if(subcmd == SCMD_IMMNET) {
    // Artus -- Mute Immnets Too :o)
    if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_NOSHOUT)) {
      send_to_char("Thou shal not immnet.\r\n", ch);
      return;
    }
  }

  if (!*argument) {
    if(subcmd == SCMD_IMMNET) {
      send_to_char("Usage: immnet <text> | #<level> <text> | *<emotetext> |\r\n"
                   "       immnet @<level> *<emotetext> | imm @\r\n",ch);
    } else {
    send_to_char("Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n"
		 "       wiznet @<level> *<emotetext> | wiz @\r\n", ch);
    }
    return;
  }
  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), LVL);
      if (level > GET_LEVEL(ch)) {
	send_to_char("You can't wizline above your own level.\r\n", ch);
	return;
      }
    } else if (emote)
      argument++;
    break;
  case '@':
    for (d = descriptor_list; d; d = d->next) {
      if (!d->connected && GET_LEVEL(d->character) >= LVL &&
	  ((!PRF_FLAGGED(d->character, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
          (subcmd == SCMD_IMMNET && !PRF_FLAGGED(d->character,PRF_NOIMMNET))) &&
	  (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL)) {
	if (!any) {
  	  sprintf(buf1, "Gods online:\r\n");
	  any = TRUE;
	}
	sprintf(buf1, "%s  %s", buf1, GET_NAME(d->character));
	if (PLR_FLAGGED(d->character, PLR_WRITING))
	  sprintf(buf1, "%s (Writing)\r\n", buf1);
	else if (PLR_FLAGGED(d->character, PLR_MAILING))
	  sprintf(buf1, "%s (Writing mail)\r\n", buf1);
	else
	  sprintf(buf1, "%s\r\n", buf1);

      }
    }
    any = FALSE;
    for (d = descriptor_list; d; d = d->next) {
      if (!d->connected && GET_LEVEL(d->character) >= LVL &&
	  ((PRF_FLAGGED(d->character, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
          (subcmd == SCMD_IMMNET && PRF_FLAGGED(d->character,PRF_NOIMMNET))) &&
	  CAN_SEE(ch, d->character)) {
	if (!any) {
  	  sprintf(buf1, "%sGods offline:\r\n", buf1);
	  any = TRUE;
	}
	sprintf(buf1, "%s  %s\r\n", buf1, GET_NAME(d->character));
      }
    }
    send_to_char(buf1, ch);
    return;
    break;
  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if ((PRF_FLAGGED(ch, PRF_NOWIZ) && subcmd != SCMD_IMMNET) || 
      (subcmd == SCMD_IMMNET && PRF_FLAGGED(ch, PRF_NOIMMNET))) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }
  if (level > LVL) {
    if(subcmd == SCMD_IMMNET) {
      sprintf(buf1, "%s> (%d) %s%s\r\n", GET_NAME(ch), level,
              emote ? "<--- " : "", argument);
      sprintf(buf2, "Someone> (%d) %s%s\r\n", level, emote ? "<--- " : "",
              argument);
    } else if(subcmd == SCMD_ANGNET) {
      sprintf(buf1, "(%s:%d) %s%s\r\n", GET_NAME(ch), level,
              emote ? "<--- " : "", argument);
      sprintf(buf2, "(Someone:%d) %s%s\r\n", level, emote ? "<--- " : "",
              argument);
    } else {
      sprintf(buf1, "%s: <%d> %s%s\r\n", GET_NAME(ch), level,
 	      emote ? "<--- " : "", argument);
      sprintf(buf2, "Someone: <%d> %s%s\r\n", level, emote ? "<--- " : "",
	      argument);
      }
  } else {
    if(subcmd == SCMD_IMMNET) {
      sprintf(buf1, "%s> %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
              argument);
      sprintf(buf2, "Someone> %s%s\r\n", emote ? "<--- " : "", argument);
    } else if(subcmd == SCMD_ANGNET) {
      sprintf(buf1, "(%s) %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
      argument);
      sprintf(buf2, "(Someone) %s%s\r\n", emote ? "<--- " : "", argument);
    } else {
      sprintf(buf1, "%s: %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
  	      argument);
      sprintf(buf2, "Someone: %s%s\r\n", emote ? "<--- " : "", argument);
      }
  }

  for (d = descriptor_list; d; d = d->next) {
    if ((!d->connected) && (GET_LEVEL(d->character) >= level) &&
	((!PRF_FLAGGED(d->character, PRF_NOWIZ) && subcmd != SCMD_IMMNET) ||
        (subcmd == SCMD_IMMNET && !PRF_FLAGGED(d->character,PRF_NOIMMNET))) &&	
	(!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      if(subcmd == SCMD_IMMNET)
        send_to_char(CCBMAG(d->character, C_NRM), d->character);
      else if(subcmd == SCMD_ANGNET)
        send_to_char(CCBBLU(d->character, C_NRM), d->character);
      else
        send_to_char(CCBCYN(d->character, C_NRM), d->character);
      if (CAN_SEE(d->character, ch))
	send_to_char(buf1, d->character);
      else
	send_to_char(buf2, d->character);
      send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
}



ACMD(do_zreset)
{
  void reset_zone(int zone);

  int i, j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("You must specify a zone.\r\n", ch);
    return;
  }
  if (*arg == '*') {
    for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i);
    send_to_char("Reset world.\r\n", ch);
    return;
  } else if (*arg == '.')
    i = world[ch->in_room].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i >= 0 && i <= top_of_zone_table) {
    reset_zone(i);
    sprintf(buf, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number,
	    zone_table[i].name);
    send_to_char(buf, ch);
    sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
  } else
    send_to_char("Invalid zone number.\r\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  struct char_data *vict;
  long result;
  void roll_real_abils(struct char_data *ch);

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Yes, but for whom?!?\r\n", ch);
  else if (!(vict = get_char_vis(ch, arg,TRUE)))
    send_to_char("There is no such player.\r\n", ch);
  else if (IS_NPC(vict))
    send_to_char("You can't do that to a mob!\r\n", ch);
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    send_to_char("Hmmm...you'd better not.\r\n", ch);
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      if (vict->player_specials->saved.stat_order[0])
      { 
	send_to_char("Rerolled...\r\n", ch);
        roll_real_abils(vict);
        sprintf(buf, "(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
        log(buf);
        sprintf(buf, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	      GET_REAL_STR(vict), GET_REAL_ADD(vict), GET_REAL_INT(vict), GET_REAL_WIS(vict),
	      GET_REAL_DEX(vict), GET_REAL_CON(vict), GET_REAL_CHA(vict));
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf,"Can't re-roll stats for %s, old format player",GET_NAME(vict));
        send_to_char(buf,ch);
      }
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
	send_to_char("Your victim is not flagged.\r\n", ch);
	return;
      }
      REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
      send_to_char("Pardoned.\r\n", ch);
      send_to_char("You have been pardoned by the Gods!\r\n", vict);
      sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Your victim is already pretty cold.\r\n", ch);
	return;
      }
      SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
      send_to_char("Frozen.\r\n", ch);
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
	   GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	send_to_char(buf, ch);
	return;
      }
      sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
      send_to_char("Thawed.\r\n", ch);
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected) {
	while (vict->affected)
	  affect_remove(vict, vict->affected);
	send_to_char("There is a brief flash of light!\r\n"
		     "You feel slightly different.\r\n", vict);
	send_to_char("All spells removed.\r\n", ch);
      } else {
	send_to_char("Your victim does not have any affections!\r\n", ch);
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd passed to do_wizutil (act.wizard.c)");
      break;
    }
    save_char(vict, NOWHERE);
  }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, int zone)
{
  extern const char *zone_flagbits[];
  char bufstr[80];
  sprintbit(zone_table[zone].zflag, (char **)zone_flagbits, bufstr);
  sprintf(bufptr, "%s&B%3d&n %-20.20s &gAge&n: &y%3d&n; &gReset&n: &y%3d&n &b(&r%1d&b)&n; &gTop&n: &c%5d&n &gFlags&n: %s\r\n", bufptr, zone_table[zone].number, zone_table[zone].name,
	  zone_table[zone].age, zone_table[zone].lifespan,
	  zone_table[zone].reset_mode, zone_table[zone].top,bufstr);
}


ACMD(do_show)
{
  struct char_file_u vbuf;
  int i, j, k, l, con;
  char self = 0;
  struct char_data *vict;
  struct obj_data *obj;
  char field[40], value[40], birth[80];
  extern char *class_abbrevs[];
  extern char *genders[];
  extern int buf_switches, buf_largecount, buf_overflows;
  void show_shops(struct char_data * ch, char *value);

  struct show_struct {
    char *cmd;
    unsigned char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_ETRNL6 },			/* 1 */
    { "player",		LVL_GOD },
    { "rent",		LVL_GOD },
    { "stats",		LVL_ETRNL8 },
    { "errors",		LVL_IMPL },			/* 5 */
    { "death",		LVL_ANGEL },
    { "godrooms",	LVL_ANGEL },
    { "shops",		LVL_ETRNL7 },
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	sprintf(buf, "%s%-15s%s", buf, fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (!strcmp(value, "."))
    self = 1;
  buf[0] = '\0';
  switch (l) {
  case 1:			/* zone */
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, world[ch->in_room].zone);
    else if (*value && is_number(value)) {
      for (j = atoi(value), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++);
      if (i <= top_of_zone_table)
	print_zone_to_buf(buf, i);
      else {
	send_to_char("That is not a valid zone.\r\n", ch);
	return;
      }
    } else
      for (i = 0; i <= top_of_zone_table; i++)
	print_zone_to_buf(buf, i);
    send_to_char(buf, ch);
    break;
  case 2:			/* player */
    if (load_char(value, &vbuf) < 0) {
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
    sprintf(buf, "Player: %-12s (%s) [%2d %s]\r\n", vbuf.name,
      genders[(int) vbuf.sex], vbuf.level, class_abbrevs[(int) vbuf.class]);
    sprintf(buf,
	 "%sAu: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
	    buf, vbuf.points.gold, vbuf.points.bank_gold, vbuf.points.exp,
	    vbuf.char_specials_saved.alignment,
	    vbuf.player_specials_saved.spells_to_learn);
//    strcpy(birth, ctime(&vbuf.birth));
    sprintf(buf,
	    "%sStarted: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
	    buf, ctime(&vbuf.birth), ctime(&vbuf.last_logon), (int) (vbuf.played / 3600),
	    (int) (vbuf.played / 60 % 60));
    send_to_char(buf, ch);
    break;
  case 3:
    Crash_listrent(ch, value);
    break;
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    sprintf(buf, "&gCurrent stats&n:\r\n");
    sprintf(buf, "%s  &c%5d&n players in game  &C%5d&n connected\r\n", buf, i, con);
    sprintf(buf, "%s  &y%5d&n registered\r\n", buf, top_of_p_table + 1);
    sprintf(buf, "%s  &B%5d&n mobiles          &b%5d&n prototypes\r\n",
	    buf, j, top_of_mobt + 1);
    sprintf(buf, "%s  &B%5d&n objects          &b%5d&n prototypes\r\n",
	    buf, k, top_of_objt + 1);
    sprintf(buf, "%s  &B%5d&n rooms            &b%5d&n zones\r\n",
	    buf, top_of_world + 1, top_of_zone_table + 1);
    sprintf(buf, "%s  &r%5d&n large bufs\r\n", buf, buf_largecount);
    sprintf(buf, "%s  &b%5d&n buf switches     &R%5d&n overflows\r\n", buf,
	    buf_switches, buf_overflows);
    send_to_char(buf, ch);
    break;
  case 5:
    strcpy(buf, "Errant Rooms\r\n------------\r\n");
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
	if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0)
	  sprintf(buf, "%s%2d: [%5d] %s\r\n", buf, ++k, world[i].number,
		  world[i].name);
    send_to_char(buf, ch);
    break;
  case 6:
    strcpy(buf, "Death Traps\r\n-----------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH))
	sprintf(buf, "%s%2d: [%5d] %s\r\n", buf, ++j,
		world[i].number, world[i].name);
    send_to_char(buf, ch);
    break;
  case 7:
    strcpy(buf, "Godrooms\r\n--------------------------\r\n");
    for (i = 0, j = 0; i < top_of_world; i++)
      if (world[i].zone == GOD_ROOMS_ZONE)
	sprintf(buf, "%s%2d: [%5d] %s\r\n", buf, j++, world[i].number,
		world[i].name);
    send_to_char(buf, ch);
    break;
  case 8:
    show_shops(ch, value);
    break;
  default:
    send_to_char("Sorry, I don't understand that.\r\n", ch);
    break;
  }
}


#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_COMMANDS 60

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

struct set_struct {
  char *cmd;
  unsigned char level;
  char pcnpc;
  char type;
  char *help;
  char *range;
} fields[] = {
   { "brief",		LVL_GOD, 	PC, 	BINARY, "Brief Mode", "\n"		          }, /* 0 */
   { "invstart", 	LVL_GOD, 	PC, 	BINARY, "Enter game wizinvis","\n"  		  }, /* 1 */
   { "title",		LVL_GOD, 	PC, 	MISC,   "Player's title","\n"                     },
   { "nosummon", 	LVL_GRGOD, 	PC, 	BINARY, "Summonable","\n"                         },
   { "maxhit",		LVL_GRGOD, 	BOTH, 	NUMBER, "MAX Hit Points","(1 - 5000)"             },
   { "maxmana", 	LVL_GRGOD, 	BOTH, 	NUMBER, "MAX Mana Points","(1 - 5000)"            }, /* 5 */
   { "maxmove", 	LVL_GRGOD, 	BOTH, 	NUMBER, "MAX Move Points","(1 - 5000)"  	  },
   { "hit", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Current Hit Points","(-9 - MaxHit)"  	  },
   { "mana",		LVL_GRGOD, 	BOTH, 	NUMBER, "Current Mana Points","(0 - MaxMana)" 	  },
   { "move",		LVL_GRGOD, 	BOTH, 	NUMBER, "Current Move Points","(0 - MaxMove)" 	  },
   { "align",		LVL_GOD, 	BOTH, 	NUMBER, "Alignment","(-1000 - 1000)"	  	  },  /* 10 */
   { "str",		LVL_GRGOD, 	BOTH, 	NUMBER, "STAT: Strength","< GRGOD: (3 - 21), >= GRGOD (3-25)" },
   { "stradd",		LVL_GRGOD, 	BOTH, 	NUMBER, "STAT: Strength Addition","(0 - 100), Sets base str to 18" },
   { "int", 		LVL_GRGOD, 	BOTH, 	NUMBER, "STAT: Intelligence","< GRGOD: (3 - 21), >= GRGOD (3-25)"  },
   { "wis", 		LVL_GRGOD, 	BOTH, 	NUMBER, "STAT: Wisdom","< GRGOD: (3 - 21), >= GRGOD (3-25)"        },
   { "dex", 		LVL_GRGOD, 	BOTH, 	NUMBER, "STAT: Dexterity","< GRGOD: (3 - 21), >= GRGOD (3-25)"  },  /* 15 */
   { "con", 		LVL_GRGOD, 	BOTH, 	NUMBER, "STAT: Consitution","< GRGOD: (3 - 21), >= GRGOD (3-25)"   },
   { "sex", 		LVL_GRGOD, 	BOTH, 	MISC,   "Sex (Gender)","Male/Female/Neutral"    	  },
   { "ac", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Armor Class", "(-200 - 200)"	    	  	  },
   { "gold",		LVL_GOD, 	BOTH, 	NUMBER, "Gold on hand","(0 - 100000000)"        	  },
   { "bank",		LVL_GOD, 	PC, 	NUMBER, "Gold in bank","(0 - 100000000)"              	  },  /* 20 */
   { "exp", 		LVL_GRGOD, 	BOTH, 	NUMBER, "Experience Points","(0 - (Exp for level-1))" 	  },
   { "hitroll", 	LVL_GRGOD, 	BOTH, 	NUMBER, "Hitroll","(-20 - 20)" 	                          },
   { "damroll", 	LVL_GRGOD, 	BOTH, 	NUMBER, "Damroll","(-20 - 200)"                           },
   { "invis",		LVL_IMPL, 	PC, 	NUMBER, "Invis Level","(0 - Level Char)"                  },
   { "nohassle", 	LVL_GRGOD, 	PC, 	BINARY, "No Hassle (aggressive mobs)","\n"                },  /* 25 */
   { "frozen",		LVL_FREEZE, 	PC, 	BINARY, "Frozen","\n"					  },
   { "practices", 	LVL_GRGOD, 	PC, 	NUMBER, "No. of prac sessions","(0 - 100)"                },
   { "lessons", 	LVL_GRGOD, 	PC, 	NUMBER, "No. of prac sessions","(0 - 100)"		  },
   { "drunk",		LVL_GRGOD, 	BOTH, 	MISC,   "Drunk level","(Off, 0 - 24)"    		  },
   { "hunger",		LVL_GRGOD, 	BOTH, 	MISC,   "Hunger Level","(Off, 0 - 24)"	  		  },    /* 30 */
   { "thirst",		LVL_GRGOD, 	BOTH, 	MISC,	"Thirst Level","(Off, 0 - 24)"			  },
   { "killer",		LVL_GOD, 	PC, 	BINARY, "Killer flag","\n"				  },
   { "thief",		LVL_GOD, 	PC, 	BINARY, "Thief flag", "\n"				  },
   { "level",		LVL_IMPL, 	BOTH, 	NUMBER, "Level","(0 - 200)"				  },
   { "room",		LVL_IMPL, 	BOTH, 	NUMBER, "Room char is in",  "(Valid room number)"	  },  /* 35 */
   { "roomflag", 	LVL_GRGOD, 	PC, 	BINARY, "Room Flags","\n"				  },
   { "siteok",		LVL_GRGOD, 	PC, 	BINARY, "Player site-cleared","\n"			  },
   { "deleted", 	LVL_IMPL, 	PC, 	BINARY, "Player deleted (Space re-usable)","\n"		  },
   { "class",		LVL_GRGOD, 	BOTH, 	MISC, 	"Class","Valid Race"				  },
   { "nowizlist", 	LVL_GOD, 	PC, 	BINARY, "Player shouldn't be on Wizlist","\n"             },  /* 40 */
   { "quest",		LVL_GOD, 	PC, 	BINARY, "Player on Quest", "\n"				  },
   { "loadroom", 	LVL_GRGOD, 	PC, 	MISC,   "Player uses nonstandard loadroom","(On, Off, RNum)"	 },
   { "color",		LVL_GOD, 	PC, 	BINARY, "Color Level","On (complete), Off (off)"	  },
   { "idnum",		LVL_IMPL, 	PC, 	NUMBER, "Player IdNumber (doesn't work)","\n"		  },
   { "passwd",		LVL_IMPL, 	PC, 	MISC,	"Player Password (Not operational)","\n"	  },    /* 45 */
   { "nodelete", 	LVL_GOD, 	PC, 	BINARY, "Player Shouldn't be deleted","\n"		  },
   { "cha",		LVL_GRGOD, 	BOTH, 	NUMBER, "STAT: Charisma","< GRGOD (3-21), >= GRGOD (3-25)"},
   { "order",		LVL_IMPL, 	PC, 	NUMBER, "Stat Order (Looks a bit old)","1-MagicUser 2-Cleric 3-Warrior 4-Thief" },
   { "timer",           LVL_IMPL,       PC,     NUMBER, "Timer for update","None Available ATM"		  }, 
   { "holylight",       LVL_GRGOD,      PC,     BINARY, "Holylight","\n"				  },  /* 50 */
   { "infection",       LVL_GRGOD,      PC,     MISC,   "Infection","Werewolf/Vampire/None"		  },
   { "tag",             LVL_GOD,        PC,     BINARY, "Player playing TAG","\n"			  },
   { "palign",          LVL_GOD,        PC,     BINARY, "Display Alignment in prompt","\n"		  },
   { "noignore",        LVL_GOD,        PC,     BINARY, "Player cant use ignore command","\n"		  },
   { "leader",          LVL_GOD,        PC,     MISC,	"Clan Leader","(ON/OFF must be clan member)"      }, /* 55 */
   { "fix",             LVL_IMPL,       PC,     BINARY, "According to Hal: Fixes a bug","\n"		  },
   { "clan",            LVL_GOD,        PC,     MISC,   "Clan","(OFF, Clan Name)"			  },
   { "autogold",	LVL_GOD,	PC,   	BINARY, "Autoloot gold from corpses","\n"			  },
   { "autoloot",	LVL_GOD,	PC,	BINARY, "Autoloot eq from corpses","\n"		  	  },
   { "autoassist",	LVL_GOD,	PC,	MISC,	"The char that vict is autoassisting","(Off)"			  },   /* 60 */	
   { "autoassisters",	LVL_GOD,	BOTH,	MISC,	"The characters assisting vict", "(Off)"	  },	
   { "autosplit",	LVL_GOD,	BOTH,	MISC,	"Autosplit gold in groups", "(Off)"	  },	
   { "\n", 0, BOTH, MISC, "\n","\n" }
};

ACMD(do_set)
{
  int i, l , cnum = 0 ;
  struct char_data *vict;
  struct char_data *cbuf;
  struct char_file_u tmp_store;
  char charbuf [MAX_INPUT_LENGTH];
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],val_arg[MAX_INPUT_LENGTH];
  int on = 0, off = 0, value = 0;
  char is_file = 0, is_mob = 0, is_player = 0;
  int player_i;
  int parse_class(char *arg);

  half_chop(argument, name, buf);
  if (!str_cmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob")) {
    is_mob = 1;
    half_chop(buf, name, buf);
  }
  half_chop(buf, field, buf);
  strcpy(val_arg, buf);
  
  if (!*name || !*field) {
    send_to_char("Usage: set <victim> <field> <value>\r\n", ch);
    return;
  }
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name))) {
	send_to_char("There is no such player.\r\n", ch);
	return;
      }
    } else {
      if (!(vict = get_char_vis(ch, name,TRUE))) {
	send_to_char("There is no such creature.\r\n", ch);
	return;
      }
    }
  } else if (is_file) {
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if ((player_i = load_char(name, &tmp_store)) > -1) {
      store_to_char(&tmp_store, cbuf);
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch)) {
	free_char(cbuf);
	send_to_char("Sorry, you can't do that.\r\n", ch);
	return;
      }
      vict = cbuf;
    } else {
      free(cbuf);
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
  }
  if (GET_LEVEL(ch) <= LVL_GRIMPL) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char("Maybe that's not such a great idea...\r\n", ch);
      return;
    }
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (IS_NPC(vict) && (!fields[l].pcnpc && NPC)) {
    send_to_char("You can't do that to a beast!\r\n", ch);
    return;
  } else if (!IS_NPC(vict) && (!fields[l].pcnpc && PC)) {
    send_to_char("That can only be done to a beast!\r\n", ch);
    return;
  }
  if (fields[l].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char("Value must be on or off.\r\n", ch);
      return;
    }
  } else if (fields[l].type == NUMBER) {
    value = atoi(val_arg);
  }
  strcpy(buf, "Okay.");
  switch (l) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 2:
    set_title(vict, val_arg);
    sprintf(buf, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    on = !on;			/* so output will be correct */
    break;
  case 4:
    vict->points.max_hit = RANGE(1, 10000);
    affect_total(vict);
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 10000);
    affect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 10000);
    affect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    affect_total(vict);
    break;
  case 8:
    vict->points.mana = RANGE(0, vict->points.max_mana);
    affect_total(vict);
    break;
  case 9:
    vict->points.move = RANGE(0, vict->points.max_move);
    affect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-5000, 5000);
    affect_total(vict);
    break;
  case 11:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.str = value;
    vict->real_abils.str_add = 0;
    affect_total(vict);
    break;
  case 12:
    vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
    break;
  case 13:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    if (!str_cmp(val_arg, "male"))
      vict->player.sex = SEX_MALE;
    else if (!str_cmp(val_arg, "female"))
      vict->player.sex = SEX_FEMALE;
    else if (!str_cmp(val_arg, "neutral"))
      vict->player.sex = SEX_NEUTRAL;
    else {
      send_to_char("Must be 'male', 'female', or 'neutral'.\r\n", ch);
      return;
    }
    break;
  case 18:
    vict->points.armor = RANGE(-200, 200);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 21:
    if (GET_LEVEL(vict) >= LVL_CHAMP) {
      sprintf(buf,"%s is level %d, What are they going to do with exp?\r\n",GET_NAME(vict),GET_LEVEL(vict));
      send_to_char(buf,ch);
      return; 
    }
    
    vict->points.exp = RANGE(0, level_exp[GET_LEVEL(vict)]-1);
    break;
  case 22:
    vict->points.hitroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 23:
    vict->points.damroll = RANGE(-20, 200);
    affect_total(vict);
    break;
  case 24:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char("You aren't godly enough for that!\r\n", ch);
      return;
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25:
    if (GET_LEVEL(ch) < LVL_GRGOD && ch != vict) {
      send_to_char("You aren't godly enough for that!\r\n", ch);
      return;
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict) {
      send_to_char("Better not -- could be a long winter!\r\n", ch);
      return;
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
  case 28:
    GET_PRACTICES(vict) = RANGE(0, 100);
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off")) {
      GET_COND(vict, (l - 29)) = (char) -1;
      sprintf(buf, "%s's %s now off.", GET_NAME(vict), fields[l].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (l - 29)) = (char) value;
      sprintf(buf, "%s's %s set to %d.", GET_NAME(vict), fields[l].cmd,
	      value);
    } else {
      send_to_char("Must be 'off' or a value from 0 to 24.\r\n", ch);
      return;
    }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    if (( value > GET_LEVEL(ch) || value > LVL_OWNER) && (GET_IDNUM(ch) > 2)) { 
      send_to_char("You can't do that.\r\n", ch);
      return;
    } 
    RANGE(0, LVL_OWNER);
    vict->player.level = (byte) value;
    break;
  case 35:
    if ((i = real_room(value)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return;
    }
    char_from_room(vict);
    char_to_room(vict, i);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = parse_class(val_arg)) == CLASS_UNDEFINED)
      send_to_char("That is not a class, type 'race'.\r\n", ch);
    else {
      GET_CLASS(vict) = i;
      send_to_char(OK, ch);
    }
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "on"))
      SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
    else if (!str_cmp(val_arg, "off"))
      REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
    else {
      if (real_room(i = atoi(val_arg)) > -1) {
	GET_LOADROOM(vict) = i;
	sprintf(buf, "%s will enter at %d.", GET_NAME(vict),
		GET_LOADROOM(vict));
      } else
	sprintf(buf, "That room does not exist!");
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
    break;
  case 44:
    if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
      return;
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (!is_file)
      return;
    if (GET_IDNUM(ch) > 1) {
      send_to_char("Please don't use this command, yet.\r\n", ch);
      return;
    }
    if (GET_LEVEL(vict) >= LVL_GRGOD) {
      send_to_char("You cannot change that.\r\n", ch);
      return;
    }
    strncpy(tmp_store.pwd, CRYPT(val_arg, tmp_store.name), MAX_PWD_LENGTH);
    tmp_store.pwd[MAX_PWD_LENGTH] = '\0';
    sprintf(buf, "Password changed to '%s'.", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 21);
    vict->real_abils.cha = value;
    affect_total(vict);
    break;
  case 48:
    switch (value)
    {
      case 1:
	vict->player_specials->saved.stat_order[0]=4;
	vict->player_specials->saved.stat_order[1]=5;
	vict->player_specials->saved.stat_order[2]=3;
	vict->player_specials->saved.stat_order[3]=1;
	vict->player_specials->saved.stat_order[4]=2;
	vict->player_specials->saved.stat_order[5]=6;
        break;
      case 2:
	vict->player_specials->saved.stat_order[0]=5;
	vict->player_specials->saved.stat_order[1]=4;
	vict->player_specials->saved.stat_order[2]=1;
	vict->player_specials->saved.stat_order[3]=3;
	vict->player_specials->saved.stat_order[4]=2;
	vict->player_specials->saved.stat_order[5]=6;
	break;
      case 3:      
	vict->player_specials->saved.stat_order[0]=1;
	vict->player_specials->saved.stat_order[1]=3;
	vict->player_specials->saved.stat_order[2]=2;
	vict->player_specials->saved.stat_order[3]=5;
	vict->player_specials->saved.stat_order[4]=4;
	vict->player_specials->saved.stat_order[5]=6;
        break;
      case 4:
        vict->player_specials->saved.stat_order[0]=3;
	vict->player_specials->saved.stat_order[1]=1;
	vict->player_specials->saved.stat_order[2]=2;
	vict->player_specials->saved.stat_order[3]=4;
	vict->player_specials->saved.stat_order[4]=5;
	vict->player_specials->saved.stat_order[5]=6;
        break;
      default:
	sprintf(buf, "1-MagicUser 2-Cleric 3-Warrior 4-Thief\r\n");
	send_to_char(buf,ch);
	return;
        break;
    }
    break;
  case 49:
      vict->char_specials.timer=value;
    break;
  case 50:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_HOLYLIGHT);
    break;
  case 51:
      if(!str_cmp(val_arg,"werewolf")) {
        REMOVE_BIT(PRF_FLAGS(vict),PRF_VAMPIRE);
        SET_BIT(PRF_FLAGS(vict),PRF_WOLF);
      } else if(!str_cmp(val_arg,"vampire")) {
        REMOVE_BIT(PRF_FLAGS(vict),PRF_WOLF);
        SET_BIT(PRF_FLAGS(vict),PRF_VAMPIRE);
      } else if(!str_cmp(val_arg,"none")) {
        REMOVE_BIT(PRF_FLAGS(vict),PRF_WOLF | PRF_VAMPIRE);
      } else {
        send_to_char("Must be 'werewolf', 'vampire', or 'none'.\r\n",ch);
        return;
        }
      break;

  case 52:
    SET_OR_REMOVE(PRF_FLAGS(vict),PRF_TAG);
    break;
  case 53:
    SET_OR_REMOVE(PRF_FLAGS(vict),PRF_DISPALIGN);
    break;
  case 54:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOIGNORE );
    if (!str_cmp(val_arg, "on") )
    {
      GET_IGN1(vict) = 0 ;
      GET_IGN2(vict) = 0 ;
      GET_IGN3(vict) = 0 ;
      GET_IGN_LEVEL(vict) = 0;
      GET_IGN_NUM(vict) = 0 ;
    }    
    break;
  case 55:
    if ( str_cmp ( val_arg , "on" ) == 0 )
    {
     if ( GET_CLAN_NUM(vict) < 1 )
     {
      sprintf ( charbuf, "%s is not in a clan.\n\r", GET_NAME(vict) );
      send_to_char ( charbuf  , ch ) ;
     }
     else 
     {
      sprintf ( charbuf, "Leader ON for %s.\n\r", GET_NAME(vict)); 
      send_to_char ( charbuf  , ch );
	        
      if ( !EXT_FLAGGED(vict, EXT_LEADER) )
       SET_BIT(EXT_FLAGS(vict) , EXT_LEADER );		   
     }
    }
   
    if ( str_cmp ( val_arg , "off" ) == 0 )
    { 
     if ( EXT_FLAGGED(vict, EXT_LEADER) )
       REMOVE_BIT(EXT_FLAGS(vict) , EXT_LEADER );
	  
     sprintf ( charbuf, "Leader OFF for %s.\n\r", GET_NAME(vict));
     send_to_char ( charbuf  , ch ) ;
    }
    break ;
  case 56:
    REMOVE_BIT(PRF_FLAGS(vict), PRF_FIX);
    break;
  case 57:
    if ( str_cmp ( val_arg , "off" ) == 0 )
    {    
      if ( GET_CLAN_NUM(vict) < 1 )
       sprintf (buf , "%s is already clanless." , GET_NAME(vict) );
      else
      {
       sprintf (buf , "%s is now clanless." , GET_NAME(vict) );
       if ( vict != ch )
       {
        sprintf ( charbuf , "You have been banished from the %s.\n\r",
                                      get_clan_disp(vict) ) ;
        send_to_char ( charbuf , vict );
       }     
	 
       GET_CLAN_NUM (vict) = 0;
       GET_CLAN_LEV (vict) = 0 ;
	 
       REMOVE_BIT(EXT_FLAGS(vict) , EXT_CLAN ) ;
	 
       if ( EXT_FLAGGED(vict, EXT_LEADER) )
	 REMOVE_BIT(EXT_FLAGS(vict) , EXT_LEADER ) ;
		
       if ( EXT_FLAGGED(vict, EXT_SUBLEADER) )
	 REMOVE_BIT(EXT_FLAGS(vict) , EXT_SUBLEADER);

	}
     }
     else
     {
      for( i = 1 ; i < CLAN_NUM ; i++ )	 {
        if (!strn_cmp(val_arg, get_clan_name(i), 3))
         cnum = i ;
     }
	 
     if ( cnum == 0 )
      sprintf(buf, "That is not a clan.");
     else
     {   
      GET_CLAN_NUM(vict) = cnum ;   
      SET_BIT (EXT_FLAGS(vict) , EXT_CLAN );
      sprintf (buf, "%s is now a member of the %s.", GET_NAME(vict), 
                            get_clan_disp(vict));
      if ( vict != ch )
      { 
       sprintf (charbuf, "You are now a member of the %s.\n\r", 
                                 get_clan_disp(vict));
       send_to_char ( charbuf  , vict ) ;
      }
     }    
    }   
  case 58:
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_AUTOGOLD);
    break;
  case 59:
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_AUTOLOOT);
    break;
  case 60:
    if (is_file) {
      send_to_char("Autoassist isn't saved to file.\r\n",ch);
      return;
    }
    if (!str_cmp(val_arg,"off")) {
      if (AUTOASSIST(vict)) {
        sprintf(buf, "%s is no longer autoassisting %s.", 
		GET_NAME(vict),GET_NAME(AUTOASSIST(vict)));
        stop_assisting(vict);
      } else {
        sprintf(buf,"And who is %s autoassisting?.",GET_NAME(vict));
        send_to_char(buf,ch);
        return;
      } 
    } else {
      send_to_char("Options: OFF\r\n",ch);
      return;
    }
    break;
  case 61:
    if (is_file) {
      send_to_char("Autoassist isn't saved to file.\r\n",ch);
      return;
    }
    if (!str_cmp(val_arg,"off")) {
      if (vict->autoassisters) { 
        sprintf(buf, "%s is no longer being autoassisted.", GET_NAME(vict));
        stop_assisters(vict);
      } else {
        sprintf(buf,"And who autoassisting %s?.",GET_NAME(vict));
        send_to_char(buf,ch);
        return;
      } 
    } else {
      send_to_char("Options: OFF\r\n",ch);
      return;
    }
    break;
  case 62:
    SET_OR_REMOVE(EXT_FLAGS(vict), EXT_AUTOSPLIT);
    break;
  default:
    sprintf(buf, "Can't set that!");
    break;
  }

  if (fields[l].type == BINARY) {
    sprintf(buf, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on),
	    GET_NAME(vict));
    CAP(buf);
  } else if (fields[l].type == NUMBER) {
    sprintf(buf, "%s's %s set to %d.\r\n", GET_NAME(vict),
	    fields[l].cmd, value);
  } else
    strcat(buf, "\r\n");
  send_to_char(CAP(buf), ch);

  if (!is_file && !IS_NPC(vict))
    save_char(vict, NOWHERE);

  if (is_file) {
    char_to_store(vict, &tmp_store);
    fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
    fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
    free_char(cbuf);
    send_to_char("Saved in file.\r\n", ch);
  }
}

/* DM - Set Help, print out the help
      - TODO: Sort the struct first, generate it to file at boot time 
		and make it just char * */ 

ACMD(do_sethelp)
{
  int i,j;
  char buf3[MAX_INPUT_LENGTH];

  sprintf(buf,"COMMAND:        HELP:                        RANGE:\r\n");
  strcat(buf, "-----------------------------------------------------------------\r\n");

  for (i=0; *(fields[i].cmd) != '\n'; i++) {

    if (GET_LEVEL(ch) < fields[i].level)
      continue;
   
    sprintf(buf2,"%s",fields[i].cmd);

    for (j=1; j < (17-strlen(fields[i].cmd)); j++) 
      strcat(buf2," ");

    sprintf(buf3,"%s",fields[i].help);
 
    strcat(buf2,buf3);

    for (j=1; j < (40-strlen(fields[i].help)); j++)
      strcat(buf2," ");

    if (*(fields[i].range) != '\n')
      sprintf(buf3,"%s\r\n",fields[i].range);
    else
      if (fields[i].type == BINARY)
        sprintf(buf3,"(ON | OFF)\r\n");
      else
        sprintf(buf3,"\r\n");

    strcat(buf2,buf3);
  
    strcat(buf,buf2);
  }
 
  page_string(ch->desc,buf,0); 
}

static char *logtypes[] = {
"off", "brief", "normal", "complete", "\n"};

ACMD(do_syslog)
{
  int tp;

  one_argument(argument, arg);

  if (!*arg) {
    tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
    sprintf(buf, "Your syslog is currently %s.\r\n", logtypes[tp]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char("Usage: syslog { Off | Brief | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
  SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

  sprintf(buf, "Your syslog is now %s.\r\n", logtypes[tp]);
  send_to_char(buf, ch);
}

