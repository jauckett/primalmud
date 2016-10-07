/* ************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <fstream>
#include <list>

#define __COMM_C__
#include "sys/socket.h"
#include "sys/types.h"
#include "conf.h"
#include "screen.h"
#include "sysdep.h"

#ifdef CIRCLE_MACINTOSH		/* Includes for the Macintosh */
# define SIGPIPE 13
# define SIGALRM 14
  /* GUSI headers */
# include <sys/ioctl.h>
  /* Codewarrior dependant */
# include <SIOUX.h>
# include <console.h>
#endif

#ifdef CIRCLE_WINDOWS		/* Includes for Win32 */
# ifdef __BORLANDC__
#  include <dir.h>
# else /* MSVC */
#  include <direct.h>
# endif
# include <mmsystem.h>
#endif /* CIRCLE_WINDOWS */

#ifdef CIRCLE_AMIGA		/* Includes for the Amiga */
# include <sys/ioctl.h>
# include <clib/socket_protos.h>
#endif /* CIRCLE_AMIGA */

#ifdef CIRCLE_ACORN		/* Includes for the Acorn (RiscOS) */
# include <socklib.h>
# include <inetlib.h>
# include <sys/ioctl.h>
#endif

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "genolc.h"
#include "oasis.h"
#include "screen.h"
#include "casino.h"
#include "reports.h"
#include "dg_scripts.h"
#include "spells.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* externs */
extern struct ban_list_element *ban_list;
extern int num_invalid;
extern char *GREETINGS;
extern const char *circlemud_version;
extern int circle_restrict;
extern int mini_mud;
extern int no_rent_check;
extern FILE *player_fl;
extern ush_int DFLT_PORT;
extern const char *DFLT_DIR;
extern const char *DFLT_IP;
extern const char *LOGNAME;
extern int max_playing;
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */
int level_exp(struct char_data *ch, int level);
extern Burglary *burglaries;

extern struct room_data *world;	/* In db.c */
extern struct time_info_data time_info;		/* In db.c */
extern char *help;
extern GameInfo gameInfo;

/* local globals */
struct descriptor_data *descriptor_list = NULL;		/* master desc list */
struct user_data *user_list = NULL; /* master user list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
bool MOBTrigger = TRUE;         /* for MOBProgs */
struct timeval null_time;	/* zero-valued time structure */
FILE *logfile = NULL;		/* Where to send the log messages. */
int dg_act_check;              /* toggle for act_trigger */
unsigned long dg_global_pulse = 0; /* number of pulses since game start */


/* functions in this file */
RETSIGTYPE reread_wizlists(int sig);
RETSIGTYPE unrestrict_game(int sig);
RETSIGTYPE reap(int sig);
RETSIGTYPE checkpointing(int sig);
RETSIGTYPE hupsig(int sig);
ssize_t perform_socket_read(socket_t desc, char *read_point,size_t space_left);
ssize_t perform_socket_write(socket_t desc, const char *txt,size_t length);
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void sanity_check(void);
void circle_sleep(struct timeval *timeout);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(ush_int port);
void signal_setup(void);
void game_loop(socket_t mother_desc);
socket_t init_socket(ush_int port);
int new_descriptor(socket_t s);
int get_max_players(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);
void timeadd(struct timeval *sum, struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(socket_t s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
char *make_prompt(struct descriptor_data *point);
void check_idle_passwords(void);
void heartbeat(int pulse);
struct in_addr *get_bind_addr(void);
int parse_ip(const char *addr, struct in_addr *inaddr);
int set_sendbuf(socket_t s);
void setup_log(const char *filename, int fd);
int open_logfile(const char *filename, FILE *stderr_fp);
#if defined(POSIX)
sigfunc *my_signal(int signo, sigfunc * func);
#endif
char *get_user_prompt_string(struct char_data *ch);
char *get_exp_prompt_string(struct char_data *ch);
char *get_align_prompt_string(struct char_data *ch); 
char *get_point_prompt_string(int curr, int max, bool max_point);
char *get_fight_prompt(struct char_data *ch);

/* extern fcnts */
void handle_events();
void proc_color(char *inbuf, struct char_data *ch, int colour, int insize);
void reboot_wizlists(void);
void boot_world(void);
void affect_update(void);	/* In spells.c */
void mobile_activity(void);
void perform_violence(void);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname);
int isipbanned(struct in_addr inaddr, const int nameserver_is_slow);
void weather_and_time(int mode);
void rotate_arms();
void do_hint();
void do_zone_hint();
void move_ship();
void move_falcon();
void move_bus();

#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define FD_ZERO(x)
#define FD_SET(x, y) 0
#define FD_ISSET(x, y) 0
#define FD_CLR(x, y)
#endif


/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

#if defined(CIRCLE_WINDOWS) || defined(CIRCLE_MACINTOSH)

/*
 * Windows doesn't have gettimeofday, so we'll simulate it.
 * The Mac doesn't have gettimeofday either.
 * Borland C++ warns: "Undefined structure 'timezone'"
 */
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
#if defined(CIRCLE_WINDOWS)
  DWORD millisec = GetTickCount();
#elif defined(CIRCLE_MACINTOSH)
  unsigned long int millisec;
  millisec = (int)((float)TickCount() * 1000.0 / 60.0);
#endif

  t->tv_sec = (int) (millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}

#endif	/* CIRCLE_WINDOWS || CIRCLE_MACINTOSH */

// DM - log the process id
void pid_log(void) {
   FILE *pidlog;

   if (!(pidlog = fopen("circle.pid", "w"))) {
     basic_mud_log("SYSERR: Could not open pid file for writing"); 
   }

   fprintf(pidlog,"%d\n", getpid());
   fclose(pidlog);
}

#define plant_magic(x)	do { (x)[sizeof(x) - 1] = MAGIC_NUMBER; } while (0)
#define test_magic(x)	((x)[sizeof(x) - 1])

int main(int argc, char **argv)
{
  ush_int port;
  int pos = 1;
  const char *dir;

  /* Initialize these to check for overruns later. */
  plant_magic(buf);
  plant_magic(buf1);
  plant_magic(buf2);
  plant_magic(arg);

#ifdef CIRCLE_MACINTOSH
  /*
   * ccommand() calls the command line/io redirection dialog box from
   * Codewarriors's SIOUX library
   */
  argc = ccommand(&argv);
  /* Initialize the GUSI library calls.  */
  GUSIDefaultSetup();
#endif

  port = DFLT_PORT;
  dir = DFLT_DIR;

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'o':
      if (*(argv[pos] + 2))
	LOGNAME = argv[pos] + 2;
      else if (++pos < argc)
	LOGNAME = argv[pos];
      else {
	puts("SYSERR: File name to log to expected after option -o.");
	exit(1);
      }
      break;
    case 'd':
      if (*(argv[pos] + 2))
	dir = argv[pos] + 2;
      else if (++pos < argc)
	dir = argv[pos];
      else {
	puts("SYSERR: Directory arg expected after option -d.");
	exit(1);
      }
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      puts("Running in minimized mode & with no rent check.");
      break;
    case 'c':
      scheck = 1;
      puts("Syntax check mode enabled.");
      break;
    case 'q':
      no_rent_check = 1;
      puts("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      circle_restrict = 1;
      puts("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      puts("Suppressing assignment of special routines.");
      break;
    case 'h':
      /* From: Anil Mahajan <amahajan@proxicom.com> */
      printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n"
              "  -c             Enable syntax check mode.\n"
              "  -d <directory> Specify library directory (defaults to 'lib').\n"
              "  -h             Print this command line argument help.\n"
              "  -m             Start in mini-MUD mode.\n"
	      "  -o <file>      Write log to <file> instead of stderr.\n"
              "  -q             Quick boot (doesn't scan rent for object limits)\n"
              "  -r             Restrict MUD -- no new players allowed.\n"
              "  -s             Suppress special procedure assignments.\n",
		 argv[0]
      );
      exit(0);
    default:
      printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      printf("SYSERR: Illegal port number %d.\n", port);
      exit(1);
    }
  }

  /* All arguments have been parsed, try to open log file. */
  setup_log(LOGNAME, STDERR_FILENO);

  /*
   * Moved here to distinguish command line options and to show up
   * in the log if stderr is redirected to a file.
   */
  basic_mud_log(circlemud_version);
  basic_mud_log(DG_SCRIPT_VERSION);

  if (chdir(dir) < 0) {
    perror("SYSERR: Fatal error changing to data directory");
    exit(1);
  }
  basic_mud_log("Using %s as data directory.", dir);

  /* DM - log the process id */
  pid_log();

  if (scheck) {
    boot_world();
    basic_mud_log("Done.");
  } else {
    basic_mud_log("Running game on port %d.", port);
    init_game(port);
  }

  // Clean up any burglaries outstanding
  if (burglaries != NULL)
  {
	delete burglaries;
  }

  return (0);
}



