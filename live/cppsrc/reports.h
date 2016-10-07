using std::list;
using std::ifstream;
using std::ofstream;

// Report state - used by Report::state
#define REPORT_STATE_OPEN     0  // State is open - initial state
#define REPORT_STATE_ASSESSED 1  // Assessed -  read by pers responsible
#define REPORT_STATE_FEEDBACK 2  // Feedback - report has been fixed
#define REPORT_STATE_CLOSED   3  // Closed - no longer active
#define REPORT_STATE_DELETED  4  // Deleted - will not be loaded to list at boot

#define NUM_REPORT_STATES     5

// Report types - used by Report::type
#define REPORT_TYPE_IDEA        0  // An idea
#define REPORT_TYPE_TYPO        1  // A Typo - spelling mistake etc...
#define REPORT_TYPE_TODO        2  // Todo report
#define REPORT_TYPE_BUGCRASH    3  // Report causes a crash
#define REPORT_TYPE_BUGBUILDING 4  // Report concerned with building
#define REPORT_TYPE_BUGBALANCE  5  // Report concerned with balance (
#define REPORT_TYPE_BUGOTHER    6  // Report doesnt fit other categories

// Listing bug types only is done by displaying from
// NUM_REPORT_TYPES - NUM_BUG_TYPES to NUM_REPORT_TYPES 
// This way we can add additional bug types - no need to add other report
// types on top of idea/typo/todo/bug
#define NUM_BUG_TYPES           4  
#define NUM_REPORT_TYPES        7

// Adding/Editing state defines
#define REPORT_STATE_MENU          0       // Add Menu
#define REPORT_STATE_CONFIRM       1       // y or n save confirmation
#define REPORT_STATE_LONGDESC      2       // Long Description
#define REPORT_STATE_SHORTDESC     3       // Short Description
#define REPORT_STATE_CHANGEDESC    4       // Change Description
#define REPORT_STATE_TYPE          5       // Type of report 
#define REPORT_STATE_STATE         6       // State of report
#define REPORT_STATE_PLAYER        7       // Player ID/Name

// Adding/Editing mode defines
#define REPORT_MODE_BUG            0
#define REPORT_MODE_TYPO           1
#define REPORT_MODE_IDEA           2
#define REPORT_MODE_TODO           3
#define REPORT_MODE_LISTREPORT	   4
#define REPORT_MODE_PRINTREPORT	   5

// Adding/Editing macros
#define REPORT(d)          ((Report *)(d)->report)
#define REPORT_STATE(d)    (REPORT(d)->state)
#define REPORT_MODE(d)     (REPORT(d)->mode)
#define REPORT_MODIFIED(d) (REPORT(d)->modified)

#define REPORT_SHORTDESC_LENGTH  80   // DO NOT CHANGE (read/written bin length)
#define REPORT_LONGDESC_LENGTH  240  // DO NOT CHANGE (read/written bin length)

// External vars
extern const char *report_states[];
extern const char *report_types[];


void report_parse(struct descriptor_data *d, char *arg);
void report_string_cleanup(struct descriptor_data *d, int terminator); 
void getReportModeName(int mode, char *writeto);


class ReportChange {
  private:
    int changeNum;
    int reportNum;              // The report that this change belongs to
    long playerId;              // The player which made this change
    int fromState;              // The old state
    int toState;                // The new state 
    int fromType;               // The old type
    int toType;                 // The new type
    time_t changeTime;          // Time of change
    ReleaseInfo changeRelease;  // Release info at change
    char changeDescription[REPORT_SHORTDESC_LENGTH]; // Description of the chang

  public:
    // Default Constructor - initialise members
    ReportChange() {
      setChangeNum(0);
      setReportNum(0);
      setPlayerId(NOBODY);
      setTime(time(0));
      setDescription("");
    };

    // Contructs a ReportChange object with the given change details
    ReportChange(int changeNum, int reportNum, long playerId, int fromState, 
                 int toState, int fromType, int toType, time_t changeTime, 
                 ReleaseInfo rel, char *changeDesc) {
      setChangeNum(changeNum);
      setReportNum(reportNum);
      setPlayerId(playerId);
      setFromState(fromState);
      setToState(toState);
      setFromType(fromType);
      setToType(toType);
      setTime(changeTime);
      setRelease(rel);
      setDescription(changeDesc);
    };

    // set functions
    void setChangeNum(int changeNum) { ReportChange::changeNum = changeNum; };
    void setReportNum(int reportNum) { ReportChange::reportNum = reportNum; };
    void setPlayerId(long playerId) { ReportChange::playerId = playerId; };
    void setTime(time_t time) { ReportChange::changeTime = time; };
    void setFromState(int state) { ReportChange::fromState = state; };
    void setToState(int state) { ReportChange::toState = state; };
    void setFromType(int type) { ReportChange::fromType = type; };
    void setToType(int type) { ReportChange::toType = type; };
    void setRelease(ReleaseInfo rel) {
      changeRelease = ReleaseInfo(rel.getMajor(), rel.getBranch(), 
                      rel.getMinor(), rel.getTag(), rel.isCvsUpToDate(), rel.getDate());
    }

