/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* handling the affected-structures */
void	affect_total(struct char_data *ch);
void	affect_modify(struct char_data *ch, byte loc, sbyte mod, bitvector_t bitv, bool add);
void	affect_to_char(struct char_data *ch, struct affected_type *af);
void	affect_remove(struct char_data *ch, struct affected_type *af, int trig=1);
void	affect_from_char(struct char_data *ch, int type);
void	affect_join(struct char_data *ch, struct affected_type *af,
                bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
bool	affected_by_spell(struct char_data *ch, int type);

/* handling the timers-structures */

struct  timer_type *timer_new(int timertype);

void    timer_to_char(struct char_data *ch, struct timer_type *timer);
void    timer_from_char(struct char_data *ch, int type);
void    timer_remove_char(struct char_data *ch, struct timer_type *timer);
struct  timer_type *char_affected_by_timer(struct char_data *ch, int type);

void    timer_to_obj(struct obj_data *obj, struct timer_type *timer);
void    timer_from_obj(struct obj_data *ch, int type);
void    timer_remove_obj(struct obj_data *ch, struct timer_type *timer);
bool    obj_affected_by_timer(struct obj_data *obj, int type); 

int     timer_use_char(struct char_data *ch, int type);
int     timer_use_obj(struct char_data *ch, int type);

/* utility */
char *money_desc(int amount);
struct obj_data *create_money(int amount);
int	isname(const char *str, const char *namelist);
char	*fname(const char *namelist);
int	get_number(char **name);

/* ******** objects *********** */

void	obj_to_char(struct obj_data *object, struct char_data *ch, char *file, int line);
void	obj_from_char(struct obj_data *object);

bool   equip_char(struct char_data *ch, struct obj_data *obj, int pos, bool manual);
struct obj_data *unequip_char(struct char_data *ch, int pos, bool manual);
int	invalid_align(struct char_data *ch, struct obj_data *obj);

#if 0
struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
#endif
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj_num(obj_rnum nr);

void	obj_to_room(struct obj_data *object, room_rnum room);
void	obj_from_room(struct obj_data *object);
void	obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void	obj_from_obj(struct obj_data *obj);
void	object_list_new_owner(struct obj_data *list, struct char_data *ch);

void	extract_obj(struct obj_data *obj);

/* ******* characters ********* */

struct char_data *get_char_room(char *name, room_rnum room);
struct char_data *get_char_num(mob_rnum nr);
struct char_data *get_char(char *name);

void	char_from_room(struct char_data *ch);
void	char_to_room(struct char_data *ch, room_rnum room);
void	extract_char(struct char_data *ch);

struct clan_data *get_clan_by_name(char *name);
struct clan_data *get_clan_by_num(int num);

void  stop_assisting(struct char_data *ch);
void  stop_assisters(struct char_data *ch);  

#if 0
/* find if character can see */
struct char_data *get_char_room_vis(struct char_data *ch, char *name);
struct char_data *get_player_vis(struct char_data *ch, char *name, int inroom);

struct char_data *get_char_vis(struct char_data *ch, char *name, int where);
struct char_data *get_char_online(struct char_data *ch, char  *name, int where);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, 
struct obj_data *list);
struct obj_data *get_obj_vis(struct char_data *ch, char *name);
struct obj_data *get_object_in_equip_vis(struct char_data *ch, char *arg, 
                                         struct obj_data *equipment[], int *j);
#endif
struct char_data *generic_find_char(struct char_data *ch, char *arg, int where);
struct obj_data *generic_find_obj(struct char_data *ch, char *arg, int where);
struct obj_data *find_obj_eq(struct char_data *ch, char *arg);
struct obj_data *find_obj_list(struct char_data *ch, char *arg,
                               struct obj_data *list);
struct char_data *get_player_online(struct char_data *ch, char *arg, int where);
int    find_obj_eqpos(struct char_data *ch, char *arg);


/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int	generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
		struct char_data **tar_ch, struct obj_data **tar_obj);

#define FIND_CHAR_ROOM     (1 << 0)
#define FIND_CHAR_WORLD    (1 << 1)
#define FIND_OBJ_INV       (1 << 2)
#define FIND_OBJ_ROOM      (1 << 3)
#define FIND_OBJ_WORLD     (1 << 4)
#define FIND_OBJ_EQUIP     (1 << 5)
#define FIND_CHAR_INWORLD  (1 << 6)  // DM - in same primal "world"
#define FIND_CHAR_INVIS    (1 << 7)  // Artus - Show invisible chars?


/* prototypes from crash save system */

int	Crash_get_filename(char *orig_name, char *filename);
int	Crash_delete_file(char *name);
int	Crash_delete_crashfile(struct char_data *ch);
int	Crash_clean_file(char *name);
void	Crash_listrent(struct char_data *ch, char *name);
int	Crash_load(struct char_data *ch);
void	Crash_crashsave(struct char_data *ch);
void	Crash_idlesave(struct char_data *ch);
void	Crash_save_all(void);
void	Locate_save_all(void);

/* prototypes from fight.c */
void	set_fighting(struct char_data *ch, struct char_data *victim, int skillnum);
void	stop_fighting(struct char_data *ch);
void	stop_follower(struct char_data *ch);
void	hit(struct char_data *ch, struct char_data *victim, int type);
void	forget(struct char_data *ch, struct char_data *victim);
void	remember(struct char_data *ch, struct char_data *victim);
int	damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, bool vcheck);
int	skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype);
