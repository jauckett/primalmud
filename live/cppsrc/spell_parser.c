/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
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
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "clan.h" // For violence_check
#include "constants.h"

//struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

#define SINFO spell_info[spellnum]

extern struct room_data *world;
extern struct spell_info_type spell_info[];

/* local functions */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch, struct obj_data * tobj, int cargeamt);
void spello(int spl, const char *name, int max_mana, int min_mana, int mana_change, int minpos, int targets, int violent, int routines);
int mag_manacost(struct char_data * ch, int spellnum);
ACMD(do_cast);
void unused_spell(int spl);
void mag_assign_spells(void);
bool violence_check(struct char_data *ch, struct char_data *vict, int skillnum);

/* external functions */
ACMD(do_hit);
int compute_armor_class(struct char_data *ch, bool divide);
void check_killer(struct char_data * ch, struct char_data * vict);
void clan_rel_inc(struct char_data *ch, struct char_data *vict, int amt);
int is_axe(struct obj_data *obj);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);

SPECIAL(postmaster);
SPECIAL(receptionist);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable {
  const char *org;
  const char *news;
};


struct syllable syls[] = {
  {" ", " "},
  {"ar", "abra"},
  {"ate", "i"},
  {"cau", "kada"},
  {"blind", "nose"},
  {"bur", "mosa"},
  {"cu", "judi"},
  {"de", "oculo"},
  {"dis", "mar"},
  {"ect", "kamina"},
  {"en", "uns"},
  {"gro", "cra"},
  {"light", "dies"},
  {"lo", "hi"},
  {"magi", "kari"},
  {"mon", "bar"},
  {"mor", "zak"},
  {"move", "sido"},
  {"ness", "lacri"},
  {"ning", "illa"},
  {"per", "duda"},
  {"ra", "gru"},
  {"re", "candus"},
  {"son", "sabru"},
  {"tect", "infra"},
  {"tri", "cula"},
  {"ven", "nofo"},
  {"word of", "inset"},
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
  {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
  {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};

const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */

int mag_manacost(struct char_data * ch, int spellnum)
{
  int mana = MAX(SINFO.mana_max[(int) GET_CLASS(ch)] - 
	     (SINFO.mana_change[(int) GET_CLASS(ch)] *
		    (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
	     SINFO.mana_min[(int) GET_CLASS(ch)]);

  // DM - mana percentage - Artus: Modified, Div0 Bug.
  if (SINFO.mana_perc[(int)GET_CLASS(ch)] > 0)
    mana = (int)((100 * mana) / SINFO.mana_perc[(int)GET_CLASS(ch)]);
  return (mana);
}


/* say_spell erodes buf, buf1, buf2 */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch,
	            struct obj_data * tobj, int chargeamt)
{
  char lbuf[256];
  const char *format;
  int concealed = 0;
  struct char_data *i;
  int j, ofs = 0;

  *buf = '\0';
  strcpy(lbuf, skill_name(spellnum));

  while (lbuf[ofs]) 
  {
    for (j = 0; *(syls[j].org); j++) 
    {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) 
      {
	strcat(buf, syls[j].news);
	ofs += strlen(syls[j].org);
        break;
      }
    }
    if (!*syls[j].org) 
    {
      basic_mud_log("No entry in syllable table for substring of '%s'", lbuf);
      ofs++;
    }
  }

  // Generate the strings, one with clean spell name, one with 'magic words'.
  if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch)) 
  {
    if (tch == ch)
      format = "$n closes $s eyes and utters the words, '%s'.";
    else
      format = "$n stares at $N and utters the words, '%s'.";
  } else if (tobj != NULL &&
	     ((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
    format = "$n stares at $p and utters the words, '%s'.";
  else
    format = "$n utters the words, '%s'.";
  sprintf(buf1, format, skill_name(spellnum));
  sprintf(buf2, format, buf);

  // Are we concealing? -- Artus.
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_CONCEAL_SPELL_CASTING) &&
      (number(1, 101) < GET_SKILL(ch, SKILL_CONCEAL_SPELL_CASTING)))
  {
    send_to_char("You manage to conceal your spell casting.\r\n", ch);
    concealed = 1;
  }
  // Send the message to those in the room.
  for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
  {
    if ((i==ch) || (i==tch) || (!i->desc) || !AWAKE(i) || 
	((concealed==1) && LR_FAIL(i, LVL_IS_GOD)))
      continue;
    if (GET_SKILL(i, spellnum) > 0)
      perform_act(buf1, ch, tobj, tch, i);
    else
      perform_act(buf2, ch, tobj, tch, i);
  }
  // Now, Handle the Victim, if there is one. -- Artus.
  if ((tch != NULL) && (tch != ch) && (tch->desc) && AWAKE(tch) &&
      ((concealed == 0) || !LR_FAIL(tch, LVL_IS_GOD)))
  {
    if (chargeamt > 0)
      sprintf(buf1, "$n takes your money, stares at you, and utters the words, '%s'.", (GET_SKILL(tch, spellnum) > 0) ? skill_name(spellnum) : buf);
    else
      sprintf(buf1, "$n stares at you and utters the words, '%s'.",
              (GET_SKILL(tch, spellnum) > 0) ? skill_name(spellnum) : buf);
    perform_act(buf1, ch, NULL, tch, tch); 
  }
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
const char *skill_name(int num)
{
  if (num > 0 && num <= TOP_SPELL_DEFINE)
    return (spell_info[num].name);
  else if (num == -1)
    return ("UNUSED");
  else
    return ("UNDEFINED");
}

	 
int find_skill_num(char *name)
{
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256];

  for (index = 1; index <= TOP_SPELL_DEFINE; index++) {
    if (is_abbrev(name, spell_info[index].name))
      return (index);

    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg((char *)spell_info[index].name, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
	ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return (index);
  }

  return (-1);
}

/* Artus - Basic skill/spell test... */
bool basic_skill_test(struct char_data * ch, int spellnum, bool perform)
{
  if IS_NPC(ch) 
    return 0;

  if (!GET_SKILL(ch, spellnum)) {
    switch (spellnum) {
      case SKILL_AXEMASTERY:
      case SKILL_PERCEPTION:
	break;
      default:
	if (perform)
          send_to_char("You don't know how.\r\n", ch);
	break;
    }
    return 0;
  }

  if (!has_stats_for_skill(ch, spellnum, perform))
    return 0;

  if (perform) 
  {
    // DM - concentration loss formula - 
    if (number(0, 101) > GET_SKILL(ch, spellnum))
    {
      if (spellnum < MAX_SPELLS) 
        send_to_char("You lost your concentration.\r\n", ch);
      else
      {
	switch (spellnum)
	{
	  case SKILL_LISTEN: 
	    send_to_char("You can't hear anything.\r\n", ch);
	    break;
	  case SKILL_AXEMASTERY:
	  case SKILL_PERCEPTION:
	    break;
	  default:
            send_to_char("You fail.\r\n", ch);
	}
      }
      return 0;
    }
  }
  
  if (perform)
    apply_spell_skill_abil(ch, spellnum);

  return 1;
}
  
/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(struct char_data * caster, struct char_data * cvict,
	     struct obj_data * ovict, int spellnum, int level, int casttype)
{
  int savetype;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
    return (0);

  if (IS_NPC(caster) || LR_FAIL(caster, LVL_IMPL))
  {
    if (caster->nr != real_mobile(DG_CASTER_PROXY)) 
    {
      if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) &&
	  (SINFO.violent || (casttype != CAST_MAGIC_OBJ)))
      {
	send_to_char("Your magic fizzles out and dies.\r\n", caster);
	act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
	return (0);
      }
      if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
          (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) 
      {
	send_to_char("A flash of white light fills the room, dispelling your "
		     "violent magic!\r\n", caster);
	act("White light from no particular source suddenly fills the room, "
	    "then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
	return (0);
      }
    }
  }
  /*
  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC)) {
    send_to_char("Your magic fizzles out and dies.\r\n", caster);
    act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
    send_to_char("A flash of white light fills the room, dispelling your "
		 "violent magic!\r\n", caster);
    act("White light from no particular source suddenly fills the room, "
	"then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
    return (0);
  }
  */

  /* determine the type of saving throw */
  switch (casttype)
  {
    case CAST_STAFF:
    case CAST_SCROLL:
    case CAST_POTION:
    case CAST_WAND:
      savetype = SAVING_ROD;
      break;
    case CAST_SPELL:
      savetype = SAVING_SPELL;
      break;
    default:
      savetype = SAVING_BREATH;
      break;
  }

  if (IS_SET(SINFO.routines, MAG_DAMAGE))
    if (mag_damage(level, caster, cvict, spellnum, savetype) == -1)
      return (-1);	/* Successful and target died, don't cast again. */

  if (IS_SET(SINFO.routines, MAG_AFFECTS))
    mag_affects(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
    mag_unaffects(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_POINTS))
    mag_points(level, caster, cvict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
    mag_alter_objs(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_GROUPS))
    mag_groups(level, caster, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_MASSES))
    mag_masses(level, caster, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_AREAS))
    mag_areas(level, caster, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_SUMMONS))
    mag_summons(level, caster, ovict, spellnum, savetype);

  if (IS_SET(SINFO.routines, MAG_CREATIONS))
    mag_creations(level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum)
    {
      case SPELL_CHARM:		   MANUAL_SPELL(spell_charm);            break;
      case SPELL_CREATE_WATER:	   MANUAL_SPELL(spell_create_water);     break;
      case SPELL_DETECT_POISON:	   MANUAL_SPELL(spell_detect_poison);    break;
      case SPELL_ENCHANT_WEAPON:   MANUAL_SPELL(spell_enchant_weapon);   break;
      case SPELL_IDENTIFY:	   MANUAL_SPELL(spell_identify);         break;
      case SPELL_LOCATE_OBJECT:    MANUAL_SPELL(spell_locate_object);    break;
      case SPELL_SUMMON:	   MANUAL_SPELL(spell_summon);           break;
      case SPELL_WORD_OF_RECALL:   MANUAL_SPELL(spell_recall);           break;
      case SPELL_TELEPORT:	   MANUAL_SPELL(spell_teleport);         break;
      case SPELL_FINGERDEATH:	   MANUAL_SPELL(spell_fingerdeath);      break;
      case SPELL_CONTROL_WEATHER:  MANUAL_SPELL(spell_control_weather);  break;
      case SPELL_FEAR: 		   MANUAL_SPELL(spell_fear);             break;
      case SPELL_GATE: 		   MANUAL_SPELL(spell_gate);             break;
      case SPELL_UNHOLY_VENGEANCE: MANUAL_SPELL(spell_unholy_vengeance); break;
    }

  // Apply ability gains - DM
  if (casttype == CAST_SPELL)
    apply_spell_skill_abil(caster, spellnum);

  return (1);
}

/*
 * Apply ability gains based on current spell/skill ability
 * This function should be called on every successful use of a spell/skill.
 */
