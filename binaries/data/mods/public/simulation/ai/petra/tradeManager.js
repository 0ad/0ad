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
	this.targetNumTraders = this.Config.Economy.targetNumTraders;
	this.minimalGain = 3;
	this.warnedAllies = {};
};

m.TradeManager.prototype.init = function(gameState)
{
	this.traders = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "role", "trader"));
	this.traders.registerUpdates();
};

m.TradeManager.prototype.hasTradeRoute = function()
{
	return this.tradeRoute !== undefined;
};

m.TradeManager.prototype.assignTrader = function(ent)
{
	ent.setMetadata(PlayerID, "role", "trader");
	this.traders.updateEnt(ent);
};

m.TradeManager.prototype.trainMoreTraders = function(gameState, queues)
{
	if (!this.tradeRoute || queues.trader.hasQueuedUnits())
		return;

	let numTraders = this.traders.length;
	let numSeaTraders = this.traders.filter(API3.Filters.byClass("Ship")).length;
	let numLandTraders = numTraders - numSeaTraders;
	// add traders already in training
	gameState.getOwnTrainingFacilities().forEach(function(ent) {
		for (let item of ent.trainingQueue())
		{
			if (!item.metadata || !item.metadata.role || item.metadata.role !== "trader")
				continue;
			numTraders += item.count;
			if (item.metadata.sea !== undefined)
				numSeaTraders += item.count;
			else
				numLandTraders += item.count;
		}
	});
        if (numTraders >= this.targetNumTraders &&
		((!this.tradeRoute.sea && numLandTraders >= Math.floor(this.targetNumTraders/2)) ||
		  (this.tradeRoute.sea && numSeaTraders >= Math.floor(this.targetNumTraders/2))))
		return;

	let template;
	let metadata = { "role": "trader" };
	if (this.tradeRoute.sea)
	{
		// if we have some merchand ships affected to transport, try first to reaffect them
		// May-be, there were produced at an early stage when no other ship were available
		// and the naval manager will train now more appropriate ships.
		let already = false;
		let shipToSwitch;
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

		template = gameState.applyCiv("units/{civ}_ship_merchant");
		metadata.sea = this.tradeRoute.sea;
	}
	else
	{
		template = gameState.applyCiv("units/{civ}_support_trader");
		if (!this.tradeRoute.source.hasClass("NavalMarket"))
			metadata.base = this.tradeRoute.source.getMetadata(PlayerID, "base");
		else
			metadata.base = this.tradeRoute.target.getMetadata(PlayerID, "base");
	}

	if (!gameState.getTemplate(template))
	{
		if (this.Config.debug > 0)
			API3.warn("Petra error: trying to train " + template + " for civ " + gameState.civ() + " but no template found.");
		return;
	}
	queues.trader.addPlan(new m.TrainingPlan(gameState, template, metadata, 1, 1));
};

m.TradeManager.prototype.updateTrader = function(gameState, ent)
{
	if (!this.tradeRoute || !ent.isIdle() || !ent.position())
		return;
	if (ent.getMetadata(PlayerID, "transport") !== undefined)
		return;

	Engine.ProfileStart("Trade Manager");
	let access = ent.hasClass("Ship") ? ent.getMetadata(PlayerID, "sea") : gameState.ai.accessibility.getAccessValue(ent.position());
	let route = this.checkRoutes(gameState, access);
	if (!route)
	{
		// TODO try to garrison land trader inside merchant ship when only sea routes available
		if (this.Config.debug > 0)
			API3.warn(" no available route for " + ent.genericName() + " " + ent.id());
		Engine.ProfileStop();
		return;
	}

	let nearerSource = true;
	if (API3.SquareVectorDistance(route.target.position(), ent.position()) < API3.SquareVectorDistance(route.source.position(), ent.position()))
		nearerSource = false;

	if (!ent.hasClass("Ship") && route.land !== access)
	{
		if (nearerSource)
			gameState.ai.HQ.navalManager.requireTransport(gameState, ent, access, route.land, route.source.position());
		else
			gameState.ai.HQ.navalManager.requireTransport(gameState, ent, access, route.land, route.target.position());
		Engine.ProfileStop();
		return;
	}

	if (nearerSource)
		ent.tradeRoute(route.target, route.source);
	else
		ent.tradeRoute(route.source, route.target);
	ent.setMetadata(PlayerID, "route", this.routeEntToId(route));
	Engine.ProfileStop();
};

