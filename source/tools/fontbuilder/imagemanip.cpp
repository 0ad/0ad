// $Id: imagemanip.cpp,v 1.1 2004/06/17 19:32:04 philip Exp $

#include "stdafx.h"

#include "math.h"

unsigned char* GenerateImage(int width, int height)
{
	unsigned char* ImageData = new unsigned char[width*height*3];

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// Slightly off black, so you can see the bounding boxes
			// by playing in some paint program
			ImageData[(x+y*width)*3+0] = 0;
			ImageData[(x+y*width)*3+1] = 0;
			ImageData[(x+y*width)*3+2] = 1;
		}
	}

	return ImageData;
}
