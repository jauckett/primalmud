/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "dg_scripts.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"
#include "spells.h"
#include "screen.h"


extern struct descriptor_data *descriptor_list;
extern sh_int find_target_room(char_data * ch, char *rawroomstr);
extern struct index_data *mob_index;
extern struct room_data *world;
extern int dg_owner_purged;
extern const char *dirs[];

void begin_hunting(struct char_data *ch, struct char_data *vict);
void sub_write(char *arg, char_data *ch, byte find_invis, int targets);
long asciiflag_conv(char *flag);
room_data *get_room(char *name);

/*
 * Local functions.
 */

/* attaches mob's name and vnum to msg and sends it to script_log */
void mob_log(char_data *mob, char *msg)
{
    char buf[MAX_INPUT_LENGTH + 100];

    void script_log(char *msg);

    sprintf(buf, "Mob (%s, VNum %d): %s",
           GET_SHORT(mob), GET_MOB_VNUM(mob), msg);
    script_log(buf);
}
/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch) \
  (IS_NPC(ch) && (!(ch)->desc || GET_LEVEL((ch)->desc->original)>=LVL_IMPL))



/* mob commands */

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound)
{
    sh_int was_in_room;
    int  door;
  
    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;
  
    if (!*argument)
    {
       mob_log(ch, "masound called with no argument");
       return;
    }

    skip_spaces(&argument);
  
    was_in_room = IN_ROOM(ch);
    for (door = 0; door < NUM_OF_DIRS; door++)
    {
       struct room_direction_data *exit;
    
       if (((exit = world[was_in_room].dir_option[door]) != NULL) &&
           exit->to_room != NOWHERE && exit->to_room != was_in_room)
       {
           IN_ROOM(ch) = exit->to_room;
           sub_write(argument, ch, TRUE, TO_ROOM);
       }
    }
  
    IN_ROOM(ch) = was_in_room;
}


ACMD(do_mdamage) 
{
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  struct char_data *vict = NULL, *k = NULL;

  if (!(MOB_OR_IMPL(ch)))
  {
    send_to_char(HUH, ch);
    return;
  }

  two_arguments(argument, name, amount);

  if (!*name || !*amount || !isdigit(*amount)) 
  {
    mob_log(ch, "mdamage: bad syntax");
    return;
  }

  dam = atoi(amount);

  vict = get_char(name);
  if (vict)
  {
    // Artus> Sanity.
    for (k = world[IN_ROOM(ch)].people; k; k = k->next)
      if (k == vict) 
	break;
    if (!(k))
    {
      mob_log(ch, "mdamage: victim has disappeared!");
      return;
    }
    if (!LR_FAIL(vict, LVL_IS_GOD))
    {
      send_to_char("Being the cool immortal you are, you sidestep a trap, obviously placed to kill you.\r\n", vict);
      return;
    }
    if (!IS_NPC(vict) || !MOB_FLAGGED(vict, MOB_NOKILL))
    {
      // Artus> Replaced with damage() call.
      // GET_HIT(vict) -= dam;
      damage(ch, vict, dam, TYPE_UNDEFINED, TRUE);
      update_pos(vict);
      switch (GET_POS(vict)) 
      {
        case POS_MORTALLYW:
	  act("$n is mortally wounded, and will die soon, if not aided.", TRUE, vict, 0, 0, TO_ROOM);
	  send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", vict);
	  break;
	case POS_INCAP:
	  act("$n is incapacitated and will slowly die, if not aided.", TRUE, vict, 0, 0, TO_ROOM);
	  send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", vict);
	  break;
	case POS_STUNNED:
	  act("$n is stunned, but will probably regain consciousness again.", TRUE, vict, 0, 0, TO_ROOM);
	  send_to_char("You're stunned, but will probably regain consciousness again.\r\n", vict);
	  break;
	case POS_DEAD:
	  act("$n is dead!  R.I.P.", FALSE, vict, 0, 0, TO_ROOM);
	  send_to_char("You are dead!  Sorry...\r\n", vict);
	  break;
	default:                        /* >= POSITION SLEEPING */
	  if (dam > (GET_MAX_HIT(vict) >> 2))
	    act("That really did HURT!", FALSE, vict, 0, 0, TO_CHAR);
	  if (GET_HIT(vict) < (GET_MAX_HIT(vict) >> 2)) 
	  {
	    sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
		    CCRED(vict, C_SPR), CCNRM(vict, C_SPR));
	    send_to_char(buf2, vict);
	  }
       } // End Switch.
       if (GET_POS(vict) == POS_DEAD) 
       {
	 /* Artus> Logging is handled in die(). 
	 if (!IS_NPC(vict)) 
	 {
	   sprintf(buf2, "%s killed by %s mdamage at %s", GET_NAME(vict), 
		   GET_NAME(ch), world[vict->in_room].name);
	   mudlog(buf2, NRM, LVL_GOD, TRUE);
	 } */
	 die(vict, ch);
       } // Dead Check.
    } // Nokill Check.
  } else
    mob_log(ch, "mdamage: target not found");
}

