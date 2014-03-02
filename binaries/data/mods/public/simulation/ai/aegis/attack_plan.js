var AEGIS = function(m)
{

/* This is an attack plan (despite the name, it's a relic of older times).
 * It deals with everything in an attack, from picking a target to picking a path to it
 * To making sure units rae built, and pushing elements to the queue manager otherwise
 * It also handles the actual attack, though much work is needed on that.
 * These should be extremely flexible with only minimal work.
 * There is a basic support for naval expeditions here. 
 */

m.CityAttack = function CityAttack(gameState, HQ, Config, uniqueID, targetEnemy, type , targetFinder) {
	
	this.Config = Config;
	//This is the list of IDs of the units in the plan
	this.idList=[];
	
	this.state = "unexecuted";
	this.targetPlayer = targetEnemy;
	if (this.targetPlayer === -1 || this.targetPlayer === undefined) {
		// let's find our prefered target, basically counting our enemies units.
		// TODO: improve this.
		var enemyCount = {};
		for (var i = 1; i <=8; i++)
			enemyCount[i] = 0;
		gameState.getEntities().forEach(function(ent) { if (gameState.isEntityEnemy(ent) && ent.owner() !== 0) { enemyCount[ent.owner()]++; } });
		var max = 0;
		for (var i in enemyCount)
			if (enemyCount[i] > max && +i !== PlayerID)
			{
				this.targetPlayer = +i;
				max = enemyCount[i];
			}
	}
	if (this.targetPlayer === undefined || this.targetPlayer === -1)
	{
		this.failed = true;
		return false;
	}
	
	var CCs = gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre"));
	if (CCs.length === 0)
	{
		this.failed = true;
		return false;
	}

	m.debug ("Target (" + PlayerID +") = " +this.targetPlayer);
	this.targetFinder = targetFinder || this.defaultTargetFinder;
	this.type = type || "normal";
	this.name = uniqueID;
	this.healthRecord = [];
		
	this.timeOfPlanStart = gameState.getTimeElapsed();	// we get the time at which we decided to start the attack
	
	this.maxPreparationTime = 210*1000;
	
	this.pausingStart = 0;
	this.totalPausingTime = 0;
	this.paused = false;
	
	this.onArrivalReaction = "proceedOnTargets";

	// priority of the queues we'll create.
	var priority = 70;

	// priority is relative. If all are 0, the only relevant criteria is "currentsize/targetsize".
	// if not, this is a "bonus". The higher the priority, the faster this unit will get built.
	// Should really be clamped to [0.1-1.5] (assuming 1 is default/the norm)
	// Eg: if all are priority 1, and the siege is 0.5, the siege units will get built
	// only once every other category is at least 50% of its target size.
	// note: siege build order is currently added by the military manager if a fortress is there.
	this.unitStat = {};
	this.unitStat["RangedInfantry"] = { "priority" : 1, "minSize" : 6, "targetSize" : 18, "batchSize" : 3, "classes" : ["Infantry","Ranged"], "interests" : [ ["canGather", 1], ["strength",1.6], ["cost",1.5], ["costsResource", 0.3, "stone"], ["costsResource", 0.3, "metal"] ], "templates" : [] };
	this.unitStat["MeleeInfantry"]  = { "priority" : 1, "minSize" : 6, "targetSize" : 18, "batchSize" : 3, "classes" : ["Infantry","Melee"],  "interests" : [ ["canGather", 1], ["strength",1.6], ["cost",1.5], ["costsResource", 0.3, "stone"], ["costsResource", 0.3, "metal"] ], "templates" : [] };
	
	var ats = Math.random() * 15000 - 15000;	// attack time shuffle: move the exact attack time around a bit.

	// in this case we want to have the attack ready by the 14th minute. Countdown. Minimum 2 minutes.
	if (this.Config.difficulty >= 1)
		this.maxPreparationTime = (800000+ats) - gameState.getTimeElapsed() < 120000 ? 120000 : 800000 + ats - gameState.getTimeElapsed();

	if (type === "Rush") {
		// we have 3 minutes to train infantry.
		delete this.unitStat["RangedInfantry"];
		delete this.unitStat["MeleeInfantry"];
		this.unitStat["Infantry"] = { "priority" : 1, "minSize" : 10, "targetSize" : 30, "batchSize" : 2, "classes" : ["Infantry"], "interests" : [ ["strength",1], ["cost",1], ["costsResource", 0.5, "stone"], ["costsResource", 0.6, "metal"] ], "templates" : [] };
		this.maxPreparationTime = (540000+ats) - gameState.getTimeElapsed() < 120000 ? 120000 : 540000 + ats - gameState.getTimeElapsed();
		priority = 250;
	} else if (type === "superSized") {
		// our first attack has started worst case at the 14th minute, we want to attack another time by the 21th minute, so we rock 6.5 minutes
		this.maxPreparationTime = 480000;	// 8 minutes
		// basically we want a mix of citizen soldiers so our barracks have a purpose, and champion units.
		this.unitStat["RangedInfantry"]    = { "priority" : 0.7, "minSize" : 5, "targetSize" : 15, "batchSize" : 5, "classes" : ["Infantry","Ranged", "CitizenSoldier"], "interests" : [["strength",3], ["cost",1] ], "templates" : [] };
		this.unitStat["MeleeInfantry"]     = { "priority" : 0.7, "minSize" : 5, "targetSize" : 15, "batchSize" : 5, "classes" : ["Infantry","Melee", "CitizenSoldier" ], "interests" : [ ["strength",3], ["cost",1] ], "templates" : [] };
		this.unitStat["ChampRangedInfantry"] = { "priority" : 1, "minSize" : 5, "targetSize" : 25, "batchSize" : 5, "classes" : ["Infantry","Ranged", "Champion"], "interests" : [["strength",3], ["cost",1] ], "templates" : [] };
		this.unitStat["ChampMeleeInfantry"]  = { "priority" : 1, "minSize" : 5, "targetSize" : 20, "batchSize" : 5, "classes" : ["Infantry","Melee", "Champion" ], "interests" : [ ["strength",3], ["cost",1] ], "templates" : [] };
		this.unitStat["MeleeCavalry"]      = { "priority" : 0.7, "minSize" : 3, "targetSize" : 15, "batchSize" : 3, "classes" : ["Cavalry","Melee", "CitizenSoldier" ], "interests" : [ ["strength",2], ["cost",1] ], "templates" : [] };
		this.unitStat["RangedCavalry"]     = { "priority" : 0.7, "minSize" : 3, "targetSize" : 15, "batchSize" : 3, "classes" : ["Cavalry","Ranged", "CitizenSoldier"], "interests" : [ ["strength",2], ["cost",1] ], "templates" : [] };
		this.unitStat["ChampMeleeInfantry"]  = { "priority" : 1, "minSize" : 3, "targetSize" : 18, "batchSize" : 3, "classes" : ["Infantry","Melee", "Champion" ], "interests" : [ ["strength",3], ["cost",1] ], "templates" : [] };
		this.unitStat["ChampMeleeCavalry"]   = { "priority" : 1, "minSize" : 3, "targetSize" : 18, "batchSize" : 3, "classes" : ["Cavalry","Melee", "Champion" ], "interests" : [ ["strength",2], ["cost",1] ], "templates" : [] };

		priority = 90;
	}

	// TODO: there should probably be one queue per type of training building
	gameState.ai.queueManager.addQueue("plan_" + this.name, priority);
	this.queue = gameState.ai.queues["plan_" + this.name];
	gameState.ai.queueManager.addQueue("plan_" + this.name +"_champ", priority+1);
	this.queueChamp = gameState.ai.queues["plan_" + this.name +"_champ"];
	/*
	this.unitStat["Siege"]["filter"] = function (ent) {
		var strength = [ent.attackStrengths("Melee")["crush"],ent.attackStrengths("Ranged")["crush"]];
		return (strength[0] > 15 || strength[1] > 15);
	};*/

	var filter = API3.Filters.and(API3.Filters.byMetadata(PlayerID, "plan",this.name),API3.Filters.byOwner(PlayerID));
	this.unitCollection = gameState.getOwnUnits().filter(filter);
	this.unitCollection.registerUpdates();
	this.unitCollection.length;
	
	this.unit = {};
	
	// each array is [ratio, [associated classes], associated EntityColl, associated unitStat, name ]
	this.buildOrder = [];
	
	// defining the entity collections. Will look for units I own, that are part of this plan.
	// Also defining the buildOrders.
	for (var unitCat in this.unitStat) {
		var cat = unitCat;
		var Unit = this.unitStat[cat];

		filter = API3.Filters.and(API3.Filters.byClassesAnd(Unit["classes"]),API3.Filters.and(API3.Filters.byMetadata(PlayerID, "plan",this.name),API3.Filters.byOwner(PlayerID)));
		this.unit[cat] = gameState.getOwnUnits().filter(filter);
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


	var myFortresses = gameState.getOwnTrainingFacilities().filter(API3.Filters.byClass("GarrisonFortress"));
	if (myFortresses.length !== 0)
	{
		// make this our rallypoint
		for (var i in myFortresses._entities)
		{
			if (myFortresses._entities[i].position())
			{
				this.rallyPoint = myFortresses._entities[i].position();
				break;
			}
		}
	} else {
		
		if(gameState.ai.pathsToMe.length > 1)
			var position = [(gameState.ai.pathsToMe[0][0]+gameState.ai.pathsToMe[1][0])/2.0,(gameState.ai.pathsToMe[0][1]+gameState.ai.pathsToMe[1][1])/2.0];
		else if (gameState.ai.pathsToMe.length !== 0)
			var position = [gameState.ai.pathsToMe[0][0],gameState.ai.pathsToMe[0][1]];
		else
			var position = [-1,-1];
		
		if (gameState.ai.accessibility.getAccessValue(position) !== gameState.ai.myIndex)
			var position = [-1,-1];
		
		var nearestCCArray = CCs.filterNearest(position, 1).toEntityArray();
		var CCpos = nearestCCArray[0].position();
		this.rallyPoint = [0,0];
		if (position[0] !== -1) {
			this.rallyPoint[0] = position[0];
			this.rallyPoint[1] = position[1];
		} else {
			this.rallyPoint[0] = CCpos[0];
			this.rallyPoint[1] = CCpos[1];
		}
		if (type == 'harass_raid')
		{
			this.rallyPoint[0] = (position[0]*3.9 + 0.1 * CCpos[0]) / 4.0;
			this.rallyPoint[1] = (position[1]*3.9 + 0.1 * CCpos[1]) / 4.0;
		}
	}
	
	// some variables for during the attack
	this.position5TurnsAgo = [0,0];
	this.lastPosition = [0,0];
	this.position = [0,0];

	this.threatList = [];	// sounds so FBI
	this.tactics = undefined;
	
	this.assignUnits(gameState);
	
	//m.debug ("Before");
	//Engine.DumpHeap();
	
	// get a good path to an estimated target.
	this.pathFinder = new API3.aStarPath(gameState,false,false, this.targetPlayer);
	//Engine.DumpImage("widthmap.png", this.pathFinder.widthMap, this.pathFinder.width,this.pathFinder.height,255);

	this.pathWidth = 6;	// prefer a path far from entities. This will avoid units getting stuck in trees and also results in less straight paths.
	this.pathSampling = 2;
	this.onBoat = false;	// tells us if our units are loaded on boats.
	this.needsShip = false;
	
	//m.debug ("after");
	//Engine.DumpHeap();
	return true;
};

m.CityAttack.prototype.getName = function(){
	return this.name;
};
m.CityAttack.prototype.getType = function(){
	return this.type;
};
// Returns true if the attack can be executed at the current time
// Basically his checks we have enough units.
// We run a count of our units.
m.CityAttack.prototype.canStart = function(gameState){	
	for (var unitCat in this.unitStat) {
		var Unit = this.unitStat[unitCat];
		if (this.unit[unitCat].length < Unit["minSize"])
		{
			m.debug(unitCat + " doesn't have enough units : " + this.unit[unitCat].length);
			return false;
		}
	}
	return true;

	// TODO: check if our target is valid and a few other stuffs (good moment to attack?)
};
m.CityAttack.prototype.isStarted = function(){
	if ((this.state !== "unexecuted"))
		m.debug ("Attack plan already started");
	return !(this.state == "unexecuted");
};

m.CityAttack.prototype.isPaused = function(){
	return this.paused;
};
m.CityAttack.prototype.setPaused = function(gameState, boolValue){
	if (!this.paused && boolValue === true) {
		this.pausingStart = gameState.getTimeElapsed();
		this.paused = true;
		m.debug ("Pausing attack plan " +this.name);
	} else if (this.paused && boolValue === false) {
		this.totalPausingTime += gameState.getTimeElapsed() - this.pausingStart;
		this.paused = false;
		m.debug ("Unpausing attack plan " +this.name);
	}
};
m.CityAttack.prototype.mustStart = function(gameState){
	if (this.isPaused() || this.path === undefined)
		return false;
	var MaxReachedEverywhere = true;
	for (var unitCat in this.unitStat) {
		var Unit = this.unitStat[unitCat];
		if (this.unit[unitCat].length < Unit["targetSize"]) {
			MaxReachedEverywhere = false;
		}
	}
	if (MaxReachedEverywhere || (gameState.getPopulationMax() - gameState.getPopulation() < 10 && this.canStart(gameState)))
		return true;
	return (this.maxPreparationTime + this.timeOfPlanStart + this.totalPausingTime < gameState.getTimeElapsed());
};

// Adds a build order. If resetQueue is true, this will reset the queue.
m.CityAttack.prototype.addBuildOrder = function(gameState, name, unitStats, resetQueue) {
	if (!this.isStarted())
	{
		m.debug ("Adding a build order for " + name);
		// no minsize as we don't want the plan to fail at the last minute though.
		this.unitStat[name] = unitStats;
		var Unit = this.unitStat[name];
		var filter = API3.Filters.and(API3.Filters.byClassesAnd(Unit["classes"]),API3.Filters.and(API3.Filters.byMetadata(PlayerID, "plan",this.name),API3.Filters.byOwner(PlayerID)));
		this.unit[name] = gameState.getOwnUnits().filter(filter);
		this.unit[name].registerUpdates();
		this.buildOrder.push([0, Unit["classes"], this.unit[name], Unit, name]);
		if (resetQueue)
		{
			this.queue.empty();
			this.queueChamp.empty();
		}
	}
};

// Three returns possible: 1 is "keep going", 0 is "failed plan", 2 is "start"
// 3 is a special case: no valid path returned. Right now I stop attacking alltogether.
m.CityAttack.prototype.updatePreparation = function(gameState, HQ,events) {
	var self = this;
	
	if (this.path == undefined || this.target == undefined || this.path === "toBeContinued") {
		// find our target
		if (this.target == undefined)
		{
			var targets = this.targetFinder(gameState, HQ);
			if (targets.length === 0)
				targets = this.defaultTargetFinder(gameState, HQ);
				
			if (targets.length !== 0) {
				m.debug ("Aiming for " + targets);
				// picking a target
				var maxDist = -1;
				var index = 0;
				for (var i in targets._entities)
				{
					// we're sure it has a position has TargetFinder already checks that.
					var dist = API3.SquareVectorDistance(targets._entities[i].position(), this.rallyPoint);
					if (dist < maxDist || maxDist === -1)
					{
						maxDist = dist;
						index = i;
					}
				}
				this.target = targets._entities[index];
				this.targetPos = this.target.position();
			}
		}
		// when we have a target, we path to it.
		// I'd like a good high width sampling first.
		// Thus I will not do everything at once.
		// It will probably carry over a few turns but that's no issue.
		if (this.path === undefined)
			this.path = this.pathFinder.getPath(this.rallyPoint,this.targetPos, this.pathSampling, this.pathWidth,175);//,gameState);
		else if (this.path === "toBeContinued")
			this.path = this.pathFinder.continuePath();//gameState);
		
		if (this.path === undefined) {
			if (this.pathWidth == 6)
			{
				this.pathWidth = 2;
				delete this.path;
			} else {
				delete this.pathFinder;
				return 3;	// no path.
			}
		} else if (this.path === "toBeContinued") {
			// carry on.
		} else if (this.path[1] === true && this.pathWidth == 2) {
			// okay so we need a ship.
			// Basically we'll add it as a new class to train compulsorily, and we'll recompute our path.
			if (!gameState.ai.HQ.waterMap)
			{
				m.debug ("This is actually a water map.");
				gameState.ai.HQ.waterMap = true;
				return 0;
			}
			m.debug ("We need a ship.");
			this.needsShip = true;
			this.pathWidth = 3;
			this.pathSampling = 3;
			this.path = this.path[0].reverse();
			delete this.pathFinder;
			// Change the rally point to something useful (should avoid rams getting stuck in houses in my territory, which is dumb.)
			for (var i = 0; i < this.path.length; ++i)
			{
				// my pathfinder returns arrays in arrays in arrays.
				var waypointPos = this.path[i][0];
				var territory = m.createTerritoryMap(gameState);
				if (territory.getOwner(waypointPos) !== PlayerID || this.path[i][1] === true)
				{
					// if we're suddenly out of our territory or this is the point where we change transportation method.
					if (i !== 0)
						this.rallyPoint = this.path[i-1][0];
					else
						this.rallyPoint = this.path[0][0];
					if (i >= 1)
						this.path.splice(0,i-1);
					break;
				}
			}
		} else if (this.path[1] === true && this.pathWidth == 6) {
			// retry with a smaller pathwidth:
			this.pathWidth = 2;
			delete this.path;
		} else {
			this.path = this.path[0].reverse();
			delete this.pathFinder;
			
			// Change the rally point to something useful (should avoid rams getting stuck in houses in my territory, which is dumb.)
			for (var i = 0; i < this.path.length; ++i)
			{
				// my pathfinder returns arrays in arrays in arrays.
				var waypointPos = this.path[i][0];
				var territory = m.createTerritoryMap(gameState);
				if (territory.getOwner(waypointPos) !== PlayerID || this.path[i][1] === true)
				{
					// if we're suddenly out of our territory or this is the point where we change transportation method.
					if (i !== 0)
					{
						this.rallyPoint = this.path[i-1][0];
					} else
						this.rallyPoint = this.path[0][0];
					if (i >= 1)
						this.path.splice(0,i-1);
					break;
				}
			}
		}
	}

	Engine.ProfileStart("Update Preparation");
	
	// special case: if we're reached max pop, and we can start the plan, start it.
	if ((gameState.getPopulationMax() - gameState.getPopulation() < 10) && this.canStart())
	{
		this.assignUnits(gameState);
		this.queue.empty();
		this.queueChamp.empty();
		if ( gameState.ai.playedTurn % 5 == 0)
			this.AllToRallyPoint(gameState, true);
	} else if (this.mustStart(gameState) && (gameState.countOwnQueuedEntitiesWithMetadata("plan", +this.name) > 0)) {
		// keep on while the units finish being trained, then we'll start
		this.assignUnits(gameState);
		
		this.queue.empty();
		this.queueChamp.empty();

		if (gameState.ai.playedTurn % 5 == 0) {
			this.AllToRallyPoint(gameState, true);
			// TODO: should use this time to let gatherers deposit resources.
		}
		Engine.ProfileStop();
		return 1;
	} else if (!this.mustStart(gameState)) {
		// We still have time left to recruit units and do stuffs.
		
		// let's sort by training advancement, ie 'current size / target size'
		// count the number of queued units too.
		// substract priority.
		this.buildOrder.sort(function (a,b) { //}) {
			var aQueued = gameState.countOwnQueuedEntitiesWithMetadata("special","Plan_"+self.name+"_"+a[4]);
			aQueued += self.queue.countQueuedUnitsWithMetadata("special","Plan_"+self.name+"_"+a[4]);
			aQueued += self.queueChamp.countQueuedUnitsWithMetadata("special","Plan_"+self.name+"_"+a[4]);
			a[0] = (a[2].length + aQueued)/a[3]["targetSize"];
							 
			var bQueued = gameState.countOwnQueuedEntitiesWithMetadata("special","Plan_"+self.name+"_"+b[4]);
			bQueued += self.queue.countQueuedUnitsWithMetadata("special","Plan_"+self.name+"_"+b[4]);
			bQueued += self.queueChamp.countQueuedUnitsWithMetadata("special","Plan_"+self.name+"_"+b[4]);
			b[0] = (b[2].length + bQueued)/b[3]["targetSize"];
							 
			a[0] -= a[3]["priority"];
			b[0] -= b[3]["priority"];
			return (a[0]) - (b[0]);
		});
				
		this.assignUnits(gameState);
		
		if (gameState.ai.playedTurn % 5 == 0) {
			this.AllToRallyPoint(gameState, false);
			this.unitCollection.setStance("standground");	// make sure units won't disperse out of control
		}
		
		Engine.ProfileStart("Creating units.");
		
		// gets the number in training of the same kind as the first one.
		var specialData = "Plan_"+this.name+"_"+this.buildOrder[0][4];
		var inTraining = gameState.countOwnQueuedEntitiesWithMetadata("special",specialData);
				
		var queued = this.queue.countQueuedUnitsWithMetadata("special",specialData) + this.queueChamp.countQueuedUnitsWithMetadata("special",specialData)
		
		if (queued + inTraining + this.buildOrder[0][2].length <= this.buildOrder[0][3]["targetSize"]) {
			// find the actual queue we want
			var queue = this.queue;
			if (this.buildOrder[0][3]["classes"].indexOf("Champion") !== -1)
				queue = this.queueChamp;
			
			if (this.buildOrder[0][0] < 1 && queue.length() <= 5) {
				var template = HQ.findBestTrainableUnit(gameState, this.buildOrder[0][1], this.buildOrder[0][3]["interests"] );
				//m.debug ("tried " + uneval(this.buildOrder[0][1]) +", and " + template);
				// HACK (TODO replace) : if we have no trainable template... Then we'll simply remove the buildOrder, effectively removing the unit from the plan.
				if (template === undefined) {
					// TODO: this is a complete hack.
					delete this.unitStat[this.buildOrder[0][4]];	// deleting the associated unitstat.
					this.buildOrder.splice(0,1);
				} else {
					var max = this.buildOrder[0][3]["batchSize"];
					// TODO: this should be plan dependant.
					if (gameState.getTimeElapsed() > 1800000)
						max *= 2;
					if (gameState.getTemplate(template).hasClass("CitizenSoldier"))
						queue.addItem( new m.TrainingPlan(gameState,template, { "role" : "worker", "plan" : this.name, "special" : specialData, "base" : 1 }, this.buildOrder[0][3]["batchSize"],max ) );
					else
						queue.addItem( new m.TrainingPlan(gameState,template, { "role" : "attack", "plan" : this.name, "special" : specialData, "base" : 1 }, this.buildOrder[0][3]["batchSize"],max ) );
				}
			}
		}
		/*
		if (!this.startedPathing && this.path === undefined) {
			
			// find our target
			var targets = this.targetFinder(gameState, HQ);
			if (targets.length === 0){
				targets = this.defaultTargetFinder(gameState, HQ);
			}
			if (targets.length) {
				this.targetPos = undefined;
				var count = 0;
				while (!this.targetPos){
					var rand = Math.floor((Math.random()*targets.length));
					var target = targets.toEntityArray()[rand];
					this.targetPos = target.position();
					count++;
					if (count > 1000){
						m.debug("No target with a valid position found");
						Engine.ProfileStop();
						Engine.ProfileStop();
						return false;
					}
				}
				this.startedPathing = true;
				// Start pathfinding using the optimized version, with a minimal sampling of 2
				this.pathFinder.getPath(this.rallyPoint,this.targetPos, false, 2, gameState);
			}
		} else if (this.startedPathing) {
			var path = this.pathFinder.continuePath(gameState);
			if (path !== "toBeContinued") {
				this.startedPathing = false;
				this.path = path;
				m.debug("Pathing ended");
			}
		}
		*/
		Engine.ProfileStop();
		Engine.ProfileStop();
		// can happen for now
		if (this.buildOrder.length === 0) {
			m.debug ("Ending plan: no build orders");
			return 0;	// will abort the plan, should return something else
		}
		return 1;
	}
	this.unitCollection.forEach(function (entity) { entity.setMetadata(PlayerID, "role","attack"); });

	Engine.ProfileStop();
	// if we're here, it means we must start (and have no units in training left).
	// if we can, do, else, abort.
	if (this.canStart(gameState))
		return 2;
	else
		return 0;
	return 0;
};
m.CityAttack.prototype.assignUnits = function(gameState){
	var self = this;

	// TODO: assign myself units that fit only, right now I'm getting anything.
	// Assign all no-roles that fit (after a plan aborts, for example).
	var NoRole = gameState.getOwnEntitiesByRole(undefined, false);
	if (this.type === "rush")
		NoRole = gameState.getOwnEntitiesByRole("worker", true);
	NoRole.forEach(function(ent) {
		if (ent.hasClass("Unit") && ent.attackTypes() !== undefined)
		{
			if (ent.hasClasses(["CitizenSoldier", "Infantry"]))
				ent.setMetadata(PlayerID, "role", "worker");
			else
				ent.setMetadata(PlayerID, "role", "attack");
			ent.setMetadata(PlayerID, "plan", self.name);
		}
	});

};
// this sends a unit by ID back to the "rally point"
m.CityAttack.prototype.ToRallyPoint = function(gameState,id)
{	
	// Move back to nearest rallypoint
	gameState.getEntityById(id).move(this.rallyPoint[0],this.rallyPoint[1]);
}
// this sends all units back to the "rally point" by entity collections.
// It doesn't disturb ones that could be currently defending, even if the plan is not (yet) paused.
m.CityAttack.prototype.AllToRallyPoint = function(gameState, evenWorkers) {
	var self = this;
	if (evenWorkers) {
		for (var unitCat in this.unit) {
			this.unit[unitCat].forEach(function (ent) {
				if (ent.getMetadata(PlayerID, "role") != "defence")
				{
					ent.setMetadata(PlayerID,"role", "attack");
					ent.move(self.rallyPoint[0],self.rallyPoint[1]);
				}
			});
		}
	} else {
		for (var unitCat in this.unit) {
			this.unit[unitCat].forEach(function (ent) {
				if (ent.getMetadata(PlayerID, "role") != "worker" && ent.getMetadata(PlayerID, "role") != "defence")
					ent.move(self.rallyPoint[0],self.rallyPoint[1]);
			});
		}
	}
}

// Default target finder aims for conquest critical targets
m.CityAttack.prototype.defaultTargetFinder = function(gameState, HQ){
	var targets = undefined;
	
	targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("CivCentre"));
	if (targets.length == 0) {
		targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("ConquestCritical"));
	}
	// If there's nothing, attack anything else that's less critical
	if (targets.length == 0) {
		targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("Town"));
	}
	if (targets.length == 0) {
		targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("Village"));
	}
	// no buildings, attack anything conquest critical, even units (it's assuming it won't move).
	if (targets.length == 0) {
		targets = gameState.getEnemyEntities(this.targetPlayer).filter(API3.Filters.byClass("ConquestCritical"));
	}
	return targets;
};

