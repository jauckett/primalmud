/***************************************************************************
 *  File: clan.h                                         Part of PrimalMUD *
 *                                                                         *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  Copyright (C) 2001, by the Implementors of PrimalMUD                   *
 *                                                                         *
 *  Written by Artus 2000/2001.                                            *
 ************************************************************************* */

#define MAX_CLANS      8         /* Clan Count Limit - DO NOT CHANGE   */
#define CLAN_ROOM_MAX  8         /* Maximum numer of rooms in hall     */

#define LVL_CLAN_MIN   15	 /* Minimum level to use clan command  */
#define LVL_CLAN_GOD   LVL_GRGOD /* Min level to talk on any clan chan */

#define CLAN_LVL_TALK  1         /* Clan Talking By Rank? (0=Off 1=On) */
#define CLAN_BRIBING   1         /* Clan Bribing                       
				  *  (0=Off, 1=On, 2=Bastard)          */
#define CLAN_LOGLEVEL  0         /* Clan Log Level..                   
				  *  0 = Off,                          
				  *  1 = Immortal Activities Only      
				  *  2 = All Activities.               */
				 /* Unimplemented. TODO - ARTUS        */
#define CLAN_NO_IMM    0         /* Prevent Imms From Being In Clans?  
                                  * This functionality could probably  
				  * be removed, if it didn't come set  
				  * up to prevent imms joining period  
				  * I wouldn't have bothered. - ARTUS  
				  * Oh yeah. All testing is >IMMORT..  */

#define DEFAULT_APP_LVL LVL_CLAN_MIN
				 /* Default Value For Minimum App Lvl. */

// #define CLAN_ZONE        255     /* Zone Number for Clan Zone          */
#define CLAN_DESC_LENGTH 2000    /* Length of clan description         */

#define CP_LEADER    -2        /* Leader (Used in clan_check_priv)     */
#define CP_NONE      -1        /* Nil Priveledge                       */
#define CP_RECRUIT    0        /* Can recruit                          */
#define CP_PROMOTE    1        /* Can promote                          */
#define CP_DEMOTE     2        /* Can demote                           */
#define CP_BANISH     3        /* Can banish                           */
#define CP_WITHDRAW   4        /* Can withdraw clan funds              */
#define CP_SET_APPLEV 5        /* Can set minimum apply level          */
#define CP_SET_FEES   6        /* Can set application fees             */
#define CP_ENHANCE    7        /* Can enhance guard.                   */
#define CP_ROOM       8	       /* Can alter clan room layouts          */
#define CP_SET_DESC   9	       /* Can set description		       */
#define CP_ENABLE    10        /* Can enable features                  */
#define NUM_CP       11        /* Number Of Clan Privledges (Max 20)   */

#define CPC_NOCLAN    1        /* Clan Priv Check -- No Clan           */
#define CPC_NOPRIV    2        /* Clan Priv Check -- No Privs          */
#define CPC_IMM       3        /* Clan Priv Check -- Immortal          */
#define CPC_PRIV      4        /* Clan Priv Check -- Privleged         */

#define CM_DUES   1	       /* Used in do_clan_money, subcommand    */
#define CM_APPFEE 2

#define CB_DEPOSIT  1	       /* Used in do_clan_bank, subcommand     */
#define CB_BALANCE  2          
#define CB_WITHDRAW 3

/* Offence Bits CANNOT EXCEED 32 - ARTUS */
#define OFF_PKILL     0        /* Unbalanced Player Kill               */
#define NUM_OFFENCES  7        /* Number Of Offence Types (<=32)       */

#define CLC_ADDRANK   2000000  /* Add ranks (Increments)               */
#define CLC_BUILD     25000000 /* Build Rooms                          */
#define CLC_CHGAPPLEV 7500000  /* Change Application Level             */
#define CLC_CHGFEE    20000000 /* Change Appfee/Tax Raet               */
#define CLC_EGUARD    10000000 /* Enhance Guard (Increments)           */
#define CLC_EHEAL     15000000 /* Enhance Healer (Increments)          */
#define CLC_ENBOARD   20000000 /* Enable Clan Board                    */
#define CLC_ENHALL    25000000 /* Enable Clan Hall                     */
#define CLC_ENHEAL    25000000 /* Enable Clan Healer                   */
#define CLC_ENTALK    20000000 /* Enable ctalk, cltalk                 */
#define CLC_MOBDESC   20000000 /* Mob Descriptions (Healer/Guard)      */
#define CLC_PROMOTE   1000000  /* Clan Promotion Cost                  */
#define CLC_RANKTITLE 10000000 /* Change Rank Title                    */
#define CLC_REGENROOM 30000000 /* Set Room 2XREGEN                     */
#define CLC_ROOMTD    20000000 /* Room Titles/Descriptions             */
#define CLC_SETDESC   30000000 /* Change Clan Description              */

