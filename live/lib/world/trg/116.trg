#11601
Whore greet~
0 gh 100
~
if %actor.sex% == male
wait 2
emote spreads her legs wide open as she notices you walk in the room!
wait 5
say %actor.name% Come over here and have me!
say %actor.name% You know you want it
wait 30
emote starts to use her large Black Vibrator
say do you want me to beg ?
fondle %actor.name%
wait 30
point %actor.name%
laugh
say %actor.name% i have seen bigger weaniers on mice!
else
emote spreads her legs wide open as she notices you walk in the room!
say Hay thier cutie pie!!
say want to come over hear and have a munch
wait 20
%load% obj 11603
say how selfish of me to have all the fun
give Vibrator %actor.name%
wait 20
%give% black %actor.name%
say We can both have fun
wait 10
wink
end
~
#11602
Crowed Surfer~
0 gh 100
~
if %actor.level% < 100
say Hi %actor.name% come on up it is cool fun
say it is easy just "go surf"
wait 50
say It's they only to get to the stage, just don't let the bouncers get ya
end
wait 50
eval room %actor.room%
if %actor.vnum% == 11610
say Come on dude what are you weighting for?
elseif (%actor.vnum% == 11612)
say Come on dude what are you weighting for?
elseif (%room.vnum% == 11661)
%teleport% %actor% 11667
set sent out
elseif (%room.vnum% == 11662)
%teleport% %actor% 11666
set sent stage
elseif (%room.vnum% == 11663)
%teleport% %actor% 11667
set sent out
elseif (%room.vnum% == 11664)
%teleport% %actor% 11666
set sent stage
elseif (%room.vnum% == 11665)
%teleport% %actor% 11667
set sent out
else
say hmm none to send
end
wait 6
if (%sent% == stage)
%echo% A guy has lifted %actor.name% into the air.
%echo% %actor.name% surfs around on the crowd for a while.
%echo% %actor.name% surfs has been lifed over the fence.
say Have Fun dude !!!
%send% %actor% You have been lifted into the air.
%send% %actor% You surf around on the crowd for a while.
%send% %actor% You have been lifed over the fence.
elseif (%sent% == out)
%echo% A guy has lifted %actor.name% into the air.
%echo% %actor.name% surfs around on the crowd for a while.
%echo% %actor.name% surfs has been lifed over the fence.
say Have Fun dude !!!
%send% %actor% You have been lifted into the air.
%send% %actor% You surf around on the crowd for a while.
%send% %actor% You have been lifed over the fence.
%send% %actor% A bouncer has taken hold of you and dragged you of.
%send% %actor% He pushes you out a door to the left of the stage which is shut behind you.
else
say Have Fun dude !!!
end
~
#11603
queing female~
0 ghi 100
~
if %actor.sex% == female
say Hay don't push in
else
say I wish i was male
end
~
#11604
Scalper ticket~
0 gh 100
~
if %actor.vnum% == 11601
emote leans over and whispers something to the Scalper.
%force% scalper emote leans over and whispers something to the young male.
nod
%force% scalper emote give's The Young Male something.
end
~
#11605
Tickets please~
0 gh 100
~
say Tickets please!!
~
#11606
Bribe guard~
0 m 1
~
   if (%canbeseen%) & (%actor.level% < 90)
