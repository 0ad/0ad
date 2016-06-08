var PETRA = function(m)
{

/** returns some sort of DPS * health factor. If you specify a class, it'll use the modifiers against that class too. */
m.getMaxStrength = function(ent, againstClass)
{
	let strength = 0;
	let attackTypes = ent.attackTypes();
	if (!attackTypes)
		return strength;

	for (let type of attackTypes)
	{
		if (type == "Slaughter" || type == "Charge")
			continue;

		let attackStrength = ent.attackStrengths(type);
		for (let str in attackStrength)
		{
			let val = parseFloat(attackStrength[str]);
			if (againstClass)
				val *= ent.getMultiplierAgainst(type, againstClass);
			switch (str)
			{
			case "crush":
				strength += val * 0.085 / 3;
				break;
			case "hack":
				strength += val * 0.075 / 3;
				break;
			case "pierce":
				strength += val * 0.065 / 3;
				break;
			default:
				API3.warn("Petra: " + str + " unknown attackStrength in getMaxStrength");
			}
		}

		let attackRange = ent.attackRange(type);
		if (attackRange)
			strength += attackRange.max * 0.0125;

		let attackTimes = ent.attackTimes(type);
		for (let str in attackTimes)
		{
			let val = parseFloat(attackTimes[str]);
			switch (str)
			{
			case "repeat":
				strength += val / 100000;
				break;
			case "prepare":
				strength -= val / 100000;
				break;
			default:
				API3.warn("Petra: " + str + " unknown attackTimes in getMaxStrength");
			}
		}
	}

	let armourStrength = ent.armourStrengths();
	for (let str in armourStrength)
	{
		let val = parseFloat(armourStrength[str]);
		switch (str)
		{
		case "crush":
			strength += val * 0.085 / 3;
			break;
		case "hack":
			strength += val * 0.075 / 3;
			break;
		case "pierce":
			strength += val * 0.065 / 3;
			break;
		default:
			API3.warn("Petra: " + str + " unknown armourStrength in getMaxStrength");
		}
	}

	return strength * ent.maxHitpoints() / 100.0;
};

/** Decide if we should try to capture or destroy */
m.allowCapture = function(ent, target)
{
	return !target.hasClass("Siege") || !ent.hasClass("Melee") ||
		!target.isGarrisonHolder() || !target.garrisoned().length;
};

/** Makes the worker deposit the currently carried resources at the closest accessible dropsite */
m.returnResources = function(gameState, ent)
{
	if (!ent.resourceCarrying() || !ent.resourceCarrying().length || !ent.position())
		return false;

	let resource = ent.resourceCarrying()[0].type;

	let closestDropsite;
	let distmin = Math.min();
	let access = gameState.ai.accessibility.getAccessValue(ent.position());
	gameState.getOwnDropsites(resource).forEach(function(dropsite) {
		if (!dropsite.position() || dropsite.getMetadata(PlayerID, "access") !== access)
			return;
		let dist = API3.SquareVectorDistance(ent.position(), dropsite.position());
		if (dist > distmin)
			return;
		distmin = dist;
		closestDropsite = dropsite;
	});

	if (!closestDropsite)
		return false;
	ent.returnResources(closestDropsite);
	return true;
};

/** is supply full taking into account gatherers affected during this turn */
m.IsSupplyFull = function(gameState, ent)
{
	if (ent.isFull() === true)
	    return true;
	let turnCache = gameState.ai.HQ.turnCache;
	let count = ent.resourceSupplyNumGatherers();
	if (turnCache.resourceGatherer && turnCache.resourceGatherer[ent.id()])
		count += turnCache.resourceGatherer[ent.id()];
	if (count >= ent.maxGatherers())
		return true;
	return false;
};

/**
 * get the best base (in terms of distance and accessIndex) for an entity
 */
m.getBestBase = function(gameState, ent)
{
	let pos = ent.position();
	if (!pos)
	{
		let holder = m.getHolder(gameState, ent);
		if (!holder || !holder.position())
		{
			API3.warn("Petra error: entity without position, but not garrisoned");
			m.dumpEntity(ent);
			return gameState.ai.HQ.baseManagers[0];
		}
		pos = holder.position();
	}
	let distmin = Math.min();
	let bestbase;
	let accessIndex = gameState.ai.accessibility.getAccessValue(pos);
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (!base.anchor)
			continue;
		let dist = API3.SquareVectorDistance(base.anchor.position(), pos);
		if (base.accessIndex !== accessIndex)
			dist += 100000000;
		if (dist > distmin)
			continue;
		distmin = dist;
		bestbase = base;
	}
	if (!bestbase)
		bestbase = gameState.ai.HQ.baseManagers[0];
	return bestbase;
};

m.getHolder = function(gameState, ent)
{
	for (let holder of gameState.getEntities().values())
	{
		if (holder.isGarrisonHolder() && holder.garrisoned().indexOf(ent.id()) !== -1)
			return holder;
	}
	return undefined;
};

/**
 * return true if it is not worth finishing this building (it would surely decay)
 * TODO implement the other conditions
 */
m.isNotWorthBuilding = function(gameState, ent)
{
	if (gameState.ai.HQ.territoryMap.getOwner(ent.position()) !== PlayerID)
	{
		let buildTerritories = ent.buildTerritories();
		if (buildTerritories && (!buildTerritories.length || (buildTerritories.length === 1 && buildTerritories[0] === "own")))
			return true;
	}
	return false;
};

m.dumpEntity = function(ent)
{
	if (!ent)
		return;
	API3.warn(" >>> id " + ent.id() + " name " + ent.genericName() + " pos " + ent.position() +
		  " state " + ent.unitAIState());
	API3.warn(" base " + ent.getMetadata(PlayerID, "base") + " >>> role " + ent.getMetadata(PlayerID, "role") +
		  " subrole " + ent.getMetadata(PlayerID, "subrole"));
	API3.warn("owner " + ent.owner() + " health " + ent.hitpoints() + " healthMax " + ent.maxHitpoints());
	API3.warn(" garrisoning " + ent.getMetadata(PlayerID, "garrisoning") + " garrisonHolder " + ent.getMetadata(PlayerID, "garrisonHolder") +
		  " plan " + ent.getMetadata(PlayerID, "plan")	+ " transport " + ent.getMetadata(PlayerID, "transport") +
		  " gather-type " + ent.getMetadata(PlayerID, "gather-type") + " target-foundation " + ent.getMetadata(PlayerID, "target-foundation") +
		  " PartOfArmy " + ent.getMetadata(PlayerID, "PartOfArmy"));
};

return m;
}(PETRA);
