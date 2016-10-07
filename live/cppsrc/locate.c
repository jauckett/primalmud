// Artus> Reimplementation of FIND_XXX.

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "spells.h"

// External Vars.
extern struct char_data *character_list;  /* Mob/Player List		 */
extern struct descriptor_data *descriptor_list;      /* Descriptor List  */
extern struct obj_data *object_list;    /* Object List			 */
extern struct room_data *world;         /* room table			 */
extern struct zone_data *zone_table;	/* zone table			 */

// Artus> Find a char in the same room.
struct char_data *find_char_room(struct char_data *ch, int *itemnum, char *name,
                                 bool show_invis)
{
  bool player = (*itemnum == 0);

  // Artus> Self/Me.
  if ((!str_cmp(name, "me") || !str_cmp(name, "self")) && (--*itemnum < 1))
    return ch;

  for (struct char_data *k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
  {
    if (player && IS_NPC(k))
      continue;
    if (isname(name, k->player.name) && (show_invis || CAN_SEE(ch, k)) 
	&& (--*itemnum < 1))
      return k;
  }
  return NULL;
}

// Artus> Find an online player.
struct char_data *find_player_online(struct char_data *ch, int *itemnum, 
                                     char *name, int where)
{
  bool show_invis = (IS_SET(where, FIND_CHAR_INVIS));
  bool any_world = (!IS_SET(where, FIND_CHAR_INWORLD));

  for (struct descriptor_data *k = descriptor_list; k; k = k->next)
  {
    if (!IS_PLAYING(k))
      continue;
    if (isname(name, k->character->player.name) && 
	(show_invis || CAN_SEE(ch, k->character)) &&
	(any_world || same_world(ch, k->character)) &&
	(--*itemnum < 1))
      return k->character;
  }
  return NULL;
}

// Artus> Find an online player.
struct char_data *get_player_online(struct char_data *ch, char *arg, int where)
{
  int itemnum;
  char tmpname[MAX_INPUT_LENGTH];
  char *name = &tmpname[0];

  one_argument(arg, tmpname);
  if (!*name)
    return NULL;
  itemnum = MAX(1, get_number(&name));

  return find_player_online(ch, &itemnum, name, where);
}

// Artus> Find a character anywhre in the mud.
struct char_data *find_char_world(struct char_data *ch, int *itemnum, 
				  char *name, int where)
{
  struct char_data *k;
  bool any_world = (!IS_SET(where, FIND_CHAR_INWORLD));
  bool show_invis = (IS_SET(where, FIND_CHAR_INVIS));

  // Artus> Self/Me.
  if (!IS_SET(where, FIND_CHAR_ROOM) && 
      (!str_cmp(name, "me") || !str_cmp(name, "self")) && (--*itemnum < 1))
    return ch;

  k = find_player_online(ch, itemnum, name, where);
  if (k)
    return k;
  for (k = character_list; k; k = k->next)
    if (isname(name, k->player.name) && 
	(show_invis || CAN_SEE(ch, k)) && 
	(any_world || same_world(ch, k)) &&
	(--*itemnum < 1))
      return (k);
  return NULL;
}

// Artus> Find an object in a list.
struct obj_data *find_obj_in_list(struct char_data *ch, int *itemnum, 
                                  char *name, struct obj_data *list)
{
  struct obj_data *k;
  for (k = list; k; k = k->next_content)
    if (isname(name, k->name) && CAN_SEE_OBJ(ch, k) && (--*itemnum < 1))
      return k;
  return NULL;
}

// Artus> Find an object in equipment.
struct obj_data *find_obj_equip(struct char_data *ch, int *itemnum, char *name)
{
  for (int i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && 
	CAN_SEE_OBJ(ch, GET_EQ(ch, i)) && (--*itemnum < 1))
      return GET_EQ(ch, i);
  return NULL;
}

// Artus> Find an object in inventory.
struct obj_data *find_obj_inv(struct char_data *ch, int *itemnum, char *name)
{
  return find_obj_in_list(ch, itemnum, name, ch->carrying);
}

// Artus> Find an object in a room.
struct obj_data *find_obj_room(struct char_data *ch, int *itemnum, char *name)
{
  return find_obj_in_list(ch, itemnum, name, world[IN_ROOM(ch)].contents);
}

// Artus> Find an object in the world.
struct obj_data *find_obj_world(struct char_data *ch, int *itemnum, char *name)
{
  for (struct obj_data *k = object_list; k; k = k->next)
    if (isname(name, k->name) && CAN_SEE_OBJ(ch, k) && (--*itemnum < 1))
      return k;
  return NULL;
}

// Artus> Find an object equipped. Get the wear position.
int find_obj_eqpos(struct char_data *ch, char *arg)
{
  int itemnum;
  char tmpname[MAX_INPUT_LENGTH];
  char *name = &tmpname[0];
  
  one_argument(arg, tmpname);
  if (!*name)
    return -1;
  itemnum = MAX(1, get_number(&name));
  
  for (int i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && 
	CAN_SEE_OBJ(ch, GET_EQ(ch, i)) && (--itemnum < 1))
      return i;
  return -1;
}

// Artus> Find an object equipped.
struct obj_data *find_obj_eq(struct char_data *ch, char *arg)
{
  int eqpos;

  eqpos = find_obj_eqpos(ch, arg);

  if (eqpos < 0)
    return NULL;

  return (GET_EQ(ch, eqpos));
}
  
// Artus> Find an object in a list.
struct obj_data *find_obj_list(struct char_data *ch, char *arg,
                               struct obj_data *list)
{
  int itemnum;
  char tmpname[MAX_INPUT_LENGTH];
  char *name = &tmpname[0];

  one_argument(arg, tmpname);
  if (!*name)
    return 0;
  itemnum = MAX(1, get_number(&name));

  return find_obj_in_list(ch, &itemnum, name, list);
}

// Artus> Replacement for generic_find().
int generic_find(char *arg, bitvector_t findbits, struct char_data *ch,
		 struct char_data **tar_ch, struct obj_data **tar_obj)
{
  int itemnum;
  char tmpname[MAX_INPUT_LENGTH];
  char *name = &tmpname[0];

  *tar_obj = NULL;
  *tar_ch = NULL;

  // generic_find()
  one_argument(arg, tmpname);
  if (!*name)
    return 0;
  itemnum = get_number(&name);
  if (itemnum == 0) // No point searching for objects.
    REMOVE_BIT(findbits, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
			  FIND_OBJ_WORLD);
  // In Room.
  if (IS_SET(findbits, FIND_CHAR_ROOM))
    if ((*tar_ch = find_char_room(ch, &itemnum, name, IS_SET(findbits, FIND_CHAR_INVIS))) != NULL)
      return FIND_CHAR_ROOM;
  // Object - Equipped.
  if (IS_SET(findbits, FIND_OBJ_EQUIP))
    if ((*tar_obj = find_obj_equip(ch, &itemnum, name)) != NULL)
      return FIND_OBJ_EQUIP;
  // Object - Inventory.
  if (IS_SET(findbits, FIND_OBJ_INV))
    if ((*tar_obj = find_obj_inv(ch, &itemnum, name)) != NULL)
      return FIND_OBJ_INV;
  // Object - Room.
  if (IS_SET(findbits, FIND_OBJ_ROOM))
    if ((*tar_obj = find_obj_room(ch, &itemnum, name)) != NULL)
      return FIND_OBJ_ROOM;
  // In World / World
  if (IS_SET(findbits, FIND_CHAR_INWORLD))
  {
    if ((*tar_ch = find_char_world(ch, &itemnum, name, findbits)) != NULL)
      return FIND_CHAR_INWORLD;
  } else if (IS_SET(findbits, FIND_CHAR_WORLD)) {
    if ((*tar_ch = find_char_world(ch, &itemnum, name, findbits)) != NULL)
      return FIND_CHAR_WORLD;
  }
  // Object - World.
  if (IS_SET(findbits, FIND_OBJ_WORLD))
    if ((*tar_obj = find_obj_world(ch, &itemnum, name)) != NULL)
      return FIND_OBJ_WORLD;
  // We didn't find shit.
  return 0;
}

// Artus> Find a generic obj.
struct obj_data *generic_find_obj(struct char_data *ch, char *arg, int where)
{
  struct obj_data *tobj;
  struct char_data *tch;

  REMOVE_BIT(where, FIND_CHAR_ROOM | FIND_CHAR_WORLD | FIND_CHAR_INWORLD);
  if (generic_find(arg, where, ch, &tch, &tobj) != 0)
    return tobj;
  return NULL;
}

// Artus> Find a generic char.
struct char_data *generic_find_char(struct char_data *ch, char *arg, int where)
{
  struct obj_data *tobj;
  struct char_data *tch;

  REMOVE_BIT(where, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM |
		    FIND_OBJ_WORLD);
  if (generic_find(arg, where, ch, &tch, &tobj) != 0)
    return tch;
  return NULL;
}
