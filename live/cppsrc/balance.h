/**************************************************************************
*   File: balance.cpp                                   Part of PrimalMUD *
*  Usage: Assistance in balancing the world - in specific mobs and objs   *
*                                                                         *
*  All rights reserved.                                                   * 
*                                                                         *
*  Copyright (C) 2001, by the Implementors of PrimalMUD                   *
*									  *
*  Written by Dangermouse 2001.						  *
************************************************************************ */


// File string names
#define FILE_MOB_BALANCE "balances/mobs"
#define FILE_OBJ_BALANCE "balances/objs"


// Obj bitvectors
#define BALANCE_OBJ_INVALID	(1 << 0)
#define BALANCE_OBJ_HIT 	(1 << 1)
#define BALANCE_OBJ_MANA 	(1 << 2)
#define BALANCE_OBJ_MOVE	(1 << 3)
#define BALANCE_OBJ_STR 	(1 << 4)
#define BALANCE_OBJ_INT 	(1 << 5)
#define BALANCE_OBJ_WIS 	(1 << 6)
#define BALANCE_OBJ_DEX 	(1 << 7)
#define BALANCE_OBJ_CON 	(1 << 8)
#define BALANCE_OBJ_CHA 	(1 << 9)
#define BALANCE_OBJ_HITROLL	(1 << 10)
#define BALANCE_OBJ_DAMROLL	(1 << 11)
#define BALANCE_OBJ_TOHIT	(1 << 12)
#define BALANCE_OBJ_TODAM	(1 << 13)
#define BALANCE_OBJ_WEIGHT	(1 << 14)
#define BALANCE_OBJ_COST	(1 << 15)
#define BALANCE_OBJ_AC		(1 << 16)
#define BALANCE_OBJ_DAMAGE	(1 << 17)

#define NUM_OBJ_ATTRIBUTES 18


// Mob bitvectors
#define BALANCE_MOB_INVALID	(1 << 0)
#define BALANCE_MOB_MAX_HPS	(1 << 1)
#define BALANCE_MOB_MIN_HPS	(1 << 2)
#define BALANCE_MOB_MAX_DAMAGE	(1 << 3)
#define BALANCE_MOB_MIN_DAMAGE	(1 << 4)
#define BALANCE_MOB_MAX_HITROLL	(1 << 5)
#define BALANCE_MOB_MIN_HITROLL	(1 << 6)
#define BALANCE_MOB_MAX_DAMROLL	(1 << 7)
#define BALANCE_MOB_MIN_DAMROLL	(1 << 8)
#define BALANCE_MOB_MAX_AC	(1 << 9)
#define BALANCE_MOB_MIN_AC	(1 << 10)
#define BALANCE_MOB_MAX_EXP	(1 << 11)
#define BALANCE_MOB_MIN_EXP	(1 << 12)
#define BALANCE_MOB_MAX_GOLD	(1 << 13)
#define BALANCE_MOB_MIN_GOLD	(1 << 14)

#define NUM_MOB_ATTRIBUTES 15


// External variables
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;

extern const char *mob_bits[];
extern const char *obj_bits[];

// Public function prototypes
void balance_world(int mob_top, int obj_top);
long valid_obj(struct obj_data *);
long valid_mob(struct char_data *);

int mob_attr(struct char_data *, long);
int obj_attr(struct obj_data *, long);
