var PETRA = function(m)
{

/**
 * This is an attack plan:
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
	this.forced = false;  // true when this attacked has been forced to help an ally

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
		this.targetPlayer = undefined;
	}

	this.uniqueTargetId = data && data.uniqueTargetId || undefined;

	// get a starting rallyPoint ... will be improved later
	let rallyPoint;
	let rallyAccess;
	let allAccesses = {};
	for (let base of gameState.ai.HQ.baseManagers)
	{
		if (!base.anchor || !base.anchor.position())
			continue;
		let access = m.getLandAccess(gameState, base.anchor);
		if (!rallyPoint)
		{
			rallyPoint = base.anchor.position();
			rallyAccess = access;
		}
		if (!allAccesses[access])
			allAccesses[access] = base.anchor.position();
	}
	if (!rallyPoint)	// no base ?  take the position of any of our entities
	{
		for (let ent of gameState.getOwnEntities().values())
		{
			if (!ent.position())
				continue;
			let access = m.getLandAccess(gameState, ent);
			rallyPoint = ent.position();
			rallyAccess = access;
			allAccesses[access] = rallyPoint;
			break;
		}
		if (!rallyPoint)
		{
			this.failed = true;
			return false;
		}
	}
	this.rallyPoint = rallyPoint;
	this.overseas = 0;
	if (gameState.ai.HQ.navalMap)
	{
		for (let structure of gameState.getEnemyStructures().values())
		{
			if (this.target && structure.id() != this.target.id())
				continue;
			if (!structure.position())
				continue;
			let access = m.getLandAccess(gameState, structure);
			if (access in allAccesses)
			{
				this.overseas = 0;
				this.rallyPoint = allAccesses[access];
				break;
			}
			else if (!this.overseas)
			{
				let sea = gameState.ai.HQ.getSeaBetweenIndices(gameState, rallyAccess, access);
				if (!sea)
				{
					if (this.target)
					{
						API3.warn("Petra: " + this.type + " " + this.name + " has an inaccessible target " +
						          this.target.templateName() + " indices " + rallyAccess + " " + access);
						this.failed = true;
						return false;
					}
					continue;
				}
				this.overseas = sea;
				gameState.ai.HQ.navalManager.setMinimalTransportShips(gameState, sea, 1);
			}
		}
	}
	this.paused = false;
	this.maxCompletingTime = 0;

	// priority of the queues we'll create.
	let priority = 70;

	// unitStat priority is relative. If all are 0, the only relevant criteria is "currentsize/targetsize".
	// if not, this is a "bonus". The higher the priority, the faster this unit will get built.
	// Should really be clamped to [0.1-1.5] (assuming 1 is default/the norm)
	// Eg: if all are priority 1, and the siege is 0.5, the siege units will get built
	// only once every other category is at least 50% of its target size.
	// note: siege build order is currently added by the military manager if a fortress is there.
	this.unitStat = {};

	// neededShips is the minimal number of ships which should be available for transport
	if (type == "Rush")
	{
		priority = 250;
		this.unitStat.Infantry = { "priority": 1, "minSize": 10, "targetSize": 20, "batchSize": 2, "classes": ["Infantry"],
			"interests": [["strength", 1], ["costsResource", 0.5, "stone"], ["costsResource", 0.6, "metal"]] };
		this.unitStat.Cavalry = { "priority": 1, "minSize": 2, "targetSize": 4, "batchSize": 2, "classes": ["Cavalry", "CitizenSoldier"],
			"interests": [["strength", 1]] };
		if (data && data.targetSize)
			this.unitStat.Infantry.targetSize = data.targetSize;
		this.neededShips = 1;
	}
	else if (type == "Raid")
	{
		priority = 150;
		this.unitStat.Cavalry = { "priority": 1, "minSize": 3, "targetSize": 4, "batchSize": 2, "classes": ["Cavalry", "CitizenSoldier"],
			"interests": [ ["strength", 1] ] };
		this.neededShips = 1;
	}
	else if (type == "HugeAttack")
	{
		priority = 90;
		// basically we want a mix of citizen soldiers so our barracks have a purpose, and champion units.
		this.unitStat.RangedInfantry    = { "priority": 0.7, "minSize": 5, "targetSize": 20, "batchSize": 5, "classes": ["Infantry", "Ranged", "CitizenSoldier"],
			"interests": [["strength", 3]] };
		this.unitStat.MeleeInfantry     = { "priority": 0.7, "minSize": 5, "targetSize": 20, "batchSize": 5, "classes": ["Infantry", "Melee", "CitizenSoldier"],
			"interests": [["strength", 3]] };
		this.unitStat.ChampRangedInfantry = { "priority": 1, "minSize": 3, "targetSize": 18, "batchSize": 3, "classes": ["Infantry", "Ranged", "Champion"],
			"interests": [["strength", 3]] };
		this.unitStat.ChampMeleeInfantry  = { "priority": 1, "minSize": 3, "targetSize": 18, "batchSize": 3, "classes": ["Infantry", "Melee", "Champion"],
			"interests": [["strength", 3]] };
		this.unitStat.RangedCavalry     = { "priority": 0.7, "minSize": 4, "targetSize": 20, "batchSize": 4, "classes": ["Cavalry", "Ranged", "CitizenSoldier"],
			"interests": [["strength", 2]] };
		this.unitStat.MeleeCavalry      = { "priority": 0.7, "minSize": 4, "targetSize": 20, "batchSize": 4, "classes": ["Cavalry", "Melee", "CitizenSoldier"],
			"interests": [["strength", 2]] };
		this.unitStat.ChampRangedCavalry  = { "priority": 1, "minSize": 3, "targetSize": 15, "batchSize": 3, "classes": ["Cavalry", "Ranged", "Champion"],
			"interests": [["strength", 3]] };
		this.unitStat.ChampMeleeCavalry   = { "priority": 1, "minSize": 3, "targetSize": 15, "batchSize": 3, "classes": ["Cavalry", "Melee", "Champion"],
			"interests": [["strength", 2]] };
		this.unitStat.Hero                = { "priority": 1, "minSize": 0, "targetSize":  1, "batchSize": 1, "classes": ["Hero"],
			"interests": [["strength", 2]] };
		this.neededShips = 5;
	}
	else
	{
		priority = 70;
		this.unitStat.RangedInfantry = { "priority": 1, "minSize": 6, "targetSize": 16, "batchSize": 3, "classes": ["Infantry", "Ranged"],
			"interests": [["canGather", 1], ["strength", 1.6], ["costsResource", 0.3, "stone"], ["costsResource", 0.3, "metal"]] };
		this.unitStat.MeleeInfantry  = { "priority": 1, "minSize": 6, "targetSize": 16, "batchSize": 3, "classes": ["Infantry", "Melee"],
			"interests": [["canGather", 1], ["strength", 1.6], ["costsResource", 0.3, "stone"], ["costsResource", 0.3, "metal"]] };
		this.unitStat.Cavalry = { "priority": 1, "minSize": 2, "targetSize": 6, "batchSize": 2, "classes": ["Cavalry", "CitizenSoldier"],
			"interests": [["strength", 1]] };
		this.neededShips = 3;
	}

	// Put some randomness on the attack size
	let variation = randFloat(0.8, 1.2);
	// and lower priority and smaller sizes for easier difficulty levels
	if (this.Config.difficulty < 2)
	{
		priority *= 0.6;
		variation *= 0.5;
	}
	else if (this.Config.difficulty < 3)
	{
		priority *= 0.8;
		variation *= 0.8;
	}
	for (let cat in this.unitStat)
	{
		this.unitStat[cat].targetSize = Math.round(variation * this.unitStat[cat].targetSize);
		this.unitStat[cat].minSize = Math.min(this.unitStat[cat].minSize, this.unitStat[cat].targetSize);
	}

	// change the sizes according to max population
	this.neededShips = Math.ceil(this.Config.popScaling * this.neededShips);
	for (let cat in this.unitStat)
	{
		this.unitStat[cat].targetSize = Math.round(this.Config.popScaling * this.unitStat[cat].targetSize);
		this.unitStat[cat].minSize = Math.floor(this.Config.popScaling * this.unitStat[cat].minSize);
	}

	// TODO: there should probably be one queue per type of training building
	gameState.ai.queueManager.addQueue("plan_" + this.name, priority);
	gameState.ai.queueManager.addQueue("plan_" + this.name +"_champ", priority+1);
	gameState.ai.queueManager.addQueue("plan_" + this.name +"_siege", priority);

	// each array is [ratio, [associated classes], associated EntityColl, associated unitStat, name ]
	this.buildOrders = [];
	this.canBuildUnits = gameState.ai.HQ.canBuildUnits;
	this.siegeState = 0;	// 0 = not yet tested, 1 = not yet any siege trainer, 2 = siege added in build orders

	// some variables used during the attack
	this.position5TurnsAgo = [0, 0];
	this.lastPosition = [0, 0];
	this.position = [0, 0];
	this.isBlocked = false;	     // true when this attack faces walls

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
	for (let cat in this.unitStat)
	{
		let Unit = this.unitStat[cat];
		this.unit[cat] = this.unitCollection.filter(API3.Filters.byClassesAnd(Unit.classes));
		this.unit[cat].registerUpdates();
		if (this.canBuildUnits)
			this.buildOrders.push([0, Unit.classes, this.unit[cat], Unit, cat]);
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
	return this.state !== "unexecuted" && this.state !== "completing";
};

m.AttackPlan.prototype.isPaused = function()
{
	return this.paused;
};

m.AttackPlan.prototype.setPaused = function(boolValue)
{
	this.paused = boolValue;
};

/**
 * Returns true if the attack can be executed at the current time
 * Basically it checks we have enough units.
 */
m.AttackPlan.prototype.canStart = function()
{
	if (!this.canBuildUnits)
		return true;

	for (let unitCat in this.unitStat)
		if (this.unit[unitCat].length < this.unitStat[unitCat].minSize)
			return false;

	return true;
};

m.AttackPlan.prototype.mustStart = function()
{
	if (this.isPaused())
		return false;

	if (!this.canBuildUnits)
		return this.unitCollection.hasEntities();

	let MaxReachedEverywhere = true;
	let MinReachedEverywhere = true;
	for (let unitCat in this.unitStat)
	{
		let Unit = this.unitStat[unitCat];
		if (this.unit[unitCat].length < Unit.targetSize)
			MaxReachedEverywhere = false;
		if (this.unit[unitCat].length < Unit.minSize)
		{
			MinReachedEverywhere = false;
			break;
		}
	}

	if (MaxReachedEverywhere)
		return true;
	if (MinReachedEverywhere)
		return this.type == "Raid" && this.target && this.target.foundationProgress() &&
		                                             this.target.foundationProgress() > 50;
	return false;
};

m.AttackPlan.prototype.forceStart = function()
{
	for (let unitCat in this.unitStat)
	{
		let Unit = this.unitStat[unitCat];
		Unit.targetSize = 0;
		Unit.minSize = 0;
	}
	this.forced = true;
};

/** Adds a build order. If resetQueue is true, this will reset the queue. */
m.AttackPlan.prototype.addBuildOrder = function(gameState, name, unitStats, resetQueue)
{
	if (!this.isStarted())
	{
		// no minsize as we don't want the plan to fail at the last minute though.
		this.unitStat[name] = unitStats;
		let Unit = this.unitStat[name];
		this.unit[name] = this.unitCollection.filter(API3.Filters.byClassesAnd(Unit.classes));
		this.unit[name].registerUpdates();
		this.buildOrders.push([0, Unit.classes, this.unit[name], Unit, name]);
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
	if (this.siegeState == 2 || this.state !== "unexecuted")
		return false;

	let civ = gameState.getPlayerCiv();
	let classes = [[ "Siege", "Melee"], ["Siege", "Ranged"], ["Elephant", "Melee", "Champion"]];
	let hasTrainer = [false, false, false];
	for (let ent of gameState.getOwnTrainingFacilities().values())
	{
		let trainables = ent.trainableEntities(civ);
		if (!trainables)
			continue;
		for (let trainable of trainables)
		{
			if (gameState.isTemplateDisabled(trainable))
				continue;
			let template = gameState.getTemplate(trainable);
			if (!template || !template.available(gameState))
				continue;
			for (let i = 0; i < classes.length; ++i)
				if (classes[i].every(c => template.hasClass(c)))
					hasTrainer[i] = true;
		}
	}
	if (hasTrainer.every(e => !e))
		return false;
	let i = this.name % classes.length;
	for (let k = 0; k < classes.length; ++k)
	{
		if (hasTrainer[i])
			break;
		i = ++i % classes.length;
	}

	this.siegeState = 2;
	let targetSize;
	if (this.Config.difficulty < 3)
		targetSize = this.type == "HugeAttack" ? Math.max(this.Config.difficulty, 1) : Math.max(this.Config.difficulty - 1, 0);
	else
		targetSize = this.type == "HugeAttack" ? this.Config.difficulty + 1 : this.Config.difficulty - 1;
	targetSize = Math.max(Math.round(this.Config.popScaling * targetSize), this.type == "HugeAttack" ? 1 : 0);
	if (!targetSize)
		return true;
	// no minsize as we don't want the plan to fail at the last minute though.
	let stat = { "priority": 1, "minSize": 0, "targetSize": targetSize, "batchSize": Math.min(targetSize, 2),
		 "classes": classes[i], "interests": [ ["siegeStrength", 3] ] };
	this.addBuildOrder(gameState, "Siege", stat, true);
	return true;
};

/** Three returns possible: 1 is "keep going", 0 is "failed plan", 2 is "start". */
m.AttackPlan.prototype.updatePreparation = function(gameState)
{
	// the completing step is used to return resources and regroup the units
	// so we check that we have no more forced order before starting the attack
	if (this.state == "completing")
	{
		// if our target was destroyed, go back to "unexecuted" state
		if (this.targetPlayer === undefined || !this.target || !gameState.getEntityById(this.target.id()))
		{
			this.state = "unexecuted";
			this.target = undefined;
		}
		else
		{
			// check that all units have finished with their transport if needed
			if (this.waitingForTransport())
				return 1;
			// bloqued units which cannot finish their order should not stop the attack
			if (gameState.ai.elapsedTime < this.maxCompletingTime && this.hasForceOrder())
				return 1;
			return 2;
		}
	}

	if (this.Config.debug > 3 && gameState.ai.playedTurn % 50 === 0)
		this.debugAttack();

	// if we need a transport, wait for some transport ships
	if (this.overseas && !gameState.ai.HQ.navalManager.seaTransportShips[this.overseas].length)
		return 1;

	if (this.type != "Raid" || !this.forced)    // Forced Raids have special purposes (as relic capture)
		this.assignUnits(gameState);
	if (this.type != "Raid" && gameState.ai.HQ.attackManager.getAttackInPreparation("Raid") !== undefined)
		this.reassignCavUnit(gameState);    // reassign some cav (if any) to fasten raid preparations

	// Fasten the end game.
	if (gameState.ai.playedTurn % 5 == 0 && this.hasSiegeUnits())
	{
		let totEnemies = 0;
		let hasEnemies = false;
		for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
		{
			if (!gameState.isPlayerEnemy(i) || gameState.ai.HQ.attackManager.defeated[i])
				continue;
			hasEnemies = true;
			totEnemies += gameState.getEnemyUnits(i).length;
		}
		if (hasEnemies && this.unitCollection.length > 20 + 2 * totEnemies)
			this.forceStart();
	}

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
				let am = gameState.ai.HQ.attackManager;
				API3.warn(" attacks upcoming: raid " + am.upcomingAttacks.Raid.length +
					  " rush " + am.upcomingAttacks.Rush.length +
					  " attack " + am.upcomingAttacks.Attack.length +
					  " huge " + am.upcomingAttacks.HugeAttack.length);
				API3.warn(" attacks started: raid " + am.startedAttacks.Raid.length +
					  " rush " + am.startedAttacks.Rush.length +
					  " attack " + am.startedAttacks.Attack.length +
					  " huge " + am.startedAttacks.HugeAttack.length);
			}
			return 0;
		}
	}
	else if (this.mustStart())
	{
		if (gameState.countOwnQueuedEntitiesWithMetadata("plan", +this.name) > 0)
		{
			// keep on while the units finish being trained, then we'll start
			this.queue.empty();
			this.queueChamp.empty();
			this.queueSiege.empty();
			return 1;
		}
	}
	else
	{
		if (this.canBuildUnits)
		{
			// We still have time left to recruit units and do stuffs.
			if (this.siegeState == 0 || this.siegeState == 1 && gameState.ai.playedTurn % 5 == 0)
				this.addSiegeUnits(gameState);
			this.trainMoreUnits(gameState);
			// may happen if we have no more training facilities and build orders are canceled
			if (!this.buildOrders.length)
				return 0;	// will abort the plan
		}
		return 1;
	}

	// if we're here, it means we must start
	this.state = "completing";

	// Raids have their predefined target
	if (!this.target && !this.chooseTarget(gameState))
		return 0;
	if (!this.overseas)
		this.getPathToTarget(gameState);

	if (this.type == "Raid")
		this.maxCompletingTime = this.forced ? 0 : gameState.ai.elapsedTime + 20;
	else
	{
		if (this.type == "Rush" || this.forced)
			this.maxCompletingTime = gameState.ai.elapsedTime + 40;
		else
			this.maxCompletingTime = gameState.ai.elapsedTime + 60;
		// warn our allies so that they can help if possible
		if (!this.requested)
			Engine.PostCommand(PlayerID, { "type": "attack-request", "source": PlayerID, "player": this.targetPlayer });
	}

	// Remove those units which were in a temporary bombing attack
	for (let unitIds of gameState.ai.HQ.attackManager.bombingAttacks.values())
	{
		for (let entId of unitIds.values())
		{
			let ent = gameState.getEntityById(entId);
			if (!ent || ent.getMetadata(PlayerID, "plan") != this.name)
				continue;
			unitIds.delete(entId);
			ent.stopMoving();
		}
	}

	let rallyPoint = this.rallyPoint;
	let rallyIndex = gameState.ai.accessibility.getAccessValue(rallyPoint);
	for (let ent of this.unitCollection.values())
	{
		// For the time being, if occupied in a transport, remove the unit from this plan   TODO improve that
		if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
		{
			ent.setMetadata(PlayerID, "plan", -1);
			continue;
		}
		ent.setMetadata(PlayerID, "role", "attack");
		ent.setMetadata(PlayerID, "subrole", "completing");
		let queued = false;
		if (ent.resourceCarrying() && ent.resourceCarrying().length)
			queued = m.returnResources(gameState, ent);
		let index = m.getLandAccess(gameState, ent);
		if (index == rallyIndex)
			ent.moveToRange(rallyPoint[0], rallyPoint[1], 0, 15, queued);
		else
			gameState.ai.HQ.navalManager.requireTransport(gameState, ent, index, rallyIndex, rallyPoint);
	}

	// reset all queued units
	let plan = this.name;
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
	for (let order of this.buildOrders)
	{
		let special = "Plan_" + this.name + "_" + order[4];
		let aQueued = gameState.countOwnQueuedEntitiesWithMetadata("special", special);
		aQueued += this.queue.countQueuedUnitsWithMetadata("special", special);
		aQueued += this.queueChamp.countQueuedUnitsWithMetadata("special", special);
		aQueued += this.queueSiege.countQueuedUnitsWithMetadata("special", special);
		order[0] = order[2].length + aQueued;
	}
	this.buildOrders.sort((a, b) => {
		let va = a[0]/a[3].targetSize - a[3].priority;
		if (a[0] >= a[3].targetSize)
			va += 1000;
		let vb = b[0]/b[3].targetSize - b[3].priority;
		if (b[0] >= b[3].targetSize)
			vb += 1000;
		return va - vb;
	});

	if (this.Config.debug > 1 && gameState.ai.playedTurn%50 === 0)
	{
		API3.warn("====================================");
		API3.warn("======== build order for plan " + this.name);
		for (let order of this.buildOrders)
		{
			let specialData = "Plan_"+this.name+"_"+order[4];
			let inTraining = gameState.countOwnQueuedEntitiesWithMetadata("special", specialData);
			let queue1 = this.queue.countQueuedUnitsWithMetadata("special", specialData);
			let queue2 = this.queueChamp.countQueuedUnitsWithMetadata("special", specialData);
			let queue3 = this.queueSiege.countQueuedUnitsWithMetadata("special", specialData);
			API3.warn(" >>> " + order[4] + " done " + order[2].length + " training " + inTraining +
				  " queue " + queue1 + " champ " + queue2 + " siege " + queue3 + " >> need " + order[3].targetSize);
		}
		API3.warn("====================================");
	}

	let firstOrder = this.buildOrders[0];
	if (firstOrder[0] < firstOrder[3].targetSize)
	{
		// find the actual queue we want
		let queue = this.queue;
		if (firstOrder[3].classes.indexOf("Siege") != -1 || firstOrder[3].classes.indexOf("Elephant") != -1 &&
		    firstOrder[3].classes.indexOf("Melee") != -1 && firstOrder[3].classes.indexOf("Champion") != -1)
			queue = this.queueSiege;
		else if (firstOrder[3].classes.indexOf("Hero") != -1)
			queue = this.queueSiege;
		else if (firstOrder[3].classes.indexOf("Champion") != -1)
			queue = this.queueChamp;

		if (queue.length() <= 5)
		{
			let template = gameState.ai.HQ.findBestTrainableUnit(gameState, firstOrder[1], firstOrder[3].interests);
			// HACK (TODO replace) : if we have no trainable template... Then we'll simply remove the buildOrder,
			// effectively removing the unit from the plan.
			if (template === undefined)
			{
				if (this.Config.debug > 1)
					API3.warn("attack no template found " + firstOrder[1]);
				delete this.unitStat[firstOrder[4]];	// deleting the associated unitstat.
				this.buildOrders.splice(0, 1);
			}
			else
			{
				if (this.Config.debug > 2)
					API3.warn("attack template " + template + " added for plan " + this.name);
				let max = firstOrder[3].batchSize;
				let specialData = "Plan_" + this.name + "_" + firstOrder[4];
				let data = { "plan": this.name, "special": specialData, "base": 0 };
				data.role = gameState.getTemplate(template).hasClass("CitizenSoldier") ? "worker" : "attack";
				let trainingPlan = new m.TrainingPlan(gameState, template, data, max, max);
				if (trainingPlan.template)
					queue.addPlan(trainingPlan);
				else if (this.Config.debug > 1)
					API3.warn("training plan canceled because no template for " + template + "   build1 " + uneval(firstOrder[1]) +
						  " build3 " + uneval(firstOrder[3].interests));
			}
		}
	}
};

