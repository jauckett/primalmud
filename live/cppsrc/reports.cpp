/*
 * Reporting system - by DM
 *        
 * Quick background on expected usage and lifecycle of a report:
 * Initially when a report is created, the state is set to OPEN. When the
 * responsible person looks at the report, they should edit it and change the
 * state to ASSESSED (so the reporter knows the report has been looked at).
 * The responsible person should then fix/respond to the problem and change
 * the state to FEEDBACK. The reporter then verifies the fix/responce and
 * edits the report changing the state to CLOSED. Obvious exceptions apply to
 * a more complicated report, but the state principals as mentioned above
 * should be used consistantly as described.
 *
 * Every time that a report is edited, notification is given to the involved
 * parties (all people that have made a change to the initial report). This
 * notification is in the form of email if possible, and mudmail.
 *
 * Basic idea is to replace the existing bug/idea/typo and add todo stuff.
 * Reports are read from a random access file at boot time, and converted to
 * Report objects contained within a list. The reports stored to file with
 * state DELETED are ignored when converting to objects at boot time.
 *
 * As far as player usage goes, the following restrictions apply on report
 * usage:
 *
 * command      list/print                      edit/delete
 * -------
 * BUG          <  GOD - owned reports          <  GRGOD - owned reports
 *              >= GOD - all reports            >= GRGOD - all reports
 *
 * IDEA         <  ANGEL - owned reports        <  GRGOD - owned reports
 *              >= ANGEL - all reports          >= GRGOD - all reports
 *
 * TYPO         <  IMMORT - owned reports       <  GOD - owned reports
 *              >= IMMORT - all reports         >= GOD - all reports
 *
 * TODO         <  ANGEL - owned reports        <  GRGOD - owned reports
 *              >= ANGEL - all reports          >= GRGOD - all reports
 *
 * Additionally, only GRGOD+ can add TODO reports, and the following opetions
 * in add and edit states apply:
 * 
 * delete - < limit (owned) - deletion is only allowed if state is CLOSED 
 *          (The record will be kept in file with state DELETED)
 *
 * edit   - < limit (owned) - change of type is not allowed, 
 *    ORIGINAL THOUGHT - state change is restricted only to FEEDBACK -> CLOSED
 *    BUT NAH, DONT BOTHER .....
 *
 * add    - for BUG give bug type options only, for others (TYPO/IDEA/TODO)
 *          onlu display the the given type
 *
 * Well thats it for now I guess.... Hope it kicks arse? ;p 
 */

#include <list>
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "reports.h"
#include "comm.h"
#include "improved-edit.h"
#include "genolc.h"
#include "mail.h"

extern ReportList *reportList;
extern ReleaseInfo release;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern void do_odd_write(struct descriptor_data *d, ubyte wtype, int maxlen);

void report_string_cleanup(struct descriptor_data *d, int terminator)
{
  if (terminator == STRINGADD_ABORT)
  {
    SEND_TO_Q("Description aborted.\r\n\r\n", d);
  } else {
//    REPORT_MODIFIED(d) = 1;
    if (*d->str)
//      REPORT(d)->setLongDescription(*d->str);
;
    else
      // REPORT(d)->setLongDescription("");
;
  }
  switch (STATE(d))
  {
    case CON_REPORT_ADD:  reportList->dispAddMenu(d);  break;
    case CON_REPORT_EDIT: reportList->dispEditMenu(d); break;
  }
}

ACMD(do_reporting)
{
  int number;
  char arg1[MAX_INPUT_LENGTH], rest[MAX_INPUT_LENGTH];
  char usage[MAX_INPUT_LENGTH];

  sprintf(usage, "&1Usage: &4%s [  'add'\r" 
          "\n             | 'edit' <number> \r"
	  "\n             | 'delete' <number>\r"
	  "\n             | 'list' ['self' | <state> | <player>]\r"
	  "\n             | 'print' <number> ]&n\r\n",
	  cmd_info[cmd].command);

  half_chop(argument, arg1, rest);

  // DM - a new one "listreports" same as report list - but for all types
  if (subcmd == REPORT_MODE_LISTREPORT)
  {
    reportList->listReports(ch->desc, subcmd, arg1);
    return;
  }
  if (subcmd == REPORT_MODE_PRINTREPORT)
  {
    number = atoi(arg1);
    reportList->printReport(ch->desc, number, subcmd);
    return;
  }
  if (!*arg1)
  {
    send_to_char(usage, ch);
    return;
  }
  // report add - create a blank report, set states and report pointer then
  // display the add menu
  if (is_abbrev(arg1, "add"))
  {
    if (GET_LEVEL(ch) < LVL_GRGOD && subcmd == REPORT_MODE_TODO)
    {
      send_to_char("Sorry, you are unable to add a &4todo&n report.\r\n", ch);
      return;
    }
    Report *newReport = new Report();
    switch (subcmd)
    {
      case REPORT_MODE_BUG:  newReport->setType(REPORT_TYPE_BUGCRASH); break;
      case REPORT_MODE_IDEA: newReport->setType(REPORT_TYPE_IDEA);     break;
      case REPORT_MODE_TYPO: newReport->setType(REPORT_TYPE_TYPO);     break;
      case REPORT_MODE_TODO: newReport->setType(REPORT_TYPE_TODO);     break;
    }
    newReport->setReporterId(GET_IDNUM(ch));
    newReport->setPlayerId(GET_IDNUM(ch));
    newReport->setOrgTime(time(0));
    newReport->setOrgRoom(GET_ROOM_VNUM(ch->in_room));
    newReport->setRelease(release);
    act("$n begins adding a report.", TRUE, ch, NULL, NULL, TO_ROOM);
    STATE(ch->desc) = CON_REPORT_ADD;
//    REPORT(ch->desc) = (Report *) newReport;
//    REPORT_MODE(ch->desc) = subcmd;
    SET_BIT(PLR_FLAGS(ch), PLR_REPORTING);
    reportList->dispAddMenu(ch->desc);
    // report edit - show menu
  } else if (is_abbrev(arg1, "edit")) {
    half_chop(rest, arg1, rest);
    number = atoi(arg1);
    // need to create a copy of existing ...
    // then when we save, we copy details and free the temp ...
    // copyReport makes a copy of the existing report and sets the report
    // pointer of the descriptor. It also checks to see that no-one is editing
    // that report.
    if (!reportList->copyReport(ch->desc, number, subcmd))
      return;
    act("$n begins editing a report.", TRUE, ch, NULL, NULL, TO_ROOM);
    SET_BIT(PLR_FLAGS(ch), PLR_REPORTING);
    STATE(ch->desc) = CON_REPORT_EDIT;
    reportList->dispEditMenu(ch->desc);
    // report delete <number>
  } else if (is_abbrev(arg1, "delete")) {
    half_chop(rest, arg1, rest);
    number = atoi(arg1);
    reportList->removeReport(ch->desc, number, subcmd);
    // report list [filter] 
  } else if (is_abbrev(arg1, "list")) {
    half_chop(rest, arg1, rest);
    reportList->listReports(ch->desc, subcmd, arg1);
    // report print <number>
  } else if (is_abbrev(arg1, "print")) {
    half_chop(rest, arg1, rest);
    number = atoi(arg1);
    reportList->printReport(ch->desc, number, subcmd);
  } else {
    sprintf(buf, usage, cmd_info[cmd].command);
    send_to_char(buf, ch);
    return;
  }
}

