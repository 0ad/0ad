var AEGIS = function(m)
{

// Defines an army for the defence manager's use.
/*
 An army is a collection of an enemy player's units
 And of my own defenders against those units.
 This doesn't use entity collections are they aren't really useful
 and it would probably slow the rest of the system down too much.
 All entities are therefore lists of ID
 */

m.Army = function(gameState, defManager, entities, ownEntities, alliedEntities)
{
	this.Config = defManager.Config; 
	this.defenceRatio = this.Config.Defence.defenceRatio;
	this.compactSize = this.Config.Defence.armyCompactSize;
	this.breakawaySize = this.Config.Defence.armyBreakawaySize;
	this.watchTSMultiplicator = this.Config.Defence.armyStrengthWariness;
	this.watchDecrement = this.Config.Defence.prudence;
	
	this.defenceManager = defManager;
	
	if (!entities.length)
	{
		warn ("An army was created with no enemy units");
		return zgsgf;	// should error out to give the "stacktrace".
	}
	
	this.ID = m.playerGlobals[PlayerID].uniqueIDDefManagerArmy++;
	
	this.position = [0,0];
	this.positionLastUpdate = gameState.getTimeElapsed();
	
	this.state = 2;	// 1 or 2, 1 for "watch", 2 for "attack"
								// representation of how much attention we're giving to this army
								// If the state is 1 (watch), and this gets too high, we'll start attacking it (-> state = 2)
	
	this.watchLevel = 0;
	this.timeOfFoundation = gameState.getTimeElapsed();
	
	// target particularly units with this Class
	this.priorityTarget = "";
	// target particularly those units (takes precedence over the Class one).
	this.priorityTargets = [];
	
	// Some caching
	// A list of our defenders (and allies?) that were tasked with attacking a particular unit
	// This doesn't mean that they actually are since they could move on to something else on their own.
	this.assignedAgainst = {};
	// a list of who we assigned ourx defenders too.
	this.assignedTo = {};
	
	this.entities = [];
	this.totalStrength = 0;
	this.strength = {
		"spear" : 0,
		"sword" : 0,
		"ranged" : 0,
		"meleeCav" : 0,
		"rangedCav" : 0,
		"elephant" : 0,
		"rangedSiege" : 0,
		"meleeSiege" : 0
	};
	
	this.ownEntities = [];
	this.ownTotalStrenght = 0;
	this.ownStrenght = {
		"spear" : 0,
		"sword" : 0,
		"ranged" : 0,
		"meleeCav" : 0,
		"rangedCav" : 0,
		"elephant" : 0,
		"rangedSiege" : 0,
		"meleeSiege" : 0
	};
	/*
	 // TODO: allies
	 if (alliedEntities === undefined)
	 return true;
	 
	 this.ownEntities = ownEntities;
	 */
	
	// actually add units
	for (var i in entities)
		this.addEnemy(gameState,entities[i], true);
	for (var i in ownEntities)
		this.addDefender(gameState,ownEntities[i]);
	
	// let's now calculate some sensible default values
	// no sanity check here, shouldn't be needed.
	this.owner = gameState.getEntityById(this.entities[0]).owner();
	
	for (var i in this.entities)
	{
		var ent = gameState.getEntityById(this.entities[i]);
		if (!ent)
		{
			warn("Tried to create an army with unusable units, crashing for stacktrace");
			warn("debug : " + uneval(this.entities) + ", " + uneval(this.entities[i]));
			return sofgkjs;
		}
		this.evaluateStrength(ent);
		var pos = ent.position();
		this.position[0] += pos[0];
		this.position[1] += pos[1];
		ent.setMetadata(PlayerID, "DefManagerArmy", this.ID);
	}
	this.position[0] /= this.entities.length;
	this.position[1] /= this.entities.length;
	
	for (var i in this.ownEntities)
	{
		var ent = gameState.getEntityById(this.ownEntities[i]);
		if (!ent)
		{
			warn("Tried to create an army with unusable units, crashing for stacktrace");
			warn("debug : " + uneval(this.entities) + ", " + uneval(this.entities[i]));
			return sofgkjs;
		}
		this.evaluateStrength(ent, true);
	}
	// TODO: allies

	this.checkDangerosity(gameState);	// might push us to 1.
	this.watchLevel = this.totalStrength * this.watchTSMultiplicator;
	
	return true;
}

m.Army.prototype.recalculatePosition = function(gameState, force)
{
	if (!force && this.positionLastUpdate === gameState.getTimeElapsed())
		return;
	var pos = [0,0];
	for (var i in this.entities)
	{
		var ent = gameState.getEntityById(this.entities[i]);
		if (!ent)	// whaaat?
			continue;
		var epos = ent.position();
		if (epos == undefined)
			continue;
		pos[0] += epos[0];
		pos[1] += epos[1];
	}
	this.position[0] = pos[0]/this.entities.length;
	this.position[1] = pos[1]/this.entities.length;
}

m.Army.prototype.recalculateStrength = function (gameState)
{
	for (var i in this.entities)
		this.evaluateStrength(gameState.getEntityById(this.entities[i]));
	for (var i in this.ownEntities)
		this.evaluateStrength(gameState.getEntityById(this.ownEntities[i], true));
}

m.Army.prototype.evaluateStrength = function (ent, isOwn, remove)
{
	var entStrength = m.getMaxStrength(ent);
	if (remove)
		entStrength *= -1;
	if (isOwn)
		this.ownTotalStrenght += entStrength;
	else
		this.totalStrength += entStrength;
	
	if (ent.hasClass("Infantry"))
	{
		// maybe don't use "else" for multiple attacks later?
		if (ent.hasClass("Spear"))
			this.strength.spear += entStrength;
		else if (ent.hasClass("Sword"))
			this.strength.sword += entStrength;
		else if (ent.hasClass("Ranged"))
			this.strength.ranged += entStrength;
	} else if (ent.hasClass("Elephant"))
		this.strength.elephant += entStrength;
	else if (ent.hasClass("Cavalry"))
	{
		if (ent.hasClass("Ranged"))
			this.strength.rangedCav += entStrength;
		else
			this.strength.meleeCav += entStrength;
	}
	else if (ent.hasClass("Siege"))
	{
		if (ent.hasClass("Ranged"))
			this.strength.rangedSiege += entStrength;
		else
			this.strength.meleeSiege += entStrength;
	}
}

// add an entity to the army
// Will return true if the entity was added and false otherwise.
m.Army.prototype.addEnemy = function (gameState, enemyID, force)
{
	if (this.entities.indexOf(enemyID) !== -1)
		return false;
	var ent = gameState.getEntityById(enemyID);
	if (ent === undefined)
		return false;
	if (ent.position() === undefined)
		return false;
	
	// check distance
	if (!force)
	{
		this.recalculatePosition(gameState);
		if (API3.SquareVectorDistance(ent.position(),this.position) > this.compactSize)
			return false;
	}
	
	this.entities.push(enemyID);
	this.assignedAgainst[enemyID] = [];
	this.recalculatePosition(gameState, true);
	this.evaluateStrength(ent);
	ent.setMetadata(PlayerID, "DefManagerArmy", this.ID);

	return true;
}

// returns true if the entity was removed and false otherwise.
// TODO: when there is a technology update, we should probably recompute the strengths, or weird stuffs might happen.
m.Army.prototype.removeEnemy = function (gameState, enemyID, enemyEntity)
{
	var idx = this.entities.indexOf(enemyID);
	if (idx === -1)
		return false;
	var ent = enemyEntity === undefined ? gameState.getEntityById(enemyID) : enemyEntity;
	if (ent === undefined)
		return false;
	
	this.entities.splice(idx, 1);
	this.position[0] = (this.position[0] * (this.entities.length+1) - ent.position()[0])/this.entities.length;
	this.position[0] = (this.position[1] * (this.entities.length+1) - ent.position()[1])/this.entities.length;
	this.evaluateStrength(ent, false, true);
	ent.setMetadata(PlayerID, "DefManagerArmy", undefined);
	
	for (var i in this.assignedAgainst[enemyID])
		this.assignDefender(gameState,this.assignedAgainst[enemyID][i]);

	delete this.assignedAgainst[enemyID];

	// TODO: reassign defenders assigned to it.
	
	return true;
}

m.Army.prototype.addDefender = function (gameState, defenderID)
{
	if (this.ownEntities.indexOf(defenderID) !== -1)
		return false;
	var ent = gameState.getEntityById(defenderID);
	if (ent === undefined)
		return false;
	if (ent.position() === undefined)
		return false;
		
	this.ownEntities.push(defenderID);
	this.evaluateStrength(ent, true);
	ent.setMetadata(PlayerID, "DefManagerArmy", this.ID);
	this.assignedTo[defenderID] = 0;
	
	var formerRole = ent.getMetadata(PlayerID, "role");
	var formerSubRole = ent.getMetadata(PlayerID, "subrole");

	if (formerRole !== undefined)
		ent.setMetadata(PlayerID,"formerRole", formerRole);
	if (formerSubRole !== undefined)
		ent.setMetadata(PlayerID,"formerSubRole", formerSubRole);
	ent.setMetadata(PlayerID, "role", "defense");
	ent.setMetadata(PlayerID, "subrole", "defending");
	ent.stopMoving();

	this.assignDefender(gameState,defenderID);
	
	return true;
}

m.Army.prototype.removeDefender = function (gameState, defenderID, defenderObj)
{
	var idx = this.ownEntities.indexOf(defenderID);
	if (idx === -1)
		return false;
	var ent = defenderObj === undefined ? gameState.getEntityById(defenderID) : defenderObj;
	if (ent === undefined)
		return false;
	
	this.ownEntities.splice(idx, 1);
	this.evaluateStrength(ent, true, true);
	if (this.assignedTo[defenderID])
	{
		var temp = this.assignedAgainst[this.assignedTo[defenderID]];
		if (temp)
			temp.splice(temp.indexOf(defenderID), 1);
	}
	delete this.assignedTo[defenderID];
	
	if (defenderObj !== undefined || ent.owner() !== PlayerID)
		return false;	// assume this means dead.
	
	ent.setMetadata(PlayerID, "DefManagerArmy", undefined);
	
	var formerRole = ent.getMetadata(PlayerID, "formerRole");
	var formerSubRole = ent.getMetadata(PlayerID, "formerSubRole");
	if (formerRole !== undefined)
		ent.setMetadata(PlayerID,"role", formerRole);
	if (formerSubRole !== undefined)
		ent.setMetadata(PlayerID,"subrole", formerSubRole);
	
	// tell the defence manager this unit has been released for reusage.
	this.defenceManager.releasedDefenders.push(defenderID);
	return true;
}

// this one is "undefined entity" proof because it's called at odd times.
m.Army.prototype.assignDefender = function (gameState, entID)
{
	// we'll assume this defender is ours already.
	// we'll also override any previous assignment
	
	var ent = gameState.getEntityById(entID);

	if (!ent)
		return false;
	
	// TODO: improve the logic in there.
	var maxVal = 1000000;
	var maxEnt = -1;
		
	for (var i in this.entities)
	{
		var id = this.entities[i];
		var eEnt = gameState.getEntityById(id);
		if (!eEnt)
			continue;
		// antigarrisonCheck
		if (eEnt.position() === undefined)
		{
			this.removeEnemy(gameState, id);
			return false;	// need to wait one turn or it'll act weird
		}
		if (maxVal > this.assignedAgainst[id].length)
		{
			maxVal = this.assignedAgainst[id].length;
			maxEnt = id;
		}
	}
	if (maxEnt === -1)
		return false;
	
	// let's attack id
	this.assignedAgainst[maxEnt].push(entID);
	this.assignedTo[entID] = maxEnt;
	
	ent.attack(id);
	
	return true;
}

m.Army.prototype.clear = function (gameState, events)
{
	// release all units by deleting metadata about them, defenders are released
	for (var i in this.entities)
		gameState.getEntityById(this.entities[i]).setMetadata(PlayerID, "DefManagerArmy", undefined);
	for (var i in this.ownEntities)
	{
		var ent = gameState.getEntityById(this.ownEntities[i]);
		ent.setMetadata(PlayerID, "DefManagerArmy", undefined);
		
		var formerRole = ent.getMetadata(PlayerID, "formerRole");
		var formerSubRole = ent.getMetadata(PlayerID, "formerSubRole");
		
		if (formerRole !== undefined)
			ent.setMetadata(PlayerID,"role", formerRole);
		if (formerSubRole !== undefined)
			ent.setMetadata(PlayerID,"subrole", formerSubRole);

		if (ent.owner() === PlayerID)
			this.defenceManager.releasedDefenders.push(this.ownEntities[i]);
	}
}

m.Army.prototype.merge = function (gameState, otherArmy)
{
	if (this.owner !== otherArmy.owner)
	{
		warn("Tried to merge armies of different players, crashing for stacktrace");
		return sofgkjs;
	}
	// basically the other army will get destroyed.
	for (var i in otherArmy.assignedAgainst)
		this.assignedAgainst[i] = otherArmy.assignedAgainst[i];
	for (var i in otherArmy.assignedTo)
		this.assignedTo[i] = otherArmy.assignedTo[i];

	if (this.priorityTarget  === "" && otherArmy.priorityTarget !== "")
		this.priorityTarget = otherArmy.priorityTarget;
	this.priorityTargets = this.priorityTargets.concat(otherArmy.priorityTargets);

	this.watchLevel += otherArmy.watchLevel;
	
	// I'm not using addEnemy because it'd do needless checks and recalculations
	for (var i in otherArmy.entities)
	{
		var ent = gameState.getEntityById(otherArmy.entities[i]);
		this.entities.push(otherArmy.entities[i]);
		this.evaluateStrength(ent);
		ent.setMetadata(PlayerID, "DefManagerArmy", this.ID);
	}
	this.recalculatePosition(gameState, true);
	
	// TODO: reassign those ?
	for (var i in otherArmy.ownEntities)
	{
		var ent = gameState.getEntityById(otherArmy.ownEntities[i]);
		this.ownEntities.push(otherArmy.ownEntities[i]);
		this.evaluateStrength(ent, true);
		ent.setMetadata(PlayerID, "DefManagerArmy", this.ID);
	}
	return true;
}

// TODO: this should return cleverer results ("needs anti-elephant"…)
m.Army.prototype.needsDefenders = function (gameState, events)
{
	// some preliminary checks because we don't update for tech
	if (this.totalStrength < 0 || this.ownTotalStrenght < 0)
		this.recalculateStrength(gameState);
	
	if (this.totalStrength * this.defenceRatio < this.ownTotalStrenght)
		return false;
	return this.totalStrength * this.defenceRatio - this.ownTotalStrenght;
}

m.Army.prototype.getState = function (gameState)
{
	if (this.entities.length === 0)
		return 0;
	if (this.state === 2)
		return 2;
	if (this.watchLevel > 0)
		return 1;
	return 0;
}

// check if we should remain at state 2 or drift away
m.Army.prototype.checkDangerosity = function (gameState)
{
	this.territoryMap = m.createTerritoryMap(gameState);
	// right now we'll check if our position is "enemy" or not.
	if (this.territoryMap.getOwner(this.position) !== PlayerID)
		this.state = 1;
	else if (this.state === 1)
		this.state = 2;
}

// TODO: when there is a technology update, we should probably recompute the strengths, or weird stuffs might happen.
m.Army.prototype.checkEvents = function (gameState, events)
{
	var destroyEvents = events["Destroy"];
	var convEvents = events["OwnershipChanged"];
	
	for (var i in destroyEvents)
	{
		var msg = destroyEvents[i];
		if (msg.entityObj && msg.entityObj.owner() === this.owner)
			this.removeEnemy(gameState, msg.entity, msg.entityObj);
		else if (msg.entityObj)
			this.removeDefender(gameState, msg.entity, msg.entityObj);
	}
	for (var i in convEvents)
	{
		var msg = convEvents[i];

		if (msg.from === this.owner)
		{
			// we have converted an enemy, let's assign it as a defender
			if (this.removeEnemy(gameState, msg.entity) && msg.to === PlayerID)
				this.addDefender(gameState, msg.entity);
		} else if (msg.from === PlayerID)
			this.removeDefender(gameState, msg.entity);	// TODO: add allies
	}
}

m.Army.prototype.update = function (gameState)
{
	var breakaways = [];
	// TODO: assign unassigned defenders, cleanup of a few things.
	// perhaps occasional strength recomputation
	
	if (gameState.getTimeElapsed() - this.positionLastUpdate > 5000)
	{
		this.recalculatePosition(gameState);
		this.positionLastUpdate = gameState.getTimeElapsed();
	
		// Check for breakaways.
		for (var i in this.entities)
		{
			var id = this.entities[i];
			var ent = gameState.getEntityById(id);
			if (!ent.position)	// shouldn't be able to happen but apparently does.
				continue;
			if (API3.SquareVectorDistance(ent.position(), this.position) > this.breakawaySize)
			{
				breakaways.push(id);
				this.removeEnemy(gameState, id);
			}
		}
		
		for (var i in this.ownEntities)
		{
			var ent = gameState.getEntityById(this.ownEntities[i]);
			var eee = gameState.getEntityById(this.assignedTo[this.ownEntities[i]]);
			if (ent.isIdle())
				ent.attack(this.assignedTo[this.ownEntities[i]]);
		}
	}
	
	this.checkDangerosity(gameState);
	
	var normalWatch = this.totalStrength * this.watchTSMultiplicator;
	if (this.state === 2)
		this.watchLevel = normalWatch;
	else if (this.watchLevel > normalWatch)
		this.watchLevel = normalWatch;
	else
		this.watchLevel -= this.watchDecrement;
	
	// TODO: deal with watchLevel?
	
	return breakaways;
}

m.Army.prototype.debug = function (gameState)
{
	m.debug ("Army " + this.ID)
	m.debug ("state " + this.state);
	m.debug ("WatchLevel " + this.watchLevel);
	m.debug ("Entities " + this.entities.length);
	m.debug ("Strength " + this.totalStrength);
	for (var id in this.assignedAgainst)
		m.debug ("Assigned " + uneval(this.assignedAgainst[id]) + " against " + id);
	//for each (ent in this.entities)
	//	debug (gameState.getEntityById(ent)._templateName + ", ID " + ent);
	//debug ("Defenders " + this.ownEntities.length);
	//for each (ent in this.ownEntities)
	//{
	//	if (gameState.getEntityById(ent) !== undefined)
	//		debug (gameState.getEntityById(ent)._templateName + ", ID " + ent);
	//	else
	//		debug("ent "  + ent);
	//}
	//debug ("Strength " + this.ownTotalStrenght);
	m.debug ("");
	
}
return m;
}(AEGIS);
