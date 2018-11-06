/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
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
#include "screen.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"

/*   external vars  */
extern struct zone_data *zone_table;
extern int world_start_room[NUM_WORLDS];
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct spell_info_type spell_info[];
extern int guild_info[][3];

#define ROOM_ALIEN_ECHO		27150

/* extern functions */
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_manacost(struct char_data * ch, int spellnum);
int find_first_step(struct char_data *ch, room_rnum src, room_rnum target);
struct char_data *get_player_by_id(long idnum);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
ACMD(do_flee);
ACMD(do_gen_comm);
int check_for_event(int event, zone_rnum zone);
int get_burgle_area(room_rnum room);
int has_stats_for_prac(struct char_data *ch, int skillnum, bool show);
long get_burgle_room_type(long lArea);
void handle_fireball(struct char_data *ch);

/* local functions */
void create_suit(struct char_data *ch);
void sort_spells(void);
int compare_spells(const void *x, const void *y);
int compare_spells_levela(const void *x, const void *y);
int compare_spells_leveld(const void *x, const void *y);
const char *how_good(int percent);
void list_skills(struct char_data * ch);
SPECIAL(thrower);
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
void npc_steal(struct char_data * ch, struct char_data * victim);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(room_trap);
SPECIAL(burgle_area_occupant);

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

int spell_sort_info[NUM_SORT_TYPES][NUM_CLASSES][MAX_SKILLS + 1];

void to_upper(char *s)
{
  char *start = s;
  while( *s ) 
  {
    *s = toupper(*s);
    s++;
  }
  s = start;
}

int compare_spells_levela(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;
 
  if (spell_info[a].templevel == spell_info[b].templevel)
    return strcmp(spell_info[a].name, spell_info[b].name);
  else
    return (spell_info[a].templevel > spell_info[b].templevel);
}

int compare_spells_leveld(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;


  if (spell_info[a].templevel == spell_info[b].templevel)
    return strcmp(spell_info[a].name, spell_info[b].name);
  else
    return (spell_info[a].templevel < spell_info[b].templevel);
}

int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}

void sort_spells(void)
{
  int sort_index, class_index, spell_index;

  /* initialize array, avoiding reserved. */
  for (sort_index = 0; sort_index < NUM_SORT_TYPES; sort_index++)
    for (class_index = 0; class_index < NUM_CLASSES; class_index++)
      for (spell_index = 1; spell_index <= MAX_SKILLS; spell_index++)
        spell_sort_info[sort_index][class_index][spell_index] = spell_index;

  // For sort_type = SORT_ALPHA:
  // 	 only sort the first class_position and ignore the rest
  // So for spell_sort_info[SORT_ALPHA][x][y], x must = 0

  qsort(&spell_sort_info[0][0][1], MAX_SKILLS, sizeof(int), compare_spells);

  // Sort the rest
  for (sort_index = 1; sort_index < NUM_SORT_TYPES; sort_index++) {
    for (class_index = 0; class_index < NUM_CLASSES; class_index++) {
      for (spell_index = 1; spell_index <= MAX_SKILLS; spell_index++) {
        spell_info[spell_index].templevel = 
		spell_info[spell_index].min_level[class_index]; 
      }
      switch (sort_index) {
        case (SORT_ASCENDING):
          sprintf(buf,"Sorting spells level ascending for %s.",CLASS_NAME(class_index));
          basic_mud_log(buf);
          qsort(&spell_sort_info[sort_index][class_index][1], MAX_SKILLS, sizeof(int), 
		compare_spells_levela);
        break;

        case (SORT_DESCENDING):
          sprintf(buf,"Sorting spells level descending for %s.",CLASS_NAME(class_index));
          basic_mud_log(buf);
          qsort(&spell_sort_info[sort_index][class_index][1], MAX_SKILLS, sizeof(int), 
		compare_spells_leveld);
        break;

        default:
        break;
      }
    }
  }  
 /* Debug crap
  for (sort_index = 0; sort_index < NUM_SORT_TYPES; sort_index++) {
    sprintf(buf,"Sort Type %d:",sort_index);
    basic_mud_log(buf);
    for (class_index = 0; class_index < NUM_CLASSES; class_index++) {
      sprintf(buf,"%s = {",CLASS_NAME(class_index));
      basic_mud_log(buf);
      for (spell_index = 0; spell_index < MAX_SKILLS; spell_index++) {
        sprintf(buf2,"%d - %s, ",spell_info[spell_sort_info[sort_index][class_index][spell_index]].min_level[class_index],spell_info[spell_sort_info[sort_index][class_index][spell_index]].name);
        strcat(buf,buf2); 
      }
      strcat(buf,"}");
      basic_mud_log(buf);
    }
  }      
 */
}

const char *how_good(int percent)
{
  if (percent < 0)
    return "&[&r (error)&]  ";
  if (percent == 0)
    return "&[&r (unknown)  &]";
  if (percent <= 10)
    return " (awful)    ";
  if (percent <= 20)
    return " (bad)      ";
  if (percent <= 40)
    return " (poor)     ";
  if (percent <= 55)
    return " (average)  ";
  if (percent <= 70)
    return " (fair)     ";
  if (percent <= 80)
    return " (good)     ";
  if (percent <= 85)
    return " (very good)";

  return "&[&g (superb)&]   ";
}

const char *prac_types[] = {
  "spells",
  "skills",
  "spells and skills"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

/* actual prac_params are in class.c */
extern int prac_params[4][NUM_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

void list_skills(struct char_data * ch)
{
  int i, sortpos, mana, class_index = GET_CLASS(ch);
 
  if (!GET_PRACTICES(ch))
    strcpy(buf, "You have no practice sessions remaining.\r\n");
  else
    sprintf(buf, "You have %d practice session%s remaining.\r\n",
            GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));
 
  sprintf(buf + strlen(buf), "You know of the following %s:\r\n", SPLSKL(ch));
 
  strcpy(buf2, buf);
 
  for (sortpos = 1; sortpos <= MAX_SKILLS; sortpos++) {
    // spell_sort_info is listed alphabetically hence use [0][0][sortpos]
    i = spell_sort_info[0][0 /*class_index*/ ][sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) {
 
/*      sprintf(buf, "%-20s %s\r\n", spell_info[i].name, how_good(GET_SKILL(ch, i))); */
                    /* The above, ^ should always be safe to do. */
 
      mana = mag_manacost(ch, i);
      sprintf(buf, "%-21s %s %-3d ", spell_info[i].name, how_good(GET_SKILL(ch, i)), mana);
      strncat(buf2, buf, strlen(buf));
 
      /* Display the stat requirements */
      if (spell_info[i].str[class_index]!=0){
        if (GET_REAL_STR(ch) >= spell_info[i].str[class_index])
          sprintf(buf,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].str[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sStr: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].str[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].intl[class_index]!=0){
        if (GET_REAL_INT(ch) >= spell_info[i].intl[class_index])
          sprintf(buf,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].intl[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sInt: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].intl[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].wis[class_index]!=0){
        if (GET_REAL_WIS(ch) >= spell_info[i].wis[class_index])
          sprintf(buf,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].wis[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sWis: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].wis[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].dex[class_index]!=0){
        if (GET_REAL_DEX(ch) >= spell_info[i].dex[class_index])
          sprintf(buf,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].dex[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sDex: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].dex[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      if (spell_info[i].con[class_index]!=0){
        if (GET_REAL_CON(ch) >= spell_info[i].con[class_index])
          sprintf(buf,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].con[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sCon: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].con[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }               
      if (spell_info[i].cha[class_index]!=0){
        if (GET_REAL_CHA(ch) >= spell_info[i].cha[class_index])
          sprintf(buf,"%sCha: %s%2d%s ",CCGRN(ch,C_NRM),CCCYN(ch,C_NRM),spell_info[i].cha[class_index],CCNRM(ch,C_NRM));
        else
          sprintf(buf,"%sCha: %s%2d%s ",CCGRN(ch,C_NRM),CCRED(ch,C_NRM),spell_info[i].cha[class_index],CCNRM(ch,C_NRM));
 
        strncat(buf2, buf, strlen(buf));
      }
 
      sprintf(buf,"\r\n");
      strncat(buf2, buf, strlen(buf));
    }
  }
 
  page_string(ch->desc, buf2, 1);
}

/* Mobs with this proc will attack anyone 
 * that is not sneaking and not invisible
 * and will also look around suspiciously
 * if a character is in the room.
 *
 * They will also behave according to what
 * burgled type room they're in
 */
SPECIAL(burgle_area_occupant)
{
  struct char_data *mob = (struct char_data *)me;

  if (cmd || FIGHTING(mob))
	return FALSE;

  long lBurgleFlag = world[mob->in_room].burgle_flags;

  if (lBurgleFlag == 0)	// Not in a burgled area
	return FALSE;
  
  // Firstly, time dictates behaviour, if it's in a home
  // and it's past midnight, it's bed time
  if (lBurgleFlag == ROOM_HOME && time_info.hours >= 0 && time_info.hours < 6)
  {
    if (AWAKE(mob))
    { 
      act("$n goes to sleep.", FALSE, mob, 0, 0, TO_ROOM);
      GET_POS(mob) = POS_SLEEPING;
    }
  }

  // If it's a home and it's after 6, household wakes up
  if (lBurgleFlag == ROOM_HOME && time_info.hours >= 6)
  {
     if (!AWAKE(mob))
     {
       act("$n yawns and wakes up.", FALSE, mob, 0, 0, TO_ROOM);
       GET_POS(mob) = POS_STANDING;
     }
  }

  // If it's a warehouse and it's past midnight, go home
  if (lBurgleFlag == ROOM_WAREHOUSE && time_info.hours >= 0 && time_info.hours < 6)
  {
    act("$n heads home.", FALSE, mob, 0, 0, TO_ROOM);
    extract_char(mob);
    return TRUE;	// Bet this crashes
  }

  // If it's a shop and it's past midnight, hide self, and wait for players
  if (lBurgleFlag == ROOM_SHOP && time_info.hours >= 0 && time_info.hours < 6)
  {
     if (!IS_SET(AFF_FLAGS(mob), AFF_HIDE))
     { 
       act("The shop closes and $n leaves.", FALSE, mob, 0, 0, TO_ROOM);
       SET_BIT(AFF_FLAGS(mob), AFF_HIDE);
     }
  }
  
  // If it's a shop and it's after 6, shopkeeper returns (unhides)
  if (lBurgleFlag == ROOM_SHOP && time_info.hours >= 6)
  {
    if (IS_SET(AFF_FLAGS(mob), AFF_HIDE))
    {
      act("The shop opens and $n returns.", FALSE, mob, 0, 0, TO_ROOM);
      REMOVE_BIT(AFF_FLAGS(mob), AFF_HIDE);
    }
  }

  if (!AWAKE(mob))
  	return TRUE;	

  // Mob is awake and ready to kill some pc's!
  for (struct char_data *i = world[mob->in_room].people; i; i = i->next)
  {
    if (IS_NPC(i))
      continue;
    
    if (mob->in_room != i->in_room)
	continue;

    if (!CAN_SEE(mob, i))
      continue;
 
    if (IS_SET(AFF_FLAGS(i), AFF_SNEAK) && lBurgleFlag != ROOM_SHOP)
	continue;

    // Can see character
    act("$n says, 'Halt! Who goes there? .. Intruder!'", FALSE, mob, 0, 0, TO_ROOM);
    send_to_char("Oh oh .. you have been spotted.\r\n", i);
    hit(mob, i, TYPE_UNDEFINED);     
    return TRUE;
  }

  return TRUE;
  
}

SPECIAL(guild)
{

  const room_vnum guild_rooms[NUM_CLASSES] = {
    1135,         // Mage
    1129,         // Cleric
    1109,         // Thief
    1117,         // Warrior
    1237,         // Druid
    1239,         // Priest
    1240,         // NightBlade
    1241,         // BattleMage
    1238,         // Spellsword
    1242,         // Paladin
    1243         // Master
  };
  int skill_num, percent;

  if (!(ch) || (cmd < 1)) // Sanity.
    return (0);

  if (IS_NPC(ch) || !CMD_IS("practice"))
    return (0);

  skip_spaces(&argument);

  if (GET_ROOM_VNUM(ch->in_room) != guild_rooms[(int)GET_CLASS(ch)])
  {
    send_to_char("You must train in the guild of your profession.\r\n", ch);
    return (1); 
  }
  
  if (!*argument)
  {
    list_skills(ch);
    return (1);
  }
  if (GET_PRACTICES(ch) <= 0)
  {
    send_to_char("You do not seem to be able to practice now.\r\n", ch);
    return (1);
  }

  skill_num = find_skill_num(argument);

  if (skill_num < 1 ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) 
  {
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch)) 
    {
      sprintf(buf, "Skill_num: %d, Min level: %d\r\n", skill_num, (skill_num > 0) ? spell_info[skill_num].min_level[(int)GET_CLASS(ch)] : -1);
      send_to_char(buf, ch);
    }
#endif
    sprintf(buf, "You do not know of that %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return (1);
  }
  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You are already learned in that area.\r\n", ch);
    return (1);
  }

  // DM: Check the restrictions
  if (!has_stats_for_prac(ch, skill_num, TRUE)) {
    return (1);
  }
		  
  send_to_char("You practice for a while...\r\n", ch);
  GET_PRACTICES(ch)--;

  // DM: A bit hacky - basically we want one prac to get between 22 and 25 %
  // which will give a total after 3 pracs of 66 - 75%.
  // If the player has between 70-75% let it gain to 80%... Probably change
  // later if required.
  percent = GET_SKILL(ch, skill_num);
  percent += number(MINGAIN(ch), MAXGAIN(ch));
  //percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));
  if (GET_SKILL(ch, skill_num) >= 70 && GET_SKILL(ch, skill_num) < 75) {
    SET_SKILL(ch, skill_num, 80); 
  } else {
    SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));
  }

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);

  return (1);
}


#if 0 // Artus> Unused: Dump.
SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;
  
  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (0);

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    send_to_char("You are awarded for outstanding performance.\r\n", ch);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return (1);
}
#endif

#if 0 // Artus> Unused: Mayor.
SPECIAL(mayor)
{
  const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  switch (path[index]) {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
    do_gen_door(ch, "gate", 0, SCMD_OPEN);
    break;

  case 'C':
    do_gen_door(ch, "gate", 0, SCMD_CLOSE);
    do_gen_door(ch, "gate", 0, SCMD_LOCK);
    break;

  case '.':
    move = FALSE;
    break;

  }

  index++;
  return (FALSE);
}
#endif

/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data * ch, struct char_data * victim)
{
  int gold;

  if (IS_NPC(victim) || !LR_FAIL(ch, LVL_IMMORT))
    return;
  if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0))
  {
    act("You discover that $n has $s hands in your wallet.",
	FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (int)((GET_GOLD(victim) * number(1, 10)) / 100);
    if (gold > 0)
    {
      GET_GOLD(ch) += gold;
      GET_GOLD(victim) -= gold;
    }
  }
}

// Artus> Used lots.
SPECIAL(snake)
{
  if ((cmd) || (GET_POS(ch) != POS_FIGHTING))
    return (FALSE);
  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0))
  {
    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
    return (TRUE);
  }
  return (FALSE);
}

// New trainer - DM
SPECIAL(trainer)
{
  struct char_data *trainer = (struct char_data *) me;
  char *stat_name;
  int number = 1, i = 0, cost = 0, stat;
        
  if (!(ch) || IS_NPC(ch) || (!CMD_IS("train") && !CMD_IS("list")))
    return (FALSE);

  if (!CAN_LEVEL(ch))
  {
    act("$n&r tells you, 'There is nothing more I can do for you.'", 
	 FALSE, trainer, 0, ch, TO_VICT);
    return TRUE;
  }

  if (CMD_IS("list"))
  {
    sprintf(buf, "&1Training options:\r\n"
                 "-----------------&n\r\n"
                 "&gint&n - &c%d&n stat points\r\n"
                 "&gwis&n - &c%d&n stat points\r\n"
                 "&gstr&n - &c%d&n stat points\r\n"
                 "&gcon&n - &c%d&n stat points\r\n"
                 "&gdex&n - &c%d&n stat points\r\n"
                 "&gcha&n - &c%d&n stat points\r\n"
                 "1 &ghit&n point - &c%d&n experience points\r\n"
                 "1 &gmana&n point - &c%d&n experience points\r\n"
                 "1 &gmove&n point - &c%d&n experience points\r\n\r\n"
                 "Your allowed limits:\r\n"
                 "%sint&n - %d\r\n"
                 "%swis&n - %d\r\n"
                 "%sstr&n - %d\r\n"
                 "%scon&n - %d\r\n"
                 "%sdex&n - %d\r\n"
                 "%scha&n - %d\r\n"
                 "&ghit&n - %d, &gmana&n - %d, &gmove&n - %d\r\n\r\n"
                 "&1usage: &4train [number] <&gstat&4>&n\r\n",

	    train_cost(ch, STAT_INT, GET_REAL_INT(ch)),
	    train_cost(ch, STAT_WIS, GET_REAL_WIS(ch)),
	    train_cost(ch, STAT_STR, GET_REAL_STR(ch)),
	    train_cost(ch, STAT_CON, GET_REAL_CON(ch)),
	    train_cost(ch, STAT_DEX, GET_REAL_DEX(ch)),
	    train_cost(ch, STAT_CHA, GET_REAL_CHA(ch)),

	    train_cost(ch, STAT_HIT, GET_MAX_HIT(ch)), 
	    train_cost(ch, STAT_MANA, GET_MAX_MANA(ch)), 
	    train_cost(ch, STAT_MOVE, GET_MAX_MOVE(ch)),

	    IS_SET(pc_class_primary_stats[(int)GET_CLASS(ch)], 
		   (1 << STAT_INT)) ? " &g" : "&r!",
	    max_stat_value(ch, STAT_INT), 
	    IS_SET(pc_class_primary_stats[(int)GET_CLASS(ch)], 
		   (1 << STAT_WIS)) ? " &g" : "&r!",
	    max_stat_value(ch, STAT_WIS), 
	    IS_SET(pc_class_primary_stats[(int)GET_CLASS(ch)], 
		   (1 << STAT_STR)) ? " &g" : "&r!",
	    max_stat_value(ch, STAT_STR), 
	    IS_SET(pc_class_primary_stats[(int)GET_CLASS(ch)],
		   (1 << STAT_CON)) ? " &g" : "&r!",
	    max_stat_value(ch, STAT_CON), 
	    IS_SET(pc_class_primary_stats[(int)GET_CLASS(ch)], 
		   (1 << STAT_DEX)) ? " &g" : "&r!",
	    max_stat_value(ch, STAT_DEX), 
	    IS_SET(pc_class_primary_stats[(int)GET_CLASS(ch)],
		   (1 << STAT_CHA)) ? " &g" : "&r!",
	    max_stat_value(ch, STAT_CHA), 
	    max_stat_value(ch, STAT_HIT),
	    max_stat_value(ch, STAT_MANA),
	    max_stat_value(ch, STAT_MOVE));
    send_to_char(buf, ch);
    return (TRUE);       
  }

  // command is train - lets train!
  if (CMD_IS("train"))
  {
    two_arguments(argument, buf1, buf2);

    // multiple - that stupid old train hit, train hit, train hit, train hit
    if ((*buf) && (isdigit(*buf1)))
    {
      number = atoi(buf1);
      stat_name = buf2;
    } else {
      stat_name = buf1;
    }

    skip_spaces(&stat_name);

    // Do we have a stat/correct stat to train?
    if (!*stat_name)
    { 
      act("$n&r tells you, 'Train what? Type &4list&r for help (no stat name)'",
	  FALSE, trainer, 0, ch, TO_VICT);
      return (TRUE);
    }

    stat = search_block_case_insens(stat_name, stat_names, FALSE);

    if (stat < 0)
    {
      act("$n&r tells you, 'Train what? Type &4list&r for help (stat not found)'",
                      FALSE, trainer, 0, ch, TO_VICT);
      send_to_char(stat_name, ch);
      return (TRUE);
    }

    // check is stat is trainable
    if ((stat < STAT_HIT) &&
        !IS_SET(pc_class_primary_stats[(int)GET_CLASS(ch)], (1 << stat)))
    {
      sprintf(buf, "$n&r tells you, '%s is not a stat in your field'",
              stat_names[stat]);
      act(buf, FALSE, trainer, 0, ch, TO_VICT);
      return (TRUE);
    }

    // check max values
    if (GET_REAL_STAT(ch, stat) >= max_stat_value(ch, stat))
    {
      sprintf(buf, "$n&r tells you, 'You can't train your %s any more'", 
                      stat_names[stat]);
      act(buf, FALSE, trainer, 0, ch, TO_VICT);
      return (TRUE);
    }
      
    // check upper limit - readjust number of trains if necessary
    if (number + GET_REAL_STAT(ch, stat) > max_stat_value(ch, stat))
      number = max_stat_value(ch, stat) - GET_REAL_STAT(ch, stat);

    // calculate the accumulated cost of the train
    // Artus> Checking cost vs exp, too.
    for (i = 0; (i < number) && (cost <= GET_EXP(ch)); i++)
      cost += train_cost(ch, stat, GET_REAL_STAT(ch, stat) + i);

    // Experience Point required stats - HIT MANA MOVE
    if (stat > STAT_CHA)
    {
      if (GET_EXP(ch) < cost)
      {
	if (i <= 1)
	  sprintf(buf, "$n&r tells you, 'You can't affort to train your %s.'", 
	          stat_names[stat]);
	else
	  sprintf(buf, "$n&r tells you, 'You only have enough experience to train &c%d&r %s.'", i-1, stat_names[stat]);
        act(buf, FALSE, trainer, 0, ch, TO_VICT);
      } else {
        GET_EXP(ch) -= cost;
        switch (stat)
	{
          case STAT_HIT:
            GET_MAX_HIT(ch) += number;
            break;
          case STAT_MANA:
            GET_MAX_MANA(ch) += number;
            break;
          case STAT_MOVE:
            GET_MAX_MOVE(ch) += number;
            break;
        }
        sprintf(buf, "You train your %s &c%d&n time%s at a cost of &c%d&n experience.\r\n", stat_names[stat], number, (number > 1) ? "s" : "", cost);
        send_to_char(buf, ch);
      }
    // Quest Point required stats - INT WIS STR CON DEX CHA
    } else {
      if (GET_STAT_POINTS(ch) < cost)
      {
        sprintf(buf, "$n&r tells you, 'You don't have enough stat points to train &c%d&r %s.'", number, stat_names[stat]);
        act(buf, FALSE, trainer, 0, ch, TO_VICT);
      } else {
        GET_STAT_POINTS(ch) -= cost;
        switch (stat)
	{
          case STAT_INT:
            GET_REAL_INT(ch) += number;
            break;
          case STAT_WIS:
            GET_REAL_WIS(ch) += number;
            break;
          case STAT_STR:
            GET_REAL_STR(ch) += number;
            break;
          case STAT_CON:
            GET_REAL_CON(ch) += number;
            break;
          case STAT_DEX:
            GET_REAL_DEX(ch) += number;
            break;
          case STAT_CHA:
            GET_REAL_CHA(ch) += number;
            break;
        }
        sprintf(buf, "You train your %s &c%d&n time%s at a cost of &c%d&n stat points.\r\n", stat_names[stat], number, (number > 1) ? "s" : "", cost);
        send_to_char(buf, ch);
      }
    }
    return (TRUE);
  }
  // doh.
  return (FALSE);
}

#if 0 // Artus> Unused.
SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return (FALSE);

  if (GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && LR_FAIL(cons, LVL_IMMORT) && (!number(0, 4))) {
      npc_steal(ch, cons);
      return (TRUE);
    }
  return (FALSE);
}
#endif

// Artus> Used heaps.
SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return (TRUE);

  if (!LR_FAIL(ch, 13) && (number(0, 10) == 0))
    cast_spell(ch, vict, NULL, SPELL_SLEEP);

  if (!LR_FAIL(ch, 7) && (number(0, 8) == 0))
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

  if (!LR_FAIL(ch, 12) && (number(0, 12) == 0)) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
  }
  if (number(0, 4))
    return (TRUE);

  switch (GET_LEVEL(ch)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
    break;
  }
  return (TRUE);

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

