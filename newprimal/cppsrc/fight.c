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
#include "corpses.h"
#include "dg_scripts.h"
#include "clan.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern struct spell_info_type spell_info[]; 
extern int pk_allowed;		/* see config.c */
extern int max_exp_gain;	/* see config.c */
extern int max_exp_loss;	/* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;

// DM - Saved Corpse Data
extern CorpseData corpseData;

/* External procedures */
char *fread_action(FILE * fl, int nr);
SPECIAL(titan);
ACMD(do_flee);
ACMD(do_violent_skill);
int backstab_mult(int level);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
void make_titan_corpse(struct char_data *ch);  
int compute_armor_class(struct char_data *ch, bool divide);
int num_attacks(struct char_data *ch);
int mag_savingthrow(struct char_data * ch, int type, int modifier);
int is_axe(struct obj_data *obj);
int is_blade(struct obj_data *obj);
void clan_pk_update (struct char_data *winner, struct char_data *loser);
int level_exp(struct char_data *ch, int level);
bool violence_check(struct char_data *ch, struct char_data *vict, int skillnum);

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
void group_gain(struct char_data * ch, struct char_data * victim);
void solo_gain(struct char_data * ch, struct char_data * victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
// int compute_armor_class(struct char_data *ch, bool divide);
// int compute_thaco(struct char_data *ch);  // Artus - Not Used.

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

  if (LR_FAIL(ch, LVL_ANGEL))
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}


int compute_armor_class(struct char_data *ch, bool divide)
{
  /* Artus> I have modified this slightly so that it will always return
   * a figure between +20 and -20.. Make AC a little more meaningful. */
  int armorclass = GET_AC(ch);

  if (AWAKE(ch))
    armorclass += dex_app[GET_DEX(ch)].defensive;

  // Bonus to Armour Class if special  is set
  if ( !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_SUPERMAN))
	armorclass -= (int)(armorclass * 0.10);

  armorclass = MIN(200, armorclass);
  if (divide)
  {
    if (armorclass > 0)
      armorclass += 5;
    else
      armorclass -= 4;
    armorclass /= 10;
    return (MAX(-20, armorclass));      /* -100 is lowest */
  }
  return (MAX(-200, armorclass));
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    basic_mud_log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
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
      basic_mud_log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
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
  if (!LR_FAIL(ch, LVL_IS_GOD))
    return;
  if (((GET_CLAN(ch) > 0) || (EXT_FLAGGED(ch, EXT_PKILL))) && 
      ((GET_CLAN(vict) > 0) || (EXT_FLAGGED(vict, EXT_PKILL))))
    return;
  // Artus> Mortal Kombat.
  if (PRF_FLAGGED(ch, PRF_MORTALK) && PRF_FLAGGED(vict, PRF_MORTALK))
    return;

  SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
  sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
  mudlog(buf, BRF, LVL_ANGEL, TRUE);
  send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict, int skillnum)
{
  if (ch == vict)
    return;

  // Artus> Prevent fighting when in writing states.
  if (!IS_NPC(ch) && (ch->desc) && (STATE(ch->desc) > CON_PLAYING))
    return;
  if (!IS_NPC(vict) && (vict->desc) && (STATE(vict->desc) > CON_PLAYING))
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  /* Artus> Don't start them fighting if vict is fighting someone else, and it
   *        was a spell, or throw style skill that was performed. */
  if (FIGHTING(vict) && (FIGHTING(vict) != ch))
  {
    if ((skillnum > 0) && (skillnum <= MAX_SPELLS))
      return;
    switch (skillnum)
    {
      case SKILL_PRIMAL_SCREAM:
      case SKILL_THROW:
      case SKILL_AXETHROW:
	return;
    }
  }
  ch->next_fighting = combat_list;
  combat_list = ch;

  if (!IS_NPC(ch) && char_affected_by_timer(ch, TIMER_MEDITATE))
    timer_from_char(ch, TIMER_MEDITATE);
  
  if (!IS_NPC(ch) && char_affected_by_timer(ch, TIMER_HEAL_TRANCE))
    timer_from_char(ch, TIMER_HEAL_TRANCE);
  
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;
  
  if (!IS_NPC(ch) && !IS_NPC(vict))
    if (!INSTIGATOR(vict)) 
      INSTIGATOR(ch) = TRUE;

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

  if (!IS_NPC(ch) && (AFF_FLAGGED(ch, AFF_BERSERK)))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_BERSERK);

  if (MOUNTING(ch) && FIGHTING(MOUNTING(ch)))
	stop_fighting(MOUNTING(ch));
}

