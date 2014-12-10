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

// Some functions are run every turn
// Others once in a while
m.AttackManager.prototype.update = function(gameState, queues, events)
{
	if (this.Config.debug > 2 && gameState.ai.elapsedTime > this.debugTime + 60)
	{
		this.debugTime = gameState.ai.elapsedTime;
		API3.warn(" upcoming attacks =================");
		for (var attackType in this.upcomingAttacks)
		{
			for (var i = 0; i < this.upcomingAttacks[attackType].length; ++i)
			{
				var attack = this.upcomingAttacks[attackType][i];
				API3.warn(" type " + attackType + " state " + attack.state);
			}
		}
		API3.warn(" started attacks ==================");
		for (var attackType in this.startedAttacks)
		{
			for (var i = 0; i < this.startedAttacks[attackType].length; ++i)
			{
				var attack = this.startedAttacks[attackType][i];
				API3.warn(" type " + attackType + " state " + attack.state);
			}
		}
		API3.warn(" ==================================");
	}

	for (var attackType in this.upcomingAttacks)
	{
		for (var i = 0; i < this.upcomingAttacks[attackType].length; ++i)
		{
			var attack = this.upcomingAttacks[attackType][i];
			attack.checkEvents(gameState, events);

			// okay so we'll get the support plan
			if (!attack.isStarted())
			{
				var updateStep = attack.updatePreparation(gameState, events);
					
				// now we're gonna check if the preparation time is over
				if (updateStep === 1 || attack.isPaused() )
				{
					// just chillin'
				}
				else if (updateStep === 0 || updateStep === 3)
				{
					if (this.Config.debug > 1)
						API3.warn("Attack Manager: " + attack.getType() + " plan " + attack.getName() + " aborted.");
					attack.Abort(gameState, this);
					this.upcomingAttacks[attackType].splice(i--,1);
				}
				else if (updateStep === 2)
				{
					if (attack.StartAttack(gameState,this))
					{
						if (this.Config.debug > 1)
							API3.warn("Attack Manager: Starting " + attack.getType() + " plan " + attack.getName());
						if (this.Config.chat)
							m.chatLaunchAttack(gameState, attack.targetPlayer);
						this.startedAttacks[attackType].push(attack);
					}
					else
						attack.Abort(gameState, this);
					this.upcomingAttacks[attackType].splice(i--,1);
				}
			}
			else
			{
				if (this.Config.debug > 1)
					API3.warn("Attack Manager: Starting " + attack.getType() + " plan " + attack.getName());
				if (this.Config.chat)
					m.chatLaunchAttack(gameState, attack.targetPlayer);
				this.startedAttacks[attackType].push(attack);
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

	if (this.rushNumber < this.maxRushes && gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_barracks"), true) >= 1)
	{
		if (this.upcomingAttacks["Rush"].length === 0)
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
	else if (this.upcomingAttacks["Attack"].length === 0 && this.upcomingAttacks["HugeAttack"].length === 0
		&& (this.startedAttacks["Attack"].length + this.startedAttacks["HugeAttack"].length < Math.round(gameState.getPopulationMax()/100)))
	{
		if ((gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_barracks"), true) >= 1
				&& (gameState.currentPhase() > 1 || gameState.isResearching(gameState.townPhase())))
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

	if (this.upcomingAttacks["Raid"].length === 0 && gameState.ai.HQ.defenseManager.targetList.length)
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

m.AttackManager.prototype.Serialize = function()
{
	let properties = {
		"totalNumber": this.totalNumber,
		"attackNumber": this.attackNumber,
		"rushNumber": this.rushNumber,
		"raidNumber": this.raidNumber,
		"debugTime": this.debugTime,
		"maxRushes": this.maxRushes,
		"rushSize": this.rushSize
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