/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill) 
{
    char arg[MAX_INPUT_LENGTH];
    char_data *victim;
  
    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;
  
    one_argument(argument, arg);
    
    if (!*arg) {
       mob_log(ch, "mkill called with no argument");
       return;
    }

    if (*arg == UID_CHAR)
    {
      if (!(victim = get_char(arg)))
      {
        sprintf(buf, "mkill: victim (%s) not found",arg);
       mob_log(ch, buf);
       return;
      }
    } else if (!(victim = generic_find_char(ch, arg, FIND_CHAR_ROOM))) {
        sprintf(buf, "mkill: victim (%s) not found",arg);
       mob_log(ch, buf);
       return;
    }

    if (victim == ch)
    {
       mob_log(ch, "mkill: victim is self");
       return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
       mob_log(ch, "mkill: charmed mob attacking master");
       return;
    }

    if (FIGHTING(ch))
    {
       mob_log(ch, "mkill: already fighting");
       return;
    }
    
    hit(ch, victim, TYPE_UNDEFINED);
    return;
}


/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy 
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk)
{
    char arg[MAX_INPUT_LENGTH];
    int pos, junk_all = 0;
    obj_data *obj;
    obj_data *obj_next;
  
    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }
  
    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;

    one_argument(argument, arg);
  
    if (!*arg)
    {
       mob_log(ch, "mjunk called with no argument");
       return;
    }

    if (!str_cmp(arg, "all")) junk_all = 1;

    if ((find_all_dots(arg) != FIND_INDIV) && !junk_all)
    {
      if ((pos = find_obj_eqpos(ch,arg)) >= 0)
      {
	obj = GET_EQ(ch, pos);
        unequip_char(ch, pos, FALSE);
	extract_obj(obj);
	return;
      }
      if ((obj = find_obj_list(ch, arg, ch->carrying)) != NULL)
        extract_obj(obj);
      return;
    } else {
      for (obj = ch->carrying; obj != NULL; obj = obj_next)
      {
        obj_next = obj->next_content;
	if (arg[3] == '\0' || isname(arg+4, obj->name))
	  extract_obj(obj);
      }
      while ((pos=find_obj_eqpos(ch,arg)) >= 0)
      {
	obj = GET_EQ(ch, pos);
        unequip_char(ch, pos, FALSE);
	extract_obj(obj);
      }   
    }
    return;
}

/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround)
{
    char arg[MAX_INPUT_LENGTH];
    char_data *victim;
    char *p;

    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }
  
    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;
  
    p = one_argument(argument, arg);
    skip_spaces(&p);
    
    if (!*arg)
    {
       mob_log(ch, "mechoaround called with no argument");
       return;
    }
  
    if (*arg == UID_CHAR)
    {
      if (!(victim = get_char(arg)))
      {
        sprintf(buf, "mechoaround: victim (%s) does not exist",arg);
       mob_log(ch, buf);
       return;
      }
    } else if (!(victim = generic_find_char(ch, arg, FIND_CHAR_ROOM))) {
      sprintf(buf, "mechoaround: victim (%s) does not exist",arg);
      mob_log(ch, buf);
      return;
    }

    sub_write(p, victim, TRUE, TO_ROOM);
}


