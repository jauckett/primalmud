// Corpses - DM (should include proper header :)

#include <list>
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "corpses.h"

extern struct index_data *obj_index;	/* index table for object file	 */
extern room_rnum r_mortal_start_room;
extern void Crash_restore_weight(struct obj_data * obj);

ofstream outCorpseFile;
ifstream inCorpseFile;

/**
 * Constructs a CorpseData object deserializing the data store in file.
 * Ok, its not deserializing - we just read from the file and deposit the
 * corpses in the vnums of the room in which they were in
 */
CorpseData::CorpseData() {
  // DM: TODO - deserialize
  // Must set memoryAddress for each Corpse when this object is initialised (at
  // boot up) otherwise we'll be duping corpses when modifying them.
  // - Think this TODO is done ...
} 
  

/**
 * Loads the corpses from file into corpseData object.
 *
 * Returns the number of corpses loaded.
 */
int CorpseData::load() {
  class Corpse currCorpse;
  int numLoaded = 0;

  inCorpseFile.open(CORPSE_FILENAME, std::ios::in);

  if (!inCorpseFile) {
    basic_mud_log("SYSERR: could not open corpse file %s.", CORPSE_FILENAME);
    return (0);
  }

  // Here we only read for new corpses, once we have the corpse identifier (C),
  // we create a Corpse object and at it to our list. In the construction of
  // the Corpse object, the file is passed further for objects, and returns
  // back to this loop with the inCorpseFile stream position at either 
  // ios::eof or 'C'
  //
  // eg. passing "C <pid> <roomvnum> <gold>" 

  while (!inCorpseFile.eof()) {
    switch (inCorpseFile.peek()) {
      case 'C':
        inCorpseFile.ignore(2);

        currCorpse = Corpse();
        currCorpse.read();
        corpses.push_back(currCorpse);
	numLoaded++;
        break;

      default:
        return (numLoaded);
    }
  }
  inCorpseFile.close();
  extractOriginalCorpses(TRUE);

  return (numLoaded);
}

//
void CorpseData::extractOriginalCorpses(int retainIDs) {
  std::list<Corpse>::iterator liter; // Iterator for looping over list elements
  struct obj_data *corpse;

  for (liter = corpses.begin(); liter != corpses.end(); liter++) {
    corpse = liter->toObj();

    // setMemoryAddress should be set TRUE when initially loading the corpses
    // at boot time.
    liter->setMemory((long)corpse);
    //liter->setMemory((int)corpse);
    
    // When a char modifies a corpse (removes an object from it), if the idnum
    // is set (obj val 2), the addCorpse function is called, hence retainIDs
    // should be set TRUE when only modifying the original corpse 
    // (otherwise multiple corpses will be added to the CorpseData object)
    if (!retainIDs) {
      GET_OBJ_VAL(corpse, 2) = 0; 
    }

    if (liter->getVRoom() != NOWHERE) {
      obj_to_room(corpse, real_room(liter->getVRoom()));
    } else {
      obj_to_room(corpse, r_mortal_start_room);
    }
  }
}

void CorpseData::extractCorpses(room_vnum loadTo, int retainIDs) {
  std::list<Corpse>::iterator liter; // Iterator for looping over list elements
  struct obj_data *corpse;

  room_rnum to_room = real_room(loadTo);

  if (to_room == -1)
    return;
  
  for (liter = corpses.begin(); liter != corpses.end(); liter++)
  {
    corpse = liter->toObj();
    // When a char modifies a corpse (removes an object from it), if the idnum
    // is set (obj val 2), the addCorpse function is called, hence retainIDs
    // should be set TRUE when only modifying the original corpse 
    // (otherwise multiple corpses will be added to the CorpseData object)
    if (!retainIDs)
      GET_OBJ_VAL(corpse, 2) = 0; 
    obj_to_room(corpse, to_room); 
  }
}

/**
 * Adds a corpse to the corpse list. If an item for the corpse already exists
 * it is removed before adding the corpse information. 
 *
 * If the corpse does not exist in the existing corpse list, then weight
 * is used to set the base corpse weight, otherwise the base weight is 
 * set via the existing corpse.
 */
void CorpseData::addCorpse(struct obj_data *corpse, room_vnum inVRoom, 
                          int weight) {
  int corpseWeight;

  std::list<Corpse>::iterator liter; // Iterator for looping over list elements

  for (liter = corpses.begin(); liter != corpses.end(); liter++) {
    if (liter->memoryEqual(corpse)) {
      // Get the base corpse weight from the existing corpse
      corpseWeight = liter->getWeight(); 
      corpses.erase(liter);
      corpses.push_back(
          Corpse(corpse, inVRoom, GET_CORPSEID(corpse), corpseWeight));
      write();
      return;
    }
  }
  corpses.push_back(Corpse(corpse, inVRoom, GET_CORPSEID(corpse), weight));
  write();
}

