/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030011 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD 3.0 to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1	/* TRUE/FALSE aren't defined yet. */

/* preamble *************************************************************/

/*
 * Eventually we want to be able to redefine the below to any arbitrary
 * value.  This will allow us to use unsigned data types to get more
 * room and also remove the '< 0' checks all over that implicitly
 * assume these values. -gg 12/17/99
 */
#define NOWHERE    -1    /* nil reference for room-database	*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)


/* room-related defines *************************************************/


/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5

/* The Maximum value for stats */
#define MAX_STAT_VALUE 25

/* Clans */
#define CLAN_UNDEFINED	-1
#define CLAN_ROSE_CLUB	0
#define CLAN_AVENGERS	1
#define CLAN_GUARDIAN	2
#define CLAN_FOREVER	3
#define CLAN_USS	4

/* The number of clans */
#define CLAN_NUM	5

/* Clan ranks	*/
#define RANK_NUM	16
#define MAX_LEN 	50
#define FILE_TERM	'$'

/* (Auction) the max number of lots at once */
#define MAX_LOTS 5
#define MAX_PRICE 1000000000 

/* Were creature defines */
#define WOLF_VNUM	116
#define VAMP_VNUM	1506

/* Number of worlds */
#define NUM_WORLDS 3

/* Sort type defines */
#define NUM_SORT_TYPES 	3
#define SORT_ALPHA	0
#define SORT_ASCENDING	1
#define SORT_DESCENDING	2

/* DM - World defines */
#define WORLD_MEDIEVAL 0
#define WORLD_WEST     1
#define WORLD_FUTURE   2

/* Level that vict must be to receive protection from good/evil */ 
#define PROTECT_LEVEL 	35

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		(1 << 0)   /* Dark			*/
#define ROOM_DEATH		(1 << 1)   /* Death trap		*/
#define ROOM_NOMOB		(1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		(1 << 3)   /* Indoors			*/
#define ROOM_PEACEFUL		(1 << 4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF		(1 << 5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK		(1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		(1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		(1 << 8)   /* room for only 1 pers	*/
#define ROOM_PRIVATE		(1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM		(1 << 10)  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		(1 << 11)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH	(1 << 12)  /* (R) House needs saving	*/
#define ROOM_ATRIUM		(1 << 13)  /* (R) The door to a house	*/
#define ROOM_OLC		(1 << 14)  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK		(1 << 15)  /* (R) breath-first srch mrk	*/
#define ROOM_NEWBIE		(1 << 16)  /* Newbie room */
#define ROOM_LR_ET		(1 << 17)  /* Eternals and up */
#define ROOM_REGEN_2		(1 << 18)  /*  x2  */
#define ROOM_REGEN_HALF		(1 << 19)  /*  /2  */
#define ROOM_NOSLEEP		(1 << 20)  
#define ROOM_LR_ANG		(1 << 21)  /* Angels + */
#define ROOM_LR_IMM		(1 << 22)  /* Imms +   */
#define ROOM_LR_GOD		(1 << 23)  /* Gods +   */
#define ROOM_LR_IMP		(1 << 24)  /* Imps +   */
 

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		(1 << 3)   /* Lock can't be picked	*/


/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_FLYING	     8		   /* Wheee!			*/
#define SECT_UNDERWATER	     9		   /* Underwater		*/


/* char and mob-related defines *****************************************/

/* PC races */
#define RACE_UNDEFINED -1
#define RACE_OGRE       0
#define RACE_DEVA       1
#define RACE_MINOTAUR   2
#define RACE_ELF        3
#define RACE_CHANGLING  4
#define RACE_HUMAN      5
#define RACE_ORC        6
#define RACE_DWARF      7
#define RACE_KENDA      8
#define RACE_PIXIE      9
#define RACE_AVATAR     10
 
#define MAX_RACES       11 

/* PC classes */
#define CLASS_UNDEFINED	  -1
#define CLASS_MAGIC_USER  0
#define CLASS_CLERIC      1
#define CLASS_THIEF       2
#define CLASS_WARRIOR     3

#define CLASS_DRUID		4	// Mage-Cleric
#define CLASS_PRIEST		5	// Cleric-Thief
#define CLASS_NIGHTBLADE	6	// Warrior-thief
#define CLASS_BATTLEMAGE	7	// Warrior-Mage
#define CLASS_SPELLSWORD	8	// Mage-thief
#define CLASS_PALADIN		9	// Warrior-Cleric

#define CLASS_MASTER		10	// Warrior-thief-cleric-mage

#define NUM_CLASSES	  11  /* This must be the number of classes!! */

