/*************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"

extern int level_exp[LVL_CHAMP + 1];

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

struct room_data *world = NULL;	/* array of rooms		 */
int top_of_world = 0;		/* ref to top element of world	 */

struct char_data *character_list = NULL;	/* global linked list of
						 * chars	 */
struct index_data *mob_index;	/* index table for mobile file	 */
struct char_data *mob_proto;	/* prototypes for mobs		 */
int top_of_mobt = 0;		/* top of mobile index table	 */

struct obj_data *object_list = NULL;	/* global linked list of objs	 */
struct index_data *obj_index;	/* index table for object file	 */
struct obj_data *obj_proto;	/* prototypes for objs		 */
int top_of_objt = 0;		/* top of object index table	 */

struct zone_data *zone_table;	/* zone table			 */
int top_of_zone_table = 0;	/* top element of zone tab	 */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */

struct player_index_element *player_table = NULL;	/* index to plr file	 */
FILE *player_fl = NULL;		/* file desc of player file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;		/* ref of size of p file	 */
long top_idnum = 0;		/* highest idnum in use		 */

int mainten = 0;		/* maintenence mode		 */
int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
long boot_time = 0;		/* time of mud boot		 */
int restrict = 0;		/* level of game restriction	 */
sh_int r_mortal_start_room;	/* rnum of mortal start room	 */
sh_int r_eternal_start_room;	/* rnum of eternal start room	 */
sh_int r_immort_start_room;	/* rnum of immort start room	 */
sh_int r_frozen_start_room;	/* rnum of frozen start room	 */

char *credits = NULL;		/* game credits			 */
char *news = NULL;		/* mud news			 */
char *motd = NULL;		/* message of the day - mortals */
char *imotd = NULL;		/* message of the day - immorts */
char *help = NULL;		/* help screen			 */
char *info = NULL;		/* info page			 */
char *wizlist = NULL;		/* list of higher gods		 */
char *immlist = NULL;		/* list of peon gods		 */
char *background = NULL;	/* background story		 */
char *handbook = NULL;		/* handbook for new immortals	 */
char *policies = NULL;		/* policies page		 */
char *areas = NULL;		/* areas, zones			 */

FILE *help_fl = NULL;		/* file for help text		 */
struct help_index_element *help_index = 0;	/* the help table	 */
int top_of_helpt;		/* top of help index table	 */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	 */
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

