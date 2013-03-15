/*
 * Military Manager. NOT cleaned up from qBot, many of the functions here are deprecated for functions in attack_plan.js
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
	
	this.getEconomicTargets = function(gameState, militaryManager){
		return militaryManager.getEnemyBuildings(gameState, "Economic");
	};
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

/**
 * @param (GameState) gameState
 * @param (string) soldierTypes
 * @returns array of soldiers for which training buildings exist
 */
MilitaryAttackManager.prototype.findTrainableUnits = function(gameState, soldierType){
	var allTrainable = [];
	gameState.getOwnEntities().forEach(function(ent) {
		var trainable = ent.trainableEntities();
		for (var i in trainable){
			if (allTrainable.indexOf(trainable[i]) === -1){
				allTrainable.push(trainable[i]);
			}
		}
	});
	
	var ret = [];
	for (var i in allTrainable){
		var template = gameState.getTemplate(allTrainable[i]);
		if (soldierType == this.getSoldierType(template)){
			ret.push(allTrainable[i]);
		}
	}
	return ret;
};

// Returns the type of a soldier, either citizenSoldier, advanced or siege 
MilitaryAttackManager.prototype.getSoldierType = function(ent){
	if (ent.hasClass("Hero")){
		return undefined;
	}
	if (ent.hasClass("CitizenSoldier") && !ent.hasClass("Cavalry")){
		return "citizenSoldier";
	}else if (ent.hasClass("Champion") || ent.hasClass("CitizenSoldier")){
		return "advanced";
	}else if (ent.hasClass("Siege")){
		return "siege";
	}else{
		return undefined;
	}
};

/**
 * Returns the unit type we should begin training. (Currently this is whatever
 * we have least of.)
 */