/* Init sockets, run game, and cleanup sockets */
void init_game(ush_int port)
{
  socket_t mother_desc;

  /* We don't want to restart if we crash before we get up. */
  touch(KILLSCRIPT_FILE);

  circle_srandom(time(0));

  basic_mud_log("Finding player limit.");
  max_players = get_max_players();

  basic_mud_log("Opening mother connection.");
  mother_desc = init_socket(port);

  boot_db();

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)
  basic_mud_log("Signal trapping.");
  signal_setup();
#endif

  /* If we made it this far, we will be able to restart without problem. */
  remove(KILLSCRIPT_FILE);

  basic_mud_log("Entering game loop.");

  game_loop(mother_desc);

  Crash_save_all();
  House_save_all();

  basic_mud_log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  CLOSE_SOCKET(mother_desc);
  fclose(player_fl);

  if (circle_reboot != 2)
    save_all();
 
  if (circle_reboot) {
    basic_mud_log("Rebooting.");
    exit(52);			/* what's so great about HHGTTG, anyhow? */
  }
  basic_mud_log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
socket_t init_socket(ush_int port)
{
  socket_t s;
  struct sockaddr_in sa;
  int opt;

#ifdef CIRCLE_WINDOWS
  {
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      basic_mud_log("SYSERR: WinSock not available!");
      exit(1);
    }
    if ((wsaData.iMaxSockets - 4) < max_players) {
      max_players = wsaData.iMaxSockets - 4;
    }
    basic_mud_log("Max players set to %d", max_players);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      basic_mud_log("SYSERR: Error opening network connection: Winsock error #%d",
	  WSAGetLastError());
      exit(1);
    }
  }
#else
  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so the point is (hopefully) moot.
   */

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("SYSERR: Error creating socket");
    exit(1);
  }
#endif				/* CIRCLE_WINDOWS */

#if defined(SO_REUSEADDR) && !defined(CIRCLE_MACINTOSH)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
    perror("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }
#endif

  set_sendbuf(s);

/*
 * The GUSI sockets library is derived from BSD, so it defines
 * SO_LINGER, even though setsockopt() is unimplimented.
 *	(from Dean Takemori <dean@UHHEPH.PHYS.HAWAII.EDU>)
 */
#if defined(SO_LINGER) && !defined(CIRCLE_MACINTOSH)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
      perror("SYSERR: setsockopt SO_LINGER");	/* Not fatal I suppose. */
  }
#endif

  /* Clear the structure */
  memset((char *)&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr = *(get_bind_addr());

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    CLOSE_SOCKET(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return (s);
}


int get_max_players(void)
{
#ifndef CIRCLE_UNIX
  return (max_playing);
#else

  int max_descs = 0;
  const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
#ifdef HAS_RLIMIT
  {
    struct rlimit limit;

    /* find the limit of file descs */
    method = "rlimit";
    if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling getrlimit");
      exit(1);
    }

    /* set the current to the maximum */
    limit.rlim_cur = limit.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling setrlimit");
      exit(1);
    }
#ifdef RLIM_INFINITY
    if (limit.rlim_max == RLIM_INFINITY)
      max_descs = max_playing + NUM_RESERVED_DESCS;
    else
      max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#else
    max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
  }

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  method = "OPEN_MAX";
  max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
				 * OPEN_MAX */
#elif defined (_SC_OPEN_MAX)
  /*
   * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
   * try the POSIX sysconf() function.  (See Stevens' _Advanced Programming
   * in the UNIX Environment_).
   */
  method = "POSIX sysconf";
  errno = 0;
  if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
    if (errno == 0)
      max_descs = max_playing + NUM_RESERVED_DESCS;
    else {
      perror("SYSERR: Error calling sysconf");
      exit(1);
    }
  }
#else
  /* if everything has failed, we'll just take a guess */
  method = "random guess";
  max_descs = max_playing + NUM_RESERVED_DESCS;
#endif

  /* now calculate max _players_ based on max descs */
  max_descs = MIN(max_playing, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    basic_mud_log("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
	    max_descs, method);
    exit(1);
  }
  basic_mud_log("   Setting player limit to %d using %s.", max_descs, method);
  return (max_descs);
#endif /* CIRCLE_UNIX */
}

int pulse = 0;

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(socket_t mother_desc)
{
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, opt_time, process_time, temp_time;
  struct timeval before_sleep, now, timeout;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int /* pulse = 0,*/ missed_pulses, maxdesc, aliased;

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  FD_ZERO(&null_set);

  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) 
  {

    /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
      basic_mud_log("No connections.  Going to sleep.");
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
	if (errno == EINTR)
	  basic_mud_log("Waking up to process signal.");
	else
	  perror("SYSERR: Select coma");
      } else
	basic_mud_log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);

    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
            
#ifndef CIRCLE_WINDOWS
      if (d->descriptor > maxdesc)
	maxdesc = d->descriptor;
#endif
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, we have completed all input, output and heartbeat
     * activity from the previous iteration, so we have to put ourselves
     * to sleep until the next 0.1 second tick.  The first step is to
     * calculate how long we took processing the previous iteration.
     */
    
    gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
    timediff(&process_time, &before_sleep, &last_time);

    /*
     * If we were asleep for more than one pass, count missed pulses and sleep
     * until we're resynchronized with the next upcoming pulse.
     */
    if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
      missed_pulses = 0;
    } else {
      missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
      missed_pulses += process_time.tv_usec / OPT_USEC;
      process_time.tv_sec = 0;
      process_time.tv_usec = process_time.tv_usec % OPT_USEC;
    }

    /* Calculate the time we should wake up */
    timediff(&temp_time, &opt_time, &process_time);
    timeadd(&last_time, &before_sleep, &temp_time);

    /* Now keep sleeping until that time has come */
    gettimeofday(&now, (struct timezone *) 0);
    timediff(&timeout, &last_time, &now);

    /* Go to sleep */
    do {
      circle_sleep(&timeout);
      gettimeofday(&now, (struct timezone *) 0);
      timediff(&timeout, &last_time, &now);
    } while (timeout.tv_usec || timeout.tv_sec);

    /* Poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("SYSERR: Select poll");
      return;
    }
    /* If there are new connections waiting, accept them. */
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);

    /* Kick out the freaky folks in the exception set and marked for close */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
	FD_CLR(d->descriptor, &input_set);
	FD_CLR(d->descriptor, &output_set);
	close_socket(d);
      }
    }

    /* Process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
	if (process_input(d) < 0)
	  close_socket(d);
    }

    /* Process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      /*
       * Not combined to retain --(d->wait) behavior. -gg 2/20/98
       * If no wait state, no subtraction.  If there is a wait
       * state then 1 is subtracted. Therefore we don't go less
       * than 0 ever and don't require an 'if' bracket. -gg 2/27/99
       */
      if (d->character) {
        GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0);

        if (GET_WAIT_STATE(d->character))
          continue;
      }

      if (!get_from_q(&d->input, comm, &aliased))
        continue;

      if (d->character) {
	/* Reset the idle timer & pull char back from void if necessary */
	d->character->char_specials.timer = 0;
	if (STATE(d) == CON_PLAYING && GET_WAS_IN(d->character) != NOWHERE) {
	  if (d->character->in_room != NOWHERE)
	    char_from_room(d->character);
	  char_to_room(d->character, GET_WAS_IN(d->character));
	  GET_WAS_IN(d->character) = NOWHERE;
	  act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
	}
        GET_WAIT_STATE(d->character) = 1;
      }
      d->has_prompt = 0;

      if (d->showstr_count) /* Reading something w/ pager */
	show_string(d, comm);
      else if (d->str)         /* Writing boards, mail, etc. */
        string_add(d, comm);
      else if (STATE(d) != CON_PLAYING) /* In menus, etc. */
	nanny(d, comm);
      else {			/* else: we're playing normally. */
	if (aliased)		/* To prevent recursive aliases. */
	  d->has_prompt = 1;	/* To get newline before next cmd output. */
	else if (perform_alias(d, comm))    /* Run it through aliasing system */
	  get_from_q(&d->input, comm, &aliased);
	command_interpreter(d->character, comm); /* Send it to interpreter */
      }
    }

    /* Send queued output out to the operating system (ultimately to user). */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
        if ((d->has_prompt = process_output(d)) < 0)
          close_socket(d);
      }
   }   

    /* Print prompts for other descriptors who had no other output */
    for (d = descriptor_list; d; d = d->next) {
      if (!d->has_prompt) 
      {
	if (d->bufptr < 0)
	  write_to_descriptor(d->descriptor, "> ");
	else
	  write_to_descriptor(d->descriptor, make_prompt(d));
	d->has_prompt = 1;
      }
    }

    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
	close_socket(d);
    }

    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0) {
      basic_mud_log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
      missed_pulses = 1;
    }

    /* If we missed more than 30 seconds worth of pulses, just do 30 secs */
    if (missed_pulses > (30 * PASSES_PER_SEC)) {
      basic_mud_log("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
      missed_pulses = 30 * PASSES_PER_SEC;
    }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
      heartbeat(++pulse);

    /* Roll pulse over after 10 hours */
    if (pulse >= (600 * 60 * PASSES_PER_SEC))
      pulse = 0;

#ifdef CIRCLE_UNIX
    /* Update tics for deadlock protection (UNIX only) */
    tics++;
#endif
  }
}


