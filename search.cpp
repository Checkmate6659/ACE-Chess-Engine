#include "search.h"
#include "uci.h"


uint64_t evalNodeCount = 0;

uint64_t perft(int depth) //basic perft function for debugging
{
	uint64_t nodesSearched = 0;
	MoveList legalMoves = gen_moves();
	if (depth == 1)
	{
		return legalMoves.count; //output number of legal moves, faster, but doesn't reflect true NPS (good for debugging at high depth)
	}

	if (depth == 0)
	{
		return 1; //if no depth left, return 1 found position, avoid infinite recursion for depth=0
	}

	for (int i = 0; i < legalMoves.count; i++) //loop through legal moves
	{
		make_move(legalMoves.moves[i]); //recursive perft implementation: make move, do perft, and undo
		nodesSearched += perft(depth - 1);
		undo_move();
	}

	return nodesSearched;
}

int64_t getEvaluation()
{
	evalNodeCount++;
	return (1 + (turn * -2))*evaluate(); //do this so that it always returns eval for white at root, even with varying search depth parity
}

int64_t negamax(unsigned depth, int64_t alpha, int64_t beta, bool nmhAuth, Variation* variation, uint8_t curHMC)
{
	variation->count = 0; //just in case no variation is "returned", do not give bogus ones

	if ((evalNodeCount & 4095) == 0)
	{
		if (clock() > endt) return 0; //do not continue searching if exceeded maximum time (check for every 4096 nodes)
		if((evalNodeCount & 8191) == 0)
		{
			if (input_available()) return 0; //also stop if input available (stop signal)
		}
	}

	if (depth)
	{
		uint32_t ttIndex;

		if (probeTT(&ttIndex, zhash[ply], depth)) //probe TT (BUG: it always returns 0 on lasker reichhelm, error with depth!!!)
		{
			ttIndex %= hashSize;
			//std::cout << "found match" << ttIndex << std::endl;
			variation->count = 0; //omit PV, even at low depth, to gain TT space, because a single variation takes up 1KB
			//WARNING: idk why but the PV at the root will have a count of 255 and generates bogus ponders

			if (tt[ttIndex].flag == TT_EXACT)
			{
				return tt[ttIndex].eval;
			}
			if (tt[ttIndex].flag == TT_ALPHA && (tt[ttIndex].eval <= alpha))
			{
				return alpha;
			}
			if (tt[ttIndex].flag == TT_BETA && (tt[ttIndex].eval >= beta))
			{
				return beta;
			}
		}

		Variation currentV;

#ifdef ENABLE_NMH
		if (depth > NMH_ACTIVATE && nmhAuth && !isEndgame()) //NMH, disabled in boolean endgame
		{
			uint64_t kingMask = king_occ[ply] & occupancy[ply] & (turn ? black_occ : white_occ)[ply];
			if (!mask_atk(kingMask, !turn)) //can perform NMH, not in check
			{
				turn = !turn; //make null move
				ply++; //we have to change the ply, to account for killers, otherwise the colors will be exchanged
				int64_t score = -negamax(depth - NMH_SD_REDUCTION - 1, -beta, -beta+1, false, new Variation, curHMC);
				turn = !turn; //unmake null move
				ply--;

				if (score >= beta)
				{
					return beta;
				}
			}
		}
#endif

		MoveList legalMoves = gen_moves();

#ifdef ENABLE_LRAZOR
		int64_t score = getEvaluation() + LRAZOR_MARGIN;
		if (depth == PRE_PRE_FRONTIER && score <= alpha && !isEndgame())
		{
			depth = PRE_FRONTIER;
		}
#endif

		if (legalMoves.count && curHMC)
		{
			orderMoves(&legalMoves);

			uint8_t nextHMC = curHMC;
			int64_t evaluation;
			for (uint8_t i = 0; i < legalMoves.count; i++) //loop through legal moves
			{
#ifdef ENABLE_FP
				if (depth == FRONTIER && getEvaluation() + FUTILITY_MARGIN <= alpha && legalMoves.moves[i].flags < CHK_F) continue;
#endif
#ifdef ENABLE_EFP
				if (depth == PRE_FRONTIER && getEvaluation() + EXT_FUTILITY_MARGIN <= alpha && legalMoves.moves[i].flags < CHK_F) continue;
#endif
				make_move(legalMoves.moves[i]); //recursive negamax implementation
				if (legalMoves.moves[i].flags & CAPT_F && isInsufficientMaterial()) //if capture, then check for material insufficiency
				{
					undo_move();
					evalNodeCount++;

					//store node in TT
					/*tt[ttIndex].key = zhash[ply];
					tt[ttIndex].depth = depth; //the game has ended, so this cannot be improved anymore (MAX_SD)
					tt[ttIndex].eval = DRAW;
					tt[ttIndex].flag = TT_EXACT;*/
					return DRAW;
				}
				for (uint8_t i = ply; i--;) //check for single repetition
				{
					if (zhash[ply] == zhash[i])
					{
						undo_move();
						evalNodeCount++;

						//store node in TT
						/*tt[ttIndex].key = zhash[ply];
						tt[ttIndex].depth = depth;
						tt[ttIndex].eval = DRAW;
						tt[ttIndex].flag = TT_EXACT;*/
						return DRAW;
					}
				}

				if ((legalMoves.moves[i].flags >= CAPT_F) || (getPieceKind(legalMoves.moves[i].dst) <= Z_WP)) nextHMC = 100; //reset HMC if capture/pawn move
				evaluation = -negamax(depth - 1, -beta - ASPI_WINDOW_WIDTH, -alpha + ASPI_WINDOW_WIDTH, true, &currentV, nextHMC); //search with aspiration window

				nextHMC = curHMC;
				undo_move();
				if (evaluation >= beta)
				{
					if (legalMoves.moves[i].flags < CAPT_F) //killer move: quiet (less loud than a capture, so check/double check is included, even though double checks are searched first) and caused beta cutoff
					{
						//if move not already a primary killer (avoid duplicate killers that can make search less efficient, because of the presence of a single duplicate killer move)
						if ((legalMoves.moves[i].src != primaryKillers[ply].src) || (legalMoves.moves[i].dst != primaryKillers[ply].dst))
						{
							secondaryKillers[ply] = primaryKillers[ply]; //update killers
							primaryKillers[ply] = legalMoves.moves[i];
						}
					}

					//store node in TT
					/*tt[ttIndex].key = zhash[ply];
					tt[ttIndex].depth = depth-1;
					tt[ttIndex].eval = beta; //error?
					tt[ttIndex].flag = TT_BETA;*/
					storeTTEntry(zhash[ply+1], depth-1, beta, TT_BETA); //you have to use ply+1 to store previously made/undone move. This is usable as the resulting position's data (like the hash) is not yet overwritten
					return beta;
				}
				if (alpha < evaluation)
				{
					alpha = evaluation;
					if (legalMoves.moves[i].flags < CAPT_F) //history move
					{
						history[getPieceKind(legalMoves.moves[i].dst)][legalMoves.moves[i].dst] += depth * depth; //update score
					}
					*variation = currentV;
					variation->moves[variation->count] = legalMoves.moves[i];
					variation->count++;

					//store node in TT
					/*tt[ttIndex].key = zhash[ply];
					tt[ttIndex].depth = depth;
					tt[ttIndex].eval = alpha;
					tt[ttIndex].flag = TT_EXACT;*/

					storeTTEntry(zhash[ply + 1], depth, alpha, TT_EXACT); //store it with hash of ply+1 for same reasons as explained above
				}
				else {
					//store node in TT
					/*tt[ttIndex].key = zhash[ply];
					tt[ttIndex].depth = depth;
					tt[ttIndex].eval = alpha;
					tt[ttIndex].flag = TT_ALPHA;*/

					storeTTEntry(zhash[ply + 1], depth, alpha, TT_ALPHA);
				}
			}

			return alpha;
		}
		else //if game ended
		{
			uint64_t kingMask = king_occ[ply] & occupancy[ply] & (turn ? black_occ : white_occ)[ply];
			evalNodeCount++;
			if (mask_atk(kingMask, !turn)) return CHECKMATE + ply; //if check, it's checkmate
			return DRAW;
		}
	}
	else return !isLateEndgame() ? qSearch(alpha, beta) : qSearchEndgame(alpha, beta);
}

