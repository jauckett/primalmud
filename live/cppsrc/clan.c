/**************************************************************************
 * File: clan.c                       Intended to be used with PrimalMUD, *
 * 				               a derivative of CircleMUD. *
 * 				                                          *
 * Info: This is the code for the new clan system..                       *
 * By  : Justin Katz (Artus on PrimalMUD - mud.laybyrinth.net.au 4000)    *
 *									  *
 * This code is based on an old mod of Mehdi Keddache's origin.           * 
 * Though, I think it's getting harder to tell :o)                        *
 *  								          *
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 * CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *                                                                        *
 * This text last modified, 2001-01-15, by Justin Katz.                   *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "clan.h"
#include "genmob.h"
#include "genwld.h"
#include "screen.h"

/* External */
extern void save_char_file_u(struct char_file_u st);
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct player_index_element *player_table;
extern int rev_dir[];
extern char *punish_types[];
extern char *offence_types[];
extern void do_odd_write(struct descriptor_data *d, ubyte wtype, int maxlen);
extern struct char_data *mob_proto;
extern struct char_data *character_list;
int update_mobile_strings(struct char_data *t, struct char_data *f);
void reset_zone(zone_rnum zone);
SPECIAL(shop_keeper);

/* Local */
int num_of_clans;
struct clan_rec clan[MAX_CLANS];
sh_int find_clan_by_sid(char *test);
int clan_room_dirs (char *arg);
int clan_can_hear(struct descriptor_data *d, int c);
int do_clan_heal(struct char_data *ch, struct char_data *vict); 
void do_clan_infocom (int clan_one, int clan_two, char *arg);
void clan_tax_update (struct char_data *ch, struct char_data *vict);
void clan_pk_update (struct char_data *winner, struct char_data *loser);
void clan_rel_inc(struct char_data *ch, struct char_data *vict, int amt);

const int clan_levels[] = {
  100000,      /*  1 */
  200000,
  500000,
  1000000,
  1500000,     /*  5 */
  2000000,
  2500000,
  3000000,
  4000000,
  5000000,     /* 10 */
  7500000,
  10000000,
  20000000,
  50000000,
  100000000,   /* 15 */
  250000000,
  500000000,
  1000000000,
  1000000000,
  1000000000,  /* 20 */
  1000000000,
  1000000000,
  1250000000,
  1500000000, 
  2000000000,  /* 25 */
  2000000000,  
  2000000000,  
  2000000000,  
  2000000000,  
  2000000000,  /* 30 */
  2000000000,  
  2000000000,  
  2000000000,  
  2000000000,
  2000000000   /* 35 */
};

char clan_privileges[NUM_CP+1][20] = { "recruit"  ,"promote"  ,"demote"   ,
	                               "banish"   ,"withdraw" ,"setapplev",
				       "setfees"  ,"enhance"  ,"room"     ,
				       "setdesc"  ,"enable"
                                     };

char clan_enablers[NUM_CO+1][20] = {   "hall"     ,"healer"   ,"talk"     ,
	                               "board"
			           };

/* Send The Standard Clan Screen */
void send_clan_format(struct char_data *ch)
{
  int c,r;

  if(!LR_FAIL(ch, LVL_CLAN_GOD))
  {
    send_to_char("Clan commands available to you:\r\n"
                 "   clan about         <clan num>\r\n"
		 "   clan balance       <clan num>\r\n"
		 "   clan banish        <player>\r\n"
		 "   clan create        <leader> <clan name>\r\n"
		 "   clan demote        <player>\r\n"
		 "   clan deposit       <clan num> <amount>\r\n"
		 "   clan destroy       <clan num>\r\n"
		 "   clan disable       <clan num> <option>\r\n"
		 "   clan enable        <clan num> <option>\r\n"
		 "   clan info          <clan num>\r\n"
		 "   clan promote       <player>\r\n"
		 "   clan recruit       <player>\r\n"
		 "   clan relations     <clan num>\r\n"
		 "   clan score\r\n"
		 "   clan set           <args>\r\n"
		 "   clan stat          <clan num>\r\n"
		 "   clan who           <clan num>\r\n"
		 "   clan withdraw      <clan num> <amount>\r\n",ch);
    return;
  }

  c=find_clan_by_id(GET_CLAN(ch));
  r=GET_CLAN_RANK(ch);

  send_to_char(  "Clan commands available to you:\r\n"
		 "   clan about         <clan num>\r\n",ch);

  if (c<0) {
    send_to_char("   clan apply         <clan num>\r\n"
                 "   clan info          <clan num>\r\n",ch);
    return;
  }

  if(r >= clan[c].privilege[CP_WITHDRAW])
    send_to_char("   clan balance\r\n", ch);
  if(r>=clan[c].privilege[CP_BANISH])
    send_to_char("   clan banish        <player>\r\n" ,ch);
  if(r>=clan[c].privilege[CP_DEMOTE])
    send_to_char("   clan demote        <player>\r\n",ch);

  send_to_char(  "   clan deposit       <amount>\r\n",ch);

  if(r>=clan[c].privilege[CP_ENHANCE])
    send_to_char("   clan describe      <guard|healer>\r\n",ch);
  if(r>=clan[c].privilege[CP_ENABLE])
    send_to_char("   clan enable        <option>\r\n", ch);
  if(r>=clan[c].privilege[CP_ENHANCE])
    send_to_char("   clan enhance       <guard|healer>\r\n",ch);

  send_to_char(  "   clan info          <clan num>\r\n",ch);

  if(r>=clan[c].privilege[CP_ENHANCE])
    send_to_char("   clan name          <guard|healer>\r\n",ch);
  if(r>=clan[c].privilege[CP_PROMOTE])
    send_to_char("   clan promote       <player>\r\n",ch);
  if(r>=clan[c].privilege[CP_RECRUIT])
    send_to_char("   clan recruit       <player>\r\n",ch);

  send_to_char(  "   clan relations\r\n",ch);

  if (r>=clan[c].privilege[CP_ROOM]) 
    send_to_char("   clan room          <args>\r\n",ch);

  send_to_char(  "   clan score\r\n",ch);

  if ((r >= clan[c].privilege[CP_SET_APPLEV]) || (r >= clan[c].privilege[CP_SET_FEES]) || (r >= clan[c].privilege[CP_SET_DESC]))
    send_to_char("   clan set           <args>\r\n",ch);

  send_to_char(  "   clan status\r\n"
		 "   clan who\r\n",ch);

  if(r >= clan[c].privilege[CP_WITHDRAW])
    send_to_char("   clan withdraw      <amount>\r\n",ch);
 
}

/* do_clan_reset - Resets clan mobs/rooms.. Will need to do boards too. */
void do_clan_reset (struct char_data *ch, char *arg)
{
  int c,i,j;
  struct char_data *guard=NULL, *healer=NULL, *oldmob=NULL;
  struct room_data *room=NULL;
  zone_rnum real_zone(zone_vnum vnum);
  int real;

  if (LR_FAIL(ch, LVL_CLAN_GOD))
  {
    send_clan_format(ch);
    return;
  }
  if (!*arg) {
    send_to_char("You must specify a clan number to reset.\r\n", ch);
    return;
  }
  if (((c=atoi(arg)) < 1) || (c > MAX_CLANS)) 
  {
    send_to_char("Clan number must be between 1 and 8.\r\n", ch);
    return;
  }

  c--;

  /* Firstly, The Mobs :o) Checks out OK.. */
  if ((real = real_mobile(c + (CLAN_ZONE * 100))) > -1)
  {
    guard = &mob_proto[real];
    free(GET_LDESC(guard));
    free(GET_DDESC(guard));
    GET_LDESC(guard) = str_dup("A clan guard stands here.\r\n");
    GET_DDESC(guard) = str_dup("This clan guard has not been described yet.\r\n");
    GET_HIT(guard) = GET_MANA(guard) = GET_NDD(guard) = GET_SDD(guard) = 1;
    GET_LEVEL(guard) = GET_HITROLL(guard) = GET_DAMROLL(guard) = 50;
    GET_MOVE(guard) = 10000; 
    GET_AC(guard) = -100;
  } else {
    sprintf(buf, "Clan guard [%d] not found while resetting!", (c + (CLAN_ZONE * 100)));
    basic_mud_log (buf);
  }
  if ((real = real_mobile(c + (CLAN_ZONE * 100) + MAX_CLANS)) > -1)
  {
    healer = &mob_proto[real];
    free(GET_LDESC(healer));
    free(GET_DDESC(healer));
    GET_LDESC(healer) = str_dup("A clan healer stands here.\r\n");
    GET_DDESC(healer) = str_dup("This clan healer has not been described yet.\r\n");
    GET_HIT(healer) = GET_MANA(healer) = GET_NDD(healer) = GET_SDD(healer) = 1;
    GET_LEVEL(healer) = GET_HITROLL(healer) = GET_DAMROLL(healer) = 50;
    GET_MOVE(healer) = 10000;
    GET_AC(healer) = -100;
  } else {
    sprintf(buf, "Clan healer [%d] not found while resetting!", (c + (CLAN_ZONE * 100)));
    basic_mud_log(buf);
  }
  if ((guard != NULL) || (healer != NULL)) {
    for (oldmob = character_list; oldmob; oldmob = oldmob->next) {
      if ((guard != NULL) && (guard->nr == oldmob->nr)) 
        update_mobile_strings(oldmob, &mob_proto[guard->nr]);
      if ((healer != NULL) && (healer->nr == oldmob->nr))
        update_mobile_strings(oldmob, &mob_proto[healer->nr]);
    }
  }
  if (save_mobiles(real_zone(CLAN_ZONE)) == FALSE)
    send_to_char("Failed to save mobiles.\r\n", ch);

  /* Ok, Now, It's time to think about the rooms.. Scarey shit.. HERE */

  if ((real = real_room(c + (CLAN_ZONE*100))) != NOWHERE)
  {
    room = &world[real]; /* Guard Post */
    free(room->name);
    room->name = strdup("Guard Post");
    free(room->description);
    room->description = strdup("This guard post is yet to be described.\r\n");
    REMOVE_BIT(room->room_flags, ROOM_REGEN_2);
  } else {
    sprintf(buf, "Clan room [%d] not found while resetting!", (c + (CLAN_ZONE*100)));
    basic_mud_log(buf);
  }

  if ((real = real_room(c + MAX_CLANS + (CLAN_ZONE*100))) != NOWHERE)
  {
    room = &world[real]; /* Hall */
    free(room->name);
    room->name = strdup("Clan Hall");
    for (i = 0; i < NUM_OF_DIRS; i++)
      if (!(((c % 2 == 1) && (i == EAST)) || ((c % 2 == 0) && (i == WEST))))
        room->dir_option[i] = NULL;
  
    free(room->description);
    room->description = strdup("This clan hall is yet to be described.\r\n");
    REMOVE_BIT(room->room_flags, ROOM_REGEN_2);
  } else {
    sprintf(buf, "Clan room [%d] not found while resetting!", (c + MAX_CLANS + (CLAN_ZONE*100)));
    basic_mud_log(buf);
  }
  for (i = 2;i <= CLAN_ROOM_MAX;i++)
  { /* Rooms */
    if ((real = real_room(c + (CLAN_ZONE*100) + (i*MAX_CLANS))) != NOWHERE) {
      room = &world[real]; 
      free(room->name);
      room->name = strdup("Clan Room");
      for (j = 0; j < NUM_OF_DIRS; j++)
        room->dir_option[j] = NULL;
      free(room->description);
      room->description = strdup("This clan room is yet to be described.\r\n");
      REMOVE_BIT(room->room_flags, ROOM_REGEN_2);
    } else {
      sprintf(buf, "Clan room [%d] not found while resetting!", (c + (CLAN_ZONE*100) + (i*MAX_CLANS)));
      basic_mud_log(buf);
    }
  }
  if (save_rooms(real_zone(CLAN_ZONE)) == FALSE)
    send_to_char("Failed to save rooms!\r\n", ch);

  send_to_char("Finished resetting clan.\r\n", ch);
  return;
}

/* Simple routine.. Should stop clans getting too much coin :p */
void clan_coinage (int c, int amount)
{
  amount /= 100;
  if (amount > 0)
    clan[c].treasure = MIN(clan[c].treasure + amount, 2000000000);
  else
    clan[c].treasure = MAX(clan[c].treasure + amount, 0);
  return;
}

/* Clan Relation Change (clan 1,clan 2, amt_to_change) */
void clan_rel_change (int us, int them, int amt)
{
  if ((us < 0) || (them < 0) || (us > num_of_clans) || (them > num_of_clans) ||
      (us == them) || (amt == 0))
    return;
  if (amt > 0)
    GET_CLAN_REL(us, them) = MAX(GET_CLAN_REL(us, them) + amt, CLAN_REL_MIN);
  else
    GET_CLAN_REL(us, them) = MIN(GET_CLAN_REL(us, them) + amt, CLAN_REL_MAX);
  GET_CLAN_REL(them, us) = GET_CLAN_REL(us, them);
}

void clan_rel_inc (struct char_data *ch, struct char_data *vict, int amt)
{
  int us, them, amount;
  amount = amt;

  if (!(vict) || !(ch) || (vict == ch) || IS_NPC(ch) || IS_NPC(vict) ||
      (GET_CLAN(ch) == GET_CLAN(vict)) || 
      (GET_CLAN(ch) < 1) || (GET_CLAN(vict) < 1))
    return;
  us = find_clan_by_id(GET_CLAN(ch));
  them = find_clan_by_id(GET_CLAN(vict));
  if ((us < 0) || (them < 0)) 
    return;
  if (GET_CLAN_RANK(ch) == clan[us].ranks)
    amount += amt;
  if (GET_CLAN_RANK(vict) == clan[them].ranks)
    amount += amt;
  if (GET_CLAN_RANK(ch) == (clan[us].ranks - 1))
    amount += (int)(amt / 2);
  if (GET_CLAN_RANK(vict) == (clan[them].ranks - 1))
    amount += (int)(amt / 2);
  clan_rel_change(us, them, amt);
}

/* Determines whether a char is privledged enough for certain clan commands. */
int clan_check_priv (struct char_data *ch, int priv)
{
  int clan_num;
  if (!LR_FAIL(ch, LVL_CLAN_GOD))
    return CPC_IMM;
  if ((clan_num=find_clan_by_id(GET_CLAN(ch))) < 0)
  {
    send_to_char("You don't belong to any clan.\r\n", ch);
    return CPC_NOPRIV;
  }
  if ((priv>=0) && (GET_CLAN_RANK(ch)<clan[clan_num].privilege[priv]))
  {
    send_to_char("You are not ranked highly enough within your clan.\r\n", ch);
    return CPC_NOPRIV;
  }
  if ((priv==CP_LEADER) && (GET_CLAN_RANK(ch) < clan[clan_num].ranks))
  {
    send_to_char("You are not ranked highly enough within your clan.\r\n", ch);
    return CPC_NOPRIV;
  }
  return CPC_PRIV;
}

/* Clan Exp Gaining... */
void gain_clan_exp (struct char_data *ch, long amount)
{
  int c=0;
  char cin[80];

  if (((c = find_clan_by_id(GET_CLAN(ch))) < 0) || (clan[c].level >= 35))
    return;
  amount /= 1000; 
  if (amount <= 0)
    amount = 1;
  clan[c].exp = MIN((clan[c].exp + amount), 2000000000);
  while (clan[c].exp > clan_levels[clan[c].level])
  { /* Clan Levels */
    clan[c].exp -= clan_levels[clan[c].level];
    clan[c].level++;
    sprintf(cin, "%s has advanced to level %d.", clan[c].name, clan[c].level);
    do_clan_infocom(0, 0, cin);
  }
  save_clans();
}

