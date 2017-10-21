// See helpers/TraderGain.js for the CalculateTaderGain() function which works out how many
// resources a trader gets

// Additional gain for ships for each garrisoned trader, in percents
const GARRISONED_TRADER_ADDITION = 20;

function Trader() {}

Trader.prototype.Schema =
	"<a:help>Lets the unit generate resouces while moving between markets (or docks in case of water trading).</a:help>" +
	"<a:example>" +
		"<GainMultiplier>0.75</GainMultiplier>" +
	"</a:example>" +
	"<element name='GainMultiplier' a:help='Trader gain for a 100m distance and mapSize = 1024'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

Trader.prototype.Init = function()
{
	this.markets = [];
	this.index = -1;
	this.goods = {
		"type": null,
		"amount": null,
		"origin": null
	};
};

Trader.prototype.CalculateGain = function(currentMarket, nextMarket)
{
	let gain = CalculateTraderGain(currentMarket, nextMarket, this.template, this.entity);
	if (!gain)	// One of our markets must have been destroyed
		return null;

	// For ship increase gain for each garrisoned trader
	// Calculate this here to save passing unnecessary stuff into the CalculateTraderGain function
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity && cmpIdentity.HasClass("Ship"))
	{
		var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
		{
			var garrisonMultiplier = 1;
			var garrisonedTradersCount = 0;
			for (let entity of cmpGarrisonHolder.GetEntities())
			{
				var cmpGarrisonedUnitTrader = Engine.QueryInterface(entity, IID_Trader);
				if (cmpGarrisonedUnitTrader)
					garrisonedTradersCount++;
			}
			garrisonMultiplier *= 1 + GARRISONED_TRADER_ADDITION * garrisonedTradersCount / 100;

			if (gain.traderGain)
				gain.traderGain = Math.round(garrisonMultiplier * gain.traderGain);
			if (gain.market1Gain)
				gain.market1Gain = Math.round(garrisonMultiplier * gain.market1Gain);
			if (gain.market2Gain)
				gain.market2Gain = Math.round(garrisonMultiplier * gain.market2Gain);
		}
	}

	return gain;
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
		else
		{
			this.index = 0;
			this.markets.push(target);
			cmpTargetMarket.AddTrader(this.entity);
			this.goods.amount = this.CalculateGain(this.markets[0], this.markets[1]);
		}
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

Trader.prototype.HasBothMarkets = function()
{
	return this.markets.length >= 2;
};

Trader.prototype.CanTrade = function(target)
{
	var cmpTraderIdentity = Engine.QueryInterface(this.entity, IID_Identity);

	var cmpTargetMarket = QueryMiragedInterface(target, IID_Market);
	if (!cmpTargetMarket)
		return false;

	var cmpTargetFoundation = Engine.QueryInterface(target, IID_Foundation);
	if (cmpTargetFoundation)
		return false;

	if (!(cmpTraderIdentity.HasClass("Organic") && cmpTargetMarket.HasType("land")) &&
		!(cmpTraderIdentity.HasClass("Ship") && cmpTargetMarket.HasType("naval")))
		return false;

	var cmpTraderPlayer = QueryOwnerInterface(this.entity, IID_Player);
	var cmpTargetPlayer = QueryOwnerInterface(target, IID_Player);
	var targetPlayerId = cmpTargetPlayer.GetPlayerID();

	return !cmpTraderPlayer.IsEnemy(targetPlayerId);
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
	this.goods.origin = currentMarket;

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
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	var max = 1;
	if (cmpObstruction)
		max += cmpObstruction.GetUnitRadius()*1.5;
	return { "min": 0, "max": max};
};

Trader.prototype.OnGarrisonedUnitsChanged = function()
{
	if (this.HasBothMarkets())
		this.goods.amount = this.CalculateGain(this.markets[0], this.markets[1]);
};

Engine.RegisterComponentType(IID_Trader, "Trader", Trader);
