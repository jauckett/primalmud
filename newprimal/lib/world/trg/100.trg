#10000
Merchant controll~
0 h 100
~
eval class %actor.class%
if (%class% == thief)
emote grabs his purse creating a jingling effect!
say I can SEE you! Be gone you Scoundrel!
elseif (%class% == warrior)
emote peers around nervously
say Beware the Den of Thieves, you brave %class%
elseif (%class% == cleric)
emote breaths a huge sigh of relief!
say At least I know my purse is safe with a %class% around town!
elseif (%class% == magician)
emote tries to run away in fear!
flee
end
~
#10001
Empty Trigger~
0 h 100
~
say this is free to use!
~
#10025
Going through the rapids~
2 g 100
~
wait 5
%echo% You are pulled down into the water and sucked out the crevice!
wait 5 
%echo% You are flung over the edge of an underground waterfall!
wait 5
%echo% You decent continues down at an alarming rate!
wait 5
%echo% You touch splash down and touch bottom!
wait 2
%echo% The current grabs you again and pushes you forward!
wait 5
%echo% You are swept swiftly down the river!
wait 5
%echo% You are thrown from side to side as you hurtle over the underground rapids!
wait 5
%echo% You notice a decrease in your forward movement!
wait 10
%echo% You are at the edge of the rapids and you find you can start to control your own body!
wait 15
%echo% You land on your feet and discover you are in waist high water!
%teleport% all 10026
~
$~
