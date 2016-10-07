/* ************************************************************************
*   File: modify.c                                      Part of CircleMUD *
*  Usage: Run-time modification of game variables                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <list>
#include <fstream>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "improved-edit.h"
#include "oasis.h"
#include "tedit.h"
#include "reports.h"
#include "genwld.h" /* Used in do_odd_write - ARTUS */
#include "genmob.h" // ""
#include "genzon.h" // ""
#include "clan.h"

void show_string(struct descriptor_data *d, char *input);

extern struct spell_info_type spell_info[];
extern const char *MENU;
extern const char *unused_spellname;	/* spell_parser.c */
extern struct room_data *world;	        /* array of rooms		 */
extern struct zone_data *zone_table;	/* zone table			 */
int is_colour(struct char_data *ch, char code, bool colour_code_only);

/* local functions */
void smash_tilde(char *str);
ACMD(do_skillset);
char *next_page(char *str, int page_width, int page_length);
int count_pages(char *str, int page_width, int page_length);
void paginate_string(char *str, struct descriptor_data *d);
void playing_string_cleanup(struct descriptor_data *d, int action);
void exdesc_string_cleanup(struct descriptor_data *d, int action);
void do_odd_write(struct descriptor_data *d, ubyte wtype, int maxlen); // ARTUS
void odd_write_cleanup(struct descriptor_data *d, int action);
void carbon_copy(struct descriptor_data *d, char *msg, int ccsize);

const char *string_fields[] =
{
  "name",
  "short",
  "long",
  "description",
  "title",
  "delete-description",
  "\n"
};


/* maximum length for text field x+1 */
int length[] =
{
  15,
  60,
  256,
  240,
  60
};

// TODO: move to more appropiate place
void odd_write_cleanup(struct descriptor_data *d, int action)
{
  if (action == STRINGADD_ABORT) {
    SEND_TO_Q("Description aborted.\r\n", d);
    return;
  } 

  switch (d->character->player_specials->write_type) {
  case PWT_REPORT_LONGDESC:
    report_string_cleanup(d, action);
    break;

  case PWT_CLANDESC:
    // DM: quick fix for *d->str being NULL ... 
    if (*d->str == NULL) {
      strcpy(clan[find_clan_by_id(GET_CLAN(d->character))].description, ""); 
    } else {
      strncpy(clan[find_clan_by_id(GET_CLAN(d->character))].description, 
                *d->str, CLAN_DESC_LENGTH);
    }
    SEND_TO_Q("Description saved.\r\n", d);
    if ((d->character) && LR_FAIL(d->character, LVL_CLAN_GOD))
      GET_GOLD(d->character) -= CLC_SETDESC;
    save_clans();
    break;
  case PWT_ROOM:
    if (d->character->player_specials->write_extra >= 1) {
      free(world[d->character->player_specials->write_extra].description);
      // DM: another quick fix for *d->str being NULL
      if (*d->str == NULL) {
        world[d->character->player_specials->write_extra].description = strdup(""); 
      } else {
        world[d->character->player_specials->write_extra].description = 
            strdup(*d->str);
      }
      if (save_rooms(world[d->character->player_specials->write_extra].zone)) {
        // TODO: tidy this up ...
        if ((d->character) && 
            zone_table[world[d->character->in_room].zone].number == CLAN_ZONE &&
            LR_FAIL(d->character, LVL_CLAN_GOD))
          GET_GOLD(d->character) -= CLC_MOBDESC;
        SEND_TO_Q("Description saved.\r\n", d);
      } else {
        SEND_TO_Q("Unable to save zone file.\r\n", d);
      }
      d->character->player_specials->write_extra = 0;
    } else {
      SEND_TO_Q("Unable to set room description at this time.\r\n", d);
    }
    break;
  case PWT_MOB:
    if (d->character->player_specials->write_extra >= 1) {
      extern struct index_data *mob_index;
      extern struct zone_data *zone_table;
      extern struct char_data *character_list;
      extern struct char_data *mob_proto;
      int update_mobile_strings(struct char_data *t, struct char_data *f);
      struct char_data *mob, *live_mob;
      mob = &mob_proto[d->character->player_specials->write_extra];
      free(mob->player.description);
      if (*d->str == NULL) {
        mob->player.description = str_dup(NULL);
      } else {
        mob->player.description = str_dup(*d->str);
      }

      if (save_mobiles(real_zone(zone_table[real_zone_by_thing(mob_index[mob->nr].vnum)].number)) != FALSE)
        SEND_TO_Q("Description saved.\r\n", d);

      if (LR_FAIL(d->character, LVL_CLAN_GOD))
        GET_GOLD(d->character) -= CLC_MOBDESC;
      else
        SEND_TO_Q("Unable to save zone file.\r\n", d);

      for (live_mob = character_list; live_mob; live_mob = live_mob->next)
        if (mob->nr == live_mob->nr)
          update_mobile_strings(live_mob, &mob_proto[mob->nr]);

    } else {
      SEND_TO_Q("Unable to set mob description at this time.\r\n", d);
    }
    break;
  default:
    SEND_TO_Q("Looks like a bug, please report.\r\n", d);
    break;
  }
}

