/*
***********************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern struct room_data *world;
extern int top_of_world;
extern int mini_mud;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname))
{
  if (real_mobile(mob) >= 0)
    mob_index[real_mobile(mob)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant mob #%d",
	    mob);
    log(buf);
  }
}

void ASSIGNOBJ(int obj, SPECIAL(fname))
{
  if (real_object(obj) >= 0)
    obj_index[real_object(obj)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant obj #%d",
	    obj);
    log(buf);
  }
}

void ASSIGNROOM(int room, SPECIAL(fname))
{
  if (real_room(room) >= 0)
    world[real_room(room)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant rm. #%d",
	    room);
    log(buf);
  }
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  SPECIAL(throw);
  SPECIAL(postmaster);
  SPECIAL(cityguard);
  SPECIAL(receptionist);
  SPECIAL(cryogenicist);
  SPECIAL(guild_guard);
  SPECIAL(trainer);
  SPECIAL(guild);
  SPECIAL(puff);
  SPECIAL(fido);
  SPECIAL(janitor);
  SPECIAL(mayor);
  SPECIAL(snake);
  SPECIAL(thief);
  SPECIAL(magic_user);
  SPECIAL(citizen);
  SPECIAL(oompa);
  SPECIAL(tin_man);
  SPECIAL(scarecrow);
  SPECIAL(c_lion);
  SPECIAL(sleepy);
  SPECIAL(sneazy);
  SPECIAL(grumpy);
  SPECIAL(bashful);
  SPECIAL(lazy);
  SPECIAL(doc);
  SPECIAL(wind_summon);
  SPECIAL(dopy);
  SPECIAL(giant);
  SPECIAL(construction_worker);
  SPECIAL(recep_guard);
  SPECIAL(cleric);
  SPECIAL(cleric2);
  SPECIAL(warrior);
  SPECIAL(warrior1);
  SPECIAL(head_druid);
  SPECIAL(quest_sentry);
  SPECIAL(corridor_guard);
  SPECIAL(acid_breath);
  SPECIAL(regen); 
  SPECIAL(constrictor);
  SPECIAL(bounty_hunter);
  SPECIAL(assasin);
  SPECIAL(elec_shock);
  SPECIAL(virus);
  SPECIAL(bacteria);
  SPECIAL(blood_sucker);
  SPECIAL(Jabba);
  SPECIAL(blink_demon);
  SPECIAL(beholder);
  SPECIAL(self_destruct);
  SPECIAL(disposable);
  SPECIAL(zombie); /* zombie for anim dead spell */
  SPECIAL(clone);  /* clone for clone spell      */
  SPECIAL(vampire);  /* vampire and bat that turns into vampire proc */
  SPECIAL(werewolf); /* werewolf of haven proc for both changer and wolf */
  SPECIAL(school); /* for the piranah */
  SPECIAL(trojan);
  SPECIAL(drainer); 
  SPECIAL(fire_breath);
  SPECIAL(gaze_npc);
  SPECIAL(banish);
  SPECIAL(phoenix);
  SPECIAL(aphrodite);
  SPECIAL(richard_garfield);
  SPECIAL(dragon);
  SPECIAL(santa);
  SPECIAL(caroller);
  SPECIAL(easter_bunny);
  SPECIAL(ice_breath);
  SPECIAL(peacekeeper);

  SPECIAL(delenn); 	/* Delenn*/
  SPECIAL(alien_voice);
  SPECIAL(alien_voice_echo);

  // The island forever - Talisman's spec_procs
  SPECIAL(fate);
  SPECIAL(titan);
  SPECIAL(anxiousleader);
  SPECIAL(packmember);
  SPECIAL(packleader);
  SPECIAL(avenger);
  SPECIAL(insanity);
  SPECIAL(arrogance);
  SPECIAL(playerhunter);

  SPECIAL(rose_club_guard);
  SPECIAL(avengers_guard);
  SPECIAL(guardian_guard);
  SPECIAL(jedi_guard);
  SPECIAL(uss_guard);

  SPECIAL(healer);

  SPECIAL(high_dice);
  SPECIAL(craps);
  SPECIAL(triples);
  SPECIAL(seven);
  SPECIAL(roulette);
  SPECIAL(high_dice1);
  SPECIAL(craps1);
  SPECIAL(triples1);
  SPECIAL(seven1);
  SPECIAL(roulette1);