int64_t qSearch(int64_t alpha, int64_t beta)
{
	if (ply == MAX_SD - 1) return getEvaluation(); //avoid overflows: cap the search depth; this can be removed, if guaranteed that MAX_SD will never be reached

	int64_t stand_pat = getEvaluation();
	if (stand_pat >= beta) return beta;

	if (stand_pat <= alpha - DELTA) return alpha; //delta pruning

	alpha = max(alpha, stand_pat);

	MoveList loudMoves = gen_loud_moves();

	if (loudMoves.count)
	{
		orderMoves(&loudMoves);

		int64_t evaluation;

		for (uint8_t i = 0; i < loudMoves.count; i++) //loop through legal moves
		{
			make_move(loudMoves.moves[i]); //recursive qsearch implementation

			evaluation = -qSearch(-beta, -alpha);
			undo_move();
			if (evaluation >= beta)
			{
				return beta;
			}
			alpha = max(evaluation, alpha);
		}

		return alpha;
	}
	return stand_pat;
}

int64_t qSearchEndgame(int64_t alpha, int64_t beta)
{
	if (ply == MAX_SD - 1) return getEvaluation(); //avoid overflows: cap the search depth; this can be removed, if guaranteed that MAX_SD will never be reached

	int64_t stand_pat = getEvaluation();
	if (stand_pat >= beta) return beta;

	alpha = max(alpha, stand_pat);

	MoveList loudMoves = gen_loud_moves();

	if (loudMoves.count)
	{
		orderMoves(&loudMoves);

		int64_t evaluation;

		for (uint8_t i = 0; i < loudMoves.count; i++) //loop through legal moves
		{
			make_move(loudMoves.moves[i]); //recursive qsearch implementation
			if (isInsufficientMaterial()) //check for material insufficiency (no capture check required, as most loud moves are captures)
			{
				undo_move();
				return DRAW;
			}

			evaluation = -qSearchEndgame(-beta, -alpha);
			undo_move();
			if (evaluation >= beta)
			{
				return beta;
			}
			alpha = max(evaluation, alpha);
		}

		return alpha;
	}
	return stand_pat;
}

