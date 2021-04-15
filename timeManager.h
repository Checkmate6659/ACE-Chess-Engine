#pragma once

#include <inttypes.h>
#include "board.h"

#define TIME_SAFETY_MARGIN 50 //removes 50ms from the allocated time, to compensate for engine<->GUI latency


//simple time manager script
uint32_t allocatedTime(int32_t time, int32_t inc, uint8_t movestogo);
