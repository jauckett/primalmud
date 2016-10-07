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
#include "color.h"

extern struct descriptor_data *descriptor_list;
extern struct time_data time_info;
extern struct room_data *world;
extern struct auction_lot avail_lots[MAX_LOTS];

struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void die_follower(struct char_data * ch);
void add_follower(struct char_data * ch, struct char_data * leader);
void prune_crlf(char *txt);
void die_clone(struct char_data *ch, struct char_data *killer);

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
    log("SYSERR: number() should be called with lowest, then highest. number(%d, %d), not number(%d, %d).", from, to, to, from);
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


char *CAP(char *txt)
{
  *txt = UPPER(*txt);
  return (txt);
}


/* Create a duplicate of a string */
char *str_dup(const char *source)
{
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source));
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
    log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
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
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}

/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  char buf[256];

  sprintf(buf, "%s hit death trap #%d (%s)", GET_NAME(ch),
	  GET_ROOM_VNUM(IN_ROOM(ch)), world[ch->in_room].name);
  mudlog(buf, BRF, LVL_IMMORT, TRUE);
}

/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_log(const char *format, ...)
{
  va_list args;
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL)
    puts("SYSERR: Using log() before stream was initialized!");
  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);

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
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(const char *str, int type, int level, int file)
{
  char buf[MAX_STRING_LENGTH], tp;
  struct descriptor_data *i;

  if (str == NULL)
    return;	/* eh, oh well. */
  if (file)
    log(str);
  if (level < 0)
    return;

  sprintf(buf, "[ %s ]\r\n", str);

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_LEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));
    if (tp < type)
      continue;

    send_to_char(CCGRN(i->character, C_NRM), i->character);
    send_to_char(buf, i->character);
    send_to_char(CCNRM(i->character, C_NRM), i->character);
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
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
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

  sprintf(filename, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
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
  log("SYSERR: Assertion failed at %s:%d!", who, line);

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

/* check if player has minimum stats to use this skill */
int has_stats_for_skill(struct char_data *ch, int skillnum)
{
  int return_val = 1;
  int class_index = GET_CLASS(ch);
 
  if (GET_REAL_STR(ch) < spell_info[skillnum].str[class_index]) {
    send_to_char("You don't have the natural strength to use this skill\n\r", ch);
    return_val = 0;
  }

  if (GET_REAL_INT(ch) < spell_info[skillnum].intl[class_index]) {
    send_to_char("You don't have the natural intelligence to use this skill\n\r", ch);
    return_val = 0;
  }

  if (GET_REAL_WIS(ch) < spell_info[skillnum].wis[class_index]) {
    send_to_char("You don't have the natural wisdom to use this skill\n\r", ch);
    return_val = 0;
  }

  if (GET_REAL_DEX(ch) < spell_info[skillnum].dex[class_index]) {
    send_to_char("You don't have the natural dexterity to use this skill\n\r", ch);
    return_val = 0;
  }

  if (GET_REAL_CON(ch) < spell_info[skillnum].con[class_index]) {
    send_to_char("You don't have the natural constitution to use this skill\n\r", ch);
    return_val = 0;
  }

  if (GET_REAL_CHA(ch) < spell_info[skillnum].cha[class_index]) {
    send_to_char("You don't have the natural charisma to use this skill\n\r", ch);
    return_val = 0;
  }
 
  return(return_val);
} 

/* Get world from zone_table now - DM */
int get_world(struct char_data *ch)
{
  extern struct zone_data *zone_table;
  int zone;
 
        if (!ch)
                return(1);
  // DM - the world values are saved as 1,2,3 (silly)
  // and they are used in all code as 0,1,2
  zone = zone_table[world[IN_ROOM(ch)].zone].world;
  
  log("zone: %d",zone);
 
  return zone-1;
} 

int same_world(struct char_data *ch,struct char_data *ch2)
{
  if (GET_LEVEL(ch)==LVL_IMPL || GET_LEVEL(ch2)==LVL_IMPL)
    return 1;
  return (get_world(ch) == get_world(ch2));
} 

/* Info channel added by hal */
void info_channel( char *str , struct char_data *ch )
{
  char buf[MAX_STRING_LENGTH];
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
 
  sprintf(buf, "INFO: [ %s ]\r\n", str);
 
  for (i = descriptor_list; i ; i = i->next)
    if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING)) {
      if (!(PRF_FLAGGED(i->character, PRF_NOINFO) )) {
        if ( CAN_SEE(i->character, ch) ) {
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
  if (!IS_NPC(ch)) {
    if (GET_SKILL(ch,SKILL_3RD_ATTACK) > 0)
      return 3;
    else if (GET_SKILL(ch,SKILL_2ND_ATTACK) > 0)
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

int add_lot(struct obj_data *obj, struct char_data *ch, long amount)
{
 
 int i = 0;
 int done= 0;
 
 while ( !done && i < MAX_LOTS )
 {
   if ( !avail_lots[i].seller )
   {
 
           avail_lots[i].seller = ch;
           avail_lots[i].buyer = NULL;
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
        char buyer[30];
        if (COLOR_LEV(ch) >= C_NRM)
                        send_to_char(KMAG, ch);
 
 
 
        while ( !(avail_lots[i].seller == NULL) )
                {
                        if ( avail_lots[i].buyer == NULL )
                                strcpy ( buyer, "no one");
                        else
                                strcpy ( buyer, avail_lots[i].buyer->player.name);
 
 
                        sprintf(buf, "Item %d: %s is selling %s to %s for %ld.\r\n",
                                i+1,
                                avail_lots[i].seller->player.name,
                                avail_lots[i].obj->short_description,
                                buyer,
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
 
        while ( !(avail_lots[i].seller == NULL) )
                {
                    if ( avail_lots[i].seller == ch)
                         check = i;
 
                        i++;
                }
 
        return check;
}
 
void remove_lot(int lot)
{
        int i = lot;
 
        avail_lots[lot].seller = NULL;
 
        while ( !(avail_lots[i+1].seller == NULL) )
                {
                    avail_lots[i].seller = avail_lots[i+1].seller;
                avail_lots[i].buyer = avail_lots[i+1].buyer;
                avail_lots[i].obj = avail_lots[i+1].obj;
                avail_lots[i].offer = avail_lots[i+1].offer;
 
                        avail_lots[i+1].seller = NULL;
 
                        i++;
                }
 
 
}   

// DM - TODO - work this out
// return the extra values for the characters special modifiers
// ie. permanent's 
float special_modifier(struct char_data *ch)
{

  // accumulate the additional modifier cost for each of the specials in
  // SPEC_FLAGGED (ch->player_specials.primalsaved.misc_specials; 
  
  return 0.0;
}

void set_default_colour(struct char_data *ch, int i)
{
  if (i < 0 || i > 9) {
    send_to_char("Some nuff-nuff coder didn't check your colour number.\r\n",ch);
    return;
  } else {
    GET_COLOUR(ch,i) = default_colour_codes[i]; 
  }
}

void set_colour(struct char_data *ch, int i, int colour_code)
{
  if ((i < 0 || i > 9) || (colour_code < 0 || colour_code > 30)) {
    send_to_char("Some nuff-nuff coder didn't check your colour code.\r\n",ch);
    return;
  } else {
    GET_COLOUR(ch,i) = colour_code;
  }
}

