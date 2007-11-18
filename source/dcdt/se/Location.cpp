//Location.cpp

#include "precompiled.h"

#include <cstdlib>
#include <math.h>
#include "se_dcdt.h"
#include <fstream>

//the list of unprocessed triangles
template <class T>
SrArray<SeLinkFace<T> *> SeLinkFace<T>::processing;

//DJD: Point location function definitions {

//initializes each sector midpoint to point to the triangle in which it is contained
void SeDcdt::InitializeSectors()
{
#if defined EXPERIMENT
	//count the number of triangles
	int num = 0;
#endif
	//calculate the width and height of the triangulation
	float width = _xmax - _xmin;
	float height = _ymax - _ymin;
	//calculate the width and height of each sector
	sectorWidth = width / (float)xSectors;
	sectorHeight = height / (float)ySectors;
	//initialize all sectors to null
	for (int i = 0; i < ySectors; i++)
	{
		for (int j = 0; j < xSectors; j++)
		{
			sectors[i][j] = NULL;
		}
	}
	float x, y;
	float x1, y1, x2, y2, x3, y3;
	//go through the unprocessed triangles
	for (int k = 0; k < SeDcdtFace::Faces(); k++)
	{
		//retrieve the next valid triangle
		SeDcdtFace *currentFace = SeDcdtFace::Face(k);
		if (currentFace == NULL)
		{
			continue;
		}
#if defined EXPERIMENT
		//keep track of the number of triangles
		num++;
#endif
		//get the minimum and maximum x and y values
		TriangleVertices(currentFace, x1, y1, x2, y2, x3, y3);
		float left = Min(x1, x2, x3);
		float right = Max(x1, x2, x3);
		float top = Min(y1, y2, y3);
		float bottom = Max(y1, y2, y3);
		//calculate the minimum and maximum x and y sector indices covered
		int xIndexMin = (int)((left - _xmin + 0.5f * sectorWidth) / sectorWidth);
		int xIndexMax = (int)((right - _xmin - 0.5f * sectorWidth) / sectorWidth);
		int yIndexMin = (int)((top - _ymin + 0.5f * sectorHeight) / sectorHeight);
		int yIndexMax = (int)((bottom - _ymin - 0.5f * sectorHeight) / sectorHeight);
		//go through the covered sector midpoints
		for (int i = yIndexMin; i <= yIndexMax; i++)
		{
			for (int j = xIndexMin; j <= xIndexMax; j++)
			{
				//calculate the location of the midpoint
				x = ((float)j + 0.5f) * sectorWidth + _xmin;
				y = ((float)i + 0.5f) * sectorHeight + _ymin;
				//if the midpoint is in the current triangle,
				if (InTriangle(currentFace, x, y))
				{
					//point the sector to that triangle
					sectors[i][j] = currentFace;
				}
			}
		}
	}
#if defined EXPERIMENT
	//if we are running an experiment, output the number of triangles to a data file
	std::ofstream outFile("Data.txt", std::ios_base::app);
	outFile << "Triangles: " << num << "\t";
	outFile.close();
#endif
}

//check if the given point is within a given triangle
bool SeDcdt::InTriangle(SeDcdtFace *face, float x, float y)
{
	//get the triangle's vertices
	float x1, y1, x2, y2, x3, y3;
	TriangleVertices(face, x1, y1, x2, y2, x3, y3);
	//check that the point is counterclockwise of all edges
	return ((Orientation(x1, y1, x2, y2, x, y) >= 0)
		&& (Orientation(x2, y2, x3, y3, x, y) >= 0)
		&& (Orientation(x3, y3, x1, y1, x, y) >= 0));
}

//uses the sectors to locate in which triangle the given point lies
SeTriangulator::LocateResult SeDcdt::LocatePoint(float x, float y, SeBase* &result)
{
	//calculates the indices of the sector in which the point lies
	int xIndex = (int)((x - _xmin) / sectorWidth);
	int yIndex = (int)((y - _ymin) / sectorHeight);
	//makes sure the indices are within the proper bounds
	xIndex = (xIndex < 0) ? 0 : (xIndex >= xSectors) ? xSectors - 1 : xIndex;
	yIndex = (yIndex < 0) ? 0 : (yIndex >= ySectors) ? ySectors - 1 : yIndex;
	//retrieve the triangle in which that sector midpoint is located
	SeFace *iniface = sectors[yIndex][xIndex];
	//if it was invalid, start at the usual place
	if (iniface == NULL)
	{
		iniface = _search_face();
	}
	//use this starting triangle to start point location
	return _triangulator->locate_point(iniface, x, y, result);
}

//locate the given point starting at the usual place
SeTriangulator::LocateResult SeDcdt::LocatePointOld(float x, float y, SeBase* &result)
{
	return _triangulator->locate_point(_search_face(), x, y, result);
}

//returns the maximum of the 3 values passed
float SeDcdt::Max(float a, float b, float c)
{
	return ((a >= b) && (a >= c)) ? a :
		((b >= a) && (b >= c)) ? b : c;
}

//returns the minimum of the 3 values passed
float SeDcdt::Min(float a, float b, float c)
{
	return ((a <= b) && (a <= c)) ? a :
		((b <= a) && (b <= c)) ? b : c;
}

//return a random value in the range of x values (used for testing)
float SeDcdt::RandomX()
{
	return RandomBetween(_xmin, _xmax);
}

//return a random value in the range of y values (used for testing)
float SeDcdt::RandomY()
{
	return RandomBetween(_ymin, _ymax);
}

//returns a random number between the values given
float SeDcdt::RandomBetween(float min, float max)
{
	float base = (float)rand() / (float)RAND_MAX;	// TODO: Make net-safe
	return (base * (max - min) + min);
}

//Point location function definitions }
