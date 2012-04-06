/**
 * This class makes a worker do as instructed by the economy manager
 */

var Worker = function(ent) {
	this.ent = ent;
};

Worker.prototype.update = function(gameState) {
	var subrole = this.ent.getMetadata("subrole");
	
	if (subrole === "gatherer"){
		if (!(this.ent.unitAIState().split(".")[1] === "GATHER" && this.getResourceType(this.ent.unitAIOrderData().type) === this.ent.getMetadata("gather-type"))
				&& !(this.ent.unitAIState().split(".")[1] === "RETURNRESOURCE")){
			// TODO: handle combat for hunting animals
			Engine.ProfileStart("Start Gathering");
			this.startGathering(gameState);
			Engine.ProfileStop();
		}
	}else if(subrole === "builder"){
		if (this.ent.unitAIState().split(".")[1] !== "REPAIR"){
			var target = this.ent.getMetadata("target-foundation");
			this.ent.repair(target);
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
	
	gameState.updatingCollection("active-dropsite-" + resource, Filters.byMetadata("active-dropsite" + resource, true), 
			gameState.getOwnDropsites(resource)).forEach(function (dropsite){
				if (dropsite.position()){
					var dist = VectorDistance(ent.postion(), dropsite.position());
					if (dist < minDropsiteDist){
						minDropsiteDist = dist;
						nearestResources = dropsite.getMetadata("nearby-resources-" + type);
						nearestDropsite = dropsite;
					}
				}
			});
	
	if (!nearestResources){
		nearestResources = gameState.getResourceSupplies(resource);
	}
	
	if (nearestResources.length === 0){
		warn("No " + resource + " found! (1)");
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
		if (dist < 200 && supply.resourceSupplyType().generic == "treasure"){
			dist /= 1000;
		}

		// Skip targets that are far too far away (e.g. in the
		// enemy base), only do this for common supplies
		if (dist > 600){
			return;
		}
		
		if (dist < nearestSupplyDist){
			nearestSupplyDist = dist;
			nearestSupply = supply;
		} 
	});
	
	if (nearestSupply) {
		ent.gather(nearestSupply);
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