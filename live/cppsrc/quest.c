/* Artus> New File. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "house.h"
#include "quest.h"

// External Vars.
extern struct index_data *obj_index;	/* object index			 */
extern struct obj_data *obj_proto;	/* object prototypes		 */
extern obj_rnum top_of_objt;		/* top of object index table	 */
extern struct char_data *mob_proto;	/* mobile prototypes		 */
extern mob_rnum top_of_mobt;		/* top of mobile index table	 */
extern struct index_data *mob_index;	/* mobile index			 */
extern struct room_data *world;         /* room table			 */
extern struct zone_data *zone_table;	/* zone table			 */
extern struct event_list events;	/* events list			 */
extern struct descriptor_data *descriptor_list;      /* master desc list */
extern struct char_data *character_list;  /* global linked list of chars */ 
extern const char *class_abbrevs[];	/* Cl, Ma, Wa, Th, etc..	 */
extern const char *quest_names[];	/* Item Hunt, etc..		 */
extern const char *enhancement_names[]; /* Enhancement Names		 */
extern struct house_control_rec house_control[MAX_HOUSES];
extern int num_of_houses;
extern struct questlist_element *questlist_table;
SPECIAL(shop_keeper);			/* Shopkeeper Specproc		 */

// External Functions.
void add_mud_event(struct event_data *ev);
void update_questlist(struct char_data *vict, int qitemno);
void add_to_questlist(char *name, long idnum, struct quest_obj_data *item);

// Local Functions.
ACMD(do_qcomm);

//////////////////////////////////////////////////////////The Source

// Artus> Find a quest event.
struct event_data *find_quest_event()
{
  struct event_data *temp;
  for (temp = events.list; temp; temp = temp->next)
    if (temp->type == EVENT_QUEST)
      return temp;
  return NULL;
}

// Helper function for is_quest_participant
struct quest_score_data *is_idnum_in_qlist(int idnum, 
                                           struct quest_score_data *qs)
{
  struct quest_score_data *i;
  for (i=qs; i; i=i->next)
    if (idnum == i->chID)
      return i;
  return NULL;
}

// Get Quest Scores.
struct quest_score_data *get_quest_scores(struct event_data *ev)
{
  if (!ev)
    return NULL;
  if (ev->info1 == QUEST_ITEM_HUNT)
  {
    struct itemhunt_data *qevih = (struct itemhunt_data *)ev->info2;
    if (!(qevih) || (qevih->ev != ev))
      return NULL;
    return qevih->players;
  }
  if (ev->info2 == QUEST_TRIVIA)
  {
    struct trivia_data *qevtr = (struct trivia_data *)ev->info2;
    if (!(qevtr) || (qevtr->ev != ev))
      return NULL;
    return qevtr->players;
  }
  return NULL;
}

// Artus> Send a message to everyone with a quest flag.
void send_to_questors(char *buf, struct event_data *ev)
{
  struct descriptor_data *i;
  struct quest_score_data *qs=NULL;
  if (ev)
    qs = get_quest_scores(ev);

  for (i = descriptor_list; i; i = i->next)
  {
    if (!(i->character) || IS_NPC(i->character) ||
	!PRF_FLAGGED(i->character, PRF_QUEST))
      continue;
    if (qs && LR_FAIL(i->character, LVL_GOD))
    {
      if (!is_idnum_in_qlist(GET_IDNUM(i->character), qs))
      {
	REMOVE_BIT(PRF_FLAGS(i->character), PRF_QUEST);
	continue;
      }
    }
    send_to_char(buf, i->character);
  }
}

// Is this person a participant.
struct quest_score_data *is_quest_participant(struct char_data *ch, 
                                              struct event_data *ev)
{
  struct quest_score_data *qs;

  if ((ch == NULL) || (ev == NULL) || (ev->info1 < 1))
    return NULL;
 
  qs = get_quest_scores(ev);

  if (!qs)
    return NULL;

  return (is_idnum_in_qlist(GET_IDNUM(ch), qs));
}

// Artus> Free Single Trivia Response.
void free_trivia_response(struct trivia_data *qevtr)
{
  struct trivia_response_data *goner;
  goner = qevtr->resp;
  qevtr->resp = goner->next;
  free(goner);
}

// Artus> Free Remaining Trivia Responses.
void free_remaining_responses(struct trivia_data *qevtr)
{
  while (qevtr->resp)
    free_trivia_response(qevtr);
}