void
ReportList::dispAddMenu(struct descriptor_data *d)
{
  char dateString[25], typeString[20], stateString[20];
  time_t dispTime = REPORT(d)->getOrgTime();
#ifdef NO_LOCALTIME
  struct tm lt;
#endif

  REPORT_STATE(d) = REPORT_STATE_MENU;
#ifndef NO_LOCALTIME
  strncpy(dateString, (char *) asctime(localtime(&dispTime)), 25);
#else
  jk_localtime(&lt, dispTime);
  strncpy(dateString, (char *) asctime(&lt), 25);
#endif
  dateString[24] = '\0';
  sprinttype(REPORT(d)->getState(), report_states, stateString);
  sprinttype(REPORT(d)->getType(), report_types, typeString);
  sprintf(buf, "\r\n-- Report Number: [&cUNASSIGNED&n] Reporter: [&7%s&n]"
	       " Room: [&8%d&n]\r\n"
	       "-- Date: [&g%s&n] Revision: [&c%d.%d.%d&n] State: [&4%s&n]\r\n"
	       "&g1&n) S-Desc : &0%s\r\n"
	       "&g2&n) L-Desc : %s\r\n",
	  get_name_by_id(REPORT(d)->getReporterId()),
	  (int) REPORT(d)->getOrgRoom(), dateString, release.getMajor(),
	  release.getBranch(), release.getMinor(), stateString,
	  REPORT(d)->getShortDescription(), REPORT(d)->getLongDescription());
  if (!REPORT(d)->isRestricted(d, REPORT_MODE(d)))
    sprintf(buf+strlen(buf), "&g3&n) State  : &c%s\r\n", stateString);
  if (REPORT_MODE(d) == REPORT_MODE_BUG || 
      !REPORT(d)->isRestricted(d, REPORT_MODE(d)))
    sprintf(buf+strlen(buf), "&g4&n) Type   : &c%s\r\n", typeString);
  if (!REPORT(d)->isRestricted(d, REPORT_MODE(d)))
    sprintf(buf+strlen(buf), "&g5&n) Player : &7%s&n\r\n", 
            get_name_by_id(REPORT(d)->getPlayerId()));
  strcat(buf, "&gQ&n) Quit\r\n" "&nEnter Choice : ");

  SEND_TO_Q(buf, d);
}

void
ReportList::dispEditMenu(struct descriptor_data *d)
{
  char
    dateString[25], typeString[20], stateString[20];
  time_t dispTime = REPORT(d)->getOrgTime();
#ifdef NO_LOCALTIME
  struct tm lt;
#endif

  REPORT_STATE(d) = REPORT_STATE_MENU;
#ifndef NO_LOCALTIME
  strncpy(dateString, (char *) asctime(localtime(&dispTime)), 25);
#else
  jk_localtime(&lt, dispTime);
  strncpy(dateString, (char *) asctime(&lt), 25);
#endif
  dateString[24] = '\0';
  sprinttype(REPORT(d)->getState(), report_states, stateString);
  sprinttype(REPORT(d)->getType(), report_types, typeString);
  sprintf(buf, "\r\n-- Report Number: [&g%d&n] Reporter: [&7%s&n]"
	       " Room: [&8%d&n]\r\n"
	       "-- Date: [&g%s&n] Revision: [&c%d.%d.%d&n]\r\n"
	       "&g1&n) S-Desc : &0%s\r\n"
	       "&g2&n) L-Desc : %s\r\n",
	  REPORT(d)->getReportNum(), get_name_by_id(REPORT(d)->getReporterId()),
	  (int) REPORT(d)->getOrgRoom(), dateString,
	  REPORT(d)->getOrgRelease().getMajor(),
	  REPORT(d)->getOrgRelease().getBranch(),
	  REPORT(d)->getOrgRelease().getMinor(),
	  REPORT(d)->getShortDescription(), REPORT(d)->getLongDescription());
  // allow player to change the type in bug mode, or if they do not have edit
  // restrictions imposed. 
  if (REPORT_MODE(d) == REPORT_MODE_BUG || 
      !REPORT(d)->isRestricted(d, REPORT_MODE(d)))
  {
    sprintf(buf+strlen(buf), "&g3&n) State  : &c%s\r\n" 
		             "&g4&n) Type   : &c%s\r\n",
            stateString, typeString);
    if (!REPORT(d)->isRestricted(d, REPORT_MODE(d)))
      sprintf(buf+strlen(buf), "&g5&n) Player : &7%s&n\r\n", 
	      get_name_by_id(REPORT(d)->getPlayerId()));
  } else
    sprintf(buf+strlen(buf), "&g3&n) State  : &c%s\r\n", stateString);

  sprintf(buf, "%s" "&gQ&n) Quit\r\n" "&nEnter Choice : ", buf);

  SEND_TO_Q(buf, d);
}

