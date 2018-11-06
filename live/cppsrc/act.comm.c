/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
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
#include "constants.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "spells.h"
#include "improved-edit.h" 
#include "dg_scripts.h"
#include "mail.h"

#define LAST_GOSSIPS_TO_SAVE 10
GossipItem last_gossips[LAST_GOSSIPS_TO_SAVE];

/* extern variables */
extern int level_can_shout;
extern int holler_move_cost;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct auc_data *auc_list;
extern int lot_tot;   

struct char_data *get_player_by_id(long id);
void write_auction_file();

/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
void do_auc_stat(struct char_data *ch, int lot);
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
// ACMD(do_gen_write);
ACMD(do_qcomm);

/* modified do_say to affect speech when drunk.. - Vader */
#define DRUNKNESS    ch->player_specials->saved.conditions[DRUNK]
#define PISS_FACTOR  (25 - DRUNKNESS) * 50

ACMD(do_say)
{
  char *speech;
  unsigned int i,j;

  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  else {
   speech = str_dup(argument);
/* this loop goes thru and randomly drops letters from what was said
 * depanding on how drunk the person is.. 24 is max drunk.. when someone
 * is 24 drunk they pretty much cant be understood.. - Vader
 */
   for(i = 0,j = 0; i < strlen(speech); i++) {
     if(number(1,PISS_FACTOR) > DRUNKNESS) {
       speech[j] = speech[i];
       j++;
       }
     }
     speech[j] = '\0';
	
     if(!IS_NPC(ch) && UNDERWATER(ch)){
        send_to_char("Bubbles raise from your mouth as you speak!\r\n",ch);
        act("Bubbles raise from $n's mouth as $e speaks.", TRUE, ch, 0, 0, TO_ROOM);
      }

    sprintf(buf, "&n$n says, '%s&n'", speech); 
    //act(buf, FALSE, ch, 0, 0, TO_ROOM);
    act(buf, FALSE, ch, 0, 0, TO_ROOM|DG_NO_TRIG);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      delete_doubledollar(argument);
      sprintf(buf, "&nYou say, '%s&n'\r\n", speech);  
      send_to_char(buf, ch);
    }
  }
  
  /* trigger check */
  speech_mtrigger(ch, argument);
  speech_wtrigger(ch, argument);
}


ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("But you are not the member of a group!\r\n", ch);
    return;
  }
  if (!*argument)
    send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
  else {
    if (ch->master)
      k = ch->master;
    else
      k = ch;

   sprintf(buf, "&R$n &rtells the group, '%s&r'&n", argument);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "&rYou tell the group, '%s&r'&n\r\n", argument);   
      send_to_char(buf, ch);
    }
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  sprintf(buf, "$n &rtells you, '%s&r'&n", arg);
  act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    sprintf(buf, "&rYou tell $N&r, '%s&r'&n", arg);
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  }

  if (!IS_NPC(vict) && !IS_NPC(ch))
    GET_LAST_TELL(vict) = GET_IDNUM(ch);

// The above is stock circle code and the below was primal
//  GET_LAST_TELL(vict) = ch;
}

int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
#ifdef NOTELL_SOUNDPROOF
  else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
    send_to_char("The walls seem to absorb your words.\r\n", ch);
#endif
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL))
#ifdef NOTELL_SOUNDPROOF
           || ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF)
