/*
   Modifications done by Brett Murphy to introduce character races
   Spells are set for magic user [array zero]
*/
/*
 * newspellparser.c Portions from DikuMUD, Copyright (C) 1990, 1991. Written
 * by Fred Merkel and Jeremy Elson Part of JediMUD Copyright (C) 1993
 * Trustees of The Johns Hopkins Unversity All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "structs.h"
#include "utils.h" 
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"

struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

#define SINFO spell_info[spellnum]

extern struct room_data *world;

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, this should provide
 * ample slots for skills.
 */

char *spells[] =
{
  "!RESERVED!",			/* 0 - reserved */

  /* SPELLS */

  "armor",			/* 1 */
  "teleport",
  "bless",
  "blindness",
  "burning hands",
  "call lightning",
  "charm person",
  "chill touch",
  "clone",
  "color spray",		/* 10 */
  "control weather",
  "create food",
  "create water",
  "cure blind",
  "cure critic",
  "cure light",
  "curse",
  "detect alignment",
  "detect invisibility",
  "detect magic",		/* 20 */
  "detect poison",
  "dispel evil",
  "earthquake",
  "enchant weapon",
  "energy drain",
  "fireball",
  "harm",
  "heal",
  "invisibility",
  "lightning bolt",		/* 30 */
  "locate object",
  "magic missile",
  "poison",
  "protection from evil",
  "remove curse",
  "sanctuary",
  "shocking grasp",
  "sleep",
  "strength",
  "summon",			/* 40 */
  "ventriloquate",
  "word of recall",
  "remove poison",
  "sense life",
  "animate dead",
  "dispel good",
  "group armor",
  "group heal",
  "group recall",
  "infravision",		/* 50 */
  "waterwalk",
  "finger of death",
  "advanced heal",
  "refresh",
  "fear",
  "dimension gate",
  "meteor swarm",
  "group sanctuary",
  "serpent skin",
  "fly",			/* 60 */
  "plasma blast",
  "cloud kill",
  "paralyze",
  "remove paralysis",
  "waterbreathe",	/* 65 */
  "wraith touch",
  "dragon", 
  "mana",
  "whirlwind",
  "spirit armor",	/* 70 */
  "holy aid", 
  "divine protection", 
  "haste", 
  "nohassle", 
  "stoneskin",	/* 75 */
  "divine heal", 
  "lightning shield", 
  "fire shield", 
  "fire wall", 
  "protection from good",	/* 80 */
  "identify", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 85 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 90 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 95 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 100 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 105 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 110 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 115 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 120 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 125 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", 
  "Werewolf/Vampire",	/* 130 */

  /* SKILLS */

  "backstab",			/* 131 */
  "bash",
  "hide",
  "kick",
  "pick lock",
  "punch",
  "rescue",
  "sneak",
  "steal",
  "track",			/* 140 */
  "second attack",
  "third attack",
  "primal scream",
  "scan",
  "hunt",	/* 145 */
  "retreat",
  "throw",
  "compare", 
  "spy", 
  "!UNUSED!",	/* 150 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 155 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 160 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 165 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 170 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 175 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 180 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 185 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 190 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 195 */
  "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",	/* 200 */

  /* OBJECT SPELLS AND NPC SPELLS/SKILLS */

  "identify",			/* 201 */
  "fire breath",
  "gas breath",
  "frost breath",
  "acid breath",
  "lightning breath",

  "\n"				/* the end */
};


struct syllable {
  char *org;
  char *new;
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

int mag_manacost(struct char_data * ch, int spellnum)
{
  int mana;

  mana = MAX(SINFO.mana_max - (SINFO.mana_change *
		    (GET_LEVEL(ch) - SINFO.min_level[0])),
	     SINFO.mana_min);

/* a 25% increase across the board for casting */
  return (mana * 1.25);

}

/* say_spell erodes buf, buf1, buf2 */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch,
	            struct obj_data * tobj)
{
  char lbuf[256];

  struct char_data *i;
  int j, ofs = 0;

  *buf = '\0';
  strcpy(lbuf, spells[spellnum]);