// Returns whether or not the char has restrictions imposed on editing/adding
// ie. The char is editing/adding this report, but unless they meet the level
// requirements, they are restricted in what they can edit. 
bool Report::isRestricted(struct descriptor_data *d, int mode)
{
  switch (mode)
  {
    case REPORT_MODE_TODO:
    case REPORT_MODE_IDEA:
    case REPORT_MODE_BUG:
      if (GET_LEVEL(d->character) >= LVL_GRGOD)
	return (FALSE);
      break;
    case REPORT_MODE_TYPO:
      if (GET_LEVEL(d->character) >= LVL_GOD)
	return (FALSE);
      break;
  }
  return (TRUE);
}

// Returns whether or not the char can list/print the given report
bool Report::isViewable(struct descriptor_data *d)
{
  switch (getReportType())
  {
    case REPORT_MODE_IDEA:
    case REPORT_MODE_TODO:
      if ((GET_LEVEL(d->character) < LVL_ANGEL) &&
	  (GET_IDNUM(d->character) != getPlayerId()) &&
          (GET_IDNUM(d->character) != getReporterId()))
	return (FALSE);
      return (TRUE);
    case REPORT_MODE_TYPO:
      if ((GET_LEVEL(d->character) < LVL_ISNOT_GOD) &&
          (GET_IDNUM(d->character) != getPlayerId()) &&
	  (GET_IDNUM(d->character) != getReporterId()))
	return (FALSE);
      return (TRUE);
    case REPORT_MODE_BUG:
      if ((GET_LEVEL(d->character) < LVL_GOD) &&
	  (GET_IDNUM(d->character) != getPlayerId()) &&
	  (GET_IDNUM(d->character) != getReporterId()))
	return (FALSE);
      return (TRUE);
  }
  return (FALSE);
}

// Returns whether the char can edit/delete the given report
bool Report::isEditable(struct descriptor_data * d, int mode)
{
  switch (mode)
  {
    // Bug/Idea/Todo Mode - restricted to owner, LVL_GRGOD
    case REPORT_MODE_BUG:
    case REPORT_MODE_IDEA:
    case REPORT_MODE_TODO:
      if ((GET_LEVEL(d->character) < LVL_GRGOD) &&
	  (GET_IDNUM(d->character) != getPlayerId()))
	return (FALSE);
      return (TRUE);

    // Typo Mode - restricted to owner, LVL_GOD
    case REPORT_MODE_TYPO:
      if ((GET_LEVEL(d->character) < LVL_GOD) &&
	  (GET_IDNUM(d->character) != getPlayerId()))
	return (FALSE);
      return (TRUE);
  }
  return (FALSE);
}