struct obj_data *make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  room_rnum deathroom = NOWHERE; // Artus> Replaces room_vnum roomvnum;
  int i;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;

  sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = str_dup(buf2);

  sprintf(buf2, "the corpse of %s", GET_NAME(ch));
  corpse->short_description = str_dup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */

  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  // DM - Using obj value 2 to store whether the idnum of the player if it is 
  // a PC corpse, use GET_CORPSEID macro to get idnum
  if (IS_NPC(ch)) {
    GET_OBJ_VAL(corpse, 2) = 0;	/* npc corpse identifier */
    corpse->name = str_dup("corpse");
  } else {
    sprintf(buf,"Creating PC corpse of %s", GET_NAME(ch));
    basic_mud_log(buf);
    corpse->name = str_dup("corpse pcorpse");
    GET_OBJ_VAL(corpse, 2) = GET_IDNUM(ch);	/* pc corpse identifier */
  }
  //sprintf(buf,"Idnum of corpse: %d", GET_CORPSEID(corpse));
  //basic_mud_log(buf);

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
    {
      remove_otrigger(GET_EQ(ch, i), ch);
      obj_to_obj(unequip_char(ch, i, FALSE), corpse);
    }

  /* transfer gold */
  if (GET_GOLD(ch) > 0)
  {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc))
    {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

#define VNUM_TAVERN 1112
// Artus> What's the point?... We use the rnum for everything.
// room_vnum roomvnum;

  // Artus> Changed to drop corpse outside DTs.
  if (!IS_NPC(ch) && EXT_FLAGGED(ch, EXT_AUTOCORPSE))
    deathroom = real_room(VNUM_TAVERN);
  else
    deathroom = IN_ROOM(ch);
  if ((deathroom == NOWHERE) || is_death_room(deathroom))
  {
    if (LASTROOM(ch) != NOWHERE)
      deathroom = LASTROOM(ch);
  }
  if (!IS_NPC(ch))
  {
    if (deathroom == NOWHERE)
    {
      deathroom = real_room(VNUM_TAVERN);
      if (deathroom == NOWHERE)
      {
	sprintf(buf, "SYSERR: NOWHERE for corpse of %s. (Tav: %d, In: %d(v%d), Last: %d(v%d)", GET_NAME(ch), VNUM_TAVERN, IN_ROOM(ch), 
	       world[IN_ROOM(ch)].number, LASTROOM(ch), 
	       world[LASTROOM(ch)].number);
	mudlog(buf, NRM, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE);
	deathroom = 1;
      }
    }
    sprintf(buf, "(PCorpse) Corpse of %s left in room #%d - %s.",
	    GET_NAME(ch), world[deathroom].number, world[deathroom].name);
    mudlog(buf, CMP, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE);
  }

  /* Artus> The old way...
  if (!IS_NPC(ch) && EXT_FLAGGED(ch, EXT_AUTOCORPSE))
    roomvnum = VNUM_TAVERN;
  else
    roomvnum = GET_ROOM_VNUM(ch->in_room); */

  // DM - Save the corpse data, addCorpse will be called again in obj_to_room,
  // but we must set the weight first ...
  if (!IS_NPC(ch))
    corpseData.addCorpse(corpse, world[deathroom].number, GET_WEIGHT(ch)); 

  obj_to_room(corpse, deathroom);

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
  void clan_tax_update (struct char_data *ch, struct char_data *vict);
  struct obj_data *corpse = NULL, *obj, *next_obj = NULL;
  bool found=FALSE, arms_full=FALSE;
  int amount, num=1, share;
  struct char_data *k, *scab;
  struct follow_type *f;
  struct affected_type *af, *next_af;
  int corpseModified = 0;
  void handle_quest_mob_death(struct char_data *ch, struct char_data *killer);
 
  extern struct index_data *mob_index;
 
  void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
				  struct obj_data * cont, int mode, 
				  int *corpseModified);
  stop_fighting(ch);
  // Artus> Stop the killer fighting, if necessary.
  if ((killer) && (FIGHTING(killer) == ch)) 
    stop_fighting(killer);
 
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_QUEST))
    handle_quest_mob_death(ch, killer);

  for(af = ch->affected; af; af = next_af)
  {
    next_af = af->next;
    if ((af->duration != CLASS_ABILITY) &&
	(af->duration != CLASS_ITEM))
      affect_remove(ch, ch->affected);
  }
 
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_MORTALK))
  {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_MORTALK);
    call_magic(ch,ch,NULL,SPELL_WORD_OF_RECALL,GET_LEVEL(ch),CAST_MAGIC_OBJ);
    GET_HIT(ch) = MAX(GET_HIT(ch), GET_MAX_HIT(ch));
    GET_MANA(ch) = 100;
    GET_POS(ch) = POS_STUNNED;
  } else {
    /* remove wolf/vamp infections so ya hafta be reinfected after death */
    if (!IS_NPC(ch))
    {
      REMOVE_BIT(PRF_FLAGS(ch),PRF_WOLF | PRF_VAMPIRE);
      SET_BIT(EXT_FLAGS(ch), EXT_GHOST);
    }

    if(GET_HIT(ch) > 0)
      GET_HIT(ch) = 0;
 
    // DM - added killer != ch, check - suicide ...
    if (killer && killer != ch) 
    {
      if (!IS_NPC(ch) && !IS_NPC(killer)) 
        clan_pk_update(killer, ch);
      if (IS_NPC(ch) && !IS_NPC(killer))
      {
	GET_MOBKILLS(killer)++;
	clan_tax_update(killer, ch);
      }
      if (!IS_NPC(ch) && IS_NPC(killer))
	GET_KILLSBYMOB(ch)++;
      if (death_mtrigger(ch, killer))
        death_cry(ch);
    } else
      death_cry(ch);

    if (GET_MOB_SPEC(ch) == titan)
    {
      make_titan_corpse(ch);
      extract_char(ch);
      return;
    }
    corpse = make_corpse(ch);
 
    // Dismount
    if (MOUNTING(ch)) 
	MOUNTING(MOUNTING(ch)) = NULL;
    if (MOUNTING_OBJ(ch))
	OBJ_RIDDEN(MOUNTING_OBJ(ch)) = NULL;

    MOUNTING(ch) = NULL;
    MOUNTING_OBJ(ch) = NULL;

    /* DM - check to see if someone killed ch, or if we dont have a corpse */
    if (!killer || !corpse) 
    {
      extract_char(ch);
      return;
    }
 
  /* DM - autoloot/autogold - automatically loot if ch is NPC
        and they are in the same room */
 
    if (((!IS_NPC(killer) && IS_NPC(ch)) && 
         (killer->in_room == ch->in_room)) ||
         (IS_CLONE_ROOM(killer))) 
    {
      // make sure we have the master if a clone made the kill
      scab = (IS_CLONE_ROOM(killer) ? killer->master : killer);
      if (!scab || IS_NPC(scab)) // bail
        return;
      /* Auto Loot */
      if (EXT_FLAGGED(scab, EXT_AUTOLOOT)) 
      {
        for (obj = corpse->contains; obj && !arms_full; obj = next_obj) {
          if (IS_CARRYING_N(scab) >= CAN_CARRY_N(scab)) 
	  {
            send_to_char("Your arms are already full!\r\n", scab);
            arms_full = TRUE;
            continue;
          }
          next_obj = obj->next_content;
          if (CAN_SEE_OBJ(scab, obj) && (GET_OBJ_TYPE(obj) != ITEM_MONEY)) 
	  {
            // DM - autoloot isnt going to work on PC corpses, ignore corpse
            // saving
            perform_get_from_container(scab, obj, corpse, 0, &corpseModified);
            found = TRUE;
          }
        }
        if (!found)
          act("$p seems to be empty.", FALSE, scab, corpse, 0, TO_CHAR);
      } 
 
      /* Auto Gold */
      if (EXT_FLAGGED(scab, EXT_AUTOGOLD)) 
      {
        for (obj = corpse->contains; obj; obj = next_obj) 
	{
          next_obj = obj->next_content;
          if ((GET_OBJ_TYPE(obj) == ITEM_MONEY) && (GET_OBJ_VAL(obj, 0) > 0)) 
	  {
            /* Auto Split */
            if (EXT_FLAGGED(scab, EXT_AUTOSPLIT) && 
		IS_AFFECTED(scab, AFF_GROUP)) 
	    {
              amount = GET_OBJ_VAL(obj, 0);
              k = (scab->master ? scab->master : scab);
              // check followers of master
              for (f = k->followers; f; f = f->next) {
                if (IS_AFFECTED(f->follower, AFF_GROUP) && 
		    (!IS_NPC(f->follower)) && (f->follower != scab) && 
                    (f->follower->in_room == scab->in_room))
                  num++;
              }
              // now check scab vs master
              if (k != scab && k->in_room == scab->in_room)
                num++;
              if (num == 1)
                perform_get_from_container(scab, obj, corpse, 0,
                                &corpseModified);
              else 
	      {
                obj_from_obj(obj); 
                share = amount / num;
                GET_GOLD(scab) += share;
                sprintf(buf, "You split &Y%d&n coins among %d members -- &Y%d&n coins each.\r\n", amount, num, share); send_to_char(buf, scab);
                // check followers of master
                for (f=k->followers; f;f=f->next) 
		{
                  if (IS_AFFECTED(f->follower, AFF_GROUP) &&
                      (!IS_NPC(f->follower)) &&
                      (f->follower->in_room == scab->in_room) &&
                      f->follower != scab) 
		  {
                    GET_GOLD(f->follower) += share;
                    sprintf(buf, "&7%s&n splits &Y%d&n coins; you receive &Y%d&n.\r\n",
                      GET_NAME(scab), amount, share);
                    send_to_char(buf, f->follower);
                  }
                }
		extract_obj(obj);
                // now check scab vs master
                if (k != scab && IS_AFFECTED(k, AFF_GROUP) && (k->in_room == scab->in_room)
                    && !(IS_NPC(k))) 
		{
                  GET_GOLD(k) += share;
                  sprintf(buf, "&7%s&n splits &Y%d&n coins; you receive &Y%d&n.\r\n", GET_NAME(scab), amount, share);
                  send_to_char(buf, k);
                }
              }
          /* Not grouped or autosplitting */
            } else
              // DM - autoloot isnt going to work on PC corpses, ignore corpse
              // saving
              perform_get_from_container(scab, obj, corpse, 0, &corpseModified);
          }
        }
      } /* End of Autogold */
    } /* End of Auto loot/gold/split able */
    extract_char(ch);
  } 
}



void die(struct char_data * ch, struct char_data *killer, char *msg)
{
  int exp_lost = 0;
  extern struct zone_data *zone_table;
  
  if (killer)
    GET_WAIT_STATE(killer) = 0;

  if (IS_NPC(ch))
  {
    raw_kill(ch, killer);
    return;
  }

  basic_mud_log("DBG: %s exp prior to death: %d", GET_NAME(ch), GET_EXP(ch));

  if (!PRF_FLAGGED(ch, PRF_MORTALK))
  {
  /* DM_exp : If exp is not negative (which it shouldn't be ie level min = 0)
                use (exp this level)/2 */
    if (GET_EXP(ch) > 0) 
    {
      exp_lost = MIN((int)(level_exp(ch, GET_LEVEL(ch) + 2)/4),
	             (int)(GET_EXP(ch)/2));
      exp_lost = 0-gain_exp(ch, 0-exp_lost);
      // autocorpse - half of current level exp ...
      if (EXT_FLAGGED(ch, EXT_AUTOCORPSE))
        exp_lost += gain_exp(ch, exp_lost>>1);
    } else
      GET_EXP(ch) = 0;
    if (killer)
    {
      if (PLR_FLAGGED(ch, PLR_KILLER) && (killer != ch))
	REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER);
      sprintf(buf2, "%s killed by %s at %s (Lost %d Exp)", GET_NAME(ch), 
	      (killer != ch) ? GET_NAME(killer) : "stupidity", 
	      world[killer->in_room].name, exp_lost);
      mudlog(buf2, BRF, LVL_ANGEL, TRUE);
    } else if (msg) {
      sprintf(buf2, "%s killed by %s at %s (Lost %d Exp)", GET_NAME(ch),
	      msg, world[IN_ROOM(ch)].name, exp_lost);
      mudlog(buf2, BRF, LVL_ANGEL, TRUE);
    } else {
      sprintf(buf2, "%s killed by unknown at %s (Lost %d Exp)", 
	      GET_NAME(ch), world[IN_ROOM(ch)].name, exp_lost);
      mudlog(buf2, BRF, LVL_ANGEL, TRUE);
    }
    sprintf(buf2, "&[&7%s&] has been slain in %s!", GET_NAME(ch), 
	    zone_table[world[IN_ROOM(ch)].zone].name);
    info_channel(buf2, ch);
    GET_GOLD(ch)=(int)(GET_GOLD(ch)*0.9);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  }

  basic_mud_log("DBG: %s exp after death: %d", GET_NAME(ch), GET_EXP(ch));

  // Artus - Newbie Protection
  if (LR_FAIL_MAX(ch, LVL_NEWBIE))
  {
    if (killer && killer != ch)
    {
      if (!IS_NPC(ch) && !IS_NPC(killer)) 
        clan_pk_update(killer, ch);
      if (IS_NPC(ch) && !IS_NPC(killer))
	GET_MOBKILLS(killer)++;
      if (!IS_NPC(ch) && IS_NPC(killer))
	GET_KILLSBYMOB(ch)++;
      if (death_mtrigger(ch, killer))
        death_cry(ch);
    }
    stop_fighting(ch);
    char_from_room(ch);
    char_to_room(ch, real_room(SAVE_ROOM_VNUM));
    GET_HIT(ch) = 1;
    GET_POS(ch) = POS_RESTING;
    SET_BIT(EXT_FLAGS(ch), EXT_GHOST);
    send_to_char("You wake up in Haven's riverside tavern, however you find yourself to be\r\nnot quite whole. You are have become a &WGhost&n. This affect will last\r\nuntil you have reached your maximum hit point level.\r\n", ch);
    return;
  }

  raw_kill(ch,killer);
}



