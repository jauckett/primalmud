/* ************************************************************************
*  file:  showplay.c                                  Part of CircleMud   *
*  Usage: list a diku playerfile                                          *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include "../conf.h"
#include "../sysdep.h"

#include "../structs.h"
#include "../utils.h"

const char *pc_class_types[] = {
  "Mag",
  "Cle",
  "Thi",
  "War",
  "Dru",
  "Pri",
  "Nig",
  "Bat",
  "Spe",
  "Pal",
  "Mas",
  "\n"
};     

const char *special_ability_bits[] = {
	"Perm-\nInvis",
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
	"ElfFighter",
	"\n"
};

/* Punishment Types - ARTUS */
const char *punish_types[] = {
  "Freeze",
  "Mute",
  "Notitle",
  "Laggy",
  "Low_Exp",
  "Low_Regen",
  "Aggravate",
  "\n"
};

/* Offence Types - ARTUS */
const char *offence_types[] = {
  "Pkill",
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

const char *timer_bits[] = 
{
  "HEALING_SKILL",
  "ADRENALINE",
  "\n"
};

/* AFF_x */
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
  "REPORTING",
  "\n"
};  

/* EXT_x */
const char *extended_bits[] = {
        "!NEWBIE",
        "!CTALK",
        "CLAN",
        "PKILL",
        "UNDEFINED",
        "AUTOGOLD",
        "AUTOLOOT",
        "AUTOSPLIT",
        "!HINT",
        "PKFROZEN",
        "\n"
};

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
  "ADRENALINE",
  "!DISARM",
  "\n"
};

const char *battery_types[] = {
  "mana",
  "\n"
};

const char *pc_race_types[] = {
  "Ogr",
  "Dev",
  "Min",
  "Elf",
  "Hum",
  "Dwa",
  "Pix",
  "\n"
};  

//#define IS_SET(flag,bit)  ((flag) && (bit))
/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
void sprintbit(bitvector_t bitvector, const char *names[], char *result)
{
  long nr;

  *result = '\0';

  for (nr = 0; bitvector; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      if (*names[nr] != '\n') {
	strcat(result, names[nr]);
	strcat(result, " ");
      } else
	strcat(result, "UNDEFINED ");
    }
    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    strcpy(result, "NOBITS ");
}

