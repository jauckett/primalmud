#31000
Snow Falling~
2 g 10
~
if !(%actor.vnum% < 0)
  return 0
  halt
end
wait 2
%send% %actor.name% You wipe some snow from your face as it begins to melt.
return 0
~
#31001
Turn Command~
2 c 100
*~
* NPCs don't turn.
if ((%actor.vnum% >= 0) || (%actor.level% >= 110))
  return 0
  halt
end
* Not sliding, no need to turn.
if !(%actor.varexists(310slide)%)
  if (%cmd% != turn)
    return 0
    halt
  end
  %send% %actor% But you're not sliding.
  return 1
  halt
end
if (%cmd.mudcommand%) 
  set testcmd %cmd.mudcommand%
else
  set testcmd %cmd%
end
switch %testcmd%
  case north
  case east
  case west
  case south
    if (%testcmd% == %actor.310slide%)
      return 0
      halt
    end
  case cast
  case down
  case up
  case flee
  case retreat
  case enter
  case escape
  case go
  case leave
  case sit
  case sleep
  case rest
  case follow
    %send% %actor% You're too busy trying to keep your balance as you slide across the ice.
    return 1
    halt
    break
  case turn
    break
  default
    return 0
    halt
done
* Counter... When it reaches 5, the user falls through the ice... Basically,
* typing turn 5 times in the same room, without sliding.
set 310slide %actor.310slide%
if !(%actor.varexists(310slcnt)%)
  set 310slcnt 0
  remote 310slcnt %actor.id%
end
eval 310slcnt %actor.310slcnt% + 1
remote 310slcnt %actor.id%
* Send them to the ice room.
if (%310slcnt% > 4)
  %send% %actor% You wear a hole in the ice, and fall through.
  %echoaround% %actor.name% %actor.name% wears a hole in the ice, and falls through.
  %teleport% %actor% 31028
  set 310hole %self.vnum%
  remote 310hole %actor.id%
  rdelete 310slide %actor.id%
  rdelete 310slcnt %actor.id%
  return 1
  halt
end
unset 310slcnt
* Change their direction. (Dex Check First)
if (%random.18% < %actor.dex%)
  switch (%310slide%)
    case north
      if !(%self.east%)
	if !(%self.south%)
	  if !(%self.west%)
	    break
	  end
	  set 310slide west
	  break
	end
	set 310slide south
	break
      end
      set 310slide east
      break
    case east
      if !(%self.south%)
	if !(%self.west%)
	  if !(%self.north%)
	    break
	  end
	  set 310slide north
	  break
	end
	set 310slide west
	break
      end
      set 310slide south
      break
    case south
      if !(%self.west)
	if !(%self.north%)
	  if !(%self.east%)
	    break
	  end
	  set 310slide east
	  break
	end
	set 310slide north
	break
      end
      set 310slide west
      break
    default
      if !(%self.north%)
	if !(%self.east%)
	  if !(%self.south%)
	    break
	  end
	  set 310slide south
	  break
	end
	set 310slide east
	break
      end
      set 310slide north
      break
  done
end
if (%310slide% != %actor.310slide%)
  %send% %actor% You plant your foot, and turn %310slide%.
  %echoaround% %actor.name% %actor.name% skillfuly plants %actor.hisher% foot, turning %310slide%.
  remote 310slide %actor.id%
else
  %send% %actor% You plant your foot, to no effect.
end
~
#31002
Ice Skating (Enter)~
2 g 101
~
* Dont care about NPCs...
if ((%actor.vnum% >= 0) || (%actor.level% >= 110))
  halt
end
* Usual snow stuff...
if (%random.100% < 11)
  wait 1
  %send% %actor% You wipe some snow from your face as it begins to melt
end
* Remove the slide counter...
if (%actor.varexists(310slcnt)%)
  rdelete 310slcnt %actor.id%
end
* Don't slide whilst flying...
if (%actor.affected(17)%)
  halt
