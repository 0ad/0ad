var PETRA = function(m)
{

/**
 * Describes a transport plan
 * Constructor assign units (units is an ID array), a destination (position).
 * The naval manager will try to deal with it accordingly.
 *
 * By this I mean that the naval manager will find how to go from access point 1 to access point 2
 * and then carry units from there.
 *
 * Note: only assign it units currently over land, or it won't work.
 * Also: destination should probably be land, otherwise the units will be lost at sea.
 *
 * metadata for units:
 *   transport = this.ID
 *   onBoard = ship.id() when affected to a ship but not yet garrisoned
 *           = "onBoard" when garrisoned in a ship
 *           = undefined otherwise
 *   endPos  = position of destination
 *
 *   metadata for ships
 *   transporter = this.ID
 */

m.TransportPlan = function(gameState, units, startIndex, endIndex, endPos, ship)
{
	this.ID = gameState.ai.uniqueIDs.transports++;
	this.debug = gameState.ai.Config.debug;
	this.flotilla = false;   // when false, only one ship per transport ... not yet tested when true

	this.endPos = endPos;
	this.endIndex = endIndex;
	this.startIndex = startIndex;
	// TODO only cases with land-sea-land are allowed for the moment
	// we could also have land-sea-land-sea-land
	if (startIndex === 1)
	{
		// special transport from already garrisoned ship
		if (!ship)
		{
			this.failed = true;
			return false;
		}
		this.sea = ship.getMetadata(PlayerID, "sea");
		ship.setMetadata(PlayerID, "transporter", this.ID);
		for (let ent of units)
			ent.setMetadata(PlayerID, "onBoard", "onBoard");
	}
	else
	{
		this.sea = gameState.ai.HQ.getSeaBetweenIndices(gameState, startIndex, endIndex);
		if (!this.sea)
		{
			this.failed = true;
			if (this.debug > 1)
				API3.warn("transport plan with bad path: startIndex " + startIndex + " endIndex " + endIndex);
			return false;
		}
	}

	for (let ent of units)
	{
		ent.setMetadata(PlayerID, "transport", this.ID);
		ent.setMetadata(PlayerID, "endPos", endPos);
	}

	if (this.debug > 1)
		API3.warn("Starting a new transport plan with ID " + this.ID +
			" to index " + endIndex + " with units length " + units.length);

	this.state = "boarding";
	this.boardingPos = {};
	this.needTransportShips = ship === undefined;
	this.nTry = {};
	return true;
};

m.TransportPlan.prototype.init = function(gameState)
{
	this.units = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "transport", this.ID));
	this.ships = gameState.ai.HQ.navalManager.ships.filter(API3.Filters.byMetadata(PlayerID, "transporter", this.ID));
	this.transportShips = gameState.ai.HQ.navalManager.transportShips.filter(API3.Filters.byMetadata(PlayerID, "transporter", this.ID));

	this.units.registerUpdates();
	this.ships.registerUpdates();
	this.transportShips.registerUpdates();

	this.boardingRange = 18*18;	// TODO compute it from the ship clearance and garrison range
};

/** count available slots */
m.TransportPlan.prototype.countFreeSlots = function()
{
	let self = this;
	let slots = 0;
	this.transportShips.forEach(function (ship) { slots += self.countFreeSlotsOnShip(ship); });
	return slots;
};

m.TransportPlan.prototype.countFreeSlotsOnShip = function(ship)
{
	if (ship.hitpoints() < ship.garrisonEjectHealth() * ship.maxHitpoints())
		return 0;
	let occupied = ship.garrisoned().length +
		this.units.filter(API3.Filters.byMetadata(PlayerID, "onBoard", ship.id())).length;
	return Math.max(ship.garrisonMax() - occupied, 0);
};

m.TransportPlan.prototype.assignUnitToShip = function(gameState, ent)
{
	if (this.needTransportShips)
		return;

	for (let ship of this.transportShips.values())
	{
		if (this.countFreeSlotsOnShip(ship) === 0)
			continue;
		ent.setMetadata(PlayerID, "onBoard", ship.id());
		if (this.debug > 1)
		{
			if (ent.getMetadata(PlayerID, "role") === "attack")
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [2,0,0]});
			else
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [0,2,0]});
		}
		return;
	}

	if (this.flotilla)
		this.needTransportShips = true;
	else
		gameState.ai.HQ.navalManager.splitTransport(gameState, this);
};

