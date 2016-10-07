# Generated automatically from Makefile.in by configure.
# CircleMUD 3.0 Makefile.in - Makefile template used by 'configure'
#

# C compiler to use
CC = g++
#CC = /usr/local/bin/g++ 

# Path to cxref utility
CXREF = cxref

# Stuff for debug libraries - Artus.
#LD_LIBRARY_PATH = /usr/lib/debug

# Any special flags you want to pass to the compiler
#
# -DNO_LOCALTIME      - Don't use system localtime().
# -DNOTELL_SOUNDPROOF - Forbid telling to/from soundproof rooms.
# 
MYFLAGS=-DPRIMAL_LIVE -Wall -fpermissive -Wno-write-strings

# Debugging Libraries -- Artus.
#LD_LIBRARY_PATH = /usr/lib/debug/

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE) 

# Uncomment -lcrypt if needed for linking
#LIBS = -lstdc++
LIBS = -lcrypt

OBJFILES = act.comm.o act.informative.o act.item.o act.movement.o \
	act.offensive.o act.other.o act.social.o act.wizard.o alias.o \
	balance.o ban.o boards.o casino.o castle.o clan.o class.o colour.o \
	comm.o config.o constants.o corpses.o db.o fight.o genmob.o genobj.o \
	genolc.o genshp.o genwld.o genzon.o graph.o handler.o house.o \
	improved-edit.o interpreter.o limits.o locate.o magic.o mail.o medit.o \
	mobact.o modify.o oasis.o objsave.o oedit.o olc.o quest.o random.o \
	redit.o reports.o sedit.o shop.o spec_assign.o spec_procs.o \
	spell_parser.o spells.o tedit.o utils.o weather.o zedit.o \
	dg_comm.o dg_db_scripts.o dg_event.o dg_handler.o dg_mobcmd.o \
	dg_misc.o dg_objcmd.o dg_scripts.o dg_triggers.o dg_wldcmd.o dg_olc.o

CXREF_FILES = act.comm.c act.informative.c act.item.c act.movement.c \
	act.offensive.c act.other.c act.social.c act.wizard.c alias.c \
	balance.c ban.c boards.c castle.c clan.c class.c colour.c \
	comm.c corpses.cp config.c constants.c db.c fight.c genmob.c genobj.c \
	genolc.c genshp.c genwld.c genzon.c graph.c handler.c house.c \
	improved-edit.c interpreter.c limits.c locate.c magic.c mail.c medit.c \
	mobact.c modify.c oasis.c objsave.c oedit.c olc.c quest.c random.c \
	redit.c reports.cpp sedit.c shop.c spec_assign.c spec_procs.c \
	spell_parser.c spells.c tedit.c utils.c weather.c zedit.c \
	dg_comm.c dg_db_scripts.c dg_event.c dg_handler.c dg_mobcmd.c \
	dg_misc.c dg_objcmd.c dg_scripts.c dg_triggers.c dg_wldcmd.c dg_olc.c

default: all

all: .accepted
	$(MAKE) $(BINDIR)/circle
#	$(MAKE) utils

.accepted:
	@./licheck less

utils: .accepted
	(cd util; $(MAKE) all)
circle:
	$(MAKE) $(BINDIR)/circle

version:
	./update_revisions 

$(BINDIR)/circle : $(OBJFILES)
	$(CC) -o $(BINDIR)/circle $(PROFILE) $(OBJFILES) $(LIBS)
	$(MAKE) version

tags: *.c *.cpp *.h
	ctags *.c *.cpp *.h

clean:
	rm -f *.o
ref:
#
# Create the cross reference files
# Note, this is not meant to be used unless you've installed cxref...
#
	@for file in $(CXREF_FILES) ; do \
	  echo Cross referencing $$file ; \
	  $(CXREF) -D__CXREF__ -xref -Odoc -Ncircle $$file ; \
	done
#
# Create the source files using cxref
#
	@for file in $(CXREF_FILES) ; do \
	   echo Documenting $$file ; \
	   ( cd . ; $(CXREF) -D__CXREF__ -warn-xref -xref -Odoc -Ncircle -html $$file ) ; \
	   rm -f $(DOCS) ; \
	done
#
# Create the index using cxref
#
	@echo Indexing
	@( cd . ; $(CXREF) -D__CXREF__ -index-all -Odoc -Ncircle -html )
	@rm -f $(DOCS)
