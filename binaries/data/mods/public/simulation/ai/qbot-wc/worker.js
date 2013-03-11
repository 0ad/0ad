/**
 * This class makes a worker do as instructed by the economy manager
 */

var Worker = function(ent) {
	this.ent = ent;
	this.maxApproachTime = 45000;
	this.unsatisfactoryResource = false;	// if true we'll reguarly check if we can't have better now.
};

Worker.prototype.update = function(gameState) {
	
	
	var subrole = this.ent.getMetadata(PlayerID, "subrole");

	if (!this.ent.position() || (this.ent.getMetadata(PlayerID,"fleeing") && gameState.getTimeElapsed() - this.ent.getMetadata(PlayerID,"fleeing") < 8000)){
		// If the worker has no position then no work can be done
		return;
	}
	if (this.ent.getMetadata(PlayerID,"fleeing"))
		this.ent.setMetadata(PlayerID,"fleeing", undefined);
	
	if (subrole === "gatherer") {
		if (this.ent.unitAIState().split(".")[1] !== "GATHER" && this.ent.unitAIState().split(".")[1] !== "COMBAT" && this.ent.unitAIState().split(".")[1] !== "RETURNRESOURCE"){
			// TODO: handle combat for hunting animals
			if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0 || 
					this.ent.resourceCarrying()[0].type === this.ent.getMetadata(PlayerID, "gather-type")){
				Engine.ProfileStart("Start Gathering");
				this.startGathering(gameState);
				Engine.ProfileStop();
			} else if (this.ent.unitAIState().split(".")[1] !== "RETURNRESOURCE") {
				// Should deposit resources
				Engine.ProfileStart("Return Resources");
				if (!this.returnResources(gameState))
				{
					// no dropsite, abandon cargo.
					
					// if we have a new order
					if (this.ent.resourceCarrying()[0].type !== this.ent.getMetadata(PlayerID, "gather-type"))
						this.startGathering(gameState);
					else {
						this.ent.setMetadata(PlayerID, "gather-type",undefined);
						this.ent.setMetadata(PlayerID, "subrole", "idle");
						this.ent.stopMoving();
					}
				}
				Engine.ProfileStop();
			}
			this.startApproachingResourceTime = gameState.getTimeElapsed();
			
			//Engine.PostCommand({"type": "set-shading-color", "entities": [this.ent.id()], "rgb": [10,0,0]});
		} else if (this.ent.unitAIState().split(".")[1] === "GATHER") {
			if (this.unsatisfactoryResource && (this.ent.id() + gameState.ai.playedTurn) % 20 === 0)
			{
				Engine.ProfileStart("Start Gathering");
				this.startGathering(gameState);
				Engine.ProfileStop();
			}
			/*if (gameState.getTimeElapsed() - this.startApproachingResourceTime > this.maxApproachTime) {
				if (this.gatheringFrom) {
					var ent = gameState.getEntityById(this.gatheringFrom);
					if ((ent && ent.resourceSupplyAmount() == ent.resourceSupplyMax())) {
						// if someone gathers from it, it's only that the pathfinder sucks.
						debug (ent.toString() + " is inaccessible");
						ent.setMetadata(PlayerID, "inaccessible", true);
						this.ent.flee(ent);
						this.ent.setMetadata(PlayerID, "subrole", "idle");
						this.gatheringFrom = undefined;
					}
				}
			}*/
		} else if (this.ent.unitAIState().split(".")[1] === "COMBAT") {
			/*if (gameState.getTimeElapsed() - this.startApproachingResourceTime > this.maxApproachTime) {
				var ent = gameState.getEntityById(this.ent.unitAIOrderData()[0].target);
				if (ent && !ent.isHurt()) {
					// if someone gathers from it, it's only that the pathfinder sucks.
					debug (ent.toString() + " is inaccessible from Combat");
					ent.setMetadata(PlayerID, "inaccessible", true);
					this.ent.flee(ent);
					this.ent.setMetadata(PlayerID, "subrole", "idle");
					this.gatheringFrom = undefined;
				}
			}*/
		} else {
			this.startApproachingResourceTime = gameState.getTimeElapsed();
		}
	} else if(subrole === "builder") {
		if (this.ent.unitAIState().split(".")[1] !== "REPAIR"){
			var target = this.ent.getMetadata(PlayerID, "target-foundation");
			if (target.foundationProgress() === undefined && target.needsRepair() == false)
				this.ent.setMetadata(PlayerID, "subrole", "idle");
			else
				this.ent.repair(target);
		}
		this.startApproachingResourceTime = gameState.getTimeElapsed();
		//Engine.PostCommand({"type": "set-shading-color", "entities": [this.ent.id()], "rgb": [0,10,0]});
	}  else if(subrole === "hunter") {
		if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0){
			Engine.ProfileStart("Start Hunting");
			this.startHunting(gameState);
			Engine.ProfileStop();
		}
	} else {
		this.startApproachingResourceTime = gameState.getTimeElapsed();
	}
	
	Engine.ProfileStart("Update Gatherer Counts");
	this.updateGathererCounts(gameState);
	Engine.ProfileStop();
};

