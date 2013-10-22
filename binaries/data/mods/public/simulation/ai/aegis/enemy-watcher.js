/*
 * A class that keeps track of enemy buildings, units, and pretty much anything I can think of (still a LOT TODO here)
 * Only watches one enemy, you'll need one per enemy.
 * Note: pretty much unused in the current version.
 */

var enemyWatcher = function(gameState, playerToWatch) {
	
	this.watched = playerToWatch;
	
	// using global entity collections, shared by any AI that knows the name of this.
	
	var filter = Filters.and(Filters.byClass("Structure"), Filters.byOwner(this.watched));
	this.enemyBuildings = gameState.updatingGlobalCollection("player-" +this.watched + "-structures", filter);

	filter = Filters.and(Filters.byClass("Unit"), Filters.byOwner(this.watched));
	this.enemyUnits = gameState.updatingGlobalCollection("player-" +this.watched + "-units", filter);

	filter = Filters.and(Filters.byClass("Worker"), Filters.byOwner(this.watched));
	this.enemyCivilians = gameState.updatingGlobalCollection("player-" +this.watched + "-civilians", filter);
							 
	filter = Filters.and(Filters.byClassesOr(["CitizenSoldier", "Hero", "Champion", "Siege"]), Filters.byOwner(this.watched));
	this.enemySoldiers = gameState.updatingGlobalCollection("player-" +this.watched + "-soldiers", filter);
	
	filter = Filters.and(Filters.byClass("Worker"), Filters.byOwner(this.watched));
	this.enemyCivilians = gameState.getEnemyEntities().filter(filter);
	this.enemyCivilians.registerUpdates();

	filter = Filters.and(Filters.byClassesOr(["CitizenSoldier", "Hero", "Champion", "Siege"]), Filters.byOwner(this.watched));
	this.enemySoldiers = gameState.getEnemyEntities().filter(filter);
	this.enemySoldiers.registerUpdates();

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

enemyWatcher.prototype.getEnemyBuildings = function(gameState, specialClass, OneTime) {
	var filter = Filters.byClass(specialClass);
	
	if (OneTime && gameState.getGEC("player-" +this.watched + "-structures-" +specialClass))
		return gameState.getGEC("player-" +this.watched + "-structures-" +specialClass);
	else if (OneTime)
		return this.enemyBuildings.filter(filter);
	
	return gameState.updatingGlobalCollection("player-" +this.watched + "-structures-" +specialClass, filter, gameState.getGEC("player-" +this.watched + "-structures"));
};
enemyWatcher.prototype.getDangerousArmies = function() {
	var toreturn = {};
	for (var i in this.dangerousArmies)
		toreturn[this.dangerousArmies[i]] = this.armies[this.dangerousArmies[i]];
	return toreturn;
};
enemyWatcher.prototype.getSafeArmies = function() {
	var toreturn = {};
	for (var i in this.armies)
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
	for (var i in this.armies) {
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
	for (var armyID in this.armies) {
		var army = this.armies[armyID];
		debug ("Army " +armyID);
		debug (army.length +" members, centered around " +army.getCentrePosition());
	}
}

// this will monitor any unmonitored soldier.
enemyWatcher.prototype.detectArmies = function(gameState){
	//this.cleanDebug();
	
	var self = this;
	if (gameState.ai.playedTurn % 4 === 0) {
		Engine.ProfileStart("Looking for new soldiers");
		// let's loop through unmonitored enemy soldiers
		this.unmonitoredEnemySoldiers.forEach( function (enemy) { //}){
			if (enemy.position() === undefined)
				return;
			
			// this was an unmonitored unit, we do not know any army associated with it. We assign it a new army (we'll merge later if needed)
			enemy.setMetadata(PlayerID, "monitored","true");
			var armyID = gameState.player + "" + self.totalNBofArmies;
			self.totalNBofArmies++,
			enemy.setMetadata(PlayerID, "EnemyWatcherArmy",armyID);
			var filter = Filters.byMetadata(PlayerID, "EnemyWatcherArmy",armyID);
			var army = self.enemySoldiers.filter(filter);
			self.armies[armyID] = army;
			self.armies[armyID].registerUpdates();
			self.armies[armyID].length;
		});
		Engine.ProfileStop();
	} else if (gameState.ai.playedTurn % 16 === 3) {
		Engine.ProfileStart("Merging");
		this.mergeArmies();	// calls "scrap empty armies"
		Engine.ProfileStop();
	} else if (gameState.ai.playedTurn % 16 === 7) {
		Engine.ProfileStart("Splitting");
		this.splitArmies(gameState);
		Engine.ProfileStop();
	}
};
// this will merge any two army who are too close together. The distance for "army" is fairly big.
// note: this doesn't actually merge two entity collections... It simply changes the unit metadatas, and will clear the empty entity collection
enemyWatcher.prototype.mergeArmies = function(){
	for (var army in this.armies) {
		var firstArmy = this.armies[army];
		if (firstArmy.length !== 0) {
			var firstAPos = firstArmy.getApproximatePosition(4);
			for (var otherArmy in this.armies) {
				if (otherArmy !== army && this.armies[otherArmy].length !== 0) {
					var secondArmy = this.armies[otherArmy];
					// we're not self merging, so we check if the two armies are close together
					if (inRange(firstAPos,secondArmy.getApproximatePosition(4), 4000 ) ) {
						// okay so we merge the two together
						
						// if the other one was dangerous and we weren't, we're now.
						if (this.dangerousArmies.indexOf(otherArmy) !== -1 && this.dangerousArmies.indexOf(army) === -1)
							this.dangerousArmies.push(army);
						
						secondArmy.forEach( function(ent) {
							ent.setMetadata(PlayerID, "EnemyWatcherArmy",army);
						});
					}
				}
			}
		}
	}
	this.ScrapEmptyArmies();
};
enemyWatcher.prototype.ScrapEmptyArmies = function(){
	var removelist = [];
	for (var army in this.armies) {
		if (this.armies[army].length === 0) {
			removelist.push(army);
			// if the army was dangerous, we remove it from the list
			if (this.dangerousArmies.indexOf(army) !== -1)
				this.dangerousArmies.splice(this.dangerousArmies.indexOf(army),1);
		}
	}
	for each (var toRemove in removelist) {
		delete this.armies[toRemove];
	}
};
// splits any unit too far from the centerposition
enemyWatcher.prototype.splitArmies = function(gameState){
	var self = this;
	
	var map = Map.createTerritoryMap(gameState);
	
	for (var armyID in this.armies) {
		var army = this.armies[armyID];
		var centre = army.getApproximatePosition(4);
		
		if (map.getOwner(centre) === gameState.player)
			continue;
		
		army.forEach( function (enemy) {
			if (enemy.position() === undefined)
				return;
					 
			if (!inRange(enemy.position(),centre, 3500) ) {
				var newArmyID = gameState.player + "" + self.totalNBofArmies;
				if (self.dangerousArmies.indexOf(armyID) !== -1)
					 self.dangerousArmies.push(newArmyID);
				
				self.totalNBofArmies++,
				enemy.setMetadata(PlayerID, "EnemyWatcherArmy",newArmyID);
				var filter = Filters.byMetadata(PlayerID, "EnemyWatcherArmy",newArmyID);
				var newArmy = self.enemySoldiers.filter(filter);
				self.armies[newArmyID] = newArmy;
				self.armies[newArmyID].registerUpdates();
				self.armies[newArmyID].length;
			}
		});
	}
};
