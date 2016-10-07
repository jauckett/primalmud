/**************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "colour.h"
#include "clan.h" // New clan stuff - ARTUS
#include "dg_scripts.h"
#include "quest.h"

// Artus> Err.. ^= anyone? :o)
// #define SETREMOVE(flag, bit) if (IS_SET((flag), (bit))) REMOVE_BIT((flag), (bit)); else SET_BIT((flag),(bit))
#define SETREMOVE(flag, bit) (flag ^= bit)
// Make this room a bounty collection office or something more intersting
#define BOUNTY_RETURN_ROOM 	1115


/* extern variables */
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct user_data *user_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern long NUM_PLAYERS;
extern long top_idnum;
// extern struct clan_data clan_info[NUM_CLANS]; -- ARTUS: Old clan stuff.
extern ReleaseInfo release;

extern char *credits;
extern char *areas;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *hint_table[];
extern  struct index_data *mob_index;
extern struct event_list events;
extern struct zone_data *zone_table;
extern Burglary *burglaries;
extern struct index_data *obj_index;
extern const char *social_ranks[];
extern const char *quest_names[];
extern struct char_data *mob_proto;

extern int num_hints;


/* extern functions */
ACMD(do_track);
ACMD(do_action);
long find_class_bitvector(char arg);
int level_exp(struct char_data *ch, int level);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(struct char_data *ch, bool divide);
int is_colour(struct char_data *ch, char code, bool colour_code_only);
// int compute_thaco(struct char_data *ch); // Artus - Not Used ?
void add_mud_event(struct event_data *ev);
void remove_mud_event(struct event_data *ev);
void send_to_zone(const char *, zone_rnum);
void send_to_not_zone_world(const char *, zone_rnum);
sh_int find_clan_by_sid(char *test);
bool attach_rooms(long lFirst, long lSecond);
char *rand_desc(int nAreaType);
char *rand_name(int nAreaType);
void handle_quest_event(struct event_data *ev);

/* local functions */
void do_hint(void);
void do_zone_hint(void);
void store_mail(long, long, char *);
struct char_data *get_char_online(struct char_data *ch, char *name, int where);
void print_object_location(int num, struct obj_data * obj, struct char_data * ch, int recur, char *writeto);
void show_obj_to_char(struct obj_data * object, struct char_data * ch, int mode);
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode, int show);
void list_obj_to_char2(struct obj_data * list, struct char_data * ch, int mode, int show);
int invalid_level(struct char_data *ch, struct obj_data *object, bool display);
int reverse_dir(int dir);
struct char_data *get_player_by_id(long id);
void remove_burglary(Burglary *target);
long is_room_burgled(int nRoom);
int get_burgle_area(room_rnum nRoom);
void apply_burglar_stats_to_mob(struct char_data *mob, long lBurglarID, float fStrength);

ACMD(do_bounties);
ACMD(do_sense);
ACMD(do_search);
ACMD(do_immlist);
ACMD(do_info);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_setcolour);
ACMD(do_listen);
void perform_mortal_where(struct char_data * ch, char *arg);
void perform_immort_where(struct char_data * ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
ACMD(do_show_hint);
void sort_commands(void);
ACMD(do_commands);
void diag_char_to_char(struct char_data * i, struct char_data * ch);
void look_at_char(struct char_data * i, struct char_data * ch);
void list_one_char(struct char_data * i, struct char_data * ch);
void list_char_to_char(struct char_data * list, struct char_data * ch);
void do_auto_exits(struct char_data * ch);
ACMD(do_events);
ACMD(do_exits);
void look_in_direction(struct char_data * ch, int dir);
void look_in_obj(struct char_data * ch, char *arg);
void show_contents_to(struct char_data *ch, struct obj_data *obj);
char *find_exdesc(char *word, struct extra_descr_data * list);
void look_at_target(struct char_data *ch, char *arg, 
                    struct char_data **tch=NULL, struct obj_data **tobj=NULL);
/* void display_clan_table(struct char_data *ch, struct clan_data *clan);
 * -- ARTUS: Old clan stuff. */
void toggle_display(struct char_data *ch, struct char_data *vict);  
void ch_trigger_trap(struct char_data *ch, struct obj_data *obj);

int compareAffectDuration(const void *x, const void *y);
int compareCharName(const void *x, const void *y);
int compareCharLevelA(const void *x, const void *y);
int compareCharLevelD(const void *x, const void *y);
void print_who_info(struct char_data *array[], int length, 
                    struct char_data *ch);
void show_ability_messages_to_char(struct char_data *ch);
long get_burgle_room_type(long lArea);
bool room_has_exit_to(int nFirst, int nSecond);

/* Burglary class and class types */
void Burglary::DescribeSelf(struct char_data *ch)
{
  char out[MAX_STRING_LENGTH];
  sprintf(out, "   %s is burgling %d rooms.\r\n",
	  get_name_by_id(chID), CountBurgledRooms());
  send_to_char(out, ch);
}

int Burglary::CountBurgledRooms()
{
  int nCount = 0;
  for (int i = 0; i < MAX_BURGLED_ROOMS; i++)
    if (burgledRooms[i].rNum != -1)
      nCount++;
  return nCount;
}

// Links the rooms up, and populates with goodies and baddies, 
// traps and hidden items etc.
int Burglary::Initialise(int nStart, int nDir)
{
  int nCount = CountBurgledRooms(), nRealCount = 0;;
  
  if (nCount <= 0)
	return -1;

  // One way link to the start room from the direction specified
  world[burgledRooms[0].rNum].dir_option[nDir]->to_room = nStart;
  world[burgledRooms[0].rNum].dir_option[nDir]->exit_info = EX_ISDOOR;
  world[burgledRooms[0].rNum].dir_option[nDir]->keyword = "exit";

  // Create the area (attach rooms)
  for (int i = 0; i < nCount; i++)
  {
    int nRoom = burgledRooms[number(0, nCount -1)].rNum;

    // Want to limit the amount of real rotations, there's a point
    // where the attachment is obviously just not going to work
    if (nRealCount > 10 * MAX_BURGLED_ROOMS)
    {
	// Shows up as just another case where player has failed
	mudlog("Breaking out of Burglary::Initialise - Unable to attach rooms properly.",
		NRM, LVL_GOD, TRUE);
	Clear();
	return NOWHERE;
    }

    // Check if we're trying to connect to ourselves
    if (nRoom == burgledRooms[i].rNum)
    {
	i--;
        nRealCount++;
	continue;
    }

    // Check if the two rooms are already attached
    if (room_has_exit_to(nRoom, burgledRooms[i].rNum))
    {
	i--; 
	nRealCount++;
	continue;
    }

    // Give this room a description - TODO, make this a member function
    int nTargetRoom = burgledRooms[i].rNum;
    long lArea = burgledRooms[i].lArea;

    world[nTargetRoom].name = rand_name(lArea);
    world[nTargetRoom].description =  rand_desc(lArea);
    SET_BIT(world[nTargetRoom].burgle_flags, get_burgle_room_type(lArea));

    if (!attach_rooms(GET_ROOM_VNUM(burgledRooms[i].rNum), GET_ROOM_VNUM(nRoom)))
	i--;
    nRealCount++;
  }

  initialArea = get_burgle_area(nStart);

  InitialiseMobs();
  InitialiseObjects();
  InitialiseTraps();
  InitialiseSpecials();

  return burgledRooms[0].rNum;
}

#define BURGLE_WAREHOUSE_BOSS		30902	// foreman
#define BURGLE_WAREHOUSE_SUB		30901	// worker
#define BURGLE_WAREHOUSE_UNDERLING	30903	// gofer

#define BURGLE_HOME_BOSS		0
#define BURGLE_HOME_SUB			0
#define BURGLE_HOME_UNDERLING		0

#define BURGLE_SHOP_BOSS		30904
#define BURGLE_SHOP_SUB			0
#define BURGLE_SHOP_UNDERLING		0

void Burglary::InitialiseMobs() 
{ 
  long lMob1=0,	// Boss
       lMob2=0,   // Sub
       lMob3=0;   // Underling

  if (initialArea == ROOM_AREA_WAREHOUSE_RICH || 
      initialArea == ROOM_AREA_WAREHOUSE_REGULAR || 
      initialArea == ROOM_AREA_WAREHOUSE_POOR)
  {
    lMob1 = BURGLE_WAREHOUSE_BOSS;
    lMob2 = BURGLE_WAREHOUSE_SUB;
    lMob3 = BURGLE_WAREHOUSE_UNDERLING;
  }
  else
  if (initialArea == ROOM_AREA_HOME_RICH ||
      initialArea == ROOM_AREA_HOME_REGULAR ||
      initialArea == ROOM_AREA_HOME_POOR)
  {
    lMob1 = BURGLE_HOME_BOSS;
    lMob2 = BURGLE_HOME_SUB;
    lMob3 = BURGLE_HOME_UNDERLING;
  }
  else
  if (initialArea == ROOM_AREA_SHOP_RICH ||
      initialArea == ROOM_AREA_SHOP_REGULAR ||
      initialArea == ROOM_AREA_SHOP_POOR)
  {
    lMob1 = BURGLE_SHOP_BOSS;
    lMob2 = BURGLE_SHOP_SUB;
    lMob3 = BURGLE_SHOP_UNDERLING;
  }

  // We have the mobs, now load some of them into the rooms, depending
  // on the time of day, basically, no mobs for warehouses after midnight,
  // same for shops. Shops should have other protections (items, traps, specials)
  if (lMob1 == BURGLE_WAREHOUSE_BOSS && time_info.hours >= 0 && time_info.hours < 6)
	return; // No mobs

  // For every room, some chance of either
  int nCount = CountBurgledRooms();
  for (int i = 0; i < nCount; i++)
  {
    int nRoom = burgledRooms[i].rNum;

    // Boss
    if (number(1, MAX_BURGLED_ROOMS) == 1)
    {
	struct char_data *m = read_mobile(lMob1, VIRTUAL);
        if (m != NULL)
        {
	  apply_burglar_stats_to_mob(m, chID, 1.2);
	  char_to_room(m, nRoom);
	}
	else
	  basic_mud_log("Boss Mobile #%ld cannot be burgle loaded.", lMob1);
    }

    // Subs
    for (int j = 0; j < 2; j++)
    {
      if (number(1, int((MAX_BURGLED_ROOMS * 2)/3)) == 1 && lMob2 != 0)
      {
    	struct char_data *m = read_mobile(lMob2, VIRTUAL);
	if (m != NULL)
	{
	  apply_burglar_stats_to_mob(m, chID, 1.0);
	  char_to_room(m, nRoom);
	}
	else
	  basic_mud_log("Sub Mobile #%ld cannot be burgle loaded.", lMob2);
      }
    }

    // Underlings
    for (int k = 0; k < 5; k++)
    {
      if (number(1, int(MAX_BURGLED_ROOMS/2)) == 1 && lMob3 != 0)
      {
  	struct char_data *m = read_mobile(lMob3, VIRTUAL);
	if (m != NULL)
	{
	  apply_burglar_stats_to_mob(m, chID, 0.95);
	  char_to_room(m, nRoom);
	}
	else
	  basic_mud_log("Underling Mobile #%ld cannot be burgle loaded.", lMob3);
      }
    }

  }
}

#define BURGLE_LOOT	30900
#define BURGLE_UNIQUE   0

// All objects are hidden, some are traps
void Burglary::InitialiseObjects() 
{ 
  int nCount = CountBurgledRooms();
  int nItem = 0;
  struct char_data *ch = get_player_by_id(chID);

  for (int i = 0; i < nCount; i++)
  {
    int nRoom = burgledRooms[i].rNum;
    struct obj_data *obj = NULL;

    // Some chance of loot
    if (number(1, nCount) == 1)
    {
	obj = read_object(BURGLE_LOOT, VIRTUAL);
	int nRange1, nRange2;
	nRange1 = int(float(GET_LEVEL(ch) * 10) * GET_MODIFIER(ch));
	nRange2 = int(float(GET_LEVEL(ch) * 50) * GET_MODIFIER(ch));

	GET_OBJ_COST(obj) = number(nRange1, nRange2);
	// Hide the object
        SET_BIT(GET_OBJ_EXTRA(obj), ITEM_HIDDEN);
	obj_to_room(obj, nRoom);
    }
    
    obj = NULL;
    // Some chance of rare object, must have 5 more at least existing and
    // they must be sequential after BURGLE_LOOT item (ie 901, 902 etc)
    // - Object will also depend on burglar's level
    if ((nItem = number(1, MAX_BURGLED_ROOMS * 100) <= 5)) // Very small chance
    {
      long lItem = 0;
      int nChance = 0;
      if (GET_LEVEL(ch) > 80)
	nChance = 5;			// Any up to and including ethereal blade
      else if (GET_LEVEL(ch) > 60)
	nChance = 4;			// Any up to and including golden shortsword
      else if (GET_LEVEL(ch) > 40)
	nChance = 3;			// Any up to and including thin rapier
      else if (GET_LEVEL(ch) > 20)
	nChance = 2;			// Any up to and including jagged scimitar
      else
	nChance = 1;			// Viscious dirk

      lItem = BURGLE_LOOT + number(1, nChance);	// Allocate Obj VNUM 
      obj = read_object(lItem, VIRTUAL);
      if (obj != NULL)
      {
        SET_BIT(GET_OBJ_EXTRA(obj), ITEM_HIDDEN);
        obj_to_room(obj, nRoom); 
      }
    }
    
    obj = NULL;
    // Little chance of unique object
    if (number(1, MAX_BURGLED_ROOMS * 1000) == 1 && BURGLE_UNIQUE != 0)
    {
      obj = read_object(BURGLE_UNIQUE, VIRTUAL);
      obj_to_room(obj, nRoom);
    }
  }
}
/* Looks through all the items in the rooms assigned, and 
 * can potentially trap them.
 */
void Burglary::InitialiseTraps() 
{ 
  int nCount = CountBurgledRooms();
  int chance = 0;
  struct char_data *burglar = get_player_by_id(chID);

  for (int i = 0; i < nCount; i++)
  {
    int nRoom = burgledRooms[i].rNum;
    struct obj_data *oNext = NULL;
    long lBurgleFlag = get_burgle_room_type(burgledRooms[i].lArea);
    for(struct obj_data *obj = world[nRoom].contents; obj; obj = oNext)
    {
      oNext = obj->next;
      if (lBurgleFlag == ROOM_SHOP)
        chance = 1; // Every item is a trap
      else if (lBurgleFlag == ROOM_HOME)
	chance = 2; // Every second
      else
	chance = 3;

      if (number(1, chance) != 1)
      {
	continue;
      }

      long lObjId = GET_OBJ_VNUM(obj);
      if (lObjId == BURGLE_LOOT)	// Loot isn't trapped
      {
	continue;
      }

      // Create a trap, set it to have the old objects vnum after 
      // it's triggered
      GET_OBJ_TYPE(obj) = ITEM_TRAP;
      GET_OBJ_VAL(obj, 0) = 1;	// Set to hurt person who sets it off only
      GET_OBJ_VAL(obj, 1) = 10;  // 10  size dice
      GET_OBJ_VAL(obj, 2) = GET_LEVEL(burglar); // Level damage dice
      GET_OBJ_VAL(obj, 3) = lObjId;	// Original item to load in place after trigger
    }
  }
}

/* TODO: Make some specials - depends on active quests etc */
void Burglary::InitialiseSpecials() { }


// Clears all objects, mobs, and sets all exits to NOWHERE
void Burglary::Clear()
{
  int nCount = CountBurgledRooms();
  
  for(int i = 0; i < nCount; i++)
  {
    int nRoom = burgledRooms[i].rNum;

    // Clear out the objects
    struct obj_data *objs;
    for (objs = world[nRoom].contents; objs; objs = world[nRoom].contents)
	extract_obj(objs);

    // Clear out the mobs
    struct char_data *mobs;
    for (mobs = world[nRoom].people; mobs; mobs = world[nRoom].people)
    {
      if (IS_NPC(mobs))
        extract_char(mobs);
      else
      {
        send_to_char("You are kicked out of the crime scene.\r\n", mobs);
        char_to_room(mobs, 1115); 
      }
    }

    // Reset the exits
    for (int j = 0; j < 4; j++)
    {
      world[nRoom].dir_option[j]->to_room = NOWHERE;
      world[nRoom].dir_option[j]->keyword = "";
      world[nRoom].dir_option[j]->exit_info = 0;      
    }

    // Reset the burgled flags
    BURGLE_FLAGS(nRoom) = 0;

    // Remove this room from the burgledRooms
    burgledRooms[i].rNum = -1;
    burgledRooms[i].lArea = 0;   
  }
}
// end burglary  and types 

void do_zone_hint(void)
{
  struct descriptor_data *d;
  int rnumb;
  zone_rnum zrnum;

  for (d = descriptor_list; d; d = d->next)
  {
    if (((STATE(d) == CON_PLAYING) && d->character) &&
        (!EXT_FLAGGED(d->character, EXT_NOHINTS)))
    {
      zrnum = world[d->character->in_room].zone;
      if (zone_table[zrnum].nohnts > 0)
      {
	rnumb = number(0, zone_table[zrnum].nohnts-1);
	sprintf(buf, "&W[ &bZone: &B%s &W]&n\r\n",
	        zone_table[zrnum].hints[rnumb]);
	send_to_char(buf,d->character); 
      }
    }
  } 
}

// Display a random hint stored in hint_table - called every PULSE_HINTS 
void do_hint(void)
{
  struct descriptor_data *d;
  int rnumb;

  rnumb = number(0, num_hints-1);
  sprintf(buf, "&W[ &B%s &W]&n\r\n", hint_table[rnumb]);
  for (d = descriptor_list; d; d = d->next)
  {
    if (((STATE(d) == CON_PLAYING) && d->character) &&
        (!EXT_FLAGGED(d->character, EXT_NOHINTS)))
      send_to_char(buf,d->character); 
  } 
}

ACMD(do_show_hint)
{
  int rnumb;
  char arg[MAX_INPUT_LENGTH];

  half_chop(argument, arg, argument);

  if (*arg)
  {
    if (!str_cmp(arg, "ON"))
    {
      send_to_char("You will now see the hint channel.\r\n", ch);
      REMOVE_BIT(EXT_FLAGS(ch), EXT_NOHINTS);
      save_char(ch, NOWHERE);
      return;
    } else if (!str_cmp(arg, "OFF")) {
      send_to_char("You will no longer see the hint channel.\r\n", ch);
      SET_BIT(EXT_FLAGS(ch), EXT_NOHINTS);
      save_char(ch, NOWHERE);
      return;
    }
  }
  rnumb = number(0, num_hints-1);
  sprintf(buf, "&W[ &B%s &W]&n\r\n", hint_table[rnumb]);
  send_to_char(buf, ch);
}

void mortal_detectcurses(struct char_data *ch)
{
  struct obj_data *obj;
  int curseCounter = 0;

  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    if (IS_OBJ_STAT(obj, ITEM_NODROP) && (number(1, 101) < GET_SKILL(ch, SKILL_SENSE_CURSE)))
    {
      curseCounter++;
      act("You sense that $p is cursed.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
      for (struct obj_data *inner = obj->contains; inner;
	   inner = inner->next_content)
	if ((IS_OBJ_STAT(inner, ITEM_NODROP) && 
	    (number (1, 101) < GET_SKILL(ch, SKILL_SENSE_CURSE))))
	{
	  curseCounter++;
	  act("You sense that $p contains cursed items.", FALSE, ch, obj, 0, TO_CHAR);
	  break;
	}
  }	
  if (!curseCounter)
    send_to_char("You sense nothing out of the ordinary.\r\n", ch);
}

void mortal_stat(struct char_data *ch, char *arg)
{
  struct char_data *victim;
  int diff = 0;

  if (!*arg)
  {
    send_to_char("Sense whose stats?\r\n", ch);
    return;
  }
  // Find the target
  if (!(victim = generic_find_char(ch, arg, FIND_CHAR_ROOM))) 
  {
    send_to_char("You sense a big empty spot where you think they should be!\r\n", ch);
    return;
  }
  /* If it's an npc, the amount of info they get varies according to 
   * relative power */
  if (IS_NPC(victim))
  {
    diff = GET_LEVEL(victim) - GET_LEVEL(ch);	
    // Give some basic info
    sprinttype(GET_CLASS(victim), npc_class_types, buf2);
    sprintf(buf, "%-20s %s\r\n%-20s %s \r\n%-20s %d\r\n", "Name:",
	    GET_NAME(victim), "Type:", buf2, "Level:", GET_LEVEL(victim));
    if (diff < 5) // Npc is possibly much more powerful
    {
      sprintf(buf + strlen(buf), "%-20s %s\r\n%-20s %d\r\n", "Special Powers:",
      GET_MOB_SPEC(victim) ? "Yes" : "No", "Gold:", GET_GOLD(victim));
    }
    if (diff <= 0)	// PC is somewhat more powerful
    {
      sprintf(buf + strlen(buf), "%-20s %d/%d \r\n%-20s %d\r\n", "Life Force:", 
	      GET_HIT(victim), GET_MAX_HIT(victim), "Combat Skill Rating:", 
	      thaco(victim, NULL));
    }
    if (diff < -5) // Player is way more powerful, give some good info
    {
      sprintf(buf + strlen(buf), "%-20s %d \r\n%-20s %d\r\n%-20s %d\r\n",
	      "Damage Bonus:", GET_DAMROLL(victim), 
	      "Hitroll Bonus:", GET_HITROLL(victim), 
	      "Armour Class:", compute_armor_class(victim, 0));
    }
    send_to_char(buf, ch);
    return;
  }
  // Okay they're sensing a player
  if ((GET_LEVEL(victim) >= LVL_ISNOT_GOD) && 
      (GET_LEVEL(victim) > GET_LEVEL(ch)))
  {
    send_to_char("You sense a mighty being, able to crush you with a snap of their fingers!\r\n",ch);
    act("$n just tried to sense your abilities, but failed, as mortals are wont to do.", FALSE, ch, 0, victim, TO_VICT);
    return;
  }
  // TODO - Artus> It looks like this is kind of unfinished.
}

ACMD(do_sense)
{
  two_arguments(argument, buf, buf1);
  if (!GET_SKILL(ch, SKILL_SENSE_CURSE) && !GET_SKILL(ch, SKILL_SENSE_STATS))
  {
    send_to_char("You have no idea how to.\r\n", ch);
    return;
  }
  if (!*buf)
  {
    strcpy(buf, "You are able to sense");
    if (GET_SKILL(ch, SKILL_SENSE_CURSE))
      strcat(buf, " &Bcurses&n");
    if (GET_SKILL(ch, SKILL_SENSE_CURSE) && GET_SKILL(ch, SKILL_SENSE_STATS))
      strcat(buf, " and");
    if (GET_SKILL(ch, SKILL_SENSE_STATS))
      strcat(buf, " &Bstats&n");
    strcat(buf, ".\r\n");
    send_to_char(buf, ch);
    return;
  }	
  toUpper((char *)buf);
  if (strcmp(buf, "STATS") == 0)
    mortal_stat(ch, buf1);
  else if (strcmp(buf, "CURSES") == 0)
    mortal_detectcurses(ch);
  else
    send_to_char("You have no idea how to sense for that.\r\n", ch);
  return;
}

void ch_trigger_trap(struct char_data *ch, struct obj_data *obj)
{
  struct obj_data *temp;
  struct descriptor_data *d;
  int dam;

  if (GET_OBJ_VAL(obj, 0) == 0)
  {
    send_to_char("&bPhew, it wasn't loaded.&n\r\n", ch);
    if (GET_OBJ_VAL(obj, 3) == 0)
      return;	// Nothing more to do
    temp = read_object(GET_OBJ_VAL(obj, 3), VIRTUAL);
    extract_obj(obj);
    obj_to_room(temp, ch->in_room);
    act("The trap was protecting $p!", FALSE, ch, temp, 0, TO_CHAR);
    act("$n triggered a trap, which was fortunately unloaded, finding $p.", FALSE, ch, temp, 0, TO_ROOM);
    return;
  }
  send_to_char("You trigger it!&n\r\n", ch);
  act("...$n triggers a trap!", FALSE, ch, 0, 0, TO_ROOM);
  dam = dice(GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
  if (GET_OBJ_VAL(obj, 0) == 1) 
  {
     send_to_char("You take the brunt of the damage.\r\n", ch);
     // GET_HIT(ch) -= damage;
     damage(NULL, ch, dam, TYPE_UNDEFINED, FALSE);
     update_pos(ch);
  } else {   // Room affect
    send_to_char("The trap goes off, damaging everyone nearby.\r\n", ch);
    act("The trap goes off, damaging everyone in the vicinity!", 
	FALSE, ch, 0, 0, TO_ROOM);	
    for (d = descriptor_list; d; d = d->next)
    {
      if (d->character && (d->character->in_room == ch->in_room) &&
          (d->character != ch))
      damage(NULL, d->character, dam, TYPE_UNDEFINED, FALSE);
    } // Every descriptor
  } // Room affect
  // Check if we replace the obj
  if (GET_OBJ_VAL(obj, 3) >= 1)
  {
    temp = read_object(GET_OBJ_VAL(obj, 3), VIRTUAL);
    act("You find $p in the aftermath.", FALSE, ch, temp,0,TO_CHAR);
    act("You spot $p in the aftermath.", FALSE, ch, temp,0,TO_ROOM);
    obj_to_room(temp, ch->in_room);
  }
  // Get rid of the trap
  extract_obj(obj);
}

ACMD(do_search)
{
  int chance = 0, i, counter = 0, baseChance = 0;
  struct room_data *r;
  struct obj_data *o;

  if (IS_NPC(ch))
    return;
  if (AFF_FLAGGED(ch, AFF_PARALYZED))
  {
    send_to_char("Your limbs won't respond. You're paralysed!\r\n",ch);
    return;
  }
  if (!GET_SKILL(ch, SKILL_SEARCH))
  {
    send_to_char("You stumble around, but achieve nothing.\r\n",ch);
    return;
  }
  if (IS_THIEF(ch)) // Favour thief for multi-classed
    baseChance = 15;	// 15%
  else if (IS_MAGIC_USER(ch))
    baseChance = 5;	// 5%
  else
    baseChance = 2;	// 2%
  if (IS_SET(GET_SPECIALS(ch), SPECIAL_THIEF))
    baseChance += 40;	// +40% for enhanced thieves
  r = &world[ch->in_room];
  // first, check for exits
  for (i = 0; i < NUM_OF_DIRS; i++)
  {
    if (r->dir_option[i] == NULL)
      continue;
    if (EXIT_FLAGGED(EXIT(ch, i), EX_ISDOOR) &&
        EXIT_FLAGGED(EXIT(ch, i), EX_CLOSED))
    {
      sprintf(buf, "You find an exit leading %s.\r\n", dirs[i]);
      send_to_char(buf, ch);
      counter++;
    } 
  }
  // Change the basechance according to player level and class and modifier
  if (GET_LEVEL(ch) > 90) 
    chance = baseChance + 8;
  else if (GET_LEVEL(ch) > 50)
    chance = baseChance + 5;
  else if (GET_LEVEL(ch) > 25)
    chance = baseChance + 2;
  if (GET_CLASS(ch) == CLASS_MASTER)
    chance += 8;
  else if ((GET_CLASS(ch) == CLASS_NIGHTBLADE) ||
           (GET_CLASS(ch) == CLASS_SPELLSWORD))
    chance += 7;
  else if (GET_CLASS(ch) > CLASS_WARRIOR)
    chance += 3;
  chance += 4 + (int)(GET_MODIFIER(ch));

  /* Artus> The Old Way
  chance = baseChance + 
           (GET_LEVEL(ch) > 90 ? 10 :
	    GET_LEVEL(ch) > 50 ? 7  :
	    GET_LEVEL(ch) > 25 ? 4  : 2) +
	   (GET_CLASS(ch) == CLASS_MASTER     ? 10 : 
	    GET_CLASS(ch) == CLASS_NIGHTBLADE ? 9  :
	    GET_CLASS(ch) == CLASS_SPELLSWORD ? 9  :
	    GET_CLASS(ch) >  CLASS_WARRIOR    ? 5  : 2) +
	   int(GET_MODIFIER(ch)); */

  if (chance > 90)
    chance = 90;
  // Now do the real looking
  for (o=r->contents; o; o=o->next_content)
  {
    if (OBJ_FLAGGED(o, ITEM_HIDDEN))
      if (number(1, 100) > 100-chance)
      {
	act("You find $p, hidden nearby!", FALSE, ch, o, 0, TO_CHAR);
	act("$n finds $p, hidden nearby!", FALSE, ch, o, 0, TO_ROOM);
	counter++;
	REMOVE_BIT(o->obj_flags.extra_flags, ITEM_HIDDEN);
	// Check if it's a trap!
	if (GET_OBJ_TYPE(o) == ITEM_TRAP)
	{
	  send_to_char("&RIt's a trap! ", ch);
	  if (number(1, baseChance) == baseChance)  // favour thieves
	    ch_trigger_trap(ch, o);
	  else
          {
	    send_to_char("&g .. but you manage not to trigger it.&n\r\n",ch);
	    // Check if it was hiding an item
	    if (GET_OBJ_VAL(o, 3) != 0)
	    {
	      extract_obj(o);
	      o = read_object(GET_OBJ_VAL(o, 3), VIRTUAL);
	      act("... The trap was protecting $p!", FALSE, ch, o, 0, TO_CHAR);
	      act("$n found $p under a trap!", FALSE, ch, o, 0, TO_ROOM);
	      obj_to_room(o, ch->in_room);
	    }
	  }
	}
      }
  }
  if (!counter)
    send_to_char("You find nothing unusual.\r\n", ch);
}

void sort(long list[], long owners[], int num)
{
  int i, j;
  long hold;
  for (i = 0; i < num; i++)
    for (j = 0; j < num - 1; j++) 
      if (list[j] > list[j + 1])
      {
        hold = list[j];
        list[j] = list[j + 1];
        list[j + 1] = hold;
        // Move the owner with it
        hold = owners[j];
        owners[j] = owners[j + 1];
        owners[j + 1] = hold;
      }
}

ACMD(do_immlist) 
{
  int n = 0;
  extern struct imm_list_element *immlist_table;
  bool dark = false;

  // No Imms.
  if (!(immlist_table))
  {
    send_to_char("C'mon guys, stop being such a bunch of pussies.\r\n", ch);
    return;
  }

  // Now display the stats
  sprintf(buf, "\r\n        &r.-&R'-.&y_.&Y-'&w-&W Primal Immortals &w-&Y'-&y._.&R-'&r-.&n\r\n");
  sprintf(buf + strlen(buf), "\r\n\r\n       &BName                  Kills   Unholiness\r\n");
  for (struct imm_list_element *i = immlist_table; i; i = i->next)
  {
    sprintf(buf + strlen(buf), "       &%c%-20s%7ld%13d\r\n",
	    (dark ? 'C' : 'c'), i->name, i->kills, i->unholiness);
    dark = !(dark);
    n++;
  }
  sprintf(buf + strlen(buf), "\r\n       &B%d &bImmortal%s listed.\r\n&n",
          n, n == 1 ? "" : "s"); 	
  page_string(ch->desc, buf, TRUE);
}

/* Command to display and set player information */
ACMD(do_info)
{
  long tchid;
  struct char_data *tch = NULL;
  struct char_file_u tmp;
  char *newstr;

  if (IS_NPC(ch))
  {
    send_to_char("You already know everything there is to know!\r\n", ch);
    return;
  }

  // two_arguments(argument, arg, buf1);
  newstr = one_argument(argument, arg);

  if (!*arg) 
  {
    do_gen_ps(ch, argument, cmd, SCMD_INFO);
    return;
  }

  // Artus> Moved level restriction inside, so newbies can call with no params.
  if (LR_FAIL_MAX(ch, 10))
  {
    send_to_char("You need to be at least level 10 to set your info!\r\n", ch);
    return;
  }

  if (PLR_FLAGGED(ch, PLR_NOINFO) && LR_FAIL(ch, LVL_GRGOD))
  {
    send_to_char("You can't set your info. Privilidges have been revoked.\r\n",
                    ch);
    return;
  }

  // Just want to know about themselves?
  /* Artus> Info with no args can behave the way it used to behave :o)
  if (!*arg) {
    sprintf(buf, "&gYour current details are:&n\r\n"
            "&y Email:&n %s\r\n &yWebpage:&n %s\r\n&y Personal:&n %s&n\r\n",
            GET_EMAIL(ch) == NULL ? "None" : GET_EMAIL(ch), 
            GET_WEBPAGE(ch) == NULL ? "None" : GET_WEBPAGE(ch),
            GET_PERSONAL(ch) == NULL ? "None" : GET_PERSONAL(ch));
    send_to_char(buf, ch);
    return;
  } */

  if (*newstr)
    skip_spaces(&newstr);
  // Looking for someone?
  if (!*newstr) 
  {
    bool isonline = FALSE;
    if (!str_cmp(arg, "me") || !str_cmp(arg, "self"))
    { // Artus> Check for me/self/online players. .. Slack. :o)
      tch = ch;
      isonline = TRUE;
    } else {
      struct char_data *k;
      for (k = character_list; k; k = k->next)
      {
	if (IS_NPC(k))
	  continue;
	if (str_cmp(arg, GET_NAME(k)))
	  continue;
	tch = k;
	isonline = TRUE;
	break;
      } // Search Online List.
    } // Me/Self/Other Check.
    if (!isonline)
    {
      if ((tchid = get_id_by_name(arg)) < 1) 
      {
	send_to_char("No such player!\r\n", ch);
	return;
      }
      // Load the character up
      CREATE(tch, struct char_data, 1);
      clear_char(tch);
      load_char(arg, &tmp);
      store_to_char(&tmp, tch);
      char_to_room(tch, 0);
    }
    sprintf(buf, "&g%s is a level %d %s, socially ranked as '%s'.&n\r\n",
	GET_NAME(tch), GET_LEVEL(tch), pc_class_types[(int)GET_CLASS(tch)],
	social_ranks[GET_SOCIAL_STATUS(tch)]);
    send_to_char(buf, ch);
    sprintf(buf,"&y Email: &n%s\r\n&y Webpage:&n %s\r\n&y Personal: &n%s&n\r\n",
	    GET_EMAIL(tch) == NULL ? "None" : GET_EMAIL(tch), 
	    GET_WEBPAGE(tch) == NULL ? "None" : GET_WEBPAGE(tch),
	    GET_PERSONAL(tch) == NULL ? "None" : GET_PERSONAL(tch));
    send_to_char(buf, ch);
    if (!isonline)
      extract_char(tch);
    return;		 
  }

  //to_lower(arg);
  // We have both arguments
  if (is_abbrev(arg, "email"))
  {
    if (GET_EMAIL(ch))
      free(GET_EMAIL(ch));
    if (str_cmp(newstr, "clear") == 0 )
    {
      GET_EMAIL(ch) = str_dup("None");
      *newstr = '\0';
      send_to_char("You clear your email.\r\n", ch);
    } else {
      GET_EMAIL(ch) = str_dup(newstr);
      send_to_char("You set your email.\r\n", ch);
    }
    return;
  }

  if (is_abbrev(arg, "webpage"))
  {
    if (GET_WEBPAGE(ch))
      free(GET_WEBPAGE(ch));
    if (!str_cmp(newstr, "clear"))
    {
      GET_WEBPAGE(ch) = str_dup("None");
      send_to_char("You clear your webpage.\r\n", ch);
    } else {
      GET_WEBPAGE(ch) = str_dup(newstr);
      send_to_char("You set your webpage.\r\n", ch);
    }
    return;
  }

  if (is_abbrev(arg, "personal"))
  {
    if (GET_PERSONAL(ch))
      free(GET_PERSONAL(ch));
    if (!str_cmp(newstr, "clear"))
    {
      GET_PERSONAL(ch) = str_dup("None");
      send_to_char("You clear your personal info.\r\n",ch);
    } else {
      GET_PERSONAL(ch) = str_dup(newstr);
      send_to_char("You set your personal info.\r\n", ch);
    }
    return;
  }
  send_to_char("Set what?!\r\n", ch);
}


const char *show_object_damage(struct obj_data *obj)
{
  const char *damage_msgs[] =
  {
    "It is in &gperfect&n condition.\r\n",
    "It is &Bslightly damaged&n.\r\n",
    "It is &Ysomewhat damaged&n.\r\n",
    "It is &ymoderately damaged&n.\r\n",
    "It is &rquite damaged&n.\r\n",
    "It is &Rseverely damaged&n.\r\n",
    "It is &mbroken&n.\r\n",
    "It is &Gindestructable!&n\r\n"
  };
  int dmgno = 0;
  float damage;

  if (obj == NULL)
  {
    mudlog("SYSERR: Showing object damage on null object.", BRF, LVL_GRGOD, 
	   TRUE);
    return "-"; 
  }
  if (GET_OBJ_MAX_DAMAGE(obj) == 0)
    dmgno = 6;
  else if (GET_OBJ_MAX_DAMAGE(obj) <= -1)
    dmgno = 7;
  else
    damage = (GET_OBJ_DAMAGE(obj) *100) / GET_OBJ_MAX_DAMAGE(obj);
  if (dmgno != 0)
    dmgno = dmgno;	// Pre - allocated ( Broken or Indes )
  else if (damage == 0)   // Busted
    dmgno = 6;
  else if (damage <= 20)	// Severely damaged   (01 - 20%)
    dmgno = 5;
  else if (damage <= 50)  // Quite damaged      (21 - 50%)
    dmgno = 4;
  else if (damage <= 70)  // Moderately damaged (51 - 70%)
    dmgno = 3;
  else if (damage <= 85)  // Somewhat damaged   (71 - 85%)
    dmgno = 2;
  else if (damage <= 99)  // Slightly damaged   (86 - 99%) 
    dmgno = 1;
  else dmgno = 0;		// Perfect condition  (  100%  )
//	basic_mud_log("Item %s, %d*100/%d = %lf", obj->short_description, GET_OBJ_DAMAGE(obj),
//		GET_OBJ_MAX_DAMAGE(obj), damage);
  return damage_msgs[dmgno];
}


/*
 * This function screams bitvector... -gg 6/45/98
 */
void show_obj_to_char(struct obj_data * object, struct char_data * ch,
			int mode)
{
  *buf = '\0';
  if ((mode == 0) && object->description)
  {
    strcpy(buf, "&5");
    strcat(buf, object->description);
  } else if ((object->short_description) &&
             ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4)))
  {
    /* Can't use */
    if (invalid_level(ch, object, FALSE))
      strcpy(buf, "&R");
    /* Is Quest */
    else if (OBJ_FLAGGED(object, ITEM_QEQ))
      strcpy(buf, "&Y");
    /* Anything else. */
    else
      strcpy(buf, "&5");
    strcat(buf, object->short_description);
  } else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_NOTE)
    {
      if (object->action_description)
      {
	strcpy(buf, "There is something written upon it:\r\n\r\n");
	strcat(buf, object->action_description);
	page_string(ch->desc, buf, 1);
      } else
	send_to_char("It's blank.\r\n", ch);
      // Artus> This probably shouldn't be here.
      // sprintf(buf, "%s\r\n", show_object_damage(object));
      // send_to_char(buf, ch);
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else			/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");

  }
  // Artus> Couldn't see any point to this.
  // show_object_damage(object);
  if (mode != 3)
  {
    if( IS_OBJ_STAT(object, ITEM_HIDDEN))
      strcat(buf, " (hidden)");
    if (OBJ_RIDDEN(object))
	strcat(buf, " (ridden)");
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE))
      strcat(buf, " (invisible)");
    if (IS_OBJ_STAT(object, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
      strcat(buf, " ..It glows blue!");
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
      strcat(buf, " ..It glows yellow!");
    if (IS_OBJ_STAT(object, ITEM_GLOW))
      strcat(buf, " ..It has a soft glowing aura!");
    if (IS_OBJ_STAT(object, ITEM_HUM))
      strcat(buf, " ..It emits a faint humming sound!");
  }
  strcat(buf, "&n\r\n");
  page_string(ch->desc, buf, TRUE);
}

