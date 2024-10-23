
(define (domain miconic)
  (:requirements :typing )
  (:types passenger floor)
  (:predicates (not-boarded ?v0 - passenger)
	(down ?v0 - floor)
	(boarded ?v0 - passenger)
	(depart ?v0 - floor ?v1 - passenger)
	(not-served ?v0 - passenger)
	(origin ?v0 - passenger ?v1 - floor)
	(board ?v0 - floor ?v1 - passenger)
	(lift-at ?v0 - floor)
	(served ?v0 - passenger)
	(destin ?v0 - passenger ?v1 - floor)
	(up ?v0 - floor)
	(above ?v0 - floor ?v1 - floor)
  )




	(:action do_down
		:parameters (?f1 - floor ?f2 - floor)
		:precondition (and (above ?f2 ?f1)
			(down ?f2)
			(lift-at ?f1))
		:effect (and
			(lift-at ?f2)
			(not (lift-at ?f1)))
	)


	(:action do_board
		:parameters (?f - floor ?p - passenger)
		:precondition (and (board ?f ?p)
			(lift-at ?f)
			(origin ?p ?f))
		:effect (and
			(boarded ?p))
	)


	(:action do_up
		:parameters (?f1 - floor ?f2 - floor)
		:precondition (and (above ?f1 ?f2)
			(lift-at ?f1)
			(up ?f2))
		:effect (and
			(lift-at ?f2)
			(not (lift-at ?f1)))
	)


	(:action do_depart
		:parameters (?f - floor ?p - passenger)
		:precondition (and (boarded ?p)
			(depart ?f ?p)
			(destin ?p ?f)
			(lift-at ?f))
		:effect (and
			(not (boarded ?p))
			(served ?p))
	)



  (:action macro0000
    :parameters (?var0000 - floor ?var0001 - passenger ?var0002 - floor)
    :precondition (and (above ?var0000 ?var0002)
      (board ?var0000 ?var0001)
      (depart ?var0002 ?var0001)
      (destin ?var0001 ?var0002)
      (down ?var0000)
      (lift-at ?var0000)
      (origin ?var0001 ?var0000)
      (up ?var0002))
    :effect (and
      (served ?var0001))
  )

  (:action macro0001
    :parameters (?var0000 - floor ?var0001 - passenger ?var0002 - floor)
    :precondition (and (above ?var0002 ?var0000)
      (board ?var0002 ?var0001)
      (boarded ?var0001)
      (depart ?var0000 ?var0001)
      (destin ?var0001 ?var0000)
      (down ?var0002)
      (lift-at ?var0000)
      (origin ?var0001 ?var0002)
      (up ?var0000))
    :effect (and
      (served ?var0001))
  )

  (:action macro0002
    :parameters (?var0000 - floor ?var0001 - floor ?var0002 - passenger)
    :precondition (and (above ?var0001 ?var0000)
      (board ?var0001 ?var0002)
      (down ?var0001)
      (lift-at ?var0000)
      (origin ?var0002 ?var0001)
      (up ?var0000))
    :effect (and
      (boarded ?var0002))
  )

  (:action macro0003
    :parameters (?var0000 - floor ?var0001 - floor ?var0002 - passenger)
    :precondition (and (above ?var0001 ?var0000)
      (boarded ?var0002)
      (depart ?var0001 ?var0002)
      (destin ?var0002 ?var0001)
      (down ?var0001)
      (lift-at ?var0000)
      (up ?var0000))
    :effect (and
      (not (boarded ?var0002))
      (served ?var0002))
  )

  (:action macro0004
    :parameters (?var0000 - floor ?var0001 - floor ?var0002 - passenger)
    :precondition (and (above ?var0000 ?var0001)
      (board ?var0001 ?var0002)
      (down ?var0000)
      (lift-at ?var0000)
      (origin ?var0002 ?var0001)
      (up ?var0001))
    :effect (and
      (boarded ?var0002))
  )

  (:action macro0005
    :parameters (?var0000 - floor ?var0001 - passenger ?var0002 - floor)
    :precondition (and (above ?var0000 ?var0002)
      (board ?var0000 ?var0001)
      (depart ?var0002 ?var0001)
      (destin ?var0001 ?var0002)
      (down ?var0000)
      (lift-at ?var0000)
      (origin ?var0001 ?var0000)
      (up ?var0002))
    :effect (and
      (boarded ?var0001)
      (served ?var0001))
  )

  (:action macro0006
    :parameters (?var0000 - floor ?var0001 - floor ?var0002 - passenger ?var0003 - passenger)
    :precondition (and (above ?var0001 ?var0000)
      (board ?var0000 ?var0003)
      (board ?var0001 ?var0002)
      (down ?var0001)
      (lift-at ?var0000)
      (origin ?var0002 ?var0001)
      (origin ?var0003 ?var0000)
      (up ?var0000))
    :effect (and
      (boarded ?var0002)
      (boarded ?var0003))
  )

  (:action macro0007
    :parameters (?var0000 - floor ?var0001 - floor ?var0002 - passenger ?var0003 - passenger)
    :precondition (and (above ?var0001 ?var0000)
      (board ?var0000 ?var0003)
      (boarded ?var0002)
      (depart ?var0001 ?var0002)
      (destin ?var0002 ?var0001)
      (down ?var0001)
      (lift-at ?var0000)
      (origin ?var0003 ?var0000)
      (up ?var0000))
    :effect (and
      (not (boarded ?var0002))
      (boarded ?var0003)
      (served ?var0002))
  )
)
