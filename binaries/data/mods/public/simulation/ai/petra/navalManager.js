var PETRA = function(m)
{

/* Naval Manager
 Will deal with anything ships.
 -Basically trade over water (with fleets and goals commissioned by the economy manager)
 -Defense over water (commissioned by the defense manager)
	-subtask being patrols, escort, naval superiority.
 -Transport of units over water (a few units).
 -Scouting, ultimately.
 Also deals with handling docks, making sure we have access and stuffs like that.
 Does not build them though, that's for the base manager to handle.
 */

m.NavalManager = function(Config)
{
	this.Config = Config;
	// accessibility zones for which we have a dock.
	// Connexion is described as [landindex] = [seaIndexes];
	// technically they also exist for sea zones but I don't care.
	this.landZoneDocked = [];	
	// list of seas I have a dock on.
	this.accessibleSeas = [];
	
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

// More initialisation for stuff that needs the gameState
m.NavalManager.prototype.init = function(gameState, queues)
{
	// finished docks
	this.docks = gameState.getOwnStructures().filter(API3.Filters.and(API3.Filters.byClassesOr(["Dock", "Shipyard"]),
		API3.Filters.not(API3.Filters.isFoundation())));
	this.docks.allowQuickIter();
	this.docks.registerUpdates();
	
	this.ships = gameState.getOwnUnits().filter(API3.Filters.byClass("Ship"));
	// note: those two can overlap (some transport ships are warships too and vice-versa).
	this.transportShips = this.ships.filter(API3.Filters.and(API3.Filters.byCanGarrison(), API3.Filters.not(API3.Filters.byClass("FishingBoat"))));
	this.warShips = this.ships.filter(API3.Filters.byClass("Warship"));
	this.fishShips = this.ships.filter(API3.Filters.byClass("FishingBoat"));

	this.ships.registerUpdates();
	this.transportShips.registerUpdates();
	this.warShips.registerUpdates();
	this.fishShips.registerUpdates();
	
	for (var i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
	{
		if (gameState.ai.accessibility.regionType[i] !== "water")
		{
			// push dummies
			this.seaShips.push(new API3.EntityCollection(gameState.sharedScript));
			this.seaTransportShips.push(new API3.EntityCollection(gameState.sharedScript));
			this.seaWarShips.push(new API3.EntityCollection(gameState.sharedScript));
			this.seaFishShips.push(new API3.EntityCollection(gameState.sharedScript));
			this.wantedTransportShips.push(0);
			this.wantedWarShips.push(0);
			this.wantedFishShips.push(0);
			this.neededTransportShips.push(0);
			this.neededWarShips.push(0);
		}
		else
		{
			var collec = this.ships.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaShips.push(collec);
			collec = this.transportShips.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaTransportShips.push(collec);
			var collec = this.warShips.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaWarShips.push(collec);
			var collec = this.fishShips.filter(API3.Filters.byMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaFishShips.push(collec);
			this.wantedTransportShips.push(0);
			this.wantedWarShips.push(0);
			this.wantedFishShips.push(this.Config.Economy.targetNumFishers);
			this.neededTransportShips.push(0);
			this.neededWarShips.push(0);
		}
		
		this.landZoneDocked.push([]);
	}

	// determination of the possible landing zones
	var width = gameState.getMap().width;
	var length = width * gameState.getMap().height;
	for (var i = 0; i < length; ++i)
	{
		var land = gameState.ai.accessibility.landPassMap[i];
		if (land < 2)
			continue;
		var naval = gameState.ai.accessibility.navalPassMap[i];
		if (naval < 2)
			continue;
		if (!this.landingZones[land])
			this.landingZones[land] = {};
		if (!this.landingZones[land][naval])
			this.landingZones[land][naval] = [];
		this.landingZones[land][naval].push(i);
	}
	// and keep only thoses with enough room around when possible
	for (var land in this.landingZones)
	{
		for (var sea in this.landingZones[land])
		{
			var nbmax = 0;
			for (var i = 0; i < this.landingZones[land][sea].length; i++)
			{
				var j = this.landingZones[land][sea][i];
				var nb = 0;
				if (this.landingZones[land][sea].indexOf(j-1) !== -1)
					nb++;
				if (this.landingZones[land][sea].indexOf(j+1) !== -1)
					nb++;
				if (this.landingZones[land][sea].indexOf(j+width) !== -1)
					nb++;
				if (this.landingZones[land][sea].indexOf(j-width) !== -1)
					nb++;
			}
			if (nb > nbmax)
				nbmax = nb;
			var nbcut = Math.min(2, nbmax);
			for (var i = 0; i < this.landingZones[land][sea].length; i++)
			{
				var j = this.landingZones[land][sea][i];
				var nb = 0;
				if (this.landingZones[land][sea].indexOf(j-1) !== -1)
					nb++;
				if (this.landingZones[land][sea].indexOf(j+1) !== -1)
					nb++;
				if (this.landingZones[land][sea].indexOf(j+width) !== -1)
					nb++;
				if (this.landingZones[land][sea].indexOf(j-width) !== -1)
					nb++;
				if (nb < nbcut)
					this.landingZones[land][sea].splice(i--, 1);
			}
		}
	}

	// load units and buildings from the config files
	var civ = gameState.playerData.civ;
	if (civ in this.Config.buildings.naval)
		this.bNaval = this.Config.buildings.naval[civ];
	else
		this.bNaval = this.Config.buildings.naval['default'];

	for (var i in this.bNaval)
		this.bNaval[i] = gameState.applyCiv(this.bNaval[i]);

	// Assign our docks
	var self = this;
	this.docks.forEach(function(dock) { self.assignDock(gameState, dock); });
};

m.NavalManager.prototype.resetFishingBoats = function(gameState)
{
	for (var i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
		this.wantedFishShips[i] = 0;
};

m.NavalManager.prototype.assignDock = function(gameState, dock)
{
	var land = dock.getMetadata(PlayerID, "access");
	if (land === undefined)
	{
		land = this.getDockIndex(gameState, dock, false);
		dock.setMetadata(PlayerID, "access", land);
	}
	var sea = dock.getMetadata(PlayerID, "sea");
	if (sea === undefined)
	{
		sea = this.getDockIndex(gameState, dock, true);
		dock.setMetadata(PlayerID, "sea", sea);
	}

	if (this.landZoneDocked[land].indexOf(sea) === -1)
		this.landZoneDocked[land].push(sea);
	if (this.accessibleSeas.indexOf(sea) === -1)
		this.accessibleSeas.push(sea);
};

// get the indices for our starting docks and those of our allies
// land index when onWater=false, sea indes when true
m.NavalManager.prototype.getDockIndex = function(gameState, dock, onWater)
{
	var index = gameState.ai.accessibility.getAccessValue(dock.position(), onWater);
	if (index < 2)
	{
		// pre-positioned docks are sometimes not well positionned
		var dockPos = dock.position();
		var radius = dock.footprintRadius();
		for (var i = 0; i < 16; i++)
		{
			var pos = [ dockPos[0] + radius*Math.cos(i*Math.PI/8), dockPos[1] + radius*Math.sin(i*Math.PI/8)];

			index = gameState.ai.accessibility.getAccessValue(pos, onWater);
			if (index >= 2)
				break;
		}
	}
	if (index < 2)
		API3.warn("ERROR in Petra navalManager because of dock position (onWater=" + onWater + ") index " + index);
	return index;
};

m.NavalManager.prototype.getUnconnectedSeas = function(gameState, region)
{
	var seas = gameState.ai.accessibility.regionLinks[region]
	if (seas.length === 0)
		return [];

	for (var i = 0; i < seas.length; ++i)
	{
		if (this.landZoneDocked[region].indexOf(seas[i]) !== -1)
			seas.splice(i--,1);
	}
	return seas;
};

m.NavalManager.prototype.checkEvents = function(gameState, queues, events)
{
	var evts = events["ConstructionFinished"];
	// TODO: probably check stuffs like a base destruction.
	for (var evt of evts)
	{
		if (!evt || !evt.newentity)
			continue;
		var entity = gameState.getEntityById(evt.newentity);
		if (entity && entity.hasClass("Dock") && entity.isOwn(PlayerID))
			this.assignDock(gameState, entity);
	}

	var evts = events["TrainingFinished"];
	for (var evt of evts)
	{
		if (!evt || !evt.entities)
			continue;
		for (var entId of evt.entities)
		{
			var entity = gameState.getEntityById(entId);
			if (!entity || !entity.hasClass("Ship") || !entity.isOwn(PlayerID))
				continue;
			var pos = gameState.ai.accessibility.gamePosToMapPos(entity.position());
			var index = pos[0] + pos[1]*gameState.ai.accessibility.width;
			var sea = gameState.ai.accessibility.navalPassMap[index];
			entity.setMetadata(PlayerID, "sea", sea);
		}
	}

	var evts = events["Destroy"];
	for (var evt of evts)
	{
		if (!evt.entityObj || evt.entityObj.owner() !== PlayerID || !evt.metadata || !evt.metadata[PlayerID])
			continue;
		if (!evt.entityObj.hasClass("Ship") || !evt.metadata[PlayerID]["transporter"])
			continue;
		var plan = this.getPlan(evt.metadata[PlayerID]["transporter"]);
		if (!plan)
			continue;

		var shipId = evt.entityObj.id();
		if (this.Config.debug > 0)
			API3.warn("one ship " + shipId + " from plan " + plan.ID + " destroyed during " + plan.state);
		if (plan.state === "boarding")
		{
			// just reset the units onBoard metadata and wait for a new ship to be assigned to this plan
			plan.units.forEach(function (ent) {
				if ((ent.getMetadata(PlayerID, "onBoard") === "onBoard" && ent.position())
					|| ent.getMetadata(PlayerID, "onBoard") === shipId)
					ent.setMetadata(PlayerID, "onBoard", undefined);
			});
			plan.needTransportShips = (plan.transportShips.length === 0);
		}
		else if (plan.state === "sailing")
		{
			var endIndex = plan.endIndex;
			var self = this;
			plan.units.forEach(function (ent) {
				if (!ent.position())  // unit from another ship of this plan ... do nothing
					return;
				var access = gameState.ai.accessibility.getAccessValue(ent.position());
				var endPos = ent.getMetadata(PlayerID, "endPos");
				ent.setMetadata(PlayerID, "transport", undefined);
				ent.setMetadata(PlayerID, "onBoard", undefined);
				ent.setMetadata(PlayerID, "endPos", undefined);
				// nothing else to do if access = endIndex as already at destination
				// otherwise, we should require another transport
				// TODO if attacking and no more ships available, remove the units from the attack
				// to avoid delaying it too much
				if (access !== endIndex)
					self.requireTransport(gameState, ent, access, endIndex, endPos);
			});
		}
	}
};


m.NavalManager.prototype.getPlan = function(ID)
{
	for (var plan of this.transportPlans)
	{
		if (plan.ID !== ID)
			continue;
		return plan;
	}
	return undefined;
};

m.NavalManager.prototype.addPlan = function(plan)
{
	this.transportPlans.push(plan);
};

// complete already existing plan or create a new one for this requirement
// (many units can then call this separately and end up in the same plan)
// TODO  check garrison classes
m.NavalManager.prototype.requireTransport = function(gameState, entity, startIndex, endIndex, endPos)
{
	if (entity.getMetadata(PlayerID, "transport") !== undefined)
	{
		if (this.Config.debug > 0)
			API3.warn("Petra naval manager error: unit " + entity.id() +  " has already required a transport");
		return false;
	}

	for (var plan of this.transportPlans)
	{
		if (plan.startIndex !== startIndex || plan.endIndex !== endIndex)
			continue
		if (plan.state !== "boarding")
			continue
		plan.addUnit(entity, endPos);
		return true;
	}
	var plan = new m.TransportPlan(gameState, [entity], startIndex, endIndex, endPos);
	if (plan.failed)
	{
		if (this.Config.debug > 0)
			API3.warn(">>>> transport plan aborted <<<<");
		return false;
	}
	this.transportPlans.push(plan);
	return true;
};

// split a transport plan in two, moving all entities not yet affected to a ship in the new plan
m.NavalManager.prototype.splitTransport = function(gameState, plan)
{
	if (this.Config.debug > 0)
		API3.warn(">>>> split of transport plan started <<<<");
	var newplan = new m.TransportPlan(gameState, [], plan.startIndex, plan.endIndex, plan.endPos);
	if (newplan.failed)
	{
		if (this.Config.debug > 0)
			API3.warn(">>>> split of transport plan aborted <<<<");
		return false;
	}

	var nbUnits = 0;
	plan.units.forEach(function (ent) {
		if (ent.getMetadata(PlayerID, "onBoard"))
			return;
		++nbUnits;
		newplan.addUnit(ent, ent.getMetadata(PlayerID, "endPos"));
	});
	if (this.Config.debug > 0)
		API3.warn(">>>> previous plan left with units " + plan.units.length);
	if (nbUnits)
		this.transportPlans.push(newplan);
	return (nbUnits !== 0);
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
	if (queues.ships.length() !== 0)
		return;

	for (var sea = 0; sea < this.neededTransportShips.length; sea++)
		this.neededTransportShips[sea] = 0;

	for (var plan of this.transportPlans)
	{
		if (!plan.needTransportShips || plan.units.length < 2)
			continue;
		var sea = plan.sea;
		if (gameState.countOwnQueuedEntitiesWithMetadata("sea", sea) > 0
			|| this.seaTransportShips[sea].length < this.wantedTransportShips[sea])
			continue;
		++this.neededTransportShips[sea];
		if (this.wantedTransportShips[sea] === 0 || this.seaTransportShips[sea].length < plan.transportShips.length + 2)
		{
			++this.wantedTransportShips[sea];
			return;
		}
	}

	for (var sea = 0; sea < this.neededTransportShips.length; sea++)
		if (this.neededTransportShips[sea] > 2)
			++this.wantedTransportShips[sea];
};

m.NavalManager.prototype.maintainFleet = function(gameState, queues)
{
	if (queues.ships.length() > 0)
		return;
	if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_dock"), true) +
		gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_super_dock"), true) === 0)
		return;
	// check if we have enough transport ships per region.
	for (var sea = 0; sea < this.seaShips.length; ++sea)
	{
		if (this.accessibleSeas.indexOf(sea) === -1)
			continue;
		if (gameState.countOwnQueuedEntitiesWithMetadata("sea", sea) > 0)
			continue;

		if (this.seaTransportShips[sea].length < this.wantedTransportShips[sea])
		{
			var template = this.getBestShip(gameState, sea, "transport");
			if (template)
			{
				queues.ships.addItem(new m.TrainingPlan(gameState, template, { "sea": sea }, 1, 1));
				continue;
			}
		}


		if (this.seaFishShips[sea].length < this.wantedFishShips[sea])
		{
			var template = this.getBestShip(gameState, sea, "fishing");
			if (template)
			{
				queues.ships.addItem(new m.TrainingPlan(gameState, template, { "base": 0, "role": "worker", "sea": sea }, 1, 1));
				continue;
			}
		}
	}
};

// assigns free ships to plans that need some
m.NavalManager.prototype.assignShipsToPlans = function(gameState)
{
	for (var plan of this.transportPlans)
		if (plan.needTransportShips)
			plan.assignShip(gameState);
};

// let blocking ships move apart from active ships (waiting for a better pathfinder)
m.NavalManager.prototype.moveApart = function(gameState)
{
	var self = this;
	for (var sea = 0; sea < gameState.ai.accessibility.regionSize.length; ++sea)
	{
		this.seaShips[sea].forEach(function(ship) {
			if (ship.getMetadata(PlayerID, "transporter") === undefined)
				return;
			self.seaShips[sea].forEach(function(blockingShip) {
				if (blockingShip === ship || !blockingShip.isIdle())
					return;
				if (API3.SquareVectorDistance(ship.position(), blockingShip.position()) > 900)
					return;
				if (blockingShip.getMetadata(PlayerID, "transporter") === undefined)
					blockingShip.moveApart(ship.position(), 12);
				else
					blockingShip.moveApart(ship.position(), 6);
			});
		});
	}
};

m.NavalManager.prototype.buildNavalStructures = function(gameState, queues)
{
	if (!gameState.ai.HQ.navalMap || !gameState.ai.HQ.baseManagers[1])
		return;

	if (gameState.getPopulation() > this.Config.Economy.popForDock)
	{
		if (queues.economicBuilding.countQueuedUnitsWithClass("NavalMarket") === 0 &&
			gameState.countEntitiesAndQueuedByType(gameState.applyCiv("structures/{civ}_dock"), true) === 0)
		{
			if (gameState.ai.HQ.canBuild(gameState, "structures/{civ}_dock"))
			{
				var remaining = this.getUnconnectedSeas(gameState, gameState.ai.HQ.baseManagers[1].accessIndex);
				for (var sea of remaining)
				{
					if (gameState.ai.HQ.navalRegions.indexOf(sea) !== -1)
					{
						queues.economicBuilding.addItem(new m.ConstructionPlan(gameState, "structures/{civ}_dock", { "sea": sea }));
						break;
					}
				}
			}
		}
	}

	if (gameState.currentPhase() > 1 && gameState.getPopulation() > this.Config.Economy.popForTown + 15
		&& queues.militaryBuilding.length() === 0 && this.bNaval.length !== 0)
	{
		var nNaval = 0;
		for (var naval of this.bNaval)
			nNaval += gameState.countEntitiesAndQueuedByType(naval, true);

		var docks = gameState.getOwnStructures().filter(API3.Filters.byClass("Dock")).toEntityArray();
		if (docks.length && (nNaval === 0 || (nNaval < this.bNaval.length && gameState.getPopulation() > 120)))
		{
			for (var naval of this.bNaval)
			{
				if (gameState.countEntitiesAndQueuedByType(naval, true) < 1 && gameState.ai.HQ.canBuild(gameState, naval))
				{
					var sea = docks[0].getMetadata(PlayerID, "sea");
					queues.militaryBuilding.addItem(new m.ConstructionPlan(gameState, naval, { "sea": sea }));
					break;
				}
			}
		}
	}
};

// goal can be either attack (choose ship with best arrowCount) or transport (choose ship with best capacity)
m.NavalManager.prototype.getBestShip = function(gameState, sea, goal)
{
	var trainableShips = [];
	gameState.getOwnTrainingFacilities().filter(API3.Filters.byMetadata(PlayerID, "sea", sea)).forEach(function(ent) {
		var trainables = ent.trainableEntities();
		for (var trainable of trainables)
		{
			var template = gameState.getTemplate(trainable);
			if (template.hasClass("Ship") && trainableShips.indexOf(trainable) === -1)
				trainableShips.push(trainable);
		}
	});

	var best = 0;
	var bestShip = undefined;
	var limits = gameState.getEntityLimits();
	var current = gameState.getEntityCounts();
	for (var trainable of trainableShips)
	{
		var template = gameState.getTemplate(trainable);
		if (!template.available(gameState))
			continue;

		var aboveLimit = false;
		for (var limitedClass in limits)
		{
			if (!template.hasClass(limitedClass) || current[limitedClass] < limits[limitedClass])
				continue;
			aboveLimit = true;
			break;
		}
		if (aboveLimit)
			continue;

		var arrows = +(template.getDefaultArrow() || 0);
		if (goal === "attack")    // choose the maximum default arrows
		{
			if (best > arrows)
				continue;
			best = arrows;
		}
		else if (goal === "transport")   // choose the maximum capacity, with a bonus if arrows or if siege transport
		{
			var capacity = +(template.garrisonMax() || 0);
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
		{
			if (!template.hasClass("FishingBoat"))
				continue;
		}
		bestShip = trainable;
	}
	return bestShip;
};

m.NavalManager.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("Naval Manager update");
	
	this.checkEvents(gameState, queues, events);

	this.buildNavalStructures(gameState, queues);

	// close previous transport plans if finished
	for (var i = 0; i < this.transportPlans.length; ++i)
	{
		var remaining = this.transportPlans[i].update(gameState);
		if (remaining === 0)
		{
			if (this.Config.debug > 0)
				API3.warn("no more units on transport plan " + this.transportPlans[i].ID);
			this.transportPlans[i].releaseAll();
			this.transportPlans.splice(i--, 1);
		}
	}

	// assign free ships to plans which need them
	this.assignShipsToPlans(gameState);
	// and require for more ships if needed
	this.checkLevels(gameState, queues);
	this.maintainFleet(gameState, queues);
	// let inactive ships move apart from active ones (waiting for a better pathfinder)
	this.moveApart(gameState);

	Engine.ProfileStop();
};

return m;
}(PETRA);
