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
	this.upcomingAttacks = { "CityAttack": [], "Rush": [], "Raid": [] };
	this.startedAttacks = { "CityAttack": [], "Rush": [], "Raid": [] };
	this.debugTime = 0;
};

// More initialisation for stuff that needs the gameState
m.AttackManager.prototype.init = function(gameState, queues)
{
	this.maxRushes = 0;
	if (this.Config.personality.aggressive > 0.9)
		this.maxRushes = 2
	else if (this.Config.personality.aggressive > 0.7)
		this.maxRushes = 1;
	else
		this.maxRushes = 0;
};

// Some functions are run every turn
// Others once in a while
m.AttackManager.prototype.update = function(gameState, queues, events)
{
	if (this.Config.debug == 2 &&  gameState.getTimeElapsed() > this.debugTime + 60000)
	{
		this.debugTime = gameState.getTimeElapsed();
		warn(" upcoming attacks =================");
		for (var attackType in this.upcomingAttacks)
		{
			for (var i = 0; i < this.upcomingAttacks[attackType].length; ++i)
			{
				var attack = this.upcomingAttacks[attackType][i];
				warn(" type " + attackType + " state " + attack.state + " paused " + attack.isPaused());
			}
		}
		warn(" started attacks ==================");
		for (var attackType in this.startedAttacks)
		{
			for (var i = 0; i < this.startedAttacks[attackType].length; ++i)
			{
				var attack = this.startedAttacks[attackType][i];
				warn(" type " + attackType + " state " + attack.state + " paused " + attack.isPaused());
			}
		}
		warn(" ==================================");
	}

	for (var attackType in this.upcomingAttacks)
	{
		for (var i = 0; i < this.upcomingAttacks[attackType].length; ++i)
		{
			var attack = this.upcomingAttacks[attackType][i];
			attack.checkEvents(gameState, events, queues);

			// okay so we'll get the support plan
			if (!attack.isStarted())
			{
				var updateStep = attack.updatePreparation(gameState, this,events);
					
				// now we're gonna check if the preparation time is over
				if (updateStep === 1 || attack.isPaused() )
				{
					// just chillin'
				}
				else if (updateStep === 0 || updateStep === 3)
				{
					if (this.Config.debug)
						warn("Attack Manager: " + attack.getType() + " plan " + attack.getName() + " aborted.");
					if (updateStep === 3)
						this.attackPlansEncounteredWater = true;
					attack.Abort(gameState, this);
					this.upcomingAttacks[attackType].splice(i--,1);
				}
				else if (updateStep === 2)
				{
					if (attack.StartAttack(gameState,this))
					{
						var targetName = gameState.sharedScript.playersData[attack.targetPlayer].name;
						var proba = Math.random();
						if (proba < 0.2)
							var chatText = "I am launching an attack against " + targetName + ".";
						else if (proba < 0.4)
							var chatText = "Attacking " + targetName + ".";
						else if (proba < 0.7)
							var chatText = "I have sent an army against " + targetName + ".";
						else
							var chatText = "I'm starting an attack against " + targetName + ".";
						gameState.ai.chatTeam(chatText);
		
						if (this.Config.debug)
							warn("Attack Manager: Starting " + attack.getType() + " plan " + attack.getName());
						this.startedAttacks[attackType].push(attack);
					}
					else
						attack.Abort(gameState, this);
					this.upcomingAttacks[attackType].splice(i--,1);
				}
			}
			else
			{
				var targetName = gameState.sharedScript.playersData[attack.targetPlayer].name;
				var proba = Math.random();
				if (proba < 0.2)
					var chatText = "I am launching an attack against " + targetName + ".";
				else if (proba < 0.4)
					var chatText = "Attacking " + targetName + ".";
				else if (proba < 0.7)
					var chatText = "I have sent an army against " + targetName + ".";
				else
					var chatText = "I'm starting an attack against " + targetName + ".";
				gameState.ai.chatTeam(chatText);
					
				if (this.Config.debug)
					warn("Attack Manager: Starting " + attack.getType() + " plan " + attack.getName());
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
			attack.checkEvents(gameState, events, queues);
			// okay so then we'll update the attack.
			if (attack.isPaused())
				continue;
			var remaining = attack.update(gameState,this,events);
			if (!remaining)
			{
				if (this.Config.debug)
					warn("Military Manager: " + attack.getType() + " plan " + attack.getName() + " is finished with remaining " + remaining);
				attack.Abort(gameState);
				this.startedAttacks[attackType].splice(i--,1);
			}
		}
	}
	
	// creating plans after updating because an aborted plan might be reused in that case.

	// TODO: remove the limitation to attacks when on water maps.
	if (!gameState.ai.HQ.waterMap && !this.attackPlansEncounteredWater)
	{
		if (this.rushNumber < this.maxRushes && gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_barracks"), true) >= 1)
		{
			if (this.upcomingAttacks["Rush"].length === 0)
			{ 
				// we have a barracks and we want to rush, rush.
				var attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, -1, "Rush");
				if (this.Config.debug)
					warn("Headquarters: Rushing plan " + this.totalNumber + " with maxRushes " + this.maxRushes);
				this.rushNumber++;
				this.totalNumber++;
				this.upcomingAttacks["Rush"].push(attackPlan);
			}
		}
		// if we have a barracks, there's no water, we're at age >= 1 and we've decided to attack.
		else if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_barracks"), true) >= 1
			&& (gameState.currentPhase() > 1 || gameState.isResearching(gameState.townPhase())))
		{
			if (gameState.countEntitiesByType(gameState.applyCiv("structures/{civ}_dock"), true) === 0 && gameState.ai.HQ.waterMap)
			{
				// wait till we get a dock.
			}
			else if (this.upcomingAttacks["CityAttack"].length === 0)
			{
				if (this.attackNumber < 2)
					var attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, -1);
				else
					var attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, -1, "superSized");

				if (attackPlan.failed)
					this.attackPlansEncounteredWater = true; // hack
				else
				{
					if (this.Config.debug)
						warn("Military Manager: Creating the plan " + this.totalNumber);
					this.attackNumber++;
					this.totalNumber++;
					this.upcomingAttacks["CityAttack"].push(attackPlan);
				}
			}
		}

		if (this.upcomingAttacks["Raid"].length === 0 && gameState.ai.HQ.defenseManager.targetList.length)
		{
			var target = undefined;
			for each (var targetId in gameState.ai.HQ.defenseManager.targetList)
			{
				target = gameState.getEntityById(targetId);
				if (target)
					break;
			}
			if (target)
			{
				// prepare a raid against this target
				var attackPlan = new m.AttackPlan(gameState, this.Config, this.totalNumber, target.owner(), "Raid");
				if (this.Config.debug > 0)
					warn("Headquarters: Raiding plan " + this.totalNumber);
				this.raidNumber++;
				this.totalNumber++;
				this.upcomingAttacks["Raid"].push(attackPlan);
			}
		}
	}
};

