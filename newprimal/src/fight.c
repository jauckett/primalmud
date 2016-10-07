/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern int pk_allowed;		/* see config.c */
extern int max_exp_gain;	/* see config.c */
extern int max_exp_loss;	/* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;

/* External procedures */
char *fread_action(FILE * fl, int nr);
SPECIAL(titan);
ACMD(do_flee);
int backstab_mult(int level);
int thaco(struct char_data *ch);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
void make_titan_corpse(struct char_data *ch);  

/* local functions */
void perform_group_gain(struct char_data * ch, int base, struct char_data * victim);
void dam_message(int dam, struct char_data * ch, struct char_data * victim, int w_type);
void appear(struct char_data * ch);
void load_messages(void);
void check_killer(struct char_data * ch, struct char_data * vict);
struct obj_data *make_corpse(struct char_data * ch);
void change_alignment(struct char_data * ch, struct char_data * victim);
void death_cry(struct char_data * ch);
void raw_kill(struct char_data * ch, struct char_data *killer);
void die(struct char_data * ch, struct char_data *killer);
void group_gain(struct char_data * ch, struct char_data * victim);
void solo_gain(struct char_data * ch, struct char_data * victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int compute_armor_class(struct char_data *ch);
int compute_thaco(struct char_data *ch);

/* Weapon attack texts */
// DM - TODO fix the shoot stuff ...
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"},
  {"shot", "shoots"},
  {"ERR", "ERRORS"},
  {"ERR", "ERRORS"},
  {"ERR", "ERRORS"},
  {"ERR", "ERRORS"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"blast", "blast"}     
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void appear(struct char_data * ch)
{
  if( !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_INVIS) )
	return;

  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_ANGEL)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}


int compute_armor_class(struct char_data *ch)
{
  int armorclass = GET_AC(ch);

  if (AWAKE(ch))
    armorclass += dex_app[GET_DEX(ch)].defensive * 10;

  // Bonus to Armour Class if special  is set
  if ( !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_SUPERMAN))
	armorclass -= armorclass * 0.10;

  return (MAX(-100, armorclass));      /* -100 is lowest */
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
    fgets(chk, 128, fl);
  }

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
      fgets(chk, 128, fl);
    }
  }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  char buf[256];

  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;
  if (GET_LEVEL(ch) > LVL_IS_GOD) 
    return;

  SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
  sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
  mudlog(buf, BRF, LVL_ANGEL, TRUE);
  send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!pk_allowed)
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);

  if (MOUNTING(ch) && FIGHTING(MOUNTING(ch)))
	stop_fighting(MOUNTING(ch));
}



struct obj_data *make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;
  corpse->name = str_dup("corpse");

  sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = str_dup(buf2);

  sprintf(buf2, "the corpse of %s", GET_NAME(ch));
  corpse->short_description = str_dup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_COST(corpse) = GET_LEVEL(ch);  
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_obj(unequip_char(ch, i), corpse);

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, ch->in_room);

  return corpse;
}

// DM - TODO - see if ch->affected->type is actually being used

/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  struct affected_type *af, *next;        

  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;

  /* Check alignment restrictions - DM */
  if (affected_by_spell(ch,SPELL_PROT_FROM_GOOD) && !IS_EVIL(ch))
      for (af = ch->affected; af; af = next) {
        next = af->next;
        if ((af->type == SPELL_PROT_FROM_GOOD) && (af->duration > 0)) {
          send_to_char("You no longer feel the need for protection from good beings.\r\n", ch);
          affect_remove(ch,af);
        }
      }
 
  if (affected_by_spell(ch,SPELL_PROT_FROM_EVIL) && !IS_GOOD(ch))
      for (af = ch->affected; af; af = next) {
        next = af->next;
        if ((af->type == SPELL_PROT_FROM_EVIL) && (af->duration > 0)) {
          send_to_char("You no longer feel the need for protection from evil beings.\r\n", ch);
          affect_remove(ch,af);
        }
      }     
}



void death_cry(struct char_data * ch)
{
  int door;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room("Your blood freezes as you hear someone's death cry.\r\n",
		world[ch->in_room].dir_option[door]->to_room);
}



