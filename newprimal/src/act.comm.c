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
#include "improved-edit.h" 

/* extern variables */
extern int level_can_shout;
extern int holler_move_cost;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct auction_lot avail_lots[MAX_LOTS];
extern int lot_tot;   

/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);

/* modified do_say to affect speech when drunk.. - Vader */
#define DRUNKNESS    ch->player_specials->saved.conditions[DRUNK]
#define PISS_FACTOR  (25 - DRUNKNESS) * 50
#define BASE_SECT(n) ((n) & 0x000f)  

ACMD(do_say)
{
  char *speech;
  int i,j;

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
	
     if(!IS_NPC(ch) && ((BASE_SECT(world[ch->in_room].sector_type) == SECT_UNDERWATER))){
        send_to_char("Bubbles raise from your mouth as you speak!\r\n",ch);
        act("Bubbles raise from $n's mouth as $e speaks.", TRUE, ch, 0, 0, TO_ROOM);
      }

    sprintf(buf, "&n$n says, '%s&n'", speech); 
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      delete_doubledollar(argument);
      sprintf(buf, "&nYou say, '%s&n'", speech);  
      send_to_char(buf, ch);
    }
  }
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
      sprintf(buf, "&rYou tell the group, '%s&r'&n", argument);   
      send_to_char(buf, ch);
    }
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  sprintf(buf, "&R$n &rtells you, '%s&r'&n", arg);
  act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    sprintf(buf, "&rYou tell &R$N&r, '%s&r'&n", arg);
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
  else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
    send_to_char("The walls seem to absorb your words.\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF))
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

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char("Who do you wish to tell what??\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
    send_to_char(NOPERSON, ch);
  else if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PRF_FLAGGED(vict, PRF_NOTELL))
        act("$E's deaf to your tells; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PRF_FLAGGED(vict, PRF_AFK))
        act("$E's is marked (AFK); try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ( ( GET_IGN1(vict) == GET_IDNUM(ch) ||
             GET_IGN2(vict) == GET_IDNUM(ch) ||
             GET_IGN3(vict) == GET_IDNUM(ch) ||
             GET_IGN_LEVEL(vict) >= GET_LEVEL(ch)) &&
             !IS_NPC(ch) )
            act("$E's ignoring you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP );
  else if (PRF_FLAGGED(ch, PRF_AFK))
    send_to_char("It is unfair to tell people things while marked AFK.  They can't reply!!.\r\n", ch);
  else
    perform_tell(ch, vict, buf2);
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
  } else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
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

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", penname);
      send_to_char(buf, ch);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
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
    char *backstr = NULL;

    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
      backstr = str_dup(paper->action_description);
      send_to_char("There's something written on it already:\r\n", ch);
      send_to_char(paper->action_description, ch);
    }
  
    /* we can write - hooray! */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
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
      if (GET_LEVEL(ch) > LVL_GOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char("You will never be godly enough to do that!\r\n", ch);
      return;
    }
    if ((vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char("There is no such person in the game!\r\n", ch);
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  char s[800];/* zap */       
  char *speech;
  int l,j; 

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    EXT_NONEWBIE,
    EXT_NOCTALK, 
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][4] = {
    {"You cannot holler!!\r\n",
      "holler",
      "",
    KYEL},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
    KYEL},

    {"You cannot gossip!!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n",
    KYEL},

    {"You cannot auction!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n",
    KMAG},

    {"You cannot congratulate!\r\n",
      "congrat",
      "You aren't even on the channel!\r\n",
    KGRN},

    {"You cannot use the newbie channel!\r\n",
      "newbie",
      "You aren't even on the channel!\r\n",
    KCYN},
 
    {"You cannot use clan talk!\r\n",
      "clan talk",
      "You aren't even on the channel!\r\n",
    KWHT}
  };

  /* changed so mobs can shout and holler in spec procs - Vader */
  if (!ch->desc && subcmd != SCMD_SHOUT && subcmd != SCMD_HOLLER)
    return;  

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(com_msgs[subcmd][0], ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  /* level_can_shout defined in config.c */
  if ((subcmd != SCMD_NEWBIE) && GET_LEVEL(ch) < level_can_shout) {
    sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
	    level_can_shout, com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  /* make sure the char is on the channel */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, channels[subcmd]) && subcmd <= SCMD_GRATZ ) {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
	    com_msgs[subcmd][1], com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < holler_move_cost) {
      send_to_char("You're too exhausted to holler.\r\n", ch);
      return;
    } else
      GET_MOVE(ch) -= holler_move_cost;
  }

  /* check that there not in a clan if using clan talk */
  if ( subcmd == SCMD_CTALK && GET_CLAN_NUM(ch) == -1 )
  {
   send_to_char("Your not even in a clan.\n\r" , ch);
   return;
  }

  if (scan_buffer_for_xword(argument))
  {
    sprintf(s,"WATCHLOG SWEAR: %s %ss, '%s'", ch->player.name,com_msgs[subcmd][1], argument);
    mudlog(s,NRM,LVL_IMPL,TRUE);
    send_to_char("&nPlease dont swear on the open channels.\r\n",ch);
    return ;
  }

    speech = str_dup(argument);
