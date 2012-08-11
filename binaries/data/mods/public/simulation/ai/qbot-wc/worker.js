/**
 * This class makes a worker do as instructed by the economy manager
 */

var Worker = function(ent) {
	this.ent = ent;
	this.approachCount = 0;
};

Worker.prototype.update = function(gameState) {
	var subrole = this.ent.getMetadata("subrole");
	
	if (!this.ent.position()){
		// If the worker has no position then no work can be done
		return;
	}
	
	if (subrole === "gatherer"){
		if (!(this.ent.unitAIState().split(".")[1] === "GATHER" && this.ent.unitAIOrderData()[0].type
				&& this.getResourceType(this.ent.unitAIOrderData()[0].type) === this.ent.getMetadata("gather-type"))
				&& !(this.ent.unitAIState().split(".")[1] === "RETURNRESOURCE")){
			// TODO: handle combat for hunting animals
			if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0 || 
					this.ent.resourceCarrying()[0].type === this.ent.getMetadata("gather-type")){
				Engine.ProfileStart("Start Gathering");
				this.startGathering(gameState);
				Engine.ProfileStop();
			} else if (this.ent.unitAIState().split(".")[1] !== "RETURNRESOURCE") {
				// Should deposit resources
				Engine.ProfileStart("Return Resources");
				this.returnResources(gameState);
				Engine.ProfileStop();
			}
			
			this.startApproachingResourceTime = gameState.getTimeElapsed();
			
			//Engine.PostCommand({"type": "set-shading-color", "entities": [this.ent.id()], "rgb": [10,0,0]});
		}else{
			// If we haven't reached the resource in 1 minutes twice in a row and none of the resource has been 
			// gathered then mark it as inaccessible.
			if (gameState.getTimeElapsed() - this.startApproachingResourceTime > 60000){
				if (this.gatheringFrom){
					var ent = gameState.getEntityById(this.gatheringFrom);
					if (ent && ent.resourceSupplyAmount() == ent.resourceSupplyMax()){
						// if someone gathers from it, it's only that the pathfinder sucks.
						if (this.approachCount > 0 && ent.getMetadata("gatherer-count") <= 2){
							ent.setMetadata("inaccessible", true);
							this.ent.setMetadata("subrole", "idle");
							this.ent.flee(ent);
						} 
						this.approachCount++;
					}else{
						this.approachCount = 0;
					}
					
					this.startApproachingResourceTime = gameState.getTimeElapsed();
				}
			}
		}
	} else if(subrole === "builder") {
		if (this.ent.unitAIState().split(".")[1] !== "REPAIR"){
			var target = this.ent.getMetadata("target-foundation");
			if (target.foundationProgress() === undefined && target.needsRepair() == false)
				this.ent.setMetadata("subrole", "idle");
			else
				this.ent.repair(target);
		}
		//Engine.PostCommand({"type": "set-shading-color", "entities": [this.ent.id()], "rgb": [0,10,0]});
	}
	
	Engine.ProfileStart("Update Gatherer Counts");
	this.updateGathererCounts(gameState);
	Engine.ProfileStop();
};

Worker.prototype.updateGathererCounts = function(gameState, dead){
	// update gatherer counts for the resources
	if (this.ent.unitAIState().split(".")[2] === "GATHERING" && !dead){
		if (this.gatheringFrom !== this.ent.unitAIOrderData()[0].target){
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent && ent.resourceSupplyType()){
					ent.setMetadata("gatherer-count", ent.getMetadata("gatherer-count") - 1);
					this.markFull(gameState,ent);
				}
			}
			this.gatheringFrom = this.ent.unitAIOrderData()[0].target;
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent && ent.resourceSupplyType()){
					ent.setMetadata("gatherer-count", (ent.getMetadata("gatherer-count") || 0) + 1);
					this.markFull(gameState,ent);
				}
			}
		} 
	} else if (this.ent.unitAIState().split(".")[2] === "RETURNRESOURCE" && !dead) {
		// We remove us from the counting is we have no following order or its not "return to collected resource".
		if (this.ent.unitAIOrderData().length === 1) {
			var ent = gameState.getEntityById(this.gatheringFrom);
			if (ent && ent.resourceSupplyType()){
				ent.setMetadata("gatherer-count", ent.getMetadata("gatherer-count") - 1);
				this.markFull(gameState,ent);
			}
			this.gatheringFrom = undefined;
		}
		if (!this.ent.unitAIOrderData()[1].target || this.gatheringFrom !== this.ent.unitAIOrderData()[1].target){
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent && ent.resourceSupplyType()){
					ent.setMetadata("gatherer-count", ent.getMetadata("gatherer-count") - 1);
					this.markFull(gameState,ent);
				}
			}
			this.gatheringFrom = undefined;
		}
	} else {
		if (this.gatheringFrom){
			var ent = gameState.getEntityById(this.gatheringFrom);
			if (ent && ent.resourceSupplyType()){
				ent.setMetadata("gatherer-count", ent.getMetadata("gatherer-count") - 1);
				this.markFull(gameState,ent);
			}
			this.gatheringFrom = undefined;
		}
	}
};

