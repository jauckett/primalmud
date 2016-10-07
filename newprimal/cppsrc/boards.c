/* ************************************************************************
*   File: boards.c                                      Part of CircleMUD *
*  Usage: handling of multiple bulletin boards                            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* FEATURES & INSTALLATION INSTRUCTIONS ***********************************

This board code has many improvements over the infamously buggy standard
Diku board code.  Features include:

- Arbitrary number of boards handled by one set of generalized routines.
  Adding a new board is as easy as adding another entry to an array.
- Safe removal of messages while other messages are being written.
- Does not allow messages to be removed by someone of a level less than
  the poster's level.


TO ADD A NEW BOARD, simply follow our easy 4-step program:

1 - Create a new board object in the object files

2 - Increase the NUM_OF_BOARDS constant in boards.h

3 - Add a new line to the board_info array below.  The fields, in order, are:

	Board's virtual number.
	Min level one must be to look at this board or read messages on it.
	Min level one must be to post a message to the board.
	Min level one must be to remove other people's messages from this
		board (but you can always remove your own message).
	Filename of this board, in quotes.
	Last field must always be 0.

4 - In spec_assign.c, find the section which assigns the special procedure
    gen_board to the other bulletin boards, and add your new one in a
    similar fashion.

*/


#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "clan.h"

/* Board appearance order. */
#define	NEWEST_AT_TOP	FALSE

extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

/*
format:	vnum, read lvl, write lvl, remove lvl, filename, 0 at end
Be sure to also change NUM_OF_BOARDS in board.h
*/

