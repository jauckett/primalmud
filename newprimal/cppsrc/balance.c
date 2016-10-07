/**************************************************************************
*   File: balance.c                                     Part of PrimalMUD *
*  Usage: Assistance in balancing the world - in specific mobs and objs   *
*                                                                         *
*  All rights reserved.                                                   * 
*                                                                         *
*  Copyright (C) 2001, by the Implementors of PrimalMUD                   *
*									  *
*  Written by Dangermouse 2001.                                           *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "balance.h"

// Local variables
int *mob_levels;
int *obj_levels;

// Private function prototypes
void balance_mobs(int);
void balance_objs(int);
void output_obj(struct obj_data *, FILE *, long);
void output_mob(struct char_data *, FILE *, long);

const char *mob_bits[NUM_MOB_ATTRIBUTES + 1] = {
  "INVALID_MOB",
  "MAX_HPS",
  "MIN_MPS",
  "MAX_DAMAGE",
  "MIN_DAMAGE",
  "MAX_HITROLL",
  "MIN_HITROLL",
  "MAX_DAMROLL",
  "MIN_DAMROLL",
  "MAX_AC",
  "MIN_AC",
  "MAX_EXP",
  "MIN_EXP",
  "MAX_GOLD",
  "MIN_GOLD",
  "\n"
};

const char *obj_bits[NUM_OBJ_ATTRIBUTES + 1] = {
  "INVALID_OBJ",
  "HIT",
  "MANA",
  "MOVE",
  "STR",
  "INT",
  "WIS",
  "DEX",
  "CON",
  "CHA",
  "HITROLL",
  "DAMROLL",
  "TOHIT",
  "TODAM",
  "WEIGHT",
  "COST",
  "AC",
  "DAMAGE",
  "\n"
};


//////////////////////////////////////////////////////////////////////////////
//                            Public functions                              //
//////////////////////////////////////////////////////////////////////////////

void balance_world(int mob_top, int obj_top)
{
  basic_mud_log("Checking world balancing...");
  balance_mobs(mob_top);
  balance_objs(obj_top);
}


/*
 * Check the given obj against the allowable limits,
 * Returns a long with invalid bits set to 1, valid bits sets to 0
 */
