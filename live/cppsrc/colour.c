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
#include "comm.h"
#include "constants.h"
#include "colour.h"

const char *COLOURLIST[] = {CNRM, CRED, CGRN, CYEL, CBLU, CMAG, CCYN, CWHT,
			    BRED, BGRN, BYEL, BBLU, BMAG, BCYN, BWHT,
			    BKRED, BKGRN, BKYEL, BKBLU, BKMAG, BKCYN, BKWHT,
			    CAMP, CSLH, BKBLK, CBLK, CFSH, CRVS, CUDL, BBLK };

int isnum(char s)
{
  return( (s>='0') && (s<='9') );
}

/*
 * Returns the index position in the string txt at which the first displayable
 * character to the screen exists. (Skips over colour codes)
 */
int first_disp_char(const char *txt)
{
  int k = 0;

  for (int j = strlen(txt); j > 1 && k < j; k++) {
    // C = colour code | 'n'
    // (&C)& - only worry if starting with a colour code ...
    if (txt[k] == '&') {
      // (&C)&C
      if (k + 1 < j && (is_colour(NULL, txt[k+1], FALSE) || txt[k+1] == 'n')) {
        k++;
      // (&C)& - reached end of string 
      } else if (k + 1 >= j) {
        return k;
      // (&C)&[&|\]	
      } else if (txt[k+1] == '&' || txt[k+1] == '\\') {
        return k+1;
      } 
    } else {
      return k;
    }
  }
  return k;
}

#if 0
// returns string, truncated to maxlen display characters.
char *strdispmax(char *source, char *dest, int maxlen)
{
  int i = 0, displen = 0;
  bool spec_code = FALSE;
  if (maxlen <= 0)
    return (strcpy(dest, ""));
  if ((int)strlen(input) < maxlen)
    return (strcpy(dest, source));
  strcpy(dest, input);
  for (; (displen <= maxlen); i++)
  {
    /* If end of string, return the number of displayed characters */
    if (dest[i] == '\0')
      return (dest);
    /* Check for the begining of an ANSI color code block. */
    else if (dest[i] == '\x1B')
      spec_code = TRUE;
    /* Check for the end of an ANSI color code block. */
    else if (dest[i] == 'm' && spec_code)
      spec_code = FALSE;
    /* Check for everything else. */
    else if (!spec_code)
    {
      if ((dest[i] == '&') && (dest[i+1] != '\0'))
      {
        if (dest("nmbBcCgGkKmMrRwWyYaA0123456789@[]|", dest[i+1]) != NULL)
	{
	  i++;
	  continue;
	} else if (dest[i+1] == '&' || dest[i+1] == '\\') {
          displen++;
	  i++;
	  continue;
	}
      }
      displen++;
    }
  }
  dest[i+1] = '\0';
  return dest;
}
#endif

// returns the number of characters which will be displayed of the string on 
// the screen (taking the colour codes out) ...
int strdisplen(const char *string)
{
  int number = 0, i = 0;
  char str[MAX_STRING_LENGTH];
  bool spec_code = FALSE;

  if (string == NULL)
    return 0;
  strcpy(str, string);
  for (;; i++)
  {
    /* If end of string, return the number of displayed characters */
    if (str[i] == '\0')
      return (number);
    /* Check for the begining of an ANSI color code block. */
    else if (str[i] == '\x1B' && !spec_code)
      spec_code = TRUE;
    /* Check for the end of an ANSI color code block. */
    else if (str[i] == 'm' && spec_code)
      spec_code = FALSE;
    /* Check for everything else. */
    else if (!spec_code)
    {
      // DM - easy colour codes
      // '&'
      if ((str[i+1] != '\0') && str[i] == '&')
      {
	// "&<colour code>", "&<number code>"
	// (is_colour returns 0 for n)
        if (str[i+1] == 'n' || is_colour(NULL, str[i+1], FALSE))
	{
	  i++;
	  continue;
	// "&&", "&\"
	} else if (str[i+1] == '&' || str[i+1] == '\\') {
          number++;
	  i++;
	  continue;
	}
      }
      /* Carriage return without newline puts us in column one. */
      // reset number as previous test is written over?
      if ((str[i] == '\r') && (str[i+1] != '\n'))
      {
	number = 0;
        continue;
      }
      if (str[i] == '\n' || str[i] == '\r')
        continue;
      number++;
    }
  }
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
  switch (code)
  {
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
    case  'f': return -1;       break;  /* Artus> Was: Flash */
    case  '@': return 26;       break;  /* Artus> New Flash. */
    case  'v': return 27;       break;	/* Reverse video */
    case  'u': return 28;       break;	/* Underline (Only for mono screens) */

    case CPREV_CODE: 
      if (!colour_code_only && ch == NULL) return TRUE;
      if (ch == NULL) return FALSE;
      if (GET_PREV_COLOUR(ch) >= 0 && GET_PREV_COLOUR(ch) < MAX_COLORS)
	return GET_PREV_COLOUR(ch); 
      else
	return 0; // CNRM
      break;

    case CMARK_PLACE: 
      if (!colour_code_only && ch == NULL) return TRUE;
      if (ch == NULL) return 0;
      if (GET_MARK_COLOUR(ch) >= 0 && GET_MARK_COLOUR(ch) < MAX_COLORS)
	return GET_MARK_COLOUR(ch); 
      else
	return 0; // CNRM 
      break;

    case CMARK_CODE: 
      if (!colour_code_only && ch == NULL) return TRUE;
      if (ch == NULL) return 0;
      if (GET_LAST_COLOUR(ch) >= 0 && GET_LAST_COLOUR(ch) < MAX_COLORS)
	return GET_LAST_COLOUR(ch); 
      else
	return 0; // CNRM 
      break;


    default:   
      // Checking to see if it is a code - return TRUE or FALSE
      if (!colour_code_only && ch == NULL)
      {
	switch(code)
	{
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
	    return -1;
	  break;
	}
      }
      // Outputting - return the colour value
      if (!colour_code_only && ch)
      {
	switch (code)
	{
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
	  default:   return 0;		break;
	}
      } 
      break;
  }
  return -1;
}