/* sends the message to only the victim */
ACMD(do_msend)
{
    char arg[MAX_INPUT_LENGTH];
    char_data *victim;
    char *p;

    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;
  
    p = one_argument(argument, arg);
    skip_spaces(&p);
  
    if (!*arg)
    {
       mob_log(ch, "msend called with no argument");
       return;
    }

    if (*arg == UID_CHAR)
    {
      if (!(victim = get_char(arg)))
      {
        sprintf(buf, "msend: victim (%s) does not exist",arg);
        mob_log(ch, buf);
        return;
      }
    } else if (!(victim = generic_find_char(ch, arg, FIND_CHAR_ROOM))) {
      sprintf(buf, "msend: victim (%s) does not exist",arg);
      mob_log(ch, buf);
      return;
    }
  
    sub_write(p, victim, TRUE, TO_CHAR);
}

// Artus> Zone Echo.
ACMD(do_mzoneecho)
{
  char *p;
  void send_to_zone(const char *msg, zone_rnum zone);

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;
  if (IN_ROOM(ch) == NOWHERE)
  {
    mob_log(ch, "mzoneecho called from mobile in NOWHERE.");
    return;
  }
  if (!*argument)
  {
    mob_log(ch, "mzoneecho called with no arguments");
    return;
  }
  p = argument;
  skip_spaces(&p);
  send_to_zone(p, world[IN_ROOM(ch)].zone);
}


/* prints the message to the room at large */
ACMD(do_mecho)
{
    char *p;
  
    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }
  
    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;
  
    if (!*argument) {
       mob_log(ch, "mecho called with no arguments");
       return;
    }
    p = argument;
    skip_spaces(&p);
  
    sub_write(p, ch, TRUE, TO_ROOM);
}


/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE. 
 */
ACMD(do_mload)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0;
    char_data *mob;
    obj_data *object;
  
    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;

    if( ch->desc && GET_LEVEL(ch->desc->original) < LVL_IMPL)
       return;
  
    two_arguments(argument, arg1, arg2);
  
    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
       mob_log(ch, "mload: bad syntax");
       return;
    }

    if (is_abbrev(arg1, "mob")) {
       if ((mob = read_mobile(number, VIRTUAL)) == NULL) {
           mob_log(ch, "mload: bad mob vnum");
           return;
       }
       char_to_room(mob, IN_ROOM(ch));
        load_mtrigger(mob);
    }
  
    else if (is_abbrev(arg1, "obj")) {
       if ((object = read_object(number, VIRTUAL)) == NULL) {
           mob_log(ch, "mload: bad object vnum");
           return;
       }
       if (CAN_WEAR(object, ITEM_WEAR_TAKE)) {
           obj_to_char(object, ch, __FILE__, __LINE__);
       } else {
           obj_to_room(object, IN_ROOM(ch));
       }
        load_otrigger(object);
    }

    else
       mob_log(ch, "mload: bad type");
}


/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  obj_data  *obj;
  
  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;
 
  one_argument(argument, arg);
 
  if (!*arg)
  {
    /* 'purge' */
    char_data *vnext;
    obj_data  *obj_next;
    for (victim = world[IN_ROOM(ch)].people; victim; victim = vnext)
    {
      vnext = victim->next_in_room;
      if (IS_NPC(victim) && victim != ch)
	extract_char(victim);
    }
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj_next)
    {
      obj_next = obj->next_content;
      extract_obj(obj);
    }
    return;
  }

  if (*arg == UID_CHAR)
    victim = get_char(arg);
  else victim = generic_find_char(ch, arg, FIND_CHAR_ROOM);

  if (victim == NULL)
  {
    if ((obj = generic_find_obj(ch, arg, FIND_OBJ_EQUIP | FIND_OBJ_INV |
					 FIND_OBJ_ROOM)))
    {
      if (GET_CORPSEID(obj) == 0)
	extract_obj(obj);
      else
	mob_log(ch, "mpurge: Refusing to purge player's corpse.");
    } else 
      mob_log(ch, "mpurge: bad argument");
    return;
  }
  if (!IS_NPC(victim))
  {
     mob_log(ch, "mpurge: purging a PC");
     return;
  }
  if (victim==ch)
    dg_owner_purged = 1;
  extract_char(victim);
}


