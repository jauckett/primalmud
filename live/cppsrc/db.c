/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include <fstream>
#include <iostream>
#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "clan.h"
#include "constants.h"
#include "colour.h"
#include "casino.h"
#include "corpses.h"
#include "genzon.h"
#include "reports.h"
#include "dg_scripts.h"
#include "oasis.h"
#include "balance.h"
#include "shop.h"

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

CorpseData corpseData = CorpseData();
ReleaseInfo release = ReleaseInfo();
ReportList *reportList = new ReportList();
GameInfo gameInfo = GameInfo();

struct room_data *world = NULL;	/* array of rooms		 */
room_rnum top_of_world = 0;	/* ref to top element of world	 */

struct char_data *character_list = NULL;  /* global linked list of chars */ 
struct hunt_data *hunt_list = NULL;  /* Artus> Hunting/Hunted List */
struct index_data **trig_index; /* index table for triggers      */
int top_of_trigt = 0;           /* top of trigger index table    */
long max_id = MOBOBJ_ID_BASE;   /* for unique mob/obj id's       */

struct index_data *mob_index;	/* index table for mobile file	 */
struct char_data *mob_proto;	/* prototypes for mobs		 */
mob_rnum top_of_mobt = 0;	/* top of mobile index table	 */

struct obj_data *object_list = NULL;	/* global linked list of objs	 */
struct index_data *obj_index;	/* index table for object file	 */
struct obj_data *obj_proto;	/* prototypes for objs		 */
obj_rnum top_of_objt = 0;	/* top of object index table	 */

struct zone_data *zone_table;	/* zone table			 */
zone_rnum top_of_zone_table = 0;/* top element of zone tab	 */

struct auc_data *auc_list = NULL;

struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */

struct player_index_element *player_table = NULL;	/* index to plr file	 */
struct imm_list_element *immlist_table = NULL;		/* immlist data */
struct questlist_element *questlist_table = NULL;	/* Questlist Table */

FILE *player_fl = NULL;		/* file desc of player file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;		/* ref of size of p file	 */
long top_idnum = 0;		/* highest idnum in use		 */
int num_rooms_burgled = 0;	/* number of burgled rooms, duh  */

long NUM_PLAYERS = 0;		/* Number of players in DB	 */
int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
room_rnum r_mortal_start_room;	/* rnum of mortal start room	 */
room_rnum r_immort_start_room;	/* rnum of immort start room	 */
room_rnum r_frozen_start_room;	/* rnum of frozen start room	 */

char *credits = NULL;		/* game credits			 */
char *news = NULL;		/* mud news			 */
char *motd = NULL;		/* message of the day - mortals */
char *imotd = NULL;		/* message of the day - immorts */
char *GREETINGS = NULL;		/* opening credits screen	*/
char *help = NULL;		/* help screen			 */
char *info = NULL;		/* info page			 */
char *wizlist = NULL;		/* list of higher gods		 */
char *immlist = NULL;		/* list of peon gods		 */
char *background = NULL;	/* background story		 */
char *handbook = NULL;		/* handbook for new immortals	 */
char *policies = NULL;		/* policies page		 */
char *areas = NULL;		/* areas			 */
char *hint_table[MAX_HINTS];    /* hint table                    */

int num_hints = 0;

struct help_index_element *help_table = 0;	/* the help table	 */
int top_of_helpt = 0;		/* top of help index table	 */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	*/
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

/* Clan info struct 
struct clan_data clan_info[NUM_CLANS];
char clan_tables[NUM_CLANS][MAX_STRING_LENGTH];
-- ARTUS: Old clan stuff. */

/* Casino Stuff */
struct game_data casino_race;
struct blackjack_data casino_blackjack;

/* Event list */
struct event_list events;

/* Burglaries */
Burglary *burglaries = NULL;

/* local functions */
int check_object_spell_number(struct obj_data *obj, int val);
int check_object_level(struct obj_data *obj, int val);
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode, char *filename, char *zonefilename,
                        zone_rnum rznum);
int check_object(struct obj_data *);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr, zone_vnum vznum, zone_rnum rznum);
char *parse_object(FILE * obj_f, int nr, zone_vnum vznum, zone_rnum rznum);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE *fl, int wizflag);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
int _is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE * fl);
bitvector_t asciiflag_conv(char *flag);
void parse_simple_mob(FILE *mob_f, int i, int nr);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE *mob_f, int i, int nr);
void get_one_line(FILE *fl, char *buf);
void save_etext(struct char_data * ch);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
long get_ptable_by_name(char *name);
int load_primal_spell_levels(void);
void load_hints(void);
void load_zone_hints(FILE *, zone_rnum);
void add_spell_help(FILE *spell_help_file); 
int check_spell_values(int spell_type, int line, sh_int intl, sh_int wis, 
        sh_int dex, sh_int str, sh_int cha, sh_int con, int min_level, 
        int mana_min, int mana_max, int mana_change, int spell_effec, 
        int mana_perc);

// void clan_boot(void);   
// void copy_clan_tables(void);
// void add_clan_table(struct char_file_u ch);
void loadRevision(void);

/* external functions */
void apply_specials(struct char_data *ch, bool initial); 
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias_data *a);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);	/* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void init_auctions(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count, 
                        zone_vnum vznum, zone_rnum rznum);
int find_name(char *name);
int hsort(const void *a, const void *b);
void prune_crlf(char *txt);
void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
void to_upper(char *s);
void save_char_vars(struct char_data *ch);

/* external vars */
extern const char *level_bits[];
extern int no_specials;
extern int scheck;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type spell_info[];
extern const char *unused_spellname;
extern struct shop_data *shop_index;
extern int top_shop;

#define READ_SIZE 256

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

void generate_zone_data()
{
  // reset data 
  for (zone_rnum i = 0; i < top_of_zone_table; i++) {
    zone_table[i].nowlds = 0;
    zone_table[i].nomobs = 0;
    zone_table[i].noobjs = 0;
    zone_table[i].noshps = 0;
    zone_table[i].notrgs = 0;
//    zone_table[i].nohnts = 0;
  }

  // count up rooms
  for (room_rnum i = 0; i < top_of_world; i++) {
    zone_table[world[i].zone].nowlds++;
  }  

  // count up objects 
  for (obj_rnum i = 0; i < top_of_objt; i++) {
    zone_table[obj_index[i].rznum].noobjs++;
  }

  // count up mobs 
  for (mob_rnum i = 0; i < top_of_mobt; i++) {
    zone_table[mob_index[i].rznum].nomobs++;
  }
  
  // triggers
  for (int i = 0; i < top_of_trigt; i++) {
    zone_table[trig_index[i]->rznum].notrgs++;
  }
  
  // shops
  for (shop_rnum i = 0; i < top_shop; i++) {
    zone_table[shop_index[i].rznum].noshps++;
  }
  
  // DEBUG log info
  for (zone_rnum i = 0; i < top_of_zone_table; i++) {
    basic_mud_log("Zone rnum: %d, vnum: %d", i, zone_table[i].number);
    basic_mud_log("Number wlds: %d", zone_table[i].nowlds); 
    basic_mud_log("Number trgs: %d", zone_table[i].notrgs); 
    basic_mud_log("Number objs: %d", zone_table[i].noobjs); 
    basic_mud_log("Number mobs: %d", zone_table[i].nomobs); 
    basic_mud_log("Number shps: %d", zone_table[i].noshps); 
    basic_mud_log("Number hnts: %d", zone_table[i].nohnts);
    for (int j = 0; j < zone_table[i].nohnts; j++) {
      basic_mud_log("  Hint %d: %s", j, zone_table[i].hints[j]);
    }
  }
}

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
//  file_to_string_alloc(IMMLIST_FILE, &immlist);
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
  int i;

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
//    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
    file_to_string_alloc(AREAS_FILE, &areas);

    if (!load_primal_spell_levels())
      send_to_char("SYSERR: Error loading primal spells and skills.\r\n"
                     "For more details see the syslog\r\n",ch);

    if (help_table) {
      for (i = 0; i <= top_of_helpt; i++) {
        if (help_table[i].keyword)
          free(help_table[i].keyword);
        if (help_table[i].entry && !help_table[i].duplicate)
          free(help_table[i].entry);
      }
      free(help_table);
    }
    top_of_helpt = 0;
    index_boot(DB_BOOT_HLP);

  } else if (!str_cmp(arg, "wizlist"))
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
//  else if (!str_cmp(arg, "immlist"))
//    file_to_string_alloc(IMMLIST_FILE, &immlist);
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
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "background"))
    file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "areas"))
    file_to_string_alloc(AREAS_FILE, &areas);
  else if (!str_cmp(arg, "greetings")) {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
  } else if (!str_cmp(arg, "xhelp")) {
    if (help_table) {
      for (i = 0; i <= top_of_helpt; i++) {
        if (help_table[i].keyword)
	  free(help_table[i].keyword);
        if (help_table[i].entry && !help_table[i].duplicate)
	  free(help_table[i].entry);
      }
      free(help_table);
    }
    top_of_helpt = 0;
    index_boot(DB_BOOT_HLP);
  } else if (!str_cmp(arg,"spells")) {
    if (!load_primal_spell_levels()) 
      send_to_char("SYSERR: Error loading primal spells and skills.\r\n"
                     "For more details see the syslog\r\n",ch);
    return;
  } else {
    send_to_char("Unknown reload option.\r\n", ch);
    return;
  }

  send_to_char(OK, ch);
}


void boot_world(void)
{
  basic_mud_log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  basic_mud_log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  basic_mud_log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  basic_mud_log("Renumbering rooms.");
  renum_world();

  basic_mud_log("Checking start rooms.");
  check_start_rooms();

  basic_mud_log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  basic_mud_log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  basic_mud_log("Renumbering zone table.");
  renum_zone_table();

  if (!no_specials) {
    basic_mud_log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }

  basic_mud_log("Loading zone hint files.");
  index_boot(DB_BOOT_HNT);

  // DM - originally was going to process each index, now calculated at
  // load time, shall use generate_zone_data as some sort of reload option. 
  basic_mud_log("Generating zone data. DETAILS:");
  generate_zone_data();
}

  
void loadRevision() {
  ifstream revFile;
  char temp[READ_SIZE], tag[READ_SIZE], date[READ_SIZE];
  int major, minor, branch;
  bool cvsUpToDate;

  revFile.open(REV_FILE, std::ios::in);

  if (!revFile) {
    basic_mud_log("SYSERR: couldn't open revision file: %s", REV_FILE);
    return;
  }

  while (!revFile.eof()) {
    revFile >> temp >> major;
    release.setMajor(major);
    revFile.ignore(1);

    revFile >> temp >> branch;
    release.setBranch(branch);
    revFile.ignore(1);

    revFile >> temp >> minor;
    release.setMinor(minor);
    revFile.ignore(1);

    revFile >> temp >> tag;
    release.setTag(tag);
    revFile.ignore(1);

    revFile >> temp >> cvsUpToDate;
    release.setCvsUpToDate(cvsUpToDate);

    revFile >> temp;
    revFile.getline(date, 30);
    release.setDate(date);
    break;
  }

  basic_mud_log("Release: %d.%d.%d, tag: %s, cvs-up-to-date: %d, date: %s", 
      release.getMajor(), release.getBranch(), release.getMinor(), release.getTag(), release.isCvsUpToDate(), release.getDate());

  revFile.close();
}