void heartbeat(int pulse)
{
  static int mins_since_crashsave = 0;
  void process_events(void);
  void object_activity(void);
  void world_activity(void);
  void rapid_blasts(void);
  
  dg_global_pulse++;
  
  process_events();
  
  if (!(pulse % PULSE_DG_SCRIPT))
  {
    if (pulse % (SECS_PER_MUD_HOUR RL_SEC) > 4)
      object_activity();
      // script_trigger_check();
    world_activity();
  }

  if (!(pulse % (30 RL_SEC)))
    sanity_check();

  if (!(pulse % PULSE_ZONE))
    zone_update();

  if (!(pulse % (15 RL_SEC)))		/* 15 seconds */
    check_idle_passwords();

  if (!(pulse % PULSE_MOBILE))
  {
    handle_events();
    mobile_activity();
  }

  if (!(pulse % PULSE_VIOLENCE))
  {
    perform_violence();
    rapid_blasts(); // Artus> Gauntlet, Compound...
  }

  // Artus> Put a lot of stuff inside here, so it only checks 60 sec every
  //        15 sec, 120sec every 60 sec, 300 sec every 60 sec, etc.
  if (!(pulse % (PULSE_CASINO))) // Casino, Island Forever Spec. (15 Sec)
  {
    if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) // Tick! (45 Sec)
    {
      weather_and_time(1);
      affect_update();
      point_update();
      object_activity();
      fflush(player_fl);
    }
    check_games();      // Tali's gambling.
    rotate_arms();      // Tali's island forever spec.
    if (auto_save && !(pulse % (60 RL_SEC))) // Auto Save (60 Sec)
    {
      if (++mins_since_crashsave >= autosave_time)
      {
	mins_since_crashsave = 0;
	Crash_save_all();
	House_save_all();
      }
      if (!(pulse % (PULSE_ZONE_HINTS))) // Zone Hints (300 Sec)
      {
	if (!(pulse % (PULSE_HINTS)))    // Hints (600 Sec)
	  do_hint();
	do_zone_hint();
	record_usage();
      }
      if (!(pulse % (120 RL_SEC)))       // Move Ship/Falcon (120 Sec)
      {   
	move_ship();
	move_falcon();
      }
    }
  }

  if ((pulse % (35 RL_SEC)) == 13)
    move_bus();
}


/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
void timediff(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  if (a->tv_sec < b->tv_sec)
    *rslt = null_time;
  else if (a->tv_sec == b->tv_sec) {
    if (a->tv_usec < b->tv_usec)
      *rslt = null_time;
    else {
      rslt->tv_sec = 0;
      rslt->tv_usec = a->tv_usec - b->tv_usec;
    }
  } else {			/* a->tv_sec > b->tv_sec */
    rslt->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_usec < b->tv_usec) {
      rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
      rslt->tv_sec--;
    } else
      rslt->tv_usec = a->tv_usec - b->tv_usec;
  }
}

/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  rslt->tv_sec = a->tv_sec + b->tv_sec;
  rslt->tv_usec = a->tv_usec + b->tv_usec;

  while (rslt->tv_usec >= 1000000) {
    rslt->tv_usec -= 1000000;
    rslt->tv_sec++;
  }
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (STATE(d) == CON_PLAYING)
      sockets_playing++;
  }

  basic_mud_log("nusage: %-3d sockets connected, %-3d sockets playing, %-3d max sockets playing this period, %-3d max sockets playing",
	  sockets_connected, sockets_playing, gameInfo.getMaxUsersOnlinePeriod(), gameInfo.getMaxUsersOnline());
  gameInfo.setMaxUsersOnlinePeriod(sockets_playing);

#ifdef RUSAGE	/* Not RUSAGE_SELF because it doesn't guarantee prototype. */
  {
    struct rusage ru;

    getrusage(RUSAGE_SELF, &ru);
    basic_mud_log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
	    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
  }
#endif

}



/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) 0
  };

  SEND_TO_Q(on_string, d);
}

char *get_point_prompt_string(int curr, int max, bool max_point)
{
  static char point_prompt[MAX_PROMPT_LENGTH + 1];

  *point_prompt = '\0';

  if (!max_point) {
    if (curr < max / 4)
      strcpy(point_prompt, "&r");
    else if (curr < max / 2)
      strcpy(point_prompt, "&R");
    else if (curr < max)
      strcpy(point_prompt, "&Y");
    else 
      strcpy(point_prompt, "&G");
  } else {
    strcpy(point_prompt, "&G");
  }

  if (!max_point) {
    sprintf(point_prompt, "%s%d&n", point_prompt, curr); 
  } else {
    sprintf(point_prompt, "%s%d&n", point_prompt, max);
  }
  return (point_prompt);
}

char *get_align_prompt_string(struct char_data *ch) {
  static char align_prompt[MAX_PROMPT_LENGTH + 1];

  *align_prompt = '\0';

  if (GET_ALIGNMENT(ch) > 350)
    strcpy(align_prompt, "&c");
  else if (GET_ALIGNMENT(ch) < -350)
    strcpy(align_prompt, "&r");
  else
    strcpy(align_prompt, "&y");

  sprintf(align_prompt, "%s%d", align_prompt, GET_ALIGNMENT(ch));
  
  return (align_prompt); 
}

char *get_exp_prompt_string(struct char_data *ch) {
  static char exp_prompt[MAX_PROMPT_LENGTH + 1];
  double temp;

  *exp_prompt = '\0';

  if (!ch || IS_NPC(ch) || !CAN_LEVEL(ch))
  {
    sprintf(exp_prompt, "&R*");
    return (exp_prompt);
  }

  temp = (int)(level_exp(ch, GET_LEVEL(ch)) - GET_EXP(ch));

  if (temp >= 1000000) {
    sprintf(exp_prompt, "&c%3.1fm", temp/1000000);
  } else if (temp >= 1000) {
    sprintf(exp_prompt, "&c%3.1fk", temp/1000);
  } else {
    sprintf(exp_prompt, "&c%d", (int)temp);
  }

  return (exp_prompt);
}

