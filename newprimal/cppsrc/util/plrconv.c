/* ************************************************************************
*  file: plrconv.c                                      Part of CircleMUD *
*  Usage: Convert the player file to 128 Bits		    Soren P. Skou *
*  All Rights Reserved                                                    *
*  Copyright (C) 1997, The Realm of Chaos				  *
**************************************************************************/

/* Modified by DM, to convert the old primal player file to the new one */

// Modified DM, 28/12/2001 for latest structs
// (and changed the way things are read - now we use _old structs instead of
// _new structs for a lot easier maintenance. Previously we were reading the 
// old structs.h and utils.h files, now we are reading the new

#include <stdio.h>
#include "../conf.h"
#include "../sysdep.h"
#include "../structs.h"
#include "../utils.h"

#define OLD_MAX_SKILLS 200 
#define OLD_HOST_LENGTH 30
#define OLD_MAX_TONGUE 3
#define OLD_MAX_NAME_LENGTH 20
#define OLD_EXDSCR_LENGTH 240
#define OLD_MAX_TITLE_LENGTH 80 
#define OLD_MAX_PWD_LENGTH 10 
#define OLD_MAX_AFFECT 32

#define PM_ARRAY_MAX 4
#define AF_ARRAY_MAX 4
#define PR_ARRAY_MAX 4

bool debug = FALSE;
time_t start_time = time(0); 

////////////////////////////////////////////////////////////////////////////////
// OLD STRUCTURES
////////////////////////////////////////////////////////////////////////////////

/* Definitions of the files... *ARGGH!* */

struct char_special_data_saved_old {
   int  alignment;              /* +-1000 for alignments                */
   long idnum;                  /* player's idnum; -1 for mobiles       */
   long act;    /* act flag for NPC's; player flag for PC's */
 
   long affected_by; /* Bitvector for spells/skills affected by */
   sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)              */
};

struct player_special_data_saved_old {
   ubyte skills[OLD_MAX_SKILLS+1];   /* array of skills plus skill 0         */
   ubyte spells_to_learn;
   bool talks[OLD_MAX_TONGUE];      /* PC s Tongues 0 for NPC               */
   int  wimp_level;            /* Below this # of hit points,flee!     */
   ubyte freeze_level;          /* Level of god who froze char, if any  */
   sh_int invis_level;           /* level of invisibility                */
   sh_int load_room;         /* Which room to place char in          */
   long pref;   /* preference flags for PC's.           */
   ubyte bad_pws;               /* number of bad password attemps       */
   sbyte conditions[3];         /* Drunk, full, thirsty                 */
 
   ubyte stat_order[6];

   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */
 
   int world_entry[3];
   int ignore1;
   int ignore2;
   int ignore3;
   int ignorelev;
   int ignorenum;
   int clan_num;
   int clan_level;
   int invis_type_flag;
   long ext_flag;
   long spare18;
   long spare19;
   long	spare20;
   long spare21;
}; 

struct char_ability_data_old {
  sbyte str;
  sbyte str_add;
  sbyte intel;
  sbyte wis;
  sbyte dex;
  sbyte con;
  sbyte cha;
};

struct char_point_data_old {
  sh_int mana;
  sh_int max_mana;
  sh_int hit;
  sh_int max_hit;
  sh_int move;
  sh_int max_move;

  sh_int armor;
  int gold;
  int bank_gold;
  int exp;

  sbyte hitroll;
  sbyte damroll;
};

struct affected_type_old {
  sh_int type;
  sh_int duration;
  sbyte modifier;
  byte location;
  long bitvector;

  struct affected_type_old *next;
  
};

struct char_file_u_old {
   /* char_player_data */
   char name[OLD_MAX_NAME_LENGTH+1];
   char description[OLD_EXDSCR_LENGTH];
   char title[OLD_MAX_TITLE_LENGTH+1];
   byte sex;
   byte chclass;
   ubyte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int  played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;
 
   /* password */
   char pwd[OLD_MAX_PWD_LENGTH+1];
 
   struct char_special_data_saved_old char_specials_saved;
   struct player_special_data_saved_old player_specials_saved;
   struct char_ability_data_old abilities;
   struct char_point_data_old points;
   struct affected_type_old affected[OLD_MAX_AFFECT];
 
   time_t last_logon;           /* Time (in secs) of last logon */
   char host[OLD_HOST_LENGTH+1];    /* host of last logon */
}; 

// Values are the Index position in const char *COLOURLIST[] (colour.c) 
cpp_extern const int default_colour_codes[] = {
  2,    //"Headings",
  3,    //"Sub Headings",
  14,   //"Help Headings",
  2,    //"Help Attribute Headings",
  6,    //"Commands",
  2,    //"Objects",
  3,    //"Mobs",
  3,    //"Players",
  6,    //"Room Headings",
  0     //"Unused"
};