#
# Make html files for the .h files
#
	@echo Creating .h.html files...
	@for file in *.h ; do \
	  echo $$file ; \
	  cat /home/jelson/mud/htmlh-head $$file /home/jelson/mud/htmlh-tail > doc/$$file.html ; \
	done
# Copy over to the html directory
	cp doc/*.html /home/jelson/public_html/circle/cdp/cxref
	chmod 644 /home/jelson/public_html/circle/cdp/cxref/*.html

# Dependencies for the object files (automagically generated with
# gcc -MM)

depend:
	$(CC) -MM *.c > depend
act.build.o: act.build.c
	$(CC) -c $(CFLAGS) act.build.c
act.comm.o: act.comm.c conf.h sysdep.h constants.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h screen.h spells.h improved-edit.h dg_scripts.h
	$(CC) -c $(CFLAGS) act.comm.c
act.informative.o: act.informative.c conf.h sysdep.h structs.h utils.h \
 comm.h interpreter.h handler.h db.h spells.h screen.h constants.h colour.h \
 quest.h dg_scripts.h
	$(CC) -c $(CFLAGS) act.informative.c
act.item.o: act.item.c conf.h sysdep.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h spells.h constants.h corpses.h quest.h \
 dg_scripts.h
	$(CC) -c $(CFLAGS) act.item.c
act.movement.o: act.movement.c conf.h sysdep.h structs.h utils.h \
 comm.h interpreter.h handler.h db.h spells.h house.h constants.h dg_scripts.h
	$(CC) -c $(CFLAGS) act.movement.c
act.offensive.o: act.offensive.c conf.h sysdep.h structs.h utils.h \
 comm.h interpreter.h handler.h db.h spells.h constants.h
	$(CC) -c $(CFLAGS) act.offensive.c
act.other.o: act.other.c conf.h sysdep.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h spells.h screen.h house.h constants.h corpses.h \
 dg_scripts.h
	$(CC) -c $(CFLAGS) act.other.c
act.social.o: act.social.c conf.h sysdep.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) act.social.c
act.wizard.o: act.wizard.c conf.h sysdep.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h spells.h house.h screen.h constants.h \
 oasis.h quest.h genzon.h dg_scripts.h
	$(CC) -c $(CFLAGS) act.wizard.c
alias.o: alias.c conf.h sysdep.h structs.h utils.h interpreter.h db.h
	$(CC) -c $(CFLAGS) alias.c
balance.o: balance.c conf.h sysdep.h structs.h utils.h db.h handler.h
	$(CC) -c $(CFLAGS) balance.c
ban.o: ban.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h \
 handler.h db.h
	$(CC) -c $(CFLAGS) ban.c
boards.o: boards.c conf.h sysdep.h structs.h utils.h comm.h db.h \
 boards.h interpreter.h handler.h
	$(CC) -c $(CFLAGS) boards.c
reports.o: reports.cpp conf.h sysdep.h structs.h utils.h db.h handler.h \
 reports.h interpreter.h comm.h improved-edit.h genolc.h
	$(CC) -c $(CFLAGS) reports.cpp
casino.o: casino.c conf.h sysdep.h structs.h utils.h interpreter.h \
 comm.h db.h
	$(CC) -c $(CFLAGS) casino.c 
castle.o: castle.c conf.h sysdep.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) castle.c
clan.o: clan.c conf.h sysdep.h structs.h utils.h comm.h handler.h db.h clan.h
	$(CC) -c $(CFLAGS) clan.c
class.o: class.c conf.h sysdep.h structs.h db.h utils.h spells.h \
 interpreter.h handler.h constants.h
	$(CC) -c $(CFLAGS) class.c
colour.o: colour.c conf.h sysdep.h structs.h db.h utils.h comm.h \
 constants.h colour.h
	$(CC) -c $(CFLAGS) colour.c
comm.o: comm.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h \
 handler.h db.h house.h genolc.h oasis.h screen.h casino.h reports.h \
 dg_scripts.h spells.h
	$(CC) -c $(CFLAGS) comm.c
config.o: config.c conf.h sysdep.h structs.h interpreter.h
	$(CC) -c $(CFLAGS) config.c
constants.o: constants.c
	$(CC) -c $(CFLAGS) constants.c
corpses.o: corpses.cpp conf.h sysdep.h structs.h utils.h db.h handler.h \
 corpses.h
	$(CC) -c $(CFLAGS) corpses.cpp
db.o: db.c conf.h sysdep.h structs.h utils.h db.h comm.h handler.h \
 spells.h mail.h interpreter.h house.h clan.h constants.h colour.h casino.h \
 corpses.h genzon.h reports.h dg_scripts.h oasis.h balance.h
	$(CC) -c $(CFLAGS) db.c
dg_comm.o: dg_comm.c conf.h sysdep.h structs.h dg_scripts.h utils.h comm.h \
 handler.h db.h spells.h
	$(CC) -c $(CFLAGS) dg_comm.c
dg_db_scripts.o: dg_db_scripts.c conf.h sysdep.h structs.h dg_scripts.h \
 utils.h comm.h handler.h db.h dg_event.h
	$(CC) -c $(CFLAGS) dg_db_scripts.c
dg_event.o: dg_event.c conf.h sysdep.h dg_event.h utils.h structs.h
	$(CC) -c $(CFLAGS) dg_event.c
dg_handler.o: dg_handler.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
 comm.h handler.h spells.h db.h dg_event.h
	$(CC) -c $(CFLAGS) dg_handler.c
dg_misc.o: dg_misc.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
 comm.h handler.h db.h comm.h screen.h
	$(CC) -c $(CFLAGS) dg_misc.c
dg_mobcmd.o: dg_mobcmd.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
 comm.h handler.h db.h interpreter.h comm.h spells.h
	$(CC) -c $(CFLAGS) dg_mobcmd.c
dg_objcmd.o: dg_objcmd.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
 comm.h handler.h db.h interpreter.h comm.h
	$(CC) -c $(CFLAGS) dg_objcmd.c
dg_scripts.o: dg_scripts.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
 comm.h handler.h db.h comm.h screen.h
	$(CC) -c $(CFLAGS) dg_scripts.c
dg_triggers.o: dg_triggers.c conf.h sysdep.h structs.h dg_scripts.h \
 utils.h comm.h handler.h db.h interpreter.h
	$(CC) -c $(CFLAGS) dg_triggers.c
dg_wldcmd.o: dg_wldcmd.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
 comm.h handler.h db.h comm.h interpreter.h
	$(CC) -c $(CFLAGS) dg_wldcmd.c
dg_olc.o: dg_olc.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
 comm.h db.h oasis.h dg_olc.h dg_event.h
	$(CC) -c $(CFLAGS) dg_olc.c
fight.o: fight.c conf.h sysdep.h structs.h utils.h comm.h handler.h \
 interpreter.h db.h spells.h screen.h constants.h corpses.h dg_scripts.h
	$(CC) -c $(CFLAGS) fight.c
# DM - todo - obviously need to define these rules
genmob.o: genmob.c dg_olc.h
	$(CC) -c $(CFLAGS) genmob.c
genobj.o: genobj.c dg_olc.h
	$(CC) -c $(CFLAGS) genobj.c
genolc.o: genolc.c
	$(CC) -c $(CFLAGS) genolc.c
genshp.o: genshp.c
	$(CC) -c $(CFLAGS) genshp.c
genwld.o: genwld.c dg_olc.h
	$(CC) -c $(CFLAGS) genwld.c
genzon.o: genzon.c
	$(CC) -c $(CFLAGS) genzon.c
graph.o: graph.c
	$(CC) -c $(CFLAGS) graph.c
handler.o: handler.c conf.h sysdep.h structs.h utils.h comm.h db.h \
 handler.h interpreter.h spells.h clan.h corpses.h dg_scripts.h
	$(CC) -c $(CFLAGS) handler.c
house.o: house.c conf.h sysdep.h structs.h comm.h handler.h db.h \
 interpreter.h utils.h house.h constants.h genwld.h
	$(CC) -c $(CFLAGS) house.c
improved-edit.o: improved-edit.c
	$(CC) -c $(CFLAGS) improved-edit.c
interpreter.o: interpreter.c conf.h sysdep.h structs.h comm.h utils.h \
 interpreter.h db.h spells.h comm.h handler.h mail.h screen.h genolc.h oasis.h \
 tedit.h constants.h reports.h dg_scripts.h
	$(CC) -c $(CFLAGS) interpreter.c
limits.o: limits.c conf.h sysdep.h structs.h utils.h spells.h comm.h \
 db.h handler.h interpreter.h dg_scripts.h
	$(CC) -c $(CFLAGS) limits.c
locate.o: locate.c conf.h sysdep.h structs.h interpreter.h comm.h db.h utils.h \
         handler.h spells.h
	$(CC) -c $(CFLAGS) locate.c
magic.o: magic.c conf.h sysdep.h structs.h utils.h comm.h spells.h \
 handler.h db.h interpreter.h dg_scripts.h
	$(CC) -c $(CFLAGS) magic.c
mail.o: mail.c conf.h sysdep.h structs.h utils.h comm.h db.h \
 interpreter.h handler.h mail.h
	$(CC) -c $(CFLAGS) mail.c
medit.o: medit.c conf.h sysdep.h structs.h interpreter.h comm.h spells.h \
 utils.h db.h shop.h genolc.h genmob.h genzon.h oasis.h handler.h constants.h \
 improved-edit.h dg_olc.h
	$(CC) -c $(CFLAGS) medit.c
mobact.o: mobact.c conf.h sysdep.h structs.h utils.h db.h comm.h \
 interpreter.h handler.h spells.h constants.h
	$(CC) -c $(CFLAGS) mobact.c
modify.o: modify.c reports.h genwld.h clan.h # hmm.. should be more than this
	$(CC) -c $(CFLAGS) modify.c
oasis.o: oasis.c
	$(CC) -c $(CFLAGS) oasis.c
objsave.o: objsave.c conf.h sysdep.h structs.h comm.h handler.h db.h \
 interpreter.h utils.h spells.h
	$(CC) -c $(CFLAGS) objsave.c
oedit.o: oedit.c conf.h sysdep.h structs.h comm.h interpreter.h spells.h \
 utils.h db.h boards.h constants.h shop.h genolc.h genobj.h oasis.h \
 improved-edit.h dg_olc.h
	$(CC) -c $(CFLAGS) oedit.c
olc.o: olc.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h \
 handler.h db.h olc.h dg_olc.h
	$(CC) -c $(CFLAGS) olc.c
quest.o: quest.c conf.h sysdep.h structs.h interpreter.h comm.h db.h utils.h \
         handler.h house.h quest.h
	$(CC) -c $(CFLAGS) quest.c
random.o: random.c
	$(CC) -c $(CFLAGS) random.c
redit.o: redit.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h db.h \
 boards.h genolc.h genwld.h oasis.h improved-edit.h dg_olc.h
	$(CC) -c $(CFLAGS) redit.c
sedit.o: sedit.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h db.h \
 shop.h genolc.h genshp.h oasis.h
	$(CC) -c $(CFLAGS) sedit.c
shop.o: shop.c conf.h sysdep.h structs.h comm.h handler.h db.h \
 interpreter.h utils.h shop.h constants.h dg_scripts.h
	$(CC) -c $(CFLAGS) shop.c
spec_assign.o: spec_assign.c conf.h sysdep.h structs.h db.h \
 interpreter.h utils.h clan.h boards.h
	$(CC) -c $(CFLAGS) spec_assign.c
spec_procs.o: spec_procs.c conf.h sysdep.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h spells.h constants.h dg_scripts.h # casino.h
	$(CC) -c $(CFLAGS) spec_procs.c
spell_parser.o: spell_parser.c conf.h sysdep.h structs.h utils.h \
 interpreter.h spells.h handler.h comm.h db.h clan.h constants.h
	$(CC) -c $(CFLAGS) spell_parser.c
spells.o: spells.c
	$(CC) -c $(CFLAGS) spells.c
tedit.o: tedit.c conf.h sysdep.h structs.h utils.h interpreter.h comm.h db.h \
 genolc.h oasis.h improved-edit.h tedit.h
	$(CC) -c $(CFLAGS) tedit.c
utils.o: utils.c conf.h sysdep.h structs.h utils.h db.h comm.h \
 screen.h spells.h handler.h interpreter.h constants.h colour.h
	$(CC) -c $(CFLAGS) utils.c
weather.o: weather.c conf.h sysdep.h structs.h utils.h comm.h \
 handler.h interpreter.h db.h
	$(CC) -c $(CFLAGS) weather.c
zedit.o: zedit.c conf.h sysdep.h structs.h comm.h interpreter.h utils.h db.h \
 constants.h genolc.h genzon.h oasis.h dg_scripts.h
	$(CC) -c $(CFLAGS) zedit.c