m.TransportPlan.prototype.assignShip = function(gameState)
{
	let pos;
	// choose a unit of this plan not yet assigned to a ship
	for (let ent of this.units.values())
	{
		if (!ent.position() || ent.getMetadata(PlayerID, "onBoard") !== undefined)
			continue;
		pos = ent.position();
		break;
	}
	// and choose the nearest available ship from this unit
	let distmin = Math.min();
	let nearest;
	gameState.ai.HQ.navalManager.seaTransportShips[this.sea].forEach(function (ship) {
		if (ship.getMetadata(PlayerID, "transporter"))
			return;
		if (pos)
		{
			let dist = API3.SquareVectorDistance(pos, ship.position());
			if (dist > distmin)
				return;
			distmin = dist;
			nearest = ship;
		}
		else if (!nearest)
			nearest = ship;
	});
	if (!nearest)
		return false;

	nearest.setMetadata(PlayerID, "transporter", this.ID);
	this.ships.updateEnt(nearest);
	this.transportShips.updateEnt(nearest);
	this.needTransportShips = false;
	return true;
};

/** add a unit to this plan */
m.TransportPlan.prototype.addUnit = function(unit, endPos)
{
	unit.setMetadata(PlayerID, "transport", this.ID);
	unit.setMetadata(PlayerID, "endPos", endPos);
	this.units.updateEnt(unit);
};

m.TransportPlan.prototype.releaseAll = function()
{
	this.ships.forEach(function (ship) {
		ship.setMetadata(PlayerID, "transporter", undefined);
		if (ship.getMetadata(PlayerID, "role") === "switchToTrader")
			ship.setMetadata(PlayerID, "role", "trader");
	});
	this.units.forEach(function (ent) {
		ent.setMetadata(PlayerID, "endPos", undefined);
		ent.setMetadata(PlayerID, "onBoard", undefined);
		ent.setMetadata(PlayerID, "transport", undefined);
// TODO if the index of the endPos of the entity is !== , require again another transport (we could need land-sea-land-sea-land)
	});
	this.transportShips.unregister();
	this.ships.unregister();
	this.units.unregister();
};

m.TransportPlan.prototype.cancelTransport = function(gameState)
{
	let ent = this.units.toEntityArray()[0];
	let base = gameState.ai.HQ.getBaseByID(ent.getMetadata(PlayerID, "base"));
	if (!base.anchor || !base.anchor.position())
	{
		for (let newbase of gameState.ai.HQ.baseManagers)
		{
			if (!newbase.anchor || !newbase.anchor.position())
				continue;
			ent.setMetadata(PlayerID, "base", newbase.ID);
			base = newbase;
			break;
		}
		if (!base.anchor || !base.anchor.position())
			return false;
		this.units.forEach(function (ent) {
			ent.setMetadata(PlayerID, "base", base.ID);
		});
	}
	this.endIndex = this.startIndex;
	this.endPos = base.anchor.position();
	this.canceled = true;
	return true;
};


/**
 * try to move on. There are two states:
 * - "boarding" means we're trying to board units onto our ships
 * - "sailing" means we're moving ships and eventually unload units
 * - then the plan is cleared
 */

m.TransportPlan.prototype.update = function(gameState)
{
	if (this.state === "boarding")
		this.onBoarding(gameState);
	else if (this.state === "sailing")
		this.onSailing(gameState);

	return this.units.length;
};

