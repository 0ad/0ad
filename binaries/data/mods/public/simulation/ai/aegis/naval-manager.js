var AEGIS = function(m)
{

/* Naval Manager
 Will deal with anything ships.
 -Basically trade over water (with fleets and goals commissioned by the economy manager)
 -Defence over water (commissioned by the defense manager)
	-subtask being patrols, escort, naval superiority.
 -Transport of units over water (a few units).
 -Scouting, ultimately.
 Also deals with handling docks, making sure we have access and stuffs like that.
 Does not build them though, that's for the base manager to handle.
 */

m.NavalManager = function() {
	// accessibility zones for which we have a dock.
	// Connexion is described as [landindex] = [seaIndexes];
	// technically they also exist for sea zones but I don't care.
	this.landZoneDocked = [];
	
	// list of seas I have a dock on.
	this.accessibleSeas = [];
	
	// ship subCollections. Also exist for land zones, idem, not caring.
	this.seaShips = [];
	this.seaTpShips = [];
	this.seaWarships = [];

	// wanted NB per zone.
	this.wantedTpShips = [];
	this.wantedWarships = [];
	
	this.transportPlans = [];
	this.askedPlans = [];
};

// More initialisation for stuff that needs the gameState
m.NavalManager.prototype.init = function(gameState, queues) {
	// finished docks
	this.docks = gameState.getOwnStructures().filter(API3.Filters.and(API3.Filters.byClass("Dock"), API3.Filters.not(API3.Filters.isFoundation())));
	this.docks.allowQuickIter();
	this.docks.registerUpdates();
	
	this.ships = gameState.getOwnEntities().filter(API3.Filters.byClass("Ship"));
	// note: those two can overlap (some transport ships are warships too and vice-versa).
	this.tpShips = this.ships.filter(API3.Filters.byCanGarrison());
	this.warships = this.ships.filter(API3.Filters.byClass("Warship"));

	this.ships.registerUpdates();
	this.tpShips.registerUpdates();
	this.warships.registerUpdates();
	
	for (var i = 0; i < gameState.ai.accessibility.regionSize.length; ++i)
	{
		if (gameState.ai.accessibility.regionType[i] !== "water")
		{
			// push dummies
			this.seaShips.push(new API3.EntityCollection(gameState.sharedScript));
			this.seaTpShips.push(new API3.EntityCollection(gameState.sharedScript));
			this.seaWarships.push(new API3.EntityCollection(gameState.sharedScript));
			this.wantedTpShips.push(0);
			this.wantedWarships.push(0);
		} else {
			var collec = this.ships.filter(API3.Filters.byStaticMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaShips.push(collec);
			collec = this.tpShips.filter(API3.Filters.byStaticMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaTpShips.push(collec);
			var collec = this.warships.filter(API3.Filters.byStaticMetadata(PlayerID, "sea", i));
			collec.registerUpdates();
			this.seaWarships.push(collec);
			
			this.wantedTpShips.push(1);
			this.wantedWarships.push(1);
		}
		
		this.landZoneDocked.push([]);
	}
};

m.NavalManager.prototype.getUnconnectedSeas = function (gameState, region) {
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

// returns true if there is a path from A to B and we have docks.
m.NavalManager.prototype.canReach = function (gameState, regionA, regionB) {
	var path = gameState.ai.accessibility.getTrajectToIndex(regionA, regionB);
	if (!path)
	{
		return false;
	}
	for (var i = 0; i < path.length - 1; ++i)
	{
		if (gameState.ai.accessibility.regionType[path[i]] == "land")
			if (this.accessibleSeas.indexOf(path[i+1]) === -1)
			{
				m.debug ("cannot reach because of " + path[i+1]);
				return false;	// we wn't be able to board on that sea
			}
	}
	return true;
};


m.NavalManager.prototype.checkEvents = function (gameState, queues, events) {
	var evts = events["ConstructionFinished"];
	// TODO: probably check stuffs like a base destruction.
	for (var i in evts)
	{
		var evt = evts[i];
		if (evt && evt.newentity)
		{
			var entity = gameState.getEntityById(evt.newentity);
			if (entity && entity.hasClass("Dock") && entity.isOwn(PlayerID))
			{
				// okay we have a dock whose construction is finished.
				// let's assign it to us.
				var pos = entity.position();
				var li = gameState.ai.accessibility.getAccessValue(pos);
				var ni = entity.getMetadata(PlayerID, "sea");
				if (this.landZoneDocked[li].indexOf(ni) === -1)
					this.landZoneDocked[li].push(ni);
				if (this.accessibleSeas.indexOf(ni) === -1)
					this.accessibleSeas.push(ni);
			}
		}
	}
};

m.NavalManager.prototype.addPlan = function(plan) {
	this.transportPlans.push(plan);
};

// will create a plan at the end of the turn.
// many units can call this separately and end up in the same plan
// which can be useful.
m.NavalManager.prototype.askForTransport = function(entity, startPos, endPos) {
	this.askedPlans.push([entity, startPos, endPos]);
};

// creates aforementionned plans
m.NavalManager.prototype.createPlans = function(gameState) {
	var startID = {};

	for (var i in this.askedPlans)
	{
		var plan = this.askedPlans[i];
		var startIndex = gameState.ai.accessibility.getAccessValue(plan[1]);
		var endIndex = gameState.ai.accessibility.getAccessValue(plan[2]);
		if (startIndex === 1 || endIndex === -1)
			continue;
		if (!startID[startIndex])
		{
			startID[startIndex] = {};
			startID[startIndex][endIndex] = { "dest" : plan[2], "units": [plan[0]]};
		}
		else if (!startID[startIndex][endIndex])
			startID[startIndex][endIndex] = { "dest" : plan[2], "units": [plan[0]]};
		else
			startID[startIndex][endIndex].units.push(plan[0]);
	}
	for (var i in startID)
		for (var k in startID[i])
		{
			var tpPlan = new m.TransportPlan(gameState, startID[i][k].units, startID[i][k].dest, false)
			this.transportPlans.push (tpPlan);
		}
};

// TODO: work on this.
m.NavalManager.prototype.maintainFleet = function(gameState, queues, events) {
	// check if we have enough transport ships.
	// check per region.
	for (var i = 0; i < this.seaShips.length; ++i)
	{
		var tpNb = gameState.countOwnQueuedEntitiesWithMetadata("sea", i);
		if (this.accessibleSeas.indexOf(i) !== -1 && this.seaTpShips[i].length < this.wantedTpShips[i]
			&& tpNb + queues.ships.length() === 0 && gameState.getTemplate(gameState.applyCiv("units/{civ}_ship_bireme")).available(gameState))
		{
			// TODO: check our dock can build the wanted ship types, for Carthage.
			queues.ships.addItem(new m.TrainingPlan(gameState, "units/{civ}_ship_bireme", { "sea" : i }, 1, 1 ));
		}
	}
};

// bumps up the number of ships we want if we need more.
m.NavalManager.prototype.checkLevels = function(gameState, queues) {
	if (queues.ships.length() !== 0)
		return;
	for (var i = 0; i < this.transportPlans.length; ++i)
	{
		var plan = this.transportPlans[i];
		if (plan.needTpShips())
		{
			var zone = plan.neededShipsZone();
			if (zone && gameState.countOwnQueuedEntitiesWithMetadata("sea", zone) > 0)
				continue;
			if (zone && this.wantedTpShips[i] === 0)
				this.wantedTpShips[i]++;
			else if (zone && plan.allAtOnce)
				this.wantedTpShips[i]++;
		}
	}
};

// assigns free ships to plans that need some
m.NavalManager.prototype.assignToPlans = function(gameState, queues, events) {
	for (var i = 0; i < this.transportPlans.length; ++i)
	{
		var plan = this.transportPlans[i];
		if (plan.needTpShips())
		{
			// assign one per go.
			var zone = plan.neededShipsZone();
			if (zone)
			{
				for each (var ship in this.seaTpShips[zone]._entities)
				{
					if (!ship.getMetadata(PlayerID, "tpplan"))
					{
						m.debug ("Assigning ship " + ship.id() + " to plan" + plan.ID);
						plan.assignShip(gameState, ship);
						return true;
					}
				}
			}
		}
	}
	return false;
};

m.NavalManager.prototype.checkActivePlan = function(ID) {
	for (var i = 0; i < this.transportPlans.length; ++i)
		if (this.transportPlans[i].ID === ID)
			return true;

	return false;
};

// Some functions are run every turn
// Others once in a while
m.NavalManager.prototype.update = function(gameState, queues, events) {
	Engine.ProfileStart("Naval Manager update");
	
	this.checkEvents(gameState, queues, events);

	if (gameState.ai.playedTurn % 10 === 0)
	{
		this.maintainFleet(gameState, queues, events);
		this.checkLevels(gameState, queues);		
	}

	for (var i = 0; i < this.transportPlans.length; ++i)
		if (!this.transportPlans[i].carryOn(gameState, this))
		{
			// whatever the reason, this plan needs to be ended
			// it could be that it's finished though.
			var seaZone = this.transportPlans[i].neededShipsZone();
			
			var rallyPos = [];
			this.docks.forEach(function (dock) {
				if (dock.getMetadata(PlayerID,"sea") == seaZone)
					rallyPos = dock.position();
			});
			this.transportPlans[i].ships.move(rallyPos[0], rallyPos[1]);
			this.transportPlans[i].releaseAll(gameState);
			this.transportPlans.splice(i,1);
			--i;
		}
	
	this.assignToPlans(gameState, queues, events);
	if (gameState.ai.playedTurn % 10 === 2)
	{
		this.createPlans(gameState);
		this.askedPlans = [];
	}
	Engine.ProfileStop();
};

return m;
}(AEGIS);
