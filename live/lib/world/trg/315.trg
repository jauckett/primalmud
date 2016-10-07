#31501
Hit Red Button - 31505 - get in~
2 c 100
push~
if (%cmd% == push) && (%arg% == red)
wait 3
%echo% The Air Lock doors woosh open!
wait 6
%echo% The Air Lock doors swoosh close!
wait 2
%echo% The rushing noise of Air fills the chamber!
%teleport% all 31506
end
~
#31502
Push Green Button - 31506 - get in~
2 c 100
push~
if (%cmd% == push) && (%arg% == green)
wait 2
%echo% The inner doors sweep open infront of you!
%teleport% all 31507
elseif (%cmd% == push) && (%arg% == red)
wait 2
%echo% The Inner doors sweep shut behind you!
wait 6
%echo% The air is pumped out of the room, creating a vacuum
wait 2
%echo% The Air lock doors swoosh open!
%teleport% all 31505
end
~
#31503
Hit red button - 31506 - get out~
2 c 100
push~
if (%cmd% == push) && (%arg% == red)
wait 2
%echo% The Inner doors sweep shut behind you!
wait 6
%echo% The air is pumped out of the room, creating a vacuum
wait 2
%echo% The Air lock doors swoosh open!
%teleport% all 31505
end
~
#31504
Push Green Button - 31507 - get out~
2 c 100
push~
if (%cmd% == push) && (%arg% == green)
wait 2
%echo% The inner doors sweep open!
%teleport% all 31506
end
end
~
#31505
Elevator instructions~
2 g 100
~
wait 5
%echo% Welcome to Xpress Lifts!
wait 2
%echo% What Floor please?
~
#31506
floor jumps on elevator~
2 d 100
2585 2200 2580 2575 2590 16~
if (%speech% == Floor 2585)
wait 2
%echo% Floor 2585 will be reached in approximately 4 seconds
wait 4s
%echo% Floor 2585 - Thank you for choosing Xpress Lifts!
wait 2
%echo% The elevator door slides open!
%echo% We hope you enjoyed your ride!
%teleport% all 31510
elseif (%speech% == Floor 2200)
wait 2
%echo% Floor 2200 will be reached in approximately 5 seconds
wait 5s
%echo% Floor 2200 - Thank you for choosing Xpress Lifts!
wait 2
%echo% The elevator door slides open!
%echo% We hope you enjoyed your ride!
%teleport% all 31508
elseif (%speech% == Floor 2580)
wait 2
%echo% Floor 2580 will be reached in approximately 6 seconds.
wait 6s
%echo% Floor 2580 - Thank you for choosing Xpress Lifts!
wait 2
%echo% The elevator door slides open!
wait 2
%echo% We hope you enjoyed your ride!
%teleport% all 31533
elseif (%speech% == Floor 2575)
wait 2
%echo% Floor 2575 will be reached in approximately 7 seconds.
wait 7s
%echo% Floor 2575 - Thank you for choosing Xpress Lifts!
wait 2
%echo% The elevator door slides open!
wait 2
%echo% We hope you enjoyed your ride!
%teleport% all 31553
elseif (%speech% == Floor 2590)
wait 2
%echo% Floor 2590 will be reached in approximately 8 seconds.
wait 8s
%echo% Floor 2590 - Thank you for choosing Xpress Lifts!
wait 2
%echo% The elevator door slides open!
wait 2
%echo% We hope you enjoyed your ride!
%teleport% all 31576
elseif (%speech% == Floor 16)
wait 2
%echo% Welcome to Xpress Lifts decent to Floor 16!
wait 5
%echo% We will be going down 2567 Floors!
wait 5
%echo% and for a small extra charge you can enjoy the inlift movie "Gone with the Wind"
wait 3
wait 6
%echo% If you look to your right and your left you will notice there are no Exits!
wait 8
wait 4
%echo% In the highly unlikely event the lift having to make a crash landing...
wait 2
%echo% DEATH is certain!
wait 7s
%echo% Thank you for travelling Xpress Lifts!
wait 2
%echo% The elevator door slides open!
wait 2
%echo% We apologize for the delay!
%teleport% all 31582
~
#31507
Mice greet~
0 gh 100
~
wait 2
salute %actor.name%
wait 3
say Yes Mr Lister Sir!
wait 3
say EEEeeeeck! Eeeeeeck!
~
#31508
Talkie toaster greet~
0 gh 100
~
wait 2
say Hey %actor.name% do you want some toast?
wait 5
say I Toast! Therefore I AM!
end
~
#31509
George McIntyre speech~
0 gh 100
~
wait 5
say This must seem prrretty spookie for everyone...
wait 5
say but, I don't want you to think of me as someone whos dead,
wait 5
say more as someone whos no longer a threat to your marriages!
chuckle
wait 5
say As you know Holly is only capable of sustaining one Hologram..
wait 5
say So my advice to anyone who i smore vital to the mission than me is...
wait 5
say If you DIE, I kill you!
end
~
#31510
Paranoi speech~
0 gh 100
~
wait 2
glare
say Is that a urine stain on the front of your trousers????
~
#31511
Rimmers garbage pod holler~
0 gi 100
~
wait 4
shout It's a GARBAGE POD!
wait 10
shout It's a SMEGGING GARBAGE POD!
end
~
#31512
food dispenser 172~
2 c 100
push~
wait 2
%echo% The Food Dispensing machiine says "Yesth, Can I help you?"
~
#31513
Food command~
2 d 100
food bacon sandwich black coffee you have a lisp your vocabulary unit is broken too chicken soup~
if (%speech% == food)
wait 2
%echo% The Food Dispensing Machines says "Yesth, Can I help you?"
elseif (%speech% == bacon sandwich)
wait 2
%echo% The Food Dispensing Machines lid slides back!
wait 2
%echo% A ramp shoots forward and a pair of gumboots fall to the floor!
%load% obj 31533
elseif (%speech% == black coffee)
wait 2
%echo% The Food Dispensing Machine's lid slides back!
wait 2
%echo% A ramp shoots forward and a silver pale falls to the floor!
%load% obj 31534
elseif (%speech% == You have a lisp?)
wait 2
%echo% The Food Dispensing Machine says "Yesth I have!"
wait 2
%echo% The Food Dispensing Machine says "Thisth hasth been reported to the skuttersth!"
elseif (%speech% == Your vocabulary unit is broken too?)
wait 2
%echo% The Food Dispensing Machine says "Yesth, thisth too hasth been reported!"
elseif (%speech% == chicken soup)
wait 2
%echo% The Food Dispensing Machine's lid slides back!
wait 2
%echo% A ramp shoots forward and some food falls to the floor!
%load% obj 31535
end
~
#31514
Food machine in drive room~
2 c 100
push~
wait 2
%echo% The Food Dispensing Machine says "Good Morning!"
wait 2
%echo% The Food Dispensing Machine says "What would you like?"
end
~
#31515
food from machine in drive room~
2 d 100
food chicken vindaloo milkshake beer~
if (%speech% == food)
wait 2
%echo% The Food Dispensing Machine says "Todays Fish is Trout Ala Creme"
wait 2
%echo% The Food Dispensing Machine's lid slides back!
wait 2
%echo% A ramp slides out and the food falls to the ground!
%load% obj 31536
%load% obj 31537
wait 2
%echo% The Food Dispensing Machine says "Enjoy your meal!"
elseif (%speech% == milkshake)
wait 2
%echo% The Food Dispensing Machine says "What Flavour?"
elseif (%speech% == beer)
wait 2
%echo% The Food Dispensing Machine's lid slides back!
wait 2
%echo% A ramp slides out and the milkshake falls to the ground!
%load% obj 31538
elseif (%speech% == chicken vindaloo)
wait 2
%echo% The Food Dispensing Machine's lid slides back!
wait 2
%echo% A ramp slides out and the food falls to the ground!
%load% obj 31536
%load% obj 31539
end
~
#31516
Cat greet!~
0 gh 100
~
wait 7
shout I'm going to eat you little fishy!
wait 8
shout I'm going to eat you little fishy!
wait 9
shout I'm going to eat you little fishy!
~
#31517
pod journey to red dwarf~
2 g 100
~
wait 6s
%echo% The Pod's door closes behind you and seals itself!
wait 6s
%echo% You feel the pod being raised off the ground!
wait 6s
%echo% The pod shunts backwards violently!
wait 6
%echo% Then is flung forwards as it is launched from the space port!
wait 12s
%echo% The forward motion starts to dwindle....
wait 12
%echo% The retro's blast and the pod starts to hurtle through space!
wait 12
%echo% Time to sit back and relax!
wait 12s
%echo% The pod violently changes course throwing the mail everywhere!
wait 12
%echo% There is a face travelling at great speed towards you!
wait 12
%echo% The Face says "This is an S.O.S. distress call from the mining ship Red Dwarf"
wait 12
%echo% The Face Says "The crew are dead, killed by a radiation leak!"
wait 12
%echo% The Face says "The only survivors were Dave Lister, who was in 
%echo% suspended animation during the disaster and his pregnant cat,"
wait 6
%echo% The Face says "Who was safely sealed in the hold."
wait 12
%echo% The Face says "Revived 3 million years later,"
wait 12
%echo% The Face says "Listers only companions are a life form who evolved from his cat"
wait 12
%echo% The Face says "and Arnold Rimmer a hologram simulation of one of the dead crew!"
wait 12
%echo% The Face says "Additional...."
wait 12
%echo% The Face says "I am Holly the ships computer with an IQ of 6000"
wait 12
%echo% Holly says "The same IQ as 6000 P.E. Teachers!"
wait 12
%echo% The comes to a very bumpy landing!
wait 12
%echo% The pod door opens!
%teleport% all 31500
~
#31518
testing zoneecho~
2 g 100
~
wait 2
%zoneecho% 31516 It is safe to come out now Dave!
wait 2
%zoneecho% 31516 3 Million Years Dave!
end
~
$~
