#include "TimeInMs.h"

#include <sys/time.h>

#include "KlonkError.h"

uint64_t timeInMs()
{
	timeval t;
	if (gettimeofday(&t, 0))
		throw KlonkError("Unable to get time of day!");
	return t.tv_sec * 1000 + t.tv_usec / 1000;
}