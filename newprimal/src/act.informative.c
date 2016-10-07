/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
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
#include "screen.h"
#include "constants.h"

/* extern variables */
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern long NUM_PLAYERS;
extern long top_idnum;

extern char *credits;
extern char *areas;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *class_abbrevs[];

/* extern functions */
ACMD(do_action);
long find_class_bitvector(char arg);
int level_exp(struct char_data *ch, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(struct char_data *ch);

/* local functions */
void print_object_location(int num, struct obj_data * obj, struct char_data * ch, int recur);
void show_obj_to_char(struct obj_data * object, struct char_data * ch, int mode);
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode, int show);
ACMD(do_search);
ACMD(do_immlist);
ACMD(do_info);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
void perform_mortal_where(struct char_data * ch, char *arg);
void perform_immort_where(struct char_data * ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
void sort_commands(void);
ACMD(do_commands);
void diag_char_to_char(struct char_data * i, struct char_data * ch);
void look_at_char(struct char_data * i, struct char_data * ch);
void list_one_char(struct char_data * i, struct char_data * ch);
void list_char_to_char(struct char_data * list, struct char_data * ch);
void do_auto_exits(struct char_data * ch);
ACMD(do_exits);
void look_in_direction(struct char_data * ch, int dir);
void look_in_obj(struct char_data * ch, char *arg);
char *find_exdesc(char *word, struct extra_descr_data * list);
void look_at_target(struct char_data * ch, char *arg);

void display_clan_table(struct char_data *ch, struct clan_data *clan);
void toggle_display(struct char_data *ch, struct char_data *vict);  

void trigger_trap(struct char_data *ch, struct obj_data *obj) {

	struct obj_data *temp;
        struct descriptor_data *d;
	int damage;

	if( GET_OBJ_VAL(obj, 0) == 0 ) {
	  send_to_char("&bPhew, it wasn't loaded.&n\r\n", ch);
	  if( GET_OBJ_VAL(obj, 3) == 0 )
		return;	// Nothing more to do

	  temp = read_object(GET_OBJ_VAL(obj, 3), VIRTUAL);
 	  extract_obj(obj);
	  obj_to_room(temp, ch->in_room);
	  act("The trap was protecting $p!", FALSE, ch, temp, 0, TO_CHAR);
	  act("$n triggered a trap, which was fortunately unloaded, finding $p.", FALSE, ch, temp, 0, TO_ROOM);
	  
	  return;
	}

	send_to_char("You trigger it!&n\r\n", ch);
	act("...$n triggers a trap!", FALSE, ch, 0, 0, TO_ROOM);

	damage = dice(GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));

	if( GET_OBJ_VAL(obj, 0) == 1 ) {
	   send_to_char("You take the brunt of the damage.\r\n", ch);
	   GET_HIT(ch) -= damage;
	   update_pos(ch);
	}	
	else {   // Room affect
	   send_to_char("The trap goes off, damaging everyone nearby.\r\n", ch);
	   act("The trap goes off, damaging everyone in the vicinity!", 
		FALSE, ch, 0, 0, TO_ROOM);	
	   for( d = descriptor_list; d; d = d->next ) {
		if( ((d->character)->in_room == ch->in_room) &&
		   (d->character != ch)) 
		   GET_HIT(d->character) -= damage; // Do damage
	   } // Every descriptor
	} // Room affect

	// Check if we replace the obj
	if( GET_OBJ_VAL(obj, 3) >= 1 ) {
	    temp = read_object(GET_OBJ_VAL(obj, 3), VIRTUAL);
            act("You find $p in the aftermath.", FALSE, ch, temp,0,TO_CHAR);
	    act("You spot $p in the aftermath.", FALSE, ch, temp,0,TO_ROOM);
	    obj_to_room(temp, ch->in_room);
	}

	// Get rid of the trap
        extract_obj(obj);
}

ACMD(do_search) {

	int chance = 0, i, counter = 0;
	struct room_data *r;
	struct obj_data *o;

	if( IS_NPC(ch))
		return;

	if( AFF_FLAGGED(ch, AFF_PARALYZED) ) {
	  send_to_char("Your limbs won't respond. You're paralysed!\r\n",ch);
	  return;
	}
	
 	if( !GET_SKILL(ch, SKILL_SEARCH)) {
	   send_to_char("You stumble around, but achieve nothing.\r\n",ch);
	   return;
	}
	
	if( IS_THIEF(ch) )	// Favour thief for multi-classed
		chance = 15;	// 15%
	else if( IS_MAGIC_USER(ch) )
		chance = 5;	// 5%
	else
		chance = 2;	// 2%

	r = &world[ch->in_room];

	// first, check for exits
	for(i = 0; i < NUM_OF_DIRS; i++ ) {
		if( r->dir_option[i] == NULL )
			continue;

		if( EXIT_FLAGGED(EXIT(ch, i), EX_ISDOOR) &&
		    EXIT_FLAGGED(EXIT(ch, i), EX_CLOSED) ) {
		   sprintf(buf, "You find an exit leading %s.\r\n", dirs[i]);
		   send_to_char(buf, ch);
		   counter++;
		} 
	}

	// Now do the real looking
	for( o = r->contents; o; o = o->next_content) {
	  if( OBJ_FLAGGED(o, ITEM_HIDDEN))
	     if( number(1, 100) > 100-chance ) {
		act("You find $p, hidden nearby!", FALSE, ch, o, 0, TO_CHAR);
	        act("$n finds $p, hidden nearby!", FALSE, ch, o, 0, TO_ROOM);
	        counter++;
		REMOVE_BIT(o->obj_flags.extra_flags, ITEM_HIDDEN);
		// Check if it's a trap!
		if( GET_OBJ_TYPE(o) == ITEM_TRAP ) {
                   send_to_char("&RIt's a trap! ", ch);
		   if( number(1, chance) == chance) {  // favour thieves
		      trigger_trap(ch, o);
		   }
		   else
			send_to_char("&g .. but you manage not to trigger it.&n\r\n",ch);
		}
	     }
	}

	if( !counter )
		send_to_char("You find nothing unusual.\r\n", ch);
}



void sort(long list[], long owners[], int num) {

	int i, j;
	long hold;

	for(i =0; i < num; i++)
            for(j=0; j < num - 1; j++ ) 
                if( list[j] > list[j+1] ){	    
	    	  hold = list[j];
		  list[j] = list[j+1];
		  list[j+1] = hold;

		  // Move the owner with it
		  hold = owners[j];
		  owners[j] = owners[j+1];
		  owners[j+1] = hold;
		}


}

ACMD(do_immlist) {

	int counter, found = 0;
	struct char_data *tch;
	struct char_file_u tmp;
	long  immpts[NUM_PLAYERS], // kills vs imms
	      immid[NUM_PLAYERS];  // Id's of imms found


	for(counter = 1; counter <= top_idnum; counter++) {
               if( load_char(get_name_by_id(counter), &tmp) < 0) 
		    continue;
		// Player exists 
		CREATE(tch, struct char_data, 1);
		clear_char(tch);
		store_to_char(&tmp, tch);
		char_to_room(tch, 0);

		if( GET_LEVEL(tch) >= LVL_IMMORT ){
		   immid[found] = counter;
		   immpts[found] = GET_IMMKILLS(ch);
		   found++;
		}		  				
		extract_char(tch);
	}	

	// sort in ascending order
	sort(immpts, immid, found);

	// Now display the stats
	sprintf(buf, "\r\n   &r.-&R'-.&y_.&Y-'&w-&W Primal Immortals &w-&Y'-&y._.&R-'&r-.&n\r\n");
	sprintf(buf + strlen(buf), "\r\n\r\n       &BName          Rank     Kills\r\n");
	for(counter = found; counter; counter--) {
           load_char(get_name_by_id(immid[counter-1]), &tmp); 
	   CREATE(tch, struct char_data, 1);
	   clear_char(tch);
	   store_to_char(&tmp, tch);
	   char_to_room(tch, 0);
           
	   sprintf(buf+strlen(buf), "&y       %-12s  %2d         %ld&n\r\n", 
		GET_NAME(tch), (found - counter + 1),GET_IMMKILLS(tch) ); 
	   extract_char(tch);
        }
	sprintf(buf + strlen(buf), "\r\n       &B%d &bImmortal%s listed.\r\n&n",
	  found, found == 1 ? "" : "s"); 	
	send_to_char(buf, ch);
}

