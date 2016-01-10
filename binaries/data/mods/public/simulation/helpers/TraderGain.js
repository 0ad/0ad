// This constant used to adjust gain value depending on distance
const DISTANCE_FACTOR = 1 / 115;

// Additional gain (applying to each market) for trading performed between markets of different players, in percents
const INTERNATIONAL_TRADING_ADDITION = 25;

// If trader undefined, the trader owner is supposed to be the same as the first market
function CalculateTraderGain(firstMarket, secondMarket, template, trader)
{
	var gain = {};

	var cmpFirstMarketPosition = Engine.QueryInterface(firstMarket, IID_Position);
	var cmpSecondMarketPosition = Engine.QueryInterface(secondMarket, IID_Position);
	if (!cmpFirstMarketPosition || !cmpFirstMarketPosition.IsInWorld() ||
	    !cmpSecondMarketPosition || !cmpSecondMarketPosition.IsInWorld())
		return null;
	var firstMarketPosition = cmpFirstMarketPosition.GetPosition2D();
	var secondMarketPosition = cmpSecondMarketPosition.GetPosition2D();

	// Calculate ordinary Euclidean distance between markets.
	// We don't use pathfinder, because ordinary distance looks more fair.
	var distance = firstMarketPosition.distanceTo(secondMarketPosition);
	// We calculate gain as square of distance to encourage trading between remote markets
	gain.traderGain = Math.pow(distance * DISTANCE_FACTOR, 2);
	if (template && template.GainMultiplier)
	{
		if (trader)
			gain.traderGain *= ApplyValueModificationsToEntity("Trader/GainMultiplier", +template.GainMultiplier, trader);
		else	// called from the gui with modifications already applied
			gain.traderGain *= template.GainMultiplier;
	}
	// If trader undefined, the trader owner is supposed to be the same as the first market
	var cmpOwnership = trader ? Engine.QueryInterface(trader, IID_Ownership) : Engine.QueryInterface(firstMarket, IID_Ownership);
	if (!cmpOwnership)
		return null;
	gain.traderOwner = cmpOwnership.GetOwner();

	// If markets belong to different players, add gain from international trading
	var ownerFirstMarket = Engine.QueryInterface(firstMarket, IID_Ownership).GetOwner();
	var ownerSecondMarket = Engine.QueryInterface(secondMarket, IID_Ownership).GetOwner();
	if (ownerFirstMarket != ownerSecondMarket)
	{
		gain.market1Gain = gain.traderGain * ApplyValueModificationsToEntity("Trade/International", INTERNATIONAL_TRADING_ADDITION, firstMarket) / 100;
		gain.market1Owner = ownerFirstMarket;
		gain.market2Gain = gain.traderGain * ApplyValueModificationsToEntity("Trade/International", INTERNATIONAL_TRADING_ADDITION, secondMarket) / 100;
		gain.market2Owner = ownerSecondMarket;
	}

	// Add potential trade multipliers and roundings
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(gain.traderOwner), IID_Player);
	if (cmpPlayer)
		gain.traderGain *= cmpPlayer.GetTradeRateMultiplier();
	gain.traderGain = Math.round(gain.traderGain);

	if (ownerFirstMarket != ownerSecondMarket)
	{
		if ((cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(gain.market1Owner), IID_Player)))
			gain.market1Gain *= cmpPlayer.GetTradeRateMultiplier();
		gain.market1Gain = Math.round(gain.market1Gain);

		if ((cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(gain.market2Owner), IID_Player)))
			gain.market2Gain *= cmpPlayer.GetTradeRateMultiplier();
		gain.market2Gain = Math.round(gain.market2Gain);
	}

	return gain;
}

Engine.RegisterGlobal("CalculateTraderGain", CalculateTraderGain);
