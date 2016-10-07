#include <list>
#include <iostream>
#include <fstream>

using std::ifstream;
using std::ofstream;

// filename corpseData is stored to
#define CORPSE_FILENAME LIB_ETC"corpses" 

/**
 * This class is used to encapsulate the virtual number of an "obj" and
 * the "objs" that are contained within this "obj".
 */
class Contain {
  public:
    // Constructor - just initialise defaults
    Contain();

    // Constructor ...
    Contain(struct obj_data *obj, long playerId); 

    // Exception thrown when the "obj" could not be created from vnum itemNumber
    class NonExistantObject {};

    // Creates an "obj" based on the data encapsuled within this object
    struct obj_data *createObject();

    // returns the list of "objs" that this "obj" contains
    std::list<Contain> getContents() { return contains; } 

    // returns vnum of this "obj"
    obj_vnum getVNum() { return itemNumber; }               

    // Read the data associate with this "obj" to file
    void read(long playerId);

    // Write the data associate with this "obj" to file
    void write();

    // Destructor ...
    ~Contain() {}                       

  private:
    // idnum of owner 
    long playerId;

    // obj save data
    struct obj_file_elem save;

    // vnum of this "obj"	
    obj_vnum itemNumber;     

    // list of "objs" that this "obj" containts
    std::list<class Contain> contains;  
    
    // reads the data associate with this object from file
    void read();

};


/**
 * This class encapsulate the "static" data associated with a corpse including
 * the "objs" that the corpse contains. The class constructor copies the
 * "static" data contained within a corpse, and the toObj() function converts
 * this object to an "obj".
 */
class Corpse {
  public:
    // Default Constructor - just set default vals
    Corpse();

    // Constructor ...
    Corpse(struct obj_data *corpse, room_vnum inVRoom, long playerid, 
                    int weight);

    // Converts this object to an "obj"
    struct obj_data *toObj();

    // Exception thrown when the "obj" could not be created from vnum itemNumber
    class NonExistantObject {};

    // Returns the virtual number that the corpse was saved in
    room_vnum getVRoom(void) {
      return in_room;
    }

    // returns true if memory addresses are equal
    bool memoryEqual(struct obj_data *corpse) { 
      return ((int)corpse == memoryAddress);
    }

    void setMemory(int memAddress) {
      memoryAddress = memAddress;
    }

    int getWeight() {
      return weight;
    }

    // Writes the data to file associated with this corpse
    void write(); 

    // Reads the data from file associated with this corpse
    void read();

    // Desctructor ...
    ~Corpse() {}

  private:

    void initialise(long playerId, int inVRoom, int gold, int weight, 
                       struct obj_flag_data obj_flags);

    // The address of the corpse obj in memory
    int memoryAddress;

    // The virtual number of the room where the corpse is. If in_room is
    // NOWHERE, the corpse will be loaded to CORPSE_LOAD_ROOM.
    room_vnum in_room;      

    // Player idnum
    long playerId;

    // The amount of gold contained in the corpse
    int gold;

    // The base weight of the corpse
    int weight;
    
    // Name of "obj" - corpse
    char *name;

    // Description of "obj" - The corpse of <name> is lying here
    char *description;
    
    // Short Description of "obj" - the corpse of <name>
    char *short_description;

    // "Object" information 
    struct obj_flag_data obj_flags;

    // Wear it is worn - ITEM_WEAR_TAKE
    sh_int worn_on;                

    // The max durability of the corpse
    sh_int max_damage;       

    // State of "obj"
    sh_int damage;     

    // A list to encapsulate the "objs" contained within the corpse.
    // This is a list of the "root" items in the corpse, each of these items
    // includes the virtual number of the "obj". Each item may also be
    // a container, the contents of each item are stored within a list in the
    // Contain object.
    std::list<Contain> contains;  
};

/**
 * This class encapsulate the data associated with all the player corpses
 * currently active in the game. It contains a list of Corpse's, and functions
 * to add and remove corpse "objs". Serialize and deserialize functions
 * are used to save (when a change is made to a corpse) and load (at boot time)
 * to ensure that no corpses are lost in the event of a crash.
 */ 
class CorpseData {
  public:
    CorpseData();
    int load();
    void addCorpse(struct obj_data *corpse, room_vnum inVRoom, int weight);
    void removeCorpse(struct obj_data *corpse);
    void removeAllCorpses(void);
    void extractOriginalCorpses(int retainIDs);
    void extractCorpses(room_vnum, int retainIDs);
    ~CorpseData();
  private:
    void write();
    std::list<Corpse> corpses;
};