long valid_obj(struct obj_data *obj)
{
  int tohit = 0, todam = 0, dicesize = 0, diceroll = 0;
  long invalidbits = 0;
  int objapps[NUM_APPLYS];

  if (!obj || !obj->affected) {
    SET_BIT(invalidbits, BALANCE_OBJ_INVALID);
    return invalidbits;
  }

  for (int i = 0; i < NUM_APPLYS; i++) {
    objapps[i] = 0;
  }

  for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj->affected[i].modifier) {
      objapps[(int)obj->affected[i].location] += obj->affected[i].modifier;
    }
  }

  // buf - Tohit
  // buf1 - Todam
  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WEAPON:
      diceroll = GET_OBJ_VAL(obj, 1);
      dicesize = GET_OBJ_VAL(obj, 2);
      break;

      // DM - FIREWEAPON and MISSILE are still unimplemented!
    case ITEM_FIREWEAPON:
      tohit = GET_OBJ_VAL(obj, 0);
      diceroll = GET_OBJ_VAL(obj, 1);
      dicesize = GET_OBJ_VAL(obj, 2);
      break;

    case ITEM_MISSILE:
      tohit = GET_OBJ_VAL(obj, 0);
      todam = GET_OBJ_VAL(obj, 1);
      break;

    default:
      sprintf(buf, "0");
      sprintf(buf1, "0");
      break;
  }

  // HIT
  if (objapps[APPLY_HIT] > obj_attr(obj, BALANCE_OBJ_HIT)) {
    SET_BIT(invalidbits, BALANCE_OBJ_HIT);
  }
  // MANA 
  if (objapps[APPLY_MANA] > obj_attr(obj, BALANCE_OBJ_MANA)) {
    SET_BIT(invalidbits, BALANCE_OBJ_MANA);
  }
  // MOVE 
  if (objapps[APPLY_MOVE] > obj_attr(obj, BALANCE_OBJ_MOVE)) {
    SET_BIT(invalidbits, BALANCE_OBJ_MOVE);
  }
  // HITROLL
  if (objapps[APPLY_HITROLL] > obj_attr(obj, BALANCE_OBJ_HITROLL)) {
    SET_BIT(invalidbits, BALANCE_OBJ_HITROLL);
  }
  // DAMROLL
  if (objapps[APPLY_DAMROLL] > obj_attr(obj, BALANCE_OBJ_DAMROLL)) {
    SET_BIT(invalidbits, BALANCE_OBJ_DAMROLL);
  }
  // STR
  if (objapps[APPLY_STR] > obj_attr(obj, BALANCE_OBJ_STR)) {
    SET_BIT(invalidbits, BALANCE_OBJ_STR);
  }
  // INT
  if (objapps[APPLY_INT] > obj_attr(obj, BALANCE_OBJ_INT)) {
    SET_BIT(invalidbits, BALANCE_OBJ_INT);
  }
  // WIS
  if (objapps[APPLY_WIS] > obj_attr(obj, BALANCE_OBJ_WIS)) {
    SET_BIT(invalidbits, BALANCE_OBJ_WIS);
  }
  // DEX
  if (objapps[APPLY_DEX] > obj_attr(obj, BALANCE_OBJ_DEX)) {
    SET_BIT(invalidbits, BALANCE_OBJ_DEX);
  }
  // CON
  if (objapps[APPLY_CON] > obj_attr(obj, BALANCE_OBJ_CON)) {
    SET_BIT(invalidbits, BALANCE_OBJ_CON);
  }
  // CHA
  if (objapps[APPLY_CHA] > obj_attr(obj, BALANCE_OBJ_CHA)) {
    SET_BIT(invalidbits, BALANCE_OBJ_CHA);
  }
  // AC 
  if (objapps[APPLY_AC] > obj_attr(obj, BALANCE_OBJ_AC)) {
    SET_BIT(invalidbits, BALANCE_OBJ_AC);
  }
  // WEIGHT 
  if (GET_OBJ_WEIGHT(obj) > obj_attr(obj, BALANCE_OBJ_WEIGHT)) {
    SET_BIT(invalidbits, BALANCE_OBJ_WEIGHT);
  }
  // COST 
  if (GET_OBJ_COST(obj) > obj_attr(obj, BALANCE_OBJ_COST)) {
    SET_BIT(invalidbits, BALANCE_OBJ_COST);
  }

  // DM: this is probably unimplemented - just leave in here for now...
  // ToHIT
  if (tohit > obj_attr(obj, BALANCE_OBJ_TOHIT)) {
    SET_BIT(invalidbits, BALANCE_OBJ_TOHIT);
  }
  // ToDAM
  if (todam > obj_attr(obj, BALANCE_OBJ_TODAM)) {
    SET_BIT(invalidbits, BALANCE_OBJ_TODAM);
  }
  // DAMAGE
  if (diceroll * dicesize > obj_attr(obj, BALANCE_OBJ_DAMAGE)) {
    SET_BIT(invalidbits, BALANCE_OBJ_DAMAGE);
  }

  return invalidbits;
}


/*
 * Checks the given mob against the allowable limits,
 *
 * Returns a bitvector with invalid bits set to 1.
 */
