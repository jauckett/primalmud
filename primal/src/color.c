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

#include "color.h"

const char *COLOURLIST[] = {CNRM, CRED, CGRN, CYEL, CBLU, CMAG, CCYN, CWHT,
                            BRED, BGRN, BYEL, BBLU, BMAG, BCYN, BWHT,
                            BKRED, BKGRN, BKYEL, BKBLU, BKMAG, BKCYN, BKWHT,
                            CAMP, CSLH, BKBLK, CBLK, CFSH, CRVS, CUDL, BBLK };

int isnum(char s)
{
  return( (s>='0') && (s<='9') );
}

int is_colour(char code)
{
  switch (code) {
  /* Normal colours */
//  case  'k': return 25; break;	/* Black */
  case  'r': return 1;	break;	/* Red */
  case  'g': return 2;	break;	/* Green */
  case  'y': return 3;	break;	/* Yellow */
  case  'b': return 4;	break;	/* Blue */
  case  'm': return 5;	break;	/* Magenta */
  case  'c': return 6;	break;	/* Cyan */
  case  'w': return 7;	break;	/* White */

  /* Bold colours */
//  case  'K': return 29; break;  /* Bold black (Just for completeness) */
  case  'R': return 8;	break;	/* Bold red */
  case  'G': return 9;	break;	/* Bold green */
  case  'Y': return 10;	break;	/* Bold yellow */
  case  'B': return 11;	break;	/* Bold blue */
  case  'M': return 12;	break;	/* Bold magenta */
  case  'C': return 13;	break;	/* Bold cyan */
  case  'W': return 14;	break;	/* Bold white */
  
  /* Background colours */
//  case  '0': return 24;	break; 	/* Black background */
//  case  '1': return 15;	break;	/* Red background */
//  case  '2': return 16;	break;	/* Green background */
//  case  '3': return 17;	break;	/* Yellow background */
//  case  '4': return 18;	break;	/* Blue background */
//  case  '5': return 19;	break;	/* Magenta background */
//  case  '6': return 20;	break;	/* Cyan background */
//  case  '7': return 21;	break;	/* White background */

  /* Misc characters */
  case  '&': return 22;	break;	/* The & character */
  case '\\': return 23;	break;	/* The \ character */
  
  /* Special codes */
  case  'n': return 0;	break;	/* Normal */
  case  'f': return -1;	break;	/* Artus> Was: Flash */
  case  '@': return 26; break;  /* Artus> New Flash. */
  case  'v': return 27; break;	/* Reverse video */
  case  'u': return 28; break;	/* Underline (Only for mono screens) */

  default:   return -1;	break;
  }
  return -1;
}

void proc_color(char *inbuf, int colour)
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
    } else if ((inbuf[j] == '&') && !(is_colour(inbuf[j + 1]) == -1)) {
      c = is_colour(inbuf[j + 1]);
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
