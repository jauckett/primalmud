/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
    Modified by Brett Murphy
*/

#include "structs.h"

const char circlemud_version[] = {
"PrimalMUD, version 2.00 [A200309031613]\r\n"};


/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */


/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "\n"
};


/* ROOM_x */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "!MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "!TRACK",
  "!MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "NEWBIE",
  "LR_5",
  "LR_10",
  "LR_15",
  "LR_20",
  "LR_25",
  "LR_30",
  "LR_ETERNAL",
  "REGENx2",
  "REGEN/2",
  "NOSLEEP",
  "LR_IMMORT",
  "LR_IMPL",
  "LR_35",
  "\n"
};


/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "\n"
};


/* SECT_ */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "Underwater",
  "In Flight",
  "\n"
};


/* SEX_x */
const char *genders[] =
{
  "Neutral",
  "Male",
  "Female"
};


/* POS_x */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};


/* PLR_x */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "!WIZL",
  "!DEL",
  "INVST",
  "CRYO",
  "!IGNORE",
  "SPECIAL",
  "GODMAIL",
  "\n"
};


/* MOB_x */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "!CHARM",
  "!SUMMN",
  "!SLEEP",
  "!BASH",
  "!BLIND",
  "QUEST",
  "INTEL",
  "2ND",
  "3RD",
  "!STEAL",
  "MOUNT", 
  "\n"
};


/* PRF_x */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "!TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "!HASS",
  "QUEST",
  "SUMN",
  "!REP",
  "LIGHT",
  "C1",
  "C2",
  "!WIZ",
  "L1",
  "L2",
  "!AUC",
  "!GOS",
  "!GTZ",
  "RMFLG",
  "!IMMN",
  "AFK",
  "WOLF",
  "VAMPIRE",
  "TAG",
  "MORTALK",
  "D_EXP",
  "D_ALIGN",
  "!INFO",
  "\n"
};

/* EXT_x */
const char *extended_bits[] = {
	"!NEWBIE",
	"!CTALK",
        "CLAN",
        "SUBLEADER",
	"LEADER",
        "AUTOGOLD",
        "AUTOLOOT",
	"AUTOSPLIT",
	"\n"
};

/* AFF_x */
const char *affected_bits[] =
{
  "BLIND",
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "!TRACK",
  "REFLECT",
  "FLY",
  "SNEAK",
  "HIDE",
  "PARALYZED",
  "CHARM",
  "WATERBREATHE",
  "HASTE",
  "ADV-INVIS",
  "DET-ADV-INVIS",
  "NOHASSLE",
  "BROKEN_IN",
  "\n"
};


/* CON_x */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Select Stats",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Selecting Race",
  "On Line Editing",
  "\n"
};


/* WEAR_x - for eq list */
const char *where[] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<held>               "
};


/* WEAR_x - for stat */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "\n"
};

/* ITEM_x (ordinal object types) */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "MAGIC EQ",
  "JOINABLE",
  "RIDEABLE",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "NO_GOOD",
  "NO_EVIL",
  "NO_NEUTRAL",
  "!MAGE",
  "!CLERIC",
  "!THIEF",
  "!WARRIOR",
  "NO_SELL",
  "RESERVED", /* Bits 18 - 24 are used for level restrictions */
  "RESERVED",
  "RESERVED",
  "RESERVED",
  "RESERVED",
  "RESERVED",
  "RESERVED", /* Top level restriction bit */
  "NO_HUNGER",
  "NO_THIRST",
  "NO_DRUNK",
  "LR_35",
  "LR_ETERNAL",
  "LR_40",
  "LR_45",
  "LR_50",
  "LR_55",
  "LR_60",
  "LR_65",
  "LR_70",
  "LR_75",
  "LR_80",
  "LR_85",
  "LR_90",
  "LR_95",
  "LR_IMPL",
  "\n"
};

/* ZONE_x (flag bits) bm 12/94*/
const char *zone_flagbits[] = {
  "PKILL",
  "NEWB",
  "LR_5",
  "LR_10",
  "LR_15",
  "LR_20",
  "LR_25",
  "LR_30",
  "LR_35",
  "LR_40",
  "LR_45",
  "LR_50",
  "LR_55",
  "LR_60",
  "LR_65",
  "LR_70",
  "LR_75",
  "LR_80",
  "LR_85",
  "LR_90",
  "LR_95",
  "LR_ET",
  "LR_IMM",
  "LR_IMP",
  "!STEAL",
  "!TELEPORT",
  "\n"
};


