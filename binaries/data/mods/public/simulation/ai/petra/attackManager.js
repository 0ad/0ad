var PETRA = function(m)
{

/* Attack Manager
 */

m.AttackManager = function(Config)
{
	this.Config = Config;

	this.totalNumber = 0;
	this.attackNumber = 0;
	this.rushNumber = 0;
	this.raidNumber = 0;
	this.upcomingAttacks = { "Rush": [], "Raid": [], "Attack": [], "HugeAttack": [] };
	this.startedAttacks = { "Rush": [], "Raid": [], "Attack": [], "HugeAttack": [] };
	this.debugTime = 0;
	this.maxRushes = 0;
	this.rushSize = [];
	this.currentEnemyPlayer = undefined; // enemy player we are currently targeting
};

// More initialisation for stuff that needs the gameState
m.AttackManager.prototype.init = function(gameState)
{
	this.outOfPlan = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "plan", -1));
	this.outOfPlan.registerUpdates();
};

m.AttackManager.prototype.setRushes = function()
{
	if (this.Config.personality.aggressive > 0.8)
	{
		this.maxRushes = 3
		this.rushSize = [ 16, 22, 28 ];
	}
	else if (this.Config.personality.aggressive > 0.6)
	{
		this.maxRushes = 2;
		this.rushSize = [ 18, 28 ];
	}
	else if (this.Config.personality.aggressive > 0.3)
	{
		this.maxRushes = 1;
		this.rushSize = [ 24 ];
	}
};

m.AttackManager.prototype.checkEvents = function(gameState, events)
{
	let PlayerEvents = events["AttackRequest"];
	let answer = false;
	let other = undefined;
	let targetPlayer = undefined;
	for (let evt of PlayerEvents)
	{
		if (evt.source === PlayerID || !gameState.isPlayerAlly(evt.source) || !gameState.isPlayerEnemy(evt.target))
			continue;
		targetPlayer = evt.target;
		let available = 0;
		for (let attackType in this.upcomingAttacks)
		{
			for (let attack of this.upcomingAttacks[attackType])
			{
				if (attack.state === "completing")
				{
					if (attack.targetPlayer && attack.targetPlayer === targetPlayer)
						available += attack.unitCollection.length;
					else if (attack.targetPlayer && attack.targetPlayer !== targetPlayer)
						other = attack.targetPlayer;
					continue;
				}
				if (!attack.targetPlayer || attack.targetPlayer !== targetPlayer)
				{
					let oldTargetPlayer = attack.targetPlayer;
					let oldTarget = attack.target;
					attack.targetPlayer = targetPlayer;
					attack.target = attack.getNearestTarget(gameState, attack.rallyPoint);
					if (!attack.target)
					{
						attack.targetPlayer = oldTargetPlayer;
						attack.target = oldTarget;
						continue;
					}
					attack.targetPos = attack.target.position();
				}
				if (attack.targetPlayer && attack.targetPlayer === targetPlayer)
					available += attack.unitCollection.length;
			}
		}

		if (available > 12)	// launch the attack immediately
		{
			for (let attackType in this.upcomingAttacks)
			{
				for (let attack of this.upcomingAttacks[attackType])
				{
					if (attack.state !== "completing" && attack.targetPlayer && attack.targetPlayer === targetPlayer)
					{
						attack.forceStart();
						attack.requested = true;
					}
				}
			}
			answer = true;
		}
		break;  // take only the first attack request into account
	}
	if (targetPlayer)
		m.chatAnswerRequestAttack(gameState, targetPlayer, answer, other);
};

