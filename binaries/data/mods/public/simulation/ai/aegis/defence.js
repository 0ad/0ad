// directly imported from Marilyn, with slight modifications to work with qBot.

function Defence(){
	this.defenceRatio = Config.Defence.defenceRatio;// How many defenders we want per attacker.  Need to balance fewer losses vs. lost economy
							// note: the choice should be a no-brainer most of the time: better deflect the attack.
							// This is also sometimes forcebly overcome by the defense manager.
	this.armyCompactSize = Config.Defence.armyCompactSize;	// a bit more than 40 wide in diameter
	this.armyBreakawaySize = Config.Defence.armyBreakawaySize;	// a bit more than 45 wide in diameter
	
	this.totalAttackNb = 0;	// used for attack IDs
	this.attacks = [];
	this.toKill = [];
	
	
	// keeps a list of targeted enemy at instant T
	this.enemyArmy = {};	// array of players, storing for each an array of armies.
	this.attackerCache = {};
	this.listOfEnemies = {};
	this.listedEnemyCollection = null;	// entity collection of this.listOfEnemies
	
	// Some Stats
	this.nbAttackers = 0;
	this.nbDefenders = 0;

	// Caching variables
	this.totalArmyNB = 0;
	this.enemyUnits = {};
	this.enemyArmyLoop = {};
	// boolean 0/1 that's for optimization
	this.attackerCacheLoopIndicator = 0;
	
	// this is a list of units to kill. They should be gaia animals, or lonely units. Works the same as listOfEnemies, ie an entityColelction which I'll have to cleanup
	this.listOfWantedUnits = {};
	this.WantedUnitsAttacker = {};	// same as attackerCache.
	
	this.defenders = null;
	this.idleDefs = null;
}

// DO NOTE: the Defence manager, when it calls for Defence, makes the military manager go into "Defence mode"... This makes it not update any plan that's started or not.
// This allows the Defence manager to take units from the plans for Defence.

// Defcon levels
// 5: no danger whatsoever detected
// 4: a few enemy units are being dealt with, but nothing too dangerous.
// 3: A reasonnably sized enemy army is being dealt with, but it should not be a problem.
// 2: A big enemy army is in the base, but we are not outnumbered
// 1: Huge army in the base, outnumbering us.


