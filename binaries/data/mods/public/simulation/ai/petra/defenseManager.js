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
	Engine.ProfileStart("Defense Manager");

	this.territoryMap = gameState.ai.HQ.territoryMap;

	this.checkEvents(gameState, events);

	this.checkEnemyArmies(gameState, events);
	this.checkEnemyUnits(gameState);
	this.assignDefenders(gameState);

	Engine.ProfileStop();
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
	if (entity.unitAIState() && entity.unitAIState() == "INDIVIDUAL.REPAIR.REPAIRING")
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
		var allyCC = gameState.getExclusiveAllyEntities().filter(API3.Filters.byClass("CivCentre"));
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
	var nbPlayers = gameState.sharedScript.playersData.length;
	var i = gameState.ai.playedTurn % nbPlayers;
	if (i === PlayerID || gameState.isPlayerAlly(i))
		return;

	var self = this;
	var filter = API3.Filters.and(API3.Filters.byClass("Unit"), API3.Filters.byOwner(i));
	var enemyUnits = gameState.updatingGlobalCollection("player-" +i + "-units", filter);

	// loop through enemy units
	enemyUnits.forEach( function (ent) {
		// first check: is this unit already part of an army.
		if (ent.getMetadata(PlayerID, "PartOfArmy") !== undefined)
			return;

		// TODO do not bother with animals for the time being
		if (ent.hasClass("Animal"))
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
		if (this.Config.personality.cooperative > 0.3)
			var bases = gameState.getAllyEntities().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
		else
			var bases = gameState.getOwnStructures().filter(API3.Filters.byClass("CivCentre")).toEntityArray();
	 	for (var i in bases)
		{
			if (API3.SquareVectorDistance(bases[i].position(), army.foePosition) < 40000)
			{
				if(this.Config.debug > 0)
					API3.warn("army in neutral territory, but still near one of our CC");
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

m.DefenseManager.prototype.assignDefenders = function(gameState)
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
				API3.warn(" defense/armiesNeeding " + uneval(armiesNeeding[a]["need"]));
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

	if (armiesNeeding.length === 0)
		return;
	// If shortage of defenders, produce ranged infantry garrisoned in nearest civil centre
	var armiesPos = [];
	for (var a = 0; a < armiesNeeding.length; ++a)
		armiesPos.push(armiesNeeding[a]["army"].foePosition);
	gameState.ai.HQ.trainEmergencyUnits(gameState, armiesPos);
};

// If our defense structures are attacked, garrison soldiers inside when possible
// and if a support unit is attacked and has less than 30% health, garrison it inside the nearest cc
m.DefenseManager.prototype.checkEvents = function(gameState, events)
{
	var self = this;
	var attackedEvents = events["Attacked"];
	for (var evt of attackedEvents)
	{
		var target = gameState.getEntityById(evt.target);
		if (!target || !gameState.isEntityOwn(target) || !target.position())
			continue;
		if (target.hasClass("Ship"))    // TODO integrate ships later   need to be sure it is accessible
			continue;

		if (target.hasClass("Support") && target.healthLevel() < 0.4)
		{
			this.garrisonUnitForHealing(gameState, target);
			continue;
		}

		var attacker = gameState.getEntityById(evt.attacker);
		if (!attacker || !attacker.position())
			continue;

		if (target.isGarrisonHolder() && target.getArrowMultiplier())
			this.garrisonRangedUnitsInside(gameState, target, attacker);
	}
};

m.DefenseManager.prototype.garrisonRangedUnitsInside = function(gameState, target, attacker)
{
	if (gameState.ai.HQ.garrisonManager.numberOfGarrisonedUnits(target) >= target.garrisonMax())
		return;
	var attackTypes = target.attackTypes();
	if (!attackTypes || attackTypes.indexOf("Ranged") === -1)
		return;
	var dist = API3.SquareVectorDistance(attacker.position(), target.position());
	var range = target.attackRange("Ranged").max;
	if (dist >= range*range)
		return;
	var index = gameState.ai.accessibility.getAccessValue(target.position());
	var garrisonManager = gameState.ai.HQ.garrisonManager;
	gameState.getOwnUnits().filter(API3.Filters.byClassesAnd(["Infantry", "Ranged"])).filterNearest(target.position()).forEach(function(ent) {
		if (garrisonManager.numberOfGarrisonedUnits(target) >= target.garrisonMax())
			return;
		if (!ent.position())
			return;
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
		if (gameState.ai.accessibility.getAccessValue(ent.position()) !== index)
			return;
		garrisonManager.garrison(gameState, ent, target, "protection");
	});
};

// garrison a hurt unit inside the nearest healing structure
m.DefenseManager.prototype.garrisonUnitForHealing = function(gameState, unit)
{
	var distmin = Math.min();
	var nearest = undefined;
	var unitAccess = gameState.ai.accessibility.getAccessValue(unit.position());
	var garrisonManager = gameState.ai.HQ.garrisonManager;
	gameState.getAllyEntities().filter(API3.Filters.byClass("Structure")).forEach(function(ent) {
		if (!ent.buffHeal())
			return;
		if (!MatchesClassList(ent.garrisonableClasses(), unit.classes()))
			return;
		if (garrisonManager.numberOfGarrisonedUnits(ent) >= ent.garrisonMax())
			return;
		var entAccess = ent.getMetadata(PlayerID, "access");
		if (!entAccess)
		{
			entAccess = gameState.ai.accessibility.getAccessValue(ent.position());
			ent.setMetadata(PlayerID, "access", entAccess);
		}
		if (entAccess !== unitAccess)
			return;
		var dist = API3.SquareVectorDistance(ent.position(), unit.position());
		if (dist > distmin)
			return;
		distmin = dist;
		nearest = ent;
	});
	if (nearest)
		garrisonManager.garrison(gameState, unit, nearest, "protection");
};

return m;
}(PETRA);