#endif
          )
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (TRUE);

  return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = NULL;
  int i;
  struct char_data *generic_find_char(struct char_data *ch, char *arg, 
                                      int where);

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char("Who do you wish to tell what??\r\n", ch);
  else if (!(vict = generic_find_char(ch, buf, FIND_CHAR_WORLD)))
    send_to_char(NOPERSON, ch);
  else if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (IS_BUILDING(vict))
    act("$E's building right now; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (!IS_NPC(vict) && PLR_FLAGGED(vict, PLR_WRITING | PLR_REPORTING | PLR_ODDWRITE))
    act("$E's writing a message right now; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL))
    act("$E's deaf to your tells; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_AFK))
    act("$E's is marked (AFK); try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AFK))
    send_to_char("It is unfair to tell people things while marked AFK.  They can't reply!!.\r\n", ch);
  else if (GET_IGN_LVL(vict) > GET_LEVEL(ch) && !IS_NPC(ch)) {
      act("$E's ignoring you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP );
      return;
  } else {    
    if (!IS_NPC(ch) && (LR_FAIL(ch, MAX(GET_LEVEL(vict), LVL_IS_GOD))))
      for (i=0; i < MAX_IGNORE; i++) {
        if (GET_IGNORE(vict, i) == GET_IDNUM(ch)) {       
          act("$E's ignoring you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP );
          return;
        }
      } 
    perform_tell(ch, vict, buf2);
  }
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char("You have no-one to reply to!\r\n", ch);
  else if (!*argument)
    send_to_char("What is your reply?\r\n", ch);
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */
				     
    /*
     * XXX: A descriptor list based search would be faster although
     *      we could not find link dead people.  Not that they can
     *      hear tells anyway. :) -gg 2/24/98
     */
    while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (tch == NULL)
      send_to_char("They are no longer playing.\r\n", ch);
    else if (is_tell_ok(ch, tch))
      perform_tell(ch, tch, argument);
  }
}


ACMD(do_spec_comm)
{
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  switch (subcmd) {
  case SCMD_WHISPER:
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    break;

  case SCMD_ASK:
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    break;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
    send_to_char(buf, ch);
  } else if (!(vict = generic_find_char(ch, buf, FIND_CHAR_ROOM)))
    send_to_char(NOPERSON, ch);
  else if (vict == ch)
    send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
  else {
    sprintf(buf, "&C$n &c%s you, '%s&c'&n", action_plur, buf2);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "&cYou %s &C%s&c, '%s&c'&n\r\n", action_sing, GET_NAME(vict), buf2);
      send_to_char(buf, ch);
    }
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper, *pen = NULL;
  char *papername, *penname;

  if (IS_NPC(ch)) 
  {
    send_to_char("Your a mob, you can hardly read let alone write.\r\n", ch);
    return;
  }
    
  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) 
  {		/* nothing was delivered */
    send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
    return;
  }
  if (*penname) 
  {		/* there were two arguments */
    if (!(paper = generic_find_obj(ch, papername, FIND_OBJ_INV)))
    {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (!(pen = generic_find_obj(ch, penname, FIND_OBJ_INV)))
    {
      sprintf(buf, "You have no %s.\r\n", penname);
      send_to_char(buf, ch);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = generic_find_obj(ch, papername, FIND_OBJ_INV)))
    {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN)	/* oops, a pen.. */
    {
      pen = paper;
      paper = NULL;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char("That thing has nothing to do with writing.\r\n", ch);
      return;
    }
    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD)) {
      sprintf(buf, "You can't write with %s %s alone.\r\n", AN(papername),
	      papername);
      send_to_char(buf, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
      send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD);
    else
      pen = GET_EQ(ch, WEAR_HOLD);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else {
    //char *backstr = NULL;

    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
      //backstr = str_dup(paper->action_description);
      send_to_char("There's something written on it already:\r\n", ch);
      //send_to_char(paper->action_description, ch);
    }
  
    /* we can write - hooray! */
    /* this is the PERFECT code example of how to set up:
     * a) the text editor with a message already loaed
     * b) the abort buffer if the player aborts the message
     */
    ch->desc->backstr = NULL;
    send_to_char("Write your note.  (/s saves /h for help)\r\n", ch);
    /* ok, here we check for a message ALREADY on the paper */
    if (paper->action_description) {
      /* we str_dup the original text to the descriptors->backstr */
      ch->desc->backstr = str_dup(paper->action_description);
      /* send to the player what was on the paper (cause this is already */
      /* loaded into the editor) */
      send_to_char(paper->action_description, ch);
    }

    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    //send_editor_help(ch->desc);
    /* assign the descriptor's->str the value of the pointer to the text */
    /* pointer so that we can reallocate as needed (hopefully that made */
    /* sense :>) */
    //string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, NULL);

  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char("Monsters can't page.. go away.\r\n", ch);
  else if (!*arg)
    send_to_char("Whom do you wish to page?\r\n", ch);
  else {
    sprintf(buf, "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "all")) {
      if (!LR_FAIL(ch, LVL_GOD)) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char("You will never be godly enough to do that!\r\n", ch);
      return;
    }
    if ((vict = generic_find_char(ch, arg, FIND_CHAR_WORLD)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char("There is no such person in the game!\r\n", ch);
  }
}

void show_last_gossips(struct char_data *ch)
{
  int nCount = 0;
  sprintf(buf2, "&BThe last &M%d&B gossips were:\r\n&n", 
	  LAST_GOSSIPS_TO_SAVE);
  send_to_char(buf2, ch);
  for (int i = 0; i < LAST_GOSSIPS_TO_SAVE; i++)
  {
    last_gossips[i].getGossip(ch, buf2);
    if (strcmp(buf2, "") == 0)
      continue;
    send_to_char(buf2, ch);
    nCount++;
  }
  if (nCount == 0)
    send_to_char("   Nothing has been said.\r\n", ch);
}

void log_gossip(struct char_data *ch, char *gossip)
{
  // move the gossips up one
  for (int i = 1; i < LAST_GOSSIPS_TO_SAVE; i++)
    last_gossips[i - 1].copyGossip(last_gossips[i]);
  last_gossips[LAST_GOSSIPS_TO_SAVE - 1].setGossip(ch, gossip);
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  struct char_data *realch;
  char color_on[24];
  char s[800];/* zap */       
  char *speech;
  unsigned int l,j,ignore; 

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    EXT_NONEWBIE
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][4] = {
    {"You cannot holler!!\r\n",
      "&yholler",
      "",
    KYEL},

    {"You cannot shout!!\r\n",
      "&yshout",
      "Turn off your noshout flag first!\r\n",
    KYEL},

    {"You cannot gossip!!\r\n",
      "&ygossip",
      "You aren't even on the channel!\r\n",
    KYEL},

    {"You cannot auction!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n",
    KMAG},

    {"You cannot congratulate!\r\n",
      "&gcongrat",
      "You aren't even on the channel!\r\n",
    KGRN},

    {"You cannot use the newbie channel!\r\n",
      "&cnewbie",
      "You aren't even on the channel!\r\n",
    KCYN}//,
 
  };

  /* changed so mobs can shout and holler in spec procs - Vader */
  if (!ch->desc && subcmd != SCMD_SHOUT && subcmd != SCMD_HOLLER)
    return;  

/*  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(com_msgs[subcmd][0], ch);
    return;
  }
  -- Replaced with PUN_FLAGGED.... -- ARTUS
*/
 
  if (!IS_NPC(ch))
  {
    if (PUN_FLAGGED(ch, PUN_MUTE)) { /* ARTUS - Punishment Replace NOSHOUT */
      send_to_char(com_msgs[subcmd][0], ch);
      if (ch->player_specials->saved.phours[PUN_MUTE] < 0)
	sprintf(buf, "This must be removed by an immortal, you must have been bad.\r\n");
      else
	sprintf(buf, "This will be removed in %dhrs.\r\n", ch->player_specials->saved.phours[PUN_MUTE]);
      send_to_char(buf, ch);
      return;
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
    {
      send_to_char("The walls seem to absorb your words.\r\n", ch);
      return;
    }
    /* level_can_shout defined in config.c */
    if ((subcmd != SCMD_NEWBIE) && LR_FAIL_MAX(ch, level_can_shout))
    {
      sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
	      level_can_shout, com_msgs[subcmd][1]);
      send_to_char(buf1, ch);
      return;
    }
    /* make sure the char is on the channel */
    if (PRF_FLAGGED(ch, channels[subcmd]) && subcmd <= SCMD_GRATZ)
    {
      send_to_char(com_msgs[subcmd][2], ch);
      return;
    }
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument)
  {
    sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
	    com_msgs[subcmd][1], com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }

  // log newbie channel
  // if (subcmd == SCMD_NEWBIE)
  //   do_gen_write(ch,argument,0,SCMD_NEWBIE); 

  if ((subcmd == SCMD_HOLLER) && !IS_NPC(ch))
  {
    if (GET_MOVE(ch) < holler_move_cost)
    {
      send_to_char("You're too exhausted to holler.\r\n", ch);
      return;
    } else
      GET_MOVE(ch) -= holler_move_cost;
  }

  if (scan_buffer_for_xword(argument))
  {
    sprintf(s,"WATCHLOG SWEAR: %s %ss, '%s'", ch->player.name,com_msgs[subcmd][1], argument);
    mudlog(s,NRM,LVL_IMPL,TRUE);
    send_to_char("Please dont swear on the open channels.&n\r\n",ch);
    return ;
  }

  speech = str_dup(argument);
 
  if(strcmp(speech, "last") == 0 && subcmd == SCMD_GOSSIP)
  {
    show_last_gossips(ch);
    return;
  }

/* this loop goes thru and randomly drops letters from what was said
 * depanding on how drunk the person is.. 24 is max drunk.. when someone
 * is 24 drunk they pretty much cant be understood.. - Vader
 */
  for(l = 0,j = 0; l < strlen(speech); l++)
    if(number(1,PISS_FACTOR) > DRUNKNESS)
    {
      speech[j] = speech[l];
      j++;
    }

  /* set up the color on code */
  strcpy(color_on, com_msgs[subcmd][3]);

  /* first, set up strings to be given to the communicator */
  if (!IS_NPC(ch) &&PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    // was C_CMP - DM
    if (COLOR_LEV(ch) >= C_NRM)
     sprintf(buf1, "%sYou %s, '%s&n%s'&n", color_on, com_msgs[subcmd][1],
              speech, color_on);
    else
      sprintf(buf1, "You %s, '%s&n'", com_msgs[subcmd][1], speech);
    act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
  }

  if (subcmd == SCMD_GOSSIP)
    log_gossip(ch, speech);

  // The actual gos..
  sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], argument);


  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    realch = (i->original) ? i->original : i->character;
    // ARTUS - Bug fixed (gen_comms not being sent to chars..
    //     if (STATE(i) == CON_PLAYING && realch && i != realch->desc && 
    if (STATE(i) == CON_PLAYING && realch && i != ch->desc &&
       ((!PRF_FLAGGED(realch, channels[subcmd]) && subcmd <= SCMD_GRATZ )||
       (!EXT_FLAGGED(realch, channels[subcmd]) && subcmd >= SCMD_NEWBIE ))&&
        !PLR_FLAGGED(realch, PLR_WRITING) &&
        !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      // Ignore all communication
      ignore = FALSE;
      if (!IS_NPC(ch))
        for (j=0; j < MAX_IGNORE; j++)
	{
          if (GET_IGNORE(i->character,j) == GET_IDNUM(ch) && GET_IGNORE_ALL(i->character,j))
            ignore = TRUE;
        }

      if (ignore)
        continue;

      if (subcmd == SCMD_SHOUT &&
	  ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	   !AWAKE(i->character)))
	continue;


      if (COLOR_LEV(i->character) >= C_NRM)
        sprintf(buf, "%s$n %ss, '%s&n%s'&n", color_on, com_msgs[subcmd][1], speech, color_on);
      else
        sprintf(buf, "$n %ss, '%s'&n", com_msgs[subcmd][1], speech);

       act(buf, TRUE, ch, 0, i->character, TO_VICT | TO_SLEEP);

//      if (COLOR_LEV(i->character) >= C_NRM)
//	send_to_char(color_on, i->character);
//     if (COLOR_LEV(i->character) >= C_NRM)
//	send_to_char(KNRM, i->character);
    }
  }
}