/* NPC classes (currently unused - feel free to implement!) */
#define CLASS_OTHER       0
#define CLASS_UNDEAD      1		// Undead
#define CLASS_HUMANOID    2		// Humans, mainly
#define CLASS_ANIMAL      3		// Beasts
#define CLASS_DRAGON      4		// Dragon types
#define CLASS_GIANT       5		// Large gigantic mobs
#define CLASS_AQUATIC     6		// Sea faring mobs
#define CLASS_DEMIHUMAN   7		// Demi humans (kobolds, orcs, etc)
  
#define NUM_NPC_CLASSES   8

/* Ability codes */
#define CLASS_ABILITY		-1

/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

/* Specials available */
#define SPECIAL_INVIS           (1 << 0)        // Permanant invis
#define SPECIAL_SNEAK           (1 << 1)        // Permanant sneak
#define SPECIAL_MULTIWEAPON     (1 << 2)        // Can use multiple weapons
#define SPECIAL_FOREST_SPELLS   (1 << 3)        // Spells more powerful in forest
#define SPECIAL_FOREST_HELP     (1 << 4)        // Druid ability - forest aid
#define SPECIAL_HEALER          (1 << 5)        // Healing spells more powerful
#define SPECIAL_PRIEST          (1 << 6)        // Charges for spells
#define SPECIAL_BACKSTAB        (1 << 7)        // Can backstab during battle
#define SPECIAL_BATTLEMAGE      (1 << 8)        // Offensive spells more powerful
#define SPECIAL_MANA_THIEF      (1 << 9)        // Steals mana on hit
#define SPECIAL_HOLY            (1 << 10)       // Destroy undead with one hit
 
#define SPECIAL_DISGUISE        (1 << 11)       // Can disguise self as a mob
#define SPECIAL_ESCAPE          (1 << 12)       // Can always get to daylight safely
#define SPECIAL_INFRA           (1 << 13)       // Perm infravision
#define SPECIAL_DWARF           (1 << 14)       // Bonus while inside (not city)
#define SPECIAL_GROUP_SNEAK     (1 << 15)       // Sneak to self and group
#define SPECIAL_THIEF           (1 << 16)       // Very good thieving abilities
#define SPECIAL_GORE            (1 << 17)       // Gore damage in battle
#define SPECIAL_MINOTAUR        (1 << 18)       // +5% to damroll
#define SPECIAL_CHARMER         (1 << 19)       // Can charm demi humans
#define SPECIAL_SUPERMAN        (1 << 20)       // 21 Str and Con, bonus to AC, +2% to damroll
#define SPECIAL_FLY             (1 << 21)       // Can fly
#define SPECIAL_ELF             (1 << 22)       // Bonus while fighting in forest
 
#define MAX_SPECIALS            23