char *get_user_prompt_string(struct char_data *ch) {
  char temp[25 + 1], **prompt = &GET_PROMPT(ch);
  static char user_prompt[MAX_PROMPT_LENGTH + 1];
  unsigned int i;

  *user_prompt = '\0';

  if (!ch || !*prompt) 
    return (user_prompt);
  
  strcpy(temp, *prompt);

  for (i = 0; i < strlen(temp); i++) {
    if (temp[i] == '%') {

      if (i++ >= strlen(temp)) {
        sprintf(buf, "%c", temp[i-1]);
        strcat(user_prompt, buf);
        continue;
      }

      switch (temp[i]) {
        case 'h':
          sprintf(buf, "%s", 
                get_point_prompt_string(GET_HIT(ch), GET_MAX_HIT(ch), FALSE));
          break;
        case 'H':
          sprintf(buf, "%s", 
                get_point_prompt_string(GET_HIT(ch), GET_MAX_HIT(ch), TRUE));
          break;
        case 'm':
          sprintf(buf, "%s", 
                get_point_prompt_string(GET_MANA(ch), GET_MAX_MANA(ch), FALSE));
          break;
        case 'M':
          sprintf(buf, "%s", 
                get_point_prompt_string(GET_MANA(ch), GET_MAX_MANA(ch), TRUE));
          break;
        case 'v':
          sprintf(buf, "%s", 
                get_point_prompt_string(GET_MOVE(ch), GET_MAX_MOVE(ch), FALSE));
          break;
        case 'V':
          sprintf(buf, "%s", 
                get_point_prompt_string(GET_MOVE(ch), GET_MAX_MOVE(ch), TRUE));
          break;
        case 'x':
          strcpy(buf, get_exp_prompt_string(ch));
          break;
        case 'l':
          sprintf(buf, get_align_prompt_string(ch)); 
          break;

        // Nothing
        default:
          sprintf(buf, "%c", temp[i - 1]);
          i--;
          break;
      }
      strcat(user_prompt, buf);
    } else {
      sprintf(buf, "%c", temp[i]);
      strcat(user_prompt, buf);
    }
  }
  return (user_prompt);
}

char *get_fight_prompt(struct char_data *ch) {
  
  float health;
  int bar, i, numslots;
  static char prompt[MAX_PROMPT_LENGTH + 1];
  
  *prompt = '\0';

  switch (GET_FIGHT_PROMPT(ch)) {
    case PROMPT_HEALTHOMETER:
    default:
      // modified by DM, 9/2/2001
      /* modifications to add the Health-o-Meter(tm) to the prompt - Vader */

      // min of 5 slots, max of 20.
      // players get extra slot every 5 levels after level 10 ...
      numslots = (int)(GET_LEVEL(ch) - 10) / 5 + 5;
      numslots = MIN(20, MAX(5, numslots));

      health = ((float)GET_HIT(FIGHTING(ch)) / 
                (float)GET_MAX_HIT(FIGHTING(ch))) * 100;
      bar = (int)(health / (100 / numslots));

      sprintf(buf1, "%s[%s", CCBLU(ch, C_NRM), CCGRN(ch, C_NRM));
      strncat(prompt, buf1, strlen(buf1));

      if (GET_HIT(FIGHTING(ch)) <= 0) {
        sprintf(buf1, "%s", CCRED(ch, C_NRM));
        strncat(prompt, buf1, strlen(buf1));
        for (i = 0; i < numslots; i++) {
          strncat(prompt, "-", 1);
        }
      } else {

        for (i = 0; i < bar && i < numslots; i++) {
          strncat(prompt,"*", 1);
        }

        if (bar < numslots) {
          sprintf(buf1, "%s", CCRED(ch, C_NRM));
          strncat(prompt, buf1, strlen(buf1));
          for(i = bar; i < numslots; i++) {
            strncat(prompt, "-", 1);
          }
        }
      }
      sprintf(buf1, "%s] ", CCBLU(ch, C_NRM));
      strncat(prompt, buf1, strlen(buf1));
      return (prompt);
  }
}

char *make_prompt(struct descriptor_data *d)
{
  static char prompt[MAX_PROMPT_LENGTH + 1];
  char **user_prompt = 
          (d->original) ? &GET_PROMPT(d->original) : &GET_PROMPT(d->character);

  /* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h )*/

  if (d->showstr_count) { 
    sprintf(prompt, "\r&n[ Return to continue, (&4q&n)uit, (&4r&n)efresh, "
                    "(&4b&n)ack, or page number (&c%d&n/&c%d&n) ]",
	    d->showstr_page, d->showstr_count);
    proc_color(prompt, d->character, (clr(d->character, C_NRM)), sizeof(prompt));
  } else if (d->str) {
    sprintf(prompt, "(&c%d&n/&C%d&n) &y]&n ", 
            (*d->str ? strlen(*d->str) : 0), d->max_str - 1);
    proc_color(prompt, d->character, (clr(d->character, C_NRM)), sizeof(prompt));
  } else if (STATE(d) == CON_PLAYING && !IS_NPC(d->character)) {
    *prompt = '\0';

    if (GET_INVIS_LEV(d->character)) {
      switch (GET_INVIS_TYPE(d->character)) {
        case INVIS_SPECIFIC:
          sprintf(buf, "i(%s) ", get_name_by_id(GET_INVIS_LEV(d->character)));
          break;
        case INVIS_SINGLE:
          sprintf(buf, "i%ds ", GET_INVIS_LEV(d->character));
          break;
        case INVIS_NORMAL:
          sprintf(buf,"i%d ", GET_INVIS_LEV(d->character));
          break;
        // Invis range, top range level is GET_INVIS_TYPE
        default:
          sprintf(buf, "i%d-%d ", GET_INVIS_LEV(d->character),
              GET_INVIS_TYPE(d->character));
          break;
      }
      strcat(prompt, buf);
    }

    if (*user_prompt != NULL) {
      sprintf(buf, "%s", get_user_prompt_string(d->character));
      strncat(prompt, buf, strlen(buf));
    } else {

/* modifications for cooly, frilly, golly, sticky prompties! - Vader */
      if (PRF_FLAGGED(d->character, PRF_DISPHP)) {
        sprintf(buf2, "%sH ", get_point_prompt_string(GET_HIT(d->character), 
                                GET_MAX_HIT(d->character), FALSE));
        strncat(prompt, buf2, strlen(buf2));
      }
 
      if (PRF_FLAGGED(d->character, PRF_DISPMANA)) {
        sprintf(buf2, "%sM ", get_point_prompt_string(GET_MANA(d->character), 
                                GET_MAX_MANA(d->character), FALSE));
        strncat(prompt, buf2, strlen(buf2));
      }

      if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) {
        sprintf(buf2, "%sV ", get_point_prompt_string(GET_MOVE(d->character), 
                                GET_MAX_MOVE(d->character), FALSE));
        strncat(prompt, buf2, strlen(buf2));
      }
 
      if (PRF_FLAGGED(d->character, PRF_DISPEXP)) {
        sprintf(buf1, "X:%s%s&n ", CCCYN(d->character, C_NRM), 
                get_exp_prompt_string(d->character)); 
        strncat(prompt, buf1, strlen(buf1));
      }

      if (PRF_FLAGGED(d->character, PRF_DISPALIGN)){
        sprintf(buf1, "Al:&n%s&n ", get_align_prompt_string(d->character));
        strncat(prompt, buf1, strlen(buf1));
      }
    }    

    if (FIGHTING(d->character)) {
      sprintf(buf1, "%s&n", get_fight_prompt(d->character));
      strncat(prompt, buf1, strlen(buf1));
    }

    if (!GET_PROMPT(d->character)) {
      strcat(prompt, "&n> ");
    } else {
      strcat(prompt, "&n"); 
    } 
    //sprintf(prompt, "&n%s&n> ", prompt);
    proc_color(prompt, d->character, (clr(d->character, C_NRM)), sizeof(prompt));
  } else if (STATE(d) == CON_PLAYING && IS_NPC(d->character)) {
      sprintf(prompt, "%s> ", GET_NAME(d->character));
  } else {
    *prompt = '\0';
  }

  return (prompt);
}


void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *newt;

  CREATE(newt, struct txt_block, 1);
  newt->text = str_dup(txt);
  newt->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    newt->next = NULL;
    queue->head = queue->tail = newt;
  } else {
    queue->tail->next = newt;
    queue->tail = newt;
    newt->next = NULL;
  }
}



int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return (0);

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return (1);
}



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
}

