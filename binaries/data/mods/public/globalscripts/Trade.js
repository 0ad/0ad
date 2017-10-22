/**
 * Normalize the trade gain as a function of mapSize for a default of: size=1024 and distance= 100m
 */
function TradeGainNormalization(mapSize)
{
	return 1 / TradeGain(10000, mapSize);
}

/**
 * Part of the trade gain which depends on the distance, the full gain being TradeGainNormalization * TradeGain.
 */
function TradeGain(distanceSquared, mapSize)
{
	return distanceSquared;
}