/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_NOIGNORE    (1 << 16)  /* Player cant' use ignore cmd. - Hal*/
#define PLR_LAGGED      (1 << 17)  /* Player has been given artificial lag */
#define PLR_GODMAIL     (1 << 18)  /* Player is writing mail to the gods */
#define PLR_MOUNTED     (1 << 19)  /* Player is a slut */
#define PLR_NOINFO	(1 << 20)  /* Player cannot use INFO command	*/

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	 (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_EVIL	 (1 << 8)  /* auto attack evil PC's		*/
#define MOB_AGGR_GOOD	 (1 << 9)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 10) /* auto attack neutral PC's		*/
#define MOB_MEMORY	 (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	 (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	 (1 << 13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (1 << 15) /* Mob can't be slept		*/
#define MOB_NOBASH	 (1 << 16) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	 (1 << 17) /* Mob can't be blinded		*/
#define MOB_QUEST	 (1 << 18) /* Mob is part of a quest - shoudln't be killed */
#define MOB_INTELLIGENT  (1 << 19) /* mob will equip thigns it picks up */
#define MOB_2ND_ATTACK   (1 << 20) /* Mob gets an extra attack		*/
#define MOB_3RD_ATTACK   (1 << 21) /* Mob gets a further attack		*/
#define MOB_NO_STEAL	 (1 << 22) /* Mob can't be stolen from */
#define MOB_MOUNTABLE	 (1 << 23) /* Mob is a mount */


/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(1 << 3)  /* Can't receive tells		*/
#define PRF_DISPHP	(1 << 4)  /* Display hit points in prompt	*/
#define PRF_DISPMANA	(1 << 5)  /* Display mana points in prompt	*/
#define PRF_DISPMOVE	(1 << 6)  /* Display move points in prompt	*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_QUEST	(1 << 9)  /* On quest				*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR_1	(1 << 13) /* Color (low bit)			*/
#define PRF_COLOR_2	(1 << 14) /* Color (high bit)			*/
#define PRF_NOWIZ	(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	(1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	(1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_CLS         (1 << 22) /* Clear screen in OasisOLC           */
#define PRF_NOIMMNET    (1 << 23) /* Player won't see IMMNET		*/
#define PRF_AFK		(1 << 24) /* Player is AFK */
#define PRF_WOLF	(1 << 25) /* Player is a werewolf		*/
#define PRF_VAMPIRE	(1 << 26) /* Player is a vamp			*/
#define PRF_TAG		(1 << 27) /* Player is playing Tag		*/
#define PRF_MORTALK     (1 << 28) /* player is in MortalK		*/
#define PRF_DISPEXP     (1 << 29) /* Player is seeing XP in their prompt*/
#define PRF_DISPALIGN   (1 << 30) /* Player is seeing align in prompt   */
#define PRF_NOINFO      (1 << 31) /* Player can't see the info channel  */


/* Extended flags ( char_data.player_specials.ext_flags ) */
#define EXT_NONEWBIE	(1 << 0)	/* Can't hear newbie ch */
#define EXT_NOCTALK	(1 << 1)	/* Can't hear clan ch */
#define EXT_CLAN	(1 << 2)	/* Can join a clan */
#define EXT_SUBLEADER	(1 << 3)	/* Is a clan subleader */
#define EXT_LEADER	(1 << 4)	/* Is a clan leader	*/
#define EXT_AUTOGOLD	(1 << 5)	/* Auto gold looting	*/
#define EXT_AUTOLOOT	(1 << 6)	/* Auto looting corpse 	*/
#define EXT_AUTOSPLIT	(1 << 7)	/* Auto splitting gold	*/


/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_DETECT_ALIGN      (1 << 2)	   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1 << 6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed		*/
#define AFF_INFRAVISION       (1 << 10)	   /* Char can see in dark	*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1 << 13)	   /* Char protected from good  */
#define AFF_SLEEP             (1 << 14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK	      (1 << 15)	   /* Char can't be tracked	*/
#define AFF_REFLECT	      (1 << 16)	   /* Damage reflects back	*/
#define AFF_FLY		      (1 << 17)	   /* Char is flying		*/
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly	*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden		*/
#define AFF_PARALYZED	      (1 << 20)	   /* Char is paralyzed		*/
#define AFF_CHARM             (1 << 21)	   /* Char is charmed		*/
#define AFF_WATERBREATHE	(1 << 22)  /* Char can breath underwater*/
#define AFF_HASTE		(1 << 23)  /* Char is hasted		*/
#define AFF_ADVANCED_INVIS	(1 << 24)  /* Char is invis, adv.	*/
#define AFF_DETECT_ADVANED	(1 << 25)  /* Shouldn't need this, DM   */
#define AFF_NOHASSLE		(1 << 26)  /* Char has NOHASSLE set	*/
#define AFF_BROKEN_IN		(1 << 27)  /* Char is tamed and rideable*/
#define AFF_ADRENALINE          (1 << 28)  /* Char affected by adrenaline*/


/* The Maximum number of tic related timers used in char_specials */ 
#define MAX_TIMERS    2 

#define TIMER_HEALING_SKILLS    0
#define TIMER_ADRENALINE        1

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0		/* Playing - Nominal state	*/
#define CON_CLOSE	 1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	 7		/* Sex?				*/
#define CON_QCLASS	 8		/* Class?			*/
#define CON_RMOTD	 9		/* PRESS RETURN after MOTD	*/
#define CON_MENU	 10		/* Your choice: (main menu)	*/
#define CON_EXDESC	 11		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12		/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 13		/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   14		/* Verify new password		*/
#define CON_DELCNF1	 15		/* Delete confirmation 1	*/
#define CON_DELCNF2	 16		/* Delete confirmation 2	*/
#define CON_DISCONNECT	 17		/* In-game disconnection	*/
#define CON_OEDIT        18             /* OLC mode - object editor     */
#define CON_REDIT        19             /* OLC mode - room editor       */
#define CON_ZEDIT        20             /* OLC mode - zone info editor  */
#define CON_MEDIT        21             /* OLC mode - mobile editor     */
#define CON_SEDIT        22             /* OLC mode - shop editor       */
#define CON_TEDIT        23             /* OLC mode - text editor       */ 
#define CON_QRACE	 24		/* Querying race		*/
#define CON_QSTATCHECK	 25		/* Verifying stats		*/
#define CON_QCOLOUR	 26		/* Querying colour		*/

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17

#define NUM_WEARS      18	/* This must be the # of eq positions!! */


/* object-related defines ********************************************/

/* JA defines for how many protective item types there are */
#define BASE_PROTECT_GEAR 30
#define MAX_PROTECT_GEAR  30

/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_MISSILE    7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_MAGIC_EQ  24		/* Item is magic eq - Vader	*/
#define ITEM_JOINABLE  25		/* Item is part of something 	*/
#define ITEM_BATTERY   26		/* Item can hold charges	*/
#define ITEM_QUEST     27		/* Quest item			*/

/* New item types for objects needd tomove through hostile environments */
#define ITEM_RESPIRATE	30		/* In thin air */
#define ITEM_BREATHER	31		/* In unbreathable */
#define ITEM_VACSUIT	32		/* Safe in vacuum */
#define ITEM_ENVIRON	33		/* Safe in corrosive air */
#define ITEM_STASIS	34		/* Safe anywhere */
#define ITEM_HEATRES	35		/* Heat resistant */
#define ITEM_HEATPROOF  36		/* Heat proof */
#define ITEM_COLD	37		/* Item protects from cold */
#define ITEM_SUBZERO	38		/* Protects from freezing	*/
#define ITEM_GRAV1	39		/* Anti-grav device up to 2x	*/
#define ITEM_GRAV3	40		/* Anti-grav device up to 3x	*/
#define ITEM_GATEWAY	41		/* GAteway object		*/
#define ITEM_RAD1PROOF	42		/* ? */


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/


/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW          (1 << 0)	/* Item is glowing		*/
#define ITEM_HUM           (1 << 1)	/* Item is humming		*/
#define ITEM_NORENT        (1 << 2)	/* Item cannot be rented	*/
#define ITEM_NODONATE      (1 << 3)	/* Item cannot be donated	*/
#define ITEM_NOINVIS	   (1 << 4)	/* Item cannot be made invis	*/
#define ITEM_INVISIBLE     (1 << 5)	/* Item is invisible		*/
#define ITEM_MAGIC         (1 << 6)	/* Item is magical		*/
#define ITEM_NODROP        (1 << 7)	/* Item is cursed: can't drop	*/
#define ITEM_BLESS         (1 << 8)	/* Item is blessed		*/
#define ITEM_ANTI_GOOD     (1 << 9)	/* Not usable by good people	*/
#define ITEM_ANTI_EVIL     (1 << 10)	/* Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL  (1 << 11)	/* Not usable by neutral people	*/
#define ITEM_ANTI_MAGIC_USER (1 << 12)	/* Not usable by mages		*/
#define ITEM_ANTI_CLERIC   (1 << 13)	/* Not usable by clerics	*/
#define ITEM_ANTI_THIEF	   (1 << 14)	/* Not usable by thieves	*/
#define ITEM_ANTI_WARRIOR  (1 << 15)	/* Not usable by warriors	*/
#define ITEM_NOSELL	   (1 << 16)	/* Shopkeepers won't touch it	*/
#define ITEM_RIDDEN	   (1 << 17)	/* Item is a mount		*/
#define ITEM_HIDDEN	   (1 << 18)	/* Item is hidden		*/

#define ITEM_ANTI_DRUID    (1 << 19)    /* Not usable by druids         */
#define ITEM_ANTI_PRIEST   (1 << 20)    /* Not usable by priests        */
#define ITEM_ANTI_NIGHTBLADE (1 << 21)    /* Not usable by nightblades  */
#define ITEM_ANTI_BATTLEMAGE (1 << 22)    /* Not usable by battlemages  */
#define ITEM_ANTI_SPELLSWORD (1 << 23)    /* Not usable by spellswords  */

/* Skipped bitvectors to make backward compatible, now that LR's are treated
   to a separate bitvector (level_flags) */

#define ITEM_NOHUNGER	   (1 << 24)
#define ITEM_NOTHIRST	   (1 << 25)
#define ITEM_NODRUNK	   (1 << 26)

#define ITEM_ANTI_PALADIN  (1 << 27)    /* Not usable by paladins       */
#define ITEM_ANTI_MASTER   (1 << 28)    /* Not usable by masters        */

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to constitution	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/


/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15
#define LIQ_CHAMPAGNE  16

/* Zone defines */  

#define GOD_ROOMS_ZONE 2 

#define ZN_PK_ALLOWED   (1<<0)
#define ZN_NEWBIE       (1<<1)
#define ZN_LR_5         (1<<2)
#define ZN_LR_10        (1<<3)
#define ZN_LR_15        (1<<4)
#define ZN_LR_20        (1<<5)
#define ZN_LR_25        (1<<6)
#define ZN_LR_30        (1<<7)
#define ZN_LR_35        (1<<8)
#define ZN_LR_40        (1<<9)
#define ZN_LR_45        (1<<10)
#define ZN_LR_50        (1<<11)
#define ZN_LR_55        (1<<12)
#define ZN_LR_60        (1<<13)
#define ZN_LR_65        (1<<14)
#define ZN_LR_70        (1<<15)
#define ZN_LR_75        (1<<16)
#define ZN_LR_80        (1<<17)
#define ZN_LR_85        (1<<18)
#define ZN_LR_90        (1<<19)
#define ZN_LR_95        (1<<20)
#define ZN_LR_ET        (1<<21)
#define ZN_LR_IMM       (1<<22)
#define ZN_LR_IMP       (1<<23)
#define ZN_NO_STEAL     (1<<24)
#define ZN_NO_TELE      (1<<25)

/* Quest Equipment Enchancement data */
#define MAX_NUM_ENHANCEMENTS            5    // # of enh's an item is allowed
#define MAX_ENHANCEMENT_VALUES          3    // # of enhancements on a particular enh
/* Sample enhancement data */
// Enhantment #1 = ENHANCE_DAMROLL -- Boost #1 = +3, Boost #2 = +1
// Total boost affect: DR +4
 
/* Quest eq enhancement defines */
#define ENHANCE_NONE            0       // Dud enhancement
#define ENHANCE_STR             1
#define ENHANCE_DEX             2
#define ENHANCE_INT             3
#define ENHANCE_WIS             4
#define ENHANCE_CON             5
#define ENHANCE_CHA             6
#define ENHANCE_CLASS           7       // Not in use, but it's in the A <location> <value> of items
#define ENHANCE_LEVEL           8       // Some possibilities
#define ENHANCE_AGE             9
#define ENHANCE_WEIGHT          10
#define ENHANCE_HEIGHT          11
#define ENHANCE_MANA            12
#define ENHANCE_HIT             13
#define ENHANCE_MOVE            14
#define ENHANCE_GOLD            15
#define ENHANCE_EXP             16
#define ENHANCE_AC              17
#define ENHANCE_HITROLL         18
#define ENHANCE_DAMROLL         19
#define ENHANCE_SAVING_PARA     20
#define ENHANCE_SAVING_ROD      21
#define ENHANCE_SAVING_PETRI    22
#define ENHANCE_SAVING_BREATH   23
#define ENHANCE_SAVING_SPELL    24   

/* other miscellaneous defines *******************************************/

#define MAX_GAME_BETS	10
#define MAX_QUEST_ITEMS 18

/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2

/* Invis states - used by player_special_data_saved.invis_type_flag
        When an invis range is set, the value of invis_type_flag is the top range */
#define INVIS_SPECIFIC  -2
#define INVIS_SINGLE    -1
#define INVIS_NORMAL    0

/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3

/* Moon conditions for were creatures */
#define MOON_NONE		0
#define MOON_1ST_QTR		1
#define MOON_HALF		2
#define MOON_3RD_QTR		3
#define MOON_FULL		4
#define MOON_2ND_3RD_QTR	5
#define MOON_2ND_HALF		6
#define MOON_FINAL_QTR		7

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5


/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1.
 */
#define LVL_OWNER	200
#define LVL_GRIMPL	175
#define LVL_IMPL	150
#define LVL_GRGOD	140
#define LVL_GOD		125
#define LVL_ANGEL	110
#define LVL_IMMORT	100
#define LVL_ETRNL9	99
#define LVL_ETRNL8	98
#define LVL_ETRNL7	97
#define LVL_ETRNL6	96
#define LVL_ETRNL5	95
#define LVL_ETRNL4	94
#define LVL_ETRNL3	93
#define LVL_ETRNL2	92
#define LVL_ETRNL1	91

#define LVL_NEWBIE	15

/* Level of the 'freeze' command */
#define LVL_FREEZE	LVL_GRGOD
#define LVL_IS_GOD	LVL_ANGEL
#define LVL_ISNOT_GOD	LVL_IMMORT

#define NUM_OF_DIRS	6	/* number of directions in a room (nsewud) */
#define MAGIC_NUMBER	(0x06)	/* Arbitrary number that won't be in a string */

#define OPT_USEC	100000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (10 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)


/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE	   (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE		5	/* Keep last 5 commands. */
// #define MAX_STRING_LENGTH	8192
#define MAX_STRING_LENGTH	16384	
#define MAX_INPUT_LENGTH	256	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512	/* Max size of *raw* input */
#define MAX_MESSAGES		60
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH		10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH		30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH		240 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE		3   /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS		200 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		32  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT		6 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define POOF_LENGTH		60  /* Used in char_file_u *DO*NOT*CHANGE* */
#define NUM_COLOUR_SETTINGS	10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define NUM_WORLDS              3   /* Used in char_file_u *DO*NOT*CHANGE* */
  

/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (12 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       MAX_INPUT_LENGTH  /* Max length of prompt  */
#define GARBAGE_SPACE		32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE		1024        /* Static output buffer size   */

/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char			bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef char			byte;
#endif

typedef sh_int room_vnum;      	/* A room's vnum type.          */
typedef sh_int obj_vnum;       	/* An object's vnum type.       */
typedef sh_int mob_vnum;       	/* A mob's vnum type.           */
typedef sh_int	zone_vnum;     	/* A virtual zone number.	*/
typedef sh_int shop_vnum;      	/* A virtual shop number.       */

typedef sh_int	room_rnum;	/* A room's real (internal) number type */
typedef sh_int	obj_rnum;	/* An object's real (internal) num type */
typedef sh_int	mob_rnum;	/* A mobile's real (internal) num type */
typedef sh_int	zone_rnum;	/* A zone's real (array index) number.	*/
typedef sh_int shop_rnum;      	/* A shop's real (array index) number.  */

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef unsigned long int	bitvector_t;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};


/* object-related structures ******************************************/


/* object flags; used in obj_data */
struct obj_flag_data {
   int value[4];       /* Values of the item (see list)        */
   byte type_flag;     /* Type of item                         */
   int level;          /* Minimum level of object.             */
   int /*bitvector_t*/ wear_flags;     /* Where you can wear it        */
   int /*bitvector_t*/ extra_flags;    /* If it hums, glows, etc.      */ 
   long		       level_flags;    /* Level restrictive flags      */ 
  int	weight;		/* Weigt what else                  */
   int	cost;		/* Value when sold (gp.)            */
   int	cost_per_day;	/* Cost to keep pr. real day        */
   int	timer;		/* Timer for object                 */
   long /*bitvector_t*/	bitvector;	/* To set chars bits                */
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
   byte location;      /* Which ability to change (APPLY_XXX) */
   sbyte modifier;     /* How much it changes by              */
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			*/
   room_rnum in_room;		/* In what room -1 when conta/carr	*/

   struct obj_flag_data obj_flags;/* Object information               */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *ridden_by;
   struct char_data *worn_by;	  /* Worn by?			      */
   sh_int worn_on;		  /* Worn where?		      */
 
  sh_int max_damage;		  /* Max durability of item	      */ 
  sh_int damage;		  /* State of item		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */
};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
struct obj_file_elem {
   obj_vnum item_number;

#if USE_AUTOEQ
   sh_int location;
#endif
   int	value[4];
   int /*bitvector_t*/	extra_flags;
   int	weight;
   int	timer;
   long /*bitvector_t*/	bitvector;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
};


/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
   int	time;
   int	rentcode;
   int	net_cost_per_diem;
   int	gold;
   int	account;
   int	nitems;
   int	spare0;
   int	spare1;
   int	spare2;
   int	spare3;
   int	spare4;
   int	spare5;
   int	spare6;
   int	spare7;
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
   char	*general_description;       /* When look DIR.			*/

   char	*keyword;		/* for open/close			*/

   sh_int /*bitvector_t*/ exit_info;	/* Exit info			*/
   obj_vnum key;		/* Key's number (-1 for no key)		*/
   room_rnum to_room;		/* Where direction leads (NOWHERE)	*/
};


/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		/* Rooms number	(vnum)		      */
   zone_rnum zone;              /* Room zone (for resetting)          */
   int	sector_type;            /* sector type (move/hide)            */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   int /*bitvector_t*/ room_flags;		/* DEATH,DARK ... etc */

   byte light;                  /* Number of lightsources in room     */
   SPECIAL(*func);

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int	played;     /* This is the total accumulated time played in secs */
};


