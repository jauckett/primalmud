#5200
tumblweed rolls in- enter~
0 bgi 100
~
* Disabled by artus -- Continuous crashing. Lots of checks missed.
halt
if %actor.canbeseen%
%mpecho% A heavy gust of wind blows the tumble weed in  your direction
%emote% tangles itself around your legs making it difficult for you to move
%damage% %actor% 200
wait 2
kill %actor.name%
end
~
#5201
new trigger~
0 g 100
0~
say My trigger commandlist is not complete!
mpecho YEH YEH
s
s
~
$~