m.TransportPlan.prototype.onBoarding = function(gameState)
{
	let ready = true;
	let self = this;
	let time = gameState.ai.elapsedTime;
	this.units.forEach(function (ent) {
		if (!ent.getMetadata(PlayerID, "onBoard"))
		{
			ready = false;
			self.assignUnitToShip(gameState, ent);
			if (ent.getMetadata(PlayerID, "onBoard"))
			{
				let shipId = ent.getMetadata(PlayerID, "onBoard");
				let ship = gameState.getEntityById(shipId);
				if (!self.boardingPos[shipId])
				{
					self.boardingPos[shipId] = self.getBoardingPos(gameState, ship, self.startIndex, self.sea, ent.position(), false);
					ship.move(self.boardingPos[shipId][0], self.boardingPos[shipId][1]);
					ship.setMetadata(PlayerID, "timeGarrison", time);
				}
				ent.garrison(ship);
				ent.setMetadata(PlayerID, "timeGarrison", time);
				ent.setMetadata(PlayerID, "posGarrison", ent.position());
			}
		}
		else if (ent.getMetadata(PlayerID, "onBoard") !== "onBoard" && !self.isOnBoard(ent))
		{
			ready = false;
			let shipId = ent.getMetadata(PlayerID, "onBoard");
			let ship = gameState.getEntityById(shipId);
			if (!ship)    // the ship must have been destroyed
				ent.setMetadata(PlayerID, "onBoard", undefined);
			else
			{
				let distShip = API3.SquareVectorDistance(self.boardingPos[shipId], ship.position());
				if (time - ship.getMetadata(PlayerID, "timeGarrison") > 8 && distShip > self.boardingRange)
				{
					if (!self.nTry[shipId])
						self.nTry[shipId] = 1;
					else
						++self.nTry[shipId];
					if (self.nTry[shipId] > 1)	// we must have been blocked by something ... try with another boarding point
					{
						self.nTry[shipId] = 0;
						if (self.debug > 1)
							API3.warn("ship " + shipId + " new attempt for a landing point ");
						self.boardingPos[shipId] = self.getBoardingPos(gameState, ship, self.startIndex, self.sea, undefined, false);
					}
					ship.move(self.boardingPos[shipId][0], self.boardingPos[shipId][1]);
					ship.setMetadata(PlayerID, "timeGarrison", time);
				}
				else if (time - ent.getMetadata(PlayerID, "timeGarrison") > 2)
				{
					let oldPos = ent.getMetadata(PlayerID, "posGarrison");
					let newPos = ent.position();
					if (oldPos[0] === newPos[0] && oldPos[1] === newPos[1])
					{
						if (distShip < self.boardingRange)	// looks like we are blocked ... try to go out of this trap
						{
							if (!self.nTry[ent.id()])
								self.nTry[ent.id()] = 1;
							else
								++self.nTry[ent.id()];
							if (self.nTry[ent.id()] > 5)
							{
								if (self.debug > 1)
									API3.warn("unit blocked, but no ways out of the trap ... destroy it");
								self.resetUnit(gameState, ent);
								ent.destroy();
								return;
							}
							if (self.nTry[ent.id()] > 1)
								ent.moveToRange(newPos[0], newPos[1], 30, 30);
							ent.garrison(ship, true);
						}
						else			// wait for the ship
							ent.moveToRange(self.boardingPos[shipId][0], self.boardingPos[shipId][1], 0, 15);
					}
					else
						self.nTry[ent.id()] = 0;
					ent.setMetadata(PlayerID, "timeGarrison", time);
					ent.setMetadata(PlayerID, "posGarrison", ent.position());
				}
			}
		}
	});

	if (!ready)
		return;

	this.ships.forEach(function (ship) {
		self.boardingPos[ship.id()] = undefined;
		self.boardingPos[ship.id()] = self.getBoardingPos(gameState, ship, self.endIndex, self.sea, self.endPos, true);
		ship.move(self.boardingPos[ship.id()][0], self.boardingPos[ship.id()][1]);
	});
	this.state = "sailing";
	this.nTry = {};
	this.unloaded = [];
	this.recovered = [];
};

/** tell if a unit is garrisoned in one of the ships of this plan, and update its metadata if yes */
m.TransportPlan.prototype.isOnBoard = function(ent)
{
	for (let ship of this.transportShips.values())
	{
		if (ship.garrisoned().indexOf(ent.id()) === -1)
			continue;
		ent.setMetadata(PlayerID, "onBoard", "onBoard");
		return true;
	}
	return false;
};

