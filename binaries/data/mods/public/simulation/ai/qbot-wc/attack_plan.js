// basically an attack plan. The name is an artifact.
function CityAttack(gameState, militaryManager, uniqueID, targetEnemy, type , targetFinder){
	
	//This is the list of IDs of the units in the plan
	this.idList=[];
	
	this.state = "unexecuted";
	this.targetPlayer = targetEnemy;
	if (this.targetPlayer === -1 || this.targetPlayer === undefined) {
		// let's find our prefered target, basically counting our enemies units.
		var enemyCount = {};
		for (var i = 1; i <=8; i++)
			enemyCount[i] = 0;
		gameState.getEntities().forEach(function(ent) { if (gameState.isEntityEnemy(ent) && ent.owner() !== 0) { enemyCount[ent.owner()]++; } });
		var max = 0;
		for (i in enemyCount)
			if (enemyCount[i] >= max)
			{
				this.targetPlayer = +i;
				max = enemyCount[i];
			}
	}
	debug ("Target = " +this.targetPlayer);
	this.targetFinder = targetFinder || this.defaultTargetFinder;
	this.type = type || "normal";
	this.name = uniqueID;
	this.healthRecord = [];
		
	this.timeOfPlanStart = gameState.getTimeElapsed();	// we get the time at which we decided to start the attack
	this.maxPreparationTime = 300*1000;
	
	this.pausingStart = 0;
	this.totalPausingTime = 0;
	this.paused = false;
	
	this.onArrivalReaction = "proceedOnTargets";

	this.unitStat = {};
	this.unitStat["RangedInfantry"] = { "priority" : 1, "minSize" : 4, "targetSize" : 10, "batchSize" : 5, "classes" : ["Infantry","Ranged"], "templates" : [] };
	this.unitStat["MeleeInfantry"] = { "priority" : 1, "minSize" : 4, "targetSize" : 10, "batchSize" : 5, "classes" : ["Infantry","Melee"], "templates" : [] };
	this.unitStat["MeleeCavalry"] = { "priority" : 1, "minSize" : 3, "targetSize" : 8 , "batchSize" : 3, "classes" : ["Cavalry","Ranged"], "templates" : [] };
	this.unitStat["RangedCavalry"] = { "priority" : 1, "minSize" : 3, "targetSize" : 8 , "batchSize" : 3, "classes" : ["Cavalry","Melee"], "templates" : [] };
	this.unitStat["Siege"] = { "priority" : 1, "minSize" : 0, "targetSize" : 2 , "batchSize" : 1, "classes" : ["Siege"], "templates" : [] };

	var filter = Filters.and(Filters.byMetadata("plan",this.name),Filters.byOwner(gameState.player));
	this.unitCollection = gameState.getOwnEntities().filter(filter);
	this.unitCollection.registerUpdates();
	this.unitCollection.length;
	
	this.unit = {};
	
	// each array is [ratio, [associated classes], associated EntityColl, associated unitStat, name ]
	this.buildOrder = [];
	
	// defining the entity collections. Will look for units I own, that are part of this plan.
	// Also defining the buildOrders.
	for (unitCat in this.unitStat) {
		var cat = unitCat;
		var Unit = this.unitStat[cat];

		var filter = Filters.and(Filters.byClassesAnd(Unit["classes"]),Filters.and(Filters.byMetadata("plan",this.name),Filters.byOwner(gameState.player)));
		this.unit[cat] = gameState.getOwnEntities().filter(filter);
		this.unit[cat].registerUpdates();
		this.unit[cat].length;
		this.buildOrder.push([0, Unit["classes"], this.unit[cat], Unit, cat]);
	}
	/*if (gameState.getTimeElapsed() > 900000)	// 15 minutes
	{
		
		this.unitStat.Cavalry.Ranged["minSize"] = 5;
		this.unitStat.Cavalry.Melee["minSize"] = 5;
		this.unitStat.Infantry.Ranged["minSize"] = 10;
		this.unitStat.Infantry.Melee["minSize"] = 10;
		this.unitStat.Cavalry.Ranged["targetSize"] = 10;
		this.unitStat.Cavalry.Melee["targetSize"] = 10;
		this.unitStat.Infantry.Ranged["targetSize"] = 20;
		this.unitStat.Infantry.Melee["targetSize"] = 20;
		this.unitStat.Siege["targetSize"] = 5;
		this.unitStat.Siege["minSize"] = 2;
		 
	} else {
		this.maxPreparationTime = 180000;
	}*/
	// todo: REACTIVATE (in all caps)
	if (type === "harass_raid" && 0 == 1)
	{
		this.targetFinder = this.raidingTargetFinder;
		this.onArrivalReaction = "huntVillagers";
		
		this.type = "harass_raid";
		// This is a Cavalry raid against villagers. A Cavalry Swordsman has a bonus against these. Only build these
		this.maxPreparationTime = 180000;	// 3 minutes.
		if (gameState.playerData.civ === "hele")	// hellenes have an ealry Cavalry Swordsman
		{
			this.unitCount.Cavalry.Melee = { "subCat" : ["Swordsman"] , "usesSubcategories" : true, "Swordsman" : undefined, "priority" : 1, "currentAmount" : 0, "minimalAmount" : 0, "preferedAmount" : 0 };
			this.unitCount.Cavalry.Melee.Swordsman = { "priority" : 1, "currentAmount" : 0, "minimalAmount" : 4, "preferedAmount" : 7, "fallback" : "abort" };
		} else {
			this.unitCount.Cavalry.Melee = { "subCat" : undefined , "usesSubcategories" : false, "priority" : 1, "currentAmount" : 0, "minimalAmount" : 4, "preferedAmount" : 7 };
		}
		this.unitCount.Cavalry.Ranged["minimalAmount"] = 0;
		this.unitCount.Cavalry.Ranged["preferedAmount"] = 0;
		this.unitCount.Infantry.Ranged["minimalAmount"] = 0;
		this.unitCount.Infantry.Ranged["preferedAmount"] = 0;
		this.unitCount.Infantry.Melee["minimalAmount"] = 0;	
		this.unitCount.Infantry.Melee["preferedAmount"] = 0;
		this.unitCount.Siege["preferedAmount"] = 0;
	}
	this.anyNotMinimal = true;	// used for support plans

	// taking this so that fortresses won't crash it for now. TODO: change the rally point if it becomes invalid
	if(gameState.ai.pathsToMe.length > 1)
		var position = [(gameState.ai.pathsToMe[0][0]+gameState.ai.pathsToMe[1][0])/2.0,(gameState.ai.pathsToMe[0][1]+gameState.ai.pathsToMe[1][1])/2.0];
	else
		var position = [gameState.ai.pathsToMe[0][0],gameState.ai.pathsToMe[0][1]];
	
	var CCs = gameState.getOwnEntities().filter(Filters.byClass("CivCentre"));
	var nearestCCArray = CCs.filterNearest(position, 1).toEntityArray();
	var CCpos = nearestCCArray[0].position();
	this.rallyPoint = [0,0];
	this.rallyPoint[0] = (position[0]*3 + CCpos[0]) / 4.0;
	this.rallyPoint[1] = (position[1]*3 + CCpos[1]) / 4.0;
	if (type == 'harass_raid')
	{
		this.rallyPoint[0] = (position[0]*3.9 + 0.1 * CCpos[0]) / 4.0;
		this.rallyPoint[1] = (position[1]*3.9 + 0.1 * CCpos[1]) / 4.0;
	}
	
	// some variables for during the attack
	this.lastPosition = [0,0];
	this.position = [0,0];

	this.threatList = [];	// sounds so FBI
	this.tactics = undefined;
	
	gameState.ai.queueManager.addQueue("plan_" + this.name, 100);
	this.queue = gameState.ai.queues["plan_" + this.name];
	
	
	this.assignUnits(gameState);

};