/* general player-related info, usually PC's and NPC's */
struct char_player_data {
   char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
   char	*name;	       /* PC / NPC s name (kill ...  )         */
   char	*short_descr;  /* for NPC 'actions'                    */
   char	*long_descr;   /* for 'look'			       */
   char	*description;  /* Extra descriptions                   */
   char	*title;        /* PC / NPC's title                     */
   
   char *email;		/* Email */
   char *webpage;	/* Webpage */
   char *personal;	/* Personal info */

   byte sex;           /* PC / NPC's sex                       */
   byte chclass;       /* PC / NPC's class		       */
   ubyte level;         /* PC / NPC's level                     */
   int	hometown;      /* PC s Hometown (zone)                 */
   struct time_data time;  /* PC's AGE in days                 */
   ubyte weight;       /* PC / NPC's weight                    */
   ubyte height;       /* PC / NPC's height                    */
};


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
   sbyte str;
   sbyte str_add;      /* 000 - 100 if strength 18             */
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
   sh_int mana;
   sh_int max_mana;     /* Max mana for PC/NPC			   */
   sh_int hit;
   sh_int max_hit;      /* Max hit for PC/NPC                      */
   sh_int move;
   sh_int max_move;     /* Max move for PC/NPC                     */

   sh_int armor;        /* Internal -100..100, external -10..10 AC */
   int	gold;           /* Money carried                           */
   int	bank_gold;	/* Gold the char has in a bank account	   */
   int	exp;            /* The experience of the player            */

   ubyte hitroll;       /* Any bonus or penalty to the hit roll    */
   ubyte damroll;       /* Any bonus or penalty to the damage roll */
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
   int	alignment;		/* +-1000 for alignments                */
   long	idnum;			/* player's idnum; -1 for mobiles	*/
   long /*bitvector_t*/ act;	/* act flag for NPC's; player flag for PC's */

   long /*bitvector_t*/	affected_by;
				/* Bitvector for spells/skills affected by */
   sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)		*/
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
   struct char_data *fighting;	  /* Opponent				*/
   struct char_data *hunting;	  /* Char hunted by this char		*/
   struct char_data *mounting;
   struct obj_data *mounting_obj;	// DM - TODO - check with tali char_data ...
   struct char_data *autoassist;  /* The char to be autoassisted   	*/ 

   float modifier;

   byte position;		/* Standing, fighting, sleeping, etc.	*/

   int	carry_weight;		/* Carried weight			*/
   byte carry_items;		/* Number of items carried		*/
   int	timer;			/* Timer for update			*/

   struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
   byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
   byte PADDING0;		/* used to be spells_to_learn		*/
   bool talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
   ubyte wimp_level;		/* Below this # of hit points,flee!	*/
   ubyte freeze_level;		/* Level of god who froze char, if any	*/
   ubyte invis_level;		/* level of invisibility		*/
   room_vnum load_room;		/* Which room to place char in		*/
   long /*bitvector_t*/	pref;	/* preference flags for PC's.		*/
   ubyte bad_pws;		/* number of bad password attemps	*/
   sbyte conditions[3];         /* Drunk, full, thirsty			*/

   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */

   ubyte spare0;
   ubyte spare1;
   ubyte spare2;
   ubyte spare3;
   ubyte spare4;
   ubyte spare5;
   int spells_to_learn;		/* How many can you learn yet this level*/
   int olc_zone;
   int spare8;
   int spare9;
   int spare10;
   int spare11;
   int spare12;
   int spare13;
   int spare14;
   int spare15;
   int spare16;
   long	spare17;
   long	spare18;
   long	spare19;
   long	spare20;
   long	spare21;
};