long valid_mob(struct char_data *mob)
{
  long invalidbits = 0;

  // hit points
  if ((GET_MOVE(mob) + GET_HIT(mob) * GET_MANA(mob)) > 
       	mob_attr(mob, BALANCE_MOB_MAX_HPS)) {
    SET_BIT(invalidbits, BALANCE_MOB_MAX_HPS);
  }
  if ((GET_MOVE(mob) + GET_HIT(mob) * GET_MANA(mob)) < 
       	mob_attr(mob, BALANCE_MOB_MIN_HPS)) {
    SET_BIT(invalidbits, BALANCE_MOB_MIN_HPS);
  }

  // damage
  if ((mob->mob_specials.damnodice * mob->mob_specials.damsizedice) > 
       	mob_attr(mob, BALANCE_MOB_MAX_DAMAGE)) {
    SET_BIT(invalidbits, BALANCE_MOB_MAX_DAMAGE);
  }
  if ((mob->mob_specials.damnodice * mob->mob_specials.damsizedice) < 
       	mob_attr(mob, BALANCE_MOB_MIN_DAMAGE)) {
    SET_BIT(invalidbits, BALANCE_MOB_MIN_DAMAGE);
  }

  // Hitroll
  if (GET_HITROLL(mob) > mob_attr(mob, BALANCE_MOB_MAX_HITROLL)) {
    SET_BIT(invalidbits, BALANCE_MOB_MAX_HITROLL);
  }
  if (GET_HITROLL(mob) < mob_attr(mob, BALANCE_MOB_MIN_HITROLL)) {
    SET_BIT(invalidbits, BALANCE_MOB_MIN_HITROLL);
  }

  // Damroll
  if (GET_DAMROLL(mob) > mob_attr(mob, BALANCE_MOB_MAX_DAMROLL)) {
    SET_BIT(invalidbits, BALANCE_MOB_MAX_DAMROLL);
  }
  if (GET_DAMROLL(mob) < mob_attr(mob, BALANCE_MOB_MIN_DAMROLL)) {
    SET_BIT(invalidbits, BALANCE_MOB_MIN_DAMROLL);
  }
  
  // AC
  if (GET_AC(mob) < mob_attr(mob, BALANCE_MOB_MAX_AC)) {
    SET_BIT(invalidbits, BALANCE_MOB_MAX_AC);
  }
  if (GET_AC(mob) > mob_attr(mob, BALANCE_MOB_MIN_AC)) {
    SET_BIT(invalidbits, BALANCE_MOB_MIN_AC);
  }
  
  // EXP
  if (GET_EXP(mob) > mob_attr(mob, BALANCE_MOB_MAX_EXP)) {
    SET_BIT(invalidbits, BALANCE_MOB_MAX_EXP);
  }
  if (GET_EXP(mob) < mob_attr(mob, BALANCE_MOB_MIN_EXP)) {
    SET_BIT(invalidbits, BALANCE_MOB_MIN_EXP);
  }

  // GOLD
  if (GET_GOLD(mob) > mob_attr(mob, BALANCE_MOB_MAX_GOLD)) {
    SET_BIT(invalidbits, BALANCE_MOB_MAX_GOLD);
  }
  if (GET_GOLD(mob) < mob_attr(mob, BALANCE_MOB_MIN_GOLD)) {
    SET_BIT(invalidbits, BALANCE_MOB_MIN_GOLD);
  }

  return invalidbits;
}


//////////////////////////////////////////////////////////////////////////////
//                            Sorting Functions                             //
//////////////////////////////////////////////////////////////////////////////

/*
 * Returns true if the index 'a' (casted int) entry in the obj_levels array 
 * is greater than the index b entry.
 *
 * (obj_levels[i] > obj_levels[i + 1])
 */
int compare_obj_level(const void *x, const void *y)
{
  int   a = *(const int *)x,
        b = *(const int *)y;

  return (obj_levels[a] > obj_levels[b]);
}

/*
 * Returns true if the index 'a' (casted int) entry in the mob_levels array 
 * is greater than the index b entry.
 *
 * (mob_levels[i] > mob_levels[i + 1])
 */
int compare_mob_level(const void *x, const void *y)
{
  int   a = *(const int *)x,
        b = *(const int *)y;

  return (mob_levels[a] > mob_levels[b]);
}



//////////////////////////////////////////////////////////////////////////////
//                                   OBJS                                   //
//////////////////////////////////////////////////////////////////////////////

/*
 * Process all mobs in mob_table. Check balancing limits and print 
 * abnormalities.
 */
void balance_objs(int obj_top)
{
  struct obj_data *obj;
  FILE *obj_file; 
  int *sorted_objs;

  if (!(obj_file = fopen(FILE_OBJ_BALANCE, "w"))) {
    basic_mud_log("SYSERR: Error creating mobile balance file %s.", FILE_OBJ_BALANCE);
    return;
  }

  CREATE(obj_levels, int, obj_top); 
  CREATE(sorted_objs, int, obj_top);

  // copy the obj levels into an array for sorting and fill
  // the sorted_objs array with the obj rnums
  for (int i = 0; i < obj_top; i++) {
    obj = &obj_proto[i];
    obj_levels[i] = GET_OBJ_LR(obj);
    sorted_objs[i] = i;
  }

  // sort the sorted_objs array ...
  qsort(sorted_objs, obj_top, sizeof(int), compare_obj_level);   

  basic_mud_log("   Outputing object balance data to %s", FILE_OBJ_BALANCE);
  fprintf(obj_file, 
    "LVL NAME (vnum)                               "
    "HIT MAN MOV TOHIT   TODAM(MAX) HR DR STR INT WIS CON DEX CHA   "
    "AC WEIGHT   COST\r\n"
    "---------------------------------------------------------------"
    "--------------------------------------------------------------\r\n");
  
  // now process each obj
  for (int i=0; i < obj_top; i++) {
    obj = &obj_proto[sorted_objs[i]];

    if ((GET_OBJ_VNUM(obj) == NOTHING) || (OBJ_FLAGGED(obj, ITEM_QEQ)))
      continue;

    // Now apply limit tests
    output_obj(obj, obj_file, valid_obj(obj));
    fflush(obj_file);
  }
  fclose(obj_file);
}


