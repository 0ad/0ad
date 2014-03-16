var AEGIS = function(m)
{

/**
 * This class makes a worker do as instructed by the economy manager
 */

m.Worker = function(ent) {
	this.ent = ent;
	this.maxApproachTime = 45000;
	this.unsatisfactoryResource = false;	// if true we'll reguarly check if we can't have better now.
	this.baseID = 0;
};

m.Worker.prototype.update = function(baseManager, gameState) {
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
					m.debug ("here " + this.startApproachingResourceAmount + "," + ent.resourceSupplyAmount());
					if (ent && this.startApproachingResourceAmount == ent.resourceSupplyAmount() && this.startEnt == ent.id()) {
						m.debug (ent.toString() + " is inaccessible");
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
						m.debug (ent.toString() + " is inaccessible");
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
					m.debug (ent.toString() + " is inaccessible from Combat");
					ent.setMetadata(PlayerID, "inaccessible", true);
					this.ent.flee(ent);
					this.ent.setMetadata(PlayerID, "subrole", "idle");
					this.gatheringFrom = undefined;
				}
			}*/
		}
	} else if(subrole === "builder") {
		
		// check for transport.
		/*if (gameState.ai.playedTurn % 5 === 0)
		{
			if (this.ent.unitAIOrderData().length && this.ent.unitAIState().split(".")[2] && this.ent.unitAIState().split(".")[2] === "APPROACHING" && this.ent.unitAIOrderData()[0]["target"])
			{
				var ress = gameState.getEntityById(this.ent.unitAIOrderData()[0]["target"]);
				if (ress !== undefined)
				{
					var index = gameState.ai.accessibility.getAccessValue(ress.position());
					var mIndex = gameState.ai.accessibility.getAccessValue(this.ent.position());
					if (index !== mIndex && index !== 1)
					{
						gameState.ai.HQ.navalManager.askForTransport(this.ent.id(), this.ent.position(), ress.position());
					}
				}
			}
		}*/
		
		if (this.ent.unitAIState().split(".")[1] !== "REPAIR") {
			var target = gameState.getEntityById(this.ent.getMetadata(PlayerID, "target-foundation"));
			// okay so apparently we aren't working.
			// Unless we've been explicitely told to keep our role, make us idle.
			if (!target || target.foundationProgress() === undefined && target.needsRepair() == false)
			{
				if (!this.ent.getMetadata(PlayerID, "keepSubrole"))
					this.ent.setMetadata(PlayerID, "subrole", "idle");
			} else
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
m.Worker.prototype.checkUnsatisfactoryResource = function(gameState) {
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

m.Worker.prototype.startGathering = function(baseManager, gameState) {
	var resource = this.ent.getMetadata(PlayerID, "gather-type");
	var ent = this.ent;
	var self = this;
	
	if (!ent.position()) {
		// TODO: work out what to do when entity has no position
		return;
	}
	
	/* Procedure for gathering resources
	 * Basically the AI focuses a lot on dropsites, and we will too
	 * So what we want is trying to find the best dropsites
	 * Traits: it needs to have a lot of resources available
	 *	-it needs to have room available for me
	 *  -it needs to be as close as possible (meaning base)
	 * Once we've found the best dropsite, we'll just pick a random close resource.
	 * TODO: we probably could pick something better. 
	 */

	var wantedDropsite = 0;
	var wantedDropsiteCoeff = 0;

	var forceFaraway = false;

	// So for now what I'll do is that I'll check dropsites in my base, then in other bases.

	for (var id in baseManager.dropsites)
	{
		var dropsiteInfo = baseManager.dropsites[id][resource];
		var capacity = baseManager.getDpWorkerCapacity(gameState, id, resource);
		if (capacity === undefined)	// presumably we're not ready
			continue;
		if (dropsiteInfo)
		{
			var coeff = dropsiteInfo[3] + (capacity-dropsiteInfo[5].length)*1000;
			if (gameState.getEntityById(id).hasClass("CivilCentre"))
				coeff += 20000;
			if (coeff > wantedDropsiteCoeff)
			{
				wantedDropsiteCoeff = coeff;
				wantedDropsite = id;
			}
		}
	}

	if (wantedDropsite === 0)
	{
		for (var id in baseManager.dropsites)
		{
			var dropsiteInfo = baseManager.dropsites[id][resource];
			var capacity = baseManager.getDpWorkerCapacity(gameState, id, resource, true);
			if (capacity === undefined)	// presumably we're not ready
				continue;
			if (dropsiteInfo)
			{
				var coeff = dropsiteInfo[4] + (capacity-dropsiteInfo[5].length)*100;
				if (coeff > wantedDropsiteCoeff)
				{
					wantedDropsiteCoeff = coeff;
					wantedDropsite = id;
					forceFaraway = true;
				}
			}
		}
	}
	// so if we're here we have checked our whole base for a proper dropsite.
	// for food, try to build fields if there are any.
	if (wantedDropsiteCoeff < 200 && resource === "food" && this.buildAnyField(gameState))
		return;

	// haven't found any, check in other bases
	// TODO: we should pick closest/most accessible bases first.
	if (wantedDropsiteCoeff < 200)
	{
		for each (var base in gameState.ai.HQ.baseManagers)
		{
			// TODO: check we can access that base, and/or pick the best base.
			if (base.ID === this.baseID || wantedDropsite !== 0)
				continue;
			for (var id in base.dropsites)
			{
				// if we have at least 1000 resources (including faraway) on this d
				var dropsiteInfo = base.dropsites[id][resource];
				var capacity = base.getDpWorkerCapacity(gameState, id, resource);
				if (capacity === undefined)	// presumably we're not ready
					continue;
				if (dropsiteInfo && dropsiteInfo[3] > 600 && dropsiteInfo[5].length < capacity)
				{
					// we want to change bases.
					this.ent.setMetadata(PlayerID,"base",base.ID);
					wantedDropsite = id;
					break;
				}
			}
		}
	}
	
	// I know, this is horrible code repetition.
	// TODO: avoid horrible code repetition

	// haven't found any, check in other bases
	// TODO: we should pick closest/most accessible bases first.
	if (wantedDropsiteCoeff < 200)
	{
		for each (var base in gameState.ai.HQ.baseManagers)
		{
			if (base.ID === this.baseID || wantedDropsite !== 0)
				continue;
			for (var id in base.dropsites)
			{
				// if we have at least 1000 resources (including faraway) on this d
				var dropsiteInfo = base.dropsites[id][resource];
				var capacity = baseManager.getDpWorkerCapacity(gameState, id, resource, true);
				if (capacity === undefined)	// presumably we're not ready
					continue;
				if (dropsiteInfo && dropsiteInfo[4] > 600 && dropsiteInfo[5].length < capacity)
				{
					this.ent.setMetadata(PlayerID,"base",base.ID);
					wantedDropsite = id;
					break;	// here I'll break, TODO.
				}
			}
		}
	}

	if (wantedDropsite === 0)
	{
		//TODO: something.
		// Okay so we haven't found any appropriate dropsite anywhere.
		m.debug("No proper dropsite found for " + resource + ", waiting.");
		return;
	} else
		this.pickResourceNearDropsite(gameState, resource, wantedDropsite, forceFaraway);
};


m.Worker.prototype.pickResourceNearDropsite = function(gameState, resource, dropsiteID, forceFaraway)
{
	// get the entity.
	var dropsite = gameState.getEntityById(dropsiteID);
	
	if (!dropsite)
		return false;

	// get the dropsite info
	var baseManager = this.ent.getMetadata(PlayerID,"base");
	baseManager = gameState.ai.HQ.baseManagers[baseManager];

	var dropsiteInfo = baseManager.dropsites[dropsiteID][resource];
	if (!dropsiteInfo)
		return false;

	var faraway = (forceFaraway === true) ? true : false;

	var capacity = baseManager.getDpWorkerCapacity(gameState, dropsiteID, resource, faraway);
	if (dropsiteInfo[5].length >= capacity || dropsiteInfo[3] < 200)
		faraway = true;

	var resources = (faraway === true) ? dropsiteInfo[1] : dropsiteInfo[0];

	var wantedSupply = 0;
	var wantedSupplyCoeff = Math.min();
	
	// Pick the best resource
	resources.forEach(function(supply) {
		if (!supply.position())
			return;
		
		if (supply.getMetadata(PlayerID, "inaccessible") === true)
			return;

		if (m.IsSupplyFull(gameState, supply) === true)
			return;
		
		// TODO: make a quick accessibility check for sanity
/*		if (!gameState.ai.accessibility.pathAvailable(gameState, ent.position(), supply.position())) {
			//m.debug ("nopath");
			return;
		}*/

		// some simple check for chickens: if they're in a square that's inaccessible, we won't gather from them.
		// TODO: make sure this works with rounding.
		if (supply.footprintRadius() < 1)
		{
			var fakeMap = new API3.Map(gameState.sharedScript,gameState.getMap().data);
			var id = fakeMap.gamePosToMapPos(supply.position())[0] + fakeMap.width*fakeMap.gamePosToMapPos(supply.position())[1];
			if ( (gameState.sharedScript.passabilityClasses["pathfinderObstruction"] & gameState.getMap().data[id]) )
			{
				supply.setMetadata(PlayerID, "inaccessible", true)
				return;
			}
		}

		// Factor in distance to the dropsite.
		var dist = API3.SquareVectorDistance(supply.position(), dropsite.position());
				
		var territoryOwner = m.createTerritoryMap(gameState).getOwner(supply.position());
		if (territoryOwner != PlayerID && territoryOwner != 0) {
			dist *= 5.0;
		} else if (dist < 40000 && supply.resourceSupplyType().generic == "treasure"){
			// go for treasures if they're not in enemy territory
			dist /= 1000;
		}

		if (dist < wantedSupplyCoeff) {
			wantedSupplyCoeff = dist;
			wantedSupply = supply;
		}
	});
	
	if (!wantedSupply)
	{
		if (resource === "food" && this.buildAnyField(gameState))
			return true;

		//m.debug("Found a proper dropsite for " + resource + " but apparently no resources are available.");
		return false;
	}

	var pos = wantedSupply.position();
		
	// add the worker to the turn cache
	m.AddTCGatherer(gameState,wantedSupply.id());
	// helper to check if it's accessible.
	this.maxApproachTime = Math.max(30000, API3.VectorDistance(pos,this.ent.position()) * 5000);
	this.startApproachingResourceAmount = wantedSupply.resourceSupplyAmount();
	// helper for unsatisfactory resource.
	this.startEnt = wantedSupply.id();
	this.ent.gather(wantedSupply);
	// sanity.
	this.ent.setMetadata(PlayerID, "linked-to-dropsite", dropsiteID);
	this.ent.setMetadata(PlayerID, "target-foundation", undefined);
	return true;
};

// Makes the worker deposit the currently carried resources at the closest dropsite
m.Worker.prototype.returnResources = function(gameState){
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
			var d = API3.SquareVectorDistance(self.ent.position(), dropsite.position());
			if (d < dist){
				dist = d;
				closestDropsite = dropsite;
			}
		}
	});
	
	if (!closestDropsite){
		m.debug("No dropsite found to deposit " + resource);
		return false;
	}
	
	this.ent.returnResources(closestDropsite);
	return true;
};