/* Artus> It would be nice to use this shit again at some point. */
int scan_buffer_for_xword(char* buf)
{
  char tmpword[MAX_INPUT_LENGTH+65];
  unsigned int i;
  int count = -1;
  for (i=0;i<=strlen(buf);i++)
  {
    count++;
    tmpword[count]=buf[i];
    if (tmpword[count]==' ' || tmpword[i]=='\n' || i==strlen(buf))
    {
      tmpword[count]='\0';
      count = -1;
      if (!Valid_Name(tmpword, FALSE))
        return 1;
    }
  }
  return 0;
} 

/* =================== Auction Routines -- Artus =================== */

// Artus> Show help syntax.
void show_auction_help(struct char_data *ch, int subcmd)
{
  switch (subcmd)
  {
    case AUC_BID:
      send_to_char("Syntax: Auction Bid <item #> <amount>\r\n"
	           "  Bid <amount> on auctioned item <item #>\r\n", ch);
      break;
    case AUC_PURGE:
      send_to_char("Syntax: Auction Purge <item #>\r\n"
	           "  Removes <item #> from the auction list.\r\n", ch);
      break;
    case AUC_SELL:
      send_to_char("Syntax: Auction Sell <item> <starting bid>\r\n"
	           "  Sells <item> for a minimum total of <starting bid>\r\n", ch);
      break;
    case AUC_STAT:
      sprintf(buf, "Syntax: Auction Stat <item #>\r\n"
	           "  Displays some statistics of auction <item #>\r\n"
		   "  This service is charged at %d coins.\r\n", 
		   AUC_STAT_COST);
      send_to_char(buf, ch);
      break;
    case AUC_NONE:
    default:
      send_to_char("Auction bid <item #> <amount>\r\n"
		   "Auction cancel\r\n"
		   "Auction list\r\n"
		   "Auction sell <item> <starting bid>\r\n"
		   "Auction sold\r\n"
		   "Auction stat <item #>\r\n", ch);
      if (LR_FAIL(ch, LVL_GRGOD))
        break;
      send_to_char("Auction purge <item #>\r\n", ch);
  }
  return;
}

