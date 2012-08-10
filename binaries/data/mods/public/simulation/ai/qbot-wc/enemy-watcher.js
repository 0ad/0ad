/*
 * A class that keeps track of enem buildings, units, and pretty much anything I can think of (still a LOT TODO here)
 * Only watches one enemy, you'll need one per enemy.
 */

var enemyWatcher = function(gameState, playerToWatch) {
	
	this.watched = playerToWatch;
	
	// creating fitting entity collections
	var filter = Filters.and(Filters.byClass("Structure"), Filters.byOwner(this.watched));
	this.enemyBuildings = gameState.getEnemyEntities().filter(filter);
	this.enemyBuildings.registerUpdates();

	filter = Filters.and(Filters.byClass("Worker"), Filters.byOwner(this.watched));
	this.enemyCivilians = gameState.getEnemyEntities().filter(filter);
	this.enemyCivilians.registerUpdates();

	filter = Filters.and(Filters.byClassesOr(["CitizenSoldier", "Hero", "Champion", "Siege"]), Filters.byOwner(this.watched));
	this.enemySoldiers = gameState.getEnemyEntities().filter(filter);
	this.enemySoldiers.registerUpdates();
	
	// okay now we register here only enemy soldiers that we are monitoring (ie we see as part of an army…)
	filter = Filters.and(Filters.byClassesOr(["CitizenSoldier", "Hero", "Champion", "Siege"]), Filters.and(Filters.byMetadata("monitored","true"),Filters.byOwner(this.watched)));
	this.monitoredEnemySoldiers = gameState.getEnemyEntities().filter(filter);
	this.monitoredEnemySoldiers.registerUpdates();
	
	// and here those that we do not monitor
	filter = Filters.and(Filters.byClassesOr(["CitizenSoldier","Hero","Champion","Siege"]), Filters.and(Filters.not(Filters.byMetadata("monitored","true")),Filters.byOwner(this.watched)));
	this.unmonitoredEnemySoldiers = gameState.getEnemyEntities().filter(filter);
	this.unmonitoredEnemySoldiers.registerUpdates();

	// entity collections too.
	this.armies = {};	
	
	this.enemyBuildingClass = {};
	this.totalNBofArmies = 0;
	
	// this is an array of integers, refering to "this.armies[ XX ]"
	this.dangerousArmies = [];
	
};
enemyWatcher.prototype.getAllEnemySoldiers = function() {
	return this.enemySoldiers;
};
enemyWatcher.prototype.getAllEnemyBuildings = function() {
	return this.enemyBuildings;
};
enemyWatcher.prototype.getEnemyBuildings = function(specialClass, OneTime) {
	var filter = Filters.byClass(specialClass);
	var returnable = this.enemyBuildings.filter(filter);

	if (!this.enemyBuildingClass[specialClass] && !OneTime) {
		this.enemyBuildingClass[specialClass] = returnable;
		this.enemyBuildingClass[specialClass].registerUpdates();
		return this.enemyBuildingClass[specialClass];
	}
	return returnable;
};
enemyWatcher.prototype.getDangerousArmies = function() {
	var toreturn = {};
	for (i in this.dangerousArmies)
		toreturn[this.dangerousArmies[i]] = this.armies[this.dangerousArmies[i]];
	return toreturn;
};
enemyWatcher.prototype.getSafeArmies = function() {
	var toreturn = {};
	for (i in this.armies)
		if (this.dangerousArmies.indexOf(i) == -1)
			toreturn[i] = this.armies[i];
	return toreturn;
};
enemyWatcher.prototype.resetDangerousArmies = function() {
	this.dangerousArmies = [];
};
enemyWatcher.prototype.setAsDangerous = function(armyID) {
	if (this.dangerousArmies.indexOf(armyID) === -1)
		this.dangerousArmies.push(armyID);
};
enemyWatcher.prototype.isDangerous = function(armyID) {
	if (this.dangerousArmies.indexOf(armyID) === -1)
		return false;
	return true;
};
// returns [id, army]
enemyWatcher.prototype.getArmyFromMember = function(memberID) {
	for (i in this.armies) {
		if (this.armies[i].toIdArray().indexOf(memberID) !== -1)
			return [i,this.armies[i]];
	}
	return undefined;
};
enemyWatcher.prototype.isPartOfDangerousArmy = function(memberID) {
	var armyID = this.getArmyFromMember(memberID)[0];
	if (this.isDangerous(armyID))
		return true;
	return false;
};
enemyWatcher.prototype.cleanDebug = function() {
	for (armyID in this.armies) {
		var army = this.armies[armyID];
		debug ("Army " +armyID);
		debug (army.length +" members, centered around " +army.getCentrePosition());
	}
}

