// This constant used to adjust gain value depending on distance
const DISTANCE_FACTOR = 1 / 50;

// Additional gain for trading performed between markets of different players, in percents
const INTERNATIONAL_TRADING_ADDITION = 50;
// Additional gain for ships for each garrisoned trader, in percents
const GARRISONED_TRADER_ADDITION = 20;

// Array of resource names
const RESOURCES = ["food", "wood", "stone", "metal"];

function Trader() {}

Trader.prototype.Schema =
	"<a:help>Lets the unit generate resouces while moving between markets (or docks in case of water trading).</a:help>" +
	"<a:example>" +
		"<MaxDistance>2.0</MaxDistance>" +
		"<GainMultiplier>1.0</GainMultiplier>" +
	"</a:example>" +
	"<element name='MaxDistance' a:help='Max distance from market when performing deal'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='GainMultiplier' a:help='Additional gain multiplier'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

Trader.prototype.Init = function()
{
	this.firstMarket = INVALID_ENTITY;
	this.secondMarket = INVALID_ENTITY;
	// Gain from one pass between markets
	this.gain = null;
	// Selected resource for trading
	this.preferredGoods = "metal";
	// Currently carried goods
	this.goods = { "type": null, "amount": 0 };
}

Trader.prototype.CalculateGain = function(firstMarket, secondMarket)
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

	// For ship increase gain for each garrisoned trader
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity.HasClass("Ship"))
	{
		var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
		{
			var garrisonedTradersCount = 0;
			for each (var entity in cmpGarrisonHolder.GetEntities())
			{
				var cmpGarrisonedUnitTrader = Engine.QueryInterface(entity, IID_Trader);
				if (cmpGarrisonedUnitTrader)
					garrisonedTradersCount++;
			}
			gain *= 1 + GARRISONED_TRADER_ADDITION * garrisonedTradersCount / 100;
		}
	}

	if (this.template.GainMultiplier)
		gain *= this.template.GainMultiplier;
	gain = Math.round(gain);
	return gain;
}

Trader.prototype.GetGain = function()
{
	return this.gain;
}

// Set target as target market.
// Return true if at least one of markets was changed.
Trader.prototype.SetTargetMarket = function(target)
{
	// Check that target is a market
	var cmpTargetIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpTargetIdentity)
		return false;
	if (!cmpTargetIdentity.HasClass("Market") && !cmpTargetIdentity.HasClass("NavalMarket"))
		return false;
	var marketsChanged = false;
	if (this.secondMarket)
	{
		// If we already have both markets - drop them
		// and use the target as first market
		this.firstMarket = target;
		this.secondMarket = INVALID_ENTITY;
		marketsChanged = true;
	}
	else if (this.firstMarket)
	{
		// If we have only one market and target is different from it,
		// set the target as second one
		if (target != this.firstMarket)
		{
			this.secondMarket = target;
			this.gain = this.CalculateGain(this.firstMarket, this.secondMarket);
			marketsChanged = true;
		}
	}
	else
	{
		// Else we don't have target markets at all,
		// set the target as first market
		this.firstMarket = target;
		marketsChanged = true;
	}
	if (marketsChanged)
	{
		// Drop carried goods
		this.goods.amount = 0;
	}
	return marketsChanged;
}

Trader.prototype.GetFirstMarket = function()
{
	return this.firstMarket;
}

Trader.prototype.GetSecondMarket = function()
{
	return this.secondMarket;
}

Trader.prototype.HasBothMarkets = function()
{
	return this.firstMarket && this.secondMarket;
}

Trader.prototype.GetPreferredGoods = function()
{
	return this.preferredGoods;
}

Trader.prototype.SetPreferredGoods = function(preferredGoods)
{
	// Check that argument is a correct resource name
	if (RESOURCES.indexOf(preferredGoods) == -1)
		return;
	this.preferredGoods = preferredGoods;
}

Trader.prototype.CanTrade = function(target)
{
	var cmpTraderIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	var cmpTargetIdentity = Engine.QueryInterface(target, IID_Identity);
	// Check that the target exists
	if (!cmpTargetIdentity)
		return false;
	var landTradingPossible = cmpTraderIdentity.HasClass("Organic") && cmpTargetIdentity.HasClass("Market");
	var seaTradingPossible = cmpTraderIdentity.HasClass("Ship") && cmpTargetIdentity.HasClass("NavalMarket");
	if (!landTradingPossible && !seaTradingPossible)
		return false;

	var cmpTraderPlayer = QueryOwnerInterface(this.entity, IID_Player);
	var traderPlayerId = cmpTraderPlayer.GetPlayerID();
	var cmpTargetPlayer = QueryOwnerInterface(target, IID_Player);
	var targetPlayerId = cmpTargetPlayer.GetPlayerID();
	var ownershipSuitableForTrading = (traderPlayerId == targetPlayerId) || cmpTraderPlayer.IsAlly(targetPlayerId);
	if (!ownershipSuitableForTrading)
		return false;
	return true;
}

Trader.prototype.PerformTrade = function()
{
	if (this.goods.amount > 0)
	{
		var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
		cmpPlayer.AddResource(this.goods.type, this.goods.amount);
	}
	this.goods.type = this.preferredGoods;
	this.goods.amount = this.gain;
}

Trader.prototype.GetGoods = function()
{
	return this.goods;
}

Trader.prototype.StopTrading = function()
{
	// Drop carried goods
	this.goods.amount = 0;
	// Reset markets
	this.firstMarket = INVALID_ENTITY;
	this.secondMarket = INVALID_ENTITY;
}

// Get range in which deals with market are available,
// i.e. trader should be in no more than MaxDistance from market
// to be able to trade with it.
Trader.prototype.GetRange = function()
{
	return { "min": 0, "max": +this.template.MaxDistance };
}

Engine.RegisterComponentType(IID_Trader, "Trader", Trader);

