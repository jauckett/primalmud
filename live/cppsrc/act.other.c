/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "corpses.h"
#include "dg_scripts.h"

/* extern variables */
extern CorpseData corpseData;
extern const char *pc_class_types[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern char nameserver_is_slow;
extern int auto_save;
extern int track_through_doors;
extern int world_start_room[NUM_WORLDS];
extern void look_at_char (struct char_data *i, struct char_data *ch);
extern struct zone_data *zone_table;

/* extern procedures */
void list_skills (struct char_data *ch);
void appear (struct char_data *ch);
void write_aliases (struct char_data *ch);
void perform_immort_vis (struct char_data *ch);
SPECIAL (shop_keeper);
ACMD (do_gen_comm);
void Crash_rentsave (struct char_data *ch, int cost);
void ShowRemortsToChar (struct char_data *ch);
void ability_from_char (struct char_data *ch, int type);
bool add_to_quest (struct event_data *ev, struct char_data *ch);
void add_mud_event (struct event_data *ev);

struct char_data *get_char_online (struct char_data *ch, char *name,
				   int where);
struct char_data *get_player_by_id (long idnum);
struct event_data *find_quest_event ();

void set_race_specials (struct char_data *ch);
void set_class_specials (struct char_data *ch);
bool basic_skill_test (struct char_data *ch, int spellnum, bool mod_abil);
int invalid_level (struct char_data *ch, struct obj_data *object,
		   bool display);

/* local functions */
ACMD (do_combine);
ACMD (do_torch);
ACMD (do_compost);
ACMD (do_corpse);
ACMD (do_charge);
ACMD (do_ritual);
ACMD (do_memorise);
ACMD (do_disguise);
ACMD (do_pagewidth);
ACMD (do_pagelength);
ACMD (do_remort);
ACMD (do_quit);
ACMD (do_save);
ACMD (do_not_here);
ACMD (do_sneak);
ACMD (do_hide);
ACMD (do_steal);
ACMD (do_practice);
ACMD (do_visible);
ACMD (do_title);
int perform_group (struct char_data *ch, struct char_data *vict);
void print_group (struct char_data *ch);
ACMD (do_group);
ACMD (do_ungroup);
ACMD (do_report);
ACMD (do_split);
ACMD (do_use);
ACMD (do_wimpy);
ACMD (do_display);
// ACMD(do_gen_write);
ACMD (do_gen_tog);
ACMD (do_purse);
ACMD (do_suicide);
ACMD (do_modifiers);

extern struct obj_data *obj_proto;

int
numInContainer (struct obj_data *aCont, int vnum)
{
  struct obj_data *itr = aCont->contains;
  int number = 0;
  basic_mud_log ("Iterating through %s\n", aCont->name);
  while (itr != NULL)
    {
      basic_mud_log ("Checking %d", itr->item_number);
      if (itr->item_number == real_object (vnum))
	{
	  number++;
	}
      itr = itr->next_content;
    }
  basic_mud_log ("Found %d %d's in container", number, vnum);
  return number;
}

bool
canMake (ObjProductClass opc, struct obj_data * obj, char_data * ch)
{
  int i = real_object (opc.vnum);
  list < ObjMaterialClass > *materials = obj_proto[i].materials;
  // first check if the container holds the right materials
  list < ObjMaterialClass >::iterator itrMat = materials->begin ();
  // iterate thru each material, and check number in the bag
  while (materials->end () != itrMat)
    {
      if (numInContainer (obj, itrMat->vnum) != itrMat->number)
	{
	  basic_mud_log ("didn't find enough %d's in bag", itrMat->vnum);
	  delete materials;
	  return 0;
	}
      itrMat++;
    }
  // check skills and level

  return 1;
}

void
removeFromContainer (obj_data * aCont, int materialVnum)
{
  basic_mud_log ("Removing %d from %s\n", materialVnum, aCont->name);
  obj_data *oitr = aCont->contains;
  while (oitr != NULL)
    {
      basic_mud_log ("Checking iterm in container = %d", oitr->item_number);
      if (oitr->item_number == real_object (materialVnum))
	{
	  basic_mud_log ("Found %d in container", oitr->item_number);
	  obj_from_obj (oitr);
	  break;
	}
      oitr = oitr->next_content;
    }
}

void
removeMaterials (obj_data * aCont, list < ObjMaterialClass > *materials,
		 bool success)
{
  bool removeObject = false;
  // iterate through the list of required materials
  list < ObjMaterialClass >::iterator itrMat = materials->begin ();
  while (materials->end () != itrMat)
    {
      basic_mud_log ("Checking material %d to remove from %s", itrMat->vnum,
		     aCont->name);
//    int materialVnum = itrMat->vnum;
      if (success)
	{			// check if the object should be remove on sucess
	  if (itrMat->consumedOnSuccess)
	    {
	      removeObject = true;
	    }
	}
      else
	{
	  if (itrMat->consumedOnFail)
	    {
	      removeObject = true;
	    }
	}
      if (removeObject)
	{
	  for (int i = 0; i < itrMat->number; i++)
	    {
	      removeFromContainer (aCont, itrMat->vnum);
	    }
	}
      itrMat++;
    }
}

int
skillSuccess (char_data * ch, int productSkill, int productLevel)
{
  int charSkillLevel = GET_SKILL (ch, productSkill);
#ifndef IGNORE_DEBUG
  if (GET_DEBUG (ch))
    {
      sprintf (buf2, "Your skill in %d is %d\r\n", productSkill,
	       charSkillLevel);
      send_to_char (buf2, ch);
    }
#endif
  if (charSkillLevel == 0)
    {
      send_to_char ("You don't know how.\r\n", ch);
      return -1;
    }
  int chance = charSkillLevel * 10;	// scale to 0..1000 
#ifndef IGNORE_DEBUG
  if (GET_DEBUG (ch))
    {
      sprintf (buf2, "chlvl=%d itemlvl=%d \r\n", GET_LEVEL (ch),
	       productLevel);
      send_to_char (buf2, ch);
    }
#endif
  if (GET_LEVEL (ch) < productLevel)
    {
      // char level less so work out the reduced chance of success
      // chance = chance - leveldiff squared
      chance -=
	(productLevel - GET_LEVEL (ch)) * (productLevel - GET_LEVEL (ch));
    }
  else
    {
      // increased chance of sucess if you are higher level than the object
      chance += (GET_LEVEL (ch) - productLevel);
    }
#ifndef IGNORE_DEBUG
  if (GET_DEBUG (ch))
    {
      sprintf (buf2, "Your skill in %d is %d after level diff\r\n",
	       productSkill, chance);
      send_to_char (buf2, ch);
    }
#endif


  if (chance > 995)
    {				// always a small chance of failure
      chance = 995;		// 0.5% chance
    }
  if (chance <= 5)
    {				// always a small chance of sucess
      chance = 5;
    }

  int roll = number (1, 1000);
  if (roll > chance)
    {
#ifndef IGNORE_DEBUG
      if (GET_DEBUG (ch))
	{
	  sprintf (buf2, "Returning false Roll was %d  chance was %d\r\n",
		   roll, chance);
	  send_to_char (buf2, ch);
	}
#endif
      return 0;
    }
#ifndef IGNORE_DEBUG
  if (GET_DEBUG (ch))
    {
      sprintf (buf2, "Returning true Roll was %d  chance was %d\r\n", roll,
	       chance);
      send_to_char (buf2, ch);
    }
#endif
  apply_spell_skill_abil (ch, productSkill);
  return 1;
}

ACMD (do_combine)
{
  struct obj_data *obj = NULL;
  one_argument (argument, arg);
  if (!*arg)
    send_to_char ("Combine what?\r\n", ch);
  else if (!(obj = find_obj_list (ch, arg, ch->carrying)))
    {
      sprintf (buf, "You don't seem to have %s %s.\r\n", AN (arg), arg);
      send_to_char (buf, ch);
    }
  else
    {
      if (obj->products == NULL)
	{
	  send_to_char ("That item cannot be used to make things.\r\n", ch);
	  return;
	}
    }

  // check if container holds a valid combination of materials
  list < ObjProductClass >::iterator itrProd = obj->products->begin ();
  int newObjectVnum = 0;
  while (obj->products->end () != itrProd)
    {
      basic_mud_log ("Checking makable for %d", (*itrProd).vnum);
      if (canMake (*itrProd, obj, ch))
	{
	  newObjectVnum = itrProd->vnum;
	  break;
	}
      itrProd++;
    }
  if (!newObjectVnum)
    {
      send_to_char
	("You do not have the right materials to produce anything.\r\n", ch);
      return;
    }
  // check the skill here
  int wasSuccessful = skillSuccess (ch, itrProd->skill, itrProd->level);
  if (wasSuccessful == -1)
    {				// didn't know how to do this skill
      return;
    }
  obj_data *newobj = NULL;
  if (wasSuccessful)
    {
      newobj = read_object ((*itrProd).vnum, VIRTUAL);
      int load_into_inventory = 1;
      if (load_into_inventory)
	obj_to_char (newobj, ch, __FILE__, __LINE__);
      else
	obj_to_room (newobj, ch->in_room);
//    removeMaterials(obj, obj_proto[real_object(newObjectVnum)].materials, wasSuccessful);
      sprintf (buf2, "You combine the contents of the %s to make a %s\r\n",
	       obj->name, newobj->name);
      send_to_char (buf2, ch);
      sprintf (buf2, "$n has fashioned a %s\r\n", newobj->name);
      act (buf2, FALSE, ch, 0, NULL, TO_ROOM);
      return;
    }
  sprintf (buf2, "You failed to fashion anything.\r\n");
  send_to_char (buf2, ch);
  return;
}


ACMD (do_modifiers)
{
  struct char_data *vict = ch;
  float elitist_mod = 0;
  float unholiness_mod = 0;

  if (IS_NPC (ch))
    return;

  one_argument (argument, arg);

  if (*arg && !LR_FAIL (ch, LVL_IS_GOD))
    if (generic_find_char (ch, arg, FIND_CHAR_WORLD) != NULL)
      vict = generic_find_char (ch, arg, FIND_CHAR_WORLD);

  sprintf (buf, "\r\nModifiers for &7%s&n:\r\n", GET_NAME (vict));
  send_to_char (buf, ch);
  sprintf (buf, "&b==============================\r\n"
	   "&b| &gRace Modifier:       &c%5.3f&b |\r\n"
	   "&b| &gClass Modifier:      &c%5.3f&b |\r\n"
	   "&b| &gSpecials Modifier:   &c%5.3f&b |\r\n",
	   race_modifiers[GET_RACE (vict)],
	   class_modifiers[(int) GET_CLASS (vict)], special_modifier (vict));

  elitist_mod = elitist_modifier (vict);
  if (elitist_mod < 0)
    sprintf (buf, "%s&b| &gElitist Bonus:      &c%6.3f&b |\r\n", buf,
	     elitist_mod);
  unholiness_mod = unholiness_modifier (vict);
  if (unholiness_mod < 0)
    sprintf (buf, "%s&b| &gUnholy Modifier:    &c%6.3f&b |\r\n", buf,
	     unholiness_mod);

  send_to_char (buf, ch);

  send_to_char ("&b==============================&n\r\n", ch);
  sprintf (buf, "&b| &gModifier Total:     &c%6.3f&b |\r\n",
	   GET_MODIFIER (vict));
  send_to_char (buf, ch);
  send_to_char ("&b==============================\r\n"
		"           &GSpecials           \r\n"
		"&b==============================\r\n", ch);
  long lSpecials = GET_SPECIALS (vict);

  if (IS_SET (lSpecials, SPECIAL_BACKSTAB))	// 12%
    send_to_char ("&b| &gBackstab             &c0.120&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_BATTLEMAGE))	// 7%
    send_to_char ("&b| &gBattle Mage          &c0.070&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_CHARMER))	// 3%
    send_to_char ("&b| &gCharmer              &c0.030&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_DISGUISE))	// 1%
    send_to_char ("&b| &gDisguise             &c0.010&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_DWARF))	// 2%
    send_to_char ("&b| &gDwarf                &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_ELF))	// 5%
    send_to_char ("&b| &gElf                  &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_EMPATH))	// 1%
    send_to_char ("&b| &gEmpath               &c0.010&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_ESCAPE))	// 1%
    send_to_char ("&b| &gEscape               &c0.010&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_FLY))	// 2%
    send_to_char ("&b| &gFly                  &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_FOREST_SPELLS))	// 2%
    send_to_char ("&b| &gForest Spells        &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_FOREST_HELP))	// 2%
    send_to_char ("&b| &gForest Aid           &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_GORE))	// 5%
    send_to_char ("&b| &gGore                 &c0.050&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_PRIEST))	// %2
    send_to_char ("&b| &gGreed                &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_GROUP_SNEAK))	// 3%
    send_to_char ("&b| &gGroup Sneak          &c0.030&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_HEALER))	// 12%
    send_to_char ("&b| &gHealer               &c0.120&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_HOLY))	// 3%
    send_to_char ("&b| &gHoly                 &c0.030&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_INFRA))	// 2%
    send_to_char ("&b| &gInfravision          &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_INVIS))	// 2%
    send_to_char ("&b| &gInvisibility         &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_MANA_THIEF))	// 5%
    send_to_char ("&b| &gMana Thief           &c0.050&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_MINOTAUR))	// 8%
    send_to_char ("&b| &gMinotaur             &c0.080&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_MULTIWEAPON))	// 9%
    send_to_char ("&b| &gMultiWeapon          &c0.090&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_SNEAK))	// 2%
    send_to_char ("&b| &gSneak                &c0.020&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_SUPERMAN))	// 4%
    send_to_char ("&b| &gSuperman             &c0.040&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_THIEF))	// 5%
    send_to_char ("&b| &gThief                &c0.050&b |\r\n", ch);
  if (IS_SET (lSpecials, SPECIAL_TRACKER))	// 3%
    send_to_char ("&b| &gTracker              &c0.030&b |\r\n", ch);
  send_to_char ("&b==============================\r\n", ch);
}

ACMD (do_suicide)
{
  struct obj_data *weapon = NULL;

  if (IS_NPC (ch))
    {
      return;
    }

  // DM - must be wielding a weapon
  if ((weapon = GET_EQ (ch, WEAR_WIELD)))
    {
      if (!basic_skill_test (ch, SKILL_SUICIDE, TRUE))
	{
	  return;
	}

      sprintf (buf, "$n uses %s to take $s own life!",
	       weapon->short_description);
      sprintf (buf1, "You use %s to take your own life!",
	       weapon->short_description);

      act (buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char (buf1, ch);

      sprintf (buf, "%s has killed %sself. (Lost %d Exp)", GET_NAME (ch),
	       HMHR (ch), (int) (GET_EXP (ch) / 10));
      mudlog (buf, NRM, LVL_ANGEL, TRUE);
      sprintf (buf, "%s has killed %sself.", GET_NAME (ch), HMHR (ch));
      info_channel (buf, ch);

      // need some loss - loose 1/10 exp?
      GET_EXP (ch) = (int) (GET_EXP (ch) * 0.9);
      raw_kill (ch, ch);
    }
  else
    {
      send_to_char
	("Before you go killing yourself, how about finding something to do it with.\r\n",
	 ch);
    }
}

// DM TODO: create a new firey torch obj
#define TORCH_VNUM 1108

ACMD (do_torch)
{
  struct obj_data *torch = NULL;
  void handle_fireball (struct char_data *ch);

  if (IS_NPC (ch))
    {
      return;
    }

  if (GET_EQ (ch, WEAR_LIGHT))
    {
      send_to_char ("You are already using a light.\r\n", ch);
      return;
    }

  // check room type - hills/field/forrest
  if (SECT (ch->in_room) == SECT_FIELD || SECT (ch->in_room) == SECT_FOREST ||
      SECT (ch->in_room) == SECT_HILLS)
    {
      // Artus> Chance of torch backfiring.. Muhahahahhahahaha!
      if (number (1, 101) > GET_SKILL (ch, SKILL_TORCH))
	{
	  send_to_char
	    ("&rYou light the torch, which goes up in a ball of flame.&n\r\n",
	     ch);
	  act ("$n lights a torch which goes up in a ball of flames!", FALSE,
	       ch, 0, ch, TO_NOTVICT);
	  damage (NULL, ch, GET_HIT (ch) / 20, TYPE_UNDEFINED, FALSE);
	  handle_fireball (ch);	// Artus> Gotta love this :o)
	  return;
	}
      apply_spell_skill_abil (ch, SKILL_TORCH);
      // Create the torch and equip the character
      CREATE (torch, struct obj_data, 1);
      if (!(torch = read_object (TORCH_VNUM, VIRTUAL)))
	{
	  send_to_char
	    ("Hmm, something is wrong. You should report this.\r\n", ch);
	  sprintf (buf, "SYSERR: do_torch: obj %d not found", TORCH_VNUM);
	  mudlog (buf, BRF, LVL_ANGEL, TRUE);
	  free (torch);
	  return;
	}
      // length is based on class effeciency and skill knowledge
      GET_OBJ_VAL (torch, 2) = MAX (3, (int) ((GET_SKILL (ch, SKILL_TORCH) *
					       SPELL_EFFEC (ch,
							    SKILL_TORCH)) /
					      1000));
#ifndef IGNORE_DEBUG
      if (GET_DEBUG (ch))
	{
	  sprintf (buf, "&[&gDEBUG: %d/%d/%d&]\r\n",
		   GET_SKILL (ch, SKILL_TORCH), SPELL_EFFEC (ch, SKILL_TORCH),
		   (int) (GET_SKILL (ch, SKILL_TORCH) *
			  SPELL_EFFEC (ch, SKILL_TORCH) / 1000));
	  send_to_char (buf, ch);
	}
#endif


      if (equip_char (ch, torch, WEAR_LIGHT, TRUE))
	{
	  // DM TODO: cooler messges?
	  act ("You create and hold your new firey torch.",
	       FALSE, ch, 0, 0, TO_CHAR);
	  act ("$n creates and holds a firey torch.",
	       FALSE, ch, 0, ch, TO_NOTVICT);
	}
      else
	{
	  send_to_char
	    ("Hmm, something is wrong. You should report this.\r\n", ch);
	  sprintf (buf, "SYSERR: equiping char in do_torch(%d).", TORCH_VNUM);
	  mudlog (buf, BRF, LVL_ANGEL, TRUE);
	  free (torch);
	}

    }
  else
    {
      send_to_char ("You cannot acquire the resources you need here.\r\n",
		    ch);
      return;
    }
}

ACMD (do_meditate)
{
  int timer;

  if (IS_NPC (ch))
    {
      return;
    }

  switch (subcmd)
    {
    case SKILL_MEDITATE:
      timer = TIMER_MEDITATE;
      break;
    case SKILL_HEAL_TRANCE:
      timer = TIMER_HEAL_TRANCE;
      break;
    default:
      send_to_char ("Bug in do_meditate. Please report.\r\n", ch);
      return;
    }

  if (char_affected_by_timer (ch, TIMER_MEDITATE))
    {
      send_to_char ("Stop mucking around and get back to meditating.\r\n",
		    ch);
      return;
    }
  if (char_affected_by_timer (ch, TIMER_HEAL_TRANCE))
    {
      send_to_char ("You're too busy concentrating on your healing.\r\n", ch);
      return;
    }

  if (!basic_skill_test (ch, subcmd, TRUE))
    return;

  timer_to_char (ch, timer_new (timer));
  GET_POS (ch) = POS_SITTING;
  switch (subcmd)
    {
    case SKILL_MEDITATE:
      act ("You go into a deep meditating rest.", FALSE, ch, 0, NULL,
	   TO_CHAR);
      act ("$n begins to meditate.", FALSE, ch, 0, NULL, TO_ROOM);
      break;
    case SKILL_HEAL_TRANCE:
      act ("You go into a deep healing trance.", FALSE, ch, 0, NULL, TO_CHAR);
      act ("$n falls into a deep healing trance.", FALSE, ch, 0, NULL,
	   TO_ROOM);
      break;
    default:
      act ("You rest yourself.", FALSE, ch, 0, NULL, TO_CHAR);
    }
}

/*
 * DM: Corpse utility function.
 *
 * Currently: copyall - extracts (copys) all corpses in the corpseData object,
 *                      and loads them to the room the char is in.
 *
 *            deleteAll - removes all corpses in the corpseData object.
 *
 * Idea is that this be used for further expansion:
 *  - automatic transfer of corpses to char etc ....
 */
ACMD (do_corpse)
{
  if (IS_NPC (ch))
    {
      return;
    }

  two_arguments (argument, buf, buf1);
  // Copy all corpses in CorpseData object to ch's existing room
  if (!str_cmp (buf, "copyall"))
    {
      corpseData.extractCorpses (GET_ROOM_VNUM (ch->in_room), FALSE);
      sprintf (buf, "CORPSE: %s extracted all corpses to room %d",
	       GET_NAME (ch), GET_ROOM_VNUM (ch->in_room));
      mudlog (buf, NRM, MAX (LVL_ANGEL, GET_INVIS_LEV (ch)), TRUE);
      act ("$n utters some magic words 'copy all corpses' :)",
	   TRUE, ch, 0, 0, TO_ROOM);
      send_to_char ("All corpses copied to room.\r\n", ch);
      // Delete all corpses in the CorpseData object 
    }
  else if (!str_cmp (buf, "deleteall"))
    {
      corpseData.removeAllCorpses ();
      sprintf (buf, "CORPSE: %s deleted all corpses from CorpseData object",
	       GET_NAME (ch));
      mudlog (buf, NRM, MAX (LVL_ANGEL, GET_INVIS_LEV (ch)), TRUE);
      send_to_char ("All saved corpses deleted.\r\n", ch);
    }
  else
    {
      send_to_char ("Usage: corpse { copyall deleteall }\r\n", ch);
    }
}

ACMD (do_poisonblade)
{
  struct obj_data *wielded = GET_EQ (ch, WEAR_WIELD);
  struct obj_data *obj, *next_obj;

  if (IS_NPC (ch))
    return;

  // check if we are using the right type of weapon ...
  if (wielded == NULL)
    {
      send_to_char ("You arn't even wielding a weapon.\r\n", ch);
      return;
    }
  else
    {
      if (!(GET_OBJ_TYPE (wielded) == ITEM_WEAPON) &&
	  (GET_OBJ_VAL (wielded, 3) == TYPE_SLASH ||
	   GET_OBJ_VAL (wielded, 3) == TYPE_CLAW ||
	   GET_OBJ_VAL (wielded, 3) == TYPE_PIERCE))
	{
	  send_to_char ("You don't seem to be using the right weapon.\r\n",
			ch);
	  return;
	}
    }

  // now see if we have a poison item in inventory
  for (obj = ch->carrying; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if (GET_OBJ_TYPE (obj) == ITEM_DRINKCON && GET_OBJ_VAL (obj, 3) != 0
	  && GET_OBJ_VAL (obj, 1) > 0)
	{
	  if (basic_skill_test (ch, SKILL_POISONBLADE, 1))
	    {
	      GET_OBJ_VAL (obj, 1)--;
	      timer_to_obj (wielded, timer_new (TIMER_POISONBLADE));
	      send_to_char ("You coat your weapon in a poisonous venom.\r\n",
			    ch);
	      return;
	    }
	  else
	    {
	      return;
	    }
	}
    }

  send_to_char ("You're missing a vital ingredient.\r\n", ch);
}

ACMD (do_ritual)
{
  struct timer_type *tim;

  if (IS_NPC (ch))
    return;

  if (!has_stats_for_skill (ch, SKILL_DARKRITUAL, TRUE))
    return;

  // attach timer if it doesn't exist ...
  if (!(char_affected_by_timer (ch, TIMER_DARKRITUAL)))
    {
      tim = timer_new (TIMER_DARKRITUAL);
      timer_to_char (ch, tim);
    }

  if (timer_use_char (ch, TIMER_DARKRITUAL))
    {
      // If they're not evil, they'll get spanked for doing this
      if (GET_ALIGNMENT (ch) > -350)
	{
	  if (GET_HIT (ch) > 1)
	    {
	      act ("$n twists and burns as $s utters evil incantations.",
		   FALSE, ch, 0, 0, TO_ROOM);
	      send_to_char
		("As you utter the evil incantations, you twist and suffer.\r\n",
		 ch);
	      GET_HIT (ch) -= ((int) (GET_MAX_HIT (ch) * 0.25));
	      if (GET_HIT (ch) <= 0)
		GET_HIT (ch) = 1;
	      return;
	    }

	  // Oh oh, they tried it while critically hurt. Bye bye.
	  send_to_char
	    ("As you pray to evil gods to save you, your soul twists and suffers\r\nbeyond the point of return.\r\n",
	     ch);
	  act ("$n whispers dark names on $s dying breath.", FALSE, ch, 0, 0,
	       TO_ROOM);
	  raw_kill (ch, NULL);
	  return;
	}

      if (GET_MANA (ch) > GET_MAX_MANA (ch))
	{
	  send_to_char
	    ("Your dark gods drain you of life as you greedily ask for more.\r\n",
	     ch);
	  act ("$n suffers greatly as shadows drain $s life force.", FALSE,
	       ch, 0, 0, TO_ROOM);
	  GET_HIT (ch) -= ((int) (GET_MAX_HIT (ch) / 3));
	  if (GET_HIT (ch) < 0)
	    GET_HIT (ch) = 0;
	  return;
	}
      if (GET_MANA (ch) == 0)
	{
	  send_to_char ("Your dark masters require some base power.\r\n", ch);
	  act ("$n whispers dark words, but nothing seems to happen.",
	       FALSE, ch, 0, 0, TO_ROOM);
	  return;
	}

      // Check that the player has enough lifeforce to trade in
      if (GET_HIT (ch) < ((int) (GET_MAX_HIT (ch) / 3)) + 1)
	{
	  send_to_char
	    ("You feel a faint stirring, but you haven't enough life force to offer in return.\r\n",
	     ch);
	  act ("$n whispers dark words, but nothing seems to happen.", FALSE,
	       ch, 0, 0, TO_ROOM);
	  return;
	}

      send_to_char
	("Your dark lords imbue your spirit with power while draining your body.\r\n",
	 ch);
      act ("$n wilts and then begins to glow with radiant power.", FALSE, ch,
	   0, 0, TO_ROOM);

      GET_HIT (ch) -= ((int) (GET_MAX_MANA (ch) / 3));

      // Sanctuary and mana increasing 
      // DM - was mana doubling before I coverted this to a skill
      //    - slack Tali, slack :) ...
      struct affected_type af;
      af.type = SPELL_SANCTUARY;
      af.duration = ((int) (GET_CHA (ch) / 2));
      af.modifier = 0;
      af.location = 0;
      af.bitvector = AFF_SANCTUARY;
      affect_to_char (ch, &af);

      GET_MANA (ch) =
	(int) (GET_MANA (ch) * (GET_SKILL (ch, SKILL_DARKRITUAL) + 100) /
	       100);

      apply_spell_skill_abil (ch, SKILL_DARKRITUAL);
    }
  else
    {
      send_to_char
	("You must rest before you convene with the dark lords again.\r\n",
	 ch);
    }
}


/* Charm function for SPECIAL_CHARMER flagged pc's */
void
charm_room (struct char_data *ch)
{
  int room = ch->in_room;
  struct char_data *vict;
  struct affected_type af;

  if (room == NOWHERE)
    return;

  for (vict = world[room].people; vict; vict = vict->next_in_room)
    {
      if (vict == ch || !IS_NPC (vict))
	continue;

      if (vict->in_room != room)
	break;			// Exceeded room, shouldnt' need this, but 
      // i refuse to let it do over 3k loops for no reason
      if (GET_CLASS (vict) == CLASS_DEMIHUMAN
	  && !AFF_FLAGGED (vict, AFF_CHARM)
	  && !MOB_FLAGGED (vict, MOB_NOCHARM))
	{
	  // Make sure they're not too powerful
	  if (GET_LEVEL (vict) > (GET_LEVEL (ch) * 1.10))
	    continue;

	  if (number (0, 25 - GET_CHA (ch)) == 0)
	    {
	      if (vict->master)
		stop_follower (vict);
	      add_follower (vict, ch);
	      af.type = SPELL_CHARM;

	      af.duration = GET_CHA (ch) * 2;
	      af.modifier = 0;
	      af.location = 0;
	      af.bitvector = AFF_CHARM;
	      affect_to_char (vict, &af);

	      act ("$n's charm gets the better of you.", FALSE, ch, 0, vict,
		   TO_VICT);
	      act ("Your natural charms influence $N to join your cause.\r\n",
		   FALSE, ch, 0, vict, TO_CHAR);
	      if (IS_NPC (vict))
		{
		  REMOVE_BIT (MOB_FLAGS (vict), MOB_AGGRESSIVE);
		  REMOVE_BIT (MOB_FLAGS (vict), MOB_SPEC);
		}
	    }
	}
    }
}

ACMD (do_memorise)
{
  struct char_data *tch = NULL;
  struct obj_data *obj;
  if (IS_NPC (ch))
    return;

  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_DISGUISE))
    {
      send_to_char ("You have no real talent for this.\r\n", ch);
      return;
    }

  one_argument (argument, arg);

  if (!*arg)
    {
      send_to_char ("Whose features would you like to memorize?\r\n", ch);
      return;
    }

  generic_find (arg, FIND_CHAR_ROOM, ch, &tch, &obj);

  if (tch == NULL)
    {
      send_to_char ("Noone here by that name.\r\n", ch);
      return;
    }

  if (!IS_NPC (tch) || (GET_LEVEL (tch) >= (GET_LEVEL (ch) * 1.10)) &&
      LR_FAIL (ch, LVL_IMPL))
    {
      send_to_char ("Their features are too detailed for you to capture.\r\n",
		    ch);
      return;
    }

  // Do a check here for particular types of mobs. Dragon's can't be mem'ed, etc

  CHAR_MEMORISED (ch) = GET_MOB_VNUM (tch);
  sprintf (buf, "You commit %s's features to memory.\r\n", GET_NAME (tch));
  send_to_char (buf, ch);
}

ACMD (do_disguise)
{
  struct char_data *tch;

  if (IS_NPC (ch))
    return;

  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_DISGUISE))
    {
      send_to_char ("You have no idea how!\r\n", ch);
      return;
    }

  if (!CHAR_MEMORISED (ch))
    {
      send_to_char ("You have noone memorised!\r\n", ch);
      return;
    }

  half_chop (argument, arg, buf1);

  tch = read_mobile (CHAR_MEMORISED (ch), VIRTUAL);
  char_to_room (tch, 0);

  if (strcmp (arg, "on") == 0)
    {
      CHAR_DISGUISED (ch) = CHAR_MEMORISED (ch);
      sprintf (buf, "You now appear as %s.\r\n", GET_NAME (tch));
      send_to_char (buf, ch);
    }
  else if (strcmp (arg, "off") == 0)
    {
      if (CHAR_DISGUISED (ch) == 0)
	{
	  send_to_char ("You're not disguised at the moment.\r\n", ch);
	}
      else
	{
	  CHAR_DISGUISED (ch) = 0;
	  send_to_char ("You now appear as yourself.\r\n", ch);
	}
    }
  else
    {
      sprintf (buf, "Disguise &gon&n or &goff&n! (Currently %s)\r\n",
	       (CHAR_DISGUISED (ch) == 0 ? "off" : "on"));
      send_to_char (buf, ch);
    }
  if (tch)
    extract_char (tch);
}

void
show_ability_messages_to_char (struct char_data *ch)
{
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_INVIS))
    send_to_char ("&WYour body is invisible.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_SNEAK))
    send_to_char ("&WMoving silently is second nature.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_MULTIWEAPON))
    send_to_char ("&WYou are able to wield weapons in either hand.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_FOREST_SPELLS))
    send_to_char ("&WYour spells are more potent in forests.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_FOREST_HELP))
    send_to_char ("&WForest creatures are your allies.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_HEALER))
    send_to_char ("&WYour healing spells are very potent.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_PRIEST))
    send_to_char ("&WOthers must pay for the benefits of your spells.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_BACKSTAB))
    send_to_char
      ("&WYour speed and skill allow you to backstab during battle.&n\r\n",
       ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_BATTLEMAGE))
    send_to_char ("&WYour offensive spells are incredibly powerful.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_MANA_THIEF))
    send_to_char ("&WYou are able to drain opponents of mana.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_HOLY))
    send_to_char
      ("&WYour holiness can destroy undead with a single strike.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_DISGUISE))
    send_to_char ("&WYou have the ability to change your appearance.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_EMPATH))
    send_to_char
      ("&WYou are sensitive to the pain and suffering of others.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_ESCAPE))
    send_to_char
      ("&WYou have the uncanny ability to find the outdoors.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_INFRA))
    send_to_char ("&WYou can always see in the dark.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_DWARF))
    send_to_char ("&WWhile indoors your battle skills are enhanced.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_GROUP_SNEAK))
    send_to_char ("&WYour ability to move quietly is second nature.\r\n"
		  "... you are also able to help your allies do the same.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_THIEF))
    send_to_char ("&WYour thieving abilities are legendary.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_GORE))
    send_to_char ("&WYou are able to gore your opponents in battle.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_MINOTAUR))
    send_to_char
      ("&WYour ferocity in battle helps you damage your opponents.&n\r\n",
       ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_CHARMER))
    send_to_char ("&WYou are very persuasive with demi-humans.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_FLY))
    send_to_char ("&WYou have mastered the art of missing the ground.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_SUPERMAN))
    send_to_char ("&WYou are naturally strong, resilient and tough.&n\r\n",
		  ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_ELF))
    send_to_char
      ("&WYou are able to fight better than usual in forests.&n\r\n", ch);
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_TRACKER))
    send_to_char ("&WYou are able to track better than usual.\r\n", ch);

}

void
apply_specials (struct char_data *ch, bool initial)
{

  struct affected_type af, *afptr;

  // Remove all ability affects from the character
  for (afptr = ch->affected; afptr; afptr = afptr->next)
    if (afptr->duration == CLASS_ABILITY)
      ability_from_char (ch, afptr->type);

  // Reapply class abilities that require affs
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_INVIS))
    {
      af.type = SPELL_INVISIBLE;
      af.duration = CLASS_ABILITY;
      af.modifier = -40;
      af.location = APPLY_AC;
      af.bitvector = AFF_INVISIBLE;
      affect_to_char (ch, &af);
//        if( initial ) 
//          send_to_char("&WYou merge your skills and vanish.&n\r\n", ch);      
    }
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_SNEAK) ||
      IS_SET (GET_SPECIALS (ch), SPECIAL_GROUP_SNEAK))
    {
      af.duration = CLASS_ABILITY;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_SNEAK;
      af.type = SKILL_SNEAK;
      affect_to_char (ch, &af);
//        if( initial )
//          send_to_char("&WYour ability to move quietly becomes second nature.&n\r\n",ch);
    }
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_FLY))
    {
      af.duration = CLASS_ABILITY;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_FLY;
      af.type = SPELL_FLY;
      affect_to_char (ch, &af);
//        if( initial )
//          send_to_char("&WYou can now miss the ground when falling.&n\r\n",ch);
    }
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_INFRA))
    {
      af.duration = CLASS_ABILITY;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_INFRAVISION;
      af.type = SPELL_INFRAVISION;
      affect_to_char (ch, &af);
//        if( initial )
//          send_to_char("&WYour eyes take on a permanant red glow.&n\r\n",ch);
    }

  if (IS_SET (GET_SPECIALS (ch), SPECIAL_EMPATH))
    {
      af.duration = CLASS_ABILITY;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_SENSE_WOUNDS;
      af.type = SPELL_SENSE_WOUNDS;
      affect_to_char (ch, &af);
    }

  // Set their str and con to 21 if they're supermen, and not a god
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_SUPERMAN)
      && GET_LEVEL (ch) < LVL_GOD)
    {
      if (GET_REAL_STR (ch) < 21)
	GET_REAL_STR (ch) = 21;
      if (GET_REAL_CON (ch) < 21)
	GET_REAL_CON (ch) = 21;
    }
  if (initial)
    show_ability_messages_to_char (ch);

}


void
remove_race_specials (struct char_data *ch)
{
  switch (GET_RACE (ch))
    {
    case RACE_HUMAN:
      break;
    case RACE_OGRE:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_SUPERMAN);
      break;
    case RACE_DWARF:		// DM - added thief to dwarf, removed infra
      // TAli - Dwarves see inside. Pixies maybe should
      //        get thief.. We'll talk.. 
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_DWARF);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_ESCAPE);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_THIEF);
      break;
    case RACE_ELF:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_ELF);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_INFRA);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_TRACKER);
      break;
    case RACE_MINOTAUR:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_GORE);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_MINOTAUR);
      break;
    case RACE_PIXIE:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_FLY);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_FOREST_HELP);
      break;
    case RACE_DEVA:		// DM - added charm and disguise to Deva
      //REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_CHARMER);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_FLY);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_DISGUISE);
      break;
    default:
      basic_mud_log ("Unknown race type for %s (%ld).", GET_NAME (ch),
		     GET_RACE (ch));
      break;
    }
}

void
set_race_specials (struct char_data *ch)
{
  switch (GET_RACE (ch))
    {
    case RACE_HUMAN:
      break;
    case RACE_OGRE:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_SUPERMAN);
      break;
    case RACE_DWARF:		// DM - added thief to dwarf, removed infra
      SET_BIT (GET_SPECIALS (ch), SPECIAL_DWARF);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_ESCAPE);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_THIEF);
      break;
    case RACE_ELF:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_ELF);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_INFRA);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_TRACKER);
      break;
    case RACE_MINOTAUR:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_GORE);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_MINOTAUR);
      break;
    case RACE_PIXIE:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_FLY);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_FOREST_HELP);
      break;
    case RACE_DEVA:		// DM - added charm and disguise to Deva
      //SET_BIT(GET_SPECIALS(ch), SPECIAL_CHARMER);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_FLY);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_DISGUISE);
      /*case RACE_AVATAR: */
      break;
    default:
      basic_mud_log ("Unknown race type for %s (%ld).", GET_NAME (ch),
		     GET_RACE (ch));
      break;
    }
}

void
set_class_specials (struct char_data *ch)
{
  switch (GET_CLASS (ch))
    {
    case CLASS_WARRIOR:
    case CLASS_CLERIC:
    case CLASS_THIEF:
    case CLASS_MAGIC_USER:
      break;			// Nothing for base
    case CLASS_DRUID:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_FOREST_SPELLS);
      SET_BIT (GET_SPECIALS (ch), SPECIAL_FOREST_HELP);
      break;
    case CLASS_PRIEST:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_HEALER);	// Better healer
      SET_BIT (GET_SPECIALS (ch), SPECIAL_PRIEST);	// Charge for spells
      break;
    case CLASS_NIGHTBLADE:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_BACKSTAB);	// Backstab during battles
      SET_BIT (GET_SPECIALS (ch), SPECIAL_SNEAK);	// Perm sneak
      break;
    case CLASS_BATTLEMAGE:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_MULTIWEAPON);	// Multi Weapon
      SET_BIT (GET_SPECIALS (ch), SPECIAL_BATTLEMAGE);	// Powerful offensive spells
      break;
    case CLASS_SPELLSWORD:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_MANA_THIEF);	// Steals mana with hits
      SET_BIT (GET_SPECIALS (ch), SPECIAL_INVIS);	// Perm invis
      SET_BIT (GET_SPECIALS (ch), SPECIAL_SNEAK);	// Perm sneak
      break;
    case CLASS_PALADIN:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_HEALER);	// Better healer
      // SET_BIT(GET_SPECIALS(ch), SPECIAL_HOLY);       // Holy powers (destroy undead)
      break;
    case CLASS_MASTER:
      SET_BIT (GET_SPECIALS (ch), SPECIAL_SNEAK);	// Perm sneak
      SET_BIT (GET_SPECIALS (ch), SPECIAL_MULTIWEAPON);	// Multi Weapon
      SET_BIT (GET_SPECIALS (ch), SPECIAL_HEALER);	// Better Healer
      break;
    default:
      sprintf (buf, "SYSERR: %s has unknown class type '%d'.", GET_NAME (ch),
	       GET_CLASS (ch));
      mudlog (buf, BRF, LVL_GOD, TRUE);
      break;
    }
}