void raw_kill(struct char_data * ch, struct char_data *killer)
{
  struct obj_data *corpse, *obj, *next_obj;
  bool found=FALSE, arms_full=FALSE;
  int amount, num=1, share;
  struct char_data *k;
  struct follow_type *f;
 
  extern struct index_data *mob_index;
 
void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
                                     struct obj_data * cont, int mode);
 stop_fighting(ch);
 
  while (ch->affected)
    affect_remove(ch, ch->affected);
 
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_MORTALK)){
        REMOVE_BIT(PRF_FLAGS(ch), PRF_MORTALK);
        call_magic(ch,ch,NULL,42,GET_LEVEL(ch),CAST_MAGIC_OBJ);
        GET_HIT(ch)= GET_MAX_HIT(ch);
        GET_MANA(ch) = 100;
        GET_POS(ch) = POS_STUNNED;
  } else {
 
  /* remove wolf/vamp infections so ya hafta be reinfected after death */
    if (!IS_NPC(ch))
      REMOVE_BIT(PRF_FLAGS(ch),PRF_WOLF | PRF_VAMPIRE);

    if(GET_HIT(ch) > 0)
      GET_HIT(ch) = 0;
 
    death_cry(ch);
 
    if (GET_MOB_SPEC(ch) == titan)
      make_titan_corpse(ch);    
    else
      corpse = make_corpse(ch);
 
    /* DM - check to see if someone killed ch, or if we dont have a corpse */
    if (!killer || !corpse) {
      extract_char(ch);
      return;
    }
 
  /* DM - autoloot/autogold - automatically loot if ch is NPC
        and they are in the same room */
 
    if ( (!IS_NPC(killer) && IS_NPC(ch)) &&
        (killer->in_room == ch->in_room)) {
 
    /* Auto Loot */
      if (EXT_FLAGGED(killer, EXT_AUTOLOOT)) {
        for (obj = corpse->contains; obj && !arms_full; obj = next_obj) {
          if (IS_CARRYING_N(killer) >= CAN_CARRY_N(killer)) {
            send_to_char("Your arms are already full!\r\n", killer);
            arms_full = TRUE;
            continue;
          }
          next_obj = obj->next_content;
          if (CAN_SEE_OBJ(killer, obj) && (GET_OBJ_TYPE(obj) != ITEM_MONEY)) {
            perform_get_from_container(killer, obj, corpse, 0);
            found = TRUE;
          }
        }
        if (!found)
          act("$p seems to be empty.", FALSE, killer, corpse, 0, TO_CHAR);
      } 
 
    /* Auto Gold */
      if (EXT_FLAGGED(killer, EXT_AUTOGOLD)) {
        for (obj = corpse->contains; obj; obj = next_obj) {
          next_obj = obj->next_content;
          if ((GET_OBJ_TYPE(obj) == ITEM_MONEY) && (GET_OBJ_VAL(obj, 0) > 0)) {
 
            /* Auto Split */
            if (EXT_FLAGGED(killer, EXT_AUTOSPLIT) && IS_AFFECTED(killer, AFF_GROUP)) {
 
              amount = GET_OBJ_VAL(obj, 0);
 
              k = (killer->master ? killer->master : killer);
 
sprintf(buf,"Master: %s, ",GET_NAME(k));
send_to_char(buf,killer);
 
        /* DM - TODO - rewrite autosplit?? */
       /* DM - fix in here when a follower makes kill not in room as others */
 
              for (f = k->followers; f; f = f->next)
                if (IS_AFFECTED(f->follower, AFF_GROUP) && (!IS_NPC(f->follower)) &&
                      /* (f->follower != killer) && */
                      (f->follower->in_room == killer->in_room))
                  num++;
 
sprintf(buf,"Number in Group: %d, ",num);
send_to_char(buf,killer);
 
              if (num == 1)
                perform_get_from_container(killer, obj, corpse, 0);
              else {
                obj_from_obj(obj); 
                share = amount / num;
 
                GET_GOLD(killer) += share;
                sprintf(buf, "You split %s%d%s coins among %d members -- %s%d%s coins each.\r\n",
                        CCGOLD(killer,C_NRM),amount,CCNRM(killer,C_NRM), num,
                        CCGOLD(killer,C_NRM),share,CCNRM(killer,C_NRM));
                        send_to_char(buf, killer);
 
                if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == killer->in_room)
                    && !(IS_NPC(k)) && k != killer) {
                  GET_GOLD(k) += share;
                  sprintf(buf, "%s splits %s%d%s coins; you receive %s%d%s.\r\n", GET_NAME(killer),
                           CCGOLD(k,C_NRM),amount,CCNRM(k,C_NRM),CCGOLD(k,C_NRM),
                           share,CCNRM(k,C_NRM));
                  send_to_char(buf, k);
                }
 
                for (f=k->followers; f;f=f->next) {
                  if (IS_AFFECTED(f->follower, AFF_GROUP) &&
                      (!IS_NPC(f->follower)) &&
                      (f->follower->in_room == killer->in_room) &&
                      f->follower != killer) {
                    GET_GOLD(f->follower) += share;
                    sprintf(buf, "%s splits %s%d%s coins; you receive %s%d%s.\r\n",
                    GET_NAME(killer),CCGOLD(f->follower,C_NRM),amount,CCNRM(f->follower,C_NRM),
                    CCGOLD(f->follower,C_NRM), share,CCNRM(f->follower,C_NRM));
                    send_to_char(buf, f->follower);
                  }
                }
              }
          /* Not grouped or autosplitting */
            } else
              perform_get_from_container(killer, obj, corpse, 0);
          }
        }
      } /* End of Autogold */
    } /* End of Auto loot/gold/split able */
    
    // Dismount
    if (MOUNTING(ch)) 
	MOUNTING(MOUNTING(ch)) = NULL;
    if (MOUNTING_OBJ(ch))
	OBJ_RIDDEN(MOUNTING_OBJ(ch)) = NULL;

    MOUNTING(ch) = NULL;
    MOUNTING_OBJ(ch) = NULL;

    extract_char(ch);
  } 

}



