var PETRA = function(m)
{

/** Attack Manager */

m.AttackManager = function(Config)
{
	this.Config = Config;

	this.totalNumber = 0;
	this.attackNumber = 0;
	this.rushNumber = 0;
	this.raidNumber = 0;
	this.upcomingAttacks = { "Rush": [], "Raid": [], "Attack": [], "HugeAttack": [] };
	this.startedAttacks = { "Rush": [], "Raid": [], "Attack": [], "HugeAttack": [] };
	this.bombingAttacks = new Map();// Temporary attacks for siege units while waiting their current attack to start
	this.debugTime = 0;
	this.maxRushes = 0;
	this.rushSize = [];
	this.currentEnemyPlayer = undefined; // enemy player we are currently targeting
	this.defeated = {};
};

/** More initialisation for stuff that needs the gameState */
m.AttackManager.prototype.init = function(gameState)
{
	this.outOfPlan = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "plan", -1));
	this.outOfPlan.registerUpdates();
};

m.AttackManager.prototype.setRushes = function(allowed)
{
	if (this.Config.personality.aggressive > this.Config.personalityCut.strong && allowed > 2)
	{
		this.maxRushes = 3;
		this.rushSize = [ 16, 20, 24 ];
	}
	else if (this.Config.personality.aggressive > this.Config.personalityCut.medium && allowed > 1)
	{
		this.maxRushes = 2;
		this.rushSize = [ 18, 22 ];
	}
	else if (this.Config.personality.aggressive > this.Config.personalityCut.weak && allowed > 0)
	{
		this.maxRushes = 1;
		this.rushSize = [ 20 ];
	}
};

m.AttackManager.prototype.checkEvents = function(gameState, events)
{
	for (let evt of events.PlayerDefeated)
		this.defeated[evt.playerId] = true;

	let answer = "decline";
	let other;
	let targetPlayer;
	for (let evt of events.AttackRequest)
	{
		if (evt.source === PlayerID || !gameState.isPlayerAlly(evt.source) || !gameState.isPlayerEnemy(evt.player))
			continue;
		targetPlayer = evt.player;
		let available = 0;
		for (let attackType in this.upcomingAttacks)
		{
			for (let attack of this.upcomingAttacks[attackType])
			{
				if (attack.state === "completing")
				{
					if (attack.targetPlayer === targetPlayer)
						available += attack.unitCollection.length;
					else if (attack.targetPlayer !== undefined && attack.targetPlayer !== targetPlayer)
						other = attack.targetPlayer;
					continue;
				}

				attack.targetPlayer = targetPlayer;

				if (attack.unitCollection.length > 2)
					available += attack.unitCollection.length;
			}
		}

		if (available > 12)	// launch the attack immediately
		{
			for (let attackType in this.upcomingAttacks)
			{
				for (let attack of this.upcomingAttacks[attackType])
				{
					if (attack.state === "completing" ||
						attack.targetPlayer !== targetPlayer ||
						attack.unitCollection.length < 3)
						continue;
					attack.forceStart();
					attack.requested = true;
				}
			}
			answer = "join";
		}
		else if (other !== undefined)
			answer = "other";
		break;  // take only the first attack request into account
	}
	if (targetPlayer !== undefined)
		m.chatAnswerRequestAttack(gameState, targetPlayer, answer, other);

	for (let evt of events.EntityRenamed)	// take care of packing units in bombing attacks
	{
		for (let [targetId, unitIds] of this.bombingAttacks)
		{
			if (targetId == evt.entity)
			{
				this.bombingAttacks.set(evt.newentity, unitIds);
				this.bombingAttacks.delete(evt.entity);
			}
			else if (unitIds.has(evt.entity))
			{
				unitIds.add(evt.newentity);
				unitIds.delete(evt.entity);
			}
		}
	}
};

/**
 * Check for any structure in range from within our territory, and bomb it
 */
