/*
 * Military strategy:
 *   * Try training an attack squad of a specified size
 *   * When it's the appropriate size, send it to attack the enemy
 *   * Repeat forever
 *
 */

var MilitaryAttackManager = function() {
	this.targetSquadSize = 10;
	this.targetScoutTowers = 10;

	// these use the structure soldiers[unitId] = true|false to register the
	// units
	this.soldiers = {};
	this.assigned = {};
	this.unassigned = {};
	this.garrisoned = {};
	this.enemyAttackers = {};

	this.attackManagers = [AttackMoveToCC];
	this.availableAttacks = [];
	this.currentAttacks = [];
	
	this.defineUnitsAndBuildings();
};

MilitaryAttackManager.prototype.init = function(gameState) {
	var civ = gameState.playerData.civ;
	if (civ in this.uCivCitizenSoldier) {
		this.uCitizenSoldier = this.uCivCitizenSoldier[civ];
		this.uAdvanced = this.uCivAdvanced[civ];
		this.uSiege = this.uCivSiege[civ];

		this.bAdvanced = this.bCivAdvanced[civ];
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
	
	for (var i in this.attackManagers){
		this.availableAttacks[i] = new this.attackManagers[i](gameState, this);
	}
	
	var filter = Filters.and(Filters.isEnemy(), Filters.byClassesOr(["CitizenSoldier", "Super"]));
	this.enemySoldiers = new EntityCollection(gameState.ai, gameState.entities._entities, filter, gameState);
};

MilitaryAttackManager.prototype.defineUnitsAndBuildings = function(){
	// units
	this.uCivCitizenSoldier= {};
	this.uCivAdvanced = {};
	this.uCivSiege = {};
	
	this.uCivCitizenSoldier.hele = [ "units/hele_infantry_spearman_b", "units/hele_infantry_javelinist_b", "units/hele_infantry_archer_b" ];
	this.uCivAdvanced.hele = [ "units/hele_cavalry_swordsman_b", "units/hele_cavalry_javelinist_b", "units/hele_champion_cavalry_mace", "units/hele_champion_infantry_mace", "units/hele_champion_infantry_polis", "units/hele_champion_ranged_polis" , "units/thebes_sacred_band_hoplitai", "units/thespian_melanochitones","units/sparta_hellenistic_phalangitai", "units/thrace_black_cloak"];
	this.uCivSiege.hele = [ "units/hele_mechanical_siege_oxybeles", "units/hele_mechanical_siege_lithobolos" ];

	this.uCivCitizenSoldier.cart = [ "units/cart_infantry_spearman_b", "units/cart_infantry_archer_b" ];
	this.uCivAdvanced.cart = [ "units/cart_cavalry_javelinist_b", "units/cart_champion_cavalry", "units/cart_infantry_swordsman_2_b", "units/cart_cavalry_spearman_b", "units/cart_infantry_javelinist_b", "units/cart_infantry_slinger_b", "units/cart_cavalry_swordsman_b", "units/cart_infantry_swordsman_b", "units/cart_cavalry_swordsman_2_b", "units/cart_sacred_band_cavalry"];
	this.uCivSiege.cart = ["units/cart_mechanical_siege_ballista", "units/cart_mechanical_siege_oxybeles"];
	
	this.uCivCitizenSoldier.celt = [ "units/celt_infantry_spearman_b", "units/celt_infantry_javelinist_b" ];
	this.uCivAdvanced.celt = [ "units/celt_cavalry_javelinist_b", "units/celt_cavalry_swordsman_b", "units/celt_champion_cavalry_gaul", "units/celt_champion_infantry_gaul", "units/celt_champion_cavalry_brit", "units/celt_champion_infantry_brit", "units/celt_fanatic" ];
	this.uCivSiege.celt = ["units/celt_mechanical_siege_ram"];

	this.uCivCitizenSoldier.iber = [ "units/iber_infantry_spearman_b", "units/iber_infantry_slinger_b", "units/iber_infantry_swordsman_b", "units/iber_infantry_javelinist_b" ];
	this.uCivAdvanced.iber = ["units/iber_cavalry_spearman_b", "units/iber_champion_cavalry", "units/iber_champion_infantry" ];
	this.uCivSiege.iber = ["units/iber_mechanical_siege_ram"];
	
	//defaults
	this.uCitizenSoldier = ["units/{civ}_infantry_spearman_b", "units/{civ}_infantry_slinger_b", "units/{civ}_infantry_swordsman_b", "units/{civ}_infantry_javelinist_b", "units/{civ}_infantry_archer_b" ];
	this.uAdvanced = ["units/{civ}_cavalry_spearman_b", "units/{civ}_cavalry_javelinist_b", "units/{civ}_champion_cavalry", "units/{civ}_champion_infantry"];
	this.uSiege = ["units/{civ}_mechanical_siege_oxybeles", "units/{civ}_mechanical_siege_lithobolos", "units/{civ}_mechanical_siege_ballista","units/{civ}_mechanical_siege_ram"];

	// buildings
	this.bModerate = [ "structures/{civ}_barracks" ]; //same for all civs

	this.bCivAdvanced = {};
	this.bCivAdvanced.hele = [ "structures/{civ}_gymnasion", "structures/{civ}_fortress" ];
	this.bCivAdvanced.cart = [ "structures/{civ}_fortress", "structures/{civ}_embassy_celtic", "structures/{civ}_embassy_iberian", "structures/{civ}_embassy_italiote" ];
	this.bCivAdvanced.celt = [ "structures/{civ}_kennel", "structures/{civ}_fortress_b", "structures/{civ}_fortress_g" ];
	this.bCivAdvanced.iber = [ "structures/{civ}_fortress" ];
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

MilitaryAttackManager.prototype.attackElephants = function(gameState) {
	var eles = gameState.entities.filter(function(ent) {
		return (ent.templateName().indexOf("elephant") > -1);
	});

	warn(uneval(eles._entities));
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

MilitaryAttackManager.prototype.defence = function(gameState) {
	var ents = gameState.entities._entities;

	var myCivCentres = gameState.getOwnEntities().filter(function(ent) {
		return ent.hasClass("CivCentre");
	});

	if (myCivCentres.length === 0)
		return;

	var defenceRange = 200; // just beyond town centres territory influence
	var self = this;
	
	var newEnemyAttackers = {};

	myCivCentres.forEach(function(ent) {
		var pos = ent.position();
		self.getEnemySoldiers(gameState).forEach(function(ent) {
			if (gameState.playerData.isEnemy[ent.owner()]
					&& (ent.hasClass("CitizenSoldier") || ent.hasClass("Super"))
					&& ent.position()) {
				var dist = VectorDistance(ent.position(), pos);
				if (dist < defenceRange) {
					newEnemyAttackers[ent.id()] = true;
				}
			}
		});
	});
	
	for (var id in this.enemyAttackers){
		if (!newEnemyAttackers[id]){
			this.unassignDefenders(gameState, id);
		}
	}
	
	this.enemyAttackers = newEnemyAttackers;
	
	var enemyAttackStrength = 0;
	var availableStrength = this.measureAvailableStrength();
	var garrisonedStrength = 0;
	for (var i in this.garrisoned){
		if (this.entity(i) !== undefined){
			if (Filters.isSoldier()(this.entity(i))){
				garrisonedStrength += this.getUnitStrength(this.entity(i));
			}
		}
	}
	
	for (var id in this.enemyAttackers) {
		var ent = new Entity(gameState.ai, ents[id]);
		enemyAttackStrength+= this.getUnitStrength(ent);
	}

	if(2 * enemyAttackStrength < availableStrength + garrisonedStrength) {
		this.ungarrisonAll(gameState);
		return;
	} else {
		this.garrisonCitizens(gameState);
	}

	if(enemyAttackStrength > availableStrength + garrisonedStrength) {
		this.garrisonSoldiers(gameState);
	}

	for (id in this.enemyAttackers) {
		if(!this.assignDefenders(gameState,id)) {
			break;
		}
	}
	
};

MilitaryAttackManager.prototype.assignDefenders = function(gameState,target) {
	var defendersPerAttacker = 3;
	var ent = new Entity(gameState.ai, gameState.entities._entities[target]);
	if (ent.getMetadata("attackers") === undefined || ent.getMetadata("attackers").length < defendersPerAttacker) {
		var tasked = this.getAvailableUnits(3);
		if (tasked.length > 0) {
			Engine.PostCommand({
				"type" : "attack",
				"entities" : tasked,
				"target" : ent.id(),
				"queued" : false
				});
			ent.setMetadata("attackers", tasked);
			for (var i in tasked) {
				this.entity(tasked[i]).setMetadata("attacking", id);
			}
		} else {
			return false;
		}
	}
	return true;
};

MilitaryAttackManager.prototype.unassignDefenders = function(gameState, target){
	var myCivCentres = gameState.getOwnEntities().filter(function(ent) {
		return ent.hasClass("CivCentre");
	}).toEntityArray();
	var pos = undefined;
	if (myCivCentres.length > 0 && myCivCentres[0].position()){
		pos = myCivCentres[0].position();
	}
	
	var ent = this.entity(target);
	if (ent && ent.getMetadata() && ent.getMetadata().attackers){
		for (var i in ent.metadata.attackers){
			var attacker = this.entity(ent.getMetadata().attackers[i]);
			if (attacker){
				attacker.deleteMetadata('attacking');
				if (pos){
					attacker.move(pos[0], pos[1]);
				}
				this.unassignUnit(attacker.id());
			}
		}
		ent.deleteMetadata('attackers');
	}
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
			return (gameState.isEntityEnemy(ent) && ent.hasClass("Structure") && ent.hasClass(cls) && ent.owner() !== 0);
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
MilitaryAttackManager.prototype.getAvailableUnits = function(n) {
	var ret = [];
	var count = 0;
	for (var i in this.unassigned) {
		ret.push(+i);
		delete this.unassigned[i];
		this.assigned[i] = true;
		this.entity(i).setMetadata("role", "soldier");
		this.entity(i).setMetadata("subrole", "unavailable");
		count++;
		if (count >= n) {
			break;
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
		this.unassigned[unit[i]] = true;
		this.assigned[unit[i]] = false;
	}
};

MilitaryAttackManager.prototype.countAvailableUnits = function(){
	var count = 0;
	for (var i in this.unassigned){
		if (this.unassigned[i]){
			count += 1;
		}
	}
	return count;
};

MilitaryAttackManager.prototype.handleEvents = function(gameState, events) {
	for (var i in events) {
		var e = events[i];

		if (e.type === "Destroy") {
			var id = e.msg.entity;
			delete this.unassigned[id];
			delete this.assigned[id];
			delete this.soldiers[id];
			var metadata = e.msg.metadata[gameState.ai._player];
			if (metadata && metadata.attacking){
				var attacking = this.entity(metadata.attacking);
				if (attacking && attacking.getMetadata('attackers')){
					var attackers = attacking.getMetadata('attackers');
					attackers.splice(attackers.indexOf(metadata.attacking), 1);
					attacking.setMetadata('attackers', attackers);
				}
			}
			if (metadata && metadata.attackers){
				for (var i in metadata.attackers){
					var attacker = this.entity(metadata.attackers[i]);
					if (attacker && attacker.getMetadata('attacking')){
						attacker.deleteMetadata('attacking');
						this.unassignUnit(attacker.id());
					}
				}
			}
		}
	}
};

// Takes an entity id and returns an entity object or false if there is no entity with that id
// Also sends a debug message warning if the id has no entity
MilitaryAttackManager.prototype.entity = function(id) {
	if (this.gameState.entities._entities[id]) {
		return new Entity(this.gameState.ai, this.gameState.entities._entities[id]);
	}else{
		debug("Entity " + id + " requested does not exist");
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
		if (this.unassigned[i]){
			strength += this.getUnitStrength(this.entity(i));
		}
	}
	return strength;
};

MilitaryAttackManager.prototype.getEnemySoldiers = function(gameState){
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
	if (gameState.countEntitiesAndQueuedWithType(gameState.applyCiv('structures/{civ}_scout_tower'))
			+ queues.defenceBuilding.totalLength() <= gameState.getBuildLimits()["ScoutTower"]) {
		queues.defenceBuilding.addItem(new BuildingConstructionPlan(gameState, 'structures/{civ}_scout_tower'));
	}
};

MilitaryAttackManager.prototype.update = function(gameState, queues, events) {

	Engine.ProfileStart("military update");
	this.gameState = gameState;

	this.handleEvents(gameState, events);

	// this.attackElephants(gameState);
	this.registerSoldiers(gameState);
	this.defence(gameState);
	this.buildDefences(gameState, queues);
	
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
			gameState.ai.modules[0].targetNumWorkers * 0.8){
		if (queues.militaryBuilding.totalLength() === 0){
			for (var i in this.bAdvanced){
				if (gameState.countEntitiesAndQueuedWithType(gameState.applyCiv(this.bAdvanced[i])) < 1){
					queues.militaryBuilding.addItem(new BuildingConstructionPlan(gameState, this.bAdvanced[i]));
				}
			}
		}
	}
	
	// Look for attack plans which can be executed
	for (var i = 0; i < this.availableAttacks.length; i++){
		if (this.availableAttacks[i].canExecute(gameState, this)){
			this.availableAttacks[i].execute(gameState, this);
			this.currentAttacks.push(this.availableAttacks[i]);
			this.availableAttacks.splice(i, 1, new this.attackManagers[i](gameState, this));
		}
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

	Engine.ProfileStop();
};