Worker.prototype.markFull = function(gameState,ent){
	var maxCounts = {"food": 15, "wood": 6, "metal": 15, "stone": 15, "treasure": 1};
	var resource = ent.resourceSupplyType().generic;
	if (ent.resourceSupplyType() && ent.getMetadata("gatherer-count") >= maxCounts[resource]){
		if (!ent.getMetadata("full")){
			ent.setMetadata("full", true);
			// update the dropsite
			var dropsite = gameState.getEntityById(ent.getMetadata("linked-dropsite"));
			if (dropsite == undefined || dropsite.getMetadata("linked-resources-" + resource) === undefined)
				return;
			if (ent.getMetadata("linked-dropsite-nearby") == true) {
				dropsite.setMetadata("resource-quantity-" + resource, +dropsite.getMetadata("resource-quantity-" + resource) - (+ent.getMetadata("dp-update-value")));
				dropsite.getMetadata("linked-resources-" + resource).updateEnt(ent);
				dropsite.getMetadata("nearby-resources-" + resource).updateEnt(ent);
			} else {
				dropsite.setMetadata("resource-quantity-far-" + resource, +dropsite.getMetadata("resource-quantity-" + resource) - (+ent.getMetadata("dp-update-value")));
				dropsite.getMetadata("linked-resources-" + resource).updateEnt(ent);
			}
		}
	}else{
		if (ent.getMetadata("full")){
			ent.setMetadata("full", false);
			// update the dropsite
			var dropsite = gameState.getEntityById(ent.getMetadata("linked-dropsite"));
			if (dropsite == undefined || dropsite.getMetadata("linked-resources-" + resource) === undefined)
				return;
			if (ent.getMetadata("linked-dropsite-nearby") == true) {
				dropsite.setMetadata("resource-quantity-" + resource, +dropsite.getMetadata("resource-quantity-" + resource) + ent.resourceSupplyAmount());
				dropsite.getMetadata("linked-resources-" + resource).updateEnt(ent);
				dropsite.getMetadata("nearby-resources-" + resource).updateEnt(ent);
			} else {
				dropsite.setMetadata("resource-quantity-far-" + resource, +dropsite.getMetadata("resource-quantity-" + resource) + ent.resourceSupplyAmount());
				dropsite.getMetadata("linked-resources-" + resource).updateEnt(ent);
			}
		}
	}
};

