#pragma once

#include <inttypes.h>
#include <iostream>

#define NUM_TT_ENTRIES_1MB 65536 //Number of TT entries per MB
#define TT_SEARCH_WINDOW 1024 //Probe 1024 entries at most

#define TT_NONE 0
#define TT_ALPHA 1
#define TT_BETA 2
#define TT_EXACT 3


typedef struct { uint64_t key; int64_t eval; uint8_t depth; uint8_t flag; } TTEntry; //not most efficient way to store that, but good enough for now

extern uint64_t hashSize;
extern TTEntry tt[16 * NUM_TT_ENTRIES_1MB]; //temp, we will add allocation shit later, now its 16MB no matter what (1M entries max)

extern uint64_t ttHits; //DEBUG

bool probeTT(uint32_t* indexptr, uint64_t key, uint8_t depth); //Probe TT for a given key. If there is a match, then function will return true, and indexptr will point to the match index
void storeTTEntry(uint64_t key, int64_t eval, uint8_t depth, uint8_t flag); //store entry in TT
