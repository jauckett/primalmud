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
#include "handler.h"
#include "constants.h"

extern int siteok_everyone;
extern int world_start_room[NUM_WORLDS];

/* local functions */
int parse_class(char *arg);
long find_class_bitvector(char arg);
byte saving_throws(struct char_data *ch, int type);
void roll_real_abils(struct char_data * ch);
void do_start(struct char_data * ch);
int backstab_mult(int level);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(struct char_data *ch, int level);

void send_to_char(const char *msg, struct char_data *ch);

/* Names first */

const char *class_abbrevs[] =
{
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

const int exp_reqs[LVL_IMMORT] =
{
          0,      2000,      4000,      8000,     16000, //   0-4
      32000,     64000,     96000,    128000,    192000, //   5-9
     256000,    352000,    448000,    576000,    704000, //  10-14
     896000,    896000,    896000,   1088000,   1088000, //  15-19
    1088000,   1088000,   1600000,   1600000,   1600000, //  20-24
    1952000,   1952000,   1952000,   2030400,   2030400, //  25-29
    2030400,   2752000,   2752000,   3200000,   3200000, //  30-34
    3776000,   3776000,   4352000,   4352000,   5056000, //  35-39
    5056000,   5056000,   5760000,   5760000,   6556000, //  40-44
    6556000,   7562000,   7562000,   9162000,   9162000, //  45-49
   11114000,  11114000,  13066000,  13066000,  15018000, //  50-54
   15018000,  17322000,  17322000,  20074000,  20074000, //  55-59
   23274000,  23274000,  27050000,  27050000,  43114000, //  60-64
   43114000,  50676000,  50676000,  50676000,  59838000, //  65-69
   59838000,  59838000,  70952000,  70952000,  84018000, //  70-74
   84018000,  99036000,  99036000,  99036000, 116308000, //  75-79
  116308000, 116308000, 116308000, 136432000, 136432000, //  80-84
  136432000, 150000000, 150000000, 150000000, 175000000, //  85-89
  175000000, 175000000, 200000000, 200000000, 200000000, //  90-94
  225000000, 225000000, 250000000, 250000000, 300000000, //  95-99
  350000000, 400000000, 450000000, 500000000, 550000000, // 100-104
};

const char *race_abbrevs[] = {
  "Ogr",
  "Dev",
  "Min",
  "Elf",
  "Hum",
  "Dwa",
  "Pix",
  "\n"
};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"  [&cC&n]leric\r\n"
"  [&cT&n]hief\r\n"
"  [&cW&n]arrior\r\n"
"  [&cM&n]agician\r\n"
"&gSelect a Class&n: ";

/* Race menu shown at character creation */
const char *race_menu =
"  [&cO&n]gre\r\n"
"  [&cD&n]eva\r\n"
"  [&cM&n]inotaur\r\n"
"  [&cE&n]lf\r\n"
"  [&cH&n]uman\r\n"
"   D[&cW&n]arf\r\n"
"  [&cP&n]ixie\r\n"
"&gSelect a Race&n: ";

const char *race_help =
"\r\nA Race determines the attributes that your character possesses.\r\n"
"The three primary attributes determined by race are the &BMaximum Stat Values&n,\r\n"
"&BExtra Abilities&n and &BEquipment Positions&n.\r\n"
"The following Races are available on PrimalMUD:\r\n"
"&c1&n). [&cO&n]gre\r\n"
"&c2&n). [&cD&n]eva\r\n"
"&c3&n). [&cM&n]inotaur\r\n"
"&c4&n). [&cE&n]lf\r\n"
"&c5&n). [&cH&n]uman\r\n"
"&c6&n).  D[&cW&n]arf\r\n"
"&c7&n). [&cP&n]ixie\r\n\r\n"
"For further information on each of the availble races choose the appropiate\r\n"
"option (1-7,O,D,M,E,H,W,P), or just press &cEnter&n to return.\r\n"
"\r\n&gChoice&n: ";

const char *class_help = 
"\r\nAs of PrimalMUD 3.0, classes have been implemented over the previous classless\r\n"
"system. A character first chooses one of the available &Bbase classes&n:\r\n"
"&c1&n). [&cM&n]&gagician&n\r\n"
"&c2&n). [&cC&n]&gleric&n\r\n"
"&c3&n). [&cT&n]&ghief&n\r\n"
"&c4&n). [&cW&n]&garrior&n\r\n\r\n"
"When between your first levels of [&c50&n-&c100&n] you have the option of remorting\r\n"
"into one of the following &Bfirst level remort&n classes:\r\n"
"&c5&n). [&cD&n]&gruid&n      (Cleric and Magician)\r\n"
"&c6&n). [&cP&n]&griest&n     (Thief and Cleric)\r\n"
"&c7&n). [&cN&n]&gightblade&n (Thief and Warrior)\r\n"
"&c8&n). [&cB&n]&gattlemage&n (Warrior and Magician)\r\n"
"&c9&n). [&cS&n]&gpellsword&n (Thief and Magician)\r\n"
"&c0&n). [&cPA&n]&gladin&n    (Warrior and Cleric)\r\n\r\n"
"Remorting allows the character to become familiar with the traits of another\r\n"
"&Bbase class&n and you re-enter the world with these new abilities.\r\n"
"When between levels [&c75&n-&c100&n] of your &Bfirst remort level&n, you are able\r\n"
"to remort into the &n[&cMAS&n]&Bter&n class. The &BMaster&n class is a supreme class which\r\n"
"has traits of all of the &Bbase classes&n.\r\n\r\n"
"For further information on each of the availble classes choose the appropiate\r\n"
"option (0-9,M,C,T,W,D,P,N,B,S,Pa,Mas), or just press &cEnter&n to return.\r\n"
"\r\n&gChoice&n: ";

const char *char_help = 
"This is the character creation help *TODO*\r\n";

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char *arg)
{
  char letter = LOWER(arg[0]);
 
  // M - Mage
  // M<x>g - Mage
  // M<x>a - Master
  // C - Cleric
  // W - Warrior
  // T - Thief
  // N - Nightblade
  // B - BattleMage
  // S - Spellsword
  // P - Priest
  // Pr - Priest
  // Pa - Paladin
  
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
      // (This is used in both race selection and race setting)
      // Dwarfs and Devas: if strlen > 1, check second char
      // W is the abbreviation for dWarf
      case 'd': 
        if (strlen(arg) > 1) {
          switch(LOWER(arg[1])) {
            case 'w': return RACE_DWARF;
            case 'e': return RACE_DEVA;
            default: return RACE_UNDEFINED;
          }
        } else {
          return RACE_DEVA;  
        }
      case 'o': return RACE_OGRE;
      case 'm': return RACE_MINOTAUR;
      case 'e': return RACE_ELF;
      case 'h': return RACE_HUMAN;
      case 'p': return RACE_PIXIE;
      case 'w': return RACE_DWARF;
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
  {75,		75,	75,	75,     75,	75,	75,	75, 
	   75,      75,     75},		
  {25,		25,	25,	25,     25,     25, 	25,	25, 
	   25,      25,     25},		
  {22,		22,	22,	22,     22, 	22, 	22,	22, 
	   22,      22,     22},	
  { SPELL_BOTH, SPELL_BOTH, SPELL_BOTH, SPELL_BOTH, SPELL_BOTH, SPELL_BOTH, 
    SPELL_BOTH, SPELL_BOTH, SPELL_BOTH, SPELL_BOTH, SPELL_BOTH}		
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

// DM: not bothering with guards ..
//int guild_info[][3] = {

/* Midgaard */
//  {CLASS_MAGIC_USER,	3017,	SCMD_SOUTH},
//  {CLASS_CLERIC,	3004,	SCMD_NORTH},
//  {CLASS_THIEF,		3027,	SCMD_EAST},
//  {CLASS_WARRIOR,	3021,	SCMD_EAST},

/* Brass Dragon */
//  {-999 /* all */ ,	5065,	SCMD_WEST},

/* this must go last -- add new guards above! */
//  {-1, -1, -1}
//};



/*
 * Saving throws for:
 *   PARA, ROD, PETRI, BREATH, SPELL
 *
 */

byte saving_throws(struct char_data *ch, int type)
{
 
  const byte saving[5][MAX_RACES][NUM_CLASSES] = { 
  /* Paralysis */
  {
    //Ma  C   T   W   D   Pr  N   B   S   Pa  Mr
    { 87, 80, 82, 87, 77, 72, 78, 84, 79, 77, 66}, /* Ogre */
    { 87, 80, 82, 87, 77, 72, 78, 84, 79, 77, 66}, /* Deva */
    { 91, 84, 86, 91, 81, 76, 82, 88, 83, 81, 70}, /* Minotaur */
    { 87, 80, 82, 87, 77, 72, 78, 84, 79, 77, 66}, /* Elf */
    { 83, 76, 78, 83, 73, 68, 74, 80, 75, 73, 62}, /* Human */
    { 85, 78, 80, 85, 75, 70, 76, 82, 77, 75, 64}, /* Dwarf */
    { 89, 82, 84, 89, 79, 74, 80, 86, 81, 79, 68}},/* Pixie */
  /* Rod  */
  {
    //Ma  C   T   W   D   Pr  N   B   S   Pa  Mr
    { 84, 84, 92, 92, 74, 82, 90, 82, 82, 82, 70}, /* Ogre */
    { 76, 76, 84, 84, 66, 74, 82, 74, 74, 74, 62}, /* Deva */
    { 88, 88, 96, 96, 78, 86, 94, 86, 86, 86, 74}, /* Minotaur */
    { 76, 76, 84, 84, 66, 74, 82, 74, 74, 74, 62}, /* Elf */
    { 84, 84, 92, 92, 74, 82, 90, 82, 82, 82, 70}, /* Human */
    { 73, 73, 81, 81, 63, 71, 79, 71, 71, 71, 59}, /* Dwarf */
    { 76, 76, 84, 84, 66, 74, 82, 74, 74, 74, 62}},/* Pixie */
  /* Petrify */
  {
    //Ma  C   T   W   D   Pr  N   B   S   Pa  Mr
    { 86, 82, 84, 87, 78, 76, 81, 83, 80, 79, 69}, /* Ogre */
    { 84, 80, 82, 85, 76, 74, 79, 81, 78, 77, 67}, /* Deva */
    { 93, 89, 91, 94, 85, 83, 88, 90, 87, 86, 76}, /* Minotaur */
    { 79, 75, 77, 80, 71, 69, 74, 76, 73, 72, 62}, /* Elf */
    { 86, 82, 84, 87, 78, 76, 81, 83, 80, 79, 69}, /* Human */
    { 82, 78, 80, 83, 74, 72, 77, 79, 76, 75, 65}, /* Dwarf */
    { 86, 82, 84, 87, 78, 76, 81, 83, 80, 79, 69}},/* Pixie */
  /* Breath */
  {
    //Ma  C   T   W   D   Pr  N   B   S   Pa  Mr
    { 88, 89, 84, 86, 85, 81, 78, 82, 80, 83, 71}, /* Ogre */
    { 84, 85, 80, 82, 81, 77, 74, 78, 76, 79, 67}, /* Deva */
    { 96, 97, 92, 94, 93, 89, 86, 90, 88, 91, 79}, /* Minotaur */
    { 70, 71, 66, 68, 67, 63, 60, 64, 62, 65, 53}, /* Elf */
    { 82, 83, 78, 80, 79, 75, 72, 76, 74, 77, 65}, /* Human */
    { 82, 83, 78, 80, 79, 75, 72, 76, 74, 77, 65}, /* Dwarf */
    { 79, 80, 75, 77, 76, 72, 69, 73, 71, 74, 62}},/* Pixie */
  /* Spell */
  {
    //Ma  C   T   W   D   Pr  N   B   S   Pa  Mr
    { 82, 84, 93, 96, 70, 81, 92, 81, 79, 83, 66}, /* Ogre */
    { 69, 71, 80, 83, 57, 68, 79, 68, 66, 70, 53}, /* Deva */
    { 85, 87, 96, 99, 73, 84, 95, 84, 82, 86, 69}, /* Minotaur */
    { 69, 71, 80, 83, 57, 68, 79, 68, 66, 70, 53}, /* Elf */
    { 78, 80, 89, 92, 66, 77, 88, 77, 75, 79, 62}, /* Human */
    { 66, 68, 77, 80, 54, 65, 76, 65, 63, 67, 50}, /* Dwarf */
    { 72, 74, 83, 86, 60, 71, 82, 71, 69, 73, 56}} /* Pixie */
  };	
  
  			       // 00-10, 11-20, 21-30, 31-40, 41-50
  const byte saving_levels[10] = {   2  ,   4  ,   6  ,   8  ,  10  ,
  			       // 51-60, 61-70, 71-80, 81-90, 91-100
			           13  ,  16  ,  19  ,  23  ,  27 };
  
  			       // 00-10, 11-20, 21-30, 31-40, 41-50
  const byte npc_saves[20] = {     95  ,  90  ,  85  ,  80  ,  75, 
  			       // 51-60, 61-70, 71-80, 81-90, 91-100
			           70  ,  65  ,  60  ,  55  ,  50, 
  			       // 100-110, 111-120, 121-130, 131-140, 141-150
			           45  ,  40  ,  35  ,  30  ,  25, 
  			       // 151-160, 161-170, 171-180, 181-190, 191-200
			           20  ,  15  ,  10  ,   5  ,   0 };
	  
  if (IS_NPC(ch))
  {
     if (GET_LEVEL(ch) > LVL_OWNER)
       return 0;
     return (byte)(npc_saves[(ubyte)(GET_LEVEL(ch) / 10)]);
  } 

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (0);

  // subtract the level bonus from the given saving throw
  return (byte)(saving[type][GET_RACE(ch)][(int)GET_CLASS(ch)] - saving_levels[(int)(GET_LEVEL(ch) / 10)] + GET_SAVE(ch, type)); 
}

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(struct char_data *ch, struct char_data *vict)
{
  /* ARTUS - Bah, lets try and balance this shit out better... */
  int base = 20;
  int level = 0;

  if (IS_NPC(ch)) 
  {
    level = GET_LEVEL(ch);
    /*
    if (vict)
    {
      // Artus - Trial Mob Thaco Levels
      base = 15;
      if (level > GET_LEVEL(vict))
	base = 5;
      if (level > (GET_LEVEL(vict) + 10))
	base = 0;
      if (level == GET_LEVEL(vict))
	base = 10;
      if (level < (GET_LEVEL(vict) - 10))
	base = 20;
    } else {
    */ 
      base = thaco_ac[MIN(level, 100)][CLASS_WARRIOR];
    /* } */
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch))
    {
      sprintf(buf, "DBG: Thaco(%s) = %d\r\n", GET_NAME(ch), base);
      send_to_char(buf, ch);
    }
#endif
    return base;
  }

  // remort chars keep last remort level thac0, or their current level if higher
  level = MAX(GET_LEVEL(ch), GET_LAST_LVL(ch));

  base = thaco_ac[MIN(level, 100)][(int)GET_CLASS(ch)];

  base -= MAX(0, (int)((GET_DEX(ch) - 13) / 2));