/* ************************************************************************
*  modification of malloc'ed strings                                      *
************************************************************************ */

/*
 * Put '#if 1' here to erase ~, or roll your own method.  A common idea
 * is smash/show tilde to convert the tilde to another innocuous character
 * to save and then back to display it. Whatever you do, at least keep the
 * function around because other MUD packages use it, like mudFTP.
 *   -gg 9/9/98
 */
void smash_tilde(char *str)
{
#if 1
  /*
   * Erase any ~'s inserted by people in the editor.  This prevents anyone
   * using online creation from causing parse errors in the world files.
   * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
   * his tildes thank you very much.), -gg 2/20/98
   */
    while ((str = strchr(str, '~')) != NULL)
      *str = ' ';
#endif
}

/* 
 * Simplified Internal Editor - It doesn't get much better than this.
 * Wtypes should be defined as PWT_ in structs.h, add in something in
 * the wtype_descs array too. Could be used in stat later on *shrugs*..
 * Either way, this will have it's uses. Of course, you'll need to modify
 * the string_add function and add in stuff to handle what happens with
 * the string. - ARTUS 
 */
void do_odd_write(struct descriptor_data *d, ubyte wtype, int maxlen)
{
  d->backstr = NULL;
  d->character->player_specials->write_type = wtype;
  SET_BIT(PLR_FLAGS(d->character), PLR_ODDWRITE | PLR_WRITING);
  d->max_str = maxlen;
  d->str = (char **) malloc(sizeof(char *));
  *(d->str) = NULL;
  d->mail_to = 0;
}

/*
 * Basic API function to start writing somewhere.
 *
 * 'data' isn't used in stock CircleMUD but you can use it to pass whatever
 * else you may want through it.  The improved editor patch when updated
 * could use it to pass the old text buffer, for instance.
 */
void string_write(struct descriptor_data *d, char **writeto, size_t len, long mailto, void *data)
{
  if (d->character && !IS_NPC(d->character))
    SET_BIT(PLR_FLAGS(d->character), PLR_WRITING);

  if (using_improved_editor)
    d->backstr = (char *)data;
  else if (data)
    free(data);

  d->str = writeto;
  d->max_str = len;
  d->mail_to = mailto;
}

