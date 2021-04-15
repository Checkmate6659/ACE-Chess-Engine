#include "board.h"


uint64_t occupancy[MAX_SD];
uint64_t white_occ[MAX_SD];
uint64_t black_occ[MAX_SD];
uint64_t pawn_occ[MAX_SD];
uint64_t knight_occ[MAX_SD];
uint64_t bishop_occ[MAX_SD];
uint64_t rook_occ[MAX_SD];
uint64_t queen_occ[MAX_SD];
uint64_t king_occ[MAX_SD];

uint64_t zhash[MAX_SD];

uint8_t castle_rights[MAX_SD];
uint64_t ep[MAX_SD];

MoveList moves[MAX_SD] = {};
uint8_t ply = 0;

bool turn = WHITE;
uint8_t hmc;


void load_fen(std::string fen)
{
	occupancy[0] = 0;
	white_occ[0] = 0;
	black_occ[0] = 0;
	pawn_occ[0] = 0;
	knight_occ[0] = 0;
	bishop_occ[0] = 0;
	rook_occ[0] = 0;
	queen_occ[0] = 0;
	king_occ[0] = 0;

	uint64_t offset;
	uint8_t sqIdx = 56;
	int8_t c = 0;
	int phase = 0;
	turn = WHITE;

	ply = 0;
	hmc = 100;
	castle_rights[0] = 0; //MERGE

	for (; c < fen.length() && !phase; c++)
	{
		offset = 1ULL << sqIdx;
		switch (fen.at(c))
		{
		case 'P':
			occupancy[0] |= offset;
			white_occ[0] |= offset;
			pawn_occ[0] |= offset;
			break;
		case 'N':
			occupancy[0] |= offset;
			white_occ[0] |= offset;
			knight_occ[0] |= offset;
			break;
		case 'B':
			occupancy[0] |= offset;
			white_occ[0] |= offset;
			bishop_occ[0] |= offset;
			break;
		case 'R':
			occupancy[0] |= offset;
			white_occ[0] |= offset;
			rook_occ[0] |= offset;
			break;
		case 'Q':
			occupancy[0] |= offset;
			white_occ[0] |= offset;
			queen_occ[0] |= offset;
			break;
		case 'K':
			occupancy[0] |= offset;
			white_occ[0] |= offset;
			king_occ[0] |= offset;
			break;
		case 'p':
			occupancy[0] |= offset;
			black_occ[0] |= offset;
			pawn_occ[0] |= offset;
			break;
		case 'n':
			occupancy[0] |= offset;
			black_occ[0] |= offset;
			knight_occ[0] |= offset;
			break;
		case 'b':
			occupancy[0] |= offset;
			black_occ[0] |= offset;
			bishop_occ[0] |= offset;
			break;
		case 'r':
			occupancy[0] |= offset;
			black_occ[0] |= offset;
			rook_occ[0] |= offset;
			break;
		case 'q':
			occupancy[0] |= offset;
			black_occ[0] |= offset;
			queen_occ[0] |= offset;
			break;
		case 'k':
			occupancy[0] |= offset;
			black_occ[0] |= offset;
			king_occ[0] |= offset;
			break;
		case '8':
			sqIdx += 7;
			break;
		case '7':
			sqIdx += 6;
			break;
		case '6':
			sqIdx += 5;
			break;
		case '5':
			sqIdx += 4;
			break;
		case '4':
			sqIdx += 3;
			break;
		case '3':
			sqIdx += 2;
			break;
		case '2':
			sqIdx += 1;
			break;
		case '/':
			sqIdx -= 17;
			break;
		case ' ':
			phase = 1;
		}
		sqIdx += 1;
	}

	zhash[0] = zkeys[Z_TURN_IDX];
	for (uint8_t i = 0; i < 64; i++)
	{
		zhash[0] ^= getPieceKey(i); //initialize hash
	}

	if (c == fen.length()) return;
	if (fen.at(c) == 'b')
	{
		turn = BLACK;
		zhash[0] ^= zkeys[Z_TURN_IDX];
	}

	c += 2; //skip the space character between turn and castle rights
	castle_rights[0] = 0x0; //failsafe: if no castle rights specified, do not allow any
	if (c == fen.length()) return;

	for (; c < fen.length() && phase; c++)
	{
		switch (fen.at(c))
		{
		case 'K':
			castle_rights[0] |= 0x1;
			break;
		case 'Q':
			castle_rights[0] |= 0x2;
			break;
		case 'k':
			castle_rights[0] |= 0x4;
			break;
		case 'q':
			castle_rights[0] |= 0x8;
			c++; //black queenside castle right is always the last one
		case ' ':
			phase = 0;
		}
	}

	zhash[0] ^= getCastleKey(castle_rights[0]);

	if (c == fen.length()) return;

	switch (fen.at(c))
	{
	case '-':
		ep[0] = 0; //no ep
		phase = 1;
		break;
	default:
		ep[0] = 1ULL << (fen.at(c) - 'a'); //only keep lower 4 bits, and substract 1, as rank 1 should be represented as 0
		c++;
	}

	if (c == fen.length())
	{
		ep[0] = 0;
		return;
	}
	if (!phase) ep[0] <<= (fen.at(c) - '1') * 8; //same thing here
	if (isEpPseudo(M42::lsb(ep[0])))
	{
		zhash[0] ^= getEpKey(ep[0]);
	}

	c += 2; //skip over space character
	if (c >= fen.length()) return;

	//isolate HMC
	std::string hmc_str = fen.substr(c, 2);
	if(hmc_str.at(1) == ' ') hmc_str = hmc_str.substr(0, 1);
	hmc = 100 - atoi(hmc_str.c_str()); //remember: variable is 100 - HMC
}