/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto)
{
    char arg[MAX_INPUT_LENGTH];
    sh_int location;
  
    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;
  
    one_argument(argument, arg);

    if (!*arg) {
       mob_log(ch, "mgoto called with no argument");
       return;
    }
  
    if ((location = find_target_room(ch, arg)) == NOWHERE) {
       mob_log(ch, "mgoto: invalid location");
       return;
    }
  
    if (FIGHTING(ch))
       stop_fighting(ch);
    
    char_from_room(ch);
    char_to_room(ch, location);
}

/* lets the mob heal themself */
ACMD(do_mrestore)
{
  int amount;

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  skip_spaces(&argument);

  if (!(*argument) || (!isdigit(*argument) && (*argument != '-')))
  {
    sprintf(buf, "mrestore: bad argument (%s)", argument);
    mob_log(ch, buf);
    return;
  }

  if (GET_HIT(ch) >= GET_MAX_HIT(ch))
    return;
  amount = atoi(argument);
  amount = GET_HIT(ch) + amount;
  GET_HIT(ch) = MAX(0, MIN(amount, GET_MAX_HIT(ch)));
}

/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat)
{
    char arg[MAX_INPUT_LENGTH];
    sh_int location;
    sh_int original;
  
    if (!MOB_OR_IMPL(ch))
    {
       send_to_char(HUH, ch);
       return;
    }
  
    if (AFF_FLAGGED(ch, AFF_CHARM))
       return;
  
    argument = one_argument( argument, arg );
  
    if (!*arg || !*argument) {
       mob_log(ch, "mat: bad argument");
       return;
    }
  
    if ((location = find_target_room(ch, arg)) == NOWHERE) {
       mob_log(ch, "mat: invalid location");
       return;
    }
  
    original = IN_ROOM(ch);
    char_from_room(ch);
    char_to_room(ch, location);
    command_interpreter(ch, argument);
    
    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    if (IN_ROOM(ch) == location) {
       char_from_room(ch);
       char_to_room(ch, original);
    }
}


/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  sh_int target;
  char_data *vict, *next_ch;
  
  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  argument = two_arguments(argument, arg1, arg2);
    
  if (!*arg1 || !*arg2)
  {
    mob_log(ch, "mteleport: bad syntax");
    return;
  }

  target = find_target_room(ch, arg2);

  if (target == NOWHERE)
    mob_log(ch, "mteleport target is an invalid room");
  else if (!str_cmp(arg1, "all"))
  {
    if (target == IN_ROOM(ch))
    {
      mob_log(ch, "mteleport all target is itself");
      return;
    }
    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_ch)
    {
      next_ch = vict->next_in_room;
      if (LR_FAIL(vict, LVL_IS_GOD))
      {
	char_from_room(vict);
	char_to_room(vict, target);
      } else if (vict->desc) {
	sprintf(buf, "%s tried to teleport you to room %d.\r\n", 
		GET_NAME(ch), world[target].number);
	send_to_char(buf, vict);
      }
    }
  } else {
    if (*arg1 == UID_CHAR)
    {
      if (!(vict = get_char(arg1)))
      {
	sprintf(buf, "mteleport: victim (%s) does not exist",arg1);
	mob_log(ch, buf);
	return;
      }
    } else if (!(vict = generic_find_char(ch, arg1, FIND_CHAR_WORLD))) {
      sprintf(buf, "mteleport: victim (%s) does not exist",arg1);
      mob_log(ch, buf);
      return;
    }
    // DM - immorts need them for uses like the script to take u to the island
    if (LR_FAIL(ch, LVL_IS_GOD))
    {
      char_from_room(vict);
      char_to_room(vict, target);
    } else if (vict->desc) {
      sprintf(buf, "%s tried to teleport you to room %d.\r\n", GET_NAME(ch),
	      world[target].number);
      send_to_char(buf, vict);
    }
  }
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce)
{
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;
  
  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;
  
  argument = one_argument(argument, arg);
  
  if (!*arg || !*argument)
  {
    mob_log(ch, "mforce: bad syntax");
    return;
  }
  
  if (!str_cmp(arg, "all"))
  {
    struct descriptor_data *i;
    char_data *vch;
    for (i = descriptor_list; i ; i = i->next)
    {
      if ((i->character != ch) && !i->connected &&
	  (IN_ROOM(i->character) == IN_ROOM(ch)))
      {
	vch = i->character;
        if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) &&
	    LR_FAIL(vch, LVL_IS_GOD))
	  command_interpreter(vch, argument);
	else if (!LR_FAIL(vch, LVL_IS_GOD) && (vch->desc))
	{
	  sprintf(buf, "%s tried to force you to %s.\r\n",
		  GET_NAME(ch), argument);
	  send_to_char(buf, ch);
        }
      }
    }
  } else {
    char_data *victim;
    if (*arg == UID_CHAR)
    {
      if (!(victim = get_char(arg)))
      {
	sprintf(buf, "mforce: victim (%s) does not exist",arg);
	mob_log(ch, buf);
	return;
      }
    } else if ((victim = generic_find_char(ch, arg, FIND_CHAR_ROOM)) == NULL) {
      mob_log(ch, "mforce: no such victim");
      return;
    }
    if (victim == ch)
    {
      mob_log(ch, "mforce: forcing self");
      return;
    }
    if (LR_FAIL(ch, LVL_IS_GOD))
      command_interpreter(victim, argument); 
    else if (victim->desc)
    {
      sprintf(buf, "%s tried to force you to %s.\r\n",
	      GET_NAME(ch), argument);
      send_to_char(buf, ch);
    }
  }
}


