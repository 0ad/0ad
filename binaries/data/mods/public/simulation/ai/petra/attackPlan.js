var PETRA = function(m)
{

/* This is an attack plan (despite the name, it's a relic of older times).
 * It deals with everything in an attack, from picking a target to picking a path to it
 * To making sure units are built, and pushing elements to the queue manager otherwise
 * It also handles the actual attack, though much work is needed on that.
 * These should be extremely flexible with only minimal work.
 * There is a basic support for naval expeditions here.
 */

m.AttackPlan = function(gameState, Config, uniqueID, type, data)
{
	this.Config = Config;
	this.name = uniqueID;
	this.type = type || "Attack";	
	this.state = "unexecuted";

	if (data && data.target)
	{
		this.target = data.target;
		this.targetPos = this.target.position();
		this.targetPlayer = this.target.owner();
	}
	else
	{
		this.target = undefined;
		this.targetPos = undefined;
		this.targetPlayer = this.getEnemyPlayer(gameState);
	}

	if (this.targetPlayer === undefined)
	{
		this.failed = true;
		return false;
	}
	
	// get a starting rallyPoint ... will be improved later
	var rallyPoint = undefined;
	for each (var base in gameState.ai.HQ.baseManagers)
	{
		if (!base.anchor || !base.anchor.position())
			continue;
		rallyPoint = base.anchor.position();
		break;
	}
	if (!rallyPoint)	// no base ?  take the position of any of our entities
	{
		gameState.getOwnEntities().forEach(function (ent) {
			if (rallyPoint || !ent.position())
				return;
			rallyPoint = ent.position();
		});
		if (!rallyPoint)
		{
			this.failed = true;
			return false;
		}
	}
	this.rallyPoint = rallyPoint;

	this.overseas = undefined;
	this.paused = false;
	this.maxCompletingTurn = 0;	

	// priority of the queues we'll create.
	var priority = 70;

	// priority is relative. If all are 0, the only relevant criteria is "currentsize/targetsize".
	// if not, this is a "bonus". The higher the priority, the faster this unit will get built.
	// Should really be clamped to [0.1-1.5] (assuming 1 is default/the norm)
	// Eg: if all are priority 1, and the siege is 0.5, the siege units will get built
	// only once every other category is at least 50% of its target size.
	// note: siege build order is currently added by the military manager if a fortress is there.
	this.unitStat = {};

	// neededShips is the minimal number of ships which should be available for transport
	if (type === "Rush")
	{
		priority = 250;
		this.unitStat["Infantry"] = { "priority": 1, "minSize": 10, "targetSize": 26, "batchSize": 2, "classes": ["Infantry"],
			"interests": [ ["strength",1], ["cost",1], ["costsResource", 0.5, "stone"], ["costsResource", 0.6, "metal"] ] };
		if (data && data.targetSize)
			this.unitStat["Infantry"]["targetSize"] = data.targetSize;
		this.neededShips = 1;
	}
	else if (type === "Raid")
	{
		priority = 150;
		this.unitStat["Cavalry"] = { "priority": 1, "minSize": 3, "targetSize": 4, "batchSize": 2, "classes": ["Cavalry", "CitizenSoldier"],
			"interests": [ ["strength",1], ["cost",1] ] };
		this.neededShips = 1;
	}
	else if (type === "HugeAttack")
	{
		priority = 90;
		// basically we want a mix of citizen soldiers so our barracks have a purpose, and champion units.
		this.unitStat["RangedInfantry"]    = { "priority": 0.7, "minSize": 5, "targetSize": 15, "batchSize": 5, "classes": ["Infantry","Ranged", "CitizenSoldier"],
			"interests": [["strength",3], ["cost",1] ] };
		this.unitStat["MeleeInfantry"]     = { "priority": 0.7, "minSize": 5, "targetSize": 15, "batchSize": 5, "classes": ["Infantry","Melee", "CitizenSoldier" ],
			"interests": [ ["strength",3], ["cost",1] ] };
		this.unitStat["ChampRangedInfantry"] = { "priority": 1, "minSize": 5, "targetSize": 25, "batchSize": 5, "classes": ["Infantry","Ranged", "Champion"],
			"interests": [["strength",3], ["cost",1] ] };
		this.unitStat["ChampMeleeInfantry"]  = { "priority": 1, "minSize": 5, "targetSize": 20, "batchSize": 5, "classes": ["Infantry","Melee", "Champion" ],
			"interests": [ ["strength",3], ["cost",1] ] };
		this.unitStat["MeleeCavalry"]      = { "priority": 0.7, "minSize": 3, "targetSize": 15, "batchSize": 3, "classes": ["Cavalry","Melee", "CitizenSoldier" ],
			"interests": [ ["strength",2], ["cost",1] ] };
		this.unitStat["RangedCavalry"]     = { "priority": 0.7, "minSize": 3, "targetSize": 15, "batchSize": 3, "classes": ["Cavalry","Ranged", "CitizenSoldier"],
			"interests": [ ["strength",2], ["cost",1] ] };
		this.unitStat["ChampMeleeInfantry"]  = { "priority": 1, "minSize": 3, "targetSize": 18, "batchSize": 3, "classes": ["Infantry","Melee", "Champion" ],
			"interests": [ ["strength",3], ["cost",1] ] };
		this.unitStat["ChampMeleeCavalry"]   = { "priority": 1, "minSize": 3, "targetSize": 18, "batchSize": 3, "classes": ["Cavalry","Melee", "Champion" ],
			"interests": [ ["strength",2], ["cost",1] ] };
		this.neededShips = 5;
		// decrease a bit the targetSize according to max population
		if (gameState.getPopulationMax() < 300)
		{
			var reduc = Math.sqrt(gameState.getPopulationMax() / 300);
			for (var unitCat in this.unitStat)
				this.unitStat[unitCat]["targetSize"] = Math.floor(reduc * this.unitStat[unitCat]["targetSize"]);
		}
	}
	else
	{
		priority = 70;
		this.unitStat["RangedInfantry"] = { "priority": 1, "minSize": 6, "targetSize": 18, "batchSize": 3, "classes": ["Infantry","Ranged"],
			"interests": [ ["canGather", 1], ["strength",1.6], ["cost",1.5], ["costsResource", 0.3, "stone"], ["costsResource", 0.3, "metal"] ] };
		this.unitStat["MeleeInfantry"]  = { "priority": 1, "minSize": 6, "targetSize": 18, "batchSize": 3, "classes": ["Infantry","Melee"],
			"interests": [ ["canGather", 1], ["strength",1.6], ["cost",1.5], ["costsResource", 0.3, "stone"], ["costsResource", 0.3, "metal"] ] };
		this.neededShips = 3;
	}

	// Put some randomness on the attack size
	var variation = 0.8 + 0.4*Math.random();
	for (var cat in this.unitStat)
	{
		this.unitStat[cat]["targetSize"] = Math.round(variation * this.unitStat[cat]["targetSize"]);
		this.unitStat[cat]["minSize"] = Math.min(this.unitStat[cat]["minSize"], this.unitStat[cat]["targetSize"]);
	}

	// TODO: there should probably be one queue per type of training building
	gameState.ai.queueManager.addQueue("plan_" + this.name, priority);
	this.queue = gameState.ai.queues["plan_" + this.name];
	gameState.ai.queueManager.addQueue("plan_" + this.name +"_champ", priority+1);
	this.queueChamp = gameState.ai.queues["plan_" + this.name +"_champ"];
	gameState.ai.queueManager.addQueue("plan_" + this.name +"_siege", priority);
	this.queueSiege = gameState.ai.queues["plan_" + this.name +"_siege"];
	/*
	this.unitStat["Siege"]["filter"] = function (ent) {
		var strength = [ent.attackStrengths("Melee")["crush"],ent.attackStrengths("Ranged")["crush"]];
		return (strength[0] > 15 || strength[1] > 15);
	};*/

	this.unitCollection = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "plan", this.name));
	this.unitCollection.registerUpdates();
	
	this.unit = {};
	
	// each array is [ratio, [associated classes], associated EntityColl, associated unitStat, name ]
	this.buildOrder = [];
	this.canBuildUnits = gameState.ai.HQ.canBuildUnits;
	
	// defining the entity collections. Will look for units I own, that are part of this plan.
	// Also defining the buildOrders.
	for (var cat in this.unitStat)
	{
		var Unit = this.unitStat[cat];
		var filter = API3.Filters.and(API3.Filters.byClassesAnd(Unit["classes"]), API3.Filters.byMetadata(PlayerID, "plan",this.name));
		this.unit[cat] = gameState.getOwnUnits().filter(filter);
		this.unit[cat].registerUpdates();
		if (this.canBuildUnits)
			this.buildOrder.push([0, Unit["classes"], this.unit[cat], Unit, cat]);
	}
	
	// some variables for during the attack
	this.position5TurnsAgo = [0,0];
	this.lastPosition = [0,0];
	this.position = [0,0];

	// get a good path to an estimated target.
	this.pathFinder = new API3.aStarPath(gameState, false, false, this.targetPlayer);
	//Engine.DumpImage("widthmap.png", this.pathFinder.widthMap, this.pathFinder.width,this.pathFinder.height,255);

	this.pathWidth = 6;	// prefer a path far from entities. This will avoid units getting stuck in trees and also results in less straight paths.
	this.pathSampling = 2;

	return true;
};

