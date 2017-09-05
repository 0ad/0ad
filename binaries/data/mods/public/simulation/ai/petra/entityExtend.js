var PETRA = function(m)
{

/** returns true if this unit should be considered as a siege unit */
m.isSiegeUnit = function(ent)
{
	return ent.hasClass("Siege") || ent.hasClass("Elephant") && ent.hasClass("Champion");
};

/** returns some sort of DPS * health factor. If you specify a class, it'll use the modifiers against that class too. */
m.getMaxStrength = function(ent, againstClass)
{
	let strength = 0;
	let attackTypes = ent.attackTypes();
	if (!attackTypes)
		return strength;

	for (let type of attackTypes)
	{
		if (type == "Slaughter")
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

/** Get access and cache it in metadata if not already done */
m.getLandAccess = function(gameState, ent)
{
	let access = ent.getMetadata(PlayerID, "access");
	if (!access)
	{
		access = gameState.ai.accessibility.getAccessValue(ent.position());
		ent.setMetadata(PlayerID, "access", access);
	}
	return access;
};

m.getSeaAccess = function(gameState, ent, warning = true)
{
	let sea = ent.getMetadata(PlayerID, "sea");
	if (!sea)
	{
		sea = gameState.ai.accessibility.getAccessValue(ent.position(), true);
		if (sea < 2)	// pre-positioned docks are sometimes not well positionned
		{
			let entPos = ent.position();
			let radius = ent.footprintRadius();
			for (let i = 0; i < 16; ++i)
			{
				let pos = [ entPos[0] + radius*Math.cos(i*Math.PI/8),
				            entPos[1] + radius*Math.sin(i*Math.PI/8) ];
				sea = gameState.ai.accessibility.getAccessValue(pos, true);
				if (sea >= 2)
					break;
			}
		}
		if (warning && sea < 2)
			API3.warn("ERROR in Petra getSeaAccess because of position with sea index " + sea);
		ent.setMetadata(PlayerID, "sea", sea);
	}
	return sea;
};

/** Decide if we should try to capture (returns true) or destroy (return false) */
m.allowCapture = function(gameState, ent, target)
{
	if (!target.isCapturable() || !ent.canCapture(target))
		return false;
	// always try to recapture cp from an allied, except if it's decaying
	if (gameState.isPlayerAlly(target.owner()))
		return !target.decaying();

	let antiCapture = target.defaultRegenRate();
	if (target.isGarrisonHolder() && target.garrisoned())
		antiCapture += target.garrisonRegenRate() * target.garrisoned().length;
	if (target.decaying())
		antiCapture -= target.territoryDecayRate();

	let capture;
	let capturableTargets = gameState.ai.HQ.capturableTargets;
	if (!capturableTargets.has(target.id()))
	{
		capture = ent.captureStrength() * m.getAttackBonus(ent, target, "Capture");
		capturableTargets.set(target.id(), { "strength": capture, "ents": new Set([ent.id()]) });
	}
	else
	{
		let capturable = capturableTargets.get(target.id());
		if (!capturable.ents.has(ent.id()))
		{
			capturable.strength += ent.captureStrength() * m.getAttackBonus(ent, target, "Capture");
			capturable.ents.add(ent.id());
		}
		capture = capturable.strength;
	}
	capture *= 1 / ( 0.1 + 0.9*target.healthLevel());
	let sumCapturePoints = target.capturePoints().reduce((a, b) => a + b);
	if (target.hasDefensiveFire() && target.isGarrisonHolder() && target.garrisoned())
		return capture > antiCapture + sumCapturePoints/50;
	return capture > antiCapture + sumCapturePoints/80;
};

m.getAttackBonus = function(ent, target, type)
{
	let attackBonus = 1;
	if (!ent.get("Attack/" + type) || !ent.get("Attack/" + type + "/Bonuses"))
		return attackBonus;
	let bonuses = ent.get("Attack/" + type + "/Bonuses");
	for (let key in bonuses)
	{
		let bonus = bonuses[key];
		if (bonus.Civ && bonus.Civ !== target.civ())
			continue;
		if (bonus.Classes && bonus.Classes.split(/\s+/).some(cls => !target.hasClass(cls)))
			continue;
		attackBonus *= bonus.Multiplier;
	}
	return attackBonus;
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
	let dropsiteCollection = gameState.playerData.hasSharedDropsites ? gameState.getAnyDropsites(resource) : gameState.getOwnDropsites(resource);
	for (let dropsite of dropsiteCollection.values())
	{
		if (!dropsite.position())
			continue;
		let owner = dropsite.owner();
		// owner !== PlayerID can only happen when hasSharedDropsites === true, so no need to test it again
		if (owner !== PlayerID && (!dropsite.isSharedDropsite() || !gameState.isPlayerMutualAlly(owner)))
			continue;
		let dropsiteAccess = dropsite.getMetadata(PlayerID, "access");
		if (!dropsiteAccess)
		{
			dropsiteAccess = gameState.ai.accessibility.getAccessValue(dropsite.position());
			dropsite.setMetadata(PlayerID, "access", dropsiteAccess);
		}
		if (dropsiteAccess !== access)
			continue;
		let dist = API3.SquareVectorDistance(ent.position(), dropsite.position());
		if (dist > distmin)
			continue;
		distmin = dist;
		closestDropsite = dropsite;
	}

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
m.getBestBase = function(gameState, ent, onlyConstructedBase = false)
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
		if (!base.anchor || onlyConstructedBase && base.anchor.foundationProgress() !== undefined)
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
		if (buildTerritories && (!buildTerritories.length || buildTerritories.length === 1 && buildTerritories[0] === "own"))
			return true;
	}
	return false;
};

/**
 * Check if the straight line between the two positions crosses an enemy territory
 */
m.isLineInsideEnemyTerritory = function(gameState, pos1, pos2, step=70)
{
	let n = Math.floor(Math.sqrt(API3.SquareVectorDistance(pos1, pos2))/step) + 1;
	let stepx = (pos2[0] - pos1[0]) / n;
	let stepy = (pos2[1] - pos1[1]) / n;
	for (let i = 1; i < n; ++i)
	{
		let pos = [pos1[0]+i*stepx, pos1[1]+i*stepy];
		let owner = gameState.ai.HQ.territoryMap.getOwner(pos);
		if (owner && gameState.isPlayerEnemy(owner))
			return true;
	}
	return false;
};

m.gatherTreasure = function(gameState, ent, water = false)
{
	if (!gameState.ai.HQ.treasures.hasEntities())
		return false;
	if (!ent || !ent.position())
		return false;
	let rates = ent.resourceGatherRates();
	if (!rates || !rates.treasure || rates.treasure <= 0)
		return false;
	let treasureFound;
	let distmin = Math.min();
	let access = gameState.ai.accessibility.getAccessValue(ent.position(), water);
	for (let treasure of gameState.ai.HQ.treasures.values())
	{
		if (m.IsSupplyFull(gameState, treasure))
			continue;
		// let some time for the previous gatherer to reach the treasure before trying again
		let lastGathered = treasure.getMetadata(PlayerID, "lastGathered");
		if (lastGathered && gameState.ai.elapsedTime - lastGathered < 20)
			continue;
		if (!water && access !== m.getLandAccess(gameState, treasure))
			continue;
		if (water && access !== m.getSeaAccess(gameState, treasure, false))
			continue;
		let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(treasure.position());
		if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))
			continue;
		let dist = API3.SquareVectorDistance(ent.position(), treasure.position());
		if (dist > 120000 || territoryOwner !== PlayerID && dist > 14000) // AI has no LOS, so restrict it a bit
			continue;
		if (dist > distmin)
			continue;
		distmin = dist;
		treasureFound = treasure;
	}
	if (!treasureFound)
		return false;
	treasureFound.setMetadata(PlayerID, "lastGathered", gameState.ai.elapsedTime);
	ent.gather(treasureFound);
	gameState.ai.HQ.AddTCGatherer(treasureFound.id());
	ent.setMetadata(PlayerID, "supply", treasureFound.id());
	return true;
};

m.dumpEntity = function(ent)
{
	if (!ent)
		return;
	API3.warn(" >>> id " + ent.id() + " name " + ent.genericName() + " pos " + ent.position() +
		  " state " + ent.unitAIState());
	API3.warn(" base " + ent.getMetadata(PlayerID, "base") + " >>> role " + ent.getMetadata(PlayerID, "role") +
		  " subrole " + ent.getMetadata(PlayerID, "subrole"));
	API3.warn("owner " + ent.owner() + " health " + ent.hitpoints() + " healthMax " + ent.maxHitpoints() +
	          " foundationProgress " + ent.foundationProgress());
	API3.warn(" garrisoning " + ent.getMetadata(PlayerID, "garrisoning") + " garrisonHolder " + ent.getMetadata(PlayerID, "garrisonHolder") +
		  " plan " + ent.getMetadata(PlayerID, "plan")	+ " transport " + ent.getMetadata(PlayerID, "transport") +
		  " gather-type " + ent.getMetadata(PlayerID, "gather-type") + " target-foundation " + ent.getMetadata(PlayerID, "target-foundation") +
		  " PartOfArmy " + ent.getMetadata(PlayerID, "PartOfArmy"));
};

return m;
}(PETRA);
