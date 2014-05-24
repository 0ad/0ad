var PETRA = function(m)
{

m.DefenseManager = function(Config)
{
	this.armies = [];	// array of "army" Objects
	this.Config = Config;
	this.targetList = [];
}

m.DefenseManager.prototype.init = function(gameState)
{
	this.armyMergeSize = this.Config.Defense.armyMergeSize;
};

m.DefenseManager.prototype.update = function(gameState, events)
{
	this.territoryMap = gameState.ai.HQ.territoryMap;
	
	this.checkDefenseStructures(gameState, events);

	this.checkEnemyArmies(gameState, events);
	this.checkEnemyUnits(gameState);
	this.assignDefenders(gameState);
	
	this.MessageProcess(gameState,events);
};

m.DefenseManager.prototype.makeIntoArmy = function(gameState, entityID)
{
	// Try to add it to an existing army.
	for (var o in this.armies)
		if (this.armies[o].addFoe(gameState,entityID))
			return;	// over

	// Create a new army for it.
	var army = new m.DefenseArmy(gameState, this, [], [entityID]);
	this.armies.push(army);
};

m.DefenseManager.prototype.getArmy = function(partOfArmy)
{
	// Find the army corresponding to this ID partOfArmy
	for (var o in this.armies)
		if (this.armies[o].ID === partOfArmy)
		    return this.armies[o];

	return undefined;
};

// TODO: this algorithm needs to be improved, sorta.
m.DefenseManager.prototype.isDangerous = function(gameState, entity)
{
	if (!entity.position())
		return false;

	if (this.territoryMap.getOwner(entity.position()) === entity.owner())
		return false;

	// check if the entity is trying to build a new base near our buildings, and if yes, add this base in our target list
	if (entity.unitAIState() == "INDIVIDUAL.REPAIR.REPAIRING")
	{
		var targetId = entity.unitAIOrderData()[0]["target"];
		if (this.targetList.indexOf(targetId) !== -1)
			return true;
		var target = gameState.getEntityById(targetId);
		if (target && this.territoryMap.getOwner(entity.position()) === PlayerID)
		{
			this.targetList.push(targetId);
			return true;
		}
		else if (target && target.hasClass("CivCentre"))
		{
			var myBuildings = gameState.getOwnStructures();
			for (var i in myBuildings._entities)
			{
				if (API3.SquareVectorDistance(myBuildings._entities[i].position(), entity.position()) > 30000)
					continue;
				this.targetList.push(targetId);
				return true;
			}
		}
	}

	if (entity.attackTypes() === undefined || entity.hasClass("Support"))
		return false;

	for (var i = 0; i < this.targetList.length; ++i)
	{
		var target = gameState.getEntityById(this.targetList[i]);
		if (!target || !target.position())   // the enemy base is either destroyed or built
			this.targetList.splice(i--, 1);
		else if (API3.SquareVectorDistance(target.position(), entity.position()) < 6000)
			return true;
	}

	var myCCFoundations = gameState.getOwnFoundations().filter(API3.Filters.byClass("CivCentre"));
	for (var i in myCCFoundations._entities)
	{
		if (!myCCFoundations._entities[i].getBuildersNb())
			continue;
		if (API3.SquareVectorDistance(myCCFoundations._entities[i].position(), entity.position()) < 6000)
			return true;
	}

	if (this.Config.personality.cooperative > 0.3)
	{
		var allyCC = gameState.getAllyEntities().filter(API3.Filters.byClass("CivCentre"));
		for (var i in allyCC._entities)
		{
			if (this.Config.personality.cooperative < 0.6 && allyCC._entities[i].foundationProgress() !== undefined)
				continue;
			if (API3.SquareVectorDistance(allyCC._entities[i].position(), entity.position()) < 6000)
				return true;
		}
	}

	var myBuildings = gameState.getOwnStructures();
	for (var i in myBuildings._entities)
		if (API3.SquareVectorDistance(myBuildings._entities[i].position(), entity.position()) < 6000)
			return true;

	return false;
};


m.DefenseManager.prototype.checkEnemyUnits = function(gameState)
{
	var self = this;
	
	// loop through enemy units
	var nbPlayers = gameState.sharedScript.playersData.length - 1;
	var i = 1 + gameState.ai.playedTurn % nbPlayers;
	if (i === PlayerID || gameState.isPlayerAlly(i))
		return;
	
	var filter = API3.Filters.and(API3.Filters.byClass("Unit"), API3.Filters.byOwner(i));
	var enemyUnits = gameState.updatingGlobalCollection("player-" +i + "-units", filter);
	
	enemyUnits.forEach( function (ent) {
		// first check: is this unit already part of an army.
		if (ent.getMetadata(PlayerID, "PartOfArmy") !== undefined)
			return;
		
		// TODO what to do for ships ?
		if (ent.hasClass("Ship") || ent.hasClass("Trader"))
			return;
		
		// check if unit is dangerous "a priori"
		if (self.isDangerous(gameState, ent))
			self.makeIntoArmy(gameState, ent.id());
	});
};

m.DefenseManager.prototype.checkEnemyArmies = function(gameState, events)
{
	var self = this;

	for (var o = 0; o < this.armies.length; ++o)
	{
		var army = this.armies[o];
		army.checkEvents(gameState, events);	// must be called every turn for all armies
		
		// this returns a list of IDs: the units that broke away from the army for being too far.
		var breakaways = army.update(gameState);

		for (var u in breakaways)
		{
			// assume dangerosity
			this.makeIntoArmy(gameState,breakaways[u]);
		}
		
		if (army.getState(gameState) === 0)
		{
			army.clear(gameState);
			this.armies.splice(o--,1);
			continue;
		}
	}
	// Check if we can't merge it with another.
	for (var o = 0; o < this.armies.length; ++o)
	{
		var army = this.armies[o];
		for (var p = o+1; p < this.armies.length; ++p)
		{
			var otherArmy = this.armies[p];
			if (API3.SquareVectorDistance(army.foePosition, otherArmy.foePosition) < this.armyMergeSize)
			{
				// no need to clear here.
				army.merge(gameState, otherArmy);
				this.armies.splice(p--,1);
			}
		}
	}

	if (gameState.ai.playedTurn % 5 !== 0)
		return;
	// Check if any army is no more dangerous (possibly because it has defeated us and destroyed our base)
	for (var o = 0; o < this.armies.length; ++o)
	{
		var army = this.armies[o];
		army.recalculatePosition(gameState);
		var owner = this.territoryMap.getOwner(army.foePosition);
		if (gameState.isPlayerAlly(owner))
			continue;
		else if (owner !== 0)   // enemy army back in its territory
		{
			army.clear(gameState);
			this.armies.splice(o--,1);
			continue;
		}

		// army in neutral territory // TODO check smaller distance with all our buildings instead of only ccs with big distance
		var stillDangerous = false;
		var bases = gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
		if (this.Config.personality.cooperative > 0.3)
		{
			var allyCC = gameState.getAllyEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
			bases = bases.concat(allyCC);
		}
	 	for (var i in bases)
		{
			if (API3.SquareVectorDistance(bases[i].position(), army.foePosition) < 40000)
			{
				if(this.Config.debug > 0)
					warn("army in neutral territory, but still near one of our CC");
				stillDangerous = true;
				break;
			}
		}
		if (stillDangerous)
			continue;

		army.clear(gameState);
		this.armies.splice(o--,1);
	}
};

m.DefenseManager.prototype.assignDefenders = function(gameState, events)
{
	if (this.armies.length === 0)
		return;
	
	var armiesNeeding = [];
	// Okay, let's add defenders
	// TODO: this is dumb.
	for (var i in this.armies)
	{
		var army = this.armies[i];
		var needsDef = army.needsDefenders(gameState);
		if (needsDef === false)
			continue;
		
		// Okay for now needsDef is the total needed strength.
		// we're dumb so we don't choose if we have a defender shortage.
		armiesNeeding.push( {"army": army, "need": needsDef} );
	}

	if (armiesNeeding.length === 0)
		return;

	// let's get our potential units
	var potentialDefenders = []; 
	gameState.getOwnUnits().forEach(function(ent) {
		if (!ent.position())
			return;
		if (ent.getMetadata(PlayerID, "plan") === -2 || ent.getMetadata(PlayerID, "plan") === -3)
			return;
		if (ent.hasClass("Siege") || ent.hasClass("Support") || ent.attackTypes() === undefined)
			return;
		if (ent.hasClass("FishingBoat") || ent.hasClass("Trader"))
			return;
		if (ent.getMetadata(PlayerID, "transport") !== undefined || ent.getMetadata(PlayerID, "transporter") !== undefined)
			return;
		if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
		{
			var subrole = ent.getMetadata(PlayerID, "subrole");
			if (subrole && (subrole === "completing" || subrole === "walking" || subrole === "attacking"))
				return;
		}
		potentialDefenders.push(ent.id());
	});
	
	for (var a = 0; a < armiesNeeding.length; ++a)
		armiesNeeding[a]["army"].recalculatePosition(gameState);

	for (var i = 0; i < potentialDefenders.length; ++i)
	{
		var ent = gameState.getEntityById(potentialDefenders[i]);
		if (!ent.position())
			continue;
		var aMin = undefined;
		var distMin = undefined;
		for (var a = 0; a < armiesNeeding.length; ++a)
		{
			var dist = API3.SquareVectorDistance(ent.position(), armiesNeeding[a]["army"].foePosition);
			if (aMin !== undefined && dist > distMin)
				continue;
			aMin = a;
			distMin = dist;
		}
		if (aMin === undefined)
		{
			for (var a = 0; a < armiesNeeding.length; ++a)
				warn(" defense/armiesNeeding " + uneval(armiesNeeding[a]["need"]));
		}

		var str = m.getMaxStrength(ent);
		armiesNeeding[aMin]["need"] -= str;
		armiesNeeding[aMin]["army"].addOwn(gameState, potentialDefenders[i]);
		armiesNeeding[aMin]["army"].assignUnit(gameState, potentialDefenders[i]);

		if (armiesNeeding[aMin]["need"] <= 0)
			armiesNeeding.splice(aMin, 1);
		if (armiesNeeding.length === 0)
			break;
	}

	// If shortage of defenders: increase the priority of soldiers queues
	if (armiesNeeding.length !== 0)
		gameState.ai.HQ.boostSoldiers(gameState);
	else
		gameState.ai.HQ.unboostSoldiers(gameState);
};

// If our defense structures are attacked, garrison soldiers inside when possible
// TODO transfer most of that code in a garrisonManager
m.DefenseManager.prototype.checkDefenseStructures = function(gameState, events)
{
	var self = this;
	var attackedEvents = events["Attacked"];
	for (var key in attackedEvents)
	{
		var e = attackedEvents[key];
		var target = gameState.getEntityById(e.target);
		if (!target || !gameState.isEntityOwn(target) || !target.getArrowMultiplier())
			continue;
		if (!target.isGarrisonHolder() || gameState.ai.HQ.garrisonManager.numberOfGarrisonedUnits(target) >= target.garrisonMax())
			continue;
		if (target.hasClass("Ship"))    // TODO integrate ships later   need to be sure it is accessible
			continue;
		var attacker = gameState.getEntityById(e.attacker);
		if (!attacker)
			continue;
		var attackTypes = target.attackTypes();
		if (!attackTypes || attackTypes.indexOf("Ranged") === -1)
			continue;
		var dist = API3.SquareVectorDistance(attacker.position(), target.position());
		var range = target.attackRange("Ranged").max;
		if (dist >= range*range)
			continue;
		var index = gameState.ai.accessibility.getAccessValue(target.position());
		var garrisonManager = gameState.ai.HQ.garrisonManager;
		gameState.getOwnUnits().filter(API3.Filters.byClassesAnd(["Infantry", "Ranged"])).filterNearest(target.position()).forEach(function(ent) {
			if (garrisonManager.numberOfGarrisonedUnits(target) >= target.garrisonMax())
				return;

			if (!ent.position())
				return;
			var army = ent.getMetadata(PlayerID, "PartOfArmy");
			if (army !== undefined)
				army = self.getArmy(army);
			if (army !== undefined)
				army.removeOwn(gameState, ent.id(), ent);
			if (ent.getMetadata(PlayerID, "transport") !== undefined)
				return;
			if (ent.getMetadata(PlayerID, "plan") === -2 || ent.getMetadata(PlayerID, "plan") === -3)
				return;
			if (ent.getMetadata(PlayerID, "plan") !== undefined && ent.getMetadata(PlayerID, "plan") !== -1)
			{
				var subrole = ent.getMetadata(PlayerID, "subrole");
				if (subrole && (subrole === "completing" || subrole === "walking" || subrole === "attacking")) 
					return;
			}
			if (gameState.ai.accessibility.getAccessValue(target.position()) !== index)
				return;
			garrisonManager.garrison(gameState, ent, target, "protection");
		});
	}
};

// this processes the attackmessages
// So that a unit that gets attacked will not be completely dumb.
// warning: big levels of indentation coming.
m.DefenseManager.prototype.MessageProcess = function(gameState,events) {
/*	var self = this;
	var attackedEvents = events["Attacked"];
	for (var key in attackedEvents){
		var e = attackedEvents[key];
		if (gameState.isEntityOwn(gameState.getEntityById(e.target))) {
			var attacker = gameState.getEntityById(e.attacker);
			var ourUnit = gameState.getEntityById(e.target);
			
			// the attacker must not be already dead, and it must not be me (think catapults that miss).
			if (attacker === undefined || attacker.owner() === PlayerID || attacker.position() === undefined)
				continue;
			
			var mapPos = this.dangerMap.gamePosToMapPos(attacker.position());
			this.dangerMap.addInfluence(mapPos[0], mapPos[1], 4, 1, 'constant');

			// disregard units from attack plans and defense.
			if (ourUnit.getMetadata(PlayerID, "role") == "defense" || ourUnit.getMetadata(PlayerID, "role") == "attack")
				continue;
			
			var territory = this.territoryMap.getOwner(attacker.position());

			if (attacker.owner() == 0)
			{
				if (ourUnit !== undefined && ourUnit.hasClass("Unit") && !ourUnit.hasClass("Support"))
					ourUnit.attack(e.attacker);
				else
				{
					ourUnit.flee(attacker);
					ourUnit.setMetadata(PlayerID,"fleeing", gameState.getTimeElapsed());
				}
				if (territory === PlayerID)
				{
					// anyway we'll register the animal as dangerous, and attack it (note: only on our territory. Don't care otherwise).
					this.listOfWantedUnits[attacker.id()] = new EntityCollection(gameState.sharedScript);
					this.listOfWantedUnits[attacker.id()].addEnt(attacker);
					this.listOfWantedUnits[attacker.id()].freeze();
					this.listOfWantedUnits[attacker.id()].registerUpdates();
					
					var filter = Filters.byTargetedEntity(attacker.id());
					this.WantedUnitsAttacker[attacker.id()] = this.myUnits.filter(filter);
					this.WantedUnitsAttacker[attacker.id()].registerUpdates();
				}
			} // Disregard military units except in our territory. Disregard all calls in enemy territory.
			else if (territory == PlayerID || (territory != attacker.owner() && ourUnit.hasClass("Support")))
			{
				// TODO: this does not differentiate with buildings...
				// These ought to be treated differently.
				// units in attack plans will react independently, but we still list the attacks here.
				if (attacker.hasClass("Structure")) {
					// todo: we ultimately have to check wether it's a danger point or an isolated area, and if it's a danger point, mark it as so.
					
					// Right now, to make the AI less gameable, we'll mark any surrounding resource as inaccessible.
					// usual tower range is 80. Be on the safe side.
					var close = gameState.getResourceSupplies("wood").filter(Filters.byDistance(attacker.position(), 90));
					close.forEach(function (supply) { //}){
								  supply.setMetadata(PlayerID, "inaccessible", true);
								  });
				} else {
					// TODO: right now a soldier always retaliate... Perhaps it should be set in "Defense" mode.

					// TODO: handle the ship case
					if (attacker.hasClass("Ship"))
						continue;

					// This unit is dangerous. if it's in an army, it's being dealt with.
					// if it's not in an army, it means it's either a lone raider, or it has got friends.
					// In which case we must check for other dangerous units around, and perhaps armify them.
					// TODO: perhaps someday army detection will have improved and this will require change.
					var armyID = attacker.getMetadata(PlayerID, "inArmy");
					if (armyID == undefined || !this.enemyArmy[attacker.owner()] || !this.enemyArmy[attacker.owner()][armyID]) {
						if (this.reevaluateEntity(gameState, attacker))
						{
							var position = attacker.position();
							var close = HQ.enemyWatchers[attacker.owner()].enemySoldiers.filter(Filters.byDistance(position, self.armyCompactSize));
							
							if (close.length > 2 || ourUnit.hasClass("Support") || attacker.hasClass("Siege"))
							{
								// armify it, then armify units close to him.
								this.armify(gameState,attacker);
								armyID = attacker.getMetadata(PlayerID, "inArmy");
								
								close.forEach(function (ent) { //}){
											  if (API3.SquareVectorDistance(position, ent.position()) < self.armyCompactSize)
											  {
											  ent.setMetadata(PlayerID, "inArmy", armyID);
											  self.enemyArmy[ent.owner()][armyID].addEnt(ent);
											  }
											  });
								return;	// don't use too much processing power. If there are other cases, they'll be processed soon enough.
							}
						}
						// Defensemanager will deal with them in the next turn.
					}
					if (ourUnit !== undefined && ourUnit.hasClass("Unit")) {
						if (ourUnit.hasClass("Support")) {
							// let's try to garrison this support unit.
							if (ourUnit.position())
							{
								var buildings = gameState.getOwnEntities().filter(Filters.byCanGarrison()).filterNearest(ourUnit.position(),4).toEntityArray();
								var garrisoned = false;
								for (var i in buildings)
								{
									var struct = buildings[i];
									if (struct.garrisoned() && struct.garrisonMax() - struct.garrisoned().length > 0)
									{
										garrisoned = true;
										ourUnit.garrison(struct);
										break;
									}
								}
								if (!garrisoned) {
									ourUnit.flee(attacker);
									ourUnit.setMetadata(PlayerID,"fleeing", gameState.getTimeElapsed());
								}
							}
						} else {
							// It's a soldier. Right now we'll retaliate
							// TODO: check for stronger units against this type, check for fleeing options, etc.
							// Check also for neighboring towers and garrison there perhaps?
							ourUnit.attack(e.attacker);
						}
					}
				}
			}
		}
	}
 */
}; // nice sets of closing brackets, isn't it?

return m;
}(PETRA);