uint64_t getCastleKey(uint8_t castle)
{
	uint64_t key = 0;
	if (castle & 0x1) key = zkeys[Z_CASTLE_IDX + Z_CASTLE_WK];
	if (castle & 0x2) key ^= zkeys[Z_CASTLE_IDX + Z_CASTLE_WQ];
	if (castle & 0x4) key ^= zkeys[Z_CASTLE_IDX + Z_CASTLE_BK];
	if (castle & 0x8) key ^= zkeys[Z_CASTLE_IDX + Z_CASTLE_BQ];
	return key;
}

uint8_t getPieceKind(uint8_t sq)
{
	uint8_t kind = 0;
	uint64_t shift = 1ULL << sq;
	if (shift & white_occ[ply]) kind |= Z_WHITE; //if white piece, then use white kind (bit 0)
	if (shift & (knight_occ[ply] | rook_occ[ply] | king_occ[ply])) kind |= Z_KNIGHT; //set bit 1 accordingly
	if (shift & (bishop_occ[ply] | rook_occ[ply])) kind |= Z_BISHOP; //set bit 2 accordingly
	if (shift & (queen_occ[ply] | king_occ[ply])) kind |= Z_QUEEN; //set bit 3 accordingly
	return kind;
}

uint64_t getPieceKey(uint8_t sq)
{
	uint64_t shift = 1ULL << sq;
	if ((occupancy[ply] & shift) == 0) return NULL; //failsafe: if empty square passed in, return null (no changes)
	return zkeys[getPieceKind(sq) << 6 | sq]; //get piece key (index is 0 to 767)
}

bool isEpPseudo(uint8_t sq)
{
	if (M42::pawn_attacks(!turn, sq) & pawn_occ[ply] & (turn ? black_occ : white_occ)[ply]) { //see if our pawn diag from ep square
		return true;
	}
	return false;
}

uint64_t getEpKey(uint64_t map)
{
	if (map)
	{
		register uint8_t sq = M42::lsb(map);
		if (isEpPseudo(sq))
		{
			return zkeys[Z_EP_IDX + (sq & 7)];
		}
	}
	return 0;
}