/* clan guards */
  ASSIGNMOB(117,  rose_club_guard);
  ASSIGNMOB(118,  avengers_guard);
  ASSIGNMOB(119,  guardian_guard);
  ASSIGNMOB(120,  jedi_guard);
  ASSIGNMOB(121,  uss_guard);

/* healer */
  ASSIGNMOB(122,  healer);

/* Gambling House */
/* commented out while objects are non-existant - bill 
  ASSIGNMOB(13800, high_dice);
  ASSIGNMOB(13801, craps);  
  ASSIGNMOB(13802, triples); 
  ASSIGNMOB(13803, seven); 
  ASSIGNMOB(13804, roulette);
  ASSIGNMOB(13807, high_dice1);
  ASSIGNMOB(13808, craps1);  
  ASSIGNMOB(13809, triples1); 
  ASSIGNMOB(13810, seven1); 
  ASSIGNMOB(13811, roulette1);       */

  ASSIGNMOB(22303, easter_bunny); /* guess... */
  ASSIGNMOB(22302, santa); /* santa claus */
  ASSIGNMOB(22303, caroller); /* christmas caroller */
  ASSIGNMOB(1505, vampire); /* bat */
  ASSIGNMOB(1506, vampire); /* vampire */
  ASSIGNMOB(115, werewolf); /* mob that changes into.. */
  ASSIGNMOB(116, werewolf); /* ...the werewolf         */
  ASSIGNMOB(22300, clone);
  ASSIGNMOB(22301, zombie); /* anim dead zombie */

  ASSIGNMOB(13222, receptionist); 
  ASSIGNMOB(13205, peacekeeper);  /* sheriff and deputy, city of Hope, westworld */
  ASSIGNMOB(13206, peacekeeper);

  ASSIGNMOB(2536, richard_garfield); /* richard garfield for magic zone */
  ASSIGNMOB(2473, dragon);

  ASSIGNMOB(909,bounty_hunter);
  ASSIGNMOB(10002,assasin);
  ASSIGNMOB(9060, corridor_guard);
  ASSIGNMOB(5, receptionist);
  ASSIGNMOB(910, quest_sentry);
  ASSIGNMOB(108, citizen);
  ASSIGNMOB(113, construction_worker);
  ASSIGNMOB(110, janitor);
  ASSIGNMOB(20, cityguard);
  ASSIGNMOB(21, postmaster);  
  ASSIGNMOB(10, guild);
  ASSIGNMOB(11, guild);
  ASSIGNMOB(12, guild);
  ASSIGNMOB(13, guild);
  ASSIGNMOB(111, virus);
  ASSIGNMOB(112, bacteria);
  ASSIGNMOB(114, trainer);

  /* luna city */
  ASSIGNMOB(22000, janitor);
  ASSIGNMOB(22005, cityguard);
  ASSIGNMOB(22006, receptionist);
  ASSIGNMOB(22017, guild);
  ASSIGNMOB(22018, guild);
  ASSIGNMOB(22019, guild);
  /* danger island */
  ASSIGNMOB(8803, acid_breath);

  /* moon surface */
  ASSIGNMOB(23000, elec_shock);
  ASSIGNMOB(23003, elec_shock);
  ASSIGNMOB(23002, elec_shock);
  ASSIGNMOB(23001, regen);             /* prospector */
  /* vampire */
  ASSIGNMOB(1014, blood_sucker);

  /* helpers summoned by clerics */ 
  ASSIGNMOB(1016 , magic_user); 
  ASSIGNMOB(1017 , magic_user);
  ASSIGNMOB(1018 , magic_user);
  ASSIGNMOB(1019 , magic_user);
  ASSIGNMOB(1020 , magic_user);
  ASSIGNMOB(1021 , magic_user);

  ASSIGNMOB(900, magic_user);     /* Gods */ 
  ASSIGNMOB(901, magic_user);
  ASSIGNMOB(902, magic_user);

  /* Rushmoor swamp */
  ASSIGNMOB(8504, acid_breath);         /* wasp */
  ASSIGNMOB(8506, magic_user);    /* wraith */
  ASSIGNMOB(8508, cleric);        /* night hag */
  ASSIGNMOB(8505, regen);         /* mudman */
  ASSIGNMOB(8509, magic_user);    /* Ningauble wizard */
  ASSIGNMOB(8507, snake);         /* crawler */
  ASSIGNMOB(8503, elec_shock);    /* eel */

