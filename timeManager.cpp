#include "timeManager.h"


uint32_t allocatedTime(int32_t time, int32_t inc, uint8_t movestogo)
{
	return (time/movestogo)+inc-TIME_SAFETY_MARGIN;
}