// tupdate
m.CityAttack.prototype.raidingTargetFinder = function(gameState, HQ, Target){
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
		return this.defaultTargetFinder(gameState, HQ);
	}
};

// Executes the attack plan, after this is executed the update function will be run every turn
// If we're here, it's because we have in our IDlist enough units.
// now the IDlist units are treated turn by turn
m.CityAttack.prototype.StartAttack = function(gameState, HQ){
	
	// check we have a target and a path.
	if (this.targetPos && this.path !== undefined) {
		// erase our queue. This will stop any leftover unit from being trained.
		gameState.ai.queueManager.removeQueue("plan_" + this.name);
		gameState.ai.queueManager.removeQueue("plan_" + this.name + "_champ");
		
		var curPos = this.unitCollection.getCentrePosition();
		
		this.unitCollection.forEach(function(ent) { ent.setMetadata(PlayerID, "subrole", "walking"); ent.setMetadata(PlayerID, "role", "attack") ;});
		// optimize our collection now.
		this.unitCollection.allowQuickIter();
		
		this.unitCollection.move(this.path[0][0][0], this.path[0][0][1]);
		this.unitCollection.setStance("aggressive");
		this.unitCollection.filter(API3.Filters.byClass("Siege")).setStance("defensive");

		this.state = "walking";
	} else {
		gameState.ai.gameFinished = true;
		m.debug ("I do not have any target. So I'll just assume I won the game.");
		return false;
	}
	return true;
};

