/*
   Modifications done by Brett Murphy to introduce character races
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h> 
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "comm.h"

extern void obj_to_char (struct obj_data *obj,struct char_data *ch);

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */

/* Names first */

const char *class_abbrevs[] = {
  "Og",
  "Dv",
  "Hr",
  "El",
  "Ch",
  "Hu",
  "Or",
  "Kb",
  "Dw",
  "Gn",
  "Kn",
  "Px",
  "\n"
};


const char *pc_class_types[] = {
  "Ogre",
  "Deva",
  "Haruchai",
  "Elf",
  "Changling",
  "Human",
  "Orc",
  "Kobold",
  "Dwarf",
  "Gnome",
  "Kenda",
  "Pixie",
  "\n"
};

const int race_stats[][6] = {
  {3,3,-2,-2,-3,-3},
  {-3,-3,1,3,3,1},
  {1,1,3,-1,-1,0},
  {-1,-2,3,2,1,3},
  {-1,-3,1,3,1,1},
  {0,0,0,0,0,0},
  {3,3,0,-3,-2,-2},
  {3,1,-2,-3,-1,-3},
  {4,3,-2,-2,-2,-3},
  {2,3,1,-2,-3,-2},
  {-1,0,3,0,-1,2},
  {0,-1,3,0,-1,2}
};  



/* The menu for choosing a class in interpreter.c: */
const char *class_stat_menu =
"\r\n"
"NOTE: PrimalMUD does not use a class system.\r\n" 
"      Selecting a class below only determines\r\n"
"      the default NATURAL stat order.\r\n"
"\r\nSelect a class to base your rolled statistics order on:\r\n"
"  [S]elect stats order manually\r\n"
"  [C]leric     (WIS,INT,STR,DEX,CON,CHA)\r\n"
"  [T]hief      (DEX,STR,CON,INT,WIS,CHA)\r\n"
"  [M]agic User (INT,WIS,DEX,STR,CON,CHA)\r\n"
"  [W]arrior    (STR,DEX,CON,WIS,INT,CHA)\r\n";


#define RF 1

/* The menu for choosing a race in interpreter.c: */
/* bm not used, is built up from other data */
char class_menu[15*50]="\r\n";
char class_info[15*50]="\r\n";
void build_class_menu()
{
  int i,j;
  char buff[20],buff2[5]; 
  if (strlen(class_menu) > 10)
    return; 
  strcat(class_menu,"Select a Race ('?' for Race Information).\r\n"); 
  strcat(class_info,"      RACE       STR CON DEX INT WIS CHA\r\n");  
  for (i=0;i<=11;i++)
  {
    sprintf(buff,"[%2d]  %-10s",i+1,pc_class_types[i]); 
    strcat(class_menu,buff);
    strcat(class_menu,"\r\n");
    strcat(class_info,buff);
    for (j=0;j<6;j++)
    { 
      sprintf(buff2,  "%4d",race_stats[i][j]/RF);  
      strcat(class_info,buff2);
    }
    strcat(class_info,"\r\n");
  }
}


/*
 * The code to interpret a class letter (used in interpreter.c when a
 * new character is selecting a class).
 */

int parse_class(char* arg)
{
  int valu;
  valu=0; 
  if (is_number(arg))
    valu=atoi(arg);
  switch (valu) {
  case 1:
    return CLASS_OGRE;
    break;
  case 2:
    return CLASS_DEVA;
    break;
  case 3:
    return CLASS_HARUCHAI;
    break;
  case 4:
    return CLASS_ELF;
    break;
  case 5:
    return CLASS_CHANGLING;
    break;
  case 6:
    return CLASS_HUMAN;
    break;
  case 7:
    return CLASS_ORC;
    break;
  case 8:
    return CLASS_KOBOLD;
    break;
  case 9:
    return CLASS_DWARF;
    break;
  case 10:
    return CLASS_GNOME;
    break;
  case 11:
    return CLASS_KENDA;
    break;
  case 12:
    return CLASS_PIXIE;
    break;
  default:
    return CLASS_UNDEFINED;
    break;
  }
}