/* Creating a new clan */
void do_clan_create (struct char_data *ch, char *arg)
{
  struct char_data *leader = NULL;
  char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH],cin[80];
  int new_id=0,i;

  if ((!*arg) || (LR_FAIL(ch, LVL_CLAN_GOD)))
  {
    send_clan_format(ch);
    return;
  }

  if(num_of_clans == MAX_CLANS)
  {
    send_to_char("Max clans reached. WOW!\r\n",ch);
    return; 
  }

  half_chop(arg, arg1, arg2);

  if (!(leader=generic_find_char(ch,arg1,FIND_CHAR_ROOM)))
  {
    send_to_char("The leader of the new clan must be present.\r\n",ch);
    return;
  }

  if (strlen(arg2) >= 32)
  {
    send_to_char("Clan name too long! (32 characters max)\r\n",ch);
    return; 
  }

  if ((GET_LEVEL(leader) >= LVL_IS_GOD) && (CLAN_NO_IMM > 0))
  {
    send_to_char("You cannot set an immortal as the leader of a clan.\r\n",ch);
    return; 
  }

  if (GET_LEVEL(leader) < LVL_CLAN_MIN)
  {
    send_to_char("You cannot set someone that inexperienced as the clan leader.\r\n", ch);
    return;
  }
  
  if ((GET_CLAN(leader) != 0) && (GET_CLAN_RANK(leader) != 0))
  {
    send_to_char("The leader already belongs to a clan!\r\n",ch);
    return; 
  }

  if (find_clan(arg2) != -1)
  {
    send_to_char("That clan name alread exists!\r\n",ch);
    return;
  }

  strncpy(clan[num_of_clans].name, CAP((char *)arg2), 32);
  for (i = 1; i <= MAX_CLANS; i++)
    if (find_clan_by_id(i) < 0)
    {
      new_id=i;
      break;
    }
  clan[num_of_clans].id=new_id;
  clan[num_of_clans].ranks =  2;
  strcpy(clan[num_of_clans].rank_name[0],"Member");
  strcpy(clan[num_of_clans].rank_name[1],"Leader");
  clan[num_of_clans].treasure = 0 ;
  clan[num_of_clans].members = 1 ;
  clan[num_of_clans].power = GET_LEVEL(leader) ;
  clan[num_of_clans].app_fee = 500000 ;
  clan[num_of_clans].taxrate = 5 ;
  clan[num_of_clans].app_level = DEFAULT_APP_LVL ;
  for(i = 0; i < 5; i++)
    clan[num_of_clans].spells[i]=0;
  for(i = 0; i < 20; i++)
    clan[num_of_clans].privilege[i]=clan[num_of_clans].ranks;
  for(i = 0; i < MAX_CLANS; i++)
  {
    GET_CLAN_REL(i, num_of_clans) = 0;
    GET_CLAN_REL(num_of_clans, i) = 0;
    GET_CLAN_FRAGS(num_of_clans, i) = 0;
    GET_CLAN_DEATHS(i, num_of_clans) = 0;
    GET_CLAN_FRAGS(i, num_of_clans) = 0;
    GET_CLAN_DEATHS(num_of_clans, i) = 0;
  }

  clan[num_of_clans].options = 0;
  
  strcpy(clan[num_of_clans].description, "A newly formed clan.");
  num_of_clans++;
  save_clans();
  send_to_char("Clan created\r\n", ch);
  GET_CLAN(leader) = clan[num_of_clans-1].id;
  GET_CLAN_RANK(leader) = clan[num_of_clans-1].ranks;
  REMOVE_BIT(EXT_FLAGS(leader), EXT_PKILL);
  save_char(leader, leader->in_room);
  sprintf(cin, "A new clan, %s has been formed.", clan[num_of_clans-1].name);
  do_clan_infocom(0, 0, cin);

  return;
}

/* Clan Scoreboard -- Needs Work! TODO - ARTUS 
 * Requires sorting, better algorithm.. */
void do_clan_score (struct char_data *ch)
{
  int i, j;

  struct clan_score {
    char name[32];
    int frags;
    int power;
  };

  struct clan_score cscore[MAX_CLANS];

  send_to_char("Clan Scoreboard... Under Construcation\r\n"
	       "--------------------------------------\r\n", ch);

  for (i=0;i<num_of_clans;i++) {
    cscore[i].frags = 0;
    strncpy(cscore[i].name, clan[i].name, 32);
    cscore[i].power = clan[i].power;
    for (j=0;j<num_of_clans;j++) {
      if (i == j) 
	continue;
      cscore[i].frags += GET_CLAN_FRAGS(i, j) - GET_CLAN_DEATHS(i, j);
    }
  }

  for (i=0;i<num_of_clans;i++) 
  {
    sprintf(buf, "%-32s        Frags: %6d  Power: %6d\r\n", cscore[i].name, cscore[i].frags, cscore[i].power);
    send_to_char(buf, ch);
  }
  
  return;
}

/* Clan charging */
int do_clan_charge (struct char_data *ch, int amount)
{
  if (!LR_FAIL(ch, LVL_CLAN_GOD))
    return 1;
  if (GET_GOLD(ch) < amount)
    return 0;
  GET_GOLD(ch) -= amount;
  return 1;
}

/* Clan destruction */     
void do_clan_destroy (struct char_data *ch, char *arg)
{

  int i,j;
  extern int top_of_p_table;
  struct char_file_u chdata;
  struct char_data *victim=NULL;
  char cin[80];

  if(LR_FAIL(ch, LVL_CLAN_GOD))
  {
    send_clan_format(ch);
    return;
  }

  if (!*arg)
  {
    send_to_char("You must specify which clan to destroy.\r\n", ch);
    return;
  }

  if (!is_number(arg))
  {
    send_to_char("You must destroy clans by their number!\r\n", ch);
    return;
  }
  if ((i = find_clan_by_sid(arg)) < 0) 
  {
    send_to_char("Unknown or invalid clan number specified.\r\n", ch);
    return; 
  }
  sprintf(cin, "Clan %s has been destroyed.", clan[i].name);
  do_clan_infocom(0, 0, cin);

  for (j = 0; j <= top_of_p_table; j++){
    if((victim=get_player_online(ch, (player_table+j)->name, FIND_CHAR_INVIS))) 
    {
      if(GET_CLAN(victim)==clan[i].id) 
      {
        GET_CLAN(victim)=0;
        GET_CLAN_RANK(victim)=0;
        save_char(victim, victim->in_room); 
      } 
    } else {
      load_char((player_table + j)->name, &chdata);
      if(chdata.player_specials_saved.clan==clan[i].id)
      {
        chdata.player_specials_saved.clan=0;
        chdata.player_specials_saved.clan_rank=0;
        save_char_file_u(chdata); 
      } 
    } 
  }
  
  do_clan_reset(ch, arg);

  for (j = 0; j <= num_of_clans; j++)
  {
    GET_CLAN_REL(j, i)   = 0;
    GET_CLAN_FRAGS(j, i) = 0;
    GET_CLAN_DEATHS(j, i)= 0;
  }
  
  memset(&clan[i], sizeof(struct clan_rec), 0);
  for (j = i; j < num_of_clans - 1; j++) 
    clan[j] = clan[j + 1];
  num_of_clans--;
  send_to_char("Clan deleted.\r\n", ch);
  save_clans();
  return;
}

/* Statting Clans.. Needs colourising. TODO - ARTUS
 * Coloring can be done after integration - I don't have & codes.. */
void do_clan_stat (struct char_data *ch, char *arg)
{
  int c, i, crap=0;

  if (LR_FAIL(ch, LVL_CLAN_GOD))
  {
    send_clan_format(ch);
    return;
  }

  if (!(*arg))
  {
    send_to_char ("What clan did you want to stat, anyway?\r\n", ch);
    return;
  }
  if ((c=find_clan_by_sid(arg)) < 0) {
    send_to_char ("That clan seems to only exist in your imagination.\r\n", ch);
    return;
  }

  /* Line 1: Name, IDnum, Rnum */
  sprintf(buf, "Clan '%s' IDNUM: [%d] RNUM: [%d]\r\n", clan[c].name, clan[c].id, c);

  /* Line 2: Members, Power, Bank, Ranks */
  sprintf(buf, "%sMembers: [%d] Power: [%d] Bank: [%ld00] Ranks: [%d]\r\n", buf, clan[c].members, clan[c].power, clan[c].treasure, clan[c].ranks);

  /* Line 3: Level, Exp */
  sprintf(buf, "%sLevel: [%d] Exp: [%ld]\r\n", buf, clan[c].level, clan[c].exp);
  
  /* Line 4: Taxrate, Applev, Spells */
  sprintf(buf, "%sTaxrate: [%d] Applev: [%d] Spells: [%d/%d/%d/%d/%d]\r\nRelations: ", buf, clan[c].taxrate, clan[c].app_level, clan[c].spells[0], clan[c].spells[1], clan[c].spells[2], clan[c].spells[3], clan[c].spells[4]);
  
  /* Line 5: Relations */
  for (i = 0; i < num_of_clans; i++)
    sprintf(buf, "%s%d(%d)%s", buf, clan[i].id, GET_CLAN_REL(c, i), ((i < (num_of_clans-1)) ? " " : "\r\nFrags: "));

  /* Line 6: Frags-Deaths */
  for (i = 0; i < num_of_clans; i++)
    sprintf(buf, "%s%d(%d-%d)%s", buf, clan[i].id, GET_CLAN_FRAGS(c, i), 
	    GET_CLAN_DEATHS(c, i), 
	    ((i < (num_of_clans-1)) ? " " : "\r\nOptions: "));

  /* Line 7: Options */
  for (i = 0; i < NUM_CO; i++) {
    if (CLAN_HASOPT(c, i)) {
      sprintf(buf, "%s%s%s", buf, ((crap > 0) ? ", " : ""), clan_enablers[i]);
      crap = 1;
    }
  }
  if (crap > 0)
    strcat(buf, "\r\nPrivileges: ");
  else
    strcat(buf, "None Enabled.\r\nPrivileges: ");

  crap = 0;
    

  /* Line 8: Privileges */
  for (i = 0; i < NUM_CP; i++)
    sprintf(buf, "%s%s[%d]%s", buf, clan_privileges[i], clan[c].privilege[i], ((i < (NUM_CP-1)) ? ", " : "\r\nRanks: "));

  /* Line 9: Rank Titles */
  for (i = 0; i < clan[c].ranks; i++)
    sprintf(buf, "%s%d-%s%s", buf, (i+1), clan[c].rank_name[i], ((i < (clan[c].ranks-1)) ? ", " : "\r\n"));

  send_to_char(buf, ch);
}