void die(struct char_data * ch, struct char_data *killer)
{

  /* bm changed exp lost to  level minimum and 10% of carried gold*/
  if (!IS_NPC(ch) && PRF_FLAGGED(ch,PRF_MORTALK))
    REMOVE_BIT(PRF_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  else{
  /* DM_exp : If exp is not negative (which it shouldn't be ie level min = 0)
                use (exp this level)/2 */
    if (GET_EXP(ch) > 0)
      gain_exp(ch, -(GET_EXP(ch)/2));
    else
      GET_EXP(ch) = 0;
//      gain_exp(ch,-(level_exp(GET_CLASS(ch),GET_LEVEL(ch))*0.1));

    GET_GOLD(ch)*=0.9;
    if (!IS_NPC(ch))
      REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  }
  raw_kill(ch,killer);

  if (killer)
    GET_WAIT_STATE(killer) = 0;
}



void perform_group_gain(struct char_data * ch, int base,
			     struct char_data * victim)
{
  int share;

  share = MIN(max_exp_gain, MAX(1, base));

  if (PRF_FLAGGED(ch, PRF_MORTALK))
        return;
  if (PLR_FLAGGED(victim, PLR_KILLER))
        return;  

  if (share > 1) {
    sprintf(buf2, "You receive your share of experience -- &5%d&n points.\r\n", share);
    send_to_char(buf2, ch);
  } else
    send_to_char("You receive your share of experience -- one measly little point!\r\n", ch);

  gain_exp(ch, share);
  change_alignment(ch, victim);
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base, tot_gain, exp;
  struct char_data *k;
  struct follow_type *f;
  int min_level = LVL_IMPL, max_level = 0, group_level, av_group_level, percent; 

  if (!(k = ch->master))
    k = ch;

  if (PRF_FLAGGED(ch, PRF_MORTALK)){
     stop_fighting(ch);
     stop_fighting(victim);
     send_to_char("You are the SUPREME warrior of the MOrtal Kombat arena!\r\n", ch);
     return;
  }
  if (PLR_FLAGGED(victim, PLR_KILLER)){
           send_to_char("You recieve No EXP for killing players with the KILLER flag\r\n", ch);
           return;
   }
 
  group_level = GET_LEVEL(k);
  max_level = min_level = MAX(max_level, GET_LEVEL(k));

  if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)

    /* DM - Dont include CLONES if they are grouped */
    if (!IS_CLONE(f->follower))
    {
      tot_members++;
      group_level += GET_LEVEL(f->follower);
      min_level = MIN(min_level, GET_LEVEL(f->follower));
      max_level = MAX(max_level, GET_LEVEL(f->follower));
    }   

/* cap it to LVL_IMPL */
 group_level = MIN(LVL_IMPL, group_level - tot_members);  

  /* round up to the next highest tot_members */
  base = (GET_EXP(victim) / 3) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    base = MIN(max_exp_loss * 2 / 3, tot_gain);

//  if (tot_members >= 1)
//    base = MAX(1, tot_gain / tot_members);
//  else
//    base = 0;

/* DM group gain percentage */
  if (tot_members >= 1) {
    /* base = MAX(1, GET_EXP(victim) / 3) / tot_members; */
    if (tot_members == 2)
        base=(GET_EXP(victim)*80)/100;
    else if (tot_members == 3)
        base=(GET_EXP(victim)*85)/100;
    else if (tot_members == 4)
        base=(GET_EXP(victim)*90)/100;
    else if (tot_members == 5)
        base=(GET_EXP(victim)*70)/100;
    else if (tot_members == 6)
        base=(GET_EXP(victim)*65)/100;
    else if (tot_members == 7)
        base=(GET_EXP(victim)*55)/100;
    else if (tot_members == 8)
        base=(GET_EXP(victim)*20)/100;
    else
      base=0;
  } else
    base = 0;  


//  if (AFF_FLAGGED(k, AFF_GROUP) && k->in_room == ch->in_room)
//    perform_group_gain(k, base, victim);

  if (IS_AFFECTED(k, AFF_GROUP) && k->in_room == ch->in_room)
  {
      if (GET_LEVEL(victim) < av_group_level)
      {
        base = ((GET_EXP(victim) / 3) + tot_members - 1)/tot_members;
        percent = MAX(10, 100 - ((GET_LEVEL(ch) - GET_LEVEL(victim)) * 10));
        exp = (base * percent) / 100;
        sprintf(buf2, "Your opponent was out of your group's league! You don't learn as much.\n\r");
        send_to_char(buf2, k);
      }
      else
        exp = base;
 
      if (((max_level - min_level) > 10) && (tot_members > 1))
      {
        if (exp > (((GET_EXP(victim) / 3) + tot_members - 1)/tot_members))
          exp = base = ((GET_EXP(victim) / 3) + tot_members - 1)/tot_members;
        sprintf(buf2, "The group is unbalanced! You learn much less.\n\r");
        send_to_char(buf2, k);
        exp = (exp * GET_LEVEL(k)) / group_level;
      }
      perform_group_gain(k, exp, victim);
   }                                       

//  for (f = k->followers; f; f = f->next)
//    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
//      perform_group_gain(f->follower, base, victim);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
    {
    /* DM - Dont include CLONES if they are grouped */
    if (!IS_CLONE(f->follower)) {
 
      if (GET_LEVEL(victim) < av_group_level)
      {
        percent = MAX(10, 100 - ((GET_LEVEL(ch) - GET_LEVEL(victim)) * 10));
        exp = (base * percent) / 100;
        sprintf(buf2, "Your opponent was out of your group's league! You don't learn as much.\n\r");
        send_to_char(buf2, f->follower);
      }
      else
        exp = base;
      if ((max_level - min_level) > 10)
      {
        sprintf(buf2, "The group is unbalanced! You learn much less.\n\r");
        send_to_char(buf2, f->follower);
        exp = (exp * GET_LEVEL(f->follower)) / (group_level*2);
      }
      perform_group_gain(f->follower, exp, victim);
    }
  }           
}


