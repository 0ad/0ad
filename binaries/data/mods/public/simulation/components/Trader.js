// See helpers/TraderGain.js for the CalculateTaderGain() function which works out how many
// resources a trader gets

function Trader() {}

Trader.prototype.Schema =
	"<a:help>Lets the unit generate resources while moving between markets (or docks in case of water trading).</a:help>" +
	"<a:example>" +
		"<GainMultiplier>0.75</GainMultiplier>" +
		"<GarrisonGainMultiplier>0.2</GarrisonGainMultiplier>" +
	"</a:example>" +
	"<element name='GainMultiplier' a:help='Trader gain for a 100m distance and mapSize = 1024'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='GarrisonGainMultiplier' a:help='Additional gain for garrisonable unit for each garrisoned trader (1.0 means 100%)'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>";

Trader.prototype.Init = function()
{
	this.markets = [];
	this.index = -1;
	this.goods = {
		"type": null,
		"amount": null
	};
};

Trader.prototype.CalculateGain = function(currentMarket, nextMarket)
{
	let cmpMarket = QueryMiragedInterface(currentMarket, IID_Market);
	let gain = cmpMarket && cmpMarket.CalculateTraderGain(nextMarket, this.template, this.entity);
	if (!gain)	// One of our markets must have been destroyed
		return null;

	// For garrisonable unit increase gain for each garrisoned trader
	// Calculate this here to save passing unnecessary stuff into the CalculateTraderGain function
	let garrisonGainMultiplier = this.GetGarrisonGainMultiplier();
	if (garrisonGainMultiplier === undefined)
		return gain;

	let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
		return gain;

	let garrisonMultiplier = 1;
	let garrisonedTradersCount = 0;
	for (let entity of cmpGarrisonHolder.GetEntities())
	{
		let cmpGarrisonedUnitTrader = Engine.QueryInterface(entity, IID_Trader);
		if (cmpGarrisonedUnitTrader)
			++garrisonedTradersCount;
	}
	garrisonMultiplier *= 1 + garrisonGainMultiplier * garrisonedTradersCount;

	if (gain.traderGain)
		gain.traderGain = Math.round(garrisonMultiplier * gain.traderGain);
	if (gain.market1Gain)
		gain.market1Gain = Math.round(garrisonMultiplier * gain.market1Gain);
	if (gain.market2Gain)
		gain.market2Gain = Math.round(garrisonMultiplier * gain.market2Gain);

	return gain;
};

/**
 * Remove market from trade route iff only first market is set.
 * @param {number} id of market to be removed.
 * @return {boolean} true iff removal was successful.
 */
Trader.prototype.RemoveTargetMarket = function(target)
{
	if (this.markets.length != 1 || this.markets[0] != target)
		return false;
	let cmpTargetMarket = QueryMiragedInterface(target, IID_Market);
	if (!cmpTargetMarket)
		return false;
	cmpTargetMarket.RemoveTrader(this.entity);
	this.index = -1;
	this.markets = [];
	return true;
};

// Set target as target market.
// Return true if at least one of markets was changed.
Trader.prototype.SetTargetMarket = function(target, source)
{
	let cmpTargetMarket = QueryMiragedInterface(target, IID_Market);
	if (!cmpTargetMarket)
		return false;

	if (source)
	{
		// Establish a trade route with both markets in one go.
		let cmpSourceMarket = QueryMiragedInterface(source, IID_Market);
		if (!cmpSourceMarket)
			return false;
		this.markets = [source];
	}
	if (this.markets.length >= 2)
	{
		// If we already have both markets - drop them
		// and use the target as first market
		for (let market of this.markets)
		{
			let cmpMarket = QueryMiragedInterface(market, IID_Market);
			if (cmpMarket)
				cmpMarket.RemoveTrader(this.entity);
		}
		this.index = 0;
		this.markets = [target];
		cmpTargetMarket.AddTrader(this.entity);
	}
	else if (this.markets.length == 1)
	{
		// If we have only one market and target is different from it,
		// set the target as second one
		if (target == this.markets[0])
			return false;

		this.index = 0;
		this.markets.push(target);
		cmpTargetMarket.AddTrader(this.entity);
		this.goods.amount = this.CalculateGain(this.markets[0], this.markets[1]);
	}
	else
	{
		// Else we don't have target markets at all,
		// set the target as first market
		this.index = 0;
		this.markets = [target];
		cmpTargetMarket.AddTrader(this.entity);
	}
	// Drop carried goods if markets were changed
	this.goods.amount = null;
	return true;
};

Trader.prototype.GetFirstMarket = function()
{
	return this.markets[0] || null;
};

Trader.prototype.GetSecondMarket = function()
{
	return this.markets[1] || null;
};

