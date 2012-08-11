/*
 * Military strategy:
 *   * Try training an attack squad of a specified size
 *   * When it's the appropriate size, send it to attack the enemy
 *   * Repeat forever
 *
 */

var MilitaryAttackManager = function() {
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
	for (var i = 1; i <= 8; i++)
		if (gameState.player != i && gameState.isPlayerEnemy(i)) {
			this.enemyWatchers[i] = new enemyWatcher(gameState, i);
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
			   aTopParam += a[1].getMaxStrength() * param[1];
			   bTopParam += b[1].getMaxStrength() * param[1];
			   }
			   if (param[0] == "speed") {
			   aTopParam += a[1].walkSpeed() * param[1];
			   bTopParam += b[1].walkSpeed() * param[1];
			   }
			   
			   if (param[0] == "cost") {
			   aDivParam += a[1].costSum() * param[1];
			   bDivParam += b[1].costSum() * param[1];
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
		ent.setMetadata("role", "military");
		ent.setMetadata("military", "unassigned");
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
		ent.setMetadata("military", "assigned");
		ent.setMetadata("role", "military");
		count++;
		if (count >= n) {
			return;
		}
	});
	return ret;
};

// Takes a single unit id, and marks it unassigned
MilitaryAttackManager.prototype.unassignUnit = function(unit){
	this.entity(unit).setMetadata("military", "unassigned");
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
	if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv('structures/{civ}_defense_tower'))
			+ queues.defenceBuilding.totalLength() < gameState.getBuildLimits()["DefenseTower"]) {
		
		gameState.getOwnEntities().forEach(function(dropsiteEnt) {
			if (dropsiteEnt.resourceDropsiteTypes() && dropsiteEnt.getMetadata("defenseTower") !== true){
				var position = dropsiteEnt.position();
				if (position){
					queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, 'structures/{civ}_defense_tower', position));
				}
				dropsiteEnt.setMetadata("defenseTower", true);
			}
		});
	}
	
	var numFortresses = 0;
	for (var i in this.bFort){
		numFortresses += gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bFort[i]));
	}
	
	if (numFortresses + queues.defenceBuilding.totalLength() < 1){ //gameState.getBuildLimits()["Fortress"]) {
		if (gameState.getTimeElapsed() > 840 * 1000 + numFortresses * 300 * 1000){
			if (gameState.ai.pathsToMe && gameState.ai.pathsToMe.length > 0){
				var position = gameState.ai.pathsToMe.shift();
				// TODO: pick a fort randomly from the list.
				queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, this.bFort[0], position));
			}else{
				queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, this.bFort[0]));
			}
		}
	}
};

MilitaryAttackManager.prototype.constructTrainingBuildings = function(gameState, queues) {
	// Build more military buildings
	// TODO: make military building better
	Engine.ProfileStart("Build buildings");
	if (gameState.countEntitiesByType(gameState.applyCiv("units/{civ}_support_female_citizen")) > 25) {
		if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0]))
				+ queues.militaryBuilding.totalLength() < 1) {
			queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
		}
	}
	//build advanced military buildings
	if (gameState.getTimeElapsed() > 720*1000){
		if (queues.militaryBuilding.totalLength() === 0){
			var inConst = 0;
			for (var i in this.bAdvanced)
				inConst += gameState.countFoundationsWithType(gameState.applyCiv(this.bAdvanced[i]));
			if (inConst == 0 && this.bAdvanced !== undefined) {
				var i = Math.floor(Math.random() * this.bAdvanced.length);
				if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bAdvanced[i])) < 1){
					queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bAdvanced[i]));
				}
			}
		}
	}
	Engine.ProfileStop();
};