m.AttackManager.prototype.assignBombers = function(gameState)
{
	// First some cleaning of current bombing attacks
	for (let [targetId, unitIds] of this.bombingAttacks)
	{
		let target = gameState.getEntityById(targetId);
		if (!target || !gameState.isPlayerEnemy(target.owner()))
			this.bombingAttacks.delete(targetId);
		else
		{
			for (let entId of unitIds.values())
			{
				let ent = gameState.getEntityById(entId);
				if (ent && ent.owner() == PlayerID)
				{
					let plan = ent.getMetadata(PlayerID, "plan");
					let orders = ent.unitAIOrderData();
					let lastOrder = orders && orders.length ? orders[orders.length-1] : null;
					if (lastOrder && lastOrder.target && lastOrder.target == targetId && plan != -2 && plan != -3)
						continue;
				}
				unitIds.delete(entId);
			}
			if (!unitIds.size)
				this.bombingAttacks.delete(targetId);
		}
	}

	let bombers = gameState.updatingCollection("bombers", API3.Filters.byClassesOr(["BoltShooter", "Catapult"]), gameState.getOwnUnits());
	for (let ent of bombers.values())
	{
		if (!ent.position() || !ent.isIdle() || !ent.attackRange("Ranged"))
			continue;
		if (ent.getMetadata(PlayerID, "plan") == -2 || ent.getMetadata(PlayerID, "plan") == -3)
			continue;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") != -1)
		{
			let subrole = ent.getMetadata(PlayerID, "subrole");
			if (subrole && (subrole == "completing" || subrole == "walking" || subrole == "attacking"))
				continue;
		}
		let alreadyBombing = false;
		for (let unitIds of this.bombingAttacks.values())
		{
			if (!unitIds.has(ent.id()))
				continue;
			alreadyBombing = true;
			break;
		}
		if (alreadyBombing)
			break;

		let range =  ent.attackRange("Ranged").max;
		let entPos = ent.position();
		let access = gameState.ai.accessibility.getAccessValue(entPos);
		for (let struct of gameState.getEnemyStructures().values())
		{
			let structPos = struct.position();
			let x;
			let z;
			if (struct.hasClass("Field"))
			{
				if (!struct.resourceSupplyNumGatherers() ||
				    !gameState.isPlayerEnemy(gameState.ai.HQ.territoryMap.getOwner(structPos)))
					continue;
			}
			let dist = API3.VectorDistance(entPos, structPos);
			if (dist > range)
			{
				let safety = struct.footprintRadius() + 30;
				x = structPos[0] + (entPos[0] - structPos[0]) * safety / dist;
				z = structPos[1] + (entPos[1] - structPos[1]) * safety / dist;
				let owner = gameState.ai.HQ.territoryMap.getOwner([x, z]);
				if (owner != 0 && gameState.isPlayerEnemy(owner))
					continue;
				x = structPos[0] + (entPos[0] - structPos[0]) * range / dist;
				z = structPos[1] + (entPos[1] - structPos[1]) * range / dist;
				if (gameState.ai.HQ.territoryMap.getOwner([x, z]) != PlayerID ||
				    gameState.ai.accessibility.getAccessValue([x, z]) != access)
					continue;
			}
			let attackingUnits;
			for (let [targetId, unitIds] of this.bombingAttacks)
			{
				if (targetId != struct.id())
					continue;
				attackingUnits = unitIds;
				break;
			}
			if (attackingUnits && attackingUnits.size > 4)
				continue;	// already enough units against that target
			if (!attackingUnits)
			{
				attackingUnits = new Set();
				this.bombingAttacks.set(struct.id(), attackingUnits);
			}
			attackingUnits.add(ent.id());
			if (dist > range)
				ent.move(x, z);
			ent.attack(struct.id(), false, dist > range);
			break;
		}
	}
};

/**
 * Some functions are run every turn
 * Others once in a while
 */
