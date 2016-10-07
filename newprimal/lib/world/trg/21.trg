#2101
Vine - limit to 2 players in room~
2 g 100
~
if %Actor.canbeseen%
if (%direction% == down)
set numb %self.people(countall)%
if (%numb% == 2)
%send% %actor% There is not enough room for more then two people on the vine!
return 0
else
%send% %actor% You climb up the vine
end
end
end
if %Actor.canbeseen%
if (%direction% == up)
set numb %self.people(countall)%
if (%numb% == 2)
%send% %actor% There is not enough room for more then two people on the vine!
return 0
else
%send% %actor% You climb down the vine
end
end
end
~
#2102
Green tree snake - bite(casts poison)~
0 k 100
~
* get players health and store it as variable %dam%
eval dam %actor.hitp% / 10
* face bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake takes a bite at your face but misses!
%echoaround% %actor% A tree snake takes a bite at %actor.name%'s face but misses!
wait 50
end
end
* face bite get
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake lunges at your face and bites it!
%echoaround% %actor% A tree snake lunges at %actor.name%'s face and bites it!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
* leg bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake lunges for a bite at your leg but misses!
%echoaround% %actor% A tree snake lunges for a bite at %actor.name%'s leg but misses!
wait 50
end
end
*leg bite get
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake lunges at your face and bites it! YIKES!
%echoaround% %actor% A tree snake lunges at %actor.name%'s face and bites it! YIKES!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
* arm bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake lunges for a bite at your arm, but smacks into the vine instead!! HAHAHA!
%echoaround% %actor% A tree snake lunges for a bite at %actor.name%'s arm, but smacks into the vine instead!! HAHAHA!
wait 50
end
end
* arm bite get
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake lunges for a bite at your arm and is successful! YEEEOOOWWWCCCH!
%echoaround% %actor% A tree snake lunges for a bite at %actor.name%'s arm and is successful! YEEEOOOWWWCCCH!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
* groin bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake lunges for a bite at your groin, but smacks into the vine instead!! HAHAHA!
%echoaround% %actor% A tree snake lunges for a bite at %actor.name%'s groin, but smacks into the vine instead!! HAHAHA!
wait 50
end
end
* groin bite get
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% A tree snake lunges for a bite at your groin and is successful! OOOOOOOHHH THE PAIN!!!
%echoaround% %actor% A tree snake lunges for a bite at %actor.name%'s groin and is successful! OOOOOOOHHH THE PAIN!!!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
~
#2103
Reach top of vine~
2 g 100
~
if %actor.canbeseen%
if (%direction% == down)
%send% %actor% You manage to climb up onto the huge branch.
end
end
~
#2104
Room 2128 - door open when enter from East~
2 g 100
~
if (%direction% == East)
%door% 2128 east flag a
end
if (%direction% == north)
%door% 2128 north flag a
end
~
#2105
Room 2129 - door open thwn enter from south~
2 g 100
~
if (%direction% == south)
%door% 2129 south flag a
end
if (%direction% == east)
%door% 2129 east flag a
end
~
#2106
Room 2130 - door open when enter from West~
2 g 100
~
if (%direction% == west)
%door% 2130 west flag a
end
if (%direction% == north)
%door% 2130 north flag a
end
~
#2107
Room 2117 - door open when enter from West/east/north~
2 g 100
~
if (%direction% == west)
%door% 2117 west flag a
end
if (%direction% == north)
%door% 2117 north flag a
end
if (%direction% == east)
%door% 2117 east flag a
end
~
#2108
Doors open on enter - room 2129~
2 g 100
~
if (%direction% == south)
%door% 2129 south flag a
end
if (%direction% == east)
%door% 2129 east flag a
end
~
#2109
Doors open - room 2128 (north/east)~
2 g 100
~
if (%direction% == east)
%door% 2128 east flag a
end
if (%direction% == north)
%door% 2128 north flag a
end
~
#2110
Tree inhabitant - attack if not evil~
0 ghi 100
~
if %actor.canbeseen%
if (%actor.align% <= -350)
say Welcome to our treehouse %actor.name%. I hope you enjoy your stay with us!
smile
end
end
if %actor.canbeseen%
if (%actor.align% > -350)
say You should not have ventured here %actor.name%!
wait 1s
say You shall be taught to keep away the hard way.
wait 1s
eval room %actor.room%
if ((%room.vnum%  2100) && (%room.vnum%  2200))
kill %actor.name%
end
end
end
~
#2111
Door trigger - room 2120 - west exit~
2 g 100
~
if (%direction% == west)
%door% 2120 west flag a
wait 10s
%echo% The wind picks up momentarily and blows the door shut.
%door% 2120 west flag ab
end
~
#2112
Door trigger - room 2119 - east exit~
2 g 100
~
if (%direction% == east)
%door% 2119 east flag a
wait 10s
%echo% The wind outside picks up and blows the door shut.
%door% 2119 east flag ab
end
~
#2113
Room - enter - load possum falling/hit ground~
2 g 100
~
if %actor.canbeseen%
if %actor.vnum% < 0
if (%direction% == west)
if (%load_possum% != 1)
%purge% tree
wait 1s
%echo% There is a rustling in the trees above you.
wait 2s
%echo% There is a loud crack from above!
wait 3s
%load% mob 2108
%load% obj 2120
%damage% possum 200
wait 5s
%load% mob 2109
%load% mob 2109
wait 10s
%load% mob 2109
%load% mob 2109
wait 300s
end
end
end
end
~
#2114
Possum fall - screams and hits ground~
0 n 100
~
wait 1
scream
emote hits the ground so hard it shatters it's legs!
set load_possum 1
global load_possum
~
#2115
tree branch fall and hit ground~
1 n 100
~
wait 1
%echo% A tree branch hits the ground with such force it leaves a small crater!
set load_tree 1
global load_tree
wait 200
%purge% self
~
#2116
possum die, unset load_possum~
0 f 100
~
unset load_possum
~
#2117
Soldier bullant - load and attack possum~
0 n 100
~
wait 1
emote has arrived to investigate.
wait 5
say You stupid bloody possum!
laugh
wait 5
kill possum
~
#2118
secret chamber - door open when enter from up~
2 g 100
~
if (%direction% == up)
%door% 2157 up flag a
end
~
#2119
slide down from room 2152-2153-2154~
2 g 100
~
if %actor.canbeseen%
if %actor.level% < 111
wait 1s
%echo% You slide down the steep tunnel into the darkness below
%force% %actor% down
end
end
~
#2120
room 2154 enter from up - thud and stun actor~
2 g 100
~
if %actor.canbeseen%
if (%direction% == up)
wait 2
%echo% You hit the base of the tunnel with a thud, sending an echo throughout.
%damage% %actor% 200
end
end
~
#2121
mob 2133 - bullant guard prevent access down~
0 ghi 100
~
if %actor.vnum% < 0
if %actor.canbeseen%
wait 1
kill %actor.name%
end
end
~
#2122
mob 2133 - bullant guard - room 2149, prevent access north~
0 c 100
n~
if %actor.vnum% > 0
if %actor.canbeseen%
say Yeah right!
wait 1
kill %actor.name%
end
end
~
#2123
*free trigger*~
1 n 10
~
wait 1s
%echo% A white bullant egg is laid by the Queen Bullant.
~
#2124
Player enter room 2165 - queen bullant - eggs become babies~
2 g 100
~
if %actor.vnum% < 0
wait 1
%purge% egg
%load% mob 2115
%purge% egg
%load% mob 2115
%purge% egg
%load% mob 2115
%purge% egg
%load% mob 2115
%purge% egg
%load% mob 2115
end
~
#2125
worker bullant becomes soldier bullant~
0 n 100
~
wait 60s
wait 60s
emote grows a little stronger
wait 60s
wait 60s
emote grows a little stronger
wait 60s
wait 60s
emote grows a little stronger
wait 60s
wait 60s
emote grows a little stronger
wait 60s
wait 60s
emote suddenly has a growth spurt and becomes a soldier!
%load% mob 2116
%purge% self
~
#2126
baby bullant becomes a worker bullant~
0 n 100
~
wait 60s
emote grows a little stronger
wait 60s
emote grows a little stronger
wait 60s
emote grows a little stronger
wait 60s
emote grows a little stronger
wait 60s
emote grows a little stronger
wait 120s
emote grows a little stronger
wait 120s
emote grows a little stronger
wait 120s
emote grows a little stronger
wait 120s
emote grows a little stronger
wait 125s
emote suddenly has a growth spurt and becomes a worker!
%load% mob 2112
%purge% self
~
#2127
Worker/soldier bullant - load~
0 n 100
~
wait 2
emote feels much stronger!
~
#2128
Baby bullant is born~
0 n 100
~
wait 2
emote is born into the world!
~
#2129
*empty trigger*~
1 c 100
~
* Empty script~
#2130
new trigger~
0 g 100
~

~
$~