/* increases the target's exp */
ACMD(do_mexp)
{
  char_data *victim;
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

  mob_log(ch, "WARNING: mexp command is depracated! Use: %actor.exp(amount-to-add)%");

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;  

  two_arguments(argument, name, amount);

  if (!*name || !*amount)
  {
    mob_log(ch, "mexp: too few arguments");
    return;
  }

  if (*name == UID_CHAR)
  {
    if (!(victim = get_char(name)))
    {
      sprintf(buf, "mexp: victim (%s) does not exist",name);
      mob_log(ch, buf);
      return;
    }
  } else if (!(victim = generic_find_char(ch, name, FIND_CHAR_WORLD))) {
    sprintf(buf, "mexp: victim (%s) does not exist",name);
    mob_log(ch, buf);
    return;
  }
  gain_exp(victim, atoi(amount));      
}


/* increases the target's gold */
ACMD(do_mgold)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];

  mob_log(ch, "WARNING: mgold command is depracated! Use: %actor.gold(amount-to-add)%");

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;  

  two_arguments(argument, name, amount);

  if (!*name || !*amount)
  {
    mob_log(ch, "mgold: too few arguments");
    return;
  }
  if (*name == UID_CHAR)
  {
    if (!(victim = get_char(name)))
    {
      sprintf(buf, "mgold: victim (%s) does not exist",name);
      mob_log(ch, buf);
      return;
    }
  } else if (!(victim = generic_find_char(ch, name, FIND_CHAR_WORLD))) {
    sprintf(buf, "mgold: victim (%s) does not exist",name);
    mob_log(ch, buf);
    return;
  }
  if ((GET_GOLD(victim) += atoi(amount)) < 0)
  {
    mob_log(ch, "mgold subtracting more gold than character has");
    GET_GOLD(victim) = 0;
  }
}


/* hunt for someone */
ACMD(do_mhunt)
{
  char_data *victim;
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;
  
  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;
  
  one_argument(argument, arg);

  if (!*arg)
  {
    mob_log(ch, "mhunt called with no argument");
    return;
  }

  if (FIGHTING(ch)) return;

  if (*arg == UID_CHAR)
  {
    if (!(victim = get_char(arg)))
    {
      sprintf(buf, "mhunt: victim (%s) does not exist", arg);
      mob_log(ch, buf);
      return;
    }
  } else if (!(victim = generic_find_char(ch, arg, FIND_CHAR_WORLD))) {
    sprintf(buf, "mhunt: victim (%s) does not exist", arg);
    mob_log(ch, buf);
    return;
  }
  begin_hunting(ch, victim);
}


