/**
 * This class makes a worker do as instructed by the economy manager
 */

var Worker = function(ent) {
	this.ent = ent;
	this.maxApproachTime = 45000;
	this.unsatisfactoryResource = false;	// if true we'll reguarly check if we can't have better now.
	this.baseID = 0;
};

Worker.prototype.update = function(baseManager, gameState) {
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
	
	// If we're fighting, let's not start gathering, heh?
	// TODO: remove this when we're hunting?
	if (this.ent.unitAIState().split(".")[1] === "COMBAT" || this.ent.getMetadata(PlayerID, "role") === "transport")
	{
		return;
	}
	
	if (subrole === "gatherer") {
		if (this.ent.isIdle()) {
			// if we aren't storing resources or it's the same type as what we're about to gather,
			// let's just pick a new resource.
			if (!this.ent.resourceCarrying() || this.ent.resourceCarrying().length === 0 ||
					this.ent.resourceCarrying()[0].type === this.ent.getMetadata(PlayerID, "gather-type")){
				Engine.ProfileStart("Start Gathering");
				this.unsatisfactoryResource = false;
				this.startGathering(baseManager,gameState);
				Engine.ProfileStop();

				this.startApproachingResourceTime = gameState.getTimeElapsed();

			} else {
				// Should deposit resources
				Engine.ProfileStart("Return Resources");
				if (!this.returnResources(gameState))
				{
					// no dropsite, abandon cargo.
					
					// if we were ordered to gather something else, try that.
					if (this.ent.resourceCarrying()[0].type !== this.ent.getMetadata(PlayerID, "gather-type"))
						this.startGathering(baseManager,gameState);
					else {
						// okay so we haven't found a proper dropsite for the resource we're supposed to gather
						// so let's get idle and the base manager will reassign us, hopefully well.
						this.ent.setMetadata(PlayerID, "gather-type",undefined);
						this.ent.setMetadata(PlayerID, "subrole", "idle");
						this.ent.stopMoving();
					}
				}
				Engine.ProfileStop();
			}
			// debug: show the resource we're gathering from
			//Engine.PostCommand({"type": "set-shading-color", "entities": [this.ent.id()], "rgb": [10,0,0]});
		} else if (this.ent.unitAIState().split(".")[1] === "GATHER") {
			
			// check for transport.
			if (gameState.ai.playedTurn % 5 === 0)
			{
				if (this.ent.unitAIOrderData().length && this.ent.unitAIState().split(".")[2] === "APPROACHING" && this.ent.unitAIOrderData()[0]["target"])
				{
					var ress = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
					if (ress !== undefined)
					{
						var index = gameState.ai.accessibility.getAccessValue(ress.position());
						var mIndex = gameState.ai.accessibility.getAccessValue(this.ent.position());
						if (index !== mIndex && index !== 1)
						{
							//gameState.ai.HQ.navalManager.askForTransport(this.ent.id(), this.ent.position(), ress.position());
						}
					}
				}
			}
			
			/*
			if (gameState.getTimeElapsed() - this.startApproachingResourceTime > this.maxApproachTime)
			{
				if (this.ent.unitAIOrderData().length && this.ent.unitAIState().split(".")[1] === "GATHER"
					&& this.ent.unitAIOrderData()[0]["target"])
				{
					var ent = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
					debug ("here " + this.startApproachingResourceAmount + "," + ent.resourceSupplyAmount());
					if (ent && this.startApproachingResourceAmount == ent.resourceSupplyAmount() && this.startEnt == ent.id()) {
						debug (ent.toString() + " is inaccessible");
						ent.setMetadata(PlayerID, "inaccessible", true);
						this.ent.flee(ent);
						this.ent.setMetadata(PlayerID, "subrole", "idle");
					}
				}
			}*/
			
			// we're gathering. Let's check that it's not a resource we'd rather not gather from.
			if ((this.ent.id() + gameState.ai.playedTurn) % 6 === 0 && this.checkUnsatisfactoryResource(gameState))
			{
				Engine.ProfileStart("Start Gathering");
				this.startGathering(baseManager,gameState);
				Engine.ProfileStop();
			}
			// TODO: reimplement the "reaching time" check.
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
		}
	} else if(subrole === "builder") {
		
		// check for transport.
		if (gameState.ai.playedTurn % 5 === 0)
		{
			if (this.ent.unitAIOrderData().length && this.ent.unitAIState().split(".")[2] === "APPROACHING" && this.ent.unitAIOrderData()[0]["target"])
			{
				var ress = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
				if (ress !== undefined)
				{
					var index = gameState.ai.accessibility.getAccessValue(ress.position());
					var mIndex = gameState.ai.accessibility.getAccessValue(this.ent.position());
					if (index !== mIndex && index !== 1)
					{
						//gameState.ai.HQ.navalManager.askForTransport(this.ent.id(), this.ent.position(), ress.position());
					}
				}
			}
		}

		
		if (this.ent.unitAIState().split(".")[1] !== "REPAIR") {
			var target = gameState.getEntityById(this.ent.getMetadata(PlayerID, "target-foundation"));
			// okay so apparently we aren't working.
			// Unless we've been explicitely told to keep our role, make us idle.
			if (!target || target.foundationProgress() === undefined && target.needsRepair() == false)
			{
				if (!this.ent.getMetadata(PlayerID, "keepSubrole"))
					this.ent.setMetadata(PlayerID, "subrole", "idle");
			}
			else
				this.ent.repair(target);
		}
		this.startApproachingResourceTime = gameState.getTimeElapsed();
		//Engine.PostCommand({"type": "set-shading-color", "entities": [this.ent.id()], "rgb": [0,10,0]});
		// TODO: we should maybe decide on our own to build other buildings, not rely on the assigntofoundation stuff.
	}  else if(subrole === "hunter") {
		if (this.ent.isIdle()){
			Engine.ProfileStart("Start Hunting");
			this.startHunting(gameState, baseManager);
			Engine.ProfileStop();
		}
	}
};