#define CLAN_MIN_BRIB 100000   /* Minimum Guard Bribe (x100)           */
#define CLAN_MIN_TAX  2        /* Minimum Clan Tax Percentage...       */
#define CLAN_MAX_TAX  35       /* Maximum Clan Tax Percentage...       */
#define CLAN_MIN_APP  200000   /* Minimum Clan Application Fee         */
#define CLAN_MAX_APP  25000000 /* Maximum Clan Application Fee         */

/* Clan Options.. */
#define CO_HALL       0        /* Clan has a hall.                     */
#define CO_HEALER     1        /* Clan has a healer.                   */
#define CO_TALK       2        /* Clan has clantalk                    */
#define CO_BOARD      3        /* Clan has a board.                    */
#define NUM_CO        4

#define COF_FLAGS(c)              clan[c].options
#define CLAN_HASOPT(c, opt)       (IS_SET(COF_FLAGS(c), (1 << opt)))

#define GET_CLAN(ch)              ((ch)->player_specials->saved.clan)
#define GET_CLAN_RANK(ch)         ((ch)->player_specials->saved.clan_rank)
#define GET_CLAN_REL(us, them)    clan[us].relations[clan[them].id-1]
#define GET_CLAN_FRAGS(us, them)  clan[us].frags[clan[them].id-1]
#define GET_CLAN_DEATHS(us, them) clan[us].deaths[clan[them].id-1]
#define CLAN_AT_WAR(us, them)     (GET_CLAN_REL(us, them) < CLAN_REL_WAR)
#define CLAN_AT_PEACE(us, them)	  (GET_CLAN_REL(us, them) >= CLAN_REL_PEACEFUL)
#define CLAN_ALLIED(us, them)	  (GET_CLAN_REL(us, them) >= CLAN_REL_ALLIANCE)

void save_clans(void);
void init_clans(void);
sh_int find_clan_by_id(int clan_id);
sh_int find_clan(char *name);

extern struct clan_rec clan[MAX_CLANS];
extern int num_of_clans;
extern const int clan_levels[];

struct clan_rec 
{
  int id;				/* Clan ID                */
  char name[32];			/* Clan Name              */
  ubyte ranks;                          /* Clan Rank Count        */
  char rank_name[20][20];               /* Clan Rank Names        */
  unsigned long treasure;               /* Clan Treasury          */
  int members;                          /* Clan Member Count      */
  int power;                            /* Clan Power             */
  int app_fee;                          /* Clan Application Fee   */
  ubyte taxrate;                        /* Clan Taxrate           */
  int spells[5];                        /* Clan Spells^           */
  int app_level;                        /* Clan Apply Level       */
  ubyte privilege[20];                  /* Clan Privileges        */
  int relations[MAX_CLANS];             /* Clan Relations         */
  int frags[MAX_CLANS];                 /* Clan Kills             */
  int deaths[MAX_CLANS];                /* Clan Deaths            */
  ubyte level;                          /* Clan Level....?        */
  long exp;                             /* Clan Experience....?   */
  long options; /* bitvector_t */       /* Clan Options           */
  // byte options[32]; / / bitvector this!! / * Clan Options           */
  char description[CLAN_DESC_LENGTH];   /* Clan Description       */
};

/* Spells... Well.. I know it wasn't asked for, I know it probably won't get
 * used at all.. It happened to be there and do nothing in the original code..
 * Perhaps it could be used later on so that a group of say 3 clan members
 * could strum up together and cast a clan-wide enchantment, such as 
 * sanctuary or haste.. Who knows.. Just a thought...                  - ARTUS
 */

/* Yeah, could've put a little more effort into the options and made it a 
 * bitvector probably.. *shrugs* Wouldn't be hard to change..          - ARTUS
 */

#define UNPK_ON_PK_LOSS 0		/* Remove PK Flag If Loss of PK FIGHT */

/* Clan Relationship Defines */
#define CLAN_REL_MIN		-3000	/* Minimum Value For Relationships  */
#define CLAN_REL_BLOOD_WAR 	-2000   /* Blood Feud 		-3000:-2001 */
#define CLAN_REL_WAR		-1500	/* War        		-2000:-1501 */
#define CLAN_REL_ANGER		-1000	/* Angered    		-1500:-1001 */
#define CLAN_REL_UPSET		-500	/* Upset      		-1000:- 501 */
#define CLAN_REL_NEUTRAL	500	/* Neutral    		- 500:+ 499 */
#define CLAN_REL_FRIENDLY	1000	/* Friendly     	+ 500:+ 999 */
#define CLAN_REL_TRADE		1500	/* Trade Relationship	+1000:+1499 */
#define CLAN_REL_PEACEFUL	2000	/* Peaceful		+1500:+1999 */
#define CLAN_REL_ALLIANCE	3000	/* Alliance		+2000:+3000 */
#define CLAN_REL_MAX		3000	/* Maximum Value For Relationships  */