m.TradeManager.prototype.setTradingGoods = function(gameState)
{
	let tradingGoods = {};
	for (let res in gameState.ai.HQ.wantedRates)
		tradingGoods[res] = 0;
	// first, try to anticipate future needs
	let stocks = gameState.ai.HQ.getTotalResourceLevel(gameState);
	let mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
	let remaining = 100;
	let targetNum = this.Config.Economy.targetNumTraders;
	for (let res in stocks)
	{
		if (res === "food")
			continue;
		let wantedRate = gameState.ai.HQ.wantedRates[res];
		if (stocks[res] < 200)
		{
			tradingGoods[res] = wantedRate > 0 ? 20 : 10;
			targetNum += Math.min(5, 3 + Math.ceil(wantedRate/30));
		}
		else if (stocks[res] < 500)
		{
			tradingGoods[res] = wantedRate > 0 ? 15 : 10;
			targetNum += 2;
		}
		else if (stocks[res] < 1000)
		{
			tradingGoods[res] = 10;
			targetNum += 1;
		}
		remaining -= tradingGoods[res];
	}
	this.targetNumTraders = Math.round(this.Config.popScaling * targetNum);


	// then add what is needed now
	let mainNeed = Math.floor(remaining * 70 / 100);
	let nextNeed = remaining - mainNeed;

	tradingGoods[mostNeeded[0].type] += mainNeed;
	if (mostNeeded[1].wanted > 0)
		tradingGoods[mostNeeded[1].type] += nextNeed;
	else
		tradingGoods[mostNeeded[0].type] += nextNeed;
	Engine.PostCommand(PlayerID, {"type": "set-trading-goods", "tradingGoods": tradingGoods});
	if (this.Config.debug > 2)
		API3.warn(" trading goods set to " + uneval(tradingGoods));
};

/**
 * Try to barter unneeded resources for needed resources.
 * only once per turn because the info is not updated within a turn
 */