/*
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
		           int show)
{
  struct obj_data *i;
  bool found = FALSE;

  for (i = list; i; i = i->next_content) {
    if (CAN_SEE_OBJ(ch, i)) {
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}
*/
void list_obj_to_char2(struct obj_data * list, struct char_data * ch, int mode,
                      int show)
{
  // Artus> What the fuck is this supposed to do?
}

void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
                      int show)
{
  struct obj_data *i;
  bool found, exists;
  sh_int *unique=NULL, item_num;
  struct obj_data **u_item_ptrs = NULL;
  int size=0, num, num_unique;
 
  for (i = list; i; i = i->next_content, size++);
 
  if (size != 0)
    CREATE(unique,sh_int,size*2);

  if (unique == NULL)
  {
    list_obj_to_char2(list, ch, mode, show);
    return;
  }
  
  if (size != 0)
    CREATE(u_item_ptrs,struct obj_data*,size);

  if (u_item_ptrs == NULL)
  {
    if (unique != NULL)
      free(unique);
    list_obj_to_char2(list, ch, mode, show);
    return;
  }

  found=FALSE;
  num_unique=0;
  for (i = list; i; i = i->next_content)
  {
    item_num = i->item_number;
    if (item_num < 0)
    {
      if (CAN_SEE_OBJ(ch, i))
      {
        show_obj_to_char(i, ch, mode);
        found = TRUE;
      }
    } else {
      found=TRUE;         
      exists=FALSE;
      num=num_unique;
      while (num && !exists)
      {
        if (unique[(num-1)*2] == item_num)
	{
          exists=TRUE;
          unique[(num-1)*2+1]++;
        }
        num--;
      }
      if (!exists)
      {
        u_item_ptrs[num_unique]=i;
        unique[num_unique*2]=item_num;
        unique[num_unique*2+1]=1;
        num_unique++;
      }
    }
  }
  for (num=0; num<num_unique; num++)
    if (CAN_SEE_OBJ(ch,u_item_ptrs[num]))
    {
      if (unique[num*2+1]>1)
      {
        sprintf(buf2, "&b(&5%d&b)&5 ", unique[num*2+1]);
        send_to_char(buf2, ch);
      }
      show_obj_to_char(u_item_ptrs[num], ch, mode);
    }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);

  if (unique != NULL)
    free(unique);
  if (u_item_ptrs != NULL)
    free(u_item_ptrs);
}

void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " is in &Gexcellent&n condition.\r\n");
  else if (percent >= 90)
    strcat(buf, " has &ga few scratches&n.\r\n");
  else if (percent >= 75)
    strcat(buf, " has some &Ysmall wounds and bruises&n.\r\n");
  else if (percent >= 50)
    strcat(buf, " has &yquite a few wounds&n.\r\n");
  else if (percent >= 30)
    strcat(buf, " has some &Rbig nasty wounds&n and scratches.\r\n");
  else if (percent >= 15)
    strcat(buf, " looks &rpretty hurt&n.\r\n");
  else if (percent >= 0)
    strcat(buf, " is in &Mawful&n condition.\r\n");
  else
    strcat(buf, " is &mbleeding awfully&n from big wounds.\r\n");

   // Some mount information
   if (MOUNTING(i))
   {
      if (IS_NPC(i))
	sprintf(buf1, "Mounted by %s.\r\n", GET_NAME(MOUNTING(i)));
      else
	sprintf(buf1, "Mounted on %s.\r\n", GET_NAME(MOUNTING(i)));	
      strcat(buf, buf1);
   }
   if (MOUNTING_OBJ(i))
   {
      sprintf(buf1, "Mounted on %s.\r\n", MOUNTING_OBJ(i)->short_description);
      strcat(buf, buf1);
   }

  send_to_char(buf, ch);
}


void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found;
  struct char_data *tch;

  if (!ch->desc)
    return;

  // Disguise special
  if (!IS_NPC(i) && IS_SET(GET_SPECIALS(i), SPECIAL_DISGUISE) && 
      CHAR_DISGUISED(i) && !IS_SET(PRF_FLAGS(i), PRF_HOLYLIGHT))
  {
    tch = read_mobile(CHAR_DISGUISED(i), VIRTUAL);
    char_to_room(tch, i->in_room);
    look_at_char(tch, ch);
    extract_char(tch);
    return;       	
  }

  // Artus> Vanity.
  if (!IS_NPC(i) && (GET_IDNUM(i) == 1) && (GET_LEVEL(ch) < LVL_IMPL))
  {
    act("You attempt to gaze upon the mighty $n, but your eyes fail to focus.\r\nYou feel humbled by the radiant essence that eminates from $m.\r\nYou cannot deny that this is one being not to be reckoned with.\r\n", FALSE, i, 0, ch, TO_VICT);
    return;
  }

  if (i->player.description)
    send_to_char(i->player.description, ch);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  
  if (found)
  {
    send_to_char("\r\n", ch);	/* act() does capitalization. */
    act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, wear_positions[j]) && 
	  CAN_SEE_OBJ(ch, GET_EQ(i, wear_positions[j])))
      {
	/* Artus -- This works :o) */
        if ((wear_positions[j] == WEAR_HOLD) && IS_DUAL_WIELDING(i))
    	  send_to_char("<wielded (2nd)>      ", ch);
	else
	  send_to_char(where[wear_positions[j]], ch);
	show_obj_to_char(GET_EQ(i, wear_positions[j]), ch, 1);
      }
  }
  if (ch != i && (IS_THIEF(ch) || !LR_FAIL(ch, LVL_CHAMP)))
  {
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    list_obj_to_char(i->carrying, ch, 1, FALSE);
  }
}

void list_rider(struct char_data *i, struct char_data *ch, int mode)
{
  if (!(CAN_SEE(ch, i) && MOUNTING(i)))
    return;
  if (IS_NPC(i))
  {
    sprintf(buf2, "...ridden by %s.", 
	    (MOUNTING(i) == ch ? "you" :
	     CAN_SEE(ch,MOUNTING(i)) ? GET_NAME(MOUNTING(i)) : "someone"));
    if (mode == 1)
      strcat(buf, buf2);
    if (mode == 0)
      act(buf2, FALSE, i, 0, ch, TO_VICT);
  } else {
    if (mode == 1)
      strcat(buf2, " (mounted)");
    if (mode == 0)
      act(" (mounted)", FALSE, i, 0, ch, TO_VICT);
  }
}

void list_one_char(struct char_data * i, struct char_data * ch)
{
  const char *positions[] =
  {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here."
  };

  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i))
  {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      strcpy(buf, "&6*");
    else
      strcpy(buf, "&6");
    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
    {
      if (IS_EVIL(i))
	strcat(buf, "&r(Red Aura) &6");
      else if (IS_GOOD(i))
	strcat(buf, "&b(Blue Aura) &6");
    }
    if (!IS_NPC(i))
	list_rider(i, ch, 1);
    //strcat(buf, "&6");
    strcat(buf, i->player.long_descr);
    send_to_char(buf, ch);
    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_REFLECT))
      act("...You can see your reflection in $s skin!",FALSE,i,0,ch,TO_VICT);
    if (IS_NPC(i))
      list_rider(i, ch, 0);
    return;
  }
  if (IS_NPC(i))
  {
    strcpy(buf, "&6");
    strcat(buf, i->player.short_descr);
    CAP(buf);
  } else {
/* make it so ya can see they are a wolf/vampire - Vader */
    if(PRF_FLAGGED(i,PRF_WOLF) && affected_by_spell(i,SPELL_CHANGED))
      sprintf(buf,"&7%s the Werewolf",i->player.name);
    else if(PRF_FLAGGED(i,PRF_VAMPIRE) && affected_by_spell(i,SPELL_CHANGED))
      sprintf(buf,"&7%s %s&7",i->player.name,
              (GET_SEX(i) == SEX_MALE ? "the Vampire" : "the Vampiress"));
    else if(EXT_FLAGGED(i,EXT_GHOST))
      sprintf(buf,"&7%s the Ghost", i->player.name);
    else 
    {
      if (strlen(GET_TITLE(i)) > 0)
        sprintf(buf, "&7%s %s&7", i->player.name, GET_TITLE(i));
      else
	sprintf(buf, "&7%s", i->player.name);
    }
  }
  if (char_affected_by_timer(i, TIMER_MEDITATE))
    strcat(buf, " (meditating)");
  if (char_affected_by_timer(i, TIMER_HEAL_TRANCE))
    strcat(buf, " (entranced)");
  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    strcat(buf, " (invisible)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    strcat(buf, " (linkless)");
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
    strcat(buf, " (writing)");
  if (GET_POS(i) != POS_FIGHTING)
  {
    if(IS_AFFECTED(i,AFF_FLY))
      strcat(buf, " is floating here.");
    else
      strcat(buf, positions[(int) GET_POS(i)]);
  } else {
    if (FIGHTING(i))
    {
      strcat(buf, " is here, fighting ");
      if (FIGHTING(i) == ch)
	strcat(buf, "YOU!");
      else
      {
	if (i->in_room == FIGHTING(i)->in_room)
	  strcat(buf, PERS(FIGHTING(i), ch));
	else
	  strcat(buf, "someone who has already left");
	strcat(buf, "!");
      }
    } else			/* NIL fighting pointer */
      strcat(buf, " is here struggling with thin air.");
  }
  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
  {
    if (IS_EVIL(i))
      strcat(buf, " &r(Red Aura)&7");
    else if (IS_GOOD(i))
      strcat(buf, " &b(Blue Aura)&n");
  }
  if (!IS_NPC(i))
    list_rider(i, ch, 1);

  strcat(buf, "&n\r\n");
  send_to_char(buf, ch);

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
  if (IS_AFFECTED(i, AFF_BLIND))
    act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
  if (IS_AFFECTED(i, AFF_REFLECT))
    act("...You can see your reflection in $s skin!",FALSE,i,0,ch,TO_VICT);
  if (IS_NPC(i))
    list_rider(i, ch, 0);
}

void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i)
    {
      if (CAN_SEE(ch, i))
      {
	if (IS_NPC(ch))
	  continue;

        if (IS_NPC(i) || !CHAR_DISGUISED(i))
	 list_one_char(i, ch);
        else
        {
 	  struct char_data *mob = read_mobile(CHAR_DISGUISED(i), VIRTUAL);
	  sprintf(buf, "&6%s", mob->player.long_descr);
 	  send_to_char(buf, ch);
        }
      }
      else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) &&
	       AFF_FLAGGED(i, AFF_INFRAVISION))
	send_to_char("You see a pair of glowing red eyes looking your way.\r\n",
	             ch);
    }
}

void do_auto_exits(struct char_data * ch)
{
  int door, slen = 0;

  *buf = '\0';
  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF))
    do_exits(ch, 0, 0, 0);
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
      slen += sprintf(buf + slen, "%c ", LOWER(*dirs[door]));
  sprintf(buf2, "&8[ Exits: %s]&n\r\n", *buf ? buf : "None! ");
  send_to_char(buf2, ch);
}


ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (AFF_FLAGGED(ch, AFF_BLIND))
  {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
    {
      if (!LR_FAIL(ch, LVL_IS_GOD))
	sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door],
		GET_ROOM_VNUM(EXIT(ch, door)->to_room),
		world[EXIT(ch, door)->to_room].name);
      else
      {
	sprintf(buf2, "%-5s - ", dirs[door]);
	if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
	  strcat(buf2, "Too dark to tell\r\n");
	else
	{
	  strcat(buf2, world[EXIT(ch, door)->to_room].name);
	  strcat(buf2, "\r\n");
	}
      }
      strcat(buf, CAP(buf2));
    }
  send_to_char("Obvious exits:\r\n", ch);
  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
}

void show_wounded_to_char(struct char_data *ch)
{
  struct char_data *pers;
  for (pers = world[ch->in_room].people; pers; pers = pers->next_in_room)
  {
    int perc; 
    if (IS_NPC(pers) || (pers == ch) || !CAN_SEE(ch, pers))
      continue;
    if ((pers == FIGHTING(ch)) || (FIGHTING(pers) == ch))
      continue;
    perc = (int)((100 * GET_HIT(pers)) / GET_MAX_HIT(pers));
    if (perc > 90)
      continue;
    if (perc > 75)
    {
      act("$N&r could use a little healing!&n", TRUE, ch, NULL, pers, TO_CHAR);
      continue;
    }
    if (perc > 50)
    {
      act("$N&r could use some healing!&n", TRUE, ch, NULL, pers, TO_CHAR);
      continue;
    }
    if (perc > 25)
    {
      act("$N&r could use alot of healing!&n", TRUE, ch, NULL, pers, TO_CHAR);
      continue;
    }
    if (perc > 10)
    {
      act("$N&r could use some major healing!&n", TRUE, ch, NULL, pers, TO_CHAR);
      continue;
    }
    act("$N&r could use some miraculous healing, or $E will surely die!!&n\r\n",
	TRUE, ch, NULL, pers, TO_CHAR);
  }
}