/*
 * Returns the allowable limit for the given attribute.
 */
int obj_attr(struct obj_data *obj, long attribute)
{
  int level = GET_OBJ_LEVEL(obj);
  // assume level 0 objs follow level 1 formulas 
  level = MAX(1, level);
  
  switch (attribute) {
    case BALANCE_OBJ_HIT:
    case BALANCE_OBJ_MANA:
    case BALANCE_OBJ_MOVE:
      return (level);

    case BALANCE_OBJ_STR:
    case BALANCE_OBJ_INT:
    case BALANCE_OBJ_WIS:
    case BALANCE_OBJ_DEX:
    case BALANCE_OBJ_CON:
    case BALANCE_OBJ_CHA:
      return (int)(1 + level / 25);

    case BALANCE_OBJ_HITROLL:
    case BALANCE_OBJ_DAMROLL:
    case BALANCE_OBJ_AC:
      return (int)(1 + level / 10);

    case BALANCE_OBJ_TOHIT:
      return (int)(level/10) + (int)(level/20) + 1;

    case BALANCE_OBJ_TODAM:
      return (int)(level/10) + (int)(level/20) + 1;

    case BALANCE_OBJ_DAMAGE:
      return (12 + level - (int)(level / 9));

    case BALANCE_OBJ_WEIGHT:
      // ignore drink containers and fountains ...
      if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON || GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)
        return (99999999);
      return (10 + level);

    case BALANCE_OBJ_COST:
      return (level * 1800);

    default:
      basic_mud_log("SYSERR: default case reached in obj_attr");
      return (-1);
  }
}

 
/*
 * Print the relevant object information to file 'obj_file'
 *
 - objapps is the array of affect modifiers, 
 - invalidbits is a bitvector with the invalid bits set to 1.
 *
 */
void output_obj(struct obj_data *obj, FILE *obj_file, long invalidbits)
{
  int objapps[NUM_APPLYS];

  for (int i = 0; i < NUM_APPLYS; i++)
    objapps[i] = 0;

  for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj->affected[i].modifier) {
      objapps[(int)obj->affected[i].location] += obj->affected[i].modifier;
    }
  }

  if (invalidbits == 0) {
    return;
  }

  // buf - Tohit
  // buf1 - Todam
  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_WEAPON:
    case ITEM_FIREWEAPON:
      sprintf(buf, "%d", GET_OBJ_VAL(obj, 0));
      sprintf(buf1, "%dd%d(%4d)", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), 
                          GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 2));
      break;

    case ITEM_MISSILE:
      sprintf(buf, "%d", GET_OBJ_VAL(obj, 0));
      sprintf(buf1, "%d", GET_OBJ_VAL(obj, 1));

    default:
      sprintf(buf, "0");
      sprintf(buf1, "0");
      break;
  }

  fprintf(obj_file, "%3d %-33s (%5d) %3d %3d %3d %5s %12s %2d %2d %3d %3d "
                    "%3d %3d %3d %3d %4d %6d %6d\r\n",
      GET_OBJ_LR(obj), obj->short_description, GET_OBJ_VNUM(obj),
      objapps[APPLY_HIT], objapps[APPLY_MANA], objapps[APPLY_MOVE],
      buf, buf1, // todo
      objapps[APPLY_HITROLL],objapps[APPLY_DAMROLL], 
      objapps[APPLY_STR], objapps[APPLY_INT], objapps[APPLY_WIS], 
      objapps[APPLY_CON], objapps[APPLY_DEX], objapps[APPLY_CHA],
      objapps[APPLY_AC],
      GET_OBJ_WEIGHT(obj), // WEIGHT
      GET_OBJ_COST(obj) // COST
    );
}


//////////////////////////////////////////////////////////////////////////////
//                                   MOBS                                   //
//////////////////////////////////////////////////////////////////////////////

/*
 * Process all mobs in mob_table. Check balancing limits and print 
 * abnormalities.
 */
