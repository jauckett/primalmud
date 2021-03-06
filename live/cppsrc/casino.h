/* Gambling codes */
#define NO_GAME         0
#define END_GAME        1
#define START_GAME      2
#define DECLARE_GAME    5
 
/* Black jack codes */
#define BJ_NO_CODES      0
#define BJ_HAS_SPLIT     1
#define BJ_INSURANCE     2
 
/* Card game codes */
#define CARD_CONV_ACE   1
#define CARD_ACE        14
#define CARD_KING       13
#define CARD_QUEEN      12
#define CARD_JACK       11
#define CARD_TEN        10
#define CARD_NINE       9
#define CARD_EIGHT      8
#define CARD_SEVEN      7
#define CARD_SIX        6
#define CARD_FIVE       5
#define CARD_FOUR       4
#define CARD_THREE      3
#define CARD_TWO        2
#define CARD_NONE       0

SPECIAL(casino);
void init_race();		
void check_races(void);	
void check_games(void);	
void check_blackjack(void);
void removeBJPlayer(int);
void init_blackjack();
void init_race();
void init_games();
void gamble_race(struct char_data *ch, char *arg);
void gamble_blackjack(struct char_data *ch, char *argument);
void showBJHand(struct game_item *, int);
void setBJScore(struct game_item *, int);
int getCard();
char *getCardName(int);
void endBJGame(struct game_item *, int);
void handle_hit(struct char_data *ch);
void removeBJPlayer(int);
void handle_split(int);
int canSplit(int);
int hasBlackjack(int);
int isInGame(struct char_data *, char);
struct game_item *getGamePlayer(struct char_data *, char);
void gamble_slots(struct char_data *, char *arg);
void play_slots(struct char_data *, int);
void calcSlotResult(struct char_data *ch, int reel[5], int amount);