void make_move(Move move)
{
	register uint64_t src_shift = 1ULL << move.src;
	register uint64_t dst_shift = 1ULL << move.dst;

	register uint64_t not_src_shift = ~src_shift;
	register uint64_t not_dst_shift = ~dst_shift;

	ply++;

	//copy position
	occupancy[ply] = occupancy[ply - 1];
	white_occ[ply] = white_occ[ply - 1];
	black_occ[ply] = black_occ[ply - 1];
	pawn_occ[ply] = pawn_occ[ply - 1];
	knight_occ[ply] = knight_occ[ply - 1];
	bishop_occ[ply] = bishop_occ[ply - 1];
	rook_occ[ply] = rook_occ[ply - 1];
	queen_occ[ply] = queen_occ[ply - 1];
	king_occ[ply] = king_occ[ply - 1];

	castle_rights[ply] = castle_rights[ply - 1]; //copy castle rights
	zhash[ply] = zhash[ply - 1]; //copy hash

	//update zobrist hash pieces (remove source/destination pieces)
	zhash[ply] ^= getPieceKey(move.src);
	zhash[ply] ^= getPieceKey(move.dst);

	//clear destination square
	occupancy[ply] &= not_dst_shift;
	white_occ[ply] &= not_dst_shift;
	black_occ[ply] &= not_dst_shift;
	pawn_occ[ply] &= not_dst_shift;
	knight_occ[ply] &= not_dst_shift;
	bishop_occ[ply] &= not_dst_shift;
	rook_occ[ply] &= not_dst_shift;
	queen_occ[ply] &= not_dst_shift;
	king_occ[ply] &= not_dst_shift;

	//if not promotion
	if (!(move.flags & PROMO_F))
	{
		occupancy[ply] |= !!(occupancy[ply - 1] & src_shift) * dst_shift;
		white_occ[ply] |= !!(white_occ[ply - 1] & src_shift) * dst_shift;
		black_occ[ply] |= !!(black_occ[ply - 1] & src_shift) * dst_shift;
		pawn_occ[ply] |= !!(pawn_occ[ply - 1] & src_shift) * dst_shift;
		knight_occ[ply] |= !!(knight_occ[ply - 1] & src_shift) * dst_shift;
		bishop_occ[ply] |= !!(bishop_occ[ply - 1] & src_shift) * dst_shift;
		rook_occ[ply] |= !!(rook_occ[ply - 1] & src_shift) * dst_shift;
		queen_occ[ply] |= !!(queen_occ[ply - 1] & src_shift) * dst_shift;
		king_occ[ply] |= !!(king_occ[ply - 1] & src_shift) * dst_shift;

		//if kingside castle
		if (move.flags & KCASTLE_F)
		{
			occupancy[ply] ^= 10ULL << move.src; //invert rook occupancies to the right of starting square
			rook_occ[ply] ^= 10ULL << move.src;
			white_occ[ply] ^= (10ULL << move.src) & 0xa0;
			black_occ[ply] ^= (10ULL << move.src) & 0xa000000000000000;
		}
		//if queenside castle
		else if (move.flags & QCASTLE_F)
		{
			not_dst_shift = move.src - 4; //reusing the variable not_dst_shift to access value faster
			occupancy[ply] ^= 9ULL << not_dst_shift; //invert rook occupancies to the left of starting square
			rook_occ[ply] ^= 9ULL << not_dst_shift;
			white_occ[ply] ^= (9ULL << not_dst_shift) & 0x9;
			black_occ[ply] ^= (9ULL << not_dst_shift) & 0x900000000000000;
		}
		//if ep
		else if (move.flags & EP_F)
		{
			occupancy[ply] &= ~(65537ULL * dst_shift >> 8); //remove pieces above (white) or below (black) target square
			pawn_occ[ply] &= ~(65537ULL * dst_shift >> 8); //remove pawns on that square
			white_occ[ply] &= ~(dst_shift << 8); //remove white pieces below target square
			black_occ[ply] &= ~(dst_shift >> 8); //same for black pieces
		}
	}
	else {
		occupancy[ply] |= !!(occupancy[ply - 1] & src_shift) * dst_shift;
		white_occ[ply] |= !!(white_occ[ply - 1] & src_shift) * dst_shift;
		black_occ[ply] |= !!(black_occ[ply - 1] & src_shift) * dst_shift;

		knight_occ[ply] |= dst_shift * (move.promo == KNIGHT_P);
		bishop_occ[ply] |= dst_shift * (move.promo == BISHOP_P);
		rook_occ[ply] |= dst_shift * (move.promo == ROOK_P);
		queen_occ[ply] |= dst_shift * (move.promo == QUEEN_P);
	}

	occupancy[ply] &= not_src_shift;
	white_occ[ply] &= not_src_shift;
	black_occ[ply] &= not_src_shift;
	pawn_occ[ply] &= not_src_shift;
	knight_occ[ply] &= not_src_shift;
	bishop_occ[ply] &= not_src_shift;
	rook_occ[ply] &= not_src_shift;
	queen_occ[ply] &= not_src_shift;
	king_occ[ply] &= not_src_shift;

	//update zobrist hash piece after the move (add it back in)
	zhash[ply] ^= getPieceKey(move.dst);

	//update en passant
	ep[ply] = 0;
	if (move.flags & DPP_F)
	{
		ep[ply] = (src_shift * 0x10001 & 0xff0000ff000000) >> 8; //only keep ep squares on 3rd and 6th rank, to avoid glitch eps on rank 1 for black
	}

	//invert turn
	turn = !turn;

	//update zobrist hash turn
	zhash[ply] ^= zkeys[Z_TURN_IDX];

	//if a king moved, revoke side's castle rights
	if ((king_occ[ply] & 0x10) == 0) castle_rights[ply] &= 0b1100;
	if ((king_occ[ply] & 0x1000000000000000) == 0) castle_rights[ply] &= 0b0011;

	//if a rook moved, remove its castle right
	if ((rook_occ[ply] & white_occ[ply] & 0x1) == 0) castle_rights[ply] &= 0b1101;
	if ((rook_occ[ply] & white_occ[ply] & 0x80) == 0) castle_rights[ply] &= 0b1110;
	if ((rook_occ[ply] & black_occ[ply] & 0x100000000000000) == 0) castle_rights[ply] &= 0b0111;
	if ((rook_occ[ply] & black_occ[ply] & 0x8000000000000000) == 0) castle_rights[ply] &= 0b1011;

	//update zobrist hash castle/ep
	zhash[ply] ^= getCastleKey(castle_rights[ply - 1]);
	zhash[ply] ^= getCastleKey(castle_rights[ply]);
	zhash[ply] ^= getEpKey(ep[ply - 1]);
	zhash[ply] ^= getEpKey(ep[ply]);

	return;
}

