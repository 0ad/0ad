var PETRA = function(m)
{

/**
 * Manage the trade
 */

m.TradeManager = function(Config)
{
	this.Config = Config;
	this.tradeRoute = undefined;
	this.potentialTradeRoute = undefined;
	this.routeProspection = false;
	this.targetNumTraders = Math.round(this.Config.popScaling * this.Config.Economy.targetNumTraders);
};

m.TradeManager.prototype.init = function(gameState)
{
	this.traders = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "role", "trader"));
	this.traders.allowQuickIter();
	this.traders.registerUpdates();
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
	if (!this.tradeRoute || queues.trader.countQueuedUnits())
		return;

	var numTraders = this.traders.length;
	var numSeaTraders = this.traders.filter(API3.Filters.byClass("Ship")).length;
	var numLandTraders = numTraders - numSeaTraders;
	// add traders already in training
	gameState.getOwnTrainingFacilities().forEach(function(ent) {
		ent.trainingQueue().forEach(function(item) {
			if (!item.metadata || !item.metadata.role || item.metadata.role !== "trader")
				return;
			numTraders += item.count;
			if (item.sea !== undefined)
				numSeaTraders += item.count;
			else
				numLandTraders += item.count;
		});
	});
        if (numTraders >= this.targetNumTraders
		&& ((!this.tradeRoute.sea && numLandTraders >= Math.floor(this.targetNumTraders/2))
		|| (this.tradeRoute.sea && numSeaTraders >= Math.floor(this.targetNumTraders/2))))
		return;

	if (this.tradeRoute.sea)
	{
		// if we have some merchand ships affected to transport, try first to reaffect them
		// May-be, there were produced at an early stage when no other ship were available
		// and the naval manager will train now more appropriate ships.
		var already = false;
		var shipToSwitch = undefined;
		gameState.ai.HQ.navalManager.seaTransportShips[this.tradeRoute.sea].forEach(function(ship) {
			if (already || !ship.hasClass("Trader"))
				return;
			if (ship.getMetadata(PlayerID, "role") === "switchToTrader")
			{
				already = true;
				return;
			}
			shipToSwitch = ship;
		});
		if (already)
			return;
		if (shipToSwitch)
		{
			if (shipToSwitch.getMetadata(PlayerID, "transporter") === undefined)
				shipToSwitch.setMetadata(PlayerID, "role", "trader");
			else
				shipToSwitch.setMetadata(PlayerID, "role", "switchToTrader");
			return;
		}

		var template = gameState.applyCiv("units/{civ}_ship_merchant");
		var metadata = { "role": "trader", "sea": this.tradeRoute.sea };
	}
	else
	{
		var template = gameState.applyCiv("units/{civ}_support_trader");
		if (!this.tradeRoute.source.hasClass("NavalMarket"))
			var metadata = { "role": "trader", "base": this.tradeRoute.source.getMetadata(PlayerID, "base") };
		else
			var metadata = { "role": "trader", "base": this.tradeRoute.target.getMetadata(PlayerID, "base") };
	}

	if (!gameState.getTemplate(template))
	{
		if (this.Config.debug > 0)
			API3.warn("Petra error: trying to train " + template + " for civ " + gameState.civ() + " but no template found.");
		return;
	}
	queues.trader.addItem(new m.TrainingPlan(gameState, template, metadata, 1, 1));
};

m.TradeManager.prototype.updateTrader = function(gameState, ent)
{
	if (!this.tradeRoute || !ent.isIdle() || !ent.position())
		return;

	if (ent.hasClass("Ship"))
		var access = ent.getMetadata(PlayerID, "sea");
	else
		var access = gameState.ai.accessibility.getAccessValue(ent.position());
	var route = this.checkRoutes(gameState, access);
	if (!route)
	{
		// TODO try to garrison land trader inside merchant ship when only sea routes available
		if (this.Config.debug > 0)
			API3.warn(" no available route for " + ent.genericName() + " " + ent.id());
		return;
	}

	var nearerSource = true;
	if (API3.SquareVectorDistance(route.target.position(), ent.position()) < API3.SquareVectorDistance(route.source.position(), ent.position()))
		nearerSource = false;

	if (!ent.hasClass("Ship") && route.land !== access)
	{
		if (nearerSource)
			gameState.ai.HQ.navalManager.requireTransport(gameState, ent, access, route.land, route.source.position());
		else
			gameState.ai.HQ.navalManager.requireTransport(gameState, ent, access, route.land, route.target.position());
	}
	else if (nearerSource)
		ent.tradeRoute(route.target, route.source);
	else
		ent.tradeRoute(route.source, route.target);
	ent.setMetadata(PlayerID, "route", route);
};