m.AttackPlan.prototype.getName = function()
{
	return this.name;
};

m.AttackPlan.prototype.getType = function()
{
	return this.type;
};

m.AttackPlan.prototype.isStarted = function()
{
	return (this.state !== "unexecuted" && this.state !== "completing");
};

m.AttackPlan.prototype.isPaused = function()
{
	return this.paused;
};

m.AttackPlan.prototype.setPaused = function(boolValue)
{
	this.paused = boolValue;
};

m.AttackPlan.prototype.getEnemyPlayer = function(gameState)
{
	var enemyPlayer = undefined;

	// first check if there is a preferred enemy based on our victory conditions
	if (gameState.getGameType() === "wonder")
	{
		var moreAdvanced = undefined;
		var enemyWonder = undefined;
		var wonders = gameState.getEnemyStructures().filter(API3.Filters.byClass("Wonder")).toEntityArray();
		for (var wonder of wonders)
		{
			var progress = wonder.foundationProgress();
			if (progress === undefined)
				return wonder.owner();
			if (enemyWonder && moreAdvanced > progress)
				continue;
			enemyWonder = wonder;
			moreAdvanced = progress;
		}
		if (enemyWonder)
			return enemyWonder.owner();
	}

	// then let's find our prefered target enemy, basically counting our enemies units
	// with priority to enemies with civ center
	var enemyCount = {};
	var enemyDefense = {};
	var enemyCivCentre = {};
	for (var i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		enemyCount[i] = 0;
		enemyDefense[i] = 0;
		enemyCivCentre[i] = false;
	}
	gameState.getEntities().forEach(function(ent) { 
		if (gameState.isEntityEnemy(ent) && ent.owner() !== 0)
		{
			enemyCount[ent.owner()]++;
			if (ent.hasClass("Tower") || ent.hasClass("Fortress"))
				enemyDefense[ent.owner()]++;
			if (ent.hasClass("CivCentre"))
				enemyCivCentre[ent.owner()] = true;
		}
	});
	var max = 0;
	for (var i in enemyCount)
	{
		if (this.type === "Rush" && enemyDefense[i] > 6)  // No rush if enemy too well defended (iberians)
			continue;
		var count = enemyCount[i];
		if (enemyCivCentre[i])
			count += 500;
		if (count > max)
		{
			enemyPlayer = +i;
			max = count;
		}
	}
	return enemyPlayer;
};

// Returns true if the attack can be executed at the current time
// Basically it checks we have enough units.
m.AttackPlan.prototype.canStart = function(gameState)
{	
	if (!this.canBuildUnits)
		return true;

	for (var unitCat in this.unitStat)
	{
		var Unit = this.unitStat[unitCat];
		if (this.unit[unitCat].length < Unit["minSize"])
			return false;
	}
	return true;
};

m.AttackPlan.prototype.mustStart = function(gameState)
{
	if (this.isPaused() || this.path === undefined)
		return false;

	if (!this.canBuildUnits)
		return true;

	var MaxReachedEverywhere = true;
	var MinReachedEverywhere = true;
	for (var unitCat in this.unitStat)
	{
		var Unit = this.unitStat[unitCat];
		if (this.unit[unitCat].length < Unit["targetSize"])
			MaxReachedEverywhere = false;
		if (this.unit[unitCat].length < Unit["minSize"])
		{
			MinReachedEverywhere = false;
			break;
		}
	}

	if (MaxReachedEverywhere)
		return true;
	if (MinReachedEverywhere)
	{
		if ((gameState.getPopulationMax() - gameState.getPopulation() < 10) ||
			(this.type === "Raid" && this.target && this.target.foundationProgress() && this.target.foundationProgress() > 60))
			return true;
	}
	return false;
};

// Adds a build order. If resetQueue is true, this will reset the queue.
m.AttackPlan.prototype.addBuildOrder = function(gameState, name, unitStats, resetQueue)
{
	if (!this.isStarted())
	{
		// no minsize as we don't want the plan to fail at the last minute though.
		this.unitStat[name] = unitStats;
		var Unit = this.unitStat[name];
		var filter = API3.Filters.and(API3.Filters.byClassesAnd(Unit["classes"]), API3.Filters.byMetadata(PlayerID, "plan", this.name));
		this.unit[name] = gameState.getOwnUnits().filter(filter);
		this.unit[name].registerUpdates();
		this.buildOrder.push([0, Unit["classes"], this.unit[name], Unit, name]);
		if (resetQueue)
		{
			this.queue.empty();
			this.queueChamp.empty();
			this.queueSiege.empty();
		}
	}
};

