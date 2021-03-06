/* ************************************************************************
*   File: gambling.c                                    Part of CircleMUD *
*  Usage: Special procedures for Casino in West World                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Special procedures for Gambling House by Drax & Pkunk                  *
*  Coded by Drax and Pkunk                                                *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdlib.h>

#include "structs.h"
#include "screen.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"


/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
/* Mob specs for the gambling house */


void play_high_dice(struct char_data *ch, struct char_data *dealer, int bet, int minbet, int maxbet)
{
     int die1, die2, die3, die4;

     if (GET_GOLD(ch) < bet) {
        act("$n says, 'I'm sorry but you don't have that much gold.'",
            FALSE, dealer, 0, ch, TO_VICT);
        return;
     } else if (bet > maxbet) {
	strcpy(buf, "");
        sprintf(buf, "$n says, 'I'm sorry but the limit is %d gold pieces.'", maxbet);
        act(buf , FALSE, dealer, 0, ch, TO_VICT);
        return;
      }else if (bet < minbet) {
          strcpy(buf, "");
          sprintf(buf, "$n says, 'I'm sorry but the minimum bet is %d gold pieces.'", minbet);
          act(buf , FALSE, dealer, 0, ch, TO_VICT);
          return;
     } else {
        GET_GOLD(ch) -= bet;
        act("$N places $S bet on the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
        act("You place your bet on the table.", FALSE, ch, 0, 0, TO_CHAR);
     }
 
     /* dealer rolls two dice */
     die1 = number(1, 6);
     die2 = number(1, 6);
 
     sprintf(buf, "$n rolls the dice, $e gets %d, and %d, for a total of %d.",
       die1, die2, (die1 + die2));
     act(buf, FALSE, dealer, 0, ch, TO_ROOM);
     /* now its the players turn */
     die3 = number(1, 6);
     die4 = number(1, 6);
 
     sprintf(buf, "$N rolls the dice, $E gets %d, and %d, for a total of %d.",
       die3, die4, (die3 + die4));
     act(buf, FALSE, dealer, 0, ch, TO_NOTVICT);
   sprintf(buf, "You roll the dice, and get %d, and %d, for a total of %d.\r\n",
       die3, die4, (die3 + die4));
     send_to_char(buf, ch);
 
     if ((die1+die2) >= (die3+die4))
     {
        sprintf(buf, "The house wins %d coins from $N.", bet);
        act(buf, FALSE, dealer, 0, ch, TO_NOTVICT);
        sprintf(buf, "The house wins %d coins from you.\r\n", bet);
        send_to_char(buf, ch);
     } else {
        sprintf(buf, "$N wins %d gold coins!", bet*2);
        act(buf, FALSE, dealer, 0, ch, TO_NOTVICT);
        sprintf(buf, "You win %d gold coins!\r\n", bet*2);
        send_to_char(buf, ch);
        GET_GOLD(ch) += (bet*2);
     }
     return;
}       

SPECIAL(high_dice){
 struct char_data *dealr = (struct char_data *) me;
 int bet;

   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
  send_to_char("To play look rules or look croupier.\r\n", ch);
        return 1;
 }
 
 if (CMD_IS("bet")){
        one_argument(argument, buf);
        if(!*buf){
      send_to_char("Yes but bet how much? bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Please bet a valid positive, non-zero amount!\r\n", ch);
          return 1;
        }
        play_high_dice(ch, dealr, bet, 10, 10000);
        return 1;
  }
return 0;
}

SPECIAL(high_dice1){
 struct char_data *dealr = (struct char_data *) me;
 int bet;

   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
  send_to_char("To play look rules or look croupier.\r\n", ch);
        return 1;
 }
 
 if (CMD_IS("bet")){
        one_argument(argument, buf);
        if(!*buf){
      send_to_char("Yes but bet how much? bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Please bet a valid positive, non-zero amount!\r\n", ch);
          return 1;
        }
        play_high_dice(ch, dealr, bet, 10000, 100000);
        return 1;
  }
return 0;
}


