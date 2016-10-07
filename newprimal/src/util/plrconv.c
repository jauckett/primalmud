/* ************************************************************************
*  file: plrconv.c                                      Part of CircleMUD *
*  Usage: Convert the player file to 128 Bits		    Soren P. Skou *
*  All Rights Reserved                                                    *
*  Copyright (C) 1997, The Realm of Chaos				  *
**************************************************************************/

/* Modified by DM, to convert the old primal player file to the new one */
// *****************************************************************************

// *****************************************************************************

// #include "../conf.h"
// #include "../sysdep.h"

#include <stdio.h>
#include "../structs.h"
#include "../utils.h"

#define POOF_LENGTH 60

#define PM_ARRAY_MAX 4
#define AF_ARRAY_MAX 4
#define PR_ARRAY_MAX 4

/* The New format */
struct char_special_data_saved_new {
   int  alignment;              /* +-1000 for alignments                */
   long idnum;                  /* player's idnum; -1 for mobiles       */
   long /*bitvector_t*/ act;    /* act flag for NPC's; player flag for PC's */
 
   long /*bitvector_t*/ affected_by;
                                /* Bitvector for spells/skills affected by */
   sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)              */
};

struct player_special_data_saved_new {
   byte skills[MAX_SKILLS+1];   /* array of skills plus skill 0         */
   byte PADDING0;               /* used to be spells_to_learn           */
   bool talks[MAX_TONGUE];      /* PC s Tongues 0 for NPC               */
   ubyte wimp_level;            /* Below this # of hit points,flee!     */
   ubyte freeze_level;          /* Level of god who froze char, if any  */
   ubyte invis_level;           /* level of invisibility                */
   sh_int /*room_vnum*/ load_room;         /* Which room to place char in          */
   long /*bitvector_t*/ pref;   /* preference flags for PC's.           */
   ubyte bad_pws;               /* number of bad password attemps       */
   sbyte conditions[3];         /* Drunk, full, thirsty                 */
 
   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */
 
   ubyte spare0;
   ubyte spare1;
   ubyte spare2;
   ubyte spare3;
   ubyte spare4;
   ubyte spare5;
   int spells_to_learn;         /* How many can you learn yet this level*/
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
   long spare17;
   long spare18;
   long spare19;
   long spare20;
   long spare21;
}; 

struct primal_extend_data {
 
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
        long modifier;          /* XP modifier          */
        long race;              /* Race                 */

        int line_width;
        int page_length;
        char poofin[POOF_LENGTH+1];
        char poofout[POOF_LENGTH+1];
 
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


/* Definitions of the files... *ARGGH!* */

struct char_file_u_old {
   /* char_player_data */
   char name[MAX_NAME_LENGTH+1];
   char description[EXDSCR_LENGTH];
   char title[MAX_TITLE_LENGTH+1];
   byte sex;
   byte class;
   ubyte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int  played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;
 
   /* password */
   char pwd[MAX_PWD_LENGTH+1];
 
   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];
 
   time_t last_logon;           /* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];    /* host of last logon */
}; 

struct char_file_u_new {
   /* char_player_data */
   char name[MAX_NAME_LENGTH+1];
   char description[EXDSCR_LENGTH];
   char title[MAX_TITLE_LENGTH+1];
   byte sex;
   byte chclass;
   ubyte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int  played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;
 
   char email[MAX_INPUT_LENGTH + 1];
   char webpage[MAX_INPUT_LENGTH + 1];
   char personal[MAX_INPUT_LENGTH + 1];
 
   char pwd[MAX_PWD_LENGTH+1];    /* character's password */
 
   struct char_special_data_saved_new char_specials_saved;
   struct player_special_data_saved_new player_specials_saved;
   // New primal struct
   struct primal_extend_data player_specials_primalsaved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];
 
   time_t last_logon;           /* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];    /* host of last logon */
};