CityAttack.prototype.getName = function(){
	return this.name;
};
CityAttack.prototype.getType = function(){
	return this.type;
};
// Returns true if the attack can be executed at the current time
// Basically his checks we have enough units.
// We run a count of our units.
CityAttack.prototype.canStart = function(gameState){	
	for (unitCat in this.unitStat) {
		var Unit = this.unitStat[unitCat];
		if (this.unit[unitCat].length < Unit["minSize"])
			return false;
	}
	return true;

	// TODO: check if our target is valid and a few other stuffs (good moment to attack?)
};
CityAttack.prototype.isStarted = function(){
	return !(this.state == "unexecuted");
};

CityAttack.prototype.isPaused = function(){
	return this.paused;
};
CityAttack.prototype.setPaused = function(gameState, boolValue){
	if (!this.paused && boolValue === true) {
		this.pausingStart = gameState.getTimeElapsed();
		this.paused = true;
		debug ("Pausing attack plan " +this.name);
	} else if (this.paused && boolValue === false) {
		this.totalPausingTime += gameState.getTimeElapsed() - this.pausingStart;
		this.paused = false;
		debug ("Unpausing attack plan " +this.name);
	}
};
CityAttack.prototype.mustStart = function(gameState){
	var MaxReachedEverywhere = true;
	for (unitCat in this.unitStat) {
		var Unit = this.unitStat[unitCat];
		if (this.unit[unitCat].length < Unit["targetSize"])
			MaxReachedEverywhere = false;
	}
	if (MaxReachedEverywhere)
		return true;
	return (this.maxPreparationTime + this.timeOfPlanStart + this.totalPausingTime < gameState.getTimeElapsed());
};

