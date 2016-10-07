/* ===================================================================== 

	












  ====================================================================== */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"

#include "edit.h"         
#include "llist.h"


extern struct room_data *world;
extern int top_of_world;
static struct llist_node *rooms_in_use = 0;
static struct llist_node *my_world = 0;        


/* ========================= Local functions  ========================== */
void display_room(struct room_data *room, struct char_data *coder);
void release_room(long vnum);
void edit_help(struct descriptor_data *d);
void test();




/* ========================= Global vars  ========================== */
static char *directions[] = {"North", "East", "South", "West", "Up", "Down"};
static int my_room_bits[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};

#define NUM_RFLAGS 11

static char *my_room_flag_bits[] = 
   {" DARK ", " DEATH ", " NO_MOB ", " INDOORS ", " LAWFULL ", " NEUTRAL ",
    " CHAOTIC ", " NO_MAGIC ", " TUNNEL ", " PRIVATE ", " GODROOM "};









/*====================== Prompting functions ==============*/
      

/* ========================= Displaying functions ========================== */
void show_room_flags(int rflag, struct char_data *coder)
{
   int i;

   sprintf(buf, " ");
   for (i=0;i < NUM_RFLAGS; i++)
      if (my_room_bits[i]&rflag) 
         strcat(buf, my_room_flag_bits[i]);

   strcat(buf, "\n\n");
   send_to_char(buf, coder);
}

void show_exits(struct room_data *room, struct char_data *coder)
{
   int i;
   char tmp[200];

   sprintf(buf,"\n");
   for (i=0; i < NUM_OF_DIRS; i++)
   {
      if (room->dir_option[i] != NULL)
      {
         if (room->dir_option[i]->to_room != NOWHERE)
         {
            sprintf(tmp, "    %s:    \tvnum:%5d\treal: %d\n", 
                  directions[i], (world[ room->dir_option[i]->to_room ]).number,
                  room->dir_option[i]->to_room);
            strcat(buf, tmp);

            if (room->dir_option[i]->keyword != NULL)
            {
               sprintf(tmp, "\t[%s] ", room->dir_option[i]->keyword);
               strcat(buf, tmp);
            }
            if (room->dir_option[i]->general_description != NULL)
            {
               sprintf(tmp, "\n       %s\n", room->dir_option[i]->general_description);
               strcat(buf, tmp);
            }
            else
               strcat(buf, "\n");
         }
      }
   }
   strcat(buf, "\n");
   send_to_char(buf, coder);
}

void show_extra_keywords(struct room_data *room, struct char_data *coder)
{
   struct extra_descr_data *ptr;
   char tmp[100];

   sprintf(buf, "\n");

   if (room->ex_description == NULL)
      return;
   else
   {
      for (ptr = room->ex_description; ptr; ptr = ptr->next)
      {
         sprintf(tmp, " [%s] ", ptr->keyword);
         strcat(buf, tmp);
      }
      send_to_char(buf, coder);
   }
   send_to_char("\n\n", coder);
}

/* ========================= Room functions ========================== */



/* Create a default room. */
struct room_data *create_room(long vnum)
{
   struct room_data *room;
   int i;

   CREATE(room, struct room_data, 1);
   room->number = room->zone = room->sector_type = room->room_flags = 0;
   room->number = vnum;
   room->name = DEFAULT_ROOM_NAME;
   room->description = DEFAULT_ROOM_DESCRIPTION;
   for (i=0; i < NUM_OF_DIRS; i++)
   {
      CREATE(room->dir_option[i], struct room_direction_data, 1);
      room->dir_option[i]->to_room = NOWHERE;
   }
   my_world = add_room_to_llist(my_world, room);
   return(room);
}

/* Get the virtual number of the room. */
void get_vnum(struct room_data *room, struct char_data *coder)
{
   long vnum;
   struct descriptor_data *d = coder->desc;

   if ((STATE(d) == CON_REDITM)&&(d->edit_mode == 17))     /* Check input for validity. */
   {
      send_to_char("testing vnum\n", coder);

      if ((vnum = strtol(coder->desc->edit_str, NULL, 10)) > 0)
         room->number = vnum;
      else
         send_to_char("Invalid Vnum..\n", coder);

      RESET_EDITM();
      return;
   }
   
   cls();
   send_to_char(VNUM_MENU, coder);

   SET_REDITM(1);

   GET_INPUT_STR(24);
}