/* APPLY_x */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "\n"
};


/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "champagne",
  "\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "champagne",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
const int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13},
  {3, 0, 2}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "golden yellow"
};


/* level of fullness for drink containers */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
const struct str_app_type str_app[35] = {
  {-5, -4, 0, 0},    /* 0  */
  {-5, -4, 3, 1},    /* 1  */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},   /* 5  */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},   /* 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 170, 14},
  {0, 0, 170, 15},   /* 15 */
  {0, 1, 195, 16},
  {1, 1, 220, 18},
  {1, 2, 255, 20},   /* 18 */
  {3, 7, 640, 40},
  {3, 8, 700, 40},   /* 20 */
  {4, 9, 810, 40},
  {4, 10, 970, 40},
  {5, 11, 1130, 40},
  {6, 12, 1440, 40},
  {7, 14, 1750, 40}, /* 25 */
  {1, 3, 280, 22},   /* 18/00 - 18/50 */
  {2, 3, 305, 24},   /* 18/51 - 18/75 */
  {2, 4, 330, 26},   /* 18/76 - 18/90 */
  {2, 5, 380, 28},   /* 18/91 - 28/99 */
  {3, 6, 480, 30},   /* 18/100 */
  {3, 6, 480, 30},  
  {3, 6, 480, 30},
  {3, 6, 480, 30},
  {3, 6, 480, 30}    /* 18/100   (34) */
};



/* [dex] skill apply (thieves only) */
const struct dex_skill_type dex_app_skill[36] = {
  {-99, -99, -90, -99, -60},
  {-90, -90, -60, -90, -50},
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {30, 30, 20, 25, 25},
  {30, 30, 20, 25, 25},
  {30, 35, 20, 30, 30},
  {30, 35, 20, 30, 30},
  {35, 35, 25, 30, 30},
  {35, 35, 25, 35, 35},
  {35, 35, 25, 35, 35},
  {35, 40, 25, 35, 35},
  {35, 40, 30, 35, 35},
  {40, 40, 30, 35, 35}		/* 35 */
};

/* [dex] apply (all) */
struct dex_app_type dex_app[36] = {
  {-7, -7, 6},
  {-6, -6, 5},
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},
  {3, 3, -4},
  {3, 3, -4},
  {4, 4, -5},
  {4, 4, -5},
  {4, 4, -5},
  {5, 5, -6},
  {5, 5, -6},
  {5, 5, -6},
  {5, 5, -6},
  {6, 6, -7},
  {6, 6, -7},
  {6, 6, -7},
  {6, 6, -7},
  {7, 7, -8},
  {7, 7, -8},
  {7, 7, -8},
  {7, 7, -6}			/* 35 */
};



/* [con] apply (all) */
struct con_app_type con_app[36] = {
  {-4, 20},
  {-3, 25},
  {-2, 30},
  {-2, 35},
  {-1, 40},
  {-1, 45},
  {-1, 50},
  {0, 55},
  {0, 60},
  {0, 65},
  {0, 70},
  {0, 75},
  {0, 80},
  {0, 85},
  {0, 88},
  {1, 90},
  {2, 95},
  {2, 97},
  {3, 99},
  {3, 99},
  {4, 99},
  {5, 99},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99},
  {6, 100},
  {7, 100},
  {7, 100},
  {7, 100},
  {7, 100},
  {8, 100},
  {8, 100},
  {8, 100},
  {8, 100},
  {9, 100}			/* 35 */
};



/* [int] apply (all) */
struct int_app_type int_app[36] = {
  {3},
  {5},				/* 1 */
  {7},
  {8},
  {9},
  {10},				/* 5 */
  {11},
  {12},
  {13},
  {15},
  {17},				/* 10 */
  {19},
  {22},
  {25},
  {30},
  {35},				/* 15 */
  {40},
  {45},
  {50},
  {53},
  {55},				/* 20 */
  {56},
  {57},
  {58},
  {59},
  {60},				/* 25 */
  {61},
  {62},
  {63},
  {64},
  {65},				/* 30 */
  {66},
  {69},
  {70},
  {80},
  {99}				/* 35 */
};