m.AttackPlan.prototype.addSiegeUnits = function(gameState)
{
	if (this.unitStat["Siege"] || this.state !== "unexecuted")
		return false;
	// no minsize as we don't want the plan to fail at the last minute though.
	var stat = { "priority": 1., "minSize": 0, "targetSize": 4, "batchSize": 2, "classes": ["Siege"],
		"interests": [ ["siegeStrength", 3], ["cost",1] ] };
	if (gameState.civ() === "maur")
		stat["classes"] = ["Elephant", "Champion"];
	this.addBuildOrder(gameState, "Siege", stat, true);
	return true;
};

// Three returns possible: 1 is "keep going", 0 is "failed plan", 2 is "start"
// 3 is a special case: no valid path returned. Right now I stop attacking alltogether.
m.AttackPlan.prototype.updatePreparation = function(gameState, events)
{
	// the completing step is used to return resources and regroup the units
	// so we check that we have no more forced order before starting the attack
	if (this.state === "completing")
	{
		// check that all units have finished with their transport if needed
		if (this.waitingForTransport())
			return 1;
		// bloqued units which cannot finish their order should not stop the attack
		if (gameState.ai.playedTurn < this.maxCompletingTurn && this.hasForceOrder())
			return 1;
		return 2;
	}

	if (this.Config.debug > 2 && gameState.ai.playedTurn % 50 === 0)
		this.debugAttack();

	// find our target
	if (this.target === undefined)
	{
		this.target = this.getNearestTarget(gameState, this.rallyPoint);
		if (!this.target)
		{
			var oldTargetPlayer = this.targetPlayer;
			// may-be all our previous enemey targets have been destroyed ?
			this.targetPlayer = this.getEnemyPlayer(gameState);
			if (this.Config.debug > 0)
				API3.warn(" === no more target for enemy player " + oldTargetPlayer + " let us switch against player " + this.targetPlayer);
			this.target = this.getNearestTarget(gameState, this.rallyPoint);
		}
		if (!this.target)
			return 0;
		this.targetPos = this.target.position();
		// redefine a new rally point for this target if we have a base on the same land
		// find a new one on the pseudo-nearest base (dist weighted by the size of the island)
		var targetIndex = gameState.ai.accessibility.getAccessValue(this.targetPos);
		var rallyIndex = gameState.ai.accessibility.getAccessValue(this.rallyPoint);
		if (targetIndex !== rallyIndex)
		{
			var distminSame = Math.min();
			var rallySame = undefined;
			var distminDiff = Math.min();
			var rallyDiff = undefined;
			for each (var base in gameState.ai.HQ.baseManagers)
			{
				var anchor = base.anchor;
				if (!anchor || !anchor.position())
					continue;
				var dist = API3.SquareVectorDistance(anchor.position(), this.targetPos);
				if (base.accessIndex === targetIndex)
				{
					if (dist < distminSame)
					{
						distminSame = dist;
						rallySame = anchor.position();
					}
				}
				else
				{
					dist = dist / Math.sqrt(gameState.ai.accessibility.regionSize[base.accessIndex]);
					if (dist < distminDiff)
					{
						distminDiff = dist;
						rallyDiff = anchor.position();
					}
				}
			}
			if (rallySame)
				this.rallyPoint = rallySame;
			else if (rallyDiff)
			{
				this.rallyPoint = rallyDiff;
				this.overseas = gameState.ai.HQ.getSeaIndex(gameState, rallyIndex, targetIndex);
				if (this.overseas)
					gameState.ai.HQ.navalManager.setMinimalTransportShips(gameState, this.overseas, this.neededShips);
				else
					return 0;
			}
		}
	}

	// when we have a target, we path to it.
	// I'd like a good high width sampling first.
	// Thus I will not do everything at once.
	// It will probably carry over a few turns but that's no issue.
	if (this.path === undefined || this.path === "toBeContinued")
	{
		var ret = this.getPathToTarget(gameState);
		if (ret >= 0)
			return ret;
	}

	// if we need a transport, wait for some transport ships
	if (this.overseas && !gameState.ai.HQ.navalManager.seaTransportShips[this.overseas].length)
		return 1;

	this.assignUnits(gameState);

	// special case: if we've reached max pop, and we can start the plan, start it.
	if (gameState.getPopulationMax() - gameState.getPopulation() < 10)
	{
		if (this.canStart())
		{
			this.queue.empty();
			this.queueChamp.empty();
			this.queueSiege.empty();
		}
		else	// Abort the plan so that its units will be reassigned to other plans.
		{
			if (this.Config.debug > 0)
			{
				var am = gameState.ai.HQ.attackManager;
				API3.warn(" attacks upcoming: raid " + am.upcomingAttacks["Raid"].length
					+ " rush " + am.upcomingAttacks["Rush"].length
					+ " attack " + am.upcomingAttacks["Attack"].length
					+ " huge " + am.upcomingAttacks["HugeAttack"].length);
				API3.warn(" attacks started: raid " + am.startedAttacks["Raid"].length
					+ " rush " + am.startedAttacks["Rush"].length
					+ " attack " + am.startedAttacks["Attack"].length
					+ " huge " + am.startedAttacks["HugeAttack"].length);
			}
			return 0;
	    }
	}
	else if (this.mustStart(gameState) && (gameState.countOwnQueuedEntitiesWithMetadata("plan", +this.name) > 0))
	{
		// keep on while the units finish being trained, then we'll start
		this.queue.empty();
		this.queueChamp.empty();
		this.queueSiege.empty();
		return 1;
	}
	else if (!this.mustStart(gameState))
	{
		if (this.canBuildUnits)
		{
			// We still have time left to recruit units and do stuffs.
			this.trainMoreUnits(gameState);
			// may happen if we have no more training facilities and build orders are canceled
			if (this.buildOrder.length === 0)
				return 0;	// will abort the plan
		}
		return 1;
	}

	// if we're here, it means we must start (and have no units in training left).
	this.state = "completing";
	this.maxCompletingTurn = gameState.ai.playedTurn + 60;

	var rallyPoint = this.rallyPoint;
	var rallyIndex = gameState.ai.accessibility.getAccessValue(rallyPoint);
	this.unitCollection.forEach(function (entity) {
		// For the time being, if occupied in a transport, remove the unit from this plan   TODO improve that
		if (entity.getMetadata(PlayerID, "transport") !== undefined || entity.getMetadata(PlayerID, "transporter") !== undefined)
		{
			entity.setMetadata(PlayerID, "plan", -1);
			return;
		}
		entity.setMetadata(PlayerID, "role", "attack");
		entity.setMetadata(PlayerID, "subrole", "completing");
		var queued = false;
		if (entity.resourceCarrying() && entity.resourceCarrying().length)
		{
			if (!entity.getMetadata(PlayerID, "worker-object"))
				entity.setMetadata(PlayerID, "worker-object", new m.Worker(entity));
			queued = entity.getMetadata(PlayerID, "worker-object").returnResources(gameState);
		}
		var index = gameState.ai.accessibility.getAccessValue(entity.position());
		if (index === rallyIndex)
			entity.move(rallyPoint[0], rallyPoint[1], queued);
		else
			gameState.ai.HQ.navalManager.requireTransport(gameState, entity, index, rallyIndex, rallyPoint);
	});

	// reset all queued units
	var plan = this.name;
	gameState.ai.queueManager.removeQueue("plan_" + plan);
	gameState.ai.queueManager.removeQueue("plan_" + plan + "_champ");
	gameState.ai.queueManager.removeQueue("plan_" + plan + "_siege");
	return	1;
};


