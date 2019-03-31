var PETRA = function(m)
{

m.DefenseManager = function(Config)
{
	this.armies = [];	// array of "army" Objects
	this.Config = Config;
	this.targetList = [];
	this.armyMergeSize = this.Config.Defense.armyMergeSize;
	// stats on how many enemies are currently attacking our allies
	// this.attackingArmies[enemy][ally] = number of enemy armies inside allied territory
	// this.attackingUnits[enemy][ally] = number of enemy units not in armies inside allied territory
	// this.attackedAllies[ally] = number of enemies attacking the ally
	this.attackingArmies = {};
	this.attackingUnits = {};
	this.attackedAllies = {};
};

m.DefenseManager.prototype.update = function(gameState, events)
{
	Engine.ProfileStart("Defense Manager");

	this.territoryMap = gameState.ai.HQ.territoryMap;

	this.checkEvents(gameState, events);

	// Check if our potential targets are still valid
	for (let i = 0; i < this.targetList.length; ++i)
	{
		let target = gameState.getEntityById(this.targetList[i]);
		if (!target || !target.position() || !gameState.isPlayerEnemy(target.owner()))
			this.targetList.splice(i--, 1);
	}

	// Count the number of enemies attacking our allies in the previous turn
	// We'll be more cooperative if several enemies are attacking him simultaneously
	this.attackedAllies = {};
	let attackingArmies = clone(this.attackingArmies);
	for (let enemy in this.attackingUnits)
	{
		if (!this.attackingUnits[enemy])
			continue;
		for (let ally in this.attackingUnits[enemy])
		{
			if (this.attackingUnits[enemy][ally] < 8)
				continue;
			if (attackingArmies[enemy] === undefined)
				attackingArmies[enemy] = {};
			if (attackingArmies[enemy][ally] === undefined)
				attackingArmies[enemy][ally] = 0;
			attackingArmies[enemy][ally] += 1;
		}
	}
	for (let enemy in attackingArmies)
	{
		for (let ally in attackingArmies[enemy])
		{
			if (this.attackedAllies[ally] === undefined)
				this.attackedAllies[ally] = 0;
			this.attackedAllies[ally] += 1;
		}
	}
	this.checkEnemyArmies(gameState);
	this.checkEnemyUnits(gameState);
	this.assignDefenders(gameState);

	Engine.ProfileStop();
};

m.DefenseManager.prototype.makeIntoArmy = function(gameState, entityID, type = "default")
{
	if (type == "default")
	{
		// Try to add it to an existing army.
		for (let army of this.armies)
			if (army.getType() == type && army.addFoe(gameState, entityID))
				return;	// over
	}

	// Create a new army for it.
	let army = new m.DefenseArmy(gameState, [entityID], type);

	this.armies.push(army);
};

m.DefenseManager.prototype.getArmy = function(partOfArmy)
{
	// Find the army corresponding to this ID partOfArmy
	for (let army of this.armies)
		if (army.ID == partOfArmy)
			return army;

	return undefined;
};

m.DefenseManager.prototype.isDangerous = function(gameState, entity)
{
	if (!entity.position())
		return false;

	let territoryOwner = this.territoryMap.getOwner(entity.position());
	if (territoryOwner != 0 && !gameState.isPlayerAlly(territoryOwner))
		return false;
	// check if the entity is trying to build a new base near our buildings,
	// and if yes, add this base in our target list
	if (entity.unitAIState() && entity.unitAIState() == "INDIVIDUAL.REPAIR.REPAIRING")
	{
		let targetId = entity.unitAIOrderData()[0].target;
		if (this.targetList.indexOf(targetId) != -1)
			return true;
		let target = gameState.getEntityById(targetId);
		if (target)
		{
			let isTargetEnemy = gameState.isPlayerEnemy(target.owner());
			if (isTargetEnemy && territoryOwner == PlayerID)
			{
				if (target.hasClass("Structure"))
					this.targetList.push(targetId);
				return true;
			}
			else if (isTargetEnemy && target.hasClass("CivCentre"))
			{
				let myBuildings = gameState.getOwnStructures();
				for (let building of myBuildings.values())
				{
					if (building.foundationProgress() == 0)
						continue;
					if (API3.SquareVectorDistance(building.position(), entity.position()) > 30000)
						continue;
					this.targetList.push(targetId);
					return true;
				}
			}
		}
	}

	if (entity.attackTypes() === undefined || entity.hasClass("Support"))
		return false;
	let dist2Min = 6000;
	// TODO the 30 is to take roughly into account the structure size in following checks. Can be improved
	if (entity.attackTypes().indexOf("Ranged") != -1)
		dist2Min = (entity.attackRange("Ranged").max + 30) * (entity.attackRange("Ranged").max + 30);

	for (let targetId of this.targetList)
	{
		let target = gameState.getEntityById(targetId);
		if (!target || !target.position())   // the enemy base is either destroyed or built
			continue;
		if (API3.SquareVectorDistance(target.position(), entity.position()) < dist2Min)
			return true;
	}

	let ccEnts = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
	for (let cc of ccEnts.values())
	{
		if (!gameState.isEntityExclusiveAlly(cc) || cc.foundationProgress() == 0)
			continue;
		let cooperation = this.GetCooperationLevel(cc.owner());
		if (cooperation < 0.6 && cc.foundationProgress() !== undefined)
			continue;
		if (cooperation < 0.3)
			continue;
		if (API3.SquareVectorDistance(cc.position(), entity.position()) < dist2Min)
			return true;
	}

	for (let building of gameState.getOwnStructures().values())
	{
		if (building.foundationProgress() == 0 ||
		    API3.SquareVectorDistance(building.position(), entity.position()) > dist2Min)
			continue;
		if (!this.territoryMap.isBlinking(building.position()) || gameState.ai.HQ.isDefendable(building))
			return true;
	}

	if (gameState.isPlayerMutualAlly(territoryOwner))
	{
		// If ally attacked by more than 2 enemies, help him not only for cc but also for structures
		if (territoryOwner != PlayerID && this.attackedAllies[territoryOwner] &&
		                                  this.attackedAllies[territoryOwner] > 1 &&
		                                  this.GetCooperationLevel(territoryOwner) > 0.7)
		{
			for (let building of gameState.getAllyStructures(territoryOwner).values())
			{
				if (building.foundationProgress() == 0 ||
				    API3.SquareVectorDistance(building.position(), entity.position()) > dist2Min)
					continue;
				if (!this.territoryMap.isBlinking(building.position()))
					return true;
			}
		}

		// Update the number of enemies attacking this ally
		let enemy = entity.owner();
		if (this.attackingUnits[enemy] === undefined)
			this.attackingUnits[enemy] = {};
		if (this.attackingUnits[enemy][territoryOwner] === undefined)
			this.attackingUnits[enemy][territoryOwner] = 0;
		this.attackingUnits[enemy][territoryOwner] += 1;
	}

	return false;
};

m.DefenseManager.prototype.checkEnemyUnits = function(gameState)
{
	const nbPlayers = gameState.sharedScript.playersData.length;
	let i = gameState.ai.playedTurn % nbPlayers;
	this.attackingUnits[i] = undefined;

	if (i == PlayerID)
	{
		if (!this.armies.length)
		{
			// check if we can recover capture points from any of our notdecaying structures
			for (let ent of gameState.getOwnStructures().values())
			{
				if (ent.decaying())
					continue;
				let capture = ent.capturePoints();
				if (capture === undefined)
					continue;
				let lost = 0;
				for (let j = 0; j < capture.length; ++j)
					if (gameState.isPlayerEnemy(j))
						lost += capture[j];
				if (lost < Math.ceil(0.25 * capture[i]))
					continue;
				this.makeIntoArmy(gameState, ent.id(), "capturing");
				break;
			}
		}
		return;
	}
	else if (!gameState.isPlayerEnemy(i))
		return;

	// loop through enemy units
	for (let ent of gameState.getEnemyUnits(i).values())
	{
		if (ent.getMetadata(PlayerID, "PartOfArmy") !== undefined)
			continue;

		// keep animals attacking us or our allies
		if (ent.hasClass("Animal"))
		{
			if (!ent.unitAIState() || ent.unitAIState().split(".")[1] != "COMBAT")
				continue;
			let orders = ent.unitAIOrderData();
			if (!orders || !orders.length || !orders[0].target)
				continue;
			let target = gameState.getEntityById(orders[0].target);
			if (!target || !gameState.isPlayerAlly(target.owner()))
				continue;
		}

		// TODO what to do for ships ?
		if (ent.hasClass("Ship") || ent.hasClass("Trader"))
			continue;

		// check if unit is dangerous "a priori"
		if (this.isDangerous(gameState, ent))
			this.makeIntoArmy(gameState, ent.id());
	}

	if (i != 0 || this.armies.length > 1 || gameState.ai.HQ.numActiveBases() == 0)
		return;
	// look for possible gaia buildings inside our territory (may happen when enemy resign or after structure decay)
	// and attack it only if useful (and capturable) or dangereous
	for (let ent of gameState.getEnemyStructures(i).values())
	{
		if (!ent.position() || ent.getMetadata(PlayerID, "PartOfArmy") !== undefined)
			continue;
		if (!ent.capturePoints() && !ent.hasDefensiveFire())
			continue;
		let owner = this.territoryMap.getOwner(ent.position());
		if (owner == PlayerID)
			this.makeIntoArmy(gameState, ent.id(), "capturing");
	}
};

m.DefenseManager.prototype.checkEnemyArmies = function(gameState)
{
	for (let i = 0; i < this.armies.length; ++i)
	{
		let army = this.armies[i];
		// this returns a list of IDs: the units that broke away from the army for being too far.
		let breakaways = army.update(gameState);
		for (let breaker of breakaways)
			this.makeIntoArmy(gameState, breaker);		// assume dangerosity

		if (army.getState() == 0)
		{
			if (army.getType() == "default")
				this.switchToAttack(gameState, army);
			army.clear(gameState);
			this.armies.splice(i--, 1);
			continue;
		}
	}
	// Check if we can't merge it with another
	for (let i = 0; i < this.armies.length - 1; ++i)
	{
		let army = this.armies[i];
		if (army.getType() != "default")
			continue;
		for (let j = i+1; j < this.armies.length; ++j)
		{
			let otherArmy = this.armies[j];
			if (otherArmy.getType() != "default" ||
				API3.SquareVectorDistance(army.foePosition, otherArmy.foePosition) > this.armyMergeSize)
				continue;
			// no need to clear here.
			army.merge(gameState, otherArmy);
			this.armies.splice(j--, 1);
		}
	}

	if (gameState.ai.playedTurn % 5 != 0)
		return;
	// Check if any army is no more dangerous (possibly because it has defeated us and destroyed our base)
	this.attackingArmies = {};
	for (let i = 0; i < this.armies.length; ++i)
	{
		let army = this.armies[i];
		army.recalculatePosition(gameState);
		let owner = this.territoryMap.getOwner(army.foePosition);
		if (!gameState.isPlayerEnemy(owner))
		{
			if (gameState.isPlayerMutualAlly(owner))
			{
				// update the number of enemies attacking this ally
				for (let id of army.foeEntities)
				{
					let ent = gameState.getEntityById(id);
					if (!ent)
						continue;
					let enemy = ent.owner();
					if (this.attackingArmies[enemy] === undefined)
						this.attackingArmies[enemy] = {};
					if (this.attackingArmies[enemy][owner] === undefined)
						this.attackingArmies[enemy][owner] = 0;
					this.attackingArmies[enemy][owner] += 1;
					break;
				}
			}
			continue;
		}
		else if (owner != 0)   // enemy army back in its territory
		{
			army.clear(gameState);
			this.armies.splice(i--, 1);
			continue;
		}

		// army in neutral territory
		// TODO check smaller distance with all our buildings instead of only ccs with big distance
		let stillDangerous = false;
		let bases = gameState.updatingGlobalCollection("allCCs", API3.Filters.byClass("CivCentre"));
		for (let base of bases.values())
		{
			if (!gameState.isEntityAlly(base))
				continue;
			let cooperation = this.GetCooperationLevel(base.owner());
			if (cooperation < 0.3 && !gameState.isEntityOwn(base))
				continue;
			if (API3.SquareVectorDistance(base.position(), army.foePosition) > 40000)
				continue;
			if(this.Config.debug > 1)
				API3.warn("army in neutral territory, but still near one of our CC");
			stillDangerous = true;
			break;
		}
		if (stillDangerous)
			continue;
		// Need to also check docks because of oversea bases
		for (let dock of gameState.getOwnStructures().filter(API3.Filters.byClass("Dock")).values())
		{
			if (API3.SquareVectorDistance(dock.position(), army.foePosition) > 10000)
				continue;
			stillDangerous = true;
			break;
		}
		if (stillDangerous)
			continue;

		if (army.getType() == "default")
			this.switchToAttack(gameState, army);
		army.clear(gameState);
		this.armies.splice(i--, 1);
	}
};

m.DefenseManager.prototype.assignDefenders = function(gameState)
{
	if (!this.armies.length)
		return;

	let armiesNeeding = [];
	// let's add defenders
	for (let army of this.armies)
	{
		let needsDef = army.needsDefenders(gameState);
		if (needsDef === false)
			continue;

		let armyAccess;
		for (let entId of army.foeEntities)
		{
			let ent = gameState.getEntityById(entId);
			if (!ent || !ent.position())
				continue;
			armyAccess = m.getLandAccess(gameState, ent);
			break;
		}
		if (!armyAccess)
			API3.warn(" Petra error: attacking army " + army.ID + " without access");
		army.recalculatePosition(gameState);
		armiesNeeding.push({ "army": army, "access": armyAccess, "need": needsDef });
	}

	if (!armiesNeeding.length)
		return;

	// let's get our potential units
	let potentialDefenders = [];
	gameState.getOwnUnits().forEach(function(ent) {
		if (!ent.position())
			return;
		if (ent.getMetadata(PlayerID, "plan") == -2 || ent.getMetadata(PlayerID, "plan") == -3)
			return;
		if (ent.hasClass("Support") || ent.attackTypes() === undefined)
			return;
		if (ent.hasClass("Catapult"))
			return;
		if (ent.hasClass("FishingBoat") || ent.hasClass("Trader"))
			return;
		if (ent.getMetadata(PlayerID, "transport") !== undefined ||
		    ent.getMetadata(PlayerID, "transporter") !== undefined)
			return;
		if (gameState.ai.HQ.victoryManager.criticalEnts.has(ent.id()))
			return;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") != -1)
		{
			let subrole = ent.getMetadata(PlayerID, "subrole");
			if (subrole && (subrole == "completing" || subrole == "walking" || subrole == "attacking"))
				return;
		}
		potentialDefenders.push(ent.id());
	});

	for (let ipass = 0; ipass < 2; ++ipass)
	{
		// First pass only assign defenders with the right access
		// Second pass assign all defenders
		// TODO could sort them by distance
		let backup = 0;
		for (let i = 0; i < potentialDefenders.length; ++i)
		{
			let ent = gameState.getEntityById(potentialDefenders[i]);
			if (!ent || !ent.position())
				continue;
			let aMin;
			let distMin;
			let access = ipass == 0 ? m.getLandAccess(gameState, ent) : undefined;
			for (let a = 0; a < armiesNeeding.length; ++a)
			{
				if (access && armiesNeeding[a].access != access)
					continue;
				let dist = API3.SquareVectorDistance(ent.position(), armiesNeeding[a].army.foePosition);
				if (aMin !== undefined && dist > distMin)
					continue;
				aMin = a;
				distMin = dist;
			}

			// If outside our territory (helping an ally or attacking a cc foundation)
			// or if in another access, keep some troops in backup
			if (backup < 12 && (aMin == undefined || distMin > 40000 &&
			        this.territoryMap.getOwner(armiesNeeding[aMin].army.foePosition) != PlayerID))
			{
				++backup;
				potentialDefenders[i] = undefined;
				continue;
			}
			else if (aMin === undefined)
				continue;

			armiesNeeding[aMin].need -= m.getMaxStrength(ent);
			armiesNeeding[aMin].army.addOwn(gameState, potentialDefenders[i]);
			armiesNeeding[aMin].army.assignUnit(gameState, potentialDefenders[i]);
			potentialDefenders[i] = undefined;

			if (armiesNeeding[aMin].need <= 0)
				armiesNeeding.splice(aMin, 1);
			if (!armiesNeeding.length)
				return;
		}
	}

	// If shortage of defenders, produce infantry garrisoned in nearest civil center
	let armiesPos = [];
	for (let a = 0; a < armiesNeeding.length; ++a)
		armiesPos.push(armiesNeeding[a].army.foePosition);
	gameState.ai.HQ.trainEmergencyUnits(gameState, armiesPos);
};

m.DefenseManager.prototype.abortArmy = function(gameState, army)
{
	army.clear(gameState);
	for (let i = 0; i < this.armies.length; ++i)
	{
		if (this.armies[i].ID != army.ID)
			continue;
		this.armies.splice(i, 1);
		break;
	}
};

/**
 * If our defense structures are attacked, garrison soldiers inside when possible
 * and if a support unit is attacked and has less than 55% health, garrison it inside the nearest healing structure
 * and if a ranged siege unit (not used for defense) is attacked, garrison it in the nearest fortress
 * If our hero is attacked with regicide victory condition, the victoryManager will handle it
 */
m.DefenseManager.prototype.checkEvents = function(gameState, events)
{
	// must be called every turn for all armies
	for (let army of this.armies)
		army.checkEvents(gameState, events);

	for (let evt of events.OwnershipChanged)   // capture events
	{
		if (gameState.isPlayerMutualAlly(evt.from) && evt.to > 0)
		{
			let ent = gameState.getEntityById(evt.entity);
			if (ent && ent.hasClass("CivCentre")) // one of our cc has been captured
				gameState.ai.HQ.attackManager.switchDefenseToAttack(gameState, ent, { "range": 150 });
		}
	}

	let allAttacked = {};
	for (let evt of events.Attacked)
		allAttacked[evt.target] = evt.attacker;

	for (let evt of events.Attacked)
	{
		let target = gameState.getEntityById(evt.target);
		if (!target || !target.position())
			continue;

		let attacker = gameState.getEntityById(evt.attacker);
		if (attacker && gameState.isEntityOwn(attacker) && gameState.isEntityEnemy(target) && !attacker.hasClass("Ship") &&
		   (!target.hasClass("Structure") || target.attackRange("Ranged")))
		{
			// If enemies are in range of one of our defensive structures, garrison it for arrow multiplier
			// (enemy non-defensive structure are not considered to stay in sync with garrisonManager)
			if (attacker.position() && attacker.isGarrisonHolder() && attacker.getArrowMultiplier() &&
			    (target.owner() != 0 || !target.hasClass("Unit") ||
			     target.unitAIState() && target.unitAIState().split(".")[1] == "COMBAT"))
				this.garrisonUnitsInside(gameState, attacker, { "attacker": target });
		}

		if (!gameState.isEntityOwn(target))
			continue;

		// If attacked by one of our allies (he must trying to recover capture points), do not react
		if (attacker && gameState.isEntityAlly(attacker))
			continue;

		if (attacker && attacker.position() && target.hasClass("FishingBoat"))
		{
			let unitAIState = target.unitAIState();
			let unitAIStateOrder = unitAIState ? unitAIState.split(".")[1] : "";
			if (target.isIdle() || unitAIStateOrder == "GATHER")
			{
				let pos = attacker.position();
				let range = attacker.attackRange("Ranged") ? attacker.attackRange("Ranged").max + 15 : 25;
				if (range * range > API3.SquareVectorDistance(pos, target.position()))
					target.moveToRange(pos[0], pos[1], range, range);
			}
			continue;
		}

		if (target.hasClass("Ship"))    // TODO integrate other ships later, need to be sure it is accessible
			continue;

		// If a building on a blinking tile is attacked, check if it can be defended.
		// Same thing for a building in an isolated base (not connected to a base with anchor).
		if (target.hasClass("Structure"))
		{
			let base = gameState.ai.HQ.getBaseByID(target.getMetadata(PlayerID, "base"));
			if (this.territoryMap.isBlinking(target.position()) && !gameState.ai.HQ.isDefendable(target) ||
			    !base || gameState.ai.HQ.baseManagers.every(b => !b.anchor || b.accessIndex != base.accessIndex))
			{
				let capture = target.capturePoints();
				if (!capture)
					continue;
				let captureRatio = capture[PlayerID] / capture.reduce((a, b) => a + b);
				if (captureRatio > 0.50 && captureRatio < 0.70)
					target.destroy();
				continue;
			}
		}


		// If inside a started attack plan, let the plan deal with this unit
		let plan = target.getMetadata(PlayerID, "plan");
		if (plan !== undefined && plan >= 0)
		{
			let attack = gameState.ai.HQ.attackManager.getPlan(plan);
			if (attack && attack.state != "unexecuted")
				continue;
		}

		// Signal this attacker to our defense manager, except if we are in enemy territory
		// TODO treat ship attack
		if (attacker && attacker.position() && attacker.getMetadata(PlayerID, "PartOfArmy") === undefined &&
			!attacker.hasClass("Structure") && !attacker.hasClass("Ship"))
		{
			let territoryOwner = this.territoryMap.getOwner(attacker.position());
			if (territoryOwner == 0 || gameState.isPlayerAlly(territoryOwner))
				this.makeIntoArmy(gameState, attacker.id());
		}

		if (target.getMetadata(PlayerID, "PartOfArmy") !== undefined)
		{
			let army = this.getArmy(target.getMetadata(PlayerID, "PartOfArmy"));
			if (army.getType() == "capturing")
			{
				let abort = false;
				// if one of the units trying to capture a structure is attacked,
				// abort the army so that the unit can defend itself
				if (army.ownEntities.indexOf(target.id()) != -1)
					abort = true;
				else if (army.foeEntities[0] == target.id() && target.owner() == PlayerID)
				{
					// else we may be trying to regain some capture point from one of our structure
					abort = true;
					let capture = target.capturePoints();
					for (let j = 0; j < capture.length; ++j)
					{
						if (!gameState.isPlayerEnemy(j) || capture[j] == 0)
							continue;
						abort = false;
						break;
					}
				}
				if (abort)
					this.abortArmy(gameState, army);
			}
			continue;
		}

		// try to garrison any attacked support unit if low healthlevel
		if (target.hasClass("Support") && target.healthLevel() < this.Config.garrisonHealthLevel.medium &&
			!target.getMetadata(PlayerID, "transport") && plan != -2 && plan != -3)
		{
			this.garrisonAttackedUnit(gameState, target);
			continue;
		}

		// try to garrison any attacked catapult
		if (target.hasClass("Catapult") &&
			!target.getMetadata(PlayerID, "transport") && plan != -2 && plan != -3)
		{
			this.garrisonSiegeUnit(gameState, target);
			continue;
		}

		if (!attacker || !attacker.position())
			continue;

		if (target.isGarrisonHolder() && target.getArrowMultiplier())
			this.garrisonUnitsInside(gameState, target, { "attacker": attacker });

		if (target.hasClass("Unit") && attacker.hasClass("Unit"))
		{
			// Consider if we should retaliate or continue our task
			if (target.hasClass("Support") || target.attackTypes() === undefined)
				continue;
			let orderData = target.unitAIOrderData();
			let currentTarget = orderData && orderData.length && orderData[0].target ?
				gameState.getEntityById(orderData[0].target) : undefined;
			if (currentTarget)
			{
				let unitAIState = target.unitAIState();
				let unitAIStateOrder = unitAIState ? unitAIState.split(".")[1] : "";
				if (unitAIStateOrder == "COMBAT" && (currentTarget == attacker.id() ||
					!currentTarget.hasClass("Structure") && !currentTarget.hasClass("Support")))
					continue;
				if (unitAIStateOrder == "REPAIR" && currentTarget.hasDefensiveFire())
					continue;
				if (unitAIStateOrder == "COMBAT" && !m.isSiegeUnit(currentTarget) &&
				    gameState.ai.HQ.capturableTargets.has(orderData[0].target))
				{
					// take the nearest unit also attacking this structure to help us
					let capturableTarget = gameState.ai.HQ.capturableTargets.get(orderData[0].target);
					let minDist;
					let minEnt;
					let pos = attacker.position();
					capturableTarget.ents.delete(target.id());
					for (let entId of capturableTarget.ents)
					{
						if (allAttacked[entId])
							continue;
						let ent = gameState.getEntityById(entId);
						if (!ent || !ent.position())
							continue;
						// Check that the unit is still attacking the structure (since the last played turn)
						let state = ent.unitAIState();
						if (!state || !state.split(".")[1] || state.split(".")[1] != "COMBAT")
							continue;
						let entOrderData = ent.unitAIOrderData();
						if (!entOrderData || !entOrderData.length || !entOrderData[0].target ||
						     entOrderData[0].target != orderData[0].target)
							continue;
						let dist = API3.SquareVectorDistance(pos, ent.position());
						if (minEnt && dist > minDist)
							continue;
						minDist = dist;
						minEnt = ent;
					}
					if (minEnt)
					{
						capturableTarget.ents.delete(minEnt.id());
						minEnt.attack(attacker.id(), m.allowCapture(gameState, minEnt, attacker));
					}
				}
			}
			target.attack(attacker.id(), m.allowCapture(gameState, target, attacker));
		}
	}
};

m.DefenseManager.prototype.garrisonUnitsInside = function(gameState, target, data)
{
	if (target.hitpoints() < target.garrisonEjectHealth() * target.maxHitpoints())
		return false;
	let minGarrison = data.min || target.garrisonMax();
	if (gameState.ai.HQ.garrisonManager.numberOfGarrisonedUnits(target) >= minGarrison)
		return false;
	if (data.attacker)
	{
		let attackTypes = target.attackTypes();
		if (!attackTypes || attackTypes.indexOf("Ranged") == -1)
			return false;
		let dist = API3.SquareVectorDistance(data.attacker.position(), target.position());
		let range = target.attackRange("Ranged").max;
		if (dist >= range*range)
			return false;
	}
	let access = m.getLandAccess(gameState, target);
	let garrisonManager = gameState.ai.HQ.garrisonManager;
	let garrisonArrowClasses = target.getGarrisonArrowClasses();
	let typeGarrison = data.type || "protection";
	let allowMelee = gameState.ai.HQ.garrisonManager.allowMelee(target);
	if (allowMelee === undefined)
	{
		// Should be kept in sync with garrisonManager to avoid garrisoning-ungarrisoning some units
		if (data.attacker)
			allowMelee = data.attacker.hasClass("Structure") ? data.attacker.attackRange("Ranged") : !m.isSiegeUnit(data.attacker);
		else
			allowMelee = true;
	}
	let units = gameState.getOwnUnits().filter(ent => {
		if (!ent.position())
			return false;
		if (!MatchesClassList(ent.classes(), garrisonArrowClasses))
			return false;
		if (typeGarrison != "decay" && !allowMelee && ent.attackTypes().indexOf("Melee") != -1)
			return false;
		if (ent.getMetadata(PlayerID, "transport") !== undefined)
			return false;
		let army = ent.getMetadata(PlayerID, "PartOfArmy") ? this.getArmy(ent.getMetadata(PlayerID, "PartOfArmy")) : undefined;
		if (!army && (ent.getMetadata(PlayerID, "plan") == -2 || ent.getMetadata(PlayerID, "plan") == -3))
			return false;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") >= 0)
		{
			let subrole = ent.getMetadata(PlayerID, "subrole");
			// when structure decaying (usually because we've just captured it in enemy territory), also allow units from an attack plan
			if (typeGarrison != "decay" && subrole && (subrole == "completing" || subrole == "walking" || subrole == "attacking"))
				return false;
		}
		if (m.getLandAccess(gameState, ent) != access)
			return false;
		return true;
	}).filterNearest(target.position());

	let ret = false;
	for (let ent of units.values())
	{
		if (garrisonManager.numberOfGarrisonedUnits(target) >= minGarrison)
			break;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") >= 0)
		{
			let attackPlan = gameState.ai.HQ.attackManager.getPlan(ent.getMetadata(PlayerID, "plan"));
			if (attackPlan)
				attackPlan.removeUnit(ent, true);
		}
		let army = ent.getMetadata(PlayerID, "PartOfArmy") ? this.getArmy(ent.getMetadata(PlayerID, "PartOfArmy")) : undefined;
		if (army)
			army.removeOwn(gameState, ent.id());
		garrisonManager.garrison(gameState, ent, target, typeGarrison);
		ret = true;
	}
	return ret;
};

/** garrison a attacked siege ranged unit inside the nearest fortress */
m.DefenseManager.prototype.garrisonSiegeUnit = function(gameState, unit)
{
	let distmin = Math.min();
	let nearest;
	let unitAccess = m.getLandAccess(gameState, unit);
	let garrisonManager = gameState.ai.HQ.garrisonManager;
	for (let ent of gameState.getAllyStructures().values())
	{
		if (!ent.isGarrisonHolder())
			continue;
		if (!MatchesClassList(unit.classes(), ent.garrisonableClasses()))
			continue;
		if (garrisonManager.numberOfGarrisonedUnits(ent) >= ent.garrisonMax())
			continue;
		if (ent.hitpoints() < ent.garrisonEjectHealth() * ent.maxHitpoints())
			continue;
		if (m.getLandAccess(gameState, ent) != unitAccess)
			continue;
		let dist = API3.SquareVectorDistance(ent.position(), unit.position());
		if (dist > distmin)
			continue;
		distmin = dist;
		nearest = ent;
	}
	if (nearest)
		garrisonManager.garrison(gameState, unit, nearest, "protection");
	return nearest !== undefined;
};

/**
 * Garrison a hurt unit inside a player-owned or allied structure
 * If emergency is true, the unit will be garrisoned in the closest possible structure
 * Otherwise, it will garrison in the closest healing structure
 */
m.DefenseManager.prototype.garrisonAttackedUnit = function(gameState, unit, emergency = false)
{
	let distmin = Math.min();
	let nearest;
	let unitAccess = m.getLandAccess(gameState, unit);
	let garrisonManager = gameState.ai.HQ.garrisonManager;
	for (let ent of gameState.getAllyStructures().values())
	{
		if (!ent.isGarrisonHolder())
			continue;
		if (!emergency && !ent.buffHeal())
			continue;
		if (!MatchesClassList(unit.classes(), ent.garrisonableClasses()))
			continue;
		if (garrisonManager.numberOfGarrisonedUnits(ent) >= ent.garrisonMax() &&
		    (!emergency || !ent.garrisoned().length))
			continue;
		if (ent.hitpoints() < ent.garrisonEjectHealth() * ent.maxHitpoints())
			continue;
		if (m.getLandAccess(gameState, ent) != unitAccess)
			continue;
		let dist = API3.SquareVectorDistance(ent.position(), unit.position());
		if (dist > distmin)
			continue;
		distmin = dist;
		nearest = ent;
	}
	if (!nearest)
		return false;

	if (!emergency)
	{
		garrisonManager.garrison(gameState, unit, nearest, "protection");
		return true;
	}
	if (garrisonManager.numberOfGarrisonedUnits(nearest) >= nearest.garrisonMax()) // make room for this ent
		nearest.unload(nearest.garrisoned()[0]);

	garrisonManager.garrison(gameState, unit, nearest, nearest.buffHeal() ? "protection" : "emergency");
	return true;
};

/**
 * Be more inclined to help an ally attacked by several enemies
 */
m.DefenseManager.prototype.GetCooperationLevel = function(ally)
{
	let cooperation = this.Config.personality.cooperative;
	if (this.attackedAllies[ally] && this.attackedAllies[ally] > 1)
		cooperation += 0.2 * (this.attackedAllies[ally] - 1);
	return cooperation;
};

/**
 * Switch a defense army into an attack if needed
 */
m.DefenseManager.prototype.switchToAttack = function(gameState, army)
{
	if (!army)
		return;
	for (let targetId of this.targetList)
	{
		let target = gameState.getEntityById(targetId);
		if (!target || !target.position() || !gameState.isPlayerEnemy(target.owner()))
			continue;
		let targetAccess = m.getLandAccess(gameState, target);
		let targetPos = target.position();
		for (let entId of army.ownEntities)
		{
			let ent = gameState.getEntityById(entId);
			if (!ent || !ent.position() || m.getLandAccess(gameState, ent) != targetAccess)
				continue;
			if (API3.SquareVectorDistance(targetPos, ent.position()) > 14400)
				continue;
			gameState.ai.HQ.attackManager.switchDefenseToAttack(gameState, target, { "armyID": army.ID, "uniqueTarget": true });
			return;
		}
	}
};

m.DefenseManager.prototype.Serialize = function()
{
	let properties = {
		"targetList": this.targetList,
		"armyMergeSize": this.armyMergeSize,
		"attackingUnits": this.attackingUnits,
		"attackingArmies": this.attackingArmies,
		"attackedAllies": this.attackedAllies
	};

	let armies = [];
	for (let army of this.armies)
		armies.push(army.Serialize());

	return { "properties": properties, "armies": armies };
};

m.DefenseManager.prototype.Deserialize = function(gameState, data)
{
	for (let key in data.properties)
		this[key] = data.properties[key];

	this.armies = [];
	for (let dataArmy of data.armies)
	{
		let army = new m.DefenseArmy(gameState, []);
		army.Deserialize(dataArmy);
		this.armies.push(army);
	}
};

return m;
}(PETRA);
