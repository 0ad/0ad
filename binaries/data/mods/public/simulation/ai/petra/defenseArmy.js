var PETRA = function(m)
{

/** Armies used by the defense manager.
 * An army is a collection of own entities and enemy entities.
 *
 * Types of armies:
 * "default":   army to counter an invading army
 * "capturing": army set to capture a gaia building or recover capture points to one of its own structures
 *            It must contain only one foe (the building to capture) and never be merged
 */
m.DefenseArmy = function(gameState, foeEntities, type)
{
	this.ID = gameState.ai.uniqueIDs.armies++;
	this.type = type || "default";

	this.Config = gameState.ai.Config;
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

	this.recalculatePosition(gameState, true);

	return true;
};

/**
 * add an entity to the enemy army
 * Will return true if the entity was added and false otherwise.
 * won't recalculate our position but will dirty it.
 * force is true at army creation or when merging armies, so in this case we should add it even if far
 */
m.DefenseArmy.prototype.addFoe = function(gameState, enemyId, force)
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
m.DefenseArmy.prototype.removeFoe = function(gameState, enemyId, enemyEntity)
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
m.DefenseArmy.prototype.addOwn = function(gameState, id, force)
{
	if (this.ownEntities.indexOf(id) !== -1)
		return false;
	let ent = gameState.getEntityById(id);
	if (!ent || !ent.position() && !force)
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

m.DefenseArmy.prototype.removeOwn = function(gameState, id, Entity)
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
 * resets the army properly.
 * assumes we already cleared dead units.
 */
m.DefenseArmy.prototype.clear = function(gameState)
{
	while (this.foeEntities.length > 0)
		this.removeFoe(gameState, this.foeEntities[0]);

	// Go back to our or allied territory if needed
	let posOwn = [0, 0];
	let nOwn = 0;
	let posAlly = [0, 0];
	let nAlly = 0;
	let posOther = [0, 0];
	let nOther = 0;
	for (let entId of this.ownEntities)
	{
		let ent = gameState.getEntityById(entId);
		if (!ent || !ent.position())
			continue;
		let pos = ent.position();
		let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(pos);
		if (territoryOwner === PlayerID)
		{
			posOwn[0] += pos[0];
			posOwn[1] += pos[1];
			++nOwn;
		}
		else if (gameState.isPlayerMutualAlly(territoryOwner))
		{
			posAlly[0] += pos[0];
			posAlly[1] += pos[1];
			++nAlly;
		}
		else
		{
			posOther[0] += pos[0];
			posOther[1] += pos[1];
			++nOther;
		}
	}
	let destination;
	let defensiveFound;
	let distmin;
	let radius = 0;
	if (nOwn > 0)
		destination = [posOwn[0]/nOwn, posOwn[1]/nOwn];
	else if (nAlly > 0)
		destination = [posAlly[0]/nAlly, posAlly[1]/nAlly];
	else
	{
		posOther[0] /= nOther;
		posOther[1] /= nOther;
		let armyAccess = gameState.ai.accessibility.getAccessValue(posOther);
		for (let struct of gameState.getAllyStructures().values())
		{
			let pos = struct.position();
			if (!pos || !gameState.isPlayerMutualAlly(gameState.ai.HQ.territoryMap.getOwner(pos)))
				continue;
			if (m.getLandAccess(gameState, struct) !== armyAccess)
				continue;
			let defensiveStruct = struct.hasDefensiveFire();
			if (defensiveFound && !defensiveStruct)
				continue;
			let dist = API3.SquareVectorDistance(posOther, pos);
			if (distmin && dist > distmin && (defensiveFound || !defensiveStruct))
				continue;
			if (defensiveStruct)
				defensiveFound = true;
			distmin = dist;
			destination = pos;
			radius = struct.obstructionRadius().max;
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

			if (destination && !gameState.isPlayerMutualAlly(gameState.ai.HQ.territoryMap.getOwner(ent.position())))
				ent.moveToRange(destination[0], destination[1], radius, radius+5);
			else
				ent.stopMoving();
		}
	}

	this.assignedAgainst = {};
	this.assignedTo = {};

	this.recalculateStrengths(gameState);
	this.recalculatePosition(gameState);
};

m.DefenseArmy.prototype.assignUnit = function(gameState, entID)
{
	// we'll assume this defender is ours already.
	// we'll also override any previous assignment

	let ent = gameState.getEntityById(entID);
	if (!ent || !ent.position())
		return false;

	// try to return its resources, and if any, the attack order will be queued
	let queued = m.returnResources(gameState, ent);

	let idMin;
	let distMin;
	let idMinAll;
	let distMinAll;
	for (let id of this.foeEntities)
	{
		let eEnt = gameState.getEntityById(id);
		if (!eEnt || !eEnt.position())	// probably can't happen.
			continue;

		if (eEnt.hasClass("Unit") && eEnt.unitAIOrderData() && eEnt.unitAIOrderData().length &&
			eEnt.unitAIOrderData()[0].target && eEnt.unitAIOrderData()[0].target == entID)
		{   // being attacked  >>> target the unit
			idMin = id;
			break;
		}

		// already enough units against it
		if (this.assignedAgainst[id].length > 8 ||
			this.assignedAgainst[id].length > 5 && !eEnt.hasClass("Hero") && !m.isSiegeUnit(eEnt))
			continue;

		let dist = API3.SquareVectorDistance(ent.position(), eEnt.position());
		if (idMinAll === undefined || dist < distMinAll)
		{
			idMinAll = id;
			distMinAll = dist;
		}
		if (this.assignedAgainst[id].length > 2)
			continue;
		if (idMin === undefined || dist < distMin)
		{
			idMin = id;
			distMin = dist;
		}
	}

	let idFoe;
	if (idMin !== undefined)
		idFoe = idMin;
	else if (idMinAll !== undefined)
		idFoe = idMinAll;
	else
		return false;

	let ownIndex = gameState.ai.accessibility.getAccessValue(ent.position());
	let foeEnt = gameState.getEntityById(idFoe);
	let foePosition = foeEnt.position();
	let foeIndex = gameState.ai.accessibility.getAccessValue(foePosition);
	if (ownIndex == foeIndex || ent.hasClass("Ship"))
	{
		this.assignedTo[entID] = idFoe;
		this.assignedAgainst[idFoe].push(entID);
		ent.attack(idFoe, m.allowCapture(gameState, ent, foeEnt), queued);
	}
	else
		gameState.ai.HQ.navalManager.requireTransport(gameState, ent, ownIndex, foeIndex, foePosition);
	return true;
};

m.DefenseArmy.prototype.getType = function()
{
	return this.type;
};

m.DefenseArmy.prototype.getState = function()
{
	if (!this.foeEntities.length)
		return 0;
	return 1;
};

/**
 * merge this army with another properly.
 * assumes units are in only one army.
 * also assumes that all have been properly cleaned up (no dead units).
 */
m.DefenseArmy.prototype.merge = function(gameState, otherArmy)
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

m.DefenseArmy.prototype.needsDefenders = function(gameState)
{
	let defenseRatio;
	let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(this.foePosition);
	if (territoryOwner == PlayerID)
		defenseRatio = this.Config.Defense.defenseRatio.own;
	else if (gameState.isPlayerAlly(territoryOwner))
	{
		defenseRatio = this.Config.Defense.defenseRatio.ally;
		let numExclusiveAllies = 0;
		for (let p = 1; p < gameState.sharedScript.playersData.length; ++p)
			if (p != territoryOwner && gameState.sharedScript.playersData[p].isAlly[territoryOwner])
				++numExclusiveAllies;
		defenseRatio /= (1 + 0.5*Math.max(0, numExclusiveAllies-1));
	}
	else
		defenseRatio = this.Config.Defense.defenseRatio.neutral;

	// some preliminary checks because we don't update for tech so entStrength removed can be > entStrength added
	if (this.foeStrength <= 0 || this.ownStrength <= 0)
		this.recalculateStrengths(gameState);

	if (this.foeStrength * defenseRatio <= this.ownStrength)
		return false;
	return this.foeStrength * defenseRatio - this.ownStrength;
};


/** if not forced, will only recalculate if on a different turn. */
m.DefenseArmy.prototype.recalculatePosition = function(gameState, force)
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

m.DefenseArmy.prototype.recalculateStrengths = function(gameState)
{
	this.ownStrength = 0;
	this.foeStrength = 0;

	for (let id of this.foeEntities)
		this.evaluateStrength(gameState.getEntityById(id));
	for (let id of this.ownEntities)
		this.evaluateStrength(gameState.getEntityById(id), true);
};

/** adds or remove the strength of the entity either to the enemy or to our units. */
m.DefenseArmy.prototype.evaluateStrength = function(ent, isOwn, remove)
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

m.DefenseArmy.prototype.checkEvents = function(gameState, events)
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

m.DefenseArmy.prototype.update = function(gameState)
{
	for (let entId of this.ownEntities)
	{
		let ent = gameState.getEntityById(entId);
		if (!ent)
			continue;
		let orderData = ent.unitAIOrderData();
		if (!orderData.length && !ent.getMetadata(PlayerID, "transport"))
			this.assignUnit(gameState, entId);
		else if (orderData.length && orderData[0].target && orderData[0].attackType && orderData[0].attackType === "Capture")
		{
			let target = gameState.getEntityById(orderData[0].target);
			if (target && !m.allowCapture(gameState, ent, target))
				ent.attack(orderData[0].target, false);
		}
	}

	if (this.type == "capturing")
	{
		if (this.foeEntities.length && gameState.getEntityById(this.foeEntities[0]))
		{
			// Check if we still still some capturePoints to recover
			// and if not, remove this foe from the list (capture army have only one foe)
			let capture = gameState.getEntityById(this.foeEntities[0]).capturePoints();
			if (capture)
				for (let j = 0; j < capture.length; ++j)
					if (gameState.isPlayerEnemy(j) && capture[j] > 0)
						return [];
			this.removeFoe(gameState, this.foeEntities[0]);
		}
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

m.DefenseArmy.prototype.Serialize = function()
{
	return {
		"ID": this.ID,
		"type": this.type,
		"foePosition": this.foePosition,
		"positionLastUpdate": this.positionLastUpdate,
		"assignedAgainst": this.assignedAgainst,
		"assignedTo": this.assignedTo,
		"foeEntities": this.foeEntities,
		"foeStrength": this.foeStrength,
		"ownEntities": this.ownEntities,
		"ownStrength": this.ownStrength
	};
};

m.DefenseArmy.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