void apply_spell_skill_abil(struct char_data *ch, int spellnum) {

  int ability=0;
  int chance;
  int roll;

  if (IS_NPC(ch))
    return;

  switch (spellnum)
  {
    case SKILL_TRACK:
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_TRACKER))
	ability = MAX(1, GET_SKILL(ch, spellnum));
      break;
    case SKILL_BACKSTAB:
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_BACKSTAB))
	ability = MAX(1, GET_SKILL(ch, spellnum));
    default:
      ability = GET_SKILL(ch, spellnum);
  }

  if ((ability < 1) || (ability >= MAX_SKILL_ABIL))
    return;

  chance = MAX(1, (int)((2 * ability)/3));
  roll = number(1, chance);

  // Chance of 2/3 * current skill level for char to increase ability  
  if (roll == 1) 
  {
    GET_SKILL(ch, spellnum) += 1;
    switch (spellnum) {
      case SKILL_PERCEPTION:
	break;
      default:
	if (ability >= MAX_SKILL_ABIL)
	  sprintf(buf, "&0You are now fully learend in %s!&n\r\n", 
	          spell_info[spellnum].name);
	else
	  sprintf(buf, "&0Your ability in using %s increases.&n\r\n", 
			spell_info[spellnum].name);
        send_to_char(buf, ch);
    }
  }
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */

void mag_objectmagic(struct char_data * ch, struct obj_data * obj,
		          char *argument)
{
  int i, k;
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  if (!*arg)
  {
    tch = ch;
    k = FIND_CHAR_ROOM;
  }
  else
    k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		     FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj))
  {
    case ITEM_STAFF:
      if (k==0)
      {
	send_to_char("Could not find the requested target.\r\n", ch);
	return;
      }
      act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
      else
	act("$n taps $p on the ground three times.", FALSE, ch, obj, 0, 
	    TO_ROOM);
      if (GET_OBJ_VAL(obj, 2) <= 0)
      {
	send_to_char("It seems powerless.\r\n", ch);
	act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
	return;
      }
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), k, CAST_WAND);
      break;
    case ITEM_WAND:
      if (k == FIND_CHAR_ROOM)
      {
	if (tch == ch)
	{
	  act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
	} else {
	  act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
	  if (obj->action_description)
	    act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
	  else
	    act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
	}
      } else if (tobj != NULL) {
	act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
	if (obj->action_description)
	  act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
	else
	  act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
      } else if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES)) {
	/* Wands with area spells don't need to be pointed. */
	act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
	act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
      } else {
	act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
	return;
      }
      if (GET_OBJ_VAL(obj, 2) <= 0)
      {
	send_to_char("It seems powerless.\r\n", ch);
	act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
	return;
      }
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      if (GET_OBJ_VAL(obj, 0))
	call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		   GET_OBJ_VAL(obj, 0), CAST_WAND);
      else
	call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		   DEFAULT_WAND_LVL, CAST_WAND);
      break;
    case ITEM_SCROLL:
      if (*arg)
      {
	if (!k)
	{
	  act("There is nothing to here to affect with $p.", FALSE,
	      ch, obj, NULL, TO_CHAR);
	  return;
	}
      } else
	tch = ch;
      act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
      else
	act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);
      WAIT_STATE(ch, PULSE_VIOLENCE);
      for (i = 1; i <= 3; i++)
	if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
			 GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
	  break;

      if (obj != NULL)
	extract_obj(obj);
      break;
    case ITEM_POTION:
      tch = ch;
      act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
      if (obj->action_description)
	act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
      else
	act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);

      /* Artus> 20031013 - 6 Potions/Round if Adrenaline, else 5. */
      if (IS_AFFECTED(ch, AFF_ADRENALINE))
	WAIT_STATE(ch, (PULSE_VIOLENCE / 6));
      else
	WAIT_STATE(ch, (PULSE_VIOLENCE / 5));
      for (i = 1; i <= 3; i++)
	if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
                       GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
	  break;
      if (obj != NULL)
	extract_obj(obj);
      break;
    default:
      basic_mud_log("SYSERR: Unknown object_type %d in mag_objectmagic.",
	GET_OBJ_TYPE(obj));
    break;
  }
}

// Artus> handle_spell_payment... For SPECIAL_PRIEST.
int handle_spell_payment(struct char_data *ch, struct char_data *tch, 
                          int spellnum)
{
  int amt = 0;

  if (GET_GOLD(tch) < 1) 
    return 0;

  switch (spellnum)
  {
    case SPELL_ARMOR: 
    case SPELL_BLESS:
    case SPELL_DETECT_MAGIC:
    case SPELL_DETECT_POISON:
    case SPELL_DIVINE_PROTECTION:
    case SPELL_DRAGON:
    case SPELL_GREATER_REMOVE_CURSE:
    case SPELL_HOLY_AID:
    case SPELL_STRENGTH:
    case SPELL_SPIRIT_ARMOR:
    case SPELL_STONESKIN:
      // Don't charge them if they already had it.
      if (affected_by_spell(tch, spellnum))
	return 0;
      break;
    case SPELL_ADV_HEAL:
    case SPELL_CURE_CRITIC:
    case SPELL_CURE_LIGHT:
    case SPELL_DIVINE_HEAL:
    case SPELL_HEAL:
      // Don't charge them if they were already maxed out.
      if (GET_HIT(tch) >= GET_MAX_HIT(tch))
	return 0;
      break;
      // Special Cases
    case SPELL_CURE_BLIND:
      if (AFF_FLAGGED(tch, AFF_BLIND))
	return 0;
      break;
    case SPELL_DETECT_ALIGN:
      if (AFF_FLAGGED(tch, AFF_DETECT_ALIGN))
	return 0;
      break;
    case SPELL_DETECT_INVIS:
      if (AFF_FLAGGED(tch, AFF_DETECT_INVIS))
	return 0;
      break;
    case SPELL_FLY:
      if (AFF_FLAGGED(tch, AFF_FLY))
	return 0;
      break;
    case SPELL_HASTE:
      if (AFF_FLAGGED(tch, AFF_HASTE))
	return 0;
      break;
    case SPELL_INFRAVISION:
      if (AFF_FLAGGED(tch, AFF_INFRAVISION))
	return 0;
      break;
    case SPELL_INVISIBLE:
      if (AFF_FLAGGED(tch, AFF_INVISIBLE | AFF_ADVANCED_INVIS))
	return 0;
      break;
    case SPELL_PROT_FROM_EVIL:
      if (AFF_FLAGGED(tch, AFF_PROTECT_EVIL))
	return 0;
      break;
    case SPELL_PROT_FROM_GOOD:
      if (AFF_FLAGGED(tch, AFF_PROTECT_GOOD))
	return 0;
      break;
    case SPELL_REFRESH:
      if (GET_MOVE(tch) >= GET_MAX_MOVE(tch))
	return 0;
      break;
    case SPELL_REMOVE_CURSE:
      if (AFF_FLAGGED(tch, AFF_CURSE))
	return 0;
      break;
    case SPELL_REMOVE_PARA:
      if (!(affected_by_spell(ch, SPELL_PARALYZE)))
	return 0;
      break;
    case SPELL_REMOVE_POISON:
      if (!(affected_by_spell(tch, SPELL_POISON)))
	return 0;
      break;
    case SPELL_SANCTUARY:
      if (AFF_FLAGGED(tch, AFF_SANCTUARY))
	return 0;
      break;
    case SPELL_SENSE_LIFE:
      if (AFF_FLAGGED(tch, AFF_SENSE_LIFE))
	return 0;
      break;
    case SPELL_SENSE_WOUNDS:
      if (AFF_FLAGGED(tch, AFF_SENSE_WOUNDS))
	return 0;
      break;
    case SPELL_SERPENT_SKIN:
      if (AFF_FLAGGED(tch, AFF_REFLECT))
	return 0;
      break;
    case SPELL_SUMMON:
      if (IN_ROOM(ch) == IN_ROOM(tch))
	return 0;
      break;
    case SPELL_WATERWALK:
      if (AFF_FLAGGED(tch, AFF_WATERWALK | AFF_WATERBREATHE))
	return 0;
      break;
    case SPELL_WATERBREATHE:
      if (AFF_FLAGGED(tch, AFF_WATERBREATHE))
	return 0;
      break;
      // Lightning Shield / Fire Shield / Fire Wall are all intertwined.
    case SPELL_LIGHT_SHIELD:
    case SPELL_FIRE_SHIELD:
      if (!GET_EQ(tch, WEAR_SHIELD))
	return 0;
    case SPELL_FIRE_WALL:
      if (affected_by_spell(tch, SPELL_FIRE_WALL) ||
	  affected_by_spell(tch, SPELL_FIRE_SHIELD) ||
	  affected_by_spell(tch, SPELL_LIGHT_SHIELD))
	return 0;
      break;
    default: // Anything that we're not handling doesn't get charged.:
      return 0;
  }
  // If we got this far, we know we're going to charge.
  amt = MIN(GET_LEVEL(tch) * 100, mag_manacost(ch, spellnum) * MAX(1, GET_LEVEL(ch) - SINFO.min_level[(int)GET_CLASS(ch)]));
#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: Mana: %d, Min Level: %d, Amt: %d\r\n", 
	    mag_manacost(ch, spellnum), SINFO.min_level[(int)GET_CLASS(ch)],
	    amt);
    send_to_char(buf, ch);
  }
#endif
  return MIN(GET_GOLD(tch), amt);
}

/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */

int cast_spell(struct char_data * ch, struct char_data * tch,
	           struct obj_data * tobj, int spellnum)
{
  if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE)
  {
    basic_mud_log("SYSERR: cast_spell trying to call spellnum %d/%d.\n", spellnum,
	TOP_SPELL_DEFINE);
    return (0);
  }
    
  if (GET_POS(ch) < SINFO.min_position) {
    switch (GET_POS(ch)) {
      case POS_SLEEPING:
      send_to_char("You dream about great magical powers.\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("You cannot concentrate while resting.\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("You can't do this sitting!\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("Impossible!  You can't concentrate enough!\r\n", ch);
      break;
    default:
      send_to_char("You can't do much of anything like this!\r\n", ch);
      break;
    }
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char("You are afraid you might hurt your master!\r\n", ch);
    return (0);
  }
  if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY)) {
    send_to_char("You can only cast this spell upon yourself!\r\n", ch);
    return (0);
  }
  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
    send_to_char("You cannot cast this spell upon yourself!\r\n", ch);
    return (0);
  }
  if (IS_SET(SINFO.routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("You can't cast this spell if you're not in a group!\r\n",ch);
    return (0);
  }
  // Redundant when coming from do_cast, but not so in other cases.
  if (SINFO.violent && !violence_check(ch, tch, spellnum))
    return (0);

  send_to_char(OK, ch);

  if (!IS_NPC(ch) && (IS_SET(GET_SPECIALS(ch), SPECIAL_PRIEST)) && (tch) &&
      (ch != tch) && !IS_NPC(tch))
  {
    int retval = 0;
    int chargeamt = handle_spell_payment(ch, tch, spellnum);
    say_spell(ch, spellnum, tch, tobj, chargeamt);
    retval = call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch), CAST_SPELL);
    if (retval > 0)
    {
      GET_GOLD(tch) -= chargeamt;
      GET_GOLD(ch) += chargeamt;
    }
    return retval;
  }
  say_spell(ch, spellnum, tch, tobj, 0);
  return call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch), CAST_SPELL);
}