long find_class_bitvector(char* arg)
{
  
  int valu;
  valu=0;
  if (is_number(arg))  
    valu=atoi(arg);
  switch (valu) {
    case 1:
      return 1;
      break;
    case 2:
      return 2;
      break;
    case 3:
      return 4;
      break;
    case 4:
      return 8;
      break;
    case 5:
      return 16;
      break;
    case 6:
      return 32;
      break;
    case 7:
      return 64;
      break;
    case 8:
      return 128;
      break;
    case 9:
      return 256;
      break;
    case 10:
      return 512;
      break;
    case 11:
      return 1024;
      break;
    case 12:
      return 2048;
      break;
    default:
      return 0;
      break;
  }
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * charcter is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */


/* ...And the appropriate rooms for each guildmaster/guildguard */
int guild_info[][3] = {

/* Midgaard */
  {-999,	3017,	SCMD_SOUTH},
  {-999,	3004,	SCMD_NORTH},
  {-999,		3027,	SCMD_EAST},
  {-999,	3021,	SCMD_EAST},

/* Brass Dragon */
  {-999 /* all */ ,	5065,	SCMD_WEST},

/* New Sparta */
  {-999,	21075,	SCMD_NORTH},
  {-999,	21019,	SCMD_WEST},
  {-999,		21014,	SCMD_SOUTH},
  {-999,	21023,	SCMD_SOUTH},

/* this must go last -- add new guards above */
{-1, -1, -1}};

int delta_stat(int abil)
{
  return (abil - 10);
}


int thaco(struct char_data * ch)
{
  extern struct str_app_type str_app[];
  sbyte str;
  sbyte dex;
  sbyte wis;
  sbyte intel;
  int thac; 
  str = ch->real_abils.str;
  dex = ch->real_abils.dex;
  wis = ch->real_abils.wis;
  intel = ch->real_abils.intel;
  /* thac= 40-GET_LEVEL(ch)-(0.25*delta_stat(intel)+0.25*delta_stat(wis)+delta_stat(dex)+delta_stat(str)); */
  if (IS_NPC(ch))
      thac =20;
  else
      thac = 25-GET_LEVEL(ch);
  thac -= str_app[STRENGTH_AFF_APPLY_INDEX(ch)].tohit;
  thac -= (int) ((GET_AFF_INT(ch) - 13) / 2);	/* Intelligence helps! */
  thac -= (int) ((GET_AFF_WIS(ch) - 13) / 2);	/* So does wisdom */

/*  if (thac >20) thac=20;
  if (thac <1) thac=1;
 */
  return thac;
}


/*
Both the primary and the scondary statistics are based on 3d6 rolls, except for
HEIGHT which is determined by a single 1d6 roll. The 3d6 rolls approximate a 
standard bell curve, representing the ditribution within a population.
METHOD:
  1. The player chooses two stats and take best 3 of 8d6, re-rolling 1's
  2. The player choooses two remaning stats and take best of 4d6, re-roll 1's
  3. Roll 3d6 for remaining stats, re-roll 1's
  4. Roll 1d6 for height modifier.
  5. Roll 3d6 for weight modifier.
  6. Roll 3d6 for Movement modifier.
The entire character can be re-rolled if: (not implemented yet)
  - the average of the rolls in (1) is less than 16
  - the average of the rolls in (2) is less than 13
  - the average of the rolls in (3) is less than 10
*/
void roll_real_abils(struct char_data * ch)
{
  int i, j, k, temp;
  

  ubyte table[6];
  ubyte rolls[8];
  ubyte stat[6];
  
  const int num_rolls[6] = {8,8,4,4,3,3};
  int class;

  for (i=0;i<6;i++)
  {
    table[i]=0;
    stat[i]=0;
  }

  for (i = 0; i < 6; i++) 
  { 
    for (j = 0; j < 8; j++)
      rolls[j] = 0;
    for (j = 0; j < num_rolls[i]; j++)
      while ((rolls[j] = number(1, 6)) == 1) ;
    for (j = 0; j < 8; j++) 
      for (k=0;k<7;k++)
        if (rolls[k] < rolls[k+1]) 
        {
          temp = rolls[k];
          rolls[k] = rolls[k+1];
          rolls[k+1]= temp;
        }
/* printf("stat %d",i);for (k=0;k<8;k++) printf("[%d] ",rolls[k]);printf("\r\n"); */
   for (j = 0; j < 3; j++)
      table[i] += rolls[j];
 }
  ch->real_abils.str_add = 0;

  for (i=0;i<6;i++)
    for (j=0;j<5;j++)
      if (table[j] < table[j+1])
      {
        temp = table[j];
        table[j]= table[j+1];
        table[j+1]=temp;
      }
  for (i=0;i<6;i++)
    stat[ch->player_specials->saved.stat_order[i]-1] = table[i];
/*
printf("order ");for (i=0;i<6;i++) printf("[%d] ",ch->player_specials->saved.stat_order[i]-1);printf("\r\n");
printf("roll  ");for (i=0;i<6;i++) printf("[%d] ",table[i]);printf("\r\n");
printf("sorted");for (i=0;i<6;i++) printf("[%d] ",stat[i]); printf("\r\n");
*/
  class=GET_CLASS(ch);
/* divide table by a factor, they were too good  */
  ch->real_abils.str   = MIN(stat[0] + race_stats[class][0]/RF,21);
  ch->real_abils.con   = MIN(stat[1] + race_stats[class][1]/RF,21);
  ch->real_abils.dex   = MIN(stat[2] + race_stats[class][2]/RF,21);
  ch->real_abils.intel = MIN(stat[3] + race_stats[class][3]/RF,21);
  ch->real_abils.wis   = MIN(stat[4] + race_stats[class][4]/RF,21);
  ch->real_abils.cha   = MIN(stat[5] + race_stats[class][5]/RF,21);
  
  if (ch->real_abils.str == 18)
    ch->real_abils.str_add = number(0, 100);
    
  ch->aff_abils = ch->real_abils;
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch)
{
  void advance_level(struct char_data * ch);

  extern sh_int world_start_room[3];
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

  set_title(ch, "");
  roll_real_abils(ch);
  ch->points.max_hit = 10;

  SET_SKILL(ch, SKILL_SNEAK, 1);
  SET_SKILL(ch, SKILL_HIDE, 1);
  SET_SKILL(ch, SKILL_STEAL, 1);
  SET_SKILL(ch, SKILL_BACKSTAB, 1);
  SET_SKILL(ch, SKILL_PICK_LOCK, 1);
  SET_SKILL(ch, SKILL_TRACK, 1);
 
  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
  ENTRY_ROOM(ch,1) = world_start_room[0];
  ENTRY_ROOM(ch,2) = world_start_room[1];
  ENTRY_ROOM(ch,3) = world_start_room[2];
}

/* equip the newbies :) D. */
void newbie_equip(struct char_data * ch)
{
  struct obj_data *obj;
  int rnum,vnum,counter = 0;
  int equipm[14] = { 1,1,1,5,12,26,60,70,80,100,110,121,307 };

  do {
    vnum = equipm[counter];
    if(!(rnum = real_object(vnum)))
    {
      return;
      /* add in a log command here...:) */
    }
    obj = read_object(rnum,REAL);
    obj_to_char(obj,ch);
    counter++;
  } while (counter < 14);
  log("Equiped newbie");
}
    

/* Gain maximum in various points */
void advance_level(struct char_data * ch)
{
  int add_hp = 0, add_mana = 0, add_move = 0, i;

  extern struct wis_app_type wis_app[];
  extern struct con_app_type con_app[];

  add_hp = con_app[GET_AFF_CON(ch)].hitp;

  add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
  /* add_mana = MIN(add_mana, 10); */
  add_hp += number(10, 15);    
   add_move = number(1, 3);
    

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

    GET_PRACTICES(ch) += MAX(2, wis_app[GET_AFF_WIS(ch)].bonus);

  if (GET_LEVEL(ch) >= LVL_IS_GOD) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  save_char(ch, NOWHERE);

  sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
  info_channel(buf , ch );  
}

/* DM_demote Loose maximum in various points */
void demote_level(struct char_data * ch,int newlevel)
{
  int i, sub_hp = 0, sub_mana = 0, sub_move = 0, currlevel=GET_LEVEL(ch);

  extern struct wis_app_type wis_app[];
  extern struct con_app_type con_app[];

  /* Subtract for each level */
  for (currlevel=GET_LEVEL(ch);currlevel!=newlevel;currlevel--)
  {
    /* Determine the hps to subtract */
    sub_hp = con_app[GET_AFF_CON(ch)].hitp;
    sub_hp += number(10, 15);    

    /* Determine the mana to subtract */
    sub_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    /* add_mana = MIN(add_mana, 10); */

    /* Determine the move to subtract */
    sub_move = number(1, 3);
    
    ch->points.max_hit -= MAX(1, sub_hp);
    ch->points.max_move -= MAX(1, sub_move);
    ch->points.max_mana -= sub_mana;
    if ((GET_PRACTICES(ch) - MAX(2, wis_app[GET_AFF_WIS(ch)].bonus)) > 0)
      GET_PRACTICES(ch) -= MAX(2, wis_app[GET_AFF_WIS(ch)].bonus);

    GET_LEVEL(ch) = GET_LEVEL(ch) - 1;
  }

  /* Some sanity checks - still can be screwed a bit at least atm */
  if (ch->points.max_hit < 25) 
    ch->points.max_hit = 25;
  
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;

  if (ch->points.max_move < 85)
    ch->points.max_move = 85;

  /* roomflags */
  if (GET_LEVEL(ch) < LVL_ETRNL2) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_ROOMFLAGS);
  }

  /* holylight */ 
  if (GET_LEVEL(ch) < LVL_ETRNL4) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }
  
  /* invis */
  if (GET_LEVEL(ch) < LVL_ANGEL) {
    GET_INVIS_LEV(ch) = 0;
  }

  /* syslog */
  if (GET_LEVEL(ch) < LVL_GOD)
    REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
  
  /* nohassle, hunger, thirst, drunk */
  if (GET_LEVEL(ch) < LVL_CHAMP) {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_NOHASSLE);
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
  }
  save_char(ch, NOWHERE);

  sprintf(buf, "%s demoted to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
  info_channel(buf , ch );  
}

