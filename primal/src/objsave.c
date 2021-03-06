/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
   Modified by Brett Murphy to remove Cryogenic code and introduce
   a rent_per_day on/off flag
*/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "screen.h"

extern char *ALIAS_DIRNAME;
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int min_rent_cost;
extern int rent_per_day;
extern int rent_discount;


/* Extern functions */
ACMD(do_tell);
SPECIAL(receptionist);
extern int get_world(struct char_data *ch);

struct obj_data *Obj_from_store(struct obj_file_elem object)
{
  struct obj_data *obj;
/*  int j; */

  if (real_object(object.item_number) > -1) {
    obj = read_object(object.item_number, VIRTUAL);
/*    GET_OBJ_VAL(obj, 0) = object.value[0];
    GET_OBJ_VAL(obj, 1) = object.value[1];
    GET_OBJ_VAL(obj, 2) = object.value[2];
    GET_OBJ_VAL(obj, 3) = object.value[3];
    GET_OBJ_EXTRA(obj) = object.extra_flags;
    GET_OBJ_WEIGHT(obj) = object.weight;
    GET_OBJ_TIMER(obj) = object.timer;
    obj->obj_flags.bitvector = object.bitvector;

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      obj->affected[j] = object.affected[j];
*/
    return obj;
  } else
    return NULL;
}



int Obj_to_store(struct obj_data * obj, FILE * fl)
{
  int j;
  struct obj_file_elem object;

  object.item_number = GET_OBJ_VNUM(obj);
  object.value[0] = GET_OBJ_VAL(obj, 0);
  object.value[1] = GET_OBJ_VAL(obj, 1);
  object.value[2] = GET_OBJ_VAL(obj, 2);
  object.value[3] = GET_OBJ_VAL(obj, 3);
  object.extra_flags = GET_OBJ_EXTRA(obj);
  object.weight = GET_OBJ_WEIGHT(obj);
  object.timer = GET_OBJ_TIMER(obj);
  object.bitvector = obj->obj_flags.bitvector;
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    object.affected[j] = obj->affected[j];

  if (fwrite(&object, sizeof(struct obj_file_elem), 1, fl) < 1) {
    perror("Error writing object in Obj_to_store");
    return 0;
  }
  return 1;
}

int Alias_delete_file(char *name)
{
  char flname[256];
  FILE *afile;

  strcpy(flname,ALIAS_DIRNAME);
  strcat(flname,name);
  if (!(afile = fopen(flname, "rb"))) {
    if (errno != ENOENT) {	/* if it fails but NOT because of no file */
      sprintf(buf1, "SYSERR: deleting alias file %s (1)", flname);
      perror(buf1);
    }
    return 0;
  }
  fclose(afile);

  if (unlink(flname) < 0) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: deleting alias file %s (2)", flname);
      perror(buf1);
    }
  }
  return (1);
}

int Crash_delete_file(char *name)
{
  char filename[50];
  FILE *fl;

  if (!get_filename(name, filename, CRASH_FILE))
    return 0;
  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) {	/* if it fails but NOT because of no file */
      sprintf(buf1, "SYSERR: deleting crash file %s (1)", filename);
      perror(buf1);
    }
    return 0;
  }
  fclose(fl);

  if (unlink(filename) < 0) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: deleting crash file %s (2)", filename);
      perror(buf1);
    }
  }
  return (1);
}


int Crash_delete_crashfile(struct char_data * ch)
{
  char fname[MAX_INPUT_LENGTH];
  struct rent_info rent;
  FILE *fl;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
    return 0;
  if (!(fl = fopen(fname, "rb"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: checking for crash file %s (3)", fname);
      perror(buf1);
    }
    return 0;
  }
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  fclose(fl);

  if (rent.rentcode == RENT_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return 1;
}


int Crash_clean_file(char *name)
{
  char fname[MAX_STRING_LENGTH], filetype[20];
  struct rent_info rent;
  extern int rent_file_timeout, crash_file_timeout;
  FILE *fl;

  if (!get_filename(name, fname, CRASH_FILE))
    return 0;
  /*
   * open for write so that permission problems will be flagged now, at boot
   * time.
   */
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: OPENING OBJECT FILE %s (4)", fname);
      perror(buf1);
    }
    return 0;
  }
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  fclose(fl);

  if ((rent.rentcode == RENT_CRASH) ||
      (rent.rentcode == RENT_FORCED) || (rent.rentcode == RENT_TIMEDOUT)) {
    if (rent.time < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      switch (rent.rentcode) {
      case RENT_CRASH:
	strcpy(filetype, "crash");
	break;
      case RENT_FORCED:
	strcpy(filetype, "forced rent");
	break;
      case RENT_TIMEDOUT:
	strcpy(filetype, "idlesave");
	break;
      default:
	strcpy(filetype, "UNKNOWN!");
	break;
      }
      sprintf(buf, "    Deleting %s's %s file.", name, filetype);
      log(buf);
      return 1;
    }
    /* Must retrieve rented items w/in 30 days */
  } else if (rent.rentcode == RENT_RENTED)
    if (rent.time < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      sprintf(buf, "    Deleting %s's rent file.", name);
      log(buf);
      return 1;
    }
  return (0);
}