/* Command to display and set player information */
ACMD(do_info){

	long tchid;
	struct char_data *tch;
	struct char_file_u tmp;

	if( IS_NPC(ch)) {
		send_to_char("You already know everything there is to know!\r\n", ch);
		return;
	}
	
        if( PLR_FLAGGED(ch, PLR_NOINFO) && (GET_LEVEL(ch) < LVL_GRGOD)) {
	   send_to_char("You can't set your info. Privilidges have been revoked.\r\n", ch);				
	   return;
	}

	two_arguments(argument, arg, buf1);
	// Just want to know about themselves?
	if( !*arg ) {
		sprintf(buf, "&gYour current details are:&n\r\n"
		  "&y Email:&n %s\r\n &yWebpage:&n %s\r\n&y Personal:&n %s&n\r\n",
		  GET_EMAIL(ch) == NULL ? "None" : GET_EMAIL(ch), 
		  GET_WEBPAGE(ch) == NULL ? "None" : GET_WEBPAGE(ch),
		  GET_PERSONAL(ch) == NULL ? "None" : GET_PERSONAL(ch));
		send_to_char(buf, ch);
		return;
	}

	// Looking for someone?
	if( !*buf1 ) {
		if( (tchid = get_id_by_name(arg)) < 1 ) {
			send_to_char("No such player!\r\n", ch);
			return;
		}
		// Load the character up
		CREATE(tch, struct char_data, 1);
		clear_char(tch);
		load_char(arg, &tmp);
		store_to_char(&tmp, tch);
		char_to_room(tch, 0);
		sprintf(buf, "&g%s's Info&n\r\n"
		  "&y Email: &n%s\r\n&y Webpage:&n %s\r\n&y Personal: &n%s&n\r\n",
		  GET_NAME(tch), 
		  GET_EMAIL(tch) == NULL ? "None" : GET_EMAIL(tch), 
		  GET_WEBPAGE(tch) == NULL ? "None" : GET_WEBPAGE(tch),
		  GET_PERSONAL(tch) == NULL ? "None" : GET_PERSONAL(tch));
		send_to_char(buf, ch);
		extract_char(tch);
		return;		 
	}

	//to_lower(arg);
	// We have both arguments
	if( strcmp(arg, "email") == 0 ) {
		if( GET_EMAIL(ch) )
			free(GET_EMAIL(ch));
                if( strcmp(buf1, "clear") == 0 ) {
                     GET_EMAIL(ch) = str_dup("None");
		     *buf1 = '\0';
		     send_to_char("You clear your email.\r\n", ch);
		}
		else {
                     GET_EMAIL(ch) = str_dup(buf1);
		     *buf1 = '\0';
		     send_to_char("You set your email.\r\n", ch);
		}
		return;
	}
	if( strcmp(arg, "webpage") == 0 ) {
		if( GET_WEBPAGE(ch))
			free(GET_WEBPAGE(ch));
                if( strcmp(buf1, "clear") == 0 ) {
	           GET_WEBPAGE(ch) = str_dup("None");
		   send_to_char("You clear your webpage.\r\n", ch);
 		}
		else {
		   GET_WEBPAGE(ch) = str_dup(buf1);
		   send_to_char("You set your webpage.\r\n", ch);
		}
                return;
	}
	if( strcmp(arg, "personal") == 0 ) {
		if( GET_PERSONAL(ch))
			free(GET_PERSONAL(ch));
		if( strcmp(buf1, "clear") == 0) {
		  GET_PERSONAL(ch) = str_dup("None");
		  send_to_char("You clear your personal info.\r\n",ch);
		}
		else {
		     GET_PERSONAL(ch) = str_dup(buf1);
		     send_to_char("You set your personal info.\r\n", ch);
		}
		return;
	}

	send_to_char("Set what?!\r\n", ch);

}


char *show_object_damage(struct obj_data *obj) {

	char *damage_msgs[] = {
		"It is in &gperfect&n condition.",
		"It is &Yslightly damaged&n.",
		"It is &ymoderately damaged&n.",
		"It is &rquite damaged&n.",
		"It is &Rseverely damaged&n.",
		"It is &mbroken&n.",
		"It is &Gindestructable!&n"
        };
	int dmgno = 0;
	float damage;

	if (obj == NULL) {
	  mudlog("SYSERR: Showing object damage on null object.", BRF, LVL_GRGOD, TRUE);
	  return; 
	}

	if (GET_OBJ_MAX_DAMAGE(obj) == 0)
	    dmgno = 5;
	else if (GET_OBJ_MAX_DAMAGE(obj) <= -1)
	    dmgno = 6;
	else
	    damage = (GET_OBJ_DAMAGE(obj) *100) / GET_OBJ_MAX_DAMAGE(obj);

        if (dmgno != 0)
		dmgno = dmgno;
	else if (damage == 0) 
	   dmgno = 5;
        else if (damage <= 20)
	   dmgno = 4;
	else if (damage <= 50)
	   dmgno = 3;
	else if (damage <= 70)
	   dmgno = 2;
	else if (damage <= 95)
	   dmgno = 1;
	else dmgno = 0;

	log("Item %s, %d*100/%d = %lf", obj->short_description, GET_OBJ_DAMAGE(obj),
		GET_OBJ_MAX_DAMAGE(obj), damage);

	return damage_msgs[dmgno];
}


/*
 * This function screams bitvector... -gg 6/45/98
 */
void show_obj_to_char(struct obj_data * object, struct char_data * ch,
			int mode)
{
  *buf = '\0';
  if ((mode == 0) && object->description) {
    strcpy(buf, object->description);
  }
  else if (object->short_description && ((mode == 1) ||
				 (mode == 2) || (mode == 3) || (mode == 4))) {
    strcpy(buf,"&5");
    strcat(buf, object->short_description);
  }
  else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
      if (object->action_description) {
	strcpy(buf, "There is something written upon it:\r\n\r\n");
	strcat(buf, object->action_description);
	page_string(ch->desc, buf, 1);
      } else
	send_to_char("It's blank.\r\n", ch);
      show_object_damage(object);
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else			/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");

  }
  show_object_damage(object);
  if (mode != 3) {
    if( IS_OBJ_STAT(object, ITEM_HIDDEN))
      strcat(buf, " (hidden)");
    if (OBJ_RIDDEN(object))
	strcat(buf, " (ridden)");
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE))
      strcat(buf, " (invisible)");
    if (IS_OBJ_STAT(object, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
      strcat(buf, " ..It glows blue!");
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
      strcat(buf, " ..It glows yellow!");
    if (IS_OBJ_STAT(object, ITEM_GLOW))
      strcat(buf, " ..It has a soft glowing aura!");
    if (IS_OBJ_STAT(object, ITEM_HUM))
      strcat(buf, " ..It emits a faint humming sound!");
  }
  strcat(buf, "&n\r\n");
  page_string(ch->desc, buf, TRUE);
}