#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: Thaco(%s) = %d\r\n", GET_NAME(ch), base);
    send_to_char(buf, ch);
  }
#endif

  return base;
  
/*
  int thac, base = 25,  classType = ( GET_CLASS(ch) <= CLASS_WARRIOR ? 3 : (GET_CLASS(ch) <= CLASS_PALADIN ? 2 : 1 ));
 
  // DM - TODO - Tali decide on the thac0 system - before it was just
  // 25 - level - see below
  // TALI - Yeah, I see. Adjusted formula: BASE(25) - (Level/ClassType(1 | 2 | 3) * Modifier)
  // --> Favours remorted chars, and those with high modifiers who will need the Thac0. <--
  if (IS_NPC(ch))
      thac =20;
  else {
      / *thac = 25-GET_LEVEL(ch);* /
        thac = (int) (base - (GET_LEVEL(ch)/classType) ); 
  }
 
  thac -= str_app[STRENGTH_AFF_APPLY_INDEX(ch)].tohit;
  // Int & Wis? anyhow apply dex ...
  //thac -= (int) ((GET_AFF_INT(ch) - 13) / 2);   // Intelligence helps!
  //thac -= (int) ((GET_AFF_WIS(ch) - 13) / 2);   // So does wisdom
  thac -= (int) (GET_AFF_DEX(ch));   // Dex helps
 
  return thac;
  */
}  


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
//
//
//Roll NUM_STAT_CREATION_ROLLS complete sets of stats, choose best and copy it to the real character abilitisreal_abilsgive to 
//char.
//each stat is rolled as best 2 of 3d6 + 5
// Following now obsolete - DM 20/7/2002
//and they are given 6 points (in pool) to distribute 
//Allow char to remove stats at the transfer cost of -2/+1 to stat pool...
//