int64_t negamaxRoot(unsigned depth, Move* moveptr, Variation* variation)
{
	depth = min(depth, (unsigned)MAX_SD - 1); //cap depth to avoid overflows

	int64_t bestEval = CHECKMATE;
	uint8_t failLowLMR = false;

	Variation currentV;
	currentV.count = 0;

	MoveList legalMoves = gen_moves();
	orderMoves(&legalMoves);

	int64_t evaluation;
	uint8_t j = 0;

	for (uint8_t i = 0; i < legalMoves.count; i++, j += LMR_INV_LEFT_PROP) //loop through legal moves
	{
		make_move(legalMoves.moves[i]);
		if(legalMoves.moves[i].flags < CHK_F && j >= i) //do not reduce checks and loud moves
		{
			evaluation = -negamax(depth - 1 - LMR_REDUCTION, CHECKMATE, -CHECKMATE, true, &currentV, hmc); //LMR
		}
		else evaluation = -negamax(depth - 1, CHECKMATE, -CHECKMATE, true, &currentV, hmc);
		undo_move();

		if (evaluation > bestEval)
		{
			bestEval = evaluation;
			*moveptr = legalMoves.moves[i];
			*variation = currentV;
			variation->moves[variation->count] = *moveptr;
			variation->count++;
		}
	}

	return bestEval;
}

int64_t negamaxRootIterDeepening(unsigned long allocatedTime, Move* moveptr)
{
	allocatedTime *= 1000 / CLOCKS_PER_SEC;
	clock_t start;
	uint32_t search_time;

	clearKillers();
	clearHistory();

	endt = 2147483647; //manually forbid a cutoff during the 3-ply search

	Move bestMove; //temporary, will be replaced by PV later
	Variation pv;
	int64_t evaluation = negamaxRoot(START_SD, moveptr, &pv); //very quick search time (depth 3 ply), avoid null move bug with insufficient time

	endt = clock() + allocatedTime;
	if (clock() > endt) endt = 2147483647; //if overflow, then "infinite" time

	int64_t prevEval = evaluation;
	unsigned depth = START_SD;
	do
	{
		ttHits = 0; //DEBUG

		depth++;

		start = clock();
		evalNodeCount = 0;
		previousPV = pv;
		prevEval = evaluation;
		bestMove = *moveptr;
		evaluation = negamaxRoot(depth, moveptr, &pv);
		search_time = (clock() - start) * 1000 / CLOCKS_PER_SEC;
		std::cout << "info depth " << depth << " score " << str_eval(evaluation) << " nodes " << evalNodeCount << " time " << search_time << " nps " << 1000 * evalNodeCount / max(search_time, (uint32_t)1);

		if (clock() >= endt || input_available())
		{
			std::cout << std::endl;
			break;
		}

		std::cout << " pv " << str_move(bestMove); //display PV

		pv.count--;
		while (pv.count--)
		{
			std::cout << " ";
			displayMove(pv.moves[pv.count]);
		}
		std::cout << std::endl;

		std::cout << "TT hits: " << ttHits << "\n"; //DEBUG
	} while (clock() < endt && !input_available() && depth < endsd && abs(evaluation) < MATE_DISP_THRESHOLD); //stop search when a cap is reached, or when engine finds mate

	std::cout << "bestmove " << str_move(bestMove);
	/*if(pv.count > 2) std::cout << " ponder " << str_move(pv.moves[pv.count - 2]);
	std::cout << " MOVE COUNT " << (int)pv.count;*/ //somehow move count can be 255 and bogus ponders displayed
	std::cout << std::endl;

	return prevEval;
}
