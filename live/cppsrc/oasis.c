/************************************************************************
 * OasisOLC - General / oasis.c					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genwld.h"
#include "genobj.h"
#include "oasis.h"
#include "screen.h"
#include "dg_olc.h"
#include "constants.h"

const char *nrm, *grn, *cyn, *yel;

/*
 * External data structures.
 */
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern zone_rnum top_of_zone_table;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;	/* index table for object file	 */
extern struct obj_data *obj_proto;	/* prototypes for objs		 */
extern struct index_data *mob_index;	/* index table for mobile file	 */
extern obj_rnum top_of_objt;	/* top of object index table	 */
extern mob_rnum top_of_mobt;	/* top of mobile index table	 */

/*
 * Internal data structures.
 */
struct olc_scmd_info_t {
  const char *text;
  int con_type;
} olc_scmd_info[] = {
  { "room",	CON_REDIT },
  { "object",	CON_OEDIT },
  { "zone",	CON_ZEDIT },
  { "mobile",	CON_MEDIT },
  { "shop",	CON_SEDIT },
  { "trigger",  CON_TRIGEDIT},
  { "\n",	-1	  }
};

/* -------------------------------------------------------------------------- */

/*
 * Only player characters should be using OLC anyway.
 */
void clear_screen(struct descriptor_data *d)
{
  if (PRF_FLAGGED(d->character, PRF_CLS))
    send_to_char("[H[J", d->character);
}

/* -------------------------------------------------------------------------- */

/*
 * Exported ACMD do_oasis function.
 *
 * This function is the OLC interface.  It deals with all the 
 * generic OLC stuff, then passes control to the sub-olc sections.
 */