// Three returns possible: 1 is "keep going", 0 is "failed plan", 2 is "start"
CityAttack.prototype.updatePreparation = function(gameState, militaryManager,events) {
	if (this.isPaused())
		return 0;	// continue
	
	Engine.ProfileStart("Update Preparation");
	
	// let's sort by training advancement, ie 'current size / target size'
	this.buildOrder.sort(function (a,b) {
		a[0] = a[2].length/a[3]["targetSize"];
		b[0] = b[2].length/b[3]["targetSize"];
		return (a[0]) - (b[0]);
	});
	
	this.assignUnits(gameState);
	
	if ( (gameState.ai.turn + gameState.ai.player) % 40 == 0)
		this.AllToRallyPoint(gameState);

	var canstart = this.canStart(gameState);

	Engine.ProfileStart("Creating units and looking through events");
	
	// gets the number in training of the same kind as the first one.
	var specialData = "Plan_"+this.name+"_"+this.buildOrder[0][4];
	var inTraining = gameState.countOwnQueuedEntitiesWithMetadata("special",specialData);
	if (this.queue.countTotalQueuedUnits() + inTraining + this.buildOrder[0][2].length < Math.min(15,this.buildOrder[0][3]["targetSize"]) ) {
		if (this.buildOrder[0][0] < 1 && this.queue.countTotalQueuedUnits() < 5) {
			
			var template = militaryManager.findBestTrainableUnit(gameState, this.buildOrder[0][1], [ ["strength",1], ["cost",1] ] );
			//debug ("tried " + uneval(this.buildOrder[0][1]) +", and " + template);
			// HACK (TODO replace) : if we have no trainable template... Then we'll simply remove the buildOrder, effectively removing the unit from the plan.
			if (template === undefined) {
				debug ("Abandonning the idea of recruiting " + uneval(this.buildOrder[0][1]) + " as no recruitable template were found" );
				this.buildOrder.splice(0,1);
			} else {
				this.queue.addItem( new UnitTrainingPlan(gameState,template, { "role" : "attack", "plan" : this.name, "special" : specialData },this.buildOrder[0][3]["batchSize"] ) );
			}
		}
	}
	// can happen for now
	if (this.buildOrder.length === 0) {
		return 0;	// will abort the plan, should return something else
	}

	for (var key in events){
		var e = events[key];
		if (e.type === "Attacked" && e.msg){
			if (this.unitCollection.toIdArray().indexOf(e.msg.target) !== -1){
				var attacker = gameState.getEntityById(e.msg.attacker);
				if (attacker && attacker.position()) {
					this.unitCollection.attack(e.msg.attacker);
					break;
				}
			}
		}
	}
	Engine.ProfileStop();
	// we count our units by triggering "canStart"
	// returns false if we can no longer have time and cannot start.
	// returns 0 if I must start and can't, returns 1 if I don't have to start, and returns 2 if I must start and can
	if (!this.mustStart(gameState))
		return 1;
	else if (canstart)
		return 2;
	else
		return 0;
	return 0;
};
CityAttack.prototype.assignUnits = function(gameState){
	var self = this;

	// TODO: assign myself units that fit only, right now I'm getting anything.
	
	/*
	 // I'll take any unit set to "Defense" that has no subrole (ie is set to be a defensive unit, but has no particular task)
	// I assign it to myself, and then it's mine, the entity collection will detect it.
	var Defenders = gameState.getOwnEntitiesByRole("defence");
	Defenders.forEach(function(ent) {
		if (ent.getMetadata("subrole") == "idle" || !ent.getMetadata("subrole")) {
			ent.setMetadata("role", "attack");
			ent.setMetadata("plan", self.name);
		}
	});*/
	// Assign all no-roles that fit (after a plan aborts, for example).
	var NoRole = gameState.getOwnEntitiesByRole(undefined);
	NoRole.forEach(function(ent) {
		ent.setMetadata("role", "attack");
		ent.setMetadata("plan", self.name);
	});

};
// this sends a unit by ID back to the "rally point"
CityAttack.prototype.ToRallyPoint = function(gameState,id)
{	
	// Move back to nearest rallypoint
	gameState.getEntityById(id).move(this.rallyPoint[0],this.rallyPoint[1]);
}
// this sends all units back to the "rally point" by entity collections.
CityAttack.prototype.AllToRallyPoint = function(gameState) {
	for (unitCat in this.unit) {
		this.unit[unitCat].move(this.rallyPoint[0],this.rallyPoint[1]);
	}
}

