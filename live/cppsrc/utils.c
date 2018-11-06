/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
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
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "colour.h"

extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct time_data time_info;
extern struct room_data *world;
extern struct event_list events;

struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);

extern int level_exp(struct char_data *ch, int level);
extern struct questlist_element *questlist_table;


/* DM - temporary localtime, till the deb libs are fixed
struct tm *localtime(const time_t *timer) {
  struct tm *tmptime;

  tmptime = (struct tm *)malloc(sizeof(struct tm));

  tmptime->tm_sec = 0;
  tmptime->tm_min = 0;
  tmptime->tm_hour = 17;
  tmptime->tm_mday = 5;
  tmptime->tm_mon = 1;
  tmptime->tm_year = 2002;
  tmptime->tm_wday = 6;
  tmptime->tm_yday = 5;
  tmptime->tm_isdst = 1;

  return tmptime;
} */

#ifdef NO_LOCALTIME
/* Artus> Alternative localtime, this one won't crash out :o)..
 * Pretty much ripped the __offtime() function, but with a hardcoded offset.  */
//int jk_localtime(struct tm *lt, long int t)
int jk_localtime_now(struct tm *lt)
{
  return jk_localtime(lt, time(0));
}

int jk_localtime(struct tm *lt, long int t)
{ 
#ifndef SECS_PER_DAY
#define SECS_PER_DAY 86400
#define SECS_PER_HOUR 3600
#endif
  long int days, rem, y;
  const unsigned short int *ip;
  const unsigned short int mon_yday[2][13] =
  {
    /* Normal years.  */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
  };

  days = t / SECS_PER_DAY;
  rem = t % SECS_PER_DAY;
  rem += 10 * SECS_PER_HOUR;
  while (rem < 0)
    {
      rem += SECS_PER_DAY;
      --days;
    }
  while (rem >= SECS_PER_DAY)
    {
      rem -= SECS_PER_DAY;
      ++days;
    }
  lt->tm_hour = rem / SECS_PER_HOUR;
  rem %= SECS_PER_HOUR;
  lt->tm_min = rem / 60;
  lt->tm_sec = rem % 60;
  /* January 1, 1970 was a Thursday.  */
  lt->tm_wday = (4 + days) % 7;
  if (lt->tm_wday < 0)
    lt->tm_wday += 7;
  y = 1970;

#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV (y, 4) - DIV (y, 100) + DIV (y, 400))

  while (days < 0 || days >= (__isleap (y) ? 366 : 365))
    {
      /* Guess a corrected year, assuming 365 days per year.  */
      long int yg = y + days / 365 - (days % 365 < 0);

      /* Adjust DAYS and Y to match the guessed year.  */
      days -= ((yg - y) * 365
	       + LEAPS_THRU_END_OF (yg - 1)
	       - LEAPS_THRU_END_OF (y - 1));
      y = yg;
    }
  lt->tm_year = y - 1900;
  if (lt->tm_year != y - 1900)
    return 1;
  lt->tm_yday = days;
  ip = mon_yday[__isleap(y)];

  for (y = 11; days < (long int) ip[y]; --y)
    continue;
  days -= ip[y];
  lt->tm_mon = y;
  lt->tm_mday = days + 1;
  
  return 0;
}
#endif


// DM - quest eq check
// returns TRUE if the character is allowed to use the item,
// FALSE otherwise.
bool quest_obj_ok(struct char_data *ch, struct obj_data *obj) {

  extern struct index_data *obj_index;
  
  if (ch == NULL || obj == NULL) {
    mudlog("NULL ch or obj passed to quest_obj_ok().", NRM, LVL_ANGEL, TRUE);
    return FALSE;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_QUEST) {
    return TRUE;
  }
 
  for (int i = 0; i < MAX_QUEST_ITEMS; i++) {
    if (GET_QUEST_ITEM(ch, i) == GET_OBJ_VNUM(obj)) {
      return TRUE;
    }
  }
  
  return FALSE;
}

// Returns the maximum stat value for the given type of stat based on the
// character race (stats) and class (points)
int max_stat_value(struct char_data *ch, int stat)
{
  if (IS_NPC(ch))
  {
    basic_mud_log("SYSERR: mobile passed to max_stat_value");
    return 25;
  }
  
  // Impose some limits for gods
  if (!LR_FAIL(ch, LVL_IS_GOD))
    switch (stat) 
    {
      case STAT_INT:
      case STAT_WIS:
      case STAT_STR:
      case STAT_CON:
      case STAT_DEX:
      case STAT_CHA:
        return 25;
      case STAT_HIT:
      case STAT_MANA:
      case STAT_MOVE:
        return 32000;
      default:
        basic_mud_log("SYSERR: Invalid stat value passed to max_stat_value"); 
        return 25;
    }

  /* Masters max stat values are 21 */
  if (GET_CLASS(ch) == CLASS_MASTER) 
    switch (stat) 
    {
      case STAT_INT:
      case STAT_WIS:
      case STAT_STR:
      case STAT_CON:
      case STAT_DEX:
      case STAT_CHA:
        return 21;
      case STAT_HIT:
      case STAT_MANA:
      case STAT_MOVE:
        return (int) ((pc_max_class_points[(int)GET_CLASS(ch)][stat-6] * GET_LEVEL(ch))
                      / 100);
      default:
        basic_mud_log("SYSERR: Invalid stat value passed to max_stat_value"); 
    }
  
  /*
   * Max stat limits obtained directly from pc_max_race_stats array
   */
  switch (stat)
  {
    case STAT_INT:
    case STAT_WIS:
    case STAT_STR:
    case STAT_CON:
    case STAT_DEX:
    case STAT_CHA:
      return pc_max_race_stats[GET_RACE(ch)][stat];

    /*
     * Max point limit is linearly based on the max point values for the given 
     * class:
     *
     * (max point limit * char level) / 100
     */
    case STAT_HIT:
    case STAT_MANA:
    case STAT_MOVE:
      return (int) ((pc_max_class_points[(int)GET_CLASS(ch)][stat-6] * 
	             GET_LEVEL(ch)) / 100);

    default:
      basic_mud_log("SYSERR: Invalid stat value passed to max_stat_value"); 
      break;
  }
  // Doh.
  return 0;
}

