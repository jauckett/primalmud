#1
memory test trigger~
0 o 100
~
* assign this to a mob, force the mob to mremember you, then enter the^M
* room the mob is in while visible (not via goto)^M
say I remember you, %actor.name%!^M
~
#2
mob greet test~
0 g 100
~
if %direction%^M
  say Hello, %actor.name%, how are things to the %direction%?^M
else^M
* if the character popped in (word of recall, etc) this will be hit^M
  say Where did YOU come from, %actor.name%?^M
end^M
~
#3
obj get test~
1 g 100
~
%echo% You hear, 'Please put me down, %actor.name%'^M
~
#4
room test~
2 g 100
~
wait 50^M
wsend %actor% you enter a room^M
~
#5
car/cdr test~
0 d 100
test~
say speech: %speech%^M
say car: %speech.car%^M
say cdr: %speech.cdr%^M
~
#6
subfield test~
0 c 100
test~
* test to make sure %actor.skill(skillname)% works^M
say your hide ability is %actor.skill(hide)% percent.^M
*^M
* make sure %actor.eq(name)% works too^M
eval headgear %actor.eq(head)%^M
if %headgear%^M
  say You have some sort of helmet on^M
else^M
  say Where's your headgear?^M
  halt^M
end^M
say Fix your %headgear.name%^M
~
#7
object otransform test~
1 jl 7
test~
* test of object transformation (and remove trigger)^M
* test is designed for objects 3020 and 3021^M
* assign the trigger then wear/remove the item^M
* repeatedly.^M
%echo% Beginning object transform.^M
if %self.vnum% == 3020^M
  otransform 3021^M
else^M
  otransform 3020^M
end^M
%echo% Transform complete.^M
~
#8
makeuid and remote testing~
2 c 100
test~
* makeuid test ---- assuming your MOBOBJ_ID_BASE is 200000,^M
* this will display the names of the first 10 mobs loaded on your MUD,^M
* if they are still around.^M
eval counter 0^M
while (%counter% < 10)^M
  makeuid mob 200000+%counter%^M
  %echo% #%counter%      %mob.id%   %mob.name%^M
  eval counter %counter% + 1^M
done^M
%echoaround% %actor% %actor.name% cannot see this line.^M
*^M
*^M
* this will also serve as a test of getting a remote mob's globals.^M
* we know that puff, when initially loaded, is id 200000. We'll use remote^M
* to give her a global, then %mob.globalname% to read it.^M
makeuid mob 200000^M
eval globalname 12345^M
remote globalname %mob.id%^M
%echo% %mob.name%'s "globalname" value is %mob.globalname%^M
~
#9
mtransform test~
0 g 100
~
* mtransform test^M
* as a greet trigger, entering the room will cause^M
* the mob this is attached to, to toggle between mob 1 and 99.^M
%echo% Beginning transform.^M
if %self.vnum%==1^M
  mtransform -99^M
else^M
  mtransform -1^M
end^M
%echo% Transform complete.^M
~
#10
attach test~
0 d 100
attach~
attach 9 %self.id%^M
~
#11
attach test~
0 d 100
detach~
detach 9 %self.id%^M
~
#12
spellcasting test~
0 c 100
kill~
* This command trigger will disallow anyone from trying to^M
* use the kill command, and will toss a magic missile at them^M
* for trying.^M
dg_cast 'magic missile' %actor%^M
return 0^M
~
$~