/* body of the booting system */
void boot_db(void)
{
  zone_rnum i;

  basic_mud_log("Boot db -- BEGIN.");

  loadRevision();

  basic_mud_log("Resetting the game time:");
  reset_time();

  basic_mud_log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
//  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  file_to_string_alloc(AREAS_FILE, &areas);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
    prune_crlf(GREETINGS);


  basic_mud_log("Loading spell definitions.");
  mag_assign_spells();

  basic_mud_log("Assigning spell and skill levels.");
  if (!load_primal_spell_levels())  // Loads the spells definitions from file,
    exit(1);	
  // DM - Ok, now we just recreate spells.hlp when loading - so the next lin

  basic_mud_log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

//  basic_mud_log("Sorting help table.");
//  qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
//  top_of_helpt--;

  basic_mud_log("Sorting command list and spells.");
  sort_commands();
  sort_spells(); 	

  boot_world();

  // DM - world balancing ... check mob and obj values 
  balance_world(top_of_mobt, top_of_objt);

  if (!mini_mud) {
    basic_mud_log("Initialising gambling structures.");
    init_games();
    basic_mud_log("Loading hints file.");
    load_hints();
  } 

  basic_mud_log("Generating player index.");
  build_player_index();

/*  basic_mud_log("Copying clan tables.");
  copy_clan_tables();
  -- ARTUS: Old clan stuff.. 
*/
  basic_mud_log("Loading saved corpse data.");
  basic_mud_log("  loaded %d corpses.", corpseData.load());

  basic_mud_log("Loading report file.");
  reportList->loadFile();
  
  basic_mud_log("Loading fight messages.");
  load_messages();

  basic_mud_log("Loading social messages.");
  boot_social_messages();

  basic_mud_log("Assigning function pointers:");

  if (!no_specials) {
    basic_mud_log("   Mobiles.");
    assign_mobiles();
    basic_mud_log("   Shopkeepers.");
    assign_the_shopkeepers();
    basic_mud_log("   Objects.");
    assign_objects();
    basic_mud_log("   Rooms.");
    assign_rooms();
  }

  basic_mud_log("Booting mail system.");
  if (!scan_file()) {
    basic_mud_log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  basic_mud_log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    basic_mud_log("Deleting timed-out crash and rent files:");
    update_obj_file();
    basic_mud_log("   Done.");
  }

  for (i = 0; i <= top_of_zone_table; i++) {
    basic_mud_log("Resetting %s (rooms %d-%d).", zone_table[i].name,
	(i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
    reset_zone(i);
  }

  /* Moved here so the object limit code works. -gg 6/24/98 
   * Artus> Moved back so limit code isn't affected by house objects. */
  if (!mini_mud) {
    basic_mud_log("Booting houses.");
    House_boot();
  }

  basic_mud_log("Reading auctions.");
  init_auctions();


  reset_q.head = reset_q.tail = NULL;
  init_clans(); /* Artus */
  
  boot_time = time(0);

/*  basic_mud_log("Copying clan tables.");
  copy_clan_tables(); 
  -- ARTUS: Old clan stuff..
*/

  basic_mud_log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
#if defined(CIRCLE_MACINTOSH)
  long beginning_of_time = -1561789232;
#else
  long beginning_of_time = 650336715;
#endif

  time_info = *mud_time_passed(time(0), beginning_of_time);

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

  basic_mud_log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);

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
}



/* generate index table for the player file */
void build_player_index(void)
{
  int nr = -1, i;
  long size, recs;
  struct char_file_u dummy;
//  struct quest_obj_data *qobj;
  void add_to_immlist(char *name, long idnum, long immkills, ubyte unholiness);
  void add_to_questlist(char *name, long idnum, struct quest_obj_data *item);

  if (!(player_fl = fopen(PLAYER_FILE, "r+b")))
  {
    if (errno != ENOENT)
    {
      perror("SYSERR: fatal error opening playerfile");
      exit(1);
    } else {
      basic_mud_log("No playerfile.  Creating a new one.");
      touch(PLAYER_FILE);
      if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
	perror("SYSERR: fatal error opening playerfile");
	exit(1);
      }
    }
  }

  fseek(player_fl, 0L, SEEK_END);
  size = ftell(player_fl);
  rewind(player_fl);
  if (size % sizeof(struct char_file_u))
    basic_mud_log("\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!");
  recs = size / sizeof(struct char_file_u);
  if (recs)
  {
    basic_mud_log("   %ld players in database.", recs);
    NUM_PLAYERS = recs;
    CREATE(player_table, struct player_index_element, recs);
  } else {
    player_table = NULL;
    top_of_p_file = top_of_p_table = -1;
    return;
  }
  for (; !feof(player_fl);)
  {
    fread(&dummy, sizeof(struct char_file_u), 1, player_fl);
    if (!feof(player_fl))
    { /* new record */
      /* DM */
      /* add_clan_table(dummy); -- ARTUS */
      nr++;
      CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
      for (i = 0;
	   (*(player_table[nr].name + i) = /*LOWER(*/*(dummy.name + i))/*)*/; i++);
      player_table[nr].id = dummy.char_specials_saved.idnum;
      top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum);

      if (!IS_SET(dummy.char_specials_saved.act, PLR_DELETED))
      {
	if ((dummy.chclass == CLASS_MASTER) &&
	    (((dummy.level >= LVL_CHAMP) && (dummy.level < LVL_IS_GOD)) ||
	     ((dummy.player_specials_saved.unholiness > 0) && 
	      (dummy.level < LVL_IS_GOD) && (dummy.level >= 95))))
	  add_to_immlist(dummy.name, dummy.char_specials_saved.idnum,
			 dummy.player_kill_data.immkills,
			 dummy.player_specials_saved.unholiness);
	for (i = 0; i < MAX_QUEST_ITEMS; i++)
	  if (dummy.player_specials_primalsaved.quest_eq[i].vnum > 0)
	    add_to_questlist(dummy.name, dummy.char_specials_saved.idnum,
			     &dummy.player_specials_primalsaved.quest_eq[i]);
      } // !(Deleted)
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
 
      if ((clan_num >= 0) && (clan_num < CLAN_NUM))
        if ((curr=load_char(dummy.name
 
      CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
 
} */  

/* DM - add player to clan table */
/* ARTUS - perhaps not :o) 
void add_clan_table(struct char_file_u ch)
{
  int i;
  int clan_n, clan_r;

  clan_n = ch.player_specials_primalsaved.clan_num;
  clan_r = ch.player_specials_primalsaved.clan_level;
 
  / * Check for valid clan * /
  if (((clan_n >= 0) && (clan_n < NUM_CLANS)) &&
      ((clan_r >= 0) && (clan_r < NUM_CLAN_RANKS))) {
 
    sprintf(buf,"%s",ch.name);
 
    for (i=0;i < (MAX_NAME_LENGTH-strlen(ch.name)); i++)
      strcat(buf, " ");
 
    sprintf(buf2,"%s\r\n", clan_info[clan_n].ranks[clan_r]);
    strcat(buf,buf2);
 
   / * if ((strlen(buf) + strlen(clan_tables[clan_n])) < MAX_STRING_LENGTH) * /
      strcat(clan_tables[clan_n],buf);
 
  / *  realloc(clan_tables[clan_n], strlen(buf) + strlen(clan_tables[clan_n]));* /
  }
}  
*/
/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
	goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:	
  basic_mud_log("SYSERR: Unexpected end of help file.");
  exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}



void index_boot(int mode)
{
  const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
  FILE *index, *db_file;
  int rec_count = 0, size[2];
  int wizflag = FALSE;

  switch (mode) {
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
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
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_HNT:
    prefix = HNT_PREFIX;
    break;
  default:
    basic_mud_log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    basic_mud_log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (mode != DB_BOOT_HNT && *buf1 != '$') {
    sprintf(buf2, "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      basic_mud_log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
	  index_filename, strerror(errno));
      fscanf(index, "%s\n", buf1);
      continue;
    } else {
      if (mode == DB_BOOT_ZON)
	rec_count++;
      else if (mode == DB_BOOT_HLP)
	rec_count += count_alias_records(db_file);
      else
	rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (mode != DB_BOOT_HNT && !rec_count) {
    if (mode == DB_BOOT_SHP)
      return;
    basic_mud_log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
	index_filename);
    exit(1);
  }

  /* Any idea why you put this here Jeremy? */
  rec_count++;

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode) {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    basic_mud_log("   %d triggers, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    size[0] = sizeof(struct room_data) * rec_count;
    basic_mud_log("   %d rooms, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct char_data) * rec_count;
    basic_mud_log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct obj_data) * rec_count;
    basic_mud_log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    size[0] = sizeof(struct zone_data) * rec_count;
    basic_mud_log("   %d zones, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    basic_mud_log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  }

  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      basic_mud_log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }

    // DM - restrict wizhelp
    if (mode == DB_BOOT_HLP) {
      sprintf(buf,"Loading Help File: %s",buf1);
      basic_mud_log(buf);
      if (!(strcmp(buf1,"wizhelp.hlp"))) {
        basic_mud_log("Found wizhelp.hlp - applying level restrictions");
        wizflag = TRUE;
      } else
        wizflag = FALSE;
    }
      
    zone_rnum rzone;
    zone_vnum vzone;
    char *zonefilename;
    zonefilename = buf1;//str_dup(prefix);

    switch (mode) {
    case DB_BOOT_TRG:
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
      // DM
      // At this stage - the zones should always be loaded, so we find out
      // the zone rnum so we can attribute trgs, wlds, objs, mobs with the real
      // zone number - which will save large calculations later on.
      // Sacrificing some boot time for quicker run time.
      vzone = (zone_vnum)get_number(&zonefilename);
      rzone = real_zone(vzone);
    //basic_mud_log("mode = %d, zonefilename = %s, buf1 = %s", mode, zonefilename, buf1);
    //basic_mud_log("rzone = %d", rzone);
    //basic_mud_log("vzone = %d", vzone);
      discrete_load(db_file, mode, buf2, buf1, rzone);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      /*
       * If you think about it, we have a race here.  Although, this is the
       * "point-the-gun-at-your-own-foot" type of race.
       */
      load_help(db_file,wizflag);
      break;
    case DB_BOOT_SHP:
      vzone = (zone_vnum)get_number(&zonefilename);
      rzone = real_zone(vzone);
      boot_the_shops(db_file, buf2, rec_count, vzone, rzone);
      break;
    case DB_BOOT_HNT:
      vzone = (zone_vnum)get_number(&zonefilename);
      rzone = real_zone(vzone);
      //basic_mud_log("zonefilename = %s, vzone = %d, rzone = %d", zonefilename, vzone, rzone);
      load_zone_hints(db_file, rzone);
      break;
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
    //free(zonefilename);
  }
  fclose(index);

  /* sort the help index */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }
}

void load_zone_hints(FILE *fl, zone_rnum rzone) {

  int i = 0;
  char line[READ_SIZE+1];
  line[0] = '\0';

  basic_mud_log("reading hints for rzone %d", rzone);

  while (*line != '$') {
    get_line(fl, line);
    while (*line == '#') {
      get_line(fl, line);
    }

    if (*line && strcmp(line, "") && *line != '$') {
      zone_table[rzone].hints[zone_table[rzone].nohnts++] = str_dup(line);
    }

    if (i == MAX_ZONE_HINTS) {
      sprintf(buf,"SYSERR: Loaded maximum number of hints for zone %d (%d)",
	zone_table[rzone].number, MAX_ZONE_HINTS);
      basic_mud_log(buf);
    }
  }
  return;
}

void discrete_load(FILE * fl, int mode, char *filename, char *zonefilename,
                        zone_rnum rzone)
{
  int nr = -1, last;
  char line[READ_SIZE+1];
  line[0] = '\0';

  zone_vnum vznum = zone_table[rzone].number;
  //(zone_vnum)get_number(&zonefilename);
  //zone_rnum new_rzone = real_zone(vznum);

  //basic_mud_log("DEBUG: discrete load: zonefilename = %s, rzone = %d, vznum = %d", zonefilename, rzone, vznum);

  const char *modes[] = {"world", "mob", "obj", "zon", "shp", "hlp", "trg"};
  /* modes positions correspond to DB_BOOT_xxx in db.h */

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	if (nr == -1) {
	  basic_mud_log("SYSERR: %s file %s is empty!", modes[mode], filename);
	} else {
	  basic_mud_log("SYSERR: Format error in %s after %s #%d\n"
	      "...expecting a new %s, but file ended!\n"
	      "(maybe the file is not terminated with '$'?)", filename,
	      modes[mode], nr, modes[mode]);
	}
	exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	basic_mud_log("SYSERR: Format error after %s #%d", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
        case DB_BOOT_TRG:
          parse_trigger(fl, nr, vznum, rzone);
          break;
	case DB_BOOT_WLD:
          // worlds calculate real zone number by top of zone room vnum
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr, vznum, rzone);
	  break;
	case DB_BOOT_OBJ:
	  strcpy(line, parse_object(fl, nr, vznum, rzone));
	  break;
	}
    } else {
      basic_mud_log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
	  filename, modes[mode], nr);
      basic_mud_log("SYSERR: ... offending line: '%s'", line);
      exit(1);
    }
  }
}