/* local functions */
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
void char_to_store(struct char_data * ch, struct char_file_u * st);
void store_to_char(struct char_file_u * st, struct char_data * ch);
int is_empty(int zone_nr);
void reset_zone(int zone);
int file_to_string(char *name, char *buf);
int file_to_string_alloc(char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(int zone, int cmd_no, char *message);
void reset_time(void);
void clear_char(struct char_data * ch);
void clan_boot(void);
/* void build_clan_indexs(void); */
void copy_clan_tables(void);
void add_clan_table(struct char_file_u ch);
void init_blackjack(void);
void init_race(void);

/* external functions */
extern char *ALIAS_DIRNAME;
extern struct descriptor_data *descriptor_list;
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void update_obj_file(void);	/* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
struct help_index_element *build_help_index(FILE * fl, int *num);
extern int check_clan(struct char_data *ch, struct char_data *vict ,int sub_allow); 
void boot_social_messages(void);


/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}


ACMD(do_reboot)
{
  int i;

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(AREA_FILE, &areas);
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
  } else if (!str_cmp(arg, "wizlist"))
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
  else if (!str_cmp(arg, "immlist"))
    file_to_string_alloc(IMMLIST_FILE, &immlist);
  else if (!str_cmp(arg, "news"))
    file_to_string_alloc(NEWS_FILE, &news);
  else if (!str_cmp(arg, "credits"))
    file_to_string_alloc(CREDITS_FILE, &credits);
  else if (!str_cmp(arg, "motd"))
    file_to_string_alloc(MOTD_FILE, &motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string_alloc(IMOTD_FILE, &imotd);
  else if (!str_cmp(arg, "help"))
    file_to_string_alloc(HELP_PAGE_FILE, &help);
  else if (!str_cmp(arg, "info"))
    file_to_string_alloc(INFO_FILE, &info);
  else if (!str_cmp(arg, "areas"))
    file_to_string_alloc(AREA_FILE, &areas);
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "background"))
    file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "xhelp")) {
    if (help_fl)
      fclose(help_fl);
    if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
      return;
    else {
      for (i = 0; i < top_of_helpt; i++)
	free(help_index[i].keyword);
      free(help_index);
      help_index = build_help_index(help_fl, &top_of_helpt);
    }
  } else {
    send_to_char("Unknown reboot option.\r\n", ch);
    return;
  }

  send_to_char(OK, ch);
}

/* body of the booting system */
void boot_db(void)
{
  int i;
  extern int no_specials;

  log("Boot db -- BEGIN.");
 
  log("Resetting the game time:");
  reset_time();

  log("Reading news, credits, help, bground, areas, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(AREA_FILE, &areas);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);

  log("Opening help file.");
  if (!(help_fl = fopen(HELP_KWRD_FILE, "r")))
    log("   Could not open help file.");
  else
    help_index = build_help_index(help_fl, &top_of_helpt);

  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Renumbering zone table.");
  renum_zone_table();

  if (!mini_mud) {
	log("Booting clan file.");
    clan_boot();
  }
  boot_time = time(0);

  log("Generating player index.");
  build_player_index();
/*
  log("Generating clan indexes.");
  build_clan_indexs();
*/

  log("Copying clan tables.");
  copy_clan_tables();

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();

  if (!no_specials) {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }
  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
  }
  log("   Spells.");
  mag_assign_spells();

  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  log("Booting mail system.");
  if (!scan_file()) {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("Done.");
  }
  for (i = 0; i <= top_of_zone_table; i++) {
    sprintf(buf2, "Resetting %s (rooms %d-%d).",
	    zone_table[i].name, (i ? (zone_table[i - 1].top + 1) : 0),
	    zone_table[i].top);
    log(buf2);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

  if (!mini_mud) {
    log("Booting houses.");
    House_boot();
    log("Initialising gambling structures.");
    init_race();
    init_blackjack();
  }

  boot_time = time(0);

  log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
  long beginning_of_time = 650336715;
  struct time_info_data mud_time_passed(time_t t2, time_t t1);

  time_info = mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  sprintf(buf, "   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);
  log(buf);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;

/* set the initial moon value (random) - Vader */
  weather_info.moon = number(MOON_NONE,MOON_FINAL_QTR);

}



/* generate index table for the player file */
void build_player_index(void)
{
  /* char s[100]; */

  int nr = -1, i;
  long size, recs;
  struct char_file_u dummy;

  if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
    perror("Error opening playerfile");
    exit(1);
  }

  fseek(player_fl, 0L, SEEK_END);
  size = ftell(player_fl);
  rewind(player_fl);
  if (size % sizeof(struct char_file_u))
    fprintf(stderr, "WARNING:  PLAYERFILE IS PROBABLY CORRUPT!\n");
  recs = size / sizeof(struct char_file_u);
  if (recs) {
    sprintf(buf, "   %ld players in database.", recs);
    log(buf);
    CREATE(player_table, struct player_index_element, recs);
  } else {
    player_table = 0;
    top_of_p_file = top_of_p_table = -1;
    return;
  }

  for (; !feof(player_fl);) {
    fread(&dummy, sizeof(struct char_file_u), 1, player_fl);
    if (!feof(player_fl)) {	/* new record */
      /* DM */
      add_clan_table(dummy);

      nr++;
      CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
      for (i = 0;
	   (*(player_table[nr].name + i) = LOWER(*(dummy.name + i))); i++) {
        player_table[nr].id = dummy.char_specials_saved.idnum;
      }
      top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum);
if (top_idnum>20000) 
{
/* sprintf(s,"DEBUG IDNUM: top_idnum=%ld charsave.idnum=%ld\n",top_idnum,
dummy.char_specials_saved.idnum); 
log(s);  */
}

    }
  }

  top_of_p_file = top_of_p_table = nr;
}

/* DM - Add clan members to clan_info[].members 
void build_clan_indexs(void)
{
  int clan_num, nr = -1, i;
  long size, recs;
  struct char_file_u dummy;
  FILE *fp;
 
  if (!(fp = fopen(PLAYER_FILE, "rb"))) {
    perror("Error opening playerfile");
    exit(1);
  }
 
  for (; !feof(fp);) {
    fread(&dummy, sizeof(struct char_file_u), 1, fp);
    if (!feof(player_fl)) {     
      clan_num=dummy.player_specials_saved.clan_num;

      if ((clan_num > 0) && (clan_num < CLAN_NUM))
        if ((curr=load_char(dummy.name

      CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);

} */

/* DM - add player to clan table */
void add_clan_table(struct char_file_u ch)
{
  int i;
  int clan_n=ch.player_specials_saved.clan_num;
  int clan_r=ch.player_specials_saved.clan_level;

  /* Check for valid clan */
  if (((clan_n > 0) && (clan_n < CLAN_NUM)) &&
      ((clan_r >= 0) && (clan_r < RANK_NUM))) {

    sprintf(buf,"%s",ch.name);
    
    for (i=0;i < (MAX_NAME_LENGTH-strlen(ch.name)); i++)
      strcat(buf, " ");

    sprintf(buf2,"%s\r\n", clan_info[clan_n].ranks[clan_r]);
    strcat(buf,buf2);
  
   /* if ((strlen(buf) + strlen(clan_tables[clan_n])) < MAX_STRING_LENGTH) */
      strcat(clan_tables[clan_n],buf);
 
  /*  realloc(clan_tables[clan_n], strlen(buf) + strlen(clan_tables[clan_n]));*/
  } 
} 

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}



void index_boot(int mode)
{
  char *index_filename, *prefix;
  FILE *index, *db_file;
  int rec_count = 0;

  switch (mode) {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand to index_boot!");
    exit(1);
    break;
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s/%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    sprintf(buf1, "Error opening index file '%s'", buf2);
    perror(buf1);
    exit(1);
  }
  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      exit(1);
    } else {
      if (mode == DB_BOOT_ZON)
	rec_count++;
      else
	rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  if (!rec_count) {
    log("SYSERR: boot error - 0 records counted");
    exit(1);
  }
  rec_count++;

  switch (mode) {
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    break;
  }

  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
      discrete_load(db_file, mode);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }
}