void balance_mobs(int mob_top) 
{
  struct char_data *mob;
  FILE *mob_file; 
  int *sorted_mobs;

  if (!(mob_file = fopen(FILE_MOB_BALANCE, "w"))) {
    basic_mud_log("SYSERR: Error creating mobile balance file %s.", FILE_MOB_BALANCE);
    return;
  }

  CREATE(mob_levels, int, mob_top); 
  CREATE(sorted_mobs, int, mob_top);

  // copy the mob levels into an array for sorting and fill
  // the sorted_mobs array with the mob rnums
  for (int i = 0; i < mob_top; i++) {
    mob = &mob_proto[i];
    mob_levels[i] = GET_LEVEL(mob);
    sorted_mobs[i] = i;
  }

  // sort the sorted_mobs array ...
  qsort(sorted_mobs, mob_top, sizeof(int), compare_mob_level);   

  basic_mud_log("   Outputing mobile balance data to %s", FILE_MOB_BALANCE);
  fprintf(mob_file, 
    "LVL NAME (vnum)                                   "
    "HPS  HR  DR   AC  DMG DICE(MAX)      EXP    GOLD\r\n"
    "----------------------------------------------------"
    "----------------------------------------------\r\n");
  
  // now process each obj
  for (int i=0; i < mob_top; i++) {
    mob = &mob_proto[sorted_mobs[i]];

    // Sanity check - just incase ...
    if (GET_MOB_VNUM(mob) == NOBODY)
      continue;

    // Print abnormal mobs to file
    // valid_mob(mob) returns a bitvector - if no bits are set, output_mob will
    // not write details to file ...
    output_mob(mob, mob_file, valid_mob(mob));

    fflush(mob_file);
  }
  fclose(mob_file);
}


/*
 * Returns the allowable limit for the given attribute.
 */
int mob_attr(struct char_data *mob, long attribute)
{

  // Formulas for min and max mob attributes
  int level = MIN(100, MAX(1, GET_LEVEL(mob)));
  int minhps = (int)((level*10)+(level/6*5));
  int maxhps = (int)(minhps + (level*10) + (10 * (101 % level)) + (level*level)); 
  int minhitroll = (int)(level / 3);
  int maxhitroll = 4 + level;
  int mindamroll = minhitroll;
  int maxdamroll = maxhitroll;
  int mindamage = level * 3;
  int maxdamage = level * 10; 
  int minac = (20 - (int)(level/3)); 
  int maxac = (minac - 7); 
  int minexp = int((32*level) + ((level*level*level*level)/(8+(level*3))));
//=INT(32*level + (level*level*level*level)/(8+(level*3)))
  int maxexp = level * 10000; 
  int mingold = 0; 
  int maxgold = 900 * level; 

  switch (attribute) {
    case BALANCE_MOB_MAX_HPS:
      return maxhps;
    case BALANCE_MOB_MIN_HPS:
      return minhps; 

    case BALANCE_MOB_MAX_HITROLL:
      return maxhitroll;
    case BALANCE_MOB_MIN_HITROLL:
      return minhitroll;

    case BALANCE_MOB_MAX_DAMROLL:
      return maxdamroll;
    case BALANCE_MOB_MIN_DAMROLL:
      return mindamroll;

    case BALANCE_MOB_MAX_DAMAGE:
      return maxdamage;
    case BALANCE_MOB_MIN_DAMAGE:
      return mindamage;

    case BALANCE_MOB_MAX_AC:
      return maxac * 10;	// Display in [-200, 200]
    case BALANCE_MOB_MIN_AC:
      return minac * 10;	// Display in [-200, 200]

    case BALANCE_MOB_MAX_EXP:
      return maxexp;
    case BALANCE_MOB_MIN_EXP:
      return minexp;

    case BALANCE_MOB_MAX_GOLD:
      return maxgold;
    case BALANCE_MOB_MIN_GOLD:
      return mingold;

    default:
      basic_mud_log("SYSERR: default case reached for mob_attr");
      return (-1);
  }
}


/*
 * Print the relevant mob details to file 'mob_file'
 */
void output_mob(struct char_data *mob, FILE *mob_file, long invalidbits)
{
  if (invalidbits == 0) {
    return;
  }

  // buf - damage roll
  sprintf(buf, "%dd%d(%5d)", mob->mob_specials.damnodice, 
      mob->mob_specials.damsizedice, 
      mob->mob_specials.damnodice * mob->mob_specials.damsizedice);

  fprintf(mob_file, "%3d %-35s (%5d) %5d %3d %3d %4d %15s %7d %7d\r\n",
      GET_LEVEL(mob), GET_NAME(mob), GET_MOB_VNUM(mob), 
      GET_MOVE(mob) + GET_HIT(mob) * GET_MANA(mob), 
      GET_HITROLL(mob), GET_DAMROLL(mob), GET_AC(mob), buf, GET_EXP(mob), 
      GET_GOLD(mob));
}