m.AttackManager.prototype.update = function(gameState, queues, events)
{
	if (this.Config.debug > 2 && gameState.ai.elapsedTime > this.debugTime + 60)
	{
		this.debugTime = gameState.ai.elapsedTime;
		API3.warn(" upcoming attacks =================");
		for (let attackType in this.upcomingAttacks)
			for (let attack of this.upcomingAttacks[attackType])
				API3.warn(" plan " + attack.name + " type " + attackType + " state " + attack.state + " units " + attack.unitCollection.length);
		API3.warn(" started attacks ==================");
		for (let attackType in this.startedAttacks)
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
			else if (updateStep === 0)
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
						m.chatLaunchAttack(gameState, attack.targetPlayer, attack.getType());
					this.startedAttacks[attackType].push(attack);
				}
				else
					attack.Abort(gameState);
				this.upcomingAttacks[attackType].splice(i--,1);
			}
		}
	}

	for (let attackType in this.startedAttacks)
	{
		for (let i = 0; i < this.startedAttacks[attackType].length; ++i)
		{
			let attack = this.startedAttacks[attackType][i];
			attack.checkEvents(gameState, events);
			// okay so then we'll update the attack.
			if (attack.isPaused())
				continue;
			let remaining = attack.update(gameState, events);
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

	let barracksNb = gameState.getOwnEntitiesByClass("Barracks", true).filter(API3.Filters.isBuilt()).length;
	if (this.rushNumber < this.maxRushes && barracksNb >= 1)
	{
		if (unexecutedAttacks.Rush === 0)
		{
			// we have a barracks and we want to rush, rush.
			let data = { "targetSize": this.rushSize[this.rushNumber] };
			let attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, "Rush", data);
			if (!attackPlan.failed)
			{
				if (this.Config.debug > 1)
					API3.warn("Military Manager: Rushing plan " + this.totalNumber + " with maxRushes " + this.maxRushes);
				this.totalNumber++;
				attackPlan.init(gameState);
				this.upcomingAttacks.Rush.push(attackPlan);
			}
			this.rushNumber++;
		}
	}
	else if (unexecutedAttacks.Attack == 0 && unexecutedAttacks.HugeAttack == 0 &&
		this.startedAttacks.Attack.length + this.startedAttacks.HugeAttack.length < Math.min(2, 1 + Math.round(gameState.getPopulationMax()/100)) &&
		(this.startedAttacks.Attack.length + this.startedAttacks.HugeAttack.length == 0 || gameState.getPopulationMax() - gameState.getPopulation() > 12))
	{
		if (barracksNb >= 1 && (gameState.currentPhase() > 1 || gameState.isResearching(gameState.getPhaseName(2))) ||
			!gameState.ai.HQ.baseManagers[1])	// if we have no base ... nothing else to do than attack
		{
			let type = this.attackNumber < 2 || this.startedAttacks.HugeAttack.length > 0 ? "Attack" : "HugeAttack";
			let attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, type);
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

	if (unexecutedAttacks.Raid === 0 && gameState.ai.HQ.defenseManager.targetList.length)
	{
		let target;
		for (let targetId of gameState.ai.HQ.defenseManager.targetList)
		{
			target = gameState.getEntityById(targetId);
			if (!target)
				continue;
			if (gameState.isPlayerEnemy(target.owner()))
				break;
			target = undefined;
		}
		if (target) // prepare a raid against this target
			this.raidTargetEntity(gameState, target);
	}

	// Check if we have some unused ranged siege unit which could do something useful while waiting
	if (this.Config.difficulty > 1 && gameState.ai.playedTurn % 5 == 0)
		this.assignBombers(gameState);
};

m.AttackManager.prototype.getPlan = function(planName)
{
	for (let attackType in this.upcomingAttacks)
	{
		for (let attack of this.upcomingAttacks[attackType])
			if (attack.getName() == planName)
				return attack;
	}
	for (let attackType in this.startedAttacks)
	{
		for (let attack of this.startedAttacks[attackType])
			if (attack.getName() == planName)
				return attack;
	}
	return undefined;
};

m.AttackManager.prototype.pausePlan = function(planName)
{
	let attack = this.getPlan(planName);
	if (attack)
		attack.setPaused(true);
};

m.AttackManager.prototype.unpausePlan = function(planName)
{
	let attack = this.getPlan(planName);
	if (attack)
		attack.setPaused(false);
};

m.AttackManager.prototype.pauseAllPlans = function()
{
	for (let attackType in this.upcomingAttacks)
		for (let attack of this.upcomingAttacks[attackType])
			attack.setPaused(true);

	for (let attackType in this.startedAttacks)
		for (let attack of this.startedAttacks[attackType])
			attack.setPaused(true);
};

m.AttackManager.prototype.unpauseAllPlans = function()
{
	for (let attackType in this.upcomingAttacks)
		for (let attack of this.upcomingAttacks[attackType])
			attack.setPaused(false);

	for (let attackType in this.startedAttacks)
		for (let attack of this.startedAttacks[attackType])
			attack.setPaused(false);
};

m.AttackManager.prototype.getAttackInPreparation = function(type)
{
	return this.upcomingAttacks[type].length ? this.upcomingAttacks[type][0] : undefined;
};

/**
 * Determine which player should be attacked: when called when starting the attack,
 * attack.targetPlayer is undefined and in that case, we keep track of the chosen target
 * for future attacks.
 */
m.AttackManager.prototype.getEnemyPlayer = function(gameState, attack)
{
	let enemyPlayer;

	// First check if there is a preferred enemy based on our victory conditions.
	// If both wonder and relic, choose randomly between them TODO should combine decisions

	if (gameState.getVictoryConditions().has("wonder"))
		enemyPlayer = this.getWonderEnemyPlayer(gameState, attack);

	if (gameState.getVictoryConditions().has("capture_the_relic"))
		if (!enemyPlayer || randBool())
			enemyPlayer = this.getRelicEnemyPlayer(gameState, attack) || enemyPlayer;

	if (enemyPlayer)
		return enemyPlayer;

	let veto = {};
	for (let i in this.defeated)
		veto[i] = true;
	// No rush if enemy too well defended (i.e. iberians)
	if (attack.type == "Rush")
	{
		for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
		{
			if (!gameState.isPlayerEnemy(i) || veto[i])
				continue;
			if (this.defeated[i])
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
	if (attack.type != "HugeAttack")
	{
		if (attack.targetPlayer === undefined && this.currentEnemyPlayer !== undefined &&
			!this.defeated[this.currentEnemyPlayer] &&
			gameState.isPlayerEnemy(this.currentEnemyPlayer) &&
			gameState.getEntities(this.currentEnemyPlayer).hasEntities())
			return this.currentEnemyPlayer;

		let distmin;
		let ccmin;
		let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
		for (let ourcc of ccEnts.values())
		{
			if (ourcc.owner() != PlayerID)
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
				if (access != gameState.ai.accessibility.getAccessValue(enemyPos))
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
	let max = 0;
	for (let i = 1; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (veto[i])
			continue;
		if (!gameState.isPlayerEnemy(i))
			continue;
		let enemyCount = 0;
		let enemyCivCentre = false;
		for (let ent of gameState.getEntities(i).values())
		{
			enemyCount++;
			if (ent.hasClass("CivCentre"))
				enemyCivCentre = true;
		}
		if (enemyCivCentre)
			enemyCount += 500;
		if (!enemyCount || enemyCount < max)
			continue;
		max = enemyCount;
		enemyPlayer = i;
	}
	if (attack.targetPlayer === undefined)
		this.currentEnemyPlayer = enemyPlayer;
	return enemyPlayer;
};

/**
 * Target the player with the most advanced wonder.
 * TODO currently the first built wonder is kept, should chek on the minimum wonderDuration left instead.
 */
m.AttackManager.prototype.getWonderEnemyPlayer = function(gameState, attack)
{
	let enemyPlayer;
	let enemyWonder;
	let moreAdvanced;
	for (let wonder of gameState.getEnemyStructures().filter(API3.Filters.byClass("Wonder")).values())
	{
		if (wonder.owner() == 0)
			continue;
		let progress = wonder.foundationProgress();
		if (progress === undefined)
		{
			enemyWonder = wonder;
			break;
		}
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
	}
	return enemyPlayer;
};

/**
 * Target the player with the most relics (including gaia).
 */
m.AttackManager.prototype.getRelicEnemyPlayer = function(gameState, attack)
{
	let enemyPlayer;
	let allRelics = gameState.updatingGlobalCollection("allRelics", API3.Filters.byClass("Relic"));
	let maxRelicsOwned = 0;
	for (let i = 0; i < gameState.sharedScript.playersData.length; ++i)
	{
		if (!gameState.isPlayerEnemy(i) || this.defeated[i] ||
		    i == 0 && !gameState.ai.HQ.gameTypeManager.tryCaptureGaiaRelic)
			continue;

		let relicsCount = allRelics.filter(relic => relic.owner() == i).length;
		if (relicsCount <= maxRelicsOwned)
			continue;
		maxRelicsOwned = relicsCount;
		enemyPlayer = i;
	}
	if (enemyPlayer !== undefined)
	{
		if (attack.targetPlayer === undefined)
			this.currentEnemyPlayer = enemyPlayer;
		if (enemyPlayer == 0)
			gameState.ai.HQ.gameTypeManager.resetCaptureGaiaRelic(gameState);
	}
	return enemyPlayer;
};

/** f.e. if we have changed diplomacy with another player. */
m.AttackManager.prototype.cancelAttacksAgainstPlayer = function(gameState, player)
{
	for (let attackType in this.upcomingAttacks)
		for (let attack of this.upcomingAttacks[attackType])
			if (attack.targetPlayer === player)
				attack.targetPlayer = undefined;

	for (let attackType in this.startedAttacks)
		for (let i = 0; i < this.startedAttacks[attackType].length; ++i)
		{
			let attack = this.startedAttacks[attackType][i];
			if (attack.targetPlayer === player)
			{
				attack.Abort(gameState);
				this.startedAttacks[attackType].splice(i--, 1);
			}
		}
};

m.AttackManager.prototype.raidTargetEntity = function(gameState, ent)
{
	let data = { "target": ent };
	let attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, "Raid", data);
	if (attackPlan.failed)
		return null;
	if (this.Config.debug > 1)
		API3.warn("Military Manager: Raiding plan " + this.totalNumber);
	this.raidNumber++;
	this.totalNumber++;
	attackPlan.init(gameState);
	this.upcomingAttacks.Raid.push(attackPlan);
	return attackPlan;
};

/**
 * Return the number of units from any of our attacking armies around this position
 */
m.AttackManager.prototype.numAttackingUnitsAround = function(pos, dist)
{
	let num = 0;
	for (let attackType in this.startedAttacks)
		for (let attack of this.startedAttacks[attackType])
			if (API3.SquareVectorDistance(pos, attack.position) < dist*dist)
				num += attack.unitCollection.length;
	return num;
};

/**
 * Switch defense armies into an attack one against the given target
 * data.range: transform all defense armies inside range of the target into a new attack
 * data.armyID: transform only the defense army ID into a new attack
 * data.uniqueTarget: the attack will stop when the target is destroyed or captured
 */
m.AttackManager.prototype.switchDefenseToAttack = function(gameState, target, data)
{
	if (!target || !target.position())
		return false;
	if (!data.range && !data.armyID)
	{
		API3.warn(" attackManager.switchDefenseToAttack inconsistent data " + uneval(data));
		return false;
	}
	let attackData = data.uniqueTarget ? { "uniqueTargetId": target.id() } : undefined;
	let pos = target.position();
	let attackType = "Attack";
	let attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, attackType, attackData);
	if (attackPlan.failed)
		return false;
	this.totalNumber++;
	attackPlan.init(gameState);
	this.startedAttacks[attackType].push(attackPlan);

	for (let army of gameState.ai.HQ.defenseManager.armies)
	{
		if (data.range)
		{
			army.recalculatePosition(gameState);
			if (API3.SquareVectorDistance(pos, army.foePosition) > data.range * data.range)
				continue;
		}
		else if (army.ID != +data.armyID)
			continue;

		while (army.foeEntities.length > 0)
			army.removeFoe(gameState, army.foeEntities[0]);
		while (army.ownEntities.length > 0)
		{
			let unitId = army.ownEntities[0];
			army.removeOwn(gameState, unitId);
			let unit = gameState.getEntityById(unitId);
			if (unit && attackPlan.isAvailableUnit(gameState, unit))
			{
				unit.setMetadata(PlayerID, "plan", attackPlan.name);
				attackPlan.unitCollection.updateEnt(unit);
			}
		}
	}
	if (!attackPlan.unitCollection.hasEntities())
	{
		attackPlan.Abort(gameState);
		return false;
	}
	attackPlan.targetPlayer = target.owner();
	attackPlan.targetPos = pos;
	attackPlan.target = target;
	attackPlan.state = "arrived";
	return true;
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
		"currentEnemyPlayer": this.currentEnemyPlayer,
		"defeated": this.defeated
	};

	let upcomingAttacks = {};
	for (let key in this.upcomingAttacks)
	{
		upcomingAttacks[key] = [];
		for (let attack of this.upcomingAttacks[key])
			upcomingAttacks[key].push(attack.Serialize());
	}

	let startedAttacks = {};
	for (let key in this.startedAttacks)
	{
		startedAttacks[key] = [];
		for (let attack of this.startedAttacks[key])
			startedAttacks[key].push(attack.Serialize());
	}

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
			let attack = new m.AttackPlan(gameState, this.Config, dataAttack.properties.name);
			attack.Deserialize(gameState, dataAttack);
			attack.init(gameState);
			this.upcomingAttacks[key].push(attack);
		}
	}

	this.startedAttacks = {};
	for (let key in data.startedAttacks)
	{
		this.startedAttacks[key] = [];
		for (let dataAttack of data.startedAttacks[key])
		{
			let attack = new m.AttackPlan(gameState, this.Config, dataAttack.properties.name);
			attack.Deserialize(gameState, dataAttack);
			attack.init(gameState);
			this.startedAttacks[key].push(attack);
		}
	}
};

return m;
}(PETRA);
