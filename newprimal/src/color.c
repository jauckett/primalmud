/* ************************************************************************
*   File: colour.c                                      Part of CircleMUD *
*  Usage: interprets inline colour codes                                  *
*  Name: Easy Colour v2.2                                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*  Modifications Copyright Trevor Man 1997                                *
*  Based on the Easy Color patch by mud@proimages.proimages.com           *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "color.h"

const char *COLOURLIST[] = {CNRM, CRED, CGRN, CYEL, CBLU, CMAG, CCYN, CWHT,
			    BRED, BGRN, BYEL, BBLU, BMAG, BCYN, BWHT,
			    BKRED, BKGRN, BKYEL, BKBLU, BKMAG, BKCYN, BKWHT,
			    CAMP, CSLH, BKBLK, CBLK, CFSH, CRVS, CUDL, BBLK };

int isnum(char s)
{
  return( (s>='0') && (s<='9') );
}

/* The is_colour function is used in a variety of ways: 
* The first is to check whether or not code is a number or letter code 
*	(colour_code_only == FALSE, ch == NULL)
*	Used: when paging (checking)
* The second is to check whether or not code is a letter only code
*	(colour_only_code == TRUE, ch == NULL) 
*	Used: when setting colours (choosing)
* The third is to return the value for the colour code 
*	(colour_code_only == FALSE, ch != NULL)
*	Used: proc_color (outputting)
*/
 
int is_colour(struct char_data *ch, char code, bool colour_code_only)
{
  switch (code) {
  /* Normal colours */
  case  'k': return 25; break;	/* Black */
  case  'r': return 1;	break;	/* Red */
  case  'g': return 2;	break;	/* Green */
  case  'y': return 3;	break;	/* Yellow */
  case  'b': return 4;	break;	/* Blue */
  case  'm': return 5;	break;	/* Magenta */
  case  'c': return 6;	break;	/* Cyan */
  case  'w': return 7;	break;	/* White */

  /* Bold colours */
  case  'K': return 29; break;  /* Bold black (Just for completeness) */
  case  'R': return 8;	break;	/* Bold red */
  case  'G': return 9;	break;	/* Bold green */
  case  'Y': return 10;	break;	/* Bold yellow */
  case  'B': return 11;	break;	/* Bold blue */
  case  'M': return 12;	break;	/* Bold magenta */
  case  'C': return 13;	break;	/* Bold cyan */
  case  'W': return 14;	break;	/* Bold white */
  
  /* Special codes */
  case  'n': return 0;	break;	/* Normal */
  case  'f': return 26;	break;	/* Flash */
  case  'v': return 27; break;	/* Reverse video */
  case  'u': return 28; break;	/* Underline (Only for mono screens) */

  default:   

    // Checking to see if it is a code - return TRUE or FALSE
    if (!colour_code_only && ch == NULL) {
      switch(code) {
        case '0': 
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
	case '9':
          return TRUE;
  
        break;

        default:
          return FALSE;
        break;
      }
    }

    // Outputting - return the colour value
    if (!colour_code_only && ch) {
      switch (code) {
        case  '0': return GET_COLOUR(ch,0);	break; 	
        case  '1': return GET_COLOUR(ch,1);	break;	
        case  '2': return GET_COLOUR(ch,2);	break;	
        case  '3': return GET_COLOUR(ch,3);	break;	
        case  '4': return GET_COLOUR(ch,4);	break;	
        case  '5': return GET_COLOUR(ch,5);	break;	
        case  '6': return GET_COLOUR(ch,6);	break;	
        case  '7': return GET_COLOUR(ch,7);	break;	
        case  '8': return GET_COLOUR(ch,8);	break;	
        case  '9': return GET_COLOUR(ch,9);	break;	

        /* Misc characters */
        case  '&': return 22;	break;	/* The & character */
        case '\\': return 23;	break;	/* The \ character */

        default:   return 0;			break;
      }
    } 
  break;

  }
  return -1;
}

void proc_color(char *inbuf, struct char_data *ch, int colour)
{
  register int j = 0, p = 0;
  int k, max, c = 0;
  char out_buf[32768];

  if (inbuf[0] == '\0')
    return;

  while (inbuf[j] != '\0') {
    if ((inbuf[j]=='\\') && (inbuf[j+1]=='c')
        && isnum(inbuf[j + 2]) && isnum(inbuf[j + 3])) {
      c = (inbuf[j + 2] - '0')*10 + inbuf[j + 3]-'0';
      j += 4;
    } else if ((inbuf[j] == '&') && !(is_colour(ch, inbuf[j + 1], FALSE) == -1)) {
      c = is_colour(ch, inbuf[j + 1], FALSE);
      j += 2;
    } else {
      out_buf[p] = inbuf[j];
      j++;
      p++;
      continue;
    }
    if (c > MAX_COLORS)
      c = 0;
    max = strlen(COLOURLIST[c]);
    if (colour || max == 1)
      for (k = 0; k < max; k++) {
        out_buf[p] = COLOURLIST[c][k];
	p++;
      }
  }

  out_buf[p] = '\0';

  strcpy(inbuf, out_buf);
}