/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */

ACMD(do_cast)
{
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char *s, *t;
  int mana, spellnum, target = 0;

  if (IS_NPC(ch))
    return;

  /* get: blank, spell name, target name */
  s = strtok(argument, "'");

  if (s == NULL)
  {
    send_to_char("Cast what where?\r\n", ch);
    return;
  }
  s = strtok(NULL, "'");
  if (s == NULL)
  {
    send_to_char("Spell names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
    return;
  }
  t = strtok(NULL, "\0");

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum > MAX_SPELLS))
  {
    send_to_char("Cast what?!?\r\n", ch);
    return;
  }

  /* check min stats required */
  if (!has_stats_for_skill(ch, spellnum, TRUE))
    return;
 
  /* Berserk - No spells. */
  if (IS_AFFECTED(ch, AFF_BERSERK))
  {
    send_to_char("You can't cast spells while berserk.\r\n", ch);
    return;
  }

  /* Chance of failng whilest dual wielding.. -- Artus */
  if (IS_DUAL_WIELDING(ch) && LR_FAIL(ch, LVL_IMPL))
    if (number(0, 18) > GET_DEX(ch))
    {
      send_to_char("Having your hands full encumbers your ability to perform the incantations!\r\n", ch);
      return;
    }

  /* Check to see if the caster is using a shield for lightning/fire shield - DM */
  if ((spellnum == SPELL_LIGHT_SHIELD) || (spellnum == SPELL_FIRE_SHIELD))
  {
    if (!GET_EQ(ch,WEAR_SHIELD))
    {
      send_to_char("You need to be using a shield to cast this.\r\n",ch);
      return;
    }
  }
 
  /* Check for alignment and existing protection for protection from evil/good - DM */
  if ((spellnum == SPELL_PROT_FROM_EVIL) || (spellnum == SPELL_PROT_FROM_GOOD))
  {
    if (affected_by_spell(ch,SPELL_PROT_FROM_EVIL) ||
                affected_by_spell(ch,SPELL_PROT_FROM_GOOD))
    {
       send_to_char("You already feel invulnerable.\r\n",ch);
       return;
    }
 
    /* Check the required aligns are right */
    if (spellnum == SPELL_PROT_FROM_EVIL)
    {
      if (!IS_GOOD(ch))
      {
        send_to_char("If only you were a nicer person.\r\n",ch);
        return;
      }
    } else {
      if (!IS_EVIL(ch))
      {
        send_to_char("If only you were meaner.\r\n",ch);
        return;
      }
    }
  }

  /* Find the target */
  if (t != NULL)
  {
    one_argument(strcpy(arg, t), t);
    skip_spaces(&t);
  }

  //sprintf(buf,"targets : %d\r\n", SINFO.targets);
  //send_to_char(buf,ch);

  if (IS_SET(SINFO.targets, TAR_IGNORE))
    target = TRUE;
  else if (t != NULL && *t)
  {
    int locatebits = 0;
    if (IS_SET(SINFO.targets, TAR_CHAR_WORLD))
      SET_BIT(locatebits, FIND_CHAR_ROOM | FIND_CHAR_WORLD);
    else if (IS_SET(SINFO.targets, TAR_CHAR_INWORLD))
      SET_BIT(locatebits, FIND_CHAR_ROOM | FIND_CHAR_INWORLD);
    else if (IS_SET(SINFO.targets, TAR_CHAR_ROOM))
      SET_BIT(locatebits, FIND_CHAR_ROOM);
    if (IS_SET(SINFO.targets, TAR_OBJ_INV))
      SET_BIT(locatebits, FIND_OBJ_INV);
    if (IS_SET(SINFO.targets, TAR_OBJ_EQUIP))
      SET_BIT(locatebits, FIND_OBJ_EQUIP);
    if (IS_SET(SINFO.targets, TAR_OBJ_WORLD))
      SET_BIT(locatebits, FIND_OBJ_ROOM | FIND_OBJ_WORLD);
    else if (IS_SET(SINFO.targets, TAR_OBJ_ROOM))
      SET_BIT(locatebits, FIND_OBJ_ROOM);
    if (generic_find(t, locatebits, ch, &tch, &tobj) != 0)
      target = true;
  } else {			/* if target string is empty */
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL)
      {
	tch = ch;
	target = TRUE;
      }
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL)
      {
	tch = FIGHTING(ch);
	target = TRUE;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
	!SINFO.violent)
    {
      tch = ch;
      target = TRUE;
    }
    if (!target)
    {
      sprintf(buf, "Upon %s should the spell be cast?\r\n",
	 IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? ((IS_SET(SINFO.targets, TAR_CHAR_ROOM | TAR_CHAR_WORLD | TAR_CHAR_INWORLD)) ? "who/what" : "what") : "who");
      send_to_char(buf, ch);
      return;
    }
  }
  
  switch (spellnum) // Any special checks.
  { 
    case SPELL_HASTE: // Restrict Victim Level.
    case SPELL_SERPENT_SKIN:
      if (target && tch && (tch != ch) && !IS_NPC(ch) && 
	  LR_FAIL(tch, LVL_NEWBIE) && LR_FAIL(ch, LVL_IMPL))
      {
	sprintf(buf, "Perhaps you should wait until &7%s&n is a little more experienced..\r\n", HSSH(tch));
	send_to_char(buf, ch);
	return;
      }
      break;
  }

  // Artus> Violence Checks.
  if ((SINFO.violent) && (!violence_check(ch, tch, spellnum)))
    return;

  if (target && (tch == ch) && SINFO.violent)
  {
    send_to_char("You shouldn't cast that on yourself -- could be bad for your health!\r\n", ch);
    return;
  }
  if (!target)
  {
    send_to_char("Cannot find the target of your spell!\r\n", ch);
    return;
  }
#ifndef IGNORE_DEBUG
  else {
    if (GET_DEBUG(ch))
    {
      sprintf(buf, "DEBUG: target is (%s)\r\n", (tch) ? GET_NAME(tch) : "None");
      send_to_char(buf, ch);
    }
  }
#endif
  mana = mag_manacost(ch, spellnum);
  if ((mana > 0) && (GET_MANA(ch) < mana) && (LR_FAIL(ch, LVL_IS_GOD)))
  {
    send_to_char("You haven't the energy to cast that spell!\r\n", ch);
    return;
  }

  // Check MAGIC flags before loosing concentration
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC) && LR_FAIL(ch, LVL_IMPL))
  {
    send_to_char("Your magic fizzles out and dies.\r\n", ch);
    //act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    return;
  }
  
  /* You throws the dice and you takes your chances.. 101% is total failure */
  // DM - lost concentration formula
  if (number(0, 101) > GET_SKILL(ch, spellnum))
  {
    WAIT_STATE(ch, (int)PULSE_VIOLENCE/2);
    if (!tch || !skill_message(0, ch, tch, spellnum))
      send_to_char("You lost your concentration!\r\n", ch);
    if (mana > 0)
      GET_MANA(ch) = MAX(0, GET_MANA(ch)-(mana/2));
//      GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana / 2)));
    if (SINFO.violent && tch && IS_NPC(tch) && AWAKE(tch) && 
	CAN_SEE(tch, ch))
/*  Artus> Removed: Violence check already performed.
 *    if (violence_check(ch, tch, spellnum)) */
        hit(tch, ch, TYPE_UNDEFINED);
  } else { /* cast spell returns 1 on success; subtract mana & set waitstate */
    if (cast_spell(ch, tch, tobj, spellnum))
    {
      // Priests charge for their services
#if 0 // Artus> Moved to say_spell..
      if ((GET_CLASS(ch) == CLASS_PRIEST) && (tch != ch) && !IS_NPC(tch))
      {
        spell_cost = (int) (mana * GET_MODIFIER(ch) * GET_LEVEL(ch));
        if((GET_GOLD(tch) + GET_BANK_GOLD(tch)) < spell_cost)
	{
          send_to_char("They dont have enough money to pay for your services!\r\n", ch);
          return;
        }
        GET_GOLD(tch) -= spell_cost;
        if (GET_GOLD(tch) < 0)
	{
           GET_BANK_GOLD(tch) -= GET_GOLD(tch);
           GET_GOLD(tch) = 0;
        }
      }
#endif

      WAIT_STATE(ch, PULSE_VIOLENCE);
      if (mana > 0)
	GET_MANA(ch) = MAX(0, GET_MANA(ch) - mana);
//	GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
    }
  }
}

void list_fighters_to_char(struct char_data *ch, int skillnum)
{
  struct char_data *c;
  int nFound = 0;

  for(c = world[ch->in_room].people; c; c = c->next) {
    if (FIGHTING(c) == ch) {
      nFound++;
      if (nFound == 1)
        sprintf(buf, "You are fighting %s%s&n", IS_NPC(c) ? "&6" : "&7", 
                     GET_NAME(c));
      else
        sprintf(buf + strlen(buf), "&n, %s%s", IS_NPC(c) ? "&6" : "&7", 
                     GET_NAME(c));
    }
  }

  if (nFound != 0)
    sprintf(buf + strlen(buf), "&n.\r\n");
  else
    sprintf(buf, "%s whom?\r\n", 
                 (skillnum == SCMD_HIT) ? "Hit" : 
                 ((skillnum == SCMD_MURDER) ? "Murder" : "Kill"));

  send_to_char(buf, ch);
}

/*
 * This functin is used by all offensive skills to check the common situations
 * which apply when using each skill.
 *
 * Returns TRUE if ok to continue, FALSE otherwise.
 * 
 * was going to be a todo: add another argument - bool to display or not 
 * display verbal shit - think its covered now in basic_skill_test
 */
