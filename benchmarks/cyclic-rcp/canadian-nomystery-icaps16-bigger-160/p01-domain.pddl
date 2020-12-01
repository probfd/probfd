(define (domain canadian-transport-l8-e12-t1-p4--minP200--maxP200--s228388) (:requirements :typing :probabilistic-effects) (:types location package road status fuel) (:predicates (at ?l - location) (p-at ?p - package ?l - location) (road-status ?r - road ?s - status) (trunk ?p - package) (available_fuel ?f - fuel) (diff ?f0 ?f1 ?f2 - fuel)) (:constants l0 l1 l2 l3 l4 l5 l6 l7 - location p0 p1 p2 p3 - package r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 - road unknown clear blocked - status f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17 f18 f19 f20 f21 f22 f23 f24 f25 f26 f27 f28 f29 f30 f31 f32 f33 f34 f35 f36 f37 f38 f39 f40 f41 f42 f43 f44 f45 f46 f47 f48 f49 f50 f51 f52 f53 f54 f55 f56 f57 f58 f59 f60 f61 f62 f63 f64 f65 f66 f67 f68 f69 f70 f71 f72 f73 f74 f75 f76 f77 f78 f79 f80 f81 f82 f83 f84 f85 f86 f87 f88 f89 f90 f91 f92 f93 f94 f95 f96 f97 f98 f99 f100 f101 f102 f103 f104 f105 f106 f107 f108 f109 f110 f111 f112 - fuel) (:functions (total-cost)) (:action load :parameters (?p - package ?l - location) :precondition (and (at ?l) (p-at ?p ?l)) :effect (and (increase (total-cost) 1) (trunk ?p) (not (p-at ?p ?l)))) (:action unload :parameters (?p - package ?l - location) :precondition (and (at ?l) (trunk ?p)) :effect (and (increase (total-cost) 1) (not (trunk ?p)) (p-at ?p ?l))) (:action try-driving-l0-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l0) (road-status r0 unknown) (available_fuel ?oldf) (diff ?oldf f16 ?newf)) :effect (and (increase (total-cost) 16) (not (road-status r0 unknown)) (probabilistic 200/1000 (and (road-status r0 blocked)) 800/1000 (and (road-status r0 clear) (at l4) (not (at l0)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l0-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l0) (road-status r0 clear) (available_fuel ?oldf) (diff ?oldf f16 ?newf)) :effect (and (increase (total-cost) 16) (at l4) (not (at l0)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l4-l0 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r0 unknown) (available_fuel ?oldf) (diff ?oldf f16 ?newf)) :effect (and (increase (total-cost) 16) (not (road-status r0 unknown)) (probabilistic 200/1000 (and (road-status r0 blocked)) 800/1000 (and (road-status r0 clear) (at l0) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l4-l0 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r0 clear) (available_fuel ?oldf) (diff ?oldf f16 ?newf)) :effect (and (increase (total-cost) 16) (at l0) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l0-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l0) (road-status r1 unknown) (available_fuel ?oldf) (diff ?oldf f21 ?newf)) :effect (and (increase (total-cost) 21) (not (road-status r1 unknown)) (probabilistic 200/1000 (and (road-status r1 blocked)) 800/1000 (and (road-status r1 clear) (at l5) (not (at l0)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l0-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l0) (road-status r1 clear) (available_fuel ?oldf) (diff ?oldf f21 ?newf)) :effect (and (increase (total-cost) 21) (at l5) (not (at l0)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l5-l0 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r1 unknown) (available_fuel ?oldf) (diff ?oldf f21 ?newf)) :effect (and (increase (total-cost) 21) (not (road-status r1 unknown)) (probabilistic 200/1000 (and (road-status r1 blocked)) 800/1000 (and (road-status r1 clear) (at l0) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l5-l0 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r1 clear) (available_fuel ?oldf) (diff ?oldf f21 ?newf)) :effect (and (increase (total-cost) 21) (at l0) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l0-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l0) (road-status r2 unknown) (available_fuel ?oldf) (diff ?oldf f15 ?newf)) :effect (and (increase (total-cost) 15) (not (road-status r2 unknown)) (probabilistic 200/1000 (and (road-status r2 blocked)) 800/1000 (and (road-status r2 clear) (at l6) (not (at l0)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l0-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l0) (road-status r2 clear) (available_fuel ?oldf) (diff ?oldf f15 ?newf)) :effect (and (increase (total-cost) 15) (at l6) (not (at l0)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l6-l0 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r2 unknown) (available_fuel ?oldf) (diff ?oldf f15 ?newf)) :effect (and (increase (total-cost) 15) (not (road-status r2 unknown)) (probabilistic 200/1000 (and (road-status r2 blocked)) 800/1000 (and (road-status r2 clear) (at l0) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l6-l0 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r2 clear) (available_fuel ?oldf) (diff ?oldf f15 ?newf)) :effect (and (increase (total-cost) 15) (at l0) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l1-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l1) (road-status r3 unknown) (available_fuel ?oldf) (diff ?oldf f5 ?newf)) :effect (and (increase (total-cost) 5) (not (road-status r3 unknown)) (probabilistic 200/1000 (and (road-status r3 blocked)) 800/1000 (and (road-status r3 clear) (at l4) (not (at l1)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l1-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l1) (road-status r3 clear) (available_fuel ?oldf) (diff ?oldf f5 ?newf)) :effect (and (increase (total-cost) 5) (at l4) (not (at l1)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l4-l1 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r3 unknown) (available_fuel ?oldf) (diff ?oldf f5 ?newf)) :effect (and (increase (total-cost) 5) (not (road-status r3 unknown)) (probabilistic 200/1000 (and (road-status r3 blocked)) 800/1000 (and (road-status r3 clear) (at l1) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l4-l1 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r3 clear) (available_fuel ?oldf) (diff ?oldf f5 ?newf)) :effect (and (increase (total-cost) 5) (at l1) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l1-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l1) (road-status r4 unknown) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (not (road-status r4 unknown)) (probabilistic 200/1000 (and (road-status r4 blocked)) 800/1000 (and (road-status r4 clear) (at l6) (not (at l1)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l1-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l1) (road-status r4 clear) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (at l6) (not (at l1)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l6-l1 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r4 unknown) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (not (road-status r4 unknown)) (probabilistic 200/1000 (and (road-status r4 blocked)) 800/1000 (and (road-status r4 clear) (at l1) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l6-l1 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r4 clear) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (at l1) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l2-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l2) (road-status r5 unknown) (available_fuel ?oldf) (diff ?oldf f22 ?newf)) :effect (and (increase (total-cost) 22) (not (road-status r5 unknown)) (probabilistic 200/1000 (and (road-status r5 blocked)) 800/1000 (and (road-status r5 clear) (at l5) (not (at l2)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l2-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l2) (road-status r5 clear) (available_fuel ?oldf) (diff ?oldf f22 ?newf)) :effect (and (increase (total-cost) 22) (at l5) (not (at l2)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l5-l2 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r5 unknown) (available_fuel ?oldf) (diff ?oldf f22 ?newf)) :effect (and (increase (total-cost) 22) (not (road-status r5 unknown)) (probabilistic 200/1000 (and (road-status r5 blocked)) 800/1000 (and (road-status r5 clear) (at l2) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l5-l2 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r5 clear) (available_fuel ?oldf) (diff ?oldf f22 ?newf)) :effect (and (increase (total-cost) 22) (at l2) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l2-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l2) (road-status r6 unknown) (available_fuel ?oldf) (diff ?oldf f1 ?newf)) :effect (and (increase (total-cost) 1) (not (road-status r6 unknown)) (probabilistic 200/1000 (and (road-status r6 blocked)) 800/1000 (and (road-status r6 clear) (at l6) (not (at l2)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l2-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l2) (road-status r6 clear) (available_fuel ?oldf) (diff ?oldf f1 ?newf)) :effect (and (increase (total-cost) 1) (at l6) (not (at l2)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l6-l2 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r6 unknown) (available_fuel ?oldf) (diff ?oldf f1 ?newf)) :effect (and (increase (total-cost) 1) (not (road-status r6 unknown)) (probabilistic 200/1000 (and (road-status r6 blocked)) 800/1000 (and (road-status r6 clear) (at l2) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l6-l2 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r6 clear) (available_fuel ?oldf) (diff ?oldf f1 ?newf)) :effect (and (increase (total-cost) 1) (at l2) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l3-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l3) (road-status r7 unknown) (available_fuel ?oldf) (diff ?oldf f3 ?newf)) :effect (and (increase (total-cost) 3) (not (road-status r7 unknown)) (probabilistic 200/1000 (and (road-status r7 blocked)) 800/1000 (and (road-status r7 clear) (at l5) (not (at l3)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l3-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l3) (road-status r7 clear) (available_fuel ?oldf) (diff ?oldf f3 ?newf)) :effect (and (increase (total-cost) 3) (at l5) (not (at l3)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l5-l3 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r7 unknown) (available_fuel ?oldf) (diff ?oldf f3 ?newf)) :effect (and (increase (total-cost) 3) (not (road-status r7 unknown)) (probabilistic 200/1000 (and (road-status r7 blocked)) 800/1000 (and (road-status r7 clear) (at l3) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l5-l3 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r7 clear) (available_fuel ?oldf) (diff ?oldf f3 ?newf)) :effect (and (increase (total-cost) 3) (at l3) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l3-l7 :parameters (?oldf ?newf - fuel) :precondition (and (at l3) (road-status r8 unknown) (available_fuel ?oldf) (diff ?oldf f10 ?newf)) :effect (and (increase (total-cost) 10) (not (road-status r8 unknown)) (probabilistic 200/1000 (and (road-status r8 blocked)) 800/1000 (and (road-status r8 clear) (at l7) (not (at l3)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l3-l7 :parameters (?oldf ?newf - fuel) :precondition (and (at l3) (road-status r8 clear) (available_fuel ?oldf) (diff ?oldf f10 ?newf)) :effect (and (increase (total-cost) 10) (at l7) (not (at l3)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l7-l3 :parameters (?oldf ?newf - fuel) :precondition (and (at l7) (road-status r8 unknown) (available_fuel ?oldf) (diff ?oldf f10 ?newf)) :effect (and (increase (total-cost) 10) (not (road-status r8 unknown)) (probabilistic 200/1000 (and (road-status r8 blocked)) 800/1000 (and (road-status r8 clear) (at l3) (not (at l7)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l7-l3 :parameters (?oldf ?newf - fuel) :precondition (and (at l7) (road-status r8 clear) (available_fuel ?oldf) (diff ?oldf f10 ?newf)) :effect (and (increase (total-cost) 10) (at l3) (not (at l7)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l4-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r9 unknown) (available_fuel ?oldf) (diff ?oldf f14 ?newf)) :effect (and (increase (total-cost) 14) (not (road-status r9 unknown)) (probabilistic 200/1000 (and (road-status r9 blocked)) 800/1000 (and (road-status r9 clear) (at l5) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l4-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r9 clear) (available_fuel ?oldf) (diff ?oldf f14 ?newf)) :effect (and (increase (total-cost) 14) (at l5) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l5-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r9 unknown) (available_fuel ?oldf) (diff ?oldf f14 ?newf)) :effect (and (increase (total-cost) 14) (not (road-status r9 unknown)) (probabilistic 200/1000 (and (road-status r9 blocked)) 800/1000 (and (road-status r9 clear) (at l4) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l5-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r9 clear) (available_fuel ?oldf) (diff ?oldf f14 ?newf)) :effect (and (increase (total-cost) 14) (at l4) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l4-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r10 unknown) (available_fuel ?oldf) (diff ?oldf f9 ?newf)) :effect (and (increase (total-cost) 9) (not (road-status r10 unknown)) (probabilistic 200/1000 (and (road-status r10 blocked)) 800/1000 (and (road-status r10 clear) (at l6) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l4-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l4) (road-status r10 clear) (available_fuel ?oldf) (diff ?oldf f9 ?newf)) :effect (and (increase (total-cost) 9) (at l6) (not (at l4)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l6-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r10 unknown) (available_fuel ?oldf) (diff ?oldf f9 ?newf)) :effect (and (increase (total-cost) 9) (not (road-status r10 unknown)) (probabilistic 200/1000 (and (road-status r10 blocked)) 800/1000 (and (road-status r10 clear) (at l4) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l6-l4 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r10 clear) (available_fuel ?oldf) (diff ?oldf f9 ?newf)) :effect (and (increase (total-cost) 9) (at l4) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l5-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r11 unknown) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (not (road-status r11 unknown)) (probabilistic 200/1000 (and (road-status r11 blocked)) 800/1000 (and (road-status r11 clear) (at l6) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l5-l6 :parameters (?oldf ?newf - fuel) :precondition (and (at l5) (road-status r11 clear) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (at l6) (not (at l5)) (not (available_fuel ?oldf)) (available_fuel ?newf))) (:action try-driving-l6-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r11 unknown) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (not (road-status r11 unknown)) (probabilistic 200/1000 (and (road-status r11 blocked)) 800/1000 (and (road-status r11 clear) (at l5) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))))) (:action drive-l6-l5 :parameters (?oldf ?newf - fuel) :precondition (and (at l6) (road-status r11 clear) (available_fuel ?oldf) (diff ?oldf f20 ?newf)) :effect (and (increase (total-cost) 20) (at l5) (not (at l6)) (not (available_fuel ?oldf)) (available_fuel ?newf))))