/* Struct for player kill info */
struct kill_data {
	
	long mobkills;
	long killedbymobs;
	long pckills;
	long killedbypcs;
	long immkills;
	long killedbyimms;

	long sparekilldata1;
	long sparekilldata2;
	long sparekilldata3;
};

struct quest_obj_data {
  obj_vnum vnum;
  byte max_number;
  // Enhancement listing
  short enh_setting[MAX_NUM_ENHANCEMENTS];      // Enhancent positions
  short enhancements[MAX_NUM_ENHANCEMENTS][MAX_ENHANCEMENT_VALUES];  // Values at positions
  struct obj_data *owner; // Attach an owner after enhancing item
};

/* Basically a copy of the above struct's spares, from the
   old version of primal, leaving the 'spares' above, free   */
struct primal_extend_data {

 	struct quest_obj_data quest_eq[MAX_QUEST_ITEMS];
	struct kill_data kills; 

	int world_entry[NUM_WORLDS];
	int ignore1;
	int ignore2;
	int ignore3;
	int ignorelev;
	int ignorenum;
	int clan_num;
	int clan_level;
	int invis_type_flag;	
	long ext_flag;

// DM - Tali Read
// Rather than storing the modifier, calculate it at load time depending upon the class.
// Use bitvectors to store the remmort specials (misc_specials), and then the equation 
// simply is something like: GET_MODIFIER(ch) = calc_mod(ch) + spec_mod(ch) .....
//	long modifier;		/* XP modifier 		*/
        long abilities;         /* ability bitvector    */ 

