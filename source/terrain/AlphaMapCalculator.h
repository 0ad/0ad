#ifndef _ALPHAMAPCALCULATOR_H
#define _ALPHAMAPCALCULATOR_H

#include <string.h>
#include "BlendShapes.h"

namespace CAlphaMapCalculator {	
	int Calculate(BlendShape8 shape,unsigned int& flipflags);
}

#endif