#define NUM_STAT_CREATION_ROLLS 3

void roll_real_abils(struct char_data * ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[3];

  struct char_ability_data sets[NUM_STAT_CREATION_ROLLS];
  
  for (int t = 0; t < NUM_STAT_CREATION_ROLLS; t++) {
    for (i = 0; i < 6; i++)
      table[i] = 0;

    for (i = 0; i < 6; i++) {

      for (j = 0; j < 3; j++)
        rolls[j] = number(1, 6);

      temp = rolls[0] + rolls[1] + rolls[2] -
	MIN(rolls[0], MIN(rolls[1], rolls[2])) + 5;

      // sorts table in acsending order
      for (k = 0; k < 6; k++)
        if (table[k] < temp) {
	  temp ^= table[k];
	  table[k] ^= temp;
	  temp ^= table[k];
        }
    }

    basic_mud_log("roll %d of %d set of stats:", t, NUM_STAT_CREATION_ROLLS);
    basic_mud_log("table[0] = %d", table[0]);
    basic_mud_log("table[1] = %d", table[1]);
    basic_mud_log("table[2] = %d", table[2]);
    basic_mud_log("table[3] = %d", table[3]);
    basic_mud_log("table[4] = %d", table[4]);
    basic_mud_log("table[5] = %d", table[5]);
    basic_mud_log("Sum = %d", table[0] + table[1] + table[2] + table[3] + table[4] + table[5]);

    switch (GET_CLASS(ch)) {
    case CLASS_MAGIC_USER:
      sets[t].intel = MIN(table[0], max_stat_value(ch, STAT_INT));
      sets[t].wis = MIN(table[1], max_stat_value(ch, STAT_WIS));
      sets[t].dex = MIN(table[2], max_stat_value(ch, STAT_DEX));
      sets[t].str = MIN(table[3], max_stat_value(ch, STAT_STR));
      sets[t].con = MIN(table[4], max_stat_value(ch, STAT_CON));
      sets[t].cha = MIN(table[5], max_stat_value(ch, STAT_CHA));
      break;
    case CLASS_CLERIC:
      sets[t].wis = MIN(table[0], max_stat_value(ch, STAT_WIS));
      sets[t].con = MIN(table[1], max_stat_value(ch, STAT_CON));
      sets[t].intel = MIN(table[2], max_stat_value(ch, STAT_INT));
      sets[t].str = MIN(table[3], max_stat_value(ch, STAT_STR));
      sets[t].dex = MIN(table[4], max_stat_value(ch, STAT_DEX));
      sets[t].cha = MIN(table[5], max_stat_value(ch, STAT_CHA));
      break;
    case CLASS_THIEF:
      sets[t].dex = MIN(table[0], max_stat_value(ch, STAT_DEX));
      sets[t].intel = MIN(table[1], max_stat_value(ch, STAT_INT));
      sets[t].str = MIN(table[2], max_stat_value(ch, STAT_STR));
      sets[t].con = MIN(table[3], max_stat_value(ch, STAT_CON));
      sets[t].wis = MIN(table[4], max_stat_value(ch, STAT_WIS));
      sets[t].cha = MIN(table[5], max_stat_value(ch, STAT_CHA));
      break;
    case CLASS_WARRIOR:
      sets[t].str = MIN(table[0], max_stat_value(ch, STAT_STR));
      sets[t].con = MIN(table[1], max_stat_value(ch, STAT_CON));
      sets[t].dex = MIN(table[2], max_stat_value(ch, STAT_DEX));
      sets[t].wis = MIN(table[3], max_stat_value(ch, STAT_WIS));
      sets[t].intel = MIN(table[4], max_stat_value(ch, STAT_INT));
      sets[t].cha = MIN(table[5], max_stat_value(ch, STAT_CHA));
      break;
    }
  }

  // find the roll with the greatest sum ...
  int max = 0, maxcount = 0;
  for (int t = 0; t < NUM_STAT_CREATION_ROLLS; t++) {
    int count = sets[t].wis + sets[t].intel + sets[t].str + 
	    sets[t].dex + sets[t].cha + sets[t].con;
    if (count > maxcount) {
      max = t;
      maxcount = count;
    }
  }
  
  ch->real_abils.str_add = 0;
  if (ch->real_abils.str == 18)
    ch->real_abils.str_add = number(0, 100);

  basic_mud_log("Best roll set of stats:");
  basic_mud_log("str = %d", sets[max].str);
  basic_mud_log("int = %d", sets[max].intel); 
  basic_mud_log("wis = %d", sets[max].wis);
  basic_mud_log("cha = %d", sets[max].cha);
  basic_mud_log("con = %d", sets[max].con);
  basic_mud_log("dex = %d", sets[max].dex);
  basic_mud_log("Sum = %d", sets[max].str + sets[max].intel + sets[max].wis + sets[max].dex + sets[max].cha + sets[max].con);

  ch->real_abils.wis = sets[max].wis;
  ch->real_abils.intel= sets[max].intel;
  ch->real_abils.str= sets[max].str;
  ch->real_abils.dex = sets[max].dex;
  ch->real_abils.con = sets[max].con;
  ch->real_abils.cha = sets[max].cha;

  ch->player_specials->saved.stat_pool = 6;
  ch->aff_abils = ch->real_abils;
}