bool violence_check(struct char_data *ch, struct char_data *vict, int skillnum)
{
  extern struct index_data *mob_index;
  extern struct zone_data *zone_table;	
  extern char pk_allowed;
  bool imm = FALSE;

  // Artus> Impl+ get certain rights :o)
  if (!IS_NPC(ch) && !LR_FAIL(ch, LVL_IMPL))
    imm = TRUE;

  /* Artus> Moved this above !vict check, Prevents casting MAG_AREA spells
   *        in peaceful rooms. */
  // peaceful rooms.
  if ((!imm) && 
      (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) &&
      (!IS_NPC(ch) || ch->nr != real_mobile(DG_CASTER_PROXY)))
  {
    GET_WAIT_STATE(ch) = 0;
    send_to_char(PEACEROOM, ch);
    return FALSE;
  }
 
  // No victim - continue ...
  if (vict == NULL)
    return TRUE;

  // Victim is not in Ch's room, make sure their room is !peaceful.
  if ((IN_ROOM(ch) != IN_ROOM(vict)) && !imm && 
      (ROOM_FLAGGED(IN_ROOM(vict), ROOM_PEACEFUL)) &&
      (!IS_NPC(ch) || ch->nr != real_mobile(DG_CASTER_PROXY)))
  {
    GET_WAIT_STATE(ch) = 0;
    sprintf(buf, "The room %s is in seems way too peaceful.\r\n", HSSH(vict));
    send_to_char(buf, ch);
    return FALSE;
  }

  // Moved from damage(): Should never happen...
  if (GET_POS(vict) <= POS_DEAD) 
  {
    basic_mud_log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
        GET_NAME(vict), GET_ROOM_VNUM(IN_ROOM(vict)), GET_NAME(ch));
    die(vict,ch);
    return FALSE;		/* -je, 7/7/92 */
  }

  // attacking self?
  if (vict == ch) 
  {
    send_to_char("Go inflict some pain on yourself elsewhere...\r\n", ch);
    return FALSE;
  } 

  if (IS_NPC(vict))
  {
    /* Moved from damage(): shopkeeper protection */
    if (!ok_damage_shopkeeper(ch, vict))
      return (0);

    // !KILL Flag.
    if (!imm && MOB_FLAGGED(vict,MOB_NOKILL))
    {
      sprintf(buf, "You cannot possibly harm %s.\r\n", HMHR(vict));
      send_to_char(buf, ch);
      return FALSE;
    }

    // Moved from damage(): Quest Mobs.
    if (!imm && MOB_FLAGGED(vict,MOB_QUEST) && !PRF_FLAGGED(ch, PRF_QUEST))
    {
      send_to_char("Sorry, they are part of a quest.\r\n",ch);
      return FALSE;
    } 

    // postmaster protection
    if (!imm && (GET_MOB_SPEC(vict) == postmaster)) 
    {
      send_to_char("You cannot attack the postmaster!!\r\n", ch);
      return FALSE;
    } 

    // receptionist protection
    if (!imm && (GET_MOB_SPEC(vict) == receptionist))
    {
      send_to_char("You cannot attack the Receptionist!\r\n", ch); 
      return FALSE;
    }
  }

  // attacking master?
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict)) 
  {
    act("$N is just such a good friend, you simply can't hurt $M.", 
                    FALSE, ch, 0, vict, TO_CHAR);
    return FALSE;
  }   

  // ARTUS - PK.. Don't need to return false just yet, we'll let the original
  // pk code do that for us... We'll take care of mortal kombat here too.
  if (!IS_NPC(ch) && !IS_NPC(vict)) 
  {
    if (((GET_CLAN(ch) || EXT_FLAGGED(ch, EXT_PKILL)) && 
	 (GET_CLAN(vict) || EXT_FLAGGED(vict, EXT_PKILL))) ||
	(PRF_FLAGGED(ch, PRF_MORTALK) && PRF_FLAGGED(vict, PRF_MORTALK)))
      return TRUE;
  }

  // mounts
  if (IS_NPC(vict) && MOUNTING(vict)) 
  {
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch))
    {
      sprintf(buf, "DEBUG: Ch: %s Vict: %s Mounting(ch): %s\r\n",
	  GET_NAME(ch), GET_NAME(vict), GET_NAME(MOUNTING(vict)));
      send_to_char(buf, ch);
    }
#endif
    if (MOUNTING(vict) == ch) 
    {
      send_to_char("You can't attack something you are mounting!\r\n", ch);
      return FALSE;
    }
    // Moved from damage(): Cannot attack rider.
    if (vict == MOUNTING(ch) && IS_NPC(ch))
    {
      send_to_char("Your rider controls your urge to kill them.\r\n", ch);
      return FALSE;
    }
    if ((GET_CLAN(MOUNTING(vict)) || EXT_FLAGGED(MOUNTING(vict), EXT_PKILL)) &&
      (GET_CLAN(ch) || EXT_FLAGGED(ch, EXT_PKILL)))
        return TRUE;
  } 

/*  if (IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict)) {
    send_to_char("That's someone's mount! Use 'murder' to attack another "
                 "player.\r\n", ch);
    return FALSE;
  } */

  /* Artus - Pkill Flag/Clan Checking... Looks redundant. 20031103
  if (!IS_NPC(ch) && !IS_NPC(vict))
  {
    if (((GET_CLAN(ch) > 0) || (EXT_FLAGGED(ch, EXT_PKILL))) &&
        ((GET_CLAN(vict) > 0) || (EXT_FLAGGED(vict, EXT_PKILL))))
      return TRUE;
  } */

  // pk zone check
  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag, ZN_PK_ALLOWED) && 
                  !IS_NPC(vict) && !IS_NPC(ch) && !imm) 
  {
    send_to_char("Player Killing is not allowed in this Zone.\r\n", ch);
    act("$n just tried to attack $N, and failed miserably!", 
                    FALSE, ch, 0, vict, TO_NOTVICT);
    return FALSE;
  }

  if (!imm && !pk_allowed) 
  {
    if (!IS_NPC(vict) && !IS_NPC(ch) && skillnum != SCMD_MURDER &&
	skillnum != SPELL_CHANGED) {
      send_to_char("Use 'murder' to attack another player.\r\n", ch);
      return FALSE;
    }

    /* you can't order a charmed pet to attack a player */
    if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
      return FALSE;                 
  }

  return TRUE;
}

/* 
 * Similar to do_cast, this function is the entry point for violient PC-used 
 * skills ... 
 */