// This array maps the previous spells and skill numbers to the new numbers
// TODO: check diffs
int skill_array[MAX_SKILLS+1] = { 
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
	70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
	90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 
	110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
	130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
	140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
	150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
	170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
	180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
	190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
	200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 
	210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
	220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
	230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
	250, 251, 252, 253, 254, 255, 256, 257, 258, 259,
	260, 261, 262, 263, 264, 265, 266, 267, 268, 269,
	270, 271, 272, 273, 274, 275, 276, 277, 278, 279,
	280, 281, 282, 283, 284, 285, 286, 287, 288, 289,
	290, 291, 292, 293, 294, 295, 296, 297, 298, 299,
	300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 
	310, 311, 312, 313, 314, 315, 316, 317, 318, 319,
	320, 321, 322, 323, 324, 325, 326, 327, 328, 329,
	330, 331, 332, 333, 334, 335, 336, 337, 338, 339,
	340, 341, 342, 343, 344, 345, 346, 347, 348, 349,
	350
};

byte skill_set(struct char_file_u_old oldpl, int skill_num) {
  if (skill_num > OLD_MAX_SKILLS) {
    return 0;
  }

  // returns the ability of the "mapped skill"
  // (for SKILL/SPELL define differences)
  return oldpl.player_specials_saved.skills[skill_array[skill_num]];
}

