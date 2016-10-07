/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data */

cpp_extern const char *circlemud_version =
	"PrimalMUD, version 3.00 based on CircleMUD, version 3.00 beta patchlevel 17";

/* strings corresponding to ordinals/bitvectors in structs.h ***********/

const char *sort_names[] = {
  "alphabetical",
  "ascending",
  "descending",
  "\n"
};

/* The world names */
const char *world_names[] = {
  "Medieval",
  "West World",
  "Future World",
  "\n"
};

/* (Note: strings for class definitions in class.c instead of here) */
/* enhancement text names */
const char* enhancement_names[] = {
        "None",
        "Strength",
        "Dexterity",
        "Intelligence",
        "Wisdom",
        "Constitution",
        "Charisma",
        "Class (Unused)",
        "Level (Unused)",
        "Age",
        "Weight",
        "Height",
        "Mana",
        "HitPoints",
        "Movement",
        "Gold (Unused)",
        "Experience (Unused)", 
        "AC",
        "Hitroll",
        "Damroll",
        "Saving_Para",
        "Saving_Rod",
        "Saving_Petri",
        "Saving_Breath",
        "Saving_Spell",
        "\n"
};      

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


const char *special_ability_bits[] = {
	"Perm-Invis",
	"Perm-Sneak",
	"MultiWeapon",
	"ForestPower",
	"ForestHelp",
	"Healer",
	"Priest",
	"CombatBackstab",
	"CombatEnhanced",
	"ManaThief",
	"Holy",
	"Disguise",
	"Escape",
	"PermInfra",
	"DwarfFighter",
	"Perm-GroupSneak",
	"ThiefEnhanced",
	"Gore",
	"Minotaur",
	"Charmer",
	"Superman",
	"Perm-Fly",
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
  "ETERNAL",
  "REGENx2",
  "REGEN/2",
  "!SLEEP",
  "ANGEL",
  "IMMORT",
  "GOD",
  "IMPL",
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


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
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
  "MOUNTED",
  "!INFOCMD",
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
  "INTELLIGENT",
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
  "CLS",
  "!IMMNET",
  "AFK",
  "WEREWOLF",
  "VAMPIRE", 
  "TAG",
  "MORTALKOMBAT",
  "DISPXP",
  "DISPALIGN",
  "!INFO",
  "\n"
};

const char *timer_bits[] = 
{
  "HEALING_SKILL",
  "ADRENALINE",
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
  "ADV_INVIS",
  "DET_ADV_INVIS",
  "!HASSLE",
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
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Select race",
  "Verify stats",
  "\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
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
  "MAGIC_EQ",
  "JOINABLE",
  "BATTERY",
  "QUEST",
  "UNDEFINED",
  "UNDEFINED",
  "RESPIRATOR",
  "BREATHER",
  "VACSUIT",
  "ENVIRON",
  "STASIS",
  "HEAT_RES",
  "HEAT_PROOF",
  "COLD_RES",
  "FREEZE",
  "GRAV",
  "HIGH_GRAV",
  "GATEWAY",
  "RADPROOF",
  "\n"
};