m.TradeManager.prototype.performBarter = function(gameState)
{
	let barterers = gameState.getOwnEntitiesByClass("BarterMarket", true).filter(API3.Filters.isBuilt()).toEntityArray();
	if (barterers.length === 0)
		return false;

	// Available resources after account substraction
	let available = gameState.ai.queueManager.getAvailableResources(gameState);
	let needs = gameState.ai.queueManager.currentNeeds(gameState);

	let rates = gameState.ai.HQ.GetCurrentGatherRates(gameState);

	let prices = gameState.getBarterPrices();
	// calculates conversion rates
	let getBarterRate = function (prices,buy,sell) { return Math.round(100 * prices.sell[sell] / prices.buy[buy]); };

	// loop through each missing resource checking if we could barter and help finishing a queue quickly.
	for (let buy of needs.types)
	{
		if (needs[buy] === 0 || needs[buy] < rates[buy]*30) // check if our rate allows to gather it fast enough
			continue;

		// pick the best resource to barter.
		let bestToSell;
		let bestRate = 0;
		for (let sell of needs.types)
		{
			if (sell === buy)
				continue;
			if (needs[sell] > 0 || available[sell] < 500)    // do not sell if we need it or do not have enough buffer
				continue;

			let barterRateMin;
			if (sell === "food")
			{
				barterRateMin = 30;
				if (available[sell] > 40000)
					barterRateMin = 0;
				else if (available[sell] > 15000)
					barterRateMin = 5;
				else if (available[sell] > 1000)
					barterRateMin = 10;
			}
			else
			{
				barterRateMin = 70;
				if (available[sell] > 1000)
					barterRateMin = 50;
				if (buy === "food")
					barterRateMin += 20;
			}

			let barterRate = getBarterRate(prices, buy, sell);
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
				API3.warn("Necessity bartering: sold " + bestToSell +" for " + buy + " >> need sell " + needs[bestToSell] +
					  " need buy " + needs[buy] + " rate buy " + rates[buy] + " available sell " + available[bestToSell] +
					  " available buy " + available[buy] + " barterRate " + bestRate);
			return true;
		}
	}

	// now do contingency bartering, selling food to buy finite resources (and annoy our ennemies by increasing prices)
	if (available.food < 1000 || needs.food > 0)
		return false;
	let bestToBuy;
	let bestChoice = 0;
	for (let buy of needs.types)
	{
		if (buy === "food")
			continue;
		let barterRateMin = 80;
		if (available[buy] < 5000 && available.food > 5000)
			barterRateMin -= 20 - Math.floor(available[buy]/250);
		let barterRate = getBarterRate(prices, buy, "food");
		if (barterRate < barterRateMin)
			continue;
		let choice = barterRate / (100 + available[buy]);
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
			API3.warn("Contingency bartering: sold food for " + bestToBuy + " available sell " + available.food +
				  " available buy " + available[bestToBuy] + " barterRate " + getBarterRate(prices, bestToBuy, "food"));
		return true;
	}

	return false;
};

m.TradeManager.prototype.checkEvents = function(gameState, events)
{
	// check if one market from a traderoute is renamed, change the route accordingly
	for (let evt of events.EntityRenamed)
	{
		let ent = gameState.getEntityById(evt.newentity);
		if (!ent || !ent.hasClass("Market"))
			continue;
		for (let trader of this.traders.values())
		{
			let route = trader.getMetadata(PlayerID, "route");
			if (!route)
				continue;
			if (route.source === evt.entity)
				route.source = evt.newentity;
			else if (route.target === evt.entity)
				route.target = evt.newentity;
			else
				continue;
			trader.setMetadata(PlayerID, "route", route);
		}
	}

	// if one market is destroyed, we should look for a better route
	for (let evt of events.Destroy)
	{
		if (!evt.entityObj)
			continue;
		let ent = evt.entityObj;
		if (!ent || ent.foundationProgress() !== undefined || !ent.hasClass("Market") || !gameState.isPlayerAlly(ent.owner()))
			continue;
		this.routeProspection = true;
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_market");
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_dock");
		return true;
	}

	// same thing if one market is built
	for (let evt of events.Create)
	{
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || ent.foundationProgress() !== undefined || !ent.hasClass("Market") || !gameState.isPlayerAlly(ent.owner()))
			continue;
		this.routeProspection = true;
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_market");
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_dock");
		return true;
	}


	// and same thing for captured markets
	for (let evt of events.OwnershipChanged)
	{
		if (!gameState.isPlayerAlly(evt.from) && !gameState.isPlayerAlly(evt.to))
			continue;
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || ent.foundationProgress() !== undefined || !ent.hasClass("Market"))
			continue;
		this.routeProspection = true;
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_market");
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_dock");
		return true;
	}

	// or if diplomacy changed
	if (events.DiplomacyChanged.length)
	{
		this.routeProspection = true;
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_market");
		gameState.ai.HQ.restartBuild(gameState, "structures/{civ}_dock");
		return true;
	}

	return false;
};

/**
 * fills the best trade route in this.tradeRoute and the best potential route in this.potentialTradeRoute
 * If an index is given, it returns the best route with this index or the best land route if index is a land index
 */
