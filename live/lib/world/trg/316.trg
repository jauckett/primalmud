#31600
shatter glass - 31621 - get in~
2 c 100
shatter~
wait 3
%echo% You obliterate the glass with your mighty punch!
wait 3
%echo% Glass goes flying outward with the sudden impact
wait 3
%teleport% all 31621
end
~
#31601
Kryten greet to Anne, Jane, & tracey~
0 gh 100
~
if %actor.vnum% == 31606
wait 2
gasp 
say Miss Jane!
wait 2
say You haven't brushed your hair!
wait 2
%emote% grabs a brush and starts brushing Miss Jane's hair!
wait 2
say Thats better, smart but casual!
elseif %actor.vnum% == 31608
wait 2
gasp
say Miss Anne!
wait 2
say Why you haven't touched your soup!
wait 2
say No wonder your beginning to look so pasty!
elseif %actor.vnum% == 31607
wait 2
sigh
say Miss Tracey!
wait 2
say No! You look absolutely PerFECT!
else wait 2
say Come In! Come In!
wait 2
say How lovely to meet you!
bow
end
~
#31602
kryten speech~
0 dg 100
they are all dead they are no yes come home with me~
if (%speech% == they are all dead)
wait 2
say Who's dead?
elseif (%speech% == they are)
wait 2
say Oh my GOD! I was only away two minutes!
wait 2
say How do you know? Are you a doctor?
elseif (%speech% == no)
wait 2
say Your that sure they're dead?
elseif (%speech% == yes)
wait 2
say What am i going to do?
elseif (%speech% == come home with me)
wait 2
say I can't leave them! But you can!
end
~
#31603
return journey on blue midget~
2 g 100
~
wait 2
wait 20s0s
%echo% The Blue Midget takes off!
wait 20s
%echo% The Blue Midget lands!
%echo% The ramp on the Blue Midget lowers!
%echo% You are dumped uncerimonously on your arse in the middle of the loading dock!
%echo% The Blue Midget takes off again!
%teleport% all 31504
end
~
#31604
dog greet~
0 g 100
~
wait 5
say Well what do we have here?
peer
wait 5
say What a funny looking dog!
wait 5
say Come on now! I wanna be your buddy!
wait 5
say I tell you what I am going to smell you BEhind!
wait 5
say Then you can smell mine!
wait 5
say Now is that a deal?
end
~
#31605
chrissys bunkie - greet~
0 g 100
~
wait 2
say Hi!
smile
wait 2
say Are you looking for Christine?
~
#31606
Chrissys bunkie speech~
0 d 100
yes where take me there~
if (%speech% == yes)
say She is still on planet leave!
elseif (%speech% == Where)
say At the Ganymede Holiday Inn!
elseif (%speech% == Take me there)
say Okay here you go!
%teleport% %actor% 31636
end
~
#31607
listers room from the past~
2 g 100
~
wait 20s
%echo% Lister and Chrissy walk into the room!
%load% mob 31618
%load% mob 31619
wait 10s
%echo% Rimmer pops up from the sink!
%load% mob 31620
end
~
#31608
future lister speech on load~
0 n 100
~
say I don't want anyone to get in a flap here!
wait 5
say but I am the Rimmer who comes from the double double future!
wait 5
say I'm the Rimmer who is with the Lister that married Kochanski!
wait 5
say Now from this point on things get a little confusing!
smile
end
~
#31609
load of marilyn~
2 g 100
~
%purge%
wait 5s
%load% mob 31623
%echo% Marilyn Munroe walks towards you!
end
~
#31610
host of game~
0 g 100
~
wait 2
say Welcome to Better than Life!
wait 5
say You must be hungry! We have a restaurant several miles down the beach!
wait 5
say Your transportation is awaiting you!
wait 10
say but do choose carefully!
end
~
#31611
listers speech~
0 g 100
~
if %actor.vnum% == 31630
wait 2s
glare
elseif %actor.vnum% == 31629
wait 2s
say RIMmer your such a SMEGhead!
elseif
wait 11s
say Have you checked into your room?
wait 2s
say Mine is absolutely BRILLIANT!
wait 2s
say I've got this vibrating leopard skin waterbed!
wait 1s
say In the shape of a guitar!
end
~
#31612
cats speech~
0 g 100
~
if %actor.vnum% == 31630
wait 2s
say Hey Bud!
elseif %actor.vnum% == 31629
point rimmer
laugh
elseif
wait 16s
say YEAH!
wait 2s
say Well you should take a look at my Wardrobe!
wait 2s
say It's so Big it crosses an International time zone!
wait 2s
say When it is 3 o'clock where my shirts are it's 7 in the morning for my socks!
end
~
#31613
arnold speech~
0 gh 100
~
if %actor.vnum% == 31630
wait 2s
say I think there must be some mistake! I am not an Admiral!
%load% mob 31629
%purge% self
elseif
wait 2s
say I'm sorry!
wait 1s
say I don't know what happened!
wait 1s
say I was driving along and suddenly there was McGrudger!
wait 2s
say Well,... One thing lead to another and.....
wait 2s
say GOOD GOD! This is a GREAT game!
wait 2s
say Twice in ONE lifetime!!! I'm turning into Hugh Hefner!
wait 12s
say Who is that?
peer
wait 2s
say Just because some hoity-toity gonad-brain git knows an admiral!
say Does he have to broadcast it?
peer
wait 2s
yawn
say Yawn-a-rama city. We KNOW an Admiral!
~
#31614
Loading of Junior on entry~
2 g 100
~
wait 20s
%load% mob 31630
~
#31615
juniors shout!~
0 n 100
~
wait 2s
shout Admiral!
wait 2s
shout Admiral!
wait 2s
shout Admiral Rimmer Sir!
wait 5s
w
%door% 31644 east room 31646
salute rimmer
say Admiral RIMmer SIR
Admiral RIMmer SIR!
wait 2s
say Field Marshal Clifton sends his compliments and...
wait 1
say wonders if you care to join him for port and cigars?
wait 3s
e
~
#31616
resetting a door~
0 f 100
~
%door% 31644 east room 31645
~
#31617
admiral rimmers speech~
0 n 100
~
wait 2s
say I LOVE this game!
wait 2s
say Excuse me Gentlemen!
wait 1s
stand
e
~
#31618
rimmers dad~
0 g 100
~
if %actor.vnum% == 31629
say Arnold!
wait 2s
say Arnold I just wanted to say...
wait 2s
say I just wanted to say....
wait 2s
say Your a complete and utter SMEG-Head!
end
~
#31619
opening of locker~
1 c 100
open~
wait 1s
%echo% The locker door flies open! 
%load% mob 31634
%echo% Out jumps the revenue man!
wait 1s
%echo% The Locker falls to pieces!
%purge% self
~
$~