void perform_group_gain(struct char_data * ch, int base,
			     struct char_data * victim)
{
  int share;
  struct event_data *ev;
  extern struct event_list events;

  share = MIN(max_exp_gain, MAX(1, base));

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_MORTALK))
    return;
  if (PLR_FLAGGED(victim, PLR_KILLER))
    return;
  if (IS_NPC(victim) && (GET_MOB_VZNUM(victim) == CLAN_ZONE) || 
      MOB_FLAGGED(victim, MOB_NOKILL))
    return;

  if (share > 1) 
  {
    sprintf(buf2, "You receive your share of experience -- &c%d&n points.\r\n", (int)(share * (1/GET_MODIFIER(ch))));
    send_to_char(buf2, ch);
  } else
    send_to_char("You receive your share of experience -- one measly little point!\r\n", ch);

  share = gain_exp(ch, share);
  if (share > 0) 
    for(ev = events.list; ev; ev = ev->next)
      if (ev->type == EVENT_HAPPY_HR)
      {
	sprintf(buf, "Happy Hour! You gain a further &c%d&n exp!\r\n", share);
	send_to_char(buf, ch);
	gain_exp(ch, share);
	break;
      }
  change_alignment(ch, victim);
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base, exp;
  struct char_data *k;
  struct follow_type *f;
  int min_level = LVL_IMPL, max_level = 0, group_level, av_group_level, percent; 

  if (!(k = ch->master))
    k = ch;

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_MORTALK))
  {
     stop_fighting(ch);
     stop_fighting(victim);
     send_to_char("You are the SUPREME warrior of the Mortal Kombat arena!\r\n",
                  ch);
     return;
  }
  if (PLR_FLAGGED(victim, PLR_KILLER)){
           send_to_char("You receive No EXP for killing players with the KILLER flag\r\n", ch);
           return;
   }
 
  group_level = GET_LEVEL(k);
  max_level = min_level = MAX(max_level, GET_LEVEL(k));

  if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  // TODO: construct a fancier base exp - based on classes in group etc ...
  // ie. something to replace the next 30 odd lines in a new function.