ACMD(do_oasis)
{
  int number = -1, save = 0, real_num;
  struct descriptor_data *d;
  // No screwing around as a mobile.
  if (IS_NPC(ch))
    return;
  
  if ((GET_OLC_ZONE(ch) == 0) && LR_FAIL(ch, LVL_IMPL))
  {
    send_to_char("You do not have a zone assigned to you.\r\n", ch);
    return;
  }

  /*
   * The command to see what needs to be saved, typically 'olc'.
   */
  if (subcmd == SCMD_OLC_SAVEINFO)
  {
    do_show_save_list(ch);
    return;
  }

  /* 
   * Don't allow olc'ing while fighting. -- Artus> 20031014 
   */
  if (FIGHTING(ch))
  {
    send_to_char("No no no.. Stop fighting first.\r\n", ch);
    return;
  }

  /*
   * Parse any arguments.
   */
  two_arguments(argument, buf1, buf2);
  if (!*buf1)
  {		/* No argument given. */
    switch (subcmd)
    {
      case SCMD_OASIS_ZEDIT:
      case SCMD_OASIS_REDIT:
	number = GET_ROOM_VNUM(IN_ROOM(ch));
	break;
      case SCMD_OASIS_TRIGEDIT:
      case SCMD_OASIS_OEDIT:
      case SCMD_OASIS_MEDIT:
      case SCMD_OASIS_SEDIT:
	sprintf(buf, "Specify a %s VNUM to edit.\r\n",
	        olc_scmd_info[subcmd].text);
	send_to_char(buf, ch);
	return;
    }
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) == 0) 
    {
      save = TRUE;
      if ((number = (*buf2 ? atoi(buf2) : 
	            (GET_OLC_ZONE(ch) ? GET_OLC_ZONE(ch) : -1)) * 100) < 0) 
      {
	send_to_char("Save which zone?\r\n", ch);
	return;
      }
    } else if (!str_cmp("new", buf1) && (subcmd == SCMD_OASIS_ZEDIT) && 
	       !LR_FAIL(ch, LVL_IMPL)) {
      if ((*buf2) && (number = atoi(buf2)) >= 0)
	zedit_new_zone(ch, number);
      else
	send_to_char("Specify a new zone number.\r\n", ch);
      return;
    } else if ((str_cmp("sect", buf1) == 0) && (subcmd == SCMD_OASIS_ZEDIT) &&
	       ((GET_OLC_ZONE(ch) == zone_table[world[IN_ROOM(ch)].zone].number)
		|| !LR_FAIL(ch, LVL_IMPL))) {
      if (!(*buf2))
	number = -1;
      else
	number = atoi(buf2);
      zedit_sect_all(ch, number);
      return;
    } else {
      send_to_char("Yikes!  Stop that, someone will get hurt!\r\n", ch);
      return;
    }
  }

  /*
   * If a numeric argument was given (like a room number), get it.
   */
  if (number == -1)
    number = atoi(buf1);

  /*
   * Check that whatever it is isn't already being edited.
   */
  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == olc_scmd_info[subcmd].con_type)
      if (d->olc && OLC_NUM(d) == number)
      {
	sprintf(buf, "That %s is currently being edited by %s.\r\n", olc_scmd_info[subcmd].text, PERS(d->character, ch));
	send_to_char(buf, ch);
	return;
      }
  d = ch->desc;
 
  /*
   * Give descriptor an OLC structure.
   */
  if (d->olc)
  {
    mudlog("SYSERR: do_oasis: Player already had olc structure.", 
	   BRF, LVL_BUILDER, TRUE);
    free(d->olc);
  }
  CREATE(d->olc, struct oasis_olc_data, 1);

  /*
   * Find the zone.
   */

  // DM: DEBUG INFO
  //sprintf(buf, "number = %d, real_zone_by_thing(number) = %d", number,
  //    real_zone_by_thing(number));
  //mudlog(buf, NRM, LVL_GOD, TRUE);
  
  if ((OLC_ZNUM(d) = real_zone_by_thing(number)) == -1)
  {
    send_to_char("Sorry, there is no zone for that number!\r\n", ch);
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /*
   * Everyone but IMPLs can only edit zones they have been assigned.
   * DM: allow full access for chars with olc zone number 0
   * Artus> Coz primr keeps crashing it by pasting lots.. <IMPL now get a 
   *        wait_state.
   */
  if (LR_FAIL(ch, LVL_IMPL))
  {
    if ((GET_OLC_ZONE(ch) != 0) && 
	(zone_table[OLC_ZNUM(d)].number != GET_OLC_ZONE(ch)))
    {
      send_to_char("You do not have permission to edit this zone.\r\n", ch);
      free(d->olc);
      d->olc = NULL;
      return;
    }
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
  }

  if (save)
  {
    const char *type = NULL;
 
    if (subcmd >= 0 && subcmd <= (int)(sizeof(olc_scmd_info) / sizeof(struct olc_scmd_info_t) - 1))
      type = olc_scmd_info[subcmd].text;
    else
    {
      send_to_char("Oops, I forgot what you wanted to save.\r\n", ch);
      return;
    }
    sprintf(buf, "Saving all %ss in zone %d.\r\n", type,
	    zone_table[OLC_ZNUM(d)].number);
    send_to_char(buf, ch);
    sprintf(buf, "OLC: %s saves %s info for zone %d.",
	    GET_NAME(ch), type, zone_table[OLC_ZNUM(d)].number);
    mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE);
    switch (subcmd)
    {
      case SCMD_OASIS_REDIT: save_rooms(OLC_ZNUM(d));   break;
      case SCMD_OASIS_ZEDIT: save_zone(OLC_ZNUM(d));    break;
      case SCMD_OASIS_OEDIT: save_objects(OLC_ZNUM(d)); break;
      case SCMD_OASIS_MEDIT: save_mobiles(OLC_ZNUM(d)); break;
      case SCMD_OASIS_SEDIT: save_shops(OLC_ZNUM(d));   break;
    }
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;
  /*
   * Steal player's descriptor and start up subcommands.
   */
  switch (subcmd)
  {
    case SCMD_OASIS_TRIGEDIT:
      if ((real_num = real_trigger(number)) >= 0)
	trigedit_setup_existing(d, real_num);
      else
	trigedit_setup_new(d);
      STATE(d) = CON_TRIGEDIT;
      break;
    case SCMD_OASIS_REDIT:
      if ((real_num = real_room(number)) >= 0)
	redit_setup_existing(d, real_num);
      else
	redit_setup_new(d);
      STATE(d) = CON_REDIT;
      break;
    case SCMD_OASIS_ZEDIT:
      if ((real_num = real_room(number)) < 0)
      {
	send_to_char("That room does not exist.\r\n", ch);
	free(d->olc);
	d->olc = NULL;
	return;
      }
      zedit_setup(d, real_num);
      STATE(d) = CON_ZEDIT;
      break;
    case SCMD_OASIS_MEDIT:
      if ((real_num = real_mobile(number)) < 0)
	medit_setup_new(d);
      else
	medit_setup_existing(d, real_num);
      STATE(d) = CON_MEDIT;
      break;
    case SCMD_OASIS_OEDIT:
      if ((number == 1351) || (number == 1350))
      { // Artus> Only allow these to be changed offline.
	send_to_char("Cannot edit map/atlas with OLC.\r\n", ch);
	free(d->olc);
	d->olc = NULL;
	return;
      }
      if ((real_num = real_object(number)) >= 0)
	oedit_setup_existing(d, real_num);
      else
	oedit_setup_new(d);
      STATE(d) = CON_OEDIT;
      break;
    case SCMD_OASIS_SEDIT:
      if ((real_num = real_shop(number)) >= 0)
	sedit_setup_existing(d, real_num);
      else
	sedit_setup_new(d);
      STATE(d) = CON_SEDIT;
      break;
  }
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}