/* Get the new zone number for the room. */
void zone_number(struct room_data *room, struct char_data *coder)
{
   long znum;
   struct descriptor_data *d = coder->desc;

   if (STATE(d) == CON_REDITM)     /* Check input for validity. */
   {
      send_to_char("testing znum\n", coder);

      if ((znum = strtol(coder->desc->edit_str, NULL, 10)) > 0)
         room->zone = znum;
      else
         send_to_char("Invalid Zone number..\n", coder);

      RESET_EDITM();
      return;
   }
   
   cls();
   send_to_char(ZONE_MENU, coder);
   SET_REDITM(2);
   GET_INPUT_STR(24);
}


void sector_type(struct room_data *room, struct char_data *coder, char sec)
{
   struct descriptor_data *d = coder->desc;

   if (d->edit_mode == 3)     /* Check input for validity. */
   {
      if ((sec >= '0')&&(sec <= '7'))
         room->sector_type = sec - '0';
      else
         send_to_char("Invalid Sector Type ..\n", coder);

      RESET_EDITM();
      return;
   }

   cls();
   send_to_char(SECTOR_TYPE_MENU, coder);
   d->edit_mode = 3;;

}

void sector_type2(struct room_data *room, struct char_data *coder)
{
   long sec;
   struct descriptor_data *d = coder->desc;

   if (STATE(d) == CON_REDITM)     /* Check input for validity. */
   {
      send_to_char("testing sec\n", coder);
      sec = strtol(coder->desc->edit_str, NULL, 10);

      if ((sec >= 0)&&(sec <= 7))
         room->sector_type = sec;
      else
         send_to_char("Invalid Sector Type ..\n", coder);

      RESET_EDITM();
      return; 
   }

   cls();
   send_to_char(SECTOR_TYPE_MENU, coder);
   SET_REDITM(3);
   GET_INPUT_STR(24);

}

void room_flags(struct room_data *room, struct char_data *coder, char ch1)
{
   struct descriptor_data *d = coder->desc;

   if ((ch1)&&(d->edit_mode == 4))     /* Check input for validity. */
   {
      switch(ch1){
      case '0' :
      case '1' :
      case '2' :
      case '3' :
      case '4' :
      case '5' :
      case '6' :
      case '7' :
      case '8' :
        room->room_flags = room->room_flags XOR my_room_bits[ch1 - '0']; 
        break;
      case 'P' :
      case 'p' :
        room->room_flags = room->room_flags XOR my_room_bits[9];
        break;
      case 'G' :
      case 'g' :
        room->room_flags = room->room_flags XOR my_room_bits[10];
        break;
      case 'Q' :
      case 'q' :
         RESET_EDITM();
         return;
         break;
      case 'H' :
      case 'h' :
         edit_help(d);
         break;
      default :
         break;
      }
   }

   sprintf(buf, "======================== <Room Flags> ======================== \n");
   strcat(buf, "<room_flags>  \nCurrent room flag bitvector: \n");
   send_to_char(buf, coder);
   show_room_flags(room->room_flags, coder);

   send_to_char(ROOM_FLAG_MENU2, coder);
   d->edit_mode = 4;
}
/******
 sprintf(tmp, "\t%5i", (world[ room->dir_option[i]->to_room ]).number);
******/



struct room_direction_data *to_room(struct room_direction_data *dir, struct char_data *coder)
{
   long vnum;
   struct descriptor_data *d = coder->desc;