// Send an auction email to player.
void auction_mail_to(struct auc_data *lot, int target, int subcmd)
{
  char mailstr[MAX_STRING_LENGTH] = "";
  char *tmp_name = NULL;
  struct char_data *tch = NULL;

  // If they're online they'll see the message.
  if ((tch = get_player_by_id(target)) && 
      !ROOM_FLAGGED(IN_ROOM(tch), ROOM_SOUNDPROOF))
    return;

  switch (subcmd)
  {
    case AUC_SOLD:
      sprintf(mailstr, 
  "Congratuations, &7%s&n, you have won the auction of item #%d.\r\n\r\n"
  "  Item Name  : &5%s&n\r\n"
  "  Your Bid   : &Y%ld&n\r\n"
  "  Seller Name; &7%s&n\r\n\r\n"
  "This item has been automatically placed into your inventory, and the\r\n"
  "gold automatically deducted from your account.\r\n",
	lot->buyername, lot->idnum, lot->obj->short_description, lot->offer,
	lot->sellername);
      break;
    case AUC_BID:
      if (target == lot->buyerid)
      {
	tmp_name = get_name_by_id(target);
	if (!(tmp_name))
	  return;
	sprintf(mailstr,
  "Dear &7%s&n,\r\n\r\n"
  "While you were away, the auction you were bidding on received a\r\n"
  "bid higher than your previous bid. The details are as follows:\r\n\r\n"
  "  Item Number: &y%d&n\r\n"
  "  Item Name  : &5%s&n\r\n"
  "  New Bid    : &Y%ld&n\r\n"
  "  New Bidder : &7%s&n\r\n\r\n"
  "Good luck with your future bidding!\r\n",
          tmp_name, lot->idnum, lot->obj->short_description, lot->offer,
	  lot->buyername);
      } else {
        sprintf(mailstr,
  "Dear &7%s&n,\r\n\r\n"
  "While you were away, the auction you are hosting received a new bid.\r\n"
  "The details are as follows:\r\n\r\n"
  "  New Bid   : &Y%ld&n\r\n"
  "  New Bidder: &7%s&n\r\n\r\n"
  "Mmmmmmmmm... Coin.\r\n",
          lot->sellername, lot->offer, lot->buyername);
      }
      break;
    case AUC_CANCEL:
      sprintf(mailstr,
  "Dear &7%s&n,\r\n\r\n"
  "While you were away, the auction you were bidding on was cancelled by\r\n"
  "the seller. The details are as follows:\r\n\r\n"
  "  Seller Name: &7%s&n\r\n"
  "  Item Number: &y%d&n\r\n"
  "  Item Name  : &5%s&n\r\n\r\n"
  "Go kick their butt! :o)\r\n",
              lot->buyername, lot->sellername, lot->idnum, 
	      lot->obj->short_description);
      break;
    case AUC_PURGE:
      sprintf(mailstr,
  "Dear &y%s&n,\r\n\r\n"
  "While you were away, the auction you were %s was purged, probably\r\n"
  "%s idle, or the auction was deemed unfit for the system.\r\n"
  "The details are as follows:\r\n\r\n"
  "  Item Number: &y%d&n\r\n"
  "  Item Name  : &5%s&n\r\n"
  "  Seller Name: &7%s&n\r\n\r\n"
  "Better luck next time!\r\n", 
          (target == lot->buyerid) ? lot->buyername : lot->sellername,
	  (target == lot->buyerid) ? "bidding on" : "hosting",
	  (target == lot->buyerid) ? "the seller was" : "you were",
	  lot->idnum, lot->obj->short_description, lot->sellername);
      break;
    default:
      sprintf(buf, "SYSERR: Unknown subcommand passed to auction_mail_to: %d.",
	      subcmd);
      mudlog(buf, NRM, LVL_IMPL, TRUE);
  }
  store_mail(target, MAIL_FROM_AUCTION, mailstr);
}

