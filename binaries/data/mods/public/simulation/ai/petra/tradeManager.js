var PETRA = function(m)
{

/**
 * Manage the trade
 */

m.TradeManager = function(Config)
{
	this.Config = Config;
	this.tradeRoute = undefined;
	this.targetNumTraders = this.Config.Economy.targetNumTraders;
};

m.TradeManager.prototype.init = function(gameState)
{
	this.traders = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "role", "trader"));
	this.traders.allowQuickIter();
	this.traders.registerUpdates();
};

m.TradeManager.prototype.setTradeRoute = function(market1, market2)
{
	this.tradeRoute = { "source": market1, "target": market2 };
};

m.TradeManager.prototype.hasTradeRoute = function()
{
	return (this.tradeRoute !== undefined);
};

m.TradeManager.prototype.assignTrader = function(ent)
{
	unit.setMetadata(PlayerID, "role", "trader");
	this.traders.updateEnt(unit);
};

// TODO take trader ships into account
m.TradeManager.prototype.trainMoreTraders = function(gameState, queues)
{
	if (this.tradeRoute.source.hasClass("NavalMarket") && this.tradeRoute.target.hasClass("NavalMarket"))
		return;    // TODO naval trade not yet implemented

	if (!this.tradeRoute.source.hasClass("NavalMarket"))
		var base = this.tradeRoute.source.getMetadata(PlayerID, "base");
	else
		var base = this.tradeRoute.target.getMetadata(PlayerID, "base");

	var numTraders = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("units/{civ}_support_trader"), true);
	if (numTraders < this.targetNumTraders && queues.trader.countQueuedUnits() === 0)
	{
		var template = gameState.applyCiv("units/{civ}_support_trader")
		queues.trader.addItem(new m.TrainingPlan(gameState, template, { "role": "trader", "base": base }, 1, 1));
	}
};

// TODO deal with garrisoned trader & check if the trade route (i.e. its markets) still exist
m.TradeManager.prototype.updateTrader = function(ent)
{
	if (!ent.isIdle() || !ent.position())
		return;

	if (API3.SquareVectorDistance(this.tradeRoute.target.position(), ent.position()) > API3.SquareVectorDistance(this.tradeRoute.source.position(), ent.position()))
		ent.tradeRoute(this.tradeRoute.target, this.tradeRoute.source);
	else
		ent.tradeRoute(this.tradeRoute.source, this.tradeRoute.target);
};