   if ((STATE(d) == CON_REDITM)&&(d->edit_mode == 17))    /* Check input for validity. */
   {
      vnum = strtol(coder->desc->edit_str, NULL, 10);
      if (!strlen(coder->desc->edit_str))
      {   
         STATE(d) = CON_EDITM;
         d->edit_mode = 7;
         return(NULL);
      }
      else if ((dir->to_room = real_room(vnum)) < 0)
         send_to_char("Room with that Vnum does not exist, try again.\n\n", coder);  
      else 
      {
         coder->desc->room_editing->dir_option[d->tmp_int]->to_room = dir->to_room;
         STATE(d) = CON_EDITM;
         d->edit_mode = 7;
         return (dir);
      }
   }

   cls();
   sprintf(buf, "<to_room> <NL>: \n");
   strcat(buf, "  Enter he virtual number of the room to which the exit leads. \n");
   send_to_char(buf, coder);
   SET_REDITM(17);

   GET_INPUT_STR(12);
   return(dir);

}


void get_directions(struct room_data *room, struct char_data *coder, char arg)
{
   long choice = arg - '0';
   struct descriptor_data *d = coder->desc;

   if ((arg)&&(d->edit_mode == 7))     /* Check input for validity. */
   {
      switch(arg){
      case '0' :
      case '1' :
      case '2' :
      case '3' :
      case '4' :
      case '5' :
         d->tmp_int = choice;
         if (room->dir_option[choice] == NULL)
            CREATE(room->dir_option[choice], struct room_direction_data, 1);

         room->dir_option[choice] = to_room(room->dir_option[choice], coder);
         break;
      case 'Q' :
      case 'q' :
         RESET_EDITM();
         return;
         break;
      case 'H' :
      case 'h' :
         edit_help(d);
         break;
      default :
         send_to_char("\n -------------- Current Exits:  --------------\n", coder);
         show_exits(room, coder);
         send_to_char(DIRECTION_MENU, coder);
         d->edit_mode = 7;
         break;
      }
   }
   else
   {
      send_to_char("\n -------------- Current Exits:  --------------\n", coder);
      show_exits(room, coder);
      send_to_char(DIRECTION_MENU, coder);

      d->edit_mode = 7;
   }
}

void remove_spaces(char *str)
{
   char new_str[110];
   int new = 0;
   int curr = 0;

   for (; curr < strlen(str); curr++)
   {
      if ((str[curr] == '\n'))
         str[curr] = ' ';

      if ((str[curr] != '@')&&(str[curr] != '~'))
      {
         if (curr - 1 > 0)
            if ((isspace(str[curr - 1]))&&(!isspace(str[curr]))) 
               new_str[new++] = ' '; 

         if (!isspace(str[curr]))
            new_str[new++] = str[curr];
      }
      else
        new_str[new++] = '\0';
   }

   strcpy(str, new_str);

}

void extra_desc(struct room_data *room, struct char_data *coder)
{
   struct extra_descr_data *extra;
   struct descriptor_data *d = coder->desc;

   if (STATE(d) == CON_REDITM)     /* Check input for validity. */
   {
      send_to_char("testing keylist\n", coder);
      if (strlen(coder->desc->edit_str) == 0)
      {
         coder->desc->edit_mode = 0; STATE(d) = CON_EDITM; return;
      }
      else
      {
         sprintf(buf, "<description><NL>~<NL>\n");
         strcat(buf, "\tex:  You see a tall grey marle statue standing here.\n\n");
         strcat(buf, "\nEnter extra description: (@ to end)\n");
         send_to_char(buf, coder);

         extra = (struct extra_descr_data *) malloc (sizeof(struct extra_descr_data));
         extra->description = (char *) malloc (201*(sizeof(char)));
         extra->keyword = (char *) malloc (81*(sizeof(char)));

         strcpy(extra->keyword, coder->desc->edit_str);
         remove_spaces(extra->keyword);

         extra->next = NULL;
         if (room->ex_description != NULL)
            extra->next = room->ex_description;

         room->ex_description = extra;

         coder->desc->str = &extra->description;
         *(coder->desc->str) = 0;
         coder->desc->max_str = 200;
      }
      coder->desc->edit_mode = 0; STATE(d) = CON_EDITM; 
      return;
   }

      cls();
      send_to_char("Current extra descriptions: \n", coder);
      show_extra_keywords(room, coder);
      sprintf(buf, "Add extra description: \n---------------------- \n");
      strcat(buf, "<blank separated keyword list>~<NL>  \n");
      strcat(buf, "\tex:  statue grey marble\n\n");
      strcat(buf, "Enter --keyword-- list, ending with a @:\n");

      send_to_char(buf, coder);

   coder->desc->edit_mode = 8;   STATE(d) = CON_REDITM;

   coder->desc->str = &coder->desc->edit_str;
   *(coder->desc->str) = 0;
   coder->desc->max_str = 100;

}