// Default target finder aims for conquest critical targets
CityAttack.prototype.defaultTargetFinder = function(gameState, militaryManager){
	var targets = undefined;
	
	targets = militaryManager.enemyWatchers[this.targetPlayer].getEnemyBuildings("ConquestCritical");
	// If there's nothing, attack anything else that's less critical
	if (targets.length == 0) {
		targets = militaryManager.enemyWatchers[this.targetPlayer].getEnemyBuildings("Town");
	}
	if (targets.length == 0) {
		targets = militaryManager.enemyWatchers[this.targetPlayer].getEnemyBuildings("Village");
	}
	return targets;
};

// tupdate
CityAttack.prototype.raidingTargetFinder = function(gameState, militaryManager, Target){
	var targets = undefined;
	if (Target == "villager")
	{
		// let's aim for any resource dropsite. We assume villagers are in the neighborhood (note: the human player could certainly troll us... small (scouting) TODO here.)
		targets = gameState.entities.filter(function(ent) {
				return (ent.hasClass("Structure") && ent.resourceDropsiteTypes() !== undefined && !ent.hasClass("CivCentre") && ent.owner() === this.targetPlayer && ent.position());
		});
		if (targets.length == 0) {
			targets = gameState.entities.filter(function(ent) {
												return (ent.hasClass("CivCentre") && ent.resourceDropsiteTypes() !== undefined && ent.owner() === this.targetPlayer  && ent.position());
												});
		}
		if (targets.length == 0) {
			// if we're here, it means they also don't have no CC... So I'll just take any building at this point.
			targets = gameState.entities.filter(function(ent) {
												return (ent.hasClass("Structure") && ent.owner() === this.targetPlayer  && ent.position());
												});
		}
		return targets;
	} else {
		return this.defaultTargetFinder(gameState, militaryManager);
	}
};