void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
		           int show)
{
  struct obj_data *i;
  bool found = FALSE;

  for (i = list; i; i = i->next_content) {
    if (CAN_SEE_OBJ(ch, i)) {
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}


void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  if (IS_NPC(ch))
    strcpy(buf,"&6");
  else 
    strcpy(buf,"&7");
  strcat(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " is in excellent condition.&n\r\n");
  else if (percent >= 90)
    strcat(buf, " has a few scratches.&n\r\n");
  else if (percent >= 75)
    strcat(buf, " has some small wounds and bruises.&n\r\n");
  else if (percent >= 50)
    strcat(buf, " has quite a few wounds.&n\r\n");
  else if (percent >= 30)
    strcat(buf, " has some big nasty wounds and scratches.&n\r\n");
  else if (percent >= 15)
    strcat(buf, " looks pretty hurt.&n\r\n");
  else if (percent >= 0)
    strcat(buf, " is in awful condition.&n\r\n");
  else
    strcat(buf, " is bleeding awfully from big wounds.&n\r\n");

  send_to_char(buf, ch);
}


void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found;
  struct obj_data *tmp_obj;
  struct char_data *tch;

  if (!ch->desc)
    return;

  if (!IS_NPC(i) && IS_SET(GET_SPECIALS(i), SPECIAL_DISGUISE) && CHAR_DISGUISED(i) &&
    !IS_SET(PRF_FLAGS(i), PRF_HOLYLIGHT))
  {
	tch = read_mobile(CHAR_DISGUISED(i), VIRTUAL);
	char_to_room(tch, i->in_room);
	look_at_char(tch, ch);
	extract_char(tch);
	return;       	
  }

   if (i->player.description)
    send_to_char(i->player.description, ch);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found) {
    send_to_char("\r\n", ch);	/* act() does capitalization. */
    act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
	send_to_char(where[j], ch);
	show_obj_to_char(GET_EQ(i, j), ch, 1);
      }
  }
  if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_IMMORT)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, 1);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char("You can't see anything.\r\n", ch);
  }
}

void list_rider(struct char_data *i, struct char_data *ch, int mode) {

	if (CAN_SEE(ch, i)) {
	   if (MOUNTING(i)) {
	      if (IS_NPC(i)) {
		sprintf(buf2, "...ridden by %s.", (MOUNTING(i) == ch ? "you" : (CAN_SEE(ch,MOUNTING(i))
			 ? GET_NAME(MOUNTING(i)) : "someone")) );
		if (mode == 1)
		    strcat(buf, buf2);
		if (mode == 0)
		    act(buf2, FALSE, i, 0, ch, TO_VICT);
	      }
	      else {
		if (mode == 1)
		   strcat(buf2, " (mounted)");
		if (mode == 0)
		   act( " (mounted)", FALSE, i, 0, ch, TO_VICT);
	      }
	   }
	}

}


void list_one_char(struct char_data * i, struct char_data * ch)
{
  const char *positions[] = {
    " &yis lying here, dead.",
    " &yis lying here, mortally wounded.",
    " &yis lying here, incapacitated.",
    " &yis lying here, stunned.",
    " &yis sleeping here.",
    " &yis resting here.",
    " &yis sitting here.",
    "!FIGHTING!",
    " &yis standing here."
  };

  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      strcpy(buf, "*");
    else
      *buf = '\0';

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	strcat(buf, "&r(Red Aura)&n ");
      else if (IS_GOOD(i))
	strcat(buf, "&b(Blue Aura)&n ");
    }
    if (!IS_NPC(i))
	list_rider(i, ch, 1);

    strcat(buf, "&7");
    strcat(buf, i->player.long_descr);
    send_to_char(buf, ch);

    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_REFLECT))
      act("...You can see your reflection in $s skin!",FALSE,i,0,ch,TO_VICT);
    if (IS_NPC(i))
	list_rider(i, ch, 0);
    return;
  }
  if (IS_NPC(i)) {
    strcpy(buf, "&7");
    strcat(buf, i->player.short_descr);
    CAP(buf);
  } else
/* make it so ya can see they are a wolf/vampire - Vader */
    if(PRF_FLAGGED(i,PRF_WOLF) && affected_by_spell(i,SPELL_CHANGED))
      sprintf(buf,"%s the Werewolf",i->player.name);
    else if(PRF_FLAGGED(i,PRF_VAMPIRE) && affected_by_spell(i,SPELL_CHANGED))
      sprintf(buf,"%s %s",i->player.name,
              (GET_SEX(i) == SEX_MALE ? "the Vampire" : "the Vampiress"));
    else
      sprintf(buf, "%s %s", i->player.name, GET_TITLE(i));

  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    strcat(buf, " (invisible)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    strcat(buf, " (linkless)");
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
    strcat(buf, " (writing)");

  if (GET_POS(i) != POS_FIGHTING)
    if(IS_AFFECTED(i,AFF_FLY))
      strcat(buf, " &yis floating here.");
    else
      strcat(buf, positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      strcat(buf, " &yis here, fighting ");
      if (FIGHTING(i) == ch)
	strcat(buf, "YOU!");
      else {
	if (i->in_room == FIGHTING(i)->in_room)
	  strcat(buf, PERS(FIGHTING(i), ch));
	else
	  strcat(buf, "someone who has already left");
	strcat(buf, "!");
      }
    } else			/* NIL fighting pointer */
      strcat(buf, " &yis here struggling with thin air.");
  }

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      strcat(buf, " &r(Red Aura)&n");
    else if (IS_GOOD(i))
      strcat(buf, " &b(Blue Aura)&n");
  }
  if (!IS_NPC(i))
	list_rider(i, ch, 1);

  strcat(buf, "&n\r\n");
  send_to_char(buf, ch);

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
  if (IS_AFFECTED(i, AFF_BLIND))
    act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
  if (IS_AFFECTED(i, AFF_REFLECT))
    act("...You can see your reflection in $s skin!",FALSE,i,0,ch,TO_VICT);
  if (IS_NPC(i))
	list_rider(i, ch, 0);
}



void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i) {
      if (CAN_SEE(ch, i))
	list_one_char(i, ch);
      else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) &&
	       AFF_FLAGGED(i, AFF_INFRAVISION))
	send_to_char("You see a pair of glowing red eyes looking your way.\r\n", ch);
    }
}


void do_auto_exits(struct char_data * ch)
{
  int door, slen = 0;

  *buf = '\0';

  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF))
    do_exits(ch, 0, 0, 0);

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
      slen += sprintf(buf + slen, "%c ", LOWER(*dirs[door]));

  sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM),
	  *buf ? buf : "None! ", CCNRM(ch, C_NRM));

  send_to_char(buf2, ch);
}


ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
      if (GET_LEVEL(ch) >= LVL_IS_GOD)
	sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door],
		GET_ROOM_VNUM(EXIT(ch, door)->to_room),
		world[EXIT(ch, door)->to_room].name);
      else {
	sprintf(buf2, "%-5s - ", dirs[door]);
	if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
	  strcat(buf2, "Too dark to tell\r\n");
	else {
	  strcat(buf2, world[EXIT(ch, door)->to_room].name);
	  strcat(buf2, "\r\n");
	}
      }
      strcat(buf, CAP(buf2));
    }
  send_to_char("Obvious exits:\r\n", ch);

  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
}



void look_at_room(struct char_data * ch, int ignore_brief)
{
  if (!ch->desc)
    return;

  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
  }
  send_to_char(CCCYN(ch, C_NRM), ch);
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintbit(ROOM_FLAGS(ch->in_room), room_bits, buf);
    sprintf(buf2, "[%5d] %s [ %s]", GET_ROOM_VNUM(IN_ROOM(ch)),
	    world[ch->in_room].name, buf);
    send_to_char(buf2, ch);
  } else
    send_to_char(world[ch->in_room].name, ch);

  send_to_char(CCNRM(ch, C_NRM), ch);
  send_to_char("\r\n", ch);

  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
    send_to_char(world[ch->in_room].description, ch);

  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(ch);

  /* now list characters & objects */
  //send_to_char(CCGRN(ch, C_NRM), ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  //send_to_char(CCYEL(ch, C_NRM), ch);
  list_char_to_char(world[ch->in_room].people, ch);
  if(HUNTING(ch)) {
    send_to_char(CCRED(ch, C_NRM), ch);
    do_track(ch, "", 0, SCMD_AUTOHUNT);
    }
  send_to_char(CCNRM(ch, C_NRM), ch);
}



