#30800
Jester info~
0 ghi 100
~
if %canbeseen% ==
say %actor.name% my saviour!
smile
wait 10
sta
say You have somehow managed to kill that bastard of a guard!
smile
wait 30
say Although you do not have to, I would appreciate it if you let me go. I could make it worth your while for a price...
wait 45
say for 10 million gold coins and a way out of here I could help you also.
else
say Welcome %actor.name%. You do realise that even though you somehow made it here, 
your end is near.
end
end
~
#30801
Royal Guards - welcome message~
0 ghi 100
~
if %actor.canbeseen%
if (%direction% == down)
%echo% The guard blocks the entrance to the North.
say HALT %actor.name%! Nobody sees the King at all.
wait 15
say Now.....be off with you. We do not want any trouble!
poke %actor.name%
end
end
~
#30802
King calls upon queen~
0 l 75
~
if %actor.canbeseen%
eval room %actor.room%
if %room.vnum% == 30957
if (%load_queen% != 1)
%echo% The King Death Shadow's hollers, Ohhhhh Darliiiing!
wait 40
%load% mob 30815
set load_queen 1
global load_queen
%load% obj 30874
give key queen
%force% queen hold key
end
end
end
~
#30803
Death shadow - Lay down and die~
0 bhi 20
~
if %actor.vnum% < 0
say You've got to be kidding %actor.name%! Why don't you just kill yourself now and save us all the trouble!?
wait 30
It would be the wise thing to do...
end
~
#30804
Portal guard halt~
0 ghi 100
~
if %actor.canbeseen%
if (%direction% == down)
say HALT!! You cannot go any further! His highness has ordered me not to be disturbed! 
say Turn back now %actor.name%, you have 10 seconds!
wait 10s
eval room %actor.room%
if %room.vnum% == 30876
say I warned you %actor.name% and you have pushed the limit of my patience, cop this!
mkill %actor.name%
end
end
end
~
#30805
Portal guard greet~
0 g 100
~
say You are not permitted to go any futher %actor.name%. If you go any further I will be forced to kill you.
smile %actor.name%
~
#30806
Load 2 guards before throne apon entry~
2 c 100
o~
if (%cmd% == open)
say I don't think so %actor.name%!!
shout INTRUDER!
%echo% An invisible door to the east opens and 2 guards step out!
wait 5
%echo% The door closes even though you know it's not really there!
%load% mob 30816
%load% mob 30816
%force% guard shake
%force% 2.guard shake
%force% guard say Hey %actor.name%, where do you think you're going?!
%force% 2.guard say Hey %actor.name%, where do you think you're going?!
wait 30
%force% guard kill %actor.name%
%force% 2.guard kill %actor.name%
end
end
~
#30807
Death shadow is the Kings servant~
0 bg 40
~
if %actor.vnum% < 0
say I serve the almighty King himself!
end
~
#30808
Death Shadow Kings breaksfast has arrived~
0 bg 10
~
if %actor.vnum% < 0
say If I had to choose between living here and dying....DYING for sure!
wait 10
say Now that I have warned you, would you do me the honour of killing me?
smile
end
~
#30809
Portal guard death - load another~
0 f 100
~
if (%load_guard% != 1)
%echo% There is a bright flash of light and another Portal Guard appears!
%load% mob 30857
wait 10
say %actor.name% you FOOL! If there was no REAL security for his magesty, we would not be here!
wait 15
say %actor.name% you don't learn do you. We have sworn with our lives to protect his magesty, you are NOT getting passed!
set load_guard 1
global load_guard
end
~
#30810
Bat-attack~
0 ghi 100
~
if %actor.vnum% < 0
if %self.canbeseen%
wait 5
say hmmmm nummy, fresh blooood! You can't escape now %actor.name%!
wait 5
mkill %actor.name%
else
say I need some fffffffresh bloooooood!
end
end
~
#30811
Well beast hungry~
0 ghi 100
~
if %actor.canbeseen%
if (%direction% == south)
say I hope you are healthy %actor.name%, healthy people taste better!
wait 10
kill %actor.name%
elseif (%direction% == up)
say I hope you are healthy %actor.name%, healthy people taste better!
wait 10
kill %actor.name%
end
end
end
~
#30812
Death shadow-not passing me~
0 g 100
~
say You must be a worth fighter to make it here %actor.name%
wait 5
say If you want to go any further, you'll have to fight me!
wait 5
grin
~
#30813
Spider hiss and attack~
0 g 100
~
if %actor.canbeseen%
emote hisses at you!
wait 15
kill %actor.name%
end
~
#30814
Royal Guard attack if go north~
0 c 100
n~
%send% %actor% You are grabbed by a guard and whacked in the back of the head!
%damage% %actor% 500
%echoaround% %actor% The trouble maker is grabbed by a guard and whacked in the back of the head! OUCH!
wait 30
%send% %actor% The guard throws you down the stairs!
%echoaround% %actor% The trouble maker is then thrown down the stairs! I wouldn't attempt to pass if I were you!!
%teleport% %actor% 30954
say I told %actor.name% no further....bloody deaf %actor.class%
%force% %actor% look
wait 5
%send% %actor% You bounce off a step!
%damage% %actor% 250
%teleport% %actor% 30953
wait 5
%force% %actor% look
%send% %actor% You bounce off a another step....OUCH again!
%damage% %actor% 250
%teleport% %actor% 30952
%send% %actor% You land at the bottom of the stairs with a loud THUD..banging your head on the floor....NIGHTY NIGHT!
%damage% %actor% 450
%force% %actor% sleep
Shout Anyone else wanna go me? EH EH EH?!?!
end
~
#30815
Call on baby spiders to assist~
0 l 50
~
context %self.id%
if (%already_fighting%)
unset already_fighting
wait 300
%purge% %30811%
say Ok enough of this playing around %actor.name%!
%purge% %30811%
else
shout CHILDREEEEEEN, %actor.name% is here for dinner!
wait 30
%load% mob 30811
%load% mob 30811
set already_fighting 1
global already_fighting
wait 300
end
~
#30816
Stone door closes upon entry~
2 g 100
~
If (%direction% == east)
emote the Stone door closes behind you!
close stone
end
~
#30817
Head guard - piss off~
0 hi 100
~
if %actor.canbeseen%
if (%direction% == south)
say I don't think so %actor.name%...his magesty is busy....go away!
wait 75
eval room %actor.room%
if %room.vnum% == 30956
say OI %actor.name% ARE YOU DEAF? PISS OFF!
wait 45
eval room %actor.room%
if %room.vnum% == 30956
say GUARDS fix up this %actor.sex%!
end
end
end
end
~
#30818
Head guard close door~
0 hi 100
~
if %actor.canbeseen%
if (%direction% == north)
remove desk
%load% obj 30812
get security
hold security
wait 5
say Next time close his magesties door %actor.name%!
emote walks over to the Throne Room door.
wait 10
close throne
lock throne
wait 5
emote walks back to his desk
say Now PISS off, I have work to do!
wait 30
sit
wait 5
remove security
hold desk
wait 5
emote throws the Throne Key back in the draw
mjunk all
wait 5
growl
end
end
~
#30819
mini devil speak then attack~
0 ghi 100
~
if %actor.canbeseen%
say You have come far %actor.name%, yet your about to realise comming here was a bad idea!
wait 5
shout Your going to die %actor.name%!
wait 10
kill %actor.name%
end
~
#30820
Purple octopus defence~
0 ce 100
d~
if (%cmd% == down)
say Over my dead body %actor.name%!
wait 15
sta
kill %actor.name%
elseif (%cmd% == dow)
say Over my dead body %actor.name%!
wait 15
sta
kill %actor.name%
elseif (%cmd% == do)
say Over my dead body %actor.name%!
wait 15
sta
kill %actor.name%
elseif (%cmd% == d)
say Over my dead body %actor.name%!
wait 15
sta
kill %actor.name%
end
end
end
end
~
#30821
Portal guard attack if try open stone~
0 c 100
unlock~
if %actor.canbeseen%
say Over my dead body %actor.name%!
kill %actor.name%
else
say Bloody ghosts around here!
end
~
#30822
Spider mother poison bite~
0 k 100
~
* get players health and store it as variable %dam%
eval dam %actor.hitp% / 10
* face bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother takes a bite at your face but misses!
%echoaround% %actor% The spider mother takes a bite at %actor.name%'s face but misses!
wait 50
end
end
* face bite get
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother lunges at your face and bites it!
%echoaround% %actor% The spider mother lunges at %actor.name%'s face and bites it!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
* leg bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother lunges for a bite at your leg but misses!
%echoaround% %actor% The spider mother lunges for a bite at %actor.name%'s leg but misses!
wait 50
end
end
*leg bite get
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother lunges at your face and bites it! YIKES!
%echoaround% %actor% The spider mother lunges at %actor.name%'s face and bites it! YIKES!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
* arm bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother lunges for a bite at your arm, but smacks into the ground instead!! HAHAHA!
%echoaround% %actor% The spider mother lunges for a bite at %actor.name%'s arm, but smacks into the ground instead!! HAHAHA!
wait 50
end
end
* arm bite get
f %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother lunges for a bite at your arm and is successful! YEEEOOOWWWCCCH!
%echoaround% %actor% The spider mother lunges for a bite at %actor.name%'s arm and is successful! YEEEOOOWWWCCCH!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
* groin bite miss
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother lunges for a bite at your groin, but smacks into the ground instead!! HAHAHA!
%echoaround% %actor% The spider mother lunges for a bite at %actor.name%'s groin, but smacks into the ground instead!! HAHAHA!
wait 50
end
end
* groin bite get
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% == 2125 || %room.vnum% == 2126 || %room.vnum% == 2127)
%send% %actor% The spider mother lunges for a bite at your groin and is successful! OOOOOOOHHH THE PAIN!!!
%echoaround% %actor% The spider mother lunges for a bite at %actor.name%'s groin and is successful! OOOOOOOHHH THE PAIN!!!
%damage% %actor% %dam%
dg_cast 'poison' %actor.name%
wait 75
end
end
~
#30823
Head guard attack~
0 c 100
unlock~
if %actor.canbeseen%
say Over my dead body %actor.name%!
stand
wait 5
mkill %actor.name%
end
~
#30824
Crocodile attack on enter~
0 ghi 100
~
if %actor.canbeseen%
emote bites your leg as you go into its lair!
%damage% %actor% 500
emote swallows the chunk
stare %actor.name%
wait 2s
kill %actor.name%
end
~
#30825
Purge death shadow blade on death~
0 f 100
~
%purge% blade
~
#30826
guards before head guard - attack if no north~
0 c 100
n~
if ((%actor.canbeseen%) && (%actor.level% < 101))
%send% %actor% You are grabbed by a guard and whacked in the back of the head!
%damage% %actor% 500
%echoaround% %actor% The trouble maker is grabbed by a guard and whacked in the back of the head! OUCH!
wait 30
%send% %actor% The guard throws you down the stairs!
%echoaround% %actor% The trouble maker is then thrown down the stairs! I wouldn't attempt to pass if I were you!!
%teleport% %actor% 30954
say I told %actor.name% no further....bloody deaf %actor.class%
%force% %actor% look
wait 5
%send% %actor% You bounce off a step!
%damage% %actor% 250
%teleport% %actor% 30953
wait 5
%force% %actor% look
%send% %actor% You bounce off a another step....OUCH again!
%damage% %actor% 250
%teleport% %actor% 30952
%send% %actor% You land at the bottom of the stairs with a loud THUD..banging your head on the floor....NIGHTY NIGHT!
%damage% %actor% 450
%force% %actor% sleep
Shout Anyone else wanna go me? EH EH EH?!?!
return 1
halt
end
if ((%actor.canbeseen%) && (%actor.level% > 100))
say over my dead body %actor.name%!
mkill %actor.name%
return 1
halt
end
return 0
~
#30827
Purple dragon protect portal~
0 c 100
go~
if %actor.canbeseen%
emote hisses at %actor.name% and smoke begins to rise from her nostrels!.
wait 45
eval dragon %actor.room%
if %dragon.vnum% == 30810
emote hisses even louder at %actor.name%!
wait 70
eval dragon %actor.room%
if %dragon.vnum% == 30810
sta
emote breathes fire at %actor.name% and %actor.name% is scorched in flames!
%damage% %actor% 1000
kill %actor.name%
wait 300
eval dragon %actor.room%
if %dragon.vnum% == 30810
sit
else
emote moves a little and scratches itself.
end
end
end
end
~
#30828
Death dragon burn~
0 k 100
~
wait 200
eval dragon %actor.room%
if %dragon.vnum% == 30810
emote breathes fire at %actor.name% and %actor.name% is scorched in flames!
%damage% %actor% 1000
wait 100
~
#30829
Genetically altered shark circling attack~
0 ghi 50
~
* Artus> This one is hella broken.
halt
if %actor.canbeseen%
if %actor.vnum% < 0
eval room %actor.room%
if room ==
emote takes a chunk out of %actor.name%'s ass
%damage% %actor% 300
emote swallows it whole
grin
kill %actor.name%
else smile
end
end
~
#30830
Death shadow prevent lost pyramid unlocking~
0 c 100
unlock~
if %actor.canbeseen%
emote lets out an ear piercing scream!
dg_cast 'fireball' %actor.name%
else
%echo% A strange feeling prevents you from unlocking the Pyramid.
end
~
#30831
Jester bribe~
0 m 10000000
~
if (%amount% == 10000000)
wait 15
%load% obj 8400
emote makes a magical jesture and creates a shiny serpent's eye!
wait 5
%load% obj 30898
emote makes a magical jesture and creates a Moogaheal!
p eye bag
p mooga bag
drop all
smile
tell %actor.name% Use these wisely! Keep it to yourself. They would kill me if the King found out!
wait 5
wave
wait 5
emote runs out of the cell to his freedom.
mpurge self
else
set has_bribed_jester 1
global has_bribed_jester
tell %actor.name% I said 10 million %actor.name%. I need to leave if I tell you this!
wait 5
give %amount% %actor.name%
end
~
#30832
Jester is free~
2 d 100
Royal Jester says, 'I better get going~
%purge% mob 30822
~
#30833
Purple octopus special~
0 k 100
~
emote wrapps a tentacle around your body and squeezes!
%damage% %actor% 250
wait 6s
~
#30834
mid royal guard attack~
0 ghi 100
~
if %actor.canbeseen%
if (%direction% == south)
wait 130
eval room %actor.room%
if %room.vnum% == 30956
say You should have listened to him %actor.name%!
emote smacks you across the head
%damage% %actor% 250
say I'd leave if I were you because the next one ain't gonna be so pretty!
wait 2s
eval room %actor.room%
if %room.vnum% == 30956
emote smacks you in the face!
%damage% %actor% 500
mkill %actor.name%
end
end
end
end
~
#30835
King special #1 - 1000 damage~
0 k 40
~
* get players health and store it as variable %dam%
eval dam %actor.hitp% / 3
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% > 30800) && (%room.vnum% < 30999)
grin %actor.name%
%send% %actor% King Death Shadow calls upon his GOD for assistance.
%echoaround% %actor% King Death Shadow calls upon his GOD for assistance.
%damage% %actor% %dam%
%send% %actor% King Death Shadow's call is answered and you are burnt by a fireball sent from above!
%echoaround% %actor% King Death Shadow's call is answered and %actor.name% is burnt by a fireball sent from above!
%send% %actor% The damage inflicted on you by the fireball is given to your attacker!
%echoaround% %actor% The damage inflicted on %actor.name% by the fireball is given to their attacker!
mrestore 500
wait 10s
end
end
~
#30836
*free trigger - dun work*~
0 k 40
~
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% > 30800) && (%room.vnum% < 30999)
grin %actor.name%
piledrive %actor.name%
wait 5
eval room %actor.room%
if (%room.vnum% > 30800) && (%room.vnum% < 30999)
bearhug %actor.name%
wait 5
eval room %actor.room%
if (%room.vnum% > 30800) && (%room.vnum% < 30999)
bodyslam %actor.name%
wait 6s
end
end
end
end
~
#30838
waterfall move player - top to bottom~
2 g 100
~
if %actor.canbeseen%
wait 5
%send% %actor% The current is so strong you can't move back from where you came from!
%echoaround% %actor% The current is so strong %actor.name% can't go back!
wait 15
%send% %actor% You struggle to prevent yourself going over the edge!
%echoaround% %actor% %actor.name% struggles to keep themself from toppling over the edge!
wait 5
%send% %actor% Your determination fails and you topple over the edge!
%echoaround% %actor% %actor.name%'s determinations fails and they topple over the edge!
%force% %actor% down
end
~
#30839
Dart trap before lake~
2 g 100
~
if %actor.canbeseen%
if %actor.level% < 111
if !(%actor.affected(17)%)
wait 5
%send% %actor% You activate a trap embedded in the floor!
%echoaround% %actor% %actor.name% activates a trap embedded in the floor!
%send% %actor% Your hit by a dart!
%echoaround% %actor% %actor.name% is hit by a dart!
%damage% %actor% 200
wait 1
%send% %actor% Your hit by a dart!
%echoaround% %actor% %actor.name% is hit by a dart!
%damage% %actor% 200
end
end
end
~
#30840
Killer vampire bat - load vampire on death~
0 f 100
~
%load% mob 30859
~
#30841
two-headed beast - guard vault~
0 c 100
unlock~
if %actor.canbeseen%
shout You dare to unlock the vault before me %actor.name%?
wait 5
shout I will crush you like an insect!
wait 5
mkill %actor.name%
else
say There must be ghosts down here....
wait 5
say No wonder the King created me to guard his vault.
end
~
#30842
two-headed beast - headbutt~
0 bk 100
~
if %actor.canbeseen%
wait 450
eval room %actor.room%
if %room.vnum% == 30970
say Cop this %actor.name%!
%send% %actor% Two-headed beast headbutts you
%echoaround% %actor% Two-headed beast headbutts %actor.name%!
%damage% %actor% 500
wait 5
%send% %actor% Two-headed beast headbutts you with its other head!
%echoaround% %actor% Two-headed beast headbutts %actor.name% with its other head!
%damage% %actor% 500
end
end
~
#30843
Health shot special~
1 j 100
wear~
%send% actor% The health shot clings to you and pricks you softly
cast 'advanced heal' %actor%
cast 'advanced heal' %actor%
cast 'invis' %actor%
~
#30844
Remove and Hint problem fix! Do NOT remove!~
1 c 100
hint on hint off~
if %cmd% == %hint%
halt
~
#30845
tracking device - drop - load soul searcher~
1 h 100
drop~
wait 5
eval room %actor.room%
if (%room.vnum% > 30800) && (%room.vnum% < 30999)
%load% mob 30841
wait 2s
%force% soul whap %actor.name%
%force% soul say You were told not to drop that device %actor.name%!
%send% %actor% You are covered in a strange purple aura!
%echoaround% %actor% %actor.name% is covered in a strange purple aura!
%echoaround% %actor% %actor.name% is transported to before the Head Soul Searcher
%purge% soul
%teleport% %actor.name% 30800
%force% %actor% sleep
%purge% self
else
wait 5
%echo% The Ring of the soul tracker detects %actor.name% is out of the Death Castle and self destructs!
%purge% self
end
~
#30846
psycotic servant axe throw~
0 k 100
~
if %actor.canbeseen%
wait 100
eval room %room.actor%
if %room.actor% ==
throw axe %actor.name%
%load% obj 30850
hold axe
end
end
~
#30847
psycotic servant - chop chop~
0 ghi 100
0~
if %actor.canbeseen%
say chop chop!
end
~
#30848
psycotic servant kick~
0 ghi 100
~
if %actor.canbeseen% && if %actor.vnum% < 0
grin
emote kicks %actor.name% in the family jewels!
%damage% %actor% 200
mkill %actor.name%
end
~
#30849
Head soul searcher - level check for zone/transport in zone~
0 ghi 100
~
if %actor.canbeseen%
if %actor.level% > 69
say Welcome %actor.name%
smile
tell %actor.name% I can transport you inside the Castle so you can go get your ass kicked!
tell %actor.name% say 'transport please' for me to transfer you.
else
shout %actor.name% go get yourself some more experience else where before you commit yourself to die here!
end
end
~
#30850
Head Soul searcher - teleport upon request~
0 d 0
transport please~
if %actor.canbeseen%
%load% obj 30881
give ring %actor.name%
say The conditions of entering this Castle are you must carry that item %actor.name%
wait 5
say Failure to carry that Ring will result in you returning south of here, even if you don't like it!
wait 5
say When you want to leave simply drop the ring, or if you have recalled, drop it and it will know you have left the Death Castle
Emote blinks and %actor.name% is covered in a purple aura
say There you go %actor.name%. I hope being a %actor.class% assists you in staying alive a little longer
wait 2s
emote blinks again and %actor.name% materialises
%teleport% %actor.name% 30896
%force% %actor% look
end
~
#30851
Room trigger - lower drawbridge - speech lower please~
2 d 0
lower please~
if %actor.canbeseen%
if %actor.vnum% < 0
%purge% raised
%load% obj 30862
wait 17s
%purge% lowered
%load% obj 30863
end
end
~
#30852
Drawbridge attendant - no thanks~
0 d 0
no thanks~
wait 2
Emote grumbles
wait 5
say Suit yourself %actor.name%....
wait 5
say Although I bet at %actor.level% and being a %actor.class%, you could gain some vital experience and equipment in that castle.
wait 20
say Then again you probably would not come out alive!
wait 5
smile %actor.name%
~
#30853
Death shadow GOD statue - look~
2 g 100
~
if %actor.canbeseen%
if %actor.level% < 101
wait 1s
%echo% The statue says "%actor.name% you shall suffer at the hand of the highest evil imaginable!"
wait 1s
%echo% The statue creates two fireballs in its eyes!
wait 1s
%send% %actor% The statue sends the fireballs at you, severely burning you!
%echoaround% %actor% The statue sends the fireballs at %actor.name% who is severely burnt!
%damage% %actor% 1000
else
wait 1s
%echo% The statue says "%actor.name% why do you constantly have to harass me?"
end
end
~
#30854
Door attendant - open door~
0 ghi 100
~
if (%direction% == north)
if %actor.vnum% < 0
open throne
say Have a pleasant trip %actor.name%
smile
wait 200
close throne
end
end
~
#30855
Royal servant - speech~
0 ghi 20
~
if %actor.vnum% < 0
say Welcome %actor.name%!
wait 2s
say Are you staying for a bite to eat?
end
~
#30856
Huge serpent - can't pass south~
0 c 100
s~
if %actor.canbeseen%
%send% %actor% You think you can move over a serpent that blocks the tunnel?
%echoaround% %actor% %actor.name% tries to get passed the serpent to no avail...
wait 1s
laugh
else
%echo% You can't pass a snake that blocks the tunnel!
end
~
#30857
King teleport at 10%H - to 30984~
0 l 10
~
eval room %actor.room%
if %room.vnum% == 30957
shout Where the f#*k are all my guards!
emote vanishes!
%teleport% %self% 30984
wait 5
mrestore 5000
rest
else
eval room %actor.room%
if %room.vnum% == 30984
shout I will have my vengance you cowards!
end
end
~
#30858
Queen special~
0 k 40
~
* get players health and store it as variable %dam%
eval dam %actor.hitp% / 5
if %actor.canbeseen%
eval room %actor.room%
if (%room.vnum% > 30800) && (%room.vnum% < 30999)
grin %actor.name%
%send% %actor% Queen Death Shadow calls upon his GOD for assistance.
%echoaround% %actor% Queen Death Shadow calls upon his GOD for assistance.
%damage% %actor% %dam%
%send% %actor% Queen Death Shadow's call is answered and you are burnt by a fireball sent from above!
%echoaround% %actor% Queen Death Shadow's call is answered and %actor.name% is burnt by a fireball sent from above!
wait 10s
end
end
~
#30859
Portal guard load speech~
0 n 100
~
wait 10
say %actor.name% you FOOL! If there was no REAL security for his magesty, we would not be here!
wait 15
say %actor.name% you don't learn do you. We have sworn with our lives to protect his magesty, you are NOT getting passed!
~
#30860
Queen death shadow - speech/attack~
0 n 100
~
wait 2
%load% obj 30875
wear strength
%load% obj 30875
wear strength
%load% obj 30803
wield blade
vis
emote has arrived to assist her beloved King!
wait 10
say Oh another pain in the ass! Let me help you honey!
wait 10
assist king
~
#30861
Serpent Soul - load serpent eye on drop~
1 h 100
~
wait 2
%load% obj 8400
%echo% As the Serpent Soul hits the ground a bright flash of light forms from within it!
%purge% self
~
#30862
Death shadow blade sheath - load Ghost on get~
1 g 100
~
eval room %actor.room%
if (%room.vnum% > 30800) && (%room.vnum% < 30999)
wait 2
%send% %actor% A cold feeling fills the room
%echoaround% %actor% A cold feeling fills the room
wait 1s
%send% %actor% A strange evil feeling overcomes you.
wait 5s
%echoaround% %actor% You sense an evil presense in the room.
%load% mob 30839
%load% obj 30813
%force% god get blade
%force% god wield blade
set load_ghost 1
global load_ghost
%force% ghost say %actor.name% I might be dead but I can still kill you for stealing my precious blade!
%force% ghost kill %actor.name%
else
wait 2
%echo% A cold feeling fills the room
wait 1s
%echo% You sense an evil presense in the room.
%load% mob 30839
set load_ghost 1
global load_ghost
%force% god say %actor.name% you have a stolen artifact in your posession! You are lucky not to be in the Death Castle!
%Purge% god
%purge% blade
wait 5
%echo% The Ghost of the death shadow GOD disappears
set load_ghost 0
end
end
~
#30863
Ghost of Death shadow GOD - load~
0 n 100
~
%load% obj 30813
wield blade
~
#30864
Ghost death shadow - special~
0 k 75
~
if %actor.canbeseen%
%send% %actor% Ghost of the death shadow GOD stares at your eyes and you are burnt from the inside!
%echoaround% %actor% Ghost of the death shadow GOD stares at %actor.name%'s eyes and %actor.name% is burnt from the inside!
%damage% %actor% 1500
%send% %actor% Smoke comes off your body!
%echoaround% %actor% Smoke comes off of %actor.name%'s body!
wait 200
end
~
#30865
test trigger for fly~
2 g 100
~
if %actor.affected(17)% ==
wait 5
%send% %actor% You activate a trap embedded in the floor!
end
~
#30866
Ring of soul searcher - Junk protection.~
1 h 100
junk~
if %actor.canbeseen%
%send% %actor% The ring bounces back into your hand in defiance!
end
return 0
~
#30867
Death Shadow GOD - special #2~
0 k 50
~
dg_Cast 'fireball' %actor.name%
dg_Cast 'fireball' %actor.name%
dg_Cast 'fireball' %actor.name%
wait 45
~
#30868
Soul searcher GOD - load~
0 n 100
~
emote is summoned and nods
~
#30869
Ring of soul searcher is dropped~
0 n 100
~
Emote appears almost blinding you!
wait 1s
Emote dims revealing a strange figure
%load% mob 30840
%purge% self%
~
#30870
Ring of soul searcher - wear ring~
1 j 100
~
if %actor.canbeseen%
wait 2
%send% %actor% You feel as if you are being tracked.
end
return 0
~
#30871
Baby spider - load~
0 n 100
~
Emote appears
wait 5
gasp
say Mummmmmmyyyyyyyyyy!
assist mother
~
#30872
Killer bat - poison~
0 l 50
~
dg_cast 'poison' %actor.name%
wait 300
~
#30873
Waterfall room 30959 - below edge~
2 g 100
~
if %actor.canbeseen%
wait 5
%send% %actor% You are falling down the waterfall and your spells are useless to save you!
%force% %actor% down
end
~
#30874
Waterfall - below waterfall - after 1000 damage~
2 g 100
~
if %actor.canbeseen%
if %actor.vnum% < 0
wait 5
%echo% The current pushes you down deeper
%force% %actor% down
end
end
~
#30875
Waterfall - bottom - hit water~
2 g 100
~
if %actor.canbeseen%
if (%direction% == up)
wait 1
%echo% You hit the water with such force it knocks the wind out of you!
%damage% %actor% 1000
%force% %actor% d
end
end
~
#30876
Evil shadow - zone 1 - load ghost on death~
0 f 100
~
%load% mob 30848
~
#30877
Ghost of evil shadow - load~
0 n 100
~
wait 5
say You honestly think you can kill me that easily??
~
#30878
mobs before statue - attack if command up~
0 c 100
u~
if %actor.canbeseen%
say Over my dead body %actor.name%!
kill %actor.name%
else
say Bloody imm's around here!
end
~
#30879
Evil shadow - zone 2 - load ghost on death~
0 f 100
~
%load% mob 30849
~
#30880
Evil shadow - zone 3 - load ghost on death~
0 f 100
~
%load% mob 30850
~
#30881
Evil shadow - zone 4 - load ghost on death~
0 f 100
~
%load% mob 30851
~
#30882
Evil shadow - zone 5 - load ghost on death~
0 f 100
~
%load% mob 30852
~
#30883
Evil shadow - zone 6 - load ghost on death~
0 f 100
~
%load% mob 30853
~
#30884
Evil shadow - zone 7 - load ghost on death~
0 f 100
~
%load% mob 30854
~
#30885
Evil shadow - zone 8 - load ghost on death~
0 f 100
~
%load% mob 30855
~
#30886
Evil shadow - zone 9 - load ghost on death~
0 f 100
~
%load% mob 30856
~
#30887
Door attendant - close door - come from south~
0 ghi 100
~
if (%direction% == south)
if %actor.vnum% < 0
wait 10
close throne
say The King is not in a good mood %actor.name%
smile
end
end
~
#30888
Object dropped above lava - purge it~
2 h 100
~
wait 1
%echo% %object.shortdesc% falls down into the lava and is vaporised!
%purge% %object%
~
#30889
item dropped at portal guard - falls~
2 h 100
~
wait 1
%echo% %object.shortdesc% somehow hovers here
~
#30890
mob cast fireball - 60% of fight~
0 k 60
~
dg_cast 'fireball' %actor%
~
#30891
Vampire bat load~
0 n 100
~
wait 1
%emote% appears in its true form.
~
$~
