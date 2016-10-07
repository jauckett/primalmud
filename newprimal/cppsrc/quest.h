/* Artus> Quest Structs, Constants, Etc. */

// Different type of quest definitions
#define QUEST_UNKNOWN		0	// Unknown Quest Type.
#define QUEST_ITEM_HUNT		1	// Item Hunt Quest.
#define QUEST_TRIVIA		2	// Trivia Quest.

// Structure for quest scores -- Artus.
struct quest_score_data
{
  long chID;
  char name[MAX_NAME_LENGTH+1];
  int score;
  struct quest_score_data *next;
};

// Structure for item hunt -- Artus.
struct itemhunt_data
{
  struct event_data *ev;
  obj_rnum itemrnum;
  obj_vnum itemvnum;
  int lowlevel;
  int mobsloaded;
  struct quest_score_data *players;
};

// Structure for trivia responses -- Artus.
struct trivia_response_data
{
  struct quest_score_data *player;
  char response[MAX_INPUT_LENGTH+1];
  struct trivia_response_data *next;
};

// Structure for trivia -- Artus.
struct trivia_data
{
  struct event_data *ev;
  int trivmaster;
  int qcount;
  char question[MAX_INPUT_LENGTH+1];
  char description[MAX_INPUT_LENGTH+1];
  struct trivia_response_data *resp;
  struct quest_score_data *players;
};
