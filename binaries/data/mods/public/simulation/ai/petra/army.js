var PETRA = function(m)
{

/**
 * Defines an army
 * An army is a collection of own entities and enemy entities.
 * This doesn't use entity collections are they aren't really useful
 * and it would probably slow the rest of the system down too much.
 * All entities are therefore lists of ID
 * Inherited by the defense manager and several of the attack manager's attack plan.
 */

m.Army = function(gameState, ownEntities, foeEntities)
{
	this.ID = gameState.ai.uniqueIDs.armies++;

	this.Config = gameState.ai.Config;
	this.defenseRatio = this.Config.Defense.defenseRatio;
	this.compactSize = this.Config.Defense.armyCompactSize;
	this.breakawaySize = this.Config.Defense.armyBreakawaySize;

	// average
	this.foePosition = [0,0];
	this.positionLastUpdate = gameState.ai.elapsedTime;

	// Some caching
	// A list of our defenders that were tasked with attacking a particular unit
	// This doesn't mean that they actually are since they could move on to something else on their own.
	this.assignedAgainst = {};
	// who we assigned against, for quick removal.
	this.assignedTo = {};

	this.foeEntities = [];
	this.foeStrength = 0;

	this.ownEntities = [];
	this.ownStrength = 0;

	// actually add units
	for (let id of foeEntities)
		this.addFoe(gameState, id, true);
	for (let id of ownEntities)
		this.addOwn(gameState, id);

	this.recalculatePosition(gameState, true);

	return true;
};

/** if not forced, will only recalculate if on a different turn. */
m.Army.prototype.recalculatePosition = function(gameState, force)
{
	if (!force && this.positionLastUpdate === gameState.ai.elapsedTime)
		return;

	let npos = 0;
	let pos = [0, 0];
	for (let id of this.foeEntities)
	{
		let ent = gameState.getEntityById(id);
		if (!ent || !ent.position())
			continue;
		npos++;
		let epos = ent.position();
		pos[0] += epos[0];
		pos[1] += epos[1];
	}
	// if npos = 0, the army must have been destroyed and will be removed next turn. keep previous position
	if (npos > 0)
	{
		this.foePosition[0] = pos[0]/npos;
		this.foePosition[1] = pos[1]/npos;
	}

	this.positionLastUpdate = gameState.ai.elapsedTime;
};

m.Army.prototype.recalculateStrengths = function (gameState)
{
	this.ownStrength = 0;
	this.foeStrength = 0;

	for (let id of this.foeEntities)
		this.evaluateStrength(gameState.getEntityById(id));
	for (let id of this.ownEntities)
		this.evaluateStrength(gameState.getEntityById(id), true);
};

/** adds or remove the strength of the entity either to the enemy or to our units. */
m.Army.prototype.evaluateStrength = function (ent, isOwn, remove)
{
	if (!ent)
		return;

	let entStrength;
	if (ent.hasClass("Structure"))
	{
		if (ent.owner() !== PlayerID)
			entStrength = ent.getDefaultArrow() ? 6*ent.getDefaultArrow() : 4;
		else	// small strength used only when we try to recover capture points
			entStrength = 2;
	}
	else
		entStrength = m.getMaxStrength(ent);

	// TODO adapt the getMaxStrength function for animals.
	// For the time being, just increase it for elephants as the returned value is too small.
	if (ent.hasClass("Animal") && ent.hasClass("Elephant"))
		entStrength *= 3;

	if (remove)
		entStrength *= -1;

	if (isOwn)
		this.ownStrength += entStrength;
	else
		this.foeStrength += entStrength;
};

/**
 * add an entity to the enemy army
 * Will return true if the entity was added and false otherwise.
 * won't recalculate our position but will dirty it.
 * force is true at army creation or when merging armies, so in this case we should add it even if far
 */
m.Army.prototype.addFoe = function (gameState, enemyId, force)
{
	if (this.foeEntities.indexOf(enemyId) !== -1)
		return false;
	let ent = gameState.getEntityById(enemyId);
	if (!ent || !ent.position())
		return false;

	// check distance
	if (!force && API3.SquareVectorDistance(ent.position(), this.foePosition) > this.compactSize)
			return false;

	this.foeEntities.push(enemyId);
	this.assignedAgainst[enemyId] = [];
	this.positionLastUpdate = 0;
	this.evaluateStrength(ent);
	ent.setMetadata(PlayerID, "PartOfArmy", this.ID);

	return true;
};

/**
 * returns true if the entity was removed and false otherwise.
 * TODO: when there is a technology update, we should probably recompute the strengths, or weird stuffs will happen.
 */
m.Army.prototype.removeFoe = function (gameState, enemyId, enemyEntity)
{
	let idx = this.foeEntities.indexOf(enemyId);
	if (idx === -1)
		return false;

	this.foeEntities.splice(idx, 1);

	this.assignedAgainst[enemyId] = undefined;
	for (let to in this.assignedTo)
		if (this.assignedTo[to] == enemyId)
			this.assignedTo[to] = undefined;

	let ent = enemyEntity ? enemyEntity : gameState.getEntityById(enemyId);
	if (ent)    // TODO recompute strength when no entities (could happen if capture+destroy)
	{
		this.evaluateStrength(ent, false, true);
		ent.setMetadata(PlayerID, "PartOfArmy", undefined);
	}

	return true;
};

/**
 * adds a defender but doesn't assign him yet.
 * force is true when merging armies, so in this case we should add it even if no position as it can be in a ship
 */
m.Army.prototype.addOwn = function (gameState, id, force)
{
	if (this.ownEntities.indexOf(id) !== -1)
		return false;
	let ent = gameState.getEntityById(id);
	if (!ent || (!ent.position() && !force))
		return false;

	this.ownEntities.push(id);
	this.evaluateStrength(ent, true);
	ent.setMetadata(PlayerID, "PartOfArmy", this.ID);
	this.assignedTo[id] = 0;

	let plan = ent.getMetadata(PlayerID, "plan");
	if (plan !== undefined)
		ent.setMetadata(PlayerID, "plan", -2);
	else
 		ent.setMetadata(PlayerID, "plan", -3);
	let subrole = ent.getMetadata(PlayerID, "subrole");
	if (subrole === undefined || subrole !== "defender")
		ent.setMetadata(PlayerID, "formerSubrole", subrole);
	ent.setMetadata(PlayerID, "subrole", "defender");
	return true;
};

m.Army.prototype.removeOwn = function (gameState, id, Entity)
{
	let idx = this.ownEntities.indexOf(id);
	if (idx === -1)
		return false;

	this.ownEntities.splice(idx, 1);

	if (this.assignedTo[id] !== 0)
	{
		let temp = this.assignedAgainst[this.assignedTo[id]];
		if (temp)
			temp.splice(temp.indexOf(id), 1);
	}
	this.assignedTo[id] = undefined;

	let ent = Entity ? Entity : gameState.getEntityById(id);
	if (!ent)
		return true;

	this.evaluateStrength(ent, true, true);
	ent.setMetadata(PlayerID, "PartOfArmy", undefined);
	if (ent.getMetadata(PlayerID, "plan") === -2)
		ent.setMetadata(PlayerID, "plan", -1);
	else
		ent.setMetadata(PlayerID, "plan", undefined);

	let formerSubrole = ent.getMetadata(PlayerID, "formerSubrole");
	if (formerSubrole !== undefined)
		ent.setMetadata(PlayerID, "subrole", formerSubrole);
	else
		ent.setMetadata(PlayerID, "subrole", undefined);
	ent.setMetadata(PlayerID, "formerSubrole", undefined);

/*
	// TODO be sure that all units in the transport need the cancelation
	if (!ent.position())	// this unit must still be in a transport plan ... try to cancel it
	{
		let planID = ent.getMetadata(PlayerID, "transport");
		// no plans must mean that the unit was in a ship which was destroyed, so do nothing
		if (planID)
		{
			if (gameState.ai.Config.debug > 0)
				warn("ent from army still in transport plan: plan " + planID + " canceled");
			let plan = gameState.ai.HQ.navalManager.getPlan(planID);
			if (plan && !plan.canceled)
				plan.cancelTransport(gameState);
		}
	}
*/

	return true;
};

/**
 * Special army set to capture a gaia building.
 * It must only contain one foe (the building to capture) and never be merged
 */
m.Army.prototype.isCapturing = function (gameState)
{
	if (this.foeEntities.length != 1)
		return false;
	let ent = gameState.getEntityById(this.foeEntities[0]);
	return ent && ent.hasClass("Structure");
};

/**
 * this one is "undefined entity" proof because it's called at odd times.
 * Orders a unit to attack an enemy.
 * overridden by specific army classes.
 */
m.Army.prototype.assignUnit = function (gameState, entID)
{
};

/**
 * resets the army properly.
 * assumes we already cleared dead units.
 */
m.Army.prototype.clear = function (gameState)
{
	while (this.foeEntities.length > 0)
		this.removeFoe(gameState, this.foeEntities[0]);

	// Go back to our territory, using the nearest (defensive if any) structure.
	let armyPos = [0, 0];
	let npos = 0;
	for (let entId of this.ownEntities)
	{
		let ent = gameState.getEntityById(entId);
		if (!ent)
			continue;
		let pos = ent.position();
		if (!pos || gameState.ai.HQ.territoryMap.getOwner(pos) === PlayerID)
			continue;
		armyPos[0] += pos[0];
		armyPos[1] += pos[1];
		++npos;
	}
	let nearestStructurePos;
	let defensiveFound;
	let distmin;
	let radius;
	if (npos > 0)
	{
		armyPos[0] /= npos;
		armyPos[1] /= npos;
		let armyAccess = gameState.ai.accessibility.getAccessValue(armyPos);
		for (let struct of gameState.getOwnStructures().values())
		{
			let pos = struct.position();
			if (!pos || gameState.ai.HQ.territoryMap.getOwner(pos) !== PlayerID)
				continue;
			if (m.getLandAccess(gameState, struct) !== armyAccess)
				continue;
			let defensiveStruct = struct.hasDefensiveFire();
			if (defensiveFound && !defensiveStruct)
				continue;
			let dist = API3.SquareVectorDistance(armyPos, pos);
			if (distmin && dist > distmin && (defensiveFound || !defensiveStruct))
				continue;
			if (defensiveStruct)
				defensiveFound = true;
			distmin = dist;
			nearestStructurePos = pos;
			radius = struct.obstructionRadius();
		}
	}
	while (this.ownEntities.length > 0)
	{
		let entId = this.ownEntities[0];
		this.removeOwn(gameState, entId);
		let ent = gameState.getEntityById(entId);
		if (ent)
		{
			if (!ent.position() || ent.getMetadata(PlayerID, "transport") !== undefined ||
			                       ent.getMetadata(PlayerID, "transporter") !== undefined)
				continue;
			if (ent.healthLevel() < this.Config.garrisonHealthLevel.low &&
			    gameState.ai.HQ.defenseManager.garrisonAttackedUnit(gameState, ent))
				continue;

			if (nearestStructurePos && gameState.ai.HQ.territoryMap.getOwner(ent.position()) !== PlayerID)
				ent.moveToRange(nearestStructurePos[0], nearestStructurePos[1], radius, radius+5);
			else
				ent.stopMoving();
		}
	}

	this.assignedAgainst = {};
	this.assignedTo = {};

	this.recalculateStrengths(gameState);
	this.recalculatePosition(gameState);
};

/**
 * merge this army with another properly.
 * assumes units are in only one army.
 * also assumes that all have been properly cleaned up (no dead units).
 */
m.Army.prototype.merge = function (gameState, otherArmy)
{
	// copy over all parameters.
	for (let i in otherArmy.assignedAgainst)
	{
		if (this.assignedAgainst[i] === undefined)
			this.assignedAgainst[i] = otherArmy.assignedAgainst[i];
		else
			this.assignedAgainst[i] = this.assignedAgainst[i].concat(otherArmy.assignedAgainst[i]);
	}
	for (let i in otherArmy.assignedTo)
		this.assignedTo[i] = otherArmy.assignedTo[i];

	for (let id of otherArmy.foeEntities)
		this.addFoe(gameState, id, true);
	// TODO: reassign those ?
	for (let id of otherArmy.ownEntities)
		this.addOwn(gameState, id, true);

	this.recalculatePosition(gameState, true);
	this.recalculateStrengths(gameState);

	return true;
};

m.Army.prototype.checkEvents = function (gameState, events)
{
	// Warning the metadata is already cloned in shared.js. Futhermore, changes should be done before destroyEvents
	// otherwise it would remove the old entity from this army list
	// TODO we should may-be reevaluate the strength
	for (let evt of events.EntityRenamed)	// take care of promoted and packed units
	{
		if (this.foeEntities.indexOf(evt.entity) !== -1)
		{
			let ent = gameState.getEntityById(evt.newentity);
			if (ent && ent.templateName().indexOf("resource|") !== -1)  // corpse of animal killed
				continue;
			let idx = this.foeEntities.indexOf(evt.entity);
			this.foeEntities[idx] = evt.newentity;
			this.assignedAgainst[evt.newentity] = this.assignedAgainst[evt.entity];
			this.assignedAgainst[evt.entity] = undefined;
			for (let to in this.assignedTo)
				if (this.assignedTo[to] === evt.entity)
					this.assignedTo[to] = evt.newentity;
		}
		else if (this.ownEntities.indexOf(evt.entity) !== -1)
		{
			let idx = this.ownEntities.indexOf(evt.entity);
			this.ownEntities[idx] = evt.newentity;
			this.assignedTo[evt.newentity] = this.assignedTo[evt.entity];
			this.assignedTo[evt.entity] = undefined;
			for (let against in this.assignedAgainst)
			{
				if (!this.assignedAgainst[against])
					continue;
				if (this.assignedAgainst[against].indexOf(evt.entity) !== -1)
					this.assignedAgainst[against][this.assignedAgainst[against].indexOf(evt.entity)] = evt.newentity;
			}
		}
	}

	for (let evt of events.Garrison)
		this.removeFoe(gameState, evt.entity);

	for (let evt of events.OwnershipChanged)	// captured
	{
		if (!gameState.isPlayerEnemy(evt.to))
			this.removeFoe(gameState, evt.entity);
		else if (evt.from === PlayerID)
			this.removeOwn(gameState, evt.entity);
	}

	for (let evt of events.Destroy)
	{
		let entityObj = evt.entityObj || undefined;
		// we may have capture+destroy, so do not trust owner and check all possibilities
		this.removeOwn(gameState, evt.entity, entityObj);
		this.removeFoe(gameState, evt.entity, entityObj);
	}
};

/** assumes cleaned army. */
m.Army.prototype.onUpdate = function (gameState)
{
	if (this.isCapturing(gameState))
	{
		// Check if we have still some capturePoints to recover
		// and if not, remove this foe from the list (capture army have only one foe)
		let capture = gameState.getEntityById(this.foeEntities[0]).capturePoints();
		if (capture !== undefined)
		{
			for (let j = 0; j < capture.length; ++j)
				if (gameState.isPlayerEnemy(j) && capture[j] > 0)
					return [];
		}
		this.removeFoe(gameState, this.foeEntities[0]);
		return [];
	}

	let breakaways = [];
	// TODO: assign unassigned defenders, cleanup of a few things.
	// perhaps occasional strength recomputation

	// occasional update or breakaways, positions…
	if (gameState.ai.elapsedTime - this.positionLastUpdate > 5)
	{
		this.recalculatePosition(gameState);
		this.positionLastUpdate = gameState.ai.elapsedTime;

		// Check for breakaways.
		for (let i = 0; i < this.foeEntities.length; ++i)
		{
			let id = this.foeEntities[i];
			let ent = gameState.getEntityById(id);
			if (!ent || !ent.position())
				continue;
			if (API3.SquareVectorDistance(ent.position(), this.foePosition) > this.breakawaySize)
			{
				breakaways.push(id);
				if (this.removeFoe(gameState, id))
					i--;
			}
		}

		this.recalculatePosition(gameState);
	}

	return breakaways;
};

return m;
}(PETRA);
