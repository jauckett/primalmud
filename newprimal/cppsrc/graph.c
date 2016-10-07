/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
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

#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define FD_ZERO(x)
#define FD_SET(x, y) 0
#define FD_ISSET(x, y) 0
#define FD_CLR(x, y)
#endif

/* Externals */
ACMD(do_say);
extern struct char_data *character_list;
extern struct hunt_data *hunt_list;
extern const char *dirs[];
extern struct room_data *world;
extern int track_through_doors;
extern int process_output(struct descriptor_data *t);

/* local functions */
int VALID_EDGE(struct char_data *ch, room_rnum x, int y);
void bfs_enqueue(room_rnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);
int find_first_step(struct char_data *ch, room_rnum src, room_rnum target);
ACMD(do_track);
void hunt_victim(struct char_data * ch);
int getMeHere(sh_int chroom, sh_int victroom);
int MY_VALID_EDGE(sh_int room, int dir );
void do_playerhunt( struct char_data *ch, struct char_data *victim );

struct bfs_queue_struct {
  room_rnum room;
  char dir;
  struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;

/* Utility macros */
#define MARK(room)	(SET_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room)	(REMOVE_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room)	(ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y)	(world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y)	(EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))

int VALID_EDGE(struct char_data *ch, room_rnum x, int y)
{
  if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
    return 0;
  if (track_through_doors == FALSE && IS_CLOSED(x, y))
    return 0;
  if IS_MARKED(TOROOM(x, y))
  {
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch))
      send_to_char(".", ch);
#endif
    return 0;
  }
  if ((LR_FAIL(ch, LVL_CHAMP)) && 
      ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) &&
      !IS_SET(GET_SPECIALS(ch), SPECIAL_TRACKER))
  {
#ifndef IGNORE_DEBUG
    if (GET_DEBUG(ch))
      send_to_char("!", ch);
#endif
    return 0;
  }
  return 1;
}

void bfs_enqueue(room_rnum room, int dir)
{
  struct bfs_queue_struct *curr;

  CREATE(curr, struct bfs_queue_struct, 1);
  curr->room = room;
  curr->dir = dir;
  curr->next = 0;

  if (queue_tail) {
    queue_tail->next = curr;
    queue_tail = curr;
  } else
    queue_head = queue_tail = curr;
}


void bfs_dequeue(void)
{
  struct bfs_queue_struct *curr;

  curr = queue_head;

  if (!(queue_head = queue_head->next))
    queue_tail = 0;
  free(curr);
}