void extra_desc2(struct room_data *room, struct char_data *coder)
{

   struct extra_descr_data *extra;
   struct descriptor_data *d = coder->desc;

   if (STATE(d) == CON_REDITM)     /* Check input for validity. */
   {
      send_to_char("testing keylist\n", coder);
      if (strlen(coder->desc->edit_str) != 0)
      {
         CREATE(extra, struct extra_descr_data, 1);
         strcpy(extra->keyword, coder->desc->edit_str);

         if (room->ex_description != NULL)
            extra->next = coder->desc->room_editing->ex_description;
         coder->desc->room_editing->ex_description = extra;

         sprintf(buf, "<description><NL>~<NL>\n");
         strcat(buf, "\tex:  You see a tall grey marble statue standing here.\n\n");
         strcat(buf, "\nEnter extra description: (@ to end)\n");
         CREATE(extra->description, char, 200);

         send_to_char(buf, coder);


         GET_FIELD(d->room_editing->ex_description->description, 200);

         if (strlen(d->room_editing->ex_description->description) > 0)
            send_to_char (d->room_editing->ex_description->description, coder);

      }
      RESET_EDITM();
      return;
   }

   cls();
   send_to_char("Current extra descriptions: \n", coder);
   show_extra_keywords(room, coder);


   send_to_char(KEYWORD_MENU, coder);

   SET_REDITM(8);
   GET_INPUT_STR(80);

}






/* Get the title of the room. */
void room_title(struct room_data *room, struct char_data *coder)
{

   cls();
   send_to_char(ROOM_TITLE_MENU, coder);
   GET_FIELD(room->name, 240);

}

/* Get the new description of the room. */
void room_desc(struct room_data *room, struct char_data *coder)
{
   send_to_char(ROOM_DESC_MENU, coder);

   GET_FIELD(room->description, 2400);

}




void display_room(struct room_data *room, struct char_data *coder)
{
   STATE(coder->desc) = CON_EDITM;

   if (room != NULL)
   {
      sprintf(buf, "\n\n-----------------------------------------------\n");
      strcat(buf, "|                  Room Data                  |\n");
      strcat(buf, "-----------------------------------------------\n\n");
      send_to_char(buf, coder);

      sprintf(buf, "1. Vnum: %i \t2. Zone: %i \t3. Sector_type: %i \n4. room_flags: \n",
             room->number, room->zone, room->sector_type);
      send_to_char(buf, coder);

      show_room_flags(room->room_flags, coder); 

      sprintf(buf, "5. Title: %s \n6. Room desc: \n  %s \n\n7. Exits: \n",
             room->name, room->description);

      send_to_char(buf, coder);

      show_exits(room, coder); 
      send_to_char("8. Extra desciptions: \n", coder);
      show_extra_keywords(room, coder); 

      sprintf(buf, "s. Save current room to file.\n");
      strcat(buf, "w. Save all modified rooms to file.\n");
/******
      strcat(buf, "r. Attempt to renum world.\n");
*****/
      strcat(buf, "h. Help\nq. Quit!!\n\n");
      strcat(buf, "Please enter menu choice: ");
      send_to_char(buf, coder);
   }

}