void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
{
	printf("Crash_clean %s\n", (player_table+i)->name);
	if ((player_table + i))
    Crash_clean_file((player_table + i)->name);
}
  return;
}



void Crash_listrent(struct char_data * ch, char *name)
{
  FILE *fl;
  char fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct obj_data *obj;
  struct rent_info rent;

  if (!get_filename(name, fname, CRASH_FILE))
    return;
  if (!(fl = fopen(fname, "rb"))) {
    sprintf(buf, "%s has no rent file.\r\n", name);
    send_to_char(buf, ch);
    return;
  }
  sprintf(buf, "%s\r\n", fname);
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  switch (rent.rentcode) {
  case RENT_RENTED:
    strcat(buf, "Rent\r\n");
    break;
  case RENT_CRASH:
    strcat(buf, "Crash\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
    strcat(buf, "TimedOut\r\n");
    break;
  default:
    strcat(buf, "Undef\r\n");
    break;
  }
  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      fclose(fl);
      return;
    }
    if (!feof(fl))
      if (real_object(object.item_number) > -1) {
	obj = read_object(object.item_number, VIRTUAL);
	sprintf(buf, "%s [%5d] (%5dau) %-20s\r\n", buf,
		object.item_number, GET_OBJ_RENT(obj),
		obj->short_description);
	extract_obj(obj);
      }
  }
  send_to_char(buf, ch);
  fclose(fl);
}



int Crash_write_rentcode(FILE * fl, struct rent_info * rent)
/* Have removed ch as an arg to this function *shrug* Darius */
{
  if (fwrite(rent, sizeof(struct rent_info), 1, fl) < 1) {
    perror("Writing rent code.");
    return 0;
  }
  return 1;
}



int Crash_load(struct char_data * ch)
/* return values:
	0 - successful load, keep char in rent room.
	1 - load failure or load of crash items -- put char in temple.
	2 - rented equipment lost (no $)
*/
/* info channel code by hal */
{
  void Crash_crashsave(struct char_data * ch);

  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct rent_info rent;
  int cost, orig_rent_code;
  float num_of_days;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
    return 1;
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING OBJECT FILE %s (5)", fname);
      perror(buf1);
      send_to_char("\r\n********************* NOTICE *********************\r\n"
		   "There was a problem loading your objects from disk.\r\n"
		   "Contact a God for assistance.\r\n", ch);
    }
    sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
    
    
    return 1;
  }
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);

  if (rent.rentcode == RENT_RENTED || rent.rentcode == RENT_TIMEDOUT) {
    if (rent_per_day)
      num_of_days = (float) (time(0) - rent.time) / SECS_PER_REAL_DAY;
    else
      num_of_days=1;
    
    /* note: discount is applied at rent time and stored in rent file, so dont re-apply discount */
    cost = (int) (rent.net_cost_per_diem * num_of_days);
    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      fclose(fl);
      sprintf(buf, "%s entering game, rented equipment lost (no $).",
	      GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
      Crash_crashsave(ch);
    
    
      return 2;
    } else {
      GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
      GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
      if(real_room(ENTRY_ROOM(ch,get_world(ch))))
        save_char(ch,ENTRY_ROOM(ch,get_world(ch)));
      else save_char(ch,NOWHERE);
    }
  }
  switch (orig_rent_code = rent.rentcode) {
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
    mudlog(buf, BRF, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
    break;
  }

  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      perror("Reading crash file: Crash_load.");
      fclose(fl);
      return 1;
    }
    if (!feof(fl))
      obj_to_char(Obj_from_store(object), ch);
  }

  /* turn this into a crash file by re-writing the control block */
  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  rewind(fl);
  Crash_write_rentcode(fl, &rent);

  fclose(fl);

  if (orig_rent_code == RENT_RENTED)
    return 0;
  else
    return 1;
}



int Crash_save(struct obj_data * obj, FILE * fp)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->contains, fp);
    Crash_save(obj->next_content, fp);
    result = Obj_to_store(obj, fp);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return 0;
  }
  return TRUE;
}


void Crash_restore_weight(struct obj_data * obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}



void Crash_extract_objs(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}