bitvector_t asciiflag_conv(char *flag)
{
  bitvector_t flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return (flags);
}

char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);  
  } while (isspace(c));
  return c;
}

/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256], flags[128], burgleFlags[128];
  struct extra_descr_data *new_descr;
  char letter;

  sprintf(buf2, "room #%d", virtual_nr);

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    basic_mud_log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      basic_mud_log("SYSERR: Room %d is outside of any zone (%d).", virtual_nr, top_of_zone_table);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    basic_mud_log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
	virtual_nr);
    exit(1);
  }

  i = sscanf(line, " %d %s %d %s", t, flags, t + 2, burgleFlags);
  if (i != 3 && i != 4) {
    basic_mud_log("SYSERR: Format error in roomflags/sector type of room #%d",
	virtual_nr);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
  world[room_nr].room_flags = asciiflag_conv(flags);

  if (i == 4) {
    world[room_nr].burgle_flags = asciiflag_conv(burgleFlags);
  } else {
    world[room_nr].burgle_flags = 0;
  }

  world[room_nr].sector_type = t[2];

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;	/* Zero light sources */
  world[room_nr].small_bits = 0;

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  sprintf(buf,"SYSERR: Format error in room #%d (expecting D/E/S)",virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      basic_mud_log(buf);
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
      /* DG triggers -- script is defined after the end of the room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter=='T') {
        dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
        letter = fread_letter(fl);
        ungetc(letter, fl);
      }
      top_of_world = room_nr++;
      return;
    default:
      basic_mud_log(buf);
      exit(1);
    }
  }
}



/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int t[5];
  char line[256];

  sprintf(buf2, "room #%d, direction D%d", GET_ROOM_VNUM(room), dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    basic_mud_log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    basic_mud_log("SYSERR: Format error, %s", buf2);
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
  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    basic_mud_log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    if (!mini_mud)
      basic_mud_log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    if (!mini_mud)
      basic_mud_log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
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
  int cmd_no, a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda = ZCMD.arg1;
      oldb = ZCMD.arg2;
      oldc = ZCMD.arg3;
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	c = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
	b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      case 'T': /* a trigger */
        /* designer's choice: convert this later */
        /* b = ZCMD.arg2 = real_trigger(ZCMD.arg2); */
        b = real_trigger(ZCMD.arg2); /* leave this in for validation */
        break;
      case 'V': /* trigger variable assignment */
        //if (ZCMD.arg1 == WLD_TRIGGER)
        //  b = ZCMD.arg2 = real_room(ZCMD.arg2);
        if (ZCMD.arg1 == WLD_TRIGGER)
          c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      }
      if (a < 0 || b < 0 || c < 0) {
	if (!mini_mud) {
	  sprintf(buf,  "Invalid vnum %d, cmd disabled",
			 (a < 0) ? olda : ((b < 0) ? oldb : oldc));
	  log_zone_error(zone, cmd_no, buf);
	}
	ZCMD.command = '*';
      }
    }
}



void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int j, t[10];
  char line[256];
  bool hasClass;

  mob_proto[i].real_abils.str = 11;
  mob_proto[i].real_abils.intel = 11;
  mob_proto[i].real_abils.wis = 11;
  mob_proto[i].real_abils.dex = 11;
  mob_proto[i].real_abils.con = 11;
  mob_proto[i].real_abils.cha = 11;

  if (!get_line(mob_f, line)) {
    basic_mud_log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	  t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    basic_mud_log("SYSERR: Format error in mob #%d, first line after S flag\n"
	"...expecting line of form '# # # #d#+# #d#+#'", nr);
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

  if (!get_line(mob_f, line)) {
      basic_mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
	  "...expecting line of form '# #', but file ended!", nr);
      exit(1);
    }

  if (sscanf(line, " %d %d ", t, t + 1) != 2) {
    basic_mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
	"...expecting line of form '# #'", nr);
    exit(1);
  }

  GET_GOLD(mob_proto + i) = t[0];
  GET_EXP(mob_proto + i) = t[1];

  if (!get_line(mob_f, line)) {
    basic_mud_log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #', but file ended!", nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 4) {
    hasClass = FALSE;
    if( sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3 ) {
      basic_mud_log("SYSERR: Format error in last line of mob #%d\n"
        "...expecting line of form '# # #' or '# # # #'", nr);
      exit(1);
    }
  }
  else
        hasClass = TRUE;

  mob_proto[i].char_specials.position = t[0];
  mob_proto[i].char_specials.small_bits = 0;
  mob_proto[i].mob_specials.default_pos = t[1];
  mob_proto[i].player.sex = t[2];

  if( hasClass ) 
     mob_proto[i].player.chclass = t[3];
  else
     mob_proto[i].player.chclass = 0;
  mob_proto[i].player.weight = 200;
  mob_proto[i].player.height = 198;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 5; j++)
    GET_SAVE(mob_proto + i, j) = 0;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, int i, int nr)
{
  int num_arg, matched = 0;

  num_arg = atoi(value);

  CASE("BareHandAttack") {
    RANGE(0, 99);
    mob_proto[i].mob_specials.attack_type = num_arg;
  }

  CASE("Str") {
    RANGE(3, 25);
    mob_proto[i].real_abils.str = num_arg;
  }

  CASE("StrAdd") {
    RANGE(0, 100);
    mob_proto[i].real_abils.str_add = num_arg;    
  }

  CASE("Int") {
    RANGE(3, 25);
    mob_proto[i].real_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(3, 25);
    mob_proto[i].real_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(3, 25);
    mob_proto[i].real_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(3, 25);
    mob_proto[i].real_abils.con = num_arg;
  }

  CASE("Cha") {
    RANGE(3, 25);
    mob_proto[i].real_abils.cha = num_arg;
  }

  if (!matched) {
    basic_mud_log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
	    keyword, nr);
  }    
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
#if 0	/* Need to evaluate interpret_espec()'s NULL handling. */
  }
#else
  } else
    ptr = "";
#endif
  interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[256];

  parse_simple_mob(mob_f, i, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E"))	/* end of the enhanced section */
      return;
    else if (*line == '#') {	/* we've hit the next mob, maybe? */
      basic_mud_log("SYSERR: Unterminated E section in mob #%d", nr);
      exit(1);
    } else
      parse_espec(line, i, nr);
  }

  basic_mud_log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  exit(1);
}


void parse_mobile(FILE * mob_f, int nr, zone_vnum vznum, zone_rnum rznum)
{
  static int i = 0;
  int j, t[10];
  char line[256], *tmpptr, letter;
  char f1[128], f2[128];

  mob_index[i].vnum = nr;
  mob_index[i].vznum = vznum;
  mob_index[i].rznum = rznum; 
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */
  mob_proto[i].player_specials = &dummy_mob;
  sprintf(buf2, "mob vnum %d", nr);

  /***** String data *****/
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
  if (!get_line(mob_f, line)) {
    basic_mud_log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}', but file ended!", nr);
    exit(1);
  }

#ifdef CIRCLE_ACORN	/* Ugh. */
  if (sscanf(line, "%s %s %d %s", f1, f2, t + 2, &letter) != 4) {
#else
  if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4) {
#endif
    basic_mud_log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}'", nr);
    exit(1);
  }
  MOB_FLAGS(mob_proto + i) = asciiflag_conv(f1);
  SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  AFF_FLAGS(mob_proto + i) = asciiflag_conv(f2);
  GET_ALIGNMENT(mob_proto + i) = t[2];

//  MOUNTING(mob_proto + i) = NULL;
//  MOUNTING_OBJ(mob_proto + i) = NULL;

  switch (UPPER(letter)) {
  case 'S':	/* Simple monsters */
    parse_simple_mob(mob_f, i, nr);
    break;
  case 'E':	/* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, i, nr);
    break;
  /* add new mob types here.. */
  default:
    basic_mud_log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
    exit(1);
  }

  /* DG triggers -- script info follows mob S/E section */
  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter=='T') {
    dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }

  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  zone_table[rznum].nomobs++;
  top_of_mobt = i++;
}




/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr, zone_vnum vznum, zone_rnum rznum)
{
  static int i = 0;
  static char line[256];
  int t[10], j, retval, level_restrict = 0;
  char *tmpptr;
  char f1[256], f2[256];
  struct extra_descr_data *new_descr;

  obj_index[i].vnum = nr;
  obj_index[i].vznum = vznum;
  obj_index[i].rznum = rznum;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].item_number = i;

  sprintf(buf2, "object #%d", nr);

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    basic_mud_log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    basic_mud_log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, " %d %s %s %d", t, f1, f2, t + 3)) != 4) {
    if (retval == 3)
      t[3] = 0;
    else {
      basic_mud_log("SYSERR: Format error in first numeric line (expecting 4 args, got %d), %s", retval, buf2);
      exit(1);
    } 
  }
  obj_proto[i].obj_flags.type_flag = t[0];
  obj_proto[i].obj_flags.extra_flags = asciiflag_conv(f1);
  obj_proto[i].obj_flags.wear_flags = asciiflag_conv(f2);
  obj_proto[i].obj_flags.bitvector = t[3];  

  if (!get_line(obj_f, line)) {
    basic_mud_log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    basic_mud_log("SYSERR: Format error in second numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.value[0] = t[0];
  obj_proto[i].obj_flags.value[1] = t[1];
  obj_proto[i].obj_flags.value[2] = t[2];
  obj_proto[i].obj_flags.value[3] = t[3];

  if (!get_line(obj_f, line)) {
    basic_mud_log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t+4)) != 5) {
    if (retval == 3) {
	t[3] = 0;
	t[4] = default_item_damage[(int)obj_proto[i].obj_flags.type_flag];
    }
    else if (retval == 4) {
	t[4] = default_item_damage[(int)obj_proto[i].obj_flags.type_flag];
    }
    else {
      basic_mud_log("SYSERR: Format error in third numeric line (expecting 5 args, got %d), %s", retval,buf2);
      exit(1);
    }   
  }
  obj_proto[i].obj_flags.weight = t[0];
  obj_proto[i].obj_flags.cost = t[1];
  obj_proto[i].obj_flags.cost_per_day = t[2];
  obj_proto[i].obj_flags.level = t[3];  
  obj_proto[i].damage = obj_proto[i].max_damage = t[4];

  //
  // DM - ignoring unit calculations for drink containers and fountains
  // 	  using the normal weight attribute
  //
  /* check to make sure that weight of containers exceeds curr. quantity 
  if (obj_proto[i].obj_flags.type_flag == ITEM_DRINKCON ||
      obj_proto[i].obj_flags.type_flag == ITEM_FOUNTAIN) {
    if (obj_proto[i].obj_flags.weight < obj_proto[i].obj_flags.value[1])
      obj_proto[i].obj_flags.weight = obj_proto[i].obj_flags.value[1] + 5;
  } */

