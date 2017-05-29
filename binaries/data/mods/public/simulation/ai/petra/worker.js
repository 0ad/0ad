var PETRA = function(m)
{

/**
 * This class makes a worker do as instructed by the economy manager
 */

m.Worker = function(base)
{
	this.ent = undefined;
	this.base = base;
	this.baseID = base.ID;
};

m.Worker.prototype.update = function(gameState, ent)
{
	if (!ent.position() || ent.getMetadata(PlayerID, "plan") === -2 || ent.getMetadata(PlayerID, "plan") === -3)
		return;

	// If we are waiting for a transport or we are sailing, just wait
	if (ent.getMetadata(PlayerID, "transport") !== undefined)
		return;

	// base 0 for unassigned entities has no accessIndex, so take the one from the entity
	if (this.baseID === gameState.ai.HQ.baseManagers[0].ID)
		this.accessIndex = gameState.ai.accessibility.getAccessValue(ent.position());
	else
		this.accessIndex = this.base.accessIndex;

	let subrole = ent.getMetadata(PlayerID, "subrole");
	if (!subrole)	// subrole may-be undefined after a transport, garrisoning, army, ...
	{
		ent.setMetadata(PlayerID, "subrole", "idle");
		this.base.reassignIdleWorkers(gameState, [ent]);
		this.update(gameState, ent);
		return;
	}

	this.ent = ent;

	let unitAIState = ent.unitAIState();
	if ((subrole === "hunter" || subrole === "gatherer") &&
	    (unitAIState === "INDIVIDUAL.GATHER.GATHERING" || unitAIState === "INDIVIDUAL.GATHER.APPROACHING" ||
	     unitAIState === "INDIVIDUAL.COMBAT.APPROACHING"))
	{
		if (this.isInaccessibleSupply(gameState) && !this.retryGathering(gameState, subrole))
			ent.stopMoving();

		// Check that we have not drifted too far
		if (unitAIState === "INDIVIDUAL.COMBAT.APPROACHING" && ent.unitAIOrderData().length)
		{
			let orderData = ent.unitAIOrderData()[0];
			if (orderData && orderData.target)
			{
				let supply = gameState.getEntityById(orderData.target);
				if (supply && supply.resourceSupplyType() && supply.resourceSupplyType().generic === "food")
				{
					let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supply.position());
					if (gameState.isPlayerEnemy(territoryOwner) && !this.retryGathering(gameState, subrole))
						ent.stopMoving();
					else if (!gameState.isPlayerAlly(territoryOwner))
					{
						let distanceSquare = ent.hasClass("Cavalry") ? 90000 : 30000;
						let supplyAccess = gameState.ai.accessibility.getAccessValue(supply.position());
						let foodDropsites = gameState.playerData.hasSharedDropsites ?
						                    gameState.getAnyDropsites("food") : gameState.getOwnDropsites("food");
						let hasFoodDropsiteWithinDistance = false;
						for (let dropsite of foodDropsites.values())
						{
							if (!dropsite.position())
								continue;
							let owner = dropsite.owner();
							// owner !== PlayerID can only happen when hasSharedDropsites === true, so no need to test it again
							if (owner !== PlayerID && (!dropsite.isSharedDropsite() || !gameState.isPlayerMutualAlly(owner)))
								continue;
							if (supplyAccess !== m.getLandAccess(gameState, dropsite))
								continue;
							if (API3.SquareVectorDistance(supply.position(), dropsite.position()) < distanceSquare)
							{
								hasFoodDropsiteWithinDistance = true;
								break;
							}
						}
						if (!hasFoodDropsiteWithinDistance && !this.retryGathering(gameState, subrole))
							ent.stopMoving();
					}
				}
			}
		}
	}
	else if (ent.getMetadata(PlayerID, "approachingTarget"))
	{
		ent.setMetadata(PlayerID, "approachingTarget", undefined);
		ent.setMetadata(PlayerID, "alreadyTried", undefined);
	}

	let unitAIStateOrder = unitAIState.split(".")[1];
	// If we're fighting or hunting, let's not start gathering
	// but for fishers where UnitAI must have made us target a moving whale.
	// Also, if we are attacking, do not capture
	if (unitAIStateOrder === "COMBAT")
	{
		if (subrole === "fisher")
			this.startFishing(gameState);
		else if (unitAIState === "INDIVIDUAL.COMBAT.ATTACKING" && ent.unitAIOrderData().length &&
			!ent.getMetadata(PlayerID, "PartOfArmy"))
		{
			let orderData = ent.unitAIOrderData()[0];
			if (orderData && orderData.target && orderData.attackType && orderData.attackType === "Capture")
			{
				// If we are here, an enemy structure must have targeted one of our workers
				// and UnitAI sent it fight back with allowCapture=true
				let target = gameState.getEntityById(orderData.target);
				if (target && target.owner() > 0 && !gameState.isPlayerAlly(target.owner()))
					ent.attack(orderData.target, m.allowCapture(gameState, ent, target));
			}
		}
		return;
	}

	// Okay so we have a few tasks.
	// If we're gathering, we'll check that we haven't run idle.
	// And we'll also check that we're gathering a resource we want to gather.

	if (subrole === "gatherer")
	{
		if (ent.isIdle())
		{
			// if we aren't storing resources or it's the same type as what we're about to gather,
			// let's just pick a new resource.
			// TODO if we already carry the max we can ->  returnresources
			if (!ent.resourceCarrying() || !ent.resourceCarrying().length ||
				ent.resourceCarrying()[0].type === ent.getMetadata(PlayerID, "gather-type"))
			{
				this.startGathering(gameState);
			}
			else if (!m.returnResources(gameState, ent))     // try to deposit resources
			{
				// no dropsite, abandon old resources and start gathering new ones
				this.startGathering(gameState);
			}
		}
		else if (unitAIStateOrder === "GATHER")
		{
			// we're already gathering. But let's check if there is nothing better
			// in case UnitAI did something bad
			if (ent.unitAIOrderData().length)
			{
				let supplyId = ent.unitAIOrderData()[0].target;
				let supply = gameState.getEntityById(supplyId);
				if (supply && !supply.hasClass("Field") && !supply.hasClass("Animal") &&
					supply.resourceSupplyType().generic !== "treasure" &&
					supplyId !== ent.getMetadata(PlayerID, "supply"))
				{
					let nbGatherers = supply.resourceSupplyNumGatherers() + gameState.ai.HQ.GetTCGatherer(supplyId);
					if (nbGatherers > 1 && supply.resourceSupplyAmount()/nbGatherers < 30)
					{
						gameState.ai.HQ.RemoveTCGatherer(supplyId);
						this.startGathering(gameState);
					}
					else
					{
						let gatherType = ent.getMetadata(PlayerID, "gather-type");
						let nearby = this.base.dropsiteSupplies[gatherType].nearby;
						let isNearby = nearby.some(sup => sup.id === supplyId);
						if (nearby.length === 0 || isNearby)
							ent.setMetadata(PlayerID, "supply", supplyId);
						else
						{
							gameState.ai.HQ.RemoveTCGatherer(supplyId);
							this.startGathering(gameState);
						}
					}
				}
			}
		}
		else if (unitAIState === "INDIVIDUAL.RETURNRESOURCE.APPROACHING" && gameState.ai.playedTurn % 10 === 0)
		{
			// Check from time to time that UnitAI does not send us to an inaccessible dropsite
			let dropsite = gameState.getEntityById(ent.unitAIOrderData()[0].target);
			if (dropsite && dropsite.position())
			{
				let access = gameState.ai.accessibility.getAccessValue(ent.position());
				let goalAccess = dropsite.getMetadata(PlayerID, "access");
				if (!goalAccess || dropsite.hasClass("Elephant"))
				{
					goalAccess = gameState.ai.accessibility.getAccessValue(dropsite.position());
					dropsite.setMetadata(PlayerID, "access", goalAccess);
				}
				if (access !== goalAccess)
					m.returnResources(gameState, this.ent);
			}
		}
	}
	else if (subrole === "builder")
	{
		if (unitAIStateOrder === "REPAIR")
		{
			// update our target in case UnitAI sent us to a different foundation because of autocontinue
			if (ent.unitAIOrderData()[0] && ent.unitAIOrderData()[0].target &&
				ent.getMetadata(PlayerID, "target-foundation") !== ent.unitAIOrderData()[0].target)
				ent.setMetadata(PlayerID, "target-foundation", ent.unitAIOrderData()[0].target);
			// and check that the target still exists (useful in REPAIR.APPROACHING)
			let target = ent.getMetadata(PlayerID, "target-foundation");
			if (target && gameState.getEntityById(target))
				return;
			ent.stopMoving();
		}
		// okay so apparently we aren't working.
		// Unless we've been explicitely told to keep our role, make us idle.
		let target = gameState.getEntityById(ent.getMetadata(PlayerID, "target-foundation"));
		if (!target || (target.foundationProgress() === undefined && target.needsRepair() === false))
		{
			ent.setMetadata(PlayerID, "subrole", "idle");
			ent.setMetadata(PlayerID, "target-foundation", undefined);
			// If worker elephant, move away to avoid being trapped in between constructions
			if (ent.hasClass("Elephant"))
				this.moveAway(gameState);
			else if (this.baseID !== gameState.ai.HQ.baseManagers[0].ID)
			{
				// reassign it to something useful
				this.base.reassignIdleWorkers(gameState, [ent]);
				this.update(gameState, ent);
				return;
			}
		}
		else
		{
			let access = gameState.ai.accessibility.getAccessValue(ent.position());
			let goalAccess = m.getLandAccess(gameState, target);
			let queued = m.returnResources(gameState, ent);
			if (access === goalAccess)
				ent.repair(target, target.hasClass("House"), queued);  // autocontinue=true for houses
			else
				gameState.ai.HQ.navalManager.requireTransport(gameState, ent, access, goalAccess, target.position());
		}
	}
	else if (subrole === "hunter")
	{
		let lastHuntSearch = ent.getMetadata(PlayerID, "lastHuntSearch");
		if (ent.isIdle() && (!lastHuntSearch || gameState.ai.elapsedTime - lastHuntSearch > 20))
		{
			if (!this.startHunting(gameState))
			{
				// nothing to hunt around. Try another region if any
				let nowhereToHunt = true;
				for (let base of gameState.ai.HQ.baseManagers)
				{
					if (!base.anchor || !base.anchor.position())
						continue;
					let basePos = base.anchor.position();
					if (this.startHunting(gameState, basePos))
					{
						ent.setMetadata(PlayerID, "base", base.ID);
						let access = gameState.ai.accessibility.getAccessValue(ent.position());
						if (base.accessIndex === access)
							ent.move(basePos[0], basePos[1]);
						else
							gameState.ai.HQ.navalManager.requireTransport(gameState, ent, access, base.accessIndex, basePos);
						nowhereToHunt = false;
						break;
					}
				}
				if (nowhereToHunt)
					ent.setMetadata(PlayerID, "lastHuntSearch", gameState.ai.elapsedTime);
			}
		}
		else	// Perform some sanity checks
		{
			if (unitAIStateOrder === "GATHER" || unitAIStateOrder === "RETURNRESOURCE")
			{
				// we may have drifted towards ennemy territory during the hunt, if yes go home
				let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(ent.position());
				if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
					this.startHunting(gameState);
				else if (unitAIState === "INDIVIDUAL.RETURNRESOURCE.APPROACHING")
				{
					// Check that UnitAI does not send us to an inaccessible dropsite
					let dropsite = gameState.getEntityById(ent.unitAIOrderData()[0].target);
					if (dropsite && dropsite.position())
					{
						let access = gameState.ai.accessibility.getAccessValue(ent.position());
						let goalAccess = dropsite.getMetadata(PlayerID, "access");
						if (!goalAccess || dropsite.hasClass("Elephant"))
						{
							goalAccess = gameState.ai.accessibility.getAccessValue(dropsite.position());
							dropsite.setMetadata(PlayerID, "access", goalAccess);
						}
						if (access !== goalAccess)
							m.returnResources(gameState, ent);
					}
				}
			}
		}
	}
	else if (subrole === "fisher")
	{
		if (ent.isIdle())
			this.startFishing(gameState);
		else	// if we have drifted towards ennemy territory during the fishing, go home
		{
			let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(ent.position());
			if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
				this.startFishing(gameState);
		}
	}
};

