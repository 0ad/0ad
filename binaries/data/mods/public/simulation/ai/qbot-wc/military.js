/*
 * Military Manager.
 * Basically this deals with constructing defense and attack buildings, but it's not very developped yet.
 * There's a lot of work still to do here.
 * It also handles the attack plans (see attack_plan.js)
 * Not completely cleaned up from the original version in qBot.
 */

var MilitaryAttackManager = function() {
	
	this.fortressStartTime = 0;
	this.fortressLapseTime = Config.Military.fortressLapseTime * 1000;
	this.defenceBuildingTime = Config.Military.defenceBuildingTime * 1000;
	this.attackPlansStartTime = Config.Military.attackPlansStartTime * 1000;
	this.defenceManager = new Defence();
	
	
	this.TotalAttackNumber = 0;
	this.upcomingAttacks = { "CityAttack" : [] };
	this.startedAttacks = { "CityAttack" : [] };
};

MilitaryAttackManager.prototype.init = function(gameState) {
	var civ = gameState.playerData.civ;
	
	// load units and buildings from the config files
	
	if (civ in Config.buildings.moderate){
		this.bModerate = Config.buildings.moderate[civ];
	}else{
		this.bModerate = Config.buildings.moderate['default'];
	}
	
	if (civ in Config.buildings.advanced){
		this.bAdvanced = Config.buildings.advanced[civ];
	}else{
		this.bAdvanced = Config.buildings.advanced['default'];
	}
	
	if (civ in Config.buildings.fort){
		this.bFort = Config.buildings.fort[civ];
	}else{
		this.bFort = Config.buildings.fort['default'];
	}

	for (var i in this.bAdvanced){
		this.bAdvanced[i] = gameState.applyCiv(this.bAdvanced[i]);
	}
	for (var i in this.bFort){
		this.bFort[i] = gameState.applyCiv(this.bFort[i]);
	}
	
	// TODO: figure out how to make this generic
	for (var i in this.attackManagers){
		this.availableAttacks[i] = new this.attackManagers[i](gameState, this);
	}
	
	var enemies = gameState.getEnemyEntities();
	var filter = Filters.byClassesOr(["CitizenSoldier", "Champion", "Hero", "Siege"]);
	this.enemySoldiers = enemies.filter(filter); // TODO: cope with diplomacy changes
	this.enemySoldiers.registerUpdates();
	
	// each enemy watchers keeps a list of entity collections about the enemy it watches
	// It also keeps track of enemy armies, merging/splitting as needed
	this.enemyWatchers = {};
	this.ennWatcherIndex = [];
	for (var i = 1; i <= 8; i++)
		if (PlayerID != i && gameState.isPlayerEnemy(i)) {
			this.enemyWatchers[i] = new enemyWatcher(gameState, i);
			this.ennWatcherIndex.push(i);
			this.defenceManager.enemyArmy[i] = [];
		}

};

// picks the best template based on parameters and classes
MilitaryAttackManager.prototype.findBestTrainableUnit = function(gameState, classes, parameters) {
	var units = gameState.findTrainableUnits(classes);
	
	if (units.length === 0)
		return undefined;
	
	
	units.sort(function(a, b) { //}) {
		var aDivParam = 0, bDivParam = 0;
		var aTopParam = 0, bTopParam = 0;
		for (var i in parameters) {
			var param = parameters[i];
			
			if (param[0] == "base") {
				aTopParam = param[1];
				bTopParam = param[1];
			}
			if (param[0] == "strength") {
				aTopParam += getMaxStrength(a[1]) * param[1];
				bTopParam += getMaxStrength(b[1]) * param[1];
			}
			if (param[0] == "siegeStrength") {
				aTopParam += getMaxStrength(a[1], "Structure") * param[1];
				bTopParam += getMaxStrength(b[1], "Structure") * param[1];
			}
			if (param[0] == "speed") {
				aTopParam += a[1].walkSpeed() * param[1];
				bTopParam += b[1].walkSpeed() * param[1];
			}
			
			if (param[0] == "cost") {
				aDivParam += a[1].costSum() * param[1];
				bDivParam += b[1].costSum() * param[1];
			}
			if (param[0] == "canGather") {
				// checking against wood, could be anything else really.
				if (a[1].resourceGatherRates() && a[1].resourceGatherRates()["wood.tree"])
					aTopParam *= param[1];
				if (b[1].resourceGatherRates() && b[1].resourceGatherRates()["wood.tree"])
					bTopParam *= param[1];
			}
		}
		return -(aTopParam/(aDivParam+1)) + (bTopParam/(bDivParam+1));
	});
	return units[0][0];
};