m.AttackPlan.prototype.trainMoreUnits = function(gameState)
{
	// let's sort by training advancement, ie 'current size / target size'
	// count the number of queued units too.
	// substract priority.
	for (var i = 0; i < this.buildOrder.length; ++i)
	{
		var special = "Plan_" + this.name + "_" + this.buildOrder[i][4];
		var aQueued = gameState.countOwnQueuedEntitiesWithMetadata("special", special);
		aQueued += this.queue.countQueuedUnitsWithMetadata("special", special);
		aQueued += this.queueChamp.countQueuedUnitsWithMetadata("special", special);
		aQueued += this.queueSiege.countQueuedUnitsWithMetadata("special", special);
		this.buildOrder[i][0] = this.buildOrder[i][2].length + aQueued;
	}
	this.buildOrder.sort(function (a,b) {
		var va = a[0]/a[3]["targetSize"] - a[3]["priority"];
		if (a[0] >= a[3]["targetSize"])
			va += 1000;
		var vb = b[0]/b[3]["targetSize"] - b[3]["priority"];
		if (b[0] >= b[3]["targetSize"])
			vb += 1000;
		return va - vb;
	});

	if (this.Config.debug > 0 && gameState.ai.playedTurn%50 === 0)
	{
		API3.warn("====================================");
		API3.warn("======== build order for plan " + this.name);
		for (var order of this.buildOrder)
		{
			var specialData = "Plan_"+this.name+"_"+order[4];
			var inTraining = gameState.countOwnQueuedEntitiesWithMetadata("special", specialData);
			var queue1 = this.queue.countQueuedUnitsWithMetadata("special", specialData);
			var queue2 = this.queueChamp.countQueuedUnitsWithMetadata("special", specialData);
			var queue3 = this.queueSiege.countQueuedUnitsWithMetadata("special", specialData);
			API3.warn(" >>> " + order[4] + " done " + order[2].length + " training " + inTraining
				+ " queue " + queue1 + " champ " + queue2 + " siege " + queue3 + " >> need " + order[3].targetSize); 
		}
		API3.warn("====================================");
	}

	if (this.buildOrder[0][0] < this.buildOrder[0][3]["targetSize"])
	{
		// find the actual queue we want
		var queue = this.queue;
		if (this.buildOrder[0][3]["classes"].indexOf("Siege") !== -1 ||
			(gameState.civ() == "maur" && this.buildOrder[0][3]["classes"].indexOf("Elephant") !== -1 && this.buildOrder[0][3]["classes"].indexOf("Champion")))
			queue = this.queueSiege;
		else if (this.buildOrder[0][3]["classes"].indexOf("Champion") !== -1)
			queue = this.queueChamp;

		if (queue.length() <= 5)
		{
			var template = gameState.ai.HQ.findBestTrainableUnit(gameState, this.buildOrder[0][1], this.buildOrder[0][3]["interests"]);
			// HACK (TODO replace) : if we have no trainable template... Then we'll simply remove the buildOrder,
			// effectively removing the unit from the plan.
			if (template === undefined)
			{
				if (this.Config.debug > 0)
					API3.warn("attack no template found " + this.buildOrder[0][1]);
				delete this.unitStat[this.buildOrder[0][4]];	// deleting the associated unitstat.
				this.buildOrder.splice(0,1);
			}
			else
			{
				if (this.Config.debug > 1)
					API3.warn("attack template " + template + " added for plan " + this.name);
				var max = this.buildOrder[0][3]["batchSize"];
				var specialData = "Plan_" + this.name + "_" + this.buildOrder[0][4];
				if (gameState.getTemplate(template).hasClass("CitizenSoldier"))
					var trainingPlan = new m.TrainingPlan(gameState, template, { "role": "worker", "plan": this.name, "special": specialData, "base": 0 }, max, max);
				else
					var trainingPlan = new m.TrainingPlan(gameState, template, { "role": "attack", "plan": this.name, "special": specialData, "base": 0 }, max, max);
				if (trainingPlan.template)
					queue.addItem(trainingPlan);
				else if (this.Config.debug > 0)
					API3.warn("training plan canceled because no template for " + template + "   build1 " + uneval(this.buildOrder[0][1])
						+ " build3 " + uneval(this.buildOrder[0][3]["interests"]));
			}
		}
	}
};

m.AttackPlan.prototype.assignUnits = function(gameState)
{
	var plan = this.name;
	var added = false;
	var self = this;
	// If we can not build units, assign all available except those affected to allied defense to the current attack
	if (!this.canBuildUnits)
	{
		gameState.getOwnUnits().forEach(function(ent) {
			if (!ent.position())
				return;
			if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
				return;
			if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
				return;
			if (ent.getMetadata(PlayerID, "allied"))
				return;
			ent.setMetadata(PlayerID, "plan", plan);
			self.unitCollection.updateEnt(ent);
			added = true; 
		});
		return added;
	}

	// TODO: assign myself units that fit only, right now I'm getting anything.
	// Assign all no-roles that fit (after a plan aborts, for example).
	if (this.type === "Raid")
	{
		var num = 0;
		gameState.getOwnUnits().filter(API3.Filters.byClass("Cavalry")).forEach(function(ent) {
			if (!ent.position())
				return;
			if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
				return;
			if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
				return;
			if (num++ < 2)
				return;
			ent.setMetadata(PlayerID, "plan", plan);
			self.unitCollection.updateEnt(ent);
			added = true;
		});
		return added;
	}

	var noRole = gameState.getOwnEntitiesByRole(undefined, false).filter(API3.Filters.byClass("Unit"));
	noRole.forEach(function(ent) {
		if (!ent.position())
			return;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
			return;
		if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
			return;
		if (ent.hasClass("Ship") || ent.hasClass("Support") || ent.attackTypes() === undefined)
			return;
		ent.setMetadata(PlayerID, "plan", plan);
		self.unitCollection.updateEnt(ent);
		added = true;
	});
	// Add units previously in a plan, but which left it because needed for defense or attack finished
	gameState.ai.HQ.attackManager.outOfPlan.forEach(function(ent) {
		if (!ent.position())
			return;
		if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
			return;
		ent.setMetadata(PlayerID, "plan", plan);
		self.unitCollection.updateEnt(ent);
		added = true;
	});

	if (this.type !== "Rush")
		return added;

	// For a rush, assign also workers (but keep a minimum number of defenders)
	var worker = gameState.getOwnEntitiesByRole("worker", true).filter(API3.Filters.byClass("Unit"));
	var num = 0;
	worker.forEach(function(ent) {
		if (!ent.position())
			return;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
			return;
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			return;
		if (ent.hasClass("Ship") || ent.hasClass("Support") || ent.attackTypes() === undefined)
			return;
		if (num++ < 9)
			return;
		ent.setMetadata(PlayerID, "plan", plan);
		self.unitCollection.updateEnt(ent);
		added = true;
	});
	return added;
};