/*JA : some spec procs for the new forest zone till I work out
       some better ones */
  /* LHANKWOOD FOREST */
  ASSIGNMOB(8301, magic_user);    /* grand high elf */
  ASSIGNMOB(8305, magic_user);    /* high priest elf */
  ASSIGNMOB(8307, acid_breath);   /* green dragon */
  ASSIGNMOB(8310, magic_user);    /* lich */    
  ASSIGNMOB(8308, regen);         /* 2 headed troll */ 
  ASSIGNMOB(8312, constrictor);   /* python */

  /* mountains */
  ASSIGNMOB(5501, regen);         /* troll */
  ASSIGNMOB(5504, magic_user);    /* elemental */

  
  /* DESERT */
  ASSIGNMOB(807, magic_user);     /* sphinx */
  ASSIGNMOB(802, acid_breath);    /* purple worm */
  
  /* gaveyard */
  ASSIGNMOB(1605, snake);         /* small spider */
  ASSIGNMOB(1604, snake);         /* brown snake */
 
  /* toyheim */
  ASSIGNMOB(9009, cityguard);    /* GI Joe */

  /* tudor */
  ASSIGNMOB(8106, cleric);       /* bishop */
  ASSIGNMOB(8112, cityguard) ;   /* guard */
  ASSIGNMOB(8109, cleric);       /* monk */
  ASSIGNMOB(8100, magic_user);   /* queen */
  ASSIGNMOB(8101, cleric);       /* melchid */
  ASSIGNMOB(8117, warrior1);
  ASSIGNMOB(8118, warrior1);
  ASSIGNMOB(8119, warrior1);
  ASSIGNMOB(8120, gaze_npc);

  /* shadow keep */
  ASSIGNMOB(9504, magic_user);    /* shadowen */
  ASSIGNMOB(9505, magic_user);    /* larger shadowen */
  ASSIGNMOB(9514, regen);         /* disembodied */
  ASSIGNMOB(9512, regen);
  ASSIGNMOB(9513, regen);
  ASSIGNMOB(9517, acid_breath);   /* night crawler */
  ASSIGNMOB(9515, snake);         /* spider */
  ASSIGNMOB(9520, cleric);        /* priest */
  ASSIGNMOB(9506, cleric);        /* cleric */
  ASSIGNMOB(9507, magic_user);    /* mage */
  ASSIGNMOB(9530, magic_user);    /* big stone */
  ASSIGNMOB(9503, magic_user);    /* geat statue */
  /* ROME */
  ASSIGNMOB(12018, cityguard);
  ASSIGNMOB(12021, cityguard);
  ASSIGNMOB(12009, magic_user);
  ASSIGNMOB(12025, magic_user);

  ASSIGNMOB(12020, magic_user);
  ASSIGNMOB(12025, magic_user);
  ASSIGNMOB(12030, gaze_npc);
  ASSIGNMOB(12031, magic_user);
  ASSIGNMOB(12032, warrior1);
  ASSIGNMOB(12033, magic_user);

  ASSIGNMOB(7105, magic_user);      /* dragon */

  ASSIGNMOB(1012, snake);           /*ghast */
  ASSIGNMOB(702, snake);            /* snake in grass */

  /* toyheim */
  ASSIGNMOB(9012, magic_user);      /* chuckie */

  ASSIGNMOB(109, magic_user);   /* the trickster - low-level magii */
  ASSIGNMOB(501, recep_guard);  /* to un-rent players :) */
  ASSIGNMOB(1002, snake);       /* poison spiders in moathouse */
  ASSIGNMOB(1003, snake);       /* poison snake in moathouse   */

  /* island */
  /* ASSIGNMOB(8711, snake); */      /* fungi are snakes ie poison for now */
  ASSIGNMOB(8701, cleric);      /* druid */
  ASSIGNMOB(8702, magic_user);  /* eye of the deep */
  ASSIGNMOB(8705, magic_user);  /* chimera */
  ASSIGNMOB(8706, snake);       /* frogs */
  ASSIGNMOB(8707, magic_user);  /* astralo wolf */
  ASSIGNMOB(8711, self_destruct); /* fungus */