// Returns the cost to train the given stat 1 time
int train_cost(struct char_data *ch, int stat, int curr_value)
{
  int tempi1, tempi2;
  float tempf1, tempf2;

  if (IS_NPC(ch))
  {
    basic_mud_log("SYSERR: mobile passed to train_cost");
    return 99999999;
  }

  // Not that this should really get used, but it wont cost gods jack to train
  if (!LR_FAIL(ch, LVL_ISNOT_GOD))
    return 0;

  /*
   * Training costs for stats INT WIS STR CON DEX CHA:
   * stat_costs[max stat value - current value] * stat_mods[current value]
   *
   * ie. a function of stat_costs (actual cost for max stat difference) and 
   * stat_mods (relative costs for actual stat value)
   */
  switch (stat)
  {
    case STAT_INT:
    case STAT_WIS:
    case STAT_STR:
    case STAT_CON:
    case STAT_DEX:
    case STAT_CHA:
      tempi1 = stat_costs[max_stat_value(ch, stat) - curr_value]; 
      tempf1 = stat_mods[curr_value + 1];
      tempi2 = (int) ((float)tempi1 * tempf1);

//      basic_mud_log("stat_costs[max (%d) - current (%d)] = %d", max_stat_value(ch, stat), curr_value, tempi1);
//      basic_mud_log("stat_mods[stat (%d) + 1] = %f", curr_value, tempf1);
//      basic_mud_log("1 * 2 = %d", tempi2);

      return (tempi2);

    /*
     * Training costs for HMV points:
     * current points   level_exp
     * -------------- * ----------
     *   max points     level * 10
     *
     * perform calculation as (so we dont go over any limits in calculation):
     *   (current value / (level * 10))
     *     *
     *   (level_exp / max value)
     */
     
    case STAT_HIT:
    case STAT_MANA:
    case STAT_MOVE:
      //hit_perc = GET_REAL_STAT(ch, stat) * level_exp(ch, GET_LEVEL(ch));
      //lvl_perc = max_stat_value(ch, stat) * GET_LEVEL(ch);

      tempf1 = ((float)curr_value / (GET_LEVEL(ch) * 10));
      tempf2= ((float)level_exp(ch, GET_LEVEL(ch)) / max_stat_value(ch, stat));

//      basic_mud_log("current point value (%d) / (char level (%d) * 10) = %0.2f", curr_value, GET_LEVEL(ch), tempf1);
//      basic_mud_log("level_exp (%d) / max stat val (%d) = %0.2f", level_exp(ch, GET_LEVEL(ch)), max_stat_value(ch, stat), tempf2); 

      tempf2 = tempf2 * tempf1;

//      basic_mud_log("1 / 2 = %0.2f", tempf2);

      return (int)(tempf2);

    default:
      basic_mud_log("SYSERR: Invalid stat value passed to max_stat_value"); 
      break;
  }
  // Doh.
  return (99999999);
}

  // returns -1 if where position is not finger,
  //          0 if where position is finger, but char cant use that position 
  //          1 if where position is finger, and char can use that position
  int can_wear_finger(struct char_data *ch, int where) {

    const byte finger_positions[NUM_CLASSES] = {
      3, // Mage
      2, // Cleric    
      2, // Thief
      1, // Warrior
      5, // Druid
      3, // Priest
      3, // Nightblade
      4, // Battlemage
      4, // Spellsword
      3, // Paladin
      5  // Master
    };
	  
    // Class restricted positions - fingers 
    if (where == WEAR_FINGER_1 || where == WEAR_FINGER_2) { 
      return (where <= finger_positions[(int)GET_CLASS(ch)]);
    }

    // (reminder - WEAR_FINGER_3 is 18, and does not procede WEAR_FINGER_2 = 2
    // hence difference is 15 (WEAR_FINGER_3 (18) - 15 = 3
    if (where >= WEAR_FINGER_3 && where <= WEAR_FINGER_5) { 
      return ((where-15) <= finger_positions[(int)GET_CLASS(ch)]);
    }

    return -1;
}

int eq_pos_ok(struct char_data *ch, int where)
{
  // mobs can wear eq anywhere - should we limit this?
  if (IS_NPC(ch))
    return TRUE; 
  switch (can_wear_finger(ch, where))
  {
    case 0: // wear position is finger, but position restricted by class
      break;
    case 1: // wear position is finger, can use position
      return TRUE;
    // wear position is not finger
    default:       
      // Race restricted positions - rest
      // Use the mask defined in the array for each race, check if the wear
      // bit is set. 
      if (IS_SET(pc_race_eq_masks[GET_RACE(ch)], (1 << where)))
	return TRUE;
  }
  if (GET_UNHOLINESS(ch) > 0)
    return TRUE;
  return FALSE;
}


int is_axe(struct obj_data *obj)
{
  	char *desc = obj->name;
	char *keywords[3] = {
		"AXE",
		"HATCHET",
		"AX"
	};
	// Only if its a weapon.
	if (!(CAN_WEAR(obj, ITEM_WEAR_WIELD)))
	  return FALSE;

	for (int i = 0; i < 3; i++)
	{
		half_chop(desc, buf2, buf);
		while(*buf2)
		{	
			toUpper((char *)buf2);
			if( strcmp(buf2, keywords[i]) == 0)
				return (TRUE); 

			half_chop(buf, buf2, buf);
		} 
	}

	return (FALSE);
}

int is_blade(struct obj_data *obj)
{
  	char *desc = obj->name;
	char *keywords[5] = {
		"SWORD",
		"BLADE",
		"KNIFE",
		"SCIMITAR",
		"DAGGER"
	};

	for (int i = 0; i < 5; i++)
	{
		half_chop(desc, buf2, buf);
		while(*buf2)
		{	
			toUpper((char *)buf2);
			if( strcmp(buf2, keywords[i]) == 0)
				return (TRUE); 

			half_chop(buf, buf2, buf);
		} 
	}

	return (FALSE);
}

char *toUpper(char *oStr)
{
   int nSize = strlen(oStr), i;

   for(i = 0; i < nSize; i++)
     oStr[i] = toupper(oStr[i]);

   return oStr;
}

/* JA is_wearing function to check if player is wearing a certain item type */
int is_wearing(struct char_data *ch, int item_type)
{
  int i;
 
  for (i = 0; i < NUM_WEARS; i++)
        {
                if (ch->equipment[i])
        if (GET_OBJ_TYPE(ch->equipment[i]) == item_type)
          return(1);
        }
  return(0);
}
 