void show(char *filename)
{
  char buf[256], buf2[256];
  char sexname;
  char classname[10];
  char racename[10];
  FILE *fl;
  struct char_file_u player;
  int num = 0;
  long size;

  if (!(fl = fopen(filename, "r+"))) {
    perror("error opening playerfile");
    exit(1);
  }
  fseek(fl, 0L, SEEK_END);
  size = ftell(fl);
  rewind(fl);
  if (size % sizeof(struct char_file_u)) {
    fprintf(stderr, "\aWARNING:  File size does not match structure, recompile showplay.\n");
    fclose(fl);
    exit(1);
  }

  for (;;) {
    fread(&player, sizeof(struct char_file_u), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      exit(0);
    }

    printf("----------------------------------------"
	   "----------------------------------------\n");
    printf("Name: %-20s IDNUM: [%-8ld] Level: [%-3d]     Sex: %d\n", 
		    player.name, player.char_specials_saved.idnum, 
		    player.level, player.sex);
    printf("----------------------------------------"
           "----------------------------------------\n");
    printf("Title: '%s'\n", player.title);
    printf("Description:\n'%s'\n", player.description);

    if (player.chclass < 0 || player.chclass > 10) {
      strcpy(classname, "NONE");
    } else {
      sprintf(classname, "%s", pc_class_types[player.chclass]);
    }
    if (player.player_specials_primalsaved.race < 0 || 
		    player.player_specials_primalsaved.race > 7) {
      strcpy(racename, "NONE");
    } else {
      sprintf(racename, "%s", pc_race_types[player.player_specials_primalsaved.race]);
    }

    printf("Class: %-4s  Race: %-4s\n", classname, racename);
    printf("Hometown: [%d]  Birth: [%ld]  Played: [%d]\n"
	   "Weight: [%-3d]  Height: [%-3d]\n",
		    player.hometown, player.birth, player.played, 
		    player.weight, player.height); 
 
    printf("Email: '%s' Webpage: '%s'\nPersonal: '%s'\n", 
		    player.email, player.webpage, player.personal);
    printf("Poofin: '%s' Poofout: '%s'\n", player.poofin, player.poofout);
    printf("Prompt String: '%s'\n", player.prompt_string);
    printf("Password: '%s'\n", player.pwd);

    
    ///////////////////////
    // char_specials_saved
    ///////////////////////
    sprintbit(player.char_specials_saved.affected_by, affected_bits, buf);
    sprintbit(player.char_specials_saved.act, player_bits, buf2);

    printf("Alignment: [%d]\nPlr Flags: (%ld) %s\nAffected by: (%ld) %s\n", 
		    player.char_specials_saved.alignment, 
		    player.char_specials_saved.act, buf2, 
		    player.char_specials_saved.affected_by, buf);

    sprintbit(player.player_specials_saved.pref, preference_bits, buf);
    printf("Preferences: (%ld) %s\n", player.player_specials_saved.pref, buf); 

    printf("Saving Throws: [");
    for (int i = 0; i < 5; i++) {
      printf(" %d", (int)player.char_specials_saved.apply_saving_throw[i]);
    }
    printf("]\n");
  
    ///////////////////////
    // player_specials_saved
    ///////////////////////
   
// *****************************************************************************
// NOTE
// PADDING0 used to be spells_to_learn, but it seems that in the version we
// are using spells_to_learn anyhow, and not PADDING0 at all 
// *****************************************************************************
    printf("Wimp Level: [%d]  Freeze Level: [%d]  Invis Level: [%d]\n", 
		    player.player_specials_saved.wimp_level, 
		    player.player_specials_saved.freeze_level, 
		    player.player_specials_saved.invis_level);


    printf("Bad Pwds: [%d]  Conditions: [%d, %d, %d]\n", 
		    player.player_specials_saved.bad_pws, 
		    player.player_specials_saved.conditions[0], 
		    player.player_specials_saved.conditions[1], 
		    player.player_specials_saved.conditions[2]); 
    
    sprintbit(player.player_specials_saved.ptype, punish_types, buf);

    for (int i = 0; i < NUM_PUNISHES; i++) {
      if (IS_SET(player.player_specials_saved.ptype, (1 << i))) {
        printf("Punishment %d: %s %s %d hrs\n", i, 
			offence_types[player.player_specials_saved.offences[i]], 
			punish_types[i], player.player_specials_saved.phours[i]); 
      }
    }

    printf("Clan Num: [%d]  Clan rank: [%d]  Stat Pool: [%d]  Spells to Learn: [%d]\n",
		    player.player_specials_saved.clan, 
		    player.player_specials_saved.clan_rank, 
		    player.player_specials_saved.stat_pool, 
		    player.player_specials_saved.spells_to_learn);


    /*
    newpl.player_specials_saved.spare2 = 0; 
    newpl.player_specials_saved.spare3 = 0; 
    newpl.player_specials_saved.spare4 = 0; 
    newpl.player_specials_saved.spare5 = 0; 
    */

    printf("OLC Zone: [%d] Social Status: [%d] "
	   "Social Points: [%ld] Worship Points: [%ld]\n", 
		    player.player_specials_saved.olc_zone, 
		    player.player_specials_saved.social_status, 
		    player.player_specials_saved.social_points, 
		    player.player_specials_saved.worship_points);

    /*
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
*/

    
    ///////////////////////
    // primal_extend_data
    ///////////////////////

    for (int i = 0; i < MAX_QUEST_ITEMS; i++) {
      if (player.player_specials_primalsaved.quest_eq[i].vnum > 0) {
        printf("Quest Eq %d: vnum: [%d] max_num: [%d]\n", 
			i, player.player_specials_primalsaved.quest_eq[i].vnum, 
			player.player_specials_primalsaved.quest_eq[i].max_number);
      }
      for (int j = 0; j < MAX_NUM_ENHANCEMENTS; j++) {
	if (player.player_specials_primalsaved.quest_eq[i].enh_setting[j] > 0) {
	  printf("  Enhancement %d: %s Enhancements:", j, enhancement_names[player.player_specials_primalsaved.quest_eq[i].enh_setting[j]]);	
	}
        for (int k = 0; k < MAX_NUM_ENHANCEMENTS; k++) {
		
          if (player.player_specials_primalsaved.quest_eq[i].enhancements[j][k] > 0) {
            printf(" (%d) [%d]", k, player.player_specials_primalsaved.quest_eq[i].enhancements[j][k]);
	  }
        }
      }
    }

    printf("Kills: mobs: [%ld]  bymobs: [%ld]  pcs: [%ld]"
	   "bypcs: [%ld]  imms: [%ld]  byimms [%ld]\n", 
		    player.player_specials_primalsaved.kills.mobkills, 
		    player.player_specials_primalsaved.kills.killedbymobs, 
		    player.player_specials_primalsaved.kills.pckills, 
		    player.player_specials_primalsaved.kills.killedbypcs, 
		    player.player_specials_primalsaved.kills.immkills, 
		    player.player_specials_primalsaved.kills.killedbyimms); 

    printf("Ignoring: [");
    for (int i = 0; i < MAX_IGNORE; i++) {
      if (player.player_specials_primalsaved.ignore[i].playerid > 0) {
        printf(" %ld, all: %s\n", 
			player.player_specials_primalsaved.ignore[i].playerid, 
			(player.player_specials_primalsaved.ignore[i].allFlag) ? 
				"TRUE" : "FALSE");
      }
    }
    printf("]\n");

    printf("Friends: [");
    for (int i = 0; i < MAX_FRIENDS; i++) {
      if (player.player_specials_primalsaved.friends[i] > 0) {
        printf(" %ld", player.player_specials_primalsaved.friends[i]);
      }
    }
    printf("]\n");

    printf("World Entry Rooms: [%d, %d, %d]\n", 
		    player.player_specials_primalsaved.world_entry[0], 
		    player.player_specials_primalsaved.world_entry[1], 
		    player.player_specials_primalsaved.world_entry[2]);

   
    printf("Stat Points: [%d]  IgnoreLvl: [%d]  IgnoreLvlAll: [%d]\n", 
		    player.player_specials_primalsaved.stat_points, 
		    player.player_specials_primalsaved.ignorelvl, 
		    player.player_specials_primalsaved.ignorelvlall);

    int tmp = player.player_specials_primalsaved.invis_type_flag;

    printf("Invis type: '%s'\n", 
		    (tmp == -1) ? "Specific" : ((tmp == -1) ? "Single" : "Normal")); 

    sprintbit(player.player_specials_primalsaved.ext_flag, extended_bits, buf); 
    printf("Ext Flags: [%s]\n", buf);

    printf("Score bitvector: [%ld]\n", player.player_specials_primalsaved.score_flag);

    tmp = player.player_specials_primalsaved.fight_prompt;
    printf("Fight Prompt: (%d) '%s'", 
		    tmp, 
		    (tmp == 0) ? "Healometer" : ((tmp == 1) ? "Percentage" : "Stats"));


    sprintbit(player.player_specials_primalsaved.abilities, special_ability_bits, buf);
    printf("\nAbilities: [%s]\n", buf);

    sprintbit(player.player_specials_primalsaved.misc_specials, 
		    special_ability_bits, buf);

    printf("Misc Specials: [%s]\n", buf);

    printf("Char Disguised: [%ld], Char Memorised: [%ld]\n", 
		    player.player_specials_primalsaved.char_disguised, 
		    player.player_specials_primalsaved.char_memorised);
		  
    printf("Page Width: [%3d], Page Length: [%2d]\n", 
		    player.player_specials_primalsaved.page_width, 
		    player.player_specials_primalsaved.page_length);
    
    printf("Colour Settings: [");
    for (int i = 0; i < NUM_COLOUR_SETTINGS; i++) {
      printf(" %d", player.player_specials_primalsaved.colour_settings[i]);
    }

    printf("]\nLast Logon: [%ld, %s]\nLast UnsuccessfulLogon: [%ld, %s]\n", 
		    player.player_specials_primalsaved.last_logon, 
		    player.player_specials_primalsaved.host, 
		    player.player_specials_primalsaved.lastUnsuccessfulLogon, 
		    player.player_specials_primalsaved.lastUnsuccessfulHost);

    /*
   
    newpl.player_specials_primalsaved.spare1 = 0;
    newpl.player_specials_primalsaved.spare2 = 0;
    newpl.player_specials_primalsaved.spare3 = 0;
    newpl.player_specials_primalsaved.spare4 = 0;
    newpl.player_specials_primalsaved.spare5 = 0;
    newpl.player_specials_primalsaved.spare6 = 0;
    
    */
    
    printf("Start World: %d, Remort Lvl 1: [%3d], Remort Lvl 2: [%3d]\n", 
		    player.player_specials_primalsaved.start_world, 
		    player.player_specials_primalsaved.remort_one, 
		    player.player_specials_primalsaved.remort_two);
   
    printf("spesh_who: [");
    for (int i = 0; i < 20; i++) {
      printf(" %d", player.player_specials_primalsaved.spesh_who[i]);
    }
    printf("]\n");
 

    printf("STR: [%2d/%3d] INT: [%2d] WIS: [%2d] DEX: [%2d] CON: [%2d] CHA: [%2d]\n",
		    player.abilities.str,
		    player.abilities.str_add,
		    player.abilities.intel,
		    player.abilities.wis,
		    player.abilities.dex,
		    player.abilities.con,
		    player.abilities.cha);

    printf("Exp: [%d]  Hit: (%d/%d) Mana: (%d/%d) Move: (%d/%d)\n",
		    player.points.exp,
		    player.points.hit,
		    player.points.max_hit,
		    player.points.mana,
		    player.points.max_mana,
		    player.points.move,
		    player.points.max_move);

    printf("Armor: [%d] HR: [%d] DR: [%d] Gold: [%d] Bank: [%d]\n",
		    player.points.armor,
		    player.points.hitroll,
		    player.points.damroll,
		    player.points.gold,
		    player.points.bank_gold);

    tmp = 0;

    printf("Affects:");
    for (int i = 0; i < MAX_AFFECT; i++) {
      if (player.affected[i].type > 0) {
        tmp = 1;
        printf("\nType: [%d] Duration: [%d] Modifier: [%d] Location: [%d] Bitvector: [%ld]", 
			player.affected[i].type, 
			player.affected[i].duration, 
			player.affected[i].modifier, 
			player.affected[i].location, 
			player.affected[i].bitvector); 
      }
    }

    if (tmp == 0) {
      printf(" NONE\n");
    } else {
      printf("\n");
    }

    tmp = 0;

    printf("Timers:");
    for (int i = 0; i < MAX_TIMERS; i++) {
      if (player.timers[i].type > 0) {
	tmp = 1;
        printf("\nType: [%d] Duration: [%d] Uses: [%d] Max_Uses: [%d]",
			player.timers[i].type,
			player.timers[i].duration,
			player.timers[i].uses,
			player.timers[i].max_uses);
      }
    }
    
    if (tmp == 0) {
      printf(" NONE\n");
    } else {
      printf("\n");
    }

    printf("Skills:\n");

    // SKILLS - ok some have changed number and we now have an extra maximum 
    // of 150, so we need to take special care of them
    for (int i = 1; i < MAX_SKILLS+1; i++) {
      printf("%3d: %-3d%s", i, (int)player.player_specials_saved.skills[i], (i % 8 == 0) ? "\n" : " ");
    }
    printf("\n");

  }
}


int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    show(argv[1]);

  return (0);
}