//  OBJ_RIDDEN(obj_proto + i) = NULL;  
  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  strcat(buf2, ", after numeric constants\n"
	 "...expecting 'E', 'A', '$', or next object number");


  obj_proto[i].materials = NULL;
  obj_proto[i].products    = NULL;
  j = 0;
  for (;;) {
    if (!get_line(obj_f, line)) {
      basic_mud_log("SYSERR: Format error in (%c): %s", *line, buf2);
    //  basic_mud_log("SYSERR: Format error in %s", buf2);
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
	basic_mud_log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      if (!get_line(obj_f, line)) {
	basic_mud_log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }

      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	basic_mud_log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
      }
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      j++;
      break;
      
    // All appears obsolete now ...
    case 'L':
	if( level_restrict ) {
		fprintf(stderr, "Multiple level restriction defined.\n");
		exit(1);
	}
	sscanf(line+1, " %d", &level_restrict);

        /*
	if( ( level_restrict > LVL_GRGOD ) || ( level_restrict < 0) ) {
		fprintf(stderr, "Level restrict out of range (0 -> LVL_GRGOD).");
		exit(1);
	}
        */
	
	// obj_proto[i].obj_flags.extra_flags |= level_restrict << 17;
	/* Revised levelling flags */

 	obj_proto[i].obj_flags.level_flags |= level_restrict;
	break;

    case 'T':  /* DG triggers */
      dg_obj_trigger(line, &obj_proto[i]);
      break;
    case 'P':
      int prodVnum;
      int prodSkill;
      int prodLevel;
      int numArgs;
      numArgs = sscanf(line+1, " %d %d %d", &prodVnum, &prodSkill, &prodLevel);
      if (obj_proto[i].products == NULL) { // firse one in the list so create the list
        obj_proto[i].products = new list<ObjProductClass>;
      }
      // create an new ObjProductClass and put it in the list
      ObjProductClass *opc;
      opc = new ObjProductClass(prodVnum, prodSkill, prodLevel);
      obj_proto[i].products->push_back(*opc);
      break;
    case 'M':
      int materialVnum;
      int materialNumber;
      int sFlag;
      int fFlag;
      numArgs = sscanf(line+1, " %d %d %d %d", &materialVnum, &materialNumber, &sFlag, &fFlag);
      if (numArgs < 2) {
        // error
        basic_mud_log("SYSERR: Format error in %s, line %d", line, vznum );
        exit(1);
      }
      
      if (obj_proto[i].materials == NULL) {
        basic_mud_log("Object has materials");
        obj_proto[i].materials = new list<ObjMaterialClass>;
      }
      ObjMaterialClass *omc;
      omc = new ObjMaterialClass(materialVnum, materialNumber, sFlag, fFlag);
      obj_proto[i].materials->push_back(*omc);
      break;
    case '$':
    case '#':
      check_object(&obj_proto[i]);
      top_of_objt = i++;
      zone_table[rznum].noobjs++;
      return (line);
    default:
      basic_mud_log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
  }
}


#define Z	zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static zone_rnum zone = 0;
  int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error;
  char *ptr, buf[256], zname[256];
  char t1[80], t2[80];

  strcpy(zname, zonename);

  while (get_line(fl, buf))
    num_of_cmds++;		/* this should be correct within 3 or so */
  rewind(fl);

  if (num_of_cmds == 0) {
    basic_mud_log("SYSERR: %s is empty!", zname);
    exit(1);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%hd %d", &Z.number, &Z.world) != 2) {
    basic_mud_log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  sprintf(buf2, "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = str_dup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %hd %d %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode, &Z.zflag) != 4) {   
  //if (sscanf(buf, " %hd %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode) != 3) {
    basic_mud_log("SYSERR: Format error in 4-constant line of %s", zname);
   // basic_mud_log("SYSERR: Format error in 3-constant line of %s", zname);
    exit(1);
  }
  cmd_no = 0;

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0) {
      basic_mud_log("SYSERR: Format error in %s - premature end of file", zname);
      exit(1);
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
    if (strchr("MOEPDTV", ZCMD.command) == NULL) {     /* a 3-arg command */
      if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
	error = 1;
    } else if (ZCMD.command=='V') { /* a string-arg command */
      if (sscanf(ptr, " %d %d %d %d %s %s", &tmp, &ZCMD.arg1, &ZCMD.arg2, 
                              &ZCMD.arg3, t1, t2) != 6)
        error = 1;
      else {
        ZCMD.sarg1 = str_dup(t1);
        ZCMD.sarg2 = str_dup(t2);
      }
    } else {
      if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
		 &ZCMD.arg3) != 4)
	error = 1;
    }

    ZCMD.if_flag = tmp;

    if (error) {
      basic_mud_log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  zone_table[top_of_zone_table].nowlds = 0;
  zone_table[top_of_zone_table].noobjs = 0;
  zone_table[top_of_zone_table].nomobs = 0;
  zone_table[top_of_zone_table].notrgs = 0;
  zone_table[top_of_zone_table].noshps = 0;
  zone_table[top_of_zone_table].nohnts = 0;
  for (int i = 0; i < MAX_ZONE_HINTS; i++) {
    zone_table[top_of_zone_table].hints[i] = NULL;
  }
  top_of_zone_table = zone++;
  //basic_mud_log("DEBUG: loaded zone %s, top_of_zone_table = %d", zname, top_of_zone_table);
}

#undef Z


void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL)
  {
    basic_mud_log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}

// load the hints file (one hint is a line, # starting a line is a comment
// file ends with $
void load_hints()
{
  FILE *hint_file;
  char line[READ_SIZE+1]; 

  if (!(hint_file = fopen(HINTS_FILE, "r"))) {
    basic_mud_log("SYSERR: File '%s' : %s", HINTS_FILE, strerror(errno));
    exit(1);  
  } 

  while (*line != '$') {
    get_line(hint_file, line);
    while (*line == '#') {
      get_line(hint_file, line);
    }
    //strcat(line,"\r\n"); 
    if (*line != '$') {
      hint_table[num_hints++] = str_dup(line);
    }
    if (num_hints == MAX_HINTS) {
      sprintf(buf,"SYSERR: Loaded maximum number of hints (%d)",MAX_HINTS);
      basic_mud_log(buf);
      fclose(hint_file);
    }
  }
  sprintf(buf,"   Loaded %d hints...",num_hints);
  basic_mud_log(buf);
  fclose(hint_file);
}


void load_help(FILE *fl, int wizflag)
{
#if defined(CIRCLE_MACINTOSH)
  static char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384]; /* ? */
#else
  char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384];
#endif
  char line[READ_SIZE+1], *scan;
  struct help_index_element el;

  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$') {
    /* read in the corresponding help entry */
    strcpy(entry, strcat(key, "\r\n"));
    get_one_line(fl, line);
    while (*line != '#') {
      strcat(entry, strcat(line, "\r\n"));
      get_one_line(fl, line);
    }

    // DM - for further expansion, you can add a check for a level
    // restriction at the end of the keyword line I guess
     
    // DM - apply the level restriction on help
    if (wizflag)
      el.level_restriction = LVL_ANGEL;
    else
      el.level_restriction = 0;

    /* now, add the entry to the index with each keyword on the keyword line */
    el.duplicate = 0;
    el.entry = str_dup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keyword = str_dup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }

    /* get next keyword line (or $) */
    get_one_line(fl, key);
  }
}