/* JA is_carrying function to check if player is carrying a certain item type */
int is_carrying(struct char_data *ch, int item_type)
{
  struct obj_data *obj;
 
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == item_type)
      return(1);
  return(0);
} 

/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  /* error checking in case people call number() incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
    basic_mud_log("SYSERR: number() should be called with lowest, then highest. number(%d, %d), not number(%d, %d).", from, to, to, from);
  }

  return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int number, int size)
{
  int sum = 0;

  if (size <= 0 || number <= 0)
    return (0);

  while (number-- > 0)
    sum += ((circle_random() % size) + 1);

  return (sum);
}


int MIN(int a, int b)
{
  return (a < b ? a : b);
}


int MAX(int a, int b)
{
  return (a > b ? a : b);
}


int MINMAX(int min, int max, int val) {
  return MAX(min, MIN(max, val)); 
}

char *CAP(char *txt)
{
  int i = first_disp_char(txt);
  *(txt+i) = UPPER(*(txt+i));
  return (txt);
}


/* Create a duplicate of a string */
char *str_dup(const char *source)
{
  char *new_z;

  // DM: will crash if source is null, best just return NULL ...
  if (source != NULL) {
    CREATE(new_z, char, strlen(source) + 1);
    return (strcpy(new_z, source));
  } else {
    return NULL;
  }
}


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}


/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    basic_mud_log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}


/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    basic_mud_log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}

// Artus> Is the room a death room. Probably won't catch everything ;o)
bool is_death_room(room_rnum nr)
{
  int environ = world[nr].sector_type & 0xfff0;

  if (ROOM_FLAGGED(nr, ROOM_DEATH)) // Room flagged as DEATH
    return TRUE;

  switch (TEMPERATURE(environ)) // Incinerate, Absolute Zero Rooms.
  {
    case SECT_INCINERATE:
    case SECT_ABSZERO:
      return TRUE;
  }
  
  if (GRAVITY(environ) == SECT_CRUSH) // Crushing Rooms.
    return TRUE;

  return FALSE;
}

/* log a death trap hit */
void log_death_trap(struct char_data *ch, int dtype)
{
  char fmt[256] = "";

  switch (dtype)
  {
    case DT_MISC:
      strcat(fmt, "%s hit \"misc\" death trap #%d (%s)");
      break;
    case DT_DEATH:
      strcat(fmt, "%s hit \"death\" death trap #%d (%s)");
      break;
    case DT_INCINERATE:
      strcat(fmt, "%s hit \"incinerate\" death trap #%d (%s)");
      break;
    case DT_ABSZERO:
      strcat(fmt, "%s hit \"absolute zero\" death trap #%d (%s)");
      break;
    default:
      sprintf(fmt, "%%s hit \"unknown (%d)\" death trap #%%d (%%s)", dtype);
      break;
  }
  sprintf(buf, fmt, GET_NAME(ch),
	  GET_ROOM_VNUM(IN_ROOM(ch)), world[ch->in_room].name);
  mudlog(buf, BRF, LVL_ANGEL, TRUE);
}

/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  // DM .. ok crashing on the localtime call in here on the deb libs
  // have temporarily commented that stuff out until a fix is found.
#ifndef NO_LOCALTIME
  time_t ct;
  char *time_s = NULL;
  
  ct = time(0);
  time_s = (char *)asctime(localtime(&ct));
#else
  struct tm lt;
  char time_s[20];

  if (jk_localtime_now(&lt))
    strcpy(time_s, "jk_localtime ERR ");
  else
    sprintf(time_s, "%04d-%02d-%02d %02d:%02d:%02d ", (1900 + lt.tm_year), lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
#endif 
  
  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";
 
  time_s[strlen(time_s) - 1] = '\0';

#ifndef NO_LOCALTIME
  fprintf(logfile, "%-15.15s :: ", time_s + 4);
#else
  fprintf(logfile, "%-19.19s :: ", time_s);
#endif

  va_start(args, format);
  vfprintf(logfile, format, args);
  va_end(args);

  fprintf(logfile, "\n");
  fflush(logfile);
}

/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    basic_mud_log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}

/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 *
 * Modified DM - check on level for punishments
 * if level < LVL_GOD, then always display
 */
void mudlog(const char *str, int type, int level, int file)
{
  char buf[MAX_STRING_LENGTH], tp;
  struct descriptor_data *i;

   if (str == NULL)
     return;    /* eh, oh well. */
   if (file)
    basic_mud_log("%s", str);
   if (level < 0)
     return;    

  sprintf(buf, "&G[&g %s &G]&n\r\n", str);

  for (i = descriptor_list; i; i = i->next)
  {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (LR_FAIL(i->character, level))
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));
    if (type == DBG)
      tp = (GET_DEBUG(i->character) ? DBG : tp);
    if (tp < type)
      continue;
    send_to_char(buf, i->character);
  }
}



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



void sprinttype(int type, const char *names[], char *result)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  if (*names[nr] != '\n')
    strcpy(result, names[nr]);
  else
    strcpy(result, "UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	/* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return (&now);
}



