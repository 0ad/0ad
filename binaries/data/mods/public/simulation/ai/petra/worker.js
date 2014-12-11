var PETRA = function(m)
{

/**
 * This class makes a worker do as instructed by the economy manager
 */

m.Worker = function(baseManager)
{
	this.ent = undefined;
	this.baseManager = baseManager
	this.baseID = baseManager.ID;
};

m.Worker.prototype.update = function(ent, gameState)
{
	if (!ent.position() || ent.getMetadata(PlayerID, "plan") === -2 || ent.getMetadata(PlayerID, "plan") === -3)
		return;

	// If we are waiting for a transport or we are sailing, just wait
	if (ent.getMetadata(PlayerID, "transport") !== undefined)
		return;

	this.ent = ent;
	var subrole = this.ent.getMetadata(PlayerID, "subrole");

	// Check for inaccessible targets (in RMS maps, we quite often have chicken or bushes inside obstruction of other entities).
	if (ent.unitAIState() === "INDIVIDUAL.GATHER.APPROACHING" || ent.unitAIState() === "INDIVIDUAL.COMBAT.APPROACHING")
		if (this.isInaccessibleSupply(gameState) && ((subrole === "hunter" && !this.startHunting(gameState))
				|| (subrole === "gatherer" && !this.startGathering(gameState))))
			this.ent.stopMoving();

	// If we're fighting or hunting, let's not start gathering
	if (ent.unitAIState().split(".")[1] === "COMBAT")
		return;

	// Okay so we have a few tasks.
	// If we're gathering, we'll check that we haven't run idle.
	// And we'll also check that we're gathering a resource we want to gather.

	if (subrole === "gatherer")
	{
		if (this.ent.isIdle())
		{
			// if we aren't storing resources or it's the same type as what we're about to gather,
			// let's just pick a new resource.
			// TODO if we already carry the max we can ->  returnresources
			if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0 ||
					this.ent.resourceCarrying()[0].type === this.ent.getMetadata(PlayerID, "gather-type"))
			{
				this.startGathering(gameState);
			}
			else if (!m.returnResources(this.ent, gameState))     // try to deposit resources
			{
				// no dropsite, abandon old resources and start gathering new ones
				this.startGathering(gameState);
			}
		}
		else if (this.ent.unitAIState().split(".")[1] === "GATHER")
		{
			// we're already gathering. But let's check if there is nothing better
			// in case UnitAI did something bad
			if (this.ent.unitAIOrderData().length)
			{
				var supplyId = this.ent.unitAIOrderData()[0]["target"];
				var supply = gameState.getEntityById(supplyId);
				if (supply && !supply.hasClass("Field") && !supply.hasClass("Animal")
					&& supplyId !== this.ent.getMetadata(PlayerID, "supply"))
				{
					var nbGatherers = supply.resourceSupplyGatherers().length
						+ m.GetTCGatherer(gameState, supplyId);
					if ((nbGatherers > 0 && supply.resourceSupplyAmount()/nbGatherers < 40))
					{
						m.RemoveTCGatherer(gameState, supplyId);
						this.startGathering(gameState);
					}
					else
					{
						var gatherType = this.ent.getMetadata(PlayerID, "gather-type");
						var nearby = this.baseManager.dropsiteSupplies[gatherType]["nearby"];
						var isNearby = nearby.some(function(sup) {
							if (sup.id === supplyId)
								return true;
							return false;
						});
						if (nearby.length === 0 || isNearby)
							this.ent.setMetadata(PlayerID, "supply", supplyId);
						else
						{
							m.RemoveTCGatherer(gameState, supplyId);
							this.startGathering(gameState);
						}
					}
				}
			}
		}
		else if (this.ent.unitAIState() === "INDIVIDUAL.RETURNRESOURCE.APPROACHING"
			&& gameState.ai.playedTurn % 10 === 0)
		{
			// Check from time to time that UnitAI does not send us to an inaccessible dropsite
			var dropsite = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
			if (dropsite && dropsite.position())
			{
				var access = gameState.ai.accessibility.getAccessValue(this.ent.position());
				var goalAccess = dropsite.getMetadata(PlayerID, "access");
				if (!goalAccess || dropsite.hasClass("Elephant"))
				{
					goalAccess = gameState.ai.accessibility.getAccessValue(dropsite.position());
					dropsite.setMetadata(PlayerID, "access", goalAccess);
				}
				if (access !== goalAccess)
					m.returnResources(this.ent, gameState);
			}
		}
	}
	else if (subrole === "builder")
	{	
		if (this.ent.unitAIState().split(".")[1] === "REPAIR")
			return;
		// okay so apparently we aren't working.
		// Unless we've been explicitely told to keep our role, make us idle.
		var target = gameState.getEntityById(this.ent.getMetadata(PlayerID, "target-foundation"));
		if (!target || (target.foundationProgress() === undefined && target.needsRepair() === false))
		{
			this.ent.setMetadata(PlayerID, "subrole", "idle");
			this.ent.setMetadata(PlayerID, "target-foundation", undefined);
			// If worker elephant, move away to avoid being trapped in between constructions
			if (this.ent.hasClass("Elephant"))
				this.moveAway(gameState);
		}
		else
		{
			var access = gameState.ai.accessibility.getAccessValue(this.ent.position());
			var goalAccess = target.getMetadata(PlayerID, "access");
			if (!goalAccess)
			{
				goalAccess = gameState.ai.accessibility.getAccessValue(target.position());
				target.setMetadata(PlayerID, "access", goalAccess);
			}
			if (access === goalAccess)
				this.ent.repair(target);
			else
				gameState.ai.HQ.navalManager.requireTransport(gameState, this.ent, access, goalAccess, target.position());
		}
	}
	else if (subrole === "hunter")
	{
		var lastHuntSearch = this.ent.getMetadata(PlayerID, "lastHuntSearch");
		if (this.ent.isIdle() && (!lastHuntSearch || (gameState.ai.elapsedTime - lastHuntSearch) > 20))
		{
			if (!this.startHunting(gameState))
			{
				// nothing to hunt around. Try another region if any
				var nowhereToHunt = true;
				for (var i in gameState.ai.HQ.baseManagers)
				{
					var base = gameState.ai.HQ.baseManagers[i];
					if (!base.anchor || !base.anchor.position())
						continue;
					var basePos = base.anchor.position();
					if (this.startHunting(gameState, basePos))
					{
						this.ent.setMetadata(PlayerID, "base", base.ID);
						var access = gameState.ai.accessibility.getAccessValue(this.ent.position());
						if (base.accessIndex === access)
							this.ent.move(basePos[0], basePos[1]);
						else
							gameState.ai.HQ.navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, basePos);
						nowhereToHunt = false;
						break;
					}
				}
				if (nowhereToHunt)
					this.ent.setMetadata(PlayerID, "lastHuntSearch", gameState.ai.elapsedTime);
			}
		}
		else	// Perform some sanity checks
		{
			if (this.ent.unitAIState().split(".")[1] === "GATHER"
				|| this.ent.unitAIState().split(".")[1] === "RETURNRESOURCE")
			{
				// we may have drifted towards ennemy territory during the hunt, if yes go home
				var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(this.ent.position());
				if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
					this.startHunting(gameState);
				else if (this.ent.unitAIState() === "INDIVIDUAL.RETURNRESOURCE.APPROACHING")
				{
					// Check that UnitAI does not send us to an inaccessible dropsite
					var dropsite = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
					if (dropsite && dropsite.position())
					{
						var access = gameState.ai.accessibility.getAccessValue(this.ent.position());
						var goalAccess = dropsite.getMetadata(PlayerID, "access");
						if (!goalAccess || dropsite.hasClass("Elephant"))
						{
							goalAccess = gameState.ai.accessibility.getAccessValue(dropsite.position());
							dropsite.setMetadata(PlayerID, "access", goalAccess);
						}
						if (access !== goalAccess)
							m.returnResources(this.ent, gameState);
					}
				}
			}
		}
	}
	else if (subrole === "fisher")
	{
		if (this.ent.isIdle())
			this.startFishing(gameState);
		else	// if we have drifted towards ennemy territory during the fishing, go home
		{
			var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(this.ent.position());
			if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
				this.startFishing(gameState);
		}
	}
};