void solo_gain(struct char_data * ch, struct char_data * victim)
{
  int exp, percent;

  exp = MIN(max_exp_gain, GET_EXP(victim) / 3);
 
  /* Calculate level-difference bonus */
  if (IS_NPC(ch) && !IS_CLONE(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
  else {
//          exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
 
/* JA new level difference code */
    if (GET_LEVEL(victim) < GET_LEVEL(ch)) {
      percent = MAX(10, 100 - ((GET_LEVEL(ch) - GET_LEVEL(victim)) * 10));
      percent = MIN(percent, 100);
      exp = (exp * percent) / 50;
      if (percent <= 60) {
        sprintf(buf2, "Your opponent was out of your league! You don't learn as much.\n\r");
        if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
          send_to_char(buf2, ch->master);
        else
          send_to_char(buf2, ch);
      }
    }
  }
 
/* this is to stop people abusing the no exp loss below your level code
 * and it adds a punishment for killing in a non-pk zone - Vader
 */
#ifdef JOHN_DISABLE
  if(GET_EXP(victim) <= level_exp(ch,GET_LEVEL(victim)) + 10000 ||
     !IS_SET(zone_table[world[ch->in_room].zone].zflag,ZN_PK_ALLOWED))
    exp = -(GET_LEVEL(victim) * 10000);
  else
#endif
    exp = MAX(exp, 1);    

  // DM - TODO - check player specials in here ...
  if (PLR_FLAGGED(victim, PLR_KILLER)){
    if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
      send_to_char("You recieve No EXP for killing players with the KILLER flag!.\r\n", ch->master);
    else
      send_to_char("You recieve No EXP for killing players with the KILLER flag!.\r\n", ch);
    exp = 0;
  } else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_MORTALK)) {
    if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
      send_to_char("You are the SUPREME winner of Mortal Kombat!!\r\n", ch->master);
    else
      send_to_char("You are the SUPREME winner of Mortal Kombat!!\r\n", ch);
    exp = 0;
  } else if (exp > 1) {
    if (IS_CLONE(ch) && IS_CLONE_ROOM(ch)) {
      sprintf(buf2, "You receive %s%d%s experience points.\r\n",
                CCEXP(ch->master,C_NRM),exp,CCNRM(ch->master,C_NRM));
      send_to_char(buf2, ch->master);
    } else {
      sprintf(buf2, "You receive %s%d%s experience points.\r\n",
                CCEXP(ch,C_NRM),exp,CCNRM(ch,C_NRM));
      send_to_char(buf2, ch);
    }
  } else 
    if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
      send_to_char("You receive one lousy experience point.\r\n", ch->master);
    else
      send_to_char("You receive one lousy experience point.\r\n", ch);
 
  /* DM - dont let clones steal exp */
  if (IS_CLONE_ROOM(ch)) {
    gain_exp(ch->master, exp);
    change_alignment(ch->master, victim);
  } else {
    gain_exp(ch, exp);
    if (!IS_CLONE(ch))
      change_alignment(ch, victim); 
  }
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim,
		      int w_type)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {
 
    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */
 
    {
      "$n tries to #w $N, but misses.", /* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },
 
    {
      "$n tickles $N as $e #W $M.",     /* 1: 1..2  */
      "You tickle $N as you #w $M.",
      "$n tickles you as $e #W you."
    },
 
    {
      "$n barely #W $N.",               /* 2: 3..4  */
      "You barely #w $N.",
      "$n barely #W you."
    },
 
    {
      "$n #W $N.",                      /* 3: 5..6  */
      "You #w $N.",
      "$n #W you."
    },
 
    {
      "$n #W $N hard.",                 /* 4: 7..10  */
      "You #w $N hard.",
      "$n #W you hard."
    },
 
    {
      "$n #W $N very hard.",            /* 5: 11..14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },
 
    {
      "$n #W $N extremely hard.",       /* 6: 15..19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },
 
    {
      "$n causes internal BLEEDING on $N with $s #w!!", /* 8: 23..30   */
      "You cause internal BLEEDING on $N  with your #w!!",
      "$n causes internal BLEEDING on you with $s #w!!"
    },
 
    {
      "$n severely injures $N with $s #w!!",    /* 8: 23..30   */
      "You severely injure $N with your #w!!",
      "$n severely injures you with $s #w!!"
    },
 
    {
      "$n wipes $N out with $s deadly #w!!",    /* 8: 23..30   */
      "You wipe out $N with your deadly #w!!",
      "$n wipes you with $s #w!!"
    },
 
    {
      "$n massacres $N to small fragments with $s #w.", /* 7: 19..23 */
      "You massacre $N to small fragments with your #w.",
      "$n massacres you to small fragments with $s #w."
    },
 
 
    {
      "$n SHATTERS $N with $s deadly #w!!",     /* 8: 23..30   */
      "You SHATTER $N with your deadly #w!!",
      "$n SHATTERS you with $s deadly #w!!"
    },
 
    {    
      "$n OBLITERATES $N with $s deadly #w!!",  /* 8: 23..30   */
      "You OBLITERATE $N with your deadly #w!!",
      "$n OBLITERATES you with $s deadly #w!!"
    },
 
    {
      "$n DESTROYS $N with $s deadly #w!!",     /* 9: > 30   */
      "You DESTROY $N with your deadly #w!!",
      "$n DESTROYS you with $s deadly #w!!"
    },
    {
      "$n FUBARS $N with $s deadly #w!!",
      "You FUBAR $N with your deadly #w!!",
      "$n FUBARS you with $s deadly #w!!"
    },
    {
      "$n NUKES $N with $s deadly #w!!",
      "You NUKE $N with your deadly #w!!",
      "$n NUKES you with $s deadly #w!!"
    },
    {
      "$n does UNSPEAKABLE things to $N with $s deadly #w!!",
      "You do UNSPEAKABLE things to $N with your deadly #w!!",
      "$n does UNSPEAKABLE things to you with $s deadly #w!!"
    }
  };                                             

  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 8)    msgnum = 2;
  else if (dam <= 14)   msgnum = 3;
  else if (dam <= 20)   msgnum = 4;
  else if (dam <= 25)   msgnum = 5;
  else if (dam <= 30)   msgnum = 6;
  else if (dam <= 35)   msgnum = 7;
  else if (dam <= 40)   msgnum = 8;
  else if (dam <= 45)   msgnum = 9;
  else if (dam <= 50)   msgnum = 10;
  else if (dam <= 60)   msgnum = 11;
  else if (dam <= 70)   msgnum = 12;
  else if (dam <= 80)   msgnum = 13;
  else if (dam <= 90)   msgnum = 14;
  else if (dam <=100)   msgnum = 15;
  else                  msgnum = 16;  

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  send_to_char(CCYEL(ch, C_CMP), ch);
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(CCNRM(ch, C_CMP), ch);

  /* damage message to damagee */
  send_to_char(CCRED(victim, C_CMP), victim);
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(victim, C_CMP), victim);
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data * ch, struct char_data * vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;
  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_ANGEL)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
	if (GET_POS(vict) == POS_DEAD) {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
	send_to_char(CCYEL(ch, C_CMP), ch);
	act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	send_to_char(CCNRM(ch, C_CMP), ch);

	send_to_char(CCRED(vict, C_CMP), vict);
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(vict, C_CMP), vict);

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}

