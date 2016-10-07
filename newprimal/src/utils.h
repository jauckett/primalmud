/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* external declarations and prototypes **********************************/

extern struct weather_data weather_info;
extern FILE *logfile;

#define log			basic_mud_log

/* public functions in utils.c */
int     is_wearing(struct char_data *ch, int item_type);
int     is_carrying(struct char_data *ch, int item_type);
char	*str_dup(const char *source);
int	str_cmp(const char *arg1, const char *arg2);
int	strn_cmp(const char *arg1, const char *arg2, int n);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
int	touch(const char *path);
void	mudlog(const char *str, int type, int level, int file);
void    info_channel( char *str, struct char_data *ch);
void	log_death_trap(struct char_data *ch);
int	number(int from, int to);
int	dice(int number, int size);
void	sprintbit(bitvector_t vektor, const char *names[], char *result);
void	sprinttype(int type, const char *names[], char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *orig_name, char *filename, int mode);
struct time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);
void	core_dump_real(const char *, int);
int 	has_stats_for_skill(struct char_data *ch, int skillnum);
int     get_world(struct char_data *ch); 
int     same_world(struct char_data *ch,struct char_data *ch2);  
float 	special_modifier(struct char_data *ch);
void 	set_default_colour(struct char_data *ch, int i); 
void 	set_colour(struct char_data *ch, int i, int colour_code); 


#define core_dump()		core_dump_real(__FILE__, __LINE__)

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);

/* in act.comm.c */
int scan_buffer_for_xword(char* buf); 

/* in magic.c */
bool	circle_follow(struct char_data *ch, struct char_data * victim);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in limits.c */
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch);
void    demote_level(struct char_data *ch,int newlevel);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);

/* class.c */
float	exp_modifiers(int classnum);

/* various constants *****************************************************/

/* Spec MOB used mainly in magic.c, moved for access in other modules */
#define MOB_CLONE               22300

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define ALIAS_FILE	2

/* breadth-first searching */
#define BFS_ERROR		-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		-3

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */
#define SECS_PER_MUD_HOUR	75
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
#define IF_STR(st) ((st) ? (st) : "\0")

#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)	((ch)->char_specials.saved.act)
#define PLR_FLAGS(ch)	((ch)->char_specials.saved.act)
#define PRF_FLAGS(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref))
#define EXT_FLAGS(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.ext_flag)) 
#define SPEC_FLAGS(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.misc_specials)) 
#define GET_SPECIALS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.abilities))
#define CHAR_MEMORISED(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.char_memorised))
#define CHAR_DISGUISED(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.char_disguised))

#define AFF_FLAGS(ch)	((ch)->char_specials.saved.affected_by)
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
#define SPELL_ROUTINES(spl)	(spell_info[spl].routines)
  
/* See http://www.circlemud.org/~greerga/todo.009 to eliminate MOB_ISNPC. */
#define IS_NPC(ch)	(IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)  (IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0)

/* DM - clone utils */
#define IS_CLONE(ch) (IS_NPC(ch) && (GET_MOB_VNUM(ch) == MOB_CLONE))
#define IS_CLONE_ROOM(ch) (IS_CLONE(ch) && \
                ((ch)->in_room == (ch->master)->in_room))

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET(AFF_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET(PRF_FLAGS(ch), (flag)))
#define EXT_FLAGGED(ch, flag) (IS_SET(EXT_FLAGS(ch), (flag)))
#define SPEC_FLAGGED(ch,flag) (IS_SET(SPEC_FLAGS(ch),(flag)))

#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET((obj)->obj_flags.wear_flags, (flag)))
#define OBJ_FLAGGED(obj, flag) (IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define EXT_TOG_CHK(ch,flag) ((TOGGLE_BIT(EXT_FLAGS(ch), (flag))) & (flag))


/* room utils ************************************************************/


#define SECT(room)	(world[(room)].sector_type)

#define IS_DARK(room)  ( !world[room].light && \
                         (ROOM_FLAGGED(room, ROOM_DARK) || \
                          ( ( SECT(room) != SECT_INSIDE && \
                              SECT(room) != SECT_CITY ) && \
                            (weather_info.sunlight == SUN_SET || \
			     weather_info.sunlight == SUN_DARK)) ) )

#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_RNUM(rnum)	((rnum) >= 0 && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) (VALID_RNUM(room) ? world[(room)].func : NULL)

/* char utils ************************************************************/


#define IN_ROOM(ch)	((ch)->in_room)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)

