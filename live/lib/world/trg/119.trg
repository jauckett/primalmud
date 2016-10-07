#11901
Greeting Trigger~
0 g 100
~
eval level %actor.level%
if (%actor.level% <=90)
wait 2
say Welcome to the Labyrinth!
say Are you ready for the challenge %actor.name%?
wait 2
say Well WHAT is it going to be? YES or NO??
wait 2
say Come on %actor.name% we haven't got all day!
elseif (%actor.level% >=91)
%load% obj 11909
say Here is the clock you want!
give clock %actor.name%
say Trust ME you want it!
end
~
#11902
The worm~
0 g 100
~
wait 1
say ello %actor.name%!
wait 5
say Come inside and meet the Misses!
wait 9
say Come on %actor.name%! Come inside and have a nice cuppa tea!
~
#11903
Helping hands speech - remove us ~
0 c 100
down~
if %actor.canbeseen%
say to go down you have to remove me!
end
~
#11904
Crystal Ball~
2 g 100
~
wait 5
%echo% A Crystal Ball comes rolling along the path from the east!
%load% obj 11936
wait 15
%echo% A Crystal Ball goes rolling along the path to the north!
%purge%
~
#11905
Old Man Room Trigger~
2 g 100
~
wait 5
%echo% A crystal ball rolls in from the south straight up to the old man!
wait 6
%echo% He moves his hand out and the ball jumps into his palm!
~
#11906
Fortune Teller~
0 dg 1
yes no thank you~
wait 2
if (%speech% == yes)
say Sometimes the way backward is the way forward!
elseif (%speech% == no)
say Make a donation to a poor old man?
elseif (%speech% == thank you)
say Your welcome %actor.name%
wait 5
%echo% The fortune teller rattles his collection box waiting for a donation!
~
#11907
Fortune teller greet~
0 g 100
~
wait 2
say Would %actor.name% like some advice from a poor old fortune teller?
wait 2
%echo% The fortune teller rattles his collection box waiting for a donation!
end
~
#11908
Fortune Teller bribe~
0 m 0
~
if (%amount% <= 19)
say Thank you for your kindly donation %actor.name%
elseif (%amount% >= 20)
say You are extremely generous and you shall not go unrewarded my dear friend %actor.name%
%load% obj 11936
wait 2
give ball %actor.name%
end
~
#11909
Falling over the edge~
2 g 100
~
wait 5
%echo% As you try to move back against the cliff the stones under your feet give way
%teleport% all 12025
%force% %actor% look
~
#11910
Steelgates purged room 12062~
2 g 100
0~
%echo% The steelgates swing open!
wait 2
%echo% The steelgate slams closed behind you!
%door% 12062 south flag b
%echo% Oh DAMN it locked itself too!
~
#11911
hands make u fall 1 - death~
0 f 100
~
%send% %actor% You realise there is nothing supporting you and you fall!
%teleport% all 11943
%force% %actor% look
~
#11912
hands make u fall 2 - death~
0 f 100
~
%send% %actor% You realise there is nothing supporting you and you fall!
%teleport% all 11944
%force% %actor% look
~
#11913
hands make u fall 3 - death~
0 f 100
~
%send% %actor% You realise there is nothing supporting you and you fall!
%teleport% all 11945
%force% %actor% look
~
#11914
hands make u fall 4 - death~
0 f 100
~
%send% %actor% You realise there is nothing supporting you and you fall!
%teleport% all 11946
%force% %actor% look
~
#11915
hands make u fall 5 - death~
0 f 100
~
%send% %actor% You realise there is nothing supporting you and you fall!
%teleport% all 11947
%force% %actor% look
~
#11916
hands make u fall 6 - death~
0 f 100
~
%send% %actor% You realise there is nothing supporting you and you fall!
%teleport% all 11948
%force% %actor% look
~
#11917
hands make u fall 7 - death~
0 f 100
~
%send% %actor% You realise there is nothing supporting you and you fall!
%teleport% all 11949
%force% %actor% look
~
#11918
Dropping of Chair in Room 12050~
2 h 100
An ornate Chair.~
wait 2
%echo% CCccccccrrrrrrrrraaaaaaaaaaaaaaaccccccccccckkkkkkkkkkkkk!
wait 1
%echo% Dropping such a heavy chair on a glass floor is not a good idea!
wait 15
%echo% You find yourself standing on thin air as all the glass falls to the ground!
wait 5
%echo% You begin to drift downwards towards the ground.
%teleport% all 12051
~
#11919
Purging steelgates in room 12061~
2 d 50
QUICK!~
%echo% The Steelgates SLAM SHUT!!
wait 2
%echo% *CLICK* and lock from the other side!
%door% 12061 north flag b
end
~
#11920
Flamingo speech on fortunes words ~
0 d 100
backward forward~
wait 5
say Oh! Oh! You call that advise? Geeze!
~
#11921
Sleeping Guard cries for help!~
0 k 100
~
say Oh my! Help %actor.name% is attacking me!
wait 15
say QUICK! Lock the GATES! Were under ATTACK!
wait 15
say Oh MY! HELP I am being ATTACKED!
wait 360s
~
#11922
Reset the steelgates in room 12061~
0 f 100
~
%door% 12061 north flag a
%echo% CLICK, CLICK, CLICK and the steelgates swing open!
~
#11923
Atempt to open steelgate in room 12061~
0 c 100
open~
eval level %actor.level%
eval neq %actor.eq(neck)%
if (%actor.level% <=90)
wake
stand
say I am the City Guard! You can not pass me %actor.name%!
wait 15
say Go away or I will get the gates locked!
elseif (%actor.level% >=91)
wake
stand
say Oh %actor.name% it is you!
wait 2
say Have you got the clock???
wait 2
open steelgate
say You may pass!
end
~
#11924
Howling sounds in room 11995~
2 g 100
~
wait 20
%echo% HHhhhhhOOOOOooooooWWWWWWWWlllllllllllll
~
#11925
Owl transforming into Goblin King room 11901~
0 d 100
Yes No What~
wait 2
if (%speech% == yes)
%echo% The owl shimmers and transforms itself into.....
mtransform 11936
say Muhahahahaha, So %actor.name%!
wait 5
say I am glad you are ready for the challenge!
say Remember 13 hours is all you have to complete the labyrinth!
wait 5
say If you fail I get to keep Toby and transform him into a goblin!
say Since I know you %actor.race%'s are so stupid!
wait 5
say I will give you a clock that will remind you, you only have 13 hours!
%force% all drop all.clock
%load% obj 11910
give clock %actor.name%
%echo% The Goblin King shimmers and transforms itself into....
mtransform 11904
elseif (%speech% == no)
%teleport% %actor% 1112
mtransform 11904
elseif
say I have Toby due to Sarah's request!
say You have 13 hours to solve my labyrinth! 
say You will need the clock that will remind thee of the time used!
mtransform 11904
end
~
#11926
Timer on Clock from room 11901~
1 ai 100
~
wait 1
%echo% The Clock starts to tick!
wait 276s
%echo% The clock strikes the 3rd hour! Only 10 to go!
wait 276s
%echo% The clock strikes the 6th hour! Only 7 to go!
wait 276s
%echo% The clock strikes the 9th hour! Only 4 more hours to go!
wait 350s
%echo% As the Clock begins to strike the 13th hour!
wait 2
%echo% The air infront of you starts to shimmer and take shape....
wait 2
  %echo% A body of a man is transforming out of thin air infront of you.