/**
 * Removes a corpse from the corpse list if it exists.
 */
void CorpseData::removeCorpse(struct obj_data *corpse) {

  std::list<Corpse>::iterator liter; // Iterator for looping over list elements

  for (liter = corpses.begin(); liter != corpses.end(); liter++) {
    if (liter->memoryEqual(corpse)) {
      corpses.erase(liter);
      write();
      return;
    }
  }
}

/**
 * Removes all corpses from the list.
 */
void CorpseData::removeAllCorpses(void) {

  std::list<Corpse>::iterator liter; // Iterator for looping over list elements

  for (liter = corpses.begin(); liter != corpses.end(); liter++) {
    //corpses.erase(liter);
  }
  write();
}

/**
 * Destructor, clean up.
 */
CorpseData::~CorpseData() {
  // cleanup
}

/**
 * Serialize function to store the current information encapsulated in this
 * object to file - ok no simple serialize function, use the write functions.
 */ 
void CorpseData::write() {

  // open file for writing
  outCorpseFile.open(CORPSE_FILENAME, std::ios::out);

  // check it opened
  if (!outCorpseFile) {
    basic_mud_log("SYSERR: corpse file: %s could not be opened for writing",
                CORPSE_FILENAME);
    return;
  }
  
  std::list<Corpse>::iterator liter;

  for (liter = corpses.begin(); liter != corpses.end(); liter++) {
    liter->write();
  }
  outCorpseFile.close();
}


/**
 * Write the information to file encapsulated by this object.
 */
void Corpse::write() {
  
  // Write corpse data needed - vnum of room, player id, gold, obj_flags
  outCorpseFile << "C " << playerId << " " << in_room << " " << gold << " " << weight << "\n";
  // Artus> I'm going to try and get g++3.2 compatibility..
  // outCorpseFile.write((struct obj_flag_data *)&obj_flags, sizeof(obj_flags));
  outCorpseFile.write(reinterpret_cast <char *>(&obj_flags), sizeof(obj_flags)); 
  outCorpseFile.flush();

  std::list<Contain>::iterator liter; // Iterator for looping over list elements

  for (liter = contains.begin(); liter != contains.end(); liter++) {
    liter->write();
  } 
  outCorpseFile << "\n";
}

/**
 * Constructs a Corpse object with the basic attributes, but does not touch
 * objects.
 */
void Corpse::initialise(long playerId, int inVRoom, int gold, int weight, 
                           struct obj_flag_data obj_flags) {
  Corpse::gold = gold;
  Corpse::playerId = playerId;
  Corpse::in_room = inVRoom;
  Corpse::weight = weight;

  Corpse::memoryAddress = 0;
  if ((get_name_by_id(playerId)) == NULL) {
    basic_mud_log("SYSERR: initialising Corpse object: pid = %ld", playerId);
    return;
  }

  Corpse::name = str_dup("corpse pcorpse");
  
  sprintf(buf, "The corpse of %s is lying here.", 
                get_name_by_id(playerId));
  Corpse::description = str_dup(buf);

  sprintf(buf, "the corpse of %s", 
                get_name_by_id(playerId));
  Corpse::short_description = str_dup(buf);

  // Copy values
  Corpse::obj_flags.value[0] = obj_flags.value[0]; 
  Corpse::obj_flags.value[1] = obj_flags.value[1]; 
  Corpse::obj_flags.value[2] = obj_flags.value[2]; 
  Corpse::obj_flags.value[3] = obj_flags.value[3]; 

  // Copy obj_flag's ...
  Corpse::obj_flags.type_flag = obj_flags.type_flag;
  Corpse::obj_flags.level = obj_flags.level;
  Corpse::obj_flags.wear_flags = obj_flags.wear_flags;
  Corpse::obj_flags.extra_flags = obj_flags.extra_flags;
  Corpse::obj_flags.level_flags = obj_flags.level_flags;
  Corpse::obj_flags.weight = obj_flags.weight;
  Corpse::obj_flags.cost = obj_flags.cost;
  Corpse::obj_flags.cost_per_day = obj_flags.cost_per_day;
  Corpse::obj_flags.timer = obj_flags.timer;
  Corpse::obj_flags.bitvector = obj_flags.bitvector;

}

/**
 *
 */
Corpse::Corpse() {
  Corpse::playerId = 0;
  Corpse::in_room = 0;
  Corpse::gold = 0;
}

