function AttackMoveToLocation(gameState, militaryManager, minAttackSize, maxAttackSize, targetFinder){
	this.minAttackSize = minAttackSize || Config.attack.minAttackSize;
	this.maxAttackSize = maxAttackSize || Config.attack.maxAttackSize;
	this.idList=[];
	
	this.previousTime = 0;
	this.state = "unexecuted";
	
	this.targetFinder = targetFinder || this.defaultTargetFinder;
	
	this.healthRecord = [];
};

// Returns true if the attack can be executed at the current time 
AttackMoveToLocation.prototype.canExecute = function(gameState, militaryManager){
	var enemyStrength = militaryManager.measureEnemyStrength(gameState);
	var enemyCount = militaryManager.measureEnemyCount(gameState);
	
	// We require our army to be >= this strength
	var targetStrength = enemyStrength * Config.attack.enemyRatio;
	
	var availableCount = militaryManager.countAvailableUnits();
	var availableStrength = militaryManager.measureAvailableStrength();
	
	debug("Troops needed for attack: " + this.minAttackSize + " Have: " + availableCount);
	debug("Troops strength for attack: " + targetStrength + " Have: " + availableStrength);
	
	return ((availableStrength >= targetStrength && availableCount >= this.minAttackSize)
			|| availableCount >= this.maxAttackSize);
};

// Default target finder aims for conquest critical targets
AttackMoveToLocation.prototype.defaultTargetFinder = function(gameState, militaryManager){
	// Find the critical enemy buildings we could attack
	var targets = militaryManager.getEnemyBuildings(gameState,"ConquestCritical");
	// If there are no critical structures, attack anything else that's critical
	if (targets.length == 0) {
		targets = gameState.entities.filter(function(ent) {
			return (gameState.isEntityEnemy(ent) && ent.hasClass("ConquestCritical") && ent.owner() !== 0 && ent.position());
		});
	}
	// If there's nothing, attack anything else that's less critical
	if (targets.length == 0) {
		targets = militaryManager.getEnemyBuildings(gameState,"Town");
	}
	if (targets.length == 0) {
		targets = militaryManager.getEnemyBuildings(gameState,"Village");
	}
	return targets;
};

// Executes the attack plan, after this is executed the update function will be run every turn
AttackMoveToLocation.prototype.execute = function(gameState, militaryManager){
	var availableCount = militaryManager.countAvailableUnits();
	var numWanted = Math.min(availableCount, this.maxAttackSize);
	this.idList = militaryManager.getAvailableUnits(numWanted);
	
	var pending = EntityCollectionFromIds(gameState, this.idList);
	
	var targets = this.targetFinder(gameState, militaryManager);
	
	if (targets.length === 0){
		targets = this.defaultTargetFinder(gameState, militaryManager);
	}
	
	// If we have a target, move to it
	if (targets.length) {
		// Add an attack role so the economic manager doesn't try and use them
		pending.forEach(function(ent) {
			ent.setMetadata("role", "attack");
		});
		
		var curPos = pending.getCentrePosition();
		
		// pick a random target from the list 
		var rand = Math.floor((Math.random()*targets.length));
		this.targetPos = undefined;
		var count = 0;
		while (!this.targetPos){
			var target = targets.toEntityArray()[rand];
			this.targetPos = target.position();
			count++;
			if (count > 1000){
				warn("No target with a valid position found");
				return;
			}
		}
		
		// Find possible distinct paths to the enemy 
		var pathFinder = new PathFinder(gameState);
		var pathsToEnemy = pathFinder.getPaths(curPos, this.targetPos);
		if (!pathsToEnemy || !pathsToEnemy[0] || pathsToEnemy[0][0] === undefined || pathsToEnemy[0][1] === undefined){
			pathsToEnemy = [[this.targetPos]];
		}
		
		var rand = Math.floor(Math.random() * pathsToEnemy.length);
		this.path = pathsToEnemy[rand];

		pending.move(this.path[0][0], this.path[0][1]);
	} else if (targets.length == 0 ) {
		gameState.ai.gameFinished = true;
		return;
	}
	
	this.state = "walking";
};