void convert(char *filename) {
  FILE * fl;
  FILE * outfile;
  struct char_file_u_old oldpl;
  struct char_file_u newpl;
  int size;
  int recs;

  if (!(fl = fopen(filename, "r+"))) {
    printf("Can't open %s.", filename);
    exit(1);
  }

  outfile = fopen("players.new", "w");
  printf("Converting.\n");

  if (debug) {
    printf("Size of struct char_file_u: %d\n",
		     sizeof(struct char_file_u));
    printf("Size of struct char_file_u_old: %d\n",
		     sizeof(struct char_file_u_old));
    printf("Size of struct char_special_data_saved_old: %d\n",
		     sizeof(struct char_special_data_saved_old));
    printf("Size of struct player_special_data_saved_old: %d\n",
		     sizeof(struct player_special_data_saved_old));
    printf("Size of struct char_ability_data_old: %d\n",
		     sizeof(struct char_ability_data_old));
    printf("Size of struct char_point_data_old: %d\n",
		     sizeof(struct char_point_data_old));
    printf("Size of struct affected_type_old: %d\n",
		     sizeof(struct affected_type_old));
  }
 
  for (; ; ) {
    fread(&oldpl, sizeof(struct char_file_u_old), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      fclose(outfile);
      puts("Done.");
      printf("Convert statistics:\n");
      outfile = fopen("players.new", "r");
      fseek(outfile, 0L, SEEK_END);
      size = ftell(outfile);
      recs = size / sizeof(struct char_file_u);
      if (size % sizeof(struct char_file_u))
        printf("WARNING:  PLAYERFILE IS PROBABLY CORRUPT!, recs %d\n",recs);
      else
        printf("  Recs in Pfile: %d\n",recs);
      printf("  Convert Time:  %ld secs\n", (time(0) - start_time) % 3600);
		    
      fclose(outfile);
      exit(0);
    }

   ///////////////////////
   // char_file_u
   ///////////////////////

    strcpy(newpl.name, oldpl.name);
    strcpy(newpl.description, oldpl.description);
    strcpy(newpl.title, oldpl.title);
    newpl.sex = oldpl.sex;

// *****************************************************************************
   // NOTE
   // Primal did not save the PC's class - when u detect chclass = -1,
   // prompt the player to choose a class as it is the first time they
   // have logged on with the new playerfile
   // also, player_specials_primalsaved.modifier will = -1. 
// *****************************************************************************
    newpl.chclass = 0;

    newpl.level = oldpl.level;
    newpl.hometown = oldpl.hometown;
    newpl.birth = oldpl.birth;
    newpl.played = oldpl.played; 
    newpl.weight = oldpl.weight;
    newpl.height = oldpl.height;
 
    strcpy(newpl.email,"");
    strcpy(newpl.webpage,"");
    strcpy(newpl.personal,"");

    strcpy(newpl.poofin,"");
    strcpy(newpl.poofout,"");
 
    strcpy(newpl.prompt_string,"");

    strcpy(newpl.pwd, oldpl.pwd);

    ///////////////////////
    // char_specials_saved
    ///////////////////////
    newpl.char_specials_saved.alignment = oldpl.char_specials_saved.alignment;
    newpl.char_specials_saved.idnum = oldpl.char_specials_saved.idnum;
    newpl.char_specials_saved.act = oldpl.char_specials_saved.act;
    newpl.char_specials_saved.affected_by = 
	    oldpl.char_specials_saved.affected_by;
    memcpy(newpl.char_specials_saved.apply_saving_throw, 
		    oldpl.char_specials_saved.apply_saving_throw, 5);
  
    ///////////////////////
    // player_specials_saved
    ///////////////////////
   
    // SKILLS - ok some have changed number and we now have an extra maximum 
    // of 150, so we need to take special care of them
    for (int i = 0; i < MAX_SKILLS; i++) {
      newpl.player_specials_saved.skills[i] = skill_set(oldpl, i);
    }

// *****************************************************************************
// NOTE
// PADDING0 used to be spells_to_learn, but it seems that in the version we
// are using spells_to_learn anyhow, and not PADDING0 at all 
// *****************************************************************************
    newpl.player_specials_saved.PADDING0 = 
	   oldpl.player_specials_saved.spells_to_learn;

    memcpy(newpl.player_specials_saved.talks, 
		   oldpl.player_specials_saved.talks, MAX_TONGUE);
    newpl.player_specials_saved.wimp_level = 
	   oldpl.player_specials_saved.wimp_level;
    newpl.player_specials_saved.freeze_level = 
	   oldpl.player_specials_saved.freeze_level;
    newpl.player_specials_saved.invis_level = 
	   oldpl.player_specials_saved.invis_level;
    newpl.player_specials_saved.load_room = -1; // Depreciated 

    newpl.player_specials_saved.pref = oldpl.player_specials_saved.pref;
    newpl.player_specials_saved.bad_pws = oldpl.player_specials_saved.bad_pws;
    memcpy(newpl.player_specials_saved.conditions, 
		   oldpl.player_specials_saved.conditions, 3);

    newpl.player_specials_saved.ptype = 0;
    for (int i = 0; i < 32; i++)
      newpl.player_specials_saved.phours[i] = 0;
    for (int i = 0; i < 32; i++)
      newpl.player_specials_saved.offences[i] = 0;

    newpl.player_specials_saved.clan_rank = 0;
    newpl.player_specials_saved.stat_pool = 0;

    newpl.player_specials_saved.spare2 = 0; 
    newpl.player_specials_saved.spare3 = 0; 
    newpl.player_specials_saved.spare4 = 0; 
    newpl.player_specials_saved.spare5 = 0; 
 
    newpl.player_specials_saved.spells_to_learn = 
	   oldpl.player_specials_saved.spells_to_learn;
    newpl.player_specials_saved.olc_zone = (newpl.level >= LVL_IMPL) ? 0 : -1;
    newpl.player_specials_saved.social_status = 0; 
    newpl.player_specials_saved.clan = 0; 

    newpl.player_specials_saved.spare10 = 0; 
    newpl.player_specials_saved.spare11 = 0; 
    newpl.player_specials_saved.spare12 = 0; 
    newpl.player_specials_saved.spare13 = 0; 
    newpl.player_specials_saved.spare14 = 0; 
    newpl.player_specials_saved.spare15 = 0; 
    newpl.player_specials_saved.spare16 = 0; 
    newpl.player_specials_saved.spare17 = 0; 
    newpl.player_specials_saved.spare18 = 0; 
    newpl.player_specials_saved.spare19 = 0; 

    newpl.player_specials_saved.social_points = 0; 
    newpl.player_specials_saved.worship_points = 0; 

    ///////////////////////
    // primal_extend_data
    ///////////////////////

    for (int i = 0; i < MAX_QUEST_ITEMS; i++) {
      newpl.player_specials_primalsaved.quest_eq[i].vnum = 0;
      newpl.player_specials_primalsaved.quest_eq[i].max_number = 0;
      for (int j = 0; j < MAX_NUM_ENHANCEMENTS; j++) {
        newpl.player_specials_primalsaved.quest_eq[i].enh_setting[j] = 0;
        for (int k = 0; k < MAX_NUM_ENHANCEMENTS; k++) {
          newpl.player_specials_primalsaved.quest_eq[i].enhancements[j][k] = 0;
        }
      }
      newpl.player_specials_primalsaved.quest_eq[i].owner = NULL;
    }

    newpl.player_specials_primalsaved.kills.mobkills = 0;
    newpl.player_specials_primalsaved.kills.killedbymobs = 0;
    newpl.player_specials_primalsaved.kills.pckills = 0;
    newpl.player_specials_primalsaved.kills.killedbypcs = 0;
    newpl.player_specials_primalsaved.kills.immkills = 0;
    newpl.player_specials_primalsaved.kills.killedbyimms = 0;

    newpl.player_specials_primalsaved.kills.sparekilldata1 = 0;
    newpl.player_specials_primalsaved.kills.sparekilldata2 = 0;
    newpl.player_specials_primalsaved.kills.sparekilldata3 = 0;

    for (int i = 0; i < MAX_IGNORE; i++) {
      newpl.player_specials_primalsaved.ignore[i].playerid = 0;
      newpl.player_specials_primalsaved.ignore[i].allFlag = 0;
    }

    for (int i = 0; i < MAX_FRIENDS; i++) {
      newpl.player_specials_primalsaved.friends[i] = 0;
    }

    for (int i = 0; i < NUM_WORLDS; i++) {
      newpl.player_specials_primalsaved.world_entry[i] = -1;
    }
   
    newpl.player_specials_primalsaved.stat_points = 0;
    newpl.player_specials_primalsaved.ignorelvl = 0;
    newpl.player_specials_primalsaved.ignorelvlall = 0;

    newpl.player_specials_primalsaved.invis_type_flag = 0;
    newpl.player_specials_primalsaved.ext_flag = 0;
    newpl.player_specials_primalsaved.score_flag = 0;
    newpl.player_specials_primalsaved.fight_prompt = 0;
   
    newpl.player_specials_primalsaved.abilities = 0;
    newpl.player_specials_primalsaved.misc_specials = 0;

// *****************************************************************************
// NOTE
// newpl.player_specials_primalsaved.race
//     long race;
// Primal only saved the race type, which was: byte class in struct char_file_u
// *****************************************************************************
// ... Umm why is it a long???
//
// TODO: handle old races (we have only 7 in new base)
    newpl.player_specials_primalsaved.race = oldpl.chclass;

    newpl.player_specials_primalsaved.char_disguised = 0;
    newpl.player_specials_primalsaved.char_memorised = 0;

    newpl.player_specials_primalsaved.page_width = 80;
    newpl.player_specials_primalsaved.page_length = 22;

    for (int i = 0; i < NUM_COLOUR_SETTINGS; i++)
	    newpl.player_specials_primalsaved.colour_settings[i] =
		    default_colour_codes[i];

    newpl.player_specials_primalsaved.last_logon = oldpl.last_logon;
    newpl.player_specials_primalsaved.lastUnsuccessfulLogon = 0; 
    strcpy(newpl.player_specials_primalsaved.host, oldpl.host);
    strcpy(newpl.player_specials_primalsaved.lastUnsuccessfulHost, "-");

    newpl.player_specials_primalsaved.spare1 = 0;
    newpl.player_specials_primalsaved.spare2 = 0;
    newpl.player_specials_primalsaved.spare3 = 0;
    newpl.player_specials_primalsaved.spare4 = 0;
    newpl.player_specials_primalsaved.spare5 = 0;
    newpl.player_specials_primalsaved.spare6 = 0;
    newpl.player_specials_primalsaved.start_world = 0;
    newpl.player_specials_primalsaved.remort_one = 0;
    newpl.player_specials_primalsaved.remort_two = 0;
   
    for (int i = 0; i < 20; i++) {
      newpl.player_specials_primalsaved.spesh_who[i] = 0;
    }
 
    ///////////////////
    // char_ability_data
    ///////////////////
    memcpy(&newpl.abilities, &oldpl.abilities, 
		    sizeof(struct char_ability_data));

    ///////////////////
    // char_point_data
    ///////////////////
    newpl.points.mana = oldpl.points.mana;
    newpl.points.max_mana = oldpl.points.max_mana;
    newpl.points.hit = oldpl.points.hit;
    newpl.points.max_hit = oldpl.points.max_hit;
    newpl.points.move = oldpl.points.move;
    newpl.points.max_move = oldpl.points.max_move;

    newpl.points.armor = oldpl.points.armor;
    newpl.points.gold = oldpl.points.gold;
    newpl.points.bank_gold = oldpl.points.bank_gold;
    newpl.points.exp = 0; //oldpl.points.exp;
    newpl.points.hitroll = oldpl.points.hitroll;
    newpl.points.damroll = oldpl.points.damroll;

    //memcpy(&newpl.points, &oldpl.points, sizeof(struct char_point_data));

    ///////////////////
    // affected_type 
    ///////////////////
    memcpy(&newpl.affected, &oldpl.affected, 
		   MAX_AFFECT * sizeof(struct char_point_data));
    ///////////////////
    // timer_type 
    ///////////////////
    for (int i = 0; i < MAX_TIMERS; i++) {
      newpl.timers[i].type = 0;
      newpl.timers[i].duration = 0;
      newpl.timers[i].uses = 0;
      newpl.timers[i].max_uses = 0;

      newpl.timers[i].next = 0;
    }

    printf("Converting User: [ %3ld ] %20s (%3d)\n", 
		    newpl.char_specials_saved.idnum, newpl.name, newpl.level);

    fwrite(&newpl, sizeof(struct char_file_u), 1, outfile);
  }
}

int main(int argc, char *argv[]) {
   if (argc != 2)
      printf("Usage: %s playerfile-name\n", argv[0]);
   else
      convert(argv[1]);
}