//  base = calc_group_exp_base(ch, &min_level, &max_level, &tot_members);
  
  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && 
                    f->follower->in_room == ch->in_room)

      // DM - Dont include CLONES if they are grouped 
      // if (!IS_CLONE(f->follower)) {
      // Artus - Don't include NPCs if they are grouped.
      if (!IS_NPC(f->follower))
      {
        tot_members++;
        group_level += GET_LEVEL(f->follower);
        min_level = MIN(min_level, GET_LEVEL(f->follower));
        max_level = MAX(max_level, GET_LEVEL(f->follower));
      }   
  
  av_group_level = group_level / tot_members;

  /* cap it to LVL_IMPL */
  group_level = MIN(LVL_IMPL, group_level - tot_members);  

  /* round up to the next highest tot_members */
  base = (GET_EXP(victim) / 3) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  //if (!IS_NPC(victim))
  //  base = MIN(max_exp_loss * 2 / 3, tot_gain);

  /* DM group gain percentage */
  if (tot_members >= 1) {
    /* base = MAX(1, GET_EXP(victim) / 3) / tot_members; */
    if (tot_members == 2)
        base=(GET_EXP(victim)*50/*80*/)/100;
    else if (tot_members == 3)
        base=(GET_EXP(victim)*55/*85*/)/100;
    else if (tot_members == 4)
        base=(GET_EXP(victim)*60/*90*/)/100;
    else if (tot_members == 5)
        base=(GET_EXP(victim)*40/*70*/)/100;
    else if (tot_members == 6)
        base=(GET_EXP(victim)*35/*65*/)/100;
    else if (tot_members == 7)
        base=(GET_EXP(victim)*25/*55*/)/100;
    else if (tot_members == 8)
        base=(GET_EXP(victim)*5/*20*/)/100;
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
  int exp, percent, mod_exp;
  struct event_data *ev;
  extern struct event_list events;

  exp = MIN(max_exp_gain, GET_EXP(victim) / 3);
 
  if (IS_NPC(victim) && (GET_MOB_VZNUM(victim) == CLAN_ZONE))
  {
    if (!IS_NPC(ch))
      send_to_char("You are satisfied with your kill even though you gain nothing.\r\n", ch);
    return;
  }

  /* Calculate level-difference bonus */
  if (IS_NPC(ch) && !IS_CLONE(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
  else {
//          exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
 
    // Give exp to rider
    if (MOUNTING(ch) && IS_NPC(ch) && !IS_CLONE(ch))
      ch = MOUNTING(ch);

/* JA new level difference code */
    if (GET_LEVEL(victim) < GET_LEVEL(ch)) {
      percent = MAX(10, 100 - ((GET_LEVEL(ch) - GET_LEVEL(victim)) * 10));
      percent = MIN(percent, 100);
      exp = (exp * percent) / 50;
      if (percent <= 60) {
        sprintf(buf2, "Your opponent was out of your league! You don't learn "
                        "as much.\n\r");
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

  // Disp the actual exp they will gain - apply the modifiers here...
  if (IS_NPC(ch) && !IS_CLONE(ch)) {
    mod_exp = exp;
  } else {
    if (IS_CLONE(ch) ) {
      mod_exp = (int)(exp * (1 / GET_MODIFIER(ch->master)));
    } else {
      mod_exp = (int)(exp * (1 / GET_MODIFIER(ch)));
    }
  }

  if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
    ch = ch->master;

  // DM - TODO - check player specials in here ...
  if (PLR_FLAGGED(victim, PLR_KILLER)){
/*  if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
      send_to_char("You receive No EXP for killing players with the KILLER "
                      "flag!.\r\n", ch->master);
    else
    */
      send_to_char("You receive No EXP for killing players with the KILLER "
                      "flag!.\r\n", ch);
    exp = 0;
  } else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_MORTALK)) {
/*  if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
      send_to_char("You are the SUPREME winner of Mortal Kombat!!\r\n", 
                      ch->master);
    else
    */
      send_to_char("You are the SUPREME winner of Mortal Kombat!!\r\n", ch);
    exp = 0;
  } else if (exp > 1) {
/*  if (IS_CLONE(ch) && IS_CLONE_ROOM(ch)) {
      sprintf(buf2, "You receive %s%d%s experience points.\r\n",
                CCEXP(ch->master,C_NRM),mod_exp,CCNRM(ch->master,C_NRM));
      send_to_char(buf2, ch->master);
    } else {
      sprintf(buf2, "You receive %s%d%s experience points.\r\n",
                CCEXP(ch,C_NRM),mod_exp,CCNRM(ch,C_NRM));
      send_to_char(buf2, ch);
    }
    */
    sprintf(buf2, "You receive %s%d%s experience points.\r\n",
	      CCEXP(ch,C_NRM),mod_exp,CCNRM(ch,C_NRM));
    send_to_char(buf2, ch);
  } else 
/*  if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
      send_to_char("You receive one lousy experience point.\r\n", ch->master);
    else
    */
      send_to_char("You receive one lousy experience point.\r\n", ch);
 
  /* DM - dont let clones steal exp */
  /*
  if (IS_CLONE_ROOM(ch)) {
    mod_exp = gain_exp(ch->master, exp);
    change_alignment(ch->master, victim);
  } else {
    mod_exp = gain_exp(ch, exp);
    if (!IS_CLONE(ch))
      change_alignment(ch, victim); 
  }
  */
  mod_exp = gain_exp(ch, exp);
  if (!IS_CLONE(ch))
    change_alignment(ch, victim);

  if (mod_exp > 0) 
    for(ev = events.list; ev; ev = ev->next)
      if (ev->type == EVENT_HAPPY_HR)
      {
	sprintf(buf, "Happy Hour! You gain a further &c%d&n exp!\r\n", mod_exp);
	send_to_char(buf, ch);
	gain_exp(ch, mod_exp);
	break;
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

      if (!IS_NPC(vict) && !LR_FAIL(vict, LVL_IS_GOD)) {
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

/*
 * Called when an item is broken, appropiately handles what to do with the item
 * TODO: do we use an act to notify others? or does send_to_char suffice?
 */
void break_obj(struct obj_data *obj, struct char_data *ch) {

  if (!obj || !ch) {
    basic_mud_log("SYSERR: NULL object or char passed to break_obj");
    return;
  }

  switch (GET_OBJ_TYPE(obj) /* WTF is this -> ??? == ITEM_LIGHT ??? */) {
    // Destoryable items
    case ITEM_LIGHT:
    case ITEM_SCROLL:
    case ITEM_STAFF:
    case ITEM_WEAPON:
    case ITEM_FIREWEAPON:
    case ITEM_MISSILE:
    case ITEM_ARMOR:
    case ITEM_POTION:
    case ITEM_WORN:
    case ITEM_TRASH:
    case ITEM_DRINKCON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_PEN:
    case ITEM_BOAT:
    case ITEM_JOINABLE:
      sprintf(buf, "... &r%s is destroyed!&n\r\n", obj->short_description);
      obj_from_char(obj);  // Obj from before extraction. 
      extract_obj(obj);
      send_to_char(buf, ch);
      break;

    // Handle containers - make contents go to what container was in, and
    // filter downwards towards ground??
    // Or just make them unopenable till they are fixed? - easiest code wise
    // and rather reasonable I think?
    case ITEM_CONTAINER:
      sprintf(buf, "&r%s is broken and seals shut!&n\r\n", 
                      obj->short_description);
      send_to_char(buf, ch);
      GET_OBJ_VAL(obj, 1) = CONT_CLOSED; 
      break;

    // Undestoryable items (fixable with max damage)
    case ITEM_OTHER:
    case ITEM_WAND:
    case ITEM_TREASURE:
    case ITEM_TRAP:
    case ITEM_NOTE:
    case ITEM_MONEY:
    case ITEM_FOUNTAIN:
    case ITEM_MAGIC_EQ:
    case ITEM_QUEST:
      sprintf(buf, "... &r%s breaks!&n\r\n", obj->short_description);
      send_to_char(buf, ch);
      // put in inventory if obj gets broken
      for (int i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(ch, i) == obj) {
          unequip_char(ch, i, FALSE);
          obj_to_char(obj, ch, __FILE__, __LINE__); 
        }
      }
      break;
    default:
      basic_mud_log("SYSERR: invalid obj type given to break_obj (%d)", 
                      GET_OBJ_TYPE(obj));
      return;
  }
}

/* perform the damage and display the message */
void damage_obj(struct obj_data *obj, int damage, struct char_data *ch) {
  if (!obj || !ch)
  {
    basic_mud_log("SYSERR: NULL obj or char passed to damage_obj");
    return;
  }
  GET_OBJ_DAMAGE(obj) -= damage;
  sprintf(buf, "&r%s is damaged.&n\r\n", obj->short_description);
  send_to_char(buf, ch);
}

/* Damages a random piece of equipment on the target */
void damage_equipment(struct char_data *ch) {

  int i, found = 0;
  struct obj_data *obj;

  // ahh lets ignore eq on mobs? ... DM
  // short way to fix something that cores - and we dont need
  // it damaging mob eq :)
  if (IS_NPC(ch)) {
    return;
  }
  
  for (i = 0; i < NUM_WEARS && !found; i++)
  {
    if (GET_EQ(ch, i) && (number(1, NUM_WEARS) == NUM_WEARS) &&
	(GET_OBJ_MAX_DAMAGE(GET_EQ(ch, i)) != -1) &&
	!OBJ_FLAGGED(GET_EQ(ch, i), ITEM_QEQ))
    {
      damage_obj(GET_EQ(ch, i), number(1, 5), ch);
      found = 1;
      break;
    }
  }
  // Nothing damaged. Try inventory.
  if (found)
  {
    if (GET_OBJ_DAMAGE(GET_EQ(ch, i)) <= 0 && 
	GET_OBJ_MAX_DAMAGE(GET_EQ(ch, i)) != -1)
    {
      GET_OBJ_DAMAGE(GET_EQ(ch,i)) = 0;
      break_obj(GET_EQ(ch, i), ch);
    }
    return;
  }
  // nothing was damaged, slight chance of getting inventory
  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
    if ((number(0, 10) == 0) &&
	(GET_OBJ_DAMAGE(obj) != -1) && !OBJ_FLAGGED(obj, ITEM_QEQ))
    {
      damage_obj(obj, number(1, 5), ch);
      if (GET_OBJ_DAMAGE(obj) <= 0)
      {
	GET_OBJ_DAMAGE(obj) = 0;
	break_obj(obj, ch);
      }
      return;
    }
  }
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data * ch, struct char_data * victim, int dam, int attacktype, bool vcheck)
{
  int manastolen = 0, damChance = 0; 

  /////// TODO - Artus> Break this up if possible.
  if (!victim)
  {
    if (ch)
      basic_mud_log("SYSERR: Attempt to damage null victim in room #%d by '%s'.", world[IN_ROOM(ch)].number, GET_NAME(ch));
    else
      basic_mud_log("SYSERR: Damage called with no char or victim!");
    return (0);
  }

  if (IS_GHOST(victim))
    return (0);

  if ((ch) && (vcheck))
  {
    if (violence_check(ch, victim, attacktype) == FALSE)
      return (0);
  } else {
    if (GET_POS(victim) <= POS_DEAD)
    {
      basic_mud_log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
 		    GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
      die(victim,ch);
      return (0);			/* -je, 7/7/92 */
    }
  }

#ifndef IGNORE_DEBUG
  if ((ch) && GET_DEBUG(ch))
  {
    sprintf(buf, "DEBUG: Damage=%d AttackType=%d\r\n", dam, attacktype);
    send_to_char(buf, ch);
  }
#endif
  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && !LR_FAIL(victim, LVL_IS_GOD))
    dam = 0;

  if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOKILL))
    dam = 0;

  if ((ch) && (victim != ch))
  {
    struct obj_data *weapon;

    /* Start the attacker fighting the victim */
    if ((GET_POS(ch) > POS_STUNNED) && (FIGHTING(ch) == NULL) &&
	(IN_ROOM(ch) == IN_ROOM(victim))) 
    {
      set_fighting(ch, victim, attacktype);

      // DM - TODO - wtf is this??
      // -Tal- I dont know, I didnt put it in   
      // ch and victim are mobs, victim has a master, 1/10 times if vict charmed and vict's master is
      // in same room as ch, then stop ch fighting, and make ch fight the victims master
      if (IS_NPC(ch) && IS_NPC(victim) && victim->master &&
          !number(0, 10) && IS_AFFECTED(victim, AFF_CHARM) &&
          (victim->master->in_room == ch->in_room))
      {
        if (FIGHTING(ch))
          stop_fighting(ch);
        hit(ch, victim->master, TYPE_UNDEFINED);
        return (0);
      }    
    }

    /* If you attack a pet, it hates your guts */
    if ((victim->master) && (victim->master == ch))
    {
      if (IS_CLONE(victim))
      {
	sprintf(buf, "Being your clone, %s knows your weakness.\r\n", HSSH(ch));
	send_to_char(buf, ch);
	GET_HIT(ch) = 0;
	GET_POS(ch) = POS_DEAD;
	raw_kill(ch, victim);
	raw_kill(victim, NULL);
	return (0);
      }
      stop_follower(victim);
    }
    
    /* If the victim is autoassisting the attacker, make them stop. */
    if (AUTOASSIST(victim) == ch)
      stop_assisting(victim);

    /* If the attacker is invisible, he becomes visible */
    if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
      appear(ch);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch, attacktype);

      // stay down u little bitch
      if (dam > 0 && attacktype == SKILL_TRIP)
        GET_POS(victim) = POS_SITTING;

      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }

    /* check for particular type of weapon mastery for more damage */
    if (!IS_NPC(ch) && (GET_EQ(ch, WEAR_WIELD))) 
    {
      if (GET_SKILL(ch, SKILL_AXEMASTERY) && is_axe(GET_EQ(ch, WEAR_WIELD))) 
      {
	//send_to_char("Your axe mastery comes in handy.\r\n", ch);
	dam += (int)(dam * 0.10);
      }
      if (GET_SKILL(ch, SKILL_BLADEMASTERY) && is_blade(GET_EQ(ch, WEAR_WIELD))) 
      {
	//send_to_char("Your blade mastery comes in handy.\r\n", ch);
	dam += (int)(dam * 0.10);
      }
    }

    /* check if victim has DEFEND skill  */
    /* Could potentially do much more with this skill -- ie, parry, riposte, and so on */
    // TODO: check for attack type?
    if (!IS_NPC(victim) && GET_SKILL(victim, SKILL_DEFEND))
    {
      if (number(1, 101) < GET_SKILL(victim, SKILL_DEFEND)) 
      {
	if (GET_LEVEL(ch) >= LVL_CHAMP)
	  dam -= ((int)(dam / 2));	// champs can halve damage
	else if (GET_LEVEL(ch) >= 75)
	  dam -= ((int)(dam / 3));	// 2/3's damage
	else if (GET_LEVEL(ch) >= 40)
	  dam -= ((int)(dam / 4));	// 3/4's damage
	else
	  dam -= ((int)(dam  / 5));	// 4/5's damage
	if (dam < 0)
	  dam = 0;   
	apply_spell_skill_abil(ch, SKILL_DEFEND);
  //	act("You defend yourself well against $n's attack.", FALSE, ch, 0, victim, TO_VICT);
  //	act("$N defends against your attack, avoiding some damage.", FALSE, ch, 0, victim,TO_CHAR);
      }
    }

    /* Double damage if char is berserk */
    if (!IS_NPC(ch))
    {
      if (AFF_FLAGGED(ch, AFF_BERSERK))
	if ((SPELL_EFFEC(ch, SKILL_BERSERK) / 100) > 0.5)
	  dam = (int)(dam * (2 * (SPELL_EFFEC(ch, SKILL_BERSERK) / 100)));
      // Artus> 1.5* Damage if BattleMage && Spell Type Attack.
      if ((attacktype > 1) && (attacktype < NUM_SPELLS))
	  dam = (int)((double)(dam * 1.5));
    }
  
    /* Check for PK if this is not a PK MUD */
    if (!pk_allowed) 
    {
      check_killer(ch, victim);
      if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
	dam = 0;
    }

    // DM: Tumble skill - chance vict tumbles avoiding attack
    // hmm is this best place? - first thoughts yeah, but of course this will
    // get called each attack (ie. wouldn't make sense tumbling three times if
    // ch has 3 attacks - not that it would be likely to happen. 
    // TODO: check for attack type?
    if (basic_skill_test(victim, SKILL_TUMBLE, FALSE)) 
      // 1 in 10 chance victim gets to attempt a tumble
      if (!number(0, 9))
	if (number(1, 100) < GET_SKILL(victim, SKILL_TUMBLE))
	{
	  dam = 0;
	  act("$N performs a tumble roll avoiding your attack!",
	      TRUE, ch, 0, victim, TO_CHAR);
	  act("You perform a tumble roll avoiding $n's attack!",
	      TRUE, ch, 0, victim, TO_VICT);
	  act("$N performs a tumble roll avoiding $n's attack!",
	      TRUE, ch, 0, victim, TO_NOTVICT);
	  apply_spell_skill_abil(victim, SKILL_TUMBLE);
	}
    
    // DM: Poison blade - Add percentage based poison damage, set poison on vict
    // NOTE: dont think there is a MAX_TYPE thingy for attacks - if anything is 
    // added after TYPE_STAB - this should be updated.
    //
    // We are ignoring the number of uses - it 
    // TODO: add in any resistance spells etc ...
    if ((weapon = GET_EQ(ch, WEAR_WIELD))) 
    {
      if (obj_affected_by_timer(weapon, TIMER_POISONBLADE) && attacktype >= 
	  TYPE_HIT && attacktype <= TYPE_STAB) 
      {
	dam = dam + (int)(MIN(50, (int)((SPELL_EFFEC(ch, SKILL_POISONBLADE) / 100) * (GET_SKILL(ch, SKILL_POISONBLADE) / 100) * (GET_LEVEL(ch) / 2))));
	act("$N is affected by your poison blade!", TRUE, ch, 0, victim, TO_CHAR);
	act("You are poisoned by $n's blade!", TRUE, ch, 0, victim, TO_VICT);
	act("$N is poisoned by $n's blade!", TRUE, ch, 0, victim, TO_ROOM);

	// add affect to char if not already affected by poison
	if (!IS_AFFECTED(victim, AFF_POISON)) 
	{
	  struct affected_type af;
	  af.type = SPELL_POISON;
	  af.duration = MIN(1, (int)(GET_LEVEL(ch) / GET_LEVEL(victim)));
	  af.modifier = 0;
	  af.location = APPLY_NONE;
	  af.bitvector = AFF_POISON;
	  affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
	}
      }
    } // PoisonBlade Check.

    /* Gain exp for the hit */
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_MORTALK))
    {
      if (!IS_NPC(victim) || 
	  ((GET_MOB_VZNUM(victim) != CLAN_ZONE) &&
	   !MOB_FLAGGED(victim, MOB_NOKILL)))
	gain_exp(ch, GET_LEVEL(victim) * dam);
    } else if (IS_NPC(ch)) {
      gain_exp(ch, GET_LEVEL(victim) * dam);
    }
  } // (ch) && (ch != victim)

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;

  /* Cut in half again if vict is berserk */
  if (!IS_NPC(victim) && (AFF_FLAGGED(victim, AFF_BERSERK)))
    if ((SPELL_EFFEC(victim, SKILL_BERSERK) / 100) > 0.5) 
      dam = (int)(dam / (2 * (SPELL_EFFEC(victim, SKILL_BERSERK) / 100)));

  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 1000), 0);
  GET_HIT(victim) -= dam;

  update_pos(victim);

  if (!(ch))
    return (dam);

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
  if (!IS_WEAPON(attacktype))
  {
    skill_message(dam, ch, victim, attacktype);
  } else {
    if (GET_POS(victim) == POS_DEAD || dam == 0)
    {
      if (!skill_message(dam, ch, victim, attacktype))
	dam_message(dam, ch, victim, attacktype);
    } else {
      dam_message(dam, ch, victim, attacktype);
    }
  }