/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str)
{
  int action;

  delete_doubledollar(str);
  smash_tilde(str);

  /* determine if this is the terminal string, and truncate if so */
  /* changed to only accept '@' at the beginning of line - J. Elson 1/17/94 */
  if ((action = (*str == '@')))
    *str = '\0';
  else
    if ((action = improved_editor_execute(d, str)) == STRINGADD_ACTION)
      return;

  if (!(*d->str) && action == STRINGADD_OK) {
    if (strlen(str) + /*5*/3 > d->max_str) {
      send_to_char("String too long - Truncated.\r\n", d->character);
      //str[d->max_str-4] = '&';
      //str[d->max_str-3] = 'n';
      str[d->max_str-2] = '\r';
      str[d->max_str-1] = '\n';
      str[d->max_str] = '\0';
      
      // DM - added the following two lines that where missing from the base
      // code!
      CREATE(*d->str, char, d->max_str);
      strcpy(*d->str, str);

      if (!using_improved_editor)
        action = STRINGADD_SAVE;

    } else {
      CREATE(*d->str, char, strlen(str) + /*5*/3);
      strcpy(*d->str, str);
    }
  } else if (action == STRINGADD_OK) {
    if (strlen(str) + strlen(*d->str) + /*5*/3 > d->max_str) { 
      send_to_char("String too long.  Last line skipped.\r\n", d->character);

      return;

      if (!using_improved_editor)
        action = STRINGADD_SAVE;
    } else {
      RECREATE(*d->str, char, strlen(*d->str) + strlen(str) + /*5*/3); /* &n\r\n\0 */
      strcat(*d->str, str);
    }
  }

  /*
   * Common cleanup code.
   */
  switch (action) {
    case STRINGADD_ABORT:
      switch (STATE(d)) {
        case CON_TEDIT:
        case CON_REDIT:
        case CON_MEDIT:
        case CON_OEDIT:
        case CON_TRIGEDIT:
        case CON_EXDESC:
        case CON_REPORT_ADD:
        case CON_REPORT_EDIT:
          free(*d->str);
          *d->str = d->backstr;
          d->backstr = NULL;
          d->str = NULL;
          break;
        default:
          basic_mud_log("SYSERR: string_add: Aborting write from unknown origin.");
          break;
      }
      break;
    case STRINGADD_SAVE:
      if (d->str && *d->str && **d->str == '\0') {
        free(*d->str);
        *d->str = str_dup("Nothing.\r\n");
      }
      if (d->backstr)
        free(d->backstr);
      d->backstr = NULL;
      break;
  }

  /* Ok, now final cleanup. */

  if (action) {
    int i;
    struct {
      int mode; 
      void (*func)(struct descriptor_data *d, int action);
    } cleanup_modes[] = {
      { CON_MEDIT  , medit_string_cleanup },
      { CON_OEDIT  , oedit_string_cleanup },
      { CON_REDIT  , redit_string_cleanup },
      { CON_TEDIT  , tedit_string_cleanup },
      { CON_TRIGEDIT,trigedit_string_cleanup },
      { CON_EXDESC , exdesc_string_cleanup },
      { CON_PLAYING, playing_string_cleanup },
      { -1, NULL }
    };

    /* Clan Description Stuff.. - ARTUS */
    if (PLR_FLAGGED(d->character, PLR_ODDWRITE)) {
      odd_write_cleanup(d, action);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_ODDWRITE);
    }

    for (i = 0; cleanup_modes[i].func; i++)
      if (STATE(d) == cleanup_modes[i].mode)
        (*cleanup_modes[i].func)(d, action);

    /* Common post cleanup code. */
    d->str = NULL;
    d->mail_to = 0;
    d->max_str = 0;
    if (d->character && !IS_NPC(d->character)) {
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_GODMAIL);

      // DM: Save player data - if they dont re-enter game when entering
      //     description, changes are lost ...
      if (STATE(d) == CON_EXDESC) {
        save_char(d->character, NOWHERE);
      }
    }
  } else if (strlen(*d->str) <= (d->max_str - /*5*/3)) /* 5 = &n\r\n\0 */
    strcat(*d->str, /*"&n\r\n"*/"\r\n");
}

