#7510
lift~
2 c 100
press~
if %arg% == 1
%echo% The lift hums as it begins to move.
wait 75
%echo% The lift comes to a gentle stop, the doors open and you step out.
%teleport% %actor% 7627
%force% %actor% look
elseif %arg% == 4
%echo% The lift hums as it begins to move.
wait 75
%echo% The lift comes to a gentle stop, the doors open and you step out.
%teleport% %actor% 7512
%force% %actor% look
elseif %arg% == 6
%echo% The lift hums as it begins to move.
wait 75
%echo% The lift comes to a gentle stop, the doors open and you step out.
%teleport% %actor% 7507
%force% %actor% look
elseif %arg% == 8
%echo% The lift hums as it begins to move.
wait 75
%echo% The lift comes to a gentle stop, the doors open and you step out.
%teleport% %actor% 7552
%force% %actor% look
else
%echo% The female voice tells you you are not authorized to travel there, and that you may only travel to decks 1, 4, 6 or 8 without proper Starfleet identification.
end
~
$~
