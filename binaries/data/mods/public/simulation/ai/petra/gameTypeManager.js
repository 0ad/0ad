var PETRA = function(m)
{

/**
 * Handle events that are important to specific gameTypes
 * In regicide, train and manage healer guards for the hero
 * TODO: Assign military units to guard the hero in regicide
 */

m.GameTypeManager = function(Config)
{
	this.Config = Config;
	this.criticalEnts = new Map();
	// Holds ids of all ents who are (or can be) guarding and if the ent is currently guarding
	this.guardEnts = new Map();
	this.healersPerCriticalEnt = 2 + Math.round(this.Config.personality.defensive * 2);
};

/**
 * Cache the ids of any inital gameType-critical entities.
 * In regicide, these are the inital heroes that the player starts with.
 */
m.GameTypeManager.prototype.init = function(gameState)
{
	if (gameState.getGameType() !== "regicide")
		return;

	let heroEnts = gameState.getOwnEntitiesByClass("Hero", true).toEntityArray();
	for (let hero of heroEnts)
	{
		let heroStance = hero.hasClass("Soldier") ? "aggressive" : "passive";
		hero.setStance(heroStance);
		this.criticalEnts.set(hero.id(), {
			"garrisonEmergency": false,
			"stance": heroStance,
			"healersAssigned": 0,
			"guards": new Map() // ids of ents who are currently guarding this hero
		});
	}
};

/**
 * In regicide mode, if the hero has less than 70% health, try to garrison it in a healing structure
 * If it is less than 40%, try to garrison in the closest possible structure
 * If the hero cannot garrison, retreat it to the closest base
 */
m.GameTypeManager.prototype.checkEvents = function(gameState, events)
{
	if (gameState.getGameType() === "wonder")
	{
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

		for (let evt of events.ConstructionFinished)
		{
			if (!evt || !evt.newentity)
				continue;

			let ent = gameState.getEntityById(evt.newentity);
			if (ent && ent.isOwn(PlayerID) && ent.hasClass("Wonder"))
				this.criticalEnts.set(ent.id(), { "guardsAssigned": 0, "guards": new Map() });
		}
	}

	if (gameState.getGameType() === "regicide")
	{
		for (let evt of events.Attacked)
		{
			if (!this.criticalEnts.has(evt.target))
				continue;

			let target = gameState.getEntityById(evt.target);
			if (!target || !target.position() || target.healthLevel() > 0.7)
				continue;

			let plan = target.getMetadata(PlayerID, "plan");
			let hero = this.criticalEnts.get(evt.target);
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

				hero.garrisonEmergency = target.healthLevel() < 0.4;
				this.pickCriticalEntRetreatLocation(gameState, target, hero.garrisonEmergency);
			}
			else if (target.healthLevel() < 0.4 && !hero.garrisonEmergency)
			{
				// the hero is severely wounded, try to retreat/garrison quicker
				gameState.ai.HQ.garrisonManager.cancelGarrison(target);
				this.pickCriticalEntRetreatLocation(gameState, target, true);
				hero.garrisonEmergency = true;
			}
		}

		for (let evt of events.TrainingFinished)
			for (let entId of evt.entities)
			{
				let ent = gameState.getEntityById(entId);
				if (ent && ent.isOwn(PlayerID) && ent.getMetadata(PlayerID, "role") === "criticalEntHealer")
					this.assignGuardToCriticalEnt(gameState, ent);
			}

		for (let evt of events.Garrison)
		{
			if (!this.criticalEnts.has(evt.entity))
				continue;

			let hero = this.criticalEnts.get(evt.entity);
			if (hero.garrisonEmergency)
				hero.garrisonEmergency = false;

			let holderEnt = gameState.getEntityById(evt.holder);
			if (!holderEnt || !holderEnt.hasClass("Ship"))
				continue;

			// If the hero is garrisoned on a ship, remove its guards
			for (let guardId of hero.guards)
			{
				let guardEnt = gameState.getEntityById(guardId);
				if (!guardEnt)
					continue;

				guardEnt.removeGuard();
				this.guardEnts.set(guardId, false);
			}
			hero.guards.clear();
		}
	}

	// Check if new healers/guards need to be assigned to an ent
	for (let evt of events.Destroy)
	{
		if (!evt.entityObj || evt.entityObj.owner() !== PlayerID)
			continue;

		let entId = evt.entityObj.id();
		if (this.criticalEnts.has(entId))
		{
			for (let [guardId, role] of this.criticalEnts.get(entId).guards)
			{
				let guardEnt = gameState.getEntityById(guardId);
				if (!guardEnt)
					continue;

				if (role === "healer")
					this.guardEnts.set(guardId, false);
				else
				{
					guardEnt.setMetadata(PlayerID, "plan", -1);
					guardEnt.setMetadata(PlayerID, "role", undefined);
					this.guardEnts.delete(guardId);
				}
			}

			this.criticalEnts.delete(entId);
			continue;
		}

		if (!this.guardEnts.has(entId))
			continue;

		for (let data of this.criticalEnts.values())
			if (data.guards.has(entId))
			{
				data.guards.delete(entId);
				if (evt.entityObj.hasClass("Healer"))
					--data.healersAssigned;
				else
					--data.guardsAssigned;
			}

		this.guardEnts.delete(entId);
	}

	for (let evt of events.UnGarrison)
	{
		if (!this.guardEnts.has(evt.entity) && !this.criticalEnts.has(evt.entity))
			continue;

		let ent = gameState.getEntityById(evt.entity);
		if (!ent)
			continue;

		// If this ent travelled to a criticalEnt's accessValue, try again to assign as a guard
		if ((ent.getMetadata(PlayerID, "role") === "criticalEntHealer" ||
		     ent.getMetadata(PlayerID, "role") === "criticalEntGuard") && !this.guardEnts.get(evt.entity))
		{
			this.assignGuardToCriticalEnt(gameState, ent);
			continue;
		}

		if (!this.criticalEnts.has(evt.entity))
			continue;

		// If this is a hero, try to assign ents that should be guarding it, but couldn't previously
		let criticalEnt = this.criticalEnts.get(evt.entity);
		for (let [id, isGuarding] of this.guardEnts)
		{
			if (criticalEnt.guards.size >= this.healersPerCriticalEnt)
				break;

			if (!isGuarding)
			{
				let guardEnt = gameState.getEntityById(id);
				if (guardEnt)
					this.assignGuardToCriticalEnt(gameState, guardEnt, evt.entity);
			}
		}
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

/**
 * Try to keep some military units guarding any criticalEnts.
 * TODO: Prefer champion units, and swap citizen soldier guards with champions if they become available.
 */
m.GameTypeManager.prototype.manageCriticalEntGuards = function(gameState)
{
	for (let [id, data] of this.criticalEnts)
	{
		let criticalEnt = gameState.getEntityById(id);
		if (!criticalEnt)
			continue;

		let militaryGuardsPerCriticalEnt = (criticalEnt.hasClass("Wonder") ? 10 : 4) +
			Math.round(this.Config.personality.defensive * 5);

		if (data.guardsAssigned >= militaryGuardsPerCriticalEnt)
			continue;

		// First try to pick guards in the criticalEnt's accessIndex, to avoid unnecessary transports
		for (let checkForSameAccess of [true, false])
		{
			for (let entity of gameState.ai.HQ.attackManager.outOfPlan.values())
			{
				if (entity.getMetadata(PlayerID, "transport") !== undefined ||
				    checkForSameAccess && (!entity.position() || !criticalEnt.position() ||
				    m.getLandAccess(gameState, criticalEnt) !== m.getLandAccess(gameState, entity)))
					continue;

				this.assignGuardToCriticalEnt(gameState, entity, id);
				entity.setMetadata(PlayerID, "plan", -2);
				entity.setMetadata(PlayerID, "role", "criticalEntGuard");
				if (++data.guardsAssigned >= militaryGuardsPerCriticalEnt)
					break;
			}

			if (data.guardsAssigned >= militaryGuardsPerCriticalEnt)
				break;

			for (let entity of gameState.getOwnEntitiesByClass("Soldier", true).values())
			{
				if (entity.getMetadata(PlayerID, "plan") !== undefined ||
					entity.getMetadata(PlayerID, "transport") !== undefined ||
				    checkForSameAccess && (!entity.position() || !criticalEnt.position() ||
				    m.getLandAccess(gameState, criticalEnt) !== m.getLandAccess(gameState, entity)))
					continue;

				this.assignGuardToCriticalEnt(gameState, entity, id);
				entity.setMetadata(PlayerID, "plan", -2);
				entity.setMetadata(PlayerID, "role", "criticalEntGuard");
				if (++data.guardsAssigned >= militaryGuardsPerCriticalEnt)
					break;
			}

			if (data.guardsAssigned >= militaryGuardsPerCriticalEnt)
				break;
		}
	}
};

m.GameTypeManager.prototype.pickCriticalEntRetreatLocation = function(gameState, criticalEnt, emergency)
{
	gameState.ai.HQ.defenseManager.garrisonAttackedUnit(gameState, criticalEnt, emergency);
	let plan = criticalEnt.getMetadata(PlayerID, "plan");

	if (plan === -2 || plan === -3)
		return;

	// Couldn't find a place to garrison, so the ent will flee from attacks
	criticalEnt.setStance("passive");
	let accessIndex = gameState.ai.accessibility.getAccessValue(criticalEnt.position());
	let basePos = m.getBestBase(gameState, criticalEnt);
	if (basePos && basePos.accessIndex == accessIndex)
		criticalEnt.move(basePos.anchor.position()[0], basePos.anchor.position()[1]);
};

/**
 * The number of healers trained per critical ent (dependent on the defensive trait)
 * may not be the number of healers actually guarding an ent at any one time.
 */
m.GameTypeManager.prototype.trainCriticalEntHealer = function(gameState, queues, id)
{
	if (gameState.ai.HQ.saveResources || !gameState.getOwnEntitiesByClass("Temple", true).hasEntities())
		return;

	let template = gameState.applyCiv("units/{civ}_support_healer_b");

	queues.villager.addPlan(new m.TrainingPlan(gameState, template, { "role": "criticalEntHealer", "base": 0 }, 1, 1));
	++this.criticalEnts.get(id).healersAssigned;
};

/**
 * Only send the guard command if the guard's accessIndex is the same as the critical ent
 * and the critical ent has a position (i.e. not garrisoned).
 * Request a transport if the accessIndex value is different
 */
m.GameTypeManager.prototype.assignGuardToCriticalEnt = function(gameState, guardEnt, criticalEntId)
{
	if (guardEnt.getMetadata(PlayerID, "transport") !== undefined || !guardEnt.canGuard())
		return;

	if (!criticalEntId)
	{
		// Assign to the critical ent with the fewest guards
		let min = Math.min();
		for (let [id, data] of this.criticalEnts)
		{
			if (data.guards.size > min)
				continue;

			criticalEntId = id;
			min = data.guards.size;
		}
	}

	if (!criticalEntId)
		return;

	let criticalEnt = gameState.getEntityById(criticalEntId);
	if (!criticalEnt || !criticalEnt.position() || !guardEnt.position())
	{
		this.guardEnts.set(guardEnt.id(), false);
		return;
	}

	let guardEntAccess = gameState.ai.accessibility.getAccessValue(guardEnt.position());
	let criticalEntAccess = gameState.ai.accessibility.getAccessValue(criticalEnt.position());
	if (guardEntAccess === criticalEntAccess)
	{
		let queued = m.returnResources(gameState, guardEnt);
		guardEnt.guard(criticalEnt, queued);
		let guardRole = guardEnt.getMetadata(PlayerID, "role") === "criticalEntHealer" ? "healer" : "guard";
		this.criticalEnts.get(criticalEntId).guards.set(guardEnt.id(), guardRole);
	}
	else
		gameState.ai.HQ.navalManager.requireTransport(gameState, guardEnt, guardEntAccess, criticalEntAccess, criticalEnt.position());
	this.guardEnts.set(guardEnt.id(), guardEntAccess === criticalEntAccess);
};

m.GameTypeManager.prototype.update = function(gameState, events, queues)
{
	// Wait a turn for trigger scripts to spawn any critical ents (i.e. in regicide)
	if (gameState.ai.playedTurn === 1)
		this.init(gameState);

	this.checkEvents(gameState, events);

	if (gameState.getGameType() === "wonder")
	{
		this.buildWonder(gameState, queues);
		this.manageCriticalEntGuards(gameState);
	}

	if (gameState.getGameType() !== "regicide")
		return;

	if (gameState.ai.playedTurn % 50 === 0)
		for (let [id, data] of this.criticalEnts)
		{
			let ent = gameState.getEntityById(id);
			if (ent && ent.healthLevel() > 0.7 && ent.hasClass("Soldier") &&
			    data.stance !== "aggressive")
				ent.setStance("aggressive");
		}

	for (let [id, data] of this.criticalEnts)
		if (data.healersAssigned < this.healersPerCriticalEnt &&
		    this.guardEnts.size < gameState.getPopulationMax() / 10)
			this.trainCriticalEntHealer(gameState, queues, id);
};

m.GameTypeManager.prototype.Serialize = function()
{
	return {
		"criticalEnts": this.criticalEnts,
		"guardEnts": this.guardEnts,
		"healersPerCriticalEnt": this.healersPerCriticalEnt
	};
};

m.GameTypeManager.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