// Find an auction based on the auction number.
struct auc_data *find_auction_by_id(int target)
{
  struct auc_data *lot;
  for (lot = auc_list; lot; lot = lot->next)
    if (lot->idnum == target)
      return lot;
  return NULL;
}

// Find an auction based on the seller idnum.
struct auc_data *find_auction_by_seller(int target)
{
  struct auc_data *lot;
  for (lot = auc_list; lot; lot = lot->next)
    if (lot->sellerid == target)
      return lot;
  return NULL;
}

// Give the offered amount to the target.
void auction_offer_to(struct auc_data *lot, int target)
{
  struct char_file_u tmp_store;
  struct char_data *ch;
  extern FILE *player_fl;
  int player_i = 0;

  if (target < 1)
    return;
  ch = get_player_by_id(target);
  if (ch)
  {
    GET_BANK_GOLD(ch) += lot->offer;
    return;
  }
  player_i = load_char(get_name_by_id(target), &tmp_store);
  if (player_i < 0)
  {
    sprintf(buf, "SYSERR: load_char failed in auction_offer_to. [%s/%d/%d]", 
	    get_name_by_id(target), target, player_i);
    mudlog(buf, NRM, LVL_IMPL, TRUE);
    return;
  }
  // They logged off.. Load them from file.
  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  store_to_char(&tmp_store, ch);
  char_to_room(ch, 0);
  GET_BANK_GOLD(ch) += lot->offer;
  char_to_store(ch, &tmp_store);
  fseek(player_fl, (player_i) *sizeof(struct char_file_u), SEEK_SET);
  fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
  fflush(player_fl);
  free(ch);
}

// Give the item to the target.
void auction_item_to(struct auc_data *lot, int target)
{
  struct char_file_u tmp_store;
  struct char_data *ch;
  char filename[50];
  FILE *fd;
  struct rent_info rent;
  struct tmp_elem {
    struct obj_file_elem obj;
    struct tmp_elem *next;
  };
  struct tmp_elem *ele_index=NULL, *prev=NULL, *ele=NULL;
  void Obj_to_file_elem(struct obj_data *obj, struct obj_file_elem *target,
                        int location);
  
  if (target < 1)
    return;
  if ((ch = get_player_by_id(target)))
  {
    obj_from_room(lot->obj);
    obj_to_char(lot->obj, ch, __FILE__, __LINE__);
    return;
  }
  if (load_char(get_name_by_id(target), &tmp_store) < 0)
  {
    obj_from_room(lot->obj);
    extract_obj(lot->obj);
    return;       
  }
  // They logged off.. Load them from file.
  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  store_to_char(&tmp_store, ch);
  obj_from_room(lot->obj);
  char_to_room(ch, 0);
  // Don't worry about it if they're deleted.
  if (!PLR_FLAGGED(ch, PLR_DELETED) &&
      get_filename(GET_NAME(ch), filename, CRASH_FILE) &&
      (fd = fopen(filename, "rb")))
  {
    fread(&rent, sizeof(struct rent_info), 1, fd);
    while (!feof(fd))
    {
      CREATE(ele, struct tmp_elem, 1);
      fread(&ele->obj, sizeof(struct obj_file_elem), 1, fd);
      if (ferror(fd))
	break;
      ele->next = NULL;
      if (feof(fd))
	break;
      if (prev)
	prev->next = ele;
      else
	ele_index = ele;
      prev = ele;
    }
    Obj_to_file_elem(lot->obj, &ele->obj, 0);
    if (prev)
      prev->next = ele;
    else
      ele_index = ele;
    fclose(fd);
    fd = fopen(filename, "wb");
    fwrite(&rent, sizeof(struct rent_info), 1, fd);
    ele = ele_index;
    while (ele)
    {
      struct tmp_elem *next_ele;
      next_ele = ele->next;
      fwrite(&ele->obj, sizeof(struct obj_file_elem), 1, fd);
      free(ele);
      ele = next_ele;
    }
    fclose(fd);
  }
  extract_obj(lot->obj);
  extract_char(ch);
}

// Delete an auction.
void auction_delete(struct auc_data *lot, int subcmd)
{
  struct auc_data *temp; // Needed for REMOVE_FROM_LIST

  switch (subcmd)
  {
    case AUC_CANCEL:
    case AUC_PURGE:
      auction_offer_to(lot, lot->buyerid);
      auction_item_to(lot, lot->sellerid);
      break;
    case AUC_SOLD:
      auction_offer_to(lot, lot->sellerid);
      auction_item_to(lot, lot->buyerid);
      break;
    default:
      sprintf(buf, "SYSERR: Unknown subcommand [%d] passed to auction_delete().", subcmd);
      mudlog(buf, NRM, LVL_IMPL, TRUE);
  }
  REMOVE_FROM_LIST(lot, auc_list, next);
  lot->obj = NULL;
  free(lot);
  write_auction_file();
}

