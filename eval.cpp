#include "eval.h"


bool isEndgame()
{
	uint8_t count = 0;

	count += 2 * _mm_popcnt_u64(knight_occ[ply] | bishop_occ[ply]); //2 for each minor piece
	count += 3 * _mm_popcnt_u64(rook_occ[ply]); //3 for each rook
	count += 4 * _mm_popcnt_u64(queen_occ[ply]); //4 for each queen

	return count < 13; //if the count is 12 or less, then it is an endgame
	//Results: 2 queens + 2 minors is endgame, but not 2 queens + 2rooks
	//6 minors are endgame, as well as 4 rooks
	//Any materially unbalanced position, for which this function could be wrong, would be winning for one side, as material is key in most endgames
}

bool isLateEndgame()
{
	return _mm_popcnt_u64(occupancy[ply]) < 6; //if 5 pieces or less, then late endgame
}

bool isInsufficientMaterial()
{
	if (_mm_popcnt_u64(occupancy[ply]) > 4) return false; //if 5 pieces or more, then never insufficient material

	if (_mm_popcnt_u64(occupancy[ply]) < 4) //only 3pcs on board
	{
		if (bishop_occ[ply]) return true; //if minor (knights handled later), then insufficient material
	}
	if (occupancy[ply] == (king_occ[ply] | knight_occ[ply])) return true; //if only knights on board, then draw (only reached at <=4 pieces, so KNK, KNNK or KNKN only)

	return false;
}

int getGamePhase()
{
	int phase = 0; //this number ranges from 0 to 24 in normal situations
	phase += _mm_popcnt_u64(knight_occ[ply] | bishop_occ[ply]);
	phase += 2 * _mm_popcnt_u64(rook_occ[ply]);
	phase += 4 * _mm_popcnt_u64(queen_occ[ply]);

	return std::max(phase, 24)*255/24; //0 = empty board; 255 = initial position; cap the phase to avoid negative parameters/bugs with promotions
}

int64_t evaluate()
{
	int64_t eval = 0;
	int gamePhase = getGamePhase();

	eval += countMaterial();
	eval += computePSqTables(gamePhase);
	eval += calcMaterialAdjustments(gamePhase);
	eval += calcKingSafety() * (255 - gamePhase) >> 8;

	return eval;
}

int64_t countMaterial()
{
	int64_t eval = 0;

	eval += 100 * _mm_popcnt_u64(pawn_occ[ply]);
	eval += 320 * _mm_popcnt_u64(knight_occ[ply]);
	eval += 335 * _mm_popcnt_u64(bishop_occ[ply]);
	eval += 500 * _mm_popcnt_u64(rook_occ[ply]);
	eval += 900 * _mm_popcnt_u64(queen_occ[ply]);

	eval -= 200 * _mm_popcnt_u64(black_occ[ply] & pawn_occ[ply]);
	eval -= 640 * _mm_popcnt_u64(black_occ[ply] & knight_occ[ply]);
	eval -= 670 * _mm_popcnt_u64(black_occ[ply] & bishop_occ[ply]);
	eval -= 1000 * _mm_popcnt_u64(black_occ[ply] & rook_occ[ply]);
	eval -= 1800 * _mm_popcnt_u64(black_occ[ply] & queen_occ[ply]);

	return eval;
}

int64_t computePSqTables(int phase)
{
	//return 0; //temp (this shit is slow af, but we will improve it)

	int64_t eval = 0;

	for (uint8_t i = 0; i<64; i++)
	{
		if ((occupancy[ply] & (1ULL << i)) == 0) continue; //if no piece, continue

		switch (getPieceKind(i))
		{
		case Z_WP:
			eval += PSQ::pawn[IDX_WHITE_PSQ(i)];
			break;
		case Z_BP:
			eval -= PSQ::pawn[IDX_BLACK_PSQ(i)];
			break;
		case Z_WN:
			eval += PSQ::knight[IDX_WHITE_PSQ(i)];
			break;
		case Z_BN:
			eval -= PSQ::knight[IDX_BLACK_PSQ(i)];
			break;
		case Z_WB:
#ifdef ENABLE_BISHOP_TAPER
			eval += ((PSQ::bishop[IDX_WHITE_PSQ(i)] * phase) + (PSQ::bishopEg[IDX_WHITE_PSQ(i)] * (255 - phase))) >> 8;
#else
			eval += PSQ::bishop[IDX_WHITE_PSQ(i)];
#endif
			break;
		case Z_BB:
#ifdef ENABLE_BISHOP_TAPER
			eval -= ((PSQ::bishop[IDX_BLACK_PSQ(i)] * phase) + (PSQ::bishopEg[IDX_BLACK_PSQ(i)] * (255 - phase))) >> 8;
#else
			eval -= PSQ::bishop[IDX_BLACK_PSQ(i)];
#endif
			break;
		case Z_WR:
			eval += (PSQ::rook[IDX_WHITE_PSQ(i)] * phase) >> 8;
			break;
		case Z_BR:
			eval -= (PSQ::rook[IDX_BLACK_PSQ(i)] * phase) >> 8;
			break;
		case Z_WQ:
			eval += PSQ::queen[IDX_WHITE_PSQ(i)];
			break;
		case Z_BQ:
			eval -= PSQ::queen[IDX_BLACK_PSQ(i)];
			break;
		case Z_WK:
			eval += ((PSQ::kingMg[IDX_WHITE_PSQ(i)] * phase) + (PSQ::kingEg[IDX_WHITE_PSQ(i)] * (255 - phase))) >> 8;
			break;
		case Z_BK:
			eval -= ((PSQ::kingMg[IDX_BLACK_PSQ(i)] * phase) + (PSQ::kingEg[IDX_BLACK_PSQ(i)] * (255 - phase))) >> 8;
			break;
		}
	}

	return eval;
}