void discrete_load(FILE * fl, int mode)
{
  int nr = -1, last = 0;
  char line[256];

  char *modes[] = {"world", "mob", "obj"};

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	fprintf(stderr, "Format error after %s #%d\n", modes[mode], nr);
	exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	fprintf(stderr, "Format error after %s #%d\n", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
	case DB_BOOT_WLD:
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr);
	  break;
	case DB_BOOT_OBJ:
	  strcpy(line, parse_object(fl, nr));
	  break;
	}
    } else {
      fprintf(stderr, "Format error in %s file near %s #%d\n",
	      modes[mode], modes[mode], nr);
      fprintf(stderr, "Offending line: '%s'\n", line);
      exit(1);
    }
  }
}



/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256];
  struct extra_descr_data *new_descr;

  sprintf(buf2, "room #%d", virtual_nr);

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    fprintf(stderr, "Room #%d is below zone %d.\n", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      fprintf(stderr, "Room %d is outside of any zone.\n", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (!get_line(fl, line) || sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error in room #%d\n", virtual_nr);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
  world[room_nr].room_flags = t[1];
  world[room_nr].sector_type = t[2];

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;	/* Zero light sources */

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  sprintf(buf, "Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      fprintf(stderr, "%s\n", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S':			/* end of room */
      top_of_world = room_nr++;
      return;
      break;
    default:
      fprintf(stderr, "%s\n", buf);
      exit(1);
      break;
    }
  }
}



/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int t[5];
  char line[256];

  sprintf(buf2, "room #%d, direction D%d", world[room].number, dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = t[1];
  world[room].dir_option[dir]->to_room = t[2];
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  extern sh_int mortal_start_room;
  extern sh_int eternal_start_room;
  extern sh_int immort_start_room;
  extern sh_int frozen_start_room;

  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_eternal_start_room = real_room(eternal_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Eternal start room does not exist.  Change in config.c.");
    r_eternal_start_room = r_mortal_start_room;
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  register int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
	if (world[room].dir_option[door]->to_room != NOWHERE)
	  world[room].dir_option[door]->to_room =
	    real_room(world[room].dir_option[door]->to_room);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int zone, cmd_no, a, b;

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = 0;
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	b = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  b = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	b = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      }
      if (a < 0 || b < 0) {
	if (!mini_mud)
	  log_zone_error(zone, cmd_no, "Invalid vnum, cmd disabled");
#if 0
	  fprintf(stderr, "Invalid vnum in zone reset cmd: zone #%d, cmd %d .. command disabled.\n",
		  zone_table[zone].number, cmd_no + 1);
#endif
	ZCMD.command = '*';
      }
    }
}



void parse_mobile(FILE * mob_f, int nr)
{
  static int i = 0;
  int j, t[10];
  char line[256], *tmpptr, letter;

  mob_index[i].virtual = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  mob_proto[i].player_specials = &dummy_mob;
  sprintf(buf2, "mob vnum %d", nr);

  /***** String data *** */
  mob_proto[i].player.name = fread_string(mob_f, buf2);
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.description = fread_string(mob_f, buf2);
  mob_proto[i].player.title = NULL;

  /* *** Numeric data *** */
  get_line(mob_f, line);
  sscanf(line, "%d %d %d %c", t, t + 1, t + 2, &letter);
  MOB_FLAGS(mob_proto + i) = t[0];
  SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  AFF_FLAGS(mob_proto + i) = t[1];
  GET_ALIGNMENT(mob_proto + i) = t[2];

  switch (letter) {
  case 'S':			/* Simple monsters */
    mob_proto[i].real_abils.str = 11;
    mob_proto[i].real_abils.intel = 11;
    mob_proto[i].real_abils.wis = 11;
    mob_proto[i].real_abils.dex = 11;
    mob_proto[i].real_abils.con = 11;

    get_line(mob_f, line);
    if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	  t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
      fprintf(stderr, "Format error in mob #%d, first line after S flag\n"
	      "...expecting line of form '# # # #d#+# #d#+#'\n", nr);
      exit(1);
    }
    GET_LEVEL(mob_proto + i) = t[0];
    mob_proto[i].points.hitroll = 20 - t[1];
    mob_proto[i].points.armor = 10 * t[2];

    /* max hit = 0 is a flag that H, M, V is xdy+z */
    mob_proto[i].points.max_hit = 0;
    mob_proto[i].points.hit = t[3];
    mob_proto[i].points.mana = t[4];
    mob_proto[i].points.move = t[5];

    mob_proto[i].points.max_mana = 10;
    mob_proto[i].points.max_move = 50;

    mob_proto[i].mob_specials.damnodice = t[6];
    mob_proto[i].mob_specials.damsizedice = t[7];
    mob_proto[i].points.damroll = t[8];

    get_line(mob_f, line);
    sscanf(line, " %d %d ", t, t + 1);
    GET_GOLD(mob_proto + i) = t[0];
    GET_EXP(mob_proto + i) = t[1];

    get_line(mob_f, line);
    if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) == 4)
      mob_proto[i].mob_specials.attack_type = t[3];
    else
      mob_proto[i].mob_specials.attack_type = 0;

    mob_proto[i].char_specials.position = t[0];
    mob_proto[i].mob_specials.default_pos = t[1];
    mob_proto[i].player.sex = t[2];

    mob_proto[i].player.class = 0;
    mob_proto[i].player.weight = 200;
    mob_proto[i].player.height = 198;

    for (j = 0; j < 3; j++)
      GET_COND(mob_proto + i, j) = -1;

    /*
     * these are now save applies; base save numbers for MOBs are now from
     * the warrior save table.
     */
    for (j = 0; j < 5; j++)
      GET_SAVE(mob_proto + i, j) = 0;

    break;
  default:
    fprintf(stderr, "Unsupported mob type %c in mob #%d\n", letter, nr);
    exit(1);
    break;
  }

  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  top_of_mobt = i++;
}