// Executes the attack plan, after this is executed the update function will be run every turn
// If we're here, it's because we have in our IDlist enough units.
// now the IDlist units are treated turn by turn
CityAttack.prototype.StartAttack = function(gameState, militaryManager){

	var targets = [];
	if (this.type === "harass_raid")
		targets = this.targetFinder(gameState, militaryManager, "villager");
	else
	{
		targets = this.targetFinder(gameState, militaryManager);
	
		if (targets.length === 0){
			targets = this.defaultTargetFinder(gameState, militaryManager);
		}
	}
	
	// If we have a target, move to it
	if (targets.length) {		
		var curPos = this.unitCollection.getCentrePosition();
		
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
				return false;
			}
		}
		
		// Find possible distinct paths to the enemy 
		var pathFinder = new PathFinder(gameState);
		var pathsToEnemy = pathFinder.getPaths(curPos, this.targetPos);
		if (! pathsToEnemy){
			pathsToEnemy = [[this.targetPos]];
		}
		this.path = [];
		
		if (this.type !== "harass_raid")
		{
			var rand = Math.floor(Math.random() * pathsToEnemy.length);
			this.path = pathsToEnemy[rand];
		} else {
			this.path = pathsToEnemy[Math.min(2,pathsToEnemy.length-1)];		
		}
		
		this.unitCollection.forEach(function(ent) { ent.setMetadata("subrole", "attacking");});
		
		// filtering by those that started to attack only
		var filter = Filters.byMetadata("subrole","attacking");
		this.unitCollection = this.unitCollection.filter(filter);
		//this.unitCollection.registerUpdates();
		//this.unitCollection.length;
		
		for (unitCat in this.unitStat) {
			var cat = unitCat;
			this.unit[cat] = this.unit[cat].filter(filter);
		}
		
		this.unitCollection.move(this.path[0][0], this.path[0][1]);
		debug ("Started to attack with the plan " + this.name);
		this.state = "walking";
	} else if (targets.length == 0 ) {
		gameState.ai.gameFinished = true;
		debug ("I do not have any target. So I'll just assume I won the game.");
		return true;
	}
	return true;
};

