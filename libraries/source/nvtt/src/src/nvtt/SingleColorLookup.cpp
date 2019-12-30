
#include "SingleColorLookup.h"

#include "nvcore/Debug.h"

#include <stdlib.h> // abs

// Globals
uint8 OMatch5[256][2];
uint8 OMatch6[256][2];
uint8 OMatchAlpha5[256][2];
uint8 OMatchAlpha6[256][2];



static int Mul8Bit(int a, int b)
{
	int t = a * b + 128;
	return (t + (t >> 8)) >> 8;
}

static inline int Lerp13(int a, int b)
{
#ifdef DXT_USE_ROUNDING_BIAS
    // with rounding bias
    return a + Mul8Bit(b-a, 0x55);
#else
    // without rounding bias
    // replace "/ 3" by "* 0xaaab) >> 17" if your compiler sucks or you really need every ounce of speed.
    return (a * 2 + b) / 3;
#endif
}

static void PrepareOptTable(uint8 * table, const uint8 * expand, int size, bool alpha_mode)
{
	for (int i = 0; i < 256; i++)
	{
		int bestErr = 256 * 100;

		for (int min = 0; min < size; min++)
		{
			for (int max = 0; max < size; max++)
			{
				int mine = expand[min];
				int maxe = expand[max];

				int err;
                if (alpha_mode) err = abs((maxe + mine)/2 - i);
                else err = abs(Lerp13(maxe, mine) - i);
                err *= 100;

                // DX10 spec says that interpolation must be within 3% of "correct" result,
                // add this as error term. (normally we'd expect a random distribution of
                // +-1.5% error, but nowhere in the spec does it say that the error has to be
                // unbiased - better safe than sorry).
				err += abs(max - min) * 3;

				if (err < bestErr)
				{
					table[i*2+0] = max;
					table[i*2+1] = min;
					bestErr = err;
				}
			}
		}
	}
}


NV_AT_STARTUP(initSingleColorLookup());

void initSingleColorLookup()
{
	uint8 expand5[32];
	uint8 expand6[64];

    for (int i = 0; i < 32; i++) {
		expand5[i] = (i<<3) | (i>>2);
    }

    for (int i = 0; i < 64; i++) {
		expand6[i] = (i<<2) | (i>>4);
    }

	PrepareOptTable(&OMatch5[0][0], expand5, 32, false);
	PrepareOptTable(&OMatch6[0][0], expand6, 64, false);
    PrepareOptTable(&OMatchAlpha5[0][0], expand5, 32, true);
	PrepareOptTable(&OMatchAlpha6[0][0], expand6, 64, true);
}