  while (*(lbuf + ofs)) {
    for (j = 0; *(syls[j].org); j++) {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
	strcat(buf, syls[j].new);
	ofs += strlen(syls[j].org);
      }
    }
  }

  if (tch != NULL && tch->in_room == ch->in_room) {
    if (tch == ch)
      sprintf(lbuf, "$n closes $s eyes and utters the words, '%%s'.");
    else
      sprintf(lbuf, "$n stares at $N and utters the words, '%%s'.");
  } else if (tobj != NULL && tobj->in_room == ch->in_room)
    sprintf(lbuf, "$n stares at $p and utters the words, '%%s'.");
  else
    sprintf(lbuf, "$n utters the words, '%%s'.");

  sprintf(buf1, lbuf, spells[spellnum]);
  sprintf(buf2, lbuf, buf);

  for (i = world[ch->in_room].people; i; i = i->next_in_room) {
    if (i == ch || i == tch || !i->desc || !AWAKE(i))
      continue;
    if (GET_CLASS(ch) == GET_CLASS(i))
      perform_act(buf1, ch, tobj, tch, i);
    else
      perform_act(buf2, ch, tobj, tch, i);
  }

  if (tch != NULL && tch != ch) {
    sprintf(buf1, "$n stares at you and utters the words, '%s'.",
	    GET_CLASS(ch) == GET_CLASS(tch) ? spells[spellnum] : buf);
    act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }
}


int find_skill_num(char *name)
{
  int index = 0, ok;
  char *temp, *temp2;
  char first[256], first2[256];

  while (*spells[++index] != '\n') {
    if (is_abbrev(name, spells[index]))
      return index;

    ok = 1;
    temp = any_one_arg(spells[index], first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first, first2))
	ok = 0;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return index;
  }

  return -1;
}



/*
 * All invocations of any spell must come through this function,
 * call_magic(). This is also the entry point for non-spoken or unrestricted
 * spells. Spellnum 0 is legal but silently ignored here, to make callers
 * simpler.
 */
int call_magic(struct char_data * caster, struct char_data * cvict,
	     struct obj_data * ovict, int spellnum, int level, int casttype)
{
  int savetype;
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index;

  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
    return 0;