// Deals with building fortresses and towers.
// Currently build towers next to every useful dropsites.
// TODO: Fortresses are placed randomly atm.
MilitaryAttackManager.prototype.buildDefences = function(gameState, queues){ 

	var workersNumber = gameState.getOwnEntitiesByRole("worker").filter(Filters.not(Filters.byHasMetadata(PlayerID,"plan"))).length;

	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv('structures/{civ}_defense_tower'))
		+ queues.defenceBuilding.totalLength() < gameState.getEntityLimits()["DefenseTower"] && queues.defenceBuilding.totalLength() < 4
		&& gameState.currentPhase() > 1 && queues.defenceBuilding.totalLength() < 3) {
		gameState.getOwnEntities().forEach(function(dropsiteEnt) {
			if (dropsiteEnt.resourceDropsiteTypes() && dropsiteEnt.getMetadata(PlayerID, "defenseTower") !== true
				&& (dropsiteEnt.getMetadata(PlayerID, "resource-quantity-wood") > 400 || dropsiteEnt.getMetadata(PlayerID, "resource-quantity-stone") > 500
					|| dropsiteEnt.getMetadata(PlayerID, "resource-quantity-metal") > 500) ){
				var position = dropsiteEnt.position();
				if (position){
					queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, 'structures/{civ}_defense_tower', position));
				}
				dropsiteEnt.setMetadata(PlayerID, "defenseTower", true);
			}
		});
	}
	
	var numFortresses = 0;
	for (var i in this.bFort){
		numFortresses += gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bFort[i]));
	}
	
	if (queues.defenceBuilding.totalLength() < 1 && (gameState.currentPhase() > 2 || gameState.isResearching("phase_city_generic")))
	{
		if (workersNumber >= 80 && gameState.getTimeElapsed() > numFortresses * this.fortressLapseTime + this.fortressStartTime)
		{
			if (!this.fortressStartTime)
				this.fortressStartTime = gameState.getTimeElapsed();
			queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, this.bFort[0]));
			debug ("Building a fortress");
		}
	}
	if (gameState.countEntitiesByType(gameState.applyCiv(this.bFort[i])) >= 1) {
		// let's add a siege building plan to the current attack plan if there is none currently.
		if (this.upcomingAttacks["CityAttack"].length !== 0)
		{
			var attack = this.upcomingAttacks["CityAttack"][0];
			if (!attack.unitStat["Siege"])
			{
				// no minsize as we don't want the plan to fail at the last minute though.
				var stat = { "priority" : 1.1, "minSize" : 0, "targetSize" : 4, "batchSize" : 2, "classes" : ["Siege"],
					"interests" : [ ["siegeStrength", 3], ["cost",1] ]  ,"templates" : [] };
				if (gameState.civ() == "cart" || gameState.civ() == "maur")
					stat["classes"] = ["Elephant"];
				attack.addBuildOrder(gameState, "Siege", stat, true);
			}
		}
	}
};

// Deals with constructing military buildings (barracks, stables…)
// They are mostly defined by Config.js. This is unreliable since changes could be done easily.
// TODO: We need to determine these dynamically. Also doesn't build fortresses since the above function does that.
// TODO: building placement is bad. Choice of buildings is also fairly dumb.
MilitaryAttackManager.prototype.constructTrainingBuildings = function(gameState, queues) {
	Engine.ProfileStart("Build buildings");
	var workersNumber = gameState.getOwnEntitiesByRole("worker").filter(Filters.not(Filters.byHasMetadata(PlayerID, "plan"))).length;
	
	if (workersNumber > 30 && (gameState.currentPhase() > 1 || gameState.isResearching("phase_town"))) {
		if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0])) + queues.militaryBuilding.totalLength() < 1) {
			debug ("Trying to build barracks");
			queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
		}
	}
	
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0])) < 2 && workersNumber > 85)
		if (queues.militaryBuilding.totalLength() < 1)
			queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
	
	if (gameState.countEntitiesByType(gameState.applyCiv(this.bModerate[0])) === 2  && gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0])) < 3 && workersNumber > 125)
		if (queues.militaryBuilding.totalLength() < 1)
		{
			queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
			if (gameState.civ() == "gaul" || gameState.civ() == "brit" || gameState.civ() == "iber") {
				queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
				queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
			}
		}
	//build advanced military buildings
	if (workersNumber >= 75 && gameState.currentPhase() > 2){
		if (queues.militaryBuilding.totalLength() === 0){
			var inConst = 0;
			for (var i in this.bAdvanced)
				inConst += gameState.countFoundationsWithType(gameState.applyCiv(this.bAdvanced[i]));
			if (inConst == 0 && this.bAdvanced && this.bAdvanced.length !== 0) {
				var i = Math.floor(Math.random() * this.bAdvanced.length);
				if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i])) < 1){
					queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bAdvanced[i]));
				}
			}
		}
	}
	if (gameState.civ() !== "gaul" && gameState.civ() !== "brit" && gameState.civ() !== "iber" &&
		 workersNumber > 130 && gameState.currentPhase() > 2)
	{
		var Const = 0;
		for (var i in this.bAdvanced)
			Const += gameState.countEntitiesByType(gameState.applyCiv(this.bAdvanced[i]));
		if (inConst == 1) {
			var i = Math.floor(Math.random() * this.bAdvanced.length);
			if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i])) < 1){
				queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bAdvanced[i]));
				queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bAdvanced[i]));
			}
		}
	}

	Engine.ProfileStop();
};

