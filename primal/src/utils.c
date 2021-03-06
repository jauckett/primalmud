/* ************************************************************************
*   File: utility.c                                     Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <arpa/telnet.h>
#include <netinet/in.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"

extern struct time_data time_info;
extern struct room_data *world;

extern struct char_data *get_char(char *name);
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];
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

/* Darius number of items in room for houses...:) */
int num_items_room(struct obj_data *tobj)
{
  int counter = 0;

	if (!tobj)
		return(0);
  if(tobj->next_content)
    counter = num_items_room(tobj->next_content);
  if(tobj->contains)
    counter += num_items_room(tobj->contains);
  return ++counter;
}

int MIN(int a, int b)
{
  return a < b ? a : b;
}


int MAX(int a, int b)
{
  return a > b ? a : b;
}


/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  if (to < from) {
    mudlog("SYSERR: number() to < from", BRF, LVL_GRGOD, TRUE);
    return 0;
  }
  return ((random() % (to - from + 1)) + from);
}



/* simulates dice roll */
int dice(int number, int size)
{
  int r;
  int sum = 0;
  
  /* JA this assert has been going off a bit */
  if (size <= 0)
    size = 1;

  assert(size >= 1);

  for (r = 1; r <= number; r++)
    sum += ((random() % size) + 1);
  return (sum);
}



/* Create a duplicate of a string */
char *str_dup(const char *source)
{
  char *new;

  CREATE(new, char, strlen(source) + 1);
  return (strcpy(new, source));
}



/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(char *arg1, char *arg2)
{
  int chk, i;

  for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
      if (chk < 0)
	return (-1);
      else
	return (1);
  return (0);
}



/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(char *arg1, char *arg2, int n)
{
  int chk, i;

  for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
      if (chk < 0)
	return (-1);
      else
	return (1);

  return (0);
}

/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  char buf[150];
  extern struct room_data *world;

  sprintf(buf, "%s hit death trap #%d (%s)", GET_NAME(ch),
	  world[ch->in_room].number, world[ch->in_room].name);
  mudlog(buf, BRF, LVL_ETRNL1, TRUE);
}


/* writes a string to the log */
void pmlog(char *str)
{
  long ct;
  char *tmstr;

  ct = time(0);
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  fprintf(stderr, "%-19.19s :: %s\n", tmstr, str);
}


/* the "touch" command, essentially. */
int touch(char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a")))
    return 0;
  else {
    fclose(fl);
    return 1;
  }
}


/* New PROC: syslog by Fen Jul 3, 1992 */
void mudlog(char *str, char type, ubyte level, byte file)
{
  char buf[1024];
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
  char *tmp;
  long ct;
  char tp;

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (file)
    fprintf(stderr, "%-19.19s :: %s\n", tmp, str);
  
/* JA changed level to unsigned char  
	if (level < 0)
    return;
*/

  sprintf(buf, "[ %s ]\r\n", str);

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING)) {
      tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
	    (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));

      if ((GET_LEVEL(i->character) >= level) && (tp >= type)) {
	send_to_char(CCGRN(i->character, C_NRM), i->character);
	send_to_char(buf, i->character);
	send_to_char(CCNRM(i->character, C_NRM), i->character);
      }
    }
}

/* End of Modification */

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