// sameLand true means that we look for a target for which we do not need to take a transport
m.AttackPlan.prototype.getNearestTarget = function(gameState, position, sameLand)
{
	if (this.type === "Raid")
		var targets = this.raidTargetFinder(gameState);
	else if (this.type === "Rush" || this.type === "Attack")
		var targets = this.rushTargetFinder(gameState);
	else
		var targets = this.defaultTargetFinder(gameState);
	if (targets.length === 0)
		return undefined;

	var land = gameState.ai.accessibility.getAccessValue(position);

	// picking the nearest target
	var minDist = -1;
	var index = undefined;
	for (var i in targets._entities)
	{
		if (!targets._entities[i].position())
			continue;
		if (sameLand && gameState.ai.accessibility.getAccessValue(targets._entities[i].position()) !== land)
			continue;
		var dist = API3.SquareVectorDistance(targets._entities[i].position(), position);
		if (dist < minDist || minDist === -1)
		{
			minDist = dist;
			index = i;
		}
	}
	if (!index)
		return undefined;
	return targets._entities[index];
};

// Default target finder aims for conquest critical targets
m.AttackPlan.prototype.defaultTargetFinder = function(gameState)
{
	var targets = undefined;
	if (gameState.getGameType() === "wonder")
	{
		targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("Wonder"));
		if (targets.length)
			return targets;
	}

	var targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("CivCentre"));
	if (!targets.length)
		targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("ConquestCritical"));
	// If there's nothing, attack anything else that's less critical
	if (!targets.length)
		targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("Town"));
	if (!targets.length)
		targets = gameState.getEnemyStructures(this.targetPlayer).filter(API3.Filters.byClass("Village"));
	// no buildings, attack anything conquest critical, even units
	if (!targets.length)
		targets = gameState.getEnemyEntities(this.targetPlayer).filter(API3.Filters.byClass("ConquestCritical"));
	return targets;
};

// Rush target finder aims at isolated non-defended buildings
m.AttackPlan.prototype.rushTargetFinder = function(gameState)
{
	var targets = new API3.EntityCollection(gameState.sharedScript);
	var buildings = gameState.getEnemyStructures().toEntityArray();
	if (buildings.length === 0)
		return targets;

	this.position = this.unitCollection.getCentrePosition();
	if (!this.position)
		this.position = this.rallyPoint;

	var minDist = Math.min();
	var target = undefined;
	for (var building of buildings)
	{
		if (building.owner() === 0)
			continue;
		// TODO check on Arrow count
		if (building.hasClass("CivCentre") || building.hasClass("Tower") || building.hasClass("Fortress"))
			continue;
		var pos = building.position();
		var defended = false;
		for (var defense of buildings)
		{
			if (!defense.hasClass("CivCentre") && !defense.hasClass("Tower") && !defense.hasClass("Fortress"))
				continue;
			var dist = API3.SquareVectorDistance(pos, defense.position());
			if (dist < 4900)   // TODO check on defense range rather than this fixed 70*70
			{
				defended = true;
				break;
			}
		}
		if (defended)
			continue;
		var dist = API3.SquareVectorDistance(pos, this.position);
		if (dist > minDist)
			continue;
		minDist = dist;
		target = building;
	}
	if (target)
		targets.addEnt(target);

	if (targets.length == 0 && this.type === "Attack")
		targets = this.defaultTargetFinder(gameState);

	return targets;
};

// Raid target finder aims at destructing foundations from which our defenseManager has attacked the builders
m.AttackPlan.prototype.raidTargetFinder = function(gameState)
{
	var targets = new API3.EntityCollection(gameState.sharedScript);
	for (var targetId of gameState.ai.HQ.defenseManager.targetList)
	{
		var target = gameState.getEntityById(targetId);
		if (target && target.position())
			targets.addEnt(target);
	}
	return targets
};

m.AttackPlan.prototype.getPathToTarget = function(gameState)
{
	if (this.path === undefined)
		this.path = this.pathFinder.getPath(this.rallyPoint, this.targetPos, this.pathSampling, this.pathWidth, 175);
	else if (this.path === "toBeContinued")
		this.path = this.pathFinder.continuePath();
		
	if (this.path === undefined)
	{
		if (this.pathWidth == 6)
		{
			this.pathWidth = 2;
			delete this.path;
		}
		else
		{
			delete this.pathFinder;
			return 3;	// no path.
		}
	}
	else if (this.path === "toBeContinued")
	{
		return 1;	// carry on
	} 
	else if (this.path[1] === true && this.pathWidth == 2)
	{
		// okay so we need a ship.
		// Basically we'll add it as a new class to train compulsorily, and we'll recompute our path.
		if (!gameState.ai.HQ.navalMap)
		{
			gameState.ai.HQ.navalMap = true;
			return 0;
		}
		this.pathWidth = 3;
		this.pathSampling = 3;
		this.path = this.path[0].reverse();
		delete this.pathFinder;
		// Change the rally point to something useful (should avoid rams getting stuck in our territor)
		this.setRallyPoint(gameState);
	}
	else if (this.path[1] === true && this.pathWidth == 6)
	{
		// retry with a smaller pathwidth:
		this.pathWidth = 2;
		delete this.path;
	}
	else
	{
		this.path = this.path[0].reverse();
		delete this.pathFinder;
		// Change the rally point to something useful (should avoid rams getting stuck in our territor)
		this.setRallyPoint(gameState);
	}
	return -1;    // ok
};

m.AttackPlan.prototype.setRallyPoint = function(gameState)
{
	for (var i = 0; i < this.path.length; ++i)
	{
		// my pathfinder returns arrays in arrays in arrays.
		var waypointPos = this.path[i][0];
		if (gameState.ai.HQ.territoryMap.getOwner(waypointPos) !== PlayerID || this.path[i][1] === true)
		{
			// Set rally point at the border of our territory
			// or where we need to change transportation method.
			if (i !== 0)
				this.rallyPoint = this.path[i-1][0];
			else
				this.rallyPoint = this.path[0][0];

			if (i >= 2)
				this.path.splice(0, i-1);
			break;
		}
	}
};