Defence.prototype.update = function(gameState, events, HQ){
	
	Engine.ProfileStart("Defence Manager");
	
	// a litlle cache-ing
	if (!this.idleDefs) {
		var filter = Filters.and(Filters.byMetadata(PlayerID, "role", "defence"), Filters.isIdle());
		this.idleDefs = gameState.getOwnEntities().filter(filter);
		this.idleDefs.registerUpdates();
	}
	if (!this.defenders) {
		var filter = Filters.byMetadata(PlayerID, "role", "defence");
		this.defenders = gameState.getOwnEntities().filter(filter);
		this.defenders.registerUpdates();
	}
	/*if (!this.listedEnemyCollection) {
		var filter = Filters.byMetadata(PlayerID, "listed-enemy", true);
		this.listedEnemyCollection = gameState.getEnemyEntities().filter(filter);
		this.listedEnemyCollection.registerUpdates();
	}
	this.myBuildings = gameState.getOwnEntities().filter(Filters.byClass("Structure")).toEntityArray();
	this.myUnits = gameState.getOwnEntities().filter(Filters.byClass("Unit"));
	*/
	var filter = Filters.and(Filters.byClassesOr(["CitizenSoldier", "Hero", "Champion", "Siege"]), Filters.byOwner(PlayerID));
	this.myUnits = gameState.updatingGlobalCollection("player-" +PlayerID + "-soldiers", filter);
	
	filter = Filters.and(Filters.byClass("Structure"), Filters.byOwner(PlayerID));
	this.myBuildings = gameState.updatingGlobalCollection("player-" +PlayerID + "-structures", filter);

	this.territoryMap = Map.createTerritoryMap(gameState);	// used by many func

	// First step: we deal with enemy armies, those are the highest priority.
	this.defendFromEnemies(gameState, events, HQ);

	// second step: we loop through messages, and sort things as needed (dangerous buildings, attack by animals, ships, lone units, whatever).
	// TODO : a lot.
	this.MessageProcess(gameState,events,HQ);
	
	this.DealWithWantedUnits(gameState,events,HQ);

	/*
	var self = this;
	// putting unneeded units at rest
	this.idleDefs.forEach(function(ent) {
		if (ent.getMetadata(PlayerID, "formerrole"))
			ent.setMetadata(PlayerID, "role", ent.getMetadata(PlayerID, "formerrole") );
		else
			ent.setMetadata(PlayerID, "role", "worker");
		ent.setMetadata(PlayerID, "subrole", undefined);
		self.nbDefenders--;
	});*/

	Engine.ProfileStop();
	
	return;
};
/*
// returns armies that are still seen as dangerous (in the LOS of any of my buildings for now)
Defence.prototype.reevaluateDangerousArmies = function(gameState, armies) {
	var stillDangerousArmies = {};
	for (var i in armies) {
		var pos = armies[i].getCentrePosition();
		if (pos === undefined)
			
		if (+this.territoryMap.point(pos) - 64 === +PlayerID) {
			stillDangerousArmies[i] = armies[i];
			continue;
		}
		for (var o in this.myBuildings) {
			// if the armies out of my buildings LOS (with a little more, because we're cheating right now and big armies could go undetected)
			if (inRange(pos, this.myBuildings[o].position(),this.myBuildings[o].visionRange()*this.myBuildings[o].visionRange() + 2500)) {
				stillDangerousArmies[i] = armies[i];
				break;
			}
		}		
	}
	return stillDangerousArmies;
}
// returns armies we now see as dangerous, ie in my territory
Defence.prototype.evaluateArmies = function(gameState, armies) {
	var DangerousArmies = {};
	for (var i in armies) {
		if (armies[i].getCentrePosition() && +this.territoryMap.point(armies[i].getCentrePosition()) - 64 === +PlayerID) {
			DangerousArmies[i] = armies[i];
		}
	}
	return DangerousArmies;
}*/
// Incorporates an entity in an army. If no army fits, it creates a new one around this one.
// an army is basically an entity collection.
Defence.prototype.armify = function(gameState, entity, HQ, minNBForArmy) {
	if (entity.position() === undefined)
		return;
	if (this.enemyArmy[entity.owner()] === undefined)
	{
		this.enemyArmy[entity.owner()] = {};
	} else {
		for (var armyIndex in this.enemyArmy[entity.owner()])
		{
			var army = this.enemyArmy[entity.owner()][armyIndex];
			if (army.getCentrePosition() === undefined)
			{
			} else {
				if (SquareVectorDistance(army.getCentrePosition(), entity.position()) < this.armyCompactSize)
				{
					entity.setMetadata(PlayerID, "inArmy", armyIndex);
					army.addEnt(entity);
					return;
				}
			}
		}
	}
	if (HQ)
	{
		var self = this;
		var close = HQ.enemyWatchers[entity.owner()].enemySoldiers.filter(Filters.byDistance(entity.position(), self.armyCompactSize));
		if (!minNBForArmy || close.length >= minNBForArmy)
		{
			// if we're here, we need to create an army for it, and freeze it to make sure no unit will be added automatically
			var newArmy = new EntityCollection(gameState.sharedScript, {}, [Filters.byOwner(entity.owner())]);
			newArmy.addEnt(entity);
			newArmy.freeze();
			newArmy.registerUpdates();
			entity.setMetadata(PlayerID, "inArmy", this.totalArmyNB);
			this.enemyArmy[entity.owner()][this.totalArmyNB] = newArmy;
			close.forEach(function (ent) { //}){
				if (ent.position() !== undefined && self.reevaluateEntity(gameState, ent))
				{
					ent.setMetadata(PlayerID, "inArmy", self.totalArmyNB);
					self.enemyArmy[ent.owner()][self.totalArmyNB].addEnt(ent);
				}
			});
			this.totalArmyNB++;
		}
	} else {
		// if we're here, we need to create an army for it, and freeze it to make sure no unit will be added automatically
		var newArmy = new EntityCollection(gameState.sharedScript, {}, [Filters.byOwner(entity.owner())]);
		newArmy.addEnt(entity);
		newArmy.freeze();
		newArmy.registerUpdates();
		entity.setMetadata(PlayerID, "inArmy", this.totalArmyNB);
		this.enemyArmy[entity.owner()][this.totalArmyNB] = newArmy;
		this.totalArmyNB++;
	}
	return;
}
// Returns if a unit should be seen as dangerous or not.
Defence.prototype.evaluateRawEntity = function(gameState, entity) {
	if (entity.position && +this.territoryMap.point(entity.position) - 64 === +PlayerID && entity._template.Attack !== undefined)
		return true;
	return false;
}
Defence.prototype.evaluateEntity = function(gameState, entity) {
	if (!entity.position())
		return false;
	if (this.territoryMap.point(entity.position()) - 64 === entity.owner() || entity.attackTypes() === undefined)
		return false;
	
	for (var i in this.myBuildings._entities)
	{
		if (!this.myBuildings._entities[i].hasClass("ConquestCritical"))
			continue;
		if (SquareVectorDistance(this.myBuildings._entities[i].position(), entity.position()) < 6000)
			return true;
	}
	return false;
}
// returns false if the unit is in its territory
Defence.prototype.reevaluateEntity = function(gameState, entity) {
	if ( (entity.position() && +this.territoryMap.point(entity.position()) - 64 === +entity.owner()) || entity.attackTypes() === undefined)
		return false;
	return true;
}
// This deals with incoming enemy armies, setting the defcon if needed. It will take new soldiers, and assign them to attack
// TODO: still is still pretty dumb, it could use improvements.
Defence.prototype.defendFromEnemies = function(gameState, events, HQ) {
	var self = this;
	
	// New, faster system will loop for enemy soldiers, and also females on occasions ( TODO )
	// if a dangerous unit is found, it will check for neighbors and make them into an "army", an entityCollection
	//			> updated against owner, for the day when I throw healers in the deal.
	// armies are checked against each other now and then to see if they should be merged, and units in armies are checked to see if they should be taken away from the army.
	// We keep a list of idle defenders. For any new attacker, we'll check if we have any idle defender available, and if not, we assign available units.
	// At the end of each turn, if we still have idle defenders, we either assign them to neighboring units, or we release them.
	
	var nbOfAttackers = 0;	// actually new attackers.
	var newEnemies = [];

	// clean up using events.
	for each(var evt in events)
	{
		if (evt.type == "Destroy")
		{
			if (this.listOfEnemies[evt.msg.entity] !== undefined)
			{
				if (this.attackerCache[evt.msg.entity] !== undefined) {
					this.attackerCache[evt.msg.entity].forEach(function(ent) { ent.stopMoving(); });
					delete self.attackerCache[evt.msg.entity];
				}
				delete this.listOfEnemies[evt.msg.entity];
				this.nbAttackers--;
			} else if (evt.msg.entityObj && evt.msg.entityObj.owner() === PlayerID && evt.msg.metadata[PlayerID] && evt.msg.metadata[PlayerID]["role"]
					   && evt.msg.metadata[PlayerID]["role"] === "defence")
			{
				// lost a brave man there.
				this.nbDefenders--;
			}
		}
	}

	// Optimizations: this will slowly iterate over all units (saved at an instant T) and all armies.
	// It'll add new units if they are now dangerous and were not before
	// It'll also deal with cleanup of armies.
	// When it's finished it'll start over.
	for (var enemyID in this.enemyArmy)
	{
		//this.enemyUnits[enemyID] = HQ.enemyWatchers[enemyID].getAllEnemySoldiers();
		if (this.enemyUnits[enemyID] === undefined || this.enemyUnits[enemyID].length === 0)
		{
			this.enemyUnits[enemyID] = HQ.enemyWatchers[enemyID].enemySoldiers.toEntityArray();
		} else {
			// we have some units still to check in this array. Check 15 (TODO: DIFFLEVEL)
			// Note: given the way memory works, if the entity has been recently deleted, its reference may still exist.
			// and this.enemyUnits[enemyID][0] may still point to that reference, "reviving" the unit.
			// So we've got to make sure it's not supposed to be dead.
			for (var check = 0; check < 20; check++)
			{
				if (this.enemyUnits[enemyID].length > 0 && gameState.getEntityById(this.enemyUnits[enemyID][0].id()) !== undefined)
				{
					if (this.enemyUnits[enemyID][0].getMetadata(PlayerID, "inArmy") !== undefined)
					{
						this.enemyUnits[enemyID].splice(0,1);
					} else {
						var dangerous = this.evaluateEntity(gameState, this.enemyUnits[enemyID][0]);
						if (dangerous)
							this.armify(gameState, this.enemyUnits[enemyID][0], HQ,2);
						this.enemyUnits[enemyID].splice(0,1);
					}
				} else if (this.enemyUnits[enemyID].length > 0 && gameState.getEntityById(this.enemyUnits[enemyID][0].id()) === undefined)
				{
					this.enemyUnits[enemyID].splice(0,1);
				}
			}
		}
		// okay then we'll check one of the armies
		// creating the array to iterate over.
		if (this.enemyArmyLoop[enemyID] === undefined || this.enemyArmyLoop[enemyID].length === 0)
		{
			this.enemyArmyLoop[enemyID] = [];
			for (var i in this.enemyArmy[enemyID])
				this.enemyArmyLoop[enemyID].push([this.enemyArmy[enemyID][i],i]);
		}
		// and now we check the last known army.
		if (this.enemyArmyLoop[enemyID].length !== 0) {
			var army = this.enemyArmyLoop[enemyID][0][0];
			var position = army.getCentrePosition();

			if (!position)
			{
				var index = this.enemyArmyLoop[enemyID][0][1];
				delete this.enemyArmy[enemyID][index];
				this.enemyArmyLoop[enemyID].splice(0,1);
			} else {
				army.forEach(function (ent) { //}){
					// check if the unit is a breakaway
					if (ent.position() && SquareVectorDistance(position, ent.position()) > self.armyBreakawaySize)
					{
						ent.setMetadata(PlayerID, "inArmy", undefined);
						army.removeEnt(ent);
						if (self.evaluateEntity(gameState,ent))
							self.armify(gameState,ent);
					} else {
						// check if we have registered that unit already.
						if (self.listOfEnemies[ent.id()] === undefined) {
							self.listOfEnemies[ent.id()] = new EntityCollection(gameState.sharedScript, {}, [Filters.byOwner(ent.owner())]);
							self.listOfEnemies[ent.id()].freeze();
							self.listOfEnemies[ent.id()].addEnt(ent);
							self.listOfEnemies[ent.id()].registerUpdates();
							
							self.attackerCache[ent.id()] = self.myUnits.filter(Filters.byTargetedEntity(ent.id()));
							self.attackerCache[ent.id()].registerUpdates();
							nbOfAttackers++;
							self.nbAttackers++;
							newEnemies.push(ent);
						} else if (self.attackerCache[ent.id()] === undefined || self.attackerCache[ent.id()].length == 0) {
							nbOfAttackers++;
							newEnemies.push(ent);
						} else if (!self.reevaluateEntity(gameState,ent))
						{
							 ent.setMetadata(PlayerID, "inArmy", undefined);
							 army.removeEnt(ent);
							 if (self.attackerCache[ent.id()] !== undefined)
							 {
								self.attackerCache[ent.id()].forEach(function(ent) { ent.stopMoving(); });
								delete self.attackerCache[ent.id()];
								delete self.listOfEnemies[ent.id()];
								self.nbAttackers--;
							 }
						}
					}
				});
				// TODO: check if the army itself is not dangerous anymore.
				this.enemyArmyLoop[enemyID].splice(0,1);
			}
		}
		
		// okay so now the army update is done.
	}

	// Reordering attack because the pathfinder is for now not dynamically updated
	for (var o in this.attackerCache) {
		if ((this.attackerCacheLoopIndicator + o) % 2 === 0) {
			this.attackerCache[o].forEach(function (ent) {
				var attackPos = gameState.getEntityById(+o).position()
				if (attackPos)
					ent.attackMove(attackPos[0],attackPos[1]);
				ent.setStance("aggressive");
			});
		}
	}
	this.attackerCacheLoopIndicator++;
	this.attackerCacheLoopIndicator = this.attackerCacheLoopIndicator % 2;
	
	if (this.nbAttackers === 0 && this.nbDefenders === 0) {
		// Release all our units
		this.myUnits.filter(Filters.byMetadata(PlayerID, "role","defence")).forEach(function (defender) { //}){
			defender.stopMoving();
			if (defender.getMetadata(PlayerID, "formerrole"))
				defender.setMetadata(PlayerID, "role", defender.getMetadata(PlayerID, "formerrole") );
			else
				defender.setMetadata(PlayerID, "role", "worker");
			defender.setMetadata(PlayerID, "subrole", undefined);
			self.nbDefenders--;
		});
		HQ.ungarrisonAll(gameState);
		HQ.unpauseAllPlans(gameState);
		return;
	} else if (this.nbAttackers === 0 && this.nbDefenders !== 0) {
		// Release all our units
		this.myUnits.filter(Filters.byMetadata(PlayerID, "role","defence")).forEach(function (defender) { //}){
			defender.stopMoving();
			if (defender.getMetadata(PlayerID, "formerrole"))
				defender.setMetadata(PlayerID, "role", defender.getMetadata(PlayerID, "formerrole") );
			else
				defender.setMetadata(PlayerID, "role", "worker");
			defender.setMetadata(PlayerID, "subrole", undefined);
			self.nbDefenders--;
		});
		HQ.ungarrisonAll(gameState);
		HQ.unpauseAllPlans(gameState);
		return;
	}
	if ( (this.nbDefenders < 4 && this.nbAttackers >= 5) || this.nbDefenders === 0) {
		HQ.ungarrisonAll(gameState);
	}
	
	//debug ("total number of attackers:"+ this.nbAttackers);
	//debug ("total number of defenders:"+ this.nbDefenders);

	// If I'm here, I have a list of enemy units, and a list of my units attacking it (in absolute terms, I could use a list of any unit attacking it).
	// now I'll list my idle defenders, then my idle soldiers that could defend.
	// and then I'll assign my units.
	// and then rock on.	
	
	if (this.nbAttackers > 15){
		gameState.setDefcon(3);
	} else if (this.nbAttackers > 5){
		gameState.setDefcon(4);
	}
	
	// we're having too many. Release those that attack units already dealt with, or idle ones.
	if (this.myUnits.filter(Filters.byMetadata(PlayerID, "role","defence")).length > nbOfAttackers*this.defenceRatio*1.2) {
		this.myUnits.filter(Filters.byMetadata(PlayerID, "role","defence")).forEach(function (defender) { //}){
			if ( defender.isIdle() || (defender.unitAIOrderData() && defender.unitAIOrderData()["target"])) {
				if ( defender.isIdle() || (self.attackerCache[defender.unitAIOrderData()["target"]] && self.attackerCache[defender.unitAIOrderData()["target"]].length > 3)) {
					// okay release me.
					defender.stopMoving();
					if (defender.getMetadata(PlayerID, "formerrole"))
						defender.setMetadata(PlayerID, "role", defender.getMetadata(PlayerID, "formerrole") );
					else
						defender.setMetadata(PlayerID, "role", "worker");
					defender.setMetadata(PlayerID, "subrole", undefined);
					self.nbDefenders--;
				}
			}
		});
	}
	

	var nonDefenders = this.myUnits.filter(Filters.or(Filters.not(Filters.byMetadata(PlayerID, "role","defence")),Filters.isIdle()));
	nonDefenders = nonDefenders.filter(Filters.not(Filters.byClass("Female")));
	nonDefenders = nonDefenders.filter(Filters.not(Filters.byMetadata(PlayerID, "subrole","attacking")));
	var defenceRatio = this.defenceRatio;
	
	if (newEnemies.length + this.nbAttackers > (this.nbDefenders + nonDefenders.length) * 0.8 && this.nbAttackers > 9)
		gameState.setDefcon(2);
	
	if (newEnemies.length + this.nbAttackers > (this.nbDefenders + nonDefenders.length) * 1.5 && this.nbAttackers > 5)
		gameState.setDefcon(1);

	//debug ("newEnemies.length "+ newEnemies.length);
	//debug ("nonDefenders.length "+ nonDefenders.length);
		
	if (gameState.defcon() > 3)
		HQ.unpauseAllPlans(gameState);
	
	if ( (nonDefenders.length + this.nbDefenders > newEnemies.length + this.nbAttackers)
		|| this.nbDefenders + nonDefenders.length < 4)
	{
		var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).toEntityArray();
		buildings.forEach( function (struct) {
			if (struct.garrisoned() && struct.garrisoned().length)
				struct.unloadAll();
			});
	};
	
	if (newEnemies.length === 0)
		return;

	/*
	if (gameState.defcon() < 2 && (this.nbAttackers-this.nbDefenders) > 15)
	{
		HQ.pauseAllPlans(gameState);
	} else if (gameState.defcon() < 3 && this.nbDefenders === 0 && newEnemies.length === 0) {
		HQ.ungarrisonAll(gameState);
	}*/
	
	// A little sorting to target sieges/champions first.
	newEnemies.sort (function (a,b) {
		var vala = 1;
		var valb = 1;
		if (a.hasClass("Siege"))
			vala = 10;
		else if (a.hasClass("Champion") || a.hasClass("Hero"))
			vala = 5;
		if (b.hasClass("Siege"))
			valb = 10;
		else if (b.hasClass("Champion") || b.hasClass("Hero"))
			valb = 5;
		return valb - vala;
	});
	
	// For each enemy, we'll pick two units.
	for each (var enemy in newEnemies) {
		if (nonDefenders.length === 0 || self.nbDefenders >= self.nbAttackers * 1.8)
			break;
		// garrisoned.
		if (enemy.position() === undefined)
			continue;
		
		var assigned = self.attackerCache[enemy.id()].length;
		
		var defRatio = defenceRatio;
		if (enemy.hasClass("Siege"))
			defRatio *= 1.2;

		if (assigned >= defRatio)
			return;
		
		// We'll sort through our units that can legitimately attack.
		var data = [];
		for (var id in nonDefenders._entities)
		{
			var ent = nonDefenders._entities[id];
			if (ent.position())
				data.push([id, ent, SquareVectorDistance(enemy.position(), ent.position())]);
		}
		// refine the defenders we want. Since it's the distance squared, it has the effect
		// of tending to always prefer closer units, though this refinement should change it slighty.
		data.sort(function (a, b) {
			var vala = a[2];
			var valb = b[2];
			
			// don't defend with siege units unless enemy is also a siege unit.
			if (a[1].hasClass("Siege") && !enemy.hasClass("Siege"))
				  vala *= 9;
			if (b[1].hasClass("Siege") && !enemy.hasClass("Siege"))
				valb *= 9;
			// If it's a siege unit, We basically ignore units that only deal pierce damage.
			if (enemy.hasClass("Siege") && a[1].attackStrengths("Melee") !== undefined)
				vala /= (a[1].attackStrengths("Melee")["hack"] + a[1].attackStrengths("Melee")["crush"]);
			if (enemy.hasClass("Siege") && b[1].attackStrengths("Melee") !== undefined)
				valb /= (b[1].attackStrengths("Melee")["hack"] + b[1].attackStrengths("Melee")["crush"]);
			// If it's a counter, it's better.
			if (a[1].countersClasses(b[1].classes()))
				vala *= 0.1;	// quite low but remember it's squared distance.
			if (b[1].countersClasses(a[1].classes()))
				valb *= 0.1;
			// If the unit is idle, we prefer. ALso if attack plan.
			if ((a[1].isIdle() || a[1].getMetadata(PlayerID, "plan") !== undefined) && !a[1].hasClass("Siege"))
				vala *= 0.15;
			if ((b[1].isIdle() || b[1].getMetadata(PlayerID, "plan") !== undefined) && !b[1].hasClass("Siege"))
				valb *= 0.15;
			return (vala - valb); });

		var ret = {};
		for each (var val in data.slice(0, Math.min(nonDefenders._length, defRatio - assigned)))
			ret[val[0]] = val[1];
		
		var defs = new EntityCollection(nonDefenders._ai, ret);

		// successfully sorted
		defs.forEach(function (defender) { //}){
			if (defender.getMetadata(PlayerID, "plan") != undefined && (gameState.defcon() < 4 || defender.getMetadata(PlayerID,"subrole") == "walking"))
				HQ.pausePlan(gameState, defender.getMetadata(PlayerID, "plan"));
			//debug ("Against " +enemy.id() + " Assigning " + defender.id());
			if (defender.getMetadata(PlayerID, "role") == "worker" || defender.getMetadata(PlayerID, "role") == "attack")
				defender.setMetadata(PlayerID, "formerrole", defender.getMetadata(PlayerID, "role"));
			defender.setMetadata(PlayerID, "role","defence");
			defender.setMetadata(PlayerID, "subrole","defending");
			var attackPos = enemy.position();
			if (attackPos)
				defender.attackMove(attackPos[0],attackPos[1]);
			defender.setStance("aggressive");
			defender._entity.idle = false; // hack to prevent a bug as informations aren't updated during a turn
			nonDefenders.updateEnt(defender);
			assigned++;
			self.nbDefenders++;
		});
		
		/*if (gameState.defcon() <= 3)
		{
			// let's try to garrison neighboring females.
			var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).toEntityArray();
			var females = gameState.getOwnEntities().filter(Filters.byClass("Support"));
				
			var cache = {};
			var garrisoned = false;
			females.forEach( function (ent) { //}){
				garrisoned = false;
				if (ent.position())
				{
					if (SquareVectorDistance(ent.position(), enemy.position()) < 3500)
					{
						for (var i in buildings)
						{
							var struct = buildings[i];
							if (!cache[struct.id()])
								cache[struct.id()] = 0;
							if (struct.garrisoned() && struct.garrisonMax() - struct.garrisoned().length - cache[struct.id()] > 0)
							{
								garrisoned = true;
								ent.garrison(struct);
								cache[struct.id()]++;
								break;
							}
						}
						if (!garrisoned) {
							ent.flee(enemy);
							ent.setMetadata(PlayerID,"fleeing", gameState.getTimeElapsed());
						}
					}
				}
			});
		}*/
	}

	return;
}