/* equip the newbies :) D. */
// TODO: if we can be bothered, actually equip (make the newbie wear the items)
void newbie_equip(struct char_data * ch)
{
  struct obj_data *obj;
  int vnum,counter = 0;

  int clericeq[5] =  { 1379, 1380, 1381, 1382, 1383 };
  int mageeq[5] =    { 1369, 1370, 1371, 1372, 1373 };
  int warrioreq[5] = { 1374, 1375, 1376, 1377, 1378 }; 
  int thiefeq[5] =   { 1384, 1385, 1386, 1387, 1388 };

  //               ---- bread ----   skin  atlas  map
  int equip[6] = { 1101, 1101, 1101, 1126, 1351, 1350 };
         
  // Equip class specific items 
  for (counter = 0; counter < 5; counter++) {
    switch (GET_CLASS(ch)) {
    case CLASS_CLERIC:
      vnum = clericeq[counter];
      break;
    case CLASS_WARRIOR:
      vnum = warrioreq[counter];
      break;
    case CLASS_MAGIC_USER:
      vnum = mageeq[counter];
      break;
    case CLASS_THIEF:
      vnum = thiefeq[counter];
      break;
    default:
      vnum = -1;
      break;
    }
    if ((obj = read_object(vnum, VIRTUAL))) 
      obj_to_char(obj,ch, __FILE__, __LINE__);
  }
  
  // Equip general items
  for (counter = 0; counter < 6; counter++) {
    vnum = equip[counter];
    if ((obj = read_object(vnum, VIRTUAL))) 
      obj_to_char(obj,ch, __FILE__, __LINE__);
  }

  sprintf(buf, "Equiped Newbie: %s", GET_NAME(ch));
  basic_mud_log(buf);
} 