// check if our current resource is unsatisfactory
// this can happen in two ways:
//		-either we were on an unsatisfactory resource last time we started gathering (this.unsatisfactoryResource)
//		-Or we auto-moved to a bad resource thanks to the great UnitAI.
Worker.prototype.checkUnsatisfactoryResource = function(gameState) {
	if (this.unsatisfactoryResource)
		return true;
	if (this.ent.unitAIOrderData().length && this.ent.unitAIState().split(".")[1] === "GATHER" && this.ent.unitAIState().split(".")[2] === "GATHERING" && this.ent.unitAIOrderData()[0]["target"])
	{
		var ress = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
		if (!ress || !ress.getMetadata(PlayerID,"linked-dropsite") || !ress.getMetadata(PlayerID,"linked-dropsite-nearby") || gameState.ai.accessibility.getAccessValue(ress.position()) === -1)
			return true;
	}
	return false;
};

Worker.prototype.startGathering = function(baseManager, gameState) {
	var resource = this.ent.getMetadata(PlayerID, "gather-type");
	var ent = this.ent;
	var self = this;
	
	if (!ent.position()){
		// TODO: work out what to do when entity has no position
		return;
	}
	
	// TODO: this is not necessarily optimal.
	
	// find closest dropsite which has nearby resources of the correct type
	var minDropsiteDist = Math.min(); // set to infinity initially
	var nearestResources = undefined;
	var nearestDropsite = undefined;
	
	// first step: count how many dropsites we have that have enough resources "close" to them.
	// TODO: this is a huge part of multi-base support. Count only those in the same base as the worker.
	var number = 0;
	
	var ourDropsites = EntityCollectionFromIds(gameState,Object.keys(baseManager.dropsites));
	if (ourDropsites.length === 0)
	{
		debug ("We do not have a dropsite for " + resource + ", aborting");
		return;
	}
	
	var maxPerDP = 20;
	if (resource === "food")
		maxPerDP = 200;
	
	ourDropsites.forEach(function (dropsite) {
		if (baseManager.dropsites[dropsite.id()][resource] && baseManager.dropsites[dropsite.id()][resource][4] > 1000
			&& baseManager.dropsites[dropsite.id()][resource][5].length < maxPerDP)
			number++;
	});
	
	// Allright second step, if there are any such dropsites, we pick the closest.
	// we pick one with a lot of resource, or we pick the only one available (if it's high enough, otherwise we'll see with "far" below).
	if (number > 0)
	{
		ourDropsites.forEach(function (dropsite) { //}){
			if (baseManager.dropsites[dropsite.id()][resource] === undefined)
				return;
			if (dropsite.position() && (baseManager.dropsites[dropsite.id()][resource][4] > 1000 || (number === 1 && baseManager.dropsites[dropsite.id()][resource][4] > 200) )
				&& baseManager.dropsites[dropsite.id()][resource][5].length < maxPerDP) {
				var dist = SquareVectorDistance(ent.position(), dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestResources = baseManager.dropsites[dropsite.id()][resource][1];
					nearestDropsite = dropsite;
				}
			}
		});
	}
	// we've found no fitting dropsites close enough from us.
	// So'll try with far away.
	if (!nearestResources || nearestResources.length === 0) {
		ourDropsites.forEach(function (dropsite) { //}){
			if (baseManager.dropsites[dropsite.id()][resource] === undefined)
				return;
			if (dropsite.position() && baseManager.dropsites[dropsite.id()][resource][4] > 400
				&& baseManager.dropsites[dropsite.id()][resource][5].length < maxPerDP) {
				var dist = SquareVectorDistance(ent.position(), dropsite.position());
				if (dist < minDropsiteDist){
					minDropsiteDist = dist;
					nearestResources = baseManager.dropsites[dropsite.id()][resource][1];
					nearestDropsite = dropsite;
				}
			}
		});
	}

	if (!nearestResources || nearestResources.length === 0){
		if (resource === "food")
			if (this.buildAnyField(gameState))
				return;

		if (this.unsatisfactoryResource == true)
			return;	// we were already not satisfied, we're still not, change not.

		if (gameState.ai.HQ.switchWorkerBase(gameState, this.ent, resource))
			return;

		//debug ("No fitting dropsite for " + resource + " found, iterating the map.");
		nearestResources = gameState.getResourceSupplies(resource);
		this.unsatisfactoryResource = true;
		// TODO: should try setting up dropsites.
	} else {
		this.unsatisfactoryResource = false;
	}
	
	if (nearestResources.length === 0){
		if (resource === "food")
		{
			if (this.buildAnyField(gameState))
				return;
			if (gameState.ai.HQ.switchWorkerBase(gameState, this.ent, resource))
				return;
			debug("No " + resource + " found! (1)");
		}
		else
		{
			if (gameState.ai.HQ.switchWorkerBase(gameState, this.ent, resource))
				return;
			debug("No " + resource + " found! (1)");
		}
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
		
		if (supply.getMetadata(PlayerID, "inaccessible") === true) {
			//debug ("inaccessible");
			return;
		}

		if (supply.isFull() === true
			|| (gameState.turnCache["ressGathererNB"] && gameState.turnCache["ressGathererNB"][supply.id()]
				&& gameState.turnCache["ressGathererNB"][supply.id()] + supply.resourceSupplyGatherers().length >= supply.maxGatherers))
			return;

							 
		// Don't gather enemy farms or farms from another base
		if ((!supply.isOwn(PlayerID) && supply.owner() !== 0) || (supply.isOwn(PlayerID) && supply.getMetadata(PlayerID,"base") !== self.baseID)) {
			//debug ("enemy");
			return;
		}
		
		// quickscope accessbility check.
		if (!gameState.ai.accessibility.pathAvailable(gameState, ent.position(), supply.position())) {
			//debug ("nopath");
			return;
		}
		// some simple check for chickens: if they're in a square that's inaccessible, we won't gather from them.
		if (supply.footprintRadius() < 1)
		{
			var fakeMap = new Map(gameState.sharedScript,gameState.getMap().data);
			var id = fakeMap.gamePosToMapPos(supply.position())[0] + fakeMap.width*fakeMap.gamePosToMapPos(supply.position())[1];
			if ( (gameState.sharedScript.passabilityClasses["pathfinderObstruction"] & gameState.getMap().data[id]) )
			{
				supply.setMetadata(PlayerID, "inaccessible", true)
				return;
			}
		}

		// measure the distance to the resource (largely irrelevant)
		 var dist = SquareVectorDistance(supply.position(), ent.position());
							 
		if (dist > 4900 && supply.hasClass("Animal"))
		return;

		// Add on a factor for the nearest dropsite if one exists
		if (nearestDropsite !== undefined ){
			dist += 4*SquareVectorDistance(supply.position(), nearestDropsite.position());
			dist /= 5.0;
		}
					
		var territoryOwner = Map.createTerritoryMap(gameState).getOwner(supply.position());
		if (territoryOwner != PlayerID && territoryOwner != 0) {
			dist *= 5.0;
			//return;
		} else if (dist < 40000 && supply.resourceSupplyType().generic == "treasure"){
			// go for treasures if they're not in enemy territory
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
				// TODO: ought to change behavior here.
				return; // wait. a farm should appear.
			}
		}
		if (!tried) {
			if (!gameState.turnCache["ressGathererNB"])
			{
				gameState.turnCache["ressGathererNB"] = {};
				gameState.turnCache["ressGathererNB"][nearestSupply.id()] = 1;
			} else if (!gameState.turnCache["ressGathererNB"][nearestSupply.id()])
				gameState.turnCache["ressGathererNB"][nearestSupply.id()] = 1;
			else
				gameState.turnCache["ressGathererNB"][nearestSupply.id()]++;
			
			this.maxApproachTime = Math.max(30000, VectorDistance(pos,this.ent.position()) * 5000);
			this.startApproachingResourceAmount = ent.resourceSupplyAmount();
			this.startEnt = ent.id();
			ent.gather(nearestSupply);
			ent.setMetadata(PlayerID, "target-foundation", undefined);
		}
	} else {
		if (resource === "food" && this.buildAnyField(gameState))
			return;

		if (gameState.ai.HQ.switchWorkerBase(gameState, this.ent, resource))
			return;
		
		if (resource !== "food")
			debug("No " + resource + " found! (2)");
		// If we had a fitting closest dropsite with a lot of resources, mark it as not good. It means it's probably full. Then retry.
		// it'll be resetted next time it's counted anyway.
		if (nearestDropsite && nearestDropsite.getMetadata(PlayerID, "resource-quantity-" +resource)+nearestDropsite.getMetadata(PlayerID, "resource-quantity-far-" +resource) > 400)
		{
			nearestDropsite.setMetadata(PlayerID, "resource-quantity-" +resource, 0);
			nearestDropsite.setMetadata(PlayerID, "resource-quantity-far-" +resource, 0);
			this.startGathering(baseManager, gameState);
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

Worker.prototype.startHunting = function(gameState, baseManager){
	var ent = this.ent;
	
	if (!ent.position() || !baseManager.isHunting)
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
		
		if (supply.isFull() === true)
			return;
		
		if (!supply.hasClass("Animal"))
			return;
					  
		// measure the distance to the resource
		var dist = SquareVectorDistance(supply.position(), ent.position());

		var territoryOwner = Map.createTerritoryMap(gameState).getOwner(supply.position());
		if (territoryOwner != PlayerID && territoryOwner != 0) {
			dist *= 3.0;
		}
		
		// quickscope accessbility check
		if (!gameState.ai.accessibility.pathAvailable(gameState, ent.position(), supply.position(),false, true))
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
			baseManager.isHunting = false;
			ent.setMetadata(PlayerID, "role", undefined);
			debug ("No dropsite for hunting food");
			return;
		}
		if (minDropsiteDist > 35000) {
			baseManager.isHunting = false;
			ent.setMetadata(PlayerID, "role", undefined);
		} else {
			ent.gather(nearestSupply);
			ent.setMetadata(PlayerID, "target-foundation", undefined);
		}
	} else {
		baseManager.isHunting = false;
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

Worker.prototype.getGatherRate = function(gameState) {
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
		//debug (+rates[tstring] + " for " + tstring + " for " + this.ent._templateName);
		if (rates[tstring])
			return rates[tstring];
		return 0;
	}
	return 0;
};

Worker.prototype.buildAnyField = function(gameState){
	var self = this;
	var okay = false;
	var foundations = gameState.getOwnFoundations().filter(Filters.byMetadata(PlayerID,"base",this.baseID));
	foundations.filterNearest(this.ent.position(), foundations.length);
	foundations.forEach(function (found) {
		if (found._template.BuildRestrictions.Category === "Field" && !okay) {
			self.ent.repair(found);
			okay = true;
			return;
		}
	});
	if (!okay)
	{
		var foundations = gameState.getOwnFoundations();
		foundations.filterNearest(this.ent.position(), foundations.length);
		foundations.forEach(function (found) {
			if (found._template.BuildRestrictions.Category === "Field" && !okay) {
				self.ent.repair(found);
				self.ent.setMetadata(PlayerID,"base", found.getMetadata(PlayerID,"base"));
				okay = true;
				return;
			}
		});
	}
	return okay;
};