/* ========================= Save functions ========================== */
void save_entry(struct room_data *room)
{
   FILE *fptr;
   struct extra_descr_data *ptr;
   char tmp[800];
   int i;

   if (room == NULL)
      return;

   fptr = fopen("/ncsu/segodfr/bin/.mud/circle/bin/redit.save.wld", "a");

/*****
   fptr = fopen(SAVE_FILE, "a");
*****/

   fprintf(fptr, "#%d \n", room->number);
   if (room->name != NULL)
      fprintf(fptr, "%s~ \n", room->name);

   if (room->description != NULL)
      fprintf(fptr, "%s\n~\n", room->description);
   fprintf(fptr, "%i %i %i\n", room->zone, room->room_flags, room->sector_type);

   for (i=0; i < NUM_OF_DIRS; i++)
   {
      if (room->dir_option[i] != NULL)
         if (room->dir_option[i]->to_room != NOWHERE)
         {
            fprintf(fptr, "D%i \n", i);   /* Exit direction */
   
            if (room->dir_option[i]->general_description != NULL)
               fprintf(fptr, "%s", room->dir_option[i]->general_description);
   
            fprintf(fptr, "~\n");
   
            if (room->dir_option[i]->keyword != NULL)
               fprintf(fptr, "%s", room->dir_option[i]->keyword);
   
            fprintf(fptr, "~\n");
            fprintf(fptr, "%i %i %i \n", 
                room->dir_option[i]->exit_info, room->dir_option[i]->key,
                           room->dir_option[i]->to_room);
         }
   }

   if (room->ex_description != NULL)
   {
      sprintf(buf, "\0");
      for (ptr = room->ex_description; ptr->next != NULL; ptr = ptr->next)
      {
         sprintf(tmp, "E\n%s~\n%s\n~\n", ptr->keyword, ptr->description);
         strcat(buf, tmp);
      }
      sprintf(tmp, "E\n%s~\n%s\n~\n", ptr->keyword, ptr->description);
      strcat(buf, tmp);
      fprintf(fptr, "%s", buf);
   }

   fprintf(fptr, "S\n");

   fclose(fptr);
}

void save_world()
{
   struct llist_node *rooms = 0;
   FILE *fptr;

   fptr = fopen("/ncsu/segodfr/bin/.mud/circle/bin/redit.save.wld", "w");
   fclose(fptr);

   if (my_world)
   {
      rooms = my_world;

      while (rooms)
      {
         save_entry(rooms->room);
         rooms = rooms->next;
      } 
   }
}
/* ========================= Help functions ========================== */

void edit_help(struct descriptor_data *d)
{
   SEND_TO_Q(HELP_MENU, d);
}

/* ========================= Main functions ========================== */

void renum_new_world()
{
   struct room_data *new_world;
   int new_top_of_world = 0;
   struct llist_node *rooms = 0;
   register int room, door;
   int i = 0;
   char tmp[100];

   new_world = (struct room_data *) malloc ((1000 + top_of_world)*sizeof(struct room_data));
   if (my_world) 
      rooms = my_world;

   while ((i < top_of_world) && (rooms))
   {
      if (rooms->room->number < world[i].number)
      {
         new_world[new_top_of_world++] = *rooms->room;
         rooms = rooms->next;
      }
      else if (rooms->room->number == world[i].number)
      {
         new_world[new_top_of_world++] = world[i++];
         rooms = rooms->next;
      }
      else
         new_world[new_top_of_world++] = world[i++];

   }

   if (rooms)
      for(; rooms; rooms = rooms->next)
         new_world[new_top_of_world++] = *rooms->room;
   else
      while (i <= top_of_world)
         new_world[new_top_of_world++] = world[i++];

   new_top_of_world--;

   world = new_world;
   top_of_world = new_top_of_world;

   for (room = 0; room <= top_of_world; room++)
      for (door = 0; door <= 5; door++)
         if (world[room].dir_option[door])
            if (world[room].dir_option[door]->to_room != NOWHERE)
               world[room].dir_option[door]->to_room =
                   real_room(world[room].dir_option[door]->to_room);

}








/*=========================================================*/

void test()
{
   struct llist_node *rooms = 0;
   int count = 0;

   rooms = my_world;

   while(rooms)
   {
      sprintf(buf, "%d:  vnum:  %d", count, rooms->room->number);
      log(buf);
      count++;
      rooms = rooms->next;
   }
}


/*=========================================================*/