/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

//  set_title(ch, NULL);
//  roll_real_abils(ch); <- Not needed with re-roll prompts on char creation
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

  for (int i = 0; i < NUM_WORLDS; i++)
    ENTRY_ROOM(ch, i) = NOWHERE;
  START_WORLD(ch) = WORLD_MEDIEVAL;

  /* JA Set some flags to help newbies */
  GET_GOLD(ch) = 10000;
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
  void add_to_immlist(char *name, long idnum, long immkills, ubyte unholiness);

  add_hp = con_app[GET_CON(ch)].hitp;

  switch (GET_CLASS(ch))
  {
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
      add_mana = number(3, 5);//0;
      add_move = number(1, 3);
      break;
    case CLASS_WARRIOR:
      add_hp += number(10, 15);
      add_mana = number(3, 4);//0;
      add_move = number(1, 3);
      break;
    case CLASS_DRUID: // m-c
      add_hp += number(6, 12);
      add_mana = number(GET_LEVEL(ch), (int) (1.75 * GET_LEVEL(ch)));
      add_mana = MIN(add_mana, 10);
      add_move = number(0, 2);
      break;
    case CLASS_PRIEST: // t-c
      add_hp += number(8, 13);
      add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
      add_mana = MIN(add_mana, 10);
      add_move = number(1, 3);
      break;
    case CLASS_NIGHTBLADE: // w-t
      add_hp += number(12, 17);
      add_mana = number(4, 5);
      add_move = number(2, 4);
      break;
    case CLASS_BATTLEMAGE: // w-m
      add_hp += number(10, 15);
      add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
      add_mana = MIN(add_mana, 10);
      add_move = number(1, 3);
      break;
    case CLASS_SPELLSWORD: // m-t
      add_hp += number(8, 13);
      add_mana = number(GET_LEVEL(ch), (int) (1.6 * GET_LEVEL(ch)));
      add_mana = MIN(add_mana, 10);
      add_move = number(1, 3);
      break;
    case CLASS_PALADIN: // w-c
      add_hp += number(11, 16);
      add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
      add_mana = MIN(add_mana, 10);
      add_move = number(1, 3);
      break;
    case CLASS_MASTER:
      add_hp += number(13, 19);
      add_mana = number(GET_LEVEL(ch), (int) (1.75 * GET_LEVEL(ch)));
      add_mana = MIN(add_mana, 10);
      add_move = number(2, 4);
      break;
  }
  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);

  /*
  if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
    GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
  else
    GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));
  */

  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    if (GET_LEVEL(ch) < LVL_ANGEL)
      add_to_immlist(GET_NAME(ch), GET_IDNUM(ch), GET_IMMKILLS(ch),
	             GET_UNHOLINESS(ch));
  }

  save_char(ch, NOWHERE);

  *buf = '\0';
  int cmd_num = 0;
  // Show the character their new commands
  while (*cmd_info[cmd_num].command != '\n')
  {
    if (cmd_info[cmd_num].minimum_level == GET_LEVEL(ch) && GET_LEVEL(ch) > 1)
    {
        strcat(buf, " ");
	strcat(buf, cmd_info[cmd_num].command); 
    }
    cmd_num++;
  }
 
  if (strlen(buf) > 1)
  {
    send_to_char("&GYou have some new commands available!&g\r\n", ch);
    send_to_char(buf, ch);
    send_to_char("&n\r\n", ch);
  }


  // Give 1 stat point per level for lvls 1-49, 2 for levels 50-74, 3 for level 75-100
  if (GET_LEVEL(ch) > 1)
  {
    int points = 1;
    if (GET_LEVEL(ch) >= (int)(LVL_CHAMP*0.75))
      points = 3;
    else if (GET_LEVEL(ch) >= (int)(LVL_CHAMP*0.5))
      points = 2;
    GET_STAT_POINTS(ch) += points;
    sprintf(buf, "&gYou gain &c%d&g stat point%s.\r\n", points, 
	    (points > 1) ? "s" : "");
    send_to_char(buf, ch);
  }

  sprintf(buf, "&7%s&g advanced to level &c%d&g", GET_NAME(ch), GET_LEVEL(ch));
  sprintf(buf2, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf2, BRF, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
  info_channel(buf , ch );
}