// DM - need a check in here for 2nd/3rd attack - when calling 
// damage(ch,vict,0,2/3attack)?? This elimates the "extra message" shown when 
// the 2nd or 3rd attack isn't called.

// Artus - From now on skills with violent set to > 1 will also eliminate the
// message and flee checks.. These skills should later call damage with the
// same skillnum and a damage of 0... Stops flee message being sent before
// skill message.
  
/*  if (!((dam == 0) && 
	(attacktype == SKILL_2ND_ATTACK || attacktype == SKILL_3RD_ATTACK))) */
  if (!(((dam == 0) && (attacktype == SKILL_2ND_ATTACK || 
	              attacktype == SKILL_3RD_ATTACK)) ||
       ((dam != 0) && (IS_SKILL(attacktype) &&
		       spell_info[attacktype].violent > 1))))

  {
    /* Use send_to_char -- act() doesn't send message if you are DEAD. */
    switch (GET_POS(victim)) 
    {
      case POS_MORTALLYW:
	act("$n is mortally wounded, and will die soon, if not aided.", 
	    TRUE, victim, 0, 0, TO_ROOM);
	send_to_char("You are mortally wounded, and will die soon, if not "
		     "aided.\r\n", victim);
	break;
      case POS_INCAP:
	act("$n is incapacitated and will slowly die, if not aided.", 
	    TRUE, victim, 0, 0, TO_ROOM);
	send_to_char("You are incapacitated an will slowly die, if not "
			"aided.\r\n", victim);
	break;
      case POS_STUNNED:
	act("$n is stunned, but will probably regain consciousness again.", 
	    TRUE, victim, 0, 0, TO_ROOM);
	send_to_char("You're stunned, but will probably regain consciousness "
		      "again.\r\n", victim);
	break;
      case POS_DEAD:
	act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
	send_to_char("You are dead!  Sorry...\r\n", victim);
	break;
      default:			/* >= POSITION SLEEPING */
	if (dam > (GET_MAX_HIT(victim) / 4))
	  send_to_char("That really did HURT!\r\n", victim);
	if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) 
	{
	  sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so "
			"much!%s\r\n",
		  CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
	  send_to_char(buf2, victim);
	  if ((ch != victim && MOB_FLAGGED(victim, MOB_WIMPY) && 
	       GET_POS(victim) >= POS_FIGHTING) &&
	      (IN_ROOM(ch) == IN_ROOM(victim)))
  	  do_flee(victim, NULL, 0, 0);
	}
	if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	    GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0 &&
	    GET_POS(victim) >= POS_FIGHTING && IN_ROOM(ch) == IN_ROOM(victim))
	{
	  send_to_char("You wimp out, and attempt to flee!\r\n", victim);
	  do_flee(victim, NULL, 0, 0);
	}
	break;
    } // Switch (POS)
  } // If (Going to send messages, check flees).

  /* 
   *By this stage,  they're having a swing, damage eq regardless. 
   * Random chance of damaging EQ
   */
  if ((dam > 0) && (attacktype != SPELL_POISON))
  {
    damChance = 100 - (GET_LEVEL(ch) - GET_LEVEL(victim)) - GET_STR(ch);
    // Want to probably modify it depending on the
    // ch's damRoll as well (a % perhaps)
    if (damChance < 0)
      damChance = 1;
    if (number(0, damChance) == damChance) 	// Damaged eq! 
      damage_equipment(victim);
  }

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED)
  {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim))
    {
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
  if (GET_POS(victim) == POS_DEAD)
  {
    /* fix for poisoned players getting exp, and their group! bm */
    if (attacktype==SPELL_POISON || attacktype==TYPE_SUFFERING) 
    {
      if ((IS_NPC(ch)) && (MOB_FLAGGED(ch, MOB_MEMORY)))
	forget(ch, victim);
      die(victim,ch,"poison");
      return(0);  
    }

    if ((ch != victim) && (IS_NPC(victim) || victim->desc))
    {
      /* DM - if CLONE makes kill in group - only give exp to CLONES master */
      if (AFF_FLAGGED(ch, AFF_GROUP) && !IS_CLONE(ch))
        group_gain(ch, victim);
      else if (MOUNTING(ch) && IS_NPC(ch))
        solo_gain(MOUNTING(ch), victim);	// XP to rider
      else
        solo_gain(ch, victim);
    }
    if (!IS_NPC(victim)) 
    {
/* moved to die()
 * sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
 *	      world[victim->in_room].name);
 *    mudlog(buf2, BRF, LVL_ANGEL, TRUE); */
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    die(victim,ch);
    return (-1);
  }
  // Mana stealing for mana thieves (spellswords)
  if ((dam > 0) && !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_MANA_THIEF))
  {
    /* Some semi random chance of draining
     * Current mana / (level * modifier) -- Becomes more likely when mana is
     * lower */
    if (number(0, (int)(GET_MANA(ch)/(GET_LEVEL(ch) * GET_MODIFIER(ch)))) != 0)
      return (dam);
    if (GET_MANA(ch) < GET_MAX_MANA(ch))
    {
      if (!IS_NPC(victim))
	manastolen += number(1, (int)(GET_MAX_MANA(victim) * 0.05));
      else
	manastolen += number(1, (int)(GET_MAX_MANA(ch) * 0.05));
      if (manastolen > GET_MANA(victim) && !IS_NPC(victim))
	manastolen = GET_MANA(victim);
      if (!IS_NPC(victim))
        GET_MANA(victim) -= manastolen;
      GET_MANA(ch) += manastolen;
      if (GET_MANA(ch) > GET_MAX_MANA(ch))
	GET_MANA(ch) = GET_MAX_MANA(ch);
      if (manastolen > 0)
      {
	act("You manage to steal some mana from $N as you hit $M.", FALSE,
	    ch, 0, victim, TO_CHAR);
	act("$n stole some of your mana!", FALSE, ch, 0, victim, TO_VICT);
      }
    }
  }

  // Extra damage for those with SPECIAL_FOREST_HELP
  if ((dam > 0) && !IS_NPC(ch) && 
      IS_SET(GET_SPECIALS(ch), SPECIAL_FOREST_HELP) &&
      (SECT(ch->in_room) == SECT_FOREST) && (victim != ch))
  {
    // Semi random chance of aid
    // Current hp / (level * modifier) -- Becomes more likely as druid is dying 
    if (number(0, (int)(GET_HIT(ch)/(GET_LEVEL(ch) * GET_MODIFIER(ch)))) != 0)
      return (dam);
    if (GET_CLASS(victim) == CLASS_DRUID)
    {
      act("Your forest allies are unable to harm another druid.",
	  FALSE, ch, 0, 0, TO_CHAR);
      act("You discourage $n's forest allies from attacking you.",
	  FALSE, ch, 0, victim, TO_VICT);
      return (dam);
    }
    act("Your forest allies come to your aid, briefly attacking $N.",
	FALSE, ch, 0, victim, TO_CHAR);
    act("$n's forest allies aid $s attack on $N.",
	FALSE, ch, 0, victim, TO_ROOM);
    act("$n's forest allies attack you briefly, aiding $m.",
	FALSE, ch, 0, victim, TO_VICT);
    dam += number(1, (int)(GET_LEVEL(ch) * GET_MODIFIER(ch) *
		  (GET_WIS(ch) < 15 ? 1 : GET_WIS(ch) < 19 ? 2 : 3)));
  }
 
  // Chance for outright destruction of undead for paladins
  if ((dam > 0) && !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_HOLY) &&
      (GET_CLASS(victim) == CLASS_UNDEAD))
  {
    if (number(1, (int)(GET_HIT(ch) / (GET_MODIFIER(ch) *
		      (GET_WIS(ch) < 15 ? 1 : GET_WIS(ch) < 19 ? 2 : 3)))) != 1)
      return (dam);
    act("&WThe holy powers within you arise and slay $N as you strike at $m.&n",
	FALSE, ch, 0, 0, TO_CHAR);
    act("&WOverwhelmed by $n's holy power, $N is banished from this realm!&n",
	FALSE, ch, 0, victim, TO_ROOM);
    act("&R$n's holy power overwhelms you, banishing you to the nether realms!&n", FALSE, ch, 0, victim, TO_VICT); 
    raw_kill(victim, ch);
    return (-1); // deaaaaaaaaad
  }

  // Damage enhancement for dwarves and elves
  if (!IS_NPC(ch) && ((IS_SET(GET_SPECIALS(ch), SPECIAL_ELF) &&
      (SECT(ch->in_room) == SECT_FOREST)) ||
      (IS_SET(GET_SPECIALS(ch), SPECIAL_DWARF) &&
      (SECT(ch->in_room) == SECT_INSIDE))))
    dam = (int)(dam * 1.10); 	

  // Extra damage for those with SPECIAL_GORE
  if ((dam > 0) && !IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_GORE))
  {
    /* Semi random chance of aid
     * Current hp / (level * modifier) -- Becomes more likely as character is
     * dying */
    if (number(0, (int)(GET_HIT(ch)/(GET_LEVEL(ch) * GET_MODIFIER(ch)))) !=0)
      return (dam);	

    if (GET_CLASS(victim) == CLASS_DRUID)
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
    act("$n's horns dig deeply into you.", FALSE, ch, 0, victim, TO_VICT);
    dam += number(1, (int)(GET_LEVEL(ch) * GET_MODIFIER(ch) *
	                   (GET_STR(ch) < 15 ? 1 : GET_STR(ch) < 19 ? 2 : 3))); 
  }
  return (dam);
}