int64_t calcMaterialAdjustments(int phase)
{
	int64_t eval = 0;

	if (_mm_popcnt_u64(bishop_occ[ply] & white_occ[ply]) > 1) //white bishop pair
	{
		eval += 10 + (255 - phase)*30/255; //+10cp initially, approaches +40cp in endgame
	}

	if (_mm_popcnt_u64(bishop_occ[ply] & black_occ[ply]) > 1) //black bishop pair
	{
		eval -= 10 + (255 - phase) * 30 / 255;
	}

	if (_mm_popcnt_u64(pawn_occ[ply] & CENTER_MASK) > 5) //closed center: 6 or more pawns
	{
		eval += 40 * _mm_popcnt_u64(knight_occ[ply] & white_occ[ply] & CENTER_MASK); //+40cp for each knight in the center
		eval -= 40 * _mm_popcnt_u64(knight_occ[ply] & black_occ[ply] & CENTER_MASK);
		eval -= 20 * _mm_popcnt_u64(knight_occ[ply] & white_occ[ply] & ~DOMAIN_CENTER); //-20cp for each bishop on files A/B/G/H
		eval += 20 * _mm_popcnt_u64(knight_occ[ply] & black_occ[ply] & ~DOMAIN_CENTER);
	}

	return eval;
}

int64_t calcKingSafety()
{
	int64_t eval = 0;

	if (king_occ[ply] & white_occ[ply] & DOMAIN_KCENTER) //white king in center
	{
		eval -= 80; //huge penalty
		if (castle_rights[ply] & KCASTLE_F) eval += 15; //partially compensated by having castle rights
		if (castle_rights[ply] & QCASTLE_F) eval += 15;
	}

	if (king_occ[ply] & black_occ[ply] & DOMAIN_KCENTER) //black king in center
	{
		eval += 80;
		if (castle_rights[ply] & KCASTLE_F << 2) eval -= 15;
		if (castle_rights[ply] & QCASTLE_F << 2) eval -= 15;
	}

	eval += calcPawnShields();

	return eval;
}

int64_t calcPawnShields()
{
	int64_t eval = 0;

	if (king_occ[ply] & white_occ[ply] & DOMAIN_KINGSIDE & (RANK_1 | RANK_2)) //if white king on kingside
	{
		eval += 35 * _mm_popcnt_u64(pawn_occ[ply] & white_occ[ply] & WHITE_KSIDE_PSHIELD_MSB);
		eval += 15 * _mm_popcnt_u64(pawn_occ[ply] & white_occ[ply] & WHITE_KSIDE_PSHIELD_LSB);
	}
	else if (king_occ[ply] & white_occ[ply] & DOMAIN_QUEENSIDE & (RANK_1 | RANK_2)) //if white king on queenside
	{
		eval += 35 * _mm_popcnt_u64(pawn_occ[ply] & white_occ[ply] & WHITE_QSIDE_PSHIELD_MSB);
		eval += 15 * _mm_popcnt_u64(pawn_occ[ply] & white_occ[ply] & WHITE_QSIDE_PSHIELD_LSB);
	}

	if (king_occ[ply] & black_occ[ply] & DOMAIN_KINGSIDE & (RANK_7 | RANK_8)) //if black king on kingside
	{
		eval -= 35 * _mm_popcnt_u64(pawn_occ[ply] & black_occ[ply] & BLACK_KSIDE_PSHIELD_MSB);
		eval -= 15 * _mm_popcnt_u64(pawn_occ[ply] & black_occ[ply] & BLACK_KSIDE_PSHIELD_LSB);
	}
	else if (king_occ[ply] & black_occ[ply] & DOMAIN_QUEENSIDE & (RANK_7 | RANK_8)) //if black king on queenside
	{
		eval -= 35 * _mm_popcnt_u64(pawn_occ[ply] & black_occ[ply] & BLACK_QSIDE_PSHIELD_MSB);
		eval -= 15 * _mm_popcnt_u64(pawn_occ[ply] & black_occ[ply] & BLACK_QSIDE_PSHIELD_LSB);
	}

	return eval;
}