MilitaryAttackManager.prototype.findBestNewUnit = function(gameState, queue, soldierType) {
	var units = this.findTrainableUnits(gameState, soldierType);
	// Count each type
	var types = [];
	for ( var tKey in units) {
		var t = units[tKey];
		types.push([t, gameState.countEntitiesAndQueuedByType(gameState.applyCiv(t))
						+ queue.countAllByType(gameState.applyCiv(t)) ]);
	}

	// Sort by increasing count
	types.sort(function(a, b) {
		return a[1] - b[1];
	});

	if (types.length === 0){
		return false;
	}
	return types[0][0];
};
// picks the best template based on parameters and classes
MilitaryAttackManager.prototype.findBestTrainableUnit = function(gameState, classes, parameters) {
	var units = gameState.findTrainableUnits(classes);
	
	if (units.length === 0)
		return undefined;
	
	
	units.sort(function(a, b) { //}) {
		var aDivParam = 0, bDivParam = 0;
		var aTopParam = 0, bTopParam = 0;
		for (i in parameters) {
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



MilitaryAttackManager.prototype.registerSoldiers = function(gameState) {
	var soldiers = gameState.getOwnEntitiesByRole("soldier");
	var self = this;

	soldiers.forEach(function(ent) {
		ent.setMetadata(PlayerID, "role", "military");
		ent.setMetadata(PlayerID, "military", "unassigned");
	});
};

// return count of enemy buildings for a given building class
MilitaryAttackManager.prototype.getEnemyBuildings = function(gameState,cls) {
	var targets = gameState.entities.filter(function(ent) {
			return (gameState.isEntityEnemy(ent) && ent.hasClass("Structure") && ent.hasClass(cls) && ent.owner() !== 0  && ent.position());
		});
	return targets;
};

// return n available units and makes these units unavailable
MilitaryAttackManager.prototype.getAvailableUnits = function(n, filter) {
	var ret = [];
	var count = 0;
	
	var units = undefined;
	
	if (filter){
		units = this.getUnassignedUnits().filter(filter);
	}else{
		units = this.getUnassignedUnits();
	}
	
	units.forEach(function(ent){
		ret.push(ent.id());
		ent.setMetadata(PlayerID, "military", "assigned");
		ent.setMetadata(PlayerID, "role", "military");
		count++;
		if (count >= n) {
			return;
		}
	});
	return ret;
};

// Takes a single unit id, and marks it unassigned
MilitaryAttackManager.prototype.unassignUnit = function(unit){
	this.entity(unit).setMetadata(PlayerID, "military", "unassigned");
};

// Takes an array of unit id's and marks all of them unassigned 
MilitaryAttackManager.prototype.unassignUnits = function(units){
	for (var i in units){
		this.unassignUnit(units[i]);
	}
};

MilitaryAttackManager.prototype.getUnassignedUnits = function(){
	return this.gameState.getOwnEntitiesByMetadata("military", "unassigned");
};

MilitaryAttackManager.prototype.countAvailableUnits = function(filter){
	var count = 0;
	if (filter){
		return this.getUnassignedUnits().filter(filter).length;
	}else{
		return this.getUnassignedUnits().length;
	}
};

// Takes an entity id and returns an entity object or undefined if there is no entity with that id
// Also sends a debug message warning if the id has no entity
MilitaryAttackManager.prototype.entity = function(id) {
	return this.gameState.getEntityById(id);
};

// Returns the military strength of unit 
MilitaryAttackManager.prototype.getUnitStrength = function(ent){
	var strength = 0.0;
	var attackTypes = ent.attackTypes();
	var armourStrength = ent.armourStrengths();
	var hp = 2 * ent.hitpoints() / (160 + 1*ent.maxHitpoints()); //100 = typical number of hitpoints
	for (var typeKey in attackTypes) {
		var type = attackTypes[typeKey];
		var attackStrength = ent.attackStrengths(type);
		var attackRange = ent.attackRange(type);
		var attackTimes = ent.attackTimes(type);
		for (var str in attackStrength) {
			var val = parseFloat(attackStrength[str]);
			switch (str) {
				case "crush":
					strength += (val * 0.085) / 3;
					break;
				case "hack":
					strength += (val * 0.075) / 3;
					break;
				case "pierce":
					strength += (val * 0.065) / 3;
					break;
			}
		}
		if (attackRange){
			strength += (attackRange.max * 0.0125) ;
		}
		for (var str in attackTimes) {
			var val = parseFloat(attackTimes[str]);
			switch (str){
				case "repeat":
					strength += (val / 100000);
					break;
				case "prepare":
					strength -= (val / 100000);
					break;
			}
		}
	}
	for (var str in armourStrength) {
		var val = parseFloat(armourStrength[str]);
		switch (str) {
			case "crush":
				strength += (val * 0.085) / 3;
				break;
			case "hack":
				strength += (val * 0.075) / 3;
				break;
			case "pierce":
				strength += (val * 0.065) / 3;
				break;
		}
	}
	return strength * hp;
};

// Returns the  strength of the available units of ai army
MilitaryAttackManager.prototype.measureAvailableStrength = function(){
	var  strength = 0.0;
	var self = this;
	this.getUnassignedUnits(this.gameState).forEach(function(ent){
		strength += self.getUnitStrength(ent);
	});
	return strength;
};

MilitaryAttackManager.prototype.getEnemySoldiers = function(){
	return this.enemySoldiers;
};

// Returns the number of units in the largest enemy army
MilitaryAttackManager.prototype.measureEnemyCount = function(gameState){
	// Measure enemy units
	var isEnemy = gameState.playerData.isEnemy;
	var enemyCount = [];
	var maxCount = 0;
	for ( var i = 1; i < isEnemy.length; i++) {
		enemyCount[i] = 0;
	}
	
	// Loop through the enemy soldiers and add one to the count for that soldiers player's count
	this.enemySoldiers.forEach(function(ent) {
		enemyCount[ent.owner()]++;
		
		if (enemyCount[ent.owner()] > maxCount) {
			maxCount = enemyCount[ent.owner()];
		}
	});
	
	return maxCount;
};

// Returns the strength of the largest enemy army
MilitaryAttackManager.prototype.measureEnemyStrength = function(gameState){
	// Measure enemy strength
	var isEnemy = gameState.playerData.isEnemy;
	var enemyStrength = [];
	var maxStrength = 0;
	var self = this;
	
	for ( var i = 1; i < isEnemy.length; i++) {
		enemyStrength[i] = 0;
	}
	
	// Loop through the enemy soldiers and add the strength to that soldiers player's total strength
	this.enemySoldiers.forEach(function(ent) {
		enemyStrength[ent.owner()] += self.getUnitStrength(ent);
		
		if (enemyStrength[ent.owner()] > maxStrength) {
			maxStrength = enemyStrength[ent.owner()];
		}
	});
	
	return maxStrength;
};

// Adds towers to the defenceBuilding queue
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

MilitaryAttackManager.prototype.constructTrainingBuildings = function(gameState, queues) {
	// Build more military buildings
	// TODO: make military building better
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

// TODO: use pop()
MilitaryAttackManager.prototype.garrisonAllFemales = function(gameState) {
	var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).toEntityArray();
	var females = gameState.getOwnEntities().filter(Filters.byClass("Support"));
	
	var cache = {};
	
	females.forEach( function (ent) {
		for (i in buildings)
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
		for (i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, true);
		}
	}
}
MilitaryAttackManager.prototype.unpausePlan = function(gameState, planName) {
	for (attackType in this.upcomingAttacks) {
		for (i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(gameState, false);
		}
	}
}
MilitaryAttackManager.prototype.pauseAllPlans = function(gameState) {
	for (attackType in this.upcomingAttacks) {
		for (i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
			attack.setPaused(gameState, true);
		}
	}
}
MilitaryAttackManager.prototype.unpauseAllPlans = function(gameState) {
	for (attackType in this.upcomingAttacks) {
		for (i in this.upcomingAttacks[attackType]) {
			var attack = this.upcomingAttacks[attackType][i];
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

	//Engine.ProfileStart("Updating enemy watchers");
	//this.enemyWatchers[ this.ennWatcherIndex[gameState.ai.playedTurn % this.ennWatcherIndex.length] ].detectArmies(gameState,this);
	//Engine.ProfileStop();

	this.defenceManager.update(gameState, events, this);
	
	/*Engine.ProfileStart("Plan new attacks");
	// Look for attack plans which can be executed, only do this once every minute
	for (var i = 0; i < this.availableAttacks.length; i++){
		if (this.availableAttacks[i].canExecute(gameState, this)){
			this.availableAttacks[i].execute(gameState, this);
			this.currentAttacks.push(this.availableAttacks[i]);
			//debug("Attacking!");
		}
		this.availableAttacks.splice(i, 1, new this.attackManagers[i](gameState, this));
	}
	Engine.ProfileStop();
	
	Engine.ProfileStart("Update attacks");
	// Keep current attacks updated
	for (var i in this.currentAttacks){
		this.currentAttacks[i].update(gameState, this, events);
	}
	Engine.ProfileStop();*/
	
	Engine.ProfileStart("Looping through attack plans");
	// create plans if I'm at peace. I'm not starting plans if there is a sizable force in my realm (hence defcon 4+)
	
	//if (gameState.defcon() >= 4 && this.canStartAttacks === true) {
		//if ((this.preparingNormal) == 0 && this.BuildingInfoManager.getNumberBuiltByRole("Barracks") > 0) {

	// this will updats plans. Plans can be updated up to defcon 2, where they'll be paused (TODO)
	//if (0 == 1)	// remove to activate attacks
	//if (gameState.defcon() >= 3) {
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
						//this.abortedAttacks.push(attack);
						
						this.upcomingAttacks[attackType].splice(i--,1);
					} else if (updateStep === 2) {
						var chatText = "I am launching an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name;
						if (Math.random() < 0.2)
							chatText = "Attacking " + gameState.sharedScript.playersData[attack.targetPlayer].name;
						else if (Math.random() < 0.3)
							chatText = "Starting to attack " + gameState.sharedScript.playersData[attack.targetPlayer].name;
						else if (Math.random() < 0.3)
							chatText = "I'm starting an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name;
						gameState.ai.chatTeam(chatText);
						debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
						attack.StartAttack(gameState,this);
						this.startedAttacks[attackType].push(attack);
						this.upcomingAttacks[attackType].splice(i--,1);
					}
				} else {
					var chatText = "I am launching an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name;
					if (Math.random() < 0.2)
						chatText = "Attacking " + gameState.sharedScript.playersData[attack.targetPlayer].name;
					else if (Math.random() < 0.3)
						chatText = "Starting to attack " + gameState.sharedScript.playersData[attack.targetPlayer].name;
					else if (Math.random() < 0.3)
						chatText = "I'm starting an attack against " + gameState.sharedScript.playersData[attack.targetPlayer].name;
					gameState.ai.chatTeam(chatText);
					debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
					this.startedAttacks[attackType].push(attack);
					this.upcomingAttacks[attackType].splice(i--,1);
				}
			}
		}
		//if (this.abortedAttacks.length !== 0)
		//	this.abortedAttacks[gameState.ai.mainCounter % this.abortedAttacks.length].releaseAnyUnit(gameState);
	}
	for (attackType in this.startedAttacks) {
		for (var i = 0; i < this.startedAttacks[attackType].length; ++i) {
			var attack = this.startedAttacks[attackType][i];
			// okay so then we'll update the raid.
			if (!attack.isPaused())
			{
				var remaining = attack.update(gameState,this,events);
				if (remaining == 0 || remaining == undefined) {
					debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" is now finished.");
					attack.Abort(gameState);
				
					//this.abortedAttacks.push(attack);
					this.startedAttacks[attackType].splice(i--,1);
				}
			}
		}
	}
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

	
	/*Engine.ProfileStart("Use idle military as workers");
	// Set unassigned to be workers TODO: fix this so it doesn't scan all units every time
	this.getUnassignedUnits(gameState).forEach(function(ent){
		if (self.getSoldierType(ent) === "citizenSoldier"){
			ent.setMetadata(PlayerID, "role", "worker");
		}
	});
	Engine.ProfileStop();*/
	
	Engine.ProfileStop();
};