// Send a message to all on the mud.
void auction_spam(struct auc_data *lot, struct char_data *ch, int subcmd)
{
  switch (subcmd)
  {
    case AUC_BID:
      sprintf(buf1, "&m[Auction]: $n has bid &Y%ld&m on Item #&y%d&m!&n",
	      lot->offer, lot->idnum);
      break;
    case AUC_SELL:
      sprintf(buf1, "&m[Auction]: $n is selling $p for %ld (Item #%d.)&n", lot->offer, lot->idnum);
      break;
    case AUC_CANCEL:
      strcpy(buf1, "&m[Auction]: $n has cancelled the sale of $p.&n");
      break;
    case AUC_PURGE:
      sprintf(buf1, "&m[Auction]: Auction #&y%d&m has been purged.&n",lot->idnum);
      break;
    case AUC_SOLD:
      sprintf(buf1, "&m[Auction]: $n has sold $p to %s for %ld.&n", lot->buyername, lot->offer);
      break;
    default:
      sprintf(buf1, "SYSERR: Unknown subcmd [%d] in auction_spam().", subcmd);
      mudlog(buf1, NRM, LVL_IMPL, TRUE);
      return;
  }
  for (struct descriptor_data *d = descriptor_list; d; d = d->next)
  {
    if ((STATE(d) != CON_PLAYING) || !(d->character) || IS_NPC(d->character) || 
	(PRF_FLAGGED(d->character, PRF_NOAUCT)))
      continue;
    act(buf1, FALSE, ch, lot->obj, d->character, TO_VICT);
  }
}

// Bid on an auction.
void auction_bid(struct char_data *ch, char *argument)
{
  int amount, min_amt;
  unsigned int item, was_buying = 0;
  struct auc_data *lot;

  skip_spaces(&argument);
  if (!*argument)
  {
    show_auction_help(ch, AUC_BID);
    return;
  }
  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if ((!*arg) || !is_number(arg) || !is_number(argument))
  {
    show_auction_help(ch, AUC_BID);
    return;
  }
  amount = atoi(argument);
  if (amount < 1) 
  {
    send_to_char("You can't have it for free!\r\n", ch);
    return;
  }
  item = atoi(arg);
  if (!(lot = find_auction_by_id(item)))
  {
    send_to_char("That auction is a figment of your imagination.\r\n", ch);
    return;
  }
  if (lot->sellerid == GET_IDNUM(ch))
  {
    send_to_char("You can't bid on your own auction!\r\n", ch);
    return;
  }
  if (lot->buyerid == GET_IDNUM(ch))
  {
    send_to_char("But you're already the high bidder!\r\n", ch);
    return;
  }
  if (lot->buyerid == 0) 
    min_amt = lot->offer;
  else
    min_amt = MAX(10, lot->offer + (int)(lot->offer/100));
  if (amount < min_amt)
  {
    sprintf(buf, "The minimum amount you can offer is %d.\r\n", min_amt);
    send_to_char(buf, ch);
    return;
  }
  if (GET_BANK_GOLD(ch) < amount)
  {
    send_to_char("You don't have that much gold in the bank!\r\n", ch);
    return;
  }
  auction_offer_to(lot, lot->buyerid);
  was_buying = lot->buyerid;
  lot->buyerid = GET_IDNUM(ch);
  GET_BANK_GOLD(ch) -= amount;
  lot->offer = amount;
  memset(lot->buyername, '\0', MAX_NAME_LENGTH+1);
  strncpy(lot->buyername, GET_NAME(ch), MAX_NAME_LENGTH);
  auction_mail_to(lot, was_buying, AUC_BID);
  auction_mail_to(lot, lot->sellerid, AUC_BID);
  write_auction_file();
  auction_spam(lot, ch, AUC_BID);
}

// Cancel an auction.
void auction_cancel(struct char_data *ch)
{
  struct auc_data *lot;
  if (!(lot = find_auction_by_seller(GET_IDNUM(ch))))
  {
    send_to_char("But you aren't auctioning anything!\r\n", ch);
    return;
  }
  auction_spam(lot, ch, AUC_CANCEL);
  auction_mail_to(lot, lot->buyerid, AUC_CANCEL);
  auction_delete(lot, AUC_CANCEL);
}

// Create an auction record.
void auction_create(struct char_data *ch, struct obj_data *obj, int amount)
{
  struct auc_data *lot, *last=NULL, *next=NULL;
  int idnum = 1;
  
  if ((!ch) || (!obj) || (amount < 1))
    return;

  // Deterimne the auction's idnum.
  for (struct auc_data *k = auc_list; k; k = k->next)
  {
    if (idnum == k->idnum)
    {
      next = k->next;
      idnum++;
    } else {
      next = k;
      break;
    }
    last = k;
  }
  CREATE(lot, struct auc_data, 1);
  lot->idnum = idnum;
  memset(lot->sellername, '\0', MAX_NAME_LENGTH+1);
  memset(lot->buyername, '\0', MAX_NAME_LENGTH+1);
  strncpy(lot->sellername, GET_NAME(ch), MAX_NAME_LENGTH+1);
  strncpy(lot->buyername, "noone", MAX_NAME_LENGTH+1);
  lot->sellerid = GET_IDNUM(ch);
  lot->buyerid = 0;
  lot->obj = obj;
  if (next)
    lot->next = next;
  else 
    lot->next = NULL;
  lot->offer = amount;
  if (!(last))
    auc_list = lot;
  else
    last->next = lot;
  obj_from_char(obj);
  obj_to_room(obj, real_room(AUC_ROOM));
  auction_spam(lot, ch, AUC_SELL);
  write_auction_file();
}

