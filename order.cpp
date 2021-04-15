#include "order.h"


Move primaryKillers[MAX_SD]; //killer moves: quiet moves that cause a beta cutoff
Move secondaryKillers[MAX_SD]; //2 layers of killers
uint32_t history[12][64]; //history moves: indexed by piece kind and destination square
Variation previousPV;

void clearKillers()
{
	for (int i = 0; i < MAX_SD; i++)
	{
		primaryKillers[i].src = 0;
		primaryKillers[i].dst = 0;
		secondaryKillers[i].src = 0;
		secondaryKillers[i].dst = 0;
	}
}

void clearHistory()
{
	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 12; j++)
		{
			history[j][i] = 0;
		}
	}
}

void orderMoves(MoveList* mlist)
{
	uint8_t key, j;
	uint8_t keys[MAX_MOVE] = { getKey(mlist->moves[0]) };
	Move keyMove;
	Move* moves = mlist->moves;

	for (uint8_t i = 1; i < mlist->count; i++)
	{
		keyMove = moves[i];
		key = getKey(keyMove);

		j = i;
		while (j && keys[j - 1] < key)
		{
			keys[j] = keys[j - 1];
			moves[j] = moves[j - 1];

			j--;
		}
		keys[j] = key;
		moves[j] = keyMove;
	}

	return;
}

uint8_t getKey(Move move)
{
	uint8_t key = 0;

	//search PV of previous iteration first (it strangely does nothing, even though it should help a bunch in iterative deepening)
	if ((move.src == previousPV.moves[ply].src) && (move.dst == previousPV.moves[ply].dst)) key += PV_O;

	//idk what these killer heuristics are doing to the eval bruh, its just a move ordering thing! (fixed with history heuristics)
	if ((move.src == primaryKillers[ply].src) && (move.dst == primaryKillers[ply].dst)) return KILLER1_O;
	if ((move.src == secondaryKillers[ply].src) && (move.dst == secondaryKillers[ply].dst)) return KILLER2_O;

	key = move.flags & 0xf0; //only interested in promo/capture/check

	if (move.flags & CAPT_F) //capture: apply MVV-LVA
	{
		if (pawn_occ[ply] & (1ULL << move.dst)) key -= 4;
		else if ((bishop_occ[ply] & knight_occ[ply]) & (1ULL << move.dst)) key -= 3;
		else if (rook_occ[ply] & (1ULL << move.dst)) key -= 2;
		//only queen capture preserves full capture bonus

		if (pawn_occ[ply] & (1ULL << move.src)) key += 4;
		else if ((bishop_occ[ply] & knight_occ[ply]) & (1ULL << move.src)) key += 3;
		else if (rook_occ[ply] & (1ULL << move.src)) key += 2;
		//no bonus for queen/king captures
	}

	if (move.flags & PROMO_F)
	{
		key -= move.promo << 3; //examine promotions in this order: queen (most common), knight (most useful underpromotion), rook (to avoid stalemate, less common) and bishop (least common, almost never quickest win)
	}

	if (move.flags & DCHK_F) //double check
	{
		key |= PROMO_F; //can only be done after handling promo, otherwise the promo code will be executed
	}

	//history heuristic
	key += std::max(history[getPieceKind(move.src)][move.dst], HISTORY_MAX_O);

	return key;
}