m.Worker.prototype.startHunting = function(gameState, baseManager){
	var ent = this.ent;
	
	if (!ent.position() || !baseManager.isHunting)
		return;
	
	// So here we're doing it basic. We check what we can hunt, we hunt it. No fancies.
	
	var resources = gameState.getResourceSupplies("food");
	
	if (resources.length === 0){
		m.debug("No food found to hunt!");
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
		var dist = API3.SquareVectorDistance(supply.position(), ent.position());

		var territoryOwner = m.createTerritoryMap(gameState).getOwner(supply.position());
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
				var dist = API3.SquareVectorDistance(pos, dropsite.position());
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
			m.debug ("No dropsite for hunting food");
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
		m.debug("No food found for hunting! (2)");
	}
};

m.Worker.prototype.getResourceType = function(type){
	if (!type || !type.generic){
		return undefined;
	}
	
	if (type.generic === "treasure"){
		return type.specific;
	}else{
		return type.generic;
	}
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
		//m.debug (+rates[tstring] + " for " + tstring + " for " + this.ent._templateName);
		if (rates[tstring])
			return rates[tstring];
		return 0;
	}
	return 0;
};

m.Worker.prototype.buildAnyField = function(gameState){
	var self = this;
	var foundations = gameState.getOwnFoundations();
	var baseFoundations = foundations.filter(API3.Filters.byMetadata(PlayerID,"base",this.baseID));
	
	var maxGatherers = gameState.getTemplate(gameState.applyCiv("structures/{civ}_field")).maxGatherers();

	var bestFarmEnt = undefined;
	var bestFarmCoeff = 10000000;
	baseFoundations.forEach(function (found) {
		if (found.hasClass("Field")) {
			var coeff = API3.SquareVectorDistance(found.position(), self.ent.position());
			if (found.getBuildersNb() && found.getBuildersNb() >= maxGatherers)
				return;
			if (coeff <= bestFarmCoeff)
			{
				bestFarmEnt = found;
				bestFarmCoeff = coeff;
			}
		}
	});
	if (bestFarmEnt !== undefined)
	{
		self.ent.repair(bestFarmEnt);
		return true;
	}
	foundations.forEach(function (found) {
		if (found.hasClass("Field")) {
			var coeff = API3.SquareVectorDistance(found.position(), self.ent.position());
			if (found.getBuildersNb() && found.getBuildersNb() >= found.maxGatherers())
				return;
			if (coeff <= bestFarmCoeff)
			{
				bestFarmEnt = found;
				bestFarmCoeff = coeff;
			}
		}
	});
	if (bestFarmEnt !== undefined)
	{
		self.ent.repair(bestFarmEnt);
		self.ent.setMetadata(PlayerID,"base", bestFarmEnt.getMetadata(PlayerID,"base"));
		this.startEnt = bestFarmEnt.id();
		self.ent.gather(bestFarmEnt,true);
		self.ent.setMetadata(PlayerID, "target-foundation", undefined);
		return true;
	}
	return false;
};

return m;
}(AEGIS);
