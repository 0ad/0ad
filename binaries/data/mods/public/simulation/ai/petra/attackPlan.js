var PETRA = function(m)
{

/* This is an attack plan:
 * It deals with everything in an attack, from picking a target to picking a path to it
 * To making sure units are built, and pushing elements to the queue manager otherwise
 * It also handles the actual attack, though much work is needed on that.
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
		this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
	}

	if (this.targetPlayer === undefined)
	{
		this.failed = true;
		return false;
	}
	
	// get a starting rallyPoint ... will be improved later
	var rallyPoint = undefined;
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (!base.anchor || !base.anchor.position())
			continue;
		rallyPoint = base.anchor.position();
		break;
	}
	if (!rallyPoint)	// no base ?  take the position of any of our entities
	{
		for (let ent of gameState.getOwnEntities().values())
		{
			if (!ent.position())
				continue;
			rallyPoint = ent.position();
			break;
		}
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

	// unitStat priority is relative. If all are 0, the only relevant criteria is "currentsize/targetsize".
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
		this.unitStat["MeleeInfantry"]     = { "priority": 0.7, "minSize": 5, "targetSize": 15, "batchSize": 5, "classes": ["Infantry","Melee", "CitizenSoldier"],
			"interests": [ ["strength",3], ["cost",1] ] };
		this.unitStat["ChampRangedInfantry"] = { "priority": 1, "minSize": 5, "targetSize": 25, "batchSize": 5, "classes": ["Infantry","Ranged", "Champion"],
			"interests": [["strength",3], ["cost",1] ] };
		this.unitStat["ChampMeleeInfantry"]  = { "priority": 1, "minSize": 5, "targetSize": 20, "batchSize": 5, "classes": ["Infantry","Melee", "Champion"],
			"interests": [ ["strength",3], ["cost",1] ] };
		this.unitStat["MeleeCavalry"]      = { "priority": 0.7, "minSize": 3, "targetSize": 15, "batchSize": 3, "classes": ["Cavalry","Melee", "CitizenSoldier"],
			"interests": [ ["strength",2], ["cost",1] ] };
		this.unitStat["RangedCavalry"]     = { "priority": 0.7, "minSize": 3, "targetSize": 15, "batchSize": 3, "classes": ["Cavalry","Ranged", "CitizenSoldier"],
			"interests": [ ["strength",2], ["cost",1] ] };
		this.unitStat["ChampMeleeInfantry"]  = { "priority": 1, "minSize": 3, "targetSize": 18, "batchSize": 3, "classes": ["Infantry","Melee", "Champion"],
			"interests": [ ["strength",3], ["cost",1] ] };
		this.unitStat["ChampMeleeCavalry"]   = { "priority": 1, "minSize": 3, "targetSize": 18, "batchSize": 3, "classes": ["Cavalry","Melee", "Champion"],
			"interests": [ ["strength",2], ["cost",1] ] };
		this.unitStat["Hero"]                = { "priority": 1, "minSize": 0, "targetSize":  1, "batchSize": 1, "classes": ["Hero"],
			"interests": [ ["strength",2], ["cost",1] ] };
		this.neededShips = 5;
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
	// and lower priority and smaller sizes for easier difficulty levels
	if (this.Config.difficulty < 2)
	{
		priority *= 0.6;
		variation *= 0.6;
	}
	else if (this.Config.difficulty < 3)
	{
		priority *= 0.8;
		variation *= 0.8;
	}
	for (let cat in this.unitStat)
	{
		this.unitStat[cat]["targetSize"] = Math.round(variation * this.unitStat[cat]["targetSize"]);
		this.unitStat[cat]["minSize"] = Math.min(this.unitStat[cat]["minSize"], this.unitStat[cat]["targetSize"]);
	}

	// change the sizes according to max population
	this.neededShips = Math.ceil(this.Config.popScaling * this.neededShips);
	for (let cat in this.unitStat)
	{
		this.unitStat[cat]["targetSize"] = Math.round(this.Config.popScaling * this.unitStat[cat]["targetSize"]);
		this.unitStat[cat]["minSize"] = Math.floor(this.Config.popScaling * this.unitStat[cat]["minSize"]);
	}

	// TODO: there should probably be one queue per type of training building
	gameState.ai.queueManager.addQueue("plan_" + this.name, priority);
	gameState.ai.queueManager.addQueue("plan_" + this.name +"_champ", priority+1);
	gameState.ai.queueManager.addQueue("plan_" + this.name +"_siege", priority);
	
	// each array is [ratio, [associated classes], associated EntityColl, associated unitStat, name ]
	this.buildOrder = [];
	this.canBuildUnits = gameState.ai.HQ.canBuildUnits;

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

m.AttackPlan.prototype.init = function(gameState)
{
	this.queue = gameState.ai.queues["plan_" + this.name];
	this.queueChamp = gameState.ai.queues["plan_" + this.name +"_champ"];
	this.queueSiege = gameState.ai.queues["plan_" + this.name +"_siege"];

	this.unitCollection = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "plan", this.name));
	this.unitCollection.registerUpdates();
	
	this.unit = {};

	// defining the entity collections. Will look for units I own, that are part of this plan.
	// Also defining the buildOrders.
	for (var cat in this.unitStat)
	{
		var Unit = this.unitStat[cat];
		this.unit[cat] = this.unitCollection.filter(API3.Filters.byClassesAnd(Unit["classes"]));
		this.unit[cat].registerUpdates();
		if (this.canBuildUnits)
			this.buildOrder.push([0, Unit["classes"], this.unit[cat], Unit, cat]);
	}
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

// Returns true if the attack can be executed at the current time
// Basically it checks we have enough units.
m.AttackPlan.prototype.canStart = function()
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

m.AttackPlan.prototype.mustStart = function()
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
		if (this.type === "Raid" && this.target && this.target.foundationProgress() && this.target.foundationProgress() > 60)
			return true;
	}
	return false;
};

m.AttackPlan.prototype.forceStart = function()
{
	for (let unitCat in this.unitStat)
	{
		let Unit = this.unitStat[unitCat];
		Unit["targetSize"] = 0;
		Unit["minSize"] = 0;
	}
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
	if (this.Config.difficulty < 2)
		stat["targetSize"] = 1;
	else if (this.Config.difficulty < 3)
		stat["targetSize"] = 2;
        stat["targetSize"] = Math.round(this.Config.popScaling * stat["targetSize"]);
	this.addBuildOrder(gameState, "Siege", stat, true);
	return true;
};

// Three returns possible: 1 is "keep going", 0 is "failed plan", 2 is "start"
// 3 is a special case: no valid path returned. Right now I stop attacking alltogether.
m.AttackPlan.prototype.updatePreparation = function(gameState)
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

	if (this.Config.debug > 3 && gameState.ai.playedTurn % 50 == 0)
		this.debugAttack();

	// find our target (if not yet done or because our previous one was destroyed)
	if (!this.target || !gameState.getEntityById(this.target.id()))
	{
		this.target = this.getNearestTarget(gameState, this.rallyPoint);
		if (!this.target)
		{
			// may-be all our previous enemey targets have been destroyed ?
			this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
			if (this.targetPlayer !== undefined)
				this.target = this.getNearestTarget(gameState, this.rallyPoint);
			if (!this.target)
				return 0;
		}
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
			for (var base of gameState.ai.HQ.baseManagers)
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
		// reset the path so that we recompute it for this new target
		this.path = undefined;
		if (!this.pathFinder)
		{
			this.pathFinder = new API3.aStarPath(gameState, false, false, this.targetPlayer);
			this.pathWidth = 6;
			this.pathSampling = 2;
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
	if (this.type !== "Raid" && gameState.ai.HQ.attackManager.getAttackInPreparation("Raid") !== undefined)
		this.reassignCavUnit(gameState);    // reassign some cav (if any) to fasten raid preparations

	// special case: if we've reached max pop, and we can start the plan, start it.
	if (gameState.getPopulationMax() - gameState.getPopulation() < 5)
	{
		let lengthMin = 16;
		if (gameState.getPopulationMax() < 300)
			lengthMin -= Math.floor(8 * (300 - gameState.getPopulationMax()) / 300);
		if (this.canStart() || this.unitCollection.length > lengthMin)
		{
			this.queue.empty();
			this.queueChamp.empty();
			this.queueSiege.empty();
		}
		else	// Abort the plan so that its units will be reassigned to other plans.
		{
			if (this.Config.debug > 1)
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
	else if (this.mustStart() && gameState.countOwnQueuedEntitiesWithMetadata("plan", +this.name) > 0)
	{
		// keep on while the units finish being trained, then we'll start
		this.queue.empty();
		this.queueChamp.empty();
		this.queueSiege.empty();
		return 1;
	}
	else if (!this.mustStart())
	{
		if (this.canBuildUnits)
		{
			// We still have time left to recruit units and do stuffs.
			if (!this.unitStat["Siege"])
			{
				var numSiegeBuilder = 0;
				if (gameState.civ() !== "mace" && gameState.civ() !== "maur")
					numSiegeBuilder += gameState.getOwnEntitiesByClass("Fortress", true).filter(API3.Filters.isBuilt()).length;
				if (gameState.civ() === "mace" || gameState.civ() === "maur" || gameState.civ() === "rome")
					numSiegeBuilder += gameState.countEntitiesByType(gameState.ai.HQ.bAdvanced[0], true);
				if (numSiegeBuilder > 0)
					this.addSiegeUnits(gameState);
			}
			this.trainMoreUnits(gameState);
			// may happen if we have no more training facilities and build orders are canceled
			if (this.buildOrder.length == 0)
				return 0;	// will abort the plan
		}
		return 1;
	}

	// if we're here, it means we must start
	this.state = "completing";
	if (this.type === "Raid")
		this.maxCompletingTurn = gameState.ai.playedTurn + 20;
	else
	{
		this.maxCompletingTurn = gameState.ai.playedTurn + 60;
		// warn our allies so that they can help if possible
		if (!this.requested)
			Engine.PostCommand(PlayerID, {"type": "attack-request", "source": PlayerID, "target": this.targetPlayer});
	}

	var rallyPoint = this.rallyPoint;
	var rallyIndex = gameState.ai.accessibility.getAccessValue(rallyPoint);
	for (var entity of this.unitCollection.values())
	{
		// For the time being, if occupied in a transport, remove the unit from this plan   TODO improve that
		if (entity.getMetadata(PlayerID, "transport") !== undefined || entity.getMetadata(PlayerID, "transporter") !== undefined)
		{
			entity.setMetadata(PlayerID, "plan", -1);
			continue;
		}
		entity.setMetadata(PlayerID, "role", "attack");
		entity.setMetadata(PlayerID, "subrole", "completing");
		var queued = false;
		if (entity.resourceCarrying() && entity.resourceCarrying().length)
			queued = m.returnResources(entity, gameState);
		var index = gameState.ai.accessibility.getAccessValue(entity.position());
		if (index === rallyIndex)
			entity.moveToRange(rallyPoint[0], rallyPoint[1], 0, 15, queued);
		else
			gameState.ai.HQ.navalManager.requireTransport(gameState, entity, index, rallyIndex, rallyPoint);
	}

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

	if (this.Config.debug > 1 && gameState.ai.playedTurn%50 == 0)
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
		else if (this.buildOrder[0][3]["classes"].indexOf("Hero") !== -1)
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
				if (this.Config.debug > 1)
					API3.warn("attack no template found " + this.buildOrder[0][1]);
				delete this.unitStat[this.buildOrder[0][4]];	// deleting the associated unitstat.
				this.buildOrder.splice(0,1);
			}
			else
			{
				if (this.Config.debug > 2)
					API3.warn("attack template " + template + " added for plan " + this.name);
				var max = this.buildOrder[0][3]["batchSize"];
				var specialData = "Plan_" + this.name + "_" + this.buildOrder[0][4];
				if (gameState.getTemplate(template).hasClass("CitizenSoldier"))
					var trainingPlan = new m.TrainingPlan(gameState, template, { "role": "worker", "plan": this.name, "special": specialData, "base": 0 }, max, max);
				else
					var trainingPlan = new m.TrainingPlan(gameState, template, { "role": "attack", "plan": this.name, "special": specialData, "base": 0 }, max, max);
				if (trainingPlan.template)
					queue.addItem(trainingPlan);
				else if (this.Config.debug > 1)
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
	// If we can not build units, assign all available except those affected to allied defense to the current attack
	if (!this.canBuildUnits)
	{
		for (let ent of gameState.getOwnUnits().values())
		{
			if (!ent.position())
				continue;
			if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
				continue;
			if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
				continue;
			if (ent.getMetadata(PlayerID, "allied"))
				continue;
			ent.setMetadata(PlayerID, "plan", plan);
			this.unitCollection.updateEnt(ent);
			added = true; 
		}
		return added;
	}

	if (this.type === "Raid")
	{
		// Raid are fast cavalry attack: assign all cav except some for hunting
		var num = 0;
		for (let ent of gameState.getOwnUnits().values())
		{
			if (!ent.hasClass("Cavalry"))
				continue;
			if (!ent.position())
				continue;
			if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
				continue;
			if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
				continue;
			if (num++ < 2)
				continue;
			ent.setMetadata(PlayerID, "plan", plan);
			this.unitCollection.updateEnt(ent);
			added = true;
		}
		return added;
	}

	// Assign all units without specific role
	for (let ent of gameState.getOwnEntitiesByRole(undefined, true).values())
	{
		if (!ent.hasClass("Unit"))
			continue;
		if (!ent.position())
			continue;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
			continue;
		if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
			continue;
		if (ent.hasClass("Ship") || ent.hasClass("Support") || ent.attackTypes() === undefined)
			continue;
		ent.setMetadata(PlayerID, "plan", plan);
		this.unitCollection.updateEnt(ent);
		added = true;
	}
	// Add units previously in a plan, but which left it because needed for defense or attack finished
	for (let ent of gameState.ai.HQ.attackManager.outOfPlan.values())
	{
		if (!ent.position())
			continue;
		if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
			continue;
		ent.setMetadata(PlayerID, "plan", plan);
		this.unitCollection.updateEnt(ent);
		added = true;
	}

	// Finally add also some workers,
	// If Rush, assign all kind of workers, keeping a minimum number of defenders
	// Otherwise, assign only idle workers if too much of them
	let worker = gameState.getOwnEntitiesByRole("worker", true);
	let num = 0;
	let numbase = {};
	for (let ent of worker.values())
	{
		if (!ent.position())
			continue;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
			continue;
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			continue;
		if (!ent.hasClass("CitizenSoldier"))
			continue;
		let baseID = ent.getMetadata(PlayerID, "base");
		if (baseID)
			numbase[baseID] = numbase[baseID] ? ++numbase[baseID] : 1;
		else
		{
			API3.warn("Petra problem ent without base ");
			m.dumpEntity(ent);
			continue;
		}
		if (this.type !== "Rush" && ent.getMetadata(PlayerID, "subrole") !== "idle")
			continue;
		if (num++ < 9 || numbase[baseID] < 5)
			continue;
		ent.setMetadata(PlayerID, "plan", plan);
		this.unitCollection.updateEnt(ent);
		added = true;
	}
	return added;
};

// Reassign one (at each turn) Cav unit to fasten raid preparation
m.AttackPlan.prototype.reassignCavUnit = function(gameState)
{
	var found = undefined;
	for (var ent of this.unitCollection.values())
	{
		if (!ent.position() || ent.getMetadata(PlayerID, "transport") !== undefined)
			continue;
		if (!ent.hasClass("Cavalry") || !ent.hasClass("CitizenSoldier"))
			continue;
		found = ent;
		break;
	}
	if (!found)
		return;
	let raid = gameState.ai.HQ.attackManager.getAttackInPreparation("Raid");
	found.setMetadata(PlayerID, "plan", raid.name);
	this.unitCollection.updateEnt(found);
	raid.unitCollection.updateEnt(found);
};

// sameLand true means that we look for a target for which we do not need to take a transport
m.AttackPlan.prototype.getNearestTarget = function(gameState, position, sameLand)
{
	if (this.type === "Raid")
		var targets = this.raidTargetFinder(gameState);
	else if (this.type === "Rush" || this.type === "Attack")
		var targets = this.rushTargetFinder(gameState, this.targetPlayer);
	else
		var targets = this.defaultTargetFinder(gameState, this.targetPlayer);
	if (targets.length == 0)
		return undefined;

	var land = gameState.ai.accessibility.getAccessValue(position);

	// picking the nearest target
	var minDist = -1;
	var target = undefined;
	for (var ent of targets.values())
	{
		if (!ent.position())
			continue;
		if (sameLand && gameState.ai.accessibility.getAccessValue(ent.position()) != land)
			continue;
		var dist = API3.SquareVectorDistance(ent.position(), position);
		if (dist < minDist || minDist == -1)
		{
			minDist = dist;
			target = ent;
		}
	}
	if (!target)
		return undefined;
	// Rushes can change their enemy target if nothing found with the preferred enemy
	this.targetPlayer = target.owner();
	return target;
};

// Default target finder aims for conquest critical targets
m.AttackPlan.prototype.defaultTargetFinder = function(gameState, playerEnemy)
{
	var targets = undefined;
	if (gameState.getGameType() === "wonder")
	{
		targets = gameState.getEnemyStructures(playerEnemy).filter(API3.Filters.byClass("Wonder"));
		if (targets.length)
			return targets;
	}

	var targets = gameState.getEnemyStructures(playerEnemy).filter(API3.Filters.byClass("CivCentre"));
	if (!targets.length)
		targets = gameState.getEnemyStructures(playerEnemy).filter(API3.Filters.byClass("ConquestCritical"));
	// If there's nothing, attack anything else that's less critical
	if (!targets.length)
		targets = gameState.getEnemyStructures(playerEnemy).filter(API3.Filters.byClass("Town"));
	if (!targets.length)
		targets = gameState.getEnemyStructures(playerEnemy).filter(API3.Filters.byClass("Village"));
	// no buildings, attack anything conquest critical, even units
	if (!targets.length)
		targets = gameState.getEnemyEntities(playerEnemy).filter(API3.Filters.byClass("ConquestCritical"));
	return targets;
};

// Rush target finder aims at isolated non-defended buildings
m.AttackPlan.prototype.rushTargetFinder = function(gameState, playerEnemy)
{
	var targets = new API3.EntityCollection(gameState.sharedScript);
	if (playerEnemy !== undefined)
		var buildings = gameState.getEnemyStructures(playerEnemy).toEntityArray();
	else
		var buildings = gameState.getEnemyStructures().toEntityArray();
	if (buildings.length == 0)
		return targets;

	this.position = this.unitCollection.getCentrePosition();
	if (!this.position)
		this.position = this.rallyPoint;

	var minDist = Math.min();
	var target = undefined;
	for (var building of buildings)
	{
		if (building.owner() == 0)
			continue;
		if (building.getDefaultArrow() || building.getArrowMultiplier())
			continue;
		var pos = building.position();
		var defended = false;
		for (var defense of buildings)
		{
			if (!building.getDefaultArrow() && !building.getArrowMultiplier())
				continue;
			var dist = API3.SquareVectorDistance(pos, defense.position());
			if (dist < 6400)   // TODO check on defense range rather than this fixed 80*80
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

	if (targets.length == 0)
	{
		if (this.type === "Attack")
			targets = this.defaultTargetFinder(gameState, playerEnemy);
		else if (this.type === "Rush" && playerEnemy)
			targets = this.rushTargetFinder(gameState);
	}

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
	if (this.Config.debug > 1)
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
		
		for (var ent of this.unitCollection.values())
			ent.setMetadata(PlayerID, "subrole", "walking");
		this.unitCollection.setStance("aggressive");

		if (gameState.ai.accessibility.getAccessValue(this.targetPos) === gameState.ai.accessibility.getAccessValue(this.rallyPoint))
		{
			if (!this.path[0][0][0] || !this.path[0][0][1])
			{
				if (this.Config.debug > 1)
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
			for (var ent of this.unitCollection.values())
				gameState.ai.HQ.navalManager.requireTransport(gameState, ent, startIndex, endIndex, endPos);
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
	if (this.unitCollection.length == 0)
		return 0;

	Engine.ProfileStart("Update Attack");

	this.position = this.unitCollection.getCentrePosition();
	var IDs = this.unitCollection.toIdArray();

	var self = this;

	// we are transporting our units, let's wait
	// TODO instead of state "arrived", made a state "walking" with a new path
	if (this.state === "transporting")
	{
		var done = true;
		for (var ent of this.unitCollection.values())
		{
			if (this.Config.debug > 1 && ent.getMetadata(PlayerID, "transport") !== undefined)
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [2,2,0]});
			else if (this.Config.debug > 1)
				Engine.PostCommand(PlayerID,{"type": "set-shading-color", "entities": [ent.id()], "rgb": [1,1,1]});
			if (!done)
				continue;
			if (ent.getMetadata(PlayerID, "transport") !== undefined)
				done = false;
		}

		if (done)
			this.state = "arrived";
		else
		{
			// if we are attacked while waiting the rest of the army, retaliate
			var attackedEvents = events["Attacked"];
			for (var evt of attackedEvents)
			{
				if (IDs.indexOf(evt.target) == -1)
					continue;
				var attacker = gameState.getEntityById(evt.attacker);
				var ourUnit = gameState.getEntityById(evt.target);
				if (!attacker || !ourUnit)
					continue;
				for (var ent of this.unitCollection.values())
				{
					if (ent.getMetadata(PlayerID, "transport") !== undefined)
						continue;
					if (!ent.isIdle())
						continue;
					ent.attack(attacker.id());
				}
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
		var attackedUnitNB = 0;
		var attackedEvents = events["Attacked"];
		for (var evt of attackedEvents)
		{
			if (IDs.indexOf(evt.target) == -1)
				continue;
			var attacker = gameState.getEntityById(evt.attacker);
			var ourUnit = gameState.getEntityById(evt.target);

			if (attacker && (attacker.owner() != 0 || this.targetPlayer == 0))
			{
				attackedNB++;
				if (attacker.hasClass("Unit"))
					attackedUnitNB++;
			}
		}
		// Are we arrived at destination ?
		var maybe = true;
		if (attackedUnitNB == 0)
		{
			var siegeNB = 0;
			for (var ent of this.unitCollection.values())
				if (this.isSiegeUnit(gameState, ent))
					siegeNB++;
			if (siegeNB == 0)
				maybe = false;
		}
		if (maybe && ((gameState.ai.HQ.territoryMap.getOwner(this.position) === this.targetPlayer && attackedNB > 1) || attackedNB > 3))
			this.state = "arrived";
	}

	if (this.state === "walking")
	{
		// basically haven't moved an inch: very likely stuck)
		if (API3.SquareVectorDistance(this.position, this.position5TurnsAgo) < 10 && this.path.length > 0 && gameState.ai.playedTurn % 5 == 0)
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
		if (gameState.ai.playedTurn % 5 == 0)
			this.position5TurnsAgo = this.position;
		
		if (this.lastPosition && API3.SquareVectorDistance(this.position, this.lastPosition) < 20 && this.path.length > 0)
		{
			if (!this.path[0][0][0] || !this.path[0][0][1])
				API3.warn("Start: Problem with path " + uneval(this.path));
			// We're stuck, presumably. Check if there are no walls just close to us. If so, we're arrived, and we're gonna tear down some serious stone.
			var nexttoWalls = false;
			gameState.getEnemyStructures().filter(API3.Filters.byClass("StoneWall")).forEach( function (ent) {
				if (!nexttoWalls && API3.SquareVectorDistance(self.position, ent.position()) < 800)
					nexttoWalls = true;
			});
			// there are walls but we can attack
			if (nexttoWalls && this.unitCollection.filter(API3.Filters.byCanAttack("StoneWall")).length !== 0)
			{
				if (this.Config.debug > 1)
					API3.warn("Attack Plan " + this.type + " " + this.name + " has met walls and is not happy.");
				this.state = "arrived";
			}
			else if (nexttoWalls)	// abort plan
			{
				if (this.Config.debug > 1)
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
			if (this.Config.debug > 1)
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
				if (this.Config.debug > 1)
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
			var newtarget = this.getNearestTarget(gameState, this.position);
			if (newtarget)
			{
				this.target = newtarget;
				this.targetPos = this.target.position();
			}
		}
	}
	
	// basic state of attacking.
	if (this.state === "")
	{
		// First update the target if needed:
		if (this.target && this.target.owner() === 0 && this.targetPlayer !== 0)  // this enemy has resigned
			this.target = undefined;
		if (!this.target || !gameState.getEntityById(this.target.id()))
		{
			if (this.Config.debug > 1)
				API3.warn("Seems like our target has been destroyed. Switching.");
			this.target = this.getNearestTarget(gameState, this.position, true);
			if (!this.target)
			{
				// Check if we could help any current attack
				var attackManager = gameState.ai.HQ.attackManager;
				var accessIndex = gameState.ai.accessibility.getAccessValue(this.position);
				for (let attackType in attackManager.startedAttacks)
				{
					if (this.target)
						break;
					for (let attack of attackManager.startedAttacks[attackType])
					{
						if (attack.name === this.name)
							continue;
						if (!attack.target || !gameState.getEntityById(attack.target.id()))
							continue;
						if (accessIndex !== gameState.ai.accessibility.getAccessValue(attack.targetPos))
							continue;
						if (attack.target.owner() === 0 && attack.targetPlayer !== 0)	// looks like it has resigned     
							continue;
						this.target = attack.target;
						this.targetPlayer = attack.targetPlayer;
						break;
					}
				}

				// If not, let's look for another enemy
				if (!this.target)
				{
					this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
					if (this.targetPlayer !== undefined)
						this.target = this.getNearestTarget(gameState, this.position, true);
					if (!this.target)
					{
						if (this.Config.debug > 1)
							API3.warn("No new target found. Remaining units " + this.unitCollection.length);
						Engine.ProfileStop();
						return false;
					}
				}
				if (this.Config.debug > 1)
					API3.warn("We will help one of our other attacks");
			}
			this.targetPos = this.target.position();
		}
		// and regularly update the target position in case it's a unit.
		if (this.target.hasClass("Unit"))
			this.targetPos = this.target.position();

		var time = gameState.ai.elapsedTime;
		var attackedEvents = events["Attacked"];
		for (var evt of attackedEvents)
		{
			if (IDs.indexOf(evt.target) == -1)
				continue;
			var attacker = gameState.getEntityById(evt.attacker);
			if (!attacker || !attacker.position() || !attacker.hasClass("Unit"))
				continue;
			var ourUnit = gameState.getEntityById(evt.target);
			if (this.isSiegeUnit(gameState, ourUnit))
			{	// if our siege units are attacked, we'll send some units to deal with enemies.
				var collec = this.unitCollection.filter(API3.Filters.not(API3.Filters.byClass("Siege"))).filterNearest(ourUnit.position(), 5);
				for (var ent of collec.values())
				{
					if (this.isSiegeUnit(gameState, ent))	// needed as mauryan elephants are not filtered out
						continue;
					ent.attack(attacker.id());
					ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
				}
				// And if this attacker is a non-ranged siege unit and our unit also, attack it
				if (this.isSiegeUnit(gameState, attacker) && attacker.hasClass("Melee") && ourUnit.hasClass("Melee"))
				{
					ourUnit.attack(attacker.id());
					ourUnit.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
				}
			}
			else
			{
				if (this.isSiegeUnit(gameState, attacker))
				{	// if our unit is attacked by a siege unit, we'll send some melee units to help it.
					var collec = this.unitCollection.filter(API3.Filters.byClass("Melee")).filterNearest(ourUnit.position(), 5);
					for (var ent of collec.values())
					{
						ent.attack(attacker.id());
						ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
					}
				}
				else
				{	// if units are attacked, abandon their target (if it was a structure or a support) and retaliate
					// also if our unit is attacking a range unit and the attacker is a melee unit, retaliate
					var orderData = ourUnit.unitAIOrderData();
					if (orderData.length !== 0 && orderData[0]["target"])
					{
						var target = gameState.getEntityById(orderData[0]["target"]);
						if (target && !target.hasClass("Structure") && !target.hasClass("Support"))
						{
							if (!target.hasClass("Ranged") || !attacker.hasClass("Melee"))
								continue;
						}
					}
					ourUnit.attack(attacker.id());
					ourUnit.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
				}
			}
		}
		
		var enemyUnits = gameState.getEnemyUnits(this.targetPlayer);
		var enemyStructures = gameState.getEnemyStructures(this.targetPlayer);

		// Count the number of times an enemy is targeted, to prevent all units to follow the same target
		let unitTargets = {};
		for (let ent of this.unitCollection.values())
		{
			if (ent.hasClass("Ship"))	// TODO What to do with ships
				continue;
			let orderData = ent.unitAIOrderData();
			if (!orderData || !orderData.length || !orderData[0]["target"])
				continue;
			let targetId = orderData[0]["target"];
			let target = gameState.getEntityById(targetId);
			if (!target || target.hasClass("Structure"))
				continue;
			if (!(targetId in unitTargets))
			{
				if (this.isSiegeUnit(gameState, target) || target.hasClass("Hero"))
					unitTargets[targetId] = -8;
				else if (target.hasClass("Champion") || target.hasClass("Ship"))
					unitTargets[targetId] = -5;
				else
					unitTargets[targetId] = -3;
			}
			++unitTargets[targetId];
		}
		let veto = {};
		for (let target in unitTargets)
			if (unitTargets[target] > 0)
				veto[target] = true;

		var targetClassesUnit;
		var targetClassesSiege;
		if (this.type === "Rush")
			targetClassesUnit = {"attack": ["Unit", "Structure"], "avoid": ["StoneWall", "Tower", "Fortress"], "vetoEntities": veto};
		else
		{
			if (this.target.hasClass("Fortress"))
				targetClassesUnit = {"attack": ["Unit", "Structure"], "avoid": ["StoneWall"], "vetoEntities": veto};
			else if (this.target.hasClass("StoneWall"))
				targetClassesUnit = {"attack": ["Unit", "Structure"], "avoid": ["Fortress"], "vetoEntities": veto};
			else
				targetClassesUnit = {"attack": ["Unit", "Structure"], "avoid": ["Fortress", "StoneWall"], "vetoEntities": veto};
		}
		if (this.target.hasClass("Structure"))
			targetClassesSiege = {"attack": ["Structure"], "avoid": [], "vetoEntities": veto};
		else
			targetClassesSiege = {"attack": ["Unit", "Structure"], "avoid": [], "vetoEntities": veto};

		// do not loose time destroying buildings which do not help enemy's defense and can be easily captured later
		if (this.target.getDefaultArrow() || this.target.getArrowMultiplier())
		{
			targetClassesUnit.avoid = targetClassesUnit.avoid.concat("House", "Storehouse", "Farmstead", "Field", "Blacksmith");
			targetClassesSiege.avoid = targetClassesSiege.avoid.concat("House", "Storehouse", "Farmstead", "Field", "Blacksmith");
		}

		if (this.unitCollUpdateArray === undefined || this.unitCollUpdateArray.length == 0)
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

			let targetId = undefined;
			let orderData = ent.unitAIOrderData();
			if (orderData && orderData.length && orderData[0]["target"])
				targetId = orderData[0]["target"];
	
			// update the order if needed
			var needsUpdate = false;
			var maybeUpdate = false;
			var siegeUnit = this.isSiegeUnit(gameState, ent);
			if (ent.isIdle())
				needsUpdate = true;
			else if (siegeUnit && targetId)
			{
				var target = gameState.getEntityById(targetId);
				if (!target)
					needsUpdate = true;
				else if (unitTargets[targetId] && unitTargets[targetId] > 0)
				{
					needsUpdate = true;
					--unitTargets[targetId];
				}
				else if (!target.hasClass("Structure"))
					maybeUpdate = true;
			}
			else if (targetId)
			{
				var target = gameState.getEntityById(targetId);
				if (!target)
					needsUpdate = true;
				else if (unitTargets[targetId] && unitTargets[targetId] > 0)
				{
					needsUpdate = true;
					--unitTargets[targetId];
				}
				else if (target.hasClass("Structure") || (target.hasClass("Ship") && !ent.hasClass("Ship")))
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
				let deltat = (ent.unitAIState() === "INDIVIDUAL.COMBAT.APPROACHING") ? 10 : 5;
				var lastAttackPlanUpdateTime = ent.getMetadata(PlayerID, "lastAttackPlanUpdateTime");
				if (lastAttackPlanUpdateTime && (time - lastAttackPlanUpdateTime) < deltat)
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
			var entIndex = gameState.ai.accessibility.getAccessValue(ent.position());
			// Checking for gates if we're a siege unit.
			if (siegeUnit)
			{
				var mStruct = enemyStructures.filter(function (enemy) {
					if (!enemy.position() || (enemy.hasClass("StoneWall") && !ent.canAttackClass("StoneWall")))
						return false;
					if (API3.SquareVectorDistance(enemy.position(), ent.position()) > range)
						return false;
					if (enemy.foundationProgress() == 0)
						return false;
					if (gameState.ai.accessibility.getAccessValue(enemy.position()) !== entIndex)
						return false;
					return true;
				}).toEntityArray();
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
				{
					if (!ent.hasClass("Ranged"))
					{
						let targetClasses = {"attack": targetClassesSiege.attack, "avoid": targetClassesSiege.avoid.concat("Ship"), "vetoEntities": veto};
						ent.attackMove(this.targetPos[0], this.targetPos[1], targetClasses);
					}
					else
						ent.attackMove(this.targetPos[0], this.targetPos[1], targetClassesSiege);
				}
			}
			else
			{
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
					// if already too much units targeting this enemy, let's continue towards our main target
					if (veto[enemy.id()] && API3.SquareVectorDistance(self.targetPos, ent.position()) > 2500)
						return false;
					enemy.setMetadata(PlayerID, "distance", Math.sqrt(dist));
					return true;
				}).toEntityArray();
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
						if (veto[unitA.id()])
							vala -= 20000;
						if (veto[unitB.id()])
							valb -= 20000;
						return valb - vala;
					});
					var rand = Math.floor(Math.random() * mUnit.length * 0.1);
					ent.attack(mUnit[rand].id());
				}
				else if (API3.SquareVectorDistance(this.targetPos, ent.position()) > 2500 )
				{
					let targetClasses = targetClassesUnit;
					if (maybeUpdate && ent.unitAIState() === "INDIVIDUAL.COMBAT.APPROACHING")	// we may be blocked by walls, attack everything
					{
						if (!ent.hasClass("Ranged") && !ent.hasClass("Ship"))
							targetClasses = {"attack": ["Unit", "Structure"], "avoid": ["Ship"], "vetoEntities": veto};
						else
							targetClasses = {"attack": ["Unit", "Structure"], "vetoEntities": veto};
					}
					else if (!ent.hasClass("Ranged") && !ent.hasClass("Ship"))
						targetClasses = {"attack": targetClassesUnit.attack, "avoid": targetClassesUnit.avoid.concat("Ship"), "vetoEntities": veto};
					ent.attackMove(this.targetPos[0], this.targetPos[1], targetClasses);
				}
				else
				{
					var mStruct = enemyStructures.filter(function (enemy) {
						if (!enemy.position() || (enemy.hasClass("StoneWall") && !ent.canAttackClass("StoneWall")))
							return false;
						if (API3.SquareVectorDistance(enemy.position(), ent.position()) > range)
							return false;
						if (gameState.ai.accessibility.getAccessValue(enemy.position()) !== entIndex)
							return false;
						return true;
					}).toEntityArray();
					if (mStruct.length !== 0)
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
					else if (needsUpdate)  // really nothing   let's try to help our nearest unit
					{
						var distmin = Math.min();
						var attackerId = undefined;
						this.unitCollection.forEach( function (unit) {
							if (!unit.position())
								return;
							if (unit.unitAIState().split(".")[1] !== "COMBAT" || unit.unitAIOrderData().length == 0
								|| !unit.unitAIOrderData()[0]["target"])
								return;
							var dist = API3.SquareVectorDistance(unit.position(), ent.position());
							if (dist > distmin)
								return;
							distmin = dist;
							attackerId = unit.unitAIOrderData()[0]["target"];

						});
						if (attackerId)
							ent.attack(attackerId);
					}
				}
			}
		}
		this.unitCollUpdateArray.splice(0, lgth);
		this.startingAttack = false;

		// check if this enemy has resigned
		if (this.target && this.target.owner() === 0 && this.targetPlayer !== 0)
			this.target = undefined;
	}
	this.lastPosition = this.position;
	Engine.ProfileStop();
	
	return this.unitCollection.length;
};

// reset any units
m.AttackPlan.prototype.Abort = function(gameState)
{
	this.unitCollection.unregister();
	if (this.unitCollection.length)
	{
		// If the attack was started, and we are on the same land as the rallyPoint, go back there
		var rallyPoint = this.rallyPoint;
		var withdrawal = (this.isStarted() && !this.overseas);
		for (let ent of this.unitCollection.values())
		{
			ent.stopMoving();
			if (withdrawal)
				ent.move(rallyPoint[0], rallyPoint[1]);
			this.removeUnit(ent);
		}
	}

	for (let unitCat in this.unitStat)
		this.unit[unitCat].unregister();

	gameState.ai.queueManager.removeQueue("plan_" + this.name);
	gameState.ai.queueManager.removeQueue("plan_" + this.name + "_champ");
	gameState.ai.queueManager.removeQueue("plan_" + this.name + "_siege");
};

m.AttackPlan.prototype.removeUnit = function(ent, update)
{
	if (ent.hasClass("CitizenSoldier") && ent.getMetadata(PlayerID, "role") !== "worker")
	{
		ent.setMetadata(PlayerID, "role", "worker");
		ent.setMetadata(PlayerID, "subrole", undefined);
	}
	ent.setMetadata(PlayerID, "plan", -1);
	if (update)
		this.unitCollection.updateEnt(ent);
};

m.AttackPlan.prototype.checkEvents = function(gameState, events)
{
	let renameEvents = events["EntityRenamed"];
	for (let evt of renameEvents)
	{
		if (this.target && this.target.id() == evt.entity)
		{
			this.target = gameState.getEntityById(evt.newentity);
			if (this.target)
				this.targetPos = this.target.position();
		}
	}

	let captureEvents = events["OwnershipChanged"];
	for (let evt of captureEvents)
		if (this.target && this.target.id() == evt.entity && gameState.isPlayerAlly(evt.to))
			this.target = undefined;
};

m.AttackPlan.prototype.waitingForTransport = function()
{
	for (let ent of this.unitCollection.values())
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			return true;
	return false;
};

m.AttackPlan.prototype.hasForceOrder = function(data, value)
{
	for (let ent of this.unitCollection.values())
	{
		if (data && +(ent.getMetadata(PlayerID, data)) !== value)
			continue;
		let orders = ent.unitAIOrderData();
		for (let order of orders)
			if (order.force)
				return true;
	}
	return false;
};

m.AttackPlan.prototype.isSiegeUnit = function(gameState, ent)
{
	return (ent.hasClass("Siege") || (ent.hasClass("Elephant") && ent.hasClass("Champion")));
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

m.AttackPlan.prototype.Serialize = function()
{
	let properties = {
		"name": this.name,
		"type": this.type,
		"state": this.state,
		"rallyPoint": this.rallyPoint,
		"overseas": this.overseas,
		"paused": this.paused,
		"maxCompletingTurn": this.maxCompletingTurn,
		"neededShips": this.neededShips,
		"unitStat": this.unitStat,
		"position5TurnsAgo": this.position5TurnsAgo,
		"lastPosition": this.lastPosition,
		"position": this.position,
		"targetPlayer": this.targetPlayer,
		"target": ((this.target !== undefined) ? this.target.id() : undefined),
		"targetPos": this.targetPos
	};

	let path = {
		"path": this.path,
		"pathSampling": this.pathSampling,
		"pathWidth": this.pathWidth
	};

	return { "properties": properties, "path": path };
};

m.AttackPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.properties)
		this[key] = data.properties[key];

	if (this.target)
		this.target = gameState.getEntityById(this.target);

	// if the path was not fully computed, we will recompute it as it is not serialized
	if (data.path.path != "toBeContinued")
		for (let key in data.path)
			this[key] = data.path[key];

	this.failed = undefined;
};

return m;
}(PETRA);