int check_spell_values(int spell_type, int lineno, sh_int intl, sh_int wis, sh_int dex, sh_int str, 
	sh_int cha, sh_int con, int min_level, int mana_min, int mana_max, int mana_change, 
        int spell_effec, int mana_perc)
{
  // Check the Values
  if (intl > MAX_STAT_VAL || intl < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for INT", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (wis > MAX_STAT_VAL || wis < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for WIS", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (dex > MAX_STAT_VAL || dex < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for DEX", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (str > MAX_STAT_VAL || str < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for STR", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (cha > MAX_STAT_VAL || cha < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for CHA", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (con > MAX_STAT_VAL || con < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for CON", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (min_level > LVL_OWNER || min_level < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for LEVEL (%d)", PRIMAL_SP_FILE, lineno, min_level);
    basic_mud_log(buf);
    return FALSE;
  }
  if (mana_max > 10000 || mana_max < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for MANA_MAX", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (mana_min > 10000 || mana_min < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for MANA_MIN", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (mana_change > 10000 || mana_change < 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for MANA_CHANGE", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (spell_effec > 300 || spell_effec <= 0)
  {
    sprintf(buf, "%s, line %d: Invalid value for SPELL_EFFEC", PRIMAL_SP_FILE, lineno);
    basic_mud_log(buf);
    return FALSE;
  }
  if (spell_type == SPELL)
  {
    if (mana_perc > 300 || mana_perc <= 0)
    {
      sprintf(buf, "%s, line %d: Invalid value for MANA_CHANGE", PRIMAL_SP_FILE, lineno);
      basic_mud_log(buf);
      return FALSE;
    }
  }

  return TRUE;
}

void add_spell_help(FILE *spell_help_file)
{
  int rec_count=0, size;

  rec_count += count_alias_records(spell_help_file);
  rewind(spell_help_file);

  RECREATE(help_table, struct help_index_element, top_of_helpt + rec_count);  

  load_help(spell_help_file, FALSE);

  size = sizeof(struct help_index_element) * (rec_count+top_of_helpt);

  basic_mud_log("Reallocating help table with spell help entries:");
  sprintf(buf,"   %d entries, %d bytes.", top_of_helpt + rec_count - 1, size);
  basic_mud_log(buf);

  basic_mud_log("Sorting help table.");
  qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
  top_of_helpt--;
}

int get_lowest_valid_level(int base_min_level[SECOND_REMORT_CLASS - 1], int bclass1, int bclass2, int level) {

  int bl1 = base_min_level[bclass1];
  int bl2 = base_min_level[bclass2];

  if (bl1 > 0 && bl2 > 0) {
    return MIN(bl1, bl2);
  }

  if (bl1 < 0) {
    if (bl2 < 0) {
      return level;
    } else {
      return bl2;
    }
  } else {
    return bl1;
  }
  return level;
}

#if 0
int get_class_level(int base_min_lev[SECOND_REMORT_CLASS - 1], int class_index, int level) 
{
  return base_min_lev[class_index];
  
  switch (class_index) {
    case CLASS_MAGIC_USER:
    case CLASS_CLERIC:
    case CLASS_THIEF:
    case CLASS_WARRIOR:
      return base_min_lev[class_index];

    // lowest valid level for base classes - 10
    case CLASS_DRUID: // CM
      return MAX(1, get_lowest_valid_level(base_min_lev, CLASS_CLERIC, CLASS_MAGIC_USER, level) - 10);
    case CLASS_PRIEST: // CT
      return MAX(1, get_lowest_valid_level(base_min_lev, CLASS_CLERIC, CLASS_THIEF, level) - 10);
    case CLASS_NIGHTBLADE: //TW
      return MAX(1, get_lowest_valid_level(base_min_lev, CLASS_WARRIOR, CLASS_THIEF, level) - 10);
    case CLASS_BATTLEMAGE: //MW
      return MAX(1, get_lowest_valid_level(base_min_lev, CLASS_MAGIC_USER, CLASS_WARRIOR, level) - 10);
    case CLASS_SPELLSWORD: //TM
      return MAX(1, get_lowest_valid_level(base_min_lev, CLASS_MAGIC_USER, CLASS_THIEF, level) - 10);
    case CLASS_PALADIN: //CW
      return MAX(1, get_lowest_valid_level(base_min_lev, CLASS_CLERIC, CLASS_WARRIOR, level) - 10);

    // lowest valid level for all classes - 20
    case CLASS_MASTER:
      return (MAX(1, MIN(get_lowest_valid_level(base_min_lev, CLASS_CLERIC, CLASS_WARRIOR, level), 
	       get_lowest_valid_level(base_min_lev, CLASS_MAGIC_USER, CLASS_THIEF, level)) - 20)); 
    default:
      return LVL_IMPL;
  }
}
#endif

// Processes spells and skills in PRIMAL_SP_FILE and loads the data into the
// spell_info struct. First add the data into temp_spell_info and if sucessful
// memcopy it to spell_info and returns TRUE, otherwise returns FALSE
int load_primal_spell_levels()
{
  FILE *db_file, *help_file;
  int bad = 0, temp, spell_type, lineno = 0, while_count, found;
  int class_index;
  unsigned int index;
  int mana_min, mana_max, mana_change, min_level, spell_effec, mana_perc;
  sh_int intl, wis, dex, con, cha, str;
  int spellnum;
  int i, j;
  struct spell_info_type temp_spell_info[TOP_SPELL_DEFINE + 1];
  int num_spells = 0;

  char help_entry[MAX_STRING_LENGTH], *scan;
  char name[READ_SIZE+1]; 
  char class_name[READ_SIZE+1], spell_ident[READ_SIZE+1]; 
  char line[READ_SIZE+1], help_line[READ_SIZE+1], temp_line[READ_SIZE+1];
                       
  if (!(db_file = fopen(PRIMAL_SP_FILE, "r")))
  {
    basic_mud_log("SYSERR: File '%s' : %s", PRIMAL_SP_FILE, strerror(errno));
    //exit(1);
    return FALSE;  
  } 

  if (!(help_file = fopen("text/help/spells.hlp", "w")))
  {
    basic_mud_log("SYSERR: Temp file 'text/help/spells.hlp' : %s", strerror(errno));
    //exit(1);  
    return FALSE;
  } 
  rewind(help_file);

  // set temp_spell_info to defaults

  for (i = 0; i < TOP_SPELL_DEFINE; i++)
  { 
    for (j = 0; j < NUM_CLASSES; j++)
    {
      temp_spell_info[i].min_level[j] = LVL_OWNER + 1;
      temp_spell_info[i].mana_max[j] = -1;
      temp_spell_info[i].mana_min[j] = -1;
      temp_spell_info[i].mana_change[j] = -1;
      temp_spell_info[i].intl[j] = -1;
      temp_spell_info[i].dex[j] = -1;
      temp_spell_info[i].wis[j] = -1;
      temp_spell_info[i].str[j] = -1;
      temp_spell_info[i].con[j] = -1;
      temp_spell_info[i].cha[j] = -1;
      // Artus> Spell Effectiveness should probably be here too..
      temp_spell_info[i].spell_effec[j] = 0;
    }
    temp_spell_info[i].min_position = 0;
    temp_spell_info[i].targets = 0;
    temp_spell_info[i].violent = 0;
    temp_spell_info[i].routines = 0;
    temp_spell_info[i].name = unused_spellname;  
  }
  
  // First Line
  lineno += get_line(db_file, line);
  
  // Ignore Initial Comments
  while (*line == '#')
    lineno += get_line(db_file, line);

  while (*line != '$')
  {
    strcpy(temp_line,line);
    while (*temp_line != 'S' && *temp_line != 'K' && *temp_line+1 != ' ')
    {
      if (*line == '$')
      {
        //memcpy(&spell_info, &temp_spell_info, sizeof(struct spell_info_type)*num_spells);
        for (i = 0; i < TOP_SPELL_DEFINE; i++)
	{
          for (j = 0; j < NUM_CLASSES; j++)
	  {
            spell_info[i].min_level[j] = temp_spell_info[i].min_level[j];
            spell_info[i].mana_max[j] = temp_spell_info[i].mana_max[j];
            spell_info[i].mana_min[j] = temp_spell_info[i].mana_min[j];
            spell_info[i].mana_change[j] = temp_spell_info[i].mana_change[j];
            spell_info[i].intl[j] = temp_spell_info[i].intl[j];
            spell_info[i].dex[j] = temp_spell_info[i].dex[j];
            spell_info[i].wis[j] = temp_spell_info[i].wis[j];
            spell_info[i].str[j] = temp_spell_info[i].str[j];
            spell_info[i].con[j] = temp_spell_info[i].con[j];
            spell_info[i].cha[j] = temp_spell_info[i].cha[j];
	    // Artus> Spell Effectiveness should probably be here too..
	    spell_info[i].spell_effec[j] = temp_spell_info[i].spell_effec[j];
          }
//          spell_info[i].min_position = temp_spell_info[i].min_position;
//          spell_info[i].targets = temp_spell_info[i].targets;
//          spell_info[i].violent = temp_spell_info[i].violent;
//          spell_info[i].routines = temp_spell_info[i].routines;
//          strcpy(spell_info[i].name, temp_spell_info[i].name); 
        }
	fputs("$\r\n",help_file);
        fflush(help_file);
	fclose(help_file);

// DM - used to use add_spell_help - now just create the file and let
// the normal help load it ... if we reload the spells - make sure we reload
// the help
        
//	help_file = fopen("tempspell.hlp", "r");
//	rewind(help_file);
//	add_spell_help(help_file);
//      fclose(help_file);
        return TRUE;
      }
      if (*line == '#') {
        lineno += get_line(db_file, line);
      }
      strcpy(temp_line,line);
    }

    //    sprintf(buf,"Parsing Line: %s",line);
    //    basic_mud_log(buf);

    
    if (sscanf(buf, "%c ", &spell_ident[0]) != 1) {
      //error
    }

    strcpy(temp_line,one_argument(temp_line,spell_ident));
    scan = one_word(temp_line,name);

    //DEBUG
    //sprintf(buf,"Spell_Ident: %c, Spell Name: %s", spell_ident[0], name);
    //basic_mud_log(buf);

    //    if (sscanf(line,"%c %s", &spell_ident, name) != 2) {
    //   sprintf(buf, "%s, line %d: Spell/Skill name expected but not found...",
    //    PRIMAL_SP_FILE, lineno);
    //  basic_mud_log(buf);
    //  exit(1);

/* Artus> This just doesn't make sense.
    if (FALSE)
    {
    } else {
*/
    to_upper(name);
    to_upper(spell_ident);

    switch (spell_ident[0])
    {
      case 'S':
	spell_type = SPELL;
	lineno += get_line(db_file, line);
	if (sscanf(line, "%d %d %d", &mana_min, &mana_max, &mana_change) != 3)
	{
	  sprintf(buf,"%s, line %d: Expecting Mana Line, Skipping spell %s", 
		  PRIMAL_SP_FILE, lineno, name);
	  basic_mud_log(buf);
	  return FALSE;
	  //exit(1);
	}
        break;
 
      case 'K':
	spell_type = SKILL;
	mana_min = 0;
	mana_max = 0;
	mana_change = 0;      
	mana_perc = 0;
	break;

      default:
	sprintf(buf,"%s, line %d: Spell/Skill type must be 'S' or 'K'",
		PRIMAL_SP_FILE,lineno);
	basic_mud_log(buf);
	//exit(1);
	return FALSE;
	break;
    }
    lineno += get_line(db_file, line);
    found = TRUE;
    // Find Spell here before checking class
    spellnum = find_skill_num(name);
    if (spellnum < 0)
      found = FALSE;
    if (!found)
    {
      sprintf(buf,"%s, line %d: Undefined Spell/Skill (%s)", PRIMAL_SP_FILE,
	      lineno, name);
      basic_mud_log(buf);
      return FALSE;
      //exit(1);
    } else {
      int base_min_level[SECOND_REMORT_CLASS - 1];
      while_count = 0;
      for (int i = 0; i < SECOND_REMORT_CLASS-1; i++)
	base_min_level[i] = -1;
	
      // Parse the class lines
      while (*line != '~' && while_count <= NUM_CLASSES) 
      {
	//  sprintf(buf,"Parsing Class Line [%s]: %s",name,line);
	//  basic_mud_log(buf);
	if (*line == '$') 
	{
	  // BAIL
	  sprintf(buf,"%s, line %d: EOF found before expected", 
		  PRIMAL_SP_FILE, lineno);
	  basic_mud_log(buf);
	  return FALSE;
	  //exit(1);
	}
	while_count++;
	bad = FALSE;
	if (spell_type == SPELL) 
	{
	  if ((temp = sscanf(line, "%s %hd %hd %hd %hd %hd %hd %d %d %d", 
			     class_name, &intl, &str, &wis, &con, &dex, &cha, 
			     &min_level, &spell_effec, &mana_perc)) != 10) 
	  {
	    sprintf(buf,"%s, line %d: Incorrect Format. Expecting Class Spell Defintion or ~", PRIMAL_SP_FILE, lineno);
	    basic_mud_log(buf);
	    return FALSE;
	    //exit(1);
	  } 
	  // SKILL
	} else {
	  if ((temp = sscanf(line, "%s %hd %hd %hd %hd %hd %hd %d %d", 
			     class_name, &intl, &str, &wis, &con, &dex, &cha, 
			     &min_level, &spell_effec)) != 9) 
	  {
	    sprintf(buf,"%s, line %d: Incorrect Format. Expecting Class Skill Defintion or ~", PRIMAL_SP_FILE, lineno);
	    basic_mud_log(buf);
	    return FALSE;
	    //exit(1);
	  }
	  // basic_mud_log(buf); // Artus> Why? (2004-05-10)
	}
	if (!check_spell_values(spell_type, lineno, intl, wis, dex, str, cha, 
				con, min_level, mana_min, mana_max, 
				mana_change, spell_effec, mana_perc))
	{
	  return FALSE;
	  //exit(1);
	} 
	class_index = search_block_case_insens(class_name,pc_class_types,TRUE);
	// sprintf(buf,"%s %s %d",class_name,pc_class_types[1],class_index);
	// basic_mud_log(buf);

	// Check class name
	if (class_index > -1) 
	{ 
	  
	  // Redefining stats for an already defined class spell.
	  if (temp_spell_info[spellnum].min_level[class_index] != (LVL_OWNER+1))
	  {
	    sprintf(buf,"%s, line %d: Class already defined for Spell/Skill (%s) %d", PRIMAL_SP_FILE, lineno, name, temp_spell_info[spellnum].min_level[class_index]);
	    basic_mud_log(buf);
	    return FALSE;
	    //exit(1);
	  }

	  // Calculate the remort class min_level based on the existing
	  // base_min_level values
	  // ie. the lowest valid level for the existing base classes...
//	  if (class_index < SECOND_REMORT_CLASS)
//	    base_min_level[class_index] = min_level;
//	  else
//	    min_level = get_class_level(base_min_level, class_index, min_level);
	  base_min_level[class_index] = min_level;
	  temp_spell_info[spellnum].min_level[class_index] = min_level;
	  // Recall spell always level 15 ...
	  if (spellnum == SPELL_WORD_OF_RECALL)
	    temp_spell_info[spellnum].min_level[class_index] = 15;
	  temp_spell_info[spellnum].spell_type = spell_type;
	  temp_spell_info[spellnum].mana_min[class_index] = mana_min;
	  temp_spell_info[spellnum].mana_max[class_index] = mana_max;
	  temp_spell_info[spellnum].mana_change[class_index] = mana_change;
	  temp_spell_info[spellnum].str[class_index] = str;
	  temp_spell_info[spellnum].intl[class_index] = intl;
	  temp_spell_info[spellnum].wis[class_index] = wis;
	  temp_spell_info[spellnum].dex[class_index] = dex;
	  temp_spell_info[spellnum].con[class_index] = con;
	  temp_spell_info[spellnum].cha[class_index] = cha;
	  temp_spell_info[spellnum].mana_perc[class_index] = mana_perc;
	  temp_spell_info[spellnum].spell_effec[class_index] = spell_effec;
	  // basic_mud_log(buf); // Artus> Why? 2004-05-10
	  num_spells++;
	} else { 
	  sprintf(buf,"%s, line %d: Undefined class name (%s)",PRIMAL_SP_FILE,
		  lineno, class_name);
	  basic_mud_log(buf);
	  return FALSE;
	  //exit(1);
	}
	lineno += get_line(db_file,line);
      } // found ~ or while_count = 10

      if (while_count > NUM_CLASSES)
      {
	// lines of junk??
	sprintf(buf,"%s, line: %d: Maximum number of classes exceeded.", 
		PRIMAL_SP_FILE, lineno);
	basic_mud_log(buf);
	return FALSE;
	//exit(1);
      } else { // Now add help to help_index
	lineno++;
	get_one_line(db_file, help_line);
	strcpy(help_entry,"");
	// if name is more than one word, enclose in " "
	found = FALSE;
	for (index=0;index<strlen(name);index++)
	  if (isspace(name[index]))
	    found = TRUE;
	if (found)
	  sprintf(buf,"\"%s\"\r\n",name);
	else
	  sprintf(buf,"%s\r\n",name);
	strcat(help_entry,buf);
	while (*help_line != '~')
	{
	  //  sprintf(buf,"Parsing Help Line: %s",help_line);
	  // basic_mud_log(buf);
	  if (*line == '$')
	  {
	    sprintf(buf,"%s, line %d: End of help char (~) not found.",PRIMAL_SP_FILE,lineno);
	    basic_mud_log(buf);
	    return FALSE;
	    //exit(1);
	  }
	  if (*line != '#')
	  { 
	    strcat(help_entry, help_line);
	    strcat(help_entry,"\r\n");
	  }
	  get_one_line(db_file, help_line);
	  lineno++;
	}
	strcat(help_entry, "#\r\n");
	fputs(help_entry, help_file);
	fflush(help_file);
	lineno += get_line(db_file,line);
      } // End of parse class lines 
    } // End of found defined spell
  } // End of ok spell line parse 
//  } // End of main while loop

  //memcpy(&spell_info, &temp_spell_info, sizeof(struct spell_info_type)*num_spells);
  for (i = 0; i < TOP_SPELL_DEFINE; i++) 
  {
    for (j = 0; j < NUM_CLASSES; j++) 
    {
      spell_info[i].min_level[j] = temp_spell_info[i].min_level[j];
      spell_info[i].mana_max[j] = temp_spell_info[i].mana_max[j];
      spell_info[i].mana_min[j] = temp_spell_info[i].mana_min[j];
      spell_info[i].mana_change[j] = temp_spell_info[i].mana_change[j];
      spell_info[i].intl[j] = temp_spell_info[i].intl[j];
      spell_info[i].dex[j] = temp_spell_info[i].dex[j];
      spell_info[i].wis[j] = temp_spell_info[i].wis[j];
      spell_info[i].str[j] = temp_spell_info[i].str[j];
      spell_info[i].con[j] = temp_spell_info[i].con[j];
      spell_info[i].cha[j] = temp_spell_info[i].cha[j];
      // Artus> Might want this, too :o)
      spell_info[i].spell_effec[j] = temp_spell_info[i].spell_effec[j];
    }
//    spell_info[i].min_position = temp_spell_info[i].min_position;
//    spell_info[i].targets = temp_spell_info[i].targets;
//    spell_info[i].violent = temp_spell_info[i].violent;
//    spell_info[i].routines = temp_spell_info[i].routines;
//    strcpy(spell_info[i].name, temp_spell_info[i].name);
  }
  fputs("$\r\n",help_file);
  fflush(help_file);
  fclose(help_file);
  return TRUE;
// DM - used to use add_spell_help - now just create the file and let
// the normal help load it ... if we reload the spells - make sure we reload
// the help
        
//  help_file = fopen("tempspell.hlp", "r");
//  rewind(help_file);
//  add_spell_help(help_file);
//  fclose(help_file);
}

int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/

int list_mobiles(struct char_data *ch, zone_vnum vznum, int type) {
  int nr, found = 0;
        
  buf[0] = '\0';
  if (real_zone(vznum) != NOWHERE) {
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_index[nr].vznum == vznum) {
        if (type == NOTHING || mob_proto[nr].player.chclass == type) {
          sprintf(buf2, "%3d. &8[%5d]&n &6%s&n\r\n", ++found,
                  mob_index[nr].vnum, mob_proto[nr].player.short_descr);
          strncat(buf, buf2, strlen(buf2)); 
        }
      }
    }       
    page_string(ch->desc, buf, TRUE);
    return TRUE;
  } 
  return FALSE;
}

int vnum_mobile(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  buf[0] = '\0';

  for (nr = 0; nr <= top_of_mobt; nr++) {
    if (isname(searchname, mob_proto[nr].player.name)) {
      sprintf(buf1, "%3d. &8[%5d]&n &6%s&n\r\n", ++found,
	      mob_index[nr].vnum,
	      mob_proto[nr].player.short_descr);
      strncat(buf, buf1, strlen(buf1));
    }
  }
  if (found) {
    page_string(ch->desc, buf, TRUE);
  }

  return (found);
}


int list_objects(struct char_data *ch, zone_vnum vznum, int type) {
  int nr, found = 0;
        
  buf[0] = '\0';
  if (real_zone(vznum) != NOWHERE) {
    for (nr = 0; nr <= top_of_objt; nr++) {
      if (obj_index[nr].vznum == vznum) {
        if (type == NOTHING || obj_proto[nr].obj_flags.type_flag == type) {
          sprintf(buf2, "%3d. &8[%5d]&n &5%s&n\r\n", ++found,
                  obj_index[nr].vnum, obj_proto[nr].short_description);
          strncat(buf, buf2, strlen(buf2)); 
        }
      }
    }       
    page_string(ch->desc, buf, TRUE);
    return TRUE;
  } 
  return FALSE;
}


int vnum_object(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  buf[0] = '\0';

  for (nr = 0; nr <= top_of_objt; nr++) {
    if (isname(searchname, obj_proto[nr].name)) {
      sprintf(buf1, "%3d. &8[%5d]&n &5%s&n\r\n", ++found,
	      obj_index[nr].vnum,
	      obj_proto[nr].short_description);
      strncat(buf, buf1, strlen(buf1));
    }
  }
  if (found) {
    page_string(ch->desc, buf, TRUE);
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
  GET_ID(ch) = max_id++;

  return (ch);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL)
  {
    if ((i = real_mobile(nr)) < 0)
    {
      basic_mud_log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (NULL);
    }
  } else {
    i = nr;
  }

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  if (!mob->points.max_hit)
  {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
      mob->points.move;
  } else {
    mob->points.max_hit = number(mob->points.hit, mob->points.mana);
  }
  if (GET_SEX(mob) == SEX_RANDOM)
  {
    if (number(0, 1) == 0) 
      GET_SEX(mob) = SEX_MALE;
    else
      GET_SEX(mob) = SEX_FEMALE;
  }
  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);

  mob_index[i].number++;
  GET_ID(mob) = max_id++;
  assign_triggers(mob, MOB_TRIGGER);

  return (mob);
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;
  GET_ID(obj) = max_id++;
  assign_triggers(obj, OBJ_TRIGGER);

  return (obj);
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;
  obj_rnum i;

  if (nr < 0) {
    basic_mud_log("SYSERR: Trying to create obj with negative (%d) num!", nr);
    return (NULL);
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      basic_mud_log("Object (V) %d does not exist in database.", nr);
      return (NULL);
    }
  } else
    i = nr;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;
  GET_ID(obj) = max_id++;
  assign_triggers(obj, OBJ_TRIGGER);

  return (obj);
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
  }	/* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
	_is_empty(update_u->zone_to_reset)) {
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

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  char buf[256];

  sprintf(buf, "SYSERR: zone file: %s", message);
  mudlog(buf, NRM, LVL_GOD, TRUE);

  sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	  ZCMD.command, zone_table[zone].number, ZCMD.line);
  mudlog(buf, NRM, LVL_GOD, TRUE);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
  int cmd_no, last_cmd = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;
  //int room_vnum, room_rnum;
  struct char_data *tmob=NULL; /* for trigger assignment */
  struct obj_data *tobj=NULL;  /* for trigger assignment */
  room_vnum rvnum;
  room_rnum rrnum;

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
        load_mtrigger(mob);
        tmob = mob;
	last_cmd = 1;
      } else
	last_cmd = 0;
      tobj = NULL;
      break;

    case 'O':			/* read an object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	if (ZCMD.arg3 >= 0) {
	  obj = read_object(ZCMD.arg1, REAL);
	  obj_to_room(obj, ZCMD.arg3);
          load_otrigger(obj);
          tobj = obj;
	  last_cmd = 1;
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  obj->in_room = NOWHERE;
          tobj = obj;
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      tmob = NULL;
      break;

    case 'P':			/* object to object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
	  ZONE_ERROR("target obj not found, command disabled");
	  ZCMD.command = '*';
	  break;
	}
	obj_to_obj(obj, obj_to);
        load_otrigger(obj);
        tobj = obj;
	last_cmd = 1;
      } else
	last_cmd = 0;
      tmob = NULL;
      break;

    case 'G':			/* obj_to_char */
      if (!mob) {
	ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
	ZCMD.command = '*';
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	obj_to_char(obj, mob, __FILE__, __LINE__);
        tobj = obj;
        load_otrigger(obj);
	last_cmd = 1;
      } else
	last_cmd = 0;
      tmob = NULL;
      break;

    case 'E':			/* object to equipment list */
      if (!mob) {
	ZONE_ERROR("trying to equip non-existant mob, command disabled");
	ZCMD.command = '*';
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  //equip_char(mob, obj, ZCMD.arg3);
          //IN_ROOM(obj) = IN_ROOM(mob);
          load_otrigger(obj);
          if (wear_otrigger(obj, mob, ZCMD.arg3))
            equip_char(mob, obj, ZCMD.arg3, FALSE);
          else
            obj_to_char(obj, mob, __FILE__, __LINE__);
          tobj = obj;

	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      tmob = NULL;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL)
        extract_obj(obj);
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
      break;


    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
	ZONE_ERROR("door does not exist, command disabled");
	ZCMD.command = '*';
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
      tmob = NULL;
      tobj = NULL;
      break;
    case 'T': /* trigger command; details to be filled in later */
      if (ZCMD.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob))
          CREATE(SCRIPT(tmob), struct script_data, 1);
        add_trigger(SCRIPT(tmob), read_trigger(real_trigger(ZCMD.arg2)), -1);
        last_cmd = 1;
      } else if (ZCMD.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj))
          CREATE(SCRIPT(tobj), struct script_data, 1);
        add_trigger(SCRIPT(tobj), read_trigger(real_trigger(ZCMD.arg2)), -1);
        last_cmd = 1;
      }
      break;
      
    case 'V':
      if (ZCMD.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob)) {
          ZONE_ERROR("Attempt to give variable to scriptless mobile");
        } else
          add_var(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2, 
                          ZCMD.arg3);
        last_cmd = 1;
      } else if (ZCMD.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj)) {
          ZONE_ERROR("Attempt to give variable to scriptless object");
        } else
          add_var(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1, ZCMD.sarg2, 
                          ZCMD.arg3);
        last_cmd = 1;
      } else if (ZCMD.arg1==WLD_TRIGGER) {
        if (ZCMD.arg2<0 || ZCMD.arg2>top_of_world) {
          ZONE_ERROR("Invalid room number in variable assignment");
        } else {
          if (!(world[ZCMD.arg2].script)) {
            ZONE_ERROR("Attempt to give variable to scriptless object");
          } else
            add_var(&(world[ZCMD.arg2].script->global_vars), ZCMD.sarg1, 
                            ZCMD.sarg2, ZCMD.arg3);
          last_cmd = 1;
        }
      }
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;

  /* handle reset_wtrigger's */
  rvnum = zone_table[zone].number * 100;
  while (rvnum <= zone_table[zone].top) {
    rrnum = real_room(rvnum);
    if (rrnum != NOWHERE) 
      reset_wtrigger(&world[rrnum]);
    rvnum++;
  }
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int _is_empty(zone_rnum zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
  {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;
    if (GET_LEVEL(i->character) >= LVL_IS_GOD)
      continue;
    if (world[i->character->in_room].zone != zone_nr)
      continue;
    return (0);
  }
  return (1);
}