// Some functions are run every turn
// Others once in a while
m.AttackManager.prototype.update = function(gameState, queues, events)
{
	if (this.Config.debug > 2 && gameState.ai.elapsedTime > this.debugTime + 60)
	{
		this.debugTime = gameState.ai.elapsedTime;
		API3.warn(" upcoming attacks =================");
		for (var attackType in this.upcomingAttacks)
			for (let attack of this.upcomingAttacks[attackType])
				API3.warn(" plan " + attack.name + " type " + attackType + " state " + attack.state  + " units " + attack.unitCollection.length);
		API3.warn(" started attacks ==================");
		for (var attackType in this.startedAttacks)
			for (let attack of this.startedAttacks[attackType])
				API3.warn(" plan " + attack.name + " type " + attackType + " state " + attack.state + " units " + attack.unitCollection.length);
		API3.warn(" ==================================");
	}

	this.checkEvents(gameState, events);

	let unexecutedAttacks = {"Rush": 0, "Raid": 0, "Attack": 0, "HugeAttack": 0};
	for (let attackType in this.upcomingAttacks)
	{
		for (let i = 0; i < this.upcomingAttacks[attackType].length; ++i)
		{
			let attack = this.upcomingAttacks[attackType][i];
			attack.checkEvents(gameState, events);

			if (attack.isStarted())
				API3.warn("Petra problem in attackManager: attack in preparation has already started ???");

			let updateStep = attack.updatePreparation(gameState);
			// now we're gonna check if the preparation time is over
			if (updateStep === 1 || attack.isPaused() )
			{
				// just chillin'
				if (attack.state === "unexecuted")
					++unexecutedAttacks[attackType];
			}
			else if (updateStep === 0 || updateStep === 3)
			{
				if (this.Config.debug > 1)
					API3.warn("Attack Manager: " + attack.getType() + " plan " + attack.getName() + " aborted.");
				attack.Abort(gameState);
				this.upcomingAttacks[attackType].splice(i--,1);
			}
			else if (updateStep === 2)
			{
				if (attack.StartAttack(gameState))
				{
					if (this.Config.debug > 1)
						API3.warn("Attack Manager: Starting " + attack.getType() + " plan " + attack.getName());
					if (this.Config.chat)
						m.chatLaunchAttack(gameState, attack.targetPlayer);
					this.startedAttacks[attackType].push(attack);
				}
				else
					attack.Abort(gameState);
				this.upcomingAttacks[attackType].splice(i--,1);
			}
		}
	}

	for (var attackType in this.startedAttacks)
	{
		for (var i = 0; i < this.startedAttacks[attackType].length; ++i)
		{
			var attack = this.startedAttacks[attackType][i];
			attack.checkEvents(gameState, events);
			// okay so then we'll update the attack.
			if (attack.isPaused())
				continue;
			var remaining = attack.update(gameState, events);
			if (!remaining)
			{
				if (this.Config.debug > 1)
					API3.warn("Military Manager: " + attack.getType() + " plan " + attack.getName() + " is finished with remaining " + remaining);
				attack.Abort(gameState);
				this.startedAttacks[attackType].splice(i--,1);
			}
		}
	}

	// creating plans after updating because an aborted plan might be reused in that case.

	var barracksNb = gameState.getOwnEntitiesByClass("Barracks", true).filter(API3.Filters.isBuilt()).length;
	if (this.rushNumber < this.maxRushes && barracksNb >= 1)
	{
		if (unexecutedAttacks["Rush"] === 0)
		{
			// we have a barracks and we want to rush, rush.
			var data = { "targetSize": this.rushSize[this.rushNumber] };
			var attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, "Rush", data);
			if (!attackPlan.failed)
			{
				if (this.Config.debug > 1)
					API3.warn("Headquarters: Rushing plan " + this.totalNumber + " with maxRushes " + this.maxRushes);
				this.totalNumber++;
				attackPlan.init(gameState);
				this.upcomingAttacks["Rush"].push(attackPlan);
			}
			this.rushNumber++;
		}
	}
	else if (unexecutedAttacks["Attack"] === 0 && unexecutedAttacks["HugeAttack"] === 0
		&& (this.startedAttacks["Attack"].length + this.startedAttacks["HugeAttack"].length < Math.min(2, 1 + Math.round(gameState.getPopulationMax()/100))))
	{
		if ((barracksNb >= 1 && (gameState.currentPhase() > 1 || gameState.isResearching(gameState.townPhase())))
			|| !gameState.ai.HQ.baseManagers[1])	// if we have no base ... nothing else to do than attack
		{
			if (this.attackNumber < 2 || this.startedAttacks["HugeAttack"].length > 0)
				var type = "Attack";
			else
				var type = "HugeAttack";

			var attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, type);
			if (attackPlan.failed)
				this.attackPlansEncounteredWater = true; // hack
			else
			{
				if (this.Config.debug > 1)
					API3.warn("Military Manager: Creating the plan " + type + "  " + this.totalNumber);
				this.totalNumber++;
				attackPlan.init(gameState);
				this.upcomingAttacks[type].push(attackPlan);
			}
			this.attackNumber++;
		}
	}

	if (unexecutedAttacks["Raid"] === 0 && gameState.ai.HQ.defenseManager.targetList.length)
	{
		var target = undefined;
		for (var targetId of gameState.ai.HQ.defenseManager.targetList)
		{
			target = gameState.getEntityById(targetId);
			if (target)
				break;
		}
		if (target)
		{
			// prepare a raid against this target
			var data = { "target": target };
			var attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, "Raid", data);
			if (!attackPlan.failed)
			{
				if (this.Config.debug > 1)
					API3.warn("Headquarters: Raiding plan " + this.totalNumber);
				this.totalNumber++;
				attackPlan.init(gameState);
				this.upcomingAttacks["Raid"].push(attackPlan);
			}
			this.raidNumber++;
		}
	}
};