m.Worker.prototype.startGathering = function(gameState)
{
	var self = this;
	var access = gameState.ai.accessibility.getAccessValue(this.ent.position());

	// First look for possible treasure if any
	var treasureFound = undefined;
	var distmin = Math.min();
	gameState.ai.HQ.treasures.forEach(function (treasure) {
		var treasureAccess = treasure.getMetadata(PlayerID, "access");
		if (!treasureAccess)
		{
			treasureAccess = gameState.ai.accessibility.getAccessValue(treasure.position());
			treasure.setMetadata(PlayerID, "access", treasureAccess);
		}
		if (treasureAccess !== access)
			return;
		var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(treasure.position());
		if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))
			return;
		var lastGathered = treasure.getMetadata(PlayerID, "lastGathered");
		// let some time for the previous gatherer to reach the treasure
		if (lastGathered && gameState.ai.elapsedTime - lastGathered < 20)
			return;
		var dist = API3.SquareVectorDistance(self.ent.position(), treasure.position());
		if (territoryOwner !== PlayerID && dist > 14000)  //  AI has no LOS, so restrict it a bit
			return;
		if (dist > distmin)
			return;
		distmin = dist;
		treasureFound = treasure;
	});
	if (treasureFound)
	{
		treasureFound.setMetadata(PlayerID, "lastGathered", gameState.ai.elapsedTime);
		this.ent.gather(treasureFound);
		m.AddTCGatherer(gameState, treasureFound.id());
		this.ent.setMetadata(PlayerID, "supply", treasureFound.id());
		return true;
	}

	var resource = this.ent.getMetadata(PlayerID, "gather-type");

	// If we are gathering food, try to hunt first
	if (resource === "food" && this.startHunting(gameState))
		return true;
	
	var findSupply = function(ent, supplies) {
		var ret = false;
		for (let i = 0; i < supplies.length; ++i)
		{
			// exhausted resource, remove it from this list
			if (!supplies[i].ent || !gameState.getEntityById(supplies[i].id))
			{
				supplies.splice(i--, 1);
				continue;
			}
			if (m.IsSupplyFull(gameState, supplies[i].ent) === true)
				continue;
			let inaccessibleTime = supplies[i].ent.getMetadata(PlayerID, "inaccessibleTime");
			if (inaccessibleTime && gameState.ai.elapsedTime < inaccessibleTime)
				continue;
			// check if available resource is worth one additionnal gatherer (except for farms)
			var nbGatherers = supplies[i].ent.resourceSupplyGatherers().length
				+ m.GetTCGatherer(gameState, supplies[i].id);
			if (supplies[i].ent.resourceSupplyType()["specific"] !== "grain"
				&& nbGatherers > 0 && supplies[i].ent.resourceSupplyAmount()/(1+nbGatherers) < 40)
				continue;
			// not in ennemy territory
			var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supplies[i].ent.position());
			if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
				continue;
			m.AddTCGatherer(gameState, supplies[i].id);
			ent.setMetadata(PlayerID, "supply", supplies[i].id);
			ret = supplies[i].ent;
			break;
		}
		return ret;
	};

	var navalManager = gameState.ai.HQ.navalManager;
	var supply;

	// first look in our own base if accessible from our present position
	if (this.baseManager.accessIndex === access)
	{
		if ((supply = findSupply(this.ent, this.baseManager.dropsiteSupplies[resource]["nearby"])))
		{
			this.ent.gather(supply);
			return true;
		}
		// --> for food, try to gather from fields if any, otherwise build one if any
		if (resource === "food")
		{
			if ((supply = this.gatherNearestField(gameState, this.baseID)))
			{
				this.ent.gather(supply);
				return true;
			}
			else if ((supply = this.buildAnyField(gameState, this.baseID)))
			{
				this.ent.repair(supply);
				return true;
			}
		}
		if ((supply = findSupply(this.ent, this.baseManager.dropsiteSupplies[resource]["medium"])))
		{
			this.ent.gather(supply);
			return true;
		}
	}
	// So if we're here we have checked our whole base for a proper resource (or it was not accessible)
	// --> check other bases directly accessible
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		if (base.accessIndex !== access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource]["nearby"])))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			this.ent.gather(supply);
			return true;
		}
	}
	if (resource === "food")	// --> for food, try to gather from fields if any, otherwise build one if any
	{
		for each (var base in gameState.ai.HQ.baseManagers)
		{
			if (base.ID === this.baseID)
				continue;
			if (base.accessIndex !== access)
				continue;
			if ((supply = this.gatherNearestField(gameState, base.ID)))
			{
				this.ent.setMetadata(PlayerID, "base", base.ID);
				this.ent.gather(supply);
				return true;
			}
			if ((supply = this.buildAnyField(gameState, base.ID)))
			{
				this.ent.setMetadata(PlayerID, "base", base.ID);
				this.ent.repair(supply);
				return true;
			}
		}
	}
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		if (base.accessIndex !== access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource]["medium"])))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			this.ent.gather(supply);
			return true;
		}
	}

	// Okay may-be we haven't found any appropriate dropsite anywhere.
	// Try to help building one if any accessible foundation available
	var foundations = gameState.getOwnFoundations().toEntityArray();
	var shouldBuild = foundations.some(function(foundation) {
		if (!foundation || foundation.getMetadata(PlayerID, "access") !== access)
			return false;
		if (foundation.resourceDropsiteTypes() && foundation.resourceDropsiteTypes().indexOf(resource) !== -1)
		{
			if (foundation.getMetadata(PlayerID, "base") !== self.baseID)
				self.ent.setMetadata(PlayerID, "base", foundation.getMetadata(PlayerID, "base"));
			self.ent.setMetadata(PlayerID, "target-foundation", foundation.id());
			self.ent.repair(foundation);
			return true;
		}
		return false;
	});
	if (shouldBuild)
		return true;

	// Still nothing ... try bases which need a transport
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.accessIndex === access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource]["nearby"])))
		{
			if (base.ID !== this.baseID)
				this.ent.setMetadata(PlayerID, "base", base.ID);
			navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position());
			return true;
		}
	}
	if (resource === "food")	// --> for food, try to gather from fields if any, otherwise build one if any
	{
		for each (var base in gameState.ai.HQ.baseManagers)
		{
			if (base.accessIndex === access)
				continue;
			if ((supply = this.gatherNearestField(gameState, base.ID)))
			{
				if (base.ID !== this.baseID)
					this.ent.setMetadata(PlayerID, "base", base.ID);
				navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position());
				return true;
			}
			if ((supply = this.buildAnyField(gameState, base.ID)))
			{
				if (base.ID !== this.baseID)
					this.ent.setMetadata(PlayerID, "base", base.ID);
				navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position());
				return true;
			}
		}
	}
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.accessIndex === access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource]["medium"])))
		{
			if (base.ID !== this.baseID)
				this.ent.setMetadata(PlayerID, "base", base.ID);
			navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position());
			return true;
		}
	}
	// Okay so we haven't found any appropriate dropsite anywhere.
	// Try to help building one if any non-accessible foundation available
	var shouldBuild = foundations.some(function(foundation) {
		if (!foundation || foundation.getMetadata(PlayerID, "access") === access)
			return false;
		if (foundation.resourceDropsiteTypes() && foundation.resourceDropsiteTypes().indexOf(resource) !== -1)
		{
			if (foundation.getMetadata(PlayerID, "base") !== self.baseID)
				self.ent.setMetadata(PlayerID, "base", foundation.getMetadata(PlayerID, "base"));
			self.ent.setMetadata(PlayerID, "target-foundation", foundation.id());
			navalManager.requireTransport(gameState, self.ent, access, base.accessIndex, foundation.position());
			return true;
		}
		return false;
	});
	if (shouldBuild)
		return true;

	// Still nothing, we look now for faraway resources, first in the accessible ones, then in the others
	if (this.baseManager.accessIndex === access)
	{
		if ((supply = findSupply(this.ent, this.baseManager.dropsiteSupplies[resource]["faraway"])))
		{
			this.ent.gather(supply);
			return true;
		}
	}
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		if (base.accessIndex !== access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource]["faraway"])))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			this.ent.gather(supply);
			return true;
		}
	}
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.accessIndex === access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource]["faraway"])))
		{
			if (base.ID !== this.baseID)
				this.ent.setMetadata(PlayerID, "base", base.ID);
			navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position());
			return true;
		}
	}

	// If we are here, we have nothing left to gather ... certainly no more resources of this type
	gameState.ai.HQ.lastFailedGather[resource] = gameState.ai.elapsedTime;
	if (gameState.ai.Config.debug > 2)
		warn(" >>>>> worker with gather-type " + resource + " with nothing to gather ");
	this.ent.setMetadata(PlayerID, "subrole", "idle");
	return false;
};