void define_as_root()
{
	occupancy[0] = occupancy[ply];
	white_occ[0] = white_occ[ply];
	black_occ[0] = black_occ[ply];
	pawn_occ[0] = pawn_occ[ply];
	knight_occ[0] = knight_occ[ply];
	bishop_occ[0] = bishop_occ[ply];
	rook_occ[0] = rook_occ[ply];
	queen_occ[0] = queen_occ[ply];
	king_occ[0] = king_occ[ply];
	castle_rights[0] = castle_rights[ply];
	ep[0] = ep[ply];
	zhash[0] = zhash[ply];

	ply = 0;
}

void undo_move()
{
	ply--;
	turn = !turn;
}

bool calc_atk(uint8_t sq, bool _turn)
{
	uint64_t offset = 1ULL << sq;

	uint64_t col_occ = (_turn ? black_occ : white_occ)[ply];
	register uint64_t map = M42::calc_king_attacks(col_occ & king_occ[ply]) | M42::calc_knight_attacks(col_occ & knight_occ[ply]) | M42::calc_pawn_attacks(_turn, col_occ & pawn_occ[ply] & occupancy[ply]); //pawn bitboard is anded with occupancy as pawns are the only piece where the occupancy can change without moving them (en passant)
	if (map & offset) return true;

	map = (rook_occ[ply] | queen_occ[ply]) & col_occ & M42::rook_attacks(sq, occupancy[ply]);
	map |= (bishop_occ[ply] | queen_occ[ply]) & col_occ & M42::bishop_attacks(sq, occupancy[ply]);
	if (map) return true;

	return false;
}