void
report_parse(struct descriptor_data *d, char *arg)
{
  int option;
  long playerId = 0;

  switch (REPORT_STATE(d)) {

    case REPORT_STATE_MENU:

    switch (*arg) {
      case 'q':
      case 'Q':
      if (REPORT_MODIFIED(d)) {
	SEND_TO_Q("Do you wish to save changes? : ", d);
	REPORT_STATE(d) = REPORT_STATE_CONFIRM;
      } else {
	delete(Report *) REPORT(d);
//	REPORT(d) = NULL;
	act("$n cancels adding a report.",
	    TRUE, d->character, NULL, NULL, TO_ROOM);
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_REPORTING);
	STATE(d) = CON_PLAYING;
      }
      return;

      case '1':
      REPORT_STATE(d) = REPORT_STATE_SHORTDESC;
      SEND_TO_Q("Enter short description : ", d);
      return;

      case '2':
      REPORT_STATE(d) = REPORT_STATE_LONGDESC;
      SEND_TO_Q("\r\n", d);
      send_editor_help(d);
      SEND_TO_Q("Enter details:\r\n\r\n", d);
      do_odd_write(d, PWT_REPORT_LONGDESC, REPORT_LONGDESC_LENGTH);
      return;

      case '3':
      if (STATE(d) == CON_REPORT_EDIT ||
                !REPORT(d)->isRestricted(d, REPORT_MODE(d))) {
	reportList->dispStateMenu(d);
      } else {
	if (STATE(d) == CON_REPORT_ADD) {
	  reportList->dispAddMenu(d);
	} else {
	  reportList->dispEditMenu(d);
	}
      }
      return;

      case '4':
      if (!REPORT(d)->isRestricted(d, REPORT_MODE(d)) || 
                REPORT_MODE(d) == REPORT_MODE_BUG) {
	reportList->dispTypeMenu(d);
      } else {
	if (STATE(d) == CON_REPORT_ADD) {
	  reportList->dispAddMenu(d);
	} else {
	  reportList->dispEditMenu(d);
	}
      }
      return;

      // 5) playerid
      case '5':
      if (!REPORT(d)->isRestricted(d, REPORT_MODE(d))) {
	SEND_TO_Q("Enter player name : ", d);
	REPORT_STATE(d) = REPORT_STATE_PLAYER;
      } else {
	reportList->dispEditMenu(d);
      }
      return;

      default:
      if (STATE(d) == CON_REPORT_ADD) {
	reportList->dispAddMenu(d);
      } else {
	reportList->dispEditMenu(d);
      }
      break;
    }
    break;

    case REPORT_STATE_PLAYER:
      // search by player name
      playerId = get_id_by_name(arg);
      if (playerId == -1) {
	SEND_TO_Q("Character not found, try again : ", d);
	return;
      }
      REPORT_MODIFIED(d) = 1;
      REPORT(d)->setPlayerId(playerId);

      if (STATE(d) == CON_REPORT_ADD) {
        reportList->dispAddMenu(d);
      } else {
        reportList->dispEditMenu(d);
      }
      return;

    case REPORT_STATE_TYPE:
    option = atoi(arg);

    if (REPORT(d)->isRestricted(d, REPORT_MODE(d))) {
      if (option < (NUM_REPORT_TYPES - NUM_BUG_TYPES + 1) ||
	  option > NUM_REPORT_TYPES) {
	SEND_TO_Q("Invalid choice, try again : ", d);
	return;
      } 
    } else if (option < 1 || option > NUM_REPORT_TYPES) {
      SEND_TO_Q("Invalid choice, try again : ", d);
      return;
    }
    REPORT(d)->setType(option - 1);
    REPORT_MODIFIED(d) = 1;
    if (STATE(d) == CON_REPORT_ADD) {
      reportList->dispAddMenu(d);
    } else {
      reportList->dispEditMenu(d);
    }
    break;

    case REPORT_STATE_STATE:
    option = atoi(arg);
    if (option < 1 || option > NUM_REPORT_STATES) {
      SEND_TO_Q("Invalid choice, try again : ", d);
      return;
    } else {
      REPORT(d)->setState(option - 1);
      REPORT_MODIFIED(d) = 1;
      if (STATE(d) == CON_REPORT_ADD) {
	reportList->dispAddMenu(d);
      } else {
	reportList->dispEditMenu(d);
      }
    }
    break;

    case REPORT_STATE_SHORTDESC:
    if (!genolc_checkstring(d, arg))
      break;
    REPORT(d)->setShortDescription(arg);
    REPORT_MODIFIED(d) = 1;
    if (STATE(d) == CON_REPORT_ADD) {
      reportList->dispAddMenu(d);
    } else {
      reportList->dispEditMenu(d);
    }
    break;

    case REPORT_STATE_LONGDESC:
    if (!genolc_checkstring(d, arg))
      break;
    REPORT(d)->setLongDescription(arg);
    REPORT_MODIFIED(d) = 1;
    if (STATE(d) == CON_REPORT_ADD) {
      reportList->dispAddMenu(d);
    } else {
      reportList->dispEditMenu(d);
    }
    break;

    case REPORT_STATE_CHANGEDESC:
      if (!genolc_checkstring(d, arg))
        break;
      REPORT(d)->setChangeDescription(arg);
      reportList->setReport(d);
      delete(Report *) REPORT(d);
//      REPORT(d) = NULL;
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_REPORTING);
      STATE(d) = CON_PLAYING;
      break;

    // Handle any add/edit differences
    case REPORT_STATE_CONFIRM:
    switch (*arg) {
      // First we must check if we are editing - if so, then prompt for change
      // description ...
      case 'y':
      case 'Y':
      if (STATE(d) == CON_REPORT_EDIT) {
	SEND_TO_Q("Enter change description : ", d);
	REPORT_STATE(d) = REPORT_STATE_CHANGEDESC;
	return;
      }
      reportList->setReport(d);
      break;

      case 'n':
      case 'N':
      act("$n cancels reporting.", TRUE, d->character, NULL, NULL, TO_ROOM);
      break;

      default:
      SEND_TO_Q("Invalid choice!\r\n", d);
      SEND_TO_Q("Do you wish to save changes? : ", d);
      return;
    }
    delete(Report *) REPORT(d);
//    REPORT(d) = NULL;
    REMOVE_BIT(PLR_FLAGS(d->character), PLR_REPORTING);
    STATE(d) = CON_PLAYING;
    break;

    default:
    mudlog("SYSERR: REPORT: Reached default case in report_parse",
	   BRF, MAX(GET_LEVEL(d->character), GET_INVIS_LEV(d->character)),
	   TRUE);
    SEND_TO_Q("Opps...\r\n", d);
    break;
  }
}

void ReportList::setReport(struct descriptor_data *d)
{
  int reportNum = reportList->addReport(d, REPORT(d));

  if (reportNum <= 0) {
    return;
  }

  getReportModeName(REPORT(d)->getReportType(), buf2);

  if (STATE(d) == CON_REPORT_ADD) {
    sprintf(buf, "REPORT: %s added %s report number %d",
	    GET_NAME(d->character), buf2, reportNum); 
  } else {
    sprintf(buf, "REPORT: %s edited %s report number %d",
	    GET_NAME(d->character), buf2, reportNum);
  }

  mudlog(buf, NRM,
	 MAX(GET_LEVEL(d->character), GET_INVIS_LEV(d->character)), TRUE);

  act("$n finishes reporting.", TRUE, d->character, NULL, NULL, TO_ROOM);

  // email/mudmail ppl ....
  reportList->notify(reportNum);
}

void
ReportList::dispTypeMenu(struct descriptor_data *d)
{
  int
    counter, columns = 0, begin = 1, end = NUM_REPORT_TYPES;

  // Only display bug types if restricted, otherwise display all
  if (REPORT(d)->isRestricted(d, REPORT_MODE(d))) {
    begin = NUM_REPORT_TYPES - NUM_BUG_TYPES + 1;
    end = NUM_REPORT_TYPES;
  }

  sprintf(buf, "\r\nReport Types:\r\n");
  for (counter = begin; counter <= end; counter++) {
    sprintf(buf, "%s &g%2d&n) &c%-20.20s %s", buf, counter,
	    report_types[counter - 1], !(++columns % 2) ? "\r\n" : "");
  }
  if ((columns % 2) > 0) {
    sprintf(buf, "%s\r\n", buf);
  }
  sprintf(buf, "%s&nEnter report type : ", buf);
  REPORT_STATE(d) = REPORT_STATE_TYPE;
  page_string(d, buf, TRUE);
}