// if position is given, we only check if we could hunt from this position but do nothing
// otherwise the position of the entity is taken, and if something is found, we directly start the hunt
m.Worker.prototype.startHunting = function(gameState, position)
{
	var resources = gameState.getHuntableSupplies();	
	if (resources.length === 0)
		return false;

	if (position)
		var entPosition = position;
	else
		var entPosition = this.ent.position();

	var nearestSupplyDist = Math.min();
	var nearestSupply = undefined;

	var isCavalry = this.ent.hasClass("Cavalry");
	var isRanged = this.ent.hasClass("Ranged");
	var foodDropsites = gameState.getOwnDropsites("food");
	var access = gameState.ai.accessibility.getAccessValue(entPosition);

	var nearestDropsiteDist = function(supply) {
		var distMin = 1000000;
		var pos = supply.position();
		foodDropsites.forEach(function (dropsite) {
			if (!dropsite.position() || dropsite.getMetadata(PlayerID, "access") !== access)
				return;
			var dist = API3.SquareVectorDistance(pos, dropsite.position());
			if (dist < distMin)
				distMin = dist;
		});
		return distMin;
	};
	
	resources.forEach(function(supply)
	{
		if (!supply.position())
			return;

		let inaccessibleTime = supply.getMetadata(PlayerID, "inaccessibleTime");
		if (inaccessibleTime && gameState.ai.elapsedTime < inaccessibleTime)
			return;

		if (m.IsSupplyFull(gameState, supply) === true)
			return;
		// check if available resource is worth one additionnal gatherer (except for farms)
		var nbGatherers = supply.resourceSupplyGatherers().length
			+ m.GetTCGatherer(gameState, supply.id());
		if (nbGatherers > 0 && supply.resourceSupplyAmount()/(1+nbGatherers) < 40)
			return;

		// Only cavalry and range units should hunt fleeing animals 
		if (!supply.hasClass("Domestic") && !isCavalry && !isRanged)
			return;

		// quickscope accessbility check
		if (!gameState.ai.accessibility.pathAvailable(gameState, entPosition, supply.position(), false, true))
			return;
					  
		var supplyAccess = gameState.ai.accessibility.getAccessValue(supply.position());
		if (supplyAccess !== access)
			return;

		// measure the distance to the resource
		var dist = API3.SquareVectorDistance(entPosition, supply.position());
		// Only cavalry should hunt faraway
		if (!isCavalry && dist > 25000)
			return;

		// Avoid ennemy territory
		var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supply.position());
		if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
			return;

		var dropsiteDist = nearestDropsiteDist(supply);
		if (dropsiteDist > 35000)
			return;
		// Only cavalry should hunt far from dropsite (specially for non domestic animals which flee)
		if (!isCavalry && (dropsiteDist > 10000 || ((dropsiteDist > 7000 || territoryOwner == 0 ) && !supply.hasClass("Domestic"))))
			return;

		if (dist < nearestSupplyDist)
		{
			nearestSupplyDist = dist;
			nearestSupply = supply;
		}
	});
	
	if (nearestSupply)
	{
		if (position)
			return true;
		m.AddTCGatherer(gameState, nearestSupply.id());
		this.ent.gather(nearestSupply);
		this.ent.setMetadata(PlayerID, "supply", nearestSupply.id());
		this.ent.setMetadata(PlayerID, "target-foundation", undefined);
		return true;
	}
	return false;
};

