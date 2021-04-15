#pragma once

#include <inttypes.h>
#include "board.h"

#define PV_O 32 //Score for a previous PV node
#define KILLER1_O 64 //score for primary killer move
#define KILLER2_O 63 //score for secondary killer move
#define HISTORY_MAX_O (uint32_t)31 //History moves can only get an extra bonus below 32
#define HISTORY_RSH_O 3 //History move score should be right-shifted by that amount

typedef struct { Move moves[MAX_SD]; uint8_t count; } Variation; //A possible variation

extern Move primaryKillers[MAX_SD]; //killer moves: quiet moves that cause a beta cutoff
extern Move secondaryKillers[MAX_SD]; //2 layers of killers
extern uint32_t history[12][64]; //history moves: indexed by piece kind and destination square
extern Variation previousPV;


void clearKillers();
void clearHistory();

void orderMoves(MoveList* mlist);
uint8_t getKey(Move move);
