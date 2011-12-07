var AttackMoveToCC = function(gameState, militaryManager){
	this.minAttackSize = 20;
	this.maxAttackSize = 60;
	this.idList=[];
	
	this.previousTime = 0;
	this.state = "unexecuted";
	
	this.healthRecord = [];
};

// Returns true if the attack can be executed at the current time 
AttackMoveToCC.prototype.canExecute = function(gameState, militaryManager){
	var enemyStrength = militaryManager.measureEnemyStrength(gameState);
	var enemyCount = militaryManager.measureEnemyCount(gameState);
	
	// We require our army to be >= this strength
	var targetStrength = enemyStrength * 1.5;
	
	var availableCount = militaryManager.countAvailableUnits();
	var availableStrength = militaryManager.measureAvailableStrength();
	
	debug("Troops needed for attack: " + this.minAttackSize + " Have: " + availableCount);
	debug("Troops strength for attack: " + targetStrength + " Have: " + availableStrength);
	
	return ((availableStrength >= targetStrength && availableCount >= this.minAttackSize)
			|| availableCount >= this.maxAttackSize);
};

// Executes the attack plan, after this is executed the update function will be run every turn
AttackMoveToCC.prototype.execute = function(gameState, militaryManager){
	var availableCount = militaryManager.countAvailableUnits();
	this.idList = militaryManager.getAvailableUnits(availableCount);
	
	var pending = EntityCollectionFromIds(gameState, this.idList);
	
	// Find the critical enemy buildings we could attack
	var targets = militaryManager.getEnemyBuildings(gameState,"ConquestCritical");
	// If there are no critical structures, attack anything else that's critical
	if (targets.length == 0) {
		targets = gameState.entities.filter(function(ent) {
			return (gameState.isEntityEnemy(ent) && ent.hasClass("ConquestCritical") && ent.owner() !== 0);
		});
	}
	// If there's nothing, attack anything else that's less critical
	if (targets.length == 0) {
		targets = militaryManager.getEnemyBuildings(gameState,"Town");
	}
	if (targets.length == 0) {
		targets = militaryManager.getEnemyBuildings(gameState,"Village");
	}
	

	// If we have a target, move to it
	if (targets.length) {
		// Add an attack role so the economic manager doesn't try and use them
		pending.forEach(function(ent) {
			ent.setMetadata("role", "attack");
		});
		
		var curPos = pending.getCentrePosition();
		
		var target = targets.toEntityArray()[0];
		this.targetPos = target.position();
		
		// Find possible distinct paths to the enemy 
		var pathFinder = new PathFinder(gameState);
		var pathsToEnemy = pathFinder.getPaths(curPos, this.targetPos);
		if (! pathsToEnemy){
			pathsToEnemy = [this.targetPos];
		}
		
		var rand = Math.floor(Math.random() * pathsToEnemy.length);
		this.path = pathsToEnemy[rand];

		pending.move(this.path[0][0], this.path[0][1]);
	} else if (targets.length == 0 ) {
		gameState.ai.gameFinished = true;
	}
	
	this.state = "walking";
};

// Runs every turn after the attack is executed
// This removes idle units from the attack
AttackMoveToCC.prototype.update = function(gameState, militaryManager, events){
	
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

