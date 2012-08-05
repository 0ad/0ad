// directly imported from Marilyn, with slight modifications to work with qBot.

function Defence(){
	this.defenceRatio = 1.8; // How many defenders we want per attacker.  Need to balance fewer losses vs. lost economy
	// note: the choice should be a no-brainer most of the time: better deflect the attack.
	
	this.totalAttackNb = 0;	// used for attack IDs
	this.attacks = [];
	this.toKill = [];
	
	// keeps a list of targeted enemy at instant T
	this.attackerCache = {};
	this.listOfEnemies = {};
	this.listedEnemyCollection = null;	// entity collection of this.listOfEnemies
	
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
// 6 (or more): no danger whatsoever detected
// 5: local zones of danger (ie a tower somewhere, things like that)
// 4: a few enemy units inbound, like a scout or something. (local danger). Usually seen as the last level before a true "attack"
// 3: reasonnably sized enemy army inbound, local danger
// 2: well sized enemy army inbound, general danger
// 1: Sizable enemy army inside of my base, general danger.


Defence.prototype.update = function(gameState, events, militaryManager){
	
	Engine.ProfileStart("Defence Manager");
			
	// a litlle cache-ing
	if (!this.idleDefs) {
		var filter = Filters.and(Filters.byMetadata("role", "defence"), Filters.isIdle());
		this.idleDefs = gameState.getOwnEntities().filter(filter);
		this.idleDefs.registerUpdates();
	}
	if (!this.defenders) {
		var filter = Filters.byMetadata("role", "defence");
		this.defenders = gameState.getOwnEntities().filter(filter);
		this.defenders.registerUpdates();
	}
	if (!this.listedEnemyCollection) {
		var filter = Filters.byMetadata("listed-enemy", true);
		this.listedEnemyCollection = gameState.getEnemyEntities().filter(filter);
		this.listedEnemyCollection.registerUpdates();
	}
	this.myBuildings = gameState.getOwnEntities().filter(Filters.byClass("Structure")).toEntityArray();
	this.myUnits = gameState.getOwnEntities().filter(Filters.byClass("Unit"));
	
	this.territoryMap = Map.createTerritoryMap(gameState);	// used by many func

	// First step: we deal with enemy armies, those are the highest priority.
	this.defendFromEnemyArmies(gameState, events, militaryManager);
	
	// second step: we loop through messages, and sort things as needed (dangerous buildings, attack by animals, ships, lone units, whatever).
	// TODO
	this.MessageProcess(gameState,events,militaryManager);
	
	this.DealWithWantedUnits(gameState,events,militaryManager);

	// putting unneeded units at rest
	this.idleDefs.forEach(function(ent) {
		ent.setMetadata("role", ent.getMetadata("formerrole") );
		ent.setMetadata("subrole", undefined);
	});

	Engine.ProfileStop();
	
	return;
};
// returns armies that are still seen as dangerous (in the LOS of any of my buildings for now)
Defence.prototype.reevaluateDangerousArmies = function(gameState, armies) {
	var stillDangerousArmies = {};
	for (i in armies) {
		var pos = armies[i].getCentrePosition();
		if (armies[i].getCentrePosition() && +this.territoryMap.point(armies[i].getCentrePosition()) - 64 === +gameState.player) {
			stillDangerousArmies[i] = armies[i];
			continue;
		}
		for (o in this.myBuildings) {
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
	for (i in armies) {
		if (armies[i].getCentrePosition() && +this.territoryMap.point(armies[i].getCentrePosition()) - 64 === +gameState.player) {
			DangerousArmies[i] = armies[i];
		}
	}
	return DangerousArmies;
}
// This deals with incoming enemy armies, setting the defcon if needed. It will take new soldiers, and assign them to attack
// it's still a fair share of dumb, so TODO improve
Defence.prototype.defendFromEnemyArmies = function(gameState, events, militaryManager) {
	
	// The enemy Watchers keep a list of armies. This class here tells them if an army is dangerous, and they manage the merging/splitting/disbanding.
	// With this system, we can get any dangerous armies. Thus, we can know where the danger is, and react.
	// So Defence deals with attacks from animals too (which aren't watched).
	// The attackrs here are dealt with on a per unit basis.
	// We keep a list of idle defenders. For any new attacker, we'll check if we have any idle defender available, and if not, we assign available units.
	// At the end of each turn, if we still have idle defenders, we either assign them to neighboring units, or we release them.
	
	var dangerArmies = {};
	this.enemyUnits = {};
	
	// for now armies are never seen as "no longer dangerous"... TODO
	for (enemyID in militaryManager.enemyWatchers) {
		this.enemyUnits[enemyID] = militaryManager.enemyWatchers[enemyID].getAllEnemySoldiers();
		
		var dangerousArmies = militaryManager.enemyWatchers[enemyID].getDangerousArmies();
		// we check if all the dangerous armies are still dangerous.
		var newDangerArmies = this.reevaluateDangerousArmies(gameState,dangerousArmies);
		
		var safeArmies = militaryManager.enemyWatchers[enemyID].getSafeArmies();
		// we check not dangerous armies, to see if they suddenly became dangerous
		var unsafeArmies = this.evaluateArmies(gameState,safeArmies);
		for (i in unsafeArmies)
			newDangerArmies[i] = unsafeArmies[i];
		
		// and any dangerous armies we push in "dangerArmies"
		militaryManager.enemyWatchers[enemyID].resetDangerousArmies();		
		for (o in newDangerArmies)
			militaryManager.enemyWatchers[enemyID].setAsDangerous(o);
		
		for (i in newDangerArmies)
			dangerArmies[i] = newDangerArmies[i];
	}
	
	var self = this;
	
	var nbOfAttackers = 0;
	
	// clean up before adding new units (slight speeding up, since new units can't already be dead)
	for (i in this.listOfEnemies) {
		if (this.listOfEnemies[i].length === 0) {
			// if we had defined the attackerCache, ie if we had tried to attack this unit.
			if (this.attackerCache[i] !== undefined) {
				this.attackerCache[i].forEach(function(ent) { ent.stopMoving(); });
				delete this.attackerCache[i];
			}
			delete this.listOfEnemies[i];
		} else {
			var unit = this.listOfEnemies[i].toEntityArray()[0];
			var enemyWatcher = militaryManager.enemyWatchers[unit.owner()];
			if (enemyWatcher.isPartOfDangerousArmy(unit.id())) {
				nbOfAttackers++;
			} else {
				// if we had defined the attackerCache, ie if we had tried to attack this unit.
				if (this.attackerCache[unit.id()] != undefined) {
					this.attackerCache[unit.id()].forEach(function(ent) { ent.stopMoving(); });
					delete this.attackerCache[unit.id()];
				}
				this.listOfEnemies[unit.id()].toEntityArray()[0].setMetadata("listed-enemy",undefined);
				delete this.listOfEnemies[unit.id()];
			}
		}
	}
		
	// okay so now, for every dangerous armies, we loop.
	for (armyID in dangerArmies) {
		// looping through army units
		dangerArmies[armyID].forEach(function(ent) {
			// do we have already registered an entityCollection for it?
			if (self.listOfEnemies[ent.id()] === undefined) {
				// no, we register a new entity collection in listOfEnemies, listing exactly one unit as long as it remains alive and owned by my enemy.
				// can't be bothered to recode everything
				var owner = ent.owner();
				var filter = Filters.and(Filters.byOwner(owner),Filters.byID(ent.id()));
				self.listOfEnemies[ent.id()] = self.enemyUnits[owner].filter(filter);
				self.listOfEnemies[ent.id()].registerUpdates();
				self.listOfEnemies[ent.id()].length;
				self.listOfEnemies[ent.id()].toEntityArray()[0].setMetadata("listed-enemy",true);

				// let's also register an entity collection for units attacking this unit (so we can new if it's attacked)
				filter = Filters.and(Filters.byOwner(gameState.player),Filters.byTargetedEntity(ent.id()));
				self.attackerCache[ent.id()] = self.myUnits.filter(filter);
				self.attackerCache[ent.id()].registerUpdates();
				nbOfAttackers++;
			}
		});
	}
	// Reordering attack because the pathfinder is for now not dynamically updated
	for (o in this.attackerCache) {
		if ((this.attackerCacheLoopIndicator + o) % 2 === 0) {
			this.attackerCache[o].forEach(function (ent) {
										  ent.attack(+o);
										  });
		}
	}
	this.attackerCacheLoopIndicator++;
	this.attackerCacheLoopIndicator = this.attackerCacheLoopIndicator % 2;

	if (nbOfAttackers === 0) {
		militaryManager.unpauseAllPlans(gameState);
		return;
	}
	// If I'm here, I have a list of enemy units, and a list of my units attacking it (in absolute terms, I could use a list of any unit attacking it).
	// now I'll list my idle defenders, then my idle soldiers that could defend.
	// and then I'll assign my units.
	// and then rock on.	
	/*
	 if (nbOfAttackers === 0) {
		return;
	} else if (nbOfAttackers < 5){
		gameState.upDefcon(4);	// few local units
	} else if (nbOfAttackers >= 5){
		gameState.upDefcon(3);	// local attack, dangerous but not hugely threatening for my survival
	}
	
	if (this.idleDefs.length < nbOfAttackers) {
		gameState.upDefcon(2);	// general danger
	}
	*/
	
	// todo: improve the logic against attackers.
		
	// reupdate the existing defenders.
	
	this.idleDefs.forEach(function(ent) {
		ent.setMetadata("subrole","newdefender");
	});

	
	
	nbOfAttackers *= this.defenceRatio;
	// Assume those taken care of.
	nbOfAttackers -= +(this.defenders.length);
	
	// need new units?
	if (nbOfAttackers <= 0)
		return;
	
	// yes. We'll pick new units (pretty randomly for now, todo)
	// first from attack plans, then from workers.
	var newSoldiers = gameState.getOwnEntities().filter(function (ent) {
		if (ent.getMetadata("plan") != undefined)
			return true;
		return false;
	});
	newSoldiers.forEach(function(ent) {
		if (ent.getMetadata("subrole","attacking"))	// gone with the wind to avenge their brothers.
			return;
		if (nbOfAttackers <= 0)
			return;
		militaryManager.pausePlan(gameState,ent.getMetadata("plan"));
		ent.setMetadata("formerrole", ent.getMetadata("role"));
		ent.setMetadata("role","defence");
		ent.setMetadata("subrole","newdefender");
		nbOfAttackers--;
	});


	if (nbOfAttackers > 0) {
		newSoldiers = gameState.getOwnEntitiesByRole("worker");
		newSoldiers.forEach(function(ent) {
			if (nbOfAttackers <= 0)
				return;
			// If we're not female, we attack
			// and if we're not already assigned from above (might happen, not sure, rather be cautious)
			if (ent.hasClass("CitizenSoldier") && ent.getMetadata("subrole") != "newdefender") {
				ent.setMetadata("formerrole", "worker");
				ent.setMetadata("role","defence");
				ent.setMetadata("subrole","newdefender");
				nbOfAttackers--;
			}
		});
	}
	// okay
	newSoldiers = gameState.getOwnEntitiesByMetadata("subrole","newdefender");
	// TODO. For now, each unit will pick the closest unit that is attacked by only one/zero guy, or any if there is none.
	// ought to regroup them first for optimization.
	
	newSoldiers.forEach(function(ent) { //}) {
		var enemies = self.listedEnemyCollection.filterNearest(ent.position()).toEntityArray();
		var target = -1;
		var secondaryTarget = enemies[0];	// second best pick
		for (o in enemies) {
			var enemy = enemies[o];
			if (self.attackerCache[enemy.id()].length < 2) {
				target = +enemy.id();
				break;
			}
		}
		ent.setMetadata("subrole","defending");
		ent.attack(+target);
	});

	return;
}

// this processes the attackmessages
// So that a unit that gets attacked will not be completely dumb.
// warning: huge levels of indentation coming.
Defence.prototype.MessageProcess = function(gameState,events, militaryManager) {
	for (var key in events){
		var e = events[key];
		if (e.type === "Attacked" && e.msg){
			if (gameState.isEntityOwn(gameState.getEntityById(e.msg.target))) {
				var attacker = gameState.getEntityById(e.msg.attacker);
				var ourUnit = gameState.getEntityById(e.msg.target);
				// the attacker must not be already dead, and it must not be me (think catapults that miss).
				if (attacker !== undefined && attacker.owner() !== gameState.player) {
					// note: our unit can already by dead by now... We'll then have to rely on the enemy to react.
					// if we're not on enemy territory
					var territory = +this.territoryMap.point(attacker.position())  - 64;
					
					// let's check for animals
					if (attacker.owner() == 0) {
						// if our unit is still alive, we make it react
						// in this case we attack.
						if (ourUnit !== undefined) {
							if (ourUnit.hasClass("Unit") && !ourUnit.hasClass("Support"))
								ourUnit.attack(e.msg.attacker);
							else {
								ourUnit.flee(attacker);
							}
						}
						// anyway we'll register the animal as dangerous, and attack it.
						var filter = Filters.byID(attacker.id());
						this.listOfWantedUnits[attacker.id()] = gameState.getEntities().filter(filter);
						this.listOfWantedUnits[attacker.id()].registerUpdates();
						this.listOfWantedUnits[attacker.id()].length;
						filter = Filters.and(Filters.byOwner(gameState.player),Filters.byTargetedEntity(attacker.id()));
						this.WantedUnitsAttacker[attacker.id()] = this.myUnits.filter(filter);
						this.WantedUnitsAttacker[attacker.id()].registerUpdates();
						this.WantedUnitsAttacker[attacker.id()].length;
					} else if (territory != attacker.owner()) {	// preliminary check: attacks in enemy territory are not counted as attacks
						// Also TODO: this does not differentiate with buildings...
						// These ought to be treated differently.
						// units in attack plans will react independently, but we still list the attacks here.
						if (attacker.hasClass("Structure")) {
							// todo: we ultimately have to check wether it's a danger point or an isolated area, and if it's a danger point, mark it as so.
						} else {
							// TODO: right now a soldier always retaliate... Perhaps it should be set in "Defence" mode.							
							if (!attacker.hasClass("Female") && !attacker.hasClass("Ship")) {
								// This unit is dangerous. We'll ask the enemy manager if it's part of a big army, in which case we'll list it as dangerous (so it'll be treated next turn by the other manager)
								// If it's not part of a big army, depending on our priority we may want to kill it (using the same things as animals for that)
								// TODO (perhaps not any more, but let's mark it anyway)
								var army = militaryManager.enemyWatchers[attacker.owner()].getArmyFromMember(attacker.id());
								if (army[1].length > 5) {
									militaryManager.enemyWatchers[attacker.owner()].setAsDangerous(army[0]);
								} else if (!militaryManager.enemyWatchers[attacker.owner()].isDangerous(army[0])) {
									// we register this unit as wanted, TODO register the whole army
									// another function will deal with it.
									var filter = Filters.and(Filters.byOwner(attacker.owner()),Filters.byID(attacker.id()));
									this.listOfWantedUnits[attacker.id()] = this.enemyUnits[attacker.owner()].filter(filter);
									this.listOfWantedUnits[attacker.id()].registerUpdates();
									this.listOfWantedUnits[attacker.id()].length;
									filter = Filters.and(Filters.byOwner(gameState.player),Filters.byTargetedEntity(attacker.id()));
									this.WantedUnitsAttacker[attacker.id()] = this.myUnits.filter(filter);
									this.WantedUnitsAttacker[attacker.id()].registerUpdates();
									this.WantedUnitsAttacker[attacker.id()].length;
								}
								if (ourUnit && ourUnit.hasClass("Unit") && ourUnit.getMetadata("role") != "attack") {
									if (ourUnit.hasClass("Support")) {
										// TODO: it's a villager. Garrison it.
										// TODO: make other neighboring villagers garrison

										// Right now we'll flee from the attacker.
										ourUnit.flee(attacker);
									} else {
										// It's a soldier. Right now we'll retaliate
										// TODO: check for stronger units against this type, check for fleeing options, etc.
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
};
// At most, this will put defcon to 5
Defence.prototype.DealWithWantedUnits = function(gameState, events, militaryManager) {
	//if (gameState.defcon() < 3)
	//	return;
	
	var self = this;
	
	var nbOfAttackers = 0;
	var nbOfDealtWith = 0;
	
	// clean up before adding new units (slight speeding up, since new units can't already be dead)
	for (i in this.listOfWantedUnits) {
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
	
	// at most, we'll deal with two enemies at once.
	if (nbOfDealtWith >= 2)
		return;
	
	// dynamic properties are not updated nearly fast enough here so a little caching
	var addedto = {};
	
	// we send 3 units to each target just to be sure. TODO refine.
	// we do not use plan units
	this.idleDefs.forEach(function(ent) {
		if (nbOfDealtWith < 2 && nbOfAttackers > 0 && ent.getMetadata("plan") == undefined)
			for (o in self.listOfWantedUnits) {
				if ( (addedto[o] == undefined && self.WantedUnitsAttacker[o].length < 3) || (addedto[o] && self.WantedUnitsAttacker[o].length + addedto[o] < 3)) {
					if (self.WantedUnitsAttacker[o].length === 0)
						nbOfDealtWith++;
					ent.setMetadata("subrole", "defending");
					ent.attack(+o);
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
	
	//gameState.upDefcon(5);
	
	var newSoldiers = gameState.getOwnEntitiesByRole("worker");
	newSoldiers.forEach(function(ent) {
		// If we're not female, we attack
		if (ent.hasClass("CitizenSoldier"))
			if (nbOfDealtWith < 2 && nbOfAttackers > 0)
				for (o in self.listOfWantedUnits) {
					if ( (addedto[o] == undefined && self.WantedUnitsAttacker[o].length < 3) || (addedto[o] && self.WantedUnitsAttacker[o].length + addedto[o] < 3)) {
						if (self.WantedUnitsAttacker[o].length === 0)
							nbOfDealtWith++;
						ent.setMetadata("subrole", "defending");
						ent.attack(+o);
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