const int default_item_damage[] = {

	0,		// No item at position 0
	5,		// Light
	2,		// Scroll
	4,		// Wand
	7,		// Staff
        50,		// Weapon
	15,		// Fireweapon
	3,		// Missile
	50,		// Treasure
	75,		// Armor
	1,		// Potion
	20,		// Worn		(10)
	20,		// Other
	5,		// Trash
	-1,		// Trap
	25,		// Container
	1,		// Note
	20,		// Drink container
	-1,		// Key
	3,		// Food
	40,		// Money
	8,		// Pen
	35,		// Boat
	-1,		// Fountain
	80,		// Magic EQ
	30,		// Joinable
	25,		// Battery
        -1, 	 	// Empty
	-1,		//   "
	-1, 		//   "
	15,		// Respirator
	15,		// Breather
	35,		// VacSuit
	150,		// Environ
	-1,		// Stasis
	50,		// Heat res
	75,		// Heat proof
	50,		// Cold res
	75,		// Subzero
	30,		// Grav1
	40,		// Grav3
	-1,		// Gateway
	-1		// Rad proof
	
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
  "!RENT",
  "!DONATE",
  "!INVIS",
  "INVISIBLE",
  "MAGIC",
  "!DROP",
  "BLESS",
  "!GOOD",
  "!EVIL",
  "!NEUTRAL",
  "!MAGE",
  "!CLERIC",
  "!THIEF",
  "!WARRIOR",
  "!SELL",
  "RIDDEN",
  "HIDDEN",
  "!DRUID",
  "!PRIEST",
  "!NIGHTBLADE",
  "!BATTLEMAGE",
  "!SPELLSWORD",
  "!HUNGER",
  "!THIRST",
  "!DRUNK",
  "!PALADIN",
  "!MASTER",
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

const char *level_bits[] = {

	"LR_1",
	"LR_2",
	"LR_3",
	"LR_4",
	"LR_5",
	"LR_6",
	"LR_7",
	"LR_8",
	"LR_9",
	"LR_10",
	"LR_11",
	"LR_12",
	"LR_13",
	"LR_14",
	"LR_15",
	"LR_16",
	"LR_17",
	"LR_18",
	"LR_19",
	"LR_20",
	"LR_21",
	"LR_22",
	"LR_23",
	"LR_24",
	"LR_25",
	"LR_26",
	"LR_27",
	"LR_28",
	"LR_29",
	"LR_30",
	"LR_31",
	"LR_32",
	"LR_33",
	"LR_34",
	"LR_35",
	"LR_36",
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
int drink_aff[][3] = {
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
  {0, 0, 13}
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
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
cpp_extern const struct str_app_type str_app[] = {
  {-5, -4, 0, 0},	/* str = 0 */
  {-5, -4, 3, 1},	/* str = 1 */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},	/* str = 5 */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},	/* str = 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 170, 14},
  {0, 0, 170, 15},	/* str = 15 */
  {0, 1, 195, 16},
  {1, 1, 220, 18},
  {1, 2, 255, 20},	/* str = 18 */
  {3, 7, 640, 40},
  {3, 8, 700, 40},	/* str = 20 */
  {4, 9, 810, 40},
  {4, 10, 970, 40},
  {5, 11, 1130, 40},
  {6, 12, 1440, 40},
  {7, 14, 1750, 40},	/* str = 25 */
  {1, 3, 280, 22},	/* str = 18/0 - 18-50 */
  {2, 3, 305, 24},	/* str = 18/51 - 18-75 */
  {2, 4, 330, 26},	/* str = 18/76 - 18-90 */
  {2, 5, 380, 28},	/* str = 18/91 - 18-99 */
  {3, 6, 480, 30}	/* str = 18/100 */
};

/* 
struct dex_skill_type {
  sh_int p_pocket;
  sh_int p_locks;
  sh_int traps;
  sh_int sneak;
  sh_int hide;
  sh_int ambush;
}; */     

/* [dex] skill apply (thieves only) */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
  {-99, -99, -90, -99, -60, -99},	/* dex = 0 */
  {-90, -90, -60, -90, -50, -90},	/* dex = 1 */
  {-80, -80, -40, -80, -45, -80},
  {-70, -70, -30, -70, -40, -70},
  {-60, -60, -30, -60, -35, -60},
  {-50, -50, -20, -50, -30, -50},	/* dex = 5 */
  {-40, -40, -20, -40, -25, -40},
  {-30, -30, -15, -30, -20, -30},
  {-20, -20, -15, -20, -15, -20},
  {-15, -10, -10, -20, -10, -15},
  {-10, -5, -10, -15, -5, -10},	/* dex = 10 */
  {-5, 0, -5, -10, 0, -5},
  {0, 0, 0, -5, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},		/* dex = 15 */
  {0, 5, 0, 0, 0, 0},
  {5, 10, 0, 5, 5, 5},
  {10, 15, 5, 10, 10, 5},		/* dex = 18 */
  {15, 20, 10, 15, 15, 10},
  {15, 20, 10, 15, 15, 10},		/* dex = 20 */
  {20, 25, 10, 15, 20, 15},
  {20, 25, 15, 20, 20, 15},
  {25, 25, 15, 20, 20, 15},
  {25, 30, 15, 25, 25, 20},
  {25, 30, 15, 25, 25, 20}		/* dex = 25 */
};



/* [dex] apply (all) */
cpp_extern const struct dex_app_type dex_app[] = {
  {-7, -7, 6},		/* dex = 0 */
  {-6, -6, 5},		/* dex = 1 */
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},		/* dex = 5 */
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},		/* dex = 15 */
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},		/* dex = 18 */
  {3, 3, -4},
  {3, 3, -4},		/* dex = 20 */
  {4, 4, -5},
  {4, 4, -5},
  {4, 4, -5},
  {5, 5, -6},
  {5, 5, -6}		/* dex = 25 */
};



/* [con] apply (all) */
cpp_extern const struct con_app_type con_app[] = {
  {-4, 20},		/* con = 0 */
  {-3, 25},		/* con = 1 */
  {-2, 30},
  {-2, 35},
  {-1, 40},
  {-1, 45},		/* con = 5 */
  {-1, 50},
  {0, 55},
  {0, 60},
  {0, 65},
  {0, 70},		/* con = 10 */
  {0, 75},
  {0, 80},
  {0, 85},
  {0, 88},
  {1, 90},		/* con = 15 */
  {2, 95},
  {2, 97},
  {3, 99},		/* con = 18 */
  {3, 99},
  {4, 99},		/* con = 20 */
  {5, 99},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99}		/* con = 25 */
};