void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;
  struct char_data *real_vict = victim;    
  struct char_data *char_room; 

  // Artus> Are we sane?
  if (!VALID_FIGHT(ch, victim))
  {
    if (ch && FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  // dual wield attack
  if (type == TYPE_DUAL_ATTACK)
    wielded = GET_EQ(ch, WEAR_HOLD);

  /* check if the character has a fight trigger */
  if ((fight_mtrigger(ch) == 1) && !VALID_FIGHT(ch, victim))
    return;

/* JA code to randomly select a player to hit */
/* to stop the tank from getting hit all the time */
 
  /* pseudo-randomly choose someone in the room who is fighting me */
  /* Artus> Might make life easier if we put this bak into victim, no? */
  if (IS_NPC(ch))
  {
    for (real_vict = world[ch->in_room].people; real_vict; real_vict = real_vict->next_in_room)
      if (FIGHTING(real_vict) == ch && !number(0, 3))
        break;
  }

  if (real_vict == NULL)
    real_vict = victim;

  /* Artus> Is this really needed anymore? :o)
  assert(real_vict); */ 

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
  /* changed so mobs dont use ammo - Vader */
  if (wielded && !IS_NPC(ch) && OBJ_IS_GUN(wielded))
  {
    /* out of ammo */
    if (GET_OBJ_VAL(wielded, 0) <= 0)
    {
      send_to_char("*CLICK*\n\r", ch);
      return;
    } else
      MAX(0, --GET_OBJ_VAL(wielded, 0));
  }      


  calc_thaco = thaco(ch, victim) - (int)(ch->points.hitroll / 5); 

  /* Calculate the raw armor including magic armor.  Lower AC is better. */
  victim_ac = compute_armor_class(victim, 1);

  /* roll the die and take your chances... */
  diceroll = number(1, 20);

  /* decide whether this is a hit or a miss */
  // Artus - Time for me to play.
#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: Thaco(%s)/AC(%s)/Diceroll: %d/%d/%d\r\n", GET_NAME(ch), 
	GET_NAME(victim), calc_thaco, victim_ac, diceroll);
    send_to_char(buf, ch);
  }
  if (GET_DEBUG(victim))
  {
    sprintf(buf, "DBG: Thaco(%s)/AC(%s)/Diceroll: %d/%d/%d\r\n", GET_NAME(ch),
	GET_NAME(victim), calc_thaco, victim_ac, diceroll);
    send_to_char(buf, victim);
  }