m.Worker.prototype.startFishing = function(gameState)
{
	if (!this.ent.position())
		return false;
	
	// So here we're doing it basic. We check what we can hunt, we hunt it. No fancies.
	
	var resources = gameState.getFishableSupplies();
	if (resources.length === 0)
	{
		gameState.ai.HQ.navalManager.resetFishingBoats(gameState);
		this.ent.destroy();
		return false;
	}

	var nearestSupplyDist = Math.min();
	var nearestSupply = undefined;

	var entPosition = this.ent.position();
	var fisherSea = this.ent.getMetadata(PlayerID, "sea");
	var docks = gameState.getOwnStructures().filter(API3.Filters.and(API3.Filters.byClass("Dock"), API3.Filters.not(API3.Filters.isFoundation())));

	var nearestDropsiteDist = function(supply) {
		var distMin = 1000000;
		var pos = supply.position();
		docks.forEach(function (dock) {
			if (!dock.position() || dock.getMetadata(PlayerID, "sea") !== fisherSea)
				return;
			var dist = API3.SquareVectorDistance(pos, dock.position());
			if (dist < distMin)
				distMin = dist;
		});
		return distMin;
	};
	
	resources.forEach(function(supply)
	{
		if (!supply.position())
			return;

		if (m.IsSupplyFull(gameState, supply) === true)
			return;
		// check if available resource is worth one additionnal gatherer (except for farms)
		var nbGatherers = supply.resourceSupplyGatherers().length
			+ m.GetTCGatherer(gameState, supply.id());
		if (nbGatherers > 0 && supply.resourceSupplyAmount()/(1+nbGatherers) < 40)
			return;

		// check that it is accessible
		if (!supply.getMetadata(PlayerID, "sea"))
			supply.setMetadata(PlayerID, "sea", gameState.ai.accessibility.getAccessValue(supply.position(), true));
		if (supply.getMetadata(PlayerID, "sea") !== fisherSea)
			return;

		// measure the distance to the resource
		var dist = API3.SquareVectorDistance(entPosition, supply.position());
		if (dist > 40000)
			return;

		// Avoid ennemy territory
		var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supply.position());
		if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
			return;

		var dropsiteDist = nearestDropsiteDist(supply);
		if (dropsiteDist > 35000)
			return;

		if (dist < nearestSupplyDist)
		{
			nearestSupplyDist = dist;
			nearestSupply = supply;
		}
	});
	
	if (nearestSupply)
	{
		m.AddTCGatherer(gameState, nearestSupply.id());
		this.ent.gather(nearestSupply);
		this.ent.setMetadata(PlayerID, "supply", nearestSupply.id());
		this.ent.setMetadata(PlayerID, "target-foundation", undefined);
		return true;
	}
	else
	{
		if (this.ent.getMetadata(PlayerID,"subrole") === "fisher")
			this.ent.setMetadata(PlayerID, "subrole", "idle");
		return false;
	}
};