%load% mob 11925
%purge% self
end
~
#11927
Time expired king speech on mob 11925~
0 n 100
~
set inrm %self.room%
if ((%inrm% > 11900) && (%inrm% < 12099))
wait 10
say Well young *SNIF* THING I knew you would not be able to solve my Labyrinth.
say For you lack of ability I get to keep Toby!
wait 10
chuckle
say Come here Toby!
wait 10
mecho The air quivers as a body comes flying through the air.
mload mob 11934
set ch %actor.next_in_room%
while (%ch)
if (%ch.vnum% < 0) 
mforce %ch% look
end
set ch %ch.next_in_room%
done
unset ch
wait 10
say Now Toby you are mine!
wait 4
say Now come come Toby it isn't that bad, just stand still this won't take long.
else
wait 2
mecho The King detects he is not in HIS world and vanishes along with the clock!
mpurge self
end
~
#11928
Tobys whine on mob 11934~
0 n 100
~
wait 10
say Well gee thanks!
wait 2
say Looks like I have to live with him for the rest of my life!
wait 6
say Would someone just kill me now? Before he transforms me?
wait 2
%echo% Toby's body starts to convulse in pain as the Goblin King transforms him into....
mtransform -11937
force %actor% look
~
#11929
Goblin king fight on mob 11925~
0 k 100
~
eval level %actor.level%
if (%actor.level% <11)
say *SNIFF* Dream on %actor.name% you are too small to fight me!
say Away with you Ugly Goblin!
%echo% The ugly goblin shimmers and vanishes
%purge% %11934%
%echo% The Goblin King snaps his fingers!
mtransform -11916
wait 500s
elseif (%actor.level% >11) && (%actor.level% <20)
say *SCOFF* This is still too easy for me!
say I refuse to waste a nice new goblin on the likes of YOU!
%purge% %11934%
say I am not going to waste my time any further on you futile THINGS!
%echo% The Goblin King transforms himself into......
mtransform -11917
wait 500s
elseif (%actor.level% >20) && (%actor.level% <25)
say FINALLY! A challenge worth taking!
say Give it your best shot!
wait 15
say IS that THE best you can DO?
wait 500s
elseif (%actor.level% >25) && (%actor.level% <40)
say So you think you are good enough to take on the Goblin King!
say Well I think I need to improve my defenses before i attempt YOU!
%echo% The Goblin King prays to the powers that be and expands into....
mtransform -11936
wait 2
say My that does feel better, now it will be an even challenge!
wait 500s
end
~
#11930
Clock purge on drop object 11910~
1 h 100
~
wait 1
%echo% As the Ornate Clock touches ground it disolves into nothing!
%purge% self
~
#11931
give purge on clock~
1 i 100
~
wait 2
%echo% As an Ornate Clock leaves %actor.name% hands....
wait 2
%echo% It vanishes into thin air!
%purge% self
end
~
#11932
Toby Fight trigger on mob 11934~
0 k 100
~
eval level %actor.level%
if (%actor.level% <11)
say What?? You can't even solve the problem NOW you want to fight me?
say Since your not good enough yet to save my life...
say Here is something you can play with!
%echo% The UGLY goblin transforms himself into.....
mtransform -11907
wait 500s
elseif (%actor.level% >11) && (%actor.level% <21)
say Oh you are rather amusing, but you are still too stupid to solve the Labyrinth!
say You were given a clock and even then you couldn't figure it out within the time!
say Do you SERIOUSLY believe I am going to waste my time with you?
%echo% The UGLY goblin shimmers into.....
mtransform -11919
wait 500s
elseif (%actor.level% >21) && (%actor.level% <30)
say Well if you believe you are good enough you can have TOBY back and try HIM on for size!
say Since HUMANS are weak and pathetic creatures, you should kill him quickly enough!
%echo% As the air shimmers you find yourself looking at.....
mtransform -11934
wait 500s
elseif (%actor.level% >30)
say Finally you are worth my time!
wait 500s
end
~
#11933
Floating down from room 12051~
2 ac 100
l~
wait 15
%echo% You continue your gentle decent downwards!
%teleport% all 12052
~
#11934
Floating down from room 12052~
2 ac 100
l~
wait 2
%echo% After Slowly floating down to the ground, you find yourself in a pile of rubbish!
%teleport% all 12053
~
#11935
Resetting door in room 12061~
2 f 100
~
%door% 12061 north flag a
~
#11936
Opening of blanket box~
1 c 100
look~
%load% mob 11946
~
#11937
giggle of fat gremlin~
0 n 100
~
%load% obj 11917
wear all
wait 1
%echo% A fat little gremlin jumps out of a Pine Blanket Box.
wait 2
giggle
~
#11939
Checking for the clock~
0 gn 100
~
eval neq %actor.eq(neck)%
if (%neq.vnum% == 11909)
wait 5
say Oh goodness me what have you DONE?
wait 5
say Don't you know that CLOCK is the beginning of the END?
wait 5
say Quickly! Quickly! Remove it!
wait 5
%force% %actor.name% rem clock
wait 5
wait 5
%echo% A sudden gust of wind temporarily blinds you!
wait 10
%force% %actor.name% look
wait 5
say Oh Deary Me too LATE!
end
~
#11940
extra description on eternal's clock~
1 j 100
~
wait 5
%echo% The shiny Ornate clock starts to hum and shine very brightly!
end
~
#11941
Metal Goblin check for Level && Clock~
0 c 100
open~
eval level %actor.level%
eval neq %actor.eq(neck)%
if (%actor.level% <=90)
say You can't go past here! Be gone with you!
transfer 1112
elseif (%actor.level% >=91) && (%neq.vnum% == 11909)
say You %actor.name% may PASS but only because you have the Clock with you!
open wooden
elseif (%actor.level% >=91)
say Wheres the CLOCK?
end
~
#11942
Remove and Purging of Clock~
1 l 100
~
wait 2
%load% mob 11948
%echo% Disolves before your very eyes!
%purge% self
~
#11943
Captain of the Guard Load ~
0 n 100
~
set inrm %self.room%
if ((%inrm% > 12062) && (%inrm% < 12099))
wait 20
say Well Now! You seem to have solved the Labyrinth in a very timely manner!
wait 2
say The Goblin King will be most displeased!
wait 2
say I must prepare our last line of defense!
wait 5
say Cavalry to ME!
%load% mob 11947
%load% mob 11947
%load% mob 11947
e
%load% mob 11949
%load% mob 11949
%load% mob 11949
e
%load% mob 11950
%load% mob 11950
%load% mob 11950
n
%load% mob 11951
%load% mob 11951
%load% mob 11951
n
else
wait 2
mecho The Captain of the Guards detects he is not in his city and vanishes
mpurge self
end
~
#11944
Ludo call for help~
0 k 100
~
wait 5s
mecho Howls in pain and frustration
%load% mob 11952
wait 5s
~
#12017
falling through tunnel~
2 g 100
~
wait 15
%echo% you begin to plummit downwards.
wait 15
%echo% You can't see what's around you for all the dirt and rocks.
wait 15
%echo% you continue on at an alarming rate until...
wait 15
%teleport% all 12020
%force% %actor% look
~
$~