	long misc_specials;	/* Bitvector representation of misc. specials */
	long race;		/* Race			*/

        long char_disguised;    /* Mob disguised as     */
        long char_memorised;    /* Mob memorised        */

        int page_width;
        int page_length;

//        char poofin[POOF_LENGTH+1];
//        char poofout[POOF_LENGTH+1];

        byte colour_settings[NUM_COLOUR_SETTINGS];

	/* Just some more blanks, for no particular reason */
	long spare1;
	long spare2;
	long spare3;
	int spare4;
	int spare5;
	int spare6;
	ubyte spare7;
	ubyte spare8;
	ubyte spare9;

};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
   struct player_special_data_saved saved;
   struct primal_extend_data primalsaved;
   struct kill_data player_kills;

   char	*poofin;		/* Description on arrival of a god.     */
   char	*poofout;		/* Description upon a god's exit.       */
   struct alias_data *aliases;	/* Character's aliases			*/
   long last_tell;		/* idnum of last tell from		*/
   void *last_olc_targ;		/* olc control				*/
   int last_olc_mode;		/* olc control				*/
   struct timer_type *timers;
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
   byte last_direction;     /* The last direction the monster went     */
   int	attack_type;        /* The Attack Type Bitvector for NPC's     */
   byte default_pos;        /* Default position for NPC                */
   memory_rec *memory;	    /* List of attackers to remember	       */
   byte damnodice;          /* The number of damage dice's	       */
   byte damsizedice;        /* The size of the damage dice's           */
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
   sh_int type;          /* The type of spell that caused this      */
   sh_int duration;      /* For how long its effects will last      */
   sbyte modifier;       /* This is added to apropriate ability     */
   byte location;        /* Tells which ability to change(APPLY_XXX)*/
   long /*bitvector_t*/	bitvector; /* Tells which bits to set (AFF_XXX) */

   struct affected_type *next;
};