MilitaryAttackManager.prototype.trainMilitaryUnits = function(gameState, queues){
	Engine.ProfileStart("Train Units");
	// Continually try training new units, in batches of 5
	if (queues.citizenSoldier.length() < 6) {
		var newUnit = this.findBestNewUnit(gameState, queues.citizenSoldier, "citizenSoldier");
		if (newUnit){
			queues.citizenSoldier.addItem(new UnitTrainingPlan(gameState, newUnit, {
				"role" : "soldier"
			}, 5));
		}
	}
	if (queues.advancedSoldier.length() < 2) {
		var newUnit = this.findBestNewUnit(gameState, queues.advancedSoldier, "advanced");
		if (newUnit){
			queues.advancedSoldier.addItem(new UnitTrainingPlan(gameState, newUnit, {
				"role" : "soldier"
			}, 5));
		}
	}
	if (queues.siege.length() < 4) {
		var newUnit = this.findBestNewUnit(gameState, queues.siege, "siege");
		if (newUnit){
			queues.siege.addItem(new UnitTrainingPlan(gameState, newUnit, {
				"role" : "soldier"
			}, 2));
		}
	}
	Engine.ProfileStop();
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
	
	//this.registerSoldiers(gameState);
	
	//this.trainMilitaryUnits(gameState, queues);
	
	this.constructTrainingBuildings(gameState, queues);
	
	if(gameState.getTimeElapsed() > 300*1000)
		this.buildDefences(gameState, queues);
	
	for (watcher in this.enemyWatchers)
		this.enemyWatchers[watcher].detectArmies(gameState,this);
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
			for (i in this.upcomingAttacks[attackType]) {
				
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
							debug("I dare not wet my feet");
						}
						attack.Abort(gameState, this);
						//this.abortedAttacks.push(attack);
						
						i--;
						this.upcomingAttacks[attackType].splice(i,1);
					} else if (updateStep === 2) {
						debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
						attack.StartAttack(gameState,this);
						this.startedAttacks[attackType].push(attack);
						i--;
						this.upcomingAttacks[attackType].splice(i-1,1);
					}
				} else {
					debug ("Military Manager: Starting " +attack.getType() +" plan " +attack.getName());
					this.startedAttacks[attackType].push(attack);
					i--;
					this.upcomingAttacks[attackType].splice(i-1,1);
				}
			}
		}
		//if (this.abortedAttacks.length !== 0)
		//	this.abortedAttacks[gameState.ai.mainCounter % this.abortedAttacks.length].releaseAnyUnit(gameState);
	}
	for (attackType in this.startedAttacks) {
		for (i in this.startedAttacks[attackType]) {
			var attack = this.startedAttacks[attackType][i];
			// okay so then we'll update the raid.
			var remaining = attack.update(gameState,this,events);
			if (remaining == 0 || remaining == undefined) {
				debug ("Military Manager: " +attack.getType() +" plan " +attack.getName() +" is now finished.");
				attack.Abort(gameState);
				
				//this.abortedAttacks.push(attack);
				this.startedAttacks[attackType].splice(i,1);
				i--;
			}
		}
	}
	// creating plans after updating because an aborted plan might be reused in that case.
	if (gameState.countEntitiesByType(gameState.applyCiv(this.bModerate[0])) >= 1 && !this.attackPlansEncounteredWater) {
		if (this.upcomingAttacks["CityAttack"].length == 0 && gameState.getTimeElapsed() < 25*60000) {
			var Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1);
			debug ("Military Manager: Creating the plan " +this.TotalAttackNumber);
			this.TotalAttackNumber++;
			this.upcomingAttacks["CityAttack"].push(Lalala);
		} else if (this.upcomingAttacks["CityAttack"].length == 0) {
			var Lalala = new CityAttack(gameState, this,this.TotalAttackNumber, -1, "superSized");
			debug ("Military Manager: Creating the super sized plan " +this.TotalAttackNumber);
			this.TotalAttackNumber++;
			this.upcomingAttacks["CityAttack"].push(Lalala);
		}
	}
	/*
	 if (this.HarassRaiding && this.preparingRaidNumber + this.startedRaidNumber < 1 && gameState.getTimeElapsed() < 780000) {
	 var Lalala = new CityAttack(gameState, this,this.totalStartedAttackNumber, -1, "harass_raid");
	 if (!Lalala.createSupportPlans(gameState, this, queues.advancedSoldier)) {
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
			ent.setMetadata("role", "worker");
		}
	});
	Engine.ProfileStop();*/
	
	Engine.ProfileStop();
};
