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

Engine.RegisterGlobal("MarketMirage", MarketMirage);

Market.prototype.Mirage = function(mirageID, miragePlayer)
{
	let mirage = new MarketMirage();
	mirage.Init(this, mirageID, this.entity, miragePlayer);
	return mirage;
};

Engine.RegisterComponentType(IID_Market, "Market", Market);