void
ReportList::dispStateMenu(struct descriptor_data *d)
{
  int
    counter, columns = 0;

  sprintf(buf, "\r\nState Types:\r\n");
  for (counter = 1; counter <= NUM_REPORT_STATES; counter++) {
    sprintf(buf, "%s &g%2d&n) &c%-20.20s %s", buf, counter,
	    report_states[counter - 1], !(++columns % 2) ? "\r\n" : "");
  }
  if ((columns % 2) > 0) {
    sprintf(buf, "%s\r\n", buf);
  }
  sprintf(buf, "%s&nEnter report state : ", buf);

  // We only have the ability to change the state in edit...
  REPORT_STATE(d) = REPORT_STATE_STATE;
  page_string(d, buf, TRUE);
}


// REPORTLIST
// list related functions
int
ReportList::addReport(struct descriptor_data *d, Report * report)
{
  // 2 cases: a new report reportNum will be 0 (when adding a new report)
  //   and the other - when editing, will already have a report number.

  // Edit case
  if (report->getReportNum()) {
    list < Report >::iterator liter;	// iterator for looping over list elements
    for (liter = reports.begin(); liter != reports.end(); liter++) {

      if (report->getReportNum() == liter->getReportNum()) {

        ReportChange reportChange = ReportChange(++topChangeNum, 
                                           report->getReportNum(),
				           GET_IDNUM(d->character),
				           liter->getState(), 
                                           report->getState(),
				           liter->getType(), 
                                           report->getType(),
				           time(0), 
                                           release,
				           report->getChangeDescription());

/*
	// found it - add the change report and copy editable data
	liter->addChange(ReportChange(++topChangeNum, report->getReportNum(),
				      GET_IDNUM(d->character),
				      liter->getState(), report->getState(),
				      liter->getType(), report->getType(),
				      time(0), release,
				      report->getChangeDescription()));
*/
        liter->addChange(reportChange);

        liter->setPlayerId(report->getPlayerId());
	liter->setState(report->getState());
	liter->setType(report->getType());
	liter->setShortDescription(report->getShortDescription());
	liter->setLongDescription(report->getLongDescription());
        writeReport(liter->toFileElem());
        writeChange(reportChange.toFileElem());

	return (liter->getReportNum());
      }
    }

    // Add case
  } else {

    report->setReportNum(++topReportNum);
    reports.push_back(Report(report->getPlayerId(),
	  		     report->getReporterId(),
			     report->getReportNum(), report->getState(),
			     report->getType(), report->getShortDescription(),
			     report->getLongDescription(),
			     report->getOrgRoom(), report->getOrgTime(),
			     report->getOrgRelease()));

    writeReport(report->toFileElem());

    return (report->getReportNum());
  }
  return (-1);
}

bool
ReportList::removeReport(struct descriptor_data * d, int number, int mode)
{
  list < Report >::iterator liter;	// iterator for looping over list elements

  for (liter = reports.begin(); liter != reports.end(); liter++) {
    if (liter->getReportNum() == number) {

      if (liter->getReportType() != mode)
	continue;

      // Check delete restrictions - if its restricted, then state has to be
      // closed
      if (liter->isEditable(d, mode)) {
	if (liter->isRestricted(d, mode) && 
                  liter->getState() != REPORT_STATE_CLOSED) {
	  SEND_TO_Q("You do not have access to delete this report.\r\n", d);
	  return (FALSE);
	}
        // Change state to deleted and save so its not loaded to list next time
        liter->setState(REPORT_STATE_DELETED);
        writeReport(liter->toFileElem());

        // Remove from list
	reports.erase(liter);
	
        sprintf(buf, "REPORT: %s deleted report number %d",
	        GET_NAME(d->character), liter->getReportNum());
        mudlog(buf, NRM,
	     MAX(GET_LEVEL(d->character), GET_INVIS_LEV(d->character)), TRUE);

	return (TRUE);
      } else {
	SEND_TO_Q("You do not have access to delete this report.\r\n", d);
	return (FALSE);
      }
    }
  }
  getNotFoundMesg(mode, buf);
  SEND_TO_Q(buf, d);
  return (FALSE);
}

// printing functions
void
ReportList::listReports(struct descriptor_data *d, int mode, char *arg)
{
  int i = 0, state = -1;
  long playerId = -1;
  bool onlySelf = FALSE;
  list < Report >::iterator liter;  // iterator for looping over list elements

  basic_mud_log("ReportList::listReports: arg = '%s', mode = %d", (*arg) ? arg : "", mode);
  if (*arg) {
    // states
    state = search_block_case_insens(arg, report_states, FALSE);
    if (state == -1) {
      // players reports
      if (!str_cmp(arg, "self")) {
        onlySelf = TRUE;
      // player name
      } else {
        playerId = get_id_by_name(arg);
      }
    }
  }
  buf[0] = '\0';
  for (liter = reports.begin(); liter != reports.end(); liter++) {
    if ((mode != REPORT_MODE_LISTREPORT) && liter->getReportType() != mode)
      continue;

    if (liter->isViewable(d)) {

      // only players reports 
      if ((onlySelf == TRUE) && liter->getPlayerId() != GET_IDNUM(d->character))
        continue;
      // of certain state
      if ((state > -1) && liter->getState() != state)
        continue;
      // characters name
      if ((playerId != -1) && liter->getPlayerId() != playerId)
        continue;
      liter->printBriefDescription(d, buf1);
      sprintf(buf, "%s%s", buf, buf1);
      i++;
    }
  }
  if (i == 0) {
    getNoneExistMesg(mode, buf);
    SEND_TO_Q(buf, d);
  } else {
    sprintf(buf, "%s\r\n%d Reports listed.\r\n", buf, i);
    page_string(d, buf, TRUE);
  }
}

bool
ReportList::printReport(struct descriptor_data *d, int number, int mode)
{

  list < Report >::iterator liter;	// iterator for looping ober list elements

  if (number < 1 || number > getTopReportNum()) {
    getOutOfRangeMesg(mode, buf);
    SEND_TO_Q(buf, d);
    return (FALSE);
  }

  for (liter = reports.begin(); liter != reports.end(); liter++) {
    if (liter->getReportNum() == number) {

      // only look at reports of this type (REPORT_MODE_xxx)
      if (mode != REPORT_MODE_PRINTREPORT && liter->getReportType() != mode)
	continue;

      // Check if player is able to view the report
      if (liter->isViewable(d)) {
	liter->printDetails(d, buf);
	page_string(d, buf, TRUE);
	return (TRUE);
      } else {
	getRestrictedMesg(mode, buf);
	SEND_TO_Q(buf, d);
	return (FALSE);
      }
    }
  }

  getNotFoundMesg(mode, buf);
  SEND_TO_Q(buf, d);
  return (FALSE);
}