/* Damages a random piece of equipment on the target */
void damage_equipment(struct char_data *ch) {

	int i, found = 0;
	struct obj_data *obj;

	for (i = 0; i < NUM_WEARS && !found; i++)
	{
	   if (GET_EQ(ch, i)) 
           {
		if (number(1, NUM_WEARS) == NUM_WEARS) 
		{
                   if (GET_OBJ_MAX_DAMAGE(GET_EQ(ch, i)) == -1)
			continue;

		   GET_OBJ_DAMAGE(GET_EQ(ch, i)) -= number(1, 5);
		   found = 1;
		   break;
 		}
	   }
	}

	if(found)
	{
	   if (GET_OBJ_DAMAGE(GET_EQ(ch, i)) <= 0 && GET_OBJ_MAX_DAMAGE(GET_EQ(ch, i)) != -1) 
	   {
		GET_OBJ_DAMAGE(GET_EQ(ch,i)) = 0;
		sprintf(buf1, "&rYour &R%s&r just broke!&n\r\n", GET_EQ(ch,i)->short_description);
	   	send_to_char(buf1, ch);
	   }
	}
	else {	// nothing was damage, slight chance of getting inventory
		for(obj = ch->carrying; obj; obj = obj->next_content)
		{
		
		}
	} 
	

}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data * ch, struct char_data * victim, int dam, int attacktype)
{
  int manastolen = 0, damChance = 0; 

  if (GET_POS(victim) <= POS_DEAD) {
    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim,ch);
    return (0);			/* -je, 7/7/92 */
  }

  if (IS_NPC(victim))
    if(MOB_FLAGGED(victim,MOB_QUEST))
    {
      send_to_char("Sorry, they are part of a quest.\r\n",ch);
      return;
    } 

  /* peaceful rooms */
  if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char(PEACEROOM, ch);
    GET_WAIT_STATE(ch) = 0;
    return (0);
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return (0);

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_ANGEL))
    dam = 0;

  /* Can't attack your own mounts */
  if (victim == MOUNTING(ch)) {
	send_to_char("Better get off your mount before you attack it.\r\n", ch);
	return (0);
  }

  /* Mounts will not attack their riders */ // - May change this to make them buck instead
  if (victim == MOUNTING(ch) && IS_NPC(ch)) {
	send_to_char("Your rider controls your urge to kill them.\r\n", ch);
	return (0);
  }	

  
  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL)) {
      set_fighting(ch, victim);

      // DM - TODO - wtf is this??
      // -Tal- I dont know, I didnt put it in   
      // ch and victim are mobs, victim has a master, 1/10 times if vict charmed and vict's master is
      // in same room as ch, then stop ch fighting, and make ch fight the victims master
      if (IS_NPC(ch) && IS_NPC(victim) && victim->master &&
          !number(0, 10) && IS_AFFECTED(victim, AFF_CHARM) &&
          (victim->master->in_room == ch->in_room)) {
        if (FIGHTING(ch))
          stop_fighting(ch);
        hit(ch, victim->master, TYPE_UNDEFINED);
        return;
      }    
    }

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);

      // stay down u little bitch
      if (dam > 0 && attacktype == SKILL_TRIP)
        GET_POS(victim) = POS_SITTING;

      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  if (AUTOASSIST(victim) == ch)
    stop_assisting(victim);

  /* If the attacker is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;

  /* Check for PK if this is not a PK MUD */
  if (!pk_allowed) {
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 1000), 0);
  GET_HIT(victim) -= dam;

// DM - Fix getting all SYSERRS now cause MOBS are using PRF_FLAGGED stuff
  /* Gain exp for the hit */
  if (ch != victim)
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_MORTALK))  
      gain_exp(ch, GET_LEVEL(victim) * dam);
    else
      gain_exp(ch, GET_LEVEL(victim) * dam);

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (!IS_WEAPON(attacktype)) {
    skill_message(dam, ch, victim, attacktype);
  } else {
    if (GET_POS(victim) == POS_DEAD || dam == 0) {
      if (!skill_message(dam, ch, victim, attacktype)) {
	dam_message(dam, ch, victim, attacktype);
      }
    } else {
      dam_message(dam, ch, victim, attacktype);
    }
  }

// DM - do we need a check in here for 2nd/3rd attack - when calling damage(ch,vict,0,2/3attack)?? 
// This seems to elimate the "extra message" shown when the 2nd or 3rd attack isn't called.
 
