var WalkToCC = function(gameState, militaryManager){
	this.minAttackSize = 20;
	this.maxAttackSize = 60;
	this.idList=[];
};

// Returns true if the attack can be executed at the current time 
WalkToCC.prototype.canExecute = function(gameState, militaryManager){
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
WalkToCC.prototype.execute = function(gameState, militaryManager){
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
		// Remove the pending role
		pending.forEach(function(ent) {
			ent.setMetadata("role", "attack");
		});

		var target = targets.toEntityArray()[0];
		var targetPos = target.position();

		// TODO: this should be an attack-move command
		pending.move(targetPos[0], targetPos[1]);
	} else if (targets.length == 0 ) {
		gameState.ai.gameFinished = true;
	}
};

// Runs every turn after the attack is executed
// This removes idle units from the attack
WalkToCC.prototype.update = function(gameState, militaryManager){
	var removeList = [];
	for (var idKey in this.idList){
		var id = this.idList[idKey];
		var ent = militaryManager.entity(id);
		if(ent)
		{
			if(ent.isIdle()) {
				militaryManager.unassignUnit(id);
				removeList.push(id);
			}
		} else {
			removeList.push(id);
		}
	}
	for (var i in removeList){
		this.idList.splice(this.idList.indexOf(removeList[i]),1);
	}
};