/* Hells Mountain Fortress 
  ASSIGNMOB(1100, magic_user);
  ASSIGNMOB(1101, warrior);
  ASSIGNMOB(1103, cleric2);
  ASSIGNMOB(1104, magic_user);  
  ASSIGNMOB(1105, cleric);  
  ASSIGNMOB(1108, cleric);
  ASSIGNMOB(1109, magic_user);
  ASSIGNMOB(1111, fire_breath);
  ASSIGNMOB(1112, acid_breath);  
  ASSIGNMOB(1113, ice_breath);
  ASSIGNMOB(1114, elec_shock);
  ASSIGNMOB(1115, acid_breath);
  ASSIGNMOB(1138, assasin);  
  ASSIGNMOB(1147, magic_user);
*/

  /* Moanders Domain */
   ASSIGNMOB(1800, snake);          /* Decay Deamon*/
   ASSIGNMOB(1801, magic_user);     /* Deamon of fire*/
   ASSIGNMOB(1806, magic_user);     /* Saurial Wizard*/
   ASSIGNMOB(1814, throw);          /* Moander */
   ASSIGNMOB(1813, regen);          /* Bone Guard */
   ASSIGNMOB(1815, blink_demon);    /* Blink Demon */
   ASSIGNMOB(1804, beholder);       /* Beholder */

  /* Fairytale Forrest! */
    ASSIGNMOB(4303, oompa);		/* ommpa lumpas */
    ASSIGNMOB(4355,tin_man);		/* Tin Man */
    ASSIGNMOB(4356,scarecrow);		/* Scarecrow */
    ASSIGNMOB(4357,c_lion);		/* Cowardly lion */
    ASSIGNMOB(4315,giant);		/* Jacks giant!! */
    ASSIGNMOB(4328,sleepy);
    ASSIGNMOB(4329,grumpy);
    ASSIGNMOB(4327,sneazy);
    ASSIGNMOB(4330,dopy);
    ASSIGNMOB(4333,lazy);
    ASSIGNMOB(4332,bashful);
    ASSIGNMOB(4331,doc);
    ASSIGNMOB(4319,throw);