void playing_string_cleanup(struct descriptor_data *d, int action)
{
  char *m;
  unsigned int count = 0;

  if (PLR_FLAGGED(d->character, PLR_MAILING))
  {
    if (action == STRINGADD_SAVE && *d->str)
    {
      /* Check for godmail */
      if (PLR_FLAGGED(d->character, PLR_GODMAIL))
      {
        // List the gods to send mail to, here. Ignore SANDII, she is default 
        // GodMailto: Dangermouse
        if (get_id_by_name("Dangermouse")> 0 )
          store_mail(get_id_by_name("Dangermouse"), 
                          GET_IDNUM(d->character), *d->str);
        // GodMailto:Artus
        if (get_id_by_name("Artus") > 0)
          store_mail(get_id_by_name("Artus"), GET_IDNUM(d->character), *d->str);

	// GodMailto:Rhcp
	if (get_id_by_name("Rhcp") > 0)
	  store_mail(get_id_by_name("Rhcp"), GET_IDNUM(d->character), *d->str);
      }
      /* Multi mail via CC: implementation */
      m = *d->str;
      if (LOWER(m[0]) == 'c' && LOWER(m[1]) == 'c' && m[2] == ':')
      {
        //i = 3; // Skip the 'CC:'
        // Get the names
        while ((m[count] != '.') && (count < strlen(m)) ) 
          count++;
        // If we found a full stop, CC the message
        if (count <= strlen(m))
          carbon_copy(d, m, count);   // CC the message
        else
          send_to_char("Carbon copy failed: No '.' found in list.\r\n", 
                          d->character);
      }
      // if godmail ... GodMailto: Sandii
      store_mail(d->mail_to, GET_IDNUM(d->character), *d->str);
      SEND_TO_Q("Message sent!\r\n", d);
    } else {
      sprintf(buf, "Mail aborted, I'll only give you back &Y%d&n "
                   "coins for your hassle.\r\n", STAMP_PRICE / 2);
      SEND_TO_Q(buf, d);
      GET_GOLD(d->character) += (STAMP_PRICE / 2);
    }
    free(*d->str);
    free(d->str);
  } 

  /*
   * We have no way of knowing which slot the post was sent to so we can only give the message...
   */
  if (d->mail_to >= BOARD_MAGIC)
  {
    Board_save_board(d->mail_to - BOARD_MAGIC);
    if (action == STRINGADD_ABORT)
      SEND_TO_Q("Post not aborted, use REMOVE <post #>.\r\n", d);
  }
}

void exdesc_string_cleanup(struct descriptor_data *d, int action)
{
  if (action == STRINGADD_ABORT)
    SEND_TO_Q("Description aborted.\r\n", d);

  SEND_TO_Q(MENU, d);
  STATE(d) = CON_MENU;
}

/* **********************************************************************
*  Modification of character skills                                     *
********************************************************************** */

ACMD(do_skillset)
{
  struct char_data *vict;
  char name[MAX_INPUT_LENGTH], buf2[128];
  char buf[MAX_INPUT_LENGTH], help[MAX_STRING_LENGTH];
  int skill, value, i, qend;

  argument = one_argument(argument, name);

  if (!*name) {			/* no arguments. print an informative text */
    send_to_char("Syntax: skillset <name> '<skill>' <value>\r\n", ch);
    strcpy(help, "Skill being one of the following:\r\n");
    for (qend = 0, i = 0; i <= TOP_SPELL_DEFINE; i++) {
      if (spell_info[i].name == unused_spellname)	/* This is valid. */
	continue;
      sprintf(help + strlen(help), "%18s", spell_info[i].name);
      if (qend++ % 4 == 3) {
	strcat(help, "\r\n");
	send_to_char(help, ch);
	*help = '\0';
      }
    }
    if (*help)
      send_to_char(help, ch);
    send_to_char("\r\n", ch);
    return;
  }

  if (!(vict = generic_find_char(ch, name, FIND_CHAR_WORLD)))
  {
    send_to_char(NOPERSON, ch);
    return;
  }
  skip_spaces(&argument);

  /* If there is no chars in argument */
  if (!*argument) {
    send_to_char("Skill name expected.\r\n", ch);
    return;
  }
  if (*argument != '\'') {
    send_to_char("Skill must be enclosed in: ''\r\n", ch);
    return;
  }
  /* Locate the last quote and lowercase the magic words (if any) */

  for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
    argument[qend] = LOWER(argument[qend]);

  if (argument[qend] != '\'') {
    send_to_char("Skill must be enclosed in: ''\r\n", ch);
    return;
  }
  strcpy(help, (argument + 1));
  help[qend - 1] = '\0';
  if ((skill = find_skill_num(help)) <= 0) {
    send_to_char("Unrecognized skill.\r\n", ch);
    return;
  }
  argument += qend + 1;		/* skip to next parameter */
  argument = one_argument(argument, buf);

  if (!*buf) {
    send_to_char("Learned value expected.\r\n", ch);
    return;
  }
  value = atoi(buf);
  if (value < 0) {
    send_to_char("Minimum value for learned is 0.\r\n", ch);
    return;
  }
  if (value > 100) {
    send_to_char("Max value for learned is 100.\r\n", ch);
    return;
  }
  if (IS_NPC(vict)) {
    send_to_char("You can't set NPC skills.\r\n", ch);
    return;
  }

  /*
   * find_skill_num() guarantees a valid spell_info[] index, or -1, and we
   * checked for the -1 above so we are safe here.
   */
  sprintf(buf2, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict),
	  spell_info[skill].name, value);
  mudlog(buf2, BRF, -1, TRUE);

  SET_SKILL(vict, skill, value);

  sprintf(buf2, "You change %s's %s to %d.\r\n", GET_NAME(vict),
	  spell_info[skill].name, value);
  send_to_char(buf2, ch);
}