// this will monitor any unmonitored soldier.
enemyWatcher.prototype.detectArmies = function(gameState){
	//this.cleanDebug();
	
	var self = this;
	// let's loop through unmonitored enemy soldiers
	this.unmonitoredEnemySoldiers.forEach( function (enemy) {
		if (enemy.position() == undefined)
			return;

		// this was an unmonitored unit, we do not know any army associated with it. We assign it a new army (we'll merge later if needed)
		enemy.setMetadata("monitored","true");
		var armyID = uneval( gameState.player + "" + self.totalNBofArmies);
		self.totalNBofArmies++,
		enemy.setMetadata("EnemyWatcherArmy",armyID);
		var filter = Filters.byMetadata("EnemyWatcherArmy",armyID);
		var army = self.enemySoldiers.filter(filter);
		self.armies[armyID] = army;
		self.armies[armyID].registerUpdates();
		self.armies[armyID].length;
	});
	this.mergeArmies();	// calls "scrap empty armies"
	this.splitArmies(gameState);
};
// this will merge any two army who are too close together. The distance for "army" is fairly big.
// note: this doesn't actually merge two entity collections... It simply changes the unit metadatas, and will clear the empty entity collection
enemyWatcher.prototype.mergeArmies = function(){
	for (army in this.armies) {
		var firstArmy = this.armies[army];
		if (firstArmy.length > 0)
			for (otherArmy in this.armies) {
				if (otherArmy !== army && this.armies[otherArmy].length > 0) {
					var secondArmy = this.armies[otherArmy];
					// we're not self merging, so we check if the two armies are close together
					if (inRange(firstArmy.getCentrePosition(),secondArmy.getCentrePosition(), 3000 ) ) {
						// okay so we merge the two together
						
						// if the other one was dangerous and we weren't, we're now.
						if (this.dangerousArmies.indexOf(otherArmy) !== -1 && this.dangerousArmies.indexOf(army) === -1)
							this.dangerousArmies.push(army);
						
						secondArmy.forEach( function(ent) {
							ent.setMetadata("EnemyWatcherArmy",army);
						});
					}
				}
			}
	}
	this.ScrapEmptyArmies();
};
enemyWatcher.prototype.ScrapEmptyArmies = function(){
	var removelist = [];
	for (army in this.armies) {
		if (this.armies[army].length === 0) {
			removelist.push(army);
			// if the army was dangerous, we remove it from the list
			if (this.dangerousArmies.indexOf(army) !== -1)
				this.dangerousArmies.splice(this.dangerousArmies.indexOf(army),1);
		}
	}
	for each (toRemove in removelist) {
		delete this.armies[toRemove];
	}
};
// splits any unit too far from the centerposition
enemyWatcher.prototype.splitArmies = function(gameState){
	var self = this;
	for (armyID in this.armies) {
		var army = this.armies[armyID];
		var centre = army.getCentrePosition();
		army.forEach( function (enemy) {
			if (enemy.position() == undefined)
				return;
					 
			// debug ("entity " +enemy.templateName() + " is currently " +enemy.visibility(gameState.player));
					 
			if (!inRange(enemy.position(),centre, 3500) ) {
				var newArmyID = uneval( gameState.player + "" + self.totalNBofArmies);
				if (self.dangerousArmies.indexOf(armyID) !== -1)
					 self.dangerousArmies.push(newArmyID);
				
				self.totalNBofArmies++,
				enemy.setMetadata("EnemyWatcherArmy",newArmyID);
				var filter = Filters.byMetadata("EnemyWatcherArmy",newArmyID);
				var newArmy = self.enemySoldiers.filter(filter);
				self.armies[newArmyID] = newArmy;
				self.armies[newArmyID].registerUpdates();
				self.armies[newArmyID].length;
			}
		});
	}
};