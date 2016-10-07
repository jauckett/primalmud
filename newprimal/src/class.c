/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"

extern int siteok_everyone;
extern sh_int world_start_room[NUM_WORLDS];

/* local functions */
int parse_class(char *arg);
long find_class_bitvector(char arg);
byte saving_throws(struct char_data *ch, int type);
int thaco(struct char_data *ch);
void roll_real_abils(struct char_data * ch);
void do_start(struct char_data * ch);
int backstab_mult(int level);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(struct char_data *ch, int level);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);

/* Names first */

const char *class_abbrevs[] = {
  "Ma",
  "Cl",
  "Th",
  "Wa",
  "Dr",
  "Pr",
  "Ni",
  "Bm", 
  "Ss",
  "Pa",
  "MA",
  "\n"
};

/* Moved to constants
const char *pc_class_types[] = {
  "Magician",
  "Cleric",
  "Thief",
  "Warrior",
  "Druid",
  "Priest",
  "Nightblade",
  "Battlemage",
  "Spellsword",
  "Paladin",
  "Master",
  "\n"
};
*/

/* Moved to constants
const char *pc_race_types[] = {
  "Ogre",
  "Deva",
  "Aruchai",
  "Elf",
  "Changling",
  "Human",
  "Orc",
  "Kobold",
  "Dwarf",
  "Gnome",
  "Kenda",
  "Pixie",
  "Avatar",
  "\n"
};
*/


const float class_modifiers[NUM_CLASSES] = {

	0.7, 		// Magician
	0.65,		// Cleric
	0.5, 		// Thief
	0.6,		// Warrior
	1.4,		// Druid
	1.2,		// Priest
	1.3,		// Nightblade
	1.4,		// Battlemage
	1.35, 		// Spellsword
        1.4, 		// Paladin
	1.75		// Master

};

const float race_modifiers[MAX_RACES] = {
  0.1,        // Ogre
  0.3,  // Deva
  0.25, // Minotaur
  0.2,  // Elf
  0.1,  // Changeling
  0.0,  // Human
  0.1,  // Orc
  0.2,  // Dwarf
  0.3,  // Kenda
  0.25, // Pixie
  5.0   // Avatar
};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"Select a class:\r\n"
"  [&gC&n]leric\r\n"
"  [&gT&n]hief\r\n"
"  [&gW&n]arrior\r\n"
"  [&gM&n]agician\r\n";

/* Race menu shown at character creation */
const char *race_menu =
"\r\n"
"Select a race:\r\n"
"  &gO&ngre\r\n"
"  &gD&neva\r\n"
"  &gM&ninotaur\r\n"
"  &gE&nlf\r\n"
"  &gC&nhangling\r\n"
"  &gH&numan\r\n"
"  o&gR&nc\r\n"
"  d&gW&narf\r\n"
"  &gK&nenda\r\n"
"  &gP&nixie\r\n";

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char *arg)
{
  char letter = LOWER(arg[0]), nextletter;
 
  switch (letter) {
  case 'm':
        if( strlen(arg) < 3)
           return CLASS_MAGIC_USER;
        switch(LOWER(arg[2])) {
                case 'g': return CLASS_MAGIC_USER;
                case 's': return CLASS_MASTER;
                default: return CLASS_UNDEFINED;
        }
  case 'c': return CLASS_CLERIC;
  case 'w': return CLASS_WARRIOR;
  case 't': return CLASS_THIEF;
  case 'n': return CLASS_NIGHTBLADE;
  case 'b': return CLASS_BATTLEMAGE; 
  case 's': return CLASS_SPELLSWORD;
  case 'd': return CLASS_DRUID;
  case 'p':
        if( strlen(arg) < 2)
           return CLASS_PRIEST;
        switch(LOWER(arg[1])) {
                case 'r': return CLASS_PRIEST;
                case 'a': return CLASS_PALADIN;
                default: return CLASS_UNDEFINED;
        }
  default:  return CLASS_UNDEFINED;
  }
}   

int parse_race_name(char *arg) {
  switch(LOWER(arg[0])) {
      case 'o': switch(LOWER(arg[1])) {
                      case 'r': return RACE_ORC;
                      case 'g': return RACE_OGRE;
                      default: return RACE_UNDEFINED;
                }
        break;
      case 'd': switch(LOWER(arg[1])) {
                      case 'w': return RACE_DWARF;
                      case 'e': return RACE_DEVA;
                      default: return RACE_UNDEFINED;
                }
        break;
      case 'm': return RACE_MINOTAUR;
      case 'e': return RACE_ELF;
      case 'c': return RACE_CHANGLING;
      case 'h': return RACE_HUMAN;
      case 'k': return RACE_KENDA;
      case 'p': return RACE_PIXIE;
      case 'a': return RACE_AVATAR;
      default: return RACE_UNDEFINED;
  }
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
    case 'm': return (1 << CLASS_MAGIC_USER);
    case 'c': return (1 << CLASS_CLERIC);
    case 't': return (1 << CLASS_THIEF);
    case 'w': return (1 << CLASS_WARRIOR);
    default:  return 0;
  }
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
  /* MAG	CLE	THE	WAR	DRU	PRI	NBL	BTM
	   SPS	    PAL	    MAS	 */
  {95,		95,	85,	80,     10,	10,	10,	10, 
	   10,      10,     10},		
  {100,		100,	12,	12,     10,     10, 	10,	10, 
	   10,      10,     10},		
  {25,		25,	0,	0,      0, 	0, 	10,	10, 
	   10,      10,     10},	
  {SPELL,	SPELL,	SKILL,	SKILL,  SPELL, SPELL_BOTH, SPELL_BOTH, SPELL_BOTH, 
           SPELL_BOTH,    SPELL_BOTH,   SPELL_BOTH}		
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
int guild_info[][3] = {

/* Midgaard */
  {CLASS_MAGIC_USER,	3017,	SCMD_SOUTH},
  {CLASS_CLERIC,	3004,	SCMD_NORTH},
  {CLASS_THIEF,		3027,	SCMD_EAST},
  {CLASS_WARRIOR,	3021,	SCMD_EAST},

/* Brass Dragon */
  {-999 /* all */ ,	5065,	SCMD_WEST},

/* this must go last -- add new guards above! */
  {-1, -1, -1}
};



/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */

byte saving_throws(struct char_data *ch, int type)
{
  int save, level=GET_LEVEL(ch), class_num=GET_CLASS(ch);
 
// DM - TODO - ADD SAVING THROWS 

  return 0;

  switch (class_num) {
  case CLASS_MAGIC_USER:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 66;
      case  6: return 65;
      case  7: return 63;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 57;
      case 12: return 55;
      case 13: return 54;
      case 14: return 53;
      case 15: return 53;
      case 16: return 52;
      case 17: return 51;
      case 18: return 50;
      case 19: return 48;
      case 20: return 46;
      case 21: return 45;
      case 22: return 44;
      case 23: return 42;
      case 24: return 40;
      case 25: return 38;
      case 26: return 36;
      case 27: return 34;
      case 28: return 32;
      case 29: return 30;
      case 30: return 28;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for mage paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 55;
      case  2: return 53;
      case  3: return 51;
      case  4: return 49;
      case  5: return 47;
      case  6: return 45;
      case  7: return 43;
      case  8: return 41;
      case  9: return 40;
      case 10: return 39;
      case 11: return 37;
      case 12: return 35;
      case 13: return 33;
      case 14: return 31;
      case 15: return 30;
      case 16: return 29;
      case 17: return 27;
      case 18: return 25;
      case 19: return 23;
      case 20: return 21;
      case 21: return 20;
      case 22: return 19;
      case 23: return 17;
      case 24: return 15;
      case 25: return 14;
      case 26: return 13;
      case 27: return 12;
      case 28: return 11;
      case 29: return 10;
      case 30: return  9;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for mage rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 65;
      case  2: return 63;
      case  3: return 61;
      case  4: return 59;
      case  5: return 57;
      case  6: return 55;
      case  7: return 53;
      case  8: return 51;
      case  9: return 50;
      case 10: return 49;
      case 11: return 47;
      case 12: return 45;
      case 13: return 43;
      case 14: return 41;
      case 15: return 40;
      case 16: return 39;
      case 17: return 37;
      case 18: return 35;
      case 19: return 33;
      case 20: return 31;
      case 21: return 30;
      case 22: return 29;
      case 23: return 27;
      case 24: return 25;
      case 25: return 23;
      case 26: return 21;
      case 27: return 19;
      case 28: return 17;
      case 29: return 15;
      case 30: return 13;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for mage petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 75;
      case  2: return 73;
      case  3: return 71;
      case  4: return 69;
      case  5: return 67;
      case  6: return 65;
      case  7: return 63;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 57;
      case 12: return 55;
      case 13: return 53;
      case 14: return 51;
      case 15: return 50;
      case 16: return 49;
      case 17: return 47;
      case 18: return 45;
      case 19: return 43;
      case 20: return 41;
      case 21: return 40;
      case 22: return 39;
      case 23: return 37;
      case 24: return 35;
      case 25: return 33;
      case 26: return 31;
      case 27: return 29;
      case 28: return 27;
      case 29: return 25;
      case 30: return 23;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for mage breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 60;
      case  2: return 58;
      case  3: return 56;
      case  4: return 54;
      case  5: return 52;
      case  6: return 50;
      case  7: return 48;
      case  8: return 46;
      case  9: return 45;
      case 10: return 44;
      case 11: return 42;
      case 12: return 40;
      case 13: return 38;
      case 14: return 36;
      case 15: return 35;
      case 16: return 34;
      case 17: return 32;
      case 18: return 30;
      case 19: return 28;
      case 20: return 26;
      case 21: return 25;
      case 22: return 24;
      case 23: return 22;
      case 24: return 20;
      case 25: return 18;
      case 26: return 16;
      case 27: return 14;
      case 28: return 12;
      case 29: return 10;
      case 30: return  8;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for mage spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
    break;
  case CLASS_CLERIC:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 60;
      case  2: return 59;
      case  3: return 48;
      case  4: return 46;
      case  5: return 45;
      case  6: return 43;
      case  7: return 40;
      case  8: return 37;
      case  9: return 35;
      case 10: return 34;
      case 11: return 33;
      case 12: return 31;
      case 13: return 30;
      case 14: return 29;
      case 15: return 27;
      case 16: return 26;
      case 17: return 25;
      case 18: return 24;
      case 19: return 23;
      case 20: return 22;
      case 21: return 21;
      case 22: return 20;
      case 23: return 18;
      case 24: return 15;
      case 25: return 14;
      case 26: return 12;
      case 27: return 10;
      case 28: return  9;
      case 29: return  8;
      case 30: return  7;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for cleric paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 66;
      case  5: return 65;
      case  6: return 63;
      case  7: return 60;
      case  8: return 57;
      case  9: return 55;
      case 10: return 54;
      case 11: return 53;
      case 12: return 51;
      case 13: return 50;
      case 14: return 49;
      case 15: return 47;
      case 16: return 46;
      case 17: return 45;
      case 18: return 44;
      case 19: return 43;
      case 20: return 42;
      case 21: return 41;
      case 22: return 40;
      case 23: return 38;
      case 24: return 35;
      case 25: return 34;
      case 26: return 32;
      case 27: return 30;
      case 28: return 29;
      case 29: return 28;
      case 30: return 27;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for cleric rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 65;
      case  2: return 64;
      case  3: return 63;
      case  4: return 61;
      case  5: return 60;
      case  6: return 58;
      case  7: return 55;
      case  8: return 53;
      case  9: return 50;
      case 10: return 49;
      case 11: return 48;
      case 12: return 46;
      case 13: return 45;
      case 14: return 44;
      case 15: return 43;
      case 16: return 41;
      case 17: return 40;
      case 18: return 39;
      case 19: return 38;
      case 20: return 37;
      case 21: return 36;
      case 22: return 35;
      case 23: return 33;
      case 24: return 31;
      case 25: return 29;
      case 26: return 27;
      case 27: return 25;
      case 28: return 24;
      case 29: return 23;
      case 30: return 22;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for cleric petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 80;
      case  2: return 79;
      case  3: return 78;
      case  4: return 76;
      case  5: return 75;
      case  6: return 73;
      case  7: return 70;
      case  8: return 67;
      case  9: return 65;
      case 10: return 64;
      case 11: return 63;
      case 12: return 61;
      case 13: return 60;
      case 14: return 59;
      case 15: return 57;
      case 16: return 56;
      case 17: return 55;
      case 18: return 54;
      case 19: return 53;
      case 20: return 52;
      case 21: return 51;
      case 22: return 50;
      case 23: return 48;
      case 24: return 45;
      case 25: return 44;
      case 26: return 42;
      case 27: return 40;
      case 28: return 39;
      case 29: return 38;
      case 30: return 37;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for cleric breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 75;
      case  2: return 74;
      case  3: return 73;
      case  4: return 71;
      case  5: return 70;
      case  6: return 68;
      case  7: return 65;
      case  8: return 63;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 56;
      case 13: return 55;
      case 14: return 54;
      case 15: return 53;
      case 16: return 51;
      case 17: return 50;
      case 18: return 49;
      case 19: return 48;
      case 20: return 47;
      case 21: return 46;
      case 22: return 45;
      case 23: return 43;
      case 24: return 41;
      case 25: return 39;
      case 26: return 37;
      case 27: return 35;
      case 28: return 34;
      case 29: return 33;
      case 30: return 32;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for cleric spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
    break;
  case CLASS_THIEF:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 65;
      case  2: return 64;
      case  3: return 63;
      case  4: return 62;
      case  5: return 61;
      case  6: return 60;
      case  7: return 59;
      case  8: return 58;
      case  9: return 57;
      case 10: return 56;
      case 11: return 55;
      case 12: return 54;
      case 13: return 53;
      case 14: return 52;
      case 15: return 51;
      case 16: return 50;
      case 17: return 49;
      case 18: return 48;
      case 19: return 47;
      case 20: return 46;
      case 21: return 45;
      case 22: return 44;
      case 23: return 43;
      case 24: return 42;
      case 25: return 41;
      case 26: return 40;
      case 27: return 39;
      case 28: return 38;
      case 29: return 37;
      case 30: return 36;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for thief paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 64;
      case  5: return 62;
      case  6: return 60;
      case  7: return 58;
      case  8: return 56;
      case  9: return 54;
      case 10: return 52;
      case 11: return 50;
      case 12: return 48;
      case 13: return 46;
      case 14: return 44;
      case 15: return 42;
      case 16: return 40;
      case 17: return 38;
      case 18: return 36;
      case 19: return 34;
      case 20: return 32;
      case 21: return 30;
      case 22: return 28;
      case 23: return 26;
      case 24: return 24;
      case 25: return 22;
      case 26: return 20;
      case 27: return 18;
      case 28: return 16;
      case 29: return 14;
      case 30: return 13;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for thief rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 60;
      case  2: return 59;
      case  3: return 58;
      case  4: return 58;
      case  5: return 56;
      case  6: return 55;
      case  7: return 54;
      case  8: return 53;
      case  9: return 52;
      case 10: return 51;
      case 11: return 50;
      case 12: return 49;
      case 13: return 48;
      case 14: return 47;
      case 15: return 46;
      case 16: return 45;
      case 17: return 44;
      case 18: return 43;
      case 19: return 42;
      case 20: return 41;
      case 21: return 40;
      case 22: return 39;
      case 23: return 38;
      case 24: return 37;
      case 25: return 36;
      case 26: return 35;
      case 27: return 34;
      case 28: return 33;
      case 29: return 32;
      case 30: return 31;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for thief petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 80;
      case  2: return 79;
      case  3: return 78;
      case  4: return 77;
      case  5: return 76;
      case  6: return 75;
      case  7: return 74;
      case  8: return 73;
      case  9: return 72;
      case 10: return 71;
      case 11: return 70;
      case 12: return 69;
      case 13: return 68;
      case 14: return 67;
      case 15: return 66;
      case 16: return 65;
      case 17: return 64;
      case 18: return 63;
      case 19: return 62;
      case 20: return 61;
      case 21: return 60;
      case 22: return 59;
      case 23: return 58;
      case 24: return 57;
      case 25: return 56;
      case 26: return 55;
      case 27: return 54;
      case 28: return 53;
      case 29: return 52;
      case 30: return 51;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for thief breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 75;
      case  2: return 73;
      case  3: return 71;
      case  4: return 69;
      case  5: return 67;
      case  6: return 65;
      case  7: return 63;
      case  8: return 61;
      case  9: return 59;
      case 10: return 57;
      case 11: return 55;
      case 12: return 53;
      case 13: return 51;
      case 14: return 49;
      case 15: return 47;
      case 16: return 45;
      case 17: return 43;
      case 18: return 41;
      case 19: return 39;
      case 20: return 37;
      case 21: return 35;
      case 22: return 33;
      case 23: return 31;
      case 24: return 29;
      case 25: return 27;
      case 26: return 25;
      case 27: return 23;
      case 28: return 21;
      case 29: return 19;
      case 30: return 17;
      case 31: return  0;
      case 32: return  0;
      case 33: return  0;
      case 34: return  0;
      case 35: return  0;
      case 36: return  0;
      case 37: return  0;
      case 38: return  0;
      case 39: return  0;
      case 40: return  0;
      default:
	log("SYSERR: Missing level for thief spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
    break;
  case CLASS_WARRIOR:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 65;
      case  5: return 62;
      case  6: return 58;
      case  7: return 55;
      case  8: return 53;
      case  9: return 52;
      case 10: return 50;
      case 11: return 47;
      case 12: return 43;
      case 13: return 40;
      case 14: return 38;
      case 15: return 37;
      case 16: return 35;
      case 17: return 32;
      case 18: return 28;
      case 19: return 25;
      case 20: return 24;
      case 21: return 23;
      case 22: return 22;
      case 23: return 20;
      case 24: return 19;
      case 25: return 17;
      case 26: return 16;
      case 27: return 15;
      case 28: return 14;
      case 29: return 13;
      case 30: return 12;
      case 31: return 11;
      case 32: return 10;
      case 33: return  9;
      case 34: return  8;
      case 35: return  7;
      case 36: return  6;
      case 37: return  5;
      case 38: return  4;
      case 39: return  3;
      case 40: return  2;
      case 41: return  1;	/* Some mobiles. */
      case 42: return  0;
      case 43: return  0;
      case 44: return  0;
      case 45: return  0;
      case 46: return  0;
      case 47: return  0;
      case 48: return  0;
      case 49: return  0;
      case 50: return  0;
      default:
	log("SYSERR: Missing level for warrior paralyzation saving throw.");
	break;	
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 80;
      case  2: return 78;
      case  3: return 77;
      case  4: return 75;
      case  5: return 72;
      case  6: return 68;
      case  7: return 65;
      case  8: return 63;
      case  9: return 62;
      case 10: return 60;
      case 11: return 57;
      case 12: return 53;
      case 13: return 50;
      case 14: return 48;
      case 15: return 47;
      case 16: return 45;
      case 17: return 42;
      case 18: return 38;
      case 19: return 35;
      case 20: return 34;
      case 21: return 33;
      case 22: return 32;
      case 23: return 30;
      case 24: return 29;
      case 25: return 27;
      case 26: return 26;
      case 27: return 25;
      case 28: return 24;
      case 29: return 23;
      case 30: return 22;
      case 31: return 20;
      case 32: return 18;
      case 33: return 16;
      case 34: return 14;
      case 35: return 12;
      case 36: return 10;
      case 37: return  8;
      case 38: return  6;
      case 39: return  5;
      case 40: return  4;
      case 41: return  3;
      case 42: return  2;
      case 43: return  1;
      case 44: return  0;
      case 45: return  0;
      case 46: return  0;
      case 47: return  0;
      case 48: return  0;
      case 49: return  0;
      case 50: return  0;
      default:
	log("SYSERR: Missing level for warrior rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 75;
      case  2: return 73;
      case  3: return 72;
      case  4: return 70;
      case  5: return 67;
      case  6: return 63;
      case  7: return 60;
      case  8: return 58;
      case  9: return 57;
      case 10: return 55;
      case 11: return 52;
      case 12: return 48;
      case 13: return 45;
      case 14: return 43;
      case 15: return 42;
      case 16: return 40;
      case 17: return 37;
      case 18: return 33;
      case 19: return 30;
      case 20: return 29;
      case 21: return 28;
      case 22: return 26;
      case 23: return 25;
      case 24: return 24;
      case 25: return 23;
      case 26: return 21;
      case 27: return 20;
      case 28: return 19;
      case 29: return 18;
      case 30: return 17;
      case 31: return 16;
      case 32: return 15;
      case 33: return 14;
      case 34: return 13;
      case 35: return 12;
      case 36: return 11;
      case 37: return 10;
      case 38: return  9;
      case 39: return  8;
      case 40: return  7;
      case 41: return  6;
      case 42: return  5;
      case 43: return  4;
      case 44: return  3;
      case 45: return  2;
      case 46: return  1;
      case 47: return  0;
      case 48: return  0;
      case 49: return  0;
      case 50: return  0;
      default:
	log("SYSERR: Missing level for warrior petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 85;
      case  2: return 83;
      case  3: return 82;
      case  4: return 80;
      case  5: return 75;
      case  6: return 70;
      case  7: return 65;
      case  8: return 63;
      case  9: return 62;
      case 10: return 60;
      case 11: return 55;
      case 12: return 50;
      case 13: return 45;
      case 14: return 43;
      case 15: return 42;
      case 16: return 40;
      case 17: return 37;
      case 18: return 33;
      case 19: return 30;
      case 20: return 29;
      case 21: return 28;
      case 22: return 26;
      case 23: return 25;
      case 24: return 24;
      case 25: return 23;
      case 26: return 21;
      case 27: return 20;
      case 28: return 19;
      case 29: return 18;
      case 30: return 17;
      case 31: return 16;
      case 32: return 15;
      case 33: return 14;
      case 34: return 13;
      case 35: return 12;
      case 36: return 11;
      case 37: return 10;
      case 38: return  9;
      case 39: return  8;
      case 40: return  7;
      case 41: return  6;
      case 42: return  5;
      case 43: return  4;
      case 44: return  3;
      case 45: return  2;
      case 46: return  1;
      case 47: return  0;
      case 48: return  0;
      case 49: return  0;
      case 50: return  0;
      default:
	log("SYSERR: Missing level for warrior breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 85;
      case  2: return 83;
      case  3: return 82;
      case  4: return 80;
      case  5: return 77;
      case  6: return 73;
      case  7: return 70;
      case  8: return 68;
      case  9: return 67;
      case 10: return 65;
      case 11: return 62;
      case 12: return 58;
      case 13: return 55;
      case 14: return 53;
      case 15: return 52;
      case 16: return 50;
      case 17: return 47;
      case 18: return 43;
      case 19: return 40;
      case 20: return 39;
      case 21: return 38;
      case 22: return 36;
      case 23: return 35;
      case 24: return 34;
      case 25: return 33;
      case 26: return 31;
      case 27: return 30;
      case 28: return 29;
      case 29: return 28;
      case 30: return 27;
      case 31: return 25;
      case 32: return 23;
      case 33: return 21;
      case 34: return 19;
      case 35: return 17;
      case 36: return 15;
      case 37: return 13;
      case 38: return 11;
      case 39: return  9;
      case 40: return  7;
      case 41: return  6;
      case 42: return  5;
      case 43: return  4;
      case 44: return  3;
      case 45: return  2;
      case 46: return  1;
      case 47: return  0;
      case 48: return  0;
      case 49: return  0;
      case 50: return  0;
      default:
	log("SYSERR: Missing level for warrior spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }
  default:
    log("SYSERR: Invalid class saving throw.");
    break;
  }

  /* Should not get here unless something is wrong. */
  return 100;
}

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(struct char_data *ch)
{
  int thac, base = 25,
    classType = ( GET_CLASS(ch) <= CLASS_WARRIOR ? 3 : (GET_CLASS(ch) <= CLASS_PALADIN ? 2 : 1 ));
 
  // DM - TODO - Tali decide on the thac0 system - before it was just
  // 25 - level - see below
  // TALI - Yeah, I see. Adjusted formula: BASE(25) - (Level/ClassType(1 | 2 | 3) * Modifier)
  // --> Favours remorted chars, and those with high modifiers who will need the Thac0. <--
  if (IS_NPC(ch))
      thac =20;
  else {
      /*thac = 25-GET_LEVEL(ch);*/
        thac = base - ( (GET_LEVEL(ch)/classType) * GET_MODIFIER(ch));
  }
 
  thac -= str_app[STRENGTH_AFF_APPLY_INDEX(ch)].tohit;
  thac -= (int) ((GET_AFF_INT(ch) - 13) / 2);   /* Intelligence helps! */
  thac -= (int) ((GET_AFF_WIS(ch) - 13) / 2);   /* So does wisdom */
 
  return thac;
 
}  


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data * ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch)) {
  case CLASS_MAGIC_USER:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_CLERIC:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_THIEF:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_WARRIOR:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = number(0, 100);
    break;
  }
  ch->aff_abils = ch->real_abils;
}

/* equip the newbies :) D. */
void newbie_equip(struct char_data * ch)
{
  struct obj_data *obj;
  int rnum,vnum,counter = 0;
  int equipm[15] = { 1,1,1,5,26,60,70,80,90,100,110,121,251,307 };
         
  do {
    vnum = equipm[counter];
    obj = read_object(vnum,VIRTUAL);

    if (obj) {
      obj_to_char(obj,ch);
    } 

    counter++;
  } while (counter < 15);
  
  log("Equiped newbie");
} 

/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

//  set_title(ch, NULL);
  roll_real_abils(ch);
  ch->points.max_hit = 10;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    break;

  case CLASS_CLERIC:
    break;

  case CLASS_THIEF:
    SET_SKILL(ch, SKILL_SNEAK, 10);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_STEAL, 15);
    SET_SKILL(ch, SKILL_BACKSTAB, 10);
    SET_SKILL(ch, SKILL_PICK_LOCK, 10);
    SET_SKILL(ch, SKILL_TRACK, 10);
    break;

  case CLASS_WARRIOR:
    break;
  }

  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  ENTRY_ROOM(ch,WORLD_MEDIEVAL) = world_start_room[WORLD_MEDIEVAL];
  ENTRY_ROOM(ch,WORLD_WEST) = world_start_room[WORLD_WEST];
  ENTRY_ROOM(ch,WORLD_FUTURE) = world_start_room[WORLD_FUTURE];
  log("entry rooms: %d %d %d",world_start_room[WORLD_MEDIEVAL],
      world_start_room[WORLD_WEST], world_start_room[WORLD_FUTURE]);

  /* JA Set some flags to help newbies */
  GET_GOLD(ch) = 2500;
  GET_WIMP_LEV(ch) = 7;
  PRF_TOG_CHK(ch, PRF_AUTOEXIT);
  SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
                                                 
  newbie_equip(ch);

  if (siteok_everyone)
    SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data * ch)
{
  int add_hp, add_mana = 0, add_move = 0, i;

  add_hp = con_app[GET_CON(ch)].hitp;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    add_hp += number(3, 8);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(0, 2);
    break;

  case CLASS_CLERIC:
    add_hp += number(5, 10);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(0, 2);
    break;

  case CLASS_THIEF:
    add_hp += number(7, 13);
    add_mana = 0;
    add_move = number(1, 3);
    break;

  case CLASS_WARRIOR:
    add_hp += number(10, 15);
    add_mana = 0;
    add_move = number(1, 3);
    break;
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
    GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
  else
    GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  save_char(ch, NOWHERE);

  sprintf(buf, "&G%s&g advanced to level &c%d&g", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
  info_channel(buf , ch );
}

/* DM_demote Loose maximum in various points */
void demote_level(struct char_data * ch,int newlevel)
{
  int i, sub_hp = 0, sub_mana = 0, sub_move = 0, currlevel=GET_LEVEL(ch);
 
//  extern struct wis_app_type wis_app[];
//  extern struct con_app_type con_app[];
 
  /* Subtract for each level */
  for (currlevel=GET_LEVEL(ch);currlevel!=newlevel;currlevel--)
  {
    /* Determine the hps to subtract */
    sub_hp = con_app[GET_AFF_CON(ch)].hitp;
    sub_hp += number(10, 15);
 
    /* Determine the mana to subtract */
    sub_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    /* add_mana = MIN(add_mana, 10); */
 
    /* Determine the move to subtract */
    sub_move = number(1, 3);
 
    ch->points.max_hit -= MAX(1, sub_hp);
    ch->points.max_move -= MAX(1, sub_move);
    ch->points.max_mana -= sub_mana;
    if ((GET_PRACTICES(ch) - MAX(2, wis_app[GET_AFF_WIS(ch)].bonus)) > 0)
      GET_PRACTICES(ch) -= MAX(2, wis_app[GET_AFF_WIS(ch)].bonus);
 
    GET_LEVEL(ch) = GET_LEVEL(ch) - 1;
  }
 
  /* Some sanity checks - still can be screwed a bit at least atm */
  if (ch->points.max_hit < 25) 
    ch->points.max_hit = 25;

  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
 
  if (ch->points.max_move < 85)
    ch->points.max_move = 85;
 
  /* roomflags */
  if (GET_LEVEL(ch) < LVL_ETRNL2) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_ROOMFLAGS);
  }
 
  /* holylight */
  if (GET_LEVEL(ch) < LVL_ETRNL4) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }
 
  /* invis */
  if (GET_LEVEL(ch) < LVL_ANGEL) {
    GET_INVIS_LEV(ch) = 0;
  }
 
  /* syslog */
  if (GET_LEVEL(ch) < LVL_GOD)
    REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
 
  /* nohassle, hunger, thirst, drunk */
  if (GET_LEVEL(ch) < LVL_IMMORT) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_NOHASSLE);
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
  }
  save_char(ch, NOWHERE); 

  sprintf(buf, "&G%s&g demoted to level &c%d&g", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
  info_channel(buf, ch );
}

