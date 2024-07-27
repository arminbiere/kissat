Version 4.0.0
-------------

  - source code matches competition version 'sc2024'
  - fast variable elimination during preprocessing (in `fastel.c`)
  - lucky phases as in `CaDiCaL` but before and after preprocessing
    and with unit extraction and SLURM semantics
  - reason jumping only for formulas with large binary clauses fraction
  - U-shaped delta scaling of probing and elimination interval 
  - option `-o <output>` to write simplified formula to a file
  - dynamically increased reduced-clauses fraction (60% - 90%)
  - bounded variable addition (in `factor.c`)
  - clausal congruence closure algorithm (in `congruence.c`)
  - generic preprocessing phase (using a subset-set of simplifiers)
  - more vivification (tier0=irredundant,tier1,tier2,tier3)
  - added `--no-conflicts` as synonym to `--conflicts=0`
  - optimized and simplified vivification

Version 3.1.1
-------------

  - configuration option `--safe` disables writing through `popen`

Version 3.1.0
-------------

  - replaced `__gcov_flush` by `__gcov_dump`
  - moved to `clang-format` instead of `indent`
  - removed warnings for MacOS (latest clang)
  - added back irredundant clause vivification
  - reason jumping during binary reason clause chains
  - more focused sweeping (more effort and unscheduling)
  - probing and elimination preprocessing initially
  - relevancy and occurrence score based variable elimination
  - faster parsing and proof writing
  - no redundant virtual clauses anymore
  - doubled allowed maximum variable index to more than 1 billion variables

Version 3.0.0
-------------

Removed from sc2022-light two heuristic bugs and went back to monotonically
increasing version numbers (see also [`VERSION`](VERSION)).

  - avoid increasing number of conflicts during vivification
  - do set used flags (two bits) of learned clauses initially

Thus we follow our plan to continue extending and maintaining 
this light version of Kissat on sc2022-light.

Version sc2022-light
--------------------

We started with version sc2022-bulky and then removed code which only gave a
minor improvement on the last three SAT Competition 2019-2021.  The goal was
to shrink the code base in order to concentrate on the really useful
algorithms and heuristics and thus prepare for extending the solver.

This is the list of removed features:

  - autarky reasoning
  - eager forward and backward subsumption during variable elimination
    (relying on global forward subsumption instead)
  - caching and reusing of minimum assignments during local search
  - failed literal probing
  - hyper binary resolution
  - hyper ternary resolution
  - transitive reduction of the binary implication graph
  - eager subsumption of recently learned clauses during clause learning
  - xor gate extraction during variable elimination
  - alternative radix heap implementation for advanced shrinking
  - priority queue for variable elimination (elimination attempts of
    variables in the given variable order is now enforced)
  - delaying of inprocessing functions based on formula size (initially
    'really' and if not successful 'delay')
  - reusing the trail during restarts
  - vivification of irredundant clauses
  - forced to keep untried elimination, backbone and vivification candidates
    for next inprocessing round (removed options to disable keeping them)
  - initial focused mode phase limited by conflicts only (not ticks anymore)

Removing hyper binary resolution provided us with one more bit for encoding
literals as virtual binary clauses kept only in watch lists do not have to
be distinguish between being derived through hyper binary resolution or
learned through conflict analysis.  The former were often generated in huge
amounts during failed-literal probing and had to be deleted during clause
data-base reduction eagerly to avoid cloaking up the memory.  As a
consequence the solver now supports half a billion ('2^29-1') instead of a
quarter billion ('2^28-1') variables in versions with hyper resolution.

Version sc2022-hyper
--------------------

This version can be seen as an intermediate version between sc2022-bulky and
sc2022-light but actually evolved from an earlier version sc2022-light by
adding back support for hyper binary resolution. This feature turns out to
be somewhat useful for unsatisfiable formulas even though the improved SAT
sweeping in sc2022-bulky and kept for sc2022-light covers up for some of the
losses incurred by taking out hyper-binary resolution.

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