/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/


long get_ptable_by_name(char *name)
{
  int i;

  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, arg))
      return (i);

  return (-1);
}


long get_id_by_name(char *name)
{
  int i;

  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, arg))
      return (player_table[i].id);

  return (-1);
}


char *get_name_by_id(long id)
{
  int i;
  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name);
  return (NULL);
}


/* Load a char, TRUE if loaded, FALSE if not */
int load_char(char *name, struct char_file_u * char_element)
{
  int player_i;
  if ((player_i = find_name(name)) >= 0) {
    fseek(player_fl, (long) (player_i * sizeof(struct char_file_u)), SEEK_SET);
    fread(char_element, sizeof(struct char_file_u), 1, player_fl);
    return (player_i);
  } else {
    return (-1);
  }
}

// set the last unsuccessful host and time, since it is unsuccessful, the
// hostname is obtained within the given descriptor
// eg. When a character is already logged on, and another attempt is made to 
// log that char on, we obtain the hostname from the failed descriptor. 
void set_unsuccessful_logon(struct char_data *ch, struct descriptor_data *desc,
                struct char_file_u *st) {
  GET_BAD_PWS(ch)++;
  strncpy(ch->player_specials->primalsaved.lastUnsuccessfulHost,
                  desc->host, HOST_LENGTH);
  ch->player_specials->primalsaved.lastUnsuccessfulHost[HOST_LENGTH] = '\0';
  ch->player_specials->primalsaved.lastUnsuccessfulLogon = time(0);
  if (st != NULL) {
    strncpy(st->player_specials_primalsaved.lastUnsuccessfulHost, 
                    desc->host, HOST_LENGTH);
    st->player_specials_primalsaved.lastUnsuccessfulHost[HOST_LENGTH] = '\0';
    st->player_specials_primalsaved.lastUnsuccessfulLogon = time(0); 
  }
}