int Crash_is_unrentable(struct obj_data * obj)
{
  if (!obj)
    return 0;

  if (IS_OBJ_STAT(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 ||
      GET_OBJ_RNUM(obj) <= NOTHING || GET_OBJ_TYPE(obj) == ITEM_KEY
      || GET_OBJ_TYPE(obj) == ITEM_POTION)
    return 1;

  return 0;
}


void Crash_extract_norents(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}


void Crash_extract_expensive(struct obj_data * obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
    if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
      max = tobj;
  extract_obj(max);
}



void Crash_calculate_rent(struct obj_data * obj, int *cost)
{
  if (obj) {
    *cost += MAX(0, GET_OBJ_RENT(obj));
    Crash_calculate_rent(obj->contains, cost);
    Crash_calculate_rent(obj->next_content, cost);
  }
}


void Crash_shopsave(struct char_data *ch,int shopnum)
{
  char buf[MAX_INPUT_LENGTH];
  FILE *fp;

  sprintf(buf,"shoprent/%d.shop",shopnum);
  if(!(fp = fopen(buf,"wb"))) return;

  if(!Crash_save(ch->carrying,fp)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);
  fclose(fp);
}

void Crash_shopload(struct char_data *ch,int shopnum)
{
  char buf[MAX_INPUT_LENGTH];
  FILE *fl;
  struct obj_file_elem object;

  sprintf(buf,"shoprent/%d.shop",shopnum);
  if(!(fl = fopen(buf,"rb")))
    if(errno != ENOENT)
      mudlog("Something wierd with a shop rental file :(",NRM,LVL_GRGOD,TRUE);

  while(!feof(fl)) {
    fread(&object,sizeof(struct obj_file_elem),1,fl);
    if(ferror(fl)) {
      fclose(fl);
      return;
    }
    if(!feof(fl))
      obj_to_char(Obj_from_store(object),ch);
  }
  fclose(fl);
}


void Crash_crashsave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch)) return;
  if(world[ch->in_room].zone == GOD_ROOMS_ZONE) return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  if (!Crash_write_rentcode(fp, &rent)) {
    fclose(fp);
    return;
  }
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);

  for (j = 0; j < NUM_WEARS; j++)
    if (ch->equipment[j]) {
      if (!Crash_save(ch->equipment[j], fp)) {
	fclose(fp);
	return;
      }
      Crash_restore_weight(ch->equipment[j]);
    }
  fclose(fp);
  REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


 void Crash_idlesave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  long cost;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

  for (j = 0; j < NUM_WEARS; j++)
    if (ch->equipment[j])
      obj_to_char(unequip_char(ch, j), ch);

  Crash_extract_norents(ch->carrying);

  cost = 0;
  Crash_calculate_rent(ch->carrying, (int *)&cost);
  cost = cost - rent_discount/100*cost;
  cost *= 1;			/* forcerent cost is 1x normal rent */
  while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying) {
    Crash_extract_expensive(ch->carrying);
    cost = 0;
    Crash_calculate_rent(ch->carrying, (int *)&cost);
    cost = cost - rent_discount/100*cost;
    cost *=1;
  }

  if (!ch->carrying) {
    fclose(fp);
    Crash_delete_file(GET_NAME(ch));
    return;
  }
  
/* Darius - this is where idlesave was free! */
  rent.net_cost_per_diem = cost;
  rent.rentcode = RENT_TIMEDOUT;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(fp, &rent)) {
    fclose(fp);
    return;
  }
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_rentsave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

  for (j = 0; j < NUM_WEARS; j++)
    if (ch->equipment[j])
      obj_to_char(unequip_char(ch, j), ch);

  Crash_extract_norents(ch->carrying);

  rent.net_cost_per_diem = cost;
  rent.rentcode = RENT_RENTED;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(fp, &rent)) {
    fclose(fp);
    return;
  }
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */

void Crash_rent_deadline(struct char_data * ch, struct char_data * recep,
			      long cost)
{
  long rent_deadline;
  if (!cost)
    return;

  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
  if (rent_per_day)    
    sprintf(buf,
      "$n tells you, 'You can rent for %ld day%s with the gold you have\r\n"
	  "on hand and in the bank.'\r\n",
	  rent_deadline, (rent_deadline > 1) ? "s" : "");
  
  act(buf, FALSE, recep, 0, ch, TO_VICT);
}

int Crash_report_unrentables(struct char_data * ch, struct char_data * recep,
			         struct obj_data * obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj) {
    if (Crash_is_unrentable(obj)) {
      has_norents = 1;
      sprintf(buf, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
      act(buf, FALSE, recep, 0, ch, TO_VICT);
    }
    has_norents += Crash_report_unrentables(ch, recep, obj->contains);
    has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
  }
  return (has_norents);
}



void Crash_report_rent(struct char_data * ch, struct char_data * recep,
		            struct obj_data * obj, long *cost, long *nitems, int display,int factor)
{
  static char buf[256];

