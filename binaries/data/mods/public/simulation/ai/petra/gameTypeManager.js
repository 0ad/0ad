var PETRA = function(m)
{

/**
 * Handle events that are important to specific gameTypes
 * TODO: Handle when there is more than one hero in regicide
 * TODO: Assign military units to guard the hero in regicide
 */

m.GameTypeManager = function()
{
	this.heroGarrisonEmergency = false;
};

/**
 * In regicide mode, if the hero has less than 70% health, try to garrison it in a healing structure
 * If it is less than 40%, try to garrison in the closest possible structure
 * If the hero cannot garrison, retreat it to the closest base
 */
m.GameTypeManager.prototype.checkEvents = function(gameState, events)
{
	if (gameState.getGameType() === "wonder")
		for (let evt of events.Create)
		{
			let ent = gameState.getEntityById(evt.entity);
			if (!ent || !ent.isOwn(PlayerID) || ent.foundationProgress() === undefined ||
				!ent.hasClass("Wonder"))
				continue;

			// Let's get a few units from other bases to build the wonder.
			let base = gameState.ai.HQ.getBaseByID(ent.getMetadata(PlayerID, "base"));
			let builders = gameState.ai.HQ.bulkPickWorkers(gameState, base, 10);
			if (builders)
				for (let worker of builders.values())
				{
					worker.setMetadata(PlayerID, "base", base.ID);
					worker.setMetadata(PlayerID, "subrole", "builder");
					worker.setMetadata(PlayerID, "target-foundation", ent.id());
				}
		}

	if (gameState.getGameType() !== "regicide")
		return;

	for (let evt of events.Attacked)
	{
		let target = gameState.getEntityById(evt.target);
		if (!target || !gameState.isEntityOwn(target) || !target.position() ||
		    !target.hasClass("Hero") || target.healthLevel() > 0.7)
			continue;

		let plan = target.getMetadata(PlayerID, "plan");
		if (plan !== -2 && plan !== -3)
		{
			target.stopMoving();

			if (plan >= 0)
			{
				let attackPlan = gameState.ai.HQ.attackManager.getPlan(plan);
				if (attackPlan)
					attackPlan.removeUnit(target, true);
			}

			if (target.getMetadata(PlayerID, "PartOfArmy"))
			{
				let army = gameState.ai.HQ.defenseManager.getArmy(target.getMetadata(PlayerID, "PartOfArmy"));
				if (army)
					army.removeOwn(gameState, target.id());
			}

			this.heroGarrisonEmergency = target.healthLevel() < 0.4;
			this.pickHeroRetreatLocation(gameState, target, this.heroGarrisonEmergency);
		}
		else if (target.healthLevel() < 0.4 && !this.heroGarrisonEmergency)
		{
			// the hero is severely wounded, try to retreat/garrison quicker
			gameState.ai.HQ.garrisonManager.cancelGarrison(target);
			this.pickHeroRetreatLocation(gameState, target, true);
			this.heroGarrisonEmergency = true;
		}
	}

	if (!this.heroGarrisonEmergency)
		return;

	for (let evt of events.Garrison)
		if (gameState.getEntityById(evt.entity).hasClass("Hero"))
			this.heroGarrisonEmergency = false;
};

m.GameTypeManager.prototype.buildWonder = function(gameState, queues)
{
	if (queues.wonder && queues.wonder.hasQueuedUnits() ||
	    gameState.getOwnEntitiesByClass("Wonder", true).hasEntities() ||
	    !gameState.ai.HQ.canBuild(gameState, "structures/{civ}_wonder"))
		return;

	if (!queues.wonder)
		gameState.ai.queueManager.addQueue("wonder", 1000);
	queues.wonder.addPlan(new m.ConstructionPlan(gameState, "structures/{civ}_wonder"));
};

m.GameTypeManager.prototype.pickHeroRetreatLocation = function(gameState, heroEnt, emergency)
{
	gameState.ai.HQ.defenseManager.garrisonAttackedUnit(gameState, heroEnt, emergency);
	let plan = heroEnt.getMetadata(PlayerID, "plan");

	if (plan === -2 || plan === -3)
		return;

	// couldn't find a place to garrison, so the hero will flee from attacks
	heroEnt.setStance("passive");
	let accessIndex = gameState.ai.accessibility.getAccessValue(heroEnt.position());
	let basePos = m.getBestBase(gameState, heroEnt);
	if (basePos && basePos.accessIndex == accessIndex)
		heroEnt.move(basePos.anchor.position()[0], basePos.anchor.position()[1]);
};

m.GameTypeManager.prototype.update = function(gameState, events, queues)
{
	this.checkEvents(gameState, events);

	if (gameState.getGameType() === "wonder")
		this.buildWonder(gameState, queues);
	else if (gameState.getGameType() === "regicide" && gameState.ai.playedTurn % 50 === 0)
	{
		let heroEnt = gameState.getOwnEntitiesByClass("Hero", true).toEntityArray()[0];
		if (heroEnt && heroEnt.healthLevel() > 0.7)
			heroEnt.setStance("aggressive");
	}
};

m.GameTypeManager.prototype.Serialize = function()
{
	return { "heroGarrisonEmergency": this.heroGarrisonEmergency };
};

m.GameTypeManager.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
