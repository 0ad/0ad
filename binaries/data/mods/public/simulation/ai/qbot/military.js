/*
 * Military strategy:
 *   * Try training an attack squad of a specified size
 *   * When it's the appropriate size, send it to attack the enemy
 *   * Repeat forever
 *
 */

var MilitaryAttackManager = function() {
	// these use the structure soldiers[unitId] = true|false to register the units
	this.soldiers = {};
	this.assigned = {};
	this.unassigned = {};
	this.garrisoned = {};
	this.enemyAttackers = {};

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
	if (civ in Config.units.citizenSoldier){
		this.uCitizenSoldier = Config.units.citizenSoldier[civ];
	}else{
		this.uCitizenSoldier = Config.units.citizenSoldier['default'];
	}
	
	if (civ in Config.units.advanced){
		this.uAdvanced = Config.units.advanced[civ];
	}else{
		this.uAdvanced = Config.units.advanced['default'];
	}
	
	if (civ in Config.units.siege){
		this.uSiege = Config.units.siege[civ];
	}else{
		this.uSiege = Config.units.siege['default'];
	}
	
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

	for (var i in this.uCitizenSoldier){
		this.uCitizenSoldier[i] = gameState.applyCiv(this.uCitizenSoldier[i]);
	}
	for (var i in this.uAdvanced){
		this.uAdvanced[i] = gameState.applyCiv(this.uAdvanced[i]);
	}
	for (var i in this.uSiege){
		this.uSiege[i] = gameState.applyCiv(this.uSiege[i]);
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
	
	var filter = Filters.and(Filters.isEnemy(), Filters.byClassesOr(["CitizenSoldier", "Super", "Siege"]));
	this.enemySoldiers = new EntityCollection(gameState.ai, gameState.entities._entities, filter, gameState);
};

/**
 * @param (GameState) gameState
 * @returns array of soldiers for which training buildings exist
 */
MilitaryAttackManager.prototype.findTrainableUnits = function(gameState, soldierTypes){
	var ret = [];
	gameState.getOwnEntities().forEach(function(ent) {
		var trainable = ent.trainableEntities();
		for (var i in trainable){
			if (soldierTypes.indexOf(trainable[i]) !== -1){
				if (ret.indexOf(trainable[i]) === -1){
					ret.push(trainable[i]);
				}
			} 
		}
		return true;
	});
	return ret;
};

/**
 * Returns the unit type we should begin training. (Currently this is whatever
 * we have least of.)
 */
MilitaryAttackManager.prototype.findBestNewUnit = function(gameState, queue, soldierTypes) {
	var units = this.findTrainableUnits(gameState, soldierTypes);
	// Count each type
	var types = [];
	for ( var tKey in units) {
		var t = units[tKey];
		types.push([t, gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(t))
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
	var soldiers = gameState.getOwnEntitiesWithRole("soldier");
	var self = this;

	soldiers.forEach(function(ent) {
		ent.setMetadata("role", "registeredSoldier");
		self.soldiers[ent.id()] = true;
		self.unassigned[ent.id()] = true;
	});
};

// Ungarrisons all units
MilitaryAttackManager.prototype.ungarrisonAll = function(gameState) {
	debug("ungarrison units");
	
	this.getGarrisonBuildings(gameState).forEach(function(bldg){
		bldg.unloadAll();
	});
	
	for ( var i in this.garrisoned)	{
		if(this.assigned[i]) {
			this.unassignUnit(i);
		}
		if (this.entity(i)){
			this.entity(i).setMetadata("subrole","idle");
		}
	}
	this.garrisoned = {};
};

//Garrisons citizens
MilitaryAttackManager.prototype.garrisonCitizens = function(gameState) {
	var self = this;
	debug("garrison Citizens"); 
	gameState.getOwnEntities().forEach(function(ent) {
		var dogarrison = false;
		// Look for workers which have a position (i.e. not garrisoned)
		if(ent.hasClass("Worker") && ent.position()) {
			for (id in self.enemyAttackers) {
				if(self.entity(id).visionRange() >= VectorDistance(self.entity(id).position(),ent.position())) {
					dogarrison = true;
					break;
				}
			}
			if(dogarrison) {
				self.garrisonUnit(gameState,ent.id());
			}
		}
		return true;
	});
};

// garrison the soldiers
MilitaryAttackManager.prototype.garrisonSoldiers = function(gameState) {
	debug("garrison Soldiers"); 
	var units = this.getAvailableUnits(this.countAvailableUnits());
	for (var i in units) {
		this.garrisonUnit(gameState,units[i]);
		if(!this.garrisoned[units[i]]) {
			this.unassignUnit(units[i]);
		}
	}
};

MilitaryAttackManager.prototype.garrisonUnit = function(gameState,id) {
	if (this.entity(id).position() === undefined){
		return;
	}
 	var garrisonBuildings = this.getGarrisonBuildings(gameState).toEntityArray();
	var bldgDistance = [];
	for (var i in garrisonBuildings) {
		var bldg = garrisonBuildings[i];
		if(bldg.garrisoned().length <= bldg.garrisonMax()) {
			bldgDistance.push([i,VectorDistance(bldg.position(),this.entity(id).position())]);
		}
	}
	if(bldgDistance.length > 0) {
		bldgDistance.sort(function(a,b) { return (a[1]-b[1]); });
		var building = garrisonBuildings[bldgDistance[0][0]];
		//debug("garrison id "+id+"into building "+building.id()+"walking distance "+bldgDistance[0][1]);
		this.entity(id).garrison(building);
		this.garrisoned[id] = true;
		this.entity(id).setMetadata("subrole","garrison");
	}
};

// return count of enemy buildings for a given building class
MilitaryAttackManager.prototype.getEnemyBuildings = function(gameState,cls) {
	var targets = gameState.entities.filter(function(ent) {
			return (gameState.isEntityEnemy(ent) && ent.hasClass("Structure") && ent.hasClass(cls) && ent.owner() !== 0  && ent.position());
		});
	return targets;
};

// return count of own buildings for a given building class
MilitaryAttackManager.prototype.getGarrisonBuildings = function(gameState) {
	var targets = gameState.getOwnEntities().filter(function(ent) {
			return (ent.hasClass("Structure") && ent.garrisonableClasses());
		});
	return targets;
};

// return n available units and makes these units unavailable
MilitaryAttackManager.prototype.getAvailableUnits = function(n, filter) {
	var ret = [];
	var count = 0;
	for (var i in this.unassigned) {
		if (this.unassigned[i]){
			var ent = this.entity(i);
			if (filter){
				if (!filter(ent)){
					continue;
				}
			}
			ret.push(+i);
			delete this.unassigned[i];
			this.assigned[i] = true;
			ent.setMetadata("role", "assigned");
			ent.setMetadata("subrole", "unavailable");
			count++;
			if (count >= n) {
				break;
			}
		}
	}
	return ret;
};

// Takes a single unit id, and marks it unassigned
MilitaryAttackManager.prototype.unassignUnit = function(unit){
	this.unassigned[unit] = true;
	this.assigned[unit] = false;
};

// Takes an array of unit id's and marks all of them unassigned 
MilitaryAttackManager.prototype.unassignUnits = function(units){
	for (var i in units){
		this.unassigned[units[i]] = true;
		this.assigned[units[i]] = false;
	}
};

MilitaryAttackManager.prototype.countAvailableUnits = function(filter){
	var count = 0;
	for (var i in this.unassigned){
		if (this.unassigned[i]){
			if (filter){
				if (filter(this.entity(i))){
					count += 1;
				}
			}else{
				count += 1;				
			}
		}
	}
	return count;
};

MilitaryAttackManager.prototype.handleEvents = function(gameState, events) {
	var myCivCentres = gameState.getOwnEntities().filter(function(ent) {
		return ent.hasClass("CivCentre");
	}).toEntityArray();
	var pos = undefined;
	if (myCivCentres.length > 0 && myCivCentres[0].position()){
		pos = myCivCentres[0].position();
	}
	
	for (var i in events) {
		var e = events[i];

		if (e.type === "Destroy") {
			var id = e.msg.entity;
			delete this.unassigned[id];
			delete this.assigned[id];
			delete this.soldiers[id];
		}
	}
};

// Takes an entity id and returns an entity object or false if there is no entity with that id
// Also sends a debug message warning if the id has no entity
MilitaryAttackManager.prototype.entity = function(id) {
	if (this.gameState.entities._entities[id]) {
		return new Entity(this.gameState.ai, this.gameState.entities._entities[id]);
	}else{
		//debug("Entity " + id + " requested does not exist");
	}
	return undefined;
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
	for (var i in this.unassigned){
		if (this.unassigned[i] && this.entity(i)){
			strength += this.getUnitStrength(this.entity(i));
		}
	}
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
	if (gameState.countEntitiesAndQueuedWithType(gameState.applyCiv('structures/{civ}_defense_tower'))
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
		numFortresses += gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(this.bFort[i]));
	}
	
	if (numFortresses + queues.defenceBuilding.totalLength() < gameState.getBuildLimits()["Fortress"]) {
		if (gameState.countEntitiesWithType(gameState.applyCiv("units/{civ}_support_female_citizen")) > gameState.ai.modules[0].targetNumWorkers * 0.5){
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

MilitaryAttackManager.prototype.update = function(gameState, queues, events) {

	Engine.ProfileStart("military update");
	this.gameState = gameState;

	this.handleEvents(gameState, events);

	// this.attackElephants(gameState);
	this.registerSoldiers(gameState);
	//this.defence(gameState);
	this.buildDefences(gameState, queues);
	
	this.defenceManager.update(gameState, events, this);
	
	// Continually try training new units, in batches of 5
	if (queues.citizenSoldier.length() < 6) {
		var newUnit = this.findBestNewUnit(gameState, queues.citizenSoldier, this.uCitizenSoldier);
		if (newUnit){
			queues.citizenSoldier.addItem(new UnitTrainingPlan(gameState, newUnit, {
				"role" : "soldier"
			}, 5));
		}
	}
	if (queues.advancedSoldier.length() < 2) {
		var newUnit = this.findBestNewUnit(gameState, queues.advancedSoldier, this.uAdvanced);
		if (newUnit){
			queues.advancedSoldier.addItem(new UnitTrainingPlan(gameState, newUnit, {
				"role" : "soldier"
			}, 5));
		}
	}
	if (queues.siege.length() < 4) {
		var newUnit = this.findBestNewUnit(gameState, queues.siege, this.uSiege);
		if (newUnit){
			queues.siege.addItem(new UnitTrainingPlan(gameState, newUnit, {
				"role" : "soldier"
			}, 2));
		}
	}

	// Build more military buildings
	// TODO: make military building better
	if (gameState.countEntitiesWithType(gameState.applyCiv("units/{civ}_support_female_citizen")) > 30) {
		if (gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(this.bModerate[0]))
				+ queues.militaryBuilding.totalLength() < 1) {
			queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bModerate[0]));
		}
	}
	//build advanced military buildings
	if (gameState.countEntitiesWithType(gameState.applyCiv("units/{civ}_support_female_citizen")) > 
			gameState.ai.modules[0].targetNumWorkers * 0.7){
		if (queues.militaryBuilding.totalLength() === 0){
			for (var i in this.bAdvanced){
				if (gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(this.bAdvanced[i])) < 1){
					queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bAdvanced[i]));
				}
			}
		}
	}
	
	// Look for attack plans which can be executed, only do this once every minute
	for (var i = 0; i < this.availableAttacks.length; i++){
		if (this.availableAttacks[i].canExecute(gameState, this)){
			this.availableAttacks[i].execute(gameState, this);
			this.currentAttacks.push(this.availableAttacks[i]);
			debug("Attacking!");
		}
		this.availableAttacks.splice(i, 1, new this.attackManagers[i](gameState, this));
	}

	// Keep current attacks updated
	for (i in this.currentAttacks){
		this.currentAttacks[i].update(gameState, this, events);
	}
	
	// Set unassigned to be workers
	for (var i in this.unassigned){
		if (this.entity(i).hasClass("CitizenSoldier") && ! this.entity(i).hasClass("Cavalry")){
			this.entity(i).setMetadata("role", "worker");
		}
	}
	
	// Dynamically change priorities
	
	var females = gameState.countEntitiesWithType(gameState.applyCiv("units/{civ}_support_female_citizen"));
	var femalesTarget = gameState.ai.modules[0].targetNumWorkers;
	var enemyStrength = this.measureEnemyStrength(gameState);
	var availableStrength = this.measureAvailableStrength();
	var additionalPriority = (enemyStrength - availableStrength) * 5;
	
	additionalPriority = Math.min(Math.max(additionalPriority, -50), 220);
	var advancedProportion = (availableStrength / 40) * (females/femalesTarget);
	advancedProportion = Math.min(advancedProportion, 0.7);
	gameState.ai.priorities.citizenSoldier = (1-advancedProportion) * (150 + additionalPriority) + 1;
	gameState.ai.priorities.advancedSoldier = advancedProportion * (150 + additionalPriority) + 1;
	
	if (females/femalesTarget > 0.7){
		gameState.ai.priorities.defenceBuilding = 70;
	}
	
	Engine.ProfileStop();
};
