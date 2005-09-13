#include "stdafx.h"
#include "random.h"

using namespace boost;

mt19937 rng;

void SeedRand(unsigned long s) {
	rng.seed(s);
}

int RandInt(int maxVal) {
	return rng()%maxVal;
}

float RandFloat() {
	return float(rng()) * (1.0f/4294967296.0f);
}

float RandFloat(float minVal, float maxVal) {
	return minVal + RandFloat() * (maxVal - minVal);
}

int RandInt(int minVal, int maxVal) {
	return minVal + RandInt(maxVal - minVal + 1);
}


