#pragma once

#include <string>
#include "m42.h"
#include "keys.h"

#define WHITE false
#define BLACK true

#define QUEEN_P 0
#define KNIGHT_P 1
#define ROOK_P 2
#define BISHOP_P 3

#define PROMO_F 0x80
#define CAPT_F 0x40
#define DCHK_F 0x20 //double check flag
#define CHK_F 0x10 //check flag
#define DPP_F 0x04 //double pawn push flag: used for en passant
#define EP_F 0x08 //en passant capture flag
#define KCASTLE_F 0x01 //kingside castle flag
#define QCASTLE_F 0x02 //queenside castle flag
#define CASTLE_F 0x10 //general castle flag, used inside move converter

#define MAX_SD 255 //maximal search depth; if it is changed above 256, some types in the code have to be changed from uint8_t to uint16_t (even though that's a very unlikely scenario)
#define MAX_MOVE 255 //maximal number of legal moves in a valid position

#define CHECKMATE -4611686018427387904
#define DRAW 0

#define CENTER_MASK 0x3c3c3c3c0000 //MERGE

#define FILE_A 0x101010101010101
#define FILE_B 0x202020202020202
#define FILE_C 0x404040404040404
#define FILE_D 0x808080808080808
#define FILE_A 0x1010101010101010
#define FILE_B 0x2020202020202020
#define FILE_C 0x4040404040404040
#define FILE_D 0x8080808080808080

#define RANK_1 0xff
#define RANK_2 0xff00
#define RANK_3 0xff0000
#define RANK_4 0xff000000
#define RANK_5 0xff00000000
#define RANK_6 0xff0000000000
#define RANK_7 0xff000000000000
#define RANK_8 0xff00000000000000

#define DOMAIN_CENTER 0x3c3c3c3c3c3c3c3c
#define DOMAIN_KCENTER 0x3838383838383838
#define DOMAIN_KINGSIDE 0xc0c0c0c0c0c0c0c0
#define DOMAIN_QUEENSIDE 0x707070707070707


typedef struct { uint8_t src; uint8_t dst; uint8_t promo; uint8_t flags; } Move;
typedef struct { Move moves[MAX_MOVE]; uint16_t count; } MoveList;

extern uint64_t occupancy[MAX_SD]; //board occupancy
extern uint64_t white_occ[MAX_SD]; //white piece occupancy
extern uint64_t black_occ[MAX_SD]; //black piece occupancy
extern uint64_t pawn_occ[MAX_SD]; //pawn bitboard
extern uint64_t knight_occ[MAX_SD]; //knight bitboard
extern uint64_t bishop_occ[MAX_SD]; //bishop bitboard
extern uint64_t rook_occ[MAX_SD]; //rook bitboard
extern uint64_t queen_occ[MAX_SD]; //queen bitboard
extern uint64_t king_occ[MAX_SD]; //king bitboard

extern uint64_t zhash[MAX_SD]; //zobrist hash

extern uint8_t castle_rights[MAX_SD]; //castling rights; order: qkQK (reverse of the FEN castle right notation)
extern uint64_t ep[MAX_SD]; //en passant captures

extern uint8_t ply;
extern bool turn;
extern uint8_t hmc; //This is NOT the half-move clock, but the number of plies before it hits 100 (it is 100-half_move_clock)


void load_fen(std::string fen);
uint64_t getCastleKey(uint8_t castle);
uint8_t getPieceKind(uint8_t sq);
uint64_t getPieceKey(uint8_t sq);
bool isEpPseudo(uint8_t sq);
uint64_t getEpKey(uint64_t map);
void make_move(Move move);
void define_as_root();
void undo_move();
bool calc_atk(uint8_t sq, bool _turn);
bool mask_atk(uint64_t mask, bool _turn);
MoveList gen_moves();
MoveList gen_loud_moves();