// Artus> Close a question.
void perform_quest_trclose(struct char_data *ch, struct event_data *ev,
                           char *arg)
{
  struct trivia_data *qevtr;

  // Sanity.
  if (!ch->desc)
    return;
  qevtr = (struct trivia_data *)ev->info2;
  if (!(qevtr) || (qevtr->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  if (qevtr->trivmaster != GET_IDNUM(ch))
  {
    send_to_char("Only the trivia master may close the question.\r\n", ch);
    return;
  }
  if (qevtr->resp)
  {
    send_to_char("There are still unchecked responses.\r\n", ch);
    return;
  }
  if (!*qevtr->question)
  {
    send_to_char("There is no question to close!\r\n", ch);
    return;
  }
  if (arg)
    skip_spaces(&arg);
  if (!*arg)
  {
    send_to_char("Please give the correct answer to the plebs.\r\n", ch);
    return;
  }
  // Do the work.
  sprintf(buf, "&YNoone has gotten the correct answer.\r\n"
               "Answer: &0%s&Y.&n\r\n", arg);
  send_to_questors(buf, ev);
  strcpy(qevtr->question, "");
  return;
}

// Artus> Reject an answer.
void perform_quest_trfail(struct char_data *ch, struct event_data *ev)
{
  struct trivia_data *qevtr;

  // Sanity.
  if (!ch->desc)
    return;
  qevtr = (struct trivia_data *)ev->info2;
  if (!(qevtr) || (qevtr->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  if (qevtr->trivmaster != GET_IDNUM(ch))
  {
    send_to_char("Only the trivia master may reject responses.\r\n", ch);
    return;
  }
  if (!qevtr->resp)
  {
    send_to_char("There is no response to reject!\r\n", ch);
    return;
  }
  // Do the work.
  sprintf(buf, "&7%s&Y has incorrectly answered '&0%s&y'.\r\n",
          qevtr->resp->player->name, qevtr->resp->response);
  send_to_questors(buf, ev);
  free_trivia_response(qevtr);
}

// Artus> Approve of an answer.
void perform_quest_trpass(struct char_data *ch, struct event_data *ev)
{
  struct trivia_data *qevtr;

  // Sanity.
  if (!ch->desc)
    return;
  qevtr = (struct trivia_data *)ev->info2;
  if (!(qevtr) || (qevtr->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  if (qevtr->trivmaster != GET_IDNUM(ch))
  {
    send_to_char("Only the trivia master may accept responses.\r\n", ch);
    return;
  }
  if (!(qevtr->resp))
  {
    send_to_char("There is currently no response to accept.\r\n", ch);
    return;
  }
  qevtr->resp->player->score++;
  sprintf(buf, "&7%s&Y has correctly answered '&0%s&Y', for a total of &0%d&n.\r\n", 
          qevtr->resp->player->name, qevtr->resp->response,
	  qevtr->resp->player->score);
  send_to_questors(buf, ev);
  free_remaining_responses(qevtr);
  strcpy(qevtr->question, "");
  return;
}

// Artus> Show the most recent response to the questmaster.
void perform_quest_trresponse(struct char_data *ch, struct event_data *ev)
{
  struct trivia_data *qevtr;

  // Sanity.
  if (!ch->desc)
    return;
  qevtr = (struct trivia_data *)ev->info2;
  if (!(qevtr) || (qevtr->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  if (LR_FAIL(ch, LVL_ANGEL))
  {
    send_to_char("You are not holy enough to view trivia responses.\r\n", ch);
    return;
  }
  if (!(qevtr->resp))
  {
    send_to_char("There is not currently an answer to view.\r\n", ch);
    return;
  }
  sprintf(buf, "&7%s&0 Answers:&n %s\r\n", qevtr->resp->player->name,
          qevtr->resp->response);
  send_to_char(buf, ch);
}

// Artus> Ask a trivia question.
void perform_quest_trask(struct char_data *ch, struct event_data *ev, 
                         char *arg)
{
  struct trivia_data *qevtr;

  // Sanity.
  if (!ch->desc)
    return;
  qevtr = (struct trivia_data *)ev->info2;
  if (!(qevtr) || (qevtr->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  if (*qevtr->question)
  {
    send_to_char("Please close the open question, first.\r\n", ch);
    return;
  }
  if (qevtr->trivmaster != GET_IDNUM(ch))
  {
    send_to_char("Only the trivia master may set questions.\r\n", ch);
    return;
  }
  if (arg)
    skip_spaces(&arg);
  if (!*arg)
  {
    send_to_char("You must specify a question!\r\n", ch);
    return;
  }
  // Handle it.
  if (!PRF_FLAGGED(ch, PRF_QUEST))
    SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
  strncpy(qevtr->question, arg, MAX_INPUT_LENGTH);
  qevtr->qcount++;
  sprintf(buf, "&YQ%d) %s\r\n", qevtr->qcount, qevtr->question);
  send_to_questors(buf, ev);
}

// Artus> Display the current trivia question.
void perform_quest_trquestion(struct char_data *ch, struct event_data *ev)
{
  struct trivia_data *qevtr;
 
  // Sanity.
  if (!ch->desc) 
    return;
  qevtr = (struct trivia_data *)ev->info2;
  if (!(qevtr) || (qevtr->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  if (!*qevtr->question)
  {
    send_to_char("Please wait for the question to be asked.\r\n", ch);
    return;
  }
  // Output.
  sprintf(buf, "Question &0%d&n: &0%s&n\r\n", qevtr->qcount, qevtr->question);
  send_to_char(buf, ch);
  return;
}

// Artus> Answer a trivia question.
void perform_quest_transwer(struct char_data *ch, struct event_data *ev, 
                            char *arg)
{
  struct trivia_data *qevtr;
  struct trivia_response_data *resp, *lastresp = NULL, *i;
  struct quest_score_data *qs;

  if (!ch->desc)
    return;

  qevtr = (struct trivia_data *)ev->info2;
  if (!(qevtr) || (qevtr->ev != ev) || !(qevtr->players))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  qs = is_idnum_in_qlist(GET_IDNUM(ch), qevtr->players);
  if (qs == NULL)
  {
    send_to_char("You don't seem to be part of this trivia quest.\r\n", ch);
    return;
  }
  if (!*qevtr->question)
  {
    send_to_char("Please wait for the question to be asked!\r\n", ch);
    return;
  }
  if (arg)
    skip_spaces(&arg);
  if (!*arg)
  {
    send_to_char("You must supply an answer.\r\n", ch);
    return;
  }
  // Do the work.. Make sure they haven't responded already.
  for (i = qevtr->resp; i; i = i->next)
  {
    if (i->player->chID == GET_IDNUM(ch))
    {
      send_to_char("You have already submitted a response!\r\n", ch);
      return;
    }
    if (!i->next)
      lastresp = i;
  }
  CREATE(resp, struct trivia_response_data, 1);
  memset(resp, 0, sizeof(struct trivia_response_data));
  resp->player = qs;
  strncpy(resp->response, arg, MAX_INPUT_LENGTH);
  resp->next = NULL;
  if (!lastresp)
    qevtr->resp = resp;
  else 
    lastresp->next = resp;
  do_qcomm(ch, "$n &Yhas submitted an answer!", CMD_NONE, SCMD_QECHO);
  return;
}

// Artus> Let the quest players know that another mob/obj has been loaded.
void quest_load_notify(struct char_data *mob, obj_rnum objr)
{
  if (mob == NULL)
  {
    if (objr == NOTHING)
      return;
    sprintf(buf, "&YYou hear rumors of &5%s&Y lying around.&n\r\n",
	    obj_proto[objr].short_description);
    send_to_questors(buf, find_quest_event());
    return;
  }
  if (objr == NOTHING)
  {
    sprintf(buf, "&YYou hear rumors of &6%s&Y lurking around.&n\r\n",
	    GET_NAME(mob));
    send_to_questors(buf, find_quest_event());
    return;
  }
  sprintf(buf, "&YYou hear rumors of &6%s&Y carrying a &5%s&Y.&n\r\n",
          GET_NAME(mob), obj_proto[objr].short_description);
  send_to_questors(buf, find_quest_event());
  return;
}

#define BAG_VNUM	1110
// Load a mob/obj/mob+obj into the quest.
bool handle_quest_load(struct char_data *ch, mob_rnum mobr, obj_rnum objr, 
                      room_rnum room, bool notify)
{
  struct obj_data *obj = NULL, *bag = NULL;
  struct char_data *mob = NULL;
  obj_rnum bagrnum = NOTHING;
  // Sanity.
  if ((room <= NOWHERE) || ((mobr <= NOBODY) && (objr <= NOTHING)))
    return false;
  if (room > top_of_world)
  {
    sprintf(buf, "SYSERR: handle_quest_load() called with invalid room rnum %d.", room);
    mudlog(buf, NRM, LVL_IMPL, TRUE);
    return false;
  }
  if ((mobr > NOBODY) && (mobr > top_of_mobt))
  {
    sprintf(buf, "SYSERR: handle_quest_load() called with invalid mob rnum %d.",
	    mobr);
    mudlog(buf, NRM, LVL_IMPL, TRUE);
    return false;
  }
  if (objr > NOTHING)
  {
    if (objr > top_of_objt)
    {
      sprintf(buf, "SYSERR: handle_quest_load() called with invalid obj rnum %d.",
	      objr);
      mudlog(buf, NRM, LVL_IMPL, TRUE);
      return false;
    }
    bagrnum = real_object(BAG_VNUM);
    if ((bagrnum == NOTHING) || 
	(GET_OBJ_TYPE(&obj_proto[bagrnum]) != ITEM_CONTAINER))
    {
      sprintf(buf, "SYSERR: BAG_VNUM (%d) invalid, or not a container.\r\n",
	      BAG_VNUM);
      mudlog(buf, NRM, LVL_IMPL, TRUE);
      return false;
    }
  }
  // We'll load the mob first.
  if (mobr > NOBODY)
  {
    mob = read_mobile(mobr, REAL);
    if (!(mob))
      return false;
    SET_BIT(MOB_FLAGS(mob), MOB_STAY_ZONE | MOB_QUEST | MOB_NOSUMMON);
    REMOVE_BIT(MOB_FLAGS(mob), MOB_AGGRESSIVE | MOB_SENTINEL | MOB_AGGR_GOOD |
			       MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL);
    SET_BIT(AFF_FLAGS(mob), AFF_NOTRACK);
    char_to_room(mob, room);
    if (objr <= NOTHING) // We're done.
    {
      if (ch)
      {
	sprintf(buf, "You create a &6%s&n for the questors.\r\n",GET_NAME(mob));
	send_to_char(buf, ch);
      }
      if (notify)
	quest_load_notify(mob, objr);
      return true;
    }
  }
  bag = read_object(bagrnum, REAL);
  if (!(bag))
  {
    if (mob)
      extract_char(mob);
    return false;
  }
  obj = read_object(objr, REAL);
  if (!(obj))
  {
    if (mob)
      extract_char(mob);
    extract_obj(bag);
    return false;
  }
  GET_OBJ_TYPE(obj) = ITEM_QUEST;
  switch (number(0, 20))
  {
    case 0:  GET_OBJ_VAL(obj, 0) = 3; break;
    case 1: 
    case 2:  GET_OBJ_VAL(obj, 0) = 2; break; 
    case 3:
    case 4:
    case 5:  GET_OBJ_VAL(obj, 0) = 0; break;
    default: GET_OBJ_VAL(obj, 0) = 1; break;
  }
  obj_to_obj(obj, bag);
  if (mob)
    obj_to_char(bag, mob, __FILE__, __LINE__);
  else
    obj_to_room(bag, room);
  if (ch)
  {
    sprintf(buf, "You create &5%s&n for the questors.\r\n",
	    obj->short_description);
    send_to_char(buf, ch);
  }
  if (notify)
    quest_load_notify(mob, objr);
  return true;
}

// Artus> Someone is getting a quest item.
int handle_quest_item_take(struct char_data *ch, struct obj_data *obj)
{
  struct event_data *ev;
  struct itemhunt_data *qevih;
  struct quest_score_data *player;
  int i;

  if (IS_NPC(ch))
    return (0);
  if (!PRF_FLAGGED(ch, PRF_QUEST) && LR_FAIL(ch, LVL_GOD))
  {
    act("$p: you are not part of the quest!", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  ev = find_quest_event();
  if (!(ev) || (ev->type != EVENT_QUEST) || (ev->info1 != QUEST_ITEM_HUNT))
  {
    act("$p fades away like the illusion it was.", FALSE, ch, obj, 0, TO_CHAR);
    extract_obj(obj);
    return (0);
  }
  qevih = (struct itemhunt_data *)ev->info2;
  if (!(qevih) || (qevih->ev != ev))
  {
    act("$p fades away like the illusion it was.", FALSE, ch, obj, 0, TO_CHAR);
    extract_obj(obj);
    return (0);
  }
  player = is_idnum_in_qlist(GET_IDNUM(ch), qevih->players);
  if (player)
  {
    i = GET_OBJ_VAL(obj, 0);
    player->score += i;
    if (i == 0)
      strcpy(buf, "$n &Yhas picked up a herring item!");
    else if (i == 1)
      sprintf(buf, "$n &Yhas picked up an item worth &gone&Y point, for a total of &g%d&Y.", player->score);
    else
      sprintf(buf, "$n &Yhas picked up an item worth &g%d&Y points, for a total of &g%d&Y.", i, player->score);
    do_qcomm(ch, buf, CMD_NONE, SCMD_QECHO);
    extract_obj(obj);
  } else {
    if (!LR_FAIL(ch, LVL_GOD))
    {
      act("&rWarning:&n $p is a quest item.", FALSE, ch, obj, 0, TO_CHAR);
      return (1);
    }
    act("$p: you are not part of the quest!", FALSE, ch, obj, 0, TO_CHAR);
  }
  return (0);
}

// Artus> Purge all quest objs.
void purge_quest_objs(void)
{
  struct obj_data *next_o;
  extern struct obj_data *object_list;
  for (struct obj_data *o = object_list; o; o = next_o)
  {
    next_o = o->next;
    if (GET_OBJ_TYPE(o) == ITEM_QUEST)
      extract_obj(o);
  }
}

// Artus> Purge all quest mobs.
void purge_quest_mobs(void)
{
  struct char_data *next_k;
  for (struct char_data *k = character_list; k; k = next_k)
  {
    next_k = k->next;
    if (IS_NPC(k) && MOB_FLAGGED(k, MOB_QUEST))
    {
      while (k->carrying)
	extract_obj(k->carrying);
      extract_char(k);
    }
  }
}

// Artus> A quest mob has died.
void handle_quest_mob_death(struct char_data *ch, struct char_data *killer)
{
  struct event_data *ev;
  struct itemhunt_data *qevih;

  ev = find_quest_event();
  if (!ev)
    return;
  
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      if ((qevih) && (qevih->ev == ev))
	qevih->mobsloaded--;
      return;
  }
}

// Artus> Remove a player from a quest.
bool remove_from_quest(struct event_data *ev, struct char_data *ch)
{
  struct quest_score_data *qs;
  struct itemhunt_data *qevih;
  struct trivia_data *qevtr;

  if (IS_NPC(ch) || (ev->type != EVENT_QUEST))
    return false;
  if (!PRF_FLAGGED(ch, PRF_QUEST))
    return false;
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      if (!(qevih) || (qevih->ev != ev))
	break;
      for (qs = qevih->players; qs; qs = qs->next)
	if (qs->chID == GET_IDNUM(ch))
	{
	  do_qcomm(ch, "$n &Yhas left the quest!", CMD_NONE, SCMD_QECHO);
	  break;
	}
      break;
    case QUEST_TRIVIA:
      qevtr = (struct trivia_data *)ev->info2;
      if (!(qevtr) || (qevtr->ev != ev))
	break;
      for (qs = qevtr->players; qs; qs = qs->next)
        if (qs->chID == GET_IDNUM(ch))
	{
	  do_qcomm(ch, "$n &Yhas left the quest!", CMD_NONE, SCMD_QECHO);
	  break;
	}
      break;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_QUEST);
  return true;
}

// Artus> Add a player to a quest.
bool add_to_quest(struct event_data *ev, struct char_data *ch)
{
  struct quest_score_data *qs;
  struct itemhunt_data *qevih; 
  struct trivia_data *qevtr;

  // No Mobs!
  if (IS_NPC(ch) || (ev->type != EVENT_QUEST))
    return false;
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      if (!(qevih) || (qevih->ev != ev))
	return false;
      for (qs = qevih->players; qs; qs = qs->next)
	if (qs->chID == GET_IDNUM(ch))
	{
	  if (PRF_FLAGGED(ch, PRF_QUEST))
	    return false;
	  SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
	  do_qcomm(ch, "$n&Y has rejoined the quest!&n", CMD_NONE, SCMD_QECHO);
	  return true;
	}
      CREATE(qs, struct quest_score_data, 1);
      memset(qs->name, 0, MAX_NAME_LENGTH+1);
      qs->chID = GET_IDNUM(ch);
      strncpy(qs->name, GET_NAME(ch), MAX_NAME_LENGTH);
      qs->score = 0;
      qs->next = qevih->players;
      qevih->players = qs;
      if (GET_LEVEL(ch) < qevih->lowlevel)
	qevih->lowlevel = GET_LEVEL(ch);
      SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
      do_qcomm(ch, "$n&Y has joined the quest!&n", CMD_NONE, SCMD_QECHO);
      return true;
    case QUEST_TRIVIA:
      qevtr = (struct trivia_data *)ev->info2;
      if (!(qevtr) || (qevtr->ev != ev))
	return false;
      for (qs = qevtr->players; qs; qs = qs->next)
	if (qs->chID == GET_IDNUM(ch))
	{
	  if (PRF_FLAGGED(ch, PRF_QUEST))
	    return false;
	  SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
	  do_qcomm(ch, "$n&Y has rejoined the quest!&n", CMD_NONE, SCMD_QECHO);
	  return true;
	}
      CREATE(qs, struct quest_score_data, 1);
      memset(qs, 0, sizeof(quest_score_data));
      qs->chID = GET_IDNUM(ch);
      strncpy(qs->name, GET_NAME(ch), MAX_NAME_LENGTH);
      qs->score = 0;
      qs->next = qevtr->players;
      qevtr->players = qs;
      SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
      do_qcomm(ch, "$n&Y has joined the quest!&n", CMD_NONE, SCMD_QECHO);
      return true;
  }
  return false;
}

// Artus> Quest Load.
void perform_quest_load(struct char_data *ch, struct event_data *ev, char *arg)
{
  struct itemhunt_data *qevih;
  mob_vnum mobvnum;
  mob_rnum mobrnum;

  skip_spaces(&arg);

  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      if (!(qevih) || (qevih->ev != ev))
      {
	send_to_char("This quest seems to be broken.\r\n", ch);
	return;
      }
      if ((!*arg) || !is_number(arg))
      {
	send_to_char("You must specify a mob vnum to load.\r\n", ch);
	return;
      }
      mobvnum = atoi(arg);
      mobrnum = real_mobile(mobvnum);
      if (mobrnum == NOBODY)
      {
	send_to_char("There is no such mobile!\r\n", ch);
	return;
      }
      if (obj_index[qevih->itemrnum].vnum != qevih->itemvnum)
	qevih->itemrnum = real_object(qevih->itemvnum);
      if (handle_quest_load(ch, mobrnum, qevih->itemrnum, IN_ROOM(ch), true))
	qevih->mobsloaded++;
      return;
    default:
      send_to_char("Quest Load cannot be used with this quest type.\r\n", ch);
      return;
  }
}

// Artus> Quest Inc/Dec.
void perform_quest_adjust(struct char_data *ch, struct event_data *ev, 
                          char *arg, int amt)
{
  struct quest_score_data *qs = NULL;
  struct trivia_data *qevtr;
  struct itemhunt_data *qevih;

  if (arg)
    skip_spaces(&arg);
  if (!(*arg))
  {
    send_to_char("Who's score did you want to adjust?\r\n", ch);
    return;
  }
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      if (!(qevih) || (qevih->ev != ev))
      {
	send_to_char("This quest seems to be broken.\r\n", ch);
	return;
      }
      for (qs = qevih->players; qs; qs = qs->next)
        if (is_abbrev(arg, qs->name))
	  break;
      break;
    case QUEST_TRIVIA:
      qevtr = (struct trivia_data *)ev->info2;
      if (!(qevtr) || (qevtr->ev != ev))
      {
	send_to_char("This quest seems to be broken.\r\n", ch);
	return;
      }
      if (GET_IDNUM(ch) != qevtr->trivmaster)
      {
	send_to_char("Only the trivia master may do that!\r\n", ch);
	return;
      }
      for (qs = qevtr->players; qs; qs = qs->next)
	if (is_abbrev(arg, qs->name))
	  break;
      break;
    default:
      send_to_char("This quest seems to be broken.\r\n", ch);
      return;
  }
  if (qs)
  {
    qs->score += amt;
    sprintf(buf, "Score set to &c%d&n for &7%s&n.\r\n", qs->score, qs->name);
    send_to_char(buf, ch);
    return;
  }
  send_to_char("They don't seem to be part of this quest.\r\n", ch);
}

// Artus> Quest End.
void perform_quest_end(struct char_data *ch, struct event_data *ev)
{
  extern const char *quest_names[];
  void remove_mud_event(struct event_data *ev);
  struct itemhunt_data *qevih = NULL;
  struct trivia_data *qevtr = NULL;
  struct quest_score_data *player = NULL;
  struct descriptor_data *d;

  if (GET_NAME(ch))
  {
    send_to_char("You end the quest.\r\n", ch); // QUEST_ITEM_HUNT
    sprintf(buf, "(GC) Quest '%s' ended by %s.", quest_names[ev->info1],
	    GET_NAME(ch));
  } else {
    sprintf(buf, "Quest '%s' has ended.", quest_names[ev->info1]);
  }
  mudlog(buf, BRF, LVL_GOD, TRUE);
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      if (!(ev->info2))
	break;
      qevih = (struct itemhunt_data *)ev->info2;
      if (qevih->ev != ev)
	break;
      sprintf(buf, "&Y- QUEST END - &0%s&n at &0%s&n\r\n",
	      quest_names[ev->info1], zone_table[ev->room->zone].name);
      player = qevih->players;
      while (player)
      {
	sprintf(buf, "%s&7%-20s&n - &5%3d&n Items.\r\n", buf, 
		player->name, player->score);
	qevih->players = player->next;
	free(player);
	player = qevih->players;
      }
      purge_quest_mobs();
      purge_quest_objs();
      send_to_questors(buf, ev);
      break;
    case QUEST_TRIVIA:
      if (!(ev->info2))
	break;
      qevtr = (struct trivia_data *)ev->info2;
      if (qevtr->ev != ev)
	break;
      if (GET_IDNUM(ch) != qevtr->trivmaster)
      { // Only the trivia master may end trivia, unless s/he's disappeared.
	for (d = descriptor_list; d; d = d->next)
	  if ((d->character) && (GET_IDNUM(d->character) == qevtr->trivmaster))
	    break;
	if (d)
	{
	  send_to_char("Only the trivia master may end trivia.\r\n", ch);
	  return;
	}
      }
      if (*qevtr->question)
      {
	send_to_char("Please close the current question, first.\r\n", ch);
	return;
      }
      if (qevtr->resp)
	free_remaining_responses(qevtr);
      sprintf(buf, "&Y- QUEST END - &0%s&n\r\nTopic: %s\r\n",
	      quest_names[ev->info1], qevtr->description);
      player = qevtr->players;
      while (player)
      {
	sprintf(buf, "%s&7%-20s&n - &5%3d&n Correct.\r\n", buf,
	        player->name, player->score);
	qevtr->players = player->next;
	free(player);
	player = qevtr->players;
      }
      send_to_questors(buf, ev);
    default:
      break;
  }
  ev->type = EVENT_OVER;
  remove_mud_event(ev);
  for (d = descriptor_list; d; d=d->next)
    if ((d->character) && !IS_NPC(d->character) &&
	PRF_FLAGGED(d->character, PRF_QUEST) && LR_FAIL(d->character, LVL_GOD))
      REMOVE_BIT(PRF_FLAGS(d->character), PRF_QUEST);
  return;
}

// Artus> Notify all of the quest.
void quest_notify(struct char_data *ch, struct event_data *ev)
{
  struct itemhunt_data *qevih;
  struct trivia_data *qevtr;

  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      sprintf(buf1, "\r\n&Y-- QUEST NOTICE --&n\r\n"
		    "&0Quest Type:&n %s\r\n"
		    "&0  Location:&n %s\r\n"
		    "&0      Item:&n %s\r\n", quest_names[ev->info1],
		    zone_table[ev->room->zone].name,
		    obj_proto[qevih->itemrnum].short_description);
      send_to_all(buf1);
      break;
    case QUEST_TRIVIA:
      qevtr = (struct trivia_data *)ev->info2;
      sprintf(buf1, "\r\n&Y-- QUEST NOTICE --&n\r\n"
	            "&0Quest Type:&n %s\r\n"
		    "&0     Topic:&n %s\r\n", quest_names[ev->info1],
		    qevtr->description);
      send_to_all(buf1);
      break;
    default:
      sprintf(buf1, "\r\n&Y-- QUEST NOTICE --&n\r\n"
	            "&0Quest Type:&n %s\r\n"
		    "For more information, speak to &7%s&n.\r\n",
		    quest_names[ev->info1], GET_NAME(ch));
      send_to_all(buf1);
      break;
  }
  sprintf(buf, "%s Quest initiated by %s.", quest_names[ev->info1],
	  GET_NAME(ch));
  mudlog(buf, BRF, LVL_GOD, TRUE);
}

// Artus> Create Trivia Quest.
void quest_create_trivia(struct char_data *ch, char *arg)
{
  struct event_data *ev;
  struct trivia_data *qevtr;
 
  // Sanity.
  if (!(ch) || (IS_NPC(ch)))
    return;
  if (arg)
    skip_spaces(&arg);
  if (!*arg)
  {
    send_to_char("Please enter a topic for the trivia quest.\r\n", ch);
    return;
  }

  // Create event structure.
  CREATE(ev, struct event_data, 1);
  ev->chID = GET_IDNUM(ch);
  ev->type = EVENT_QUEST;
  ev->room = &world[ch->in_room];
  ev->info1 = QUEST_TRIVIA;

  // Create trivia_data structure.
  CREATE(qevtr, struct trivia_data, 1);
  qevtr->ev = ev;
  qevtr->trivmaster = GET_IDNUM(ch);
  qevtr->qcount = 0;
  strcpy(qevtr->question, "");
  strncpy(qevtr->description, arg, MAX_INPUT_LENGTH);
  qevtr->description[MAX_INPUT_LENGTH] = '\0';
  qevtr->resp = NULL;
  qevtr->players = NULL;

  // Add the questmaster to the quest.
  SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
  ev->info2 = (long)qevtr;
  add_to_quest(ev, ch);
  add_mud_event(ev);

  // Output.
  quest_notify(ch, ev);
  send_to_char("&GYou begin a trivia quest.&n\r\n", ch);
}

// Artus> Create Item Hunt Quest.
void quest_create_itemhunt(struct char_data *ch, char *arg)
{
  obj_vnum objv;
  obj_rnum objr;
  struct event_data *ev;
  struct itemhunt_data *qevih;

  // Sanity.
  if (!(ch) || (IS_NPC(ch)))
    return;
  if (arg)
    skip_spaces(&arg);
  if (!*arg)
  {
    send_to_char("Just what item should the players be questing for!\r\n", ch);
    return;
  }
  if(!isdigit(arg[0]))
  {
    send_to_char("You must specify the vnum of the item to quest for.\r\n", ch);
    return;
  }
  objv = atoi(arg);
  objr = real_object(objv);
  if (objr == NOTHING)
  {
    send_to_char("That object does not seem to exist!\r\n", ch);
    return;
  }

  // Create event structure.
  CREATE(ev, struct event_data, 1);
  ev->chID = GET_IDNUM(ch);
  ev->type = EVENT_QUEST;
  ev->room = &world[ch->in_room];
  ev->info1 = QUEST_ITEM_HUNT;

  // Create itemhunt_data structure.
  CREATE(qevih, struct itemhunt_data, 1);
  qevih->itemvnum = objv;
  qevih->itemrnum = objr;
  qevih->lowlevel = GET_LEVEL(ch);
  qevih->mobsloaded = 0;
  qevih->ev = ev;
  qevih->players = NULL;

  // Add the questmaster to the quest.
  SET_BIT(PRF_FLAGS(ch), PRF_QUEST);
  ev->info2 = (long)qevih;
  add_to_quest(ev, ch);
  add_mud_event(ev);

  // Output.
  quest_notify(ch, ev);
  send_to_char("&GYou begin an item hunt quest.&n\r\n", ch);
}

// Artus> Quest Create.
void perform_quest_create(struct char_data *ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH], *leftovers;

  // Get Arguments.
  leftovers = one_argument(argument, arg1);
  skip_spaces(&leftovers);

  // Handle Arguments.
  if (is_abbrev(arg1, "itemhunt")) // Itemhunt.
  {
    if (!*leftovers || !is_number(leftovers))
    {
      send_to_char("Syntax: quest create itemhunt <item vnum>\r\n", ch);
      return;
    }
    quest_create_itemhunt(ch, leftovers);
    return;
  }
  if (is_abbrev(arg1, "trivia")) // Trivia.
  {
    if (!*leftovers)
    {
      send_to_char("Syntax: quest create trivia <description>\r\n", ch);
      return;
    }
    quest_create_trivia(ch, leftovers);
    return;
  }
  send_to_char("Invalid quest time. Valid types are: itemhunt, trivia.\r\n",ch);
  return;
}

// Artus> Show quest info.
void perform_quest_info(struct char_data *ch)
{
  struct event_data *ev;
  struct itemhunt_data *qevih;
  struct trivia_data *qevtr;
  struct quest_score_data *qs;

  ev = find_quest_event();
  if (!(ev))
  {
    send_to_char("There doesn't seem to be a quest in progress.\r\n", ch);
    return;
  }
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      if (!(qevih) || (qevih->ev != ev) || !(qevih->players))
      {
	send_to_char("This quest seems to be broken.\r\n", ch);
	return;
      }
      if (obj_index[qevih->itemrnum].number != qevih->itemvnum)
        qevih->itemrnum = real_object(qevih->itemvnum);
      if (LR_FAIL(ch, LVL_ANGEL))
      {
	sprintf(buf, "There is an item hunt happening at &8%s&n.\r\n"
	             "Participants are searching for: &5%s&n.\r\n",
		zone_table[ev->room->zone].name,
		obj_proto[qevih->itemrnum].short_description);
	send_to_char(buf, ch);
	return;
      }
      sprintf(buf2, qevih->players->name);
      for (qs = qevih->players->next; qs; qs = qs->next)
	sprintf(buf2, "%s&n,&c %s", buf2, qs->name);
      sprintf(buf, "  Quest Type: &c%s&n\r\n"
	           "        Zone: &c%s&n\r\n"
		   "Item Details: &c%d &n(&c%s&n)\r\n"
		   "Mobs Present: &c%d&n\r\n"
		   "Participants: &c%s&n\r\n", quest_names[ev->info1],
              zone_table[ev->room->zone].name, qevih->itemvnum,
	      obj_proto[qevih->itemrnum].short_description, qevih->mobsloaded, 
	      buf2);
      send_to_char(buf, ch);
      return;
    case QUEST_TRIVIA:
      qevtr = (struct trivia_data *)ev->info2;
      if (!(qevtr) || (qevtr->ev != ev) || !(qevtr->players))
      {
	send_to_char("This quest seems to be broken.\r\n", ch);
	return;
      }
      if (LR_FAIL(ch, LVL_ANGEL))
      {
	sprintf(buf, "There is a trivia quest happening.\r\n"
	             "&0%d&n questions have been asked about &0%s&n.\r\n",
		     qevtr->qcount, qevtr->description);
	send_to_char(buf, ch);
	return;
      }
      sprintf(buf2, qevtr->players->name);
      for (qs = qevtr->players->next; qs; qs = qs->next)
	sprintf(buf2, "%s&n,&c %s", buf2, qs->name);
      sprintf(buf, "  Quest Type: &c%s&n\r\n"
	           "       Topic: &c%s&n\r\n"
	           "      Master: &c%s&n\r\n"
		   "No Questions: &c%d&n\r\n"
		   "Participants: &c%s&n\r\n", quest_names[ev->info1],
		   qevtr->description, get_name_by_id(qevtr->trivmaster),
		   qevtr->qcount, buf2);
      send_to_char(buf, ch);
      return;
    default:
      send_to_char("This quest seems to be broken.\r\n", ch);
      return;
  }
  return;
}

// Artus> Show quest score.
void perform_quest_score(struct char_data *ch)
{
  struct event_data *ev;
  struct itemhunt_data *qevih;
  struct trivia_data *qevtr;
  struct quest_score_data *qs = NULL;
  int totalscore = 0;

  ev = find_quest_event();
  if (!(ev))
  {
    send_to_char("There doesn't seem to be a quest in progress.\r\n", ch);
    return;
  }
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT:
      qevih = (struct itemhunt_data *)ev->info2;
      if (!(qevih) || (qevih->ev != ev) || !(qevih->players))
	break;;
      qs = qevih->players;
      break;
    case QUEST_TRIVIA:
      qevtr = (struct trivia_data *)ev->info2;
      if (!(qevtr) || (qevtr->ev != ev) || !(qevtr->players))
	break;
      qs = qevtr->players;
      break;
  }
  if (!(qs))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  send_to_char("Player               - Score\r\n"
	       "----------------------------\r\n", ch);
  for (; qs; qs = qs->next)
  {
    sprintf(buf, "&5%-20s&n - &C%5d&n\r\n", qs->name, qs->score);
    send_to_char(buf, ch);
    totalscore += qs->score;
  }
  sprintf(buf, "&YTOTAL                - %5d&n\r\n", totalscore);
  send_to_char(buf, ch);
  return;
}

// Artus> Remove a player from the quest.
void perform_quest_leave(struct char_data *ch, char *argument)
{
  struct char_data *vict;
  struct event_data *ev;
  
  if (argument)
    skip_spaces(&argument);
  if (!(*argument))
    vict = ch;
  else
  {
    vict = generic_find_char(ch, argument, FIND_CHAR_WORLD);
    if (!(vict))
    {
      send_to_char("Could not find anyone by that name.\r\n", ch);
      return;
    }
  }
  if (IS_NPC(vict))
  {
    send_to_char("Mobs cannot be part of quests!\r\n", ch);
    return;
  }
  ev = find_quest_event();
  if (!(ev))
  {
    send_to_char("There is no quest running at this time.\r\n", ch);
    return;
  }
  if (vict != ch)
  {
    if ((GET_LEVEL(vict) >= GET_LEVEL(ch)) && (GET_IDNUM(ch) > 3))
    {
      send_to_char("I'm sorry dave, I'm afraid i can't do that.\r\n", ch);
      return;
    }
  }
  remove_from_quest(ev, vict);
}

// Artus> Join a player to the quest.
void perform_quest_join(struct char_data *ch, char *argument)
{
  struct char_data *vict;
  struct event_data *ev;

  if (argument)
    skip_spaces(&argument);
  if (!(*argument))
  {
    vict = ch;
  } else {
    vict = generic_find_char(ch, argument, FIND_CHAR_WORLD);
    if (!(vict))
    {
      send_to_char("Could not find anyone by that name.\r\n", ch);
      return;
    }
  }
  if (IS_NPC(vict))
  {
    send_to_char("Mobiles cannot enter quests!\r\n", ch);
    return;
  }
  ev = find_quest_event();
  if (!(ev))
  {
    send_to_char("Could not find a quest to join to.\r\n", ch);
    return;
  }
  if (vict != ch)
  {
    if ((GET_LEVEL(vict) >= GET_LEVEL(ch)) && (GET_IDNUM(ch) > 3))
    {
      send_to_char("I'm sorry, Dave, I'm afraid I can't do that.\r\n", ch);
      return;
    }
  }
  add_to_quest(ev, vict);
}

// Quest Restore.
void perform_quest_restore(struct char_data *ch, struct event_data *ev)
{
  struct itemhunt_data *qevih;

  // Sanity.
  if (ev->info1 != QUEST_ITEM_HUNT)
  {
    sprintf(buf, "This command does not apply to %s quests.\r\n",
	    quest_names[ev->info1]);
    send_to_char(buf, ch);
    return;
  }
  qevih = (struct itemhunt_data *)ev->info2;
  if (!(qevih) || (qevih->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return;
  }
  // No Cheating.
  if (world[IN_ROOM(ch)].zone != ev->room->zone)
  {
    sprintf(buf, "You may only restore in the quest zone, &3%s&n.\r\n",
	    zone_table[ev->room->zone].name);
    send_to_char(buf, ch);
    return;
  }
  if (!PRF_FLAGGED(ch, PRF_QUEST) || 
      !is_idnum_in_qlist(GET_IDNUM(ch), qevih->players))
  {
    send_to_char("But you're not part of an itemhunt quest.\r\n", ch);
    return;
  }
  GET_EXP(ch) -= MAX(0, (int)(GET_EXP(ch) * 0.1));
  GET_HIT(ch) = MAX(GET_HIT(ch), GET_MAX_HIT(ch));
  GET_MANA(ch) = MAX(GET_MANA(ch), GET_MAX_MANA(ch));
  GET_MOVE(ch) = MAX(GET_MOVE(ch), GET_MAX_MOVE(ch));
  send_to_char("You feel fully revitalised.\r\n", ch);
  return;
}

// Handle random spawning of items/mobs.
void handle_quest_item_hunt(struct event_data *ev)
{
  extern mob_rnum top_of_mobt;
  struct itemhunt_data *qevih;
  struct quest_score_data *player;
  int i, pcount = 0, mobrnum = NOBODY, roomrnum = NOWHERE;

  // Sanity.
  qevih = (struct itemhunt_data *)ev->info2;
  if ((qevih->ev != ev) || (number(1, 3) != 1))
    return;
  // Calc total found..
  for (player = qevih->players; player; player = player->next)
    pcount++;
  // Make sure we don't flood the mud.
  if (pcount <= qevih->mobsloaded)
    return;
  // If we get here, we want to randomly load a mob.
  for (i = 0; i < 10; i++)
  {
    mobrnum = number(0, top_of_mobt-1);
    if ((GET_LEVEL(&mob_proto[mobrnum]) <= qevih->lowlevel+10) &&
	(!MOB_FLAGGED(&mob_proto[mobrnum], MOB_NOKILL) &&
	 (mob_index[mobrnum].func != shop_keeper)))
      break;
    mobrnum = NOBODY;
  }
  // Artus> Didn't find a mob to use.
  if (mobrnum == NOBODY)
    return;
  for (i = 0; i < top_of_world; i++)
  {
    if (world[i].zone != ev->room->zone)
      continue;
    roomrnum = i + number(1, 300);
    if ((roomrnum > top_of_world) || (world[roomrnum].zone != ev->room->zone) ||
        (ROOM_FLAGGED(roomrnum, ROOM_DEATH | ROOM_PEACEFUL)))
      continue;
    break;
  }
  if ((roomrnum > top_of_world) || (world[roomrnum].zone != ev->room->zone))
    return;
  // Artus> Sanity.
  if (obj_proto[qevih->itemrnum].item_number != qevih->itemvnum)
    qevih->itemrnum = real_object(qevih->itemvnum);
  if (handle_quest_load(NULL, mobrnum, qevih->itemrnum, roomrnum, true))
    qevih->mobsloaded++;
}

// Handle quest randomness/timered stuff.
void handle_quest_event(struct event_data *ev)
{
  switch(ev->info1)
  {
    case QUEST_ITEM_HUNT: handle_quest_item_hunt(ev); break;
    case QUEST_TRIVIA: break;
    default:
      sprintf(buf, "Unhandled quest type: %ld found.", ev->info1);
      mudlog(buf, BRF, LVL_IMPL, TRUE);
      break;
  }
}

// Show itemhunt options. (Used by show_quest_options, do_quest).
void show_itemhunt_options(struct char_data *ch, struct event_data *ev)
{
  sprintf(buf, "        quest restore    - Heal yourself for &3%d&n exp.\r\n"
               "        quest locate     - Locate items for &310&n mana.\r\n",
          (int)(GET_EXP(ch) * 0.1));
  if (!LR_FAIL(ch, LVL_GOD))
    strcat(buf, "        quest load vnum  - Load mob vnum with quest objs, here.\r\n");
  send_to_char(buf, ch);
  return;
}

// Show trivia options. (Used by show_quest_options, do_quest).
void show_trivia_options(struct char_data *ch, struct event_data *ev)
{
  struct trivia_data *qevtr = (struct trivia_data *)ev->info2;

  send_to_char("        quest question   - View the current question\r\n", ch);
  if (qevtr->trivmaster != GET_IDNUM(ch))
  {
    send_to_char("        quest answer xxx - Submit your answer of 'xxx'\r\n",
	         ch);
    return;
  }
  send_to_char("        quest ask xxx    - Ask the question 'x'\r\n"
               "        quest close xxx  - Abort question. Show answer 'xxx'\r\n"
	       "        quest fail       - Reject the response\r\n"
	       "        quest pass       - Accept the response\r\n"
	       "        quest response   - View the next response\r\n", ch);
  return;
}

// Show quest options. (Used by do_quest)
void show_quest_options(struct char_data *ch, struct event_data *ev)
{
  bool imm = (!LR_FAIL(ch, LVL_GOD));

  // No Event.
  if (!(ev))
  {
    send_to_char("Syntax: quest create itemhunt <item vnum>\r\n"
	         "        quest create trivia <description>\r\n", ch);
    return;
  }
  // Basic Commands.
  send_to_char("Syntax: quest info       - Quest Description\r\n"
               "        quest score      - Quest Scoreboard\r\n", ch);
  if (!imm && !PRF_FLAGGED(ch, PRF_QUEST))
  { // Mortals not in quest.
    send_to_char("        quest join       - Join the current quest\r\n", ch);
    return;
  }
  if (imm)
    send_to_char("        quest join xxx   - Force xxx to join the quest\r\n"
	         "        quest leave xxx  - Boot xxx from the quest\r\n"
		 "        quest dec xxx    - Subtract one from xxx's score\r\n"
		 "        quest inc xxx    - Add one to xxx's score\r\n"
	         "        quest end        - End the current quest\r\n", ch);
  else
    send_to_char("        quest leave      - Leave the current quest\r\n", ch);
  switch (ev->info1)
  {
    case QUEST_ITEM_HUNT: show_itemhunt_options(ch, ev); break;
    case QUEST_TRIVIA:    show_trivia_options(ch, ev);   break; 
    default:
      send_to_char("This quest seems kinda broken.\r\n", ch);
      return;
  }
}

bool handle_itemhunt_cmd(struct char_data *ch, struct event_data *ev,
                         char *arg, char *params)
{
  struct itemhunt_data *qevih = (struct itemhunt_data *)ev->info2;

  // Sanity.
  if ((!qevih) || (qevih->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return true;
  }
  if (arg)
    skip_spaces(&arg);
  if (!*arg)
    return false;
  // Mortal Commands.
  if (is_abbrev(arg, "restore"))
  {
    perform_quest_restore(ch, ev);
    return true;
  }
  if (is_abbrev(arg, "locate"))
  {
    int i = 0;
    if (GET_MANA(ch) < 10)
    {
      send_to_char("Locating quest objects requires &g10&n mana.\r\n", ch);
      return true;
    }
    GET_MANA(ch) -= 10;
    GET_WAIT_STATE(ch) += PULSE_VIOLENCE;
    if (qevih->mobsloaded < 1)
    {
      act("There are no rumors of $p floating about at the moment.", FALSE,
	  ch, &obj_proto[qevih->itemrnum], 0, TO_CHAR);
      return true;
    }
    for (struct char_data *k = character_list; k; k = k->next)
      if (IS_NPC(k) && MOB_FLAGGED(k, MOB_QUEST) && 
	  (world[IN_ROOM(k)].zone == ev->room->zone))
      {
	i++;
	act("You hear that $N has $p.", FALSE, ch, &obj_proto[qevih->itemrnum],
	    k, TO_CHAR);
	if (i >= qevih->mobsloaded)
	  break;
      }
    return true;
  }
  if (LR_FAIL(ch, LVL_GOD))
    return false;
  // Imm Commands.
  if (is_abbrev(arg, "load"))
  {
    perform_quest_load(ch, ev, params);
    return true;
  }
  return false;
}

// Interpret Trivia Specific Commands.
bool handle_trivia_cmd(struct char_data *ch, struct event_data *ev, 
                       char *arg, char *params)
{
  struct trivia_data *qevtr = (struct trivia_data *)ev->info2;

  // Sanity.
  if ((!qevtr) || (qevtr->ev != ev))
  {
    send_to_char("This quest seems to be broken.\r\n", ch);
    return true;
  }
  if (arg)
    skip_spaces(&arg);
  if (!*arg)
    return false;
  
  // First off, the mortal commands.
  if (is_abbrev(arg, "answer"))
  {
    perform_quest_transwer(ch, ev, params);
    return true;
  }
  if (is_abbrev(arg, "question"))
  {
    perform_quest_trquestion(ch, ev);
    return true;
  }
  // Imm Commands Follow.
  if (LR_FAIL(ch, LVL_GOD))
    return false;
  if (is_abbrev(arg, "response"))
  {
    perform_quest_trresponse(ch, ev);
    return true;
  }
  if (qevtr->trivmaster != GET_IDNUM(ch))
    return false;
  // Now, the trivia master commands.
  if (!str_cmp(arg, "ask"))
  {
    perform_quest_trask(ch, ev, params);
    return true;
  }
  if (!str_cmp(arg, "pass"))
  {
    perform_quest_trpass(ch, ev);
    return true;
  }
  if (!str_cmp(arg, "fail"))
  {
    perform_quest_trfail(ch, ev);
    return true;
  }
  if (!str_cmp(arg, "close"))
  {
    perform_quest_trclose(ch, ev, params);
    return true;
  }
  return false;
}

// Qsay, Qecho.
ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_QUEST))
  {
    send_to_char("You aren't even part of the quest!\r\n", ch);
    return;
  }
  if (argument)
    skip_spaces(&argument);

  if (!*argument)
  {
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

// QuestOff.
ACMD(do_questoff)
{
  struct char_data *vict;
  char buff[120];
  half_chop(argument, arg, buf);
 
  if (!*arg)
  {
    send_to_char("Quest Off for who?\r\n", ch);
    return;
  }
  if (!str_cmp(arg, "all"))
  {
    int found = 0;
    for (vict = character_list; vict; vict=vict->next)
    {
      if (IS_NPC(vict) || (GET_LEVEL(vict) >= GET_LEVEL(ch)) ||
	  !PRF_FLAGGED(vict, PRF_QUEST))
	continue;
      act("$N is no longer in the quest.", FALSE, ch, 0, vict, TO_CHAR);
      REMOVE_BIT(PRF_FLAGS(vict), PRF_QUEST);
      found++;
    }
    if (found < 1) 
      send_to_char("Noone to remove from the quest.\r\n", ch);
    return;
  }
  if (!(vict = generic_find_char(ch, arg,FIND_CHAR_WORLD)))
  {
    send_to_char(NOPERSON, ch);
    return;
  }            REMOVE_BIT(PRF_FLAGS(vict),PRF_QUEST);
  sprintf(buff,"%s is no longer in the quest.\r\n",vict->player.name);
  send_to_char(buff,ch);
} 

// QuestOn
ACMD(do_queston)
{
  struct char_data *vict;
  char buff[120];

  half_chop(argument, arg, buf);
 
  if (!*arg)
  {
    send_to_char("Quest On for who, or \"list\" participants?\r\n", ch);
    return;
  }
  if (!str_cmp(arg, "list"))
  {
    bool found=false;
    strcpy(buf, "Who                  - Lvl Cl - Where\r\n"
	        "-----------------------------------------------------------------------------\r\n");
    for (vict = character_list; vict; vict = vict->next)
    {
      if (IS_NPC(vict) || !(PRF_FLAGGED(vict, PRF_QUEST)) ||
	  !CAN_SEE(ch, vict))
	continue;
      sprintf(buf, "%s&7%-20s&n - %3d %2s - [&8%5d&n] %s\r\n", buf, 
	      GET_NAME(vict), GET_LEVEL(vict), 
	      class_abbrevs[(int)GET_CLASS(vict)],
	      world[IN_ROOM(vict)].number, world[IN_ROOM(ch)].name);
      found = true;
    }
    if (found)
    {
      strcat(buf, "-----------------------------------------------------------------------------\r\n");
      send_to_char(buf, ch);
    } else {
      send_to_char("There is currently noone in the quest.\r\n", ch);
    }
    return;
  }
  if (!(vict = generic_find_char(ch, arg,FIND_CHAR_WORLD)))
  {
    send_to_char(NOPERSON, ch);
    return;
  }
  SET_BIT(PRF_FLAGS(vict),PRF_QUEST);
  sprintf(buff,"%s is now in the quest.\r\n",vict->player.name);
  send_to_char(buff,ch);
}

// Artus> Quest
ACMD(do_quest)
{
  bool imm = (!LR_FAIL(ch, LVL_GOD));
  struct event_data *ev;
  char *leftovers;

  if (IS_NPC(ch))
    return;

  ev = find_quest_event();

  if (!(ev) && !(imm))
  {
    send_to_char("There is no quest running at this time.\r\n", ch);
    return;
  }
  if (!argument || !*argument)
  {
    show_quest_options(ch, ev);
    return;
  }

  leftovers = one_argument(argument, arg);
  if (is_abbrev(arg, "join")) 
  {
    if (imm)
      perform_quest_join(ch, leftovers);
    else
      perform_quest_join(ch, "");
    return;
  }
  if (is_abbrev(arg, "score"))
  {
    perform_quest_score(ch);
    return;
  }
  if (is_abbrev(arg, "info"))
  {
    perform_quest_info(ch);
    return;
  }
  // You can't do this stuff if you're not in the quest.
  if (!PRF_FLAGGED(ch, PRF_QUEST) && !imm)
  {
    send_to_char("But you're not even part of the quest.\r\n", ch);
    return;
  }
  if (is_abbrev(arg, "leave"))
  {
    if (imm)
      perform_quest_leave(ch, leftovers);
    else
      perform_quest_leave(ch, "");
    return;
  }
  if (ev)
  {
    switch (ev->info1)
    {
      case QUEST_TRIVIA:
	if (handle_trivia_cmd(ch, ev, arg, leftovers))
	  return;
	break;
      case QUEST_ITEM_HUNT:
	if (handle_itemhunt_cmd(ch, ev, arg, leftovers))
	  return;
	break;
      default:
	send_to_char("This quest seems to be broken.\r\n", ch);
	return;
    }
    if (imm)
    {
      if (!str_cmp(arg, "end"))
      {
	perform_quest_end(ch, ev);
	return;
      }
      if (!str_cmp(arg, "inc"))
      {
	perform_quest_adjust(ch, ev, leftovers, +1);
	return;
      }
      if (!str_cmp(arg, "dec"))
      {
	perform_quest_adjust(ch, ev, leftovers, -1);
	return;
      }
      if (!str_cmp(arg, "load"))
      {
	perform_quest_load(ch, ev, leftovers);
	return;
      }
    }
    send_to_char("That doesn't seem to be a valid choice. Try one of these:\r\n", ch);
    show_quest_options(ch, ev);
    return;
  }
  // Beginning of immortal quest options.
  if (is_abbrev(arg, "create"))
  {
    if (ev)
    {
      send_to_char("There is already a quest in progress!\r\n", ch);
      return;
    }
    perform_quest_create(ch, leftovers);
    return;
  }
  // Whoops. We didn't match anything.
  send_to_char("That is not a valid choice. Try one of these:\r\n", ch);
  show_quest_options(ch, ev);
  return;
}

#ifdef WANT_QL_ENHANCE
// Enhance a quest item.
void enhance_quest_item(struct obj_data *qitem, struct char_data *ch, 
                        int itemnumber)
{
  int i, j, k, l;
  for (i = 0; i < MAX_QUEST_ITEMS; i++) 
  {
    if (GET_QUEST_ITEM(ch, i) == 0)
      continue;
    if (GET_QUEST_ITEM(ch, i) == GET_OBJ_VNUM(qitem))
      itemnumber--;	
    // We have the item to enhance (i = item)
    if (itemnumber == 0) 
    {
      // For every available enhancement
      for (j = 0; j < MAX_NUM_ENHANCEMENTS; j++) 
      {
	// If enhancement j for item i = 0
	if (GET_QUEST_ENHANCEMENT(ch, i, j) == 0)
	  continue;	
	// For every actual value (k) on enhancement j
	for (k = 0; k < MAX_ENHANCEMENT_VALUES; k++)
	{
	  int found = 0;
	  if (GET_QUEST_ENHANCEMENT_VALUE(ch, i, j, k) == 0)
	    continue;
	  // for every affect on the object
	  for (l = 0; l < MAX_OBJ_AFFECT; l++)
	  {
	    if (qitem->affected[l].location == GET_QUEST_ENHANCEMENT(ch, i, j))
	    {
	      qitem->affected[l].modifier += GET_QUEST_ENHANCEMENT_VALUE(ch,i,j, k);
	      found = 1;
	      break;
	    }
	  } // Loop through object affects.
	  if(!found) // New Affect
	  {
	    for (l = 0 ; l < MAX_OBJ_AFFECT; l++)
	      if (qitem->affected[l].location == APPLY_NONE)
	      {
		qitem->affected[l].modifier = GET_QUEST_ENHANCEMENT_VALUE(ch, i, j, k);
	        qitem->affected[l].location = GET_QUEST_ENHANCEMENT(ch, i, j);
		break;
	      }
	  } // End new affect (!found)
	} // For every enhancement value
      } // For every enhancement
    } // If this is the enhancement we want
  } // For every quest item
}

// Apply quest enhancement.
void apply_quest_enhancements(struct char_data *ch)
{
  struct obj_data *itemList[2048], *tmp;
  int itemcount, enhanced = 0, megacount = 0, i, j, k;

  // Create the inv/eq/house item megalist
  for (tmp = ch->carrying; tmp; tmp = tmp->next_content)
  {   // Inventory
    itemList[megacount] = tmp;
    megacount++;
  }

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) != NULL)
    {
      itemList[megacount] = ch->equipment[i];
      megacount++;
    }

  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].owner == get_id_by_name(GET_NAME(ch)))
      for (tmp = world[real_room(house_control[i].vnum)].contents; tmp; tmp = tmp->next)
      {
	itemList[megacount] = tmp;
	megacount++;
      } 

  // Go through each quest item
  for (i = 0; i < MAX_QUEST_ITEMS; i++)
  {
    if (GET_QUEST_ITEM(ch, i) == 0)
      continue;
    itemcount = 0;
    // Count the number of instances of enhancements on this item
    for (j = 0; j < MAX_QUEST_ITEMS; j++)
      if (GET_QUEST_ITEM(ch, j) == GET_QUEST_ITEM(ch, i))
	itemcount++;
    // Find all items of this type, enhance them
    if (itemcount >= 1)
      for (k = 0; k < megacount; k++)
	if (GET_OBJ_VNUM(itemList[k]) == GET_QUEST_ITEM(ch, i))
	{
	  if (itemcount <= 0)
	    break;
	  // Replace this item with a freshly loaded one, to remove
	  // previous enhancements and enhancement replication
	  extract_obj(itemList[k]);
	  itemList[k] = NULL;
	  enhanced++;
	  RECREATE(itemList[k], struct obj_data, 1);
	  itemList[k] = read_object(GET_QUEST_ITEM(ch, i), VIRTUAL);
	  enhance_quest_item(itemList[k], ch, enhanced);
	  // Give it back to the player, if it was in their house, bad luck
	  obj_to_char(itemList[k], ch, __FILE__, __LINE__);
	  itemcount--;  
	}
  }  
  if (enhanced)
  {
    sprintf(buf, "%d enhancement%s applied.\r\n", enhanced, enhanced == 1 ? "" : "s");
    send_to_char(buf, ch);
    return;
  }
  send_to_char("Failed to apply enhancement.\r\n", ch);
}

// Show quest enhancement options.
void show_enhancements_to_player(struct char_data *ch)
{
  send_to_char("&1Quest Item Enhancements:&n\r\n", ch);
  send_to_char( "&y1&n  - Strength\r\n"
		"&y2&n  - Dexterity\r\n"
		"&y3&n  - Intelligence\r\n"
		"&y4&n  - Wisdom\r\n"
		"&y5&n  - Constitution\r\n"
		"&y6&n  - Charisma\r\n"
		"&y7&n  - Class      (Not used)\r\n"
		"&y8&n  - Level      (Not used)\r\n"
		"&y9&n  - Age\r\n"
		"&y10&n - Weight\r\n"
		"&y11&n - Height\r\n"
		"&y12&n - Mana\r\n"
		"&y13&n - Hit\r\n"
		"&y14&n - Move\r\n"
		"&y15&n - Gold       (Not used)\r\n"
		"&y16&n - Experience (Not used)\r\n"
		"&y17&n - Armour Class\r\n"
		"&y18&n - Hitroll\r\n"
		"&y19&n - Damroll\r\n"
		"&y20&n - Save vs Paralysation\r\n"
		"&y21&n - Save vs Rod, Staff, Wand\r\n"
		"&y22&n - Save vs Petrification\r\n"
		"&y23&n - Save vs Breath Weapon\r\n"
		"&y24&n - Save vs Spells\r\n", ch);
}

// Remove a quest enhancement.
void remove_enhancement(struct char_data *ch, struct char_data *vict, 
                        int itemno, int enhno)
{
  int i = 0;
  sprintf(buf, "Quest item enhancement (set on %s) for &7%s&n removed.\r\n",
	  enhancement_names[GET_QUEST_ENHANCEMENT(vict, itemno, enhno)], 
	  GET_NAME(ch));
  send_to_char(buf, ch);
  for (i = 0; i < MAX_ENHANCEMENT_VALUES; i++)
    GET_QUEST_ENHANCEMENT_VALUE(vict, itemno, enhno, i) = 0;
  GET_QUEST_ENHANCEMENT(vict, itemno, enhno) = 0; 
}
#endif

void questlog_list(struct char_data *ch, char *arg)
{
#ifdef WANT_QL_ENHANCE
  bool isenh = false;
  int i;
#endif
  bool found = false;
  long srchid = -1, lastid = -1;
  obj_vnum srchnum = -1;

  if (!LR_FAIL(ch, LVL_GOD) && (*arg))
  {
    skip_spaces(&arg);
    if (*arg)
    {
      if (is_number(arg))
	srchnum = atoi(arg);
      else if (str_cmp(arg, "all") && ((srchid = get_id_by_name(arg)) == -1))
      {
	send_to_char("Could not find a player by that name.\r\n", ch);
	return;
      }
    }
  } else 
    srchid = GET_IDNUM(ch);
  if (!(questlist_table))
  {
    send_to_char("There are currently no quest items in the list.\r\n", ch);
    return;
  }
  if (srchnum > -1)
  {
    for (struct questlist_element *k = questlist_table; k; k = k->next)
    {
      if (srchnum != k->objdata.vnum)
	continue; //<<<<<<<<<<<<<<
      if (!found)
      {
#ifdef WANT_QL_ENHANCE
	sprintf(buf, "&7Name                &rNum&n Enh \"&5%s&n\r\n", 
	        obj_proto[k->objrnum].short_description);
#else
	sprintf(buf, "&7Name                &rNum&n --- &5%s&n\r\n", 
	        obj_proto[k->objrnum].short_description);
#endif
	send_to_char(buf, ch);
	found = true;
      }
#ifdef WANT_QL_ENHANCE
      isenh = false;
      for (i = 0; i < MAX_NUM_ENHANCEMENTS; i++)
	if (k->objdata.enh_setting[i] != 0)
	{
	  isenh = true;
	  break;
	}
      sprintf(buf, "%-20s%3d %3s\r\n", k->name, k->objdata.max_number,
	      (isenh ? "Yes" : "No"));
#else
      sprintf(buf, "%-20s%3d\r\n", k->name, k->objdata.max_number);
#endif
      send_to_char(buf, ch);
    } // Traverse questlist list.
    if (!(found))
      send_to_char("Noone seems to have that quest item.\r\n", ch);
    return;
  }
  for (struct questlist_element *k = questlist_table; k; k = k->next)
  {
    if ((srchid > -1) && (srchid != k->id))
      continue; //<<<<<<<<<<<
    found = true;
    if (lastid != k->id)
    {
      sprintf(buf, "&7%s's&n Quest Items:\r\n", k->name);
      send_to_char(buf, ch);
    }
#ifdef WANT_QL_ENHANCE
    isenh = false;
    for (i = 0; i < MAX_NUM_ENHANCEMENTS; i++)
      if (k->objdata.enh_setting[i] != 0)
      {
	isenh = true;
	break;
      }
    sprintf(buf, "    &c[%5d] &r%dx&5%s &y%s&n\r\n",
	    k->objdata.vnum, k->objdata.max_number, 
	    (k->objrnum >= 0) ? obj_proto[k->objrnum].short_description : 
				"&r<Invalid Obj>", (isenh) ? "(Enh)" : "");
#else
    sprintf(buf, "    [&c%5d&n] &r%dx&5%s&n\r\n", 
	    k->objdata.vnum, k->objdata.max_number, 
	    (k->objrnum >= 0) ? obj_proto[k->objrnum].short_description :
	                        "&r<Invalid Obj>");
#endif
    send_to_char(buf, ch);
    lastid = k->id;
  }
  if (!found)
  {
    if (srchid != -1)
    {
      if (srchid == GET_IDNUM(ch))
	send_to_char("You don't have any quest items.\r\n", ch);
      else
      {
	sprintf(buf, "%s doesn't have any quest items.\r\n", get_name_by_id(srchid));
	send_to_char(buf, ch);
      }
      return;
    }
    send_to_char("Something screwed up.\r\n", ch);
  }
  return;
}  

// Questlog Add x
void questlog_add(struct char_data *ch, struct char_data *vict, char *rest)
{
  struct questlist_element *k=NULL, *l=NULL;
  char arg3[MAX_INPUT_LENGTH];
  bool found = false;
  int i;
  obj_rnum rnum;
  obj_vnum objnum;

  half_chop(rest, arg3, rest);
  if (!(isdigit(*arg3)))
  {
    send_to_char("&1Usage: &4questlog add <player> &c<obj vnum>&n\r\n",ch);
    return;
  }
  if (!(objnum = atoi(arg3)))
  {
    send_to_char("Thats not a vnum.\r\n",ch);
    return;
  }
  if ((rnum = real_object(objnum)) < 0)
  {
    send_to_char("There is no object with that number.\r\n", ch);
    return;
  }
  // Artus> Warn that item is not QEQ... Stop RHCP giving out dud items.
  if (!(OBJ_FLAGGED(&obj_proto[rnum], ITEM_QEQ)))
  {
    sprintf(buf, "Warning: &5%s&n is not QEQ flagged.\r\n", 
	    obj_proto->short_description);
    send_to_char(buf, ch);
  }
  for (i=0; (i < MAX_QUEST_ITEMS) && (!found); i++)
    if (GET_QUEST_ITEM(vict,i) == objnum)
    {
      found = true;
      GET_QUEST_ITEM_NUMB(vict,i)++;
      for (k=questlist_table; k; k=k->next)
	if ((k->id == GET_IDNUM(vict)) && 
	    (k->objdata.vnum == GET_QUEST_ITEM(vict,i)))
	{
	  k->objdata.max_number = GET_QUEST_ITEM_NUMB(vict,i);
	  break;
	}
    }
  // find first available space ...
  for (i=0; (i < MAX_QUEST_ITEMS) && (!found); i++)
  {
    if (GET_QUEST_ITEM(vict,i) <= 0)
    {
      found = true;
      GET_QUEST_ITEM(vict,i) = objnum;
      GET_QUEST_ITEM_NUMB(vict,i) = 1;
      for (k=questlist_table; k; k=k->next)
      {
	l=k;
	if ((k->id == GET_IDNUM(vict)) && 
	    (!(k->next) || k->next->id != GET_IDNUM(vict)))
	  break;
      }
      if (l)
      {
	struct questlist_element *noob;
	CREATE(noob, struct questlist_element, 1);
	noob->next = l->next;
	l->next = noob;
	noob->id = GET_IDNUM(vict);
	noob->name = str_dup(GET_NAME(vict));
	noob->objrnum = rnum;
	memcpy(&noob->objdata, &GET_QUEST_ITEM_DATA(vict,i), 
	       sizeof(struct quest_obj_data));
      } else
	add_to_questlist(GET_NAME(vict), GET_IDNUM(vict),
	                 &GET_QUEST_ITEM_DATA(vict,i));
    }
  }
  if (found)
    send_to_char("Object Added.\r\n",ch);
  else
    send_to_char("Maximum number of objects exceeded.\r\n",ch);
}

// Questlog Delete x
void questlog_del(struct char_data *ch, struct char_data *vict, char *rest)
{
  bool found = false;
  char arg3[MAX_INPUT_LENGTH];
  unsigned int qsearch = 0, i = 0;
  struct questlist_element *k;
  void perform_remove(struct char_data *ch, int pos);
  obj_vnum objnum;

  half_chop(rest, arg3, rest);
  if (!(isdigit(*arg3)))
  {
    send_to_char("&1Usage: &4questlog del <player> &c<obj vnum>&n\r\n",ch);
    return;
  }
  // Get either the <x>. or the rnum
  qsearch = atoi(arg3);
  // See if they are looking for a particular instance
  while ((arg3[i] != '.') && (arg3[i] != ' ') && (i < strlen(arg3)))
    i++;
  // Allocate the rnum, will be 0 if there was no <x>.<rnum>
  objnum = atoi(&arg3[i+1]);
  // Check if there was no . in the arg
  if (objnum == 0)
  {
    objnum = qsearch;         // QSearch is holding the rnum then
    qsearch = 1;            // Set to first occurance
  }
  // If it's still 0, they haven't set a vnum
  if (objnum == 0)
  {
    send_to_char("You must specify an object vnum.\r\n", ch);
    return;
  }
  for (i=0; i < MAX_QUEST_ITEMS; i++)
  {
    if (GET_QUEST_ITEM(vict,i) == objnum)
    {
      if (qsearch == 1)
      {
	found = true;
	if (GET_QUEST_ITEM_NUMB(vict, i) > 1)
	{
	  GET_QUEST_ITEM_NUMB(vict, i)--;
	  update_questlist(vict, i);
	  break;
	}
	for (k = questlist_table; k; k = k->next)
	  if ((k->id == GET_IDNUM(vict)) && 
	      (k->objdata.vnum == GET_QUEST_ITEM(vict, i)))
	  {
	    struct questlist_element *temp = NULL;
	    REMOVE_FROM_LIST(k, questlist_table, next);
	    free(k->name);
	    free(k);
	    break;
	  }
	GET_QUEST_ITEM(vict, i) = 0;
	// ahh what about the enhancements tali???
	for (int j = 0; j < MAX_NUM_ENHANCEMENTS; j++)
	{
	  GET_QUEST_ENHANCEMENT(ch, i, j) = 0;
	  for (int k = 0; k < MAX_ENHANCEMENT_VALUES; k++)
	    GET_QUEST_ENHANCEMENT_VALUE(ch, i, j, k) = 0;
	}
	// do an ingame check
	break;
      } else
	qsearch--;
    }
  }
  if (found)
  {
    send_to_char("Object Removed.\r\n",ch);
    if (vict->in_room != NOWHERE)
      for (int i = 0; i < NUM_WEARS; i++)
	if (GET_EQ(vict, i) && GET_OBJ_VNUM(GET_EQ(vict, i)) == objnum)
	  perform_remove(vict, i);
  } else {
    send_to_char("Object not found in list.\r\n",ch);
  }
}

#ifdef WANT_QL_ENHANCE // Artus> I think i'll just throw enhance/stat for now.
// Questlog Enhance x
void questlog_enhance(struct char_data *ch, struct char_data *vict, char *rest)
{
  char arg3[MAX_INPUT_LENGTH] = "";
  int found = 0;

  half_chop(rest, arg3, rest);
  if (!*arg3)
  {
    send_to_char("&1Enhancement usage: &4questlog enhance <player> <x>.<obj vnum> <enh #> <val>\r\n"
		 "       &1Optionally: &4questlog enhance <player> <list | apply | delete>\r\n", ch);
    return;
  }
  // If they're not enhancing they can list, apply or delete
  if (!(isdigit(*arg3)))
  {
    if (strcmp(arg3, "apply") == 0) // Apply enhancements to players items
    {
      apply_quest_enhancements(vict);
      return;
    } else if (strcmp(arg3, "delete") == 0) {
      half_chop(rest, arg3, rest);
      if (!isdigit(*arg3))
      {
	if (strcmp(arg3, "all") == 0)
	{
	  for (i = 0; i < MAX_QUEST_ITEMS; i++)
	    if (GET_QUEST_ITEM(ch, i) != 0)
	      for (j = 0; j < MAX_NUM_ENHANCEMENTS; j++)
		if (GET_QUEST_ENHANCEMENT(vict, i, j) != 0)
		  remove_enhancement(ch, vict, i, j); 
	  return;
	}
	send_to_char("Deletion of enhancements must be either 'all' or a number.\r\n",ch);
	return;
      }
      // They gave us a number..
      found = 0;
      for (i = 0; i < MAX_QUEST_ITEMS; i++)
	if (GET_QUEST_ITEM(ch, i) != 0)
	  for (j = 0; j < MAX_NUM_ENHANCEMENTS; j++)
	    if (GET_QUEST_ENHANCEMENT(ch, i, j) != 0)
	    {
	      if (counter == atoi(arg3))
	      {
		remove_enhancement(ch, vict, i, j);
		found = true; 
		return;
	      }
	      counter++;
	    }	
      if (!found)
	send_to_char("That enhancement doesn't exist!\r\n", ch);
      return; // end of delete enhancement
    } else if (strcmp(arg3, "list") == 0) { // Are they listing?
      // Prepare the list
      sprintf(buf, "Quest item enhancements for &7%s&n:\r\n", GET_NAME(vict));
      for (i = 0; i < MAX_QUEST_ITEMS; i++) 
      {
	if (GET_QUEST_ITEM(vict, i) != 0)
	{
	  found++;
	  for (j = 0; j < MAX_NUM_ENHANCEMENTS; j++)
	    if (GET_QUEST_ENHANCEMENT(vict, i, j) != 0)
	      for (k = 0; k < MAX_ENHANCEMENT_VALUES; k++)
		if (GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k) != 0)
		{ 
		  foundEnh++;
		  sprintf(buf + strlen(buf), "  Item #&c%2d&n(&C%5d&n) Enh #&g%d&n - adds &y%s&n with value &W%d&n\r\n",
		  i+1, GET_QUEST_ITEM(vict, i), j + 1,
		  enhancement_names[GET_QUEST_ENHANCEMENT(vict, i, j)],
		  GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k));
		}
	} // Found quest item.
      } 
      if (!found)
	send_to_char("They have no quest items..\r\n", ch);
      else if (!foundEnh)
	send_to_char("They have no enhancements on their items.\r\n", ch);
      else
	send_to_char(buf, ch);
      return;
    }
    send_to_char("&1Usage: &4questlog enhance <name> <num in list>.<obj vnum> <enh #> <val>\r\n",ch);
    return;
  }
  // Get either the <x>. or the rnum
  qsearch = atoi(arg3);
  strcpy(arg2, arg3);

  // See if they are looking for a particular instance
  i = 0;
  while ((arg3[i] != '.') && (arg3[i] != ' ') && (i < strlen(arg3)))
    i++;
 
  // Allocate the rnum, will be 0 if there was no <x>.<rnum>
  rnum = atoi(&arg3[i+1]);
  // Check if there was no . in the arg
  if (rnum == 0)
  {
    rnum = qsearch;         // QSearch is holding the rnum then
    qsearch = 1;            // Set to first occurance
  }
  // If it's still 0, they haven't set a vnum
  if (rnum == 0)
  {
    send_to_char("You must specify an object vnum.\r\n", ch);
    break;
  }
  // Get the enhancement
  half_chop(rest, arg3, rest);
  if (!isdigit(*arg3))
  {
    send_to_char("The enhancement must be an integer value, 1 to 24.\r\n", ch);
    break;
  }
  enhancement = atoi(arg3);
 
  if (enhancement == 0)
  {
    send_to_char("You must specify a valid enhancement number and value.\r\n" , ch);
    break;
  }
  // Get the value
  half_chop(rest, arg3, rest);
  if (!isdigit(*arg3) && atoi(arg3) >= 0)
  {
    send_to_char("The value must be an integer value.\r\n", ch);
    break;
  }
  value = atoi(arg3);
 
  if ((value == 0) || (value < -125) || (value > 125))
  {
    send_to_char("You must provide a valid value for the enhancement.\r\n", ch);
    break;
  }
 
  sprintf(buf, "Seeking to enhance item number %d with vnum %d.\r\n", qsearch, rnum);
  send_to_char(buf, ch);
	
  found = 0;
  // OKay, we have all values, do enhancement 
  for (i = 0; i < MAX_QUEST_ITEMS; i++)
  {
    // If this quest item has the rnum we're looking for
    if (GET_QUEST_ITEM(vict, i) == rnum)
    {
      if (qsearch != 1)
      {
	qsearch--;
	continue;
      }
    } else
      continue;
    found = 1;
    // rnum and qsearch are now valid, so set the quest item enhancement
    GET_QUEST_ITEM(vict, i) = rnum;
    GET_QUEST_ITEM_OBJ(vict, i) = 0;	// Not sure if i'm going to use this yet
    // Search for an enhancement slot, either with same enhancement, or blank
    for (j = 0; j < MAX_NUM_ENHANCEMENTS; j ++)
      if((GET_QUEST_ENHANCEMENT(vict, i, j) == 0) || 
	 (GET_QUEST_ENHANCEMENT(vict, i, j) == enhancement)) 
	break;

    if (j >= MAX_NUM_ENHANCEMENTS)
    {
      send_to_char("Item already has as many enhancements as possible.\r\n", ch);	
      break;
    }
    GET_QUEST_ENHANCEMENT(vict, i, j) = enhancement;
    // Look for a free slot to place the value in
    for (k = 0; k < MAX_ENHANCEMENT_VALUES; k++)
      if (GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k) == 0)
	break;
    if (k >= MAX_ENHANCEMENT_VALUES)
    {
      sprintf(buf, "That item already has as many boosts to %s as possible.\r\n", enhancement_names[GET_QUEST_ENHANCEMENT(ch, i, j)]);
      send_to_char(buf, ch);
      break;
    }
    GET_QUEST_ENHANCEMENT_VALUE(vict, i, j, k) = value;
    sprintf(buf, "Quest item #&c%d&n(&c%d&n) for &b%s&n was enhanced by &W%d&n on &y%s&n.\r\n", 
    qsearch, rnum, GET_NAME(vict), value, enhancement_names[enhancement]);
    send_to_char(buf, ch);
    break;
  }
  if (!found)
    send_to_char("Could not locate that quest item in the log!\r\n",ch);
  return;
}