m.Worker.prototype.gatherNearestField = function(gameState, baseID)
{
	var self = this;
	var ownFields = gameState.getOwnEntitiesByType(gameState.applyCiv("structures/{civ}_field"), true).filter(API3.Filters.byMetadata(PlayerID, "base", baseID));
	var bestFarmEnt = false;
	var bestFarmDist = 10000000;

	ownFields.forEach(function (field) {
		if (m.IsSupplyFull(gameState, field) === true)
			return;
		var dist = API3.SquareVectorDistance(field.position(), self.ent.position());
		if (dist < bestFarmDist)
		{
			bestFarmEnt = field;
			bestFarmDist = dist;
		}
	});
	if (bestFarmEnt)
	{
		m.AddTCGatherer(gameState, bestFarmEnt.id());
		this.ent.setMetadata(PlayerID, "supply", bestFarmEnt.id());
	}
	return bestFarmEnt;
};

/**
 * WARNING with the present options of AI orders, the unit will not gather after building the farm.
 * This is done by calling the gatherNearestField function when construction is completed. 
 */
m.Worker.prototype.buildAnyField = function(gameState, baseID)
{
	var self = this;
	var baseFoundations = gameState.getOwnFoundations().filter(API3.Filters.byMetadata(PlayerID, "base", baseID));
	
	var maxGatherers = gameState.getTemplate(gameState.applyCiv("structures/{civ}_field")).maxGatherers();

	var bestFarmEnt = false;
	var bestFarmDist = 10000000;
	baseFoundations.forEach(function (found) {
		if (found.hasClass("Field")) {
			var current = found.getBuildersNb();
			if (current === undefined || current >= maxGatherers)
				return;
			var dist = API3.SquareVectorDistance(found.position(), self.ent.position());
			if (dist < bestFarmDist)
			{
				bestFarmEnt = found;
				bestFarmDist = dist;
			}
		}
	});
	return bestFarmEnt;
};