bool
ReportList::copyReport(struct descriptor_data * d, int number, int mode)
{
  struct descriptor_data *dsc;

  if (number <= 0 || number > getTopReportNum()) {
    getOutOfRangeMesg(mode, buf);
    SEND_TO_Q(buf, d);
    return (FALSE);
  }

  list < Report >::iterator liter;	// iterator for looping over list elements
  for (liter = reports.begin(); liter != reports.end(); liter++) {

    if (liter->getReportType() != mode)
      continue;

    if (liter->getReportNum() == number) {

      if (!liter->isEditable(d, mode)) {
	getRestrictedMesg(mode, buf);
	SEND_TO_Q(buf, d);
	return (FALSE);
      }
      // found report - now check to see that no other descriptors are editing
      // this report ...
      for (dsc = descriptor_list; dsc; dsc = dsc->next) {
	if (REPORT(dsc) && REPORT(dsc)->getReportNum() == number) {
	  SEND_TO_Q("Report currently being edited.", d);
	  return (FALSE);
	}
      }

      // Now make a copy and set report pointer on descriptor 
/* JEA, unused report
      Report *
	newReport = new Report(liter->getPlayerId(),
	    		       liter->getReporterId(),
			       liter->getReportNum(), liter->getState(),
			       liter->getType(),
			       liter->getShortDescription(),
			       liter->getLongDescription(),
			       liter->getOrgRoom(), liter->getOrgTime(),
			       liter->getOrgRelease());
*/

//      REPORT(d) = (Report *) newReport;
      REPORT_MODE(d) = mode;
      return (TRUE);
    }
  }

  getNotFoundMesg(mode, buf);
  SEND_TO_Q(buf, d);
  return (FALSE);
}


void
Report::mailChanges() {
  list < long > sendTo; 
  list < long >::iterator sliter;
  list < ReportChange >::iterator cliter;
  bool found = FALSE;

  // New report (no changes)
  // notify responsible player if it is not the reporter
  if (changes.begin() == changes.end()) 
    if (getPlayerId() != getReporterId())
      sendTo.push_back(getPlayerId()); 
  
  // Report Changes
  // 
  // Construct a list of playerIDs whom should receive notification
  for (cliter = changes.begin(); cliter != changes.end(); cliter++) 
  {
    found = FALSE;
    // First add reporter to sendTo list.
    if (sendTo.begin() == sendTo.end()) 
      sendTo.push_back(getReporterId()); 
    else
    {
      // Check to see player id doesn't already exist.
      for (sliter = sendTo.begin(); sliter != sendTo.end(); sliter++) 
        if ((*sliter) == cliter->getPlayerId())
          found = TRUE;
      if (!found)
	sendTo.push_back(cliter->getPlayerId()); 
    } 
  }
  
  if (sendTo.begin() == sendTo.end())
    return; 

  cliter = changes.end();
  cliter--;

  getReportModeName(getReportType(), buf2);

  if (changes.begin() == changes.end()) 
    sprintf(buf, "&0Report Added by &7%s&n\r\n"
                 "&0Short Description:&n %s\r\n"
                 "&0Long Description:&n \r\n%s\r\n\r\n"
                 "For full details type &4%s print %d&n\r\n",
	    get_name_by_id(getReporterId()),
	    getShortDescription(), getLongDescription(), buf2, getReportNum());
  else
    sprintf(buf, "&0Report Change by &7%s&n\r\n"
                 "&0Change Description:&n %s\r\n\r\n"
                 "For full details type &4%s print %d&n\r\n",
	    get_name_by_id(cliter->getPlayerId()),
	    cliter->getChangeDescription(), buf2, getReportNum());

  found = FALSE;
  // Now mail the info out
  for (sliter = sendTo.begin(); sliter != sendTo.end(); sliter++) 
  {
    //basic_mud_log("Sending mail to player id %ld", (*sliter));
    if ((*sliter) == getPlayerId())
      found = TRUE;
    store_mail((*sliter), MAIL_FROM_REPORT, buf);
  }

  if (!found)
    store_mail(getPlayerId(), MAIL_FROM_REPORT, buf);
}

void
ReportList::notify(int number) {

  list < Report >::iterator liter;

  for (liter = reports.begin(); liter != reports.end(); liter++) {
    if (liter->getReportNum() == number) {

      if (liter->getReportNum() == number) {
        liter->mailChanges(); 
      }
    }
  }
}

// file related functions

void ReportList::loadFile()
{
  struct report_file_elem report;
  struct reportchange_file_elem reportchange;

  inReportFile.open(REPORTS_FILE, std::ios::in);
  if (!inReportFile) {
    basic_mud_log("SYSERR: could not open reports file %s.", REPORTS_FILE);
    return;
  }

  inReportFile.read((char *)&report, sizeof(report));

  while (!inReportFile.eof()) {
    topReportNum++; 

    // skip adding deleted report to list ...
    if (report.state != REPORT_STATE_DELETED) {
      addReport(report);
    }

    inReportFile.read((char *)&report, sizeof(report));
  }
  inReportFile.close();

  inChangeFile.open(RCHANGES_FILE, std::ios::in);
  if (!inChangeFile) {
    basic_mud_log("SYSERR: could not open report changes file %s.", RCHANGES_FILE);
    return;
  }

  inChangeFile.read((char *)&reportchange, sizeof(reportchange));

  while (!inChangeFile.eof()) {
    topChangeNum++;
    addChange(reportchange);
    inChangeFile.read((char *)&reportchange, sizeof(reportchange));
  }
  inChangeFile.close();
  basic_mud_log("  Loaded %d reports and %d reportchanges", topReportNum, topChangeNum);
}