struct time_info_data *age(struct char_data * ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return (&player_age);
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch || k == MOUNTING(ch))
      return (TRUE);
  }

  return (FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master == NULL) {
    core_dump();
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
  }

  // Display group leave message
  if (AFF_FLAGGED(ch, AFF_GROUP)) {
    act("$n leaves the group.", FALSE, ch, 0, ch->master, TO_VICT);
    act("You leave the group.", FALSE, ch, 0, NULL, TO_CHAR);
    for (k = ch->master->followers; k != NULL; k = k->next) {
      if (k->follower != ch) {
        act("$n leaves the group.", FALSE, ch, 0, k->follower, TO_VICT);
      }
    } 
  }

  if (ch->master->followers->follower == ch) {	/* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {			/* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);
    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
  REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}

/* DM - send clone message, and kill it */
void die_clone(struct char_data *ch, struct char_data *killer) {
	act("$n realises that $e is not the real $n!", FALSE, ch, 0, 0, TO_ROOM);
	act("You realise you are merely a clone of $n...", FALSE, ch, 0,0, TO_CHAR);
	act("$n grasps $s chest in pain and dies of shock!", FALSE, ch, 0,0, TO_ROOM);
	act("You die from the shock of not being who you are!", FALSE, ch,0, 0, TO_CHAR);
	raw_kill(ch, killer);
}

/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data * ch, struct char_data * leader)
{
  struct follow_type *k;

  if (ch->master) {
    core_dump();
    return;
  }

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (CAN_SEE(leader, ch))
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {
    fgets(temp, 256, fl);
    if (feof(fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n');

  temp[strlen(temp) - 1] = '\0';
  strcpy(buf, temp);
  return (lines);
}


int get_filename(char *orig_name, char *filename, int mode)
{
  const char *prefix, *middle, *suffix;
  char name[64], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    basic_mud_log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  default:
    return (0);
  }

  strcpy(name, orig_name);
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  sprintf(filename, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
  return (1);
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_real(const char *who, int line)
{
  basic_mud_log("SYSERR: Assertion failed at %s:%d!", who, line);

#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise... */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
    abort();
#endif
}

int has_stats_for_prac(struct char_data *ch, int skillnum, bool show)
{
  int return_val = 1;
  int class_index = GET_CLASS(ch);
  // Level...
  if (LR_FAIL(ch, spell_info[skillnum].min_level[(int) GET_CLASS(ch)]))
  {
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch))
    {
      sprintf(buf, "&gDBG: Skillnum: %d, Level Failed.&n\r\n", skillnum);
      send_to_char(buf, ch);
    }
#endif
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char("You do not know of that skill!\r\n", ch);
      else
	send_to_char("You do not know of that spell!\r\n", ch);
    }
    return(0);
  }
  // STR...
  if (GET_REAL_STR(ch) < spell_info[skillnum].str[class_index]) 
  {
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char("You don't have the natural strength to use this skill\n\r", ch);
      else
	send_to_char("You don't have the natural strength to use this spell\n\r", ch);
    }
    return_val = 0;
  }
  // INT...
  if (GET_REAL_INT(ch) < spell_info[skillnum].intl[class_index]) 
  {
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char("You don't have the natural intelligence to use this skill\n\r", ch);
      else
	send_to_char("You don't have the natural intelligence to use this spell\n\r", ch);
    }
    return_val = 0;
  }
  // WIS...
  if (GET_REAL_WIS(ch) < spell_info[skillnum].wis[class_index]) 
  {
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char("You don't have the natural wisdom to use this skill\n\r", ch);
      else
	send_to_char("You don't have the natural wisdom to use this spell\n\r", ch);
    }
    return_val = 0;
  }
  // DEX...
  if (GET_REAL_DEX(ch) < spell_info[skillnum].dex[class_index]) 
  {
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char("You don't have the natural dexterity to use this skill\n\r", ch);
      else
	send_to_char("You don't have the natural dexterity to use this spell\n\r", ch);
    }
    return_val = 0;
  }
  // CON...
  if (GET_REAL_CON(ch) < spell_info[skillnum].con[class_index]) 
  {
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char("You don't have the natural constitution to use this skill\n\r", ch);
      else
	send_to_char("You don't have the natural constitution to use this spell\n\r", ch);
    }
    return_val = 0;
  }
  // CHA...
  if (GET_REAL_CHA(ch) < spell_info[skillnum].cha[class_index]) 
  {
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char("You don't have the natural charisma to use this skill\n\r", ch);
      else
	send_to_char("You don't have the natural charisma to use this spell\n\r", ch);
    }
    return_val = 0;
  }
  return(return_val);
}

/* check if player has minimum stats to use this skill */
int has_stats_for_skill(struct char_data *ch, int skillnum, bool show)
{
  // What ever. I do what i want.
  if (!LR_FAIL(ch, LVL_IMPL))
    return(1);
  if (GET_SKILL(ch, skillnum) == 0) 
  {
    if (show)
    {
      if (IS_SKILL(skillnum))
	send_to_char(UNFAMILIARSKILL, ch);
      else
	send_to_char(UNFAMILIARSPELL, ch);
    }
    return(0);
  }
  return(has_stats_for_prac(ch, skillnum, show));
} 

/* Get world from zone_table now - DM */
// probably should use room rnum instead of ch, anyhow ...
int get_world(room_rnum room)
{
  extern struct zone_data *zone_table;
  int zone;
 
  if (room < 0 || room > top_of_world)
    return (-1);

  // DM - the world values are saved as 1,2,3 (silly)
  // and they are used in all code as 0,1,2
  // see: WORLD_XXX
  zone = zone_table[world[room].zone].world;
  
  return (zone-1);
} 

int same_world(struct char_data *ch,struct char_data *ch2)
{
  if (!LR_FAIL(ch, LVL_IMPL) || !LR_FAIL(ch2, LVL_IMPL))
    return 1;
  return (get_world(ch->in_room) == get_world(ch2->in_room));
} 

/* Info channel added by hal */
void info_channel( char *str , struct char_data *ch )
{
  char buf[MAX_STRING_LENGTH];
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
 
  sprintf(buf, "INFO: [ %s ]\r\n", str);
 
  for (i = descriptor_list; i ; i = i->next)
    if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING))
    {
      if (!(PRF_FLAGGED(i->character, PRF_NOINFO) ))
      {
        if ( CAN_SEE(i->character, ch) )
	{
          send_to_char(CCGRN(i->character, C_NRM), i->character);
          send_to_char(buf, i->character);
          send_to_char(CCNRM(i->character, C_NRM), i->character);
        }
      }
    }
} 

/* Called when char stops autoassisting another single char     */
/* This will NOT do if a character quits/dies!!          */
void stop_assisting(struct char_data * ch)
{
  struct char_data *autoassisting;
  struct assisters_type *temp, *temp2;
 
  autoassisting=AUTOASSIST(ch);
 
  /* The character that ch is autoassisting */
  if (autoassisting) {
    act("You stop auto assisting $N.", FALSE, ch, 0, autoassisting, TO_CHAR);
    act("$n stops auto assisting $N.", TRUE, ch, 0, autoassisting, TO_NOTVICT);
    act("$n stops auto assisting you.", TRUE, ch, 0, autoassisting, TO_VICT);
 
    if (autoassisting->autoassisters->assister == ch) {  /* Head of assister-list? */
      temp = autoassisting->autoassisters;
      autoassisting->autoassisters = temp->next;
      temp->assister=NULL;
      free(temp);
    } else {                      /* locate assister who is not head of list */
      for (temp = autoassisting->autoassisters; temp->next->assister != ch; temp = temp->next);
 
      temp2 = temp->next;
      temp->next = temp2->next;
      temp2->assister=NULL;
      free(temp2);
    }
 
    AUTOASSIST(ch) = NULL;
  }
} 

