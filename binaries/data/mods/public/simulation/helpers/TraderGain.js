// This constant used to adjust gain value depending on distance
const DISTANCE_FACTOR = 1 / 110;

// Additional gain for trading performed between markets of different players, in percents
const INTERNATIONAL_TRADING_ADDITION = 50;

function CalculateTraderGain(firstMarket, secondMarket, template)
{
	var cmpFirstMarketPosition = Engine.QueryInterface(firstMarket, IID_Position);
	var cmpSecondMarketPosition = Engine.QueryInterface(secondMarket, IID_Position);
	if (!cmpFirstMarketPosition || !cmpFirstMarketPosition.IsInWorld() || !cmpSecondMarketPosition || !cmpSecondMarketPosition.IsInWorld())
		return null;
	var firstMarketPosition = cmpFirstMarketPosition.GetPosition2D();
	var secondMarketPosition = cmpSecondMarketPosition.GetPosition2D();

	// Calculate ordinary Euclidean distance between markets.
	// We don't use pathfinder, because ordinary distance looks more fair.
	var distance = Math.sqrt(Math.pow(firstMarketPosition.x - secondMarketPosition.x, 2) + Math.pow(firstMarketPosition.y - secondMarketPosition.y, 2));
	// We calculate gain as square of distance to encourage trading between remote markets
	var gain = Math.pow(distance * DISTANCE_FACTOR, 2);

	// If markets belongs to different players, multiple gain to INTERNATIONAL_TRADING_MULTIPLIER
	var cmpFirstMarketOwnership = Engine.QueryInterface(firstMarket, IID_Ownership);
	var cmpSecondMarketOwnership = Engine.QueryInterface(secondMarket, IID_Ownership);
	if (cmpFirstMarketOwnership.GetOwner() != cmpSecondMarketOwnership.GetOwner())
		gain *= 1 + INTERNATIONAL_TRADING_ADDITION / 100;

	if (template && template.GainMultiplier)
		gain *= template.GainMultiplier;
	gain = Math.round(gain);
	return gain;
}

Engine.RegisterGlobal("CalculateTraderGain", CalculateTraderGain);