/* DM - The timer structure. */
struct timer_type {
  sh_int type;
  sh_int duration;
  sh_int uses;
  sh_int max_uses;
  
  struct timer_type *next;
};

/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};

/* Structure used for a list of chars autoassisting this char */
struct assisters_type {
  struct char_data *assister;
  struct assisters_type *next;
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
   int pfilepos;			 /* playerfile pos		  */
   mob_rnum nr;                          /* Mob's rnum			  */
   room_rnum in_room;                    /* Location (real room number)	  */
   room_rnum was_in_room;		 /* location for linkdead people  */
   int wait;				 /* wait for how many loops	  */

   struct char_player_data player;       /* Normal data                   */
   struct char_ability_data real_abils;	 /* Abilities without modifiers   */
   struct char_ability_data aff_abils;	 /* Abils with spells/stones/etc  */
   struct char_point_data points;        /* Points                        */
   struct char_special_data char_specials;	/* PC/NPC specials	  */
   struct player_special_data *player_specials; /* PC specials		  */
   struct mob_special_data mob_specials;	/* NPC specials		  */

   struct affected_type *affected;       /* affected by what spells       */
   struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

   struct obj_data *carrying;            /* Head of list                  */
   struct descriptor_data *desc;         /* NULL for mobiles              */

   struct char_data *next_in_room;     /* For room->people - list         */
   struct char_data *next;             /* For either monster or ppl-list  */
   struct char_data *next_fighting;    /* For fighting list               */
   struct assisters_type *autoassisters;  /* For players assisting char   */ 