bool mask_atk(register uint64_t mask, bool _turn) //no en passant bug fix: ep-d pawns can never stop castling (only use case of this function where perfection is required), too far away
{
	register uint16_t sq;
	uint64_t col_occ = (_turn ? black_occ : white_occ)[ply];
	register uint64_t map = M42::calc_king_attacks(col_occ & king_occ[ply]) | M42::calc_knight_attacks(col_occ & knight_occ[ply]) | M42::calc_pawn_attacks(_turn, col_occ & pawn_occ[ply]);
	if (map & mask) return true;

	while (mask)
	{
		sq = M42::lsb(mask); //square index is lsb position

		map = (rook_occ[ply] | queen_occ[ply]) & col_occ & M42::rook_attacks(sq, occupancy[ply]);
		map |= (bishop_occ[ply] | queen_occ[ply]) & col_occ & M42::bishop_attacks(sq, occupancy[ply]);
		if (map) return true;

		mask &= mask - 1; //remove lsb
	}

	return false;
}

uint64_t piece_mask_atk(uint8_t sq, uint64_t mask, uint8_t type, uint64_t occ)
{
	switch (type)
	{
	case Z_WP:
		return mask & M42::pawn_attacks(WHITE, sq);
	case Z_BP:
		return mask & M42::pawn_attacks(BLACK, sq);
	case Z_WN:
	case Z_BN:
		return mask & M42::knight_attacks(sq);
	case Z_WB:
	case Z_BB:
		return mask & M42::bishop_attacks(sq, occ);
	case Z_WR:
	case Z_BR:
		return mask & M42::rook_attacks(sq, occ);
	case Z_WQ:
	case Z_BQ:
		return mask & M42::queen_attacks(sq, occ);
	} //no king, because it cannot directly give check
	return 0;
}