#if 0 // Artus> Unused.
SPECIAL(puff)
{
  if (cmd)
    return (0);

  switch (number(0, 60)) {
  case 0:
    do_say(ch, "My god!  It's full of stars!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "How'd all those fish get up here?", 0, 0);
    return (1);
  case 2:
    do_say(ch, "I'm a very female dragon.", 0, 0);
    return (1);
  case 3:
    do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
    return (1);
  default:
    return (0);
  }
}
#endif

/* PRIMAL PROCS */
void gemReward(struct char_data *ch, int num, int type)
{
	int nLevel = GET_LEVEL(ch);
	int multipler = num * nLevel, reward = 0;
	const char *rewardnames[] = {
		"\n",
		"gold coin",
		"experience point",
		"stat point"
	};

	switch(type)
	{
		case 1:		// Gold
			reward = multipler * GET_LEVEL(ch) * 50 *
				(GET_CLASS(ch) == CLASS_MASTER ? 5  : 
				 GET_CLASS(ch) <  CLASS_WARRIOR ? 1 : 2);
			GET_GOLD(ch) += reward;	
			break;
		case 2:		// XP
			reward = multipler * GET_LEVEL(ch) * 50 * 
				(GET_CLASS(ch) == CLASS_MASTER ? 5  : 
				 GET_CLASS(ch) <  CLASS_WARRIOR ? 1 : 2);
			GET_EXP(ch) += reward;
			break;
		case 3:		// QP
			reward = num;
			GET_STAT_POINTS(ch) += reward;
			break;
	}

	sprintf(buf, "You receive %s%d&n %s%s!\r\n", type == 1 ? "&Y" : type == 2  ? "&b" : "&M", 
		reward, rewardnames[type], reward == 1 ? "" : "s");
	send_to_char(buf, ch);   
}

#if 0 // Artus> Quest Procs never did get finished.
/* Quest master for the automated quests */
SPECIAL(quest_master)
{
  if (!(cmd))
    return FALSE;
  // Giving something?
  if (CMD_IS("trade"))
  {
    half_chop(argument, arg, buf1);
    if (!*arg)
    {
      send_to_char("Trade what in?\r\n", ch);
      return TRUE;
    }			
    act("$N doesn't seem interested.", FALSE, ch, 0, me, TO_CHAR);
    return TRUE;
  }
  return FALSE;
}

SPECIAL(quest_sentry)
{
  ACMD(do_qcomm);
  ACMD(do_give);
  struct obj_data *obj;
  int r_num, quest_item_number = 0;
  struct char_data *quest = (struct char_data *) me;
  char buff[256];
  
  if (!(cmd))
    return FALSE;

  if (!(CMD_IS("give")) && !(CMD_IS("set")))
    return FALSE;

  if (CMD_IS("set"))
  {
    two_arguments(argument,buf1,buf2);
    if (strcmp(buf1,"quest")) return FALSE;
    if (LR_FAIL(ch, LVL_IS_GOD))
    {
      send_to_char("Now aren't you a bit young for that?",ch);
      return TRUE;
    }
    if (!*buf2)
    {
      send_to_char("What exactly are you questing for???",ch);
      return TRUE;
    }
    if ((quest_item_number = atoi(buf2)) < 0)
    {
      send_to_char("What exactly are you questing for???",ch);
      return TRUE;
    }

    if (quest_item_number > 0) {
      if ((r_num = real_object(quest_item_number)) < 0) {
        send_to_char("No object with that number.",ch);
        return TRUE;
      }
      obj = read_object(r_num,REAL);
      sprintf(buf1,"You are looking for the %s!",obj->name);
      do_qcomm(quest,buf1,0,SCMD_QSAY);
    }
    send_to_char("Quest Item Set.", ch);
    return TRUE;
  } 
  /* otherwise command = give */

  /*if (!one_argument(argreplace,arg1)) return FALSE;
  if (!one_argument(argreplace,arg2)) return FALSE;
  if (!isname(arg2,GET_NAME(quest))) return FALSE;*/

  if (quest_item_number == 0) return FALSE;
  do_give(ch,argument,0,0);
  if ((obj = get_obj_in_list_num(quest_item_number,quest->carrying)))
  {
    sprintf(buff,"The Quest is over!  The %s has been found by %s!",obj->name,GET_NAME(ch));
    do_qcomm(quest,buff,0,SCMD_QSAY);
    return TRUE;
  } else return TRUE;
}
#endif

#if 0 // Artus> Old Trainer.
SPECIAL(trainer)
{
  sbyte           *stat = NULL;
  int             *stat2=NULL;
  long int        cost;
 
  if (IS_NPC(ch) || !CMD_IS("train"))
    return(0);
 
  skip_spaces(&argument);

  send_to_char("No more of this shite. See &BTalisman&n if you want to whinge.\r\n", ch);
  return (1);

  if (!*argument)
  {
    // list the prices 
    send_to_char("\nThese are the prices for training...\n", ch);
                send_to_char("The trainer will help you increase one ability, for a price.\n", ch);
                send_to_char("Type 'train <stat>' (eg 'train str').\n", ch);
                send_to_char("----------------------\n",ch);
                send_to_char("13->14 : 1,000,000\n", ch);
                send_to_char("14->15 : 2,000,000\n", ch);
                send_to_char("15->16 : 4,000,000\n", ch);
                send_to_char("16->17 : 7,000,000\n", ch);
                send_to_char("17->18 : 11,000,000\n", ch);
                send_to_char("18->19 : 16,000,000\n", ch);
                send_to_char("19->20 : 22,000,000\n", ch);
                send_to_char("20->21 : 30,000,000\n", ch);
                send_to_char("-- Fixed price stats --\n", ch);
                send_to_char("mana   : 2,000 X level & 1 Practice point\n", ch);
                send_to_char("hit    : 2,000 X level.\n", ch);
                send_to_char("move   : 2,000 X level.\n", ch);
                send_to_char("----------------------\n", ch);
    return(1);
  }
  if (!strncmp(argument, "str", 3))
    stat = &(GET_REAL_STR(ch));
  if (!strncmp(argument, "con", 3))
    stat = &(GET_REAL_CON(ch));          if (!strncmp(argument, "dex", 3))
    stat = &(GET_REAL_DEX(ch));
  if (!strncmp(argument, "int", 3))
    stat = &(GET_REAL_INT(ch));
  if (!strncmp(argument, "wis", 3))
    stat = &(GET_REAL_WIS(ch));
  if (!strncmp(argument, "cha", 3))
    stat = &(GET_REAL_CHA(ch));
  if (!strncmp(argument, "mana", 4))
    stat2 = &(GET_MAX_MANA(ch));
  if (!strncmp(argument, "hit", 3))
    stat2 = &(GET_MAX_HIT(ch));
  if (!strncmp(argument, "move", 4))
    stat2 = &(GET_MAX_MOVE(ch));
        if ((!stat) && (!stat2))
        {
                send_to_char("You can only train STR INT WIS CON DEX CHA MANA MOVE or HIT!\n\r", ch);
                return(1);
        }
  if ( (!strncmp(argument, "mana",4)) || (!strncmp(argument, "hit",3)) || 
	(!strncmp(argument, "move", 4)) )
  cost = 2000 * GET_LEVEL(ch);
  else
  if (*stat < 14)
    cost = 1000000;
  else
    switch(*stat)
    {
      case 13 : cost = 1000000;  break;
      case 14 : cost = 2000000;  break;
      case 15 : cost = 4000000;  break;
      case 16 : cost = 7000000;  break;
      case 17 : cost = 11000000; break;
      case 18 : cost = 16000000; break;
      case 19 : cost = 22000000; break;
      case 20 : cost = 30000000; break;
      default : cost = -1;
  }
 
  if (cost < 0)
  {
    send_to_char("You are already learned in that area\n", ch);
    return(1);
  }
  if (GET_GOLD(ch) < cost)
  {
    send_to_char("You don't have enough gold to train!\n", ch);
    return(1);
  }

  if ((!strncmp(argument, "move",4)))
  {
    if ((GET_MAX_MOVE(ch)>=(GET_LEVEL(ch)*10))&&(!strncmp(argument, "move",4)))
    {
      send_to_char("You have gained maximum MOVE for your level, you cannot train more.\n", ch);
      return(1);
    } else {
      send_to_char("You train long and hard!\n", ch);
      sprintf(buf, "That will cost %s%ld%s gold coins\n", 
		CCGOLD(ch,C_NRM),cost,CCNRM(ch,C_NRM));
      send_to_char(buf, ch);
 
      (*stat2)++;
      GET_GOLD(ch) -= cost;
      return(1);
    }
  }
 
  if ((!strncmp(argument, "mana",4)))
  {
    if ((GET_MAX_MANA(ch)>=(GET_LEVEL(ch)*25))&&(!strncmp(argument, "mana",4)))
    {
      send_to_char("You have gained maximum MANA for your level, you cannot train more.\n", ch);
      return(1);
    }
 
    if ((!strncmp(argument, "mana",4) && GET_PRACTICES(ch)==0)){
          send_to_char("You don't seem to have any Practice points at the moment!\r\n", ch);
          return(1);
    }
    else{
                send_to_char("You train long and hard!\n", ch);
                sprintf(buf, "That will cost %s%ld%s gold coins\n",
		  CCGOLD(ch,C_NRM),cost,CCNRM(ch,C_NRM));
                send_to_char(buf, ch);
                if (!strncmp(argument, "mana",4))
                  GET_PRACTICES(ch) -= 1;
 
                (*stat2)++;
                GET_GOLD(ch) -= cost;
                return(1);
    }
  }                                     // Check if Hitpoints being trained
 
  if ((!strncmp(argument, "hit",3)))
  {
    if ((GET_MAX_HIT(ch)>=(GET_LEVEL(ch)*45))&&(!strncmp(argument,"hit",3)))
    {
      send_to_char("You have gained maximum HP for your level, you cannot train more.\n", ch);
      return(1);
    }

  /*
    if (((!strncmp(argument, "hit", 3)) && GET_PRACTICES(ch)==0 && GET_LEVEL(ch)>29)){
          send_to_char("You don't seem to have any Practice points at the moment!\n", ch);
          return(1);
    }*/

    else{
                send_to_char("You train long and hard!\n", ch);
                sprintf(buf, "That will cost %s%ld%s gold coins\n",
		  CCGOLD(ch,C_NRM),cost,CCNRM(ch,C_NRM));
                send_to_char(buf, ch);

          /*      if ((!strncmp(argument, "hit",3) && GET_LEVEL(ch)>29)){
                  send_to_char("Your training progresses well your skill warrants me to extract a higher toll for services.\n", ch);
                  GET_PRACTICES(ch) -= 1;
                }*/
 
                (*stat2)++;
                GET_GOLD(ch) -= cost;
                return(1);
    }
  }
 
  send_to_char("You train long and hard!\n", ch);
  sprintf(buf, "That will cost %s%ld%s gold coins\n",
		CCGOLD(ch,C_NRM),cost,CCNRM(ch,C_NRM));
  send_to_char(buf, ch);                     
  (*stat)++;
  GET_GOLD(ch) -= cost;
  return(1);
}   
#endif

SPECIAL(cleric)
{
  struct char_data *vict, *mob;
  int mob_num;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);
  
  /* random chance that cleric mob will heal itself */
  if (!number(0, 4))
  {
    int level = GET_LEVEL(ch);

    if (level <= 3)
    {
      cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT);
      return TRUE;
    }
    if (level <= 9)
    {
      cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
      return TRUE;
    }
    cast_spell(ch, ch, NULL, SPELL_HEAL);
    return TRUE;
  }

  /* chance that mob will conjure some help */
  if (!number(0, 2))
  { 
    if (GET_LEVEL(ch) <= 10)
    { 
      if (IS_EVIL(ch))
        mob_num = 1016;
      else
        mob_num = 1019;
    }
    else
      if (GET_LEVEL(ch) <= 20)
      { 
        if (IS_EVIL(ch))
          mob_num = 1017;
        else
          mob_num = 1020;
      }
      else
      {
        if (IS_EVIL(ch))
          mob_num = 1018;
        else
          mob_num = 1021;
      }

/* load the mob and set it fighting */
    mob = read_mobile(mob_num, VIRTUAL);
    char_to_room(mob, ch->in_room);
    act("$n prays to their deity who sends assistance", FALSE, ch, 0, 0, TO_ROOM);
    act("$n joins in the fight!", FALSE, mob, 0, 0, TO_ROOM);
    hit(mob, vict, TYPE_UNDEFINED);
    return TRUE;
  }

/* other wise just cast some spells */

  switch (GET_LEVEL(ch)) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
    break;
  }
  return TRUE;
}

SPECIAL(cleric2)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);
  
  /* random chance that cleric mob will heal itself */
  if (!number(0, 4))
  {
    int level = GET_LEVEL(ch);

    if (level <= 3)
    {
      cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);
      return TRUE;
    }
    if (level <= 9)
    {
      cast_spell(ch, ch, NULL, SPELL_HEAL);
      return TRUE;
    }
    cast_spell(ch, ch, NULL, SPELL_ADV_HEAL);
    return TRUE;
  }

/* other wise just cast some spells */

  switch (GET_LEVEL(ch)) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE);
    break;
  case 14:
  case 15:cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
    break;
  default:
    cast_spell(ch, ch, NULL, SPELL_ADV_HEAL);
    break;
  }
  return TRUE;

}
SPECIAL(warrior)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);
  
  switch (number(0, 50)) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SKILL_KICK);
    break;
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
    cast_spell(ch, vict, NULL, SKILL_BASH);
    break;
  case 11:
  case 12:
  case 13:
  case 14:
    act("$n headbutts $N. OUCH!", FALSE, ch, 0, vict, TO_ROOM);
    act("$n headbutts you. OUCH what a headache!", FALSE, ch, 0, 0, TO_VICT);
    damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*5),TYPE_UNDEFINED,FALSE);
    break;
  case 15:
  case 16:
  case 17:
   default:
    cast_spell(ch, vict, NULL, SKILL_BASH);
    break;
  }
  return TRUE;

}

SPECIAL(warrior1)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);
  
  switch (number(0, 50)) 
  {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 8:
    case 9:
    case 10:
      act("$n grabs $N in a monstrous bearhug. You hear bones crunching.", FALSE,ch,0,vict,TO_ROOM);
      sprintf(buf, "%s%s bearhugs you. %s seems to have a crush on you.%s\r\n",CCRED(vict,C_NRM), GET_NAME(ch), GET_NAME(ch) , CCNRM(vict, C_NRM));
      send_to_char(buf, vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*8), TYPE_UNDEFINED,FALSE);
      break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
      act("$n trips $N who goes down in a heap. GIGGLE!", FALSE,ch,0,vict,TO_ROOM);
      sprintf(buf, "%s%s trips you.  You go down in a messy heap.%s\r\n",CCRED(vict,C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
      send_to_char(buf, vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*2), TYPE_UNDEFINED,FALSE);
      if (GET_HIT(vict) > 0)
      {
	GET_POS(vict) = POS_SITTING;
	WAIT_STATE(vict, PULSE_VIOLENCE * 3);
	act("$n scrambles to $s feet!.", FALSE, vict, 0, 0, TO_ROOM);
	sprintf(buf, "%sYou scramble to your feet!.%s\r\n",CCRED(vict, C_NRM), CCNRM(vict, C_NRM));
	send_to_char(buf, vict);
	GET_POS(vict) = POS_FIGHTING;
      }
      break;
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
      act("$n headbutts $N. OUCH!",FALSE,ch,0,vict,TO_ROOM);
      sprintf(buf, "%s%s headbutts you. OUCH what a headache!%s\r\n",CCRED(vict,C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
      send_to_char(buf, vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*5), TYPE_UNDEFINED, 
	     FALSE);
      break;
    default:
      act("$n picks $N up and piledrives $M.  OUCH that looks most uncomfortable.", FALSE, ch, 0, vict, TO_ROOM);
      sprintf(buf, "%s%s picks you up and piledrives you.  Awww look at all the pretty stars!!%s\r\n", CCRED(vict, C_NRM), GET_NAME(ch), CCNRM(vict, C_NRM));
      send_to_char(buf, vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*7), TYPE_UNDEFINED, 
	     FALSE);
      if (GET_HIT(vict) < 0)
      {
	GET_POS(vict) = POS_SITTING;
	WAIT_STATE(vict, PULSE_VIOLENCE);
	act("$n scrambles to $s feet!.", FALSE, vict, 0, 0, TO_ROOM);
	sprintf(buf, "%sYou scramble to your feet!.%s\r\n",CCRED(vict, C_NRM), CCNRM(vict, C_NRM));
	send_to_char(buf, vict);
	GET_POS(vict) = POS_FIGHTING;
      }
      break;
  }
  return TRUE;
}

SPECIAL(regen)
{
  int regen_points;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 2))
    return TRUE;

  act("$n's wounds heal before your very eyes! ", FALSE, ch, 0, 0, TO_ROOM);
  
  regen_points = GET_LEVEL(ch) + 20;
  if (GET_HIT(ch) + regen_points > GET_MAX_HIT(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);
  else
    GET_HIT(ch) += regen_points;
  return(TRUE);
}

SPECIAL(school)
{
  struct char_data *temp, *vict, *mob, *this_mob=(struct char_data *) me;
  int count=1;

  if (cmd || GET_POS(this_mob) != POS_FIGHTING)
    return FALSE;

  if (GET_HIT(this_mob) < (GET_MAX_HIT(this_mob)/3))
  {
    do_flee(this_mob, "", 0, 0);
    return(1);
  }

  if (!(vict=FIGHTING(this_mob)))
    return FALSE; 
  
  if (number(0, 6))
    return TRUE;
	
  /* Check the number of "school" in room */
  for (temp = world[this_mob->in_room].people; temp; temp = temp->next_in_room)
    if (IS_NPC(temp) && (GET_MOB_VNUM(temp) == GET_MOB_VNUM(this_mob))
	&& temp != vict)
      count++;

  if (count > 7)
    return TRUE;

/* load the mob and set it fighting */
  mob = read_mobile(GET_MOB_VNUM(this_mob), VIRTUAL);
  char_to_room(mob, ch->in_room);
  act("$n arrives to join in!", FALSE, ch, 0, 0, TO_ROOM);
/*  act("$n joins in the fight!", FALSE, mob, 0, 0, TO_ROOM); */
  hit(mob, vict, TYPE_UNDEFINED); 
  return TRUE;
}

SPECIAL(trojan)
{
  struct char_data *vict, *mob;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  // Artus> Lets make this a little more likely :o)
  if (number(0, 8))
    return TRUE;
	
/* load the mob and set it fighting */
  mob = read_mobile(4824, VIRTUAL);
  char_to_room(mob, ch->in_room);
  mob->char_specials.timer=number(1,2);
  act("$n runs out of the $N screaming.  ARRARGGGGHHH!!!", FALSE, mob, 0, ch, TO_ROOM);
  act("$n joins the fight.", FALSE, mob, 0,0, TO_ROOM);
  vict = FIGHTING(ch);
  hit(mob, vict, TYPE_UNDEFINED); 
  return TRUE;
}

SPECIAL(drainer)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;
	
  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  switch(number(0,50))
  {
    case 1:
      act("$n's stares at $N and screams!", FALSE, ch, 0, vict, TO_NOTVICT);
      act("$n's stares at you and screams!", FALSE, ch, 0, vict, TO_VICT);
      act("$n's lifeforce is drained permanently!", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("Your lifeforce is drained permanently!\n\r", vict);

      GET_MAX_HIT(vict)--;
    
       if (GET_MAX_HIT(vict) < 1)
	    GET_MAX_HIT(vict) = 1;

      return(1);
      break;
    case 2:
    case 3:
      act("$n's stares at $N and shouts!", FALSE, ch, 0, vict, TO_NOTVICT);
      act("$n's stares at you and shouts!", FALSE, ch, 0, vict, TO_VICT);
      act("$n's lifeforce is drained temporarely!", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("Your lifeforce is drained temporarily!\n\r", vict);
      damage(ch, vict, GET_LEVEL(ch)*5, TYPE_UNDEFINED, FALSE);
      GET_MANA(vict) -= GET_LEVEL(ch);
      return(1);
      break;
    default: 
      return(1);
  }
}
  
SPECIAL(constrictor)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  act("$N wraps its body around $n and squeezes the life out of $n! ", FALSE, vict, 0, ch, TO_NOTVICT);
  send_to_char("You feel your ribs crack and the breath squeezed out of you!\r\n", vict);

  damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)+18), TYPE_UNDEFINED, FALSE);
  return(TRUE);  
}

SPECIAL(self_destruct)
{
  struct char_data *vict;
 
  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  if (number(0, (GET_HIT(ch)/300)+2) > 1)
    return FALSE;

  act("$n explodes in a fireball of poisonous gas!", FALSE, ch, 0, 0, TO_ROOM);

  /* go thru all the pcs fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) && !IS_NPC(vict)) /* fighting me so they get the acid bath */
    {
      act("$n is blown to tiny bits by the explosion!", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("You are blown to bits from the explosion!\r\n", vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*10), TYPE_UNDEFINED,
	     FALSE);
      handle_fireball(vict);
      die(ch,NULL);    
    }
  return(TRUE); 
}

SPECIAL(acid_breath)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  act("$n breathes a cloud of acid into the air", FALSE, ch, 0, 0, TO_ROOM);

  /* go thru all the pcs fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) && !IS_NPC(vict)) /* fighting me so they get the acid bath */
    {
      act("$n's skin explodes in a quivering mess ", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("Your skin is reduced to a quivering blob of flesh by the acid cloud!\r\n", vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*2), TYPE_UNDEFINED,
	     FALSE);
    }
  return(TRUE); 
}
  
SPECIAL(fire_breath)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  act("$n breathes a plume of hot flames into the air", FALSE, ch, 0, 0, TO_ROOM);

  /* go thru all the pcs fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) && !IS_NPC(vict)) /* fighting me so they get the acid bath */
    {
      act("$n's skin explodes in a quivering mess ", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("Your are burnt to a crisp by the fire breath!\r\n", vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*3), TYPE_UNDEFINED,
	     FALSE);
      handle_fireball(vict);
    }
  return(TRUE); 
}

SPECIAL(ice_breath)
{
  struct char_data *vict;
 
  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
 
  if (number(0, 3))
    return TRUE;
 
  act("$n breaths a cloud of frozen water vapour into you!", FALSE, ch, 0, 0, TO_ROOM);
 
  /* go thru all the pcs fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) && !IS_NPC(vict)) /* fighting me so they get the ice bath */
    {
      act("$n's skin is shredded by the frozen ice needles", FALSE, vict, 0, 0, TO_NOTVICT);
      
      send_to_char("Your skin is shredded by the frozen ice needles.\r\n",vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*3), TYPE_UNDEFINED,
             FALSE);
    }
  return(TRUE);
}
  
SPECIAL(gaze_npc)
{
  struct char_data *vict, *next;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  act("$n sends out a deadly gaze at its attackers.", FALSE, ch, 0, 0, TO_ROOM);

  /* go thru all the chars fighting me */
  for (vict = world[ch->in_room].people; vict; vict = next)
  {
    next = vict->next_in_room;
    if (vict == ch)  // Artus> Don't want to hurt myself.
      continue;
    if (FIGHTING(vict) != ch) // Artus> Only hurt those that are fighting me.
      continue;
    if (IS_NPC(vict)) // NPCs get killed instantly.
    {
      act("$n grabs $s chest gasping for life. $n drops DEAD!\r\n", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("Your heart suddenly stops working. You see your life flash before your eyes.\r\n", vict);
      raw_kill(vict,ch);
      continue;
    }
    // PCs that are fighting me.
    act("$n grabs $s chest and screams in pain.\r\n", FALSE, vict,0,0,TO_NOTVICT);
    send_to_char("Your heart misses a few beats. You see your life flash before your eyes.\r\n", vict);
    damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*10), TYPE_UNDEFINED,
           FALSE);
  } 
  return(TRUE); 
}

SPECIAL(elec_shock)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 3))
    return TRUE;

  act("$n sends out a charge of electricity!", FALSE, ch, 0, 0, TO_ROOM);

  /* go thru all the pcs fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if ((FIGHTING(vict)==ch) && !IS_NPC(vict)) /* fighting me so they get the acid bath */
    {
      act("$n is lit up like a christmas tree from an electric shock! ", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("Your muscles convulse in agony from electrocution!\r\n", vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*2), TYPE_UNDEFINED,
	     FALSE);
    }
  return(TRUE); 
}  

#if 0 // Artus> Unused.
SPECIAL(head_druid)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
     return FALSE;

  if (number (0,2))
    return TRUE;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number (0,4)) break;

  if (vict == NULL)
     vict = FIGHTING(ch);

  cast_spell (ch, vict, NULL, SPELL_CALL_LIGHTNING); 
  return TRUE;
}
#endif

SPECIAL(blood_sucker)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (number (0, 4))
    return TRUE;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number (0,4)) break;

  if (vict == NULL)
     vict = FIGHTING(ch);

  act("$N goes for $n's throat! ", FALSE, vict, 0, ch, TO_NOTVICT);
  act("$N goes for your throat! ", FALSE, vict, 0, ch, TO_CHAR);

  damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*2+20), TYPE_UNDEFINED,
         FALSE);
  cast_spell(ch, vict, NULL, SPELL_HARM); 
  return TRUE;
}


/* Peacekeeper (sheriffs etc), grabs a fighting player sends them to jail for disturbing the peace - bill */

#define LOCALJAIL 13251

/*this is a temporary kludge pending a more general jailroom selection mechanism */

SPECIAL(peacekeeper)
{
  struct char_data * vict, * iter;    /* victim and iterator for list */
  vict = 0;

  /* can't arrest anyone if im asleep or busy fighting */
  if (cmd || !AWAKE(ch) || GET_POS(ch) == POS_FIGHTING)
    return FALSE;

   /* maybe show leniency */
  if (number(0,1))
    return FALSE;

  /* look for fighting players */
  for (iter = world[ch->in_room].people;iter;iter = iter->next_in_room) {
    if (CAN_SEE(ch, iter) && FIGHTING(iter) && !IS_NPC(iter))
      vict = iter;
  }

  /* if i found any, lock em up! */
  if (vict) {
    act("$n says, 'That's enough fighting! I run a peacable town!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n grabs you by the neck and hauls you off to jail.", FALSE, ch, 0 , vict, TO_VICT);
    act("$n says, 'That's what you get for disturbing the peace!'", FALSE, ch, 0, vict, TO_VICT);
    send_to_char("You are dumped unceremoniously in the local jail.\n", vict);
    act("$n grabs $N by the scruff of the neck and hauls $M off to jail!", FALSE, ch, 0, vict, 
        TO_NOTVICT);
    char_from_room(vict);
    char_to_room(vict, real_room(LOCALJAIL));
    GET_POS(vict) = POS_SITTING;
    look_at_room(vict,0);
    return TRUE;
  }
  return FALSE;
}  

/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(werewolf)
{
  ACMD(do_gen_comm);
  struct char_data *vict, *wolf = (struct char_data *) me;
 
  if(cmd)
    return 0;

  if (!(weather_info.moon == MOON_FULL))
    return 0;
  if (!(weather_info.sunlight == SUN_DARK))
    return 0;

  if (FIGHTING(wolf))
  {
    switch(number(0,2)) 
    {
      case 0: wolf->mob_specials.attack_type = 4; /* bite */
                break;
      case 1: wolf->mob_specials.attack_type = 8; /* claw */
                break;
      case 2: wolf->mob_specials.attack_type = 9; /* maul */
                break;
    }
    for (vict = world[IN_ROOM(wolf)].people; vict; vict = vict->next)
    {
      if ((IS_NPC(vict)) || (FIGHTING(vict) != wolf))
	continue;
      if (PRF_FLAGGED(vict, PRF_WOLF | PRF_VAMPIRE))
	continue;
      if (number(1, 10) == 1)
      {
	act("$n bites you VERY hard. You slip out of consciousness.", 
	    FALSE, wolf, NULL, vict, TO_VICT);
	act("$n bites $N EXTREMELY hard. $N falls to the ground, unconscious.",
	    FALSE, wolf, NULL, vict, TO_NOTVICT);
	stop_fighting(vict);
	if (FIGHTING(wolf) && (FIGHTING(wolf) == vict))
	  stop_fighting(wolf);
	SET_BIT(PRF_FLAGS(vict), PRF_WOLF);
	GET_POS(vict) = POS_STUNNED;
	return 1;
      }
    }
    return 0;
  }
  if (number(0, 10) == 0)
  {
    do_gen_comm(wolf,"HHHHHHOOOOOOWWWWWWLLLLLL!!!!!!",0,SCMD_HOLLER);
    return 1;
  }
  return 0;

}

SPECIAL(vampire)
{
  ACMD(do_say);
  ACMD(do_gen_comm);
  struct char_data *vict, *vamp = (struct char_data *) me;
 
  if(cmd)
    return 0;
  
  if (!(weather_info.moon == MOON_FULL))
    return 0;
  if (!(weather_info.sunlight == SUN_DARK))
    return 0;

  if (FIGHTING(vamp))
  {
    for (vict = world[IN_ROOM(vamp)].people; vict; vict = vict->next)
    {
      if ((IS_NPC(vict)) || (FIGHTING(vict) != vamp))
	continue;
      if (PRF_FLAGGED(vict, PRF_WOLF | PRF_VAMPIRE))
	continue;
      if (number(1, 10) == 1)
      {
	stop_fighting(vict);
	if (FIGHTING(vamp) && (FIGHTING(vamp) == vict))
	  stop_fighting(vamp);
	act("You grab $N and sink your teeth into $S neck.",
	    FALSE,vamp,0,vict,TO_CHAR);
	act("$n grabs you from behind and sinks $s fangs into your neck!",
	    FALSE,vamp,0,vict,TO_VICT);
	act("$n grabs $N from behind and sinks $s fangs into $N's neck!",
	    FALSE,vamp,0,vict,TO_NOTVICT);
	act("$N slumps to the ground, the life drained out of $M...",
	    FALSE,vamp,0,vict,TO_NOTVICT);
	do_say(vamp,"I have drained you to within an inch of your life, and now I make you an offer you cannot refuse...",0,0);
	act("$n rejuvenates $N with some drops of $s own blood.",
	    FALSE,vamp,0,vict,TO_NOTVICT);
	act("In your desperation to live you drink from $n's wrist...",
	    FALSE,vamp,0,vict,TO_VICT);
	SET_BIT(PRF_FLAGS(vict), PRF_VAMPIRE);
	GET_POS(vict) = POS_STUNNED;
	return 1;
      }
    }
    return 0;
  }
  if (number(0, 10) == 0)
  {
    do_gen_comm(vamp,"Anyone interested in coming to dinner? Muhuhahahaha!",0,SCMD_HOLLER);
    act("$n smiles evilly, revealing $s large fangs.",FALSE,vamp,0,0,TO_ROOM);
    return 1;
  }
  return 0;
}

#if 0

#define WOLF_NORMAL   0
#define WOLF_CHANGING 1
#define WOLF_NOCHANGE 2

/* Ok this makes a mob change into a werewolf when the run goes down
 * and return to the original mob when it comes up again..
 * this spec_proc controls both the changing mob and the wolf after
 * change to make the wolf attack mobs and do other cool stuff - VADER
 */
SPECIAL(werewolf)
{
  ACMD(do_say);
  ACMD(do_gen_comm);
  ACMD(do_sleep);
  ACMD(do_wake);
  ACMD(do_stand);
  int room,vnum;
  struct char_data *vict, *wolf = (struct char_data *) me;
 
  if(cmd)
    return 0;
 
  switch(weather_info.sunlight) {
    case SUN_SET: if((wolf->char_specials.timer == WOLF_NORMAL) &&
                     (GET_MOB_VNUM(wolf) != WOLF_VNUM)) {
                    if(weather_info.moon == MOON_FULL)
                      wolf->char_specials.timer = WOLF_CHANGING;
                    else
                      wolf->char_specials.timer = WOLF_NOCHANGE;
                    }
                  break;
    case SUN_DARK:if((wolf->char_specials.timer == WOLF_CHANGING) &&
                     (GET_MOB_VNUM(wolf) != WOLF_VNUM)) {
                    do_gen_comm(wolf,"Help! It's happening! Run! Save yourselves!",0,SCMD_HOLLER);
                    act("A bloody pentangle appears on $n's hand!",FALSE,wolf,0,0,TO_ROOM);
                    act("$n screams in pain as he transforms into a wolf!",FALSE,wolf,0,0,TO_ROOM);
                    act("You scream in agony as the transformation takes place...",FALSE,wolf,0,0,TO_CHAR);
                    do_gen_comm(wolf,"HHHHHHHHHHHOOOOOOOOOOO--",0,SCMD_HOLLER);
                    room = wolf->in_room;
                    vnum = GET_MOB_VNUM(wolf);
                    extract_char(wolf);
                    wolf = read_mobile(WOLF_VNUM, VIRTUAL);
                    wolf->char_specials.timer = vnum;
                    char_to_room(wolf,room);
                    do_gen_comm(wolf,"--WWWWWWWWWWLLLLLLLLLL!!!!!!",0,SCMD_HOLLER);
                    return 0;
                    }
                  break;
    case SUN_RISE:if(GET_MOB_VNUM(wolf) == WOLF_VNUM) {
                    act("$n whimpers in pain.",FALSE,wolf,0,0,TO_ROOM);
                    do_sleep(wolf,"",0,0);
                    act("$n returns to $s original form.",FALSE,wolf,0,0,TO_ROOM);
                    room = wolf->in_room;
                    vnum = wolf->char_specials.timer;
                    extract_char(wolf);
                    if(vnum > 0) {
                      wolf = read_mobile(vnum, VIRTUAL);
                      char_to_room(wolf,room);
                      GET_POS(wolf) = POS_SLEEPING;
                      do_wake(wolf,"",0,0);
                      do_stand(wolf,"",0,0);
                      do_say(wolf,"Where am I?",0,0);
                      wolf->char_specials.timer = WOLF_NORMAL;
                      }
                    }
                  break;
    case SUN_LIGHT:if((GET_MOB_VNUM(wolf) != WOLF_VNUM) &&
                      (wolf->char_specials.timer == WOLF_NOCHANGE)) {
                     wolf->char_specials.timer = WOLF_NORMAL;
                     }
                   return 0;
                   break;
    }
 
/* make the fool tell ppl hes the werewolf for the tic between sunset
 * and night..
 */
  if((wolf->char_specials.timer == WOLF_CHANGING) &&
     (GET_MOB_VNUM(wolf) != WOLF_VNUM) && (!number(0,5))) {
    do_say(wolf,"Oh no! It's happening, I can feel it!",0,0);
    act("$n begins to sweat.",FALSE,wolf,0,0,TO_ROOM);
    }
 
  if(GET_MOB_VNUM(wolf) == WOLF_VNUM) {
/* change attack type between bite, claw, and maul so its real wolfy */
    switch(number(0,2)) {
      case 0: wolf->mob_specials.attack_type = 4; /* bite */
                break;
      case 1: wolf->mob_specials.attack_type = 8; /* claw */
                break;
      case 2: wolf->mob_specials.attack_type = 9; /* maul */
                break;
      }
 
/* make the wolf attack mobs and player killers */
    if(!FIGHTING(wolf)) {
    for(vict = world[wolf->in_room].people; vict; vict = vict->next_in_room) {
      if((vict != wolf) && (!number(0,3)) && (CAN_SEE(wolf,vict)) &&
         (IS_NPC(vict) || (!IS_NPC(vict) && 
          IS_SET(PLR_FLAGS(vict),PLR_KILLER))))
        break;
      }
    if(vict == NULL)
      return 0;
 
    if(!number(0,2)) {
      act("$n growls angrily.",FALSE,wolf,0,0,TO_ROOM);
      hit(wolf,vict,TYPE_UNDEFINED);
      }
    } else {
/* make the wolf infect ppl */
      if((number(0,10) == 0) && (!PRF_FLAGGED(FIGHTING(wolf),PRF_WOLF | PRF_VAMPIRE))) {
        vict = FIGHTING(wolf);
        stop_fighting(vict);
        stop_fighting(wolf);
        act("You sink your teeth into $N's hand...",FALSE,wolf,0,vict,TO_CHAR);
        act("$n bites you on the hand sending $s saliva into your bloodstream...",FALSE,wolf,0,vict,TO_VICT);
        act("$n sinks $s teeth into $N's hand.",FALSE,wolf,0,vict,TO_NOTVICT);
        act("You begin to feel weak and fall helpless to the ground...",FALSE,wolf,0,vict,TO_VICT);
        act("$N turns a strange shade of white and falls to the ground...",FALSE,wolf,0,vict,TO_NOTVICT);
        SET_BIT(PRF_FLAGS(vict), PRF_WOLF);
        GET_POS(vict) = POS_STUNNED;
        }
      }
    }
 
  return 0;
}


