// True price of 100 units of resource (for case if some resource is more worth).
// With current bartering system only relative values makes sense
// so if for example stone is two times more expensive than wood,
// there will 2:1 exchange rate.
const TRUE_PRICES = { "food": 100, "wood": 100, "stone": 100, "metal": 100 };

// Constant part of price difference between true price and buy/sell price.
// In percents.
// Buy price equal to true price plus constant difference.
// Sell price equal to true price minus constant difference.
const CONSTANT_DIFFERENCE = 10;

// Additional difference of prices, added after each deal to specified resource price.
// In percents.
const DIFFERENCE_PER_DEAL = 2;

// Price difference which restored each restore timer tick
// In percents.
const DIFFERENCE_RESTORE = 0.5;

// Interval of timer which slowly restore prices after deals
const RESTORE_TIMER_INTERVAL = 5000;

// Array of resource names
const RESOURCES = ["food", "wood", "stone", "metal"];

function Barter() {}

Barter.prototype.Schema =
	"<a:component type='system'/><empty/>";

Barter.prototype.Init = function()
{
	this.priceDifferences = {};
	for each (var resource in RESOURCES)
		this.priceDifferences[resource] = 0;
	this.restoreTimer = undefined;
};

Barter.prototype.GetPrices = function()
{
	var prices = { "buy": {}, "sell": {} };
	for each (var resource in RESOURCES)
	{
		prices["buy"][resource] = TRUE_PRICES[resource] * (100 + CONSTANT_DIFFERENCE + this.priceDifferences[resource]) / 100;
		prices["sell"][resource] = TRUE_PRICES[resource] * (100 - CONSTANT_DIFFERENCE + this.priceDifferences[resource]) / 100;
	}
	return prices;
};

Barter.prototype.PlayerHasMarket = function(playerEntity)
{
	var cmpPlayer = Engine.QueryInterface(playerEntity, IID_Player);
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var entities = cmpRangeManager.GetEntitiesByPlayer(cmpPlayer.GetPlayerID());
	for each (var entity in entities)
	{
		var cmpFoundation = Engine.QueryInterface(entity, IID_Foundation);
		var cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
		if (!cmpFoundation && cmpIdentity && cmpIdentity.HasClass("BarterMarket"))
			return true;
	}
	return false;
}

Barter.prototype.ExchangeResources = function(playerEntity, resourceToSell, resourceToBuy, amount)
{
	// Data verification
	if (amount <= 0)
	{
		warn("ExchangeResources: incorrect amount: " + uneval(amount));
		return;
	}
	if (RESOURCES.indexOf(resourceToSell) == -1)
	{
		warn("ExchangeResources: incorrect resource to sell: " + uneval(resourceToSell));
		return;
	}
	if (RESOURCES.indexOf(resourceToBuy) == -1)
	{
		warn("ExchangeResources: incorrect resource to buy: " + uneval(resourceToBuy));
		return;
	}
	if (!this.PlayerHasMarket(playerEntity))
	{
		warn("ExchangeResources: player has no markets");
		return;
	}

	var cmpPlayer = Engine.QueryInterface(playerEntity, IID_Player);
	var prices = this.GetPrices();
	var amountsToSubtract = {};
	amountsToSubtract[resourceToSell] = amount;
	if (cmpPlayer.TrySubtractResources(amountsToSubtract))
	{
		var amountToAdd = Math.round(prices["sell"][resourceToSell] / prices["buy"][resourceToBuy] * amount);
		cmpPlayer.AddResource(resourceToBuy, amountToAdd);
		var numberOfDeals = Math.round(amount / 100);

		var cmpStatisticsTracker = Engine.QueryInterface(playerEntity, IID_StatisticsTracker);
		if (cmpStatisticsTracker)
		{
			cmpStatisticsTracker.IncreaseResourcesSoldCounter(resourceToSell, amount);
			cmpStatisticsTracker.IncreaseResourcesBoughtCounter(resourceToBuy, amountToAdd);
		}

		// Increase price difference for both exchange resources.
		// Overal price difference (constant + dynamic) can't exceed +-99%
		// so both buy/sell prices limited to [1%; 199%] interval.
		this.priceDifferences[resourceToSell] -= DIFFERENCE_PER_DEAL * numberOfDeals;
		this.priceDifferences[resourceToSell] = Math.min(99-CONSTANT_DIFFERENCE, Math.max(CONSTANT_DIFFERENCE-99, this.priceDifferences[resourceToSell]));
		this.priceDifferences[resourceToBuy] += DIFFERENCE_PER_DEAL * numberOfDeals;
		this.priceDifferences[resourceToBuy] = Math.min(99-CONSTANT_DIFFERENCE, Math.max(CONSTANT_DIFFERENCE-99, this.priceDifferences[resourceToBuy]));
	}

	if (this.restoreTimer === undefined)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.restoreTimer = cmpTimer.SetInterval(this.entity, IID_Barter, "ProgressTimeout", RESTORE_TIMER_INTERVAL, RESTORE_TIMER_INTERVAL, {});
	}
};

Barter.prototype.ProgressTimeout = function(data)
{
	var needRestore = false;
	for each (var resource in RESOURCES)
	{
		// Calculate value to restore, it should be limited to [-DIFFERENCE_RESTORE; DIFFERENCE_RESTORE] interval
		var differenceRestore = Math.min(DIFFERENCE_RESTORE, Math.max(-DIFFERENCE_RESTORE, this.priceDifferences[resource]));
		differenceRestore = -differenceRestore;
		this.priceDifferences[resource] += differenceRestore;
		// If price difference still exists then set flag to run timer again
		if (this.priceDifferences[resource] != 0)
			needRestore = true;
	}

	if (!needRestore)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.restoreTimer);
		this.restoreTimer = undefined;
	}
}

Engine.RegisterSystemComponentType(IID_Barter, "Barter", Barter);

