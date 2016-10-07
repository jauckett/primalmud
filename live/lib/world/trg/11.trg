#1100
memory test trigger~
0 o 100
~
* assign this to a mob, force the mob to mremember you, then enter the
* room the mob is in while visible (not via goto)
wait 1
say I remember you, %actor.name%!
~
#1102
mob greet test~
0 ag 100
~
if %direction%
  wait 3
  say Hello, %actor.name%, how are things to the %direction%?
mremember %actor%
else
* if the character popped in (word of recall, etc) this will be hit
  wait 3
  say Where did YOU come from, %actor.name%?
end
~
#1103
obj get test~
1 g 100
~
%echo% You hear, 'Please put me down, %actor.name%'
~
#1104
room test~
2 g 100
~
wait 50
wsend %actor% you enter a room
~
#1105
car/cdr test~
0 d 1
this is a piece of string~
 say speech: %speech%
 say car: %speech.car%
 say cdr: %speech.cdr%
say strlen: %strlen%
* OK THIS DOES NOT SEEM TO WORK CORRECTLY - NOR DOES STRLEN
~
#1106
subfield test~
0 c 100
test~
* test to make sure %actor.skill(skillname)% works
say your hide ability is %actor.skill(hide)% percent.
*
* make sure %actor.eq(name)% works too
eval headgear %actor.eq(head)%
if %headgear%
  say You have some sort of helmet on
else
  say Where's your headgear?
  halt
end
say Fix your %headgear.name%
~
#1107
object otransform test~
1 jl 7
test~
* test of object transformation (and remove trigger)
* test is designed for objects 3311 and 3322
* assign the trigger then wear/remove the item
* repeatedly.
%echo% Beginning object transform.
if %self.vnum% == 3311
  otransform 3322
else 
  otransform 3311 
end
%echo% Transform complete.
~
#1108
makeuid and remote testing~
2 c 100
test~
* DM - AGAIN COULDN'T TRIGGER - I THINK ITS JUST CAUSE I DONT KNOW HOW TO
* TRIGGER IT
* makeuid test ---- assuming your MOBOBJ_ID_BASE is 200000,
* this will display the names of the first 10 mobs loaded on your MUD,
* if they are still around.
eval counter 0
while (%counter% < 10)
  makeuid mob 200000+%counter%
  %echo% #%counter%      %mob.id%   %mob.name%
  eval counter %counter% + 1
done
%echoaround% %actor% %actor.name% cannot see this line.
*
*
* this will also serve as a test of getting a remote mob's globals.
* we know that puff, when initially loaded, is id 200000. We'll use remote
* to give her a global, then %mob.globalname% to read it.
makeuid mob 200000
eval globalname 12345
remote globalname %mob.id%
%echo% %mob.name%'s "globalname" value is %mob.globalname%
~
#1109
mtransform test~
0 g 100
~
* DM - WORKS FINE
* mtransform test
* as a greet trigger, entering the room will cause
* the mob this is attached to, to toggle between mob 1101 and 1102.
%echo% Beginning transform.
if %self.vnum%==1101
  mtransform -1102
else
  mtransform -1101