/* vampire spec proc used for the thing thats turns into a vamp (a bat) and
 * the vamp itself - Vader
 */
SPECIAL(vampire)
{
  ACMD(do_gen_comm);
  ACMD(do_say);
  int room,vnum;
  struct char_data *vict, *vamp = (struct char_data *) me;
 
  if(cmd)
    return 0;
 
  switch(weather_info.sunlight) {
    case SUN_SET:  /* is we're a bat un-hide us */
      if(GET_MOB_VNUM(vamp) != VAMP_VNUM) {
        GET_POS(vamp) = POS_STANDING;
        REMOVE_BIT(AFF_FLAGS(vamp),AFF_HIDE);
        }
      break;
    case SUN_DARK:  /* if its a full moon lets vamp out */
      if((weather_info.moon == MOON_FULL) && (GET_MOB_VNUM(vamp) != VAMP_VNUM)) {
        act("With a puff of smoke $n changes into a man with large fangs...",FALSE,vamp,0,0,TO_ROOM);
        room = vamp->in_room;
        vnum = GET_MOB_VNUM(vamp);
        extract_char(vamp);
        vamp = read_mobile(VAMP_VNUM, VIRTUAL);
        vamp->char_specials.timer = vnum;
        char_to_room(vamp,room);
        do_gen_comm(vamp,"Anyone interested in coming to dinner? Muhuhahahaha!",0,SCMD_HOLLER);
        act("$n smiles evilly, revealing $s large fangs.",FALSE,vamp,0,0,TO_ROOM);
        }
      break;
    case SUN_RISE:  /* suns up?!?! lets get outta here! */
      if(GET_MOB_VNUM(vamp) == VAMP_VNUM) {
        act("$n screams at the sight of the sun.",FALSE,vamp,0,0,TO_ROOM);
        sprintf(buf,"With a puff of smoke, %s transforms into $n!",GET_NAME(vamp));
        room = vamp->in_room;
        vnum = vamp->char_specials.timer;
        extract_char(vamp);
        if(vnum > 0) {
          vamp = read_mobile(vnum, VIRTUAL);
          char_to_room(vamp,room);
          act(buf,FALSE,vamp,0,0,TO_ROOM);
          }
        }
      break;
    case SUN_LIGHT:  /* if its daylight lets hide */
      if(!IS_SET(AFF_FLAGS(vamp),AFF_HIDE)) {
        GET_POS(vamp) = POS_RESTING;
        SET_BIT(AFF_FLAGS(vamp),AFF_HIDE);
        }
      break;
    }
 
/* this bitll make em infect if fighting */
  if((GET_MOB_VNUM(vamp) == VAMP_VNUM) && (FIGHTING(vamp)) && (number(0,10) == 0)
     && (!PRF_FLAGGED(FIGHTING(vamp),PRF_WOLF | PRF_VAMPIRE))) {
    vict = FIGHTING(vamp);
    stop_fighting(vict);
    stop_fighting(vamp);
    act("You grab $N and sink your teeth into $S neck.",FALSE,vamp,0,vict,TO_CHAR);
    act("$n grabs you from behind and sinks $s fangs into your neck!",FALSE,vamp,0,vict,TO_VICT);
    act("$n grabs $N from behind and sinks $s fangs into $N's neck!",FALSE,vamp,0,vict,TO_NOTVICT);
    act("$N slumps to the ground, the life drained out of $M...",FALSE,vamp,0,vict,TO_NOTVICT);
    do_say(vamp,"I have drained you to within an inch of your life, and now I make you an offer you cannot refuse...",0,0);
    act("$n rejuvenates $N with some drops of $s own blood.",FALSE,vamp,0,vict,TO_NOTVICT);
    act("In your desperation to live you drink from $n's wrist...",FALSE,vamp,0,vict,TO_VICT);
    SET_BIT(PRF_FLAGS(vict),PRF_VAMPIRE);
    GET_POS(vict) = POS_STUNNED;
    }
 
  return 0;
}
#endif

/* Modified by DM */
SPECIAL(clone)
{
  ACMD(do_assist);
  struct char_data *char_room, *master, *clone_mob;

  clone_mob = (struct char_data *) me;
  master = clone_mob->master;

/* Order are being abused for exp.. thisll fix it - Vader */
  GET_EXP(clone_mob) = 0;

  // Ordering clones was done in here, but moved to do_order ...

  if (cmd)
    return FALSE;

  if (!master) {
    die_clone(clone_mob, NULL);
    return TRUE;
  }

  /* Check to see if master is in same room as clone when fighting - DM */
  if (FIGHTING(clone_mob) && (!(master->in_room == clone_mob->in_room))) { 
     die_clone(clone_mob, NULL);
     return TRUE;
  }

  /* Check to see if PC's are fighting clone - DM */
  if (FIGHTING(clone_mob))
    for (char_room = world[clone_mob->in_room].people; 
		char_room; char_room = char_room->next_in_room) {
      if (!IS_NPC(char_room) && (FIGHTING(char_room) == clone_mob))
      {
        die_clone(clone_mob, NULL);
        return TRUE;
      }
    }

  /* Assist the master */
  if (master && (clone_mob->in_room == master->in_room)) {

    if ((FIGHTING(master) && !FIGHTING(clone_mob)) && IS_NPC(FIGHTING(master)))
      do_assist(clone_mob, GET_NAME(master),0,0);        

    } else if (!master) 
      die_clone(clone_mob, NULL);
    
  return TRUE;
}

SPECIAL(zombie)
{
  ACMD(do_say);
  struct char_data *vict;

  if(cmd || FIGHTING(ch))
    return 0; 

  /* this is just so that they expire after a while */
  if(number(0,15) == 0)
    ch->char_specials.timer--;

  if(ch->char_specials.timer == 0) {
    act("$n appears to fall apart as the life drains out of $m.",FALSE,ch,0,0,TO_ROOM);
    act("You return to your peaceful, lifeless slumber...",FALSE,ch,0,0,TO_CHAR);
    raw_kill(ch,NULL);
    return 1;
    }

  for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    if((vict != ch) && !(number(0,3))) 
      break;
    }
  if(vict == NULL)
    return 0; 

  switch(number(0,60)) {
    case 0: send_to_char("You groan loudly.",ch);
	    act("$n groans loudly.",FALSE,ch,0,0,TO_ROOM);
	    return 1;
    case 1: send_to_char("You start to drool.\r\nYou mumble, 'brains...', hungrily.",ch);
	    act("$n starts to drool.",FALSE,ch,0,0,TO_ROOM);
	    act("$n mumbles, 'brains...', hungrily.",FALSE,ch,0,0,TO_ROOM);
	    return 1;
    case 2: act("You study $N's skull.",FALSE,ch,0,vict,TO_CHAR);
	    act("$n taps on $N's skull.",FALSE,ch,0,vict,TO_NOTVICT);
	    act("$n taps on your skull.",FALSE,ch,0,vict,TO_VICT);
	    do_say(ch,"Brains?",0,0);
	    return 1;
    case 3: act("You drool all over $N.",FALSE,ch,0,vict,TO_CHAR);
	    act("$n drools all over $n.",FALSE,ch,0,vict,TO_NOTVICT);
	    act("$n drools all over you.",FALSE,ch,0,vict,TO_VICT);
	    return 1;
    default: return 0;
  }
}

SPECIAL(disposable)
{
  if(cmd || FIGHTING(ch))
    return 0;

  /* this is just so that they expire after a while */
  if(number(0,5) == 0)
    ch->char_specials.timer--;

  if(ch->char_specials.timer == 0)
  {
    act("$n suddenly pulls out a knife and mumbles about the hardship of war.",FALSE,ch,0,0,TO_ROOM);
    act("$n lets out a mournful sigh.  And places the knife in $s heart. R.I.P.",FALSE,ch,0,0,TO_ROOM);
    act("You see the life flash before your eyes and it wasn't a pretty one. R.I.P!",FALSE,ch,0,0,TO_CHAR);
    extract_char(ch);
  }
  return 0;
} 

/* spec proc to make a mob throw players out of the room - Vader */
SPECIAL(thrower)
{
  struct char_data *vict;
  int dir,room;

  if(cmd)
    return 0;
  if(!FIGHTING(ch))
    return 0;

  if(number(0,20) == 1) 
  {
    vict = FIGHTING(ch);
    dir = number(NORTH,DOWN);

    if(!CAN_GO(vict,dir)) // Can't throw them out.
      return 0;

    stop_fighting(ch);
    stop_fighting(vict);
    act("$n grabs $N by the neck and throws $M out of the room!",
	FALSE,ch,0,vict,TO_NOTVICT);
    act("You grab $N by the neck and throw $M out of the room!",
	FALSE,ch,0,vict,TO_CHAR);
    act("$n grabs you by the neck and throws you out of the room!",
	FALSE,ch,0,vict,TO_VICT);
    room = EXIT(ch,dir)->to_room;
    char_from_room(vict);
    char_to_room(vict,room);
    act("$n flies into the room and lands with a thud.",
	FALSE,vict,0,0,TO_ROOM);
    look_at_room(vict,0);
    send_to_char("\r\nYou hear something crack as you land...\r\n",vict);
    damage(ch, vict, MIN(GET_HIT(vict), dice(5,10)+GET_LEVEL(vict)), 
	   TYPE_UNDEFINED, FALSE);
    return 1;
  }
  return 0;
}

// DM: not bothering with guards - if this is implemented update guild_info in
// class.c
/*
SPECIAL(guild_guard)
{
  int i;
  extern int guild_info[][3];
  struct char_data *guard = (struct char_data *) me;
  char *buf = "The guard humiliates you, and blocks your way.\r\n";
  char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  for (i = 0; guild_info[i][0] != -1; i++) {
    if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
	world[ch->in_room].number == guild_info[i][1] &&
	cmd == guild_info[i][2]) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  return FALSE;
}
*/

SPECIAL(bounty_hunter)
{
  struct char_data *hunter = (struct char_data *) me;
  struct char_data *hunted = NULL;
  int dir;
  ACMD(do_look);

  if(CMD_IS("look"))
  {
    if(one_argument(argument,buf))
      if(generic_find_char(ch,buf,FIND_CHAR_ROOM) == hunter)
      {
        do_look(ch,buf,0,SCMD_LOOK);
        if(number(0,2))
	{
          act("\r\n$n looks at you.",TRUE,hunter,0,ch,TO_VICT);
          act("$n looks at $N.",TRUE,hunter,0,ch,TO_NOTVICT);
        }
        return TRUE;
      }
    return FALSE;
  }

  if (!hunter->player.title)
    return FALSE;
  if (!hunted) 
    if (!(hunted = generic_find_char(hunter,hunter->player.title,FIND_CHAR_ROOM | FIND_CHAR_WORLD))) 
      return FALSE;
  if (cmd)
    return FALSE;
  if (GET_POS(hunter)==POS_FIGHTING)
    return magic_user(ch,me,cmd,arg);
  else
  {
    if (number(0,30)>hunter->player.level)
      return FALSE;
    dir=find_first_step(hunter, hunter->in_room,hunted->in_room);
    if(dir==BFS_ALREADY_THERE)
    {
      hit(hunter,hunted,TYPE_UNDEFINED);
      return FALSE;
    } else if ((dir==BFS_NO_PATH)||(dir==BFS_ERROR))
      return FALSE;
    perform_move(hunter,dir,0);
    return FALSE;
  }
}


SPECIAL(assasin)
{
  struct char_data *hunter = (struct char_data *) me;
  struct char_data *victim = NULL;
  struct char_data *master = NULL;
  int dir;
  char buf2[200],buf3[200];
  char *victim_name,*master_name;
  int cost;
  int base_cost=10000;
  ACMD(do_look);

  if(CMD_IS("look"))
  {
    if (one_argument(argument,buf))
      if (generic_find_char(ch,buf,FIND_CHAR_ROOM) == hunter)
      {
        do_look(ch,buf,0,SCMD_LOOK);
        if(number(0,2))
	{
          act("\r\n$n looks at you.",TRUE,hunter,0,ch,TO_VICT);
          act("$n looks at $N.",TRUE,hunter,0,ch,TO_NOTVICT);
        }
        return TRUE;
      }
    return FALSE;
  }

  /* retrieve master and victim info */
  master_name=NULL;
  victim_name=NULL;
  if (hunter->player.title)
  {
    sscanf(hunter->player.title,"%s %s",buf2,buf3);
    victim_name=strdup(buf2);
    master_name=strdup(buf3);
    victim = get_player_online(ch,victim_name,FIND_CHAR_WORLD);  
    master = get_player_online(ch,master_name,FIND_CHAR_WORLD);
    if (!master || !victim)
    { 
      if (GET_TITLE(hunter) != NULL)
        free(GET_TITLE(hunter));
      GET_TITLE(hunter) = str_dup("\0");

      /* check to see if master has pissed off */
      if (victim && !master)
        if (FIGHTING(hunter)==victim)
        {
          stop_fighting(hunter);
          stop_fighting(victim);
          send_to_char("The assassin stops fighting you, his master has abandoned him.\n",victim);
          return FALSE;
        }  

      /* note: tried to free, but crashed */
      master=NULL;
      victim=NULL;
    }
  }

  if(CMD_IS("assassinate")) 
  {
    one_argument(argument,buf);
  
    /* DM - ensure ch is PC */
    if (IS_NPC(ch))
    {
      /* Check mobs master */
      if (ch->master) 
        send_to_char("You've got a mouth, two feet and two arms, use them you wuss.\r\n",ch->master);
      else
        send_to_char("Getting smart now are we?\r\n",ch);
       
      return TRUE;
    }

    /* no arg, no master */
    if (buf[0]==0 && !master)
    {
      send_to_char("Assassinate who?\n",ch);
      return TRUE;
    }
    /* assasin busy, issue not victim */
    if (master && victim)
    {  
      if (victim!=ch)
      {
        send_to_char("This assassin is busy!\n",ch);       
        return TRUE; 
      }
    }     
    /* no master and arg, check player is on and visble */     
    if (!master)
    {
      if (!(victim = get_player_online(ch, buf, FIND_CHAR_WORLD)) ||
	  (GET_INVIS_LEV(victim) > 0))
      { // Victim is not online, or is imm invis.
	send_to_char("That player is not around.\r\n",ch);
	return TRUE;
      }
      if (!LR_FAIL(ch, LVL_IS_GOD))
      { // Prevent assassinating IMMS.
	send_to_char("What, you think I can slay a god?!?\r\n", ch);
	return TRUE;
      }
      if (get_world(victim->in_room) != get_world(ch->in_room))
      { // Different Worlds...
	send_to_char("That player is in another world!\r\n", ch);
	return TRUE;
      }
      if (!CAN_SEE(ch, victim))
      { // Can we see the victim?
	send_to_char("That player does not appear to be around.\r\n", ch);
	return TRUE;
      }
    }
    /* cant kill myself! */
    if (victim==ch && buf[0]!=0 && !master)
    {
      send_to_char("Go and slit your own throat!\n",ch);       
      return TRUE;
    }
    /* check gold cost is base_cost*level for initial hire, and that again every hire*/
    base_cost *= GET_LEVEL(victim);
    cost=0;
    cost = GET_GOLD(hunter); /* re-apply original cost */
    if (!cost)
      cost= base_cost;
    if (GET_GOLD(ch)<cost)
    {
      sprintf(buf2,"The Assasin costs %d gold!\n",cost);       
      send_to_char(buf2,ch);       
      return TRUE;
    }    
    /* set hitroll and damroll  */
    GET_HITROLL(hunter)=GET_DAMROLL(hunter)=GET_LEVEL(victim);
    /* EXP */
    GET_EXP(hunter)=10000*GET_LEVEL(victim);
    /* set hitpoints */
    GET_HIT(hunter)=GET_MAX_HIT(hunter)=GET_MAX_HIT(victim);  
    sprintf(buf2,"The assassin takes %d gold from you.\n",cost);       
    send_to_char(buf2,ch);
      GET_GOLD(ch)-=cost;       
    if (!GET_GOLD(hunter))
      GET_GOLD(hunter)=base_cost;
    /* set title of assasin to victim/master */
    if (!master)
    {
      sprintf(buf2,"%s %s",buf,ch->player.name);
      set_title(hunter,buf2);
    }
    else
    {
      sprintf(buf2,"%s %s",master->player.name,ch->player.name);
      set_title(hunter,buf2);
      if (FIGHTING(hunter)==ch)
      {
        stop_fighting(hunter);
        stop_fighting(ch);
        send_to_char("The assassin stops fighting you and hunts your enemy.\n",ch);
      }  
    }
    return TRUE;
  }

  if(!victim) return FALSE;
  if(cmd) return FALSE;
    dir=find_first_step(hunter, hunter->in_room,victim->in_room);
    if(dir==BFS_ALREADY_THERE) {   
      hit(hunter,victim,TYPE_UNDEFINED);
      send_to_char("A divine hint- type 'assassinate' to end fight\n",victim);
      return FALSE;
    } else if ((dir==BFS_NO_PATH)||(dir==BFS_ERROR)) return FALSE;
    perform_move(hunter,dir,0);
    return FALSE; 
}

#if 0
SPECIAL(virus)
{
  struct char_data *mob;
  struct char_data *cons;
  struct char_data *virus = (struct char_data *) me;
  struct char_data *infected = NULL;
  int dir;

  /* OK! this is tough, too many commands to check fpr offensive action, so
     we will start by trapping ALL spells in the same room */
  if(CMD_IS("cast"))
  {
    send_to_char("The virus interferes with your casting!\n",ch);      
    return TRUE;
  }

  /* now we have to trap ALL offensive commands to the virus */
  if(CMD_IS("hit") || CMD_IS("kill") || CMD_IS("cast") || CMD_IS("kick") || CMD_IS("murder") 
                   || CMD_IS("backstab") || CMD_IS("bash")|| CMD_IS("steal")) 
  {
    one_argument(argument,buf);
    if(get_char_room_vis(ch,buf) == virus) 
    {
      send_to_char("The virus is too small!\n",ch);      
      return TRUE;
    }
  }
  /* multiply */
  if (!number(0,30))
  {
    /* load the mob*/
    mob = read_mobile(111, VIRTUAL);
    char_to_room(mob, virus->in_room);
    act("$n multiplies", FALSE, virus, 0, 0, TO_ROOM);
  }

  if(!virus->player.title) 
  {
    /* pick first player in room */
    for (cons = world[virus->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_ISNOT_GOD)  && (!number(0, 
20))) 
    {
      set_title(virus,cons->player.name);
      send_to_char("You have been infected\n",cons);
    }
  }
	
  /* JA fix for null pointer in virus->player.title */
  if (!virus->player.title)
    return(FALSE);

  if(!infected) if(!(infected = get_char_vis(virus,virus->player.title,FALSE)))     return FALSE;
  if(cmd) return FALSE;

  dir=find_first_step(virus, virus->in_room,infected->in_room);
  if(dir==BFS_ALREADY_THERE && GET_POS(infected) != POS_SLEEPING) 
  {
    send_to_char("You are infected! You feel sick. Only sleep can save you!\n",infected);
    GET_HIT(infected) -=2;
    /*damage(infected,infected,2,TYPE_SUFFERING);*/
    if (GET_HIT(infected)<0)
    {
      GET_HIT(infected)=0;
      die(virus,NULL);
    }
    update_pos(infected);
    return FALSE;
  } else 
    if ((dir==BFS_NO_PATH)||(dir==BFS_ERROR)) return FALSE;
  perform_move(virus,dir,0);
  return FALSE;  
}    

SPECIAL(bacteria)
{
  struct char_data *bacteria = (struct char_data *) me;
  struct char_data *hunted = NULL;
	struct char_data *mob;
  int dir;

  /* OK! this is tough, too many commands to check fpr offensive action, so
     we will start by trapping ALL spells in the same room */
  if(CMD_IS("cast"))
  {
    send_to_char("The bacteria interferes with your casting!\n",ch);      
    return TRUE;
  }
  /* now we have to trap ALL offensive commands to the bacteria */
  if(CMD_IS("hit") || CMD_IS("kill") || CMD_IS("cast") || CMD_IS("kick") || CMD_IS("murder") 
                   || CMD_IS("backstab") || CMD_IS("bash")|| CMD_IS("steal")) 
  {
    one_argument(argument,buf);
    if(get_char_room_vis(ch,buf) == bacteria) 
    {
      send_to_char("The bacteria is too small!\n",ch);      
      return TRUE;
    }
  }

  /* multiply */

  if (!number(0,500))
  {
    mob = read_mobile(112, VIRTUAL);
    char_to_room(mob, bacteria->in_room);
    act("$n multiplies", FALSE, bacteria, 0, 0, TO_ROOM);
  }

  /* hunt the virus */
	
	if (number(0, 3))
		return(FALSE);

  if(!hunted) if(!(hunted = get_char_vis(bacteria,"virus",FALSE))) return FALSE;
  if(cmd) return FALSE;
  dir=find_first_step(bacteria, bacteria->in_room,hunted->in_room);
  if(dir==BFS_ALREADY_THERE) 
  {
    hit(bacteria,hunted,TYPE_UNDEFINED);
    return FALSE;
  } else 
    if ((dir==BFS_NO_PATH)||(dir==BFS_ERROR)) return FALSE;
  perform_move(bacteria,dir,0);
  return FALSE;  
}    

SPECIAL(corridor_guard)
{
  struct char_data *guard = (struct char_data *) me;

  char *buf = "The guard humiliates you, and blocks your way.\r\n";
  char *buf2 = "The guard humiliates $n, and blocks $s way.";
  ACMD(do_say);

  if(GET_POS(guard)==POS_FIGHTING) return magic_user(ch,me,cmd,arg);
  if ((cmd != SCMD_NORTH) || (!CAN_GO(ch,NORTH)) || (ch==guard)) 
    return FALSE;
/* if character does not have 50000 gold, turn them away...:) */
/* JA 1/1/95 : Stop everyone from using the corridor they have to
   kill him to get thru, I never liked the corridor anyway */

      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      do_say(guard,"No body goes through here now, be off with you!!",0,0);
      return TRUE;

/*
  if (GET_GOLD(ch) < 50000) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      do_say(guard,"You need to have 50,000gp to use this Corridor!",0,0);
      return TRUE;
  }
*/

  GET_GOLD(ch) -= 50000;
  return FALSE;
}

SPECIAL(recep_guard)
{
  int load_room;
  /*extern void newbie_equip(struct char_data *ch);*/
  char *buf = "The guard says 'Sorry but this world is still under construction'\r\n";
  char *buf2 = "The guard informs $n that this world is under construction.";
  char mestowld[MAX_STRING_LENGTH];
  
  if (!IS_MOVE(cmd) || IS_NPC(ch)) return FALSE;
/*
  if ((GET_LEVEL(ch) < 5) && (world[ch->in_room].number == 503))
    return(FALSE);
*/
  sprintf(mestowld,"%s has entered the world.",GET_NAME(ch));

  /* next line meant to make sure players casting recall in westworldgoto right room - bill*/
  ENTRY_ROOM(ch,WORLD_WEST) = world_start_room[WORLD_WEST];


  if ((world[ch->in_room].number ==  503) && (cmd == SCMD_WEST))
  {
    send_to_char(buf, ch);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }
  else if((world[ch->in_room].number == 505) && (cmd == SCMD_EAST) 
          && !LR_FAIL(ch, 5))
  {
    /*if((GET_EXP(ch)==0)&&(!ch->carrying))
      newbie_equip(ch);*/
    if((load_room = real_room(ENTRY_ROOM(ch,WORLD_FUTURE))) < 0)
      load_room = real_room(ENTRY_ROOM(ch,WORLD_FUTURE) =
          world_start_room[WORLD_FUTURE]);
    char_from_room(ch);
    char_to_room(ch,load_room);
    send_to_char("The guard hands you your equipment as you enter.\r\n",ch);
    act(mestowld,FALSE,ch,0,0,TO_ROOM);
    look_at_room(ch,0);
    if(Crash_load(ch) == 2) {
      send_to_char("You could not afford rent here!\r\n"
                   "Your possessions have been donated to the Salvation Army!\r\n",ch);
    }
    return TRUE;
  }
  else if((world[ch->in_room].number == 506) && (cmd == SCMD_SOUTH))
  {
    /*if((GET_EXP(ch)==0)&&(!ch->carrying))
      newbie_equip(ch);*/
    if((load_room = real_room(ENTRY_ROOM(ch,WORLD_MEDIEVAL))) < 0)
      load_room = real_room(ENTRY_ROOM(ch,WORLD_MEDIEVAL) = 
          world_start_room[WORLD_MEDIEVAL]);
    char_from_room(ch);
    char_to_room(ch,load_room);
    send_to_char("The guard hands you your equipment as you enter.\r\n",ch);
    act(mestowld,FALSE,ch,0,0,TO_ROOM);
    look_at_room(ch,0);
    if(Crash_load(ch) == 2) {
      send_to_char("You could not afford rent here!\r\n"
                   "Your possessions have been donated to the Salvation Army!\r\n",ch);
    }
    return TRUE;
  }
  return FALSE;
}
#endif



/* JA : we don't need this one anymnore */
/* SPECIAL(puff)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 60)) {
  case 0:
    do_say(ch, "My god!  It's full of stars!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "How'd all those fish get up here?", 0, 0);
    return (1);
  case 2:
    do_say(ch, "I'm a very female dragon.", 0, 0);
    return (1);
  case 3:
    do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
    return (1);
  default:
    return (0);
  }
}
*/

SPECIAL(citizen)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 120))
  {
    case 0: case 1: case 2: case 3:
      do_say(ch, "It's a fine day isn't it?", 0, 0);
      return (1);
    case 4: case 5: case 6:
      do_say(ch, "Have you been to Rome lately?", 0, 0);
      return (1);
    case 7:
      do_say(ch, "I hear the brigands are holed up in the moathouse!", 0, 0);
      return (1);
    case 8: case 9:
      do_say(ch, "Its a good day to DIE isn't it!!!", 0, 0);
      return (1);
    case 10: case 11:
      do_say(ch, "Welcome to the free city of Haven!", 0, 0);
      return (1);
    case 12: case 13:
      do_say(ch, "Have you heard the rumours about the Werewolf of Haven?",0,0);
      return (1);
    default:
      return (0);
  }
}

SPECIAL(delenn)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2:  case 3: case 4:  
	  do_say(ch, "Summoned I come. In Valenn's name I take the place that has been prepared for me", 0, 0);
	  return(1);
  case 5: case 6: case 7: case 8: 
	  do_say(ch, "I am gray, I stand between the candle and the star.", 0, 0);
	  return(1);  
   case 9: case 10: case 11: case 12: case 13:
    do_say(ch, "We are gray, we stand between the darkness and the light.", 0, 0);
    return (1);
  default:
    return (0);
  }
}

SPECIAL(oompa)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4: case 5: case 6:
    do_say(ch, "Oompa loompa doopedy dooooo....", 0, 0);
    return (1);
  case 7: case 8: case 9: case 10: case 11: case 12: case 13:
    do_say(ch, "I've got little puzzle for you....", 0, 0);
    return (1);
  default:
    return (0);
  }
}

SPECIAL(tin_man)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4: case 5: case 6:
  case 7: case 8: case 9: case 10: case 11: case 12: case 13:
    do_say(ch, "If I ONLY had a heart.. SIGH", 0, 0);
    return (1);
  default:
    return (0);
  }
}

