#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "ole.h"

/*   external vars  */
extern struct index_data *obj_index;

/* for objects */
extern char *item_types[];
extern char *wear_bits[];
extern char *extra_bits[];

/* for chars */
extern char *apply_types[];

extern void do_list_obj_values(struct obj_data *k, char * buf);
void send_to_char(char *messg, struct char_data *ch);

void do_ole_list(struct char_data * ch, struct obj_data * j)
{
 
  int i, virtual, found;
  char buf[1024];
  virtual = GET_OBJ_VNUM(j);
  sprintf(buf,"\r\n");
  sprintf(buf,"%sItem-Values:\r\n",buf);
  sprintf(buf,"%s------------\r\n",buf);
  sprintf(buf,"%sINTERNAL Item-Nr       : %d\r\n",buf,GET_OBJ_RNUM(j)); 
  sprintf(buf,"%s1.)  Item-Nr       : %d\r\n",buf,virtual);
  sprintf(buf,"%s2.)  Item-NameList : %s\r\n",buf,j->name);
  sprintf(buf,"%s3.)  Item-ShortDesc: %s\r\n",buf,j->short_description);
  sprintf(buf,"%s4.)  Item-LongDesc : %s\r\n",buf,j->description);
  sprintf(buf,"%s5.)  Item-ActDesc  ",buf);
  if(j->action_description==NULL)
    sprintf(buf,"%s: <NOT SET>\r\n",buf);
  else
  if(j->obj_flags.type_flag==ITEM_WEAPON)
    sprintf(buf,"%s: %s\r\n",buf,j->action_description);
  else
    sprintf(buf,"%s-> Set   (Not viewed)\r\n",buf);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  sprintf(buf,"%s6.)  Item-Type     : %s\r\n",buf,buf1);
  sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf1);
  sprintf(buf,"%s7.)  Item-Extras   : %s\r\n",buf,buf1); 
  sprintbit(j->obj_flags.wear_flags, wear_bits, buf1);
  sprintf(buf,"%s8.)  Item-Wear     : %s\r\n",buf,buf1);
  sprintf(buf,"%s9.)  Item-Weight   : %d\r\n",buf,GET_OBJ_WEIGHT(j));
  sprintf(buf,"%s10.) Item-Cost     : %d\r\n",buf,GET_OBJ_COST(j));
  sprintf(buf,"%s11.) Item-Cost/Day : %d\r\n",buf,GET_OBJ_RENT(j));
  sprintf(buf,"%s12.) Item-Timer    : %d\r\n",buf,GET_OBJ_TIMER(j));
  do_list_obj_values(j,buf1);
  sprintf(buf,"%s13.) %s\r\n",buf,buf1);
  /*Values[0..3]  : %d %d %d %d\r\n",buf,j->obj_flags.value[0],j->obj_flags.value[1],j->obj_flags.value[2],j->obj_flags.value[3]);*/
  sprintf(buf,"%s14.) Item-Apply    : ",buf);
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s%s %+d to %s",buf, found++ ? "," : "",
	      j->affected[i].modifier, buf2);
    }
  sprintf(buf,"%s\r\n",buf);
  sprintf(buf,"%s15.) Item-Affects  : (NOT USED)\r\n",buf);

  sprintf(buf,"%s16.) Item-ExtraDesc",buf);
  if(j->ex_description==NULL)
    sprintf(buf,"%s: <NOT SET>\r\n",buf);
  else
    sprintf(buf,"%s-> Set   (Not viewed)\r\n",buf);
  sprintf(buf,"%sole>",buf); 
  send_to_char(buf,ch);   
}

void ole_set_string(char* dest, char* src, struct char_data *ch)
{
  skip_spaces(&src);
  delete_doubledollar(src);
  if (strlen(src)==0)
    send_to_char("Enter a string after item number.\r\n", ch);
  else
  {
  /*
    if (*dest)
      free(dest);
    dest =str_dup(src);
  */
    strcpy(dest,src);
  }
}
void ole_set_number(int* dest, char* src, struct char_data *ch)
{
  int valu=-1;
  skip_spaces(&src);
  delete_doubledollar(src);
  if (is_number(src))
    valu=atoi(src);

  if (valu<=0)
    send_to_char("Enter a value after item number.\r\n", ch);
  else
    *dest=valu;
}

#define MAX_ITEMTYPE 23 