ACMD(do_violent_skill) 
{
  int skillnum = subcmd;

  struct char_data *vict = NULL;
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  extern struct index_data *mob_index;
  byte percent, prob;
  struct char_data *next_vict;
  sh_int room;
  int door, dam, skip = 0;
  bool performed = FALSE;

  struct obj_data *boots;
  int weight = 0, type = SKILL_KICK;
  int calc_dam_amt(struct char_data *ch, struct char_data *vict, int skillnum);
  int char_can_enter(struct char_data *ch, struct room_data *room, bool show);

  // Check that no mounted char is trying to do an attack by itself
/*  if (IS_NPC(ch) && MOUNTING(ch) && !FIGHTING(MOUNTING(ch))) {
    return;
  } */

  // Only let PC hit and kill commands through
  if (IS_NPC(ch) && !(skillnum == SCMD_HIT || skillnum == SCMD_KILL))
  {
    basic_mud_log("SYSERR: NPC (%s) using skill (%d)", GET_NAME(ch), skillnum);
    return;
  }

  // Check if the char is a ghost.
  if (!IS_NPC(ch))
  {
    if (IS_GHOST(ch))
    {
      send_to_char("As a ghost, you find yourself unable to harm others.\r\n", 
	           ch);
      return;
    }

    // Check the chars skill ability 
    switch (skillnum)
    {
      case SCMD_HIT:
      case SCMD_MURDER:
      case SCMD_KILL:
	break;
      case SKILL_BACKSTAB:
	if (IS_SET(GET_SPECIALS(ch), SPECIAL_BACKSTAB))
	  break;
      default:
	if (!has_stats_for_skill(ch, skillnum, TRUE))
	  return;
    } // Skillnum.
  } // !IS_NPC

  one_argument(argument, arg);
  vict = generic_find_char(ch, arg, FIND_CHAR_ROOM);

  switch (skillnum)
  {
   
    // ********* MURDER, HIT, KILL (the abnormal defines) *********************
    case SCMD_MURDER:
    case SCMD_HIT:
    case SCMD_KILL:
      if (!*arg)
      {
        list_fighters_to_char(ch, skillnum);
        return;
      } else if (vict == NULL) {
        send_to_char("They don't seem to be here.\r\n", ch);
        return;
      }
      // Implementor kill ....
      if (skillnum == SCMD_KILL && !IS_NPC(ch) && !LR_FAIL(ch, LVL_IMPL))
      {
        act("You chop $M to pieces!  Ah!  The &rblood&n!", 
	    FALSE, ch, 0, vict, TO_CHAR);
        act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
        act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        raw_kill(vict,ch);
        return;
      }
      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;
      // Was in do_hit - wondering why atm - should move into violence_check
      if (IS_AFFECTED(ch,AFF_CHARM) && !ch->master)
        REMOVE_BIT(AFF_FLAGS(ch),AFF_CHARM);
      // ----------- Not fighting ---------- target != fighting ----
      if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
      {
        if (wielded && OBJ_IS_GUN(wielded))
	{
          send_to_char("You would do better to shoot this weapon!\n\r", ch);
          return;
        }
        // Set killer flags etc ...
        if (skillnum == SCMD_MURDER)
          check_killer(ch, vict);
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE + 2;
        hit(ch, vict, TYPE_UNDEFINED);
        // Order mount to attack 
        if (!IS_NPC(ch) && MOUNTING(ch) && !FIGHTING(MOUNTING(ch)))
	{
          send_to_char("You order your mount to attack!\r\n", ch);
          act("$n orders $s mount to attack.\r\n", FALSE, ch, 0, 0, TO_ROOM);
          do_violent_skill(MOUNTING(ch), arg, 0, SCMD_HIT);
        }
      // -- ch is fighting, check for switching  -----------------------
      } else if (FIGHTING(vict) != ch) { // vict isn't fighting ch - cant switch
        send_to_char("You're already fighting someone!\r\n", ch);
      // -- Switch player target ---------------------------------------
      } else if (FIGHTING(vict) == ch && FIGHTING(ch) != vict) {
        act("You turn to fight $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n turns to fight $N.", FALSE, ch, 0, vict, TO_ROOM);
        act("$n turns to fight you!", FALSE, ch, 0, vict, TO_VICT);
        FIGHTING(ch) = vict;
      // -- ch already fighting vict -----------------------------------
      } else {
        send_to_char("You do the best you can!\r\n", ch);
      }
      break;
    // ********************** AXETHROW (Hurl Axes) ****************************
    case SKILL_AXETHROW:
      // Check if char has axe in inventory.
      if (vict == NULL) 
      {
	send_to_char("Throw your axe at who, exactly?\r\n", ch);
	return;
      }
      if (!violence_check(ch, vict, skillnum))
        return;
      for (boots = ch->carrying; boots; boots = boots->next_content)
	if (is_axe(boots))
	  break;
      if (!(boots))
      {
	send_to_char("You don't have any axes to throw!\r\n", ch);
	return;
      }
      dam = dice(GET_OBJ_VAL(boots, 0), GET_OBJ_VAL(boots, 1));
      skip = 0;
      // Char has item... Time to throw..
      obj_from_char(boots);
      skip = basic_skill_test(ch, SKILL_AXEMASTERY, 1);
      if (skip) 
	dam = (int)(dam * 1.75);
      if (DEX_CHECK(ch, 0) && !skip)
      {
	act("$N catches your axe as you throw it at $m.", FALSE, ch, NULL, vict, TO_CHAR);
	act("You skillfully catch an axe thrown by $n.", FALSE, ch, NULL, vict, TO_VICT);
	act("$N skillfully catches an axe thrown by $n.", FALSE, ch, NULL, vict, TO_NOTVICT);
	obj_to_char(boots, vict, __FILE__, __LINE__);
	return;
      }
      if (DEX_CHECK(ch, 18) || skip)
      {
	act("You hit $N with your axe, and catch it again.", FALSE, ch, NULL, vict, TO_CHAR);
	act("$n hits you with an axe, and catches it again.", FALSE, ch, NULL, vict, TO_VICT);
	act("$n hits $N with an axe, and skillfully catches it again.", FALSE, ch, NULL, vict, TO_NOTVICT);
	obj_to_char(boots, ch, __FILE__, __LINE__);
      } else {
	act("You hit $N with your axe.", FALSE, ch, NULL, vict, TO_CHAR);
	act("$n hits you with an axe.", FALSE, ch, NULL, vict, TO_VICT);
	act("$n throws an axe at $N.", FALSE, ch, NULL, vict, TO_NOTVICT);
	obj_to_char(boots, vict, __FILE__, __LINE__);
      }
      damage(ch, vict, dam, SKILL_AXETHROW, FALSE);
      clan_rel_inc(ch, vict, -5);

      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
	
      break;
    // ********************** BACKSTAB ****************************************
    case SKILL_BACKSTAB:
      // Check for target
      if (vict == NULL)
      {
	if (!(FIGHTING(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_BACKSTAB)))
	{ 
	  send_to_char("Backstab who?\r\n", ch);
	  return;
	} else 
	  vict = FIGHTING(ch);
      }
      // Check to see if ch is wielding a weapon at all
      if (!wielded)
      {
        send_to_char("You need to wield a weapon to make it a success.\r\n", 
                        ch);
        return;
      }
      // Check for valid backstab weapon
      if (GET_OBJ_VAL(wielded, 3) != TYPE_PIERCE - TYPE_HIT)
      {
        send_to_char("You must use a piercing weapon as your primary weapon in order to backstab!\r\n", ch);
        return;
      }
      // Check if victim is fighting - and backstab special
      if (!IS_SET(GET_SPECIALS(ch), SPECIAL_BACKSTAB))
      {
	if (FIGHTING(vict))
	{
	  send_to_char("You can't backstab a fighting person -- "
		       "they're too alert!\r\n", ch);
	  return;
	}
	if (FIGHTING(ch))
	{
	  send_to_char(CHARFIGHTING, ch);
	  return;
	}
      }
      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;
      // ARTUS - Clones
      if (IS_CLONE(vict))
      {
        send_to_char("You can't backstab a clone!\r\n", ch);
        return;
      }
      // ARTUS - Zomies
      if (IS_ZOMBIE(vict))
      {
        send_to_char("You can't backstab a zombie!\r\n", ch);
        return;
      }
      // Aware Mob messages ...
      if (MOB_FLAGGED(vict, MOB_AWARE))
      {
        switch (number(1,4))
	{
	  case 1:
	    send_to_char("Your weapon hits an invisible barrier and fails to "
                         "penetrate it.\r\n", ch);
	    break;
	  case 2:
	    act("$N skillfuly blocks your backstab and attacks with rage!", 
		FALSE, ch, 0 , vict, TO_CHAR);
	    break;
	  case 3:
	    act("$N cleverly avoids your backstab.", 
		FALSE, ch, 0 , vict, TO_CHAR);
	    break;
	  case 4:
	    act("$N steps to the side avoiding your backstab!", 
		FALSE, ch, 0 , vict, TO_CHAR);
	    break;
        }
        hit(vict, ch, TYPE_UNDEFINED);
        return;
      }
      percent = number(1, 101);     // 101% is a complete failure
      if (IS_SET(GET_SPECIALS(ch), SPECIAL_BACKSTAB))
	prob = MAX(50, MAX(GET_LEVEL(ch), GET_SKILL(ch, SKILL_BACKSTAB)));
      else
	prob = GET_SKILL(ch, SKILL_BACKSTAB);
      if (AWAKE(vict) && (percent > prob))
        damage(ch, vict, 0, SKILL_BACKSTAB, FALSE);
      else
        hit(ch, vict, SKILL_BACKSTAB);
      clan_rel_inc(ch, vict, -5);
      // Wait of 2 violence rounds if affected by adrenaline, or have backstab
      // special, otherwise the normal 3
      if (IS_AFFECTED(ch, AFF_ADRENALINE) || 
          IS_SET(GET_SPECIALS(ch), SPECIAL_BACKSTAB))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      break;

    // ********************** BASH ********************************************
    case SKILL_BASH:

      if (vict == NULL) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
          vict = FIGHTING(ch);
        } else {
          send_to_char("Bash who?\r\n", ch);
          return;
        }
      }

      if (!wielded) {
        send_to_char("You need to wield a weapon to make it a success.\r\n", 
                        ch);
        return;
      }

      if (GET_POS(vict) < POS_FIGHTING) {
        send_to_char("Your victim is already down!.\r\n", ch);
        return;
      }

      if (!violence_check(ch, vict, skillnum))
        return;

      percent = number(1, 111);  /* 101% is a complete failure */
      prob = GET_SKILL(ch, SKILL_BASH);

      if (MOB_FLAGGED(vict, MOB_NOBASH))
        percent = 101;
      if (PRF_FLAGGED(ch, PRF_MORTALK))
        percent = 101;

      // Wait of 2 violence rounds if affected by adrenaline, otherwise normal 3
      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;

      if (percent > prob) {
        damage(ch, vict, 0, SKILL_BASH, FALSE);
        GET_POS(ch) = POS_SITTING;
      } else {
        // If we bash a player and they wimp out, they will move to the previous
        // room before we set them sitting.  If we try to set the victim sitting
        // first to make sure they don't flee, then we can't bash them!  So now
        // we only set them sitting if they didn't flee. -gg 9/21/98
        // -1 = dead, 0 = miss 
	dam = calc_dam_amt(ch, vict, SKILL_BASH);
        if (damage(ch, vict, dam, SKILL_BASH, FALSE) > 0) 
	{
          GET_WAIT_STATE(vict) = PULSE_VIOLENCE;
          if (IN_ROOM(ch) == IN_ROOM(vict)) 
	  {
            GET_POS(vict) = POS_SITTING;
            GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 2;
          }
        }
      }
      clan_rel_inc(ch, vict, -5);
      break;

    // ********************** BERSERK *****************************************
    case SKILL_BERSERK:
      if (!FIGHTING(ch)) {
	send_to_char("You don't seem to be fighting anyone.\r\n", ch);
	return;
      }
      if (IS_AFFECTED(ch, AFF_BERSERK)) {
	send_to_char("You can't be any more berserk.\r\n", ch);
	return;
      }
      if (number(0, 101) > GET_SKILL(ch, SKILL_BERSERK)) { // 101 - Fail
	send_to_char("The madness of berserk eludes you.\r\n", ch);
	return;
      }
      if (!(char_affected_by_timer(ch, TIMER_BERSERK))) 
        timer_to_char(ch,timer_new(TIMER_BERSERK));
      if (timer_use_char(ch,TIMER_BERSERK)) {
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
	apply_spell_skill_abil(ch, SKILL_BERSERK);
        SET_BIT(AFF_FLAGS(ch), AFF_BERSERK);
        send_to_char("You find yourself in a battle frenzy.\r\n", ch);
        act("$n laughs madly and fights with new energy.", FALSE, ch, 0, 0, TO_ROOM);
      } else {
        send_to_char(RESTSKILL,ch);
        return;
      }
      return;
      break;
    // ********************** KICK ********************************************
    case SKILL_KICK:
      if (!(vict)) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
          vict = FIGHTING(ch);
        } else {
          send_to_char("Kick who?\r\n", ch);
          return;
        }
      }

      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;

      // 101% is a complete failure 
      percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
      prob = GET_SKILL(ch, SKILL_KICK);

      // Wait of 2 violence rounds if affected by adrenaline, otherwise normal 3
      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;

      if (percent > prob) {
        damage(ch, vict, 0, SKILL_KICK, FALSE);
      } else {
	dam = calc_dam_amt(ch, vict, SKILL_KICK);
	// Power kick baby - only base it on first kick?
	// Its pretty generous
	if (basic_skill_test(ch, SKILL_POWERKICK, FALSE)) 
	{
	  if ((boots = GET_EQ(ch, WEAR_FEET))) 
            weight = GET_OBJ_WEIGHT(boots);
          dam += (int)((2 * weight) + str_app[GET_STR(ch)].todam); 
	  apply_spell_skill_abil(ch, SKILL_POWERKICK);
	  type = SKILL_POWERKICK;
	}

	if (damage(ch, vict, dam, type, FALSE) > 0) 
	{
          if ((GET_SKILL(ch, SKILL_DOUBLE_KICK) &&
	      (IN_ROOM(ch) == IN_ROOM(vict)) && // Sanity - Artus
              (number(1, 101 - (GET_LEVEL(vict) - GET_LEVEL(ch))) < 
                   GET_SKILL(ch, SKILL_DOUBLE_KICK)))) 
	  {
            act("...$n jumps delivering a quick and powerful second kick!", 
                            FALSE, ch, 0, 0, TO_ROOM);
            act("...$n jumps delivering another powerful kick to you!", 
                            FALSE, ch, 0, vict, TO_VICT);
            act("...you quickly jump and deliver another powerful kick to $N!", 
                            FALSE, ch, 0, vict, TO_CHAR);
            damage(ch, vict, GET_LEVEL(ch), SKILL_KICK, FALSE);
	    apply_spell_skill_abil(ch, SKILL_DOUBLE_KICK); // Artus> 20031014
	  }
        }
      }
      clan_rel_inc(ch, vict, -1);
      break;

    // ********************** PRIMAL SCREAM ***********************************
    case SKILL_PRIMAL_SCREAM:

      //primal scream does not call violence_check - so we must do all those
      //checks correctly in here ...
      
      prob = GET_SKILL(ch, SKILL_PRIMAL_SCREAM);
      percent = number(1,101); /* 101 is a complete failure */

      if (FIGHTING(ch)) 
      {
        send_to_char("You can't prepare yourself properly while fighting!\r\n",
                        ch);
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
        return;
      }

      if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) 
      {
        send_to_char(PEACEROOM, ch);
        return;
      }

      if (percent > prob) 
      {
        act("$n lets out a feeble wimper as $e attempts a primal scream.",
                        FALSE,ch,0,0,TO_ROOM);
        act("You let out a sad little wimper.",FALSE,ch,0,0,TO_CHAR);
        return;
      }

      act("$n inhales deeply and lets out an ear shattering scream!!\r\n",
                        FALSE,ch,0,0,TO_ROOM);
      act("You fill your lungs to capacity and let out an ear shattering "
            "scream!!",FALSE,ch,0,0,TO_CHAR);

      room = ch->in_room;
      /* this shood make it be heard in a 4 room radius */
      for (door = 0; door < NUM_OF_DIRS; door++)
      {
        if (!world[ch->in_room].dir_option[door])
          continue;
        ch->in_room = world[ch->in_room].dir_option[door]->to_room;
        if (room != ch->in_room && ch->in_room != NOWHERE)
          act("You hear a frightening scream coming from somewhere nearby...",
                          FALSE, ch, 0, 0, TO_ROOM);
        ch->in_room = room;
      }

      for (vict = world[ch->in_room].people; vict; vict = next_vict) 
      {
        next_vict = vict->next_in_room;

        if (vict == ch)
          continue; /* ch is the victim skip to next person */
	if (IS_NPC(ch))
	{
	  if (IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
	    continue;
	  if (!IS_NPC(vict) && !LR_FAIL(vict, LVL_GOD))
	    continue;
	} else {
	  if (!IS_NPC(vict))
	    continue;
	  if (IS_AFFECTED(vict, AFF_CHARM))
	    continue;
	  if (MOB_FLAGGED(vict, MOB_NOKILL))
	    continue;
	}

	performed = TRUE;
	dam = calc_dam_amt(ch,vict,SKILL_PRIMAL_SCREAM);
	damage(ch,vict,dam,SKILL_PRIMAL_SCREAM,FALSE);
      }

      // Wait of 2 violence rounds if affected by adrenaline, otherwise normal 3
      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;

      if (!performed)
        return;
      
      // Artus> This shouldnt touch PCs anyway. clan_rel_inc(ch, vict, 1);
      break;
 

    // ********************** HEADBUTT ****************************************
    case SKILL_HEADBUTT:

      if (!(vict)) 
      {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) 
	{
          vict = FIGHTING(ch);
        } else {
          send_to_char("Headbutt who?\r\n", ch);
          return;
        }
      }

      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;

      /* 101% is a complete failure */
      percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
      prob = GET_SKILL(ch, SKILL_HEADBUTT);

      // Wait of 2 violence rounds if affected by adrenaline, otherwise normal 3
      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;

      if (percent > prob) 
      {
        damage(ch, vict, 0, SKILL_HEADBUTT, FALSE);
        return;
      }
      /* Artus> Lets try this another way...
       *  if (damage(ch, vict, (int)(GET_LEVEL(ch) * 2 * (SPELL_EFFEC(ch, skillnum) / 100) + GET_EQ_WEIGHT(ch, WEAR_HEAD) + str_app[GET_STR(ch)].todam), SKILL_HEADBUTT, FALSE) > 0) 
       *   GET_WAIT_STATE(vict) = PULSE_VIOLENCE; */
      dam = calc_dam_amt(ch, vict, SKILL_HEADBUTT);
      if (damage(ch, vict, dam, SKILL_HEADBUTT, FALSE) > 0)
	GET_WAIT_STATE(vict) = PULSE_VIOLENCE;
      
      clan_rel_inc(ch, vict, -3);
      break; 

    // ********************** PILEDRIVE ***************************************
    case SKILL_PILEDRIVE:

      if (!(vict)) 
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) 
	{
          vict = FIGHTING(ch);
        } else {
          send_to_char("Piledrive who?\r\n", ch);
          return;
        }

      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;

      /* 101% is a complete failure */
      percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
      prob = GET_SKILL(ch, SKILL_PILEDRIVE);

      // Wait of 2 violence rounds if affected by adrenaline, otherwise normal 3
      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;

      if (percent > prob) {
        damage(ch, vict, 0, SKILL_PILEDRIVE, FALSE);
        return;
      }
      /* Artus> Lets do this a little differently.. Too.
       * if (damage(ch, vict, (int)(GET_LEVEL(ch) * 3 * (SPELL_EFFEC(ch, skillnum) / 100) + str_app[GET_STR(ch)].todam), SKILL_PILEDRIVE, FALSE)) 
       *   GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 3; */
      dam = calc_dam_amt(ch, vict, SKILL_PILEDRIVE);
      if (damage(ch, vict, dam, SKILL_PILEDRIVE, FALSE))
	GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 3;
      clan_rel_inc(ch, vict, -4);
      break;

    // ********************** TRIP ********************************************
    case SKILL_TRIP:

      if (!(vict)) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
          vict = FIGHTING(ch);
        } else {
          send_to_char("Trip who?\r\n", ch);
          return;
        }
      }

      if (GET_POS(vict) == POS_SITTING) {
        send_to_char("They are already down.\r\n",ch);
        return;
      }

      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;

      /* 101% is a complete failure */
      percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
      prob = GET_SKILL(ch, SKILL_TRIP);

      // Wait of 2 violence rounds if affected by adrenaline, otherwise normal 3
      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;

      if (percent > prob) 
      {
        damage(ch, vict, 0, SKILL_TRIP, FALSE);
        return;
      }
      dam = calc_dam_amt(ch, vict, SKILL_TRIP);
      if (damage(ch, vict, dam, SKILL_TRIP, FALSE) > 0) 
      {
	  GET_POS(vict) = POS_SITTING;
          GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 2;
      }
      clan_rel_inc(ch, vict, -2);
      break;

    // ********************** BEARHUG *****************************************
    case SKILL_BEARHUG:

      if (!(vict)) 
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) 
	{
          vict = FIGHTING(ch);
        } else {
          send_to_char("Bearhug who?\r\n", ch);
          return;
        }

      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;

      /* 101% is a complete failure */
      percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
      prob = GET_SKILL(ch, SKILL_BEARHUG);

      if (IS_AFFECTED(ch, AFF_ADRENALINE))
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
      else
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;

      if (percent > prob) 
      {
        damage(ch, vict, 0, SKILL_BEARHUG, FALSE);
        return;
      } 
 
      dam = calc_dam_amt(ch, vict, SKILL_BEARHUG);
      
      if (damage(ch, vict, dam, SKILL_BEARHUG, FALSE) > 0)
	GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 2;

      clan_rel_inc(ch, vict, -3);
      break;

    // ********************** BATTLECRY ***************************************
    case SKILL_BATTLECRY:
 
      if (!(vict)) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
          vict = FIGHTING(ch);
        } else {
          send_to_char("On whom should you perform a battlecry?\r\n", ch);
          return;
        }
      }

      // Common violence checks
      if (!violence_check(ch, vict, skillnum)) {
        return;
      }

      if (!basic_skill_test(ch, SKILL_BATTLECRY, TRUE)) {
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
        act("You fail a battlecry attempt on $N!",
            FALSE, ch, 0, vict, TO_CHAR);
        act("$n fails a battlecry attempt on you!",
            TRUE, ch, 0, vict, TO_VICT);
        act("$n fails a battlecry attempt on $N!",
            TRUE, ch, 0, vict, TO_NOTVICT);
        hit(ch, vict, TYPE_UNDEFINED);
        return;
      } else {
        act("You scream 'AAARRGGGG' as you perform a battlecry on $N!",
            FALSE, ch, 0, vict, TO_CHAR);
        act("$n screams 'AAARRGGGG' as $m performs $s battlecry on you!",
            TRUE, ch, 0, vict, TO_VICT);
        act("$n screams 'AAARRGGGG' as $m performs $s battlecry on $N!",
            TRUE, ch, 0, vict, TO_NOTVICT);

        if (basic_skill_test(ch, SKILL_KICK, FALSE) && vict && 
	    IN_ROOM(ch) == IN_ROOM(vict)) {
          sprintf(buf, " %s", arg);
          do_violent_skill(ch, buf, 0, SKILL_KICK);
        }

        if (basic_skill_test(ch, SKILL_BEARHUG, FALSE) && vict && 
	    IN_ROOM(ch) == IN_ROOM(vict)) {
          sprintf(buf, " %s", arg);
          do_violent_skill(ch, buf, 0, SKILL_BEARHUG);
        }

        if (basic_skill_test(ch, SKILL_HEADBUTT, FALSE) && vict && 
	    IN_ROOM(ch) == IN_ROOM(vict)) {
          sprintf(buf, " %s", arg);
          do_violent_skill(ch, buf, 0, SKILL_HEADBUTT);
        }

        if (basic_skill_test(ch, SKILL_PILEDRIVE, FALSE) && vict && 
	    IN_ROOM(ch) == IN_ROOM(vict)) {
          sprintf(buf, " %s", arg);
          do_violent_skill(ch, buf, 0, SKILL_PILEDRIVE);
        }

        if (basic_skill_test(ch, SKILL_BASH, FALSE) && vict && 
	    IN_ROOM(ch) == IN_ROOM(vict) && wielded != NULL) {
          sprintf(buf, " %s", arg);
          do_violent_skill(ch, buf, 0, SKILL_BASH);
        }

        GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 4;
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
	clan_rel_inc(ch, vict, -7);
      }
      break;

    // ********************** BODYSLAM ****************************************
    case SKILL_BODYSLAM:
      int attempt;

      if (!(vict)) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
            vict = FIGHTING(ch);
        } else {
          send_to_char("Bodyslam who?\r\n", ch);
          return;
        }
      }

      // Common Violence Checks.
      if (!violence_check(ch, vict, skillnum)) 
        return;

      // clan protection (guards and healers) - dont want them moving rooms
      if (IS_NPC(vict) && GET_MOB_VZNUM(vict) == CLAN_ZONE)
      {
        sprintf(buf, 
	    "%s stops you before you can bodyslam %s.\r\n", 
	    GET_NAME(vict), HMHR(vict));
	send_to_char(buf, ch);
	return;
      }

      /* 101% is a complete failure */
      percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
      prob = GET_SKILL(ch, SKILL_BODYSLAM);

      GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;

      if (percent > prob) {
	act("You attempt to bodyslam $N and miss dearly!", 
	    TRUE, ch, 0, vict, TO_CHAR);
	act("$n attempts to bodyslam you and misses dearly!", 
	    TRUE, ch, 0, vict, TO_VICT);
	act("$n attempts to bodyslam $N but only misses dearly!",
	    TRUE, ch, 0, vict, TO_ROOM);
        damage(ch, vict, 0, SKILL_BODYSLAM, FALSE);
      } else {
	// 6 chances of ch removing victim from room, based on a random exit ...
        if (vict && IN_ROOM(ch) == IN_ROOM(vict)) 
	{
	  /* Select a random direction */
	  attempt = number(0, NUM_OF_DIRS + MAX(0, 18 - GET_DEX(ch)));
	  if ((attempt < NUM_OF_DIRS) && CAN_GO(vict, attempt) && 
	      !ROOM_FLAGGED(EXIT(vict, attempt)->to_room, ROOM_DEATH) &&
	      (EXIT(vict, attempt)->to_room != IN_ROOM(vict)) &&
	      char_can_enter(vict, &world[EXIT(vict, attempt)->to_room], FALSE))
	  {
	    performed = TRUE; // well moved from room acutally
	    if (GET_EQ(ch, WEAR_BODY))
	      dam = number(1, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BODY)));
	    else
	      dam = number(1, 3);
	    if (damage(ch, vict, dam, SKILL_BODYSLAM, FALSE) > 0)
	    {
	      sprintf(buf, "You are bodyslammed out of the room, %s, by $n!", dirs[attempt]);
	      act(buf, TRUE, ch, 0, vict, TO_VICT);
	      sprintf(buf, "$N is bodyslammed %s from the room by $n!", dirs[attempt]);
	      act(buf, TRUE, ch, 0, vict, TO_ROOM);
	      sprintf(buf, "You bodyslam $N out of the room, %s!", dirs[attempt]);
	      act(buf, TRUE, ch, 0, vict, TO_CHAR);
	      // move the vict to room
	      char_from_room(vict);
	      char_to_room(vict, EXIT(ch, attempt)->to_room);
	      look_at_room(vict, TRUE);
	      act("$n is forced into the room.", TRUE, vict, 0, NULL, TO_ROOM);
	      GET_WAIT_STATE(ch) = 0;
	      stop_fighting(vict);
	      if (FIGHTING(ch) == vict)
		stop_fighting(ch);
	    }
	    break;
	  }
	}

	if (!performed)
	{ // not moved from room
	    dam = calc_dam_amt(ch, vict, SKILL_BODYSLAM);
            if (damage(ch, vict, dam, SKILL_BODYSLAM, FALSE) > 0) 
	    {
              act("$N is slammed to the ground by $n's bodyslam!",
                  TRUE, ch, 0, vict, TO_ROOM);
              act("You slam $N into the ground!",
                  TRUE, ch, 0, vict, TO_CHAR);
              act("You are slammed into the ground by $n's bodyslam! OUCH!",
                  TRUE, ch, 0, vict, TO_VICT);
	    }
          }
	}
	clan_rel_inc(ch, vict, -3);
	break;

    // ********************** FLYING TACKLE ***********************************
    case SKILL_FLYINGTACKLE:

      if (!(vict)) {
        if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
          vict = FIGHTING(ch);
        } else {
          send_to_char("Whom do you wish to fly tackle?\r\n", ch);
          return;
        }
      }

      // Artus> TODO: Base on carried weight.

      // Common violence checks
      if (!violence_check(ch, vict, skillnum))
        return;

      /* 101% is a complete failure */
      percent = ((10 - compute_armor_class(vict, 1)) * 2) + number(1, 101);
      prob = GET_SKILL(ch, SKILL_FLYINGTACKLE);

      GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - 5); 

      if (percent > prob) {
        if (IS_AFFECTED(ch, AFF_ADRENALINE)) {
          GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
        } else {
          GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
        }
        damage(ch, vict, 0, SKILL_FLYINGTACKLE, FALSE);
        return;
      } else {
	dam = calc_dam_amt(ch, vict, SKILL_FLYINGTACKLE);
        if (damage(ch, vict, dam, SKILL_FLYINGTACKLE, FALSE)) {
          if (IS_AFFECTED(ch, AFF_ADRENALINE)) {
            GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;
          } else {
            GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 3;
          }
	  GET_WAIT_STATE(vict) = PULSE_VIOLENCE * 3;
	}
      }
      clan_rel_inc(ch, vict, -3);
      break;

    default:
      basic_mud_log("SYSERR: Invalid skill (%d) number passed to do_violent_skill", 
                      skillnum);
      return;
  }

  // We made it - usage of skill was a success - now apply ability gains ....
  if (skillnum != SCMD_HIT && skillnum != SCMD_KILL && skillnum != SCMD_MURDER)
    apply_spell_skill_abil(ch, skillnum); 

  // DM: TODO: shoot
}