void convert(char *filename)
{
   FILE * fl;
   FILE * outfile;
   struct char_file_u_old oldpl;
   struct char_file_u_new newpl;
   int size;
   int recs;

   if (!(fl = fopen(filename, "r+"))) {
      printf("Can't open %s.", filename);
      exit(1);
   }

   outfile = fopen("players.new", "w");
   printf("Converting.\n");

 printf("Size of struct char_file_u_new: %d\n",sizeof(struct char_file_u_new));
 printf("Size of struct primal_extend_data: %d\n",sizeof(struct primal_extend_data));
 printf("Size of struct player_special_data_saved_new: %d\n",sizeof(struct player_special_data_saved_new));



   for (; ; ) {
      fread(&oldpl, sizeof(struct char_file_u), 1, fl);
      if (feof(fl)) {
         fclose(fl);
         fclose(outfile);
         puts("Done.");
   	 printf("Convert statistics:\n");
         outfile = fopen("players.new", "r");
         fseek(outfile, 0L, SEEK_END);
         size = ftell(outfile);
         recs = size / sizeof(struct char_file_u_new);
         if (size % sizeof(struct char_file_u_new))
           printf("WARNING:  PLAYERFILE IS PROBABLY CORRUPT!, recs %d\n",recs);
         else
           printf("  Recs in Pfile: %d\n",recs);
        fclose(outfile);
        exit(0);
      }

   /* char_player_data */
// *****************************************************************************
   // NOTE - make sure that struct kill_data is saved somewhere
// *****************************************************************************

   // memcpy(newpl.name, oldpl.name, MAX_NAME_LENGTH+1);
   strcpy(newpl.name, oldpl.name);
   // memcpy(newpl.description, oldpl.description, EXDSCR_LENGTH);
   strcpy(newpl.description, oldpl.description);
   // memcpy(newpl.title, oldpl.title, MAX_TITLE_LENGTH+1);
   strcpy(newpl.title, oldpl.title);
   newpl.sex = oldpl.sex;

// *****************************************************************************
   // NOTE
   // Primal did not save the PC's class - when u detect chclass = -1,
   // prompt the player to choose a class as it is the first time they
   // have logged on with the new playerfile
   // also, player_specials_primalsaved.modifier will = -1. 
// *****************************************************************************
   newpl.chclass = -1;

   newpl.level = oldpl.level;
   newpl.hometown = oldpl.hometown;
   newpl.birth = oldpl.birth;
   newpl.played = oldpl.played; 
   newpl.weight = oldpl.weight;
   newpl.height = oldpl.height;
 
   strcpy(newpl.email,"");
   strcpy(newpl.webpage,"");
   strcpy(newpl.personal,"");
   // newpl.email[0] = '\0';
   // newpl.webpage[0] = '\0';
   // newpl.personal[0] = '\0';
 
   //memcpy(newpl.pwd, oldpl.pwd, MAX_PWD_LENGTH+1);
   strcpy(newpl.pwd, oldpl.pwd);

   newpl.char_specials_saved.alignment = oldpl.char_specials_saved.alignment;
   newpl.char_specials_saved.idnum = oldpl.char_specials_saved.idnum;
   newpl.char_specials_saved.act = oldpl.char_specials_saved.act;
   newpl.char_specials_saved.affected_by = oldpl.char_specials_saved.affected_by;
   memcpy(newpl.char_specials_saved.apply_saving_throw, oldpl.char_specials_saved.apply_saving_throw, 5);
  
   memcpy(newpl.player_specials_saved.skills, oldpl.player_specials_saved.skills, MAX_SKILLS+1);

// *****************************************************************************
// NOTE
// PADDING0 used to be spells_to_learn, but it seems that in the version we
// are using spells_to_learn anyhow, and not PADDING0 at all 
// *****************************************************************************
   newpl.player_specials_saved.PADDING0 = oldpl.player_specials_saved.spells_to_learn;

   memcpy(newpl.player_specials_saved.talks, oldpl.player_specials_saved.talks, MAX_TONGUE);
   newpl.player_specials_saved.wimp_level = oldpl.player_specials_saved.wimp_level;
   newpl.player_specials_saved.freeze_level = oldpl.player_specials_saved.freeze_level;
   newpl.player_specials_saved.invis_level = oldpl.player_specials_saved.invis_level;
   newpl.player_specials_saved.load_room = oldpl.player_specials_saved.load_room;
   newpl.player_specials_saved.pref = oldpl.player_specials_saved.pref;
   newpl.player_specials_saved.bad_pws = oldpl.player_specials_saved.bad_pws;
   memcpy(newpl.player_specials_saved.conditions, oldpl.player_specials_saved.conditions, 3);
   newpl.player_specials_saved.spare0 = 0; 
   newpl.player_specials_saved.spare1 = 0; 
   newpl.player_specials_saved.spare2 = 0; 
   newpl.player_specials_saved.spare3 = 0; 
   newpl.player_specials_saved.spare4 = 0; 
   newpl.player_specials_saved.spare5 = 0; 
 
   newpl.player_specials_saved.spells_to_learn = oldpl.player_specials_saved.spells_to_learn;
   newpl.player_specials_saved.olc_zone = -1; 

   newpl.player_specials_saved.spare8 = 0; 
   newpl.player_specials_saved.spare9 = 0; 
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
   newpl.player_specials_saved.spare20 = 0; 
   newpl.player_specials_saved.spare21 = 0; 

   memcpy(newpl.player_specials_primalsaved.world_entry, oldpl.player_specials_saved.world_entry,3);
   newpl.player_specials_primalsaved.ignore1 = oldpl.player_specials_saved.ignore1;
   newpl.player_specials_primalsaved.ignore2 = oldpl.player_specials_saved.ignore2;
   newpl.player_specials_primalsaved.ignore3 = oldpl.player_specials_saved.ignore3;
   newpl.player_specials_primalsaved.ignorelev = oldpl.player_specials_saved.ignorelev;
   newpl.player_specials_primalsaved.ignorenum = oldpl.player_specials_saved.ignorenum;
   newpl.player_specials_primalsaved.clan_num = oldpl.player_specials_saved.clan_num;
   newpl.player_specials_primalsaved.clan_level = oldpl.player_specials_saved.clan_level;
   newpl.player_specials_primalsaved.invis_type_flag = oldpl.player_specials_saved.invis_type_flag;
   newpl.player_specials_primalsaved.ext_flag = oldpl.player_specials_saved.ext_flag;
   newpl.player_specials_primalsaved.modifier = -1;

   newpl.player_specials_primalsaved.line_width = 80;
   newpl.player_specials_primalsaved.page_length = 22;
   newpl.player_specials_primalsaved.poofin[0]='\0'; 
   newpl.player_specials_primalsaved.poofout[0]='\0'; 

// *****************************************************************************
// NOTE
// newpl.player_specials_primalsaved.race
//     long race;
// Primal only saved the race type, which was: byte class in struct char_file_u
// *****************************************************************************
   newpl.player_specials_primalsaved.race = oldpl.class;

   newpl.player_specials_primalsaved.spare1 = 0;
   newpl.player_specials_primalsaved.spare2 = 0;
   newpl.player_specials_primalsaved.spare3 = 0;
   newpl.player_specials_primalsaved.spare4 = 0;
   newpl.player_specials_primalsaved.spare5 = 0;
   newpl.player_specials_primalsaved.spare6 = 0;
   newpl.player_specials_primalsaved.spare7 = 0;
   newpl.player_specials_primalsaved.spare8 = 0;
   newpl.player_specials_primalsaved.spare9 = 0;
 
   memcpy(&newpl.abilities, &oldpl.abilities, sizeof(struct char_ability_data));
   memcpy(&newpl.points, &oldpl.points, sizeof(struct char_point_data));
   memcpy(&newpl.affected, &oldpl.affected, MAX_AFFECT * sizeof(struct char_point_data));

   newpl.last_logon = oldpl.last_logon;
   memcpy(newpl.host, oldpl.host, HOST_LENGTH+1);
 
   printf("Converting User: [ %3d lw: %d pl: %d] %s %s\n", newpl.level, newpl.player_specials_primalsaved.line_width, newpl.player_specials_primalsaved.page_length, newpl.name, newpl.player_specials_primalsaved.poofin);

   fwrite(&newpl, sizeof(struct char_file_u_new ), 1, outfile);

/*   fseek(outfile, 0L, SEEK_END);
   size = ftell(outfile);
   recs = size / sizeof(struct char_file_u_new);
   if (size % sizeof(struct char_file_u_new))
     printf("  WARNING:  PLAYERFILE IS PROBABLY CORRUPT!, recs %d\n",recs);
   else
     printf("  Recs in Pfile: %d\n",recs);
*/

  }


}

void main(int argc, char *argv[])
{
   if (argc != 2)
      printf("Usage: %s playerfile-name\n", argv[0]);
   else
      convert(argv[1]);
}