void
remove_class_specials (struct char_data *ch)
{
  switch (GET_CLASS (ch))
    {
    case CLASS_WARRIOR:
    case CLASS_CLERIC:
    case CLASS_THIEF:
    case CLASS_MAGIC_USER:
      break;
    case CLASS_DRUID:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_FOREST_SPELLS);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_FOREST_HELP);
      break;
    case CLASS_PRIEST:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_HEALER);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_PRIEST);
      break;
    case CLASS_NIGHTBLADE:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_BACKSTAB);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_SNEAK);
      break;
    case CLASS_BATTLEMAGE:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_MULTIWEAPON);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_BATTLEMAGE);
      break;
    case CLASS_SPELLSWORD:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_MANA_THIEF);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_INVIS);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_SNEAK);
      break;
    case CLASS_PALADIN:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_HEALER);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_HOLY);
      break;
    case CLASS_MASTER:
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_SNEAK);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_MULTIWEAPON);
      REMOVE_BIT (GET_SPECIALS (ch), SPECIAL_HEALER);
      break;
    default:
      sprintf (buf, "SYSERR: %s has unknown class type '%d'.", GET_NAME (ch),
	       GET_CLASS (ch));
      mudlog (buf, BRF, LVL_GOD, TRUE);
      break;
    }
}