#define GET_PC_NAME(ch)	((ch)->player.name)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->player.short_descr : GET_PC_NAME(ch))
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_PASSWD(ch)	((ch)->player.passwd)
#define GET_PFILEPOS(ch)((ch)->pfilepos)
#define GET_EMAIL(ch)   ((ch)->player.email)
#define GET_WEBPAGE(ch) ((ch)->player.webpage)
#define GET_PERSONAL(ch)((ch)->player.personal)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_MODIFIER(ch)	((ch)->char_specials.modifier)

#define GET_CLASS(ch)   ((ch)->player.chclass)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_SEX(ch)	((ch)->player.sex)

#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_REAL_STR(ch)	((ch)->real_abils.str)
#define GET_REAL_ADD(ch)	((ch)->real_abils.str_add)
#define GET_REAL_DEX(ch)	((ch)->real_abils.dex)
#define GET_REAL_INT(ch)    	((ch)->real_abils.intel)
#define GET_REAL_WIS(ch)     	((ch)->real_abils.wis)
#define GET_REAL_CON(ch)     	((ch)->real_abils.con)
#define GET_REAL_CHA(ch)     	((ch)->real_abils.cha)

#define GET_AFF_STR(ch)     ((ch)->aff_abils.str)
#define GET_AFF_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_AFF_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_AFF_INT(ch)     ((ch)->aff_abils.intel)
#define GET_AFF_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_AFF_CON(ch)     ((ch)->aff_abils.con)
#define GET_AFF_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_EXP(ch)	  ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)	  ((ch)->points.hit)
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
#define GET_MOVE(ch)	  ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)	  ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)	  ((ch)->points.gold)
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)

#define GET_POS(ch)	  ((ch)->char_specials.position)
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
#define HUNTING(ch)	  ((ch)->char_specials.hunting)
#define MOUNTING(ch)      ((ch)->char_specials.mounting)
#define MOUNTING_OBJ(ch)  ((ch)->char_specials.mounting_obj)
#define GET_SAVE(ch, i)	  ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

#define GET_COLOUR(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.colour_settings[(i)]))

/* DM - Autoassist (who this char is autoassisting, max 1)
      - Autoassisted (who is autoassisting this char, list) */
#define AUTOASSIST(ch)    ((ch)->char_specials.autoassist)
#define AUTOASSISTED(ch)  ((ch)->char_specials.autoassisted)  

#define GET_IMMKILLS(ch)	((ch)->player_specials->player_kills.immkills)
#define GET_RACE(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.race))
//#define GET_MODIFIER(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.modifier))
#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define GET_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.spells_to_learn))
#define GET_INVIS_TYPE(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->primalsaved.invis_type_flag))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
/* Extended primal character variables */
#define GET_IGN1(ch) 		((ch)->player_specials->primalsaved.ignore1)
#define GET_IGN2(ch)		((ch)->player_specials->primalsaved.ignore2)
#define GET_IGN3(ch)		((ch)->player_specials->primalsaved.ignore3)
#define GET_IGN_LEVEL(ch)	((ch)->player_specials->primalsaved.ignorelev)
#define GET_IGN_NUM(ch)		((ch)->player_specials->primalsaved.ignorenum)
#define GET_CLAN_NUM(ch)	((ch)->player_specials->primalsaved.clan_num)
#define GET_CLAN_LEV(ch)	((ch)->player_specials->primalsaved.clan_level)
#define ENTRY_ROOM(ch, wrld)    ((ch)->player_specials->primalsaved.world_entry[wrld])
#define GET_PAGE_WIDTH(ch)	((ch)->player_specials->primalsaved.page_width)
#define GET_PAGE_LENGTH(ch)	((ch)->player_specials->primalsaved.page_length)
#define GET_QUEST_ITEM_NUMB(ch,i) ((ch)->player_specials->primalsaved.quest_eq[(i)].max_number)
#define GET_QUEST_ITEM(ch,i)	((ch)->player_specials->primalsaved.quest_eq[(i)].vnum)
#define GET_QUEST_ITEM_OBJ(ch,i) ((ch)->player_specials->primalsaved.quest_eq[(i)].owner)
#define GET_QUEST_ENHANCEMENT(ch, i, j) ((ch)->player_specials->primalsaved.quest_eq[(i)].enh_setting[(j)])
#define GET_QUEST_ENHANCEMENT_VALUE(ch, i, j, k) ((ch)->player_specials->primalsaved.quest_eq[(i)].enhancements[(j)][(k)])
#define GET_SKILL(ch, i)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[i]))
#define SET_SKILL(ch, i, pct)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.skills[i]) = pct; } while(0)

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : -1)

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)