/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
  static int i = 0;
  static char line[256];
  int t[10], j;
  char *tmpptr;
  struct extra_descr_data *new_descr;
  int level_restrict = 0;

  obj_index[i].virtual = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].in_room = NOWHERE;
  obj_proto[i].item_number = i;

  sprintf(buf2, "object #%d", nr);

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    fprintf(stderr, "Null obj name or format error at or near %s\n", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (*tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    *tmpptr = UPPER(*tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line) || sscanf(line, " %d %d %d", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error in first numeric line, %s\n", buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.type_flag = t[0];
  obj_proto[i].obj_flags.extra_flags = t[1];
  obj_proto[i].obj_flags.wear_flags = t[2];

  if (!get_line(obj_f, line) || sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
    fprintf(stderr, "Format error in second numeric line, %s\n", buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.value[0] = t[0];
  obj_proto[i].obj_flags.value[1] = t[1];
  obj_proto[i].obj_flags.value[2] = t[2];
  obj_proto[i].obj_flags.value[3] = t[3];

  if (!get_line(obj_f, line) || sscanf(line, "%d %d %d", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error in third numeric line, %s\n", buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.weight = t[0];
  obj_proto[i].obj_flags.cost = t[1];
  obj_proto[i].obj_flags.cost_per_day = t[2];

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  strcat(buf2, ", after numeric constants (expecting E/A/#xxx)");
  j = 0;
  
  for (;;) {
    if (!get_line(obj_f, line)) {
      fprintf(stderr, "Format error in %s\n", buf2);
      exit(1);
    }
    switch (*line) {
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
    case 'A':
      if (j >= MAX_OBJ_AFFECT) {
	fprintf(stderr, "Too many A fields (%d max), %s\n", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      get_line(obj_f, line);
      sscanf(line, " %d %d ", t, t + 1);
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      j++;
      break;
    case 'L':
      if (level_restrict)
      {
        fprintf(stderr, "Multiple level restrict defines\n");
        exit(1);
      } 
      sscanf(line+1, " %d", &level_restrict);

      if ((level_restrict > 127) || (level_restrict < 0)) {
        fprintf(stderr, "Level restriction out of range (0-127) for object %d\n", nr);
        exit(1);
      }

      obj_proto[i].obj_flags.extra_flags |= level_restrict << 17;
      break;

    case '$':
    case '#':
      top_of_objt = i++;
      return line;
      break;
    default:
      printf("%s\n\r", line);
      fprintf(stderr, "Format error in %s\n", buf2);
      exit(1);
      break;
    }
  }
}


#define Z	zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static int zone = 0;
  int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error;
  char *ptr, buf[256], zname[256];

  strcpy(zname, zonename);

  while (get_line(fl, buf))
    num_of_cmds++;		/* this should be correct within 3 or so */
  rewind(fl);

  if (num_of_cmds == 0) {
    fprintf(stderr, "%s is empty!\n", zname);
    exit(0);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  /* Add the world number in here - DM */
  if (sscanf(buf, "#%d %d", &Z.number, &Z.world) != 2) {
    fprintf(stderr, "Format error in %s, line %d\n", zname, line_num);
    exit(0);
  }
  if ((Z.world < 1) || (Z.world > TOP_WORLDS)) {
    fprintf(stderr, "Format error in %s, line %d, (bad zone number)\n", zname, line_num);
    exit(0);
  }
  sprintf(buf2, "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = str_dup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %d %d %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode, &Z.zflag) != 4) {
    fprintf(stderr, "Format error in 4-constant line of %s", zname);
    exit(0);
  }
  cmd_no = 0;

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0) {
      fprintf(stderr, "Format error in %s - premature end of file\n", zname);
      exit(0);
    }
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPD", ZCMD.command) == NULL) {	/* a 3-arg command */
      if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
	error = 1;
    } else {
      if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
		 &ZCMD.arg3) != 4)
	error = 1;
    }

    ZCMD.if_flag = tmp;

    if (error) {
      fprintf(stderr, "Format error in %s, line %d: '%s'\n", zname, line_num, buf);
      exit(0);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  top_of_zone_table = zone++;
}

#undef Z


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*********************************************************************** */



int vnum_mobile(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_mobt; nr++) {
    if (isname(searchname, mob_proto[nr].player.name)) {
      sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
	      mob_index[nr].virtual,
	      mob_proto[nr].player.short_descr);
      send_to_char(buf, ch);
    }
  }

  return (found);
}



int vnum_object(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++) {
    if (isname(searchname, obj_proto[nr].name)) {
      sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
	      obj_index[nr].virtual,
	      obj_proto[nr].short_description);
      send_to_char(buf, ch);
    }
  }
  return (found);
}


/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;

  return ch;
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(int nr, int type)
{
  int i;
  struct char_data *mob;

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) < 0) {
      sprintf(buf, "Mobile (V) %d does not exist in database.", nr);
      return (0);
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
      mob->points.move;
  } else
    mob->points.max_hit = number(mob->points.hit, mob->points.mana);

  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);

  mob_index[i].number++;

  return mob;
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  return obj;
}