/* Recruiting New Members */
void do_clan_recruit (struct char_data *ch, char *arg)
{
  struct char_data *vict=NULL;
  int clan_num=-1,immcom=FALSE;
  char cin[80];

  if (!(*arg))
  {
    send_to_char("Just who did you want to recruit?\r\n", ch);
    return; 
  }

  switch (clan_check_priv(ch, CP_RECRUIT))
  {
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_IMM:
      immcom=TRUE;
      break;
    case CPC_PRIV:
      clan_num = find_clan_by_id(GET_CLAN(ch));
      break;
    default: 
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if(!(vict=generic_find_char(ch,arg,FIND_CHAR_ROOM)))
  {
    send_to_char("Er, Who ??\r\n",ch);
    return;
  }

  if (immcom)
    if ((clan_num = find_clan_by_id(GET_CLAN(vict))) == -1)
    {
      send_to_char("They don't seem to be in a clan.\r\n", ch);
      return;
    }

  if(GET_CLAN(vict) != clan[clan_num].id)
  {
    if(GET_CLAN_RANK(vict) > 0)
      send_to_char("They're already in a clan.\r\n",ch);
    else 
      send_to_char("They haven't applied to your clan.\r\n",ch);
    return;
  }
  if(GET_CLAN_RANK(vict) > 0) {
    send_to_char("They're already in your clan.\r\n",ch);
    return;
  }
  if (GET_LEVEL(vict) >= LVL_IS_GOD)
  {
    send_to_char("You cannot recruit immortals in clans.\r\n",ch);
    return; 
  }
  
  GET_CLAN_RANK(vict)++;
  save_char(vict, vict->in_room);
  clan[clan_num].power += GET_LEVEL(vict);
  clan[clan_num].members++;
  sprintf(buf, "You're application to %s has been accepted.\r\n", clan[clan_num].name);
  send_to_char(buf, vict);
  send_to_char("Okay.\r\n",ch);
  sprintf(cin, "%s has been recruited.", GET_NAME(vict));
  do_clan_infocom(GET_CLAN(vict), 0, cin);
  return;
}

/* Cutdown version of get_player_vis .. used by do_clan_banish - ARTUS */
struct char_data *get_player (struct char_data *ch, char *name)
{
  struct char_data *i;
  for (i = character_list; i; i = i->next) 
  {
    if (IS_NPC(i))
      continue;
    if (str_cmp(i->player.name, name))
      continue;
    if (GET_INVIS_LEV(i) > GET_LEVEL(ch))
      continue;
    return i;
  }
  return NULL;
}

/* Banishing Members - Works on online and offline. */
void do_clan_banish (struct char_data *ch, char *arg)
{
  extern int top_of_p_table;
  extern FILE *player_fl;
  int clan_num=-1,immcom=FALSE,i,player_i = 0;
  char cin[80];
  struct char_data *vict;

  if (!(*arg)) {
    send_to_char("Just who did you want to banish?\r\n", ch);
    return;
  }

  switch (clan_check_priv(ch, CP_BANISH)) {
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_IMM:
      immcom=TRUE;
      break;
    case CPC_PRIV:
      clan_num = find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if ((vict = get_player(ch, arg)) != NULL) {
    if (immcom)
      clan_num = find_clan_by_id(GET_CLAN(vict));

    if (!immcom && (GET_CLAN(vict) != clan[clan_num].id)) {
      send_to_char("They don't seem to belong to your clan.\r\n", ch);
      return;
    }
    if (!immcom && (GET_CLAN_RANK(vict) >= GET_CLAN_RANK(ch))) {
      send_to_char("You cannot banish someone who is of equal or higher rank than yourself.\r\n", ch);
      return;
    }
    sprintf(buf, "%s has been banished from your clan.\r\n", GET_NAME(vict));
    send_to_char(buf, ch);
    send_to_char("You have been banished from your clan.\r\n", vict);
    GET_CLAN(vict) = 0;
    GET_CLAN_RANK(vict) = 0;
    clan[clan_num].members--;
    clan[clan_num].power -= GET_LEVEL(vict);
    sprintf(cin, "%s has been banished.", GET_NAME(vict));
    SET_BIT(PLR_FLAGS(vict), PLR_CRASH);
    do_clan_infocom(clan[clan_num].id, 0, cin);
    return;
  }

  if (immcom) {
    send_to_char("Couldn't find that player online.. Perhaps you need to set file 'x' clan 0.\r\n", ch);
    return;
  }

  for (i = 0; i <= top_of_p_table; i++) {
    struct char_file_u tch;
    if ((player_i = load_char(player_table[i].name, &tch)) >= 0) {
      if ((!strcasecmp(tch.name, arg)) && (tch.player_specials_saved.clan == clan[clan_num].id)) {
	if (tch.player_specials_saved.clan_rank >= GET_CLAN_RANK(ch)) {
          send_to_char("Cannot banish someone who's not ranked lower than yourself.\r\n", ch);
	  return;
	}
	i = top_of_p_table;
        sprintf(cin, "%s has been banished.", tch.name);
        do_clan_infocom(clan[clan_num].id, 0, cin);
	tch.player_specials_saved.clan = 0;
	tch.player_specials_saved.clan_rank = 0;
        fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
        fwrite(&tch, sizeof(struct char_file_u), 1, player_fl);
	fflush(player_fl);
        clan[clan_num].members--;
        clan[clan_num].power-=tch.level;
	return;
      }
    }
  }
  
  send_to_char("Could not find anyone by that name belonging to your clan.\r\n", ch);
  return;
}

/* Demotions */
void do_clan_demote (struct char_data *ch, char *arg)
{
  struct char_data *vict=NULL;
  int clan_num=-1,immcom=FALSE;
  char cin[80];

  if (!(*arg)) {
    send_to_char("Just who did you want to demote?\r\n", ch);
    return; 
  }

  switch (clan_check_priv(ch, CP_DEMOTE)) {
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_IMM:
      immcom = TRUE;
      break;
    case CPC_PRIV:
      clan_num = find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }
  if(!(vict=generic_find_char(ch,arg,FIND_CHAR_ROOM)))
  {
    send_to_char("Er, Who ??\r\n",ch);
    return; 
  }
  if(!immcom && GET_CLAN(vict)!=clan[clan_num].id)
  {
    send_to_char("They're not in your clan.\r\n",ch);
    return; 
  }
  if (immcom)
  {
    if ((clan_num = find_clan_by_id(GET_CLAN(vict))) == -1)
    {
      send_to_char("They're not in a clan.\r\n", ch);
      return;
    }
  }
  if(GET_CLAN_RANK(vict)==1)
  {
    send_to_char("They can't be demoted any further, use banish now.\r\n",ch);
    return; 
  }
  if(GET_CLAN_RANK(vict)>=GET_CLAN_RANK(ch) && !immcom)
  {
    send_to_char("You cannot demote a person of this rank!\r\n",ch);
    return; 
  }
  GET_CLAN_RANK(vict)--;
  save_char(vict, vict->in_room);
  sprintf(buf, "%s has demoted you to %s.\r\n", ((CAN_SEE(ch, vict)) ? ch->player.name : "someone"), clan[clan_num].rank_name[GET_CLAN_RANK(vict) - 1]);
  send_to_char(buf, vict);
  send_to_char("Done.\r\n", ch);
  sprintf(cin, "%s has been demoted to %s.", GET_NAME(vict), clan[clan_num].rank_name[GET_CLAN_RANK(vict) - 1]);
  do_clan_infocom(GET_CLAN(vict), 0, cin);
  return;
}

/* Promotions */
void do_clan_promote (struct char_data *ch, char *arg)
{
  struct char_data *vict=NULL;
  int clan_num=-1,immcom=FALSE;
  char cin[80];

  if (!(*arg)) {
    send_to_char("Who is it you wish to promote?\r\n", ch);
    return; 
  }

  switch (clan_check_priv(ch, CP_DEMOTE)) {
    case CPC_IMM:
      immcom = TRUE;
      break;
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_PRIV:
      clan_num = find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if(!(vict=generic_find_char(ch,arg,FIND_CHAR_ROOM)))
  {
    send_to_char("Er, Who ??\r\n",ch);
    return; 
  }

  if (immcom)
  {
    if ((clan_num = find_clan_by_id(GET_CLAN(vict))) == -1)
    {
      send_to_char("They aren't even in a clan.\r\n", ch);
      return;
    }
  }
  
  if(!immcom && GET_CLAN(vict) != GET_CLAN(ch)) {
    send_to_char("They're not in your clan.\r\n",ch);
    return; 
  }

  if(GET_CLAN_RANK(vict) == 0) {
    send_to_char("Their application has not been approved yet.\r\n",ch);
    return; 
  }
  if(!immcom && (GET_CLAN_RANK(vict) >= GET_CLAN_RANK(ch))) {
    send_to_char("You cannot promote that person over your rank!\r\n",ch);
    return; 
  }
  if(GET_CLAN_RANK(vict) == clan[clan_num].ranks) {
    send_to_char("You cannot promote someone over the top rank!\r\n",ch);
    return; 
  }
  GET_CLAN_RANK(vict)++;
  save_char(vict, vict->in_room);
  sprintf(buf, "%s has promoted you to %s.\r\n", ((CAN_SEE(ch, vict)) ? ch->player.name : "someone"), clan[clan_num].rank_name[GET_CLAN_RANK(vict) - 1]);
  send_to_char(buf, vict);
  send_to_char("Done.\r\n", ch);
  sprintf(cin, "%s promoted to %s.", GET_NAME(vict), clan[clan_num].rank_name[GET_CLAN_RANK(vict) - 1]);
  do_clan_infocom(GET_CLAN(vict), 0, cin);
  return;
}

/* Member Listing.. Probably should be sorted. TODO - ARTUS */
void do_clan_who (struct char_data *ch, char *arg)
{
  extern int top_of_p_table;
  int test_clan, clan_num;
  unsigned int i;
  char line_disp[90];

  switch (clan_check_priv(ch, -1)) {
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_IMM:
      if ((test_clan=find_clan_by_sid(arg)) < 0) {
	send_to_char("That clan seems to exist only in your world.\r\n", ch);
	return;
      }
      test_clan=clan[test_clan].id;
      break;
    case CPC_PRIV: 
      test_clan=GET_CLAN(ch);
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  clan_num = find_clan_by_id(test_clan);

  sprintf(buf, "\r\nMembers of %s\r\n", clan[clan_num].name);
  for (i = 0; i <= (strlen(clan[clan_num].name) + 10); i++)
    strcat(buf, "-"); /* Happy, DM? :o) - ARTUS */
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
  for (i = 0; (int)i <= top_of_p_table; i++) {
    struct char_file_u tch;
    if (load_char(player_table[i].name, &tch) >= 0) {
      if ((tch.player_specials_saved.clan == test_clan) && (tch.player_specials_saved.clan_rank > 0)) {
        sprintf(line_disp,"%-15s  %s\r\n", tch.name, clan[clan_num].rank_name[tch.player_specials_saved.clan_rank-1]);
        send_to_char(line_disp,ch); 
      }
    }
  }
  return;
}

void do_clan_about (struct char_data *ch, char *arg) {
  int clan_num;
  
  if ((clan_num = find_clan_by_sid(arg)) < 0) {
    if ((clan_num = find_clan_by_id(GET_CLAN(ch))) < 0) {
      send_to_char ("There is no such clan.\r\n", ch);
      return;
    }
  }

  sprintf(buf, "Description of \"%s\":\r\n\r\n", clan[clan_num].name);
  sprintf(buf, "%s%s\r\n", buf, clan[clan_num].description);
  page_string(ch->desc, buf, 1);
  return;
}

/* Clan Info */
void do_clan_infocom (int clan_one, int clan_two, char *arg) {

  struct descriptor_data *i;

  if (arg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (LR_FAIL(i->character, LVL_CLAN_GOD))
    {
      if ((GET_CLAN(i->character) < 1) && 
	  (GET_LEVEL(i->character) < LVL_CLAN_GOD))
        continue;
      if ((GET_CLAN_RANK(i->character) < 1))
        continue;
      if (((clan_one > 0) || (clan_two > 0)) && 
	  (GET_CLAN(i->character) != clan_one) && 
	  (GET_CLAN(i->character) != clan_two))
        continue;
    }
    if (EXT_FLAGGED(i->character, EXT_NOCI))
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;

    if (GET_LEVEL(i->character) >= LVL_CLAN_GOD) 
      sprintf(buf, "CLAN: [ (%d) %s ]\r\n", clan_one, arg);
    else 
      sprintf(buf, "CLAN: [ %s ]\r\n", arg);
    send_to_char(CCGRN(i->character, C_NRM), i->character);
    send_to_char(buf, i->character);
    send_to_char(CCNRM(i->character, C_NRM), i->character);
  }

  return;
}

/* Display players status within the clan */
void do_clan_status (struct char_data *ch)
{
  char line_disp[90];
  int clan_num;

  clan_num = find_clan_by_id(GET_CLAN(ch));

  if(GET_CLAN_RANK(ch)==0)
  {
    if(clan_num>=0) {
      sprintf(line_disp,"You applied to %s\r\n",clan[clan_num].name);
      send_to_char(line_disp,ch);
      return; 
    }
    send_to_char("You do not belong to a clan!\r\n",ch);
    return; 
  }
  sprintf(line_disp,"You are %s (Rank %d) of %s (ID %d)\r\n",
          clan[clan_num].rank_name[GET_CLAN_RANK(ch)-1],GET_CLAN_RANK(ch),
          clan[clan_num].name,clan[clan_num].id);
  send_to_char(line_disp,ch);

  return;
}

/* Applications to clans.. */
void do_clan_apply (struct char_data *ch, char *arg)
{
  int clan_num, immcom=FALSE;
  char cin[80];

  if((GET_LEVEL(ch) >= LVL_IS_GOD) && (CLAN_NO_IMM == 1))
  {
    send_to_char("Gods cannot apply for any clan.\r\n",ch);
    return; 
  }
  if(!LR_FAIL(ch, LVL_CLAN_GOD))
    immcom = TRUE;

  if(GET_CLAN_RANK(ch) > 0)
  {
    send_to_char("You already belong to a clan!\r\n",ch);
    return; 
  }
  if (!(*arg)) {
    send_to_char("To which clan were you wanting to apply? (Try \"clan list\")\r\n", ch);
    return; 
  }
  if ((clan_num = find_clan_by_sid(arg)) < 0) {
    send_to_char("Unknown clan specified.\r\n", ch);
    return; 
  }

  if (LR_FAIL(ch, clan[clan_num].app_level))
  {
    sprintf(buf, "You must be at least level %d to join %s.\r\n", clan[clan_num].app_level, clan[clan_num].name);
    send_to_char(buf, ch);
    return; 
  }
  if(do_clan_charge(ch, clan[clan_num].app_fee) < 1) {
    sprintf(buf, "You can't afford the %d coin application fee.\r\n", clan[clan_num].app_fee);
    send_to_char(buf, ch);
    return; 
  }

  clan_coinage (clan_num, clan[clan_num].app_fee);

  save_clans();
  GET_CLAN(ch)=clan[clan_num].id;
  REMOVE_BIT(EXT_FLAGS(ch), EXT_PKILL);
  save_char(ch, ch->in_room);
  sprintf(buf, "You submit your application and %d coins to %s.\r\n", clan[clan_num].app_fee, clan[clan_num].name);
  send_to_char(buf, ch);
  sprintf(cin, "%s has applied.", GET_NAME(ch));
  do_clan_infocom(GET_CLAN(ch), 0, cin);
  return;
}

/* Clan Info */
void do_clan_info (struct char_data *ch, char *arg)
{
  int i=0,j=0,x=0,y=0;

  if(num_of_clans == 0) {
    send_to_char("No clans have formed yet.\r\n",ch);
    return; 
  }

  if(!(*arg)) {
    sprintf(buf, "\r");
    for(i=0; i < num_of_clans; i++)
      sprintf(buf, "%s[%-2d]  %-20s  Level: %2d  Appfee: %9d  Tax Rate: %d%%\r\n",buf
 , clan[i].id, clan[i].name,clan[i].level,clan[i].app_fee,clan[i].taxrate);
    page_string(ch->desc,buf, 1);
    return; 
  }
  if ((i=find_clan_by_sid(arg)) < 0) {
    send_to_char("Which clan?!\r\n", ch);
    return;
  }

  sprintf(buf, "Info for the level %d clan, %s:\r\n",clan[i].level,clan[i].name);
  send_to_char(buf, ch);
  sprintf(buf, "Ranks      : %d\r\nTitles     : ",clan[i].ranks);
  for(j=0;j<clan[i].ranks;j++)
    sprintf(buf, "%s%s%s ",buf,clan[i].rank_name[j],((j==clan[i].ranks - 1) ? "." : ","));
    sprintf(buf, "%s\r\nMembers    : %d\r\nPower      : %d\r\n",buf, clan[i].members, clan[i].power);
  for(j = 0; j < num_of_clans; j++) 
  {
    x += GET_CLAN_FRAGS(i, j);
    y += GET_CLAN_DEATHS(i, j);
  }
  sprintf(buf, "%sKill/Deaths: %d/%d\r\n", buf, x, y);
  
  for(j=0; j<5;j++)
    if(clan[i].spells[j])
      sprintf(buf, "%s%d ",buf,clan[i].spells[j]);
  sprintf(buf, "%s\r\n",buf);
  send_to_char(buf, ch);
  sprintf(buf,"Clan privileges:\r\n");
  for(j=0; j<NUM_CP;j++)
    sprintf(buf, "%s   %-10s: %s%s",buf,clan_privileges[j],clan[i].rank_name[clan[i].privilege[j]-1], (((j % 2) == 0) ? "     " : "\r\n"));
  sprintf(buf, "%s%s",buf,(((NUM_CP % 2) == 1) ? "\r\n\r\n" : "\r\n"));
  send_to_char(buf, ch);
  sprintf(buf, "Application fee  : %d gold\r\nTax Rate         : %d%%\r\n", clan[i].app_fee, clan[i].taxrate);
  send_to_char(buf, ch);
  sprintf(buf, "Application level: %d\r\n", clan[i].app_level);
  send_to_char(buf, ch);

  return;
}

/* Find clan by either string idnum or name */
sh_int find_clan_by_sid(char *test)
{
  if (is_number(test)) 
    return find_clan_by_id(atoi(test));

  for (int i = 0; i < num_of_clans; i++)
    if (is_abbrev(test, clan[i].name))
      return i;

  return -1;
}

/* Finding index of clan by ID number. */
sh_int find_clan_by_id(int idnum)
{
  int i;
  for( i=0; i < num_of_clans; i++)
    if(idnum==(clan[i].id))
      return i;
  return -1;
}

/* Finding index of clan by clan name.. Only really used for dupe name check */
sh_int find_clan(char *name)
{
  int i;
  for( i=0; i < num_of_clans; i++)
    if(strcmp(CAP(name), CAP(clan[i].name))==0)
      return i;
  return -1;
}

/* Save clans to disk */
void save_clans()
{
  FILE *fl;

  if (!(fl = fopen(CLAN_FILE, "wb"))) {
    basic_mud_log("SYSERR: Unable to open clan file");
    return; 
  }

  fwrite(&num_of_clans, sizeof(int), 1, fl);
  fwrite(clan, sizeof(struct clan_rec), num_of_clans, fl);
  fclose(fl);
  return;
}

/* Initialise clans */
void init_clans()
{
  FILE *fl;
  int i,j;
  extern int top_of_p_table;
  struct char_file_u chdata;

  memset(clan,0,sizeof(struct clan_rec)*MAX_CLANS);
  num_of_clans=0;
  i=0;

  if (!(fl = fopen(CLAN_FILE, "rb"))) {
    basic_mud_log("   Clan file does not exist. Will create a new one");
    save_clans();
    return; 
  }

  fread(&num_of_clans, sizeof(int), 1, fl);
  fread(clan, sizeof(struct clan_rec), num_of_clans, fl);
  fclose(fl);

  basic_mud_log("   Calculating powers and members");
  for(i=0;i<num_of_clans;i++) {
    clan[i].power=0;
    clan[i].members=0;
  }
  for (j = 0; j <= top_of_p_table; j++){
    load_char((player_table + j)->name, &chdata);
    if((i=find_clan_by_id(chdata.player_specials_saved.clan))>=0) {
      clan[i].power+=chdata.level;
      clan[i].members++;
    }
  }
  return;
}

/* Clan balance, withdraw, deposit */
void do_clan_bank(struct char_data *ch, char *arg, int action)
{
  int clan_num,immcom=0,result;
  long amount=0;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  if (action >= CB_BALANCE)
    result = clan_check_priv(ch, CP_WITHDRAW);
  else
    result = clan_check_priv(ch, -1);
  
  switch (result) {
    case CPC_IMM:  
      immcom = 1; 
      half_chop(arg,arg1,arg2);
      strcpy(arg,arg2);
      if ((clan_num = find_clan_by_sid(arg1)) < 0) {
	send_to_char("That clan does not seem to exist.\r\n", ch);
	return;
      }
      break;
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_PRIV:
      clan_num = find_clan_by_id(GET_CLAN(ch));
      break;
    default: send_to_char("Bug. Please report.\r\n", ch); return;
  }

  if ((!(*arg)) && (action != CB_BALANCE)) {
    sprintf(buf, "How much did you want to %s?\r\n", ((action == CB_DEPOSIT) ? "Deposit" : "Withdraw"));
    send_to_char(buf, ch);
    return; 
  }
 
  amount=atoi(arg);

  switch(action) {
    case CB_BALANCE:
      if (clan[clan_num].treasure > 0) 
        sprintf (buf, "The treasury of %s is at %ld00 coins.\r\n", clan[clan_num].name, clan[clan_num].treasure);
      else 
        sprintf (buf, "The treasury of %s is seeking donations.\r\n", clan[clan_num].name);
      send_to_char(buf, ch);
      break;
    case CB_WITHDRAW:
      if ((amount < 100) || ((amount % 100) > 0)) {
	send_to_char("You must withdraw a multiple of 100 coins.\r\n", ch);
	return;
      }
      if ((int)clan[clan_num].treasure < (amount / 100)) {
	send_to_char("Your clan is not *that* wealthy!\r\n", ch);
	return;
      }
      GET_GOLD(ch)+=(amount);
      clan[clan_num].treasure-=(amount / 100);
      send_to_char("You withdraw from the clan's treasure.\r\n",ch);
      break;
    case CB_DEPOSIT:
      if ((amount < 100) || ((amount % 100) > 0)) {
	send_to_char("You must deposit in multiples of 100 coins.\r\n", ch);
	return;
      }
      if(!immcom) { 
        if (GET_GOLD(ch) < amount) {
  	  send_to_char("You don't have that much gold!\r\n", ch);
	  return;
        }
	GET_GOLD(ch)-=amount;
      }
      clan_coinage(clan_num, amount);
      send_to_char("You add to the clan's treasure.\r\n",ch);
      break;
    default:
      send_to_char("Problem in command, please report.\r\n",ch);
      break;
  }
  save_char(ch, ch->in_room); 
  save_clans();
  return;
}

/* Changing tax rate, appfee */
void do_clan_changefees(struct char_data *ch, char *arg, int action)
{
  int clan_num,immcom=0;
  long amount=0;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  if (!(*arg)) {
    switch(action) {
      case CM_APPFEE:
	if (LR_FAIL(ch, LVL_CLAN_GOD))
	  send_to_char("Syntax: clan set appfee <amount>\r\n", ch);
	else
	  send_to_char("Syntax: clan set appfee <clan num> <amount>\r\n", ch);
        break;
      case CM_DUES:
	if (LR_FAIL(ch, LVL_CLAN_GOD))
	  send_to_char("Syntax: clan set tax <2-50>  (This number is a percentage.\r\n", ch);
	else
	  send_to_char("Syntax: clan set tax <clan num> <2-50>  (Percentage)\r\n", ch);
	break;
      default:
	send_to_char("Bug in command. Please report.\r\n", ch);
    }
    return; 
  }

  switch (clan_check_priv(ch, CP_SET_FEES)) {
    case CPC_IMM:
      immcom = 1;
      half_chop(arg,arg1,arg2);
      strcpy(arg,arg2);
      if ((clan_num = find_clan_by_sid(arg1)) < 0) {
	send_to_char("That clan doesn't appear to exist.\r\n", ch);
	return;
      }
      break;
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if(!(*arg)) {
    send_to_char("Set it to how much?\r\n",ch);
    return;
  }

  if(!is_number(arg)) {
    send_to_char("Set it to what?\r\n",ch);
    return;
  }

  amount=atoi(arg);

  switch(action) {
    case CM_APPFEE:
      if ((amount < CLAN_MIN_APP) || (amount > CLAN_MAX_APP)) {
	sprintf(buf, "Application fee must be between %d and %d coins.\r\n", CLAN_MIN_APP, CLAN_MAX_APP);
	send_to_char(buf, ch);
	return;
      }
      amount = (int)(amount / 100); /* I know this seems silly.. But need the result to be */
      amount *= 100; /* a multiple of 100 coins. Trust me. - ARTUS          */
      clan[clan_num].app_fee=amount;
      send_to_char("You change the application fee.\r\n",ch);
      break;
    case CM_DUES:
      if ((amount < CLAN_MIN_TAX) || (amount > CLAN_MAX_TAX)) {
        sprintf(buf, "Clan tax must be between %d and %d\r\n", CLAN_MIN_TAX, CLAN_MAX_TAX);
      }
      if (do_clan_charge(ch, CLC_CHGFEE) < 1) {
        sprintf(buf, "You need %d gold to change the tax rate.\r\n", CLC_CHGFEE);
        send_to_char(buf, ch);
        return;
      }
      clan[clan_num].taxrate=amount;
      sprintf(buf, "Taxrate changed to %ld. (%d coins deducted.)\r\n", amount,
              (immcom) ? 0 : CLC_CHGFEE);
      send_to_char(buf,ch);
      break;
    default:
      send_to_char("Problem in command, please report.\r\n",ch);
      break;
  }

  save_clans();
  return;
}

/* Changing Number of clan ranks.. */
void do_clan_set_ranks(struct char_data *ch, char *arg)
{
  int i,j;
  int clan_num,immcom=0;
  int new_ranks,amtreq=0;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char cin[80];
  extern int top_of_p_table;
  struct char_file_u chdata;
  struct char_data *victim=NULL;

  switch (clan_check_priv(ch, CP_LEADER)) {
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_IMM:
      immcom=1;
      half_chop(arg,arg1,arg2);
      strcpy(arg,arg2);
      if ((clan_num = find_clan_by_sid(arg1)) < 0) {
        send_to_char("Unknown clan specified.\r\n", ch);
        return;
      }
      break;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if(!(*arg)) {
    send_to_char("Set how many ranks?\r\n",ch);
    return;
  } 

  if(!is_number(arg) && immcom) {
    send_to_char("Set the ranks to what?\r\n",ch);
    return;
  }

  new_ranks=atoi(arg);

  if (new_ranks == clan[clan_num].ranks) {
    sprintf(buf, "%s already consists of %d ranks.\r\n", clan[clan_num].name, clan[clan_num].ranks);
    send_to_char(buf, ch);
    return;
  }

  if (new_ranks > clan[clan_num].ranks)
    for (i = (clan[clan_num].ranks+1); i <= new_ranks; i++)
      amtreq += (i * CLC_ADDRANK);

  if(new_ranks<2 || new_ranks>20) {
    send_to_char("Number of ranks must be between 2 and 20.\r\n",ch);
    return;
  }

  if(do_clan_charge(ch, amtreq) < 1) {
    sprintf(buf, "You will need %d coins to expand to %d ranks.\r\n", amtreq, new_ranks);
    send_to_char(buf, ch);
    return;
  }

  for (j = 0; j <= top_of_p_table; j++)
  {
    if((victim=get_player_online(ch, (player_table +j)->name, FIND_CHAR_INVIS)))
    {
      if(GET_CLAN(victim)==clan[clan_num].id) 
      {
        if(GET_CLAN_RANK(victim)<clan[clan_num].ranks && GET_CLAN_RANK(victim)>0)
          GET_CLAN_RANK(victim)=MIN(GET_CLAN_RANK(victim), new_ranks - 1); /* Should Keep Ranks In Some Orderly Fashion */
        if(GET_CLAN_RANK(victim)==clan[clan_num].ranks)
          GET_CLAN_RANK(victim)=new_ranks;
        save_char(victim, victim->in_room);
      }
    } else {
      load_char((player_table + j)->name, &chdata);
      if(chdata.player_specials_saved.clan==clan[clan_num].id) {
        if(chdata.player_specials_saved.clan_rank<clan[clan_num].ranks && chdata.player_specials_saved.clan_rank>0)
          chdata.player_specials_saved.clan_rank=MIN(chdata.player_specials_saved.clan_rank, new_ranks -1); /* Rank preservation technique */
        if(chdata.player_specials_saved.clan_rank==clan[clan_num].ranks)
          chdata.player_specials_saved.clan_rank=new_ranks;
        save_char_file_u(chdata);
      }
    }
  }

  if (clan[clan_num].ranks > new_ranks)
    sprintf(cin, "Number of ranks reduced to %d.", new_ranks);
  if (clan[clan_num].ranks < new_ranks)
    sprintf(cin, "Number of ranks increased to %d.", new_ranks);

  do_clan_infocom(clan[clan_num].id, 0, cin);
  sprintf(buf, "Number of ranks is now %d. (%d coins deducted)\r\n", new_ranks,
      (immcom) ? 0 : amtreq);
  send_to_char(buf, ch);
  strcpy(clan[clan_num].rank_name[new_ranks - 1], clan[clan_num].rank_name[clan[clan_num].ranks - 1]);  
  if (new_ranks > clan[clan_num].ranks) {
    for(i=0;i<new_ranks-1;i++) {
      if (i >= (clan[clan_num].ranks - 1))
        strcpy(clan[clan_num].rank_name[i],"Undefined Rank");
    }
  } 

  for(i=0;i<NUM_CP;i++)
    if (clan[clan_num].privilege[i]==clan[clan_num].ranks)
      clan[clan_num].privilege[i]=new_ranks;
    else
      clan[clan_num].privilege[i]=MIN(clan[clan_num].privilege[i], new_ranks);

  clan[clan_num].ranks=new_ranks;

  save_clans();
  return;
}

/* Changing clan rank titles */
void do_clan_titles( struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], cin[80];
  int clan_num=0,rank;

  if (!(*arg)) {
    if (LR_FAIL(ch, LVL_CLAN_GOD))
      send_to_char("Syntax: clan set title <rank #> <title>\r\n", ch);
    else
      send_to_char("Syntax: clan set title <clan #> <rank #> <title>\r\n", ch);
    return; 
  }

  if (GET_LEVEL(ch) < LVL_IS_GOD)
  {
    if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0)
    {
      send_to_char("You don't belong to any clan!\r\n",ch);
      return;
    }
    if(GET_CLAN_RANK(ch)!=clan[clan_num].ranks) {
      send_to_char("You're not influent enough in the clan to do that!\r\n",ch);
      return;
    }
  } else {
    if (LR_FAIL(ch, LVL_CLAN_GOD))
    {
      send_to_char("You do not have clan privileges.\r\n", ch);
      return; 
    }
    half_chop(arg,arg1,arg2);
    strcpy(arg,arg2);
    if((clan_num=find_clan_by_sid(arg1))<0) {
      send_to_char("Unknown clan specified.\r\n",ch);
      return;
    }
  }

  half_chop(arg,arg1,arg2);

  if(is_number(arg1) < 1) {
    send_to_char("You need to specify a rank number.\r\n",ch);
    return; 
  }

  rank=atoi(arg1);

  if(rank<1 || rank>clan[clan_num].ranks) {
    send_to_char("This clan has no such rank number.\r\n",ch);
    return; 
  }

  if(strlen(arg2)<1 || strlen(arg2)>19) {
    send_to_char("You need a clan title of under 20 characters.\r\n",ch);
    return; 
  }

  if(do_clan_charge(ch, CLC_RANKTITLE) < 1) {
    sprintf(buf, "Changing rank titles will cost you %d.\r\n", CLC_RANKTITLE);
    send_to_char(buf, ch);
    return;
  }
  sprintf(cin, "Rank %d changed to: %s.", rank, arg2);
  do_clan_infocom(clan[clan_num].id, 0, cin);

  strcpy(clan[clan_num].rank_name[rank-1],arg2);
  save_clans();
  sprintf(buf, "Done for %d coins.\r\n", (!LR_FAIL(ch, LVL_CLAN_GOD)) ? 0 : CLC_RANKTITLE);
  send_to_char(buf, ch);
  return;
}

/* CHange clans minimum application level. */
void do_clan_application( struct char_data *ch, char *arg)
{
  int clan_num,immcom=0;
  int applevel;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char cin[80];

  switch (clan_check_priv(ch, CP_SET_APPLEV)) {
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_IMM:
      immcom=1;
      half_chop(arg,arg1,arg2);
      strcpy(arg,arg2);
      if ((clan_num = find_clan_by_sid(arg1)) < 0) {
        send_to_char("Unknown clan specified.\r\n", ch);
        return;
      }
      break;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if (!(*arg))
  {
    if (LR_FAIL(ch, LVL_CLAN_GOD))
      send_to_char("Syntax: clan set applev <level>\r\n", ch);
    else
      send_to_char("Syntax: clan set applev <clan #> <level>\r\n", ch);
    return; 
  }

  applevel=atoi(arg);

  if(applevel<LVL_CLAN_MIN || applevel>100) {
    sprintf(buf, "The application level must be between %d and 100.\r\n",LVL_CLAN_MIN);
    send_to_char(buf, ch);
    return;
  }

  if (do_clan_charge(ch, CLC_CHGAPPLEV) < 1) {
    sprintf(buf, "Changing the application level requires %d coins.\r\n", CLC_CHGAPPLEV);
    send_to_char(buf, ch);
    return;
  }

  sprintf(buf, "Players must now be level %d to apply to %s.\r\n", applevel, clan[clan_num].name);
  send_to_char(buf, ch);

  sprintf(cin, "Application level changed to %d.", applevel);
  do_clan_infocom(clan[clan_num].id, 0, cin);

  clan[clan_num].app_level=applevel;
  save_clans();

  return;
}

/* Set Clan Privledges */
void do_clan_sp(struct char_data *ch, char *arg, int priv)
{
  int clan_num,immcom=0;
  int rank;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  if (!(*arg))
  {
    if (LR_FAIL(ch, LVL_CLAN_GOD))
      send_to_char("Syntax: clan set privilege <privilege> <rank #>\r\n", ch);
    else
      send_to_char("Syntax: clan set privilege <privilege> <clan #> <rank #>\r\n", ch);
    return; 
  }

  switch (clan_check_priv(ch, CP_LEADER)) {
    case CPC_NOCLAN:
    case CPC_NOPRIV:
      return;
    case CPC_IMM:
      immcom=1;
      half_chop(arg,arg1,arg2);
      strcpy(arg,arg2);
      if ((clan_num = find_clan_by_sid(arg1)) < 0) {
        send_to_char("Unknown clan specified.\r\n", ch);
        return;
      }
      break;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if(!(*arg)) {
    send_to_char("Set the privilege to which rank?\r\n",ch);
    return;
  }

  if(!is_number(arg)) {
    send_to_char("Set the privilege to what?\r\n",ch);
    return;
  }

  rank=atoi(arg);

  if(rank<1 || rank>clan[clan_num].ranks) {
    send_to_char("There is no such rank in the clan.\r\n",ch);
    return;
  }

  send_to_char("Done.\r\n", ch);

  clan[clan_num].privilege[priv]=rank;
  save_clans();

  return;
}

/* Change clan description (clan about) */
void do_clan_set_desc(struct char_data *ch, char *arg)
{
  int clan_num;

  switch(clan_check_priv(ch, CP_SET_DESC)) {
    case CPC_NOPRIV:
    case CPC_NOCLAN:
      return;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    case CPC_IMM:
      if (!(*arg)) {
        send_to_char("Which clan's description did you want to set?\r\n", ch);
        return;
      }
      if ((clan_num = find_clan_by_sid(arg)) < 0) {
        send_to_char("Unknown clan specified.\r\n", ch);
        return; 
      }
      break;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if ((GET_GOLD(ch) < CLC_SETDESC) && LR_FAIL(ch, LVL_CLAN_GOD))
  {
    sprintf(buf, "Changing the clan description will cost %d coins.\r\n", CLC_SETDESC);
    send_to_char(buf, ch);
    return;
  }

  if(strlen(clan[clan_num].description)==0) {
    strcat(clan[clan_num].description, "An unfinished clan."); 
    sprintf(buf, "Enter the description for \"%s\": (/h for help)\r\n\r\n",clan[clan_num].name);
    send_to_char(buf, ch);
  } else {
    send_to_char("Enter new description: (/h for help)\r\n\r\n", ch);
    sprintf(buf, "Old description for \"%s\":\r\n", clan[clan_num].name);
    send_to_char(buf, ch);
    page_string(ch->desc, clan[clan_num].description, 1);
  }
 /******************
  * Should look at changing it in future to return something... So that we
  * know whether for the description was changed or not, and can reimburse 
  * the coin if that be the case... *shrugs*.. Dunno how it'd work.. Coin 
  * at this stage will be taken in modify.c...
  ******************/
  do_odd_write(ch->desc, PWT_CLANDESC, CLAN_DESC_LENGTH);
}

/* Clan Privileges */
void do_clan_set_priv( struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
  int i;

  half_chop(arg,arg1,arg2);

  if (is_abbrev(arg1,"setabout" )) { do_clan_sp(ch,arg2,CP_SET_DESC);   return ;}
  if (is_abbrev(arg1,"recruit"  )) { do_clan_sp(ch,arg2,CP_RECRUIT);    return ;}
  if (is_abbrev(arg1,"banish"   )) { do_clan_sp(ch,arg2,CP_BANISH);     return ;}
  if (is_abbrev(arg1,"promote"  )) { do_clan_sp(ch,arg2,CP_PROMOTE);    return ;}
  if (is_abbrev(arg1,"demote"   )) { do_clan_sp(ch,arg2,CP_DEMOTE);     return ;}
  if (is_abbrev(arg1,"withdraw" )) { do_clan_sp(ch,arg2,CP_WITHDRAW);   return ;}
  if (is_abbrev(arg1,"setfees"  )) { do_clan_sp(ch,arg2,CP_SET_FEES);   return ;}
  if (is_abbrev(arg1,"setapplev")) { do_clan_sp(ch,arg2,CP_SET_APPLEV); return ;}
  if (is_abbrev(arg1,"enhance"  )) { do_clan_sp(ch,arg2,CP_ENHANCE);    return ;}
  if (is_abbrev(arg1,"room"     )) { do_clan_sp(ch,arg2,CP_ROOM);       return ;}
  if (is_abbrev(arg1,"setdesc"  )) { do_clan_sp(ch,arg2,CP_SET_DESC);   return ;}
  if (is_abbrev(arg1,"enable"   )) { do_clan_sp(ch,arg2,CP_ENABLE);     return ;
}
  send_to_char("Clan privileges:\r\n", ch);
  for(i=0;i<NUM_CP;i++) {
    sprintf(arg1,"\t%s\r\n",clan_privileges[i]);
    send_to_char(arg1,ch); 
  }  
}
 
/* Clan Room Stuff */
void do_clan_room(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int clan_num = 0;
  zone_rnum real_zone(zone_vnum vnum);

  switch(clan_check_priv(ch, CP_SET_DESC)) {
    case CPC_NOPRIV:
    case CPC_NOCLAN:
      return;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    case CPC_IMM:
      send_to_char("Use OLC you slacker.\r\n", ch);
      return;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }
  if((clan_num=find_clan_by_id(GET_CLAN(ch)))<0) {
    send_to_char("You don't belong to any clan!\r\n",ch);
    return; 
  }
  if ((GET_ROOM_VNUM((ch)->in_room) < (CLAN_ZONE * 100)) || (GET_ROOM_VNUM((ch)->in_room) > ((CLAN_ZONE * 100)+351))) { 
    send_to_char("That room doesn't appear to belong to your clan!\r\n", ch);
    return;
  }
  clan_num = (clan[clan_num].id - 1);
  if (((GET_ROOM_VNUM((ch)->in_room)-(CLAN_ZONE*100)) % MAX_CLANS) != clan_num) {
    send_to_char("You don't appear to be in your clan hall.\r\n", ch);
    return;
  }
  half_chop(arg, arg1, arg2);
  if (is_abbrev(arg1, "title")) {
    if (strlen(arg2) < 2) {
      send_to_char("You must specify the new title.\r\n", ch);
      return;
    }
    if (strlen(arg2) > 75) {
      send_to_char("New title is too long, please keep to under 75 chars.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_ROOMTD) < 1) {
      sprintf(buf, "You'll need %d gold coins to change the room title.\r\n", CLC_ROOMTD);
      send_to_char(buf, ch);
      return;
    }
    free(world[ch->in_room].name);
    world[ch->in_room].name = strdup(arg2);
    
    if (save_rooms(world[ch->in_room].zone) == TRUE) {
      sprintf(buf, "Done. (%d coins deducted).\r\n", CLC_ROOMTD);
      send_to_char(buf, ch);
    } else {
      send_to_char("Failed to save clan zone to disk.\r\n", ch);
      if (LR_FAIL(ch, LVL_CLAN_GOD))
	GET_GOLD(ch) += CLC_ROOMTD;
    }
    return;
  }
  if (is_abbrev(arg1, "regen")) {
    if ((GET_ROOM_VNUM((ch)->in_room) < (CLAN_ZONE * 100 + MAX_CLANS-1))) {
      send_to_char("Cannot make the guard post 2xRegen.\r\n", ch);
      return;
    }
    if (ROOM_FLAGGED((ch)->in_room, ROOM_REGEN_2)) {
      send_to_char("This room is already marked REGENx2.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_REGENROOM) < 1) {
      sprintf(buf, "You'll need %d coins to make this room REGENx2.\r\n", CLC_REGENROOM);
      send_to_char(buf, ch);
      return;
    }
    TOGGLE_BIT(world[ch->in_room].room_flags, ROOM_REGEN_2);
    clan_num = save_rooms(world[ch->in_room].zone);
    if (clan_num == TRUE) {
      sprintf(buf, "Current room is now Regen*2. (%d coins deducted).\r\n",
          CLC_REGENROOM);
      send_to_char(buf, ch);
      return;
    }
    send_to_char("Failed to save clan zone to disk.\r\n", ch);
    if (LR_FAIL(ch, LVL_CLAN_GOD))
      GET_GOLD(ch) += CLC_REGENROOM;
    return;
  }
  if (is_abbrev(arg1, "description")) {
    if ((GET_GOLD(ch) < CLC_ROOMTD) && LR_FAIL(ch, LVL_CLAN_GOD))
    {
      sprintf(buf, "Changing a room's description costs %d coins, which you do not have.\r\n", CLC_ROOMTD);
      send_to_char(buf, ch);
      return;
    }
    ch->player_specials->write_extra = ch->in_room;
    do_odd_write(ch->desc, PWT_ROOM, 800);
    return;
  }
  if (is_abbrev(arg1, "build")) {
    int i, j, new_room = -1, direction;
    if ((GET_ROOM_VNUM((ch)->in_room) < (CLAN_ZONE * 100 + MAX_CLANS-1)))
    {
      send_to_char("Cannot build rooms off the guard post.\r\n", ch);
      return;
    }
    direction = clan_room_dirs(arg2);
    if (direction < 0)
    {
      send_to_char("What kind of direction is that?!\r\n", ch);
      return;
    }
    for (i = 1; i < CLAN_ROOM_MAX; i++)
    {
      char found = 1;
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (W_EXIT(real_room((i * MAX_CLANS) + clan_num + (CLAN_ZONE * 100)), j))
          found = 0;
      if ((found == 1) && (new_room < 1)) 
        new_room = real_room((i * MAX_CLANS) + clan_num + (CLAN_ZONE * 100));
    }
    if (world[ch->in_room].dir_option[direction])
    {
      send_to_char("There already seems to be a room there!\r\n", ch);
      return;
    }
    if (new_room <= 0)
    {
      send_to_char("There is no room to expand your hall further.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_BUILD) < 1)
    {
      sprintf(buf, "Hiring builders for your new room will cost %d coins.\r\n", CLC_BUILD);
      send_to_char(buf, ch);
      return;
    }
    CREATE(world[ch->in_room].dir_option[direction], struct room_direction_data, 1);
    CREATE(world[new_room].dir_option[rev_dir[direction]], struct room_direction_data, 1);
    world[ch->in_room].dir_option[direction]->to_room = new_room;
    world[new_room].dir_option[rev_dir[direction]]->to_room = ch->in_room;
    clan_num = save_rooms(real_zone(CLAN_ZONE));
    if (clan_num != TRUE)
    {
      send_to_char("Failed to save clan zone to disk.\r\n", ch);
      if (LR_FAIL(ch, LVL_CLAN_GOD))
        GET_GOLD(ch) += CLC_BUILD;
      return;
    }
    sprintf(buf, "The builders charge %d coins for their work.\r\n", CLC_BUILD);
    send_to_char(buf, ch);
    return;
  }

/* Demolish -- This is currently unsafe.. It doesn't crash, it just causes
 * problems.. Eventually I should get around to rewriting it so it can be
 * useful.. *shrugs*..                                                   */

/*  if (is_abbrev(arg1, "demolish")) {
    int direction, old_rnum, demlow, demhigh;
    send_clan_format(ch);
    return;
    if ((GET_ROOM_VNUM((ch)->in_room) < (CLAN_ZONE * 100 + MAX_CLANS-1))) {
      send_to_char("Cannot demolish rooms off the guard post.\r\n", ch);
      return;
    }
    demlow = (CLAN_ZONE * 100) + (MAX_CLANS * 2);
    demhigh = demlow + (MAX_CLANS * CLAN_ROOM_MAX) - 1; 
    direction = clan_room_dirs(arg2);
    if (direction == -1) {
      send_to_char("You seem unable to demolish in that direction.\r\n", ch);
      return;
    } else if (direction == -2) {
      send_to_char("What kind of direction is that?!\r\n", ch);
      return;
    }
    if (!(world[ch->in_room].dir_option[direction])) {
      send_to_char("There doesn't appear to be a room there!\r\n", ch);
      return;
    }
    old_rnum = world[ch->in_room].dir_option[direction]->to_room;
      if ((world[old_rnum].number < demlow) || (world[old_rnum].number > demhigh)) {
      send_to_char("There doesn't seem to be any way to demolish that room.\r\n", ch);
      return;
    }
    world[old_rnum].dir_option[rev_dir[direction]] = NULL;
    world[ch->in_room].dir_option[direction] = NULL;
    send_to_char("The ground shakes violently as your clan room caves in to nothing.\r\n", ch);
    save_rooms(real_zone(CLAN_ZONE));
    return;
  } */
   
  send_to_char("The following room changes are available:-\r\n\r\n"
               "   clan room title    <title>\r\n"
               "   clan room desc\r\n"
               "   clan room regen\r\n"
               "   clan room build    <dir>\r\n", ch);
//               "   clan room demolish <dir>\r\n", ch);
}

/* Change name of healer/guard */
void do_clan_name_mob(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int clan_num=0;
  struct char_data *mob, *live_mob;
  
  zone_rnum real_zone(zone_vnum vnum);

  switch(clan_check_priv(ch, CP_ENHANCE))
  {
    case CPC_NOPRIV:
    case CPC_NOCLAN:
      return;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    case CPC_IMM:
      send_to_char("Use OLC you slacker.\r\n", ch);
      return;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  if ((clan_num=find_clan_by_id(GET_CLAN(ch))) < 0)
  {
    send_to_char("You don't seem to belong to a clan.\r\n", ch);
    return;
  }

  clan_num = clan[clan_num].id - 1;
  half_chop(arg,arg1,arg2);

  if (is_abbrev(arg1, "guard"))
  {
    if (!(mob=generic_find_char(ch, arg1, FIND_CHAR_ROOM)))
    {
      send_to_char("Your clan guard doesn't seem to be here.\r\n", ch);
      return;
    }
    if (GET_MOB_VNUM(mob) != CLAN_ZONE*100+clan_num)
    {
      send_to_char("Why would you want to name their guard?!\r\n", ch);
      return;
    }
    if (strlen(arg2) < 10)
    {
      send_to_char("Guard's name must be at least 10 chars.\r\n", ch);
      return;
    }
    if (strlen(arg2) >= 78)
    {
      send_to_char("Guard's name cannot exceed 78 characters.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_MOBDESC) < 1)
    {
      sprintf(buf, "Naming your guard will cost %d coins.\r\n", CLC_MOBDESC);
      send_to_char(buf, ch);
      return;
    }

    sprintf(buf, "Done. (%d coins deducted).\r\n", CLC_MOBDESC);
    send_to_char(buf, ch);
    strcat(arg2, "\r\n");

    /* Should probably put this in a function when can be fragged - ARTUS */
    mob=&mob_proto[mob->nr];
    free(GET_LDESC(mob));
    GET_LDESC(mob) = str_dup(arg2);

    if (save_mobiles(real_zone(CLAN_ZONE)) == FALSE)
    {
      send_to_char("Unable to save zone file.\r\n", ch);
      GET_GOLD(ch) += CLC_MOBDESC;
    }
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (mob->nr == live_mob->nr)
        update_mobile_strings(live_mob, &mob_proto[mob->nr]);

    return;
  }
  if (is_abbrev(arg1, "healer"))
  {
    if (!(mob=generic_find_char(ch, arg1, FIND_CHAR_ROOM)))
    {
      send_to_char("Your clan healer doesn't seem to be here.\r\n", ch);
      return;
    }
    if (GET_MOB_VNUM(mob) != (CLAN_ZONE*100+clan_num+MAX_CLANS))
    {
      send_to_char("Why would you want to name their healer?!\r\n", ch);
      return;
    }
    if (strlen(arg2) < 10)
    {
      send_to_char("Healer's name must be at least 10 characters.\r\n", ch);
      return;
    }
    if (strlen(arg2) >= 78)
    {
      send_to_char("Healer's name cannot exceed 78 characters.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_MOBDESC) < 1)
    {
      sprintf(buf, "Naming your healer will cost %d coins.\r\n", CLC_MOBDESC);
      send_to_char(buf, ch);
      return;
    }
    sprintf(buf, "Done. (%d coins deducted).\r\n", CLC_MOBDESC);
    send_to_char(buf, ch);

    strcat(arg2, "\r\n");
    mob=&mob_proto[mob->nr];
    free(GET_LDESC(mob));
    GET_LDESC(mob) = str_dup(arg2);

    if (save_mobiles(real_zone(CLAN_ZONE)) == FALSE)
    {
      send_to_char("Unable to save zone file.\r\n", ch);
      GET_GOLD(ch) += CLC_MOBDESC;
    }

    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (mob->nr == live_mob->nr)
        update_mobile_strings(live_mob, &mob_proto[mob->nr]);

    return;
  }

  send_to_char("Just what did you want to name?\r\n", ch);
  return;
  
}

/* Change description of healers/guards */
void do_clan_describe_mob(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int clan_num=0;
  struct char_data *mob;
  zone_rnum real_zone(zone_vnum vnum);

  switch(clan_check_priv(ch, CP_ENHANCE)) {
    case CPC_NOPRIV:
    case CPC_NOCLAN:
      return;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    case CPC_IMM:
      send_to_char("Use OLC you slacker.\r\n", ch);
      return;
    default:
      send_to_char("Bug. Please report.\r\n", ch);
      return;
  }

  clan_num = clan[clan_num].id - 1;
  half_chop(arg,arg1,arg2);

  if (is_abbrev(arg1, "guard"))
  {
    strcpy(arg1, "clanguard");
    if (!(mob=generic_find_char(ch, arg1, FIND_CHAR_ROOM)))
    {
      send_to_char("Your clan guard doesn't seem to be here.\r\n", ch);
      return;
    }
    if (GET_MOB_VNUM(mob) != CLAN_ZONE*100+clan_num)
    {
      send_to_char("Why would you want to describe their guard?!\r\n", ch);
      return;
    }
  } else if (is_abbrev(arg1, "healer")) {
    sprintf(arg1, "clanhealer");
    if (!(mob=generic_find_char(ch, arg1, FIND_CHAR_ROOM)))
    {
      send_to_char("Your clan healer doesn't seem to be here.\r\n", ch);
      return;
    }
    if (GET_MOB_VNUM(mob) != (CLAN_ZONE*100+clan_num+MAX_CLANS))
    {
      send_to_char("Why would you want to describe their healer?!\r\n", ch);
      return;
    }
  } else {
    send_to_char("Describe what?!?\r\n", ch);
    return;
  }
  if (GET_GOLD(ch) < CLC_MOBDESC)
  {
    sprintf(buf, "Changing it's appearance will require %d coins.\r\n", CLC_MOBDESC);
    send_to_char(buf, ch);
  }
  ch->player_specials->write_extra = mob->nr;
  do_odd_write(ch->desc, PWT_MOB, EXDSCR_LENGTH);
  return;
}

/* Display Clan Relations to members */

/* Ideas for relationships and their affects...
 *
 * Well... Atm, relationships are just text... That's no fun now is it..
 * To Consider:
 * 
 * Blood War - Clan members auto pk each other (devin)
 * War - Cannot bribe way into hall, cannot group (artus) [Done]
 *
 * Peaceful - Bonus when grouping (artus)
 * Alliance - Enter halls at free will. (artus) [Done]
 * 
 */

void do_clan_showrel (struct char_data *ch, char *arg) {
  int i, n, c;

  switch (clan_check_priv(ch, -1)) {
    case CPC_NOPRIV:
    case CPC_NOCLAN:
      return;
      break;
    case CPC_IMM:
      if((c=find_clan_by_sid(arg))<0) {
	send_to_char("Unknown clan specified.\r\n", ch);
	return;
      }
      break;
    case CPC_PRIV:
      c=find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug in command. Please report.\r\n", ch);
      return;
  }

  if (num_of_clans == 1) {
    send_to_char ("There is only one clan in existance.\r\n", ch);
    return;
  }

  send_to_char("Clan Relationships:\r\n"
	       "-------------------\r\n", ch);
  for (i = 0; i < num_of_clans; i++) {
    n = 0;
    if (i == c)
      continue;
    
    n = GET_CLAN_REL(c, i);
    sprintf(buf, "%-32s:  %5d  (", clan[i].name, n);
    if ((n >= CLAN_REL_MIN) && (n < CLAN_REL_BLOOD_WAR))
      strcat (buf, "blood feud");
    else if (n < CLAN_REL_WAR)
      strcat (buf, "war");
    else if (n < CLAN_REL_ANGER) 
      strcat (buf, "angered");
    else if (n < CLAN_REL_UPSET)
      strcat (buf, "upset");
    else if (n < CLAN_REL_NEUTRAL)
      strcat (buf, "neutral");
    else if (n < CLAN_REL_FRIENDLY)
      strcat (buf, "friendly");
    else if (n < CLAN_REL_TRADE)
      strcat (buf, "trade");
    else if (n < CLAN_REL_PEACEFUL)
      strcat (buf, "peaceful");
    else if (n < CLAN_REL_MAX)
      strcat (buf, "alliance");
    else /* Should never get here right... */
      strcat (buf, "VALUE OOB!");
    strcat (buf, ")\r\n");
    send_to_char(buf, ch);
  }
  return;
}

/* Clan Enable Format */
void send_clan_enable_f(struct char_data *ch)
{
  int c,r;
  c=find_clan_by_id(GET_CLAN(ch));
  r=GET_CLAN_RANK(ch);
  if (c < 0) 
  {
    if (LR_FAIL(ch, LVL_CLAN_GOD))
    {
      send_to_char("You don't seem to belong to a clan.\r\n", ch);
      return;
    }
    send_to_char("The following clan options can be enabled/disabled:-\r\n"
	         "hall    - Clan Hall\r\n"
	         "healer  - Clan Healer\r\n"
	         "board   - Clan Board\r\n"
	         "ctalk   - Clan && Clan Leader Channels.\r\n", ch);
    return;
  }
  if (r < clan[c].privilege[CP_ENABLE]) 
  {
    send_to_char("You are not privileged enough to use this command.\r\n",ch);
    return;
  }
  sprintf(buf, "The following clan options can be enabled:-\r\n");
  sprintf(buf, "%shall      - Clan Hall%s\r\n", buf, (CLAN_HASOPT(c, CO_HALL) ? " (enabled)" : ""));
  sprintf(buf, "%shealer    - Clan Healer%s\r\n", buf, (CLAN_HASOPT(c, CO_HEALER) ? " (enabled)" : ""));
  sprintf(buf, "%sboard     - Clan Board%s\r\n", buf, (CLAN_HASOPT(c, CO_BOARD) ? " (enabled)" : ""));
  sprintf(buf, "%sctalk     - Clan && Clan Leader Channels.%s\r\n", buf, (CLAN_HASOPT(c, CO_TALK) ? " (enabled)" : ""));
  send_to_char(buf, ch);
}

/* Clan Enable */
void do_clan_enable(struct char_data *ch, char *arg) 
{

  int c=0;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  if (!(*arg)) {
    send_clan_enable_f(ch);
    return;
  }

  switch (clan_check_priv(ch, CP_ENABLE)) {
    case CPC_NOPRIV:
    case CPC_NOCLAN:
      return;
    case CPC_IMM:
      half_chop(arg,arg1,arg2);
      strcpy(arg,arg2);
      if((c=find_clan_by_sid(arg1))<0) {
	send_to_char("Unknown clan specified.\r\n", ch);
	return;
      }
      break;
    case CPC_PRIV:
      if((c=find_clan_by_id(GET_CLAN(ch))) < 0) {
	send_to_char("Bug in command. Please report.\r\n", ch);
	return;
      }
      break;
    default:
      send_to_char("Bug in command. Please report.\r\n", ch);
      return;
  }

  if (is_abbrev(arg, "hall")) {
    if (CLAN_HASOPT(c, CO_HALL)) {
      send_to_char("Your clan already seems to have a hall.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_ENHALL) > 0) {
      sprintf(buf, "Done. (%d coins deducted).\r\n", CLC_ENHALL);
      send_to_char(buf, ch);
      SET_BIT(COF_FLAGS(c), (1 << CO_HALL));
      do_clan_infocom(clan[c].id, 0, "Your clan hall has been enabled.");
      save_clans();
      return;
    } 
    sprintf(buf, "Readying the clan hall requires %d coins.\r\n", CLC_ENHALL);
    send_to_char(buf, ch);
    return;
  }
  if (is_abbrev(arg,"ctalk")) {
    if (CLAN_HASOPT(c, CO_TALK)) {
      send_to_char("Your clan already has their chanel enabled.\r\n", ch);
      return;
    } 
    if (do_clan_charge(ch, CLC_ENTALK) > 0) {
      sprintf(buf, "Done. (%d coins deducted.)\r\n", CLC_ENTALK);
      send_to_char(buf, ch);
      SET_BIT(COF_FLAGS(c), (1 << CO_TALK));
      do_clan_infocom(clan[c].id, 0, "Clan Channel Now Enabled.");
      save_clans();
      return;
    }
    sprintf(buf, "You will need %d coins to activate the clantalk channel.\r\n", CLC_ENTALK);
    send_to_char(buf, ch);
    return;
  }
  if (is_abbrev(arg,"healer")) {
    if (CLAN_HASOPT(c, CO_HEALER)) {
      send_to_char("Your clan's healer has already been set up.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_ENHEAL) > 0) {
      sprintf(buf, "Done. (%d coins deducted).\r\n", CLC_ENHEAL);
      send_to_char(buf, ch);
      SET_BIT(COF_FLAGS(c), (1 << CO_HEALER));
      do_clan_infocom(clan[c].id, 0, "Clan Healer Now Enabled.");
      save_clans();
      return;
    }
    sprintf(buf, "You will need %d coins to activate the clan healer.\r\n", CLC_ENHEAL);
    send_to_char(buf, ch);
    return;
  }
  if (is_abbrev(arg,"board")) {
    if (CLAN_HASOPT(c, CO_BOARD)) {
      send_to_char("Your clan's board is already enabled.\r\n", ch);
      return;
    }
    if (do_clan_charge(ch, CLC_ENBOARD) > 0) {
      sprintf(buf, "Done. (%d coins deducted).\r\n", CLC_ENBOARD);
      send_to_char(buf, ch);
      SET_BIT(COF_FLAGS(c), (1 << CO_BOARD));
      do_clan_infocom(clan[c].id, 0, "Clan Board Now Enabled.");
      save_clans();
      return;
    }
    sprintf(buf, "You will need %d coins to activate the clan board.\r\n", CLC_ENBOARD);
    send_to_char(buf, ch);
    return;
  }
  send_clan_enable_f(ch);
  return;
}

/* Enhance Guards/Healers */
void do_clan_enhance(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int clan_num=0, old_vnum=0, amtreq=0, i=0;
  struct char_data *mob;	       /* For Mob Copies..         */
  struct char_data *oldmob;       
  zone_rnum real_zone(zone_vnum vnum); /* For Saving To File       */
  room_rnum gpost;		       /* Remember where mob was.. */

  switch (clan_check_priv(ch, CP_ENHANCE)) {
    case CPC_NOPRIV:
    case CPC_NOCLAN:
      return;
    case CPC_IMM:
      send_to_char("Use OLC you slacker!\r\n", ch);
      break;
    case CPC_PRIV:
      clan_num=find_clan_by_id(GET_CLAN(ch));
      break;
    default:
      send_to_char("Bug in command. Please report.\r\n", ch);
      return;
  }
  
  clan_num = clan[clan_num].id - 1;
  half_chop(arg,arg1,arg2);
  if (is_abbrev(arg1, "guard")) {
    sprintf(arg1, "clanguard");
    if (!(oldmob=generic_find_char(ch, arg1, FIND_CHAR_ROOM)))
    {
      send_to_char("Your clan guard doesn't seem to be here.\r\n", ch);
      return;
    }
    if (GET_MOB_VNUM(oldmob) != CLAN_ZONE*100+clan_num)
    {
      send_to_char("Why would you want to enhance their guard?!\r\n", ch);
      return;
    }
    if (GET_LEVEL(oldmob) >= 100)
    {
      send_to_char("They don't come any tougher than that!\r\n", ch);
      return;
    }
    amtreq = CLC_EGUARD;
    for (i = 50; i <= 90; i += 10)
      if (GET_LEVEL(oldmob) > i)
	amtreq += CLC_EGUARD;
    if (do_clan_charge(ch, amtreq) < 1)
    {
      sprintf(buf, "Enhancing your guard will require %d coins.\r\n", amtreq);
      send_to_char(buf, ch);
      return;
    }
  } else if (is_abbrev(arg1, "healer")) {
    sprintf(arg1, "clanhealer");
    if (!(oldmob=generic_find_char(ch, arg1, FIND_CHAR_ROOM)))
    {
      send_to_char("Your clan healer doesn't seem to be here.\r\n", ch);
      return;
    }
    if (GET_MOB_VNUM(oldmob) != (CLAN_ZONE*100+clan_num+MAX_CLANS))
    {
      send_to_char("Why would you want to enhance their healer?!\r\n", ch);
      return;
    }
    if (GET_LEVEL(oldmob) >= 100)
    {
      send_to_char("They don't come any tougher than that!\r\n", ch);
      return;
    }

    amtreq = CLC_EHEAL;
    for (i = 50; i <= 90; i += 10)
      if (GET_LEVEL(oldmob) > i)
	amtreq += CLC_EHEAL;

    if (do_clan_charge(ch, amtreq) < 1)
    {
      sprintf(buf, "Enhancing your healer requires %d coins.\r\n", amtreq);
      send_to_char(buf, ch);
      return;
    }
  } else {
    send_to_char("Enhance what?!?\r\n", ch);
    return;
  }

  gpost = oldmob->in_room;

  /* Firstly, Lets work with the stuff from do_clan_reset .. */
  mob = &mob_proto[oldmob->nr];
  
  switch (GET_LEVEL(mob))
  {
    case 60:
      GET_LEVEL(mob) = 70;
      GET_NDD(mob) = GET_SDD(mob) = 25;
      GET_HITROLL(mob) = 75;
      GET_DAMROLL(mob) = 75;
      GET_HIT(mob) = GET_MANA(mob) = 1;
      GET_MOVE(mob) = 10000;
      GET_AC(mob) = -125;
      break;
    case 70:
      GET_LEVEL(mob) = 80;
      GET_NDD(mob) = GET_SDD(mob) = 25;
      GET_HITROLL(mob) = 90;
      GET_DAMROLL(mob) = 90;
      GET_HIT(mob) = GET_MANA(mob) = 1;
      GET_MOVE(mob) = 15000;
      GET_AC(mob) = -140;
      break;
    case 80:
      GET_LEVEL(mob) = 90;
      GET_NDD(mob) = GET_SDD(mob) = 30;
      GET_HITROLL(mob) = GET_DAMROLL(mob) = 115;
      GET_HIT(mob) = GET_MANA(mob) = 1;
      GET_MOVE(mob) = 20000;
      GET_AC(mob) = -200;
      break;
    case 90:
      GET_LEVEL(mob) = 100;
      GET_NDD(mob) = GET_SDD(mob) = 50;
      GET_HITROLL(mob) = GET_DAMROLL(mob) = 127;
      GET_HIT(mob) = GET_MANA(mob) = 1;
      GET_MOVE(mob) = 30000;
      GET_AC(mob) = -250;
      break;
    default:
      GET_LEVEL(mob) = 60;
      GET_NDD(mob) = GET_SDD(mob) = 25;
      GET_HITROLL(mob) = 65;
      GET_DAMROLL(mob) = 65;
      GET_HIT(mob) = GET_MANA(mob) = 1;
      GET_MOVE(mob) = 8000;
      GET_AC(mob) = -110;
      break;
  }

  for (oldmob = character_list; oldmob; oldmob = oldmob->next)
    if (oldmob->nr == mob->nr) 
      extract_char(oldmob);

  old_vnum = save_mobiles(real_zone(CLAN_ZONE));

  reset_zone(real_zone(CLAN_ZONE));

  sprintf(buf, "Training it to level %d cost %d coins.\r\n", GET_LEVEL(mob),
      amtreq);

  send_to_char(buf, ch);
  return;

}

/* Clan Set */
void do_clan_set(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
  int c=0,r=0;

  half_chop(arg,arg1,arg2);

  if (is_abbrev(arg1, "about"    )) { do_clan_set_desc(ch,arg2);        return ; }
  if (is_abbrev(arg1, "ranks"    )) { do_clan_set_ranks(ch,arg2);       return ; }
  if (is_abbrev(arg1, "title"    )) { do_clan_titles(ch,arg2);          return ; }
  if (is_abbrev(arg1, "privilege")) { do_clan_set_priv(ch,arg2);        return ; }
  if (is_abbrev(arg1, "tax"      )) { do_clan_changefees(ch,arg2,CM_DUES);   return ; }
  if (is_abbrev(arg1, "appfee"   )) { do_clan_changefees(ch,arg2,CM_APPFEE); return ; }
  if (is_abbrev(arg1, "applev"   )) { do_clan_application(ch,arg2);     return ; }

  if (!LR_FAIL(ch, LVL_CLAN_GOD))
  {
    send_to_char("The following set commands are available to you:\r\n\r\n"
		 "   clan set about     <clan #>\r\n"
		 "   clan set appfee    <clan #>    <amount>\r\n"
                 "   clan set applev    <clan #>    <level>\r\n"
                 "   clan set privilege <privilege> <clan #>   <rank>\r\n"
                 "   clan set ranks     <clan #>    <number>\r\n"
                 "   clan set tax       <clan #>    <2-50>\r\n"
                 "   clan set title     <clan #>    <rank #>   <title>\r\n",ch);
    return;
  }

  if ((c=find_clan_by_id(GET_CLAN(ch))) < 0) 
  {
    send_to_char("But you do not belong to a clan!\r\n", ch);
    return;
  }
  
  r=GET_CLAN_RANK(ch);

  if ((r < clan[c].privilege[CP_SET_DESC]) &&
      (r < clan[c].privilege[CP_SET_FEES]) &&
      (r < clan[c].privilege[CP_SET_APPLEV])) {
    send_to_char("You are not privileged enough to use this command.\r\n", ch);
    return;
  }

  send_to_char("You have access to the following set commands:\r\n\r\n", ch);
	  
  if(r>=clan[c].privilege[CP_SET_DESC])
    send_to_char("   clan set about\r\n", ch);
  if(r>=clan[c].privilege[CP_SET_FEES])
    send_to_char("   clan set appfee    <amount>\r\n",ch);
  if(r>=clan[c].privilege[CP_SET_APPLEV])
    send_to_char("   clan set applev    <level>\r\n",ch);
  if(r==clan[c].ranks)
    send_to_char("   clan set privilege <privilege> <rank>\r\n"
                 "   clan set rank      <num ranks>\r\n",ch);
  if(r>=clan[c].privilege[CP_SET_FEES])
    send_to_char("   clan set tax       <2-50>\r\n",ch);
  if(r==clan[c].ranks)
    send_to_char("   clan set title     <rank #> <title>\r\n",ch);
  return;

}

/* Used To Check Validity of Room Build/Demolish Arguments. */
int clan_room_dirs (char *arg) {
  if (is_abbrev(arg, "north"))
    return(NORTH);
  if (is_abbrev(arg, "south"))
    return(SOUTH);
  if (is_abbrev(arg, "east"))
    return(EAST);
  if (is_abbrev(arg, "west"))
    return(WEST);
  if (is_abbrev(arg, "up"))
    return(UP);
  if (is_abbrev(arg, "down")) 
    return(DOWN);
  else 
    return (-1);
}

/* Determine what to do */
ACMD(do_clan)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "create"  )) { do_clan_create(ch,arg2);   return ;}
  if (is_abbrev(arg1, "destroy" )) { do_clan_destroy(ch,arg2);  return ;}
  if (is_abbrev(arg1, "recruit" )) { do_clan_recruit(ch,arg2);  return ;}
  if (is_abbrev(arg1, "banish"  )) { do_clan_banish(ch,arg2);   return ;}
  if (is_abbrev(arg1, "who"     )) { do_clan_who(ch,arg2);      return ;}
  if (!LR_FAIL(ch, LVL_CLAN_GOD))
    if (is_abbrev(arg1, "stat"  )) { do_clan_stat(ch,arg2);     return ;}
  if (is_abbrev(arg1, "status"  )) { do_clan_status(ch);        return ;}
  if (is_abbrev(arg1, "info"    )) { do_clan_info(ch,arg2);     return ;}
  if (is_abbrev(arg1, "list"    )) { do_clan_info(ch,arg2);     return ;}
  if (is_abbrev(arg1, "apply"   )) { do_clan_apply(ch,arg2);    return ;}
  if (is_abbrev(arg1, "demote"  )) { do_clan_demote(ch,arg2);   return ;}
  if (is_abbrev(arg1, "promote" )) { do_clan_promote(ch,arg2);  return ;}
  if (is_abbrev(arg1, "set"     )) { do_clan_set(ch,arg2);      return ;}
  if (is_abbrev(arg1, "about"   )) { do_clan_about(ch, arg2);   return ;}
  if (is_abbrev(arg1, "enhance" )) { do_clan_enhance(ch,arg2);  return ;}
  if (is_abbrev(arg1, "name"    )) { do_clan_name_mob(ch,arg2);     return ;}
  if (is_abbrev(arg1, "describe")) { do_clan_describe_mob(ch,arg2); return ;}
  if (is_abbrev(arg1, "room"    )) { do_clan_room(ch,arg2);     return ;}
  if (is_abbrev(arg1, "withdraw")) { do_clan_bank(ch,arg2,CB_WITHDRAW); return ;}
  if (is_abbrev(arg1, "deposit" )) { do_clan_bank(ch,arg2,CB_DEPOSIT);  return ;}
  if (is_abbrev(arg1, "balance" )) { do_clan_bank(ch,arg2,CB_BALANCE);  return ;}
  if (is_abbrev(arg1, "relations")){ do_clan_showrel(ch,arg2);  return ;} 
  if (is_abbrev(arg1, "score"   )) { do_clan_score(ch);         return ;}
  if (is_abbrev(arg1, "enable"  )) { do_clan_enable(ch,arg2);   return ;}
  if (is_abbrev(arg1, "reset"   )) { do_clan_reset(ch,arg2);    return ;}
  send_clan_format(ch);
}

/* Enable player killing. -- Irreversable except by joining a clan. */
ACMD(do_pkill)
{
  if (IS_NPC(ch))
  {
    send_to_char("NPCs cannot be pkill.\r\n", ch);
    return;
  }
  if (GET_LEVEL(ch) >= LVL_CHAMP + MIN(5, GET_UNHOLINESS(ch)))
  {
    send_to_char("Thou shall not kill mortals.\r\n", ch);
    return;
  }
  if ((EXT_FLAGGED(ch, EXT_PKILL)) || 
      ((GET_CLAN(ch) > 0) && (find_clan_by_id(GET_CLAN(ch)) >= 0)))
  {
    send_to_char("You are already pkill material.\r\n", ch);
    return;
  }
  skip_spaces(&argument);
  if ((!(*argument)) || (strcasecmp("enable", argument)))
  {
    send_to_char("Becoming pkill is irreversable, until you join a clan.\r\n"
		 "To become pkill, enter 'pkill enable'.\r\n", ch);
    return;
  }
  send_to_char("You enter the violent world of player killing.\r\n", ch);
  SET_BIT(EXT_FLAGS(ch), EXT_PKILL);
}
  
/* Used by healer specproc.. */
int do_clan_heal(struct char_data *ch, struct char_data *vict)
{
  int clan_num, add_h = 0, add_m = 0, add_v = 0;

  clan_num = (1 + GET_MOB_VNUM(ch) - (CLAN_ZONE * 100) - MAX_CLANS);
  if ((clan_num < 1) || (clan_num > MAX_CLANS))
  {
    sprintf(buf, "SYSERR: %s (#%d) calling do_clan_heal()", GET_NAME(ch),
	    GET_MOB_VNUM(ch));
    mudlog(buf, NRM, LVL_GOD, TRUE);
    return FALSE;
  }

  if (GET_CLAN(vict) != clan_num)
    return FALSE;

  if ((GET_HIT(vict) >= GET_MAX_HIT(vict)) && 
      (GET_MOVE(vict) >= GET_MAX_MOVE(vict)) && 
      (GET_MANA(vict) >= GET_MAX_MANA(vict)))
    return FALSE;

  switch GET_LEVEL(ch)
  {
    case 50: /* Level 50, Heal */
      add_h = 100 + number(0, 50);
      break;
    case 60: /* Level 60, Adv Heal */
      add_h = 300 + number(0, 50);
      break;
    case 70: /* Level 70, Adv Heal / Refresh */
      add_h = 300 + number(0, 50);
      add_v = 75 + number(0, 25);
      break;
    case 80: /* Level 80, Adv Heal / Refresh / Mana */
      add_h = 300 + number(0, 50);
      add_v = 75 + number(0, 25);
      add_m = 100 + number(0, 25);
      break;
    case 90: /* Level 90, Div Heal / Refresh / Adv Mana */
      add_h = 400 + number(0, 75);
      add_v = 75 + number(0, 25);
      add_m = 300 + number(0, 50);
      break;
    case 100: /* Level 100, (Restore/2), Div Heal, Refresh, Adv Mana */
      if ((GET_HIT(vict) < (GET_MAX_HIT(vict) >> 1)) || 
	  (GET_MOVE(vict) < (GET_MAX_MOVE(vict) >> 1)) || 
	  (GET_MANA(vict) < (GET_MAX_MANA(vict) >> 1)))
      {
        GET_MOVE(vict) = MAX((GET_MAX_MOVE(vict) >> 1) + 1, GET_MOVE(vict));
        GET_MANA(vict) = MAX((GET_MAX_MANA(vict) >> 1) + 1, GET_MANA(vict));
        GET_HIT(vict) = MAX((GET_MAX_HIT(vict) >> 1)+ 1, GET_HIT(vict));
	return TRUE;
      }
      add_h = 400 + number(0, 75);
      add_v = 75 + number(0, 25);
      add_m = 300 + number(0, 50);
      break;
    case 110: /* Level 110 (Imm Set Only), Restore - Should make em Jizzm */
      GET_HIT(vict) = GET_MAX_HIT(vict);
      GET_MANA(vict) = GET_MAX_MANA(vict);
      GET_MOVE(vict) = GET_MAX_MOVE(vict);
      return TRUE;
    default:
      send_to_char("The clan healer is too busy contemplating the meaning of life to heal you.\r\n", vict);
      sprintf(buf, "Default case in do_clan_heal. (Mob Vnum: %d)\r\n", 
	      GET_MOB_VNUM(ch));
      mudlog(buf, NRM, LVL_GRGOD, TRUE);
      return FALSE;
  }
  if (GET_HIT(vict) < GET_MAX_HIT(vict))
  {
    GET_HIT(vict) += add_h;
    if (GET_HIT(vict) > GET_MAX_HIT(vict))
      GET_HIT(vict) = GET_MAX_HIT(vict);
    return TRUE;
  }
  if ((GET_MANA(vict) < GET_MAX_MANA(vict)) && (add_m > 0))
  {
    GET_MANA(vict) += add_m;
    if (GET_MANA(vict) > GET_MAX_MANA(vict))
      GET_MANA(vict) = GET_MAX_MANA(vict);
    return TRUE;
  }
  if ((GET_MOVE(vict) < GET_MAX_MOVE(vict)) && (add_v > 0))
  {
    GET_MOVE(vict) += add_v;
    if (GET_MOVE(vict) > GET_MAX_MOVE(vict))
      GET_MOVE(vict) = GET_MAX_MOVE(vict);
    return TRUE;
  }
  return FALSE;
}


/* Function for collecting taxes.. */
void clan_tax_update (struct char_data *ch, struct char_data *vict)
{
  int clan_num, tax_taken;

  if (IS_NPC(ch)) 
    return;
  if ((GET_CLAN(ch) < 1) || (GET_CLAN_RANK(ch) < 1))
    return;

  clan_num = find_clan_by_id(GET_CLAN(ch));
  if (clan_num < 0) 
    return;

  tax_taken = (GET_GOLD(vict) * clan[clan_num].taxrate) / 100;
  tax_taken /= 100;
  tax_taken *= 100;

  if (tax_taken < 100)
    return;

  sprintf(buf, "Your clan takes %d coins in taxes.\r\n", tax_taken);
  send_to_char(buf, ch);
  clan_coinage(clan_num, tax_taken);
  GET_GOLD(vict) -= tax_taken;
}

/* Whether or not clan members can hear whats going on.. */
int clan_can_hear(struct descriptor_data *d, int c) {
  if (STATE(d) != CON_PLAYING) 
    return FALSE;
  if (EXT_FLAGGED(d->character, EXT_NOCT))
    return FALSE;
  if (PLR_FLAGGED(d->character, PLR_WRITING))
    return FALSE;
  if (ROOM_FLAGGED(d->character->in_room, ROOM_SOUNDPROOF))
    return FALSE;
  if (!LR_FAIL(d->character, LVL_CLAN_GOD))
    return TRUE;
  if (GET_CLAN(d->character) < 1)
    return FALSE;

  if (c < 0) {
    if (GET_CLAN_RANK(d->character) != clan[find_clan_by_id(GET_CLAN(d->character))].ranks)
      return FALSE;
  } else if (GET_CLAN(d->character) != c) {
      return FALSE;
  }
  if (!CLAN_HASOPT(find_clan_by_id(GET_CLAN(d->character)), CO_TALK))
    return FALSE;

  return TRUE;
}

#if 0 // Artus> Now Unused.
/* Make mobs fight players that are AGGRAVATE punished. */
int pun_aggro_check (struct char_data *ch)
{
  struct char_data *i;
  int found = 0;

  for (i = world[ch->in_room].people; i; i = i->next_in_room) {
    if (!IS_NPC(i))
      continue;
    if ((STATE(ch->desc) == CON_PLAYING) && (i->in_room == ch->in_room) && (FIGHTING(i) == NULL) && (CAN_SEE(i, ch)) && (AWAKE(i)) && (mob_index[GET_MOB_RNUM(i)].func != shop_keeper) && !MOB_FLAGGED(i, MOB_NOKILL)) {
      act("$n senses dishonour in you, and attacks hastily.", FALSE, i, 0, ch, TO_VICT );
      hit(i, ch, -1); // -1 - TYPE_UNDEFINED
      found = 1;
    }
  }
  return found;
}
#endif

/* Function for updating punishment on tick.. */
void punish_update (struct char_data *ch) 
{
  int i;
  char *unpunish_messages[] = {
    "The ice around you suddenly melts away to nothing.\r\n", /* Freeze */
    "You feel your freedom to speak return.\r\n", /* Mute */
    "Your ability to change your title returns.\r\n", /* Notitle */
    "A lag monster sneaks out from your backpack and disappears.\r\n", /* Lag */
    "You feel more eager to learn.\r\n", /* Low_Exp */
    "Your blood starts to course normally once more.\r\n", /* Low_Regen */
    "You feel the anger towards you lessen.\r\n" /* Aggravate */
  };

  if (PUN_FLAGS(ch) == 0)
    return;

  for (i = 0; i < 31; i++) 
    if (ch->player_specials->saved.phours[i] > 0) 
    {
      ch->player_specials->saved.phours[i]--;
      if (ch->player_specials->saved.phours[i] == 0) 
      {
	REMOVE_BIT(PUN_FLAGS(ch), (1 << i));
        send_to_char(unpunish_messages[i], ch);
      }
    }
}

/* Determine the punishment number by name.. */
int parse_punish(char *arg)
{
  int i;

  for (i = 0; i < NUM_PUNISHES; i++)
    if (is_abbrev(arg, punish_types[i]))
      return i;

  return -1;
}

/* Determine the offence number by name.. */
int parse_offence(char *arg)
{
  int i;

  if (!strcmp("0", arg))
    return -1;

  for (i = 0; i < NUM_OFFENCES; i++)
    if (is_abbrev(arg, offence_types[i]))
      return i;

  return -2;
}

/* Function For Setting Punishments - Victim, PUN_xxx, Num_Hours, OFF_xxx */
void perform_punish (struct char_data * ch, int ptype, int hours, int offence)
{
  if (IS_NPC(ch)) /* Mobs can fuck themselves :o) */
    return;

  if (hours == 0) { /* Remove Punishment on 0 hours */
    REMOVE_BIT(PUN_FLAGS(ch), (1 << ptype));
    PUN_HOURS(ch, ptype) = 0;
    return;
  }

  if ((offence >= 0) && (hours != 0))
  {
    HAS_OFFENDED(ch, offence)++;
    if (HAS_OFFENDED(ch, offence) < 1) HAS_OFFENDED(ch, offence) = 1;
  }

  if (hours < 0) { /* Unlimited on <0 hours */
    SET_BIT(PUN_FLAGS(ch), (1 << ptype));
    PUN_HOURS(ch, ptype) = -1;
    return;
  }

  if (PUN_HOURS(ch, ptype) < 0)
    return;

  /* Set it already :o) */
  SET_BIT(PUN_FLAGS(ch), (1 << ptype));
  PUN_HOURS(ch, ptype) = hours;
  return;
}

/* Function for updating frag counts && relations on pk. */
/* BUG: From time to time, it seems this function gets called twice,
 * probably have to look at somewhere else to call it from.. */
void clan_pk_update (struct char_data *winner, struct char_data *loser)
{
  int win_clan, lose_clan, relmod, level_diff, severity;
  char cin[80];
  int level_exp(struct char_data *ch, int level);

  /* No Mobs */
  if (IS_NPC(winner) || IS_NPC(loser))
    return;

  /* Credit players for kills/deaths... */
  if (GET_LEVEL(loser) >= LVL_CHAMP)
    GET_IMMKILLS(winner)++;
  else
    GET_PCKILLS(winner)++;
  if (GET_LEVEL(winner) >= LVL_CHAMP)
    GET_KILLSBYIMM(loser)++;
  else
    GET_KILLSBYPC(loser)++;
  

  REMOVE_BIT(EXT_FLAGS(loser), EXT_PKILL);

  /* TODO - Work out some funky algorithms && punishments for pking little
   * guys as big guys and shit.. */

  if ((INSTIGATOR(winner) == TRUE) && (GET_LEVEL(winner) < LVL_ANGEL))
  {
    level_diff = GET_LEVEL(winner) - GET_LEVEL(loser);
    level_diff -= 10;
    if (GET_LEVEL(winner) >= LVL_CHAMP)
      severity = level_diff * 3;
    else if (GET_LEVEL(winner) >= LVL_ETRNL1)
      severity = level_diff * 2;
    else
      severity = level_diff;
    if (severity > 1) {
      level_diff = number(0, (NUM_PUNISHES-1));
      severity += HAS_OFFENDED(winner, OFF_PKILL);
      sprintf (buf, "Killing someone that small is weak. You are punished for the next %d hours.\r\n", severity);
      send_to_char(buf, winner);
      perform_punish(winner, level_diff, (PUN_HOURS(winner, level_diff) + severity), OFF_PKILL);
    }
  }

  if (GET_LEVEL(loser) >= LVL_CHAMP)
  {
    if ((GET_LEVEL(winner) < GET_LEVEL(loser)) &&
	(GET_LEVEL(loser) == LVL_CHAMP + GET_UNHOLINESS(loser)))
    {
      demote_level(loser, GET_LEVEL(loser)-1, "Lost PK vs Lesser.");
      GET_EXP(loser) = level_exp(loser, GET_LEVEL(loser)) / 2;
    }
  }

  /* Remove Instigator Bits */
  INSTIGATOR(winner) = FALSE;
  INSTIGATOR(loser)  = FALSE;

  win_clan = find_clan_by_id(GET_CLAN(winner));
  lose_clan= find_clan_by_id(GET_CLAN(loser));
  
  if (lose_clan < 0) /* Loser didn't belong to a clan. Who cares? */
    return;

  if (win_clan < 0) 
  {
    sprintf(cin, "%s pkilled by %s.", GET_NAME(loser), GET_NAME(winner));
    do_clan_infocom(GET_CLAN(loser), 0, cin);
    return;
  }

  if (win_clan == lose_clan)
  {
    sprintf(cin, "%s pkilled by %s.", GET_NAME(loser), GET_NAME(winner));
    do_clan_infocom(GET_CLAN(loser), 0, cin);
    return;
  }

  sprintf(cin, "%s pkilled by %s.", GET_NAME(loser), GET_NAME(winner));
  do_clan_infocom(GET_CLAN(loser), GET_CLAN(winner), cin);

  relmod = 0;
  if (GET_CLAN_RANK(loser) == clan[lose_clan].ranks)
    relmod -= 50;
  if (GET_CLAN_RANK(winner) == clan[win_clan].ranks)
    relmod -= 50;

  if (GET_CLAN_RANK(loser) == clan[lose_clan].ranks-1)
    relmod -= 25;
  if (GET_CLAN_RANK(winner) == clan[win_clan].ranks-1)
    relmod -= 25;

  if (GET_CLAN_RANK(winner) < clan[win_clan].ranks-1)
    relmod -= 10;
  if (GET_CLAN_RANK(loser) < clan[lose_clan].ranks-1)
    relmod -= 10;

  GET_CLAN_FRAGS(win_clan, lose_clan)++;
  GET_CLAN_DEATHS(lose_clan, win_clan)++;

  clan_rel_change(win_clan, lose_clan, relmod);
  save_clans();
}

/* Specprocs */
SPECIAL(clan_healer)
{
  struct char_data *to = NULL;
  struct char_data *self = (struct char_data *) me;
  int clan_id = ((GET_MOB_VNUM(self) - (CLAN_ZONE * 100) - MAX_CLANS)+1);

  if (cmd) 
    return FALSE;

  if (GET_MOB_VZNUM(self) != CLAN_ZONE)
  {
    sprintf(buf, "SYSERR: %s (#%d) calling clan_healer specproc.", 
	    GET_NAME(self), GET_MOB_VNUM(self));
    mudlog(buf, NRM, LVL_ANGEL, TRUE);
    return FALSE;
  }
  
  if (!CLAN_HASOPT(find_clan_by_id(clan_id), CO_HEALER))
    return (TRUE);

  for (to = world[self->in_room].people; to; to = to->next_in_room)
    if (!IS_NPC(to))
      if (do_clan_heal(self, to))
	send_to_char ("The clan healer touches you. You feel mildly healthier.\r\n", to);
      
  return (TRUE);
}

/* Clan Guard Specproc */
SPECIAL(clan_guard)
{
  struct char_data *self = (struct char_data *) me;
  int amount, clan_id, amt_req = 0, us, them;

  clan_id = ((GET_MOB_VNUM(self) - (CLAN_ZONE * 100))+1);

  if (!ch->desc || IS_NPC(ch))
    return (FALSE); /* Mobs can stay outside. */

  if (!CMD_IS("enter") && !(CMD_IS("bribe")))
    return (FALSE); /* Common practice, really. */

  us = find_clan_by_id(clan_id);
  them = find_clan_by_id(GET_CLAN(ch));

  if (LR_FAIL(ch, LVL_CLAN_GOD))
  {
    if ((them < 0)) // Char is not in clan.
    {
      act("$n slaps you, 'Thou art not a part of this clan. Begone!'", FALSE, self, 0, ch, TO_VICT);
      act("$N slaps $n.", FALSE, ch, 0, self, TO_ROOM);
      return(TRUE);
    } 

    if (!CLAN_HASOPT(find_clan_by_id(clan_id), CO_HALL)) 
    {
      act("$n &rtells you, 'This clan's hall has not been built yet.'&n", FALSE, self, 0, ch, TO_VICT);
      return (TRUE);
    }

    if (CMD_IS("enter")) 
    { /* Only Allow Clan members... */
      if ((us != them) && !CLAN_AT_PEACE(us, them))
      {
	act("$n slaps you, 'Thou art not a part of this clan. Begone!'", FALSE, self, 0, ch, TO_VICT);
	act("$N slaps $n.", FALSE, ch, 0, self, TO_ROOM);
	return(TRUE);
      } else if (GET_CLAN_RANK(ch)==0) {
	act("$n &rtells you, 'Thy application must be approved before entering.'&n", FALSE, self, 0, ch, TO_VICT);
	return(TRUE);
      }
      act ("$n waves a hand over you, and you find yourself in the clan room.", FALSE, self, 0, ch, TO_VICT);
      act ("$n has been transferred away to $s calling.", FALSE, ch, 0, 0, TO_ROOM);
    } else if (CMD_IS("bribe")) { /* There is nothing money cannot buy. */
      if (CLAN_BRIBING == 0) /* Bribing has been turned off. */
	return(FALSE);
      if ((us != them) && !CLAN_AT_PEACE(us, them)) 
      {
	amount = atoi(argument);
	if (amount < 1) 
	{
	  send_to_char("How much did you want to bribe the guard??\r\n", ch);
	  return(TRUE);
	}
	if (amount > GET_GOLD(ch)) 
	{
	  send_to_char("You don't have that much gold!\r\n", ch);
	  return(TRUE);
	}
	GET_GOLD(ch) -= amount;
	clan_coinage(us, amount);
	/* Modify Relationships.. */
	if (GET_CLAN(ch)) 
	{
	  if (GET_CLAN_RANK(ch) == clan[them].ranks)
	    clan_rel_change(us, them, -25);
	  else
	    clan_rel_change(us, them, -10);
	}
	if (CLAN_BRIBING == 2) 
	{  /* Clan Bribing is in "Bastard" Mode :o)      */
	  act("$n takes the gold and laughs in your face.", FALSE, self, 0, ch, TO_VICT);
	  act("$N laughs hard at $n", FALSE, ch, 0, self, TO_ROOM);
	  return (FALSE);
	}
	switch(GET_LEVEL(self)) 
	{ /* Possible theory on guard strengtehning at the clans cost..
	   * I'm sure we can work something better out later if it's not
	   * good enough as is.. */
	   case 110: amt_req = 2000000000; break;
	   case 100: amt_req = 10000 * GET_LEVEL(ch); break;
	   case 90: amt_req = 5000 * GET_LEVEL(ch); break;
	   case 80: amt_req = 2500 * GET_LEVEL(ch); break;
	   case 70: amt_req = 1000 * GET_LEVEL(ch); break;
	   case 60: amt_req = 100 * GET_LEVEL(ch); break;
	   case 50: amt_req = 0; break;
	   default: amt_req = 0;
	}
	if (amt_req < CLAN_MIN_BRIB) /* Basic attempt at stopping level 10  */
	  amt_req = CLAN_MIN_BRIB;   /* getting in too cheaply              */
	if (CLAN_AT_WAR(us, them))
	  amt_req += (int)(amt_req / 2);
	else if (GET_CLAN_REL(us, them) < CLAN_REL_ANGER)
	  amt_req += (int)(amt_req / 3);
	else if (GET_CLAN_REL(us, them) > CLAN_REL_TRADE)
	  amt_req -= (int)(amt_req / 3);
	if ((amount < amt_req) || (GET_CLAN_REL(us, them) < CLAN_REL_BLOOD_WAR))
	{
	  act("$n takes the gold and laughs in your face.", FALSE, self, 0, ch, TO_VICT);
	  act("$N laughs hard at $n.", FALSE, ch, 0, self, TO_ROOM);
	  return(TRUE);
	}
	if (amount < (amt_req + (amt_req / 4))) /* The in-between range...   */
	  if (number(0, 5) > 0)
	  {    /* Anything higher and the guard will cum */
	    act("$n takes the gold and laughs in your face.", FALSE, 
		self, 0, ch, TO_VICT);
	    act("$N laughs hard at $n.", FALSE, ch, 0, self, TO_ROOM);
	    return(TRUE);
	  }
	act("$n quickly pockets the coin and sneaks you inside.", FALSE, 
	    self, 0, ch, TO_VICT);
	act("$N whistles dixie as $n disappears.", FALSE, ch, 0, self, TO_ROOM);
      } else {
	act("$n looks at the gold, 'Don't worry about it, this one's on me!'", 
	    FALSE, self, 0, ch, TO_VICT);
      } // Same Clan/Relation Check.
    } // Bribe command.
  } // Clan God Test
  char_from_room(ch);
  char_to_room(ch, real_room(GET_MOB_VNUM(self) + MAX_CLANS));
  look_at_room(ch, 0);
  if (amt_req < 1)
    act("$n has arrived in the clan hall.", FALSE, ch, 0, 0, TO_ROOM);
  return(TRUE);
}

/* Sentence - Display current punishments.. */
ACMD (do_sentence)
{
  int i, found=0;

  sprintf(buf, "You are waiting out the following sentences:-\r\n");
  for (i = 0; i < NUM_PUNISHES; i++) {
    if (ch->player_specials->saved.phours[i] == 0)
      continue;
    if (ch->player_specials->saved.phours[i] > 0)
      sprintf(buf, "%s  (%3dhr): %s\r\n", buf, ch->player_specials->saved.phours[i], punish_types[i]);
    else
      sprintf(buf, "%s  (unlim): %s\r\n", buf, punish_types[i]);
    found = 1;
  }
  if (found == 0) {
    send_to_char("You have not been sentenced, you must have been well-behaved.\r\n", ch);
    return;
  }
  send_to_char(buf, ch);
}

/* Punish - Replaces Mute, Freeze and a shitload of others. */
ACMD (do_punish)
{
  char victim[MAX_INPUT_LENGTH];
  char punishment[MAX_INPUT_LENGTH];
  char offence[MAX_INPUT_LENGTH];
  char hours[MAX_INPUT_LENGTH];
  char reason[MAX_INPUT_LENGTH];
  int subc=0,onum=0;
  struct char_data *vict;
  int min_levels[] = {
	  LVL_GRGOD, /* Freeze     */
	  LVL_ANGEL, /* Mute       */
	  LVL_GRGOD, /* Notitle    */
	  LVL_GRGOD, /* Laggy      */
	  LVL_GRGOD, /* Low_Exp    */
	  LVL_GRGOD, /* Low_Regen  */
	  LVL_GRGOD, /* Aggro      */
  };
  char syntax[] = "Syntax: punish <victim> <punishment> <hours> <offence> <reason>\r\n";

  /* Args: <victim> <punishment> <hours> <offence> <reason> */

  if (!*argument) {
    send_to_char(syntax, ch);
    return;
  }
  half_chop(argument, victim, argument);
  if (!*argument) { // list_punishments();
    int i;
    sprintf(buf, "Punishments Available To You: \r\n  ");
    if (LR_FAIL(ch, LVL_ANGEL))
    {
      strcat(buf, "None.\r\n");
      send_to_char(buf, ch);
      return;
    }
    if (GET_LEVEL(ch) == LVL_ANGEL)
    {
      strcat(buf, "Mute.\r\n");
      send_to_char(buf, ch);
      return;
    }
    for (i = 0; i < NUM_PUNISHES; i++)
      sprintf(buf, "%s %s", buf, punish_types[i]);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }
  half_chop(argument, punishment, argument);
  if (!*argument)
  {
    send_to_char("Number of hours not specified. (0 = off, -1 = Permanent).\r\n", ch);
    return;
  }
  half_chop(argument, hours, argument);
  if ((!*argument) && (strcmp(hours, "0")))
  { // list_offences();
    int i;
    sprintf(buf, "Available Offences:  0,");
    for (i = 0; i < NUM_OFFENCES; i++)
      sprintf(buf, "%s %s", buf, offence_types[i]);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }
  half_chop(argument, offence, reason);
  if ((!*reason) && (strcmp(hours, "0")))
  {
    send_to_char(syntax, ch);
    return;
  }

  if (!(vict=generic_find_char(ch, victim, FIND_CHAR_WORLD)))
  {
    send_to_char("Doesn't seem to be anyone by that name around.\r\n", ch);
    return;
  }
  // DM - added level check
  if (GET_LEVEL(ch) <= GET_LEVEL(vict))
  {
    send_to_char("I think not!\r\n", ch);
    return;
  }
  if ((subc=parse_punish(punishment)) < 0)
  {
    send_to_char("There is no such punishment.\r\n", ch);
    return;
  }
  if ((onum=parse_offence(offence)) < -1)
  {
    send_to_char("There is no such offence.\r\n", ch);
    return;
  }
  if ((!is_number(hours)) && (strcmp(hours, "-1")))
  {
    send_to_char("Value of hours must be numeric.\r\n", ch);
    return;
  }
  if ((strlen(reason) < 10) && (strcmp(hours, "0")))
  {
    send_to_char("Reason must be at least 10 characters.\r\n", ch);
    return;
  }

  /* OK, The input looks good.. Lets be bastards. */

  if (LR_FAIL(ch, min_levels[subc]))
  {
    send_to_char("Punishment unavailable.\r\n", ch);
    return;
  }
  if ((min_levels[subc] < 1) && LR_FAIL(ch, LVL_GRGOD))
  {
    send_to_char("Punishment unavailable (Missing min_level).\r\n", ch);
    return;
  }
  if (atoi(hours) > 0)
    sprintf(buf, "(GC) &7%s&n has punished &7%s&n for &1%s&n. (&1%d&nhrs of &1%s&n).",      GET_NAME(ch), GET_NAME(vict),
            (onum >= 0) ? offence_types[onum] : "N/A", atoi(hours),
	    punish_types[subc]);
  else if (atoi(hours) == 0)
    sprintf(buf, "(GC) &7%s&n has removed the &1%s&n punishment on &7%s&n.",
	    GET_NAME(ch), punish_types[subc], GET_NAME(vict));
  else // (atoi(hours) < 0)
    sprintf(buf, "(GC) &7%s&n has punished &7%s&n for &1%s&n. (Indefinite &1%s&n).",        GET_NAME(ch), GET_NAME(vict), 
	    (onum >= 0) ? offence_types[onum] : "N/A", punish_types[subc]);
  mudlog(buf, NRM, MAX(LVL_ANGEL, GET_INVIS_LEV(ch)), TRUE);
  send_to_char("Done.\r\n", ch);
  if (atoi(hours) != 0)
    sprintf(buf, "&RYou have been punished for &1%s&r. Type \"&gsentence&R\" for details.\r\n", (onum >= 0) ? offence_types[onum] : "something");
  else
    sprintf(buf, "&RYou are no longer punished for &1%s&R.\r\n",
	    (onum >= 0) ? offence_types[onum] : "something");
  send_to_char(buf, vict);
  perform_punish(vict, subc, atoi(hours), onum);
}

/* Clan Talk / Leader channels. */
ACMD (do_ctalk)
{
  struct descriptor_data *i;
  int c=0, minlev=1; 
  char level_string[5]="\0\0\0\0";

  skip_spaces (&argument);
  
  if (!LR_FAIL(ch, LVL_CLAN_GOD))
  {
    c = atoi(argument);
    if (find_clan_by_id(c) < 0)
    {
      send_to_char ("That clan doesn't seem to exist..\r\n", ch);
      return;
    }
    while ((*argument != ' ') && (*argument != '\0')) 
      argument++;
    while (*argument == ' ') argument++;
  } else if ((c=GET_CLAN(ch)) == 0 || GET_CLAN_RANK(ch)==0) {
    send_to_char ("You're not part of any clan.\r\n", ch);
    return;
  } 
  if (!CLAN_HASOPT(find_clan_by_id(c), CO_TALK))
  {
    send_to_char ("Clan talking has not been enabled for your clan.\r\n", ch);
    return;
  }
  skip_spaces (&argument);	 

  if (!*argument) 
{
    send_to_char ("Perhaps you should think about what you want to say first..\r\n", ch);
    return;
  }
  
  if ((*argument == '#') && (CLAN_LVL_TALK == 1) && 
      LR_FAIL(ch, LVL_CLAN_GOD))
  {
    argument++;
    minlev = atoi (argument);
    if (minlev > GET_CLAN_RANK(ch))
    {
      send_to_char ("You can't clan talk above your own rank!\r\n", ch);
      return;
    }
    while (*argument != ' ') argument++;
    while (*argument == ' ') argument++;
    if (minlev > 1)
      sprintf(level_string, " (%02d)", minlev);
  }
  if (CLAN_LVL_TALK < 1) 
    minlev = 1;
  
  sprintf (buf1, "You clan talk%s, '%s'\r\n", level_string, argument);
  send_to_char (buf1, ch);

  for (i = descriptor_list; i; i=i->next) {
    if ((i != ch->desc) && (clan_can_hear(i, c))) {
      if (i->character->player_specials->saved.clan_rank >= minlev) {
       sprintf (buf, "%s clan talks%s, '%s'\r\n",
                ((!CAN_SEE(i->character, ch)) ? "someone" : ch->player.name),
		level_string, argument);
	  send_to_char (buf, i->character);
      } else if (!LR_FAIL(ch, LVL_CLAN_GOD)) {
        sprintf (buf, "%s clan talks [%d]%s, '%s'\r\n",
		((!CAN_SEE(i->character, ch)) ? "someone" : ch->player.name),
		GET_CLAN(ch), level_string, argument);
        send_to_char (buf, i->character);
      }
    }
  }
}

/* Clan Leader Talk Channel */
ACMD (do_cltalk) 
{
  struct descriptor_data *i;
  int c=0;

  skip_spaces (&argument);
  
  c=find_clan_by_id(GET_CLAN(ch));
  if ((c < 0) && LR_FAIL(ch, LVL_CLAN_GOD))
  {
    send_to_char("You don't belong to a clan, let alone lead one!\r\n", ch);
    return;
  }
  if (LR_FAIL(ch, LVL_CLAN_GOD) && (GET_CLAN_RANK(ch) < clan[c].ranks))
  {
    send_to_char("Only clan leaders may use this channel.\r\n", ch);
    return;
  }
  if (EXT_FLAGGED(ch, EXT_NOCT))
  {
    send_to_char("You are not currently on that channel.\r\n", ch);
    return;
  }
  if (!*argument)
  {
    send_to_char("Just what did you want to say to the other leaders?\r\n", ch);
    return;
  }
  /* Yeah, I tried using && here, but the problem with that is clan[-1].options
   * doesn't exist =) -- ARTUS */
  if (GET_LEVEL(ch) < LVL_CLAN_GOD)
    if (!CLAN_HASOPT(find_clan_by_id(GET_CLAN(ch)), CO_TALK))
    {
      send_to_char ("Clan talking has not been enabled for your clan.\r\n", ch);
      return;
    }

  sprintf (buf1, "You leader talk, '%s'\r\n", argument);
  send_to_char (buf1, ch);

  for (i = descriptor_list; i; i=i->next) {
    if ((i != ch->desc) && (clan_can_hear(i, -1))) {
      sprintf (buf, "%s leader talks, '%s'\r\n",
               ((!CAN_SEE(i->character, ch)) ? "someone" : ch->player.name), 
	       argument);
      send_to_char (buf, i->character);
    }
  }
}