if (!((dam == 0) && (attacktype == SKILL_2ND_ATTACK || attacktype == SKILL_3RD_ATTACK)))

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", victim);
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", victim);
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You're stunned, but will probably regain consciousness again.\r\n", victim);
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("You are dead!  Sorry...\r\n", victim);
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char("That really did HURT!\r\n", victim);

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
      sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
	      CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      send_to_char(buf2, victim);
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, NULL, 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
      send_to_char("You wimp out, and attempt to flee!\r\n", victim);
      do_flee(victim, NULL, 0, 0);
    }
    break;
  }

  /* 
   *By this stage,  they're having a swing, damage eq regardless. 
   * Random chance of damaging EQ
   */
  damChance = 100 - (GET_LEVEL(ch) - GET_LEVEL(victim)) - GET_STR(ch);
  if (damChance < 0)
	damChance = 1;
  if (number(0, damChance) == damChance) 	// Damaged eq! 
	damage_equipment(victim);

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = victim->in_room;
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
  if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {

   /* fix for poisoned players getting exp, and their group! bm */
      if (attacktype==SPELL_POISON || attacktype==TYPE_SUFFERING)
      {
        sprintf(buf2, "%s died at %s", GET_NAME(ch),world[victim->in_room].name);
        mudlog(buf2, BRF, LVL_ETRNL1, TRUE);
        if (MOB_FLAGGED(ch, MOB_MEMORY))
          forget(ch, victim);
        die(victim,ch);
        return;  
      }

    if ((ch != victim) && (IS_NPC(victim) || victim->desc)) {
      /* DM - if CLONE makes kill in group - only give exp to CLONES master */
      if (AFF_FLAGGED(ch, AFF_GROUP) && !IS_CLONE(ch))
        group_gain(ch, victim);
      else if (MOUNTING(ch) && IS_NPC(ch))
        solo_gain(MOUNTING(ch), victim);	// XP to rider
      else
        solo_gain(ch, victim);
    }
    if (!IS_NPC(victim)) {
      sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
	      world[victim->in_room].name);
      mudlog(buf2, BRF, LVL_ANGEL, TRUE);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    die(victim,ch);
    return (-1);
  }
  // Mana stealing for mana thieves (spellswords)
  if( (dam > 0) && !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_MANA_THIEF) ) 
  {
        // Some semi random chance of draining
        // Current mana / (level * modifier) -- Becomes more likely when mana is lower
        if( number(0, (int)(GET_MANA(ch)/(GET_LEVEL(ch) * GET_MODIFIER(ch))) ) !=0)
                return (dam);
 
        if( GET_MANA(ch) < GET_MAX_MANA(ch) )
        {
          if( !IS_NPC(victim) )
                manastolen += number(1, (int)(GET_MAX_MANA(victim) * 0.05));
          else
                manastolen += number(1, (int)(GET_MAX_MANA(ch) * 0.05));
 
          if( manastolen > GET_MANA(victim) && !IS_NPC(victim) )
                manastolen = GET_MANA(victim);
          if( !IS_NPC(victim) )
               GET_MANA(victim) -= manastolen;
          GET_MANA(ch) += manastolen;
          if( GET_MANA(ch) > GET_MAX_MANA(ch) )
                GET_MANA(ch) = GET_MAX_MANA(ch);
          if( manastolen > 0 )
          {
             act("You manage to steal some mana from $N as you hit $M.", FALSE,
                ch, 0, victim, TO_CHAR);
             act("$n stole some of your mana!", FALSE, ch, 0, victim, TO_VICT);
          }
        }
  }

  // Extra damage for those with SPECIAL_FOREST_HELP
  if ( (dam > 0 ) && !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_FOREST_HELP) 
	&& (SECT(ch->in_room) == SECT_FOREST))
  {
        // Semi random chance of aid
        // Current hp / (level * modifier) -- Becomes more likely as druid is dying 
        if( number(0, (int)(GET_HIT(ch)/(GET_LEVEL(ch) * GET_MODIFIER(ch))) ) !=0)
                return (dam);
 
        if (GET_CLASS(victim) == CLASS_DRUID )
        {
           act("Your forest allies are unable to harm another druid.",
                        FALSE, ch, 0, 0, TO_CHAR);
           act("You discourage $n's forest allies from attacking you.",
                        FALSE, ch, 0, victim, TO_VICT);
           return (dam);
        }
        act("Your forest allies come to your aid, briefly attacking $N.",
                FALSE, ch, 0, victim, TO_CHAR);
        act("$n's forest allies aid $s attack on $N.", FALSE, ch, 0, victim, TO_ROOM);
        act("$n's forest allies attack you briefly, aiding $m.",
                FALSE, ch, 0, victim, TO_VICT);
        dam += number(1, (int)(GET_LEVEL(ch) * GET_MODIFIER(ch)
                            * (GET_WIS(ch) < 15 ? 1 : GET_WIS(ch) < 19 ? 2 : 3) ) );
  }
 
  // Chance for outright destruction of undead for paladins
  if( (dam > 0) && !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_HOLY) 
	&& (GET_CLASS(victim) == CLASS_UNDEAD) )
  {
	if( number(1, GET_HIT(ch) / (GET_MODIFIER(ch) 
	                    * (GET_WIS(ch) < 15 ? 1 : GET_WIS(ch) < 19 ? 2 : 3) ) ) != 1)
		return (dam);
	act("&WThe holy powers within you arise and slay $N as you strike at $m.&n",
		FALSE, ch, 0, 0, TO_CHAR);
	act("&WOverwhelmed by $n's holy power, $N is banished from this realm!&n",
		FALSE, ch, 0, victim, TO_ROOM);
	act("&R$n's holy power overwhelms you, banishing you to the nether realms!&n",
		FALSE, ch, 0, victim, TO_VICT); 
	raw_kill(victim, ch);
	return (-1); // deaaaaaaaaad
  }

  // Damage enhancement for dwarves and elves
  if ( !IS_NPC(ch) && ((IS_SET(GET_SPECIALS(ch), SPECIAL_ELF) 
        && (SECT(ch->in_room) == SECT_FOREST))
   ||  (IS_SET(GET_SPECIALS(ch), SPECIAL_DWARF) 
        && (SECT(ch->in_room) == SECT_INSIDE))) )
     dam  = (int)(dam * 1.10); 	

  // Extra damage for those with SPECIAL_GORE
  if ( (dam > 0 ) && !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_GORE) )
  {
	// Semi random chance of aid
	// Current hp / (level * modifier) -- Becomes more likely as character is dying
	if( number(0, (int)(GET_HIT(ch)/(GET_LEVEL(ch) * GET_MODIFIER(ch))) ) !=0)
		return (dam);	

        if (GET_CLASS(victim) == CLASS_DRUID )
	{
	   act("Your forest allies are unable to harm another druid.",
			FALSE, ch, 0, 0, TO_CHAR);
	   act("You discourage $n's forest allies from attacking you.",
			FALSE, ch, 0, victim, TO_VICT);
	   return (dam);
	}
	act("You viciously lay about with your horns, wounding $N.",
		FALSE, ch, 0, victim, TO_CHAR);
	act("$n's horns gore $N.", FALSE, ch, 0, victim, TO_ROOM);
	act("$n's horns dig deeply into you.",
		FALSE, ch, 0, victim, TO_VICT);
	dam += number(1, (int)(GET_LEVEL(ch) * GET_MODIFIER(ch) 
	                    * (GET_STR(ch) < 15 ? 1 : GET_STR(ch) < 19 ? 2 : 3) ) ); 
  }

  return (dam);
}