// DM - no longer used
void spell_level(int spell, int chclass, int level, byte intl, byte str, byte wis, byte con, byte dex, byte cha)
{
  int bad = 0;

  if (spell < 0 || spell > TOP_SPELL_DEFINE) {
    basic_mud_log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
    return;
  }

  if (chclass < 0 || chclass >= NUM_CLASSES) {
    basic_mud_log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
		chclass, NUM_CLASSES - 1);
    bad = 1;
  }

  if (level < 1 || level > LVL_IMPL) {
    basic_mud_log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell),
		level, LVL_IMPL);
    bad = 1;
  }

  if (!bad) {    
    spell_info[spell].min_level[chclass] = level;

    spell_info[spell].str[chclass] = str;
    spell_info[spell].intl[chclass] = intl;
    spell_info[spell].wis[chclass] = wis;
    spell_info[spell].dex[chclass] = dex;
    spell_info[spell].con[chclass] = con;
    spell_info[spell].cha[chclass] = cha;
  }

}


/* Assign the spells on boot up */
void spello(int spl, const char *name, int max_mana, int min_mana,
	int mana_change, int minpos, int targets, int violent, int routines)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++) {
    spell_info[spl].min_level[i] = LVL_OWNER+1;
    spell_info[spl].mana_max[i] = -1;
    spell_info[spl].mana_min[i] = -1;
    spell_info[spl].mana_change[i] = -1;
    spell_info[spl].intl[i] = -1;
    spell_info[spl].dex[i] = -1;
    spell_info[spl].wis[i] = -1;
    spell_info[spl].str[i] = -1;
    spell_info[spl].con[i] = -1;
    spell_info[spl].cha[i] = -1;
  }
  spell_info[spl].min_position = minpos;
  spell_info[spl].targets = targets;
  spell_info[spl].violent = violent;
  spell_info[spl].routines = routines;
  spell_info[spl].name = name;
}


