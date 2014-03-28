var PETRA = function(m)
{

/**
 * This class makes a worker do as instructed by the economy manager
 */

m.Worker = function(ent) {
	this.ent = ent;
	this.baseID = 0;
	this.lastUpdate = undefined;
};

m.Worker.prototype.update = function(baseManager, gameState) {
	this.lastUpdate = gameState.ai.playedTurn;
	this.baseID = baseManager.ID;
	var subrole = this.ent.getMetadata(PlayerID, "subrole");

	if (!this.ent.position() || (this.ent.getMetadata(PlayerID,"fleeing") && gameState.getTimeElapsed() - this.ent.getMetadata(PlayerID,"fleeing") < 8000)){
		// If the worker has no position then no work can be done
		return;
	}
	if (this.ent.getMetadata(PlayerID,"fleeing"))
		this.ent.setMetadata(PlayerID,"fleeing", undefined);
	
	// Okay so we have a few tasks.
	// If we're gathering, we'll check that we haven't run idle.
	// ANd we'll also check that we're gathering a resource we want to gather.
	
	// If we're fighting or hunting, let's not start gathering, heh?
	if (this.ent.unitAIState().split(".")[1] === "COMBAT" || this.ent.getMetadata(PlayerID, "role") === "transport")
		return;
	
/*	if (this.ent.unitAIState() == "INDIVIDUAL.IDLE" && subrole != "hunter")
	{
		var role = this.ent.getMetadata(PlayerID, "role");
		var base = this.ent.getMetadata(PlayerID, "base");
		var founda = this.ent.getMetadata(PlayerID, "target-foundation");
	    warn(" unit idle " + this.ent.id() + " role " + role + " subrole " + subrole + " base " + base + " founda " + founda);
	} */

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
				this.startGathering(gameState, baseManager);
			}
			else if (!this.returnResources(gameState))     // try to deposit resources
			{
				// no dropsite, abandon old resources and start gathering new ones
				this.startGathering(gameState, baseManager);
			}
		}
		else if (this.ent.unitAIState().split(".")[1] === "GATHER")
		{
			// we're already gathering. But let's check from time to time if there is nothing better
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
						this.startGathering(gameState, baseManager);
					}
					else
					{
						var gatherType = this.ent.getMetadata(PlayerID, "gather-type");
						var nearby = baseManager.dropsiteSupplies[gatherType]["nearby"];
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
							this.startGathering(gameState, baseManager);
						}
					}
				}
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
				this.moveAway(baseManager, gameState);
		}
		else
		{
//			if (target && target.foundationProgress() === undefined && target.needsRepair() === true)
//				warn(" target " + target.id() + " needs repair " + target.hitpoints() + " max " + target.maxHitpoints());
			this.ent.repair(target);
		}
	}
	else if (subrole === "hunter")
	{
		if (this.ent.isIdle())
			this.startHunting(gameState, baseManager);
		else	// if we have drifted towards ennemy territory during the hunt, go home
		{
			var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(this.ent.position());
			if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
				this.startHunting(gameState, baseManager);
		}
	}
};