void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;
  struct char_data *real_vict = victim;    
  struct char_data *char_room; 

  int attacktype;
  int reloadable;   

/* JA code to randomly select a player to hit */
/* to stop the tank from getting hit all the time */
 
  /* pseudo-randomly choose someone in the room who is fighting me */
  if (IS_NPC(ch))
    for (real_vict = world[ch->in_room].people; real_vict; real_vict = real_vict->next_in_room)
      if (FIGHTING(real_vict) == ch && !number(0, 3))
        break;
 
  if (real_vict == NULL)
    real_vict = victim;
        assert(real_vict);  

  /* Do some sanity checking, in case someone flees, etc. */
  if (ch->in_room != victim->in_room) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  /* Find the weapon type (for display purposes only) */
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

/* JA ------------------------- ammo code  */
//  w_type = attacktype & 0x7fff;
//  reloadable = attacktype & 0x8000;
 
/* changed so mobs dont use ammo - Vader */
//  if (reloadable && !IS_NPC(ch))
//  {
//    if (GET_OBJ_VAL(wielded, 0) <= 0)
//    {
//      send_to_char("*CLICK*\n\r", ch);
//      return;
//       /* out of ammo */
//    }
//    else
//      MAX(0, --GET_OBJ_VAL(wielded, 0));
//  }      


  calc_thaco = thaco(ch) - ch->points.hitroll; 

  // DM - TODO - decide whether we want this armor system here...

  /* Calculate the raw armor including magic armor.  Lower AC is better. */
  victim_ac = compute_armor_class(victim) / 10;

  /* roll the die and take your chances... */
  diceroll = number(1, 20);

  /* decide whether this is a hit or a miss */
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    /* the attacker missed the victim */

    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, SKILL_BACKSTAB);
    else if (type == SKILL_2ND_ATTACK || type == SKILL_3RD_ATTACK)
      damage(ch, real_vict, 0, type);  /* thisll make it use the rite messages for missing */
    else
      damage(ch, real_vict, 0, w_type);

  } else {
    /* okay, we know the guy has been hit.  now calculate damage. */

    /* Start with the damage bonuses: the damroll and strength apply */
    dam = str_app[STRENGTH_REAL_APPLY_INDEX(ch)].todam;
    dam += GET_DAMROLL(ch);

    if ( !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch) , SPECIAL_MINOTAUR))
	dam += (int)((GET_DAMROLL(ch) * 0.05) + 1);

    if (!IS_NPC(ch) && IS_SET(GET_SPECIALS(ch) , SPECIAL_SUPERMAN))
	dam += (int)((GET_DAMROLL(ch) * 0.02) + 1);

    /* Maybe holding arrow? */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      /* Add weapon-based damage if a weapon is being wielded */
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
    } else {
      /* If no weapon, add bare hand damage instead */
      if (IS_NPC(ch)) {
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      } else {
	dam += number(0, 2);	/* Max 2 bare hand damage for players */
      }
    }

    /*
     * Include a damage multiplier if victim isn't ready to fight:
     *
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     *
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants.
     */
    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

    /* Check for protection from evil/good on victim - DM */
    if (((IS_GOOD(ch) && affected_by_spell(real_vict,SPELL_PROT_FROM_GOOD)) ||
        (IS_EVIL(ch) && affected_by_spell(real_vict,SPELL_PROT_FROM_EVIL))) &&
        (GET_LEVEL(ch) >= PROTECT_LEVEL) ) {
 
      dam -= ( GET_LEVEL(real_vict) / MAX(2,num_attacks(real_vict)) );
    } 

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);
    if(type == SKILL_2ND_ATTACK && dam > 1)
      dam *= 0.666; /* 2 3rds damage on second attack */
    if(type == SKILL_3RD_ATTACK && dam > 1)
      dam *= 0.333; /* 1/3 damage if on 3rd attack */

    if (type == SKILL_BACKSTAB) {
      dam *= (1+GET_LEVEL(ch)/10);
      damage(ch, real_vict, dam, SKILL_BACKSTAB);
    } else
      damage(ch, real_vict, dam, w_type);
    
    /* if they have reflect then reflect */
    if(IS_AFFECTED(real_vict,AFF_REFLECT) && dam > 0 && GET_POS(real_vict)> POS_MORTALLYW)
      damage(real_vict,ch,MIN(dam,MAX(GET_HIT(ch) - 1,0)),SPELL_SERPENT_SKIN);
 
/* offensive/defensive spells - Lighting/Fire Shield, Fire Wall */
    if (affected_by_spell(real_vict,SPELL_LIGHT_SHIELD))
      if (mag_savingthrow(real_vict,SAVING_SPELL))
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/4,MAX(GET_HIT(ch)-1,0)),SPELL_LIGHT_SHIELD);
    if (affected_by_spell(real_vict,SPELL_FIRE_SHIELD))
      if (mag_savingthrow(real_vict,SAVING_SPELL))
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/3,MAX(GET_HIT(ch)-1,0)),SPELL_FIRE_SHIELD);
    if (affected_by_spell(real_vict,SPELL_FIRE_WALL))
      if (mag_savingthrow(real_vict,SAVING_SPELL))
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/2,MAX(GET_HIT(ch)-1,0)),SPELL_FIRE_WALL);
 