end
%echo% Transform complete.
~
#1110
attach test~
0 d 100
attach~
attach 9 %self.id%
~
#1111
attach test~
0 d 100
detach~
detach 9 %self.id%
~
#1112
spellcasting test~
0 c 100
kill~
* This command trigger will disallow anyone from trying to
* use the kill command, and will toss a magic missile at them
* for trying.
dg_cast 'magic missile' %actor%
return 0
~
#1113
new trigger~
0 g 100
~
say my command list is incomplete
~
#1114
new trigger~
0 d 100
~
say my command list is incomplete
~
#1115
new trigger~
0 c 0
~
say my command list is incomplete
~
#1135
Thief guildmaster trigger~
0 g 100
~
wait 2
say Hello %actor.class% %actor.name%
~
#1150
cleric greet~
0 g 100
~
if %actor.class% == cleric
say welcome %actor.name%, my child.
elseif %actor.class% == warrior
%send% %actor% an Old Cleric tells you, 'Your guild is 2 south then 1 west from where you start at main street.'
say go in peace %actor.name%.
elseif %actor.class% == thief
%send% %actor% an Old Cleric tells you 'Your guild is 2 north, then 2 west, then 1 south from where you start at main street.'
say go in peace %actor.name%.
elseif %actor.class% == magician
%send% %actor% an Old Cleric tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
say go in peace %actor.name%
else
say go in peace %actor.name%
end
end
wait 1
if %actor.vnum% < 0
~
#1151
mage greet~
0 g 100
~
if %actor.vnum% <0
if %actor.class% == magician
say Welcome %actor.name%, my fine apprentice.
elseif %actor.class% == cleric
%send% %actor% an Aging Witch tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
say Great smoking fireballs %actor.name%, this is not your guild.
elseif %actor.class% == warrior
%send% %actor% an Aging Witch tells you, 'Your guild is 2 south then 1 west from where you start at main street.'
say Great smoking fireballs %actor.name%, this is not your guild.
elseif %actor.class% == thief
%send% %actor% an Aging Witch tells you, 'Your guild is 2 north then 2 west then 1 south from where you start at main street.'
say Great smoking fireballs %actor.name%, this is not your guild.
else
say Great smoking fireballs %actor.name%, this is not your guild.
end
end
wait 1
~
#1152
warrior greet~
0 g 100
~
if %actor.vnum% <0
if %actor.class% == warrior
say Welcome noble %actor.name%.
elseif %actor.class% == cleric
%send% %actor% a Seasoned Warrior tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
%echo% a Seasoned Warrior shakes his head and says, '%actor.name%, ye must learn where yer own guild is.'
elseif %actor.class% == thief
%send% %actor% a Seasoned Warrior tells you, 'Your guild is 2 north then 2 west then 1 south from where you start at main street.'
%echo% a Seasoned Warrior shakes his head and says, '%actor.name%, ye have to learn where yer own guild is.'
elseif %actor.class% == magician
%send% %actor% a Seasoned Warrior tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
%echo% a Seasoned Warrior shakes his head and says, '%actor.name%, ye have to learn where yer own guild is.'
else
%echo% a Seasoned Warrior shakes his head and says, '%actor.name%, ye have to learn where yer own guild is.'
end
end
wait 1
~
#1153
Thief greeting~
0 g 100
~
if %actor.vnum% <0
if %actor.class% == thief
say Welcome %actor.name%, my little shadow.
elseif %actor.class% == warrior
%send% %actor% the Master of Thieves tells you, 'Your guild is 2 south then 1 west from where you start at main street.'
say find thee thine own guild %actor.name%, lest thy money and thyself be shortly parted.
elseif %actor.class% == cleric
%send% %actor% the Master of Thieves tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
say find thee thine own guild %actor.name%, lest thy money and thyself be shortly parted.
elseif %actor.class% == magician
%send% %actor% the Master of Thieves tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
say Find thee thine own guild %actor.name%, lest thy money and thyself be shortly parted.
else
say Find thee thine own guild %actor.name%, lest thy money and thy self be shortly parted.
end
end
wait 1
~
#1154
priest greeting~
0 g 100
~
if %actor.vnum% <0
if %actor.class% == priest
say Welcome %actor.name%, happy day.
elseif %actor.class% == cleric
%send% %actor% Priest tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
%echo% The priest mutters something about adventurers who can't find their guilds in latin.
elseif %actor.class% == warrior
%send% %actor% Priest tells you, 'your guild is 2 south then 1 west from where you start at main street.'
%echo% The priest mutters something about adventurers who can't find their guilds in latin.
elseif %actor.class% == magician
%send% %actor% Priest tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
%echo% The priest mutters something about adventurers who can't find their guilds in latin.
elseif %actor.class% == thief
%send% %actor% Priest tells you, 'Your guild is 2 north then 2 west then 1 south from where yous tart at main street.'
%echo% The priest mutters something about adventurers who can't find their guilds in latin.
else
%echo% The priest mutters something about adventurers who can't find their guilds in latin.
end
end
wait 1
~
#1155
NightBlade greeting~
0 g 100
~
if %actor.vnum% < 0
if %actor.class% == nightblade
say Welcome %actor.name%, shadow warrior.
elseif %actor.class% == cleric
%send% %actor% the NightBlade Guildmaster tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
%echo% The NightBlade Guildmaster jabs her sword in the direction of the door, suggesting that %actor.name% find their own guild.
elseif %actor.class% == warrior
%send% %actor% the NightBlade Guildmaster tells you, 'Your guild is 2 south then 1 west from where you start at main street.'
%echo% The NightBlade Guildmaster jabs her sword in the direction of the door suggesting that %actor.name% find their own guild.
elseif %actor.class% == thief
%send% %actor% the NightBlade Guildmaster tells you, 'Your guild is 2 north then 2 west then 1 south from where you start at main street.'
%echo% The NightBlade Guildmaster jabs her sword in the direction of the door, suggesting that %actor.name% find their own guild.
elseif %actor.class% == magician
%send% %actor% the NightBlade Guildmaster tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
%echo% The NightBlade guildmaster jabs her sword in the direction of the door, suggesting that %actor.name% find their own guild.
else
%echo% The NightBlade Guildmaster jabbs her sword in the direction of the door, suggesting that %actor.name% find their own guild.
end
end
wait 1
~
#1156
SpellSword greeting~
0 g 100
~
if %actor.vnum% < 0
if %actor.class% == spellsword
say Welcome %actor.name%, mystical shadow.
elseif %actor.class% == cleric
%send% %actor% the Guildmaster tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
say Alas %actor.name%, your destiny does not lie within this guild.
elseif %actor.class% == warrior
%send% %actor% the Guildmaster tells you, 'Your guild is 2 south then 1 west from main street.'
say Alas %actor.name%, your destiny does not lie within this guild.
elseif %actor.class% == thief
%send% %actor% the Guildmaster tells you, 'Your guild is 2 north then 2 west then 1 south from where yous tart at main street.'
say Alas %actor.name%, your destiny does not lie within this guild.
elseif %actor.class% == magician
%send% %actor% the Guildmaster tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
say Alas %actor.name% your destiny does not lie within this guild.
else
say Alas %actor.name%, your destiny does not lie within this guild.
end
end
wait 1
~
#1157
battlemage greeting~
0 g 100
~
if %actor.vnum% < 0
if %actor.class% == battlemage
say Welcome %actor.name%, mystical warrior.
elseif %actor.class% == cleric
%send% %actor% the BattleMage master tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
%send% %actor% It might be your instinct, or it might be the deadly sparks flying from the sword of the master,
%send% %actor% but you get the impression you aren't welcome here.
elseif %actor.class% == warrior
%send% %actor% the BattleMage Master tells you, 'Your guild is 2 south then 1 west from where you start at main street.'
%send% %actor% it might be your instinct, or it might be the deadly sparks flying from the sword of the master,
%send% %actor% but you get the impression you are not welcome here.
elseif %actor.class% == thief
%send% %actor% the BattleMage master tells you 'your guild is 2 north then 2 west then 1 south from where you start at main street.'
%send% %actor% It might be your instinct, or it might be the deadly sparks flying from the sword of the master,
%send% %actor% but you get the impression you are not welcome here.
elseif %actor.class% == magician
%send% %actor% 'the BattleMage master tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
%send% %actor% it might be your instinct, or it might be the deadly sparks flying from the sword of the master,
%send% %actor% but you get the impression you are not welcome here.
else
%send% %actor% It might be your instinct, or it might be the deadly sparks flying from the sword of the master,
%send% %actor% but you get the impression you are not welcome here.
end
end
wait 1
~
#1158
Paladin greeting~
0 g 100
~
if %actor.vnum% < 0
if %actor.class% == paladin
say Welcome %actor.name%, holy warrior.
elseif %actor.class% == cleric
%send% %actor% the Paladin Master tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
say I am sorry %actor.name%, only the most holy may study here.
elseif %actor.class% == warrior
%send% %actor% 'the Paladin Master tells you, 'Your guild is 2 south then 1 west from where you start at main street.'
say I am sorry %actor.name%, only the most holy may study here.
elseif %actor.class% == thief
%send% %actor% the Paladin Master tells you, 'Your guild is 2 north then 2 west then 1 south from where you start at main street.'
say I am sorry %actor.name%, only the most holy may study here.
elseif %actor.class% == magician
%send% %actor% the Paladin Master tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
say I am sorry %actor.name%, only the most holy may study here.
else
say I am sorry %actor.name%, only the most holy may study here.
end
end
wait 1
~
#1159
Druid greeting~
0 g 100
~
if %actor.vnum% < 0
if %actor.class% == druid
say Welcome %actor.name%, the force is strong within you.
elseif %actor.class% == cleric
%send% %actor% the Druid Master tells you, 'Your guild is 1 south then 3 west then 1 south from where you start at main street.'
say Your aura is all wrong %actor.name%, I don't think I can help you.
elseif %actor.class% == warrior
%send% %actor% the Druid Master tells you, 'Your guild is 2 south then 1 west from where you start at main street.'
say Your aura is all wrong %actor.name%, I don't think I can help you.
elseif %actor.class% == thief
%send% %actor% the Druid Master tells you, 'Your guild is 2 north then 2 west then 1 south from where you start at main street.'
say Your aura is all wrong %actor.name%, I don't think I can help you.
elseif %actor.class% == magician
%send% %actor% the Druid Master tells you, 'Your guild is 1 south then 3 east then 1 south from where you start at main street.'
say Your aura is all wrong %actor.name%, I don't think I can help you.
else
say Your aura is all wrong %actor.name%, I don't think I can help you.
end
end
wait 1
~
#1160
gamina1~
0 d 100
"heal please"~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
if (%actor.fighting%)
tell %actor.name% Now, %actor.name%, we don't want any trouble!
halt
end
if (%actor.level% <= 15)
%echo% Gamina waves her hands and utters strange incantations.
dg_cast 'heal' %actor%
wait 1
halt
end
%echo% Gamina smiles sweetly at %actor.name%
tell %actor.name% I think you've been around long enough to be able to heal yourself, don't you?
~
#1161
gamina2~
0 d 100
heal~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
tell %actor.name% If you wish me to help you, perhaps you should be more polite.
~
#1162
gamina3~
0 d 100
"spells please"~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
if (%actor.level% <= 15)
%echo% Gamina makes several strange gestures. An aura of bright light surrounds %actor.name%
dg_cast 'sanctuary' %actor%
dg_cast 'bless' %actor%
dg_cast 'armor' %actor%
wait 1
halt
end
%echo% Gamina smiles sweetly at %actor.name%
tell %actor.name% I think you've been here long enough to be able to look after yourself now, don't you?
~
#1163
gamina4~
0 d 100
spells~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
tell %actor.name% If you wish me to help you, perhaps you should be more polite.
~
#1165
gamina6~
0 d 100
"eq please"~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
if !(%actor.level% < 10)
emote smiles at %actor.name%
tell %actor.name% I think you've been around long enough to get eq for yourself.
halt
end
if (%actor.class% == cleric)
junk all
%load% obj 1379
%load% obj 1380
%load% obj 1381
%load% obj 1382
%load% obj 1383
%load% obj 1101
%load% obj 1126
%load% obj 1351
give all %actor.name%
junk all
wait 1
halt
end
if (%actor.class% == thief)
junk all
%load% obj 1384
%load% obj 1385
%load% obj 1386
%load% obj 1387
%load% obj 1388
%load% obj 1101
%load% obj 1126
%load% obj 1351
give all %actor.name%
junk all
wait 1
halt
end
if (%actor.class% == warrior)
junk all
%load% obj 1374
%load% obj 1375
%load% obj 1376
%load% obj 1377
%load% obj 1378
%load% obj 1101
%load% obj 1126
%load% obj 1351
give all %actor.name%
junk all
wait 1
halt
end
if (%actor.class% == magician)
junk all
%load% obj 1369
%load% obj 1370
%load% obj 1371
%load% obj 1372
%load% obj 1373
%load% obj 1101
%load% obj 1126
%load% obj 1351
give all %actor.name%
junk all
wait 1
halt
end
emote smiles at %actor.name%
tell %actor.name% nice try, but someone who's remorted should know where to get the basics to survive.
~
#1166
gamina7~
0 d 100
eq~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
tell %actor.name% If you wish me to help you, perhaps you should be more polite.
~
#1167
gamina8~
0 g 40
~
if ((%actor.canbeseen%) && (%actor.level% <= 15) && (%actor.vnum% < 0))
tell %actor.name% I can help you if you are stuck, just say 'help please' in this room.
end
~
#1168
gamina9~
0 d 100
"help please"~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
if (%actor.level% > 15)
tell %actor.name% You're a little too advanced for me to help.
halt
end
emote smiles sweetly at %actor.name%
tell %actor.name% If you need some protective spells to get you started, say 'spells please'
tell %actor.name% If you are hurt and need to be healed, say 'heal please'
tell %actor.name% If you are exhausted, and need more movement, say 'refresh pelase'
say Have fun! :o)
wait 1
~
#1169
gamina10~
0 d 100
"refresh please"~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
if (%actor.fighting%)
tell %actor.name% Now, %actor.name%, we don't want any trouble here!
halt
end
if (%actor.level% > 15)
tell %actor.name% I think you're big enough to look after yourself now, %actor.name%
halt
end
dg_cast 'refresh' %actor%
wait 1
halt
~
#1170
gamina11~
0 d 100
refresh~
if !((%actor.canbeseen%) && (%actor.vnum% < 0))
halt
end
tell %actor.name% If you want me to help you, you're going to have to be more polite.
~
#1171
Thief Short Cut~
0 g 100
~
eval class %actor.class%
if (%class% == thief)
tell %actor.name% Do you want to go home? 
tell %actor.name% Just say the magic words!
~
#1172
new trigger~
0 d 100
Take me home~
if (%speech% == Take me home)
%teleport% %actor% 10012
~
#1180
jail noquit~
2 c 100
qui~
%send% %actor% No, that'd be too easy!
~
#1181
jail nogoto~
2 c 100
got~
if %actor.level% > 99
%send% %actor% Nice try, but you can't goto out of jail!
else 
%send% %actor% Huh?!?
~
#1182
jail noat~
2 c 100
at~
if %actor.level% >109
%send% %actor% Wouldn't it be nice if you could use that command.
else
%send% %actor% Huh?!?
~
#1183
jail nosuicide~
2 c 100
sui~
%send% %actor% Suicide is... well, not allowed here!
~
#1198
Artus Test~
2 c 1
enter~
echo Artus sux.
~
#1199
mdamage test~
0 g 100
~
%damage% %actor% 100
say did that hurt?
~
#1200
actor name test~
0 g 100
~
say Hello %actor.name% how are you?
~
#1201
Temp~
2 c 100
testid~
wecho My idnum is: %self.id%
eval rnum %self.id% - 50000
wecho My rnum should be: %rnum%
~
#1252
Mist Blocking Trigger~
2 g 100
~
if %actor.level% > 20
wait 1
else
wait 15
%send% %actor% You wander around helplessly in the mist, trying to find your way.
wait 15
%send% %actor% Forward, backwards, anywhere!
wait 15
%send% %actor% As you start darting from side to side, looking for a way out, for absolution, your head locates a cold, moist, hard rock.
wait 15
%teleport% %actor% 1112
end
wait 1
~
#1255
Arch Restriction Trigger~
1 c 100
go~
if %actor.class% == master
%teleport% %actor% 1244
%force% %actor% look
else
%send% %actor% Sorry, only those who have attained the exulted status of master may pass through the arch.
%teleport% %actor% 1251
end
wait 1
~
#1299
Auction Room~
2 c 100
*~
if ((%cmd% == goto) || (%cmd.mudcommand% == goto))
return 0
halt
end
wsend %actor% No. This is the auction room. Get the fuck out.
wsend %actor% Available Commands: goto.
~
$~