// Workers elephant should move away from the buildings they've built to avoid being trapped in between constructions
// For the time being, we move towards the nearest gatherer (providing him a dropsite)
m.Worker.prototype.moveAway = function(gameState){
	var gatherers = this.baseManager.workersBySubrole(gameState, "gatherer");
	var pos = this.ent.position();
	var dist = Math.min();
	var destination = pos;
	for (var gatherer of gatherers.values())
	{
		if (!gatherer.position() || gatherer.getMetadata(PlayerID, "transport") !== undefined)
			continue;
		if (gatherer.isIdle())
			continue;
		var distance = API3.SquareVectorDistance(pos, gatherer.position());
		if (distance > dist)
			continue;
		dist = distance;
		destination = gatherer.position();
	}
	this.ent.move(destination[0], destination[1]);
};

// Check accessibility of the target when in approach (in RMS maps, we quite often have chicken or bushes
// inside obstruction of other entities). The resource will be flagged as inaccessible during 10 mn (in case
// it will be cleared later).
m.Worker.prototype.isInaccessibleSupply = function(gameState)
{
	if (!this.ent.unitAIOrderData()[0] || !this.ent.unitAIOrderData()[0]["target"])
		return false;
	let approachingTime = this.ent.getMetadata(PlayerID, "approachingTime");
	if (!approachingTime || gameState.ai.elapsedTime - approachingTime > 5)
	{
		let presentPos = this.ent.position();
		let approachingPos = this.ent.getMetadata(PlayerID, "approachingPos");
		if (!approachingPos || approachingPos[0] != presentPos[0] || approachingPos[1] != presentPos[1])
		{
			this.ent.setMetadata(PlayerID, "approachingTime", gameState.ai.elapsedTime);
			this.ent.setMetadata(PlayerID, "approachingPos", presentPos);
		}
		else if (gameState.ai.elapsedTime - approachingTime > 15)
		{
			let targetId = this.ent.unitAIOrderData()[0]["target"];
			let target = gameState.getEntityById(targetId);
			if (target)
				target.setMetadata(PlayerID, "inaccessibleTime", gameState.ai.elapsedTime + 600);
			return true;
		}
	}
	return false;
}
return m;
}(PETRA);