void
remort (struct char_data *ch, int rclass, long lAbility)
{

  int pclass = GET_CLASS (ch);
  const char *remort_msg[NUM_CLASSES] = {
    "\n",			// Mage
    "\n",			// Warrior
    "\n",			// Cleric
    "\n",			// Thief
    "You combine your abilities into the most magical mix available.\r\nAs a &gDruid&n you hold all arcane powers at your disposal.\r\n",
    "A &yPriest&n combines your abilities into a profitable mix which relies on\r\ncunning and personal strength to survive and prosper.\r\n",
    "You combine your abilities into a deadly mix of cunning and strength,\r\nbecoming a &RNightblade&n.\r\n",
    "Focusing your abilities into offensive skills, as a &gBattlemage&n\r\nyou are widely reknown for your fighting prowess and unstoppable\r\nstrength.\r\n",
    "Combining your abilities for profit, you begin your new life as a\r\n&rSpellSword&n.\r\n",
    "Combining your abilities you become a &gPaladin&n, a holy warrior\r\nof good and pure heart.\r\n",
    "You take the final step into personal perfection, and rejoin the\r\nmortal realm as a &YMaster&n of all trades. Your abilities and potential\r\nare staggering, and should you achieve Immortality,\r\nyou shall stand tall with the most powerful in the realm.\r\n"
  };


  /* Perform class adaption checking, ie, thieves cannot become druids */
  switch (pclass)
    {
    case CLASS_MAGIC_USER:
      if (rclass != CLASS_SPELLSWORD && rclass != CLASS_BATTLEMAGE &&
	  rclass != CLASS_DRUID)
	{
	  send_to_char ("Mages cannot specialise there.\r\n", ch);
	  return;
	}
      break;
    case CLASS_WARRIOR:
      if (rclass != CLASS_PALADIN && rclass != CLASS_BATTLEMAGE &&
	  rclass != CLASS_NIGHTBLADE)
	{
	  send_to_char ("Warriors cannot specialise there.\r\n", ch);
	  return;
	}
      break;
    case CLASS_CLERIC:
      if (rclass != CLASS_PRIEST && rclass != CLASS_PALADIN &&
	  rclass != CLASS_DRUID)
	{
	  send_to_char ("Clerics cannot specialise there.\r\n", ch);
	  return;
	}
      break;
    case CLASS_THIEF:
      if (rclass != CLASS_NIGHTBLADE && rclass != CLASS_SPELLSWORD &&
	  rclass != CLASS_PRIEST)
	{
	  send_to_char ("Thieves cannot specialise there.\r\n", ch);
	  return;
	}
      break;
    case CLASS_NIGHTBLADE:
    case CLASS_SPELLSWORD:
    case CLASS_DRUID:
    case CLASS_PRIEST:
    case CLASS_PALADIN:
    case CLASS_BATTLEMAGE:
      break;

    default:
      sprintf (buf, "Remort Error: %s attempting to remort into class #%d.",
	       GET_NAME (ch), rclass);
      mudlog (buf, NRM, LVL_GOD, TRUE);
      return;
    }

  /* Do checking here for special locations, items, alignment etc */


  // DM - get rid of any affects ...
  if (ch->affected)
    for (struct affected_type * affect = ch->affected; affect;
	 affect = affect->next)
      if (ch->affected->duration != -1)
	affect_remove (ch, ch->affected);

  /* Apply changing conditions here (Maxmana, etc) */
  // DM - shouldn't this be class based???
  // capped min and max values? - Artus - Min Yes.
  GET_MAX_MANA (ch) = MAX ((int) (GET_MAX_MANA (ch) * 0.1), 100);
  GET_MAX_HIT (ch) = MAX ((int) (GET_MAX_HIT (ch) * 0.1), 50);
  GET_MAX_MOVE (ch) = MAX ((int) (GET_MAX_MOVE (ch) * 0.1), 80);
  GET_MANA (ch) = MIN (GET_MANA (ch), GET_MAX_MANA (ch));
  GET_HIT (ch) = MIN (GET_HIT (ch), GET_MAX_HIT (ch));
  GET_MOVE (ch) = MIN (GET_MOVE (ch), GET_MAX_MOVE (ch));

  // Artus> Reset Wimpy.
  if (GET_WIMP_LEV (ch) > (GET_MAX_HIT (ch) / 2))
    GET_WIMP_LEV (ch) = (int) (GET_MAX_HIT (ch) / 2);

  /* Set player's level here */
  GET_CLASS (ch) = rclass;

  if (GET_REM_ONE (ch) >= 50)	// Set old level values.
    GET_REM_TWO (ch) = GET_LEVEL (ch);
  else
    GET_REM_ONE (ch) = GET_LEVEL (ch);

  GET_LEVEL (ch) = 1;
  GET_EXP (ch) = 0;
  calc_modifier (ch);

  for (int i = 0; i < NUM_WORLDS; i++)
    ENTRY_ROOM (ch, i) = -1;

  // Any new ability?
  if (lAbility > 0)
    SET_BIT (GET_SPECIALS (ch), lAbility);

  set_class_specials (ch);
  apply_specials (ch, TRUE);

#define REMORT_START_VROOM 1115

  // Move them back to haven
  char_from_room (ch);
  char_to_room (ch, real_room (REMORT_START_VROOM));

  sprintf (buf, "%s rejoins the mortal realm as a %s.", GET_NAME (ch),
	   pc_class_types[rclass]);
  mudlog (buf, BRF, LVL_ETRNL1, TRUE);

  GET_POS (ch) = POS_STUNNED;

  send_to_char (remort_msg[rclass], ch);
}


void
ShowRemortsToChar (struct char_data *ch)
{
  if (GET_CLASS (ch) > CLASS_WARRIOR)
    {
      send_to_char
	("You can take the final step, and attempt to become &cMaster&n of all the arts.\r\n",
	 ch);
      return;
    }

  send_to_char ("Your possible paths are - ", ch);
  switch (GET_CLASS (ch))
    {
    case CLASS_WARRIOR:
      send_to_char ("&cBattlemage&n, &cNightblade&n, &cPaladin&n.\r\n", ch);
      break;
    case CLASS_CLERIC:
      send_to_char ("&cDruid&n, &cPaladin&n, &cPriest&n.\r\n", ch);
      break;
    case CLASS_THIEF:
      send_to_char ("&cNightblade&n, &cPriest&n, &cSpellsword&n.\r\n", ch);
      break;
    case CLASS_MAGIC_USER:
      send_to_char ("&cDruid&n, &cBattlemage&n, &cSpellsword&n.\r\n", ch);
      break;
    default:
      send_to_char
	("You seem to have a strange base class .... reporting error.\r\n",
	 ch);
      sprintf (buf,
	       "Unknown base class for %s (class #%d) in ShowRemortsToChar",
	       GET_NAME (ch), GET_CLASS (ch));
      mudlog (buf, NRM, LVL_GOD, TRUE);
      break;
    }
  return;

}

void
list_ability_upgrades_to_char (struct char_data *ch, char newclass)
{
  send_to_char
    ("\r\nThe following abilities are available to you...\r\nYou may only select &c1&n of these abilities, so choose carefully.\r\n\r\n",
     ch);

  send_to_char ("  Ability    XP Cost  Description\r\n", ch);
  // Combat Backstab
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_BACKSTAB) &&
      (newclass != CLASS_NIGHTBLADE))
    send_to_char
      ("  &gBackstab&n - &R[&r+12%&R]&n Can backstab even when fighting\r\n",
       ch);
  // Charmer
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_CHARMER))
    send_to_char
      ("  &gCharmer&n  - &R[&r +3%&R]&n Chance of automatically charming mobs\r\n",
       ch);
  // Disguise...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_DISGUISE))
    send_to_char
      ("  &gDisguise&n - &R[&r +1%&R]&n Can appear as any other creature\r\n",
       ch);
  // Dual Wield (MultiWeapon)
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_MULTIWEAPON) &&
      (newclass != CLASS_BATTLEMAGE))
    send_to_char
      ("  &gDualWield&n- &R[&r +9%&R]&n Dual Wielding Capability\r\n", ch);
  /* Indoor Battle Bonus (Dwarf); Not included, Race Specific.
     if (!IS_SET(GET_SPECIALS(ch), SPECIAL_DWARF))
     send_to_char(       "  &gDwarf&n    - &R[&r +2%&R]&n Deal more damage when figting indoors\r\n", ch); */
  /* Forest Battle Bonus (Elf): Not included, Race Specific.
     if (!IS_SET(GET_SPECIALS(ch), SPECIAL_ELF))
     send_to_char(       "  &gElf&n      - &R[&r +5%&R]&n Track through !TRACK; +10\% damage in forests\r\n", ch); */
  // Empath...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_EMPATH))
    send_to_char
      ("  &gEmpath&n   - &R[&r +1%&R]&n Feel the pain of others\r\n", ch);
  // Escape...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_ESCAPE))
    send_to_char
      ("  &gEscape&n   - &R[&r +1%&R]&n Escape from indoors to outside\r\n",
       ch);
  // Perm Fly...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_FLY))
    send_to_char ("  &gFly&n      - &R[&r +2%&R]&n Permanant fly\r\n", ch);
  // Forest Aid.
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_FOREST_HELP) &&
      (newclass != CLASS_DRUID))
    send_to_char
      ("  &gForestAid&n- &R[&r +2%&R]&n Allies aid your attack in forest\r\n",
       ch);
  // Forest Spells
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_FOREST_SPELLS) &&
      (newclass != CLASS_DRUID))
    send_to_char
      ("  &gForestSpl&n- &R[&r +2%&R]&n Greater spell power in forest\r\n",
       ch);
  /* Gore (Not Included, Minotaur Only.)
     if (!IS_SET(GET_SPECIALS(ch), SPECIAL_GORE))
     send_to_char(       "  &gGore&n     - &R[&r +5%&R]&n Deal gore damage in combat\r\n", ch); */
  // Greed (Priest)
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_PRIEST) &&
      (newclass != CLASS_PRIEST))
    send_to_char
      ("  &gGreed&n    - &R[&r +2%&R]&n Charge players for spell casting\r\n",
       ch);
  // Group Sneak
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_GROUP_SNEAK))
    send_to_char ("  &gGrpSneak&n - &R[&r +3%&R]&n Group sneaking\r\n", ch);
  // Healer...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_HEALER) &&
      (newclass != CLASS_PRIEST) && (newclass != CLASS_PALADIN) &&
      (newclass != CLASS_MASTER))
    send_to_char
      ("  &gHealer&n   - &R[&r+12%&R]&n More potent healing spells\r\n", ch);
  /* Holy (Undead Bane)
     if (!IS_SET(GET_SPECIALS(ch), SPECIAL_HOLY) &&
     (newclass != CLASS_PALADIN))
     send_to_char(       "  &gHoly     &n- &R[&r +3%&R]&n Chance of killing undead in one hit\r\n", ch); */
  // Perm Infra...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_INFRA))
    send_to_char ("  &gInfra&n    - &R[&r +2%&R]&n Permanant infravision\r\n",
		  ch);
  // Perm Invis...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_INVIS) &&
      (newclass != CLASS_SPELLSWORD))
    send_to_char
      ("  &gInvis&n    - &R[&r +2%&R]&n Permanant invisibility\r\n", ch);
  // Mana Thief (Mana Theft)
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_MANA_THIEF) &&
      (newclass != CLASS_SPELLSWORD))
    send_to_char
      ("  &gManaThief&n- &R[&r +5%&R]&n Chance of draining 5% of opponent's mana\r\n",
       ch);
  /* Minotaur: Not Included (Race Specific)
     if (!IS_SET(GET_SPECIALS(ch), SPECIAL_MINOTAUR))
     send_to_char(       "  &gMinotaur&n - &R[&r +8%&R]&n 5% bonus to damroll\r\n", ch); */
  // Perm Sneak...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_SNEAK) &&
      (newclass != CLASS_NIGHTBLADE) && (newclass != CLASS_SPELLSWORD) &&
      (newclass != CLASS_MASTER))
    send_to_char ("  &gSneak&n    - &R[&r +2%&R]&n Permanant sneak\r\n", ch);
  // Superman... (Only for Masters)
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_SUPERMAN)
      && (newclass == CLASS_MASTER))
    send_to_char
      ("  &gSuperman&n - &R[&r +4%&R]&n Superman (21STR, +10% AC, +2\% damage)\r\n",
       ch);
  // Thievery...
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_THIEF))
    send_to_char
      ("  &gThief&n    - &R[&r +5%&R]&n Enhanced thief abilities\r\n", ch);
  if (!IS_SET (GET_SPECIALS (ch), SPECIAL_TRACKER))
    send_to_char
      ("  &gTracker&n  - &R[&r +5%&R]&n Track through !TRACK areas\r\n", ch);
}

bool
charge_for_remort_ability (struct char_data *ch, long lAbility)
{
  /* TODO: Decide what each ability will cost, and apply EXP modifier, 
     as well as stripping ch of anything we want/need */

  return TRUE;
}

long
process_ability_purchase (struct char_data *ch, char newclass,
			  char *remortAbility)
{
  void to_upper (char *s);
  skip_spaces (&remortAbility);

  if (is_abbrev (remortAbility, "backstab"))	// Backstab...
    {
      if (charge_for_remort_ability (ch, SPECIAL_BACKSTAB))
	return SPECIAL_BACKSTAB;
    }
  else if (is_abbrev (remortAbility, "charmer"))
    {				// Charmer...
      if (charge_for_remort_ability (ch, SPECIAL_CHARMER))
	return SPECIAL_CHARMER;
    }
  else if (is_abbrev (remortAbility, "disguise"))
    {				// Disguise...
      if (!is_abbrev (remortAbility, "dualwield") &&
	  !is_abbrev (remortAbility, "dwarf") &&
	  charge_for_remort_ability (ch, SPECIAL_DISGUISE))
	return SPECIAL_DISGUISE;
    }
  else if (is_abbrev (remortAbility, "dualwield"))
    {				// Dual Wield...
      if (!is_abbrev (remortAbility, "dwarf") &&
	  charge_for_remort_ability (ch, SPECIAL_MULTIWEAPON))
	return SPECIAL_MULTIWEAPON;
    }
  else if (is_abbrev (remortAbility, "dwarf"))
    {				// Dwarf...
      if (charge_for_remort_ability (ch, SPECIAL_DWARF))
	return SPECIAL_DWARF;
    }
  else if (is_abbrev (remortAbility, "elf"))
    {				// Elf...
      if (!is_abbrev (remortAbility, "escape") &&
	  charge_for_remort_ability (ch, SPECIAL_ELF))
	return SPECIAL_ELF;
    }
  else if (is_abbrev (remortAbility, "empath"))
    {				// Empath...
      if (!is_abbrev (remortAbility, "elf") &&
	  charge_for_remort_ability (ch, SPECIAL_EMPATH))
	return SPECIAL_EMPATH;
    }
  else if (is_abbrev (remortAbility, "escape"))
    {				// Escape...
      if (charge_for_remort_ability (ch, SPECIAL_ESCAPE))
	return SPECIAL_ESCAPE;
    }
  else if (is_abbrev (remortAbility, "fly"))
    {				// Fly...
      if (!is_abbrev (remortAbility, "forestaid") &&
	  !is_abbrev (remortAbility, "forestspl") &&
	  charge_for_remort_ability (ch, SPECIAL_FLY))
	return SPECIAL_FLY;
    }
  else if (is_abbrev (remortAbility, "forestaid"))
    {				// Forest Aid...
      if (!is_abbrev (remortAbility, "forestspl") &&
	  charge_for_remort_ability (ch, SPECIAL_FOREST_HELP))
	return SPECIAL_FOREST_HELP;
    }
  else if (is_abbrev (remortAbility, "forestspl"))
    {				// Forest Help...
      if (charge_for_remort_ability (ch, SPECIAL_FOREST_SPELLS))
	return SPECIAL_FOREST_SPELLS;
    }
  else if (is_abbrev (remortAbility, "gore"))
    {				// Gore...
      if (!is_abbrev (remortAbility, "greed") &&
	  !is_abbrev (remortAbility, "grpsneak") &&
	  charge_for_remort_ability (ch, SPECIAL_GORE))
	return SPECIAL_GORE;
    }
  else if (is_abbrev (remortAbility, "greed"))
    {				// Greed...
      if (!is_abbrev (remortAbility, "grpsneak") &&
	  charge_for_remort_ability (ch, SPECIAL_PRIEST))
	return SPECIAL_PRIEST;
    }
  else if (is_abbrev (remortAbility, "grpsneak"))
    {				// Group Sneak...
      if (charge_for_remort_ability (ch, SPECIAL_GROUP_SNEAK))
	return SPECIAL_GROUP_SNEAK;
    }
  else if (is_abbrev (remortAbility, "healer"))
    {				// Healer...
      if (!is_abbrev (remortAbility, "holy") &&
	  charge_for_remort_ability (ch, SPECIAL_HEALER))
	return SPECIAL_HEALER;
/*  } else if (is_abbrev(remortAbility, "holy")) {	// Holy... (Undead Bane)
    if (charge_for_remort_ability(ch, SPECIAL_HOLY))
      return SPECIAL_HOLY; */
    }
  else if (is_abbrev (remortAbility, "infravision"))
    {				// Infravision...
      if (!is_abbrev (remortAbility, "invisibility") &&
	  charge_for_remort_ability (ch, SPECIAL_INFRA))
	return SPECIAL_INFRA;
    }
  else if (is_abbrev (remortAbility, "invisibility"))
    {				// Invisibility...
      if (charge_for_remort_ability (ch, SPECIAL_INVIS))
	return SPECIAL_INVIS;
    }
  else if (is_abbrev (remortAbility, "manathief"))
    {				// Mana Thief...
      if (!is_abbrev (remortAbility, "minotaur") &&
	  charge_for_remort_ability (ch, SPECIAL_MANA_THIEF))
	return SPECIAL_MANA_THIEF;
    }
  else if (is_abbrev (remortAbility, "minotaur"))
    {				// Minotaur...
      if (charge_for_remort_ability (ch, SPECIAL_MINOTAUR))
	return SPECIAL_MINOTAUR;
    }
  else if (is_abbrev (remortAbility, "sneak"))
    {				// Sneak...
      if (!is_abbrev (remortAbility, "superman") &&
	  charge_for_remort_ability (ch, SPECIAL_SNEAK))
	return SPECIAL_SNEAK;
    }
  else if (is_abbrev (remortAbility, "superman"))
    {				// Superman...
      if (charge_for_remort_ability (ch, SPECIAL_SUPERMAN))
	return SPECIAL_SUPERMAN;
    }
  else if (is_abbrev (remortAbility, "thief"))
    {				// Thief...
      if (!is_abbrev (remortAbility, "tracker") &&
	  charge_for_remort_ability (ch, SPECIAL_THIEF))
	return SPECIAL_THIEF;
    }
  else if (is_abbrev (remortAbility, "tracker"))
    {				// Tracker...
      if (charge_for_remort_ability (ch, SPECIAL_TRACKER))
	return SPECIAL_TRACKER;
    }
  else if (!strcasecmp (remortAbility, "none"))
    {				// None...
      return 0;
    }

  // No argument matched.
  return -1;
}

/* Remort command for players */
/* Works:
 *	Base class (warrior, cleric, magician, thief) can do it FROM level 50
 *      Specialist class (nightblade, etc) can do it AT level 75
 *      
 */
ACMD (do_remort)
{
  char remortClass[MAX_INPUT_LENGTH], remortAbility[MAX_INPUT_LENGTH];
  char newclass = 0;
  long lAbility = -1;

  if (GET_CLASS (ch) == CLASS_MASTER)
    {
      send_to_char ("You have mastered all classes!\r\n", ch);
      return;
    }

  // Give some explanation/help, and enforce level restrictions
  if (GET_CLASS (ch) > CLASS_WARRIOR)
    {
      if (LR_FAIL (ch, 75))
	{
	  send_to_char
	    ("You can remort at level &G75&n, and travel the path of the Master.\r\n",
	     ch);
	  return;
	}
    }
/*  else if (GET_CLASS(ch) > CLASS_WARRIOR && GET_LEVEL(ch) == 75)
    send_to_char("You may now remort, and become a Master of all the arts!\r\n", ch);
  else if (GET_CLASS(ch) > CLASS_WARRIOR && GET_LEVEL(ch) > 75)
  {
  send_to_char("You have become very specialised in your field."
		" Your desire and focus to be the best at what you do "
		"drives you more than the desire to master all the arts, " 
  		"and you can not remort.\r\n", ch);
    return;
  } */
  else				// They're base class
    {
      if (LR_FAIL (ch, 50))
	{
	  send_to_char
	    ("You can remort at level &G50&n, and specialise in a field of your choice.\r\n",
	     ch);
	  ShowRemortsToChar (ch);
	  return;
	}
    }				// End of explanation, help and checking for level validity

  // Get the class they think they can become, and the (if any) ability
  // they wish to purchase as part of the remort.
  half_chop (argument, remortClass, remortAbility);

  // Have they specified a class?
  if (!*remortClass)
    {
      // Base classes can remort from 50 onward
      send_to_char
	("You may now remort, into a field of your choice ... \r\n", ch);
      ShowRemortsToChar (ch);
//    send_to_char("\r\nYou must specify what class you would like to evolve into.\r\n", ch);
      send_to_char
	("\r\n   Usage: remort <class> <list abilities | ability | none>\r\n",
	 ch);
      return;
    }

  // Okay, get on with the remort.....................
  if (is_abbrev (remortClass, "nightblade") &&
      ((GET_CLASS (ch) == CLASS_WARRIOR) || (GET_CLASS (ch) == CLASS_THIEF)))
    newclass = CLASS_NIGHTBLADE;
  else if (is_abbrev (remortClass, "battlemage") &&
	   ((GET_CLASS (ch) == CLASS_WARRIOR) ||
	    (GET_CLASS (ch) == CLASS_MAGIC_USER)))
    newclass = CLASS_BATTLEMAGE;
  else if (is_abbrev (remortClass, "spellsword") &&
	   ((GET_CLASS (ch) == CLASS_MAGIC_USER) ||
	    (GET_CLASS (ch) == CLASS_THIEF)))
    newclass = CLASS_SPELLSWORD;
  else if (is_abbrev (remortClass, "paladin") &&
	   ((GET_CLASS (ch) == CLASS_WARRIOR) ||
	    (GET_CLASS (ch) == CLASS_CLERIC)))
    newclass = CLASS_PALADIN;
  else if (is_abbrev (remortClass, "druid") &&
	   ((GET_CLASS (ch) == CLASS_CLERIC) ||
	    (GET_CLASS (ch) == CLASS_MAGIC_USER)))
    newclass = CLASS_DRUID;
  else if (is_abbrev (remortClass, "priest") &&
	   ((GET_CLASS (ch) == CLASS_CLERIC) ||
	    (GET_CLASS (ch) == CLASS_THIEF)))
    newclass = CLASS_PRIEST;
  else if (is_abbrev (remortClass, "master") &&
	   (GET_CLASS (ch) > CLASS_WARRIOR)
	   && (GET_CLASS (ch) < CLASS_MASTER))
    newclass = CLASS_MASTER;
  else
    {
      send_to_char
	("Invalid or unavailable class specified, type remort for a list.\r\n",
	 ch);
      return;
    }

  // Process their ability
  if (*remortAbility)
    {
      if (is_abbrev (remortAbility, "list abilities")
	  || is_abbrev (remortAbility, "help"))
	{
	  list_ability_upgrades_to_char (ch, newclass);
	  return;
	}

      // They wish to purchase an ability
      lAbility = process_ability_purchase (ch, newclass, remortAbility);
    }
  if (lAbility < 0)
    {
      send_to_char
	("\r\nYou MUST specify 'none' if you do not want an ability upgrade.\r\n",
	 ch);
      send_to_char
	("   Usage: remort <class> <list abilities | ability | none>\r\n",
	 ch);
      return;
    }

  // DM - make em take their eq off first ...
  for (int i = 0; i < NUM_WEARS; i++)
    {
      if (GET_EQ (ch, i) != NULL)
	{
	  send_to_char
	    ("Hmm, best take your equipment off first. Wouldn't want"
	     " anything to happen to it!\r\n", ch);
	  return;
	}
    }

  remort (ch, newclass, lAbility);

#if 0				//Artus> The old way...
  // Okay, get on with the remort.....................
  if (is_abbrev (remortClass, "nightblade"))
    {
      remort (ch, CLASS_NIGHTBLADE, lAbility);
      return;
    }

  if (is_abbrev (remortClass, "battlemage"))
    {
      remort (ch, CLASS_BATTLEMAGE, lAbility);
      return;
    }

  if (is_abbrev (remortClass, "spellsword"))
    {
      remort (ch, CLASS_SPELLSWORD, lAbility);
      return;
    }

  if (is_abbrev (remortClass, "paladin"))
    {
      remort (ch, CLASS_PALADIN, lAbility);
      return;
    }

  if (is_abbrev (remortClass, "druid"))
    {
      remort (ch, CLASS_DRUID, lAbility);
      return;
    }

  if (is_abbrev (remortClass, "priest"))
    {
      remort (ch, CLASS_PRIEST, lAbility);
      return;
    }

  if (is_abbrev (remortClass, "master"))
    {
      remort (ch, CLASS_MASTER, lAbility);
      return;
    }

  send_to_char ("No such class exists!\r\n", ch);
#endif
}