/* Quest Mobs spec procs */

   ASSIGNMOB(4500, gaze_npc);          /* Slizzarr*/
   ASSIGNMOB(4501, wind_summon);       /* Wind Duke */
   ASSIGNMOB(4502, snake);		/* wolfspider */ 
  /* Future world spec procs */

  ASSIGNMOB(26004, Jabba);
  
  ASSIGNMOB(26007, magic_user);     /* Boba Fett */

	/* specs for the river zone */
	ASSIGNMOB(8402, school);					/* piranah */
	ASSIGNMOB(8400, constrictor);			/* serpent */
	ASSIGNMOB(8401, magic_user);			/* water element */
	ASSIGNMOB(8403, drainer);         /* vargoyle */

	/* specs for */
	ASSIGNMOB(1901, elec_shock);					/* anhkeg */
	ASSIGNMOB(1908, cleric);					/* night hag thing */

	/* Spiral*/
	ASSIGNMOB(2200, regen);		/* morkoth */
	ASSIGNMOB(2201, throw);		
	ASSIGNMOB(2202, blood_sucker);	/* lolth */
	ASSIGNMOB(2203, constrictor);	/* ogremoch */
	ASSIGNMOB(2205, regen);		/* trogoladyte */
	ASSIGNMOB(2206, regen);		/* slithering tracker */
        ASSIGNMOB(2210, fire_breath);   /* fire bat */
	ASSIGNMOB(1905, regen);         /* caveman guard */
	ASSIGNMOB(2216, warrior1);       /*Troglodyte guard*/
        ASSIGNMOB(2217, warrior1);
        ASSIGNMOB(2226, warrior1);
        ASSIGNMOB(2236, warrior1);
        ASSIGNMOB(2220, cleric2);       /* cardinal */
        ASSIGNMOB(2229, cleric2);	/* cardinal */
        ASSIGNMOB(2238, cleric2);	/* cardinal */
        ASSIGNMOB(2219, gaze_npc);	/* Priest */
        ASSIGNMOB(2228, gaze_npc);	/* priest */
        ASSIGNMOB(2237, gaze_npc);	/* priest */	
        ASSIGNMOB(2222, warrior1);      /* misstress */
	ASSIGNMOB(2221, magic_user);
        ASSIGNMOB(2239, magic_user);
        ASSIGNMOB(2239, magic_user);
        ASSIGNMOB(2223, cleric2);
        ASSIGNMOB(2241, gaze_npc);
        ASSIGNMOB(2243, warrior1);	/* drider king */
        ASSIGNMOB(2242, warrior1);	/* Drider queen */
        ASSIGNMOB(2240, magic_user);	/* Drider misstress */
        ASSIGNMOB(2235, warrior1);	/* drider knight */
        ASSIGNMOB(2234, magic_user); 	/* Troglod King */
        ASSIGNMOB(2233, magic_user);	/* Troglod Queen */
        ASSIGNMOB(2232, warrior1);	/* Troglod Master */
        ASSIGNMOB(2231, cleric2);	/* Troglod Misstress */
        ASSIGNMOB(2230, magic_user);	/* Troglod Apprentice */
        ASSIGNMOB(2227, gaze_npc);	/* Troglod Priest */
        ASSIGNMOB(2224, magic_user);	/* trakker Queen */
        ASSIGNMOB(2225, magic_user);    /* trakker King */
        ASSIGNMOB(2218, cleric2);    	/* trakker Priest */
        ASSIGNMOB(2215, warrior1);      /* trakker */

		/* Anscient Greece */

        ASSIGNMOB(4800, warrior1);     /* Minotaur */
        ASSIGNMOB(4801, snake);        /* Vore */
        ASSIGNMOB(4802, regen);        /* gorgon */
        ASSIGNMOB(4803, warrior1);     /* sterope */
        ASSIGNMOB(4808, magic_user);   /* nix */
        ASSIGNMOB(4809, fire_breath);  /* steed */
        ASSIGNMOB(4810, warrior1);     /* margoyle */
        ASSIGNMOB(4811, aphrodite);    /* helen */
        ASSIGNMOB(4812, cleric2);      /* pegasus */
        ASSIGNMOB(4813, phoenix);      /* phoenix */
        ASSIGNMOB(4814, warrior1);     /* medusa */
        ASSIGNMOB(4816, trojan);       /* Trojan horse */
        ASSIGNMOB(4818, warrior1);     /* theseus */
        ASSIGNMOB(4819, regen);        /* man */
        ASSIGNMOB(4820, regen);        /* woman */
        ASSIGNMOB(4821, gaze_npc);     /* king minos */
        ASSIGNMOB(4822, warrior1);     /* perseus */
        ASSIGNMOB(4823, banish);       /* Zeus */
        ASSIGNMOB(4824, disposable);   /* Trojan Warrior */
        ASSIGNMOB(4825, aphrodite);    /* aphrodite */
        ASSIGNMOB(4826, gaze_npc);     /* Pluto */
        ASSIGNMOB(4827, magic_user);   /* guys in hell */
        ASSIGNMOB(4828, magic_user);   /* guys in hell */
        ASSIGNMOB(4829, magic_user);   /* guys in hell */
        ASSIGNMOB(4830, magic_user);   /* guys in hell */
        ASSIGNMOB(4832, acid_breath);  /* cerberus */

	// The island forever

  	ASSIGNMOB(10101, fate);
  	ASSIGNMOB(10154, playerhunter);
  	ASSIGNMOB(10156, playerhunter);
  	ASSIGNMOB(10157, titan);
  	ASSIGNMOB(10114, anxiousleader);
  	ASSIGNMOB(10113, anxiousleader);
  	ASSIGNMOB(10108, packmember);
  	ASSIGNMOB(10109, packleader);
  	ASSIGNMOB(10105, avenger);
  	ASSIGNMOB(10150, insanity);
  	ASSIGNMOB(10102, playerhunter);
  	ASSIGNMOB(10103, arrogance);

                  /*The Sphere*/

        ASSIGNMOB(27001, acid_breath);   /*poisonous Gas*/
        ASSIGNMOB(27006, throw);   /*Cpt Burns*/
        ASSIGNMOB(27007, warrior); /*Cor. Fletch*/
        ASSIGNMOB(27009, ice_breath);   /*poisonoud Gas*/
        ASSIGNMOB(27012, regen);         /*Corpral Levy*/
        ASSIGNMOB(27013, warrior);      /*captain Burns*/
        ASSIGNMOB(27015, snake);       /*green snake*/
        ASSIGNMOB(27016, cleric);     /*corporal Tina*/
        ASSIGNMOB(27017, constrictor); /*Squid*/
        ASSIGNMOB(27018, school);   /*squid sacs*/
        ASSIGNMOB(27019, constrictor); /*tentacle*/
        ASSIGNMOB(27020, regen);      /*shrip*/
        ASSIGNMOB(27021, snake);
	ASSIGNMOB(27022, alien_voice);	/* alien voice */
        ASSIGNMOB(27024, warrior);    /*sentroid*/
        ASSIGNMOB(27028, gaze_npc);  /*evil blackness*/
        ASSIGNMOB(27032, corridor_guard); /*guardian droid*/
        ASSIGNMOB(27033, corridor_guard);   /* guardian droid KF*/
	ASSIGNMOB(27040, alien_voice_echo);	    /* alien hunter */

                 /*Babylon5*/
        ASSIGNMOB(30000, magic_user);   /*Soul Hunter*/
        ASSIGNMOB(30002, magic_user);  /*Drak*/
        ASSIGNMOB(30004, warrior);   /*Sheridian*/
        ASSIGNMOB(30005, cleric2);  /*franklin*/
        ASSIGNMOB(30006, warrior);  /*ivanova*/
        ASSIGNMOB(30007, gaze_npc); /*kosh*/
        ASSIGNMOB(30011, warrior1); /*drall*/
        ASSIGNMOB(30015, magic_user); /*bester*/
        ASSIGNMOB(30016, magic_user); /*Lyta*/
        ASSIGNMOB(30021, warrior1);  /*shadow*/
        ASSIGNMOB(30022, warrior1);  /*Vorlon*/
        ASSIGNMOB(30025, magic_user); /*Psi Cop*/
	ASSIGNMOB(30008, delenn); /* Delenn*/
              

               /*Holy City*/

       ASSIGNMOB(5203, warrior);  /*crypt thing*/
       ASSIGNMOB(5204, regen);   /*dragon horse*/
       ASSIGNMOB(5208, warrior1); /*cloud giant*/
       ASSIGNMOB(5209, cleric);  /*ki-rin*/
       ASSIGNMOB(5210, gaze_npc); /*God Pazuzu*/
       ASSIGNMOB(5211, magic_user); /*deva*/
       ASSIGNMOB(5212, magic_user); /*deva*/
       ASSIGNMOB(5213, cleric);  /*deva*/
}