Worker.prototype.updateGathererCounts = function(gameState, dead){
	// update gatherer counts for the resources
	if (this.ent.unitAIState().split(".")[1] === "GATHER" && !dead){
		if (this.gatheringFrom !== this.ent.unitAIOrderData()[0].target){
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent && ent.resourceSupplyType()){
					ent.setMetadata(PlayerID, "gatherer-count", ent.getMetadata(PlayerID, "gatherer-count") - 1);
					this.markFull(gameState,ent);
				}
			}
			this.gatheringFrom = this.ent.unitAIOrderData()[0].target;
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent && ent.resourceSupplyType()){
					ent.setMetadata(PlayerID, "gatherer-count", (ent.getMetadata(PlayerID, "gatherer-count") || 0) + 1);
					this.markFull(gameState,ent);
				}
			}
			this.startApproachingResourceTime = gameState.getTimeElapsed();
		}
	} else if (this.ent.unitAIState().split(".")[1] === "RETURNRESOURCE" && !dead) {
		// We remove us from the counting is we have no following order or its not "return to collected resource".
		if (this.ent.unitAIOrderData().length === 1) {
			var ent = gameState.getEntityById(this.gatheringFrom);
			if (ent && ent.resourceSupplyType()){
				ent.setMetadata(PlayerID, "gatherer-count", ent.getMetadata(PlayerID, "gatherer-count") - 1);
				this.markFull(gameState,ent);
			}
			this.gatheringFrom = undefined;
		} else if (!this.ent.unitAIOrderData()[1].target || this.gatheringFrom !== this.ent.unitAIOrderData()[1].target){
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent && ent.resourceSupplyType()){
					ent.setMetadata(PlayerID, "gatherer-count", ent.getMetadata(PlayerID, "gatherer-count") - 1);
					this.markFull(gameState,ent);
				}
			}
			this.gatheringFrom = undefined;
		}
	} else {
		if (this.gatheringFrom){
			var ent = gameState.getEntityById(this.gatheringFrom);
			if (ent && ent.resourceSupplyType()){
				ent.setMetadata(PlayerID, "gatherer-count", ent.getMetadata(PlayerID, "gatherer-count") - 1);
				this.markFull(gameState,ent);
			}
			this.gatheringFrom = undefined;
		}
	}
};

Worker.prototype.markFull = function(gameState,ent){
	var maxCounts = {"food": 15, "wood": 6, "metal": 15, "stone": 15, "treasure": 1};
	var resource = ent.resourceSupplyType().generic;
	if (ent.resourceSupplyType() && ent.getMetadata(PlayerID, "gatherer-count") >= maxCounts[resource]){
		if (!ent.getMetadata(PlayerID, "full")){
			ent.setMetadata(PlayerID, "full", true);
			// update the dropsite
			var dropsite = gameState.getEntityById(ent.getMetadata(PlayerID, "linked-dropsite"));
			if (dropsite == undefined || dropsite.getMetadata(PlayerID, "linked-resources-" + resource) === undefined)
				return;
			if (ent.getMetadata(PlayerID, "linked-dropsite-nearby") == true) {
				dropsite.setMetadata(PlayerID, "resource-quantity-" + resource, +dropsite.getMetadata(PlayerID, "resource-quantity-" + resource) - (+ent.getMetadata(PlayerID, "dp-update-value")));
				dropsite.getMetadata(PlayerID, "linked-resources-" + resource).updateEnt(ent);
				dropsite.getMetadata(PlayerID, "nearby-resources-" + resource).updateEnt(ent);
			} else {
				dropsite.setMetadata(PlayerID, "resource-quantity-far-" + resource, +dropsite.getMetadata(PlayerID, "resource-quantity-" + resource) - (+ent.getMetadata(PlayerID, "dp-update-value")));
				dropsite.getMetadata(PlayerID, "linked-resources-" + resource).updateEnt(ent);
			}
		}
	}else{
		if (ent.getMetadata(PlayerID, "full")){
			ent.setMetadata(PlayerID, "full", false);
			// update the dropsite
			var dropsite = gameState.getEntityById(ent.getMetadata(PlayerID, "linked-dropsite"));
			if (dropsite == undefined || dropsite.getMetadata(PlayerID, "linked-resources-" + resource) === undefined)
				return;
			if (ent.getMetadata(PlayerID, "linked-dropsite-nearby") == true) {
				dropsite.setMetadata(PlayerID, "resource-quantity-" + resource, +dropsite.getMetadata(PlayerID, "resource-quantity-" + resource) + ent.resourceSupplyAmount());
				dropsite.getMetadata(PlayerID, "linked-resources-" + resource).updateEnt(ent);
				dropsite.getMetadata(PlayerID, "nearby-resources-" + resource).updateEnt(ent);
			} else {
				dropsite.setMetadata(PlayerID, "resource-quantity-far-" + resource, +dropsite.getMetadata(PlayerID, "resource-quantity-" + resource) + ent.resourceSupplyAmount());
				dropsite.getMetadata(PlayerID, "linked-resources-" + resource).updateEnt(ent);
			}
		}
	}
};