/* Add a new string to a player's output queue */
void write_to_output(const char *txt, struct descriptor_data *t)
{
  int size;

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  size = strlen(txt);

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size)
  {
    strcpy(t->output + t->bufptr, txt);
//    strncpy(t->output + t->bufptr, txt, strlen(t->output + t->bufptr));
    t->bufspace -= size;
    t->bufptr += size;
    return;
  }
  /*
   * If the text is too big to fit into even a large buffer, chuck the
   * new text and switch to the overflow state.
   */
  if (size + t->bufptr > LARGE_BUFSIZE - 1) 
  {
    t->bufptr = -1;
    buf_overflows++;
    return;
  }
  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) 
  {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else {			/* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
  t->output = t->large_outbuf->text;	/* make big buffer primary */
  strcat(t->output, txt);	/* now add new text */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;
}



/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

struct in_addr *get_bind_addr()
{
  static struct in_addr bind_addr;

  /* Clear the structure */
  memset((char *) &bind_addr, 0, sizeof(bind_addr));

  /* If DLFT_IP is unspecified, use INADDR_ANY */
  if (DFLT_IP == NULL) {
    bind_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* If the parsing fails, use INADDR_ANY */
    if (!parse_ip(DFLT_IP, &bind_addr)) {
      basic_mud_log("SYSERR: DFLT_IP of %s appears to be an invalid IP address",DFLT_IP);
      bind_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  /* Put the address that we've finally decided on into the logs */
  if (bind_addr.s_addr == htonl(INADDR_ANY))
    basic_mud_log("Binding to all IP interfaces on this host.");
  else
    basic_mud_log("Binding only to IP address %s", inet_ntoa(bind_addr));

  return (&bind_addr);
}

#ifdef HAVE_INET_ATON

/*
 * inet_aton's interface is the same as parse_ip's: 0 on failure, non-0 if
 * successful
 */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  return (inet_aton(addr, inaddr));
}

#elif HAVE_INET_ADDR

/* inet_addr has a different interface, so we emulate inet_aton's */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  long ip;

  if ((ip = inet_addr(addr)) == -1) {
    return (0);
  } else {
    inaddr->s_addr = (unsigned long) ip;
    return (1);
  }
}

#else

/* If you have neither function - sorry, you can't do specific binding. */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  basic_mud_log("SYSERR: warning: you're trying to set DFLT_IP but your system has no\n"
      "functions to parse IP addresses (how bizarre!)");
  return (0);
}

#endif /* INET_ATON and INET_ADDR */



/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s)
{
#if defined(SO_SNDBUF) && !defined(CIRCLE_MACINTOSH)
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return (-1);
  }

#if 0
  if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt RCVBUF");
    return (-1);
  }
#endif

#endif

  return (0);
}