/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0)
    return 1;	  /* level 0 */
  else if (level <= 10)
    return 2;	  /* level 1 - 10 */
  else if (level <= 20)
    return 3;	  /* level 11 - 20 */
  else if (level <= 30)
    return 4;	  /* level 21 - 30 */
  else if (level <= 40)
    return 5;	  /* level 31 - 40 */
  else if (level <= 50)
    return 6;	  /* level 41 - 50 */
  else if (level <= 60)
    return 7;	  /* level 51 - 60 */
  else if (level <= 70)
    return 8;	  /* level 61 - 70 */
  else if (level <= 80)
    return 9;	  /* level 71 - 80 */
  else if (level <= 90)
    return 10;	  /* level 81 - 90 */
  else if (level < LVL_IMMORT)
    return 11;	  /* all remaining mortal levels ( 91  - 100 ) */
  else
    return 20;	  /* IMMS and stuff */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */

int invalid_class(struct char_data *ch, struct obj_data *obj) {
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch)))
	return 1;
  else
	return 0;
}




/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void)
{

  /* spell_level(SPELL_NUM, CLASS_NUM, LEVEL, INT, STR, WIS, CON, DEX, CHA) */

  /* Primal Spells */

// Advanced Heal
  spell_level(SPELL_ADV_HEAL, CLASS_MAGIC_USER, 35, 0, 0, 20, 0, 0, 0);

// Animate Dead
  spell_level(SPELL_ANIMATE_DEAD, CLASS_MAGIC_USER, 20, 0, 12, 18, 16, 0, 0);

// Armor
  spell_level(SPELL_ARMOR, CLASS_MAGIC_USER, 1, 0, 0, 14, 0, 0, 0);

// Backstab
  spell_level(SKILL_BACKSTAB, CLASS_MAGIC_USER, 3, 0, 0, 0, 0, 14, 0);

// bash                  
  spell_level(SKILL_BASH, CLASS_MAGIC_USER, 40, 0, 17, 0, 0, 0, 0);

// bless                 
  spell_level(SPELL_BLESS, CLASS_MAGIC_USER, 5, 0, 0, 9, 0, 0, 0);

// blindness             
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 6, 0, 0, 12, 0, 0, 0);

