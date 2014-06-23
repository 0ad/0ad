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
	this.holders = {};
};

m.GarrisonManager.prototype.update = function(gameState, queues)
{
	for (var id in this.holders)
	{
		if (this.holders[id] === undefined)
			continue;

		var holder = gameState.getEntityById(id);
		if (!holder)    // this holder was certainly destroyed. Let's remove it
		{
			for (var entId of this.holders[id])
			{
				var ent = gameState.getEntityById(entId);
				if (ent && ent.getMetadata(PlayerID, "garrisonHolder") === id)
					this.leaveGarrison(ent);
			}
			this.holders[id] = undefined;
			continue;
		}

		var list = this.holders[id];
		// Update the list of garrisoned units
		for (var j = 0; j < list.length; ++j)
		{
			var ent = gameState.getEntityById(list[j]);
			if (!ent)	// unit must have been killed while garrisoning
				list.splice(j--, 1);    
			else if (holder._entity.garrisoned.indexOf(list[j]) !== -1)   // unit is garrisoned
			{
				this.leaveGarrison(ent);
				list.splice(j--, 1);
			}
		}

		if (!holder.position())     // could happen with siege unit inside a ship
			continue;

		if (gameState.ai.playedTurn - holder.getMetadata(PlayerID, "holderUpdate") > 5)
		{
			if (holder.attackRange("Ranged"))
				var range = holder.attackRange("Ranged").max;
			else
				var range = 80;
			var enemiesAround = gameState.getEnemyEntities().toEntityArray().some(function(ent) {
				if (!ent.position() || ent.owner() === 0)
					return false;
				var dist = API3.SquareVectorDistance(ent.position(), holder.position());
				if (dist < range*range)
					return true;
				return false;
			});

			var healer = holder.buffHeal();

			for (var entId of holder._entity.garrisoned)
			{
				var ent = gameState.getEntityById(entId);
				if (!this.keepGarrisoned(ent, holder, enemiesAround))
					holder.unload(entId);
			}
			for (var j = 0; j < list.length; ++j)
			{
				var ent = gameState.getEntityById(list[j]);
				if (this.keepGarrisoned(ent, holder, enemiesAround))
					continue;
				if (ent.getMetadata(PlayerID, "garrisonHolder") === id)
					this.leaveGarrison(ent);
				list.splice(j--, 1);
			}
			if (this.numberOfGarrisonedUnits(holder) === 0)
				this.holders[id] = undefined;
			else
				holder.setMetadata(PlayerID, "holderUpdate", gameState.ai.playedTurn);
		}
	}
};

// TODO should add the units garrisoned inside garrisoned units
m.GarrisonManager.prototype.numberOfGarrisonedUnits = function(holder)
{
	if (!this.holders[holder.id()])
		return holder.garrisoned().length;

	return (holder.garrisoned().length + this.holders[holder.id()].length);
};

// This is just a pre-garrison state, while the entity walk to the garrison holder
m.GarrisonManager.prototype.garrison = function(gameState, ent, holder, type)
{
	if (this.numberOfGarrisonedUnits(holder) >= holder.garrisonMax())
		return;

	this.registerHolder(gameState, holder);
	this.holders[holder.id()].push(ent.id());

	if (gameState.ai.HQ.Config.debug > 1)
	{
		warn("garrison unit " + ent.genericName() + " in " + holder.genericName() + " with type " + type);
		warn(" we try to garrison a unit with plan " + ent.getMetadata(PlayerID, "plan") + " and role " + ent.getMetadata(PlayerID, "role")
			+ " and subrole " +  ent.getMetadata(PlayerID, "subrole") + " and transport " +  ent.getMetadata(PlayerID, "transport"));
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
		case 'protection':	// hurt unit for healing or ranged infantry for defense
			var healer = holder.buffHeal();
			if (healer && healer > 0 && ent.isHurt())
				return true;
			if (enemiesAround && (ent.hasClass("Support") || (ent.hasClass("Ranged") && ent.hasClass("Infantry"))))
				return true;
			return false;
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
	if (this.holders[holder.id()])    // already registered
		return;
	this.holders[holder.id()] = [];
	holder.setMetadata(PlayerID, "holderUpdate", gameState.ai.playedTurn);
};

return m;
}(PETRA);