void convertToMoves(MoveList* mlist, uint8_t start, register uint64_t bitboard, uint8_t promo, uint8_t sflag, uint64_t sflag_bb)
{
	Move current_move;
	current_move.src = start;
	current_move.promo = QUEEN_P;

	register uint64_t *oppColOcc = (turn ? white_occ+ply : black_occ+ply); //pointer to current opponent color occupancy
	bool isCheck = false;
	register uint64_t buffer;
	uint8_t kingSquare = M42::lsb(king_occ[ply] & (occupancy[ply] ^ *oppColOcc)); //get own king square

	occupancy[ply] ^= 1ULL << start; //empty starting square (because it will always be occupied)

	for (; bitboard; bitboard &= bitboard - 1)
	{
		current_move.flags = NULL;
		current_move.dst = M42::lsb(bitboard);

		if (1ULL << current_move.src & king_occ[ply]) //if the king has moved
		{
			isCheck = calc_atk(current_move.dst, !turn); //see if target square is attacked (starting square occupancy already inverted)
		}
		else
		{
			if ((1ULL << current_move.dst) & ep[ply] && (1ULL << current_move.src) & pawn_occ[ply]) current_move.flags |= EP_F; //en passant capture

			*oppColOcc ^= 1ULL << current_move.dst; //invert enemy piece on that square: if it is a null piece, it will be skipped
			if(occupancy[ply] & 1ULL << current_move.dst)
			{
				isCheck = calc_atk(kingSquare, !turn); //calculate if enemy attacks square
			}
			else {
				buffer = occupancy[ply]; //save occupancy
				occupancy[ply] ^= 1ULL << current_move.dst; //do same XOR as for start: square is now occupied
				if(current_move.flags & EP_F) occupancy[ply] &= ~(65537ULL * (1ULL << current_move.dst) >> 8); //if ep, remove captured pawn
				isCheck = calc_atk(kingSquare, !turn); //calculate if enemy attacks square
				occupancy[ply] = buffer; //restore occupancy
			}
			*oppColOcc ^= 1ULL << current_move.dst; //do same for opponent color occupancy
		}

		if (isCheck) continue;

		mlist->moves[mlist->count] = current_move;
		mlist->count++;

		if ((1ULL << current_move.dst) & sflag_bb) //if current move destination corresponds to flag
		{
			if(sflag & CASTLE_F) //castling
			{
				//if not one type of castling, then it is the other
				if (!((1ULL << current_move.dst) & 0x400000000000004)) mlist->moves[mlist->count - 1].flags |= KCASTLE_F;
				if (!((1ULL << current_move.dst) & 0x4000000000000040)) mlist->moves[mlist->count - 1].flags |= QCASTLE_F;
			}
			else mlist->moves[mlist->count - 1].flags |= sflag;
		}

		if ((1ULL << current_move.dst) & *oppColOcc) mlist->moves[mlist->count - 1].flags |= CAPT_F; //give capture flag if destination is enemy
		if (piece_mask_atk(current_move.dst, king_occ[ply] & *oppColOcc, getPieceKind(current_move.src), occupancy[ply])) mlist->moves[mlist->count - 1].flags |= CHK_F; //if enemy king attacked by destination square through moveset of piece on starting square, then check

		occupancy[ply] ^= 1ULL << current_move.dst; //destination square is occupied: avoid a king/slider fake discovered check that could mess up the move ordering a bit, even though there is no risk for double check
		if (mask_atk(king_occ[ply] & *oppColOcc, turn)) mlist->moves[mlist->count - 1].flags += CHK_F; //addition: if the two checks add up, it is not a simple check, but a double one. There will be always be check if FLAGS & 0x30 is non-zero.
		occupancy[ply] ^= 1ULL << current_move.dst; //restore occupancy

		if ((1ULL << current_move.dst) & (promo * 0x100000000000001)) //if current move destination is promo
		{
			mlist->moves[mlist->count - 1].flags |= PROMO_F;
			current_move.flags |= PROMO_F;

			current_move.promo++;
			mlist->moves[mlist->count] = current_move;
			mlist->count++;

			current_move.promo++;
			mlist->moves[mlist->count] = current_move;
			mlist->count++;

			current_move.promo++;
			mlist->moves[mlist->count] = current_move;
			mlist->count++;

			current_move.promo = QUEEN_P;
		}
	}
	
	occupancy[ply] ^= 1ULL << start; //XOR out the starting square mask from the beginning
}