void bfs_clear_queue(void)
{
  while (queue_head)
    bfs_dequeue();
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(struct char_data *ch, room_rnum src, room_rnum target)
{
  int curr_dir;
  room_rnum curr_room;

  if (src < 0 || src > top_of_world || target < 0 || target > top_of_world) 
  {
    basic_mud_log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
    return (BFS_ERROR);
  }
  if (src == target)
    return (BFS_ALREADY_THERE);

  /* clear marks first, some OLC systems will save the mark. */
  for (curr_room = 0; curr_room <= top_of_world; curr_room++)
    UNMARK(curr_room);

  MARK(src);

  /* first, enqueue the first steps, saving which direction we're going. */
  for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
    if (VALID_EDGE(ch, src, curr_dir)) 
    {
      MARK(TOROOM(src, curr_dir));
      bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
    }

  /* now, do the classic BFS. */
  while (queue_head) {
    if (queue_head->room == target) 
    {
      curr_dir = queue_head->dir;
      bfs_clear_queue();
      return (curr_dir);
    } else {
      for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
	if (VALID_EDGE(ch, queue_head->room, curr_dir)) 
	{
	  MARK(TOROOM(queue_head->room, curr_dir));
	  bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
	}
      bfs_dequeue();
    }
  }

  return (BFS_NO_PATH);
}

// Artus> Set a character as hunting, add to the hunt list.
void begin_hunting(struct char_data *ch, struct char_data *vict)
{
  struct hunt_data *huntrec;
  if ((is_valid_char(ch) != ch) || (is_valid_char(vict) != vict))
    return;
  CREATE(huntrec, struct hunt_data, 1);
  huntrec->hunter = ch;
  huntrec->victim = vict;
  if (hunt_list)
    huntrec->next = hunt_list;
  else
    huntrec->next = NULL;
  hunt_list = huntrec;
  HUNTING(ch) = vict;
}
// Artus> Stop a character hunting, remove from the hunt list.
void stop_hunting(struct char_data *ch)
{
  struct hunt_data *hcur, *hnext;
  for (hcur = hunt_list; hcur; hcur = hnext)
  {
    hnext = hcur->next;
    if (hcur->hunter == ch) 
    {
      struct hunt_data *temp;
      REMOVE_FROM_LIST(hcur, hunt_list, next);
      free(hcur);
    }
  }
  HUNTING(ch) = NULL;
}
// Artus> Is this hunt still valid.. Sanity.
struct hunt_data *valid_hunt(struct char_data *ch)
{
  if ((HUNTING(ch) == NULL || hunt_list == NULL))
    return NULL;
  for (struct hunt_data *hrec = hunt_list; hrec; hrec = hrec->next)
    if ((hrec->hunter == ch) && (hrec->victim == HUNTING(ch)))
      return hrec;
  return NULL;
}
// Artus> Stop a character being hunted.
void stop_hunters(struct char_data *ch)
{
  struct hunt_data *hcur, *hnext;
  if ((ch == NULL) || (hunt_list == NULL))
    return;
  for (hcur = hunt_list; hcur; hcur = hnext)
  {
    hnext = hcur->next;
    if (hcur->victim == ch) 
    {
      struct hunt_data *temp;
      if (hcur->hunter != NULL)
      {
	if (hcur->hunter->desc)
	  send_to_char("&[&rYour victim no longer seems to exist.&]\r\n", 
	               hcur->hunter);
	if (IS_NPC(hcur->hunter))
	  do_say(hcur->hunter, "Damn!  My prey is gone!!", 0, 0);
	if (ch == HUNTING(hcur->hunter))
	  stop_hunting(hcur->hunter);
      }
      REMOVE_FROM_LIST(hcur, hunt_list, next);
      free(hcur);
    }
  }
}

/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/

ACMD(do_track)
{
  struct char_data *vict;
  int dir, num;

  if ((!GET_SKILL(ch, SKILL_TRACK) && 
       !IS_SET(GET_SPECIALS(ch), SPECIAL_TRACKER)) || 
      (!GET_SKILL(ch, SKILL_HUNT) && (subcmd == /*SCMD_HUNT*/SKILL_HUNT))) 
  {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
 
  if(subcmd == SCMD_AUTOHUNT) 
  {
    if (!(valid_hunt(ch)) || !CAN_SEE(ch, HUNTING(ch)) ||
	(IN_ROOM(ch) == NOWHERE) ||
	(IN_ROOM(HUNTING(ch)) == NOWHERE))
    {
      stop_hunting(ch);
      send_to_char("You seem to have lost the trail!\r\n", ch);
/*
    if((!CAN_SEE(ch,vict)) || (HUNTING(ch)->in_room == NOWHERE)) 
    {
      send_to_char("You seem to have lost the trail!\r\n",ch);
      HUNTING(ch) = NULL;
*/
      return;
    }
    vict = HUNTING(ch);
  } else {
    one_argument(argument, arg);
    if (!*arg) 
    {
      if(subcmd == /*SCMD_HUNT*/SKILL_HUNT) 
      {
	if(HUNTING(ch)) 
	{
	  act("You stop hunting $N.",FALSE,ch,0,HUNTING(ch),TO_CHAR);
	  stop_hunting(ch);
	  return;
        } else {
          send_to_char("Hunt who?\r\n",ch);
          return;
	}
      } else {
	send_to_char("Whom are you trying to track?\r\n", ch);
	return;
      }
    }
    if (!(vict = generic_find_char(ch, arg, FIND_CHAR_WORLD))) 
    {
      send_to_char("No-one around by that name.\r\n", ch);
      return;
    }
    if (IS_AFFECTED(vict, AFF_NOTRACK))
    {
      send_to_char("You sense no trail.\r\n", ch);
      return;
    }
    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_TAG)) 
    {
      send_to_char("CHEAT! Ya can't track people who are playing tag!\r\n",ch);
      return;
    }
    // Artus> Otherworlds.
    if (!same_world(ch, vict))
    {
      send_to_char("They aren't even in this world!\r\n", ch);
      return;
    }
    if(subcmd == /*SCMD_HUNT*/SKILL_HUNT)
      begin_hunting(ch, vict);
  }
  /* this shood stop errors if the peron ya hunting dies b4 ya get there */
  if((subcmd == /*SCMD_HUNT*/SKILL_HUNT || subcmd == SCMD_AUTOHUNT) && HUNTING(ch))
    if((HUNTING(ch)->in_room < 0 || HUNTING(ch)->in_room > top_of_world) ||
       (GET_HIT(HUNTING(ch)) <= 0)) 
    {
      send_to_char("You can no longer find a path to your prey!\r\n",ch);
      stop_hunting(ch);
      return;
    }
    dir = find_first_step(ch, ch->in_room, vict->in_room);
    switch (dir) 
    {
      case BFS_ERROR:
	send_to_char("Hmm.. something seems to be wrong.\r\n", ch);
	break;
      case BFS_ALREADY_THERE:
	if ((subcmd == /*SCMD_HUNT*/SKILL_HUNT || subcmd == SCMD_AUTOHUNT) && 
	    HUNTING(ch)) 
	{
	  send_to_char("You've found your prey!\r\n",ch);
	  stop_hunting(ch);
	} else {
	  send_to_char("You're already in the same room!!\r\n", ch);
	}
	break;
      case BFS_NO_PATH: 
	sprintf(buf, "You can't sense a trail to %s from here.\r\n",
		HMHR(vict));
	send_to_char(buf, ch);
	break;
      default:
	if (!IS_SET(GET_SPECIALS(ch), SPECIAL_TRACKER))
	{
	  num = number(0, 101);       /* 101% is a complete failure */
	  if (GET_SKILL(ch, SKILL_TRACK) < num)
	    dir = 0;
	  else if (subcmd == SKILL_TRACK)
	    apply_spell_skill_abil(ch, SKILL_TRACK);
	  else
	    apply_spell_skill_abil(ch, SKILL_HUNT);
	}
/* this can sometimes get into an endless loop */
/*      do {
        dir = number(0, NUM_OF_DIRS - 1);
      } while (!CAN_GO(ch, dir));
*/
	sprintf(buf, "You sense a trail %s from here!\r\n", dirs[dir]);
	send_to_char(buf, ch);
	break;
    }
} 