/* create a new object from a prototype */
struct obj_data *read_object(int nr, int type)
{
  struct obj_data *obj;
  int i;

  if (nr < 0) {
    log("SYSERR: trying to create obj with negative num!");
    return NULL;
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      sprintf(buf, "Object (V) %d does not exist in database.", nr);
      return NULL;
    }
  } else
    i = nr;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;

  return obj;
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;
  char buf[128];

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode)
	(zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

	CREATE(update_u, struct reset_q_element, 1);

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }				/* end - one minute has passed */
  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
	is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset);
      sprintf(buf, "Auto zone reset: %s",
	      zone_table[update_u->zone_to_reset].name);
      mudlog(buf, CMP, LVL_GOD, FALSE);
      /* dequeue */
      if (update_u == reset_q.head)
	reset_q.head = reset_q.head->next;
      else {
	for (temp = reset_q.head; temp->next != update_u;
	     temp = temp->next);

	if (!update_u->next)
	  reset_q.tail = temp;

	temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

void log_zone_error(int zone, int cmd_no, char *message)
{
  char buf[256];

  sprintf(buf, "SYSERR: error in zone file: %s", message);
  mudlog(buf, NRM, LVL_GOD, TRUE);

  sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	  ZCMD.command, zone_table[zone].number, ZCMD.line);
  mudlog(buf, NRM, LVL_GOD, TRUE);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(int zone)
{
  int cmd_no, last_cmd = 0;
  struct char_data *mob;
  struct obj_data *obj, *obj_to;

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

    if (ZCMD.if_flag && !last_cmd)
      continue;

    switch (ZCMD.command) {
    case '*':			/* ignore command */
      last_cmd = 0;
      break;

    case 'M':			/* read a mobile */
      if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
	mob = read_mobile(ZCMD.arg1, REAL);
	char_to_room(mob, ZCMD.arg3);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'O':			/* read an object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
	if (ZCMD.arg3 >= 0) {
	  if (!get_obj_in_list_num(ZCMD.arg1, world[ZCMD.arg3].contents,REAL)) {
	    obj = read_object(ZCMD.arg1, REAL);
	    obj_to_room(obj, ZCMD.arg3);
	    last_cmd = 1;
	  } else
	    last_cmd = 0;
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  obj->in_room = NOWHERE;
	  last_cmd = 1;
	}
      else
	last_cmd = 0;
      break;

    case 'P':			/* object to object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
	  ZONE_ERROR("target obj not found");
	  break;
	}
	obj_to_obj(obj, obj_to);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'G':			/* obj_to_char */
      if (!mob) {
	ZONE_ERROR("attempt to give obj to non-existant mob");
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	obj_to_char(obj, mob);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'E':			/* object to equipment list */
      if (!mob) {
	ZONE_ERROR("trying to equip non-existant mob");
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);

	/* JA */
	  equip_char(mob, obj, ZCMD.arg3);
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      break;

    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
	ZONE_ERROR("door does not exist");
      } else
	switch (ZCMD.arg3) {
	case 0:
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_CLOSED);
	  break;
	case 1:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  break;
	case 2:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_LOCKED);
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  break;
	}
      last_cmd = 1;
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table!");
      break;
    }
  }

  zone_table[zone].age = 0;
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr)
	return (0);

  return (1);
}





/*************************************************************************
*  stuff related to the save/load player system				 *
*********************************************************************** */


long get_id_by_name(char *name)
{
  int i;

  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
    if (!strcmp((player_table + i)->name, arg))
      return ((player_table + i)->id);

  return -1;
}


char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if ((player_table + i)->id == id)
      return ((player_table + i)->name);

  return NULL;
}

void load_aliases(struct char_data *ch)
{
  FILE *afile;
  struct alias *al;
  char alias_filename[256];
  char lreplace[256];
  char lalias[256];
  int counter;

  strcpy(alias_filename,ALIAS_DIRNAME);
  strcat(alias_filename,GET_NAME(ch));
  if ((afile = fopen(alias_filename,"r")) == NULL) return;
  while (fscanf(afile,"%s",lalias) != EOF)
  {
    if (!fgets(lreplace,256,afile)) return;
    counter=0;
    while(lreplace[++counter]!='\0')
      if(lreplace[counter]=='\n')
        lreplace[counter]='\0';
    CREATE(al,struct alias,1);
    al->alias = str_dup(lalias);
    al->replacement = str_dup(lreplace);
    if (strchr(lreplace,ALIAS_SEP_CHAR) || strchr(lreplace,ALIAS_VAR_CHAR))
      al->type = ALIAS_COMPLEX;
    else
      al->type = ALIAS_SIMPLE;
    al->next = GET_ALIASES(ch);
    GET_ALIASES(ch) = al;
  }
  fclose(afile);
}

