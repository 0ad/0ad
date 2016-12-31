var PETRA = function(m)
{

/**
 * Handle events that are important to specific gameTypes
 * In regicide, train and manage healer guards for the hero
 * TODO: Handle when there is more than one hero in regicide
 * TODO: Assign military units to guard the hero in regicide
 */

m.GameTypeManager = function(Config)
{
	this.Config = Config;
	this.heroGarrisonEmergency = false;
	this.healersAssignedToHero = 0; // Accounts for healers being trained as well
	this.heroGuards = []; // Holds id of ents currently guarding the hero
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

	// check if new healers/guards need to be assigned to the hero
	for (let evt of events.Destroy)
	{
		if (!evt.entityObj || evt.entityObj.owner() !== PlayerID ||
		    this.heroGuards.indexOf(evt.entityObj.id()) === -1)
			continue;

		this.heroGuards.splice(this.heroGuards.indexOf(evt.entityObj.id()), 1);
		if (evt.entityObj.hasClass("Healer"))
			--this.healersAssignedToHero;
	}

	for (let evt of events.TrainingFinished)
		for (let entId of evt.entities)
		{
			let ent = gameState.getEntityById(entId);
			if (!ent || !ent.isOwn(PlayerID) || ent.getMetadata(PlayerID, "role") !== "regicideHealer")
				continue;

			this.assignGuardToRegicideHero(gameState, ent);
		}

	for (let evt of events.Garrison)
	{
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.isOwn(PlayerID) || !ent.hasClass("Hero"))
			continue;

		if (this.heroGarrisonEmergency)
			this.heroGarrisonEmergency = false;

		if (!gameState.getEntityById(evt.holder).hasClass("Ship"))
			continue;

		// If the hero is garrisoned on a ship, remove its guards
		for (let guardId of this.heroGuards)
			gameState.getEntityById(guardId).removeGuard();

		this.heroGuards = [];
	}

	for (let evt of events.UnGarrison)
	{
		let ent = gameState.getEntityById(evt.entity);
		if (!ent || !ent.isOwn(PlayerID))
			continue;

		// If this ent travelled to a hero's accessValue, try again to assign as a guard
		if (ent.getMetadata(PlayerID, "role") === "regicideHealer" &&
		    this.heroGuards.indexOf(evt.entity) === -1)
		{
			this.assignGuardToRegicideHero(gameState, ent);
			continue;
		}

		if (!ent.hasClass("Hero"))
			continue;

		// If this is the hero, try to assign ents that should be guarding it, but couldn't previously
		let regicideHealers = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "role", "regicideHealer"));
		for (let healer of regicideHealers.values())
			if (this.heroGuards.indexOf(healer.id()) === -1)
				this.assignGuardToRegicideHero(gameState, healer);
	}
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

m.GameTypeManager.prototype.trainRegicideHealer = function(gameState, queues)
{
	if (gameState.ai.HQ.saveResources || !gameState.getOwnEntitiesByClass("Temple", true).hasEntities())
		return;

	let template = gameState.applyCiv("units/{civ}_support_healer_b");

	queues.villager.addPlan(new m.TrainingPlan(gameState, template, { "role": "regicideHealer", "base": 0 }, 1, 1));
	++this.healersAssignedToHero;
};

/**
 * Only send the guard command if the guard's accessIndex is the same as the hero
 * and the hero has a position (i.e. not garrisoned)
 * request a transport if the accessIndex value is different
 */
m.GameTypeManager.prototype.assignGuardToRegicideHero = function(gameState, ent)
{
	let heroEnt = gameState.getOwnEntitiesByClass("Hero", true).toEntityArray()[0];
	if (!heroEnt || !heroEnt.position() ||
	    !ent.position() || ent.getMetadata(PlayerID, "transport") !== undefined || !ent.canGuard())
		return;

	let entAccess = gameState.ai.accessibility.getAccessValue(ent.position());
	let heroAccess = gameState.ai.accessibility.getAccessValue(heroEnt.position());
	if (entAccess === heroAccess)
	{
		ent.guard(heroEnt);
		this.heroGuards.push(ent.id());
	}
	else
		gameState.ai.HQ.navalManager.requireTransport(gameState, ent, entAccess, heroAccess, heroEnt.position());
};

m.GameTypeManager.prototype.update = function(gameState, events, queues)
{
	this.checkEvents(gameState, events);

	if (gameState.getGameType() === "wonder")
		this.buildWonder(gameState, queues);

	if (gameState.getGameType() !== "regicide")
		return;

	if (gameState.ai.playedTurn % 50 === 0)
	{
		let heroEnt = gameState.getOwnEntitiesByClass("Hero", true).toEntityArray()[0];
		if (heroEnt && heroEnt.healthLevel() > 0.7)
			heroEnt.setStance("aggressive");
	}

	if (this.healersAssignedToHero < 2 + Math.round(this.Config.personality.defensive * 2))
		this.trainRegicideHealer(gameState, queues);
};

m.GameTypeManager.prototype.Serialize = function()
{
	return {
		"heroGarrisonEmergency": this.heroGarrisonEmergency,
		"healersAssignedToHero": this.healersAssignedToHero,
		"heroGuards": this.heroGuards
	};
};

m.GameTypeManager.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