#endif
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac))))
  {
    /* the attacker missed the victim */
    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, SKILL_BACKSTAB, FALSE); 
    else if (type == SKILL_2ND_ATTACK || type == SKILL_3RD_ATTACK)
      damage(ch, real_vict, 0, type, FALSE);  /* thisll make it use the rite messages for missing */
    else
      damage(ch, real_vict, 0, w_type, FALSE);
  } else {
    /* okay, we know the guy has been hit.  now calculate damage. */

    /* Start with the damage bonuses: the damroll and strength apply */
    // DM: make todam also level based - lvl 1 char with 21 dam will kick
    // some major arse ...
    // DM: ok changed it from the apply to number(0, apply) ...
    dam = number(0, str_app[STRENGTH_REAL_APPLY_INDEX(ch)].todam);
    dam += GET_DAMROLL(ch);

    if (!IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_MINOTAUR))
      dam += MAX(5, (int)((GET_DAMROLL(ch) * 0.1)));

    if (!IS_NPC(ch) && IS_SET(GET_SPECIALS(ch) , SPECIAL_SUPERMAN))
      dam += MAX(5, (int)((GET_DAMROLL(ch) * 0.1)));

    /* Maybe holding arrow? */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      /* Add weapon-based damage if a weapon is being wielded */
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
#if 0 // Artus> Lets change this so that NPCs do their bare hand damage, too.
    } else {
      /* If no weapon, add bare hand damage instead */
      if (IS_NPC(ch)) {
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      } else {
	dam += number(0, 2);	/* Max 2 bare hand damage for players */
      }
    }
#endif
    } else {
      if (!IS_NPC(ch))
	dam += number(0, 2);
    }
    // Artus> Add NPC bare hand damage.
    if (IS_NPC(ch))
      dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);

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
        (GET_LEVEL(ch) >= PROTECT_LEVEL))
      dam -= (GET_LEVEL(real_vict) / MAX(2,num_attacks(real_vict)));

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);
    if(type == SKILL_2ND_ATTACK && dam > 1)
      dam = (int) (dam * 0.666); /* 2 3rds damage on second attack */
    if(type == SKILL_3RD_ATTACK && dam > 1)
      dam = (int) (dam * 0.333); /* 1/3 damage if on 3rd attack */

    // dual wield/attack (dam * skill ability percentage)
    if ((type == TYPE_DUAL_ATTACK) && (GET_SKILL(ch, SKILL_AMBIDEXTERITY)))
    {
      dam = (int)(dam * GET_SKILL(ch, SKILL_AMBIDEXTERITY) / 100);
      apply_spell_skill_abil(ch, SKILL_AMBIDEXTERITY);
    }
    
    if (type == SKILL_BACKSTAB)
    {
      dam *= (1+GET_LEVEL(ch)/10);
      if (damage(ch, real_vict, dam, SKILL_BACKSTAB, FALSE) > 0)
      { // ^ Check it's still alive
        if (GET_SKILL(ch, SKILL_DOUBLE_BACKSTAB))
        { // Give it another go
    	  if (number(1, 101 - (GET_LEVEL(victim) - GET_LEVEL(ch))) 
		< GET_SKILL(ch, SKILL_DOUBLE_BACKSTAB))		// Good enough?
	  {
	    act("...$n whirls with lightning speed backstabbing again!", FALSE, ch, 0, 0, TO_ROOM);
	    act("...you whirl and deal a double backstab to $N!", FALSE, ch, 0, victim, TO_CHAR);
	    act("...$n whirls with lightning speed backstabbing you again!", FALSE, ch, 0, victim, TO_VICT);
	    damage(ch, real_vict, dam * 2, SKILL_BACKSTAB, FALSE);
	    apply_spell_skill_abil(ch, SKILL_DOUBLE_BACKSTAB);
	  }
        }
      }	 
    } else
      damage(ch, real_vict, dam, w_type, FALSE);
    
    if (!VALID_FIGHT(ch, real_vict))
      return;

    /* if they have reflect then reflect */
    if ((IS_AFFECTED(real_vict, AFF_REFLECT)) && (dam > 0) && 
	(GET_POS(real_vict)> POS_MORTALLYW) && 
	(IN_ROOM(ch) == IN_ROOM(real_vict)))
    {
      damage(real_vict,ch,MIN(dam,MAX(GET_HIT(ch) - 1,0)),SPELL_SERPENT_SKIN,FALSE);
      if (!VALID_FIGHT(ch, real_vict))
	return;
    }
 
/* offensive/defensive spells - Lighting/Fire Shield, Fire Wall */
    if (affected_by_spell(real_vict,SPELL_LIGHT_SHIELD))
      if (mag_savingthrow(real_vict,SAVING_SPELL,0))
      {
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/4,MAX(GET_HIT(ch)-1,0)),SPELL_LIGHT_SHIELD,FALSE);
	if (!VALID_FIGHT(ch, real_vict))
	  return;
      }
    if (affected_by_spell(real_vict,SPELL_FIRE_SHIELD))
      if (mag_savingthrow(real_vict,SAVING_SPELL,0))
      {
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/3,MAX(GET_HIT(ch)-1,0)),SPELL_FIRE_SHIELD,FALSE);
	if (!VALID_FIGHT(ch, real_vict))
	  return;
      }
    if (affected_by_spell(real_vict,SPELL_FIRE_WALL))
      if (mag_savingthrow(real_vict,SAVING_SPELL,0))
      {
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/2,MAX(GET_HIT(ch)-1,0)),SPELL_FIRE_WALL,FALSE);
	if (!VALID_FIGHT(ch, real_vict))
	  return;
      }
 