// Try to setup trade routes  TODO complete it
// TODO use also docks (should be counted in Class("Market"), but may be build one when necessary
m.TradeManager.prototype.buildTradeRoute = function(gameState, queues)
{
	var filter = API3.Filters.and(API3.Filters.byClass("Market"), API3.Filters.not(API3.Filters.isFoundation()));
	var market1 = gameState.getOwnStructures().filter(filter).toEntityArray();
	var market2 = gameState.getAllyEntities().filter(filter).toEntityArray();
	if (market1.length + market2.length < 1)  // We have to wait  ... a first market will be built soon
		return false;

	var needed = 2;
	if (market2.length > 0)
		var needed = 1;
	if (market1.length < needed)
	{
		// TODO what to do if market1 is invalid ??? should not happen
		if (!market1[0] || !market1[0].position())
			return false;
		// We require at least two finished bases
		var nbase = 0;
		for each (var base in gameState.ai.HQ.baseManagers)
			if (base && !base.constructing)
				nbase++;
		if (nbase < 2)
			return false;
		if (queues.economicBuilding.countQueuedUnitsWithClass("Market") > 0 ||
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_market"), true) >= needed)
			return false;
		if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_market"))
			return false;
		// We have to build a second market ... try to put it as far as possible from the first one
		// we affect it temporarily to the farthest base, but the real placement will be done
		// in queueplan-building.js
		var marketBase =  market1[0].getMetadata(PlayerID, "base");
		var distmax = -1;
		var base = -1;
		for (var i in gameState.ai.HQ.baseManagers)
		{
			var baseManager = gameState.ai.HQ.baseManagers[i];
			if (marketBase === +i)
				continue;
			if (!baseManager.anchor || !baseManager.anchor.position())
				continue;
			var dist = API3.SquareVectorDistance(market1[0].position(), baseManager.anchor.position());
			if (dist < distmax)
				continue;
			distmax = dist;
			base = +i;
		}
		if (distmax > 0)
		{
			if (this.Config.debug > 1)
				warn(" a second market will be built in base " + base);
			// TODO build also docks when better
			queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_market", { "base": base }));
		}
		return false;
	}

	if (market2.length === 0)
		market2 = market1;
	var gainFactor = 1/(110*110);    // todo  take it directly from helpers/TraderGain.js
	var distmax = -1;
	var imax = -1;
	var jmax = -1;
	for each (var m1 in market1)
	{
		if (!m1.position())
			continue;
		var index1 = gameState.ai.accessibility.getAccessValue(m1.position());
		for each (var m2 in market2)
		{
			if (m1.id() === m2.id())
				continue;
			if (!m2.position())
				continue;
			var index2 = gameState.ai.accessibility.getAccessValue(m2.position());
			if (m2.hasClass("Dock") && m2.getMetadata(PlayerID, "sea") === undefined)
			{
				// m2 may-be an allied dock, without sea already affected to it
				var sea = gameState.ai.HQ.navalManager.getDockSeaIndex(gameState, m2);
				m2.setMetadata(PlayerID, "sea", sea);
			}
			if (index1 !== index2 && !(m1.hasClass("Dock") && m2.hasClass("Dock") && m1.getMetadata(PlayerID, "sea") === m2.getMetadata(PlayerID, "sea")))
				continue;
			var dist = API3.SquareVectorDistance(m1.position(), m2.position());
			if (dist < distmax)
				continue;
			distmax = dist;

			this.setTradeRoute(m1, m2, Math.round(distmax*gainFactor));
		}
	}
	if (distmax < 0)
	{
		if (this.Config.debug)
			warn("no trade route possible");
		return false;
	}
	if (this.Config.debug)
		warn("one trade route set with gain " + Math.round(distmax*gainFactor));
	return true;
};

m.TradeManager.prototype.setTradingGoods = function(gameState)
{
	var tradingGoods = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	// first, try to anticipate future needs 
	var stocks = gameState.ai.HQ.getTotalResourceLevel(gameState);
	var remaining = 100;
	this.targetNumTraders = this.Config.Economy.targetNumTraders;
	for (var type in stocks)
	{
		if (type == "food")
			continue;
		if (stocks[type] < 200)
		{
			tradingGoods[type] = 20;
			this.targetNumTraders += 2;
		}
		else if (stocks[type] < 500)
		{
			tradingGoods[type] = 10;
			this.targetNumTraders += 1;
		}
		remaining -= tradingGoods[type];
	}

	// then add what is needed now
	var mainNeed = Math.floor(remaining * 70 / 100)
	var nextNeed = remaining - mainNeed;

	var wantedRates = gameState.ai.queueManager.wantedGatherRates(gameState);
	var mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
	tradingGoods[mostNeeded[0]] += mainNeed;
	if (wantedRates[mostNeeded[1]] > 0)
		tradingGoods[mostNeeded[1]] += nextNeed;
	else
		tradingGoods[mostNeeded[0]] += nextNeed;
	Engine.PostCommand(PlayerID, {"type": "set-trading-goods", "tradingGoods": tradingGoods});
	if (this.Config.debug == 2)
		warn(" trading goods set to " + uneval(tradingGoods));
};

// Try to barter unneeded resources for needed resources.
// only once per turn because the info doesn't update between a turn and fixing isn't worth it.
m.TradeManager.prototype.performBarter = function(gameState)
{
	var markets = gameState.getOwnEntitiesByType(gameState.applyCiv("structures/{civ}_market"), true).toEntityArray();

	if (markets.length === 0)
		return false;

	// Available resources after account substraction
	var available = gameState.ai.queueManager.getAvailableResources(gameState);
	var needs = gameState.ai.queueManager.currentNeeds(gameState);

	var rates = gameState.ai.HQ.GetCurrentGatherRates(gameState)

	var prices = gameState.getBarterPrices();
	// calculates conversion rates
	var getBarterRate = function (prices,buy,sell) { return Math.round(100 * prices["sell"][sell] / prices["buy"][buy]); };

	// loop through each queues checking if we could barter and help finishing a queue quickly.
	for each (var buy in needs.types)
	{
		if (needs[buy] == 0 || needs[buy] < rates[buy]*30) // check if our rate allows to gather it fast enough
			continue;

		// pick the best resource to barter.
		var bestToSell = undefined;
		var bestRate = 0;
		for each (var sell in needs.types)
		{
			if (sell === buy)
				continue;
			if (needs[sell] > 0 || available[sell] < 500)    // do not sell if we need it or do not have enough buffer
				continue;

			if (sell === "food")
			{
				var barterRateMin = 30;
				if (available[sell] > 40000)
					barterRateMin = 0;
				else if (available[sell] > 15000)
					barterRateMin = 5;
				else if (available[sell] > 1000)
					barterRateMin = 10;
			}
			else 
			{
				var barterRateMin = 70;
				if (available[sell] > 1000)
					barterRateMin = 50;
				if (buy === "food")
					barterRateMin += 20;
			}

			var barterRate = getBarterRate(prices, buy, sell);
			if (barterRate > bestRate && barterRate > barterRateMin)
			{
				bestRate = barterRate;
				bestToSell = sell;
			}
		}
		if (bestToSell !== undefined)
		{
			markets[0].barter(buy, bestToSell, 100);
			if (this.Config.debug > 1)
				warn("Necessity bartering: sold " + bestToSell +" for " + buy + " >> need sell " + needs[bestToSell]
					 + " need buy " + needs[buy] + " rate buy " + rates[buy] + " available sell " + available[bestToSell]
					 + " available buy " + available[buy] + " barterRate " + bestRate);
			return true;
		}
	}

	// now do contingency bartering, selling food to buy finite resources (and annoy our ennemies by increasing prices)
	if (available["food"] < 1000 || needs["food"] > 0)
		return false;
	var bestToBuy = undefined;
	var bestChoice = 0;
	for each (var buy in needs.types)
	{
		if (buy === "food")
			continue;
		var barterRate = getBarterRate(prices, buy, "food");
		if (barterRate < 80)
			continue;
		var choice = barterRate / (100 + available[buy]);
		if (choice > bestChoice)
		{
			bestChoice = choice;
			bestToBuy = buy;
		}
	}
	if (bestToBuy !== undefined)
	{
		markets[0].barter(bestToBuy, "food", 100);
		if (this.Config.debug > 1)
			warn("Contingency bartering: sold food for " + bestToBuy + " available sell " + available["food"]
				 + " available buy " + available[bestToBuy] + " barterRate " + getBarterRate(prices, bestToBuy, "food"));
		return true;
	}

	return false;
};

m.TradeManager.prototype.update = function(gameState, queues)
{
	this.performBarter(gameState);

	if (!this.tradeRoute && (gameState.ai.playedTurn % 5 !== 2 || !this.buildTradeRoute(gameState, queues)))
		return;

	var source = this.tradeRoute.source;
	var target = this.tradeRoute.target;
	if (!source || !target || !gameState.getEntityById(source.id()) || !gameState.getEntityById(target.id()))
	{
		if (this.Config.debug > 1)
			warn("We have lost our trade route");
		this.tradeRoute = undefined;
		return;
	}
	var self = this;
	if (gameState.ai.playedTurn % 100 === 9)
		this.setTradingGoods(gameState);
	this.trainMoreTraders(gameState, queues);
	this.traders.forEach(function(ent) { self.updateTrader(ent) });
};

return m;
}(PETRA);
