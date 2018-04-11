var PETRA = function(m)
{

/**
 * Handle events that are important to specific victory conditions:
 *   in capture_the_relic, capture gaia relics and train military guards.
 *   in regicide, train healer and military guards for the hero.
 *   in wonder, train military guards.
 */

m.GameTypeManager = function(Config)
{
	this.Config = Config;
	this.criticalEnts = new Map();
	// Holds ids of all ents who are (or can be) guarding and if the ent is currently guarding
	this.guardEnts = new Map();
	this.healersPerCriticalEnt = 2 + Math.round(this.Config.personality.defensive * 2);
	this.tryCaptureGaiaRelic = false;
	this.tryCaptureGaiaRelicLapseTime = -1;
	// Gaia relics which we are targeting currently and have not captured yet
	this.targetedGaiaRelics = new Map();
};

/**
 * Cache the ids of any inital gameType-critical entities.
 */
m.GameTypeManager.prototype.init = function(gameState)
{
	if (gameState.getVictoryConditions().has("wonder"))
	{
		for (let wonder of gameState.getOwnEntitiesByClass("Wonder", true).values())
			this.criticalEnts.set(wonder.id(), { "guardsAssigned": 0, "guards": new Map() });
	}

	if (gameState.getVictoryConditions().has("regicide"))
	{
		for (let hero of gameState.getOwnEntitiesByClass("Hero", true).values())
		{
			let defaultStance = hero.hasClass("Soldier") ? "aggressive" : "passive";
			if (hero.getStance() != defaultStance)
				hero.setStance(defaultStance);
			this.criticalEnts.set(hero.id(), {
				"garrisonEmergency": false,
				"healersAssigned": 0,
				"guardsAssigned": 0, // for non-healer guards
				"guards": new Map() // ids of ents who are currently guarding this hero
			});
		}
	}

	if (gameState.getVictoryConditions().has("capture_the_relic"))
	{
		for (let relic of gameState.updatingGlobalCollection("allRelics", API3.Filters.byClass("Relic")).values())
		{
			if (relic.owner() == PlayerID)
				this.criticalEnts.set(relic.id(), { "guardsAssigned": 0, "guards": new Map() });
		}
	}
};

/**
 * In regicide mode, if the hero has less than 70% health, try to garrison it in a healing structure
 * If it is less than 40%, try to garrison in the closest possible structure
 * If the hero cannot garrison, retreat it to the closest base
 */
m.GameTypeManager.prototype.checkEvents = function(gameState, events)
{
	if (gameState.getVictoryConditions().has("wonder"))
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

	if (gameState.getVictoryConditions().has("regicide"))
	{
		for (let evt of events.Attacked)
		{
			if (!this.criticalEnts.has(evt.target))
				continue;

			let target = gameState.getEntityById(evt.target);
			if (!target || !target.position() || target.healthLevel() > this.Config.garrisonHealthLevel.high)
				continue;

			let plan = target.getMetadata(PlayerID, "plan");
			let hero = this.criticalEnts.get(evt.target);
			if (plan != -2 && plan != -3)
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

				hero.garrisonEmergency = target.healthLevel() < this.Config.garrisonHealthLevel.low;
				this.pickCriticalEntRetreatLocation(gameState, target, hero.garrisonEmergency);
			}
			else if (target.healthLevel() < this.Config.garrisonHealthLevel.low && !hero.garrisonEmergency)
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
				if (ent && ent.isOwn(PlayerID) && ent.getMetadata(PlayerID, "role") == "criticalEntHealer")
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
			if (!holderEnt)
				continue;

			if (holderEnt.hasClass("Ship"))
			{
				// If the hero is garrisoned on a ship, remove its guards
				for (let guardId of hero.guards.keys())
				{
					let guardEnt = gameState.getEntityById(guardId);
					if (!guardEnt)
						continue;

					guardEnt.removeGuard();
					this.guardEnts.set(guardId, false);
				}
				hero.guards.clear();
				continue;
			}

			// Move the current guards to the garrison location.
			// TODO: try to garrison them with the critical ent.
			for (let guardId of hero.guards.keys())
			{
				let guardEnt = gameState.getEntityById(guardId);
				if (!guardEnt)
					continue;

				let plan = guardEnt.getMetadata(PlayerID, "plan");

				// Current military guards (with Soldier class) will have been assigned plan metadata, but healer guards
				// are not assigned a plan, and so they could be already moving to garrison somewhere due to low health.
				if (!guardEnt.hasClass("Soldier") && (plan == -2 || plan == -3))
					continue;

				let pos = holderEnt.position();
				let radius = holderEnt.obstructionRadius().max;
				if (pos)
					guardEnt.moveToRange(pos[0], pos[1], radius, radius + 5);
			}
		}
	}

	for (let evt of events.EntityRenamed)
	{
		if (!this.guardEnts.has(evt.entity))
			continue;
		for (let data of this.criticalEnts.values())
		{
			if (!data.guards.has(evt.entity))
				continue;
			data.guards.set(evt.newentity, data.guards.get(evt.entity));
			data.guards.delete(evt.entity);
			break;
		}
		this.guardEnts.set(evt.newentity, this.guardEnts.get(evt.entity));
		this.guardEnts.delete(evt.entity);
	}

	// Check if new healers/guards need to be assigned to an ent
	for (let evt of events.Destroy)
	{
		if (!evt.entityObj || evt.entityObj.owner() != PlayerID)
			continue;

		let entId = evt.entityObj.id();
		if (this.criticalEnts.has(entId))
		{
			this.removeCriticalEnt(gameState, entId);
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
				break;
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
		if ((ent.getMetadata(PlayerID, "role") == "criticalEntHealer" ||
		     ent.getMetadata(PlayerID, "role") == "criticalEntGuard") && !this.guardEnts.get(evt.entity))
		{
			this.assignGuardToCriticalEnt(gameState, ent, ent.getMetadata(PlayerID, "guardedEnt"));
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

	for (let evt of events.OwnershipChanged)
	{
		if (evt.from == PlayerID && this.criticalEnts.has(evt.entity))
		{
			this.removeCriticalEnt(gameState, evt.entity);
			continue;
		}
		if (evt.from == 0 && this.targetedGaiaRelics.has(evt.entity))
			this.abortCaptureGaiaRelic(gameState, evt.entity);

		if (evt.to != PlayerID)
			continue;

		let ent = gameState.getEntityById(evt.entity);
		if (ent && (gameState.getVictoryConditions().has("wonder") && ent.hasClass("Wonder") ||
		            gameState.getVictoryConditions().has("capture_the_relic") && ent.hasClass("Relic")))
		{
			this.criticalEnts.set(ent.id(), { "guardsAssigned": 0, "guards": new Map() });
			// Move captured relics to the closest base
			if (ent.hasClass("Relic"))
				this.pickCriticalEntRetreatLocation(gameState, ent, false);
		}
	}
};

m.GameTypeManager.prototype.removeCriticalEnt = function(gameState, criticalEntId)
{
	for (let [guardId, role] of this.criticalEnts.get(criticalEntId).guards)
	{
		let guardEnt = gameState.getEntityById(guardId);
		if (!guardEnt)
			continue;

		if (role == "healer")
			this.guardEnts.set(guardId, false);
		else
		{
			guardEnt.setMetadata(PlayerID, "plan", -1);
			guardEnt.setMetadata(PlayerID, "role", undefined);
			this.guardEnts.delete(guardId);
		}

		if (guardEnt.getMetadata(PlayerID, "guardedEnt"))
			guardEnt.setMetadata(PlayerID, "guardedEnt", undefined);
	}
	this.criticalEnts.delete(criticalEntId);
};

/**
 * Train more healers to be later affected to critical entities if needed
 */
m.GameTypeManager.prototype.manageCriticalEntHealers = function(gameState, queues)
{
	if (gameState.ai.HQ.saveResources || queues.healer.hasQueuedUnits() ||
	    !gameState.getOwnEntitiesByClass("Temple", true).hasEntities() ||
	    this.guardEnts.size > Math.min(gameState.getPopulationMax() / 10, gameState.getPopulation() / 4))
		return;

	for (let data of this.criticalEnts.values())
	{
		if (data.healersAssigned === undefined || data.healersAssigned >= this.healersPerCriticalEnt)
			continue;
		let template = gameState.applyCiv("units/{civ}_support_healer_b");
		queues.healer.addPlan(new m.TrainingPlan(gameState, template, { "role": "criticalEntHealer", "base": 0 }, 1, 1));
		return;
	}
};

/**
 * Try to keep some military units guarding any criticalEnts, if we can afford it.
 * If we have too low a population and require units for other needs, remove guards so they can be reassigned.
 * TODO: Swap citizen soldier guards with champions if they become available.
 */
m.GameTypeManager.prototype.manageCriticalEntGuards = function(gameState)
{
	let numWorkers = gameState.getOwnEntitiesByRole("worker", true).length;
	if (numWorkers < 20)
	{
		for (let data of this.criticalEnts.values())
		{
			for (let guardId of data.guards.keys())
			{
				let guardEnt = gameState.getEntityById(guardId);
				if (!guardEnt || !guardEnt.hasClass("CitizenSoldier") ||
				    guardEnt.getMetadata(PlayerID, "role") != "criticalEntGuard")
					continue;

				guardEnt.removeGuard();
				guardEnt.setMetadata(PlayerID, "plan", -1);
				guardEnt.setMetadata(PlayerID, "role", undefined);
				this.guardEnts.delete(guardId);
				--data.guardsAssigned;

				if (guardEnt.getMetadata(PlayerID, "guardedEnt"))
					guardEnt.setMetadata(PlayerID, "guardedEnt", undefined);

				if (++numWorkers >= 20)
					break;
			}
			if (numWorkers >= 20)
				break;
		}
	}

	let minWorkers = 25;
	let deltaWorkers = 3;
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
			// First try to assign any Champion units we might have
			for (let entity of gameState.getOwnEntitiesByClass("Champion", true).values())
			{
				if (!this.tryAssignMilitaryGuard(gameState, entity, criticalEnt, checkForSameAccess))
					continue;
				if (++data.guardsAssigned >= militaryGuardsPerCriticalEnt)
					break;
			}

			if (data.guardsAssigned >= militaryGuardsPerCriticalEnt || numWorkers <= minWorkers + deltaWorkers * data.guardsAssigned)
				break;

			for (let entity of gameState.ai.HQ.attackManager.outOfPlan.values())
			{
				if (!this.tryAssignMilitaryGuard(gameState, entity, criticalEnt, checkForSameAccess))
					continue;
				--numWorkers;
				if (++data.guardsAssigned >= militaryGuardsPerCriticalEnt || numWorkers <= minWorkers + deltaWorkers * data.guardsAssigned)
					break;
			}

			if (data.guardsAssigned >= militaryGuardsPerCriticalEnt || numWorkers <= minWorkers + deltaWorkers * data.guardsAssigned)
				break;

			for (let entity of gameState.getOwnEntitiesByClass("Soldier", true).values())
			{
				if (!this.tryAssignMilitaryGuard(gameState, entity, criticalEnt, checkForSameAccess))
					continue;
				--numWorkers;
				if (++data.guardsAssigned >= militaryGuardsPerCriticalEnt || numWorkers <= minWorkers + deltaWorkers * data.guardsAssigned)
					break;
			}

			if (data.guardsAssigned >= militaryGuardsPerCriticalEnt || numWorkers <= minWorkers + deltaWorkers * data.guardsAssigned)
				break;
		}
	}
};

m.GameTypeManager.prototype.tryAssignMilitaryGuard = function(gameState, guardEnt, criticalEnt, checkForSameAccess)
{
	if (guardEnt.getMetadata(PlayerID, "plan") !== undefined ||
	    guardEnt.getMetadata(PlayerID, "transport") !== undefined || this.criticalEnts.has(guardEnt.id()) ||
	    checkForSameAccess && (!guardEnt.position() || !criticalEnt.position() ||
	    m.getLandAccess(gameState, criticalEnt) != m.getLandAccess(gameState, guardEnt)))
		return false;

	if (!this.assignGuardToCriticalEnt(gameState, guardEnt, criticalEnt.id()))
		return false;

	guardEnt.setMetadata(PlayerID, "plan", -2);
	guardEnt.setMetadata(PlayerID, "role", "criticalEntGuard");
	return true;
};

m.GameTypeManager.prototype.pickCriticalEntRetreatLocation = function(gameState, criticalEnt, emergency)
{
	gameState.ai.HQ.defenseManager.garrisonAttackedUnit(gameState, criticalEnt, emergency);
	let plan = criticalEnt.getMetadata(PlayerID, "plan");

	if (plan == -2 || plan == -3)
		return;

	if (this.criticalEnts.get(criticalEnt.id()).garrisonEmergency)
		this.criticalEnts.get(criticalEnt.id()).garrisonEmergency = false;

	// Couldn't find a place to garrison, so the ent will flee from attacks
	if (!criticalEnt.hasClass("Relic") && criticalEnt.getStance() != "passive")
		criticalEnt.setStance("passive");
	let accessIndex = m.getLandAccess(gameState, criticalEnt);
	let bestBase = m.getBestBase(gameState, criticalEnt, true);
	if (bestBase.accessIndex == accessIndex)
	{
		let bestBasePos = bestBase.anchor.position();
		criticalEnt.move(bestBasePos[0], bestBasePos[1]);
	}
};

/**
 * Only send the guard command if the guard's accessIndex is the same as the critical ent
 * and the critical ent has a position (i.e. not garrisoned).
 * Request a transport if the accessIndex value is different, and if a transport is needed,
 * the guardEnt will be given metadata describing which entity it is being sent to guard,
 * which will be used once its transport has finished.
 * Return false if the guardEnt is not a valid guard unit (i.e. cannot guard or is being transported).
 */
m.GameTypeManager.prototype.assignGuardToCriticalEnt = function(gameState, guardEnt, criticalEntId)
{
	if (guardEnt.getMetadata(PlayerID, "transport") !== undefined || !guardEnt.canGuard())
		return false;

	if (criticalEntId && !this.criticalEnts.get(criticalEntId))
	{
		criticalEntId = undefined;
		if (guardEnt.getMetadata(PlayerID, "guardedEnt"))
			guardEnt.setMetadata(PlayerID, "guardedEnt", undefined);
	}

	if (!criticalEntId)
	{
		let isHealer = guardEnt.hasClass("Healer");

		// Assign to the critical ent with the fewest guards
		let min = Math.min();
		for (let [id, data] of this.criticalEnts)
		{
			if (isHealer && (data.healersAssigned === undefined || data.healersAssigned > min))
				continue;
			if (!isHealer && data.guardsAssigned > min)
				continue;

			criticalEntId = id;
			min = isHealer ? data.healersAssigned : data.guardsAssigned;
		}
		if (criticalEntId)
		{
			let data = this.criticalEnts.get(criticalEntId);
			if (isHealer)
				++data.healersAssigned;
			else
				++data.guardsAssigned;
		}
	}

	if (!criticalEntId)
	{
		if (guardEnt.getMetadata(PlayerID, "guardedEnt"))
			guardEnt.setMetadata(PlayerID, "guardedEnt", undefined);
		return false;
	}

	let criticalEnt = gameState.getEntityById(criticalEntId);
	if (!criticalEnt || !criticalEnt.position() || !guardEnt.position())
	{
		this.guardEnts.set(guardEnt.id(), false);
		return false;
	}

	if (guardEnt.getMetadata(PlayerID, "guardedEnt") != criticalEntId)
		guardEnt.setMetadata(PlayerID, "guardedEnt", criticalEntId);

	let guardEntAccess = m.getLandAccess(gameState, guardEnt);
	let criticalEntAccess = m.getLandAccess(gameState, criticalEnt);
	if (guardEntAccess == criticalEntAccess)
	{
		let queued = m.returnResources(gameState, guardEnt);
		guardEnt.guard(criticalEnt, queued);
		let guardRole = guardEnt.getMetadata(PlayerID, "role") == "criticalEntHealer" ? "healer" : "guard";
		this.criticalEnts.get(criticalEntId).guards.set(guardEnt.id(), guardRole);

		// Switch this guard ent to the criticalEnt's base
		if (criticalEnt.hasClass("Structure") && criticalEnt.getMetadata(PlayerID, "base") !== undefined)
			guardEnt.setMetadata(PlayerID, "base", criticalEnt.getMetadata(PlayerID, "base"));
	}
	else
		gameState.ai.HQ.navalManager.requireTransport(gameState, guardEnt, guardEntAccess, criticalEntAccess, criticalEnt.position());

	this.guardEnts.set(guardEnt.id(), guardEntAccess == criticalEntAccess);
	return true;
};

m.GameTypeManager.prototype.resetCaptureGaiaRelic = function(gameState)
{
	// Do not capture gaia relics too frequently as the ai has access to the entire map
	this.tryCaptureGaiaRelicLapseTime = gameState.ai.elapsedTime + 240 - 30 * (this.Config.difficulty - 3);
	this.tryCaptureGaiaRelic = false;
};

m.GameTypeManager.prototype.update = function(gameState, events, queues)
{
	// Wait a turn for trigger scripts to spawn any critical ents (i.e. in regicide)
	if (gameState.ai.playedTurn == 1)
		this.init(gameState);

	this.checkEvents(gameState, events);

	if (gameState.ai.playedTurn % 10 != 0 ||
	    !gameState.getVictoryConditions().has("wonder") && !gameState.getVictoryConditions().has("regicide") &&
	    !gameState.getVictoryConditions().has("capture_the_relic"))
		return;

	this.manageCriticalEntGuards(gameState);

	if (gameState.getVictoryConditions().has("wonder"))
		gameState.ai.HQ.buildWonder(gameState, queues, true);

	if (gameState.getVictoryConditions().has("regicide"))
	{
		for (let id of this.criticalEnts.keys())
		{
			let ent = gameState.getEntityById(id);
			if (ent && ent.healthLevel() > this.Config.garrisonHealthLevel.high && ent.hasClass("Soldier") &&
			    ent.getStance() != "aggressive")
				ent.setStance("aggressive");
		}
		this.manageCriticalEntHealers(gameState, queues);
	}

	if (gameState.getVictoryConditions().has("capture_the_relic"))
	{
		if (!this.tryCaptureGaiaRelic && gameState.ai.elapsedTime > this.tryCaptureGaiaRelicLapseTime)
			this.tryCaptureGaiaRelic = true;

		// Reinforce (if needed) any raid currently trying to capture a gaia relic
		for (let relicId of this.targetedGaiaRelics.keys())
		{
			let relic = gameState.getEntityById(relicId);
			if (!relic || relic.owner() != 0)
				this.abortCaptureGaiaRelic(gameState, relicId);
			else
				this.captureGaiaRelic(gameState, relic);
		}
		// And look for some new gaia relics visible by any of our units
		// or that may be on our territory
		let allGaiaRelics = gameState.updatingGlobalCollection("allRelics", API3.Filters.byClass("Relic")).filter(relic => relic.owner() == 0);
		for (let relic of allGaiaRelics.values())
		{
			let relicPosition = relic.position();
			if (!relicPosition || this.targetedGaiaRelics.has(relic.id()))
				continue;
			let territoryOwner = gameState.ai.HQ.territoryMap.getOwner(relicPosition);
			if (territoryOwner == PlayerID)
			{
				this.targetedGaiaRelics.set(relic.id(), []);
				this.captureGaiaRelic(gameState, relic);
				break;
			}

			if (territoryOwner != 0 && gameState.isPlayerEnemy(territoryOwner))
				continue;

			for (let ent of gameState.getOwnUnits().values())
			{
				if (!ent.position() || !ent.visionRange())
					continue;
				if (API3.SquareVectorDistance(ent.position(), relicPosition) > Math.square(ent.visionRange()))
					continue;
				this.targetedGaiaRelics.set(relic.id(), []);
				this.captureGaiaRelic(gameState, relic);
				break;
			}
		}
	}
};

/**
 * Send an expedition to capture a gaia relic, or reinforce an existing one.
 */
m.GameTypeManager.prototype.captureGaiaRelic = function(gameState, relic)
{
	let capture = -relic.defaultRegenRate();
	let sumCapturePoints = relic.capturePoints().reduce((a, b) => a + b);
	let plans = this.targetedGaiaRelics.get(relic.id());
	for (let plan of plans)
	{
		let attack = gameState.ai.HQ.attackManager.getPlan(plan);
		if (!attack)
			continue;
		for (let ent of attack.unitCollection.values())
			capture += ent.captureStrength() * m.getAttackBonus(ent, relic, "Capture");
	}
	// No need to make a new attack if already enough units
	if (capture > sumCapturePoints / 50)
		return;
	let relicPosition = relic.position();
	let access = m.getLandAccess(gameState, relic);
	let units = gameState.getOwnUnits().filter(ent => {
		if (!ent.position() || !ent.canCapture(relic))
			return false;
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			return false;
		if (ent.getMetadata(PlayerID, "PartOfArmy") !== undefined)
			return false;
		let plan = ent.getMetadata(PlayerID, "plan");
		if (plan == -2 || plan == -3)
			return false;
		if (plan !== undefined && plan >= 0)
		{
			let attack = gameState.ai.HQ.attackManager.getPlan(plan);
			if (attack && (attack.state != "unexecuted" || attack.type == "Raid"))
				return false;
		}
		if (m.getLandAccess(gameState, ent) != access)
			return false;
		return true;
	}).filterNearest(relicPosition);
	let expedition = [];
	for (let ent of units.values())
	{
		capture += ent.captureStrength() * m.getAttackBonus(ent, relic, "Capture");
		expedition.push(ent);
		if (capture > sumCapturePoints / 25)
			break;
	}
	if (!expedition.length || !plans.length && capture < sumCapturePoints / 100)
		return;
	let attack = gameState.ai.HQ.attackManager.raidTargetEntity(gameState, relic);
	if (!attack)
		return;
	let plan = attack.name;
	attack.rallyPoint = undefined;
	for (let ent of expedition)
	{
		ent.setMetadata(PlayerID, "plan", plan);
		attack.unitCollection.updateEnt(ent);
		if (!attack.rallyPoint)
			attack.rallyPoint = ent.position();
	}
	attack.forceStart();
	this.targetedGaiaRelics.get(relic.id()).push(plan);
};

m.GameTypeManager.prototype.abortCaptureGaiaRelic = function(gameState, relicId)
{
	for (let plan of this.targetedGaiaRelics.get(relicId))
	{
		let attack = gameState.ai.HQ.attackManager.getPlan(plan);
		if (attack)
			attack.Abort(gameState);
	}
	this.targetedGaiaRelics.delete(relicId);
};

m.GameTypeManager.prototype.Serialize = function()
{
	return {
		"criticalEnts": this.criticalEnts,
		"guardEnts": this.guardEnts,
		"healersPerCriticalEnt": this.healersPerCriticalEnt,
		"tryCaptureGaiaRelic": this.tryCaptureGaiaRelic,
		"tryCaptureGaiaRelicLapseTime": this.tryCaptureGaiaRelicLapseTime,
		"targetedGaiaRelics": this.targetedGaiaRelics
	};
};

m.GameTypeManager.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
