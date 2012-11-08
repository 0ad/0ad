/*
 * Military strategy:
 *   * Try training an attack squad of a specified size
 *   * When it's the appropriate size, send it to attack the enemy
 *   * Repeat forever
 *
 */

var MilitaryAttackManager = function() {
	// these use the structure soldiers[unitId] = true|false to register the units
	this.attackManagers = [AttackMoveToLocation];
	this.availableAttacks = [];
	this.currentAttacks = [];
	
	// Counts how many attacks we have sent at the enemy.
	this.attackCount = 0;
	this.lastAttackTime = 0;
	
	this.defenceManager = new Defence();
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
			+ queues.defenceBuilding.totalLength() < gameState.getEntityLimits()["DefenseTower"]) {
		
		
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
	
	if (numFortresses + queues.defenceBuilding.totalLength() < gameState.getEntityLimits()["Fortress"]) {
		if (gameState.countEntitiesByType(gameState.applyCiv("units/{civ}_support_female_citizen")) > gameState.ai.modules["economy"].targetNumWorkers * 0.5){
			if (gameState.getTimeElapsed() > 350 * 1000 * numFortresses){
				if (gameState.ai.pathsToMe && gameState.ai.pathsToMe.length > 0){
					var position = gameState.ai.pathsToMe.shift();
					// TODO: pick a fort randomly from the list.
					queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, this.bFort[0], position));
				}else{
					queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, this.bFort[0]));
				}
			}
		}
	}
};

MilitaryAttackManager.prototype.constructTrainingBuildings = function(gameState, queues) {
	// Build more military buildings
	// TODO: make military building better
	Engine.ProfileStart("Build buildings");
	if (gameState.countEntitiesByType(gameState.applyCiv("units/{civ}_support_female_citizen")) > 30) {
		if (gameState.countEntitiesAndQueuedByType(gameState.applyCiv(this.bModerate[0]))
				+ queues.militaryBuilding.totalLength() < 1) {
			queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
		}
	}
	//build advanced military buildings
	if (gameState.countEntitiesByType(gameState.applyCiv("units/{civ}_support_female_citizen")) > 
			gameState.ai.modules["economy"].targetNumWorkers * 0.7){
		if (queues.militaryBuilding.totalLength() === 0){
			for (var i in this.bAdvanced){
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

MilitaryAttackManager.prototype.update = function(gameState, queues, events) {
	var self = this;
	Engine.ProfileStart("military update");
	this.gameState = gameState;
	
	this.registerSoldiers(gameState);
	
	this.trainMilitaryUnits(gameState, queues);
	
	this.constructTrainingBuildings(gameState, queues);
	
	this.buildDefences(gameState, queues);
	
	this.defenceManager.update(gameState, events, this);
	
	Engine.ProfileStart("Plan new attacks");
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
	Engine.ProfileStop();
	
	Engine.ProfileStart("Use idle military as workers");
	// Set unassigned to be workers TODO: fix this so it doesn't scan all units every time
	this.getUnassignedUnits(gameState).forEach(function(ent){
		if (self.getSoldierType(ent) === "citizenSoldier"){
			ent.setMetadata("role", "worker");
		}
	});
	Engine.ProfileStop();
	
	Engine.ProfileStop();
};
