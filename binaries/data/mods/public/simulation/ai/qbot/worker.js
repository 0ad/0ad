/**
 * This class makes a worker do as instructed by the economy manager
 */

var Worker = function(ent) {
	this.ent = ent;
};

Worker.prototype.update = function(gameState) {
	var subrole = this.ent.getMetadata("subrole");
	
	if (subrole === "gatherer"){
		if (!(this.ent.unitAIState().split(".")[1] === "GATHER" && this.ent.unitAIOrderData().type 
				&& this.getResourceType(this.ent.unitAIOrderData().type) === this.ent.getMetadata("gather-type"))
				&& !(this.ent.unitAIState().split(".")[1] === "RETURNRESOURCE")){
			// TODO: handle combat for hunting animals
			Engine.ProfileStart("Start Gathering");
			this.startGathering(gameState);
			Engine.ProfileStop();
			
			//Engine.PostCommand({"type": "set-shading-color", "entities": [this.ent.id()], "rgb": [10,0,0]});
		}
	}else if(subrole === "builder"){
		if (this.ent.unitAIState().split(".")[1] !== "REPAIR"){
			var target = this.ent.getMetadata("target-foundation");
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
		if (this.gatheringFrom !== this.ent.unitAIOrderData().target){
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent){
					ent.setMetadata("gatherer-count", ent.getMetadata("gatherer-count") - 1);
					this.markFull(ent);
				}
			}
			this.gatheringFrom = this.ent.unitAIOrderData().target;
			if (this.gatheringFrom){
				var ent = gameState.getEntityById(this.gatheringFrom);
				if (ent){
					ent.setMetadata("gatherer-count", (ent.getMetadata("gatherer-count") || 0) + 1);
					this.markFull(ent);
				}
			}
		} 
	}else{
		if (this.gatheringFrom){
			var ent = gameState.getEntityById(this.gatheringFrom);
			if (ent){
				ent.setMetadata("gatherer-count", ent.getMetadata("gatherer-count") - 1);
				this.markFull(ent);
			}
			this.gatheringFrom = undefined;
		}
	}
};

Worker.prototype.markFull = function(ent){
	var maxCounts = {"food": 20, "wood": 5, "metal": 20, "stone": 20, "treasure": 1};
	if (ent.resourceSupplyType() && ent.getMetadata("gatherer-count") >= maxCounts[ent.resourceSupplyType().generic]){
		if (!ent.getMetadata("full")){
			ent.setMetadata("full", true);
		}
	}else{
		if (ent.getMetadata("full")){
			ent.setMetadata("full", false);
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
	
	// find closest dropsite which has nearby resources of the correct type
	var minDropsiteDist = Math.min(); // set to infinity initially
	var nearestResources = undefined;
	var nearestDropsite = undefined;
	
	gameState.updatingCollection("active-dropsite-" + resource, Filters.byMetadata("active-dropsite-" + resource, true), 
			gameState.getOwnDropsites(resource)).forEach(function (dropsite){
		if (dropsite.position()){
			var dist = VectorDistance(ent.position(), dropsite.position());
			if (dist < minDropsiteDist){
				minDropsiteDist = dist;
				nearestResources = dropsite.getMetadata("nearby-resources-" + resource);
				nearestDropsite = dropsite;
			}
		}
	});
	
	if (!nearestResources || nearestResources.length === 0){
		nearestResources = gameState.getResourceSupplies(resource);
		gameState.getOwnDropsites(resource).forEach(function (dropsite){
			if (dropsite.position()){
				var dist = VectorDistance(ent.position(), dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestDropsite = dropsite;
				}
			}
		});
	}
	
	if (nearestResources.length === 0){
		debug("No " + resource + " found! (1)");
		return;
	}
	
	var supplies = [];
	var nearestSupplyDist = Math.min();
	var nearestSupply = undefined;
	
	nearestResources.forEach(function(supply) {
		// TODO: handle enemy territories
		
		if (!supply.position()){
			return;
		}
		
		// measure the distance to the resource
		var dist = VectorDistance(supply.position(), ent.position());
		// Add on a factor for the nearest dropsite if one exists
		if (nearestDropsite){
			dist += 5 * VectorDistance(supply.position(), nearestDropsite.position());
		}
		
		// Go for treasure as a priority
		if (dist < 1000 && supply.resourceSupplyType().generic == "treasure"){
			dist /= 1000;
		}
		
		if (dist < nearestSupplyDist){
			nearestSupplyDist = dist;
			nearestSupply = supply;
		} 
	});
	
	if (nearestSupply) {
		if (!gameState.ai.accessibility.isAccessible(nearestSupply.position())){
			nearestSupply.setMetadata("inaccessible", true);
		}else{
			ent.gather(nearestSupply);
		}
	}else{
		debug("No " + resource + " found! (2)");
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