// burning hands         
  spell_level(SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 5, 11, 0, 0, 0, 0, 0);

// call lightning        
  spell_level(SPELL_CALL_LIGHTNING, CLASS_MAGIC_USER, 15, 10, 0, 0, 0, 0, 0);

// charm person          
  spell_level(SPELL_CHARM, CLASS_MAGIC_USER, 3, 11, 0, 0, 0, 0, 0);

// chill touch           
  spell_level(SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 3, 11, 0, 0, 0, 0, 0);

// clone                 
  spell_level(SPELL_CLONE, CLASS_MAGIC_USER, 55, 20, 0, 0, 0, 0, 0);

// cloud kill            
  spell_level(SPELL_CLOUD_KILL, CLASS_MAGIC_USER, 69, 20, 21, 21, 0, 0, 0);

// color spray           
  spell_level(SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 11, 11, 0, 0, 0, 0, 0);

// compare               
  spell_level(SKILL_COMPARE, CLASS_MAGIC_USER, 5, 0, 0, 0, 0, 0, 0);

// control weather       
  spell_level(SPELL_CONTROL_WEATHER, CLASS_MAGIC_USER, 17, 11, 0, 0, 0, 0, 0);

//create food           
  spell_level(SPELL_CREATE_FOOD, CLASS_MAGIC_USER, 2, 0, 0, 14, 0, 0, 0);