/* Load a char, TRUE if loaded, FALSE if not */
int load_char(char *name, struct char_file_u * char_element)
{
  int player_i;

  int find_name(char *name);

  if ((player_i = find_name(name)) >= 0) {
    fseek(player_fl, (long) (player_i * sizeof(struct char_file_u)), SEEK_SET);
    fread(char_element, sizeof(struct char_file_u), 1, player_fl);
    return (player_i);
  } else
    return (-1);
}


void save_aliases(struct char_data *ch)
{
  struct alias *al;
  FILE *afile;
  char alias_filename[256];

  al = GET_ALIASES(ch);
  if (al != NULL)
  {
    strcpy(alias_filename,ALIAS_DIRNAME);
    strcat(alias_filename,GET_NAME(ch));
    if ((afile = fopen(alias_filename,"w")) == NULL)
      send_to_char("Could not open file.",ch);
    fprintf(afile,"%s%s\n",al->alias,al->replacement);
    while (al->next != NULL)
    {
      al = al->next;
      fprintf(afile,"%s%s\n",al->alias,al->replacement);
    }
    fclose(afile);
  }
}


/* write the vital data of a player to the player file */
void save_char(struct char_data * ch, sh_int load_room)
{
  struct char_file_u st;

  if (IS_NPC(ch) || !ch->desc)
    return;

  char_to_store(ch, &st);

  strncpy(st.host, ch->desc->host, HOST_LENGTH);
  st.host[HOST_LENGTH] = '\0';

/*  if (!PLR_FLAGGED(ch, PLR_LOADROOM))
    st.player_specials_saved.load_room = load_room;
*/
  strcpy(st.pwd, ch->desc->pwd);

  fseek(player_fl, ch->desc->pos * sizeof(struct char_file_u), SEEK_SET);
  fwrite(&st, sizeof(struct char_file_u), 1, player_fl);

  save_aliases(ch);
}



/* copy data from the file structure to a char struct */
void store_to_char(struct char_file_u * st, struct char_data * ch)
{
  int i;

  /* to save memory, only PC's -- not MOB's -- have player_specials */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  GET_SEX(ch) = st->sex;
  GET_CLASS(ch) = st->class;
  GET_LEVEL(ch) = st->level;

  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.title = str_dup(st->title);
  ch->player.description = str_dup(st->description);

  ch->player.hometown = st->hometown;
  ch->player.time.birth = st->birth;
  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  ch->player.weight = st->weight;
  ch->player.height = st->height;

  ch->real_abils = st->abilities;
  ch->aff_abils = st->abilities;
  ch->points = st->points;
  ch->char_specials.saved = st->char_specials_saved;
  ch->player_specials->saved = st->player_specials_saved;
  POOFIN(ch) = NULL;
  POOFOUT(ch) = NULL;

  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;

  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->points.armor = 200;
  ch->points.hitroll = 0;
  ch->points.damroll = 0;

  CREATE(ch->player.name, char, strlen(st->name) + 1);
  strcpy(ch->player.name, st->name);

  /* Add all spell effects */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, &st->affected[i]);
  }

  ch->in_room = GET_LOADROOM(ch);

/*   affect_total(ch); also - unnecessary?? */

  /*
   * If you're not poisioned and you've been away for more than an hour,
   * we'll set your HMV back to full
   */

  if (!IS_AFFECTED(ch, AFF_POISON) &&
      (((long) (time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR)) {
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
  }
  load_aliases(ch);  /* Here is the best place believe it or not! */
}				/* store_to_char */




/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data * ch, struct char_file_u * st)
{
  int i;
  struct affected_type *af;
  struct obj_data *char_eq[NUM_WEARS];

  /* Unaffect everything a character can be affected by */

  for (i = 0; i < NUM_WEARS; i++) {
    if (ch->equipment[i])
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = NULL;
  }

  for (af = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (af) {
      st->affected[i] = *af;
      st->affected[i].next = 0;
      af = af->next;
    } else {
      st->affected[i].type = 0;	/* Zero signifies not used */
      st->affected[i].duration = 0;
      st->affected[i].modifier = 0;
      st->affected[i].location = 0;
      st->affected[i].bitvector = 0;
      st->affected[i].next = 0;
    }
  }


  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  if ((i >= MAX_AFFECT) && af && af->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  st->birth = ch->player.time.birth;
  st->played = ch->player.time.played;
  st->played += (long) (time(0) - ch->player.time.logon);
  st->last_logon = time(0);

  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  st->hometown = ch->player.hometown;
  st->weight = GET_WEIGHT(ch);
  st->height = GET_HEIGHT(ch);
  st->sex = GET_SEX(ch);
  st->class = GET_CLASS(ch);
  st->level = GET_LEVEL(ch);
  st->abilities = ch->real_abils;
  st->points = ch->points;
  st->char_specials_saved = ch->char_specials.saved;
  st->player_specials_saved = ch->player_specials->saved;

  st->points.armor = 100;
  st->points.hitroll = 0;
  st->points.damroll = 0;

  if (GET_TITLE(ch))
    strcpy(st->title, GET_TITLE(ch));
  else
    *st->title = '\0';

  if (ch->player.description)
    strcpy(st->description, ch->player.description);
  else
    *st->description = '\0';

  strcpy(st->name, GET_NAME(ch));

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, &st->affected[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);
  }
/*   affect_total(ch); unnecessary, I think !?! */
}				/* Char to store */