int new_descriptor(socket_t s)
{
  socket_t desc;
  int sockets_connected = 0;
  socklen_t i;
  static int last_desc = 0;	/* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;
  
  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, (socklen_t *)&i)) == INVALID_SOCKET){
    perror("SYSERR: accept");
    return (-1);
  }
  /* keep it from blocking */
  nonblock(desc);

  /* set the send buffer size */
  if (set_sendbuf(desc) < 0) {
    CLOSE_SOCKET(desc);
    return (0);
  }

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= max_players) {
    write_to_descriptor(desc, "Sorry, PrimalMUD is full right now... please try again later!\r\n");
    CLOSE_SOCKET(desc);
    return (0);
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the sitename */
  if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
				      sizeof(peer.sin_addr), AF_INET))) {

    /* resolution failed */
    if (!nameserver_is_slow)
      perror("SYSERR: gethostbyaddr");

    /* find the numeric site address */
    strncpy(newd->host, (char *)inet_ntoa(peer.sin_addr), HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  } else {
    strncpy(newd->host, from->h_name, HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  }

  /* determine if the site is banned */
 // if (isbanned(newd->host) == BAN_ALL) {
  // DM - OLD hostname above ^ 
  //      NEW ip below
  if (isipbanned(peer.sin_addr, nameserver_is_slow) == BAN_ALL)
  {
    CLOSE_SOCKET(desc);
    sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
    mudlog(buf2, CMP, LVL_GOD, TRUE);
    free(newd);
    return (0);
  }

  /*
   * Log new connections - probably unnecessary, but you may want it.
   * Note that your immortals may wonder if they see a connection from
   * your site, but you are wizinvis upon login.
   */
  sprintf(buf2, "New connection from [%s]", newd->host);
//  basic_mud_log(buf);
  mudlog(buf2, CMP, LVL_IMPL, TRUE);

  /* initialize descriptor data */
  newd->descriptor = desc;
  newd->idle_tics = 0;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->login_time = time(0);
  *newd->output = '\0';
  newd->bufptr = 0;
  newd->has_prompt = 1;  /* prompt is part of greetings */
  newd->bad_pws = 0; /* DM: initialise bad_pws?? */
  newd->report = NULL;
  //STATE(newd) = CON_QCOLOUR;
  STATE(newd) = CON_GET_NAME;
  
  gameInfo.setUsersOnline(gameInfo.getUsersOnline() + 1);
	
  /*
   * This isn't exactly optimal but allows us to make a design choice.
   * Do we embed the history in descriptor_data or keep it dynamically
   * allocated and allow a user defined history size?
   */
  CREATE(newd->history, char *, HISTORY_SIZE);

  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  newd->next = descriptor_list;
  descriptor_list = newd;

  if (newd->character == NULL) {
    CREATE(newd->character, struct char_data, 1);
    clear_char(newd->character);
    CREATE(newd->character->player_specials, struct player_special_data, 1);
    newd->character->desc = newd;
  }

  SEND_TO_Q(GREETINGS, newd);
  //SEND_TO_Q("Welcome to PrimalMUD!\r\nWould you like colour? ", newd);

  return (0);
}


/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 */
int process_output(struct descriptor_data *t)
{
  char i[MAX_SOCK_BUF]="";
  int written = 0, offset, result;

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
  {
    strcat(i, "\r\n**OVERFLOW**\r\n");
  } else {
    /* we may need this \r\n for later -- see below */
    strcpy(i, "\r\n");

    /* now, append the 'real' output */
    if (strlen(t->output) >= sizeof(i)-3)
    {
      strncpy(i+2, t->output, sizeof(i)-GARBAGE_SPACE-2);
      i[sizeof(i)-GARBAGE_SPACE-2] = '\0';
      strcat(i, "**OVERFLOW**\r\n");
    } else {
      strncpy(i + 2, t->output, sizeof(i)-3);
      i[sizeof(i)-1] = '\0';
    }
  }
  /* add the extra CRLF if the person isn't in compact mode */
  if (STATE(t) == CON_PLAYING && t->character && !IS_NPC(t->character) && !PRF_FLAGGED(t->character, PRF_COMPACT) && (strlen(i) < sizeof(i)-3))
    strcat(i + 2, "\r\n");

  if (t->character)
    proc_color(i, t->character, (clr(t->character, C_NRM)), sizeof(i)-GARBAGE_SPACE);

  /* primal Easy color - DM 
  if (t->character)
    if (GET_LEVEL(t->character) > 0)
      proc_color(i, (clr(t->character, C_NRM)));
    else
      proc_color(i, C_OFF);

  */

  // DM - TESTING
  /* add a prompt */
  if (t->character)
  {
    if (strlen(i) + MAX_PROMPT_LENGTH < sizeof(i))
      strncat(i + 2, make_prompt(t), MAX_PROMPT_LENGTH);
    else
    {
      char ps[MAX_PROMPT_LENGTH]="";
      strncpy(ps, make_prompt(t), MAX_PROMPT_LENGTH);
      if ((strlen(i) + strlen(ps)) >= sizeof(i)-1)
      {
	i[sizeof(i)-GARBAGE_SPACE-strlen(ps)] = '\0';
	strcat(i, "\r\n**OVERFLOW**\r\n");
	strcat(i, ps);
      }
    }
  }

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (t->has_prompt)		/* && !t->connected) */
    offset = 0;
  else
    offset = 2;
  
  result = write_to_descriptor(t->descriptor, i + offset);
  written = result >= 0 ? result : -result;

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  /* Error, cut off. */
  if (result == 0)
    return (-1);
  
  /* Normal case, wrote ok. */
  if (result > 0)
    return (1);
  
  /*
   * We blocked, restore the unwritten output. Known
   * bug in that the snooping immortal will see it twice
   * but details...
   */
  write_to_output(i + written + offset, t);
  return (0);  
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

#if defined(CIRCLE_WINDOWS)

ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  ssize_t result;

  result = send(desc, txt, length, 0);

  if (result > 0) {
    /* Write was sucessful */
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    basic_mud_log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /* result < 0: An error was encountered. */

  /* Transient error? */
  if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
    return (0);

  /* Must be a fatal error. */
  return (-1);
}

#else

#if defined(CIRCLE_ACORN)
#define write	socketwrite
#endif

/* perform_socket_write for all Non-Windows platforms */
ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  ssize_t result;

  result = write(desc, txt, length);

  if (result > 0) {
    /* Write was successful. */
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    basic_mud_log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /*
   * result < 0, so an error was encountered - is it transient?
   * Unfortunately, different systems use different constants to
   * indicate this.
   */

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return (0);
#endif

#ifdef EWOULDBLOCK	/* BSD */
  if (errno == EWOULDBLOCK)
    return (0);
#endif

#ifdef EDEADLK		/* Macintosh */
  if (errno == EDEADLK)
    return (0);
#endif

  /* Looks like the error was fatal.  Too bad. */
  return (-1);
}

#endif /* CIRCLE_WINDOWS */

    
/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 *  0  If all is well and good,
 * e1  If an error was encnuntered, so that the player should be cut off
 * encountered. 'written' is updated to add how many bytes were sent
 * over the socket successfully prior to the return. It is not zero'd.
 *
 * Returns:
 *  +  All is well and good.
 *  0  A fatal or unexpected error was encountered.
 *  -  The socket write would block.
 */
int write_to_descriptor(socket_t desc, const char *txt)
{
  ssize_t bytes_written, total_written = 0;
  size_t total = strlen(txt);
 
   while (total > 0) {
     bytes_written = perform_socket_write(desc, txt, total);
 
     if (bytes_written < 0) {
       /* Fatal error.  Disconnect the player. */
       perror("SYSERR: write_to_descriptor");
       return (0);
     } else if (bytes_written == 0) {
       /*
        * Temporary failure -- socket buffer full.  For now we'll just
        * cut off the player, but eventually we'll stuff the unsent
        * text into a buffer and retry the write later.  JE 30 June 98.
        * Implemented the anti-cut-off code he wanted. GG 13 Jan 99.
        */
       basic_mud_log("WARNING: write_to_descriptor: socket write would block.");
       return (-total_written);
     } else {
       txt += bytes_written;
       total -= bytes_written;
       total_written += bytes_written;
     }
   }                 

  return (total_written);
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left)
{
  ssize_t ret;

#if defined(CIRCLE_ACORN)
  ret = recv(desc, read_point, space_left, MSG_DONTWAIT);
#elif defined(CIRCLE_WINDOWS)
  ret = recv(desc, read_point, space_left, 0);
#else
  ret = read(desc, read_point, space_left);
#endif

  /* Read was successful. */
  if (ret > 0)
    return (ret);

  /* read() returned 0, meaning we got an EOF. */
  if (ret == 0) {
    basic_mud_log("WARNING: EOF on socket read (connection broken by peer)");
    return (-1);
  }

  /*
   * read returned a value < 0: there was an error
   */

#if defined(CIRCLE_WINDOWS)	/* Windows */
  if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
    return (0);
#else

#ifdef EINTR		/* Interrupted system call - various platforms */
  if (errno == EINTR)
    return (0);
#endif

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return (0);
#endif

#ifdef EWOULDBLOCK	/* BSD */
  if (errno == EWOULDBLOCK)
    return (0);
#endif /* EWOULDBLOCK */

#ifdef EDEADLK		/* Macintosh */
  if (errno == EDEADLK)
    return (0);
#endif

#endif /* CIRCLE_WINDOWS */

  /*
   * We don't know what happened, cut them off. This qualifies for
   * a SYSERR because we have no idea what happened at this point.
   */
  perror("SYSERR: perform_socket_read: about to lose connection");
  return (-1);
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 *
 * Ever wonder why 'tmp' had '+8' on it?  The crusty old code could write
 * MAX_INPUT_LENGTH+1 bytes to 'tmp' if there was a '$' as the final
 * character in the input buffer.  This would also cause 'space_left' to
 * drop to -1, which wasn't very happy in an unsigned variable.  Argh.
 * So to fix the above, 'tmp' lost the '+8' since it doesn't need it
 * and the code has been changed to reserve space by accepting one less
 * character. (Do you really need 256 characters on a line?)
 * -gg 1/21/2000
 */
int process_input(struct descriptor_data *t)
{
  int buf_length, failed_subst;
  ssize_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 3;//1;

  do {
    if (space_left <= 0) {
      basic_mud_log("WARNING: process_input: about to close connection: input overflow");
      return (-1);
    }

    bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

    if (bytes_read < 0)	/* Error, disconnect them. */
      return (-1);
    else if (bytes_read == 0)	/* Just blocking, no problems. */
      return (0);

    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';	/* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if !defined(POSIX_NONBLOCK_BROKEN)
  } while (nl_pos == NULL);
#else
  } while (0);

  if (nl_pos == NULL)
    return (0);
#endif /* POSIX_NONBLOCK_BROKEN */

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 3;//1;

    /* The '> 1' reserves room for a '$ => $$' expansion. */
    for (ptr = read_point; (space_left > 3/*1*/) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b' || *ptr == 127) { /* handle backspacing or delete key */
	if (write_point > tmp) {
	  if (*(--write_point) == '$') {
	    write_point--;
	    space_left += 2;
	  } else
	    space_left++;
	}
      } else if (isascii(*ptr) && isprint(*ptr)) {
	if ((*(write_point++) = *ptr) == '$') {		/* copy one character */
	  *(write_point++) = '$';	/* if it's a $, double it */
	  space_left -= 2;
	} else
	  space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) == 0)     /* write_to_output? */  
	return (-1);
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1)))	/* Redo last command. */
      strcpy(tmp, t->last_input);
    else if (*tmp == '!' && *(tmp + 1)) {
      char *commandln = (tmp + 1);
      int starting_pos = t->history_pos,
	  cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
	if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
	  strcpy(tmp, t->history[cnt]);
	  strcpy(t->last_input, tmp);
          SEND_TO_Q(tmp, t); SEND_TO_Q("\r\n", t);
	  break;
	}
        if (cnt == 0)	/* At top, loop to bottom. */
	  cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);
    } else {
      strcpy(t->last_input, tmp);
      if (t->history[t->history_pos])
	free(t->history[t->history_pos]);	/* Clear the old line. */
      t->history[t->history_pos] = str_dup(tmp);	/* Save the new. */
      if (++t->history_pos >= HISTORY_SIZE)	/* Wrap to top. */
	t->history_pos = 0;
    }

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return (1);
}