say go away fool
wait 1
elseif (%canbeseen%) & (%amount% < 50000)
say Did you really think I was that cheap, %actor.name%.
snarl 
elseif (%canbeseen%) & (%actor.level% > 90)
context %actor.id%;   set has_bribed_guard 1
wait 10
emote opens the Steel
wait 5
emote helps %actor.name% through the door
%send% %actor% You were helped through the door by the Guard
%teleport% %actor% 11610
wait 5
%send% %actor% The door is closed behind you
emote closes the Steel
wait 6
whistle
else
whistle
end
~
#11607
Vacant~
2 g 100
~
wait 5
%echo% &r************************************&n
%echo% &r*        &YConcert Goers Food        &r*&n
%echo% &r*         &yPositions Vacant         &r*&n
%echo% &r*                                  &r*&n
%echo% &r*             &yTo apply             &r*&n
%echo% &r*       &ySay "I want to Work"       &r*&n
%echo% &r*                                  &r*&n
%echo% &r************************************&n
~
#11608
real fan~
0 gh 100
~
say hi %actor.name%, Only real fans can enter.
wait 20
eval tome %actor.eq(body)%
if %tome.vnum% == 11606
wait 5 
say %actor.name%, Congratulations you are a real fan.
wait 10
%teleport% %actor% 11624
wait 3
%echo% The Bouncer helps %actor.name% through the door into the hall
%send% %actor% You are hurried through the door into the hall.
else 
say %actor.name%, you are not a real fan
say Don't bother coming back till you can prove yourself worthy.
end
~
#11609
Ticket collector~
0 j 100
~
if (%actor.level% < 70)
say You are too young, 
return 0
elseif (%object.vnum% == 11601)
wait 5
say thanks
wait 10
emote opens the Steel.
wait 5 
emote helps %actor.name% through the door
%send% %actor% You were helped through the door by the Ticket Collector.
%teleport% %actor% 11610
wait 5
%send% %actor% The door is closed behind you.
emote closes the Steel
wait 5
junk ticket
else 
say I don't want that!
return 0
end
~
#11610
job Offer~
2 d 0
I want to Work~
%load% mob 11625
~
#11611
HR Manager~
0 n 100
~
wait 10
emote has arrived.
say so I here you want a job 
say well this is your lucky day
say one of our biggest clients has just asked us to fill a role for them.
wait 5
say I think you should suit this job well and it even comes with a uniform
say Well the first thing you need to do is put on the uniform
say then say "I  am ready"
%load% obj 11650
%load% obj 11651
drop all
~
#11612
I am ready~
2 d 0
I am ready~
eval weq %actor.eq(waist)%
eval heq %actor.eq(head)%
if (%weq.vnum% == 11650) && (%heq.vnum% == 11651)
%load% obj 11652
%load% obj 11653
%load% obj 11654
%load% mob 11626
%purge% HR
wait 5
%echo% The HR Manager waves his arms around and the room fills with smoke.
%echo% when the smoke clears you find you are standing in Concert Goers Food.
else 
wait 5
%echo% The HR Manager looks at you very sternly 
%echo% I see we arn't that smart !!!
%echo% Following instructions is the first step to getting a job !!!
end
~
#11613
Burger~
0 d 0
Cheese Please~
%load% obj 11655
wait 5
put burger warmer
wait 5
say One Up
~
#11614
Chips~
0 d 0
Chips Please~
%load% obj 11656
wait 5
put chips machine
wait 5
say chips up
~
#11615
Instructions~
0 n 100
~
wait 10
say i am the Burger Flipper, i will get the Food ready for you
say please read the instructions on the board.
%load% obj 11662
wait 5
drop board
~
#11616
Drink~
2 d 0
Coke Please~
%load% obj 11657
%echo% a empty cup is weighting to be filled
~
#11617
Menu~
2 d 0
Show Menu~
wait 5
%echo% &r****************************************
%echo% &r*        &YConcert Goers food  &r          *
%echo% &r*          &Y     MENU           &r        *
%echo% &r*                                      *
%echo% &r* &MCheese Burger &c---  &M"Cheese  Please"  &r*
%echo% &r* &MVeggie Burger &c---  &M"Veggie Please"   &r*
%echo% &r* &MBeef Nuggets  &c---  &M"Beef  Please"    &r*
%echo% &r* &MChips         &c---  &M"Chips  Please"   &r*
%echo% &r* &MCoke          &c---  &M"Coke  Please"    &r*
%echo% &r*                                      *
%echo% &r****************************************
~
#11618
Customers please~
2 d 0
Customers Please~
wait 10
eval tome %actor.eq(waist)%
if %tome.vnum% == 11650
%load% obj 11661
%echo% You are ready to work"
else
%echo% I see you arn't really ready
~
#11619
purge self~
0 n 100
~
wait 15
say i wonder how long i am going to have to wait today
wait 600
say This place is crap
say I am out of here.
%purge% self
~
#11620
random shoppers~
1 b 100
~
eval customer %random.10%
switch %customer%
case 1
%load% mob 11630
wait 250
break
case 2
%load% mob 11628
wait 250
break
case 3
%load% mob 11629
wait 250
break
case 4
%load% mob 11627
wait 250
break
case 5
%load% mob 11630
wait 250
break
case 6
%load% mob 11628
wait 250
break
case 7
%load% mob 11629
wait 250
break
case 8
%load% mob 11630
wait 250
break
case 9
%load% mob 11628
wait 250
break
default
%load% mob 11627
wait 250
break
done
~
#11621
Hungry Harry~
0 j 100
~
if (%object.vnum% == 11655)
set obj1 1
global obj1
elseif (%object.vnum% == 11656)
set obj2 1
global obj2
elseif (%object.vnum% == 11657)
set obj3 1
global obj3
else
return 0
say Nice try. Wrong item
end
if (%obj1% == 1) && (%obj2% == 1) && (%obj3% == 1)
mload obj 11660
wait 3
put token Register
say thanks
wait 5
%purge% chips
%purge% cheese
%purge% coke
%purge% self
unset obj1
unset obj2
unset obj3
end
~
#11622
Hungry Harry ~
0 n 100
~
wait 5
say I would like 
say A Cheese burger, Chip's && A Coke.
say thanks
wait 300
say This place is crap
say I am out of here
%purge% self
~
#11623
veggie~
0 d 0
Veggie Please~
%load% obj 11658
wait 5
put veggie warmer
wait 5
say Veggie Up
~
#11624
Beef Nuggets~
0 d 0
Beef Please~
%load% obj 11659
wait 5
put beef warmer
wait 5
say Beef Up
~
#11625
Vegetarian Veronica~
0 j 100
~
if (%object.vnum% == 11658)
set obj1 1
global obj1
elseif (%object.vnum% == 11656)
set obj2 1
global obj2
elseif (%object.vnum% == 11657)
set obj3 1
global obj3
else
return 0
say Nice try. Wrong item
end
if (%obj1% == 1) && (%obj2% == 1) && (%obj3% == 1)
mload obj 11660
wait 3
put token Register
say thanks
%purge% chips
%purge% veggie
%purge% coke
%purge% self
unset obj1
unset obj2
unset obj3
end
~
#11626
Nuggy Nipper~
0 j 100
~
if (%object.vnum% == 11659)
set obj1 1
global obj1
elseif (%object.vnum% == 11656)
set obj2 1
global obj2
elseif (%object.vnum% == 11657)
set obj3 1
global obj3
else
return 0
say Nice try. Wrong item
end
if (%obj1% == 1) && (%obj2% == 1) && (%obj3% == 1)
mload obj 11660
wait 3
put token Register
say thanks
%purge% chips
%purge% beef
%purge% coke
%purge% self
unset obj1
unset obj2
unset obj3
end
~
#11627
Vegetarian Veronica~
0 n 100
~
wait 5
say I would like 
say A Veggie Burger, Chips && A Coke.
say thanks
wait 300
say This place is crap
say I am out of here
%purge% self
~
#11628
Nuggy Nipper ~
0 n 100
~
wait 5
say I would like 
say Beef Nug's, Chip's && A Coke.
say thanks
wait 300
say This place is crap
say I am out of here
%purge% self
~
#11629
Thirsty Tim~
0 n 100
~
wait 5
say I would like A Coke.
say thanks
wait 300
say This place is crap
say I am out of here
%purge% self
~
#11630
Thirsty Tim~
0 j 100
~
if (%object.vnum% == 11657)
set obj1 1
global obj1
else
return 0
say Nice try. Wrong item
end
if (%obj1% == 1)
mload obj 11660
wait 3
put token Register
say thanks
%purge% coke
%purge% self
unset obj1
end
~
#11631
Random - Robber && Manager~
1 b 100
~
eval manager %random.20%
switch %manager%
case 5
%load% mob 11632
wait 250
break
case 10
%load% mob 11631
wait 250
break
case 15
%load% mob 11632
wait 250
break
default
wait 250
break
done
~
#11632
robber stealing~
0 n 100
~
%echo% An armed man has just walked into the store.
wait 5
say This is a hold up, everyone stand still and no one will get harmed.
wait 5
emote walks over to the cash register
emote forces the cash register open
emote empty's all the cash out
%purge% register
%load% obj 11661
wait 30
say now don't anyone do anything stuiped and no one will get hurt.
wait 10
emote moves quickly out the door and disappears into the street.
%purge% self
~
#11633
Manager recevies items~
0 j 100
~
if (%object.vnum% == 11660)
eval obj1 %obj1% + 1
set obj1 %obj1%
global obj1
elseif (%object.vnum% == 11664)
eval obj1 %obj1% + 50
set obj1 %obj1%
global obj1
else
return 0
say Nice try. Wrong item
end
~
#11634
count tokens~
0 d 0
count please~
wait 5
say Hmm lets see
if (%obj1% < 49)
give all.token %actor.name%
give all.hair %actor.name%
say Only %obj1% tokens looks like you haven't worked hard enough.
say I will be back, I hope you will have done better by then.
give all.hair %actor.name%
give all.token %actor.name%
elseif (%obj1% > 49) && (%obj1% < 99)
eval obj2 (%obj1% - 50) *200
set obj2 %obj2%
say %obj1% Tokens you have done very well %actor.name%
say for you hard work i will give you a Hair Tie && %obj2% gold coins.
%load% obj 11664
wait 5
give Hair %actor.name%
elseif (%obj1% > 99) && (%obj1% < 249)
eval obj2 (%obj1% - 100) * 200
set obj2 %obj2%
say %obj1% Tokens you have done very well %actor.name%
say for you hard work i will give you a Glow Stick && %obj2% gold coins.
%load% obj 11663
wait 5
give glow %actor.name%
elseif (%obj1% > 249) && (%obj1% < 499)
eval obj2 (%obj1% - 250) * 200
set obj2 %obj2%
say %obj1% Tokens you have done extremely well %actor.name%
say for this i will reward you with a Backstage Pass && %obj2% gold coins.
%load% obj 11665
wait 5
give Backstage %actor.name%
elseif (%obj1% > 499) && (%obj1% < 999)
eval (obj2 %obj1% - 500) * 200
set obj2 %obj2%
say %actor.name% what a extraordinarily effort %obj1% Tokens.
say A Tattoo && %obj2% gold coins shold be an ample reward.
%load% obj 11666
wait 5
give tattoo %actor.name%
elseif (%obj1% > 999)
eval obj2 (%obj1% - 1000) * 200
set obj2 %obj2%
say %actor.name% you have earned %obj1% Tokens which is an outstanding effort.
say You will get the superb prize of a Ring of Fire && %obj2% gold coins.
say you will need to speak to a GOD before you can use this EQ
%load% obj 11667
wait 5
give fear %actor.name%
else
give all.token %actor.name%
give all.hair %actor.name%
say hmm count was %obj1%
end
give %obj2% coins %actor.name%
%load% obj 11668
wait 5
put all brief
wait 5
%purge% brief
unset obj1
unset obj2
bow
say i will see you next time
emote leaves the building via the front door
%purge% self
~
#11635
Manager money~
0 n 100
~
%echo% In walks the manager, in his black suite and small beif case
say i am here to collect the takens for the week.
say place hand them to me and i will pay you for them
say all you need to do is once you give me all the tokens say "count  please"
wait 300
say Well it doesn't look like you get anything for me
say I will be back latter
%purge% self
unset obj2
unset obj3
unset obj1
~
#11636
help~
2 d 0
help please~
wait 5
%echo% &rThe following is a list of aliases that meight help in running the store
%eacho% 
%echo% &Galias veggie           say veggie please; say chips please; say coke please
%echo% &Galias veggie2          get veggie warmer; get chips machine; get coke; fill coke drink; give veggie Veronica; give chips Veronica;give coke Veronica
%echo% &Galias burger           say cheese please; say chips please; say coke please
%echo% &Galias burger2          get cheese warmer; get chips machine; get coke; fill coke drink; give cheese harry;give chips harry;give coke harry
%echo% &Galias beef             say beef please; say chips please; say coke please
%echo% &Galias beef2            get beef warmer; get chips machine; get coke; fill coke drink ;give beef Nipper ;give chips Nipper ;give coke Nipper
%echo% &Galias coke             say coke please; get coke; fill coke drink; give coke tim
~
#11637
cash only~
0 d 0
cash please~
eval obj2 %obj1% * 200
set obj2 %obj2%
say %obj1% Tokens you have done extremely well %actor.name%
say for this i will reward you with %obj2% gold coins.
wait 5
give %obj2% coins %actor.name%
%load% obj 11668
wait 5
put all brief
wait 5
%purge% brief
unset obj1
unset obj2
bow
say i will see you next time
emote leaves the building via the front door
%purge% self
~
#11638
Hold~
0 d 0
hold please~
%purge% register
~
#11639
new trigger~
1 fn 100
~
wait 5
eval room %carried_by.name%
%echo% hi &&y the ththththththt  &&n rewewrewrew
if (%room% == 1102)
%echo% you are in limbo
else
%echo% bang
end
~
#11640
restore mob once~
0 l 50
~
if (%actor.level% > 84) && (%have_restored% != 1)
restore %self.name%
shout i have been restored
set have_restored 1
global have_restored
else
shout %actor.name% is going to kill me
end
~
#11690
ne~
2 c 100
northeast~
   if (%actor.vnum% >= 0) 
return 0
halt
end
if (%cmd% == northeast)
%teleport% %actor% 1115
end
~
$~