/** when avoidEnnemy is true, we try to not board/unboard in ennemy territory */
m.TransportPlan.prototype.getBoardingPos = function(gameState, ship, landIndex, seaIndex, destination, avoidEnnemy)
{
	if (!gameState.ai.HQ.navalManager.landingZones[landIndex])
	{
		API3.warn(" >>> no landing zone for land " + landIndex);
		return destination;
	}
	else if (!gameState.ai.HQ.navalManager.landingZones[landIndex][seaIndex])
	{
		API3.warn(" >>> no landing zone for land " + landIndex + " and sea " + seaIndex);
		return destination;
	}

	let startPos = ship.position();
	let distmin = Math.min();
	let posmin = destination;
	let width = gameState.getPassabilityMap().width;
	let cell = gameState.getPassabilityMap().cellSize;
	let alliedDocks = gameState.getAllyStructures().filter(API3.Filters.and(
		API3.Filters.byClass("Dock"), API3.Filters.byMetadata(PlayerID, "sea", seaIndex))).toEntityArray();
	for (let i of gameState.ai.HQ.navalManager.landingZones[landIndex][seaIndex])
	{
		let pos = [i%width+0.5, Math.floor(i/width)+0.5];
		pos = [cell*pos[0], cell*pos[1]];
		let dist = API3.SquareVectorDistance(startPos, pos);
		if (destination)
			dist += API3.SquareVectorDistance(pos, destination);
		if (avoidEnnemy)
		{
			let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(pos);
			if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))
				dist += 100000000;
		}
		// require a small distance between all ships of the transport plan to avoid path finder problems
		// this is also used when the ship is blocked and we want to find a new boarding point
		for (let shipId in this.boardingPos)
			if (this.boardingPos[shipId] !== undefined &&
			    API3.SquareVectorDistance(this.boardingPos[shipId], pos) < this.boardingRange)
				dist += 1000000;
		// and not too near our allied docks to not disturb naval traffic
		for (let dock of alliedDocks)
		{
			let dockDist = API3.SquareVectorDistance(dock.position(), pos);
			if (dockDist < 4900)
			    dist += 100000 * (4900 - dockDist) / 4900;
		}
		if (dist > distmin)
			continue;
		distmin = dist;
		posmin = pos;
	}
	// We should always have either destination or the previous boardingPos defined
	// so let's return this value if everything failed
	if (!posmin && this.boardingPos[ship.id()])
		posmin = this.boardingPos[ship.id()];
	return posmin;
};