void proc_color(char *inbuf, struct char_data *ch, int colour, int insize)
{
  register int j = 0, p = 0;
  int k, max, c = 0;
  char out_buf[32768];
  struct char_data *realch;
  bool prev_code, mark_code, mark_place;

  realch = (ch->desc->original) ? ch->desc->original : ch;

  if (inbuf[0] == '\0')
    return;

  while (inbuf[j] != '\0') {
    prev_code = FALSE;
    mark_code = FALSE;
    mark_place = FALSE;
    if ((inbuf[j]=='\\') && (inbuf[j+1]=='c')
        && isnum(inbuf[j + 2]) && isnum(inbuf[j + 3])) {
      c = (inbuf[j + 2] - '0')*10 + inbuf[j + 3]-'0';
      j += 4;
    } else if ((inbuf[j] == '&') && !(is_colour(realch, inbuf[j + 1], FALSE) == -1)) {
      c = is_colour(realch, inbuf[j + 1], FALSE);
      if (inbuf[j + 1] == CPREV_CODE) {
        prev_code = TRUE;
      } else if (inbuf[j + 1] == CMARK_CODE) {
        mark_code = TRUE;
	GET_MARK_COLOUR(ch) = GET_LAST_COLOUR(ch);
      } else if (inbuf[j + 1] == CMARK_PLACE) {
        mark_place = TRUE;
      }
      GET_PREV_COLOUR(ch) = GET_LAST_COLOUR(ch);
      GET_LAST_COLOUR(ch) = c;
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
    int maxt = strlen(COLOURLIST[0]);
    if (colour || max == 1 && !mark_code) {
      // Add equivilant of a &n before &| or &]
      if (prev_code || mark_place) {
        for (k = 0; k < maxt; k++) {
          out_buf[p] = COLOURLIST[0][k];
	  p++;
        }
      }
      for (k = 0; k < max; k++) {
        out_buf[p] = COLOURLIST[c][k];
	p++;
      }
    }
  }

  out_buf[p] = '\0';

  // Artus> Made strncpy, and setting last char to null..
  strncpy(inbuf, out_buf, insize-1);
  inbuf[insize] = '\0';
}

void set_default_colour(struct char_data *ch, int i)
{
  if (i < 0 || i > 9) {
    send_to_char("Some nuff-nuff coder didn't check your colour number.\r\n",ch);
    return;
  } else {
    GET_COLOUR(ch,i) = default_colour_codes[i];
  }
}
 
void set_colour(struct char_data *ch, int i, int colour_code)
{
  if ((i < 0 || i > 9) || (colour_code < 0 || colour_code > MAX_COLORS)) {
    send_to_char("Some nuff-nuff coder didn't check your colour code.\r\n",ch);
    return;
  } else {
    GET_COLOUR(ch,i) = colour_code;
  }
} 
