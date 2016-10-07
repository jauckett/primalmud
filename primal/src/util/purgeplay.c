/* 
************************************************************************
*  file: purgeplay.c                                    Part of CircleMUD * 
*  Usage: purge useless chars from playerfile                             *
*  All Rights Reserved                                                    *
*  Copyright (C) 1992, 1993 The Trustees of The Johns Hopkins University  *
************************************************************************* */

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "../structs.h"
#include "../utils.h"


int new_id=1;


void	purge(char *filename)
{
   FILE * fl;
   FILE * outfile;
   struct char_file_u player;
   int	okay, num = 0;
   long	timeout;
   char	*ptr, reason[80];

   if (!(fl = fopen(filename, "r+"))) {
      printf("Can't open %s.", filename);
      exit();
   }

   outfile = fopen("players.new", "w");
   printf("Deleting: \n");

   for (; ; ) {
      fread(&player, sizeof(struct char_file_u ), 1, fl);
      if (feof(fl)) {
	 fclose(fl);
	 fclose(outfile);
	 puts("Done.");
	 exit();
      }

      okay = 1;
      *reason = '\0';

      for (ptr = player.name; *ptr; ptr++)
	 if (!isalpha(*ptr) || *ptr == ' ') {
	    okay = 0;
	    strcpy(reason, "Invalid name");
	 }

      if (player.level == 0) {
	 okay = 0;
	 strcpy(reason, "Never entered game");
      }

      if ((player.level == 0) || (player.level > LVL_OWNER)) {
	 okay = 0;
	 strcpy(reason, "Invalid level");
      }
if ( (player.char_specials_saved.idnum==12178) || 
     (player.char_specials_saved.idnum==11971))
 {
         okay = 0;
         strcpy(reason, "Invalid ID");
}

      /* now, check for timeouts.  They only apply if the char is not
       cryo-rented.   LEV_CHAMP + do not time out.  */

      if (okay && (player.level < 70)) {

	 if (!(player.char_specials_saved.act & PLR_CRYO)) {
	    if (player.level == 1)
	       timeout = 14;	/* Lev   1 : 2 weeks */
	    else if (player.level <= 4)	
	       timeout = 21;	/* Lev 2-4 : 3 weeks */
	    else if (player.level <= 10)	
	       timeout = 60;	/* Lev 5-10: 60 days */
	    else  
	       timeout = 180;	/* Lev 11-(LVL_CHAMP-1) : 180 days */
	 } else
	       timeout = 3650;
	 timeout *= SECS_PER_REAL_DAY;

	 if ((time(0) - player.last_logon) > timeout) {
	    okay = 0;
	    sprintf(reason, "Level %2d idle for %4ld days", player.level,
	        ((time(0) - player.last_logon) / SECS_PER_REAL_DAY));
	 }
      }

      if (player.char_specials_saved.act & PLR_DELETED) {
	 okay = 0;
	 sprintf(reason, "Deleted flag set");
       }
/*
      if (!okay && (player.char_specials_saved.act & PLR_NODELETE)) {
	 okay = 2;
	 strcat(reason, "; NOT deleted.");
      }
*/

      if (okay)
      {

	player.char_specials_saved.idnum=new_id++;

	 fwrite(&player, sizeof(struct char_file_u ), 1, outfile);
      
      }
      else
	 printf("%4d. %-20s %s\n", ++num, player.name, reason);

      if (okay == 2)
	 fprintf(stderr, "%-20s %s\n", player.name, reason);
   }
}



void main(int argc, char *argv[])
{
   if (argc != 2)
      printf("Usage: %s playerfile-name\n", argv[0]);
   else
      purge(argv[1]);

  return;
}


