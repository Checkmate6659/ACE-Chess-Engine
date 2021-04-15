#include "tt.h"


uint64_t hashSize = 16 * NUM_TT_ENTRIES_1MB; //16MB size
TTEntry tt[16 * NUM_TT_ENTRIES_1MB]; //temporary, will be changed later

uint64_t ttHits = 0; //DEBUG

bool probeTT(uint32_t* indexptr, uint64_t key, uint8_t depth) //returns success boolean
{
	uint32_t startIdx = key % hashSize; //kind of like a hash table

	for (uint32_t idx = startIdx; idx < startIdx + TT_SEARCH_WINDOW; idx++)
	{
		//there is a bug with the depth!!! it is always too high!!!!! how the fuck?!
		if (tt[idx % hashSize].key == key)
		{
			if (tt[idx % hashSize].depth >= depth) //found match, and it has equal or greater depth
			{
				*indexptr = idx % hashSize;
				//std::cout << "HIT " << (int)tt[idx % hashSize].depth << " " << (int)depth << std::endl;
				ttHits++;
				//return false; //temp
				return true; //THERE IS A BUG THO!
			}
			else { //same key, but not great depth (if there are no bugs, each entry should be unique)
				return false;
			}
		}

		if (tt[idx % hashSize].depth == 0) return false; //if found an empty entry, then it has to be before that, so it isn't
	}

	return false; //didn't find any match
}

void storeTTEntry(uint64_t key, int64_t eval, uint8_t depth, uint8_t flag)
{
	uint32_t startIdx = key % hashSize;
	uint32_t idx;
	uint32_t worstIdx = startIdx;

	for (idx = startIdx; idx < startIdx + TT_SEARCH_WINDOW; idx++)
	{
		if (tt[idx % hashSize].key == key || (tt[idx].key == 0)) break; //if found matching or empty slot, then it will always be the worst one
		if (tt[idx % hashSize].depth < tt[worstIdx].depth) worstIdx = idx; //if a lower depth is found, it will be the new worst slot (NOTE: this favors earlier bad slots)
	}

	idx %= hashSize;

	tt[idx].key = key;
	tt[idx].eval = eval;
	tt[idx].depth = depth;
	tt[idx].flag = flag;
}