m.TradeManager.prototype.setTradingGoods = function(gameState)
{
	var tradingGoods = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	// first, try to anticipate future needs 
	var stocks = gameState.ai.HQ.getTotalResourceLevel(gameState);
	var remaining = 100;
	var targetNum = this.Config.Economy.targetNumTraders;
	for (var type in stocks)
	{
		if (type == "food")
			continue;
		if (stocks[type] < 200)
		{
			tradingGoods[type] = 20;
			targetNum += 3;
		}
		else if (stocks[type] < 500)
		{
			tradingGoods[type] = 15;
			targetNum += 2;
		}
		else if (stocks[type] < 1000)
		{
			tradingGoods[type] = 10;
			targetNum += 1;
		}
		remaining -= tradingGoods[type];
	}
	this.targetNumTraders = Math.round(this.Config.popScaling * targetNum);


	// then add what is needed now
	var mainNeed = Math.floor(remaining * 70 / 100)
	var nextNeed = remaining - mainNeed;

	var mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
	tradingGoods[mostNeeded[0].type] += mainNeed;
	if (mostNeeded[1].wanted > 0)
		tradingGoods[mostNeeded[1].type] += nextNeed;
	else
		tradingGoods[mostNeeded[0].type] += nextNeed;
	Engine.PostCommand(PlayerID, {"type": "set-trading-goods", "tradingGoods": tradingGoods});
	if (this.Config.debug > 2)
		API3.warn(" trading goods set to " + uneval(tradingGoods));
};