    void setDescription(char *changeDesc) {
      if (*changeDesc) {
        strncpy(changeDescription, changeDesc, REPORT_SHORTDESC_LENGTH); 
	changeDescription[REPORT_SHORTDESC_LENGTH] = '\0';
      } else {
        changeDescription[0] = '\0';
      }
    };

    // get functions
    int getChangeNum() { return ReportChange::changeNum; };
    int getReportNum() { return ReportChange::reportNum; };
    long getPlayerId() { return ReportChange::playerId; };
    int getFromState() { return ReportChange::fromState; };
    int getToState() { return ReportChange::toState; };
    int getFromType() { return ReportChange::fromType; };
    int getToType() { return ReportChange::toType; };
    time_t getTime() { return ReportChange::changeTime; };
    char *getChangeDescription() { return ReportChange::changeDescription; };
    ReleaseInfo getRelease() { return ReportChange::changeRelease; };

    void printDetails(char *writeto);

    // file realted functions
    struct reportchange_file_elem toFileElem(); 

    ~ReportChange() {};       // Destructor - nothing to do
};


class Report {
  private:
    long reporterId;            // Originator of report
    long playerId;              // Person responsible for report
    int reportNum;              // Report Number
    int reportState;                  // Current status on report - see REPORT_STATE_XXX
    int type;                   // Type of report reported = see REPORT_TYPE_XXX
    char shortDescription[REPORT_SHORTDESC_LENGTH];  // Short desc about report
    char longDescription[REPORT_LONGDESC_LENGTH];  // Long desc about report
    room_vnum orgRoom;          // The room the player reported the report in
    time_t orgTime;             // Time the report was reported  
    ReleaseInfo orgRelease;     // Release information of initial report
    list<ReportChange> changes;    // List of the changes made to report 

    char changeDescription[REPORT_SHORTDESC_LENGTH]; // Reason for change - copied
                                                  // straight to ReportChange
                                                  // Used as temp for REPORT(d)

  public:
    int state;                  // Report adding/editing state - current state 
    int mode;                   // Report adding/editing modes idea/report /typo/todo
    int modified;               // Report adding/editing modified flag
    
    // Default Constructor - Initialise members 
    Report() {
      setPlayerId(NOBODY);
      setReporterId(NOBODY);
      setReportNum(0);
      setState(REPORT_STATE_OPEN);
      setType(REPORT_TYPE_BUGCRASH);
      setShortDescription("");
      setLongDescription("");
      setChangeDescription("");
      setOrgRoom(NOWHERE);
      setOrgTime(time(0));

      mode = -1;
      modified = 0;
    };
    
    // Constructs a Report object with the given report information
    Report(long playerId, long reporterId, int reportNum, int state, int type, 
              char *shortDesc, char *longDesc, room_vnum orgRoom, 
              time_t orgTime, ReleaseInfo orgRel) {
      setPlayerId(playerId);
      setReporterId(reporterId);
      setReportNum(reportNum);
      setState(state);
      setType(type);
      setShortDescription(shortDesc);
      setLongDescription(longDesc);
      setChangeDescription("");
      setOrgRoom(orgRoom);
      setOrgTime(orgTime);
      setRelease(orgRel);

      mode = -1;
      modified = 0;
    };

    // set functions
    void setReporterId(long playerId) { Report::reporterId = playerId; };
    void setPlayerId(long playerId) { Report::playerId = playerId; };
    void setReportNum(int reportNum) { Report::reportNum = reportNum; };
    void setState(int state) { Report::reportState = state; };
    void setType(int type) { Report::type = type; };
    void setOrgRoom(room_vnum orgRoom) { Report::orgRoom = orgRoom; };
    void setOrgTime(time_t orgTime) { Report::orgTime = orgTime; };
    void setRelease(ReleaseInfo release) { 
      orgRelease = ReleaseInfo(release.getMajor(), release.getBranch(), 
                      release.getMinor(), release.getTag(), 
                      release.isCvsUpToDate(), release.getDate());
    }; 
    void setShortDescription(char *shortDesc) { 
      if (*shortDesc) {
        strncpy(shortDescription, shortDesc, REPORT_SHORTDESC_LENGTH); 
	shortDescription[REPORT_SHORTDESC_LENGTH-1] = '\0';
      } else {
        shortDescription[0] = '\0';
      }
    };
    void setLongDescription(char *longDesc) { 
      if (*longDesc) {
        strncpy(longDescription, longDesc, REPORT_LONGDESC_LENGTH); 
	longDescription[REPORT_LONGDESC_LENGTH-1] = '\0';
      } else {
        longDescription[0] = '\0';
      }
    };
    void setChangeDescription(char *changeDesc) {
      if (*changeDesc) {
        strncpy(changeDescription, changeDesc, REPORT_SHORTDESC_LENGTH);
	changeDescription[REPORT_SHORTDESC_LENGTH-1] = '\0';
      } else {
        changeDescription[0] = '\0';
      }
    }