/* [wis] apply (all) */
struct wis_app_type wis_app[36] = {
  {0},				/* 0 */
  {0},				/* 1 */
  {0},
  {0},
  {0},
  {0},				/* 5 */
  {0},
  {0},
  {0},
  {0},
  {0},				/* 10 */
  {0},
  {1},
  {1},
  {2},
  {2},				/* 15 */
  {3},
  {3},
  {4},				/* 18 */
  {4},
  {5},				/* 20 */
  {5},
  {0},
  {0},
  {0},
  {0},				/* 25 */
  {0},
  {0},
  {0},
  {0},
  {0},				/* 30 */
  {0},
  {0},
  {0},
  {0},
  {0}				/* 35 */
};



const char *spell_wear_off_msg[] = {
  "RESERVED DB.C",				/* 0 */
  "You feel less protected.",	
  "!Teleport!",
  "You feel less righteous.",
  "You feel a cloak of blindness disolve.",
  "!Burning Hands!",				/* 5 */
  "!Call Lightning",
  "You feel more self-confident.",
  "You feel your strength return.",
  "!Clone!",
  "!Color Spray!",				/* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",				/* 15 */
  "!Cure Light!",
  "You feel more optimistic.",
  "You feel less aware.",
  "Your eyes stop tingling.",
  "The detect magic wears off.",		/* 20 */
  "The detect poison wears off.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",				/* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal!",
  "You feel yourself exposed.",
  "!Lightning Bolt!",				/* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "You feel less sick.",
  "You feel less invulnerable.",
  "!Remove Curse!",				/* 35 */
  "The white aura around your body fades.",
  "!Shocking Grasp!",
  "You feel less tired.",
  "You feel weaker.",
  "!Summon!",					/* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your suroundings.",
  "!Animate Dead!",				/* 45 */
  "!Dispel Good!",
  "!Group Armor!",
  "!Group Heal!",
  "!Group Recall!",
  "Your night vision seems to fade.",		/* 50 */
  "Your feet seem less boyant.",
  "!Finger Death!",
  "!Adv Heal!",
  "!Refresh!",
  "!Fear!",					/* 55 */
  "!Dimensional Gate!",
  "!Meteor Swarm!",
  "!Group Sanct!",
  "Your skin ceases to sparkle.",
  "Gravity regains its grip on you...",		/* 60 */
  "!Plasma Blast!",
  "!Cloud Kill!",
  "!Paralyze!",
  "You stumble slightly as your legs begin to function again.",
  "Your magical gills suddenly vanish..", 	/* 65 */
  "!wraith touch!",
  "Your revert back to your true form.",
  "!Mana!",
  "!Whirlwind!",
  "The spirits leave you less protected.", 	/* 70 */
  "Your god stops helping you.",
  "Your deity stops protecting you.",
  "Your body slows down to normal pace.",
  "Your untouchability fades away.",
  "A skin of stone peels away from you.",	/* 75 */
  "!Divine Heal!",
  "The lightning upon your shield fades away.",
  "The flaming fire of your shield burns out.", 
  "Your surrounding wall of fire slowely burns away.",
  "You feel less invulnerable."			/* 80 */
};



const char *npc_class_types[] = {
  "Normal",
  "Undead",
  "\n"
};



const int rev_dir[] =
{
  2,
  3,
  0,
  1,
  5,
  4
};


const int movement_loss[] =
{
  1,				/* Inside     */
  1,				/* City       */
  2,				/* Field      */
  3,				/* Forest     */
  4,				/* Hills      */
  6,				/* Mountains  */
  4,				/* Swimming   */
  1				/* Unswimable */
};


const char *weekdays[7] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the day of the Great Gods",
"the Day of the Sun"};


const char *month_name[17] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};


const int sharp[] = {
  0,
  0,
  0,
  1,				/* Slashing */
  0,
  0,
  0,
  0,				/* Bludgeon */
  0,
  0,
  0,
0};				/* Pierce   */


/* moon messages - Vader */
const char *moon_mesg[] = {
  "!MOONLESS!",
  "new",
  "half",
  "three quarter",
  "full",
  "three quarter",
  "second half",
  "final quarter"
};

