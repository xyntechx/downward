(define (domain gripper-strips)
   (:predicates (room ?r)
        (ball ?b)
        (gripper ?g)
        (at-robby ?r)
        (at ?b ?r)
        (free ?g)
        (carry ?o ?g))

   (:action move
       :parameters  (?from ?to)
       :precondition (and  (room ?from) (room ?to) (at-robby ?from))
       :effect (and  (at-robby ?to)
             (not (at-robby ?from))))



   (:action pick
       :parameters (?obj ?room ?gripper)
       :precondition  (and  (ball ?obj) (room ?room) (gripper ?gripper)
                (at ?obj ?room) (at-robby ?room) (free ?gripper))
       :effect (and (carry ?obj ?gripper)
            (not (at ?obj ?room))
            (not (free ?gripper))))


   (:action drop
       :parameters  (?obj  ?room ?gripper)
       :precondition  (and  (ball ?obj) (room ?room) (gripper ?gripper)
                (carry ?obj ?gripper) (at-robby ?room))
       :effect (and (at ?obj ?room)
            (free ?gripper)
            (not (carry ?obj ?gripper))))

  (:action macro0000
    :parameters (?var0000 ?var0001 ?var0002 ?var0003 ?var0004)
    :precondition (and (at ?var0004 ?var0003)
      (at-robby ?var0001)
      (ball ?var0000)
      (ball ?var0004)
      (carry ?var0000 ?var0002)
      (gripper ?var0002)
      (room ?var0001)
      (room ?var0003))
    :effect (and
      (not (at ?var0004 ?var0003))
      (at ?var0004 ?var0001))
  )

  (:action macro0001
    :parameters (?var0000 ?var0001 ?var0002 ?var0003)
    :precondition (and (at-robby ?var0000)
      (ball ?var0002)
      (carry ?var0002 ?var0003)
      (gripper ?var0003)
      (room ?var0000)
      (room ?var0001))
    :effect (and
      (not (carry ?var0002 ?var0003))
      (at ?var0002 ?var0001)
      (free ?var0003))
  )

  (:action macro0002
    :parameters (?var0000 ?var0001 ?var0002 ?var0003 ?var0004)
    :precondition (and (at ?var0004 ?var0003)
      (at-robby ?var0001)
      (ball ?var0000)
      (ball ?var0004)
      (carry ?var0000 ?var0002)
      (gripper ?var0002)
      (room ?var0001)
      (room ?var0003))
    :effect (and
      (not (at ?var0004 ?var0003))
      (not (carry ?var0000 ?var0002))
      (at ?var0000 ?var0001)
      (carry ?var0004 ?var0002))
  )

  (:action macro0003
    :parameters (?var0000 ?var0001 ?var0002 ?var0003)
    :precondition (and (at ?var0003 ?var0001)
      (at-robby ?var0001)
      (ball ?var0000)
      (ball ?var0003)
      (carry ?var0000 ?var0002)
      (gripper ?var0002)
      (room ?var0001))
    :effect (and
      (not (at ?var0003 ?var0001))
      (not (carry ?var0000 ?var0002))
      (at ?var0000 ?var0001)
      (carry ?var0003 ?var0002))
  )

  (:action macro0004
    :parameters (?var0000 ?var0001 ?var0002 ?var0003 ?var0004)
    :precondition (and (at-robby ?var0001)
      (ball ?var0000)
      (ball ?var0003)
      (carry ?var0000 ?var0002)
      (carry ?var0003 ?var0004)
      (gripper ?var0002)
      (gripper ?var0004)
      (room ?var0001))
    :effect (and
      (not (carry ?var0000 ?var0002))
      (not (carry ?var0003 ?var0004))
      (carry ?var0000 ?var0004)
      (carry ?var0003 ?var0002))
  )

  (:action macro0005
    :parameters (?var0000 ?var0001 ?var0002 ?var0003 ?var0004)
    :precondition (and (at ?var0004 ?var0000)
      (at-robby ?var0000)
      (ball ?var0002)
      (ball ?var0004)
      (carry ?var0002 ?var0003)
      (gripper ?var0003)
      (room ?var0000)
      (room ?var0001))
    :effect (and
      (not (at ?var0004 ?var0000))
      (not (carry ?var0002 ?var0003))
      (at ?var0002 ?var0001)
      (carry ?var0004 ?var0003))
  )

  (:action macro0006
    :parameters (?var0000 ?var0001 ?var0002 ?var0003 ?var0004)
    :precondition (and (at ?var0004 ?var0001)
      (at-robby ?var0000)
      (ball ?var0002)
      (ball ?var0004)
      (carry ?var0002 ?var0003)
      (gripper ?var0003)
      (room ?var0000)
      (room ?var0001))
    :effect (and
      (not (at ?var0004 ?var0001))
      (not (carry ?var0002 ?var0003))
      (at ?var0002 ?var0001)
      (carry ?var0004 ?var0003))
  )

  (:action macro0007
    :parameters (?var0000 ?var0001 ?var0002 ?var0003)
    :precondition (and (at-robby ?var0001)
      (ball ?var0000)
      (carry ?var0000 ?var0002)
      (gripper ?var0002)
      (room ?var0001)
      (room ?var0003))
    :effect (and
      (not (at-robby ?var0001))
      (not (carry ?var0000 ?var0002))
      (at ?var0000 ?var0001)
      (at-robby ?var0003)
      (free ?var0002))
  )
)
