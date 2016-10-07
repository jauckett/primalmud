/* Define edit modes */
#define EM_VNUM 	1
#define EM_ZONE		2
#define EM_SECT		3
#define EM_RFLAG	4
#define EM_TITLE	5
#define EM_ROOM_DESC	6
#define EM_EXITS	7
#define EM_EXIT_DESC	8         
#define EM_DFLAG	9
#define EM_TO_ROOM	10
#define EM_KEY		11
#define EM_EXTRA_DESC	12
#define EM_HELP		20
#define EM_QUIT		21
#define EM_MENU		22

/*----------------*/
#define debug 1
/*----------------*/


/********* My defs *******/
#define XOR 	^
#define SPACE " "
#define TRUE	1
#define FALSE 	0
#define INVALID -1
#define NOWHERE -1

#define REDIT_SAVE_FILE "redit.wld"
#define SAVE_FILE       "/nscu/segodfr/redit-save.wld"

#define DEFAULT_ROOM_NAME 		"An unfinished room"
#define DEFAULT_ROOM_DESCRIPTION	"An empty room."


/*====================== Macro functions ==============*/
#define cls() {send_to_char("\n\n\n\n\n", coder);};  /* Make some space between output. */
#define GET_INPUT_STR(size) {   \
   coder->desc->str = &coder->desc->edit_str; \
   *(coder->desc->str) = 0;  \
   coder->desc->max_str = size; }

#define GET_FIELD(field, max_size) { \
   coder->desc->str = &field;   \
   *(coder->desc->str) = 0;     \
   coder->desc->max_str = max_size; }

#define RESET_EDITM() {  \
      display_room(coder->desc->room_editing, coder);  \
      coder->desc->edit_mode = 0;  \
      STATE(coder->desc) = CON_EDITM;  }

#define SET_REDITM(mode) {  \
      coder->desc->edit_mode = mode;  \
      STATE(coder->desc) = CON_REDITM;  }




/*====================== Menu definitions ==============*/

static char *SECTOR_TYPE_MENU =
      "======================== <sector_type> ======================== \n"
      "<sector_type> is: \n\n"
      "This determines how many movement points are used when moving through \n"
      "a location of the type - use one of the numbers 0..7 (they are NOT the \n"
      "movement-points used - merely indexes to a lookup-table): \n"
      "\n"
      "SECT_INSIDE          0  Uses as if walking indoors \n"
      "SECT_CITY            1  Uses as if walking in a city \n"
      "SECT_FIELD           2  Uses as if walking in a field \n"
      "SECT_FOREST          3  Uses as if walking in a forest \n"
      "SECT_HILLS           4  Uses as if walking in hills \n"
      "SECT_MOUNTAIN        5  Uses as if climbing in mountains \n"
      "SECT_WATER_SWIM      6  Uses as if swimming \n"
      "SECT_WATER_NOSWIM    7  Impossible to swim water - requires a boat \n\n"
      "Enter sector_type: ";


 
static char *ROOM_FLAG_MENU =
   "\n"
   "\t 1. DARK      7.  CHAOTIC \n"
   "\t 2. DEATH     8.  NO_MAGIC \n"
   "\t 3. NO_MOB    9.  TUNNEL \n"
   "\t 4. INDOORS   10. PRIVATE \n"
   "\t 5. LAWFULL   11. GODROOM \n"
   "\t 6. NEUTRAL   12. Quit this menu. \n\n"
   "Enter number to toggle roomvector: \n";

static char *ROOM_FLAG_MENU2 =
   "\n"
   "\t 0 - DARK      6 - CHAOTIC \n"
   "\t 1 - DEATH     7 - NO_MAGIC \n"
   "\t 2 - NO_MOB    8 - TUNNEL \n"
   "\t 3 - INDOORS   P,p - PRIVATE \n"
   "\t 4 - LAWFULL   G,g - GODROOM \n"
   "\t 5 - NEUTRAL   \n"
   "\t    Q,q - Quit this menu. \n\n"
   "Enter number to toggle roomvector: ";


static char *ROOM_DESC_MENU =
   "======================== <Room description> ======================== \n"
   "<description>~<NL>: \n"
   "\t  This is the general description of the room. \n"
   "\nEnter Room description (enter a @ and 2 CR to end): \n";



static char *DIRECTION_MENU =     
      " ------------ Direction fields:  ------------ \n\n"
      "<Exit number> is one of: \n"
      "\t 0 = North \n"
      "\t 1 = East \n"
      "\t 2 = South \n"
      "\t 3 = West \n"
      "\t 4 = Up \n"
      "\t 5 = Down \n"
      "\t H, h Help \n"
      "\t Q,q. Quit Direction Menu \n\n"
      "Enter direction: (0-5): ";

static char *KEYWORD_MENU =
      "Add extra description: \n---------------------- \n"
      "<blank separated keyword list>~<NL>  \n"
      "\tex:  statue grey marble\n\n"
      "Enter --keyword-- list, ending with a @:\n";

static char *ROOM_TITLE_MENU =
      "======================== <Room title> ======================== \n"
      "<name>~<NL>:  \n"
      "Enter Room title: \n";

static char *VNUM_MENU =
      "======================== <virtual number> ========================\n"
      "#<virtual number> \n\n"
      "Enter virtual number:  ";

static char *ZONE_MENU =
      "======================== <zone nr> ======================== \n"
      "<zone nr>\nEnter the number of the zone in which this room is located.\n"
      "End input with a @\n";

static char *HELP_MENU =
      "-------------------- HELP MENU -------------\n\n\r"
      "** You can't change Vnum for an already existing room.\n\r"
      " \n\r"
      "** At this time, you may have to hit the return key once\n\r"
      "   or twice after entering some fields.. to return to the\n\r"
      "   current menu. \n\r"
      " \n\r"
      "** Input of 1-11 toggles the room flag. input of 12 quits\n\r"
      "   for the room_flag menu.\n\r"
      " \n\r"
      "** To enter a field (other than a menu selection) end that field \n\r"
      "   with a @ \n\r"
      "             Example:    ] simple room description. @ \n\r"
      " \n\r"
      "** In the extra description field, enter keywords first. These are\n\r"
      "   the items that you can look at while in the room...\n\r"
      "   Next comes the descriptions for those keywords. \n\r"
      "\n\r"
      "\n\r"
      "\n\r"
      "\t-** Press RETURN to continue **-"; 



/*===================================================*/
void edit_room(struct room_data *room, struct descriptor_data *d);