void play_craps(struct char_data *ch, struct char_data *dealer, int bet, int minbet, int maxbet)
{
     int  die1, die2, mark = 0, last = 0;
     bool won = FALSE, firstime = TRUE;
 
     if (GET_GOLD(ch) < bet) {
        act("$n says, 'I'm sorry sir but you don't have that much gold.'",
            FALSE, dealer, 0, ch, TO_VICT);
        return;
     } else if (bet > maxbet) {
	strcpy(buf, "");
        sprintf(buf, "$n says, 'I'm sorry but the limit is %d gold pieces.'", maxbet);
        act(buf , FALSE, dealer, 0, ch, TO_VICT);
        return;
      }else if (bet < minbet) {
          strcpy(buf, "");
          sprintf(buf, "$n says, 'I'm sorry but the minimum bet is %d gold pieces.'", minbet);
          act(buf , FALSE, dealer, 0, ch, TO_VICT);
          return;
     } else {
        GET_GOLD(ch) -= bet;
        act("$N places $S bet on the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
        act("You place your bet on the table.", FALSE, ch, 0, 0, TO_CHAR);
     }
 
     act("$n hands $N the dice and says, 'roll 'em'",
          FALSE, dealer, 0, ch, TO_NOTVICT);
     act("$n hands you the dice and says, 'roll 'em'",
          FALSE, dealer, 0, ch, TO_VICT);
 
     while (won != TRUE) {
       die1 = number(1, 6);
       die2 = number(1, 6);
       mark = die1 + die2;
 
       sprintf(buf, "$n says, '$N rolls %d and %d, totalling %d.",
              die1, die2, mark);
       act(buf, FALSE, dealer, 0, ch, TO_ROOM);
 
       if ((mark == 7  || mark == 11) && firstime) {
          /* win on first roll of the dice! 3x bet */
          act("$n says, 'Congratulations $N, you win!'",
               FALSE, dealer, 0, ch, TO_ROOM);
          act("$n hands $N some gold pieces.",
               FALSE, dealer, 0, ch, TO_NOTVICT);
          sprintf(buf, "$n hands you %d gold pieces.", (bet*2));
          act(buf, FALSE, dealer, 0, ch, TO_VICT);
          GET_GOLD(ch) += (bet*2);
          won = TRUE;
       } else if ((mark == 2 || mark == 3 || mark == 12) && firstime) {
          /* player loses on first roll */
          act("$n says, 'Sorry $N, you lose.'", FALSE, dealer, 0, ch, TO_ROOM);
          act("$n takes $N's bet from the table.",
               FALSE, dealer, 0, ch, TO_NOTVICT);
          act("$n takes your bet from the table.",
               FALSE, dealer, 0, ch, TO_VICT);
          won = TRUE;
       } else if ((last == mark) && !firstime) {
          /* player makes $s mark! 2x bet */
          act("$n says, 'Congratulations $N, you win!'",
               FALSE, dealer, 0, ch, TO_ROOM);
          act("$n hands $N some gold pieces.",
               FALSE, dealer, 0, ch, TO_NOTVICT);
          sprintf(buf, "$n hands you %d gold pieces.", (bet*2));
          act(buf, FALSE, dealer, 0, ch, TO_VICT);
          GET_GOLD(ch) += (bet*2);
          won = TRUE; 
       } else if ((mark == 7) && !firstime) {
          /* player misses $s mark by rolling a 7 */
          act("$n says, 'Bad luck $N, you lost!'",
               FALSE, dealer, 0, ch, TO_ROOM);
          act("$n takes $N's gold pieces for the house.",
               FALSE, dealer, 0, ch, TO_NOTVICT);
          sprintf(buf, "$n takes your %d gold pieces.", (bet));
          act(buf, FALSE, dealer, 0, ch, TO_VICT);
          won = TRUE;    
       } else if (firstime) {
          sprintf(buf, "$n says, '$N's mark is %d.  Roll 'em again $N!'", mark);
          act(buf, FALSE, dealer, 0, ch, TO_ROOM);
          firstime = FALSE;
          last = mark;
          won = FALSE;
       }
     }
     return;
}

SPECIAL(craps){
 struct char_data *dealr = (struct char_data *) me;
 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
  send_to_char("To play Craps look rules or look croupier\r\n", ch);
        return 0;
 }  

 if (CMD_IS("bet")){
        one_argument(argument, buf);
        if(!*buf){
      send_to_char("Yes but bet how much? bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Please bet a valid positive, non-zero amount!\r\n", ch);
          return 1;
        }
        play_craps(ch, dealr, bet, 10, 10000);
        return 1;
  }
return 0;
}
 
SPECIAL(craps1){
 struct char_data *dealr = (struct char_data *) me;
 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
  send_to_char("To play Craps look rules or look croupier\r\n", ch);
        return 0;
 }  

 if (CMD_IS("bet")){
        one_argument(argument, buf);
        if(!*buf){
      send_to_char("Yes but bet how much? bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Please bet a valid positive, non-zero amount!\r\n", ch);
          return 1;
        }
        play_craps(ch, dealr, bet, 10000, 100000);
        return 1;
  }
return 0;
}