// Executes the attack plan, after this is executed the update function will be run every turn
// If we're here, it's because we have enough units.
m.AttackPlan.prototype.StartAttack = function(gameState)
{
	if (this.Config.debug)
		API3.warn("start attack " + this.name + " with type " + this.type);

	if (!this.target || !gameState.getEntityById(this.target.id()))  // our target was destroyed during our preparation
	{
		if (!this.targetPos)    // should not happen
			return false;
		var targetIndex = gameState.ai.accessibility.getAccessValue(this.targetPos);
		var rallyIndex = gameState.ai.accessibility.getAccessValue(this.rallyPoint);
		if (targetIndex === rallyIndex)
		{
			// If on the same index: if we are doing a raid, look for a better target,
			// otherwise proceed with the previous target position
			// and we will look for a better target there
			if (this.type === "Raid")
			{
				this.target = this.getNearestTarget(gameState, this.rallyPoint);
				if (!this.target)
					return false;
				this.targetPos = this.target.position();
			}
		}
		else
		{
			// Not on the same index, do not loose time to go to previous targetPos if nothing there
			// so directly look for a new target right now
			this.target = this.getNearestTarget(gameState, this.rallyPoint);
			if (!this.target)
				return false;
			this.targetPos = this.target.position();
		}
	}

	// check we have a target and a path.
	if (this.targetPos && this.path !== undefined)
	{
		// erase our queue. This will stop any leftover unit from being trained.
		gameState.ai.queueManager.removeQueue("plan_" + this.name);
		gameState.ai.queueManager.removeQueue("plan_" + this.name + "_champ");
		gameState.ai.queueManager.removeQueue("plan_" + this.name + "_siege");
		
		var curPos = this.unitCollection.getCentrePosition();
		
		this.unitCollection.forEach(function(ent) {
			ent.setMetadata(PlayerID, "subrole", "walking");
		});
		// optimize our collection now.
		this.unitCollection.allowQuickIter();

		this.unitCollection.setStance("aggressive");

		if (gameState.ai.accessibility.getAccessValue(this.targetPos) === gameState.ai.accessibility.getAccessValue(this.rallyPoint))
		{
			if (!this.path[0][0][0] || !this.path[0][0][1])
			{
				if (this.Config.debug > 0)
					API3.warn("StartAttack: Problem with path " + uneval(this.path));
				return false;
			}
			this.state = "walking";
			this.unitCollection.move(this.path[0][0][0], this.path[0][0][1]);
		}
		else
		{
			this.state = "transporting";
			var startIndex = gameState.ai.accessibility.getAccessValue(this.rallyPoint);
			var endIndex = gameState.ai.accessibility.getAccessValue(this.targetPos);
			var endPos = this.targetPos;
			// TODO require a global transport for the collection,
			// and put back its state to "walking" when the transport is finished
			this.unitCollection.forEach(function (entity) {
				gameState.ai.HQ.navalManager.requireTransport(gameState, entity, startIndex, endIndex, endPos);
			});
		}
	}
	else
	{
		gameState.ai.gameFinished = true;
		API3.warn("I do not have any target. So I'll just assume I won the game.");
		return false;
	}
	return true;
};