struct board_info_type board_info[NUM_OF_BOARDS] = {
  {1198, LVL_GOD  ,    LVL_GOD  , LVL_GRGOD, LIB_ETC_BOARD "builder", 0},
  {1294,         0,            5, LVL_GOD  , LIB_ETC_BOARD "mortal" , 0},
  {1293, LVL_CHAMP,    LVL_CHAMP, LVL_GRGOD, LIB_ETC_BOARD "immort" , 0},
  {1292,         0,            5, LVL_ANGEL, LIB_ETC_BOARD "quest"  , 0},
  {1291, LVL_IMPL ,    LVL_IMPL , LVL_IMPL , LIB_ETC_BOARD "god"    , 0},
  {1290,         0,            5, LVL_ANGEL, LIB_ETC_BOARD "master" , 0},
  {1289,         0,            5, LVL_ANGEL, LIB_ETC_BOARD "social" , 0},
  {25500,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan0"  , 0},
  {25501,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan1"  , 0},
  {25502,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan2"  , 0},
  {25503,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan3"  , 0},
  {25504,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan4"  , 0},
  {25505,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan5"  , 0},
  {25506,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan6"  , 0},
  {25507,        0, LVL_CLAN_MIN, LVL_ANGEL, LIB_ETC_BOARD "clan7"  , 0},
};

/* local functions */
SPECIAL(gen_board);
int find_slot(void);
int find_board(struct char_data * ch);
void init_boards(void);

char *msg_storage[INDEX_SIZE];
int msg_storage_taken[INDEX_SIZE];
int num_of_msgs[NUM_OF_BOARDS];
int ACMD_READ, ACMD_LOOK, ACMD_EXAMINE, ACMD_WRITE, ACMD_REMOVE;
int Board_clan_check(struct char_data *ch, int board_type, int minrel);
struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];

void assign_boards(void)
{
  ASSIGNOBJ(1198, gen_board);   /* builders board */
  ASSIGNOBJ(1294, gen_board);	/* mortal board */
  ASSIGNOBJ(1293, gen_board);	/* immortal board */
  ASSIGNOBJ(1292, gen_board);	/* quest board */
  ASSIGNOBJ(1291, gen_board);	/* gods board */
  ASSIGNOBJ(1290, gen_board);	/* master board */
  ASSIGNOBJ(1289, gen_board);	/* social board */

  ASSIGNOBJ(25500, gen_board);	/* clan board */
  ASSIGNOBJ(25501, gen_board);	/* clan board */
  ASSIGNOBJ(25502, gen_board);	/* clan board */
  ASSIGNOBJ(25503, gen_board);	/* clan board */
  ASSIGNOBJ(25504, gen_board);	/* clan board */
  ASSIGNOBJ(25505, gen_board);	/* clan board */
  ASSIGNOBJ(25506, gen_board);	/* clan board */
  ASSIGNOBJ(25507, gen_board);	/* clan board */
}

int find_slot(void)
{
  int i;

  for (i = 0; i < INDEX_SIZE; i++)
    if (!msg_storage_taken[i]) {
      msg_storage_taken[i] = 1;
      return (i);
    }
  return (-1);
}


/* search the room ch is standing in to find which board he's looking at */
int find_board(struct char_data * ch)
{
  struct obj_data *obj;
  int i;

  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
    for (i = 0; i < NUM_OF_BOARDS; i++)
      if (BOARD_RNUM(i) == GET_OBJ_RNUM(obj))
	return (i);

  return (-1);
}


void init_boards(void)
{
  int i, j, fatal_error = 0;

  for (i = 0; i < INDEX_SIZE; i++) {
    msg_storage[i] = 0;
    msg_storage_taken[i] = 0;
  }

  for (i = 0; i < NUM_OF_BOARDS; i++) {
    if ((BOARD_RNUM(i) = real_object(BOARD_VNUM(i))) == -1) {
      basic_mud_log("SYSERR: Fatal board error: board vnum %d does not exist!",
	      BOARD_VNUM(i));
      fatal_error = 1;
    }
    num_of_msgs[i] = 0;
    for (j = 0; j < MAX_BOARD_MESSAGES; j++) {
      memset((char *) &(msg_index[i][j]), 0, sizeof(struct board_msginfo));
      msg_index[i][j].slot_num = -1;
    }
    Board_load_board(i);
  }

  ACMD_READ = find_command("read");
  ACMD_WRITE = find_command("write");
  ACMD_REMOVE = find_command("remove");
  ACMD_LOOK = find_command("look");
  ACMD_EXAMINE = find_command("examine");

  if (fatal_error)
    exit(1);
}


SPECIAL(gen_board)
{
  int board_type;
  static int loaded = 0;
  struct obj_data *board = (struct obj_data *)me;

  if (!loaded) 
  {
    init_boards();
    loaded = 1;
  }
  if ((!ch) || (!ch->desc))
    return (0);

  if (cmd != ACMD_WRITE && cmd != ACMD_LOOK && cmd != ACMD_EXAMINE &&
      cmd != ACMD_READ && cmd != ACMD_REMOVE)
    return (0);

  if ((board_type = find_board(ch)) == -1) {
    basic_mud_log("SYSERR:  degenerate board!  (what the hell...)");
    return (0);
  }
  if (cmd == ACMD_WRITE)
    return (Board_write_message(board_type, ch, argument, board));
  else if (cmd == ACMD_LOOK || cmd == ACMD_EXAMINE)
    return (Board_show_board(board_type, ch, argument, board));
  else if (cmd == ACMD_READ)
    return (Board_display_msg(board_type, ch, argument, board));
  else if (cmd == ACMD_REMOVE)
    return (Board_remove_msg(board_type, ch, argument, board));
  else
    return (0);
}

// Helper function, determines whether the luser has priveledges to write
// on the board.
int Board_clan_check(struct char_data *ch, int board_type, int minrel)
{
  int c = -1, myclan = -1;
  if (!LR_FAIL(ch, LVL_CLAN_GOD)) // Gods are supreme.
    return 1;
  c = find_clan_by_id(board_info[board_type].vnum -(CLAN_ZONE * 100) + 1);
  myclan = find_clan_by_id(GET_CLAN(ch));
  if (!(CLAN_HASOPT(c, CO_BOARD)))
  { // Board is disabled. (clan enable board)
    send_to_char("This clan board is currently not operational.\r\n", ch);
    return (0);
  }
  if (myclan < 0) 
  { // Luser is not in a clan.
    send_to_char("But you're not even part of a clan!\r\n", ch);
    return (0);
  }
  if ((myclan != c) && (GET_CLAN_REL(c, myclan) < minrel))
  { // Luser is in another (not allied) clan.
    sprintf(buf, "I don't think %s would like that!\r\n", clan[c].name);
    send_to_char(buf, ch);
    return (0);
  }
  return (1);
}

int Board_write_message(int board_type, struct char_data * ch, char *arg, struct obj_data *board)
{
  char *tmstr;
#ifdef NO_LOCALTIME
  struct tm lt;
#else
  time_t ct;
#endif
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  if (IS_CLAN_BOARD(board_type))
  {
    if (Board_clan_check(ch, board_type, CLAN_REL_ALLIANCE) == 0)
      return (1);
  } else if (LR_FAIL_MAX(ch, WRITE_LVL(board_type))) {
    send_to_char("You are unable to write in the language used on this board.\r\n", ch);
    return (1);
  }
  if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES) {
    send_to_char("The board is full.\r\n", ch);
    return (1);
  }
  if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1) {
    send_to_char("The board is malfunctioning - sorry.\r\n", ch);
    basic_mud_log("SYSERR: Board: failed to find empty slot on write.");
    return (1);
  }
  /* skip blanks */
  skip_spaces(&arg);
  delete_doubledollar(arg);

  /* JE 27 Oct 95 - Truncate headline at 80 chars if it's longer than that */
  arg[80] = '\0';

  if (!*arg) {
    send_to_char("Messages must have a title!\r\n", ch);
    return (1);
  }
#ifndef NO_LOCALTIME
  ct = time(0);
  tmstr = (char *) asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
#else
  if (jk_localtime_now(&lt))
  {
    basic_mud_log("Error in jk_localtime_now [%s:%d]", __FILE__, __LINE__);
    strcpy(tmstr, "ERROR!");
  } else {
    tmstr = (char *) asctime(&lt);
  }
#endif

  sprintf(buf2, "(&7%s&n)", GET_NAME(ch));
  sprintf(buf, "%6.10s %-12s :: &0%s&n", tmstr, buf2, arg);
  NEW_MSG_INDEX(board_type).heading = str_dup(buf);
  NEW_MSG_INDEX(board_type).level = GET_LEVEL(ch);

  send_to_char("Write your message. "
               "(/s or @ on newline saves, /h for help)\r\n\r\n", ch);
  act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

  string_write(ch->desc, &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]),
		MAX_MESSAGE_LENGTH, board_type + BOARD_MAGIC, NULL);

  num_of_msgs[board_type]++;
  return (1);
}


int Board_show_board(int board_type, struct char_data * ch, char *arg, struct obj_data *board)
{
  int i;
  char tmp[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  char *boardDesc = board->short_description; 

  if (!ch->desc)
    return (0);

  one_argument(arg, tmp);

  if (!*tmp || !isname(tmp, board->name))
    return (0);

  if (IS_CLAN_BOARD(board_type))
  {
    if (LR_FAIL(ch, LVL_CLAN_GOD))
    {
      if (Board_clan_check(ch, board_type, CLAN_REL_PEACEFUL) == 0)
	return 1;
    }
  } else if (LR_FAIL_MAX(ch, READ_LVL(board_type))) {
    send_to_char("You try but fail to understand the holy words.\r\n", ch);
    return (1);
  }
  sprintf(buf, "$n studies %s.", boardDesc); 
  act(buf, TRUE, ch, 0, 0, TO_ROOM); 

  strcpy(buf,
         "&g[ Primal Message System ]&n\r\n"
         "&y         Usage: &cRead&n or &cremove&n <messg #>\r\n"
         "                &cWrite&n <title>\r\n"
         "You will need to look at this again to save your message.\r\n");
  // Higher level info on boards
  if(!LR_FAIL(ch, LVL_IS_GOD))
  {
    sprintf(buf1, "( &c%d&n others have sought wisdom here ) \r\n", GET_OBJ_VAL(board, 0));
    strcat(buf, buf1);
  }
  GET_OBJ_VAL(board, 0)++;
 
  if (!num_of_msgs[board_type])
  {
    sprintf(buf1, "%s is empty.\r\n", boardDesc);
    strcat(buf, buf1);
  } else {
    sprintf(buf + strlen(buf), "There are &W%d&n messages on %s.\r\n",
            num_of_msgs[board_type], boardDesc); 
#if NEWEST_AT_TOP
    for (i = num_of_msgs[board_type] - 1; i >= 0; i--)
#else
    for (i = 0; i < num_of_msgs[board_type]; i++)
#endif
    {
      if (MSG_HEADING(board_type, i))
#if NEWEST_AT_TOP
	sprintf(buf + strlen(buf), "%-2d : %s\r\n",
		num_of_msgs[board_type] - i, MSG_HEADING(board_type, i));
#else
	sprintf(buf + strlen(buf), "%-2d : %s\r\n", i + 1, MSG_HEADING(board_type, i));
#endif
      else {
	basic_mud_log("SYSERR: The board is fubar'd.");
	send_to_char("Sorry, the board isn't working.\r\n", ch);
	return (1);
      }
    }
  }
  page_string(ch->desc, buf, 1);

  return (1);
}


int Board_display_msg(int board_type, struct char_data * ch, char *arg, struct obj_data *board)
{
  char number[MAX_STRING_LENGTH], buffer[MAX_STRING_LENGTH];
  int msg, ind;

  one_argument(arg, number);
  if (!*number)
    return (0);
  if (isname(number, board->name))	/* so "read board" works */
    return (Board_show_board(board_type, ch, arg, board));
  if (strchr(number, '.'))	/* read 2.mail, look 2.sword */
    return (0);
  if (!isdigit(*number) || (!(msg = atoi(number))))
    return (0);

  if (IS_CLAN_BOARD(board_type))
  {
    if (Board_clan_check(ch, board_type, CLAN_REL_PEACEFUL) == 0)
      return (1);
  } else if (LR_FAIL_MAX(ch, READ_LVL(board_type))) {
    send_to_char("You try but fail to understand the holy words.\r\n", ch);
    return (1);
  }
  if (!num_of_msgs[board_type]) {
    send_to_char("The board is empty!\r\n", ch);
    return (1);
  }
  if (msg < 1 || msg > num_of_msgs[board_type]) {
    send_to_char("That message exists only in your imagination.\r\n",
		 ch);
    return (1);
  }
#if NEWEST_AT_TOP
  ind = num_of_msgs[board_type] - msg;
#else
  ind = msg - 1;
#endif
  if (MSG_SLOTNUM(board_type, ind) < 0 ||
      MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE) {
    send_to_char("Sorry, the board is not working.\r\n", ch);
    basic_mud_log("SYSERR: Board is screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
    return (1);
  }
  if (!(MSG_HEADING(board_type, ind))) {
    send_to_char("That message appears to be screwed up.\r\n", ch);
    return (1);
  }
  if (!(msg_storage[MSG_SLOTNUM(board_type, ind)])) {
    send_to_char("That message seems to be empty.\r\n", ch);
    return (1);
  }
  sprintf(buffer, "Message &y%d&n : %s\r\n\r\n%s\r\n", msg,
          MSG_HEADING(board_type, ind),
          msg_storage[MSG_SLOTNUM(board_type, ind)]); 

  page_string(ch->desc, buffer, 1);

  return (1);
}


int Board_remove_msg(int board_type, struct char_data * ch, char *arg, struct obj_data *board)
{
  int ind, msg, slot_num;
  char number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct descriptor_data *d;

  one_argument(arg, number);

  if (!*number || !isdigit(*number))
    return (0);
  if (!(msg = atoi(number)))
    return (0);

  if (!num_of_msgs[board_type]) {
    send_to_char("The board is empty!\r\n", ch);
    return (1);
  }
  if (msg < 1 || msg > num_of_msgs[board_type]) {
    send_to_char("That message exists only in your imagination.\r\n", ch);
    return (1);
  }
#if NEWEST_AT_TOP
  ind = num_of_msgs[board_type] - msg;
#else
  ind = msg - 1;
#endif
  if (!MSG_HEADING(board_type, ind)) {
    send_to_char("That message appears to be screwed up.\r\n", ch);
    return (1);
  }
  sprintf(buf, "(&7%s&n)", GET_NAME(ch));

  if (IS_CLAN_BOARD(board_type))
  {
    int c = find_clan_by_id(1+board_info[board_type].vnum - (CLAN_ZONE * 100));

    if (((LR_FAIL(ch, LVL_CLAN_GOD)) &&
	 !(strstr(MSG_HEADING(board_type, ind), buf)) &&
	 ((find_clan_by_id(GET_CLAN(ch)) != c) ||
	  (GET_CLAN_RANK(ch) < clan[c].ranks)))) {
      send_to_char("Only the clan leader may remove other peoples post.\r\n",
	           ch);
      return (1);
    }
  } else if (LR_FAIL(ch, REMOVE_LVL(board_type)) &&
             !(strstr(MSG_HEADING(board_type, ind), buf))) {
    send_to_char("You are not holy enough to remove other people's messages.\r\n", ch);
    return (1);
  } else if (GET_LEVEL(ch) < MSG_LEVEL(board_type, ind) && (GET_LEVEL(ch) < LVL_IMPL) ) {
    send_to_char("You can't remove a message holier than yourself.\r\n", ch);
    return (1);
  } 
  slot_num = MSG_SLOTNUM(board_type, ind);
  if (slot_num < 0 || slot_num >= INDEX_SIZE) {
    send_to_char("That message is majorly screwed up.\r\n", ch);
    basic_mud_log("SYSERR: The board is seriously screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
    return (1);
  }
  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_PLAYING && d->str == &(msg_storage[slot_num])) {
      send_to_char("It's still being written!\r\n", ch); 
      return (1);
    }
  if (msg_storage[slot_num])
    free(msg_storage[slot_num]);
  msg_storage[slot_num] = 0;
  msg_storage_taken[slot_num] = 0;
  if (MSG_HEADING(board_type, ind))
    free(MSG_HEADING(board_type, ind));

  for (; ind < num_of_msgs[board_type] - 1; ind++) {
    MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
    MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
    MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
  }
  num_of_msgs[board_type]--;
  send_to_char("&BMessage removed.&n\r\n", ch);
  sprintf(buf, "$n just removed message %d.", msg);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  Board_save_board(board_type);

  return (1);
}


void Board_save_board(int board_type)
{
  FILE *fl;
  int i;
  char *tmp1, *tmp2 = NULL;

  if (!num_of_msgs[board_type]) {
    remove(FILENAME(board_type));
    return;
  }
  if (!(fl = fopen(FILENAME(board_type), "wb"))) {
    perror("SYSERR: Error writing board");
    return;
  }
  fwrite(&(num_of_msgs[board_type]), sizeof(int), 1, fl);

  for (i = 0; i < num_of_msgs[board_type]; i++) {
    if ((tmp1 = MSG_HEADING(board_type, i)) != NULL)
      msg_index[board_type][i].heading_len = strlen(tmp1) + 1;
    else
      msg_index[board_type][i].heading_len = 0;

    if (MSG_SLOTNUM(board_type, i) < 0 ||
	MSG_SLOTNUM(board_type, i) >= INDEX_SIZE ||
	(!(tmp2 = msg_storage[MSG_SLOTNUM(board_type, i)])))
      msg_index[board_type][i].message_len = 0;
    else
      msg_index[board_type][i].message_len = strlen(tmp2) + 1;

    fwrite(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
    if (tmp1)
      fwrite(tmp1, sizeof(char), msg_index[board_type][i].heading_len, fl);
    if (tmp2)
      fwrite(tmp2, sizeof(char), msg_index[board_type][i].message_len, fl);
  }

  fclose(fl);
}


void Board_load_board(int board_type)
{
  FILE *fl;
  int i, len1, len2;
  char *tmp1, *tmp2;

  if (!(fl = fopen(FILENAME(board_type), "rb"))) {
    if (errno != ENOENT)
      perror("SYSERR: Error reading board");
    return;
  }
  fread(&(num_of_msgs[board_type]), sizeof(int), 1, fl);
  if (num_of_msgs[board_type] < 1 || num_of_msgs[board_type] > MAX_BOARD_MESSAGES) {
    basic_mud_log("SYSERR: Board file %d corrupt.  Resetting.", board_type);
    Board_reset_board(board_type);
    return;
  }
  for (i = 0; i < num_of_msgs[board_type]; i++) {
    fread(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
    if ((len1 = msg_index[board_type][i].heading_len) <= 0) {
      basic_mud_log("SYSERR: Board file %d corrupt!  Resetting.", board_type);
      Board_reset_board(board_type);
      return;
    }
    CREATE(tmp1, char, len1);
    fread(tmp1, sizeof(char), len1, fl);
    MSG_HEADING(board_type, i) = tmp1;

    if ((MSG_SLOTNUM(board_type, i) = find_slot()) == -1) {
      basic_mud_log("SYSERR: Out of slots booting board %d!  Resetting...", board_type);
      Board_reset_board(board_type);
      return;
    }
    if ((len2 = msg_index[board_type][i].message_len) > 0) {
      CREATE(tmp2, char, len2);
      fread(tmp2, sizeof(char), len2, fl);
      msg_storage[MSG_SLOTNUM(board_type, i)] = tmp2;
    } else
      msg_storage[MSG_SLOTNUM(board_type, i)] = NULL;
  }

  fclose(fl);
}


void Board_reset_board(int board_type)
{
  int i;

  for (i = 0; i < MAX_BOARD_MESSAGES; i++) {
    if (MSG_HEADING(board_type, i))
      free(MSG_HEADING(board_type, i));
    if (msg_storage[MSG_SLOTNUM(board_type, i)])
      free(msg_storage[MSG_SLOTNUM(board_type, i)]);
    msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
    memset((char *)&(msg_index[board_type][i]),0,sizeof(struct board_msginfo));
    msg_index[board_type][i].slot_num = -1;
  }
  num_of_msgs[board_type] = 0;
  remove(FILENAME(board_type));
}