Trader.prototype.GetTraderGainMultiplier = function()
{
	return ApplyValueModificationsToEntity("Trader/GainMultiplier", +this.template.GainMultiplier, this.entity);
};

Trader.prototype.GetGarrisonGainMultiplier = function()
{
	if (this.template.GarrisonGainMultiplier === undefined)
		return undefined;
	return ApplyValueModificationsToEntity("Trader/GarrisonGainMultiplier", +this.template.GarrisonGainMultiplier, this.entity);
};

Trader.prototype.HasBothMarkets = function()
{
	return this.markets.length >= 2;
};

Trader.prototype.CanTrade = function(target)
{
	let cmpTraderIdentity = Engine.QueryInterface(this.entity, IID_Identity);

	let cmpTargetMarket = QueryMiragedInterface(target, IID_Market);
	if (!cmpTargetMarket)
		return false;

	let cmpTargetFoundation = Engine.QueryInterface(target, IID_Foundation);
	if (cmpTargetFoundation)
		return false;

	if (!(cmpTraderIdentity.HasClass("Organic") && cmpTargetMarket.HasType("land")) &&
		!(cmpTraderIdentity.HasClass("Ship") && cmpTargetMarket.HasType("naval")))
		return false;

	let cmpTraderPlayer = QueryOwnerInterface(this.entity, IID_Player);
	let cmpTargetPlayer = QueryOwnerInterface(target, IID_Player);

	return cmpTraderPlayer && cmpTargetPlayer && !cmpTraderPlayer.IsEnemy(cmpTargetPlayer.GetPlayerID());
};

Trader.prototype.AddResources = function(ent, gain)
{
	let cmpPlayer = QueryOwnerInterface(ent);
	if (cmpPlayer)
		cmpPlayer.AddResource(this.goods.type, gain);

	let cmpStatisticsTracker = QueryOwnerInterface(ent, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseTradeIncomeCounter(gain);
};

Trader.prototype.GenerateResources = function(currentMarket, nextMarket)
{
	this.AddResources(this.entity, this.goods.amount.traderGain);

	if (this.goods.amount.market1Gain)
		this.AddResources(currentMarket, this.goods.amount.market1Gain);

	if (this.goods.amount.market2Gain)
		this.AddResources(nextMarket, this.goods.amount.market2Gain);
};

Trader.prototype.PerformTrade = function(currentMarket)
{
	let previousMarket = this.markets[this.index];
	if (previousMarket != currentMarket)  // Inconsistent markets
	{
		this.goods.amount = null;
		return INVALID_ENTITY;
	}

	this.index = ++this.index % this.markets.length;
	let nextMarket = this.markets[this.index];

	if (this.goods.amount && this.goods.amount.traderGain)
		this.GenerateResources(previousMarket, nextMarket);

	let cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return INVALID_ENTITY;

	this.goods.type = cmpPlayer.GetNextTradingGoods();
	this.goods.amount = this.CalculateGain(currentMarket, nextMarket);

	return nextMarket;
};

Trader.prototype.GetGoods = function()
{
	return this.goods;
};

/**
 * Returns true if the trader has the given market (can be either a market or a mirage)
 */
Trader.prototype.HasMarket = function(market)
{
	return this.markets.indexOf(market) != -1;
};

/**
 * Remove a market when this trader can no longer trade with it
 */
Trader.prototype.RemoveMarket = function(market)
{
	let index = this.markets.indexOf(market);
	if (index == -1)
		return;
	this.markets.splice(index, 1);
	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI)
		cmpUnitAI.MarketRemoved(market);
};

/**
 * Switch between a market and its mirage according to visibility
 */
Trader.prototype.SwitchMarket = function(oldMarket, newMarket)
{
	let index = this.markets.indexOf(oldMarket);
	if (index == -1)
		return;
	this.markets[index] = newMarket;
	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI)
		cmpUnitAI.SwitchMarketOrder(oldMarket, newMarket);
};

Trader.prototype.StopTrading = function()
{
	for (let market of this.markets)
	{
		let cmpMarket = QueryMiragedInterface(market, IID_Market);
		if (cmpMarket)
			cmpMarket.RemoveTrader(this.entity);
	}
	this.index = -1;
	this.markets = [];
	this.goods.amount = null;
	this.markets = [];
};

// Get range in which deals with market are available,
// i.e. trader should be in no more than MaxDistance from market
// to be able to trade with it.
Trader.prototype.GetRange = function()
{
	let cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	let max = 1;
	if (cmpObstruction)
		max += cmpObstruction.GetSize() * 1.5;
	return { "min": 0, "max": max };
};

Trader.prototype.OnGarrisonedUnitsChanged = function()
{
	if (this.HasBothMarkets())
		this.goods.amount = this.CalculateGain(this.markets[0], this.markets[1]);
};

Engine.RegisterComponentType(IID_Trader, "Trader", Trader);