m.TransportPlan.prototype.onSailing = function(gameState)
{
	// Check that the units recovered on the previous turn have been reloaded
	for (let recov of this.recovered)
	{
		let ent = gameState.getEntityById(recov.entId);
		if (!ent)  // entity destroyed
			continue;
		if (!ent.position())  // reloading succeeded ... move a bit the ship before trying again
		{
			let ship = gameState.getEntityById(recov.shipId);
			if (ship)
				ship.moveApart(recov.entPos, 15);
			continue;
		}
		if (this.debug > 1)
			API3.warn(">>> transport " + this.ID + " reloading failed ... <<<");
		// destroy the unit if inaccessible otherwise leave it there
		let index = gameState.ai.accessibility.getAccessValue(ent.position());
		if (gameState.ai.HQ.landRegions[index])
		{
			if (this.debug > 1)
				API3.warn(" recovered entity kept " + ent.id());
			this.resetUnit(gameState, ent);
			// TODO we should not destroy it, but now the unit could still be reloaded on the next turn
			// and mess everything
			ent.destroy();
		}
		else
		{
			if (this.debug > 1)
				API3.warn("recovered entity destroyed " + ent.id());
			this.resetUnit(gameState, ent);
			ent.destroy();
		}
	}
	this.recovered = [];

	// Check that the units unloaded on the previous turn have been really unloaded and in the right position
	let shipsToMove = {};
	for (let entId of this.unloaded)
	{
		let ent = gameState.getEntityById(entId);
		if (!ent)  // entity destroyed
			continue;
		else if (!ent.position())  // unloading failed
		{
			let ship = gameState.getEntityById(ent.getMetadata(PlayerID, "onBoard"));
			if (ship)
			{
				if (ship.garrisoned().indexOf(entId) !== -1)
					ent.setMetadata(PlayerID, "onBoard", "onBoard");
				else
				{
					API3.warn("Petra transportPlan problem: unit not on ship without position ???");
					this.resetUnit(gameState, ent);
					ent.destroy();
				}
			}
			else
			{
				API3.warn("Petra transportPlan problem: unit on ship, but no ship ???");
				this.resetUnit(gameState, ent);
				ent.destroy();
			}
		}
		else if (gameState.ai.accessibility.getAccessValue(ent.position()) !== this.endIndex)
		{
			// unit unloaded on a wrong region - try to regarrison it and move a bit the ship
			if (this.debug > 1)
				API3.warn(">>> unit unloaded on a wrong region ! try to garrison it again <<<");
			let ship = gameState.getEntityById(ent.getMetadata(PlayerID, "onBoard"));
			if (ship && !this.canceled)
			{
				shipsToMove[ship.id()] = ship;
				this.recovered.push( {"entId": ent.id(), "entPos": ent.position(), "shipId": ship.id()} );
				ent.garrison(ship);
				ent.setMetadata(PlayerID, "onBoard", "onBoard");
			}
			else
			{
				if (this.debug > 1)
					API3.warn("no way ... we destroy it");
				this.resetUnit(gameState, ent);
				ent.destroy();
			}
		}
		else
		{
			ent.setMetadata(PlayerID, "transport", undefined);
			ent.setMetadata(PlayerID, "onBoard", undefined);
			ent.setMetadata(PlayerID, "endPos", undefined);
		}
	}
	for (let shipId in shipsToMove)
	{
		this.boardingPos[shipId] = this.getBoardingPos(gameState, shipsToMove[shipId], this.endIndex, this.sea, this.endPos, true);
		shipsToMove[shipId].move(this.boardingPos[shipId][0], this.boardingPos[shipId][1]);
	}
	this.unloaded = [];

	if (this.canceled)
	{
		for (let ship of this.ships.values())
		{
			this.boardingPos[ship.id()] = undefined;
			this.boardingPos[ship.id()] = this.getBoardingPos(gameState, ship, this.endIndex, this.sea, this.endPos, true);
			ship.move(this.boardingPos[ship.id()][0], this.boardingPos[ship.id()][1]);
		}
		this.canceled = undefined;
	}

	for (let ship of this.transportShips.values())
	{
		if (ship.unitAIState() === "INDIVIDUAL.WALKING")
			continue;
		let shipId = ship.id();
		let dist = API3.SquareVectorDistance(ship.position(), this.boardingPos[shipId]);
		let remaining = 0;
		for (let entId of ship.garrisoned())
		{
			let ent = gameState.getEntityById(entId);
			if (!ent.getMetadata(PlayerID, "transport"))
				continue;
			remaining++;
			if (dist < 625)
			{
				ship.unload(entId);
				this.unloaded.push(entId);
				ent.setMetadata(PlayerID, "onBoard", shipId);
			}
		}

		let recovering = 0;
		for (let recov of this.recovered)
			if (recov.shipId === shipId)
				recovering++;

		if (!remaining && !recovering)   // when empty, release the ship and move apart to leave room for other ships. TODO fight
		{
			ship.moveApart(this.boardingPos[shipId], 15);
			ship.setMetadata(PlayerID, "transporter", undefined);
			if (ship.getMetadata(PlayerID, "role") === "switchToTrader")
				ship.setMetadata(PlayerID, "role", "trader");
			continue;
		}

		if (dist > this.boardingRange)
		{
			if (!this.nTry[shipId])
				this.nTry[shipId] = 1;
			else
				++this.nTry[shipId];
			if (this.nTry[shipId] > 2)	// we must have been blocked by something ... try with another boarding point
			{
				this.nTry[shipId] = 0;
				if (this.debug > 1)
					API3.warn(shipId + " new attempt for a landing point ");
				this.boardingPos[shipId] = this.getBoardingPos(gameState, ship, this.endIndex, this.sea, undefined, true);
			}
			ship.move(this.boardingPos[shipId][0], this.boardingPos[shipId][1]);
		}
	}
};

m.TransportPlan.prototype.resetUnit = function(gameState, ent)
{
	ent.setMetadata(PlayerID, "transport", undefined);
	ent.setMetadata(PlayerID, "onBoard", undefined);
	ent.setMetadata(PlayerID, "endPos", undefined);
	// if from an army or attack, remove it
	if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") >= 0)
	{
		let attackPlan = gameState.ai.HQ.attackManager.getPlan(ent.getMetadata(PlayerID, "plan"));
		if (attackPlan)
			attackPlan.removeUnit(ent, true);
	}
	if (ent.getMetadata(PlayerID, "PartOfArmy"))
	{
		let army = gameState.ai.HQ.defenseManager.getArmy(ent.getMetadata(PlayerID, "PartOfArmy"));
		if (army)
			army.removeOwn(gameState, ent.id());
	}
};

m.TransportPlan.prototype.Serialize = function()
{
	return {
		"ID": this.ID,
		"flotilla": this.flotilla,
		"endPos": this.endPos,
		"endIndex": this.endIndex,
		"startIndex": this.startIndex,
		"sea": this.sea,
		"state": this.state,
		"boardingPos": this.boardingPos,
		"needTransportShips": this.needTransportShips,
		"nTry": this.nTry,
		"canceled": this.canceled,
		"unloaded": this.unloaded,
		"recovered": this.recovered
	};
};

m.TransportPlan.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];

	this.failed = false;
};

return m;
}(PETRA);