/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char newsub[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return (1);
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return (1);
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig, (strpos - orig));
  newsub[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(newsub, second, (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub);

  return (0);
}



void close_socket(struct descriptor_data *d)
{
  char buf[128];
  struct descriptor_data *temp;
  struct user_data *tempuser;

  REMOVE_FROM_LIST(d, descriptor_list, next);
  CLOSE_SOCKET(d->descriptor);
  flush_queues(d);

  gameInfo.setUsersOnline(gameInfo.getUsersOnline() - 1);

  for (tempuser = user_list; tempuser; tempuser = tempuser->next)
  {
    if (tempuser->number == d->desc_num) {
      tempuser->logout = time(0);
      break;
    }
  }
  
  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) 
  {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }

  if (d->character) 
  {
    /*
     * Plug memory leak, from Eric Green.
     */
    if (!IS_NPC(d->character) && PLR_FLAGGED(d->character, PLR_MAILING) && d->str) {
      if (*(d->str))
        free(*(d->str));
      free(d->str);
    }
    if (STATE(d) == CON_PLAYING || STATE(d) == CON_DISCONNECT || OLC(d) ||
        REPORT(d)) 
    {
      act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
      if (!IS_NPC(d->character))
      {
	save_char(d->character, NOWHERE);
	sprintf(buf, "Closing link to: &7%s&g.", GET_NAME(d->character));
	mudlog(buf, NRM, MAX(LVL_ANGEL, GET_INVIS_LEV(d->character)), TRUE);
        info_channel(buf , d->character );
      }
      d->character->desc = NULL;
    } else {
      sprintf(buf, "Losing player: %s.",
	      GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      mudlog(buf, CMP, LVL_ANGEL, TRUE);
        
      // DM - delete new characters which haven't been fully created.. 
      if (IS_SET(PLR_FLAGS(d->character), PLR_DELETED))
      {
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
        save_char(d->character, NOWHERE);
	Crash_delete_file(GET_NAME(d->character));	
      }
      free_char(d->character);
    }
  } else
    mudlog("Losing descriptor without char.", CMP, LVL_ANGEL, TRUE);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  /* Clear the command history. */
  if (d->history)
  {
    int cnt;
    for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
      if (d->history[cnt])
	free(d->history[cnt]);
    free(d->history);
  }

  if (d->showstr_head)
    free(d->showstr_head);
  if (d->showstr_count)
    free(d->showstr_vector);

  /*. Kill any OLC stuff .*/
  switch (d->connected)
  {
    case CON_OEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
    case CON_TRIGEDIT:
      cleanup_olc(d, CLEANUP_ALL);
      break;
    case CON_TEDIT:
      if (OLC(d))
        free(OLC(d));
      break;
    case CON_REPORT_ADD:
    case CON_REPORT_EDIT:
      if (REPORT(d)) {
        basic_mud_log("cleaning up descriptor - deleting report %d", (int)d->report);
        delete (Report *)REPORT(d);
      }
      break;
    default:
      break;
  } 

  free(d);
}


void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d)
  {
    next_d = d->next;
    switch (STATE(d))
    {
      case CON_PLAYING:
      case CON_CLOSE:
      case CON_DISCONNECT:
      case CON_OEDIT:
      case CON_REDIT:
      case CON_ZEDIT:
      case CON_MEDIT:
      case CON_SEDIT:
      case CON_TEDIT:
      case CON_REPORT_ADD:
      case CON_REPORT_EDIT:
      case CON_TRIGEDIT:
	continue;
    }
    d->idle_tics++;
    if (d->idle_tics > 2)
    {
      echo_on(d);
      SEND_TO_Q("\r\nClosing idle connection... Goodbye.\r\n", d);
      STATE(d) = CON_CLOSE;
    }
  }
}


#if 0 // Artus> Lets change this a but.
void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  // Use the define to specify name prompt timeouts
  
//#undef IGNORE_NAME
#define IGNORE_NAME
  for (d = descriptor_list; d; d = next_d)
  {
    next_d = d->next;
#if defined(IGNORE_NAME)
    if (STATE(d) != CON_PASSWORD)
      continue;
#else
    if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME)
      continue;
#endif
    if (!d->idle_tics)
    {
      d->idle_tics++;
      continue;
    } else {
      echo_on(d);
      SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
      STATE(d) = CON_CLOSE;
    }
  }
}
#endif


/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#if defined(CIRCLE_WINDOWS)

void nonblock(socket_t s)
{
  unsigned long val = 1;
  ioctlsocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_AMIGA)

void nonblock(socket_t s)
{
  long val = 1;
  IoctlSocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_ACORN)

void nonblock(socket_t s)
{
  int val = 1;
  socket_ioctl(s, FIONBIO, &val);
}

#elif defined(CIRCLE_VMS)

void nonblock(socket_t s)
{
  int val = 1;

  if (ioctl(s, FIONBIO, &val) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

#elif defined(CIRCLE_UNIX) || defined(CIRCLE_OS2) || defined(CIRCLE_MACINTOSH)

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

#endif  /* CIRCLE_UNIX || CIRCLE_OS2 || CIRCLE_MACINTOSH */


/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)

RETSIGTYPE reread_wizlists(int sig)
{
  mudlog("Signal received - rereading wizlists.", NRM, LVL_GOD, TRUE);
  reboot_wizlists();
}


RETSIGTYPE unrestrict_game(int sig)
{
  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	 BRF, LVL_GRGOD, TRUE);
  ban_list = NULL;
  circle_restrict = 0;
  num_invalid = 0;
}

#ifdef CIRCLE_UNIX

/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);

  my_signal(SIGCHLD, reap);
}

RETSIGTYPE checkpointing(int sig)
{
  if (!tics) {
    basic_mud_log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
    abort();
  } else
    tics = 0;
}

RETSIGTYPE hupsig(int sig)
{
  struct descriptor_data *i;

  // DM - lets save chars in descriptor list
  for (i = descriptor_list; i; i=i->next) {
    if (i->character) {
      save_char(i->character, NOWHERE);
    }
  }

  basic_mud_log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.");
  basic_mud_log("        Saving chars and Shutting down...");
  exit(1);			/* perhaps something more elegant should
				 * substituted */
}

#endif	/* CIRCLE_UNIX */

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return (SIG_ERR);

  return (oact.sa_handler);
}
#endif				/* POSIX */


void signal_setup(void)
{
#ifndef CIRCLE_MACINTOSH
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.
   */
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGCHLD, reap);
#endif /* CIRCLE_MACINTOSH */
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
}

#endif	/* CIRCLE_UNIX || CIRCLE_MACINTOSH */

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

void send_to_zone(const char *msg, zone_rnum zone)
{
  struct descriptor_data *i;

  if (msg == NULL)
	return;

  for(i = descriptor_list; i; i = i->next)
    if ((STATE(i) == CON_PLAYING) && 
	(world[(i->character)->in_room].zone == zone))
      SEND_TO_Q(msg, i);
}

#if 0 // Artus> This is currently unused.
void send_to_not_zone(const char *msg, zone_rnum zone)
{
  struct descriptor_data *i;

  if (msg == NULL)
	return;

  for(i = descriptor_list; i; i = i->next)
  {
      if (STATE(i) == CON_PLAYING && (world[(i->character)->in_room].zone != zone ) )
  	SEND_TO_Q(msg, i);
  }	
}
#endif
// Artus> Send to all not in the zone, but in the same world.
void send_to_not_zone_world(const char *msg, zone_rnum zone)
{
  struct descriptor_data *i;
  extern struct zone_data *zone_table;

  if (msg == NULL)
    return;
  for (i = descriptor_list; i; i = i->next)
  {
    if ((STATE(i) == CON_PLAYING) && 
	(world[(i->character)->in_room].zone != zone) &&
	(zone_table[world[(i->character)->in_room].zone].world == 
	 zone_table[zone].world))
      SEND_TO_Q(msg, i);
  }
}

void send_to_char(const char *messg, struct char_data *ch)
{
  if (ch->desc && messg)
    SEND_TO_Q(messg, ch->desc);
}


void send_to_all(const char *messg)
{
  struct descriptor_data *i;

  if (messg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (STATE(i) == CON_PLAYING)
      SEND_TO_Q(messg, i);
}


void send_to_outdoor(const char *messg)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) || !OUTSIDE(i->character))
      continue;
    SEND_TO_Q(messg, i);
  }
}



void send_to_room(const char *messg, room_rnum room)
{
  struct char_data *i;

  if (messg == NULL)
    return;

  for (i = world[room].people; i; i = i->next_in_room)
    if (i->desc)
      SEND_TO_Q(messg, i->desc);
}