void unused_spell(int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++) {
    spell_info[spl].min_level[i] = LVL_OWNER + 1;
    spell_info[spl].mana_max[i] = -1;
    spell_info[spl].mana_min[i] = -1;
    spell_info[spl].mana_change[i] = -1;
    spell_info[spl].intl[i] = -1;
    spell_info[spl].dex[i] = -1;
    spell_info[spl].wis[i] = -1;
    spell_info[spl].str[i] = -1;
    spell_info[spl].con[i] = -1;
    spell_info[spl].cha[i] = -1;
  }
  spell_info[spl].min_position = 0;
  spell_info[spl].targets = 0;
  spell_info[spl].violent = 0;
  spell_info[spl].routines = 0;
  spell_info[spl].name = unused_spellname;
}

#define skillo(skill, name) spello(skill, name, 0, 0, 0, 0, 0, 0, 0);


/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * spellname: The name of the spell.
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i <= TOP_SPELL_DEFINE; i++)
    unused_spell(i);
  /* Do not change the loop above. */

  spello(SPELL_ADV_HEAL, "advanced heal", 130, 70, 2,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS | MAG_UNAFFECTS);
                                                  
  spello(SPELL_ANIMATE_DEAD, "animate dead", 100, 80, 2,
         POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS);
 
  spello(SPELL_ARMOR, "armor", 30, 15, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_BLESS, "bless", 35, 5, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_BLINDNESS, "blindness", 35, 25, 1,
         POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS);
 
  spello(SPELL_BURNING_HANDS, "burning hands", 30, 10, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_CHARM, "charm", 75, 50, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL);
 
  spello(SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS);

  spello(SPELL_CLONE, "clone", 145, 100, 2,
         POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMONS);
 
  spello(SPELL_CLOUD_KILL, "cloud kill", 250, 250, 3,
         POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);
 
  spello(SPELL_COLOR_SPRAY, "color spray", 30, 15, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5,
         POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL);
 
  spello(SPELL_CREATE_FOOD, "create food", 30, 5, 4,
         POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS);
 
  spello(SPELL_CREATE_WATER, "create water", 30, 5, 4,
         POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL);
 
  spello(SPELL_CURE_BLIND, "cure blind", 30, 5, 2,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS);
 
  spello(SPELL_CURE_CRITIC, "cure critical", 30, 10, 2,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_CURE_LIGHT, "cure light", 30, 10, 2,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS);
 
  spello(SPELL_CURSE, "curse", 80, 50, 2,
         POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, TRUE, MAG_AFFECTS | MAG_ALTER_OBJS);
 
  spello(SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_DETECT_INVIS, "detect invisibility", 20, 15, 2,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_DETECT_POISON, "detect poison", 15, 5, 1,
         POS_STANDING, TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);
 
  spello(SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_DIVINE_HEAL, "divine heal", 175, 100, 3,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS | MAG_UNAFFECTS);
 
  spello(SPELL_DIVINE_PROTECTION, "divine protection", 80, 65, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_DRAGON, "dragon", 50, 20, 5,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_EARTHQUAKE, "earthquake", 40, 25, 3,
         POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);
 
  spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10,
         POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL);
 
  spello(SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL);
 
  spello(SPELL_FEAR, "fear", 40, 20, 2,
         POS_FIGHTING, TAR_CHAR_ROOM, TRUE, MAG_MANUAL);
 
  spello(SPELL_FINGERDEATH, "finger of death", 250, 220, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL);
 
  spello(SPELL_FIREBALL, "fireball", 40, 30, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_FIRE_SHIELD, "fire shield", 175, 75, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_FIRE_WALL, "fire wall", 250, 147, 6,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_FLY, "fly", 102, 52, 2,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_GROUP_ARMOR, "group armor", 70, 60, 2,
         POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_GROUP_HEAL, "group heal", 130, 110, 5,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS);
 
  spello(SPELL_GROUP_SANCTUARY, "group sanctuary", 150, 120, 5,
         POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS);
 
  spello(SPELL_HARM, "harm", 75, 45, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_HEAL, "heal", 80, 40, 3,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS | MAG_UNAFFECTS);
 
  spello(SPELL_GATE, "dimension gate", 130, 90, 2,
         POS_STANDING, TAR_CHAR_INWORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);  

  spello(SPELL_HASTE, "haste", 150, 120, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_HOLY_AID, "holy aid", 70, 40, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_IDENTIFY, "identify", 50, 25, 2,
         POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);
 
  spello(SPELL_INFRAVISION, "infravision", 30, 20, 1,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_INVISIBLE, "invisibility", 40, 30, 1,
         POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_LIGHT_SHIELD, "lightning shield", 150, 85, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_LOCATE_OBJECT, "locate object", 30, 20, 1,
         POS_STANDING, TAR_OBJ_WORLD, FALSE, MAG_MANUAL);
 
  spello(SPELL_MAGIC_MISSILE, "magic missile", 25, 10, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_MANA, "mana", 10, 10, 10, POS_STANDING,
             TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS);

  spello(SPELL_METEOR_SWARM, "meteor swarm", 80, 70, 3,
         POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);
 
  spello(SPELL_PARALYZE, "paralyze",  50, 30, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS); 

  spello(SPELL_PLASMA_BLAST, "plasma blast", 80, 50, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
 
  spello(SPELL_POISON, "poison", 50, 20, 3,
         POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE, MAG_AFFECTS);
 
  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 123, 123, 0,
         POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);
 
  spello(SPELL_PROT_FROM_GOOD, "protection from good", 133, 133, 0,
         POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);
 
  spello(SPELL_NOHASSLE, "nohassle", 476, 476, 0,
        POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);
 
  spello(SPELL_REFRESH, "refresh", 60, 30, 3,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS);
 
  spello(SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5,
         POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_EQUIP | TAR_OBJ_INV, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS);
 
  spello(SPELL_GREATER_REMOVE_CURSE, "greater remove curse", 90, 45, 5,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_REMOVE_PARA, "remove paralysis", 50, 30, 5,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS);
 
  spello(SPELL_REMOVE_POISON, "remove poison", 40, 8, 4,
         POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_UNAFFECTS);
 
  spello(SPELL_SANCTUARY, "sanctuary", 110, 100, 5,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS); 

  spello(SPELL_SENSE_LIFE, "sense life", 45, 20, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_SENSE_WOUNDS, "sense wounds", 45, 20, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);
 
  spello(SPELL_SERPENT_SKIN, "serpent skin", 200, 70, 10,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_SLEEP, "sleep", 40, 25, 5,
         POS_STANDING, TAR_CHAR_ROOM, TRUE, MAG_AFFECTS);
 
  spello(SPELL_SPIRIT_ARMOR, "spirit armor", 60, 42, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_STONESKIN, "stoneskin", 77, 47, 3,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_STRENGTH, "strength", 35, 30, 1,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_SUMMON, "summon", 75, 60, 3,
         POS_STANDING, TAR_CHAR_INWORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);
 
  spello(SPELL_TELEPORT, "teleport", 30, 110, 3,
         POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL);

  spello(SPELL_UNHOLY_VENGEANCE, "unholy vengeance", 666, 666, 0,
         POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);
 
  spello(SPELL_WATERBREATHE, "waterbreathe", 100, 40, 2,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_WATERWALK, "waterwalk", 50, 30, 2,
         POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
 
  spello(SPELL_WHIRLWIND, "whirlwind", 80, 50, 3,
         POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);
 
  spello(SPELL_WORD_OF_RECALL, "recall", 20, 10, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL);
 
  spello(SPELL_WRAITH_TOUCH, "wraith touch", 200, 150, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);  

/*********************** stock 30bpl119 spello's
  spello(SPELL_ANIMATE_DEAD, "animate dead", 35, 10, 3, POS_STANDING,
	TAR_OBJ_ROOM, FALSE, MAG_SUMMONS);

  spello(SPELL_ARMOR, "armor", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_BLESS, "bless", 35, 5, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS);

  spello(SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CHARM, "charm person", 75, 50, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL);

  spello(SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS);

  spello(SPELL_CLONE, "clone", 80, 65, 5, POS_STANDING,
	TAR_SELF_ONLY, FALSE, MAG_SUMMONS);

  spello(SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_MANUAL);

  spello(SPELL_CREATE_FOOD, "create food", 30, 5, 4, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_CREATIONS);

  spello(SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_STANDING,
	TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL);

  spello(SPELL_CURE_BLIND, "cure blind", 30, 5, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS);

  spello(SPELL_CURE_CRITIC, "cure critic", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_CURE_LIGHT, "cure light", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_CURSE, "curse", 80, 50, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, TRUE, MAG_AFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_DETECT_INVIS, "detect invisibility", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_DRAGON, "dragon", 50, 20, 5, POS_FIGHTING, 
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_EARTHQUAKE, "earthquake", 40, 25, 3, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS);

  spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING,
	TAR_OBJ_INV, FALSE, MAG_MANUAL);

  spello(SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL);

  spello(SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_FIREBALL, "fireball", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_GROUP_HEAL, "group heal", 80, 60, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_HARM, "harm", 75, 45, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_HEAL, "heal", 60, 40, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_UNAFFECTS);

  spello(SPELL_INFRAVISION, "infravision", 25, 10, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_STANDING,
	TAR_OBJ_WORLD, FALSE, MAG_MANUAL);

  spello(SPELL_MAGIC_MISSILE, "magic missile", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_POISON, "poison", 50, 20, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
	MAG_AFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
	MAG_UNAFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_REMOVE_POISON, "remove poison", 40, 8, 4, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_SLEEP, "sleep", 40, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM, TRUE, MAG_AFFECTS);

  spello(SPELL_STRENGTH, "strength", 35, 30, 1, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_SUMMON, "summon", 75, 50, 3, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);

  spello(SPELL_TELEPORT, "teleport", 75, 50, 3, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_WATERWALK, "waterwalk", 40, 20, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_WORD_OF_RECALL, "recall", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL);


****************************************/


  /* NON-castable spells should appear below here. */

  spello(SPELL_IDENTIFY, "identify", 0, 0, 0, 0,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);

  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */

  spello(SPELL_CHANGED, "lycanthropy", 0, 0, 0, POS_SITTING, 
        TAR_IGNORE, TRUE, 0);

  spello(SPELL_CREAMED, "creamed", 0, 0, 0, POS_SITTING, TAR_IGNORE, TRUE, 0);

  spello(SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0);

  spello(SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0);

  spello(SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0);

  spello(SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0);

  spello(SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0);
  
  spello(SPELL_SUPERMAN, "superman", 0, 0, 0,
         POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL);


  /*
   * Declaration of skills - this actually doesn't do anything except
   * set it up so that immortals can use these skills by default.  The
   * min level to use the skill for other classes is set up in class.c.
   */

  skillo(SKILL_BACKSTAB, "backstab"); 
  skillo(SKILL_BASH, "bash"); 
  skillo(SKILL_COMPARE, "compare"); 
  skillo(SKILL_HIDE, "hide"); 
  skillo(SKILL_HUNT, "hunt"); 
  skillo(SKILL_KICK, "kick"); 
  skillo(SKILL_PICK_LOCK, "pick lock"); 
  skillo(SKILL_PRIMAL_SCREAM, "primal scream"); 
  skillo(SKILL_RESCUE, "rescue"); 
  skillo(SKILL_RETREAT, "retreat"); 
  skillo(SKILL_SCAN, "scan"); 
  skillo(SKILL_2ND_ATTACK, "second attack"); 
  skillo(SKILL_SNEAK, "sneak"); 
  skillo(SKILL_SPY, "spy"); 
  skillo(SKILL_STEAL, "steal"); 
  skillo(SKILL_3RD_ATTACK, "third attack"); 
  skillo(SKILL_THROW, "throw"); 
  skillo(SKILL_TRACK, "track");

  /* New skills */
  skillo(SKILL_ADRENALINE, "adrenaline");
  skillo(SKILL_AMBIDEXTERITY, "ambidexterity");
  skillo(SKILL_AMBUSH, "ambush");
  skillo(SKILL_ARMOURCRAFT, "armorcraft");
  skillo(SKILL_ATTEND_WOUNDS, "attend wounds");
  skillo(SKILL_AXEMASTERY, "axemastery");
  skillo(SKILL_BARGAIN,	"bargain");
  skillo(SKILL_BATTLECRY, "battlecry");
  skillo(SKILL_BEARHUG, "bearhug");
  skillo(SKILL_BERSERK, "berserk");
  skillo(SKILL_BLADEMASTERY, "blademastery");
  skillo(SKILL_BODYSLAM, "bodyslam");
  spell_info[SKILL_BODYSLAM].violent = 2; // Don't flee/show wounds in damage()
  skillo(SKILL_CLOT_WOUNDS, "clot wounds");
  skillo(SKILL_DETECT_DEATH, "detect deathtraps"); 
  skillo(SKILL_DOUBLE_KICK, "double kick");

  skillo(SKILL_HEALING_MASTERY, "healing mastery");
  skillo(SKILL_HEALING_EFFICIENCY, "healing efficiency");
  skillo(SKILL_BURGLE, "burglery");
  skillo(SKILL_SENSE_STATS, "sense stats");
  skillo(SKILL_SENSE_CURSE, "sense curse");
  skillo(SKILL_MOUNTAINEER, "mountaineering");
  skillo(SKILL_DOUBLE_BACKSTAB, "double backstab");
  skillo(SKILL_DEFEND, "defend");
  skillo(SKILL_CONCEAL_SPELL_CASTING, "conceal spell casting");
  skillo(SKILL_CAMPING, "camping");
  skillo(SKILL_BARGAIN, "bargaining");
  skillo(SKILL_BLADEMASTERY, "blade mastery");
  skillo(SKILL_AXEMASTERY, "axe mastery");
  skillo(SKILL_WEAPONCRAFT, "weaponcraft");
  skillo(SKILL_AMBIDEXTERITY, "ambidexterity");

  skillo(SKILL_DISARM, "disarm"); 
  skillo(SKILL_HEADBUTT, "headbutt");
  skillo(SKILL_PILEDRIVE, "piledrive");
  skillo(SKILL_PURSE, "purse evaluation");
  skillo(SKILL_TRIP, "trip");
  skillo(SKILL_WEAPONCRAFT, "weaponcraft");
  skillo(SKILL_TUMBLE, "tumble");
  skillo(SKILL_FLYINGTACKLE, "flytackle");
  skillo(SKILL_FIRST_AID, "first aid");
  skillo(SKILL_POISONBLADE, "poison blade");
  skillo(SKILL_DARKRITUAL, "dark ritual");
  skillo(SKILL_MOUNT, "mount");
  skillo(SKILL_MEDITATE, "meditate");
  skillo(SKILL_TORCH, "torch");
  skillo(SKILL_SHIELDMASTERY, "shielding mastery");
  skillo(SKILL_POWERKICK, "kicking mastery");
  skillo(SKILL_SUICIDE, "suicide");
  skillo(SKILL_COMPOST, "compost");
  skillo(SKILL_TRAP_PIT, "trap pit");
  skillo(SKILL_SLIP, "slip");
  skillo(SKILL_SLEIGHT, "sleight");
  skillo(SKILL_GLANCE, "glance");
  skillo(SKILL_LISTEN, "listen");
  skillo(SKILL_PERCEPTION, "perceiving eye");
  skillo(SKILL_HEAL_TRANCE, "healing trance");
  skillo(SKILL_AXETHROW, "axethrow");
  skillo(SKILL_TRAP_MAGIC, "trap magic");
/* manufacturing skills */
  skillo(SKILL_TAILORING, "tailoring");
  skillo(SKILL_BLACKSMITHING, "blacksmithing");
  skillo(SKILL_BREWING, "brewing");
  skillo(SKILL_JEWELRY, "jewelry");
}