Worker.prototype.startGathering = function(gameState){
	var resource = this.ent.getMetadata(PlayerID, "gather-type");
	var ent = this.ent;
	
	if (!ent.position()){
		// TODO: work out what to do when entity has no position
		return;
	}
	
	this.unsatisfactoryResource = false;

	// TODO: this is not necessarily optimal.
	
	// find closest dropsite which has nearby resources of the correct type
	var minDropsiteDist = Math.min(); // set to infinity initially
	var nearestResources = undefined;
	var nearestDropsite = undefined;
	
	// first step: count how many dropsites we have that have enough resources "close" to them.
	// TODO: this is a huge part of multi-base support. Count only those in the same base as the worker.
	var number = 0;
	var ourDropsites = gameState.getOwnDropsites(resource);
	
	if (ourDropsites.length === 0)
	{
		debug ("We do not have a dropsite for " + resource + ", aborting");
		return;
	}
	
	ourDropsites.forEach(function (dropsite) {
		if (dropsite.getMetadata(PlayerID, "linked-resources-" +resource) !== undefined
			&& dropsite.getMetadata(PlayerID, "resource-quantity-" +resource) !== undefined && dropsite.getMetadata(PlayerID, "resource-quantity-" +resource) > 200) {
			number++;
		}
	});
	
	//debug ("Available " +resource + " dropsites: " +ourDropsites.length);
	
	// Allright second step, if there are any such dropsites, we pick the closest.
	// we pick one with a lot of resource, or we pick the only one available (if it's high enough, otherwise we'll see with "far" below).
	if (number > 0)
	{
		ourDropsites.forEach(function (dropsite) { //}){
			if (dropsite.getMetadata(PlayerID, "resource-quantity-" +resource) == undefined)
				return;
			if (dropsite.position() && (dropsite.getMetadata(PlayerID, "resource-quantity-" +resource) > 700 || (number === 1 && dropsite.getMetadata(PlayerID, "resource-quantity-" +resource) > 200) ) ) {
				var dist = SquareVectorDistance(ent.position(), dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestResources = dropsite.getMetadata(PlayerID, "linked-resources-" + resource);
					nearestDropsite = dropsite;
				}
			}
		});
	}
	//debug ("Nearest dropsite: " +nearestDropsite);
	
	// Now if we have no dropsites, we repeat the process with resources "far" from dropsites but still linked with them.
	// I add the "close" value for code sanity.
	// Again, we choose a dropsite with a lot of resources left, or we pick the only one available (in this case whatever happens).
	if (!nearestResources || nearestResources.length === 0) {
		//debug ("here(1)");
		gameState.getOwnDropsites(resource).forEach(function (dropsite){ //}){
			var quantity = dropsite.getMetadata(PlayerID, "resource-quantity-" +resource)+dropsite.getMetadata(PlayerID, "resource-quantity-far-" +resource);
			if (dropsite.position() && (quantity) > 700 || number === 1) {
				var dist = SquareVectorDistance(ent.position(), dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestResources = dropsite.getMetadata(PlayerID, "linked-resources-" + resource);
					nearestDropsite = dropsite;
				}
			}
		});
		this.unsatisfactoryResource = true;
		//debug ("Nearest dropsite: " +nearestDropsite);
	}
	// If we still haven't found any fitting dropsite...
	// Then we'll just pick any resource, and we'll check for the closest dropsite to that one
	if (!nearestResources || nearestResources.length === 0){
		//debug ("No fitting dropsite for " + resource + " found, iterating the map.");
		nearestResources = gameState.getResourceSupplies(resource);
		this.unsatisfactoryResource = true;
	}
	
	if (nearestResources.length === 0){
		if (resource === "food")
		{
			if (this.buildAnyField(gameState))
				return;
			debug("No " + resource + " found! (1)");
		}
		else
			debug("No " + resource + " found! (1)");
		return;
	}
	//debug("Found " + nearestResources.length + "spots for " + resource);
	
	/*if (!nearestDropsite) {
		debug ("No dropsite for " +resource);
		return;
	}*/
	
	var supplies = [];
	var nearestSupplyDist = Math.min();
	var nearestSupply = undefined;
	
	// filter resources
	// TODo: add a bonus for resources with a lot of resources left, perhaps, to spread gathering?
	nearestResources.forEach(function(supply) { //}){

		// sanity check, perhaps sheep could be garrisoned?
		if (!supply.position()) {
			//debug ("noposition");
			return;
		}
		
		if (supply.isUnhuntable())
			return;

		if (supply.getMetadata(PlayerID, "inaccessible") === true) {
			//debug ("inaccessible");
			return;
		}
							 
		// too many workers trying to gather from this resource
		if (supply.getMetadata(PlayerID, "full") === true) {
			//debug ("full");
			return;
		}
							 
		// Don't gather enemy farms
		if (!supply.isOwn(PlayerID) && supply.owner() !== 0) {
			//debug ("enemy");
			return;
		}

		// quickscope accessbility check.
		if (!gameState.ai.accessibility.pathAvailable(gameState, ent.position(), supply.position(), true)) {
			//debug ("nopath");
			return;
		}
		// some simple check for chickens: if they're in a square that's inaccessible, we won't gather from them.
		if (supply.footprintRadius() < 1)
		{
			var fakeMap = new Map(gameState,gameState.getMap().data);
			var id = fakeMap.gamePosToMapPos(supply.position())[0] + fakeMap.width*fakeMap.gamePosToMapPos(supply.position())[1];
			if ( (gameState.sharedScript.passabilityClasses["pathfinderObstruction"] & gameState.getMap().data[id]) )
			{
				supply.setMetadata(PlayerID, "inaccessible", true)
				return;
			}
		}

		// measure the distance to the resource (largely irrelevant)
		 var dist = SquareVectorDistance(supply.position(), ent.position());
							 
		// Add on a factor for the nearest dropsite if one exists
		if (nearestDropsite !== undefined ){
			dist += 4*SquareVectorDistance(supply.position(), nearestDropsite.position());
			dist /= 5.0;
		}
					
		var territoryOwner = Map.createTerritoryMap(gameState).getOwner(supply.position());
		if (territoryOwner != PlayerID && territoryOwner != 0) {
			dist *= 3.0;
			//return;
		}
		
		// Go for treasure as a priority
		if (dist < 40000 && supply.resourceSupplyType().generic == "treasure"){
			dist /= 1000;
		}

		if (dist < nearestSupplyDist) {
			nearestSupplyDist = dist;
			nearestSupply = supply;
		} 
	});
	
	if (nearestSupply) {
		var pos = nearestSupply.position();
		
		// find a fitting dropsites in case we haven't already.
		if (!nearestDropsite) {
			ourDropsites.forEach(function (dropsite){ //}){
				if (dropsite.position()){
					var dist = SquareVectorDistance(pos, dropsite.position());
					if (dist < minDropsiteDist){
						minDropsiteDist = dist;
						nearestDropsite = dropsite;
					}
				}
			});
			if (!nearestDropsite)
			{
				debug ("No dropsite for " +resource);
				return;
			}
		}

		// if the resource is far away, try to build a farm instead.
		var tried = false;
		if (resource === "food" && SquareVectorDistance(pos,this.ent.position()) > 22500)
		{
			tried = this.buildAnyField(gameState);
			if (!tried && SquareVectorDistance(pos,this.ent.position()) > 62500) {
				return; // wait. a farm should appear.
			}
		}
		if (!tried) {
			this.maxApproachTime = Math.max(25000, VectorDistance(pos,this.ent.position()) * 1000);
			ent.gather(nearestSupply);
			ent.setMetadata(PlayerID, "target-foundation", undefined);
		}
	} else {
		if (resource === "food" && this.buildAnyField(gameState))
			return;

		debug("No " + resource + " found! (2)");
		// If we had a fitting closest dropsite with a lot of resources, mark it as not good. It means it's probably full. Then retry.
		// it'll be resetted next time it's counted anyway.
		if (nearestDropsite && nearestDropsite.getMetadata(PlayerID, "resource-quantity-" +resource)+nearestDropsite.getMetadata(PlayerID, "resource-quantity-far-" +resource) > 400)
		{
			nearestDropsite.setMetadata(PlayerID, "resource-quantity-" +resource, 0);
			nearestDropsite.setMetadata(PlayerID, "resource-quantity-far-" +resource, 0);
			this.startGathering(gameState);
		}
	}
};