// Sell an Item.
void auction_sell(struct char_data *ch, char *argument)
{
  int amount = 0, auc_count = 0;
  struct obj_data *obj = NULL;
  skip_spaces(&argument);
  if (!*argument)
  {
    show_auction_help(ch, AUC_SELL);
    return;
  }
  argument = one_argument(argument, arg);
  skip_spaces(&argument);
  if (!*argument || !*arg || !is_number(argument))
  {
    show_auction_help(ch, AUC_SELL);
    return;
  }
  if (find_auction_by_seller(GET_IDNUM(ch)))
  {
    send_to_char("You can only have one auction at a time!\r\n", ch);
    return;
  }
  amount = atoi(argument);
  if (!(obj = generic_find_obj(ch, arg, FIND_OBJ_INV)))
  {
    sprintf(buf, "You don't have a %s.\r\n", arg);
    send_to_char(buf, ch);
    return;
  }
  if (OBJ_FLAGGED(obj, ITEM_QEQ))
  {
    send_to_char("You can't auction quest items!\r\n", ch);
    return;
  }
  if (obj->item_number == NOTHING) // Artus> Corpses, Mail, etc..
  {
    send_to_char("You can't auction that!\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_REWARD)
  {
    send_to_char("You can't auction rewards!\r\n", ch);
    return;
  }
  if (OBJ_FLAGGED(obj, ITEM_NODROP))
  {
    send_to_char("You can't auction something that is cursed!\r\n", ch);
    return;
  }
  if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && (obj->contains != NULL))
  {
    send_to_char("Empty it first!\r\n", ch);
    return;
  }
  if (OBJ_RIDDEN(obj))
  {
    send_to_char("Get off it first!\r\n", ch);
    return;
  }
  if (amount < (int)(GET_OBJ_COST(obj) / 2))
  {
    send_to_char("You can't just give it away!\r\n", ch);
    return;
  }
  for (struct auc_data *k = auc_list; k; k = k->next)
    auc_count++;
  if (auc_count >= AUC_LIMIT)
  {
    send_to_char("There are already too many auctions running, please ask someone to cancel one.\r\n", ch);
    return;
  }
  auction_create(ch, obj, amount);
}

// List auctions.
void auction_list(struct char_data *ch)
{
  struct char_data *seller;
  if (!(auc_list))
  {
    send_to_char("There is currently nothing up for auction.\r\n", ch);
    return;
  }
  send_to_char("&MCurrent Auctions:&n\r\n\r\n", ch);
  for (struct auc_data *lot = auc_list; lot; lot = lot->next)
  {
    sprintf(buf, "&mItem #&y%d&m: &5%s&m currently selling for &Y%ld\r\n"
	         "&m  Auctioned by &7%s&m; Current Bidder: &7%s&n\r\n",
	    lot->idnum, lot->obj->short_description, lot->offer,
	    ((seller = get_player_by_id(lot->sellerid)) ? (CAN_SEE(ch, seller) ? GET_NAME(seller) : "&rsomeone") : lot->sellername), lot->buyername);
    send_to_char(buf, ch);
  }
}

// Purge an auction.
void auction_purge(struct char_data *ch, char *argument)
{
  struct auc_data *lot;
  int idnum;

  if (LR_FAIL(ch, LVL_GRGOD))
  {
    show_auction_help(ch, AUC_NONE);
    return;
  }
  skip_spaces(&argument);
  if ((!*argument) || !is_number(argument))
  {
    show_auction_help(ch, AUC_PURGE);
    return;
  }
  idnum = atoi(argument);
  if (!(lot = find_auction_by_id(idnum)))
  {
    send_to_char("That auction does not exist.\r\n", ch);
    return;
  }
  auction_spam(lot, ch, AUC_PURGE);
  auction_mail_to(lot, lot->sellerid, AUC_PURGE);
  auction_mail_to(lot, lot->buyerid, AUC_PURGE);
  auction_delete(lot, AUC_PURGE);
}

// Sold the auctioned item.
void auction_sold(struct char_data *ch)
{
  struct auc_data *lot;

  if (!(lot = find_auction_by_seller(GET_IDNUM(ch))))
  {
    send_to_char("But you're not auctioning anything!\r\n", ch);
    return;
  }
  if (lot->buyerid < 1)
  {
    send_to_char("But noone has bid on it. You should use Auction Cancel.\r\n", ch);
    return;
  }
  auction_spam(lot, ch, AUC_SOLD);
  auction_mail_to(lot, lot->buyerid, AUC_SOLD);
  auction_delete(lot, AUC_SOLD);
}

// Stat an auction.
void auction_stat(struct char_data *ch, char *argument)
{
  void identify_obj_to_char(struct char_data *ch, struct obj_data *obj);
  struct auc_data *lot;
  int item;

  skip_spaces(&argument);
  if ((!*argument) || !is_number(argument))
  {
    show_auction_help(ch, AUC_STAT);
    return;
  }
  if (GET_GOLD(ch) < AUC_STAT_COST)
  {
    sprintf(buf, "Statting an auction costs &Y%d&n coins, which you do not have!&\r\n", AUC_STAT_COST);
    send_to_char(buf, ch);
    return;
  }
  item = atoi(argument);
  if (item < 1) 
  {
    show_auction_help(ch, AUC_STAT);
    return;
  }
  if (!(lot = find_auction_by_id(item)))
  {
    send_to_char("That auction is a figment of your imagination.\r\n", ch);
    return;
  }
  if (lot->sellerid == GET_IDNUM(ch))
  {
    send_to_char("Perhaps you should buy a scroll of identify :o)\r\n", ch);
    return;
  }
  if (!(lot->obj))
  {
    send_to_char("This auction is screwed up.. Perhaps you should ask an imm to purge it.\r\n", ch);
    sprintf(buf, "SYSERR: Auction item seems to have disappeared! (Item %d)",
	    lot->idnum);
    mudlog(buf, NRM, LVL_IMPL, TRUE);
    return;
  }
  if (!MORT_CAN_SEE_OBJ(ch, lot->obj))
  {
    send_to_char("You can't see it!\r\n", ch);
    return;
  }
  GET_GOLD(ch) -= AUC_STAT_COST;
  identify_obj_to_char(ch, lot->obj);
}

// Artus> This had to be rewritten.
ACMD(do_auction)
{
  subcmd = AUC_NONE;
  argument = one_argument(argument, arg);
  if (IS_NPC(ch))
  {
    send_to_char("Not in this lifetime!\r\n", ch);
    return;
  }
  if (PRF_FLAGGED(ch, PRF_NOAUCT))
  {
    send_to_char("You aren't even on that channel!\r\n", ch);
    return;
  }
  if (!*arg)
  {
    show_auction_help(ch, AUC_NONE);
    return;
  }
  if (is_abbrev(arg, "sell"))
    auction_sell(ch, argument);
  else if (is_abbrev(arg, "bid"))
    auction_bid(ch, argument);
  else if (is_abbrev(arg, "cancel"))
    auction_cancel(ch);
  else if (is_abbrev(arg, "list"))
    auction_list(ch);
  else if (is_abbrev(arg, "stat"))
    auction_stat(ch, argument);
  else if (is_abbrev(arg, "sold"))
    auction_sold(ch);
  else if (!LR_FAIL(ch, LVL_GRGOD) && is_abbrev(arg, "purge"))
    auction_purge(ch, argument);
  else 
    show_auction_help(ch, AUC_NONE);
  return;
}

//////////////////////////////////////
// Class definition for GossipItem
void GossipItem::getGossip(struct char_data *forChar, char *gossip)
{
   bool bSomeone = FALSE;

   if (nInvisSpecific != 0)	// gossiper was invis to id at time
   {
     if (GET_ID(forChar) == nInvisSpecific)
	bSomeone = TRUE;
   }
  
   if (nInvisLevel != 0)
   {
     if (GET_LEVEL(forChar) < nInvisLevel)
	bSomeone = TRUE;
   }

   if (nInvisSingle != 0)
   {
	if (GET_LEVEL(forChar) == nInvisSingle)
	   bSomeone = TRUE;
   }

   if (bStandardInvis == TRUE && !IS_AFFECTED(forChar, AFF_DETECT_INVIS))
	bSomeone = TRUE;

   // See if there's no gossip yet
   if (strcmp(cGossip, "") == 0 ||
       strcmp(cGossip, " ") == 0 ||
       strcmp(cGossip, "\0") == 0)
   {
        strcpy(gossip, "");
	return;
   }
   // Put together the gossip message
   if (bSomeone)
      strcpy(gossip, "Someone");
   else
      strcpy(gossip, cGossiper);

   strcat(gossip, " gossiped, '");
   strcat(gossip, cGossip);
   strcat(gossip, "'\r\n\0");

}

void GossipItem::setGossip(struct char_data *ch, char *gossip)
{
    // Reset the variables
    nInvisSingle = nInvisLevel = nInvisSpecific = 0;
    bStandardInvis = FALSE;

    if (GET_INVIS_TYPE(ch) == INVIS_SPECIFIC)
	nInvisSpecific = GET_INVIS_LEV(ch);
    if (GET_INVIS_TYPE(ch) == INVIS_SINGLE)
	nInvisSingle = GET_INVIS_LEV(ch);
    if (GET_INVIS_TYPE(ch) == INVIS_NORMAL)
	nInvisLevel = GET_INVIS_LEV(ch);

    if (IS_AFFECTED(ch, AFF_INVISIBLE))
	bStandardInvis = TRUE;

    strcpy(cGossip, gossip);
    strcpy(cGossiper, GET_NAME(ch));
}

void GossipItem::copyGossip(GossipItem target)
{
	nInvisSpecific = target.nInvisSpecific;
	nInvisSingle = target.nInvisSingle;
	nInvisLevel = target.nInvisLevel;
	bStandardInvis = target.bStandardInvis;

	strcpy(cGossip, target.cGossip);
	strcpy(cGossiper, target.cGossiper);
}

////////  end GossipItem ////////////
