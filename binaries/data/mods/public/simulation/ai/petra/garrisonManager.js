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
				if (ent && ent.getMetadata(PlayerID, "garrisonHolder") == +id)
				{
					this.leaveGarrison(ent);
					ent.stopMoving();
				}
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
			else if (holder.garrisoned().indexOf(list[j]) !== -1)   // unit is garrisoned
			{
				this.leaveGarrison(ent);
				list.splice(j--, 1);
			}
			else
			{
				var ok = false;
				var orders = ent.unitAIOrderData();
				for (var order of orders)
				{
					if (!order.target || order.target != id)
						continue;
					ok = true;
					break;
				}
				if (ok)
					continue;
				if (ent.getMetadata(PlayerID, "garrisonHolder") == +id)
				{
					// The garrison order must have failed
					this.leaveGarrison(ent);
					list.splice(j--, 1);
				}
				else
				{
					if (gameState.ai.Config.debug > 0)
					{
						API3.warn("Petra garrison error: unit " + ent.id() + " (" + ent.genericName()
							+ ") is expected to garrison in " + id + " (" + holder.genericName()
							+ "), but has no such garrison order " + uneval(orders));
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
			if (holder.attackRange("Ranged"))
				var range = holder.attackRange("Ranged").max;
			else
				var range = 80;
			var enemiesAround = gameState.getEnemyEntities().toEntityArray().some(function(ent) {
				if (!ent.position())
					return false;
				if (ent.owner() === 0 && (!ent.unitAIState() || ent.unitAIState().split(".")[1] !== "COMBAT"))
					return false;
				var dist = API3.SquareVectorDistance(ent.position(), holder.position());
				if (dist < range*range)
					return true;
				return false;
			});

			for (var entId of holder.garrisoned())
			{
				var ent = gameState.getEntityById(entId);
				if (ent.owner() === PlayerID && !this.keepGarrisoned(ent, holder, enemiesAround))
					holder.unload(entId);
			}
			for (var j = 0; j < list.length; ++j)
			{
				var ent = gameState.getEntityById(list[j]);
				if (this.keepGarrisoned(ent, holder, enemiesAround))
					continue;
				if (ent.getMetadata(PlayerID, "garrisonHolder") == +id)
				{
					this.leaveGarrison(ent);
					ent.stopMoving();
				}
				list.splice(j--, 1);
			}
			if (this.numberOfGarrisonedUnits(holder) === 0)
				this.holders[id] = undefined;
			else
				holder.setMetadata(PlayerID, "holderTimeUpdate", gameState.ai.elapsedTime);
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

	if (gameState.ai.Config.debug > 2)
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
		case 'protection':	// hurt unit for healing or infantry for defense
			if (ent.needsHeal() && holder.buffHeal())
				return true;
			if (enemiesAround && (ent.hasClass("Support") || MatchesClassList(holder.getGarrisonArrowClasses(), ent.classes())))
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
	holder.setMetadata(PlayerID, "holderTimeUpdate", gameState.ai.elapsedTime);
};

m.GarrisonManager.prototype.Serialize = function()
{
	return { "holders": this.holders };
};

m.GarrisonManager.prototype.Deserialize = function(data)
{
	this.holders = data.holders;
};

return m;
}(PETRA);