m.Worker.prototype.retryGathering = function(gameState, subrole)
{
	switch (subrole)
	{
	case "gatherer":
		return this.startGathering(gameState);
	case "hunter":
		return this.startHunting(gameState);
	case "fisher":
		return this.startFishing(gameState);
	default:
		return false;
	}
};

m.Worker.prototype.startGathering = function(gameState)
{
	let access = gameState.ai.accessibility.getAccessValue(this.ent.position());

	// First look for possible treasure if any
	if (this.gatherTreasure(gameState))
		return true;

	let resource = this.ent.getMetadata(PlayerID, "gather-type");

	// If we are gathering food, try to hunt first
	if (resource === "food" && this.startHunting(gameState))
		return true;

	let findSupply = function(ent, supplies) {
		let ret = false;
		for (let i = 0; i < supplies.length; ++i)
		{
			// exhausted resource, remove it from this list
			if (!supplies[i].ent || !gameState.getEntityById(supplies[i].id))
			{
				supplies.splice(i--, 1);
				continue;
			}
			if (m.IsSupplyFull(gameState, supplies[i].ent))
				continue;
			let inaccessibleTime = supplies[i].ent.getMetadata(PlayerID, "inaccessibleTime");
			if (inaccessibleTime && gameState.ai.elapsedTime < inaccessibleTime)
				continue;
			// check if available resource is worth one additionnal gatherer (except for farms)
			let nbGatherers = supplies[i].ent.resourceSupplyNumGatherers() + gameState.ai.HQ.GetTCGatherer(supplies[i].id);
			if (supplies[i].ent.resourceSupplyType().specific !== "grain" &&
				nbGatherers > 0 && supplies[i].ent.resourceSupplyAmount()/(1+nbGatherers) < 30)
				continue;
			// not in ennemy territory
			let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supplies[i].ent.position());
			if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
				continue;
			gameState.ai.HQ.AddTCGatherer(supplies[i].id);
			ent.setMetadata(PlayerID, "supply", supplies[i].id);
			ret = supplies[i].ent;
			break;
		}
		return ret;
	};

	let navalManager = gameState.ai.HQ.navalManager;
	let supply;

	// first look in our own base if accessible from our present position
	if (this.accessIndex === access)
	{
		if ((supply = findSupply(this.ent, this.base.dropsiteSupplies[resource].nearby)))
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
			else if (this.ent.isBuilder() && (supply = this.buildAnyField(gameState, this.baseID)))
			{
				this.ent.repair(supply);
				return true;
			}
		}
		if ((supply = findSupply(this.ent, this.base.dropsiteSupplies[resource].medium)))
		{
			this.ent.gather(supply);
			return true;
		}
	}
	// So if we're here we have checked our whole base for a proper resource (or it was not accessible)
	// --> check other bases directly accessible
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		if (base.accessIndex !== access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource].nearby)))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			this.ent.gather(supply);
			return true;
		}
	}
	if (resource === "food")	// --> for food, try to gather from fields if any, otherwise build one if any
	{
		for (let base of gameState.ai.HQ.baseManagers)
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
			if (this.ent.isBuilder() && (supply = this.buildAnyField(gameState, base.ID)))
			{
				this.ent.setMetadata(PlayerID, "base", base.ID);
				this.ent.repair(supply);
				return true;
			}
		}
	}
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		if (base.accessIndex !== access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource].medium)))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			this.ent.gather(supply);
			return true;
		}
	}

	// Okay may-be we haven't found any appropriate dropsite anywhere.
	// Try to help building one if any accessible foundation available
	let foundations = gameState.getOwnFoundations().toEntityArray();
	let shouldBuild = this.ent.isBuilder() && foundations.some(function(foundation) {
		if (!foundation || foundation.getMetadata(PlayerID, "access") !== access)
			return false;
		if (foundation.resourceDropsiteTypes() && foundation.resourceDropsiteTypes().indexOf(resource) !== -1)
		{
			if (foundation.getMetadata(PlayerID, "base") !== this.baseID)
				this.ent.setMetadata(PlayerID, "base", foundation.getMetadata(PlayerID, "base"));
			this.ent.setMetadata(PlayerID, "target-foundation", foundation.id());
			this.ent.repair(foundation);
			return true;
		}
		return false;
	}, this);
	if (shouldBuild)
		return true;

	// Still nothing ... try bases which need a transport
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (base.accessIndex === access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource].nearby)))
		{
			if (navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position()))
			{
				if (base.ID !== this.baseID)
					this.ent.setMetadata(PlayerID, "base", base.ID);
				return true;
			}
		}
	}
	if (resource === "food")	// --> for food, try to gather from fields if any, otherwise build one if any
	{
		for (let base of gameState.ai.HQ.baseManagers)
		{
			if (base.accessIndex === access)
				continue;
			if ((supply = this.gatherNearestField(gameState, base.ID)))
			{
				if (navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position()))
				{
					if (base.ID !== this.baseID)
						this.ent.setMetadata(PlayerID, "base", base.ID);
					return true;
				}
			}
			if ((supply = this.buildAnyField(gameState, base.ID)))
			{
				if (navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position()))
				{
					if (base.ID !== this.baseID)
						this.ent.setMetadata(PlayerID, "base", base.ID);
					return true;
				}
			}
		}
	}
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (base.accessIndex === access)
			continue;
		if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource].medium)))
		{
			if (navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position()))
			{
				if (base.ID !== this.baseID)
					this.ent.setMetadata(PlayerID, "base", base.ID);
				return true;
			}
		}
	}
	// Okay so we haven't found any appropriate dropsite anywhere.
	// Try to help building one if any non-accessible foundation available
	shouldBuild = this.ent.isBuilder() && foundations.some(function(foundation) {
		if (!foundation || foundation.getMetadata(PlayerID, "access") === access)
			return false;
		if (foundation.resourceDropsiteTypes() && foundation.resourceDropsiteTypes().indexOf(resource) !== -1)
		{
			let foundationAccess = m.getLandAccess(gameState, foundation);
			if (navalManager.requireTransport(gameState, this.ent, access, foundationAccess, foundation.position()))
			{
				if (foundation.getMetadata(PlayerID, "base") !== this.baseID)
					this.ent.setMetadata(PlayerID, "base", foundation.getMetadata(PlayerID, "base"));
				this.ent.setMetadata(PlayerID, "target-foundation", foundation.id());
				return true;
			}
		}
		return false;
	}, this);
	if (shouldBuild)
		return true;

	// Still nothing, we look now for faraway resources, first in the accessible ones, then in the others
	// except for food which is not worth (farms or corrals should be used)
	if (resource !== "food")
	{
		if (this.accessIndex === access)
		{
			if ((supply = findSupply(this.ent, this.base.dropsiteSupplies[resource].faraway)))
			{
				this.ent.gather(supply);
				return true;
			}
		}
		for (let base of gameState.ai.HQ.baseManagers)
		{
			if (base.ID === this.baseID)
				continue;
			if (base.accessIndex !== access)
				continue;
			if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource].faraway)))
			{
				this.ent.setMetadata(PlayerID, "base", base.ID);
				this.ent.gather(supply);
				return true;
			}
		}
		for (let base of gameState.ai.HQ.baseManagers)
		{
			if (base.accessIndex === access)
				continue;
			if ((supply = findSupply(this.ent, base.dropsiteSupplies[resource].faraway)))
			{
				if (navalManager.requireTransport(gameState, this.ent, access, base.accessIndex, supply.position()))
				{
					if (base.ID !== this.baseID)
						this.ent.setMetadata(PlayerID, "base", base.ID);
					return true;
				}
			}
		}
	}

	// If we are here, we have nothing left to gather ... certainly no more resources of this type
	gameState.ai.HQ.lastFailedGather[resource] = gameState.ai.elapsedTime;
	if (gameState.ai.Config.debug > 2)
		warn(" >>>>> worker with gather-type " + resource + " with nothing to gather ");
	this.ent.setMetadata(PlayerID, "subrole", "idle");
	return false;
};