void look_at_room(struct char_data * ch, int ignore_brief)
{
  int i = 0;

  if (!ch->desc)
    return;

  // Artus> This is bad(tm).
  if (IN_ROOM(ch) == NOWHERE)
  {
    sprintf(buf, "SYSERR: NOWHERE while %s looking at room.", GET_NAME(ch));
    mudlog(buf, NRM, LVL_IMPL, TRUE);
    send_to_char("You are NOWHERE?!?! This is a bug.\r\n"
	         "Please email a report to bugs@mud.alphalink.com.au.\r\n", ch);
    core_dump();
    return;
  }

  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch))
  {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
  }

  send_to_char(/*CCCYN(ch, C_NRM)*/"&8", ch);
  // If the room is a burgled area, it will have varying descriptions, reget
  long lBurgleFlag;
  if ((lBurgleFlag = is_room_burgled(ch->in_room)) != 0)
  {
     world[ch->in_room].name = rand_name(lBurgleFlag);
     world[ch->in_room].description = rand_desc(lBurgleFlag);
  }
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
  {
    sprintbit(ROOM_FLAGS(ch->in_room), room_bits, buf);
    sprintbit(BURGLE_FLAGS(ch->in_room), burgle_rooms, buf1);
    sprintf(buf2, "[%5d] %s [ %s] [ %s]", GET_ROOM_VNUM(IN_ROOM(ch)),
	    world[ch->in_room].name, buf, buf1);
    send_to_char(buf2, ch);
  } else
    send_to_char(world[ch->in_room].name, ch);

  send_to_char(CCNRM(ch, C_NRM), ch);
  send_to_char("\r\n", ch);

  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
    send_to_char(world[ch->in_room].description, ch);

  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(ch);

  if ((has_stats_for_skill(ch, SKILL_DETECT_DEATH, FALSE)) && 
      (number(1, 101) < GET_SKILL(ch, SKILL_DETECT_DEATH)))
  {
    bool found=FALSE;
    for (i = 0; i < NUM_OF_DIRS; i++)
      if (EXIT(ch, i) && (EXIT(ch, i)->to_room != NOWHERE) &&
	  ROOM_FLAGGED(EXIT(ch, i)->to_room, ROOM_DEATH))
      {
	found=TRUE;
	if (i < DOWN)
	  sprintf(buf, "&rYou sense death to your %s.&n\r\n", dirs[i]);
	else
	  sprintf(buf, "&rYou sense death %s you.&n\r\n", (i == DOWN) ? "below" : "above");
	send_to_char(buf, ch);
      }
    if (found)
      apply_spell_skill_abil(ch, SKILL_DETECT_DEATH);
  }

  if (RMSM_FLAGGED(IN_ROOM(ch), RMSM_BURNED))
    send_to_char("&cThe ground here is laden with ash.&n\r\n", ch);

  /* now list characters & objects */
  //send_to_char(CCGRN(ch, C_NRM), ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  //send_to_char(CCYEL(ch, C_NRM), ch);
  list_char_to_char(world[ch->in_room].people, ch);

  //Artus> Sense Wounds.
  if(!IS_NPC(ch) && AFF_FLAGGED(ch, AFF_SENSE_WOUNDS))
    show_wounded_to_char(ch);

  if(HUNTING(ch)) 
  {
    send_to_char(CCRED(ch, C_NRM), ch);
    do_track(ch, "", 0, SCMD_AUTOHUNT);
  }
  send_to_char(CCNRM(ch, C_NRM), ch);
}

void look_in_direction(struct char_data * ch, int dir)
{
  if (EXIT(ch, dir))
  {
    if (EXIT(ch, dir)->general_description)
      send_to_char(EXIT(ch, dir)->general_description, ch);
    else
      send_to_char("You see nothing special.\r\n", ch);

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    } else if ((EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR)) &&
	       (EXIT(ch, dir)->keyword))
    {
      sprintf(buf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    }
  } else
    send_to_char("Nothing special there...\r\n", ch);
}

void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;

  if (!*arg)
  {
    send_to_char("Look in what?\r\n", ch);
    return;
  }
  if (!(generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, 
	             ch, &dummy, &obj))) 
  {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  }
  show_contents_to(ch, obj);
}

// Artus> Helper function for look_in_obj() and examine_obj().
void show_contents_to(struct char_data *ch, struct obj_data *obj)
{
  int amt;

  if (!(obj))
  {
    mudlog("SYSERR: show_contents_to() called with no obj.", NRM, LVL_IMPL,
	   TRUE);
    return;
  }
  if (!(ch))
  {
    mudlog("SYSERR: show_contents_to() called with no char.", NRM, LVL_IMPL,
	   TRUE);
    return;
  }
  
  if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
      (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
  {
    send_to_char("It's solid!\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) 
  {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
    {
      send_to_char("It is closed.\r\n", ch);
      return;
    }
    send_to_char(fname(obj->name), ch);
    if (obj->worn_by == ch)
      send_to_char(" (worn):\r\n", ch);
    else if (obj->carried_by == ch)
      send_to_char(" (carried):\r\n", ch);
    else 
      send_to_char(" (here):\r\n", ch);
    list_obj_to_char(obj->contains, ch, 2, TRUE);
    return;
  }
  /* item must be a fountain or drink container */
  if (GET_OBJ_VAL(obj, 1) <= 0)
  {
    send_to_char("It is empty.\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) 
  {
    send_to_char("Its contents seem somewhat murky.\r\n", ch); /* BUG */
    return;
  }
  amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
  sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
  sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
  send_to_char(buf, ch);
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);
  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 *
 * Artus> Modified to allow target to be read by calling functions.
 */
void look_at_target(struct char_data *ch, char *arg, 
                    struct char_data **tch, struct obj_data **tobj)
{
  int bits, found = FALSE, j, fnum, i = 0;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;
  bool glancing = FALSE;

  if (tch)
    *tch = NULL;
  if (tobj)
    *tobj = NULL;

  if (!ch->desc)
    return;

  if (!*arg)
  {
    send_to_char("Look at what?\r\n", ch);
    return;
  }

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) 
  {
    if (tch)
      *tch = found_char;
    look_at_char(found_char, ch);
    if (ch == found_char) 
      return; // Don't display look/self messages, don't apply glance.
    if (!IS_NPC(ch) && (GET_SKILL(ch, SKILL_GLANCE)))
      glancing = (number(0, 101) < GET_SKILL(ch, SKILL_GLANCE));
    if (glancing)
    {
      apply_spell_skill_abil(ch, SKILL_GLANCE);
      if (GET_LEVEL(found_char) > LVL_ISNOT_GOD)
	act("$n glances at you", TRUE, ch, 0, found_char, TO_VICT);
      return; // Don't display look at message if glancing.
    }
    if (CAN_SEE(found_char, ch))
      act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
    act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg)))
  {
    send_to_char("Look at what?\r\n", ch);
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if (((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) &&
      (++i == fnum))
  {
    page_string(ch->desc, desc, FALSE);
    return;
  }

  /* Does the argument match an extra desc in the char's equipment? */
  for (j=0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if (((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL) &&
	  (++i == fnum))
      {
	send_to_char(desc, ch);
	send_to_char(show_object_damage(GET_EQ(ch, j)), ch);
	found = TRUE;
	if (tobj)
	  *tobj = GET_EQ(ch, j);
      }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj=ch->carrying; obj && !found; obj = obj->next_content)
  {
    if (CAN_SEE_OBJ(ch, obj))
      if (((desc = find_exdesc(arg, obj->ex_description)) != NULL) &&
	  (++i == fnum))
      {
	send_to_char(desc, ch);
	send_to_char(show_object_damage(obj), ch);
	found = TRUE;
	if (tobj)
	  *tobj = obj;
      }
  }
  /* Does the argument match an extra desc of an object in the room? */
  for (obj=world[ch->in_room].contents; obj && !found; obj=obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if (((desc = find_exdesc(arg, obj->ex_description)) != NULL) &&
	  (++i == fnum))
      {
	send_to_char(desc, ch);	
	send_to_char(show_object_damage(obj), ch);
	found = TRUE;
	if (tobj)
	  *tobj = obj;
      }
  /* If an object was found back in generic_find */
  if (bits) 
  {
    if (!found)
      show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
    else
      show_obj_to_char(found_obj, ch, 6);	/* Find hum, glow etc */
    if ((tobj) && !(*tobj))
      *tobj = found_obj;
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);
  desc = NULL;
}


ACMD(do_look)
{
  char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);	/* glowing red eyes */
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Read what?\r\n", ch);
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}

ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg, &tmp_char, &tmp_object);

//  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
//		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) 
  {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) 
    {
      send_to_char("When you look inside, you see:\r\n", ch);
      show_contents_to(ch, tmp_object);
    }
  }
}

ACMD(do_gold)
{
  long total = GET_GOLD(ch) + GET_BANK_GOLD(ch);

  // No Gold.
  if (total < 1)
  {
    send_to_char("You're dead broke!\r\n", ch);
    return;
  }
  // One pathetic coin.
  if (total == 1) 
  {
    if (GET_GOLD(ch) < 1)
      send_to_char("You have one miserable little gold coin, banked away.\r\n", ch);
    else
      send_to_char("You fumble around in your pockets to make sure you haven't lost your one coin.\r\n", ch);
    return;
  }
  // All on hand.
  if (GET_BANK_GOLD(ch) < 1)
  {
    sprintf(buf, "You have &Y%d&n gold coins all on hand.\r\n", GET_GOLD(ch));
    if (GET_GOLD(ch) >= 250000)
      strcat(buf, "Perhaps you should put some in the bank?\r\n");
    send_to_char(buf, ch);
    return;
  }
  // All in bank.
  if (GET_GOLD(ch) < 1)
  {
    sprintf(buf, "You have &Y%d&n gold coins, all kept safely in the bank.\r\n", GET_BANK_GOLD(ch));
    send_to_char(buf, ch);
    return;
  }
  // Combination.
  sprintf(buf, "You have &Y%d&n gold coin%s, and &Y%d&n in the bank, totalling &Y%ld&n.\r\n", GET_GOLD(ch), (GET_GOLD(ch) > 1 ? "s" : ""), GET_BANK_GOLD(ch), total);
  if (GET_GOLD(ch) > GET_BANK_GOLD(ch))
    strcat(buf, "Maybe you should put some more in the bank?\r\n");
  send_to_char(buf, ch);
}

#if 0 // Artus> Currently Unused.
// Sets the bits in the bitvector according to user choice
long getScoreDetail(char *argument, struct char_data *ch)
{
  long lVector=0;
  int nLevel;

  if (!strcmp(argument, "brief"))
    nLevel = 1;
  else if (!strcmp(argument, "long"))
    nLevel = 3;
  else if (!strcmp(argument, "regular"))
    nLevel = 2;
  else
    return -1;

  if (nLevel >= 1) 		// Bare
  {	
    SET_BIT(lVector, SCORE_DAMROLL);
    SET_BIT(lVector, SCORE_HITROLL);
    SET_BIT(lVector, SCORE_NAME);
    SET_BIT(lVector, SCORE_AC);
    SET_BIT(lVector, SCORE_HMV);
  }
  if (nLevel >= 2)		// Other stuff
  {
    SET_BIT(lVector, SCORE_ALIGN);
    SET_BIT(lVector, SCORE_THACO);
    SET_BIT(lVector, SCORE_LEVEL);
    SET_BIT(lVector, SCORE_RACE);
    SET_BIT(lVector, SCORE_CLASS);
    SET_BIT(lVector, SCORE_STATS);
  }
  if (nLevel >= 3)		// The whole deal
  {
    SET_BIT(lVector, SCORE_AFFECTS);
    SET_BIT(lVector, SCORE_ABILITIES);
    SET_BIT(lVector, SCORE_INVENTORY);
    SET_BIT(lVector, SCORE_AGE);
    SET_BIT(lVector, SCORE_HEIGHTWEIGHT);
    SET_BIT(lVector, SCORE_SEX);
    SET_BIT(lVector, SCORE_GOLD);
    SET_BIT(lVector, SCORE_TIME);
    SET_BIT(lVector, SCORE_CARRYING);
  }
  SCORE_SETTINGS(ch) = lVector;
  return lVector;
}

void score_help_to_char(struct char_data *ch)
{
  send_to_char("The valid score settings are:\r\n"
               "  damroll  hitroll    age        level    affects\r\n"
               "  name     abilities  inventory  height   hmv\r\n"
               "  sex      gold       stats      class    ac\r\n"
               "  align    thaco      carrying   none\r\n"
               "In addition, the following sets are available:\r\n"
               "  brief    regular    long\r\n", ch);
}

void setScore(struct char_data *ch, char *argument)
{
  if (getScoreDetail(argument, ch) != -1)
    return;
  else if (strcmp(argument, "damroll") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_DAMROLL);
  else if (strcmp(argument, "hitroll") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_HITROLL);
  else if (strcmp(argument, "age") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_AGE);
  else if (strcmp(argument, "level") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_LEVEL);
  else if (strcmp(argument, "affects") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_AFFECTS);
  else if (strcmp(argument, "name") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_NAME);
  else if (strcmp(argument, "abilities") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_ABILITIES);
  else if (strcmp(argument, "inventory") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_INVENTORY);
  else if (strcmp(argument, "height") == 0 || strcmp(argument, "weight") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_HEIGHTWEIGHT);
  else if (strcmp(argument, "race") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_RACE);
  else if (strcmp(argument, "sex") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_SEX);
  else if (strcmp(argument, "gold") == 0 || strcmp(argument, "bank") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_GOLD);
  else if (strcmp(argument, "stats") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_STATS);
  else if (strcmp(argument, "class") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_CLASS);
  else if (strcmp(argument, "ac") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_AC);
  else if (strcmp(argument, "align") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_ALIGN);
  else if (strcmp(argument, "thac0") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_THACO);
  else if (strcmp(argument, "hmv") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_HMV);
  else if ((strcmp(argument, "quest") == 0) ||
	   (strcmp(argument, "questpoints") == 0))
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_QUESTPOINTS);
  else if (strcmp(argument, "carrying") == 0)
    SETREMOVE(SCORE_SETTINGS(ch), SCORE_CARRYING);
  else if (strcmp(argument, "list") == 0)
  {
    score_help_to_char(ch);
    return;
  }
  else if (strcmp(argument, "none") == 0)
  {
    send_to_char("All score settings removed.\r\n", ch);
    SCORE_SETTINGS(ch) = 0;
    return;
  }
  else
    strcpy(argument, "unknown");

  // Inform player
  if (strcmp(argument, "unknown") == 0)
    send_to_char("Unknown score setting...Try '&yscore set list&n'...\r\n", ch);
  else
  {
    sprintf(buf2, "Score setting for &B%s&n updated...\r\n", argument);
    send_to_char(buf2, ch);
  }
}

char *getColorType(int current, int max)
{
  basic_mud_log("%d, %d", current, max);

  if (current < (max / 4))
    return "&r";
  if (current < (max / 2))
    return "&R";
  if (current < max)
    return "&Y";
  
  return "&G";
}

void showScoreItem(struct char_data *ch, int item, bool show)
{
  bool bFound = TRUE;

  if (!show) 
    return;
  switch(item)
  {
    case 0:	//  Damroll
      sprintf(buf, "Damroll: %d\r\n", GET_DAMROLL(ch));
      break;
    case 1:	// Hitroll
      sprintf(buf, "Hitroll: %d\r\n", GET_HITROLL(ch));
      break;
    case 2: // Age
      sprintf(buf, "Age: %d\r\n", GET_AGE(ch));
      break;
    case 3: // Name
      sprintf(buf, "Character Name: %s\r\n", GET_NAME(ch));
      break;
    case 4: // Level
      sprintf(buf, "Level: %d\r\n", GET_LEVEL(ch));
      break;
    case 5: // Stats
      sprintf(buf, "Strength    :  %2d (%2d)\r\n"
		   "Intelligence:  %2d (%2d)\r\n"
		   "Wisdom      :  %2d (%2d)\r\n"
		   "Dexterity   :  %2d (%2d)\r\n"
		   "Constitution:  %2d (%2d)\r\n"
		   "Charisma    :  %2d (%2d)\r\n",
	      GET_REAL_STR(ch), GET_AFF_STR(ch), 
	      GET_REAL_INT(ch), GET_AFF_INT(ch),
	      GET_REAL_WIS(ch), GET_AFF_WIS(ch),
	      GET_REAL_DEX(ch), GET_AFF_DEX(ch),
	      GET_REAL_CON(ch), GET_AFF_CON(ch),
	      GET_REAL_CHA(ch), GET_AFF_CHA(ch));
      break;
    case 6: // Race
      sprinttype(GET_RACE(ch), pc_race_types, buf2);
      sprintf(buf, "Race: %s\r\n", buf2);
      break;
    case 7: // Class
      sprinttype(GET_CLASS(ch), pc_class_types, buf2);
      sprintf(buf, "Class: %s\r\n", buf2);
      break;
    case 8: // HMV
      sprintf(buf, "Hit Points: %s%5d&n / %-5d\r\n"
		   "Mana      : %s%5d&n / %-5d\r\n"
		   "Movement  : %s%5d&n / %-5d\r\n",
		   getColorType(GET_HIT(ch), GET_MAX_HIT(ch)),
		   GET_HIT(ch), GET_MAX_HIT(ch),
		   getColorType(GET_MANA(ch), GET_MAX_MANA(ch)),
		   GET_MANA(ch), GET_MAX_MANA(ch),
		   getColorType(GET_MOVE(ch), GET_MAX_MOVE(ch)),
		   GET_MOVE(ch), GET_MAX_MOVE(ch));
      break;
    case 9: // AC
//      sprintf(buf, "Armour Class: %d\r\n", GET_AC(ch));
      sprintf(buf, "Armour Class: %d\r\n", compute_armor_class(ch, 0));
      break;
    case 10: // THAC0
      sprintf(buf, "THAC0: %d\r\n", thaco(ch, NULL));
      break;
    case 11: // Height & Weight
      sprintf(buf, "Height: %d, Weight: %d\r\n",
	      GET_HEIGHT(ch), GET_WEIGHT(ch));
      break;
    case 12: // Align
      sprintf(buf, "Alignment: %d\r\n", GET_ALIGNMENT(ch));
      break;
    case 13: // Gold & Bank
      sprintf(buf, "Gold: %d, Bank: %d\r\n", GET_GOLD(ch), GET_BANK_GOLD(ch));
      break;
    case 14: // Time
      sprintf(buf, "Time played: %s\r\n", "TODO!");
      break;
    case 15: // Inv
      sprintf(buf, "Inventory stats: %s\r\n", "TODO!");
      break;
    case 16: // Affects
      sprintf(buf, "Affected by: %s\r\n", "TODO!");
      break;
    case 17: // Abilities
      sprintf(buf, "Abilities: %s\r\n", "TODO!");
      break;
    case 18: // Sex
      sprintf(buf, "Sex: %s\r\n", (GET_SEX(ch) == SEX_MALE   ? "Male"   :
	                           GET_SEX(ch) == SEX_FEMALE ? "Female" :
				   "N/A"));
      break; 
    case 19: // Quest points
      sprintf(buf, "Stat points: %d\r\n", GET_STAT_POINTS(ch));
      break;
    default: 
      bFound = FALSE;
      break;
  }
  if (bFound)
    send_to_char(buf, ch);
}
#endif

/* Primal score:
 * 	- Allows for brief/regular/detailed score commands
 *	- Allows complete customisation
 *	- Information limited by level
 */
void primal_score(struct char_data *ch, char *argument)
{
   //byte crap=0;
   void old_primal_score(struct char_data *ch);
   
   // DM - TEMP, use old score page
   old_primal_score(ch);
   return;

#if 0 // Artus> Not Used.
   half_chop(argument, arg, buf1);

   if (*argument && strcmp(arg, "set") == 0)
   {
	setScore(ch, buf1);
	return;
   }
   /*
   if (!*argument)
	lScoreDetail = getScoreDetail("regular");
   else  
	lScoreDetail = getScoreDetail(arg);
   */

   /* Old 
   send_to_char("--( TODO: Refine format  )--\r\n", ch);
   for( int i = 0; i < NUM_SCORE_SETTINGS; i++)
   {
      // Do they want to see it, and is it in their detail level chosen?
      if (IS_SET(SCORE_SETTINGS(ch), (1 << i)) / *&& IS_SET(lScoreDetail, (1 << i))* /)
	showScoreItem(ch, i, TRUE);
      else
	showScoreItem(ch, i, FALSE);	  
   }
   */
   sprintf(buf, "\r\n&r.-&R'-.&y_.&Y-'-&y._");
   for (int i = 0; i < 5; i++) 
     strcat(buf, "&r.-&R'-.&y_.&Y-'-&y._");
   strcat(buf, "&r,-&R'-.&n\r\n\r\n");

   // Line 2: Lvl sex race, name class: age:
   
   if (IS_SET(SCORE_SETTINGS(ch), SCORE_LEVEL)) {
     crap = 1;
     sprintf(buf, "%s  &0Lvl&n %3d", buf, GET_LEVEL(ch));      
   }

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_SEX)) {
     if (crap <1) 
       strcat (buf, "  ");
     else
       strcat (buf, " ");
     crap = 1;
     switch (GET_SEX(ch)) {
       case SEX_MALE:
         strcat(buf, "Male");
         break;
       case SEX_FEMALE:
         strcat(buf, "Female");
         break;
       default:
         strcat(buf, "Sexless");
     }
   }

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_RACE)) {
     if (crap <1) 
       strcat (buf, "  ");
     else
       strcat (buf, " ");
     crap = 1;
     sprintf(buf, "%s%s", buf, pc_race_types[GET_RACE(ch)]);
   }

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_NAME)) {
     if (crap <1) 
       strcat (buf, "  ");
     else
       strcat(buf, ", ");
     crap = 1;
     sprintf(buf, "%s%s", buf, GET_NAME(ch));
   }

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_CLASS)) {
     if (crap <1) 
       strcat (buf, "  ");
     else
       strcat(buf, " ");
     crap = 1;
     sprintf(buf, "%s&0Class:&n %s", buf, pc_class_types[(int)GET_CLASS(ch)]);
   }

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_AGE)) {
     if (crap <1) 
       strcat (buf, "  ");
     else
       strcat(buf, " ");
     crap = 1;
     sprintf(buf, "%s&0Age:&n %d", buf, GET_AGE(ch));
   }

   if (crap == 1)
     strcat(buf, "\r\n");

   // Line 3: 

   crap = 0;
   if (IS_SET(SCORE_SETTINGS(ch), SCORE_HMV)) {
     crap = 1;
     sprintf (buf, "%s  &9Hit:&n %d&4/&n%d&4+&n%d &9Mana:&n %d&4/&n%d&4+&n%d &9Move:&n %d&4/&n%d&4+&n%d", buf, GET_HIT(ch), GET_MAX_HIT(ch), hit_gain(ch), GET_MANA(ch), GET_MAX_MANA(ch), mana_gain(ch), GET_MOVE(ch), GET_MAX_MOVE(ch), move_gain(ch));
   }

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_THACO)) {
     if (crap < 1)
       sprintf(buf, "%s  &9THAC0:&n %-5d", buf, thaco(ch, NULL));
     else  
       sprintf(buf, "%s &9THAC0:&n %-5d", buf, thaco(ch, NULL));
     crap = 1;
   } 
   if (crap == 1)
     strcat(buf, "\r\n");

   // Line 4:

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_STATS)) {
     sprintf (buf, "%s  &9Str:&n %d&4(&n%d&4)&n &9Int:&n %d&4(&n%d&4)&n &9Wis:&n %d&4(&n%d&4)&9 Dex:&n %d&4(&n%d&4)&9 Con:&n %d&4(&n%d&4)&9 Cha:&n %d&4(&n%d&4)&n\r\n", buf, GET_REAL_STR(ch), GET_AFF_STR(ch), GET_REAL_INT(ch), GET_AFF_INT(ch), GET_REAL_WIS(ch), GET_AFF_WIS(ch), GET_REAL_DEX(ch), GET_AFF_DEX(ch), GET_REAL_CON(ch), GET_AFF_CON(ch), GET_REAL_CHA(ch), GET_AFF_CHA(ch));
   }

   // Line 5:
   crap = 0;

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_DAMROLL)) { 
     sprintf(buf, "%s  &9Damroll:&n %-11d", buf, GET_DAMROLL(ch));
     crap = 1;
   } else
     sprintf(buf, "%s%22s", buf, " ");

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_HITROLL)) { 
     sprintf(buf, "%s&9Hitroll: &n%-4d", buf, GET_HITROLL(ch));
     crap = 1;
   } else 
     sprintf(buf, "%s%13s", buf, " ");

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_AC)) { 
     sprintf(buf, "%s&9Ac:&n %4d&n  ", buf, GET_AC(ch));
     crap = 1;
   } else
     sprintf(buf, "%s%13s", buf, " ");

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_ALIGN)) { 
     crap = 1;
     if (GET_ALIGNMENT(ch) > 350) {
       sprintf(buf1, "&cGood");
       sprintf(buf2, "&c");
     } else if (GET_ALIGNMENT(ch) < -350) {
       sprintf(buf1, "&rEvil");
       sprintf(buf2, "&r");
     } else {
       sprintf(buf1, "&WNeutral");
       sprintf(buf2, "&W");
     }     
     sprintf(buf, "%s&9Align:&n %s&b(%s%d&b)&n", buf, buf1, buf2, GET_ALIGNMENT(ch)); 
   }

   if (crap == 1)
     strcat(buf, "\r\n");

   // Line 6: Gold,Bank,Height,Weight

   crap = 0;

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_GOLD)) {
     sprintf(buf,"%s  &9Gold   : &Y%-14d&9Bank: &Y%-16d&n", buf, GET_GOLD(ch), GET_BANK_GOLD(ch));
     crap = 1;
   }

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_HEIGHTWEIGHT)) {
     if (crap < 1)
       strcat(buf, "  ");
     sprintf(buf,"%s&9Height:&n %d &9Weight:&n %d", buf, GET_HEIGHT(ch), GET_WEIGHT(ch));
     crap = 1;
   }
   
   if (crap > 0)
     strcat(buf, "\r\n");

   // Line 7: Carrying Weight/Items

   if (IS_SET(SCORE_SETTINGS(ch), SCORE_CARRYING))
     sprintf(buf, "%s  &9Carrying Items:&n %3d&4/&n%-16d&9Carrying Weight:&n %d&4/&n%d\r\n", buf, IS_CARRYING_N(ch), CAN_CARRY_N(ch), IS_CARRYING_W(ch), CAN_CARRY_W(ch));

   send_to_char(buf, ch);
   
/* Score flags
#define SCORE_DAMROLL		(1 << 0) 	/ * Damroll * /  
#define SCORE_HITROLL		(1 << 1) 	/ * Hitroll * /
#define SCORE_AGE		(1 << 2)	/ * Age * /
#define SCORE_NAME		(1 << 3)	/ * Character name * /
#define SCORE_LEVEL		(1 << 4)	/ * Character Level  * /
#define SCORE_STATS		(1 << 5)	/ * Character's stats * /
#define SCORE_RACE		(1 << 6)	/ * Race * /
#define SCORE_CLASS		(1 << 7)	/ * Class * /
#define SCORE_HMV		(1 << 8)	/ * Hit/mana/move * /
#define SCORE_AC		(1 << 9)	/ * Armour class * /
#define SCORE_THACO		(1 << 10)	/ * THAC0 * /
#define SCORE_HEIGHTWEIGHT	(1 << 11)	/ * Height & Weight * /
#define SCORE_ALIGN		(1 << 12)	/ * Alignment * /
#define SCORE_GOLD		(1 << 13)	/ * Gold & Bank gold * /
#define SCORE_TIME		(1 << 14)	/ * Time played * /
#define SCORE_INVENTORY		(1 << 15)	/ * Inv details * /
#define SCORE_AFFECTS		(1 << 16)	/ * Affected by  * /
#define SCORE_ABILITIES		(1 << 17)	/ * Abilities they have * /
#define SCORE_SEX		(1 << 18)	/ * Not what you're thinking * /
#define SCORE_QUESTPOINTS	(1 << 19)	/ * Qp's * /
#define SCORE_CARRYING          (1 << 20)       / * carrying... * /
// Must be number of SCORE_ thingies
    */
#endif
}

#if 0 // Artus> Unused.
int compute_damage_roll(struct char_data *ch)
{
  int nRaw = GET_DAMROLL(ch);
  double dbTmp = 0.0;
  
  if (IS_SET(GET_SPECIALS(ch), SPECIAL_MINOTAUR))
    dbTmp += MAX(5, (int)(GET_DAMROLL(ch) * 0.1));

  if (IS_SET(GET_SPECIALS(ch), SPECIAL_SUPERMAN))
    dbTmp += MAX(5, (int)(GET_DAMROLL(ch) * 0.1));

  return (int)(dbTmp + nRaw);
}
#endif

int digits(long number)
{
  char tmp[80];
  sprintf(tmp, "%ld", number);
  return (strlen(tmp));
}