// Questlog Stat x
void questlog_stat(struct char_data *ch, struct char_data *vict, char *rest)
{
  char arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  int qsearch = 0;
  int i, megacount = 0, found = 0;
  struct obj_data *tmp, *itemList[2048];
  void do_stat_object(struct char_data * ch, struct obj_data * j);
  obj_rnum rnum;

  half_chop(rest, arg3, rest);
  if (!*arg3)
  {
    send_to_char("&1Usage: questlog stat <player> [<x>.]<obj vnum>\r\n", ch);
    return;
  }
  if (!isdigit(*arg3))
  {
    send_to_char("The quest item id must be a vnum.\r\n", ch);
    return;
  }
  // Get either the <x>. or the rnum
  qsearch = atoi(arg3);
  strcpy(arg2, arg3);
    
  // See if they are looking for a particular instance
  i = 0;
  while ((arg3[i] != '.') && (arg3[i] != ' ') && (i < (int)strlen(arg3)))
    i++;
  // Allocate the rnum, will be 0 if there was no <x>.<rnum>
  rnum = atoi(&arg3[i+1]);
  // Check if there was no . in the arg
  if (rnum == 0)
  {
    rnum = qsearch;		// QSearch is holding the rnum then
    qsearch = 1;		// Set to first occurance
  }
  // If it's still 0, they haven't set a vnum
  if (rnum == 0)
  {
    send_to_char("You must specify an object vnum.\r\n", ch);
    return;
  }
  // Create a list of all their items
  // Create the inv/eq/house item megalist
  for (tmp = ch->carrying; tmp; tmp = tmp->next_content)   // Inventory
  {
    itemList[megacount] = tmp;
    megacount++;
  }
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) != NULL)
    {
      itemList[megacount] = ch->equipment[i];
      megacount++;
    }
  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].owner == get_id_by_name(GET_NAME(ch)))
      for (tmp = world[real_room(house_control[i].vnum)].contents; tmp; tmp = tmp->next)
      {
	itemList[megacount] = tmp;
	megacount++;
      } 
  // Go through the list, finding the instance of the object they're after
  found = 0;
  for (i = 0; i < megacount; i++)
    if (GET_OBJ_VNUM(itemList[i]) == rnum)
    {
      if (qsearch != 1)
      {
	qsearch--;
	continue;
      }
      do_stat_object(ch, itemList[i]);
      found = 1;
      break;
    }

  if (!found)
    send_to_char("They don't have that item on them.\r\n", ch);	

  return;
}
#endif

