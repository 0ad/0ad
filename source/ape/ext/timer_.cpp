#include "timer_.h"
#include <time.h>

double timeStart;

double get_time()
{
	return (double)clock() / 1000;
}