// Runs every turn after the attack is executed
m.AttackPlan.prototype.update = function(gameState, events)
{
	if (this.unitCollection.length === 0)
		return 0;

	Engine.ProfileStart("Update Attack");

	this.position = this.unitCollection.getCentrePosition();
	var IDs = this.unitCollection.toIdArray();

	var self = this;

	// we are transporting our units, let's wait
	// TODO retaliate if attacked while waiting for the rest of the units
	// and instead of state "arrived", made a state "walking" with a new path
	if (this.state === "transporting")
	{
		var done = true;
		this.unitCollection.forEach(function (entity) {
			if (self.Config.debug > 0 && entity.getMetadata(PlayerID, "transport") !== undefined)
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [entity.id()], "rgb": [2,2,0]});
			else if (self.Config.debug > 0)
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [entity.id()], "rgb": [1,1,1]});
			if (!done)
				return;
			if (entity.getMetadata(PlayerID, "transport") !== undefined)
				done = false;
		});

		if (done)
			this.state = "arrived";
		else
		{
			// if we are attacked while waiting the rest of the army, retaliate
			var attackedEvents = events["Attacked"];
			for (var evt of attackedEvents)
			{
				if (IDs.indexOf(evt.target) === -1)
					continue;
				var attacker = gameState.getEntityById(evt.attacker);
				var ourUnit = gameState.getEntityById(evt.target);
				if (!attacker || !ourUnit)
					continue;
				this.unitCollection.forEach(function (entity) {
					if (entity.getMetadata(PlayerID, "transport") !== undefined)
						return;
					if (!entity.isIdle())
						return;
					entity.attack(attacker.id());
				});
				break;
			}
		}
	}

	// this actually doesn't do anything right now.
	if (this.state === "walking")
	{
		// we're marching towards the target
		// Let's check if any of our unit has been attacked.
		// In case yes, we'll determine if we're simply off against an enemy army, a lone unit/building
		// or if we reached the enemy base. Different plans may react differently.		
		var attackedNB = 0;
		var attackedEvents = events["Attacked"];
		for (var evt of attackedEvents)
		{
			if (IDs.indexOf(evt.target) === -1)
				continue;
			var attacker = gameState.getEntityById(evt.attacker);
			var ourUnit = gameState.getEntityById(evt.target);

			if (attacker && attacker.position() && attacker.hasClass("Unit") && attacker.owner() != 0)
				attackedNB++;
			// if we're being attacked by a building, flee.
			if (attacker && ourUnit && attacker.hasClass("Structure"))
				ourUnit.flee(attacker);
		}
		// Are we arrived at destination ?
		if ((gameState.ai.HQ.territoryMap.getOwner(this.position) === this.targetPlayer && attackedNB > 1) || attackedNB > 3)
			this.state = "arrived";
	}

	if (this.state === "walking")
	{
		// basically haven't moved an inch: very likely stuck)
		if (API3.SquareVectorDistance(this.position, this.position5TurnsAgo) < 10 && this.path.length > 0 && gameState.ai.playedTurn % 5 === 0)
		{
			// check for stuck siege units
			var farthest = 0;
			var farthestEnt = -1;
			this.unitCollection.filter(API3.Filters.byClass("Siege")).forEach (function (ent) {
				var dist = API3.SquareVectorDistance(ent.position(), self.position);
				if (dist < farthest)
					return;
				farthest = dist;
				farthestEnt = ent;
			});
			if (farthestEnt !== -1)
				farthestEnt.destroy();
		}
		if (gameState.ai.playedTurn % 5 === 0)
			this.position5TurnsAgo = this.position;
		
		if (this.lastPosition && API3.SquareVectorDistance(this.position, this.lastPosition) < 20 && this.path.length > 0)
		{
			if (!this.path[0][0][0] || !this.path[0][0][1])
				API3.warn("Start: Problem with path " + uneval(this.path));
			// We're stuck, presumably. Check if there are no walls just close to us. If so, we're arrived, and we're gonna tear down some serious stone.
			var nexttoWalls = false;
			gameState.getEnemyEntities().filter(API3.Filters.byClass("StoneWall")).forEach( function (ent) {
				if (!nexttoWalls && API3.SquareVectorDistance(self.position, ent.position()) < 800)
					nexttoWalls = true;
			});
			// there are walls but we can attack
			if (nexttoWalls && this.unitCollection.filter(API3.Filters.byCanAttack("StoneWall")).length !== 0)
			{
				if (this.Config.debug > 0)
					API3.warn("Attack Plan " + this.type + " " + this.name + " has met walls and is not happy.");
				this.state = "arrived";
			}
			else if (nexttoWalls)	// abort plan
			{
				if (this.Config.debug > 0)
					API3.warn("Attack Plan " + this.type + " " + this.name + " has met walls and gives up.");
				Engine.ProfileStop();
				return 0;
			}
			else
				//this.unitCollection.move(this.path[0][0][0], this.path[0][0][1]);
				this.unitCollection.moveIndiv(this.path[0][0][0], this.path[0][0][1]);
		}
	}

	// check if our units are close enough from the next waypoint.
	if (this.state === "walking")
	{
		if (API3.SquareVectorDistance(this.position, this.targetPos) < 10000)
		{
			if (this.Config.debug > 0)
				API3.warn("Attack Plan " + this.type + " " + this.name + " has arrived to destination.");
			this.state = "arrived";
		}
		else if (this.path.length && API3.SquareVectorDistance(this.position, this.path[0][0]) < 1600)
		{
			this.path.shift();
			if (this.path.length)
				this.unitCollection.move(this.path[0][0][0], this.path[0][0][1]);
			else
			{
				if (this.Config.debug > 0)
					API3.warn("Attack Plan " + this.type + " " + this.name + " has arrived to destination.");
				this.state = "arrived";
			}
		}
	}

	if (this.state === "arrived")
	{
		// let's proceed on with whatever happens now.
		this.state = "";
		this.startingAttack = true;
		this.unitCollection.forEach( function (ent) {
			ent.stopMoving();
			ent.setMetadata(PlayerID, "subrole", "attacking");
		});
		if (this.type === "Rush")   // try to find a better target for rush
		{
			var targets = this.rushTargetFinder(gameState);
			if (targets.length !== 0)
			{
				for (var i in targets._entities)
				{
					this.target = targets._entities[i];
					break;
				}
				this.targetPos = this.target.position();
			}
		}
	}
	
	// basic state of attacking.
	if (this.state === "")
	{
		var time = gameState.ai.elapsedTime;
		var attackedEvents = events["Attacked"];
		for (var evt of attackedEvents)
		{
			if (IDs.indexOf(evt.target) === -1)
				continue;
			var attacker = gameState.getEntityById(evt.attacker);
			if (!attacker || !attacker.position() || !attacker.hasClass("Unit"))
				continue;
			var ourUnit = gameState.getEntityById(evt.target);
			if (this.isSiegeUnit(gameState, ourUnit))
			{
				// if siege units are attacked, we'll send some units to deal with enemies.
				var collec = this.unitCollection.filter(API3.Filters.not(API3.Filters.byClass("Siege"))).filterNearest(ourUnit.position(), 5).toEntityArray();
				for (var ent of collec)
					if (!this.isSiegeUnit(gameState, ent))
					{
						ent.attack(attacker.id());
						ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
					}
			}
			else
			{
				// if units are attacked, abandon their target (if it was a structure or a support) and retaliate
				var orderData = ourUnit.unitAIOrderData();
				if (orderData.length !== 0 && orderData[0]["target"])
				{
					var target = gameState.getEntityById(orderData[0]["target"]);
					if (target && !target.hasClass("Structure") && !target.hasClass("Support"))
						continue;
				}
				ourUnit.attack(attacker.id());
				ourUnit.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
			}
		}
		
		var enemyUnits = gameState.getEnemyUnits(this.targetPlayer);
		var enemyStructures = gameState.getEnemyStructures(this.targetPlayer);

		if (this.unitCollUpdateArray === undefined || this.unitCollUpdateArray.length === 0)
			this.unitCollUpdateArray = this.unitCollection.toIdArray();

		// Let's check a few units each time we update (currently 10) except when attack starts
		if (this.unitCollUpdateArray.length < 15 || this.startingAttack)
			var lgth = this.unitCollUpdateArray.length;
		else
			var lgth = 10;
		for (var check = 0; check < lgth; check++)
		{
			var ent = gameState.getEntityById(this.unitCollUpdateArray[check]);
			if (!ent || !ent.position())
				continue;

			var orderData = ent.unitAIOrderData();
			if (orderData.length !== 0)
				orderData = orderData[0];
			else
				orderData = undefined;
	
			// update the order if needed
			var needsUpdate = false;
			var maybeUpdate = false;
			var siegeUnit = this.isSiegeUnit(gameState, ent);
			if (ent.isIdle())
				needsUpdate = true;
			else if (siegeUnit && orderData && orderData["target"])
			{
				var target = gameState.getEntityById(orderData["target"]);
				if (!target)
					needsUpdate = true;
				else if(!target.hasClass("Structure"))
					maybeUpdate = true;
			}
			else if (orderData && orderData["target"])
			{
				var target = gameState.getEntityById(orderData["target"]);
				if (!target)
					needsUpdate = true;
				else if (target.hasClass("Structure"))
					maybeUpdate = true;
				else if (!ent.hasClass("Cavalry") && !ent.hasClass("Ranged")
					&& target.hasClass("Female") && target.unitAIState().split(".")[1] == "FLEEING")
					maybeUpdate = true;
			}

			// don't update too soon if not necessary
			if (!needsUpdate)
			{
				if (!maybeUpdate)
					continue;
				var lastAttackPlanUpdateTime = ent.getMetadata(PlayerID, "lastAttackPlanUpdateTime");
				if (lastAttackPlanUpdateTime && (time - lastAttackPlanUpdateTime) < 5)
					continue;
			}
			ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
			var range = 60;
			var attackTypes = ent.attackTypes();
			if (attackTypes && attackTypes.indexOf("Ranged") !== -1)
				range = 30 + ent.attackRange("Ranged").max;
			else if (ent.hasClass("Cavalry"))
				range += 30;
			range = range * range;
			// let's filter targets further based on this unit.
			var entIndex = gameState.ai.accessibility.getAccessValue(ent.position());
			var mStruct = enemyStructures.filter(function (enemy) {
				if (!enemy.position() || (enemy.hasClass("StoneWall") && !ent.canAttackClass("StoneWall")))
					return false;
				if (API3.SquareVectorDistance(enemy.position(), ent.position()) > range)
					return false;
				if (siegeUnit && enemy.foundationProgress() === 0)
					return false;
				if (gameState.ai.accessibility.getAccessValue(enemy.position()) !== entIndex)
					return false;
				return true;
			});
			var nearby = (!ent.hasClass("Cavalry") && !ent.hasClass("Ranged"));
			var mUnit = enemyUnits.filter(function (enemy) {
				if (!enemy.position())
					return false;
				if (enemy.hasClass("Animal"))
					return false;
				if (nearby && enemy.hasClass("Female") && enemy.unitAIState().split(".")[1] == "FLEEING")
					return false;
				var dist = API3.SquareVectorDistance(enemy.position(), ent.position());
				if (dist > range)
					return false;
				if (gameState.ai.accessibility.getAccessValue(enemy.position()) !== entIndex)
					return false;
				enemy.setMetadata(PlayerID, "distance", Math.sqrt(dist));
				return true;
			});
			// Checking for gates if we're a siege unit.
			mUnit = mUnit.toEntityArray();
			mStruct = mStruct.toEntityArray();
			if (siegeUnit)
			{
				if (mStruct.length !== 0)
				{
					mStruct.sort(function (structa,structb)
					{
						var vala = structa.costSum();
						if (structa.hasClass("Gates") && ent.canAttackClass("StoneWall"))
							vala += 10000;
						else if (structa.getDefaultArrow() || structa.getArrowMultiplier())
							vala += 1000;
						else if (structa.hasClass("ConquestCritical"))
							vala += 200;
						var valb = structb.costSum();
						if (structb.hasClass("Gates") && ent.canAttackClass("StoneWall"))
							valb += 10000;
						else if (structb.getDefaultArrow() || structb.getArrowMultiplier())
							valb += 1000;
						else if (structb.hasClass("ConquestCritical"))
							valb += 200;
						return (valb - vala);
					});
					if (mStruct[0].hasClass("Gates"))
						ent.attack(mStruct[0].id());
					else
					{
						var rand = Math.floor(Math.random() * mStruct.length * 0.2);
						ent.attack(mStruct[rand].id());
					}
				}
				else
					ent.attackMove(self.targetPos[0], self.targetPos[1]);
			}
			else
			{
				if (mUnit.length !== 0)
				{
					mUnit.sort(function (unitA,unitB) {
						var vala = unitA.hasClass("Support") ? 50 : 0;
						if (ent.countersClasses(unitA.classes()))
							vala += 100;
						var valb = unitB.hasClass("Support") ? 50 : 0;
						if (ent.countersClasses(unitB.classes()))
							valb += 100;
						var distA = unitA.getMetadata(PlayerID, "distance");
						var distB = unitB.getMetadata(PlayerID, "distance");
						if( distA && distB)
						{
							vala -= distA;
							valb -= distB;
						}
						return valb - vala;
					});
					var rand = Math.floor(Math.random() * mUnit.length * 0.1);
					ent.attack(mUnit[rand].id());
				}
				else if (API3.SquareVectorDistance(self.targetPos, ent.position()) > 2500 )
					ent.attackMove(self.targetPos[0], self.targetPos[1]);
				else if (mStruct.length !== 0)
				{
					mStruct.sort(function (structa,structb) {
						var vala = structa.costSum();
						if (structa.hasClass("Gates") && ent.canAttackClass("StoneWall"))
							vala += 10000;
						else if (structa.hasClass("ConquestCritical"))
							vala += 100;
						var valb = structb.costSum();
						if (structb.hasClass("Gates") && ent.canAttackClass("StoneWall"))
							valb += 10000;
						else if (structb.hasClass("ConquestCritical"))
							valb += 100;
						return (valb - vala);
					});
					if (mStruct[0].hasClass("Gates"))
						ent.attack(mStruct[0].id());
					else
					{
						var rand = Math.floor(Math.random() * mStruct.length * 0.2);
						ent.attack(mStruct[rand].id());
					}
				}
			}
		}
		this.unitCollUpdateArray.splice(0, lgth);
		this.startingAttack = false;

		// updating targets.
		if (!this.target || !gameState.getEntityById(this.target.id()))
		{
			if (this.Config.debug > 0)
				API3.warn("Seems like our target has been destroyed. Switching.");
			this.target = this.getNearestTarget(gameState, this.position, true);
			if (!this.target)
			{
				if (this.Config.debug > 0)
					API3.warn("No new target found. Remaining units " + this.unitCollection.length);
				Engine.ProfileStop();
				return false;
			}
			this.targetPos = this.target.position();
		}
		
		// regularly update the target position in case it's a unit.
		if (this.target.hasClass("Unit"))
			this.targetPos = this.target.position();
	}
	this.lastPosition = this.position;
	Engine.ProfileStop();
	
	return this.unitCollection.length;
};