void look_in_direction(struct char_data * ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(EXIT(ch, dir)->general_description, ch);
    else
      send_to_char("You see nothing special.\r\n", ch);

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    } else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    }
  } else
    send_to_char("Nothing special there...\r\n", ch);
}



void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Look in what?\r\n", ch);
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				 FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    send_to_char("There's nothing inside that!\r\n", ch);
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
	send_to_char("It is closed.\r\n", ch);
      else {
	send_to_char(fname(obj->name), ch);
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(" (carried): \r\n", ch);
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(" (here): \r\n", ch);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(" (used): \r\n", ch);
	  break;
	}

	list_obj_to_char(obj->contains, ch, 2, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
	send_to_char("It is empty.\r\n", ch);
      else {
	if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) {
	  sprintf(buf, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
	  amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
	  sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
	  sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
	}
	send_to_char(buf, ch);
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = FALSE, j, fnum, i = 0;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg))) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL && ++i == fnum) {
    page_string(ch->desc, desc, FALSE);
    return;
  }

  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum) {
	send_to_char(desc, ch);
	send_to_char(show_object_damage(obj), ch);
	found = TRUE;
      }

  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	send_to_char(desc, ch);
	send_to_char(show_object_damage(obj), ch);
	found = TRUE;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	send_to_char(desc, ch);
	send_to_char(show_object_damage(obj), ch);
	found = TRUE;
      }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
    else
      show_obj_to_char(found_obj, ch, 6);	/* Find hum, glow etc */
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);

  desc = NULL;
}


ACMD(do_look)
{
  char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);	/* glowing red eyes */
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Read what?\r\n", ch);
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}



ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg);

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char("When you look inside, you see:\r\n", ch);
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char("You're broke!\r\n", ch);
  else if (GET_GOLD(ch) == 1)
    send_to_char("You have one miserable little gold coin.\r\n", ch);
  else {
    sprintf(buf, "You have &Y%d&n gold coins.\r\n", GET_GOLD(ch));
    send_to_char(buf, ch);
  }
}