    // get functions
    long getReporterId() { return Report::reporterId; };
    long getPlayerId() { return Report::playerId; };
    int getReportNum() { return Report::reportNum; };
    int getState() { return Report::reportState; };
    int getType() { return Report::type; };
    room_vnum getOrgRoom() { return Report::orgRoom; };
    time_t getOrgTime() { return Report::orgTime; };
    ReleaseInfo getOrgRelease() { return Report::orgRelease; };
    char *getShortDescription() { return Report::shortDescription; };
    char *getLongDescription() { return Report::longDescription; };
    char *getChangeDescription() { return Report::changeDescription; };

    int getReportType();
    bool isRestricted(struct descriptor_data *d, int mode);
    bool isViewable(struct descriptor_data *d);
    bool isEditable(struct descriptor_data *d, int mode);


    // ourput related functions
    void printDetails(struct descriptor_data *d, char *writeto);
    void printBriefDescription(struct descriptor_data *d, char *writeto);
    void mailChanges();

    // list related functions
    void addChange(ReportChange reportChange); 
    bool removeChange(int index);

    // File related functions
    struct report_file_elem toFileElem();

    ~Report() {};
};


class ReportList {
  private:
    list<Report> reports;  // List of all the reports
    int topReportNum;
    int topChangeNum;
    ifstream inReportFile;
//    ofstream outReportFile;
    ifstream inChangeFile;
//    ofstream outChangeFile;
    
  public:
    // Constructor 
    ReportList() { 
      topReportNum = 0; 
      topChangeNum = 0;
    };       

    void setTopReportNum(int num) { ReportList::topReportNum = num; };
    int getTopReportNum() { return ReportList::topReportNum; };

    void setReport(struct descriptor_data *d);

    // list related functions
    int addReport(struct descriptor_data *d, Report *report); 
    bool removeReport(struct descriptor_data *d, int number, int mode); 
    bool copyReport(struct descriptor_data *d, int index, int mode);

    void listReports(struct descriptor_data *d, int mode, char *arg);
    bool printReport(struct descriptor_data *d, int number, int mode);

    void getNoneExistMesg(int mode, char *writeto);
    void getOutOfRangeMesg(int mode, char *writeto);
    void getNotFoundMesg(int mode, char *writeto); 
    void getRestrictedMesg(int mode, char *writeto); 

    // Reporting functions (for adding/editing)
    void dispAddMenu(struct descriptor_data *d);
    void dispEditMenu(struct descriptor_data *d);
    void dispStateMenu(struct descriptor_data *d);
    void dispTypeMenu(struct descriptor_data *d);

    // Notification functions
    void notify(int number); 
    
    // file related functions
    void loadFile();
    void addReport(struct report_file_elem report);
    void addChange(struct reportchange_file_elem reportchange);
    void writeReport(struct report_file_elem report);
    void writeChange(struct reportchange_file_elem changereport);
    void load();
    void save();

    ~ReportList() {};      // Destructor - nothing to do
};

struct report_file_elem {
  long reporterId;            // Originator of report
  long playerId;              // Person Responsible for report
  int reportNum;              // Report Number
  int state;                  // Current status on report - see REPORT_STATE_XXX
  int type;                   // Type of report reported = see REPORT_TYPE_XXX
  char shortDescription[REPORT_SHORTDESC_LENGTH];  // Short desc about report
  char longDescription[REPORT_LONGDESC_LENGTH];    // Long desc about report
  room_vnum orgRoom;          // The room the player reported the report in
  time_t orgTime;             // Time the report was reported  
  short relMajor;       // Major release number
  short relBranch;      // Branch release number
  short relMinor;       // Minor release number
  bool relCvsUpToDate;  // Release CVS up to date
};

struct reportchange_file_elem {
  int reportNum;        // report this change belongs to
  int changeNum;
  long playerId;        // The player which made this change
  int fromState;        // The old state
  int toState;          // The new state
  int fromType;         // The old type 
  int toType;           // The new type
  time_t changeTime;    // Time of change
  short relMajor;       // Major release number
  short relBranch;      // Branch release number
  short relMinor;       // Minor release number
  bool relCvsUpToDate;  // Release CVS up to date
  char changeDescription[REPORT_SHORTDESC_LENGTH]; // Description of the change
};