   struct follow_type *followers;        /* List of chars followers       */
   struct char_data *master;             /* Who is char following?        */
};
/* ====================================================================== */


/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[EXDSCR_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   byte sex;
   byte chclass;
   ubyte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int	played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;


   char email[MAX_STRING_LENGTH + 1];
   char webpage[MAX_STRING_LENGTH + 1];
   char personal[MAX_STRING_LENGTH + 1];
   
   char poofin[POOF_LENGTH + 1];
   char poofout[POOF_LENGTH + 1];

   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   // New primal struct
   struct primal_extend_data player_specials_primalsaved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	/* host of last logon */
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct descriptor_data {
   socket_t	descriptor;	/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;              /* backup string for modify-str system  */
   size_t max_str;             /* maximum size of string in modify-str */
   long	mail_to;		/* name for mail system			*/
   int	has_prompt;		/* is the user at a prompt?             */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   void *olc; 
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};


struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
   sh_int ambush;
};


struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
};


struct str_app_type {
   sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   sh_int todam;    /* Damage Bonus/Penalty                */
   sh_int carry_w;  /* Maximum weight that can be carrried */
   sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
   byte bonus;       /* how many practices player gains per lev */
};


struct int_app_type {
   byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
   sh_int hitp;
   sh_int shock;
};


struct weather_data {
   int	pressure;	/* How is the pressure ( Mb ) */
   int	change;	/* How fast and what way does it change. */
   int	sky;	/* How is the sky. */
   int	sunlight;	/* And how much sun. */
   int moon;		/* Moon state for were creatures */
};


struct title_type {
   char	*title_m;
   char	*title_f;
   int	exp;
};


/* element in monster and object index-tables   */
struct index_data {
   sh_int	vnum;	/* virtual number of this mob/obj		*/
   int	number;		/* number of existing units of this mob/obj	*/
   SPECIAL(*func);
};


struct game_item {

        struct char_data *ch;           // Gambler
        int amount;                     // Bet
        int type;                       // Type of bet -- long, short, medium(default)
};

struct game_data {

        int timetogame;                 // State of race
        int numbets;                    // Number of bets on race
        struct game_item list[MAX_GAME_BETS];   // List of bets

} race;

struct blackjack_data {

        int numplayers;                 // Number of players 
        int cards[MAX_GAME_BETS * 2][6];// 5 cards for every player and every dealer and info card
        struct game_item list[MAX_GAME_BETS];   // Player list

} blackjack;

/* clan data */
struct clan_data {
   char name [MAX_LEN];
   char disp_name[MAX_LEN];
   char ranks[RANK_NUM][MAX_LEN];
   char *table;
   /* struct char_clan_data *members; */
};
 
/* structure for clan_data.members - DM (not used as of yet anyway) */
struct char_clan_data {
  int clan_level;
  int idnum;
  struct char_player_data player;
 
  struct char_clan_data *next;
}; 

struct auction_lot {
        struct obj_data *obj;
        struct char_data *buyer;
        struct char_data *seller;
        long offer;
};  
