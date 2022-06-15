Version sc2022-bulky
--------------------

Compared to version 2.0.2 and thus the version submitted to the competition
in 2021 we added the following features:

  - added ACIDS branching variable heuristics (disabled by default)
  - added CHB variable branching heuristic (but disabled by default)
    inspired by the success of 'kissat_mab' in the SAT Competition 2021
  - faster randomization of phases in the Kitten sub-solver
  - using literal flipping for faster refinement during sweeping
  - literal flipping in a partial model obtained by Kitten
  - disabled priority queue for variable elimination (elimination attempts
    of variables in the given variable order is now enforced)
  - disabled by default reusing the trail during restarts
  - disabled by default hyper ternary resolution
  - obtain initial local search assignment through propagation
    (similar to the "warming-up" idea of Donald Knuth and how Shaowei Cai
     for "ReasonLS" solvers initializes the local search)
  - actual watch replacement of true literals during unit propagation instead
    of just updating the blocking literal (as suggested by Norbert Manthey)
  - fixed clause length and variable occurrences during variable elimination
    instead of increasing the limits dynamically (but very rarely actually)


Version 2.0.2
-------------

- removed 'src/makefile' from git (set during configuration instead)