/*------------------------------------------------------------*\
 Exported utilities 
\*------------------------------------------------------------*/

/*
 * Set the colour string pointers for that which this char will
 * see at color level NRM.  Changing the entries here will change 
 * the colour scheme throughout the OLC.
 */
void get_char_colors(struct char_data *ch)
{
  nrm = CCNRM(ch, C_NRM);
  grn = CCGRN(ch, C_NRM);
  cyn = CCCYN(ch, C_NRM);
  yel = CCYEL(ch, C_NRM);
}

/*
 * This procedure frees up the strings and/or the structures
 * attatched to a descriptor, sets all flags back to how they
 * should be.
 */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type)
{
  /*
   * Clean up WHAT?
   */
  if (d->olc == NULL)
    return;

  /*
   * Check for a room. free_room doesn't perform
   * sanity checks, we must be careful here.
   */
  if (OLC_ROOM(d))
  {
    switch (cleanup_type)
    {
      case CLEANUP_ALL:
	free_room(OLC_ROOM(d));
	break;
      case CLEANUP_STRUCTS:
	free(OLC_ROOM(d));
	break;
      default: /* The caller has screwed up. */
	basic_mud_log("SYSERR: cleanup_olc: Unknown type!");
	break;
    }
  }

  /*
   * Check for an existing object in the OLC.  The strings
   * aren't part of the prototype any longer.  They get added
   * with str_dup().
   */
  if (OLC_OBJ(d))
  {
    free_object_strings(OLC_OBJ(d));
    free(OLC_OBJ(d));
  }

  /*
   * Check for a mob.  free_mobile() makes sure strings are not in
   * the prototype.
   */
  if (OLC_MOB(d))
    free_mobile(OLC_MOB(d));

  /*
   * Check for a zone.  cleanup_type is irrelevant here, free() everything.
   */
  if (OLC_ZONE(d))
  {
    free(OLC_ZONE(d)->name);
    free(OLC_ZONE(d)->cmd);
    free(OLC_ZONE(d));
  }

  /*
   * Check for a shop.  free_shop doesn't perform sanity checks, we must
   * be careful here.
   */
  if (OLC_SHOP(d))
  {
    switch (cleanup_type)
    {
      case CLEANUP_ALL:
	free_shop(OLC_SHOP(d));
	break;
      case CLEANUP_STRUCTS:
	free(OLC_SHOP(d));
	break;
      default:
	/* The caller has screwed up but we already griped above. */
	break;
    }
  }

  /*
   * Restore descriptor playing status.
   */
  if (d->character)
  {
    REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING);
    STATE(d) = CON_PLAYING;
    act("$n stops using OLC.", TRUE, d->character, NULL, NULL, TO_ROOM);
  }

  free(d->olc);
  d->olc = NULL;
}