m.AttackPlan.prototype.assignUnits = function(gameState)
{
	let plan = this.name;
	let added = false;
	// If we can not build units, assign all available except those affected to allied defense to the current attack
	if (!this.canBuildUnits)
	{
		for (let ent of gameState.getOwnUnits().values())
		{
			if (ent.getMetadata(PlayerID, "allied") || !this.isAvailableUnit(gameState, ent))
				continue;
			ent.setMetadata(PlayerID, "plan", plan);
			this.unitCollection.updateEnt(ent);
			added = true;
		}
		return added;
	}

	if (this.type == "Raid")
	{
		// Raid are fast cavalry attack: assign all cav except some for hunting
		let num = 0;
		for (let ent of gameState.getOwnUnits().values())
		{
			if (!ent.hasClass("Cavalry") || !this.isAvailableUnit(gameState, ent))
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
		if (!ent.hasClass("Unit") || !this.isAvailableUnit(gameState, ent))
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
		if (!this.isAvailableUnit(gameState, ent))
			continue;
		ent.setMetadata(PlayerID, "plan", plan);
		this.unitCollection.updateEnt(ent);
		added = true;
	}

	// Finally add also some workers,
	// If Rush, assign all kind of workers, keeping only a minimum number of defenders
	// Otherwise, assign only some idle workers if too much of them
	let num = 0;
	let numbase = {};
	let keep = this.type != "Rush" ?
		6 + 4 * gameState.getNumPlayerEnemies() + 8 * this.Config.personality.defensive : 8;
	keep = Math.round(this.Config.popScaling * keep);
	for (let ent of gameState.getOwnEntitiesByRole("worker", true).values())
	{
		if (!ent.hasClass("CitizenSoldier") || !this.isAvailableUnit(gameState, ent))
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
		if (num++ < keep || numbase[baseID] < 5)
			continue;
		if (this.type != "Rush" && ent.getMetadata(PlayerID, "subrole") != "idle")
			continue;
		ent.setMetadata(PlayerID, "plan", plan);
		this.unitCollection.updateEnt(ent);
		added = true;
	}
	return added;
};

m.AttackPlan.prototype.isAvailableUnit = function(gameState, ent)
{
	if (!ent.position())
		return false;
	if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1 ||
	    ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
		return false;
	if (gameState.ai.HQ.victoryManager.criticalEnts.has(ent.id()) && (this.overseas || ent.healthLevel() < 0.8))
		return false;
	return true;
};

/** Reassign one (at each turn) Cav unit to fasten raid preparation. */
m.AttackPlan.prototype.reassignCavUnit = function(gameState)
{
	for (let ent of this.unitCollection.values())
	{
		if (!ent.position() || ent.getMetadata(PlayerID, "transport") !== undefined)
			continue;
		if (!ent.hasClass("Cavalry") || !ent.hasClass("CitizenSoldier"))
			continue;
		let raid = gameState.ai.HQ.attackManager.getAttackInPreparation("Raid");
		ent.setMetadata(PlayerID, "plan", raid.name);
		this.unitCollection.updateEnt(ent);
		raid.unitCollection.updateEnt(ent);
		return;
	}
};

m.AttackPlan.prototype.chooseTarget = function(gameState)
{
	if (this.targetPlayer === undefined)
	{
		this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
		if (this.targetPlayer === undefined)
			return false;
	}

	this.target = this.getNearestTarget(gameState, this.rallyPoint);
	if (!this.target)
	{
		if (this.uniqueTargetId)
			return false;

		// may-be all our previous enemey target (if not recomputed here) have been destroyed ?
		this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
		if (this.targetPlayer !== undefined)
			this.target = this.getNearestTarget(gameState, this.rallyPoint);
		if (!this.target)
			return false;
	}
	this.targetPos = this.target.position();
	// redefine a new rally point for this target if we have a base on the same land
	// find a new one on the pseudo-nearest base (dist weighted by the size of the island)
	let targetIndex = m.getLandAccess(gameState, this.target);
	let rallyIndex = gameState.ai.accessibility.getAccessValue(this.rallyPoint);
	if (targetIndex != rallyIndex)
	{
		let distminSame = Math.min();
		let rallySame;
		let distminDiff = Math.min();
		let rallyDiff;
		for (let base of gameState.ai.HQ.baseManagers)
		{
			let anchor = base.anchor;
			if (!anchor || !anchor.position())
				continue;
			let dist = API3.SquareVectorDistance(anchor.position(), this.targetPos);
			if (base.accessIndex == targetIndex)
			{
				if (dist >= distminSame)
					continue;
				distminSame = dist;
				rallySame = anchor.position();
			}
			else
			{
				dist /= Math.sqrt(gameState.ai.accessibility.regionSize[base.accessIndex]);
				if (dist >= distminDiff)
					continue;
				distminDiff = dist;
				rallyDiff = anchor.position();
			}
		}

		if (rallySame)
		{
			this.rallyPoint = rallySame;
			this.overseas = 0;
		}
		else if (rallyDiff)
		{
			rallyIndex = gameState.ai.accessibility.getAccessValue(rallyDiff);
			this.rallyPoint = rallyDiff;
			let sea = gameState.ai.HQ.getSeaBetweenIndices(gameState, rallyIndex, targetIndex);
			if (sea)
			{
				this.overseas = sea;
				gameState.ai.HQ.navalManager.setMinimalTransportShips(gameState, this.overseas, this.neededShips);
			}
			else
			{
				API3.warn("Petra: " + this.type + " " + this.name + " has an inaccessible target" +
				          " with indices " + rallyIndex + " " + targetIndex + " from " + this.target.templateName());
				return false;
			}
		}
	}
	else if (this.overseas)
		this.overseas = 0;

	return true;
};
/**
 * sameLand true means that we look for a target for which we do not need to take a transport
 */
m.AttackPlan.prototype.getNearestTarget = function(gameState, position, sameLand)
{
	this.isBlocked = false;
	// Temporary variables needed by isValidTarget
	this.gameState = gameState;
	this.sameLand = sameLand && sameLand > 1 ? sameLand : false;

	let targets;
	if (this.uniqueTargetId)
	{
		targets = new API3.EntityCollection(gameState.sharedScript);
		let ent = gameState.getEntityById(this.uniqueTargetId);
		if (ent)
			targets.addEnt(ent);
	}
	else
	{
		if (this.type == "Raid")
			targets = this.raidTargetFinder(gameState);
		else if (this.type == "Rush" || this.type == "Attack")
		{
			targets = this.rushTargetFinder(gameState, this.targetPlayer);
			if (!targets.hasEntities() && (this.hasSiegeUnits() || this.forced))
				targets = this.defaultTargetFinder(gameState, this.targetPlayer);
		}
		else
			targets = this.defaultTargetFinder(gameState, this.targetPlayer);
	}
	if (!targets.hasEntities())
		return undefined;

	// picking the nearest target
	let target;
	let minDist = Math.min();
	for (let ent of targets.values())
	{
		if (this.targetPlayer == 0 && gameState.getVictoryConditions().has("capture_the_relic") &&
		   (!ent.hasClass("Relic") || gameState.ai.HQ.victoryManager.targetedGaiaRelics.has(ent.id())))
			continue;
		// Do not bother with some pointless targets
		if (!this.isValidTarget(ent))
			continue;
		let dist = API3.SquareVectorDistance(ent.position(), position);
		// In normal attacks, disfavor fields
		if (this.type != "Rush" && this.type != "Raid" && ent.hasClass("Field"))
			dist += 100000;
		if (dist < minDist)
		{
			minDist = dist;
			target = ent;
		}
	}
	if (!target)
		return undefined;

	// Check that we can reach this target
	target = this.checkTargetObstruction(gameState, target, position);

	if (!target)
		return undefined;
	if (this.targetPlayer == 0 && gameState.getVictoryConditions().has("capture_the_relic") && target.hasClass("Relic"))
		gameState.ai.HQ.victoryManager.targetedGaiaRelics.set(target.id(), [this.name]);
	// Rushes can change their enemy target if nothing found with the preferred enemy
	// Obstruction also can change the enemy target
	this.targetPlayer = target.owner();
	return target;
};

/**
 * Default target finder aims for conquest critical targets
 * We must apply the *same* selection (isValidTarget) as done in getNearestTarget
 */
m.AttackPlan.prototype.defaultTargetFinder = function(gameState, playerEnemy)
{
	let targets = new API3.EntityCollection(gameState.sharedScript);
	if (gameState.getVictoryConditions().has("wonder"))
		for (let ent of gameState.getEnemyStructures(playerEnemy).filter(API3.Filters.byClass("Wonder")).values())
			targets.addEnt(ent);
	if (gameState.getVictoryConditions().has("regicide"))
		for (let ent of gameState.getEnemyUnits(playerEnemy).filter(API3.Filters.byClass("Hero")).values())
			targets.addEnt(ent);
	if (gameState.getVictoryConditions().has("capture_the_relic"))
		for (let ent of gameState.updatingGlobalCollection("allRelics", API3.Filters.byClass("Relic")).filter(relic => relic.owner() == playerEnemy).values())
			targets.addEnt(ent);
	targets = targets.filter(this.isValidTarget, this);
	if (targets.hasEntities())
		return targets;

	let validTargets = gameState.getEnemyStructures(playerEnemy).filter(this.isValidTarget, this);
	targets = validTargets.filter(API3.Filters.byClass("CivCentre"));
	if (!targets.hasEntities())
		targets = validTargets.filter(API3.Filters.byClass("ConquestCritical"));
	// If there's nothing, attack anything else that's less critical
	if (!targets.hasEntities())
		targets = validTargets.filter(API3.Filters.byClass("Town"));
	if (!targets.hasEntities())
		targets = validTargets.filter(API3.Filters.byClass("Village"));
	// No buildings, attack anything conquest critical, units included.
	// TODO Should add naval attacks against the last remaining ships.
	if (!targets.hasEntities())
		targets = gameState.getEntities(playerEnemy).filter(API3.Filters.byClass("ConquestCritical")).
		                                       filter(API3.Filters.not(API3.Filters.byClass("Ship")));
	return targets;
};

m.AttackPlan.prototype.isValidTarget = function(ent)
{
	if (!ent.position())
		return false;
	if (this.sameLand && m.getLandAccess(this.gameState, ent) != this.sameLand)
		return false;
	return !ent.decaying() || ent.getDefaultArrow() || ent.isGarrisonHolder() && ent.garrisoned().length;
};

/** Rush target finder aims at isolated non-defended buildings */
m.AttackPlan.prototype.rushTargetFinder = function(gameState, playerEnemy)
{
	let targets = new API3.EntityCollection(gameState.sharedScript);
	let buildings;
	if (playerEnemy !== undefined)
		buildings = gameState.getEnemyStructures(playerEnemy).toEntityArray();
	else
		buildings = gameState.getEnemyStructures().toEntityArray();
	if (!buildings.length)
		return targets;

	this.position = this.unitCollection.getCentrePosition();
	if (!this.position)
		this.position = this.rallyPoint;

	let target;
	let minDist = Math.min();
	for (let building of buildings)
	{
		if (building.owner() == 0)
			continue;
		if (building.hasDefensiveFire())
			continue;
		if (!this.isValidTarget(building))
			continue;
		let pos = building.position();
		let defended = false;
		for (let defense of buildings)
		{
			if (!defense.hasDefensiveFire())
				continue;
			let dist = API3.SquareVectorDistance(pos, defense.position());
			if (dist < 6400)   // TODO check on defense range rather than this fixed 80*80
			{
				defended = true;
				break;
			}
		}
		if (defended)
			continue;
		let dist = API3.SquareVectorDistance(pos, this.position);
		if (dist > minDist)
			continue;
		minDist = dist;
		target = building;
	}
	if (target)
		targets.addEnt(target);

	if (!targets.hasEntities() && this.type == "Rush" && playerEnemy)
		targets = this.rushTargetFinder(gameState);

	return targets;
};

/** Raid target finder aims at destructing foundations from which our defenseManager has attacked the builders */
m.AttackPlan.prototype.raidTargetFinder = function(gameState)
{
	let targets = new API3.EntityCollection(gameState.sharedScript);
	for (let targetId of gameState.ai.HQ.defenseManager.targetList)
	{
		let target = gameState.getEntityById(targetId);
		if (target && target.position())
			targets.addEnt(target);
	}
	return targets;
};

/**
 * Check that we can have a path to this target
 * otherwise we may be blocked by walls and try to react accordingly
 * This is done only when attacker and target are on the same land
 */
m.AttackPlan.prototype.checkTargetObstruction = function(gameState, target, position)
{
	if (m.getLandAccess(gameState, target) != gameState.ai.accessibility.getAccessValue(position))
		return target;

	let targetPos = target.position();
	let startPos = { "x": position[0], "y": position[1] };
	let endPos = { "x": targetPos[0], "y": targetPos[1] };
	let blocker;
	let path = Engine.ComputePath(startPos, endPos, gameState.getPassabilityClassMask("default"));
	if (!path.length)
		return undefined;

	let pathPos = [path[0].x, path[0].y];
	let dist = API3.VectorDistance(pathPos, targetPos);
	let radius = target.obstructionRadius().max;
	for (let struct of gameState.getEnemyStructures().values())
	{
		if (!struct.position() || !struct.get("Obstruction") || struct.hasClass("Field"))
			continue;
		// we consider that we can reach the target, but nonetheless check that we did not cross any enemy gate
		if (dist < radius + 10 && !struct.hasClass("Gates"))
			continue;
		// Check that we are really blocked by this structure, i.e. advancing by 1+0.8(clearance)m
		// in the target direction would bring us inside its obstruction.
		let structPos = struct.position();
		let x = pathPos[0] - structPos[0] + 1.8 * (targetPos[0] - pathPos[0]) / dist;
		let y = pathPos[1] - structPos[1] + 1.8 * (targetPos[1] - pathPos[1]) / dist;

		if (struct.get("Obstruction/Static"))
		{
			if (!struct.angle())
				continue;
			let angle = struct.angle();
			let width = +struct.get("Obstruction/Static/@width");
			let depth = +struct.get("Obstruction/Static/@depth");
			let cosa = Math.cos(angle);
			let sina = Math.sin(angle);
			let u = x * cosa - y * sina;
			let v = x * sina + y * cosa;
			if (Math.abs(u) < width/2 && Math.abs(v) < depth/2)
			{
				blocker = struct;
				break;
			}
		}
		else if (struct.get("Obstruction/Obstructions"))
		{
			if (!struct.angle())
				continue;
			let angle = struct.angle();
			let width = +struct.get("Obstruction/Obstructions/Door/@width");
			let depth = +struct.get("Obstruction/Obstructions/Door/@depth");
			let doorHalfWidth = width / 2;
			width += +struct.get("Obstruction/Obstructions/Left/@width");
			depth = Math.max(depth, +struct.get("Obstruction/Obstructions/Left/@depth"));
			width += +struct.get("Obstruction/Obstructions/Right/@width");
			depth = Math.max(depth, +struct.get("Obstruction/Obstructions/Right/@depth"));
			let cosa = Math.cos(angle);
			let sina = Math.sin(angle);
			let u = x * cosa - y * sina;
			let v = x * sina + y * cosa;
			if (Math.abs(u) < width/2 && Math.abs(v) < depth/2)
			{
				blocker = struct;
				break;
			}
			// check that the path does not cross this gate (could happen if not locked)
			for (let i = 1; i < path.length; ++i)
			{
				let u1 = (path[i-1].x - structPos[0]) * cosa - (path[i-1].y - structPos[1]) * sina;
				let v1 = (path[i-1].x - structPos[0]) * sina + (path[i-1].y - structPos[1]) * cosa;
				let u2 = (path[i].x - structPos[0]) * cosa - (path[i].y - structPos[1]) * sina;
				let v2 = (path[i].x - structPos[0]) * sina + (path[i].y - structPos[1]) * cosa;
				if (v1 * v2 < 0)
				{
					let u0 = (u1*v2 - u2*v1) / (v2-v1);
					if (Math.abs(u0) > doorHalfWidth)
						continue;
					blocker = struct;
					break;
				}
			}
			if (blocker)
				break;
		}
		else if (struct.get("Obstruction/Unit"))
		{
			let r = +this.get("Obstruction/Unit/@radius");
			if (x*x + y*y < r*r)
			{
				blocker = struct;
				break;
			}
		}
	}

	if (blocker && blocker.hasClass("StoneWall"))
	{
/*		if (this.hasSiegeUnits())
		{ */
			this.isBlocked = true;
			return blocker;
/*		}
		return undefined; */
	}
	else if (blocker)
	{
		this.isBlocked = true;
		return blocker;
	}

	return target;
};

m.AttackPlan.prototype.getPathToTarget = function(gameState, fixedRallyPoint = false)
{
	let startAccess = gameState.ai.accessibility.getAccessValue(this.rallyPoint);
	let endAccess = m.getLandAccess(gameState, this.target);
	if (startAccess != endAccess)
		return false;

	Engine.ProfileStart("AI Compute path");
	let startPos = { "x": this.rallyPoint[0], "y": this.rallyPoint[1] };
	let endPos = { "x": this.targetPos[0], "y": this.targetPos[1] };
	let path = Engine.ComputePath(startPos, endPos, gameState.getPassabilityClassMask("large"));
	this.path = [];
	this.path.push(this.targetPos);
	for (let p in path)
		this.path.push([path[p].x, path[p].y]);
	this.path.push(this.rallyPoint);
	this.path.reverse();
	// Change the rally point to something useful
	if (!fixedRallyPoint)
		this.setRallyPoint(gameState);
	Engine.ProfileStop();

	return true;
};

/** Set rally point at the border of our territory */
m.AttackPlan.prototype.setRallyPoint = function(gameState)
{
	for (let i = 0; i < this.path.length; ++i)
	{
		if (gameState.ai.HQ.territoryMap.getOwner(this.path[i]) === PlayerID)
			continue;

		if (i === 0)
			this.rallyPoint = this.path[0];
		else if (i > 1 && gameState.ai.HQ.isDangerousLocation(gameState, this.path[i-1], 20))
		{
			this.rallyPoint = this.path[i-2];
			this.path.splice(0, i-2);
		}
		else
		{
			this.rallyPoint = this.path[i-1];
			this.path.splice(0, i-1);
		}
		break;
	}
};

/**
 * Executes the attack plan, after this is executed the update function will be run every turn
 * If we're here, it's because we have enough units.
 */
m.AttackPlan.prototype.StartAttack = function(gameState)
{
	if (this.Config.debug > 1)
		API3.warn("start attack " + this.name + " with type " + this.type);

	// if our target was destroyed during preparation, choose a new one
	if ((this.targetPlayer === undefined || !this.target || !gameState.getEntityById(this.target.id())) &&
	    !this.chooseTarget(gameState))
		return false;

	// erase our queue. This will stop any leftover unit from being trained.
	gameState.ai.queueManager.removeQueue("plan_" + this.name);
	gameState.ai.queueManager.removeQueue("plan_" + this.name + "_champ");
	gameState.ai.queueManager.removeQueue("plan_" + this.name + "_siege");

	for (let ent of this.unitCollection.values())
	{
		ent.setMetadata(PlayerID, "subrole", "walking");
		let stance = ent.isPackable() ? "standground" : "aggressive";
		if (ent.getStance() != stance)
			ent.setStance(stance);
	}

	let rallyAccess = gameState.ai.accessibility.getAccessValue(this.rallyPoint);
	let targetAccess = m.getLandAccess(gameState, this.target);
	if (rallyAccess == targetAccess)
	{
		if (!this.path)
			this.getPathToTarget(gameState, true);
		if (!this.path || !this.path[0][0] || !this.path[0][1])
			return false;
		this.overseas = 0;
		this.state = "walking";
		this.unitCollection.moveToRange(this.path[0][0], this.path[0][1], 0, 15);
	}
	else
	{
		this.overseas = gameState.ai.HQ.getSeaBetweenIndices(gameState, rallyAccess, targetAccess);
		if (!this.overseas)
			return false;
		this.state = "transporting";
		// TODO require a global transport for the collection,
		// and put back its state to "walking" when the transport is finished
		for (let ent of this.unitCollection.values())
			gameState.ai.HQ.navalManager.requireTransport(gameState, ent, rallyAccess, targetAccess, this.targetPos);
	}
	return true;
};

/** Runs every turn after the attack is executed */
m.AttackPlan.prototype.update = function(gameState, events)
{
	if (!this.unitCollection.hasEntities())
		return 0;

	Engine.ProfileStart("Update Attack");

	this.position = this.unitCollection.getCentrePosition();

	// we are transporting our units, let's wait
	// TODO instead of state "arrived", made a state "walking" with a new path
	if (this.state == "transporting")
		this.UpdateTransporting(gameState, events);

	if (this.state == "walking" && !this.UpdateWalking(gameState, events))
	{
		Engine.ProfileStop();
		return 0;
	}

	if (this.state == "arrived")
	{
		// let's proceed on with whatever happens now.
		this.state = "";
		this.startingAttack = true;
		this.unitCollection.forEach(ent => {
			ent.stopMoving();
			ent.setMetadata(PlayerID, "subrole", "attacking");
		});
		if (this.type == "Rush")   // try to find a better target for rush
		{
			let newtarget = this.getNearestTarget(gameState, this.position);
			if (newtarget)
			{
				this.target = newtarget;
				this.targetPos = this.target.position();
			}
		}
	}

	// basic state of attacking.
	if (this.state == "")
	{
		// First update the target and/or its position if needed
		if (!this.UpdateTarget(gameState))
		{
			Engine.ProfileStop();
			return false;
		}

		let time = gameState.ai.elapsedTime;
		let attackedByStructure = {};
		for (let evt of events.Attacked)
		{
			if (!this.unitCollection.hasEntId(evt.target))
				continue;
			let attacker = gameState.getEntityById(evt.attacker);
			let ourUnit = gameState.getEntityById(evt.target);
			if (!ourUnit || !attacker || !attacker.position())
				continue;
			if (!attacker.hasClass("Unit"))
			{
				attackedByStructure[evt.target] = true;
				continue;
			}
			if (m.isSiegeUnit(ourUnit))
			{	// if our siege units are attacked, we'll send some units to deal with enemies.
				let collec = this.unitCollection.filter(API3.Filters.not(API3.Filters.byClass("Siege"))).filterNearest(ourUnit.position(), 5);
				for (let ent of collec.values())
				{
					if (m.isSiegeUnit(ent))	// needed as mauryan elephants are not filtered out
						continue;
					ent.attack(attacker.id(), m.allowCapture(gameState, ent, attacker));
					ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
				}
				// And if this attacker is a non-ranged siege unit and our unit also, attack it
				if (m.isSiegeUnit(attacker) && attacker.hasClass("Melee") && ourUnit.hasClass("Melee"))
				{
					ourUnit.attack(attacker.id(), m.allowCapture(gameState, ourUnit, attacker));
					ourUnit.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
				}
			}
			else
			{
				if (this.isBlocked && !ourUnit.hasClass("Ranged") && attacker.hasClass("Ranged"))
				{
					// do not react if our melee units are attacked by ranged one and we are blocked by walls
					// TODO check that the attacker is from behind the wall
					continue;
				}
				else if (m.isSiegeUnit(attacker))
				{	// if our unit is attacked by a siege unit, we'll send some melee units to help it.
					let collec = this.unitCollection.filter(API3.Filters.byClass("Melee")).filterNearest(ourUnit.position(), 5);
					for (let ent of collec.values())
					{
						ent.attack(attacker.id(), m.allowCapture(gameState, ent, attacker));
						ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
					}
				}
				else
				{
					// Look first for nearby units to help us if possible
					let collec = this.unitCollection.filterNearest(ourUnit.position(), 2);
					for (let ent of collec.values())
					{
						if (m.isSiegeUnit(ent))
							continue;
						let orderData = ent.unitAIOrderData();
						if (orderData && orderData.length && orderData[0].target)
						{
							if (orderData[0].target === attacker.id())
								continue;
							let target = gameState.getEntityById(orderData[0].target);
							if (target && !target.hasClass("Structure") && !target.hasClass("Support"))
								continue;
						}
						ent.attack(attacker.id(), m.allowCapture(gameState, ent, attacker));
						ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
					}
					// Then the unit under attack: abandon its target (if it was a structure or a support) and retaliate
					// also if our unit is attacking a range unit and the attacker is a melee unit, retaliate
					let orderData = ourUnit.unitAIOrderData();
					if (orderData && orderData.length && orderData[0].target)
					{
						if (orderData[0].target === attacker.id())
							continue;
						let target = gameState.getEntityById(orderData[0].target);
						if (target && !target.hasClass("Structure") && !target.hasClass("Support"))
						{
							if (!target.hasClass("Ranged") || !attacker.hasClass("Melee"))
								continue;
						}
					}
					ourUnit.attack(attacker.id(), m.allowCapture(gameState, ourUnit, attacker));
					ourUnit.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
				}
			}
		}

		let enemyUnits = gameState.getEnemyUnits(this.targetPlayer);
		let enemyStructures = gameState.getEnemyStructures(this.targetPlayer);

		// Count the number of times an enemy is targeted, to prevent all units to follow the same target
		let unitTargets = {};
		for (let ent of this.unitCollection.values())
		{
			if (ent.hasClass("Ship"))	// TODO What to do with ships
				continue;
			let orderData = ent.unitAIOrderData();
			if (!orderData || !orderData.length || !orderData[0].target)
				continue;
			let targetId = orderData[0].target;
			let target = gameState.getEntityById(targetId);
			if (!target || target.hasClass("Structure"))
				continue;
			if (!(targetId in unitTargets))
			{
				if (m.isSiegeUnit(target) || target.hasClass("Hero"))
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

		let targetClassesUnit;
		let targetClassesSiege;
		if (this.type == "Rush")
			targetClassesUnit = { "attack": ["Unit", "Structure"], "avoid": ["Palisade", "StoneWall", "Tower", "Fortress"], "vetoEntities": veto };
		else
		{
			if (this.target.hasClass("Fortress"))
				targetClassesUnit = { "attack": ["Unit", "Structure"], "avoid": ["Palisade", "StoneWall"], "vetoEntities": veto };
			else if (this.target.hasClass("Palisade") || this.target.hasClass("StoneWall"))
				targetClassesUnit = { "attack": ["Unit", "Structure"], "avoid": ["Fortress"], "vetoEntities": veto };
			else
				targetClassesUnit = { "attack": ["Unit", "Structure"], "avoid": ["Palisade", "StoneWall", "Fortress"], "vetoEntities": veto };
		}
		if (this.target.hasClass("Structure"))
			targetClassesSiege = { "attack": ["Structure"], "avoid": [], "vetoEntities": veto };
		else
			targetClassesSiege = { "attack": ["Unit", "Structure"], "avoid": [], "vetoEntities": veto };

		// do not loose time destroying buildings which do not help enemy's defense and can be easily captured later
		if (this.target.hasDefensiveFire())
		{
			targetClassesUnit.avoid = targetClassesUnit.avoid.concat("House", "Storehouse", "Farmstead", "Field", "Blacksmith");
			targetClassesSiege.avoid = targetClassesSiege.avoid.concat("House", "Storehouse", "Farmstead", "Field", "Blacksmith");
		}

		if (this.unitCollUpdateArray === undefined || !this.unitCollUpdateArray.length)
			this.unitCollUpdateArray = this.unitCollection.toIdArray();

		// Let's check a few units each time we update (currently 10) except when attack starts
		let lgth = this.unitCollUpdateArray.length < 15 || this.startingAttack ? this.unitCollUpdateArray.length : 10;
		for (let check = 0; check < lgth; check++)
		{
			let ent = gameState.getEntityById(this.unitCollUpdateArray[check]);
			if (!ent || !ent.position())
				continue;
			// Do not reaffect units which have reacted to an attack in that same turn
			if (ent.getMetadata(PlayerID, "lastAttackPlanUpdateTime") == time)
				continue;

			let targetId;
			let orderData = ent.unitAIOrderData();
			if (orderData && orderData.length && orderData[0].target)
				targetId = orderData[0].target;

			// update the order if needed
			let needsUpdate = false;
			let maybeUpdate = false;
			let siegeUnit = m.isSiegeUnit(ent);
			if (ent.isIdle())
				needsUpdate = true;
			else if (siegeUnit && targetId)
			{
				let target = gameState.getEntityById(targetId);
				if (!target || gameState.isPlayerAlly(target.owner()))
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
				let target = gameState.getEntityById(targetId);
				if (!target || gameState.isPlayerAlly(target.owner()))
					needsUpdate = true;
				else if (unitTargets[targetId] && unitTargets[targetId] > 0)
				{
					needsUpdate = true;
					--unitTargets[targetId];
				}
				else if (target.hasClass("Ship") && !ent.hasClass("Ship"))
					maybeUpdate = true;
				else if (attackedByStructure[ent.id()] && target.hasClass("Field"))
					maybeUpdate = true;
				else if (!ent.hasClass("Cavalry") && !ent.hasClass("Ranged") &&
					 target.hasClass("FemaleCitizen") && target.unitAIState().split(".")[1] == "FLEEING")
					maybeUpdate = true;
			}

			// don't update too soon if not necessary
			if (!needsUpdate)
			{
				if (!maybeUpdate)
					continue;
				let deltat = ent.unitAIState() === "INDIVIDUAL.COMBAT.APPROACHING" ? 10 : 5;
				let lastAttackPlanUpdateTime = ent.getMetadata(PlayerID, "lastAttackPlanUpdateTime");
				if (lastAttackPlanUpdateTime && time - lastAttackPlanUpdateTime < deltat)
					continue;
			}
			ent.setMetadata(PlayerID, "lastAttackPlanUpdateTime", time);
			let range = 60;
			let attackTypes = ent.attackTypes();
			if (this.isBlocked)
			{
				if (attackTypes && attackTypes.indexOf("Ranged") !== -1)
					range = ent.attackRange("Ranged").max;
				else if (attackTypes && attackTypes.indexOf("Melee") !== -1)
					range = ent.attackRange("Melee").max;
				else
					range = 10;
			}
			else if (attackTypes && attackTypes.indexOf("Ranged") !== -1)
				range = 30 + ent.attackRange("Ranged").max;
			else if (ent.hasClass("Cavalry"))
				range += 30;
			range = range * range;
			let entAccess = m.getLandAccess(gameState, ent);
			// Checking for gates if we're a siege unit.
			if (siegeUnit)
			{
				let mStruct = enemyStructures.filter(enemy => {
					if (!enemy.position() || enemy.hasClass("StoneWall") && !ent.canAttackClass("StoneWall"))
						return false;
					if (API3.SquareVectorDistance(enemy.position(), ent.position()) > range)
						return false;
					if (enemy.foundationProgress() == 0)
						return false;
					if (m.getLandAccess(gameState, enemy) != entAccess)
						return false;
					return true;
				}).toEntityArray();
				if (mStruct.length)
				{
					mStruct.sort((structa, structb) => {
						let vala = structa.costSum();
						if (structa.hasClass("Gates") && ent.canAttackClass("StoneWall"))
							vala += 10000;
						else if (structa.hasDefensiveFire())
							vala += 1000;
						else if (structa.hasClass("ConquestCritical"))
							vala += 200;
						let valb = structb.costSum();
						if (structb.hasClass("Gates") && ent.canAttackClass("StoneWall"))
							valb += 10000;
						else if (structb.hasDefensiveFire())
							valb += 1000;
						else if (structb.hasClass("ConquestCritical"))
							valb += 200;
						return valb - vala;
					});
					if (mStruct[0].hasClass("Gates"))
						ent.attack(mStruct[0].id(), m.allowCapture(gameState, ent, mStruct[0]));
					else
					{
						let rand = randIntExclusive(0, mStruct.length * 0.2);
						ent.attack(mStruct[rand].id(), m.allowCapture(gameState, ent, mStruct[rand]));
					}
				}
				else
				{
					if (!ent.hasClass("Ranged"))
					{
						let targetClasses = { "attack": targetClassesSiege.attack, "avoid": targetClassesSiege.avoid.concat("Ship"), "vetoEntities": veto };
						ent.attackMove(this.targetPos[0], this.targetPos[1], targetClasses);
					}
					else
						ent.attackMove(this.targetPos[0], this.targetPos[1], targetClassesSiege);
				}
			}
			else
			{
				let nearby = !ent.hasClass("Cavalry") && !ent.hasClass("Ranged");
				let mUnit = enemyUnits.filter(enemy => {
					if (!enemy.position())
						return false;
					if (enemy.hasClass("Animal"))
						return false;
					if (nearby && enemy.hasClass("FemaleCitizen") && enemy.unitAIState().split(".")[1] == "FLEEING")
						return false;
					let dist = API3.SquareVectorDistance(enemy.position(), ent.position());
					if (dist > range)
						return false;
					if (m.getLandAccess(gameState, enemy) != entAccess)
						return false;
					// if already too much units targeting this enemy, let's continue towards our main target
					if (veto[enemy.id()] && API3.SquareVectorDistance(this.targetPos, ent.position()) > 2500)
						return false;
					enemy.setMetadata(PlayerID, "distance", Math.sqrt(dist));
					return true;
				}, this).toEntityArray();
				if (mUnit.length)
				{
					mUnit.sort((unitA, unitB) => {
						let vala = unitA.hasClass("Support") ? 50 : 0;
						if (ent.countersClasses(unitA.classes()))
							vala += 100;
						let valb = unitB.hasClass("Support") ? 50 : 0;
						if (ent.countersClasses(unitB.classes()))
							valb += 100;
						let distA = unitA.getMetadata(PlayerID, "distance");
						let distB = unitB.getMetadata(PlayerID, "distance");
						if (distA && distB)
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
					let rand = randIntExclusive(0, mUnit.length * 0.1);
					ent.attack(mUnit[rand].id(), m.allowCapture(gameState, ent, mUnit[rand]));
				}
				else if (this.isBlocked)
					ent.attack(this.target.id(), false);
				else if (API3.SquareVectorDistance(this.targetPos, ent.position()) > 2500)
				{
					let targetClasses = targetClassesUnit;
					if (maybeUpdate && ent.unitAIState() === "INDIVIDUAL.COMBAT.APPROACHING")	// we may be blocked by walls, attack everything
					{
						if (!ent.hasClass("Ranged") && !ent.hasClass("Ship"))
							targetClasses = { "attack": ["Unit", "Structure"], "avoid": ["Ship"], "vetoEntities": veto };
						else
							targetClasses = { "attack": ["Unit", "Structure"], "vetoEntities": veto };
					}
					else if (!ent.hasClass("Ranged") && !ent.hasClass("Ship"))
						targetClasses = { "attack": targetClassesUnit.attack, "avoid": targetClassesUnit.avoid.concat("Ship"), "vetoEntities": veto };
					ent.attackMove(this.targetPos[0], this.targetPos[1], targetClasses);
				}
				else
				{
					let mStruct = enemyStructures.filter(enemy => {
						if (this.isBlocked && enemy.id() != this.target.id())
							return false;
						if (!enemy.position() || enemy.hasClass("StoneWall") && !ent.canAttackClass("StoneWall"))
							return false;
						if (API3.SquareVectorDistance(enemy.position(), ent.position()) > range)
							return false;
						if (m.getLandAccess(gameState, enemy) != entAccess)
							return false;
						return true;
					}, this).toEntityArray();
					if (mStruct.length)
					{
						mStruct.sort((structa, structb) => {
							let vala = structa.costSum();
							if (structa.hasClass("Gates") && ent.canAttackClass("StoneWall"))
								vala += 10000;
							else if (structa.hasClass("ConquestCritical"))
								vala += 100;
							let valb = structb.costSum();
							if (structb.hasClass("Gates") && ent.canAttackClass("StoneWall"))
								valb += 10000;
							else if (structb.hasClass("ConquestCritical"))
								valb += 100;
							return valb - vala;
						});
						if (mStruct[0].hasClass("Gates"))
							ent.attack(mStruct[0].id(), false);
						else
						{
							let rand = randIntExclusive(0, mStruct.length * 0.2);
							ent.attack(mStruct[rand].id(), m.allowCapture(gameState, ent, mStruct[rand]));
						}
					}
					else if (needsUpdate)  // really nothing   let's try to help our nearest unit
					{
						let distmin = Math.min();
						let attacker;
						this.unitCollection.forEach(unit => {
							if (!unit.position())
								return;
							if (unit.unitAIState().split(".")[1] != "COMBAT" || !unit.unitAIOrderData().length ||
								!unit.unitAIOrderData()[0].target)
								return;
							if (!gameState.getEntityById(unit.unitAIOrderData()[0].target))
								return;
							let dist = API3.SquareVectorDistance(unit.position(), ent.position());
							if (dist > distmin)
								return;
							distmin = dist;
							attacker = gameState.getEntityById(unit.unitAIOrderData()[0].target);
						});
						if (attacker)
							ent.attack(attacker.id(), m.allowCapture(gameState, ent, attacker));
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

m.AttackPlan.prototype.UpdateTransporting = function(gameState, events)
{
	let done = true;
	for (let ent of this.unitCollection.values())
	{
		if (this.Config.debug > 1 && ent.getMetadata(PlayerID, "transport") !== undefined)
			Engine.PostCommand(PlayerID, { "type": "set-shading-color", "entities": [ent.id()], "rgb": [2, 2, 0] });
		else if (this.Config.debug > 1)
			Engine.PostCommand(PlayerID, { "type": "set-shading-color", "entities": [ent.id()], "rgb": [1, 1, 1] });
		if (!done)
			continue;
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			done = false;
	}

	if (done)
	{
		this.state = "arrived";
		return;
	}

	// if we are attacked while waiting the rest of the army, retaliate
	for (let evt of events.Attacked)
	{
		if (!this.unitCollection.hasEntId(evt.target))
			continue;
		let attacker = gameState.getEntityById(evt.attacker);
		if (!attacker || !gameState.getEntityById(evt.target))
			continue;
		for (let ent of this.unitCollection.values())
		{
			if (ent.getMetadata(PlayerID, "transport") !== undefined)
				continue;
			if (!ent.isIdle())
				continue;
			ent.attack(attacker.id(), m.allowCapture(gameState, ent, attacker));
		}
		break;
	}
};

m.AttackPlan.prototype.UpdateWalking = function(gameState, events)
{
	// we're marching towards the target
	// Let's check if any of our unit has been attacked.
	// In case yes, we'll determine if we're simply off against an enemy army, a lone unit/building
	// or if we reached the enemy base. Different plans may react differently.
	let attackedNB = 0;
	let attackedUnitNB = 0;
	for (let evt of events.Attacked)
	{
		if (!this.unitCollection.hasEntId(evt.target))
			continue;
		let attacker = gameState.getEntityById(evt.attacker);
		if (attacker && (attacker.owner() !== 0 || this.targetPlayer === 0))
		{
			attackedNB++;
			if (attacker.hasClass("Unit"))
				attackedUnitNB++;
		}
	}
	// Are we arrived at destination ?
	if (attackedNB > 1 && (attackedUnitNB || this.hasSiegeUnits()))
	{
		if (gameState.ai.HQ.territoryMap.getOwner(this.position) === this.targetPlayer || attackedNB > 3)
		{
			this.state = "arrived";
			return true;
		}
	}

	// basically haven't moved an inch: very likely stuck)
	if (API3.SquareVectorDistance(this.position, this.position5TurnsAgo) < 10 && this.path.length > 0 && gameState.ai.playedTurn % 5 === 0)
	{
		// check for stuck siege units
		let farthest = 0;
		let farthestEnt;
		for (let ent of this.unitCollection.filter(API3.Filters.byClass("Siege")).values())
		{
			let dist = API3.SquareVectorDistance(ent.position(), this.position);
			if (dist < farthest)
				continue;
			farthest = dist;
			farthestEnt = ent;
		}
		if (farthestEnt)
			farthestEnt.destroy();
	}
	if (gameState.ai.playedTurn % 5 === 0)
		this.position5TurnsAgo = this.position;

	if (this.lastPosition && API3.SquareVectorDistance(this.position, this.lastPosition) < 16 && this.path.length > 0)
	{
		if (!this.path[0][0] || !this.path[0][1])
			API3.warn("Start: Problem with path " + uneval(this.path));
		// We're stuck, presumably. Check if there are no walls just close to us.
		for (let ent of gameState.getEnemyStructures().filter(API3.Filters.byClass(["Palisade", "StoneWall"])).values())
		{
			if (API3.SquareVectorDistance(this.position, ent.position()) > 800)
				continue;
			let enemyClass = ent.hasClass("StoneWall") ? "StoneWall" : "Palisade";
			// there are walls, so check if we can attack
			if (this.unitCollection.filter(API3.Filters.byCanAttackClass(enemyClass)).hasEntities())
			{
				if (this.Config.debug > 1)
					API3.warn("Attack Plan " + this.type + " " + this.name + " has met walls and is not happy.");
				this.state = "arrived";
				return true;
			}
			// abort plan
			if (this.Config.debug > 1)
				API3.warn("Attack Plan " + this.type + " " + this.name + " has met walls and gives up.");
			return false;
		}

		// this.unitCollection.move(this.path[0][0], this.path[0][1]);
		this.unitCollection.moveIndiv(this.path[0][0], this.path[0][1]);
	}

	// check if our units are close enough from the next waypoint.
	if (API3.SquareVectorDistance(this.position, this.targetPos) < 10000)
	{
		if (this.Config.debug > 1)
			API3.warn("Attack Plan " + this.type + " " + this.name + " has arrived to destination.");
		this.state = "arrived";
		return true;
	}
	else if (this.path.length && API3.SquareVectorDistance(this.position, this.path[0]) < 1600)
	{
		this.path.shift();
		if (this.path.length)
			this.unitCollection.moveToRange(this.path[0][0], this.path[0][1], 0, 15);
		else
		{
			if (this.Config.debug > 1)
				API3.warn("Attack Plan " + this.type + " " + this.name + " has arrived to destination.");
			this.state = "arrived";
			return true;
		}
	}

	return true;
};

m.AttackPlan.prototype.UpdateTarget = function(gameState)
{
	// First update the target position in case it's a unit (and check if it has garrisoned)
	if (this.target && this.target.hasClass("Unit"))
	{
		this.targetPos = this.target.position();
		if (!this.targetPos)
		{
			let holder = m.getHolder(gameState, this.target);
			if (holder && gameState.isPlayerEnemy(holder.owner()))
			{
				this.target = holder;
				this.targetPos = holder.position();
			}
			else
				this.target = undefined;
		}
	}
	// Then update the target if needed:
	if (this.targetPlayer === undefined || !gameState.isPlayerEnemy(this.targetPlayer))
	{
		this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
		if (this.targetPlayer === undefined)
			return false;

		if (this.target && this.target.owner() !== this.targetPlayer)
			this.target = undefined;
	}
	if (this.target && this.target.owner() === 0 && this.targetPlayer !== 0)  // this enemy has resigned
		this.target = undefined;

	if (!this.target || !gameState.getEntityById(this.target.id()))
	{
		if (this.Config.debug > 1)
			API3.warn("Seems like our target for plan " + this.name + " has been destroyed or captured. Switching.");
		let accessIndex = this.getAttackAccess(gameState);
		this.target = this.getNearestTarget(gameState, this.position, accessIndex);
		if (!this.target)
		{
			if (this.uniqueTargetId)
				return false;

			// Check if we could help any current attack
			let attackManager = gameState.ai.HQ.attackManager;
			for (let attackType in attackManager.startedAttacks)
			{
				for (let attack of attackManager.startedAttacks[attackType])
				{
					if (attack.name == this.name)
						continue;
					if (!attack.target || !gameState.getEntityById(attack.target.id()) ||
					    !gameState.isPlayerEnemy(attack.target.owner()))
						continue;
					if (accessIndex != m.getLandAccess(gameState, attack.target))
						continue;
					if (attack.target.owner() == 0 && attack.targetPlayer != 0)	// looks like it has resigned
						continue;
					if (!gameState.isPlayerEnemy(attack.targetPlayer))
						continue;
					this.target = attack.target;
					this.targetPlayer = attack.targetPlayer;
					this.targetPos = this.target.position();
					return true;
				}
			}

			// If not, let's look for another enemy
			if (!this.target)
			{
				this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
				if (this.targetPlayer !== undefined)
					this.target = this.getNearestTarget(gameState, this.position, accessIndex);
				if (!this.target)
				{
					if (this.Config.debug > 1)
						API3.warn("No new target found. Remaining units " + this.unitCollection.length);
					return false;
				}
			}
			if (this.Config.debug > 1)
				API3.warn("We will help one of our other attacks");
		}
		this.targetPos = this.target.position();
	}
	return true;
};

/** reset any units */
m.AttackPlan.prototype.Abort = function(gameState)
{
	this.unitCollection.unregister();
	if (this.unitCollection.hasEntities())
	{
		// If the attack was started, look for a good rallyPoint to withdraw
		let rallyPoint;
		if (this.isStarted())
		{
			let access = this.getAttackAccess(gameState);
			let dist = Math.min();
			if (this.rallyPoint && gameState.ai.accessibility.getAccessValue(this.rallyPoint) == access)
			{
				rallyPoint = this.rallyPoint;
				dist = API3.SquareVectorDistance(this.position, rallyPoint);
			}
			// Then check if we have a nearer base (in case this attack has captured one)
			for (let base of gameState.ai.HQ.baseManagers)
			{
				if (!base.anchor || !base.anchor.position())
					continue;
				if (m.getLandAccess(gameState, base.anchor) != access)
					continue;
				let newdist = API3.SquareVectorDistance(this.position, base.anchor.position());
				if (newdist > dist)
					continue;
				dist = newdist;
				rallyPoint = base.anchor.position();
			}
		}

		for (let ent of this.unitCollection.values())
		{
			if (ent.getMetadata(PlayerID, "role") == "attack")
				ent.stopMoving();
			if (rallyPoint)
				ent.moveToRange(rallyPoint[0], rallyPoint[1], 0, 15);
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
	if (ent.getMetadata(PlayerID, "role") == "attack")
	{
		if (ent.hasClass("CitizenSoldier"))
			ent.setMetadata(PlayerID, "role", "worker");
		else
			ent.setMetadata(PlayerID, "role", undefined);
		ent.setMetadata(PlayerID, "subrole", undefined);
	}
	ent.setMetadata(PlayerID, "plan", -1);
	if (update)
		this.unitCollection.updateEnt(ent);
};

m.AttackPlan.prototype.checkEvents = function(gameState, events)
{
	for (let evt of events.EntityRenamed)
	{
		if (!this.target || this.target.id() != evt.entity)
			continue;
		if (this.type == "Raid" && !this.isStarted())
			this.target = undefined;
		else
			this.target = gameState.getEntityById(evt.newentity);
		if (this.target)
			this.targetPos = this.target.position();
	}

	for (let evt of events.OwnershipChanged)	// capture event
		if (this.target && this.target.id() == evt.entity && gameState.isPlayerAlly(evt.to))
			this.target = undefined;

	for (let evt of events.PlayerDefeated)
	{
		if (this.targetPlayer !== evt.playerId)
			continue;
		this.targetPlayer = gameState.ai.HQ.attackManager.getEnemyPlayer(gameState, this);
		this.target = undefined;
	}

	if (!this.overseas || this.state !== "unexecuted")
		return;
	// let's check if an enemy has built a structure at our access
	for (let evt of events.Create)
	{
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.position() || !ent.hasClass("Structure"))
			continue;
		if (!gameState.isPlayerEnemy(ent.owner()))
			continue;
		let access = m.getLandAccess(gameState, ent);
		for (let base of gameState.ai.HQ.baseManagers)
		{
			if (!base.anchor || !base.anchor.position())
				continue;
			if (base.accessIndex != access)
				continue;
			this.overseas = 0;
			this.rallyPoint = base.anchor.position();
		}
	}
};

m.AttackPlan.prototype.waitingForTransport = function()
{
	for (let ent of this.unitCollection.values())
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			return true;
	return false;
};

m.AttackPlan.prototype.hasSiegeUnits = function()
{
	for (let ent of this.unitCollection.values())
		if (m.isSiegeUnit(ent))
			return true;
	return false;
};

m.AttackPlan.prototype.hasForceOrder = function(data, value)
{
	for (let ent of this.unitCollection.values())
	{
		if (data && +ent.getMetadata(PlayerID, data) !== value)
			continue;
		let orders = ent.unitAIOrderData();
		for (let order of orders)
			if (order.force)
				return true;
	}
	return false;
};

/**
 * The center position of this attack may be in an inaccessible area. So we use the access
 * of the unit nearest to this center position.
 */
m.AttackPlan.prototype.getAttackAccess = function(gameState)
{
	for (let ent of this.unitCollection.filterNearest(this.position, 1).values())
		return m.getLandAccess(gameState, ent);

	return 0;
};

m.AttackPlan.prototype.debugAttack = function()
{
	API3.warn("---------- attack " + this.name);
	for (let unitCat in this.unitStat)
	{
		let Unit = this.unitStat[unitCat];
		API3.warn(unitCat + " num=" + this.unit[unitCat].length + " min=" + Unit.minSize + " need=" + Unit.targetSize);
	}
	API3.warn("------------------------------");
};

m.AttackPlan.prototype.Serialize = function()
{
	let properties = {
		"name": this.name,
		"type": this.type,
		"state": this.state,
		"forced": this.forced,
		"rallyPoint": this.rallyPoint,
		"overseas": this.overseas,
		"paused": this.paused,
		"maxCompletingTime": this.maxCompletingTime,
		"neededShips": this.neededShips,
		"unitStat": this.unitStat,
		"siegeState": this.siegeState,
		"position5TurnsAgo": this.position5TurnsAgo,
		"lastPosition": this.lastPosition,
		"position": this.position,
		"isBlocked": this.isBlocked,
		"targetPlayer": this.targetPlayer,
		"target": this.target !== undefined ? this.target.id() : undefined,
		"targetPos": this.targetPos,
		"uniqueTargetId": this.uniqueTargetId,
		"path": this.path
	};

	return { "properties": properties };
};

m.AttackPlan.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.properties)
		this[key] = data.properties[key];

	if (this.target)
		this.target = gameState.getEntityById(this.target);

	this.failed = undefined;
};

return m;
}(PETRA);