/* DM_demote Loose maximum in various points */
void demote_level(struct char_data * ch,int newlevel, const char *reason)
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
  if (GET_LEVEL(ch) < LVL_ETRNL2)
    REMOVE_BIT(PRF_FLAGS(ch), PRF_ROOMFLAGS);
 
  /* holylight */
  if (GET_LEVEL(ch) < LVL_ETRNL4)
    REMOVE_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
 
  /* invis */
  if (GET_LEVEL(ch) < LVL_ANGEL)
    GET_INVIS_LEV(ch) = 0;
 
  /* syslog */
  if (GET_LEVEL(ch) < LVL_GOD)
    REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
 
  /* nohassle, hunger, thirst, drunk */
  if (GET_LEVEL(ch) < LVL_CHAMP)
  {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_NOHASSLE);
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
  }
  save_char(ch, NOWHERE); 

  sprintf(buf, "&G&7%s&g demoted to level &c%d&g for %s", GET_NAME(ch), 
                  GET_LEVEL(ch), reason);
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
  if (level <= 0)              return 1;	  /* level 0 */
  else if (level <= 10)        return 2;	  /* level 1 - 10 */
  else if (level <= 20)        return 3;	  /* level 11 - 20 */
  else if (level <= 30)        return 4;	  /* level 21 - 30 */
  else if (level <= 40)        return 5;	  /* level 31 - 40 */
  else if (level <= 50)        return 6;	  /* level 41 - 50 */
  else if (level <= 60)        return 7;	  /* level 51 - 60 */
  else if (level <= 70)        return 8;	  /* level 61 - 70 */
  else if (level <= 80)        return 9;	  /* level 71 - 80 */
  else if (level <= 90)        return 10;	  /* level 81 - 90 */
  else if (level < LVL_CHAMP)  return 11;	  /* level 91 - 99 */
  else if (level < LVL_IMMORT) return 15;         /* level 100-104 */
  else
    return 20;	  /* IMMS and stuff */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 *
 * Artus> Changed from a big mess of || to a nice clean switch.
 */