  if (ROOM_FLAGGED(caster->in_room, ROOM_NOMAGIC) && 
      GET_LEVEL(caster)<LVL_IMPL && casttype != CAST_MAGIC_OBJ) {
    send_to_char("Your magic fizzles out and dies.\r\n", caster);
    act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
    return 0;
  }
  if (IS_SET(ROOM_FLAGS(caster->in_room), ROOM_PEACEFUL) && GET_LEVEL(caster)<LVL_IMPL &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
    send_to_char("A flash of white light fills the room, dispelling your "
		 "violent magic!\r\n", caster);
    act("White light from no particular source suddenly fills the room, "
	"then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
    return 0;
  }
  if (cvict!=NULL)
  if ((GET_LEVEL(caster) < LVL_GOD)&&(GET_MOB_SPEC(cvict) == postmaster)) {  
        send_to_char("Your spell dissipates as it hits the Postmaster.\r\n", caster);
        act("$n's magical power instantly dissipates as it hits $N.", FALSE, caster,0,cvict, TO_ROOM);
        return 0;
  }
  if (cvict!=NULL)
  if ((GET_LEVEL(caster) < LVL_GOD)&&(GET_MOB_SPEC(cvict) == receptionist)){
        send_to_char("Your spell dissipates as it hits the Receptionist.\r\n", caster);
        act("$n's magical power instantly dissipates as it hits $N.", FALSE, caster,0,cvict, TO_ROOM);
        return 0;
  }

  /* determine the type of saving throw */
  switch (casttype) {
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
    mag_damage(level, caster, cvict, spellnum, savetype);

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

  if (IS_SET(SINFO.routines, MAG_OBJECTS))
    mag_objects(level, caster, ovict, spellnum);

  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum) {
    case SPELL_ENCHANT_WEAPON:
      MANUAL_SPELL(spell_enchant_weapon);
      break;
    case SPELL_CHARM:
      MANUAL_SPELL(spell_charm);
      break;
    case SPELL_WORD_OF_RECALL:
      if (cvict != caster) cvict = caster;
      MANUAL_SPELL(spell_recall);
      break;
    case SPELL_TELEPORT:
      MANUAL_SPELL(spell_teleport);
      break;
    case SPELL_FINGERDEATH:
      MANUAL_SPELL(spell_fingerdeath);
      break;
    case SPELL_IDENTIFY:
      MANUAL_SPELL(spell_identify);
      break;
    case SPELL_SUMMON:
      MANUAL_SPELL(spell_summon);
      break;
    case SPELL_LOCATE_OBJECT:
      MANUAL_SPELL(spell_locate_object);
      break;
    case SPELL_CONTROL_WEATHER:
      MANUAL_SPELL(spell_control_weather);
      break;
    case SPELL_FEAR:
      MANUAL_SPELL(spell_fear);
      break;
    case SPELL_GATE:
      MANUAL_SPELL(spell_gate);
      break;
    }

  return 1;
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.
 *
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified.
 */

void mag_objectmagic(struct char_data * ch, struct obj_data * obj,
		          char *argument)
{
  int i, k;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;

  one_argument(argument, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		   FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_STAFF:
    act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
    if (obj->action_description)
      act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
    else
      act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
      act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
    } else {
      GET_OBJ_VAL(obj, 2)--;
      for (tch = world[ch->in_room].people; tch; tch = next_tch) {
	next_tch = tch->next_in_room;
	if (ch == tch)
	  continue;
	if (GET_OBJ_VAL(obj, 0))
	  call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3),
		     GET_OBJ_VAL(obj, 0), CAST_STAFF);
	else
	  call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3),
		     DEFAULT_STAFF_LVL, CAST_STAFF);
      }
    }
    break;
  case ITEM_WAND:
    if (k == FIND_CHAR_ROOM) {
      if (tch == ch) {
	act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
	act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
      } else {
	act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
	if (obj->action_description != NULL)
	  act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
	else
	  act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
      }
    } else if (tobj != NULL) {
      act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
      if (obj->action_description != NULL)
	act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
      else
	act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
    } else {
      act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
      return;
    }

    if (GET_OBJ_VAL(obj, 2) <= 0) {
      act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    GET_OBJ_VAL(obj, 2)--;
    if (GET_OBJ_VAL(obj, 0))
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 GET_OBJ_VAL(obj, 0), CAST_WAND);
    else
      call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3),
		 DEFAULT_WAND_LVL, CAST_WAND);
    break;
  case ITEM_SCROLL:
    if (*arg) {
      if (!k) {
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

    for (i = 1; i < 4; i++)
      if (!(call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_SCROLL)))
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

    for (i = 1; i < 4; i++)
      if (!(call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i),
		       GET_OBJ_VAL(obj, 0), CAST_POTION)))
	break;

    if (obj != NULL)
      extract_obj(obj);
    break;
  default:
    log("SYSERR: Unknown object_type in mag_objectmagic");
    break;
  }
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.
 */

int cast_spell(struct char_data * ch, struct char_data * tch,
		   struct obj_data * tobj, int spellnum)
{
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
    return 0;
  }
  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tch)) {
    send_to_char("You are afraid you might hurt your master!\r\n", ch);
    return 0;
  }
  if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY)) {
    send_to_char("You can only cast this spell upon yourself!\r\n", ch);
    return 0;
  }
  if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF)) {
    send_to_char("You cannot cast this spell upon yourself!\r\n", ch);
    return 0;
  }
  send_to_char(OK, ch);
  say_spell(ch, spellnum, tch, tobj);

  return (call_magic(ch, tch, tobj, spellnum, GET_LEVEL(ch), CAST_SPELL));
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
  int mana, spellnum, i, target = 0;

  if (IS_NPC(ch))
    return;

  /* get: blank, spell name, target name */
  s = strtok(argument, "'");

  if (s == NULL) {
    send_to_char("Cast what where?\r\n", ch);
    return;
  }
  s = strtok(NULL, "'");
  if (s == NULL) {
    send_to_char("Spell names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
    return;
  }
  t = strtok(NULL, "\0");

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum > MAX_SPELLS)) {
    send_to_char("Cast what?!?\r\n", ch);
    return;
  }
  if (GET_LEVEL(ch) < SINFO.min_level[0]) {
    send_to_char("You do not know that spell!\r\n", ch);
    return;
  }
  if (GET_SKILL(ch, spellnum) == 0) {
    send_to_char("You are unfamiliar with that spell.\r\n", ch);
    return;
  }

	/* check min stats required */
	if (!has_stats_for_skill(ch, spellnum))
		return;