// Makes the worker deposit the currently carried resources at the closest dropsite
Worker.prototype.returnResources = function(gameState){
	if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0){
		return true;	// assume we're OK.
	}
	var resource = this.ent.resourceCarrying()[0].type;
	var self = this;

	if (!this.ent.position()){
		// TODO: work out what to do when entity has no position
		return true;
	}
	
	var closestDropsite = undefined;
	var dist = Math.min();
	gameState.getOwnDropsites(resource).forEach(function(dropsite){
		if (dropsite.position()){
			var d = SquareVectorDistance(self.ent.position(), dropsite.position());
			if (d < dist){
				dist = d;
				closestDropsite = dropsite;
			}
		}
	});
	
	if (!closestDropsite){
		debug("No dropsite found to deposit " + resource);
		return false;
	}
	
	this.ent.returnResources(closestDropsite);
	return true;
};

Worker.prototype.startHunting = function(gameState){
	var ent = this.ent;
	
	if (!ent.position() || ent.getMetadata(PlayerID, "stoppedHunting"))
		return;
	
	// So here we're doing it basic. We check what we can hunt, we hunt it. No fancies.
	
	var resources = gameState.getResourceSupplies("food");
	
	if (resources.length === 0){
		debug("No food found to hunt!");
		return;
	}
	
	var supplies = [];
	var nearestSupplyDist = Math.min();
	var nearestSupply = undefined;
	
	resources.forEach(function(supply) { //}){
		if (!supply.position())
			return;
		
		if (supply.getMetadata(PlayerID, "inaccessible") === true)
			return;
		
		//if (supply.isFull === true)
		//	return;
		
		if (!supply.hasClass("Animal"))
			return;
		
		if (supply.walkSpeed() + 0.5 >= ent.walkSpeed())
			return;
					  
		// measure the distance to the resource
		var dist = SquareVectorDistance(supply.position(), ent.position());

		var territoryOwner = Map.createTerritoryMap(gameState).getOwner(supply.position());
		if (territoryOwner != PlayerID && territoryOwner != 0) {
			dist *= 3.0;
		}
		
		// quickscope accessbility check
		if (!gameState.ai.accessibility.pathAvailable(gameState, ent.position(), supply.position(), true))
			return;
		
		if (dist < nearestSupplyDist) {
			nearestSupplyDist = dist;
			nearestSupply = supply;
		}
	});
	
	if (nearestSupply) {
		var pos = nearestSupply.position();
		
		var nearestDropsite = 0;
		var minDropsiteDist = 1000000;
		// find a fitting dropsites in case we haven't already.
		gameState.getOwnDropsites("food").forEach(function (dropsite){ //}){
			if (dropsite.position()){
				var dist = SquareVectorDistance(pos, dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestDropsite = dropsite;
				}
			}
		});
		if (!nearestDropsite)
		{
			ent.setMetadata(PlayerID, "stoppedHunting", true);
			ent.setMetadata(PlayerID, "role", undefined);
			debug ("No dropsite for hunting food");
			return;
		}
		if (minDropsiteDist > 45000) {
			ent.setMetadata(PlayerID, "stoppedHunting", true);
			ent.setMetadata(PlayerID, "role", undefined);
		} else {
			ent.gather(nearestSupply);
			ent.setMetadata(PlayerID, "target-foundation", undefined);
		}
	} else {
		ent.setMetadata(PlayerID, "stoppedHunting", true);
		ent.setMetadata(PlayerID, "role", undefined);
		debug("No food found for hunting! (2)");
	}
};

Worker.prototype.getResourceType = function(type){
	if (!type || !type.generic){
		return undefined;
	}
	
	if (type.generic === "treasure"){
		return type.specific;
	}else{
		return type.generic;
	}
};

Worker.prototype.buildAnyField = function(gameState){
	var self = this;
	var okay = false;
	var foundations = gameState.getOwnFoundations();
	foundations.filterNearest(this.ent.position(), foundations.length);
	foundations.forEach(function (found) {
		if (found._template.BuildRestrictions.Category === "Field" && !okay) {
			self.ent.repair(found);
			okay = true;
			return;
		}
	});
	return okay;
};