  if (obj) {
    if (!Crash_is_unrentable(obj)) {
      (*nitems)++;
      *cost += MAX(0, (GET_OBJ_RENT(obj) * factor));
      if (display) {
	sprintf(buf, "$n tells you, '%s%5d%s coins for %s..'",
		CCGOLD(ch,C_NRM),(GET_OBJ_RENT(obj) * factor),CCNRM(ch,C_NRM),
		OBJS(obj, ch));
	act(buf, FALSE, recep, 0, ch, TO_VICT);
      }
    }
    Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
    Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
  }
}



int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist,
		         int display, int factor)
{
  extern int max_obj_save;	/* change in config.c */
  char buf[MAX_INPUT_LENGTH];
  int i;
  long totalcost = 0, numitems = 0, norent = 0;

  norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    norent += Crash_report_unrentables(ch, receptionist, ch->equipment[i]);

  if (norent)
    return 0;

  totalcost = min_rent_cost * factor;

  Crash_report_rent(ch, receptionist, ch->carrying, &totalcost, &numitems, display, factor);

  for (i = 0; i < NUM_WEARS; i++)
    Crash_report_rent(ch, receptionist, ch->equipment[i], &totalcost, &numitems, display, factor);

  if (!numitems) {
    act("$n tells you, 'But you are not carrying anything!  Just quit!'",
	FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (numitems > max_obj_save) {
    sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
	    max_obj_save);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (display) {
    sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
	    min_rent_cost * factor);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    sprintf(buf, "$n tells you, 'For a total of %s%ld%s coins%s.'",
	    CCGOLD(ch,C_NRM),totalcost,CCNRM(ch,C_NRM),(rent_per_day ? " per day" : ""));
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    if (rent_discount)
    {
      sprintf(buf, "$n tells you, 'With a discount of %d%%, for a total of %s%ld%s coins%s.'", 
	rent_discount,CCGOLD(ch,C_NRM),(totalcost-totalcost*rent_discount/100), 
	CCNRM(ch,C_NRM),(rent_per_day ? " per day" : ""));
      act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    }
    if ((totalcost-totalcost*rent_discount/100) > GET_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, receptionist, 0, ch, TO_VICT);
      return (0);
    } else if (rent_per_day)
      Crash_rent_deadline(ch, receptionist, (totalcost-totalcost*rent_discount/100));
  }
  return (totalcost-totalcost*rent_discount/100);
}



int gen_receptionist(struct char_data * ch, struct char_data * recep,
		         int cmd, char *arg)
{
  extern void extract_char(struct char_data *ch);
  int cost = 0;
  extern int free_rent;

  if (!ch->desc || IS_NPC(ch))
    return FALSE;

  if(!cmd) return FALSE;
  if (!CMD_IS("offer") && !CMD_IS("rent"))
    return FALSE;
  if (!AWAKE(recep)) {
    send_to_char("She is unable to talk to you...\r\n", ch);
    return TRUE;
  }
  if (!CAN_SEE(recep, ch)) {
    act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
    return TRUE;
  }
  if (free_rent) {
    act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
	FALSE, recep, 0, ch, TO_VICT);
    return 1;
  }
  if (CMD_IS("rent")) 
  {
    if (!(cost = Crash_offer_rent(ch, recep, FALSE,1)))
      return TRUE;
    sprintf(buf, "$n tells you, 'Rent will cost you %s%d%s gold coins %s.'",
		CCGOLD(ch,C_NRM),cost,CCNRM(ch,C_NRM),
		(rent_per_day ? " per day" : ""));
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    if (cost > GET_GOLD(ch)) 
    {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, recep, 0, ch, TO_VICT);
      return TRUE;
    }
    if (cost && (rent_per_day))
      Crash_rent_deadline(ch, recep, cost);

    act("$n stores your belongings and helps you into your private chamber.",
	  FALSE, recep, 0, ch, TO_VICT);
    Crash_rentsave(ch, cost);
    sprintf(buf, "%s has rented (%d%s, %d tot.)", GET_NAME(ch),
	      cost, (rent_per_day ? "/day" : ""), GET_GOLD(ch) + GET_BANK_GOLD(ch)); 

    mudlog(buf, NRM, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
    sprintf ( buf , "%s has quit the game.", GET_NAME(ch) );
    info_channel( buf , ch ) ;
    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch,TO_NOTVICT);
    ENTRY_ROOM(ch,get_world(ch)) = world[ch->in_room].number;
    extract_char(ch);
    save_char(ch,NOWHERE);
  } else {
    Crash_offer_rent(ch, recep, TRUE,1);
    act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
  }
  return TRUE;
}


SPECIAL(receptionist)
{
  return (gen_receptionist(ch, me, cmd, argument));
}

void Crash_save_all(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if ((d->connected == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
	Crash_crashsave(d->character);
	save_char(d->character, NOWHERE);
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}