void Corpse::read() {

  class Contain currContain;
  
  inCorpseFile >> Corpse::playerId >> Corpse::in_room >> Corpse::gold
               >> Corpse::weight;

  // ignore newline after first line 
  inCorpseFile.ignore();

  // Artus> g++ 2.3.2 compatibility
  // inCorpseFile.read((struct obj_flag_data *)&obj_flags, sizeof(obj_flags));
  inCorpseFile.read(reinterpret_cast <char *>(&obj_flags), sizeof(obj_flags));

  // ignore newline after obj_flags
  inCorpseFile.ignore();

  initialise(playerId, in_room, gold, weight, obj_flags);

  // Look at next character in file - if its an 'O' we have objects in corpse, 
  // otherwise we are expecting either a 'C' for a new corpse object, or eof.
  switch (inCorpseFile.peek()) {
    case 'O':
      while (inCorpseFile.peek() == 'O') {
        currContain = Contain();
        currContain.read(playerId);
        contains.push_back(currContain);  
      }
      break;

    // "C " or EOF
    default:
      break;
  } 
}

/**
 * Constructs a Corpse object copying all the "static" data of the corpse.
 */
Corpse::Corpse(struct obj_data *corpse, room_vnum inVRoom, long playerId,
                        int weight) {
  struct obj_data *o;

  initialise(playerId, inVRoom, 0, weight, corpse->obj_flags);

  Corpse::memoryAddress = (long)corpse;

  // Store the contents of the corpse (each "obj's" vnum is encapsulated in
  // a Content object - which also encapsulates the contents of "o")
  for (o = corpse->contains; o != NULL; o = o->next_content) {

    // check for gold, we dont add a gold "obj"
    if (GET_OBJ_TYPE(o) == ITEM_MONEY) {
      gold = GET_OBJ_VAL(o, 0);
    } else if (GET_OBJ_VNUM(o) != NOTHING) {
      contains.push_back(Contain(o, playerId));
    } else {
      sprintf(buf, "SYSERR: %s containing obj (%s) with no VNUM", 
		      corpse->short_description, o->name);
      mudlog(buf, NRM, LVL_GOD, TRUE);
    }
  }
}

/**
 * Converts this "static" Corpse information into an "obj" and loads it to
 * the room where it was previously saved. (Basically a copy of the make_corpse
 * function)
 */
struct obj_data *Corpse::toObj()
{
  struct obj_data *money = NULL;
  struct obj_data *corpse = create_obj();

  if (gold > 0)
    money = create_money(gold); 

  if (corpse == NULL)
    return NULL;
  
  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;
  corpse->name = str_dup(name);
  corpse->description = str_dup(description);
  corpse->short_description = str_dup(short_description);

  // Copy values
  corpse->obj_flags.value[0] = obj_flags.value[0];  
  corpse->obj_flags.value[1] = obj_flags.value[1]; 
  corpse->obj_flags.value[2] = obj_flags.value[2]; 
  corpse->obj_flags.value[3] = obj_flags.value[3]; 

  // Copy obj_flag's ...
  corpse->obj_flags.type_flag = obj_flags.type_flag; 
  corpse->obj_flags.level = obj_flags.level;  
  corpse->obj_flags.wear_flags = obj_flags.wear_flags;
  corpse->obj_flags.extra_flags = obj_flags.extra_flags;
  corpse->obj_flags.level_flags = obj_flags.level_flags;
  corpse->obj_flags.weight = obj_flags.weight;
  corpse->obj_flags.cost = obj_flags.cost; 
  corpse->obj_flags.cost_per_day = obj_flags.cost_per_day;
  corpse->obj_flags.timer = obj_flags.timer; 
  corpse->obj_flags.bitvector = obj_flags.bitvector;

  std::list<Contain>::iterator liter; // Iterator for looping over list elements

  for (liter = contains.begin(); liter != contains.end(); liter++)
  {
    struct obj_data *obj;
    try
    {
      obj = liter->createObject();
      obj_to_obj(obj, corpse);
    } catch (Corpse::NonExistantObject) {
      sprintf(buf, 
          "SYSERR: obj vnum %d could not be loaded to corpse of %s", 
          liter->getVNum(), get_name_by_id(playerId)); 
      basic_mud_log(buf);
    }
  } 

  // Add gold to corpse
  if (money != NULL)
    obj_to_obj(money, corpse);

  GET_OBJ_WEIGHT(corpse) = Corpse::weight;
  Crash_restore_weight(corpse);
  return corpse;
}

Contain::Contain() {
  Contain::itemNumber = 0;
  Contain::playerId = playerId;
}

