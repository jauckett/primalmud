#26101
Juliet - Where is Romeo.~
0 gik 15
~
shout O Romeo, Romeo! Wherefore art thou Romeo?
~
#26102
Romeo - To Juliet~
0 gi 15
~
shout But soft!  What light through yonder window breaks?
shout It is the East, and Juliet is the sun!
~
#26103
Bartender Welcome~
0 ghi 100
~
say Hey there Sweetie.  Have a look around, make yourself at home.
~
#26104
Director - Action~
0 gi 45
~
shout &yLights... Camera... Action... !!!
~
#26105
Soprano Guard Entrance~
0 ghi 100
~
wait 10
if %actor.canbeseen%
wait 5
say Welcome %actor.name%.
if %actor.level% < 91
wait 15
say Hold it %actor.name%!!!
wait 10
say You're only level %actor.level%...!
wait 10
say You have to be at least level 91 to pass me!!
wait 10
say Off with you %actor.name%!
else
say You're level %actor.level%, please advance through the doors.
wait 10
open Auditorium
wait 5 
say Please proceed %actor.name%.
wait 75
close Auditorium
wait 10
say They think they're Superior!
wait 10
end
end
~
#26106
Jack Nicholson~
0 ghi 100
~
wait 10
if %actor.canbeseen% 
say What... %actor.name%! You want the Truth!!!
wait 30
shout &yYou can't Handle the Truth!!!&n
wait 5
mkill %actor.name%
end
~
#26107
Bartender Whinge~
0 b 10
~
say Well, I wanna be... like an actress and stuff...  like famous too
~
#26108
Jerry - Chat~
0 gh 50
~
if %actor.canbeseen%
wait 30
say Hey, let me ask you something %actor.name%.
wait 45
say Have you ever noticed that when you purchase a jacket, there's always an extra button?!?
wait 40
say Or when you travel by jet, everything is so small.. the drink cans, the peanuts ?!?
wait 45
say I can't figure it out %actor.name%, I really cant?!?
wait 20
shrug
end
~
#26109
Ozzy - Comment 1~
0 bg 20
~
if %actor.canbeseen%
wait 30
say hey man
wait 30
say I love you %actor.name%...
wait 30
say I love you more than life itself...
wait 30
say But you're all fucking mad!
end
~
#26110
Ozzy - Comment 2~
0 bgi 60
~
if %actor.canbeseen%
wait 30
emote mumbles, 'where da fuk am I'
wait 60
emote &yshouts, 'Rock and Roll!'&n.
wait 10
end
~
#26111
Ozzy hollers Sharon~
0 k 99
~
wait 300
emote hollers, 'SHARON!!!!'
wait 10
%load% mob 26110
wait 20
%force% Sharon say Leave my husband alone you fucking little shit!!!
%force% Sharon kill %actor.name%
wait 20
%force% Sharon say Take that you little soft bastard!
wait 300
%echo% Jack Osbourne has entered the room.
%load% mob 26111
%force% Jack say Leave my Mom alone you prick!
%load% obj 26191
%force% Jack say Dad, hold my new sunglasses while I beat the crap out of %actor.name%
%force% Jack kill %actor.name%
~
#26112
Smithy's Holler~
0 k 50
~
wait 10
smile %actor.name%.
wait 10
%echo% Captain Smith shouts, 'ICEBERG... DEAD AHEAD!'.
wait 5
emote &ygrabs the genitals of &C%actor.name% &yand twists his hand 45 degrees.
%damage% %actor% 999
%damage% %actor% 250
emote &ychuckles as blood pours down the inner thigh of &C%actor.name%&n.
wait 400
end
~
#26113
Gangster - Greetings~
0 gi 100
~
if %actor.canbeseen% 
wait 50
say Hey %actor.name%...  You need anythin'... You let me know
wait 100
say I mean... we're like fucking family 'eh.
wait 100
say I mean %actor.name%... I can hook you up wif anythin' for the rite price
wait 20
nudge %actor.name%
wait 20
end
~
#26114
Gangster - Fight~
0 k 20
~
wait 20
emote &yhollers, 'Help me Boyz with this Crack Hoe!!!'&n
wait 10
%load% mob 26113
%echo% The Gangster's reinforcements have entered the room.
%force% maf kill %actor.name%
%load% mob 26113
%force% Maf say I'm gonna stick ya Ass!
%load% mob 26113
%force% Maf say You're gonna die you friggin' Lame Pussy!
wait 150
%force% Maf say Call in for more backup!!
wait 200
end
~
#26115
Macca's Voice~
0 bghi 65
~
if %actor.canbeseen% 
say Welcome to McNuggets %actor.name%, can I take your order.
wait 250
say Would you like fries with that?
wait 250
say Our burgers are 98.5 percent all preservatives.
wait 300
say Hello Sir... I mean Madame... How you going?
wait 25
%echo% The McNuggets Boss shouts, 'God damn it Eugene, map the damn floor!'
wait 250
say Would you like fries with that?
end
~
#26116
Mafia Bribe~
0 m 100
~
if (%actor.level% < 90)
say Come Back when you have Street Credit Biatch!
whap %actor.name%
else
wait 5
if (%amount% < 999990)
say I aint that cheap.  Up the anti.. And we'll talk business.
nudge %actor.name%
else
context %actor.id%;
set has_bribed_gangster 1
global has_bribed_gangster
say ehhhh
wait 30
shand %actor.name%
whisper %actor.name% It's been a business doing pleasure wif you.
wait 20
%load% obj 26109
give key %actor.name%
wait 20
%echo% The Gangster pulls out a revolver from his inner breast pocket.
wait 5
grin
wait 2
mkill %actor.name%
end
end
~
#26117
Jack - Fight~
0 k 55
~
if %self.canbeseen% (true)
wait 5
grin
wait 15
emote &nsends a blast of &rFire &ninto the face of &Y%actor.name%&n. 
%damage% %actor% 4000
emote laughs as %actor.name% ignites into &rFlames&n.
wait 200
end
~
#26118
Mafia~
2 g 50
~
if %actor.canbeseen% 
wait 10
%echo% The Junior Mafia Gangster has entered the room.
wait 10
%echo% The Gangster says, 'Look at this guy... ?'
wait 100
%echo% The Gangster says, 'Hey, %actor.name%, what are you doing in here... ?!?'
wait 50
%echo% The Gangster says, 'Hey, %actor.name%, I said... what are you doing in here... ?!?'
wait 100
%echo% The Gangster says, 'WHAT... You think you're better than me or somfin'
wait 10
%echo% The Gangster says, 'Cause I'll put a cap in your ass boy!'
wait 10
%echo% The Mafia Gangster says, 'Shut up Piss Boy and get me a cigarette!'
wait 7
%echo% The Gangster says, 'Yes Boss, I'm on my way Boss... You can count on me'
wait 10
%echo% The Mafia Gangster says, 'Just get me the damn cigarette and a light!'
wait 10
%echo% The Junior Mafia Gangster leaves the room.
end
~
#26119
Ozzy Fighting~
0 k 65
~
if %actor.canbeseen%
wait 10
emote grabs &Y%actor.name%&n and trys to bite their head off!
%damage% %actor% 990
wait 5
%echo% Blood pours out of the neck of %actor.name%.
wait 250
~
#26120
Car Rumble~
2 g 100
~
if (%actor.vnum% >= 0)
halt
end
wait 20
if (%actor.room% != %self.vnum%)
halt
end
%send% %actor% The rumble of the Ferrari's V12 QuadCam engine astonishes you.
wait 150
if (%actor.room% != %self.vnum%)
halt
end
%send% %actor% A few people have circled the Ferrari F50 trying to get a glimpse.
wait 150
if (%actor.room% != %self.vnum%)
halt
end
%send% %actor% Feeling impressed, you are starting to enjoy standing next to a Million Dollar Car.
wait 200
end
~
#26121
Tourist Photo~
0 gi 65
~
* Trigger disabled. Fix it. Checks must be made after waits. NPC Check Required.
halt
if %actor.canbeseen%
wait 30
emote takes out his camera from his backpack.
wait 20
say Hey %actor.name% the %actor.class%... say cheese!
wait 10 
smile %actor.name%
end
~
#26122
Memphis Fight~
0 k 70
~
if %actor.canbeseen%
wait 25
emote grabs a screwdriver from his jacket and stabs &Y%actor.name%&n extremely hard!
%damage% %actor% 990
wait 50
emote &yshouts, 'Did you enjoy that %actor.name%... Cause there's plenty more coming your way!'&n
wait 10
emote takes another violent swing at &Y%actor.name%&n with his screwdriver.
%damage% %actor% 900
wait 20
grin 
emote puts the screwdriver back in his pocket.
wait 200
end
~
#26123
1st Hand~
2 d 100
"I am in"~
if %actor.canbeseen% 
wait 5
%echo% The dealer says, 'We're all in then, lets start.'
wait 5
%echo% The dealer hands %actor.name% a card from the deck.
%load% obj 26111
%echo% The dealer smiles at %actor.name%.
%force% %actor.name% get jack
else
%echo% The dealer says, 'Is there a ghost in 'ere? You in or What?'
wait 20
end
~
#26124
Poker in~
2 bg 80
~
wait 10
%echo% The dealer says, 'We all in for this hand or what?'
wait 200
end
~
#26125
Jimmys Trigger~
2 d 0
i am in~
wait 10
%echo% Jimmy the Tulip says, 'hey %actor.name%.'
wait 10
%echo% Jimmy the Tulip says, 'You in for this game or what... ?'
wait 10
%echo% Jimmy the Tulip says, 'Cause if you're in, speak to the dealer.'
wait 10
%echo% Jimmy the Tulip says, 'Go tell that Sack of Shit that you're in.'
end
~
#26126
Jimmys 2nd Trigger~
0 f 100
~
%echo% The top of a card can be seen in the left sleeve of Jimmy's Corpse.
%load% obj 26114
end
~
#26127
Dealer Door Open~
0 g 100
~
smile %actor.name%
wait 25
eval ieq %actor.eq(inv)%
if (%ieq.vnum% == 26111) && (%ieq.vnum% == 26112) && (%ieq.vnum% == 26114)
wait 5
say Nice Cards %actor.name%.
else
say Need a flush to win these days!
end
~
#26128
Smith Dead~
0 f 100
~
%load% obj 26115
end
~
#26129
Jack Death~
0 f 100
~
%echo% As the corpse of Jack Nicholson falls to a heap, a card falls out of his jacket.
%load% obj 26113
end
~
$~
