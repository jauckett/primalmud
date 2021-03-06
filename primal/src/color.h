#define CNRM  "\x1B[0;0m"
#define CBLK  "\x1B[0;30m"
#define CRED  "\x1B[0;31m"
#define CGRN  "\x1B[0;32m"
#define CYEL  "\x1B[0;33m"
#define CBLU  "\x1B[0;34m"
#define CMAG  "\x1B[0;35m"
#define CCYN  "\x1B[0;36m"
#define CWHT  "\x1B[0;37m"
#define CNUL  ""
 
#define BBLK  "\x1B[1;30m"
#define BRED  "\x1B[1;31m"
#define BGRN  "\x1B[1;32m"
#define BYEL  "\x1B[1;33m"
#define BBLU  "\x1B[1;34m"
#define BMAG  "\x1B[1;35m"
#define BCYN  "\x1B[1;36m"
#define BWHT  "\x1B[1;37m"

#define BKBLK  "\x1B[40m"
#define BKRED  "\x1B[41m"
#define BKGRN  "\x1B[42m"
#define BKYEL  "\x1B[43m"
#define BKBLU  "\x1B[44m"
#define BKMAG  "\x1B[45m"
#define BKCYN  "\x1B[46m"
#define BKWHT  "\x1B[47m"
 
#define CAMP  "&"
#define CSLH  "\\"
 
#define CUDL  "\x1B[4m" /* Underline ANSI code */
#define CFSH  "\x1B[5m" /* Flashing ANSI code.  Change to #define CFSH "" if
                         * you want to disable flashing colour codes
                         */
#define CRVS  "\x1B[7m" /* Reverse video ANSI code */

#define MAX_COLORS 30
 
int is_colour(char code);