// Runs every turn after the attack is executed
m.CityAttack.prototype.update = function(gameState, HQ, events){
	var self = this;
		
	Engine.ProfileStart("Update Attack");

	// we're marching towards the target
	// Check for attacked units in our band.
	var bool_attacked = false;
	// raids don't care about attacks much
	
	if (this.unitCollection.length === 0) {
		Engine.ProfileStop();
		return 0;
	}
	
	this.position = this.unitCollection.getCentrePosition();
	
	var IDs = this.unitCollection.toIdArray();
	
	// this actually doesn't do anything right now.
	if (this.state === "walking") {
		var attackedNB = 0;
		
		var toProcess = {};
		var armyToProcess = {};
		// Let's check if any of our unit has been attacked. In case yes, we'll determine if we're simply off against an enemy army, a lone unit/builing
		// or if we reached the enemy base. Different plans may react differently.
		
		var attackedEvents = events["Attacked"];
		for (var key in attackedEvents) {
			var e = attackedEvents[key];
			if (IDs.indexOf(e.target) !== -1) {
				var attacker = gameState.getEntityById(e.attacker);
				var ourUnit = gameState.getEntityById(e.target);
				
				if (attacker && attacker.position() && attacker.hasClass("Unit") && attacker.owner() != 0 && attacker.owner() != PlayerID) {
					attackedNB++;
					//if (HQ.enemyWatchers[attacker.owner()]) {
					//toProcess[attacker.id()] = attacker;
					//var armyID = HQ.enemyWatchers[attacker.owner()].getArmyFromMember(attacker.id());
					//armyToProcess[armyID[0]] = armyID[1];
					//}
				}
				// if we're being attacked by a building, flee.
				if (attacker && ourUnit && attacker.hasClass("Structure")) {
					ourUnit.flee(attacker);
				}
				
			}
		}
		var territoryMap = m.createTerritoryMap(gameState);
		if ((territoryMap.getOwner(this.position) === this.targetPlayer && attackedNB > 1) || attackedNB > 4) {
			m.debug ("Attack Plan " +this.type +" " +this.name +" has arrived to destination.");
			// we must assume we've arrived at the end of the trail.
			this.state = "arrived";
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
							for (var j in enemySoldiers)
							{
								var enemy = enemySoldiers[j];
								if (enemy.position() === undefined)	// likely garrisoned
									continue;
								if (m.inRange(enemy.position(), attacker.position(), 1000) && this.threatList.indexOf(enemy.id()) === -1)
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
		
		this.position = this.unitCollection.getCentrePosition();

		// probably not too good.
		if (!this.position) {
			Engine.ProfileStop();
			return undefined;	// should spawn an error.
		}

		// basically haven't moved an inch: very likely stuck)
		if (API3.SquareVectorDistance(this.position, this.position5TurnsAgo) < 10 && this.path.length > 0 && gameState.ai.playedTurn % 5 === 0) {
			// check for stuck siege units
			var sieges = this.unitCollection.filter(API3.Filters.byClass("Siege"));
			var farthest = 0;
			var farthestEnt = -1;
			sieges.forEach (function (ent) {
				if (API3.SquareVectorDistance(ent.position(),self.position) > farthest)
				{
					farthest = API3.SquareVectorDistance(ent.position(),self.position);
					farthestEnt = ent;
				}
			});
			if (farthestEnt !== -1)
				farthestEnt.destroy();
		}
		if (gameState.ai.playedTurn % 5 === 0)
			this.position5TurnsAgo = this.position;
		
		if (this.lastPosition && API3.SquareVectorDistance(this.position, this.lastPosition) < 20 && this.path.length > 0) {
			this.unitCollection.moveIndiv(this.path[0][0][0], this.path[0][0][1]);
			// We're stuck, presumably. Check if there are no walls just close to us. If so, we're arrived, and we're gonna tear down some serious stone.
			var walls = gameState.getEnemyEntities().filter(API3.Filters.and(API3.Filters.byOwner(this.targetPlayer), API3.Filters.byClass("StoneWall")));
			var nexttoWalls = false;
			walls.forEach( function (ent) {
				if (!nexttoWalls && API3.SquareVectorDistance(self.position, ent.position()) < 800)
					nexttoWalls = true;
			});
			// there are walls but we can attack
			if (nexttoWalls && this.unitCollection.filter(API3.Filters.byCanAttack("StoneWall")).length !== 0)
			{
				m.debug ("Attack Plan " +this.type +" " +this.name +" has met walls and is not happy.");
				this.state = "arrived";
			} else if (nexttoWalls) {
				// abort plan.
				m.debug ("Attack Plan " +this.type +" " +this.name +" has met walls and gives up.");
				Engine.ProfileStop();
				return 0;
			}
		}

		// check if our land units are close enough from the next waypoint.
		if (API3.SquareVectorDistance(this.position, this.targetPos) < 9000 ||
			API3.SquareVectorDistance(this.position, this.path[0][0]) < 650) {
			if (this.unitCollection.filter(API3.Filters.byClass("Siege")).length !== 0
				&& API3.SquareVectorDistance(this.position, this.targetPos) >= 9000
				&& API3.SquareVectorDistance(this.unitCollection.filter(API3.Filters.byClass("Siege")).getCentrePosition(), this.path[0][0]) >= 650)
			{
			} else {
				// okay so here basically two cases. First case is "we've arrived"
				// Second case is "either we need a boat, or we need to unload"
				if (this.path[0][1] !== true)
				{
					this.path.shift();
					if (this.path.length > 0){
						this.unitCollection.move(this.path[0][0][0], this.path[0][0][1]);
					} else {
						m.debug ("Attack Plan " +this.type +" " +this.name +" has arrived to destination.");
						// we must assume we've arrived at the end of the trail.
						this.state = "arrived";
					}
				} else
				{
					// TODO: make this require an escort later on.
					this.path.shift();
					if (this.path.length === 0) {
						m.debug ("Attack Plan " +this.type +" " +this.name +" has arrived to destination.");
						// we must assume we've arrived at the end of the trail.
						this.state = "arrived";
					} else {
						/*
						var plan = new m.TransportPlan(gameState, this.unitCollection.toIdArray(), this.path[0][0], false);
						this.tpPlanID = plan.ID;
						HQ.navalManager.transportPlans.push(plan);
						m.debug ("Transporting over sea");
						this.state = "transporting";
					*/
						// TODO: fix this above
						//right now we'll abort.
						Engine.ProfileStop();
						return 0;
					}
				}
			}
		}
	} else if (this.state === "transporting") {
		// check that we haven't finished transporting, ie the plan
		if (!HQ.navalManager.checkActivePlan(this.tpPlanID))
		{
			this.state = "walking";
		}
	}


	// todo: re-implement raiding
	if (this.state === "arrived"){
		// let's proceed on with whatever happens now.
		// There's a ton of TODOs on this part.
		if (this.onArrivalReaction == "proceedOnTargets") {
			this.state = "";
			this.unitCollection.forEach( function (ent) { //}) {
				ent.stopMoving();
				ent.setMetadata(PlayerID, "subrole", "attacking");
			});
		} else if (this.onArrivalReaction == "huntVillagers") {
			// let's get any villager and target them with a tactics manager
			var enemyCitizens = gameState.entities.filter(function(ent) {
					return (gameState.isEntityEnemy(ent) && ent.hasClass("Support") && ent.owner() !== 0  && ent.position());
				});
			var targetList = [];
			enemyCitizens.forEach( function (enemy) {
				if (m.inRange(enemy.position(), units.getCentrePosition(), 2500) && targetList.indexOf(enemy.id()) === -1)
					targetList.push(enemy.id());
			});
			if (targetList.length > 0)
			{
				this.tactics = new Tactics(gameState,HeadQuarters, this.idList,targetList);
				this.state = "huntVillagers";
				var arrivedthisTurn = true;
			} else {
				this.state = "";
				var arrivedthisTurn = true;
			}
		}
	}
	
	// basic state of attacking.
	if (this.state === "") {

		// events watch: if siege units are attacked, we'll send some units to deal with enemies.
		var attackedEvents = events["Attacked"];
		for (var key in attackedEvents) {
			var e = attackedEvents[key];
			if (IDs.indexOf(e.target) !== -1) {
				var attacker = gameState.getEntityById(e.attacker);
				var ourUnit = gameState.getEntityById(e.target);
				
				if (!attacker || !attacker.position() || !attacker.hasClass("Unit") || attacker.owner() === 0 || attacker.owner() === PlayerID)
					continue;

				if (!ourUnit.hasClass("Siege"))
					continue;
				var collec = this.unitCollection.filter(API3.Filters.not(API3.Filters.byClass("Siege"))).filterNearest(ourUnit.position(), 5);
				if (collec.length === 0)
					continue;
				collec.attack(attacker.id())
			}
		}
		
		var enemyUnits = gameState.getEnemyUnits(this.targetPlayer);
		var enemyStructures = gameState.getEnemyStructures(this.targetPlayer);

		if (this.unitCollUpdateArray === undefined || this.unitCollUpdateArray.length === 0)
		{
			this.unitCollUpdateArray = this.unitCollection.toIdArray();
		} else {
			// some stuffs for locality and speed
			var territoryMap = m.createTerritoryMap(gameState);
			var timeElapsed = gameState.getTimeElapsed();
			
			// Let's check a few units each time we update. Currently 10
			var lgth = Math.min(this.unitCollUpdateArray.length,10);
			for (var check = 0; check < lgth; check++)
			{
				var ent = gameState.getEntityById(this.unitCollUpdateArray[0]);
				if (!ent)
					continue;

				// if the unit is in my territory, make it move towards the target.
				if (territoryMap.point(ent.position()) - 64 === PlayerID) {
					ent.move(this.targetPos[0],this.targetPos[1]);
					continue;
				}

				var orderData = ent.unitAIOrderData();
				if (orderData.length !== 0)
					orderData = orderData[0];
				else
					orderData = undefined;
				
				// update it.
				var needsUpdate = false;
				if (ent.isIdle())
					needsUpdate = true;
				else if (ent.hasClass("Siege") && (!orderData || !orderData["target"] || !gameState.getEntityById(orderData["target"]) || !gameState.getEntityById(orderData["target"]).hasClass("ConquestCritical")) )
					needsUpdate = true;
				else if (!ent.hasClass("Siege") && orderData && orderData["target"] && gameState.getEntityById(orderData["target"]) && gameState.getEntityById(orderData["target"]).hasClass("Structure"))
					needsUpdate = true; // try to make it attack a unit instead

				// don't update too soon.
				if (timeElapsed - ent.getMetadata(PlayerID, "lastAttackPlanUpdateTime") < 10000)
					continue;
				
				if (needsUpdate === false && !arrivedthisTurn)
					continue;
			
				ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", timeElapsed);

				// let's filter targets further based on this unit.
				var mStruct = enemyStructures.filter(function (enemy) { //}){
					if (!enemy.position() || (enemy.hasClass("StoneWall") && ent.canAttackClass("StoneWall"))) {
						return false;
					}
					if (API3.SquareVectorDistance(enemy.position(),ent.position()) > 3000) {
						return false;
					}
					return true;
				});
				var mUnit = enemyUnits.filter(function (enemy) {
					if (!enemy.position())
						return false;
					if (API3.SquareVectorDistance(enemy.position(),ent.position()) > 10000)
						return false;
					return true;
				});
				// Checking for gates if we're a siege unit.
				var isGate = false;
				mUnit = mUnit.toEntityArray();
				mStruct = mStruct.toEntityArray();
				if (ent.hasClass("Siege")) {
					mStruct.sort(function (structa,structb) {
						var vala = structa.costSum();
						if (structa.hasClass("Gates") && ent.canAttackClass("StoneWall")) { // we hate gates
							isGate = true;
							vala += 10000;
						} else if (structa.hasClass("ConquestCritical"))
							vala += 200;
						var valb = structb.costSum();
						if (structb.hasClass("Gates") && ent.canAttackClass("StoneWall")) { // we hate gates
							isGate = true;
							valb += 10000;
						} else if (structb.hasClass("ConquestCritical"))
							valb += 200;
						//warn ("Structure " +structa.genericName() + " is worth " +vala);
						//warn ("Structure " +structb.genericName() + " is worth " +valb);
						return (valb - vala);
					});
					// TODO: handle ballistas here
					if (mStruct.length !== 0) {
						if (isGate)
							ent.attack(mStruct[0].id());
						else
						{
							var rand = Math.floor(Math.random() * mStruct.length*0.2);
							ent.attack(mStruct[+rand].id());
							//m.debug ("Siege units attacking a structure from " +mStruct[+rand].owner() + " , " +mStruct[+rand].templateName());
						}
					} else if (API3.SquareVectorDistance(self.targetPos, ent.position()) > 900 ) {
						//m.debug ("Siege units moving to " + uneval(self.targetPos));
						ent.move(self.targetPos[0],self.targetPos[1]);
					}
				} else {
					if (mUnit.length !== 0) {
						mUnit.sort(function (unitA,unitB) {
							var vala = unitA.hasClass("Support") ? 50 : 0;
							if (ent.countersClasses(unitA.classes()))
								vala += 100;
							var valb = unitB.hasClass("Support") ? 50 : 0;
							if (ent.countersClasses(unitB.classes()))
								valb += 100;
							return valb - vala;
						});
						var rand = Math.floor(Math.random() * mUnit.length*0.1);
						ent.attack(mUnit[(+rand)].id());
						//m.debug ("Units attacking a unit from " +mUnit[+rand].owner() + " , " +mUnit[+rand].templateName());
					} else if (API3.SquareVectorDistance(self.targetPos, ent.position()) > 900 ){
						//m.debug ("Units moving to " + uneval(self.targetPos));
						ent.move(self.targetPos[0],self.targetPos[1]);
					} else if (mStruct.length !== 0) {
						mStruct.sort(function (structa,structb) { //}){
							var vala = structa.costSum();
							if (structa.hasClass("Gates") && ent.canAttackClass("StoneWall"))	// we hate gates
							{
								isGate = true;
								vala += 10000;
							} else if (structa.hasClass("ConquestCritical"))
								vala += 100;
							var valb = structb.costSum();
							if (structb.hasClass("Gates") && ent.canAttackClass("StoneWall"))	// we hate gates
							{
								isGate = true;
								valb += 10000;
							} else if (structb.hasClass("ConquestCritical"))
								valb += 100;
							return (valb - vala);
						});
						if (isGate)
							ent.attack(mStruct[0].id());
						else
						{
							var rand = Math.floor(Math.random() * mStruct.length*0.1);
							ent.attack(mStruct[+rand].id());
							//m.debug ("Units attacking a structure from " +mStruct[+rand].owner() + " , " +mStruct[+rand].templateName());
						}
					}
				}
			}
			this.unitCollUpdateArray.splice(0,10);
		}
		// updating targets.
		if (!gameState.getEntityById(this.target.id()))
		{			
			var targets = this.targetFinder(gameState, HQ);
			if (targets.length === 0){
				targets = this.defaultTargetFinder(gameState, HQ);
			}
			if (targets.length) {
				m.debug ("Seems like our target has been destroyed. Switching.");
				m.debug ("Aiming for " + targets);
				// picking a target
				this.targetPos = undefined;
				var count = 0;
				while (!this.targetPos){
					var rand = Math.floor((Math.random()*targets.length));
					this.target = targets.toEntityArray()[rand];
					this.targetPos = this.target.position();
					count++;
					if (count > 1000){
						m.debug("No target with a valid position found");
						Engine.ProfileStop();
						return false;
					}
				}
			}
		}
		
		// regularly update the target position in case it's a unit.
		if (this.target.hasClass("Unit"))
			this.targetPos = this.target.position();
	}
	/*
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
m.CityAttack.prototype.totalCountUnits = function(gameState){
	var totalcount = 0;
	for (var i in this.idList)
	{
		totalcount++;
	}
	return totalcount;
};
// reset any units
m.CityAttack.prototype.Abort = function(gameState){
	// Do not use QuickIter with forEach when forEach removes elements
	this.unitCollection.preventQuickIter();
	this.unitCollection.forEach(function(ent) {
		ent.setMetadata(PlayerID, "role",undefined);
		ent.setMetadata(PlayerID, "subrole",undefined);
		ent.setMetadata(PlayerID, "plan",undefined);
	});
	for (var unitCat in this.unitStat) {
		delete this.unitStat[unitCat];
		delete this.unit[unitCat];
	}
	delete this.unitCollection;
	gameState.ai.queueManager.removeQueue("plan_" + this.name);
	gameState.ai.queueManager.removeQueue("plan_" + this.name + "_champ");
};

return m;
}(AEGIS);
