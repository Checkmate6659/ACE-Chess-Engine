# ACE-Chess-Engine

ACE is a chess engine still in development.
It has a lot of pruning:

-Alpha-Beta pruning
-Null-move pruning, with R >= 3
-Futility pruning, with a margin of 350cp
-Extended futility pruning, with a margin of 550cp
-Limited razoring, reduces the search depth by 1 ply, margin of 920cp
-Late move reduction, for the later half of the moves at root
-Delta pruning, with a margin of 250cp

It has 2 killer slots/ply, and history heuristic as well, to speed up move ordering.

Bugs:
The transposition table (still in development) doesn't work
The displayed PV line is incorrect, since I started implementing the transposition table