void edit_func(struct descriptor_data *d, char arg)
{
   if (STATE(d) == CON_REDITM)
   {
      switch(d->edit_mode)
      {
      case -1:
      case 0:
         display_room(d->room_editing, d->character);
         STATE(d) = CON_EDITM;
         break;
      case 1:
         get_vnum(d->room_editing, d->character);
         break;
      case 2:
         zone_number(d->room_editing, d->character);
         break;
      case 4:           /* Room flags */
         room_flags(d->room_editing, d->character, arg);
         break;
      case 8:
         extra_desc(d->room_editing, d->character);
         break;
      case 7:
         get_directions(d->room_editing, d->character, arg);
         break;
      case 17:
/****
         to_room(coder->desc->room_editing->dir_option[d->tmp_int], d->character);
****/
         to_room(d->room_editing, d->character);
         break;
      }
   }
   else if (d->edit_mode == 3)
      sector_type(d->room_editing, d->character, arg);
   else if (d->edit_mode == 4)
      room_flags(d->room_editing, d->character, arg);
   else if ((d->edit_mode == 7)||(d->edit_mode == 17))
      get_directions(d->room_editing, d->character, arg);
   else 
   {
      switch (arg) 
      {
      case '0':
         break;
      case '1':
         if (real_room(d->room_editing->number) < 0)
            get_vnum(d->room_editing, d->character);
         else
         {
            SEND_TO_Q("\n-- Sorry!! Can't modify this field for an existing room!! --\n ", d);
            SEND_TO_Q("Please enter menu choice: ", d);
         }
         break;
      case '2':
         zone_number(d->room_editing, d->character);
         break;
      case '3':
         sector_type(d->room_editing, d->character, arg);
         break;
      case '4':   
         room_flags(d->room_editing, d->character, arg);
         break;
      case '5':
         room_title(d->room_editing, d->character);
         break;
      case '6':
         room_desc(d->room_editing, d->character);
         break;
      case '7':
         get_directions(d->room_editing, d->character, arg);
         break;
      case '8':
         extra_desc(d->room_editing, d->character);
         break;
      case 's':
         save_entry(d->room_editing);
         display_room(d->room_editing, d->character);
         break;
      case 'w':
         save_world();
         display_room(d->room_editing, d->character);
         break;
      case 'h':
         edit_help(d);
         break;
      case 'T':
         test();
         break;
/*      case 'r':
         renum_new_world();
         break;
*/
      case 'q':
         SEND_TO_Q("Quiting edit mode.\n\r", d);
         STATE(d) = CON_PLYNG;
         d->edit_mode = 0;
         release_room(d->room_editing->number);
         return;
         break;
      default:
         SEND_TO_Q("Invalid menu choice.\n\r", d);
         display_room(d->room_editing, d->character);
         d->edit_mode = -1;
         break;
      }
      if ((d->edit_mode == 0)&&(real_room(d->room_editing->number) < 0))
         display_room(d->room_editing, d->character);
   }
}

void release_room(long vnum)
{
   rooms_in_use = remove_vnum_from_llist(rooms_in_use, vnum);
}

int room_being_edited(long vnum)
{
   return((int) find_room_llist(rooms_in_use, vnum));
}

struct room_data *my_real_room(int vnum)
{
   return(find_room_llist(my_world, vnum));
} 
         
void edit_room(struct room_data *room, struct descriptor_data *d)
{

   STATE(d) = CON_EDITM;

   if (room == NULL)
   {
      if ((room = my_real_room(atoi(d->edit_str))) == NULL)
         room = create_room(strtol(d->edit_str, NULL, 10));

      d->room_editing = room;
      d->edit_mode = 0;

   }

   if (room_being_edited(room->number))
   {
      SEND_TO_Q("*** Room currently being edited. Try again later. ***\n\r", d);
      STATE(d) = CON_PLYNG;
      return;
   }
   else
   {
      my_world = add_room_to_llist(my_world, room);
      rooms_in_use = add_room_to_llist(rooms_in_use, room);
   }

   display_room(room, d->character);
}


