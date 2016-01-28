var PETRA = function(m)
{

/**
 * Manage the garrisonHolders
 * When a unit is ordered to garrison, it must be done through this.garrison() function so that
 * an object in this.holders is created. This object contains an array with the entities
 * in the process of being garrisoned. To have all garrisoned units, we must add those in holder.garrisoned().
 * Futhermore garrison units have a metadata garrisonType describing its reason (protection, transport, ...)
 */

m.GarrisonManager = function()
{
	this.holders = new Map();
	this.decayingStructures = new Map();
};

m.GarrisonManager.prototype.update = function(gameState, events)
{
	for (let [id, list] of this.holders.entries())
	{
		let holder = gameState.getEntityById(id);
		if (!holder || !gameState.isPlayerAlly(holder.owner()))
		{
			// this holder was certainly destroyed or captured. Let's remove it
			for (let entId of list)
			{
				let ent = gameState.getEntityById(entId);
				if (ent && ent.getMetadata(PlayerID, "garrisonHolder") == id)
				{
					this.leaveGarrison(ent);
					ent.stopMoving();
				}
			}
			this.holders.delete(id);
			continue;
		}

		// Update the list of garrisoned units
		for (let j = 0; j < list.length; ++j)
		{
			for (let evt of events.EntityRenamed)
				if (evt.entity === list[j])
					list[j] = evt.newentity;

			let ent = gameState.getEntityById(list[j]);
			if (!ent)	// unit must have been killed while garrisoning
				list.splice(j--, 1);    
			else if (holder.garrisoned().indexOf(list[j]) !== -1)   // unit is garrisoned
			{
				this.leaveGarrison(ent);
				list.splice(j--, 1);
			}
			else
			{
				let ok = false;
				for (let order of ent.unitAIOrderData())
				{
					if (!order.target || order.target != id)
						continue;
					ok = true;
					break;
				}
				if (ok)
					continue;
				if (ent.getMetadata(PlayerID, "garrisonHolder") == id)
				{
					// The garrison order must have failed
					this.leaveGarrison(ent);
					list.splice(j--, 1);
				}
				else
				{
					if (gameState.ai.Config.debug > 0)
					{
						API3.warn("Petra garrison error: unit " + ent.id() + " (" + ent.genericName() +
							  ") is expected to garrison in " + id + " (" + holder.genericName() +
							  "), but has no such garrison order " + uneval(ent.unitAIOrderData()));
						m.dumpEntity(ent);
					}
					list.splice(j--, 1);
				}
			}

		}

		if (!holder.position())     // could happen with siege unit inside a ship
			continue;

		if (gameState.ai.elapsedTime - holder.getMetadata(PlayerID, "holderTimeUpdate") > 3)
		{
			let range = holder.attackRange("Ranged") ? holder.attackRange("Ranged").max : 80;
			let enemiesAround = false;
			for (let ent of gameState.getEnemyEntities().values())
			{
				if (!ent.position())
					continue;
				if (ent.owner() === 0 && (!ent.unitAIState() || ent.unitAIState().split(".")[1] !== "COMBAT"))
					continue;
				let dist = API3.SquareVectorDistance(ent.position(), holder.position());
				if (dist > range*range)
					continue;
				enemiesAround = true;
				break;
			}

			for (let entId of holder.garrisoned())
			{
				let ent = gameState.getEntityById(entId);
				if (ent.owner() === PlayerID && !this.keepGarrisoned(ent, holder, enemiesAround))
					holder.unload(entId);
			}
			for (let j = 0; j < list.length; ++j)
			{
				let ent = gameState.getEntityById(list[j]);
				if (this.keepGarrisoned(ent, holder, enemiesAround))
					continue;
				if (ent.getMetadata(PlayerID, "garrisonHolder") == id)
				{
					this.leaveGarrison(ent);
					ent.stopMoving();
				}
				list.splice(j--, 1);
			}
			if (this.numberOfGarrisonedUnits(holder) === 0)
				this.holders.delete(id);
			else
				holder.setMetadata(PlayerID, "holderTimeUpdate", gameState.ai.elapsedTime);
		}
	}

	// Warning new garrison orders (as in the following lines) should be done after having updated the holders 
	// (or TODO we should add a test that the garrison order is from a previous turn when updating)
	for (let [id, gmin] of this.decayingStructures.entries())
	{
		let ent = gameState.getEntityById(id);
		if (!ent || ent.owner() !== PlayerID)
			this.decayingStructures.delete(id);		
		else if (this.numberOfGarrisonedUnits(ent) < gmin)
			gameState.ai.HQ.defenseManager.garrisonRangedUnitsInside(gameState, ent, {"min": gmin, "type": "decay"});
	}
};

// TODO should add the units garrisoned inside garrisoned units
m.GarrisonManager.prototype.numberOfGarrisonedUnits = function(holder)
{
	if (!this.holders.has(holder.id()))
		return holder.garrisoned().length;

	return (holder.garrisoned().length + this.holders.get(holder.id()).length);
};

// This is just a pre-garrison state, while the entity walk to the garrison holder
m.GarrisonManager.prototype.garrison = function(gameState, ent, holder, type)
{
	if (this.numberOfGarrisonedUnits(holder) >= holder.garrisonMax())
		return;

	this.registerHolder(gameState, holder);
	this.holders.get(holder.id()).push(ent.id());

	if (gameState.ai.Config.debug > 2)
	{
		warn("garrison unit " + ent.genericName() + " in " + holder.genericName() + " with type " + type);
		warn(" we try to garrison a unit with plan " + ent.getMetadata(PlayerID, "plan") + " and role " + ent.getMetadata(PlayerID, "role") +
		     " and subrole " +  ent.getMetadata(PlayerID, "subrole") + " and transport " +  ent.getMetadata(PlayerID, "transport"));
	}

	if (ent.getMetadata(PlayerID, "plan") !== undefined)
		ent.setMetadata(PlayerID, "plan", -2);
	else
		ent.setMetadata(PlayerID, "plan", -3);
	ent.setMetadata(PlayerID, "subrole", "garrisoning");
	ent.setMetadata(PlayerID, "garrisonHolder", holder.id());
	ent.setMetadata(PlayerID, "garrisonType", type);
	ent.garrison(holder);
};

// This is the end of the pre-garrison state, either because the entity is really garrisoned
// or because it has changed its order (i.e. because the garrisonHolder was destroyed).
m.GarrisonManager.prototype.leaveGarrison = function(ent)
{
	ent.setMetadata(PlayerID, "subrole", undefined);
	if (ent.getMetadata(PlayerID, "plan") === -2)
		ent.setMetadata(PlayerID, "plan", -1);
	else
		ent.setMetadata(PlayerID, "plan", undefined);
	ent.setMetadata(PlayerID, "garrisonHolder", undefined);
};

m.GarrisonManager.prototype.keepGarrisoned = function(ent, holder, enemiesAround)
{
	switch (ent.getMetadata(PlayerID, "garrisonType"))
	{
	case 'force':           // force the ungarrisoning
		return false;
	case 'trade':		// trader garrisoned in ship
		return true;
	case 'protection':	// hurt unit for healing or infantry for defense
		if (ent.needsHeal() && holder.buffHeal())
			return true;
		if (enemiesAround && (ent.hasClass("Support") || MatchesClassList(holder.getGarrisonArrowClasses(), ent.classes())))
			return true;
		return false;
	case 'decay':
		return this.decayingStructures.has(holder.id());
	default:
		if (ent.getMetadata(PlayerID, "onBoard") === "onBoard")  // transport is not (yet ?) managed by garrisonManager 
			return true;
		warn("unknown type in garrisonManager " + ent.getMetadata(PlayerID, "garrisonType"));
		return true;
	}
};

// Add this holder in the list managed by the garrisonManager
m.GarrisonManager.prototype.registerHolder = function(gameState, holder)
{
	if (this.holders.has(holder.id()))    // already registered
		return;
	this.holders.set(holder.id(), []);
	holder.setMetadata(PlayerID, "holderTimeUpdate", gameState.ai.elapsedTime);
};

// Garrison units in decaying structures to stop their decay
// do it only for structures useful for defense, except if we are expanding (justCaptured=true)
// in which case we also do it for structures useful for unit trainings (TODO only Barracks are done)
m.GarrisonManager.prototype.addDecayingStructure = function(gameState, entId, justCaptured)
{
	if (this.decayingStructures.has(entId))
		return true;
	let ent = gameState.getEntityById(entId);
	if (!ent || (!(ent.hasClass("Barracks") && justCaptured) && !ent.hasDefensiveFire()))
		return false;
	if (!ent.territoryDecayRate() || !ent.garrisonRegenRate())
		return false;
	let gmin = Math.ceil((ent.territoryDecayRate() - ent.defaultRegenRate()) / ent.garrisonRegenRate());
	this.decayingStructures.set(entId, gmin);
	return true;
};

m.GarrisonManager.prototype.removeDecayingStructure = function(entId)
{
	if (!this.decayingStructures.has(entId))
		return;
	this.decayingStructures.delete(entId);
};

m.GarrisonManager.prototype.Serialize = function()
{
	return { "holders": this.holders, "decayingStructures": this.decayingStructures };
};

m.GarrisonManager.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