SPECIAL(scarecrow)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4: case 5: case 6:
  case 7: case 8: case 9: case 10: case 11: case 12: case 13:
    do_say(ch, "If I ONLY had a brain... SIGH", 0, 0);
    return (1);
  default:
    return (0);
  }
}

SPECIAL(c_lion)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4: case 5: case 6:
  case 7: case 8: case 9: case 10: case 11: case 12: case 13:
    do_say(ch, "If I ONLY had da nerve... WIMPER", 0, 0);
    return (1);
  default:
    return (0);
  }
}

SPECIAL(giant)
{
  ACMD(do_say);
  ACMD(do_gen_comm);

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4: case 5: case 6:
  case 7: case 8: case 9:
    do_say(ch, "FE FI FO FUM I SMELL THE BLOOD OF AN ENGLISHMAN!!!!!!..", 0, 0);
    return (1);
  case 10: case 11: case 12: case 13:
    do_gen_comm(ch, "FE FI FO FUM I SMELL THE BLOOD OF AN ENGLISHMAN!!!!!!..", 0, SCMD_SHOUT);
    return(1);
  default:
    return (0);
  }
}

SPECIAL(sleepy)
{

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4:
	act("$n falls asleep on $s feet.  AMAZING!.",FALSE,ch,0,0,TO_ROOM);
	return(1);
  case 5: case 6: case 7: case 8: case 9:
	act("$n Yawns really loudly.  What lovely teeth he has.",FALSE,ch,0,0,TO_ROOM);
    return (1);
  case 10: case 11: case 12: case 13:
	act("$n rubs $s eyes vigorously, he looks like he can fall asleep right here.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  default:
    return (0);
  }
}
SPECIAL(sneazy)
{

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4:
        act("$n sneezes really loudly.",FALSE,ch,0,0,TO_ROOM);
	return(1);
  case 5: case 6: case 7: case 8: case 9:
        act("$n sneezes all over your clothes.  YUCK!.",FALSE,ch,0,0,TO_ROOM); 

    return (1);
  case 10: case 11: case 12: case 13:
        act("$n rubs $s nose with the back of his hand.  YUCK!!",FALSE,ch,0,0,TO_ROOM);
    return(1);
  default:
    return (0);
  }
}

SPECIAL(grumpy)
{

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4:
        act("$n mumbles something about having to do all the work.",FALSE,ch,0,0,TO_ROOM);
	return(1);
  case 5: case 6: case 7: case 8: case 9:
        act("$n mumbles Snow White my ar#&.  Stupid girl eating that damn apple.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  case 10: case 11: case 12: case 13:
        act("$n curses the weather.  #&&CKEN heat, I am sweating like a pig!.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  default:
    return (0);
  }
}
SPECIAL(bashful)
{

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4:
        act("$n blushes furiously.",FALSE,ch,0,0,TO_ROOM);
	return(1);
  case 5: case 6: case 7: case 8: case 9:
        act("$n kicks at some rocks looking abashed.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  case 10: case 11: case 12: case 13:
        act("$n's face goes all red.  If he blushed any harder his head would explode.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  default:
    return (0);
  }
}
SPECIAL(lazy)
{

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4:
        act("$n lazes around in the sun.",FALSE,ch,0,0,TO_ROOM);
	return(1);
  case 5: case 6: case 7: case 8: case 9:
        act("$n pretends to be busy.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  case 10: case 11: case 12: case 13:
        act("$n looks around with shifty eyes and pulls out a joint.",FALSE,ch,0,0,TO_ROOM);
	act("A cloud of smoke envelops you!!.",FALSE,ch,0,0,TO_ROOM);
	act("You suddenly feel light headed..Must be good stuff!!.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  default:
    return (0);
  }
}
SPECIAL(doc)
{

  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4:
        act("$n looks at his watch with a worried grimace.",FALSE,ch,0,0,TO_ROOM);
	return(1);
  case 5: case 6: case 7: case 8: case 9:
        act("$n wonders if Snow White is all right.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  case 10: case 11: case 12: case 13:
        act("$n examines his tools to make sure they are in good nick.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  default:
    return (0);
  }
}

SPECIAL(wind_summon)
{
  struct char_data *vict;
  int to_room;
  //extern int top_of_world;

  ACMD(do_say);
  if (cmd)
    return(0);

  if(!FIGHTING(ch))
    return(0);

  switch(number(0,20))
  {
    case 0:
    case 1:
    case 2:
      do_say(ch, "Now you DIE insolent mortals!.",0,0);
      return(1);
    case 3:
    case 4:
      do_say(ch, "You weak pathetic FOOLS!",0,0);
      return(1);
    case 5:
      do_say(ch, "What you are still here?!",0,0);
      return(1);		
    case 6:
    case 7:
    case 8:
      vict = FIGHTING(ch);
      act("$n summons the elements of wind.",FALSE,ch,0,0,TO_ROOM);
      act("$N is picked up by a whirlwind and rudely thrown out of the area.", FALSE,ch,0,vict,TO_NOTVICT);
      send_to_char("You are picked up by a whirlwind and rudely thrown out of the fight.", vict);
      do 
      {
	to_room = number(0, top_of_world);
      } while ((IS_SET(world[to_room].room_flags, ROOM_PRIVATE | ROOM_DEATH)) ||
	       (world[to_room].zone != world[ch->in_room].zone));
      stop_fighting(vict);
      stop_fighting(ch);
      char_from_room(vict);
      char_to_room(vict,to_room);
      act("$n is rudely deposited in the room by a whirlwind.", FALSE, vict, 0,
	  0, TO_ROOM);
      send_to_char("\r\nYou are rudely deposited by a whirlwind.  We are not in Kansas anymore, Toto!\r\n\r\n", vict);
      look_at_room(vict, 0);
      return(1);
    default:
      return(1);
  }
  return(0);
}
			
SPECIAL(banish)
{
  struct char_data *vict;
  int to_room;

  if (cmd)
    return(0);

  if(!FIGHTING(ch))
    return(0);

  vict = FIGHTING(ch);

  switch(number(0,50))
  {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      act("$n raises $s hand to the sky and summons the elements!!.", FALSE, ch, 0, 0, TO_ROOM);
      act("$n is struck by a mighty LIGHTNINGBOLT. $n's body wiggles and giggles. <CUTE>", FALSE, vict, 0, 0, TO_NOTVICT);
      send_to_char("You are hit by a mighty bolt of electricity. FU(K that HURT!\r\n", vict);
      if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL))
	break;
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*10), TYPE_UNDEFINED,
	     FALSE);
      break;
    case 45:
      to_room= number(4911,4917);
      act("$n shouts 'YOU INSOLENT MORTAL BEGONE!!!!'",FALSE,ch,0,0,TO_ROOM);
      act("$n's body wavers for a brief moment and suddenly they are no longer there.",FALSE,vict,0,0,TO_ROOM);
      act("$n stares at you.  And unleashes $s cosmic powers!",FALSE,ch,0,vict,TO_VICT);
      stop_fighting(vict);
      stop_fighting(ch);
      char_from_room(vict);
      char_to_room(vict,real_room(to_room));
      act("$n suddenly appears from thin air!",FALSE,vict,0,0,TO_ROOM);
      look_at_room(vict, 0);
      send_to_char("You shiver suddenly. A strange feeling comes over you. When your vision clears.\r\nYou realise you are in HELL!!!!.\r\n",vict);
      break;
    default: return(0);
  }
  return(0);
}

SPECIAL(dopy)
{
 ACMD(do_say);
  if (cmd)
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3: case 4:
        act("$n trips over $s own feet.  What a dope!.",FALSE,ch,0,0,TO_ROOM);
	return(1);
  case 5: case 6: case 7: case 8: case 9:
        act("$n looks around with a vacant stare.",FALSE,ch,0,0,TO_ROOM);
    return(1);
  case 10: case 11: case 12: case 13:
        do_say(ch,"1+1=3 I am soo smart.",0,0);
    return(1);
  default:
    return (0);
  }
}
SPECIAL(construction_worker)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  if (!AWAKE(ch))
    return (0);

  switch (number(0, 120)) {
  case 0: case 1: case 2: case 3:
    do_say(ch, "Have you heard about the things they're doing downstairs?",0,0);
    return (1);
  case 4: case 5: case 6:
    do_say(ch, "Apparently they are trying to summon the devil!", 0, 0);
    return (1);
  case 7:
    do_say(ch, "Whoa, they want to summon daemons to take out the gods!", 0, 0);
    return (1);
  case 8: case 9: case 10: case 11:
    do_say(ch, "Make sure you come back soon, something wicked is underway!",
	   0, 0);
    return (1);
  default:
    return (0);
  }
}

#if 0
SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for (temp = i->contains; temp; temp = next_obj) {
	next_obj = temp->next_content;
	obj_from_obj(temp);
	obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
  }
  return (FALSE);
}
#endif

SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  if (number(0, 3) != 1)
    return(FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 500)
      continue;

    if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
      continue;

    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    /* obj_to_char(i, ch); */
    extract_obj(i);    /* janitors now remove items from the game */
    return TRUE;
  }

  return FALSE;
}


SPECIAL(cityguard)
{
  ACMD(do_gen_comm);
  struct char_data *tch, *evil;
  int max_evil;

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  max_evil = 1000;
  evil = 0;

  for(tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if(!IS_NPC(tch) && CAN_SEE(ch,tch) && affected_by_spell(tch,SPELL_CHANGED)) {
      sprintf(buf,"Holy SH!T!! %s is a %s!!!! Die fiend!!!!",GET_NAME(tch),
              (PRF_FLAGGED(tch,PRF_WOLF) ? "WEREWOLF" : "VAMPIRE"));
      do_gen_comm(ch,buf,0,SCMD_SHOUT);
      hit(ch,tch,TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_KILLER)) {
      do_gen_comm(ch,"HEY!!!  You're one of those PLAYER KILLERS!!!!!!",0,SCMD_SHOUT);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_THIEF)){
      do_gen_comm(ch,"HEY!!!  You're one of those PLAYER THIEVES!!!!!!",0,SCMD_SHOUT);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
	  (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
	max_evil = GET_ALIGNMENT(tch);
	evil = tch;
      }
    }
  }

  if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
    do_gen_comm(ch,"PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!",0,SCMD_SHOUT);
    hit(ch, evil, TYPE_UNDEFINED);
    return (TRUE);
  }
  return (FALSE);
}

/* spec proc for Jabba the Hutt
 * this will either dump an attacker down into a pit with a monster or
 * dump them in the desert with some of their stuff removed... - VADER
 */
SPECIAL(Jabba)
{
  struct char_data *vict;

  if(cmd || GET_POS(ch) != POS_FIGHTING)
    return(FALSE);

  vict = FIGHTING(ch);
  switch(number(0,2)) 
  {
    case 0: 
      act("$n reaches over and pulls a lever on the wall.",FALSE,ch,0,0,TO_ROOM);
      act("$N gasps as $E falls through the floor.",FALSE,ch,0,vict,TO_NOTVICT);
      act("You gasp as the floor below you seems to disappear.",FALSE,ch,0,vict,TO_VICT);
      act("You fall!\n\r",FALSE,vict,0,0,TO_CHAR);
      char_from_room(vict);
      char_to_room(vict,real_room(26021));
      GET_POS(vict) = POS_SITTING;
      look_at_room(vict,0);
      break;
    case 1: 
      act("$n bellows 'GarfnaRR naRg jushta!!'",FALSE,ch,0,0,TO_ROOM);
      act("Two guards arrive and drag $N away.",FALSE,ch,0,vict,TO_NOTVICT);
      act("Two guards arrive and roughly drag you away!",FALSE,ch,0,vict,TO_VICT);
      act("The guards beat you up and dump you out in the desert.\n\r",FALSE,vict,0,0,TO_CHAR);
      char_from_room(vict);
      char_to_room(vict,real_room(24000+number(0,40)));
      act("$n laughs evilly.",FALSE,ch,0,vict,TO_ROOM);
      GET_POS(vict) = POS_SITTING; 
      look_at_room(vict,0);
      damage(ch, vict, (int)(GET_HIT(vict)/2), TYPE_UNDEFINED, FALSE);
      break;
  }
  return(FALSE);
}

/* Blink Demon spec proc for Drax's zone.. makes him teleport away
 * during a fight... - VADER
 * NOTE: i was gunna just use the teleport spell but it isnt suitable
 * cos drax doesnt want him going to a !mob room and ya cant cast teleport
 * while fighting (not that thats a real prob..) sorry this isnt very
 * generic.. itll only work in the moander (?) zone...
 *
 * Artus> Maybe one day if i can be fucked i'll make it more generic :o) - TODO
 */
SPECIAL(blink_demon)
{
  int room;

  if(cmd || GET_POS(ch) != POS_FIGHTING)
    return(FALSE);

  room = 1800+number(9,96);
  if(IS_SET(ROOM_FLAGS(real_room(room)), ROOM_NOMOB))
    return(FALSE);

  if(number(0, (GET_HIT(ch)/100)+2) == 1) { /* chances increase as he gets weaker */
  /*  stop_fighting(FIGHTING(ch));
    stop_fighting(ch);
 */
    act("$n blinks out of existence!\r\n",FALSE,ch,0,0,TO_ROOM);
    char_from_room(ch);
    char_to_room(ch,real_room(room));
    act("$n blinks into existence!\r\n",TRUE,ch,0,0,TO_ROOM);
    return(TRUE);
  }
  return(FALSE);
}

/* Beholder's spec proc for Moander zone.. fakes casting spells in a !magic
 * room.. this is pretty nasty...
 */
SPECIAL(beholder)
{
  struct char_data *vict;

  if(cmd || GET_POS(ch) != POS_FIGHTING)
    return(FALSE);


  /* choose someone fighting him as the vict.. doesnt hafta be the tank */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && number(0, 5) == 1)
      break;
  /* the above can leave it with no victim which causes a crash so...*/
  if(vict == NULL)
    vict = FIGHTING(ch);

  if (!LR_FAIL(ch, LVL_ANGEL+1))
    return FALSE;

  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL))
    return FALSE;

  switch(number(0,30)) {
    case 1:
    case 2:
    case 3: 
      act("$n stares at $N and utters the word, 'harm'.",FALSE,ch,0,vict,TO_NOTVICT);
      act("$n stares at you and utters the word, 'harm'.",FALSE,ch,0,vict,TO_VICT);
      act("A blast of pain shoots through you!",FALSE,ch,0,vict,TO_VICT);
      act("$N screams in pain!",FALSE,ch,0,vict,TO_NOTVICT);
      damage(ch, vict, dice(8,8)+8, TYPE_UNDEFINED, FALSE);
      return(TRUE);
      break;
    case 4:
    case 5: 
      act("$n stares at $N and utters the word, 'Disintergrate!'",FALSE,ch,0,vict,TO_NOTVICT);
      act("$n stares at you and utters the word, 'Disintergrate!'",FALSE,ch,0,vict,TO_VICT);
      act("You feel your molecules vibrate!",FALSE,ch,0,vict,TO_VICT);
      act("$N glows green as $S molecules vibrate!",FALSE,ch,0,vict,TO_NOTVICT);
      damage(ch, vict, dice(GET_LEVEL(vict),4)+GET_LEVEL(vict), TYPE_UNDEFINED,
	     FALSE);
      return(TRUE);
      break;
    case 6: 
      act("$n stares at $N with $s good eye and yells 'DEATH!'",FALSE,ch,0,vict,TO_NOTVICT);
      act("$n stares at you with $s good eye and yells 'DEATH!'",FALSE,ch,0,vict,TO_VICT);
      act("Your life flashes before your eyes!",FALSE,ch,0,vict,TO_VICT);
      act("$n laughs as $N falls to $S knees in agony!",FALSE,ch,0,vict,TO_NOTVICT);
      if(GET_HIT(vict) > 35) /* dont wanna risk increasing em if they are low.. */
        damage(ch, vict, dice(3,10)+2, TYPE_UNDEFINED, FALSE);
      else
	damage(ch, vict, GET_HIT(vict)/2, TYPE_UNDEFINED, FALSE);
      return(TRUE);
      break;
    default:
      return(FALSE);
  }
}

#if 0
SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  int pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("list")) {
    send_to_char("Available pets are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      if (!IS_NPC(pet))
        continue;      
      sprintf(buf, "%8d - %s\r\n", 3 * GET_EXP(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);
 
    if (!(pet = get_char_room(buf, pet_room)) || !IS_NPC(pet)) {
      send_to_char("There is no such pet!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < (GET_EXP(pet) * 3)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= GET_EXP(pet) * 3;

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);
    load_mtrigger(pet);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("May you enjoy your pet.\r\n", ch);
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }
  /* All commands except list and buy */
  return 0;
}
#endif

SPECIAL(phoenix)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  vict = FIGHTING(ch);
  if (IS_NPC(vict) && (MOB_FLAGGED(vict, MOB_NOKILL)))
    return FALSE;

  switch(number(0,20))
  {
    case 1:
      act("The $n stretches $s wings and draws from the power of the sun.", FALSE, ch, 0, 0, TO_ROOM);
      act("The $n's wounds heal before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You draw power from the sun and manage to heal some of your wounds\r\n",ch);
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch)+GET_LEVEL(ch)*4);
      break;
    case 2:
      act("The $n lets out a loud screech and blazes into fire.", FALSE, ch, 0, 0, TO_ROOM);
      act("$n screams in pain as $e is burnt by the searing flames.", FALSE, vict, 0, 0, TO_ROOM);
      send_to_char("You scream in pain and your skin bubbles and melts off your body..OUCH\r\n",vict);
      damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*8), TYPE_UNDEFINED,
	     FALSE);
      handle_fireball(vict);
      break;
    default: 
      return 0;
  }
  return 0;
}

SPECIAL(aphrodite)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  vict=FIGHTING(ch);
  
  switch(GET_SEX(vict))
  { 
    case SEX_MALE:
      switch(number(0,20))
      {
	case 1:
	  act("$n looks seductively at $N and gives $M a killer kiss.", FALSE, ch, 0, vict, TO_NOTVICT);
	  act("$n gives you a killer kiss.  Your heart misses a beat or two.", FALSE, ch, 0, vict, TO_VICT);
	  damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*7), TYPE_UNDEFINED,
	         FALSE);
	  break;
	case 2:
	  act("$n looks seductively at $N and gives $M a monstrous hug.", FALSE, ch, 0, vict, TO_NOTVICT);
	  act("$n gives you a monstrous hug.  Every bone in your body breaks. OUCH!.",FALSE,  ch, 0, vict, TO_VICT);
	  damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*10), TYPE_UNDEFINED,
	         FALSE);
	  break;
	default: 
	  return 0;
      }
      break;
    case SEX_FEMALE:
      switch(number(0,20))
      {
	case 1:
	  act("$n gives $N a scornful look. $N's soul shivers uncomfortably.", FALSE, ch, 0, vict, TO_NOTVICT);
	  act("$n gives you a look that can kill.  What a bitch!!!", FALSE, ch, 0 , vict, TO_VICT);
	  damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*7), TYPE_UNDEFINED,
	         FALSE);
	  break;
	case 2:
	  act("$n slaps $N really hard.  Wow that must hurt!", FALSE, ch, 0 ,vict, TO_NOTVICT);
	  act("$n slaps you REALLY HARD.  You see some pretty stars.", FALSE, ch, 0 , vict, TO_VICT);
	  damage(ch, vict, MIN(GET_HIT(vict), GET_LEVEL(ch)*10), TYPE_UNDEFINED,
	         FALSE);
	  break;
	default: 
	  return 0;
      }
      break;
    default:
      switch(number(0,20))
      {
	case 1:
	  act("$n gives $N a scornful look. $N's soul shivers uncomfortably.", FALSE, ch, 0, vict, TO_NOTVICT);
	  act("$n gives you a look that can kill.  What a bitch!!!", FALSE, ch, 0 , vict, TO_VICT);
	  damage(ch, vict, MIN(GET_HIT(ch), GET_LEVEL(ch)*7), TYPE_UNDEFINED,
	         FALSE);
	  break;
	case 2:
	  act("$n slaps $N really hard.  Wow that must hurt!", FALSE, ch, 0 ,vict, TO_NOTVICT);
	  act("$n slaps you REALLY HARD.  You see some prity stars.", FALSE, ch, 0 , vict, TO_VICT);
	  damage(ch, vict, MIN(GET_HIT(ch), GET_LEVEL(ch)*10), TYPE_UNDEFINED,
	         FALSE);
	  break;
	default: 
	  return 0;
      }
  }
  return 0;
}


/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