// set the last successful host and time, as it is successful, the hostname
// is obtained within the descriptor for the character
void set_successful_logon(struct char_data *ch, struct char_file_u *st) {
  strncpy(ch->player_specials->primalsaved.host, ch->desc->host, HOST_LENGTH);
  ch->player_specials->primalsaved.host[HOST_LENGTH] = '\0';
  ch->player_specials->primalsaved.last_logon = time(0);
  if (st != NULL) {
    strncpy(st->player_specials_primalsaved.host, ch->desc->host, HOST_LENGTH);
    st->player_specials_primalsaved.last_logon = time(0); 
    st->player_specials_primalsaved.host[HOST_LENGTH] = '\0';
  }
}


/*
 * write the vital data of a player to the player file
 *
 * NOTE: load_room should be an *RNUM* now.  It is converted to a vnum here.
 */
void save_char(struct char_data * ch, room_rnum load_room)
{
  struct char_file_u st, temp;
  struct char_data *i;

  if (IS_NPC(ch) || !ch->desc || GET_PFILEPOS(ch) < 0)
    return;

  char_to_store(ch, &st);

  // DM: if we have bad passwords, then saving due to an unsuccessful login
  if ((ch->desc->bad_pws) > 0) {
    set_unsuccessful_logon(ch, ch->desc, &st);
    // At this point, THIS character is saved, but if the character
    // is already playing, the playing data will overwrite these details next
    // save, hence we need to set unsuccessful details in the playing character
    // and save.
    for (i = character_list; i; i = i->next ) 
      if (ch != i && !IS_NPC(i) && (GET_IDNUM(ch) == GET_IDNUM(i))) {
        char_to_store(i, &temp);
        set_unsuccessful_logon(i, ch->desc, &temp);
        save_char(i, NOWHERE);
      }
  } else {
    set_successful_logon(ch, &st);
  }


/*
  if (!PLR_FLAGGED(ch, PLR_LOADROOM)) {
    if (load_room == NOWHERE)
      st.player_specials_saved.load_room = NOWHERE;
    else
      st.player_specials_saved.load_room = GET_ROOM_VNUM(load_room);
  }
  */
  fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
  fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
  fflush(player_fl);
  save_char_vars(ch);
}



/* copy data from the file structure to a char struct */
void store_to_char(struct char_file_u * st, struct char_data * ch)
{
  int i;

  /* to save memory, only PC's -- not MOB's -- have player_specials */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);
  
  GET_SEX(ch) = st->sex;
  GET_CLASS(ch) = st->chclass;
  GET_LEVEL(ch) = st->level;

  // For DG scripts
  GET_ID(ch) = GET_IDNUM(ch); 

  // Now Calculate the PC's exp modifier based on their class and their specials
  calc_modifier(ch);

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
  ch->player_specials->primalsaved = st->player_specials_primalsaved;
  ch->player_specials->player_kills = st->player_kill_data;
  ch->player_specials->last_colour_code = 0;
  ch->player_specials->prev_colour_code = 0;
  ch->player_specials->mark_colour_code = 0;

  GET_LAST_TELL(ch) = NOBODY;

  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;

  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->char_specials.small_bits = 0;
  ch->points.armor = 100;
  ch->points.hitroll = 0;
  ch->points.damroll = 0;

  if (ch->player.name == NULL)
    CREATE(ch->player.name, char, strlen(st->name) + 1);
  strcpy(ch->player.name, st->name);
  strcpy(ch->player.passwd, st->pwd);

  /* Personal details area */
  // Artus> Switched strcpy for strncpy. Create is now creating strlen+1.
  CREATE(GET_EMAIL(ch), char, strlen(st->email) + 1);
  strncpy(GET_EMAIL(ch), st->email, strlen(st->email));
  GET_EMAIL(ch)[strlen(st->email)] = '\0';
  CREATE(GET_WEBPAGE(ch), char, strlen(st->webpage) + 1);
  strncpy(GET_WEBPAGE(ch), st->webpage, strlen(st->webpage));
  GET_WEBPAGE(ch)[strlen(st->webpage)] = '\0';
  CREATE(GET_PERSONAL(ch), char, strlen(st->personal) + 1);
  strncpy(GET_PERSONAL(ch), st->personal, strlen(st->personal));
  GET_PERSONAL(ch)[strlen(st->personal)] = '\0';

  POOFIN(ch) = NULL;
  POOFOUT(ch) = NULL;

  if (strlen(st->poofin) > 0) {
    CREATE(POOFIN(ch), char, strlen(st->poofin));
    strncpy(POOFIN(ch), st->poofin, strlen(st->poofin));
  }

  if (strlen(st->poofout) > 0) {
    CREATE(POOFOUT(ch), char, strlen(st->poofout));
    strncpy(POOFOUT(ch), st->poofout, strlen(st->poofout));
  }

  GET_PROMPT(ch) = NULL;
  if (strlen(st->prompt_string) > 0) {
    CREATE(GET_PROMPT(ch), char, strlen(st->prompt_string));
    strncpy(GET_PROMPT(ch), st->prompt_string, strlen(st->prompt_string));
  }

  /* Add all spell effects */
  for (i = 0; i < MAX_AFFECT; i++)
    if (st->affected[i].type)
      affect_to_char(ch, &st->affected[i]);

  /* Add all timer effects */
  for (i = 0; i < MAX_TIMERS; i++)
    if (st->timers[i].type)
      timer_to_char(ch, &st->timers[i]);

  /*
   * If you're not poisioned and you've been away for more than an hour of
   * real time, we'll set your HMV back to full
   */

  if (!AFF_FLAGGED(ch, AFF_POISON) &&
      (((long) (time(0) - ch->player_specials->primalsaved.last_logon)) 
       >= SECS_PER_REAL_HOUR))
  {
    GET_HIT(ch) = MAX(GET_HIT(ch), GET_MAX_HIT(ch));
    GET_MOVE(ch) = MAX(GET_HIT(ch), GET_MAX_MOVE(ch));
    GET_MANA(ch) = MAX(GET_HIT(ch), GET_MAX_MANA(ch));
  }
}				/* store_to_char */




/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data * ch, struct char_file_u * st)
{
  int i;
  struct affected_type *af;
  struct timer_type *timer;
  struct obj_data *char_eq[NUM_WEARS];

  /* Unaffect everything a character can be affected by */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      char_eq[i] = unequip_char(ch, i, FALSE);
#ifndef NO_EXTRANEOUS_TRIGGERS
      remove_otrigger(char_eq[i], ch);
#endif
    } else
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

  for (timer = ch->timers, i = 0; i < MAX_TIMERS; i++) 
  {
    if ((timer) && (timer->duration != 0))
    {
      st->timers[i] = *timer;
      st->timers[i].next = 0;
      timer = timer->next;
    } else {
      st->timers[i].type = 0;	/* Zero signifies not used */
      st->timers[i].duration = 0;
      st->timers[i].uses = 0;
      st->timers[i].max_uses = 0;
      st->timers[i].next = 0;
    }
  }

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected)
    affect_remove(ch, ch->affected, 0);

  while (CHAR_TIMERS(ch))
    timer_remove_char(ch, CHAR_TIMERS(ch));

  if ((i >= MAX_AFFECT) && af && af->next)
    basic_mud_log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  st->birth = ch->player.time.birth;
  st->played = ch->player.time.played;
  st->played += (long) (time(0) - ch->player.time.logon);

  // DM: if we have bad passwords, then saving due to an unsuccessful login
  //if (GET_BAD_PWS(ch) > 0) {
  //  st->last_logon = ch->player_specials->last_logon;
  //  st->lastUnsuccessfulLogon = time(0); 
  //} else {
  //  st->last_logon = time(0);
  //  st->lastUnsuccessfulLogon = ch->player_specials->lastUnsuccessfulLogon;
  //}

  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  st->hometown = ch->player.hometown;
  st->weight = GET_WEIGHT(ch);
  st->height = GET_HEIGHT(ch);
  st->sex = GET_SEX(ch);
  st->chclass = GET_CLASS(ch);
  st->level = GET_LEVEL(ch);
  st->abilities = ch->real_abils;
  st->points = ch->points;
  st->char_specials_saved = ch->char_specials.saved;
  st->player_specials_saved = ch->player_specials->saved;
  st->player_specials_primalsaved = ch->player_specials->primalsaved;
  st->player_kill_data = ch->player_specials->player_kills;

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
  strcpy(st->pwd, GET_PASSWD(ch));

  /* Personal details */
  if (GET_EMAIL(ch))
    strcpy(st->email, GET_EMAIL(ch));
  else
    strcpy(st->email, "None");

  if (GET_WEBPAGE(ch))
    strcpy(st->webpage, GET_WEBPAGE(ch));
  else
    strcpy(st->webpage, "None");

  if (GET_PERSONAL(ch))
    strcpy(st->personal, GET_PERSONAL(ch));
  else
    strcpy(st->personal, "None");

  if (POOFIN(ch))
    strcpy(st->poofin, POOFIN(ch));
  else
    st->poofin[0] = '\0';
    //strcpy(st->poofout, "appears in a puff of smoke.");

  if (POOFOUT(ch))
    strcpy(st->poofout, POOFOUT(ch));
  else
    st->poofout[0] = '\0';
    //strcpy(st->poofout, "disappears with a sudden bang.");

  if (GET_PROMPT(ch)) {
    strcpy(st->prompt_string, GET_PROMPT(ch));
  } else {
    st->prompt_string[0] = '\0';
  }

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type) {
      affect_to_char(ch, &st->affected[i]);
    }
  }

  /* add timer effects back in now */
  for (i = 0; i < MAX_TIMERS; i++) {
    if (st->timers[i].type) {
      timer_to_char(ch, &st->timers[i]);
    }
  }

  for (i = 0; i < NUM_WEARS; i++) 
  {
    if (char_eq[i]) 
    {
      equip_char(ch, char_eq[i], i, FALSE);
#if 0 // Artus> This is called by save_char.. No a good place for wear trigs.
#ifndef NO_EXTRANEOUS_TRIGGERS
      if (wear_otrigger(char_eq[i], ch, i))
#endif
      equip_char(ch, char_eq[i], i, FALSE);
#ifndef NO_EXTRANEOUS_TRIGGERS
    else
      obj_to_char(char_eq[i], ch, __FILE__, __LINE__);
#endif
#endif
    }
  }

