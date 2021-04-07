function Market() {}

Market.prototype.Schema =
	"<element name='TradeType' a:help='Specifies the type of possible trade route (land or naval).'>" +
		"<list>" +
			"<oneOrMore>" +
				"<choice>" +
					"<value>land</value>" +
					"<value>naval</value>" +
				"</choice>" +
			"</oneOrMore>" +
		"</list>" +
	"</element>" +
	"<element name='InternationalBonus' a:help='Additional part of the gain donated when two different players trade'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

Market.prototype.Init = function()
{
	this.traders = new Set();	// list of traders with a route on this market
	this.tradeType = new Set(this.template.TradeType.split(/\s+/));
};

Market.prototype.AddTrader = function(ent)
{
	this.traders.add(ent);
};

Market.prototype.RemoveTrader = function(ent)
{
	this.traders.delete(ent);
};

Market.prototype.GetInternationalBonus = function()
{
	return ApplyValueModificationsToEntity("Market/InternationalBonus", +this.template.InternationalBonus, this.entity);
};

Market.prototype.HasType = function(type)
{
	return this.tradeType.has(type);
};

Market.prototype.GetType = function()
{
	return this.tradeType;
};

Market.prototype.GetTraders = function()
{
	return this.traders;
};

/**
 * Check if the traders attached to this market can still trade with it
 * Warning: traders currently trading with a mirage of this market are dealt with in Mirage.js
 */

Market.prototype.UpdateTraders = function(onDestruction)
{
	for (let trader of this.traders)
	{
		let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
		if (!cmpTrader)
		{
			this.RemoveTrader(trader);
			continue;
		}
		if (!cmpTrader.HasMarket(this.entity) || !onDestruction && cmpTrader.CanTrade(this.entity))
			continue;
		// this trader can no more trade
		this.RemoveTrader(trader);
		cmpTrader.RemoveMarket(this.entity);
	}
};

Market.prototype.CalculateTraderGain = function(secondMarket, traderTemplate, trader)
{
	let cmpMarket2 = QueryMiragedInterface(secondMarket, IID_Market);
	if (!cmpMarket2)
		return null;

	let cmpMarket1Player = QueryOwnerInterface(this.entity);
	let cmpMarket2Player = QueryOwnerInterface(secondMarket);
	if (!cmpMarket1Player || !cmpMarket2Player)
		return null;

	let cmpFirstMarketPosition = Engine.QueryInterface(this.entity, IID_Position);
	let cmpSecondMarketPosition = Engine.QueryInterface(secondMarket, IID_Position);
	if (!cmpFirstMarketPosition || !cmpFirstMarketPosition.IsInWorld() ||
	   !cmpSecondMarketPosition || !cmpSecondMarketPosition.IsInWorld())
		return null;
	let firstMarketPosition = cmpFirstMarketPosition.GetPosition2D();
	let secondMarketPosition = cmpSecondMarketPosition.GetPosition2D();

	let mapSize = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain).GetMapSize();
	let gainMultiplier = TradeGainNormalization(mapSize);
	if (trader)
	{
		let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
		if (!cmpTrader)
			return null;
		gainMultiplier *= cmpTrader.GetTraderGainMultiplier();
	}
	// Called from the gui, modifications already applied.
	else
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
	gain.traderGain = Math.round(gainMultiplier * TradeGain(distanceSq, mapSize));

	gain.market1Owner = cmpMarket1Player.GetPlayerID();
	gain.market2Owner = cmpMarket2Player.GetPlayerID();
	// If trader undefined, the trader owner is supposed to be the same as the first market.
	let cmpPlayer = trader ? QueryOwnerInterface(trader) : cmpMarket1Player;
	if (!cmpPlayer)
		return null;
	gain.traderOwner = cmpPlayer.GetPlayerID();

	if (gain.market1Owner != gain.market2Owner)
	{
		let internationalBonus1 = this.GetInternationalBonus();
		let internationalBonus2 = cmpMarket2.GetInternationalBonus();
		gain.market1Gain = Math.round(gain.traderGain * internationalBonus1);
		gain.market2Gain = Math.round(gain.traderGain * internationalBonus2);
	}

	return gain;
};

Market.prototype.OnDiplomacyChanged = function(msg)
{
	this.UpdateTraders(false);
};

Market.prototype.OnOwnershipChanged = function(msg)
{
	this.UpdateTraders(msg.to == INVALID_PLAYER);
};

function MarketMirage() {}
MarketMirage.prototype.Init = function(cmpMarket, entity, parent, player)
{
	this.entity = entity;
	this.parent = parent;
	this.player = player;

	this.traders = new Set();
	for (let trader of cmpMarket.GetTraders())
	{
		let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
		let cmpOwnership = Engine.QueryInterface(trader, IID_Ownership);
		if (!cmpTrader || !cmpOwnership)
		{
			cmpMarket.RemoveTrader(trader);
			continue;
		}
		if (this.player != cmpOwnership.GetOwner())
			continue;
		cmpTrader.SwitchMarket(cmpMarket.entity, this.entity);
		cmpMarket.RemoveTrader(trader);
		this.AddTrader(trader);
	}
	this.marketType = cmpMarket.GetType();
	this.internationalBonus = cmpMarket.GetInternationalBonus();
};

MarketMirage.prototype.HasType = function(type) { return this.marketType.has(type); };
MarketMirage.prototype.GetInternationalBonus = function() { return this.internationalBonus; };
MarketMirage.prototype.AddTrader = function(trader) { this.traders.add(trader); };
MarketMirage.prototype.RemoveTrader = function(trader) { this.traders.delete(trader); };

MarketMirage.prototype.UpdateTraders = function(msg)
{
	let cmpMarket = Engine.QueryInterface(this.parent, IID_Market);
	if (!cmpMarket)	// The parent market does not exist anymore
	{
		for (let trader of this.traders)
		{
			let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
			if (cmpTrader)
				cmpTrader.RemoveMarket(this.entity);
		}
		return;
	}

	// The market becomes visible, switch all traders from the mirage to the market
	for (let trader of this.traders)
	{
		let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
		if (!cmpTrader)
			continue;
		cmpTrader.SwitchMarket(this.entity, cmpMarket.entity);
		this.RemoveTrader(trader);
		cmpMarket.AddTrader(trader);
	}
};

MarketMirage.prototype.CalculateTraderGain = Market.prototype.CalculateTraderGain;

Engine.RegisterGlobal("MarketMirage", MarketMirage);

Market.prototype.Mirage = function(mirageID, miragePlayer)
{
	let mirage = new MarketMirage();
	mirage.Init(this, mirageID, this.entity, miragePlayer);
	return mirage;
};

Engine.RegisterComponentType(IID_Market, "Market", Market);
