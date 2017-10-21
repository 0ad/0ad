/**
 * Normalize the trade gain as a function of mapSize, default=1024
 */
function TradeGainNormalization(mapSize)
{
	return 1;
}

/**
 * Dependance of the gain with distance, normalized on a distance of 100m
 */
function NormalizedTradeGain(distanceSquared)
{
	return distanceSquared / 10000;
}
