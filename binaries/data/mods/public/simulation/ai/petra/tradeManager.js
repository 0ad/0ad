/**
 * Manage the trade
 */
PETRA.TradeManager = function(Config)
{
	this.Config = Config;
	this.tradeRoute = undefined;
	this.potentialTradeRoute = undefined;
	this.routeProspection = false;
	this.targetNumTraders = this.Config.Economy.targetNumTraders;
	this.warnedAllies = {};
};

PETRA.TradeManager.prototype.init = function(gameState)
{
	this.traders = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "role", "trader"));
	this.traders.registerUpdates();
	this.minimalGain = gameState.ai.HQ.navalMap ? 3 : 5;
};

PETRA.TradeManager.prototype.hasTradeRoute = function()
{
	return this.tradeRoute !== undefined;
};

PETRA.TradeManager.prototype.assignTrader = function(ent)
{
	ent.setMetadata(PlayerID, "role", "trader");
	this.traders.updateEnt(ent);
};

PETRA.TradeManager.prototype.trainMoreTraders = function(gameState, queues)
{
	if (!this.hasTradeRoute() || queues.trader.hasQueuedUnits())
		return;

	let numTraders = this.traders.length;
	let numSeaTraders = this.traders.filter(API3.Filters.byClass("Ship")).length;
	let numLandTraders = numTraders - numSeaTraders;
	// add traders already in training
	gameState.getOwnTrainingFacilities().forEach(function(ent) {
		for (let item of ent.trainingQueue())
		{
			if (!item.metadata || !item.metadata.role || item.metadata.role != "trader")
				continue;
			numTraders += item.count;
			if (item.metadata.sea !== undefined)
				numSeaTraders += item.count;
			else
				numLandTraders += item.count;
		}
	});
	if (numTraders >= this.targetNumTraders &&
		(!this.tradeRoute.sea && numLandTraders >= Math.floor(this.targetNumTraders/2) ||
		  this.tradeRoute.sea && numSeaTraders >= Math.floor(this.targetNumTraders/2)))
		return;

	let template;
	let metadata = { "role": "trader" };
	if (this.tradeRoute.sea)
	{
		// if we have some merchand ships assigned to transport, try first to reassign them
		// May-be, there were produced at an early stage when no other ship were available
		// and the naval manager will train now more appropriate ships.
		let already = false;
		let shipToSwitch;
		gameState.ai.HQ.navalManager.seaTransportShips[this.tradeRoute.sea].forEach(function(ship) {
			if (already || !ship.hasClass("Trader"))
				return;
			if (ship.getMetadata(PlayerID, "role") == "switchToTrader")
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
			API3.warn("Petra error: trying to train " + template + " for civ " +
			          gameState.getPlayerCiv() + " but no template found.");
		return;
	}
	queues.trader.addPlan(new PETRA.TrainingPlan(gameState, template, metadata, 1, 1));
};

PETRA.TradeManager.prototype.updateTrader = function(gameState, ent)
{
	if (ent.hasClass("Ship") && gameState.ai.playedTurn % 5 == 0 &&
	    !ent.unitAIState().startsWith("INDIVIDUAL.GATHER") &&
	    PETRA.gatherTreasure(gameState, ent, true))
		return;

	if (!this.hasTradeRoute() || !ent.isIdle() || !ent.position())
		return;
	if (ent.getMetadata(PlayerID, "transport") !== undefined)
		return;

	// TODO if the trader is idle and has workOrders, restore them to avoid losing the current gain

	Engine.ProfileStart("Trade Manager");
	let access = ent.hasClass("Ship") ? PETRA.getSeaAccess(gameState, ent) : PETRA.getLandAccess(gameState, ent);
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

	if (!ent.hasClass("Ship") && route.land != access)
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

PETRA.TradeManager.prototype.setTradingGoods = function(gameState)
{
	let tradingGoods = {};
	for (let res of Resources.GetCodes())
		tradingGoods[res] = 0;
	// first, try to anticipate future needs
	let stocks = gameState.ai.HQ.getTotalResourceLevel(gameState);
	let mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
	let wantedRates = gameState.ai.HQ.GetWantedGatherRates(gameState);
	let remaining = 100;
	let targetNum = this.Config.Economy.targetNumTraders;
	for (let res in stocks)
	{
		if (res == "food")
			continue;
		let wantedRate = wantedRates[res];
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
	Engine.PostCommand(PlayerID, { "type": "set-trading-goods", "tradingGoods": tradingGoods });
	if (this.Config.debug > 2)
		API3.warn(" trading goods set to " + uneval(tradingGoods));
};

/**
 * Try to barter unneeded resources for needed resources.
 * only once per turn because the info is not updated within a turn
 */
PETRA.TradeManager.prototype.performBarter = function(gameState)
{
	let barterers = gameState.getOwnEntitiesByClass("BarterMarket", true).filter(API3.Filters.isBuilt()).toEntityArray();
	if (barterers.length == 0)
		return false;

	// Available resources after account substraction
	let available = gameState.ai.queueManager.getAvailableResources(gameState);
	let needs = gameState.ai.queueManager.currentNeeds(gameState);

	let rates = gameState.ai.HQ.GetCurrentGatherRates(gameState);

	let barterPrices = gameState.getBarterPrices();
	// calculates conversion rates
	let getBarterRate = (prices, buy, sell) => Math.round(100 * prices.sell[sell] / prices.buy[buy]);

	// loop through each missing resource checking if we could barter and help finishing a queue quickly.
	for (let buy of Resources.GetCodes())
	{
		// Check if our rate allows to gather it fast enough
		if (needs[buy] == 0 || needs[buy] < rates[buy] * 30)
			continue;

		// Pick the best resource to barter.
		let bestToSell;
		let bestRate = 0;
		for (let sell of Resources.GetCodes())
		{
			if (sell == buy)
				continue;
			// Do not sell if we need it or do not have enough buffer
			if (needs[sell] > 0 || available[sell] < 500)
				continue;

			let barterRateMin;
			if (sell == "food")
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
				if (available[sell] > 5000)
					barterRateMin = 30;
				else if (available[sell] > 1000)
					barterRateMin = 50;
				if (buy == "food")
					barterRateMin += 20;
			}

			let barterRate = getBarterRate(barterPrices, buy, sell);
			if (barterRate > bestRate && barterRate > barterRateMin)
			{
				bestRate = barterRate;
				bestToSell = sell;
			}
		}
		if (bestToSell !== undefined)
		{
			let amount = available[bestToSell] > 5000 ? 500 : 100;
			barterers[0].barter(buy, bestToSell, amount);
			if (this.Config.debug > 2)
				API3.warn("Necessity bartering: sold " + bestToSell +" for " + buy +
				          " >> need sell " + needs[bestToSell] + " need buy " + needs[buy] +
				          " rate buy " + rates[buy] + " available sell " + available[bestToSell] +
				          " available buy " + available[buy] + " barterRate " + bestRate +
				          " amount " + amount);
			return true;
		}
	}

	// now do contingency bartering, selling food to buy finite resources (and annoy our ennemies by increasing prices)
	if (available.food < 1000 || needs.food > 0)
		return false;
	let bestToBuy;
	let bestChoice = 0;
	for (let buy of Resources.GetCodes())
	{
		if (buy == "food")
			continue;
		let barterRateMin = 80;
		if (available[buy] < 5000 && available.food > 5000)
			barterRateMin -= 20 - Math.floor(available[buy]/250);
		let barterRate = getBarterRate(barterPrices, buy, "food");
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
		let amount = available.food > 5000 ? 500 : 100;
		barterers[0].barter(bestToBuy, "food", amount);
		if (this.Config.debug > 2)
			API3.warn("Contingency bartering: sold food for " + bestToBuy +
			          " available sell " + available.food + " available buy " + available[bestToBuy] +
			          " barterRate " + getBarterRate(barterPrices, bestToBuy, "food") +
			          " amount " + amount);
		return true;
	}

	return false;
};

PETRA.TradeManager.prototype.checkEvents = function(gameState, events)
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
			if (route.source == evt.entity)
				route.source = evt.newentity;
			else if (route.target == evt.entity)
				route.target = evt.newentity;
			else
				continue;
			trader.setMetadata(PlayerID, "route", route);
		}
	}

	// if one market (or market-foundation) is destroyed, we should look for a better route
	for (let evt of events.Destroy)
	{
		if (!evt.entityObj)
			continue;
		let ent = evt.entityObj;
		if (!ent || !ent.hasClass("Market") || !gameState.isPlayerAlly(ent.owner()))
			continue;
		this.activateProspection(gameState);
		return true;
	}

	// same thing if one market is built
	for (let evt of events.Create)
	{
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || ent.foundationProgress() !== undefined || !ent.hasClass("Market") ||
		    !gameState.isPlayerAlly(ent.owner()))
			continue;
		this.activateProspection(gameState);
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
		this.activateProspection(gameState);
		return true;
	}

	// or if diplomacy changed
	if (events.DiplomacyChanged.length)
	{
		this.activateProspection(gameState);
		return true;
	}

	return false;
};