// DM - TODO - redesign the score screen.
ACMD(do_score)
{
  struct time_info_data playing_time;
  bool foundDamBonus = FALSE;
  float nTmp = 0;

  if (IS_NPC(ch))
    return;

  sprintf(buf, "You are %d years old.", GET_AGE(ch));

  if (age(ch)->month == 0 && age(ch)->day == 0)
    strcat(buf, "  It's your birthday today.\r\n");
  else
    strcat(buf, "\r\n");

  sprintf(buf + strlen(buf),
       "You have %d(%d) hit, %d(%d) mana and %d(%d) movement points.\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  sprintf(buf + strlen(buf),
	"Your bonus to hit is %d, while your bonus to damage is %d.\r\n", 
		GET_HITROLL(ch), GET_DAMROLL(ch));

  if (IS_SET(GET_SPECIALS(ch), SPECIAL_MINOTAUR | SPECIAL_SUPERMAN))
	foundDamBonus = TRUE;

  if (foundDamBonus)
  {
	if (IS_SET(GET_SPECIALS(ch), SPECIAL_MINOTAUR))
		nTmp += (GET_DAMROLL(ch) * 0.05);

	if (IS_SET(GET_SPECIALS(ch), SPECIAL_SUPERMAN))
		nTmp += (GET_DAMROLL(ch) * 0.02);

	sprintf(buf + strlen(buf), "However, your effective damage bonus is %d.\r\n",
		GET_DAMROLL(ch) + (int)(nTmp+1));
  }

  sprintf(buf + strlen(buf), "Your armor class is %d/10, and your alignment is %d.\r\n",
	  compute_armor_class(ch), GET_ALIGNMENT(ch));


  sprintf(buf + strlen(buf), "You have scored %d exp, and have %d gold coins.\r\n",
	  GET_EXP(ch), GET_GOLD(ch));

  if (GET_LEVEL(ch) < LVL_IMMORT)
    sprintf(buf + strlen(buf), "You need %d exp to reach your next level.\r\n",
	level_exp(ch, GET_LEVEL(ch)) - GET_EXP(ch));

  playing_time = *real_time_passed((time(0) - ch->player.time.logon) +
				  ch->player.time.played, 0);
  sprintf(buf + strlen(buf), "You have been playing for %d day%s and %d hour%s.\r\n",
     playing_time.day, playing_time.day == 1 ? "" : "s",
     playing_time.hours, playing_time.hours == 1 ? "" : "s");

  sprintf(buf + strlen(buf), "This ranks you as %s %s (level %d).\r\n",
	  GET_NAME(ch), GET_TITLE(ch), GET_LEVEL(ch));

  switch (GET_POS(ch)) {
  case POS_DEAD:
    strcat(buf, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    strcat(buf, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    strcat(buf, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    strcat(buf, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    strcat(buf, "You are resting.\r\n");
    break;
  case POS_SITTING:
    strcat(buf, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    if (FIGHTING(ch))
      sprintf(buf + strlen(buf), "You are fighting %s.\r\n",
		PERS(FIGHTING(ch), ch));
    else
      strcat(buf, "You are fighting thin air.\r\n");
    break;
  case POS_STANDING:
    strcat(buf, "You are standing.\r\n");
    break;
  default:
    strcat(buf, "You are floating.\r\n");
    break;
  }

  if (EXT_FLAGGED(ch, EXT_AUTOLOOT))
    strcat(buf, "You are autolooting corpses.\r\n");
 
  if (EXT_FLAGGED(ch, EXT_AUTOGOLD)) {
    strcat(buf, "You are autolooting gold from corpses.\r\n");
    if (EXT_FLAGGED(ch, EXT_AUTOSPLIT))
      strcat(buf, "You are autospliting gold between group members.\r\n");
  }
 
  if (AUTOASSIST(ch)) {
    sprintf(buf2, "You are autoassisting %s.\r\n",GET_NAME(AUTOASSIST(ch)));
    strcat(buf,buf2);
  } 

  if (GET_COND(ch, DRUNK) > 10)
    strcat(buf, "You are intoxicated.\r\n");

  if (GET_COND(ch, FULL) == 0)
    strcat(buf, "You are hungry.\r\n");

  if (GET_COND(ch, THIRST) == 0)
    strcat(buf, "You are thirsty.\r\n");

  if (AFF_FLAGGED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");

  if (AFF_FLAGGED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");

  if (AFF_FLAGGED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");

  if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");

  send_to_char(buf, ch);
}

ACMD(do_affects)
{
  struct affected_type *aff;
 
  strcpy(buf,"");
 
// DM - TODO - check what we want to be given in text

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");

  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");

  if (IS_AFFECTED(ch, AFF_WATERWALK))
    strcat(buf, "You have magically webbed feet.\r\n");

  if (IS_AFFECTED(ch, AFF_WATERBREATHE))
    strcat(buf, "You can breathe underwater.\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");

  if (affected_by_spell(ch, SPELL_STRENGTH))
    strcat(buf, "You feel stronger.\r\n");

  if (affected_by_spell(ch, SPELL_CHILL_TOUCH))
    strcat(buf, "You feel weakened.\r\n");

  if (IS_AFFECTED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");

  if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");

  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");

  if (IS_AFFECTED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");

  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");

  if (affected_by_spell(ch, SPELL_BLESS))
    strcat(buf, "You feel righteous.\r\n");

  if (IS_AFFECTED(ch, AFF_CURSE))
    strcat(buf, "You are cursed.\r\n");

  if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
    strcat(buf, "You feel more aware.\r\n");

  if (IS_AFFECTED(ch, AFF_SNEAK))
    strcat(buf, "You feel more sneaky.\r\n");

  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN))
    strcat(buf, "You have the ability to see auras.\r\n");

  if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
    strcat(buf, "You are more sensitive to magic.\r\n");

  if (IS_AFFECTED(ch, AFF_PROTECT_EVIL))
    strcat(buf, "You feel protected from Evil.\r\n");

  if (IS_AFFECTED(ch, AFF_PROTECT_GOOD))
     strcat(buf, "You feel protected from Good.\r\n");

  if (IS_AFFECTED(ch, AFF_SLEEP))
     strcat(buf, "You are asleep.\r\n");

  if (IS_AFFECTED(ch, AFF_REFLECT))
     strcat(buf, "You have shiny scales on your skin.\r\n");

  if (IS_AFFECTED(ch, AFF_FLY))
     strcat(buf, "You are flying.\r\n");

  if (affected_by_spell(ch,SPELL_DRAGON))
     strcat(buf, "You feel like a dragon!\r\n");

  if (affected_by_spell(ch,SPELL_CHANGED))
    if(PRF_FLAGGED(ch,PRF_WOLF))
      strcat(buf, "You're a Werewolf!\r\n");
    else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
      strcat(buf, "You're a Vampire!\r\n");

  if (IS_AFFECTED(ch, AFF_PARALYZED))
     strcat(buf, "You are paralyzed.\r\n");

  if (MOUNTING(ch)) {
	sprintf(buf2, "You are mounted on %s.\r\n", GET_NAME(MOUNTING(ch)));
	strcat(buf, buf2);
  }
  if (MOUNTING_OBJ(ch)) {
	sprintf(buf2, "You are mounted on %s.\r\n", MOUNTING_OBJ(ch)->short_description);
	strcat(buf, buf2);
  }
 
  send_to_char(buf, ch);
        if (GET_LEVEL(ch) < 20)
                return;
 
        /* Routine to show what spells a char is affected by */
  if (ch->affected)
  {
    for (aff = ch->affected; aff; aff = aff->next)
    {
      *buf2 = '\0';
      if( aff->duration == -1 ) {
        sprintf(buf, "ABL: (Unlim) &c%-21s&n ", skill_name(aff->type) );
      }
      else {
        sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1,
          CCCYN(ch, C_NRM), skill_name(aff->type), CCNRM(ch, C_NRM));
      } 
      if (aff->modifier)
      {
        sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
        strcat(buf, buf2); 
      }
      if (aff->bitvector)
      {
        if (*buf2)
          strcat(buf, ", sets ");
        else
          strcat(buf, "sets ");
        sprintbit(aff->bitvector, affected_bits, buf2);
        strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }
}

ACMD(do_exp)
{
    if (GET_LEVEL(ch) < LVL_ISNOT_GOD){
      sprintf(buf, "You need &c%d&n exp to reach your next level.\r\n",
        level_exp(ch,GET_LEVEL(ch)) - GET_EXP(ch));
      send_to_char(buf, ch);
   }
} 

ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    send_to_char(where[i], ch);
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
//	send_to_char(where[i], ch);
	show_obj_to_char(GET_EQ(ch, i), ch, 1);
	found = TRUE;
      } else {
//	send_to_char(where[i], ch);
	send_to_char("Something.\r\n", ch);
	found = TRUE;
      }
    } else {
      send_to_char("Nothing.\r\n", ch);
    }
  }
//  if (!found) {
//    send_to_char(" Nothing.\r\n", ch);
//  }
}


ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  sprintf(buf, "It is %d o'clock %s, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  day = time_info.day + 1;	/* day in [1..35] */

  if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);

  send_to_char(buf, ch);
}


ACMD(do_weather)
{
  const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if (OUTSIDE(ch)) {
    sprintf(buf, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    (weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
}

/* command to tell ya what the moon is - Vader */
ACMD(do_moon)
{
  extern struct time_info_data time_info;
  byte day = time_info.day % 3;
 
  if(weather_info.moon == MOON_NONE) {
    send_to_char("It is a moonless night.\r\n",ch);
    return;
    }
 
  if(!(time_info.hours > 12)) {
    day--;
    if(day < 0)
      day = 2;
    }
 
  switch(day) {
    case 0:
      sprintf(buf,"It is the 1st night of the %s moon.\r\n",moon_mesg[weather_info.moon]);
      break;
    case 1:
      sprintf(buf,"It is the 2nd night of the %s moon.\r\n",moon_mesg[weather_info.moon]);
      break;
    case 2:
      sprintf(buf,"It is the 3rd night of the %s moon.\r\n",moon_mesg[weather_info.moon]);
      break;
    }
 
  send_to_char(buf,ch);
} 

ACMD(do_help)
{
  int chk, bot, top, mid, minlen, count = 0;
  char *helpstring;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) {
    send_to_char("No help available.\r\n", ch);
    return;
  }

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char("There is no help on that word.\r\n", ch);
      return;
    } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      
      // DM - check the help level restriction 
      // TODO - if restricted try and find an unrestricted match before failing 
      if (GET_LEVEL(ch) < help_table[mid].level_restriction) {
        send_to_char("Sorry, you don't have access to that help.\r\n",ch);
        return;
      }
      
      /* Bypass the keywords */
      helpstring = help_table[mid].entry;      
      while(helpstring[count] != '\n')
        count++;
      
      page_string(ch->desc, &helpstring[count+1], 0);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}



#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-c classlist] [-s] [-o] [-q] [-r] [-z]\r\n"

ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH];
  char mode, s[5];
  size_t i;
  int low = 0, high = LVL_OWNER, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0;

  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';

  while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);       /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = 1;
	strcpy(buf, buf1);
	break;
      case 'c':
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      default:
	send_to_char(WHO_FORMAT, ch);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(WHO_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */

  send_to_char("Players\r\n-------\r\n", ch);

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING)
      continue;

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[ch->in_room].zone != world[tch->in_room].zone)
      continue;
    if (who_room && (tch->in_room != ch->in_room))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
    if (short_list) {
      sprintf(buf, "%s[%3d %s] %-12.12s%s%s",
      	      (GET_LEVEL(tch) >= LVL_GOD ? CCBYEL(ch,C_SPR) :
              (GET_LEVEL(tch) >= LVL_ANGEL ? CCCYN(ch,C_SPR) :
              (GET_LEVEL(tch) >= LVL_IMMORT ? CCRED(ch,C_SPR) :
              (GET_LEVEL(tch) >= LVL_ETRNL1 ? CCBLU(ch, C_SPR) : "")))),
              GET_LEVEL(tch), CLASS_ABBR(tch), GET_NAME(tch),
              (GET_LEVEL(tch) >= LVL_IS_GOD || GET_LEVEL(tch) >= LVL_ETRNL1 ? CCNRM(ch, C_SPR) : ""),
              ((!(++num_can_see % 4)) ? "\r\n" : ""));
      send_to_char(buf, ch);
    } else {
      num_can_see++;

      sprintf(s,"%s",
              (GET_LEVEL(tch) == LVL_OWNER ? "ONR" :
               (GET_LEVEL(tch) >= LVL_GRIMPL ? "BCH" :
               (GET_LEVEL(tch) >= LVL_IMPL ? "IMP" :
               (GET_LEVEL(tch) >= LVL_GRGOD ? "GOD" :
               (GET_LEVEL(tch) >= LVL_GOD ? "DEI" :
               (GET_LEVEL(tch) >= LVL_ANGEL ? "ANG" :
               (GET_LEVEL(tch) >= LVL_IMMORT ? "IMM" : 
               (GET_LEVEL(tch) == LVL_ETRNL9 ? "ET9" : 
               (GET_LEVEL(tch) == LVL_ETRNL8 ? "ET8" : 
               (GET_LEVEL(tch) == LVL_ETRNL7 ? "ET7" : 
               (GET_LEVEL(tch) == LVL_ETRNL6 ? "ET6" : 
               (GET_LEVEL(tch) == LVL_ETRNL5 ? "ET5" : 
               (GET_LEVEL(tch) == LVL_ETRNL4 ? "ET4" : 
               (GET_LEVEL(tch) == LVL_ETRNL3 ? "ET3" : 
               (GET_LEVEL(tch) == LVL_ETRNL2 ? "ET2" : 
               (GET_LEVEL(tch) == LVL_ETRNL1 ? "ET1" : "PLR"))))))))))))))))); 

      sprintf(buf, "%s[%3d %c %s %s] %s %s%s",
              (GET_LEVEL(tch) >= LVL_GOD ? CCBYEL(ch, C_SPR) :
               (GET_LEVEL(tch) >= LVL_ANGEL ? CCCYN(ch,C_SPR) :
               (GET_LEVEL(tch) >= LVL_IMMORT ? CCBLU(ch,C_SPR) : ""))),
               // (GET_LEVEL(tch) >= LVL_CHAMP ? CCBLU(ch,C_SPR) : "")))),
              GET_LEVEL(tch),
              (GET_SEX(tch)==SEX_MALE ? 'M' :
               GET_SEX(tch)==SEX_FEMALE ? 'F' : '-'),
              CLASS_ABBR(tch),
              s,
              GET_NAME(tch),
              GET_TITLE(tch),
              (GET_LEVEL(tch) >= LVL_GOD ? CCBYEL(ch, C_SPR) :
               (GET_LEVEL(tch) >= LVL_ANGEL ? CCCYN(ch,C_SPR) :
               (GET_LEVEL(tch) >= LVL_IMMORT ? CCBLU(ch,C_SPR) : ""))));
               // (GET_LEVEL(tch) >= LVL_CHAMP ? CCBLU(ch,C_SPR) : ""))))); 

      if (GET_CLAN_NUM(tch) >= 0) {
        if (EXT_FLAGGED(tch, EXT_LEADER)) 
          strcat(buf, CCBLU(ch, C_SPR));
        else if (EXT_FLAGGED(tch, EXT_SUBLEADER))
          strcat(buf, CCCYN(ch, C_SPR));
      
        sprintf(buf, "%s [%s - %s]&n", buf, get_clan_disp(tch), get_clan_rank(tch));
      }
 
      if (GET_INVIS_LEV(tch)) {
        // Standard invis?
        if (GET_INVIS_TYPE(tch) == INVIS_NORMAL)
          sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(tch));
        // Level invis?
        else if (GET_INVIS_TYPE(tch) == INVIS_SINGLE)
          sprintf(buf, "%s (i%ds)", buf, GET_INVIS_LEV(tch));
        // Player invis?
        else if (GET_INVIS_TYPE(tch) == INVIS_SPECIFIC)
          sprintf(buf, "%s (i-char)", buf);
        else
        // Ranged invis!
          sprintf(buf, "%s (i%d - %d)", buf, GET_INVIS_LEV(tch), GET_INVIS_TYPE(tch)); 
      } else if (IS_AFFECTED(tch, AFF_INVISIBLE))
        strcat(buf, " (invis)"); 

      if (PLR_FLAGGED(tch, PLR_MAILING))
	strcat(buf, " (mailing)");
      else if (PLR_FLAGGED(tch, PLR_WRITING))
	strcat(buf, " (writing)");

      if ( GET_IGN_NUM(tch) > 0 || GET_IGN_LEVEL(tch) > 0)
        strcat(buf, " (snob)");
      if (PRF_FLAGGED(tch, PRF_AFK))
        strcat(buf, " (AFK)");
      if (PRF_FLAGGED(tch, PRF_DEAF))
	strcat(buf, " (deaf)");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	strcat(buf, " (notell)");
      if (PRF_FLAGGED(tch, PRF_QUEST))
	strcat(buf, " (quest)");
      if (PLR_FLAGGED(tch, PLR_THIEF))
	strcat(buf, " (THIEF)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
	strcat(buf, " (KILLER)");
      if (PRF_FLAGGED(tch, PRF_TAG))
        strcat(buf, " (tag)");     
      if (GET_LEVEL(tch) >= LVL_ETRNL1)
        strcat(buf, CCNRM(ch, C_SPR));             
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }				/* endif shortlist */
  }				/* end of for */
  if (short_list && (num_can_see % 4))
    send_to_char("\r\n", ch);
  if (num_can_see == 0)
    sprintf(buf, "\r\nNo-one at all!\r\n");
  else if (num_can_see == 1)
    sprintf(buf, "\r\nOne lonely character displayed.\r\n");
  else
    sprintf(buf, "\r\n%d characters displayed.\r\n", num_can_see);
  send_to_char(buf, ch);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
  const char *format = "%3d %-7s %-12s %-14s %-3s %-8s ";
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  size_t i;
  int low = 0, high = LVL_OWNER-1, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      case 'c':
	playing = 1;
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      default:
	send_to_char(USERS_FORMAT, ch);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */
  strcpy(line,
	 "Num Class    Name         State          Idl Login@   Site\r\n");
  strcat(line,
	 "--- -------- ------------ -------------- --- -------- ------------------------\r\n");
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (IS_PLAYING(d)) { 
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;

      if (d->original)
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->original),
		CLASS_ABBR(d->original));
      else
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character));
    } else
      strcpy(classname, "   -   ");

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (STATE(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[STATE(d)]);

    //if (d->character && STATE(d) == CON_PLAYING && (GET_LEVEL(d->character) < LVL_GOD)
    if (d->character && STATE(d) == CON_PLAYING && (GET_LEVEL(d->character) <= GET_LEVEL(ch)))
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    if (d->character && d->character->player.name) {
      if (d->original)
	sprintf(line, format, d->desc_num, classname,
		d->original->player.name, state, idletime, timeptr);
      else
	sprintf(line, format, d->desc_num, classname,
		d->character->player.name, state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED",
	      state, idletime, timeptr);

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (STATE(d) != CON_PLAYING) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (STATE(d) != CON_PLAYING ||
		(STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
      send_to_char(line, ch);
      num_can_see++;
    }
  }

  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(strcat(strcpy(buf, circlemud_version), "\r\n"), ch);
    break;
  case SCMD_WHOAMI:
    send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
    break;
  case SCMD_AREAS:
    page_string(ch->desc, areas, 0);
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    return;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{
  int zone_name_len, j;  
  register struct char_data *i;
  register struct descriptor_data *d;
  extern struct zone_data *zone_table; 

  if (!*arg) {
//    send_to_char("Players in your Zone\r\n--------------------\r\n", ch);

    zone_name_len=strlen(zone_table[world[ch->in_room].zone].name);
    sprintf(buf,"Players in &R%s&n\r\n", zone_table[world[ch->in_room].zone].name);
    send_to_char(buf,ch);
    for (j=0; j < (11+zone_name_len); j++)
      send_to_char("-",ch);
    send_to_char("\r\n",ch); 

    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (i->in_room == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[ch->in_room].zone != world[i->in_room].zone)
	continue;
      sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (i->in_room == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[i->in_room].zone != world[ch->in_room].zone)
	continue;
      if (!isname(arg, i->player.name))
	continue;
      sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
      return;
    }
    send_to_char("No-one around by that name.\r\n", ch);
  }
}


void print_object_location(int num, struct obj_data * obj, struct char_data * ch,
			        int recur)
{
  if (num > 0)
    sprintf(buf, "&bO&n%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(buf, "%33s", " - ");

  if (obj->in_room > NOWHERE) {
    sprintf(buf + strlen(buf), "&c[%5d]&n %s\r\n",
	    GET_ROOM_VNUM(IN_ROOM(obj)), world[obj->in_room].name);
    send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(buf + strlen(buf), "carried by %s\r\n",
	    PERS(obj->carried_by, ch));
    send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(buf + strlen(buf), "worn by %s\r\n",
	    PERS(obj->worn_by, ch));
    send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(buf + strlen(buf), "inside %s%s\r\n",
	    obj->in_obj->short_description, (recur ? ", which is" : " "));
    send_to_char(buf, ch);
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else {
    sprintf(buf + strlen(buf), "in an unknown location\r\n");
    send_to_char(buf, ch);
  }
}



void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char("&gPlayers&n\r\n-------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (STATE(d) == CON_PLAYING) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
	  if (d->original)
	    sprintf(buf, "%-20s - &c[%5d]&n %s (in %s)\r\n",
		    GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		 world[d->character->in_room].name, GET_NAME(d->character));
	  else
	    sprintf(buf, "%-20s - &c[%5d]&n %s\r\n", GET_NAME(i),
		    GET_ROOM_VNUM(IN_ROOM(i)), world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	sprintf(buf, "&rM&n%3d. %-25s - &c[%5d]&n %s\r\n", ++num, GET_NAME(i),
		GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	send_to_char(buf, ch);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char("Couldn't find any such thing.\r\n", ch);
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}

// DM - TODO - decide on levels format

ACMD(do_levels)
{
  int i;

  if (IS_NPC(ch)) {
    send_to_char("You ain't nothin' but a hound-dog.\r\n", ch);
    return;
  }
  *buf = '\0';

  for (i = 1; i < LVL_IMMORT-1; i++) {
    if (i >= LVL_ETRNL1-1)
      sprintf(buf + strlen(buf), "&g[&m%3d - %2d&g]&n &c%9d&n", i, i+1, level_exp(ch, i));
    else
      sprintf(buf + strlen(buf), "&g[&n%3d - %2d&g]&n &c%9d&n", i, i+1, level_exp(ch, i));
    strcat(buf, "\r\n");
  }
  sprintf(buf + strlen(buf), "&g[   &b%3d&g  ]&n &c%9d&n : &bImmortality&n\r\n",
	  LVL_IMMORT, level_exp(ch, LVL_IMMORT-1));
  page_string(ch->desc, buf, 1);
}



ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("Easy!  Very easy indeed!\r\n", ch);
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char("Now where did that chicken go?\r\n", ch);
  else if (diff <= -5)
    send_to_char("You could do it with a needle!\r\n", ch);
  else if (diff <= -2)
    send_to_char("Easy.\r\n", ch);
  else if (diff <= -1)
    send_to_char("Fairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\r\n", ch);
  else if (diff <= 1)
    send_to_char("You would need some luck!\r\n", ch);
  else if (diff <= 2)
    send_to_char("You would need a lot of luck!\r\n", ch);
  else if (diff <= 3)
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 5)
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  else if (diff <= 10)
    send_to_char("Are you mad!?\r\n", ch);
  else if (diff <= 100)
    send_to_char("You ARE mad!\r\n", ch);

}



ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
      send_to_char(NOPERSON, ch);
    else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}


const char *ctypes[] = {
  "off", "sparse", "normal", "complete", "\n"
};

ACMD(do_color)
{
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

  sprintf(buf, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
	  CCNRM(ch, C_OFF), ctypes[tp]);
  send_to_char(buf, ch);
}


/* DM - modified for new toggles and to toggle others and from file */
ACMD(do_toggle)
{
  struct char_data *victim;
  struct char_file_u tmp_store;
 
  if (IS_NPC(ch))
    return;
 
  half_chop(argument, buf1, buf2);
 
  if ((!*buf1) || (GET_LEVEL(ch) < LVL_IMMORT)) {
    toggle_display(ch,ch);
    return;
  }
 
  if (is_abbrev(buf1, "file")) {
    if (!*buf2) {
      send_to_char("Toggle's for who?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1) {
        store_to_char(&tmp_store, victim);
        if (GET_LEVEL(victim) > GET_LEVEL(ch))
          send_to_char("Sorry, you can't do that.\r\n", ch);
        else
          toggle_display(ch, victim);
        free_char(victim);
      } else {
        send_to_char("There is no such player.\r\n", ch);
        free(victim);
      }
    }
  } else {
    if ((victim = get_char_vis(ch,buf1,TRUE))) {
      toggle_display(ch,victim);
    } else {
      send_to_char("Toggle's for who?\r\n", ch);
    }  
  }
}
 
/* DM - Toggle display on vict */
void toggle_display(struct char_data *ch, struct char_data *vict)
{ 
  if (IS_NPC(ch))
    return;
  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");
  else
    sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

  sprintf(buf,
          " Character Name: %s\r\n\r\n"
 
          "Hit Pnt Display: %-3s    " "     Brief Mode: %-3s    " " Summon Protect: %-3s\r\n"
          "   Move Display: %-3s    " "   Compact Mode: %-3s    " "       On Quest: %-3s\r\n"
          "   Mana Display: %-3s    " " Auto Show Exit: %-3s    " " Gossip Channel: %-3s\r\n"
          "    Exp Display: %-3s    " "   Repeat Comm.: %-3s    " "Auction Channel: %-3s\r\n"
          "  Align Display: %-3s    " "           Deaf: %-3s    " "  Grats Channel: %-3s\r\n"
          "       Autoloot: %-3s    " "         NoTell: %-3s    " " Newbie Channel: %-3s\r\n"
          "       Autogold: %-3s    " "     Marked AFK: %-3s    " "   Clan Channel: %-3s\r\n"
          "      Autosplit: %-3s    " " Clan Available: %-3s    " "   Info Channel: %-3s\r\n"
          "     Wimp Level: %-4s   " "    Color Level: %s\r\n",
 
          GET_NAME(vict),
 
          ONOFF(PRF_FLAGGED(vict, PRF_DISPHP)),
          ONOFF(PRF_FLAGGED(vict, PRF_BRIEF)),
          ONOFF(!PRF_FLAGGED(vict, PRF_SUMMONABLE)),
 
          ONOFF(PRF_FLAGGED(vict, PRF_DISPMOVE)),
          ONOFF(PRF_FLAGGED(vict, PRF_COMPACT)),
          YESNO(PRF_FLAGGED(vict, PRF_QUEST)), 

          ONOFF(PRF_FLAGGED(vict, PRF_DISPMANA)),
          ONOFF(PRF_FLAGGED(vict, PRF_AUTOEXIT)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOGOSS)),
 
          ONOFF(PRF_FLAGGED(vict, PRF_DISPEXP)),
          YESNO(!PRF_FLAGGED(vict, PRF_NOREPEAT)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOAUCT)),
 
          ONOFF(PRF_FLAGGED(vict, PRF_DISPALIGN)),
          YESNO(PRF_FLAGGED(vict, PRF_DEAF)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOGRATZ)),
 
          ONOFF(EXT_FLAGGED(vict, EXT_AUTOLOOT)),
          ONOFF(PRF_FLAGGED(vict, PRF_NOTELL)),
          ONOFF(!EXT_FLAGGED(vict, EXT_NONEWBIE)),
 
          ONOFF(EXT_FLAGGED(vict, EXT_AUTOGOLD)),
          YESNO(PRF_FLAGGED(vict, PRF_AFK)),
          ONOFF(!EXT_FLAGGED(vict, EXT_NOCTALK)),
 
          ONOFF(EXT_FLAGGED(vict, EXT_AUTOSPLIT)),
          YESNO(EXT_FLAGGED(vict, EXT_CLAN)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOINFO)),
 
          buf2,
          ctypes[COLOR_LEV(vict)]);
 
  send_to_char(buf, ch);

  if (GET_LEVEL(vict) >= LVL_ETRNL1) {
    sprintf(buf,
          "\r\n     Room Flags: %-3s    " " Wiznet Channel: %-3s\r\n"
          "      No Hassle: %-3s    " " Immnet Channel: %-3s\r\n"
          "     Holy Light: %-3s\r\n",
 
          ONOFF(PRF_FLAGGED(vict, PRF_ROOMFLAGS)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOWIZ)),
          ONOFF(PRF_FLAGGED(vict, PRF_NOHASSLE)),
          ONOFF(!PRF_FLAGGED(vict, PRF_NOIMMNET)),
          ONOFF(PRF_FLAGGED(vict, PRF_HOLYLIGHT)));
 
    send_to_char(buf,ch);
  } 
}


struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
  int a, b, tmp;

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  /* create data array */
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

  /* initialize it */
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }

  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}



ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char("You can't see the commands of people above your level.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  sprintf(buf, "The following &c%s%s&n are available to &b%s&n:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
    i = cmd_sort_info[cmd_num].sort_pos;
    if (cmd_info[i].minimum_level >= 0 &&
	GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
	(cmd_info[i].minimum_level >= LVL_ETRNL1) == wizhelp &&
	(wizhelp || socials == cmd_sort_info[i].is_social)) {
      sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
      if (!(no % 7))
	strcat(buf, "\r\n");
      no++;
    }
  }

  strcat(buf, "\r\n");

  page_string(ch->desc, buf, TRUE);
}

#undef CLAN_TABLE_ALL
#define CLAN_TABLE_LEADER
 
/* DM - display the clan tables */
ACMD(do_clan_table)
{
  struct clan_data *clan;
 
  one_argument(argument,arg);
 
  if (IS_NPC(ch))
    return;
 
  if (GET_LEVEL(ch) >= LVL_ANGEL)
    if (!*arg) {
      send_to_char("Which clan?\r\n",ch);
      return;
    } else {
      if (!(clan=get_clan_by_name(arg))) {
        send_to_char("That is not a clan, type 'clans' to see the list.\r\n",ch);
        return;
      } else {
        display_clan_table(ch,clan);
        return;
      }
    }
 
  #ifdef CLAN_TABLE_ALL
    if (!(clan=get_clan_by_num(GET_CLAN_NUM(ch)))) {
      send_to_char("You are not a clan member.\r\n",ch);
      return;
    } else {
      display_clan_table(ch,clan);
      return;
    }
  #endif
 
  #ifdef CLAN_TABLE_LEADER
    if ((GET_CLAN_NUM(ch) != 0) && (EXT_FLAGGED(ch, EXT_LEADER))) {
      if (!(clan=get_clan_by_num(GET_CLAN_NUM(ch)))) { 
        sprintf(buf,"Clan Error: Clan Numb %d, char %s",
                GET_CLAN_NUM(ch),GET_NAME(ch));
        return;
      } else {
        display_clan_table(ch,clan);
        return;
      }
    } else {
      send_to_char("Sorry you cannot do that.\r\n",ch);
      return;
    }
  #endif
 
  send_to_char("Sorry you cannot do that.\r\n",ch);
  return;
} 