/*********************************************************************
* New Pagination Code
* Michael Buselli submitted the following code for an enhanced pager
* for CircleMUD.  All functions below are his.  --JE 8 Mar 96
*
*********************************************************************/

// #define PAGE_LENGTH     22
// #define PAGE_WIDTH      80

/* Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
char *next_page(char *str, int page_width, int page_length)
{
  int col = 1, line = 1, spec_code = FALSE;

  for (;; str++) {
    /* If end of string, return NULL. */
    if (*str == '\0')
      return (NULL);

    /* If we're at the start of the next page, return this fact. */
    else if (line > page_length) 
      return (str);

    /* Check for the begining of an ANSI color code block. */
    else if (*str == '\x1B' && !spec_code)
      spec_code = TRUE;

    /* Check for the end of an ANSI color code block. */
    else if (*str == 'm' && spec_code)
      spec_code = FALSE;

    /* Check for everything else. */
    else if (!spec_code) {

      // DM - easy colour codes
      //
      // TODO: missing "&<spec_code>" check ...
      
      if (*str == '&') {
	// a colour code "&<colour/number code>"
	// skip leading '&' and check following characters by continuing loop
	// is_colour returns 0 for n
        if (*(str+1) == 'n' || is_colour(NULL, *(str+1), FALSE)) {
          *str++;
	  continue;
	}
	// "&&", "&\" - skip leading '&' and continue processing ...
	if (*(str+1) == '&' || *(str+1) == '\\')
	  *str++;
      }

      /* Carriage return puts us in column one. */
      if (*str == '\r')
	col = 1;
      /* Newline puts us on the next line. */
      else if (*str == '\n')
	line++;

      /* We need to check here and see if we are over the page width,
       * and if so, compensate by going to the begining of the next line.
       */
      else if (col++ > page_width) {
	col = 1;
	line++;
      }
    }
  }
}


/* Function that returns the number of pages in the string. */
int count_pages(char *str, int page_width, int page_length)
{
  int pages;

  for (pages = 1; (str = next_page(str,page_width,page_length)); pages++);
  return (pages);
}


/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(char *str, struct descriptor_data *d)
{
  int i, lw=80, pl=22;
  
  if (d->character && d->character->player_specials) {
    lw=GET_PAGE_WIDTH(d->character);
    pl=GET_PAGE_LENGTH(d->character);
  }

  if (d->showstr_count)
    *(d->showstr_vector) = str;

  for (i = 1; i < d->showstr_count && str; i++)
    str = d->showstr_vector[i] = next_page(str,lw,pl);

  d->showstr_page = 0;
}