// Runs every turn after the attack is executed
// This removes idle units from the attack
AttackMoveToLocation.prototype.update = function(gameState, militaryManager, events){
	
	// keep the list of units in good order by pruning ids with no corresponding entities (i.e. dead units)
	var removeList = [];
	var totalHealth = 0;
	for (var idKey in this.idList){
		var id = this.idList[idKey];
		var ent = militaryManager.entity(id);
		if (ent === undefined){
			removeList.push(id);
		}else{
			if (ent.hitpoints()){
				totalHealth += ent.hitpoints();
			}
		}
	}
	for (var i in removeList){
		this.idList.splice(this.idList.indexOf(removeList[i]),1);
	}
	
	var units = EntityCollectionFromIds(gameState, this.idList);
	
	if (this.path.length === 0){
		var idleCount = 0;
		var self = this;
		units.forEach(function(ent){
			if (ent.isIdle()){
				if (ent.position() && VectorDistance(ent.position(), self.targetPos) > 30){
					ent.move(self.targetPos[0], self.targetPos[1]);
				}else{
					militaryManager.unassignUnit(ent.id());
				}
			}
		});
		return;
	}
	
	var deltaHealth = 0;
	var deltaTime = 1;
	var time = gameState.getTimeElapsed();
	this.healthRecord.push([totalHealth, time]);
	if (this.healthRecord.length > 1){
		for (var i = this.healthRecord.length - 1; i >= 0; i--){
			deltaHealth = totalHealth - this.healthRecord[i][0];
			deltaTime = time - this.healthRecord[i][1];
			if (this.healthRecord[i][1] < time - 5*1000){
				break;
			}
		}
	}
	
	var numUnits = this.idList.length;
	if (numUnits < 1) return;
	var damageRate = -deltaHealth / deltaTime * 1000;
	var centrePos = units.getCentrePosition();
	if (! centrePos) return;
	
	var idleCount = 0;
	// Looks for idle units away from the formations centre
	for (var idKey in this.idList){
		var id = this.idList[idKey];
		var ent = militaryManager.entity(id);
		if (ent.isIdle()){
			if (ent.position() && VectorDistance(ent.position(), centrePos) > 20){
				var dist = VectorDistance(ent.position(), centrePos);
				var vector = [centrePos[0] - ent.position()[0], centrePos[1] - ent.position()[1]];
				vector[0] *= 10/dist;
				vector[1] *= 10/dist;
				ent.move(centrePos[0] + vector[0], centrePos[1] + vector[1]);
			}else{
				idleCount++;
			}
		}
	}
	
	if ((damageRate / Math.sqrt(numUnits)) > 2){
		if (this.state === "walking"){
			var sumAttackerPos = [0,0];
			var numAttackers = 0;
			
			for (var key in events){
				var e = events[key];
				//{type:"Attacked", msg:{attacker:736, target:1133, type:"Melee"}}
				if (e.type === "Attacked" && e.msg){
					if (this.idList.indexOf(e.msg.target) !== -1){
						var attacker = militaryManager.entity(e.msg.attacker);
						if (attacker && attacker.position()){
							sumAttackerPos[0] += attacker.position()[0];
							sumAttackerPos[1] += attacker.position()[1];
							numAttackers += 1;
						}
					}
				}
			}
			if (numAttackers > 0){
				var avgAttackerPos = [sumAttackerPos[0]/numAttackers, sumAttackerPos[1]/numAttackers];
				// Stop moving
				units.move(centrePos[0], centrePos[1]);
				this.state = "attacking";
			}
		}
	}else{
		if (this.state === "attacking"){
			units.move(this.path[0][0], this.path[0][1]);
			this.state = "walking";
		}
	}
	
	if (this.state === "walking"){
		if (VectorDistance(centrePos, this.path[0]) < 20 || idleCount/numUnits > 0.8){
			this.path.shift();
			if (this.path.length > 0){
				units.move(this.path[0][0], this.path[0][1]);
			}
		}
	}
	
	this.previousTime = time;
	this.previousHealth = totalHealth;
};