void hunt_victim(struct char_data * ch)
{
  int dir;

  if (!ch || !HUNTING(ch) || FIGHTING(ch))
    return;

  if ((!valid_hunt(ch)) || (IN_ROOM(ch) == NOWHERE) || 
      (IN_ROOM(HUNTING(ch)) == NOWHERE))
  {
    do_say(ch, "Damn!  My prey is gone!!", 0, 0);
    stop_hunting(ch);
    return;
  }
  if ((dir = find_first_step(ch, ch->in_room, HUNTING(ch)->in_room)) < 0)
  {
    sprintf(buf, "Damn!  I lost %s!", HMHR(HUNTING(ch)));
    do_say(ch, buf, 0, 0);
    stop_hunting(ch);
  } else {
    perform_move(ch, dir, 1);
    if (ch->in_room == HUNTING(ch)->in_room)
      hit(ch, HUNTING(ch), TYPE_UNDEFINED);
  }
}



void do_playerhunt( struct char_data *ch, struct char_data *victim ) {

  	ACMD(do_say);
        int direction = getMeHere(ch->in_room, victim->in_room);
 
        if( direction != -1 )
                perform_move( ch, direction, 0);
 
        // Check if we're there
        if( ch->in_room == victim->in_room ) {
                sprintf(buf1, "Your time is at hand %s, defend thyself!!", victim->player.name);
                do_say(ch, buf1, 0,0);
                hit(ch, victim, TYPE_UNDEFINED);
                return;
        }
} 

