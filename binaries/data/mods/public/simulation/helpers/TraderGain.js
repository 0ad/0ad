// This constant used to adjust gain value depending on distance
const DISTANCE_FACTOR = 1 / 110;

// Additional gain (applying to each market) for trading performed between markets of different players, in percents
const INTERNATIONAL_TRADING_ADDITION = 25;

// If trader undefined, the trader owner is supposed to be the same as the first market
function CalculateTraderGain(firstMarket, secondMarket, template, trader)
{
	var gain = {};

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
	gain.traderGain = Math.pow(distance * DISTANCE_FACTOR, 2);
	if (template && template.GainMultiplier)
		gain.traderGain *= template.GainMultiplier;
	gain.traderGain = Math.round(gain.traderGain);
	// If trader undefined, the trader owner is supposed to be the same as the first market
	if (trader)
		var cmpOwnership = Engine.QueryInterface(trader, IID_Ownership);
	else
		var cmpOwnership = Engine.QueryInterface(firstMarket, IID_Ownership);
	gain.traderOwner = cmpOwnership.GetOwner();

	// If markets belong to different players, add gain from international trading
	var ownerFirstMarket = Engine.QueryInterface(firstMarket, IID_Ownership).GetOwner();
	var ownerSecondMarket = Engine.QueryInterface(secondMarket, IID_Ownership).GetOwner();
	if (ownerFirstMarket != ownerSecondMarket)
	{
		var internationalGain1 = ApplyValueModificationsToEntity("Trade/International", INTERNATIONAL_TRADING_ADDITION, firstMarket);
		gain.market1Gain = Math.round(gain.traderGain * internationalGain1 / 100);
		gain.market1Owner = ownerFirstMarket;
		var internationalGain2 = ApplyValueModificationsToEntity("Trade/International", INTERNATIONAL_TRADING_ADDITION, secondMarket);
		gain.market2Gain = Math.round(gain.traderGain * internationalGain2 / 100);
		gain.market2Owner = ownerSecondMarket;

	}

	return gain;
}

Engine.RegisterGlobal("CalculateTraderGain", CalculateTraderGain);