// Runs every turn after the attack is executed
CityAttack.prototype.update = function(gameState, militaryManager, events){
	Engine.ProfileStart("Update Attack");

	// we're marching towards the target
	// Check for attacked units in our band.
	var bool_attacked = false;
	// raids don't care about attacks much
	
	this.position = this.unitCollection.getCentrePosition();
	
	var IDs = this.unitCollection.toIdArray();
	
	// this actually doesn't do anything right now.
	if (this.state === "walking") {
		
		var toProcess = {};
		var armyToProcess = {};
		// Let's check if any of our unit has been attacked. In case yes, we'll determine if we're simply off against an enemy army, a lone unit/builing
		// or if we reached the enemy base. Different plans may react differently.
		for (var key in events) {
			var e = events[key];
			if (e.type === "Attacked" && e.msg) {
				if (IDs.indexOf(e.msg.target) !== -1) {
					var attacker = gameState.getEntityById(e.msg.attacker);
					if (attacker && attacker.position() && attacker.hasClass("Unit") && attacker.owner() != 0) {
						toProcess[attacker.id()] = attacker;
						
						var armyID = militaryManager.enemyWatchers[attacker.owner()].getArmyFromMember(attacker.id());
						armyToProcess[armyID[0]] = armyID[1];
					}
				}
			}
		}
		
		// I don't process attacks if I'm in their base because I'll have already gone to "attacking" mode.
		// I'll process by army
		var total = 0;
		for (armyID in armyToProcess) {
			total += armyToProcess[armyID].length;
			// TODO: if it's a big army, we may want to refer the scouting/defense manager
		}
		

		/*
		 
	}&& this.type !== "harass_raid"){	// walking toward the target
		var sumAttackerPos = [0,0];
		var numAttackers = 0;
		// let's check if one of our unit is not under attack, by any chance.
		for (var key in events){
			var e = events[key];
			if (e.type === "Attacked" && e.msg){
				if (this.unitCollection.toIdArray().indexOf(e.msg.target) !== -1){
					var attacker = HeadQuarters.entity(e.msg.attacker);
					if (attacker && attacker.position()){
						sumAttackerPos[0] += attacker.position()[0];
						sumAttackerPos[1] += attacker.position()[1];
						numAttackers += 1;
						bool_attacked = true;
						// todo: differentiate depending on attacker type... If it's a ship, let's not do anythin, a building, depends on the attack type/
						if (this.threatList.indexOf(e.msg.attacker) === -1)
						{
							var enemySoldiers = HeadQuarters.getEnemySoldiers().toEntityArray();
							for (j in enemySoldiers)
							{
								var enemy = enemySoldiers[j];
								if (enemy.position() === undefined)	// likely garrisoned
									continue;
								if (inRange(enemy.position(), attacker.position(), 1000) && this.threatList.indexOf(enemy.id()) === -1)
									this.threatList.push(enemy.id());
							}
							this.threatList.push(e.msg.attacker);
						}
					}
				}
			}
		}
		if (bool_attacked > 0){
			var avgAttackerPos = [sumAttackerPos[0]/numAttackers, sumAttackerPos[1]/numAttackers];
			units.move(avgAttackerPos[0], avgAttackerPos[1]);	// let's run towards it.
			this.tactics = new Tactics(gameState,HeadQuarters, this.idList,this.threatList,true);
			this.state = "attacking_threat";
		}
	}else if (this.state === "attacking_threat"){
		this.tactics.eventMetadataCleanup(events,HeadQuarters);
		var removeList = this.tactics.removeTheirDeads(HeadQuarters);
		this.tactics.removeMyDeads(HeadQuarters);
		for (var i in removeList){
			this.threatList.splice(this.threatList.indexOf(removeList[i]),1);
		}
		if (this.threatList.length <= 0)
		{
			this.tactics.disband(HeadQuarters,events);
			this.tactics = undefined;
			this.state = "walking";
			units.move(this.path[0][0], this.path[0][1]);
		}else
		{
			this.tactics.reassignAttacks(HeadQuarters);
		}
	}*/
	}
	if (this.state === "walking"){
		if (SquareVectorDistance(this.unitCollection.getCentrePosition(), this.path[0]) < 400){
			this.path.shift();
			if (this.path.length > 0){
				this.unitCollection.move(this.path[0][0], this.path[0][1]);
			} else {
				debug ("Attack Plan " +this.type +" " +this.name +" has arrived to destination.");
				// we must assume we've arrived at the end of the trail.
				this.state = "arrived";
			}
		}
		if (this.position == this.lastPosition)
			this.unitCollection.move(this.path[0][0], this.path[0][1]);
	}
	// todo: re-implement raiding
	/*
	if (this.state === "arrived"){
		// let's proceed on with whatever happens now.
		// There's a ton of TODOs on this part.
		if (this.onArrivalReaction == "huntVillagers")
		{
			// let's get any villager and target them with a tactics manager
			var enemyCitizens = gameState.entities.filter(function(ent) {
					return (gameState.isEntityEnemy(ent) && ent.hasClass("Support") && ent.owner() !== 0  && ent.position());
				});
			var targetList = [];
			enemyCitizens.forEach( function (enemy) {
				if (inRange(enemy.position(), units.getCentrePosition(), 2500) && targetList.indexOf(enemy.id()) === -1)
					targetList.push(enemy.id());
			});
			if (targetList.length > 0)
			{
				this.tactics = new Tactics(gameState,HeadQuarters, this.idList,targetList);
				this.state = "huntVillagers";
			} else {
				this.state = "";
			}
		}
	}
	if (this.state === "huntVillagers")
	{
		this.tactics.eventMetadataCleanup(events,HeadQuarters);
		this.tactics.removeTheirDeads(HeadQuarters);
		this.tactics.removeMyDeads(HeadQuarters);
		if (this.tactics.isBattleOver())
		{
			this.tactics.disband(HeadQuarters,events);
			this.tactics = undefined;
			this.state = "";
			return 0;	// assume over
		} else
			this.tactics.reassignAttacks(HeadQuarters);
	}*/
	this.lastPosition = this.position;
	Engine.ProfileStop();
	return this.unitCollection.length;
};
CityAttack.prototype.totalCountUnits = function(gameState){
	var totalcount = 0;
	for (i in this.idList)
	{
		totalcount++;
	}
	return totalcount;
};
// reset any units
CityAttack.prototype.Abort = function(gameState){	
	this.unitCollection.forEach(function(ent) {
		ent.setMetadata("role",undefined);
		ent.setMetadata("subrole",undefined);
		ent.setMetadata("plan",undefined);
	});
	for (unitCat in this.unitStat)
		delete this.unit[unitCat];
	delete this.unitCollection;
	gameState.ai.queueManager.removeQueue("plan_" + this.name);
};