m.TradeManager.prototype.checkRoutes = function(gameState, accessIndex)
{
	let market1 = gameState.updatingCollection("OwnMarkets", API3.Filters.byClass("Market"), gameState.getOwnStructures()).toEntityArray();
	let market2 = gameState.updatingCollection("ExclusiveAllyMarkets", API3.Filters.byClass("Market"), gameState.getExclusiveAllyEntities()).toEntityArray();
	if (market1.length + market2.length < 2)  // We have to wait  ... markets will be built soon
	{
		this.tradeRoute = undefined;
		this.potentialTradeRoute = undefined;
		return false;
	}

	if (!market2.length)
		market2 = market1;
	let candidate = { "gain": 0 };
	let potential = { "gain": 0 };
	let bestIndex = { "gain": 0 };
	let bestLand  = { "gain": 0 };

	let traderTemplatesGains = gameState.getTraderTemplatesGains();

	for (let m1 of market1)
	{
		if (!m1.position())
			continue;
		let access1 = gameState.ai.accessibility.getAccessValue(m1.position());
		let sea1 = m1.hasClass("NavalMarket") ? gameState.ai.HQ.navalManager.getDockIndex(gameState, m1, true) : undefined;
		for (let m2 of market2)
		{
			if (m1.id() === m2.id())
				continue;
			if (!m2.position())
				continue;
			let access2 = gameState.ai.accessibility.getAccessValue(m2.position());
			let sea2 = m2.hasClass("NavalMarket") ? gameState.ai.HQ.navalManager.getDockIndex(gameState, m2, true) : undefined;
			let land = access1 == access2 ? access1 : undefined;
			let sea = (sea1 && sea1 == sea2) ? sea1 : undefined;
			if (!land && !sea)
				continue;
			let gainMultiplier;
			if (land && traderTemplatesGains.landGainMultiplier)
				gainMultiplier = traderTemplatesGains.landGainMultiplier;
			else if (sea && traderTemplatesGains.navalGainMultiplier)
				gainMultiplier = traderTemplatesGains.navalGainMultiplier;
			else
				continue;
			let gain = Math.round(API3.SquareVectorDistance(m1.position(), m2.position()) * gainMultiplier / 10000);
			if (gain < this.minimalGain)
				continue;
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

	if (this.Config.chat)
	{
		let owner = this.tradeRoute.source.owner();
		if (owner === PlayerID)
			owner = this.tradeRoute.target.owner();
		if (owner !== PlayerID && !this.warnedAllies[owner])
		{	// Warn an ally that we have a trade route with him
			m.chatNewTradeRoute(gameState, owner);
			this.warnedAllies[owner] = true;
		}
	}

	if (accessIndex)
	{
		if (bestIndex.gain > 0)
			return bestIndex;
		else if (gameState.ai.accessibility.regionType[accessIndex] === "land" && bestLand.gain > 0)
			return bestLand;
		return false;
	}
	return true;
};

/** Called when a market was built or destroyed, and checks if trader orders should be changed */
m.TradeManager.prototype.checkTrader = function(gameState, ent)
{
	let presentRoute = ent.getMetadata(PlayerID, "route");
	if (!presentRoute)
		return;

	if (!ent.position())
	{
		// This trader is garrisoned, we will decide later (when ungarrisoning) what to do
		ent.setMetadata(PlayerID, "route", undefined);
		return;
	}

	let access = ent.hasClass("Ship") ? ent.getMetadata(PlayerID, "sea") : gameState.ai.accessibility.getAccessValue(ent.position());
	let possibleRoute = this.checkRoutes(gameState, access);
	// Warning:  presentRoute is from metadata, so contains entity ids
	if (!possibleRoute ||
		(possibleRoute.source.id() != presentRoute.source && possibleRoute.source.id() != presentRoute.target) ||
		(possibleRoute.target.id() != presentRoute.source && possibleRoute.target.id() != presentRoute.target))
	{
		// Trader will be assigned in updateTrader
		ent.stopMoving();
		ent.setMetadata(PlayerID, "route", undefined);
	}
};

m.TradeManager.prototype.prospectForNewMarket = function(gameState, queues)
{
	if (queues.economicBuilding.hasQueuedUnitsWithClass("Market") || queues.dock.hasQueuedUnitsWithClass("Market"))
		return;
	if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_market"))
		return;
	if (!gameState.updatingCollection("OwnMarkets", API3.Filters.byClass("Market"), gameState.getOwnStructures()).length &&
		!gameState.updatingCollection("ExclusiveAllyMarkets", API3.Filters.byClass("Market"), gameState.getExclusiveAllyEntities()).length)
		return;
	let template = gameState.getTemplate(gameState.applyCiv("structures/{civ}_market"));
	if (!template)
		return;
	this.checkRoutes(gameState);
	let marketPos = gameState.ai.HQ.findMarketLocation(gameState, template);
	if (!marketPos || marketPos[3] === 0)   // marketPos[3] is the expected gain
	{	// no position found
		gameState.ai.HQ.stopBuild(gameState, "structures/{civ}_market");
		return;
	}
	this.routeProspection = false;
	if (!this.isNewMarketWorth(marketPos[3]))
		return;	// position found, but not enough gain compared to our present route

	if (this.Config.debug > 1)
	{
		if (this.potentialTradeRoute)
			API3.warn("turn " + gameState.ai.playedTurn + "we could have a new route with gain " +
				marketPos[3] + " instead of the present " + this.potentialTradeRoute.gain);
		else
			API3.warn("turn " + gameState.ai.playedTurn + "we could have a first route with gain " +
				marketPos[3]);
	}

	if (!this.tradeRoute)
		gameState.ai.queueManager.changePriority("economicBuilding", 2*this.Config.priorities.economicBuilding);
	let plan = new m.ConstructionPlan(gameState, "structures/{civ}_market");
	if (!this.tradeRoute)
		plan.onStart = function(gameState) { gameState.ai.queueManager.changePriority("economicBuilding", gameState.ai.Config.priorities.economicBuilding); };
	queues.economicBuilding.addPlan(plan);
};

m.TradeManager.prototype.isNewMarketWorth = function(expectedGain)
{
	if (this.potentialTradeRoute && expectedGain < 2*this.potentialTradeRoute.gain &&
		expectedGain < this.potentialTradeRoute.gain + 20)
		return false;
	return true;
};

m.TradeManager.prototype.update = function(gameState, events, queues)
{
	this.performBarter(gameState);

	if (this.routeProspection)
		this.prospectForNewMarket(gameState, queues);

	let self = this;

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
		if (gameState.ai.playedTurn % 60 === 0)
			this.setTradingGoods(gameState);
	}
};

m.TradeManager.prototype.routeEntToId = function(route)
{
	if (!route)
		return route;
	let ret = {};
	for (let key in route)
		ret[key] = (key == "source" || key == "target") ? route[key].id() : route[key];
	return ret;
};

m.TradeManager.prototype.routeIdToEnt = function(gameState, route)
{
	if (!route)
		return route;
	let ret = {};
	for (let key in route)
		ret[key] = (key == "source" || key == "target") ? gameState.getEntityById(route[key]) : route[key];
	return ret;
};

m.TradeManager.prototype.Serialize = function()
{
	return {
		"tradeRoute": this.routeEntToId(this.tradeRoute),
		"potentialTradeRoute": this.routeEntToId(this.potentialTradeRoute),
		"routeProspection": this.routeProspection,
		"targetNumTraders": this.targetNumTraders,
		"warnedAllies": this.warnedAllies
	};
};

m.TradeManager.prototype.Deserialize = function(gameState, data)
{
	this.tradeRoute = this.routeIdToEnt(gameState, data.tradeRoute);
	this.potentialTradeRoute = this.routeIdToEnt(gameState, data.potentialTradeRoute);
	this.routeProspection = data.routeProspection;
	this.targetNumTraders = data.targetNumTraders;
	this.warnedAllies = data.warnedAllies;
};

return m;
}(PETRA);