ACMD(do_quest_log)
{
  struct char_data *vict;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  void (*qlfunc)(struct char_data *, struct char_data *, char *);

  // Mobs.
  if (IS_NPC(ch))
    return;
  // Mortals.
  if (LR_FAIL(ch, LVL_GOD))
  {
    questlog_list(ch, NULL);
    return;
  }
  // What are we doing?
  skip_spaces(&argument);
  half_chop(argument, arg1, rest);
  if (!str_cmp(arg1,"list"))
  {
    questlog_list(ch, rest);
    return;
  } else if (!str_cmp(arg1,"add"))
    qlfunc = &questlog_add;
  else if (!str_cmp(arg1,"del"))
    qlfunc = &questlog_del;
#ifdef WANT_QL_ENHANCE
  else if (!str_cmp(arg1,"enhance"))
    qlfunc = &questlog_enhance;
  else if (!str_cmp(arg1,"stat"))
    qlfunc = &questlog_stat;
  else 
  {
    send_to_char("That'll either be &4list&n/&4add&n/&4del&n/&4enhance&n/&4stat&n.\r\n",ch);
#else
  else
  {
    send_to_char("Syntax: questlog list [player/objvnum/all]\r\n"
	         "        questlog add  <player> <objvnum>\r\n"
		 "        questlog del  <player> [n.]<objvnum>\r\n", ch);
#endif
    return;
  }

  // Obtain victim.
  half_chop(rest, arg2, rest);
  if (!*arg2)
  {
#ifdef WANT_QL_ENHANCE
    if (qlfunc == &questlog_enhance)
      show_enhancements_to_player(ch);
    else
#endif
      send_to_char("Just who did you want to do that to?\r\n", ch);
    return;
  }
  if ((vict = get_player_online(ch, arg2, FIND_CHAR_WORLD)) == NULL)
  {
    send_to_char("That player doesn't seem to be around.\r\n", ch);
    return;
  }
  (*qlfunc)(ch, vict, rest);
}