/* DM - snazzed up score, just change these defines for the given colors */
#define CCSTAR(ch,lvl) CCBLU(ch,lvl)	/* Stars 			*/
#define CCHEAD(ch,lvl) CCBRED(ch,lvl)	/* Main headings 		*/
#define CCSUB(ch,lvl)  CCRED(ch,lvl)	/* Sub headings 		*/
#define CCNUMB(ch,lvl) CCCYN(ch,lvl)	/* Numbers - Age, Time		*/
#define CCTEXT(ch,lvl) CCBWHT(ch,lvl)	/* Text				*/
#define CCNAME(ch,lvl) CCBBLU(ch,lvl)	/* Name, Title			*/
#define CCSEP(ch,lvl)  CCBBLU(ch,lvl)	/* Seperators / ( ) d h 	*/
#define CCSTAT(ch,lvl) CCCYN(ch,lvl)	/* Stats			*/
#define CCDH(ch,lvl)   CCBRED(ch,lvl)	/* Damroll, Hitroll		*/
#define CCACT(ch,lvl)  CCCYN(ch,lvl)	/* AC, Thac0			*/
#define CCGAIN(ch,lvl) CCCYN(ch,lvl)	/* Hit/Mana/Move gain		*/

void old_primal_score(struct char_data *ch)
{
  char *line="%s*******************************************************************************%s\r\n";
  char *star="%s*%s";
  char cline[MAX_INPUT_LENGTH], cstar[MAX_INPUT_LENGTH];
  char ch_name[MAX_NAME_LENGTH+1];
  //extern int thaco(struct char_data *);
  //extern struct str_app_type str_app[];
  char buf3[80],alignbuf[15];
  struct time_info_data playing_time;
  //struct time_info_data *real_time_passed(time_t t2, time_t t1);
  unsigned int i,j;
 
  if (IS_NPC(ch))
  {
    send_to_char("You're never going to score.\r\n",ch);
    return;
  }

  sprintf(buf2,line,CCSTAR(ch,C_NRM),CCNRM(ch,C_NRM));
  strcpy(cline,buf2);
  sprintf(buf2,star,CCSTAR(ch,C_NRM),CCNRM(ch,C_NRM));
  strcpy(cstar,buf2);

  send_to_char("\r\n", ch);

/* First Line - *** */
  strcpy(ch_name,GET_NAME(ch)); 

  send_to_char(cline,ch);

/* Second Line - Char Name */

  sprintf(buf,"%s &0Name&n: &7%s&n", cstar, ch_name);

  for (i=strlen(ch_name); i<27; i++)
    strcat(buf," ");
/*  for (i=1;i<(28-strlen(ch_name));i++)
      strcat(buf," "); Artus -- Norty inefficient bois :o) */

  sprintf(buf2,"%s &0Title&n: %s&n", cstar, GET_TITLE(ch));
  strcat(buf,buf2);

  for (i=strdisplen(GET_TITLE(ch)); i<34; i++)
    strcat(buf," ");

  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch); 

  /* Clan Rank, Clan Name - ARTUS */
  if (GET_CLAN(ch) > 0)
  {
    sprintf(buf2, "%s &0Clan&n: &g%s&n", cstar,
	    clan[find_clan_by_id(GET_CLAN(ch))].name);
    strcpy(buf, buf2);
    for(i=strlen(buf2); i<52; i++)
      strcat(buf, " ");
    sprintf(buf2, "%s &0Rank&n: &g%s&n", cstar, 
	   (GET_CLAN_RANK(ch) < 1) ? "Applying" :
	    clan[find_clan_by_id(GET_CLAN(ch))].rank_name[GET_CLAN_RANK(ch)-1]);
    strcat(buf, buf2);
    for(i=strlen(buf2); i<60; i++)
      strcat(buf, " ");
    strcat(buf, cstar);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
  }
  send_to_char(cline, ch);
  // Disguise
  char mobname[80];
  if (!IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_DISGUISE))
  {
    if (CHAR_MEMORISED(ch) > 0)
      strcpy(mobname, 
	     mob_proto[real_mobile(CHAR_MEMORISED(ch))].player.short_descr);
    else
      strcpy(mobname, "None");
    sprintf(buf, "%s &0Mob memorised&n: &6%s&n", cstar, mobname); 
    for (i=strlen(mobname); i<18; i++)
      strcat(buf, " ");
    sprintf(buf2, "%s &0Disguised&n: %s&n", cstar,
	    CHAR_DISGUISED(ch) == 0 ? "No" : "Yes");
    if (CHAR_DISGUISED(ch))
    {
      for (i=1; i < 28; i++)
        strcat(buf2, " ");
    } else {
      for (i=1; i < 29; i++)
        strcat(buf2, " ");
    }
    strcat(buf2, cstar);
    strcat(buf2, "\r\n");
    strcat(buf, buf2);
    send_to_char(buf, ch);
  }

  /* Kill Counts - Artus */
  sprintf(buf2, "%s &0Kills&n        : &1Imm&n[&c%ld&n] &1By Imm&n[&c%ld&n] "
		"&1Mob&n[&c%ld&n] &1By Mob&n[&c%ld&n] &1PC&n[&c%ld&n] "
		"&1By PC&n[&c%ld&n]", cstar, GET_IMMKILLS(ch),
	  GET_KILLSBYIMM(ch), GET_MOBKILLS(ch), GET_KILLSBYMOB(ch),
	  GET_PCKILLS(ch), GET_KILLSBYPC(ch));
  sprintf(buf, "%s", buf2);
  for (i=strdisplen(buf2); i<78; i++)
    strcat(buf, " ");
  strcat(buf, cstar);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
  /* Remort Levels - ARTUS */
  sprintf(buf2, "%s &0Remort Levels&n: &1One&n[&c%3d&n] &1Two&n[&c%3d&n] "
		"&1Max&n[&c%3d&n] &1Total&n[&c%3d&n]", 
	  cstar, GET_REM_ONE(ch), GET_REM_TWO(ch), GET_MAX_LVL(ch), 
      	  (GET_REM_ONE(ch) + GET_REM_TWO(ch) + GET_LEVEL(ch)));
  sprintf(buf, "%s", buf2);
  for(i=strlen(buf2); i<123; i++)
    strcat(buf, " ");
  strcat(buf, cstar);
  strcat(buf, "\r\n");
  if (GET_REM_ONE(ch) > 0)
    send_to_char(buf, ch);
  // creation date and invis info
  time_t ct;
  char *tmpstr;
  // Char creation date
  ct = ch->player.time.birth;
#ifndef NO_LOCALTIME
  tmpstr = asctime(localtime(&ct));
#else
  struct tm lt;
  if (jk_localtime(&lt, ch->player.time.birth))
    tmpstr = NULL;
  else
    tmpstr = asctime(&lt);
#endif
  sprintf(buf, "%s &0Creation Date&n: %s", cstar, tmpstr);
  // remove /n from asctime date...
  buf[strlen(buf)-1] = '\0';
  strcat(buf, "  ");
  if (GET_INVIS_LEV(ch) > 0)
  {
    if (GET_INVIS_TYPE(ch) == INVIS_SPECIFIC)
      sprintf(buf3,"&0Invis to &n[&c%s&n]", get_name_by_id(GET_INVIS_LEV(ch)));
    else if( GET_INVIS_TYPE(ch) == INVIS_SINGLE)
      sprintf(buf3,"&0Invis to Lvl &n[&c%ds&n]", GET_INVIS_LEV(ch)); 
    else if (GET_INVIS_TYPE(ch) == INVIS_NORMAL)
      sprintf(buf3,"&0Invis Lvl &n[&c%d&n]", GET_INVIS_LEV(ch));
    else
      sprintf(buf3,"&0Invis to Lvls &n[&c%d-%d&n]", GET_INVIS_LEV(ch), 
	      GET_INVIS_TYPE(ch)); 
    sprintf(buf2, " %s %s&n", cstar, buf3);
    strcat(buf, buf2);
    for (i=strlen(buf2); i<54; i++)
      strcat(buf, " ");
  } else {
    for (i=1; i < 36; i++)
      strcat(buf, " ");
  }
  
  strcat(buf, cstar);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

/* Third Line - ****** */
  send_to_char(cline,ch);

/* Fourth Line - Headings */
  strcpy(buf,"%s               &0Other&n              %s     &0Description&n        %s    &0Statistics&n   %s\r\n");
  sprintf(buf2, buf, cstar, cstar, cstar, cstar);
  send_to_char(buf2,ch);

/* Fifth Line - ****** */
  send_to_char(cline,ch);

  // DM - added class
  strcpy(buf,"");
  sprinttype(GET_CLASS(ch), pc_class_types, buf3);
  sprintf(buf,"%s &1Class&n        : %s%-18s&n%s &1Stat Points&n: &c%-9d &n%s"
	      "    &1Base App Max&n %s\r\n", 
	  cstar, CCTEXT(ch,C_NRM), buf3, cstar, GET_STAT_POINTS(ch), cstar,
	  cstar);

  //for (i=strlen(buf3); i<2; i++)
  //  strcat(buf2," ");
  //strcat(buf,buf2);

  //sprintf(buf2,"%s\r\n",cstar);
  //strcat(buf,buf2);
  send_to_char(buf,ch);

/* Sixth Line - Race, Sex, STR */
  strcpy(buf,"");
  sprinttype(GET_RACE(ch), pc_race_types, buf3);
  sprintf(buf2,"%s &1Race&n         : %s%s&n", cstar, CCTEXT(ch,C_NRM), buf3);
  for (i=strlen(buf3);i<18;i++)
    strcat(buf2," ");
  strcat(buf,buf2);
  sprintf(buf2,"%s &1Sex&n   : ", cstar);
  strcat(buf,buf2);

  switch (ch->player.sex)
  {
    case SEX_NEUTRAL:
      strcpy(buf2, "NEUTRAL");
      break;
    case SEX_MALE:
      strcpy(buf2, "MALE");
      break;
    case SEX_FEMALE:
      strcpy(buf2, "FEMALE");
      break;
    default:
      strcpy(buf2, "ILLEGAL");
      break;
  }  
  sprintf(buf3,"%s%s%s&n",CCTEXT(ch,C_NRM),buf2,CCTEXT(ch,C_NRM));
  strcat(buf,buf3);
  for (i=strlen(buf2); i<15; i++)
    strcat(buf," ");
  char tmp[80];
  /*
  if (GET_REAL_STR(ch)==18)
  {
     sprintf(buf3,"%s/%s%d",CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_REAL_ADD(ch));
     sprintf(tmp, "%d", GET_REAL_ADD(ch));
     j=1+(strlen(tmp));
  } else {
    strcpy(buf3,""); 
    j=0;
  }
  */
  sprintf(buf2,"%s &1STR&n: %s%-2d  %-2d  %-2d &n%s&n\r\n", cstar,
          CCSTAT(ch,C_NRM), GET_REAL_STR(ch), GET_AFF_STR(ch), 
	  max_stat_value(ch, STAT_STR), cstar);
  strcat(buf,buf2);
  /*
  sprintf(tmp, "%d", GET_REAL_STR(ch));
  j=j+strlen(tmp);

  if (GET_REAL_STR(ch)!=GET_AFF_STR(ch))
  {
    if (GET_AFF_STR(ch)==18) {
      sprintf(tmp, "%d", GET_REAL_ADD(ch));
      j=j+1+strlen(tmp);
      sprintf(buf3,"%s/%s%d&n",
		      CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_ADD(ch));
    } else 
      strcpy(buf3,""); 
    sprintf(buf2,"%s(%s%d%s%s)&n",
	CCSEP(ch,C_NRM),CCSTAT(ch,C_NRM),GET_AFF_STR(ch),buf3,CCSEP(ch,C_NRM));
    strcat(buf,buf2);
    sprintf(tmp, "%d", GET_AFF_STR(ch));
    j=j+2+strlen(tmp);
  }
  sprintf(buf2,"%s",CCNRM(ch,C_NRM));
  strcat(buf,buf2);
  for (i=j; i<11; i++)
    strcat(buf," ");
  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  */
  send_to_char(buf,ch);

