var PETRA = function(m)
{

/**
 * Naval Manager
 * Will deal with anything ships.
 * -Basically trade over water (with fleets and goals commissioned by the economy manager)
 * -Defense over water (commissioned by the defense manager)
 * -Transport of units over water (a few units).
 * -Scouting, ultimately.
 * Also deals with handling docks, making sure we have access and stuffs like that.
 */

m.NavalManager = function(Config)
{
	this.Config = Config;

	// ship subCollections. Also exist for land zones, idem, not caring.
	this.seaShips = [];
	this.seaTransportShips = [];
	this.seaWarShips = [];
	this.seaFishShips = [];

	// wanted NB per zone.
	this.wantedTransportShips = [];
	this.wantedWarShips = [];
	this.wantedFishShips = [];
	// needed NB per zone.
	this.neededTransportShips = [];
	this.neededWarShips = [];

	this.transportPlans = [];

	// shore-line regions where we can load and unload units
	this.landingZones = {};
};

/** More initialisation for stuff that needs the gameState */
m.NavalManager.prototype.init = function(gameState, deserializing)
{
	// docks
	this.docks = gameState.getOwnStructures().filter(API3.Filters.byClassesOr(["Dock", "Shipyard"]));
	this.docks.registerUpdates();

	this.ships = gameState.getOwnUnits().filter(API3.Filters.and(API3.Filters.byClass("Ship"), API3.Filters.not(API3.Filters.byMetadata(PlayerID, "role", "trader"))));
	// note: those two can overlap (some transport ships are warships too and vice-versa).
	this.transportShips = this.ships.filter(API3.Filters.and(API3.Filters.byCanGarrison(), API3.Filters.not(API3.Filters.byClass("FishingBoat"))));
	this.warShips = this.ships.filter(API3.Filters.byClass("Warship"));
	this.fishShips = this.ships.filter(API3.Filters.byClass("FishingBoat"));

	this.ships.registerUpdates();
	this.transportShips.registerUpdates();
	this.warShips.registerUpdates();
	this.fishShips.registerUpdates();

	let availableFishes = {};
	for (let fish of gameState.getFishableSupplies().values())
	{
		let sea = this.getFishSea(gameState, fish);
		if (sea && availableFishes[sea])
			availableFishes[sea] += fish.resourceSupplyAmount();
		else if (sea)
			availableFishes[sea] = fish.resourceSupplyAmount();
	}

	for (let i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
	{
		if (!gameState.ai.HQ.navalRegions[i])
		{
			// push dummies
			this.seaShips.push(undefined);
			this.seaTransportShips.push(undefined);
			this.seaWarShips.push(undefined);
			this.seaFishShips.push(undefined);
			this.wantedTransportShips.push(0);
			this.wantedWarShips.push(0);
			this.wantedFishShips.push(0);
			this.neededTransportShips.push(0);
			this.neededWarShips.push(0);
		}
		else
		{
			let collec = this.ships.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaShips.push(collec);
			collec = this.transportShips.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaTransportShips.push(collec);
			collec = this.warShips.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaWarShips.push(collec);
			collec = this.fishShips.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaFishShips.push(collec);
			this.wantedTransportShips.push(0);
			this.wantedWarShips.push(0);
			if (availableFishes[i] && availableFishes[i] > 1000)
				this.wantedFishShips.push(this.Config.Economy.targetNumFishers);
			else
				this.wantedFishShips.push(0);
			this.neededTransportShips.push(0);
			this.neededWarShips.push(0);
		}
	}

	if (deserializing)
		return;

	// determination of the possible landing zones
	let width = gameState.getPassabilityMap().width;
	let length = width * gameState.getPassabilityMap().height;
	for (let i = 0; i < length; ++i)
	{
		let land = gameState.ai.accessibility.landPassMap[i];
		if (land < 2)
			continue;
		let naval = gameState.ai.accessibility.navalPassMap[i];
		if (naval < 2)
			continue;
		if (!this.landingZones[land])
			this.landingZones[land] = {};
		if (!this.landingZones[land][naval])
		    this.landingZones[land][naval] = new Set();
		this.landingZones[land][naval].add(i);
	}
	// and keep only thoses with enough room around when possible
	for (let land in this.landingZones)
	{
		for (let sea in this.landingZones[land])
		{
			let landing = this.landingZones[land][sea];
			let nbaround = {};
			let nbcut = 0;
			for (let i of landing)
			{
				let nb = 0;
				if (landing.has(i-1))
					nb++;
				if (landing.has(i+1))
					nb++;
				if (landing.has(i+width))
					nb++;
				if (landing.has(i-width))
					nb++;
				nbaround[i] = nb;
				nbcut = Math.max(nb, nbcut);
			}
			nbcut = Math.min(2, nbcut);
			for (let i of landing)
			{
				if (nbaround[i] < nbcut)
					landing.delete(i);
			}
		}
	}

	// Assign our initial docks and ships
	for (let ship of this.ships.values())
		this.setShipIndex(gameState, ship);
	for (let dock of this.docks.values())
		m.setAccessIndices(gameState, dock);
};

m.NavalManager.prototype.updateFishingBoats = function(sea, num)
{
	if (this.wantedFishShips[sea])
		this.wantedFishShips[sea] = num;
};

m.NavalManager.prototype.resetFishingBoats = function(gameState, sea)
{
	if (sea !== undefined)
		this.wantedFishShips[sea] = 0;
	else
		this.wantedFishShips.fill(0);
};

m.NavalManager.prototype.setShipIndex = function(gameState, ship)
{
	let sea = gameState.ai.accessibility.getAccessValue(ship.position(), true);
	ship.setMetadata(PlayerID, "sea", sea);
};

/** Get the sea, cache it if not yet done and check if in opensea */
m.NavalManager.prototype.getFishSea = function(gameState, fish)
{
	let sea = fish.getMetadata(PlayerID, "sea");
	if (sea)
		return sea;
	const ntry = 4;
	const around = [ [-0.7,0.7], [0,1], [0.7,0.7], [1,0], [0.7,-0.7], [0,-1], [-0.7,-0.7], [-1,0] ];
	let pos = gameState.ai.accessibility.gamePosToMapPos(fish.position());
	let width = gameState.ai.accessibility.width;
	let k = pos[0] + pos[1]*width;
	sea = gameState.ai.accessibility.navalPassMap[k];
	fish.setMetadata(PlayerID, "sea", sea);
	let radius = 120 / gameState.ai.accessibility.cellSize / ntry;
	if (around.every(a =>
		{
			for (let t = 0; t < ntry; ++t)
			{
				let i = pos[0] + Math.round(a[0]*radius*(ntry-t));
				let j = pos[1] + Math.round(a[1]*radius*(ntry-t));
				if (i < 0 || i >= width || j < 0 || j >= width)
					continue;
				if (gameState.ai.accessibility.landPassMap[i + j*width] === 1)
				{
					let navalPass = gameState.ai.accessibility.navalPassMap[i + j*width];
					if (navalPass == sea)
						return true;
					else if (navalPass == 1)  // we could be outside the map
						continue;
				}
				return false;
			}
			return true;
		}))
		fish.setMetadata(PlayerID, "opensea", true);
	return sea;
};

/** check if we can safely fish at the fish position */
m.NavalManager.prototype.canFishSafely = function(gameState, fish)
{
	if (fish.getMetadata(PlayerID, "opensea"))
		return true;
	const ntry = 2;
	const around = [ [-0.7,0.7], [0,1], [0.7,0.7], [1,0], [0.7,-0.7], [0,-1], [-0.7,-0.7], [-1,0] ];
	let territoryMap = gameState.ai.HQ.territoryMap;
	let width = territoryMap.width;
	let radius = 120 / territoryMap.cellSize / ntry;
	let pos = territoryMap.gamePosToMapPos(fish.position());
	return around.every(a =>
		{
			for (let t = 0; t < ntry; ++t)
			{
				let i = pos[0] + Math.round(a[0]*radius*(ntry-t));
				let j = pos[1] + Math.round(a[1]*radius*(ntry-t));
				if (i < 0 || i >= width || j < 0 || j >= width)
					continue;
				let owner = territoryMap.getOwnerIndex(i + j*width);
				if (owner != 0 && gameState.isPlayerEnemy(owner))
					return false;
			}
			return true;
		});
};

/** get the list of seas (or lands) around this region not connected by a dock */
m.NavalManager.prototype.getUnconnectedSeas = function(gameState, region)
{
	let seas = gameState.ai.accessibility.regionLinks[region].slice();
	this.docks.forEach(function (dock) {
		if (!dock.hasClass("Dock") || dock.getMetadata(PlayerID, "access") !== region)
			return;
		let i = seas.indexOf(dock.getMetadata(PlayerID, "sea"));
		if (i !== -1)
			seas.splice(i--,1);
	});
	return seas;
};

m.NavalManager.prototype.checkEvents = function(gameState, queues, events)
{
	for (let evt of events.Create)
	{
		if (!evt.entity)
			continue;
		let ent = gameState.getEntityById(evt.entity);
		if (ent && ent.isOwn(PlayerID) && ent.foundationProgress() !== undefined && (ent.hasClass("Dock") || ent.hasClass("Shipyard")))
			m.setAccessIndices(gameState, ent);
	}

	for (let evt of events.TrainingFinished)
	{
		if (!evt.entities)
			continue;
		for (let entId of evt.entities)
		{
			let ent = gameState.getEntityById(entId);
			if (!ent || !ent.hasClass("Ship") || !ent.isOwn(PlayerID))
				continue;
			this.setShipIndex(gameState, ent);
		}
	}

	for (let evt of events.Destroy)
	{
		if (!evt.entityObj || evt.entityObj.owner() !== PlayerID || !evt.metadata || !evt.metadata[PlayerID])
			continue;
		if (!evt.entityObj.hasClass("Ship") || !evt.metadata[PlayerID].transporter)
			continue;
		let plan = this.getPlan(evt.metadata[PlayerID].transporter);
		if (!plan)
			continue;

		let shipId = evt.entityObj.id();
		if (this.Config.debug > 1)
			API3.warn("one ship " + shipId + " from plan " + plan.ID + " destroyed during " + plan.state);
		if (plan.state === "boarding")
		{
			// just reset the units onBoard metadata and wait for a new ship to be assigned to this plan
			plan.units.forEach(function (ent) {
				if (ent.getMetadata(PlayerID, "onBoard") === "onBoard" && ent.position() ||
				    ent.getMetadata(PlayerID, "onBoard") === shipId)
					ent.setMetadata(PlayerID, "onBoard", undefined);
			});
			plan.needTransportShips = !plan.transportShips.hasEntities();
		}
		else if (plan.state === "sailing")
		{
			let endIndex = plan.endIndex;
			for (let ent of plan.units.values())
			{
				if (!ent.position())  // unit from another ship of this plan ... do nothing
					continue;
				let access = gameState.ai.accessibility.getAccessValue(ent.position());
				let endPos = ent.getMetadata(PlayerID, "endPos");
				ent.setMetadata(PlayerID, "transport", undefined);
				ent.setMetadata(PlayerID, "onBoard", undefined);
				ent.setMetadata(PlayerID, "endPos", undefined);
				// nothing else to do if access = endIndex as already at destination
				// otherwise, we should require another transport
				// TODO if attacking and no more ships available, remove the units from the attack
				// to avoid delaying it too much
				if (access !== endIndex)
					this.requireTransport(gameState, ent, access, endIndex, endPos);
			}
		}
	}

	for (let evt of events.OwnershipChanged)	// capture events
	{
		if (evt.to !== PlayerID)
			continue;
		let ent = gameState.getEntityById(evt.entity);
		if (ent && (ent.hasClass("Dock") || ent.hasClass("Shipyard")))
			m.setAccessIndices(gameState, ent);
	}
};


m.NavalManager.prototype.getPlan = function(ID)
{
	for (let plan of this.transportPlans)
		if (plan.ID === ID)
			return plan;
	return undefined;
};

m.NavalManager.prototype.addPlan = function(plan)
{
	this.transportPlans.push(plan);
};

/**
 * complete already existing plan or create a new one for this requirement
 * (many units can then call this separately and end up in the same plan)
 * TODO  check garrison classes
 */
m.NavalManager.prototype.requireTransport = function(gameState, ent, startIndex, endIndex, endPos)
{
	if (!ent.canGarrison())
		return false;

	if (ent.getMetadata(PlayerID, "transport") !== undefined)
	{
		if (this.Config.debug > 0)
			API3.warn("Petra naval manager error: unit " + ent.id() + " has already required a transport");
		return false;
	}

	let plans = [];
	for (let plan of this.transportPlans)
	{
		if (plan.startIndex != startIndex || plan.endIndex != endIndex || plan.state != "boarding")
			continue;
		// Limit the number of siege units per transport to avoid problems when ungarrisoning
		if (m.isSiegeUnit(ent) && plan.units.filter(unit => m.isSiegeUnit(unit)).length > 3)
			continue;
		plans.push(plan);
	}

	if (plans.length)
	{
		plans.sort(plan => plan.units.length);
		plans[0].addUnit(ent, endPos);
		return true;
	}

	let plan = new m.TransportPlan(gameState, [ent], startIndex, endIndex, endPos);
	if (plan.failed)
	{
		if (this.Config.debug > 1)
			API3.warn(">>>> transport plan aborted <<<<");
		return false;
	}
	plan.init(gameState);
	this.transportPlans.push(plan);
	return true;
};

/** split a transport plan in two, moving all entities not yet affected to a ship in the new plan */
m.NavalManager.prototype.splitTransport = function(gameState, plan)
{
	if (this.Config.debug > 1)
		API3.warn(">>>> split of transport plan started <<<<");
	let newplan = new m.TransportPlan(gameState, [], plan.startIndex, plan.endIndex, plan.endPos);
	if (newplan.failed)
	{
		if (this.Config.debug > 1)
			API3.warn(">>>> split of transport plan aborted <<<<");
		return false;
	}
	newplan.init(gameState);

	for (let ent of plan.needSplit)
	{
		if (ent.getMetadata(PlayerID, "onBoard"))	// Should never happen.
			continue;
		newplan.addUnit(ent, ent.getMetadata(PlayerID, "endPos"));
		plan.units.updateEnt(ent);
	}

	if (newplan.units.length)
		this.transportPlans.push(newplan);
	return newplan.units.length != 0;
};

/**
 * create a transport from a garrisoned ship to a land location
 * needed at start game when starting with a garrisoned ship
 */
m.NavalManager.prototype.createTransportIfNeeded = function(gameState, fromPos, toPos, toAccess)
{
	let fromAccess = gameState.ai.accessibility.getAccessValue(fromPos);
	if (fromAccess !== 1)
		return;
	if (toAccess < 2)
		return;

	for (let ship of this.ships.values())
	{
		if (!ship.isGarrisonHolder() || !ship.garrisoned().length)
			continue;
		if (ship.getMetadata(PlayerID, "transporter") !== undefined)
			continue;
		let units = [];
		for (let entId of ship.garrisoned())
			units.push(gameState.getEntityById(entId));
		// TODO check that the garrisoned units have not another purpose
		let plan = new m.TransportPlan(gameState, units, fromAccess, toAccess, toPos, ship);
		if (plan.failed)
			continue;
		plan.init(gameState);
		this.transportPlans.push(plan);
	}
};

// set minimal number of needed ships when a new event (new base or new attack plan)
m.NavalManager.prototype.setMinimalTransportShips = function(gameState, sea, number)
{
	if (!sea)
		return;
	if (this.wantedTransportShips[sea] < number )
		this.wantedTransportShips[sea] = number;
};

// bumps up the number of ships we want if we need more.
m.NavalManager.prototype.checkLevels = function(gameState, queues)
{
	if (queues.ships.hasQueuedUnits())
		return;

	for (let sea = 0; sea < this.neededTransportShips.length; sea++)
		this.neededTransportShips[sea] = 0;

	for (let plan of this.transportPlans)
	{
		if (!plan.needTransportShips || plan.units.length < 2)
			continue;
		let sea = plan.sea;
		if (gameState.countOwnQueuedEntitiesWithMetadata("sea", sea) > 0 ||
			this.seaTransportShips[sea].length < this.wantedTransportShips[sea])
			continue;
		++this.neededTransportShips[sea];
		if (this.wantedTransportShips[sea] === 0 || this.seaTransportShips[sea].length < plan.transportShips.length + 2)
		{
			++this.wantedTransportShips[sea];
			return;
		}
	}

	for (let sea = 0; sea < this.neededTransportShips.length; sea++)
		if (this.neededTransportShips[sea] > 2)
			++this.wantedTransportShips[sea];
};

m.NavalManager.prototype.maintainFleet = function(gameState, queues)
{
	if (queues.ships.hasQueuedUnits())
		return;
	if (!this.docks.filter(API3.Filters.isBuilt()).hasEntities())
		return;
	// check if we have enough transport ships per region.
	for (let sea = 0; sea < this.seaShips.length; ++sea)
	{
		if (this.seaShips[sea] === undefined)
			continue;
		if (gameState.countOwnQueuedEntitiesWithMetadata("sea", sea) > 0)
			continue;

		if (this.seaTransportShips[sea].length < this.wantedTransportShips[sea])
		{
			let template = this.getBestShip(gameState, sea, "transport");
			if (template)
			{
				queues.ships.addPlan(new m.TrainingPlan(gameState, template, { "sea": sea }, 1, 1));
				continue;
			}
		}


		if (this.seaFishShips[sea].length < this.wantedFishShips[sea])
		{
			let template = this.getBestShip(gameState, sea, "fishing");
			if (template)
			{
				queues.ships.addPlan(new m.TrainingPlan(gameState, template, { "base": 0, "role": "worker", "sea": sea }, 1, 1));
				continue;
			}
		}
	}
};

/** assigns free ships to plans that need some */
m.NavalManager.prototype.assignShipsToPlans = function(gameState)
{
	for (let plan of this.transportPlans)
		if (plan.needTransportShips)
			plan.assignShip(gameState);
};

/** Return true if this ship is likeky (un)garrisoning units */ 
m.NavalManager.prototype.isShipBoarding = function(ship)
{
	if (!ship.position())
		return false;
	let plan = this.getPlan(ship.getMetadata(PlayerID, "transporter"));
	if (!plan || !plan.boardingPos[ship.id()])
		return false;
	return API3.SquareVectorDistance(plan.boardingPos[ship.id()], ship.position()) < plan.boardingRange;
};

/** let blocking ships move apart from active ships (waiting for a better pathfinder) */
m.NavalManager.prototype.moveApart = function(gameState)
{
	let blockedShips = [];

	for (let ship of this.ships.values())
	{
		let shipPosition = ship.position();
		if (!shipPosition)
			continue;
		if (ship.getMetadata(PlayerID, "transporter") !== undefined && this.isShipBoarding(ship))
			continue;

		let unitAIState = ship.unitAIState();
		if (ship.getMetadata(PlayerID, "transporter") !== undefined ||
		    unitAIState == "INDIVIDUAL.GATHER.APPROACHING" ||
		    unitAIState == "INDIVIDUAL.RETURNRESOURCE.APPROACHING")
		{
			let previousPosition = ship.getMetadata(PlayerID, "previousPosition");
			if (!previousPosition || previousPosition[0] != shipPosition[0] ||
			                         previousPosition[1] != shipPosition[1])
			{
				ship.setMetadata(PlayerID, "previousPosition", shipPosition)
				continue;
			}
			ship.moveToRange(shipPosition[0], shipPosition[1], 30, 30);
			blockedShips.push(ship);
		}
		else if (ship.isIdle())
		{
			let previousIdlePosition = ship.getMetadata(PlayerID, "previousIdlePosition");
			if (previousIdlePosition && previousIdlePosition[0] == shipPosition[0] &&
			                            previousIdlePosition[1] == shipPosition[1])
				continue;
			ship.setMetadata(PlayerID, "previousIdlePosition", shipPosition);
			// Check if there are some treasure around
			if (m.gatherTreasure(gameState, ship, true))
				continue;
			// Do not stay idle near a dock to not disturb other ships
			let sea = ship.getMetadata(PlayerID, "sea");
			for (let dock of gameState.getAllyStructures().filter(API3.Filters.byClass("Dock")).values())
			{
				if (m.getSeaAccess(gameState, dock) != sea)
					continue;
				if (API3.SquareVectorDistance(shipPosition, dock.position()) > 4900)
					continue;
				ship.moveToRange(dock.position()[0], dock.position()[1], 70, 70);
			}

		}
	}

	for (let ship of gameState.ai.HQ.tradeManager.traders.filter(API3.Filters.byClass("Ship")).values())
	{
		let shipPosition = ship.position();
		if (!shipPosition)
			continue;

		let unitAIState = ship.unitAIState();
		if (unitAIState == "INDIVIDUAL.TRADE.APPROACHINGMARKET")
		{
			let previousPosition = ship.getMetadata(PlayerID, "previousPosition");
			if (!previousPosition || previousPosition[0] != shipPosition[0] ||
			                         previousPosition[1] != shipPosition[1])
			{
				ship.setMetadata(PlayerID, "previousPosition", shipPosition)
				continue;
			}
			ship.moveToRange(shipPosition[0], shipPosition[1], 30, 30);
			blockedShips.push(ship);
		}
		else if (ship.isIdle())
		{
			let previousIdlePosition = ship.getMetadata(PlayerID, "previousIdlePosition");
			if (previousIdlePosition && previousIdlePosition[0] == shipPosition[0] &&
			                            previousIdlePosition[1] == shipPosition[1])
				continue;
			ship.setMetadata(PlayerID, "previousIdlePosition", shipPosition);
			// Check if there are some treasure around
			if (m.gatherTreasure(gameState, ship, true))
				continue;
			// Do not stay idle near a dock to not disturb other ships
			let sea = ship.getMetadata(PlayerID, "sea");
			for (let dock of gameState.getAllyStructures().filter(API3.Filters.byClass("Dock")).values())
			{
				if (m.getSeaAccess(gameState, dock) != sea)
					continue;
				if (API3.SquareVectorDistance(shipPosition, dock.position()) > 4900)
					continue;
				ship.moveToRange(dock.position()[0], dock.position()[1], 70, 70);
			}
		}
	}

	for (let ship of blockedShips)
	{
		let shipPosition = ship.position();
		let sea = ship.getMetadata(PlayerID, "sea");
		for (let blockingShip of this.seaShips[sea].values())
		{
			if (blockedShips.indexOf(ship.id()) != -1 || !blockingShip.position())
				continue;
			let distSquare = API3.SquareVectorDistance(shipPosition, blockingShip.position());
			let unitAIState = blockingShip.unitAIState();
			if (blockingShip.getMetadata(PlayerID, "transporter") === undefined &&
			    unitAIState != "INDIVIDUAL.GATHER.APPROACHING" &&
			    unitAIState != "INDIVIDUAL.RETURNRESOURCE.APPROACHING")
			{
				if (distSquare < 1600)
					blockingShip.moveToRange(shipPosition[0], shipPosition[1], 40, 40);
			}
			else if (distSquare < 900)
				blockingShip.moveToRange(shipPosition[0], shipPosition[1], 30, 30);

		}

		for (let blockingShip of gameState.ai.HQ.tradeManager.traders.filter(API3.Filters.byClass("Ship")).values())
		{
			if (blockingShip.getMetadata(PlayerID, "sea") != sea)
				continue;
			if (blockedShips.indexOf(ship.id()) != -1 || !blockingShip.position())
				continue;
			let distSquare = API3.SquareVectorDistance(shipPosition, blockingShip.position());
			let unitAIState = blockingShip.unitAIState();
			if (unitAIState != "INDIVIDUAL.TRADE.APPROACHINGMARKET")
			{
				if (distSquare < 1600)
					blockingShip.moveToRange(shipPosition[0], shipPosition[1], 40, 40);
			}
			else if (distSquare < 900)
				blockingShip.moveToRange(shipPosition[0], shipPosition[1], 30, 30);
		}
	}
};

m.NavalManager.prototype.buildNavalStructures = function(gameState, queues)
{
	if (!gameState.ai.HQ.navalMap || !gameState.ai.HQ.baseManagers[1])
		return;

	if (gameState.ai.HQ.getAccountedPopulation(gameState) > this.Config.Economy.popForDock)
	{
		if (queues.dock.countQueuedUnitsWithClass("NavalMarket") === 0 &&
			!gameState.getOwnStructures().filter(API3.Filters.and(API3.Filters.byClass("NavalMarket"), API3.Filters.isFoundation())).hasEntities() &&
			gameState.ai.HQ.canBuild(gameState, "structures/{civ}_dock"))
		{
			let dockStarted = false;
			for (let base of gameState.ai.HQ.baseManagers)
			{
				if (dockStarted)
					break;
				if (!base.anchor || base.constructing)
					continue;
				let remaining = this.getUnconnectedSeas(gameState, base.accessIndex);
				for (let sea of remaining)
				{
					if (!gameState.ai.HQ.navalRegions[sea])
						continue;
					let wantedLand = {};
					wantedLand[base.accessIndex] = true;
					queues.dock.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_dock", { "land": wantedLand, "sea": sea }));
					dockStarted = true;
					break;
				}
			}
		}
	}

	if (gameState.currentPhase() < 2 || gameState.ai.HQ.getAccountedPopulation(gameState) < this.Config.Economy.popPhase2 + 15 ||
	    queues.militaryBuilding.hasQueuedUnits())
		return;
	if (!this.docks.filter(API3.Filters.byClass("Dock")).hasEntities() ||
	     this.docks.filter(API3.Filters.byClass("Shipyard")).hasEntities())
		return;
	// Use in priority resources to build a market
	if (!gameState.getOwnEntitiesByClass("BarterMarket", true).hasEntities() &&
	    gameState.ai.HQ.canBuild(gameState, "structures/{civ}_market"))
		return;
	let template;
	if (gameState.ai.HQ.canBuild(gameState, "structures/{civ}_super_dock"))
		template = "structures/{civ}_super_dock";
	else if (gameState.ai.HQ.canBuild(gameState, "structures/{civ}_shipyard"))
		template = "structures/{civ}_shipyard";
	else
		return;
	let wantedLand = {};
	for (let base of gameState.ai.HQ.baseManagers)
		if (base.anchor)
			wantedLand[base.accessIndex] = true;
	let sea = this.docks.toEntityArray()[0].getMetadata(PlayerID, "sea");
	queues.militaryBuilding.addPlan(new m.ConstructionPlan(gameState, template, { "land": wantedLand, "sea": sea }));
};

/** goal can be either attack (choose ship with best arrowCount) or transport (choose ship with best capacity) */
m.NavalManager.prototype.getBestShip = function(gameState, sea, goal)
{
	let civ = gameState.getPlayerCiv();
	let trainableShips = [];
	gameState.getOwnTrainingFacilities().filter(API3.Filters.byMetadata(PlayerID, "sea", sea)).forEach(function(ent) {
		let trainables = ent.trainableEntities(civ);
		for (let trainable of trainables)
		{
			if (gameState.isTemplateDisabled(trainable))
				continue;
			let template = gameState.getTemplate(trainable);
			if (template && template.hasClass("Ship") && trainableShips.indexOf(trainable) === -1)
				trainableShips.push(trainable);
		}
	});

	let best = 0;
	let bestShip;
	let limits = gameState.getEntityLimits();
	let current = gameState.getEntityCounts();
	for (let trainable of trainableShips)
	{
		let template = gameState.getTemplate(trainable);
		if (!template.available(gameState))
			continue;

		let category = template.trainingCategory();
		if (category && limits[category] && current[category] >= limits[category])
			continue;

		let arrows = +(template.getDefaultArrow() || 0);
		if (goal === "attack")    // choose the maximum default arrows
		{
			if (best > arrows)
				continue;
			best = arrows;
		}
		else if (goal === "transport")   // choose the maximum capacity, with a bonus if arrows or if siege transport
		{
			let capacity = +(template.garrisonMax() || 0);
			if (capacity < 2)
				continue;
			capacity += 10*arrows;
			if (MatchesClassList(template.garrisonableClasses(), "Siege"))
				capacity += 50;
			if (best > capacity)
				continue;
			best = capacity;
		}
		else if (goal === "fishing")
			if (!template.hasClass("FishingBoat"))
				continue;
		bestShip = trainable;
	}
	return bestShip;
};

m.NavalManager.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("Naval Manager update");

	this.checkEvents(gameState, queues, events);

	// close previous transport plans if finished
	for (let i = 0; i < this.transportPlans.length; ++i)
	{
		let remaining = this.transportPlans[i].update(gameState);
		if (remaining)
			continue;
		if (this.Config.debug > 1)
			API3.warn("no more units on transport plan " + this.transportPlans[i].ID);
		this.transportPlans[i].releaseAll();
		this.transportPlans.splice(i--, 1);
	}
	// assign free ships to plans which need them
	this.assignShipsToPlans(gameState);

	// and require for more ships/structures if needed
	if (gameState.ai.playedTurn % 3 === 0)
	{
		this.checkLevels(gameState, queues);
		this.maintainFleet(gameState, queues);
		this.buildNavalStructures(gameState, queues);
	}
	// let inactive ships move apart from active ones (waiting for a better pathfinder)
	this.moveApart(gameState);

	Engine.ProfileStop();
};

m.NavalManager.prototype.Serialize = function()
{
	let properties = {
		"wantedTransportShips": this.wantedTransportShips,
		"wantedWarShips": this.wantedWarShips,
		"wantedFishShips": this.wantedFishShips,
		"neededTransportShips": this.neededTransportShips,
		"neededWarShips": this.neededWarShips,
		"landingZones": this.landingZones
	};

	let transports = {};
	for (let plan in this.transportPlans)
		transports[plan] = this.transportPlans[plan].Serialize();

	return { "properties": properties, "transports": transports };
};

m.NavalManager.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.properties)
		this[key] = data.properties[key];

	this.transportPlans = [];
	for (let i in data.transports)
	{
		let dataPlan = data.transports[i];
		let plan = new m.TransportPlan(gameState, [], dataPlan.startIndex, dataPlan.endIndex, dataPlan.endPos);
		plan.Deserialize(dataPlan);
		plan.init(gameState);
		this.transportPlans.push(plan);
	}
};


return m;
}(PETRA);