//create water          (superb)  6   Wis: 14
  spell_level(SPELL_CREATE_WATER, CLASS_MAGIC_USER, 2, 0, 0, 14, 0, 0, 0);

//cure blind            (superb)  6   Wis: 11
  spell_level(SPELL_CURE_BLIND, CLASS_MAGIC_USER, 2, 0, 0, 14, 0, 0, 0);

//cure critic           (superb)  12  Wis: 14
  spell_level(SPELL_CURE_CRITIC, CLASS_MAGIC_USER, 9, 0, 0, 14, 0, 0, 0);

//cure light            (superb)  12  Wis:  9
  spell_level(SPELL_CURE_LIGHT, CLASS_MAGIC_USER, 1, 0, 0, 9, 0, 0, 0);

//curse                 (superb)  62  Wis: 12
  spell_level(SPELL_CURSE, CLASS_MAGIC_USER, 14, 0, 0, 12, 0, 0, 0);

//detect alignment      (superb)  12  Wis: 13
  spell_level(SPELL_DETECT_ALIGN, CLASS_MAGIC_USER, 4, 13, 0, 0, 0, 0, 0);

// DM - TODO - choose stats for detect death traps ...

//detect death 
  spell_level(SKILL_DETECT_DEATH, CLASS_MAGIC_USER, 40, 0, 17, 0, 0, 0, 0);

//detect invisibility   (superb)  18  Wis: 14
  spell_level(SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 2, 0, 0, 14, 0, 0, 0);

//detect magic          (superb)  12  Wis: 11
  spell_level(SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 2, 11, 0, 0, 0, 0, 0);

//detect poison         (superb)  6   Wis: 13
  spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 3, 0, 0, 13, 0, 0, 0);

//dimension gate        (superb)  112 Int: 20 Wis: 20
  spell_level(SPELL_GATE, CLASS_MAGIC_USER, 52, 20, 0, 20, 0, 0, 0);

//dispel evil           (superb)  31  Wis: 10
  spell_level(SPELL_DISPEL_EVIL, CLASS_MAGIC_USER, 14, 0, 0, 10, 0, 0, 0);

//dispel good           (superb)  31  Wis: 10
  spell_level(SPELL_DISPEL_GOOD, CLASS_MAGIC_USER, 14, 0, 0, 10, 0, 0, 0);

//divine heal           (superb)  125 Int: 21 Wis: 21
  spell_level(SPELL_DIVINE_HEAL, CLASS_MAGIC_USER, 70, 21, 0, 20, 0, 0, 0);

//divine protection     (superb)  81  Wis: 20 Dex: 18 Con: 16
  spell_level(SPELL_DIVINE_PROTECTION, CLASS_MAGIC_USER, 37, 21, 0, 20, 18, 16, 0);

//dragon                (superb)  25  Int: 13 Wis: 14 Con: 15
  spell_level(SPELL_DRAGON, CLASS_MAGIC_USER, 37, 13, 0, 14, 15, 0, 0);

//earthquake            (superb)  31  Str: 14 Wis: 17
  spell_level(SPELL_DISPEL_GOOD, CLASS_MAGIC_USER, 12, 0, 0, 17, 0, 0, 0);

//enchant weapon        (superb)  125 Str: 10 Int: 16
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGIC_USER, 26, 10, 16, 17, 0, 0, 0);

//energy drain          (superb)  31  Int: 15 Con: 10
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 13, 15, 0, 0, 10, 0, 0);

//fear                  (superb)  25  Int: 15 Wis: 15
  spell_level(SPELL_FEAR, CLASS_MAGIC_USER, 35, 15, 0, 15, 0, 0, 0);
  
//finger of death       (superb)  275 Int: 18
  spell_level(SPELL_FINGERDEATH, CLASS_MAGIC_USER, 50, 18, 0, 0, 0, 0, 0);

//fire shield           (superb)  93  Str: 17 Int: 20 Wis: 21 Dex: 20
  spell_level(SPELL_FIRE_SHIELD, CLASS_MAGIC_USER, 75, 20, 17, 21, 0, 20, 0);

//fire wall             (superb)  183 Str: 20 Int: 21 Wis: 21 Dex: 21
  spell_level(SPELL_FIRE_WALL, CLASS_MAGIC_USER, 90, 21, 20, 21, 0, 21, 0);

//fireball              (superb)  37  Int: 16
  spell_level(SPELL_FIREBALL, CLASS_MAGIC_USER, 15, 16, 0, 0, 0, 0, 0);

//fly                   (superb)  65  Int: 16 Wis: 16 Dex: 14 Con: 14
  spell_level(SPELL_FLY, CLASS_MAGIC_USER, 53, 16, 0, 16, 14, 14, 0);

//group armor           (superb)  75  Wis: 17
  spell_level(SPELL_GROUP_ARMOR, CLASS_MAGIC_USER, 9, 0, 0, 17, 0, 0, 0);

//group heal            (superb)  137 Wis: 19
  spell_level(SPELL_GROUP_HEAL, CLASS_MAGIC_USER, 22, 0, 0, 19, 0, 0, 0);

//group sanctuary       (superb)  150 Int: 15 Wis: 19
  spell_level(SPELL_GROUP_SANCTUARY, CLASS_MAGIC_USER, 30, 15, 0, 0, 0, 0, 0);