/* The call that gets the paging ball rolling... */
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  int lw=80, pl=22;
  struct char_data *realch = NULL;

  if (!d)
    return;

  realch = (d->original) ? d->original : d->character;

  if (realch) {
    lw = MIN(250, MAX(40, GET_PAGE_WIDTH(realch)));  // 40 - 250
    pl = MIN(100, MAX(10, GET_PAGE_LENGTH(realch))); // 10 - 100
  }

  if (!str || !*str) {
    send_to_char("", d->character);
    return;
  }
  if (strlen(str) > MAX_STRING_LENGTH) {
    send_to_char("&RWarning, buffer length exceeded, output will be "
                    "corrupt.&n\r\n", realch);          
  }

  d->showstr_count = count_pages(str,lw,pl);
  CREATE(d->showstr_vector, char *, d->showstr_count);

  if (keep_internal) {
    d->showstr_head = str_dup(str);
    paginate_string(d->showstr_head, d);
  } else
    paginate_string(str, d);

  show_string(d, "");
}


/* The call that displays the next page. */
void show_string(struct descriptor_data *d, char *input)
{
  char buffer[MAX_STRING_LENGTH];
  int diff;

  any_one_arg(input, buf);

  /* Q is for quit. :) */
  if (LOWER(*buf) == 'q') {
    free(d->showstr_vector);
    d->showstr_count = 0;
    if (d->showstr_head) {
      free(d->showstr_head);
      d->showstr_head = NULL;
    }
    return;
  }
  /* R is for refresh, so back up one page internally so we can display
   * it again.
   */
  else if (LOWER(*buf) == 'r')
    d->showstr_page = MAX(0, d->showstr_page - 1);

  /* B is for back, so back up two pages internally so we can display the
   * correct page here.
   */
  else if (LOWER(*buf) == 'b')
    d->showstr_page = MAX(0, d->showstr_page - 2);

  /* Feature to 'goto' a page.  Just type the number of the page and you
   * are there!
   */
  else if (isdigit(*buf))
    d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));

  else if (*buf) {
    send_to_char(
		  "Valid commands while paging are RETURN, Q, R, B, or a numeric value.\r\n",
		  d->character);
    return;
  }
  /* If we're displaying the last page, just send it to the character, and
   * then free up the space we used.
   */
  if (d->showstr_page + 1 >= d->showstr_count) {
    send_to_char(d->showstr_vector[d->showstr_page], d->character);
    free(d->showstr_vector);
    d->showstr_count = 0;
    if (d->showstr_head) {
      free(d->showstr_head);
      d->showstr_head = NULL;
    }
  }
  /* Or if we have more to show.... */
  else {
    diff = d->showstr_vector[d->showstr_page + 1] - d->showstr_vector[d->showstr_page];
    if (diff >= MAX_STRING_LENGTH)
      diff = MAX_STRING_LENGTH - 1;
    strncpy(buffer, d->showstr_vector[d->showstr_page], diff);
    buffer[diff] = '\0';
    send_to_char(buffer, d->character);
    d->showstr_page++;
  }
}

void carbon_copy(struct descriptor_data *d, char *msg, int ccsize) {

  char target[MAX_NAME_LENGTH + 1];
  int counter = 0, curcount = 0;

  for (counter = 3; counter <= ccsize; counter++) {
  // Ignore extra spaces between names
    if (msg[counter] == ' ' && curcount == 0)
      continue;
    /* Check for colour codes
    if (msg[counter] == '&') {
      counter += 2;
      continue;
    } */
    if (msg[counter] == ' ' || msg[counter] == '.') { // End of name
      if (curcount == 0)
        return;  // All done
      if (curcount <= MAX_NAME_LENGTH + 1)
        target[curcount] = '\0'; // Close name
		
      if (get_id_by_name(target) >= 0 ) {
        // CC it
        store_mail(get_id_by_name(target) , GET_IDNUM(d->character), msg);
        sprintf(buf, "Carbon copy sent to %s.\r\n", target);
        SEND_TO_Q(buf, d);
      } else {
        sprintf(buf, "Failed to copy to %s.\r\n", target);
        SEND_TO_Q(buf, d);
      }
      curcount = 0;
      target[curcount] = '\0';
    } else {
      if (curcount <= MAX_NAME_LENGTH)
        target[curcount] = msg[counter];
      else if (curcount == MAX_NAME_LENGTH + 1)
        target[curcount] = '\0'; // End name regardless
      curcount++;
    }
  }
}