void  ole_ListObjType(struct obj_data *wobj,struct char_data *ch)
{
  int wint;

  sprintf(buf,"Old Type:");
  
  if(GET_OBJ_TYPE(wobj)>MAX_ITEMTYPE)
    sprintf(buf,"%s*UNKNOWN*",buf);
  else
    sprintf(buf,"%s%s\n",buf,item_types[(int)GET_OBJ_TYPE(wobj)]);
  for(wint=0;wint<MAX_ITEMTYPE-1;wint=wint+2)
  {
    if(wint<MAX_ITEMTYPE-1)
      sprintf(buf,"%s%.2d.) %-34.34s %.2d.) %-34.34s\n",buf,wint,item_types[wint],wint+1,item_types[wint+1]);
    else
      sprintf(buf,"%s%.2d.) %s\n",buf,wint,item_types[wint]);
  }
  sprintf(buf,"%sPlease enter Number of type (%s):",buf,item_types[ch->desc->ole_setting[0]]);
  send_to_char(buf,ch);
}

#define MAX_ITEMEXTRAS 28
void  ole_ListObjExtras(struct obj_data *wobj,struct char_data *ch)
{
  int wint;

  sprintbit(GET_OBJ_EXTRA(ch->desc->ole_obj), extra_bits, buf1);
  sprintf(buf,"Old Extras:%s\r\n",buf1);
  
  for(wint=0;wint<MAX_ITEMEXTRAS-1;wint=wint+2)
  {
    if(wint<MAX_ITEMEXTRAS-1)
      sprintf(buf,"%s%.2d.) %-34.34s %.2d.) %-34.34s\n",buf,wint,extra_bits[wint],wint+1,extra_bits[wint+1]);
    else
      sprintf(buf,"%s%.2d.) %s\n",buf,wint,extra_bits[wint]);
  }
  sprintbit(ch->desc->ole_setting[0], extra_bits, buf1);
  sprintf(buf,"%sCurrent %s:\r\nPlease enter Number of extra:",buf,buf1);
  send_to_char(buf,ch);
}

#define MAX_ITEMWEAR 15
void  ole_ListObjWear(struct obj_data *wobj,struct char_data *ch)
{
  int wint;

  sprintbit(GET_OBJ_WEAR(ch->desc->ole_obj), wear_bits, buf1);
  sprintf(buf,"Old Wear:%s\r\n",buf1);
  
  for(wint=0;wint<MAX_ITEMWEAR-1;wint=wint+2)
  {
    if(wint<MAX_ITEMWEAR-1)
      sprintf(buf,"%s%.2d.) %-34.34s %.2d.) %-34.34s\n",buf,wint,wear_bits[wint],wint+1,wear_bits[wint+1]);
    else
      sprintf(buf,"%s%.2d.) %s\n",buf,wint,wear_bits[wint]);
  }
  sprintbit(ch->desc->ole_setting[0], wear_bits, buf1);
  sprintf(buf,"%sCurrent %s:\r\nPlease enter Number of wear:",buf,buf1);
  send_to_char(buf,ch);
}

#define MAX_ITEMVALUE 4
void  ole_ListObjValues(struct obj_data *wobj,struct char_data *ch)
{
  int wint;
  struct obj_data tmpobj = *wobj;
  do_list_obj_values(ch->desc->ole_obj,buf1);
  sprintf(buf,"Old Values:%s\r\n",buf1);
  
  for(wint=0;wint<MAX_ITEMVALUE;wint++)
      sprintf(buf,"%s%.2d.) %d \n",buf,wint+1,(int)ch->desc->ole_setting[wint]);

  tmpobj.obj_flags.value[0]=ch->desc->ole_setting[0];
  tmpobj.obj_flags.value[1]=ch->desc->ole_setting[1];
  tmpobj.obj_flags.value[2]=ch->desc->ole_setting[2];
  tmpobj.obj_flags.value[3]=ch->desc->ole_setting[3];
  do_list_obj_values(&tmpobj,buf1);
  sprintf(buf,"%sCurrent %s:\r\nPlease enter Number of extra:",buf,buf1);
  send_to_char(buf,ch);
}

void ole_SetSettingBit(struct char_data *ch,int setting)
{
      if (IS_SET(ch->desc->ole_setting[0],setting))
	REMOVE_BIT(ch->desc->ole_setting[0],setting);
      else
        SET_BIT(ch->desc->ole_setting[0],setting);      
}



#define OLE_OBJEXTRA 3
#define OLE_OBJWEAR  4
#define OLE_OBJVALUE 5