//harm                  (superb)  56  Wis: 18
  spell_level(SPELL_HARM, CLASS_MAGIC_USER, 19, 0, 0, 18, 0, 0, 0);

//haste                 (superb)  150 Int: 20 Wis: 15 Dex: 20 Con: 21
  spell_level(SPELL_HASTE, CLASS_MAGIC_USER, 34, 20, 0, 15, 21, 20, 0);

//heal                  (superb)  50  Wis: 17
  spell_level(SPELL_HEAL, CLASS_MAGIC_USER, 16, 0, 0, 17, 0, 0, 0);

//hide                  (superb)  0   Dex: 13
  spell_level(SKILL_HIDE, CLASS_MAGIC_USER, 5, 0, 0, 0, 0, 13, 0);

//holy aid              (superb)  50  Str: 17 Wis: 20 Dex: 17
  spell_level(SPELL_HOLY_AID, CLASS_MAGIC_USER, 29, 17, 0, 20, 0, 17, 0);

//hunt                  (superb)  0   Int: 13 Dex: 16
  spell_level(SKILL_HUNT, CLASS_MAGIC_USER, 24, 13, 0, 0, 0, 16, 0);

//identify              (superb)  31  Str:  3 Int:  3 Wis:  3 Dex:  3 Con:  3 Cha:  3
  spell_level(SPELL_IDENTIFY, CLASS_MAGIC_USER, 37, 16, 0, 17, 0, 15, 0);

//infravision           (superb)  25  Int: 13
  spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 13, 13, 0, 0, 0, 0, 0);

//invisibility          (superb)  37  Int: 17
  spell_level(SPELL_INVISIBLE, CLASS_MAGIC_USER, 4, 17, 0, 0, 0, 0, 0);

//kick                  (superb)  0   Str: 15 Dex: 13
  spell_level(SKILL_KICK, CLASS_MAGIC_USER, 1, 17, 0, 0, 0, 13, 0);

//lightning bolt        (superb)  18  Int: 12
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 9, 12, 0, 0, 0, 0, 0);

//lightning shield      (superb)  106 Str: 15 Int: 19 Wis: 20 Dex: 19
  spell_level(SPELL_LIGHT_SHIELD, CLASS_MAGIC_USER, 65, 19, 15, 20, 0, 19, 0);

//locate object         (superb)  25  Int: 16
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 6, 16, 0, 0, 0, 0, 0);

// magic missile 	(superb)  12  Int: 10
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1, 10, 0, 0, 0, 0, 0);

//mana                  (superb)  12  Str:  3 Int:  3 Wis:  3 Dex:  3 Con:  3 Cha:  3
//meteor swarm          (superb)  87  Int: 20
  spell_level(SPELL_METEOR_SWARM, CLASS_MAGIC_USER, 37, 20, 0, 0, 0, 0, 0);

//nohassle              (superb)  275 Int: 21 Wis: 21 Dex: 20
  spell_level(SPELL_NOHASSLE, CLASS_MAGIC_USER, 80, 21, 0, 21, 0, 20, 0);

//paralyze              (superb)  37  Int: 15 Wis: 16
  spell_level(SPELL_NOHASSLE, CLASS_MAGIC_USER, 32, 15, 0, 16, 0, 0, 0);

//pick lock             (superb)  0   Dex: 14
  spell_level(SKILL_PICK_LOCK, CLASS_MAGIC_USER, 2, 0, 0, 0, 0, 14, 0);

//plasma blast          (superb)  62  Int: 18
  spell_level(SPELL_PLASMA_BLAST, CLASS_MAGIC_USER, 30, 18, 0, 0, 0, 0, 0);

//poison                (superb)  25  Wis: 11
  spell_level(SPELL_POISON, CLASS_MAGIC_USER, 3, 0, 0, 11, 0, 0, 0);

//primal scream         (superb)  0   Str: 18 Dex: 12 Con: 14
  spell_level(SKILL_PRIMAL_SCREAM, CLASS_MAGIC_USER, 43, 0, 18, 0, 14, 12, 0);

//protection from evil  (superb)  153 Wis: 17 Con: 16
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_MAGIC_USER, 43, 0, 0, 17, 16, 0, 0);

//protection from good  (superb)  166 Wis: 18 Con: 16
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_MAGIC_USER, 43, 0, 0, 18, 16, 0, 0);

//refresh               (superb)  37  Wis: 16 Con: 14
  spell_level(SPELL_REFRESH, CLASS_MAGIC_USER, 30, 0, 0, 16, 14, 0, 0);

//remove curse          (superb)  31  Wis: 11
  spell_level(SPELL_REMOVE_CURSE, CLASS_MAGIC_USER, 26, 0, 0, 11, 0, 0, 0);

//remove paralysis      (superb)  37  Int: 15 Wis: 17
  spell_level(SPELL_REMOVE_PARA, CLASS_MAGIC_USER, 32, 15, 0, 17, 0, 0, 0);

//remove poison         (superb)  10  Wis: 13
  spell_level(SPELL_REMOVE_POISON, CLASS_MAGIC_USER, 10, 0, 0, 13, 0, 0, 0);

//rescue                (superb)  0   Str: 16 Dex: 15 Con: 10
  spell_level(SKILL_RESCUE, CLASS_MAGIC_USER, 3, 0, 16, 0, 10, 15, 0);

//retreat               (superb)  0   Wis: 15 Dex: 21 Con: 21
  spell_level(SKILL_RETREAT, CLASS_MAGIC_USER, 58, 0, 0, 15, 21, 21, 0);

//sanctuary             (superb)  125 Int: 15 Wis: 16
  spell_level(SPELL_SANCTUARY, CLASS_MAGIC_USER, 15, 15, 0, 16, 0, 0, 0);

//scan                  (superb)  0   Int: 12 Wis: 15
  spell_level(SKILL_SCAN, CLASS_MAGIC_USER, 7, 12, 0, 15, 0, 0, 0);

//second attack         (superb)  0   Str: 16 Dex: 15
  spell_level(SKILL_2ND_ATTACK, CLASS_MAGIC_USER, 35, 0, 16, 0, 0, 15, 0);

//sense life            (superb)  25  Wis: 16 Dex: 16
  spell_level(SPELL_SENSE_LIFE, CLASS_MAGIC_USER, 20, 0, 0, 16, 0, 16, 0);

//serpent skin          (superb)  87  Int: 21 Wis: 20
  spell_level(SPELL_SERPENT_SKIN, CLASS_MAGIC_USER, 65, 21, 0, 20, 0, 18, 0);

//shocking grasp        (superb)  18  Int: 10
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 7, 10, 0, 0, 0, 0, 0);

//sleep                 (superb)  31  Int: 12
  spell_level(SPELL_SLEEP, CLASS_MAGIC_USER, 8, 12, 0, 0, 0, 0, 0);

//sneak                 (superb)  0   Dex: 15
  spell_level(SKILL_SNEAK, CLASS_MAGIC_USER, 1, 0, 0, 0, 0, 15, 0);

//spirit armor          (superb)  52  Wis: 20 Dex: 17
  spell_level(SPELL_SPIRIT_ARMOR, CLASS_MAGIC_USER, 23, 0, 0, 20, 0, 17, 0);

//spy                   (awful)   0   Wis: 15 Dex: 17 Con: 15
  spell_level(SKILL_SPY, CLASS_MAGIC_USER, 17, 0, 0, 15, 15, 17, 0);

//steal                 (superb)  0   Dex: 15
  spell_level(SKILL_STEAL, CLASS_MAGIC_USER, 4, 0, 0, 0, 0, 15, 0);

//stoneskin             (superb)  58  Int: 20 Wis: 21 Dex: 18
  spell_level(SPELL_STONESKIN, CLASS_MAGIC_USER, 78, 20, 0, 21, 0, 18, 0);

//strength              (superb)  37  Int: 10 Con: 12
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 6, 10, 0, 0, 12, 0, 0);