/* Names of class/levels and exp required for each level */
/* bm level dependant */
/* DM_exp - changed for new exp system, each position represents the exp required per level */
const unsigned long int level_exp[LVL_CHAMP + 1] = {
 	 1,        2000,     4000,     8000 	 		/* 0, 1..3 */
	,16000,    32000,    64000,    96000,    128000		/* 4..8 */
	,192000,   256000,   352000,   448000,   576000		/* 9..13 */
	,704000,   896000,   896000,   896000,   1088000	/* 14..18 */
  	,1088000,  1088000,  1088000,  1600000,  1600000	/* 19..23 */
	,1600000,  1952000,  1952000,  1952000,  2304000	/* 24..28 */
	,2304000,  2304000,  2752000,  2752000,  3200000	/* 29..33 */
	,3200000,  3776000,  3776000,  4352000,  4352000	/* 34..38 */
	,5056000,  5056000,  5056000,  5760000,  5760000	/* 39..43 */
	,6656000,  6656000,  7562000,  7562000,  9162000	/* 44..48 */
	,9162000,  11114000, 11114000, 13066000, 13066000	/* 49..53 */
	,15018000, 15018000, 17322000, 17322000, 20074000	/* 54..58 */
	,20074000, 23274000, 23274000, 27050000, 27050000	/* 59..63 */
	,43114000, 43114000, 50676000, 50676000, 50676000	/* 64..68 */
	,59838000, 59838000, 59838000, 70952000, 70952000	/* 69..73 */
	,84018000, 84018000, 99036000, 99036000, 99036000	/* 74..78 */
	,116308000,116308000,116308000,116308000,136432000	/* 79..83 */
	,136432000,136432000,150000000,150000000,150000000	/* 84..88 */
	,175000000,175000000,175000000,200000000,200000000	/* 89..93 */
	,200000000,225000000,225000000,250000000,250000000	/* 94..98 */
	,300000000						/* 99 */
};

int invalid_class(struct char_data *ch, struct obj_data *obj) {
 	return 0;
}

int invalid_level(struct char_data *ch, struct obj_data *object)
{
  int lr=0;
  char buf[80];

/*  if (IS_OBJ_STAT(object, ITEM_LR_5))
    lr=5;
  else if (IS_OBJ_STAT(object, ITEM_LR_10))
    lr=10;
  else if (IS_OBJ_STAT(object, ITEM_LR_15))
    lr=15;
  else if (IS_OBJ_STAT(object, ITEM_LR_20))
    lr=20;
  else if (IS_OBJ_STAT(object, ITEM_LR_25))
    lr=25;
  else if (IS_OBJ_STAT(object, ITEM_LR_30))
    lr=30;
  else if (IS_OBJ_STAT(object, ITEM_LR_IMM))
    lr=31;
*/

  lr = GET_OBJ_LR(object);
  if (lr && GET_LEVEL(ch) <lr)
  {
    sprintf(buf,"You can't figure out how to use this item?!?\r\n");
    act(buf, FALSE,ch,object, 0, TO_CHAR);
    return 1;
  } 
  return 0;
}