/* Called when all autoassister's stop assisting char   */
/* This will NOT do if a character quits/dies!!          */
void stop_assisters(struct char_data *ch)
{
  struct assisters_type *k, *j;
 
  for (k = ch->autoassisters; k; k = j) {
    j = k->next;
    stop_assisting(k->assister);
  }
} 

/* Called when a character that autoassists/is followed dies */
void die_assisting(struct char_data * ch)
{
  if (ch->autoassisters)
    stop_assisters(ch);
  if (AUTOASSIST(ch))
    stop_assisting(ch);
} 

/* Number of attacks the PC/NPC has available - DM */
int num_attacks(struct char_data *ch)
{
  if (!IS_NPC(ch)) 
  {
    if ((GET_SKILL(ch,SKILL_3RD_ATTACK) > 0) &&
	has_stats_for_skill(ch, SKILL_3RD_ATTACK, FALSE) &&
	(GET_SKILL(ch,SKILL_2ND_ATTACK) > 0))
      return 3;
    else if ((GET_SKILL(ch,SKILL_2ND_ATTACK) > 0) &&
	     has_stats_for_skill(ch, SKILL_3RD_ATTACK, FALSE))
      return 2;
    else
      return 1;
  } else {
    if (MOB_FLAGGED(ch,MOB_3RD_ATTACK))
      return 3;
    else if (MOB_FLAGGED(ch,MOB_2ND_ATTACK))
      return 2;
    else
      return 1;
  }
} 

/* Auction system functions */
#if 0
int add_lot(struct obj_data *obj, struct char_data *ch, long amount)
{
 
   int i = 0;
   int done= 0;
 
   while ( !done && i < MAX_LOTS )
   {
     if ( avail_lots[i].sellerid == 0 )
     {
        avail_lots[i].sellerid = GET_IDNUM(ch);
        strcpy(avail_lots[i].sellername, GET_NAME(ch));
        avail_lots[i].buyerid = 0;
        avail_lots[i].obj = obj;
        avail_lots[i].offer = amount;
  
        done = 1;
     }
     i++;
  }
 
  return i;

} 

void show_lots (struct char_data *ch)
{
        int i = 0;
        if (COLOR_LEV(ch) >= C_NRM)
                        send_to_char(KMAG, ch);
  
        while ( avail_lots[i].sellerid != 0 )
        {
             if ( avail_lots[i].buyerid == 0 )
                  strcpy ( avail_lots[i].buyername, "no one");
 
             sprintf(buf, "Item %d: %s is selling %s to %s for %ld.\r\n",
                             i+1,
                             avail_lots[i].sellername,
                             avail_lots[i].obj->short_description,
                             avail_lots[i].buyername,
                             avail_lots[i].offer );
 
             send_to_char( buf, ch);
             i++;
        }
 
        if ( i == 0 )
             send_to_char( "No items available.\r\n", ch);
 
         if (COLOR_LEV(ch) >= C_NRM)
                        send_to_char(KNRM, ch);
}  

int check_seller(struct char_data *ch)
{
    int i = 0;
    int check = -1;
 
    while ( avail_lots[i].sellerid != 0 )
    {
       if ( avail_lots[i].sellerid == GET_IDNUM(ch))
            check = i;
       i++;
    }
 
    return check;
}
 
void remove_lot(int lot)
{
        int i = lot;
 
        avail_lots[lot].sellerid = 0;
 
        while ( avail_lots[i+1].sellerid != 0 )
        {
          avail_lots[i].sellerid = avail_lots[i+1].sellerid;
          avail_lots[i].buyerid = avail_lots[i+1].buyerid;
	  strcpy(avail_lots[i].buyername, avail_lots[i+1].buyername);
          strcpy(avail_lots[i].sellername, avail_lots[i+1].sellername);

          avail_lots[i].obj = avail_lots[i+1].obj;
          avail_lots[i].offer = avail_lots[i+1].offer;
 
          avail_lots[i+1].sellerid = 0;
          i++;
        }
}   
#endif

// Calculate the unholiness modifier and return the result
double unholiness_modifier(struct char_data *ch)
{
  if (GET_UNHOLINESS(ch) < 1)
    return 0;
  if (GET_UNHOLINESS(ch) >= 10)
    return -1;
  return (double)(0-GET_UNHOLINESS(ch) * 0.1);
}

// Calculate the elitist modifier and return the result.
float elitist_modifier(struct char_data *ch)
{
  float fModifier = 0.0;
  int i;
 
  if (GET_CLASS(ch) <= CLASS_WARRIOR)
    return 0;
  
  if (GET_CLASS(ch) == CLASS_MASTER)
  {
    i = GET_REM_TWO(ch) - 75;
    if (i == 20) 
    {
      if (GET_REM_ONE(ch) == RONE_MAX_LVL)
	fModifier -= 0.5;
      else
	fModifier -= 0.4;
    }
    else if (i > 15) 
      fModifier -= 0.3;
    else if (i > 10) 
      fModifier -= 0.2;
    else if (i > 5)
      fModifier -= 0.1;
  }

  i = GET_REM_ONE(ch) - 50;
  if (i == 41) 
    fModifier -= 0.5;
  else if (i == 40)
    fModifier -= 0.4;
  else if (i >= 35)
    fModifier -= 0.35;
  else if (i >= 30)
    fModifier -= 0.3;
  else if (i >= 25)
    fModifier -= 0.25;
  else if (i >= 20)
    fModifier -= 0.2;
  else if (i >= 15)
    fModifier -= 0.15;
  else if (i >= 10)
    fModifier -= 0.1;

  return fModifier;
}