/* if its a magic weapon decided whether to cast or not then do it - Vader */
    if(wielded && FIGHTING(ch))
      if(IS_OBJ_STAT(wielded,ITEM_MAGIC) && (GET_OBJ_VAL(wielded,0) > 0))
        if(number(0,3))
          call_magic(ch,real_vict,NULL,GET_OBJ_VAL(wielded,0),2*GET_LEVEL(ch),CAST_MAGIC_OBJ); 

  }

  /* DM - autoassist check */
  if (GET_POS(victim) == POS_FIGHTING)
    for (char_room=world[ch->in_room].people;char_room;char_room=char_room->next_in_room) {
      if (!IS_NPC(char_room))
        if (AUTOASSIST(char_room) == ch)
 
          /* Ensure all in room and vict fighting
          if ((world[ch->in_room].number == world[victim->in_room].number) &&
              (world[ch->in_room].number == world[char_room->in_room].number) &&
              (GET_POS(victim) = POS_FIGHTING)) */
 
          if (IS_NPC(victim) && (GET_POS(victim) == POS_FIGHTING) &&
            (world[char_room->in_room].number == world[victim->in_room].number) &&
            (!FIGHTING(char_room)) && CAN_SEE(char_room,ch) && CAN_SEE(char_room,victim)) {
 
            act("$n assists $N!", FALSE, char_room, 0, ch, TO_NOTVICT);
            act("$N assists you!", FALSE, ch, 0, char_room, TO_CHAR);
            sprintf(buf,"You assist %s!\n\r",GET_NAME(ch));
            send_to_char(buf,char_room);
            hit(char_room,victim,TYPE_UNDEFINED);
          }
    }       

    // If they're disguised, their disguise is broken
    // Tali.. Not sure if I should put this in. Should be okay without it.
/*    if( !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_DISGUISE) && CHAR_DISGUISED(ch)){
	send_to_char("You can no longer maintain your disguise!\r\n", ch);
	CHAR_DISGUISED(ch) = 0;
    }
    if( !IS_NPC(ch) && IS_SET(GET_SPECIALS(victim), SPECIAL_DISGUISE) && CHAR_DISGUISED(victim) ) {
	send_to_char("You can no longer maintain your disguise!\r\n",victim);
	CHAR_DISGUISED(victim) = 0;
    } */
}


void perform_mount_violence(struct char_data *rider) {

	/* TODO: Extend function here to allow specials on items and such */

	// Check if the opponent is history
	if (!FIGHTING(rider))
		return;

        if (MOUNTING(rider) && !FIGHTING(MOUNTING(rider)) ) {
	    send_to_char("Your mount joins the fray!\r\n", rider);
	    do_hit(MOUNTING(rider), (FIGHTING(rider))->player.name, 0, SCMD_HIT);
        }
	if (MOUNTING_OBJ(rider)) {
	   send_to_char("Your mount aids your efforts in battle.\r\n", rider);
	   act("$n's mount aids $m against $s opponent.", FALSE, rider, 0, 0, TO_ROOM);
	   hit(rider, FIGHTING(rider), TYPE_UNDEFINED);
        }
}


/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  int loop=1, second=0, third=0, i;
  struct char_data *ch;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
      stop_fighting(ch);
      continue;
    } else {
      if (affected_by_spell(ch, SPELL_HASTE)) {
	loop = 2;
      }

// DM - TODO - fix haste - first glace it works properly now ...
// DM - TODO - although I did get it hanging for an IMP - it
// DM - TODO - still may get stuck in here or some crap - look into it ...

      for (i=0;i<loop;i++) {
	if (i == 1)
	  if (FIGHTING(ch))	
            send_to_char("You are hastened and get more attacks\r\n", ch);
      	
	if (FIGHTING(ch))	
	  hit(ch, FIGHTING(ch), TYPE_UNDEFINED);

  	/* the below bit is added to allow multiple attacks per turn - VADER */
	if(IS_NPC(ch)) 
	  second = third = 100;  			
	else {
	  second = GET_SKILL(ch, SKILL_2ND_ATTACK);
	  third  = GET_SKILL(ch, SKILL_3RD_ATTACK);
	}  

	if(((!IS_NPC(ch) && second) || (IS_NPC(ch) && MOB_FLAGGED(ch,MOB_2ND_ATTACK))) && FIGHTING(ch))
	  if((second + dice(3,(ch->aff_abils.dex))) > number(5,175)) {
	    hit(ch, FIGHTING(ch), SKILL_2ND_ATTACK);
	  } else { 
	    damage(ch, FIGHTING(ch), 0, SKILL_2ND_ATTACK);
          }

	if(((!IS_NPC(ch) && third) || (IS_NPC(ch) && MOB_FLAGGED(ch,MOB_3RD_ATTACK))) && FIGHTING(ch))
	  if((third  + dice(3,(ch->aff_abils.dex))) > number(5,200)) {
	    hit(ch, FIGHTING(ch), SKILL_3RD_ATTACK);
	  } else { 
	    damage(ch, FIGHTING(ch), 0, SKILL_3RD_ATTACK);
          }
	/* vampires and wolfs get an extra attack when changed - Vader */
	if(affected_by_spell(ch,SPELL_CHANGED) && (number(0,4) == 0) && FIGHTING(ch))
	  if(PRF_FLAGGED(ch,PRF_WOLF))
	    damage(ch,FIGHTING(ch),dice(4,GET_LEVEL(ch)),TYPE_CLAW);
	  else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
	    damage(ch,FIGHTING(ch),dice(4,GET_LEVEL(ch)),TYPE_BITE);
      }
    }


    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
        GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
        continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
        GET_POS(ch) = POS_FIGHTING;
        act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You scramble to your feet\r\n", ch);
      }
    } 

    // if (GET_POS(ch) < POS_FIGHTING) {
    //  send_to_char("You can't fight while sitting!!\r\n", ch);
    //  continue;
    //}

   /* Mounts add their own special attack */
   if (MOUNTING(ch) || MOUNTING_OBJ(ch))
	perform_mount_violence(ch);

//    hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    /* XXX: Need to see if they can handle "" instead of NULL. */
    if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
      (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
  }
}