void save_etext(struct char_data * ch)
{


}


/* create a new entry in the in-memory index table for the player file */
int create_entry(char *name)
{
  int i;

  if (top_of_p_table == -1) {
    CREATE(player_table, struct player_index_element, 1);
    top_of_p_table = 0;
  } else if (!(player_table = (struct player_index_element *)
	       realloc(player_table, sizeof(struct player_index_element) *
		       (++top_of_p_table + 1)))) {
    perror("create entry");
    exit(1);
  }
  CREATE(player_table[top_of_p_table].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (*(player_table[top_of_p_table].name + i) = LOWER(*(name + i)));
       i++);

  return (top_of_p_table);
}



/************************************************************************
*  procs of a (more or less) general utility nature			*
********************************************************************** */


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n",
	      error);
      exit(1);
    }
    /* If there is a '~', end the string; else an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return rslt;
}


/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
{
  int i;
  struct alias *a;

  void free_alias(struct alias * a);

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    free(ch->player_specials);
    if (IS_NPC(ch))
      log("SYSERR: Mob had player_specials allocated!");
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (ch->player.title)
      free(ch->player.title);
    if (ch->player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description)
      free(ch->player.description);
  } else if ((i = GET_MOB_RNUM(ch)) > -1) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
      free(ch->player.name);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
  }
  while (ch->affected)
    affect_remove(ch, ch->affected);

  while ((a = GET_ALIASES(ch)) != NULL) {
    GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
    free_alias(a);
  }

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  int nr;
  struct extra_descr_data *this, *next_one;

  if ((nr = GET_OBJ_RNUM(obj)) == -1) {
    if (obj->name)
      free(obj->name);
    if (obj->description)
      free(obj->description);
    if (obj->short_description)
      free(obj->short_description);
    if (obj->action_description)
      free(obj->action_description);
    if (obj->ex_description)
      for (this = obj->ex_description; this; this = next_one) {
	next_one = this->next;
	if (this->keyword)
	  free(this->keyword);
	if (this->description)
	  free(this->description);
	free(this);
      }
  } else {
    if (obj->name && obj->name != obj_proto[nr].name)
      free(obj->name);
    if (obj->description && obj->description != obj_proto[nr].description)
      free(obj->description);
    if (obj->short_description && obj->short_description != obj_proto[nr].short_description)
      free(obj->short_description);
    if (obj->action_description && obj->action_description != obj_proto[nr].action_description)
      free(obj->action_description);
    if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
      for (this = obj->ex_description; this; this = next_one) {
	next_one = this->next;
	if (this->keyword)
	  free(this->keyword);
	if (this->description)
	  free(this->description);
	free(this);
      }
  }

  free(obj);
}



/* read contets of a text file, alloc space, point buf to it */
int file_to_string_alloc(char *name, char **buf)
{
  char temp[MAX_STRING_LENGTH];
 
  if (file_to_string(name, temp) < 0)
    return -1;

  if (*buf)
    free(*buf);

  *buf = str_dup(temp);

  return 0;
}



/* read contents of a text file, and place in buf */
int file_to_string(char *name, char *buf)
{
  FILE *fl;
  char tmp[128];

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    sprintf(tmp, "Error reading %s", name);
    perror(tmp);
    return (-1);
  }
  do {
    fgets(tmp, 128, fl);
    tmp[strlen(tmp) - 1] = '\0';/* take off the trailing \n */
    strcat(tmp, "\r\n");

    if (!feof(fl)) {
      if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
	log("SYSERR: fl->strng: string too big (db.c, file_to_string)");
	*buf = '\0';
	return (-1);
      }
      strcat(buf, tmp);
    }
  } while (!feof(fl));

  fclose(fl);

  return (0);
}




/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    ch->equipment[i] = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  /* ch->in_room = NOWHERE; Used for start in room */
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  ch->autoassisters = NULL;
  FIGHTING(ch) = NULL;
  AUTOASSIST(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  ch->in_room = NOWHERE;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;

  GET_AC(ch) = 100;		/* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
}


void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
}




/* initialize a new character only if class is set */
void init_char(struct char_data * ch)
{
  int i;
 /* char s[200]; */
  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */

  if (top_of_p_table == 0) {
    /* DM_exp - set exp to 0 for new exp system */
    GET_EXP(ch) = 0;
    GET_LEVEL(ch) = LVL_IMPL;

    ch->points.max_hit = 500;
    ch->points.max_mana = 100;
    ch->points.max_move = 82;
  }
  set_title(ch, "");


  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.description = NULL;

  ch->player.hometown = 1;

  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  for (i = 0; i < MAX_TONGUE; i++)
    GET_TALK(ch, i) = 0;

  /* make favors for sex */
  if (ch->player.sex == SEX_MALE) {
    ch->player.weight = number(120, 180);
    ch->player.height = number(160, 200);
  } else {
    ch->player.weight = number(100, 160);
    ch->player.height = number(150, 180);
  }

  ch->points.max_mana = 100;
  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.hit = GET_MAX_HIT(ch);
  ch->points.max_move = 82;
  ch->points.move = GET_MAX_MOVE(ch);
  ch->points.armor = 100;

  player_table[top_of_p_table].id = GET_IDNUM(ch) = ++top_idnum;
if (top_idnum>20000) 
 {
/* sprintf(s,"DEBUG IDNUM 2: top_idnum=%ld CHARS NUM=%ld\n",top_idnum,
GET_IDNUM(ch));
log(s); */
}

  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0)
    else
      SET_SKILL(ch, i, 100);
  }

  ch->char_specials.saved.affected_by = 0;

  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);
}