Worker.prototype.startGathering = function(gameState){
	var resource = this.ent.getMetadata("gather-type");
	var ent = this.ent;
	
	if (!ent.position()){
		// TODO: work out what to do when entity has no position
		return;
	}
	
	// TODO: this is not necessarily optimal.
	
	// find closest dropsite which has nearby resources of the correct type
	var minDropsiteDist = Math.min(); // set to infinity initially
	var nearestResources = undefined;
	var nearestDropsite = undefined;
	
	// first, look for nearby resources.
	var number = 0;
	gameState.getOwnDropsites(resource).forEach(function (dropsite){ if (dropsite.getMetadata("linked-resources-" +resource) !== undefined
																		 && dropsite.getMetadata("linked-resources-" +resource).length > 3) { number++; } });

	gameState.getOwnDropsites(resource).forEach(function (dropsite){ //}){
		if (dropsite.getMetadata("resource-quantity-" +resource) == undefined)
			return;
		if (dropsite.position() && (dropsite.getMetadata("resource-quantity-" +resource) > 10 || number <= 1) ) {
			var dist = SquareVectorDistance(ent.position(), dropsite.position());
			if (dist < minDropsiteDist){
				minDropsiteDist = dist;
				nearestResources = dropsite.getMetadata("linked-resources-" + resource);
				nearestDropsite = dropsite;
			}
		}
	});
	// none, check even low level of resources and far away
	if (!nearestResources || nearestResources.length === 0){
		gameState.getOwnDropsites(resource).forEach(function (dropsite){ //}){
			if (dropsite.position() &&
			   (dropsite.getMetadata("resource-quantity-" +resource)+dropsite.getMetadata("resource-quantity-far-" +resource) > 10 || number <= 1)) {
				var dist = SquareVectorDistance(ent.position(), dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestResources = dropsite.getMetadata("linked-resources-" + resource);
					nearestDropsite = dropsite;
				}
			}
		});
	}
	// else, just get the closest to our closest dropsite.
	if (!nearestResources || nearestResources.length === 0){
		nearestResources = gameState.getResourceSupplies(resource);
		gameState.getOwnDropsites(resource).forEach(function (dropsite){
			if (dropsite.position()){
				var dist = SquareVectorDistance(ent.position(), dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestDropsite = dropsite;
				}
			}
		});
	}
	
	if (nearestResources.length === 0){
		if (resource === "food" && !this.buildAnyField(gameState))	// try to go build a farm
			debug("No " + resource + " found! (1)");
		else
			debug("No " + resource + " found! (1)");
		return;
	}
	
	if (!nearestDropsite) {
		debug ("No dropsite for " +resource);
		return;
	}
	
	var supplies = [];
	var nearestSupplyDist = Math.min();
	var nearestSupply = undefined;
	
	nearestResources.forEach(function(supply) { //}){
		// TODO: handle enemy territories
		
		if (!supply.position()){
			return;
		}
		// measure the distance to the resource (largely irrelevant)
		 var dist = SquareVectorDistance(supply.position(), ent.position());
							 
		// Add on a factor for the nearest dropsite if one exists
		if (supply.getMetadata("linked-dropsite") !== undefined){
			dist += 4*SquareVectorDistance(supply.position(), nearestDropsite.position());
			dist /= 5.0;
		}
		
		// Go for treasure as a priority
		if (dist < 40000 && supply.resourceSupplyType().generic == "treasure"){
			dist /= 1000;
		}
		
		if (dist < nearestSupplyDist){
			nearestSupplyDist = dist;
			nearestSupply = supply;
		} 
	});
	
	if (nearestSupply) {
		var pos = nearestSupply.position();
		
		// if the resource is far away, try to build a farm instead.
		var tried = false;
		if (resource === "food" && SquareVectorDistance(pos,this.ent.position()) > 22500)
			tried = this.buildAnyField(gameState);
		if (!tried && SquareVectorDistance(pos,this.ent.position()) > 62500) {
			return; // wait. a farm should appear.
		}
		if (!tried) {
			var territoryOwner = gameState.getTerritoryMap().getOwner(pos);
			if (!gameState.ai.accessibility.isAccessible(pos) ||
					(territoryOwner != gameState.getPlayerID() && territoryOwner != 0)){
				nearestSupply.setMetadata("inaccessible", true);
			}else{
				ent.gather(nearestSupply);
			}
		}
	}else{
		debug("No " + resource + " found! (2)");
	}
};

// Makes the worker deposit the currently carried resources at the closest dropsite
Worker.prototype.returnResources = function(gameState){
	if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0){
		return;
	}
	var resource = this.ent.resourceCarrying()[0].type;
	var self = this;

	if (!this.ent.position()){
		// TODO: work out what to do when entity has no position
		return;
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
		debug("No dropsite found for " + resource);
		return;
	}
	
	this.ent.returnResources(closestDropsite);
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
