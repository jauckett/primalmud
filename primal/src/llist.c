#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "structs.h"
#include "edit.h"
#include "llist.h"

/*
static struct llist_node *llist_root = 0;
*/


struct llist_node *search_llist(struct llist_node *llist_root, int vnum)
{
   struct llist_node *ptr = llist_root;

   while (ptr != NULL)
      if (ptr->room->number == vnum)
         return(ptr);
      else
         ptr = ptr->next;

   return(NULL);

}


struct llist_node *delete_llist(struct llist_node *llist_root, struct room_data *room)
{
   struct llist_node *ptr = llist_root;
   char tmp[100];

   if (!room)
      return;

   if (!search_llist(llist_root, room->number)) /* Room doesn't exists */
      return(llist_root);
   
   while (ptr->next != NULL)
   {
      if (ptr->room->number != room->number)
         ptr = ptr->next;
      else
         break;
   }

   if (ptr == llist_root)
   {
      llist_root = llist_root->next;
      if (llist_root)
         llist_root->prev = NULL;
   }
   else
   {
      if (ptr->prev != NULL)
         ptr->prev->next = ptr->next;
      if (ptr->next != NULL)
         ptr->next->prev = ptr->prev;
      free(ptr);
   }

   return(llist_root);
}

struct llist_node *insert_llist (struct llist_node *llist_root, struct room_data *room)
{
   struct llist_node *new;
   struct llist_node *ptr = llist_root;

   if (room == NULL) 
      return(llist_root);

   if (search_llist(llist_root, room->number) != NULL)
      return(llist_root);  /* Room already exists in list. */

   new = (struct llist_node *) malloc (sizeof(struct llist_node));
   new->room = room;
   new->prev = new->next = NULL;
   ptr = llist_root;

   if (llist_root == NULL)  /* There was no llist_root, new is the new llist_root. */ 
      return(new); 
 
   while (ptr->next != NULL)
   {
      if (ptr->room->number < room->number)
         ptr = ptr->next;
      else
         break;
   }

   if (ptr->room->number < room->number)  /* put new after ptr. */
   {
      new->prev = ptr;
      new->next = ptr->next;
      if (ptr->next != NULL)
         ptr->next->prev = new;
      ptr->next = new;
   }
   else  /* new goes before the ptr. */
   {
      new->prev = ptr->prev;
      new->next = ptr;
      if (ptr->prev != NULL)
         ptr->prev->next = new;
      ptr->prev = new;
      if (ptr == llist_root)
         llist_root = new;
   }

   return(llist_root);

}
void print_node_llist(struct llist_node *ptr)
{
   if (ptr != NULL)
      printf("vnum: %5d\n", ptr->room->number);
}


void inorder_llist(struct llist_node *ptr)
{
   if (ptr != NULL)
   {
      print_node_llist(ptr);
      inorder_llist(ptr->next);
   }
}

/*=============================== Public Functions =====================================*/
void print_llist(struct llist_node *llist_root)
{
   printf("Printing llist...\n\n");
   inorder_llist(llist_root);
}

struct llist_node *add_room_to_llist(struct llist_node *llist_root, struct room_data *room)
{
   return(insert_llist (llist_root, room));
}

struct llist_node *remove_room_from_llist(struct llist_node *llist_root, struct room_data *room)
{
   return(delete_llist(llist_root, room));
}

struct llist_node *remove_vnum_from_llist(struct llist_node *llist_root, int vnum)
{
   return(delete_llist(llist_root, (search_llist(llist_root, vnum))->room));
}


struct room_data *find_room_llist(struct llist_node *llist_root, int vnum)
{
   if (search_llist(llist_root, vnum) == NULL)
      return(NULL);
   else
      return ((search_llist(llist_root, vnum))->room);
}