/* this loop goes thru and randomly drops letters from what was said
 * depanding on how drunk the person is.. 24 is max drunk.. when someone
 * is 24 drunk they pretty much cant be understood.. - Vader
 */
    for(l = 0,j = 0; l < strlen(speech); l++) {
      if(number(1,PISS_FACTOR) > DRUNKNESS) {
        speech[j] = speech[l];
        j++;
        }
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

 // ???
  sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], argument);

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
       ((!PRF_FLAGGED(i->character, channels[subcmd]) && subcmd <= SCMD_GRATZ )||
       (!EXT_FLAGGED(i->character, channels[subcmd]) && subcmd >= SCMD_NEWBIE ))&&
        !PLR_FLAGGED(i->character, PLR_WRITING) &&
        !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

//	!PRF_FLAGGED(i->character, channels[subcmd]) &&
//	!PLR_FLAGGED(i->character, PLR_WRITING) &&
//	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	   !AWAKE(i->character)))
	continue;

      /* stops others from hearing the clan channels */
       if (subcmd == SCMD_CTALK && (GET_LEVEL(ch) < LVL_GOD) && (GET_CLAN_NUM(i->character) != GET_CLAN_NUM(ch)))
        continue;

     if (COLOR_LEV(i->character) >= C_NRM)
        sprintf(buf, "%s$n %ss, '%s&n%s'&n", color_on, com_msgs[subcmd][1], speech, color_on);
      else
        sprintf(buf, "$n %ss, '%s'&n", com_msgs[subcmd][1], speech);

      act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);