m.Worker.prototype.startGathering = function(gameState, baseManager)
{
	if (!this.ent.position())	// TODO: work out what to do when entity has no position
		return false;

	var resource = this.ent.getMetadata(PlayerID, "gather-type");

	// Then if we are gathering food, try to hunt
	if (resource === "food" && this.startHunting(gameState, baseManager))
		return true;
	
	var foundSupply = function(ent, supply) {
		var ret = false;
		for (var i = 0; i < supply.length; ++i)
		{
			// exhausted resource, remove it from this list
			if (!supply[i].ent || !gameState.getEntityById(supply[i].id))
			{
				supply.splice(i--, 1);
				continue;
			}
			if (m.IsSupplyFull(gameState, supply[i].ent) === true)
				continue;
			// check if available resource is worth one additionnal gatherer (except for farms)
			var nbGatherers = supply[i].ent.resourceSupplyGatherers().length
				+ m.GetTCGatherer(gameState, supply[i].id);
			if (supply[i].ent.resourceSupplyType()["specific"] !== "grain"
				&& nbGatherers > 0 && supply[i].ent.resourceSupplyAmount()/(1+nbGatherers) < 40)
				continue;
			// not in ennemy territory
			var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supply[i].ent.position());
			if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
				continue;
			m.AddTCGatherer(gameState, supply[i].id);
			ent.gather(supply[i].ent);
			ent.setMetadata(PlayerID, "supply", supply[i].id);
			ret = true;
			break;
		}
		return ret;
	};

	var nearby = baseManager.dropsiteSupplies[resource]["nearby"];
	if (foundSupply(this.ent, nearby))
		return true;

	// --> for food, try to gather from fields if any, otherwise build one if any
	if (resource === "food" && (this.gatherNearestField(gameState) || this.buildAnyField(gameState)))
		return true;

	var medium = baseManager.dropsiteSupplies[resource]["medium"];
	if (foundSupply(this.ent, medium))
		return true;

	// So if we're here we have checked our whole base for a proper resource without success
	// --> check other bases before going back to faraway resources
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		var nearby = base.dropsiteSupplies[resource]["nearby"];
		if (foundSupply(this.ent, nearby))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			return true;
		}
	}
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		var medium = base.dropsiteSupplies[resource]["medium"];
		if (foundSupply(this.ent, medium))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			return true;
		}
	}


	// Okay so we haven't found any appropriate dropsite anywhere.
	// Try to help building one if any foundation available in the same base
	var self = this;
	var foundations = gameState.getOwnFoundations().toEntityArray();
	var shouldBuild = foundations.some(function(foundation) {
		if (!foundation || foundation.getMetadata(PlayerID, "base") != self.baseID)
			return false;
		if ((resource !== "food" && foundation.hasClass("Storehouse")) ||
			(resource === "food" && foundation.hasClass("DropsiteFood")))
		{
			self.ent.repair(foundation);
			return true;
		}
		return false;
	});
	if (shouldBuild)
		return true;

	// Still nothing, we look now for faraway resources, first in this base, then in others
	var faraway = baseManager.dropsiteSupplies[resource]["faraway"];
	if (foundSupply(this.ent, faraway))
		return true;
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (base.ID === this.baseID)
			continue;
		var faraway = base.dropsiteSupplies[resource]["faraway"];
		if (foundSupply(this.ent, faraway))
		{
			this.ent.setMetadata(PlayerID, "base", base.ID);
			return true;
		}
	}

	if (gameState.ai.HQ.Config.debug > 0)
	{
		warn(" >>>>> worker with gather-type " + resource + " with nothing to gather ");
	}

	return false;
};

// Makes the worker deposit the currently carried resources at the closest dropsite
m.Worker.prototype.returnResources = function(gameState)
{
	if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0 || !this.ent.position())
		return false;

	var resource = this.ent.resourceCarrying()[0].type;
	var self = this;

	var closestDropsite = undefined;
	var dist = Math.min();
	gameState.getOwnDropsites(resource).forEach(function(dropsite){
		if (dropsite.position())
		{
			var d = API3.SquareVectorDistance(self.ent.position(), dropsite.position());
			if (d < dist)
			{
				dist = d;
				closestDropsite = dropsite;
			}
		}
	});
	
	if (!closestDropsite)
		return false;	
	this.ent.returnResources(closestDropsite);
	return true;
};

m.Worker.prototype.startHunting = function(gameState, baseManager)
{
	if (!this.ent.position())
		return false;
	
	// So here we're doing it basic. We check what we can hunt, we hunt it. No fancies.
	
	var resources = gameState.getHuntableSupplies();	
	if (resources.length === 0)
		return false;

	var nearestSupplyDist = Math.min();
	var nearestSupply = undefined;

	var isCavalry = this.ent.hasClass("Cavalry");
	var isRanged = this.ent.hasClass("Ranged");
	var entPosition = this.ent.position();

	var nearestDropsiteDist = function(supply){
		var distMin = 1000000;
		var pos = supply.position();
		gameState.getOwnDropsites("food").forEach(function (dropsite){
			if (!dropsite.position())
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
		
		if (supply.getMetadata(PlayerID, "inaccessible") === true)
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
		if (!gameState.ai.accessibility.pathAvailable(gameState, entPosition, supply.position(),false, true))
			return;
					  
		// measure the distance to the resource
		var dist = API3.SquareVectorDistance(entPosition, supply.position());
		// Only cavalry should hunt faraway
		if (!isCavalry && dist > 25000)
			return;

		// some simple check for chickens: if they're in a inaccessible square, we won't gather from them.
		// TODO: make sure this works with rounding.
		if (supply.footprintRadius() < 1)
		{
			var fakeMap = new API3.Map(gameState.sharedScript, gameState.getMap().data);
			var id = fakeMap.gamePosToMapPos(supply.position())[0] + fakeMap.width*fakeMap.gamePosToMapPos(supply.position())[1];
			if ((gameState.sharedScript.passabilityClasses["pathfinderObstruction"] & gameState.getMap().data[id]))
			{
				supply.setMetadata(PlayerID, "inaccessible", true)
				return;
			}
		}

		// Avoid ennemy territory
		var territoryOwner = gameState.ai.HQ.territoryMap.getOwner(supply.position());
		if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))  // player is its own ally
			return;

		var dropsiteDist = nearestDropsiteDist(supply);
		if (dropsiteDist > 35000)
			return;
		// Only cavalry should hunt faraway (specially for non domestic animals which flee)
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
		m.AddTCGatherer(gameState, nearestSupply.id());
		this.ent.gather(nearestSupply);
		this.ent.setMetadata(PlayerID, "supply", nearestSupply.id());
		this.ent.setMetadata(PlayerID, "target-foundation", undefined);
		return true;
	}
	else
	{
		if (this.ent.getMetadata(PlayerID,"subrole") === "hunter")
			this.ent.setMetadata(PlayerID, "subrole", "idle");
		return false;
	}
};