/* assign special procedures to objects */
void assign_objects(void)
{
  SPECIAL(bank);
  SPECIAL(cowboy_hat);
  SPECIAL(deadlyblade);
  SPECIAL(dimensional_gate); /* spec proc for gate spell */
  SPECIAL(frisbee);
  SPECIAL(gen_board);
  SPECIAL(love_ring);
  SPECIAL(marbles);
  SPECIAL(pokies);
  SPECIAL(pokies1);
  SPECIAL(pokies2);
  SPECIAL(pokies3);
  SPECIAL(ring);
  SPECIAL(roller_blades);
  SPECIAL(snap);
  SPECIAL(tardis);
  SPECIAL(toboggan);
  SPECIAL(xmas_tree);

  ASSIGNOBJ(10232, deadlyblade);

  ASSIGNOBJ(22327, toboggan);
  ASSIGNOBJ(22328, roller_blades);
  ASSIGNOBJ(22332, frisbee);
  ASSIGNOBJ(22329, snap);
  ASSIGNOBJ(22320, xmas_tree); /* xmas tree for tavern */
  ASSIGNOBJ(22314, cowboy_hat); /* cowboy hat to transport to westworld */
  ASSIGNOBJ(22398, tardis);
  ASSIGNOBJ(22300, dimensional_gate); /* gate spell */
  ASSIGNOBJ(3091, gen_board);	/* enterprise board */
  ASSIGNOBJ(3092, gen_board);	/* enterprise board */
  ASSIGNOBJ(3093, gen_board);	/* jedi's board */
  ASSIGNOBJ(3094, gen_board);	/* avenger's board */
  ASSIGNOBJ(3095, gen_board);	/* guardian's board */
  ASSIGNOBJ(3096, gen_board);	/* social board */
  ASSIGNOBJ(3097, gen_board);	/* freeze board */
  ASSIGNOBJ(3098, gen_board);	/* immortal board */
  ASSIGNOBJ(3099, gen_board);	/* mortal board */

  ASSIGNOBJ(3034, bank);	/* atm */
  ASSIGNOBJ(13225, bank);       /* bank in westworld = teller */

/*  ASSIGNOBJ(16024,love_ring); */
  ASSIGNOBJ(9040, marbles);

/* Gambling House */                  /* commented out till objects "exist" again */
/*  ASSIGNOBJ(13800, pokies);
  ASSIGNOBJ(13801, pokies1);
  ASSIGNOBJ(13802, pokies2);
  ASSIGNOBJ(13803, pokies3);  */

}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  extern int dts_are_dumps;
  int i;

  SPECIAL(casino);
  SPECIAL(dump);
  SPECIAL(pet_shops);
  SPECIAL(pray_for_items);
  SPECIAL(elevator); /* glass elevator */
  SPECIAL(set_tag); /* tag arena teleporter thing */
  SPECIAL(maze);

  // The island forever - Talisman's spec_procs
  SPECIAL(darkportal);
  SPECIAL(pillars);
  SPECIAL(room_magic_ripple);
  SPECIAL(titansuit);
  SPECIAL(room_magic_unstable);

  // Phoenix and Kittens cat flap
  SPECIAL(catflap);
  ASSIGNROOM(335, catflap);
  ASSIGNROOM(429, catflap);

  /* ASSIGNROOM(3030, dump);
  ASSIGNROOM(3031, pet_shops);
  ASSIGNROOM(21116, pet_shops);
*/
  ASSIGNROOM(4437, elevator);
  ASSIGNROOM(4520, set_tag);
  ASSIGNROOM(2762, maze);

  // Casino rooms
  ASSIGNROOM(100, casino);
  ASSIGNROOM(133, casino);

  // The island forever

  /* LEVEL 3 of tower */
  ASSIGNROOM(10289, room_magic_ripple);
  ASSIGNROOM(10288, room_magic_ripple);
  ASSIGNROOM(10287, room_magic_ripple);
  ASSIGNROOM(10286, room_magic_ripple);
  ASSIGNROOM(10285, room_magic_ripple);
  ASSIGNROOM(10284, room_magic_ripple);
  ASSIGNROOM(10283, room_magic_ripple);
  ASSIGNROOM(10281, room_magic_ripple);
  ASSIGNROOM(10280, room_magic_ripple);
  ASSIGNROOM(10279, room_magic_ripple);
  ASSIGNROOM(10278, room_magic_ripple);
  ASSIGNROOM(10277, room_magic_ripple);
  ASSIGNROOM(10276, room_magic_ripple);
  ASSIGNROOM(10275, room_magic_ripple);
  ASSIGNROOM(10274, room_magic_ripple);
  ASSIGNROOM(10272, room_magic_ripple);
  ASSIGNROOM(10271, room_magic_ripple);
  ASSIGNROOM(10270, room_magic_ripple); 

 /* LEVEL 4 of tower */
  ASSIGNROOM(10318, room_magic_unstable);
  ASSIGNROOM(10317, room_magic_unstable);
  ASSIGNROOM(10316, room_magic_unstable);
  ASSIGNROOM(10315, room_magic_unstable);
  ASSIGNROOM(10313, room_magic_unstable);
  ASSIGNROOM(10312, room_magic_unstable);
  ASSIGNROOM(10311, room_magic_unstable);
  ASSIGNROOM(10310, room_magic_unstable);
  ASSIGNROOM(10309, room_magic_unstable);
  ASSIGNROOM(10308, room_magic_unstable);
  ASSIGNROOM(10307, room_magic_unstable);
  ASSIGNROOM(10306, room_magic_unstable);
  ASSIGNROOM(10305, room_magic_unstable);
  ASSIGNROOM(10304, room_magic_unstable);
  ASSIGNROOM(10303, room_magic_unstable);
  ASSIGNROOM(10302, room_magic_unstable);
  ASSIGNROOM(10301, room_magic_unstable);
  ASSIGNROOM(10349, titansuit);
  ASSIGNROOM(10330, darkportal);
  ASSIGNROOM(10204, pillars);

  if (dts_are_dumps)
    for (i = 0; i < top_of_world; i++)
      if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH))
	world[i].func = dump;
}
