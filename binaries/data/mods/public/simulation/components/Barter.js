function Barter() {}

Barter.prototype.Schema =
	"<a:component type='system'/><empty/>";

/**
 * The "true price" is a base price of Barter.prototype.DEAL_AMOUNT units of resource (for the case of some resources being of more worth than others).
 * With current bartering system only relative values makes sense so if for example stone is two times more expensive than wood,
 * there will 2:1 exchange rate.
 *
 * Keep gui/session/trade/BarterButton.js in sync with this value.
 */
Barter.prototype.DEAL_AMOUNT = 100;

/**
 * Deals per mass barter.
 * Keep gui/session/trade/BarterButton.js in sync with this value.
 */
Barter.prototype.BATCH_SIZE = 5;

/**
 * Constant part of price percentage difference between true price and buy/sell price.
 * Buy price equal to true price plus constant difference.
 * Sell price equal to true price minus constant difference.
 */
Barter.prototype.CONSTANT_DIFFERENCE = 10;

/**
 * Additional difference of prices in percents, added after each deal to specified resource price.
 */
Barter.prototype.DIFFERENCE_PER_DEAL = 2;

/**
 * Price difference percentage which restored each restore timer tick
 */
Barter.prototype.DIFFERENCE_RESTORE = 0.5;

/**
 * Interval of timer which slowly restore prices after deals
 */
Barter.prototype.RESTORE_TIMER_INTERVAL = 5000;

Barter.prototype.Init = function()
{
	this.priceDifferences = {};
	for (const resource of Resources.GetBarterableCodes())
		this.priceDifferences[resource] = 0;
};

Barter.prototype.GetPrices = function(cmpPlayer)
{
	const prices = { "buy": {}, "sell": {} };
	const multiplier = cmpPlayer.GetBarterMultiplier();
	for (const resource in this.priceDifferences)
	{
		const truePrice = Resources.GetResource(resource).truePrice;
		prices.buy[resource] = truePrice * (this.DEAL_AMOUNT + this.CONSTANT_DIFFERENCE + this.priceDifferences[resource]) * multiplier.buy[resource] / this.DEAL_AMOUNT;
		prices.sell[resource] = truePrice * (this.DEAL_AMOUNT - this.CONSTANT_DIFFERENCE + this.priceDifferences[resource]) * multiplier.sell[resource] / this.DEAL_AMOUNT;
	}
	return prices;
};

Barter.prototype.ExchangeResources = function(playerID, resourceToSell, resourceToBuy, amount)
{
	if (amount <= 0)
	{
		warn("ExchangeResources: incorrect amount: " + uneval(amount));
		return;
	}

	if (!(resourceToSell in this.priceDifferences))
	{
		warn("ExchangeResources: incorrect resource to sell: " + uneval(resourceToSell));
		return;
	}

	if (!(resourceToBuy in this.priceDifferences))
	{
		warn("ExchangeResources: incorrect resource to buy: " + uneval(resourceToBuy));
		return;
	}

	if (amount !== this.DEAL_AMOUNT && amount !== (this.BATCH_SIZE * this.DEAL_AMOUNT))
		return;

	const cmpPlayer = QueryPlayerIDInterface(playerID);
	if (!cmpPlayer?.CanBarter())
		return;

	const amountsToSubtract = {
		[resourceToSell]: amount
	};
	if (!cmpPlayer.TrySubtractResources(amountsToSubtract))
		return;

	const prices = this.GetPrices(cmpPlayer);
	const amountToAdd = Math.round(prices.sell[resourceToSell] / prices.buy[resourceToBuy] * amount);
	cmpPlayer.AddResource(resourceToBuy, amountToAdd);

	Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface)?.PushNotification({
		"type": "barter",
		"players": [playerID],
		"amountGiven": amount,
		"amountGained": amountToAdd,
		"resourceGiven": resourceToSell,
		"resourceGained": resourceToBuy
	});

	const cmpStatisticsTracker = QueryPlayerIDInterface(playerID, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
	{
		cmpStatisticsTracker.IncreaseResourcesSoldCounter(resourceToSell, amount);
		cmpStatisticsTracker.IncreaseResourcesBoughtCounter(resourceToBuy, amountToAdd);
	}

	const difference = this.DIFFERENCE_PER_DEAL * amount / this.DEAL_AMOUNT;
	// Overall price difference (dynamic +/- constant) can't exceed +-99%.
	const maxDifference = this.DEAL_AMOUNT * 0.99;

	// Increase price difference for both exchanged resources.
	this.priceDifferences[resourceToSell] -= difference;
	this.priceDifferences[resourceToSell] = Math.min(maxDifference - this.CONSTANT_DIFFERENCE, Math.max(this.CONSTANT_DIFFERENCE - maxDifference, this.priceDifferences[resourceToSell]));
	this.priceDifferences[resourceToBuy] += difference;
	this.priceDifferences[resourceToBuy] = Math.min(maxDifference - this.CONSTANT_DIFFERENCE, Math.max(this.CONSTANT_DIFFERENCE - maxDifference, this.priceDifferences[resourceToBuy]));

	if (!this.restoreTimer)
		this.restoreTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).SetInterval(this.entity, IID_Barter, "ProgressTimeout", this.RESTORE_TIMER_INTERVAL, this.RESTORE_TIMER_INTERVAL, null);
};

Barter.prototype.ProgressTimeout = function(data)
{
	let needRestore = false;
	for (const resource in this.priceDifferences)
	{
		// Calculate value to restore, it should be limited to [-DIFFERENCE_RESTORE; DIFFERENCE_RESTORE] interval
		this.priceDifferences[resource] -= Math.min(this.DIFFERENCE_RESTORE, Math.max(-this.DIFFERENCE_RESTORE, this.priceDifferences[resource]));
		// If price difference still exists then set flag to keep the timer running.
		if (this.priceDifferences[resource] !== 0)
			needRestore = true;
	}

	if (!needRestore)
	{
		Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).CancelTimer(this.restoreTimer);
		delete this.restoreTimer;
	}
};

Engine.RegisterSystemComponentType(IID_Barter, "Barter", Barter);