int get_type(char *str, int type)
{
  int i;

  switch(type)
  {
    case LIST_OBJ:
      for (i = 0; i < NUM_ITEM_TYPES; i++)
        if (is_abbrev(str, item_types[i]))
          return (i);
      break;
    case LIST_MOB:
      for (i = 0; i < NUM_NPC_CLASSES; i++)
        if (is_abbrev(str, npc_class_types[i]))
          return (i);
  }
  return (NOTHING);
}

ACMD(do_oasislist)
{
  char *ousage = "&1usage: &4olist [from_vnum to_vnum obj_type]&n\r\n"; 
  char *musage = "&1usage: &4mlist [from_vnum to_vnum mob_class]&n\r\n"; 
  char *rusage = "&1usage: &4rlist [from_vnum to_vnum]&n\r\n"; 
  half_chop(argument, buf, buf1);
  int from = 0, to = obj_proto[top_of_objt].item_number; 
  int type = NOTHING, i, nr, found = 0;

  if (!*buf)
  {
    switch (subcmd)
    {
      case LIST_OBJ:
        send_to_char(ousage, ch);
        break;
      case LIST_MOB:
        send_to_char(musage, ch);
        break;
      case LIST_ROOM:
        send_to_char(rusage, ch);
        break;
    }
    return;
  } else {
    // First arg digit - from
    if (isdigit(buf[0]))
    {
      from = atoi(buf);
      to = from;
      half_chop(buf1, buf, buf1);
      // second argument
      if (*buf)
      {
        // second argument digit - to
        if (isdigit(buf[0]))
	{
          to = atoi(buf); 
          half_chop(buf1, buf, buf1);

          // third argument - object type 
          if (*buf)
            type = get_type(buf, subcmd);
        // second argument - object type
        } else {
          type = get_type(buf, subcmd);
        } 
      }
    // first argument - object type
    } else {
      type = get_type(buf, subcmd);
    }
  }

  // Now do some quick checking
  if (to < from)
  {
    send_to_char("to_vnum must be less than from_vnum.\r\n", ch);
    return;
  }

  buf[0] = '\0';
  switch (subcmd)
  {
    case LIST_OBJ:
      for (nr = 0; nr <= top_of_objt; nr++)
        if ((obj_index[nr].vnum >= from && obj_index[nr].vnum <= to) &&
            (type == NOTHING || obj_proto[nr].obj_flags.type_flag == type))
	{ 
	  sprintf(buf2, "%3d. &8[%5d]&n &5%s&n\r\n", ++found,
		  obj_index[nr].vnum, obj_proto[nr].short_description);
	  strncat(buf, buf2, strlen(buf2));
	}
      break;
    case LIST_MOB:
      for (nr = 0; nr <= top_of_mobt; nr++)
        if ((mob_index[nr].vnum >= from && mob_index[nr].vnum <= to) &&
            (type == NOTHING || mob_proto[nr].player.chclass == type))
	{
	  sprintf(buf2, "%3d. &8[%5d]&n &6%s&n\r\n", ++found,
		  mob_index[nr].vnum, mob_proto[nr].player.short_descr);
	  strncat(buf, buf2, strlen(buf2));
	}
      break;
    case LIST_ROOM:
      for (i = 0; i < top_of_world; i++)
        if (world[i].number >= from && world[i].number <= to)
	{
          sprintf(buf2, "%3d. &8[%5d] %s&n\r\n", ++found, world[i].number, 
                          world[i].name);
          strncat(buf, buf2, strlen(buf2));
        }
      break;
    default:
      break;
  }
  page_string(ch->desc, buf, TRUE);
}