void play_roulette(struct char_data *ch, struct char_data *dealer, char *guess, int bet, int minbet, int maxbet)
{
     const int black[18]={2,4,6,8,10,11,13,15,17,20,22,24,26,28,29,31,33,35};
     const int red[18]={1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36};
     const int col1[12]={1,4,7,10,13,16,19,22,25,28,31,34};
     const int col2[12]={2,5,8,11,14,17,20,23,26,29,32,35};
     const int col3[12]={3,6,9,12,15,18,21,24,27,30,33,36};

     int wheel;
     int redflag=0, blackflag=0, oddflag=0, evenflag=0, col1f, col2f, col3f;
     int anum=0, i=0, win=0, intguess=0;
     char color[10];
     div_t d;
 
	redflag = 0;blackflag = 0; oddflag = 0; anum = 0; evenflag = 0;
        col1f=0;col2f=0;col3f=0;

     if (strlen(guess) <= 2){
	anum = 1;
     }

        if (GET_GOLD(ch) < bet) {
        act("$n says, 'I'm sorry sir but you don't have that much gold.'",FALSE, dealer, 0, ch, TO_VICT);
        return;
     } else if (bet > maxbet) {
	strcpy(buf, "");
        sprintf(buf, "$n says, 'I'm sorry but the limit is %d gold pieces.'", maxbet);
        act(buf , FALSE, dealer, 0, ch, TO_VICT);
        return;
      }else if (bet < minbet) {
          strcpy(buf, "");
          sprintf(buf, "$n says, 'I'm sorry but the minimum bet is %d gold pieces.'", minbet);
          act(buf , FALSE, dealer, 0, ch, TO_VICT);
          return;
     } else
       if (anum){
	if (atoi(guess)>36){
     		act("$n tells you, 'That is not a valid number!. Must be 0-36!!.'", FALSE, dealer, 0, ch, TO_VICT); 
        	return;
	}
     }else {
        GET_GOLD(ch) -= bet;
        act("$N places $S bet on the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
        send_to_char("You place your bet on the table.\r\n", ch);
     }
 
     if (!*guess) {
        act("$n tells you, 'Please make a valid bet, look croupier or look rules.'",FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);
        GET_GOLD(ch) += bet;
        return;
     }

     if (anum==0){
       if (!(!strcmp(guess, "low18") || \
           !strcmp(guess, "high18") || \
           !strcmp(guess, "column1") || \
           !strcmp(guess, "column2") || \
           !strcmp(guess, "column3") || \
           !strcmp(guess, "red") || \
           !strcmp(guess, "black") || \
           !strcmp(guess, "low12") || \
           !strcmp(guess, "mid12") || \
           !strcmp(guess, "high12") || \
           !strcmp(guess, "even") || \
           !strcmp(guess, "odd"))) {
        	act("$n tells you, 'You need to specify your bet, look croupier.'", FALSE, dealer, 0, ch, TO_VICT); 
	        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);

	        GET_GOLD(ch) += bet;
        	return;
      }
    } 
    if (anum){
        act("$N places $S bet on the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
        act("You place your bet on the table.", FALSE, ch, 0, 0, TO_CHAR);
	GET_GOLD(ch) -= bet;
     }
     sprintf(buf, "$n placed $s bet on : %s%s%s.", CCCYN(ch, C_CMP), guess, CCNRM(ch, C_CMP));
     act(buf, FALSE, ch, 0 , 0, TO_ROOM);
     sprintf(buf, "You place your bet on: %s%s%s\r\n",  CCCYN(ch, C_CMP), guess, CCNRM(ch, C_CMP));
     send_to_char(buf, ch);

     wheel = number(0, 36);
     i = 0;
     do{
        	if (wheel == red[i])
                	redflag=1;
		i++;
     } while (i<18);

     if (redflag==0){
     i=0;
     do{
        	if (wheel == black[i])
                	blackflag=1;
		i++;
     } while (i<18);
     }
	sprintf(color, "%sgreen%s", CCGRN(ch, C_CMP), CCNRM(ch, C_CMP));
     if (redflag)
	sprintf(color, "%sred%s", CCRED(ch, C_CMP), CCNRM(ch, C_CMP));
     if (blackflag)
	sprintf(color, "%sblack%s", CCBGRY(ch, C_CMP), CCNRM(ch, C_CMP));

        act(" The croupier flicks his wrist sending the white marble", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(" The croupier flicks his wrist sending the white marble\n", ch);
        act(" ball spinning in circles before finally bouncing", FALSE, ch, 0, 0, TO_ROOM);                   
	send_to_char(" ball spinning in circles before finally bouncing\n", ch);
        act(" around and landing in a numbered slot.", FALSE, ch, 0, 0, TO_ROOM);
	send_to_char(" around and landing in a numbered slot.\n", ch);
        sprintf(buf, " ..... '%d %s!', announces the croupier.\r\n", wheel, color);
     	send_to_char(buf, ch); 
	act(buf, FALSE, ch, 0, 0, TO_ROOM);

	d = div(wheel,2);

       if (d.rem>0)
		oddflag=1;
       else
		evenflag=1;

/* check columns the number is is ie. 1 2 or 3 */

     for (i=0;i<12;i++){
        if (col1[i] == wheel)
                col1f=1;
     }
     for (i=0;i<12;i++){
        if (col2[i] == wheel)
                col2f=1;
     }
     for (i=0;i<12;i++){
        if (col3[i] == wheel)
                col3f=1;
     }

/* end column check - Drax */

     if (anum == 1)
     {
	intguess = atoi(guess);
	sprintf(buf, "Your bet is on number: %d\r\n", intguess);
	send_to_char(buf, ch);	
	if (intguess == wheel){
	  send_to_char("Congratulations you win!!!\r\n", ch);
	  win = bet * 35;
	  sprintf(buf,"$n wins %d gold coins!!", win);
	  act(buf, FALSE, ch, 0, 0, TO_ROOM);
	  sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	  send_to_char(buf, ch); 
	  GET_GOLD(ch) += win;
	  return;
       }else
	send_to_char("Sorry you lose!\r\n", ch);
	return;
     }


      if ((!strcmp(guess,"red"))|| (!strcmp(guess, "black")))
      {
         if ((!strcmp(guess, "red")) && (redflag == 1)){
	    send_to_char("Congratulations you win!!!\r\n", ch);
	    win = bet * 2;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) += win;
	    return;
         }

         if ((!strcmp(guess, "black")) && (blackflag == 1)){
	     send_to_char("Congratulations you win!!!\r\n", ch);
	     win = bet * 2;
	     sprintf(buf,"$n wins %d gold coins!!", win);
	     act(buf, FALSE, ch, 0, 0, TO_ROOM);
	     sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	     send_to_char(buf, ch);
	     GET_GOLD(ch) += win;
	     return;
         }else
	     send_to_char("Sorry you lose!\r\n", ch); 
             return;     
	} 

     if ((!strcmp(guess,"odd"))|| (!strcmp(guess, "even")))	
      {
         if ((!strcmp(guess, "odd")) && (oddflag == 1)){
	    send_to_char("Congratulations you win!!!\r\n", ch);
	    win = bet * 2;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) += win;
	    return;
         }

         if ((!strcmp(guess, "even")) && (evenflag == 1)){
	     send_to_char("Congratulations you win!!!\r\n", ch);
	     win = bet * 2;
	     sprintf(buf,"$n wins %d gold coins!!", win);
	     act(buf, FALSE, ch, 0, 0, TO_ROOM);
	     sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	     send_to_char(buf, ch);
	     GET_GOLD(ch) += win;
	     return;
         }else
	     send_to_char("Sorry you lose!\r\n", ch); 
             return;     
	} 



      if ((!strcmp(guess,"column1")) || (!strcmp(guess, "column2")) || (!strcmp(guess, "column3")))
      {
         if ((!strcmp(guess, "column1")) && (col1f == 1)){
	    send_to_char("Congratulations you win!!!\r\n", ch);
	    win = bet * 3;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) += win;
	    return;
         }
         if ((!strcmp(guess, "column2")) && (col2f == 1)){
	    send_to_char("Congratulations you win!!!\r\n", ch);
	    win = bet * 3;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) += win;
	    return;
         }
         if ((!strcmp(guess, "column3")) && (col3f == 1)){
	    send_to_char("Congratulations you win!!!\r\n", ch);
	    win = bet * 3;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) += win;
	    return;
         }
        }

