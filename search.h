#pragma once

#include <inttypes.h>
#include <time.h>

#include "board.h"
#include "eval.h"
#include "order.h"

#define FRONTIER 1 //depth at frontier nodes
#define PRE_FRONTIER 2 //depth at pre-frontier nodes
#define PRE_PRE_FRONTIER 3 //depth at pre-pre-frontier nodes

#define ENABLE_NMH //Enable null move pruning
#define ENABLE_FP //Enable futility pruning
#define ENABLE_EFP //Enable extended futility pruning
#define ENABLE_LRAZOR //Enable limited razoring

#define ASPI_WINDOW_WIDTH 0 //Aspiration window width (25cp)
#define NMH_ACTIVATE 2 //NMH only activates above this margin
#define NMH_SD_REDUCTION 1 //NMH reduction (1 ply)
#define LMR_REDUCTION 1 //LMR reduction (1 ply)
#define LMR_INV_LEFT_PROP 2 //only 1/2 of the moves are not reduced (0 means no reduction) WARNING: LMR BUGGY
#define DELTA 250 //Delta pruning safety margin (250cp < minor)
#define FUTILITY_MARGIN 350 //Futility margin (350cp > minor)
#define EXT_FUTILITY_MARGIN 550 //EF margin (550cp > rook)
#define LRAZOR_MARGIN 920 //Limited razor margin (920cp > queen)
#define LRAZOR_REDUCTION 1 //Limited razoring reduction (1 ply)

#define START_SD 3 //starting depth for iterative deepening


extern uint64_t evalNodeCount;

uint64_t perft(int depth); //basic perft function for debugging

int64_t getEvaluation();
int64_t negamax(unsigned depth, int64_t alpha, int64_t beta, bool nmhAuth, Variation* variation, uint8_t curHMC); //negamax
int64_t qSearch(int64_t alpha, int64_t beta); //quiescence search
int64_t qSearchEndgame(int64_t alpha, int64_t beta); //quiescence search with delta pruning disabled, used for the late endgame (if not later replaced by EGTB)
int64_t negamaxRoot(unsigned depth, Move* moveptr, Variation* variation); //root negamax
int64_t negamaxRootIterDeepening(unsigned long allocatedTime, Move* moveptr);