/* Seventh Line - HPS, Age, Con */
// Choose the hp color .. Artus> Isn't their a function for this?
  if(GET_HIT(ch) < GET_MAX_HIT(ch) / 4)
    sprintf(buf2,"%s",CCRED(ch,C_NRM));
  else if(GET_HIT(ch) < GET_MAX_HIT(ch) / 2)
    sprintf(buf2,"%s",CCBRED(ch,C_NRM));
  else if(GET_HIT(ch) < GET_MAX_HIT(ch))
    sprintf(buf2,"%s",CCBYEL(ch,C_NRM));
  else sprintf(buf2,"%s",CCBGRN(ch,C_NRM));
  strcpy(buf,"");
  sprintf(buf, "%s &1Hit points&n   : %s%d%s/%s%d%s+&n%s%d&n", 
	cstar, buf2, GET_HIT(ch), CCSEP(ch,C_NRM), CCBGRN(ch,C_NRM),
	GET_MAX_HIT(ch), CCSEP(ch,C_NRM), CCGAIN(ch,C_NRM), hit_gain(ch));
  char tmp1[80], tmp2[80], tmp3[80];
  int tmpl1, tmpl2, tmpl3;
  sprintf(tmp1, "%d", GET_HIT(ch));
  sprintf(tmp2, "%d", GET_MAX_HIT(ch));
  sprintf(tmp3, "%d", hit_gain(ch));
  tmpl1 = strlen(tmp1);
  tmpl2 = strlen(tmp2);
  tmpl3 = strlen(tmp3);
  j = tmpl1 + tmpl2 + tmpl3 + 2;
  for (i=j; i<18; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1Age&n   : %s%d&n",
	cstar, CCNUMB(ch,C_NRM), age(ch)->year);
  strcat(buf,buf2);
  sprintf(tmp, "%d", age(ch)->year);
  for (i=strlen(tmp); i<15; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1CON&n: %s%-2d  %-2d  %-2d &n%s&n\r\n",
	cstar, CCSTAT(ch,C_NRM), GET_REAL_CON(ch), GET_AFF_CON(ch), 
	max_stat_value(ch, STAT_CON), cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Eighth line - Mana, Height/Weight, Dex */
  /* Choose the mana color */
  if(GET_MANA(ch) < GET_MAX_MANA(ch) / 4)
    sprintf(buf2,"%s",CCRED(ch,C_NRM));
  else if(GET_MANA(ch) < GET_MAX_MANA(ch) / 2)
    sprintf(buf2,"%s",CCBRED(ch,C_NRM));
  else if(GET_MANA(ch) < GET_MAX_MANA(ch))
    sprintf(buf2,"%s",CCBYEL(ch,C_NRM));
  else
    sprintf(buf2,"%s",CCBGRN(ch,C_NRM));  

  sprintf(buf, "%s &1Mana Points&n  : %s%d%s/%s%d%s+&n%s%d&n",
	  cstar, buf2, GET_MANA(ch), CCSEP(ch,C_NRM), CCBGRN(ch,C_NRM),
	  GET_MAX_MANA(ch), CCSEP(ch,C_NRM), CCGAIN(ch,C_NRM), mana_gain(ch));
  sprintf(tmp1, "%d", GET_MANA(ch));
  sprintf(tmp2, "%d", GET_MAX_MANA(ch));
  sprintf(tmp3, "%d", mana_gain(ch));
  tmpl1 = strlen(tmp1);
  tmpl2 = strlen(tmp2);
  tmpl3 = strlen(tmp3);
  j = tmpl1 + tmpl2 + tmpl3 + 2;
  for (i=j; i<18; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1Height%s/&1Weight&n: %s%d%s,&n%s%d&n", cstar,
          CCSEP(ch,C_NRM), CCNUMB(ch,C_NRM), GET_HEIGHT(ch), CCSEP(ch,C_NRM),
	  CCNUMB(ch,C_NRM), GET_WEIGHT(ch));
  strcat(buf,buf2);
  sprintf(tmp1, "%d", GET_HEIGHT(ch));
  sprintf(tmp2, "%d", GET_WEIGHT(ch));
  for (i=(strlen(tmp1) + strlen(tmp2)); i<7; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1DEX&n: %s%-2d  %-2d  %-2d &n%s&n\r\n", cstar,
	  CCSTAT(ch,C_NRM), GET_REAL_DEX(ch), GET_AFF_DEX(ch), 
	  max_stat_value(ch, STAT_DEX), cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

  /* Ninth Line - Movement, Level, Int */
  // Choose the movement color -- Artus> Function?
  if(GET_MOVE(ch) < GET_MAX_MOVE(ch) / 4)
    sprintf(buf2,"%s",CCRED(ch,C_NRM));
  else if(GET_MOVE(ch) < GET_MAX_MOVE(ch) / 2)
    sprintf(buf2,"%s",CCBRED(ch,C_NRM));
  else if(GET_MOVE(ch) < GET_MAX_MOVE(ch))
    sprintf(buf2,"%s",CCBYEL(ch,C_NRM));
  else sprintf(buf2,"%s",CCBGRN(ch,C_NRM));
  sprintf(buf, "%s &1Movement&n     : %s%d%s/%s%d%s+&n%s%d&n", cstar, buf2,
          GET_MOVE(ch), CCSEP(ch,C_NRM), CCBGRN(ch,C_NRM), GET_MAX_MOVE(ch),
	  CCSEP(ch,C_NRM), CCGAIN(ch,C_NRM), move_gain(ch));
  sprintf(tmp1, "%d", GET_MOVE(ch));
  sprintf(tmp2, "%d", GET_MAX_MOVE(ch));
  sprintf(tmp3, "%d", move_gain(ch));
  tmpl1 = strlen(tmp1);
  tmpl2 = strlen(tmp2);
  tmpl3 = strlen(tmp3);
  j = tmpl1 + tmpl2 + tmpl3 + 2;
  for (i=j; i<18; i++)
    strcat(buf," ");
  j=GET_LEVEL(ch);
  if (j >= LVL_GOD) // Artus> Function?
    sprintf(buf3,"%s",CCBYEL(ch,C_SPR));
  else if (j >= LVL_ANGEL)
    sprintf(buf3,"%s",CCCYN(ch,C_SPR));
  else if (j >= LVL_ISNOT_GOD)
    sprintf(buf3,"%s",CCRED(ch,C_SPR));
  else if (j >= LVL_CHAMP)
    sprintf(buf3,"%s",CCBLU(ch,C_SPR));
  else
    sprintf(buf3,"%s",CCNRM(ch,C_SPR));
  sprintf(buf2,"%s &1Level&n : %s%d&n",
	cstar, buf3, GET_LEVEL(ch));
  strcat(buf,buf2);
  sprintf(tmp, "%d", j);
  for (i=strlen(tmp); i<15; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1INT&n: %s%-2d  %-2d  %-2d &n%s&n\r\n",
	cstar, CCSTAT(ch,C_NRM), GET_REAL_INT(ch), GET_AFF_INT(ch), 
	max_stat_value(ch, STAT_INT), cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Tenth line - Hitroll, Align, Wisdom */
  sprintf(buf, "%s &1Hitroll&n      : %s%d&n", cstar, 
          CCDH(ch,C_NRM), ch->points.hitroll);
  sprintf(tmp, "%d", ch->points.hitroll);
  for (i=strlen(tmp); i<18; i++)
    strcat(buf," ");
  strcpy(alignbuf,"");
  // Artus> Function?
  if ((GET_ALIGNMENT(ch)>-101)&&(GET_ALIGNMENT(ch)<101))
    strcpy(alignbuf,"Neutral");
  if ((GET_ALIGNMENT(ch)>100)&&(GET_ALIGNMENT(ch)<301))
    strcpy(alignbuf,"NeutralG");
  /* Good alignment messages */
  if ((GET_ALIGNMENT(ch)>300)&&(GET_ALIGNMENT(ch)<401))
   strcpy(alignbuf,"Fair");
  if ((GET_ALIGNMENT(ch)>400)&&(GET_ALIGNMENT(ch)<501))
   strcpy(alignbuf,"Kind");
  if ((GET_ALIGNMENT(ch)>500)&&(GET_ALIGNMENT(ch)<601))
   strcpy(alignbuf,"Friendly");
  if ((GET_ALIGNMENT(ch)>600)&&(GET_ALIGNMENT(ch)<701))
   strcpy(alignbuf,"Honest");
  if ((GET_ALIGNMENT(ch)>700)&&(GET_ALIGNMENT(ch)<801))
   strcpy(alignbuf,"Humane");
  if ((GET_ALIGNMENT(ch)>800)&&(GET_ALIGNMENT(ch)<901))
   strcpy(alignbuf,"Virtuous");
  if ((GET_ALIGNMENT(ch)>901))
   strcpy(alignbuf,"Angelic");
  /* Evil Ratings */
  if ((GET_ALIGNMENT(ch)<-100)&&(GET_ALIGNMENT(ch)>-300))
   strcpy(alignbuf,"NeutralE");
  if ((GET_ALIGNMENT(ch)<-300)&&(GET_ALIGNMENT(ch)>-401))
   strcpy(alignbuf,"Unfair");
  if ((GET_ALIGNMENT(ch)<-400)&&(GET_ALIGNMENT(ch)>-501))
   strcpy(alignbuf,"Mean");
  if ((GET_ALIGNMENT(ch)<-500)&&(GET_ALIGNMENT(ch)>-601))
   strcpy(alignbuf,"Wicked");
  if ((GET_ALIGNMENT(ch)<-600)&&(GET_ALIGNMENT(ch)>-701))
   strcpy(alignbuf,"Sinful");
  if ((GET_ALIGNMENT(ch)<-700)&&(GET_ALIGNMENT(ch)>-801))
   strcpy(alignbuf,"Villanous");
  if ((GET_ALIGNMENT(ch)<-800)&&(GET_ALIGNMENT(ch)>-901))
   strcpy(alignbuf,"Demonic");
  if ((GET_ALIGNMENT(ch)<-901))
   strcpy(alignbuf,"Satanic");
  // Artus> Function?
  if (GET_ALIGNMENT(ch) > 350)
    sprintf(buf3,"%s",CCCYN(ch,C_NRM));
  else if (GET_ALIGNMENT(ch) < -350)
    sprintf(buf3,"%s",CCRED(ch,C_NRM));
  else
    sprintf(buf3,"%s",CCBWHT(ch,C_NRM));
  sprintf(buf2,"%s &1Align&n : %s%s%s(%s%d%s)&n",
	cstar, buf3, alignbuf, CCSEP(ch,C_NRM), buf3,
	GET_ALIGNMENT(ch), CCSEP(ch,C_NRM));
  strcat(buf,buf2);
  sprintf(tmp, "%d", GET_ALIGNMENT(ch));
  j=2+strlen(alignbuf)+strlen(tmp);
  for (i=j; i<15; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1WIS&n: %s%-2d  %-2d  %-2d &n%s&n\r\n",
	  cstar, CCSTAT(ch,C_NRM), GET_REAL_WIS(ch), GET_AFF_WIS(ch), 
	  max_stat_value(ch, STAT_WIS), cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

  /* Eleventh line - Damroll, gold, CHA */
  sprintf(buf, "%s &1Damroll&n      : %s%d", cstar, CCDH(ch,C_NRM),
          ch->points.damroll);
  // MINOTAUR and SUPERMAN bonus display
  int ctmp = 0;
  if (IS_SET(GET_SPECIALS(ch), SPECIAL_MINOTAUR))
  {
    sprintf(buf, "%s+%d", buf, MAX(5, (int)(GET_DAMROLL(ch) * 0.1)));
    ctmp += MAX(5, digits((long)(GET_DAMROLL(ch) * 0.1)));
  }
  if (IS_SET(GET_SPECIALS(ch), SPECIAL_SUPERMAN))
  {
    sprintf(buf, "%s+%d", buf, (int)(GET_DAMROLL(ch) * 0.02));
    ctmp += 1 + digits((long)(GET_DAMROLL(ch) * 0.02));
  }
  sprintf(tmp, "%d", ch->points.damroll);
  for (i=(strlen(tmp)+ctmp); i<18;i++)
    strcat(buf," ");
  sprintf(buf2,"&n%s &1Gold&n  : &Y%d&n", cstar, GET_GOLD(ch));
  strcat(buf,buf2);
  sprintf(tmp, "%d", GET_GOLD(ch));
  for (i=strlen(tmp); i<15; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1CHA&n: %s%-2d  %-2d  %-2d &n%s&n\r\n",
	  cstar, CCSTAT(ch,C_NRM), GET_REAL_CHA(ch), GET_AFF_CHA(ch), 
	  max_stat_value(ch, STAT_CHA), cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Twelth line - Armor Class, Bank */
  // TODO: fix armour class
  sprintf(buf, "%s &1Armour Class&n : %s%d%s/&n%s10 %s(&n%s%d%s)&n", cstar,
          CCACT(ch,C_NRM), compute_armor_class(ch, 0), CCSEP(ch,C_NRM),
	  CCACT(ch,C_NRM), CCSEP(ch,C_NRM), CCACT(ch,C_NRM),
	  compute_armor_class(ch, 1), CCSEP(ch, C_NRM));
  for(i=strdisplen(buf); i<35; i++)
    strcat(buf, " ");
  sprintf(buf2,"%s &1Bank&n  : &Y%d&n", cstar, GET_BANK_GOLD(ch));
  strcat(buf,buf2);
  sprintf(tmp, "%d", GET_BANK_GOLD(ch));
  for (i=strlen(tmp); i<15; i++)
    strcat(buf," ");
  sprintf(buf2,"%s",cstar);
  strcat(buf,buf2);
  for (i=1; i<18; i++)
    strcat(buf," ");
  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Thirteenth line - Thac0, Time */
  sprintf(buf, "%s &1Thac&n%s0&n        : %s%d&n", cstar, CCACT(ch,C_NRM),
          CCACT(ch,C_NRM), thaco(ch, NULL)); 
  sprintf(tmp, "%d", thaco(ch, NULL));
  for (i=strlen(tmp); i<18; i++)
    strcat(buf," ");
  playing_time = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
  sprintf(buf2,"%s &1Time&n  : %s%d&n%sd&n%s%d%sh&n", cstar, CCNUMB(ch,C_NRM),
          playing_time.day, CCSEP(ch,C_NRM), CCNUMB(ch,C_NRM),
	  playing_time.hours, CCSEP(ch,C_NRM));
  strcat(buf,buf2);
  for (i=(digits(playing_time.hours)+digits(playing_time.day)+2); i<15; i++)
    strcat(buf," ");
  sprintf(buf2,"%s",cstar);
  strcat(buf,buf2);
  for (i=1; i<18; i++)
    strcat(buf," ");
  sprintf(buf2,"%s\r\n",cstar);
  strcat(buf,buf2);
  send_to_char(buf,ch);

/* Fourteenth line - ***** */
  send_to_char(cline, ch);  
  
/* Fifteenth line - Carrying Weight, Carrying Items, Can Carry Weight, Can Carry Items */
  sprintf(buf,"%s              &0Inventory                                     %s                 &n%s&n\r\n", cstar,CCHEAD(ch,C_NRM),cstar);
  send_to_char(buf,ch);
  send_to_char(cline,ch);
  sprintf(buf,"%s &1Carrying weight &n: %s%d&n", cstar, CCNUMB(ch,C_NRM),
          IS_CARRYING_W(ch));
  for (i=digits(IS_CARRYING_W(ch)); i<11; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1Carrying Items &n: %s%d&n",
                cstar, CCNUMB(ch,C_NRM), IS_CARRYING_N(ch));
  for (i=digits(IS_CARRYING_N(ch)); i < 10; i++)
    strcat(buf2," ");
  strcat(buf2,cstar);
  for (i=0; i < 17; i++)
    strcat(buf2, " ");
  strcat(buf2,cstar);
  strcat(buf2,"\r\n");
  strcat(buf,buf2);
  send_to_char(buf,ch);
  sprintf(buf,"%s &1Can Carry Weight&n: %s%d&n", cstar, CCNUMB(ch,C_NRM),
          CAN_CARRY_W(ch));
  for (i=digits(CAN_CARRY_W(ch)); i<11; i++)
    strcat(buf," ");
  sprintf(buf2,"%s &1Can Carry Items&n: %s%d&n",
                cstar, CCNUMB(ch,C_NRM), CAN_CARRY_N(ch));
  for (i=digits(CAN_CARRY_N(ch)); i < 10; i++)
    strcat(buf2," ");
  strcat(buf2,cstar);
  for (i=0; i < 17; i++)
    strcat(buf2, " ");
  strcat(buf2,cstar);
  sprintf(buf3,"%s\r\n",CCNRM(ch,C_NRM));
  strcat(buf2,buf3);
  strcat(buf,buf2);
  send_to_char(buf,ch);
  send_to_char(cline, ch);
  
  // DM - new score information
  /*
  struct affected_type *aff;
  // Routine to show what spells a char is affected by *
  if (ch->affected)
  {
    for (aff = ch->affected; aff; aff = aff->next)
    {
      *buf2 = '\0';
      if( aff->duration == -1 )
      {
        sprintf(buf, "ABL: (Unlim) &c%-21s &n ", skill_name(aff->type));
        if (aff->modifier)
	{
          sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
          strcat(buf, buf2);
        }
      } else {
        // ROD - here make Unlim for perm spells from eq
        // It appears the affect is removed when time is 1,
        // spell affects from magic eq are given with time 0
        // for abilities they are given time -1
	//
        // I hate this dodgy code of vaders ....
        // ok go through the eq list and find if the affect is given by eq
	//
        // This code is DUPLICATED in act.informative.c for affects
        bool found = FALSE;
        for (int i = 0; i < NUM_WEARS; i++)
	{
          if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_MAGIC_EQ)
	  {
            for (int j = 0; j < 3; j++)
	    {
              if (GET_OBJ_VAL(GET_EQ(ch, i), j) == aff->type)
	      {
                found = TRUE;
                break;
              }
            }
          }
        }         
        if (found)
          sprintf(buf, "SPL: (Unlim) %s%-21s%s ", CCCYN(ch, C_NRM),
		  skill_name(aff->type), CCNRM(ch, C_NRM));
        else
          sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1,
		  CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));
        if (aff->modifier)
          sprintf(buf+strlen(buf), "%+d to %s", aff->modifier,
	          apply_types[(int) aff->location]);
      }      
      if (aff->bitvector)
      {
	if (*buf2)
	  strcat(buf, ", sets ");
	else
	  strcat(buf, "sets ");
	sprintbit(aff->bitvector, affected_bits, buf2);
	strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }
  struct timer_type *timer;
  // Routine to show what timers a char is affected by 
  if (ch->timers)
  {
    for (timer = ch->timers; timer; timer = timer->next)
    {
      sprintf(buf, "TIM: (%3dhr) %s%-22s%s", timer->duration, CCCYN(ch, C_NRM),
              timer_types[timer->type], CCNRM(ch, C_NRM));
      sprintf(buf+strlen(buf), "Uses: (%2d) of max: (%2d)\r\n",timer->uses,
              timer->max_uses);
      send_to_char(buf,ch);
    } 
  }
  */

/* Sixteenth line - ***** */
 /* 
  if ( !GET_CLAN_NUM (ch) < 1 ) 
	sprintf(buf, "You are part of the %s and your rank is %s.\r\n",
         get_clan_disp(ch), get_clan_rank(ch));
  else
	sprintf(buf, "You are clanless.\r\n" );
*/
  //send_to_char(cline, ch);
  // EXP TNL
  if (CAN_LEVEL(ch))
    sprintf(buf3, "You have earned &c%d&n exp, and need &c%d&n to reach level &c%d&n.\r\n", GET_EXP(ch), level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch),
	    GET_LEVEL(ch)+1);
//  sprintf(buf, "Social Points: %d, Social Rank: %s\r\n", GET_SOCIAL_POINTS(ch),
//	social_ranks[GET_SOCIAL_STATUS(ch)]);
//  send_to_char(buf, ch);

  /* Punishment Bits - ARTUS */
  unsigned int i2 = 0, i3 = 0;
  bool bad = FALSE;
  sprintf(buf2, "%s &rPunishments&n:", cstar);
  for (i = 0; i < NUM_PUNISHES; i++)
    if (PUN_FLAGGED(ch, i)) 
    {
      sprintf(buf2, "%s &g%s&n[&c%d&n]", 
		      buf2, punish_types[i], PUN_HOURS(ch, i));
      i2 = 1;
      i3++;
    }
  if (i2 != 0) 
  {           // colour code chars (8 per punishment)
    strcpy(buf, buf2);
    if (strlen(buf2) < (92 + 8*i3))
      for (i=1; i < (92 + 8*i3 - strlen(buf2)); i++)
        strcat(buf, " ");
    strcat(buf, cstar);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    bad = TRUE;
  }
  /* Offence Counts - ARTUS */
  i2 = 0;
  i3 = 0;
  sprintf(buf2, "%s &rOffences&n:", cstar);
  for (i = 0; i < NUM_OFFENCES; i++)
    if (HAS_OFFENDED(ch,i) > 0)
    {
      sprintf(buf2, "%s &g%s&n[&c%d&n]", buf2, offence_types[i], HAS_OFFENDED(ch,i));
      i2 = 1;
      i3++;
    }
  if (i2 != 0) 
  {           // colour code chars (8 per offence)
    strcpy(buf, buf2);
    if (strlen(buf2) < (95 + 8*i3)) 
      for (i=1; i < (95 + 8*i3 - strlen(buf2)); i++)
        strcat(buf, " ");
    strcat(buf, cstar);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    bad = TRUE;
  }
  if (bad)
    send_to_char(cline, ch);
  strcpy(buf,"");  
  if (GET_LEVEL(ch))
  {
    sprintf(buf, "You need &c%d&n exp to reach your next level.\r\n", 
	(level_exp(ch, (int)GET_LEVEL(ch))) - GET_EXP(ch));
      send_to_char(buf, ch);  
  }

  strcpy(buf,"");
  switch (GET_POS(ch))
  {
    case POS_DEAD:
      strcat(buf, "You are DEAD!\r\n");
      break;
    case POS_MORTALLYW:
      strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
      break;
    case POS_INCAP:
      strcat(buf, "You are incapacitated, slowly fading away...\r\n");
      break;
    case POS_STUNNED:
      strcat(buf, "You are stunned!  You can't move!\r\n");
      break;
    case POS_SLEEPING:
      strcat(buf, "You are sleeping.\r\n");
      break;
    case POS_RESTING:
      strcat(buf, "You are resting.\r\n");
      break;
    case POS_SITTING:
      strcat(buf, "You are sitting.\r\n");
      break;
    case POS_FIGHTING:
      if (FIGHTING(ch))
	sprintf(buf, "%sYou are fighting %s.\r\n", buf, PERS(FIGHTING(ch), ch));
      else
	strcat(buf, "You are fighting thin air.\r\n");
      break;
    case POS_STANDING:
      strcat(buf, "You are standing.\r\n");
      break;
    default:
      strcat(buf, "You are floating.\r\n");
      break;
  }
  if (EXT_FLAGGED(ch, EXT_AUTOLOOT))
    strcat(buf, "You are autolooting corpses.\r\n");
  if (EXT_FLAGGED(ch, EXT_AUTOGOLD)) 
  {
    strcat(buf, "You are autolooting gold from corpses.\r\n");
    if (EXT_FLAGGED(ch, EXT_AUTOSPLIT))
      strcat(buf, "You are autospliting gold between group members.\r\n");
  }
  if (EXT_FLAGGED(ch, EXT_AUTOEAT))
    strcat(buf, "You will automatically eat when you're hungry.\r\n");
  if (AUTOASSIST(ch))
    sprintf(buf+strlen(buf), "You are autoassisting &7%s&n.\r\n",
	    GET_NAME(AUTOASSIST(ch)));
  if (GET_COND(ch, DRUNK) > 10)
    strcat(buf, "You are intoxicated.\r\n");
  if (GET_COND(ch, FULL) == 0)
    strcat(buf, "You are hungry.\r\n");
  if (GET_COND(ch, THIRST) == 0)
    strcat(buf, "You are thirsty.\r\n");
  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");
  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");
  if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");
  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");
  if (IS_AFFECTED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");
  if (IS_AFFECTED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");
  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");
  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing &rred&n.\r\n");
  if (IS_AFFECTED(ch, AFF_WATERWALK))
    strcat(buf, "You have magically webbed feet.\r\n");
  if (IS_AFFECTED(ch, AFF_WATERBREATHE))
    strcat(buf, "You can breathe underwater.\r\n");
  if (IS_AFFECTED(ch, AFF_REFLECT))
    strcat(buf, "You have shiny scales on your skin.\r\n");
  if (IS_AFFECTED(ch, AFF_FLY))
    strcat(buf, "You are flying.\r\n");
  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");
  if (MOUNTING(ch))
    sprintf(buf+strlen(buf), "You are mounted on &6%s&n.\r\n",
	    GET_NAME(MOUNTING(ch)));
  if (affected_by_spell(ch,SPELL_CHANGED))
    if(PRF_FLAGGED(ch,PRF_WOLF))
      strcat(buf, "You're a Werewolf!\r\n");
    else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
      strcat(buf, "You're a Vampire!\r\n");
  send_to_char(buf, ch);  
}

ACMD(do_score)
{
  if (IS_NPC(ch))
  {
    send_to_char("You are     : Inflatable\r\n"
		 "Occupation  : Goat\r\n"
		 "Inclination : Left 10 degrees\r\n", ch);
    return;
  }
  primal_score(ch, argument);

#if 0 // Artus> Unused.
  // Show old score if no settings defined
  //if (SCORE_SETTINGS(ch) || *argument)
  //{
  //	return;
  //}


  /** Ignore this crap
  sprintf(buf, "You are %d years old.", GET_AGE(ch));

  if (age(ch)->month == 0 && age(ch)->day == 0)
    strcat(buf, "  It's your birthday today.\r\n");
  else
    strcat(buf, "\r\n");

  sprintf(buf + strlen(buf),
       "You have %d(%d) hit, %d(%d) mana and %d(%d) movement points.\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));



  sprintf(buf + strlen(buf), "Your armor class is %d/10, and your alignment is %d.\r\n",
	  compute_armor_class(ch, 0), GET_ALIGNMENT(ch));


  sprintf(buf + strlen(buf), "You have scored %d exp, and have %d gold coins.\r\n",
	  GET_EXP(ch), GET_GOLD(ch));

  if (GET_LEVEL(ch) < LVL_IMMORT)
    sprintf(buf + strlen(buf), "You need %d exp to reach your next level.\r\n",
	level_exp(ch, GET_LEVEL(ch)) - GET_EXP(ch));

  playing_time = *real_time_passed((time(0) - ch->player.time.logon) +
				  ch->player.time.played, 0);
  sprintf(buf + strlen(buf), "You have been playing for %d day%s and %d hour%s.\r\n",
     playing_time.day, playing_time.day == 1 ? "" : "s",
     playing_time.hours, playing_time.hours == 1 ? "" : "s");

  sprintf(buf + strlen(buf), "This ranks you as %s %s (level %d).\r\n",
	  GET_NAME(ch), GET_TITLE(ch), GET_LEVEL(ch));

  switch (GET_POS(ch)) {
  case POS_DEAD:
    strcat(buf, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    strcat(buf, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    strcat(buf, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    strcat(buf, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    strcat(buf, "You are resting.\r\n");
    break;
  case POS_SITTING:
    strcat(buf, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    if (FIGHTING(ch))
      sprintf(buf + strlen(buf), "You are fighting %s.\r\n",
		PERS(FIGHTING(ch), ch));
    else
      strcat(buf, "You are fighting thin air.\r\n");
    break;
  case POS_STANDING:
    strcat(buf, "You are standing.\r\n");
    break;
  default:
    strcat(buf, "You are floating.\r\n");
    break;
  }

  if (EXT_FLAGGED(ch, EXT_AUTOLOOT))
    strcat(buf, "You are autolooting corpses.\r\n");
 
  if (EXT_FLAGGED(ch, EXT_AUTOGOLD)) {
    strcat(buf, "You are autolooting gold from corpses.\r\n");
    if (EXT_FLAGGED(ch, EXT_AUTOSPLIT))
      strcat(buf, "You are autospliting gold between group members.\r\n");
  }
 
  if (AUTOASSIST(ch)) {
    sprintf(buf2, "You are autoassisting %s.\r\n",GET_NAME(AUTOASSIST(ch)));
    strcat(buf,buf2);
  } 

  if (GET_COND(ch, DRUNK) > 10)
    strcat(buf, "You are intoxicated.\r\n");

  if (GET_COND(ch, FULL) == 0)
    strcat(buf, "You are hungry.\r\n");

  if (GET_COND(ch, THIRST) == 0)
    strcat(buf, "You are thirsty.\r\n");

  if (AFF_FLAGGED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");

  if (AFF_FLAGGED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");

  if (AFF_FLAGGED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");

  if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");

  send_to_char(buf, ch);

  // Show them their abilities
  show_ability_messages_to_char(ch);
  */
#endif
}

ACMD(do_timers)
{
  struct timer_type *timer;
  /* Routine to show what timers a char is affected by */
  if (ch->timers) 
  {
    send_to_char("Timers currently running:\r\n", ch);
    for (timer = ch->timers; timer; timer = timer->next) 
    {

      sprintf(buf,"TIM: (%3dhr) %s%-22s%s", timer->duration, CCCYN(ch, C_NRM), timer_types[timer->type], CCNRM(ch, C_NRM));
      sprintf(buf2,"Uses: (%2d) of max: (%2d)\r\n",timer->uses, timer->max_uses);

      strcat(buf, buf2);

      send_to_char(buf,ch);
    } 
  } else
    send_to_char("You are currently not affected by any timers.\r\n", ch);
}

ACMD(do_affects)
{
  struct affected_type *aff;
 
  if (IS_NPC(ch))
    return;
    
  strcpy(buf,"");
// DM - TODO - check what we want to be given in text
  /*
  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");
  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");
  if (IS_AFFECTED(ch, AFF_WATERWALK))
    strcat(buf, "You have magically webbed feet.\r\n");
  if (IS_AFFECTED(ch, AFF_WATERBREATHE))
    strcat(buf, "You can breathe underwater.\r\n");
  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");
  if (affected_by_spell(ch, SPELL_STRENGTH))
    strcat(buf, "You feel stronger.\r\n");
  if (affected_by_spell(ch, SPELL_CHILL_TOUCH))
    strcat(buf, "You feel weakened.\r\n");
  if (IS_AFFECTED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");
  if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");
  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");
  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");
  if (IS_AFFECTED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");
  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");
  if (affected_by_spell(ch, SPELL_BLESS))
    strcat(buf, "You feel righteous.\r\n");
  if (IS_AFFECTED(ch, AFF_CURSE))
    strcat(buf, "You are cursed.\r\n");
  if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
    strcat(buf, "You feel more aware.\r\n");
  if (IS_AFFECTED(ch, AFF_SNEAK))
    strcat(buf, "You feel more sneaky.\r\n");
  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN))
    strcat(buf, "You have the ability to see auras.\r\n");
  if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
    strcat(buf, "You are more sensitive to magic.\r\n");
  if (IS_AFFECTED(ch, AFF_PROTECT_EVIL))
    strcat(buf, "You feel protected from Evil.\r\n");
  if (IS_AFFECTED(ch, AFF_PROTECT_GOOD))
    strcat(buf, "You feel protected from Good.\r\n");
  if (IS_AFFECTED(ch, AFF_SLEEP))
    strcat(buf, "You are asleep.\r\n");
  if (IS_AFFECTED(ch, AFF_REFLECT))
    strcat(buf, "You have shiny scales on your skin.\r\n");
  if (IS_AFFECTED(ch, AFF_FLY))
    strcat(buf, "You are flying.\r\n");
  if (affected_by_spell(ch,SPELL_DRAGON))
    strcat(buf, "You feel like a dragon!\r\n");
  if (affected_by_spell(ch,SPELL_CHANGED))
    if(PRF_FLAGGED(ch,PRF_WOLF))
      strcat(buf, "You're a Werewolf!\r\n");
    else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
      strcat(buf, "You're a Vampire!\r\n");
  if (IS_AFFECTED(ch, AFF_PARALYZED))
     strcat(buf, "You are paralyzed.\r\n");
*/
  if (MOUNTING(ch))
    sprintf(buf+strlen(buf), "You are mounted on %s.\r\n",
	    GET_NAME(MOUNTING(ch)));
  if (MOUNTING_OBJ(ch))
    sprintf(buf+strlen(buf), "You are mounted on %s.\r\n",
	    MOUNTING_OBJ(ch)->short_description);
  send_to_char(buf, ch);
  /*
   * Rod - now level 5 restricted only
   *
        if ((GET_LEVEL(ch) < 20) && (GET_CLASS(ch) <= CLASS_WARRIOR))
                return; // Artus - Let them see the lot after remort too.
  */
  if (!ch->affected) 
  {
    send_to_char("You are currently not affected by any spells or abilities.\r\n", ch);
    return;
  }
  if (ch->affected) 
  {
    int noAffects = 0, i = 0;
    struct affected_type *affect_array[MAX_AFFECT];
    /* Artus> Couldn't see any reason not to move this to the next loop..
     * for (aff = ch->affected; aff; aff = aff->next) 
     *   noAffects++;
    */

    // Artus> Send some sort of header.
    send_to_char("You are affected by the following spells and/or skills:\r\n",
	         ch);
    for (aff = ch->affected; aff; aff = aff->next) 
    {
      noAffects++;
      *buf2 = '\0';
      affect_array[i++] = aff;
    }
    // sort array
    qsort(affect_array, noAffects, sizeof(struct affected_type *), compareAffectDuration);
    /*
    // debug:
    //basic_mud_loglog("i = %d, noAffects = %d", i, noAffects);
    for (aff = affect_array[0]; (aff) && i < noAffects; aff = affect_array[i++])
      basic_mud_log("Affect (%d) = %d %s", i, aff->type, skill_name(aff->type));
    */
    // Reset counter to 0 ...
    // now step through the sorted array ...
    for (aff = affect_array[0], i = 0; (aff) && i < noAffects;
	 aff = affect_array[++i])
    {
      if ((aff->type <= 0) || (aff->type > TOP_SPELL_DEFINE))
        continue;
      *buf2 = '\0';
      if (aff->duration == CLASS_ABILITY) 
        sprintf(buf, "ABL: (Unlim) &c%-21s&n ", skill_name(aff->type));
      else if (aff->duration == CLASS_ITEM)
	sprintf(buf, "OBJ: (Unlim) &c%-21s&n ", skill_name(aff->type));
      else
      {
        // ROD - here make Unlim for perm spells from eq
	// It appears the affect is removed when time is 1,
	// spell affects from magic eq are given with time 0
	// for abilities they are given time -1
	// I hate this dodgy code of vaders ....
	// ok go through the eq list and find if the affect is given by eq

	// This code is DUPLICATED in act.wizard.c for stat
	/* Artus> This is no longer necessary.
	bool found = FALSE;
        for (int i = 0; i < NUM_WEARS; i++)
          if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_MAGIC_EQ)
            for (int j = 0; j < 3; j++)
              if (GET_OBJ_VAL(GET_EQ(ch, i), j) == aff->type)
	      {
                found = TRUE;
                break;  
              }
        
        if (found) 
          sprintf(buf, "SPL: (Unlim) %s%-21s%s ", CCCYN(ch, C_NRM), 
              skill_name(aff->type), CCNRM(ch, C_NRM));
        else */
	sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1, 
		CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));
      } 
      strcpy(buf2, "");
      if (aff->modifier)
        sprintf(buf+strlen(buf), "%+d to %s", aff->modifier,
	        apply_types[(int) aff->location]);
      if (aff->bitvector)
      {
        if (*buf2)
          strcat(buf, ", sets ");
        else
          strcat(buf, "sets ");
        sprintbit(aff->bitvector, affected_bits, buf2);
        strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }
}

ACMD(do_exp)
{
  if (IS_NPC(ch))
    return;
  // Separate message for immorts.
  if (!LR_FAIL(ch, LVL_ISNOT_GOD))
  {
    send_to_char("You have no need for experience, you are immortal!\r\n", ch);
    return;
  }
  // No room to level, but remort is an option..
  if (!(CAN_LEVEL(ch)))
  {
    if (GET_CLASS(ch) < CLASS_MASTER)
      send_to_char("You have reached your classes maximum level. You might consider remorting.\r\n", ch);
    else
      send_to_char("You cannot gain further experience without great unholiness.\r\n", ch);
    return;
  }
  sprintf(buf, "With &c%d&n exp, you need &c%d&n exp to reach your next level.\r\n", GET_EXP(ch), level_exp(ch,GET_LEVEL(ch)) - GET_EXP(ch));
  send_to_char(buf, ch);
  return;
} 

ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
  int i, j, found = 0, okpos = 0;
  int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);

  one_argument(argument, arg);
  if (*arg)
  {
    if (is_abbrev(arg, "wield"))
      i = WEAR_WIELD;
    else if (is_abbrev(arg, "hold") || is_abbrev(arg, "held"))
      i = WEAR_HOLD;
    else if (is_abbrev(arg, "light"))
      i = WEAR_LIGHT;
    else if (is_abbrev(arg, "empty"))
    { // List all the empty bits.
      for (i = 0; i < NUM_WEARS; i++)
      {
	if (GET_EQ(ch, i))
	  continue;
	if (!eq_pos_ok(ch, i))
	  continue;
	if ((i == WEAR_SHIELD) && IS_DUAL_WIELDING(ch))
	  continue;
	send_to_char(where[i], ch);
	send_to_char("Nothing.\r\n", ch);
	found++;
      }
      if (found < 1)
	send_to_char("You don't have any empty wear positions!\r\n", ch);
      return;
    } else 
      i = find_eq_pos(ch, NULL, arg);
    if (i < 0) 
      return;
    switch (i)
    {
      case WEAR_LIGHT:
      case WEAR_BODY:
      case WEAR_HEAD:
      case WEAR_LEGS:
      case WEAR_FEET:
      case WEAR_HANDS:
      case WEAR_ARMS:
      case WEAR_SHIELD:
      case WEAR_ABOUT:
      case WEAR_WAIST:
      case WEAR_EYES: // The easy bits.
	if (!GET_EQ(ch, i))
	{
	  if (!eq_pos_ok(ch, i))
	    send_to_char("You can't wear anything there!\r\n", ch);
	  else
	    send_to_char("You're not using anything there!\r\n", ch);
	  return;
	}
        send_to_char(where[i], ch);
        if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
	  show_obj_to_char(GET_EQ(ch, i), ch, 1);
	else
  	  send_to_char("Something.\r\n", ch);
        return;
      case WEAR_FINGER_1:
	for (i = 1; i < 6; i++)
	{
	  j = wear_positions[i];
	  if (GET_EQ(ch, j))
	  {
	    okpos++;
	    found++;
	    send_to_char(where[j], ch);
	    if (CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
	      show_obj_to_char(GET_EQ(ch, j), ch, 1);
	    else
	      send_to_char("Something.\r\n", ch);
	  } else if (eq_pos_ok(ch, j))
	    okpos++;
	}
	if (okpos < 1) 
	  send_to_char("You can't wear anything there!\r\n", ch);
	if (found < okpos)
	{
	  sprintf(buf, "You have%s %d finger%s available.\r\n",
	          (found < 1) ? ((okpos > 1) ? " all" : " your") : "", 
		  okpos - found, (okpos - found == 1) ? "" : "s");
	  send_to_char(buf, ch);
	}
	return;
      case WEAR_NECK_1:
      case WEAR_WRIST_R:
      case WEAR_EAR_1:
      case WEAR_ANKLE_1:
	for (j = i; j <= i+1; j++)
	  if (GET_EQ(ch, j))
	  {
	    okpos++;
	    found++;
	    send_to_char(where[j], ch);
	    if (CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
	      show_obj_to_char(GET_EQ(ch, j), ch, 1);
	    else
	      send_to_char("Something.\r\n", ch);
	  } else if (eq_pos_ok(ch, j))
	    okpos++;
	if (okpos < 1)
	  send_to_char("You can't wear anything there!\r\n", ch);
	else if (found < okpos)
	{
	  sprintf(buf, "You can wear %d %sthing%s there!\r\n",
	          okpos - found, (found > 0) ? "more " : "", 
		  (okpos - found) == 1 ? "" : "s");
	  send_to_char(buf, ch);
	}
	return;
      case WEAR_HOLD:
	if (!IS_DUAL_WIELDING(ch))
	{
	  if (GET_EQ(ch, WEAR_HOLD))
	  {
	    okpos++;
	    found++;
	    send_to_char(where[WEAR_HOLD], ch);
	    if (CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD)))
	      show_obj_to_char(GET_EQ(ch, WEAR_HOLD), ch, 1);
	    else
	      send_to_char("Something.\r\n", ch);
	  } else if (eq_pos_ok(ch, WEAR_HOLD))
	    okpos++;
	}
	if (GET_EQ(ch, WEAR_LIGHT))
	{
	  okpos++;
	  found++;
	  send_to_char(where[WEAR_LIGHT], ch);
	  if (CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_LIGHT)))
	    show_obj_to_char(GET_EQ(ch, WEAR_LIGHT), ch, 1);
	  else
	    send_to_char("Something.\r\n", ch);
	} else if (eq_pos_ok(ch, WEAR_LIGHT))
	  okpos++;
	if (okpos < 1)
	{
	  if (IS_DUAL_WIELDING(ch))
	    send_to_char("You are dual wielding, you can't hold anything!\r\n", ch);
	  else
	    send_to_char("You can't hold anything!\r\n", ch);
	}
	if (found < 1)
	  send_to_char("You're not holding anything.\r\n", ch);
	return;
      case WEAR_WIELD:
	if (!GET_EQ(ch, i))
	{
	  if (!eq_pos_ok(ch, i))
	    send_to_char("You can't weild anything!\r\n", ch);
	  else
	    send_to_char("You're not wielding anything!\r\n", ch);
	  return;
	}
        send_to_char(where[i], ch);
        if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
	  show_obj_to_char(GET_EQ(ch, i), ch, 1);
	else
  	  send_to_char("Something.\r\n", ch);
	if (IS_DUAL_WIELDING(ch))
	{
	  send_to_char("<wielded (2nd)>      ", ch);
	  if (CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD)))
	    show_obj_to_char(GET_EQ(ch, WEAR_HOLD), ch, 1);
	  else
	    send_to_char("Something.\r\n", ch);
	}
	return;
    }
  }
  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++)
  {
    // Hide unavailable positions, unless the char happens to be wearing
    // something on that position
    if (eq_pos_ok(ch, wear_positions[i]) || GET_EQ(ch, wear_positions[i]))
    {
      // Wielding weapon in hold position - display "<wielded>"
//      if (i == WEAR_HOLD && 
//	  (GET_EQ(ch, i) && CAN_WEAR(GET_EQ(ch, i), WEAR_WIELD) && 
//	   !CAN_WEAR(GET_EQ(ch, i), WEAR_HOLD))) { 
      if ((wear_positions[i] == WEAR_HOLD) && IS_DUAL_WIELDING(ch))
        send_to_char("<wielded (2nd)>      ", ch);
      else
        send_to_char(where[wear_positions[i]], ch);

      if (GET_EQ(ch, wear_positions[i]))
      {
        if (CAN_SEE_OBJ(ch, GET_EQ(ch, wear_positions[i])))
	{
	  show_obj_to_char(GET_EQ(ch, wear_positions[i]), ch, 1);
	  found = TRUE;
        } else {
  	  send_to_char("Something.\r\n", ch);
	  found = TRUE;
        }
      } else {
        send_to_char("Nothing.\r\n", ch);
      }
    }
  }
}


ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  sprintf(buf, "It is %d o'clock %s, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));
  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;
  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
  day = time_info.day + 1;	/* day in [1..35] */
  if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";
  sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);
  send_to_char(buf, ch);
}