/* low12, mid12, high12 betting check */

      if ((!strcmp(guess,"low12")) || (!strcmp(guess, "mid12")) || (!strcmp(guess, "high12")))
      {
         if ((!strcmp(guess, "low12")) && (wheel>=1 && wheel<=12)){
	    send_to_char("Congratulations you win!!!\r\n", ch);
	    win = bet * 3;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) += win;
	    return;
         }

         if ((!strcmp(guess, "mid12")) && (wheel>=13 && wheel<=24)){
	     send_to_char("Congratulations you win!!!\r\n", ch);
	     win = bet * 3;
	     sprintf(buf,"$n wins %d gold coins!!", win);
	     act(buf, FALSE, ch, 0, 0, TO_ROOM);
	     sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	     send_to_char(buf, ch);
	     GET_GOLD(ch) += win;
	     return;
         }

         if ((!strcmp(guess, "high12")) && (wheel>=25 && wheel<=36)){
	    send_to_char("Congratulations you win!!!\r\n", ch);
	    win = bet * 3;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) += win;
	    return;
         }else
	     send_to_char("Sorry you lose!\r\n", ch); 
             return;     
	} 

/* end of low/mid/nigh12 routines */
/* Low high18 bets checks */

       if ((!strcmp(guess,"low18")) || (!strcmp(guess, "high18")))
 	{
         if ((!strcmp(guess, "low18")) && (wheel <= 18) && (wheel != 0)) {
            send_to_char("Congratulations you win!!!\r\n", ch);
            win = bet * 2;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
            sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
            send_to_char(buf, ch);
            GET_GOLD(ch) += win;
            return;  
         }

         if ((!strcmp(guess, "high18")) && (wheel >= 19) && (wheel != 0)){
            send_to_char("Congratulations you win!!!\r\n", ch);
            win = bet * 2;
	    sprintf(buf,"$n wins %d gold coins!!", win);
	    act(buf, FALSE, ch, 0, 0, TO_ROOM);
            sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
            send_to_char(buf, ch);
            GET_GOLD(ch) += win;
            return;
          } else{
             send_to_char("Sorry you lose!\r\n", ch);
             return;
        }
      }
  return;
}

SPECIAL(roulette)
{
 struct char_data *dealr = (struct char_data *) me;

 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
   send_to_char("To play look rules or look croupier\r\n", ch);
   return 1;
 }
 
 if (CMD_IS("bet")){
        two_arguments(argument, buf, arg);
        if(!*buf){
          send_to_char("Syntax is:- bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        if(!*arg){
          send_to_char("Read the rules look rules look croupier.\r\n", ch);
          return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Must place a valid bet of 1gp or more.!\r\n", ch);
          return 1;
        }
        play_roulette(ch, dealr, arg, bet, 10, 10000);
	return 1;
}
return 0;
}

SPECIAL(roulette1)
{
 struct char_data *dealr = (struct char_data *) me;

 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
   send_to_char("To play look rules or look croupier\r\n", ch);
   return 1;
 }
 
 if (CMD_IS("bet")){
        two_arguments(argument, buf, arg);
        if(!*buf){
          send_to_char("Syntax is:- bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        if(!*arg){
          send_to_char("Read the rules look rules look croupier.\r\n", ch);
          return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Must place a valid bet of 1gp or more.!\r\n", ch);
          return 1;
        }
        play_roulette(ch, dealr, arg, bet, 10000, 100000);
	return 1;
}
return 0;
}


