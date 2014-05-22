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
	
	this.ships = gameState.getOwnEntities().filter(API3.Filters.byClass("Ship"));
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
	var totalSize = width * width;
	for (var i = 0; i < totalSize; ++i)
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

	// Assign our docks
	var self = this;
	this.docks.forEach(function(dock) { self.assignDock(gameState, dock); });
};

m.NavalManager.prototype.resetFishingBoats = function()
{
	for (var i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
		this.wantedFishShips[i] = 0;
};

m.NavalManager.prototype.assignDock = function(gameState, dock)
{
	var land = dock.getMetadata(PlayerID, "access");
	var sea = dock.getMetadata(PlayerID, "sea");
	if (sea === undefined)
	{
		sea = this.getDockSeaIndex(gameState, dock);
		dock.setMetadata(PlayerID, "sea", sea);
	}

	if (this.landZoneDocked[land].indexOf(sea) === -1)
		this.landZoneDocked[land].push(sea);
	if (this.accessibleSeas.indexOf(sea) === -1)
		this.accessibleSeas.push(sea);
};

// get the sea index for our starting docks and those of our allies
m.NavalManager.prototype.getDockSeaIndex = function(gameState, dock)
{
	var sea = gameState.ai.accessibility.getAccessValue(dock.position(), true);
	if (sea < 2)
	{
		// pre-positioned docks are sometimes not on the shoreline
		var dockPos = dock.position();
		var radius = dock.footprintRadius();
		for (var i = 0; i < 16; i++)
		{
			var seaPos = [ dockPos[0] + radius*Math.cos(i*Math.PI/8), dockPos[1] + radius*Math.sin(i*Math.PI/8)];

			sea = gameState.ai.accessibility.getAccessValue(seaPos, true);
			if (sea >= 2)
				break;
		}
	}
	if (sea < 2)
		warn("ERROR in Petra navalManager because of dock position " + sea);
	return sea;
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
	for (var i in evts)
	{
		var evt = evts[i];
		if (!evt || !evt.newentity)
			continue;
		var entity = gameState.getEntityById(evt.newentity);
		if (entity && entity.hasClass("Dock") && entity.isOwn(PlayerID))
			this.assignDock(gameState, entity);
	}

	var evts = events["TrainingFinished"];
	for (var i in evts)
	{
		var evt = evts[i];
		if (!evt || !evt.entities)
			continue;
		for each (var entId in evt.entities)
		{
			var entity = gameState.getEntityById(entId);
			if (entity && entity.hasClass("Ship") && entity.isOwn(PlayerID))
			{
				var pos = gameState.ai.accessibility.gamePosToMapPos(entity.position());
				var index = pos[0] + pos[1]*gameState.ai.accessibility.width;
				var sea = gameState.ai.accessibility.navalPassMap[index];
				entity.setMetadata(PlayerID, "sea", sea);
			}
		}
	}
};

m.NavalManager.prototype.addPlan = function(plan)
{
	this.transportPlans.push(plan);
};

// complete already existing plan or create a new one for this requirement
// (many units can then call this separately and end up in the same plan)
m.NavalManager.prototype.requireTransport = function(gameState, entity, startIndex, endIndex, endPos)
{
	if (entity.getMetadata(PlayerID, "transport") !== undefined)
	{
		if (this.Config.debug > 0)
			warn("Petra naval manager error: unit " + entity.id() +  " has already required a transport");
		return false;
	}

	for each (var plan in this.transportPlans)
	{
		if (plan.startIndex !== startIndex || plan.endIndex !== endIndex)
			continue
		if (plan.state !== "boarding")
			continue
		if (plan.units.length > 12)   // TODO to be improve  ... check on ship capacity
			continue;
		plan.addUnit(entity, endPos);
		return true;
	}
	var plan = new m.TransportPlan(gameState, [entity], startIndex, endIndex, endPos, false);
	if (plan.failed)
	{
		if (this.Config.debug > 0)
			warn(">>>> transport plan aborted <<<<");
		return false;
	}
	this.transportPlans.push(plan);
	return true;
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

	for each (var plan in this.transportPlans)
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
	for (var i = 0; i < this.seaShips.length; ++i)
	{
		if (this.accessibleSeas.indexOf(i) === -1)
			continue;
		if (gameState.countOwnQueuedEntitiesWithMetadata("sea", i) > 0)
			continue;

		if (this.seaTransportShips[i].length < this.wantedTransportShips[i])
		{
			if ((gameState.civ() === "cart" && gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_super_dock"), true) === 0)
				|| gameState.civ() === "iber")
				queues.ships.addItem(new m.TrainingPlan(gameState, "units/{civ}_ship_merchant", { "sea": i }, 1, 1));
			else
				queues.ships.addItem(new m.TrainingPlan(gameState, "units/{civ}_ship_trireme", { "sea": i }, 1, 1));
		}
		else if (this.seaFishShips[i].length < this.wantedFishShips[i])
			queues.ships.addItem(new m.TrainingPlan(gameState, "units/{civ}_ship_fishing", { "base": 0, "role": "worker", "sea": i }, 1, 1));
	}
};

// assigns free ships to plans that need some
m.NavalManager.prototype.assignToPlans = function(gameState)
{
	for each (var plan in this.transportPlans)
	{
		if (!plan.needTransportShips)
			continue;

		for each (var ship in this.seaTransportShips[plan.sea]._entities)
		{
			if (ship.getMetadata(PlayerID, "transporter") || ship.getMetadata(PlayerID, "escort"))
				continue;
			plan.assignShip(ship);
			plan.needTransportShips = false;
			break;
		}
	}
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

// Some functions are run every turn
// Others once in a while
m.NavalManager.prototype.update = function(gameState, queues, events)
{
	Engine.ProfileStart("Naval Manager update");
	
	this.checkEvents(gameState, queues, events);

	for (var i = 0; i < this.transportPlans.length; ++i)
	{
		var remaining = this.transportPlans[i].update(gameState);
		if (remaining === 0)
		{
			if (this.Config.debug > 0)
				warn("no more units on transport plan " + this.transportPlans[i].ID);
//			var moveBack = true;
//			var plan = this.transportPlans[i];
//			this.docks.forEach(function(dock) {
//				if (!moveBack || dock.getMetadata(PlayerID, "sea") !== plan.sea)
//					return;
//				moveBack = false;
//				plan.ships.forEach(function(ship) { ship.move(dock.position()[0], dock.position()[1]); });
//			});
			this.transportPlans[i].releaseAll();
			this.transportPlans.splice(i--, 1);
		}
	}

	// assign free ships to plans which need them
	this.assignToPlans(gameState);
	// and require for more ships if needed
	this.checkLevels(gameState, queues);
	this.maintainFleet(gameState, queues);
	// let inactive ships move apart from active ones (waiting for a better pathfinder)
	this.moveApart(gameState);

	Engine.ProfileStop();
};

return m;
}(PETRA);
