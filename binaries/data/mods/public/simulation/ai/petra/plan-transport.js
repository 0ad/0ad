var PETRA = function(m)
{

/*
 Describes a transport plan
 Constructor assign units (units is an ID array, or an ID), a destionation (position, ingame), and a wanted escort size.
 If "onlyIfOk" is true, then the plan will only start if the wanted escort size is met.
 The naval manager will try to deal with it accordingly.
 
 By this I mean that the naval manager will find how to go from access point 1 to access point 2 (relying on in-game pathfinder for mvt)
 And then carry units from there.
 If units are over multiple accessibility indexes (ie different islands) it will first group them
 
 Note: only assign it units currently over land, or it won't work.
 Also: destination should probably be land, otherwise the units will be lost at sea.
*/

// TODO: finish the support of multiple accessibility indexes.
// TODO: this doesn't check we can actually reach in the init, which we might want?

m.TransportPlan = function(gameState, units, destination, allAtOnce, escortSize, onlyIfOK) {
	var self = this;

	this.ID = m.playerGlobals[PlayerID].uniqueIDTPlans++;
	
	var unitsID = [];
	if (units.length !== undefined)
		unitsID = units;
	else
		unitsID = [units];
	
	this.units = m.EntityCollectionFromIds(gameState, unitsID);
	this.units.forEach(function (ent) { //}){
		ent.setMetadata(PlayerID, "tpplan", self.ID);
		ent.setMetadata(PlayerID, "formerRole", ent.getMetadata(PlayerID, "role"));
		ent.setMetadata(PlayerID, "role", "transport");
	});
	
	this.units.freeze();
	this.units.registerUpdates();

	m.debug ("Starting a new plan with ID " +  this.ID + " to " + destination);
	m.debug ("units are " + uneval (units));
	
	this.destination = destination;
	this.destinationIndex = gameState.ai.accessibility.getAccessValue(destination);

	if (allAtOnce)
		this.allAtOnce = allAtOnce;
	else
		this.allAtOnce = false;

	if (escortSize)
		this.escortSize = escortSize;
	else
		this.escortSize = 0;
	
	if (onlyIfOK)
		this.onlyIfOK = onlyIfOK;
	else
		this.onlyIfOK = false;

	this.state = "unstarted";
	
	this.ships = gameState.ai.HQ.navalManager.ships.filter(Filters.byMetadata(PlayerID, "tpplan", this.ID));
	// note: those two can overlap (some transport ships are warships too and vice-versa).
	this.transportShips = gameState.ai.HQ.navalManager.tpShips.filter(Filters.byMetadata(PlayerID, "tpplan", this.ID));
	this.escortShips = gameState.ai.HQ.navalManager.warships.filter(Filters.byMetadata(PlayerID, "tpplan", this.ID));
	
	this.ships.registerUpdates();
	this.transportShips.registerUpdates();
	this.escortShips.registerUpdates();
};

// count available slots
m.TransportPlan.prototype.countFreeSlots = function(onlyTrulyFree)
{
	var slots = 0;
	this.transportShips.forEach(function (ent) { //}){
		slots += ent.garrisonMax();
		if (onlyTrulyFree)
			slots -= ent.garrisoned().length;
	});
}

m.TransportPlan.prototype.assignShip = function(gameState, ship)
{
	ship.setMetadata(PlayerID,"tpplan", this.ID);
}

m.TransportPlan.prototype.releaseAll = function(gameState)
{
	this.ships.forEach(function (ent) { ent.setMetadata(PlayerID,"tpplan", undefined) });
	this.units.forEach(function (ent) {
		var fRole = ent.getMetadata(PlayerID, "formerRole");
		if (fRole)
			ent.setMetadata(PlayerID,"role", fRole);
		ent.setMetadata(PlayerID,"tpplan", undefined)
	});
}

m.TransportPlan.prototype.releaseAllShips = function(gameState)
{
	this.ships.forEach(function (ent) { ent.setMetadata(PlayerID,"tpplan", undefined) });
}

m.TransportPlan.prototype.needTpShips = function()
{
	if ((this.allAtOnce && this.countFreeSlots() >= this.units.length) || this.transportShips.length > 0)
		return false;
	return true;
}

m.TransportPlan.prototype.needEscortShips = function()
{
	return !((this.onlyIfOK && this.escortShips.length < this.escortSize) || !this.onlyIfOK);
}

// returns the zone for which we are needing our ships
m.TransportPlan.prototype.neededShipsZone = function()
{
	if (!this.seaZone)
		return false;
	return this.seaZone;
}


// try to move on.
/* several states:
 "unstarted" is the initial state, and will determine wether we follow basic or grouping path
 Basic path:
 - "waitingForBoarding" means we wait 'till we have enough transport ships and escort ships to move stuffs.
 - "Boarding" means we're trying to board units onto our ships
 - "Moving" means we're moving ships
 - "Unboarding" means we're unbording
 - Once we're unboarded, we either return to boarding point (if we still have units to board) or we clear.
	> there is the possibility that we'll be moving units on land, but that's basically a restart too, with more clearing.
 Grouping Path is basically the same with "grouping" and we never unboard (unless there is a need to)
 */
m.TransportPlan.prototype.carryOn = function(gameState, navalManager)
{
	if (this.state === "unstarted")
	{
		// Okay so we can start the plan.
		// So what we'll do is check what accessibility indexes our units are.
		var unitIndexes = [];
		this.units.forEach( function (ent) { //}){
			var idx = gameState.ai.accessibility.getAccessValue(ent.position());
			if (unitIndexes.indexOf(idx) === -1 && idx !== 1)
				unitIndexes.push(idx);
		});
		
		// we have indexes. If there is more than 1, we'll try and regroup them.
		if (unitIndexes.length > 1)
		{
			warn("Transport Plan path is too complicated, aborting");
			return false;
			/*
			this.state = "waitingForGrouping";
			// get the best index for grouping, ie start by the one farthest away in terms of movement.
			var idxLength = {};
			for (var i = 0; i < unitIndexes.length; ++i)
				idxLength[unitIndexes[i]] = gameState.ai.accessibility.getTrajectToIndex(unitIndexes[i], this.destinationIndex).length;
			var sortedArray = unitIndexes.sort(function (a,b) { //}){
				return idxLength[b] - idxLength[a];
			});
			this.startIndex = sortedArray[0];
			// okay so we'll board units from this index and we'll try to join them with units of the next index.
			// this might not be terribly efficient but it won't be efficient anyhow.
			return true;*/
		}
		this.state = "waitingForBoarding";
		
		// let's get our index this turn.
		this.startIndex = unitIndexes[0];

		m.debug ("plan " +  this.ID + " from " + this.startIndex);

		return true;
	}
	if (this.state === "waitingForBoarding")
	{

		if (!this.path)
		{
			this.path = gameState.ai.accessibility.getTrajectToIndex(this.startIndex, this.destinationIndex);
			if (!this.path || this.path.length === 0 || this.path.length % 2 === 0)
				return false;	// TODO: improve error handling
			if (this.path[0] !== this.startIndex)
			{
				warn ("Start point of the path is not the start index, aborting transport plan");
				return false;
			}
			// we have a path, register the first sea zone.
			this.seaZone = this.path[1];
			m.debug ("Plan " + this.ID + " over seazone " + this.seaZone);
		}
		// if we currently have no baoarding spot, try and find one.
		if (!this.boardingSpot)
		{
			// TODO: improve on this whenever we have danger maps.
			// okay so we have units over an accessibility index.
			// we'll get a map going on.
			var Xibility = gameState.ai.accessibility;
			
			// custom obstruction map that uses the shore as the obstruction map
			// but doesn't really check like for a building.
			// created realtime with the other map.
			var passabilityMap = gameState.getMap();
			var territoryMap = gameState.ai.territoryMap;
			var obstructionMask = gameState.getPassabilityClassMask("foundationObstruction") | gameState.getPassabilityClassMask("building-shore");
			var obstructions = new API3.Map(gameState.sharedScript);
			
			// wanted map.
			var friendlyTiles = new API3.Map(gameState.sharedScript);
			
			for (var j = 0; j < friendlyTiles.length; ++j)
			{
				// only on the wanted island
				if (Xibility.landPassMap[j] !== this.startIndex)
					continue;
				
				// setting obstructions
				var tilePlayer = (territoryMap.data[j] & TERRITORY_PLAYER_MASK);
				// invalid is enemy-controlled or not on the right sea/land (we need a shore for this, we might want to check neighbhs instead).
				var invalidTerritory = (gameState.isPlayerEnemy(tilePlayer) && tilePlayer != 0)
				|| (Xibility.navalPassMap[j] !== this.path[1]);
				obstructions.map[j] = (invalidTerritory || (passabilityMap.data[j] & obstructionMask)) ? 0 : 255;
				
				// currently we'll just like better on our territory
				if (tilePlayer == PlayerID)
					friendlyTiles.map[j] = 100;
			}
			
			obstructions.expandInfluences();
			
			var best = friendlyTiles.findBestTile(4, obstructions);
			var bestIdx = best[0];
			
			// not good enough.
			if (best[1] <= 0)
			{
				best = friendlyTiles.findBestTile(1, obstructions);
				bestIdx = best[0];
				if (best[1] <= 0)
					return false; // apparently we won't be able to board.
			}
			
			var x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
			var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;
			
			// we have the spot we want to board at.
			this.boardingSpot = [x,z];
			m.debug ("Plan " + this.ID + " new boarding spot is  " + this.boardingSpot);
		}

		// if all at once we need to be full, else we just need enough escort ships.
		if (!this.needTpShips() && !this.needEscortShips())
		{
			// preparing variables
			// TODO: destroy former entity collection.
			this.garrisoningUnits = this.units.filter(Filters.not(Filters.isGarrisoned()));
			this.garrisoningUnits.registerUpdates();
			this.garrisoningUnits.freeze();
			
			this.garrisonShipID = -1;
			
			m.debug ("Boarding");
			this.state = "boarding";
		}
		return true;
	} else if (this.state === "waitingForGrouping")
	{
		// TODO: this.
		return true;
	}
	if (this.state === "boarding" && gameState.ai.playedTurn % 5 === 0)
	{		
		// TODO: improve error recognition.
		if (this.units.length === 0)
			return false;
		if (!this.boardingSpot)
			return false;
		if (this.needTpShips())
		{
			this.state = "waitingForBoarding";
			return true;
		}
		if (this.needEscortShips())
		{
			this.state = "waitingForBoarding";
			return true;
		}

		// check if we aren't actually finished.
		if (this.units.getCentrePosition() == undefined || this.countFreeSlots(true) === 0)
		{
			delete this.boardingSpot;
			this.garrisoningUnits.unregister();
			this.state = "moving";
			return true;
		}

		// check if we need to move our units and ships closer together
		var stillMoving = false;
		if (API3.SquareVectorDistance(this.ships.getCentrePosition(),this.boardingSpot) > 1600)
		{
			this.ships.move(this.boardingSpot[0],this.boardingSpot[1]);
			stillMoving = true;	// wait till ships are in position
		}
		if (API3.SquareVectorDistance(this.units.getCentrePosition(),this.boardingSpot) > 1600)
		{
			this.units.move(this.boardingSpot[0],this.boardingSpot[1]);
			stillMoving = true;	// wait till units are in position
		}
		if (stillMoving)
		{
			return true; // wait.
		}
		// check if we need to try and board units.
		var garrisonShip = gameState.getEntityById(this.garrisonShipID);
		var self = this;
		// check if ship we're currently garrisoning in is full
		if (garrisonShip && garrisonShip.canGarrisonInside())
		{
			// okay garrison units
			var nbStill = garrisonShip.garrisonMax() - garrisonShip.garrisoned().length;
			if (this.garrisoningUnits.length < nbStill)
			{
				Engine.PostCommand({"type": "garrison", "entities": this.garrisoningUnits.toIdArray(), "target": garrisonShip.id(),"queued": false});
			}
			return true;
		} else if (garrisonShip)
		{
			// full ship, abort
			this.garrisonShipID = -1;
			garrisonShip = false;	// will enter next if.
		}
		if (!garrisonShip)
		{
			// could have died or could have be full
			// we'll pick a new one, one that isn't full
			for (var i in this.transportShips._entities)
			{
				if (this.transportShips._entities[i].canGarrisonInside())
				{
					this.garrisonShipID = this.transportShips._entities[i].id();
					break;
				}
			}
			return true; // wait.
		}
		// could I actually get here?
		return true;
	}
	if (this.state === "moving")
	{
		if (!this.unboardingSpot)
		{
			// TODO: improve on this whenever we have danger maps.
			// okay so we have units over an accessibility index.
			// we'll get a map going on.
			var Xibility = gameState.ai.accessibility;
			
			// custom obstruction map that uses the shore as the obstruction map
			// but doesn't really check like for a building.
			// created realtime with the other map.
			var passabilityMap = gameState.getMap();
			var territoryMap = gameState.ai.territoryMap;
			var obstructionMask = gameState.getPassabilityClassMask("foundationObstruction") | gameState.getPassabilityClassMask("building-shore");
			var obstructions = new API3.Map(gameState.sharedScript);
			
			// wanted map.
			var friendlyTiles = new API3.Map(gameState.sharedScript);
			
			var wantedIndex = -1;
			
			if (this.path.length >= 3)
			{
				this.path.splice(0,2);
				wantedIndex = this.path[0];
			} else {
				m.debug ("too short at " +uneval(this.path));
				return false; // Incomputable
			}
			
			for (var j = 0; j < friendlyTiles.length; ++j)
			{
				// only on the wanted island
				if (Xibility.landPassMap[j] !== wantedIndex)
					continue;
				
				// setting obstructions
				var tilePlayer = (territoryMap.data[j] & TERRITORY_PLAYER_MASK);
				// invalid is not on the right land (we need a shore for this, we might want to check neighbhs instead).
				var invalidTerritory = (Xibility.landPassMap[j] !== wantedIndex);
				obstructions.map[j] = (invalidTerritory || (passabilityMap.data[j] & obstructionMask)) ? 0 : 255;
				
				// currently we'll just like better on our territory
				if (tilePlayer == PlayerID)
					friendlyTiles.map[j] = 100;
				else if (gameState.isPlayerEnemy(tilePlayer) && tilePlayer != 0)
					friendlyTiles.map[j] = 4;
				else
					friendlyTiles.map[j] = 50;
			}

			obstructions.expandInfluences();
			
			var best = friendlyTiles.findBestTile(4, obstructions);
			var bestIdx = best[0];
			
			// not good enough.
			if (best[1] <= 0)
			{
				best = friendlyTiles.findBestTile(1, obstructions);
				bestIdx = best[0];
				if (best[1] <= 0)
					return false; // apparently we won't be able to unboard.
			}
			
			var x = ((bestIdx % friendlyTiles.width) + 0.5) * gameState.cellSize;
			var z = (Math.floor(bestIdx / friendlyTiles.width) + 0.5) * gameState.cellSize;
			
			// we have the spot we want to board at.
			this.unboardingSpot = [x,z];
			return true;
		}
		
		// TODO: improve error recognition.
		if (this.units.length === 0)
			return false;
		if (!this.unboardingSpot)
			return false;
		
		// check if we need to move ships
		if (API3.SquareVectorDistance(this.ships.getCentrePosition(),this.unboardingSpot) > 400)
		{
			this.ships.move(this.unboardingSpot[0],this.unboardingSpot[1]);
		} else {
			this.state = "unboarding";
			return true;
		}
		return true;
	}
	if (this.state === "unboarding")
	{
		// TODO: improve error recognition.
		if (this.units.length === 0)
			return false;
		
		// check if we need to move ships
		if (API3.SquareVectorDistance(this.ships.getCentrePosition(),this.unboardingSpot) > 400)
		{
			this.ships.move(this.unboardingSpot[0],this.unboardingSpot[1]);
		} else {
			this.transportShips.forEach( function (ent) { ent.unloadAll() });
			// TODO: improve on this.
			if (this.path.length > 1)
			{
				m.debug ("plan " +  this.ID + " going back for more");
				// basically reset.
				delete this.boardingSpot;
				delete this.unboardingSpot;
				this.state = "unstarted";
				this.releaseAllShips();
				return true;
			}
			m.debug ("plan " +  this.ID + " is finished");
			return false;
		}
	}

	return true;
}

return m;
}(PETRA);