/* DM - display the clans */
ACMD(do_clans)
{
  int i,j;
 
  sprintf(buf,"Current clans:\r\n");
  strcat (buf,"--------------\r\n");
 
  for (i=0; i < CLAN_NUM; i++) {
    sprintf(buf2,"&B%s&n", clan_info[i].disp_name);
    strcat(buf,buf2);
    for (j=0; j < (MAX_LEN-strlen(clan_info[i].disp_name)); j++)
      strcat(buf," ");
    sprintf(buf2,"(&B%s&n)\r\n",clan_info[i].name);
    strcat(buf,buf2);
  }
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_classes)
{
  int index;

  sprintf(buf,"Classes:\r\n");

  for (index=0; index < NUM_CLASSES; index++) {
    sprintf(buf2,"&B%s\r\n",CLASS_NAME(index));
    strcat(buf,buf2);
  }
  page_string(ch->desc, buf, TRUE);
}
 
void display_clan_table(struct char_data *ch, struct clan_data *clan)
{
  int i;
 
  if (!clan)
    return;
 
  sprintf(buf,"Members of the &B%s&n:\r\n",clan->disp_name);
  sprintf(buf2,"----------------");
  for (i=0; i < strlen(clan->disp_name); i++)
    strcat(buf2,"-");
 
  strcat(buf2,"\r\n");
  strcat(buf,buf2);
 
  sprintf(buf2,"%s",clan->table);
  strcat(buf,buf2);
 
  page_string(ch->desc,buf,TRUE);
}