ACMD(do_ole1)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=1;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<1);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<1);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      ole_set_number(ch->desc->ole_setting[0],argument,ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole2)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ole_set_string(ch->desc->ole_obj->name,argument,ch);
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=2;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<2);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<2);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      ole_set_number(&ch->desc->ole_setting[1],argument,ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole3)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ole_set_string(ch->desc->ole_obj->short_description,argument,ch);
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=3;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<3);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<3);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      ole_set_number(&ch->desc->ole_setting[2],argument,ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole4)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ole_set_string(ch->desc->ole_obj->description,argument,ch);
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=4;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<4);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<4);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      ole_set_number(&ch->desc->ole_setting[3],argument,ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole5)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=5;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<5);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<5);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole6)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ch->desc->ole_state=OLE_OBJTYPE;
      ch->desc->ole_setting[0]=GET_OBJ_TYPE(ch->desc->ole_obj);
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=6;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<6);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<6);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole7)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ch->desc->ole_state=OLE_OBJEXTRA;
      ch->desc->ole_setting[0]=GET_OBJ_EXTRA(ch->desc->ole_obj);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=7;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<7);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<7);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole8)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ch->desc->ole_state=OLE_OBJWEAR;
      ch->desc->ole_setting[0]=GET_OBJ_WEAR(ch->desc->ole_obj);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=8;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<8);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<8);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole9)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ole_set_number(&GET_OBJ_WEIGHT(ch->desc->ole_obj),argument,ch);
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=9;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<9);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<9);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole10)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ole_set_number(&GET_OBJ_COST(ch->desc->ole_obj),argument,ch);
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=10;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<10);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<10);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole11)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ole_set_number(&ch->desc->ole_obj->obj_flags.cost_per_day,argument,ch);
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=11;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<11);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<11);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole12)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ole_set_number(&GET_OBJ_TIMER(ch->desc->ole_obj),argument,ch);
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=12;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<12);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<12);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole13)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      ch->desc->ole_state=OLE_OBJVALUE;
      ch->desc->ole_setting[0]=ch->desc->ole_obj->obj_flags.value[0];
      ch->desc->ole_setting[1]=ch->desc->ole_obj->obj_flags.value[1];
      ch->desc->ole_setting[2]=ch->desc->ole_obj->obj_flags.value[2];
      ch->desc->ole_setting[3]=ch->desc->ole_obj->obj_flags.value[3];
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_setting[0]=13;
      ole_ListObjType(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJEXTRA:
      ole_SetSettingBit(ch,1<<13);
      ole_ListObjExtras(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJWEAR:
      ole_SetSettingBit(ch,1<<13);
      ole_ListObjWear(ch->desc->ole_obj,ch);
      break;
    case OLE_OBJVALUE:
      send_to_char("Invalid Selection\r\n",ch);
      ole_ListObjValues(ch->desc->ole_obj,ch);
      break;
  }
}

ACMD(do_ole_quit)
{
  switch (ch->desc->ole_state)
  {
    case OLE_MAIN:
      extract_obj(ch->desc->ole_obj);
      STATE(ch->desc)=CON_PLAYING;
      break;
    case OLE_OBJTYPE:
      ch->desc->ole_state=OLE_MAIN; /* drop back to main */
      GET_OBJ_TYPE(ch->desc->ole_obj)=ch->desc->ole_setting[0];
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJEXTRA:
      ch->desc->ole_state=OLE_MAIN; /* drop back to main */
      GET_OBJ_EXTRA(ch->desc->ole_obj)=ch->desc->ole_setting[0];
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJWEAR:
      ch->desc->ole_state=OLE_MAIN; /* drop back to main */
      GET_OBJ_WEAR(ch->desc->ole_obj)=ch->desc->ole_setting[0];
      do_ole_list(ch,ch->desc->ole_obj);
      break;
    case OLE_OBJVALUE:
      ch->desc->ole_state=OLE_MAIN; /* drop back to main */
      ch->desc->ole_obj->obj_flags.value[0]=ch->desc->ole_setting[0];
      ch->desc->ole_obj->obj_flags.value[1]=ch->desc->ole_setting[1];
      ch->desc->ole_obj->obj_flags.value[2]=ch->desc->ole_setting[2];
      ch->desc->ole_obj->obj_flags.value[3]=ch->desc->ole_setting[3];
      do_ole_list(ch,ch->desc->ole_obj);
      break;
  }
}