/* returns the real number of the room with given virtual number */
int real_room(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) >> 1;

    if ((world + mid)->number == virtual)
      return mid;
    if (bot >= top)
      return -1;
    if ((world + mid)->number > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the monster with given virtual number */
int real_mobile(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the object with given virtual number */
int real_object(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

void clan_boot ( void )
{
  
  FILE *cfp ;
  char buf [MAX_LEN];
  int i = 1 , j = 0; 

  strcpy ( clan_info[0].name , "none" ) ; 


  if (!(cfp = fopen( CLAN_FILE, "r"))) {
    sprintf( buf, "Error reading %s", CLAN_FILE);
    perror( buf );
    return ;

  }

  while ( buf[0] != FILE_TERM )
  {
    
    fgets ( buf , MAX_LEN , cfp );
    strcpy ( clan_info[i].name , buf   );
    clan_info[i].name[strlen(clan_info[i].name) - 1] = '\0';

    fgets ( buf , MAX_LEN , cfp );
    strcpy ( clan_info[i].disp_name , buf   );
    clan_info[i].disp_name[strlen(clan_info[i].disp_name) - 1] = '\0';

    sprintf ( buf , "Loading clan: %s." , clan_info[i].disp_name );
    log ( buf ) ; 

    for ( j = 0 ; j < RANK_NUM ; j ++ )
    {
      fgets ( buf , MAX_LEN , cfp );
      strcpy ( clan_info[i].ranks[j] , buf  );
      clan_info[i].ranks[j][strlen(clan_info[i].ranks[j]) - 1] = '\0';
    }

    fgets ( buf , MAX_LEN , cfp ) ;
    i++ ;
  }

  /* Set clan_tables array to NULL */ 
  for (i=1; i < CLAN_NUM; i++)
    clan_tables[i][0]='\0';  

  fclose ( cfp ) ;
}

void copy_clan_tables(void)
{
  int i;

  for (i=1; i < CLAN_NUM; i++)
    clan_info[i].table = str_dup(clan_tables[i]);

}

char *get_clan_rank ( struct char_data *ch ) 
{
  return clan_info[GET_CLAN_NUM (ch)].ranks[GET_CLAN_LEV(ch)] ;
}

char *get_clan_disp ( struct char_data *ch ) 
{
  return clan_info[GET_CLAN_NUM (ch)].disp_name ;
}

char *get_clan_name ( int cnum ) 
{
  return clan_info[cnum].name ;
}

int check_clan ( struct char_data *ch , struct char_data *vict , int sub_allow  ) 
{

  char buf [100];
	
	
	if ( sub_allow )
	{
	 if (!EXT_FLAGGED(ch, EXT_LEADER) && !EXT_FLAGGED(ch , EXT_SUBLEADER))
	 {
          send_to_char ( "You are not a clan leader or a knight.\n\r" , ch );
          return 0 ;
	 }
	}
	else if (!EXT_FLAGGED(ch, EXT_LEADER))
        {
         send_to_char ( "You are not a clan leader.\n\r" , ch );
         return 0 ;
        }

	if (!(EXT_FLAGGED(vict, EXT_CLAN)))
        {
         sprintf ( buf , "%s is not even in a clan.\n\r" , GET_NAME (vict)) ;
         send_to_char ( buf , ch );
         return 0 ;
        }

	if ( GET_CLAN_NUM(vict) != GET_CLAN_NUM(ch) )
        {
         sprintf ( buf , "%s is not in your clan.\n\r" , GET_NAME (vict) );
         send_to_char ( buf , ch );
         return 0 ;
	}

       return 1 ;

}

#define CBACK(ch,lvl)	CCRED(ch,lvl)	/* Background		*/
#define CBLADE(ch,lvl)	CCYEL(ch,lvl)	/* Blade		*/
#define CBLADEI(ch,lvl)	CCYEL(ch,lvl)	/* Blade inside		*/
#define CHANDO(ch,lvl)  CCBLU(ch,lvl)	/* Handle color outer	*/
#define CHANDB(ch,lvl) 	CCBBLU(ch,lvl)	/* Handle color inside	*/
#define CHANDG(ch,lvl)	CCBBLU(ch,lvl)	/* Handle color grip	*/
#define CSTAR(ch,lvl)	CCYEL(ch,lvl)	/* Top Stars		*/

#define CPPCOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of P (primal)	*/
#define CPRCOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of R (primal)	*/
#define CPICOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of I (primal)	*/
#define CPMCOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of M (primal)	*/
#define CPACOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of A (primal)	*/
#define CPLCOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of L (primal)	*/
#define CMMCOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of M (mud)	*/
#define CMUCOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of U (mud)	*/
#define CMDCOL(ch,lvl)	CCBRED(ch,lvl)	/* Color of D (mud)	*/

