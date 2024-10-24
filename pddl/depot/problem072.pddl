
(define (problem depotprob6512_72) (:domain depot)
  (:objects
        crate0 - crate
	crate1 - crate
	crate2 - crate
	crate3 - crate
	crate4 - crate
	crate5 - crate
	crate6 - crate
	crate7 - crate
	depot0 - depot
	distributor0 - distributor
	distributor1 - distributor
	hoist0 - hoist
	hoist1 - hoist
	hoist2 - hoist
	pallet0 - pallet
	pallet1 - pallet
	pallet2 - pallet
	truck0 - truck
	truck1 - truck
  )
  (:init 
	(at crate0 depot0)
	(at crate1 depot0)
	(at crate2 distributor0)
	(at crate3 distributor1)
	(at crate5 distributor1)
	(at crate6 distributor0)
	(at crate7 distributor1)
	(at hoist0 depot0)
	(at hoist1 distributor0)
	(at hoist2 distributor1)
	(at pallet0 depot0)
	(at pallet1 distributor0)
	(at pallet2 distributor1)
	(at truck0 distributor0)
	(at truck1 depot0)
	(available hoist1)
	(available hoist2)
	(clear crate0)
	(clear crate6)
	(clear crate7)
	(lifting hoist0 crate4)
	(on crate0 crate1)
	(on crate1 pallet0)
	(on crate2 pallet1)
	(on crate3 crate5)
	(on crate5 pallet2)
	(on crate6 crate2)
	(on crate7 crate3)
  )
  (:goal (and
	(on crate0 crate4)
	(on crate2 crate6)
	(on crate4 crate7)
	(on crate5 pallet2)
	(on crate6 pallet1)
	(on crate7 pallet0)))
)