m.AttackManager.prototype.pausePlan = function(gameState, planName)
{
	for (var attackType in this.upcomingAttacks)
	{
		for (var i in this.upcomingAttacks[attackType])
		{
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(true);
		}
	}

	for (var attackType in this.startedAttacks)
	{
		for (var i in this.startedAttacks[attackType])
		{
			var attack = this.startedAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(true);
		}
	}
};

m.AttackManager.prototype.unpausePlan = function(gameState, planName)
{
	for (var attackType in this.upcomingAttacks)
	{
		for (var i in this.upcomingAttacks[attackType])
		{
			var attack = this.upcomingAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(false);
		}
	}

	for (var attackType in this.startedAttacks)
	{
		for (var i in this.startedAttacks[attackType])
		{
			var attack = this.startedAttacks[attackType][i];
			if (attack.getName() == planName)
				attack.setPaused(false);
		}
	}
};

m.AttackManager.prototype.pauseAllPlans = function(gameState)
{
	for (var attackType in this.upcomingAttacks)
		for (var i in this.upcomingAttacks[attackType])
			this.upcomingAttacks[attackType][i].setPaused(true);

	for (var attackType in this.startedAttacks)
		for (var i in this.startedAttacks[attackType])
			this.startedAttacks[attackType][i].setPaused(true);
};

m.AttackManager.prototype.unpauseAllPlans = function(gameState)
{
	for (var attackType in this.upcomingAttacks)
		for (var i in this.upcomingAttacks[attackType])
			this.upcomingAttacks[attackType][i].setPaused(false);

	for (var attackType in this.startedAttacks)
		for (var i in this.startedAttacks[attackType])
			this.startedAttacks[attackType][i].setPaused(false);
};

return m;
}(PETRA);