ACMD(do_weather)
{
  const char *sky_look[] =
  {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };
  if (OUTSIDE(ch))
  {
    sprintf(buf, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    (weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
}

/* command to tell ya what the moon is - Vader */
ACMD(do_moon)
{
  extern struct time_info_data time_info;
  byte day = time_info.day % 3;
 
  if(weather_info.moon == MOON_NONE)
  {
    send_to_char("It is a moonless night.\r\n",ch);
    return;
  }
  if(!(time_info.hours > 12)) 
  {
    day--;
    if(day < 0)
      day = 2;
  }
  switch(day) 
  {
    case 0:
      sprintf(buf, "It is the 1st night of the %s moon.\r\n",
	      moon_mesg[weather_info.moon]);
      break;
    case 1:
      sprintf(buf, "It is the 2nd night of the %s moon.\r\n",
	      moon_mesg[weather_info.moon]);
      break;
    case 2:
      sprintf(buf, "It is the 3rd night of the %s moon.\r\n",
	      moon_mesg[weather_info.moon]);
      break;
  }
  send_to_char(buf,ch);
} 

ACMD(do_help)
{
  int chk, bot, top, mid, minlen, count = 0;
  char *helpstring;

  if (!ch->desc)
    return;
  skip_spaces(&argument);
  if (!*argument)
  {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table)
  {
    send_to_char("No help available.\r\n", ch);
    return;
  }
  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);
  for (;;)
  {
    mid = (bot + top) / 2;
    if (bot > top)
    {
      send_to_char("There is no help on that word.\r\n", ch);
      return;
    } 
    if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen)))
    {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	     (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      // DM - check the help level restriction 
      // TODO - if restricted try and find an unrestricted match before failing 
      if (LR_FAIL_MAX(ch, help_table[mid].level_restriction))
      {
        send_to_char("Sorry, you don't have access to that help.\r\n",ch);
        return;
      }
      /* Bypass the keywords */
      helpstring = help_table[mid].entry;      
      while(helpstring[count] != '\n')
        count++;
      page_string(ch->desc, &helpstring[count+1], 0);
      return;
    }
    if (chk > 0)
      bot = mid + 1;
    else
      top = mid - 1;
  }
}

struct char_data *theWhoList[500];
#define WHO_FORMAT \
"&1usage: &4who [minlev[-maxlev]] [-i [clan num]] [-n name] [-c classlist]\r\n           [-a] [-b] [-t] [-s] [-o] [-q] [-r] [-z] [-1] [-2] [-3]\r\n"
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH];
  char clan_search[MAX_INPUT_LENGTH];
  char mode;
  size_t i;
  int low = 0, high = LVL_OWNER, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0;
  int orderCharName = 0, orderCharLevelA = 0, orderCharTime = 0;
  int showClanOnly = 0, clanNumber = 0;
  int noElements = 0;

  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';
  while (*buf)
  {
    half_chop(buf, arg, buf1);
    if (*arg == '-')
    {
      mode = *(arg + 1);       // just in case; we destroy arg in the switch 
      switch (mode)
      {
	case 'a':
	  orderCharName = 1;
	  strcpy(buf,buf1);
	  break;
	case 'b':
	  orderCharLevelA = 1;
	  strcpy(buf,buf1);
	  break;
	case 't':
	  orderCharTime = 1;
	  strcpy(buf,buf1);
	  break;
	case 'i':
	  showClanOnly = 1;
	  half_chop(buf1, clan_search, buf);
	  break;
	case 'o':
	  outlaws = 1;
	  strcpy(buf, buf1);
	  break;
	case 'z':
	  localwho = 1;
	  strcpy(buf, buf1);
	  break;
	case 's':
	  short_list = 1;
	  strcpy(buf, buf1);
	  break;
	case 'q':
	  questwho = 1;
	  strcpy(buf, buf1);
	  break;
	case 'l':
	  half_chop(buf1, arg, buf);
	  sscanf(arg, "%d-%d", &low, &high);
	  break;
	case 'n':
	  half_chop(buf1, name_search, buf);
	  break;
	case 'r':
	  who_room = 1;
	  strcpy(buf, buf1);
	  break;
	case 'c':
	  half_chop(buf1, arg, buf);
	  for (i = 0; i < strlen(arg); i++)
	    showclass |= find_class_bitvector(arg[i]);
	  break;
	default:
	  send_to_char(WHO_FORMAT, ch);
	  return;
      }				// end of switch 
    } else if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else {			// endif
      send_to_char(WHO_FORMAT, ch);
      return;
    }
  }				// end while (parser) 
  // Clan processing
  if (showClanOnly)
  {
    if (*clan_search)
    {
      clanNumber = find_clan_by_sid(clan_search);
      if ((clanNumber = find_clan_by_sid(clan_search)) < 0)
      {
        send_to_char(WHO_FORMAT,ch);
        send_to_char("That is not a clan, type 'clan info' to see the list.\r\n",ch);
        return;
      }
    } else {
      if (GET_CLAN(ch) < 1)
      {
        send_to_char(WHO_FORMAT, ch);
        send_to_char("You don't belong to a clan.\r\n",ch);
        return;
      } else {
        clanNumber = GET_CLAN(ch);
      }
    }
  }

  for (d = descriptor_list; d; d = d->next)
  {
    //if (STATE(d) != CON_PLAYING)
    if (!IS_PLAYING(d))
      continue;
    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;
    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if ((showClanOnly && GET_CLAN(tch) != clanNumber))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[ch->in_room].zone != world[tch->in_room].zone)
      continue;
    if (who_room && (tch->in_room != ch->in_room))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
    if (short_list)
    {
      // BUG FIX TODO - too few substitutes in string, who added socials?
      sprintf(buf, "%s[%3d %s] %s%-12.12s%s%s",
              // Added colour for social status
              //(GET_SOCIAL_STATUS(ch) <= SOCIAL_UNDESIRABLE  ? CCRED(ch, C_SPR) :
               //GET_SOCIAL_STATUS(ch) <= SOCIAL_LANDOWNER ? CCBLU(ch, C_SPR) : CCGRN(ch, C_SPR)),
	       // DM - ok removed social colours now based on remort
	      (GET_REM_TWO(tch) > 0) ? CCCYN(ch, C_SPR) : 
	       GET_REM_ONE(tch) > 0  ? CCGRN(ch, C_SPR) : CCWHT(ch, C_SPR),
              GET_LEVEL(tch), CLASS_ABBR(tch), 
      	      (GET_LEVEL(tch) >= LVL_GOD       ? CCBYEL(ch,C_SPR)  :
              (GET_LEVEL(tch) >= LVL_ANGEL     ? CCCYN(ch,C_SPR)   :
              (GET_LEVEL(tch) >= LVL_LEGEND    ? CCMAG(ch,C_SPR)   : 
              (GET_LEVEL(tch) >= LVL_ISNOT_GOD ? CCRED(ch,C_SPR)   :
	      (GET_LEVEL(tch) >= LVL_CHAMP     ? CCBLU(ch, C_SPR)  :
              (GET_LEVEL(tch) >= LVL_ETRNL1    ? CCBBLU(ch, C_SPR) :
	       CCWHT(ch, C_SPR))))))),
	      GET_NAME(tch),
              (GET_LEVEL(tch) >= LVL_ETRNL1 ? CCNRM(ch, C_SPR) : ""),
              ((!(++num_can_see % 4)) ? "\r\n" : ""));
      send_to_char(buf,ch);
    } else {
      theWhoList[noElements++] = tch;
      num_can_see++;
    }				// endif shortlist 
  }				// end of for 
  // Ordered by char name (alpha ascending) 
  if (orderCharName)
    qsort(theWhoList, noElements, sizeof(struct char_data *), compareCharName);
  else if (orderCharLevelA) // Ordered by Level Ascending
    qsort(theWhoList, noElements, sizeof(struct char_data *),
	  compareCharLevelA);
  else if (orderCharTime); // Unsorted (by time)
  else // Default ordering is Level Descending
    qsort(theWhoList, noElements, sizeof(struct char_data *),
	  compareCharLevelD);
  if (!short_list)
    print_who_info(theWhoList,num_can_see,ch);
  if (short_list && (num_can_see % 4))
    send_to_char("\r\n", ch);
  if (num_can_see == 0)
    sprintf(buf, "\r\n&nNo-one at all!\r\n");
  else if (num_can_see == 1)
    sprintf(buf, "\r\n&nOne lonely character displayed.\r\n");
  else
    sprintf(buf, "\r\n&c%d&n characters displayed.\r\n", num_can_see);
  send_to_char(buf, ch);
}

int compareAffectDuration(const void *l, const void *r)
{
  struct affected_type **left;
  struct affected_type **right;
  left = (struct affected_type **)l;
  right = (struct affected_type **)r;
  // sort -1 (unlimited durations) to head of list
  if ((*left)->duration == CLASS_ABILITY)
    return -1;
  if ((*right)->duration == CLASS_ABILITY)
    return 1;
  if ((*left)->duration == CLASS_ITEM)
    return -1;
  if ((*right)->duration == CLASS_ITEM)
    return 1;
  return ((*left)->duration < (*right)->duration);
}

// Compares chars names, returns normal strcmp result
int compareCharName(const void *l, const void *r)
{
  struct char_data **left;
  struct char_data **right;
  left = (struct char_data **)l;
  right = (struct char_data **)r;
  return strcmp(GET_NAME(*left),GET_NAME(*right));
}

// Compares chars levels for descending, if l < r, return 1 
int compareCharLevelD(const void *l, const void *r)
{
  struct char_data **left;
  struct char_data **right;
  left = (struct char_data **)l;
  right = (struct char_data **)r;
  if (GET_LEVEL(*left) < GET_LEVEL(*right))
    return 1;
  else if (GET_LEVEL(*left) == GET_LEVEL(*right))
    return 0;
  else
    return -1;
}

// Compares chars levels for ascending, l < r, return -1
int compareCharLevelA(const void *l, const void *r)
{
  struct char_data **left;
  struct char_data **right;
  left = (struct char_data **)l;
  right = (struct char_data **)r;
  if (GET_LEVEL(*left) < GET_LEVEL(*right))
    return -1;
  else if (GET_LEVEL(*left) == GET_LEVEL(*right))
    return 0;
  else
    return 1;
}

// Level Rank Abbreviations: PLR, CMP, GOD, etc.
char *levelrank_abbrev(int level)
{
  static char s[5];
  if (level <  LVL_ETRNL1) return("PLR");
  if (level <  LVL_CHAMP)  
  {
    sprintf(s, "ET%d", level-LVL_ETRNL1+1);
    return (s);
  }
  if (level <  LVL_IMMORT) return("CMP");
  if (level <  LVL_ANGEL)  return("IMM");
  if (level <  LVL_LEGEND) return("ANG");
  if (level <  LVL_GOD)    return("LEG");
  if (level <  LVL_GRGOD)  return("DEI");
  if (level <  LVL_IMPL)   return("GOD");
  if (level <  LVL_GRIMPL) return("IMP");
  if (level <  LVL_OWNER)  return("BCH");
  if (level == LVL_OWNER)  return("ONR");
  return("WTF");
}

// Level Rank Colours.
char *levelrank_colour(struct char_data *ch, int targlevel, int colourlevel)
{
  if ((!clr(ch, colourlevel)) || (targlevel < LVL_ETRNL1)) return "";
  if (targlevel < LVL_CHAMP)  return "&B";
  if (targlevel < LVL_IMMORT) return "&r";
  if (targlevel < LVL_ANGEL)  return "&R";
  if (targlevel < LVL_LEGEND) return "&c";
  if (targlevel < LVL_GOD)    return "&C";
  if (targlevel < LVL_IMPL)   return "&y";
  return "&Y";
}

/** 
 * Prints out the characters referenced by an array of struct char_data * 
 */ 
void print_who_info(struct char_data *array[], int length, 
                    struct char_data *ch)
{
  int i,j,k,l;
  struct char_data *tch;
  char sr[5], sc[4];
  buf2[0]='\0';
  send_to_char("&y---------\r\n &rP&Rl&Ya&Wy&Ye&Rr&rs&n\r\n&y---------&n\r\n", ch);
  for (i = 0; i < length; i++)
  {
    tch = array[i];
    strcpy(sc, levelrank_colour(ch, GET_LEVEL(tch), C_SPR));
    if (strlen(GET_WHO_STR(tch)) > 0)
    {
      j = 10; k = 20; l=0;
      //sprintf(buf, "%s[", (GET_SOCIAL_STATUS(tch) <= SOCIAL_UNDESIRABLE ? 
      //      CCRED(ch, C_SPR) : GET_SOCIAL_STATUS(tch) <= SOCIAL_LANDOWNER ? 
      //      CCBLU(ch, C_SPR) : CCGRN(ch, C_SPR)));
      // DM    - removed social status, now based on remort levels
      // Artus - fuck it. lets go back to the old level way.
      sprintf(buf, "%s[", (*sc ? sc : "&n"));
      j -= (int)(strdisplen(GET_WHO_STR(tch)) / 2);
      k -= (j+strdisplen(GET_WHO_STR(tch)));
      for (l=0; l < j; l++)
	strcat(buf, " ");
      strcat(buf, GET_WHO_STR(tch));
      for (l=0; l < k; l++)
	strcat(buf, " ");
      sprintf(buf+strlen(buf), "%s] %s %s&n", (*sc ? sc : "&n"), 
	      GET_NAME(tch), GET_TITLE(tch));
    } else {
      strcpy(sr, levelrank_abbrev(GET_LEVEL(tch)));
      sprintf(buf, "%s[ %02d/%02d/%-3d %c %s %s ] %s %s",
	      (*sc ? sc : "&n"), GET_REM_ONE(tch), GET_REM_TWO(tch),
	      GET_LEVEL(tch),
	       (GET_SEX(tch)==SEX_MALE   ? 'M' :
		GET_SEX(tch)==SEX_FEMALE ? 'F' : '-'),
	       CLASS_ABBR(tch), RACE_ABBR(tch), GET_NAME(tch),
	       GET_TITLE(tch));
    }
    if (GET_INVIS_LEV(tch))
    {
      if (GET_INVIS_TYPE(tch) == INVIS_NORMAL) // Standard invis?
	sprintf(buf+strlen(buf), " (i%d)", GET_INVIS_LEV(tch));
      else if (GET_INVIS_TYPE(tch) == INVIS_SINGLE) // Level invis?
	sprintf(buf+strlen(buf), " (i%ds)", GET_INVIS_LEV(tch));
      else if (GET_INVIS_TYPE(tch) == INVIS_SPECIFIC) // Player invis?
	strcpy(buf, " (i-char)");
      else // Ranged invis!
	sprintf(buf+strlen(buf), " (i%d - %d)", GET_INVIS_LEV(tch),
	        GET_INVIS_TYPE(tch));
    }
    if (IS_AFFECTED(tch, AFF_INVISIBLE))
      strcat(buf, " (invis)"); 
    if (IS_BUILDING(tch))
      strcat(buf, " (building)");
    else if (PLR_FLAGGED(tch, PLR_MAILING))
      strcat(buf, " (mailing)");
    else if (PLR_FLAGGED(tch, PLR_WRITING | PLR_ODDWRITE))
      strcat(buf, " (writing)");
    else if (PLR_FLAGGED(tch, PLR_REPORTING))
      strcat(buf, " (reporting)");
    for (j = 0; j < MAX_IGNORE; j++)
      if (GET_IGNORE(tch, j) > 0 || GET_IGN_LVL(tch) > 0)
      {
	strcat(buf, " (snob)");
	break;
      }
    if (PRF_FLAGGED(tch, PRF_AFK))
      strcat(buf, " (AFK)");
    if (PRF_FLAGGED(tch, PRF_DEAF))
      strcat(buf, " (deaf)");
    if (EXT_FLAGGED(tch, EXT_PKILL)) /* PK flag showing. - ARTUS */
      strcat(buf, " (PK)");
    if (PRF_FLAGGED(tch, PRF_NOTELL))
      strcat(buf, " (notell)");
    if (PRF_FLAGGED(tch, PRF_QUEST))
      strcat(buf, " (quest)");
    if (PLR_FLAGGED(tch, PLR_THIEF))
      strcat(buf, " (THIEF)");
    if (PLR_FLAGGED(tch, PLR_KILLER))
      strcat(buf, " (KILLER)");
    if (PRF_FLAGGED(tch, PRF_TAG))
      strcat(buf, " (tag)");
    if (world[IN_ROOM(tch)].number == IDLE_ROOM_VNUM)
      strcat(buf, " (limbo)");
    if (IS_GHOST(tch))
      strcat(buf, " (ghost)");
    if (GET_UNHOLINESS(tch) > 0)
      sprintf(buf+strlen(buf), " (UV: %d)", GET_UNHOLINESS(tch));
    if (GET_LEVEL(tch) >= LVL_ETRNL1)
      strcat(buf, CCNRM(ch, C_SPR));
    if (GET_CLAN(tch) > 0) // Show Clan.
    {
      int c;
      c = find_clan_by_id(GET_CLAN(tch));
      if (GET_CLAN_RANK(tch) == clan[c].ranks)
	strcat(buf, CCBLU(ch, C_SPR));
      sprintf(buf, "%s [%s - %s]&n", buf, clan[c].name, 
	      ((GET_CLAN_RANK(tch) < 1) ? "Applying" : 
	       clan[c].rank_name[(GET_CLAN_RANK(tch) - 1)]));
    }
    strcat(buf, "\r\n");
    strncat(buf2,buf,strlen(buf));
  }				/* end of for */
  page_string(ch->desc, buf2, 0);
}

#define USERS_FORMAT \
"&1format: &4users [-a name ] [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]&n\r\n"
ACMD(do_users)
{
  const char *format = "%3d %-7s %-12s %-14s %-3s %-8s ";
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, *timeptr2, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  struct user_data *users;
#ifdef NO_LOCALTIME
  struct tm lt;
#endif
  size_t i;
  int low = 0, high = LVL_OWNER-1, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;
  int h, m, s;
  time_t mytime, tally = 0;
  char temp1[11], temp2[11], *name;
  int singleuser = FALSE;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf)
  {
    half_chop(buf, arg, buf1);
    if (*arg == '-')
    {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode)
      {
	case 'a':
	  // name specified - tally times and only display that person
	  if (*buf1)
	  {
	    name = buf1;
	    skip_spaces(&name);
	    if (get_id_by_name(name) > 0)
	      singleuser = TRUE;
	  }
	  strcpy(buf, "Num Lvl   Name         Site                     Login@   Logout@  Session\r\n");
	  strcat(buf, "--- ----- ------------ ------------------------ -------- -------- --------\r\n");
	  for (users = user_list; users; users=users->next)
	  {
	    if (singleuser && str_cmp(name, users->name))
	      continue;
#ifndef NO_LOCALTIME
	    timeptr = asctime(localtime(&users->login));
#else
	    if (jk_localtime(&lt, users->login))
	    {
	      basic_mud_log("Error in jk_localtime (users->login: %ld) [%s:%d]", users->login, __FILE__, __LINE__);
	      continue;
	    }
	    timeptr = asctime(&lt);
#endif
	    timeptr += 11;
	    *(timeptr + 8) = '\0';
	    strcpy(temp1, timeptr);

	    if (users->logout)
	    {
#ifndef NO_LOCALTIME
	      timeptr2 = asctime(localtime(&users->logout));
#else
	      if (jk_localtime(&lt, users->logout))
	      {
		basic_mud_log("Error in jk_localtime(users->logout: %ld) [%s:%d]", users->logout, __FILE__, __LINE__);
		continue;
	      }
	      timeptr2 = asctime(&lt);
#endif
	      timeptr2 += 11;
	      *(timeptr2 + 8) = '\0'; 
	      strcpy(state, timeptr2);
	      strcpy(temp2, timeptr2);
	      
	      mytime = users->logout - users->login;
	    } else {
	      strcpy(temp2, "-active-");
	      mytime = time(0) - users->login;
	    }
	    if (singleuser)
	      tally = tally + mytime;
	    h = (mytime / 3600);
	    m = (mytime / 60) % 60;
	    s = mytime % 60;
	    sprintf(buf2, "%3d [%-3d] %-12s %-24s %-8s %-8s %dh%dm%ds\r\n",
		    users->number, users->level, users->name, users->host,
		    temp1, temp2, h, m, s);
	    strcat(buf, buf2);
	  }
	  if (singleuser)
	  {
	    h = (tally / 3600);
	    m = (tally / 60) % 60;
	    s = tally % 60;
	    sprintf(buf2,"Total time for %s: %dh%dm%ds\r\n", name, h, m, s);
	    strcat(buf, buf2);
	  }
	  page_string(ch->desc, buf, TRUE);
	  return;
	case 'o':
	case 'k':
	  outlaws = 1;
	  playing = 1;
	  strcpy(buf, buf1);
	  break;
	case 'p':
	  playing = 1;
	  strcpy(buf, buf1);
	  break;
	case 'd':
	  deadweight = 1;
	  strcpy(buf, buf1);
	  break;
	case 'l':
	  playing = 1;
	  half_chop(buf1, arg, buf);
	  sscanf(arg, "%d-%d", &low, &high);
	  break;
	case 'n':
	  playing = 1;
	  half_chop(buf1, name_search, buf);
	  break;
	case 'h':
	  playing = 1;
	  half_chop(buf1, host_search, buf);
	  break;
	case 'c':
	  playing = 1;
	  half_chop(buf1, arg, buf);
	  for (i = 0; i < strlen(arg); i++)
	    showclass |= find_class_bitvector(arg[i]);
	  break;
	default:
	  send_to_char(USERS_FORMAT, ch);
	  return;
      }				/* end of switch */
    } else {			/* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */
  strcpy(line,
	 "Num Class    Name         State          Idl Login@   Site\r\n");
  strcat(line, "--- -------- ------------ -------------- --- -------- ------------------------\r\n");
  send_to_char(line, ch);
  one_argument(argument, arg);
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (IS_PLAYING(d))
    { 
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;
      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low)// || GET_LEVEL(tch)>high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;
      if (d->original)
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->original),
		CLASS_ABBR(d->original));
      else
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character));
    } else
      strcpy(classname, "   -   ");
#ifndef NO_LOCALTIME
    timeptr = asctime(localtime(&d->login_time));
#else
    if (jk_localtime(&lt, d->login_time))
    {
      basic_mud_log("Error in jk_localtime (d->login_time: %ld) [%s:%d]", d->login_time, __FILE__, __LINE__);
      return;
    }
    timeptr = asctime(&lt);
#endif
    timeptr += 11;
    *(timeptr + 8) = '\0';
    if (STATE(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[STATE(d)]);
    //if (d->character && STATE(d) == CON_PLAYING && (GET_LEVEL(d->character) < LVL_GOD)
    if ((d->character && STATE(d) == CON_PLAYING) && (GET_LEVEL(d->character) <= GET_LEVEL(ch)))
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    if (d->character && d->character->player.name)
    {
      if (d->original)
	sprintf(line, format, d->desc_num, classname,
		d->original->player.name, state, idletime, timeptr);
      else
	sprintf(line, format, d->desc_num, classname,
		d->character->player.name, state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED",
	      state, idletime, timeptr);
    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");
    if (STATE(d) != CON_PLAYING)
    {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (((STATE(d) != CON_PLAYING) || (STATE(d) == CON_PLAYING)) &&
	(CAN_SEE(ch, d->character)))
    {
      send_to_char(line, ch);
      num_can_see++;
    }
  }
  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  switch (subcmd)
  {
    case SCMD_CREDITS:
      page_string(ch->desc, credits, 0);
      break;
    case SCMD_NEWS:
      page_string(ch->desc, news, 0);
      break;
    case SCMD_INFO:
      page_string(ch->desc, info, 0);
      break;
    case SCMD_WIZLIST:
      page_string(ch->desc, wizlist, 0);
      break;
    case SCMD_IMMLIST:
      page_string(ch->desc, immlist, 0);
      break;
    case SCMD_HANDBOOK:
      page_string(ch->desc, handbook, 0);
      break;
    case SCMD_POLICIES:
      page_string(ch->desc, policies, 0);
      break;
    case SCMD_MOTD:
      page_string(ch->desc, motd, 0);
      break;
    case SCMD_IMOTD:
      page_string(ch->desc, imotd, 0);
      break;
    case SCMD_CLEAR:
      send_to_char("\033[H\033[J", ch);
      break;
    case SCMD_VERSION:
      sprintf(buf, "PrimalMUD version: &c%d.%d.%d&n Copyright (C) 1994-2004\r\n"
		   "  Built on: %s\r\n\r\n"
		   "Based on CircleMUD version 3bpl17 and" 
		   " PrimalMUD version 2.00\r\n", 
	      release.getMajor(), release.getBranch(), release.getMinor(),
	      release.getDate());
      send_to_char(buf, ch);
      send_to_char(strcat(strcpy(buf, DG_SCRIPT_VERSION), "\r\n"), ch);
      //send_to_char(strcat(strcpy(buf, circlemud_version), "\r\n"), ch);
      break;
    case SCMD_WHOAMI:
      send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
      break;
    case SCMD_AREAS:
      page_string(ch->desc, areas, 0);
      break;
    default:
      basic_mud_log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
      return;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{
  int zone_name_len, j;  
  register struct char_data *i;
  register struct descriptor_data *d;
  extern struct zone_data *zone_table; 

  if (!*arg)
  {
    int k;
    zone_name_len = strlen(zone_table[world[ch->in_room].zone].name);
    sprintf(buf, "Players in &R%s&n\r\n",
	    zone_table[world[ch->in_room].zone].name);
    k = strlen(buf);
    for (j=0; j < (11+zone_name_len); j++)
      buf[k++] = '-';
    buf[k] = '\0';
    strcat(buf, "\r\n");
    send_to_char(buf,ch);
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (i->in_room == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[ch->in_room].zone != world[i->in_room].zone)
	continue;
      sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next)
    {
      if (i->in_room == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[i->in_room].zone != world[ch->in_room].zone)
	continue;
      if (!isname(arg, i->player.name))
	continue;
      sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
      return;
    }
    send_to_char("No-one around by that name.\r\n", ch);
  }
}

void print_object_location(int num, struct obj_data * obj, struct char_data *ch,
			   int recur, char *writeto)
{
  if (num > 0)
    sprintf(writeto, "&bO&n%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(writeto, "%33s", " - ");
  if (obj->in_room > NOWHERE)
  {
    sprintf(writeto + strlen(writeto), "&c[%5d]&n %s\r\n",
	    GET_ROOM_VNUM(IN_ROOM(obj)), world[obj->in_room].name);
    //send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(writeto + strlen(writeto), "carried by %s\r\n",
	    PERS(obj->carried_by, ch));
    //send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(writeto + strlen(writeto), "worn by %s\r\n",
	    PERS(obj->worn_by, ch));
    //send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(writeto + strlen(writeto), "inside %s%s\r\n",
	    obj->in_obj->short_description, (recur ? ", which is" : " "));
    //send_to_char(buf, ch);
    if (recur)
    {
      buf2[0] = '\0';
      print_object_location(0, obj->in_obj, ch, recur, buf2);
      sprintf(writeto, "%s%s", writeto, buf2);
    }
  } else {
    sprintf(writeto + strlen(writeto), "in an unknown location\r\n");
    //send_to_char(buf, ch);
  }
}