// DM - set your sexy colours
ACMD(do_setcolour)
{
  int colour_code, colour_num, no_args = TRUE, i;
  char *usage="&1Usage:&n &4colourset&n [-<0..9> { <colourcode> | Default }]";
  int colour_codes[10];
  char mode;

  if (IS_NPC(ch))
    return;

  skip_spaces(&argument);
  strcpy(buf, argument);

  for (i=0;i<10;i++)
    colour_codes[i] = -1;

  while (*buf) {
    half_chop_case_sens(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);       /* just in case; we destroy arg in the switch */
      if (isnum(mode)) {
        colour_num = mode - '0';
        
	half_chop_case_sens(buf1, arg, buf);
        if (!strn_cmp(arg,"default",3)) {
          // The magic default number
          colour_codes[colour_num] = 69; 
          no_args = FALSE;
        } else if ((colour_code = is_colour(NULL,*arg,TRUE)) >= 0) {
          colour_codes[colour_num] = colour_code;
          no_args = FALSE;
        } else {
          send_to_char("Invalid colour code.\r\n",ch);
          send_to_char(usage,ch);
          return;
        }
      } else {			/* endif isnum */
        send_to_char(usage, ch);
        return;
      }
    } else {			/* endif arg == '-' */
      send_to_char(usage, ch);
      return;
    }
  }
          
         
  if (no_args) {

    sprintf(buf,"Personal colour settings:\r\n");
    sprintf(buf2,"&&0. &0%s&n\r\n",colour_headings[0]);
    strcat(buf,buf2);
    sprintf(buf2,"&&1. &1%s&n\r\n",colour_headings[1]);
    strcat(buf,buf2);
    sprintf(buf2,"&&2. &2%s&n\r\n",colour_headings[2]);
    strcat(buf,buf2);
    sprintf(buf2,"&&3. &3%s&n\r\n",colour_headings[3]);
    strcat(buf,buf2);
    sprintf(buf2,"&&4. &4%s&n\r\n",colour_headings[4]);
    strcat(buf,buf2);
    sprintf(buf2,"&&5. &5%s&n\r\n",colour_headings[5]);
    strcat(buf,buf2);
    sprintf(buf2,"&&6. &6%s&n\r\n",colour_headings[6]);
    strcat(buf,buf2);
    sprintf(buf2,"&&7. &7%s&n\r\n",colour_headings[7]);
    strcat(buf,buf2);
    sprintf(buf2,"&&8. &8%s&n\r\n",colour_headings[8]);
    strcat(buf,buf2);
    sprintf(buf2,"&&9. &9%s&n\r\n",colour_headings[9]);
    strcat(buf,buf2);
    send_to_char(buf,ch);
  } else {
    for (i=0;i<10;i++) {
      // The magic default number
      if (colour_codes[i] == 69) {
        set_default_colour(ch,i);
      } else if (colour_codes[i] >= 0) {
        set_colour(ch,i,colour_codes[i]);
      }
    }
    send_to_char("Colour changed.\r\n",ch);
  }
}