/* Check to see if the caster is using a shield for lightning/fire shield - DM */
  if ((spellnum == SPELL_LIGHT_SHIELD) || (spellnum == SPELL_FIRE_SHIELD)) {
    if (!GET_EQ(ch,WEAR_SHIELD)) {
      send_to_char("You need to be using a shield to cast this.\r\n",ch);
      return;
    }           
  }

/* Check for alignment and existing protection for protection for evil/good - DM */
  if ((spellnum == SPELL_PROT_FROM_EVIL) || (spellnum == SPELL_PROT_FROM_GOOD)) {
    if (affected_by_spell(ch,SPELL_PROT_FROM_EVIL) || 
		affected_by_spell(ch,SPELL_PROT_FROM_GOOD)) {
       send_to_char("You already feel invulnerable.\r\n",ch);
       return;
    }

    /* Check the required aligns are right */
    if (spellnum == SPELL_PROT_FROM_EVIL) {
      if (!IS_GOOD(ch)) {
        send_to_char("If only you were a nice person.\r\n",ch);
        return;
      }
    } else {
      if (!IS_EVIL(ch)) {
        send_to_char("If only you were a little meaner.\r\n",ch);
        return;
      } 
    } 
  } 

  /* Find the target */
  if (t != NULL) {
    one_argument(strcpy(arg, t), t);
    skip_spaces(&t);
  }
  if (IS_SET(SINFO.targets, TAR_IGNORE)) {
    target = TRUE;
  } else if (t != NULL && *t) {
    if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM))) {
      if ((tch = get_char_room_vis(ch, t)) != NULL)
	target = TRUE;
    }
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, t,FALSE)))
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, t, ch->carrying)))
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP)) {
      for (i = 0; !target && i < NUM_WEARS; i++)
	if (ch->equipment[i] && !str_cmp(t, ch->equipment[i]->name)) {
	  tobj = ch->equipment[i];
	  target = TRUE;
	}
    }
    if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents)))
	target = TRUE;

    if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
      if ((tobj = get_obj_vis(ch, t)))
	target = TRUE;

  } else {			/* if target string is empty */
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL) {
	tch = ch;
	target = TRUE;
      }
    if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL) {
	tch = FIGHTING(ch);
	target = TRUE;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) &&
	!SINFO.violent) {
      tch = ch;
      target = TRUE;
    }
    if (!target) {
      sprintf(buf, "Upon %s should the spell be cast?\r\n",
	 IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD) ?
	      "what" : "who");
      send_to_char(buf, ch);
      return;
    }
  }

  if (target && (tch == ch) && SINFO.violent) {
    send_to_char("You shouldn't cast that on yourself -- could be bad for your health!\r\n", ch);
    return;
  }
  if (!target) {
    send_to_char("Cannot find the target of your spell!\r\n", ch);
    return;
  }
  mana = mag_manacost(ch, spellnum);
  if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_IS_GOD)) {
    send_to_char("You haven't the energy to cast that spell!\r\n", ch);
    return;
  }

  /* You throws the dice and you takes your chances.. 101% is total failure */
  if (number(0, 101) > GET_SKILL(ch, spellnum)) 
	{
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (!tch || !skill_message(0, ch, tch, spellnum))
      send_to_char("You lost your concentration!\r\n", ch);
    if (mana > 0)
		/* this is so it doesnt reset ya wolf/vamp mana bonus - Vader */
		if(affected_by_spell(ch,SPELL_CHANGED))
			GET_MANA(ch) = MAX(0, GET_MANA(ch) - (mana >> 1));
		else
			GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana >> 1)));
		if (SINFO.violent && tch && IS_NPC(tch))
			hit(tch, ch, TYPE_UNDEFINED);
	} 
	else 
	{
		if (cast_spell(ch, tch, tobj, spellnum)) 
		{
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (mana > 0)
			/* same as above - Vader */
				if(affected_by_spell(ch,SPELL_CHANGED))
					GET_MANA(ch) = MAX(0, GET_MANA(ch) - mana);
				else
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
    }
  }
}


/* Assign the spells on boot up */