const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
		const void *vict_obj, const struct char_data *to)
{
  const char *i = NULL;
  char lbuf[MAX_STRING_LENGTH], *buf;
  char lbuf2[MAX_STRING_LENGTH], *buf2;
  struct char_data *tch; 
  struct char_data *dg_victim = NULL;
  struct obj_data *dg_target = NULL;
  char *dg_arg = NULL;
  char buf3[MAX_STRING_LENGTH];

  buf = lbuf;
  buf2 = lbuf2;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
        if (!IS_NPC(ch) && IS_SET(GET_SPECIALS(ch), SPECIAL_DISGUISE) && CHAR_DISGUISED(ch)) {
		tch = read_mobile(CHAR_DISGUISED(ch), VIRTUAL);
		char_to_room(tch, ch->in_room);
		strcpy(buf3, GET_NAME(tch));
		sprintf(lbuf2, "&6%s&n", buf3);
		i = lbuf2;
		extract_char(tch);
        }
        else {          
          strcpy(buf3, PERS(ch, to));
	  if (strcmp(buf3, "someone")) {
            sprintf(lbuf2, "&%s%s&|", IS_NPC(ch) ? "6" : "7", buf3);
	  } else {
            sprintf(lbuf2, "&r%s&|", buf3);
	  }
	  i = lbuf2;
	}
	break;
      case 'N':
	CHECK_NULL(vict_obj, PERS((const struct char_data *) vict_obj, to));
	sprintf(lbuf2, "&%s%s&|", IS_NPC((struct char_data *) vict_obj) 
			? "6" : "7", i);
	i = lbuf2;
        dg_victim = (struct char_data *) vict_obj;
	break;
      case 'm':
	i = HMHR(ch);
	break;
      case 'M':
	CHECK_NULL(vict_obj, HMHR((const struct char_data *) vict_obj));
        dg_victim = (struct char_data *) vict_obj;
	break;
      case 's':
	i = HSHR(ch);
	break;
      case 'S':
	CHECK_NULL(vict_obj, HSHR((const struct char_data *) vict_obj));
        dg_victim = (struct char_data *) vict_obj;
	break;
      case 'e':
	i = HSSH(ch);
	break;
      case 'E':
	CHECK_NULL(vict_obj, HSSH((const struct char_data *) vict_obj));
        dg_victim = (struct char_data *) vict_obj;
	break;
      case 'o':
	CHECK_NULL(obj, OBJN(obj, to));
	sprintf(lbuf2, "&%s%s&|", strcmp(i, "something") ? "5" : "r", i);
	i = lbuf2;
	break;
      case 'O':
	CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj, to));
	sprintf(lbuf2, "&%s%s&|", strcmp(i, "something") ? "5" : "r", i);
	i = lbuf2;
        dg_victim = (struct char_data *) vict_obj;
	break;
      case 'p':
	CHECK_NULL(obj, OBJS(obj, to));
	sprintf(lbuf2, "&5%s&|", i);
	i = lbuf2;
	break;
      case 'P':
	CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to));
	sprintf(lbuf2, "&5%s&|", i);
	i = lbuf2;
        dg_victim = (struct char_data *) vict_obj;
	break;
      case 'a':
	CHECK_NULL(obj, SANA(obj));
	break;
      case 'A':
	CHECK_NULL(vict_obj, SANA((const struct obj_data *) vict_obj));
        dg_victim = (struct char_data *) vict_obj;
	break;
      case 'T':
	CHECK_NULL(vict_obj, (const char *) vict_obj);
        dg_arg = (char *) vict_obj;
	break;
      case 'F':
	CHECK_NULL(vict_obj, fname((const char *) vict_obj));
	break;
      case '$':
	i = "$";
	break;
      default:
	basic_mud_log("SYSERR: Illegal $-code to act(): %c", *orig);
	basic_mud_log("SYSERR: %s", orig);
	i = "";
	break;
      }
      while ((*buf = *(i++)))
	buf++;
      orig++;
    } else if (!(*(buf++) = *(orig++)))
      break;
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  if (to->desc)
    SEND_TO_Q(CAP(lbuf), to->desc);

  if ((IS_NPC(to) && dg_act_check) && (to != ch))
    act_mtrigger((char_data *)to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);
}

void act(const char *str, int hide_invisible, struct char_data *ch,
	 struct obj_data *obj, const void *vict_obj, int type)
{
  const struct char_data *to;
  int to_sleeping;

  if (!str || !*str)
    return;

  if (!(dg_act_check = !(type & DG_NO_TRIG)))
    type &= ~DG_NO_TRIG;

  /*
   * Warning: the following TO_SLEEP code is a hack.
   * 
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   * TO_SLEEP only works because its value "happens to be" a single bit;
   * do not change it to something else.  In short, it is a hack.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((to_sleeping = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;

  if (type == TO_CHAR) {
    if (ch && SENDOK(ch))
      perform_act(str, ch, obj, vict_obj, ch);
    return;
  }

  if (type == TO_VICT) {
    if ((to = (const struct char_data *) vict_obj) != NULL && SENDOK(to))
      perform_act(str, ch, obj, vict_obj, to);
    return;
  }
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && ch->in_room != NOWHERE)
    to = world[ch->in_room].people;
  else if (obj && obj->in_room != NOWHERE)
    to = world[obj->in_room].people;
  else {
    basic_mud_log("SYSERR: no valid target to act()!");
    return;
  }

  for (; to; to = to->next_in_room) {
    //if (!SENDOK(to) || (to == ch))
    if (!SENDTOOK(to) || (to == ch))
      continue;
    if (hide_invisible && ch && !CAN_SEE(to, ch))
      continue;
    if (type != TO_ROOM && to == vict_obj)
      continue;
    perform_act(str, ch, obj, vict_obj, to);
  }
}

/*
 * This function is called every 30 seconds from heartbeat().  It checks
 * the four global buffers in CircleMUD to ensure that no one has written
 * past their bounds.  If our check digit is not there (and the position
 * doesn't have a NUL which may result from snprintf) then we gripe that
 * someone has overwritten our buffer.  This could cause a false positive
 * if someone uses the buffer as a non-terminated character array but that
 * is not likely. -gg
 */
void sanity_check(void)
{
  int ok = TRUE;

  /*
   * If any line is false, 'ok' will become false also.
   */
  ok &= (test_magic(buf)  == MAGIC_NUMBER || test_magic(buf)  == '\0');
  ok &= (test_magic(buf1) == MAGIC_NUMBER || test_magic(buf1) == '\0');
  ok &= (test_magic(buf2) == MAGIC_NUMBER || test_magic(buf2) == '\0');
  ok &= (test_magic(arg)  == MAGIC_NUMBER || test_magic(arg)  == '\0');

  /*
   * This isn't exactly the safest thing to do (referencing known bad memory)
   * but we're doomed to crash eventually, might as well try to get something
   * useful before we go down. -gg
   * However, lets fix the problem so we don't spam the logs. -gg 11/24/98
   */
  if (!ok) {
    basic_mud_log("SYSERR: *** Buffer overflow! ***\n"
	"buf: %s\nbuf1: %s\nbuf2: %s\narg: %s", buf, buf1, buf2, arg);

    plant_magic(buf);
    plant_magic(buf1);
    plant_magic(buf2);
    plant_magic(arg);
  }

#if 0
  basic_mud_log("Statistics: buf=%d buf1=%d buf2=%d arg=%d",
	strlen(buf), strlen(buf1), strlen(buf2), strlen(arg));
#endif
}

/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd)
{
  FILE *s_fp;

#if defined(__MWERKS__) || defined(__GNUC__)
  s_fp = stderr;
#else
  if ((s_fp = fdopen(STDERR_FILENO, "w")) == NULL) {
    puts("SYSERR: Error opening stderr, trying stdout.");

    if ((s_fp = fdopen(STDOUT_FILENO, "w")) == NULL) {
      puts("SYSERR: Error opening stdout, trying a file.");

      /* If we don't have a file, try a default. */
      if (filename == NULL || *filename == '\0')
        filename = "log/syslog";
    }
  }
#endif

  if (filename == NULL || *filename == '\0') {
    /* No filename, set us up with the descriptor we just opened. */
    logfile = s_fp;
    puts("Using file descriptor for logging.");
    return;
  }

  /* We honor the default filename first. */
  if (open_logfile(filename, s_fp))
    return;

  /* Well, that failed but we want it logged to a file so try a default. */
  if (open_logfile("log/syslog", s_fp))
    return;

  /* Ok, one last shot at a file. */
  if (open_logfile("syslog", s_fp))
    return;

  /* Erp, that didn't work either, just die. */
  puts("SYSERR: Couldn't open anything to log to, giving up.");
  exit(1);
}

int open_logfile(const char *filename, FILE *stderr_fp)
{
  if (stderr_fp)	/* freopen() the descriptor. */
    logfile = freopen(filename, "w", stderr_fp);
  else
    logfile = fopen(filename, "w");

  if (logfile) {
    printf("Using log file '%s'%s.\n",
		filename, stderr_fp ? " with redirection" : "");
    return (TRUE);
  }

  printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
  return (FALSE);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */
#if defined(CIRCLE_WINDOWS)

void circle_sleep(struct timeval *timeout)
{
  Sleep(timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
}

#else

void circle_sleep(struct timeval *timeout)
{
  if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
    if (errno != EINTR) {
      perror("SYSERR: Select sleep");
      exit(1);
    }
  }
}

#endif /* CIRCLE_WINDOWS */