void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;
  buf[0] = '\0';
  buf1[0] = '\0';
  if (!*arg)
  {
    sprintf(buf, "&gPlayers&n\r\n-------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (STATE(d) == CON_PLAYING)
      {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE))
	{
	  if (d->original)
	    sprintf(buf+strlen(buf), "%-20s - &c[%5d]&n %s (%s) (in %s)\r\n",
		    GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		    world[i->in_room].name,
		    zone_table[world[i->in_room].zone].name, 
		    GET_NAME(d->character));
	  else
	    sprintf(buf+strlen(buf), "%-20s - &c[%5d]&n %s (%s)\r\n",
		    GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)),
		    world[i->in_room].name,
		    zone_table[world[i->in_room].zone].name);
	}
      }
    page_string(ch->desc, buf, TRUE);
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE 
                      && isname(arg, i->player.name))
      {
	found = 1;
	sprintf(buf+strlen(buf), "&rM&n%3d. %-25s - &c[%5d]&n %s\r\n", ++num,
	        GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	//send_to_char(buf, ch);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name))
      {
	found = 1;
	print_object_location(++num, k, ch, TRUE, buf+strlen(buf));
      }
    if (!found)
      send_to_char("Couldn't find any such thing.\r\n", ch);
    else
      page_string(ch->desc, buf, TRUE);
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);
  if (LR_FAIL(ch, LVL_ISNOT_GOD))
    perform_mortal_where(ch, arg);
  else
    perform_immort_where(ch, arg);
}

// DM - TODO - decide on levels format
ACMD(do_levels)
{
  int i;

  if (IS_NPC(ch))
  {
    send_to_char("You ain't nothin' but a hound-dog.\r\n", ch);
    return;
  }
  *buf = '\0';

  for (i = 1; i < LVL_IMMORT-1; i++)
  {
    if (i >= LVL_ETRNL1-1)
      sprintf(buf+strlen(buf), "&g[&m%3d - %2d&g]&n &c%9d&n\r\n", i, i+1,
	      level_exp(ch, i));
    else
      sprintf(buf+strlen(buf), "&g[&n%3d - %2d&g]&n &c%9d&n\r\n", i, i+1,
	      level_exp(ch, i));
  }
  sprintf(buf+strlen(buf), "&g[   &b%3d&g  ]&n &c%9d&n : &bImmortality&n\r\n",
	  LVL_IMMORT, level_exp(ch, LVL_IMMORT-1));
  page_string(ch->desc, buf, 1);
}

ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = generic_find_char(ch, buf, FIND_CHAR_ROOM)))
  {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch)
  {
    send_to_char("Easy!  Very easy indeed!\r\n", ch);
    return;
  }
  if (!IS_NPC(victim))
  {
    send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char("Now where did that chicken go?\r\n", ch);
  else if (diff <= -5)
    send_to_char("You could do it with a needle!\r\n", ch);
  else if (diff <= -2)
    send_to_char("Easy.\r\n", ch);
  else if (diff <= -1)
    send_to_char("Fairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\r\n", ch);
  else if (diff <= 1)
    send_to_char("You would need some luck!\r\n", ch);
  else if (diff <= 2)
    send_to_char("You would need a lot of luck!\r\n", ch);
  else if (diff <= 3)
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 5)
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  else if (diff <= 10)
    send_to_char("Are you mad!?\r\n", ch);
  else if (diff <= 100)
    send_to_char("You ARE mad!\r\n", ch);
}

ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf)
  {
    if (!(vict = generic_find_char(ch, buf, FIND_CHAR_ROOM)))
      send_to_char(NOPERSON, ch);
    else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}


const char *ctypes[] = { "off", "sparse", "normal", "complete", "\n" };
ACMD(do_color)
{
  int tp;
  if (IS_NPC(ch))
    return;
  one_argument(argument, arg);
  if (!*arg)
  {
    sprintf(buf, "Your current colour level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1))
  {
    send_to_char("Usage: colour { Off | Sparse | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | 
                         (PRF_COLOR_2 * (tp & 2) >> 1));
  sprintf(buf, "Your %scolour%s is now %s.\r\n", CCRED(ch, C_SPR),
	  CCNRM(ch, C_OFF), ctypes[tp]);
  send_to_char(buf, ch);
}

/* DM - modified for new toggles and to toggle others and from file */
ACMD(do_toggle)
{
  struct char_data *victim;
  struct char_file_u tmp_store;
 
  if (IS_NPC(ch))
    return;
  half_chop(argument, buf1, buf2);
  if ((!*buf1) || LR_FAIL(ch, LVL_IMMORT))
  {
    toggle_display(ch,ch);
    return;
  }
  if (is_abbrev(buf1, "file"))
  {
    if ((!*buf2) || LR_FAIL(ch, LVL_IS_GOD))
      send_to_char("Toggle's for who?\r\n", ch);
    else
    {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1)
      {
        store_to_char(&tmp_store, victim);
	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char("Sorry, you can't do that.\r\n", ch);
	else
          toggle_display(ch, victim);
        free_char(victim);
      } else {
        send_to_char("There is no such player.\r\n", ch);
        free(victim);
      }
    }
  } else {
    if ((victim = generic_find_char(ch, buf1, FIND_CHAR_WORLD)))
      toggle_display(ch,victim);
    else
      send_to_char("Toggle's for who?\r\n", ch);
  }
}
 
/* DM - Toggle display on vict */
void toggle_display(struct char_data *ch, struct char_data *vict)
{ 
  char prompt_string[25 + 1 + 2];

  if (IS_NPC(ch) || IS_NPC(vict))
    return;
  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");
  else
    sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));
  prompt_string[0] = '\0';
  if (GET_PROMPT(vict))
  {
    strcat(prompt_string, "'");
    strcat(prompt_string, GET_PROMPT(vict));
    strcat(prompt_string, "&n'");
  } else {
    if (PRF_FLAGGED(vict, PRF_DISPHP))
      strcat(prompt_string, "H");
    if (PRF_FLAGGED(vict, PRF_DISPMANA))
      strcat(prompt_string, "M");
    if (PRF_FLAGGED(vict, PRF_DISPMOVE))
      strcat(prompt_string, "V");
    if (PRF_FLAGGED(vict, PRF_DISPEXP))
      strcat(prompt_string, "X");
    if (PRF_FLAGGED(vict, PRF_DISPALIGN))
      strcat(prompt_string, "L");
  }
  sprintf(buf, " Character Name: %s\r\n\r\n"
	       "Hit Pnt Display: %-3s    " "     Brief Mode: %-3s    "
	       " Summon Protect: %-3s\r\n"
	       "   Move Display: %-3s    " "   Compact Mode: %-3s    "
	       "       On Quest: %-3s\r\n"
	       "   Mana Display: %-3s    " " Auto Show Exit: %-3s    "
	       " Gossip Channel: %-3s\r\n"
	       "    Exp Display: %-3s    " "   Repeat Comm.: %-3s    "
	       "Auction Channel: %-3s\r\n"
	       "  Align Display: %-3s    " "           Deaf: %-3s    "
	       "  Grats Channel: %-3s\r\n"
	       "       Autoloot: %-3s    " "         NoTell: %-3s    "
	       " Newbie Channel: %-3s\r\n"
	       "       Autogold: %-3s    " "     Marked AFK: %-3s    "
	       "   Clan Channel: %-3s\r\n"
	       "      Autosplit: %-3s    " " Clan Available: %-3s    "
	       "   Info Channel: %-3s\r\n"
	       "        Autoeat: %-3s    " "    Color Level: %-10s"
	       "Hint Channel: %-3s\r\n"
	       "    Page Length: %-3d    " "     Page Width: %-3d   "
	       "Corpse Retrieval: %-3s\r\n"
	       "     Wimp Level: %-4s   "  "  Prompt String: %s\r\n",
          GET_NAME(vict), ONOFF(PRF_FLAGGED(vict, PRF_DISPHP)),
          ONOFF(PRF_FLAGGED(vict, PRF_BRIEF)),
          ONOFF(!PRF_FLAGGED(vict, PRF_SUMMONABLE)),
 
          ONOFF(PRF_FLAGGED(vict, PRF_DISPMOVE)),
          ONOFF(PRF_FLAGGED(vict, PRF_COMPACT)),
          YESNO(PRF_FLAGGED(vict, PRF_QUEST)), 

          ONOFF(PRF_FLAGGED(vict, PRF_DISPMANA)),
          ONOFF(PRF_FLAGGED(vict, PRF_AUTOEXIT)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOGOSS)),
 
          ONOFF(PRF_FLAGGED(vict, PRF_DISPEXP)),
          YESNO(!PRF_FLAGGED(vict, PRF_NOREPEAT)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOAUCT)),
 
          ONOFF(PRF_FLAGGED(vict, PRF_DISPALIGN)),
          YESNO(PRF_FLAGGED(vict, PRF_DEAF)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOGRATZ)),
 
          ONOFF(EXT_FLAGGED(vict, EXT_AUTOLOOT)),
          ONOFF(PRF_FLAGGED(vict, PRF_NOTELL)),
          ONOFF(!EXT_FLAGGED(vict, EXT_NONEWBIE)),
 
          ONOFF(EXT_FLAGGED(vict, EXT_AUTOGOLD)),
          YESNO(PRF_FLAGGED(vict, PRF_AFK)),
          ONOFF(!EXT_FLAGGED(vict, EXT_NOCT)),
 
          ONOFF(EXT_FLAGGED(vict, EXT_AUTOSPLIT)),
          YESNO(EXT_FLAGGED(vict, EXT_PKILL)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOINFO)),

	  ONOFF(EXT_FLAGGED(vict, EXT_AUTOEAT)),
          ctypes[COLOR_LEV(vict)],
          ONOFF(!EXT_FLAGGED(vict, EXT_NOHINTS)),
 
          GET_PAGE_LENGTH(vict), 
	  GET_PAGE_WIDTH(vict),
	  ONOFF(EXT_FLAGGED(vict, EXT_AUTOCORPSE)),

	  buf2,
          prompt_string);
 
  send_to_char(buf, ch);
  if (GET_MAX_LVL(vict) >= LVL_ETRNL1)
  {
    sprintf(buf, "\r\n     Room Flags: %-3s    " " Wiznet Channel: %-3s\r\n"
		 "      No Hassle: %-3s    " " Immnet Channel: %-3s\r\n"
		 "     Holy Light: %-3s\r\n",
          ONOFF(PRF_FLAGGED(vict, PRF_ROOMFLAGS)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOWIZ)),
          ONOFF(PRF_FLAGGED(vict, PRF_NOHASSLE)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOIMMNET)),
          ONOFF(PRF_FLAGGED(vict, PRF_HOLYLIGHT)));
    send_to_char(buf,ch);
  } 
}

struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;
int num_of_cmds;

void sort_commands(void)
{
  int a, b, tmp;
  num_of_cmds = 0;
  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;
  /* create data array */
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);
  /* initialize it */
  for (a = 1; a < num_of_cmds; a++)
  {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }
  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;
  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0)
      {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}

#define CPASS_NONE      -1
#define CPASS_ALL       1
#define CPASS_SUBCMD    2
#define CPASS_VICT      3

/*
 * Searches in order of the above defines for a matching argument used parsing
 * the "commands" command.
 *
 * Returns CPASS_NONE if no rule is matched for the given argument, otherwise
 * set the various variables as defined below and returns the corresponding 
 * matched rule define.
 *
 * CMD_SUBCMD:  sets the value of cmd_num
 * CMD_VICT:    sets *vict, applies level checks and sets *vict to ch if level 
 *              checks do not pass 
 */
int parse_command_arg(struct char_data *ch, struct char_data **vict, 
                char *arg, int *cmd_num)
{
  int cmd = -1;
  struct char_data *find;

  if (!*arg)
    return CPASS_NONE;
  // "all"
  if (!str_cmp(arg, "all"))
    return CPASS_ALL;
  // "subcmd"
  if ((cmd = search_block_case_insens(arg, cmd_types, FALSE)) != -1)
  {
    *cmd_num = cmd; 
    return CPASS_SUBCMD;
  }
  if (LR_FAIL(ch, LVL_IS_GOD))
    return CPASS_NONE;
  // "playername"
  if ((find = generic_find_char(ch, arg, FIND_CHAR_WORLD)))
  {
    *vict = find;
    // apply restrictions
    if (IS_NPC(find) && GET_LEVEL(ch) < LVL_GOD)
      *vict = ch; 
    if (!IS_NPC(find) && GET_LEVEL(ch) < GET_LEVEL(find))
      *vict = ch;
    return CPASS_VICT;
  }
  return CPASS_NONE;
}

void display_commands(struct char_data *ch, struct char_data *vict, 
                int subcmd, int cmdtype)
{
  int cmd_num, no, i;
  bool wizhelp = FALSE, socials = FALSE;
  char command_type[MAX_INPUT_LENGTH];

  if (cmdtype > -1)
    sprintf(command_type, "&1%s ", cmd_types[cmdtype]);
  if (subcmd == SCMD_WIZHELP)
    wizhelp = TRUE;
  if (subcmd == SCMD_SOCIALS)
    socials = TRUE;
  sprintf(buf, "The following %s&4%s%s&n are available to &7%s&n:\r\n",
          (cmdtype > -1) ? command_type : "", wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));
  // cmd_num starts at 1, not 0, to remove 'RESERVED' 
  for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++)
  {
    i = cmd_sort_info[cmd_num].sort_pos;
    if (cmd_info[i].minimum_level >= 0 &&
        !LR_FAIL_MAX(vict, cmd_info[i].minimum_level) &&
        ((cmd_info[i].minimum_level >= LVL_ETRNL1) == wizhelp 
         || cmdtype == CMD_WIZ) &&
        ((wizhelp || cmdtype == CMD_WIZ) || 
         (socials == cmd_sort_info[i].is_social) || cmdtype == CMD_SOCIAL))
    {
      if (cmdtype < 0 || cmd_info[i].type == cmdtype)
      {
        sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
        if (!(no % 7))
          strcat(buf, "\r\n");
        no++;
      }
    }
  }

  strcat(buf, "\r\n");
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_commands)
{
  int cmd_num = -1;
  struct char_data *vict = ch;
  char rest[MAX_INPUT_LENGTH];
  int cmd_pass = CPASS_NONE;
  char usage[MAX_STRING_LENGTH];

  arg[0] = '\0';
  rest[0] = '\0';

  if (subcmd == SCMD_COMMANDS)
  {
    sprintf(usage, "&1Usage: &4%s&n [ all | &1cmd-type&n ] [ player ]\r\n\r\n", cmd_info[cmd].command);
    sprintf(usage + strlen(usage), "&0Command Types:&n\r\n");
    // start at 1 ignoring None ... (still leave it there, just dont display it)
    for (int i = 1; i < NUM_CMD_TYPES; i++)
      sprintf(usage + strlen(usage), "&1%-15s&n %s\r\n", cmd_types[i], cmd_bhelp[i]);
  } else if (subcmd == SCMD_WIZHELP)
    sprintf(usage, "&1Usage: &4%s&n [ player ]\r\n", cmd_info[cmd].command);
  else
    sprintf(usage, "&1Usage: &4%s&n\r\n", cmd_info[cmd].command);
  half_chop(argument, arg, rest);
  if (subcmd != SCMD_SOCIALS && *arg)
  {
    // arg 1: [ all | cmd_type | player_name ]
    cmd_pass = parse_command_arg(ch, &vict, arg, &cmd_num);
    if ((subcmd == SCMD_COMMANDS) && 
	(cmd_pass == CPASS_NONE || cmd_pass == CPASS_VICT))
    {
      send_to_char(usage, ch);
      return;
    }
    // commands "all" | "subcmd"
    if ((subcmd == SCMD_COMMANDS) && 
	(cmd_pass == CPASS_ALL || cmd_pass == CPASS_SUBCMD))
    {
      // search arg2 for commands { all | sbcmd } "player_name"
      half_chop(rest, arg, rest);
      if (*arg) // only need to set vict ...
        parse_command_arg(ch, &vict, arg, &cmd_num);
    }
  } else if (!*arg && subcmd == SCMD_COMMANDS) {
    send_to_char(usage, ch);      
    return;
  }
  display_commands(ch, vict, subcmd, cmd_num); 
}

ACMD(do_classes)
{
  int index;
  sprintf(buf,"Classes:\r\n");
  for (index=0; index < NUM_CLASSES; index++)
  {
    sprintf(buf2,"&B%s\r\n",CLASS_NAME(index));
    strcat(buf,buf2);
  }
  page_string(ch->desc, buf, TRUE);
}

// DM - set your sexy colours
ACMD(do_setcolour)
{
  int colour_code, colour_num, no_args = TRUE, i;
  char *usage = "&1Usage:&n &4colourset&n [-<0..9> { <colourcode> | Default }]\r\n";
  int colour_codes[10];
  char mode;
  if (IS_NPC(ch))
    return;
  skip_spaces(&argument);
  strcpy(buf, argument);
  for (i=0;i<10;i++)
    colour_codes[i] = -1;
  while (*buf)
  {
    half_chop_case_sens(buf, arg, buf1);
    if (*arg == '-')
    {
      mode = *(arg + 1);       /* just in case; we destroy arg in the switch */
      if (isnum(mode))
      {
        colour_num = mode - '0';
	half_chop_case_sens(buf1, arg, buf);
        if (!strn_cmp(arg,"default",3))
	{
          // The magic default number
          colour_codes[colour_num] = 69; 
          no_args = FALSE;
        } else if ((colour_code = is_colour(NULL,*arg,TRUE)) >= 0) {
          colour_codes[colour_num] = colour_code;
          no_args = FALSE;
        } else {
          send_to_char("Invalid colour code.\r\n",ch);
          send_to_char(usage,ch);
          return;
        }
      } else {			/* endif isnum */
        send_to_char(usage, ch);
        return;
      }
    } else {			/* endif arg == '-' */
      send_to_char(usage, ch);
      return;
    }
  }
  if (no_args)
  {
    sprintf(buf,"&1Personal colour settings:&n\r\n"
		"&&0. &0%s&n\r\n" "&&1. &1%s&n\r\n" "&&2. &2%s&n\r\n"
		"&&3. &3%s&n\r\n" "&&4. &4%s&n\r\n" "&&5. &5%s&n\r\n"
		"&&6. &6%s&n\r\n" "&&7. &7%s&n\r\n" "&&8. &8%s&n\r\n"
		"&&9. &9%s&n\r\n",
	    colour_headings[0], colour_headings[1], colour_headings[2],
	    colour_headings[3], colour_headings[4], colour_headings[5],
	    colour_headings[6], colour_headings[7], colour_headings[8],
	    colour_headings[9]);
    send_to_char(buf,ch);
  } else {
    for (i=0;i<10;i++)
    {
      // The magic default number
      if (colour_codes[i] == 69)
        set_default_colour(ch,i);
      else if (colour_codes[i] >= 0)
        set_colour(ch,i,colour_codes[i]);
    }
    send_to_char("Colour changed.\r\n",ch);
  }
}

extern int vote_level;
int check_for_event(int event, zone_rnum zone) 
{
  struct event_data *ev;
  for (ev = events.list; ev; ev = ev->next) 
    if (zone != -1) 
    {
      if ((ev->type == event) && (ev->room->zone == zone))
	return 1;
    } else if (ev->type == event)
      return 1;
  return 0;
}

void handle_burglary(struct event_data *ev) 
{
  // check if burglar has evacuated rooms, and act accordingly

  // Also, check if an excessive number of rooms have been burgled,
  // and initiate a curfew in a zone that's been burgled (and is a town)
}

// Artus> Happy Hour.
void handle_happy(struct event_data *ev)
{
  void destroy_event_happy(struct char_data *ch, struct event_data *ev);
  ev->info1 -= 10;
  if (ev->info1 > 0)
    return;
  destroy_event_happy(NULL, ev);
}

// Artus> Have changed this a bit... Should be better now.
void handle_gold_rush(struct event_data *ev)
{
  void send_to_not_zone_world(const char *msg, zone_rnum zone);
  void destroy_event_goldrush(struct char_data *ch, struct event_data *ev);
  extern zone_rnum top_of_zone_table;
  struct obj_data *gold;
  int i, j, k;
  room_rnum curroom;
  obj_rnum currobj;

  if (ev->type != EVENT_GOLD_RUSH)
    return;
  ev->info1 += 10;
  if (!ev->room)
  {
    mudlog("SYSERR: Gold rush has no room attached!", BRF, LVL_IMPL, TRUE);
    return;
  }
  if (((ev->info1 >= 300) || (ev->info2 >= 30)) && (number(1, 30) == 1))
  {
    destroy_event_goldrush(NULL, ev);
    return;
  }
  if ((ev->room->zone < 0) || (ev->room->zone > top_of_zone_table))
  {
    sprintf(buf, "SYSERR: Invalid zone (%d) attached to gold rush.", 
	    ev->room->zone);
    mudlog(buf, BRF, LVL_IMPL, TRUE);
    return;
  }
  if ((currobj = real_object(GOLD_OBJ_VNUM)) < 0)
  {
    sprintf(buf, "SYSERR: Could not load gold object (%d).", GOLD_OBJ_VNUM);
    mudlog(buf, BRF, LVL_IMPL, TRUE);
    return;
  }
  // Anywhere from 1 to 5 gold objs *may* be loaded.
  j = number(1, 5);
  // Since it's not ended, spread a bit more gold around the zone
  for (i = 0; i <= j; i++)
  {
    k = number(zone_table[ev->room->zone].number * 100,
	       zone_table[ev->room->zone].top);
    curroom = real_room(k);
    if (curroom == NOWHERE)
      continue; 
    // Artus> Sector Check...
    if ((BASE_SECT(SECT(curroom)) < SECT_FIELD) || 
	(BASE_SECT(SECT(curroom)) > SECT_MOUNTAIN))
      continue;
    // Load the obj, and give it a random value.
    gold = read_object(currobj, REAL);
    GET_OBJ_VAL(gold, 1) = number(1000, 30000);
    // Artus> Change value based on zone level.
    if (zone_table[ev->room->zone].zflag > 3)
    {
      int lvbit = zone_table[ev->room->zone].zflag;
      REMOVE_BIT(lvbit, ZN_NO_STEAL | ZN_NO_TELE | ZN_NO_GOLDRUSH | 
		 (1 << 27) | (1 << 28) | (1 << 29) | (1 << 30) | (1 << 31));
      if (lvbit > ZN_LR_40) // Double for Level >= 40
	GET_OBJ_VAL(gold, 1) <<= 1;
      if (lvbit > ZN_LR_70) // Quadruple for Lvl >= 70
	GET_OBJ_VAL(gold, 1) <<= 1;
    }
    // Artus> Copy the value to the cost.
    GET_OBJ_COST(gold) = GET_OBJ_VAL(gold, 1);
    ev->info2++;
    obj_to_room(gold, curroom);
  }
}

// Artus> Called by handle_fire.
void incinerate(room_rnum room)
{
  struct obj_data *burning, *next_burn;
  struct char_data *cburning, *next_cburn;
  send_to_room("The room is on fire!\r\n", room);
  if (world[room].contents)
    for (burning = world[room].contents; burning; burning = next_burn)
    {
      next_burn = burning->next_content;
      extract_obj(burning);
    }
  if (world[room].people)
    for (cburning = world[room].people; cburning; cburning = next_cburn)
    {
      next_cburn = cburning->next_in_room;
      if (LR_FAIL(cburning, LVL_GOD))
      {
	if (IS_GHOST(cburning))
	  send_to_char("The flames pass right through you!\r\n", cburning);
	else
	{
	  send_to_char("You burn to a crisp!\r\n", cburning);
	  die(cburning, NULL, "FIRE!");
	}
      } else {
	send_to_char("Fortunately, you are immortal!\r\n", cburning);
	char_from_room(cburning);
	char_to_room(cburning, real_room(1200));
      }
    }
  SET_BIT(RMSM_FLAGS(room), RMSM_BURNED);
}

// Artus> I told you I'd get around to it, Sandii :o)
void handle_fire(struct event_data *ev)
{
  room_rnum burnrnum;
  int burncount = 0, i;
  void destroy_event_fire(struct char_data *ch, struct event_data *ev);
  
  burnrnum = real_room(ev->room->number);
  if (!RMSM_FLAGGED(burnrnum, RMSM_BURNED))
  {
    incinerate(burnrnum);
    return;
  }
  for (burnrnum = 0; burnrnum <= top_of_world; burnrnum++)
  {
    if ((world[burnrnum].zone != ev->room->zone) ||
        RMSM_FLAGGED(burnrnum, RMSM_BURNED) ||
	world[burnrnum].number == 1200)
      continue;
    for (i = 0; i < NUM_OF_DIRS; i++)
      if ((W_EXIT(burnrnum, i)) && 
	  (W_EXIT(burnrnum,i)->to_room != NOWHERE) &&
	  (SECT(W_EXIT(burnrnum,i)->to_room) < SECT_WATER_SWIM) &&
	  (RMSM_FLAGGED(W_EXIT(burnrnum,i)->to_room, RMSM_BURNED)))
	{
	  burncount++;
	  incinerate(burnrnum);
	  break;
	}
    if (burncount > 3)
      break;
  }
  if (burncount < 1)
    destroy_event_fire(NULL, ev);
}

