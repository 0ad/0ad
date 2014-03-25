var PETRA = function(m)
{

/* Defines an army
 * An army is a collection of own entities and enemy entities.
 * This doesn't use entity collections are they aren't really useful
 * and it would probably slow the rest of the system down too much.
 * All entities are therefore lists of ID
 * Inherited by the defense manager and several of the attack manager's attack plan.
 */

m.Army = function(gameState, owner, ownEntities, foeEntities)
{
	this.ID = m.playerGlobals[PlayerID].uniqueIDArmy++;

	this.Config = owner.Config; 
	this.defenseRatio = this.Config.Defense.defenseRatio;
	this.compactSize = this.Config.Defense.armyCompactSize;
	this.breakawaySize = this.Config.Defense.armyBreakawaySize;
		
	// average
	this.foePosition = [0,0];		
	this.ownPosition = [0,0];
	this.positionLastUpdate = gameState.getTimeElapsed();

	// Some caching
	// A list of our defenders that were tasked with attacking a particular unit
	// This doesn't mean that they actually are since they could move on to something else on their own.
	this.assignedAgainst = {};
	// who we assigned against, for quick removal.
	this.assignedTo = {};
	
	// For substrengths, format is "name": [classes]

	this.foeEntities = [];
	this.foeStrength = 0;
	this.foeSubStrength = {};
	
	this.ownEntities = [];
	this.ownStrength = 0;
	this.ownSubStrength = {};
	
	// actually add units
	for (var i in foeEntities)
		this.addFoe(gameState,foeEntities[i], true);
	for (var i in ownEntities)
		this.addOwn(gameState,ownEntities[i]);
	
	this.recalculatePosition(gameState, true);

	return true;
}

// if not forced, will only recalculate if on a different turn.
m.Army.prototype.recalculatePosition = function(gameState, force)
{
	if (!force && this.positionLastUpdate === gameState.getTimeElapsed())
		return;
	var pos = [0,0];
	if (this.foeEntities.length !== 0)
	{
		for each (var id in this.foeEntities)
		{
			var ent = gameState.getEntityById(id);
			var epos = ent.position();
			pos[0] += epos[0];
			pos[1] += epos[1];
		}
		this.foePosition[0] = pos[0]/this.foeEntities.length;
		this.foePosition[1] = pos[1]/this.foeEntities.length;
	} else
		this.foePosition = [0,0];

	pos = [0,0];
	if (this.ownEntities.length !== 0)
	{
		for each (var id in this.ownEntities)
		{
			var ent = gameState.getEntityById(id);
			var epos = ent.position();
			pos[0] += epos[0];
			pos[1] += epos[1];
		}
		this.ownPosition[0] = pos[0]/this.ownEntities.length;
		this.ownPosition[1] = pos[1]/this.ownEntities.length;
	} else
		this.ownPosition = [0,0];

	this.positionLastUpdate = gameState.getTimeElapsed();
}

// helper
m.Army.prototype.recalculateStrengths = function (gameState)
{
	this.ownStrength  = 0;
	this.foeStrength  = 0;
	
	// todo: deal with specifics.

	for each (var id in this.foeEntities)
		this.evaluateStrength(gameState.getEntityById(id));
	for each (var id in this.ownEntities)
		this.evaluateStrength(gameState.getEntityById(id), true);
}

// adds or remove the strength of the entity either to the enemy or to our units.
m.Army.prototype.evaluateStrength = function (ent, isOwn, remove)
{
	var entStrength = m.getMaxStrength(ent);
	if (remove)
		entStrength *= -1;

	if (isOwn)
		this.ownStrength += entStrength;
	else
		this.foeStrength += entStrength;
	
	// todo: deal with specifics.
}

// add an entity to the enemy army
// Will return true if the entity was added and false otherwise.
// won't recalculate our position but will dirty it.
m.Army.prototype.addFoe = function (gameState, enemyID, force)
{
	if (this.foeEntities.indexOf(enemyID) !== -1)
		return false;
	var ent = gameState.getEntityById(enemyID);
	if (ent === undefined || ent.position() === undefined)
		return false;
	
	// check distance
	if (!force && API3.SquareVectorDistance(ent.position(), this.foePosition) > this.compactSize)
			return false;
	
	this.foeEntities.push(enemyID);
	this.assignedAgainst[enemyID] = [];
	this.positionLastUpdate = 0;
	this.evaluateStrength(ent);
	ent.setMetadata(PlayerID, "PartOfArmy", this.ID);

	return true;
}

// returns true if the entity was removed and false otherwise.
// TODO: when there is a technology update, we should probably recompute the strengths, or weird stuffs will happen.
m.Army.prototype.removeFoe = function (gameState, enemyID, enemyEntity)
{
	var idx = this.foeEntities.indexOf(enemyID);
	if (idx === -1)
		return false;
	var ent = enemyEntity === undefined ? gameState.getEntityById(enemyID) : enemyEntity;
	if (ent === undefined)
	{
		warn("Trying to remove a non-existing enemy entity, crashing for stacktrace");
		xgzrg();
	}	
	this.foeEntities.splice(idx, 1);
	this.evaluateStrength(ent, false, true);
	ent.setMetadata(PlayerID, "PartOfArmy", undefined);
	
	this.assignedAgainst[enemyID] = undefined;
	for (var to in this.assignedTo)
		if (this.assignedTo[to] == enemyID)
			this.assignedTo[to] = undefined;
	
	return true;
}

// adds a defender but doesn't assign him yet.
m.Army.prototype.addOwn = function (gameState, ID)
{
	if (this.ownEntities.indexOf(ID) !== -1)
		return false;
	var ent = gameState.getEntityById(ID);
	if (ent === undefined || ent.position() === undefined)
		return false;

	this.ownEntities.push(ID);
	this.evaluateStrength(ent, true);
	ent.setMetadata(PlayerID, "PartOfArmy", this.ID);
	this.assignedTo[ID] = 0;

	var formerSubrole = ent.getMetadata(PlayerID, "subrole");
	if (formerSubrole && formerSubrole === "defender")    // can happen when armies are merged for example
		return true;
	if (formerSubrole !== undefined)
		ent.setMetadata(PlayerID, "formerSubrole", formerSubrole);
	ent.setMetadata(PlayerID, "subrole", "defender");
	return true;
}

m.Army.prototype.removeOwn = function (gameState, ID, Entity)
{
	var idx = this.ownEntities.indexOf(ID);
	if (idx === -1)
		return false;
	var ent = Entity === undefined ? gameState.getEntityById(ID) : Entity;
	if (ent === undefined)
	{
		warn( ID);
		warn("Trying to remove a non-existing entity, crashing for stacktrace");
		xgzrg();
	}

	this.ownEntities.splice(idx, 1);
	this.evaluateStrength(ent, true, true);
	ent.setMetadata(PlayerID, "PartOfArmy", undefined);

	if (this.assignedTo[ID] !== 0)
	{
		var temp = this.assignedAgainst[this.assignedTo[ID]];
		if (temp)
			temp.splice(temp.indexOf(ID), 1);
	}
	this.assignedTo[ID] = undefined;

		
	var formerSubrole = ent.getMetadata(PlayerID, "formerSubrole");
	if (formerSubrole !== undefined)
		ent.setMetadata(PlayerID, "subrole", formerSubrole);
	else
		ent.setMetadata(PlayerID, "subrole", undefined);

	return true;
}

// this one is "undefined entity" proof because it's called at odd times.
// Orders a unit to attack an enemy.
// overridden by specific army classes.
m.Army.prototype.assignUnit = function (gameState, entID)
{
}

// resets the army properly.
// assumes we already cleared dead units.
m.Army.prototype.clear = function (gameState, events)
{
	while(this.foeEntities.length > 0)
		this.removeFoe(gameState,this.foeEntities[0]);
	while(this.ownEntities.length > 0)
		this.removeOwn(gameState,this.ownEntities[0]);

	this.assignedAgainst = {};
	this.assignedTo = {};

	this.recalculateStrengths(gameState);
	this.recalculatePosition(gameState);
}

// merge this army with another properly.
// assumes units are in only one army.
// also assumes that all have been properly cleaned up (no dead units).
m.Army.prototype.merge = function (gameState, otherArmy)
{
	// copy over all parameters.
	for (var i in otherArmy.assignedAgainst)
	{
		if (this.assignedAgainst[i] === undefined)
			this.assignedAgainst[i] = otherArmy.assignedAgainst[i];
		else
			this.assignedAgainst[i] = this.assignedAgainst[i].concat(otherArmy.assignedAgainst[i]);
	}
	for (var i in otherArmy.assignedTo)
		this.assignedTo[i] = otherArmy.assignedTo[i];
	
	for each (var id in otherArmy.foeEntities)
		this.addFoe(gameState, id);
	// TODO: reassign those ?
	for each (var id in otherArmy.ownEntities)
		this.addOwn(gameState, id);

	this.recalculatePosition(gameState, true);
	this.recalculateStrengths(gameState);
	
	return true;
}

// TODO: when there is a technology update, we should probably recompute the strengths, or weird stuffs might happen.
m.Army.prototype.checkEvents = function (gameState, events)
{
	var renameEvents = events["EntityRenamed"];   // take care of promoted and packed units
	var destroyEvents = events["Destroy"];
	var convEvents = events["OwnershipChanged"];
	var garriEvents = events["Garrison"];

	// Warning the metadata is already cloned in shared.js. Futhermore, changes should be done before destroyEvents
	// otherwise it would remove the old entity from this army list
	// TODO we should may-be reevaluate the strength
	for each (var msg in renameEvents)
	{
		if (this.foeEntities.indexOf(msg.entity) !== -1)
		{
			var idx = this.foeEntities.indexOf(msg.entity);
			this.foeEntities[idx] = msg.newentity;
			this.assignedAgainst[msg.newentity] = this.assignedAgainst[msg.entity];
			this.assignedAgainst[msg.entity] = undefined;
			for (var to in this.assignedTo)
				if (this.assignedTo[to] == msg.entity)
					this.assignedTo[to] = msg.newentity;
		}
		else if (this.ownEntities.indexOf(msg.entity) !== -1)
		{
			var idx = this.ownEntities.indexOf(msg.entity);
			this.ownEntities[idx] = msg.newentity;
			this.assignedTo[msg.newentity] = this.assignedTo[msg.entity];
			this.assignedTo[msg.entity] = undefined;
			for (var against in this.assignedAgainst)
			{
				if (!this.assignedAgainst[against])
					continue;
				if (this.assignedAgainst[against].indexOf(msg.entity) !== -1)
					this.assignedAgainst[against][this.assignedAgainst[against].indexOf(msg.entity)] = msg.newentity;
			}
		}
	}

	for each (var msg in destroyEvents)
	{
		if (msg.entityObj === undefined)
			continue;
		if (msg.entityObj._entity.owner === PlayerID)
			this.removeOwn(gameState, msg.entity, msg.entityObj);
		else
			this.removeFoe(gameState, msg.entity, msg.entityObj);
	}

	for each (var msg in garriEvents)
		this.removeFoe(gameState, msg.entity);

	for each (var msg in convEvents)
	{
		if (msg.to === PlayerID)
		{
			// we have converted an enemy, let's assign it as a defender
			if (this.removeFoe(gameState, msg.entity))
				this.addOwn(gameState, msg.entity);
		} else if (msg.from === PlayerID)
			this.removeOwn(gameState, msg.entity);	// TODO: add allies
	}
}

// assumes cleaned army.
// this only checks for breakaways.
m.Army.prototype.onUpdate = function (gameState)
{
	var breakaways = [];
	// TODO: assign unassigned defenders, cleanup of a few things.
	// perhaps occasional strength recomputation
	
	// occasional update or breakaways, positions…
	if (gameState.getTimeElapsed() - this.positionLastUpdate > 5000)
	{
		this.recalculatePosition(gameState);
		this.positionLastUpdate = gameState.getTimeElapsed();
	
		// Check for breakaways.
		for (var i = 0; i < this.foeEntities.length; ++i)
		{
			var id = this.foeEntities[i];
			var ent = gameState.getEntityById(id);
			if (API3.SquareVectorDistance(ent.position(), this.foePosition) > this.breakawaySize)
			{
				breakaways.push(id);
				if(this.removeFoe(gameState, id))
					i--;
			}
		}
		
		this.recalculatePosition(gameState);
	}

	return breakaways;
}

return m;
}(PETRA);