/* if its a magic weapon decided whether to cast or not then do it - Vader */
    if(wielded && FIGHTING(ch))
    {
      if(IS_OBJ_STAT(wielded,ITEM_MAGIC) && (GET_OBJ_VAL(wielded,0) > 0))
      {
        if(number(0,3))
	{
        // DM - only use spells we know ... Artus> This sucks
        /* if (has_stats_for_skill(ch, GET_OBJ_VAL(wielded, 0), FALSE) 
                && GET_SKILL(ch, GET_OBJ_VAL(wielded, 0)) > 0 
                && number(0, 3)) */
	  // Artus> TODO - Remove Areas Spells.
	  if (IS_SET(spell_info[GET_OBJ_VAL(wielded, 0)].routines, MAG_AREAS) &&
	      !IS_OBJ_STAT(wielded, ITEM_QEQ))
	  {
	    sprintf(buf, "SYSERR: %s [Vnum %d] attempting MAG_AREAS.",
		    wielded->short_description, GET_OBJ_VNUM(wielded));
	    mudlog(buf, NRM, LVL_IMPL, TRUE);
	    if (ch->desc)
	    {
	      sprintf(buf, "&rLighting comes from the sky, disintegrating &5%s&r.&n\r\n", OBJS(wielded, ch));
	      send_to_char(buf, ch);
	    }
	    for(int where = 0; where < NUM_WEARS; where++)
	      if (GET_EQ(ch, where) == wielded)
		unequip_char(ch, where, FALSE);
	    extract_obj(wielded);
	    wielded = NULL;
	  } else {
	    call_magic(ch, real_vict, NULL, GET_OBJ_VAL(wielded,0), 
		       2 * GET_LEVEL(ch), CAST_MAGIC_OBJ); 
	    if (!VALID_FIGHT(ch, real_vict))
	      return;
	  } // MAG_AREAS Rorting.
	} // Random chance of spell happening.
      } // Wielded item is MAGIC.
    } // Is Wielding && Fighting.
  } // Artus> End - Did we hit check.

  /* DM - autoassist check */ // Artus> Added in charmed/clone autoassist.
  if ((GET_POS(victim) == POS_FIGHTING) && IS_NPC(victim) &&
      (IN_ROOM(ch) == IN_ROOM(victim)))
  {
    struct char_data *prior = NULL;
    if (world[ch->in_room].people != victim)
    {
      for(prior = world[ch->in_room].people; prior; prior = prior->next)
      {
        if (prior->next == victim)
	  break;
	if (!(prior->next))
	  return;
      }
    }
      
    for (char_room=world[ch->in_room].people;char_room;char_room=char_room->next_in_room)
    {
      if (FIGHTING(char_room)) // Artus> We're already fighting.
	continue;
      if (IS_NPC(char_room))
      {
	// Artus> NPCs autoassist master.
	if ((char_room->master == ch) && CAN_SEE(char_room,ch) && 
	    CAN_SEE(char_room,victim))
	{
	  act("$n assists $N!", FALSE, char_room, 0, ch, TO_NOTVICT);
	  act("$N assists you!", FALSE, ch, 0, char_room, TO_CHAR);
	  sprintf(buf, "You assist %s!\r\n", GET_NAME(ch));
	  hit(char_room, victim, TYPE_UNDEFINED);
	  if (prior)
	  {
	    if (prior->next != victim)
	      return;
	  } else {
	    if (world[ch->in_room].people != victim)
	      return;
	  }
	}
	continue;
      }
      if ((AUTOASSIST(char_room) == ch) && CAN_SEE(char_room,ch) && 
	  CAN_SEE(char_room,victim))
      {
	act("$n assists $N!", FALSE, char_room, 0, ch, TO_NOTVICT);
	act("$N assists you!", FALSE, ch, 0, char_room, TO_CHAR);
	sprintf(buf,"You assist %s!\n\r",GET_NAME(ch));
	send_to_char(buf,char_room);
	hit(char_room,victim,TYPE_UNDEFINED);
	if (prior)
	{
	  if (prior->next != victim)
	    return;
	} else {
	  if (world[ch->in_room].people != victim)
	    return;
	}
      }
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

  /* check if the victim has a hitprcnt trigger */
  hitprcnt_mtrigger(victim);
}


void perform_mount_violence(struct char_data *rider)
{

  /* TODO: Extend function here to allow specials on items and such */

  // Check if the opponent is history
  if (!FIGHTING(rider))
    return;

  if (MOUNTING(rider) && !FIGHTING(MOUNTING(rider)) ) {
    send_to_char("Your mount joins the fray!\r\n", rider);
    do_violent_skill(MOUNTING(rider), (FIGHTING(rider))->player.name, 
                     0, SCMD_HIT);
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
  void show_wounded_to_char(struct char_data *ch);
  int has_stats_for_prac(struct char_data *ch, int skillnum, bool show);

  for (ch = combat_list; ch; ch = next_combat_list)
  {
    next_combat_list = ch->next_fighting;

    loop = 1; /* Artus - Haste Fix (Hack/Slash).. Hal sucks ass */

    if (!VALID_FIGHTING(ch))
    {
      stop_fighting(ch);
      continue;
    } /* else { // Artus> Why?!? */

    if (affected_by_spell(ch, SPELL_HASTE))
      loop = 2;

    for (i=0;i<loop;i++) 
    {
      if (!VALID_FIGHTING(ch))
	break;

      if (i == 1)
	send_to_char("You are hastened and get more attacks\r\n", ch);
      	
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
      if (!VALID_FIGHTING(ch))
	break;

      // dual wield
      if (IS_DUAL_WIELDING(ch))
      {
	hit(ch, FIGHTING(ch), TYPE_DUAL_ATTACK);
	if (!VALID_FIGHTING(ch))
	  break;
      }

      /* the below bit is added to allow multiple attacks per turn - VADER */
      /* Artus> No 3rd without second.. Also, changed the way npcs are handled
       *        to remove some repeat npc checking. */ 
      second = third = 0;
      if(IS_NPC(ch)) 
      {
	if (MOB_FLAGGED(ch, MOB_2ND_ATTACK))
	{
	  second = 100;
	  if (MOB_FLAGGED(ch, MOB_3RD_ATTACK))
	    third = 100;
	}
      } else {
	if ((second = GET_SKILL(ch, SKILL_2ND_ATTACK)))
	{
	  if (has_stats_for_prac(ch, SKILL_2ND_ATTACK, false))
	  {
	    third = GET_SKILL(ch, SKILL_3RD_ATTACK);
	    if (!has_stats_for_prac(ch, SKILL_3RD_ATTACK, false))
	      third = 0;
	  } else
	    second = 0;
	}
      }  

      if (second > 0)
      {
	if((second + dice(3,(ch->aff_abils.dex))) > number(5,175))
	{
	  hit(ch, FIGHTING(ch), SKILL_2ND_ATTACK);
	  if (ch)
	    apply_spell_skill_abil(ch, SKILL_2ND_ATTACK);
	  if (!VALID_FIGHTING(ch))
	    break;
	  // dual wielding
	  if (IS_DUAL_WIELDING(ch))
	  {
	    hit(ch, FIGHTING(ch), TYPE_DUAL_ATTACK);
	    if (!VALID_FIGHTING(ch))
	      break;
	  }
	} else { 
	  damage(ch, FIGHTING(ch), 0, SKILL_2ND_ATTACK, FALSE);
	}

	if(third > 0)
	{
	  if ((third + dice(3,(ch->aff_abils.dex))) > number(5,200))
	  {
	    hit(ch, FIGHTING(ch), SKILL_3RD_ATTACK);
	    apply_spell_skill_abil(ch, SKILL_3RD_ATTACK);
	    if (!VALID_FIGHTING(ch))
	      break;
	    // dual wield
            if (IS_DUAL_WIELDING(ch))
	    {
	      hit(ch, FIGHTING(ch), TYPE_DUAL_ATTACK);
	      if (!VALID_FIGHTING(ch))
		break;
	    }
	  } else { 
	    damage(ch, FIGHTING(ch), 0, SKILL_3RD_ATTACK,FALSE);
          }
	}

	/* vampires and wolfs get an extra attack when changed - Vader */
	if (affected_by_spell(ch,SPELL_CHANGED) && (number(0,4) == 0))
	{
	  if(PRF_FLAGGED(ch,PRF_WOLF))
	    damage(ch,FIGHTING(ch),dice(4,GET_LEVEL(ch)),TYPE_CLAW,FALSE);
	  else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
	    damage(ch,FIGHTING(ch),dice(4,GET_LEVEL(ch)),TYPE_BITE,FALSE);
	  if (!VALID_FIGHTING(ch))
	    break;
	}
      } // End Haste Loop
      if ((ch) && !IS_NPC(ch) && AFF_FLAGGED(ch, AFF_SENSE_WOUNDS))
	show_wounded_to_char(ch);
    }

    if (!(ch))
      continue;

    if (IS_NPC(ch))
    {
      if (GET_MOB_WAIT(ch) > 0)
      {
        GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
        continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if ((GET_POS(ch) < POS_FIGHTING) && (GET_POS(ch) > POS_STUNNED))
      {
        GET_POS(ch) = POS_FIGHTING;
        act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("You scramble to your feet\r\n", ch);
      }
    } 

    /* Mounts add their own special attack */
    if (MOUNTING(ch) || MOUNTING_OBJ(ch))
      perform_mount_violence(ch);

    /* XXX: Need to see if they can handle "" instead of NULL. */
    if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
      (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
  } // End Fighting Loop.
}