//summon                (superb)  75  Int: 16 Con: 14
  spell_level(SPELL_SUMMON, CLASS_MAGIC_USER, 10, 16, 0, 0, 14, 0, 0);

//teleport              (superb)  137 Int: 16
  spell_level(SPELL_TELEPORT, CLASS_MAGIC_USER, 35, 16, 0, 0, 0, 0, 0);

//third attack          (superb)  0   Str: 20 Dex: 18
  spell_level(SKILL_3RD_ATTACK, CLASS_MAGIC_USER, 50, 0, 20, 0, 0, 18, 0);

// throw -- no spell_level???

//track                 (superb)  0   Int: 10 Dex: 15
  spell_level(SKILL_TRACK, CLASS_MAGIC_USER, 6, 0, 0, 0, 0, 15, 0);

//waterbreathe          (superb)  50  Int: 18 Wis: 16 Con: 18
  spell_level(SPELL_WATERBREATHE, CLASS_MAGIC_USER, 45, 18, 0, 16, 18, 0, 0);

//waterwalk             (superb)  37  Int: 15 Wis: 12 Dex: 13
  spell_level(SPELL_WATERWALK, CLASS_MAGIC_USER, 28, 15, 0, 12, 0, 13, 0);

//whirlwind             (superb)  62  Str: 18 Wis: 21 Dex: 17
  spell_level(SPELL_WHIRLWIND, CLASS_MAGIC_USER, 7, 0, 18, 21, 0, 17, 0);

//word of recall        (superb)  12  Int: 16 Con: 13
  spell_level(SPELL_WORD_OF_RECALL, CLASS_MAGIC_USER, 12, 16, 0, 0, 13, 0, 0);

//wraith touch          (superb)  187 Int: 21 Wis: 21 Con: 21
  spell_level(SPELL_WRAITH_TOUCH, CLASS_MAGIC_USER, 7, 21, 0, 21, 21, 0, 0);


  /* MAGES 
  Taken out for the time being for testing (all spells been assigned to MAGIC_USER  
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 2, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 2, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 3, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 3, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_INVISIBLE, CLASS_MAGIC_USER, 4, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_ARMOR, CLASS_MAGIC_USER, 4, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 5, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 6, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_STRENGTH, CLASS_MAGIC_USER, 6, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 7, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_SLEEP, CLASS_MAGIC_USER, 8, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 9, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 9, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 10, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 11, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 13, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CURSE, CLASS_MAGIC_USER, 14, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_POISON, CLASS_MAGIC_USER, 14, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_FIREBALL, CLASS_MAGIC_USER, 15, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CHARM, CLASS_MAGIC_USER, 16, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGIC_USER, 26, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CLONE, CLASS_MAGIC_USER, 30, 10, 10, 10, 10, 10, 10);

  */

  /* spell_level(SPELL_NUM, CLASS_NUM, LEVEL, INT, STR, WIS, CON, DEX, CHA) */

  /* CLERICS 
  Taken out for the time being for testing (all spells been assigned to MAGIC_USER  
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 1, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 2, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 2, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 3, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 4, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 4, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_BLESS, CLASS_CLERIC, 5, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DETECT_INVIS, CLASS_CLERIC, 6, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 6, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 7, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 8, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_POISON, CLASS_CLERIC, 8, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 9, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 9, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_SUMMON, CLASS_CLERIC, 10, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 10, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 12, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 12, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 14, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 14, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 15, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 15, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_HEAL, CLASS_CLERIC, 16, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 17, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_SENSE_LIFE, CLASS_CLERIC, 18, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_HARM, CLASS_CLERIC, 19, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 22, 10, 10, 10, 10, 10, 10);
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 26, 10, 10, 10, 10, 10, 10);
*/
  /* spell_level(SPELL_NUM, CLASS_NUM, LEVEL, INT, STR, WIS, CON, DEX, CHA) */

  /* THIEVES 
  Taken out for the time being for testing (all spells been assigned to MAGIC_USER  
  spell_level(SKILL_SNEAK, CLASS_THIEF, 1, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_PICK_LOCK, CLASS_THIEF, 2, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_BACKSTAB, CLASS_THIEF, 3, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_STEAL, CLASS_THIEF, 4, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_HIDE, CLASS_THIEF, 5, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_TRACK, CLASS_THIEF, 6, 10, 10, 10, 10, 10, 10);
  */
  /* spell_level(SPELL_NUM, CLASS_NUM, LEVEL, INT, STR, WIS, CON, DEX, CHA) */

  /* WARRIORS
  Taken out for the time being for testing (all spells been assigned to MAGIC_USER  
  spell_level(SKILL_KICK, CLASS_WARRIOR, 1, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_RESCUE, CLASS_WARRIOR, 3, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_TRACK, CLASS_WARRIOR, 9, 10, 10, 10, 10, 10, 10);
  spell_level(SKILL_BASH, CLASS_WARRIOR, 12, 10, 10, 10, 10, 10, 10);
  */
}


/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  1000000000