/**
 * if position is given, we only check if we could hunt from this position but do nothing
 * otherwise the position of the entity is taken, and if something is found, we directly start the hunt
 */
m.Worker.prototype.startHunting = function(gameState, position)
{
	// First look for possible treasure if any
	if (!position && this.gatherTreasure(gameState))
		return true;

	let resources = gameState.getHuntableSupplies();
	if (!resources.hasEntities())
		return false;

	let nearestSupplyDist = Math.min();
	let nearestSupply;

	let isCavalry = this.ent.hasClass("Cavalry");
	let isRanged = this.ent.hasClass("Ranged");
	let entPosition = position ? position : this.ent.position();
	let access = gameState.ai.accessibility.getAccessValue(entPosition);
	let foodDropsites = gameState.playerData.hasSharedDropsites ?
	                    gameState.getAnyDropsites("food") : gameState.getOwnDropsites("food");

	let hasFoodDropsiteWithinDistance = function(supplyPosition, supplyAccess, distSquare)
	{
		for (let dropsite of foodDropsites.values())
		{
			if (!dropsite.position())
				continue;
			let owner = dropsite.owner();
			// owner !== PlayerID can only happen when hasSharedDropsites === true, so no need to test it again
			if (owner !== PlayerID && (!dropsite.isSharedDropsite() || !gameState.isPlayerMutualAlly(owner)))
				continue;
			if (supplyAccess !== m.getLandAccess(gameState, dropsite))
				continue;
			if (API3.SquareVectorDistance(supplyPosition, dropsite.position()) < distSquare)
				return true;
		}
		return false;
	};

	resources.forEach(function(supply)
	{
		if (!supply.position())
			return;

		let inaccessibleTime = supply.getMetadata(PlayerID, "inaccessibleTime");
		if (inaccessibleTime && gameState.ai.elapsedTime < inaccessibleTime)
			return;

		if (m.IsSupplyFull(gameState, supply))
			return;
		// check if available resource is worth one additionnal gatherer (except for farms)
		let nbGatherers = supply.resourceSupplyNumGatherers() + gameState.ai.HQ.GetTCGatherer(supply.id());
		if (nbGatherers > 0 && supply.resourceSupplyAmount()/(1+nbGatherers) < 30)
			return;

		let canFlee = !supply.hasClass("Domestic") && supply.templateName().indexOf("resource|") == -1;
		// Only cavalry and range units should hunt fleeing animals
		if (canFlee && !isCavalry && !isRanged)
			return;

		let supplyAccess = gameState.ai.accessibility.getAccessValue(supply.position());
		if (supplyAccess !== access)
			return;

		// measure the distance to the resource
		let dist = API3.SquareVectorDistance(entPosition, supply.position());
		if (dist > nearestSupplyDist)
			return;

		// Only cavalry should hunt faraway
		if (!isCavalry && dist > 25000)
			return;

		// Avoid ennemy territory
		let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supply.position());
		if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
			return;
		// And if in ally territory, don't hunt this ally's cattle
		if (territoryOwner !== 0 && territoryOwner !== PlayerID && supply.owner() === territoryOwner)
			return;

		// Only cavalry should hunt far from dropsite (specially for non domestic animals which flee)
 		if (!isCavalry && canFlee && territoryOwner === 0)
			return;
		let distanceSquare = isCavalry ? 35000 : ( canFlee ? 7000 : 12000);
		if (!hasFoodDropsiteWithinDistance(supply.position(), supplyAccess, distanceSquare))
			return;

		nearestSupplyDist = dist;
		nearestSupply = supply;
	});

	if (nearestSupply)
	{
		if (position)
			return true;
		gameState.ai.HQ.AddTCGatherer(nearestSupply.id());
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

	let resources = gameState.getFishableSupplies();
	if (!resources.hasEntities())
	{
		gameState.ai.HQ.navalManager.resetFishingBoats(gameState);
		this.ent.destroy();
		return false;
	}

	let nearestSupplyDist = Math.min();
	let nearestSupply;

	let fisherSea = this.ent.getMetadata(PlayerID, "sea");
	let fishDropsites = (gameState.playerData.hasSharedDropsites ? gameState.getAnyDropsites("food") : gameState.getOwnDropsites("food")).filter(API3.Filters.byClass("Dock")).toEntityArray();

	let nearestDropsiteDist = function(supply) {
		let distMin = 1000000;
		let pos = supply.position();
		for (let dropsite of fishDropsites)
		{
			if (!dropsite.position())
				continue;
			let owner = dropsite.owner();
			// owner !== PlayerID can only happen when hasSharedDropsites === true, so no need to test it again
			if (owner !== PlayerID && (!dropsite.isSharedDropsite() || !gameState.isPlayerMutualAlly(owner)))
				continue;
			if (fisherSea !== m.getSeaAccess(gameState, dropsite))
				continue;
			distMin = Math.min(distMin, API3.SquareVectorDistance(pos, dropsite.position()));
		}
		return distMin;
	};

	let exhausted = true;
	resources.forEach(function(supply)
	{
		if (!supply.position())
			return;

		// check that it is accessible
		if (gameState.ai.HQ.navalManager.getFishSea(gameState, supply) !== fisherSea)
			return;

		exhausted = false;

		if (m.IsSupplyFull(gameState, supply))
			return;
		// check if available resource is worth one additionnal gatherer (except for farms)
		let nbGatherers = supply.resourceSupplyNumGatherers() + gameState.ai.HQ.GetTCGatherer(supply.id());
		if (nbGatherers > 0 && supply.resourceSupplyAmount()/(1+nbGatherers) < 30)
			return;

		// Avoid ennemy territory
		if (!gameState.ai.HQ.navalManager.canFishSafely(gameState, supply))
			return;

		// measure the distance from the resource to the nearest dropsite
		let dist = nearestDropsiteDist(supply);
		if (dist > nearestSupplyDist)
			return;

		nearestSupplyDist = dist;
		nearestSupply = supply;
	});

	if (exhausted)
	{
		gameState.ai.HQ.navalManager.resetFishingBoats(gameState, fisherSea);
		this.ent.destroy();
		return false;
	}

	if (nearestSupply)
	{
		gameState.ai.HQ.AddTCGatherer(nearestSupply.id());
		this.ent.gather(nearestSupply);
		this.ent.setMetadata(PlayerID, "supply", nearestSupply.id());
		this.ent.setMetadata(PlayerID, "target-foundation", undefined);
		return true;
	}
	if (this.ent.getMetadata(PlayerID,"subrole") === "fisher")
		this.ent.setMetadata(PlayerID, "subrole", "idle");
	return false;
};