m.Worker.prototype.getResourceType = function(type){
	if (!type || !type.generic)
		return undefined;
	
	if (type.generic === "treasure")
		return type.specific;
	else
		return type.generic;
};

m.Worker.prototype.getGatherRate = function(gameState) {
	if (this.ent.getMetadata(PlayerID,"subrole") !== "gatherer")
		return 0;
	var rates = this.ent.resourceGatherRates();
	
	if (this.ent.unitAIOrderData().length && this.ent.unitAIState().split(".")[1] === "GATHER" && this.ent.unitAIOrderData()[0]["target"])
	{
		var ress = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
		if (!ress)
			return 0;
		var type = ress.resourceSupplyType();
		if (type.generic == "treasure")
			return 1000;
		var tstring = type.generic + "." + type.specific;
		if (rates[tstring])
			return rates[tstring];
		return 0;
	}
	return 0;
};

m.Worker.prototype.gatherNearestField = function(gameState){
	if (!this.ent.position())
		return false;

	var self = this;
	var ownFields = gameState.getOwnEntitiesByType(gameState.applyCiv("structures/{civ}_field"), true);
	var bestFarmEnt = undefined;
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
	if (bestFarmEnt !== undefined)
	{
		this.ent.setMetadata(PlayerID, "base", bestFarmEnt.getMetadata(PlayerID, "base"));
		m.AddTCGatherer(gameState, bestFarmEnt.id());
		this.ent.gather(bestFarmEnt);
		this.ent.setMetadata(PlayerID, "supply", bestFarmEnt.id());
		return true;
	}
	return false;
};

/**
 * WARNING with the present options of AI orders, the unit will not gather after building the farm.
 * This is done by calling the gatherNearestField function when construction is completed. 
 */
m.Worker.prototype.buildAnyField = function(gameState){
	var self = this;
	var foundations = gameState.getOwnFoundations();
	var baseFoundations = foundations.filter(API3.Filters.byMetadata(PlayerID, "base", this.baseID));
	
	var maxGatherers = gameState.getTemplate(gameState.applyCiv("structures/{civ}_field")).maxGatherers();

	var bestFarmEnt = undefined;
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
	if (bestFarmEnt !== undefined)
	{
		this.ent.repair(bestFarmEnt);
		return true;
	}
	// No farms found, search in other bases
	foundations.forEach(function (found) {
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
	if (bestFarmEnt !== undefined)
	{
		this.ent.repair(bestFarmEnt);
		this.ent.setMetadata(PlayerID, "base", bestFarmEnt.getMetadata(PlayerID, "base"));
		return true;
	}
	return false;
};

// Workers elephant should move away from the buildings they've built to avoid being trapped in between constructions
// For the time being, we move towards the nearest gatherer (providing him a dropsite)
m.Worker.prototype.moveAway = function(baseManager, gameState){
	var gatherers = baseManager.workersBySubrole(gameState, "gatherer").toEntityArray();
	var pos = this.ent.position();
	var dist = Math.min();
	var destination = pos;
	for (var i = 0; i < gatherers.length; ++i)
	{
		if (gatherers[i].isIdle())
			continue;
		var distance = API3.SquareVectorDistance(pos, gatherers[i].position());
		if (distance > dist)
			continue;
		dist = distance;
		destination = gatherers[i].position();
	}
	this.ent.move(destination[0], destination[1]);
};

return m;
}(PETRA);