// return the extra values for the characters special modifiers
// ie. permanent's 
// TALI - TODONE.
float special_modifier(struct char_data *ch)
{
  float fModifier = 0.0;
  long lSpecials = GET_SPECIALS(ch);
  
  if (IS_SET(lSpecials, SPECIAL_INVIS))		// 2%
    fModifier += 0.02;				
  if (IS_SET(lSpecials, SPECIAL_SNEAK))		// 2%
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_MULTIWEAPON))	// 9%
    fModifier += 0.09;
  if (IS_SET(lSpecials, SPECIAL_FOREST_SPELLS))	// 2%
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_FOREST_HELP))    // 2%
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_HEALER))	// 12%
    fModifier += 0.12;
  if (IS_SET(lSpecials, SPECIAL_PRIEST))	// %2
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_BACKSTAB))	// 12%
    fModifier += 0.12;
  if (IS_SET(lSpecials, SPECIAL_BATTLEMAGE))	// 7%
    fModifier += 0.07;
  if (IS_SET(lSpecials, SPECIAL_MANA_THIEF))	// 5%
    fModifier += 0.05;
  if (IS_SET(lSpecials, SPECIAL_HOLY))		// 3%
  {
    //  fModifier += 0.03;
    char mybuf[MAX_STRING_LENGTH]="";
    REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_HOLY);
    sprintf(mybuf, "SYSERR: %s has SPECIAL_HOLY. Removed.", GET_NAME(ch));
    mudlog(mybuf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
  }
  if (IS_SET(lSpecials, SPECIAL_DISGUISE))	// 1%
    fModifier += 0.01;
  if (IS_SET(lSpecials, SPECIAL_ESCAPE))	// 1%
    fModifier += 0.01;
  if (IS_SET(lSpecials, SPECIAL_INFRA))		// 2%
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_DWARF))		// 2%
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_GROUP_SNEAK))   // 3%
    fModifier += 0.03;
  if (IS_SET(lSpecials, SPECIAL_THIEF))		// 5%
    fModifier += 0.05;
  if (IS_SET(lSpecials, SPECIAL_GORE))		// 5%
    fModifier += 0.05;
  if (IS_SET(lSpecials, SPECIAL_MINOTAUR))	// 8%
    fModifier += 0.08;
  if (IS_SET(lSpecials, SPECIAL_CHARMER))	// 3%
    fModifier += 0.03;
  if (IS_SET(lSpecials, SPECIAL_SUPERMAN))	// 4%
    fModifier += 0.04;
  if (IS_SET(lSpecials, SPECIAL_FLY))		// 2%
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_ELF))		// 2%
    fModifier += 0.02;
  if (IS_SET(lSpecials, SPECIAL_TRACKER))	// 3%
    fModifier += 0.03;
  if (IS_SET(lSpecials, SPECIAL_EMPATH))        // 1%
    fModifier += 0.01;
#ifndef IGNORE_DEBUG
  if (GET_DEBUG(ch))
  {
    sprintf(buf, "DBG: specials_modifier = %1.3f\r\n", fModifier);
    send_to_char(buf, ch);
  }
#endif
  return fModifier;
}

// Artus calculate the modifier for ch and return the result.
double calc_modifier(struct char_data *ch)
{
  if (IS_NPC(ch))
    return 1.00;
  GET_MODIFIER(ch) = race_modifiers[GET_RACE(ch)] + 
		     class_modifiers[(int)GET_CLASS(ch)] +
		     special_modifier(ch) + 
		     elitist_modifier(ch) +
		     unholiness_modifier(ch);
  if (GET_MODIFIER(ch) < 0.5)
    GET_MODIFIER(ch) = 0.5;
  else if (GET_MODIFIER(ch) > 2)
    GET_MODIFIER(ch) = 2;
  return GET_MODIFIER(ch);
}

void add_mud_event(struct event_data *ev)
{
  struct event_data *temp = events.list;
  ev->next = NULL;
  
  // Is it the first?
  if(events.list == NULL)
  {
    events.list = ev;
    events.num_events++;
    return;
  }

  // Add it at the end..
  while (temp->next)
    temp = temp->next;
  temp->next = ev;

  events.num_events++;
}

void remove_mud_event(struct event_data *ev)
{
  struct event_data *temp = ev->next;	// Needed for REMOVE_FROM_LIST

  REMOVE_FROM_LIST(ev, events.list, next);
  free(ev);

  events.num_events--;
}

/* Calculate Damage/Heal Amounts. */
int calc_dam_amt(struct char_data *ch, struct char_data *vict, int skillnum)
{
  float dam = 0;

  if ((!ch) || (!vict))
  {
    sprintf(buf, "SYSERR: calc_dam_amt() called without ch or vict! [Skillnum: %d]", skillnum);
    mudlog(buf, NRM, LVL_IMPL, TRUE);
    return 0;
  }

  switch (skillnum)
  {
    case SKILL_BASH:
      dam = GET_LEVEL(ch);
      break;
    case SKILL_BEARHUG:
      dam = GET_LEVEL(ch);
      break;
    case SKILL_BODYSLAM:
      dam = GET_LEVEL(ch);
      dam += number(1, MAX(1, GET_WEIGHT(ch) - 100));
      if (GET_EQ(ch, WEAR_BODY) && !(GET_EQ(vict, WEAR_BODY)))
	dam *= 1.5;
      else if (GET_EQ(vict, WEAR_BODY))
	dam /= 2;
      dam *= MAX(1, STRTODAM(ch));
      break;
    case SKILL_FLYINGTACKLE:
      dam = GET_LEVEL(ch) + (GET_STR(ch) * 2);
      break;
    case SKILL_HEADBUTT:
      dam = GET_LEVEL(ch);
      if (GET_EQ(ch, WEAR_HEAD) && !(GET_EQ(vict, WEAR_HEAD)))
	dam *= 1.5;
      else if (GET_EQ(vict, WEAR_HEAD))
	dam /= 2;
      dam *= MAX(1, STRTODAM(ch));
      break;
    case SKILL_KICK:
      dam = GET_LEVEL(ch);
      break;
    case SKILL_PILEDRIVE:
      dam = GET_HEIGHT(ch) + GET_WEIGHT(ch);
      dam /= 10;
      dam += GET_LEVEL(ch);
      break;
    case SKILL_PRIMAL_SCREAM:
      dam = dice(4, GET_AFF_DEX(ch)) + 6;
      break;
    case SKILL_THROW:
      // The original throw should be fine.
      break;
    case SKILL_TRAP_PIT:
      dam = GET_LEVEL(ch) * 2 + GET_DEX(ch);
      break;
    case SKILL_TRIP:
      dam = GET_LEVEL(ch) * 2 + STRTODAM(ch);
      break;
    default:
      sprintf(buf, "SYSERR: calc_dam_amt called with unexpected skillnum: %d",
	      skillnum);
      mudlog(buf, NRM, LVL_IMPL, TRUE);
      dam = GET_LEVEL(ch);
      break;
  }
  dam = APPLY_SPELL_EFFEC(ch, skillnum, dam);
  dam = APPLY_SKILL(ch, skillnum, dam);
  return (int)(dam);
}

