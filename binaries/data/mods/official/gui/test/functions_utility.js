function getRandom(randomMin, randomMax)
{
	// Returns a random whole number in a min..max range.
	// NOTE: There should probably be an engine function for this,
	// since we'd need to keep track of random seeds for replays.

	randomNum = randomMin + (randomMax-randomMin)*Math.random();  // num is random, from A to B 
	return Math.round(randomNum);
}

// ====================================================================