SPECIAL(reward_obj)
{
#define MAX_REWARD_TYPE 7
  struct obj_data *obj = (struct obj_data *)me;
  char arg1[MAX_STRING_LENGTH] = "", arg2[MAX_STRING_LENGTH] = "";
  int i = 0;
  bool rewarded = FALSE;
  void apply_specials(struct char_data *ch, bool initial);

  if (!(cmd) || !(ch) || IS_NPC(ch) || !CMD_IS("reward"))
    return FALSE;

  if (!(*argument)) strcat(arg1, "list");
  skip_spaces(&argument);
  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "list"))
  {
    send_to_char("This reward entitles you to the following:\r\n", ch);
    for (i = 0; i <= 3; i++)
      switch (GET_OBJ_VAL(obj, i))
      {
	case 0: break; // No Reward
	case 1:
	  send_to_char("\r\nOne of the following remort abilities: \r\n"
		       "&c  Disguise  &n- Make yourself appear as mobs\r\n"
		       "&c  Empath    &n- Permanent Sense Wounds\r\n"
		       "&c  Fly       &n- Permanent Fly\r\n"
		       "&c  GrpSneak  &n- Permanent Group Sneak\r\n"
		       "&c  Healer    &n- Enhanced healing abilities\r\n"
		       "&c  Infra     &n- Permanent Infravision\r\n"
		       "&c  Invis     &n- Permanent Invisibility\r\n"
		       "&c  ManaThief &n- Drain mana while fighting\r\n"
		       "&c  Sneak     &n- Permanent Sneak\r\n"
		       "&c  Tracker   &n- Track through !TRACK areas\r\n"
		       "Syntax: reward special <one of the above>\r\n", ch);
	  break;
	case 2:
	  send_to_char("\r\n+1 Strength. (Syntax: reward str)\r\n", ch);
	  break;
	case 3:
	  send_to_char("\r\n+1 Intelligence. (Syntax: reward int)\r\n", ch);
	  break;
	case 4:
	  send_to_char("\r\n+1 Wisdom. (Syntax: reward wis)\r\n", ch);
	  break;
	case 5:
	  send_to_char("\r\n+1 Dexterity. (Syntax: reward dex)\r\n", ch);
	  break;
	case 6:
	  send_to_char("\r\n+1 Constitution. (Syntax: reward con)\r\n", ch);
	  break;
	case 7:
	  send_to_char("\r\n+1 Charisma. (Syntax: reward cha)\r\n", ch);
	  break;
	default:
	  sprintf(buf, "\r\n&Y%d&n gold coins. (Syntax: reward gold)\r\n", GET_OBJ_VAL(obj, i));
	  send_to_char(buf, ch);
	  break;
      }
    return 1;
  } // List
  if (is_abbrev(arg1, "special"))
  {
    if ((GET_OBJ_VAL(obj, 0) != 1) && (GET_OBJ_VAL(obj, 1) != 1) &&
	(GET_OBJ_VAL(obj, 2) != 1) && (GET_OBJ_VAL(obj, 3) != 1))
    {
      send_to_char("This reward cannot provide special abilities!\r\n", ch);
      return 1;
    }
    if (!(*arg2))
    {
      send_to_char("Which special did you wish to claim? Type 'reward list' for a list.\r\n", ch);
      return 1;
    }
    to_upper(arg2);
    if (!strcmp(arg2, "INVIS"))
    {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_INVIS))
      {
	send_to_char("You already have permanent invisibility!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_INVIS);
      send_to_char("Granted: Permanent invisibility.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "SNEAK")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_SNEAK))
      {
	send_to_char("You already have permanent sneak!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
      send_to_char("Granted: Permanent sneak.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "HEALER")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_HEALER))
      {
	send_to_char("You already have superior healing abilities!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_HEALER);
      send_to_char("Granted: Superior healing abilities.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "MANATHIEF")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_MANA_THIEF))
      {
	send_to_char("You already have the ability to steal mana!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_MANA_THIEF);
      send_to_char("Granted: Mana stealing abilities.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "DISGUISE")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_DISGUISE))
      {
	send_to_char("You already have the ability to disguise yourself!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_DISGUISE);
      send_to_char("Granted: The art of disguise.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "EMPATH")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_EMPATH))
      {
	send_to_char("You are already empathatic.\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_EMPATH);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "INFRA")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_INFRA))
      {
	send_to_char("You already have permanent infravision!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_INFRA);
      send_to_char("Granted: Permanent infravision.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "GRPSNEAK")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_GROUP_SNEAK))
      {
	send_to_char("You already have permanent group sneak!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_GROUP_SNEAK);
      send_to_char("Granted: Permanent group sneak.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "FLY")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_FLY))
      {
	send_to_char("You already have permanent fly!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_FLY);
      send_to_char("Granted: Permanent fly.\r\n", ch);
      rewarded = TRUE;
    } else if (!strcmp(arg2, "TRACKER")) {
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_TRACKER))
      {
	send_to_char("You already have enhanced tracking abilities!\r\n", ch);
	return 1;
      }
      SET_BIT(GET_SPECIALS(ch), SPECIAL_TRACKER);
      send_to_char("Granted: Enhanced tracking abilities.\r\n", ch);
      rewarded = TRUE;
    }
    if (!(rewarded))
    {
      send_to_char("Which special did you wish to claim? Type 'reward list' for a list.\r\n", ch);
      return 1;
    }
    apply_specials(ch, FALSE);
  } else if (is_abbrev(arg1, "gold")) {
    for (i = 0; i <= 3; i++)
      if (GET_OBJ_VAL(obj, i) > MAX_REWARD_TYPE)
      {
	sprintf(buf, "You receive &Y%d&n gold coins.\r\n", GET_OBJ_VAL(obj, i));
	send_to_char(buf, ch);
	GET_GOLD(ch) += GET_OBJ_VAL(obj, i);
	rewarded = TRUE;
	break;
      }
    if (!(rewarded))
    {
      send_to_char("This reward is not filled with gold, I'm afraid.\r\n", ch);
      return 1;
    }
  } else if (is_abbrev(arg1, "str")) {
    for (i = 0; i <= 3; i++)
      if (GET_OBJ_VAL(obj, i) == 2)
      {
	if (GET_REAL_STR(ch) >= max_stat_value(ch, STAT_STR))
	{
	  send_to_char("Your strength is already maxed out!\r\n", ch);
	  return 1;
	}
	GET_REAL_STR(ch)++;
	send_to_char("You feel your strength increase.\r\n", ch);
	rewarded = TRUE;
	break;
      }
    if (!(rewarded))
    {
      send_to_char("This reward is not filled with strength, I'm afraid.\r\n", ch);
      return 1;
    }
  } else if (is_abbrev(arg1, "int")) {
    for (i = 0; i <= 3; i++)
      if (GET_OBJ_VAL(obj, i) == 3)
      {
	if (GET_REAL_INT(ch) >= max_stat_value(ch, STAT_INT))
	{
	  send_to_char("You're as intelligent as you will ever be.\r\n", ch);
	  return 1;
	}
	GET_REAL_INT(ch)++;
	send_to_char("You feel smarter.\r\n", ch);
	rewarded = TRUE;
	break;
      }
    if (!(rewarded))
    {
      send_to_char("This reward is not filled with intelligence, I'm afraid.\r\n", ch);
      return 1;
    }
  } else if (is_abbrev(arg1, "wis")) {
    for (i = 0; i <= 3; i++)
      if (GET_OBJ_VAL(obj, i) == 4)
      {
	if (GET_REAL_WIS(ch) >= max_stat_value(ch, STAT_WIS))
	{
	  send_to_char("Don't you think you're wise enough already?\r\n", ch);
	  return 1;
	}
	GET_REAL_WIS(ch)++;
	send_to_char("You feel wiser.\r\n", ch);
	rewarded = TRUE;
	break;
      }
    if (!(rewarded))
    {
      send_to_char("This reward is not filled with wisdom, I'm afraid.\r\n", ch);
      return 1;
    }
  } else if (is_abbrev(arg1, "dex")) {
    for (i = 0; i <= 3; i++)
      if (GET_OBJ_VAL(obj, i) == 5)
      {
	if (GET_REAL_DEX(ch) >= max_stat_value(ch, STAT_DEX))
	{
	  send_to_char("Don't you think you're dextrous enough already?\r\n", ch);
	  return 1;
	}
	GET_REAL_DEX(ch)++;
	send_to_char("You feel nimble.\r\n", ch);
	rewarded = TRUE;
	break;
      }
    if (!(rewarded))
    {
      send_to_char("This reward is not filled with dexterity, I'm afraid.\r\n", ch);
      return 1;
    }
  } else if (is_abbrev(arg1, "con")) {
    for (i = 0; i <= 3; i++)
      if (GET_OBJ_VAL(obj, i) == 6)
      {
	if (GET_REAL_CON(ch) >= max_stat_value(ch, STAT_CON))
	{
	  send_to_char("You're constitution cannot possibly be increased!\r\n", ch);
	  return 1;
	}
	GET_REAL_CON(ch)++;
	send_to_char("You feel hardy.\r\n", ch);
	rewarded = TRUE;
	break;
      }
    if (!(rewarded))
    {
      send_to_char("This reward is not filled with constitution, I'm afraid.\r\n", ch);
      return 1;
    }
  } else if (is_abbrev(arg1, "cha")) {
    for (i = 0; i <= 3; i++)
      if (GET_OBJ_VAL(obj, i) == 7)
      {
	if (GET_REAL_CHA(ch) >= max_stat_value(ch, STAT_CHA))
	{
	  send_to_char("You cannot be anymore charismatic!\r\n", ch);
	  return 1;
	}
	GET_REAL_CON(ch)++;
	send_to_char("You feel charming.\r\n", ch);
	rewarded = TRUE;
	break;
      }
    if (!(rewarded))
    {
      send_to_char("This reward is not filled with charisma, I'm afraid.\r\n", ch);
      return 1;
    }
  }
  if (rewarded)
  {
    obj_from_char(obj);
    extract_obj(obj);
    save_char(ch, NOWHERE);
    send_to_char("The reward vanishes in a puff of logic!\r\n", ch);
    return 1;
  }
  send_to_char("Which reward were you hoping to receive? Type 'reward list' for a list.\r\n", ch);
  return 1;
}

SPECIAL(dimensional_gate)
{
  ACMD(do_stand);
  ACMD(do_say);
  struct obj_data *obj = (struct obj_data *) me;
 
  if (!(cmd) || !(ch) || !(CMD_IS("go")))
    return FALSE;

  one_argument(argument,buf);
  if (!is_abbrev(buf,"gate"))
    return FALSE;

  if(IS_SET(GET_OBJ_VAL(obj,1), CONT_CLOSED)) 
  {
    send_to_char("It's closed.\r\n",ch);
    return 1;
  }
  act("$n steps through the dimensional gate and disappears.",FALSE,ch,0,0,TO_ROOM);
  act("You step into the flowing current of the time/space continuum, and disappear.\r\n",FALSE,ch,0,0,TO_CHAR);
  char_from_room(ch);
  char_to_room(ch,real_room(GET_OBJ_VAL(obj,0)));
  act("The room seems to ripple as $n arrives from nowhere.\r\n",FALSE,ch,0,0,TO_ROOM);
  look_at_room(ch,0);
  GET_POS(ch) = POS_SITTING;
  switch(number(0,5))
  {
    case 1:
      do_say(ch,"What a rush!",0,0);
      break;
    case 2:
      do_say(ch,"Whoa, trippy!",0,0);
      break;
    case 3:
      do_say(ch,"Wow! I think I just saw Elvis!",0,0);
      break;
    default:
      do_stand(ch,"",0,0);
      break;
  }
  return TRUE;
}

/* Artus> Victim has been hit by trap, do the damage. */
int spring_trap(struct char_data *ch, struct obj_data *obj)
{
  struct affected_type af;
  struct char_data *caster = NULL;

  switch (GET_OBJ_VAL(obj, 0)) 
  {
    case TRAP_PIT:
      send_to_char("You twist your ankle as you fall into a pit trap.\r\n", ch);
      act("You hear a crunch as $n falls into a pit.\r\n", FALSE, ch, 0, 0, TO_ROOM);
      damage(NULL, ch, GET_OBJ_VAL(obj, 1), TYPE_UNDEFINED, FALSE);
      extract_obj(obj);
      update_pos(ch);
      if (GET_POS(ch) == POS_DEAD) 
      {
	send_to_char("The pit was spiked, the spikes pierce straight through your heart.\r\n", ch);
	die(ch,NULL,"pit trap");
	return 1;
      }
      GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
      return 1;
      break;
    case TRAP_MAGIC:
      send_to_char("You trigger a magic trap, resulting in a bright flash of light!\r\n", ch);
      act("There is a bright flash of light as $n triggers a magic trap.\r\n", 
	  FALSE, ch, 0, 0, TO_ROOM);
      caster = read_mobile(DG_CASTER_PROXY, VIRTUAL);
      if (!(caster))
      {
	sprintf(buf, "SYSERR: Could not load mob #%d for Magic Trap!", 
	        DG_CASTER_PROXY);
	mudlog(buf, NRM, LVL_IMPL, TRUE);
	return 0;
      }
      if ((GET_SKILL(ch, SKILL_SEARCH) || !LR_FAIL(ch, LVL_IMMORT)) &&
	  (get_name_by_id(GET_OBJ_VAL(obj, 2))))
	caster->player.short_descr = str_dup(get_name_by_id(GET_OBJ_VAL(obj, 2)));
      else
	caster->player.short_descr = str_dup("someone");
      caster->next_in_room = world[IN_ROOM(ch)].people;
      world[IN_ROOM(ch)].people = caster;
      caster->in_room = IN_ROOM(ch);
      call_magic(caster, ch, NULL, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 3),
	         CAST_MAGIC_OBJ);
      extract_char(caster);
      if ((!affected_by_spell(ch, SPELL_BLINDNESS)) &&
	  (!IS_NPC(ch) || !MOB_FLAGGED(ch, MOB_NOBLIND)))
      {
	af.duration = 2;
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.type = SPELL_BLINDNESS;
	af.bitvector = AFF_BLIND;
	affect_to_char(ch, &af);
      }
      extract_obj(obj);
      return 1;
      break;
    default:
      send_to_char("You are lucky, the trap was broken.\r\n", ch);
      return 0;
  }
}

SPECIAL(room_trap)
{
  struct obj_data *obj = (struct obj_data *) me;
  if ((!cmd) || !(ch))
    return 0;

  // Yeah. This is kinda dodgy .. 
  if (CMD_IS("flee") || CMD_IS("retreat") || CMD_IS("west") || 
      CMD_IS("east") || CMD_IS("north") || CMD_IS("escape") || 
      CMD_IS("south") || CMD_IS("up") || CMD_IS("down"))
  {
    if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOKILL))
      return 0;

    if (!IS_NPC(ch) && (GET_OBJ_VAL(obj, 2) == GET_IDNUM(ch))) 
    {
      send_to_char("You step safely around your trap.\r\n", ch);
      return 0;
    }
    if (number(0, 22) < GET_DEX(ch))
      return 0;
    return spring_trap(ch, obj);
  }
  return 0;
}

SPECIAL(bank)
{
  int amount;
  int fee;
  int percent=8;
  bool maxed = FALSE;

  if (!(cmd) || !(ch) || IS_NPC(ch))
    return FALSE;

  argument = one_argument(argument, arg); // Artus> For "All"

  if (CMD_IS("balance"))
  {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is &Y%d&n coins.\r\n", GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    int multiplier = 500000;
    if (GET_CLASS(ch) > CLASS_WARRIOR)
    {
      if (GET_CLASS(ch) >= CLASS_MASTER)
	multiplier <<= 1;
      else
	multiplier += multiplier >> 1;
    }

    if (!(str_cmp(arg, "all"))) // Artus> Deposit/Withdraw All.
    {
      amount = MAX(0, GET_GOLD(ch));
      if (amount <= 0)
      {
	send_to_char("You don't have any gold to deposit!\r\n", ch);
	return 1;
      }
    } else if ((amount = atoi(arg)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch) < amount)
    {
      send_to_char("You don't have that many coins!\r\n", ch);
      return 1;
    }

    // DM - bank limitations (500000 * level)
    if (GET_BANK_GOLD(ch) + amount > GET_MAX_LVL(ch) * multiplier)
    {
      amount = GET_MAX_LVL(ch) * multiplier - GET_BANK_GOLD(ch);
      maxed = TRUE;
    }

/* JA 8/3/95 added some code for bank transaction fees */
    GET_GOLD(ch) -= amount;
    fee = (amount * percent) /100;
    if (fee<10)
      fee=10;
    GET_BANK_GOLD(ch) += amount;
/*
    GET_BANK_GOLD(ch) -= fee;
*/
    sprintf(buf, "You deposit &Y%d&n coins%s.\r\n", 
	amount, (maxed) ? " maxing your bank balance" : "");
    send_to_char(buf, ch);
 /*
    sprintf(buf, "The transaction fee is %d percent which is %d coins", percent, fee);
    send_to_char(buf, ch);
*/
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if (!str_cmp(arg, "all")) // Artus> Deposit/Withdraw All
    {
      if ((amount = MAX(0, GET_BANK_GOLD(ch))) <= 0)
      {
	send_to_char("You don't have any gold in the bank!\r\n", ch);
	return 1;
      }
    } else if ((amount = atoi(arg)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (GET_BANK_GOLD(ch) < amount)
    {
      send_to_char("You don't have that many coins deposited!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) += amount;
/*    fee = (amount * percent) / 100; 
    GET_GOLD(ch) -= fee;*/
    GET_BANK_GOLD(ch) -= amount;
    sprintf(buf, "You withdraw &Y%d&n coins.\r\n", amount);
    send_to_char(buf, ch);
/*    sprintf(buf, "The transaction fee is %d percent which is %d coins", percent, fee);
    send_to_char(buf, ch);*/
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else
    return 0;
}

#if 0
SPECIAL(fountain)
{
/* get the fountain to exude virii !! */
  int amount;
  if (!(cmd) || !(ch))
    return FALSE;

  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is &Y%d&n coins.\r\n", GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) -= amount;
    GET_BANK_GOLD(ch) += amount;
    sprintf(buf, "You deposit &Y%d&n coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins deposited!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) += amount;
    GET_BANK_GOLD(ch) -= amount;
    sprintf(buf, "You withdraw &Y%d&n coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else
    return 0;
}
#endif

/* this sets tag on ppl and teleports them into the tag zone - Vader */
SPECIAL(set_tag)
{
  int room = 0;

  if (!(cmd) || !(ch))
    return FALSE;

  if(!CMD_IS("east") || !CAN_GO(ch,EAST))
    return 0;
  else {
    do {
      room = number(4500,4556);
      } while(room == 4518 || room == 4519 || room == 4520);
    act("$n leaves east.",FALSE,ch,0,0,TO_ROOM);
    send_to_char("You are whisked off to a random position in the tag arena. Good luck!\r\n\r\n",ch);
    char_from_room(ch);
    char_to_room(ch,real_room(room));
    look_at_room(ch,0);
    SET_BIT(PRF_FLAGS(ch), PRF_TAG);
    }

return 1;
}

#if 0 // Drax, this is really fucking sad.
SPECIAL(love_ring)
{
  struct obj_data *obj1 = (struct obj_data *) me;
  char mybuf[50]; 
 
  if(!CMD_IS("wear"))
    return 0;
  if(obj1->carried_by != ch)
    return 0;
 
  one_argument(argument, buf);
 
  if(!*buf)
    return 0;
 
  strcpy(mybuf, "");
  strcpy(mybuf, GET_NAME(ch));
 /* *buf = toupper(buf);*/
  if ((strcmp(buf, "ring")== 0) && strcmp(mybuf, "Takhisis") !=0)
  {
        send_to_char("The Love Ring screeches, 'THIEF'.  You will pay for this!\r\n", ch);
        GET_HIT(ch)=0;
        GET_MANA(ch) = 0;
        GET_MOVE(ch) = 0;
        return 0;
  }
  if (strcmp(buf, "ring")== 0)
     send_to_char("The Love Ring chimes in Drax's voice, 'I LOVE you Sandra!'\r\n", ch);
return 0;
}
#endif

#if 0
/* quick joining proc to make 2 objects into 1 for Ruprects zone - Vader */
SPECIAL(ring)
{
  struct obj_data *obj1 = (struct obj_data *) me;
  struct obj_data *obj2;


  if(!CMD_IS("use"))
    return 0;
  if(obj1->carried_by != ch)
    return 0;

  two_arguments(argument, buf, arg);

  if(!*arg)
    return 0;
  if(!*buf)
    return 0;
  
  to_upper(buf);
  to_upper(arg);
  if((strcmp(buf, "CLAW") == 0 && strcmp(arg, "BRIDLE") == 0) ||
     (strcmp(buf, "BRIDLE") == 0 && strcmp(arg, "CLAW") == 0)) {
    for(obj2 = ch->carrying; obj2; obj2 = obj2->next_content) {
      if(GET_OBJ_VNUM(obj2) == 4821) {
        send_to_char("They seem to fit together perfectly to form a ring!\r\n", ch);
        extract_obj(obj2);
        obj1 = read_object(4820, VIRTUAL);
        return 1;
        }
      }
    }
  return 0;
}
#endif

#if 0 // Artus> Unused.
SPECIAL(rainbow_rod)
{
  struct char_data *vict;

  if (!(cmd) || !(ch))
    return FALSE;

  if((CMD_IS("use")))
  	two_arguments(argument, buf, arg);
  else
	return 0;

	if(!*buf){
	  send_to_char("Use What?!\r\n", ch);
	  return 0;
	}
  	if(!*arg){
          send_to_char("Use the Rod on WHO!?\r\n", ch);
    	  return 0;
        }
  
	to_upper(buf);
	if (buf != "ROD")
 	return 0;	

	if (!(vict = get_char_room_vis(ch, arg))){
      		send_to_char("They aren't here.\r\n", ch);
		return 0;
	}

	if (vict == ch){
		send_to_char("Ohh real wise that!!!.\r\n", ch);
		return 0;
	}

  	if (FIGHTING(ch)){
		send_to_char("The Rod makes a wheezing sound.  And flashes slightly.\r\n",ch);
		send_to_char("It appears to be exhausted\r\n",ch);
		return 0;
  	}


	act("A Rainbow Rod shivers mightily in $n's hand.", FALSE, ch, 0, 0, TO_ROOM);
	send_to_char("The Rainbow Rod shivers mightily in your hand\r\n", ch);
/*	cast_spell (ch, vict, NULL, SPELL_HARM);
	cast_spell (ch, vict, NULL, SPELL_CALL_LIGHTNING);
	cast_spell (ch, vict, NULL, SPELL_COLOR_SPRAY);
        cast_spell (ch, vict, NULL, SPELL_FIREBALL);
	call_magic(ch, vict, 0, SPELL_CURSE, GET_LEVEL(ch), CAST_SPELL);
	call_magic(ch, vict, 0, SPELL_BLINDNESS, GET_LEVEL(ch), CAST_SPELL);
	cast_spell (ch, vict, NULL, SPELL_PLASMA_BLAST);*/
        return 0;
}
#endif

/* special procedure for Ruprects glass elevator.. sends you randomly around
 * the fairy tale zone - Vader
 * NOTE: this hasta be assigned to the room before the elevator so that 
 * it can trap the command to take you there cos the room spec is only
 * called on command - Vader
 */
SPECIAL(elevator)
{
  int room = 0;
  if (!(cmd) || !(ch))
    return FALSE;

  if(!CMD_IS("west") || !CAN_GO(ch,WEST))
    return 0;
  else {
    do {
      room = number(4300,4487);
      } while(room == 4482 || room == 4436);
    act("$n leaves west.",FALSE,ch,0,0,TO_ROOM);
    send_to_char("You step into the elevator and are flown off to...\r\n\r\n",ch);
    char_from_room(ch);
    char_to_room(ch,real_room(room));
    look_at_room(ch,0);
    }

  return 1;
}

/* this proc for DB's zone transports ppl to a room when they ask the mob
 * about the word magic - Vader
 */
SPECIAL(richard_garfield)
{
  struct char_data *rich = (struct char_data *) me;
  ACMD(do_say);
  ACMD(do_spec_comm);
  unsigned int i;
 
  if(CMD_IS("ask")) {
    do_spec_comm(ch,argument,0,SCMD_ASK);
    skip_spaces(&argument);
    for(i=0;i<strlen(argument);i++) {
      argument[i] = LOWER(argument[i]);
      }
    if(strstr(argument,"magic")) {
      sprintf(buf,"Ahhh! You look like an intelligent young %s, you should pick this up in no time!",
              (GET_SEX(ch) == SEX_MALE ? "man" : 
              (GET_SEX(ch) == SEX_FEMALE ? "girl" : "thing")));
      do_say(rich,buf,0,0);
      act("$n begins to explain the rules of Magic to you.",FALSE,rich,0,ch,TO_VICT);
      act("$n puts his arm around $N and the two start talking between themselves...",FALSE,rich,0,ch,TO_NOTVICT);
      act("$n's hypnotic voice and constant card shuffling make you dizzy...\r\nHe shuffles the cards again and everything goes dark...",FALSE,rich,0,ch,TO_VICT);
      act("$n shuffles his deck of cards over and over. Suddenly $N is gone!",FALSE,rich,0,ch,TO_NOTVICT);
      act("As $n puts the deck away you notice that the picture on the top card looks a lot like $N... No. It couldn't be...",FALSE,rich,0,ch,TO_NOTVICT);
      char_from_room(ch);
      GET_POS(ch) = POS_SLEEPING;
      char_to_room(ch,real_room(2535));
    } else {
      act("$n gives you a confused look.",FALSE,rich,0,ch,TO_VICT);
      act("$n gives $N a confused look.",FALSE,rich,0,ch,TO_NOTVICT);
      }
    return 1;
    }
 
  switch(number(0,25)) {
    case 1: act("$n looks shifty as he checks left and right then leans close to you...",FALSE,rich,0,0,TO_ROOM);
            do_say(rich,"Wanna learn about Magic?",0,0);
            break;
    case 2: do_say(rich,"Want me to teach you about Magic?",0,0);
            do_say(rich,"All you have to do is ask!",0,0);
            break;
    case 3: act("$n thoughtfully shuffles through $s deck of cards...",FALSE,rich,0,0,TO_ROOM);
            break;
    case 4: act("$n starts scribbling some new rules on the back of one of $s cards.",FALSE,rich,0,0,TO_ROOM);
            break;
    default: return 0;
    }
 
return 0;
}

/* proc to change the exits in the maze - by DB *
 * Artus> Why call real room 6 times, DB? :o)   */
SPECIAL(maze)
{
  room_rnum dud;
  dud = real_room(2764);
  if (CMD_IS("north") || CMD_IS("west") || CMD_IS("south") || 
      CMD_IS("east") || CMD_IS("up") || CMD_IS("down")) {
        world[ch->in_room].dir_option[0]->to_room = dud;
        world[ch->in_room].dir_option[1]->to_room = dud;
        world[ch->in_room].dir_option[2]->to_room = dud;
        world[ch->in_room].dir_option[3]->to_room = dud;
        world[ch->in_room].dir_option[4]->to_room = dud;
        world[ch->in_room].dir_option[5]->to_room = dud;
        world[ch->in_room].dir_option[number(0, 5)]->to_room = real_room(2761);
        return 0;
  }
  return 0;
}

/* proc by DB for  his Magic zone */
SPECIAL(dragon)
{
  struct descriptor_data *d;
  struct char_data *vict;

  if (cmd)
    return 0;
  if (number(0, 1))
    return 1;
  if (GET_POS(ch) == POS_FIGHTING)      {
    switch(number(0, 1))
    {
      case 0:
	act("$n takes a deep breath, flames flickering in $s throat.", TRUE, ch, 0, 0, TO_ROOM);
	act("You take a deep breath.", TRUE, ch, 0, 0, TO_CHAR);
	return 1;
      case 1:
	act("$n breathes a cone of flame into the air.", TRUE, ch, 0, 0, TO_ROOM);
	act("You breathe a cone of flame into the air.", TRUE, ch, 0, 0, TO_CHAR);
	for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
	{
	  if (FIGHTING(vict) && (vict != ch) && 
	      (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_NOKILL)))
	  {
	    act("$N's skin bubbles and chars from the heat of the flames.", TRUE, ch, 0, vict, TO_NOTVICT);
	    act("Flame rolls over you, charring your skin.", TRUE, ch, 0, vict, TO_VICT);
	    damage(ch, vict, GET_LEVEL(ch)*3, TYPE_UNDEFINED, FALSE);
	    if (GET_HIT(vict) < 0)  
	    {
	      act("$N burns, screaming, until only a charred skeleton remains.", TRUE, ch, 0, vict, TO_ROOM);
	      act("You are completely incinerated.", TRUE, ch, 0, vict, TO_VICT);
	      die(vict,ch);
	    }
	  }
        }
      default:
	break;
    }
    return 1;
  }
  if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
  {
    for (d = descriptor_list; d; d = d->next)
      if ((d->character) && (GET_IDNUM(d->character) == MEMORY(ch)->id))
      {
	if (BASE_SECT(world[d->character->in_room].sector_type) == SECT_INSIDE)
	{
	  act("You hear an enraged scream of frustration from outside, you're glad you're inside.", TRUE, d->character, 0, ch, TO_VICT);
	  act("You hear a wild scream of frustration from far off.", TRUE, d->character, 0, ch, TO_NOTVICT);
	  act("You hear an enraged scream of frustration from outside, you're glad you're inside.", TRUE, d->character, 0, ch, TO_VICT);
	  act("You hear a wild scream of frustration from far off.", TRUE, d->character, 0, ch, TO_VICT);
	  act("You try to fly to $N but $e's inside.", TRUE, ch, 0, d->character, TO_CHAR);
	  break;
	}
	if UNDERWATER(d->character)
	{
	  act("You see a bat-winged shape flit over the surface of the water.", TRUE, d->character, 0, ch, TO_VICT);
	  act("You see a bat-winged shape flit over the surface of the water.", TRUE, d->character, 0, ch, TO_NOTVICT);
	  act("You try to fly to $N but $e's underwater.", TRUE, ch, 0, d->character, TO_CHAR);
	  break;
	}
	switch (number(0, 5))
	{
	  case 0: 
	  case 1:
	    char_from_room(ch);
	    char_to_room(ch, d->character->in_room);
	    act("$n stoops on you, flames flickering around its maw.", TRUE, ch, 0, d->character, TO_VICT);
	    act("$n stoops on $N, spouting flames.", TRUE, ch, 0, d->character, TO_NOTVICT);
	    act("You stoop on $N, spouting flames.", TRUE, ch, 0, d->character, TO_CHAR);
	    break;
	  case 2: 
	  case 3:
	  case 4:
	  case 5:
	    act("A huge bat-winged shape circles above you.", TRUE, d->character, 0, ch, TO_CHAR);
	    act("A huge bat-winged shape circles above you.", TRUE, d->character, 0, ch, TO_ROOM);
	    act("You circle above $N.", TRUE, ch, 0, d->character, TO_CHAR);
	    break;
	  default: break;
	}
    }
  }
  return 0;
}

/* this is a spec proc for a TARDIS.. its a toy at the mo but its for a
 * zone im doing... - Vader
 */
#define EXITN(room, door)                (world[room].dir_option[door])
#define TARDIS_ROOM 2
#define TARDIS_VNUM 22399
#define TARDIS_FLIGHT 1
#define TARDIS_LANDED 0
 
SPECIAL(tardis)
{
  extern room_rnum top_of_world;
  extern sh_int find_target_room(struct char_data * ch,char *rawroomstr);
  struct obj_data *console = (struct obj_data *) me;
  struct obj_data *tardis, *next_obj;
  sh_int destination,room;
  sh_int check_dest;
  unsigned int i;
 
  if (!(cmd) || !(ch))
    return 0;

  if(!CMD_IS("goto") && !CMD_IS("exits") && !CMD_IS("where") && !CMD_IS("look"))
    return 0;
 
  if(CMD_IS("goto"))
  {
    if(LR_FAIL(ch, LVL_IS_GOD))
    {
      send_to_char("Your not sure how to work it...\r\n",ch);
      return 1;
    }
    one_argument(argument, arg);
    if(!*arg)
      return 0;
    if((check_dest = find_target_room(ch,arg)) < 0)
      return 0;
    else
      destination = check_dest;
    GET_OBJ_VAL(console,0) = world[destination].number;
    if(EXIT(console,2)->to_room != -1) {
    for(tardis = world[EXIT(console,2)->to_room].contents;tardis;tardis = next_obj) {
      next_obj = tardis->next_content;
      if(GET_OBJ_VNUM(tardis) == TARDIS_VNUM) {
        send_to_room("WWRRRRSSSSSHHHHH! WWRRRRSSSSSHHHHH!\r\nThe Police Box slowly dematerializes...\r\n",tardis->in_room);
        extract_obj(tardis);
        }
      }
      console->obj_flags.cost = TARDIS_FLIGHT;
      EXIT(console,2)->to_room = -1;
      SET_BIT(EXITN(ch->in_room, 2)->exit_info, EX_CLOSED);
/*      SET_BIT(GET_OBJ_VAL(tardis, 1), CONT_CLOSED);*/
      act("$n's hands work quickly over $p as $e sets the co-ordinates.",FALSE,ch,console,0,TO_ROOM);
      send_to_room("The door closes.\r\n",ch->in_room);
      act("The pillar in the center of $p begins to rise and fall as the TARDIS takes off.",FALSE,ch,console,0,TO_ROOM);
      send_to_char("You set the co-ordinates and start the TARDIS on its way.\r\n",ch);
      return 1;
      }
    }
  if(CMD_IS("exits")) {
    if(EXIT(console,2)->to_room == -1) {
      tardis = read_object(TARDIS_VNUM, VIRTUAL);
      if(number(0,5) == 1) {
        room = number(0,top_of_world);
        send_to_room("The time rotor flickers slightly...\r\n",console->in_room);
      } else
        room = real_room(GET_OBJ_VAL(console,0));
      obj_to_room(tardis, room);
      console->obj_flags.cost = TARDIS_LANDED;
      EXIT(console,2)->to_room = tardis->in_room;
      REMOVE_BIT(EXITN(console->in_room,2)->exit_info, EX_CLOSED);
      send_to_room("The pillar in the center of the console stops moving as the TARDIS lands.\r\n",console->in_room);
      send_to_room("The door opens.\r\n",console->in_room);
      send_to_room("WWRRRRSSSSSHHHHH! WWRRRRSSSSSHHHHH!\r\nA London Police Box materializes from nowhere.\r\n",tardis->in_room);
GET_OBJ_VAL(tardis,0) = world[console->in_room].number; /* makes any room a tardis */
      return 1;
      }
    return 0;
    }
  if(CMD_IS("where")) {
    one_argument(argument,arg);
    if(!*arg) {
      sprintf(buf,"The Destination Co-ordinate Display reads: %d.%d\r\n",GET_OBJ_VAL(console,0),real_room(GET_OBJ_VAL(console,0)));
      send_to_char(buf,ch);
      return 1;
      } else return 0;
    }
  if(CMD_IS("look")) {
    skip_spaces(&argument);
    one_argument(argument,arg);
    if((EXIT(console,2)->to_room == -1) || (!*arg))
      return 0;
    strcpy(buf,argument);
    for(i=0;i<=strlen(buf);i++) {
      buf[i] = LOWER(buf[i]);
      }
    if(!strncmp(buf,"scanner",7)) {
      sprintf(buf,"On the scanner screen you can see: %s\r\n",world[EXIT(console,2)->to_room].name);
      send_to_char(buf,ch);
    } else return 0;
    return 1;
    }
  return 1;
}

SPECIAL(marbles)
{
  struct obj_data *tobj = (struct obj_data*) me;
 
  if (tobj->in_room == NOWHERE)
    return 0;
 
  if (CMD_IS("north") || CMD_IS("south") || CMD_IS("east") || CMD_IS("west") ||
      CMD_IS("up") || CMD_IS("down")) {
    if (number(1, 100) + GET_DEX(ch) > 50) {
      act("You slip on $p and fall.", FALSE, ch, tobj, 0, TO_CHAR);
      act("$n slips on $p and falls.", FALSE, ch, tobj, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      return 1;
    }
    else {
      act("You slip on $p, but manage to retain your balance.", FALSE, ch, tobj, 0, TO_CHAR);
      act("$n slips on $p, but manages to retain $s balance.", FALSE, ch, tobj, 0, TO_ROOM);
    }
  }
  return 0;
}


/* some sickening xmas procs - VADER */

#define XMAS_BOX 22321
#define XMAS_UPPER 22348
#define XMAS_LOWER 22327
#define XMAS_CANDY 22344
#define XMAS_STRING "candy"
 
SPECIAL(xmas_tree)
{
  int i;
  static int num_pres = 0;
  struct obj_data *box, *gift;
  static char previous[5][MAX_NAME_LENGTH] = { "nobody\0",
                                               "nobody\0",
                                               "nobody\0",
                                               "nobody\0",
                                               "nobody\0" };
  ACMD(do_examine);
 
  if (!(cmd) || !(ch))
    return 0;

  if(IS_NPC(ch) || !AWAKE(ch))
    return 0;
 

  if((CMD_IS("say") || CMD_IS("'")) && GET_LEVEL(ch) >= 80) {
    if(!strcmp(argument," tree stats")) {
      sprintf(buf,"Pressies given since reboot: %d\r\n\r\n",num_pres);
      send_to_char(buf,ch);
      sprintf(buf,"Last 5 lucky gooons:\r\n");
      for(i=0;i<5;i++) {
        sprintf(buf,"%s\t%s\r\n",buf,previous[i]);
        }
      send_to_char(buf,ch);
      return 1;
      }
    return 0;
    }

  for(i=0;i<5;i++) {
    if(!(strcmp(previous[i],GET_NAME(ch))))
      return 0;
    }

  if(CMD_IS("look") || CMD_IS("examine")) {
    one_argument(argument,buf);
    if(!(strcmp(buf,"tree"))) {
      do_examine(ch,argument,0,0);
      box = read_object(XMAS_BOX,VIRTUAL);
      gift = read_object(number(XMAS_LOWER,XMAS_UPPER),VIRTUAL);
      sprintf(buf,"A Christmas present for %s is lying here.",GET_NAME(ch));
      box->description = str_dup(buf);
      obj_to_obj(gift,box);
      obj_to_room(box,ch->in_room);
      GET_OBJ_VAL(box,3) = 1;
      GET_OBJ_TIMER(box) = 5;
      send_to_room("A gift from the Gods appears at the base of the tree!\r\n",ch->in_room);
      send_to_char("It's addressed to you!\r\n",ch);
      act("It's addressed to $n!",FALSE,ch,0,0,TO_NOTVICT);
      strcpy(previous[4],previous[3]);
      strcpy(previous[3],previous[2]);
      strcpy(previous[2],previous[1]);
      strcpy(previous[1],previous[0]);
      sprintf(previous[0],"%s",GET_NAME(ch));
      num_pres++; /* keep a total of pressies given since reboot */
      return 1;
      }
    }
  return 0;
}

SPECIAL(santa)
{
  ACMD(do_say);
  ACMD(do_gen_comm);
  ACMD(do_give);
  ACMD(do_eat);
  struct char_data *vict;
  struct obj_data *candy;

  if(cmd)
    return 0;

  GET_POS(ch) = POS_STANDING;

  if(FIGHTING(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);

  for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    if((vict != ch) && !(number(0,2)))
      break;
    }


  switch(number(0,60)) {
    case 0:
      do_gen_comm(ch,"Ho Ho bloody Ho! Merry Whatever!!",0,SCMD_HOLLER);
      break;
    case 1:
      do_say(ch,"Those reindeer are damn tasty!",0,0);
      break;
    case 2:
  if(vict == NULL)
    return 0;
      candy = read_object(XMAS_CANDY,VIRTUAL);
      obj_to_char(candy,ch, __FILE__, __LINE__);
      sprintf(buf,"%s %s",XMAS_STRING,GET_NAME(vict));
      do_give(ch,buf,0,0);
      act("$n pats $N on the head.",FALSE,ch,0,vict,TO_NOTVICT);
      send_to_char("Santa pats you on the head.\r\n",vict);
      do_eat(ch,XMAS_STRING,0,SCMD_EAT);
      break;
    case 3:
  if(vict == NULL)
    return 0;
      act("$n pats $N on the head.",FALSE,ch,0,vict,TO_NOTVICT);
      send_to_char("Santa pats you on the head.\r\n",vict);
      if(GET_SEX(vict) == SEX_FEMALE)
        do_say(ch,"Hello there little girl.",0,0);
      else
        do_say(ch,"How are ya little fella?",0,0);
      do_say(ch,"Wanna come sit on my knee? Ignore the bulge, I'm pretty sure it's just a candy cane in my pocket...",0,0);
      break;
    case 4:
      do_gen_comm(ch,"Dammit! Rudolf bit me!",0,SCMD_HOLLER);
      break;
    case 5:
      do_say(ch,"Uh Oh... Where are my pants?",0,0);
      break;
    case 6:
      do_say(ch,"What would you like for Christmas??",0,0);
      act("$n mumbles, 'As if I care...'",FALSE,ch,0,0,TO_ROOM);
      break;
    case 7:
      candy = read_object(XMAS_CANDY,VIRTUAL);
      obj_to_char(candy,ch, __FILE__, __LINE__);
      do_eat(ch,XMAS_STRING,0,SCMD_EAT);
      break;
    case 8:
  if(vict == NULL)
    return 0;
      sprintf(buf,"No %s, you can't have an inflatable goat!",GET_NAME(vict));
      do_gen_comm(ch,buf,0,SCMD_HOLLER);
      break;
    case 9:
      do_say(ch,"Why have I got this ferret strapped to my face?",0,0);
      break;
    case 10:
  if(vict == NULL)
    return 0;
      candy = read_object(XMAS_CANDY,VIRTUAL);
      obj_to_char(candy,ch, __FILE__, __LINE__);
      if(GET_SEX(vict) == SEX_FEMALE) {
        act("$n pokes $N in the ribs.",FALSE,ch,0,vict,TO_NOTVICT);
        send_to_char("Santa pokes you in the ribs.\r\n",vict);
        do_say(ch,"One night with me honey and you'll be sneezing tinsel!",0,0);
        }
      sprintf(buf,"%s %s",XMAS_STRING,GET_NAME(vict));
      do_give(ch,buf,0,0);
      do_eat(ch,XMAS_STRING,0,0);
      break;
    case 11:
      do_gen_comm(ch,"Hey! I think that last kid stole my wallet!!",0,SCMD_HOLLER);
      break;
    case 12:
      act("$n grabs a passing kiddy and shoves him into his sack.",FALSE,ch,0,0,TO_ROOM);
      act("$n snickers softly to himself.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 13:
      do_say(ch,"Go look at the tree in the tavern if ya want a pressie!",0,0);
      break;
    case 14:
  if(vict == NULL)
    return 0;
      candy = read_object(XMAS_CANDY,VIRTUAL);
      obj_to_char(candy,ch, __FILE__, __LINE__);
      sprintf(buf,"%s %s",XMAS_STRING,GET_NAME(vict));
      do_give(ch,buf,0,0);
      do_eat(ch,XMAS_STRING,0,0);
      do_say(ch,"Hey there. Wanna be my special little pixie?",0,0);
      act("$n winks suggestively at $N.",FALSE,ch,0,vict,TO_NOTVICT);
      send_to_char("Santa winks suggestively at you.",vict);
      break;
    case 15:
  if(vict == NULL)
    return 0;
      if(GET_SEX(vict) == SEX_FEMALE) {
        act("$n looks $N up and down.",FALSE,ch,0,vict,TO_NOTVICT);
        send_to_char("Santa looks you up and down.",vict);
        do_say(ch,"Wow. I wouldn't mind putting some presents under her tree!",0,0);
        }
      break;
    case 16:
  if(vict == NULL)
    return 0;
      if(IS_NPC(vict)) {
        act("$N pulls $n's beard.",FALSE,ch,0,vict,TO_ROOM);
        act("In a fit of rage $n beats $N to death with a candy cane.",FALSE,ch,0,vict,TO_ROOM);
        raw_kill(vict,ch);
        act("The kids watching burst into tears.",FALSE,ch,0,0,TO_ROOM);
        do_say(ch,"Shut up ya little brats or you're next!!",0,0);
        }
      break;
    case 17:
      do_say(ch,"Is it snowing or did I buy the wrong anti-dandruff shampoo?",0,0);
      break;
    case 18:
      do_say(ch,"Is this Christmas shit over with yet?",0,0);
      break;
    case 19:
      do_say(ch,"When's someone gunna come and put presents under my tree?",0,0);
      act("$n sniffs sadly.",FALSE,ch,0,0,TO_ROOM);
      break;
    case 20:
  if(vict == NULL)
    return 0;
      do_say(ch,"Remember, I know when you've been bad.",0,0);
      act("$n winks suggestively at $N.",FALSE,ch,0,vict,TO_NOTVICT);
      send_to_char("Santa winks suggestively at you.",vict);
      break;
    case 21:
  if(vict == NULL)
    return 0;
      if(GET_SEX(vict) == SEX_FEMALE) {
        sprintf(buf,"Schwiiing!! Check out the ornaments on %s!",GET_NAME(vict));
        do_gen_comm(ch,buf,0,SCMD_HOLLER);
        }
      break;
    case 22:
      do_gen_comm(ch,"I'm hornier than Rudolf on Red Nose day!",0,SCMD_HOLLER);
      break;
    case 23:
      do_gen_comm(ch,"Anyone wanna err... pull my cracker?",0,SCMD_HOLLER);
      break;
    }
  return 1;
}

/* proc of christmas carollers - Vader */
char carol1[12][80] = {
    "And a really lame Christmas carol!",
    "Two many bugs",
    "Three snazzzy worlds",
    "Four linkless Gods",
    "Fiiiiive tiiiiiiiimed out pings",
    "Six random rebooots",
    "Seven minutes lag",
    "Eight newbies dyin",
    "Nine days downtime",
    "Ten chaotic crashes",
    "Eleven players cheatin",
    "Twelve screens o' spam"
};

char carol2[12][45] = {
    "Deck the halls with boughs of holly",
    "'Tis the season to be jolly",
    "Don we now our gay aparel",
    "Troll the ancient Yule-tide carol",
    "See the blazing Yule before us",
    "Strike the harp and join the chorus",
    "Follow me in merry measure",
    "While I tell of Yule-tide treasure",
    "Fast away the old year passes",
    "Hail the new year lads n lasses",
    "Sing we joyous, all together",
    "Heedless of the wind and weather"
};

char carol3[12][50] = {
    "Decorate Haven with newbie gizards",
    "'Tis the season for dancing lizards",
    "Don we now our best equipment",
    "Then we'll slaughter pirates by the shipment",
    "See the blazing Foool before us",
    "That'll teach him to play with matches",
    "Follow me in merry measure",
    "Or I'll kick in your teeth, it'll be my pleasure",
    "We'll mud all day from our Uni classes",
    "I'd be very surprised if any of us passes",
    "Sing we joyous, out of tune",
    "If you don't then you're a Gooon!"
};

char carol4[18][50] = {
    "O Serpent's eye, O Serpent's eye",
    "How lovely are your lashes",
    "You are so green, you're covered in slime",
    "You make me spaaaarkle all the time",
    "O Serpent's eye, O Serpent's eye",
    "How lovely are your lashes",
    "O Serpent's eye, O Serpent's eye",
    "You stare at me unblinkingly",
    "You help me fight those meaner mobs",
    "The thought of you just makes me throb",
    "O Serpent's eye, O Serpent's eye",
    "You stare at me unblinkingly",
    "O Serpent's eye, O Serpent's eye",
    "I like the way you flutter",
    "You are so good. You make me strong",
    "I'll search for yooou in Wollongong",
    "O Serpent's eye, O Serpent's eye",
    "I like the way you flutter"
};

char days[12][5] = {
    "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th",
    "10th", "11th", "12th"
};

#define SONG_12DAYS 0
#define SONG_DECK1  1
#define SONG_DECK2  2
#define SONG_EYE    3

void AllSing(char *line, struct char_data *ch) {
  ACMD(do_gen_comm);
  struct follow_type *fol;
 
  if(!ch->followers)
    do_gen_comm(ch,line,0,SCMD_HOLLER);
  else {
    fol = ch->followers;
    while(fol) {
      GET_MOVE(fol->follower) += 50;
      do_gen_comm(fol->follower,line,0,SCMD_HOLLER);
      fol = fol->next;
    }
  }
}  

SPECIAL(caroller)
{
  ACMD(do_gen_comm);
  struct char_data *vict;
  static int song, up2;
  static bool second = FALSE;
  struct affected_type af;
  int i;
  char buf[80];

  if(cmd)
    return 0;

  af.type = SPELL_CHARM;
  af.duration = 10;
  af.modifier = af.location = 0;
  af.bitvector = AFF_CHARM;

  GET_POS(ch) = POS_STANDING;

  if(FIGHTING(ch)) {
    GET_HIT(ch) = GET_MAX_HIT(ch);
    vict = FIGHTING(ch);
    stop_fighting(ch);
    stop_fighting(vict);
    add_follower(vict,ch);
    affect_to_char(vict,&af);
    }

  for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    if((vict != ch) && !vict->master && IS_NPC(vict) && (number(0,1) == 1)) {
      add_follower(vict,ch);
      SET_BIT(AFF_FLAGS(vict),AFF_CHARM);
      break;
      }
    }

  if(ch->char_specials.timer == 0 && number(0,60) == 0) {
    song = number(0,3);
    up2 = 0;
    ch->char_specials.timer = 1;
    second = FALSE;
    }

  if(ch->char_specials.timer == 1) {
    switch(song) {
      case SONG_12DAYS:
        sprintf(buf,"On the %s day of Christmas, my true love gave to me...",days[up2]);
        do_gen_comm(ch,buf,0,SCMD_HOLLER);
        if(up2 == 0)
          do_gen_comm(ch,"A really lame Christmas carol!",0,SCMD_HOLLER);
        else {
          if(up2 == 4)
            AllSing(carol1[4],ch);
          for(i = up2; i >= 0; i--)
            do_gen_comm(ch,carol1[i],0,SCMD_HOLLER);
          if(up2 == 11)
            AllSing(carol1[0],ch);
          }
        up2++;
        if(up2 > 11)
          ch->char_specials.timer = 0;
        break;
      case SONG_DECK1:
      case SONG_DECK2:
        second = !second;
        if(!second) {
          if(!((up2+1) % 4))
            AllSing("Fa-la-la, la-la-la, la-la-la",ch);
          else
            AllSing("Fa-la-la-la-la, la-la-la-la",ch);
          if(up2 > 11)
            ch->char_specials.timer = 0;
        } else {
          do_gen_comm(ch,(song == 1 ? carol2[up2] : carol3[up2]),0,SCMD_HOLLER);
          up2++;
          }
        break;
      case SONG_EYE:
        do_gen_comm(ch,carol4[up2++],0,SCMD_HOLLER);
        do_gen_comm(ch,carol4[up2++],0,SCMD_HOLLER);
        if(up2 > 17)
          ch->char_specials.timer = 0;
        break;
      }
    }

  return 1;
}
 

/* proc for rollerblades.. pretty much the same as toboggan - Vader */
#define BLADES_VNUM 22328

void StopBladin(struct obj_data *blades, struct char_data *idiot) {
  GET_OBJ_VAL(blades,0) = GET_OBJ_VAL(blades,1) = GET_OBJ_VAL(blades,2) = 0;
  if((!GET_OBJ_VAL(blades,3)) == 1)
    REMOVE_BIT(PRF_FLAGS(idiot),PRF_BRIEF);
  REMOVE_BIT(AFF_FLAGS(idiot),AFF_SNEAK);
} 

SPECIAL(roller_blades)
{
//  extern char *dirs[];
  ACMD(do_move);
  struct obj_data *blades = (struct obj_data *) me;
  struct char_data *idiot;

  if(cmd > 0 && GET_OBJ_VAL(blades,0) == 1)
    return 1;
  if(cmd > 0 && !CMD_IS("north") && !CMD_IS("south") && !CMD_IS("east") &&
                !CMD_IS("west"))
    return 0;

  if(blades->worn_on == WEAR_FEET && blades->worn_by)
    idiot = blades->worn_by;
  else
    return 0;

  if(FIGHTING(idiot) || !AWAKE(idiot))
    return 0;

  if(cmd > 0) {
    if(!CAN_GO(idiot,cmd-1))
      return 0;
    GET_OBJ_VAL(blades,0) = 1;
    GET_OBJ_VAL(blades,1) = cmd;
    if(PRF_FLAGGED(idiot,PRF_BRIEF)) /* check if they already have brief */
      GET_OBJ_VAL(blades,3) = 1; /* if so store it so we dont get rid of it */
    else
      GET_OBJ_VAL(blades,3) = 0;
    SET_BIT(PRF_FLAGS(idiot),PRF_BRIEF);
    SET_BIT(AFF_FLAGS(idiot),AFF_SNEAK);
    while(idiot->followers)
      stop_follower(idiot->followers->follower);
    }

  if(GET_OBJ_VAL(blades,0) == 1) {
    if(!CAN_GO(idiot,GET_OBJ_VAL(blades,1) - 1)) {
      sprintf(buf,"$n manages to stop rolling by slamming straight into the %s wall!",
              dirs[GET_OBJ_VAL(blades,1)-1]);
      act(buf,FALSE,idiot,0,0,TO_ROOM);
      send_to_char("You roll straight into something painfully solid. But at least you've stopped!\r\n",idiot);
      StopBladin(blades,idiot);
      return 1;
      }
    sprintf(buf,"$n rolls unsteadily %s.",dirs[GET_OBJ_VAL(blades,1)-1]);
    act(buf,FALSE,idiot,0,0,TO_ROOM);
    do_move(idiot,"",GET_OBJ_VAL(blades,1),GET_OBJ_VAL(blades,1));
    act("$n rolls into the room.",FALSE,idiot,0,0,TO_ROOM);
    if(number(0,6) == 1) {
      StopBladin(blades,idiot);
      act("$n loses $s balance and falls flat on $s face!",FALSE,idiot,0,0,TO_ROOM);
      send_to_char("You lose your balance and fall flat on your face!\r\n",idiot);
      GET_POS(idiot) = POS_STUNNED;
      }
    return 1;
    }
  return 0;
}


/* proc for the toboggan to make it go south - Vader */
#define TOB_VNUM 22327
#define TOB_ROOM 5

SPECIAL(toboggan)
{
  ACMD(do_move);
  struct obj_data *tob = (struct obj_data *) me;
  struct char_data *rider, *vict;
  bool first = FALSE;
  int num;

  if(cmd > 0 && GET_OBJ_VAL(tob,0) == 1)
    return 1;
  if(cmd > 0 && !CMD_IS("sit"))
    return 0;


  if(tob->worn_on == WEAR_HOLD && tob->worn_by) {
    rider = tob->worn_by;

    if(FIGHTING(rider) || !AWAKE(rider))
      return 0;


    if(CMD_IS("sit")) {
/* stop ppl tobogganin anywhere */
      if(world[rider->in_room].number != TOB_ROOM) {
        send_to_char("You can only tobbogan from the intersection of Southbank and Main.\r\n",ch);
        return 0;
        }
  
      while(rider->followers)
        stop_follower(rider->followers->follower);

      GET_OBJ_VAL(tob,0) = 1;
/* check n store if they had brief on before they started */
      if(PRF_FLAGGED(rider,PRF_BRIEF)) {
        GET_OBJ_VAL(tob,1) = 1;
      } else GET_OBJ_VAL(tob,1) = 0;
/* set brief on so you dont get flooded with room descs */
      SET_BIT(PRF_FLAGS(rider),PRF_BRIEF);
/* set sneak so ppl dont see the normal arrive n leaf messages */
      SET_BIT(AFF_FLAGS(rider),AFF_SNEAK);
      first = TRUE;
      act("$n sits down on $s tobbogan and launches $mself down the hill.",FALSE,rider,0,0,TO_ROOM);
      send_to_char("You sit on your toboggan and push off down the hill.\r\n",rider);
      GET_POS(rider) = POS_SITTING;
      }

    if(GET_OBJ_VAL(tob,0) == 1)
      if(!CAN_GO(ch,SOUTH)) {
        GET_OBJ_VAL(tob,0) = 0;
        act("$n slides into the room on a toboggan and skids to a halt.",FALSE,rider,0,0,TO_ROOM);
        send_to_char("You skid to a halt. What a ride!\r\n",rider);
        if(GET_OBJ_VAL(tob,1) != 1)
          REMOVE_BIT(PRF_FLAGS(rider),PRF_BRIEF);
        GET_OBJ_VAL(tob,1) = 0;
        REMOVE_BIT(AFF_FLAGS(rider),AFF_SNEAK);
        return 1;
      } else {
        if(!first)
          act("$n flies through the room on a toboggan!",FALSE,rider,0,0,TO_ROOM);
        do_move(rider,"",SOUTH+1,SCMD_SOUTH);
        num = number(0,8);
        switch(num) {
         case 0:
          act("$n flies into the room on a toboggan!",FALSE,rider,0,0,TO_ROOM);
          act("$n hits a rough patch of snow resulting in a spectacular crash!",FALSE,rider,0,0,TO_ROOM);
          send_to_char("You flip your toboggan and end up face down in the snow!\r\n",rider);
          act("The onlookers cheer as $n lays stunned in the snow.",FALSE,rider,0,0,TO_ROOM);
          act("$e looks to be in a lot of pain.",FALSE,rider,0,0,TO_ROOM);
          send_to_char("The onlookers cheer at the sight of your spectacularly painful crash...\r\n",rider);
          break;
         case 1:
         case 2:
          for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
            if((vict != ch) && !(number(0,2)))
            break;
            }
          if(vict == NULL)
            return 1;
          act("$n flies into the room in a toboggan!",FALSE,rider,0,0,TO_ROOM);
          act("You wince as $e smashes into $N with a bonecrunching crack!",FALSE,rider,0,vict,TO_NOTVICT);
          act("$n and $N lay in a broken heap surrounded by teeth and blood. Coool!",FALSE,rider,0,vict,TO_NOTVICT);
          act("$e doesn't seem to even attempt to avoid smashing into you!",FALSE,rider,0,vict,TO_VICT);
          send_to_char("There is a painful crack followed by alot of stars, and some loose teeth...\r\n",vict);
          act("$N doesn't even attempt to get out of you way!",FALSE,rider,0,vict,TO_CHAR);
          act("You smash straight into $M, almost killing you both!",FALSE,rider,0,vict,TO_CHAR);
          act("The pain is amazing. You seem to have landed on $N's weapon...\r\nAt least, you hope thats what it is...",FALSE,rider,0,vict,TO_CHAR);
          GET_POS(vict) = POS_STUNNED;
          break;
       }
       if(num < 3) { 
          GET_OBJ_VAL(tob,0) = 0;
          GET_POS(rider) = POS_STUNNED;
          if(GET_OBJ_VAL(tob,1) != 1)
            REMOVE_BIT(PRF_FLAGS(rider),PRF_BRIEF);
          GET_OBJ_VAL(tob,1) = 0;
          REMOVE_BIT(AFF_FLAGS(rider),AFF_SNEAK);
          }
        return 1;
        }
    }
  return 0;
}


/* frisbee proc that send it randomly to another goon - Vader */
SPECIAL(frisbee)
{
  struct obj_data *frisb = (struct obj_data *) me;
  struct char_data *vict;
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
  char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
  int num_players = 0, player = 0, j;
  struct obj_data *dummy;

  if (!(cmd) || !(ch))
    return 0;

  if(!CMD_IS("throw"))
    return 0;

  two_arguments(argument,arg1,arg2);

  if(strcmp(arg1,"frisbee"))
    return 0;

  if(!frisb->carried_by)
    return 0;

  if(!*arg2) {
    for(i = descriptor_list;i; i = i->next)
      if(i->character && CAN_SEE(ch,i->character))
        num_players++;
    player = number(1,num_players);
    i = descriptor_list;
    for(j = 1; j < player; j++)
      if(i->character && CAN_SEE(ch,i->character))
        i = i->next;
    vict = i->character;
  } else {
    generic_find(arg2,FIND_CHAR_WORLD,ch,&vict,&dummy);
    }
  if(vict == NULL) {
    send_to_char("Who to dickhead?!\r\n",ch);
    return 1;
    }

  obj_from_char(frisb);
  obj_to_char(frisb,vict, __FILE__, __LINE__);
  act("$n hurls a frisbee.",FALSE,ch,0,0,TO_ROOM);
/*  sprintf(buf,"You hurl the frisbee at %s.\r\n",GET_NAME(vict));
  send_to_char(buf,ch); */
  act("You hurl the frisbee at $N.",FALSE,ch,0,vict,TO_CHAR);
/*  sprintf(buf,"*BONK!* You are slapped in the face with a frisbee compliments of %s.\r\n",GET_NAME(ch));
  send_to_char(buf,vict); */
  act("*BONK!* You are slapped in the face with a frisbee compliments of $N.",FALSE,vict,0,ch,TO_CHAR);
  act("$n catches a frisbee in $s mouth!",FALSE,vict,0,0,TO_ROOM);
  return 1;
}



/* proc thats lets ya play snap with up to 4 people - Vader */

const char *card_names[] =
{
  "Joker",
  "Ace",
  "Two",
  "Three",
  "Four",
  "Five",
  "Six",
  "Seven",
  "Eight",
  "Nine",
  "Ten",
  "Jack",
  "Queen",
  "King",
  "\n"
};

#define SNAP_STARTING 1
#define SNAP_PLAYING  2

SPECIAL(snap)
{
  ACMD(do_gen_comm);
  struct obj_data *game = (struct obj_data *) me;
  static struct char_data *players[4];
  static int cards[5][53];
  static int index[5];
  int i,j,k;
  static int num_players, gameon, whos_next;
  char deck[52] = {
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};


  if (!(cmd) || !(ch))
    return 0;

  if(!CMD_IS("enter") && !CMD_IS("go") && !CMD_IS("snap") && 
     !CMD_IS("look") && !CMD_IS("exits"))
    return 0;

  if(CMD_IS("enter")) {
    if((GET_OBJ_VAL(game,0) == SNAP_STARTING) && gameon && (ch == players[0])) { 
      while(index[0] > 0)  /* shuffle */
        for(i=1;i<=num_players;i++)
          if(index[0] > 0) {
            j = number(1,index[0]);
            index[i]++;
            cards[i][index[i]] = cards[0][j];
            for(k=j;k<index[0];k++)
              cards[0][k] = cards[0][k+1];
            cards[0][index[0]] = 0;
            index[0]--;
            }
      act("Snap has started. $n goes first.",FALSE,players[0],0,0,TO_ROOM);
      send_to_char("Snap has started. You go first!\r\n",players[0]);
      whos_next = 1;
      GET_OBJ_VAL(game,0) = SNAP_PLAYING;
      gameon = 2; /* made this 2 for the look bit.. only hasta be >0 for rest to work */
      return 1;
    } else if(gameon && GET_OBJ_VAL(game,0) == 0) {
      send_to_char("There is already a game in progress.\r\n",ch);
      return 1;
    } else if(GET_OBJ_VAL(game,0) == 0) {
      GET_OBJ_VAL(game,0) = SNAP_STARTING;
      gameon = 1;
      for(i=0;i<52;i++)
        cards[0][i+1] = deck[i];
      index[0] = 52;
      for(i=1;i<5;i++)
        index[i] = 0;
      players[0] = ch; /* starter is player1 */
      for(i=1;i<=3;i++)
        players[i] = NULL;
      num_players = 1;
      send_to_char("The game is starting. Type 'enter' again when ready.\r\n",players[0]);
      act("Snap is starting! Type 'go' to join in!",FALSE,ch,0,0,TO_ROOM);
      return 1;
      }
    }

  if(CMD_IS("go")) {
    if(GET_OBJ_VAL(game,0) == SNAP_STARTING) /* join the game */
      if((num_players < 4) && (players[0] != ch) && (players[1] != ch) &&
         (players[2] != ch) && (players[3] != ch)) {
        players[num_players] = ch;
        num_players++;
        send_to_char("Welcome to the game!\r\n",ch);
        act("$n is in for a snappin!",FALSE,ch,0,0,TO_ROOM);
        return 1;
        }
    if(GET_OBJ_VAL(game,0) == SNAP_PLAYING) {
      for(i=1;i<=num_players;i++)
        if((players[i-1] == ch) && (whos_next == i)) {
          index[0]++;
          cards[0][index[0]] = cards[i][index[i]];
          cards[i][index[i]] = 0;
          index[i]--;
          sprintf(buf,"%s flips a %s.",GET_NAME(ch),card_names[cards[0][index[0]]]);
          act(buf,FALSE,ch,0,0,TO_ROOM);
          sprintf(buf,"You flip a %s.\r\n",card_names[cards[0][index[0]]]);
          send_to_char(buf,ch);
          if(whos_next < num_players) /* move to next goons go */
            whos_next++;
          else whos_next = 1;
          act("$n's turn.",FALSE,players[whos_next-1],0,0,TO_ROOM);
          send_to_char("Your turn!\r\n",players[whos_next-1]);
          while(index[whos_next] == 0) {
            send_to_char("You are out!\r\n",players[whos_next-1]);
            act("$n is out!",FALSE,players[whos_next-1],0,0,TO_ROOM);
            if(whos_next < num_players) { /* dump losers n move others */
              for(j=whos_next;j<num_players;j++) {
                players[j-1] = players[j];
                for(k=1;k<=index[j+1];k++) {
                  cards[j][k] = cards[j+1][k];
                  cards[j+1][k] = 0;
                  }
                index[j] = index[j+1];
                }
              players[num_players] = NULL;
            } else players[whos_next-1] = NULL;
            if(whos_next == num_players)
              whos_next = 1;
            num_players--;
            if(!(index[1] == 0 && index[2] == 0 && index[3] == 0 && 
                 index[4] == 0)) {
              if(num_players < 2) {
                gameon = 0;
                GET_OBJ_VAL(game,0) = 0;
                sprintf(buf,"%s has won the game!",GET_NAME(players[0]));
                act(buf,FALSE,ch,0,0,TO_ROOM);
                send_to_char("YOU WON!\r\n",players[0]);
              } else {
                act("$n's turn.",FALSE,players[whos_next-1],0,0,TO_ROOM);
                send_to_char("Your turn!\r\n",players[whos_next-1]);
                }
            } else {
              gameon = 0;
              GET_OBJ_VAL(game,0) = 0;
              send_to_room("Everyone's out! Ya buncha losers!\r\n",ch->in_room);
              return 1;
              }
            }
          return 1;
          }
      }
    }

  if(CMD_IS("look")) {
    one_argument(argument,buf);
    if((!(strcmp(buf,"deck")) || !(strcmp(buf,"cards"))) && gameon == 2) {
      sprintf(buf,"Vader's Snap Game thing...\r\n--------------------------\r\n\r\n");
      if(index[0] > 0)
        sprintf(buf,"%s%d cards on the pile. Card showing is a %s.\r\n\r\n",buf,index[0],card_names[cards[0][index[0]]]);
      else
        sprintf(buf,"%s%d cards on the pile.\r\n\r\n",buf,index[0]);
      for(i=0;i<num_players;i++)
        sprintf(buf,"%s%s has %d cards.\r\n",buf,GET_NAME(players[i]),index[i+1]);
      sprintf(buf,"%s\r\nIt's %s's turn.\r\n",buf,GET_NAME(players[whos_next-1]));
      send_to_char(buf,ch);
      return 1;
    } else return 0;
    }

  if(CMD_IS("snap") && (ch == players[0] || ch == players[1] ||
                        ch == players[2] || ch == players[3])) {
    do_gen_comm(ch,"SNAP!",0,SCMD_HOLLER);
    if((cards[0][index[0]] == cards[0][index[0]-1]) && (index[0] > 0)) {
      act("$n slams $s hand down on the pile!",FALSE,ch,0,0,TO_ROOM);
      sprintf(buf,"%s picks up %d cards.",GET_NAME(ch),index[0]);
      act(buf,FALSE,ch,0,0,TO_ROOM);
      sprintf(buf,"You pick up %d cards.\r\n",index[0]);
      send_to_char(buf,ch);
      GET_MOVE(ch) += 50;
      for(i=0;i<num_players;i++) /* give em the cards off the pile */
        if(players[i] == ch) {
          for(j=index[0];j>0;j--) { /* put em in backwards so match aint 1st */
            index[i+1]++;
            cards[i+1][index[i+1]] = cards[0][j];
            }
          index[0] = 0;
          whos_next = i+1; /* snapper gets next turn */
          send_to_char("Your turn!\r\n",ch);
          }
    } else send_to_char("Open your eyes! That's not a match!\r\n",ch);
    return 1;
    }

  if(CMD_IS("exits") && gameon && !LR_FAIL(ch, LVL_GOD) || 
     (ch == players[0] || ch == players[1] ||
      ch == players[2] || ch == players[3])) {
    send_to_char("Game reset.\r\n",ch);
    act("Snap has been reset by $n.",FALSE,ch,0,0,TO_ROOM);
    gameon = 0;
    GET_OBJ_VAL(game,0) = 0;
    for(i=0;i<4;i++)
      players[i] = NULL;
    return 1;
    }

  return 0;
}


/* stupid easter bunny proc thing - Vader */

#define EB_EGG    22347
#define EB_BUN    22348

SPECIAL(easter_bunny)
{
  ACMD(do_say);
  ACMD(do_gen_comm);
  ACMD(do_give);
  ACMD(do_eat);
  struct char_data *vict;
  struct obj_data *obj;
  int i;

  if(cmd)
    return 0;

  if(FIGHTING(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);

  for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    if((vict != ch) && !(number(0,3)))
      break;
    }

  switch(number(0,50)) {
    case 0:
    case 20:
    case 21:
  if(vict == NULL)
    return 0;
      obj = read_object(EB_EGG,VIRTUAL);
      obj_to_char(obj,ch, __FILE__, __LINE__);
      sprintf(buf,"egg %s",GET_NAME(vict));
      do_give(ch,buf,0,0);
      do_say(ch,"Happy Easter!",0,0);
      do_eat(ch,"egg",0,0);
      break;
    case 1:
    case 30:
    case 31:
  if(vict == NULL)
    return 0;
      obj = read_object(EB_BUN,VIRTUAL);
      obj_to_char(obj,ch, __FILE__, __LINE__);
      do_say(ch,"Here! Have a hot cross bun!",0,0);
      sprintf(buf,"bun %s",GET_NAME(vict));
      do_give(ch,buf,0,0);
      do_eat(ch,"bun",0,0);
      break;
    case 2:
      act("$n chews on a carrot.",FALSE,ch,0,0,TO_ROOM);
      do_say(ch,"What's up Doc?",0,0);
      break;
    case 3:
      do_gen_comm(ch,"Merry Easter and a Happy New Year!.. err.. or something..",0,SCMD_HOLLER);
      break;
    case 4:
      do_gen_comm(ch,"Show me the bunny!",0,SCMD_HOLLER);
      break;
    case 5:
      do_gen_comm(ch,"Happy Easter everyone!",0,SCMD_HOLLER);
      break;
    case 6:
      do_gen_comm(ch,"Stupid fools! Theres a razor blade in every 6th egg!",0,SCMD_HOLLER);
      do_gen_comm(ch,"Errr did I say that out loud? I meant enjoy these lovely, safe to eat, no razor blades in site, easter eggs! They're damn tasty!",0,SCMD_HOLLER);
      break;
    case 7:
  if(vict == NULL)
    return 0;
      sprintf(buf,"Stop touching me %s! I'm not a Playboy bunny!!",GET_NAME(vict));
      do_gen_comm(ch,buf,0,SCMD_HOLLER);
      break;
    case 8:
  if(vict == NULL)
    return 0;
      i = number(10,9999);
      sprintf(buf,"Cheez! %s just ate %d Easter Eggs in a row! I'm sickened.",GET_NAME(vict),i);
      do_gen_comm(ch,buf,0,SCMD_HOLLER);
      break;
    case 9:
      do_gen_comm(ch,"Got a toothache from eating too many eggs? SUFFER!",0,SCMD_HOLLER);
      break;
    case 10:
      do_gen_comm(ch,"Pah! Go blow a goat!",0,SCMD_HOLLER);
      break;
    }

  return 1;
}

/* proc to transport you to westworld when hat is worn - Vader */
#define HAT_TARGET_ROOM 13200

SPECIAL(cowboy_hat)
{
  struct obj_data *hat = (struct obj_data *) me;

  if(hat->worn_on == WEAR_HEAD && hat->worn_by) {
    send_to_char("Your cowboy hat seems a little big for you.\r\n"
                 "It slides down over your eyes...\r\n",hat->worn_by);
    char_from_room(hat->worn_by);
    char_to_room(hat->worn_by,real_room(HAT_TARGET_ROOM));
    extract_obj(hat);
    return 1;
    }
  return 0;
}

SPECIAL(alien_voice)
{
  struct char_data *vict, *mob, *in_room;
  int mob_num, echos_in_room=0;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  
  if (number(0, 2))
    return TRUE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);
  
  /* random chance that cleric mob will heal itself */
  if (!number(0, 4))
  {
    int level = GET_LEVEL(ch);

    if (level > 16)
    {
      cast_spell(ch, ch, NULL, SPELL_HEAL);
      return TRUE;
    }
  }

  /* chance that mob will conjure some help */
  if (!number(0, 1))
  { 
    mob_num = 27040;

    /* Limit the amount of echos to 3 */
    for (in_room = world[ch->in_room].people; in_room; in_room=in_room->next_in_room) {
      if (IS_NPC(in_room) && (GET_MOB_VNUM(in_room) == mob_num))
        echos_in_room++; 
    }
  
    if (echos_in_room >=3)
      return TRUE;

    /* load the mob and set it fighting */
    mob = read_mobile(mob_num, VIRTUAL);
    char_to_room(mob, ch->in_room);
    act("$n calls upon his protectors for assistance.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n appears from nowhere and willingly joins the fight!!!", FALSE, mob, 0, 0, TO_ROOM);
    /* Put the victim into the voices memory */ 
    remember(mob,vict);
    hit(mob, vict, TYPE_UNDEFINED);
    return TRUE;
  }

/* other wise just cast some spells */

  switch (GET_LEVEL(ch)) {
  default:
    cast_spell(ch, vict, NULL, SPELL_PLASMA_BLAST);
    break;
  }
  return TRUE;
}

SPECIAL(alien_voice_echo)
{
  struct char_data *the_echo = (struct char_data *) me;
  struct char_data *vict;
  memory_rec *names;
  int found=0;

  if (cmd)
    return FALSE;

  switch (GET_POS(ch)) {
    case POS_STANDING:

      /* Check all people in memory */
      for (names=MEMORY(the_echo); names && !found; names=names->next) {

	/* Extract the victim */
        if (!(vict= (struct char_data *)get_player_by_id(names->id) )) 
          continue;

        /* Check if we can attack - PC, can see, Not NO_HASSLE, not linkless */ 
        if (IS_NPC(vict) || !CAN_SEE(the_echo, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) || (!vict->desc))
          continue;	/* Try next victim in memory */

        /* Check if target is in room */
        if (IN_ROOM(vict) == IN_ROOM(the_echo)) {
	  found=1;
          act("'No-one attacks the Alien Voice and gets away with it!', exclaims $n.",
	      FALSE, the_echo, 0, 0, TO_ROOM);
          hit(the_echo, vict, TYPE_UNDEFINED);

        /* Target not in room */
        } else {

          /* Check The Echo can goto the victim */
          if (!ROOM_FLAGGED(IN_ROOM(vict),ROOM_NOMOB | ROOM_DEATH | ROOM_PEACEFUL | ROOM_TUNNEL) && 
	      (world[vict->in_room].zone != 5)) {
 
    	    act("$n looks like they have something on their mind, they disappear!",FALSE,the_echo,0,0,TO_ROOM);
            char_from_room(the_echo);
            char_to_room(the_echo, vict->in_room);
            act("$n of the alien voice come wavering in front of you..",FALSE,the_echo,0,0,TO_ROOM); 

	    found=1;
            act("'No-one attacks the Alien Voice and gets away with it!', exclaims $n.",FALSE,the_echo,0,0,TO_ROOM);
            hit(the_echo, vict, TYPE_UNDEFINED);

	  /* Cant get to victim, tell them they can't escape */
	  } else {
	    switch(number(0,10)) {
              case 1: case 2: case 3: case 4:
    	        act("$n tells you, 'You can't escape me, nobody hurts the Alien Voice!'",FALSE,the_echo,0,vict,TO_VICT);
                break;
            } /* End of switch tell */
          } /* End of cant goto */
        } /* End of target not in room */
      } /* End of memory for loop */

      /* Keep the mobs in ROOM_ALIEN_ECHO */
      if (!found && (world[the_echo->in_room].number != ROOM_ALIEN_ECHO)) {
        act("$n looks like they have something on their mind, they disappear!",FALSE,the_echo,0,0,TO_ROOM);
        char_from_room(the_echo);
        char_to_room(the_echo, real_room(ROOM_ALIEN_ECHO));
        act("$n of the alien voice come wavering in front of you..",FALSE,the_echo,0,0,TO_ROOM); 
      }
      break;
    case POS_FIGHTING:
      break; 
  } 
  return TRUE;
}
/* TALIPROCS.C
 *		Author		: 	Jason Theofilos
 *		Date Started	: 	16th September,1999
 *		Last Revision	:	2nd October, 1999
 *		Purpose		:	Defines procedures for Talisman's zone requirements
 *
 *		Language	:	C
 *		Location	:	Churchill, Gippsland, Australia.
 */

/*
 *	LAST REVISION:
 *			Changed room_magic_ripple so that spell parsing works properly.
 *			Put in a check in enter_reality_rip for valid rooms
 *
 */

/* SPEC PROC:	ROOM
 *	Checks if the person in the room is
 *   	trying to cast a spell, and has a chance
 *	of warping it.
 *	-- Doens't check if spellname is valid,
 *	why bother, the do_cast will catch it --
 */		
SPECIAL(room_magic_ripple)
{
  ACMD(do_cast);

  char *s;			// Pointer to char string
  int index;			// Counter

  if (!(cmd) || !(ch))
    return FALSE;

  if(IS_NPC(ch))
    return (FALSE); 	// Strictly player biased 

	// Check if command is what we're after
	if( CMD_IS("cast") ) {
		if( !*argument ) 		// If no argument, it's a dud, return
			return (FALSE);

		two_arguments(argument, buf1, buf2);

		// Randomly choose a number, if it's a 1, we warp the spell
		switch( number(1, 7) ) {
			case 1: // Tell player their spell is warped, and room too.
				send_to_char("Your magic is warped by the room's aura!\r\n", ch);
				act("$n's spell casting is garbled and changed by the room's aura.",
					FALSE, ch, 0, 0, TO_ROOM);

				// If they're not fighting, do a non-combat warp
				if( GET_POS(ch) != POS_FIGHTING ) {
					switch( number(1, 5) ) {
						case 1:strcpy(buf1, " 'invisibility' "); break;
						case 2:strcpy(buf1, " 'protection from evil' ");break;
						case 3:strcpy(buf1, " 'detect alignment' "); break;
						case 4:strcpy(buf1, " 'cloud kill' ");break;
						case 5:strcpy(buf1, " 'create food' ");break;
						default : strcpy(buf1, " 'detect invisibility' "); break;
					}
					strcat(buf1, buf2);
					do_cast(ch, buf1, GET_LEVEL(ch), 1);
					return (TRUE);
				}
				
				// Fighting.

				// If not a target spell, probably something nice, so cast on opponent
				// (Bad luck for us if it's a room affect spell - Be nice, let them
				//  have it)
				if( !*buf2 ) {
					// Spell name
					s = strtok(argument, "'");
					if( s == NULL )
						return (FALSE); // Dud
					s = strtok(NULL, "'");
					if( s == NULL )
						return (FALSE); // Dud

                        //              skill_num = find_skill_num(s);
			//		if ((skill_num = find_skill_num(s)) < 0)
			//			return (FALSE); // Dud

					// Get the full spell name
			//		strcpy(buf2, skill_name(find_skill_num(s))); 

                        //                strcpy(buf2, spells[skill_num]);

					// Get spell name
					 for( index = 1; index <= TOP_SPELL_DEFINE; index++ ) 
				        	if( is_abbrev(s,spell_info[index].name)) 
							strcpy(buf2, spell_info[index].name);
						
					// Get opponent
					one_argument( ( FIGHTING(ch)->player.name) ,buf);
				
					// Reconstruct the argument
					strcpy(argument, " '");
					strcat(argument, buf2);
					strcat(argument, "' ");
					strcat(argument, buf); 
	
					// Give it to the normal spell parser				
					return (FALSE); // ho ho ho
				} // End no target

				// Okay, they're targetting something, lets give them a surprise			
				switch( number(1,5) ) {
					case 1: strcpy(argument, " 'magic missile' "); break;
					case 2: strcpy(argument, " 'burnings hands' "); break;
					case 3: strcpy(argument, " 'cure light' "); break;
					case 4: strcpy(argument, " 'cloud kill' "); break;
					case 5: strcpy(argument, " 'whirlwind' "); break;
					default: strcpy(argument, " 'cure critic' "); break;					
				}								
			
				strcat( argument, buf2 );
		
				// Send it on, done our dirty work
				return (FALSE);

			default: return (FALSE);	// Spell is not warped, return
		}
	}

	return (FALSE);	// Command wasn't CAST

}

bool has_exit(room_rnum rnum) {
  for (int i = 0; i < NUM_OF_DIRS; i++)
    if (W_EXIT(rnum, i)->to_room != NOWHERE)
      return TRUE;

  return FALSE;
}

/* Sub Function for: SPECIAL(room_magic_unstable)
 *	Given a character, transports them to a random
 *	room nearby.
 */	
void enter_reality_rip(struct char_data *ch ) {

	sh_int chroom = ch->in_room, roomokay = 0;
	int counter = 0;	// Used for preventing endless looping, in case

	char_from_room(ch);			// Disappear
	while( roomokay <= 0 && counter < 10 ) {    // Find a room to re-appear into
		roomokay = world[chroom].number - (number(1,25));// Move themdown,rather thanup

		// Validate the room 
		if( world[real_room(roomokay)].number <= 0 ) {
			roomokay = 0;
		}
		else {	// Room exists

		  // Check that we're still in the same zone
		  if( (zone_table[ world[real_room(roomokay)].zone].number) 
			!=  (zone_table[ world[chroom].zone].number) ) {
			roomokay = world[chroom].number + number(1,25);	// Try again, move them up
			
			if( ((zone_table[ world[real_room(roomokay)].zone].number) 
				!= (zone_table[ world[chroom].zone ].number)) ||
			     (world[real_room(roomokay)].number <= 0) || !has_exit(roomokay)) 
				roomokay = 0;			// Can't move them, try again
		  
		  } // If room is in same zone
		} // If room exists
		counter++;					// Limit number of tries
	}

	// More than ten tries? Put them back.
	if( counter >= 10 ) {
		// Fix room up, to original room
		char_to_room(ch, chroom);
		act("$n re-appears from nowhere, dropping to the floor.", FALSE, ch, 0,0,TO_ROOM);
		send_to_char("Strange, this room looks familiar!?\r\n", ch);
		return;
	}

	basic_mud_log ("Moving char to room %d.", roomokay);	
	char_to_room( ch, real_room(roomokay));			// Move character
	send_to_char("...you feel displaced.\r\n",ch);
	look_at_room(ch, 1);				// Show them the room
	act("A rip in reality tears open, and $n drops through!", FALSE, 
	ch, 0, 0, TO_ROOM);				// Tell the room of arrival
}


/* SPEC PROC :	ROOM
 *	If someone tries to cast a spell in a room with this
 *	procedure, there is a chance that they will be teleported
 *	around the zone, randomly. 
 */	
SPECIAL(room_magic_unstable)
{

  if (!(cmd) || !(ch))
    return FALSE;

	if( IS_NPC(ch) )
		return (FALSE);


	// If command is appropriate
	if( CMD_IS("cast") ) {
		// Get text
		one_argument(argument, buf1);
		// Tsk, they're just being silly
		if( !*buf1 )
			return (FALSE);
		// Random chance of teleport from room
		switch( number(1, 15) ) {
			case 1:	 send_to_char("While invoking your magic, you accidentally touch a reality rip!\r\n", ch);
				 act("$n's magic is disrupted by a reality rip here, sending $m reeling through the universe.",
					FALSE, ch, 0, 0, TO_ROOM);
				 enter_reality_rip(ch); 	// Call function to do real work
				 return (TRUE);
			default: return (FALSE);		// Caster is safe, this time
		}
	}

	return (FALSE);	// Not appropriate command
}


/* SPEC PROC :	ROOM
 *	Creates the Modified Titan's suit, given special 
 * 	directives and conditions.
 */
SPECIAL(titansuit)
{

	struct obj_data *i, *ham, *hand;			// Temp containers

	if (!(cmd) || !(ch))
	  return FALSE;

	// If character is attempting to use something
	if( CMD_IS("use") ) {
		two_arguments(argument, buf1, buf2);		// Get the args
		if( !*buf1 || !*buf2 )				// We need both arguments to be valid
			return (FALSE);

		// If the first argument is thor's hammer
		if( strcmp(buf1, "hammer") == 0 || strcmp(buf1, "thors") == 0 ) {
			// and teh second item is the divine anvil
			if( strcmp(buf2, "anvil") == 0 || strcmp(buf2, "divine") == 0 ) {
				// look for items in room
				for( i = world[ch->in_room].contents; i; i = i->next_content ) {
					// If the right anvil is there (the one with the suit)
					if( strcmp(i->name, "divine anvil titan shell") == 0 ) {
						// Check player's eq
						ham = GET_EQ(ch, 16);
						hand = GET_EQ(ch, 17);
						if( (strcmp(ham->name, "Thors Hammer") == 0) &&
						    (strcmp(hand->name, "hand fate") == 0) ) {
							create_suit(ch);	// Create item
							return (TRUE);		// Done, get out
						} // If right items
					
					} // If correct anvil is present
				} // For every item in room
				
			} // If second command is divine anvil
		} // If first item is thors hammer
	}

	return (FALSE);		// Invalidate, let the interpreter take it.
}

/* Subfunction for SPECIAL(titansuit)
 *	Removes items that are necessary for the object 
 *	to be created, from the user, and then replaces
 *	the anvil with the item.
 */
void create_suit(struct char_data *ch) {

	struct obj_data *i;
	struct descriptor_data *p;

	// Remove the anvil
	for( i = world[ch->in_room].contents; i; i = i->next_content ) {	
		if( strcmp(i->name, "divine anvil titan shell") == 0 ) {
			extract_obj(i);
			break;
		}
	}

	// Remove the hand, and the hammer
	extract_obj(GET_EQ(ch, 16));
	extract_obj(GET_EQ(ch, 17));

	// Create the suit
	i = read_object(10209, VIRTUAL);
	obj_to_room(i, ch->in_room);

	act("$n uses Thors Hammer with the Hand of Fate to forge an item on the Divine Anvil!",
		FALSE, ch, 0, 0, TO_ROOM);

	// Inform the world
	for( p = descriptor_list; p; p = p->next ) 
		if( STATE(p) == CON_PLAYING && p->character )
		send_to_char("You feel the balance of the world shift subtly.\r\n", p->character);
	
	// Tell the player
	send_to_char("Wielding Thor's Hammer with the Hand of Fate, you strike at the anvil!\r\n", ch);
	send_to_char("...at your feet lies a newly forged artifact of immense power.\r\n", ch);
	
}

/* SPEC PROC :	MOB
 *	If set, this gives the mob a chance of 
 *	throwing a mob into a specific DT
 */ 	
SPECIAL(fate) 
{
  struct char_data *opponent;

  // Command? No Thanks.
  if (cmd) 
    return (FALSE);

  // Have to be fighting
  if(GET_POS(ch) == POS_FIGHTING) 
    switch(number(1, 10)) 
    { // 1 in 10 chance
      case 1:
	// Get the players in the room
	for( opponent = world[ch->in_room].people; opponent; opponent = opponent->next) 
	  if(FIGHTING(ch) == opponent) 
	  {
	    send_to_char("You realise you cannot fight your fate", opponent);
	    send_to_char("... and descend into darkness.\r\n", opponent);
	    char_from_room(opponent);
	    char_to_room(opponent, real_room(10350));
	    look_at_room(opponent, 1);
	    act("$n drops to the floor in a bloody mess.", FALSE, 
		opponent, 0, 0, TO_ROOM);
	    GET_HIT(opponent) = -1;
	    update_pos(opponent);
	    break;
	  }
	// Tell the room
	act("$n recruits another soul for the legion of doom!", 
	    FALSE, ch, 0, 0, TO_ROOM);
	break;
      default:
	break;	// Nothing this time you lucky player
    }
  return (FALSE);	// Get back to it
}

/* SPEC PROC : 	ROOM
 *	If someone tries to 'put'/'use' in this room
 *	the procedure checks the items in case
 *	it's a power gem, and a dark portal.
 */

SPECIAL(darkportal)
{

	struct obj_data *i;
	int sentinel = 0;

	if (!(cmd) || !(ch))
	  return FALSE;

	// Valid command? Yes? No? Mais oui!
	if( CMD_IS("put") || CMD_IS("use") ) {
		// Get the arguments
		two_arguments(argument, buf1, buf2);

		// What's this, not using the correct arguments? 
		if( ( (strcmp(buf1, "power") != 0 ) && (strcmp(buf1, "gem") !=0  )   ) ||
		    ( (strcmp(buf2, "portal") != 0) && (strcmp(buf2, "dark") != 0) ) ) {
			return (FALSE);
		}

		// Check the appropriate dark portal is in the room, we want the dead one
		for( i = world[ch->in_room].contents; i; i = i->next_content ) {
			if( strcmp(i->name, "dark portal") == 0 ) {
				sentinel = 1;	// Found it!
				break;
			}
		}
		
		// Check if it's found
		if( !sentinel )
			return (FALSE);

		// Arguments are okay, check for item
		for( i = ch->carrying; i; i = i->next_content ) {
			if( strcmp(i->name, "power gem") == 0 ) {
			// Character has the appropriate stuff, lets do it.
			   act("$n touches a Power Gem to the dark portal, bringing it back to life.",
					TRUE, ch, 0, 0, TO_ROOM);
			   send_to_char("You place the Power Gem against the portal, giving it life.\r\n", ch);

			   // Power gem gets done in
			   send_to_char("... the gem is consumed in the process.\r\n", ch);
			   extract_obj(i);

			   // Get rid of the old portal
	                   for( i = world[ch->in_room].contents; i; i = i->next_content ) {
			        if( strcmp(i->name, "dark portal") == 0 ) {
					extract_obj(i);
					break;
	               		}
			    }
			 
			   // Put new portal in room 
			    obj_to_room(read_object(10107, VIRTUAL), ch->in_room);
		           
			   return (TRUE);	// Succcccceeeess!
			}
		}
	}

	return (FALSE);	// Let the interpreter take care of this one, we'll catch the next bus
}

/* SPEC PROC :	ROOM
 *	Checks if the character tries to go north
 *	and if they are under level 85, refuses
 */
SPECIAL(pillars)
{

  if (!(cmd) || !(ch))
    return FALSE;

	if( CMD_IS("north") ) {
		if(!LR_FAIL(ch, 85)) {
			act("The pillars glow dully as $n passes between them.", FALSE, ch, 0,0,TO_ROOM);
			send_to_char("The pillars glow briefly, but are unable to affect you.\r\n",ch);
			return (FALSE);		// Dang, too powerful to stop
		}
		strcpy(buf1, "As you try to step between the pillars, a thin ray ");
		strcat(buf1, "of energy shoots out from both pillars, forming and X in front of you ");
		strcat(buf1, "and blocking your progress. Pushing through would be a bad idea.\r\n");
		send_to_char(buf1, ch);
		act("$n tries to pass between the pillars but is prevented by a deadly crossbeam.",
			FALSE, ch, 0, 0, TO_ROOM);
		return (TRUE);
	}

	return (FALSE);	// Not anything we want
}

/* SPEC PROC : MOB
 *	Dummy proc. Just used as a flag, really.
 *	Real work is in function "make_titan_corpse"
 */
SPECIAL(titan)
{

	return (FALSE);	
}

/* Subfunction for SPECIAL(titan)
 *	FUnction is called instead of the standard
 *	make_corpse function, if the character
 *	who died has the TITAN spec. 
 *	.. Instead of making a corpse, makes 
 *	object -- anything on character is lost! --
 */
// This is the actual titan spec proc
void make_titan_corpse(struct char_data *ch)
{
  struct obj_data *obj;			// Container
  obj = read_object(10207, VIRTUAL);	// Read object into container
  obj_to_room(obj, ch->in_room);	// Put it in
}

/* SPEC PROC :	MOB
 *	Just a text spec.
 */

SPECIAL(anxiousleader)
{

	if( cmd || GET_POS(ch) <= POS_SLEEPING || GET_POS(ch) == POS_FIGHTING )
		return (FALSE);

	switch(number(1,20)) {
		
		case 1:	act("$n paces the room worriedly, frustrated by the war.",
				FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 2: act("$n is struct by inspiration, and starts jotting down notes.",
				FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 3: act("$n lays a map out, and begins to study it closely.",
				FALSE, ch, 0, 0, TO_ROOM);		
			break;
		case 4: act("$n declares, 'We move at the break of dawn.'",
				FALSE, ch, 0, 0, TO_ROOM);
			break;
		default: break;
	}

	return (FALSE);		// Done, whether we spoke or not
}

/* SPEC PROC : 	MOB
 *	Packleader mob's do not attack unless they 
 *	have followers that they can call on to help 
 *	them. (So don't set agro flags)
 */
SPECIAL(packleader)
{
  ACMD(do_violent_skill);

  struct follow_type *k;
  struct descriptor_data *d;
  register struct char_data *i;

  if (cmd || GET_POS(ch) <= POS_SLEEPING || GET_POS(ch) == POS_FIGHTING)
    return (FALSE);

  // Check if the pack leader has followers, if not, he's a wimp
  if (!ch->followers)
    return (FALSE);		// BUKARK!

  // Look through player list
  for (d = descriptor_list; d; d = d->next) {
    i = (d->original ? d->original : d->character);

    if (STATE(d) != CON_PLAYING || !CAN_SEE(ch, i) || 
	!LR_FAIL(i, LVL_CHAMP) || IS_EVIL(i)) 
      continue;

    if (i->in_room == ch->in_room ) {		// If player is here
      for (k = ch->followers; k; k=k->next) {	// For every follower
        // If follower is here, and they're not fighting
        if ((ch->in_room == k->follower->in_room) 
               && (GET_POS(k->follower) != POS_FIGHTING)) {	
          act("$n orders his follower to 'kill'.", FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "kill %s", i->player.name);
          command_interpreter(k->follower, buf);
        }
      }
      // Packleader then jumps in too.
      act("$n joins his followers in battle!", FALSE, ch, 0, 0, TO_ROOM);
      do_violent_skill(ch, i->player.name, 0, SCMD_HIT);
      return (TRUE);		// Found a target, return.
    }
  }

  return (FALSE);	// No target, return
}

/* SPEC PROC : 	MOB
 * 	Packmembers will keep an eye out for
 *	mobs with the PACKLEADER spec proc, and join 
 *	them. -- Set these mobs to sentinel, so they
 *	don't leave their master, unless the code is 
 *	changed to charm the mobs. 
 */
SPECIAL(packmember)
{

	struct char_data *i;

	if( cmd || GET_POS(ch) <= POS_SLEEPING || GET_POS(ch) == POS_FIGHTING )
		return (FALSE);

	// Check if we have a master already
	if( ch->master )
		return (FALSE);

	// Look through room
	for( i = world[ch->in_room].people; i; i = i->next ) {
		if( CAN_SEE(ch, i) ) {			// If we can see them
			if( GET_MOB_SPEC(i) == packleader ) { // Found a leader!
				add_follower(ch, i);    // Add us to the master
				ch->master = i;	 	// Set master
				return (TRUE);		// Done, get out
			}
		}
	}

	return (FALSE);	

}

/* SPEC PROC : MOB
 *	Mobs set with this proc, will actively seek out 
 *	players within reachable distance.
 *	It implements similar code to the track function,
 *	but will not go through !MOB rooms. 
 *
 *	Warning: Be sure to have !MOB rooms limiting the
 *	mobs to certain areas, if they're flagged with this.
 *
 */
SPECIAL(playerhunter)
{
  void do_playerhunt(struct char_data *ch, struct char_data *victim);
  int position = GET_POS(ch);
  register struct descriptor_data *d;
  register struct char_data *i;

  if ((cmd) || (position == POS_FIGHTING || position < POS_SLEEPING))
    return(FALSE);

  /* Check if there's someone in our zone
     (Don't be first in a playerhunt zone, or they'll be straight after you) */
  for (d=descriptor_list; d; d=d->next)
  {
    i = (d->original ? d->original : d->character);
    if (STATE(d) != CON_PLAYING || !CAN_SEE(ch, i) || !LR_FAIL(i, LVL_IMMORT))
      continue;
    // One of those PC's! VIVA LA NPC REVOLUTION!
    if (i->in_room == ch->in_room)
    {
      sprintf(buf1, "Your time is at hand %s, defend thyself!", i->player.name);
      do_say(ch, buf1, 0, 0);
      hit(ch, i, TYPE_UNDEFINED);
      return (TRUE);
    }
    // Still looking for someone in the zone
    if (world[i->in_room].zone == world[ch->in_room].zone)
    {
      do_playerhunt(ch, i);  // call function to handle it
      return (TRUE);         // Outta here, success
    }
  }	
  return (FALSE);					
}

/* SPEC PROC :	MOB
 *	Just a text spec.
 */

SPECIAL(arrogance)
{
  // Check states, commands, etc
  if (cmd || GET_POS(ch) <= POS_SLEEPING)
    return (FALSE);

  // Random message 
  switch (number(1, 20))
  {
    case 1:
      act("$n sneers at you, realising how pathetic you are.", 
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      act("$n says, 'What's this? No one will fight me? Craven cowards!'",
	  FALSE, ch, 0,0,TO_ROOM);
      break;
    case 3:
      act("$n starts laughing at you. You don't seem to impress $m much.",
	  FALSE, ch, 0,0, TO_ROOM);
      break;
    case 4:
      act("$n says, 'Your mother has a wooden leg with a kickstand, and your father smells of elderberies!'", FALSE, ch, 0,0,TO_ROOM);
      break;
  }
  return (FALSE);
}

/* SPEC PROC : MOB
 *	Mobs marked with this will drool a bit and say some
 *	weird stuff.
 *	More interestingly, if they're fighting they're re-energised
 *	and healed by the strength of their insanity.
 */
SPECIAL(insanity)
{
  // Check states, commands, etc
  if (cmd || GET_POS(ch) <= POS_SLEEPING)
    return (FALSE);
  // If we're fighting, chance of vitality burst
  if (GET_POS(ch) == POS_FIGHTING)
  {
    if (number(1, 5) == 1)  // 1 in 5 chance of renewed health
    {
      act("$n cackles inanely, finding renewed strength through insanity.",
	  FALSE, ch, 0, 0, TO_ROOM);
      GET_HIT(ch) += number(1, 10 * GET_LEVEL(ch));  // Small random amount
    }
    return (TRUE);
  }
  // Random message, 1 in 5 chance
  switch (number(1, 20))
  {
    case 1:
      act("$n puts $m face in $m hands and starts sobbing hopelessly.",
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      act("$n says, 'How could we have known? ... How? ...'",
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act("$n shouts, 'MADNESS! It cannot be done! .. Can it?'",
	  FALSE, ch, 0, 0, TO_ROOM);
      break;	
    case 4:
      act("$n gibbers and drools, shaking violently.", 
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
  }
  return (TRUE);	
}

/* SPEC PROC : MOB
 * 	Set a mob as an avenger, a holy warrior that seeks
 *	retribution against the world's evils, and
 *	is supported by a deity that casts spells on it
 *	during battle.
 */
SPECIAL(avenger)
{
  // Check states, commands, etc
  if (cmd || GET_POS(ch) <= POS_SLEEPING)
    return (FALSE);
  // If we're fighting
  if (GET_POS(ch) == POS_FIGHTING)
  {
    if (number(1, 10) == 1)
    {			// 1 in 10 chance of magaffect
      act("$n is surrounded by a bright light, and the purity of $m soul is rewarded.", FALSE, ch, 0, 0, TO_ROOM);
      switch (number(1,3))
      {
	case 1:
	  mag_affects(GET_LEVEL(ch), ch, ch, SPELL_BLESS, 1);
	  act(" ... $n's deity blesses $m.", FALSE, ch, 0,0, TO_ROOM);
	  break; 
	case 2:
	  mag_affects(GET_LEVEL(ch), ch, ch, SPELL_ARMOR, 1);
	  act(" ... $n's deity begins to protect $m.", FALSE, ch, 0,0, TO_ROOM);
	  break;
	case 3:
	  mag_affects(GET_LEVEL(ch), ch, ch, SPELL_HEAL, 1);
	  act(" ... $n's wounds are soothed.", FALSE, ch, 0, 0, TO_ROOM);
	  break;
	default:
	  basic_mud_log("Avenger SPEC_PROC out of range value "); break;
      }
    }
    return (TRUE);  // Disable them from doing anything else while fighting
  }
  // Random message, 1 in 5 chance
  switch (number(1, 20))
  {
    case 1:
      act("$n scans the area for signs of evil presence.", 
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 2:
      act("$n shouts, 'Come out fiends, and face the wrath of my blade!'",
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 3:
      act("$n shouts, 'Retribution is at hand dark ones! Your end is nigh!'",
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 4:
      act("$n searches the walls for hidden passageways.",
	  FALSE, ch, 0, 0, TO_ROOM);
      break;
    default:
      break;
  }
  return (FALSE);
}

#if 0
// Artus> Replaced with room special.
/* Subfunction for handle_damage_rooms
 *	Function should be used on rooms that do high amounts of
 *	magical damage to the people in them. Level based, but still
 * 	fairly high.
 * void blast_occupants(sh_int room)
 */ 	
SPECIAL(blast_room)
{
  struct char_data *k;
  struct room_data *room = (struct room_data *)me;
  
  if (cmd) 
    return FALSE;
	
  // Get the people in ze room and give them what-for
  for (k = room->people; k; k = k->next_in_room)
  {
    if (!LR_FAIL(k, LVL_IMMORT) || IS_NPC(k) || 
	(world[IN_ROOM(k)].number != room->number))
      continue;
    island_blast(k, ISBLAST_GAUNTLET);
  }
}

#if 0
// Artus> Replaced with Special.
/* Subfunction for handle_damage_rooms
 *	Essentially a weaker and non magical version of
 *	blast_occupants. 
 */
void shoot_occupants(sh_int room)
#endif
SPECIAL(shoot_room)
{
  struct char_data *k;
  struct room_data *room = (struct room_data *)me;
  
  if (cmd)
    return(0);

  // Get the people in ze room and give them what-for
  for (k = room->people; k; k = k->next)
  {
    if(!LR_FAIL(k, LVL_IMMORT) || IS_NPC(k) || 
       (world[k->in_room].number != room->number))
    continue;
    island_blast(k, ISBLAST_ARROW);
  }
}
#endif

void island_blast(struct char_data *vict, char btype)
{
  switch (btype)
  {
    case ISBLAST_ARROW: /////////////////////////////////////////////// Archers
      act("$n is used for target practice by the archers surrounding this area.", TRUE, vict, 0, 0, TO_ROOM);
      send_to_char("Arrows fly toward you, piercing you critically in several places.\r\n", vict);
      damage(NULL, vict, dice(10, (int)(GET_LEVEL(vict)/4))+100,
	     TYPE_UNDEFINED, FALSE);
      update_pos(vict);			// Update the victim's state
      if(GET_POS(vict) <= POS_STUNNED && GET_POS(vict) > POS_DEAD) 
      {
	act("$n has been greviously damaged by the archers here, and will die.",
	    TRUE, vict, 0, 0, TO_ROOM);
	send_to_char("You have been injured seriously by the archers here, and will soon die.\r\n", vict);
      } else if (GET_POS(vict) <= POS_DEAD) {
	act("$n has been vanquished by the hidden archers here.", FALSE, vict, 0, 0,
	    TO_ROOM);
	send_to_char("You have been defeated by the archers here.\r\n", vict);
	die(vict,NULL,"archers");
      }
      return;
    case ISBLAST_GAUNTLET: /////////////////////////////////////////// Gauntlet
      act("$n is assaulted by the mystical forces protecting this area.", 
	  TRUE, vict, 0, 0, TO_ROOM);
      send_to_char("Lightning forks out from the walls and lashes your body.\r\n", vict);
      damage(NULL, vict, dice(10, (int)(GET_LEVEL(vict)/3))+100,
	     TYPE_UNDEFINED, FALSE);
      send_to_char("Blue flames streak at you from the floor.\r\n", vict);
      damage(NULL, vict, dice(10, (int)(GET_LEVEL(vict)/3))+100,
	     TYPE_UNDEFINED, FALSE);
      handle_fireball(vict);
      send_to_char("You feel an invisible force pummeling you.\r\n", vict);
      damage(NULL, vict, dice(10, (int)(GET_LEVEL(vict)/3))+100,
	     TYPE_UNDEFINED, FALSE);
      update_pos(vict);			// Update the victim's state
      if (GET_POS(vict) <= POS_STUNNED && GET_POS(vict) > POS_DEAD)
      {
	act("$n has been greviously damaged by the powers here, and will die.",
	    TRUE, vict, 0, 0, TO_ROOM);
	send_to_char("You have been injured seriously by the powers here, and will soon die.\r\n", vict);
      } else if (GET_POS(vict) <= POS_DEAD) {
	act("$n has been vanquished by the mystical powers here.",
	    FALSE, vict, 0, 0, TO_ROOM);
	send_to_char("You have been defeated by the mystical powers here.\r\n", 
                     vict);
	die(vict,NULL,"room blast");
      } 
      return;
    case ISBLAST_ALL:
      island_blast(vict, ISBLAST_ARROW);
      if (!(vict) || (GET_POS(vict) <= POS_DEAD))
	return;
      island_blast(vict, ISBLAST_GAUNTLET);
      return;
  }
  return;
}

SPECIAL(arrow_room)
{
  struct char_data *k;
  struct room_data *room = (struct room_data *)me;
  int t;
  int dam;
  char limbs[5][10] = { "hand", "arm", "thigh", "shoulder", "back" };

  if (cmd)
    return FALSE;

  for (k = room->people; k; k=k->next_in_room)
  {
    if (!LR_FAIL(k, LVL_IS_GOD) || (number(0,8)!=0))
      continue;
    t = number(0,4);  
    sprintf(buf,"$n is hit in the %s by an arrow!",limbs[t]);
    act(buf,FALSE,k,0,0,TO_NOTVICT);
    sprintf(buf,"You are hit in the %s by an arrow! It feels really pointy...\r\n",limbs[t]);
    send_to_char(buf,k);
    dam = dice(t+1,GET_LEVEL(k));
    damage(NULL, k, dam, TYPE_UNDEFINED, FALSE);
    if(GET_POS(k) <= POS_DEAD)
    {
      act("$n falls to the ground. As dead as something that isn't alive.",FALSE,k,0,0,TO_ROOM);
      die(k,NULL,"arrow");
    }
  }
  return TRUE;
}
/* Function is called by the heartbeat of the game
 * and triggers damage to the rooms listed within it
void handle_damage_rooms()
{
  blast_occupants(10208);		// The Gauntlet
  blast_occupants(10209);
  blast_occupants(10210);

  shoot_occupants(10106);		// The compound	
}
*/

/* Subfunction of rotate_arms
 *	Puts the arms at the top of the tower into original positions
 */
void arms_original()
{
  struct obj_data *obj;

  // Put down the gaps on the platforms
  obj = read_object(10220, VIRTUAL);	// Goes to north arm
  obj_to_room(obj, real_room(10331));	// Put it on north platform
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10222, VIRTUAL);	// Goes to east arm
  obj_to_room(obj, real_room(10332));	// Load it on east platform
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10224, VIRTUAL);	// Goes into south arm
  obj_to_room(obj, real_room(10333));	// Load on south platform
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10226, VIRTUAL);	// Goes into west arm
  obj_to_room(obj, real_room(10334));	// Load on west platform
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  // Put down the arms
  obj = read_object(10219, VIRTUAL);	// hole that leads to north platform
  obj_to_room(obj, real_room(10335));	// Put it in north arm
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10221, VIRTUAL);	// Hole that leads to east
  obj_to_room(obj, real_room(10336));	// east arm
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10223, VIRTUAL);	// south, 
  obj_to_room(obj, real_room(10337));	// south arm
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10225, VIRTUAL);	// west
  obj_to_room(obj, real_room(10338));	// west arm
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);
}

/* Subfunction of rotate_arms
 *	Puts the arms at the top of the tower into next position
 */
void arms_plus_one() 
{
  struct obj_data *obj;

  // Put down the gaps on the platforms
  obj = read_object(10220, VIRTUAL);	// n arm accessible at e platform
  obj_to_room(obj, real_room(10332));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10222, VIRTUAL);	// e arm accessible at s platform
  obj_to_room(obj, real_room(10333));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10224, VIRTUAL);	// s arm accessble at w platform
  obj_to_room(obj, real_room(10334));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10226, VIRTUAL);	// w arm accessbile at n platform
  obj_to_room(obj, real_room(10331));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  // Put down the arms
  obj = read_object(10221, VIRTUAL);	// access to east platform in north arm
  obj_to_room(obj, real_room(10335));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10223, VIRTUAL);	// access to south platfrom from east arm
  obj_to_room(obj, real_room(10336));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10225, VIRTUAL);	// Acess to west platfrom from south arm
  obj_to_room(obj, real_room(10337));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10219, VIRTUAL);	// Access to north platfrom from west arm
  obj_to_room(obj, real_room(10338));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);
}


/* Subfunction of rotate_arms
 *	Puts the arms at the top of the tower into opposite sides
 */
void arms_plus_two()
{
  struct obj_data *obj;

  // Put down the gaps on the platforms
  obj = read_object(10220, VIRTUAL);	// north arm accessible from south platform
  obj_to_room(obj, real_room(10333));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10222, VIRTUAL);	// east arm accessbiel from west platform
  obj_to_room(obj, real_room(10334));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10224, VIRTUAL);	// south arm accessbile from north platform
  obj_to_room(obj, real_room(10331));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10226, VIRTUAL);	// west arm accessible from east platform
  obj_to_room(obj, real_room(10332));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  // Put down the arms
  obj = read_object(10219, VIRTUAL);	// north platform accessible from south arm
  obj_to_room(obj, real_room(10337));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10221, VIRTUAL);	// East platform accessible from west arm
  obj_to_room(obj, real_room(10338));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10223, VIRTUAL);	// South platform accessible from north arm
  obj_to_room(obj, real_room(10335));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10225, VIRTUAL);	// west platform accessible from east arm 
  obj_to_room(obj, real_room(10336));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

}

/* Subfunction of rotate_arms
 *  Puts the arms at the top of the tower into position right before original */
void arms_plus_three() 
{

  struct obj_data *obj;

  // Put down the gaps on the platforms
  obj = read_object(10220, VIRTUAL);	// North arm accessbile from west platform
  obj_to_room(obj, real_room(10334));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10222, VIRTUAL);	// east arm accessible from north platform
  obj_to_room(obj, real_room(10331));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10224, VIRTUAL);	// south arm accessible from east platform
  obj_to_room(obj, real_room(10332));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10226, VIRTUAL);	// west arm accessible from south platform
  obj_to_room(obj, real_room(10333));
  act("An arm has come around to the platform.", FALSE, 0, obj, 0, TO_ROOM);

  // Put down the arms
  obj = read_object(10219, VIRTUAL);	// north platform accessibile from east arm 
  obj_to_room(obj, real_room(10336));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10221, VIRTUAL);	// east platform accessible from south arm
  obj_to_room(obj, real_room(10337));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10223, VIRTUAL);	// south platform accessible from west arm 
  obj_to_room(obj, real_room(10338));
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);

  obj = read_object(10225, VIRTUAL);
  obj_to_room(obj, real_room(10335));	// west platfrom accessible from north arm
  act("A platform has become accessible.", FALSE, 0, obj, 0, TO_ROOM);
}

/* Given a room, moves arms away from platform */
void move_arm_away(int room)
{
  struct obj_data *o;

  // Get gap
  for (o = world[real_room(room)].contents; o; o = o->next)
    if (strcmp(o->name, "gap opening arm") == 0)
    {
      act("The arm has moved too far to reach from here now.", FALSE, 0, o, 0, TO_ROOM);
      extract_obj(o);	// Extract the gateway object
      break;
    }

  // Get hole inside arm
  for (o = world[real_room(room+4)].contents; o; o = o->next)
    if(strcmp(o->name, "hole wall") == 0)
    {
      act("The arm has moved away from the platform.", FALSE, 0, o, 0, TO_ROOM);
      extract_obj(o); // Extract the gateway object
      break;
    }
}

/* Controlling function used by the game heartbeat 
 *
 * Uses a static number to position the arms at the top
 * of the tower into appropriate positions, thus, 
 * effectively rotating them.
 */
void rotate_arms()
{
  static int arm_position_counter = -1;		// Arm positions

  // Move the arms 
  arm_position_counter = (arm_position_counter +1) % 12;

  // See if arms have rotated in front of a platform
  if (arm_position_counter == 0)
  {
    arms_original();			// Set arms to original locations
    return;
  }
  if (arm_position_counter == 3)
  {	
    arms_plus_one();			// Arms at one location on
    return;
  }
  if (arm_position_counter == 6)
  {
    arms_plus_two();			// Arms at opposite ends
    return;
  }
  if (arm_position_counter == 9)
  {
    arms_plus_three();			// Arms at last location
    return;
  }

  // Check if we should extract exits (15 seconds after arriving)
  if (arm_position_counter == 1 || arm_position_counter == 4 ||
      arm_position_counter == 7 || arm_position_counter == 10)
  {
    move_arm_away(10331);		// Northern platform
    move_arm_away(10332);		// East
    move_arm_away(10333);		// South
    move_arm_away(10334);		// West
  }
}


/* SPEC PROC : 	ITEM (Weapon)
 *	Chance of outright kill with hit, but destroys item
 */

SPECIAL(deadlyblade) 
{
  ACMD(do_violent_skill);
  bool violence_check(struct char_data *ch, struct char_data *vict, int skillnum);

  struct char_data *p = 0;
  struct obj_data *obj = NULL;

  // Ensure the weapon is the one being wielded - DM ...
  // Check Dual Wield, Too -- Artus
  
  if (!(cmd) || !(ch))
    return FALSE;

  if (GET_EQ(ch, WEAR_WIELD) == (struct obj_data *)me)
    obj = GET_EQ(ch, WEAR_WIELD);
  else if (IS_DUAL_WIELDING(ch) && 
           (GET_EQ(ch, WEAR_HOLD) == (struct obj_data *)me))
    obj = GET_EQ(ch, WEAR_HOLD);
  else
    return (FALSE);

  // FFS, Tali. != POS_FIGHTING => < POS_STANDING -- Artus.
  if (CMD_IS("kill") && GET_POS(ch) < POS_STANDING)
  {
    one_argument(argument, buf1);

    if (!*buf1 || !(p = generic_find_char(ch, buf1, FIND_CHAR_ROOM))) 
      return (FALSE);

    // Calculate chance to kill 
    if (number(1, (int)GET_LEVEL(p)/5) == 1) 
    {
      if (!IS_NPC(p) || !LR_FAIL(ch, LVL_CHAMP))
	return (FALSE);
      // FFS Tali. Added violence check -- Artus.
      if (!violence_check(ch, p, SCMD_KILL))
	return (FALSE);
      sprintf(buf2, "Your deadly blade dives into %s's body, mortally wounding them!\r\n", p->player.short_descr);
      send_to_char(buf2, ch);
      send_to_char("... it is destroyed in the process.\r\n", ch);
      act("$n's blade dives into its victim's body, mortally wounding them!",
	  FALSE, ch, 0, 0, TO_ROOM);
      extract_obj(GET_EQ(ch, 16));
      damage(ch, p, GET_HIT(p)+1, TYPE_UNDEFINED, FALSE);
      GET_HIT(p) = -1;
      update_pos(p);
      do_violent_skill(ch, argument, 1, SCMD_KILL);
      return (TRUE);
    }			
  }
  return (FALSE);	
}

/* ARTUS: The clan guard specprocs were here... couldn't be fucked commenting 
 * them out.. Heh. Fuck.. Hal's code really does suck!
*/

// Returns the cost of repairing the given item and copies the text for the
// item to output
int get_repair_cost(struct char_data *ch, struct obj_data *obj, char *output) {
  float cost = 0;
        
  if (!ch || !obj) 
  {
    basic_mud_log("SYSERR: NULL char or obj passed to get_repair_cost");
    return 0;
  }

  // non-damaged items
  if (GET_OBJ_MAX_DAMAGE(obj) == GET_OBJ_DAMAGE(obj)) 
  {
    *output='\0';
    return 0;
  }
  
  // TODO: repair costs based on bargaining skills and chars charisma
  //       current damage and cost of item
  if (default_item_damage[(int)GET_OBJ_TYPE(obj)] > 0) 
  {
    cost = default_item_damage[(int)GET_OBJ_TYPE(obj)];
    cost = (100 / cost);
    cost = cost * 20 * GET_LEVEL(ch);
    cost = cost * (GET_OBJ_MAX_DAMAGE(obj) - GET_OBJ_DAMAGE(obj));
    cost = MAX(0, (int)cost);
    sprintf(output, "&5%s&n (&Y%d&n)\r\n", obj->short_description, (int)cost);
  } else {
    cost = 0; 
    sprintf(output, "&5%s&n (not repairable)\r\n", obj->short_description);
  }

  if (cost == 0)
    *output='\0';

  return (int)cost;
}

// This function is written seperately so that it can be called easily enough
// from existing specs. The calling spec should appropiately check the command
// name and pass the arguments of the repair command as arg, repairer is the
// mob that performs the repair, and ch is the repairee.
void repair_proc(struct char_data *repairer, struct char_data *ch, char *arg) {

  char arg1[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  char head[MAX_INPUT_LENGTH], tail[MAX_INPUT_LENGTH];
  bool quote = FALSE, all = FALSE;
  struct obj_data *obj = NULL, *repair_obj = NULL;
  int cost = 0;
  const char *usage = "&6%s&r tells you, \'That will be &4repair [quote] <item | \'all\'>&r'&n\r\n";

  if (!repairer || !ch) 
  {
    basic_mud_log("SYSERR: NULL reparier or char passed to repair_proc");
    return;
  }

  if (IS_NPC(ch)) 
  {
    send_to_char("Bugger off.\r\n", ch);
    return;
  }
  
  // Process args
  if (!*arg) 
  {
    sprintf(buf, usage, GET_NAME(repairer));
    send_to_char(buf, ch);
    return; 
  }

  half_chop(arg, arg1, rest);
#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch)) 
  {
    sprintf(buf, "arg = %s, arg1 = %s, rest = %s", arg, arg1, rest);
    send_to_char(buf, ch);
  }
#endif
  if (is_abbrev(arg1, "quote")) 
  {
    quote = TRUE;
    half_chop(rest, arg1, rest);
  }
#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch)) 
  {
    sprintf(buf, "After quote check: arg = %s, arg1 = %s, rest = %s", arg, arg1, rest);
    send_to_char(buf, ch);
  }
#endif
  if (!*arg1) 
  {
    sprintf(buf, usage, GET_NAME(repairer));
    send_to_char(buf, ch);
    return; 
  }

  if (is_abbrev(arg1, "all")) 
  {
    all = TRUE;
  } else {
    if (!(obj = generic_find_obj(ch, arg1, FIND_OBJ_EQUIP | FIND_OBJ_INV)))
    {
      send_to_char("You don't seem to have that item!\r\n", ch);    
      return;
    }
  }

  buf[0] = '\0';
  head[0] = '\0';
  tail[0] = '\0';

  // Set up repairer header messages
  if (quote) 
    sprintf(head,"&6%s&r tells you, 'Ok, this is what I can do you for.'&n\r\n",
            GET_NAME(repairer));
  else
    // This header shall be changed later if the char cannot afford the repair
    sprintf(head, "&6%s&r tells you, 'Ok, I've repaired the following.'&n\r\n",
            GET_NAME(repairer));

  // List all objs
  if (all) 
  {
    // Equiped objs
    for (int i = 0; i < NUM_WEARS; i++) 
      if (GET_EQ(ch, i)) 
        if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) 
	{
          cost += get_repair_cost(ch, GET_EQ(ch, i), buf1);
          sprintf(buf, "%s%s", buf, buf1);
        }
    // Inventory
    for (repair_obj = ch->carrying; repair_obj; 
	 repair_obj = repair_obj->next_content) 
      if (CAN_SEE_OBJ(ch, repair_obj))
      {
        cost += get_repair_cost(ch, repair_obj, buf1);
        sprintf(buf, "%s%s", buf, buf1);
      }
    sprintf(tail, "&6%s&r tells you, "
                   "'For a total cost of &Y%d&r coins.'&n\r\n",
                   GET_NAME(repairer), cost);
  // Single item
  } else {
    cost += get_repair_cost(ch, obj, buf1);
    sprintf(buf, "%s%s", buf, buf1);
  }

  // Fix and charge
  if (!quote)
    if (GET_GOLD(ch) >= cost)
    {
      // all items
      if (all)
      {
        // Wearing
        for (int i = 0; i < NUM_WEARS; i++)
          if (GET_EQ(ch, i))
            if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
              GET_OBJ_DAMAGE(GET_EQ(ch, i)) = GET_OBJ_MAX_DAMAGE(GET_EQ(ch, i));
        // Inventory
        for (repair_obj = ch->carrying; repair_obj;
	     repair_obj = repair_obj->next_content)
          if (CAN_SEE_OBJ(ch, repair_obj))
            GET_OBJ_DAMAGE(repair_obj) = GET_OBJ_MAX_DAMAGE(repair_obj);
      } else // single item
        GET_OBJ_DAMAGE(obj) = GET_OBJ_MAX_DAMAGE(obj);
      // Make em pay ...
      GET_GOLD(ch) -= cost;
    } else { // poor bastard - set header and tail messages
      sprintf(head, "&6%s&r tells you, "
                    "'You can't afford to repair the following.'&n\r\n",
                    GET_NAME(repairer));
      sprintf(tail, "&6%s&r tells you, "
                    "'In case you didn't notice, you can't afford it.'&n\r\n",
                    GET_NAME(repairer));
    }

  // nothing repairable
  if (cost == 0)
  {
    sprintf(head, "&6%s&r tells you, 'You have nothing repairable.'&n\r\n",
                  GET_NAME(repairer));
    send_to_char(head, ch);
  // page output
  } else {
    sprintf(buf2, "%s%s%s", head, buf, tail);
    page_string(ch->desc, buf2, TRUE);
  }
}

SPECIAL(repairer)
{
  struct char_data *repairer = (struct char_data *) me;
  
  if (IS_NPC(ch) || (!CMD_IS("repair")))
    return (FALSE);
 
  repair_proc(repairer, ch, argument); 
  return (TRUE); 
}

/* Artus> Watch Special :o) */
SPECIAL(watch_timer)
{
  struct obj_data *watch = (struct obj_data *)me;
  extern int pulse;


  if (cmd)
  {
    if (!(*argument))
      return FALSE;
    one_argument(argument, buf);
    if (CMD_IS("look") && (!str_cmp(buf, "time")))
    {
      sprintf(buf, "It is currently %d o'clock %s.\r\n", 
	      ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	      ((time_info.hours >= 12) ? "pm" : "am"));
      send_to_char(buf, ch);
      return TRUE;
    }
    if (CMD_IS("watch"))
    {
      if (!str_cmp(buf, "off"))
      {
	GET_OBJ_VAL(watch, 2) = 25;
	send_to_char("You turn your watches hourly alarm off.\r\n", ch);
	return TRUE;
      }
      if (!str_cmp(buf, "hourly"))
      {
	GET_OBJ_VAL(watch, 2) = 24;
	send_to_char("You turn your watches hourly alarm on.\r\n", ch);
	return TRUE;
      }
      if (is_number(buf))
      {
	int foo = atoi(buf);
	if ((foo < 0) || (foo > 23))
	{
	  send_to_char("You must enter an hour between 0 and 23.\r\n", ch);
	  return TRUE;
	}
	GET_OBJ_VAL(watch, 2) = foo;
	if (foo == 0)
	{
	  send_to_char("You set your watches daily alarm to midnight.\r\n", ch);
	  return TRUE;
	}
	if (foo == 12)
	{
	  send_to_char("You set your watches daily alarm to midday.\r\n", ch);
	  return TRUE;
	}
	if (foo > 12) { foo -= 12; }
	sprintf(buf, "Your watch will now alarm daily at %d o'clock %cm.\r\n", 
	        foo, (GET_OBJ_VAL(watch, 2) < 12) ? 'a' : 'p');
	send_to_char(buf, ch);
	return TRUE;
      } // Is Number.
      send_to_char("Syntax: watch off    - Turn watch alarm off\r\n"
	           "        watch hourly - Turn hourly watch alarm on\r\n"
		   "        watch [0-23] - Turn daily alarm on at specified"
		   " hour.\r\n", ch);
      return TRUE;
    }
    return FALSE;
  }
  if (!(watch->worn_by) || !(watch->worn_by->desc))
    return FALSE;
  if (GET_OBJ_VAL(watch, 3) == time_info.hours)
    return FALSE;
  if (!((GET_OBJ_VAL(watch, 2) == 24) || 
	(GET_OBJ_VAL(watch, 2) == time_info.hours)))
    return FALSE;
  if (pulse % SECS_PER_MUD_HOUR RL_SEC == 0)
  {
    sprintf(buf, "Your watch beeps, signifying %d o'clock %s.\r\n",
	    ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	    ((time_info.hours >= 12) ? "pm" : "am"));
    send_to_char(buf, watch->worn_by);
    GET_OBJ_VAL(watch, 3) = time_info.hours;
  }
  return FALSE;
}