bool aggro_attack(struct char_data *ch, struct char_data *vict, int type)
{
  if (type == AGGRA_PUNISH) // Punish.
  {
    act("$n senses dishonour in you, and attacks with rage!", FALSE, ch, 0,
	vict, TO_VICT);
    act("$n senses dishonour in $N, and attacks with rage!", FALSE, ch, 0,
	vict, TO_NOTVICT);
    hit(ch, vict, TYPE_UNDEFINED);
    return true;
  }
  if (type == AGGRA_HELPER) // Helper.
  {
    // Artus> Mounting.
    if (MOUNTING(ch))
      return false;
    act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
    hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
    return true;
  }
  if (PRF_FLAGGED(vict, PRF_NOHASSLE) ||
      AFF_FLAGGED(vict, AFF_NOHASSLE))
    return false;
  // Artus> Handle Sneak.
  if (AFF_FLAGGED(vict, AFF_SNEAK))
  {
    if (!AFF_FLAGGED(ch, AFF_SENSE_LIFE))
    {
      if (GET_DEX(vict) > 18)
	return false;
      if (number(GET_DEX(vict), 100) > 18)
	return false;
    } else if (GET_DEX(vict) > number(0, 22))
      return false;
    send_to_char("You hear a noise beneath your feet.\r\n", ch);
  }
  if (GET_CHA(ch) > number(0, 22))
  {
    act("$n looks at you with an indifference.", FALSE, ch, 0, vict, TO_VICT);
    return false;
  }
  if (type == AGGRA_AGGR_EVIL)
  {
    act("$n attempts to rid the world of your evil!", FALSE, ch, 0, vict,
	TO_VICT);
    act("$n attempts to rid the world of $N's evil!", FALSE, ch, 0, vict,
	TO_NOTVICT);
  } else if (type == AGGRA_AGGR_GOOD) {
    act("$n cannot bear your goodness a minute longer.", FALSE, ch, 0, vict,
	TO_VICT);
    act("$n cannot bear $N's goodness a minute longer.", FALSE, ch, 0, vict,
	TO_NOTVICT);
  } else {
    act("$n lunges at you.", FALSE, ch, 0, vict, TO_VICT);
    act("$n lunges at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  hit(ch, vict, TYPE_UNDEFINED);
  return true;
}
  

#define WOLF_FROM 10001 // Citizen of Darask.
// Load the werewolf and vampire on a full moon.
void init_nocturnal(void)
{
  struct char_data *wolf, *vamp, *samp;
  struct descriptor_data *d;
  room_rnum construct = NOWHERE;
  int i;
  ACMD(do_gen_comm);
  void perform_change(struct char_data *ch);

  wolf = NULL; 
  vamp = NULL;

  // Where oh where are we going to load the wolf?..
  for (i = 0; i < 10; i++)
    if ((construct = real_room(10000 + number(1, 15))) > NOWHERE)
      break;
  if (construct != NOWHERE) // Load the wolf and put it there.
  {
    wolf = read_mobile(WOLF_VNUM, VIRTUAL);
    if (wolf)
    {
      samp = read_mobile(WOLF_FROM, VIRTUAL);
      if (samp)
      {
	char_to_room(samp, construct);
	act("$n screams in agony as $e breaks out in hair!", FALSE, samp, NULL,
	    NULL, TO_ROOM);
	do_gen_comm(samp, "What the heck is happenaarghhHHHHHHHHOOOOOOOOWWWW--", 0,
	            SCMD_HOLLER);
	extract_char(samp);
	do_gen_comm(wolf, "--WWWWLLLLLLLL!!!!!!!!", 0, SCMD_HOLLER);
	char_to_room(wolf, construct);
      } else {
	char_to_room(wolf, construct);
	do_gen_comm(wolf,"HHHHHHHHOOOOOOOOWWWWWWWWLLLLLLLL!!!!!!!!",0,
	            SCMD_HOLLER);
	act("$n has arrived.", TRUE, wolf, NULL, NULL, TO_ROOM);
      }
    }
  }

  // Where oh where are we going to load the vamp?..
  for (i = 0; i < 10; i++)
    if ((construct = real_room(1600 + number(0, 104))) > NOWHERE)
      break;
  if (construct != NOWHERE) // Load the vamp and put it there.
  {
    vamp = read_mobile(VAMP_VNUM, VIRTUAL);
    if (vamp)
    {
      char_to_room(vamp, construct);
      act("$n has arrived.", TRUE, wolf, NULL, NULL, TO_ROOM);
    }
  }
  // Change infected players.
  for (d = descriptor_list; d; d = d->next)
  {
    if (!(d->character) || (STATE(d) != CON_PLAYING) || 
	!(PRF_FLAGGED(d->character, PRF_WOLF) || 
	  PRF_FLAGGED(d->character, PRF_VAMPIRE)))
      continue;
    perform_change(d->character);
  }
}

// Remove wolves/vampires from the game when the sun rises.
void exit_nocturnal(void)
{
  extern struct index_data *mob_index;
  struct char_data *vict, *next_vict = NULL;
  
  for (vict = character_list; vict; vict = next_vict)
  {
    next_vict = vict->next;
    if (!IS_NPC(vict))
    {
      if (affected_by_spell(vict, SPELL_CHANGED))
      {
	affect_from_char(vict, SPELL_CHANGED);
	send_to_char("You return to your original form.\r\n",vict);
	if(PRF_FLAGGED(vict,PRF_WOLF))
	  sprintf(buf,"$n whimpers in pain as $e returns to $s original form.");
	else if(PRF_FLAGGED(vict,PRF_VAMPIRE))
	  sprintf(buf,"$n's fangs retract as $e returns to normal.");
	else
	  sprintf(buf,"$n pulls a pair of fake fangs out of $s mouth.");
	act(buf,FALSE,vict,0,0,TO_ROOM);
      }
      continue;
    }
    if ((GET_MOB_VNUM(vict) != WOLF_VNUM) &&
	(GET_MOB_VNUM(vict) != VAMP_VNUM))
      continue;
    if (FIGHTING(vict))
    {
      stop_fighting(FIGHTING(vict));
      stop_fighting(vict);
    }
    act("$n spontaneously combusts before your very eyes!", TRUE, vict, NULL,
	NULL, TO_ROOM);
    extract_char(vict);
  }
}

// Read Auction List from File.
void init_auctions(void)
{
  extern struct auc_data *auc_list;
  struct auc_data *lot, *prev=NULL;
  struct auc_file_elem tmp_store;
  struct obj_data *obj;
  struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
  room_rnum auc_room;
  FILE *fl;
  int dummy=0;

  auc_room = real_room(AUC_ROOM);

  // Hopefully this is a waste of time :o)
  if (auc_list != NULL)
  {
    basic_mud_log("WARNING: auc_list was not null! [idnum: %d, sellerid: %ld]",
	          auc_list->idnum, auc_list->sellerid);
    auc_list = NULL;
  }

  if (!(fl = fopen(AUC_FILE, "rb")))
    return;

  fread(&tmp_store, sizeof(struct auc_file_elem), 1, fl);
  while (!feof(fl))
  {
    if (!(obj = Obj_from_store(tmp_store.obj, &dummy)))
    {
      basic_mud_log("SYSERR: Failed to read object #%d", 
	            tmp_store.obj.item_number);
      continue;
    }
    obj_to_room(obj, auc_room);
    CREATE(lot, struct auc_data, 1);
    memset(lot->buyername, '\0', MAX_NAME_LENGTH+1);
    memset(lot->sellername, '\0', MAX_NAME_LENGTH+1);
    lot->idnum = tmp_store.idnum;
    lot->sellerid = tmp_store.sellerid;
    strncpy(lot->sellername, tmp_store.sellername, MAX_NAME_LENGTH);
    lot->buyerid = tmp_store.buyerid;
    strncpy(lot->buyername, tmp_store.buyername, MAX_NAME_LENGTH);
    lot->offer = tmp_store.offer;
    lot->obj = obj;
    lot->next = NULL;
    if (!(prev))
      auc_list = lot;
    else
      prev->next = lot;
    prev = lot;
/*    basic_mud_log("Auction %d: Obj: %d (%s); Seller %d (%s); Offer %d", 
	          lot->idnum, tmp_store.obj.item_number, 
		  lot->obj->short_description, lot->sellerid, lot->sellername,
		  lot->offer); */
    fread(&tmp_store, sizeof(struct auc_file_elem), 1, fl);
  }
  fclose(fl);
}

// Artus> Make sure the character is in the list.
struct char_data *is_valid_char(struct char_data *ch)
{
  for (struct char_data *i = character_list; i; i = i->next)
    if (i == ch)
      return i;
  return NULL;
}

// Artus> Add a player to the immlist. Can also be used for updating.
void add_to_immlist(char *name, long idnum, long immkills, ubyte unholiness)
{
  extern struct imm_list_element *immlist_table;
  struct imm_list_element *k, *noob;
  // Beginning of list.
  if (!immlist_table)
  {
    CREATE(immlist_table, struct imm_list_element, 1);
    immlist_table->name = str_dup(name);
    immlist_table->id = idnum;
    immlist_table->kills = immkills;
    immlist_table->unholiness = unholiness;
    immlist_table->next = NULL;
    return;
  }
  // Updating.
  for (k = immlist_table; k->next; k = k->next)
    if (k->id == idnum)
    {
      k->kills = immkills;
      k->unholiness = unholiness;
      return;
    }
  if (k->id == idnum)
  {
    k->kills = immkills;
    k->unholiness = unholiness;
    return;
  }
  // Append list.
  CREATE(noob, struct imm_list_element, 1);
  noob->name = str_dup(name);
  noob->id = idnum;
  noob->kills = immkills;
  noob->unholiness = unholiness;
  k->next = noob;
  noob->next = NULL;
}

// Artus> Update teh questlist.
void update_questlist(struct char_data *vict, int qitemno)
{
  struct questlist_element *k;
  if (GET_QUEST_ITEM(vict, qitemno) == 0)
    return;
  for (k=questlist_table; k; k=k->next)
    if (k->objdata.vnum == GET_QUEST_ITEM(vict, qitemno))
      break;
  if (!k)
    return;
  memcpy(&k->objdata, &GET_QUEST_ITEM_DATA(vict, qitemno),
         sizeof(struct quest_obj_data));
}

// Artus> Add data to quest list.
void add_to_questlist(char *name, long idnum, struct quest_obj_data *item)
{
  struct questlist_element *k, *noob;

  CREATE(noob, struct questlist_element, 1);
  noob->name = str_dup(name);
  noob->id = idnum;
  memcpy(&noob->objdata, item, sizeof(struct quest_obj_data));
  noob->objrnum = real_object(noob->objdata.vnum);
  noob->next = NULL;
  if (!(questlist_table))
  {
    questlist_table = noob;
    return;
  }
  for (k = questlist_table; k; k = k->next)
    if (!(k->next))
    {
      k->next = noob;
      return;
    }
}

// Write Auction List to File.
void write_auction_file(void)
{
  extern struct auc_data *auc_list;
  struct auc_data *lot;
  struct auc_file_elem tmp_store;
  void Obj_to_file_elem(struct obj_data *obj, struct obj_file_elem *dest, 
			int location);
  FILE *fl;

  if (!(fl = fopen(AUC_FILE, "wb")))
    return;

  for (lot = auc_list; lot; lot = lot->next)
  {
    memset(&tmp_store.sellername, '\0', MAX_NAME_LENGTH+1);
    memset(&tmp_store.buyername, '\0', MAX_NAME_LENGTH+1);
    tmp_store.idnum = lot->idnum;
    tmp_store.sellerid = lot->sellerid;
    strncpy(tmp_store.sellername, lot->sellername, MAX_NAME_LENGTH);
    tmp_store.buyerid = lot->buyerid;
    tmp_store.offer = lot->offer;
    strncpy(tmp_store.buyername, lot->buyername, MAX_NAME_LENGTH);
    Obj_to_file_elem(lot->obj, &tmp_store.obj, 0);
    fwrite(&tmp_store, sizeof(struct auc_file_elem), 1, fl);
  }
  fflush(fl);
  fclose(fl);
}