/*   affect_total(ch); unnecessary, I think !?! */
}				/* Char to store */



void save_etext(struct char_data * ch)
{
/* this will be really cool soon */

}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(char *name)
{
  int i, pos;

  if (top_of_p_table == -1) {	/* no table */
    CREATE(player_table, struct player_index_element, 1);
    pos = top_of_p_table = 0;
  } else if ((pos = get_ptable_by_name(name)) == -1) {	/* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  // dm - changed - why convert to lower? it be stupid when comparing not to
  // convert it anyhow - ignore uses get_name_by_id - I dont want lowercase
  // version... (changed get_ptable_by_name to use str_cmp) 
  for (i = 0; (player_table[pos].name[i] = /*LOWER(*/name[i])/*)*/; i++)
	/* Nothing */;

  return (pos);
}



/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      basic_mud_log("SYSERR: fread_string: format error at or near %s", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
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
      basic_mud_log("SYSERR: fread_string: string too large (db.c)");
      basic_mud_log(error);
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

  return (rslt);
}


/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
{
  int i;
  struct alias_data *a;

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob)
  {
    while ((a = GET_ALIASES(ch)) != NULL)
    {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }

    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    free(ch->player_specials);
    if (IS_NPC(ch))
      basic_mud_log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
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
    // Artus> Some missing stuff.
    if (ch->player.email)
      free(ch->player.email);
    if (ch->player.webpage)
      free(ch->player.webpage);
    if (ch->player.personal)
      free(ch->player.personal);
    if (ch->player.prompt_string)
      free(ch->player.prompt_string);
  } else if ((i = GET_MOB_RNUM(ch)) >= 0) {
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

  while (CHAR_TIMERS(ch))
    timer_remove_char(ch, CHAR_TIMERS(ch));
  
  if (ch->desc)
    ch->desc->character = NULL;

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  if (GET_OBJ_RNUM(obj) == NOWHERE)
    free_object_strings(obj);
  else
    free_object_strings_proto(obj);  

  while (OBJ_TIMERS(obj)) {
    timer_remove_obj(obj,OBJ_TIMERS(obj));
  }

  free(obj);
}


/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
int file_to_string_alloc(const char *name, char **buf)
{
  char temp[MAX_STRING_LENGTH];
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
    if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
      return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  if (*buf)
    free(*buf);

  *buf = str_dup(temp);
  return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE+3];

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    basic_mud_log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }
  do {
    fgets(tmp, READ_SIZE, fl);
    tmp[strlen(tmp) - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");

    if (!feof(fl)) {
      if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
        basic_mud_log("SYSERR: %s: string too big (%d max)", name,
		MAX_STRING_LENGTH);
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
    GET_EQ(ch, i) = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  ch->in_room = NOWHERE;
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  AUTOASSIST(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->char_specials.small_bits = 0;

  ch->mob_specials.default_pos = POS_STANDING;

  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;

  GET_LAST_TELL(ch) = NOBODY;

  // Artus> Remove any nocturnal affects.
  if (PRF_FLAGGED(ch, PRF_WOLF | PRF_VAMPIRE) && 
      ((weather_info.sunlight != SUN_DARK) ||
       (weather_info.moon == MOON_NONE)))
    affect_from_char(ch, SPELL_CHANGED);
    
    
  // Set any class abilities
  apply_specials(ch, FALSE);
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  ch->in_room = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
//  CHAR_TIMERS(ch) = NULL;

  GET_AC(ch) = 100;		/* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
}


void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
  obj->worn_on = NOWHERE;
  obj->proto_script = NULL;
  SCRIPT(obj) = NULL;
  OBJ_TIMERS(obj) = NULL;
}


void init_char(struct char_data * ch)
{
  int i;

  ch->points.max_mana = 100;
  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.hit = GET_MAX_HIT(ch);
  ch->points.max_move = 82;
  ch->points.move = GET_MAX_MOVE(ch);
  ch->points.armor = 100;

  GET_REM_ONE(ch) = 0;
  GET_REM_TWO(ch) = 0;
  GET_IMMKILLS(ch) = 0;
  GET_MOBKILLS(ch) = 0;
  GET_PCKILLS(ch) = 0;
  GET_KILLSBYIMM(ch) = 0;
  GET_KILLSBYMOB(ch) = 0;
  GET_KILLSBYPC(ch) = 0;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */

  if (top_of_p_table == 0) {
    GET_LEVEL(ch) = LVL_OWNER;

    ch->points.max_hit = 5000;
    ch->points.max_mana = 5000;
    ch->points.max_move = 5000;
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


  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
    player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
  else
    basic_mud_log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));

  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
  }

  ch->char_specials.saved.affected_by = 0;

  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

  ch->real_abils.intel = 10;
  ch->real_abils.wis = 10;
  ch->real_abils.dex = 10;
  ch->real_abils.str = 10;
  ch->real_abils.str_add = 0;
  ch->real_abils.con = 10;
  ch->real_abils.cha = 10;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_OWNER ? -1 : 24);

  for (i = 0; i < NUM_WORLDS; i++) {
    ENTRY_ROOM(ch, i) = NOWHERE;
  }
  START_WORLD(ch) = WORLD_MEDIEVAL;
  //GET_LOADROOM(ch) = NOWHERE;

  GET_EMAIL(ch) = NULL;
  GET_WEBPAGE(ch) = NULL;
  GET_PERSONAL(ch) = NULL;

  GET_CLAN(ch) = 0;
  GET_CLAN_RANK(ch) = 0;
  GET_EXP(ch) = 0;

  POOFIN(ch) = NULL;
  POOFOUT(ch) = NULL;

  GET_PROMPT(ch) = NULL;
  
  GET_FIGHT_PROMPT(ch) = PROMPT_HEALTHOMETER;

  GET_PAGE_WIDTH(ch) = 80;
  GET_PAGE_LENGTH(ch) = 22;

  for (i=0; i < 10; i++)
    set_default_colour(ch,i);

  for (i=0; i < MAX_IGNORE; i++) {
    GET_IGNORE(ch, i) = 0;
    GET_IGNORE_ALL(ch, i) = 1;
  }

  GET_IGN_LVL(ch) = 0;
  
  GET_OLC_ZONE(ch) = -1;

  GET_STAT_POINTS(ch) = 1;

  GET_SPECIALS(ch) = 0;

  GET_SOCIAL_STATUS(ch) = SOCIAL_CITIZEN;

  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1);
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_2);
  REMOVE_BIT(PRF_FLAGS(ch), PRF_MORTALK);
}


/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum) {
  room_rnum bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((world + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((world + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}



/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(struct obj_data *obj)
{
  int error = FALSE;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has negative weight (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

  sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf);
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has unknown wear flags.",
	GET_OBJ_VNUM(obj), obj->short_description);

  sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has unknown extra flags.",
	GET_OBJ_VNUM(obj), obj->short_description);

//  sprintbit(GET_OBJ_LR(obj), level_bits, buf);
//  if (strstr(buf, "UNDEFINED") && (error = TRUE))
//    basic_mud_log("SYSERR: Object #%d (%s) has unknown level flags.",
//	GET_OBJ_VNUM(obj), obj->short_description);

  sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
  if (strstr(buf, "UNDEFINED") && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has unknown affection flags.",
	GET_OBJ_VNUM(obj), obj->short_description);

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
  {
    char onealias[MAX_INPUT_LENGTH], *space = strchr(obj->name, ' ');
    int offset = space ? space - obj->name : strlen(obj->name);

    strncpy(onealias, obj->name, offset);
    onealias[offset] = '\0';

/*   -- Buggered this check off, it's a joke! -- 
    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
      basic_mud_log("SYSERR: Object #%d (%s) doesn't have drink type as first alias. (%s)",
		GET_OBJ_VNUM(obj), obj->short_description, obj->name);
 */
  }
  /* Fall through. */
  case ITEM_FOUNTAIN:
    if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
      basic_mud_log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      basic_mud_log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
 }

  return (error);
}

int check_object_spell_number(struct obj_data *obj, int val)
{
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1 || GET_OBJ_VAL(obj, val) == 0)	/* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
    error = TRUE;
  if (IS_SKILL(GET_OBJ_VAL(obj, val)))
    error = TRUE;
  if (error)
    basic_mud_log("SYSERR: Object #%d (%s) has out of range spell #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    basic_mud_log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck)		/* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = skill_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) uses '%s' spell #%d. val %d",
		GET_OBJ_VNUM(obj), obj->short_description, spellname,
		GET_OBJ_VAL(obj, val),val);

  return (error);
}

int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has out of range level #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  return (error);
}


/* Bye-bye, replaced with init_clans() -- ARTUS
void clan_boot ( void )
{
  FILE *cfp ;
  char buf[MAX_LEN];
  int i=0, j=0;
 
//  strcpy (clan_info[0].name, "none" ) ;
 
  if (!(cfp = fopen( CLAN_FILE, "r"))) {
    sprintf(buf, "Error reading %s", CLAN_FILE);
    perror(buf);
    return;
  }
 
  while (buf[0] != FILE_TERM) {
    fgets(buf, MAX_LEN, cfp);
    strcpy(clan_info[i].name, buf);
    clan_info[i].name[strlen(clan_info[i].name) - 1] = '\0';
 
    fgets(buf, MAX_LEN, cfp);
    strcpy(clan_info[i].disp_name, buf);
    clan_info[i].disp_name[strlen(clan_info[i].disp_name) - 1] = '\0';
 
    sprintf(buf, "Loading clan: %s.", clan_info[i].disp_name);
    basic_mud_log(buf);
 
    for(j=0; j < NUM_CLAN_RANKS; j++) {
      fgets(buf, MAX_LEN, cfp);
      strcpy(clan_info[i].ranks[j], buf);
      clan_info[i].ranks[j][strlen(clan_info[i].ranks[j]) - 1] = '\0';
    }
 
    fgets(buf, MAX_LEN, cfp);
    i++ ;      
  }
 
  / * Set clan_tables array to NULL * /
  for (i=0; i < NUM_CLANS; i++)
    clan_tables[i][0]='\0';
 
  fclose(cfp);
}
*/

/* Bye bye -- ARTUS 
void copy_clan_tables(void)
{
  int i;
 
  for (i=0; i < NUM_CLANS; i++)
    clan_info[i].table = str_dup(clan_tables[i]);
 
}
*/

/* ARTUS ... */
void save_char_file_u(struct char_file_u st)
{
  int player_i;
  int find_name(char *name);
  if ((player_i = find_name(st.name)) >= 0) {
    fseek(player_fl, player_i * sizeof(struct char_file_u), SEEK_SET);
    fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
    fflush(player_fl);
  }
}