void ReportList::addReport(struct report_file_elem report)
{
  ReleaseInfo release = ReleaseInfo(report.relMajor, report.relBranch, 
                                    report.relMinor, "", report.relCvsUpToDate, "UNUSED");

  reports.push_back(Report(report.playerId, report.reporterId, report.reportNum,
			   report.state, report.type, report.shortDescription, 
                           report.longDescription, report.orgRoom, 
                           report.orgTime, release));
}

// Converts and adds the report change from file struct to reports list.
// At this stage we have the assumption that all the reports have been added to
// the list.
void ReportList::addChange(struct reportchange_file_elem reportchange)
{
  ReleaseInfo release = ReleaseInfo(reportchange.relMajor,
                                    reportchange.relBranch, 
                                    reportchange.relMinor, "", 
                                    reportchange.relCvsUpToDate,
				    "UNUSED");

  // First find the report is the report list...
  list<Report>::iterator liter;
  for (liter = reports.begin(); liter != reports.end(); liter++) {
    if (reportchange.reportNum == liter->getReportNum()) {
      
      liter->addChange(ReportChange(reportchange.changeNum,
                                    reportchange.reportNum,
                                    reportchange.playerId,
                                    reportchange.fromState, 
                                    reportchange.toState, 
                                    reportchange.fromType,
                                    reportchange.toType, 
                                    reportchange.changeTime, 
                                    release, 
                                    reportchange.changeDescription));
      return;
    }
  }
}

// Artus> We'll do this my way, now.
void ReportList::writeReport(struct report_file_elem report)
{
  FILE *outReportFile;

  outReportFile = fopen(REPORTS_FILE, "r+");
  if (outReportFile == NULL)
    outReportFile = fopen(REPORTS_FILE, "w+");
  if (outReportFile == NULL) {
    basic_mud_log("SYSERR: couldn't open reports file %s for writing", REPORTS_FILE);
    return;
  }
  fseek(outReportFile, (report.reportNum - 1) * sizeof(report), SEEK_SET);
  fwrite(&report, sizeof(report), 1, outReportFile);
  fflush(outReportFile);
  fclose(outReportFile);
}

void ReportList::writeChange(struct reportchange_file_elem reportchange)
{
  FILE *outChangeFile;

  outChangeFile = fopen(RCHANGES_FILE, "r+");
  if (outChangeFile == NULL)
    outChangeFile = fopen(RCHANGES_FILE, "w+");
  if (outChangeFile == NULL)
  {
    basic_mud_log("SYSERR: couldn't open report changes file %s for writing", RCHANGES_FILE);
    return;
  }
  fseek(outChangeFile, (reportchange.changeNum - 1) * sizeof(reportchange), SEEK_SET);
  fwrite(&reportchange, sizeof(reportchange), 1, outChangeFile);
  fflush(outChangeFile);
  fclose(outChangeFile);
}

void ReportList::getNoneExistMesg(int mode, char *writeto)
{
  char temp[10];
  getReportModeName(mode, temp);
  sprintf(writeto, "No &4%s&n reports available.\r\n", temp);
}

void ReportList::getOutOfRangeMesg(int mode, char *writeto)
{
  char temp[10];
  if (mode == REPORT_MODE_PRINTREPORT) {
    sprintf(writeto, "&4report&n report number out of range.\r\n"
	  "Use &4listreports&n to view available reports.\r\n");
  } else {
    getReportModeName(mode, temp);
    sprintf(writeto, "&4%s&n report number out of range.\r\n"
	  "Use &4%s list&n to view available reports.\r\n", temp, temp);
  }
}

void ReportList::getNotFoundMesg(int mode, char *writeto)
{
  char temp[10];
  if (mode == REPORT_MODE_PRINTREPORT) {
    sprintf(writeto,
	  "Report not found. Use &4listreports&n to list available reports.\r\n");
  } else {
    getReportModeName(mode, temp);
    sprintf(writeto,
	  "Report not found. Use &4%s list&n to list available reports.\r\n",
	  temp);
  }
}

void ReportList::getRestrictedMesg(int mode, char *writeto)
{
  char temp[10];
  getReportModeName(mode, temp);
  sprintf(writeto,
	  "Sorry, you don't have access to this report.\r\n"
	  "Use &4%s list&n to list available reports.\r\n", temp);
}

void getReportModeName(int mode, char *writeto)
{
  switch (mode) {
    case REPORT_MODE_BUG:
    sprintf(writeto, "bug");
    return;
    case REPORT_MODE_TODO:
    sprintf(writeto, "todo");
    return;
    case REPORT_MODE_IDEA:
    sprintf(writeto, "idea");
    return;
    case REPORT_MODE_TYPO:
    sprintf(writeto, "typo");
    return;
  }

  sprintf(writeto, "Undefined");
}

// REPORT
int
Report::getReportType()
{
  switch (getType()) {

    case REPORT_TYPE_IDEA:
    return REPORT_MODE_IDEA;

    case REPORT_TYPE_TYPO:
    return REPORT_MODE_TYPO;

    case REPORT_TYPE_TODO:
    return REPORT_MODE_TODO;

    default:
    return REPORT_MODE_BUG;
  }
}

  // list related functions
void
Report::addChange(ReportChange reportChange)
{
  changes.push_back(reportChange);
}


bool
Report::removeChange(int index)
{
  int
    i = 0;
  list < ReportChange >::iterator liter;	// iterator for looping over list elements

  for (liter = changes.begin(); liter != changes.end(); liter++) {
    if (i == index) {
      changes.erase(liter);
      return (TRUE);
    }
    i++;
  }
  return (FALSE);
}