int invalid_class(struct char_data *ch, struct obj_data *obj) 
{
  if (IS_NPC(ch))
    return 0;
  switch GET_CLASS(ch)
  {
    case CLASS_MAGIC_USER: return IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER);
    case CLASS_CLERIC:     return IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC);
    case CLASS_THIEF:      return IS_OBJ_STAT(obj, ITEM_ANTI_THIEF);
    case CLASS_WARRIOR:    return IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR);
    case CLASS_DRUID:      return IS_OBJ_STAT(obj, ITEM_ANTI_DRUID);
    case CLASS_PRIEST:     return IS_OBJ_STAT(obj, ITEM_ANTI_PRIEST);
    case CLASS_NIGHTBLADE: return IS_OBJ_STAT(obj, ITEM_ANTI_NIGHTBLADE);
    case CLASS_BATTLEMAGE: return IS_OBJ_STAT(obj, ITEM_ANTI_BATTLEMAGE);
    case CLASS_SPELLSWORD: return IS_OBJ_STAT(obj, ITEM_ANTI_SPELLSWORD);
    case CLASS_PALADIN:    return IS_OBJ_STAT(obj, ITEM_ANTI_PALADIN);
    case CLASS_MASTER:     return IS_OBJ_STAT(obj, ITEM_ANTI_MASTER);
  }
  sprintf(buf, "SYSERR: Wormed way to end of invalid_class. (GET_CLASS(ch): %d)\r\n", GET_CLASS(ch));
  mudlog(buf, NRM, LVL_IS_GOD, TRUE);
  return 0;
}



