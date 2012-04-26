/*
 * Military strategy:
 *   * Try training an attack squad of a specified size
 *   * When it's the appropriate size, send it to attack the enemy
 *   * Repeat forever
 *
 */

var MilitaryAttackManager = Class({

	_init: function()
	{
		this.baserate = 11;
		this.defsquad = 10;
		this.defsquadmin = 5;
		this.findatype = 1;
		this.killstrat = 0;
		this.changetime = 60*1000;
		this.changetimeReg = 60*5000;
		this.changetimeRegDef = 60*1000;
		this.changetimeRegWaiting = 60*4000;
		this.attacknumbers = 0.4
		this.squadTypes = [
			"units/{civ}_infantry_spearman_b",
			"units/{civ}_infantry_javelinist_b",
//			"units/{civ}_infantry_archer_b", // TODO: should only include this if hele
		];
	},

	/**
	 * Returns the unit type we should begin training.
	 * (Currently this is whatever we have least of.)
	 */
	findBestNewUnit: function(gameState)
	{
		// Count each type
		var types = [];
		for each (var t in this.squadTypes)
			types.push([t, gameState.countEntitiesAndQueuedWithType(t)]);

		// Sort by increasing count
		types.sort(function (a, b) { return a[1] - b[1]; });

		// TODO: we shouldn't return units that we don't have any
		// buildings capable of training
		// Let's make this shizz random...
		var randomiser = Math.floor(Math.random()*types.length);
		return types[randomiser][0];
	},

	regroup: function(gameState, planGroups)
	{
			if (gameState.getTimeElapsed() > this.changetimeReg && this.killstrat != 3){
			var regroupneeded = gameState.getOwnRoleGroup("attack");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_3p1");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_3p2");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_3p3");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_pending_3p1");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_pending_3p2");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_pending_3p3");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneeded = gameState.getOwnRoleGroup("fighting");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending");
				});
			var regroupneededPartB = gameState.getOwnRoleGroup("attack_pending");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("CivCentre");
			if (targets.length){
				var target = targets.toEntityArray()[0];
				var targetPos = target.position();

				// TODO: this should be an attack-move command
				regroupneededPartB.move(targetPos[0], targetPos[1]);
			}
			// Wait 4 mins to do this again.
			this.changetimeReg = this.changetimeReg + (60*4000);
			}
			else if (gameState.getTimeElapsed() > this.changetimeReg && this.killstrat == 3){
			var regroupneeded = gameState.getOwnRoleGroup("attack");
				regroupneeded.forEach(function(ent) {
					var section = Math.random();
					if (section < 0.3){
					ent.setMetadata("role", "attack_pending_3p1");
					}
					else if (section < 0.6){
					ent.setMetadata("role", "attack_pending_3p2");
					}
					else {
					ent.setMetadata("role", "attack_pending_3p3");
					}
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_pending");
				regroupneeded.forEach(function(ent) {
					var section = Math.random();
					if (section < 0.3){
					ent.setMetadata("role", "attack_pending_3p1");
					}
					else if (section < 0.6){
					ent.setMetadata("role", "attack_pending_3p2");
					}
					else {
					ent.setMetadata("role", "attack_pending_3p3");
					}
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_3p1");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending_3p1");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_3p2");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending_3p2");
				});
			var regroupneeded = gameState.getOwnRoleGroup("attack_3p3");
				regroupneeded.forEach(function(ent) {
					ent.setMetadata("role", "attack_pending_3p3");
				});
			var regroupneeded = gameState.getOwnRoleGroup("fighting");
				regroupneeded.forEach(function(ent) {
					var section = Math.random();
					if (section < 0.3){
					ent.setMetadata("role", "attack_pending_3p1");
					}
					else if (section < 0.6){
					ent.setMetadata("role", "attack_pending_3p2");
					}
					else {
					ent.setMetadata("role", "attack_pending_3p3");
					}
				});
				// MOVE THEM ALL
				// GROUP ONE
			var regroupneededPartB = gameState.getOwnRoleGroup("attack_pending_3p1");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("CivCentre");
			if (targets.length){
				var target = targets.toEntityArray()[0];
				var targetPos = target.position();

				// TODO: this should be an attack-move command
				regroupneededPartB.move(targetPos[0], targetPos[1]);
			}
				// MOVING GROUP TWO
			var regroupneededPartB = gameState.getOwnRoleGroup("attack_pending_3p2");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("CivCentre");
			if (targets.length){
				var target = targets.toEntityArray()[0];
				var targetPos = target.position();

				// TODO: this should be an attack-move command
				regroupneededPartB.move(targetPos[0], targetPos[1]);
			}
				// MOVING GROUP THREE
			var regroupneededPartB = gameState.getOwnRoleGroup("attack_pending_3p3");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("CivCentre");
			if (targets.length){
				var target = targets.toEntityArray()[0];
				var targetPos = target.position();

				// TODO: this should be an attack-move command
				regroupneededPartB.move(targetPos[0], targetPos[1]);
			}
			// Wait 4 mins to do this again.
			this.changetimeReg = this.changetimeReg + (60*4000);
			}
	},
	
	combatcheck: function(gameState, planGroups, assaultgroup)
	{
			var regroupneeded = gameState.getOwnRoleGroup(assaultgroup);
				regroupneeded.forEach(function(troop) {
				var currentPosition = troop.position();
			var targets = gameState.getJustEnemies().filter(function(ent) {
				var foeposition = ent.position();
				if (foeposition){
				var dist = SquareVectorDistance(foeposition, currentPosition);
				return (ent.isEnemy() && ent.owner()!= 0 && dist < 2500);
				}
				else {
				return false;
				}
			});
			if (targets.length >= 5){
				regroupneeded.forEach(function(person) {
				var targetrandomiser = Math.floor(Math.random()*targets.length);
				var target = targets.toEntityArray()[targetrandomiser];
				var targetPos = target.position();
				// TODO: this should be an attack-move command
				person.move(targetPos[0], targetPos[1]);
				person.setMetadata("role", "fighting");
				});
			}
				});	
	},
	
	CombatAnalyser: function(gameState, planGroups, startunit)
	{
				var centrepoint = startunit.position();
					var targets = gameState.entities.filter(function(squeak) {
				var currentPosition = squeak.position();
				if (currentPosition){
				var dist = SquareVectorDistance(foeposition, currentPosition);
				return (dist < 2500);
				}
				else {
				return false;
				}
			});
			if (targets.length)
			{
			var forest = 0;
			var foeTot = 0;
			var foeCav = 0;
			var foeInf = 0;
			var foeBow = 0;
			var foeRangeCav = 0;
				targets.forEach(function(item) {
				if (item.hasClass("ForestPlant")){
				forest += 1;
				}
				if (item.isEnemy() && item.owner != 0 && item.hasClass("Infantry") && item.hasClass("Melee")){
				foeTot += 1;
				foeInf += 1;
				}
				if (item.isEnemy() && item.owner != 0 && item.hasClass("Infantry" && item.hasClass("Ranged"))){
				foeTot += 1;
				foeBow += 1;
				}
				if (item.isEnemy() && item.owner != 0 && item.hasClass("Cavalry") && item.hasClass("Melee")){
				foeTot += 1;
				foeCav += 1;
				}
				if (item.isEnemy() && item.owner != 0 && item.hasClass("Cavalry" && item.hasClass("Ranged"))){
				foeTot += 1;
				foeRangeCav += 1;
				}
				});
			}

	},
	
	combatcheckMilitia: function(gameState, planGroups, assaultgroup)
	{
			var regroupneeded = gameState.getOwnRoleGroup(assaultgroup);
				regroupneeded.forEach(function(troop) {
				var currentPosition = troop.position();
			// Find nearby enemies
			var targets = gameState.getJustEnemies().filter(function(ent) {
				var foeposition = ent.position();
				if (foeposition){
				var dist = SquareVectorDistance(foeposition, currentPosition);
				return (dist < 2500);
				}
				else {
				return false;
				}
			});
			// Check that some of our own buildings are nearby
			var ownbuildings = gameState.getOwnWithClass("Village").filter(function(ent) {
				var foeposition = ent.position();
				if (foeposition){
				var dist = SquareVectorDistance(foeposition, currentPosition);
				return (dist < 2500);
				}
				else {
				return false;
				}
			});
			if (targets.length >= 5 && ownbuildings.length > 0){
				var position = ownbuildings.toEntityArray()[0].position();
				regroupneeded.forEach(function(person) {
				var ourposition = person.position();
				var distance = SquareVectorDistance(position, ourposition);
				if (distance <= 22500){
				var owntargetrandomiser = Math.floor(Math.random()*ownbuildings.length);
				var owntarget = ownbuildings.toEntityArray()[owntargetrandomiser];
				var owntargetPos = owntarget.position();
				// TODO: this should be an attack-move command
				person.move(owntargetPos[0], owntargetPos[1]);
				person.setMetadata("role", "militiafighter");
				}
				});
			}
				});	
	},

	defenseregroup: function(gameState, planGroups)
	{
			if (gameState.getTimeElapsed() > this.changetimeRegDef){
			// Send them home
					var targets = gameState.entities.filter(function(squeak) {
				return (!squeak.isEnemy() && squeak.hasClass("Village"));
			});
			if (targets.length)
			{
				var targetrandomiser = Math.floor(Math.random()*targets.length);
				var target = targets.toEntityArray()[targetrandomiser];
				var targetPos = target.position();
			}
			// Change 'em back to militia
			var defenseregroupers = gameState.getOwnRoleGroup("militiafighter");
		defenseregroupers.forEach(function(ent) {
			// If we have a target to go home to, move to it
				ent.move(targetPos[0], targetPos[1]);
				ent.setMetadata("role", "militia");
		});
			// Wait 45 secs to do this again.
			this.changetimeRegDef = this.changetimeRegDef + (30*1000);
			}
	},
	
	waitingregroup: function(gameState, planGroups)
	{
			if (gameState.getTimeElapsed() > this.changetimeRegWaiting){
			var regroupneededPartC = gameState.getOwnRoleGroup("attack_pending");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("Village");
					// If we have a target, move to it
			if (targets.length)
			{
				var targetrandomiser = Math.floor(Math.random()*targets.length);
				var target = targets.toEntityArray()[targetrandomiser];
				var targetPos = target.position();
				// TODO: this should be an attack-move command
				regroupneededPartC.move(targetPos[0], targetPos[1]);
			}
			var regroupneededPartC = gameState.getOwnRoleGroup("attack_pending_3p1");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("Village");
					// If we have a target, move to it
			if (targets.length)
			{
				var targetrandomiser = Math.floor(Math.random()*targets.length);
				var target = targets.toEntityArray()[targetrandomiser];
				var targetPos = target.position();
				// TODO: this should be an attack-move command
				regroupneededPartC.move(targetPos[0], targetPos[1]);
			}
			var regroupneededPartC = gameState.getOwnRoleGroup("attack_pending_3p2");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("Village");
					// If we have a target, move to it
			if (targets.length)
			{
				var targetrandomiser = Math.floor(Math.random()*targets.length);
				var target = targets.toEntityArray()[targetrandomiser];
				var targetPos = target.position();
				// TODO: this should be an attack-move command
				regroupneededPartC.move(targetPos[0], targetPos[1]);
			}
			var regroupneededPartC = gameState.getOwnRoleGroup("attack_pending_3p3");
				//Find a friendsly CC
			var targets = gameState.getOwnWithClass("Village");
					// If we have a target, move to it
			if (targets.length)
			{
				var targetrandomiser = Math.floor(Math.random()*targets.length);
				var target = targets.toEntityArray()[targetrandomiser];
				var targetPos = target.position();
				// TODO: this should be an attack-move command
				regroupneededPartC.move(targetPos[0], targetPos[1]);
			}
			// Wait 4 mins to do this again.
			this.changetimeRegWaiting = this.changetimeRegWaiting + (60*1500);
			}
	},
	

		
	trainDefenderSquad: function(gameState, planGroups)
	{
		var trainup = 0;
		var pendingdefense = gameState.countEntitiesAndQueuedWithRole("defenders");
			var targets = gameState.getOwnWithClass("GarrisonTower");
		if (targets.length)
		{
			targets.forEach(function(tower) {
			if (tower.foundationProgress() === undefined){
			var defno = tower.garrisoned().length;
			var defneed = 2 - defno;
			//warn("Need " + defneed + " men in the tower, with " + pendingdefense + " available.");
			if (defneed >= 1) {
				if (pendingdefense >= 1) {
					gameState.getOwnRoleGroup("defenders").forEach(function(ent) {
						ent.garrison(tower);
						ent.setMetadata("role", "towerGuard");
					});
				}
				else {
				trainup = 1;
				}
				}
				}
				});
			}
		var targetsII = gameState.getOwnWithClass("GarrisonFortress");
		if (targetsII.length)
		{
			targetsII.forEach(function(tower) {
			if (tower.foundationProgress() === undefined){
			var defno = tower.garrisoned().length;
			var defneed = 5 - defno;
			//warn("Need " + defneed + " men in the tower, with " + pendingdefense + " available.");
			if (defneed >= 1) {
				if (pendingdefense >= 1) {
					gameState.getOwnRoleGroup("defenders").forEach(function(ent) {
						ent.garrison(tower);
						ent.setMetadata("role", "towerGuard");
					});
				}
				else {
				trainup = 1;
				}
				}
				}
				});
			}
		if (trainup == 1){
			if (gameState.displayCiv() == "hele"){
				this.trainSomeDefenders(gameState, planGroups, "units/{civ}_infantry_archer_b");
				}
			else if  (gameState.displayCiv() == "cart"){
				this.trainSomeDefenders(gameState, planGroups, "units/{civ}_infantry_archer_b");
				}
			else if  (gameState.displayCiv() == "rome"){
				this.trainSomeDefenders(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
				}
			else if  (gameState.displayCiv() == "iber"){
				this.trainSomeDefenders(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
				}
			else if  (gameState.displayCiv() == "celt"){
				this.trainSomeDefenders(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
				}
			else if  (gameState.displayCiv() == "pers"){
				this.trainSomeDefenders(gameState, planGroups, "units/{civ}_infantry_archer_b");
				}
			else {
				this.trainSomeDefenders(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
				}
		}
	},
	
	trainSomeDefenders: function(gameState, planGroups, type)
	{
			var trainers = gameState.findTrainers(gameState.applyCiv(type));
			if (trainers.length != 0){
			planGroups.economyPersonnel.addPlan(150,
				new UnitTrainingPlan(gameState,
					type, 1, { "role": "defenders" })
			);
			}
	},
		
	trainSomeTroops: function(gameState, planGroups, type)
	{
			var trainers = gameState.findTrainers(gameState.applyCiv(type));
			if (trainers.length != 0){
			planGroups.economyPersonnel.addPlan(150,
				new UnitTrainingPlan(gameState,
					type, 3, { "role": "attack_pending" })
			);
			}
			else {
			this.attacknumbers = 0.99;			
			}
	},
	
	trainMachine: function(gameState, planGroups, type)
	{
			var trainers = gameState.findTrainers(gameState.applyCiv(type));
			if (trainers.length != 0){
			planGroups.economyPersonnel.addPlan(150,
				new UnitTrainingPlan(gameState,
					type, 1, { "role": "attack_pending" })
			);
			}
			else {
			this.attacknumbers = 0.99;			
			}
	},
	
	trainSomeTroops3prong: function(gameState, planGroups, type)
	{
			var trainers = gameState.findTrainers(gameState.applyCiv(type));
			var section = Math.random();
			if (trainers.length != 0 && section < 0.3){
			planGroups.economyPersonnel.addPlan(150,
				new UnitTrainingPlan(gameState,
					type, 3, { "role": "attack_pending_3p1" })
			);
			}
			else if (trainers.length != 0 && section < 0.6){
			planGroups.economyPersonnel.addPlan(150,
				new UnitTrainingPlan(gameState,
					type, 3, { "role": "attack_pending_3p2" })
			);
			}
			else if (trainers.length != 0){
			planGroups.economyPersonnel.addPlan(150,
				new UnitTrainingPlan(gameState,
					type, 3, { "role": "attack_pending_3p3" })
			);
			}
			else {
			this.attacknumbers = 0.99;			
			}
	},
	
	trainAttackSquad: function(gameState, planGroups)
	{
			if (gameState.getTimeElapsed() > this.changetime){
			this.attacknumbers = Math.random();
			this.changetime = this.changetime + (60*1000);
			}
			// Training lists for full assaults
			if (this.killstrat == 1){
			//Greeks
				if (gameState.displayCiv() == "hele"){
					if (this.attacknumbers < 0.19){
					this.trainSomeTroops(gameState, planGroups, "units/hele_champion_infantry_polis");
					return;
					}
					else if (this.attacknumbers < 0.26){
					this.trainSomeTroops(gameState, planGroups, "units/hele_champion_ranged_polis");
					return;
					}
					else if (this.attacknumbers < 0.35){
					this.trainSomeTroops(gameState, planGroups, "units/hele_champion_cavalry_mace");
					return;
					}
					else if (this.attacknumbers < 0.45){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else if (this.attacknumbers < 0.55){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.65){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else if (this.attacknumbers < 0.85){
					this.trainMachine(gameState, planGroups, "units/hele_mechanical_siege_lithobolos");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Celts
				else if (gameState.displayCiv() == "celt"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/celt_champion_infantry_brit");
					return;
					}
					else if (this.attacknumbers < 0.45){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.6){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else if (this.attacknumbers < 0.8){
					this.trainMachine(gameState, planGroups, "units/celt_mechanical_siege_ram");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Iberians
				else if (gameState.displayCiv() == "iber"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/iber_champion_infantry");
					return;
					}
					else if (this.attacknumbers < 0.4){
					this.trainSomeTroops(gameState, planGroups, "units/iber_champion_cavalry");
					return;
					}
					else if (this.attacknumbers < 0.5){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_slinger_b");
					return;
					}
					else if (this.attacknumbers < 0.6){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.7){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.9){
					this.trainMachine(gameState, planGroups, "units/iber_mechanical_siege_ram");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
				}
			//Carthaginians
				else if (gameState.displayCiv() == "cart"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/cart_champion_infantry");
					return;
					}
					else if (this.attacknumbers < 0.4){
					this.trainSomeTroops(gameState, planGroups, "units/cart_champion_cavalry");
					return;
					}
					else if (this.attacknumbers < 0.5){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else if (this.attacknumbers < 0.6){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.65){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else if (this.attacknumbers < 0.7){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.8){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_2_b");
					return;
					}
					else if (this.attacknumbers < 0.85){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_swordsman_2_b");
					return;
					}
					else if (this.attacknumbers < 0.9){
					this.trainMachine(gameState, planGroups, "units/cart_mechanical_siege_ballista");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Persians
				else if (gameState.displayCiv() == "pers"){
					if (this.attacknumbers < 0.15){
					this.trainSomeTroops(gameState, planGroups, "units/pers_champion_infantry");
					return;
					}
					else if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/pers_champion_cavalry");
					return;
					}
					else if (this.attacknumbers < 0.35){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else if (this.attacknumbers < 0.5){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.55){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else if (this.attacknumbers < 0.65){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.75){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.85){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_archer_b");
					return;
					}
					else if (this.attacknumbers < 0.9){
					this.trainMachine(gameState, planGroups, "units/pers_mechanical_siege_ram");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Romans
				else if (gameState.displayCiv() == "rome"){
					if (this.attacknumbers < 0.15){
					this.trainSomeTroops(gameState, planGroups, "units/rome_champion_infantry");
					return;
					}
					else if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/rome_champion_cavalry");
					return;
					}
					else if (this.attacknumbers < 0.35){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else if (this.attacknumbers < 0.65){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.75){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.85){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.9){
					this.trainMachine(gameState, planGroups, "units/pers_mechanical_siege_ballista");
					return;
					}
					else if (this.attacknumbers < 0.95){
					this.trainMachine(gameState, planGroups, "units/pers_mechanical_siege_ram");
					return;
					}
					else if (this.attacknumbers < 0.97){
					this.trainMachine(gameState, planGroups, "units/pers_mechanical_siege_scorpio");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
				}
			}
			// Cav raiders training list
			else if (this.killstrat == 2){
			//Greeks
				if (gameState.displayCiv() == "hele"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_javelinist_b");
					return;
					}
					else if (this.attacknumbers < 0.5){
					this.trainSomeTroops(gameState, planGroups, "units/hele_champion_cavalry_mace");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
				}
			//Celts
				else if (gameState.displayCiv() == "celt"){
					if (this.attacknumbers < 0.45){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.6){
					this.trainSomeTroops(gameState, planGroups, "units/celt_champion_cavalry_brit");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_javelinist_b");
					return;
					}
				}
			//Iberians
				else if (gameState.displayCiv() == "iber"){
					if (this.attacknumbers < 0.2){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.4){
					this.trainSomeTroops(gameState, planGroups, "units/iber_super_cavalry");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
				}
			//Carts
				else if (gameState.displayCiv() == "cart"){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
			}
			//Pers
				else if (gameState.displayCiv() == "pers"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.50){
					this.trainSomeTroops(gameState, planGroups, "units/pers_cavalry_archer_b");
					return;
					}
					else if (this.attacknumbers < 0.75){
					this.trainSomeTroops(gameState, planGroups, "units/pers_cavalry_swordsman_b");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/pers_cavalry_javelinist_b");
					return;
					}
			}
			//Romans
				else if (gameState.displayCiv() == "rome"){
					if (this.attacknumbers < 0.4){
					this.trainSomeTroops(gameState, planGroups, "units/rome_champion_cavalry");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
				}
			}
			// 3 prong attack training list
			else if (this.killstrat == 3){
			//Greeks
				if (gameState.displayCiv() == "hele"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else if (this.attacknumbers < 0.5){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Celts
				else if (gameState.displayCiv() == "celt"){
					if (this.attacknumbers < 0.45){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.6){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Iberians
				else if (gameState.displayCiv() == "iber"){
					if (this.attacknumbers < 0.2){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.4){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_slinger_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
				}
			//Carts
				else if (gameState.displayCiv() == "cart"){
					if (this.attacknumbers < 0.2){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.4){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
			}
			//Pers
				else if (gameState.displayCiv() == "pers"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.55){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
			}
			//Rome
				else if (gameState.displayCiv() == "rome"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.50){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
			}
			}
			// Generic training list
			else {
			//Greeks
				if (gameState.displayCiv() == "hele"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else if (this.attacknumbers < 0.5){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Celts
				else if (gameState.displayCiv() == "celt"){
					if (this.attacknumbers < 0.45){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.6){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Iberians
				else if (gameState.displayCiv() == "iber"){
					if (this.attacknumbers < 0.2){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.4){
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_slinger_b");
					return;
					}
					else {
					this.trainSomeTroops(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
				}
			//Carts
				else if (gameState.displayCiv() == "cart"){
					if (this.attacknumbers < 0.2){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_swordsman_b");
					return;
					}
					else if (this.attacknumbers < 0.4){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
				}
			//Pers
				else if (gameState.displayCiv() == "pers"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.55){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_archer_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_spearman_b");
					return;
					}
			}
			//Rome
				else if (gameState.displayCiv() == "rome"){
					if (this.attacknumbers < 0.25){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_cavalry_spearman_b");
					return;
					}
					else if (this.attacknumbers < 0.50){
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_javelinist_b");
					return;
					}
					else {
					this.trainSomeTroops3prong(gameState, planGroups, "units/{civ}_infantry_swordsman_b");
					return;
					}
			}
			}
	},
	
	nextAttack: function(gameState, planGroups){
	//Now set whether to do a raid or full attack next time
		var whatnext = Math.random();
		if (whatnext > 0.8){
		this.killstrat = 0;
		return;
		// Regular "train a few guys and go kill stuff" type attack.
		}
		else if (whatnext > 0.5) {
		this.killstrat = 2;
		return;
		// Cavalry raid
		}
		else if (whatnext > 0.4) {
		this.killstrat = 3;
		return;
		// 3 prong
		}
		else {
		this.killstrat = 1;
		return;
		//Full Assault!
		}
	},
	
	update: function(gameState, planGroups)
	{
		// Pause for a minute before starting any work, to give the economy a chance
		// to start up
		if (gameState.getTimeElapsed() < 60*1000)
			return;

		Engine.ProfileStart("military update");
		// Also train up some defenders

		this.combatcheck(gameState, planGroups, "attack");
		this.combatcheck(gameState, planGroups, "attack_3p1");
		this.combatcheck(gameState, planGroups, "attack_3p2");
		this.combatcheck(gameState, planGroups, "attack_3p3");
		this.combatcheckMilitia(gameState, planGroups, "militia");
		this.trainDefenderSquad(gameState, planGroups);
		this.trainAttackSquad(gameState, planGroups);
		this.regroup(gameState, planGroups);
		this.defenseregroup(gameState, planGroups);
		this.waitingregroup(gameState, planGroups);

// Variable for impetuousness, so squads vary in size.
		if (this.killstrat == 1){
		this.baserate = 31;
		}
		else if (this.killstrat == 2) {
		this.baserate = 10;
		}
		else if (this.killstrat == 3) {
		this.baserate = 27;
		}
		else {
		this.baserate = 15;
		}
		// Check we're doing a normal, not 3 pronged, attack
		if (this.killstrat != 3){
		// Find the units ready to join the attack
		var pending = gameState.getOwnRoleGroup("attack_pending");
		if (pending.length >= this.baserate)
		{
		//Point full assaults at civ centres
		if (this.killstrat == 1){
			// Find the enemy CCs we could attack
			var targets = gameState.getEnemiesWithClass("CivCentre");
			// If there's no CCs, attack anything else that's critical
			if (targets.length == 0)
			{
			var targets = gameState.getEnemiesWithClass("ConquestCritical");
			}
		}
		//Other attacks can go to any low-level structure
		else {	
			// Find the enemy dropsites we could attack
			var targets = gameState.getEnemiesWithClass("Economic");
			// If there's no dropsites, attack any village structure
			if (targets.length == 0)
			{
			var targets = gameState.getEnemiesWithClass("Village");
			}
			// If not, go for a critical thingy
			if (targets.length == 0)
			{
			var targets = gameState.getEnemiesWithClass("CivCentre");
			}
			// If not, go for a critical thingy
			if (targets.length == 0)
			{
			var targets = gameState.getEnemiesWithClass("ConquestCritical");
			}
		}
			// If we have a target, move to it
			if (targets.length)
			{
				// Remove the pending role
				pending.forEach(function(ent) {
					ent.setMetadata("role", "attack");
				});
				var targetrandomiser = Math.floor(Math.random()*targets.length);
				var target = targets.toEntityArray()[targetrandomiser];
				var targetPos = target.position();
				// TODO: this should be an attack-move command
				var assaultforce = gameState.getOwnRoleGroup("attack");
				assaultforce.move(targetPos[0], targetPos[1]);
				var otherguys = gameState.getOwnRoleGroup("randomcannonfodder");
				otherguys.move(targetPos[0], targetPos[1]);
			}
			this.nextAttack();
			
		}
		
		}
		// Here's the 3 pronged attack
		else{
		// Find the units ready to join the attack
		var pending1 = gameState.getOwnRoleGroup("attack_pending_3p1");
		var pending2 = gameState.getOwnRoleGroup("attack_pending_3p2");
		var pending3 = gameState.getOwnRoleGroup("attack_pending_3p3");
		var pendingtot = pending1.length + pending2.length + pending3.length;
		if (pendingtot >= this.baserate)
		{
		//Copy the target selector 3 times, once per attack squad
			var targets1 = gameState.getEnemiesWithClass("Village");
			// If we have a target, move to it
			if (targets1.length)
			{
				// Remove the pending role
				pending1.forEach(function(ent) {
					ent.setMetadata("role", "attack_3p1");
				});
				var targetrandomiser1 = Math.floor(Math.random()*targets1.length);
				var target1 = targets1.toEntityArray()[targetrandomiser1];
				var targetPos1 = target1.position();
				var assaultforce1 = gameState.getOwnRoleGroup("attack_3p1");
				assaultforce1.move(targetPos[0], targetPos[1]);
				var otherguys = gameState.getOwnEntitiesWithRole("randomcannonfodder");
				otherguys.move(targetPos1[0], targetPos1[1]);
			}
			var targets2 = gameState.getEnemiesWithClass("Village");
			// If we have a target, move to it
			if (targets2.length)
			{
				// Remove the pending role
				pending2.forEach(function(ent) {
					ent.setMetadata("role", "attack_3p2");
				});
				var targetrandomiser2 = Math.floor(Math.random()*targets2.length);
				var target2 = targets2.toEntityArray()[targetrandomiser2];
				var targetPos2 = target2.position();
				var assaultforce2 = gameState.getOwnRoleGroup("attack_3p2");
				assaultforce2.move(targetPos[0], targetPos[1]);
			}
			var targets3 = gameState.getEnemiesWithClass("Village");
			// If we have a target, move to it
			if (targets3.length)
			{
				// Remove the pending role
				pending3.forEach(function(ent) {
					ent.setMetadata("role", "attack_3p3");
				});
				var targetrandomiser3 = Math.floor(Math.random()*targets3.length);
				var target3 = targets3.toEntityArray()[targetrandomiser3];
				var targetPos3 = target3.position();
				var assaultforce3 = gameState.getOwnRoleGroup("attack_3p3");
				assaultforce3.move(targetPos[0], targetPos[1]);
			}
			this.nextAttack();
		}
		}


		Engine.ProfileStop();
	},

});