PETRA.TradeManager.prototype.activateProspection = function(gameState)
{
	this.routeProspection = true;
	gameState.ai.HQ.buildManager.setBuildable(gameState.applyCiv("structures/{civ}_market"));
	gameState.ai.HQ.buildManager.setBuildable(gameState.applyCiv("structures/{civ}_dock"));
};

/**
 * fills the best trade route in this.tradeRoute and the best potential route in this.potentialTradeRoute
 * If an index is given, it returns the best route with this index or the best land route if index is a land index
 */
PETRA.TradeManager.prototype.checkRoutes = function(gameState, accessIndex)
{
	let market1 = gameState.updatingCollection("OwnMarkets", API3.Filters.byClass("Market"), gameState.getOwnStructures());
	let market2 = gameState.updatingCollection("diplo-ExclusiveAllyMarkets", API3.Filters.byClass("Market"), gameState.getExclusiveAllyEntities());
	if (market1.length + market2.length < 2)  // We have to wait  ... markets will be built soon
	{
		this.tradeRoute = undefined;
		this.potentialTradeRoute = undefined;
		return false;
	}

	let onlyOurs = !market2.hasEntities();
	if (onlyOurs)
		market2 = market1;
	let candidate = { "gain": 0 };
	let potential = { "gain": 0 };
	let bestIndex = { "gain": 0 };
	let bestLand  = { "gain": 0 };

	let mapSize = gameState.sharedScript.mapSize;
	let traderTemplatesGains = gameState.getTraderTemplatesGains();

	for (let m1 of market1.values())
	{
		if (!m1.position())
			continue;
		let access1 = PETRA.getLandAccess(gameState, m1);
		let sea1 = m1.hasClass("NavalMarket") ? PETRA.getSeaAccess(gameState, m1) : undefined;
		for (let m2 of market2.values())
		{
			if (onlyOurs && m1.id() >= m2.id())
				continue;
			if (!m2.position())
				continue;
			let access2 = PETRA.getLandAccess(gameState, m2);
			let sea2 = m2.hasClass("NavalMarket") ? PETRA.getSeaAccess(gameState, m2) : undefined;
			let land = access1 == access2 ? access1 : undefined;
			let sea = sea1 && sea1 == sea2 ? sea1 : undefined;
			if (!land && !sea)
				continue;
			if (land && PETRA.isLineInsideEnemyTerritory(gameState, m1.position(), m2.position()))
				continue;
			let gainMultiplier;
			if (land && traderTemplatesGains.landGainMultiplier)
				gainMultiplier = traderTemplatesGains.landGainMultiplier;
			else if (sea && traderTemplatesGains.navalGainMultiplier)
				gainMultiplier = traderTemplatesGains.navalGainMultiplier;
			else
				continue;
			let gain = Math.round(gainMultiplier * TradeGain(API3.SquareVectorDistance(m1.position(), m2.position()), mapSize));
			if (gain < this.minimalGain)
				continue;
			if (m1.foundationProgress() === undefined && m2.foundationProgress() === undefined)
			{
				if (accessIndex)
				{
					if (gameState.ai.accessibility.regionType[accessIndex] == "water" && sea == accessIndex)
					{
						if (gain < bestIndex.gain)
							continue;
						bestIndex = { "source": m1, "target": m2, "gain": gain, "land": land, "sea": sea };
					}
					else if (gameState.ai.accessibility.regionType[accessIndex] == "land" && land == accessIndex)
					{
						if (gain < bestIndex.gain)
							continue;
						bestIndex = { "source": m1, "target": m2, "gain": gain, "land": land, "sea": sea };
					}
					else if (gameState.ai.accessibility.regionType[accessIndex] == "land")
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
		if (owner == PlayerID)
			owner = this.tradeRoute.target.owner();
		if (owner != PlayerID && !this.warnedAllies[owner])
		{	// Warn an ally that we have a trade route with him
			PETRA.chatNewTradeRoute(gameState, owner);
			this.warnedAllies[owner] = true;
		}
	}

	if (accessIndex)
	{
		if (bestIndex.gain > 0)
			return bestIndex;
		else if (gameState.ai.accessibility.regionType[accessIndex] == "land" && bestLand.gain > 0)
			return bestLand;
		return false;
	}
	return true;
};

/** Called when a market was built or destroyed, and checks if trader orders should be changed */
PETRA.TradeManager.prototype.checkTrader = function(gameState, ent)
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

	let access = ent.hasClass("Ship") ? PETRA.getSeaAccess(gameState, ent) : PETRA.getLandAccess(gameState, ent);
	let possibleRoute = this.checkRoutes(gameState, access);
	// Warning:  presentRoute is from metadata, so contains entity ids
	if (!possibleRoute ||
	    possibleRoute.source.id() != presentRoute.source && possibleRoute.source.id() != presentRoute.target ||
	    possibleRoute.target.id() != presentRoute.source && possibleRoute.target.id() != presentRoute.target)
	{
		// Trader will be assigned in updateTrader
		ent.setMetadata(PlayerID, "route", undefined);
		if (!possibleRoute && !ent.hasClass("Ship"))
		{
			let closestBase = PETRA.getBestBase(gameState, ent, true);
			if (closestBase.accessIndex == access)
			{
				let closestBasePos = closestBase.anchor.position();
				ent.moveToRange(closestBasePos[0], closestBasePos[1], 0, 15);
				return;
			}
		}
		ent.stopMoving();
	}
};

PETRA.TradeManager.prototype.prospectForNewMarket = function(gameState, queues)
{
	if (queues.economicBuilding.hasQueuedUnitsWithClass("Market") || queues.dock.hasQueuedUnitsWithClass("Market"))
		return;
	if (!gameState.ai.HQ.canBuild(gameState, "structures/{civ}_market"))
		return;
	if (!gameState.updatingCollection("OwnMarkets", API3.Filters.byClass("Market"), gameState.getOwnStructures()).hasEntities() &&
	    !gameState.updatingCollection("diplo-ExclusiveAllyMarkets", API3.Filters.byClass("Market"), gameState.getExclusiveAllyEntities()).hasEntities())
		return;
	let template = gameState.getTemplate(gameState.applyCiv("structures/{civ}_market"));
	if (!template)
		return;
	this.checkRoutes(gameState);
	let marketPos = gameState.ai.HQ.findMarketLocation(gameState, template);
	if (!marketPos || marketPos[3] == 0)   // marketPos[3] is the expected gain
	{	// no position found
		if (gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities())
			gameState.ai.HQ.buildManager.setUnbuildable(gameState, gameState.applyCiv("structures/{civ}_market"));
		else
			this.routeProspection = false;
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
	let plan = new PETRA.ConstructionPlan(gameState, "structures/{civ}_market");
	if (!this.tradeRoute)
		plan.queueToReset = "economicBuilding";
	queues.economicBuilding.addPlan(plan);
};

PETRA.TradeManager.prototype.isNewMarketWorth = function(expectedGain)
{
	if (expectedGain < this.minimalGain)
		return false;
	if (this.potentialTradeRoute && expectedGain < 2*this.potentialTradeRoute.gain &&
		expectedGain < this.potentialTradeRoute.gain + 20)
		return false;
	return true;
};

PETRA.TradeManager.prototype.update = function(gameState, events, queues)
{
	if (gameState.ai.HQ.canBarter)
		this.performBarter(gameState);

	if (this.Config.difficulty <= 1)
		return;

	if (this.checkEvents(gameState, events))  // true if one market was built or destroyed
	{
		this.traders.forEach(ent => { this.checkTrader(gameState, ent); });
		this.checkRoutes(gameState);
	}

	if (this.tradeRoute)
	{
		this.traders.forEach(ent => { this.updateTrader(gameState, ent); });
		if (gameState.ai.playedTurn % 5 == 0)
			this.trainMoreTraders(gameState, queues);
		if (gameState.ai.playedTurn % 20 == 0 && this.traders.length >= 2)
			gameState.ai.HQ.researchManager.researchTradeBonus(gameState, queues);
		if (gameState.ai.playedTurn % 60 == 0)
			this.setTradingGoods(gameState);
	}

	if (this.routeProspection)
		this.prospectForNewMarket(gameState, queues);
};

PETRA.TradeManager.prototype.routeEntToId = function(route)
{
	if (!route)
		return undefined;

	let ret = {};
	for (let key in route)
	{
		if (key == "source" || key == "target")
		{
			if (!route[key])
				return undefined;
			ret[key] = route[key].id();
		}
		else
			ret[key] = route[key];
	}
	return ret;
};

PETRA.TradeManager.prototype.routeIdToEnt = function(gameState, route)
{
	if (!route)
		return undefined;

	let ret = {};
	for (let key in route)
	{
		if (key == "source" || key == "target")
		{
			ret[key] = gameState.getEntityById(route[key]);
			if (!ret[key])
				return undefined;
		}
		else
			ret[key] = route[key];
	}
	return ret;
};

PETRA.TradeManager.prototype.Serialize = function()
{
	return {
		"tradeRoute": this.routeEntToId(this.tradeRoute),
		"potentialTradeRoute": this.routeEntToId(this.potentialTradeRoute),
		"routeProspection": this.routeProspection,
		"targetNumTraders": this.targetNumTraders,
		"warnedAllies": this.warnedAllies
	};
};

PETRA.TradeManager.prototype.Deserialize = function(gameState, data)
{
	for (let key in data)
	{
		if (key == "tradeRoute" || key == "potentialTradeRoute")
			this[key] = this.routeIdToEnt(gameState, data[key]);
		else
			this[key] = data[key];
	}
};
