// $Id: crc32.cpp,v 1.1 2004/07/08 15:21:21 philip Exp $

#include "precompiled.h"

// CRC32, based on code copied from anywhere on the internet.
// Find a thousand possible sources at
// http://www.google.com/search?q=0xEDB88320L

unsigned long crc_table[256];

int generate_table()
{
	unsigned long crc;
	int	i, j;

	const unsigned long poly = 0xEDB88320L; // magic

	for (i = 0; i < 256; ++i)
	{
		crc = i;
		for (j = 0; j < 8; ++j)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ poly;
			else
				crc >>= 1;
		}
		crc_table[i] = crc;
	}
	return 0;
}


unsigned long crc32_calculate(char* data, int len)
{
	// Only calculate the table once
	static int _temp = generate_table();

	unsigned long crc;

	crc = ~0;
	while (len)
	{
		crc = (crc>>8) ^ crc_table[ (crc^*data) & 0xff ];
		--len; ++data;
	}

	return ~crc;
}