void spello(int spl, int mlev, int clev, int tlev, int wlev,
	         int max_mana, int min_mana, int mana_change, int minpos,
	         int targets, int violent, int routines)
{

	FILE *fp;
	int str, intl, wis, dex, con, cha;
	char line[80];
	char spell_name[50];

	char *lp;
	char *cp;
  	


  spell_info[spl].min_level[0] = mlev;
  spell_info[spl].min_level[1] = clev;
  spell_info[spl].min_level[2] = tlev;
  spell_info[spl].min_level[3] = wlev;
  spell_info[spl].mana_max = max_mana;
  spell_info[spl].mana_min = min_mana;
  spell_info[spl].mana_change = mana_change;
  spell_info[spl].min_position = minpos;
  spell_info[spl].targets = targets;
  spell_info[spl].violent = violent;
  spell_info[spl].routines = routines;

	if (!(fp = fopen("etc/spell_stats", "r")))
	{
		fprintf(stderr, "Unable to open spell_stats file\n");
		goto default_stats;
	}


	while( 1 )
	{
        	lp = line;
		cp = spell_name;

		if (!fgets(line, sizeof(line), fp))
			goto default_stats;
		if (*line != '#')
		{
			/*fprintf(stderr, line);*/
			/* got a line now parse the name first */
			while (*lp != ':')
				*cp++ = *lp++;
			*cp = '\0';

			if (!strncmp(spell_name, spells[spl], 20))
			{
				/* parse stats */
				fclose(fp);

				/*fprintf(stderr, "Matched skill : %s %s\n", line, spells[spl]);*/
			
				while (!isdigit(*lp))
					lp++;
				str = atoi(lp);

				while (!isspace(*lp))
					lp++;
				intl = atoi(++lp);

				while (!isspace(*lp))
					lp++;
				wis = atoi(++lp);
	
				while (!isspace(*lp))
					lp++;
				dex = atoi(++lp);

				while (!isspace(*lp))
					lp++;
				con = atoi(++lp);

				while (!isspace(*lp))
					lp++;
				cha = atoi(++lp);
				/*fprintf(stderr, "Assigning to %s %d %d
                                 %d %d %d %d\n", spells[spl], str, intl,
                                 wis, dex, con, cha);*/

				spell_info[spl].str = str;
				spell_info[spl].intl = intl;
				spell_info[spl].wis = wis;
				spell_info[spl].dex = dex;
				spell_info[spl].con = con;
				spell_info[spl].cha = cha;
				/*fprintf(stderr, "Done\n");*/

				return;
			}
   		}
	}
			

	default_stats:
			fclose(fp);
			spell_info[spl].str = 3;
			spell_info[spl].intl = 3;
			spell_info[spl].wis = 3;
			spell_info[spl].dex = 3;
			spell_info[spl].con = 3;
			spell_info[spl].cha = 3;
}