/* place someone into the mob's memory list */
ACMD(do_mremember)
{
  char_data *victim;
  struct script_memory *mem;
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;

  argument = one_argument(argument, arg);

  if (!*arg)
  {
    mob_log(ch, "mremember: bad syntax");
    return;
  }

  if (*arg == UID_CHAR)
  {
    if (!(victim = get_char(arg)))
    {
      sprintf(buf, "mremember: victim (%s) does not exist", arg);
      mob_log(ch, buf);
      return;
    }
  } else if (!(victim = generic_find_char(ch, arg, FIND_CHAR_ROOM | 
	                                           FIND_CHAR_WORLD))) {
    sprintf(buf, "mremember: victim (%s) does not exist", arg);
    mob_log(ch, buf);
    return;
  }

  /* create a structure and add it to the list */
  CREATE(mem, struct script_memory, 1);
  if (!SCRIPT_MEM(ch)) 
    SCRIPT_MEM(ch) = mem;
  else {
    struct script_memory *tmpmem = SCRIPT_MEM(ch);
    while (tmpmem->next) tmpmem = tmpmem->next;
    tmpmem->next = mem;
  }

  /* fill in the structure */
  mem->id = GET_ID(victim);
  if (argument && *argument)
    mem->cmd = strdup(argument);
}


/* remove someone from the list */
ACMD(do_mforget)
{
  char_data *victim;
  struct script_memory *mem, *prev;
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_LEVEL(ch->desc->original) < LVL_IMPL))
    return;

  one_argument(argument, arg);

  if (!*arg)
  {
    mob_log(ch, "mforget: bad syntax");
    return;
  }

  if (*arg == UID_CHAR)
  {
    if (!(victim = get_char(arg)))
    {
      sprintf(buf, "mforget: victim (%s) does not exist", arg);
      mob_log(ch, buf);
      return;
    }
  } else if (!(victim = generic_find_char(ch, arg, FIND_CHAR_ROOM |
	                                           FIND_CHAR_WORLD))) {
    sprintf(buf, "mforget: victim (%s) does not exist", arg);
    mob_log(ch, buf);
    return;
  }

  mem = SCRIPT_MEM(ch);
  prev = NULL;
  while (mem)
  {
    if (mem->id == GET_ID(victim))
    {
      if (mem->cmd)
	free(mem->cmd);
      if (prev==NULL)
      {
	SCRIPT_MEM(ch) = mem->next;
	free(mem);
	mem = SCRIPT_MEM(ch);
      } else {
	prev->next = mem->next;
	free(mem);
	mem = prev->next;
      }
    } else {
      prev = mem;
      mem = mem->next;
    }
  }
}