void play_triples(struct char_data *ch, struct char_data *dealer, char *guess, int bet, int minbet, int maxbet) 
{
     int die1, die2, die3, total = 0;
 
     if (GET_GOLD(ch) < bet) {
        act("$n says, 'I'm sorry sir but you don't have that much gold.'",
            FALSE, dealer, 0, ch, TO_VICT);
        return;
     } else if (bet > maxbet) {
	strcpy(buf, "");
        sprintf(buf, "$n says, 'I'm sorry but the limit is %d gold pieces.'", maxbet);
        act(buf , FALSE, dealer, 0, ch, TO_VICT);
        return;
      }else if (bet < minbet) {
          strcpy(buf, "");
          sprintf(buf, "$n says, 'I'm sorry but the minimum bet is %d gold pieces.'", minbet);
          act(buf , FALSE, dealer, 0, ch, TO_VICT);
          return;
     } else {
        GET_GOLD(ch) -= bet;
     act("$N places $S bet on the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
     act("You place your bet on the table.", FALSE, ch, 0, 0, TO_CHAR);
     }
 
     if (!*guess) {
        act("$n tells you, 'You need to specify upper, lower, or triple.'",
            FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);
        GET_GOLD(ch) += bet;
        return;
     }
 
     if (!(!strcmp(guess, "upper") || \
           !strcmp(guess, "lower") || \
           !strcmp(guess, "triple"))) {
        act("$n tells you, 'You need to specify upper, lower, or triple.'",
            FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);
 
        GET_GOLD(ch) += bet;
        return;
     }
 
     die1 = number(1, 6);
     die2 = number(1, 6);
     die3 = number(1, 6);
 
     total = die1 + die2 + die3;
 
     sprintf(buf, "$N rolls %d, %d, and %d", die1, die2, die3);
     if (die1 == die2 && die2 == die3)
        strcat(buf, ", $E scored a triple!");
     else
        strcat(buf, ".");
     act(buf, FALSE, dealer, 0, ch, TO_NOTVICT);
     sprintf(buf, "You roll %d, %d, and %d, for a total of %d.\r\n", die1,
             die2, die3, total);
     send_to_char(buf, ch);
     if ((die1 == die2 && die2 == die3) && !strcmp(guess, "triple")) {
        /* scored a triple! player wins 3x the bet */
        act("$n says, 'Congratulations $N, you win.'",
             FALSE, dealer, 0, ch, TO_ROOM);
        sprintf(buf, "$n hands you %d gold pieces.", (bet*35));
        act(buf, FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands $N some gold pieces.", FALSE, dealer, 0, ch, TO_NOTVICT);
        GET_GOLD(ch) += (bet*35);
     } else if ((total <= 9 && !strcmp(guess, "lower")) ||
                (total > 9  && !strcmp(guess, "upper"))) {
        act("$n says, 'Congratulations $N, you win.'",
             FALSE, dealer, 0, ch, TO_ROOM);
        sprintf(buf, "$n hands you %d gold pieces.", (bet*2));
        act(buf, FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands $N some gold pieces.", FALSE, dealer, 0, ch, TO_NOTVICT);
        GET_GOLD(ch) += (bet*2);
     } else {
        act("$n says, 'Sorry $N, better luck next time.'",
             FALSE, dealer, 0, ch, TO_ROOM);
        act("$n greedily counts $s new winnings.",
             FALSE, dealer, 0, ch, TO_ROOM);
     }
     return;
}

SPECIAL(triples){
 struct char_data *dealr = (struct char_data *) me;
 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
   send_to_char("To play look rules or look croupier.\r\n", ch);
   return 1;
 }
 
 if (CMD_IS("bet")){
        two_arguments(argument, buf, arg);
        if(!*buf){
          send_to_char("Syntax is:- bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        if(!*arg){
   send_to_char("Read the rules, look rules or look croupier.\r\n", ch);
          return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Must place a valid bet of 1gp or more.!\r\n", ch);
          return 1;
        }
        play_triples(ch, dealr, arg, bet, 10, 10000);
        return 1;
}
return 0;
}

SPECIAL(triples1){
 struct char_data *dealr = (struct char_data *) me;
 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
   send_to_char("To play look rules or look croupier.\r\n", ch);
   return 1;
 }
 
 if (CMD_IS("bet")){
        two_arguments(argument, buf, arg);
        if(!*buf){
          send_to_char("Syntax is:- bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        if(!*arg){
   send_to_char("Read the rules, look rules or look croupier.\r\n", ch);
          return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Must place a valid bet of 1gp or more.!\r\n", ch);
          return 1;
        }
        play_triples(ch, dealr, arg, bet, 10, 100000);
        return 1;
}
return 0;
}


void play_seven(struct char_data *ch, struct char_data *dealer, char *guess, int bet, int minbet, int maxbet)
{
     int die1, die2, total = 0;
 
     if (GET_GOLD(ch) < bet) {
        act("$n says, 'I'm sorry sir but you don't have that much gold.'",
            FALSE, dealer, 0, ch, TO_VICT);
        return;
     } else if (bet > maxbet) {
	strcpy(buf, "");
        sprintf(buf, "$n says, 'I'm sorry but the limit is %d gold pieces.'", maxbet);
        act(buf , FALSE, dealer, 0, ch, TO_VICT);
        return;
      }else if (bet < minbet) {
          strcpy(buf, "");
          sprintf(buf, "$n says, 'I'm sorry but the minimum bet is %d gold pieces.'", minbet);
          act(buf , FALSE, dealer, 0, ch, TO_VICT);
          return;
     } else {
        GET_GOLD(ch) -= bet;
        act("$N places $S bet on the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
        act("You place your bet on the table.", FALSE, ch, 0, 0, TO_CHAR);
     }
 
     if (!*guess) {
        act("$n tells you, 'You need to specify under, over, or seven.'",
            FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);
        GET_GOLD(ch) += bet;
        return;
     }
     if (!(!strcmp(guess, "under") || \
           !strcmp(guess, "over")  || \
           !strcmp(guess, "seven"))) {
        act("$n tells you, 'You need to specify under, over, or seven.'",
            FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);
        GET_GOLD(ch) += bet;
 
        return;
     }
 
     act("$n says, 'Roll the dice $N and tempt lady luck.'",
         FALSE, dealer, 0, ch, TO_ROOM);
 
     die1 = number(1, 6);
     die2 = number(1, 6);
     total = die1 + die2;
 
  sprintf(buf, "$N rolls the dice, getting a %d and a %d. For a total of %d.",
                die1, die2, total);
     act(buf, FALSE, dealer, 0, ch, TO_NOTVICT);
          
    
sprintf(buf, "You roll the dice, they come up %d and %d for a total of %d.\r\n",
              die1, die2, total);
     send_to_char(buf, ch);
     
     if ((total < 7 && !strcmp(guess, "under")) || \
         (total > 7 && !strcmp(guess, "over"))) {
        /* player wins 2x $s money */
        act("$n says, 'Congratulations $N, you win!'", FALSE, dealer, 0, ch, TO_ROOM);
        act("$n hands $N some gold pieces.", FALSE, dealer, 0, ch, TO_NOTVICT);
        sprintf(buf, "$n hands you %d gold pieces.", (bet*2));
        act(buf, FALSE, dealer, 0, ch, TO_VICT);
        GET_GOLD(ch) += (bet*2);
     } else if (total == 7 && !strcmp(guess, "seven")) {
        /* player wins 5x $s money */
	act("$n says, 'Congratulations $N, you win!'", FALSE, dealer, 0, ch, TO_ROOM);
        act("$n hands $N some gold pieces.", FALSE, dealer, 0, ch, TO_NOTVICT);
        sprintf(buf, "$n hands you %d gold pieces.", (bet*5));
        act(buf, FALSE, dealer, 0, ch, TO_VICT);
        GET_GOLD(ch) += (bet*5);
     } else {
        /* player loses */
        act("$n says, 'Sorry $N, you lose.'", FALSE, dealer, 0, ch, TO_ROOM);
        act("$n takes $N's bet from the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
        act("$n takes your bet from the table.", FALSE, dealer, 0, ch, TO_VICT);
     }
     return;
}
 
SPECIAL(seven){
 struct char_data *dealr = (struct char_data *) me;
 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
   send_to_char("To play look rules or look croupier.\r\n", ch);
   return 1;
 }
 
 if (CMD_IS("bet")){
        two_arguments(argument, buf, arg);
        if(!*buf){
          send_to_char("Syntax is:- bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        if(!*arg){
   send_to_char("You need to read the rules, look rules or look croupier.\r\n", ch);
          return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Must place a valid bet of 1gp or more.!\r\n", ch);
          return 1;
        }
        play_seven(ch, dealr, arg, bet, 10, 10000);
	return 1;
}
return 0;
}

SPECIAL(seven1){
 struct char_data *dealr = (struct char_data *) me;
 int bet;
 
   if ((CMD_IS("kill") || CMD_IS("hit") || CMD_IS("cast") || CMD_IS("bash") || CMD_IS("kick") || CMD_IS("primal") || CMD_IS("backstab") || CMD_IS("steal"))){
     act("$n says, 'That kind of behaviour is not tollerated here!!'.", FALSE, dealr, 0 , 0 , TO_ROOM);
     act("$n shouts, 'GUARDS!!'.", FALSE, dealr, 0, 0 , TO_ROOM);
     act("Some mean looking Guards take $n by the hands and escort $m away from the table!", FALSE, ch, 0, 0 , TO_ROOM);
     char_from_room(ch);
     char_to_room(ch,real_room(13818));
     act("$n is roughly escorted out of the Gambling House by some mean looking guards!.",FALSE,ch,0,0,TO_ROOM);
     look_at_room(ch, 0);
     send_to_char("You are roughtly escorted out of the Gambling House!.\r\n", ch);
     return 1;
   }

 if (CMD_IS("play")){
   send_to_char("To play look rules or look croupier.\r\n", ch);
   return 1;
 }
 
 if (CMD_IS("bet")){
        two_arguments(argument, buf, arg);
        if(!*buf){
          send_to_char("Syntax is:- bet <Amount> i.e. bet 1000\r\n", ch);
        return 1;
        }
        if(!*arg){
   send_to_char("You need to read the rules, look rules or look croupier.\r\n", ch);
          return 1;
        }
        bet = atoi(buf);
        if (bet <= 0){
          send_to_char("Must place a valid bet of 1gp or more.!\r\n", ch);
          return 1;
        }
        play_seven(ch, dealr, arg, bet, 10000, 100000);
	return 1;
}
return 0;
}


/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

void play_slots(struct char_data *ch)
{
     int num1, num2, num3, win = 0;
     char *slot_msg[] = {
        "*YOU SHOULDN'T SEE THIS*",
        "a GOLDEN NUGGET",              /* 1 */
        "a PICK AXE",
        "a SPADE",               /* 5 */
        "a GOLD PAN",
     };
 
     if (GET_GOLD(ch) < 10) {
        send_to_char("You do not have enough money to play the slots!\r\n", ch);
        return;
     } else
        GET_GOLD(ch) -= 10;
        act("$n pulls on the crank of the 10gp One Arm Bandit.", FALSE,
        ch, 0, 0, TO_ROOM);
     send_to_char("You pull on the crank of the 10gp One Arm Bandit.\r\n", ch);
 
     /* very simple roll 3 random numbers from 1 to 4 */
     num1 = number(1, 4);
     num2 = number(1, 4);
     num3 = number(1, 4);
 if (num1 == num2 && num2 == num3) {    
        /* all 3 are equal! Woohoo! */
        if (num1 == 1)
           win += 300;
        else if (num1 == 2)
           win += 200;
        else if (num1 == 3)
           win += 150;
        else if (num1 == 4)
           win += 100;
        else if (num1 == 5)
           win += 50;
     }
 
     sprintf(buf, "You got %s, %s, %s, ", slot_msg[num1],
             slot_msg[num2], slot_msg[num3]);
     if (win > 1)
        sprintf(buf, "%syou win %d gold pieces!\r\n", buf, win);
     else if (win == 1)     
        sprintf(buf, "%syou win 1 gold piece!\r\n", buf);
     else
        sprintf(buf, "%syou lose.\r\n", buf);
     send_to_char(buf, ch);
     GET_GOLD(ch) += win;
 
     return;
}


SPECIAL(pokies)
{
  extern void play_slots(struct char_data *ch);

     if(CMD_IS("play")){
        play_slots(ch);
        return 1;
     }
    return 0;
}

void play_slots1(struct char_data *ch)
{
     int num1, num2, num3, win = 0;
     char *slot_msg[] = {
        "*YOU SHOULDN'T SEE THIS*",
        "a PAINTED INDIAN FACE",              /* 1 */
        "a TEE-PEE",
        "a TOMAHAWKE",               /* 5 */
        "a FEATHER HAT",
     };
 
     if (GET_GOLD(ch) < 100) {
        send_to_char("You do not have enough money to play the slots!\r\n", ch);
        return;
     } else
        GET_GOLD(ch) -= 100;
        act("$n pulls on the crank of the 100gp One Arm Bandit.", FALSE,
        ch, 0, 0, TO_ROOM);
     send_to_char("You pull on the crank of the 100gp  One Arm Bandit.\r\n", ch);
 
     /* very simple roll 3 random numbers from 1 to 4 */
     num1 = number(1, 4);
     num2 = number(1, 4);
     num3 = number(1, 4);
 if (num1 == num2 && num2 == num3) {    
        /* all 3 are equal! Woohoo! */
        if (num1 == 1)
           win += 3000;
        else if (num1 == 2)
           win += 2000;
        else if (num1 == 3)
           win += 1500;
        else if (num1 == 4)
           win += 1000;
        else if (num1 == 5)
           win += 500;
     }
 
     sprintf(buf, "You got %s, %s, %s, ", slot_msg[num1],
             slot_msg[num2], slot_msg[num3]);
     if (win > 1)
        sprintf(buf, "%syou win %d gold pieces!\r\n", buf, win);
     else if (win == 1)     
        sprintf(buf, "%syou win 1 gold piece!\r\n", buf);
     else
        sprintf(buf, "%syou lose.\r\n", buf);
     send_to_char(buf, ch);
     GET_GOLD(ch) += win;
 
     return;
}

SPECIAL(pokies1)
{
  extern void play_slots1(struct char_data *ch);

     if(CMD_IS("play")){
        play_slots1(ch);
	return 1;
     }
    return 0;
}


void play_slots2(struct char_data *ch)
{
     int num1, num2, num3, win = 0;
     char *slot_msg[] = {
        "*YOU SHOULDN'T SEE THIS*",
        "an EMPTY SALOON",              /* 1 */
        "a TUMBLEWEED",
        "a WHITE GHOST",               /* 5 */
        "a DESERTED TOWN SCENE",
     };
 
     if (GET_GOLD(ch) < 1000) {
        send_to_char("You do not have enough money to play the slots!\r\n", ch);
        return;
     } else
        GET_GOLD(ch) -= 1000;
        act("$n pulls on the crank of the 1000gp One Arm Bandit.", FALSE,
        ch, 0, 0, TO_ROOM);
     send_to_char("You pull on the crank of the 1000gp One Arm Bandit.\r\n", ch);
 
     /* very simple roll 3 random numbers from 1 to 4 */
     num1 = number(1, 4);
     num2 = number(1, 4);
     num3 = number(1, 4);
 if (num1 == num2 && num2 == num3) {    
        /* all 3 are equal! Woohoo! */
        if (num1 == 1)
           win += 30000;
        else if (num1 == 2)
           win += 20000;
        else if (num1 == 3)
           win += 15000;
        else if (num1 == 4)
           win += 10000;
        else if (num1 == 5)
           win += 5000;
     }
 
     sprintf(buf, "You got %s, %s, %s, ", slot_msg[num1],
             slot_msg[num2], slot_msg[num3]);
     if (win > 1)
        sprintf(buf, "%syou win %d gold pieces!\r\n", buf, win);
     else if (win == 1)     
        sprintf(buf, "%syou win 1 gold piece!\r\n", buf);
     else
        sprintf(buf, "%syou lose.\r\n", buf);
     send_to_char(buf, ch);
     GET_GOLD(ch) += win;
 
     return;
}

SPECIAL(pokies2)
{
  extern void play_slots2(struct char_data *ch);

     if(CMD_IS("play")){
        play_slots2(ch);
	return 1;
     }
    return 0;
}


void play_slots3(struct char_data *ch)
{
     int num1, num2, num3, win = 0;
     char *slot_msg[] = {
        "*YOU SHOULDN'T SEE THIS*",
        "a PRANCING HORSE",              /* 1 */
        "a HORSE SILHOUETTE",
        "a HORSESHOE",               /* 5 */
        "a COWBOY",
     };
 
     if (GET_GOLD(ch) < 10000) {
        send_to_char("You do not have enough money to play the slots!\r\n", ch);
        return;
     } else
        GET_GOLD(ch) -= 10000;
        act("$n pulls on the crank of the 10000gp One Arm Bandit.", FALSE,
        ch, 0, 0, TO_ROOM);
     send_to_char("You pull on the crank of the 10000gp One Arm Bandit.\r\n", ch);
 
     /* very simple roll 3 random numbers from 1 to 4 */
     num1 = number(1, 4);
     num2 = number(1, 4);
     num3 = number(1, 4);
 if (num1 == num2 && num2 == num3) {    
        /* all 3 are equal! Woohoo! */
        if (num1 == 1)
           win += 300000;
        else if (num1 == 2)
           win += 200000;
        else if (num1 == 3)
           win += 150000;
        else if (num1 == 4)
           win += 100000;
        else if (num1 == 5)
           win += 50000;
     }
 
     sprintf(buf, "You got %s, %s, %s, ", slot_msg[num1],
             slot_msg[num2], slot_msg[num3]);
     if (win > 1)
        sprintf(buf, "%syou win %d gold pieces!\r\n", buf, win);
     else if (win == 1)     
        sprintf(buf, "%syou win 1 gold piece!\r\n", buf);
     else
        sprintf(buf, "%syou lose.\r\n", buf);
     send_to_char(buf, ch);
     GET_GOLD(ch) += win;
 
     return;
}

SPECIAL(pokies3)
{
  extern void play_slots3(struct char_data *ch);

     if(CMD_IS("play")){
        play_slots3(ch);
        return 1;
     }
    return 0;
}

void big_wheel(struct char_data *ch, char *guess, int bet)
{
/*
     if (GET_GOLD(ch) < bet) {
        act("$n says, 'I'm sorry sir but you don't have that much gold.'",
            FALSE, dealer, 0, ch, TO_VICT);
        return;
     } else if (bet > 5000) {
        act("$n says, 'I'm sorry but the limit is 5000 gold pieces.'",
              FALSE, dealer, 0, ch, TO_VICT);
 	return;
     } else {
        GET_GOLD(ch) -= bet;
     act("$N places $S bet on the table.", FALSE, dealer, 0, ch, TO_NOTVICT);
     act("You place your bet on the table.", FALSE, ch, 0, 0, TO_CHAR);
     }
 
     if (!*guess) {
        act("$n tells you, 'You need to specify upper, lower, or triple.'",
            FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);
        GET_GOLD(ch) += bet;
        return;
     }
 
     if (!(!strcmp(guess, "2") || \
           !strcmp(guess, "5") || \
           !strcmp(guess, "11") || \
           !strcmp(guess, "23") || \
           !strcmp(guess, "joker") || \
           !strcmp(guess, "golden"))) {
        act("$n tells you, 'You need to read rules, look croupier or look rules'",
            FALSE, dealer, 0, ch, TO_VICT);
        act("$n hands your bet back to you.", FALSE, dealer, 0, ch, TO_VICT);
 
        GET_GOLD(ch) += bet;
        return;
     }

      switch(wheel)
	{
	case 1:
	result = 2;
	break;
	case 2:
	break;
	case 3:
	break;
        case 4:
	break;
        case 5:
	break;
        case 6:
	break;
        case 7:
	break;
        case 8:
	break;
        case 9:
	break;
 	case 10:
	break;
	case 11:
	break;
	case 12:
	break;
	case 13:
	break;
        case 14:
	break;
        case 15:
	break;
        case 16:
	break;
        case 17:
	break;
        case 18:
	break;
        case 19:
	break;
 	case 20:
	break;
	case 21:
	break;
	case 22:
	break;
	case 23:
	break;
        case 24:
	break;
        case 25:
	break;
        case 26:
	break;
        case 27:
	break;
        case 28:
	break;
        case 29:
	break;
 	case 30:
	break;
	case 31:
	break;
	case 32:
	break;
	case 33:
	break;
        case 34:
	break;
        case 35:
	break;
        case 36:
	break;
        case 37:
	break;
        case 38:
	break;
        case 39:
	break;
 	case 40:
	break;
	case 41:
	break;
	case 42:
	break;
	case 43:
	break;
        case 44:
	break;
        case 45:
	break;
        case 46:
	break;
        case 47:
	break;
        case 48:
	break;
        case 49:
	result = joker;
	break;
 	case 50:
	result = golden;
	break;
	}
    if (result = 2 && !strcmp(guess, "2")) {
          send_to_char("Congratulations you win!!!\r\n", ch);
          win = bet * 35;
          sprintf(buf,"$n wins %d gold coins!!", win);
          act(buf, FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
          send_to_char(buf, ch);
          GET_GOLD(ch) += win;
          return;
       }else
        send_to_char("Sorry you lose!\r\n", ch);
        return;
     }
   
    if (result = 5 && !strcmp(guess, "5")) {
          send_to_char("Congratulations you win!!!\r\n", ch);
          win = bet * 35;
          sprintf(buf,"$n wins %d gold coins!!", win);
          act(buf, FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
          send_to_char(buf, ch);
          GET_GOLD(ch) += win;
          return;
       }else
        send_to_char("Sorry you lose!\r\n", ch);
        return;
     }

    if (result = 11 && !strcmp(guess, "11")) {
          send_to_char("Congratulations you win!!!\r\n", ch);
          win = bet * 35;
          sprintf(buf,"$n wins %d gold coins!!", win);
          act(buf, FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
          send_to_char(buf, ch);
          GET_GOLD(ch) += win;
          return;
       }else
        send_to_char("Sorry you lose!\r\n", ch);
        return;
     }

    if (result = 23 && !strcmp(guess,"23")) {
          send_to_char("Congratulations you win!!!\r\n", ch);
          win = bet * 35;
          sprintf(buf,"$n wins %d gold coins!!", win);
          act(buf, FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
          send_to_char(buf, ch);
          GET_GOLD(ch) += win;
          return;
       }else
        send_to_char("Sorry you lose!\r\n", ch);
        return;
     }

    if (result = joker && !strcmp(guess, "joker")) {
          send_to_char("Congratulations you win!!!\r\n", ch);
          win = bet * 35;
          sprintf(buf,"$n wins %d gold coins!!", win);
          act(buf, FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
          send_to_char(buf, ch);
          GET_GOLD(ch) += win;
          return;
       }else
        send_to_char("Sorry you lose!\r\n", ch);
        return;
     }


    if (result = golden && !strcmp(guess,"golden")) {
          send_to_char("Congratulations you win!!!\r\n", ch);
          win = bet * 35;
          sprintf(buf,"$n wins %d gold coins!!", win);
          act(buf, FALSE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "%s hands you %d gold coins.\r\n", GET_NAME(dealer), win);
          send_to_char(buf, ch);
          GET_GOLD(ch) += win;
          return;
       }else
        send_to_char("Sorry you lose!\r\n", ch);
        return;
     }


*/
return;
}