MoveList gen_moves()
{
	MoveList mlist;
	mlist.count = 0;

	register uint8_t pieceIdx;
	register uint64_t offset;
	register uint64_t moves = 0;
	register uint8_t promo;

	uint64_t col_occ = (turn ? black_occ : white_occ)[ply];
	register uint8_t flag;
	register uint64_t flag_bb;

	for (register uint64_t cur_occ = col_occ; cur_occ; cur_occ &= cur_occ - 1)
	{
		pieceIdx = M42::lsb(cur_occ);
		offset = 1ULL << pieceIdx;
		promo = 0;
		flag = 0;
		flag_bb = 0;

		if (offset & king_occ[ply])
		{
			moves = M42::king_attacks(pieceIdx);

			if ((castle_rights[ply] & KCASTLE_F << (turn << 1)) && !(occupancy[ply] & (turn ? 0x6000000000000000 : 0x60)) && !(mask_atk(turn ? 0x7000000000000000 : 0x70, !turn))) //if currently active player can castle kingside, and required squares are not atked
			{
				flag = CASTLE_F;
				moves |= offset << 2; //kingside castling: move king 2 squares to the right
				flag_bb |= offset << 2; //indicate castling to move converter
			}
			if ((castle_rights[ply] & QCASTLE_F << (turn << 1)) && !(occupancy[ply] & (turn ? 0xe00000000000000 : 0xe)) && !(mask_atk(turn ? 0x1c00000000000000 : 0x1c, !turn))) //if currently active player can castle queenside, and required squares are not atked
			{
				flag = CASTLE_F;
				moves |= offset >> 2; //queenside castling: move king 2 squares to the left
				flag_bb |= offset >> 2; //indicate castling to move converter
			}
		}
		else if (offset & knight_occ[ply])
		{
			moves = M42::knight_attacks(pieceIdx);
		}
		else if (offset & pawn_occ[ply] & white_occ[ply])
		{
			moves = M42::pawn_attacks(WHITE, pieceIdx) & (occupancy[ply] ^ col_occ | ep[ply]);
			moves |= !((offset << 8) & occupancy[ply]) * (offset << 8);
			flag = DPP_F;
			flag_bb = !(pieceIdx & 248 ^ 8 | (((offset << 8) * 0x101) & occupancy[ply])) * (offset << 16);
			moves |= flag_bb;
			promo = moves >> 56; //only "bottom" rank moves result in promo
		}
		else if (offset & pawn_occ[ply] & black_occ[ply])
		{
			moves = M42::pawn_attacks(BLACK, pieceIdx) & (occupancy[ply] ^ col_occ | ep[ply]);
			moves |= !((offset >> 8) & occupancy[ply]) * (offset >> 8);
			flag = DPP_F;
			flag_bb = !(pieceIdx & 248 ^ 48 | (((offset >> 16) * 0x101) & occupancy[ply])) * (offset >> 16);
			moves |= flag_bb;
			promo = moves; //forcefully lose other data, as only "top" rank moves result in promo
		}
		else if (offset & rook_occ[ply])
		{
			moves = M42::rook_attacks(pieceIdx, occupancy[ply]);
		}
		else if (offset & bishop_occ[ply])
		{
			moves = M42::bishop_attacks(pieceIdx, occupancy[ply]);
		}
		else if (offset & queen_occ[ply])
		{
			moves = M42::queen_attacks(pieceIdx, occupancy[ply]);
		}

		moves &= ~col_occ;
		convertToMoves(&mlist, pieceIdx, moves, promo, flag, flag_bb);
	}

	return mlist;
}

MoveList gen_loud_moves()
{
	MoveList mlist;
	mlist.count = 0;

	register uint8_t pieceIdx;
	register uint64_t offset;
	register uint64_t moves = 0;
	register uint8_t promo;

	uint64_t col_occ = (turn ? black_occ : white_occ)[ply];
	register uint64_t promo_bb;

	for (register uint64_t cur_occ = col_occ; cur_occ; cur_occ &= cur_occ - 1)
	{
		pieceIdx = M42::lsb(cur_occ);
		offset = 1ULL << pieceIdx;
		promo = 0;
		promo_bb = 0;

		if (offset & king_occ[ply])
		{
			moves = M42::king_attacks(pieceIdx);
		}
		else if (offset & knight_occ[ply])
		{
			moves = M42::knight_attacks(pieceIdx);
		}
		else if(offset & pawn_occ[ply] & white_occ[ply])
		{
			moves = M42::pawn_attacks(WHITE, pieceIdx);
			moves |= offset << 8 & ~occupancy[ply];
			promo = moves >> 56; //only "bottom" rank moves result in promo
			promo_bb = 0xff00000000000000;
		}
		else if(offset & pawn_occ[ply] & black_occ[ply])
		{
			moves = M42::pawn_attacks(BLACK, pieceIdx);
			moves |= offset >> 8 & ~occupancy[ply];
			promo = moves; //forcefully lose other data, as only "top" rank moves result in promo
			promo_bb = 0xff;
		}
		else if (offset & rook_occ[ply])
		{
			moves = M42::rook_attacks(pieceIdx, occupancy[ply]);
		}
		else if (offset & bishop_occ[ply])
		{
			moves = M42::bishop_attacks(pieceIdx, occupancy[ply]);
		}
		else if (offset & queen_occ[ply])
		{
			moves = M42::queen_attacks(pieceIdx, occupancy[ply]);
		}

		moves &= occupancy[ply] ^ col_occ | ep[ply] | promo_bb; //only keep captures, en passant captures and promotions
		convertToMoves(&mlist, pieceIdx, moves, promo, 0, 0);
	}

	return mlist;
}