// this processes the attackmessages
// So that a unit that gets attacked will not be completely dumb.
// warning: huge levels of indentation coming.
Defence.prototype.MessageProcess = function(gameState,events, HQ) {
	var self = this;
	
	for (var key in events){
		var e = events[key];
		if (e.type === "Attacked" && e.msg){
			if (gameState.isEntityOwn(gameState.getEntityById(e.msg.target))) {
				var attacker = gameState.getEntityById(e.msg.attacker);
				var ourUnit = gameState.getEntityById(e.msg.target);
				// the attacker must not be already dead, and it must not be me (think catapults that miss).
				if (attacker !== undefined && attacker.owner() !== PlayerID && attacker.position() !== undefined) {
					// note: our unit can already by dead by now... We'll then have to rely on the enemy to react.
					// if we're not on enemy territory
					var territory = +this.territoryMap.point(attacker.position())  - 64;
					
					// we do not consider units that are defenders, and we do not consider units that are part of an attacking attack plan
					// (attacking attacking plans are dealing with threats on their own).
					if (ourUnit !== undefined && (ourUnit.getMetadata(PlayerID, "role") == "defence" || ourUnit.getMetadata(PlayerID, "role") == "attack"))
						continue;
					
					// let's check for animals
					if (attacker.owner() == 0) {
						// if our unit is still alive, we make it react
						// in this case we attack.
						if (ourUnit !== undefined) {
							if (ourUnit.hasClass("Unit") && !ourUnit.hasClass("Support"))
								ourUnit.attack(e.msg.attacker);
							else {
								ourUnit.flee(attacker);
								ourUnit.setMetadata(PlayerID,"fleeing", gameState.getTimeElapsed());
							}
						}
						if (territory === PlayerID)
						{
							// anyway we'll register the animal as dangerous, and attack it (note: only on our territory. Don't care otherwise).
							this.listOfWantedUnits[attacker.id()] = new EntityCollection(gameState.sharedScript);
							this.listOfWantedUnits[attacker.id()].addEnt(attacker);
							this.listOfWantedUnits[attacker.id()].freeze();
							this.listOfWantedUnits[attacker.id()].registerUpdates();

							var filter = Filters.byTargetedEntity(attacker.id());
							this.WantedUnitsAttacker[attacker.id()] = this.myUnits.filter(filter);
							this.WantedUnitsAttacker[attacker.id()].registerUpdates();
						}
					} // preliminary check: we do not count attacked military units (for sanity for now, TODO).
					else if ( (territory != attacker.owner() && ourUnit.hasClass("Support")) || (!ourUnit.hasClass("Support") && territory == PlayerID))
					{
						// Also TODO: this does not differentiate with buildings...
						// These ought to be treated differently.
						// units in attack plans will react independently, but we still list the attacks here.
						if (attacker.hasClass("Structure")) {
							// todo: we ultimately have to check wether it's a danger point or an isolated area, and if it's a danger point, mark it as so.
							
							// Right now, to make the AI less gameable, we'll mark any surrounding resource as inaccessible.
							// usual tower range is 80. Be on the safe side.
							var close = gameState.getResourceSupplies("wood").filter(Filters.byDistance(attacker.position(), 90));
							close.forEach(function (supply) { //}){
								supply.setMetadata(PlayerID, "inaccessible", true);
							});
							
						} else {
							// TODO: right now a soldier always retaliate... Perhaps it should be set in "Defence" mode.							
							if (!attacker.hasClass("Female") && !attacker.hasClass("Ship")) {
								// This unit is dangerous. if it's in an army, it's being dealt with.
								// if it's not in an army, it means it's either a lone raider, or it has got friends.
								// In which case we must check for other dangerous units around, and perhaps armify them.
								// TODO: perhaps someday army detection will have improved and this will require change.
								var armyID = attacker.getMetadata(PlayerID, "inArmy");
								if (armyID == undefined || !this.enemyArmy[attacker.owner()] || !this.enemyArmy[attacker.owner()][armyID]) {
									if (this.reevaluateEntity(gameState, attacker))
									{
										var position = attacker.position();
										var close = HQ.enemyWatchers[attacker.owner()].enemySoldiers.filter(Filters.byDistance(position, self.armyCompactSize));
										
										if (close.length > 2 || ourUnit.hasClass("Support") || attacker.hasClass("Siege"))
										{
											// armify it, then armify units close to him.
											this.armify(gameState,attacker);
											armyID = attacker.getMetadata(PlayerID, "inArmy");
											
											close.forEach(function (ent) { //}){
												if (SquareVectorDistance(position, ent.position()) < self.armyCompactSize)
												{
													ent.setMetadata(PlayerID, "inArmy", armyID);
														  self.enemyArmy[ent.owner()][armyID].addEnt(ent);
												}
											});
										return;	// don't use too much processing power. If there are other cases, they'll be processed soon enough.
										}
									}
									// Defencemanager will deal with them in the next turn.
								}
								if (ourUnit && ourUnit.hasClass("Unit")) {
									if (ourUnit.hasClass("Support")) {
										// let's try to garrison this support unit.
										if (ourUnit.position())
										{
											var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).filterNearest(ourUnit.position(),4).toEntityArray();
											var garrisoned = false;
											for (var i in buildings)
											{
												var struct = buildings[i];
												if (struct.garrisoned() && struct.garrisonMax() - struct.garrisoned().length > 0)
												{
													garrisoned = true;
													ourUnit.garrison(struct);
													break;
												}
											}
											if (!garrisoned) {
												ourUnit.flee(attacker);
												ourUnit.setMetadata(PlayerID,"fleeing", gameState.getTimeElapsed());
											}
										}
									} else {
										// It's a soldier. Right now we'll retaliate
										// TODO: check for stronger units against this type, check for fleeing options, etc.
										// Check also for neighboring towers and garrison there perhaps?
										ourUnit.attack(e.msg.attacker);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}; // nice sets of closing brackets, isn't it?

// At most, this will put defcon to 4
Defence.prototype.DealWithWantedUnits = function(gameState, events, HQ) {
	//if (gameState.defcon() < 3)
	//	return;
	
	var self = this;
	
	var nbOfAttackers = 0;
	var nbOfDealtWith = 0;
	
	// clean up before adding new units (slight speeding up, since new units can't already be dead)
	for (var i in this.listOfWantedUnits) {
		if (this.listOfWantedUnits[i].length === 0 || this.listOfEnemies[i] !== undefined) {	// unit died/was converted/is already dealt with as part of an army
			delete this.WantedUnitsAttacker[i];
			delete this.listOfWantedUnits[i];
		} else {
			nbOfAttackers++;
			if (this.WantedUnitsAttacker[i].length > 0)
				nbOfDealtWith++;
		}
	}

	// note: we can deal with units the way we want because anyway, the Army Defender has already done its task.
	// If there are still idle defenders here, it's because they aren't needed.
	// I can also call other units: they're not needed.
	// Note however that if the defcon level is too high, this won't do anything because it's low priority.
	// this also won't take units from attack managers
	
	if (nbOfAttackers === 0)
		return;
	
	// at most, we'll deal with 3 enemies at once.
	if (nbOfDealtWith >= 3)
		return;
	
	// dynamic properties are not updated nearly fast enough here so a little caching
	var addedto = {};
	
	// we send 3 units to each target just to be sure. TODO refine.
	// we do not use plan units
	this.idleDefs.forEach(function(ent) {
		if (nbOfDealtWith < 3 && nbOfAttackers > 0 && ent.getMetadata(PlayerID, "plan") == undefined)
			for (var o in self.listOfWantedUnits) {
				if ( (addedto[o] == undefined && self.WantedUnitsAttacker[o].length < 3) || (addedto[o] && self.WantedUnitsAttacker[o].length + addedto[o] < 3)) {
					if (self.WantedUnitsAttacker[o].length === 0)
						nbOfDealtWith++;
						  
					ent.setMetadata(PlayerID, "formerrole", ent.getMetadata(PlayerID, "role"));
					ent.setMetadata(PlayerID, "role","defence");
					ent.setMetadata(PlayerID, "subrole", "defending");
					var attackPos = gameState.getEntityById(+o).position();
					if (attackPos)
						ent.attackMove(attackPos[0],attackPos[1]);
					// TODO: should probably be an else here, unless it really can't happen
					ent.setStance("aggressive");
					if (addedto[o])
						addedto[o]++; 
					else
						addedto[o] = 1;
					break;
				}
				if (self.WantedUnitsAttacker[o].length == 3)
					 nbOfAttackers--;	// we hav eenough units, mark this one as being OKAY
			}
	});
	
	// still some undealt with attackers, recruit citizen soldiers
	if (nbOfAttackers > 0 && nbOfDealtWith < 2) {
	
	gameState.setDefcon(4);
	
	var newSoldiers = gameState.getOwnEntitiesByRole("worker");
	newSoldiers.forEach(function(ent) {
		// If we're not female, we attack
		if (ent.hasClass("CitizenSoldier"))
			if (nbOfDealtWith < 3 && nbOfAttackers > 0)
				for (var o in self.listOfWantedUnits) {
					if ( (addedto[o] == undefined && self.WantedUnitsAttacker[o].length < 3) || (addedto[o] && self.WantedUnitsAttacker[o].length + addedto[o] < 3)) {
						if (self.WantedUnitsAttacker[o].length === 0)
							nbOfDealtWith++;
						ent.setMetadata(PlayerID, "formerrole", ent.getMetadata(PlayerID, "role"));
						ent.setMetadata(PlayerID, "role","defence");
						ent.setMetadata(PlayerID, "subrole", "defending");
						var attackPos = gameState.getEntityById(+o).position();
						if (attackPos)
							ent.attackMove(attackPos[0],attackPos[1]);
						ent.setStance("aggressive");
						if (addedto[o])
							addedto[o]++; 
						else
							addedto[o] = 1;
						break;
					}
					if (self.WantedUnitsAttacker[o].length == 3)
						nbOfAttackers--;	// we hav eenough units, mark this one as being OKAY
				}
	});	
	}
	return;
}