//      if (COLOR_LEV(i->character) >= C_NRM)
//	send_to_char(color_on, i->character);
//     if (COLOR_LEV(i->character) >= C_NRM)
//	send_to_char(KNRM, i->character);
    }
  }
}


ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char("You aren't even part of the quest!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
	    CMD_NAME);
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      if (subcmd == SCMD_QSAY)
        sprintf(buf, "You quest-say, '%s&n'", argument); 
      else
	strcpy(buf, argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    if (subcmd == SCMD_QSAY)
      sprintf(buf, "&C$n&n quest-says, '%s&n'", argument); 
    else
      strcpy(buf, argument);

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc &&
	  PRF_FLAGGED(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}

int scan_buffer_for_xword(char* buf)
{
  char tmpword[MAX_INPUT_LENGTH+65];
  int i,count=-1;
  for (i=0;i<=strlen(buf);i++)
  {
    count++;
    tmpword[count]=buf[i];
    if (tmpword[count]==' ' || tmpword[i]=='\n' || i==strlen(buf))
    {
      tmpword[count]='\0';
      count=-1;
      if (!Valid_Name(tmpword))
        return 1;
    }
  }
  return 0;
} 

void do_auc_stat(struct char_data *ch, int lot)
{
  struct obj_data *obj;
  char buyer[30];
  long offer;
 
  extern char *spells[];
 
  offer = avail_lots[lot].offer;
  obj = avail_lots[lot].obj;
 
  if ( avail_lots[lot].buyer == NULL )
          strcpy ( buyer, "no one");
  else
          strcpy ( buyer, avail_lots[lot].buyer->player.name);
 
  if (COLOR_LEV(ch) >= C_NRM)
                        send_to_char(KMAG, ch);
 
  sprintf(buf, "\r\nItem %d\r\n", lot+1);
  sprintf(buf, "%sSeller: %s\tBuyer: %s\tPrice: %ld\r\n", buf,avail_lots[lot].seller->player.name, buyer, offer);
  send_to_char ( buf,ch);
 
  sprintf(buf, "Object '%s', Item type: ", obj->short_description);
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
 
    if (obj->obj_flags.bitvector) {
      send_to_char("Item will give you following abilities:  ", ch);
      sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
      strcat(buf, "\r\n");  
     send_to_char(buf, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
 
    sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
            GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
    send_to_char(buf, ch);
 
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
 
      if (GET_OBJ_VAL(obj, 1) >= 1)
        sprintf(buf, "%s %s", buf, skill_name(GET_OBJ_VAL(obj, 1)));
      if (GET_OBJ_VAL(obj, 2) >= 1)
        sprintf(buf, "%s %s", buf, skill_name(GET_OBJ_VAL(obj, 2)));
      if (GET_OBJ_VAL(obj, 3) >= 1)
        sprintf(buf, "%s %s", buf, skill_name(GET_OBJ_VAL(obj, 3)));
      sprintf(buf, "%s\r\n", buf);
      send_to_char(buf, ch);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
      sprintf(buf, "%s %s\r\n", buf, skill_name(GET_OBJ_VAL(obj, 3)));
      sprintf(buf, "%sIt has %d maximum charge%s and %d remaining.\r\n", buf,
              GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
              GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_WEAPON:
      sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
              GET_OBJ_VAL(obj, 2));
      sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
              (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
     send_to_char(buf, ch);  
     break;
    case ITEM_ARMOR:
      sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      break;
    }
 
  if (COLOR_LEV(ch) >= C_NRM)
    send_to_char(KNRM, ch);
 
}     

ACMD(do_auction)
{
  int lot;
  struct descriptor_data *i;
  long amount, offer;
  struct obj_data *obj;
  struct char_data *buyer;
 
  argument = one_argument(argument, arg);
 
  if (!*arg) {
    show_lots(ch);
    return;
  }

// auction <item number> <bid amount> 
  if (is_number(arg)) {
    lot = atoi(arg);
 
    if (lot > MAX_LOTS) {
      send_to_char("That is an invalid item.\r\n",ch);
      return;
    } 
 
    if (lot > lot_tot || lot > MAX_LOTS || lot < 0) {
      send_to_char("That is an invalid item.\r\n",ch);
      return;
    }

    lot--;

    if (avail_lots[lot].seller == ch) {
      send_to_char("You can't bid on your own item.\r\n",ch);
      return; 
    }
 
    argument = one_argument(argument, arg);
 
    if (is_number(arg))
      offer = atoi(arg);
    else {
      send_to_char("You need to make an offer.\r\n",ch);
      return;
    }
 
    if (offer <= avail_lots[lot].offer) {
      sprintf(buf, "Your bid needs to be higher then the current bid of %ld.\r\n", avail_lots[lot].offer);
      send_to_char(buf,ch);
      return;
    }
 
    if (offer > GET_GOLD(ch)) {
      sprintf(buf,"Your need more gold!\r\n");
      send_to_char(buf,ch);
      return;
    }
 
    avail_lots[lot].offer = offer;
    avail_lots[lot].buyer = ch;

    subcmd = SCMD_BID;

// auction STAT <item number>
  } else if (!strcmp("stat", arg)) {
    argument = one_argument(argument, arg);
 
    if (is_number(arg))
      lot = atoi(arg);
    else {
      send_to_char("Need a item number.\r\n",ch);
      return;
    }
 
    if (lot > MAX_LOTS || lot > lot_tot || lot < 1) {
      send_to_char("That is an invalid item number.\r\n",ch);
      return;
    }
 
    if (ch == avail_lots[lot-1].seller) {
      send_to_char("You cant see the stats on your own item. Go buy a an identify scroll, you cheap bastard!\r\n", ch );
      return;
    }
 
    do_auc_stat(ch, lot-1);
    return;

// auction CANCEL
  } else if (!strcmp("cancel", arg)) {
    lot = check_seller(ch);
 
    if (lot > -1) {
      remove_lot(lot);
      obj_to_char(avail_lots[lot].obj, ch);
      subcmd = SCMD_CANCEL;
    } else {
      send_to_char("Your not selling anything!\r\n", ch);
      return;
    }

// auction SOLD
  } else if (!str_cmp("sold", arg)) {
    lot = check_seller(ch);  
 
    if (lot > -1) {
      if (!(buyer = avail_lots[lot].buyer)) {
        send_to_char("No-one has offered you a price.\r\n",ch);
        return;
      };

      offer = avail_lots[lot].offer;
      obj = avail_lots[lot].obj;
 
      if (GET_GOLD(buyer) < offer) {
        subcmd = SCMD_WELCH;
      } else {
        obj_to_char(obj, buyer);
        GET_GOLD(buyer) -= offer;
        GET_GOLD(ch) += offer;
        remove_lot(lot);
        subcmd = SCMD_SOLD;
      } 
    } else {
      send_to_char("Your not selling anything!\r\n", ch);
      return;
    }

// auction <item name> <item price>
  } else {
    if (check_seller(ch) != -1 && lot_tot != 0) {
      send_to_char("One item per person.\r\n",ch);
      return;
    } 
 
    if (lot_tot >= MAX_LOTS) {
      sprintf(buf, "Only %d lots at once.\r\n", MAX_LOTS);
      send_to_char ( buf,ch);
      return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
      sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      send_to_char(buf, ch);
      return;
    } else {
      argument = one_argument(argument, arg);
      if (is_number(arg))
        amount = atoi(arg);
      else {
        send_to_char("A starting price is needed.\r\n",ch);
        return;
      }
    }
 
    if (amount <= 0) {
      send_to_char ( "The price has to be positive.\r\n",ch);
      return;
    }
 
    if (amount > MAX_PRICE) {
      send_to_char("Be reasonable!\r\n",ch);
      return;
    }
 
    obj_from_char(obj);
    lot_tot = add_lot(obj, ch, amount);
    subcmd = SCMD_NEW;
  }
 
  switch(subcmd) {
    case SCMD_NEW:
      sprintf(buf, "[Auction] $n: %ld for a %s.",  amount, obj->short_description );
      break;

    case SCMD_CANCEL:
      sprintf(buf, "[Auction] $n: The sale of %s has been canceled.",  avail_lots[lot].obj->short_description);
      break;

    case SCMD_BID:
      sprintf ( buf,"[Auction] $n: %ld for item %d: %s.", offer, lot+1, avail_lots[lot].obj->short_description);
      break;

    case SCMD_SOLD:
      sprintf ( buf,"[Auction] $n: Item %d: %s sold to %s for %ld.",  lot+1, obj->short_description, buyer->player.name, offer);
      break;

    case SCMD_WELCH:
      sprintf ( buf,"[Auction] $n: %s welched on the deal.", buyer->player.name );
      break;
  }
 
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    if (COLOR_LEV(ch) >= C_NRM)
      send_to_char(KMAG, ch);
 
    act(buf, FALSE, ch, 0, ch, TO_VICT | TO_SLEEP);
 
    if (COLOR_LEV(ch) >= C_NRM)
      send_to_char(KNRM, ch);
  }
 
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
        !PRF_FLAGGED(i->character, PRF_NOAUCT) &&   
        !PLR_FLAGGED(i->character, PLR_WRITING) &&
        !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF))
    {
      if (COLOR_LEV(i->character) >= C_NRM)
        send_to_char(KMAG, i->character);

      act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);

      if (COLOR_LEV(i->character) >= C_NRM)
        send_to_char(KNRM, i->character);
    }
  }
}           