/*
 * Arguments for spello calls:
 *
 * spellnum, levels (MCTW), maxmana, minmana, manachng, minpos, targets,
 * violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL). levels  :  Minimum level (mage, cleric,
 * thief, warrior) a player must be to cast this spell.  Use 'X' for immortal
 * only. maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell). minmana :  The minimum
 * mana this spell will take, no matter how high level the caster is.
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|'). violent :  TRUE or FALSE,
 * depending on if this is considered a violent spell and should not be cast
 * in PEACEFUL rooms or on yourself. routines:  A list of magic routines
 * which are associated with this spell. Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

#define UU (LVL_IMPL+1)
#define UNUSED UU,UU,UU,UU,0,0,0,0,0,0,0

#define X LVL_GOD

void mag_assign_spells(void)
{
  int i;

  for (i = 1; i <= TOP_SPELL_DEFINE; i++)
    spello(i, UNUSED);
  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */
  spello(SPELL_SERPENT_SKIN, 65, X, X, X, 200, 70, 10,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_MANA, X, X, X, X,  10, 10, 10, POS_STANDING,	
	     TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS);

  spello(SPELL_DRAGON, 11, X, X, X,  50, 20, 5,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_FLY, 53, X, X, X, 102, 52, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_WATERBREATHE, 45, X, X, X, 100, 40, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_GATE, 52, X, X, X, 130, 90, 2,
	 POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);

  spello(SPELL_FEAR, 35, 35, X, X, 40, 20, 2,
	 POS_FIGHTING, TAR_CHAR_ROOM, TRUE, MAG_MANUAL);

  spello(SPELL_WATERWALK, 28, 28, X, X, 50, 30, 2,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_FINGERDEATH, 50, 50, X, X, 250, 220, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL);

  spello(SPELL_TELEPORT, 35, 35, X, X, 30, 110, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL);

  spello(SPELL_ARMOR, 1, 1, X, X, 30, 15, 3,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_SPIRIT_ARMOR, 23, 23, X, X, 60, 42, 3,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_STONESKIN, 78, 78, X, X, 77, 47, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_LIGHT_SHIELD, 65, 65, X, X, 150, 85, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_FIRE_SHIELD, 75, 75, X, X, 175, 75, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_FIRE_WALL, 90, 90, X, X, 250, 147, 6,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_HASTE, 34, 34, X, X, 150, 120, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_HOLY_AID, 29, 29, X, X, 70, 40, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_DIVINE_PROTECTION, 37, 37, X, X, 80, 65, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_BLESS, 5, 5, X, X, 35, 5, 3,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_BLINDNESS, 6, 6, X, X, 35, 25, 1,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS);

  spello(SPELL_BURNING_HANDS, 5, X, X, X, 30, 10, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CALL_LIGHTNING, 15, 15, X, X, 40, 25, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CHARM, 16, X, X, X, 75, 50, 2,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL);

  spello(SPELL_CHILL_TOUCH, 3, X, X, X, 30, 10, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS);
  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */
  spello(SPELL_ANIMATE_DEAD, 20, X, X, X, 100, 80, 2,
         POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS);

  spello(SPELL_CLONE, 55, X, X, X, 145, 100, 2,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_SUMMONS);

  spello(SPELL_COLOR_SPRAY, 11, X, X, X, 30, 15, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_CONTROL_WEATHER, 17, 17, X, X, 75, 25, 5,
	 POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL);

  spello(SPELL_CREATE_FOOD, 2, 2, X, X, 30, 5, 4,
	 POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS);

  spello(SPELL_CREATE_WATER, 2, 2, X, X, 30, 5, 4,
	 POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_OBJECTS);

  spello(SPELL_CURE_BLIND, 2, 4, X, X, 30, 5, 2,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS);

  spello(SPELL_CURE_CRITIC, 9, 9, X, X, 30, 10, 2,
	 POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS);
  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */
  spello(SPELL_CURE_LIGHT, 1, 1, X, X, 30, 10, 2,
	 POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_CURSE, 14, X, X, X, 80, 50, 2,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, TRUE, MAG_AFFECTS);

  spello(SPELL_DETECT_ALIGN, 4, 4, X, X, 20, 10, 2,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_DETECT_INVIS, 2, 6, X, X, 20, 15, 2,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_DETECT_MAGIC, 2, X, X, X, 20, 10, 2,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);
  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */
  spello(SPELL_DETECT_POISON, 3, 3, X, X, 15, 5, 1,
	 POS_STANDING, TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_DISPEL_EVIL, 14, 14, X, X, 40, 25, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_DISPEL_GOOD, 14, 14, X, X, 40, 25, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_EARTHQUAKE, 12, 12, X, X, 40, 25, 3,
	 POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);

  spello(SPELL_WHIRLWIND, 36, 36, X, X, 80, 50, 3,
	 POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);

  spello(SPELL_ENCHANT_WEAPON, 26, X, X, X, 150, 100, 10,
	 POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL);

  spello(SPELL_ENERGY_DRAIN, 13, X, X, X, 40, 25, 1,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL);

  spello(SPELL_GROUP_ARMOR, 9, 9, X, X, 70, 60, 2,
	 POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS);
  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */
  spello(SPELL_FIREBALL, 15, X, X, X, 40, 30, 2,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_GROUP_HEAL, 22, 22, X, X, 130, 110, 5,
	 POS_FIGHTING, TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_HARM, 19, 19, X, X, 75, 45, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_HEAL, 16, 16, X, X, 80, 40, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS | MAG_UNAFFECTS);

  spello(SPELL_ADV_HEAL, 35, 35, X, X, 130, 70, 2,
	 POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS | MAG_UNAFFECTS);

  spello(SPELL_DIVINE_HEAL, 70, 70, X, X, 175, 100, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_AFFECTS | MAG_UNAFFECTS);

  spello(SPELL_REFRESH, 18, 18, X, X, 60, 30, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS);

  spello(SPELL_INFRAVISION, 3, 7, X, X, 30, 20, 1,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_INVISIBLE, 4, X, X, X, 40, 30, 1,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_LIGHTNING_BOLT, 9, X, X, X, 30, 15, 1,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_NOHASSLE, 80, X, X, X, 476, 476, 0,
	POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);
 
  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */
  spello(SPELL_LOCATE_OBJECT, 6, X, X, X, 30, 20, 1,
	 POS_STANDING, TAR_OBJ_WORLD, FALSE, MAG_MANUAL);

  spello(SPELL_MAGIC_MISSILE, 1, X, X, X, 25, 10, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_POISON, X, X, X, X, 50, 20, 3,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE, MAG_AFFECTS);

  spello(SPELL_PROT_FROM_EVIL, 33, 33, X, X, 123, 123, 0,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_PROT_FROM_GOOD, 38, 38, X, X, 133, 133, 0,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS);

  spello(SPELL_REMOVE_CURSE, 26, 26, X, X, 45, 25, 5,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS);

  spello(SPELL_SANCTUARY, 15, 15, X, X, 110, 100, 5,
	 POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_SHOCKING_GRASP, 7, X, X, X, 30, 15, 3,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);
  /* C L A S S E S      M A N A   */
  /* Ma  Cl  Th  Wa   Max Min Chn */
  spello(SPELL_SLEEP, 8, X, X, X, 40, 25, 5,
	 POS_STANDING, TAR_CHAR_ROOM, TRUE, MAG_AFFECTS);

  spello(SPELL_STRENGTH, 6, X, X, X, 35, 30, 1,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_SUMMON, 10, 10, X, X, 75, 60, 3,
	 POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL);

  spello(SPELL_WORD_OF_RECALL, 12, 12, X, X, 20, 10, 2,
	 POS_FIGHTING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_MANUAL);

  spello(SPELL_IDENTIFY, 37, X, X, X, 50, 25, 2, 
         POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);

  spello(SPELL_REMOVE_POISON, 10, 10, X, X, 40, 8, 4,
	 POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_UNAFFECTS);

  spello(SPELL_SENSE_LIFE, 20, 20, 20, 20, 45, 20, 3,
	 POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS);

  spello(SPELL_METEOR_SWARM, 37, 37, X, X, 80, 70, 3,
	 POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);

  spello(SPELL_CLOUD_KILL, 69, 69, X, X, 250, 250, 3,
         POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS);

  spello(SPELL_GROUP_SANCTUARY, 30, 30, X, X, 150, 120, 5,
	 POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS);

  spello(SPELL_PLASMA_BLAST, 30, X, X, X, 80, 50, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);

  spello(SPELL_PARALYZE, 32, 32, X, X, 50, 30, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS);

  spello(SPELL_REMOVE_PARA, 32, 32, X, X, 50, 30, 5,
         POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS);

  spello(SPELL_WRAITH_TOUCH, 65, X, X, X, 200, 150, 2,
         POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE);


  /*
   * SKILLS
   * 
   * The only parameters needed for skills are only the minimum levels for each
   * class.  The remaining 8 fields of the structure should be filled with
   * 0's.
   */

  /* Ma  Cl  Th  Wa  */
  spello(SKILL_PRIMAL_SCREAM, 43, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_SPY, 17, X, X, X, 
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_SCAN, 7, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_2ND_ATTACK, 35, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_3RD_ATTACK, 50, X, X, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_BACKSTAB, 3, X, 3, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_HIDE, 5, X, 5, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_KICK, 1, X, X, 1,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_PICK_LOCK, 2, X, 2, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_RESCUE, 3, X, X, 3,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_SNEAK, 1, X, 1, X,
	 0, 0, 0, 0, 0, 0, 0);

  spello(SKILL_STEAL, 4, X, 4, X,
	 0, 0, 0, 0, 0, 0, 0);
  /* Ma  Cl  Th  Wa  */
  spello(SKILL_TRACK, 6, X, 6, 9,
	 0, 0, 0, 0, 0, 0, 0);
  spello(SKILL_HUNT, 24, X, X, X,
         0, 0, 0, 0, 0, 0, 0);
 spello(SKILL_BASH, 40, X, X, X,
         0, 0, 0, 0, 0, 0, 0);
 spello(SKILL_RETREAT, 58, X, X, X,
         0, 0, 0, 0, 0, 0, 0);
 spello(SKILL_COMPARE, 5, X, X, X,
         0, 0, 0, 0, 0, 0, 0);

//  spello(SPELL_IDENTIFY, 0, 0, 0, 0, 0, 0, 0, 0,
//	 TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL);

}