m.Worker.prototype.gatherNearestField = function(gameState, baseID)
{
	let ownFields = gameState.getOwnEntitiesByClass("Field", true).filter(API3.Filters.isBuilt()).filter(API3.Filters.byMetadata(PlayerID, "base", baseID));
	let bestFarmEnt = false;
	let bestFarmDist = 10000000;

	for (let field of ownFields.values())
	{
		if (m.IsSupplyFull(gameState, field))
			continue;
		let dist = API3.SquareVectorDistance(field.position(), this.ent.position());
		if (dist < bestFarmDist)
		{
			bestFarmEnt = field;
			bestFarmDist = dist;
		}
	}
	if (bestFarmEnt)
	{
		gameState.ai.HQ.AddTCGatherer(bestFarmEnt.id());
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
	let baseFoundations = gameState.getOwnFoundations().filter(API3.Filters.byMetadata(PlayerID, "base", baseID));
	let maxGatherers = gameState.getTemplate(gameState.applyCiv("structures/{civ}_field")).maxGatherers();
	let bestFarmEnt = false;
	let bestFarmDist = 10000000;
	let pos = this.ent.position();
	for (let found of baseFoundations.values())
	{
		if (!found.hasClass("Field"))
			continue;
		let current = found.getBuildersNb();
		if (current === undefined || current >= maxGatherers)
			continue;
		let dist = API3.SquareVectorDistance(found.position(), pos);
		if (dist > bestFarmDist)
			continue;
		bestFarmEnt = found;
		bestFarmDist = dist;
	}
	return bestFarmEnt;
};

/**
 * Look for some treasure to gather
 */
m.Worker.prototype.gatherTreasure = function(gameState)
{
	let rates = this.ent.resourceGatherRates();
	if (!rates || !rates.treasure || rates.treasure <= 0)
		return false;
	let treasureFound;
	let distmin = Math.min();
	let access = gameState.ai.accessibility.getAccessValue(this.ent.position());
	for (let treasure of gameState.ai.HQ.treasures.values())
	{
		if (m.IsSupplyFull(gameState, treasure))
			continue;
		// let some time for the previous gatherer to reach the treasure befor trying again
		let lastGathered = treasure.getMetadata(PlayerID, "lastGathered");
		if (lastGathered && gameState.ai.elapsedTime - lastGathered < 20)
			continue;
		if (access !== m.getLandAccess(gameState, treasure))
			continue;
		let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(treasure.position());
		if (territoryOwner !== 0 && !gameState.isPlayerAlly(territoryOwner))
			continue;
		let dist = API3.SquareVectorDistance(this.ent.position(), treasure.position());
		if (territoryOwner !== PlayerID && dist > 14000)  //  AI has no LOS, so restrict it a bit
			continue;
		if (dist > distmin)
			continue;
		distmin = dist;
		treasureFound = treasure;
	}
	if (!treasureFound)
		return false;
	treasureFound.setMetadata(PlayerID, "lastGathered", gameState.ai.elapsedTime);
	this.ent.gather(treasureFound);
	gameState.ai.HQ.AddTCGatherer(treasureFound.id());
	this.ent.setMetadata(PlayerID, "supply", treasureFound.id());
	return true;
};

/**
 * Workers elephant should move away from the buildings they've built to avoid being trapped in between constructions
 * For the time being, we move towards the nearest gatherer (providing him a dropsite)
 */
m.Worker.prototype.moveAway = function(gameState)
{
	let gatherers = this.base.workersBySubrole(gameState, "gatherer");
	let pos = this.ent.position();
	let dist = Math.min();
	let destination = pos;
	for (let gatherer of gatherers.values())
	{
		if (!gatherer.position() || gatherer.getMetadata(PlayerID, "transport") !== undefined)
			continue;
		if (gatherer.isIdle())
			continue;
		let distance = API3.SquareVectorDistance(pos, gatherer.position());
		if (distance > dist)
			continue;
		dist = distance;
		destination = gatherer.position();
	}
	this.ent.move(destination[0], destination[1]);
};

/**
 * Check accessibility of the target when in approach (in RMS maps, we quite often have chicken or bushes
 * inside obstruction of other entities). The resource will be flagged as inaccessible during 10 mn (in case
 * it will be cleared later).
 */
m.Worker.prototype.isInaccessibleSupply = function(gameState)
{
	if (!this.ent.unitAIOrderData()[0] || !this.ent.unitAIOrderData()[0].target)
		return false;
	let targetId = this.ent.unitAIOrderData()[0].target;
	let target = gameState.getEntityById(targetId);
	if (!target)
		return true;

	if (!target.resourceSupplyType())
		return false;

	let approachingTarget = this.ent.getMetadata(PlayerID, "approachingTarget");
	let carriedAmount = this.ent.resourceCarrying().length ? this.ent.resourceCarrying()[0].amount : 0;
	if (!approachingTarget || approachingTarget !== targetId)
	{
		this.ent.setMetadata(PlayerID, "approachingTarget", targetId);
		this.ent.setMetadata(PlayerID, "approachingTime", undefined);
		this.ent.setMetadata(PlayerID, "approachingPos", undefined);
		this.ent.setMetadata(PlayerID, "carriedBefore", carriedAmount);
		let alreadyTried = this.ent.getMetadata(PlayerID, "alreadyTried");
		if (alreadyTried && alreadyTried !== targetId)
			this.ent.setMetadata(PlayerID, "alreadyTried", undefined);
	}

	let carriedBefore = this.ent.getMetadata(PlayerID, "carriedBefore");
	if (carriedBefore !== carriedAmount)
	{
		this.ent.setMetadata(PlayerID, "approachingTarget", undefined);
		this.ent.setMetadata(PlayerID, "alreadyTried", undefined);
		if (target.getMetadata(PlayerID, "inaccessibleTime"))
			target.setMetadata(PlayerID, "inaccessibleTime", 0);
		return false;
	}

	let inaccessibleTime = target.getMetadata(PlayerID, "inaccessibleTime");
	if (inaccessibleTime && gameState.ai.elapsedTime < inaccessibleTime)
		return true;

	let approachingTime = this.ent.getMetadata(PlayerID, "approachingTime");
	if (!approachingTime || gameState.ai.elapsedTime - approachingTime > 3)
	{
		let presentPos = this.ent.position();
		let approachingPos = this.ent.getMetadata(PlayerID, "approachingPos");
		if (!approachingPos || approachingPos[0] != presentPos[0] || approachingPos[1] != presentPos[1])
		{
			this.ent.setMetadata(PlayerID, "approachingTime", gameState.ai.elapsedTime);
			this.ent.setMetadata(PlayerID, "approachingPos", presentPos);
			return false;
		}
		if (gameState.ai.elapsedTime - approachingTime > 10)
		{
			if (this.ent.getMetadata(PlayerID, "alreadyTried"))
			{
				target.setMetadata(PlayerID, "inaccessibleTime", gameState.ai.elapsedTime + 600);
				return true;
			}
			// let's try again to reach it
			this.ent.setMetadata(PlayerID, "alreadyTried", targetId);
			this.ent.setMetadata(PlayerID, "approachingTarget", undefined);
			this.ent.gather(target);
			return false;
		}
	}
	return false;
};

return m;
}(PETRA);
