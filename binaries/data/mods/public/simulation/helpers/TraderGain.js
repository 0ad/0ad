function CalculateTraderGain(firstMarket, secondMarket, traderTemplate, trader)
{
	let cmpMarket1 = QueryMiragedInterface(firstMarket, IID_Market);
	let cmpMarket2 = QueryMiragedInterface(secondMarket, IID_Market);
	if (!cmpMarket1 || !cmpMarket2)
		return null;

	let cmpMarket1Player = QueryOwnerInterface(firstMarket);
	let cmpMarket2Player = QueryOwnerInterface(secondMarket);
	if (!cmpMarket1Player || !cmpMarket2Player)
		return null;

	let cmpFirstMarketPosition = Engine.QueryInterface(firstMarket, IID_Position);
	let cmpSecondMarketPosition = Engine.QueryInterface(secondMarket, IID_Position);
	if (!cmpFirstMarketPosition || !cmpFirstMarketPosition.IsInWorld() ||
	   !cmpSecondMarketPosition || !cmpSecondMarketPosition.IsInWorld())
		return null;
	let firstMarketPosition = cmpFirstMarketPosition.GetPosition2D();
	let secondMarketPosition = cmpSecondMarketPosition.GetPosition2D();

	let gainMultiplier = TradeGainNormalization(Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain).GetMapSize());
	if (trader)
	{
		let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
		if (!cmpTrader)
			return null;
		gainMultiplier *= cmpTrader.GetTraderGainMultiplier();
	}
	else	//called from the gui, modifications already applied
	{
		if (!traderTemplate || !traderTemplate.GainMultiplier)
			return null;
		gainMultiplier *= traderTemplate.GainMultiplier;
	}

	let gain = {};

	// Calculate ordinary Euclidean distance between markets.
	// We don't use pathfinder, because ordinary distance looks more fair.
	let distanceSq = firstMarketPosition.distanceToSquared(secondMarketPosition);
	// We calculate gain as square of distance to encourage trading between remote markets
	// and gainMultiplier corresponds to the gain for a 100m distance
	gain.traderGain = gainMultiplier * NormalizedTradeGain(distanceSq);

	gain.market1Owner = cmpMarket1Player.GetPlayerID();
	gain.market2Owner = cmpMarket2Player.GetPlayerID();
	// If trader undefined, the trader owner is supposed to be the same as the first market
	let cmpPlayer = trader ? QueryOwnerInterface(trader) : cmpMarket1Player;
	if (!cmpPlayer)
		return null;
	gain.traderOwner = cmpPlayer.GetPlayerID();
	// Add potential player trade multipliers
	let playerBonus = cmpPlayer.GetTradeRateMultiplier();
	// If markets belong to different players, add gain from international trading
	if (gain.market1Owner != gain.market2Owner)
	{
		let market1PlayerBonus = cmpMarket1Player.GetTradeRateMultiplier();
		let market2PlayerBonus = cmpMarket2Player.GetTradeRateMultiplier();
		let internationalBonus1 = cmpMarket1.GetInternationalBonus();
		let internationalBonus2 = cmpMarket2.GetInternationalBonus();
		gain.market1Gain = Math.round(gain.traderGain * internationalBonus1 * market1PlayerBonus);
		gain.market2Gain = Math.round(gain.traderGain * internationalBonus2 * market2PlayerBonus);
	}
	gain.traderGain = Math.round(gain.traderGain * playerBonus);

	return gain;
}

Engine.RegisterGlobal("CalculateTraderGain", CalculateTraderGain);