// TODO: use pop(). Currently unused as this is too gameable.
MilitaryAttackManager.prototype.garrisonAllFemales = function(gameState) {
	var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).toEntityArray();
	var females = gameState.getOwnEntities().filter(Filters.byClass("Support"));
	
	var cache = {};
	
	females.forEach( function (ent) {
		for (var i in buildings)
		{
			if (ent.position())
			{
				var struct = buildings[i];
				if (!cache[struct.id()])
					cache[struct.id()] = 0;
				if (struct.garrisoned() && struct.garrisonMax() - struct.garrisoned().length - cache[struct.id()] > 0)
				{
					ent.garrison(struct);
					cache[struct.id()]++;
					break;
				}
			}
		}
	});
	this.hasGarrisonedFemales = true;
};
MilitaryAttackManager.prototype.ungarrisonAll = function(gameState) {
	this.hasGarrisonedFemales = false;
	var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).toEntityArray();
	buildings.forEach( function (struct) {
		if (struct.garrisoned() && struct.garrisoned().length)
				struct.unloadAll();
	});
};

MilitaryAttackManager.prototype.pausePlan = function(gameState, planName) {
	for (attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, true);
		}
	}
	for (attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, true);
		}
	}
}
MilitaryAttackManager.prototype.unpausePlan = function(gameState, planName) {
	for (attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, false);
		}
	}
	for (attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, false);
		}
	}
}
MilitaryAttackManager.prototype.pauseAllPlans = function(gameState) {
	for (attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			attack.setPaused(gameState, true);
		}
	}
	for (attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			attack.setPaused(gameState, true);
		}
	}
}
MilitaryAttackManager.prototype.unpauseAllPlans = function(gameState) {
	for (attackType in this.upcomingAttacks) {
		for (var i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			attack.setPaused(gameState, false);
		}
	}
	for (attackType in this.startedAttacks) {
		for (var i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			attack.setPaused(gameState, false);
		}
	}
}
MilitaryAttackManager.prototype.update = function(gameState, queues, events) {
	var self = this;
	
	Engine.ProfileStart("military update");
	
	this.gameState = gameState;
	
	Engine.ProfileStart("Constructing military buildings and building defences");
	this.constructTrainingBuildings(gameState, queues);
	
	if(gameState.getTimeElapsed() > this.defenceBuildingTime)
		this.buildDefences(gameState, queues);
	Engine.ProfileStop();

	this.defenceManager.update(gameState, events, this);
	
	Engine.ProfileStart("Looping through attack plans");
	// TODO: implement some form of check before starting a new attack plans. Sometimes it is not the priority.
	if (1) {
		for (attackType in this.upcomingAttacks) {
			for (var i = 0;i < this.upcomingAttacks[attackType].length; ++i) {
				
				var attack = this.upcomingAttacks[attackType][i];

				// okay so we'll get the support plan
				if (!attack.isStarted()) {
					var updateStep = attack.updatePreparation(gameState, this,events);
					
					// now we're gonna check if the preparation time is over
					if (updateStep === 1 || attack.isPaused() ) {
						// just chillin'
					} else if (updateStep === 0 || updateStep === 3) {
						debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" aborted.");
						if (updateStep === 3) {
							this.attackPlansEncounteredWater = true;
							debug("No attack path found. Aborting.");
						}
						attack.Abort(gameState, this);
						this.upcomingAttacks[attackType].splice(i--,1);
					} else if (updateStep === 2) {
						var chatText = "I am launching an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						if (Math.random() < 0.2)
							chatText = "Attacking " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						else if (Math.random() < 0.3)
							chatText = "I have sent an army against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						else if (Math.random() < 0.3)
							chatText = "I'm starting an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
						gameState.ai.chatTeam(chatText);
						
						debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
						attack.StartAttack(gameState,this);
						this.startedAttacks[attackType].push(attack);
						this.upcomingAttacks[attackType].splice(i--,1);
					}
				} else {
					var chatText = "I am launching an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					if (Math.random() < 0.2)
						chatText = "Attacking " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					else if (Math.random() < 0.3)
						chatText = "I have sent an army against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					else if (Math.random() < 0.3)
						chatText = "I'm starting an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name + ".";
					gameState.ai.chatTeam(chatText);
					
					debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
					this.startedAttacks[attackType].push(attack);
					this.upcomingAttacks[attackType].splice(i--,1);
				}
			}
		}
	}
	for (attackType in this.startedAttacks) {
		for (var i = 0; i < this.startedAttacks[attackType].length; ++i) {
			var attack = this.startedAttacks[attackType][i];
			// okay so then we'll update the attack.
			if (!attack.isPaused())
			{
				var remaining = attack.update(gameState,this,events);
				if (remaining == 0 || remaining == undefined) {
					debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" is now finished.");
					attack.Abort(gameState);
					this.startedAttacks[attackType].splice(i--,1);
				}
			}
		}
	}
	// Note: these indications of "rush" are currently unused. 
	if (gameState.ai.strategy === "rush" && this.startedAttacks["CityAttack"].length !== 0) {
		// and then we revert.
		gameState.ai.strategy = "normal";
		Config.Economy.femaleRatio = 0.4;
		gameState.ai.modules.economy.targetNumWorkers = Math.max(Math.floor(gameState.getPopulationMax()*0.55), 1);
	} else if (gameState.ai.strategy === "rush" && this.upcomingAttacks["CityAttack"].length === 0)
	{
		Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1, "rush")
		this.TotalAttackNumber++;
		this.upcomingAttacks["CityAttack"].push(Lalala);
		debug ("Starting a little something");
	} else if (gameState.ai.strategy !== "rush")
	{
		// creating plans after updating because an aborted plan might be reused in that case.
		if (gameState.countEntitiesByType(gameState.applyCiv(this.bModerate[0])) >= 1 && !this.attackPlansEncounteredWater
			&& gameState.getTimeElapsed() > this.attackPlansStartTime) {
			if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_dock")) === 0 && gameState.ai.waterMap)
			{
					// wait till we get a dock.
			} else {
				// basically only the first plan, really.
				if (this.upcomingAttacks["CityAttack"].length == 0 && (gameState.getTimeElapsed() < 12*60000 || Config.difficulty < 1)) {
					var Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1);
					if (Lalala.failed)
					{
						this.attackPlansEncounteredWater = true; // hack
					} else {
						debug ("Military Manager: Creating the plan " +this.TotalAttackNumber);
						this.TotalAttackNumber++;
						this.upcomingAttacks["CityAttack"].push(Lalala);
					}
				} else if (this.upcomingAttacks["CityAttack"].length == 0 && Config.difficulty !== 0) {
					var Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1, "superSized");
					if (Lalala.failed)
					{
						this.attackPlansEncounteredWater = true; // hack
					} else {
						debug ("Military Manager: Creating the super sized plan " +this.TotalAttackNumber);
						this.TotalAttackNumber++;
						this.upcomingAttacks["CityAttack"].push(Lalala);
					}
				}
			}
		}
	}
	
	/*
	 // very old relic. This should be reimplemented someday so the code stays here.
	 
	 if (this.HarassRaiding && this.preparingRaidNumber + this.startedRaidNumber < 1 && gameState.getTimeElapsed() < 780000) {
	 var Lalala = new CityAttack(gameState, this,this.totalStartedAttackNumber, -1, "harass_raid");
	 if (!Lalala.createSupportPlans(gameState, this, )) {
	 debug ("Military Manager: harrassing plan not a valid option");
	 this.HarassRaiding = false;
	 } else {
	 debug ("Military Manager: Creating the harass raid plan " +this.totalStartedAttackNumber);
	 
	 this.totalStartedAttackNumber++;
	 this.preparingRaidNumber++;
	 this.currentAttacks.push(Lalala);
	 }
	 }
	 */

	
	Engine.ProfileStop();
	Engine.ProfileStop();
};