end
* Bump into mobs, if they're next in room..
if ((%actor.next_in_room%) && !(%actor.next_in_room(vnum)% < 0))
  set tmpch %actor.next_in_room%
  if (!(%tmpch.fighting%) && %actor.next_in_room(can_see_me)%)
    %send% %actor% You slide into %actor.next_in_room(name)%, who is not impressed.
    wforce %actor.next_in_room% kill %actor.name%
    rdelete 310slide %actor.id%
    return 0
    halt
  end
end
* If we're already sliding, that's enough...
if (%actor.varexists(310slide)%)
  halt
end
* They aren't marked as sliding yet.. Better fix that.
switch (%direction%)
  case west
    set 310slide east
    break
  case east
    set 310slide west
    break
  case south
    set 310slide north
    break
  case north
    set 310slide south
    break
done
remote 310slide %actor.id%
wait 1
%send% %actor% You start sliding %310slide% across the lake.
~
#31003
Ice Skating (Random)~
2 b 100
~
* Loop through all in room...
set ch %self.people%
while (%ch%)
  set next_ch %ch.next_in_room%
  * This only affects sliders...
  if ((%ch.vnum% < 0) && (%ch.varexists(310slide)%) && (%ch.level% < 110))
    * Reset the slide counter.
    if (%ch.varexists(310slcnt)%)
      rdelete 310slcnt %ch.id%
    end
    * Don't slide when fighting.
    if (%ch.fighting%)
      rdelete 310slide %ch.id%
    else
      * Determine the direction we're sliding is valid.
      set 310slide %ch.310slide%
      switch %310slide%
	case north
	  if (%self.north%)
	    set sendto %self.north(vnum)%
	  end
	  break
	case south
	  if (%self.south%)
	    set sendto %self.south(vnum)%
	  end
	  break
	case east
	  if (%self.east%)
	    set sendto %self.east(vnum)%
	  end
	  break
	case west
	  if (%self.west%)
	    set sendto %self.west(vnum)%
	  end
	  break
      done
      * If we don't have a direction, then we're stopping..
      if (!(%sendto%) || (%sendto% < 1))
	unset 310slide
	rdelete 310slide %ch.id%
	%send% %ch% You bump into the edge of the lake and come to a halt.
	%echoaround% %ch.name% %ch.name% bumps into the edge of the lake, coming to a halt.
      else
	* Time to make the char "slide"...
	wait 1
	%send% %ch% You slide across the ice, %310slide%.
	%force% %ch% %ch.310slide%
*	%echoaround% %ch% %ch.name% slides across the ice, %310slide%.
*	%teleport% %ch% %sendto%
*	%force% %ch% look
	if ((%sendto% > 31027) || (%sendto%) < 31003))
	  wait 3
	  if (%ch.varexists(310slide)%)
	    rdelete 310slide %ch.id%
	  end
	  if (%ch.varexists(310slcnt)%)
	    rdelete 310slcnt %ch.id%
	  end
	  %send% %ch% The ground here is a little less slippery.
	end
      end
    end
  end
  set ch %next_ch%
done
~
#31004
Climb Command~
2 c 100
*~
if (%actor.vnum% >= 0) 
  return 0
  halt
end
if (%cmd% == climb)
  if (%actor.varexists(310hole)%)
    set to_room %actor.310hole%
    rdelete 310hole %actor.id%
  else
    set to_room 31007
  end
  %send% %actor% You climb out of the hole you recently created.
  %teleport% %actor% %to_room%
  wait 3
  %echoaround% %actor.name% %actor.name% climbs out of a hole %actor.heshe% had created.
  %force% %actor% look
  return 1
  halt
end
if ((%cmd% == u) || (%cmd% == up) || (%cmd.mudcommand% == exits))
  %send% %actor% The only way out of here is to climb back through the hole.
  return 1
  halt