m.AttackManager.prototype.getPlan = function(planName)
{
	for (var attackType in this.upcomingAttacks)
	{
		for (var attack of this.upcomingAttacks[attackType])
			if (attack.getName() == planName)
				return attack;
	}
	for (var attackType in this.startedAttacks)
	{
		for (var attack of this.startedAttacks[attackType])
			if (attack.getName() == planName)
				return attack;
	}
	return undefined;
};

m.AttackManager.prototype.pausePlan = function(planName)
{
	var attack = this.getPlan(planName);
	if (attack)
		attack.setPaused(true);
};

m.AttackManager.prototype.unpausePlan = function(planName)
{
	var attack = this.getPlan(planName);
	if (attack)
		attack.setPaused(falsee);
};

m.AttackManager.prototype.pauseAllPlans = function()
{
	for (var attackType in this.upcomingAttacks)
		for (var attack of this.upcomingAttacks[attackType])
			attack.setPaused(true);

	for (var attackType in this.startedAttacks)
		for (var attack of this.startedAttacks[attackType])
			attack.setPaused(true);
};

m.AttackManager.prototype.unpauseAllPlans = function()
{
	for (var attackType in this.upcomingAttacks)
		for (var attack of this.upcomingAttacks[attackType])
			attack.setPaused(false);

	for (var attackType in this.startedAttacks)
		for (var attack of this.startedAttacks[attackType])
			attack.setPaused(false);
};

m.AttackManager.prototype.getAttackInPreparation = function(type)
{
	if (!this.upcomingAttacks[type].length)
		return undefined;
	return this.upcomingAttacks[type][0];
};

/**
 * determine which player should be attacked:
 * - when called when starting the attack, attack.targetPlayer is undefined
 */