// Try to barter unneeded resources for needed resources.
// only once per turn because the info doesn't update between a turn and fixing isn't worth it.
m.TradeManager.prototype.performBarter = function(gameState)
{
	var barterers = gameState.getOwnStructures().filter(API3.Filters.and(API3.Filters.byClass("BarterMarket"), API3.Filters.not(API3.Filters.isFoundation()))).toEntityArray();
	if (barterers.length === 0)
		return false;

	// Available resources after account substraction
	var available = gameState.ai.queueManager.getAvailableResources(gameState);
	var needs = gameState.ai.queueManager.currentNeeds(gameState);

	var rates = gameState.ai.HQ.GetCurrentGatherRates(gameState);

	var prices = gameState.getBarterPrices();
	// calculates conversion rates
	var getBarterRate = function (prices,buy,sell) { return Math.round(100 * prices["sell"][sell] / prices["buy"][buy]); };

	// loop through each missing resource checking if we could barter and help finishing a queue quickly.
	for (var buy of needs.types)
	{
		if (needs[buy] == 0 || needs[buy] < rates[buy]*30) // check if our rate allows to gather it fast enough
			continue;

		// pick the best resource to barter.
		var bestToSell = undefined;
		var bestRate = 0;
		for (var sell of needs.types)
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
			barterers[0].barter(buy, bestToSell, 100);
			if (this.Config.debug > 2)
				API3.warn("Necessity bartering: sold " + bestToSell +" for " + buy + " >> need sell " + needs[bestToSell]
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
	for (var buy of needs.types)
	{
		if (buy === "food")
			continue;
		var barterRateMin = 80;
		if (available[buy] < 5000 && available["food"] > 5000)
			barterRateMin -= (20 - Math.floor(available[buy]/250));
		var barterRate = getBarterRate(prices, buy, "food");
		if (barterRate < barterRateMin)
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
		barterers[0].barter(bestToBuy, "food", 100);
		if (this.Config.debug > 2)
			API3.warn("Contingency bartering: sold food for " + bestToBuy + " available sell " + available["food"]
				 + " available buy " + available[bestToBuy] + " barterRate " + getBarterRate(prices, bestToBuy, "food"));
		return true;
	}

	return false;
};

m.TradeManager.prototype.checkEvents = function(gameState, events)
{
	var destroyEvents = events["Destroy"];
	for (var evt of destroyEvents)
	{
		if (!evt.entityObj)
			continue;
		var ent = evt.entityObj;
		if (!ent || !ent.hasClass("Market") || !gameState.isPlayerAlly(ent.owner()))
			continue;
		if (this.Config.debug > 1 && evt.SuccessfulFoundation)
			API3.warn("new market build ... checking routes");
		else if (this.Config.debug > 1)
			API3.warn("one market has been destroyed ... checking routes");
		return true;
	}

	return false;
};

// fills the best trade route in this.tradeRoute and the best potential route in this.potentialTradeRoute
// If an index is given, it returns the best route with this index or the best land route if index is a land index 
m.TradeManager.prototype.checkRoutes = function(gameState, accessIndex)
{
	var market1 = gameState.getOwnStructures().filter(API3.Filters.byClass("Market")).toEntityArray();
	var market2 = gameState.getExclusiveAllyEntities().filter(API3.Filters.byClass("Market")).toEntityArray();
	if (market1.length + market2.length < 2)  // We have to wait  ... markets will be built soon
	{
		this.tradeRoute = undefined;
		this.potentialTradeRoute = undefined;
		return false;
	}

	if (market2.length === 0)
		market2 = market1;
	var candidate = { "gain": 0 };
	var potential = { "gain": 0 };
	var bestIndex = { "gain": 0 };
	var bestLand  = { "gain": 0 };
	for (var m1 of market1)
	{
		if (!m1.position())
			continue;
		var access1 = gameState.ai.accessibility.getAccessValue(m1.position());
		var sea1 = m1.hasClass("Dock") ? gameState.ai.HQ.navalManager.getDockIndex(gameState, m1, true) : undefined;
		for (var m2 of market2)
		{
			if (m1.id() === m2.id())
				continue;
			if (!m2.position())
				continue;
			var access2 = gameState.ai.accessibility.getAccessValue(m2.position());
			var sea2 = m2.hasClass("Dock") ? gameState.ai.HQ.navalManager.getDockIndex(gameState, m2, true) : undefined;
			var land = (access1 === access2) ? access1 : undefined;
			var sea = (sea1 && sea1 === sea2) ? sea1 : undefined;
			if (!land && !sea)
				continue;
			var gain = Math.round(API3.SquareVectorDistance(m1.position(), m2.position()) / this.Config.distUnitGain);
			if (m1.foundationProgress() === undefined && m2.foundationProgress() === undefined)
			{
				if (accessIndex)
				{
					if (gameState.ai.accessibility.regionType[accessIndex] === "water" && sea === accessIndex)
					{
						if (gain < bestIndex.gain)
							continue;
						bestIndex = { "source": m1, "target": m2, "gain": gain, "land": land, "sea": sea };
					}
					else if (gameState.ai.accessibility.regionType[accessIndex] === "land" && land === accessIndex)
					{
						if (gain < bestIndex.gain)
							continue;
						bestIndex = { "source": m1, "target": m2, "gain": gain, "land": land, "sea": sea };
					}
					else if (gameState.ai.accessibility.regionType[accessIndex] === "land")
					{
						if (gain < bestLand.gain)
							continue;
						bestLand = { "source": m1, "target": m2, "gain": gain, "land": land, "sea": sea };
					}
				}
				if (gain < candidate.gain)
					continue;
				candidate = { "source": m1, "target": m2, "gain": gain, "land": land, "sea": sea };
			}
			if (gain < potential.gain)
				continue;
			potential = { "source": m1, "target": m2, "gain": gain, "land": land, "sea": sea };
		}
	}

	if (potential.gain < 1)
		this.potentialTradeRoute = undefined;
	else
		this.potentialTradeRoute = potential;

	if (candidate.gain < 1)
	{
		if (this.Config.debug > 2)
			API3.warn("no better trade route possible");
		this.tradeRoute = undefined;
		return false;
	}

	if (this.Config.debug > 1 && this.tradeRoute)
	{
		if (candidate.gain > this.tradeRoute.gain)
			API3.warn("one better trade route set with gain " + candidate.gain + " instead of " + this.tradeRoute.gain);
	}
	else if (this.Config.debug > 1)
		API3.warn("one trade route set with gain " + candidate.gain);
	this.tradeRoute = candidate;

	if (accessIndex)
	{
		if (bestIndex.gain > 0)
			return bestIndex;
		else if (gameState.ai.accessibility.regionType[accessIndex] === "land" && bestLand.gain > 0)
			return bestLand;
		else
			return false;
	}
	else
		return true;
};

// Called when a market was built or destroyed, and checks if trader orders should be changed
m.TradeManager.prototype.checkTrader = function(gameState, ent)
{
	var presentRoute = ent.getMetadata(PlayerID, "route");
	if (!presentRoute)
		return;

	if (ent.hasClass("Ship"))
		var access = ent.getMetadata(PlayerID, "sea");
	else
		var access = gameState.ai.accessibility.getAccessValue(ent.position());
	var possibleRoute = this.checkRoutes(gameState, access);
	if (!possibleRoute)
	{
		ent.stopMoving();
		ent.setMetadata(PlayerID, "route", undefined);
		return;
	}

	if (possibleRoute && (
		(possibleRoute.source.id() === presentRoute.source.id() && possibleRoute.target.id() === presentRoute.target.id()) ||
		(possibleRoute.source.id() === presentRoute.target.id() && possibleRoute.target.id() === presentRoute.source.id())))
		return;

	ent.stopMoving();
	ent.setMetadata(PlayerID, "route", undefined);
};

m.TradeManager.prototype.prospectForNewMarket = function(gameState, queues)
{
	if (queues.economicBuilding.countQueuedUnitsWithClass("Market") > 0)
		return;
	if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_market"))
		return;
	if (!gameState.getOwnStructures().filter(API3.Filters.byClass("Market")).length &&
		!gameState.getExclusiveAllyEntities().filter(API3.Filters.byClass("Market")).length)
		return;
	var template = gameState.getTemplate(gameState.applyCiv("structures/{civ}_market"));
	if (!template)
		return;
	this.checkRoutes(gameState);
	var marketPos = gameState.ai.HQ.findMarketLocation(gameState, template);
	if (!marketPos || marketPos[3] === 0)   // marketPos[3] is the expected gain
		return;
	if (this.potentialTradeRoute && marketPos[3] < 2*this.potentialTradeRoute.gain
		&& marketPos[3] < this.potentialTradeRoute.gain + 20)
		return;

	if (this.Config.debug > 1)
	{
		if (this.potentialTradeRoute)
			API3.warn("turn " + gameState.ai.playedTurn + "we could have a new route with gain "
				+ marketPos[3] + " instead of the present " + this.potentialTradeRoute.gain);
		else
			API3.warn("turn " + gameState.ai.playedTurn + "we could have a first route with gain "
				+ marketPos[3]);
	}
	queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_market"));
	this.routeProspection = false;
};

m.TradeManager.prototype.update = function(gameState, events, queues)
{
	this.performBarter(gameState);

	if (this.routeProspection)
		this.prospectForNewMarket(gameState, queues);

	var self = this;

	if (this.checkEvents(gameState, events))  // true if one market was built or destroyed
	{
		this.traders.forEach(function(ent) { self.checkTrader(gameState, ent); });
		this.checkRoutes(gameState);
	}

	if (this.tradeRoute)
	{
		this.traders.forEach(function(ent) { self.updateTrader(gameState, ent); });
		if (gameState.ai.playedTurn % 5 === 0)
			this.trainMoreTraders(gameState, queues);
		if (gameState.ai.playedTurn % 20 === 0 && this.traders.length >= 2)
			gameState.ai.HQ.researchManager.researchTradeBonus(gameState, queues);
		if (gameState.ai.playedTurn % 80 === 0)
			this.setTradingGoods(gameState);
	}
};

return m;
}(PETRA);
