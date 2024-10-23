
(define (problem doors_65) (:domain doors)
  (:objects
        key-0 - key
	key-1 - key
	key-2 - key
	key-3 - key
	loc-0-0 - location
	loc-0-1 - location
	loc-0-2 - location
	loc-0-3 - location
	loc-0-4 - location
	loc-0-5 - location
	loc-0-6 - location
	loc-0-7 - location
	loc-1-0 - location
	loc-1-1 - location
	loc-1-2 - location
	loc-1-3 - location
	loc-1-4 - location
	loc-1-5 - location
	loc-1-6 - location
	loc-1-7 - location
	loc-2-0 - location
	loc-2-1 - location
	loc-2-2 - location
	loc-2-3 - location
	loc-2-4 - location
	loc-2-5 - location
	loc-2-6 - location
	loc-2-7 - location
	loc-3-0 - location
	loc-3-1 - location
	loc-3-2 - location
	loc-3-3 - location
	loc-3-4 - location
	loc-3-5 - location
	loc-3-6 - location
	loc-3-7 - location
	loc-4-0 - location
	loc-4-1 - location
	loc-4-2 - location
	loc-4-3 - location
	loc-4-4 - location
	loc-4-5 - location
	loc-4-6 - location
	loc-4-7 - location
	loc-5-0 - location
	loc-5-1 - location
	loc-5-2 - location
	loc-5-3 - location
	loc-5-4 - location
	loc-5-5 - location
	loc-5-6 - location
	loc-5-7 - location
	loc-6-0 - location
	loc-6-1 - location
	loc-6-2 - location
	loc-6-3 - location
	loc-6-4 - location
	loc-6-5 - location
	loc-6-6 - location
	loc-6-7 - location
	loc-7-0 - location
	loc-7-1 - location
	loc-7-2 - location
	loc-7-3 - location
	loc-7-4 - location
	loc-7-5 - location
	loc-7-6 - location
	loc-7-7 - location
	room-0 - room
	room-1 - room
	room-2 - room
	room-3 - room
	room-4 - room
  )
  (:init 
	(at loc-5-0)
	(keyat key-2 loc-1-1)
	(keyat key-3 loc-7-6)
	(keyforroom key-0 room-1)
	(keyforroom key-1 room-2)
	(keyforroom key-2 room-3)
	(keyforroom key-3 room-4)
	(locinroom loc-0-0 room-0)
	(locinroom loc-0-1 room-0)
	(locinroom loc-0-2 room-0)
	(locinroom loc-0-3 room-0)
	(locinroom loc-0-4 room-1)
	(locinroom loc-0-5 room-0)
	(locinroom loc-0-6 room-3)
	(locinroom loc-0-7 room-4)
	(locinroom loc-1-0 room-0)
	(locinroom loc-1-1 room-2)
	(locinroom loc-1-2 room-0)
	(locinroom loc-1-3 room-0)
	(locinroom loc-1-4 room-1)
	(locinroom loc-1-5 room-0)
	(locinroom loc-1-6 room-3)
	(locinroom loc-1-7 room-4)
	(locinroom loc-2-0 room-0)
	(locinroom loc-2-1 room-0)
	(locinroom loc-2-2 room-0)
	(locinroom loc-2-3 room-0)
	(locinroom loc-2-4 room-1)
	(locinroom loc-2-5 room-0)
	(locinroom loc-2-6 room-3)
	(locinroom loc-2-7 room-4)
	(locinroom loc-3-0 room-0)
	(locinroom loc-3-1 room-0)
	(locinroom loc-3-2 room-0)
	(locinroom loc-3-3 room-0)
	(locinroom loc-3-4 room-1)
	(locinroom loc-3-5 room-0)
	(locinroom loc-3-6 room-3)
	(locinroom loc-3-7 room-4)
	(locinroom loc-4-0 room-0)
	(locinroom loc-4-1 room-0)
	(locinroom loc-4-2 room-0)
	(locinroom loc-4-3 room-0)
	(locinroom loc-4-4 room-1)
	(locinroom loc-4-5 room-0)
	(locinroom loc-4-6 room-3)
	(locinroom loc-4-7 room-4)
	(locinroom loc-5-0 room-0)
	(locinroom loc-5-1 room-0)
	(locinroom loc-5-2 room-0)
	(locinroom loc-5-3 room-0)
	(locinroom loc-5-4 room-1)
	(locinroom loc-5-5 room-0)
	(locinroom loc-5-6 room-3)
	(locinroom loc-5-7 room-4)
	(locinroom loc-6-0 room-0)
	(locinroom loc-6-1 room-0)
	(locinroom loc-6-2 room-0)
	(locinroom loc-6-3 room-0)
	(locinroom loc-6-4 room-1)
	(locinroom loc-6-5 room-0)
	(locinroom loc-6-6 room-3)
	(locinroom loc-6-7 room-4)
	(locinroom loc-7-0 room-0)
	(locinroom loc-7-1 room-0)
	(locinroom loc-7-2 room-0)
	(locinroom loc-7-3 room-0)
	(locinroom loc-7-4 room-1)
	(locinroom loc-7-5 room-0)
	(locinroom loc-7-6 room-3)
	(locinroom loc-7-7 room-4)
	(moveto loc-0-0)
	(moveto loc-0-1)
	(moveto loc-0-2)
	(moveto loc-0-3)
	(moveto loc-0-4)
	(moveto loc-0-5)
	(moveto loc-0-6)
	(moveto loc-0-7)
	(moveto loc-1-0)
	(moveto loc-1-1)
	(moveto loc-1-2)
	(moveto loc-1-3)
	(moveto loc-1-4)
	(moveto loc-1-5)
	(moveto loc-1-6)
	(moveto loc-1-7)
	(moveto loc-2-0)
	(moveto loc-2-1)
	(moveto loc-2-2)
	(moveto loc-2-3)
	(moveto loc-2-4)
	(moveto loc-2-5)
	(moveto loc-2-6)
	(moveto loc-2-7)
	(moveto loc-3-0)
	(moveto loc-3-1)
	(moveto loc-3-2)
	(moveto loc-3-3)
	(moveto loc-3-4)
	(moveto loc-3-5)
	(moveto loc-3-6)
	(moveto loc-3-7)
	(moveto loc-4-0)
	(moveto loc-4-1)
	(moveto loc-4-2)
	(moveto loc-4-3)
	(moveto loc-4-4)
	(moveto loc-4-5)
	(moveto loc-4-6)
	(moveto loc-4-7)
	(moveto loc-5-0)
	(moveto loc-5-1)
	(moveto loc-5-2)
	(moveto loc-5-3)
	(moveto loc-5-4)
	(moveto loc-5-5)
	(moveto loc-5-6)
	(moveto loc-5-7)
	(moveto loc-6-0)
	(moveto loc-6-1)
	(moveto loc-6-2)
	(moveto loc-6-3)
	(moveto loc-6-4)
	(moveto loc-6-5)
	(moveto loc-6-6)
	(moveto loc-6-7)
	(moveto loc-7-0)
	(moveto loc-7-1)
	(moveto loc-7-2)
	(moveto loc-7-3)
	(moveto loc-7-4)
	(moveto loc-7-5)
	(moveto loc-7-6)
	(moveto loc-7-7)
	(pick key-0)
	(pick key-1)
	(pick key-2)
	(pick key-3)
	(unlocked room-0)
	(unlocked room-1)
	(unlocked room-2)
  )
  (:goal (and
	(at loc-0-7)))
)