/* Function to return the exp required for each class/level */
int level_exp(struct char_data *ch, int level)
{
  int exp=0;

  if (level > LVL_OWNER || level < 0) {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /*
   * Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist.
   */
//   if (level > LVL_IMMORT) {
//     return EXP_MAX - ((LVL_IMPL - level) * 1000);
//   }

   if (level >= LVL_IMMORT)
     return 0;

   switch(level) {
	case 1: exp=2000; break; 
	case 2: exp=4000; break; 
	case 3: exp=8000; break; 
	case 4: exp=16000; break; 
	case 5: exp=32000; break;  
	case 6: exp=64000; break;  
	case 7: exp=96000; break;  
	case 8: exp=128000; break; 
	case 9: exp=192000; break;  
	case 10: exp=256000; break;  
	case 11: exp=352000; break;  
	case 12: exp=448000; break; 
	case 13: exp=576000; break; 
 	case 14: exp=704000; break;  
	case 15: exp=896000; break;  
	case 16: exp=896000; break; 
	case 17: exp=896000; break;  
	case 18: exp=1088000; break;  
	case 19: exp=1088000; break;  
	case 20: exp=1088000; break; 
	case 21: exp=1088000; break;  
	case 22: exp=1600000; break;  
	case 23: exp=1600000; break;  
	case 24: exp=1600000; break; 
	case 25: exp=1952000; break;  
	case 26: exp=1952000; break;  
	case 27: exp=1952000; break;  
	case 28: exp=2030400; break; 
	case 29: exp=2030400; break;  
	case 30: exp=2030400; break;  
	case 31: exp=2752000; break;  
	case 32: exp=2752000; break; 
	case 33: exp=3200000; break;  
	case 34: exp=3200000; break;  
	case 35: exp=3776000; break;  
	case 36: exp=3776000; break; 
	case 37: exp=4352000; break;  
	case 38: exp=4352000; break;  
	case 39: exp=5056000; break;  
	case 40: exp=5056000; break; 
	case 41: exp=5056000; break;  
	case 42: exp=5760000; break;  
	case 43: exp=5760000; break;  
	case 44: exp=6556000; break; 
	case 45: exp=6556000; break;  
	case 46: exp=7562000; break;  
	case 47: exp=7562000; break;  
	case 48: exp=9162000; break; 
	case 49: exp=9162000; break;  
	case 50: exp=11114000; break;  
	case 51: exp=11114000; break;  
	case 52: exp=13066000; break; 
	case 53: exp=13066000; break;  
	case 54: exp=15018000; break;  
	case 55: exp=15018000; break;  
	case 56: exp=17322000; break; 
	case 57: exp=17322000; break;  
	case 58: exp=20074000; break;  
	case 59: exp=20074000; break;  
	case 60: exp=23274000; break; 
	case 61: exp=23274000; break;  
	case 62: exp=27050000; break;  
	case 63: exp=27050000; break;  
	case 64: exp=43114000; break; 
	case 65: exp=43114000; break;  
	case 66: exp=50676000; break;  
	case 67: exp=50676000; break;  
	case 68: exp=50676000; break; 
	case 69: exp=59838000; break;  
	case 70: exp=59838000; break;  
	case 71: exp=59838000; break;  
	case 72: exp=70952000; break; 
	case 73: exp=70952000; break;  
	case 74: exp=84018000; break;  
	case 75: exp=84018000; break;  
	case 76: exp=99036000; break; 
	case 77: exp=99036000; break;  
	case 78: exp=99036000; break;  
	case 79: exp=116308000; break;  
	case 80: exp=116308000; break; 
	case 81: exp=116308000; break;  
	case 82: exp=116308000; break;  
	case 83: exp=136432000; break;  
	case 84: exp=136432000; break; 
	case 85: exp=136432000; break; 
	case 86: exp=150000000; break; 
	case 87: exp=150000000; break; 
	case 88: exp=150000000; break; 
	case 89: exp=175000000; break; 
	case 90: exp=175000000; break; 
	case 91: exp=175000000; break; 
	case 92: exp=200000000; break; 
	case 93: exp=200000000; break; 
	case 94: exp=200000000; break; 
	case 95: exp=225000000; break; 
	case 96: exp=225000000; break; 
	case 97: exp=250000000; break; 
	case 98: exp=250000000; break; 
	case 99: exp=300000000; break; 
	default:
		return 300000000; break; 
   }  

//  sprintf(buf,"exp=%d  modifier=%f newexp=%d",exp,GET_MODIFIER(ch),(int)(exp*GET_MODIFIER(ch)));
//  log(buf);

   return (int)(exp*GET_MODIFIER(ch));

  /*
   * This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete -- so, complete them!
   */
  log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
}


/* 
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Man";
  if (level == LVL_IMPL)
    return "the Implementor";

  switch (chclass) {

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delver in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribe of Magic";
      case  7: return "the Seer";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjurer";
      case 11: return "the Invoker";
      case 12: return "the Enchanter";
      case 13: return "the Conjurer";
      case 14: return "the Magician";
      case 15: return "the Creator";
      case 16: return "the Savant";
      case 17: return "the Magus";
      case 18: return "the Wizard";
      case 19: return "the Warlock";
      case 20: return "the Sorcerer";
      case 21: return "the Necromancer";
      case 22: return "the Thaumaturge";
      case 23: return "the Student of the Occult";
      case 24: return "the Disciple of the Uncanny";
      case 25: return "the Minor Elemental";
      case 26: return "the Greater Elemental";
      case 27: return "the Crafter of Magics";
      case 28: return "the Shaman";
      case 29: return "the Keeper of Talismans";
      case 30: return "the Archmage";
      case LVL_IMMORT: return "the Immortal Warlock";
      case LVL_GOD: return "the Avatar of Magic";
      case LVL_GRGOD: return "the God of Magic";
      default: return "the Mage";
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deacon";
      case  8: return "the Vicar";
      case  9: return "the Priest";
      case 10: return "the Minister";
      case 11: return "the Canon";
      case 12: return "the Levite";
      case 13: return "the Curate";
      case 14: return "the Monk";
      case 15: return "the Healer";
      case 16: return "the Chaplain";
      case 17: return "the Expositor";
      case 18: return "the Bishop";
      case 19: return "the Arch Bishop";
      case 20: return "the Patriarch";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Cardinal";
      case LVL_GOD: return "the Inquisitor";
      case LVL_GRGOD: return "the God of good and evil";
      default: return "the Cleric";
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case  1: return "the Pilferer";
      case  2: return "the Footpad";
      case  3: return "the Filcher";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincher";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcher";
      case  9: return "the Sharper";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magsman";
      case 13: return "the Highwayman";
      case 14: return "the Burglar";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Killer";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assasin";
      case LVL_GOD: return "the Demi God of thieves";
      case LVL_GRGOD: return "the God of thieves and tradesmen";
      default: return "the Thief";
    }
    break;

    case CLASS_WARRIOR:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentry";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordsman";
      case  9: return "the Fencer";
      case 10: return "the Combatant";
      case 11: return "the Hero";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckler";
      case 14: return "the Mercenary";
      case 15: return "the Swordmaster";
      case 16: return "the Lieutenant";
      case 17: return "the Champion";
      case 18: return "the Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of war";
      default: return "the Warrior";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}


/* 
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Woman";
  if (level == LVL_IMPL)
    return "the Implementress";

  switch (chclass) {

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delveress in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribess of Magic";
      case  7: return "the Seeress";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjuress";
      case 11: return "the Invoker";
      case 12: return "the Enchantress";
      case 13: return "the Conjuress";
      case 14: return "the Witch";
      case 15: return "the Creator";
      case 16: return "the Savant";
      case 17: return "the Craftess";
      case 18: return "the Wizard";
      case 19: return "the War Witch";
      case 20: return "the Sorceress";
      case 21: return "the Necromancress";
      case 22: return "the Thaumaturgess";
      case 23: return "the Student of the Occult";
      case 24: return "the Disciple of the Uncanny";
      case 25: return "the Minor Elementress";
      case 26: return "the Greater Elementress";
      case 27: return "the Crafter of Magics";
      case 28: return "Shaman";
      case 29: return "the Keeper of Talismans";
      case 30: return "Archwitch";
      case LVL_IMMORT: return "the Immortal Enchantress";
      case LVL_GOD: return "the Empress of Magic";
      case LVL_GRGOD: return "the Goddess of Magic";
      default: return "the Witch";
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deaconess";
      case  8: return "the Vicaress";
      case  9: return "the Priestess";
      case 10: return "the Lady Minister";
      case 11: return "the Canon";
      case 12: return "the Levitess";
      case 13: return "the Curess";
      case 14: return "the Nunne";
      case 15: return "the Healess";
      case 16: return "the Chaplain";
      case 17: return "the Expositress";
      case 18: return "the Bishop";
      case 19: return "the Arch Lady of the Church";
      case 20: return "the Matriarch";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Priestess";
      case LVL_GOD: return "the Inquisitress";
      case LVL_GRGOD: return "the Goddess of good and evil";
      default: return "the Cleric";
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case  1: return "the Pilferess";
      case  2: return "the Footpad";
      case  3: return "the Filcheress";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincheress";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcheress";
      case  9: return "the Sharpress";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magswoman";
      case 13: return "the Highwaywoman";
      case 14: return "the Burglaress";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Murderess";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      case 34: return "the Implementress";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assasin";
      case LVL_GOD: return "the Demi Goddess of thieves";
      case LVL_GRGOD: return "the Goddess of thieves and tradesmen";
      default: return "the Thief";
    }
    break;

    case CLASS_WARRIOR:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentress";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordswoman";
      case  9: return "the Fenceress";
      case 10: return "the Combatess";
      case 11: return "the Heroine";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckleress";
      case 14: return "the Mercenaress";
      case 15: return "the Swordmistress";
      case 16: return "the Lieutenant";
      case 17: return "the Lady Champion";
      case 18: return "the Lady Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Lady Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Lady of War";
      case LVL_GOD: return "the Queen of Destruction";
      case LVL_GRGOD: return "the Goddess of war";
      default: return "the Warrior";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}

float exp_modifers(int classnum)
{
  switch(classnum) {
    case CLASS_CLERIC: return 1.0;
    case CLASS_MAGIC_USER: return 1.0;
    case CLASS_WARRIOR: return 1.0;
    case CLASS_THIEF: return 1.0;
    case CLASS_DRUID: return 1.0;
    case CLASS_PRIEST: return 1.0;
    case CLASS_NIGHTBLADE: return 1.0;
    case CLASS_BATTLEMAGE: return 1.0;
    case CLASS_SPELLSWORD: return 1.0;
    case CLASS_PALADIN: return 1.0;
    case CLASS_MASTER: return 1.0;
    default: return 1.0;
  }
}
