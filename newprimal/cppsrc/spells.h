/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define SPELL	      0
#define SKILL	      1
#define SPELL_BOTH    2		// Some classes will have both skills and spells

#define CAST_UNDEFINED	-1
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4
#define CAST_MAGIC_OBJ  5  

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)


#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD	     45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD	     46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR	     47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL	     48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL	     49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION	     50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK		     51 /* Reserved Skill[] DO NOT CHANGE */
/* Insert new spells here, up to MAX_SPELLS */
#define SPELL_FINGERDEATH		52
#define SPELL_ADV_HEAL			53
#define SPELL_REFRESH			54
#define SPELL_FEAR			55
#define SPELL_GATE			56
#define SPELL_METEOR_SWARM		57
#define SPELL_GROUP_SANCTUARY		58
#define SPELL_SERPENT_SKIN		59
#define SPELL_FLY			60
#define SPELL_PLASMA_BLAST		61
#define SPELL_CLOUD_KILL		62
#define SPELL_PARALYZE			63
#define SPELL_REMOVE_PARA		64
#define SPELL_WATERBREATHE		65
#define SPELL_WRAITH_TOUCH		66
#define SPELL_DRAGON			67
#define SPELL_MANA			68
#define SPELL_WHIRLWIND			69
#define SPELL_SPIRIT_ARMOR		70
#define SPELL_HOLY_AID			71
#define SPELL_DIVINE_PROTECTION		72
#define SPELL_HASTE			73
#define SPELL_NOHASSLE			74
#define SPELL_STONESKIN			75
#define SPELL_DIVINE_HEAL		76
#define SPELL_LIGHT_SHIELD		77
#define SPELL_FIRE_SHIELD		78
#define SPELL_FIRE_WALL			79
#define SPELL_PROT_FROM_GOOD		80
#define SPELL_IDENTIFY			81
#define SPELL_GREATER_REMOVE_CURSE      82
#define SPELL_SENSE_WOUNDS		83 /* Artus> Detect Wounded PCs */
#define SPELL_UNHOLY_VENGEANCE		84
#define NUM_SPELLS		        84 // From oasis.h

/* Were critter "spell" .. hacky code you have here, Vader */
#define MAX_SPELLS		    230

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB              231 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  232 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  233 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  234 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             235 /* Reserved Skill[] DO NOT CHANGE */
// DM - PUNCH not used ...
#define SKILL_PUNCH                 236 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                237 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 238 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 239 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		    240 /* Reserved Skill[] DO NOT CHANGE */
/* New skills may be added here up to MAX_SKILLS (300) */
#define SKILL_2ND_ATTACK            241 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_3RD_ATTACK            242 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PRIMAL_SCREAM         243 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SCAN                  244 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HUNT                  245 /*    "       "      "  "    "    */
#define SKILL_RETREAT               246 /*    "       "      "  "     "  */
#define SKILL_THROW                 247 /*    "       "      "  "     "  */
#define SKILL_COMPARE               248 /* change it n ill slap ya,..   */
#define SKILL_SPY                   249 /* DM - spy skill               */
#define SKILL_SEARCH		    250
#define SKILL_DETECT_DEATH	    251
#define SKILL_ARMOURCRAFT	    252	
#define SKILL_AMBUSH                253
#define SKILL_ATTEND_WOUNDS         254
#define SKILL_ADRENALINE            255
#define SKILL_BATTLECRY             256
#define SKILL_BEARHUG               257
#define SKILL_HEADBUTT              258
#define SKILL_PILEDRIVE             259
#define SKILL_TRIP                  260
#define SKILL_BODYSLAM              261

#define SKILL_DISARM                262
#define SKILL_CLOT_WOUNDS           263
#define SKILL_PURSE                 264 // Purse Evaluation 