#define STRENGTH_REAL_APPLY_INDEX(ch) \
        ( ((GET_REAL_ADD(ch)==0) || (GET_REAL_STR(ch) != 18)) ? GET_REAL_STR(ch) :\
          (GET_REAL_ADD(ch) <= 50) ? 26 :( \
          (GET_REAL_ADD(ch) <= 75) ? 27 :( \
          (GET_REAL_ADD(ch) <= 90) ? 28 :( \
          (GET_REAL_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) 

#define STRENGTH_AFF_APPLY_INDEX(ch) \
        ( ((GET_AFF_ADD(ch)==0) || (GET_AFF_STR(ch) != 18)) ? GET_AFF_STR(ch) :\
          (GET_AFF_ADD(ch) <= 50) ? 26 :( \
          (GET_AFF_ADD(ch) <= 75) ? 27 :( \
          (GET_AFF_ADD(ch) <= 90) ? 28 :( \
          (GET_AFF_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )

#define CAN_CARRY_W(ch) (str_app[STRENGTH_AFF_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (5 + (GET_AFF_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)


/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)	((d)->connected)

#define IS_PLAYING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT ||      \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||       \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT ||       \
                        STATE(d) == CON_PLAYING)

/* object utils **********************************************************/
#define GET_OBJ_DAMAGE(obj)	((obj)->damage)
#define GET_OBJ_MAX_DAMAGE(obj) ((obj)->max_damage)
#define OBJ_RIDDEN(obj) 	((obj)->ridden_by) 
#define GET_OBJ_LEVEL(obj)	((obj)->obj_flags.level)
#define GET_OBJ_PERM(obj)	((obj)->obj_flags.bitvector) 
#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags)
#define GET_OBJ_LR(obj)		((obj)->obj_flags.level_flags)
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(GET_OBJ_RNUM(obj) >= 0 ? \
				 obj_index[GET_OBJ_RNUM(obj)].vnum : -1)
#define IS_OBJ_STAT(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags,stat))
#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), 3) == 1)

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
	(obj_index[(obj)->item_number].func) : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))


/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || AFF_FLAGGED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 ((!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS)) && \
 (!AFF_FLAGGED((obj), AFF_HIDE) || AFF_FLAGGED(sub, AFF_SENSE_LIFE)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  ( (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))\
   && (!IS_OBJ_STAT((obj), ITEM_HIDDEN)) )

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_NAME(ch) : "someone")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)         (world[(ch)->in_room].dir_option[door])
#define W_EXIT(room, num)      (world[(room)].dir_option[(num)])
#define R_EXIT(room, num)      ((room)->dir_option[(num)])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))


#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])
#define CLASS_NAME(i)  (pc_class_types[(int)(i)]) 

#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGIC_USER) || \
				(GET_CLASS(ch) == CLASS_SPELLSWORD) || \
				(GET_CLASS(ch) == CLASS_DRUID)      || \
				(GET_CLASS(ch) == CLASS_BATTLEMAGE) || \
				(GET_CLASS(ch) == CLASS_MASTER) )

#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_CLERIC)     || \
				(GET_CLASS(ch) == CLASS_PRIEST)     || \
				(GET_CLASS(ch) == CLASS_DRUID)      || \
				(GET_CLASS(ch) == CLASS_PALADIN)    || \
				(GET_CLASS(ch) == CLASS_MASTER) )

#define IS_THIEF(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_THIEF)	    || \
				(GET_CLASS(ch) == CLASS_PRIEST)     || \
				(GET_CLASS(ch) == CLASS_NIGHTBLADE) || \
				(GET_CLASS(ch) == CLASS_SPELLSWORD) || \
				(GET_CLASS(ch) == CLASS_MASTER) )

#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARRIOR)    || \
				(GET_CLASS(ch) == CLASS_BATTLEMAGE) || \
				(GET_CLASS(ch) == CLASS_NIGHTBLADE) || \
				(GET_CLASS(ch) == CLASS_PALADIN)    || \
				(GET_CLASS(ch) == CLASS_MASTER) )

#define IS_NIGHTBLADE(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_NIGHTBLADE))
#define IS_BATTLEMAGE(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_BATTLEMAGE))
#define IS_DRUID(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_DRUID))
#define IS_PRIEST(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_PRIEST))
#define IS_PALADIN(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_PALADIN))
#define IS_SPELLSWORD(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_SPELLSWORD))

#define IS_MASTER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MASTER))


#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))


/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

/* Gun defines */
#define BASE_GUN_TYPE           20
#define MAX_GUN_TYPES           30
 
#define OBJ_IS_GUN(x)                   ((GET_OBJ_VAL((x),3) & 0x7fff) >= BASE_GUN_TYPE \
                                                                                                          && (GET_OBJ_VAL((x),3) & 0x7fff) <= \
                                                                                                         (BASE_GUN_TYPE + MAX_GUN_TYPES))