#if 0 
// Artus> I wish people would #def out functions they make obsolete... 
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

// Artus> TODO - Stats for battlecry (SKILL_BATTLECRY)

// Artus> TODO - Stats for bearhug (SKILL_BEARHUG)

// Artus> TODO - Stats for berserk (SKILL_BERSERK)
  
// bless                 
  spell_level(SPELL_BLESS, CLASS_MAGIC_USER, 5, 0, 0, 9, 0, 0, 0);

// blindness             
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 6, 0, 0, 12, 0, 0, 0);

// Artus> TODO - Stats for bodyslam (SKILL_BODYSLAM)
  
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

//remove curse          (superb)  40  Wis: 14
  spell_level(SPELL_GREATER_REMOVE_CURSE, CLASS_MAGIC_USER, 40, 0, 0, 14, 0, 0, 0);

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
#endif

/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  1000000000

/* Function to return the exp required for each class/level */
int level_exp(struct char_data *ch, int level)
{
  // Invalid Levels (Outside 0..LVL_OWNER)
  if (level > LVL_OWNER || level < 0)
  {
    basic_mud_log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }
  // Maximum reachable level.
  if (level >= LVL_IMMORT)
    return 0;
  // Big ass array.
  return exp_reqs[level];
#if 0 // Artus> Redundant, All the useful stuff was commented out.
  int exp = exp_reqs[level];
//  sprintf(buf,"exp=%d  modifier=%f newexp=%d",exp,GET_MODIFIER(ch),(int)(exp*GET_MODIFIER(ch)));
//  basic_mud_log(buf);

  // DM: moved exp modification into gain_exp
  return (int)(exp);//*GET_MODIFIER(ch));

  /*
   * This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete -- so, complete them!
   */
  basic_mud_log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
#endif
}


#if 0 // Artus> Redundant.
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
      case LVL_IMMOR: return "the Immortal Warlock";
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
#endif