m.AttackManager.prototype.getEnemyPlayer = function(gameState, attack)
{
	var enemyPlayer = undefined;

	// first check if there is a preferred enemy based on our victory conditions
	if (gameState.getGameType() === "wonder")
	{
		var moreAdvanced = undefined;
		var enemyWonder = undefined;
		var wonders = gameState.getEnemyStructures().filter(API3.Filters.byClass("Wonder"));
		for (let wonder of wonders.values())
		{
			let progress = wonder.foundationProgress();
			if (progress === undefined)
				return wonder.owner();
			if (enemyWonder && moreAdvanced > progress)
				continue;
			enemyWonder = wonder;
			moreAdvanced = progress;
		}
		if (enemyWonder)
		{
			enemyPlayer = enemyWonder.owner();
			if (attack.targetPlayer === undefined)
				this.currentEnemyPlayer = enemyPlayer;
			return enemyPlayer;
		}
	}

	// No rush if enemy too well defended (i.e. iberians) 
	var veto = {};
	if (attack.type === "Rush")
	{
		for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
		{
			if (!gameState.isPlayerEnemy(i))
				continue;
			let enemyDefense = 0;
			for (let ent of gameState.getEnemyStructures(i).values())
				if (ent.hasClass("Tower") || ent.hasClass("Fortress"))
					enemyDefense++;
			if (enemyDefense > 6)
				veto[i] = true;
		}
	} 

	// then if not a huge attack, continue attacking our previous target as long as it has some entities, 
	// otherwise target the most accessible one
	if (attack.type !== "HugeAttack")
	{
		if (attack.targetPlayer === undefined && this.currentEnemyPlayer !== undefined
			&& gameState.getEnemyEntities(this.currentEnemyPlayer) > 0)
			return this.currentEnemyPlayer;

		let distmin = undefined;
		let ccmin = undefined;
		let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
		for (let ourcc of ccEnts.values())
		{
			if (ourcc.owner() !== PlayerID)
				continue;
			let ourPos = ourcc.position();
			let access = gameState.ai.accessibility.getAccessValue(ourPos);
			for (let enemycc of ccEnts.values())
			{
				if (veto[enemycc.owner()])
					continue;
				if (!gameState.isPlayerEnemy(enemycc.owner()))
					continue;
				let enemyPos = enemycc.position();
				if (access !== gameState.ai.accessibility.getAccessValue(enemyPos))
					continue;
				let dist = API3.SquareVectorDistance(ourPos, enemyPos);
				if (distmin && dist > distmin)
					continue;
				ccmin = enemycc;
				distmin = dist;
			}
		}
		if (ccmin)
		{
			enemyPlayer = ccmin.owner();
			if (attack.targetPlayer === undefined)
				this.currentEnemyPlayer = enemyPlayer;
			return enemyPlayer;
		}
	}

	// then let's target our strongest enemy (basically counting enemies units)
	// with priority to enemies with civ center
	var max = 0;
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (veto[i])
			continue;
		if (!gameState.isPlayerEnemy(i))
			continue;
		let enemyCount = 0;
		let enemyCivCentre = false;
		for (let ent of gameState.getEnemyEntities(i).values())
		{
			enemyCount++;
			if (ent.hasClass("CivCentre"))
				enemyCivCentre = true;
		}
		if (enemyCivCentre)
			enemyCount += 500;
		if (enemyCount < max)
			continue;
		max = enemyCount;
		enemyPlayer = i;
	}
	if (attack.targetPlayer === undefined)
		this.currentEnemyPlayer = enemyPlayer;
	return enemyPlayer;
};

m.AttackManager.prototype.Serialize = function()
{
	let properties = {
		"totalNumber": this.totalNumber,
		"attackNumber": this.attackNumber,
		"rushNumber": this.rushNumber,
		"raidNumber": this.raidNumber,
		"debugTime": this.debugTime,
		"maxRushes": this.maxRushes,
		"rushSize": this.rushSize,
		"currentEnemyPlayer": this.currentEnemyPlayer
	};

	let upcomingAttacks = {};
	for (let key in this.upcomingAttacks)
	{
		upcomingAttacks[key] = [];
		for (let attack of this.upcomingAttacks[key])
			upcomingAttacks[key].push(attack.Serialize());
	};

	let startedAttacks = {};
	for (let key in this.startedAttacks)
	{
		startedAttacks[key] = [];
		for (let attack of this.startedAttacks[key])
			startedAttacks[key].push(attack.Serialize());
	};

	return { "properties": properties, "upcomingAttacks": upcomingAttacks, "startedAttacks": startedAttacks };
};

m.AttackManager.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.properties)
		this[key] = data.properties[key];

	this.upcomingAttacks = {};
	for (let key in data.upcomingAttacks)
	{
		this.upcomingAttacks[key] = [];
		for (let dataAttack of data.upcomingAttacks[key])
		{
			let attack =  new m.AttackPlan(gameState, this.Config, 0);
			attack.Deserialize(gameState, dataAttack);
			attack.init(gameState);
			this.upcomingAttacks[key].push(attack);
		}
	};

	this.startedAttacks = {};
	for (let key in data.startedAttacks)
	{
		this.startedAttacks[key] = [];
		for (let dataAttack of data.startedAttacks[key])
		{
			let attack =  new m.AttackPlan(gameState, this.Config, 0);
			attack.Deserialize(gameState, dataAttack);
			attack.init(gameState);
			this.startedAttacks[key].push(attack);
		}
	};
};

return m;
}(PETRA);