/* [int] apply (all) */
cpp_extern const struct int_app_type int_app[] = {
  {3},		/* int = 0 */
  {5},		/* int = 1 */
  {7},
  {8},
  {9},
  {10},		/* int = 5 */
  {11},
  {12},
  {13},
  {15},
  {17},		/* int = 10 */
  {19},
  {22},
  {25},
  {30},
  {35},		/* int = 15 */
  {40},
  {45},
  {50},		/* int = 18 */
  {53},
  {55},		/* int = 20 */
  {56},
  {57},
  {58},
  {59},
  {60}		/* int = 25 */
};


/* [wis] apply (all) */
cpp_extern const struct wis_app_type wis_app[] = {
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {0},  /* wis = 10 */
  {0},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {3},
  {4},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {6},
  {6},
  {7},
  {7},
  {7}  /* wis = 25 */
};



const char *spell_wear_off_msg[] = {
  "RESERVED DB.C",		/* 0 */
  "You feel less protected.",	/* 1 */
  "!Teleport!",
  "You feel less righteous.",
  "You feel a cloak of blindness disolve.",
  "!Burning Hands!",		/* 5 */
  "!Call Lightning",
  "You feel more self-confident.",
  "You feel your strength return.",
  "!Clone!",
  "!Color Spray!",		/* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",		/* 15 */
  "!Cure Light!",
  "You feel more optimistic.",
  "You feel less aware.",
  "Your eyes stop tingling.",
  "The detect magic wears off.",/* 20 */
  "The detect poison wears off.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",		/* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal!",
  "You feel yourself exposed.",
  "!Lightning Bolt!",		/* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "You feel less sick.",
  "You feel less protected.",
  "!Remove Curse!",		/* 35 */
  "The white aura around your body fades.",
  "!Shocking Grasp!",
  "You feel less tired.",
  "You feel weaker.",
  "!Summon!",			/* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your suroundings.",
  "!Animate Dead!",		/* 45 */
  "!Dispel Good!",
  "!Group Armor!",
  "!Group Heal!",
  "!Group Recall!",
  "Your night vision seems to fade.",	/* 50 */
  "Your feet seem less boyant.",
  "!Finger Death!",
  "!Adv Heal!",
  "!Refresh!",
  "!Fear!",                                     /* 55 */
  "!Dimensional Gate!",
  "!Meteor Swarm!",
  "!Group Sanct!",
  "Your skin ceases to sparkle.",
  "Gravity regains its grip on you...",         /* 60 */
  "!Plasma Blast!",
  "!Cloud Kill!",
  "!Paralyze!",
  "You stumble slightly as your legs begin to function again.",
  "Your magical gills suddenly vanish..",       /* 65 */
  "!wraith touch!",
  "Your revert back to your true form.",
  "!Mana!",
  "!Whirlwind!",
  "The spirits leave you less protected.",      /* 70 */
  "Your god stops helping you.",
  "Your deity stops protecting you.",
  "Your body slows down to normal pace.",
  "Your untouchability fades away.",
  "A skin of stone peels away from you.",       /* 75 */
  "!Divine Heal!",
  "The lightning upon your shield fades away.",
  "The flaming fire of your shield burns out.",
  "Your surrounding wall of fire slowely burns away.",
  "You feel less invulnerable.",                /* 80 */     
  "!UNUSED!"
};

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

const char *pc_race_types[] = {
  "Ogre",
  "Deva",
  "Minotaur",
  "Elf",
  "Changling",
  "Human",
  "Orc",
  "Dwarf",
  "Kenda",
  "Pixie",
  "\n"
};  

const char *npc_class_types[] = {
  "Generic",
  "Undead",
  "Humanoid",
  "Animal",
  "Dragon",
  "Giant",
  "Aquatic",
  "DemiHuman",
  "\n"
};  


int rev_dir[] =
{
  2,
  3,
  0,
  1,
  5,
  4
};


int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  1,	/* Flying     */
  5     /* Underwater */
};

/* Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the day of the Great Gods",
  "the Day of the Sun"
};


/* Not used in sprinttype(). */
const char *month_name[] = {
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

#if defined(CONFIG_OASIS_MPROG)
/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const char *mobprog_types[] = {
  "INFILE",
  "ACT",
  "SPEECH",
  "RAND",
  "FIGHT",
  "DEATH",
  "HITPRCNT",
  "ENTRY",
  "GREET",
  "ALL_GREET",
  "GIVE",
  "BRIBE",
  "\n"
};
#endif     

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

const char *colour_headings[] = {
  "Headings",
  "Sub Headings",
  "Help Headings",
  "Help Attribute Headings",
  "Commands",
  "Objects",
  "Mobs",
  "Players",
  "Unused",
  "Unused"
};

// Values are the Index position in const char *COLOURLIST[] (color.c) 
const int default_colour_codes[] = {
  2,    //"Headings",
  3,    //"Sub Headings",
  14,   //"Help Headings",
  2,    //"Help Attribute Headings",
  6,    //"Commands",
  2,    //"Objects",
  3,    //"Mobs",
  3,    //"Players",
  0,    //"Unused",
  0     //"Unused"
};