#define SKILL_MOUNT                     265
#define SKILL_DARKRITUAL                266
#define SKILL_FLYINGTACKLE		267
#define SKILL_FIRST_AID			268
#define SKILL_TUMBLE			269
#define SKILL_BERSERK			270
#define SKILL_COMPOST			271
#define SKILL_TRAP_PIT                  272
#define SKILL_SLIP			273
#define SKILL_MEDITATE			274
#define SKILL_GLANCE			275
#define SKILL_TORCH			276
#define SKILL_SHIELDMASTERY		277
#define SKILL_POWERKICK			278
#define SKILL_LISTEN			279 // Hear Noises => Listen - Artus
#define SKILL_SUICIDE			280
#define SKILL_SLEIGHT			281
#define SKILL_HEAL_TRANCE		282 // Healing Trance
#define SKILL_POISONBLADE		283
#define SKILL_HEALING_MASTERY		284 
#define SKILL_HEALING_EFFICIENCY	285
#define SKILL_BURGLE			286
#define SKILL_SENSE_STATS		287
#define SKILL_SENSE_CURSE		288
#define SKILL_MOUNTAINEER		289
#define SKILL_DOUBLE_KICK		290
#define SKILL_DOUBLE_BACKSTAB		291
#define SKILL_DEFEND			292
#define SKILL_CONCEAL_SPELL_CASTING	293
#define SKILL_CAMPING			294
#define SKILL_BARGAIN			295
#define SKILL_BLADEMASTERY		296
#define SKILL_AXEMASTERY		297
#define SKILL_WEAPONCRAFT		298
#define SKILL_AMBIDEXTERITY		299
#define SKILL_PERCEPTION		300
#define SKILL_AXETHROW			301
#define SKILL_TRAP_MAGIC		302

/* manufacturing skills */
#define SKILL_MANUFACTURE_BASE          340
#define SKILL_TAILORING                 341
#define SKILL_BLACKSMITHING             342
#define SKILL_BREWING                   343
#define SKILL_JEWELRY                   344

/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

// #define SPELL_IDENTIFY               301
#define SPELL_FIRE_BREATH            352
#define SPELL_GAS_BREATH             353
#define SPELL_FROST_BREATH           354
#define SPELL_ACID_BREATH            355
#define SPELL_LIGHTNING_BREATH       356
#define SPELL_SUPERMAN		     357
#define SPELL_CHANGED		     358
#define SPELL_CREAMED		     359

#define TOP_SPELL_DEFINE	     499
/* NEW NPC/OBJECT SPELLS can be inserted here up to 499 */


/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     400
#define TYPE_STING                   401
#define TYPE_WHIP                    402
#define TYPE_SLASH                   403
#define TYPE_BITE                    404
#define TYPE_BLUDGEON                405
#define TYPE_CRUSH                   406
#define TYPE_POUND                   407
#define TYPE_CLAW                    408
#define TYPE_MAUL                    409
#define TYPE_THRASH                  410
#define TYPE_PIERCE                  411
#define TYPE_BLAST		     412
#define TYPE_PUNCH		     413
#define TYPE_STAB		     414

#define TYPE_DUAL_ATTACK             425

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING		     499




#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4


#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF   	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP	(1 << 10)
#define TAR_CHAR_INWORLD (1 << 11)
// DM: Idea - we should have more here for like TAR_NO_MOB ...

struct spell_info_type {
   byte min_position;	/* Position for caster	 */
   int mana_min[NUM_CLASSES]; /* Min amount of mana used by a spell (highest lev) */
   int mana_max[NUM_CLASSES]; /* Max amount of mana used by a spell (lowest lev) */
   int mana_change[NUM_CLASSES]; /* Change in mana used by spell from lev to lev */

   int spell_type;

   int templevel;

   int min_level[NUM_CLASSES];
   int routines;
   byte violent;
   int targets;         /* See below for use with TAR_XXX  */
   const char *name;

   // DM - New spell features (defined for each class)
   sh_int mana_perc[NUM_CLASSES];	/* percentage of mana min/max */
   sh_int spell_effec[NUM_CLASSES];	/* normal is 100 */
 
   sh_int intl[NUM_CLASSES];
   sh_int str[NUM_CLASSES];
   sh_int wis[NUM_CLASSES];
   sh_int con[NUM_CLASSES];
   sh_int dex[NUM_CLASSES];
   sh_int cha[NUM_CLASSES];
};

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


/* Attacktypes with grammar */

struct attack_hit_type {
   const char	*singular;
   const char	*plural;
};


#define ASPELL(spellname) \
void	spellname(int level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);

ASPELL(spell_fingerdeath); 
ASPELL(spell_control_weather);
ASPELL(spell_fear);
ASPELL(spell_gate);
ASPELL(spell_unholy_vengeance);

/* basic magic calling functions */

int find_skill_num(char *name);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

void mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(int level, struct char_data *ch, int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 int spellnum, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
 int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
  int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

int	call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, int spellnum, int level, int casttype);

void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
			char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,
  struct obj_data *tobj, int spellnum);


/* other prototypes */
void spell_level(int spell, int chclass, int level, byte intl, byte str, byte wis, byte con, byte dex, byte cha);
void init_spell_levels(void);
const char *skill_name(int num);