int getMeHere(sh_int chroom, sh_int victroom) {
 
        int currdir = 0;
        sh_int curr_room = 0;
 
        // Check if the rooms are the same
        if( chroom == victroom )
                return -1;
 
        // Clear all marks
        for (curr_room = 0; curr_room <= top_of_world; curr_room++)
                UNMARK(curr_room);
 
        MARK(chroom);                                   // Mark our start room
 
        // Get the initial directions available from current room, creating queue
        for( currdir = 0; currdir < NUM_OF_DIRS; currdir++) {
                if(MY_VALID_EDGE(chroom, currdir) ) {                   // If room exists
                        MARK( TOROOM(chroom, currdir));                 // Mark it
                        bfs_enqueue( TOROOM(chroom, currdir), currdir); // Queue it
                }
        }
        // now, loop through all the potential directions,
        // and return the direction to move in
        while( queue_head  ) {
                if( queue_head->room == victroom ) {                    // If we're at the last room
                        currdir = queue_head->dir;                      // Set direction
                        bfs_clear_queue();                              // Clean up
                        return currdir;                                 // Move
                }
                else {                                                  // Didn't find target at head
                        // Check this room for potential paths, add them to the queue if valid
                        for( currdir = 0; currdir < NUM_OF_DIRS; currdir++ ) {
                                if( MY_VALID_EDGE(queue_head->room, currdir) ) {
                                        MARK( TOROOM(queue_head->room, currdir) );
                                        bfs_enqueue( TOROOM(queue_head->room, currdir), currdir );
                                }
                        }
                        bfs_dequeue();          // Got all paths off this one, remove it
 
                }
        }
 
        // If we're here, there's no valid path
        return -1;
} 

int MY_VALID_EDGE(sh_int room, int dir ) {
 
        // If direction is invalid, or room goes nowhere
        if ((world[room].dir_option[dir] == NULL) || (TOROOM(room, dir) == NOWHERE)
	    || (TOROOM(room, dir) == room) )
                return 0;
 
#ifdef TRACK_THROUGH_DOORS
#else
        // If there's a closed door in our way
        if (IS_CLOSED(room, dir))
                return 0;
#endif
 
        // If the room's been marked already, we don't want it
        if( IS_MARKED(TOROOM(room, dir)) )
                return 0;
 
        // If the room's marked with a !MOB flag, invalidate it
        if( ROOM_FLAGGED(TOROOM(room,dir), ROOM_NOMOB) )
                return 0;
 
        return 1;                       // Valid edge
} 


int EscapeValidEdge(int x, int y)
{
  if (world[x].dir_option[y] == NULL)
    return 0;
  if ((TOROOM(x, y) == NOWHERE) || (world[x].dir_option[y] == NULL) || 
      (TOROOM(x, y) == x))
    return 0;
  if (IS_MARKED(TOROOM(x, y)) || ROOM_FLAGGED(TOROOM(x, y), ROOM_GODROOM) || 
       ROOM_FLAGGED(TOROOM(x, y), ROOM_DEATH) || 
       ROOM_FLAGGED(TOROOM(x, y), ROOM_PRIVATE))
    return 0;
  return 1;
}

int Escape(struct char_data *ch)
{
  int currdir = 0, chroom = ch->in_room;
  sh_int curr_room = 0;
	 
  // Clear all marks
  for (curr_room = 0; curr_room <= top_of_world; curr_room++)
    UNMARK(curr_room);
 
  MARK(chroom);                                   // Mark our start room
	
  // Get the initial directions available from current room, creating queue
  for (currdir = 0; currdir < NUM_OF_DIRS; currdir++)
    if(EscapeValidEdge(chroom, currdir))
    {                   // If room exists
      MARK(TOROOM(chroom, currdir));                 // Mark it
      bfs_enqueue(TOROOM(chroom, currdir), currdir); // Queue it
    }
  // now, loop through all the potential directions,
  // and return the direction to move in
  while (queue_head)
  {
    if (SECT(queue_head->room) != SECT_INSIDE &&
      !ROOM_FLAGGED(queue_head->room, ROOM_INDOORS))
    {           // If we're out
      chroom = queue_head->room;
      bfs_clear_queue();                    // Clean up
      return chroom;                        // Move
    } else {                                // Didn't find target at head
      // Check this room for potential paths, add them to the queue if valid
      for (currdir = 0; currdir < NUM_OF_DIRS; currdir++)
	if (EscapeValidEdge(queue_head->room, currdir))
	{
	  MARK(TOROOM(queue_head->room, currdir));
	  bfs_enqueue(TOROOM(queue_head->room, currdir), currdir);
	}
      bfs_dequeue();          // Got all paths off this one, remove it
    }
  }
  return NOWHERE;		// Can't get outta here!
}