// Artus> Mortal version of events list.
ACMD(do_events)
{
  struct event_data *ev;
  int counter = 0;

  if (IS_NPC(ch))
    return;
  sprintf(buf2, "The following events are running:\r\n\r\n");
  for (ev = events.list; ev; ev = ev->next)
  {
    *buf = '\0';
    if (counter == 1)
      *buf2 = '\0';
    switch (ev->type)
    {
      case EVENT_QUEST:
	sprintf(buf, "%sA '%s' quest is running in %s!\r\n", buf2,
		quest_names[ev->info1], zone_table[ev->room->zone].name);
	counter++;
	break;
      case EVENT_GOLD_RUSH:
	sprintf(buf, "%s%d chunks of gold have been sighted in %s!\r\n", buf2,
	        ev->info2, zone_table[ev->room->zone].name);
	counter++;
	break;
      case EVENT_HAPPY_HR:
	if (ev->info1 <= 60) 
	  sprintf(buf, "%sHappy hour will be over in approximately %d seconds.\r\n", buf2, ev->info1);
	else
	  sprintf(buf, "%sHappy hour will be over in approximately %d minutes and %d seconds.\r\n", buf2, (int)(ev->info1 / 60), ev->info1 % 60);
	counter++;
	break;
      case EVENT_FIRE:
	sprintf(buf, "%s%s is on fire!\r\n", buf2, 
	        zone_table[ev->room->zone].name);
	counter++;
	break;
/*    case EVENT_ELECTION:
      case EVENT_CURFEW:
      case EVENT_BOUNTY_HUNT:
      case EVENT_BURGLARY:
      case EVENT_OVER: */
      default: break;
    }
    if (*buf)
      send_to_char(buf, ch);
  }

  if (counter < 1) 
  {
    send_to_char("There are currently no events running.\r\n", ch);
    return;
  }

  sprintf(buf, "\r\n%d event%s listed.\r\n", counter, (counter==1) ? "s" : "");
  send_to_char(buf, ch);
}

/* Artus - Hear Noises => Listen 
 * 
 * Would like to improve it at some stage, to factor in weights or perhaps
 * levels of mobs to give players more idea..                              */
ACMD(do_listen)
{
  struct char_data *i;
  struct room_data *target;
  int direction;
  int count = 0;
  room_rnum targnum;
  
  if (IS_NPC(ch) || ch->in_room == NOWHERE)
    return;
  
  one_argument(argument, arg);

  if (!(*arg)) {
    send_to_char("Listen in which direction?\r\n", ch);
    return;
  }
  if ((direction = search_block(arg, dirs, FALSE)) < 0) {
    send_to_char("What direction is that??\r\n", ch);
    return;
  }
  if (!world[ch->in_room].dir_option[direction])
  {
    sprintf(buf, "You stick your head against the %s and all you get is a sore ear.\r\n", ((direction < UP) ? "wall" : (direction == UP) ? "ceiling" : "floor"));
    send_to_char(buf, ch);
    return;
  }
  if ((targnum = world[ch->in_room].dir_option[direction]->to_room) == NOWHERE) 
  {
    send_to_char("There doesn't seem to be an exit there.\r\n", ch);
    return;
  }
  target = &world[targnum];
  if (!(target))
  {
    send_to_char("You can't seem to listen in that direction.\r\n", ch);
    return;
  }
  if (basic_skill_test(ch, SKILL_LISTEN, 1) == 0) 
    return;
  if (!target->people)
  {
    send_to_char("That room sounds void of life.\r\n", ch);
    return;
  }
  
  for (i = target->people; i; i = i->next_in_room)
    count++;
  if (count == 1)
    send_to_char("You can make out one lonesome creature.\r\n", ch);
  else if (count < 5)
  {
    sprintf(buf, "You can make out %d creatures", count);
    switch (direction)
    {
      case NORTH:
      case EAST:
      case SOUTH:
      case WEST:
	sprintf(buf, "%s to the %s.\r\n", buf, dirs[direction]);
	break;
      case UP:
	strcat(buf, " up above.\r\n");
	break;
      case DOWN:
	strcat(buf, " down below.\r\n");
	break;
      default:
	strcat(buf, " in an unknown direction.\r\n");
        break;
    }
    send_to_char(buf, ch);
  } else
    send_to_char("There is a lot of activity happening there!\r\n", ch);
  return;
}

void handle_curfew(struct event_data *ev)
{
// TODO - Artus, I guess Tali's never going to get around to it :p~
}

bool is_player_bountied(struct char_data *ch)
{
  struct event_data *ev;
  long lTargetId = GET_IDNUM(ch);

  for (ev = events.list; ev; ev = ev->next)
    if ((ev->type == EVENT_BOUNTY_HUNT) && (ev->chID == lTargetId))
      return TRUE;
  return FALSE;
}

/* Starts a bounty hunt event */
void init_bounty_hunt()
{
  struct descriptor_data *d;
  struct event_data *ev;
  int found = FALSE;

  // Go through every player
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
      continue;
    if(!LR_FAIL(d->character, LVL_IS_GOD))
      continue;

    // See if they're criminals
    if (GET_SOCIAL_STATUS(d->character) == SOCIAL_CRIMINAL)
    {
      // See if they're already bountied up
      for (ev = events.list; ev; ev = ev->next)
	if ((ev->type == EVENT_BOUNTY_HUNT) &&
	    (ev->chID == GET_IDNUM(d->character)))
	{
	  found = TRUE;
	  break;
	}
      // If they're not already bountied, chance of doing so
      if (!found)
      {
	// 10% Random chance of the law putting up a bounty
	if (number(1, 10) != 1)
	  continue;
	sprintf(buf, "%s bountied by the law.",GET_NAME(d->character));
	mudlog(buf, BRF, LVL_GOD, FALSE);
	CREATE(ev, struct event_data, 1);
	ev->chID = GET_IDNUM(d->character);
	ev->info1 = GET_LEVEL(d->character) * 20000 * number(1, 3);
	ev->info3 = THE_LAW; 
	ev->type = EVENT_BOUNTY_HUNT;
	ev->room = &world[real_room(BOUNTY_RETURN_ROOM)];
	add_mud_event(ev);
      }
      // Reset search sentinel
      found = FALSE;
    }	
  }
}

#if 0 
// Artus> I think i'll re-write this, too :o)
void init_gold_rush() 
{
  struct event_data *ev;
  int counter = 0, sector = 0;
  room_rnum rnum_room;
  CREATE(ev, struct event_data, 1);
  ev->chID = -1;
  ev->type = -1;
  ev->info1 = 0;
  ev->info2 = 0;
  // Get a room, checking it for a nice sector type, and a non-gold rush already zone
  for (int i = 0; i < 10; i++)
  {
    rnum_room = number(1, top_of_world);
    ev->room = &world[rnum_room];
    // Ignore !GOLDRUSH zones  (Moved from before the counter check)
    if (IS_SET(zone_table[ev->room->zone].zflag, ZN_NO_GOLDRUSH))
      continue;
    // basic_mud_log("init_gold_rush: attempting to use room rnum (%d) vnum (%d)", rnum_room, ev->room->number);
    // if room_rnum happens to be NOWHERE, set to one of the sector
    // types define at the bottom of the do while loop so we try
    // again and dont go outside the world[] array limits
    if (rnum_room == NOWHERE)
      continue;
    sector = BASE_SECT(SECT(rnum_room));
    if ((sector < SECT_FIELD) || (sector > SECT_MOUNTAIN))
      continue;
    if (check_for_event(EVENT_GOLD_RUSH, ev->room->zone))
      continue;
    ev->type = EVENT_GOLD_RUSH;
    break;
  }
  if(ev->type != EVENT_GOLD_RUSH)
  {
    free(ev);	
//    basic_mud_log("GoldRush: Unable to create event, counter limit reached.");
    return;
  }	
  add_mud_event(ev);
  // Notify the zone
  sprintf(buf, "&YA GOLD RUSH is on at %s!&n\r\n", zone_table[ev->room->zone].name);
  send_to_zone(buf, ev->room->zone);
  sprintf(buf, "&WYou hear rumours of a gold rush at %s!&n\r\n",zone_table[ev->room->zone].name);
  send_to_not_zone_world(buf, ev->room->zone);
}
#endif

// Artus> My way..
void init_gold_rush()
{
  struct event_data *ev = NULL;
  int sector = 0;
  room_rnum rnum_room;
  // Get a room, checking it for a nice sector type, and a non-gold rush already zone
  for (int i = 0; i < 10; i++)
  {
    rnum_room = number(1, top_of_world);
    if (IS_SET(zone_table[world[rnum_room].zone].zflag, ZN_NO_GOLDRUSH))
      continue;
    sector = BASE_SECT(SECT(rnum_room));
    if ((sector < SECT_FIELD) || (sector > SECT_MOUNTAIN))
      continue;
    if (check_for_event(EVENT_GOLD_RUSH, world[rnum_room].zone))
      continue;
    CREATE(ev, struct event_data, 1);
    ev->chID = 0;
    ev->type = EVENT_GOLD_RUSH;
    ev->room = &world[rnum_room];
    ev->info1 = 0;
    ev->info2 = 0;
    ev->info3 = 0;
    strcpy(ev->desc, "Random gold rush.\r\n");
    break;
  }
  if (!ev)
    return;
  add_mud_event(ev);
  // Notify the zone
  sprintf(buf, "&YA GOLD RUSH is on at %s!&n\r\n", zone_table[ev->room->zone].name);
  send_to_zone(buf, ev->room->zone);
  sprintf(buf, "&WYou hear rumours of a gold rush at %s!&n\r\n",zone_table[ev->room->zone].name);
  send_to_not_zone_world(buf, ev->room->zone);
}

// TODO: Finish with quest system
void init_event()
{
  // About a 0.1% chance of an event
  switch(number(1, 1000))
  {
    case 1: init_bounty_hunt(); break;
    case 2: init_gold_rush(); break;
    default: break;
  }
}

bool burglar_left_scene(Burglary *burglary)
{
  struct char_data *burglar = get_player_by_id(burglary->chID); 
  if (burglar == NULL)
    return TRUE; 
  int nBurglarRoom = IN_ROOM(burglar);
  int nCount = burglary->CountBurgledRooms();
  
  if (nCount == 0)
    return TRUE;	// Could have cleared burglary out
  for (int i = 0; i < nCount; i++)
    if (nBurglarRoom == burglary->burgledRooms[i].rNum)
      return FALSE;
  return TRUE;	// Not found in burgling rooms
}

/* Strength = percentage, ie 1.3 = 30%, 0.9 = -10% */
void apply_burglar_stats_to_mob(struct char_data *mob, long lID, float fStrength)
{
  struct char_data *ch = get_player_by_id(lID);
  if (ch == NULL)
    return;
  if (fStrength <= 0.0)
    return;
  int nLevel = int((number(GET_LEVEL(ch) -3, GET_LEVEL(ch) + 3) +
	           int(GET_MODIFIER(ch))) * fStrength);
  if (nLevel > 250)
    nLevel = 250;
  if (nLevel <= 0)
    nLevel = 1 + number(0,2);
  GET_LEVEL(mob) = nLevel;
  GET_AC(mob) = int(GET_AC(ch) * fStrength);
  GET_MAX_HIT(mob) = int(number(int(GET_MAX_HIT(ch) * 0.9),
	                        int(GET_MAX_HIT(ch) * 1.1)) * fStrength);
  GET_HITROLL(mob) = int(GET_HITROLL(ch) * fStrength);
  GET_DAMROLL(mob) = int((GET_DAMROLL(ch) + GET_LEVEL(ch)) * fStrength);
  mob->mob_specials.damnodice = int((5 + int(GET_MODIFIER(ch))) * fStrength);
  int nDamSizeDice = int(GET_LEVEL(ch) * fStrength);
  mob->mob_specials.damsizedice = nDamSizeDice > 120 ? 120 : nDamSizeDice;
  GET_GOLD(mob) = int((100 * GET_LEVEL(ch) * (int(GET_MODIFIER(ch))+ 1)) *
                      fStrength);
  GET_EXP(mob) = int((number(int(level_exp(ch, GET_LEVEL(ch)) * 0.02), 
		             int(level_exp(ch, GET_LEVEL(ch)) * 0.05))) *
                     fStrength);
  GET_HIT(mob) = GET_MAX_HIT(mob);
}

// The random burglar's generic mob vnum
#define RANDOM_BURGLAR		30900

void create_burgle_encounter(struct char_data *ch)
{
  struct char_data *burglar = read_mobile(RANDOM_BURGLAR, VIRTUAL);
  
  if (burglar == NULL)
  {
    mudlog("Random Burgle Encounter - cannot read mobile.", NRM, LVL_GOD, TRUE);
    return;
  }
  // Beef the burglar up
  apply_burglar_stats_to_mob(burglar, ch->in_room, 1.0);
  GET_HIT(burglar) = GET_MAX_HIT(burglar);
  char_to_room(burglar, ch->in_room);
  hit(burglar, ch, TYPE_UNDEFINED);
}

void handle_burglaries()
{
  int nDiscoveryChance;
  Burglary *current;
  
  if (burglaries == NULL)
    return;	// Nada
  // If it's daytime, chance of discovery is greater
  if (time_info.hours >= 0 && time_info.hours < 6)
    nDiscoveryChance = 2;
  else
    nDiscoveryChance = 5;
  current = burglaries;
  while (current)
  {
    struct char_data *burglar = get_player_by_id(current->chID);
    // Firstly, see if player has exited burglary rooms, and end it if so
    if (burglar_left_scene(current))
    {
      current->Clear();
      Burglary *tmp = current->next;	// Back up the next burglary
      remove_burglary(current);	// Remove this one, which clears 'next'
      current = tmp;			// Restore next (if any)
      continue;	// Done with this burglary, its history
    }
    // They're still around, see if the cops are now onto them
    if (number(1, 100 + GET_SKILL(burglar, SKILL_BURGLE)) <= nDiscoveryChance && !is_player_bountied(burglar))
    {
      // Set up a bounty on them
      struct event_data *ev;
      CREATE(ev, struct event_data, 1);
      // Set it up
      ev->chID = current->chID; 			// Target
      ev->info1 = GET_LEVEL(burglar) * 5000 * (int(GET_MODIFIER(burglar)) +1);// Bounty 
      ev->info3 = THE_LAW; 				// Bounty initiator ID	
      ev->type = EVENT_BOUNTY_HUNT;			// Ahuh
      ev->room = &world[real_room(BOUNTY_RETURN_ROOM)];// &world[burglar->in_room];
      // Add it to the list
      add_mud_event(ev);
      // Reduce the players social status severely
      GET_SOCIAL_POINTS(burglar) -= 10;
      send_to_char("&rYour actions have been noticed!&n\r\n", burglar);
      send_to_char("... a bounty has been set on you.\r\n", burglar);
      current = current->next;
      continue;
    }
    // Some chance that they're going to be cutting into someone's territory
    if (number(1,100) == 1)
    {
      send_to_char("&bYou hear a familiar rustle....&n\r\n", burglar);
      send_to_char("... you have cut in on someone else's burglary!\r\n",
	           burglar);	
      create_burgle_encounter(burglar);
      int nExpGain = number(int(level_exp(burglar, GET_LEVEL(burglar)) * 0.01),
			    int(level_exp(burglar, GET_LEVEL(burglar)) * 0.03));
      if (nExpGain > 0)
      {
	sprintf(buf, "You learn from the experience! (%d exp)\r\n", nExpGain);
	send_to_char(buf, burglar);
	GET_EXP(burglar) += nExpGain;
      }
    }  
    current = current->next;
  }
}

/* Function to go through all events and take appropriate action on them */
void handle_events() 
{

  struct event_data *ev, *nextev;
	
  init_event();		// Random chance of beginning a new event
  // For every event
  for(ev = events.list; ev; ev = nextev)
  {
    nextev = ev->next;
    switch(ev->type)
    {
      case EVENT_BURGLARY:  handle_burglary(ev);    break;
      case EVENT_GOLD_RUSH: handle_gold_rush(ev);   break;
      case EVENT_CURFEW:    handle_curfew(ev);      break;
      case EVENT_QUEST:     handle_quest_event(ev); break;
      case EVENT_FIRE:      handle_fire(ev);        break;
      case EVENT_HAPPY_HR:  handle_happy(ev);       break;
      default: break;
    }
  }
  ev=events.list;
  // Clean up any events that have ended
  while(ev)
  {
    nextev = ev->next;		
    if (ev->type == EVENT_OVER)
      remove_mud_event(ev);
    ev = nextev;
  }	
  // Separate handler for burglaries now that they have their own class
  handle_burglaries();	
}


void show_election_status(struct char_data *ch)
{
  struct event_data *ev;
  // Get the event
  for (ev = events.list; ev; ev = ev->next) 
    if (ev->type == EVENT_ELECTION)
      break;
  // A double check, just in case
  if (ev == NULL)
  {
    send_to_char("No election is being held at this time.\r\n", ch);
    return;
  }
  sprintf(buf, "Current election status is #1: %d, #2: %d, #3: %d.\r\n", 
	  ev->info1, ev->info2, ev->info3);
  send_to_char(buf, ch);
}

ACMD(do_vote)
{
  struct event_data *ev;
  if (IS_NPC(ch))
  {
    send_to_char("Hate to say it, but some votes just don't count!\r\n", ch);
    return;
  }
  one_argument(argument, arg);
  if (*arg && strcmp(arg, "status") == 0 && check_for_event(EVENT_ELECTION, -1))
  {
    show_election_status(ch);
    return;
  }
  // Check if player can vote
  if ((GET_VOTED(ch) == TRUE) || LR_FAIL(ch, vote_level) ||
      !check_for_event(EVENT_ELECTION, -1))
  {
    send_to_char("You cannot place a vote at this time.\r\n", ch);
    return;
  }
  one_argument(argument, arg);
  if (!*arg)
  {
    send_to_char("You are eligible to vote.\r\n", ch);
    return;
  }
  if (!isdigit(arg[0]))
  {
    send_to_char("Your vote must be a digit or 'status'.\r\n", ch);
    return;
  }
  // Get the event
  for (ev = events.list; ev; ev = ev->next) 
    if (ev->type == EVENT_ELECTION)
      break;
  switch(atoi(arg))
  {
    case 1:
      ev->info1 = ev->info1 + 1;
      break;
    case 2:
      ev->info2 = ev->info2 + 1;
      break;
    case 3:
      ev->info3 = ev->info3 + 1;
      break;
    default:
      send_to_char("Your vote must be either '1', '2' or '3'.\r\n", ch);
      return;
  }
  send_to_char("Your vote has been registered. Thank you for your participation.\r\n",ch);
  GET_VOTED(ch) = TRUE;
}

/* NOTE: Requires a pre-made character named ' Adjudicator' */	
ACMD(do_worship)
{
  struct char_data *divinity;
  one_argument(argument, arg);
  if (!*arg)
  {
    send_to_char("You fall to your knees and beg for forgiveness.\r\n",ch);
    act("$n falls to $s knees and grovels to the mighty.\r\n", FALSE,
	ch, 0, 0, TO_ROOM);
    return;
  }
  if ((divinity = generic_find_char(ch, arg, FIND_CHAR_ROOM)) != NULL)
  {
    act("You fall to your knees before $N and beg for forgiveness.\r\n", false,
	ch, 0, divinity, TO_CHAR);
    act("$n falls to $s knees before you, begging for forgiveness.\r\n", true, 
	ch, 0, divinity, TO_VICT);
    act("$n gets down on $s knees before $N.\r\n", true, ch, 0, divinity, 
	TO_NOTVICT);
    return;
  }
  send_to_char("You wish there was noone worthy of your worshipping.\r\n", ch);
  return;
  
#if 0 // Artus> This is not implemented.
  // Maintain a social for it
  if (!*arg)
  {
    send_to_char("You fall to your knees and beg for forgiveness.\r\n",ch);
    act("$n falls to $s knees and grovels to the mighty.\r\n", FALSE,
	ch, 0, 0, TO_ROOM);
    return;
  }
  basic_mud_log("Worshipping");
  // Check that the player has some points to give away,
  // before bothering to check anything
  if (GET_WORSHIP_POINTS(ch) <= 0)
  {
    send_to_char("You pray to the divine, in hope.\r\n", ch);
    // Notify the target anyway, if they're online.
    if ((divinity = get_player_online(ch, arg, FIND_CHAR_WORLD) ) !=NULL) 
      act("You were worshipped by $n, but they had no power left to give you.",
	  FALSE, ch, 0, divinity, TO_VICT);
    else
    { // Check if they exist
      CREATE(divinity, struct char_data, 1);
      clear_char(divinity);
      if (load_char(arg, &tmp) < 0)
      {
	send_to_char("No such divinity exists!\r\n", ch);
	free(divinity);
	return;
      }	
      // they exist, send them a mail
      store_to_char(&tmp, divinity);
      sprintf(buf, "You were worshipped by %s, but they had no power to give you.\r\n", GET_NAME(ch));	
      store_mail(get_id_by_name(GET_NAME(divinity)),
		 get_id_by_name("Adjudicator"), buf);
      char_to_room(divinity, 0);
      extract_char(divinity);
      return;
    }
    return; 
  }
  send_to_char("Worship function not completed yet. See your nearest implementor.\r\n",ch);
#endif
}

/* Command to display the current bounties available */
ACMD(do_bounties)
{
  int bounty_count = 0, amount = 0;
  struct event_data *ev = NULL;
  struct char_data *vict = NULL;
  struct obj_data *obj = NULL;

  // NPC's are alloted bounty hunting differently
  if (IS_NPC(ch))
    return;

  // Check that there is some kind of valid item in the room
  /* Do this whenever */

  // Check that the player's class is an appropriate class
/*	if( class != CLASS_WARRIOR && class != CLASS_BATTLEMAGE &&
      class != CLASS_PALADIN && class != CLASS_NIGHTBLADE) {
	  send_to_char("Better leave the hunting to the pro's.\r\n",ch);
	  return;
  }
*/
  /* Check for skill instead of class here */
  // If there are no arguments, just list the bounties
  if (!*argument)
  {
    // Go through the events, and list the details
    for (ev = events.list; ev; ev = ev->next)
    {
      // Sift out the inappropriate events
      if (ev->type != EVENT_BOUNTY_HUNT)
	continue;
      // Get the bounty victim's name
      sprintf(buf, "(%2d) Bounty open for: &r%s&n.", bounty_count + 1,
	      get_name_by_id(ev->chID));
      // The reward
      sprintf(buf + strlen(buf), "\r\n     Reward: &Y%d&n", ev->info1);
      // Who started it
      sprintf(buf + strlen(buf), " coins on behalf of %s.", 
	      (ev->info3 != THE_LAW ? get_name_by_id(ev->info3): "the law")); 
      // The room to return to
      sprintf(buf + strlen(buf), "\r\n     Collect at: %s\r\n", ev->room->name);
      send_to_char(buf, ch);
      bounty_count++;
    }
    if (bounty_count == 0)
      send_to_char("No bounties available at this time.\r\n",ch);
    return;
  } // No arguments

  /* Check here, if they're bountying an item?! -- easily done
   * but have to break up the function into multiple parts
   * so I'll do it later.
   */
  // Grab the arguments
  two_arguments(argument, arg, buf1);
  /* Check the first argument for what type of bounty they're after*/
  if (strcmp(arg, "item") == 0)
    return;
  /* If there is no second argument, attempt to remove the bounty
   * on the player indicated, assuming they set up the bounty in 
   * the first place, otherwise, they're trying to collect.
   */
  if (!*buf1)
  {
    // Are they trying to cancel or collect the bounty?
    for(ev = events.list; ev; ev = ev->next)
    {
      // Valid bounty event?
      if (ev->type != EVENT_BOUNTY_HUNT)
	continue;
      // Is the event target the argument?
      if (get_id_by_name(arg) == ev->chID)
      {
	// Did they set it up? If so, cancel.
	if (GET_IDNUM(ch) == ev->info3)
	{
	  act("$n removes a bounty.", FALSE, ch, 0, 0,TO_ROOM);
	  int nGoldBack = int(ev->info1 - (0.10*ev->info1));
	  send_to_char("You cancel the bounty, forfeitting your cancellation fee.\r\n", ch);
	  sprintf(buf, "&Y%d&n coins are returned.\r\n", nGoldBack);
	  send_to_char(buf, ch);
	  // Money back, less 10%
	  GET_GOLD(ch) += nGoldBack;
	  ev->type = EVENT_OVER;
	  return;
	} // If they set it up	
	one_argument(argument, arg); // Dunno why I need this.
	// If they didn't set it up, they're trying to collect
	if ((obj = generic_find_obj(ch, "corpse", FIND_OBJ_INV)) == NULL)
	{
	  send_to_char("You hopelessly try to collect the bounty.\r\n", ch);
	  return;
	} // Do they have the appropriate item?
	arg[0] = UPPER(arg[0]);
	sprintf(buf, "the corpse of %s", arg);
	if (strcmp(obj->short_description, buf) != 0)
	{
	  send_to_char("Hmm, that doesn't look like the right corpse.\r\n",
		       ch);
	  return;
	} 
	// Successful bounty collection!
	amount = (int)(ev->info1 * 0.95);
	act("$n hands over a bounty, collecting %s reward!\r\n",
	    FALSE, ch, 0, 0, TO_ROOM);
	sprintf(buf, "You hand over your bounty, collecting &Y%d&n coins!\r\n",
		amount);	
	send_to_char(buf, ch);
	// Extract the item
	extract_obj(obj);
	GET_GOLD(ch) += amount;
	// Notify initiator
	if (ev->info3 != THE_LAW)
	{
	  sprintf(buf, "Your bounty on %s for %d was collected!\r\n",
		  get_name_by_id(ev->chID), ev->info1);
	  send_to_char(buf, generic_find_char(ch, get_name_by_id(ev->info3),
					      FIND_CHAR_WORLD)); 
	} else {
	  // Else - Run a  message on the board, declaring capture
	  /*TODO: Message on bounty board */
	  // DM - hmm removing quest points for the mean time
	  // (commented out quest points)
	  send_to_char("The lawful authorities thank you and reward you with an additional Quest Point!\r\n", ch);
	  //GET_QUEST_POINTS(ch) += 1;
	}
	// Finally, delete the event
	ev->type = EVENT_OVER;
	return; 
      } // If this event is the one with the player
    }  // For every event
    return;
  }
  /* New Bounty!  */
  // character visible?
  if (!(vict = generic_find_char(ch, arg, FIND_CHAR_WORLD)))
  {
    send_to_char("Put a bounty up on who!?\r\n", ch);
    return;
  }
  if (!isdigit(buf1[0]))
  {
    send_to_char("You have to put up a cash amount.\r\n", ch);
    return;
  }
  amount = atoi(buf1);
  // Check their balance
  if ((GET_GOLD(ch) < amount) && (GET_BANK_GOLD(ch) < amount))
  { 
    send_to_char("You don't have the cash resources for that right now.\r\n",
	         ch);
    return;
  }
  // Validate the amount is a logical one
  if ((amount < (GET_LEVEL(vict) * 10000)) || (!LR_FAIL(vict, LVL_GOD)))
  {
    send_to_char("Get real, noone would risk that bounty for that piddling cash reward!\r\n", ch);
    return;
  }
  // Get the cash
  if (GET_GOLD(ch) >= amount)
    GET_GOLD(ch) -= amount;
  else
    GET_BANK_GOLD(ch) -= amount;
  // Create the event
  CREATE(ev, struct event_data, 1);
  // Set it up
  ev->chID = GET_IDNUM(vict); 		// Target
  ev->info1 = amount;	// Bounty offered
  ev->info3 = get_id_by_name(GET_NAME(ch)); // Bounty initiator ID	
  ev->type = EVENT_BOUNTY_HUNT;	// Ahuh.
  ev->room = &world[real_room(BOUNTY_RETURN_ROOM)]; //&world[ch->in_room];
  // Add it to the list
  add_mud_event(ev);
  // Notify the room
  act("$n puts up a new bounty.", FALSE, ch, 0, 0, TO_ROOM);
  sprintf(buf, "You put up a bounty on &B%s&n for &Y%d&n coins.\r\n", 	
	  GET_NAME(vict), amount);
  send_to_char(buf, ch);

}
