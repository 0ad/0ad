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

/**
 * Check if all traders with a route on this market can still trade
 */
Market.prototype.OnDiplomacyChanged = function(msg)
{
	for (let ent of this.traders)
	{
		let cmpTrader = Engine.QueryInterface(ent, IID_Trader);
		if (!cmpTrader)
			this.RemoveTrader(ent);
		else if (!cmpTrader.CanTrade(this.entity))
		{
			this.RemoveTrader(ent);
			cmpTrader.RemoveMarket(this.entity);
		}
	}
};

Market.prototype.OnOwnershipChanged = function(msg)
{
	for (let ent of this.traders)
	{
		let cmpTrader = Engine.QueryInterface(ent, IID_Trader);
		if (!cmpTrader)
			this.RemoveTrader(ent);
		else if (msg.to == -1)
			cmpTrader.RemoveMarket(this.entity);
		else if (!cmpTrader.CanTrade(this.entity))
		{
			this.RemoveTrader(ent);
			cmpTrader.RemoveMarket(this.entity);
		}
	}
};

Engine.RegisterComponentType(IID_Market, "Market", Market);