/* transform into a different mobile */
ACMD(do_mtransform)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *m, tmpmob;
  obj_data *obj[NUM_WEARS];
  int keep_hp = 1; /* new mob keeps the old mob's hp/max hp/exp */
  int pos;

  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_CHARM))
     return;
  
  if (ch->desc)
  {
    send_to_char("You've got no VNUM to return to, dummy! try 'switch'\r\n",
		 ch);
    return;
  }
  
  one_argument(argument, arg);

  if (!*arg)
    mob_log(ch, "mtransform: missing argument");
  else if (!isdigit(*arg) && *arg!='-')
    mob_log(ch, "mtransform: bad argument");
  else
  {
    if (isdigit(*arg))
      m = read_mobile(atoi(arg), VIRTUAL);
    else
    {
      keep_hp = 0;
      m = read_mobile(atoi(arg+1), VIRTUAL);
    }
    if (m==NULL)
    {
      mob_log(ch, "mtransform: bad mobile vnum");
      return;
    }

    /* move new obj info over to old object and delete new obj */

    for (pos = 0; pos < NUM_WEARS; pos++)
    {
      if (GET_EQ(ch, pos))
        obj[pos] = unequip_char(ch, pos, FALSE);
      else
        obj[pos] = NULL;
    }

    /* put the mob in the same room as ch so extract will work */
    char_to_room(m, IN_ROOM(ch));

    memcpy(&tmpmob, m, sizeof(*m));
    tmpmob.id = ch->id;
    tmpmob.affected = ch->affected;
    tmpmob.carrying = ch->carrying;
    tmpmob.proto_script = ch->proto_script;
    tmpmob.script = ch->script;
    tmpmob.memory = ch->memory;
    tmpmob.next_in_room = ch->next_in_room;
    tmpmob.next = ch->next;
    tmpmob.next_fighting = ch->next_fighting;
    tmpmob.followers = ch->followers;
    tmpmob.master = ch->master;

    GET_WAS_IN(&tmpmob) = GET_WAS_IN(ch);
    if (keep_hp)
    {
      GET_HIT(&tmpmob) = GET_HIT(ch);
      GET_MAX_HIT(&tmpmob) = GET_MAX_HIT(ch);
      GET_EXP(&tmpmob) = GET_EXP(ch);
    }
    GET_GOLD(&tmpmob) = GET_GOLD(ch);
    GET_POS(&tmpmob) = GET_POS(ch);
    IS_CARRYING_W(&tmpmob) = IS_CARRYING_W(ch);
    IS_CARRYING_N(&tmpmob) = IS_CARRYING_N(ch);
    FIGHTING(&tmpmob) = FIGHTING(ch);
    begin_hunting(&tmpmob, HUNTING(ch));
    memcpy(ch, &tmpmob, sizeof(*ch));

    for (pos = 0; pos < NUM_WEARS; pos++) 
      if (obj[pos])
        equip_char(ch, obj[pos], pos, FALSE);

    extract_char(m);
  }
}


ACMD(do_mdoor)
{
  char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
  char field[MAX_INPUT_LENGTH], *value;
  room_data *rm;
  struct room_direction_data *exit;
  int dir, fd, to_room;

  const char *door_field[] = {
    "purge",
    "description",
    "flags",
    "key",
    "name",
    "room",
    "\n"
  };


  if (!MOB_OR_IMPL(ch))
  {
    send_to_char(HUH, ch);
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;
  
  argument = two_arguments(argument, target, direction);
  value = one_argument(argument, field);
  skip_spaces(&value);

  if (!*target || !*direction || !*field)
  {
    mob_log(ch, "mdoor called with too few args");
    return;
  }
  
  if ((rm = get_room(target)) == NULL)
  {
    mob_log(ch, "mdoor: invalid target");
    return;
  }
  
  if ((dir = search_block(direction, dirs, FALSE)) == -1)
  {
    mob_log(ch, "mdoor: invalid direction");
    return;
  }

  if ((fd = search_block(field, door_field, FALSE)) == -1)
  {
    mob_log(ch, "odoor: invalid field");
    return;
  }

  exit = rm->dir_option[dir];

  /* purge exit */
  if (fd == 0)
  {
    if (exit)
    {
      if (exit->general_description)
	free(exit->general_description);
      if (exit->keyword)
	free(exit->keyword);
      free(exit);
      rm->dir_option[dir] = NULL;
    }
  } else {
    if (!exit)
    {
      CREATE(exit, struct room_direction_data, 1);
      rm->dir_option[dir] = exit; 
    }
    
    switch (fd)
    {
      case 1:  /* description */
	if (exit->general_description)
	  free(exit->general_description);
	CREATE(exit->general_description, char, strlen(value) + 3);
	strcpy(exit->general_description, value);
	strcat(exit->general_description, "\r\n");
        break;
      case 2:  /* flags       */
        exit->exit_info = (sh_int)asciiflag_conv(value);
        break;
      case 3:  /* key         */
        exit->key = atoi(value);
        break;
      case 4:  /* name        */
        if (exit->keyword)
	  free(exit->keyword);
	CREATE(exit->keyword, char, strlen(value) + 1);
	strcpy(exit->keyword, value);
	break;
      case 5:  /* room        */
	if ((to_room = real_room(atoi(value))) != NOWHERE)
	  exit->to_room = to_room;
	else
	  mob_log(ch, "mdoor: invalid door target");
	break;
    }
  }
}