end
if ((%cmd.mudcommand% == spy) && ((%arg% == u) || (%arg% == up))
  if (%actor.varexists(310hole)%)
    %teleport% %actor% %actor.310hole%
  else
    %teleport% %actor% 31007
  end
  %force% %actor% look
  %teleport% %actor% 31028
  return 1
  halt
end
return 0
~
#31005
Fall Through Hole (Enter)~
2 g 100
~
if ((%actor.vnum% >= 0) || (%actor.level% >= 110))
  halt
end
if !(%actor.varexists(310slide)%)
  halt
end
rdelete 310slide %actor.id%
rdelete 310slcnt %actor.id%
rdelete 310hole %actor.id%
wait 2
%send% %actor% You slide right over a hole in the ice. Gravity embraces you.
%echoaround% %actor.name% %actor.name% slides right over the hole and falls through.
%teleport% %actor% 31029
if (%actor.room% == 31029)
  %force% %actor% look
  %echoaround% %actor.name% %actor.name% falls into the room.
end
~
#31006
Break Through Ice (Command)~
2 c 100
*~
if (%actor.vnum% >= 0)
  return 0
  halt
end
if (%self.vnum% == 31039)
  if ((%cmd% == u) || (%cmd% == up))
    if (%self.up%)
      return 0
      halt
    end
    if (%random.18% > %actor.con%)
      eval dmg %actor.hitp% / 9
    else
      eval dmg %actor.hitp% / 11
    end
    %send% %actor% You hit your head on the frozen surface. You should break through the ice first.
    %echoaround% %actor.name% %actor.name% hits %actor.hisher% head on the ceiling.
    %damage% %actor% %dmg%
    return 1
    halt
  end
  if (((%cmd% == look) || (%cmd.mudcommand% == look)) && ((%arg% == u) || (%arg% == up)))
    if !(%self.up%)
      %send% %actor% It looks like you might be able to break through the ice, here.
      return 1
      halt
    end
  end
else
  if ((%cmd% == down) || (%cmd.mudcommand% == down))
    if (%self.down%)
      return 0
      halt
    end
    %send% %actor% You'll need to break through the ice, first.
    return 1
    halt
  end
  if (((%cmd% == look) || (%cmd.mudcommand% == look)) && ((%arg% == down) || (%arg.mudcommand% == down)))
    if !(%self.down%)
      %send% %actor% It looks like you might be able to break through the ice, here.
      return 1
      halt
    end
  end
end
if (%cmd% == break)
  if !(%arg%)
    %send% %actor% Just what do you wish to break?
    return 1
    halt
  end
  if (%arg% != ice)
    %send% %actor% You can't damage that!
    return 1
    halt
  end
  if (%self.vnum% == 31039)
    if (%self.up%)
      %send% %actor% It isn't getting any more broken!
      return 1
      halt
    end
  else
    if (%self.down%)
      %send% %actor% It isn't getting any more broken!
      return 1
      halt
    end
  end
  set strchk %random.18%
  if (%strchk% > %actor.str%)
    eval strchk %strchk% - %actor.str%
    %send% %actor% You fail.
    %echoaround% %actor.name% %actor.name% attempts to break the ice, but only ends up hurting %actor.himher%self.
    switch (%strchk%)
      case 1
	%damage% %actor% %actor.hitp% / 10
	break
      case 2
      case 3
	%damage% %actor% %actor.hitp% / 15
	break
      default
	%damage% %actor% %actor.hitp% / 20
	break
    done
    unset strchk
    return 1
    halt
  end
  if (%actor.varexists(310break)%)
    if (%310break% > 2)
      set 310break 1
    else
      eval 310break %actor.310break% + 1
    end
  else
    set 310break 1
  end
  remote 310break %actor.id%
  switch (%310break%)
    case 1
      %send% %actor% You manage to crack the ice.
      %echoaround% %actor% %actor.name% charges the ice, resulting in a cracking sound.
      %damage% %actor% %actor.hitp% / 20
      break
    case 2
      %send% %actor% You crack the ice a little more.
      %echoaround% %actor% %actor.name% charges the ice, resulting in a loud cracking sound!
      %damage% %actor% %actor.hitp% / 30
      break
    case 3
      %send% %actor% You break through the ice!
      %echoaround% %actor% %actor.name% has broken through the ice.
      if (%self.vnum% == 31039)
	%send% 31040 Someone has broken through the ice, from below.
      else
	%send% 31039 Someone has broken through the ice, from above.
      end
      %door% 31039 up room 31040
      %door% 31039 up description Light shines in from above.
      %door% 31040 down room 31039
      %door% 31040 down description A hole through the ice reveals water below.
      rdelete 310break %actor.id%
      break
  done
  return 1
  halt
end
return 0
~
#31007
Freeze 31039 (Zone Reset)~
2 f 100
~
if !(%self.up%)
  halt
end
%door% 31039 up purge
%echo% The water above freezes over, sealing the hole.
~
#31008
Freeze 31040 (Zone Reset)~
2 f 100
~
if !(%self.down%)
  halt
end
%door% 31040 down purge
%echo% The water below freezes over, sealing the hole.
~
#31009
Tux - Heal (Hit Percent)~
0 l 50
~
mecho Before: %self.hitp%
switch (%random.10%)
  case 1
    %damage% tux 0 - 100
  case 2
    %damage% tux 0 - %random.100%
  case 3
    %damage% tux 0 - %random.100%
    %echoaround% %self.name% %self.name% patches %self.himher%self.
    mecho After: %self.hitp%
    halt
    break
done
~
#31010
Pikefish Eats (Random)~
0 b 10
~
%echo% %self.name% swallows a small fish, whole.
~
#31011
Muskrat Eats (Random)~
0 b 10
~
if (%random.10% == 1)
  %echo% %self.name% eats a tadpole.
else
  %echo% %self.name% nibbles on the root of a weed.
end
~
#31012
Roc Above (Enter)~
2 g 100
~
if (%actor.vnum% >= 0)
  halt
end
wait 10
switch (%self.vnum%)
  case 31068
    if !(%firstchar.31084%)
      halt
    end
    break
  case 31072
    if !(%firstchar.31085%)
      halt
    end
    break
  case 31081
    if !(%firstchar.31086%)
      halt
    end
    break
done
if (%actor.room% != %self.vnum%)
  %send% %actor% You hear something hit the ground behind you.
  halt
end
if (%actor.eq(head)%)
  %send% %actor% Some snow falls from above and bounces off your headgear.
  %echoaround% %actor.name% Some snow falls from above and bouces off %actor.name%'s headgear.
  halt
end
%send% %actor% Some snow falls from above and hits you in the head.
%echoaround% %actor.name% Some snow falls from above and hits %actor.name% on the head.
%damage% %actor% %actor.level%
~
#31013
Roc Above (Command)~
2 c 100
*~
if (%actor.vnum% >= 0)
  return 0
  halt
end
if ((%cmd% == look) || (%cmd.mudcommand% == look))
  if ((%arg% == u) || (%arg% == up))
    %send% %actor% A large shadow falls from the tree above.
    return 1
    halt
  end
  return 0
  halt
end
if ((%cmd% == u) || (%cmd% == up))
  %send% %actor% What, you think you can just walk up there?  Perhaps you should try climbing.
  return 1
  halt
end
if (%cmd% != climb)
  return 0
  halt
end
if !(%arg%)
  %send% %actor% Climb what?!?
  halt
  return 1
end
if (%arg% != tree)
  %send% %actor% You can't climb that!
  halt
  return 1
end
* This is a little bit lazy, but its probably more efficient.
set retroom %self.vnum%
%teleport% %actor% 31083
switch (%self.vnum%)
  case 31068
    %force% %actor% south
    break
  case 31072
    %force% %actor% north
    break
  case 31081
    %force% %actor% east
    break
done
if ((%actor.room% == 31083) || (%actor.room% == %retroom%))
  %teleport% %actor% %retroom%
  %echoaround% %actor.name% %actor.name% tries to climb a tree, but fails.
else
  %send% %retroom% %actor.name% climbs up a tree, and disappears.
end
~
#31014
Tux Below (Enter)~
2 g 100
~
if (%actor.vnum% >= 0)
  halt
  end
if (%self.down%)
  halt
end
wait 10
if (%actor.room% == %self.vnum%)
  %send% %actor% You feel the ground move beneath your feet.
end
~
#31015
Tux Below (Command)~
2 c 100
*~
if (%actor.vnum% >= 0)
  return 0
  halt
end
if (%self.down%)
  if (%cmd% == dig)
    %send% %actor% Dig through air?!?
    return 1
    halt
  end
  return 0
  halt
end
if (((%cmd% == down) || (%cmd.mudcommand% == down)) || (((%cmd% == look) || (%cmd.mudcommand% == look)) && ((%arg% == down) || (%arg.mudcommand% == down))))
  %send% %actor% You might be able to dig your way down here.
  return 1
  halt
end
if (%cmd% != dig)
  return 0
  halt
end
if (%actor.varexists(310dig)%)
  eval 310dig %actor.310dig% + 1
  if (%310dig% > 3)
    set 310dig 1
  end
else
  set 310dig 1
end
remote 310dig %actor.id%
switch (%310dig%)
  case 1
    %send% %actor% As you dig, the ground rumbles some more.
    %echoaround% %actor.name% %actor.name% starts digging in the snow.
    set 310dig 1
    remote 310dig %actor.id%
    break
  case 2
    %send% %actor% You dig some more, until you reach something hard.
    %echoaround% %actor.name% %actor.name% digs a little further.
    break
  case 3
    %send% %actor% You break through some roots to discover an underground hideaway.
    %echoaround% %actor.name% %actor.name% has dug right through the ground!
    %door% 31080 down room 31087
    %door% 31080 down description Down below, a secret hideaway has been unveiled.
    break
done
~
#31016
Roc Dies (Death)~
0 f 100
~
switch (%self.room%)
  case 31084
    set toroom 31068
    set ch %firstchar.31068%
    break
  case 31085
    set toroom 31072
    set ch %firstchar.31072%
    break
  case 31086
    set toroom 31081
    set ch %firstchar.31081%
    break
  default
    return 1
    halt
    break
done
%echo% %self.name% falls to the ground, lifeless.
%teleport% %self% %toroom%
wsend %toroom% A large bird falls from above, quite dead.
while (%ch%)
  if (%ch.vnum% < 0)
    if (%random.18% > %ch.dex%)
      %send% %ch% You are hit by the corpse of the Roc.
      %echoaround% %ch.name% %ch.name% is hit by the corpse of the Roc.
      %damage% %ch% 100
    end
  end
  set ch %ch.next_in_room%
done
return 0
~
#31017
Tux Hole Seal (Zone Reset)~
2 f 100
~
if (%self.down%)
  %door% 31080 down purge
end
~
#31018
Tux Quest Speech (Enter)~
0 g 100
~
if (%actor.vnum% >= 0)
  halt
end
if !(%actor.varexists(310tuxq)%)
  set 310tuxq 0
else
  set 310tuxq %actor.310tuxq%
end
set modifier 0
switch (%actor.class%)
  case Magician
  case Cleric
  case Thief
  case Warrior
    break
  case Master
    if (%310tuxq% > 9)
      set modifier 10
    else
      if (%310tuxq% > 4)
	set modifier 5
      end
    end
    break
  default
    if (%310tuxq% > 4)
      set modifier 5
    end
    break
done
eval 310tuxq %310tuxq% - %modifier%
switch (%310tuxq%)
  case 0
    wait 8
    if (%actor.room% != %self.room%)
      halt
    end
    tell %actor.name% You look like a strong adventurer, %actor.name%.
    tell %actor.name% I was wondering if I might ask you for assistance.
    tell %actor.name% You see, there's this daemon hiding somewhere near the sealed lake, and he's been causing all sorts of trouble for my penguin kin.
    tell %actor.name% If you could be so kind as to rid us of this evil being, I would reward you as best I could.
    wait
    if (%actor.room% != %self.room%)
      tell %actor.name% Well, thanks for hearing me out.
      halt
    end
    tell %actor.name% If you will help me, type type 'accept' now.
    halt
  case 1
    wait 8
    if (%actor.room% != %self.room%)
      halt
    end
    tell %actor.name% Please help us get rid of the daemon, %actor.name%
    halt
  case 2
    wait 8
    if (%actor.room% != %self.room%)
      halt
    end
    tell %actor.name% Thank you so much, now my penguin friends can roam about the ice without the thrat of that evil daemon.
    tell %actor.name% Please, take this reward.
    switch (%modifier%)
      case 0
	mload obj 31005
	break
      case 5
	mload obj 31006
	break
      case 10
	mload obj 31007
	break
    done
    give reward %actor.name%
    drop reward
    eval 310tuxq 5 + %modifier%
    remote 310tuxq %actor.id%
    halt
  case 5
    wait 8
    if (%actor.room% == %self.room%)
      tell %actor.name% Always a pleasure to see you, %actor.name%. Thanks again for yur assistance.
      %echoaround% %actor.name% %self.name% winks at %actor.name%.
    end
    halt
    break
done
~
#31019
Tux Quest Accept (Command)~
0 c 100
accept~
if ((%actor.vnum% >= 1) || !(%actor.canbeseen%))
  halt
end
if (%actor.varexists(310tuxq)%)
  switch (%actor.310tuxq%)
    case 10:
      if (%actor.class% != Master)
	tell %actor.name% You cannot participate in the quest, at this time.
	halt
      end
    case 5:
      if (!((%actor.class% != Warrior) && (%actor.class% != Cleric) && (%actor.class% != Thief) && (%actor.class% != Mage)))
	tell %actor.name% You cannot participate in the quest, at this time.
	halt
      end
    case 0:
      tell %actor.name% We are ever so grateful for your time and efforts. May your journey be free of peril.
      eval 310tuxq %actor.310tuxq% + 1
      remote 310tuxq %actor.id%
      halt
      break
    default:
      tell %actor.name% You cannot participate in the quest, at this time.
      halt
  done
end
tell %actor.name% We are ever so grateful for your time and efforts. May your adventure be free of peril, and full of success.
set 310tuxq 1
remote 310tuxq %actor.id%
~
#31020
Dig Var Delete~
2 g 100
~
if ((%actor.vnum% < 0) && (%actor.varexists(310dig)%)
  rdelete 310dig %actor.id%
end
~
#31021
Enter the Daemon (Enter)~
2 g 100
~
if (%actor.vnum% >= 0)
  halt
end
if (%actor.varexists(310tuxq)%)
  set 310tuxq %actor.310tuxq
else
  halt
end
switch (%actor.class%)
  case Magician
  case Cleric
  case Thief
  case Warrior
    break
  case Master
    if (%310tuxq% > 9)
      set modifier 10
    else
      if (%310tuxq% > 4)
	set modifier 5
      end
    end
    break
  default
    if (%310tuxq% > 4)
      set modifier 5
    end
    break
done
eval 310tuxq %310tuxq% - %modifier%
if (%310tuxq% != 1)
  halt
end
* If we get this far, I guess we'd better teleport them :o)
wait 2
if (%actor.room% != 31040)
  halt
end
%send% %actor% Everything suddenly goes dark...
wait 2
if (%actor.room% != 31040)
  halt
end
set 310fall 1
remote 310fall %actor.id%
wteleport %actor% 31088
%send% %actor% All you can see and think about is the stars as you fall through unknown space.
~
#31022
Continue Falling (Random)~
2 b 100
~
set ch %self.people%
while (%ch%)
  set nextch %ch.next_in_room%
  if (%ch.vnum% >= 0)
    wsend %ch% Just what are you doing here?
    wteleport %ch% 31040
  else
    if !((%ch.varexists(310tuxq)%) && (%ch.varexists(310fall)%))
      wsend %ch% You stop falling, and land gently on the ground.
      wteleport %ch% 31040
    else
      switch (%ch.310tuxq%)
	case 1
	case 6
	case 11
	  if (%ch.310fall% < 3)
	    wsend %ch% Stars continue to fly by as you fall through unknown space.
	    eval 310fall %ch.310fall% + 1
	    remote 310fall %ch.id%
	  else
	    wsend %ch% You stop falling, and land gently on the ground.
	    wteleport %ch% 31089
	    rdelete 310fall %ch.id%
	  end
	  break
	default
	  wsend %ch% You stop falling, and land gently on the ground.
	  wteleport %ch% 31040
	  break
      done
    end
  end
  wforce %ch% look
  set ch %nextch%
done
~
#31023
Falling Command Block (Command)~
2 c 100
*~
if (%actor.level% < 110)
  %send% %actor% All you can do is think about and watch the stars as you fall through space.
  return 1
  halt
end
return 0
~
#31024
Daemon Dies (Death)~
0 f 100
~
switch (%self.room%)
  case 31089
    set ch %firstchar.31089%
    break
  case 31090
    set ch %firstchar.31090%
    break
  default
    halt
done
%door% 31090 down room 31040
%door% 31090 down description Down below, you see the extinct volcano.
while (%ch%)
  set nextch %ch.next_in_room%
  if ((%actor.vnum% < 0) && (%actor.varexists(310tuxq)%))
    switch (%actor.310tuxq%)
      case 1
      case 6
      case 11
	eval 310tuxq %actor.310tuxq% + 1
	remote 310tuxq %actor.id%
	msend %actor% \&[\&YYou have completed Tux's quest!\&]
	msend %actor% Perhaps you should go and report to him.
	break
    done
  end
  set ch %nextch%
done
~
#31025
Close exit to Volcano (ZReset)~
2 f 100
~
if (%self.up%)
  %door% 31090 down purge
end
~
#31026
Quest Daemon Fight Stuff (Fighting)~
0 k 100
~
set rval %random.50%
if (%rval% == 1)
  mechoaround %self% %self.name% closes %self.hisher% eyes, mumbles some words, and punches the ground.
  mecho A large meteor starts falling from the sky.
  wait 10
  mecho The meteor gets closer to the ground.
  wait 10
  mecho \&[\&rThe meteor comes crashing down. You hear the wailing of the banshee.\&]
  set ch %self.first_in_room%
  set delay 0
  while (%ch%)
    set nextch %ch.next_in_room%
    if (%ch.vnum% < 0)
      if (%random.2% == 1)
	mdamage %ch% %ch.hitp% - 1
      else
	mdamage %ch% %ch.hitp% / 2
      end
      set delay 1
    end
    set ch %nextch%
  done
  if (%delay% == 1)
    wait 100
  end
  halt
end
if (%rval% < 10)
  mechoaround %self% %self.name% extends %self.hisher% pitchfork, and spins.
  set ch %self.first_in_room%
  while (%ch%)
    set nextch %ch.next_in_room%
    if (%ch.vnum% < 0)
      if (%random.18% > %ch.dex%)
	msend %ch% You are hit by %self.name%'s pitchfork as %self.heshe% spins.
	mdamage %ch% %ch.hitp% / 10
      else
	msend %ch.name% You duck out of the way of %self.name%'s pitchfork.
      end
    end
    set ch %nextch%
  done
  wait 50
  halt
end
if (%rval% < 15)
  %echoaround% %self% %self.name% patches %self.himher%self up.
  eval amt %random.100% + 100
  mrestore %amt%
  wait 50
  halt
end
if (%rval% < 20)
  mat 31089 mecho A wall of fire appears from nowhere, scorching the island as it passes across.
  mat 31090 mecho A wall of fire appears from nowhere, scorching the island as it passes across.
  set ch %firstchar.31089%
  if !(%ch%)
    set ch %firstchar.31090%
  end
  while (%ch%)
    set nextch %ch.next_in_room%
    if (!(%nextch%) && (%ch.room% == 31089))
      set nextch %firstchar.31090%
    end
    if (%ch.vnum% < 0)
      msend %ch% Your eyebrows are singed!
      mdamage %ch% 50
    end
    set ch %nextch%
  done
  wait 100
  halt
end
~
$~