void Contain::read(long playerId) {
  class Contain nextContain;

  // done - end of container, just return to previous recursive all
  if (inCorpseFile.peek() == '}') {
    inCorpseFile.ignore(2);
    return;
  }

  Contain::playerId = playerId;
  
  // skip over the leading "O "
  inCorpseFile.ignore(2);

  inCorpseFile >> Contain::itemNumber;

  // ignore space before obj_file_elem data
  inCorpseFile.ignore();

  // Artus> g++ 2.3.2 compatibility
  // inCorpseFile.read((struct obj_file_elem *)&save, sizeof(save)); 
  inCorpseFile.read(reinterpret_cast <char*>(&save), sizeof(save)); 

  // ignore the newline, or leading space for '{"
  inCorpseFile.ignore();

  switch (inCorpseFile.peek()) {

    // This item is a container
    case '{':
      inCorpseFile.ignore(2);

      // Recursive call to add objs to this container
      while (inCorpseFile.peek() == 'O') {
        nextContain = Contain();
        nextContain.read(playerId);
        contains.push_back(nextContain);  
      }
      
      // ignore "{\n"
      inCorpseFile.ignore(2);
      break;

    // This was last item in a container
    case '}':
      // our work is done
      //inCorpseFile.ignore(2);
      return;

    case 'O':
      // our work is done we'll drop out of this function and be called from
      // the Corpse constructor...
      return;

    default:
      //basic_mud_log("SYSERR: corpse file expecting \'{\', \'}\' or \'O\' after obj %d",
      //          Contain::itemNumber);
      //exit(1);
      break;
  } 
}

/**
 * Constructs a Contain object initialising the vnum and recursively
 * adding the "objs" contained within this "obj" to the contains list.
 */
Contain::Contain(struct obj_data *obj, long playerId) {
  struct obj_data *o;
  int j;
	
  Contain::playerId = playerId;
  Contain::itemNumber = GET_OBJ_VNUM(obj);

  save.value[0] = GET_OBJ_VAL(obj, 0);
  save.value[1] = GET_OBJ_VAL(obj, 1);
  save.value[2] = GET_OBJ_VAL(obj, 2);
  save.value[3] = GET_OBJ_VAL(obj, 3);
  save.extra_flags = GET_OBJ_EXTRA(obj);
  save.weight = GET_OBJ_WEIGHT(obj);
  save.timer = GET_OBJ_TIMER(obj);
  save.bitvector = obj->obj_flags.bitvector;
  save.damage = obj->damage;
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    save.affected[j] = obj->affected[j];

  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
    // Add all the "objs" that this "obj" containts to the list
    for (o = obj->contains; o != NULL; o = o->next_content) {
      Contain::contains.push_back(Contain(o, playerId));
    }
  }
}

/**
 * Creates and returns a reference to the "obj" corresponding to itemNumber.
 * Throws NonExistantObject if object could not be created. 
 */
struct obj_data *Contain::createObject() {
  struct obj_data *obj = NULL;
  int j;

  obj = read_object(itemNumber, VIRTUAL);
  
  if (obj != NULL) {
  
    // Now copy the saved obj data
    GET_OBJ_VAL(obj, 0) = save.value[0];
    GET_OBJ_VAL(obj, 1) = save.value[1];
    GET_OBJ_VAL(obj, 2) = save.value[2];
    GET_OBJ_VAL(obj, 3) = save.value[3];

    GET_OBJ_EXTRA(obj) = save.extra_flags;
    GET_OBJ_WEIGHT(obj) = save.weight;
    GET_OBJ_TIMER(obj) = save.timer;
    obj->obj_flags.bitvector = save.bitvector;
    obj->damage = save.damage;
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      obj->affected[j] = save.affected[j];

    std::list<Contain>::iterator liter; // Iterator for looping over list elements

    // Create the items within this object ...
    for (liter = contains.begin(); liter != contains.end(); liter++) {
      struct obj_data *childObj;
      try {
        childObj = liter->createObject();
        obj_to_obj(childObj, obj);
      } catch (Contain::NonExistantObject) { 
	sprintf(buf, 
            "SYSERR: obj vnum %d could not be loaded to object %d", 
	    liter->getVNum(), itemNumber); 
	basic_mud_log(buf);
      }	      
    }
    return obj;
  }
  throw NonExistantObject();
}

void Contain::write() {
  // vnum of obj
  outCorpseFile << "\nO " << itemNumber << " ";

  // obj save info
  // Artus> g++ 2.3.2..
  // outCorpseFile.write((struct obj_file_elem *)&save, sizeof(save)); 
  outCorpseFile.write(reinterpret_cast <char *>(&save), sizeof(save)); 
  outCorpseFile.flush();

  std::list<Contain>::iterator liter;

  // check for items inside this item
  if (contains.begin() == contains.end()) {
    //outCorpseFile << "\n";
    return;
  }

  // recursively output our objs 
  outCorpseFile << " {";
  for (liter = contains.begin(); liter != contains.end(); liter++) {
    liter->write();  
  }
  outCorpseFile << " }";
}