void sprintbit(long vektor, char *names[], char *result)
{
  long nr;

  *result = '\0';

  if (vektor < 0) {
    char tmpbuf[40];
    sprintf(tmpbuf, "SPRINTBIT ERROR! [%ld]", vektor);
    strcpy(result, tmpbuf);
    return;
  }
  for (nr = 0; vektor; vektor >>= 1) {
    if (IS_SET(1, vektor)) {
      if ((*names[nr] != '\n') && (strcmp(names[nr], "RESERVED"))) {
	strcat(result, names[nr]);
	strcat(result, " ");
      } else
        if (strcmp(names[nr], "RESERVED"))
	  strcat(result, "UNDEFINED ");
    }
    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    strcat(result, "NOBITS ");
}



void sprinttype(int type, char *names[], char *result)
{
  int nr;

  for (nr = 0; (*names[nr] != '\n'); nr++);
  if (type < nr)
    strcpy(result, names[type]);
  else
    strcpy(result, "UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data real_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = -1;
  now.year = -1;

  return now;
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	/* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return now;
}



struct time_info_data age(struct char_data * ch)
{
  struct time_info_data player_age;

  player_age = mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return player_age;
}




/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) TELOPT_NAOFFD,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  SEND_TO_Q(on_string, d);
}



/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch && k == MOUNTING(ch) )
      return TRUE;
  }

  return FALSE;
}

/* Called when a character that autoassists/is followed dies */
void die_assisting(struct char_data * ch)
{
  if (ch->autoassisters)
    stop_assisters(ch); 
  if (AUTOASSIST(ch))
    stop_assisting(ch);
}

/* Called when char stops autoassisting another single char	*/
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

/* Called when all autoassister's stop assisting char	*/
/* This will NOT do if a character quits/dies!!          */
void stop_assisters(struct char_data *ch)
{
  struct assisters_type *k, *j;

  for (k = ch->autoassisters; k; k = j) {
    j = k->next;
    stop_assisting(k->assister);
  }

}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  assert(ch->master);

  if (IS_AFFECTED(ch, AFF_CHARM)) {
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

/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master) {
    stop_follower(ch);
  }

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

  assert(!ch->master);

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

/* DM - send clone die message and kill em */
void die_clone(struct char_data *ch, struct char_data *killer)
{
  act("$n realises that $e is not the real $n!",FALSE,ch,0,0,TO_ROOM);
  act("You realise you are merely a clone of $n...",FALSE,ch,0,0,TO_CHAR);
  act("$n grasps $s chest in pain and dies of shock!",FALSE,ch,0,0,TO_ROOM);
  act("You die from the shock of not being who you are!",FALSE,ch,0,0,TO_CHAR);
  raw_kill(ch,killer);
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
/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
int replace_str(char **string, char *pattern, char *replacement, int rep_all,
              int max_size) {
   char *replace_buffer = NULL;
   char *flow, *jetsam, temp;
   int len, i;

   if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
     return -1;

   CREATE(replace_buffer, char, max_size);
   i = 0;
   jetsam = *string;
   flow = *string;
   *replace_buffer = '\0';
   if (rep_all) {
      while ((flow = (char *)strstr(flow, pattern)) != NULL) {
       i++;
       temp = *flow;
       *flow = '\0';
       if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size) {
          i = -1;
          break;
       }
       strcat(replace_buffer, jetsam);
       strcat(replace_buffer, replacement);
       *flow = temp;
       flow += strlen(pattern);
       jetsam = flow;
      }
      strcat(replace_buffer, jetsam);
   }
   else {
      if ((flow = (char *)strstr(*string, pattern)) != NULL) {
       i++;
       flow += strlen(pattern);
       len = ((char *)flow - (char *)*string) - strlen(pattern);

       strncpy(replace_buffer, *string, len);
       strcat(replace_buffer, replacement);
       strcat(replace_buffer, flow);
      }
   }
   if (i == 0) return 0;
   if (i > 0) {
      RECREATE(*string, char, strlen(replace_buffer) + 3);
      strcpy(*string, replace_buffer);
   }
   free(replace_buffer);
   return i;
}


/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen) {
   int total_chars, cap_next = TRUE, cap_next_next = FALSE;
   char *flow, *start = NULL, temp;
   /* warning: do not edit messages with max_str's of over this value */
   char formated[MAX_STRING_LENGTH];

   flow   = *ptr_string;
   if (!flow) return;

   if (IS_SET(mode, FORMAT_INDENT)) {
      strcpy(formated, "   ");
      total_chars = 3;
   }
   else {
      *formated = '\0';
      total_chars = 0;
   }

   while (*flow != '\0') {
      while ((*flow == '\n') ||
           (*flow == '\r') ||
           (*flow == '\f') ||
           (*flow == '\t') ||
           (*flow == '\v') ||
           (*flow == ' ')) flow++;

      if (*flow != '\0') {

       start = flow++;
       while ((*flow != '\0') &&
              (*flow != '\n') &&
              (*flow != '\r') &&
              (*flow != '\f') &&
              (*flow != '\t') &&
              (*flow != '\v') &&
              (*flow != ' ') &&
              (*flow != '.') &&
              (*flow != '?') &&
              (*flow != '!')) flow++;

       if (cap_next_next) {
          cap_next_next = FALSE;
          cap_next = TRUE;
       }

       /* this is so that if we stopped on a sentance .. we move off the sentance delim. */
       while ((*flow == '.') || (*flow == '!') || (*flow == '?')) {
          cap_next_next = TRUE;
          flow++;
       }
         
       temp = *flow;
       *flow = '\0';

       if ((total_chars + strlen(start) + 1) > 79) {
          strcat(formated, "\r\n");
          total_chars = 0;
       }

       if (!cap_next) {
          if (total_chars > 0) {
             strcat(formated, " ");
             total_chars++;
          }
       }
       else {
          cap_next = FALSE;
          *start = UPPER(*start);
       }

       total_chars += strlen(start);
       strcat(formated, start);
                              
       *flow = temp;
      }

      if (cap_next_next) {
       if ((total_chars + 3) > 79) {
          strcat(formated, "\r\n");
          total_chars = 0;
       }
       else {
          strcat(formated, "  ");
          total_chars += 2;
       }
      }
   }
   strcat(formated, "\r\n");

   if (strlen(formated) > maxlen) formated[maxlen] = '\0';
   RECREATE(*ptr_string, char, MIN(maxlen, strlen(formated)+3));
   strcpy(*ptr_string, formated);
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
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
}

/* Get world from zone_table now - DM */
int get_world(struct char_data *ch)
{
  extern struct zone_data *zone_table;
  int zone;

	if (!ch)
		return(1);
  zone = zone_table[world[IN_ROOM(ch)].zone].world;
  
  return zone;
}

int same_world(struct char_data *ch,struct char_data *ch2)
{
  if (GET_LEVEL(ch)==LVL_IMPL || GET_LEVEL(ch2)==LVL_IMPL)
    return 1;
  return (get_world(ch) == get_world(ch2));
}

char *get_world_letter(struct char_data *ch)
{
/* made it always return medieval so it only uses 1 file.. so worlds are linked - Vader */
  return "m";

/*  int locat;

  locat = get_world(ch);
  if(locat == 1) return "m";
  if(locat == 2) return "w";
  return "f"; */
}

int get_filename(char *orig_name, char *filename, int mode)
{
  char *prefix, *middle, *suffix, *ptr, name[64];
  char *world_cur;

  switch (mode) {
  case CRASH_FILE:
    prefix = "plrobjs";
    suffix = "objs";
    break;
  case ETEXT_FILE:
    prefix = "plrtext";
    suffix = "text";
    break;
  default:
    return 0;
    break;
  }

  if (!*orig_name)
    return 0;

  strcpy(name, orig_name);
  world_cur = get_world_letter(get_char(name));
  strcat(name,world_cur);
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

  sprintf(filename, "%s/%s/%s.%s", prefix, middle, name, suffix);
  return 1;
}

int has_stats_for_skill(struct char_data *ch, int skillnum)
{
	int return_val = 1;

/* check if player has minimum stats to use this skill */

	if (GET_REAL_STR(ch) < spell_info[skillnum].str)
	{
		send_to_char("You don't have the natural strength to use this skill\n\r", ch);
		return_val = 0;
	}
	if (GET_REAL_INT(ch) < spell_info[skillnum].intl)
	{
		send_to_char("You don't have the natural intelligence to use this skill\n\r", ch);
		return_val = 0;
	}
	if (GET_REAL_WIS(ch) < spell_info[skillnum].wis)
	{
		send_to_char("You don't have the natural wisdom to use this skill\n\r", ch);
		return_val = 0;
	}
	if (GET_REAL_DEX(ch) < spell_info[skillnum].dex)
	{
		send_to_char("You don't have the natural dexterity to use this skill\n\r", ch);
		return_val = 0;
	}
	if (GET_REAL_CON(ch) < spell_info[skillnum].con)
	{
		send_to_char("You don't have the natural constitution to use this skill\n\r", ch);
		return_val = 0;
	}
	if (GET_REAL_CHA(ch) < spell_info[skillnum].cha)
	{
		send_to_char("You don't have the natural charisma to use this skill\n\r", ch);
		return_val = 0;
	}

return(return_val);
} 

/* Return's the no of digits of the long numb, (anything over 1 bill will be returned as neg+10) */
int digits(const long numb)
{
  long abs_val=abs(numb), neg=0;
  if (numb < 0)
    neg=1;
  if (abs_val < 10)
    return neg+1;
  else if (abs_val < 100)
    return neg+2;
  else if (abs_val < 1000)
    return neg+3;
  else if (abs_val < 10000)
    return neg+4;
  else if (abs_val < 100000)
    return neg+5;
  else if (abs_val < 1000000)
    return neg+6;
  else if (abs_val < 10000000)
    return neg+7;
  else if (abs_val < 100000000)
    return neg+8;
  else if (abs_val < 1000000000)
    return neg+9;
  else 
    return neg+10;
}