// reset any units
m.AttackPlan.prototype.Abort = function(gameState)
{
	// Do not use QuickIter with forEach when forEach removes elements
	this.unitCollection.preventQuickIter();
	if (this.unitCollection.length)
	{
		// If the attack was started, and we are on the same land as the rallyPoint, go back there
		var rallyPoint = this.rallyPoint;
		var withdrawal = (this.isStarted() && !this.overseas);
		var self = this;
		this.unitCollection.forEach(function(ent) {
			ent.stopMoving();
			if (withdrawal)
				ent.move(rallyPoint[0], rallyPoint[1]);
			self.removeUnit(ent);
		});
	}

	for (var unitCat in this.unitStat) {
		delete this.unitStat[unitCat];
		delete this.unit[unitCat];
	}
	delete this.unitCollection;
	gameState.ai.queueManager.removeQueue("plan_" + this.name);
	gameState.ai.queueManager.removeQueue("plan_" + this.name + "_champ");
	gameState.ai.queueManager.removeQueue("plan_" + this.name + "_siege");
};

m.AttackPlan.prototype.removeUnit = function(ent)
{
	if (ent.hasClass("CitizenSoldier") && ent.getMetadata(PlayerID, "role") !== "worker")
	{
		ent.setMetadata(PlayerID, "role", "worker");
		ent.setMetadata(PlayerID, "subrole", undefined);
	}
	ent.setMetadata(PlayerID, "plan", -1);
	this.unitCollection.updateEnt(ent);
};

m.AttackPlan.prototype.checkEvents = function(gameState, events)
{
	if (this.state === "unexecuted")
		return;
	var TrainingEvents = events["TrainingFinished"];
	for (var evt of TrainingEvents)
	{
		for (var id of evt.entities)
		{
			var ent = gameState.getEntityById(id);
			if (!ent || ent.getMetadata(PlayerID, "plan") === undefined)
				continue;
			if (ent.getMetadata(PlayerID, "plan") === this.name)
				ent.setMetadata(PlayerID, "plan", -1);
		}
	}
};

m.AttackPlan.prototype.waitingForTransport = function()
{
	var waiting = false;
	this.unitCollection.forEach(function (ent) {
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			waiting = true;
	});
	return waiting;
};

m.AttackPlan.prototype.hasForceOrder = function(data, value)
{
	var forced = false;
	this.unitCollection.forEach(function (ent) {
		if (data && +(ent.getMetadata(PlayerID, data)) !== value)
			return;
		var orders = ent.unitAIOrderData();
		for (var order of orders)
			if (order.force)
				forced = true;
	});
	return forced;
};

m.AttackPlan.prototype.isSiegeUnit = function(gameState, ent)
{
	return (ent.hasClass("Siege") ||
		(gameState.civ() === "maur" && ent.hasClass("Elephant") && ent.hasClass("Champion")));
};

m.AttackPlan.prototype.debugAttack = function()
{
	API3.warn("---------- attack " + this.name);
	for (var unitCat in this.unitStat)
	{
		var Unit = this.unitStat[unitCat];
		API3.warn(unitCat + " num=" + this.unit[unitCat].length + " min=" + Unit["minSize"] + " need=" + Unit["targetSize"]);
	}
	API3.warn("------------------------------");
};

return m;
}(PETRA);