ACMD (do_quit)
{
  struct descriptor_data *d, *next_d;
  int i, loadroom, cost = 0;
  int find_house (room_vnum vnum);

  if (IS_NPC (ch) || !ch->desc)
    return;

  if (subcmd < SCMD_QUIT && LR_FAIL (ch, LVL_IS_GOD))
    {
      send_to_char ("You have to type '&cquit&n'--no less, to quit!\r\n", ch);
      return;
    }
  if ((GET_POS (ch) == POS_FIGHTING) && LR_FAIL (ch, LVL_IS_GOD))
    {
      cost = MAX (cost, (int) (GET_EXP (ch) / 20));
      if (subcmd < SCMD_QUITR)
	{
	  send_to_char (CHARFIGHTING, ch);
	  sprintf (buf,
		   "You may use the &4quitreally&n command at a cost of &c%d&n experience.\r\n",
		   cost);
	  send_to_char (buf, ch);
	  return;
	}
    }
  if (GET_POS (ch) < POS_STUNNED)
    {
      send_to_char ("You die before your time...\r\n", ch);
      die (ch, NULL);
      return;
    }
  loadroom = ch->in_room;
  switch (zone_table[world[loadroom].zone].number)
    {
    case 11:			/* Haven */
    case 132:			/* City of HOPE */
    case 220:			/* Lunar City IV */
      break;
    default:
      if (find_house (world[loadroom].number) != NOWHERE)
	{
	  if (!House_can_enter (ch, world[loadroom].number))
	    {
	      send_to_char ("You are no longer welcome in this house!\r\n",
			    ch);
	      return;
	    }
	  break;
	}
      if (!LR_FAIL (ch, LVL_IS_GOD))
	break;
      cost = MAX (cost, (int) (GET_EXP (ch) / 20));
      if (subcmd < SCMD_QUITR)
	{
	  send_to_char
	    ("You may only quit from within Haven, Hope, Lunar City, or your house!\r\n",
	     ch);
	  sprintf (buf,
		   "You may use the &4quitreally&n command at a cost of &c%d&n experience.\r\n",
		   cost);
	  send_to_char (buf, ch);
	  return;
	}
      break;
    }

  if (FIGHTING (ch))
    stop_fighting (ch);

  if (cost > 0)
    {
      sprintf (buf,
	       "You forcibly remove yourself from the world at a cost of &c%d&nexp.\r\n",
	       cost);
      GET_EXP (ch) = MAX (0, GET_EXP (ch) - cost);
      send_to_char (buf, ch);
    }

  if (!GET_INVIS_LEV (ch))
    act ("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
  sprintf (buf, "&7%s &ghas quit the game.", GET_NAME (ch));
  mudlog (buf, NRM, MAX (LVL_ANGEL, GET_INVIS_LEV (ch)), TRUE);
  info_channel (buf, ch);
  send_to_char ("Goodbye, friend.. Come back soon!\r\n", ch);

  /*
   * kill off all sockets connected to the same player as the one who is
   * trying to quit.  Helps to maintain sanity as well as prevent duping.
   */
  for (d = descriptor_list; d; d = next_d)
    {
      next_d = d->next;
      if (d == ch->desc)
	continue;
      if (d->character && (GET_IDNUM (d->character) == GET_IDNUM (ch)))
	STATE (d) = CON_DISCONNECT;
    }

  if (free_rent)
    Crash_rentsave (ch, 0);
  loadroom = ch->in_room;

  /**
   * DM - if room the char is in is the default world start room, set the 
   * players world entry room to NOWHERE indicating that the default entry
   * room is to be used.
   */
  for (i = 0; i < NUM_WORLDS; i++)
    {
      if (loadroom == real_room (world_start_room[i]))
	{
	  ENTRY_ROOM (ch, i) = NOWHERE;
	  break;
	}
    }
  START_WORLD (ch) = get_world (ch->in_room);

  /* If someone is quitting in their house, let them load back here */
  if (ROOM_FLAGGED (loadroom, ROOM_HOUSE))
    {
      ENTRY_ROOM (ch, 0) = GET_ROOM_VNUM (loadroom);
      START_WORLD (ch) = 0;
    }

  extract_char (ch);		/* Char is saved in extract char */
}



ACMD (do_save)
{
  if (IS_NPC (ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd)
    {
      /*
       * This prevents item duplication by two PC's using coordinated saves
       * (or one PC with a house) and system crashes. Note that houses are
       * still automatically saved without this enabled. This code assumes
       * that guest immortals aren't trustworthy. If you've disabled guest
       * immortal advances from mortality, you may want < instead of <=.
       */
      if (auto_save && LR_FAIL (ch, LVL_IS_GOD))
	{
	  send_to_char ("Saving aliases.\r\n", ch);
	  write_aliases (ch);
	  return;
	}
      sprintf (buf, "Saving %s and aliases.\r\n", GET_NAME (ch));
      send_to_char (buf, ch);

    /**
     * DM - if room the char is in is the default world start room, set the 
     * players world entry room to NOWHERE indicating that the default entry
     * room is to be used.
     *
     * Only set values if the player actually typed "save"...
     */
      for (int i = 0; i < NUM_WORLDS; i++)
	if (ch->in_room == real_room (world_start_room[i]))
	  {
	    ENTRY_ROOM (ch, i) = NOWHERE;
	    break;
	  }
      START_WORLD (ch) = get_world (ch->in_room);
    }

  write_aliases (ch);

  save_char (ch, NOWHERE);
  Crash_crashsave (ch);
  if (ROOM_FLAGGED (ch->in_room, ROOM_HOUSE_CRASH))
    House_crashsave (GET_ROOM_VNUM (IN_ROOM (ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD (do_not_here)
{
  send_to_char ("Sorry, but you cannot do that here!\r\n", ch);
}

/* functions used for alignment */
int
get_bit (struct obj_data *obj)
{
  int i = 0;

  if (IS_OBJ_STAT (obj, ITEM_ANTI_GOOD))
    i += 1;
  if (IS_OBJ_STAT (obj, ITEM_ANTI_NEUTRAL))
    i += 2;
  if (IS_OBJ_STAT (obj, ITEM_ANTI_EVIL))
    i += 4;
  return i;
}

/* print alignment messages */
void
print_it (int bit, struct obj_data *obj, struct char_data *ch)
{
  switch (bit)
    {
    case 0:			/* no alignment restrictions */
      sprintf (buf, "%s can be used by any Tom, Dick or Harry.\r\n",
	       obj->short_description);
      break;
    case 1:			/* neutral or evil item */
      sprintf (buf, "%s can be used by neutral or evil players.\r\n",
	       obj->short_description);
      break;
    case 2:			/* good or evil item */
      sprintf (buf, "%s can be used by good or evil players.\r\n",
	       obj->short_description);
      break;
    case 3:			/* evil item */
      sprintf (buf, "%s can only be used by evil players.\r\n",
	       obj->short_description);
      break;
    case 4:			/* neutral or good item */
      sprintf (buf, "%s can be used by good and neutral players.\r\n",
	       obj->short_description);
      break;
    case 5:			/* neutral */
      sprintf (buf, "%s can only be used by neutral players.\r\n",
	       obj->short_description);
      break;
    case 6:			/* good */
      sprintf (buf, "%s can only be used by good players.\r\n",
	       obj->short_description);
      break;
    default:
      sprintf (buf, "%s is obviously fucked.\r\n", obj->short_description);
      break;
    }
  CAP (buf);
  send_to_char (buf, ch);
}

/* compare by Misty */
ACMD (do_compare)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int count1, count2, i, align1, align2;
  float dam1, dam2;
  byte prob, percent;
  struct obj_data *obj1, *obj2;
  struct char_data *temp;


  two_arguments (argument, arg1, arg2);

  if (!has_stats_for_skill (ch, SKILL_COMPARE, TRUE))
    return;

  /* Check if 2 objects were specified */
  if (!*arg1 || !*arg2)
    {
      send_to_char ("Compare what with what?\r\n", ch);
      return;
    }

  /* Find object 1 */
  generic_find (arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &temp, &obj1);
  if (!obj1)
    {
      sprintf (buf, "You don't seem to have a %s.\r\n", arg1);
      send_to_char (buf, ch);
      return;
    }

  /* Find object 2 */
  generic_find (arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &temp, &obj2);
  if (!obj2)
    {
      sprintf (buf, "You don't seem to have a %s.\r\n", arg2);
      send_to_char (buf, ch);
      return;
    }

  /* Make sure objects are of type ARMOR or WEAPON */
  if ((GET_OBJ_TYPE (obj1) != ITEM_WEAPON &&
       GET_OBJ_TYPE (obj1) != ITEM_ARMOR) ||
      GET_OBJ_TYPE (obj1) != GET_OBJ_TYPE (obj2))
    {
      sprintf (buf, "You can't compare %s to %s.\r\n", arg1, arg2);
      send_to_char (buf, ch);
      return;
    }

  percent = number (1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL (ch, SKILL_COMPARE);

  if (percent > prob)
    {
      /* vader's lame bit */
      if (GET_OBJ_TYPE (obj1) == ITEM_WEAPON)
	{
	  switch (number (0, 4))
	    {
	    case 0:
	      sprintf (buf,
		       "Have you seen anyone wielding %s lately?? It's out of fashion!\r\n",
		       obj1->short_description);
	      break;
	    case 1:
	      sprintf (buf, "Get with it! No-one kills with %s anymore.\r\n",
		       obj2->short_description);
	      break;
	    case 2:
	      sprintf (buf,
		       "In the right lighting, %s could really suit you!\r\n",
		       obj1->short_description);
	      break;
	    case 3:
	      sprintf (buf,
		       "I'm sure you'll have many fun hours of slaying newbies with %s.\r\n",
		       obj2->short_description);
	      break;
	    case 4:
	      sprintf (buf,
		       "If you this %s looks good now, wait till its stained with the blood of your enemies!!\r\n",
		       obj1->short_description);
	      break;
	    case 5:
	      sprintf (buf,
		       "Go with the %s. It makes you look tuff. Grrr.\r\n",
		       obj2->short_description);
	      break;
	    }
	}
      else
	{
	  switch (number (0, 10))
	    {
	    case 0:
	      sprintf (buf, "Oh please darling, %s is soooo last season!\r\n",
		       obj1->short_description);
	      break;
	    case 1:
	      sprintf (buf,
		       "I wouldn't even wear %s to my step mother's funeral!\r\n",
		       obj2->short_description);
	      break;
	    case 2:
	      sprintf (buf,
		       "Loose a few pounds darling. Then you can consider wearing something like %s.\r\n",
		       obj1->short_description);
	      break;
	    case 3:
	      sprintf (buf,
		       "Yes! The floral pattern on %s looks smashing on you!\r\n",
		       obj2->short_description);
	      break;
	    case 4:
	      sprintf (buf,
		       "Come on darling. If you're going to wear %s with %s then you may aswell get \"FASHION VICTIM\" tattooed on your forehead!\r\n",
		       obj1->short_description, obj2->short_description);
	      break;
	    case 5:
	      sprintf (buf,
		       "If someone's told you that you look good in %s. They lied.\r\n",
		       obj1->short_description);
	      break;
	    case 6:
	      sprintf (buf,
		       "%s goes so well with %s. You'd be a fool not to wear these together!\r\n",
		       obj1->short_description, obj2->short_description);
	      break;
	    case 7:
	      sprintf (buf,
		       "%s really sets off the purple on those socks you're wearing!\r\n",
		       obj1->short_description);
	      break;
	    case 8:
	      sprintf (buf,
		       "%s will look brilliant when its blood stained!\r\n",
		       obj2->short_description);
	      break;
	    case 9:
	      sprintf (buf,
		       "%s with %s. The perfect look for the modern mage.\r\n",
		       obj1->short_description, obj2->short_description);
	      break;
	    case 10:
	      sprintf (buf,
		       "If we were living in Darask then maybe you'd pull it off. But here? I don't think so!\r\n");
	      break;
	    }
	}
      CAP (buf);
      send_to_char (buf, ch);
      return;
    }


  /* Calculate average damage on weapon type */
  if (GET_OBJ_TYPE (obj1) == ITEM_WEAPON)
    {
      dam1 = (((GET_OBJ_VAL (obj1, 2) + 1) / 2.0) * GET_OBJ_VAL (obj1, 1));
      dam2 = (((GET_OBJ_VAL (obj2, 2) + 1) / 2.0) * GET_OBJ_VAL (obj2, 1));
      /* Compare damages and print */
      if (dam1 < dam2)
	sprintf (buf, "%s does more damage than %s.\r\n",
		 obj2->short_description, obj1->short_description);
      else if (dam1 > dam2)
	sprintf (buf, "%s does more damage than %s.\r\n",
		 obj1->short_description, obj2->short_description);
      else
	sprintf (buf, "%s does the same amount of damage as %s.\r\n",
		 obj2->short_description, obj1->short_description);
    }
  else				/* object is armor */
  /* compare armor class */ if (GET_OBJ_VAL (obj1, 0) < GET_OBJ_VAL (obj2, 0))
    sprintf (buf, "%s looks more protective than %s.\r\n",
	     obj1->short_description, obj2->short_description);
  else if (GET_OBJ_VAL (obj1, 0) > GET_OBJ_VAL (obj2, 0))
    sprintf (buf, "%s looks more protective than %s.\r\n",
	     obj2->short_description, obj1->short_description);
  else
    sprintf (buf, "%s looks just as protective as %s.\r\n",
	     obj2->short_description, obj1->short_description);

  CAP (buf);
  send_to_char (buf, ch);

  /* Count affections */
  count1 = count2 = 0;
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if ((obj1->affected[i].modifier != 0))
	count1++;
      if ((obj2->affected[i].modifier != 0))
	count2++;
    }

  /* Compare affections and print */
  if (count1 < count2)
    {
      sprintf (buf, "%s has more affections than %s.\r\n",
	       obj2->short_description, obj1->short_description);
      CAP (buf);
      send_to_char (buf, ch);
    }
  else if (count1 > count2)
    {
      sprintf (buf, "%s has more affections than %s.\r\n",
	       obj1->short_description, obj2->short_description);
      CAP (buf);
      send_to_char (buf, ch);
    }
  else /* equal affections - no affections exist in both */ if (count1 == 0)
    {
      sprintf (buf, "Neither %s or %s have any affections.\r\n",
	       obj1->short_description, obj2->short_description);
      CAP (buf);
      send_to_char (buf, ch);
    }
  else				/* equal affections exist */
    {
      sprintf (buf, "%s has the same amount of affections as %s.\r\n",
	       obj1->short_description, obj2->short_description);
      CAP (buf);
      send_to_char (buf, ch);
    }


  /* object alignment */


  /* get align values */
  align1 = get_bit (obj1);
  align2 = get_bit (obj2);

  /* compare the values - if not equal , print */
  if (align1 != align2)
    {
      print_it (align1, obj1, ch);
      print_it (align2, obj2, ch);
    }

}

ACMD (do_compost)
{
  struct obj_data *obj, *jj, *j;
  int mana_regain;
  struct char_data *dummy;

  if (IS_NPC (ch) || !GET_SKILL (ch, SKILL_COMPOST))
    {
      send_to_char ("You have no idea how.\r\n", ch);
      return;
    }
  one_argument (argument, arg);
  if (!*arg)
    {
      send_to_char ("Compost what?\r\n", ch);
      return;
    }
  if (generic_find (arg, FIND_OBJ_ROOM | FIND_OBJ_INV, ch, &dummy, &obj) == 0)
    {
      send_to_char ("Could not find a corpse by that name.\r\n", ch);
      return;
    }
  if (!IS_CORPSE (obj))
    {
      send_to_char ("That is not a corpse!\r\n", ch);
      return;
    }
  if (GET_CORPSEID (obj) != 0)
    {
      send_to_char ("You can't compost player corpses!\r\n", ch);
      return;
    }
  /* Extract items to ground */

  if (obj->contains != NULL)
    for (j = obj->contains; j; j = jj)
      {
	jj = j->next_content;
	obj_from_obj (j);

	if (obj->in_obj)
	  obj_to_obj (j, obj->in_obj);
	else if (obj->carried_by)
	  obj_to_room (j, obj->carried_by->in_room);
	else if (obj->in_room != NOWHERE)
	  obj_to_room (j, obj->in_room);
	else
	  {
	    sprintf (buf, "%s doesn't appear to be anywhere.\r\n",
		     obj->description);
	    mudlog (buf, BRF, LVL_GOD, TRUE);
	    extract_obj (j);
	  }
      }

  mana_regain = (3 * GET_OBJ_COST (obj));
  mana_regain = (int) (APPLY_SPELL_EFFEC (ch, SKILL_COMPOST, mana_regain));
  extract_obj (obj);

  /* TODO - Efficiency */

  if (basic_skill_test (ch, SKILL_COMPOST, TRUE))
    {
      if (GET_MANA (ch) < GET_MAX_MANA (ch))
	GET_MANA (ch) =
	  MIN (GET_MAX_MANA (ch), (GET_MANA (ch) + mana_regain));
      send_to_char
	("You skillfully decompose the corpse, draining all remaining magical energy.\r\n",
	 ch);
    }
  act ("$n decomposes a corpse before your very eyes.\r\n", FALSE, ch, 0, 0,
       TO_ROOM);

}

ACMD (do_sneak)
{
  struct affected_type af;
  byte percent;

  if (IS_NPC (ch) || !GET_SKILL (ch, SKILL_SNEAK))
    {
      send_to_char ("You have no idea how to do that.\r\n", ch);
      return;
    }
  if (AFF_FLAGGED (ch, AFF_SNEAK))
    {
      send_to_char ("You try and be even more silent.\r\n", ch);
      return;
    }
  send_to_char ("Okay, you'll try to move silently for a while.\r\n", ch);

  percent = number (1, 101);	/* 101% is a complete failure */

  int nBonus = IS_SET (GET_SPECIALS (ch), SPECIAL_THIEF) ? 25 : 0;	// 25% bonus for thieves

  if (percent >
      GET_SKILL (ch,
		 SKILL_SNEAK) + dex_app_skill[GET_DEX (ch)].sneak + nBonus)
    return;

  apply_spell_skill_abil (ch, SKILL_SNEAK);

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL (ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char (ch, &af);
}



ACMD (do_hide)
{
  byte percent;

  if (!has_stats_for_skill (ch, SKILL_HIDE, TRUE))
    return;

  if (IS_NPC (ch) || !GET_SKILL (ch, SKILL_HIDE))
    {
      send_to_char ("You have no idea how to do that.\r\n", ch);
      return;
    }

  if (AFF_FLAGGED (ch, AFF_HIDE))
    REMOVE_BIT (AFF_FLAGS (ch), AFF_HIDE);

  percent = number (1, 101);	/* 101% is a complete failure */

  int nBonus = IS_SET (GET_SPECIALS (ch), SPECIAL_THIEF) ? 25 : 0;	// 25% bonus for thieves

  if (percent >
      GET_SKILL (ch, SKILL_HIDE) + dex_app_skill[GET_DEX (ch)].hide + nBonus)
    {
      send_to_char ("You unsuccessfully attempt to hide yourself.\r\n", ch);
      return;
    }

  send_to_char ("You hide yourself.\r\n", ch);
  SET_BIT (AFF_FLAGS (ch), AFF_HIDE);
  apply_spell_skill_abil (ch, SKILL_HIDE);
}




ACMD (do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int gold, eq_pos, pcsteal = 0, ohoh = 0;
  double percent;

  if (!has_stats_for_skill (ch, SKILL_STEAL, TRUE))
    return;

  if (IS_NPC (ch) || !GET_SKILL (ch, SKILL_STEAL))
    {
      send_to_char ("You have no idea how to do that.\r\n", ch);
      return;
    }
  if (ROOM_FLAGGED (IN_ROOM (ch), ROOM_PEACEFUL))
    {
      send_to_char (PEACEROOM, ch);
      return;
    }

  two_arguments (argument, obj_name, vict_name);

  if (!(vict = generic_find_char (ch, vict_name, FIND_CHAR_ROOM)))
    {
      send_to_char ("Steal what from who?\r\n", ch);
      return;
    }
  else if (vict == ch)
    {
      send_to_char ("Come on now, that's rather stupid!\r\n", ch);
      return;
    }
  if (MOB_FLAGGED (vict, MOB_NO_STEAL))
    {
      do_gen_comm (vict, "THIEF!!!.  I'll get you for that!!", 0, SCMD_SHOUT);
      hit (vict, ch, TYPE_UNDEFINED);
      return;
    }
  if (!pt_allowed && LR_FAIL (ch, LVL_IMPL))
    {
      if (GET_LEVEL (ch) < GET_LEVEL (vict))
	{
	  send_to_char ("I don't think they will appreciate that.\r\n", ch);
	  return;
	}
      if (!IS_NPC (vict) && !PLR_FLAGGED (vict, PLR_THIEF) &&
	  !PLR_FLAGGED (vict, PLR_KILLER) && !PLR_FLAGGED (ch, PLR_THIEF))
	{
	  /*
	   * SET_BIT(ch->specials.act, PLR_THIEF); send_to_char("Okay, you're the
	   * boss... you're now a THIEF!\r\n",ch); sprintf(buf, "PC Thief bit set
	   * on %s", GET_NAME(ch)); log(buf);
	   */
	  if (GET_POS (vict) == POS_SLEEPING)
	    {
	      send_to_char ("That isn't very sporting now is it!\n\r", ch);
	      act ("$n tried to steal something from you!", FALSE, ch, 0,
		   vict, TO_VICT);
	      GET_POS (vict) = POS_STANDING;
	      act ("$n rifles through $N's belongings as they sleep!", FALSE,
		   ch, 0, vict, TO_NOTVICT);
	      act ("$n wakes and stands up", FALSE, vict, 0, 0, TO_ROOM);
	      send_to_char
		("You are awakened by a thief going through your things!\n\r",
		 vict);
	      send_to_char ("You get a good look at the scoundrel!\n\r\n\r",
			    vict);
	      look_at_char (ch, vict);
	      return;
	    }

	  pcsteal = 1;
	}
      if (PLR_FLAGGED (ch, PLR_THIEF))
	pcsteal = 1;
    }
  /* 101% is a complete failure */
  percent = number (1, 101) - dex_app_skill[GET_DEX (ch)].p_pocket;

  // Adjust percent if thief is enhanced
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_THIEF))
    {
      percent -= percent * 0.15;
      if (percent <= 0)
	percent = 1;
    }

  if (GET_POS (vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!pt_allowed && !IS_NPC (vict))
    pcsteal = 1;

  if (!AWAKE (vict))		/* Easier to steal from sleeping people. */
    percent -= 50;

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (!LR_FAIL (vict, LVL_IS_GOD) || pcsteal ||
      GET_MOB_SPEC (vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (!IS_NPC (vict))		/* always fail on another player character */
    percent = 101;
  if (!LR_FAIL (vict, LVL_IMPL))	/* implementors always succeed! /bm/ */
    percent = -100;

  if (str_cmp (obj_name, "coins") && str_cmp (obj_name, "gold"))
    {

      if (!(obj = find_obj_list (ch, obj_name, vict->carrying)))
	{
	  for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	    if (GET_EQ (vict, eq_pos) &&
		(isname (obj_name, GET_EQ (vict, eq_pos)->name)) &&
		CAN_SEE_OBJ (ch, GET_EQ (vict, eq_pos)))
	      {
		obj = GET_EQ (vict, eq_pos);
		break;
	      }
	  if (!obj)
	    {
	      act ("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	      return;
	    }
	  else
	    {			/* It is equipment */
	      if (OBJ_RIDDEN (obj))
		{
		  send_to_char ("They're riding that. Too obvious.\r\n", ch);
		  return;
		}
	      else if ((GET_POS (vict) > POS_STUNNED))
		{
		  send_to_char ("Steal the equipment now?  Impossible!\r\n",
				ch);
		  return;
		}
	      else
		{
		  act ("You unequip $p and steal it.", FALSE, ch, obj, 0,
		       TO_CHAR);
		  act ("$n steals $p from $N.", FALSE, ch, obj, vict,
		       TO_NOTVICT);
		  obj_to_char (unequip_char (vict, eq_pos, TRUE), ch,
			       __FILE__, __LINE__);
		  apply_spell_skill_abil (ch, SKILL_STEAL);
		}
	    }
	}
      else
	{			/* obj found in inventory */
	  if (OBJ_RIDDEN (obj))
	    {
	      send_to_char ("They're riding that. Too obvious.\r\n", ch);
	      return;
	    }

	  percent += GET_OBJ_WEIGHT (obj);	/* Make heavy harder */

	  if (percent > GET_SKILL (ch, SKILL_STEAL))
	    {
	      ohoh = TRUE;
	      send_to_char ("Oops..\r\n", ch);
	      act ("$n tried to steal something from you!", FALSE, ch, 0,
		   vict, TO_VICT);
	      act ("$n tries to steal something from $N.", TRUE, ch, 0, vict,
		   TO_NOTVICT);
	    }
	  else
	    {			/* Steal the item */
	      if (IS_CARRYING_N (ch) + 1 < CAN_CARRY_N (ch))
		{
		  if (IS_CARRYING_W (ch) + GET_OBJ_WEIGHT (obj) <
		      CAN_CARRY_W (ch))
		    {
		      obj_from_char (obj);
		      obj_to_char (obj, ch, __FILE__, __LINE__);
		      apply_spell_skill_abil (ch, SKILL_STEAL);
		      send_to_char ("Got it!\r\n", ch);
		    }
		}
	      else
		send_to_char ("You cannot carry that much.\r\n", ch);
	    }
	}
    }
  else
    {				/* Steal some coins */
      if (AWAKE (vict) && (percent > GET_SKILL (ch, SKILL_STEAL)))
	{
	  ohoh = TRUE;
	  send_to_char ("Oops..\r\n", ch);
	  act ("You discover that $n has $s hands in your wallet.", FALSE, ch,
	       0, vict, TO_VICT);
	  act ("$n tries to steal gold from $N.", TRUE, ch, 0, vict,
	       TO_NOTVICT);
	}
      else
	{
	  /* Steal some gold coins */
	  if (IS_SET (zone_table[world[ch->in_room].zone].zflag, ZN_NO_STEAL)
	      && IS_NPC (vict))
	    {

	      do_gen_comm (vict, "THIEF!!!.  I'll get you for that!!", 0,
			   SCMD_SHOUT);
	      hit (vict, ch, TYPE_UNDEFINED);
	      return;
	    }
	  gold = (int) ((GET_GOLD (vict) * number (1, 10)) / 100);
	  gold = MIN (1782, gold);
	  if (gold > 0)
	    {
	      GET_GOLD (ch) += gold;
	      GET_GOLD (vict) -= gold;
	      if (gold > 1)
		{
		  sprintf (buf, "Bingo!  You got %d gold coins.\r\n", gold);
		  send_to_char (buf, ch);
		  apply_spell_skill_abil (ch, SKILL_STEAL);
		}
	      else
		{
		  send_to_char
		    ("You manage to swipe a solitary gold coin.\r\n", ch);
		}
	    }
	  else
	    {
	      send_to_char ("You couldn't get any gold...\r\n", ch);
	    }
	}
    }

  if (ohoh && IS_NPC (vict) && AWAKE (vict))
    hit (vict, ch, TYPE_UNDEFINED);
}



ACMD (do_practice)
{
  if (IS_NPC (ch))
    return;

  one_argument (argument, arg);

  if (*arg)
    send_to_char ("You can only practice skills in your guild.\r\n", ch);
  else
    list_skills (ch);
}



ACMD (do_visible)
{
  if (GET_LEVEL (ch) > LVL_ISNOT_GOD)
    {
      perform_immort_vis (ch);
      return;
    }
  if AFF_FLAGGED
    (ch, AFF_INVISIBLE)
    {
      appear (ch);
      send_to_char ("You break the spell of invisibility.\r\n", ch);
      return;
    }
  send_to_char ("You are already visible.\r\n", ch);
}

ACMD (do_title)
{
  skip_spaces (&argument);
  delete_doubledollar (argument);

  if (IS_NPC (ch))
    send_to_char ("Your title is fine... go away.\r\n", ch);
  else if (GET_MAX_LVL (ch) < 5)
    send_to_char ("You need to be level 5 to set your title.\r\n", ch);
//  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
  else if (PUN_FLAGGED (ch, PUN_NOTITLE))	/* ARTUS - Check PUN Bit... */
    send_to_char
      ("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if (strstr (argument, "(") || strstr (argument, ")"))
    send_to_char ("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strstr (argument, "[") || strstr (argument, "]"))
    send_to_char ("Titles can't contain the [ or ] characters.\r\n", ch);
  /*
     else if (scan_buffer_for_xword(argument))
     {
     sprintf(buf,"WATCHLOG SWEAR (Title): %s title : '%s'", ch->player.name, argument);
     mudlog(buf,NRM,LVL_IMPL,TRUE);
     send_to_char("Please dont swear in your title.\r\n",ch);
     return ;
     } 
     else if ((strlen(argument) > MAX_TITLE_LENGTH) || (strlen(argument) > GET_LEVEL(ch))) {
     sprintf(buf, "Sorry, your titles can't be longer than your level in characters, with a max of %d.\r\n", MAX_TITLE_LENGTH);
     send_to_char(buf, ch);
   */
  // Dont allow last char to be & ...
  else if (argument[strlen (argument) - 1] == '&')
    {
      send_to_char ("Titles can't end with '&&'.\r\n", ch);
      return;
    }
  else
    {
      if (strlen (argument) == 0)
	{
	  set_title (ch, NULL);
	  send_to_char ("Title removed.\r\n", ch);
	}
      else
	{
	  if (strlen (argument) > 78)
	    argument[79] = '\0';
	  strcat (argument, "&n");
	  set_title (ch, argument);
	  sprintf (buf, "Okay, you're now %s %s.\r\n", GET_NAME (ch),
		   GET_TITLE (ch));
	  send_to_char (buf, ch);
	}
    }
}


int
perform_group (struct char_data *ch, struct char_data *vict)
{
  // DM: NPC check for grouping
  if ((IS_NPC (vict) && !IS_CLONE (vict))
      || AFF_FLAGGED (vict, AFF_GROUP) || !CAN_SEE (ch, vict))
    return (0);

  SET_BIT (AFF_FLAGS (vict), AFF_GROUP);
  if (ch != vict)
    {
      act ("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act ("You are now a member of $n's group.", FALSE, ch, 0, vict,
	   TO_VICT);
      act ("$N is now a member of $n's group.", FALSE, ch, 0, vict,
	   TO_NOTVICT);
    }
  else
    {
      act ("You now head the group.", FALSE, ch, 0, vict, TO_VICT);
    }

  return (1);
}

void
color_perc (char col[], int curr, int max)
{
  if (curr < max / 4)
    strcpy (col, "&r");
  else if (curr < max / 2)
    strcpy (col, "&R");
  else if (curr < max)
    strcpy (col, "&Y");
  else
    strcpy (col, "&G");
}


void
print_group (struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;
  char hcol[3], mcol[3], vcol[3], fmt[MAX_STRING_LENGTH] = "";
  char *get_exp_prompt_string (struct char_data *ch);
  char *get_align_prompt_string (struct char_data *ch);
  int widths[5], i;

  if (!AFF_FLAGGED (ch, AFF_GROUP))
    {
      send_to_char ("But you are not the member of a group!\r\n", ch);
      return;
    }
  send_to_char ("Your group consists of:\r\n", ch);

  k = (ch->master ? ch->master : ch);
  widths[0] = GET_HIT (k);
  widths[1] = GET_MANA (k);
  widths[2] = GET_MOVE (k);
  if (!IS_NPC (k))
    widths[3] = strlen (get_exp_prompt_string (k));
  else
    widths[3] = 5;
  if (GET_ALIGNMENT (k) < 0)
    widths[4] = GET_ALIGNMENT (k) * -10;
  else
    widths[4] = GET_ALIGNMENT (k);

  for (f = k->followers; f; f = f->next)
    {
      widths[0] = MAX (GET_HIT (f->follower), widths[0]);
      widths[1] = MAX (GET_MANA (f->follower), widths[1]);
      widths[2] = MAX (GET_MOVE (f->follower), widths[2]);
      if (!IS_NPC (f->follower) && CAN_LEVEL (f->follower))
	widths[3] =
	  MAX (widths[3], strlen (get_exp_prompt_string (f->follower)));
      if (GET_ALIGNMENT (f->follower) < 0)
	widths[4] = MAX (widths[4], GET_ALIGNMENT (f->follower) * -10);
      else
	widths[4] = MAX (widths[4], GET_ALIGNMENT (f->follower));
    }

  for (i = 0; i < 5; i++)
    {
      if (i == 3)
	continue;
      if (widths[i] > 9999)
	widths[i] = 5;
      else if (widths[i] > 999)
	widths[i] = 4;
      else if (widths[i] > 99)
	widths[i] = 3;
      else if (widths[i] > 9)
	widths[i] = 2;
      else
	widths[i] = 2;
    }
  widths[4] += 2;
  sprintf (fmt,
	   "  &B[%%s%%%dd&nH %%s%%%dd&nM %%s%%%dd&nV %%%d.%ds&nTnl %%%d.%ds&nAl&B] [&n%%3d %%s&B]&n %%s",
	   widths[0], widths[1], widths[2], widths[3], widths[3], widths[4],
	   widths[4]);

  color_perc (hcol, GET_HIT (k), GET_MAX_HIT (k));
  color_perc (mcol, GET_MANA (k), GET_MAX_MANA (k));
  color_perc (vcol, GET_MOVE (k), GET_MAX_MOVE (k));

  if (AFF_FLAGGED (k, AFF_GROUP))
    {
      sprintf (buf, fmt,
	       hcol, GET_HIT (k), mcol, GET_MANA (k), vcol, GET_MOVE (k),
	       get_exp_prompt_string (k), get_align_prompt_string (k),
	       GET_LEVEL (k), CLASS_ABBR (k), GET_NAME (k));
      strcat (buf, " &R(Leader)&n\r\n");
      send_to_char (buf, ch);
//    act(buf, FALSE, ch, 0, k, TO_CHAR);
    }
  strcat (fmt, "\r\n");

  for (f = k->followers; f; f = f->next)
    {
      if (!AFF_FLAGGED (f->follower, AFF_GROUP))
	continue;

      color_perc (hcol, GET_HIT (f->follower), GET_MAX_HIT (f->follower));
      color_perc (mcol, GET_MANA (f->follower), GET_MAX_MANA (f->follower));
      color_perc (vcol, GET_MOVE (f->follower), GET_MAX_MOVE (f->follower));

      sprintf (buf, fmt,
	       /*"     &B[%s%4d&nH %s%4d&nM %s%4d&nV Tnl:&c%-8.8s&n Al:%-7.7s&B] [&n%3d %s&B]&n $N", */
	       hcol, GET_HIT (f->follower),
	       mcol, GET_MANA (f->follower), vcol, GET_MOVE (f->follower),
	       get_exp_prompt_string (f->follower),
	       get_align_prompt_string (f->follower), GET_LEVEL (f->follower),
	       CLASS_ABBR (f->follower), GET_NAME (f->follower));
      send_to_char (buf, ch);
//    act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
}

void
apply_group_sneak (struct char_data *ch)
{
  struct follow_type *f, *fnext;
  struct char_data *teacher = NULL;
  bool bApplySneak = FALSE;

  // See if either the leader or anyone else has GROUP_SNEAK
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_GROUP_SNEAK))
    {
      teacher = ch;
      bApplySneak = TRUE;
    }
  else				// Group?
    {
      for (f = ch->followers; f; f = fnext)
	{
	  fnext = f->next;

	  if (IS_SET (GET_SPECIALS (f->follower), SPECIAL_GROUP_SNEAK))
	    {
	      bApplySneak = TRUE;
	      send_to_char
		("You are able to help your group members move silently.\r\n",
		 f->follower);
	      teacher = f->follower;
	      break;
	    }
	}
    }

  // See if the group has a group sneaking member
  if (bApplySneak)
    {
      // Create the affect, and give it to all the members
      struct affected_type af;
      af.type = SKILL_SNEAK;
      af.duration = CLASS_ABILITY;
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bitvector = AFF_SNEAK;

      // Give it to the leader first
      if (ch != teacher)
	{
	  if (!affected_by_spell (ch, SKILL_SNEAK))
	    {
	      affect_to_char (ch, &af);
	      sprintf (buf,
		       "%s, as part of your group, is able to help you move silently.\r\n",
		       GET_NAME (teacher));
	      send_to_char (buf, ch);
	    }
	  else
	    {
	      sprintf (buf,
		       "%s tries to show you how to move silently, but you already know!\r\n",
		       GET_NAME (teacher));
	      send_to_char (buf, ch);
	    }
	}

      // Give it to the rest of the group
      for (f = ch->followers; f; f = fnext)
	{
	  fnext = f->next;
	  if (teacher != f->follower)
	    {
	      if (!affected_by_spell (f->follower, SKILL_SNEAK))
		{
		  affect_to_char (f->follower, &af);
		  sprintf (buf,
			   "%s, as part of your group, is able to help you move silently.\r\n",
			   GET_NAME (teacher));
		  send_to_char (buf, f->follower);
		}
	      else
		{
		  sprintf (buf,
			   "%s tries to show you how to move silently, but you already know!\r\n",
			   GET_NAME (teacher));
		  send_to_char (buf, f->follower);
		}
	    }
	}
    }
}

void
unapply_group_sneak (struct char_data *ch, struct char_data *vict)
{
  struct follow_type *f, *fnext;

  // Vict has been kicked. See if they're not the one with the group sneak
  if (!IS_SET (GET_SPECIALS (vict), SPECIAL_GROUP_SNEAK))
    {
      if (!IS_SET (GET_SPECIALS (vict), SPECIAL_SNEAK))
	{
	  // Simply unaffect them, and we're done
	  ability_from_char (vict, SKILL_SNEAK);
	  send_to_char
	    ("As you depart from the group, so does your ability to move silently.\r\n",
	     vict);
	}

      return;
    }

  // They had the group sneak. See if anyone else in the group has it.
  bool bFound = FALSE;

  // Check the leader
  if (IS_SET (GET_SPECIALS (ch), SPECIAL_GROUP_SNEAK))
    bFound = TRUE;
  else				// Check the sheep 
    {
      for (f = ch->followers; f; f = fnext)
	{
	  fnext = f->next;
	  if (IS_SET (GET_SPECIALS (f->follower), SPECIAL_GROUP_SNEAK))
	    bFound = TRUE;
	}
    }

  // Departing character was last one with skill. Take them all off.
  if (!bFound)
    {
      // Leader loses it first
      if (!IS_SET (GET_SPECIALS (ch), SPECIAL_SNEAK))
	{
	  ability_from_char (ch, SKILL_SNEAK);
	  sprintf (buf,
		   "As %s departs the group, so does your ability to move silently.\r\n",
		   GET_NAME (vict));
	  send_to_char (buf, ch);
	}

      // And now for the remaining ones
      for (f = ch->followers; f; f = fnext)
	{
	  fnext = f->next;
	  if (!IS_SET (GET_SPECIALS (f->follower), SPECIAL_SNEAK))
	    {
	      ability_from_char (f->follower, SKILL_SNEAK);
	      sprintf (buf,
		       "As %s departs the group, so does your ability to move silently.\r\n",
		       GET_NAME (vict));
	      send_to_char (buf, f->follower);
	    }
	}
    }
}

ACMD (do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument (argument, buf);

  if (!*buf)
    {
      print_group (ch);
      return;
    }

  if (ch->master)
    {
      act ("You can not enroll group members without being head of a group.",
	   FALSE, ch, 0, 0, TO_CHAR);
      return;
    }

  if (!str_cmp (buf, "all"))
    {
      perform_group (ch, ch);
      for (found = 0, f = ch->followers; f; f = f->next)
	found += perform_group (ch, f->follower);
      if (!found)
	send_to_char ("Everyone following you is already in your group.\r\n",
		      ch);
      apply_group_sneak (ch);
      return;
    }

  if (!(vict = generic_find_char (ch, buf, FIND_CHAR_ROOM)))
    send_to_char (NOPERSON, ch);
  else if ((vict->master != ch) && (vict != ch))
    act ("$N must follow you to enter your group.", FALSE, ch, 0, vict,
	 TO_CHAR);
  else
    {
      if (!AFF_FLAGGED (vict, AFF_GROUP))
	{
	  perform_group (ch, vict);
	  apply_group_sneak (ch);
	}
      else
	{
	  if (ch != vict)
	    act ("$N is no longer a member of your group.", FALSE, ch, 0,
		 vict, TO_CHAR);
	  act ("You have been kicked out of $n's group!", FALSE, ch, 0, vict,
	       TO_VICT);
	  act ("$N has been kicked out of $n's group!", FALSE, ch, 0, vict,
	       TO_NOTVICT);
	  unapply_group_sneak (ch, vict);
	  REMOVE_BIT (AFF_FLAGS (vict), AFF_GROUP);
	}
    }

}



ACMD (do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument (argument, buf);

  if (!*buf)
    {
      if (ch->master || !(AFF_FLAGGED (ch, AFF_GROUP)))
	{
	  send_to_char ("But you lead no group!\r\n", ch);
	  return;
	}
      sprintf (buf2, "%s has disbanded the group.\r\n", GET_NAME (ch));
      for (f = ch->followers; f; f = next_fol)
	{
	  next_fol = f->next;
	  if (AFF_FLAGGED (f->follower, AFF_GROUP))
	    {
	      unapply_group_sneak (ch, f->follower);
	      REMOVE_BIT (AFF_FLAGS (f->follower), AFF_GROUP);
	      send_to_char (buf2, f->follower);
	      if (!AFF_FLAGGED (f->follower, AFF_CHARM))
		stop_follower (f->follower);
	    }
	}

      REMOVE_BIT (AFF_FLAGS (ch), AFF_GROUP);
      send_to_char ("You disband the group.\r\n", ch);
      return;
    }
  if (!(tch = generic_find_char (ch, buf, FIND_CHAR_ROOM)))
    {
      send_to_char ("There is no such person!\r\n", ch);
      return;
    }
  if (tch->master != ch)
    {
      send_to_char ("That person is not following you!\r\n", ch);
      return;
    }
  if (!AFF_FLAGGED (tch, AFF_GROUP))
    {
      send_to_char ("That person isn't in your group.\r\n", ch);
      return;
    }

  unapply_group_sneak (ch, tch);
  REMOVE_BIT (AFF_FLAGS (tch), AFF_GROUP);

  act ("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act ("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act ("$N has been kicked out of $n's group!", FALSE, ch, 0, tch,
       TO_NOTVICT);

  if (!AFF_FLAGGED (tch, AFF_CHARM))
    stop_follower (tch);
}




ACMD (do_report)
{
  struct char_data *k;
  struct follow_type *f;
  char hcol[5], mcol[5], vcol[6];

  if (!AFF_FLAGGED (ch, AFF_GROUP))
    {
      send_to_char ("But you are not a member of any group!\r\n", ch);
      return;
    }

  color_perc (hcol, GET_HIT (ch), GET_MAX_HIT (ch));
  color_perc (mcol, GET_MANA (ch), GET_MAX_MANA (ch));
  color_perc (vcol, GET_MOVE (ch), GET_MAX_MOVE (ch));

  sprintf (buf,
	   "%s reports: %s%d&n/&G%d&nH, %s%d&n/&G%d&nM, %s%d&n/&G%d&nV\r\n",
	   GET_NAME (ch), hcol, GET_HIT (ch), GET_MAX_HIT (ch), mcol,
	   GET_MANA (ch), GET_MAX_MANA (ch), vcol, GET_MOVE (ch),
	   GET_MAX_MOVE (ch));

  CAP (buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED (f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char (buf, f->follower);
  if (k != ch)
    send_to_char (buf, k);
  send_to_char ("You report to the group.\r\n", ch);
}



ACMD (do_split)
{
  int amount, num, share, rest;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC (ch))
    return;

  one_argument (argument, buf);

  if (is_number (buf))
    {
      amount = atoi (buf);
      if (amount <= 0)
	{
	  send_to_char ("Sorry, you can't do that.\r\n", ch);
	  return;
	}
      if (amount > GET_GOLD (ch))
	{
	  send_to_char ("You don't seem to have that much gold to split.\r\n",
			ch);
	  return;
	}
      k = (ch->master ? ch->master : ch);

      if (AFF_FLAGGED (k, AFF_GROUP) && (k->in_room == ch->in_room))
	num = 1;
      else
	num = 0;

      for (f = k->followers; f; f = f->next)
	if (AFF_FLAGGED (f->follower, AFF_GROUP) &&
	    (!IS_NPC (f->follower)) && (f->follower->in_room == ch->in_room))
	  num++;

      if (num && AFF_FLAGGED (ch, AFF_GROUP))
	{
	  share = amount / num;
	  rest = amount % num;
	}
      else
	{
	  send_to_char ("With whom do you wish to share your gold?\r\n", ch);
	  return;
	}

      GET_GOLD (ch) -= share * (num - 1);

      sprintf (buf, "%s splits &Y%d&n coins; you receive &Y%d&n.\r\n",
	       GET_NAME (ch), amount, share);
      if (rest)
	{
	  sprintf (buf + strlen (buf),
		   "&Y%d&n coin%s %s not splitable, so %s "
		   "keeps the money.\r\n", rest, (rest == 1) ? "" : "s",
		   (rest == 1) ? "was" : "were", GET_NAME (ch));
	}
      if (AFF_FLAGGED (k, AFF_GROUP) && (k->in_room == ch->in_room)
	  && !(IS_NPC (k)) && k != ch)
	{
	  GET_GOLD (k) += share;
	  send_to_char (buf, k);
	}
      for (f = k->followers; f; f = f->next)
	{
	  if (AFF_FLAGGED (f->follower, AFF_GROUP) &&
	      (!IS_NPC (f->follower)) &&
	      (f->follower->in_room == ch->in_room) && f->follower != ch)
	    {
	      GET_GOLD (f->follower) += share;
	      send_to_char (buf, f->follower);
	    }
	}
      sprintf (buf,
	       "You split &Y%d&n coins among %d members -- &Y%d&n coins each.\r\n",
	       amount, num, share);
      if (rest)
	{
	  sprintf (buf + strlen (buf),
		   "&Y%d&n coin%s %s not splitable, so you keep "
		   "the money.\r\n", rest, (rest == 1) ? "" : "s",
		   (rest == 1) ? "was" : "were");
	  GET_GOLD (ch) += rest;
	}
      send_to_char (buf, ch);
    }
  else
    {
      send_to_char
	("How many coins do you wish to split with your group?\r\n", ch);
      return;
    }
}



ACMD (do_use)
{
  struct obj_data *mag_item;

  half_chop (argument, arg, buf);
  if (!*arg)
    {
      sprintf (buf2, "What do you want to %s?\r\n", CMD_NAME);
      send_to_char (buf2, ch);
      return;
    }
  mag_item = GET_EQ (ch, WEAR_HOLD);

  if (IS_AFFECTED (ch, AFF_BERSERK))
    {
      send_to_char ("You are too entranced to use objects.\r\n", ch);
      return;
    }

  if (!mag_item || !isname (arg, mag_item->name))
    {
      switch (subcmd)
	{
	case SCMD_RECITE:
	case SCMD_QUAFF:
	  if (!(mag_item = find_obj_list (ch, arg, ch->carrying)))
	    {
	      sprintf (buf2, "You don't seem to have %s %s.\r\n", AN (arg),
		       arg);
	      send_to_char (buf2, ch);
	      return;
	    }
	  break;
	case SCMD_USE:
	  sprintf (buf2, "You don't seem to be holding %s %s.\r\n", AN (arg),
		   arg);
	  send_to_char (buf2, ch);
	  return;
	default:
	  basic_mud_log ("SYSERR: Unknown subcmd %d passed to do_use.",
			 subcmd);
	  return;
	}
    }

  switch (subcmd)
    {
    case SCMD_QUAFF:
      if (GET_OBJ_TYPE (mag_item) != ITEM_POTION)
	{
	  send_to_char ("You can only quaff potions.\r\n", ch);
	  return;
	}
      break;
    case SCMD_RECITE:
      if (GET_OBJ_TYPE (mag_item) != ITEM_SCROLL)
	{
	  send_to_char ("You can only recite scrolls.\r\n", ch);
	  return;
	}
      break;
    case SCMD_USE:
      if ((GET_OBJ_TYPE (mag_item) != ITEM_WAND) &&
	  (GET_OBJ_TYPE (mag_item) != ITEM_STAFF))
	{
	  send_to_char ("You can't seem to figure out how to use it.\r\n",
			ch);
	  return;
	}
      break;
    }

  // DM - check obj restrictions
  if (invalid_level (ch, mag_item, TRUE))
    return;

  mag_objectmagic (ch, mag_item, buf);
}



ACMD (do_wimpy)
{
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC (ch))
    return;

  one_argument (argument, arg);

  if (!*arg)
    {
      if (GET_WIMP_LEV (ch))
	{
	  sprintf (buf, "Your current wimp level is %d hit points.\r\n",
		   GET_WIMP_LEV (ch));
	  send_to_char (buf, ch);
	  return;
	}
      else
	{
	  send_to_char
	    ("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
	  return;
	}
    }
  if (isdigit (*arg))
    {
      if ((wimp_lev = atoi (arg)) != 0)
	{
	  if (wimp_lev < 0)
	    send_to_char ("Heh, heh, heh.. we are jolly funny today, eh?\r\n",
			  ch);
	  else if (wimp_lev > GET_MAX_HIT (ch))
	    send_to_char ("That doesn't make much sense, now does it?\r\n",
			  ch);
	  else if (wimp_lev > (GET_MAX_HIT (ch) / 2))
	    send_to_char
	      ("You can't set your wimp level above half your hit points.\r\n",
	       ch);
	  else
	    {
	      sprintf (buf,
		       "Okay, you'll wimp out if you drop below %d hit points.\r\n",
		       wimp_lev);
	      send_to_char (buf, ch);
	      GET_WIMP_LEV (ch) = wimp_lev;
	    }
	}
      else
	{
	  send_to_char
	    ("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
	  GET_WIMP_LEV (ch) = 0;
	}
    }
  else
    send_to_char
      ("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n",
       ch);
}


ACMD (do_display)
{
  size_t i;
  if (IS_NPC (ch))
    {
      send_to_char ("Mosters don't need displays.  Go away.\r\n", ch);
      return;
    }
  char **prompt = &GET_PROMPT (ch);

  half_chop (argument, buf, buf1);

  if (!*buf)
    {
      if (*prompt)
	{
	  sprintf (buf, "Current user defined prompt: '%s'\r\n", *prompt);
	  send_to_char (buf, ch);
	}
      send_to_char ("&1Usage: &4prompt { H | M | V | X | L | all | none | "
		    "set prompt_string}&n\r\n", ch);
      return;
    }

  if (*prompt)
    free (*prompt);
  *prompt = NULL;

  if ((!str_cmp (buf, "on")) || (!str_cmp (buf, "all")))
    SET_BIT (PRF_FLAGS (ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE |
	     PRF_DISPEXP | PRF_DISPALIGN);
  else
    {
      REMOVE_BIT (PRF_FLAGS (ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE |
		  PRF_DISPEXP | PRF_DISPALIGN);

      if (!str_cmp (buf, "set"))
	{
	  if (!*buf1)
	    {
	      send_to_char ("User defined prompt cleared.\r\n", ch);
	    }
	  else
	    {
	      if (strlen (buf1) >= 25)
		{
		  buf1[25] = '\0';
		  sprintf (buf,
			   "User defined prompt length greater than &c25&n chars."
			   "\r\nTruncated to '%s'\r\n", buf1);
		  send_to_char (buf, ch);
		}
	      *prompt = str_dup (buf1);
	    }
	}
      else
	{
	  for (i = 0; i < strlen (buf); i++)
	    {
	      switch (LOWER (buf[i]))
		{
		case 'h':
		  SET_BIT (PRF_FLAGS (ch), PRF_DISPHP);
		  break;
		case 'm':
		  SET_BIT (PRF_FLAGS (ch), PRF_DISPMANA);
		  break;
		case 'v':
		  SET_BIT (PRF_FLAGS (ch), PRF_DISPMOVE);
		  break;
		case 'x':
		  SET_BIT (PRF_FLAGS (ch), PRF_DISPEXP);
		  break;
		case 'l':
		  SET_BIT (PRF_FLAGS (ch), PRF_DISPALIGN);
		  break;
		}
	    }
	}
    }
  send_to_char (OK, ch);
}


#if 0				// Artus> No Longer Required.
ACMD (do_gen_write)
{
  FILE *fl;
  char *tmp, buf[MAX_STRING_LENGTH];
  const char *filename;
  struct stat fbuf;
#ifndef NO_LOCALTIME
  time_t ct;
#else
  struct tm lt;
#endif

  switch (subcmd)
    {
    case SCMD_BUG:
      filename = BUG_FILE;
      break;
    case SCMD_TYPO:
      filename = TYPO_FILE;
      break;
    case SCMD_IDEA:
      filename = IDEA_FILE;
      break;
    case SCMD_NEWBIE:
      filename = NEWBIE_FILE;
      break;
    default:
      return;
    }

#ifndef NO_LOCALTIME
  ct = time (0);
  tmp = asctime (localtime (&ct));
#else
  if (jk_localtime_now (&lt))
    {
      basic_mud_log ("Error in jk_localtime_now. [%s:%d]", __FILE__,
		     __LINE__);
      return;
    }
  tmp = asctime (&lt);
#endif

  if (IS_NPC (ch) && subcmd != SCMD_NEWBIE)
    {
      send_to_char ("Monsters can't have ideas - Go away.\r\n", ch);
      return;
    }

  skip_spaces (&argument);
  delete_doubledollar (argument);
  if (!*argument)
    {
      send_to_char ("That must be a mistake...\r\n", ch);
      return;
    }
  sprintf (buf, "%s %s: %s", GET_NAME (ch), CMD_NAME, argument);

  if (subcmd != SCMD_NEWBIE)
    mudlog (buf, CMP, LVL_IMMORT, FALSE);

  if (stat (filename, &fbuf) < 0)
    {
      perror ("SYSERR: Can't stat() file");
      return;
    }
  if (fbuf.st_size >= max_filesize && subcmd != SCMD_NEWBIE)
    {
      send_to_char
	("Sorry, the file is full right now.. try again later.\r\n", ch);
      return;
    }
  if (!(fl = fopen (filename, "a")))
    {
      perror ("SYSERR: do_gen_write");
      if (subcmd != SCMD_NEWBIE)
	send_to_char ("Could not open the file.  Sorry.\r\n", ch);
      return;
    }
  fprintf (fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME (ch), (tmp + 4),
	   GET_ROOM_VNUM (IN_ROOM (ch)), argument);
  fclose (fl);
  if (subcmd != SCMD_NEWBIE)
    send_to_char ("Okay.  Thanks!\r\n", ch);
}
#endif


#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD (do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
     "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
     "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
     "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
     "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
     "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
     "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
     "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
     "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
     "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
     "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
     "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
     "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
     "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
     "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
     "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
     "Autoexits enabled.\r\n"},
    {"You can now hear the Immnet Channel.\r\n",
     "You are now deaf to the Immnet Channel.\r\n"},
    {"You are no longer marked as (AFK).\r\n",
     "You are now marked as (AFK).  You will receive no tells!\r\n"},
    {"You can see the info channel now.\r\n",
     "You can't see the info channel now.\r\n"},
    {"You can hear newbies now.\r\n",
     "You are now deaf to newbies.\r\n"},
    {"You will no longer autoloot corpses.\r\n",
     "You will autoloot corpses now.\r\n"},
    {"You will no longer loot gold from corpses.\r\n",
     "You will now loot gold from corpses.\r\n"},
    {"You will no longer autosplit gold when grouped.\r\n",
     "You will now autosplit gold when grouped.\r\n"},
    {"Will no longer track through doors.\r\n",
     "Will now track through doors.\r\n"},
    {"You will now see the hint channel.\r\n",
     "You will no longer see the hint channel.\r\n"},
    {"You will now hear your clan channel.\r\n",
     "You will no longer hear your clan channel.\r\n"},
    {"You will now see clan info messages.\r\n",
     "You will no longer see clan info messages.\r\n"},
    {"You will no longer have your corpse retrieved.\r\n",
     "You will now have your corpse retrieved.\r\n"},
    {"You will no longer automatically eat when hungry.\r\n",
     "You will now automatically eat when hungry.\r\n"}
  };


  if (IS_NPC (ch))
    return;

  switch (subcmd)
    {
    case SCMD_NOSUMMON:
      result = PRF_TOG_CHK (ch, PRF_SUMMONABLE);
      break;
    case SCMD_NOHASSLE:
      result = PRF_TOG_CHK (ch, PRF_NOHASSLE);
      break;
    case SCMD_BRIEF:
      result = PRF_TOG_CHK (ch, PRF_BRIEF);
      break;
    case SCMD_COMPACT:
      result = PRF_TOG_CHK (ch, PRF_COMPACT);
      break;
    case SCMD_NOTELL:
      result = PRF_TOG_CHK (ch, PRF_NOTELL);
      break;
    case SCMD_NOAUCTION:
      result = PRF_TOG_CHK (ch, PRF_NOAUCT);
      break;
    case SCMD_DEAF:
      result = PRF_TOG_CHK (ch, PRF_DEAF);
      break;
    case SCMD_NOGOSSIP:
      result = PRF_TOG_CHK (ch, PRF_NOGOSS);
      break;
    case SCMD_NOGRATZ:
      result = PRF_TOG_CHK (ch, PRF_NOGRATZ);
      break;
    case SCMD_NOWIZ:
      result = PRF_TOG_CHK (ch, PRF_NOWIZ);
      break;
    case SCMD_ROOMFLAGS:
      result = PRF_TOG_CHK (ch, PRF_ROOMFLAGS);
      break;
    case SCMD_NOREPEAT:
      result = PRF_TOG_CHK (ch, PRF_NOREPEAT);
      break;
    case SCMD_HOLYLIGHT:
      result = PRF_TOG_CHK (ch, PRF_HOLYLIGHT);
      break;
    case SCMD_SLOWNS:
      result = (nameserver_is_slow = !nameserver_is_slow);
      break;
    case SCMD_AUTOEXIT:
      result = PRF_TOG_CHK (ch, PRF_AUTOEXIT);
      break;
    case SCMD_AFK:
      result = PRF_TOG_CHK (ch, PRF_AFK);
      break;
    case SCMD_NOINFO:
      result = PRF_TOG_CHK (ch, PRF_NOINFO);
      break;
    case SCMD_NONEWBIE:
      result = EXT_TOG_CHK (ch, EXT_NONEWBIE);
      break;
    case SCMD_AUTOLOOT:
      result = EXT_TOG_CHK (ch, EXT_AUTOLOOT);
      break;
    case SCMD_AUTOGOLD:
      result = EXT_TOG_CHK (ch, EXT_AUTOGOLD);
      break;
    case SCMD_AUTOSPLIT:
      result = EXT_TOG_CHK (ch, EXT_AUTOSPLIT);
      break;
    case SCMD_TRACK:
      result = (track_through_doors = !track_through_doors);
      break;
    case SCMD_NOHINTS:
      result = EXT_TOG_CHK (ch, EXT_NOHINTS);
      break;
    case SCMD_NOCT:
      result = EXT_TOG_CHK (ch, EXT_NOCT);
      break;
    case SCMD_NOCI:
      result = EXT_TOG_CHK (ch, EXT_NOCI);
      break;
    case SCMD_AUTOCORPSE:
      result = EXT_TOG_CHK (ch, EXT_AUTOCORPSE);
      break;
    case SCMD_AUTOEAT:
      result = EXT_TOG_CHK (ch, EXT_AUTOEAT);
      break;
    default:
      basic_mud_log ("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
      return;
    }

  if (result)
    send_to_char (tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char (tog_messages[subcmd][TOG_OFF], ch);

  return;
}

/* DM - modified spy skill written by Eric V. Bahmer */
ACMD (do_spy)
{
  int percent, prob, spy_type, return_room;
  char direction[MAX_INPUT_LENGTH];
  extern const char *dirs[];

  if (!has_stats_for_skill (ch, SKILL_SPY, TRUE))
    return;

  half_chop (argument, direction, buf);

  /* 101% is a complete failure */
  percent = number (1, 101);
  prob = GET_SKILL (ch, SKILL_SPY);
  spy_type = search_block (direction, dirs, FALSE);

  if ((spy_type < 0) || !EXIT (ch, spy_type) ||
      (EXIT (ch, spy_type)->to_room == NOWHERE))
    {
      send_to_char ("Spy where?\r\n", ch);
      return;
    }
  if (LR_FAIL (ch, LVL_CHAMP) && !(GET_MOVE (ch) >= 7))
    {
      send_to_char ("You don't have enough movement points.\r\n", ch);
      return;
    }
  if (percent > prob)
    {
      send_to_char ("You miserably attempt to spy.\r\n", ch);
      GET_MOVE (ch) = MAX (0, MIN (GET_MAX_MOVE (ch), GET_MOVE (ch) - 2));
      return;
    }
  if ((IS_SET (EXIT (ch, spy_type)->exit_info, EX_CLOSED) &&
       EXIT (ch, spy_type)->keyword))
    {
      sprintf (buf, "The %s is closed.\r\n",
	       fname (EXIT (ch, spy_type)->keyword));
      send_to_char (buf, ch);
      if (LR_FAIL (ch, LVL_CHAMP))
	GET_MOVE (ch) = MAX (0, MIN (GET_MAX_MOVE (ch), GET_MOVE (ch) - 2));
      return;
    }
  if (ROOM_FLAGGED (world[ch->in_room].dir_option[spy_type]->to_room,
		    ROOM_HOUSE | ROOM_PRIVATE))
    {
      send_to_char ("Stick your nosy nose somewhere else!\r\n", ch);
      return;
    }
  GET_MOVE (ch) = MAX (0, MIN (GET_MAX_MOVE (ch), GET_MOVE (ch) - 5));
  return_room = ch->in_room;
  char_from_room (ch);
  char_to_room (ch, world[return_room].dir_option[spy_type]->to_room);
  send_to_char ("You spy into the next room and see: \r\n\r\n", ch);
  look_at_room (ch, 1);
  char_from_room (ch);
  char_to_room (ch, return_room);
  act ("$n peeks into the next room.", TRUE, ch, 0, 0, TO_NOTVICT);
}

ACMD (do_pagewidth)
{
  int page_width = 80;

  one_argument (argument, arg);

  if (!*arg)
    {
      sprintf (buf, "Your current page width is &c%d&n chars.\r\n",
	       GET_PAGE_WIDTH (ch));
      send_to_char (buf, ch);
      return;
    }
  if (!is_number (arg))
    {
      if (!is_abbrev (arg, "default"))
	{
	  send_to_char ("Just what did you want to set it to?!?\r\n", ch);
	  return;
	}
    }
  else
    {
      page_width = atoi (arg);
      if ((page_width < 40) || (page_width > 250))
	{
	  send_to_char
	    ("Page Width must be between &c40&n-&c250&n chars.\r\n", ch);
	  return;
	}
    }
  GET_PAGE_WIDTH (ch) = page_width;
  sprintf (buf, "Page Width changed to &c%d&n chars.\r\n",
	   GET_PAGE_WIDTH (ch));
  send_to_char (buf, ch);
  save_char (ch, NOWHERE);
}

ACMD (do_pagelength)
{
  int page_length = 24;

  one_argument (argument, arg);

  if (!*arg)
    {
      sprintf (buf, "Your current page length is &c%d&n lines.\r\n",
	       GET_PAGE_LENGTH (ch));
      send_to_char (buf, ch);
      return;
    }
  if (!is_number (arg))
    {
      if (!is_abbrev (arg, "default"))
	{
	  send_to_char ("Just what exactly did you want to set it to?!?\r\n",
			ch);
	  return;
	}
    }
  else
    {
      page_length = atoi (arg);
      if ((page_length < 10) || (page_length > 100))
	{
	  send_to_char
	    ("Page Length must be between &c10&n-&c100&n lines.\r\n", ch);
	  return;
	}
    }
  GET_PAGE_LENGTH (ch) = page_length;
  sprintf (buf, "Page Length changed to &c%d&n lines.\r\n",
	   GET_PAGE_LENGTH (ch));
  send_to_char (buf, ch);
  save_char (ch, NOWHERE);
}

/* Autoassist - DM */
ACMD (do_autoassist)
{
  struct char_data *vict, *test;
  struct assisters_type *k;
  char assistname[MAX_INPUT_LENGTH];

  if (IS_NPC (ch))
    {
      send_to_char ("I'm a mob, I ain't autoassisting anyone!\r\n", ch);
      return;
    }

  one_argument (argument, assistname);
  vict = AUTOASSIST (ch);

  /* No argument */
  if (!*assistname)
    {
      if (vict)
	{
	  stop_assisting (ch);
	}
      else
	{
	  act ("Autoassist who?", FALSE, ch, 0, 0, TO_CHAR);
	}

      /* Argument given */
    }
  else
    {
      test = generic_find_char (ch, assistname, FIND_CHAR_ROOM);
      /* Already autoassisting */
      if (vict)
	{

	  /* Couldn't find target */
	  if (!(test))
	    {
	      act (NOPERSON, FALSE, ch, 0, 0, TO_CHAR);

	      /* Found different Target */
	    }
	  else if (test != vict)
	    {
	      act ("You are already assisting $N.", TRUE, ch, 0, vict,
		   TO_CHAR);

	      /* Found same Target */
	    }
	  else
	    {
	      act ("You are already autoassisting $M.", TRUE, ch, 0, vict,
		   TO_CHAR);
	    }
	}
      else
	{
	  /* Dont have victim */
	  if ((ch != test) && (test))
	    {
	      act ("$n starts autoassisting you.", FALSE, ch, 0, test,
		   TO_VICT);
	      act ("$n starts autoassisting $N.", FALSE, ch, 0, test,
		   TO_NOTVICT);
	      act ("Okay, from now on you will autoassist $N.", TRUE, ch, 0,
		   test, TO_CHAR);
	      AUTOASSIST (ch) = test;

	      /* Add to head of targets assisters_type struct */
	      CREATE (k, struct assisters_type, 1);
	      k->assister = ch;
	      k->next = test->autoassisters;
	      test->autoassisters = k;

	    }
	  else
	    {
	      if (ch == test)
		act ("Yeah, right.", FALSE, ch, 0, 0, TO_CHAR);
	      else
		act (NOPERSON, TRUE, ch, 0, 0, TO_CHAR);
	    }
	}
    }
}

/* The actual changing process.. Called by ACMD(do_change), below. */
void
perform_change (struct char_data *ch)
{
  ACMD (do_gen_comm);
  struct affected_type af;
  float wfactor = 0, vfactor = 0;

  af.type = SPELL_CHANGED;
  af.duration = 12;
  af.bitvector = 0;
  af.modifier = 0;
  af.location = APPLY_NONE;

  /* change amount of benefits depending on moon */
  switch (weather_info.moon)
    {
    case MOON_1ST_QTR:
    case MOON_FINAL_QTR:
      wfactor = 1.25;
      vfactor = 1.125;
      break;
    case MOON_HALF:
    case MOON_2ND_HALF:
      wfactor = 1.5;
      vfactor = 1.25;
      break;
    case MOON_3RD_QTR:
    case MOON_2ND_3RD_QTR:
      wfactor = 1.75;
      vfactor = 1.375;
      break;
    default:
      wfactor = 2;
      vfactor = 1.5;
      break;
    }
  /* if wolf: go wolfy else is vamp: vampout */
  if (PRF_FLAGGED (ch, PRF_WOLF))
    {
      send_to_char
	("You scream in agony as the transformation takes place...\r\n", ch);
      act ("A bloody pentangle appears on $n's hand!", FALSE, ch, 0, 0,
	   TO_ROOM);
      act ("$n screams in pain as $e transforms into a werewolf!", FALSE, ch,
	   0, 0, TO_ROOM);
      GET_MOVE (ch) += 50;	/* make sure they can holler */
      do_gen_comm (ch, "HHHHHHOOOOOOWWWWWWLLLLLL!!!!!!", 0, SCMD_HOLLER);

      af.modifier = 2;
      af.location = APPLY_STR;
      affect_join (ch, &af, 0, 0, 0, 0);
      GET_HIT (ch) = (int) (wfactor * GET_HIT (ch));
      GET_MOVE (ch) = (int) (wfactor * GET_MOVE (ch));
      if (weather_info.moon == MOON_FULL)
	af.bitvector = AFF_INFRAVISION;
      return;
    }
  if (PRF_FLAGGED (ch, PRF_VAMPIRE))
    {
      send_to_char ("You smile as your fangs extend...\r\n", ch);
      act ("You catch a glipse of $n's fangs in the moonlight.", FALSE, ch, 0,
	   0, TO_ROOM);
      af.modifier = 2;
      af.location = APPLY_WIS;
      affect_join (ch, &af, 0, 0, 0, 0);
      GET_MANA (ch) = (int) (vfactor * GET_MANA (ch));
      GET_MOVE (ch) = (int) (vfactor * GET_MOVE (ch));
      if (weather_info.moon == MOON_FULL)
	af.bitvector = AFF_INFRAVISION;
    }
}

/* This is the command used to infect other players - Artus */
ACMD (do_bite)
{
  struct char_data *vict;
  int bite_type;
  bool violence_check (struct char_data *ch, struct char_data *vict,
		       int skillnum);

  if (!(ch) || IS_NPC (ch))
    return;

  if (!*argument)
    {
      send_to_char ("Bite who?\r\n", ch);
      return;
    }

  // Locate victim...
  one_argument (argument, arg);
  vict = generic_find_char (ch, arg, FIND_CHAR_ROOM);
  if (!(vict))
    {
      send_to_char ("They don't seem to be here.\r\n", ch);
      return;
    }
  // Can't infect mobs...
  if (IS_NPC (vict))
    {
      send_to_char ("Better not, don't know what you might catch!\r\n", ch);
      return;
    }
  // Must be infected...
  if (!PRF_FLAGGED (ch, PRF_WOLF | PRF_VAMPIRE))
    {
      send_to_char ("You're not even infected! Think yourself lucky!\r\n",
		    ch);
      return;
    }
  // Must be changed...
  if (!affected_by_spell (ch, SPELL_CHANGED))
    {
      send_to_char
	("In your human form, the best you can manage is a hicky.\r\n", ch);
      act ("$n bites your neck, resulting in a large hicky.", FALSE, ch, 0,
	   vict, TO_VICT);
      act ("$n bites $N on the neck, resulting in a lovely hicky.", FALSE, ch,
	   0, vict, TO_NOTVICT);
      return;
    }
  // Level range check...
  if (GET_LEVEL (ch) + 10 < GET_LEVEL (vict))
    {
      send_to_char ("Do you have a death wish or something?\r\n", ch);
      return;
    }
  if (GET_LEVEL (ch) - 10 > GET_LEVEL (vict))
    {
      send_to_char ("Why don't you pick on someone your own size?\r\n", ch);
      return;
    }
  // Already infected?...
  bite_type = (PRF_FLAGGED (ch, PRF_WOLF)) ? PRF_WOLF : PRF_VAMPIRE;
  if (PRF_FLAGGED (vict, bite_type))
    {
      sprintf (buf, "But %s's already infected!\r\n", HSSH (vict));
      send_to_char (buf, ch);
      return;
    }
  else if (PRF_FLAGGED (vict, PRF_WOLF | PRF_VAMPIRE))
    {
      // Artus> Multiple infections == Bad News (tm).
      if (!violence_check (ch, vict, SPELL_CHANGED))
	return;
      switch (number (1, 6))
	{
	case 1:
	  act ("You sink your teeth into $N's neck.", FALSE, ch, 0, vict,
	       TO_CHAR);
	  act
	    ("$n sinks $s teeth into your neck, attempting to infect you again!",
	     FALSE, ch, 0, vict, TO_VICT);
	  act
	    ("$N screams in pain, $S blood boils, as $n bites $M in the neck.",
	     FALSE, ch, 0, vict, TO_NOTVICT);
	  damage (ch, vict, GET_HIT (vict) - 1, TYPE_UNDEFINED, FALSE);
	  GET_HIT (vict) = 1;
	  return;
	case 2:
	  act
	    ("As you bite $N, your blood tries to contain two infections.\r\nYou feel your death approaching.",
	     FALSE, ch, 0, vict, TO_CHAR);
	  act ("$n screams in pain as $e sinks $s teeth into your neck.",
	       FALSE, ch, 0, vict, TO_VICT);
	  act
	    ("$n screams in pain, $s blood boils, as $e bites $N in the neck.",
	     FALSE, ch, 0, vict, TO_NOTVICT);
	  // This should avoid pkill fuckups.
	  damage (ch, vict, 0, TYPE_UNDEFINED, FALSE);
	  damage (vict, ch, GET_HIT (ch) - 1, TYPE_UNDEFINED, FALSE);
	  return;
	default:
	  send_to_char ("You missed. How pathetic!\r\n", ch);
	  sprintf (buf, "&7%s&n tries miserably to bite you, and misses.\r\n",
		   GET_NAME (ch));
	  send_to_char (buf, vict);
	  act ("$n makes a pathetic attempt to bite $N, and misses.",
	       FALSE, ch, 0, vict, TO_NOTVICT);
	  damage (ch, vict, 0, TYPE_UNDEFINED, FALSE);
	  return;
	}
    }
  // Finally...
  if (number (1, 4) == 1)
    {				// Success... Drink.
      sprintf (buf, "You sink your teeth into into %s neck, and drink!\r\n",
	       HSHR (vict));
      send_to_char (buf, ch);
      act
	("$n sinks $s teeth into your neck. Everything suddenly goes dark.\r\n",
	 FALSE, ch, 0, vict, TO_VICT);
      act ("$n sinks $s teeth into $N's neck, and drinks heartily.\r\n",
	   FALSE, ch, 0, vict, TO_NOTVICT);
      GET_HIT (vict) >>= 1;
      SET_BIT (PRF_FLAGS (vict), bite_type);
      GET_POS (vict) = POS_STUNNED;
      GET_WAIT_STATE (ch) = GET_WAIT_STATE (vict) = PULSE_VIOLENCE * 5;
      GET_HIT (ch) += GET_HIT (vict);
      return;
    }
  act ("$N sees you going for $S neck, and knocks you to the ground!\r\n",
       FALSE, ch, 0, vict, TO_CHAR);
  act
    ("You see $n attempt to bite your neck, and throw $m to the ground!\r\n",
     FALSE, ch, 0, vict, TO_VICT);
  act ("$N skillfully throws $n to the ground, avoiding $s bite attempt!\r\n",
       FALSE, ch, 0, vict, TO_NOTVICT);
  GET_POS (ch) = POS_STUNNED;
  GET_WAIT_STATE (ch) = PULSE_VIOLENCE * 2;
}

/* this is the command used to transform into a wolf/vamp - Vader */
ACMD (do_change)
{

  if (ch == NULL)
    return;

  if (affected_by_spell (ch, SPELL_CHANGED))
    {
      send_to_char ("You're already changed!\r\n", ch);
      return;
    }

  /* are we infected? */
  if (!(PRF_FLAGGED (ch, PRF_WOLF | PRF_VAMPIRE)))
    {
      send_to_char ("You're not even infected! Think yourself lucky!\r\n",
		    ch);
      return;
    }
  /* is it dark? */
  if (weather_info.sunlight != SUN_DARK)
    {
      send_to_char
	("You can't draw any power from the moon during the day.\r\n", ch);
      return;
    }
  /* is there a moon? */
  if (weather_info.moon == MOON_NONE)
    {
      send_to_char ("There is no moon to draw power from tonight.\r\n", ch);
      return;
    }
  perform_change (ch);
}

ACMD (do_friend)
{
  int found = FALSE;
  int i, idnum = 0;

  two_arguments (argument, arg, buf2);

  // No argument - display the list of people ch is friending 
  if (!*arg)
    {
      sprintf (buf, "&1Your current friends are:\r\n"
	       "&1-------------------------&n\r\n");

      for (i = 0; i < MAX_FRIENDS; i++)
	{

	  // get_name_by_id returns NULL if id was not found in player_table
	  if ((get_name_by_id (GET_FRIEND (ch, i))))
	    {
	      sprintf (buf1, "&7%s&n\r\n",
		       get_name_by_id (GET_FRIEND (ch, i)));
	      strncat (buf, buf1, strlen (buf1));
	      found = TRUE;

	      // invalid entry - set to 0 
	    }
	  else
	    {
	      GET_FRIEND (ch, i) = 0;
	    }
	}

      if (!found)
	{
	  sprintf (buf2, "No-one.\r\n");
	  strncat (buf, buf2, strlen (buf2));
	}
      page_string (ch->desc, buf, TRUE);
      return;
    }

  idnum = get_id_by_name (arg);

  if (idnum > 0)
    {

      // see if we are already friending them
      for (i = 0; i < MAX_FRIENDS; i++)
	{
	  if (GET_FRIEND (ch, i) == idnum)
	    {
	      sprintf (buf, "&7%s&n is no longer considered a friend.\r\n",
		       get_name_by_id (idnum));
	      send_to_char (buf, ch);

	      GET_FRIEND (ch, i) = 0;
	      return;
	    }
	}

      // find first available position
      for (i = 0; i < MAX_FRIENDS; i++)
	{
	  if (!(get_name_by_id (GET_FRIEND (ch, i))))
	    {

	      // ignore self?
	      if (idnum == GET_IDNUM (ch))
		{
		  send_to_char ("Well isn't that sweet.\r\n", ch);
		  return;
		}

	      // ok, friend them
	      GET_FRIEND (ch, i) = idnum;
	      sprintf (buf, "&7%s&n is now your friend.\r\n",
		       get_name_by_id (idnum));
	      send_to_char (buf, ch);
	      return;
	    }
	}

      // no ffriend space available
      sprintf (buf,
	       "Anymore friend's and you'll win a popularity contest.\r\n");
      send_to_char (buf, ch);

      // could not find player
    }
  else
    {
      send_to_char ("No such player.\r\n", ch);
      return;
    }
}

ACMD (do_ignore)
{
  struct char_data *ignoreVict;
  struct char_file_u tmp_store;

  char *allComm = "All Communication", *tellOnly = "Tells Only";
  int found = FALSE;
  int i, idnum = 0, ignoreAll = FALSE, ignoreLvl;

  two_arguments (argument, arg, buf2);

  // No argument - display the list of people ch is ignoring
  if (!*arg)
    {
      sprintf (buf,
	       "&1You are currently ignoring:\r\n&1---------------------------&n\r\n");

      for (i = 0; i < MAX_IGNORE; i++)
	if ((get_name_by_id (GET_IGNORE (ch, i))))
	  {			// get_name_by_id returns NULL if id was not found in player_table
	    sprintf (buf1, "&7%s &B(&r%s&B)&n\r\n",
		     get_name_by_id (GET_IGNORE (ch, i)), GET_IGNORE_ALL (ch,
									  i) ?
		     allComm : tellOnly);
	    strncat (buf, buf1, strlen (buf1));
	    found = TRUE;
	  }
	else
	  GET_IGNORE (ch, i) = 0;

      // ignore level string
      sprintf (buf1,
	       "\r\nYou are ignoring everyone level %d and below &B(&r%s&B)&n.\r\n",
	       GET_IGN_LVL (ch), GET_IGN_LVL_ALL (ch) ? allComm : tellOnly);

      if (!found)
	{
	  sprintf (buf2, "No-one.\r\n");
	  strncat (buf, buf2, strlen (buf2));
	}
      if (GET_IGN_LVL (ch) > 0)
	strncat (buf, buf1, strlen (buf1));
      page_string (ch->desc, buf, TRUE);
      return;
    }

  // First argument is all
  if (!str_cmp ("all", arg))
    {
      ignoreAll = TRUE;
      strncpy (arg, buf2, strlen (buf2));
    }

  // Argument is a number
  if (isdigit (*arg))
    {
      ignoreLvl = atoi (arg);
      if (ignoreLvl)
	{
	  // check for level > self
	  if (GET_LEVEL (ch) <= ignoreLvl)
	    {
	      send_to_char ("You can only ignore levels below you.\r\n", ch);
	      return;
	    }
	  GET_IGN_LVL (ch) = ignoreLvl;
	  GET_IGN_LVL_ALL (ch) = ignoreAll;
	  sprintf (buf,
		   "You are now ignoring everyone level &r%d&n and below &B(&r%s&B)&n.\r\n",
		   ignoreLvl, (ignoreAll) ? allComm : tellOnly);
	  send_to_char (buf, ch);
	  return;
	  // new ignore level is 0 - remove old
	}
      if (GET_IGN_LVL (ch))
	{
	  sprintf (buf,
		   "You stop ignoring everyone level &r%d&n and below &B(&r%s&B)&n.\r\n",
		   GET_IGN_LVL (ch), (ignoreAll) ? allComm : tellOnly);
	  GET_IGN_LVL (ch) = 0;
	  send_to_char (buf, ch);
	  return;
	}
    }

  idnum = get_id_by_name (arg);

  if (idnum <= 0)
    {
      send_to_char ("No such player.\r\n", ch);
      return;
    }
  // see if we are alreadying ignoring them
  for (i = 0; i < MAX_IGNORE; i++)
    {
      if (GET_IGNORE (ch, i) == idnum)
	{
	  // toggle ignore all
	  if (ignoreAll)
	    {
	      GET_IGNORE_ALL (ch, i) = !GET_IGNORE_ALL (ch, i);
	      if ((get_name_by_id (idnum)))
		{
		  sprintf (buf, "You are now ignoring &7%s &B(&r%s&B)&n.\r\n",
			   get_name_by_id (idnum),
			   GET_IGNORE_ALL (ch, i) ? allComm : tellOnly);
		}
	      send_to_char (buf, ch);
	      return;
	      // remove ignore
	    }
	  else
	    {
	      sprintf (buf, "You stop ignoring &7%s&n.\r\n",
		       get_name_by_id (idnum));
	      send_to_char (buf, ch);
	      GET_IGNORE (ch, i) = 0;
	      GET_IGNORE_ALL (ch, i) = TRUE;
	      return;
	    }
	}
    }
  // find first available position
  for (i = 0; i < MAX_IGNORE; i++)
    {
      if (!(get_name_by_id (GET_IGNORE (ch, i))))
	{
	  // ignore self?
	  if (idnum == GET_IDNUM (ch))
	    {
	      send_to_char ("Are you that annoying?\r\n", ch);
	      return;
	    }
	  // check player level
	  CREATE (ignoreVict, struct char_data, 1);
	  clear_char (ignoreVict);
	  if (load_char (get_name_by_id (idnum), &tmp_store))
	    {			// this load fails on player with id 1 for some reason
	      store_to_char (&tmp_store, ignoreVict);
	      if ((GET_LEVEL (ignoreVict) >= LVL_IS_GOD) &&
		  (GET_LEVEL (ignoreVict) > GET_LEVEL (ch)))
		{
		  send_to_char
		    ("I don't think they would be too happy about that.\r\n",
		     ch);
		  free (ignoreVict);
		  return;
		}
	    }
	  free (ignoreVict);
	  // ok, ignore them
	  GET_IGNORE (ch, i) = idnum;
	  sprintf (buf, "You are now ignoring &7%s &B(&r%s&B)&n.\r\n",
		   get_name_by_id (idnum), allComm);
	  send_to_char (buf, ch);
	  return;
	}
    }
  // no ignore space available
  sprintf (buf, "You are ignoring the maximum about of players (%d).\r\n",
	   MAX_IGNORE);
  send_to_char (buf, ch);
}

/* command to join 2 objs together - Vader */
ACMD (do_join)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *part1, *part2, *joined;
  struct char_data *dummy;
  obj_vnum jointo = -1;

  two_arguments (argument, arg1, arg2);

  if (!*arg1 || !*arg2)
    {
      send_to_char ("Join what??\r\n", ch);
      return;
    }

  generic_find (arg1, FIND_OBJ_INV, ch, &dummy, &part1);
  if (!part1)
    {
      sprintf (buf, "You don't seem to have a %s.\r\n", arg1);
      send_to_char (buf, ch);
      return;
    }
  generic_find (arg2, FIND_OBJ_INV, ch, &dummy, &part2);
  if (!part2)
    {
      sprintf (buf, "You don't seem to have a %s.\r\n", arg2);
      send_to_char (buf, ch);
      return;
    }
  if (part1 == part2)
    {
      send_to_char ("You can't join that with itself!\r\n", ch);
      return;
    }

  if ((GET_OBJ_TYPE (part1) == ITEM_JOINABLE) &&
      (GET_OBJ_VAL (part1, 0) == GET_OBJ_VNUM (part2)))
    jointo = GET_OBJ_VAL (part1, 3);
  else if ((GET_OBJ_TYPE (part2) == ITEM_JOINABLE) &&
	   (GET_OBJ_VAL (part2, 0) == GET_OBJ_VNUM (part1)))
    jointo = GET_OBJ_VAL (part2, 3);
  else
    {
      sprintf (buf, "&5%s&n and &5%s&n don't seem to fit together.\r\n",
	       part1->short_description, part2->short_description);
      send_to_char (buf, ch);
      return;
    }
  joined = read_object (jointo, VIRTUAL);

/* Artus> Replaced:
  if(GET_OBJ_TYPE(part1) != ITEM_JOINABLE &&
     GET_OBJ_TYPE(part2) != ITEM_JOINABLE)
  {
    sprintf(buf,"%s and %s don't seem to fit together.\r\n",part1->short_description, part2->short_description);
    CAP(buf);
    send_to_char(buf,ch);
    return;
  }
  if((GET_OBJ_VAL(part1,0) != GET_OBJ_VNUM(part2)) &&
     (GET_OBJ_VAL(part2,0) != GET_OBJ_VNUM(part1)))
  {
    sprintf(buf,"%s and %s don't seem to fit together.\r\n",part1->short_description, part2->short_description);
    CAP(buf);  
    send_to_char(buf,ch);
    return;
  }
 
  joined = read_object(GET_OBJ_VAL(part1,3),VIRTUAL);
*/
  // Error check - DM
  if (!joined)
    {
      send_to_char ("Hmm, something looks broken, please report it.\r\n", ch);
      sprintf (buf, "SYSERR: Joining objs vnums %d and %d to make %d",
	       (part1) ? GET_OBJ_VNUM (part1) : -1,
	       (part2) ? GET_OBJ_VNUM (part2) : -1,
	       (joined) ? GET_OBJ_VNUM (joined) : -1);
      mudlog (buf, NRM, LVL_GOD, TRUE);
      return;
    }

  sprintf (buf, "%s and %s fit together perfectly to make %s.\r\n",
	   part1->short_description, part2->short_description,
	   joined->short_description);
  CAP (buf);
  send_to_char (buf, ch);
  sprintf (buf, "%s joins %s and %s together to make %s.\r\n", GET_NAME (ch),
	   part1->short_description, part2->short_description,
	   joined->short_description);
  CAP (buf);
  act (buf, FALSE, ch, 0, 0, TO_ROOM);

  obj_to_char (joined, ch, __FILE__, __LINE__);
  extract_obj (part1);
  extract_obj (part2);
}


/* this command is for the tag game - Vader */
ACMD (do_tag)
{
  struct char_data *vict;

  one_argument (argument, arg);

  if (!*arg)
    send_to_char ("Tag who?\r\n", ch);
  else if (!(vict = generic_find_char (ch, arg, FIND_CHAR_ROOM)))
    send_to_char ("They don't seem to be here...\r\n", ch);
  else if (IS_NPC (vict))
    send_to_char ("Errrrr I don't think Mobs play tag...\r\n", ch);
  else if (!PRF_FLAGGED (ch, PRF_TAG))
    send_to_char ("You're not even playing tag!\r\n", ch);
  else if (!PRF_FLAGGED (vict, PRF_TAG))
    send_to_char ("They arn't even playing tag!\r\n", ch);
  else
    {
      if (vict == ch)
	{
	  send_to_char ("You slap yourself and yell, 'You're it!'\r\n\r\n",
			ch);
	  act ("$n slap $mself and yells, 'You're it!!'", FALSE, ch, 0, 0,
	       TO_ROOM);
	  sprintf (buf, "%s has tagged themself out of the game.",
		   GET_NAME (ch));
	}
      else
	{
	  act ("You tag $N! Well done!", FALSE, ch, 0, vict, TO_CHAR);
	  act ("You have been tagged by $n!\r\n", FALSE, ch, 0, vict,
	       TO_VICT);
	  act ("$n has tagged $N!", FALSE, ch, 0, vict, TO_NOTVICT);
	  sprintf (buf, "%s has been tagged by %s", GET_NAME (vict),
		   GET_NAME (ch));
	}
      mudlog (buf, BRF, LVL_GOD, FALSE);
      REMOVE_BIT (PRF_FLAGS (vict), PRF_TAG);
      call_magic (ch, vict, NULL, SPELL_WORD_OF_RECALL, GET_LEVEL (ch),
		  CAST_MAGIC_OBJ);
    }
}

ACMD (do_mortal_kombat)
{
  struct descriptor_data *pt;
  int arena_room;
  char mybuf[256];

  if (FIGHTING (ch))
    {
      send_to_char (CHARFIGHTING, ch);
      return;
    }
  if (zone_table[world[IN_ROOM (ch)].zone].number != 11)
    {
      send_to_char
	("You can only enter the Mortal Kombat arena from the city of Haven!\r\n",
	 ch);
      return;
    }
  if (PRF_FLAGGED (ch, PRF_MORTALK))
    {
      if ((world[IN_ROOM (ch)].number < 4557) ||
	  (world[IN_ROOM (ch)].number > 4560))
	{
	  send_to_char ("You're already in the Mortal Kombat Arena!\r\n", ch);
	  return;
	}
      sprintf (buf, "SYSERR: %s had MORTALK flag outside arena!",
	       GET_NAME (ch));
      mudlog (buf, NRM, LVL_IMPL, TRUE);
      REMOVE_BIT (PRF_FLAGS (ch), PRF_MORTALK);
    }
  strcpy (mybuf, "");
  sprintf (mybuf,
	   "\r\n[NOTE] &7%s&n has entered the Mortal Kombat Arena!!\r\n",
	   GET_NAME (ch));
  if (GET_GOLD (ch) < 10000)
    {
      send_to_char
	("You will need 10,000 gold to enter the Mortal Kombat Arena!", ch);
      return;
    }
  arena_room = number (4557, 4560);
  GET_GOLD (ch) -= 10000;
  char_from_room (ch);
  char_to_room (ch, real_room (arena_room));
  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && pt->character != ch)
      send_to_char (mybuf, pt->character);
  if (PRF_FLAGGED (ch, PRF_NOREPEAT))
    send_to_char (OK, ch);
  else
    send_to_char (mybuf, ch);
  SET_BIT (PRF_FLAGS (ch), PRF_MORTALK);
  look_at_room (ch, 0);
  strcpy (buf, "");
  send_to_char
    ("\r\nWelcome to the Mortal Kombat ARENA.  Prepare for Combat!", ch);
}

/* command displays the mud local time - Hal */
ACMD (do_realtime)
{
  char *buf;
#ifndef NO_LOCALTIME
  time_t ct;
  ct = time (0);
  buf = asctime (localtime (&ct));
#else
  struct tm lt;
  if (jk_localtime_now (&lt))
    {
      basic_mud_log ("Error in jk_localtime_now() [%s:%d]", __FILE__,
		     __LINE__);
      return;
    }
  buf = asctime (&lt);
#endif
  send_to_char (buf, ch);
}

#if 0
// Artus> Moved to SPECIAL(arrow_room);
/* proc to do the damage for Ruprect's indian attacks - Vader */
void
do_arrows ()
{
  struct char_data *vict;
  int t, i;
  int dam;
  char limbs[5][10] = {
    "hand",
    "arm",
    "thigh",
    "shoulder",
    "back"
  };
  int rooms[72] = {
    13600, 13605, 13615, 13626, 13640, 13650, 13664, 13706, 13716, 13725,
    13732, 13740, 13748, 13749, 13796, 13797, 13798, 13693, 13694, 13695,
    13696, 13697, 13698, 13699, 13700, 13701, 13674, 13675, 13676, 13677,
    13678, 13679, 13680, 13734, 13735, 13736, 13737, 13738, 13741, 13742,
    13743, 13744, 13745, 13750, 13751, 13752, 13753, 13754, 13755, 13759,
    13760, 13761, 13762, 13764, 13765, 13771, 13772, 13773, 13774, 13775,
    13776, 13777, 13778, 13779, 13780, 13781, 13782, 13783, 13784, 13785,
    13786, 13787
  };

  for (i = 0; i < 72; i++)
    for (vict = world[real_room (rooms[i])].people; vict;
	 vict = vict->next_in_room)
      {
	if (IS_NPC (vict))	//Lets not bother harming NPCs.. Artus.
	  continue;
	if (!(number (0, 8)) && LR_FAIL (vict, LVL_IS_GOD))
	  t = number (0, 4);
	sprintf (buf, "$n is hit in the %s by an arrow!", limbs[t]);
	act (buf, FALSE, vict, 0, 0, TO_ROOM);
	sprintf (buf,
		 "You are hit in the %s by an arrow! It feels really pointy...\r\n",
		 limbs[t]);
	send_to_char (buf, vict);
	dam = dice ((t + 1) * 2, GET_LEVEL (vict));
	GET_HIT (vict) -= dam;
	update_pos (vict);
	if (GET_POS (vict) == POS_DEAD)
	  {
	    sprintf (buf, "%s killed by an arrow at %s", GET_NAME (vict),
		     world[vict->in_room].name);
//       if(!IS_NPC(vict))
//       {
	    mudlog (buf, NRM, LVL_GOD, 0);
	    act
	      ("$n falls to the ground. As dead as something that isn't alive.",
	       FALSE, vict, 0, 0, TO_ROOM);
	    send_to_char
	      ("The arrow seems to have done permenant damage. You're dead.\r\n",
	       vict);
	    die (vict, NULL, "arrow");
	    //       }
	  }
      }
}
#endif

ACMD (do_first_aid)
{
  struct timer_type *tim;
  struct char_data *vict;
  int prob;

  one_argument (argument, arg);

  prob = GET_SKILL (ch, SKILL_FIRST_AID);

  if (!prob)
    {
      send_to_char (UNFAMILIARSKILL, ch);
      return;
    }

  if (!has_stats_for_skill (ch, SKILL_FIRST_AID, TRUE))
    return;

  if (!*arg)
    vict = ch;
  else
    {
      if (!(vict = generic_find_char (ch, arg, FIND_CHAR_ROOM)))
	{
	  send_to_char (NOPERSON, ch);
	  return;
	}
    }

  if (!(char_affected_by_timer (ch, subcmd)))
    {
      tim = timer_new (subcmd);
      timer_to_char (ch, tim);
    }

  if (timer_use_char (ch, subcmd))
    {
      if (ch == vict)
	{
	  act ("You apply your first aid knowledge to yourself.",
	       FALSE, ch, 0, vict, TO_CHAR);
	  act ("$n applies first aid to $mself.",
	       TRUE, ch, 0, vict, TO_NOTVICT);
	}
      else
	{
	  act ("You apply your first aid knowledge to $N.", FALSE, ch, 0,
	       vict, TO_CHAR);
	  act ("$n applies first aid to you.", FALSE, ch, 0, vict, TO_VICT);
	  act ("$n applies first aid to $N.", TRUE, ch, 0, vict, TO_NOTVICT);
	}
      if (GET_HIT (vict) < GET_MAX_HIT (vict))
	GET_HIT (vict) =
	  MIN (GET_MAX_HIT (vict),
	       GET_HIT (vict) + MIN (50, GET_LEVEL (ch) * 2));
    }
  else
    {
      send_to_char (RESTSKILL, ch);
      return;
    }
}

ACMD (do_attend_wounds)
{
  struct timer_type *tim;
  struct char_data *vict;
  int prob;

  one_argument (argument, arg);

  prob = GET_SKILL (ch, SKILL_ATTEND_WOUNDS);

  if (!prob)
    {
      send_to_char (UNFAMILIARSKILL, ch);
      return;
    }

  if (!has_stats_for_skill (ch, SKILL_ATTEND_WOUNDS, TRUE))
    return;

  if (!*arg)
    vict = ch;
  else
    {
      if (!(vict = generic_find_char (ch, arg, FIND_CHAR_ROOM)))
	{
	  send_to_char (NOPERSON, ch);
	  return;
	}
    }

  if (!(char_affected_by_timer (ch, subcmd)))
    {
      tim = timer_new (subcmd);
      timer_to_char (ch, tim);
    }

  if (timer_use_char (ch, subcmd))
    {
      if (ch == vict)
	{
	  act ("You attend to your wounds.", FALSE, ch, 0, vict, TO_CHAR);
	  act ("$n attends to $s wounds.", TRUE, ch, 0, vict, TO_NOTVICT);
	}
      else
	{
	  act ("You attend to $N's wounds.", FALSE, ch, 0, vict, TO_CHAR);
	  act ("$n attends to your wounds.", FALSE, ch, 0, vict, TO_VICT);
	  act ("$n attends to $N's wounds.", TRUE, ch, 0, vict, TO_NOTVICT);
	}
      GET_HIT (vict) = MIN (GET_MAX_HIT (vict),
			    GET_HIT (vict) + MIN (250, GET_LEVEL (ch) * 3));
    }
  else
    {
      send_to_char (RESTSKILL, ch);
      return;
    }
}

ACMD (do_clot_wounds)
{
  struct timer_type *tim;
  struct char_data *vict;
  int prob;

  one_argument (argument, arg);

  prob = GET_SKILL (ch, SKILL_CLOT_WOUNDS);

  if (!prob)
    {
      send_to_char (UNFAMILIARSKILL, ch);
      return;
    }

  if (!has_stats_for_skill (ch, SKILL_CLOT_WOUNDS, TRUE))
    return;

  if (!*arg)
    vict = ch;
  else
    {
      if (!(vict = generic_find_char (ch, arg, FIND_CHAR_ROOM)))
	{
	  send_to_char (NOPERSON, ch);
	  return;
	}
    }

  if (!(char_affected_by_timer (ch, subcmd)))
    {
      tim = timer_new (subcmd);
      timer_to_char (ch, tim);
    }

  if (timer_use_char (ch, subcmd))
    {
      if (ch == vict)
	{
	  act ("You focus mentally on clotting up your wounds.", FALSE, ch, 0,
	       vict, TO_CHAR);
	  act ("$n clots up $s wounds.", TRUE, ch, 0, vict, TO_NOTVICT);
	}
      else
	{
	  act ("You mentally focus on $n and clot $s wounds.", FALSE, vict, 0,
	       vict, TO_CHAR);
	  act ("$n mentally focuses on you and clots up your wounds.", FALSE,
	       ch, 0, vict, TO_VICT);
	  act ("$n clots up $N's wounds.", TRUE, ch, 0, vict, TO_NOTVICT);
	}
      if (GET_HIT (vict) < GET_MAX_HIT (vict))
	GET_HIT (vict) =
	  MIN (GET_MAX_HIT (vict),
	       GET_HIT (vict) + MIN (400, GET_LEVEL (ch) * 5));
    }
  else
    {
      send_to_char (RESTSKILL, ch);
      return;
    }
}

ACMD (do_adrenaline)
{
  struct affected_type af;
  int prob;

  if (affected_by_spell (ch, SKILL_ADRENALINE))
    {
      send_to_char ("You're already adrenalised.\r\n", ch);
      return;
    }

  prob = GET_SKILL (ch, SKILL_ADRENALINE);
  /* // Artus> has_stats_for_skill will handle this...
     if (!prob) {
     send_to_char(UNFAMILIARSKILL,ch);
     return;
     } */

  if (!has_stats_for_skill (ch, SKILL_ADRENALINE, TRUE))
    return;

  af.type = SKILL_ADRENALINE;
  af.duration = 2;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_ADRENALINE;

  affect_join (ch, &af, FALSE, FALSE, FALSE, FALSE);
  act ("Your feel your adrenaline flow.", FALSE, ch, 0, ch, TO_CHAR);
  act ("$n is pumped by adrenaline.", TRUE, ch, 0, ch, TO_NOTVICT);
}

ACMD (do_purse)
{
  struct char_data *vict = NULL;
  char vict_name[MAX_INPUT_LENGTH];
  int percent;

  if (IS_NPC (ch))
    {
      send_to_char ("What do you care, little minion?\r\n", ch);
      return;
    }

  if (!GET_SKILL (ch, SKILL_PURSE))
    {
      send_to_char ("You have no idea how to do that.\r\n", ch);
      return;
    }

  if (FIGHTING (ch))
    {
      send_to_char ("You're fighting! How do you expect to do that?\r\n", ch);
      return;
    }

  one_argument (argument, vict_name);

  if (!*vict_name)
    {
      send_to_char ("Who's purse do you wish to evaluate?\r\n", ch);
      return;
    }
  else if (!(vict = generic_find_char (ch, vict_name, FIND_CHAR_ROOM)))
    {
      send_to_char (NOPERSON, ch);
      return;
    }

  if ((!IS_NPC (vict) && !LR_FAIL (vict, LVL_IS_GOD)))
    {
      send_to_char ("You must be as stupid as you look!\r\n", ch);
      return;
    }
  percent = number (1, 101);	// 101 is a failure

  if (LR_FAIL (ch, LVL_IMMORT))
    {
      if (GET_LEVEL (vict) > GET_LEVEL (ch))
	percent += (GET_LEVEL (vict) - GET_LEVEL (ch));
      else if (GET_LEVEL (vict) < GET_LEVEL (ch))
	percent -= (GET_LEVEL (ch) - GET_LEVEL (vict));
    }

  if (percent > GET_SKILL (ch, SKILL_PURSE))
    {
      send_to_char
	("You couldn't see how much they had in their purse...\r\n", ch);
      return;
    }

  if (GET_GOLD (vict) == 0)
    send_to_char ("They don't have ANY gold!\r\n", ch);
  else if (GET_GOLD (vict) < 100)
    send_to_char ("They don't have very much gold!\r\n", ch);
  else if (GET_GOLD (vict) >= 100 && GET_GOLD (vict) < 1000)
    send_to_char ("They have hundreds of gold.\r\n", ch);
  else if (GET_GOLD (vict) >= 1000 && GET_GOLD (vict) < 10000)
    send_to_char ("They have thousands of gold.\r\n", ch);
  else if (GET_GOLD (vict) >= 10000 && GET_GOLD (vict) < 100000)
    send_to_char ("They have 10's of thousands of gold!\r\n", ch);
  else if (GET_GOLD (vict) >= 100000 && GET_GOLD (vict) < 1000000)
    send_to_char ("They have 100's of THOUSANDS of gold!\r\n", ch);
  else if (GET_GOLD (vict) >= 1000000 && GET_GOLD (vict) < 10000000)
    send_to_char ("They have MILLIONS of GOLD!!!\r\n", ch);
  else if (GET_GOLD (vict) >= 10000000 && GET_GOLD (vict) < 100000000)
    send_to_char ("They have 10's of MILLIONS of GOLD!!!!!\r\n", ch);
  else
    send_to_char ("They have too much that you can't count it all!!!\r\n",
		  ch);
}


ACMD (do_charge)
{
  /* A "battery" stores anything of an integer value */
  /* Value 0: the battery type */
  /* Value 1: max size */
  /* Value 2: current size */
  /* Value 3: charge ratio */

  struct obj_data *battery, *dummy;
  struct char_data *vict;
  int charge_type = 0, charge_max = 0, charge_current = 0, charge_ratio = 0;
  int charge_from = 0, charge_to = 0, charge_remaining = 0;
  char *charge_names[] = { "Mana" };
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments (argument, arg1, arg2);

  if (!*arg1)
    {
      char *usage = "&1Usage:&4 charge <item [amount]> [ target ]&n\r\n";
      send_to_char (usage, ch);
      return;
    }

  /* Find the battery */
  generic_find (arg1, FIND_OBJ_INV, ch, &vict, &battery);

  if (!battery)
    {
      sprintf (buf, "You don't seem to have a %s.\r\n", arg1);
      send_to_char (buf, ch);
      return;
    }

  /* Now check the item is a battery */
  if (!(GET_OBJ_TYPE (battery) == ITEM_BATTERY))
    {
      send_to_char ("You can't charge that.\r\n", ch);
      return;
    }
  else
    {
      /* find out the battery details */
      charge_type = GET_OBJ_VAL (battery, 0);
      charge_max = GET_OBJ_VAL (battery, 1);
      charge_current = GET_OBJ_VAL (battery, 2);
      charge_ratio = GET_OBJ_VAL (battery, 3);

      charge_remaining = charge_max - charge_current;

      /* DISPLAY BATTERY INFO */
      if (!*arg2)
	{
	  sprintf (buf,
		   "&0Battery Information:\r\n&b--------------------\r\n&1Charge Stored:  &g%s&n\r\n&1Max Charge:     &c%d&n\r\n&1Current Charge: &c%d&n\r\n&1Charge Ratio:   &c%d&n:&c1&n\r\n",
		   charge_names[charge_type], charge_max, charge_current,
		   charge_ratio);
	  send_to_char (buf, ch);
	  return;
	}

      switch (charge_type)
	{
	case BATTERY_MANA:

	  /* ADD MANA TO OBJECT */
	  if (isdigit (*arg2))
	    {
	      charge_to = atoi (arg2);

	      /* Full Battery */
	      if (charge_remaining == 0)
		{
		  send_to_char ("This battery is full.\r\n", ch);
		  return;
		}

	      /* Adding 0 or a negative number */
	      if (charge_to <= 0)
		{
		  send_to_char ("Yep, your a comedian.\r\n", ch);
		  return;
		}

	      /* Adjust for (given charge > charge remaining) */
	      if (charge_to > charge_remaining)
		charge_to = charge_remaining;

	      charge_from = charge_ratio * charge_to;

	      if (GET_MANA (ch) < charge_from)
		{
		  charge_to = GET_MANA (ch) / charge_ratio;
		  charge_from = charge_ratio * charge_to;
		}

	      if (charge_from == 0)
		{
		  send_to_char
		    ("You haven't the energy to perform the charge.\r\n", ch);
		  return;
		}

	      GET_MANA (ch) -= charge_from;
	      GET_OBJ_VAL (battery, 2) += charge_to;
	      sprintf (buf,
		       "You give the battery a charge of %d at an expense of %d mana.\r\n",
		       charge_to, charge_from);
	      send_to_char (buf, ch);
	      //   send_to_char("You charge the battery.\r\n",ch);
/* OLD
           Check available mana
          if (charge_from > GET_MANA(ch)) {
 
            if ((GET_MANA(ch)/charge_ratio) > 0) {
              return;
            } else {
            }
          } else {
            GET_MANA(ch) -= charge_from;
          }
*/
	      /* EXTRACT MANA FROM OBJECT */
	    }
	  else
	    {
	      if (!str_cmp ("self", arg2))
		vict = ch;
	      else
		generic_find (arg2, FIND_CHAR_ROOM, ch, &vict, &dummy);

	      if (!vict)
		{
		  send_to_char ("If only they were here.\r\n", ch);
		  return;
		}

	      /* Flat Battery */
	      if (charge_current == 0)
		{
		  send_to_char ("This battery is flat.\r\n", ch);
		  return;
		}

	      /* Calculate victims mana */
	      charge_to = GET_MAX_MANA (vict) - GET_MANA (vict);

	      /* Victim has full mana */
	      if (GET_MANA (vict) == GET_MAX_MANA (vict))
		{
		  send_to_char ("Now that'd be a waste wouldn't it?\r\n", ch);
		  return;
		}

	      /* Use full battery */
	      if (charge_to > charge_current)
		charge_to = charge_current;

	      GET_MANA (vict) += charge_to;
	      GET_OBJ_VAL (battery, 2) -= charge_to;

	      if (ch == vict)
		{
		  sprintf (buf, "You extract %d mana from the battery.\r\n",
			   charge_to);
		  send_to_char (buf, ch);
		}
	      else
		{
		  sprintf (buf, "You extract %d mana to %s.\r\n", charge_to,
			   GET_NAME (vict));
		  send_to_char (buf, ch);
		}
	    }

	  break;

	default:
	  send_to_char ("This battery doesn't seem to be working.\r\n", ch);
	  sprintf (buf,
		   "%s using battery: %d, with undetermined battery type.\r\n",
		   GET_NAME (ch), GET_OBJ_VNUM (battery));
	  mudlog (buf, NRM, MAX (LVL_GOD, GET_INVIS_LEV (ch)), TRUE);
	  break;
	}
    }
}