void
Report::printDetails(struct descriptor_data *d, char *writeto)
{
  char stateString[20], typeString[20], dateString[25];
  char temp[MAX_STRING_LENGTH], temp1[MAX_STRING_LENGTH];
  time_t dispTime = getOrgTime();
#ifdef NO_LOCALTIME
  struct tm lt;
#endif

  sprinttype(getType(), report_types, typeString);
  sprinttype(getState(), report_states, stateString);
#ifndef NO_LOCALTIME
  strncpy(dateString, (char *) asctime(localtime(&dispTime)), 25);
#else
  jk_localtime(&lt, dispTime);
  strncpy(dateString, (char *) asctime(&lt), 25);
#endif
  dateString[24] = '\0';

  sprintf(writeto,
	  "Report Num: [&g%3d&n] Reported by &7%s&n on &g%s&n "
	  "Rev &c%d.%d.%d&n\r\n"
	  "Type: [&4%s&n] State: [&4%s&n] Room: [&8%d&n]\r\n"
	  "S-Desc: &0%s&n\r\n"
	  "L-Desc: %s\r\n",
	  getReportNum(), get_name_by_id(getReporterId()), dateString,
	  getOrgRelease().getMajor(), getOrgRelease().getBranch(),
	  getOrgRelease().getMinor(),
	  typeString, stateString, getOrgRoom(),
	  getShortDescription(), getLongDescription());

  temp[0] = '\0';
  list < ReportChange >::iterator liter;	// iterator for looping over list elemnts
  for (liter = changes.begin(); liter != changes.end(); liter++) {
    liter->printDetails(temp1);
    sprintf(temp, "%s%s", temp, temp1);
  }
  sprintf(writeto, "%s\r\n%s", writeto, temp);
}

void
Report::printBriefDescription(struct descriptor_data *d, char *writeto)
{
  char
    stateString[20], typeString[20];

  sprinttype(getType(), report_types, typeString);
  sprinttype(getState(), report_states, stateString);

  sprintf(writeto, "&g%3d&n) S-Desc: &0%s&n\r\n"
	  "     Type: [&4%12s&n] State: [&4%7s&n] "
	  "Rev [&c%d.%d.%d&n] By [&7%s&n] Res [&7%s&n]\r\n",
	  getReportNum(), getShortDescription(),
	  typeString, stateString,
	  getOrgRelease().getMajor(), getOrgRelease().getBranch(),
	  getOrgRelease().getMinor(), get_name_by_id(getReporterId()), get_name_by_id(getPlayerId()));
}

void
ReportChange::printDetails(char *writeto)
{
  char dateString[25], fromState[20], toState[20], fromType[20], toType[20];
  bool appendNewLine = FALSE;
  time_t dispTime = getTime();
#ifdef NO_LOCALTIME
  struct tm lt;
#endif

#ifndef NO_LOCALTIME
  strncpy(dateString, (char *) asctime(localtime(&dispTime)), 25);
#else
  jk_localtime(&lt, dispTime);
  strncpy(dateString, (char *) asctime(&lt), 25);
#endif
  dateString[24] = '\0';

  sprintf(writeto, "Change by &7%s&n on &g%s&n Rev &c%d.%d.%d&n\r\n",
	  get_name_by_id(getPlayerId()), dateString,
	  getRelease().getMajor(), getRelease().getBranch(),
	  getRelease().getMinor());

  if (getFromState() != getToState()) {
    sprinttype(getFromState(), report_states, fromState);
    sprinttype(getToState(), report_states, toState);

    sprintf(writeto, "%s  State: &4%s&n -> &4%s&n",
	    writeto, fromState, toState);
    appendNewLine = TRUE;
  }

  if (getFromType() != getToType()) {
    sprinttype(getFromType(), report_types, fromType);
    sprinttype(getToType(), report_types, toType);

    sprintf(writeto, "%s  Type: &4%s&n -> &4%s&n", writeto, fromType, toType);
    appendNewLine = TRUE;
  }

  sprintf(writeto, "%s%s  Description : &1%s&n\r\n",
	  writeto, (appendNewLine) ? "\r\n" : "", getChangeDescription());
}

struct report_file_elem Report::toFileElem()
{
  struct report_file_elem
    fileElem;

  fileElem.playerId = getPlayerId();
  fileElem.reporterId = getReporterId();
  fileElem.reportNum = getReportNum();
  fileElem.state = getState();
  fileElem.type = getType();
  fileElem.orgRoom = getOrgRoom();
  fileElem.orgTime = getOrgTime();
  fileElem.relMajor = getOrgRelease().getMajor();
  fileElem.relBranch = getOrgRelease().getBranch();
  fileElem.relMinor = getOrgRelease().getMinor();
  fileElem.relCvsUpToDate = getOrgRelease().isCvsUpToDate();
  strcpy(fileElem.shortDescription, getShortDescription());
  strcpy(fileElem.longDescription, getLongDescription());

  return fileElem;
}

struct reportchange_file_elem ReportChange::toFileElem()
{
  struct reportchange_file_elem fileElem;

  fileElem.changeNum = getChangeNum();
  fileElem.reportNum = getReportNum();
  fileElem.playerId = getPlayerId();
  fileElem.fromState = getFromState();
  fileElem.toState = getToState();
  fileElem.fromType = getFromType();
  fileElem.toType = getToType();
  fileElem.changeTime = getTime();
  fileElem.relMajor = getRelease().getMajor();
  fileElem.relBranch = getRelease().getBranch();
  fileElem.relMinor = getRelease().getMinor();
  fileElem.relCvsUpToDate = getRelease().isCvsUpToDate();
  strncpy(fileElem.changeDescription, getChangeDescription(), REPORT_SHORTDESC_LENGTH);

  return fileElem;
}

const char *
  report_states[] = {
  "OPEN",
  "ASSESSED",
  "FEEDBACK",
  "CLOSED",
  "DELETED",
  "\n"
};

const char *
  report_types[] = {
  "IDEA",
  "TYPO",
  "TODO",
  "BUG-CRASH",
  "BUG-BUILDING",
  "BUG-BALANCE",
  "BUG-OTHER",
  "\n"
};
