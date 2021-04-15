#pragma once

#include <inttypes.h>
#include <algorithm>
#include "board.h"
#include "psq.h"

//pawn shield masks
#define WHITE_KSIDE_PSHIELD_MSB 0x80e000
#define WHITE_KSIDE_PSHIELD_LSB 0x40e000
#define BLACK_KSIDE_PSHIELD_MSB 0xe0800000000000
#define BLACK_KSIDE_PSHIELD_LSB 0xe0400000000000

#define WHITE_QSIDE_PSHIELD_MSB 0x700
#define WHITE_QSIDE_PSHIELD_LSB 0x30700
#define BLACK_QSIDE_PSHIELD_MSB 0x7000000000000
#define BLACK_QSIDE_PSHIELD_LSB 0x7030000000000

//#define ENABLE_BISHOP_TAPER


bool isEndgame(), isLateEndgame(), isInsufficientMaterial();
int getGamePhase();
int64_t evaluate();
int64_t countMaterial();
int64_t computePSqTables(int phase);
int64_t calcMaterialAdjustments(int phase);
int64_t calcKingSafety();
int64_t calcPawnShields();
